// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  d_clisrv.c
/// \brief SRB2 Network game communication and protocol, all OS independent parts.

#include <time.h>
#ifdef __GNUC__
#include <unistd.h> //for unlink
#endif

#include "i_net.h"
#include "i_system.h"
#include "i_video.h"
#include "d_net.h"
#include "d_main.h"
#include "g_game.h"
#include "st_stuff.h"
#include "hu_stuff.h"
#include "keys.h"
#include "g_input.h" // JOY1
#include "m_menu.h"
#include "console.h"
#include "d_netfil.h"
#include "byteptr.h"
#include "p_saveg.h"
#include "z_zone.h"
#include "p_local.h"
#include "m_misc.h"
#include "am_map.h"
#include "m_random.h"
#include "mserv.h"
#include "y_inter.h"
#include "r_local.h"
#include "m_argv.h"
#include "p_setup.h"
#include "lzf.h"
#include "lua_script.h"
#include "lua_hook.h"
#include "md5.h"

#ifndef NONET
// cl loading screen
#include "v_video.h"
#include "f_finale.h"
#endif

//
// NETWORKING
//
// gametic is the tic about to (or currently being) run
// Server:
//   maketic is the tic that hasn't had control made for it yet
//   nettics is the tic for each node
//   firstticstosend is the lowest value of nettics
// Client:
//   neededtic is the tic needed by the client to run the game
//   firstticstosend is used to optimize a condition
// Normally maketic >= gametic > 0

#define PREDICTIONQUEUE BACKUPTICS
#define PREDICTIONMASK (PREDICTIONQUEUE-1)
#define MAX_REASONLENGTH 30

boolean server = true; // true or false but !server == client
#define client (!server)
boolean nodownload = false;
boolean serverrunning = false;
INT32 serverplayer = 0;
char motd[254], server_context[8]; // Message of the Day, Unique Context (even without Mumble support)

// Server specific vars
UINT8 playernode[MAXPLAYERS];
char playeraddress[MAXPLAYERS][64];

// Minimum timeout for sending the savegame
// The actual timeout will be longer depending on the savegame length
tic_t jointimeout = (10*TICRATE);
static boolean sendingsavegame[MAXNETNODES]; // Are we sending the savegame?
static tic_t freezetimeout[MAXNETNODES]; // Until when can this node freeze the server before getting a timeout?

// Incremented by cv_joindelay when a client joins, decremented each tic.
// If higher than cv_joindelay * 2 (3 joins in a short timespan), joins are temporarily disabled.
static tic_t joindelay = 0;

UINT16 pingmeasurecount = 1;
UINT32 realpingtable[MAXPLAYERS]; //the base table of ping where an average will be sent to everyone.
UINT32 playerpingtable[MAXPLAYERS]; //table of player latency values.
SINT8 nodetoplayer[MAXNETNODES];
SINT8 nodetoplayer2[MAXNETNODES]; // say the numplayer for this node if any (splitscreen)
UINT8 playerpernode[MAXNETNODES]; // used specialy for scplitscreen
boolean nodeingame[MAXNETNODES]; // set false as nodes leave game
tic_t servermaxping = 800; // server's max ping. Defaults to 800
static tic_t nettics[MAXNETNODES]; // what tic the client have received
static tic_t supposedtics[MAXNETNODES]; // nettics prevision for smaller packet
static UINT8 nodewaiting[MAXNETNODES];
static tic_t firstticstosend; // min of the nettics
static tic_t tictoclear = 0; // optimize d_clearticcmd
static tic_t maketic;

static INT16 consistancy[BACKUPTICS];

// Resynching shit!
static UINT32 resynch_score[MAXNETNODES]; // "score" for kicking -- if this gets too high then cfail kick
static UINT16 resynch_delay[MAXNETNODES]; // delay time before the player can be considered to have desynched
static UINT32 resynch_status[MAXNETNODES]; // 0 bit means synched for that player, 1 means possibly desynched
static UINT8 resynch_sent[MAXNETNODES][MAXPLAYERS]; // what synch packets have we attempted to send to the player
static UINT8 resynch_inprogress[MAXNETNODES];
static UINT8 resynch_local_inprogress = false; // WE are desynched and getting packets to fix it.
static UINT8 player_joining = false;
UINT8 hu_resynching = 0;

UINT8 adminpassmd5[16];
boolean adminpasswordset = false;

// Client specific
static ticcmd_t localcmds;
static ticcmd_t localcmds2;
static boolean cl_packetmissed;
// here it is for the secondary local player (splitscreen)
static UINT8 mynode; // my address pointofview server

static UINT8 localtextcmd[MAXTEXTCMD];
static UINT8 localtextcmd2[MAXTEXTCMD]; // splitscreen
static tic_t neededtic;
SINT8 servernode = 0; // the number of the server node
/// \brief do we accept new players?
/// \todo WORK!
boolean acceptnewnode = true;

// engine

// Must be a power of two
#define TEXTCMD_HASH_SIZE 4

typedef struct textcmdplayer_s
{
	INT32 playernum;
	UINT8 cmd[MAXTEXTCMD];
	struct textcmdplayer_s *next;
} textcmdplayer_t;

typedef struct textcmdtic_s
{
	tic_t tic;
	textcmdplayer_t *playercmds[TEXTCMD_HASH_SIZE];
	struct textcmdtic_s *next;
} textcmdtic_t;

ticcmd_t netcmds[BACKUPTICS][MAXPLAYERS];
static textcmdtic_t *textcmds[TEXTCMD_HASH_SIZE] = {NULL};


consvar_t cv_showjoinaddress = {"showjoinaddress", "On", CV_SAVE, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};

static CV_PossibleValue_t playbackspeed_cons_t[] = {{1, "MIN"}, {10, "MAX"}, {0, NULL}};
consvar_t cv_playbackspeed = {"playbackspeed", "1", 0, playbackspeed_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};

static inline void *G_DcpyTiccmd(void* dest, const ticcmd_t* src, const size_t n)
{
	const size_t d = n / sizeof(ticcmd_t);
	const size_t r = n % sizeof(ticcmd_t);
	UINT8 *ret = dest;

	if (r)
		M_Memcpy(dest, src, n);
	else if (d)
		G_MoveTiccmd(dest, src, d);
	return ret+n;
}

static inline void *G_ScpyTiccmd(ticcmd_t* dest, void* src, const size_t n)
{
	const size_t d = n / sizeof(ticcmd_t);
	const size_t r = n % sizeof(ticcmd_t);
	UINT8 *ret = src;

	if (r)
		M_Memcpy(dest, src, n);
	else if (d)
		G_MoveTiccmd(dest, src, d);
	return ret+n;
}



// Some software don't support largest packet
// (original sersetup, not exactely, but the probability of sending a packet
// of 512 bytes is like 0.1)
UINT16 software_MAXPACKETLENGTH;

/** Guesses the full value of a tic from its lowest byte, for a specific node
  *
  * \param low The lowest byte of the tic value
  * \param node The node to deduce the tic for
  * \return The full tic value
  *
  */
tic_t ExpandTics(INT32 low, INT32 node)
{
	INT32 delta;

	delta = low - (nettics[node] & UINT8_MAX);

	if (delta >= -64 && delta <= 64)
		return (nettics[node] & ~UINT8_MAX) + low;
	else if (delta > 64)
		return (nettics[node] & ~UINT8_MAX) - 256 + low;
	else //if (delta < -64)
		return (nettics[node] & ~UINT8_MAX) + 256 + low;
}

// -----------------------------------------------------------------
// Some extra data function for handle textcmd buffer
// -----------------------------------------------------------------

static void (*listnetxcmd[MAXNETXCMD])(UINT8 **p, INT32 playernum);

void RegisterNetXCmd(netxcmd_t id, void (*cmd_f)(UINT8 **p, INT32 playernum))
{
#ifdef PARANOIA
	if (id >= MAXNETXCMD)
		I_Error("Command id %d too big", id);
	if (listnetxcmd[id] != 0)
		I_Error("Command id %d already used", id);
#endif
	listnetxcmd[id] = cmd_f;
}

void SendNetXCmd(netxcmd_t id, const void *param, size_t nparam)
{
	if (localtextcmd[0]+2+nparam > MAXTEXTCMD)
	{
		// for future reference: if (cv_debug) != debug disabled.
		CONS_Alert(CONS_ERROR, M_GetText("NetXCmd buffer full, cannot add netcmd %d! (size: %d, needed: %s)\n"), id, localtextcmd[0], sizeu1(nparam));
		return;
	}
	localtextcmd[0]++;
	localtextcmd[localtextcmd[0]] = (UINT8)id;
	if (param && nparam)
	{
		M_Memcpy(&localtextcmd[localtextcmd[0]+1], param, nparam);
		localtextcmd[0] = (UINT8)(localtextcmd[0] + (UINT8)nparam);
	}
}

// splitscreen player
void SendNetXCmd2(netxcmd_t id, const void *param, size_t nparam)
{
	if (localtextcmd2[0]+2+nparam > MAXTEXTCMD)
	{
		I_Error("No more place in the buffer for netcmd %d\n",id);
		return;
	}
	localtextcmd2[0]++;
	localtextcmd2[localtextcmd2[0]] = (UINT8)id;
	if (param && nparam)
	{
		M_Memcpy(&localtextcmd2[localtextcmd2[0]+1], param, nparam);
		localtextcmd2[0] = (UINT8)(localtextcmd2[0] + (UINT8)nparam);
	}
}

UINT8 GetFreeXCmdSize(void)
{
	// -1 for the size and another -1 for the ID.
	return (UINT8)(localtextcmd[0] - 2);
}

// Frees all textcmd memory for the specified tic
static void D_FreeTextcmd(tic_t tic)
{
	textcmdtic_t **tctprev = &textcmds[tic & (TEXTCMD_HASH_SIZE - 1)];
	textcmdtic_t *textcmdtic = *tctprev;

	while (textcmdtic && textcmdtic->tic != tic)
	{
		tctprev = &textcmdtic->next;
		textcmdtic = textcmdtic->next;
	}

	if (textcmdtic)
	{
		INT32 i;

		// Remove this tic from the list.
		*tctprev = textcmdtic->next;

		// Free all players.
		for (i = 0; i < TEXTCMD_HASH_SIZE; i++)
		{
			textcmdplayer_t *textcmdplayer = textcmdtic->playercmds[i];

			while (textcmdplayer)
			{
				textcmdplayer_t *tcpnext = textcmdplayer->next;
				Z_Free(textcmdplayer);
				textcmdplayer = tcpnext;
			}
		}

		// Free this tic's own memory.
		Z_Free(textcmdtic);
	}
}

// Gets the buffer for the specified ticcmd, or NULL if there isn't one
static UINT8* D_GetExistingTextcmd(tic_t tic, INT32 playernum)
{
	textcmdtic_t *textcmdtic = textcmds[tic & (TEXTCMD_HASH_SIZE - 1)];
	while (textcmdtic && textcmdtic->tic != tic) textcmdtic = textcmdtic->next;

	// Do we have an entry for the tic? If so, look for player.
	if (textcmdtic)
	{
		textcmdplayer_t *textcmdplayer = textcmdtic->playercmds[playernum & (TEXTCMD_HASH_SIZE - 1)];
		while (textcmdplayer && textcmdplayer->playernum != playernum) textcmdplayer = textcmdplayer->next;

		if (textcmdplayer) return textcmdplayer->cmd;
	}

	return NULL;
}

// Gets the buffer for the specified ticcmd, creating one if necessary
static UINT8* D_GetTextcmd(tic_t tic, INT32 playernum)
{
	textcmdtic_t *textcmdtic = textcmds[tic & (TEXTCMD_HASH_SIZE - 1)];
	textcmdtic_t **tctprev = &textcmds[tic & (TEXTCMD_HASH_SIZE - 1)];
	textcmdplayer_t *textcmdplayer, **tcpprev;

	// Look for the tic.
	while (textcmdtic && textcmdtic->tic != tic)
	{
		tctprev = &textcmdtic->next;
		textcmdtic = textcmdtic->next;
	}

	// If we don't have an entry for the tic, make it.
	if (!textcmdtic)
	{
		textcmdtic = *tctprev = Z_Calloc(sizeof (textcmdtic_t), PU_STATIC, NULL);
		textcmdtic->tic = tic;
	}

	tcpprev = &textcmdtic->playercmds[playernum & (TEXTCMD_HASH_SIZE - 1)];
	textcmdplayer = *tcpprev;

	// Look for the player.
	while (textcmdplayer && textcmdplayer->playernum != playernum)
	{
		tcpprev = &textcmdplayer->next;
		textcmdplayer = textcmdplayer->next;
	}

	// If we don't have an entry for the player, make it.
	if (!textcmdplayer)
	{
		textcmdplayer = *tcpprev = Z_Calloc(sizeof (textcmdplayer_t), PU_STATIC, NULL);
		textcmdplayer->playernum = playernum;
	}

	return textcmdplayer->cmd;
}

static void ExtraDataTicker(void)
{
	INT32 i;

	for (i = 0; i < MAXPLAYERS; i++)
		if (playeringame[i] || i == 0)
		{
			UINT8 *bufferstart = D_GetExistingTextcmd(gametic, i);

			if (bufferstart)
			{
				UINT8 *curpos = bufferstart;
				UINT8 *bufferend = &curpos[curpos[0]+1];

				curpos++;
				while (curpos < bufferend)
				{
					if (*curpos < MAXNETXCMD && listnetxcmd[*curpos])
					{
						const UINT8 id = *curpos;
						curpos++;
						DEBFILE(va("executing x_cmd %s ply %u ", netxcmdnames[id - 1], i));
						(listnetxcmd[id])(&curpos, i);
						DEBFILE("done\n");
					}
					else
					{
						if (server)
						{
							SendKick(i, KICK_MSG_CON_FAIL | KICK_MSG_KEEP_BODY);
							DEBFILE(va("player %d kicked [gametic=%u] reason as follows:\n", i, gametic));
						}
						CONS_Alert(CONS_WARNING, M_GetText("Got unknown net command [%s]=%d (max %d)\n"), sizeu1(curpos - bufferstart), *curpos, bufferstart[0]);
						break;
					}
				}
			}
		}

	// If you are a client, you can safely forget the net commands for this tic
	// If you are the server, you need to remember them until every client has been acknowledged,
	// because if you need to resend a PT_SERVERTICS packet, you will need to put the commands in it
	if (client)
		D_FreeTextcmd(gametic);
}

static void D_Clearticcmd(tic_t tic)
{
	INT32 i;

	D_FreeTextcmd(tic);

	for (i = 0; i < MAXPLAYERS; i++)
		netcmds[tic%BACKUPTICS][i].angleturn = 0;

	DEBFILE(va("clear tic %5u (%2u)\n", tic, tic%BACKUPTICS));
}

void D_ResetTiccmds(void)
{
	INT32 i;

	memset(&localcmds, 0, sizeof(ticcmd_t));
	memset(&localcmds2, 0, sizeof(ticcmd_t));

	// Reset the net command list
	for (i = 0; i < TEXTCMD_HASH_SIZE; i++)
		while (textcmds[i])
			D_Clearticcmd(textcmds[i]->tic);
}

void SendKick(UINT8 playernum, UINT8 msg)
{
	UINT8 buf[2];

	if (!(server && cv_rejointimeout.value))
		msg &= ~KICK_MSG_KEEP_BODY;

	buf[0] = playernum;
	buf[1] = msg;
	SendNetXCmd(XD_KICK, &buf, 2);
}

// -----------------------------------------------------------------
// end of extra data function
// -----------------------------------------------------------------

// -----------------------------------------------------------------
// extra data function for lmps
// -----------------------------------------------------------------

// if extradatabit is set, after the ziped tic you find this:
//
//   type   |  description
// ---------+--------------
//   byte   | size of the extradata
//   byte   | the extradata (xd) bits: see XD_...
//            with this byte you know what parameter folow
// if (xd & XDNAMEANDCOLOR)
//   byte   | color
//   char[MAXPLAYERNAME] | name of the player
// endif
// if (xd & XD_WEAPON_PREF)
//   byte   | original weapon switch: boolean, true if use the old
//          | weapon switch methode
//   char[NUMWEAPONS] | the weapon switch priority
//   byte   | autoaim: true if use the old autoaim system
// endif
/*boolean AddLmpExtradata(UINT8 **demo_point, INT32 playernum)
{
	UINT8 *textcmd = D_GetExistingTextcmd(gametic, playernum);

	if (!textcmd)
		return false;

	M_Memcpy(*demo_point, textcmd, textcmd[0]+1);
	*demo_point += textcmd[0]+1;
	return true;
}

void ReadLmpExtraData(UINT8 **demo_pointer, INT32 playernum)
{
	UINT8 nextra;
	UINT8 *textcmd;

	if (!demo_pointer)
		return;

	textcmd = D_GetTextcmd(gametic, playernum);
	nextra = **demo_pointer;
	M_Memcpy(textcmd, *demo_pointer, nextra + 1);
	// increment demo pointer
	*demo_pointer += nextra + 1;
}*/

// -----------------------------------------------------------------
// end extra data function for lmps
// -----------------------------------------------------------------

// -----------------------------------------------------------------
// resynch player data
// -----------------------------------------------------------------
static inline void resynch_write_player(resynch_pak *rsp, const size_t i)
{
	size_t j;

	rsp->playernum = (UINT8)i;

	// Do not send anything visual related.
	// Only send data that we need to know for physics.
	rsp->playerstate = (UINT8)players[i].playerstate; //playerstate_t
	rsp->pflags = (UINT32)LONG(players[i].pflags); //pflags_t
	rsp->panim  = (UINT8)players[i].panim; //panim_t

	rsp->angleturn = (INT16)SHORT(players[i].angleturn);
	rsp->oldrelangleturn = (INT16)SHORT(players[i].oldrelangleturn);

	rsp->aiming = (angle_t)LONG(players[i].aiming);
	rsp->currentweapon = LONG(players[i].currentweapon);
	rsp->ringweapons = LONG(players[i].ringweapons);

	rsp->ammoremoval = (UINT16)SHORT(players[i].ammoremoval);
	rsp->ammoremovaltimer = (tic_t)LONG(players[i].ammoremovaltimer);
	rsp->ammoremovalweapon = LONG(players[i].ammoremovalweapon);

	for (j = 0; j < NUMPOWERS; ++j)
		rsp->powers[j] = (UINT16)SHORT(players[i].powers[j]);

	// Score is resynched in the rspfirm resync packet
	rsp->rings = SHORT(players[i].rings);
	rsp->spheres = SHORT(players[i].spheres);
	rsp->lives = players[i].lives;
	rsp->continues = players[i].continues;
	rsp->scoreadd = players[i].scoreadd;
	rsp->xtralife = players[i].xtralife;
	rsp->pity = players[i].pity;

	rsp->skincolor = players[i].skincolor;
	rsp->skin = LONG(players[i].skin);
	rsp->availabilities = LONG(players[i].availabilities);
	// Just in case Lua does something like
	// modify these at runtime
	rsp->camerascale = (fixed_t)LONG(players[i].camerascale);
	rsp->shieldscale = (fixed_t)LONG(players[i].shieldscale);
	rsp->normalspeed = (fixed_t)LONG(players[i].normalspeed);
	rsp->runspeed = (fixed_t)LONG(players[i].runspeed);
	rsp->thrustfactor = players[i].thrustfactor;
	rsp->accelstart = players[i].accelstart;
	rsp->acceleration = players[i].acceleration;
	rsp->charability = players[i].charability;
	rsp->charability2 = players[i].charability2;
	rsp->charflags = (UINT32)LONG(players[i].charflags);
	rsp->thokitem = (UINT32)LONG(players[i].thokitem); //mobjtype_t
	rsp->spinitem = (UINT32)LONG(players[i].spinitem); //mobjtype_t
	rsp->revitem = (UINT32)LONG(players[i].revitem); //mobjtype_t
	rsp->followitem = (UINT32)LONG(players[i].followitem); //mobjtype_t
	rsp->actionspd = (fixed_t)LONG(players[i].actionspd);
	rsp->mindash = (fixed_t)LONG(players[i].mindash);
	rsp->maxdash = (fixed_t)LONG(players[i].maxdash);
	rsp->jumpfactor = (fixed_t)LONG(players[i].jumpfactor);
	rsp->playerheight = (fixed_t)LONG(players[i].height);
	rsp->playerspinheight = (fixed_t)LONG(players[i].spinheight);

	rsp->speed = (fixed_t)LONG(players[i].speed);
	rsp->secondjump = players[i].secondjump;
	rsp->fly1 = players[i].fly1;
	rsp->glidetime = (tic_t)LONG(players[i].glidetime);
	rsp->climbing = players[i].climbing;
	rsp->deadtimer = players[i].deadtimer;
	rsp->exiting = (tic_t)LONG(players[i].exiting);
	rsp->homing = players[i].homing;
	rsp->dashmode = (tic_t)LONG(players[i].dashmode);
	rsp->skidtime = (tic_t)LONG(players[i].skidtime);
	rsp->cmomx = (fixed_t)LONG(players[i].cmomx);
	rsp->cmomy = (fixed_t)LONG(players[i].cmomy);
	rsp->rmomx = (fixed_t)LONG(players[i].rmomx);
	rsp->rmomy = (fixed_t)LONG(players[i].rmomy);

	rsp->weapondelay = LONG(players[i].weapondelay);
	rsp->tossdelay = LONG(players[i].tossdelay);

	rsp->starpostx = SHORT(players[i].starpostx);
	rsp->starposty = SHORT(players[i].starposty);
	rsp->starpostz = SHORT(players[i].starpostz);
	rsp->starpostnum = LONG(players[i].starpostnum);
	rsp->starposttime = (tic_t)LONG(players[i].starposttime);
	rsp->starpostangle = (angle_t)LONG(players[i].starpostangle);
	rsp->starpostscale = (fixed_t)LONG(players[i].starpostscale);

	rsp->maxlink = LONG(players[i].maxlink);
	rsp->dashspeed = (fixed_t)LONG(players[i].dashspeed);
	rsp->angle_pos = (angle_t)LONG(players[i].angle_pos);
	rsp->old_angle_pos = (angle_t)LONG(players[i].old_angle_pos);
	rsp->bumpertime = (tic_t)LONG(players[i].bumpertime);
	rsp->flyangle = LONG(players[i].flyangle);
	rsp->drilltimer = (tic_t)LONG(players[i].drilltimer);
	rsp->linkcount = LONG(players[i].linkcount);
	rsp->linktimer = (tic_t)LONG(players[i].linktimer);
	rsp->anotherflyangle = LONG(players[i].anotherflyangle);
	rsp->nightstime = (tic_t)LONG(players[i].nightstime);
	rsp->drillmeter = LONG(players[i].drillmeter);
	rsp->drilldelay = players[i].drilldelay;
	rsp->bonustime = players[i].bonustime;
	rsp->mare = players[i].mare;
	rsp->lastsidehit = SHORT(players[i].lastsidehit);
	rsp->lastlinehit = SHORT(players[i].lastlinehit);

	rsp->losstime = (tic_t)LONG(players[i].losstime);
	rsp->timeshit = players[i].timeshit;
	rsp->onconveyor = LONG(players[i].onconveyor);

	rsp->hasmo = false;
	//Transfer important mo information if the player has a body.
	//This lets us resync players even if they are dead.
	if (!players[i].mo)
		return;
	rsp->hasmo = true;

	rsp->health = LONG(players[i].mo->health);
	rsp->angle = (angle_t)LONG(players[i].mo->angle);
	rsp->rollangle = (angle_t)LONG(players[i].mo->rollangle);
	rsp->x = LONG(players[i].mo->x);
	rsp->y = LONG(players[i].mo->y);
	rsp->z = LONG(players[i].mo->z);
	rsp->momx = LONG(players[i].mo->momx);
	rsp->momy = LONG(players[i].mo->momy);
	rsp->momz = LONG(players[i].mo->momz);
	rsp->friction = LONG(players[i].mo->friction);
	rsp->movefactor = LONG(players[i].mo->movefactor);

	rsp->sprite = (spritenum_t)LONG(players[i].mo->sprite);
	rsp->frame = LONG(players[i].mo->frame);
	rsp->sprite2 = players[i].mo->sprite2;
	rsp->anim_duration = SHORT(players[i].mo->anim_duration);
	rsp->tics = LONG(players[i].mo->tics);
	rsp->statenum = (statenum_t)LONG(players[i].mo->state-states); // :(
	rsp->eflags = (UINT16)SHORT(players[i].mo->eflags);
	rsp->flags = LONG(players[i].mo->flags);
	rsp->flags2 = LONG(players[i].mo->flags2);

	rsp->radius = LONG(players[i].mo->radius);
	rsp->height = LONG(players[i].mo->height);
	rsp->scale = LONG(players[i].mo->scale);
	rsp->destscale = LONG(players[i].mo->destscale);
	rsp->scalespeed = LONG(players[i].mo->scalespeed);
}

static void resynch_read_player(resynch_pak *rsp)
{
	INT32 i = rsp->playernum, j;
	mobj_t *savedmo = players[i].mo;

	// Do not send anything visual related.
	// Only send data that we need to know for physics.
	players[i].playerstate = (UINT8)rsp->playerstate; //playerstate_t
	players[i].pflags = (UINT32)LONG(rsp->pflags); //pflags_t
	players[i].panim  = (UINT8)rsp->panim; //panim_t

	players[i].angleturn = (INT16)SHORT(rsp->angleturn);
	players[i].oldrelangleturn = (INT16)SHORT(rsp->oldrelangleturn);

	players[i].aiming = (angle_t)LONG(rsp->aiming);
	players[i].currentweapon = LONG(rsp->currentweapon);
	players[i].ringweapons = LONG(rsp->ringweapons);

	players[i].ammoremoval = (UINT16)SHORT(rsp->ammoremoval);
	players[i].ammoremovaltimer = (tic_t)LONG(rsp->ammoremovaltimer);
	players[i].ammoremovalweapon = LONG(rsp->ammoremovalweapon);

	for (j = 0; j < NUMPOWERS; ++j)
		players[i].powers[j] = (UINT16)SHORT(rsp->powers[j]);

	// Score is resynched in the rspfirm resync packet
	players[i].rings = SHORT(rsp->rings);
	players[i].spheres = SHORT(rsp->spheres);
	players[i].lives = rsp->lives;
	players[i].continues = rsp->continues;
	players[i].scoreadd = rsp->scoreadd;
	players[i].xtralife = rsp->xtralife;
	players[i].pity = rsp->pity;

	players[i].skincolor = rsp->skincolor;
	players[i].skin = LONG(rsp->skin);
	players[i].availabilities = LONG(rsp->availabilities);
	// Just in case Lua does something like
	// modify these at runtime
	players[i].camerascale = (fixed_t)LONG(rsp->camerascale);
	players[i].shieldscale = (fixed_t)LONG(rsp->shieldscale);
	players[i].normalspeed = (fixed_t)LONG(rsp->normalspeed);
	players[i].runspeed = (fixed_t)LONG(rsp->runspeed);
	players[i].thrustfactor = rsp->thrustfactor;
	players[i].accelstart = rsp->accelstart;
	players[i].acceleration = rsp->acceleration;
	players[i].charability = rsp->charability;
	players[i].charability2 = rsp->charability2;
	players[i].charflags = (UINT32)LONG(rsp->charflags);
	players[i].thokitem = (UINT32)LONG(rsp->thokitem); //mobjtype_t
	players[i].spinitem = (UINT32)LONG(rsp->spinitem); //mobjtype_t
	players[i].revitem = (UINT32)LONG(rsp->revitem); //mobjtype_t
	players[i].followitem = (UINT32)LONG(rsp->followitem); //mobjtype_t
	players[i].actionspd = (fixed_t)LONG(rsp->actionspd);
	players[i].mindash = (fixed_t)LONG(rsp->mindash);
	players[i].maxdash = (fixed_t)LONG(rsp->maxdash);
	players[i].jumpfactor = (fixed_t)LONG(rsp->jumpfactor);
	players[i].height = (fixed_t)LONG(rsp->playerheight);
	players[i].spinheight = (fixed_t)LONG(rsp->playerspinheight);

	players[i].speed = (fixed_t)LONG(rsp->speed);
	players[i].secondjump = rsp->secondjump;
	players[i].fly1 = rsp->fly1;
	players[i].glidetime = (tic_t)LONG(rsp->glidetime);
	players[i].climbing = rsp->climbing;
	players[i].deadtimer = rsp->deadtimer;
	players[i].exiting = (tic_t)LONG(rsp->exiting);
	players[i].homing = rsp->homing;
	players[i].dashmode = (tic_t)LONG(rsp->dashmode);
	players[i].skidtime = (tic_t)LONG(rsp->skidtime);
	players[i].cmomx = (fixed_t)LONG(rsp->cmomx);
	players[i].cmomy = (fixed_t)LONG(rsp->cmomy);
	players[i].rmomx = (fixed_t)LONG(rsp->rmomx);
	players[i].rmomy = (fixed_t)LONG(rsp->rmomy);

	players[i].weapondelay = LONG(rsp->weapondelay);
	players[i].tossdelay = LONG(rsp->tossdelay);

	players[i].starpostx = SHORT(rsp->starpostx);
	players[i].starposty = SHORT(rsp->starposty);
	players[i].starpostz = SHORT(rsp->starpostz);
	players[i].starpostnum = LONG(rsp->starpostnum);
	players[i].starposttime = (tic_t)LONG(rsp->starposttime);
	players[i].starpostangle = (angle_t)LONG(rsp->starpostangle);
	players[i].starpostscale = (fixed_t)LONG(rsp->starpostscale);

	players[i].maxlink = LONG(rsp->maxlink);
	players[i].dashspeed = (fixed_t)LONG(rsp->dashspeed);
	players[i].angle_pos = (angle_t)LONG(rsp->angle_pos);
	players[i].old_angle_pos = (angle_t)LONG(rsp->old_angle_pos);
	players[i].bumpertime = (tic_t)LONG(rsp->bumpertime);
	players[i].flyangle = LONG(rsp->flyangle);
	players[i].drilltimer = (tic_t)LONG(rsp->drilltimer);
	players[i].linkcount = LONG(rsp->linkcount);
	players[i].linktimer = (tic_t)LONG(rsp->linktimer);
	players[i].anotherflyangle = LONG(rsp->anotherflyangle);
	players[i].nightstime = (tic_t)LONG(rsp->nightstime);
	players[i].drillmeter = LONG(rsp->drillmeter);
	players[i].drilldelay = rsp->drilldelay;
	players[i].bonustime = rsp->bonustime;
	players[i].mare = rsp->mare;
	players[i].lastsidehit = SHORT(rsp->lastsidehit);
	players[i].lastlinehit = SHORT(rsp->lastlinehit);

	players[i].losstime = (tic_t)LONG(rsp->losstime);
	players[i].timeshit = rsp->timeshit;
	players[i].onconveyor = LONG(rsp->onconveyor);

	//We get a packet for each player in game.
	if (!playeringame[i])
		return;

	//...but keep old mo even if it is corrupt or null!
	players[i].mo = savedmo;

	//Transfer important mo information if they have a valid mo.
	if (!rsp->hasmo)
		return;

	//server thinks player has a body.
	//Give them a new body that can be then manipulated by the server's info.
	if (!players[i].mo) //client thinks it has no body.
		P_SpawnPlayer(i);

	//At this point, the player should have a body, whether they were respawned or not.
	P_UnsetThingPosition(players[i].mo);
	players[i].mo->angle = (angle_t)LONG(rsp->angle);
	players[i].mo->rollangle = (angle_t)LONG(rsp->rollangle);
	players[i].mo->eflags = (UINT16)SHORT(rsp->eflags);
	players[i].mo->flags = LONG(rsp->flags);
	players[i].mo->flags2 = LONG(rsp->flags2);
	players[i].mo->friction = LONG(rsp->friction);
	players[i].mo->health = LONG(rsp->health);
	players[i].mo->momx = LONG(rsp->momx);
	players[i].mo->momy = LONG(rsp->momy);
	players[i].mo->momz = LONG(rsp->momz);
	players[i].mo->movefactor = LONG(rsp->movefactor);

	// Don't use P_SetMobjStateNF to restore state, write/read all the values manually!
	// This should stop those stupid console errors, hopefully.
	// -- Monster Iestyn
	players[i].mo->sprite = (spritenum_t)LONG(rsp->sprite);
	players[i].mo->frame = LONG(rsp->frame);
	players[i].mo->sprite2 = rsp->sprite2;
	players[i].mo->anim_duration = SHORT(rsp->anim_duration);
	players[i].mo->tics = LONG(rsp->tics);
	players[i].mo->state = &states[LONG(rsp->statenum)];

	players[i].mo->x = LONG(rsp->x);
	players[i].mo->y = LONG(rsp->y);
	players[i].mo->z = LONG(rsp->z);
	players[i].mo->radius = LONG(rsp->radius);
	players[i].mo->height = LONG(rsp->height);
	// P_SetScale is redundant for this, as all related variables are already restored properly.
	players[i].mo->scale = LONG(rsp->scale);
	players[i].mo->destscale = LONG(rsp->destscale);
	players[i].mo->scalespeed = LONG(rsp->scalespeed);

	// And finally, SET THE MOBJ SKIN damn it.
	if ((players[i].powers[pw_carry] == CR_NIGHTSMODE) && (skins[players[i].skin].sprites[SPR2_NFLY].numframes == 0))
	{
		players[i].mo->skin = &skins[DEFAULTNIGHTSSKIN];
		players[i].mo->color = skins[DEFAULTNIGHTSSKIN].prefcolor; // this will be corrected by thinker to super flash
	}
	else
	{
		players[i].mo->skin = &skins[players[i].skin];
		players[i].mo->color = players[i].skincolor; // this will be corrected by thinker to super flash/mario star
	}

	P_SetThingPosition(players[i].mo);
}

static inline void resynch_write_ctf(resynchend_pak *rst)
{
	mobj_t *mflag;
	UINT8 i, j;

	for (i = 0, mflag = redflag; i < 2; ++i, mflag = blueflag)
	{
		rst->flagx[i] = rst->flagy[i] = rst->flagz[i] = 0;
		rst->flagloose[i] = rst->flagflags[i] = 0;
		rst->flagplayer[i] = -1;

		if (!mflag)
		{
			// Should be held by a player
			for (j = 0; j < MAXPLAYERS; ++j)
			{
				// GF_REDFLAG is 1, GF_BLUEFLAG is 2
				// redflag handling is i=0, blueflag is i=1
				// so check for gotflag == (i+1)
				if (!playeringame[j] || players[j].gotflag != (i+1))
					continue;
				rst->flagplayer[i] = (SINT8)j;
				break;
			}
			if (j == MAXPLAYERS) // fine, no I_Error
			{
				CONS_Alert(CONS_ERROR, "One of the flags has gone completely missing...\n");
				rst->flagplayer[i] = -2;
			}
			continue;
		}

		rst->flagx[i] = (fixed_t)LONG(mflag->x);
		rst->flagy[i] = (fixed_t)LONG(mflag->y);
		rst->flagz[i] = (fixed_t)LONG(mflag->z);
		rst->flagflags[i] = LONG(mflag->flags2);
		rst->flagloose[i] = LONG(mflag->fuse); // Dropped or not?
	}
}

static inline void resynch_read_ctf(resynchend_pak *p)
{
	UINT8 i;

	for (i = 0; i < MAXPLAYERS; ++i)
		players[i].gotflag = 0;

	// Red flag
	if (p->flagplayer[0] == -2)
		; // The server doesn't even know what happened to it...
	else if (p->flagplayer[0] != -1) // Held by a player
	{
		if (!playeringame[p->flagplayer[0]])
			 I_Error("Invalid red flag player %d who isn't in the game!", (INT32)p->flagplayer[0]);
		players[p->flagplayer[0]].gotflag = GF_REDFLAG;
		if (redflag)
		{
			P_RemoveMobj(redflag);
			redflag = NULL;
		}
	}
	else
	{
		if (!redflag)
			redflag = P_SpawnMobj(0,0,0,MT_REDFLAG);

		P_UnsetThingPosition(redflag);
		redflag->x = (fixed_t)LONG(p->flagx[0]);
		redflag->y = (fixed_t)LONG(p->flagy[0]);
		redflag->z = (fixed_t)LONG(p->flagz[0]);
		redflag->flags2 = LONG(p->flagflags[0]);
		redflag->fuse = LONG(p->flagloose[0]);
		P_SetThingPosition(redflag);
	}

	// Blue flag
	if (p->flagplayer[1] == -2)
		; // The server doesn't even know what happened to it...
	else if (p->flagplayer[1] != -1) // Held by a player
	{
		if (!playeringame[p->flagplayer[1]])
			 I_Error("Invalid blue flag player %d who isn't in the game!", (INT32)p->flagplayer[1]);
		players[p->flagplayer[1]].gotflag = GF_BLUEFLAG;
		if (blueflag)
		{
			P_RemoveMobj(blueflag);
			blueflag = NULL;
		}
	}
	else
	{
		if (!blueflag)
			blueflag = P_SpawnMobj(0,0,0,MT_BLUEFLAG);

		P_UnsetThingPosition(blueflag);
		blueflag->x = (fixed_t)LONG(p->flagx[1]);
		blueflag->y = (fixed_t)LONG(p->flagy[1]);
		blueflag->z = (fixed_t)LONG(p->flagz[1]);
		blueflag->flags2 = LONG(p->flagflags[1]);
		blueflag->fuse = LONG(p->flagloose[1]);
		P_SetThingPosition(blueflag);
	}
}

static inline void resynch_write_others(resynchend_pak *rst)
{
	UINT8 i;

	rst->ingame = 0;
	rst->outofcoop = 0;

	for (i = 0; i < MAXPLAYERS; ++i)
	{
		if (!playeringame[i])
		{
			rst->ctfteam[i] = 0;
			rst->score[i] = 0;
			rst->numboxes[i] = 0;
			rst->totalring[i] = 0;
			rst->realtime[i] = 0;
			rst->laps[i] = 0;
			continue;
		}

		if (!players[i].spectator)
			rst->ingame |= (1<<i);
		if (players[i].outofcoop)
			rst->outofcoop |= (1<<i);
		rst->ctfteam[i] = (INT32)LONG(players[i].ctfteam);
		rst->score[i] = (UINT32)LONG(players[i].score);
		rst->numboxes[i] = SHORT(players[i].numboxes);
		rst->totalring[i] = SHORT(players[i].totalring);
		rst->realtime[i] = (tic_t)LONG(players[i].realtime);
		rst->laps[i] = players[i].laps;
	}

	// endian safeness
	rst->ingame = (UINT32)LONG(rst->ingame);
}

static inline void resynch_read_others(resynchend_pak *p)
{
	UINT8 i;
	UINT32 loc_ingame = (UINT32)LONG(p->ingame);
	UINT32 loc_outofcoop = (UINT32)LONG(p->outofcoop);

	for (i = 0; i < MAXPLAYERS; ++i)
	{
		// We don't care if they're in the game or not, just write all the data.
		players[i].spectator = !(loc_ingame & (1<<i));
		players[i].outofcoop = (loc_outofcoop & (1<<i));
		players[i].ctfteam = (INT32)LONG(p->ctfteam[i]); // no, 0 does not mean spectator, at least not in Match
		players[i].score = (UINT32)LONG(p->score[i]);
		players[i].numboxes = SHORT(p->numboxes[i]);
		players[i].totalring = SHORT(p->totalring[i]);
		players[i].realtime = (tic_t)LONG(p->realtime[i]);
		players[i].laps = p->laps[i];
	}
}

static void SV_InitResynchVars(INT32 node)
{
	resynch_delay[node] = TICRATE; // initial one second delay
	resynch_score[node] = 0; // clean slate
	resynch_status[node] = 0x00;
	resynch_inprogress[node] = false;
	memset(resynch_sent[node], 0, MAXPLAYERS);
}

static void SV_RequireResynch(INT32 node)
{
	INT32 i;

	resynch_delay[node] = 10; // Delay before you can fail sync again
	resynch_score[node] += 200; // Add score for initial desynch
	resynch_status[node] = 0xFFFFFFFF; // No players assumed synched
	resynch_inprogress[node] = true; // so we know to send a PT_RESYNCHEND after sync

	// Initial setup
	memset(resynch_sent[node], 0, MAXPLAYERS);
	for (i = 0; i < MAXPLAYERS; ++i)
	{
		if (!playeringame[i]) // Player not in game so just drop it from required synch
			resynch_status[node] &= ~(1<<i);
		else if (playernode[i] == node); // instantly update THEIR position
		else // Send at random times based on num players
			resynch_sent[node][i] = M_RandomKey(D_NumPlayers()>>1)+1;
	}
}

static void SV_SendResynch(INT32 node)
{
	INT32 i, j;

	if (!nodeingame[node])
	{
		// player left during resynch
		// so obviously we don't need to do any of this anymore
		resynch_inprogress[node] = false;
		return;
	}

	// resynched?
	if (!resynch_status[node])
	{
		// you are now synched
		resynch_inprogress[node] = false;

		netbuffer->packettype = PT_RESYNCHEND;

		netbuffer->u.resynchend.randomseed = P_GetRandSeed();
		if (gametyperules & GTR_TEAMFLAGS)
			resynch_write_ctf(&netbuffer->u.resynchend);
		resynch_write_others(&netbuffer->u.resynchend);

		HSendPacket(node, true, 0, (sizeof(resynchend_pak)));
		return;
	}

	netbuffer->packettype = PT_RESYNCHING;
	for (i = 0, j = 0; i < MAXPLAYERS; ++i)
	{
		// if already synched don't bother
		if (!(resynch_status[node] & 1<<i))
			continue;

		// waiting for a reply or just waiting in general
		if (resynch_sent[node][i])
		{
			--resynch_sent[node][i];
			continue;
		}

		resynch_write_player(&netbuffer->u.resynchpak, i);
		HSendPacket(node, false, 0, (sizeof(resynch_pak)));

		resynch_sent[node][i] = TICRATE;
		resynch_score[node] += 2; // penalty for send

		if (++j > 3)
			break;
	}

	if (resynch_score[node] > (unsigned)cv_resynchattempts.value*250)
	{
		SendKick(nodetoplayer[node], KICK_MSG_CON_FAIL | KICK_MSG_KEEP_BODY);
		resynch_score[node] = 0;
	}
}

static void CL_AcknowledgeResynch(resynch_pak *rsp)
{
	resynch_read_player(rsp);

	netbuffer->packettype = PT_RESYNCHGET;
	netbuffer->u.resynchgot = rsp->playernum;
	HSendPacket(servernode, true, 0, sizeof(UINT8));
}

static void SV_AcknowledgeResynchAck(INT32 node, UINT8 rsg)
{
	if (rsg >= MAXPLAYERS)
		resynch_score[node] += 16384; // lol.
	else
	{
		resynch_status[node] &= ~(1<<rsg);
		--resynch_score[node]; // unpenalize
	}

	// Don't let resynch cause a timeout
	freezetimeout[node] = I_GetTime() + connectiontimeout;
}
// -----------------------------------------------------------------
// end resynch
// -----------------------------------------------------------------

static INT16 Consistancy(void);

typedef enum
{
	CL_SEARCHING,
	CL_DOWNLOADFILES,
	CL_ASKJOIN,
	CL_WAITJOINRESPONSE,
	CL_DOWNLOADSAVEGAME,
	CL_CONNECTED,
	CL_ABORTED
} cl_mode_t;

static void GetPackets(void);

static cl_mode_t cl_mode = CL_SEARCHING;

// Player name send/load

static void CV_SavePlayerNames(UINT8 **p)
{
	INT32 i = 0;
	// Players in game only.
	for (; i < MAXPLAYERS; ++i)
	{
		if (!playeringame[i])
		{
			WRITEUINT8(*p, 0);
			continue;
		}
		WRITESTRING(*p, player_names[i]);
	}
}

static void CV_LoadPlayerNames(UINT8 **p)
{
	INT32 i = 0;
	char tmp_name[MAXPLAYERNAME+1];
	tmp_name[MAXPLAYERNAME] = 0;

	for (; i < MAXPLAYERS; ++i)
	{
		READSTRING(*p, tmp_name);
		if (tmp_name[0] == 0)
			continue;
		if (tmp_name[MAXPLAYERNAME]) // overflow detected
			I_Error("Received bad server config packet when trying to join");
		memcpy(player_names[i], tmp_name, MAXPLAYERNAME+1);
	}
}

#ifndef NONET
#define SNAKE_SPEED 5

#define SNAKE_NUM_BLOCKS_X 20
#define SNAKE_NUM_BLOCKS_Y 10
#define SNAKE_BLOCK_SIZE 12
#define SNAKE_BORDER_SIZE 12

#define SNAKE_MAP_WIDTH  (SNAKE_NUM_BLOCKS_X * SNAKE_BLOCK_SIZE)
#define SNAKE_MAP_HEIGHT (SNAKE_NUM_BLOCKS_Y * SNAKE_BLOCK_SIZE)

#define SNAKE_LEFT_X ((BASEVIDWIDTH - SNAKE_MAP_WIDTH) / 2 - SNAKE_BORDER_SIZE)
#define SNAKE_RIGHT_X (SNAKE_LEFT_X + SNAKE_MAP_WIDTH + SNAKE_BORDER_SIZE * 2 - 1)
#define SNAKE_BOTTOM_Y (BASEVIDHEIGHT - 48)
#define SNAKE_TOP_Y (SNAKE_BOTTOM_Y - SNAKE_MAP_HEIGHT - SNAKE_BORDER_SIZE * 2 + 1)

enum snake_bonustype_s {
	SNAKE_BONUS_NONE = 0,
	SNAKE_BONUS_SLOW,
	SNAKE_BONUS_FAST,
	SNAKE_BONUS_GHOST,
	SNAKE_BONUS_NUKE,
	SNAKE_BONUS_SCISSORS,
	SNAKE_BONUS_REVERSE,
	SNAKE_BONUS_EGGMAN,
	SNAKE_NUM_BONUSES,
};

static const char *snake_bonuspatches[] = {
	NULL,
	"DL_SLOW",
	"TVSSC0",
	"TVIVC0",
	"TVARC0",
	"DL_SCISSORS",
	"TVRCC0",
	"TVEGC0",
};

static const char *snake_backgrounds[] = {
	"RVPUMICF",
	"FRSTRCKF",
	"TAR",
	"MMFLRB4",
	"RVDARKF1",
	"RVZWALF1",
	"RVZWALF4",
	"RVZWALF5",
	"RVZGRS02",
	"RVZGRS04",
};

typedef struct snake_s
{
	boolean paused;
	boolean pausepressed;
	tic_t time;
	tic_t nextupdate;
	boolean gameover;
	UINT8 background;

	UINT16 snakelength;
	enum snake_bonustype_s snakebonus;
	tic_t snakebonustime;
	UINT8 snakex[SNAKE_NUM_BLOCKS_X * SNAKE_NUM_BLOCKS_Y];
	UINT8 snakey[SNAKE_NUM_BLOCKS_X * SNAKE_NUM_BLOCKS_Y];
	UINT8 snakedir[SNAKE_NUM_BLOCKS_X * SNAKE_NUM_BLOCKS_Y];

	UINT8 applex;
	UINT8 appley;

	enum snake_bonustype_s bonustype;
	UINT8 bonusx;
	UINT8 bonusy;
} snake_t;

static snake_t *snake = NULL;

static void Snake_Initialise(void)
{
	if (!snake)
		snake = malloc(sizeof(snake_t));

	snake->paused = false;
	snake->pausepressed = false;
	snake->time = 0;
	snake->nextupdate = SNAKE_SPEED;
	snake->gameover = false;
	snake->background = M_RandomKey(sizeof(snake_backgrounds) / sizeof(*snake_backgrounds));

	snake->snakelength = 1;
	snake->snakebonus = SNAKE_BONUS_NONE;
	snake->snakex[0] = M_RandomKey(SNAKE_NUM_BLOCKS_X);
	snake->snakey[0] = M_RandomKey(SNAKE_NUM_BLOCKS_Y);
	snake->snakedir[0] = 0;
	snake->snakedir[1] = 0;

	snake->applex = M_RandomKey(SNAKE_NUM_BLOCKS_X);
	snake->appley = M_RandomKey(SNAKE_NUM_BLOCKS_Y);

	snake->bonustype = SNAKE_BONUS_NONE;
}

static UINT8 Snake_GetOppositeDir(UINT8 dir)
{
	if (dir == 1 || dir == 3)
		return dir + 1;
	else if (dir == 2 || dir == 4)
		return dir - 1;
	else
		return 12 + 5 - dir;
}

static void Snake_FindFreeSlot(UINT8 *x, UINT8 *y, UINT8 headx, UINT8 heady)
{
	UINT16 i;

	do
	{
		*x = M_RandomKey(SNAKE_NUM_BLOCKS_X);
		*y = M_RandomKey(SNAKE_NUM_BLOCKS_Y);

		for (i = 0; i < snake->snakelength; i++)
			if (*x == snake->snakex[i] && *y == snake->snakey[i])
				break;
	} while (i < snake->snakelength || (*x == headx && *y == heady));
}

static void Snake_Handle(void)
{
	UINT8 x, y;
	UINT8 oldx, oldy;
	UINT16 i;

	// Handle retry
	if (snake->gameover && (PLAYER1INPUTDOWN(gc_jump) || gamekeydown[KEY_ENTER]))
	{
		Snake_Initialise();
		snake->pausepressed = true; // Avoid accidental pause on respawn
	}

	// Handle pause
	if (PLAYER1INPUTDOWN(gc_pause) || gamekeydown[KEY_ENTER])
	{
		if (!snake->pausepressed)
			snake->paused = !snake->paused;
		snake->pausepressed = true;
	}
	else
		snake->pausepressed = false;

	if (snake->paused)
		return;

	snake->time++;

	x = snake->snakex[0];
	y = snake->snakey[0];
	oldx = snake->snakex[1];
	oldy = snake->snakey[1];

	// Update direction
	if (gamekeydown[KEY_LEFTARROW])
	{
		if (snake->snakelength < 2 || x <= oldx)
			snake->snakedir[0] = 1;
	}
	else if (gamekeydown[KEY_RIGHTARROW])
	{
		if (snake->snakelength < 2 || x >= oldx)
			snake->snakedir[0] = 2;
	}
	else if (gamekeydown[KEY_UPARROW])
	{
		if (snake->snakelength < 2 || y <= oldy)
			snake->snakedir[0] = 3;
	}
	else if (gamekeydown[KEY_DOWNARROW])
	{
		if (snake->snakelength < 2 || y >= oldy)
			snake->snakedir[0] = 4;
	}

	if (snake->snakebonustime)
	{
		snake->snakebonustime--;
		if (!snake->snakebonustime)
			snake->snakebonus = SNAKE_BONUS_NONE;
	}

	snake->nextupdate--;
	if (snake->nextupdate)
		return;
	if (snake->snakebonus == SNAKE_BONUS_SLOW)
		snake->nextupdate = SNAKE_SPEED * 2;
	else if (snake->snakebonus == SNAKE_BONUS_FAST)
		snake->nextupdate = SNAKE_SPEED * 2 / 3;
	else
		snake->nextupdate = SNAKE_SPEED;

	if (snake->gameover)
		return;

	// Find new position
	switch (snake->snakedir[0])
	{
		case 1:
			if (x > 0)
				x--;
			else
				snake->gameover = true;
			break;
		case 2:
			if (x < SNAKE_NUM_BLOCKS_X - 1)
				x++;
			else
				snake->gameover = true;
			break;
		case 3:
			if (y > 0)
				y--;
			else
				snake->gameover = true;
			break;
		case 4:
			if (y < SNAKE_NUM_BLOCKS_Y - 1)
				y++;
			else
				snake->gameover = true;
			break;
	}

	// Check collision with snake
	if (snake->snakebonus != SNAKE_BONUS_GHOST)
		for (i = 1; i < snake->snakelength - 1; i++)
			if (x == snake->snakex[i] && y == snake->snakey[i])
			{
				if (snake->snakebonus == SNAKE_BONUS_SCISSORS)
				{
					snake->snakebonus = SNAKE_BONUS_NONE;
					snake->snakelength = i;
					S_StartSound(NULL, sfx_adderr);
				}
				else
					snake->gameover = true;
			}

	if (snake->gameover)
	{
		S_StartSound(NULL, sfx_lose);
		return;
	}

	// Check collision with apple
	if (x == snake->applex && y == snake->appley)
	{
		if (snake->snakelength + 1 < SNAKE_NUM_BLOCKS_X * SNAKE_NUM_BLOCKS_Y)
		{
			snake->snakelength++;
			snake->snakex  [snake->snakelength - 1] = snake->snakex  [snake->snakelength - 2];
			snake->snakey  [snake->snakelength - 1] = snake->snakey  [snake->snakelength - 2];
			snake->snakedir[snake->snakelength - 1] = snake->snakedir[snake->snakelength - 2];
		}

		// Spawn new apple
		Snake_FindFreeSlot(&snake->applex, &snake->appley, x, y);

		// Spawn new bonus
		if (!(snake->snakelength % 5))
		{
			do
			{
				snake->bonustype = M_RandomKey(SNAKE_NUM_BONUSES - 1) + 1;
			} while (snake->snakelength > SNAKE_NUM_BLOCKS_X * SNAKE_NUM_BLOCKS_Y * 3 / 4
				&& (snake->bonustype == SNAKE_BONUS_EGGMAN || snake->bonustype == SNAKE_BONUS_FAST || snake->bonustype == SNAKE_BONUS_REVERSE));

			Snake_FindFreeSlot(&snake->bonusx, &snake->bonusy, x, y);
		}

		S_StartSound(NULL, sfx_s3k6b);
	}

	if (snake->snakelength > 1 && snake->snakedir[0])
	{
		UINT8 dir = snake->snakedir[0];

		oldx = snake->snakex[1];
		oldy = snake->snakey[1];

		// Move
		for (i = snake->snakelength - 1; i > 0; i--)
		{
			snake->snakex[i] = snake->snakex[i - 1];
			snake->snakey[i] = snake->snakey[i - 1];
			snake->snakedir[i] = snake->snakedir[i - 1];
		}

		// Handle corners
		if      (x < oldx && dir == 3)
			dir = 5;
		else if (x > oldx && dir == 3)
			dir = 6;
		else if (x < oldx && dir == 4)
			dir = 7;
		else if (x > oldx && dir == 4)
			dir = 8;
		else if (y < oldy && dir == 1)
			dir = 9;
		else if (y < oldy && dir == 2)
			dir = 10;
		else if (y > oldy && dir == 1)
			dir = 11;
		else if (y > oldy && dir == 2)
			dir = 12;
		snake->snakedir[1] = dir;
	}

	snake->snakex[0] = x;
	snake->snakey[0] = y;

	// Check collision with bonus
	if (snake->bonustype != SNAKE_BONUS_NONE && x == snake->bonusx && y == snake->bonusy)
	{
		S_StartSound(NULL, sfx_ncchip);

		switch (snake->bonustype)
		{
		case SNAKE_BONUS_SLOW:
			snake->snakebonus = SNAKE_BONUS_SLOW;
			snake->snakebonustime = 20 * TICRATE;
			break;
		case SNAKE_BONUS_FAST:
			snake->snakebonus = SNAKE_BONUS_FAST;
			snake->snakebonustime = 20 * TICRATE;
			break;
		case SNAKE_BONUS_GHOST:
			snake->snakebonus = SNAKE_BONUS_GHOST;
			snake->snakebonustime = 10 * TICRATE;
			break;
		case SNAKE_BONUS_NUKE:
			for (i = 0; i < snake->snakelength; i++)
			{
				snake->snakex  [i] = snake->snakex  [0];
				snake->snakey  [i] = snake->snakey  [0];
				snake->snakedir[i] = snake->snakedir[0];
			}

			S_StartSound(NULL, sfx_bkpoof);
			break;
		case SNAKE_BONUS_SCISSORS:
			snake->snakebonus = SNAKE_BONUS_SCISSORS;
			snake->snakebonustime = 60 * TICRATE;
			break;
		case SNAKE_BONUS_REVERSE:
			for (i = 0; i < (snake->snakelength + 1) / 2; i++)
			{
				UINT16 i2 = snake->snakelength - 1 - i;
				UINT8 tmpx   = snake->snakex  [i];
				UINT8 tmpy   = snake->snakey  [i];
				UINT8 tmpdir = snake->snakedir[i];

				// Swap first segment with last segment
				snake->snakex  [i] = snake->snakex  [i2];
				snake->snakey  [i] = snake->snakey  [i2];
				snake->snakedir[i] = Snake_GetOppositeDir(snake->snakedir[i2]);
				snake->snakex  [i2] = tmpx;
				snake->snakey  [i2] = tmpy;
				snake->snakedir[i2] = Snake_GetOppositeDir(tmpdir);
			}

			snake->snakedir[0] = 0;

			S_StartSound(NULL, sfx_gravch);
			break;
		default:
			if (snake->snakebonus != SNAKE_BONUS_GHOST)
			{
				snake->gameover = true;
				S_StartSound(NULL, sfx_lose);
			}
		}

		snake->bonustype = SNAKE_BONUS_NONE;
	}
}

static void Snake_Draw(void)
{
	INT16 i;

	// Background
	V_DrawFlatFill(
		SNAKE_LEFT_X + SNAKE_BORDER_SIZE,
		SNAKE_TOP_Y  + SNAKE_BORDER_SIZE,
		SNAKE_MAP_WIDTH,
		SNAKE_MAP_HEIGHT,
		W_GetNumForName(snake_backgrounds[snake->background])
	);

	// Borders
	V_DrawFill(SNAKE_LEFT_X, SNAKE_TOP_Y, SNAKE_BORDER_SIZE + SNAKE_MAP_WIDTH, SNAKE_BORDER_SIZE, 242); // Top
	V_DrawFill(SNAKE_LEFT_X + SNAKE_BORDER_SIZE + SNAKE_MAP_WIDTH, SNAKE_TOP_Y, SNAKE_BORDER_SIZE, SNAKE_BORDER_SIZE + SNAKE_MAP_HEIGHT, 242); // Right
	V_DrawFill(SNAKE_LEFT_X + SNAKE_BORDER_SIZE, SNAKE_TOP_Y + SNAKE_BORDER_SIZE + SNAKE_MAP_HEIGHT, SNAKE_BORDER_SIZE + SNAKE_MAP_WIDTH, SNAKE_BORDER_SIZE, 242); // Bottom
	V_DrawFill(SNAKE_LEFT_X, SNAKE_TOP_Y + SNAKE_BORDER_SIZE, SNAKE_BORDER_SIZE, SNAKE_BORDER_SIZE + SNAKE_MAP_HEIGHT, 242); // Left

	// Apple
	V_DrawFixedPatch(
		(SNAKE_LEFT_X + SNAKE_BORDER_SIZE + snake->applex * SNAKE_BLOCK_SIZE + SNAKE_BLOCK_SIZE / 2) * FRACUNIT,
		(SNAKE_TOP_Y  + SNAKE_BORDER_SIZE + snake->appley * SNAKE_BLOCK_SIZE + SNAKE_BLOCK_SIZE / 2) * FRACUNIT,
		FRACUNIT / 4,
		0,
		W_CachePatchLongName("DL_APPLE", PU_HUDGFX),
		NULL
	);

	// Bonus
	if (snake->bonustype != SNAKE_BONUS_NONE)
		V_DrawFixedPatch(
			(SNAKE_LEFT_X + SNAKE_BORDER_SIZE + snake->bonusx * SNAKE_BLOCK_SIZE + SNAKE_BLOCK_SIZE / 2    ) * FRACUNIT,
			(SNAKE_TOP_Y  + SNAKE_BORDER_SIZE + snake->bonusy * SNAKE_BLOCK_SIZE + SNAKE_BLOCK_SIZE / 2 + 4) * FRACUNIT,
			FRACUNIT / 2,
			0,
			W_CachePatchLongName(snake_bonuspatches[snake->bonustype], PU_HUDGFX),
			NULL
		);

	// Snake
	if (!snake->gameover || snake->time % 8 < 8 / 2) // Blink if game over
	{
		for (i = snake->snakelength - 1; i >= 0; i--)
		{
			const char *patchname;
			UINT8 dir = snake->snakedir[i];

			if (i == 0) // Head
			{
				switch (dir)
				{
					case  1: patchname = "DL_SNAKEHEAD_L"; break;
					case  2: patchname = "DL_SNAKEHEAD_R"; break;
					case  3: patchname = "DL_SNAKEHEAD_T"; break;
					case  4: patchname = "DL_SNAKEHEAD_B"; break;
					default: patchname = "DL_SNAKEHEAD_M";
				}
			}
			else // Body
			{
				switch (dir)
				{
					case  1: patchname = "DL_SNAKEBODY_L"; break;
					case  2: patchname = "DL_SNAKEBODY_R"; break;
					case  3: patchname = "DL_SNAKEBODY_T"; break;
					case  4: patchname = "DL_SNAKEBODY_B"; break;
					case  5: patchname = "DL_SNAKEBODY_LT"; break;
					case  6: patchname = "DL_SNAKEBODY_RT"; break;
					case  7: patchname = "DL_SNAKEBODY_LB"; break;
					case  8: patchname = "DL_SNAKEBODY_RB"; break;
					case  9: patchname = "DL_SNAKEBODY_TL"; break;
					case 10: patchname = "DL_SNAKEBODY_TR"; break;
					case 11: patchname = "DL_SNAKEBODY_BL"; break;
					case 12: patchname = "DL_SNAKEBODY_BR"; break;
					default: patchname = "DL_SNAKEBODY_B";
				}
			}

			V_DrawFixedPatch(
				(SNAKE_LEFT_X + SNAKE_BORDER_SIZE + snake->snakex[i] * SNAKE_BLOCK_SIZE + SNAKE_BLOCK_SIZE / 2) * FRACUNIT,
				(SNAKE_TOP_Y  + SNAKE_BORDER_SIZE + snake->snakey[i] * SNAKE_BLOCK_SIZE + SNAKE_BLOCK_SIZE / 2) * FRACUNIT,
				i == 0 && dir == 0 ? FRACUNIT / 5 : FRACUNIT / 2,
				snake->snakebonus == SNAKE_BONUS_GHOST ? V_TRANSLUCENT : 0,
				W_CachePatchLongName(patchname, PU_HUDGFX),
				NULL
			);
		}
	}

	// Length
	V_DrawString(SNAKE_RIGHT_X + 4, SNAKE_TOP_Y, V_MONOSPACE, va("%u", snake->snakelength));

	// Bonus
	if (snake->snakebonus != SNAKE_BONUS_NONE
	&& (snake->snakebonustime >= 3 * TICRATE || snake->time % 4 < 4 / 2))
		V_DrawFixedPatch(
			(SNAKE_RIGHT_X + 10) * FRACUNIT,
			(SNAKE_TOP_Y + 24) * FRACUNIT,
			FRACUNIT / 2,
			0,
			W_CachePatchLongName(snake_bonuspatches[snake->snakebonus], PU_HUDGFX),
			NULL
		);
}

//
// CL_DrawConnectionStatus
//
// Keep the local client informed of our status.
//
static inline void CL_DrawConnectionStatus(void)
{
	INT32 ccstime = I_GetTime();

	// Draw background fade
	if (!menuactive) // menu already draws its own fade
		V_DrawFadeScreen(0xFF00, 16); // force default

	// Draw the bottom box.
	M_DrawTextBox(BASEVIDWIDTH/2-128-8, BASEVIDHEIGHT-16-8, 32, 1);
	V_DrawCenteredString(BASEVIDWIDTH/2, BASEVIDHEIGHT-16-16, V_YELLOWMAP, "Press ESC to abort");

	if (cl_mode != CL_DOWNLOADFILES)
	{
		INT32 i, animtime = ((ccstime / 4) & 15) + 16;
		UINT8 palstart = (cl_mode == CL_SEARCHING) ? 32 : 96;
		// 15 pal entries total.
		const char *cltext;

		if (!(cl_mode == CL_DOWNLOADSAVEGAME && lastfilenum != -1))
			for (i = 0; i < 16; ++i)
				V_DrawFill((BASEVIDWIDTH/2-128) + (i * 16), BASEVIDHEIGHT-16, 16, 8, palstart + ((animtime - i) & 15));

		switch (cl_mode)
		{
			case CL_DOWNLOADSAVEGAME:
				if (lastfilenum != -1)
				{
					UINT32 currentsize = fileneeded[lastfilenum].currentsize;
					UINT32 totalsize = fileneeded[lastfilenum].totalsize;
					INT32 dldlength;

					cltext = M_GetText("Downloading game state...");
					Net_GetNetStat();

					dldlength = (INT32)((currentsize/(double)totalsize) * 256);
					if (dldlength > 256)
						dldlength = 256;
					V_DrawFill(BASEVIDWIDTH/2-128, BASEVIDHEIGHT-16, 256, 8, 111);
					V_DrawFill(BASEVIDWIDTH/2-128, BASEVIDHEIGHT-16, dldlength, 8, 96);

					V_DrawString(BASEVIDWIDTH/2-128, BASEVIDHEIGHT-16, V_20TRANS|V_MONOSPACE,
						va(" %4uK/%4uK",currentsize>>10,totalsize>>10));

					V_DrawRightAlignedString(BASEVIDWIDTH/2+128, BASEVIDHEIGHT-16, V_20TRANS|V_MONOSPACE,
						va("%3.1fK/s ", ((double)getbps)/1024));
				}
				else
					cltext = M_GetText("Waiting to download game state...");
				break;
			case CL_ASKJOIN:
			case CL_WAITJOINRESPONSE:
				cltext = M_GetText("Requesting to join...");
				break;
			default:
				cltext = M_GetText("Connecting to server...");
				break;
		}
		V_DrawCenteredString(BASEVIDWIDTH/2, BASEVIDHEIGHT-16-24, V_YELLOWMAP, cltext);
	}
	else
	{
		if (lastfilenum != -1)
		{
			INT32 dldlength;
			static char tempname[28];
			fileneeded_t *file = &fileneeded[lastfilenum];
			char *filename = file->filename;

			Snake_Draw();

			Net_GetNetStat();
			dldlength = (INT32)((file->currentsize/(double)file->totalsize) * 256);
			if (dldlength > 256)
				dldlength = 256;
			V_DrawFill(BASEVIDWIDTH/2-128, BASEVIDHEIGHT-16, 256, 8, 111);
			V_DrawFill(BASEVIDWIDTH/2-128, BASEVIDHEIGHT-16, dldlength, 8, 96);

			memset(tempname, 0, sizeof(tempname));
			// offset filename to just the name only part
			filename += strlen(filename) - nameonlylength(filename);

			if (strlen(filename) > sizeof(tempname)-1) // too long to display fully
			{
				size_t endhalfpos = strlen(filename)-10;
				// display as first 14 chars + ... + last 10 chars
				// which should add up to 27 if our math(s) is correct
				snprintf(tempname, sizeof(tempname), "%.14s...%.10s", filename, filename+endhalfpos);
			}
			else // we can copy the whole thing in safely
			{
				strncpy(tempname, filename, sizeof(tempname)-1);
			}

			V_DrawCenteredString(BASEVIDWIDTH/2, BASEVIDHEIGHT-16-24, V_YELLOWMAP,
				va(M_GetText("Downloading \"%s\""), tempname));
			V_DrawString(BASEVIDWIDTH/2-128, BASEVIDHEIGHT-16, V_20TRANS|V_MONOSPACE,
				va(" %4uK/%4uK",fileneeded[lastfilenum].currentsize>>10,file->totalsize>>10));
			V_DrawRightAlignedString(BASEVIDWIDTH/2+128, BASEVIDHEIGHT-16, V_20TRANS|V_MONOSPACE,
				va("%3.1fK/s ", ((double)getbps)/1024));
		}
		else
			V_DrawCenteredString(BASEVIDWIDTH/2, BASEVIDHEIGHT-16-24, V_YELLOWMAP,
				M_GetText("Waiting to download files..."));
	}
}
#endif

/** Sends a special packet to declare how many players in local
  * Used only in arbitratrenetstart()
  * Sends a PT_CLIENTJOIN packet to the server
  *
  * \return True if the packet was successfully sent
  * \todo Improve the description...
  *       Because to be honest, I have no idea what arbitratrenetstart is...
  *       Is it even used...?
  *
  */
static boolean CL_SendJoin(void)
{
	UINT8 localplayers = 1;
	if (netgame)
		CONS_Printf(M_GetText("Sending join request...\n"));
	netbuffer->packettype = PT_CLIENTJOIN;

	if (splitscreen || botingame)
		localplayers++;
	netbuffer->u.clientcfg.localplayers = localplayers;
	netbuffer->u.clientcfg._255 = 255;
	netbuffer->u.clientcfg.packetversion = PACKETVERSION;
	netbuffer->u.clientcfg.version = VERSION;
	netbuffer->u.clientcfg.subversion = SUBVERSION;
	strncpy(netbuffer->u.clientcfg.application, SRB2APPLICATION,
			sizeof netbuffer->u.clientcfg.application);

	CleanupPlayerName(consoleplayer, cv_playername.zstring);
	if (splitscreen)
		CleanupPlayerName(1, cv_playername2.zstring);/* 1 is a HACK? oh no */

	strncpy(netbuffer->u.clientcfg.names[0], cv_playername.zstring, MAXPLAYERNAME);
	strncpy(netbuffer->u.clientcfg.names[1], cv_playername2.zstring, MAXPLAYERNAME);

	return HSendPacket(servernode, true, 0, sizeof (clientconfig_pak));
}

static INT32 FindRejoinerNum(SINT8 node)
{
	char strippednodeaddress[64];
	const char *nodeaddress;
	char *port;
	INT32 i;

	// Make sure there is no dead dress before proceeding to the stripping
	if (!I_GetNodeAddress)
		return -1;
	nodeaddress = I_GetNodeAddress(node);
	if (!nodeaddress)
		return -1;

	// Strip the address of its port
	strcpy(strippednodeaddress, nodeaddress);
	port = strchr(strippednodeaddress, ':');
	if (port)
		*port = '\0';

	// Check if any player matches the stripped address
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (playeringame[i] && playeraddress[i][0] && playernode[i] == UINT8_MAX
		&& !strcmp(playeraddress[i], strippednodeaddress))
			return i;
	}

	return -1;
}

static void SV_SendServerInfo(INT32 node, tic_t servertime)
{
	UINT8 *p;

	netbuffer->packettype = PT_SERVERINFO;
	netbuffer->u.serverinfo._255 = 255;
	netbuffer->u.serverinfo.packetversion = PACKETVERSION;
	netbuffer->u.serverinfo.version = VERSION;
	netbuffer->u.serverinfo.subversion = SUBVERSION;
	strncpy(netbuffer->u.serverinfo.application, SRB2APPLICATION,
			sizeof netbuffer->u.serverinfo.application);
	// return back the time value so client can compute their ping
	netbuffer->u.serverinfo.time = (tic_t)LONG(servertime);
	netbuffer->u.serverinfo.leveltime = (tic_t)LONG(leveltime);

	netbuffer->u.serverinfo.numberofplayer = (UINT8)D_NumPlayers();
	netbuffer->u.serverinfo.maxplayer = (UINT8)cv_maxplayers.value;

	if (!node || FindRejoinerNum(node) != -1)
		netbuffer->u.serverinfo.refusereason = 0;
	else if (!cv_allownewplayer.value)
		netbuffer->u.serverinfo.refusereason = 1;
	else if (D_NumPlayers() >= cv_maxplayers.value)
		netbuffer->u.serverinfo.refusereason = 2;
	else
		netbuffer->u.serverinfo.refusereason = 0;

	strncpy(netbuffer->u.serverinfo.gametypename, Gametype_Names[gametype],
			sizeof netbuffer->u.serverinfo.gametypename);
	netbuffer->u.serverinfo.modifiedgame = (UINT8)modifiedgame;
	netbuffer->u.serverinfo.cheatsenabled = CV_CheatsEnabled();
	netbuffer->u.serverinfo.isdedicated = (UINT8)dedicated;
	strncpy(netbuffer->u.serverinfo.servername, cv_servername.string,
		MAXSERVERNAME);
	strncpy(netbuffer->u.serverinfo.mapname, G_BuildMapName(gamemap), 7);

	M_Memcpy(netbuffer->u.serverinfo.mapmd5, mapmd5, 16);

	memset(netbuffer->u.serverinfo.maptitle, 0, sizeof netbuffer->u.serverinfo.maptitle);

	if (mapheaderinfo[gamemap-1] && *mapheaderinfo[gamemap-1]->lvlttl)
	{
		char *read = mapheaderinfo[gamemap-1]->lvlttl, *writ = netbuffer->u.serverinfo.maptitle;
		while (writ < (netbuffer->u.serverinfo.maptitle+32) && *read != '\0')
		{
			if (!(*read & 0x80))
			{
				*writ = toupper(*read);
				writ++;
			}
			read++;
		}
		*writ = '\0';
		//strncpy(netbuffer->u.serverinfo.maptitle, (char *)mapheaderinfo[gamemap-1]->lvlttl, 33);
	}
	else
		strncpy(netbuffer->u.serverinfo.maptitle, "UNKNOWN", 32);

	if (mapheaderinfo[gamemap-1] && !(mapheaderinfo[gamemap-1]->levelflags & LF_NOZONE))
		netbuffer->u.serverinfo.iszone = 1;
	else
		netbuffer->u.serverinfo.iszone = 0;

	if (mapheaderinfo[gamemap-1])
		netbuffer->u.serverinfo.actnum = mapheaderinfo[gamemap-1]->actnum;

	p = PutFileNeeded();

	HSendPacket(node, false, 0, p - ((UINT8 *)&netbuffer->u));
}

static void SV_SendPlayerInfo(INT32 node)
{
	UINT8 i;
	netbuffer->packettype = PT_PLAYERINFO;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i])
		{
			netbuffer->u.playerinfo[i].num = 255; // This slot is empty.
			continue;
		}

		netbuffer->u.playerinfo[i].num = i;
		strncpy(netbuffer->u.playerinfo[i].name, (const char *)&player_names[i], MAXPLAYERNAME+1);
		netbuffer->u.playerinfo[i].name[MAXPLAYERNAME] = '\0';

		//fetch IP address
		//No, don't do that, you fuckface.
		memset(netbuffer->u.playerinfo[i].address, 0, 4);

		if (G_GametypeHasTeams())
		{
			if (!players[i].ctfteam)
				netbuffer->u.playerinfo[i].team = 255;
			else
				netbuffer->u.playerinfo[i].team = (UINT8)players[i].ctfteam;
		}
		else
		{
			if (players[i].spectator)
				netbuffer->u.playerinfo[i].team = 255;
			else
				netbuffer->u.playerinfo[i].team = 0;
		}

		netbuffer->u.playerinfo[i].score = LONG(players[i].score);
		netbuffer->u.playerinfo[i].timeinserver = SHORT((UINT16)(players[i].jointime / TICRATE));
		netbuffer->u.playerinfo[i].skin = (UINT8)(players[i].skin
#ifdef DEVELOP // it's safe to do this only because PLAYERINFO isn't read by the game itself
		% 3
#endif
		);

		// Extra data
		netbuffer->u.playerinfo[i].data = 0; //players[i].skincolor;

		if (players[i].pflags & PF_TAGIT)
			netbuffer->u.playerinfo[i].data |= 0x20;

		if (players[i].gotflag)
			netbuffer->u.playerinfo[i].data |= 0x40;

		if (players[i].powers[pw_super])
			netbuffer->u.playerinfo[i].data |= 0x80;
	}

	HSendPacket(node, false, 0, sizeof(plrinfo) * MAXPLAYERS);
}

/** Sends a PT_SERVERCFG packet
  *
  * \param node The destination
  * \return True if the packet was successfully sent
  *
  */
static boolean SV_SendServerConfig(INT32 node)
{
	INT32 i;
	UINT8 *p, *op;
	boolean waspacketsent;

	netbuffer->packettype = PT_SERVERCFG;

	netbuffer->u.servercfg.version = VERSION;
	netbuffer->u.servercfg.subversion = SUBVERSION;

	netbuffer->u.servercfg.serverplayer = (UINT8)serverplayer;
	netbuffer->u.servercfg.totalslotnum = (UINT8)(doomcom->numslots);
	netbuffer->u.servercfg.gametic = (tic_t)LONG(gametic);
	netbuffer->u.servercfg.clientnode = (UINT8)node;
	netbuffer->u.servercfg.gamestate = (UINT8)gamestate;
	netbuffer->u.servercfg.gametype = (UINT8)gametype;
	netbuffer->u.servercfg.modifiedgame = (UINT8)modifiedgame;

	// we fill these structs with FFs so that any players not in game get sent as 0xFFFF
	// which is nice and easy for us to detect
	memset(netbuffer->u.servercfg.playerskins, 0xFF, sizeof(netbuffer->u.servercfg.playerskins));
	memset(netbuffer->u.servercfg.playercolor, 0xFF, sizeof(netbuffer->u.servercfg.playercolor));
	memset(netbuffer->u.servercfg.playeravailabilities, 0xFF, sizeof(netbuffer->u.servercfg.playeravailabilities));

	memset(netbuffer->u.servercfg.adminplayers, -1, sizeof(netbuffer->u.servercfg.adminplayers));

	for (i = 0; i < MAXPLAYERS; i++)
	{
		netbuffer->u.servercfg.adminplayers[i] = (SINT8)adminplayers[i];

		if (!playeringame[i])
			continue;
		netbuffer->u.servercfg.playerskins[i] = (UINT8)players[i].skin;
		netbuffer->u.servercfg.playercolor[i] = (UINT16)players[i].skincolor;
		netbuffer->u.servercfg.playeravailabilities[i] = (UINT32)LONG(players[i].availabilities);
	}

	memcpy(netbuffer->u.servercfg.server_context, server_context, 8);
	op = p = netbuffer->u.servercfg.varlengthinputs;

	CV_SavePlayerNames(&p);
	CV_SaveNetVars(&p);
	{
		const size_t len = sizeof (serverconfig_pak) + (size_t)(p - op);

#ifdef DEBUGFILE
		if (debugfile)
		{
			fprintf(debugfile, "ServerConfig Packet about to be sent, size of packet:%s to node:%d\n",
				sizeu1(len), node);
		}
#endif

		waspacketsent = HSendPacket(node, true, 0, len);
	}

#ifdef DEBUGFILE
	if (debugfile)
	{
		if (waspacketsent)
		{
			fprintf(debugfile, "ServerConfig Packet was sent\n");
		}
		else
		{
			fprintf(debugfile, "ServerConfig Packet could not be sent right now\n");
		}
	}
#endif

	return waspacketsent;
}

#ifndef NONET
#define SAVEGAMESIZE (768*1024)

static void SV_SendSaveGame(INT32 node)
{
	size_t length, compressedlen;
	UINT8 *savebuffer;
	UINT8 *compressedsave;
	UINT8 *buffertosend;

	// first save it in a malloced buffer
	savebuffer = (UINT8 *)malloc(SAVEGAMESIZE);
	if (!savebuffer)
	{
		CONS_Alert(CONS_ERROR, M_GetText("No more free memory for savegame\n"));
		return;
	}

	// Leave room for the uncompressed length.
	save_p = savebuffer + sizeof(UINT32);

	P_SaveNetGame();

	length = save_p - savebuffer;
	if (length > SAVEGAMESIZE)
	{
		free(savebuffer);
		save_p = NULL;
		I_Error("Savegame buffer overrun");
	}

	// Allocate space for compressed save: one byte fewer than for the
	// uncompressed data to ensure that the compression is worthwhile.
	compressedsave = malloc(length - 1);
	if (!compressedsave)
	{
		CONS_Alert(CONS_ERROR, M_GetText("No more free memory for savegame\n"));
		return;
	}

	// Attempt to compress it.
	if((compressedlen = lzf_compress(savebuffer + sizeof(UINT32), length - sizeof(UINT32), compressedsave + sizeof(UINT32), length - sizeof(UINT32) - 1)))
	{
		// Compressing succeeded; send compressed data

		free(savebuffer);

		// State that we're compressed.
		buffertosend = compressedsave;
		WRITEUINT32(compressedsave, length - sizeof(UINT32));
		length = compressedlen + sizeof(UINT32);
	}
	else
	{
		// Compression failed to make it smaller; send original

		free(compressedsave);

		// State that we're not compressed
		buffertosend = savebuffer;
		WRITEUINT32(savebuffer, 0);
	}

	AddRamToSendQueue(node, buffertosend, length, SF_RAM, 0);
	save_p = NULL;

	// Remember when we started sending the savegame so we can handle timeouts
	sendingsavegame[node] = true;
	freezetimeout[node] = I_GetTime() + jointimeout + length / 1024; // 1 extra tic for each kilobyte
}

#ifdef DUMPCONSISTENCY
#define TMPSAVENAME "badmath.sav"
static consvar_t cv_dumpconsistency = {"dumpconsistency", "Off", CV_NETVAR, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};

static void SV_SavedGame(void)
{
	size_t length;
	UINT8 *savebuffer;
	char tmpsave[256];

	if (!cv_dumpconsistency.value)
		return;

	sprintf(tmpsave, "%s" PATHSEP TMPSAVENAME, srb2home);

	// first save it in a malloced buffer
	save_p = savebuffer = (UINT8 *)malloc(SAVEGAMESIZE);
	if (!save_p)
	{
		CONS_Alert(CONS_ERROR, M_GetText("No more free memory for savegame\n"));
		return;
	}

	P_SaveNetGame();

	length = save_p - savebuffer;
	if (length > SAVEGAMESIZE)
	{
		free(savebuffer);
		save_p = NULL;
		I_Error("Savegame buffer overrun");
	}

	// then save it!
	if (!FIL_WriteFile(tmpsave, savebuffer, length))
		CONS_Printf(M_GetText("Didn't save %s for netgame"), tmpsave);

	free(savebuffer);
	save_p = NULL;
}

#undef  TMPSAVENAME
#endif
#define TMPSAVENAME "$$$.sav"


static void CL_LoadReceivedSavegame(void)
{
	UINT8 *savebuffer = NULL;
	size_t length, decompressedlen;
	char tmpsave[256];

	sprintf(tmpsave, "%s" PATHSEP TMPSAVENAME, srb2home);

	length = FIL_ReadFile(tmpsave, &savebuffer);

	CONS_Printf(M_GetText("Loading savegame length %s\n"), sizeu1(length));
	if (!length)
	{
		I_Error("Can't read savegame sent");
		return;
	}

	save_p = savebuffer;

	// Decompress saved game if necessary.
	decompressedlen = READUINT32(save_p);
	if(decompressedlen > 0)
	{
		UINT8 *decompressedbuffer = Z_Malloc(decompressedlen, PU_STATIC, NULL);
		lzf_decompress(save_p, length - sizeof(UINT32), decompressedbuffer, decompressedlen);
		Z_Free(savebuffer);
		save_p = savebuffer = decompressedbuffer;
	}

	paused = false;
	demoplayback = false;
	titlemapinaction = TITLEMAP_OFF;
	titledemo = false;
	automapactive = false;

	// load a base level
	if (P_LoadNetGame())
	{
		const UINT8 actnum = mapheaderinfo[gamemap-1]->actnum;
		CONS_Printf(M_GetText("Map is now \"%s"), G_BuildMapName(gamemap));
		if (strcmp(mapheaderinfo[gamemap-1]->lvlttl, ""))
		{
			CONS_Printf(": %s", mapheaderinfo[gamemap-1]->lvlttl);
			if (!(mapheaderinfo[gamemap-1]->levelflags & LF_NOZONE))
				CONS_Printf(M_GetText(" Zone"));
			if (actnum > 0)
				CONS_Printf(" %2d", actnum);
		}
		CONS_Printf("\"\n");
	}
	else
	{
		CONS_Alert(CONS_ERROR, M_GetText("Can't load the level!\n"));
		Z_Free(savebuffer);
		save_p = NULL;
		if (unlink(tmpsave) == -1)
			CONS_Alert(CONS_ERROR, M_GetText("Can't delete %s\n"), tmpsave);
		return;
	}

	// done
	Z_Free(savebuffer);
	save_p = NULL;
	if (unlink(tmpsave) == -1)
		CONS_Alert(CONS_ERROR, M_GetText("Can't delete %s\n"), tmpsave);
	consistancy[gametic%BACKUPTICS] = Consistancy();
	CON_ToggleOff();
}
#endif

#ifndef NONET
static void SendAskInfo(INT32 node, boolean viams)
{
	const tic_t asktime = I_GetTime();
	netbuffer->packettype = PT_ASKINFO;
	netbuffer->u.askinfo.version = VERSION;
	netbuffer->u.askinfo.time = (tic_t)LONG(asktime);

	// Even if this never arrives due to the host being firewalled, we've
	// now allowed traffic from the host to us in, so once the MS relays
	// our address to the host, it'll be able to speak to us.
	HSendPacket(node, false, 0, sizeof (askinfo_pak));

	// Also speak to the MS.
	if (viams && node != 0 && node != BROADCASTADDR)
		SendAskInfoViaMS(node, asktime);
}

serverelem_t serverlist[MAXSERVERLIST];
UINT32 serverlistcount = 0;

#define FORCECLOSE 0x8000

static void SL_ClearServerList(INT32 connectedserver)
{
	UINT32 i;

	for (i = 0; i < serverlistcount; i++)
		if (connectedserver != serverlist[i].node)
		{
			Net_CloseConnection(serverlist[i].node|FORCECLOSE);
			serverlist[i].node = 0;
		}
	serverlistcount = 0;
}

static UINT32 SL_SearchServer(INT32 node)
{
	UINT32 i;
	for (i = 0; i < serverlistcount; i++)
		if (serverlist[i].node == node)
			return i;

	return UINT32_MAX;
}

static void SL_InsertServer(serverinfo_pak* info, SINT8 node)
{
	UINT32 i;

	// search if not already on it
	i = SL_SearchServer(node);
	if (i == UINT32_MAX)
	{
		// not found add it
		if (serverlistcount >= MAXSERVERLIST)
			return; // list full

		if (info->_255 != 255)
			return;/* old packet format */

		if (info->packetversion != PACKETVERSION)
			return;/* old new packet format */

		if (info->version != VERSION)
			return; // Not same version.

		if (info->subversion != SUBVERSION)
			return; // Close, but no cigar.

		if (strcmp(info->application, SRB2APPLICATION))
			return;/* that's a different mod */

		i = serverlistcount++;
	}

	serverlist[i].info = *info;
	serverlist[i].node = node;

	// resort server list
	M_SortServerList();
}

void CL_UpdateServerList(boolean internetsearch, INT32 room)
{
	SL_ClearServerList(0);

	if (!netgame && I_NetOpenSocket)
	{
		MSCloseUDPSocket();		// Tidy up before wiping the slate.
		if (I_NetOpenSocket())
		{
			netgame = true;
			multiplayer = true;
		}
	}

	// search for local servers
	if (netgame)
		SendAskInfo(BROADCASTADDR, false);

	if (internetsearch)
	{
		const msg_server_t *server_list;
		INT32 i = -1;
		server_list = GetShortServersList(room);
		if (server_list)
		{
			char version[8] = "";
#if VERSION > 0 || SUBVERSION > 0
			snprintf(version, sizeof (version), "%d.%d.%d", VERSION/100, VERSION%100, SUBVERSION);
#else
			strcpy(version, GetRevisionString());
#endif
			version[sizeof (version) - 1] = '\0';

			for (i = 0; server_list[i].header.buffer[0]; i++)
			{
				// Make sure MS version matches our own, to
				// thwart nefarious servers who lie to the MS.

				if (strcmp(version, server_list[i].version) == 0)
				{
					INT32 node = I_NetMakeNodewPort(server_list[i].ip, server_list[i].port);
					if (node == -1)
						break; // no more node free
					SendAskInfo(node, true);
					// Force close the connection so that servers can't eat
					// up nodes forever if we never get a reply back from them
					// (usually when they've not forwarded their ports).
					//
					// Don't worry, we'll get in contact with the working
					// servers again when they send SERVERINFO to us later!
					//
					// (Note: as a side effect this probably means every
					// server in the list will probably be using the same node (e.g. node 1),
					// not that it matters which nodes they use when
					// the connections are closed afterwards anyway)
					// -- Monster Iestyn 12/11/18
					Net_CloseConnection(node|FORCECLOSE);
				}
			}
		}

		//no server list?(-1) or no servers?(0)
		if (!i)
		{
			; /// TODO: display error or warning?
		}
	}
}

#endif // ifndef NONET

/** Called by CL_ServerConnectionTicker
  *
  * \param viams ???
  * \param asksent ???
  * \return False if the connection was aborted
  * \sa CL_ServerConnectionTicker
  * \sa CL_ConnectToServer
  *
  */
static boolean CL_ServerConnectionSearchTicker(boolean viams, tic_t *asksent)
{
#ifndef NONET
	INT32 i;

	// serverlist is updated by GetPacket function
	if (serverlistcount > 0)
	{
		// this can be a responce to our broadcast request
		if (servernode == -1 || servernode >= MAXNETNODES)
		{
			i = 0;
			servernode = serverlist[i].node;
			CONS_Printf(M_GetText("Found, "));
		}
		else
		{
			i = SL_SearchServer(servernode);
			if (i < 0)
				return true;
		}

		// Quit here rather than downloading files and being refused later.
		if (serverlist[i].info.refusereason)
		{
			D_QuitNetGame();
			CL_Reset();
			D_StartTitle();
			if (serverlist[i].info.refusereason == 1)
				M_StartMessage(M_GetText("The server is not accepting\njoins for the moment.\n\nPress ESC\n"), NULL, MM_NOTHING);
			else if (serverlist[i].info.refusereason == 2)
				M_StartMessage(va(M_GetText("Maximum players reached: %d\n\nPress ESC\n"), serverlist[i].info.maxplayer), NULL, MM_NOTHING);
			else
				M_StartMessage(M_GetText("You can't join.\nI don't know why,\nbut you can't join.\n\nPress ESC\n"), NULL, MM_NOTHING);
			return false;
		}

		if (client)
		{
			D_ParseFileneeded(serverlist[i].info.fileneedednum,
				serverlist[i].info.fileneeded);
			CONS_Printf(M_GetText("Checking files...\n"));
			i = CL_CheckFiles();
			if (i == 3) // too many files
			{
				D_QuitNetGame();
				CL_Reset();
				D_StartTitle();
				M_StartMessage(M_GetText(
					"You have too many WAD files loaded\n"
					"to add ones the server is using.\n"
					"Please restart SRB2 before connecting.\n\n"
					"Press ESC\n"
				), NULL, MM_NOTHING);
				return false;
			}
			else if (i == 2) // cannot join for some reason
			{
				D_QuitNetGame();
				CL_Reset();
				D_StartTitle();
				M_StartMessage(M_GetText(
					"You have WAD files loaded or have\n"
					"modified the game in some way, and\n"
					"your file list does not match\n"
					"the server's file list.\n"
					"Please restart SRB2 before connecting.\n\n"
					"Press ESC\n"
				), NULL, MM_NOTHING);
				return false;
			}
			else if (i == 1)
				cl_mode = CL_ASKJOIN;
			else
			{
				// must download something
				// can we, though?
				if (!CL_CheckDownloadable()) // nope!
				{
					D_QuitNetGame();
					CL_Reset();
					D_StartTitle();
					M_StartMessage(M_GetText(
						"You cannot connect to this server\n"
						"because you cannot download the files\n"
						"that you are missing from the server.\n\n"
						"See the console or log file for\n"
						"more details.\n\n"
						"Press ESC\n"
					), NULL, MM_NOTHING);
					return false;
				}
				// no problem if can't send packet, we will retry later
				if (CL_SendFileRequest())
				{
					cl_mode = CL_DOWNLOADFILES;
#ifndef NONET
					Snake_Initialise();
#endif
				}
			}
		}
		else
			cl_mode = CL_ASKJOIN; // files need not be checked for the server.

		return true;
	}

	// Ask the info to the server (askinfo packet)
	if (*asksent + NEWTICRATE < I_GetTime())
	{
		SendAskInfo(servernode, viams);
		*asksent = I_GetTime();
	}
#else
	(void)viams;
	(void)asksent;
	// No netgames, so we skip this state.
	cl_mode = CL_ASKJOIN;
#endif // ifndef NONET/else

	return true;
}

/** Called by CL_ConnectToServer
  *
  * \param viams ???
  * \param tmpsave The name of the gamestate file???
  * \param oldtic Used for knowing when to poll events and redraw
  * \param asksent ???
  * \return False if the connection was aborted
  * \sa CL_ServerConnectionSearchTicker
  * \sa CL_ConnectToServer
  *
  */
static boolean CL_ServerConnectionTicker(boolean viams, const char *tmpsave, tic_t *oldtic, tic_t *asksent)
{
	boolean waitmore;
	INT32 i;

#ifdef NONET
	(void)tmpsave;
#endif

	switch (cl_mode)
	{
		case CL_SEARCHING:
			if (!CL_ServerConnectionSearchTicker(viams, asksent))
				return false;
			break;

		case CL_DOWNLOADFILES:
			waitmore = false;
			for (i = 0; i < fileneedednum; i++)
				if (fileneeded[i].status == FS_DOWNLOADING
					|| fileneeded[i].status == FS_REQUESTED)
				{
					waitmore = true;
					break;
				}
			if (waitmore)
				break; // exit the case

#ifndef NONET
			if (snake)
			{
				free(snake);
				snake = NULL;
			}
#endif

			cl_mode = CL_ASKJOIN; // don't break case continue to cljoin request now
			/* FALLTHRU */

		case CL_ASKJOIN:
			CL_LoadServerFiles();
#ifndef NONET
			// prepare structures to save the file
			// WARNING: this can be useless in case of server not in GS_LEVEL
			// but since the network layer doesn't provide ordered packets...
			CL_PrepareDownloadSaveGame(tmpsave);
#endif
			if (CL_SendJoin())
				cl_mode = CL_WAITJOINRESPONSE;
			break;

#ifndef NONET
		case CL_DOWNLOADSAVEGAME:
			// At this state, the first (and only) needed file is the gamestate
			if (fileneeded[0].status == FS_FOUND)
			{
				// Gamestate is now handled within CL_LoadReceivedSavegame()
				CL_LoadReceivedSavegame();
				cl_mode = CL_CONNECTED;
			} // don't break case continue to CL_CONNECTED
			else
				break;
#endif

		case CL_WAITJOINRESPONSE:
		case CL_CONNECTED:
		default:
			break;

		// Connection closed by cancel, timeout or refusal.
		case CL_ABORTED:
			cl_mode = CL_SEARCHING;
			return false;

	}

	GetPackets();
	Net_AckTicker();

	// Call it only once by tic
	if (*oldtic != I_GetTime())
	{
		I_OsPolling();
		for (; eventtail != eventhead; eventtail = (eventtail+1) & (MAXEVENTS-1))
			G_MapEventsToControls(&events[eventtail]);

		if (gamekeydown[KEY_ESCAPE] || gamekeydown[KEY_JOY1+1])
		{
			CONS_Printf(M_GetText("Network game synchronization aborted.\n"));
//				M_StartMessage(M_GetText("Network game synchronization aborted.\n\nPress ESC\n"), NULL, MM_NOTHING);

#ifndef NONET
			if (snake)
			{
				free(snake);
				snake = NULL;
			}
#endif

			D_QuitNetGame();
			CL_Reset();
			D_StartTitle();
			memset(gamekeydown, 0, NUMKEYS);
			return false;
		}
#ifndef NONET
		else if (cl_mode == CL_DOWNLOADFILES && snake)
			Snake_Handle();
#endif

		if (client && (cl_mode == CL_DOWNLOADFILES || cl_mode == CL_DOWNLOADSAVEGAME))
			FileReceiveTicker();

		// why are these here? this is for servers, we're a client
		//if (key == 's' && server)
		//	doomcom->numnodes = (INT16)pnumnodes;
		//FileSendTicker();
		*oldtic = I_GetTime();

#ifndef NONET
		if (client && cl_mode != CL_CONNECTED && cl_mode != CL_ABORTED)
		{
			if (cl_mode != CL_DOWNLOADFILES && cl_mode != CL_DOWNLOADSAVEGAME)
			{
				F_MenuPresTicker(true); // title sky
				F_TitleScreenTicker(true);
				F_TitleScreenDrawer();
			}
			CL_DrawConnectionStatus();
			I_UpdateNoVsync(); // page flip or blit buffer
			if (moviemode)
				M_SaveFrame();
			S_UpdateSounds();
			S_UpdateClosedCaptions();
		}
#else
		CON_Drawer();
		I_UpdateNoVsync();
#endif
	}
	else
		I_Sleep();

	return true;
}

/** Use adaptive send using net_bandwidth and stat.sendbytes
  *
  * \param viams ???
  * \todo Better description...
  *
  */
static void CL_ConnectToServer(boolean viams)
{
	INT32 pnumnodes, nodewaited = doomcom->numnodes, i;
	tic_t oldtic;
#ifndef NONET
	tic_t asksent;
	char tmpsave[256];

	sprintf(tmpsave, "%s" PATHSEP TMPSAVENAME, srb2home);

	lastfilenum = -1;
#endif

	cl_mode = CL_SEARCHING;

#ifndef NONET
	// Don't get a corrupt savegame error because tmpsave already exists
	if (FIL_FileExists(tmpsave) && unlink(tmpsave) == -1)
		I_Error("Can't delete %s\n", tmpsave);
#endif

	if (netgame)
	{
		if (servernode < 0 || servernode >= MAXNETNODES)
			CONS_Printf(M_GetText("Searching for a server...\n"));
		else
			CONS_Printf(M_GetText("Contacting the server...\n"));
	}

	if (gamestate == GS_INTERMISSION)
		Y_EndIntermission(); // clean up intermission graphics etc

	DEBFILE(va("waiting %d nodes\n", doomcom->numnodes));
	G_SetGamestate(GS_WAITINGPLAYERS);
	wipegamestate = GS_WAITINGPLAYERS;

	ClearAdminPlayers();
	pnumnodes = 1;
	oldtic = I_GetTime() - 1;
#ifndef NONET
	asksent = (tic_t) - TICRATE;

	i = SL_SearchServer(servernode);

	if (i != -1)
	{
		char *gametypestr = serverlist[i].info.gametypename;
		CONS_Printf(M_GetText("Connecting to: %s\n"), serverlist[i].info.servername);
		gametypestr[sizeof serverlist[i].info.gametypename - 1] = '\0';
		CONS_Printf(M_GetText("Gametype: %s\n"), gametypestr);
		CONS_Printf(M_GetText("Version: %d.%d.%u\n"), serverlist[i].info.version/100,
		 serverlist[i].info.version%100, serverlist[i].info.subversion);
	}
	SL_ClearServerList(servernode);
#endif

	do
	{
		// If the connection was aborted for some reason, leave
#ifndef NONET
		if (!CL_ServerConnectionTicker(viams, tmpsave, &oldtic, &asksent))
#else
		if (!CL_ServerConnectionTicker(viams, (char*)NULL, &oldtic, (tic_t *)NULL))
#endif
			return;

		if (server)
		{
			pnumnodes = 0;
			for (i = 0; i < MAXNETNODES; i++)
				if (nodeingame[i])
					pnumnodes++;
		}
	}
	while (!(cl_mode == CL_CONNECTED && (client || (server && nodewaited <= pnumnodes))));

	DEBFILE(va("Synchronisation Finished\n"));

	displayplayer = consoleplayer;
}

#ifndef NONET
typedef struct banreason_s
{
	char *reason;
	struct banreason_s *prev; //-1
	struct banreason_s *next; //+1
} banreason_t;

static banreason_t *reasontail = NULL; //last entry, use prev
static banreason_t *reasonhead = NULL; //1st entry, use next

static void Command_ShowBan(void) //Print out ban list
{
	size_t i;
	const char *address, *mask;
	banreason_t *reasonlist = reasonhead;

	if (I_GetBanAddress)
		CONS_Printf(M_GetText("Ban List:\n"));
	else
		return;

	for (i = 0;(address = I_GetBanAddress(i)) != NULL;i++)
	{
		if (!I_GetBanMask || (mask = I_GetBanMask(i)) == NULL)
			CONS_Printf("%s: %s ", sizeu1(i+1), address);
		else
			CONS_Printf("%s: %s/%s ", sizeu1(i+1), address, mask);

		if (reasonlist && reasonlist->reason)
			CONS_Printf("(%s)\n", reasonlist->reason);
		else
			CONS_Printf("\n");

		if (reasonlist) reasonlist = reasonlist->next;
	}

	if (i == 0 && !address)
		CONS_Printf(M_GetText("(empty)\n"));
}

void D_SaveBan(void)
{
	FILE *f;
	size_t i;
	banreason_t *reasonlist = reasonhead;
	const char *address, *mask;

	if (!reasonhead)
		return;

	f = fopen(va("%s"PATHSEP"%s", srb2home, "ban.txt"), "w");

	if (!f)
	{
		CONS_Alert(CONS_WARNING, M_GetText("Could not save ban list into ban.txt\n"));
		return;
	}

	for (i = 0;(address = I_GetBanAddress(i)) != NULL;i++)
	{
		if (!I_GetBanMask || (mask = I_GetBanMask(i)) == NULL)
			fprintf(f, "%s 0", address);
		else
			fprintf(f, "%s %s", address, mask);

		if (reasonlist && reasonlist->reason)
			fprintf(f, " %s\n", reasonlist->reason);
		else
			fprintf(f, " %s\n", "NA");

		if (reasonlist) reasonlist = reasonlist->next;
	}

	fclose(f);
}

static void Ban_Add(const char *reason)
{
	banreason_t *reasonlist = malloc(sizeof(*reasonlist));

	if (!reasonlist)
		return;
	if (!reason)
		reason = "NA";

	reasonlist->next = NULL;
	reasonlist->reason = Z_StrDup(reason);
	if ((reasonlist->prev = reasontail) == NULL)
		reasonhead = reasonlist;
	else
		reasontail->next = reasonlist;
	reasontail = reasonlist;
}

static void Command_ClearBans(void)
{
	banreason_t *temp;

	if (!I_ClearBans)
		return;

	I_ClearBans();
	D_SaveBan();
	reasontail = NULL;
	while (reasonhead)
	{
		temp = reasonhead->next;
		Z_Free(reasonhead->reason);
		free(reasonhead);
		reasonhead = temp;
	}
}

static void Ban_Load_File(boolean warning)
{
	FILE *f;
	size_t i;
	const char *address, *mask;
	char buffer[MAX_WADPATH];

	f = fopen(va("%s"PATHSEP"%s", srb2home, "ban.txt"), "r");

	if (!f)
	{
		if (warning)
			CONS_Alert(CONS_WARNING, M_GetText("Could not open ban.txt for ban list\n"));
		return;
	}

	if (I_ClearBans)
		Command_ClearBans();
	else
	{
		fclose(f);
		return;
	}

	for (i=0; fgets(buffer, (int)sizeof(buffer), f); i++)
	{
		address = strtok(buffer, " \t\r\n");
		mask = strtok(NULL, " \t\r\n");

		I_SetBanAddress(address, mask);

		Ban_Add(strtok(NULL, "\r\n"));
	}

	fclose(f);
}

static void Command_ReloadBan(void)  //recheck ban.txt
{
	Ban_Load_File(true);
}

static void Command_connect(void)
{
	// Assume we connect directly.
	boolean viams = false;

	if (COM_Argc() < 2 || *COM_Argv(1) == 0)
	{
		CONS_Printf(M_GetText(
			"Connect <serveraddress> (port): connect to a server\n"
			"Connect ANY: connect to the first lan server found\n"
			//"Connect SELF: connect to your own server.\n"
			));
		return;
	}

	if (Playing() || titledemo)
	{
		CONS_Printf(M_GetText("You cannot connect while in a game. End this game first.\n"));
		return;
	}

	// modified game check: no longer handled
	// we don't request a restart unless the filelist differs

	server = false;
/*
	if (!stricmp(COM_Argv(1), "self"))
	{
		servernode = 0;
		server = true;
		/// \bug should be but...
		//SV_SpawnServer();
	}
	else
*/
	{
		// used in menu to connect to a server in the list
		if (netgame && !stricmp(COM_Argv(1), "node"))
		{
			servernode = (SINT8)atoi(COM_Argv(2));

			// Use MS to traverse NAT firewalls.
			viams = true;
		}
		else if (netgame)
		{
			CONS_Printf(M_GetText("You cannot connect while in a game. End this game first.\n"));
			return;
		}
		else if (I_NetOpenSocket)
		{
			MSCloseUDPSocket(); // Tidy up before wiping the slate.
			I_NetOpenSocket();
			netgame = true;
			multiplayer = true;

			if (!stricmp(COM_Argv(1), "any"))
				servernode = BROADCASTADDR;
			else if (I_NetMakeNodewPort)
			{
				if (COM_Argc() >= 3) // address AND port
					servernode = I_NetMakeNodewPort(COM_Argv(1), COM_Argv(2));
				else // address only, or address:port
					servernode = I_NetMakeNode(COM_Argv(1));
			}
			else
			{
				CONS_Alert(CONS_ERROR, M_GetText("There is no server identification with this network driver\n"));
				D_CloseConnection();
				return;
			}
		}
		else
			CONS_Alert(CONS_ERROR, M_GetText("There is no network driver\n"));
	}

	splitscreen = false;
	SplitScreen_OnChange();
	botingame = false;
	botskin = 0;
	CL_ConnectToServer(viams);
}
#endif

static void ResetNode(INT32 node);

//
// CL_ClearPlayer
//
// Clears the player data so that a future client can use this slot
//
void CL_ClearPlayer(INT32 playernum)
{
	if (players[playernum].mo)
		P_RemoveMobj(players[playernum].mo);
	memset(&players[playernum], 0, sizeof (player_t));
	memset(playeraddress[playernum], 0, sizeof(*playeraddress));
}

//
// CL_RemovePlayer
//
// Removes a player from the current game
//
static void CL_RemovePlayer(INT32 playernum, kickreason_t reason)
{
	// Sanity check: exceptional cases (i.e. c-fails) can cause multiple
	// kick commands to be issued for the same player.
	if (!playeringame[playernum])
		return;

	if (server && !demoplayback && playernode[playernum] != UINT8_MAX)
	{
		INT32 node = playernode[playernum];
		playerpernode[node]--;
		if (playerpernode[node] <= 0)
		{
			nodeingame[node] = false;
			Net_CloseConnection(node);
			ResetNode(node);
		}
	}

	if (gametyperules & GTR_TEAMFLAGS)
		P_PlayerFlagBurst(&players[playernum], false); // Don't take the flag with you!

	// If in a special stage, redistribute the player's spheres across
	// the remaining players.
	if (G_IsSpecialStage(gamemap))
	{
		INT32 i, count, sincrement, spheres, rincrement, rings;

		for (i = 0, count = 0; i < MAXPLAYERS; i++)
		{
			if (playeringame[i])
				count++;
		}

		count--;
		spheres = players[playernum].spheres;
		rings = players[playernum].rings;
		sincrement = spheres/count;
		rincrement = rings/count;

		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (playeringame[i] && i != playernum)
			{
				if (spheres < 2*sincrement)
				{
					P_GivePlayerSpheres(&players[i], spheres);
					spheres = 0;
				}
				else
				{
					P_GivePlayerSpheres(&players[i], sincrement);
					spheres -= sincrement;
				}

				if (rings < 2*rincrement)
				{
					P_GivePlayerRings(&players[i], rings);
					rings = 0;
				}
				else
				{
					P_GivePlayerRings(&players[i], rincrement);
					rings -= rincrement;
				}
			}
		}
	}

	LUAh_PlayerQuit(&players[playernum], reason); // Lua hook for player quitting

	// don't look through someone's view who isn't there
	if (playernum == displayplayer)
	{
		// Call ViewpointSwitch hooks here.
		// The viewpoint was forcibly changed.
		LUAh_ViewpointSwitch(&players[consoleplayer], &players[consoleplayer], true);
		displayplayer = consoleplayer;
	}

	// Reset player data
	CL_ClearPlayer(playernum);

	// remove avatar of player
	playeringame[playernum] = false;
	playernode[playernum] = UINT8_MAX;
	while (!playeringame[doomcom->numslots-1] && doomcom->numslots > 1)
		doomcom->numslots--;

	// Reset the name
	sprintf(player_names[playernum], "Player %d", playernum+1);

	if (IsPlayerAdmin(playernum))
	{
		RemoveAdminPlayer(playernum); // don't stay admin after you're gone
	}

	LUA_InvalidatePlayer(&players[playernum]);

	if (G_TagGametype()) //Check if you still have a game. Location flexible. =P
		P_CheckSurvivors();
	else if (gametyperules & GTR_RACE)
		P_CheckRacers();
}

void CL_Reset(void)
{
	if (metalrecording)
		G_StopMetalRecording(false);
	if (metalplayback)
		G_StopMetalDemo();
	if (demorecording)
		G_CheckDemoStatus();

	// reset client/server code
	DEBFILE(va("\n-=-=-=-=-=-=-= Client reset =-=-=-=-=-=-=-\n\n"));

	if (servernode > 0 && servernode < MAXNETNODES)
	{
		nodeingame[(UINT8)servernode] = false;
		Net_CloseConnection(servernode);
	}
	D_CloseConnection(); // netgame = false
	multiplayer = false;
	servernode = 0;
	server = true;
	resynch_local_inprogress = false;
	doomcom->numnodes = 1;
	doomcom->numslots = 1;
	SV_StopServer();
	SV_ResetServer();

	// make sure we don't leave any fileneeded gunk over from a failed join
	fileneedednum = 0;
	memset(fileneeded, 0, sizeof(fileneeded));

	// D_StartTitle should get done now, but the calling function will handle it
}

#ifndef NONET
static void Command_GetPlayerNum(void)
{
	INT32 i;

	for (i = 0; i < MAXPLAYERS; i++)
		if (playeringame[i])
		{
			if (serverplayer == i)
				CONS_Printf(M_GetText("num:%2d  node:%2d  %s\n"), i, playernode[i], player_names[i]);
			else
				CONS_Printf(M_GetText("\x82num:%2d  node:%2d  %s\n"), i, playernode[i], player_names[i]);
		}
}

SINT8 nametonum(const char *name)
{
	INT32 playernum, i;

	if (!strcmp(name, "0"))
		return 0;

	playernum = (SINT8)atoi(name);

	if (playernum < 0 || playernum >= MAXPLAYERS)
		return -1;

	if (playernum)
	{
		if (playeringame[playernum])
			return (SINT8)playernum;
		else
			return -1;
	}

	for (i = 0; i < MAXPLAYERS; i++)
		if (playeringame[i] && !stricmp(player_names[i], name))
			return (SINT8)i;

	CONS_Printf(M_GetText("There is no player named \"%s\"\n"), name);

	return -1;
}

/** Lists all players and their player numbers.
  *
  * \sa Command_GetPlayerNum
  */
static void Command_Nodes(void)
{
	INT32 i;
	size_t maxlen = 0;
	const char *address;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		const size_t plen = strlen(player_names[i]);
		if (playeringame[i] && plen > maxlen)
			maxlen = plen;
	}

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (playeringame[i])
		{
			CONS_Printf("%.2u: %*s", i, (int)maxlen, player_names[i]);

			if (playernode[i] != UINT8_MAX)
			{
				CONS_Printf(" - node %.2d", playernode[i]);
				if (I_GetNodeAddress && (address = I_GetNodeAddress(playernode[i])) != NULL)
					CONS_Printf(" - %s", address);
			}

			if (IsPlayerAdmin(i))
				CONS_Printf(M_GetText(" (verified admin)"));

			if (players[i].spectator)
				CONS_Printf(M_GetText(" (spectator)"));

			CONS_Printf("\n");
		}
	}
}

static void Command_Ban(void)
{
	if (COM_Argc() < 2)
	{
		CONS_Printf(M_GetText("Ban <playername/playernum> <reason>: ban and kick a player\n"));
		return;
	}

	if (!netgame) // Don't kick Tails in splitscreen!
	{
		CONS_Printf(M_GetText("This only works in a netgame.\n"));
		return;
	}

	if (server || IsPlayerAdmin(consoleplayer))
	{
		UINT8 buf[3 + MAX_REASONLENGTH];
		UINT8 *p = buf;
		const SINT8 pn = nametonum(COM_Argv(1));
		const INT32 node = playernode[(INT32)pn];

		if (pn == -1 || pn == 0)
			return;

		WRITEUINT8(p, pn);

		if (server && I_Ban && !I_Ban(node)) // only the server is allowed to do this right now
		{
			CONS_Alert(CONS_WARNING, M_GetText("Too many bans! Geez, that's a lot of people you're excluding...\n"));
			WRITEUINT8(p, KICK_MSG_GO_AWAY);
			SendNetXCmd(XD_KICK, &buf, 2);
		}
		else
		{
			if (server) // only the server is allowed to do this right now
			{
				Ban_Add(COM_Argv(2));
				D_SaveBan(); // save the ban list
			}

			if (COM_Argc() == 2)
			{
				WRITEUINT8(p, KICK_MSG_BANNED);
				SendNetXCmd(XD_KICK, &buf, 2);
			}
			else
			{
				size_t i, j = COM_Argc();
				char message[MAX_REASONLENGTH];

				//Steal from the motd code so you don't have to put the reason in quotes.
				strlcpy(message, COM_Argv(2), sizeof message);
				for (i = 3; i < j; i++)
				{
					strlcat(message, " ", sizeof message);
					strlcat(message, COM_Argv(i), sizeof message);
				}

				WRITEUINT8(p, KICK_MSG_CUSTOM_BAN);
				WRITESTRINGN(p, message, MAX_REASONLENGTH);
				SendNetXCmd(XD_KICK, &buf, p - buf);
			}
		}
	}
	else
		CONS_Printf(M_GetText("Only the server or a remote admin can use this.\n"));

}

static void Command_BanIP(void)
{
	if (COM_Argc() < 2)
	{
		CONS_Printf(M_GetText("banip <ip> <reason>: ban an ip address\n"));
		return;
	}

	if (server) // Only the server can use this, otherwise does nothing.
	{
		const char *address = (COM_Argv(1));
		const char *reason;

		if (COM_Argc() == 2)
			reason = NULL;
		else
			reason = COM_Argv(2);


		if (I_SetBanAddress && I_SetBanAddress(address, NULL))
		{
			if (reason)
				CONS_Printf("Banned IP address %s for: %s\n", address, reason);
			else
				CONS_Printf("Banned IP address %s\n", address);

			Ban_Add(reason);
			D_SaveBan();
		}
		else
		{
			return;
		}
	}
}

static void Command_Kick(void)
{
	if (COM_Argc() < 2)
	{
		CONS_Printf(M_GetText("kick <playername/playernum> <reason>: kick a player\n"));
		return;
	}

	if (!netgame) // Don't kick Tails in splitscreen!
	{
		CONS_Printf(M_GetText("This only works in a netgame.\n"));
		return;
	}

	if (server || IsPlayerAdmin(consoleplayer))
	{
		UINT8 buf[3 + MAX_REASONLENGTH];
		UINT8 *p = buf;
		const SINT8 pn = nametonum(COM_Argv(1));

		if (pn == -1 || pn == 0)
			return;

		// Special case if we are trying to kick a player who is downloading the game state:
		// trigger a timeout instead of kicking them, because a kick would only
		// take effect after they have finished downloading
		if (server && playernode[pn] != UINT8_MAX && sendingsavegame[playernode[pn]])
		{
			Net_ConnectionTimeout(playernode[pn]);
			return;
		}

		WRITESINT8(p, pn);

		if (COM_Argc() == 2)
		{
			WRITEUINT8(p, KICK_MSG_GO_AWAY);
			SendNetXCmd(XD_KICK, &buf, 2);
		}
		else
		{
			size_t i, j = COM_Argc();
			char message[MAX_REASONLENGTH];

			//Steal from the motd code so you don't have to put the reason in quotes.
			strlcpy(message, COM_Argv(2), sizeof message);
			for (i = 3; i < j; i++)
			{
				strlcat(message, " ", sizeof message);
				strlcat(message, COM_Argv(i), sizeof message);
			}

			WRITEUINT8(p, KICK_MSG_CUSTOM_KICK);
			WRITESTRINGN(p, message, MAX_REASONLENGTH);
			SendNetXCmd(XD_KICK, &buf, p - buf);
		}
	}
	else
		CONS_Printf(M_GetText("Only the server or a remote admin can use this.\n"));
}
#endif

static void Got_KickCmd(UINT8 **p, INT32 playernum)
{
	INT32 pnum, msg;
	char buf[3 + MAX_REASONLENGTH];
	char *reason = buf;
	kickreason_t kickreason = KR_KICK;
	boolean keepbody;

	pnum = READUINT8(*p);
	msg = READUINT8(*p);
	keepbody = (msg & KICK_MSG_KEEP_BODY) != 0;
	msg &= ~KICK_MSG_KEEP_BODY;

	if (pnum == serverplayer && IsPlayerAdmin(playernum))
	{
		CONS_Printf(M_GetText("Server is being shut down remotely. Goodbye!\n"));

		if (server)
			COM_BufAddText("quit\n");

		return;
	}

	// Is playernum authorized to make this kick?
	if (playernum != serverplayer && !IsPlayerAdmin(playernum)
		&& !(playernode[playernum] != UINT8_MAX && playerpernode[playernode[playernum]] == 2
		&& nodetoplayer2[playernode[playernum]] == pnum))
	{
		// We received a kick command from someone who isn't the
		// server or admin, and who isn't in splitscreen removing
		// player 2. Thus, it must be someone with a modified
		// binary, trying to kick someone but without having
		// authorization.

		// We deal with this by changing the kick reason to
		// "consistency failure" and kicking the offending user
		// instead.

		// Note: Splitscreen in netgames is broken because of
		// this. Only the server has any idea of which players
		// are using splitscreen on the same computer, so
		// clients cannot always determine if a kick is
		// legitimate.

		CONS_Alert(CONS_WARNING, M_GetText("Illegal kick command received from %s for player %d\n"), player_names[playernum], pnum);

		// In debug, print a longer message with more details.
		// TODO Callum: Should we translate this?
/*
		CONS_Debug(DBG_NETPLAY,
			"So, you must be asking, why is this an illegal kick?\n"
			"Well, let's take a look at the facts, shall we?\n"
			"\n"
			"playernum (this is the guy who did it), he's %d.\n"
			"pnum (the guy he's trying to kick) is %d.\n"
			"playernum's node is %d.\n"
			"That node has %d players.\n"
			"Player 2 on that node is %d.\n"
			"pnum's node is %d.\n"
			"That node has %d players.\n"
			"Player 2 on that node is %d.\n"
			"\n"
			"If you think this is a bug, please report it, including all of the details above.\n",
				playernum, pnum,
				playernode[playernum], playerpernode[playernode[playernum]],
				nodetoplayer2[playernode[playernum]],
				playernode[pnum], playerpernode[playernode[pnum]],
				nodetoplayer2[playernode[pnum]]);
*/
		pnum = playernum;
		msg = KICK_MSG_CON_FAIL;
		keepbody = true;
	}

	//CONS_Printf("\x82%s ", player_names[pnum]);

	// If a verified admin banned someone, the server needs to know about it.
	// If the playernum isn't zero (the server) then the server needs to record the ban.
	if (server && playernum && (msg == KICK_MSG_BANNED || msg == KICK_MSG_CUSTOM_BAN))
	{
		if (I_Ban && !I_Ban(playernode[(INT32)pnum]))
			CONS_Alert(CONS_WARNING, M_GetText("Too many bans! Geez, that's a lot of people you're excluding...\n"));
#ifndef NONET
		else
			Ban_Add(reason);
#endif
	}

	switch (msg)
	{
		case KICK_MSG_GO_AWAY:
			if (!players[pnum].quittime)
				HU_AddChatText(va("\x82*%s has been kicked (Go away)", player_names[pnum]), false);
			kickreason = KR_KICK;
			break;
		case KICK_MSG_PING_HIGH:
			HU_AddChatText(va("\x82*%s left the game (Broke ping limit)", player_names[pnum]), false);
			kickreason = KR_PINGLIMIT;
			break;
		case KICK_MSG_CON_FAIL:
			HU_AddChatText(va("\x82*%s left the game (Synch Failure)", player_names[pnum]), false);
			kickreason = KR_SYNCH;

			if (M_CheckParm("-consisdump")) // Helps debugging some problems
			{
				INT32 i;

				CONS_Printf(M_GetText("Player kicked is #%d, dumping consistency...\n"), pnum);

				for (i = 0; i < MAXPLAYERS; i++)
				{
					if (!playeringame[i])
						continue;
					CONS_Printf("-------------------------------------\n");
					CONS_Printf("Player %d: %s\n", i, player_names[i]);
					CONS_Printf("Skin: %d\n", players[i].skin);
					CONS_Printf("Color: %d\n", players[i].skincolor);
					CONS_Printf("Speed: %d\n",players[i].speed>>FRACBITS);
					if (players[i].mo)
					{
						if (!players[i].mo->skin)
							CONS_Printf("Mobj skin: NULL!\n");
						else
							CONS_Printf("Mobj skin: %s\n", ((skin_t *)players[i].mo->skin)->name);
						CONS_Printf("Position: %d, %d, %d\n", players[i].mo->x, players[i].mo->y, players[i].mo->z);
						if (!players[i].mo->state)
							CONS_Printf("State: S_NULL\n");
						else
							CONS_Printf("State: %d\n", (statenum_t)(players[i].mo->state-states));
					}
					else
						CONS_Printf("Mobj: NULL\n");
					CONS_Printf("-------------------------------------\n");
				}
			}
			break;
		case KICK_MSG_TIMEOUT:
			HU_AddChatText(va("\x82*%s left the game (Connection timeout)", player_names[pnum]), false);
			kickreason = KR_TIMEOUT;
			break;
		case KICK_MSG_PLAYER_QUIT:
			if (netgame && !players[pnum].quittime) // not splitscreen/bots or soulless body
				HU_AddChatText(va("\x82*%s left the game", player_names[pnum]), false);
			kickreason = KR_LEAVE;
			break;
		case KICK_MSG_BANNED:
			HU_AddChatText(va("\x82*%s has been banned (Don't come back)", player_names[pnum]), false);
			kickreason = KR_BAN;
			break;
		case KICK_MSG_CUSTOM_KICK:
			READSTRINGN(*p, reason, MAX_REASONLENGTH+1);
			HU_AddChatText(va("\x82*%s has been kicked (%s)", player_names[pnum], reason), false);
			kickreason = KR_KICK;
			break;
		case KICK_MSG_CUSTOM_BAN:
			READSTRINGN(*p, reason, MAX_REASONLENGTH+1);
			HU_AddChatText(va("\x82*%s has been banned (%s)", player_names[pnum], reason), false);
			kickreason = KR_BAN;
			break;
	}

	if (pnum == consoleplayer)
	{
		if (Playing())
			LUAh_GameQuit();
#ifdef DUMPCONSISTENCY
		if (msg == KICK_MSG_CON_FAIL) SV_SavedGame();
#endif
		D_QuitNetGame();
		CL_Reset();
		D_StartTitle();
		if (msg == KICK_MSG_CON_FAIL)
			M_StartMessage(M_GetText("Server closed connection\n(synch failure)\nPress ESC\n"), NULL, MM_NOTHING);
		else if (msg == KICK_MSG_PING_HIGH)
			M_StartMessage(M_GetText("Server closed connection\n(Broke ping limit)\nPress ESC\n"), NULL, MM_NOTHING);
		else if (msg == KICK_MSG_BANNED)
			M_StartMessage(M_GetText("You have been banned by the server\n\nPress ESC\n"), NULL, MM_NOTHING);
		else if (msg == KICK_MSG_CUSTOM_KICK)
			M_StartMessage(va(M_GetText("You have been kicked\n(%s)\nPress ESC\n"), reason), NULL, MM_NOTHING);
		else if (msg == KICK_MSG_CUSTOM_BAN)
			M_StartMessage(va(M_GetText("You have been banned\n(%s)\nPress ESC\n"), reason), NULL, MM_NOTHING);
		else
			M_StartMessage(M_GetText("You have been kicked by the server\n\nPress ESC\n"), NULL, MM_NOTHING);
	}
	else if (keepbody)
	{
		if (server && !demoplayback && playernode[pnum] != UINT8_MAX)
		{
			INT32 node = playernode[pnum];
			playerpernode[node]--;
			if (playerpernode[node] <= 0)
			{
				nodeingame[node] = false;
				Net_CloseConnection(node);
				ResetNode(node);
			}
		}

		playernode[pnum] = UINT8_MAX;

		players[pnum].quittime = 1;
	}
	else
		CL_RemovePlayer(pnum, kickreason);
}

consvar_t cv_allownewplayer = {"allowjoin", "On", CV_NETVAR, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL	};
consvar_t cv_joinnextround = {"joinnextround", "Off", CV_NETVAR, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL}; /// \todo not done
static CV_PossibleValue_t maxplayers_cons_t[] = {{2, "MIN"}, {32, "MAX"}, {0, NULL}};
consvar_t cv_maxplayers = {"maxplayers", "8", CV_SAVE, maxplayers_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
static CV_PossibleValue_t joindelay_cons_t[] = {{1, "MIN"}, {3600, "MAX"}, {0, "Off"}, {0, NULL}};
consvar_t cv_joindelay = {"joindelay", "10", CV_SAVE, joindelay_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
static CV_PossibleValue_t rejointimeout_cons_t[] = {{1, "MIN"}, {60 * FRACUNIT, "MAX"}, {0, "Off"}, {0, NULL}};
consvar_t cv_rejointimeout = {"rejointimeout", "Off", CV_SAVE|CV_FLOAT, rejointimeout_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};

static CV_PossibleValue_t resynchattempts_cons_t[] = {{1, "MIN"}, {20, "MAX"}, {0, "No"}, {0, NULL}};
consvar_t cv_resynchattempts = {"resynchattempts", "10", CV_SAVE, resynchattempts_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL	};
consvar_t cv_blamecfail = {"blamecfail", "Off", CV_SAVE, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL	};

// max file size to send to a player (in kilobytes)
static CV_PossibleValue_t maxsend_cons_t[] = {{0, "MIN"}, {51200, "MAX"}, {0, NULL}};
consvar_t cv_maxsend = {"maxsend", "4096", CV_SAVE, maxsend_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_noticedownload = {"noticedownload", "Off", CV_SAVE, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};

// Speed of file downloading (in packets per tic)
static CV_PossibleValue_t downloadspeed_cons_t[] = {{0, "MIN"}, {32, "MAX"}, {0, NULL}};
consvar_t cv_downloadspeed = {"downloadspeed", "16", CV_SAVE, downloadspeed_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};

static void Got_AddPlayer(UINT8 **p, INT32 playernum);

// called one time at init
void D_ClientServerInit(void)
{
	DEBFILE(va("- - -== SRB2 v%d.%.2d.%d "VERSIONSTRING" debugfile ==- - -\n",
		VERSION/100, VERSION%100, SUBVERSION));

#ifndef NONET
	COM_AddCommand("getplayernum", Command_GetPlayerNum);
	COM_AddCommand("kick", Command_Kick);
	COM_AddCommand("ban", Command_Ban);
	COM_AddCommand("banip", Command_BanIP);
	COM_AddCommand("clearbans", Command_ClearBans);
	COM_AddCommand("showbanlist", Command_ShowBan);
	COM_AddCommand("reloadbans", Command_ReloadBan);
	COM_AddCommand("connect", Command_connect);
	COM_AddCommand("nodes", Command_Nodes);
#ifdef PACKETDROP
	COM_AddCommand("drop", Command_Drop);
	COM_AddCommand("droprate", Command_Droprate);
#endif
#ifdef _DEBUG
	COM_AddCommand("numnodes", Command_Numnodes);
#endif
#endif

	RegisterNetXCmd(XD_KICK, Got_KickCmd);
	RegisterNetXCmd(XD_ADDPLAYER, Got_AddPlayer);
#ifndef NONET
#ifdef DUMPCONSISTENCY
	CV_RegisterVar(&cv_dumpconsistency);
#endif
	Ban_Load_File(false);
#endif

	gametic = 0;
	localgametic = 0;

	// do not send anything before the real begin
	SV_StopServer();
	SV_ResetServer();
	if (dedicated)
		SV_SpawnServer();
}

static void ResetNode(INT32 node)
{
	nodeingame[node] = false;
	nodetoplayer[node] = -1;
	nodetoplayer2[node] = -1;
	nettics[node] = gametic;
	supposedtics[node] = gametic;
	nodewaiting[node] = 0;
	playerpernode[node] = 0;
	sendingsavegame[node] = false;
	SV_InitResynchVars(node);
}

void SV_ResetServer(void)
{
	INT32 i;

	// +1 because this command will be executed in com_executebuffer in
	// tryruntic so gametic will be incremented, anyway maketic > gametic
	// is not an issue

	maketic = gametic + 1;
	neededtic = maketic;
	tictoclear = maketic;

	joindelay = 0;

	for (i = 0; i < MAXNETNODES; i++)
		ResetNode(i);

	for (i = 0; i < MAXPLAYERS; i++)
	{
		LUA_InvalidatePlayer(&players[i]);
		playeringame[i] = false;
		playernode[i] = UINT8_MAX;
		memset(playeraddress[i], 0, sizeof(*playeraddress));
		sprintf(player_names[i], "Player %d", i + 1);
		adminplayers[i] = -1; // Populate the entire adminplayers array with -1.
	}

	mynode = 0;
	cl_packetmissed = false;

	if (dedicated)
	{
		nodeingame[0] = true;
		serverplayer = 0;
	}
	else
		serverplayer = consoleplayer;

	if (server)
		servernode = 0;

	doomcom->numslots = 0;

	// clear server_context
	memset(server_context, '-', 8);

	DEBFILE("\n-=-=-=-=-=-=-= Server Reset =-=-=-=-=-=-=-\n\n");
}

static inline void SV_GenContext(void)
{
	UINT8 i;
	// generate server_context, as exactly 8 bytes of randomly mixed A-Z and a-z
	// (hopefully M_Random is initialized!! if not this will be awfully silly!)
	for (i = 0; i < 8; i++)
	{
		const char a = M_RandomKey(26*2);
		if (a < 26) // uppercase
			server_context[i] = 'A'+a;
		else // lowercase
			server_context[i] = 'a'+(a-26);
	}
}

//
// D_QuitNetGame
// Called before quitting to leave a net game
// without hanging the other players
//
void D_QuitNetGame(void)
{
	if (!netgame || !netbuffer)
		return;

	DEBFILE("===========================================================================\n"
	        "                  Quitting Game, closing connection\n"
	        "===========================================================================\n");

	// abort send/receive of files
	CloseNetFile();
	RemoveAllLuaFileTransfers();
	waitingforluafiletransfer = false;
	waitingforluafilecommand = false;

	if (server)
	{
		INT32 i;

		netbuffer->packettype = PT_SERVERSHUTDOWN;
		for (i = 0; i < MAXNETNODES; i++)
			if (nodeingame[i])
				HSendPacket(i, true, 0, 0);
		if (serverrunning && ms_RoomId > 0)
			UnregisterServer();
	}
	else if (servernode > 0 && servernode < MAXNETNODES && nodeingame[(UINT8)servernode])
	{
		netbuffer->packettype = PT_CLIENTQUIT;
		HSendPacket(servernode, true, 0, 0);
	}

	D_CloseConnection();
	ClearAdminPlayers();

	DEBFILE("===========================================================================\n"
	        "                         Log finish\n"
	        "===========================================================================\n");
#ifdef DEBUGFILE
	if (debugfile)
	{
		fclose(debugfile);
		debugfile = NULL;
	}
#endif
}

// Adds a node to the game (player will follow at map change or at savegame....)
static inline void SV_AddNode(INT32 node)
{
	nettics[node] = gametic;
	supposedtics[node] = gametic;
	// little hack because the server connects to itself and puts
	// nodeingame when connected not here
	if (node)
		nodeingame[node] = true;
}

// Xcmd XD_ADDPLAYER
static void Got_AddPlayer(UINT8 **p, INT32 playernum)
{
	INT16 node, newplayernum;
	boolean splitscreenplayer;
	boolean rejoined;
	player_t *newplayer;

	if (playernum != serverplayer && !IsPlayerAdmin(playernum))
	{
		// protect against hacked/buggy client
		CONS_Alert(CONS_WARNING, M_GetText("Illegal add player command received from %s\n"), player_names[playernum]);
		if (server)
			SendKick(playernum, KICK_MSG_CON_FAIL | KICK_MSG_KEEP_BODY);
		return;
	}

	node = READUINT8(*p);
	newplayernum = READUINT8(*p);
	splitscreenplayer = newplayernum & 0x80;
	newplayernum &= ~0x80;

	rejoined = playeringame[newplayernum];

	if (!rejoined)
	{
		// Clear player before joining, lest some things get set incorrectly
		// HACK: don't do this for splitscreen, it relies on preset values
		if (!splitscreen && !botingame)
			CL_ClearPlayer(newplayernum);
		playeringame[newplayernum] = true;
		G_AddPlayer(newplayernum);
		if (newplayernum+1 > doomcom->numslots)
			doomcom->numslots = (INT16)(newplayernum+1);

		if (server && I_GetNodeAddress)
		{
			const char *address = I_GetNodeAddress(node);
			char *port = NULL;
			if (address) // MI: fix msvcrt.dll!_mbscat crash?
			{
				strcpy(playeraddress[newplayernum], address);
				port = strchr(playeraddress[newplayernum], ':');
				if (port)
					*port = '\0';
			}
		}
	}

	newplayer = &players[newplayernum];

	newplayer->jointime = 0;
	newplayer->quittime = 0;

	READSTRINGN(*p, player_names[newplayernum], MAXPLAYERNAME);

	// the server is creating my player
	if (node == mynode)
	{
		playernode[newplayernum] = 0; // for information only
		if (!splitscreenplayer)
		{
			consoleplayer = newplayernum;
			displayplayer = newplayernum;
			secondarydisplayplayer = newplayernum;
			DEBFILE("spawning me\n");
			ticcmd_oldangleturn[0] = newplayer->oldrelangleturn;
		}
		else
		{
			secondarydisplayplayer = newplayernum;
			DEBFILE("spawning my brother\n");
			if (botingame)
				newplayer->bot = 1;
			ticcmd_oldangleturn[1] = newplayer->oldrelangleturn;
		}
		P_ForceLocalAngle(newplayer, (angle_t)(newplayer->angleturn << 16));
		D_SendPlayerConfig();
		addedtogame = true;

		if (rejoined)
		{
			if (newplayer->mo)
			{
				newplayer->viewheight = 41*newplayer->height/48;

				if (newplayer->mo->eflags & MFE_VERTICALFLIP)
					newplayer->viewz = newplayer->mo->z + newplayer->mo->height - newplayer->viewheight;
				else
					newplayer->viewz = newplayer->mo->z + newplayer->viewheight;
			}

			// wake up the status bar
			ST_Start();
			// wake up the heads up text
			HU_Start();

			if (camera.chase && !splitscreenplayer)
				P_ResetCamera(newplayer, &camera);
			if (camera2.chase && splitscreenplayer)
				P_ResetCamera(newplayer, &camera2);
		}
	}

	if (netgame)
	{
		char joinmsg[256];

		if (rejoined)
			strcpy(joinmsg, M_GetText("\x82*%s has rejoined the game (player %d)"));
		else
			strcpy(joinmsg, M_GetText("\x82*%s has joined the game (player %d)"));
		strcpy(joinmsg, va(joinmsg, player_names[newplayernum], newplayernum));

		// Merge join notification + IP to avoid clogging console/chat
		if (server && cv_showjoinaddress.value && I_GetNodeAddress)
		{
			const char *address = I_GetNodeAddress(node);
			if (address)
				strcat(joinmsg, va(" (%s)", address));
		}

		HU_AddChatText(joinmsg, false);
	}

	if (server && multiplayer && motd[0] != '\0')
		COM_BufAddText(va("sayto %d %s\n", newplayernum, motd));

	if (!rejoined)
		LUAh_PlayerJoin(newplayernum);
}

static boolean SV_AddWaitingPlayers(const char *name, const char *name2)
{
	INT32 node, n, newplayer = false;
	UINT8 buf[2 + MAXPLAYERNAME];
	UINT8 *p;
	INT32 newplayernum;

	for (node = 0; node < MAXNETNODES; node++)
	{
		// splitscreen can allow 2 player in one node
		for (; nodewaiting[node] > 0; nodewaiting[node]--)
		{
			newplayer = true;

			newplayernum = FindRejoinerNum(node);
			if (newplayernum == -1)
			{
				// search for a free playernum
				// we can't use playeringame since it is not updated here
				for (newplayernum = dedicated ? 1 : 0; newplayernum < MAXPLAYERS; newplayernum++)
				{
					if (playeringame[newplayernum])
						continue;
					for (n = 0; n < MAXNETNODES; n++)
						if (nodetoplayer[n] == newplayernum || nodetoplayer2[n] == newplayernum)
							break;
					if (n == MAXNETNODES)
						break;
				}
			}

			// should never happen since we check the playernum
			// before accepting the join
			I_Assert(newplayernum < MAXPLAYERS);

			playernode[newplayernum] = (UINT8)node;

			p = buf + 2;
			buf[0] = (UINT8)node;
			buf[1] = newplayernum;
			if (playerpernode[node] < 1)
			{
				nodetoplayer[node] = newplayernum;
				WRITESTRINGN(p, name, MAXPLAYERNAME);
			}
			else
			{
				nodetoplayer2[node] = newplayernum;
				buf[1] |= 0x80;
				WRITESTRINGN(p, name2, MAXPLAYERNAME);
			}
			playerpernode[node]++;

			SendNetXCmd(XD_ADDPLAYER, &buf, p - buf);

			DEBFILE(va("Server added player %d node %d\n", newplayernum, node));
		}
	}

	return newplayer;
}

void CL_AddSplitscreenPlayer(void)
{
	if (cl_mode == CL_CONNECTED)
		CL_SendJoin();
}

void CL_RemoveSplitscreenPlayer(void)
{
	if (cl_mode != CL_CONNECTED)
		return;

	SendKick(secondarydisplayplayer, KICK_MSG_PLAYER_QUIT);
}

// is there a game running
boolean Playing(void)
{
	return (server && serverrunning) || (client && cl_mode == CL_CONNECTED);
}

boolean SV_SpawnServer(void)
{
	if (demoplayback)
		G_StopDemo(); // reset engine parameter
	if (metalplayback)
		G_StopMetalDemo();

	if (!serverrunning)
	{
		CONS_Printf(M_GetText("Starting Server....\n"));
		serverrunning = true;
		SV_ResetServer();
		SV_GenContext();
		if (netgame && I_NetOpenSocket)
		{
			MSCloseUDPSocket();		// Tidy up before wiping the slate.
			I_NetOpenSocket();
			if (ms_RoomId > 0)
				RegisterServer();
		}

		// non dedicated server just connect to itself
		if (!dedicated)
			CL_ConnectToServer(false);
		else doomcom->numslots = 1;
	}

	return SV_AddWaitingPlayers(cv_playername.zstring, cv_playername2.zstring);
}

void SV_StopServer(void)
{
	tic_t i;

	if (gamestate == GS_INTERMISSION)
		Y_EndIntermission();
	gamestate = wipegamestate = GS_NULL;

	localtextcmd[0] = 0;
	localtextcmd2[0] = 0;

	for (i = firstticstosend; i < firstticstosend + BACKUPTICS; i++)
		D_Clearticcmd(i);

	consoleplayer = 0;
	cl_mode = CL_SEARCHING;
	maketic = gametic+1;
	neededtic = maketic;
	serverrunning = false;
}

// called at singleplayer start and stopdemo
void SV_StartSinglePlayerServer(void)
{
	server = true;
	netgame = false;
	multiplayer = false;
	G_SetGametype(GT_COOP);

	// no more tic the game with this settings!
	SV_StopServer();

	if (splitscreen)
		multiplayer = true;
}

static void SV_SendRefuse(INT32 node, const char *reason)
{
	strcpy(netbuffer->u.serverrefuse.reason, reason);

	netbuffer->packettype = PT_SERVERREFUSE;
	HSendPacket(node, true, 0, strlen(netbuffer->u.serverrefuse.reason) + 1);
	Net_CloseConnection(node);
}

// used at txtcmds received to check packetsize bound
static size_t TotalTextCmdPerTic(tic_t tic)
{
	INT32 i;
	size_t total = 1; // num of textcmds in the tic (ntextcmd byte)

	for (i = 0; i < MAXPLAYERS; i++)
	{
		UINT8 *textcmd = D_GetExistingTextcmd(tic, i);
		if ((!i || playeringame[i]) && textcmd)
			total += 2 + textcmd[0]; // "+2" for size and playernum
	}

	return total;
}

/** Called when a PT_CLIENTJOIN packet is received
  *
  * \param node The packet sender
  *
  */
static void HandleConnect(SINT8 node)
{
	char names[MAXSPLITSCREENPLAYERS][MAXPLAYERNAME + 1];
	INT32 rejoinernum;
	INT32 i;

	rejoinernum = FindRejoinerNum(node);

	if (bannednode && bannednode[node])
		SV_SendRefuse(node, M_GetText("You have been banned\nfrom the server."));
	else if (netbuffer->u.clientcfg._255 != 255 ||
			netbuffer->u.clientcfg.packetversion != PACKETVERSION)
		SV_SendRefuse(node, "Incompatible packet formats.");
	else if (strncmp(netbuffer->u.clientcfg.application, SRB2APPLICATION,
				sizeof netbuffer->u.clientcfg.application))
		SV_SendRefuse(node, "Different SRB2 modifications\nare not compatible.");
	else if (netbuffer->u.clientcfg.version != VERSION
		|| netbuffer->u.clientcfg.subversion != SUBVERSION)
		SV_SendRefuse(node, va(M_GetText("Different SRB2 versions cannot\nplay a netgame!\n(server version %d.%d.%d)"), VERSION/100, VERSION%100, SUBVERSION));
	else if (!cv_allownewplayer.value && node && rejoinernum == -1)
		SV_SendRefuse(node, M_GetText("The server is not accepting\njoins for the moment."));
	else if (D_NumPlayers() >= cv_maxplayers.value && rejoinernum == -1)
		SV_SendRefuse(node, va(M_GetText("Maximum players reached: %d"), cv_maxplayers.value));
	else if (netgame && netbuffer->u.clientcfg.localplayers > 1) // Hacked client?
		SV_SendRefuse(node, M_GetText("Too many players from\nthis node."));
	else if (netgame && !netbuffer->u.clientcfg.localplayers) // Stealth join?
		SV_SendRefuse(node, M_GetText("No players from\nthis node."));
	else if (luafiletransfers)
		SV_SendRefuse(node, M_GetText("The server is broadcasting a file\nrequested by a Lua script.\nPlease wait a bit and then\ntry rejoining."));
	else if (netgame && joindelay > 2 * (tic_t)cv_joindelay.value * TICRATE)
		SV_SendRefuse(node, va(M_GetText("Too many people are connecting.\nPlease wait %d seconds and then\ntry rejoining."),
			(joindelay - 2 * cv_joindelay.value * TICRATE) / TICRATE));
	else
	{
#ifndef NONET
		boolean newnode = false;
#endif

		for (i = 0; i < netbuffer->u.clientcfg.localplayers - playerpernode[node]; i++)
		{
			strlcpy(names[i], netbuffer->u.clientcfg.names[i], MAXPLAYERNAME + 1);
			if (!EnsurePlayerNameIsGood(names[i], rejoinernum))
			{
				SV_SendRefuse(node, "Bad player name");
				return;
			}
		}

		// client authorised to join
		nodewaiting[node] = (UINT8)(netbuffer->u.clientcfg.localplayers - playerpernode[node]);
		if (!nodeingame[node])
		{
			gamestate_t backupstate = gamestate;
#ifndef NONET
			newnode = true;
#endif
			SV_AddNode(node);

			/// \note Wait what???
			///       What if the gamestate takes more than one second to get downloaded?
			///       Or if a lagspike happens?
			// you get a free second before desynch checks. use it wisely.
			SV_InitResynchVars(node);

			if (cv_joinnextround.value && gameaction == ga_nothing)
				G_SetGamestate(GS_WAITINGPLAYERS);
			if (!SV_SendServerConfig(node))
			{
				G_SetGamestate(backupstate);
				/// \note Shouldn't SV_SendRefuse be called before ResetNode?
				ResetNode(node);
				SV_SendRefuse(node, M_GetText("Server couldn't send info, please try again"));
				/// \todo fix this !!!
				return; // restart the while
			}
			//if (gamestate != GS_LEVEL) // GS_INTERMISSION, etc?
			//	SV_SendPlayerConfigs(node); // send bare minimum player info
			G_SetGamestate(backupstate);
			DEBFILE("new node joined\n");
		}
#ifndef NONET
		if (nodewaiting[node])
		{
			if ((gamestate == GS_LEVEL || gamestate == GS_INTERMISSION) && newnode)
			{
				SV_SendSaveGame(node); // send a complete game state
				DEBFILE("send savegame\n");
			}
			SV_AddWaitingPlayers(names[0], names[1]);
			joindelay += cv_joindelay.value * TICRATE;
			player_joining = true;
		}
#endif
	}
}

/** Called when a PT_SERVERSHUTDOWN packet is received
  *
  * \param node The packet sender (should be the server)
  *
  */
static void HandleShutdown(SINT8 node)
{
	(void)node;
	if (Playing())
		LUAh_GameQuit();
	D_QuitNetGame();
	CL_Reset();
	D_StartTitle();
	M_StartMessage(M_GetText("Server has shutdown\n\nPress Esc\n"), NULL, MM_NOTHING);
}

/** Called when a PT_NODETIMEOUT packet is received
  *
  * \param node The packet sender (should be the server)
  *
  */
static void HandleTimeout(SINT8 node)
{
	(void)node;
	if (Playing())
		LUAh_GameQuit();
	D_QuitNetGame();
	CL_Reset();
	D_StartTitle();
	M_StartMessage(M_GetText("Server Timeout\n\nPress Esc\n"), NULL, MM_NOTHING);
}

#ifndef NONET
/** Called when a PT_SERVERINFO packet is received
  *
  * \param node The packet sender
  * \note What happens if the packet comes from a client or something like that?
  *
  */
static void HandleServerInfo(SINT8 node)
{
	// compute ping in ms
	const tic_t ticnow = I_GetTime();
	const tic_t ticthen = (tic_t)LONG(netbuffer->u.serverinfo.time);
	const tic_t ticdiff = (ticnow - ticthen)*1000/NEWTICRATE;
	netbuffer->u.serverinfo.time = (tic_t)LONG(ticdiff);
	netbuffer->u.serverinfo.servername[MAXSERVERNAME-1] = 0;
	netbuffer->u.serverinfo.application
		[sizeof netbuffer->u.serverinfo.application - 1] = '\0';
	netbuffer->u.serverinfo.gametypename
		[sizeof netbuffer->u.serverinfo.gametypename - 1] = '\0';

	SL_InsertServer(&netbuffer->u.serverinfo, node);
}
#endif

/** Handles a packet received from a node that isn't in game
  *
  * \param node The packet sender
  * \todo Choose a better name, as the packet can also come from the server apparently?
  * \sa HandlePacketFromPlayer
  * \sa GetPackets
  *
  */
static void HandlePacketFromAwayNode(SINT8 node)
{
	if (node != servernode)
		DEBFILE(va("Received packet from unknown host %d\n", node));

// macro for packets that should only be sent by the server
// if it is NOT from the server, bail out and close the connection!
#define SERVERONLY \
			if (node != servernode) \
			{ \
				Net_CloseConnection(node); \
				break; \
			}
	switch (netbuffer->packettype)
	{
		case PT_ASKINFOVIAMS:
#if 0
			if (server && serverrunning)
			{
				INT32 clientnode;
				if (ms_RoomId < 0) // ignore if we're not actually on the MS right now
				{
					Net_CloseConnection(node); // and yes, close connection
					return;
				}
				clientnode = I_NetMakeNode(netbuffer->u.msaskinfo.clientaddr);
				if (clientnode != -1)
				{
					SV_SendServerInfo(clientnode, (tic_t)LONG(netbuffer->u.msaskinfo.time));
					SV_SendPlayerInfo(clientnode); // Send extra info
					Net_CloseConnection(clientnode);
					// Don't close connection to MS...
				}
				else
					Net_CloseConnection(node); // ...unless the IP address is not valid
			}
			else
				Net_CloseConnection(node); // you're not supposed to get it, so ignore it
#else
			Net_CloseConnection(node);
#endif
			break;

		case PT_ASKINFO:
			if (server && serverrunning)
			{
				SV_SendServerInfo(node, (tic_t)LONG(netbuffer->u.askinfo.time));
				SV_SendPlayerInfo(node); // Send extra info
			}
			Net_CloseConnection(node);
			break;

		case PT_SERVERREFUSE: // Negative response of client join request
			if (server && serverrunning)
			{ // But wait I thought I'm the server?
				Net_CloseConnection(node);
				break;
			}
			SERVERONLY
			if (cl_mode == CL_WAITJOINRESPONSE)
			{
				// Save the reason so it can be displayed after quitting the netgame
				char *reason = strdup(netbuffer->u.serverrefuse.reason);
				if (!reason)
					I_Error("Out of memory!\n");

				D_QuitNetGame();
				CL_Reset();
				D_StartTitle();

				M_StartMessage(va(M_GetText("Server refuses connection\n\nReason:\n%s"),
					reason), NULL, MM_NOTHING);

				free(reason);

				// Will be reset by caller. Signals refusal.
				cl_mode = CL_ABORTED;
			}
			break;

		case PT_SERVERCFG: // Positive response of client join request
		{
			INT32 j;
			UINT8 *scp;

			if (server && serverrunning && node != servernode)
			{ // but wait I thought I'm the server?
				Net_CloseConnection(node);
				break;
			}
			SERVERONLY
			/// \note how would this happen? and is it doing the right thing if it does?
			if (cl_mode != CL_WAITJOINRESPONSE)
				break;

			if (client)
			{
				maketic = gametic = neededtic = (tic_t)LONG(netbuffer->u.servercfg.gametic);
				G_SetGametype(netbuffer->u.servercfg.gametype);
				modifiedgame = netbuffer->u.servercfg.modifiedgame;
				for (j = 0; j < MAXPLAYERS; j++)
					adminplayers[j] = netbuffer->u.servercfg.adminplayers[j];
				memcpy(server_context, netbuffer->u.servercfg.server_context, 8);
			}

			nodeingame[(UINT8)servernode] = true;
			serverplayer = netbuffer->u.servercfg.serverplayer;
			doomcom->numslots = SHORT(netbuffer->u.servercfg.totalslotnum);
			mynode = netbuffer->u.servercfg.clientnode;
			if (serverplayer >= 0)
				playernode[(UINT8)serverplayer] = servernode;

			if (netgame)
#ifndef NONET
				CONS_Printf(M_GetText("Join accepted, waiting for complete game state...\n"));
#else
				CONS_Printf(M_GetText("Join accepted, waiting for next level change...\n"));
#endif
			DEBFILE(va("Server accept join gametic=%u mynode=%d\n", gametic, mynode));

			memset(playeringame, 0, sizeof(playeringame));
			for (j = 0; j < MAXPLAYERS; j++)
			{
				if (netbuffer->u.servercfg.playerskins[j] == 0xFF
				 && netbuffer->u.servercfg.playercolor[j] == 0xFFFF
				 && netbuffer->u.servercfg.playeravailabilities[j] == 0xFFFFFFFF)
					continue; // not in game

				playeringame[j] = true;
				players[j].availabilities = (UINT32)LONG(netbuffer->u.servercfg.playeravailabilities[j]);
				SetPlayerSkinByNum(j, (INT32)netbuffer->u.servercfg.playerskins[j]);
				players[j].skincolor = netbuffer->u.servercfg.playercolor[j];
			}

			scp = netbuffer->u.servercfg.varlengthinputs;
			CV_LoadPlayerNames(&scp);
			CV_LoadNetVars(&scp);
#ifndef NONET
			/// \note Wait. What if a Lua script uses some global custom variables synched with the NetVars hook?
			///       Shouldn't them be downloaded even at intermission time?
			///       Also, according to HandleConnect, the server will send the savegame even during intermission...
			if (netbuffer->u.servercfg.gamestate == GS_LEVEL/* ||
				netbuffer->u.servercfg.gamestate == GS_INTERMISSION*/)
				cl_mode = CL_DOWNLOADSAVEGAME;
			else
#endif
				cl_mode = CL_CONNECTED;
			break;
		}

		// Handled in d_netfil.c
		case PT_FILEFRAGMENT:
			if (server)
			{ // But wait I thought I'm the server?
				Net_CloseConnection(node);
				break;
			}
			SERVERONLY
			PT_FileFragment();
			break;

		case PT_FILEACK:
			if (server)
				PT_FileAck();
			break;

		case PT_FILERECEIVED:
			if (server)
				PT_FileReceived();
			break;

		case PT_REQUESTFILE:
			if (server)
			{
				if (!cv_downloading.value || !PT_RequestFile(node))
					Net_CloseConnection(node); // close connection if one of the requested files could not be sent, or you disabled downloading anyway
			}
			else
				Net_CloseConnection(node); // nope
			break;

		case PT_NODETIMEOUT:
		case PT_CLIENTQUIT:
			if (server)
				Net_CloseConnection(node);
			break;

		case PT_CLIENTCMD:
			break; // This is not an "unknown packet"

		case PT_SERVERTICS:
			// Do not remove my own server (we have just get a out of order packet)
			if (node == servernode)
				break;
			/* FALLTHRU */

		default:
			DEBFILE(va("unknown packet received (%d) from unknown host\n",netbuffer->packettype));
			Net_CloseConnection(node);
			break; // Ignore it

	}
#undef SERVERONLY
}

/** Handles a packet received from a node that is in game
  *
  * \param node The packet sender
  * \todo Choose a better name
  * \sa HandlePacketFromAwayNode
  * \sa GetPackets
  *
  */
static void HandlePacketFromPlayer(SINT8 node)
{
	INT32 netconsole;
	tic_t realend, realstart;
	UINT8 *pak, *txtpak, numtxtpak;
#ifndef NOMD5
	UINT8 finalmd5[16];/* Well, it's the cool thing to do? */
#endif

	txtpak = NULL;

	if (dedicated && node == 0)
		netconsole = 0;
	else
		netconsole = nodetoplayer[node];
#ifdef PARANOIA
	if (netconsole >= MAXPLAYERS)
		I_Error("bad table nodetoplayer: node %d player %d", doomcom->remotenode, netconsole);
#endif

	switch (netbuffer->packettype)
	{
// -------------------------------------------- SERVER RECEIVE ----------
		case PT_RESYNCHGET:
			if (client)
				break;
			SV_AcknowledgeResynchAck(node, netbuffer->u.resynchgot);
			break;
		case PT_CLIENTCMD:
		case PT_CLIENT2CMD:
		case PT_CLIENTMIS:
		case PT_CLIENT2MIS:
		case PT_NODEKEEPALIVE:
		case PT_NODEKEEPALIVEMIS:
			if (client)
				break;

			// Ignore tics from those not synched
			if (resynch_inprogress[node] && nettics[node] == gametic)
				break;

			// To save bytes, only the low byte of tic numbers are sent
			// Use ExpandTics to figure out what the rest of the bytes are
			realstart = ExpandTics(netbuffer->u.clientpak.client_tic, node);
			realend = ExpandTics(netbuffer->u.clientpak.resendfrom, node);

			if (netbuffer->packettype == PT_CLIENTMIS || netbuffer->packettype == PT_CLIENT2MIS
				|| netbuffer->packettype == PT_NODEKEEPALIVEMIS
				|| supposedtics[node] < realend)
			{
				supposedtics[node] = realend;
			}
			// Discard out of order packet
			if (nettics[node] > realend)
			{
				DEBFILE(va("out of order ticcmd discarded nettics = %u\n", nettics[node]));
				break;
			}

			// Update the nettics
			nettics[node] = realend;

			// Don't do anything for packets of type NODEKEEPALIVE?
			if (netconsole == -1 || netbuffer->packettype == PT_NODEKEEPALIVE
				|| netbuffer->packettype == PT_NODEKEEPALIVEMIS)
				break;

			// If a client sends a ticcmd it should mean they are done receiving the savegame
			sendingsavegame[node] = false;

			// As long as clients send valid ticcmds, the server can keep running, so reset the timeout
			/// \todo Use a separate cvar for that kind of timeout?
			freezetimeout[node] = I_GetTime() + connectiontimeout;

			// Copy ticcmd
			G_MoveTiccmd(&netcmds[maketic%BACKUPTICS][netconsole], &netbuffer->u.clientpak.cmd, 1);

			// Check ticcmd for "speed hacks"
			if (netcmds[maketic%BACKUPTICS][netconsole].forwardmove > MAXPLMOVE || netcmds[maketic%BACKUPTICS][netconsole].forwardmove < -MAXPLMOVE
				|| netcmds[maketic%BACKUPTICS][netconsole].sidemove > MAXPLMOVE || netcmds[maketic%BACKUPTICS][netconsole].sidemove < -MAXPLMOVE)
			{
				CONS_Alert(CONS_WARNING, M_GetText("Illegal movement value received from node %d\n"), netconsole);
				//D_Clearticcmd(k);

				SendKick(netconsole, KICK_MSG_CON_FAIL);
				break;
			}

			// Splitscreen cmd
			if ((netbuffer->packettype == PT_CLIENT2CMD || netbuffer->packettype == PT_CLIENT2MIS)
				&& nodetoplayer2[node] >= 0)
				G_MoveTiccmd(&netcmds[maketic%BACKUPTICS][(UINT8)nodetoplayer2[node]],
					&netbuffer->u.client2pak.cmd2, 1);

			// A delay before we check resynching
			// Used on join or just after a synch fail
			if (resynch_delay[node])
			{
				--resynch_delay[node];
				break;
			}
			// Check player consistancy during the level
			if (realstart <= gametic && realstart > gametic - BACKUPTICS+1 && gamestate == GS_LEVEL
				&& consistancy[realstart%BACKUPTICS] != SHORT(netbuffer->u.clientpak.consistancy))
			{
				SV_RequireResynch(node);

				if (cv_resynchattempts.value && resynch_score[node] <= (unsigned)cv_resynchattempts.value*250)
				{
					if (cv_blamecfail.value)
						CONS_Printf(M_GetText("Synch failure for player %d (%s); expected %hd, got %hd\n"),
							netconsole+1, player_names[netconsole],
							consistancy[realstart%BACKUPTICS],
							SHORT(netbuffer->u.clientpak.consistancy));
					DEBFILE(va("Restoring player %d (synch failure) [%update] %d!=%d\n",
						netconsole, realstart, consistancy[realstart%BACKUPTICS],
						SHORT(netbuffer->u.clientpak.consistancy)));
					break;
				}
				else
				{
					SendKick(netconsole, KICK_MSG_CON_FAIL | KICK_MSG_KEEP_BODY);
					DEBFILE(va("player %d kicked (synch failure) [%u] %d!=%d\n",
						netconsole, realstart, consistancy[realstart%BACKUPTICS],
						SHORT(netbuffer->u.clientpak.consistancy)));
					break;
				}
			}
			else if (resynch_score[node])
				--resynch_score[node];
			break;
		case PT_TEXTCMD2: // splitscreen special
			netconsole = nodetoplayer2[node];
			/* FALLTHRU */
		case PT_TEXTCMD:
			if (client)
				break;

			if (netconsole < 0 || netconsole >= MAXPLAYERS)
				Net_UnAcknowledgePacket(node);
			else
			{
				size_t j;
				tic_t tic = maketic;
				UINT8 *textcmd;

				// ignore if the textcmd has a reported size of zero
				// this shouldn't be sent at all
				if (!netbuffer->u.textcmd[0])
				{
					DEBFILE(va("GetPacket: Textcmd with size 0 detected! (node %u, player %d)\n",
						node, netconsole));
					Net_UnAcknowledgePacket(node);
					break;
				}

				// ignore if the textcmd size var is actually larger than it should be
				// BASEPACKETSIZE + 1 (for size) + textcmd[0] should == datalength
				if (netbuffer->u.textcmd[0] > (size_t)doomcom->datalength-BASEPACKETSIZE-1)
				{
					DEBFILE(va("GetPacket: Bad Textcmd packet size! (expected %d, actual %s, node %u, player %d)\n",
					netbuffer->u.textcmd[0], sizeu1((size_t)doomcom->datalength-BASEPACKETSIZE-1),
						node, netconsole));
					Net_UnAcknowledgePacket(node);
					break;
				}

				// check if tic that we are making isn't too large else we cannot send it :(
				// doomcom->numslots+1 "+1" since doomcom->numslots can change within this time and sent time
				j = software_MAXPACKETLENGTH
					- (netbuffer->u.textcmd[0]+2+BASESERVERTICSSIZE
					+ (doomcom->numslots+1)*sizeof(ticcmd_t));

				// search a tic that have enougth space in the ticcmd
				while ((textcmd = D_GetExistingTextcmd(tic, netconsole)),
					(TotalTextCmdPerTic(tic) > j || netbuffer->u.textcmd[0] + (textcmd ? textcmd[0] : 0) > MAXTEXTCMD)
					&& tic < firstticstosend + BACKUPTICS)
					tic++;

				if (tic >= firstticstosend + BACKUPTICS)
				{
					DEBFILE(va("GetPacket: Textcmd too long (max %s, used %s, mak %d, "
						"tosend %u, node %u, player %d)\n", sizeu1(j), sizeu2(TotalTextCmdPerTic(maketic)),
						maketic, firstticstosend, node, netconsole));
					Net_UnAcknowledgePacket(node);
					break;
				}

				// Make sure we have a buffer
				if (!textcmd) textcmd = D_GetTextcmd(tic, netconsole);

				DEBFILE(va("textcmd put in tic %u at position %d (player %d) ftts %u mk %u\n",
					tic, textcmd[0]+1, netconsole, firstticstosend, maketic));

				M_Memcpy(&textcmd[textcmd[0]+1], netbuffer->u.textcmd+1, netbuffer->u.textcmd[0]);
				textcmd[0] += (UINT8)netbuffer->u.textcmd[0];
			}
			break;
		case PT_LOGIN:
			if (client)
				break;

#ifndef NOMD5
			if (doomcom->datalength < 16)/* ignore partial sends */
				break;

			if (!adminpasswordset)
			{
				CONS_Printf(M_GetText("Password from %s failed (no password set).\n"), player_names[netconsole]);
				break;
			}

			// Do the final pass to compare with the sent md5
			D_MD5PasswordPass(adminpassmd5, 16, va("PNUM%02d", netconsole), &finalmd5);

			if (!memcmp(netbuffer->u.md5sum, finalmd5, 16))
			{
				CONS_Printf(M_GetText("%s passed authentication.\n"), player_names[netconsole]);
				COM_BufInsertText(va("promote %d\n", netconsole)); // do this immediately
			}
			else
				CONS_Printf(M_GetText("Password from %s failed.\n"), player_names[netconsole]);
#endif
			break;
		case PT_NODETIMEOUT:
		case PT_CLIENTQUIT:
			if (client)
				break;

			// nodeingame will be put false in the execution of kick command
			// this allow to send some packets to the quitting client to have their ack back
			nodewaiting[node] = 0;
			if (netconsole != -1 && playeringame[netconsole])
			{
				UINT8 kickmsg;

				if (netbuffer->packettype == PT_NODETIMEOUT)
					kickmsg = KICK_MSG_TIMEOUT;
				else
					kickmsg = KICK_MSG_PLAYER_QUIT;
				kickmsg |= KICK_MSG_KEEP_BODY;

				SendKick(netconsole, kickmsg);
				nodetoplayer[node] = -1;

				if (nodetoplayer2[node] != -1 && nodetoplayer2[node] >= 0
					&& playeringame[(UINT8)nodetoplayer2[node]])
				{
					SendKick(nodetoplayer2[node], kickmsg);
					nodetoplayer2[node] = -1;
				}
			}
			Net_CloseConnection(node);
			nodeingame[node] = false;
			break;
		case PT_ASKLUAFILE:
			if (server && luafiletransfers && luafiletransfers->nodestatus[node] == LFTNS_ASKED)
				AddLuaFileToSendQueue(node, luafiletransfers->realfilename);
			break;
		case PT_HASLUAFILE:
			if (server && luafiletransfers && luafiletransfers->nodestatus[node] == LFTNS_SENDING)
				SV_HandleLuaFileSent(node);
			break;
// -------------------------------------------- CLIENT RECEIVE ----------
		case PT_RESYNCHEND:
			// Only accept PT_RESYNCHEND from the server.
			if (node != servernode)
			{
				CONS_Alert(CONS_WARNING, M_GetText("%s received from non-host %d\n"), "PT_RESYNCHEND", node);
				if (server)
					SendKick(netconsole, KICK_MSG_CON_FAIL | KICK_MSG_KEEP_BODY);
				break;
			}
			resynch_local_inprogress = false;

			P_SetRandSeed(netbuffer->u.resynchend.randomseed);

			if (gametyperules & GTR_TEAMFLAGS)
				resynch_read_ctf(&netbuffer->u.resynchend);
			resynch_read_others(&netbuffer->u.resynchend);

			break;
		case PT_SERVERTICS:
			// Only accept PT_SERVERTICS from the server.
			if (node != servernode)
			{
				CONS_Alert(CONS_WARNING, M_GetText("%s received from non-host %d\n"), "PT_SERVERTICS", node);
				if (server)
					SendKick(netconsole, KICK_MSG_CON_FAIL | KICK_MSG_KEEP_BODY);
				break;
			}

			realstart = netbuffer->u.serverpak.starttic;
			realend = realstart + netbuffer->u.serverpak.numtics;

			if (!txtpak)
				txtpak = (UINT8 *)&netbuffer->u.serverpak.cmds[netbuffer->u.serverpak.numslots
					* netbuffer->u.serverpak.numtics];

			if (realend > gametic + CLIENTBACKUPTICS)
				realend = gametic + CLIENTBACKUPTICS;
			cl_packetmissed = realstart > neededtic;

			if (realstart <= neededtic && realend > neededtic)
			{
				tic_t i, j;
				pak = (UINT8 *)&netbuffer->u.serverpak.cmds;

				for (i = realstart; i < realend; i++)
				{
					// clear first
					D_Clearticcmd(i);

					// copy the tics
					pak = G_ScpyTiccmd(netcmds[i%BACKUPTICS], pak,
						netbuffer->u.serverpak.numslots*sizeof (ticcmd_t));

					// copy the textcmds
					numtxtpak = *txtpak++;
					for (j = 0; j < numtxtpak; j++)
					{
						INT32 k = *txtpak++; // playernum
						const size_t txtsize = txtpak[0]+1;

						if (i >= gametic) // Don't copy old net commands
							M_Memcpy(D_GetTextcmd(i, k), txtpak, txtsize);
						txtpak += txtsize;
					}
				}

				neededtic = realend;
			}
			else
			{
				DEBFILE(va("frame not in bound: %u\n", neededtic));
				/*if (realend < neededtic - 2 * TICRATE || neededtic + 2 * TICRATE < realstart)
					I_Error("Received an out of order PT_SERVERTICS packet!\n"
							"Got tics %d-%d, needed tic %d\n\n"
							"Please report this crash on the Master Board,\n"
							"IRC or Discord so it can be fixed.\n", (INT32)realstart, (INT32)realend, (INT32)neededtic);*/
			}
			break;
		case PT_RESYNCHING:
			// Only accept PT_RESYNCHING from the server.
			if (node != servernode)
			{
				CONS_Alert(CONS_WARNING, M_GetText("%s received from non-host %d\n"), "PT_RESYNCHING", node);
				if (server)
					SendKick(netconsole, KICK_MSG_CON_FAIL | KICK_MSG_KEEP_BODY);
				break;
			}
			resynch_local_inprogress = true;
			CL_AcknowledgeResynch(&netbuffer->u.resynchpak);
			break;
		case PT_PING:
			// Only accept PT_PING from the server.
			if (node != servernode)
			{
				CONS_Alert(CONS_WARNING, M_GetText("%s received from non-host %d\n"), "PT_PING", node);
				if (server)
					SendKick(netconsole, KICK_MSG_CON_FAIL | KICK_MSG_KEEP_BODY);
				break;
			}

			//Update client ping table from the server.
			if (client)
			{
				UINT8 i;
				for (i = 0; i < MAXPLAYERS; i++)
					if (playeringame[i])
						playerpingtable[i] = (tic_t)netbuffer->u.pingtable[i];

				servermaxping = (tic_t)netbuffer->u.pingtable[MAXPLAYERS];
			}

			break;
		case PT_SERVERCFG:
			break;
		case PT_FILEFRAGMENT:
			// Only accept PT_FILEFRAGMENT from the server.
			if (node != servernode)
			{
				CONS_Alert(CONS_WARNING, M_GetText("%s received from non-host %d\n"), "PT_FILEFRAGMENT", node);
				if (server)
					SendKick(netconsole, KICK_MSG_CON_FAIL | KICK_MSG_KEEP_BODY);
				break;
			}
			if (client)
				PT_FileFragment();
			break;
		case PT_FILEACK:
			if (server)
				PT_FileAck();
			break;
		case PT_FILERECEIVED:
			if (server)
				PT_FileReceived();
			break;
		case PT_SENDINGLUAFILE:
			if (client)
				CL_PrepareDownloadLuaFile();
			break;
		default:
			DEBFILE(va("UNKNOWN PACKET TYPE RECEIVED %d from host %d\n",
				netbuffer->packettype, node));
	} // end switch
}

/**	Handles all received packets, if any
  *
  * \todo Add details to this description (lol)
  *
  */
static void GetPackets(void)
{
	SINT8 node; // The packet sender

	player_joining = false;

	while (HGetPacket())
	{
		node = (SINT8)doomcom->remotenode;

		if (netbuffer->packettype == PT_CLIENTJOIN && server)
		{
			HandleConnect(node);
			continue;
		}
		if (node == servernode && client && cl_mode != CL_SEARCHING)
		{
			if (netbuffer->packettype == PT_SERVERSHUTDOWN)
			{
				HandleShutdown(node);
				continue;
			}
			if (netbuffer->packettype == PT_NODETIMEOUT)
			{
				HandleTimeout(node);
				continue;
			}
		}

#ifndef NONET
		if (netbuffer->packettype == PT_SERVERINFO)
		{
			HandleServerInfo(node);
			continue;
		}
#endif

		if (netbuffer->packettype == PT_PLAYERINFO)
			continue; // We do nothing with PLAYERINFO, that's for the MS browser.

		// Packet received from someone already playing
		if (nodeingame[node])
			HandlePacketFromPlayer(node);
		// Packet received from someone not playing
		else
			HandlePacketFromAwayNode(node);
	}
}

//
// NetUpdate
// Builds ticcmds for console player,
// sends out a packet
//
// no more use random generator, because at very first tic isn't yet synchronized
// Note: It is called consistAncy on purpose.
//
static INT16 Consistancy(void)
{
	INT32 i;
	UINT32 ret = 0;
#ifdef MOBJCONSISTANCY
	thinker_t *th;
	mobj_t *mo;
#endif

	DEBFILE(va("TIC %u ", gametic));

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i])
			ret ^= 0xCCCC;
		else if (!players[i].mo);
		else
		{
			ret += players[i].mo->x;
			ret -= players[i].mo->y;
			ret += players[i].powers[pw_shield];
			ret *= i+1;
		}
	}
	// I give up
	// Coop desynching enemies is painful
	if (!G_PlatformGametype())
		ret += P_GetRandSeed();

#ifdef MOBJCONSISTANCY
	for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
	{
		if (th->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
			continue;

		mo = (mobj_t *)th;

		if (mo->flags & (MF_SPECIAL | MF_SOLID | MF_PUSHABLE | MF_BOSS | MF_MISSILE | MF_SPRING | MF_MONITOR | MF_FIRE | MF_ENEMY | MF_PAIN | MF_STICKY))
		{
			ret -= mo->type;
			ret += mo->x;
			ret -= mo->y;
			ret += mo->z;
			ret -= mo->momx;
			ret += mo->momy;
			ret -= mo->momz;
			ret += mo->angle;
			ret -= mo->flags;
			ret += mo->flags2;
			ret -= mo->eflags;
			if (mo->target)
			{
				ret += mo->target->type;
				ret -= mo->target->x;
				ret += mo->target->y;
				ret -= mo->target->z;
				ret += mo->target->momx;
				ret -= mo->target->momy;
				ret += mo->target->momz;
				ret -= mo->target->angle;
				ret += mo->target->flags;
				ret -= mo->target->flags2;
				ret += mo->target->eflags;
				ret -= mo->target->state - states;
				ret += mo->target->tics;
				ret -= mo->target->sprite;
				ret += mo->target->frame;
			}
			else
				ret ^= 0x3333;
			if (mo->tracer && mo->tracer->type != MT_OVERLAY)
			{
				ret += mo->tracer->type;
				ret -= mo->tracer->x;
				ret += mo->tracer->y;
				ret -= mo->tracer->z;
				ret += mo->tracer->momx;
				ret -= mo->tracer->momy;
				ret += mo->tracer->momz;
				ret -= mo->tracer->angle;
				ret += mo->tracer->flags;
				ret -= mo->tracer->flags2;
				ret += mo->tracer->eflags;
				ret -= mo->tracer->state - states;
				ret += mo->tracer->tics;
				ret -= mo->tracer->sprite;
				ret += mo->tracer->frame;
			}
			else
				ret ^= 0xAAAA;
			ret -= mo->state - states;
			ret += mo->tics;
			ret -= mo->sprite;
			ret += mo->frame;
		}
	}
#endif

	DEBFILE(va("Consistancy = %u\n", (ret & 0xFFFF)));

	return (INT16)(ret & 0xFFFF);
}

// send the client packet to the server
static void CL_SendClientCmd(void)
{
	size_t packetsize = 0;

	netbuffer->packettype = PT_CLIENTCMD;

	if (cl_packetmissed)
		netbuffer->packettype++;
	netbuffer->u.clientpak.resendfrom = (UINT8)(neededtic & UINT8_MAX);
	netbuffer->u.clientpak.client_tic = (UINT8)(gametic & UINT8_MAX);

	if (gamestate == GS_WAITINGPLAYERS)
	{
		// Send PT_NODEKEEPALIVE packet
		netbuffer->packettype += 4;
		packetsize = sizeof (clientcmd_pak) - sizeof (ticcmd_t) - sizeof (INT16);
		HSendPacket(servernode, false, 0, packetsize);
	}
	else if (gamestate != GS_NULL && (addedtogame || dedicated))
	{
		G_MoveTiccmd(&netbuffer->u.clientpak.cmd, &localcmds, 1);
		netbuffer->u.clientpak.consistancy = SHORT(consistancy[gametic%BACKUPTICS]);

		// Send a special packet with 2 cmd for splitscreen
		if (splitscreen || botingame)
		{
			netbuffer->packettype += 2;
			G_MoveTiccmd(&netbuffer->u.client2pak.cmd2, &localcmds2, 1);
			packetsize = sizeof (client2cmd_pak);
		}
		else
			packetsize = sizeof (clientcmd_pak);

		HSendPacket(servernode, false, 0, packetsize);
	}

	if (cl_mode == CL_CONNECTED || dedicated)
	{
		// Send extra data if needed
		if (localtextcmd[0])
		{
			netbuffer->packettype = PT_TEXTCMD;
			M_Memcpy(netbuffer->u.textcmd,localtextcmd, localtextcmd[0]+1);
			// All extra data have been sent
			if (HSendPacket(servernode, true, 0, localtextcmd[0]+1)) // Send can fail...
				localtextcmd[0] = 0;
		}

		// Send extra data if needed for player 2 (splitscreen)
		if (localtextcmd2[0])
		{
			netbuffer->packettype = PT_TEXTCMD2;
			M_Memcpy(netbuffer->u.textcmd, localtextcmd2, localtextcmd2[0]+1);
			// All extra data have been sent
			if (HSendPacket(servernode, true, 0, localtextcmd2[0]+1)) // Send can fail...
				localtextcmd2[0] = 0;
		}
	}
}

// send the server packet
// send tic from firstticstosend to maketic-1
static void SV_SendTics(void)
{
	tic_t realfirsttic, lasttictosend, i;
	UINT32 n;
	INT32 j;
	size_t packsize;
	UINT8 *bufpos;
	UINT8 *ntextcmd;

	// send to all client but not to me
	// for each node create a packet with x tics and send it
	// x is computed using supposedtics[n], max packet size and maketic
	for (n = 1; n < MAXNETNODES; n++)
		if (nodeingame[n])
		{
			// assert supposedtics[n]>=nettics[n]
			realfirsttic = supposedtics[n];
			lasttictosend = min(maketic, realfirsttic + CLIENTBACKUPTICS);

			if (realfirsttic >= lasttictosend)
			{
				// well we have sent all tics we will so use extrabandwidth
				// to resent packet that are supposed lost (this is necessary since lost
				// packet detection work when we have received packet with firsttic > neededtic
				// (getpacket servertics case)
				DEBFILE(va("Nothing to send node %u mak=%u sup=%u net=%u \n",
					n, maketic, supposedtics[n], nettics[n]));
				realfirsttic = nettics[n];
				if (realfirsttic >= lasttictosend || (I_GetTime() + n)&3)
					// all tic are ok
					continue;
				DEBFILE(va("Sent %d anyway\n", realfirsttic));
			}
			if (realfirsttic < firstticstosend)
				realfirsttic = firstticstosend;

			// compute the length of the packet and cut it if too large
			packsize = BASESERVERTICSSIZE;
			for (i = realfirsttic; i < lasttictosend; i++)
			{
				packsize += sizeof (ticcmd_t) * doomcom->numslots;
				packsize += TotalTextCmdPerTic(i);

				if (packsize > software_MAXPACKETLENGTH)
				{
					DEBFILE(va("packet too large (%s) at tic %d (should be from %d to %d)\n",
						sizeu1(packsize), i, realfirsttic, lasttictosend));
					lasttictosend = i;

					// too bad: too much player have send extradata and there is too
					//          much data in one tic.
					// To avoid it put the data on the next tic. (see getpacket
					// textcmd case) but when numplayer changes the computation can be different
					if (lasttictosend == realfirsttic)
					{
						if (packsize > MAXPACKETLENGTH)
							I_Error("Too many players: can't send %s data for %d players to node %d\n"
							        "Well sorry nobody is perfect....\n",
							        sizeu1(packsize), doomcom->numslots, n);
						else
						{
							lasttictosend++; // send it anyway!
							DEBFILE("sending it anyway\n");
						}
					}
					break;
				}
			}

			// Send the tics
			netbuffer->packettype = PT_SERVERTICS;
			netbuffer->u.serverpak.starttic = realfirsttic;
			netbuffer->u.serverpak.numtics = (UINT8)(lasttictosend - realfirsttic);
			netbuffer->u.serverpak.numslots = (UINT8)SHORT(doomcom->numslots);
			bufpos = (UINT8 *)&netbuffer->u.serverpak.cmds;

			for (i = realfirsttic; i < lasttictosend; i++)
			{
				bufpos = G_DcpyTiccmd(bufpos, netcmds[i%BACKUPTICS], doomcom->numslots * sizeof (ticcmd_t));
			}

			// add textcmds
			for (i = realfirsttic; i < lasttictosend; i++)
			{
				ntextcmd = bufpos++;
				*ntextcmd = 0;
				for (j = 0; j < MAXPLAYERS; j++)
				{
					UINT8 *textcmd = D_GetExistingTextcmd(i, j);
					INT32 size = textcmd ? textcmd[0] : 0;

					if ((!j || playeringame[j]) && size)
					{
						(*ntextcmd)++;
						WRITEUINT8(bufpos, j);
						M_Memcpy(bufpos, textcmd, size + 1);
						bufpos += size + 1;
					}
				}
			}
			packsize = bufpos - (UINT8 *)&(netbuffer->u);

			HSendPacket(n, false, 0, packsize);
			// when tic are too large, only one tic is sent so don't go backward!
			if (lasttictosend-doomcom->extratics > realfirsttic)
				supposedtics[n] = lasttictosend-doomcom->extratics;
			else
				supposedtics[n] = lasttictosend;
			if (supposedtics[n] < nettics[n]) supposedtics[n] = nettics[n];
		}
	// node 0 is me!
	supposedtics[0] = maketic;
}

//
// TryRunTics
//
static void Local_Maketic(INT32 realtics)
{
	I_OsPolling(); // I_Getevent
	D_ProcessEvents(); // menu responder, cons responder,
	                   // game responder calls HU_Responder, AM_Responder,
	                   // and G_MapEventsToControls
	if (!dedicated) rendergametic = gametic;
	// translate inputs (keyboard/mouse/joystick) into game controls
	G_BuildTiccmd(&localcmds, realtics, 1);
	if (splitscreen || botingame)
		G_BuildTiccmd(&localcmds2, realtics, 2);

	localcmds.angleturn |= TICCMD_RECEIVED;
	localcmds2.angleturn |= TICCMD_RECEIVED;
}

// create missed tic
static void SV_Maketic(void)
{
	INT32 i;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i])
			continue;

		// We didn't receive this tic
		if ((netcmds[maketic % BACKUPTICS][i].angleturn & TICCMD_RECEIVED) == 0)
		{
			ticcmd_t *    ticcmd = &netcmds[(maketic    ) % BACKUPTICS][i];
			ticcmd_t *prevticcmd = &netcmds[(maketic - 1) % BACKUPTICS][i];

			if (players[i].quittime)
			{
				// Copy the angle/aiming from the previous tic
				// and empty the other inputs
				memset(ticcmd, 0, sizeof(netcmds[0][0]));
				ticcmd->angleturn = prevticcmd->angleturn | TICCMD_RECEIVED;
				ticcmd->aiming = prevticcmd->aiming;
			}
			else
			{
				DEBFILE(va("MISS tic%4d for player %d\n", maketic, i));
				// Copy the input from the previous tic
				*ticcmd = *prevticcmd;
				ticcmd->angleturn &= ~TICCMD_RECEIVED;
			}
		}
	}

	// all tic are now proceed make the next
	maketic++;
}

void TryRunTics(tic_t realtics)
{
	// the machine has lagged but it is not so bad
	if (realtics > TICRATE/7) // FIXME: consistency failure!!
	{
		if (server)
			realtics = 1;
		else
			realtics = TICRATE/7;
	}

	if (singletics)
		realtics = 1;

	if (realtics >= 1)
	{
		COM_BufTicker();
		if (mapchangepending)
			D_MapChange(-1, 0, ultimatemode, false, 2, false, fromlevelselect); // finish the map change
	}

	NetUpdate();

	if (demoplayback)
	{
		neededtic = gametic + (realtics * cv_playbackspeed.value);
		// start a game after a demo
		maketic += realtics;
		firstticstosend = maketic;
		tictoclear = firstticstosend;
	}

	GetPackets();

#ifdef DEBUGFILE
	if (debugfile && (realtics || neededtic > gametic))
	{
		//SoM: 3/30/2000: Need long INT32 in the format string for args 4 & 5.
		//Shut up stupid warning!
		fprintf(debugfile, "------------ Tryruntic: REAL:%d NEED:%d GAME:%d LOAD: %d\n",
			realtics, neededtic, gametic, debugload);
		debugload = 100000;
	}
#endif

	if (player_joining)
		return;

	if (neededtic > gametic && !resynch_local_inprogress)
	{
		if (advancedemo)
		{
			if (timedemo_quit)
				COM_ImmedExecute("quit");
			else
				D_StartTitle();
		}
		else
			// run the count * tics
			while (neededtic > gametic)
			{
				DEBFILE(va("============ Running tic %d (local %d)\n", gametic, localgametic));

				G_Ticker((gametic % NEWTICRATERATIO) == 0);
				ExtraDataTicker();
				gametic++;
				consistancy[gametic%BACKUPTICS] = Consistancy();
			}
	}
}

/*
Ping Update except better:
We call this once per second and check for people's pings. If their ping happens to be too high, we increment some timer and kick them out.
If they're not lagging, decrement the timer by 1. Of course, reset all of this if they leave.
*/

static INT32 pingtimeout[MAXPLAYERS];

static inline void PingUpdate(void)
{
	INT32 i;
	boolean laggers[MAXPLAYERS];
	UINT8 numlaggers = 0;
	memset(laggers, 0, sizeof(boolean) * MAXPLAYERS);

	netbuffer->packettype = PT_PING;

	//check for ping limit breakage.
	if (cv_maxping.value)
	{
		for (i = 1; i < MAXPLAYERS; i++)
		{
			if (playeringame[i] && !players[i].quittime
			&& (realpingtable[i] / pingmeasurecount > (unsigned)cv_maxping.value))
			{
				if (players[i].jointime > 30 * TICRATE)
					laggers[i] = true;
				numlaggers++;
			}
			else
				pingtimeout[i] = 0;
		}

		//kick lagging players... unless everyone but the server's ping sucks.
		//in that case, it is probably the server's fault.
		if (numlaggers < D_NumPlayers() - 1)
		{
			for (i = 1; i < MAXPLAYERS; i++)
			{
				if (playeringame[i] && laggers[i])
				{
					pingtimeout[i]++;
					// ok your net has been bad for too long, you deserve to die.
					if (pingtimeout[i] > cv_pingtimeout.value)
					{
						pingtimeout[i] = 0;
						SendKick(i, KICK_MSG_PING_HIGH | KICK_MSG_KEEP_BODY);
					}
				}
				/*
					you aren't lagging,
					but you aren't free yet.
					In case you'll keep spiking,
					we just make the timer go back down. (Very unstable net must still get kicked).
				*/
				else
					pingtimeout[i] = (pingtimeout[i] == 0 ? 0 : pingtimeout[i]-1);
			}
		}
	}

	//make the ping packet and clear server data for next one
	for (i = 0; i < MAXPLAYERS; i++)
	{
		netbuffer->u.pingtable[i] = realpingtable[i] / pingmeasurecount;
		//server takes a snapshot of the real ping for display.
		//otherwise, pings fluctuate a lot and would be odd to look at.
		playerpingtable[i] = realpingtable[i] / pingmeasurecount;
		realpingtable[i] = 0; //Reset each as we go.
	}

	// send the server's maxping as last element of our ping table. This is useful to let us know when we're about to get kicked.
	netbuffer->u.pingtable[MAXPLAYERS] = cv_maxping.value;

	//send out our ping packets
	for (i = 0; i < MAXNETNODES; i++)
		if (nodeingame[i])
			HSendPacket(i, true, 0, sizeof(INT32) * (MAXPLAYERS+1));

	pingmeasurecount = 1; //Reset count
}

void NetUpdate(void)
{
	static tic_t gametime = 0;
	static tic_t resptime = 0;
	tic_t nowtime;
	INT32 i;
	INT32 realtics;

	nowtime = I_GetTime();
	realtics = nowtime - gametime;

	if (realtics <= 0) // nothing new to update
		return;
	if (realtics > 5)
	{
		if (server)
			realtics = 1;
		else
			realtics = 5;
	}

	gametime = nowtime;

	if (server)
	{
		if (netgame && !(gametime % 35)) // update once per second.
			PingUpdate();
		// update node latency values so we can take an average later.
		for (i = 0; i < MAXPLAYERS; i++)
			if (playeringame[i] && playernode[i] != UINT8_MAX)
				realpingtable[i] += GetLag(playernode[i]) * (1000.00f / TICRATE);
		pingmeasurecount++;
	}

	if (client)
		maketic = neededtic;

	Local_Maketic(realtics); // make local tic, and call menu?

	if (server)
		CL_SendClientCmd(); // send it

	GetPackets(); // get packet from client or from server

	// client send the command after a receive of the server
	// the server send before because in single player is beter

	MasterClient_Ticker(); // Acking the Master Server

	if (client)
	{
		if (!resynch_local_inprogress)
			CL_SendClientCmd(); // Send tic cmd
		hu_resynching = resynch_local_inprogress;
	}
	else
	{
		if (!demoplayback)
		{
			INT32 counts;

			hu_resynching = false;

			firstticstosend = gametic;
			for (i = 0; i < MAXNETNODES; i++)
				if (nodeingame[i] && nettics[i] < firstticstosend)
					firstticstosend = nettics[i];

			// Don't erase tics not acknowledged
			counts = realtics;

			for (i = 0; i < MAXNETNODES; ++i)
				if (resynch_inprogress[i])
				{
					if (!nodeingame[i] || nettics[i] == gametic)
					{
						SV_SendResynch(i);
						counts = -666;
					}
					else
						counts = 0; // Let the client catch up with the server
				}

			// Do not make tics while resynching
			if (counts != -666)
			{
				if (maketic + counts >= firstticstosend + BACKUPTICS)
					counts = firstticstosend+BACKUPTICS-maketic-1;

				for (i = 0; i < counts; i++)
					SV_Maketic(); // Create missed tics and increment maketic

				for (; tictoclear < firstticstosend; tictoclear++) // Clear only when acknowledged
					D_Clearticcmd(tictoclear);                    // Clear the maketic the new tic

				SV_SendTics();

				neededtic = maketic; // The server is a client too
			}
			else
				hu_resynching = true;
		}
	}

	Net_AckTicker();

	// Handle timeouts to prevent definitive freezes from happenning
	if (server)
	{
		for (i = 1; i < MAXNETNODES; i++)
			if (nodeingame[i] && freezetimeout[i] < I_GetTime())
				Net_ConnectionTimeout(i);

		// In case the cvar value was lowered
		if (joindelay)
			joindelay = min(joindelay - 1, 3 * (tic_t)cv_joindelay.value * TICRATE);
	}

	nowtime /= NEWTICRATERATIO;
	if (nowtime > resptime)
	{
		resptime = nowtime;
		M_Ticker();
		CON_Ticker();
	}

	FileSendTicker();
}

/** Returns the number of players playing.
  * \return Number of players. Can be zero if we're running a ::dedicated
  *         server.
  * \author Graue <graue@oceanbase.org>
  */
INT32 D_NumPlayers(void)
{
	INT32 num = 0, ix;
	for (ix = 0; ix < MAXPLAYERS; ix++)
		if (playeringame[ix])
			num++;
	return num;
}

tic_t GetLag(INT32 node)
{
	return gametic - nettics[node];
}

void D_MD5PasswordPass(const UINT8 *buffer, size_t len, const char *salt, void *dest)
{
#ifdef NOMD5
	(void)buffer;
	(void)len;
	(void)salt;
	memset(dest, 0, 16);
#else
	char tmpbuf[256];
	const size_t sl = strlen(salt);

	if (len > 256-sl)
		len = 256-sl;

	memcpy(tmpbuf, buffer, len);
	memmove(&tmpbuf[len], salt, sl);
	//strcpy(&tmpbuf[len], salt);
	len += strlen(salt);
	if (len < 256)
		memset(&tmpbuf[len],0,256-len);

	// Yes, we intentionally md5 the ENTIRE buffer regardless of size...
	md5_buffer(tmpbuf, 256, dest);
#endif
}

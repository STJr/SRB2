// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2016 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  hu_stuff.c
/// \brief Heads up display

#include "doomdef.h"
#include "byteptr.h"
#include "hu_stuff.h"

#include "m_menu.h" // gametype_cons_t
#include "m_cond.h" // emblems

#include "d_clisrv.h"

#include "g_game.h"
#include "g_input.h"

#include "i_video.h"
#include "i_system.h"

#include "st_stuff.h" // ST_HEIGHT
#include "r_local.h"

#include "keys.h"
#include "v_video.h"

#include "w_wad.h"
#include "z_zone.h"

#include "console.h"
#include "am_map.h"
#include "d_main.h"

#include "p_local.h" // camera, camera2
#include "p_tick.h"

#ifdef HWRENDER
#include "hardware/hw_main.h"
#endif

#ifdef HAVE_BLUA
#include "lua_hud.h"
#include "lua_hook.h"
#endif

// coords are scaled
#define HU_INPUTX 0
#define HU_INPUTY 0

#define HU_SERVER_SAY 1 // Server message (dedicated).
#define HU_CSAY       2 // Server CECHOes to everyone.

//-------------------------------------------
//              heads up font
//-------------------------------------------
patch_t *hu_font[HU_FONTSIZE];
patch_t *tny_font[HU_FONTSIZE];
patch_t *tallnum[10]; // 0-9
patch_t *nightsnum[10]; // 0-9

// Level title and credits fonts
patch_t *lt_font[LT_FONTSIZE];
patch_t *cred_font[CRED_FONTSIZE];

static player_t *plr;
boolean chat_on; // entering a chat message?
static char w_chat[HU_MAXMSGLEN];
static INT32 c_input = 0;	// let's try to make the chat input less shitty.
static boolean headsupactive = false;
boolean hu_showscores; // draw rankings
static char hu_tick;

patch_t *rflagico;
patch_t *bflagico;
patch_t *rmatcico;
patch_t *bmatcico;
patch_t *tagico;
patch_t *tallminus;

//-------------------------------------------
//              coop hud
//-------------------------------------------

patch_t *emeraldpics[7];
patch_t *tinyemeraldpics[7];
static patch_t *emblemicon;
static patch_t *tokenicon;

//-------------------------------------------
//              misc vars
//-------------------------------------------

// crosshair 0 = off, 1 = cross, 2 = angle, 3 = point, see m_menu.c
static patch_t *crosshair[HU_CROSSHAIRS]; // 3 precached crosshair graphics

// -------
// protos.
// -------
static void HU_DrawRankings(void);
static void HU_DrawCoopOverlay(void);
static void HU_DrawNetplayCoopOverlay(void);

//======================================================================
//                 KEYBOARD LAYOUTS FOR ENTERING TEXT
//======================================================================

char *shiftxform;

char english_shiftxform[] =
{
	0,
	1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
	11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
	21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
	31,
	' ', '!', '"', '#', '$', '%', '&',
	'"', // shift-'
	'(', ')', '*', '+',
	'<', // shift-,
	'_', // shift--
	'>', // shift-.
	'?', // shift-/
	')', // shift-0
	'!', // shift-1
	'@', // shift-2
	'#', // shift-3
	'$', // shift-4
	'%', // shift-5
	'^', // shift-6
	'&', // shift-7
	'*', // shift-8
	'(', // shift-9
	':',
	':', // shift-;
	'<',
	'+', // shift-=
	'>', '?', '@',
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
	'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
	'{', // shift-[
	'|', // shift-backslash - OH MY GOD DOES WATCOM SUCK
	'}', // shift-]
	'"', '_',
	'~', // shift-`
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
	'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
	'{', '|', '}', '~', 127
};

static char cechotext[1024];
static tic_t cechotimer = 0;
static tic_t cechoduration = 5*TICRATE;
static INT32 cechoflags = 0;

//======================================================================
//                          HEADS UP INIT
//======================================================================

#ifndef NONET
// just after
static void Command_Say_f(void);
static void Command_Sayto_f(void);
static void Command_Sayteam_f(void);
static void Command_CSay_f(void);
static void Got_Saycmd(UINT8 **p, INT32 playernum);
#endif

void HU_LoadGraphics(void)
{
	char buffer[9];
	INT32 i, j;

	if (dedicated)
		return;

	j = HU_FONTSTART;
	for (i = 0; i < HU_FONTSIZE; i++, j++)
	{
		// cache the heads-up font for entire game execution
		sprintf(buffer, "STCFN%.3d", j);
		if (W_CheckNumForName(buffer) == LUMPERROR)
			hu_font[i] = NULL;
		else
			hu_font[i] = (patch_t *)W_CachePatchName(buffer, PU_HUDGFX);

		// tiny version of the heads-up font
		sprintf(buffer, "TNYFN%.3d", j);
		if (W_CheckNumForName(buffer) == LUMPERROR)
			tny_font[i] = NULL;
		else
			tny_font[i] = (patch_t *)W_CachePatchName(buffer, PU_HUDGFX);
	}

	// cache the level title font for entire game execution
	lt_font[0] = (patch_t *)W_CachePatchName("LTFNT039", PU_HUDGFX); /// \note fake start hack

	// Number support
	lt_font[9] = (patch_t *)W_CachePatchName("LTFNT048", PU_HUDGFX);
	lt_font[10] = (patch_t *)W_CachePatchName("LTFNT049", PU_HUDGFX);
	lt_font[11] = (patch_t *)W_CachePatchName("LTFNT050", PU_HUDGFX);
	lt_font[12] = (patch_t *)W_CachePatchName("LTFNT051", PU_HUDGFX);
	lt_font[13] = (patch_t *)W_CachePatchName("LTFNT052", PU_HUDGFX);
	lt_font[14] = (patch_t *)W_CachePatchName("LTFNT053", PU_HUDGFX);
	lt_font[15] = (patch_t *)W_CachePatchName("LTFNT054", PU_HUDGFX);
	lt_font[16] = (patch_t *)W_CachePatchName("LTFNT055", PU_HUDGFX);
	lt_font[17] = (patch_t *)W_CachePatchName("LTFNT056", PU_HUDGFX);
	lt_font[18] = (patch_t *)W_CachePatchName("LTFNT057", PU_HUDGFX);

	j = LT_FONTSTART;
	for (i = 0; i < LT_FONTSIZE; i++)
	{
		sprintf(buffer, "LTFNT%.3d", j);
		j++;

		if (W_CheckNumForName(buffer) == LUMPERROR)
			lt_font[i] = NULL;
		else
			lt_font[i] = (patch_t *)W_CachePatchName(buffer, PU_HUDGFX);
	}

	// cache the credits font for entire game execution (why not?)
	j = CRED_FONTSTART;
	for (i = 0; i < CRED_FONTSIZE; i++)
	{
		sprintf(buffer, "CRFNT%.3d", j);
		j++;

		if (W_CheckNumForName(buffer) == LUMPERROR)
			cred_font[i] = NULL;
		else
			cred_font[i] = (patch_t *)W_CachePatchName(buffer, PU_HUDGFX);
	}

	//cache numbers too!
	for (i = 0; i < 10; i++)
	{
		sprintf(buffer, "STTNUM%d", i);
		tallnum[i] = (patch_t *)W_CachePatchName(buffer, PU_HUDGFX);
		sprintf(buffer, "NGTNUM%d", i);
		nightsnum[i] = (patch_t *) W_CachePatchName(buffer, PU_HUDGFX);
	}

	// minus for negative tallnums
	tallminus = (patch_t *)W_CachePatchName("STTMINUS", PU_HUDGFX);

	// cache the crosshairs, don't bother to know which one is being used,
	// just cache all 3, they're so small anyway.
	for (i = 0; i < HU_CROSSHAIRS; i++)
	{
		sprintf(buffer, "CROSHAI%c", '1'+i);
		crosshair[i] = (patch_t *)W_CachePatchName(buffer, PU_HUDGFX);
	}

	emblemicon = W_CachePatchName("EMBLICON", PU_HUDGFX);
	tokenicon = W_CachePatchName("TOKNICON", PU_HUDGFX);

	emeraldpics[0] = W_CachePatchName("CHAOS1", PU_HUDGFX);
	emeraldpics[1] = W_CachePatchName("CHAOS2", PU_HUDGFX);
	emeraldpics[2] = W_CachePatchName("CHAOS3", PU_HUDGFX);
	emeraldpics[3] = W_CachePatchName("CHAOS4", PU_HUDGFX);
	emeraldpics[4] = W_CachePatchName("CHAOS5", PU_HUDGFX);
	emeraldpics[5] = W_CachePatchName("CHAOS6", PU_HUDGFX);
	emeraldpics[6] = W_CachePatchName("CHAOS7", PU_HUDGFX);
	tinyemeraldpics[0] = W_CachePatchName("TEMER1", PU_HUDGFX);
	tinyemeraldpics[1] = W_CachePatchName("TEMER2", PU_HUDGFX);
	tinyemeraldpics[2] = W_CachePatchName("TEMER3", PU_HUDGFX);
	tinyemeraldpics[3] = W_CachePatchName("TEMER4", PU_HUDGFX);
	tinyemeraldpics[4] = W_CachePatchName("TEMER5", PU_HUDGFX);
	tinyemeraldpics[5] = W_CachePatchName("TEMER6", PU_HUDGFX);
	tinyemeraldpics[6] = W_CachePatchName("TEMER7", PU_HUDGFX);
}

// Initialise Heads up
// once at game startup.
//
void HU_Init(void)
{
#ifndef NONET
	COM_AddCommand("say", Command_Say_f);
	COM_AddCommand("sayto", Command_Sayto_f);
	COM_AddCommand("sayteam", Command_Sayteam_f);
	COM_AddCommand("csay", Command_CSay_f);
	RegisterNetXCmd(XD_SAY, Got_Saycmd);
#endif

	// set shift translation table
	shiftxform = english_shiftxform;

	HU_LoadGraphics();
}

static inline void HU_Stop(void)
{
	headsupactive = false;
}

//
// Reset Heads up when consoleplayer spawns
//
void HU_Start(void)
{
	if (headsupactive)
		HU_Stop();

	plr = &players[consoleplayer];

	headsupactive = true;
}

//======================================================================
//                            EXECUTION
//======================================================================

#ifndef NONET

// EVERY CHANGE IN THIS SCRIPT IS LOL XD! BY VINCYTM

static UINT32 chat_nummsg_log = 0;
static UINT32 chat_nummsg_min = 0;
static UINT32 chat_scroll = 0;		
static tic_t chat_scrolltime = 0;

static INT32 chat_maxscroll = 0;	// how far can we scroll? 

//static chatmsg_t chat_mini[CHAT_BUFSIZE];	// Display the last few messages sent.
//static chatmsg_t chat_log[CHAT_BUFSIZE];	// Keep every message sent to us in memory so we can scroll n shit, it's cool.

static char chat_log[CHAT_BUFSIZE][255];	// hold the last 48 or so messages in that log.
static char chat_mini[8][255];			// display up to 8 messages that will fade away / get overwritten
static tic_t chat_timers[8];

static boolean chat_scrollmedown = false;	// force instant scroll down on the chat log. Happens when you open it / send a message.

// remove text from minichat table

static INT16 addy = 0;	// use this to make the messages scroll smoothly when one fades away 

static void HU_removeChatText_Mini(void)
{
    // MPC: Don't create new arrays, just iterate through an existing one
	int i;
    for(i=0;i<chat_nummsg_min-1;i++) {
        strcpy(chat_mini[i], chat_mini[i+1]);
        chat_timers[i] = chat_timers[i+1];
    }
	chat_nummsg_min--;	// lost 1 msg.

	// use addy and make shit slide smoothly af.
	addy += (vid.width < 640) ? 8 : 6;

}

// same but w the log. TODO: optimize this and maybe merge in a single func? im bad at C.
static void HU_removeChatText_Log(void)
{
	// MPC: Don't create new arrays, just iterate through an existing one
	int i;
    for(i=0;i<chat_nummsg_log-1;i++) {
        strcpy(chat_log[i], chat_log[i+1]);
    }
    chat_nummsg_log--;	// lost 1 msg.
}
	
void HU_AddChatText(const char *text)
{
	
	// TODO: check if we're oversaturating the log (we can only log CHAT_BUFSIZE messages.)
	
	if (chat_nummsg_log >= CHAT_BUFSIZE)
		HU_removeChatText_Log();
	
	strcpy(chat_log[chat_nummsg_log], text);
	chat_nummsg_log++;
	
	if (chat_nummsg_min >= 8)
		HU_removeChatText_Mini();
	
	strcpy(chat_mini[chat_nummsg_min], text);
	chat_timers[chat_nummsg_min] = TICRATE*cv_chattime.value;
	chat_nummsg_min++;
}	

/** Runs a say command, sending an ::XD_SAY message.
  * A say command consists of a signed 8-bit integer for the target, an
  * unsigned 8-bit flag variable, and then the message itself.
  *
  * The target is 0 to say to everyone, 1 to 32 to say to that player, or -1
  * to -32 to say to everyone on that player's team. Note: This means you
  * have to add 1 to the player number, since they are 0 to 31 internally.
  *
  * The flag HU_SERVER_SAY will be set if it is the dedicated server speaking.
  *
  * This function obtains the message using COM_Argc() and COM_Argv().
  *
  * \param target    Target to send message to.
  * \param usedargs  Number of arguments to ignore.
  * \param flags     Set HU_CSAY for server/admin to CECHO everyone.
  * \sa Command_Say_f, Command_Sayteam_f, Command_Sayto_f, Got_Saycmd
  * \author Graue <graue@oceanbase.org>
  */
  
  
static void DoSayCommand(SINT8 target, size_t usedargs, UINT8 flags)
{
	XBOXSTATIC char buf[254];
	size_t numwords, ix;
	char *msg = &buf[2];
	const size_t msgspace = sizeof buf - 2;

	numwords = COM_Argc() - usedargs;
	I_Assert(numwords > 0);

	if (cv_mute.value && !(server || adminplayer == consoleplayer))	// TODO: Per Player mute.
	{
		HU_AddChatText(va("%s>ERROR: The chat is muted. You can't say anything.", "\x85"));
		return;
	}

	// Only servers/admins can CSAY.
	if(!server && adminplayer != consoleplayer)
		flags &= ~HU_CSAY;

	// We handle HU_SERVER_SAY, not the caller.
	flags &= ~HU_SERVER_SAY;
	if(dedicated && !(flags & HU_CSAY))
		flags |= HU_SERVER_SAY;

	buf[0] = target;
	buf[1] = flags;
	msg[0] = '\0';

	for (ix = 0; ix < numwords; ix++)
	{
		if (ix > 0)
			strlcat(msg, " ", msgspace);
		strlcat(msg, COM_Argv(ix + usedargs), msgspace);
	}
	
	if (strlen(msg) > 4 && strnicmp(msg, "/pm", 3) == 0)	// used /pm
	{
		// what we're gonna do now is check if the node exists
		// with that logic, characters 4 and 5 are our numbers:
		int spc = 1;	// used if nodenum[1] is a space.
		char *nodenum = (char*) malloc(3);
		strncpy(nodenum, msg+3, 5);
		// check for undesirable characters in our "number"
		if 	(((nodenum[0] < '0') || (nodenum[0] > '9')) || ((nodenum[1] < '0') || (nodenum[1] > '9')))
		{	
			// check if nodenum[1] is a space
			if (nodenum[1] == ' ')
				spc = 0;
				// let it slide
			else
			{	
				HU_AddChatText("\x82NOTICE: \x80Invalid command format. Correct format is \'/pm<node> \'.");
				return;
			}	
		}
		// I'm very bad at C, I swear I am, additional checks eww!
			if (spc != 0)
			{	
				if (msg[5] != ' ')
				{
					HU_AddChatText("\x82NOTICE: \x80Invalid command format. Correct format is \'/pm<node> \'.");
					return;
				}
			}
		
		target = atoi((const char*) nodenum);	// turn that into a number
		//CONS_Printf("%d\n", target);
		
		// check for target player, if it doesn't exist then we can't send the message!
		if (playeringame[target])	// player exists
			target++;				// even though playernums are from 0 to 31, target is 1 to 32, so up that by 1 to have it work!
		else
		{
			HU_AddChatText(va("\x82NOTICE: \x80Player %d does not exist.", target));	// same
			return;
		}
		buf[0] = target;
		const char *newmsg = msg+5+spc;
		memcpy(msg, newmsg, 255);
	}

	SendNetXCmd(XD_SAY, buf, strlen(msg) + 1 + msg-buf);
}

/** Send a message to everyone.
  * \sa DoSayCommand, Command_Sayteam_f, Command_Sayto_f
  * \author Graue <graue@oceanbase.org>
  */
static void Command_Say_f(void)
{
	if (COM_Argc() < 2)
	{
		CONS_Printf(M_GetText("say <message>: send a message\n"));
		return;
	}

	DoSayCommand(0, 1, 0);
}

/** Send a message to a particular person.
  * \sa DoSayCommand, Command_Sayteam_f, Command_Say_f
  * \author Graue <graue@oceanbase.org>
  */
static void Command_Sayto_f(void)
{
	INT32 target;

	if (COM_Argc() < 3)
	{
		CONS_Printf(M_GetText("sayto <playername|playernum> <message>: send a message to a player\n"));
		return;
	}

	target = nametonum(COM_Argv(1));
	if (target == -1)
	{
		CONS_Alert(CONS_NOTICE, M_GetText("No player with that name!\n"));
		return;
	}
	target++; // Internally we use 0 to 31, but say command uses 1 to 32.

	DoSayCommand((SINT8)target, 2, 0);
}

/** Send a message to members of the player's team.
  * \sa DoSayCommand, Command_Say_f, Command_Sayto_f
  * \author Graue <graue@oceanbase.org>
  */
static void Command_Sayteam_f(void)
{
	if (COM_Argc() < 2)
	{
		CONS_Printf(M_GetText("sayteam <message>: send a message to your team\n"));
		return;
	}

	if (dedicated)
	{
		CONS_Alert(CONS_NOTICE, M_GetText("Dedicated servers can't send team messages. Use \"say\".\n"));
		return;
	}

	DoSayCommand(-1, 1, 0);
}

/** Send a message to everyone, to be displayed by CECHO. Only
  * permitted to servers and admins.
  */
static void Command_CSay_f(void)
{
	if (COM_Argc() < 2)
	{
		CONS_Printf(M_GetText("csay <message>: send a message to be shown in the middle of the screen\n"));
		return;
	}

	if(!server && adminplayer != consoleplayer)
	{
		CONS_Alert(CONS_NOTICE, M_GetText("Only servers and admins can use csay.\n"));
		return;
	}

	DoSayCommand(0, 1, HU_CSAY);
}

static tic_t stop_spamming_you_cunt[MAXPLAYERS];

/** Receives a message, processing an ::XD_SAY command.
  * \sa DoSayCommand
  * \author Graue <graue@oceanbase.org>
  */
static void Got_Saycmd(UINT8 **p, INT32 playernum)
{
	SINT8 target;
	UINT8 flags;
	const char *dispname;
	char *msg;
	boolean action = false;
	char *ptr;
	
	CONS_Debug(DBG_NETPLAY,"Received SAY cmd from Player %d (%s)\n", playernum+1, player_names[playernum]);

	target = READSINT8(*p);
	flags = READUINT8(*p);
	msg = (char *)*p;
	SKIPSTRING(*p);

	if ((cv_mute.value || flags & (HU_CSAY|HU_SERVER_SAY)) && playernum != serverplayer && playernum != adminplayer)
	{
		CONS_Alert(CONS_WARNING, cv_mute.value ?
			M_GetText("Illegal say command received from %s while muted\n") : M_GetText("Illegal csay command received from non-admin %s\n"),
			player_names[playernum]);
		if (server)
		{
			XBOXSTATIC UINT8 buf[2];

			buf[0] = (UINT8)playernum;
			buf[1] = KICK_MSG_CON_FAIL;
			SendNetXCmd(XD_KICK, &buf, 2);
		}
		return;
	}

	//check for invalid characters (0x80 or above)
	{
		size_t i;
		const size_t j = strlen(msg);
		for (i = 0; i < j; i++)
		{
			if (msg[i] & 0x80)
			{
				CONS_Alert(CONS_WARNING, M_GetText("Illegal say command received from %s containing invalid characters\n"), player_names[playernum]);
				if (server)
				{
					XBOXSTATIC char buf[2];

					buf[0] = (char)playernum;
					buf[1] = KICK_MSG_CON_FAIL;
					SendNetXCmd(XD_KICK, &buf, 2);
				}
				return;
			}
		}
	}
	
	int spam_eatmsg = 0;
	
	// before we do anything, let's verify the guy isn't spamming, get this easier on us.
	
	//if (stop_spamming_you_cunt[playernum] != 0 && cv_chatspamprotection.value && !(flags & HU_CSAY))
	if (stop_spamming_you_cunt[playernum] != 0 && consoleplayer != playernum && cv_chatspamprotection.value && !(flags & HU_CSAY))
	{	
		CONS_Debug(DBG_NETPLAY,"Received SAY cmd too quickly from Player %d (%s), assuming as spam and blocking message.\n", playernum+1, player_names[playernum]);
		stop_spamming_you_cunt[playernum] = 4;
		spam_eatmsg = 1;
	}
	else
		stop_spamming_you_cunt[playernum] = 4;	// you can hold off for 4 tics, can you?
	
	// run the lua hook even if we were supposed to eat the msg, netgame consistency goes first.
	
/*#ifdef HAVE_BLUA
	if (LUAh_PlayerMsg(playernum, target, flags, msg, spam_eatmsg))
		return;
#endif*/
	// Kill PlayerMsg for now, it breaks the purpose of this EXE.
	
	if (spam_eatmsg)
		return;	// don't proceed if we were supposed to eat the message.
	
	// If it's a CSAY, just CECHO and be done with it.
	if (flags & HU_CSAY)
	{
		HU_SetCEchoDuration(5);
		I_OutputMsg("Server message: ");
		HU_DoCEcho(msg);
		return;
	}

	// Handle "/me" actions, but only in messages to everyone.
	if (target == 0 && strlen(msg) > 4 && strnicmp(msg, "/me ", 4) == 0)
	{
		msg += 4;
		action = true;
	}

	if (flags & HU_SERVER_SAY)
		dispname = "SERVER";
	else
		dispname = player_names[playernum];

	// Clean up message a bit
	// If you use a \r character, you can remove your name
	// from before the text and then pretend to be someone else!
	ptr = msg;
	while (*ptr != '\0')
	{
		if (*ptr == '\r')
			*ptr = ' ';

		ptr++;
	}

	// Show messages sent by you, to you, to your team, or to everyone:
	if (playernum == consoleplayer // By you
	|| (target == -1 && ST_SameTeam(&players[consoleplayer], &players[playernum])) // To your team
	|| target == 0 // To everyone
	|| consoleplayer == target-1) // To you
	{
		const char *prefix = "", *cstart = "", *cend = "", *adminchar = "\x82~\x83", *remotechar = "\x82@\x83", *fmt, *fmt2;
		char *tempchar = NULL;
		
		// In CTF and team match, color the player's name.
		if (G_GametypeHasTeams())
		{
			cend = "";
			if (players[playernum].ctfteam == 1) // red	
				cstart = "\x85";		
			else if (players[playernum].ctfteam == 2) // blue
				cstart = "\x84";
			
		}
		
		// player is a spectator?
		if (players[playernum].spectator)
				cstart = "\x86";	// grey name

		// Give admins and remote admins their symbols.
		if (playernum == serverplayer)
			tempchar = (char *)Z_Calloc(strlen(cstart) + strlen(adminchar) + 1, PU_STATIC, NULL);
		else if (playernum == adminplayer)
			tempchar = (char *)Z_Calloc(strlen(cstart) + strlen(remotechar) + 1, PU_STATIC, NULL);
		if (tempchar)
		{
			if (playernum == serverplayer)
				strcat(tempchar, adminchar);
			else
				strcat(tempchar, remotechar);
			strcat(tempchar, cstart);
			cstart = tempchar;
		}

		// Choose the proper format string for display.
		// Each format includes four strings: color start, display
		// name, color end, and the message itself.
		// '\4' makes the message yellow and beeps; '\3' just beeps.
		if (action)
		{	
			fmt = "\3* %s%s%s%s \x82%s\n";	// don't make /me yellow, yellow will be for mentions and PMs!
			fmt2 = "* %s%s%s%s \x82%s";
		}	
		else if (target == 0) // To everyone
		{
			fmt = "\3%s\x83<%s%s%s\x83>\x80 %s\n";
			fmt2 = "%s\x83<%s%s%s\x83>\x80 %s";
		}	
		else if (target-1 == consoleplayer) // To you
		{
			prefix = "\x82[PM]";
			cstart = "\x82";
			fmt = "\4%s<%s%s>%s\x80 %s\n";	// make this yellow, however.
			fmt2 = "%s<%s%s>%s\x80 %s";
		}
		else if (target > 0) // By you, to another player
		{
			// Use target's name.
			dispname = player_names[target-1];
			/*fmt = "\3\x82[TO]\x80%s%s%s* %s\n";
			fmt2 = "\x82[TO]\x80%s%s%s* %s";*/
			prefix = "\x82[TO]";
			cstart = "\x82";
			fmt = "\4%s<%s%s>%s\x80 %s\n";	// make this yellow, however.
			fmt2 = "%s<%s%s>%s\x80 %s";
			
		}
		else // To your team
		{
			if (players[playernum].ctfteam == 1) // red	
				prefix = "\x85[TEAM]";		
			else if (players[playernum].ctfteam == 2) // blue
				prefix = "\x84[TEAM]";
			else
				prefix = "\x83";	// makes sure this doesn't implode if you sayteam on non-team gamemodes
				
			fmt = "\3%s<%s%s>\x80%s %s\n";
			fmt2 = "%s<%s%s>\x80%s %s";
		
		}		
		
		if OLDCHAT
		{	
			CONS_Printf(fmt, prefix, cstart, dispname, cend, msg);	
			HU_AddChatText(va(fmt2, prefix, cstart, dispname, cend, msg));	// add it reguardless, in case we decide to change our mind about our chat type.
		}	
		else
		{	
			HU_AddChatText(va(fmt2, prefix, cstart, dispname, cend, msg));
			CON_LogMessage(va(fmt, prefix, cstart, dispname, cend, msg));	// save to log.txt
			if (cv_chatnotifications.value)
				S_StartSound(NULL, sfx_radio);
		}	
		
		if (tempchar)
			Z_Free(tempchar);
	}
#ifdef _DEBUG
	// I just want to point out while I'm here that because the data is still
	// sent to all players, techincally anyone can see your chat if they really
	// wanted to, even if you used sayto or sayteam.
	// You should never send any sensitive info through sayto for that reason.
	else
		CONS_Printf("Dropped chat: %d %d %s\n", playernum, target, msg);
#endif
}
#endif

// Handles key input and string input
//
static inline boolean HU_keyInChatString(char *s, char ch)
{
	size_t l;

	if ((ch >= HU_FONTSTART && ch <= HU_FONTEND && hu_font[ch-HU_FONTSTART])
	  || ch == ' ') // Allow spaces, of course
	{
		l = strlen(s);
		if (l < HU_MAXMSGLEN - 1)
		{
			if (c_input >= strlen(s))	// don't do anything complicated
			{
				s[l++] = ch;
				s[l]=0;
			}	
			else
			{	
				
				// move everything past c_input for new characters:
				INT32 m = HU_MAXMSGLEN-1;
				for (;(m>=c_input);m--)
				{
					if (s[m])
						s[m+1] = (s[m]);
				}
				s[c_input] = ch;		// and replace this.
			}
			c_input++;
			return true;
		}
		return false;
	}
	else if (ch == KEY_BACKSPACE)
	{
		if (c_input <= 0)
			return false;
		size_t i = c_input;
		if (!s[i-1])
			return false;
		
		if (i >= strlen(s)-1)
		{	
			s[strlen(s)-1] = 0;
			c_input--;
			return false;
		}	
		
		for (; (i < HU_MAXMSGLEN); i++)
		{	
			s[i-1] = s[i];
		}
		c_input--;
	}
	else if (ch != KEY_ENTER)
		return false; // did not eat key

	return true; // ate the key
}

//
//
void HU_Ticker(void)
{
	if (dedicated)
		return;

	hu_tick++;
	hu_tick &= 7; // currently only to blink chat input cursor

	if (PLAYER1INPUTDOWN(gc_scores))
		hu_showscores = !chat_on;
	else
		hu_showscores = false;
}

static boolean teamtalk = false;
/*static char chatchars[QUEUESIZE];
static INT32 head = 0, tail = 0;*/
// WHY DO YOU OVERCOMPLICATE EVERYTHING?????????

//
//
static void HU_queueChatChar(char c)
{
	// send automaticly the message (no more chat char)
	if (c == KEY_ENTER)
	{
		char buf[2+256];
		size_t ci = 2;
		char *msg = &buf[2];
		do {
			c = w_chat[-2+ci++];
			if (!c || (c >= ' ' && !(c & 0x80))) // copy printable characters and terminating '\0' only.
				buf[ci-1]=c;
		} while (c);
		size_t i = 0;
		for (;(i<HU_MAXMSGLEN);i++)
			w_chat[i] = 0;	// reset this.
		
		c_input = 0;
		
		// last minute mute check
		if (cv_mute.value && !(server || adminplayer == consoleplayer))
		{
			HU_AddChatText(va("%s>ERROR: The chat is muted. You can't say anything.", "\x85"));
			return;
		}
		
		INT32 target = 0;
		
		if (strlen(msg) > 4 && strnicmp(msg, "/pm", 3) == 0)	// used /pm
		{
			// what we're gonna do now is check if the node exists
			// with that logic, characters 4 and 5 are our numbers:
			
			// teamtalk can't send PMs, just don't send it, else everyone would be able to see it, and no one wants to see your sex RP sicko.
			if (teamtalk)
			{
				HU_AddChatText(va("%sCannot send sayto in Say-Team.", "\x85"));
				return;
			}	
			
			int spc = 1;	// used if nodenum[1] is a space.
			char *nodenum = (char*) malloc(3);
			strncpy(nodenum, msg+3, 5);
			// check for undesirable characters in our "number"
			if 	(((nodenum[0] < '0') || (nodenum[0] > '9')) || ((nodenum[1] < '0') || (nodenum[1] > '9')))
			{	
				// check if nodenum[1] is a space
				if (nodenum[1] == ' ')
					spc = 0;
					// let it slide
				else
				{	
					HU_AddChatText("\x82NOTICE: \x80Invalid command format. Correct format is \'/pm<node> \'.");
					return;
				}	
			}
			// I'm very bad at C, I swear I am, additional checks eww!
			if (spc != 0)
			{	
				if (msg[5] != ' ')
				{
					HU_AddChatText("\x82NOTICE: \x80Invalid command format. Correct format is \'/pm<node> \'.");
					return;
				}
			}
			
			target = atoi((const char*) nodenum);	// turn that into a number
			//CONS_Printf("%d\n", target);
		
			// check for target player, if it doesn't exist then we can't send the message!
			if (playeringame[target])	// player exists
				target++;				// even though playernums are from 0 to 31, target is 1 to 32, so up that by 1 to have it work!
			else
			{
				HU_AddChatText(va("\x82NOTICE: \x80Player %d does not exist.", target));	// same
				return;
			}
			// we need to get rid of the /pm<node>
			const char *newmsg = msg+5+spc;
			memcpy(msg, newmsg, 255);
		}	
		if (ci > 3) // don't send target+flags+empty message.
		{
			if (teamtalk)
				buf[0] = -1; // target
			else
				buf[0] = target;
			
			buf[1] = 0; // flags
			SendNetXCmd(XD_SAY, buf, 2 + strlen(&buf[2]) + 1);
		}
		return;
	}
}

void HU_clearChatChars(void)
{
	size_t i = 0;
	for (;i<HU_MAXMSGLEN;i++)
		w_chat[i] = 0;	// reset this.
	chat_on = false;
	c_input = 0;
}

static boolean justscrolleddown;
static boolean justscrolledup;

//
// Returns true if key eaten
//
boolean HU_Responder(event_t *ev)
{
	UINT8 c=0;
		
	if (ev->type != ev_keydown)
		return false;

	// only KeyDown events now...
	
	if (!chat_on)
	{
		// enter chat mode
		if ((ev->data1 == gamecontrol[gc_talkkey][0] || ev->data1 == gamecontrol[gc_talkkey][1])
			&& netgame && (!cv_mute.value || server || (adminplayer == consoleplayer)))
		{
			if (cv_mute.value && !(server || adminplayer == consoleplayer))
				return false;
			chat_on = true;
			w_chat[0] = 0;
			teamtalk = false;
			chat_scrollmedown = true;
			return true;
		}
		if ((ev->data1 == gamecontrol[gc_teamkey][0] || ev->data1 == gamecontrol[gc_teamkey][1])
			&& netgame && (!cv_mute.value || server || (adminplayer == consoleplayer)))
		{
			if (cv_mute.value && !(server || adminplayer == consoleplayer))
				return false;
			chat_on = true;
			w_chat[0] = 0;
			teamtalk = true;
			chat_scrollmedown = true;
			return true;
		}
	}
	else // if chat_on
	{
		
		// Ignore modifier keys
		// Note that we do this here so users can still set
		// their chat keys to one of these, if they so desire.
		if (ev->data1 == KEY_LSHIFT || ev->data1 == KEY_RSHIFT
		 || ev->data1 == KEY_LCTRL || ev->data1 == KEY_RCTRL
		 || ev->data1 == KEY_LALT || ev->data1 == KEY_RALT)
			return true;

		c = (UINT8)ev->data1;
		
		// capslock
		if (c && c == KEY_CAPSLOCK)	// it's a toggle.
		{	
			if (capslock)
				capslock = false;
			else	
				capslock = true;
			return true;
		}	
		
		// use console translations	

		if (c >= 'a' && c <= 'z')
		{
			if (capslock ^ shiftdown)
				c = shiftxform[c];
		}
		else if (shiftdown)
			c = shiftxform[c];
				
		// pasting. pasting is cool. chat is a bit limited, though :(
		if ((c == 'v' || c == 'V') && ctrldown)
		{
			const char *paste = I_ClipboardPaste();
			
			// create a dummy string real quickly
			
			if (paste == NULL)
				return true;
			
			size_t chatlen = strlen(w_chat);
			size_t pastelen = strlen(paste);
			if (chatlen+pastelen > HU_MAXMSGLEN)
				return true; // we can't paste this!!
			
			if (c_input >= strlen(w_chat))	// add it at the end of the string.
			{
				memcpy(&w_chat[chatlen], paste, pastelen);	// copy all of that.
				c_input += pastelen;
				/*size_t i = 0;
				for (;i<pastelen;i++)
				{
					HU_queueChatChar(paste[i]);				// queue it so that it's actually sent. (this chat write thing is REALLY messy.)
				}*/
				return true;
			}
			else	// otherwise, we need to shift everything and make space, etc etc
			{	
				size_t i = HU_MAXMSGLEN-1;
				for (; i>=c_input;i--)
				{
					if (w_chat[i])
						w_chat[i+pastelen] = w_chat[i];
					
				}
				memcpy(&w_chat[c_input], paste, pastelen);	// copy all of that.
				c_input += pastelen;
				return true;
			}
		}
		
		if (HU_keyInChatString(w_chat,c))
		{	
			HU_queueChatChar(c);
		}	
		if (c == KEY_ENTER)
		{	
			chat_on = false;
			c_input = 0;			// reset input cursor
			chat_scrollmedown = true; // you hit enter, so you might wanna autoscroll to see what you just sent. :)
		}	
		else if (c == KEY_ESCAPE)
		{	
			chat_on = false;
			c_input = 0;			// reset input cursor
		}	
		else if ((c == KEY_UPARROW || c == KEY_MOUSEWHEELUP) && chat_scroll > 0)	// CHAT SCROLLING YAYS!
		{
			chat_scroll--;
			justscrolledup = true;
			chat_scrolltime = 4;
		}	
		else if ((c == KEY_DOWNARROW || c == KEY_MOUSEWHEELDOWN) && chat_scroll < chat_maxscroll && chat_maxscroll > 0)
		{	
			chat_scroll++;
			justscrolleddown = true;
			chat_scrolltime = 4;
		}
		else if (c == KEY_LEFTARROW && c_input != 0)	// i said go back
			c_input--;
		else if (c == KEY_RIGHTARROW && c_input < strlen(w_chat))
			c_input++;	
		return true;
	}
	return false;
}

//======================================================================
//                         HEADS UP DRAWING
//======================================================================

// Gets string colormap, used for 0x80 color codes
//
static UINT8 *CHAT_GetStringColormap(INT32 colorflags)	// pasted from video.c, sorry for the mess.
{
	switch ((colorflags & V_CHARCOLORMASK) >> V_CHARCOLORSHIFT)
	{
	case 1: // 0x81, purple
		return purplemap;
	case 2: // 0x82, yellow
		return yellowmap;
	case 3: // 0x83, lgreen
		return lgreenmap;
	case 4: // 0x84, blue
		return bluemap;
	case 5: // 0x85, red
		return redmap;
	case 6: // 0x86, gray
		return graymap;
	case 7: // 0x87, orange
		return orangemap;
	default: // reset
		return NULL;
	}
}

// Precompile a wordwrapped string to any given width.
// This is a muuuch better method than V_WORDWRAP.
// again stolen and modified a bit from video.c, don't mind me, will need to rearrange this one day.
// this one is simplified for the chat drawer.
char *CHAT_WordWrap(INT32 x, INT32 w, INT32 option, const char *string)
{
	int c;
	size_t chw, i, lastusablespace = 0;
	size_t slen;	
	char *newstring = Z_StrDup(string);
	INT32 spacewidth = (vid.width < 640) ? 8 : 4, charwidth = (vid.width < 640) ? 8 : 4;

	slen = strlen(string);
	x = 0;

	for (i = 0; i < slen; ++i)
	{
		c = newstring[i];
		if ((UINT8)c >= 0x80 && (UINT8)c <= 0x89) //color parsing! -Inuyasha 2.16.09
			continue;

		if (c == '\n')
		{
			x = 0;
			lastusablespace = 0;
			continue;
		}

		if (!(option & V_ALLOWLOWERCASE))
			c = toupper(c);
		c -= HU_FONTSTART;

		if (c < 0 || c >= HU_FONTSIZE || !hu_font[c])
		{
			chw = spacewidth;
			lastusablespace = i;
		}
		else
			chw = charwidth;

		x += chw;

		if (lastusablespace != 0 && x > w)
		{
			//CONS_Printf("Wrap at index %d\n", i);
			newstring[lastusablespace] = '\n';
			i = lastusablespace+1;	
			lastusablespace = 0;
			x = 0;
		}
	}
	return newstring;
}


// 30/7/18: chaty is now the distance at which the lowest point of the chat will be drawn if that makes any sense.

INT16 chatx = 13, chaty = 169;	// let's use this as our coordinates, shh

// chat stuff by VincyTM LOL XD!

// HU_DrawMiniChat

static void HU_drawMiniChat(void)
{
	if (!chat_nummsg_min)
		return;	// needless to say it's useless to do anything if we don't have anything to draw.

	INT32 x = chatx+2;
	INT32 charwidth = 4, charheight = 6;
	INT32 dx = 0, dy = 0;
	size_t i = chat_nummsg_min;
	boolean prev_linereturn = false;	// a hack to prevent double \n while I have no idea why they happen in the first place.

	INT32 msglines = 0;
	// process all messages once without rendering anything or doing anything fancy so that we know how many lines each message has...

	for (; i>0; i--)
	{
		const char *msg = CHAT_WordWrap(x+2, cv_chatwidth.value-(charwidth*2), V_SNAPTOBOTTOM|V_SNAPTOLEFT|V_ALLOWLOWERCASE, chat_mini[i-1]);
		size_t j = 0;
		INT32 linescount = 0;

		while(msg[j])	// iterate through msg
		{
			if (msg[j] < HU_FONTSTART)	// don't draw
			{
				if (msg[j] == '\n')	// get back down.
				{
					++j;
					if (!prev_linereturn)
					{	
						linescount += 1;
						dx = 0;
					}	
					prev_linereturn = true;
					continue;
				}
				else if (msg[j] & 0x80) // stolen from video.c, nice.
				{
					++j;
					continue;
				}

				++j;
			}
			else
			{
				j++;
			}
			prev_linereturn = false;
			dx += charwidth;
			if (dx >= cv_chatwidth.value)
			{
				dx = 0;
				linescount += 1;
			}
		}
		dy = 0;
		dx = 0;
		msglines += linescount+1;
	}

	INT32 y = chaty - charheight*(msglines+1);
	dx = 0;
	dy = 0;
	i = 0;
	prev_linereturn = false;

	for (; i<=(chat_nummsg_min-1); i++)	// iterate through our hot messages
	{
		INT32 clrflag = 0;
		INT32 timer = ((cv_chattime.value*TICRATE)-chat_timers[i]) - cv_chattime.value*TICRATE+9;	// see below...
		INT32 transflag = (timer >= 0 && timer <= 9) ? (timer*V_10TRANS) : 0;	// you can make bad jokes out of this one.
		size_t j = 0;
		const char *msg = CHAT_WordWrap(x+2, cv_chatwidth.value-(charwidth*2), V_SNAPTOBOTTOM|V_SNAPTOLEFT|V_ALLOWLOWERCASE, chat_mini[i]);	// get the current message, and word wrap it.

		while(msg[j])	// iterate through msg
		{
			if (msg[j] < HU_FONTSTART)	// don't draw
			{
				if (msg[j] == '\n')	// get back down.
				{
					++j;
					if (!prev_linereturn)
					{	
						dy += charheight;
						dx = 0;
					}	
					prev_linereturn = true;
					continue;
				}
				else if (msg[j] & 0x80) // stolen from video.c, nice.
				{
					clrflag = ((msg[j] & 0x7f) << V_CHARCOLORSHIFT) & V_CHARCOLORMASK;
					++j;
					continue;
				}

				++j;
			}
			else
			{
				UINT8 *colormap = CHAT_GetStringColormap(clrflag);

				if (cv_chatbacktint.value)	// on request of wolfy
					V_DrawFillConsoleMap(x + dx + 2, y+dy, charwidth, charheight, 239|V_SNAPTOBOTTOM|V_SNAPTOLEFT);

				V_DrawChatCharacter(x + dx + 2, y+dy, msg[j++] |V_SNAPTOBOTTOM|V_SNAPTOLEFT|transflag, !cv_allcaps.value, colormap);
			}

			dx += charwidth;
			prev_linereturn = false;
			if (dx >= cv_chatwidth.value)
			{
				dx = 0;
				dy += charheight;
			}
		}
		dy += charheight;
		dx = 0;
	}

	// decrement addy and make that shit smooth:
	addy /= 2;

}

// HU_DrawUpArrow
// You see, we don't have arrow graphics in 2.1 and I'm too lazy to include a 2 bytes file for it.

static void HU_DrawUpArrow(INT32 x, INT32 y, INT32 options)
{
	// Ok I'm super lazy so let's make this as the worst draw function:
	V_DrawFill(x+2, y, 1, 1, 103|options);
	V_DrawFill(x+1, y+1, 3, 1, 103|options);
	V_DrawFill(x, y+2, 5, 1, 103|options);	// that's the yellow part, I swear
	
	V_DrawFill(x+3, y, 1, 1, 26|options);
	V_DrawFill(x+4, y+1, 1, 1, 26|options);
	V_DrawFill(x+5, y+2, 1, 1, 26|options);
	V_DrawFill(x, y+3, 6, 1, 26|options);	// that's the black part. no racism intended. i swear.
}

// HU_DrawDownArrow
// Should we talk about anime waifus to pass the time? This feels retarded.

static void HU_DrawDownArrow(INT32 x, INT32 y, INT32 options)
{
	// Ok I'm super lazy so let's make this as the worst draw function:
	V_DrawFill(x, y, 6, 1, 26|options);
	V_DrawFill(x, y+1, 5, 1, 26|options);
	V_DrawFill(x+1, y+2, 3, 1, 26|options);
	V_DrawFill(x+2, y+3, 1, 1, 26|options);	// that's the black part. no racism intended. i swear.
	
	V_DrawFill(x, y, 5, 1, 103|options);
	V_DrawFill(x+1, y+1, 3, 1, 103|options);
	V_DrawFill(x+2, y+2, 1, 1, 103|options);	// that's the yellow part, I swear
}	

// HU_DrawChatLog
// TODO: fix dumb word wrapping issues

static void HU_drawChatLog(INT32 offset)
{

	// before we do anything, make sure that our scroll position isn't "illegal";
	if (chat_scroll > chat_maxscroll)
		chat_scroll = chat_maxscroll;

	INT32 charwidth = 4, charheight = 6;
	INT32 x = chatx+2, y = chaty - offset*charheight - (chat_scroll*charheight) - cv_chatheight.value*charheight - 12, dx = 0, dy = 0;
	UINT32 i = 0;
	INT32 chat_topy = y + chat_scroll*charheight;
	INT32 chat_bottomy = chat_topy + cv_chatheight.value*charheight;
	boolean atbottom = false;

	V_DrawFillConsoleMap(chatx, chat_topy, cv_chatwidth.value, cv_chatheight.value*charheight +2, 239|V_SNAPTOBOTTOM|V_SNAPTOLEFT);	// log box

	for (i=0; i<chat_nummsg_log; i++)	// iterate through our chatlog
	{
		INT32 clrflag = 0;
		INT32 j = 0;
		const char *msg = CHAT_WordWrap(x+2, cv_chatwidth.value-(charwidth*2), V_SNAPTOBOTTOM|V_SNAPTOLEFT|V_ALLOWLOWERCASE, chat_log[i]);	// get the current message, and word wrap it.
		while(msg[j])	// iterate through msg
		{
			if (msg[j] < HU_FONTSTART)	// don't draw
			{
				if (msg[j] == '\n')	// get back down.
				{
					++j;
					dy += charheight;
					dx = 0;
					continue;
				}
				else if (msg[j] & 0x80) // stolen from video.c, nice.
				{
					clrflag = ((msg[j] & 0x7f) << V_CHARCOLORSHIFT) & V_CHARCOLORMASK;
					++j;
					continue;
				}

				++j;
			}
			else
			{
				if ((y+dy+2 >= chat_topy) && (y+dy < (chat_bottomy)))
				{
					UINT8 *colormap = CHAT_GetStringColormap(clrflag);
					V_DrawChatCharacter(x + dx + 2, y+dy+2, msg[j++] |V_SNAPTOBOTTOM|V_SNAPTOLEFT, !cv_allcaps.value, colormap);
				}
				else
					j++;	// don't forget to increment this or we'll get stuck in the limbo.
			}

			dx += charwidth;
			if (dx >= cv_chatwidth.value-charwidth-2 && i<chat_nummsg_log && msg[j] >= HU_FONTSTART) // end of message shouldn't count, nor should invisible characters!!!!
			{
				dx = 0;
				dy += charheight;
			}
		}
		dy += charheight;
		dx = 0;
	}

	if (((chat_scroll >= chat_maxscroll) || (chat_scrollmedown)) && !(justscrolleddown || justscrolledup || chat_scrolltime))	// was already at the bottom of the page before new maxscroll calculation and was NOT scrolling.
	{
		atbottom = true;	// we should scroll
	}
	chat_scrollmedown = false;

	// getmaxscroll through a lazy hack. We do all these loops, so let's not do more loops that are gonna lag the game more. :P
	chat_maxscroll = (dy/charheight);	// welcome to C, we don't know what min() and max() are.
	if (chat_maxscroll <= (UINT32)cv_chatheight.value)
		chat_maxscroll = 0;
	else
		chat_maxscroll -= cv_chatheight.value;

	// if we're not bound by the time, autoscroll for next frame:
	if (atbottom)
		chat_scroll = chat_maxscroll;

	// draw arrows to indicate that we can (or not) scroll.

	if (chat_scroll > 0)
		HU_DrawUpArrow(chatx-8, ((justscrolledup) ? (chat_topy-1) : (chat_topy)), V_SNAPTOBOTTOM | V_SNAPTOLEFT);
	if (chat_scroll < chat_maxscroll)
		HU_DrawDownArrow(chatx-8, chat_bottomy-((justscrolleddown) ? 3 : 4), V_SNAPTOBOTTOM | V_SNAPTOLEFT);

	justscrolleddown = false;
	justscrolledup = false;
}

//
// HU_DrawChat
//
// Draw chat input
//

static INT16 typelines = 1;	// number of drawfill lines we need. it's some weird hack and might be one frame off but I'm lazy to make another loop.
static void HU_DrawChat(void)
{
	INT32 charwidth = 4, charheight = 6;
	INT32 t = 0, c = 0, y = chaty - (typelines*charheight);
	UINT32 i = 0;
	const char *ntalk = "Say: ", *ttalk = "Team: ";
	const char *talk = ntalk;

	if (teamtalk)
	{
		talk = ttalk;
#if 0
		if (players[consoleplayer].ctfteam == 1)
			t = 0x500;  // Red
		else if (players[consoleplayer].ctfteam == 2)
			t = 0x400; // Blue
#endif
	}

	V_DrawFillConsoleMap(chatx, y-1, cv_chatwidth.value, (typelines*charheight), 239 | V_SNAPTOBOTTOM | V_SNAPTOLEFT);

	while (talk[i])
	{
		if (talk[i] < HU_FONTSTART)
			++i;
		else
			V_DrawChatCharacter(chatx + c + 2, y, talk[i++] |V_SNAPTOBOTTOM|V_SNAPTOLEFT, !cv_allcaps.value, NULL);

		c += charwidth;
	}

	i = 0;
	typelines = 1;

	if ((strlen(w_chat) == 0 || c_input == 0) && hu_tick < 4)
		V_DrawChatCharacter(chatx + 2 + c, y+1, '_' |V_SNAPTOBOTTOM|V_SNAPTOLEFT|t, !cv_allcaps.value, NULL);

	while (w_chat[i])
	{
		boolean skippedline = false;
		if (c_input == (i+1))
		{
			int cursorx = (c+charwidth < cv_chatwidth.value-charwidth) ? (chatx + 2 + c+charwidth) : (chatx+1);	// we may have to go down.
			int cursory = (cursorx != chatx+1) ? (y) : (y+charheight);
			if (hu_tick < 4)
				V_DrawChatCharacter(cursorx, cursory+1, '_' |V_SNAPTOBOTTOM|V_SNAPTOLEFT|t, !cv_allcaps.value, NULL);

			if (cursorx == chatx+1)	// a weirdo hack
			{
				typelines += 1;
				skippedline = true;
			}

		}

		//Hurdler: isn't it better like that?
		if (w_chat[i] < HU_FONTSTART)
			++i;
		else
			V_DrawChatCharacter(chatx + c + 2, y, w_chat[i++] | V_SNAPTOBOTTOM|V_SNAPTOLEFT | t, !cv_allcaps.value, NULL);

		c += charwidth;
		if (c > cv_chatwidth.value-(charwidth*2) && !skippedline)
		{
			c = 0;
			y += charheight;
			typelines += 1;
		}
	}

	// handle /pm list.
	if (strnicmp(w_chat, "/pm", 3) == 0 && vid.width >= 400 && !teamtalk)	// 320x200 unsupported kthxbai
	{
		i = 0;
		INT32 count = 0;
		INT32 p_dispy = chaty - charheight -1;
		for(i=0; (i<MAXPLAYERS); i++)
		{

			// filter: (code needs optimization pls help I'm bad with C)
			if (w_chat[3])
			{

				// right, that's half important: (w_chat[4] may be a space since /pm0 msg is perfectly acceptable!)
				if ( ( ((w_chat[3] != 0) && ((w_chat[3] < '0') || (w_chat[3] > '9'))) || ((w_chat[4] != 0) && (((w_chat[4] < '0') || (w_chat[4] > '9'))))) && (w_chat[4] != ' '))
					break;


				char *nodenum = (char*) malloc(3);
				strncpy(nodenum, w_chat+3, 4);
				UINT32 n = atoi((const char*) nodenum);	// turn that into a number
				// special cases:

				if ((n == 0) && !(w_chat[4] == '0'))
				{
					if (!(i<10))
						continue;
				}
				else if ((n == 1) && !(w_chat[3] == '0'))
				{
					if (!((i == 1) || ((i >= 10) && (i <= 19))))
						continue;
				}
				else if ((n == 2) && !(w_chat[3] == '0'))
				{
					if (!((i == 2) || ((i >= 20) && (i <= 29))))
						continue;
				}
				else if ((n == 3) && !(w_chat[3] == '0'))
				{
					if (!((i == 3) || ((i >= 30) && (i <= 31))))
						continue;
				}
				else	// general case.
				{
					if (i != n)
						continue;
				}
			}

			if (playeringame[i])
			{
				char name[MAXPLAYERNAME+1];
				strlcpy(name, player_names[i], 7);	// shorten name to 7 characters.
				V_DrawFillConsoleMap(chatx+ cv_chatwidth.value + 2, p_dispy- (6*count), 48, 6, 239 | V_SNAPTOBOTTOM | V_SNAPTOLEFT);	// fill it like the chat so the text doesn't become hard to read because of the hud.
				V_DrawSmallString(chatx+ cv_chatwidth.value + 4, p_dispy- (6*count), V_SNAPTOBOTTOM|V_SNAPTOLEFT|V_ALLOWLOWERCASE, va("\x82%d\x80 - %s", i, name));
				count++;
			}
		}
		if (count == 0)	// no results.
		{
			V_DrawFillConsoleMap(chatx-50, p_dispy- (6*count), 48, 6, 239 | V_SNAPTOBOTTOM | V_SNAPTOLEFT);	// fill it like the chat so the text doesn't become hard to read because of the hud.
			V_DrawSmallString(chatx-48, p_dispy- (6*count), V_SNAPTOBOTTOM|V_SNAPTOLEFT|V_ALLOWLOWERCASE, "NO RESULT.");
		}
	}

	HU_drawChatLog(typelines-1);	// typelines is the # of lines we're typing. If there's more than 1 then the log should scroll up to give us more space.

}

// why the fuck would you use this...

static void HU_DrawChat_Old(void)
{
	INT32 t = 0, c = 0, y = HU_INPUTY;
	size_t i = 0;
	const char *ntalk = "Say: ", *ttalk = "Say-Team: ";
	const char *talk = ntalk;
	INT32 charwidth = 8 * con_scalefactor; //SHORT(hu_font['A'-HU_FONTSTART]->width) * con_scalefactor;
	INT32 charheight = 8 * con_scalefactor; //SHORT(hu_font['A'-HU_FONTSTART]->height) * con_scalefactor;
	if (teamtalk)
	{
		talk = ttalk;
#if 0
		if (players[consoleplayer].ctfteam == 1)
			t = 0x500;  // Red
		else if (players[consoleplayer].ctfteam == 2)
			t = 0x400; // Blue
#endif
	}

	while (talk[i])
	{
		if (talk[i] < HU_FONTSTART)
		{
			++i;
			//charwidth = 4 * con_scalefactor;
		}
		else
		{
			//charwidth = SHORT(hu_font[talk[i]-HU_FONTSTART]->width) * con_scalefactor;
			V_DrawCharacter(HU_INPUTX + c, y, talk[i++] | cv_constextsize.value | V_NOSCALESTART, !cv_allcaps.value);
		}
		c += charwidth;
	}
	
	if ((strlen(w_chat) == 0 || c_input == 0) && hu_tick < 4)
		V_DrawCharacter(HU_INPUTX+c, y+2*con_scalefactor, '_' |cv_constextsize.value | V_NOSCALESTART|t, !cv_allcaps.value);
	
	i = 0;
	while (w_chat[i])
	{
		
		if (c_input == (i+1) && hu_tick < 4)
		{
			int cursorx = (HU_INPUTX+c+charwidth < vid.width) ? (HU_INPUTX + c + charwidth) : (HU_INPUTX);	// we may have to go down.
			int cursory = (cursorx != HU_INPUTX) ? (y) : (y+charheight);
			V_DrawCharacter(cursorx, cursory+2*con_scalefactor, '_' |cv_constextsize.value | V_NOSCALESTART|t, !cv_allcaps.value);	
		}	
		
		//Hurdler: isn't it better like that?
		if (w_chat[i] < HU_FONTSTART)
		{
			++i;
			//charwidth = 4 * con_scalefactor;
		}
		else
		{
			//charwidth = SHORT(hu_font[w_chat[i]-HU_FONTSTART]->width) * con_scalefactor;
			V_DrawCharacter(HU_INPUTX + c, y, w_chat[i++] | cv_constextsize.value | V_NOSCALESTART | t, !cv_allcaps.value);
		}

		c += charwidth;
		if (c >= vid.width)
		{
			c = 0;
			y += charheight;
		}
	}
}


// draw the Crosshair, at the exact center of the view.
//
// Crosshairs are pre-cached at HU_Init

static inline void HU_DrawCrosshair(void)
{
	INT32 i, y;

	i = cv_crosshair.value & 3;
	if (!i)
		return;

	if ((netgame || multiplayer) && players[displayplayer].spectator)
		return;

#ifdef HWRENDER
	if (rendermode != render_soft)
		y = (INT32)gr_basewindowcentery;
	else
#endif
		y = viewwindowy + (viewheight>>1);

	V_DrawScaledPatch(vid.width>>1, y, V_NOSCALESTART|V_OFFSET|V_TRANSLUCENT, crosshair[i - 1]);
}

static inline void HU_DrawCrosshair2(void)
{
	INT32 i, y;

	i = cv_crosshair2.value & 3;
	if (!i)
		return;

	if ((netgame || multiplayer) && players[secondarydisplayplayer].spectator)
		return;

#ifdef HWRENDER
	if (rendermode != render_soft)
		y = (INT32)gr_basewindowcentery;
	else
#endif
		y = viewwindowy + (viewheight>>1);

	if (splitscreen)
	{
#ifdef HWRENDER
		if (rendermode != render_soft)
			y += (INT32)gr_viewheight;
		else
#endif
			y += viewheight;

		V_DrawScaledPatch(vid.width>>1, y, V_NOSCALESTART|V_OFFSET|V_TRANSLUCENT, crosshair[i - 1]);
	}
}

static void HU_DrawCEcho(void)
{
	INT32 i = 0;
	INT32 y = (BASEVIDHEIGHT/2)-4;
	INT32 pnumlines = 0;

	UINT32 realflags = cechoflags;
	INT32 realalpha = (INT32)((cechoflags & V_ALPHAMASK) >> V_ALPHASHIFT);

	char *line;
	char *echoptr;
	char temp[1024];

	for (i = 0; cechotext[i] != '\0'; ++i)
		if (cechotext[i] == '\\')
			pnumlines++;

	y -= (pnumlines-1)*((realflags & V_RETURN8) ? 4 : 6);

	// Prevent crashing because I'm sick of this
	if (y < 0)
	{
		CONS_Alert(CONS_WARNING, "CEcho contained too many lines, not displaying\n");
		cechotimer = 0;
		return;
	}

	// Automatic fadeout
	if (realflags & V_AUTOFADEOUT)
	{
		UINT32 tempalpha = (UINT32)max((INT32)(10 - cechotimer), realalpha);

		realflags &= ~V_ALPHASHIFT;
		realflags |= (tempalpha << V_ALPHASHIFT);
	}

	strcpy(temp, cechotext);
	echoptr = &temp[0];

	while (*echoptr != '\0')
	{
		line = strchr(echoptr, '\\');

		if (line == NULL)
			break;

		*line = '\0';

		V_DrawCenteredString(BASEVIDWIDTH/2, y, realflags, echoptr);
		y += ((realflags & V_RETURN8) ? 8 : 12);

		echoptr = line;
		echoptr++;
	}

	--cechotimer;
}

static void HU_drawGametype(void)
{
	INT32 i = 0;

	for (i = 0; gametype_cons_t[i].strvalue; i++)
	{
		if (gametype_cons_t[i].value == gametype)
		{
			if (splitscreen)
				V_DrawString(4, 184, 0, gametype_cons_t[i].strvalue);
			else
				V_DrawString(4, 192, 0, gametype_cons_t[i].strvalue);
			return;
		}
	}
}

//
// demo info stuff
//
UINT32 hu_demoscore;
UINT32 hu_demotime;
UINT16 hu_demorings;

static void HU_DrawDemoInfo(void)
{
	V_DrawString(4, 188-24, V_YELLOWMAP, va(M_GetText("%s's replay"), player_names[0]));
	if (modeattacking)
	{
		V_DrawString(4, 188-16, V_YELLOWMAP|V_MONOSPACE, "SCORE:");
		V_DrawRightAlignedString(120, 188-16, V_MONOSPACE, va("%d", hu_demoscore));

		V_DrawString(4, 188- 8, V_YELLOWMAP|V_MONOSPACE, "TIME:");
		if (hu_demotime != UINT32_MAX)
			V_DrawRightAlignedString(120, 188- 8, V_MONOSPACE, va("%i:%02i.%02i",
				G_TicsToMinutes(hu_demotime,true),
				G_TicsToSeconds(hu_demotime),
				G_TicsToCentiseconds(hu_demotime)));
		else
			V_DrawRightAlignedString(120, 188- 8, V_MONOSPACE, "--:--.--");

		if (modeattacking == ATTACKING_RECORD)
		{
			V_DrawString(4, 188   , V_YELLOWMAP|V_MONOSPACE, "RINGS:");
			V_DrawRightAlignedString(120, 188   , V_MONOSPACE, va("%d", hu_demorings));
		}
	}
}

// Heads up displays drawer, call each frame
//
void HU_Drawer(void)
{
	// draw chat string plus cursor
	if (chat_on)
	{	
		// count down the scroll timer. 
		if (chat_scrolltime > 0)
			chat_scrolltime--;
		if (!OLDCHAT)
			HU_DrawChat();
		else
			HU_DrawChat_Old();	// why the fuck.........................
	}
	else
	{
		if (!OLDCHAT)
		{	
			HU_drawMiniChat();		// draw messages in a cool fashion.
			chat_scrolltime = 0;	// do scroll anyway.
			typelines = 0;			// make sure that the chat doesn't have a weird blinking huge ass square if we typed a lot last time.
		}	
	}

	if (netgame)	// would handle that in hu_drawminichat, but it's actually kinda awkward when you're typing a lot of messages. (only handle that in netgames duh)
	{
		size_t i = 0;
		
		// handle spam while we're at it:
		for(; (i<MAXPLAYERS); i++)
		{	
			if (stop_spamming_you_cunt[i] > 0)
				stop_spamming_you_cunt[i]--;
		}	
		
		// handle chat timers
		for (i=0; (i<chat_nummsg_min); i++)
		{	
			if (chat_timers[i] > 0)
				chat_timers[i]--;
			else
				HU_removeChatText_Mini();
		}
	}

	if (cechotimer)
		HU_DrawCEcho();

	if (demoplayback && hu_showscores)
		HU_DrawDemoInfo();

	if (!Playing()
	 || gamestate == GS_INTERMISSION || gamestate == GS_CUTSCENE
	 || gamestate == GS_CREDITS      || gamestate == GS_EVALUATION
	 || gamestate == GS_GAMEEND)
		return;

	// draw multiplayer rankings
	if (hu_showscores)
	{
		if (netgame || multiplayer)
		{
#ifdef HAVE_BLUA
			if (LUA_HudEnabled(hud_rankings))
#endif
			HU_DrawRankings();
			if (gametype == GT_COOP)
				HU_DrawNetplayCoopOverlay();
		}
		else
			HU_DrawCoopOverlay();
#ifdef HAVE_BLUA
		LUAh_ScoresHUD();
#endif
	}

	if (gamestate != GS_LEVEL)
		return;

	// draw the crosshair, not when viewing demos nor with chasecam
	if (!automapactive && cv_crosshair.value && !demoplayback && !camera.chase && !players[displayplayer].spectator)
		HU_DrawCrosshair();

	if (!automapactive && cv_crosshair2.value && !demoplayback && !camera2.chase && !players[secondarydisplayplayer].spectator)
		HU_DrawCrosshair2();

	// draw desynch text
	if (hu_resynching)
	{
		static UINT32 resynch_ticker = 0;
		char resynch_text[14];
		UINT32 i;

		// Animate the dots
		resynch_ticker++;
		strcpy(resynch_text, "Resynching");
		for (i = 0; i < (resynch_ticker / 16) % 4; i++)
			strcat(resynch_text, ".");

		V_DrawCenteredString(BASEVIDWIDTH/2, 180, V_YELLOWMAP | V_ALLOWLOWERCASE, resynch_text);
	}
}

//======================================================================
//                 HUD MESSAGES CLEARING FROM SCREEN
//======================================================================

// Clear old messages from the borders around the view window
// (only for reduced view, refresh the borders when needed)
//
// startline: y coord to start clear,
// clearlines: how many lines to clear.
//
static INT32 oldclearlines;

void HU_Erase(void)
{
	INT32 topline, bottomline;
	INT32 y, yoffset;

#ifdef HWRENDER
	// clear hud msgs on double buffer (OpenGL mode)
	boolean secondframe;
	static INT32 secondframelines;
#endif

	if (con_clearlines == oldclearlines && !con_hudupdate && !chat_on)
		return;

#ifdef HWRENDER
	// clear the other frame in double-buffer modes
	secondframe = (con_clearlines != oldclearlines);
	if (secondframe)
		secondframelines = oldclearlines;
#endif

	// clear the message lines that go away, so use _oldclearlines_
	bottomline = oldclearlines;
	oldclearlines = con_clearlines;
	if (chat_on && OLDCHAT)
		if (bottomline < 8)
			bottomline = 8;	// only do it for consolechat. consolechat is gay.

	if (automapactive || viewwindowx == 0) // hud msgs don't need to be cleared
		return;

	// software mode copies view border pattern & beveled edges from the backbuffer
	if (rendermode == render_soft)
	{
		topline = 0;
		for (y = topline, yoffset = y*vid.width; y < bottomline; y++, yoffset += vid.width)
		{
			if (y < viewwindowy || y >= viewwindowy + viewheight)
				R_VideoErase(yoffset, vid.width); // erase entire line
			else
			{
				R_VideoErase(yoffset, viewwindowx); // erase left border
				// erase right border
				R_VideoErase(yoffset + viewwindowx + viewwidth, viewwindowx);
			}
		}
		con_hudupdate = false; // if it was set..
	}
#ifdef HWRENDER
	else if (rendermode != render_none)
	{
		// refresh just what is needed from the view borders
		HWR_DrawViewBorder(secondframelines);
		con_hudupdate = secondframe;
	}
#endif
}

//======================================================================
//                   IN-LEVEL MULTIPLAYER RANKINGS
//======================================================================

//
// HU_drawPing
//
void HU_drawPing(INT32 x, INT32 y, INT32 ping, boolean notext)
{
	UINT8 numbars = 1;		// how many ping bars do we draw?
	UINT8 barcolor = 128;	// color we use for the bars (green, yellow or red)
	SINT8 i = 0;
	SINT8 yoffset = 6;
	if (ping < 128)
	{	
		numbars = 3;
		barcolor = 184;
	}	
	else if (ping < 256)
	{	
		numbars = 2;	// Apparently ternaries w/ multiple statements don't look good in C so I decided against it.
		barcolor = 103;
	}	
	
	INT32 dx = x+1 - (V_SmallStringWidth(va("%dms", ping), V_ALLOWLOWERCASE)/2);
	if (!notext || vid.width >= 640)	// how sad, we're using a shit resolution.
		V_DrawSmallString(dx, y+4, V_ALLOWLOWERCASE, va("%dms", ping));
	
	for (i=0; (i<3); i++)		// Draw the ping bar
	{	
		V_DrawFill(x+2 *(i-1), y+yoffset-4, 2, 8-yoffset, 31);
		if (i < numbars)
			V_DrawFill(x+2 *(i-1), y+yoffset-3, 1, 8-yoffset-1, barcolor);
		
		yoffset -= 2;
	}
}	

//
// HU_DrawTabRankings
//
void HU_DrawTabRankings(INT32 x, INT32 y, playersort_t *tab, INT32 scorelines, INT32 whiteplayer)
{
	INT32 i;
	const UINT8 *colormap;

	//this function is designed for 9 or less score lines only
	I_Assert(scorelines <= 9);

	V_DrawFill(1, 26, 318, 1, 0); //Draw a horizontal line because it looks nice!

	for (i = 0; i < scorelines; i++)
	{
		if (players[tab[i].num].spectator)
			continue; //ignore them.
		
		if (!splitscreen)	// don't draw it on splitscreen,
		{
			if (!(tab[i].num == serverplayer))
				HU_drawPing(x+ 253, y+2, playerpingtable[tab[i].num], false);
			//else
			//	V_DrawSmallString(x+ 246, y+4, V_YELLOWMAP, "SERVER"); 
		}	
		
		V_DrawString(x + 20, y,
		             ((tab[i].num == whiteplayer) ? V_YELLOWMAP : 0)
		             | ((players[tab[i].num].health > 0) ? 0 : V_60TRANS)
		             | V_ALLOWLOWERCASE, tab[i].name);

		// Draw emeralds
		if (!players[tab[i].num].powers[pw_super]
			|| ((leveltime/7) & 1))
		{
			HU_DrawEmeralds(x-12,y+2,tab[i].emeralds);
		}

		if (players[tab[i].num].health <= 0)
			V_DrawSmallTranslucentPatch (x, y-4, V_80TRANS, livesback);
		else
			V_DrawSmallScaledPatch (x, y-4, 0, livesback);

		if (tab[i].color == 0)
		{
			colormap = colormaps;
			if (players[tab[i].num].powers[pw_super])
				V_DrawSmallScaledPatch(x, y-4, 0, superprefix[players[tab[i].num].skin]);
			else
			{
				if (players[tab[i].num].health <= 0)
					V_DrawSmallTranslucentPatch(x, y-4, V_80TRANS, faceprefix[players[tab[i].num].skin]);
				else
					V_DrawSmallScaledPatch(x, y-4, 0, faceprefix[players[tab[i].num].skin]);
			}
		}
		else
		{
			if (players[tab[i].num].powers[pw_super])
			{
				colormap = R_GetTranslationColormap(players[tab[i].num].skin, players[tab[i].num].mo ? players[tab[i].num].mo->color : tab[i].color, GTC_CACHE);
				V_DrawSmallMappedPatch (x, y-4, 0, superprefix[players[tab[i].num].skin], colormap);
			}
			else
			{
				colormap = R_GetTranslationColormap(players[tab[i].num].skin, players[tab[i].num].mo ? players[tab[i].num].mo->color : tab[i].color, GTC_CACHE);
				if (players[tab[i].num].health <= 0)
					V_DrawSmallTranslucentMappedPatch (x, y-4, V_80TRANS, faceprefix[players[tab[i].num].skin], colormap);
				else
					V_DrawSmallMappedPatch (x, y-4, 0, faceprefix[players[tab[i].num].skin], colormap);
			}
		}

		if (G_GametypeUsesLives()) //show lives
			V_DrawRightAlignedString(x, y+4, V_ALLOWLOWERCASE|((players[tab[i].num].health > 0) ? 0 : V_60TRANS), va("%dx", players[tab[i].num].lives));
		else if (G_TagGametype() && players[tab[i].num].pflags & PF_TAGIT)
		{
			if (players[tab[i].num].health <= 0)
				V_DrawSmallTranslucentPatch(x-32, y-4, V_60TRANS, tagico);
			else
				V_DrawSmallScaledPatch(x-32, y-4, 0, tagico);
		}

		if (gametype == GT_RACE)
		{
			if (circuitmap)
			{
				if (players[tab[i].num].exiting)
					V_DrawRightAlignedString(x+240, y, 0, va("%i:%02i.%02i", G_TicsToMinutes(players[tab[i].num].realtime,true), G_TicsToSeconds(players[tab[i].num].realtime), G_TicsToCentiseconds(players[tab[i].num].realtime)));
				else
					V_DrawRightAlignedString(x+240, y, ((players[tab[i].num].health > 0) ? 0 : V_60TRANS), va("%u", tab[i].count));
			}
			else
				V_DrawRightAlignedString(x+240, y, ((players[tab[i].num].health > 0) ? 0 : V_60TRANS), va("%i:%02i.%02i", G_TicsToMinutes(tab[i].count,true), G_TicsToSeconds(tab[i].count), G_TicsToCentiseconds(tab[i].count)));
		}
		else
			V_DrawRightAlignedString(x+240, y, ((players[tab[i].num].health > 0) ? 0 : V_60TRANS), va("%u", tab[i].count));

		y += 16;
	}
}

//
// HU_Draw32Emeralds
//
static void HU_Draw32Emeralds(INT32 x, INT32 y, INT32 pemeralds)
{
	//Draw the emeralds, in the CORRECT order, using tiny emerald sprites.
	if (pemeralds & EMERALD1)
		V_DrawSmallScaledPatch(x  , y, 0, tinyemeraldpics[0]);

	if (pemeralds & EMERALD2)
		V_DrawSmallScaledPatch(x+4, y, 0, tinyemeraldpics[1]);

	if (pemeralds & EMERALD3)
		V_DrawSmallScaledPatch(x+8, y, 0, tinyemeraldpics[2]);

	if (pemeralds & EMERALD4)
		V_DrawSmallScaledPatch(x+12  , y, 0, tinyemeraldpics[3]);

	if (pemeralds & EMERALD5)
		V_DrawSmallScaledPatch(x+16, y, 0, tinyemeraldpics[4]);

	if (pemeralds & EMERALD6)
		V_DrawSmallScaledPatch(x+20, y, 0, tinyemeraldpics[5]);

	if (pemeralds & EMERALD7)
		V_DrawSmallScaledPatch(x+24,   y,   0, tinyemeraldpics[6]);
}

//
// HU_Draw32TeamTabRankings
//
static void HU_Draw32TeamTabRankings(playersort_t *tab, INT32 whiteplayer)
{
	INT32 i,x,y;
	INT32 redplayers = 0, blueplayers = 0;
	const UINT8 *colormap;
	char name[MAXPLAYERNAME+1];

	V_DrawFill(160, 26, 1, 154, 0); //Draw a vertical line to separate the two teams.
	V_DrawFill(1, 26, 318, 1, 0); //And a horizontal line to make a T.
	V_DrawFill(1, 180, 318, 1, 0); //And a horizontal line near the bottom.
	
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (players[tab[i].num].spectator)
			continue; //ignore them.

		if (tab[i].color == skincolor_redteam) //red
		{
			redplayers++;
			x = 14 + (BASEVIDWIDTH/2);
			y = (redplayers * 9) + 20;
		}
		else if (tab[i].color == skincolor_blueteam) //blue
		{
			blueplayers++;
			x = 14;
			y = (blueplayers * 9) + 20;
		}
		else //er?  not on red or blue, so ignore them
			continue;

		strlcpy(name, tab[i].name, 8);
		V_DrawString(x + 10, y,
		             ((tab[i].num == whiteplayer) ? V_YELLOWMAP : 0)
		             | ((players[tab[i].num].health > 0) ? 0 : V_TRANSLUCENT)
		             | V_ALLOWLOWERCASE, name);

		if (gametype == GT_CTF)
		{
			if (players[tab[i].num].gotflag & GF_REDFLAG) // Red
				V_DrawFixedPatch((x-10)*FRACUNIT, (y)*FRACUNIT, FRACUNIT/4, 0, rflagico, 0);
			else if (players[tab[i].num].gotflag & GF_BLUEFLAG) // Blue
				V_DrawFixedPatch((x-10)*FRACUNIT, (y)*FRACUNIT, FRACUNIT/4, 0, bflagico, 0);
		}

		// Draw emeralds
		if (!players[tab[i].num].powers[pw_super]
			|| ((leveltime/7) & 1))
		{
			HU_Draw32Emeralds(x+60, y+2, tab[i].emeralds);
		}

		if (players[tab[i].num].powers[pw_super])
		{
			colormap = R_GetTranslationColormap(players[tab[i].num].skin, players[tab[i].num].mo ? players[tab[i].num].mo->color : tab[i].color, GTC_CACHE);
			V_DrawFixedPatch(x*FRACUNIT, y*FRACUNIT, FRACUNIT/4, 0, superprefix[players[tab[i].num].skin], colormap);
		}
		else
		{
			colormap = R_GetTranslationColormap(players[tab[i].num].skin, players[tab[i].num].mo ? players[tab[i].num].mo->color : tab[i].color, GTC_CACHE);
			if (players[tab[i].num].health <= 0)
				V_DrawFixedPatch(x*FRACUNIT, y*FRACUNIT, FRACUNIT/4, V_HUDTRANSHALF, faceprefix[players[tab[i].num].skin], colormap);
			else
				V_DrawFixedPatch(x*FRACUNIT, y*FRACUNIT, FRACUNIT/4, 0, faceprefix[players[tab[i].num].skin], colormap);
		}
		V_DrawRightAlignedThinString(x+128, y, ((players[tab[i].num].health > 0) ? 0 : V_TRANSLUCENT), va("%u", tab[i].count));
		if (!splitscreen)
		{	
			if (!(tab[i].num == serverplayer))
				HU_drawPing(x+ 135, y+3, playerpingtable[tab[i].num], true);
		//else
			//V_DrawSmallString(x+ 129, y+4, V_YELLOWMAP, "HOST"); 
		}
	}
}

//
// HU_DrawTeamTabRankings
//
void HU_DrawTeamTabRankings(playersort_t *tab, INT32 whiteplayer)
{
	INT32 i,x,y;
	INT32 redplayers = 0, blueplayers = 0;
	boolean smol = false;

	// before we draw, we must count how many players are in each team. It makes an additional loop, but we need to know if we have to draw a big or a small ranking.
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (players[tab[i].num].spectator)
			continue; //ignore them.

		if (tab[i].color == skincolor_redteam) //red
		{
			if (redplayers++ > 8)
			{
				smol = true;
				break;	// don't make more loops than we need to.
			}	
		}
		else if (tab[i].color == skincolor_blueteam) //blue
		{
			if (blueplayers++ > 8)
			{	
				smol = true;
				break;
			}	
		}
		else //er?  not on red or blue, so ignore them
			continue;
		
	}
	
	// I'll be blunt with you, this may add more lines, but I'm not adding weird cases for this, so we're executing a separate function.
	if (smol == true || cv_compactscoreboard.value)
	{	
		HU_Draw32TeamTabRankings(tab, whiteplayer);
		return;
	}	
	
	V_DrawFill(160, 26, 1, 154, 0); //Draw a vertical line to separate the two teams.
	V_DrawFill(1, 26, 318, 1, 0); //And a horizontal line to make a T.
	V_DrawFill(1, 180, 318, 1, 0); //And a horizontal line near the bottom.
	
	const UINT8 *colormap;
	char name[MAXPLAYERNAME+1];
	
	i=0, redplayers=0, blueplayers=0;
	
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (players[tab[i].num].spectator)
			continue; //ignore them.

		if (tab[i].color == skincolor_redteam) //red
		{
			if (redplayers++ > 8)
				continue;
			x = 32 + (BASEVIDWIDTH/2);
			y = (redplayers * 16) + 16;
		}
		else if (tab[i].color == skincolor_blueteam) //blue
		{
			if (blueplayers++ > 8)
				continue;
			x = 32;
			y = (blueplayers * 16) + 16;
		}
		else //er?  not on red or blue, so ignore them
			continue;

		strlcpy(name, tab[i].name, 7);
		V_DrawString(x + 20, y,
		             ((tab[i].num == whiteplayer) ? V_YELLOWMAP : 0)
		             | ((players[tab[i].num].health > 0) ? 0 : V_TRANSLUCENT)
		             | V_ALLOWLOWERCASE, name);

		if (gametype == GT_CTF)
		{
			if (players[tab[i].num].gotflag & GF_REDFLAG) // Red
				V_DrawSmallScaledPatch(x-28, y-4, 0, rflagico);
			else if (players[tab[i].num].gotflag & GF_BLUEFLAG) // Blue
				V_DrawSmallScaledPatch(x-28, y-4, 0, bflagico);
		}

		// Draw emeralds
		if (!players[tab[i].num].powers[pw_super]
			|| ((leveltime/7) & 1))
		{
			HU_DrawEmeralds(x-12,y+2,tab[i].emeralds);
		}

		if (players[tab[i].num].powers[pw_super])
		{
			colormap = R_GetTranslationColormap(players[tab[i].num].skin, players[tab[i].num].mo ? players[tab[i].num].mo->color : tab[i].color, GTC_CACHE);
			V_DrawSmallMappedPatch (x, y-4, 0, superprefix[players[tab[i].num].skin], colormap);
		}
		else
		{
			colormap = R_GetTranslationColormap(players[tab[i].num].skin, players[tab[i].num].mo ? players[tab[i].num].mo->color : tab[i].color, GTC_CACHE);
			if (players[tab[i].num].health <= 0)
				V_DrawSmallTranslucentMappedPatch (x, y-4, 0, faceprefix[players[tab[i].num].skin], colormap);
			else
				V_DrawSmallMappedPatch (x, y-4, 0, faceprefix[players[tab[i].num].skin], colormap);
		}
		V_DrawRightAlignedThinString(x+100, y, ((players[tab[i].num].health > 0) ? 0 : V_TRANSLUCENT), va("%u", tab[i].count));
		if (!splitscreen)
		{	
			if (!(tab[i].num == serverplayer))
				HU_drawPing(x+ 113, y+2, playerpingtable[tab[i].num], false);
		//else
		//	V_DrawSmallString(x+ 94, y+4, V_YELLOWMAP, "SERVER"); 
		}
	}
}

//
// HU_DrawDualTabRankings
//
void HU_DrawDualTabRankings(INT32 x, INT32 y, playersort_t *tab, INT32 scorelines, INT32 whiteplayer)
{
	INT32 i;
	const UINT8 *colormap;
	char name[MAXPLAYERNAME+1];

	V_DrawFill(160, 26, 1, 154, 0); //Draw a vertical line to separate the two sides.
	V_DrawFill(1, 26, 318, 1, 0); //And a horizontal line to make a T.
	V_DrawFill(1, 180, 318, 1, 0); //And a horizontal line near the bottom.

	for (i = 0; i < scorelines; i++)
	{
		if (players[tab[i].num].spectator)
			continue; //ignore them.

		strlcpy(name, tab[i].name, 7);
		if (!(tab[i].num == serverplayer))
			HU_drawPing(x+ 113, y+2, playerpingtable[tab[i].num], false);
		//else
		//	V_DrawSmallString(x+ 94, y+4, V_YELLOWMAP, "SERVER"); 
		
		V_DrawString(x + 20, y,
		             ((tab[i].num == whiteplayer) ? V_YELLOWMAP : 0)
		             | ((players[tab[i].num].health > 0) ? 0 : V_TRANSLUCENT)
		             | V_ALLOWLOWERCASE, name);

		if (G_GametypeUsesLives()) //show lives
			V_DrawRightAlignedString(x, y+4, V_ALLOWLOWERCASE, va("%dx", players[tab[i].num].lives));
		else if (G_TagGametype() && players[tab[i].num].pflags & PF_TAGIT)
			V_DrawSmallScaledPatch(x-28, y-4, 0, tagico);

		// Draw emeralds
		if (!players[tab[i].num].powers[pw_super]
			|| ((leveltime/7) & 1))
		{
			HU_DrawEmeralds(x-12,y+2,tab[i].emeralds);
		}

		//V_DrawSmallScaledPatch (x, y-4, 0, livesback);
		if (tab[i].color == 0)
		{
			colormap = colormaps;
			if (players[tab[i].num].powers[pw_super])
				V_DrawSmallScaledPatch (x, y-4, 0, superprefix[players[tab[i].num].skin]);
			else
			{
				if (players[tab[i].num].health <= 0)
					V_DrawSmallTranslucentPatch (x, y-4, 0, faceprefix[players[tab[i].num].skin]);
				else
					V_DrawSmallScaledPatch (x, y-4, 0, faceprefix[players[tab[i].num].skin]);
			}
		}
		else
		{
			if (players[tab[i].num].powers[pw_super])
			{
				colormap = R_GetTranslationColormap(players[tab[i].num].skin, players[tab[i].num].mo ? players[tab[i].num].mo->color : tab[i].color, GTC_CACHE);
				V_DrawSmallMappedPatch (x, y-4, 0, superprefix[players[tab[i].num].skin], colormap);
			}
			else
			{
				colormap = R_GetTranslationColormap(players[tab[i].num].skin, players[tab[i].num].mo ? players[tab[i].num].mo->color : tab[i].color, GTC_CACHE);
				if (players[tab[i].num].health <= 0)
					V_DrawSmallTranslucentMappedPatch (x, y-4, 0, faceprefix[players[tab[i].num].skin], colormap);
				else
					V_DrawSmallMappedPatch (x, y-4, 0, faceprefix[players[tab[i].num].skin], colormap);
			}
		}

		// All data drawn with thin string for space.
		if (gametype == GT_RACE)
		{
			if (circuitmap)
			{
				if (players[tab[i].num].exiting)
					V_DrawRightAlignedThinString(x+146, y, 0, va("%i:%02i.%02i", G_TicsToMinutes(players[tab[i].num].realtime,true), G_TicsToSeconds(players[tab[i].num].realtime), G_TicsToCentiseconds(players[tab[i].num].realtime)));
				else
					V_DrawRightAlignedThinString(x+146, y, ((players[tab[i].num].health > 0) ? 0 : V_TRANSLUCENT), va("%u", tab[i].count));
			}
			else
				V_DrawRightAlignedThinString(x+146, y, ((players[tab[i].num].health > 0) ? 0 : V_TRANSLUCENT), va("%i:%02i.%02i", G_TicsToMinutes(tab[i].count,true), G_TicsToSeconds(tab[i].count), G_TicsToCentiseconds(tab[i].count)));
		}
		else
			V_DrawRightAlignedThinString(x+100, y, ((players[tab[i].num].health > 0) ? 0 : V_TRANSLUCENT), va("%u", tab[i].count));

		y += 16;
		if (y > 160)
		{
			y = 32;
			x += BASEVIDWIDTH/2;
		}
	}
}

//
// HU_Draw32TabRankings
//
void HU_Draw32TabRankings(INT32 x, INT32 y, playersort_t *tab, INT32 scorelines, INT32 whiteplayer)
{
	INT32 i;
	const UINT8 *colormap;
	char name[MAXPLAYERNAME+1];

	V_DrawFill(160, 26, 1, 154, 0); //Draw a vertical line to separate the two sides.
	V_DrawFill(1, 26, 318, 1, 0); //And a horizontal line to make a T.
	V_DrawFill(1, 180, 318, 1, 0); //And a horizontal line near the bottom.

	for (i = 0; i < scorelines; i++)
	{
		if (players[tab[i].num].spectator)
			continue; //ignore them.

		strlcpy(name, tab[i].name, 7);
		if (!splitscreen)	// don't draw it on splitscreen,
		{
			if (!(tab[i].num == serverplayer))
				HU_drawPing(x+ 135, y+3, playerpingtable[tab[i].num], true);
		//else
		//	V_DrawSmallString(x+ 129, y+4, V_YELLOWMAP, "HOST"); 
		}
		
		V_DrawString(x + 10, y,
		             ((tab[i].num == whiteplayer) ? V_YELLOWMAP : 0)
		             | ((players[tab[i].num].health > 0) ? 0 : V_TRANSLUCENT)
		             | V_ALLOWLOWERCASE, name);

		if (G_GametypeUsesLives()) //show lives
			V_DrawRightAlignedThinString(x-1, y, V_ALLOWLOWERCASE, va("%d", players[tab[i].num].lives));
		else if (G_TagGametype() && players[tab[i].num].pflags & PF_TAGIT)
			V_DrawFixedPatch((x-10)*FRACUNIT, (y)*FRACUNIT, FRACUNIT/4, 0, tagico, 0);

		// Draw emeralds
		if (!players[tab[i].num].powers[pw_super]
			|| ((leveltime/7) & 1))
		{
			HU_Draw32Emeralds(x+60, y+2, tab[i].emeralds);
			//HU_DrawEmeralds(x-12,y+2,tab[i].emeralds);
		}

		//V_DrawSmallScaledPatch (x, y-4, 0, livesback);
		if (tab[i].color == 0)
		{
			colormap = colormaps;
			if (players[tab[i].num].powers[pw_super])
				V_DrawFixedPatch(x*FRACUNIT, y*FRACUNIT, FRACUNIT/4, 0, superprefix[players[tab[i].num].skin], 0);
			else
			{
				if (players[tab[i].num].health <= 0)
					V_DrawFixedPatch(x*FRACUNIT, (y)*FRACUNIT, FRACUNIT/4, V_HUDTRANSHALF, faceprefix[players[tab[i].num].skin], 0);
				else
					V_DrawFixedPatch(x*FRACUNIT, (y)*FRACUNIT, FRACUNIT/4, 0, faceprefix[players[tab[i].num].skin], 0);
			}
		}
		else
		{
			if (players[tab[i].num].powers[pw_super])
			{
				colormap = R_GetTranslationColormap(players[tab[i].num].skin, players[tab[i].num].mo ? players[tab[i].num].mo->color : tab[i].color, GTC_CACHE);
				V_DrawFixedPatch(x*FRACUNIT, y*FRACUNIT, FRACUNIT/4, 0, superprefix[players[tab[i].num].skin], colormap);
			}
			else
			{
				colormap = R_GetTranslationColormap(players[tab[i].num].skin, players[tab[i].num].mo ? players[tab[i].num].mo->color : tab[i].color, GTC_CACHE);
				if (players[tab[i].num].health <= 0)
					V_DrawFixedPatch(x*FRACUNIT, (y)*FRACUNIT, FRACUNIT/4, V_HUDTRANSHALF, faceprefix[players[tab[i].num].skin], colormap);
				else
					V_DrawFixedPatch(x*FRACUNIT, (y)*FRACUNIT, FRACUNIT/4, 0, faceprefix[players[tab[i].num].skin], colormap);
			}
		}

		// All data drawn with thin string for space.
		if (gametype == GT_RACE)
		{
			if (circuitmap)
			{
				if (players[tab[i].num].exiting)
					V_DrawRightAlignedThinString(x+128, y, 0, va("%i:%02i.%02i", G_TicsToMinutes(players[tab[i].num].realtime,true), G_TicsToSeconds(players[tab[i].num].realtime), G_TicsToCentiseconds(players[tab[i].num].realtime)));
				else
					V_DrawRightAlignedThinString(x+128, y, ((players[tab[i].num].health > 0) ? 0 : V_TRANSLUCENT), va("%u", tab[i].count));
			}
			else
				V_DrawRightAlignedThinString(x+128, y, ((players[tab[i].num].health > 0) ? 0 : V_TRANSLUCENT), va("%i:%02i.%02i", G_TicsToMinutes(tab[i].count,true), G_TicsToSeconds(tab[i].count), G_TicsToCentiseconds(tab[i].count)));
		}
		else
			V_DrawRightAlignedThinString(x+128, y, ((players[tab[i].num].health > 0) ? 0 : V_TRANSLUCENT), va("%u", tab[i].count));

		y += 9;
		if (i == 16)
		{
			y = 32;
			x += BASEVIDWIDTH/2;
		}
	}
}

//
// HU_DrawEmeralds
//
void HU_DrawEmeralds(INT32 x, INT32 y, INT32 pemeralds)
{
	//Draw the emeralds, in the CORRECT order, using tiny emerald sprites.
	if (pemeralds & EMERALD1)
		V_DrawSmallScaledPatch(x  , y-6, 0, tinyemeraldpics[0]);

	if (pemeralds & EMERALD2)
		V_DrawSmallScaledPatch(x+4, y-3, 0, tinyemeraldpics[1]);

	if (pemeralds & EMERALD3)
		V_DrawSmallScaledPatch(x+4, y+3, 0, tinyemeraldpics[2]);

	if (pemeralds & EMERALD4)
		V_DrawSmallScaledPatch(x  , y+6, 0, tinyemeraldpics[3]);

	if (pemeralds & EMERALD5)
		V_DrawSmallScaledPatch(x-4, y+3, 0, tinyemeraldpics[4]);

	if (pemeralds & EMERALD6)
		V_DrawSmallScaledPatch(x-4, y-3, 0, tinyemeraldpics[5]);

	if (pemeralds & EMERALD7)
		V_DrawSmallScaledPatch(x,   y,   0, tinyemeraldpics[6]);
}

//
// HU_DrawSpectatorTicker
//
static inline void HU_DrawSpectatorTicker(void)
{
	int i;
	int length = 0, height = 174;
	int totallength = 0, templength = 0;

	for (i = 0; i < MAXPLAYERS; i++)
		if (playeringame[i] && players[i].spectator)
			totallength += (signed)strlen(player_names[i]) * 8 + 16;

	length -= (leveltime % (totallength + BASEVIDWIDTH));
	length += BASEVIDWIDTH;

	for (i = 0; i < MAXPLAYERS; i++)
		if (playeringame[i] && players[i].spectator)
		{
			char *pos;
			char initial[MAXPLAYERNAME+1];
			char current[MAXPLAYERNAME+1];

			strcpy(initial, player_names[i]);
			pos = initial;

			if (length >= -((signed)strlen(player_names[i]) * 8 + 16) && length <= BASEVIDWIDTH)
			{
				if (length < 0)
				{
					UINT8 eatenchars = (UINT8)(abs(length) / 8 + 1);

					if (eatenchars <= strlen(initial))
					{
						// Eat one letter off the left side,
						// then compensate the drawing position.
						pos += eatenchars;
						strcpy(current, pos);
						templength = length % 8 + 8;
					}
					else
					{
						strcpy(current, " ");
						templength = length;
					}
				}
				else
				{
					strcpy(current, initial);
					templength = length;
				}

				V_DrawString(templength, height + 8, V_TRANSLUCENT, current);
			}

			length += (signed)strlen(player_names[i]) * 8 + 16;
		}
}

//
// HU_DrawRankings
//
static void HU_DrawRankings(void)
{
	patch_t *p;
	playersort_t tab[MAXPLAYERS];
	INT32 i, j, scorelines;
	boolean completed[MAXPLAYERS];
	UINT32 whiteplayer;

	// draw the current gametype in the lower right
	HU_drawGametype();

	if (G_GametypeHasTeams())
	{
		if (gametype == GT_CTF)
			p = bflagico;
		else
			p = bmatcico;

		V_DrawSmallScaledPatch(128 - SHORT(p->width)/4, 4, 0, p);
		V_DrawCenteredString(128, 16, 0, va("%u", bluescore));

		if (gametype == GT_CTF)
			p = rflagico;
		else
			p = rmatcico;

		V_DrawSmallScaledPatch(192 - SHORT(p->width)/4, 4, 0, p);
		V_DrawCenteredString(192, 16, 0, va("%u", redscore));
	}

	if (gametype != GT_RACE && gametype != GT_COMPETITION && gametype != GT_COOP)
	{
		if (cv_timelimit.value && timelimitintics > 0)
		{
			INT32 timeval = (timelimitintics+1-leveltime)/TICRATE;

			if (leveltime <= timelimitintics)
			{
				V_DrawCenteredString(64, 8, 0, "TIME LEFT");
				V_DrawCenteredString(64, 16, 0, va("%u", timeval));
			}

			// overtime
			if ((leveltime > (timelimitintics + TICRATE/2)) && cv_overtime.value)
			{
				V_DrawCenteredString(64, 8, 0, "TIME LEFT");
				V_DrawCenteredString(64, 16, 0, "OVERTIME");
			}
		}

		if (cv_pointlimit.value > 0)
		{
			V_DrawCenteredString(256, 8, 0, "POINT LIMIT");
			V_DrawCenteredString(256, 16, 0, va("%d", cv_pointlimit.value));
		}
	}
	else if (gametype == GT_COOP)
	{
		INT32 totalscore = 0;
		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (playeringame[i])
				totalscore += players[i].score;
		}

		V_DrawCenteredString(256, 8, 0, "TOTAL SCORE");
		V_DrawCenteredString(256, 16, 0, va("%u", totalscore));
	}
	else
	{
		if (circuitmap)
		{
			V_DrawCenteredString(64, 8, 0, "NUMBER OF LAPS");
			V_DrawCenteredString(64, 16, 0, va("%d", cv_numlaps.value));
		}
	}

	// When you play, you quickly see your score because your name is displayed in white.
	// When playing back a demo, you quickly see who's the view.
	whiteplayer = demoplayback ? displayplayer : consoleplayer;

	scorelines = 0;
	memset(completed, 0, sizeof (completed));
	memset(tab, 0, sizeof (playersort_t)*MAXPLAYERS);

	for (i = 0; i < MAXPLAYERS; i++)
	{
		tab[i].num = -1;
		tab[i].name = 0;

		if (gametype == GT_RACE && !circuitmap)
			tab[i].count = INT32_MAX;
	}

	for (j = 0; j < MAXPLAYERS; j++)
	{
		if (!playeringame[j] || players[j].spectator)
			continue;

		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (playeringame[i] && !players[i].spectator)
			{
				if (gametype == GT_RACE)
				{
					if (circuitmap)
					{
						if ((unsigned)players[i].laps+1 >= tab[scorelines].count && completed[i] == false)
						{
							tab[scorelines].count = players[i].laps+1;
							tab[scorelines].num = i;
							tab[scorelines].color = players[i].skincolor;
							tab[scorelines].name = player_names[i];
						}
					}
					else
					{
						if (players[i].realtime <= tab[scorelines].count && completed[i] == false)
						{
							tab[scorelines].count = players[i].realtime;
							tab[scorelines].num = i;
							tab[scorelines].color = players[i].skincolor;
							tab[scorelines].name = player_names[i];
						}
					}
				}
				else if (gametype == GT_COMPETITION)
				{
					// todo put something more fitting for the gametype here, such as current
					// number of categories led
					if (players[i].score >= tab[scorelines].count && completed[i] == false)
					{
						tab[scorelines].count = players[i].score;
						tab[scorelines].num = i;
						tab[scorelines].color = players[i].skincolor;
						tab[scorelines].name = player_names[i];
						tab[scorelines].emeralds = players[i].powers[pw_emeralds];
					}
				}
				else
				{
					if (players[i].score >= tab[scorelines].count && completed[i] == false)
					{
						tab[scorelines].count = players[i].score;
						tab[scorelines].num = i;
						tab[scorelines].color = players[i].skincolor;
						tab[scorelines].name = player_names[i];
						tab[scorelines].emeralds = players[i].powers[pw_emeralds];
					}
				}
			}
		}
		completed[tab[scorelines].num] = true;
		scorelines++;
	}

	//if (scorelines > 20)
	//	scorelines = 20; //dont draw past bottom of screen, show the best only
	// shush, we'll do it anyway.

	if (G_GametypeHasTeams())
		HU_DrawTeamTabRankings(tab, whiteplayer); //separate function for Spazzo's silly request
	else if (scorelines <= 9 && !cv_compactscoreboard.value)
		HU_DrawTabRankings(40, 32, tab, scorelines, whiteplayer);
	else if (scorelines <= 20 && !cv_compactscoreboard.value)
		HU_DrawDualTabRankings(32, 32, tab, scorelines, whiteplayer);
	else
		HU_Draw32TabRankings(14, 28, tab, scorelines, whiteplayer);

	// draw spectators in a ticker across the bottom
	if (!splitscreen && G_GametypeHasSpectators())
		HU_DrawSpectatorTicker();
}

static void HU_DrawCoopOverlay(void)
{
	if (token
#ifdef HAVE_BLUA
	&& LUA_HudEnabled(hud_tokens)
#endif
	)
	{
		V_DrawString(168, 176, 0, va("- %d", token));
		V_DrawSmallScaledPatch(148, 172, 0, tokenicon);
	}

#ifdef HAVE_BLUA
	if (LUA_HudEnabled(hud_tabemblems))
#endif
	if (!modifiedgame || savemoddata)
	{
		V_DrawString(160, 144, 0, va("- %d/%d", M_CountEmblems(), numemblems+numextraemblems));
		V_DrawScaledPatch(128, 144 - SHORT(emblemicon->height)/4, 0, emblemicon);
	}

#ifdef HAVE_BLUA
	if (!LUA_HudEnabled(hud_coopemeralds))
		return;
#endif

	if (emeralds & EMERALD1)
		V_DrawScaledPatch((BASEVIDWIDTH/2)-8   , (BASEVIDHEIGHT/3)-32, 0, emeraldpics[0]);
	if (emeralds & EMERALD2)
		V_DrawScaledPatch((BASEVIDWIDTH/2)-8+24, (BASEVIDHEIGHT/3)-16, 0, emeraldpics[1]);
	if (emeralds & EMERALD3)
		V_DrawScaledPatch((BASEVIDWIDTH/2)-8+24, (BASEVIDHEIGHT/3)+16, 0, emeraldpics[2]);
	if (emeralds & EMERALD4)
		V_DrawScaledPatch((BASEVIDWIDTH/2)-8   , (BASEVIDHEIGHT/3)+32, 0, emeraldpics[3]);
	if (emeralds & EMERALD5)
		V_DrawScaledPatch((BASEVIDWIDTH/2)-8-24, (BASEVIDHEIGHT/3)+16, 0, emeraldpics[4]);
	if (emeralds & EMERALD6)
		V_DrawScaledPatch((BASEVIDWIDTH/2)-8-24, (BASEVIDHEIGHT/3)-16, 0, emeraldpics[5]);
	if (emeralds & EMERALD7)
		V_DrawScaledPatch((BASEVIDWIDTH/2)-8   , (BASEVIDHEIGHT/3)   , 0, emeraldpics[6]);
}

static void HU_DrawNetplayCoopOverlay(void)
{
	int i;

#ifdef HAVE_BLUA
	if (!LUA_HudEnabled(hud_coopemeralds))
		return;
#endif

	for (i = 0; i < 7; ++i)
	{
		if (emeralds & (1 << i))
			V_DrawScaledPatch(20 + (i * 20), 6, 0, emeraldpics[i]);
	}
}


// Interface to CECHO settings for the outside world, avoiding the
// expense (and security problems) of going via the console buffer.
void HU_ClearCEcho(void)
{
	cechotimer = 0;
}

void HU_SetCEchoDuration(INT32 seconds)
{
	cechoduration = seconds * TICRATE;
}

void HU_SetCEchoFlags(INT32 flags)
{
	// Don't allow cechoflags to contain any bits in V_PARAMMASK
	cechoflags = (flags & ~V_PARAMMASK);
}

void HU_DoCEcho(const char *msg)
{
	I_OutputMsg("%s\n", msg); // print to log

	strncpy(cechotext, msg, sizeof(cechotext));
	strncat(cechotext, "\\", sizeof(cechotext) - strlen(cechotext) - 1);
	cechotext[sizeof(cechotext) - 1] = '\0';
	cechotimer = cechoduration;
}

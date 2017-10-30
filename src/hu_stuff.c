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
static boolean headsupactive = false;
boolean hu_showscores; // draw rankings
static char hu_tick;

patch_t *rflagico;
patch_t *bflagico;
patch_t *rmatcico;
patch_t *bmatcico;
patch_t *tagico;
patch_t *tallminus;
patch_t *tallinfin;

//-------------------------------------------
//              coop hud
//-------------------------------------------

patch_t *emeraldpics[7];
patch_t *tinyemeraldpics[7];
static patch_t *emblemicon;
patch_t *tokenicon;
static patch_t *exiticon;

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
	tallinfin = (patch_t *)W_CachePatchName("STTINFIN", PU_HUDGFX);

	// cache the crosshairs, don't bother to know which one is being used,
	// just cache all 3, they're so small anyway.
	for (i = 0; i < HU_CROSSHAIRS; i++)
	{
		sprintf(buffer, "CROSHAI%c", '1'+i);
		crosshair[i] = (patch_t *)W_CachePatchName(buffer, PU_HUDGFX);
	}

	emblemicon = W_CachePatchName("EMBLICON", PU_HUDGFX);
	tokenicon = W_CachePatchName("TOKNICON", PU_HUDGFX);
	exiticon = W_CachePatchName("EXITICON", PU_HUDGFX);

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
	char buf[254];
	size_t numwords, ix;
	char *msg = &buf[2];
	const size_t msgspace = sizeof buf - 2;

	numwords = COM_Argc() - usedargs;
	I_Assert(numwords > 0);

	if (cv_mute.value && !(server || adminplayer == consoleplayer))
	{
		CONS_Alert(CONS_NOTICE, M_GetText("The chat is muted. You can't say anything at the moment.\n"));
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
			UINT8 buf[2];

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
					char buf[2];

					buf[0] = (char)playernum;
					buf[1] = KICK_MSG_CON_FAIL;
					SendNetXCmd(XD_KICK, &buf, 2);
				}
				return;
			}
		}
	}

#ifdef HAVE_BLUA
	if (LUAh_PlayerMsg(playernum, target, flags, msg))
		return;
#endif

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
		const char *cstart = "", *cend = "", *adminchar = "~", *remotechar = "@", *fmt;
		char *tempchar = NULL;

		// In CTF and team match, color the player's name.
		if (G_GametypeHasTeams())
		{
			cend = "\x80";
			if (players[playernum].ctfteam == 1) // red
				cstart = "\x85";
			else if (players[playernum].ctfteam == 2) // blue
				cstart = "\x84";
		}

		// Give admins and remote admins their symbols.
		if (playernum == serverplayer)
			tempchar = (char *)Z_Calloc(strlen(cstart) + strlen(adminchar) + 1, PU_STATIC, NULL);
		else if (playernum == adminplayer)
			tempchar = (char *)Z_Calloc(strlen(cstart) + strlen(remotechar) + 1, PU_STATIC, NULL);
		if (tempchar)
		{
			strcat(tempchar, cstart);
			if (playernum == serverplayer)
				strcat(tempchar, adminchar);
			else
				strcat(tempchar, remotechar);
			cstart = tempchar;
		}

		// Choose the proper format string for display.
		// Each format includes four strings: color start, display
		// name, color end, and the message itself.
		// '\4' makes the message yellow and beeps; '\3' just beeps.
		if (action)
			fmt = "\4* %s%s%s \x82%s\n";
		else if (target == 0) // To everyone
			fmt = "\3<%s%s%s> %s\n";
		else if (target-1 == consoleplayer) // To you
			fmt = "\3*%s%s%s* %s\n";
		else if (target > 0) // By you, to another player
		{
			// Use target's name.
			dispname = player_names[target-1];
			fmt = "\3->*%s%s%s* %s\n";
		}
		else // To your team
			fmt = "\3>>%s%s%s<< (team) %s\n";

		CONS_Printf(fmt, cstart, dispname, cend, msg);
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
			s[l++] = ch;
			s[l]=0;
			return true;
		}
		return false;
	}
	else if (ch == KEY_BACKSPACE)
	{
		l = strlen(s);
		if (l)
			s[--l] = 0;
		else
			return false;
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

#define QUEUESIZE 256

static boolean teamtalk = false;
static char chatchars[QUEUESIZE];
static INT32 head = 0, tail = 0;

//
// HU_dequeueChatChar
//
char HU_dequeueChatChar(void)
{
	char c;

	if (head != tail)
	{
		c = chatchars[tail];
		tail = (tail + 1) & (QUEUESIZE-1);
	}
	else
		c = 0;

	return c;
}

//
//
static void HU_queueChatChar(char c)
{
	// send automaticly the message (no more chat char)
	if (c == KEY_ENTER)
	{
		char buf[2+256];
		size_t ci = 2;

		do {
			c = HU_dequeueChatChar();
			if (!c || (c >= ' ' && !(c & 0x80))) // copy printable characters and terminating '\0' only.
				buf[ci++]=c;
		} while (c);

		// last minute mute check
		if (cv_mute.value && !(server || adminplayer == consoleplayer))
		{
			CONS_Alert(CONS_NOTICE, M_GetText("The chat is muted. You can't say anything at the moment.\n"));
			return;
		}

		if (ci > 3) // don't send target+flags+empty message.
		{
			if (teamtalk)
				buf[0] = -1; // target
			else
				buf[0] = 0; // target
			buf[1] = 0; // flags
			SendNetXCmd(XD_SAY, buf, 2 + strlen(&buf[2]) + 1);
		}
		return;
	}

	if (((head + 1) & (QUEUESIZE-1)) == tail)
		CONS_Printf(M_GetText("[Message unsent]\n")); // message not sent
	else
	{
		if (c == KEY_BACKSPACE)
		{
			if (tail != head)
				head = (head - 1) & (QUEUESIZE-1);
		}
		else
		{
			chatchars[head] = c;
			head = (head + 1) & (QUEUESIZE-1);
		}
	}
}

void HU_clearChatChars(void)
{
	while (tail != head)
		HU_queueChatChar(KEY_BACKSPACE);
	chat_on = false;
}

//
// Returns true if key eaten
//
boolean HU_Responder(event_t *ev)
{
	UINT8 c;

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

		// use console translations
		if (shiftdown)
			c = shiftxform[c];

		if (HU_keyInChatString(w_chat,c))
			HU_queueChatChar(c);
		if (c == KEY_ENTER)
			chat_on = false;
		else if (c == KEY_ESCAPE)
			chat_on = false;

		return true;
	}
	return false;
}

//======================================================================
//                         HEADS UP DRAWING
//======================================================================

//
// HU_DrawChat
//
// Draw chat input
//
static void HU_DrawChat(void)
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
			V_DrawCharacter(HU_INPUTX + c, y, talk[i++] | cv_constextsize.value | V_NOSCALESTART, true);
		}
		c += charwidth;
	}

	i = 0;
	while (w_chat[i])
	{
		//Hurdler: isn't it better like that?
		if (w_chat[i] < HU_FONTSTART)
		{
			++i;
			//charwidth = 4 * con_scalefactor;
		}
		else
		{
			//charwidth = SHORT(hu_font[w_chat[i]-HU_FONTSTART]->width) * con_scalefactor;
			V_DrawCharacter(HU_INPUTX + c, y, w_chat[i++] | cv_constextsize.value | V_NOSCALESTART | t, true);
		}

		c += charwidth;
		if (c >= vid.width)
		{
			c = 0;
			y += charheight;
		}
	}

	if (hu_tick < 4)
		V_DrawCharacter(HU_INPUTX + c, y, '_' | cv_constextsize.value |V_NOSCALESTART|t, true);
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
	const char *strvalue = NULL;
	
	if (gametype < 0 || gametype >= NUMGAMETYPES)
		return; // not a valid gametype???

	strvalue = Gametype_Names[gametype];
		
	if (splitscreen)
		V_DrawString(4, 184, 0, strvalue);
	else
		V_DrawString(4, 192, 0, strvalue);
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
		HU_DrawChat();

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
	if (chat_on)
		if (bottomline < 8)
			bottomline = 8;

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

#define supercheckdef ((players[tab[i].num].powers[pw_super] && players[tab[i].num].mo && (players[tab[i].num].mo->state < &states[S_PLAY_SUPER_TRANS1] || players[tab[i].num].mo->state >= &states[S_PLAY_SUPER_TRANS6])) || (players[tab[i].num].powers[pw_carry] == CR_NIGHTSMODE && skins[players[tab[i].num].skin].flags & SF_SUPER))
#define greycheckdef ((players[tab[i].num].mo && players[tab[i].num].mo->health <= 0) || players[tab[i].num].spectator)

//
// HU_DrawTabRankings
//
void HU_DrawTabRankings(INT32 x, INT32 y, playersort_t *tab, INT32 scorelines, INT32 whiteplayer)
{
	INT32 i;
	const UINT8 *colormap;
	boolean greycheck, supercheck;

	//this function is designed for 9 or less score lines only
	I_Assert(scorelines <= 9);

	V_DrawFill(1, 26, 318, 1, 0); //Draw a horizontal line because it looks nice!

	for (i = 0; i < scorelines; i++)
	{
		if (players[tab[i].num].spectator && gametype != GT_COOP)
			continue; //ignore them.

		greycheck = greycheckdef;
		supercheck = supercheckdef;

		V_DrawString(x + 20, y,
		             ((tab[i].num == whiteplayer) ? V_YELLOWMAP : 0)
		             | (greycheck ? V_60TRANS : 0)
		             | V_ALLOWLOWERCASE, tab[i].name);

		// Draw emeralds
		if (!players[tab[i].num].powers[pw_super]
			|| ((leveltime/7) & 1))
		{
			HU_DrawEmeralds(x-12,y+2,tab[i].emeralds);
		}

		if (greycheck)
			V_DrawSmallTranslucentPatch (x, y-4, V_80TRANS, livesback);
		else
			V_DrawSmallScaledPatch (x, y-4, 0, livesback);

		if (tab[i].color == 0)
		{
			colormap = colormaps;
			if (supercheck)
				V_DrawSmallScaledPatch(x, y-4, 0, superprefix[players[tab[i].num].skin]);
			else
			{
				if (greycheck)
					V_DrawSmallTranslucentPatch(x, y-4, V_80TRANS, faceprefix[players[tab[i].num].skin]);
				else
					V_DrawSmallScaledPatch(x, y-4, 0, faceprefix[players[tab[i].num].skin]);
			}
		}
		else
		{
			if (supercheck)
			{
				colormap = R_GetTranslationColormap(players[tab[i].num].skin, players[tab[i].num].mo->color, GTC_CACHE);
				V_DrawSmallMappedPatch (x, y-4, 0, superprefix[players[tab[i].num].skin], colormap);
			}
			else
			{
				colormap = R_GetTranslationColormap(players[tab[i].num].skin, players[tab[i].num].mo ? players[tab[i].num].mo->color : tab[i].color, GTC_CACHE);
				if (greycheck)
					V_DrawSmallTranslucentMappedPatch (x, y-4, V_80TRANS, faceprefix[players[tab[i].num].skin], colormap);
				else
					V_DrawSmallMappedPatch (x, y-4, 0, faceprefix[players[tab[i].num].skin], colormap);
			}
		}

		if (G_GametypeUsesLives() && !(gametype == GT_COOP && (cv_cooplives.value == 0 || cv_cooplives.value == 3)) && (players[tab[i].num].lives != 0x7f)) //show lives
			V_DrawRightAlignedString(x, y+4, V_ALLOWLOWERCASE|(greycheck ? V_60TRANS : 0), va("%dx", players[tab[i].num].lives));
		else if (G_TagGametype() && players[tab[i].num].pflags & PF_TAGIT)
		{
			if (greycheck)
				V_DrawSmallTranslucentPatch(x-32, y-4, V_60TRANS, tagico);
			else
				V_DrawSmallScaledPatch(x-32, y-4, 0, tagico);
		}

		if (players[tab[i].num].exiting)
			V_DrawSmallScaledPatch(x - SHORT(exiticon->width)/2 - 1, y-3, 0, exiticon);

		if (gametype == GT_RACE)
		{
			if (circuitmap)
			{
				if (players[tab[i].num].exiting)
					V_DrawRightAlignedString(x+240, y, 0, va("%i:%02i.%02i", G_TicsToMinutes(players[tab[i].num].realtime,true), G_TicsToSeconds(players[tab[i].num].realtime), G_TicsToCentiseconds(players[tab[i].num].realtime)));
				else
					V_DrawRightAlignedString(x+240, y, (greycheck ? V_60TRANS : 0), va("%u", tab[i].count));
			}
			else
				V_DrawRightAlignedString(x+240, y, (greycheck ? V_60TRANS : 0), va("%i:%02i.%02i", G_TicsToMinutes(tab[i].count,true), G_TicsToSeconds(tab[i].count), G_TicsToCentiseconds(tab[i].count)));
		}
		else
			V_DrawRightAlignedString(x+240, y, (greycheck ? V_60TRANS : 0), va("%u", tab[i].count));

		y += 16;
	}
}

//
// HU_DrawTeamTabRankings
//
void HU_DrawTeamTabRankings(playersort_t *tab, INT32 whiteplayer)
{
	INT32 i,x,y;
	INT32 redplayers = 0, blueplayers = 0;
	const UINT8 *colormap;
	char name[MAXPLAYERNAME+1];
	boolean greycheck, supercheck;

	V_DrawFill(160, 26, 1, 154, 0); //Draw a vertical line to separate the two teams.
	V_DrawFill(1, 26, 318, 1, 0); //And a horizontal line to make a T.
	V_DrawFill(1, 180, 318, 1, 0); //And a horizontal line near the bottom.

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

		greycheck = greycheckdef;
		supercheck = supercheckdef;

		strlcpy(name, tab[i].name, 9);
		V_DrawString(x + 20, y,
		             ((tab[i].num == whiteplayer) ? V_YELLOWMAP : 0)
		             | (greycheck ? V_TRANSLUCENT : 0)
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

		if (supercheck)
		{
			colormap = R_GetTranslationColormap(players[tab[i].num].skin, players[tab[i].num].mo ? players[tab[i].num].mo->color : tab[i].color, GTC_CACHE);
			V_DrawSmallMappedPatch (x, y-4, 0, superprefix[players[tab[i].num].skin], colormap);
		}
		else
		{
			colormap = R_GetTranslationColormap(players[tab[i].num].skin, players[tab[i].num].mo ? players[tab[i].num].mo->color : tab[i].color, GTC_CACHE);
			if (greycheck)
				V_DrawSmallTranslucentMappedPatch (x, y-4, V_80TRANS, faceprefix[players[tab[i].num].skin], colormap);
			else
				V_DrawSmallMappedPatch (x, y-4, 0, faceprefix[players[tab[i].num].skin], colormap);
		}
		V_DrawRightAlignedThinString(x+120, y, (greycheck ? V_TRANSLUCENT : 0), va("%u", tab[i].count));
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
	boolean greycheck, supercheck;

	V_DrawFill(160, 26, 1, 154, 0); //Draw a vertical line to separate the two sides.
	V_DrawFill(1, 26, 318, 1, 0); //And a horizontal line to make a T.
	V_DrawFill(1, 180, 318, 1, 0); //And a horizontal line near the bottom.

	for (i = 0; i < scorelines; i++)
	{
		if (players[tab[i].num].spectator && gametype != GT_COOP)
			continue; //ignore them.

		greycheck = greycheckdef;
		supercheck = supercheckdef;

		strlcpy(name, tab[i].name, 9);
		V_DrawString(x + 20, y,
		             ((tab[i].num == whiteplayer) ? V_YELLOWMAP : 0)
		             | (greycheck ? V_TRANSLUCENT : 0)
		             | V_ALLOWLOWERCASE, name);

		if (G_GametypeUsesLives() && !(gametype == GT_COOP && (cv_cooplives.value == 0 || cv_cooplives.value == 3)) && (players[tab[i].num].lives != 0x7f)) //show lives
			V_DrawRightAlignedString(x, y+4, V_ALLOWLOWERCASE, va("%dx", players[tab[i].num].lives));
		else if (G_TagGametype() && players[tab[i].num].pflags & PF_TAGIT)
			V_DrawSmallScaledPatch(x-28, y-4, 0, tagico);

		if (players[tab[i].num].exiting)
			V_DrawSmallScaledPatch(x - SHORT(exiticon->width)/2 - 1, y-3, 0, exiticon);

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
			if (supercheck)
				V_DrawSmallScaledPatch (x, y-4, 0, superprefix[players[tab[i].num].skin]);
			else
			{
				if (greycheck)
					V_DrawSmallTranslucentPatch (x, y-4, V_80TRANS, faceprefix[players[tab[i].num].skin]);
				else
					V_DrawSmallScaledPatch (x, y-4, 0, faceprefix[players[tab[i].num].skin]);
			}
		}
		else
		{
			if (supercheck)
			{
				colormap = R_GetTranslationColormap(players[tab[i].num].skin, players[tab[i].num].mo ? players[tab[i].num].mo->color : tab[i].color, GTC_CACHE);
				V_DrawSmallMappedPatch (x, y-4, 0, superprefix[players[tab[i].num].skin], colormap);
			}
			else
			{
				colormap = R_GetTranslationColormap(players[tab[i].num].skin, players[tab[i].num].mo ? players[tab[i].num].mo->color : tab[i].color, GTC_CACHE);
				if (greycheck)
					V_DrawSmallTranslucentMappedPatch (x, y-4, V_80TRANS, faceprefix[players[tab[i].num].skin], colormap);
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
					V_DrawRightAlignedThinString(x+156, y, 0, va("%i:%02i.%02i", G_TicsToMinutes(players[tab[i].num].realtime,true), G_TicsToSeconds(players[tab[i].num].realtime), G_TicsToCentiseconds(players[tab[i].num].realtime)));
				else
					V_DrawRightAlignedThinString(x+156, y, (greycheck ? V_TRANSLUCENT : 0), va("%u", tab[i].count));
			}
			else
				V_DrawRightAlignedThinString(x+156, y, (greycheck ? V_TRANSLUCENT : 0), va("%i:%02i.%02i", G_TicsToMinutes(tab[i].count,true), G_TicsToSeconds(tab[i].count), G_TicsToCentiseconds(tab[i].count)));
		}
		else
			V_DrawRightAlignedThinString(x+120, y, (greycheck ? V_TRANSLUCENT : 0), va("%u", tab[i].count));

		y += 16;
		if (y > 160)
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
		if (!playeringame[j])
			continue;

		if (gametype != GT_COOP && players[j].spectator)
			continue;

		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (!playeringame[i])
				continue;

			if (gametype != GT_COOP && players[i].spectator)
				continue;

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
		completed[tab[scorelines].num] = true;
		scorelines++;
	}

	if (scorelines > 20)
		scorelines = 20; //dont draw past bottom of screen, show the best only

	if (G_GametypeHasTeams())
		HU_DrawTeamTabRankings(tab, whiteplayer); //separate function for Spazzo's silly request
	else if (scorelines <= 9)
		HU_DrawTabRankings(40, 32, tab, scorelines, whiteplayer);
	else
		HU_DrawDualTabRankings(32, 32, tab, scorelines, whiteplayer);

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

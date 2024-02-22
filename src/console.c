// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2023 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  console.c
/// \brief Console drawing and input

#ifdef __GNUC__
#include <unistd.h>
#endif

#include "doomdef.h"
#include "console.h"
#include "g_game.h"
#include "g_input.h"
#include "hu_stuff.h"
#include "keys.h"
#include "r_main.h"
#include "r_defs.h"
#include "sounds.h"
#include "st_stuff.h"
#include "s_sound.h"
#include "v_video.h"
#include "i_video.h"
#include "z_zone.h"
#include "i_system.h"
#include "i_threads.h"
#include "d_main.h"
#include "m_menu.h"
#include "filesrch.h"
#include "m_misc.h"

#ifdef _WINDOWS
#include "win32/win_main.h"
#endif

#ifdef HWRENDER
#include "hardware/hw_main.h"
#endif

#define MAXHUDLINES 20

#ifdef HAVE_THREADS
I_mutex con_mutex;

#  define Lock_state()    I_lock_mutex(&con_mutex)
#  define Unlock_state() I_unlock_mutex(con_mutex)
#else/*HAVE_THREADS*/
#  define Lock_state()
#  define Unlock_state()
#endif/*HAVE_THREADS*/

static boolean con_started = false; // console has been initialised
       boolean con_startup = false; // true at game startup
       boolean con_refresh = false; // screen needs refreshing
static boolean con_forcepic = true; // at startup toggle console translucency when first off
       boolean con_recalc;          // set true when screen size has changed

static tic_t con_tick; // console ticker for blinking prompt cursor
                        // con_scrollup should use time (currenttime - lasttime)..

static boolean consoletoggle; // true when console key pushed, ticker will handle
static boolean consoleready;  // console prompt is ready

       INT32 con_destlines; // vid lines used by console at final position
static INT32 con_curlines;  // vid lines currently used by console

       INT32 con_clipviewtop; // (useless)

static UINT8  con_hudlines;             // number of console heads up message lines
static UINT32 con_hudtime[MAXHUDLINES]; // remaining time of display for hud msg lines

       INT32 con_clearlines;      // top screen lines to refresh when view reduced
       boolean con_hudupdate;   // when messages scroll, we need a backgrnd refresh

// console text output
static char *con_line;          // console text output current line
static size_t con_cx;           // cursor position in current line
static size_t con_cy;           // cursor line number in con_buffer, is always
                                // increasing, and wrapped around in the text
                                // buffer using modulo.

static size_t con_totallines;      // lines of console text into the console buffer
static size_t con_width;           // columns of chars, depend on vid mode width

static size_t con_scrollup;        // how many rows of text to scroll up (pgup/pgdn)
UINT32 con_scalefactor;            // text size scale factor

// hold 32 last lines of input for history
#define CON_MAXPROMPTCHARS 256
#define CON_PROMPTCHAR '$'

static char inputlines[32][CON_MAXPROMPTCHARS]; // hold last 32 prompt lines

static INT32 inputline;    // current input line number
static INT32 inputhist;    // line number of history input line to restore
static size_t input_cur; // position of cursor in line
static size_t input_sel; // position of selection marker (I.E.: anything between this and input_cur is "selected")
static size_t input_len; // length of current line, used to bound cursor and such
// notice: input does NOT include the "$" at the start of the line. - 11/3/16

// protos.
static void CON_InputInit(void);
static void CON_RecalcSize(void);
static void CON_ChangeHeight(void);

static void CON_DrawBackpic(void);
static void CONS_height_Change(void);
static void CONS_hudlines_Change(void);
static void CONS_backcolor_Change(void);

//======================================================================
//                   CONSOLE VARS AND COMMANDS
//======================================================================
#ifdef macintosh
#define CON_BUFFERSIZE 4096 // my compiler can't handle local vars >32k
#else
#define CON_BUFFERSIZE 32768
#endif

static char con_buffer[CON_BUFFERSIZE];

// how many seconds the hud messages lasts on the screen
// CV_Unsigned can overflow when multiplied by TICRATE later, so let's use a 3-year limit instead
static CV_PossibleValue_t hudtime_cons_t[] = {{0, "MIN"}, {99999999, "MAX"}, {0, NULL}};
static consvar_t cons_hudtime = CVAR_INIT ("con_hudtime", "5", CV_SAVE, hudtime_cons_t, NULL);

// number of lines displayed on the HUD
static CV_PossibleValue_t hudlines_cons_t[] = {{0, "MIN"}, {MAXHUDLINES, "MAX"}, {0, NULL}};
static consvar_t cons_hudlines = CVAR_INIT ("con_hudlines", "5", CV_CALL|CV_SAVE, hudlines_cons_t, CONS_hudlines_Change);

// number of lines console move per frame
// (con_speed needs a limit, apparently)
static CV_PossibleValue_t speed_cons_t[] = {{0, "MIN"}, {64, "MAX"}, {0, NULL}};
static consvar_t cons_speed = CVAR_INIT ("con_speed", "8", CV_SAVE, speed_cons_t, NULL);

// percentage of screen height to use for console
static consvar_t cons_height = CVAR_INIT ("con_height", "50", CV_CALL|CV_SAVE, CV_Unsigned, CONS_height_Change);

static CV_PossibleValue_t backpic_cons_t[] = {{0, "translucent"}, {1, "picture"}, {0, NULL}};
// whether to use console background picture, or translucent mode
static consvar_t cons_backpic = CVAR_INIT ("con_backpic", "translucent", CV_SAVE, backpic_cons_t, NULL);

static CV_PossibleValue_t backcolor_cons_t[] = {{0, "White"}, 		{1, "Black"},		{2, "Sepia"},
												{3, "Brown"},		{4, "Pink"},		{5, "Raspberry"},
												{6, "Red"},			{7, "Creamsicle"},	{8, "Orange"},
												{9, "Gold"},		{10,"Yellow"},		{11,"Emerald"},
												{12,"Green"},		{13,"Cyan"},		{14,"Steel"},
												{15,"Periwinkle"},	{16,"Blue"},		{17,"Purple"},
												{18,"Lavender"},
												{0, NULL}};


consvar_t cons_backcolor = CVAR_INIT ("con_backcolor", "Green", CV_CALL|CV_SAVE, backcolor_cons_t, CONS_backcolor_Change);

static void CON_Print(char *msg);

// Change the console height on demand
//
static void CONS_height_Change(void)
{
	Lock_state();

	if (con_destlines > 0 && !con_startup) // If the console is open (as in, not using "bind")...
		CON_ChangeHeight(); // ...update its height now, not only when it's closed and re-opened

	Unlock_state();
}

//
//
static void CONS_hudlines_Change(void)
{
	INT32 i;

	Lock_state();

	// Clear the currently displayed lines
	for (i = 0; i < con_hudlines; i++)
		con_hudtime[i] = 0;

	con_hudlines = cons_hudlines.value;

	Unlock_state();

	CONS_Printf(M_GetText("Number of console HUD lines is now %d\n"), con_hudlines);
}

// Clear console text buffer
//
static void CONS_Clear_f(void)
{
	Lock_state();

	memset(con_buffer, 0, CON_BUFFERSIZE);

	con_cx = 0;
	con_cy = con_totallines-1;
	con_line = &con_buffer[con_cy*con_width];
	con_scrollup = 0;

	Unlock_state();
}

// Choose english keymap
//
/*static void CONS_English_f(void)
{
	shiftxform = english_shiftxform;
	CONS_Printf(M_GetText("%s keymap.\n"), M_GetText("English"));
}*/

static char *bindtable[NUMINPUTS];

static void CONS_Bind_f(void)
{
	size_t na;
	char *newcmd = NULL;
	//size_t newlen = 0;
	unsigned int i;
	INT32 key;

	na = COM_Argc();

	if (na < 2)
	{
		CONS_Printf(M_GetText("bind <keyname> [<command>] [<arg1>] [...]: create shortcut keys to command(s)\n"));
		CONS_Printf("\x82%s", M_GetText("Bind table :\n"));
		na = 0;
		for (key = 0; key < NUMINPUTS; key++)
			if (bindtable[key])
			{
				CONS_Printf("%s : \"%s\"\n", G_KeyNumToName(key), bindtable[key]);
				na = 1;
			}
		if (!na)
			CONS_Printf(M_GetText("(empty)\n"));
		return;
	}

	key = G_KeyNameToNum(COM_Argv(1));
	if (key <= 0 || key >= NUMINPUTS)
	{
		CONS_Alert(CONS_NOTICE, M_GetText("Invalid key name\n"));
		return;
	}

	Z_Free(bindtable[key]);
	bindtable[key] = NULL;

	if (na < 3)
		return;

	for (i = 2; i < na; ++i)
	{
		const char *arg = COM_Argv(i);

		// on the second iteration, and after
		if (i > 2)
		{
			size_t newlen = strlen(bindtable[key]) + strlen(arg) + 1; // new length, allow space for ' ' and '\0'
			size_t curpos = newcmd - bindtable[key]; // offset from newcmd to original pointer

			newcmd = bindtable[key] = Z_Realloc(bindtable[key], newlen, PU_STATIC, NULL);
			newcmd += curpos; // reapply offset

			newcmd[0] = ' '; // replace previous '\0' w/ ' '
			++newcmd; // make sure later strcpy doesnt overwrite ' '
		}
		// first iteration
		else
			// allocate space for argument and a ' ' or '\0'
			newcmd = bindtable[key] = Z_Calloc(strlen(arg) + 1, PU_STATIC, NULL);

		// the copy
		strcpy(newcmd, arg);

		// move window past copied argument for next iteration
		newcmd += strlen(arg);
	}
}

//======================================================================
//                          CONSOLE SETUP
//======================================================================

// Font colormap colors
// TODO: This could probably be improved somehow...
// These colormaps are 99% identical, with just a few changed bytes
// This could EASILY be handled by modifying a centralised colormap
// for software depending on the prior state - but yknow, OpenGL...
UINT8 *yellowmap, *magentamap, *lgreenmap, *bluemap, *graymap, *redmap, *orangemap, *skymap, *purplemap, *aquamap, *peridotmap, *azuremap, *brownmap, *rosymap, *invertmap;

// Console BG color
UINT8 *consolebgmap = NULL;
UINT8 *promptbgmap = NULL;
static UINT8 promptbgcolor = UINT8_MAX;

void CON_SetupBackColormapEx(INT32 color, boolean prompt)
{
	UINT16 i, palsum;
	UINT8 j, palindex;
	UINT8 *pal = W_CacheLumpName(GetPalette(), PU_CACHE);
	INT32 shift = 6;

	if (color == INT32_MAX)
		color = cons_backcolor.value;

	shift = 6; // 12 colors -- shift of 7 means 6 colors

	switch (color)
	{
		case 0:		palindex = 15; 	break; 	// White
		case 1:		palindex = 31;	break; 	// Black
		case 2:		palindex = 251;	break;	// Sepia
		case 3:		palindex = 239;	break; 	// Brown
		case 4:		palindex = 215; shift = 7; 	break; 	// Pink
		case 5:		palindex = 37; shift = 7;	break; 	// Raspberry
		case 6:		palindex = 47; shift = 7;	break; 	// Red
		case 7:		palindex = 53;	shift = 7;	break;	// Creamsicle
		case 8:		palindex = 63;	break; 	// Orange
		case 9:		palindex = 56; shift = 7;	break; 	// Gold
		case 10:	palindex = 79; shift = 7;	break; 	// Yellow
		case 11:	palindex = 119; shift = 7; 	break; 	// Emerald
		case 12:	palindex = 111;	break; 	// Green
		case 13:	palindex = 136;	shift = 7; break; 	// Cyan
		case 14:	palindex = 175; shift = 7;	break; 	// Steel
		case 15:	palindex = 166;	shift = 7; 	break; 	// Periwinkle
		case 16:	palindex = 159;	break; 	// Blue
		case 17:	palindex = 187; shift = 7; 	break; 	// Purple
		case 18:	palindex = 199; shift = 7; 	break; 	// Lavender
		// Default green
		default:	palindex = 111; break;
	}

	if (prompt)
	{
		if (!promptbgmap)
			promptbgmap = (UINT8 *)Z_Malloc(256, PU_STATIC, NULL);

		if (color == promptbgcolor)
			return;
		else
			promptbgcolor = color;
	}
	else if (!consolebgmap)
		consolebgmap = (UINT8 *)Z_Malloc(256, PU_STATIC, NULL);

	// setup background colormap
	for (i = 0, j = 0; i < 768; i += 3, j++)
	{
		palsum = (pal[i] + pal[i+1] + pal[i+2]) >> shift;
		if (prompt)
			promptbgmap[j] = (UINT8)(palindex - palsum);
		else
			consolebgmap[j] = (UINT8)(palindex - palsum);
	}
}

void CON_SetupBackColormap(void)
{
	CON_SetupBackColormapEx(cons_backcolor.value, false);
	CON_SetupBackColormapEx(1, true); // default to gray
}

static void CONS_backcolor_Change(void)
{
	CON_SetupBackColormapEx(cons_backcolor.value, false);
}

static void CON_SetupColormaps(void)
{
	INT32 i;
	UINT8 *memorysrc = (UINT8 *)Z_Malloc((256*15), PU_STATIC, NULL);

	magentamap = memorysrc;
	yellowmap  = (magentamap+256);
	lgreenmap  = (yellowmap+256);
	bluemap    = (lgreenmap+256);
	redmap     = (bluemap+256);
	graymap    = (redmap+256);
	orangemap  = (graymap+256);
	skymap     = (orangemap+256);
	purplemap  = (skymap+256);
	aquamap    = (purplemap+256);
	peridotmap = (aquamap+256);
	azuremap   = (peridotmap+256);
	brownmap   = (azuremap+256);
	rosymap    = (brownmap+256);
	invertmap  = (rosymap+256);

	// setup the other colormaps, for console text

	// these don't need to be aligned, unless you convert the
	// V_DrawMappedPatch() into optimised asm.

	for (i = 0; i < (256*15); i++, ++memorysrc)
		*memorysrc = (UINT8)(i & 0xFF); // remap each color to itself...

#define colset(map, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p) \
	map[0x0] = (UINT8)a;\
	map[0x1] = (UINT8)b;\
	map[0x2] = (UINT8)c;\
	map[0x3] = (UINT8)d;\
	map[0x4] = (UINT8)e;\
	map[0x5] = (UINT8)f;\
	map[0x6] = (UINT8)g;\
	map[0x7] = (UINT8)h;\
	map[0x8] = (UINT8)i;\
	map[0x9] = (UINT8)j;\
	map[0xA] = (UINT8)k;\
	map[0xB] = (UINT8)l;\
	map[0xC] = (UINT8)m;\
	map[0xD] = (UINT8)n;\
	map[0xE] = (UINT8)o;\
	map[0xF] = (UINT8)p;

	// Tried to keep the colors vanilla while adding some shades in between them ~SonicX8000

	//                      0x1       0x3                           0x9                           0xF
	colset(magentamap, 177, 177, 178, 178, 178, 180, 180, 180, 182, 182, 182, 182, 184, 184, 184, 185);
	colset(yellowmap,   82,  82,  73,  73,  73,  74,  74,  74,  66,  66,  66,  66,  67,  67,  67,  68);
	colset(lgreenmap,   96,  96,  98,  98,  98, 100, 100, 100, 103, 103, 103, 103, 105, 105, 105, 107);
	colset(bluemap,    146, 146, 147, 147, 147, 148, 148, 148, 149, 149, 149, 149, 150, 150, 150, 151);
	colset(redmap,      32,  32,  33,  33,  33,  34,  34,  34,  35,  35,  35,  35,  37,  37,  37,  39);
	colset(graymap,      8,   9,  10,  11,  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,  22,  23);
	colset(orangemap,   50,  50,  52,  52,  52,  54,  54,  54,  56,  56,  56,  56,  59,  59,  59,  60);
	colset(skymap,     129, 129, 130, 130, 130, 131, 131, 131, 133, 133, 133, 133, 135, 135, 135, 136);
	colset(purplemap,  160, 160, 161, 161, 161, 162, 162, 162, 163, 163, 163, 163, 164, 164, 164, 165);
	colset(aquamap,    120, 120, 121, 121, 121, 122, 122, 122, 123, 123, 123, 123, 124, 124, 124, 125);
	colset(peridotmap,  73,  73, 188, 188, 188, 189, 189, 189, 190, 190, 190, 190, 191, 191, 191,  94);
	colset(azuremap,   144, 144, 145, 145, 145, 146, 146, 146, 170, 170, 170, 170, 171, 171, 171, 172);
	colset(brownmap,   219, 219, 221, 221, 221, 222, 222, 222, 224, 224, 224, 224, 227, 227, 227, 229);
	colset(rosymap,    200, 200, 201, 201, 201, 202, 202, 202, 203, 203, 203, 203, 204, 204, 204, 205);

#undef colset

	// Yeah just straight up invert it like a normal person
	for (i = 0x00; i <= 0x1F; i++)
		invertmap[0x1F - i] = i;

	// Init back colormap
	CON_SetupBackColormap();
}

// Setup the console text buffer
//
void CON_Init(void)
{
	INT32 i;

	for (i = 0; i < NUMINPUTS; i++)
		bindtable[i] = NULL;

	Lock_state();

	// clear all lines
	memset(con_buffer, 0, CON_BUFFERSIZE);

	// make sure it is ready for the loading screen
	con_width = 0;

	Unlock_state();

	CON_RecalcSize();

	CON_SetupColormaps();

	Lock_state();

	//note: CON_Ticker should always execute at least once before D_Display()
	con_clipviewtop = -1; // -1 does not clip

	con_hudlines = atoi(cons_hudlines.defaultvalue);

	Unlock_state();

	// setup console input filtering
	CON_InputInit();

	// register our commands
	//
	COM_AddCommand("cls", CONS_Clear_f, 0);
	//COM_AddCommand("english", CONS_English_f);
	// set console full screen for game startup MAKE SURE VID_Init() done !!!
	Lock_state();

	con_destlines = vid.height;
	con_curlines = vid.height;

	Unlock_state();

	if (!dedicated)
	{
		Lock_state();

		con_started = true;
		con_startup = true;
		con_refresh = true; // needs explicit screen refresh until we are in the main game loop
		consoletoggle = false;

		Unlock_state();

		CV_RegisterVar(&cons_hudtime);
		CV_RegisterVar(&cons_hudlines);
		CV_RegisterVar(&cons_speed);
		CV_RegisterVar(&cons_height);
		CV_RegisterVar(&cons_backpic);
		CV_RegisterVar(&cons_backcolor);
		COM_AddCommand("bind", CONS_Bind_f, 0);
	}
	else
	{
		Lock_state();

		con_started = true;
		con_startup = false;
		con_refresh = false; // disable explicit screen refresh
		consoletoggle = true;

		Unlock_state();
	}
}

void CON_StartRefresh(void)
{
	if (con_startup)
		con_refresh = true;
}

void CON_StopRefresh(void)
{
	if (con_startup)
		con_refresh = false;
}

// Console input initialization
//
static void CON_InputInit(void)
{
	Lock_state();

	// prepare the first prompt line
	memset(inputlines, 0, sizeof (inputlines));
	inputline = 0;
	input_cur = input_sel = input_len = 0;

	Unlock_state();
}

//======================================================================
//                        CONSOLE EXECUTION
//======================================================================

// Called at screen size change to set the rows and line size of the
// console text buffer.
//
static void CON_RecalcSize(void)
{
	size_t conw, oldcon_width, oldnumlines, i, oldcon_cy;
	char *tmp_buffer;
	char *string;

	Lock_state();

	switch (cv_constextsize.value)
	{
	case V_NOSCALEPATCH:
		con_scalefactor = 1;
		break;
	case V_SMALLSCALEPATCH:
		con_scalefactor = vid.smalldup;
		break;
	case V_MEDSCALEPATCH:
		con_scalefactor = vid.meddup;
		break;
	default:	// Full scaling
		con_scalefactor = vid.dup;
		break;
	}

	con_recalc = false;

	if (dedicated)
		conw = 1;
	else
		conw = (vid.width>>3) / con_scalefactor - 2;

	if (con_curlines == vid.height) // first init
	{
		con_curlines = vid.height;
		con_destlines = vid.height;
	}

	if (con_destlines > 0) // Resize console if already open
	{
		CON_ChangeHeight();
		con_curlines = con_destlines;
	}

	// check for change of video width
	if (conw == con_width)
	{
		Unlock_state();
		return; // didn't change
	}

	Unlock_state();

	tmp_buffer = Z_Malloc(CON_BUFFERSIZE, PU_STATIC, NULL);
	string = Z_Malloc(CON_BUFFERSIZE, PU_STATIC, NULL); // BP: it is a line but who know

	Lock_state();

	oldcon_width = con_width;
	oldnumlines = con_totallines;
	oldcon_cy = con_cy;
	M_Memcpy(tmp_buffer, con_buffer, CON_BUFFERSIZE);

	if (conw < 1)
		con_width = (BASEVIDWIDTH>>3) - 2;
	else
		con_width = conw;

	con_width += 11; // Graue 06-19-2004 up to 11 control chars per line

	con_totallines = CON_BUFFERSIZE / con_width;
	memset(con_buffer, ' ', CON_BUFFERSIZE);

	con_cx = 0;
	con_cy = con_totallines-1;
	con_line = &con_buffer[con_cy*con_width];
	con_scrollup = 0;

	Unlock_state();

	// re-arrange console text buffer to keep text
	if (oldcon_width) // not the first time
	{
		for (i = oldcon_cy + 1; i < oldcon_cy + oldnumlines; i++)
		{
			if (tmp_buffer[(i%oldnumlines)*oldcon_width])
			{
				M_Memcpy(string, &tmp_buffer[(i%oldnumlines)*oldcon_width], oldcon_width);
				conw = oldcon_width - 1;
				while (string[conw] == ' ' && conw)
					conw--;
				string[conw+1] = '\n';
				string[conw+2] = '\0';
				CON_Print(string);
			}
		}
	}

	Z_Free(string);
	Z_Free(tmp_buffer);
}

static void CON_ChangeHeight(void)
{
	INT32 minheight;

	Lock_state();

	minheight = 20 * con_scalefactor;	// 20 = 8+8+4

	// toggle console in
	con_destlines = (cons_height.value*vid.height)/100;
	if (con_destlines < minheight)
		con_destlines = minheight;
	else if (con_destlines > vid.height)
		con_destlines = vid.height;

	con_destlines &= ~0x3; // multiple of text row height

	Unlock_state();
}

// Handles Console moves in/out of screen (per frame)
//
static void CON_MoveConsole(void)
{
	static fixed_t fracmovement = 0;

	Lock_state();

	// instant
	if (!cons_speed.value)
	{
		con_curlines = con_destlines;
		Unlock_state();
		return;
	}

	// Not instant - Increment fracmovement fractionally
	fracmovement += FixedMul(cons_speed.value*vid.fdup, renderdeltatics);

	if (con_curlines < con_destlines) // Move the console downwards
	{
		con_curlines += FixedInt(fracmovement); // Move by fracmovement's integer value
		if (con_curlines > con_destlines) // If we surpassed the destination...
			con_curlines = con_destlines; // ...clamp to it!
	}
	else // Move the console upwards
	{
		con_curlines -= FixedInt(fracmovement);
		if (con_curlines < con_destlines)
			con_curlines = con_destlines;

		if (con_destlines == 0) // If the console is being closed, not just moved up...
			con_tick = 0; // ...don't show the blinking cursor
	}

	fracmovement %= FRACUNIT; // Reset fracmovement's integer value, but keep the fraction

	Unlock_state();
}

// Clear time of console heads up messages
//
void CON_ClearHUD(void)
{
	INT32 i;

	Lock_state();

	for (i = 0; i < con_hudlines; i++)
		con_hudtime[i] = 0;

	Unlock_state();
}

// Force console to move out immediately
// note: con_ticker will set consoleready false
void CON_ToggleOff(void)
{
	Lock_state();

	if (!con_destlines)
	{
		Unlock_state();
		return;
	}

	con_destlines = 0;
	con_curlines = 0;
	CON_ClearHUD();
	con_forcepic = 0;
	con_clipviewtop = -1; // remove console clipping of view

	I_UpdateMouseGrab();

	Unlock_state();
}

boolean CON_Ready(void)
{
	boolean ready;
	Lock_state();
	{
		ready = consoleready;
	}
	Unlock_state();
	return ready;
}

// Console ticker: handles console move in/out, cursor blinking
//
void CON_Ticker(void)
{
	INT32 i;
	INT32 minheight;

	Lock_state();

	minheight = 20 * con_scalefactor;	// 20 = 8+8+4

	// cursor blinking
	con_tick++;
	con_tick &= 7;

	// console key was pushed
	if (consoletoggle)
	{
		consoletoggle = false;

		// toggle off console
		if (con_destlines > 0)
		{
			con_destlines = 0;
			CON_ClearHUD();
			I_UpdateMouseGrab();
		}
		else
			CON_ChangeHeight();
	}

	// clip the view, so that the part under the console is not drawn
	con_clipviewtop = -1;
	if (cons_backpic.value) // clip only when using an opaque background
	{
		if (con_curlines > 0)
			con_clipviewtop = con_curlines - viewwindowy - 1 - 10;
		// NOTE: BIG HACK::SUBTRACT 10, SO THAT WATER DON'T COPY LINES OF THE CONSOLE
		//       WINDOW!!! (draw some more lines behind the bottom of the console)
		if (con_clipviewtop < 0)
			con_clipviewtop = -1; // maybe not necessary, provided it's < 0
	}

	// check if console ready for prompt
	if (con_destlines >= minheight)
		consoleready = true;
	else
		consoleready = false;

	// make overlay messages disappear after a while
	for (i = 0; i < con_hudlines; i++)
	{
		if (con_hudtime[i])
			con_hudtime[i]--;
	}

	Unlock_state();
}

//
// ----
//
// Shortcuts for adding and deleting characters, strings, and sections
// Necessary due to moving cursor
//

static void CON_InputClear(void)
{
	Lock_state();

	memset(inputlines[inputline], 0, CON_MAXPROMPTCHARS);
	input_cur = input_sel = input_len = 0;

	Unlock_state();
}

static void CON_InputSetString(const char *c)
{
	Lock_state();

	memset(inputlines[inputline], 0, CON_MAXPROMPTCHARS);
	strcpy(inputlines[inputline], c);
	input_cur = input_sel = input_len = strlen(c);

	Unlock_state();
}

static void CON_InputAddString(const char *c)
{
	size_t csize = strlen(c);

	Lock_state();

	if (input_len + csize > CON_MAXPROMPTCHARS-1)
	{
		Unlock_state();
		return;
	}
	if (input_cur != input_len)
		memmove(&inputlines[inputline][input_cur+csize], &inputlines[inputline][input_cur], input_len-input_cur);
	memcpy(&inputlines[inputline][input_cur], c, csize);
	input_len += csize;
	input_sel = (input_cur += csize);

	Unlock_state();
}

static void CON_InputDelSelection(void)
{
	size_t start, end, len;

	Lock_state();

	if (!input_cur)
	{
		Unlock_state();
		return;
	}

	if (input_cur > input_sel)
	{
		start = input_sel;
		end = input_cur;
	}
	else
	{
		start = input_cur;
		end = input_sel;
	}
	len = (end - start);

	if (end != input_len)
		memmove(&inputlines[inputline][start], &inputlines[inputline][end], input_len-end);
	memset(&inputlines[inputline][input_len - len], 0, len);

	input_len -= len;
	input_sel = input_cur = start;

	Unlock_state();
}

static void CON_InputAddChar(char c)
{
	if (input_len >= CON_MAXPROMPTCHARS-1)
		return;

	Lock_state();

	if (input_cur != input_len)
		memmove(&inputlines[inputline][input_cur+1], &inputlines[inputline][input_cur], input_len-input_cur);
	inputlines[inputline][input_cur++] = c;
	inputlines[inputline][++input_len] = 0;
	input_sel = input_cur;

	Unlock_state();
}

static void CON_InputDelChar(void)
{
	if (!input_cur)
		return;

	Lock_state();

	if (input_cur != input_len)
		memmove(&inputlines[inputline][input_cur-1], &inputlines[inputline][input_cur], input_len-input_cur);
	inputlines[inputline][--input_len] = 0;
	input_sel = --input_cur;

	Unlock_state();
}

//
// ----
//

// Handles console key input
//
boolean CON_Responder(event_t *ev)
{
	static UINT8 consdown = false; // console is treated differently due to rare usage

	// sequential completions a la 4dos
	static char completioncmd[80 + sizeof("find ")] = "find ";
	static char *completion = &completioncmd[sizeof("find ")-1];

	static INT32 skips;

	static INT32   com_skips;
	static INT32   var_skips;
	static INT32 alias_skips;

	const char *cmd = NULL;
	INT32 key;

	if (chat_on)
		return false;

	// let go keyup events, don't eat them
	if (ev->type != ev_keydown && ev->type != ev_text && ev->type != ev_console)
	{
		if (ev->key == gamecontrol[GC_CONSOLE][0] || ev->key == gamecontrol[GC_CONSOLE][1])
			consdown = false;
		return false;
	}

	key = ev->key;

	// check for console toggle key
	if (ev->type != ev_console)
	{
		if (modeattacking || metalrecording || marathonmode)
			return false;

		if (ev->type == ev_keydown && ((key == gamecontrol[GC_CONSOLE][0] || key == gamecontrol[GC_CONSOLE][1]) && !shiftdown))
		{
			if (consdown) // ignore repeat
				return true;
			consoletoggle = true;
			consdown = true;
			return true;
		}

		// check other keys only if console prompt is active
		if (!consoleready && key < NUMINPUTS) // metzgermeister: boundary check!!
		{
			if (ev->type == ev_keydown && !menuactive && bindtable[key])
			{
				COM_BufAddText(bindtable[key]);
				COM_BufAddText("\n");
				return true;
			}
			return false;
		}

		// escape key toggle off console
		if (key == KEY_ESCAPE)
		{
			consoletoggle = true;
			return true;
		}
	}

	if (ev->type == ev_text)
	{
		if (!consoletoggle)
			CON_InputAddChar(key);
		return true;
	}

	// Always eat ctrl/shift/alt if console open, so the menu doesn't get ideas
	if (key == KEY_LSHIFT || key == KEY_RSHIFT
	 || key == KEY_LCTRL || key == KEY_RCTRL
	 || key == KEY_LALT || key == KEY_RALT)
		return true;

	if (key == KEY_LEFTARROW)
	{
		if (input_cur != 0)
		{
			if (ctrldown)
				input_cur = M_JumpWordReverse(inputlines[inputline], input_cur);
			else
				--input_cur;
		}
		if (!shiftdown)
			input_sel = input_cur;
		return true;
	}
	else if (key == KEY_RIGHTARROW)
	{
		if (input_cur < input_len)
		{
			if (ctrldown)
				input_cur += M_JumpWord(&inputlines[inputline][input_cur]);
			else
				++input_cur;
		}
		if (!shiftdown)
			input_sel = input_cur;
		return true;
	}

	// backspace and delete command prompt
	if (input_sel != input_cur)
	{
		if (key == KEY_BACKSPACE || key == KEY_DEL)
		{
			CON_InputDelSelection();
			return true;
		}
	}
	else if (key == KEY_BACKSPACE)
	{
		if (ctrldown)
		{
			input_sel = M_JumpWordReverse(inputlines[inputline], input_cur);
			CON_InputDelSelection();
		}
		else
			CON_InputDelChar();
		return true;
	}
	else if (key == KEY_DEL)
	{
		if (input_cur == input_len)
			return true;

		if (ctrldown)
		{
			input_sel = input_cur + M_JumpWord(&inputlines[inputline][input_cur]);
			CON_InputDelSelection();
		}
		else
		{
			++input_cur;
			CON_InputDelChar();
		}
		return true;
	}

	// ctrl modifier -- changes behavior, adds shortcuts
	if (ctrldown)
	{
		// show all cvars/commands that match what we have inputted
		if (key == KEY_TAB)
		{
			if (!completion[0])
			{
				if (!input_len || input_len >= 40 || strchr(inputlines[inputline], ' '))
					return true;
				strcpy(completion, inputlines[inputline]);
			}
			COM_BufInsertText(completioncmd);
			completion[0] = 0;
			return true;
		}
		// ---

		if (key == KEY_HOME) // oldest text in buffer
		{
			con_scrollup = (con_totallines-((con_curlines-16)>>3));
			return true;
		}
		else if (key == KEY_END) // most recent text in buffer
		{
			con_scrollup = 0;
			return true;
		}

		if (key == 'x' || key == 'X')
		{
			if (input_sel > input_cur)
				I_ClipboardCopy(&inputlines[inputline][input_cur], input_sel-input_cur);
			else
				I_ClipboardCopy(&inputlines[inputline][input_sel], input_cur-input_sel);
			CON_InputDelSelection();
			completion[0] = 0;
			return true;
		}
		else if (key == 'c' || key == 'C')
		{
			if (input_sel > input_cur)
				I_ClipboardCopy(&inputlines[inputline][input_cur], input_sel-input_cur);
			else
				I_ClipboardCopy(&inputlines[inputline][input_sel], input_cur-input_sel);
			return true;
		}
		else if (key == 'v' || key == 'V')
		{
			const char *paste = I_ClipboardPaste();
			if (input_sel != input_cur)
				CON_InputDelSelection();
			if (paste != NULL)
				CON_InputAddString(paste);
			completion[0] = 0;
			return true;
		}

		// Select all
		if (key == 'a' || key == 'A')
		{
			input_sel = 0;
			input_cur = input_len;
			return true;
		}

		// ...why shouldn't it eat the key? if it doesn't, it just means you
		// can control Sonic from the console, which is silly
		return true;//return false;
	}

	// command completion forward (tab) and backward (shift-tab)
	if (key == KEY_TAB)
	{
		// sequential command completion forward and backward

		// remember typing for several completions (a-la-4dos)
		if (!completion[0])
		{
			if (!input_len || input_len >= 40 || strchr(inputlines[inputline], ' '))
				return true;
			strcpy(completion, inputlines[inputline]);
			skips       = 0;
			com_skips   = 0;
			var_skips   = 0;
			alias_skips = 0;
		}
		else
		{
			if (shiftdown)
			{
				if (skips > 0)
					skips--;
			}
			else
			{
				skips++;
			}
		}

		if (skips <= com_skips)
		{
			cmd = COM_CompleteCommand(completion, skips);

			if (cmd && skips == com_skips)
			{
				com_skips  ++;
				var_skips  ++;
				alias_skips++;
			}
		}

		if (!cmd && skips <= var_skips)
		{
			cmd = CV_CompleteVar(completion, skips - com_skips);

			if (cmd && skips == var_skips)
			{
				var_skips  ++;
				alias_skips++;
			}
		}

		if (!cmd && skips <= alias_skips)
		{
			cmd = COM_CompleteAlias(completion, skips - var_skips);

			if (cmd && skips == alias_skips)
			{
				alias_skips++;
			}
		}

		if (cmd)
		{
			CON_InputSetString(va("%s ", cmd));
		}
		else
		{
			skips--;
		}

		return true;
	}

	// move up (backward) in console textbuffer
	if (key == KEY_PGUP)
	{
		if (con_scrollup < (con_totallines-((con_curlines-16)>>3)))
			con_scrollup++;
		return true;
	}
	else if (key == KEY_PGDN)
	{
		if (con_scrollup > 0)
			con_scrollup--;
		return true;
	}
	else if (key == KEY_HOME)
	{
		input_cur = 0;
		if (!shiftdown)
			input_sel = input_cur;
		return true;
	}
	else if (key == KEY_END)
	{
		input_cur = input_len;
		if (!shiftdown)
			input_sel = input_cur;
		return true;
	}

	// At this point we're messing with input
	// Clear completion
	completion[0] = 0;

	// command enter
	if (key == KEY_ENTER)
	{
		if (!input_len)
			return true;

		// push the command
		COM_BufAddText(inputlines[inputline]);
		COM_BufAddText("\n");

		CONS_Printf("\x86""%c""\x80""%s\n", CON_PROMPTCHAR, inputlines[inputline]);

		inputline = (inputline+1) & 31;
		inputhist = inputline;
		CON_InputClear();

		return true;
	}

	// move back in input history
	if (key == KEY_UPARROW)
	{
		// copy one of the previous inputlines to the current
		do
			inputhist = (inputhist - 1) & 31; // cycle back
		while (inputhist != inputline && !inputlines[inputhist][0]);

		// stop at the last history input line, which is the
		// current line + 1 because we cycle through the 32 input lines
		if (inputhist == inputline)
			inputhist = (inputline + 1) & 31;

		CON_InputSetString(inputlines[inputhist]);
		return true;
	}

	// move forward in input history
	if (key == KEY_DOWNARROW)
	{
		if (inputhist == inputline)
			return true;
		do
			inputhist = (inputhist + 1) & 31;
		while (inputhist != inputline && !inputlines[inputhist][0]);

		// back to currentline
		if (inputhist == inputline)
			CON_InputClear();
		else
			CON_InputSetString(inputlines[inputhist]);
		return true;
	}

	// allow people to use keypad in console (good for typing IP addresses) - Calum
	if (key >= KEY_KEYPAD7 && key <= KEY_KPADDEL)
	{
		char keypad_translation[] = {'7','8','9','-',
		                             '4','5','6','+',
		                             '1','2','3',
		                             '0','.'};

		key = keypad_translation[key - KEY_KEYPAD7];
	}
	else if (key == KEY_KPADSLASH)
		key = '/';

	// enter a char into the command prompt
	if (key < 32 || key > 127)
		return true;

	if (input_sel != input_cur)
		CON_InputDelSelection();
	if (ev->type == ev_console)
		CON_InputAddChar(key);

	return true;
}

// Insert a new line in the console text buffer
//
static void CON_Linefeed(void)
{
	// set time for heads up messages
	if (con_hudlines)
		con_hudtime[con_cy%con_hudlines] = cons_hudtime.value*TICRATE;

	con_cy++;
	con_cx = 0;

	con_line = &con_buffer[(con_cy%con_totallines)*con_width];
	memset(con_line, ' ', con_width);

	// make sure the view borders are refreshed if hud messages scroll
	con_hudupdate = true; // see HU_Erase()
}

// Outputs text into the console text buffer
static void CON_Print(char *msg)
{
	size_t l;
	INT32 controlchars = 0; // for color changing
	char color = '\x80';  // keep color across lines

	if (msg == NULL)
		return;

	if (*msg == '\3') // chat text, makes ding sound
		S_StartSound(NULL, sfx_radio);
	else if (*msg == '\4') // chat action, dings and is in yellow
	{
		*msg = '\x82'; // yellow
		S_StartSound(NULL, sfx_radio);
	}

	Lock_state();

	if (!(*msg & 0x80))
	{
		con_line[con_cx++] = '\x80';
		controlchars = 1;
	}

	while (*msg)
	{
		// skip non-printable characters and white spaces
		while (*msg && *msg <= ' ')
		{
			if (*msg & 0x80)
			{
				color = con_line[con_cx++] = *(msg++);
				controlchars++;
				continue;
			}
			else if (*msg == '\r') // carriage return
			{
				con_cy--;
				CON_Linefeed();
				color = '\x80';
				controlchars = 0;
			}
			else if (*msg == '\n') // linefeed
			{
				CON_Linefeed();
				con_line[con_cx++] = color;
				controlchars = 1;
			}
			else if (*msg == ' ') // space
			{
				con_line[con_cx++] = ' ';
				if (con_cx - controlchars >= con_width-11)
				{
					CON_Linefeed();
					con_line[con_cx++] = color;
					controlchars = 1;
				}
			}
			else if (*msg == '\t')
			{
				// adds tab spaces for nice layout in console

				do
				{
					con_line[con_cx++] = ' ';
				} while ((con_cx - controlchars) % 4 != 0);

				if (con_cx - controlchars >= con_width-11)
				{
					CON_Linefeed();
					con_line[con_cx++] = color;
					controlchars = 1;
				}
			}
			msg++;
		}

		if (*msg == '\0')
		{
			Unlock_state();
			return;
		}

		// printable character
		for (l = 0; l < (con_width-11) && msg[l] > ' '; l++)
			;

		// word wrap
		if ((con_cx - controlchars) + l > con_width-11)
		{
			CON_Linefeed();
			con_line[con_cx++] = color;
			controlchars = 1;
		}

		// a word at a time
		for (; l > 0; l--)
			con_line[con_cx++] = *(msg++);
	}

	Unlock_state();
}

void CON_LogMessage(const char *msg)
{
	char txt[8192], *t;
	const char *p = msg, *e = txt+sizeof (txt)-2;

	for (t = txt; *p != '\0'; p++)
	{
		if (*p == '\n' || *p >= ' ') // don't log or console print CON_Print's control characters
			*t++ = *p;

		if (t >= e)
		{
			*t = '\0'; //end of string
			I_OutputMsg("%s", txt); //print string
			t = txt; //reset t pointer
			memset(txt,'\0', sizeof (txt)); //reset txt
		}
	}
	*t = '\0'; //end of string
	I_OutputMsg("%s", txt);
}

// Console print! Wahooo! Lots o fun!
//

void CONS_Printf(const char *fmt, ...)
{
	va_list argptr;
	static char *txt = NULL;
	boolean refresh;

	if (txt == NULL)
		txt = malloc(8192);

	va_start(argptr, fmt);
	vsprintf(txt, fmt, argptr);
	va_end(argptr);

	// echo console prints to log file
	DEBFILE(txt);

	// write message in con text buffer
	if (con_started)
		CON_Print(txt);

	CON_LogMessage(txt);

	Lock_state();

	// make sure new text is visible
	con_scrollup = 0;
	refresh = con_refresh;

	Unlock_state();

	// if not in display loop, force screen update
	if (refresh)
	{
		CON_Drawer(); // here we display the console text
		I_FinishUpdate(); // page flip or blit buffer
	}
}

void CONS_Alert(alerttype_t level, const char *fmt, ...)
{
	va_list argptr;
	static char *txt = NULL;

	if (txt == NULL)
		txt = malloc(8192);

	va_start(argptr, fmt);
	vsprintf(txt, fmt, argptr);
	va_end(argptr);

	switch (level)
	{
		case CONS_NOTICE:
			// no notice for notices, hehe
			CONS_Printf("\x83" "%s" "\x80 ", M_GetText("NOTICE:"));
			break;
		case CONS_WARNING:
			refreshdirmenu |= REFRESHDIR_WARNING;
			CONS_Printf("\x82" "%s" "\x80 ", M_GetText("WARNING:"));
			break;
		case CONS_ERROR:
			refreshdirmenu |= REFRESHDIR_ERROR;
			CONS_Printf("\x85" "%s" "\x80 ", M_GetText("ERROR:"));
			break;
	}

	// I am lazy and I feel like just letting CONS_Printf take care of things.
	// Is that okay?
	CONS_Printf("%s", txt);
}

void CONS_Debug(INT32 debugflags, const char *fmt, ...)
{
	va_list argptr;
	static char *txt = NULL;

	if ((cv_debug & debugflags) != debugflags)
		return;

	if (txt == NULL)
		txt = malloc(8192);

	va_start(argptr, fmt);
	vsprintf(txt, fmt, argptr);
	va_end(argptr);

	// Again I am lazy, oh well
	CONS_Printf("%s", txt);
}


// Print an error message, and wait for ENTER key to continue.
// To make sure the user has seen the message
//
void CONS_Error(const char *msg)
{
#if defined(RPC_NO_WINDOWS_H) && defined(_WINDOWS)
	if (!graphics_started)
	{
		MessageBoxA(vid.WndParent, msg, "SRB2 Warning", MB_OK);
		return;
	}
#endif
	CONS_Printf("\x82%s", msg); // write error msg in different colour
	CONS_Printf(M_GetText("Press ENTER to continue\n"));

	// dirty quick hack, but for the good cause
	while (I_GetKey() != KEY_ENTER)
		I_OsPolling();
}

//======================================================================
//                          CONSOLE DRAW
//======================================================================

// draw console prompt line
//
static void CON_DrawInput(void)
{
	INT32 charwidth = (INT32)con_scalefactor << 3;
	const char *p = inputlines[inputline];
	size_t c, clen, cend;
	UINT8 lellip = 0, rellip = 0;
	INT32 x, y, i;

	y = con_curlines - 12 * con_scalefactor;
	x = charwidth*2;

	clen = con_width-13;

	if (input_len <= clen)
	{
		c = 0;
		clen = input_len;
	}
	else // input line scrolls left if it gets too long
	{
		clen -= 2; // There will always be some extra truncation -- but where is what we'll find out

		if (input_cur <= clen/2)
		{
			// Close enough to right edge to show all
			c = 0;
			// Always will truncate right side from this position, so always draw right ellipsis
			rellip = 1;
		}
		else
		{
			// Cursor in the middle (or right side) of input
			// Move over for the ellipsis
			c = input_cur - (clen/2) + 2;
			x += charwidth*2;
			lellip = 1;

			if (c + clen >= input_len)
			{
				// Cursor in the right side of input
				// We were too far over, so move back
				c = input_len - clen;
			}
			else
			{
				// Cursor in the middle -- ellipses on both sides
				clen -= 2;
				rellip = 1;
			}
		}
	}

	if (lellip)
	{
		x -= charwidth*3;
		if (input_sel < c)
			V_DrawFill(x, y, charwidth*3, (10 * con_scalefactor), 77 | V_NOSCALESTART);
		for (i = 0; i < 3; ++i, x += charwidth)
			V_DrawCharacter(x, y, '.' | cv_constextsize.value | V_GRAYMAP | V_NOSCALESTART, true);
	}
	else
		V_DrawCharacter(x-charwidth, y, CON_PROMPTCHAR | cv_constextsize.value | V_GRAYMAP | V_NOSCALESTART, true);

	for (cend = c + clen; c < cend; ++c, x += charwidth)
	{
		if ((input_sel > c && input_cur <= c) || (input_sel <= c && input_cur > c))
		{
			V_DrawFill(x, y, charwidth, (10 * con_scalefactor), 77 | V_NOSCALESTART);
			V_DrawCharacter(x, y, p[c] | cv_constextsize.value | V_YELLOWMAP | V_NOSCALESTART, true);
		}
		else
			V_DrawCharacter(x, y, p[c] | cv_constextsize.value | V_NOSCALESTART, true);

		if (c == input_cur && con_tick >= 4)
			V_DrawCharacter(x, y + (con_scalefactor*2), '_' | cv_constextsize.value | V_NOSCALESTART, true);
	}
	if (cend == input_cur && con_tick >= 4)
		V_DrawCharacter(x, y + (con_scalefactor*2), '_' | cv_constextsize.value | V_NOSCALESTART, true);
	if (rellip)
	{
		if (input_sel > cend)
			V_DrawFill(x, y, charwidth*3, (10 * con_scalefactor), 77 | V_NOSCALESTART);
		for (i = 0; i < 3; ++i, x += charwidth)
			V_DrawCharacter(x, y, '.' | cv_constextsize.value | V_GRAYMAP | V_NOSCALESTART, true);
	}
}

// draw the last lines of console text to the top of the screen
static void CON_DrawHudlines(void)
{
	UINT8 *p;
	size_t i;
	INT32 y;
	INT32 charflags = 0;
	INT32 charwidth = 8 * con_scalefactor;
	INT32 charheight = 8 * con_scalefactor;

	if (!con_hudlines)
		return;

	if (chat_on && OLDCHAT)
		y = charheight; // leave place for chat input in the first row of text (only do it if consolechat is on.)
	else
		y = 0;

	for (i = con_cy - con_hudlines; i <= con_cy; i++)
	{
		size_t c;
		INT32 x;

		if ((signed)i < 0)
			continue;
		if (con_hudtime[i%con_hudlines] == 0)
			continue;

		p = (UINT8 *)&con_buffer[(i%con_totallines)*con_width];

		for (c = 0, x = 0; c < con_width; c++, x += charwidth, p++)
		{
			while (*p & 0x80) // Graue 06-19-2004
			{
				charflags = (*p & 0x7f) << V_CHARCOLORSHIFT;
				p++;
				c++;
			}
			if (c >= con_width)
				break;
			if (*p < HU_FONTSTART)
				;//charwidth = 4 * con_scalefactor;
			else
			{
				//charwidth = (hu_font['A'-HU_FONTSTART]->width) * con_scalefactor;
				V_DrawCharacter(x, y, (INT32)(*p) | charflags | cv_constextsize.value | V_NOSCALESTART, true);
			}
		}

		//V_DrawCharacter(x, y, (p[c]&0xff) | cv_constextsize.value | V_NOSCALESTART, true);
		y += charheight;
	}

	// top screen lines that might need clearing when view is reduced
	con_clearlines = y; // this is handled by HU_Erase();
}

// Lactozilla: Draws the console's background picture.
static void CON_DrawBackpic(void)
{
	patch_t *con_backpic;
	lumpnum_t piclump;
	int x, w, h;

	// Get the lumpnum for CONSBACK, STARTUP (Only during game startup) or fallback into MISSING.
	if (con_startup)
		piclump = W_CheckNumForName("STARTUP");
	else
		piclump = W_CheckNumForName("CONSBACK");

	if (piclump == LUMPERROR)
		piclump = W_GetNumForName("MISSING");

	// Cache the patch.
	con_backpic = W_CachePatchNum(piclump, PU_PATCH);

	// Center the backpic, and draw a vertically cropped patch.
	w = con_backpic->width * vid.dup;
	x = (vid.width / 2) - (w / 2);
	h = con_curlines/vid.dup;

	// If the patch doesn't fill the entire screen,
	// then fill the sides with a solid color.
	if (x > 0)
	{
		column_t *column = &con_backpic->columns[0];
		if (column->num_posts && !column->posts[0].topdelta)
		{
			UINT8 *source = column->pixels;
			INT32 color = (source[0] | V_NOSCALESTART);
			// left side
			V_DrawFill(0, 0, x, con_curlines, color);
			// right side
			V_DrawFill((x + w), 0, (vid.width - w), con_curlines, color);
		}
	}

	// Draw the patch.
	V_DrawCroppedPatch(x << FRACBITS, 0, FRACUNIT, FRACUNIT, V_NOSCALESTART, con_backpic, NULL,
			0, (BASEVIDHEIGHT - h) << FRACBITS, BASEVIDWIDTH << FRACBITS, h << FRACBITS);

	// Unlock the cached patch.
	W_UnlockCachedPatch(con_backpic);
}

// draw the console background, text, and prompt if enough place
//
static void CON_DrawConsole(void)
{
	UINT8 *p;
	size_t i;
	INT32 y;
	INT32 charflags = 0;
	INT32 charwidth = (INT32)con_scalefactor << 3;
	INT32 charheight = charwidth;
	INT32 minheight = 20 * con_scalefactor;	// 20 = 8+8+4

	if (con_curlines <= 0)
		return;

	//FIXME: refresh borders only when console bg is translucent
	con_clearlines = con_curlines; // clear console draw from view borders
	con_hudupdate = true; // always refresh while console is on

	// draw console background
	if (cons_backpic.value || con_forcepic)
		CON_DrawBackpic();
	else
	{
		// inu: no more width (was always 0 and vid.width)
		if (rendermode != render_none)
			V_DrawFadeConsBack(con_curlines); // translucent background
	}

	// draw console text lines from top to bottom
	if (con_curlines >= minheight)
	{
		i = con_cy - con_scrollup;

		// skip the last empty line due to the cursor being at the start of a new line
		i--;

		i -= (con_curlines - minheight) / charheight;

		if (rendermode == render_none) return;

		for (y = (con_curlines-minheight) % charheight; y <= con_curlines-minheight; y += charheight, i++)
		{
			INT32 x;
			size_t c;

			p = (UINT8 *)&con_buffer[((i > 0 ? i : 0)%con_totallines)*con_width];

			for (c = 0, x = charwidth; c < con_width; c++, x += charwidth, p++)
			{
				while (*p & 0x80)
				{
					charflags = (*p & 0x7f) << V_CHARCOLORSHIFT;
					p++;
					c++;
				}
				if (c >= con_width)
					break;
				V_DrawCharacter(x, y, (INT32)(*p) | charflags | cv_constextsize.value | V_NOSCALESTART, true);
			}
		}
	}

	// draw prompt if enough place (not while game startup)
	if ((con_curlines >= (minheight-charheight)) && !con_startup)
		CON_DrawInput();
}

// Console refresh drawer, call each frame
//
void CON_Drawer(void)
{
	Lock_state();

	if (!con_started || !graphics_started)
	{
		Unlock_state();
		return;
	}

	if (con_recalc)
	{
		CON_RecalcSize();
		if (con_curlines <= 0)
			CON_ClearHUD();
	}

	// console movement
	if (con_curlines != con_destlines)
		CON_MoveConsole();

	if (con_curlines > 0)
		CON_DrawConsole();
	else if (gamestate == GS_LEVEL || gamestate == GS_INTERMISSION || gamestate == GS_CREDITS
	|| gamestate == GS_EVALUATION || gamestate == GS_ENDING || gamestate == GS_CUTSCENE
	|| gamestate == GS_WAITINGPLAYERS || cv_debug)
		CON_DrawHudlines();

	Unlock_state();
}

// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 2011-2016 by Matthew "Kaito Sinclaire" Walsh.
// Copyright (C) 1999-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  m_menu.h
/// \brief Menu widget stuff, selection and such

#ifndef __X_MENU__
#define __X_MENU__

#include "doomstat.h" // for NUMGAMETYPES
#include "d_event.h"
#include "command.h"
#include "f_finale.h" // for ttmode_enum
#include "i_threads.h"
#include "mserv.h"
#include "r_things.h" // for SKINNAMESIZE

// Compatibility with old-style named NiGHTS replay files.
#define OLDNREPLAYNAME

//
// MENUS
//

// If menu hierarchies go deeper, change this up to 5.
// Zero-based, inclusive.
#define NUMMENULEVELS 3
#define MENUBITS 6

// Menu IDs sectioned by numeric places to signify hierarchy
/**
 * IF YOU MODIFY THIS, MODIFY MENUTYPES_LIST[] IN dehacked.c TO MATCH.
 */
typedef enum
{
	MN_NONE,

	MN_MAIN,

	// Single Player
	MN_SP_MAIN,

	MN_SP_LOAD,
	MN_SP_PLAYER,

	MN_SP_LEVELSELECT,
	MN_SP_LEVELSTATS,

	MN_SP_TIMEATTACK,
	MN_SP_TIMEATTACK_LEVELSELECT,
	MN_SP_GUESTREPLAY,
	MN_SP_REPLAY,
	MN_SP_GHOST,

	MN_SP_NIGHTSATTACK,
	MN_SP_NIGHTS_LEVELSELECT,
	MN_SP_NIGHTS_GUESTREPLAY,
	MN_SP_NIGHTS_REPLAY,
	MN_SP_NIGHTS_GHOST,

	MN_SP_MARATHON,

	// Multiplayer
	MN_MP_MAIN,
	MN_MP_SPLITSCREEN, // SplitServer
	MN_MP_SERVER,
	MN_MP_CONNECT,
	MN_MP_ROOM,
	MN_MP_PLAYERSETUP, // MP_PlayerSetupDef shared with SPLITSCREEN if #defined NONET
	MN_MP_SERVER_OPTIONS,

	// Options
	MN_OP_MAIN,

	MN_OP_P1CONTROLS,
	MN_OP_CHANGECONTROLS, // OP_ChangeControlsDef shared with P2
	MN_OP_P1MOUSE,
	MN_OP_P1JOYSTICK,
	MN_OP_JOYSTICKSET, // OP_JoystickSetDef shared with P2
	MN_OP_P1CAMERA,

	MN_OP_P2CONTROLS,
	MN_OP_P2MOUSE,
	MN_OP_P2JOYSTICK,
	MN_OP_P2CAMERA,

	MN_OP_PLAYSTYLE,

	MN_OP_VIDEO,
	MN_OP_VIDEOMODE,
	MN_OP_COLOR,
	MN_OP_OPENGL,
	MN_OP_OPENGL_LIGHTING,

	MN_OP_SOUND,

	MN_OP_SERVER,
	MN_OP_MONITORTOGGLE,

	MN_OP_DATA,
	MN_OP_ADDONS,
	MN_OP_SCREENSHOTS,
	MN_OP_ERASEDATA,

	// Extras
	MN_SR_MAIN,
	MN_SR_PANDORA,
	MN_SR_LEVELSELECT,
	MN_SR_UNLOCKCHECKLIST,
	MN_SR_EMBLEMHINT,
	MN_SR_PLAYER,
	MN_SR_SOUNDTEST,

	// Addons (Part of MISC, but let's make it our own)
	MN_AD_MAIN,

	// MISC
	// MN_MESSAGE,
	// MN_SPAUSE,

	// MN_MPAUSE,
	// MN_SCRAMBLETEAM,
	// MN_CHANGETEAM,
	// MN_CHANGELEVEL,

	// MN_MAPAUSE,
	// MN_HELP,

	MN_SPECIAL,
	NUMMENUTYPES,
} menutype_t; // up to 63; MN_SPECIAL = 53
#define MTREE2(a,b) (a | (b<<MENUBITS))
#define MTREE3(a,b,c) MTREE2(a, MTREE2(b,c))
#define MTREE4(a,b,c,d) MTREE2(a, MTREE3(b,c,d))

typedef struct
{
	char bgname[8]; // name for background gfx lump; lays over titlemap if this is set
	SINT8 fadestrength;  // darken background when displaying this menu, strength 0-31 or -1 for undefined
	INT32 bgcolor; // fill color, overrides bg name. -1 means follow bg name rules.
	INT32 titlescrollxspeed; // background gfx scroll per menu; inherits global setting
	INT32 titlescrollyspeed; // y scroll
	boolean bghide; // for titlemaps, hide the background.

	SINT8 hidetitlepics; // hide title gfx per menu; -1 means undefined, inherits global setting
	ttmode_enum ttmode; // title wing animation mode; default TTMODE_OLD
	UINT8 ttscale; // scale of title wing gfx (FRACUNIT / ttscale); -1 means undefined, inherits global setting
	char ttname[9]; // lump name of title wing gfx. If name length is <= 6, engine will attempt to load numbered frames (TTNAMExx)
	INT16 ttx; // X position of title wing
	INT16 tty; // Y position of title wing
	INT16 ttloop; // # frame to loop; -1 means dont loop
	UINT16 tttics; // # of tics per frame

	char musname[7]; ///< Music track to play. "" for no music.
	UINT16 mustrack; ///< Subsong to play. Only really relevant for music modules and specific formats supported by GME. 0 to ignore.
	boolean muslooping; ///< Loop the music
	boolean musstop; ///< Don't play any music
	boolean musignore; ///< Let the current music keep playing

	boolean enterbubble; // run all entrance line execs after common ancestor and up to child. If false, only run the child's exec
	boolean exitbubble; // run all exit line execs from child and up to before common ancestor. If false, only run the child's exec
	INT32 entertag; // line exec to run on menu enter, if titlemap
	INT32 exittag; // line exec to run on menu exit, if titlemap
	INT16 enterwipe; // wipe type to run on menu enter, -1 means default
	INT16 exitwipe; // wipe type to run on menu exit, -1 means default
} menupres_t;

extern menupres_t menupres[NUMMENUTYPES];
extern UINT32 prevMenuId;
extern UINT32 activeMenuId;

void M_InitMenuPresTables(void);
UINT8 M_GetYoungestChildMenu(void);
void M_ChangeMenuMusic(const char *defaultmusname, boolean defaultmuslooping);
void M_SetMenuCurBackground(const char *defaultname);
void M_SetMenuCurFadeValue(UINT8 defaultvalue);
void M_SetMenuCurTitlePics(void);

// Called by main loop,
// saves config file and calls I_Quit when user exits.
// Even when the menu is not displayed,
// this can resize the view and change game parameters.
// Does all the real work of the menu interaction.
boolean M_Responder(event_t *ev);

// Called by main loop, only used for menu (skull cursor) animation.
void M_Ticker(void);

// Called by main loop, draws the menus directly into the screen buffer.
void M_Drawer(void);

// Called by D_SRB2Main, loads the config file.
void M_Init(void);

// Called by D_SRB2Main also, sets up the playermenu and description tables.
void M_InitCharacterTables(void);

// Called by intro code to force menu up upon a keypress,
// does nothing if menu is already up.
void M_StartControlPanel(void);

// Called upon end of a mode attack run
void M_EndModeAttackRun(void);

// Called on new server add, or other reasons
void M_SortServerList(void);

// Draws a box with a texture inside as background for messages
void M_DrawTextBox(INT32 x, INT32 y, INT32 width, INT32 boxlines);

// the function to show a message box typing with the string inside
// string must be static (not in the stack)
// routine is a function taking a INT32 in parameter
typedef enum
{
	MM_NOTHING = 0, // is just displayed until the user do someting
	MM_YESNO,       // routine is called with only 'y' or 'n' in param
	MM_EVENTHANDLER // the same of above but without 'y' or 'n' restriction
	                // and routine is void routine(event_t *) (ex: set control)
} menumessagetype_t;
void M_StartMessage(const char *string, void *routine, menumessagetype_t itemtype);

typedef enum
{
	M_NOT_WAITING,

	M_WAITING_VERSION,
	M_WAITING_ROOMS,
	M_WAITING_SERVERS,
}
M_waiting_mode_t;

extern M_waiting_mode_t m_waiting_mode;

// Called by linux_x/i_video_xshm.c
void M_QuitResponse(INT32 ch);

// Determines whether to show a level in the list (platter version does not need to be exposed)
boolean M_CanShowLevelInList(INT32 mapnum, INT32 gt);

// flags for items in the menu
// menu handle (what we do when key is pressed
#define IT_TYPE             15     // (1+2+4+8)
#define IT_CALL              0     // call the function
#define IT_SPACE             1     // no handling
#define IT_ARROWS            2     // call function with 0 for left arrow and 1 for right arrow in param
#define IT_KEYHANDLER        4     // call with the key in param
#define IT_SUBMENU           6     // go to sub menu
#define IT_CVAR              8     // handle as a cvar
#define IT_PAIR             11     // no handling, define both sides of text
#define IT_MSGHANDLER       12     // same as key but with event and sometime can handle y/n key (special for message)

#define IT_DISPLAY   (48+64+128)    // 16+32+64+128
#define IT_NOTHING            0     // space
#define IT_PATCH             16     // a patch or a string with big font
#define IT_STRING            32     // little string (spaced with 10)
#define IT_WHITESTRING       48     // little string in white
#define IT_DYBIGSPACE        64     // same as noting
#define IT_DYLITLSPACE   (16+64)    // little space
#define IT_STRING2       (32+64)    // a simple string
#define IT_GRAYPATCH     (16+32+64) // grayed patch or big font string
#define IT_BIGSLIDER        128     // volume sound use this
#define IT_TRANSTEXT     (16+128)   // Transparent text
#define IT_TRANSTEXT2    (32+128)   // used for control names
#define IT_HEADERTEXT    (48+128)   // Non-selectable header option, displays in yellow offset to the left a little
#define IT_QUESTIONMARKS (64+128)   // Displays as question marks, used for secrets
#define IT_CENTER           256     // if IT_PATCH, center it on screen

//consvar specific
#define IT_CVARTYPE   (512+1024+2048)
#define IT_CV_NORMAL         0
#define IT_CV_SLIDER       512
#define IT_CV_STRING      1024
#define IT_CV_NOPRINT     1536
#define IT_CV_NOMOD       2048
#define IT_CV_INVISSLIDER 2560
#define IT_CV_INTEGERSTEP 4096      // if IT_CV_NORMAL and cvar is CV_FLOAT, modify it by 1 instead of 0.0625
#define IT_CV_FLOATSLIDER 4608      // IT_CV_SLIDER, value modified by 0.0625 instead of 1 (for CV_FLOAT cvars)

//call/submenu specific
// There used to be a lot more here but ...
// A lot of them became redundant with the advent of the Pause menu, so they were removed
#define IT_CALLTYPE   (512+1024)
#define IT_CALL_NORMAL          0
#define IT_CALL_NOTMODIFIED   512

// in INT16 for some common use
#define IT_BIGSPACE    (IT_SPACE  +IT_DYBIGSPACE)
#define IT_LITLSPACE   (IT_SPACE  +IT_DYLITLSPACE)
#define IT_CONTROL     (IT_STRING2+IT_CALL)
#define IT_CVARMAX     (IT_CVAR   +IT_CV_NOMOD)
#define IT_DISABLED    (IT_SPACE  +IT_GRAYPATCH)
#define IT_GRAYEDOUT   (IT_SPACE  +IT_TRANSTEXT)
#define IT_GRAYEDOUT2  (IT_SPACE  +IT_TRANSTEXT2)
#define IT_HEADER      (IT_SPACE  +IT_HEADERTEXT)
#define IT_SECRET      (IT_SPACE  +IT_QUESTIONMARKS)

#define MAXSTRINGLENGTH 32

typedef union
{
	struct menu_s *submenu;      // IT_SUBMENU
	consvar_t *cvar;             // IT_CVAR
	void (*routine)(INT32 choice); // IT_CALL, IT_KEYHANDLER, IT_ARROWS
} itemaction_t;

//
// MENU TYPEDEFS
//
typedef struct menuitem_s
{
	// show IT_xxx
	UINT16 status;

	const char *patch;
	const char *text; // used when FONTBxx lump is found

// FIXME: should be itemaction_t
	void *itemaction;

	// hotkey in menu or y of the item
	UINT16 alphaKey;
} menuitem_t;

extern menuitem_t MP_RoomMenu[];
extern UINT32     roomIds[NUM_LIST_ROOMS];

typedef struct menu_s
{
	UINT32         menuid;             // ID to encode menu type and hierarchy
	const char    *menutitlepic;
	INT16          numitems;           // # of menu items
	struct menu_s *prevMenu;           // previous menu
	menuitem_t    *menuitems;          // menu items
	void         (*drawroutine)(void); // draw routine
	INT16          x, y;               // x, y of menu
	INT16          lastOn;             // last item user was on in menu
	boolean      (*quitroutine)(void); // called before quit a menu return true if we can
} menu_t;

void M_SetupNextMenu(menu_t *menudef);
void M_ClearMenus(boolean callexitmenufunc);

// Maybe this goes here????? Who knows.
boolean M_MouseNeeded(void);

#ifdef HAVE_THREADS
extern I_mutex m_menu_mutex;
#endif

extern menu_t *currentMenu;

extern menu_t MainDef;
extern menu_t SP_LoadDef;

// Call upon joystick hotplug
void M_SetupJoystickMenu(INT32 choice);
extern menu_t OP_JoystickSetDef;

// Stuff for customizing the player select screen
typedef struct
{
	boolean used;
	char notes[441];
	char picname[8];
	char skinname[SKINNAMESIZE*2+2]; // skin&skin\0
	patch_t *charpic;
	UINT8 prev;
	UINT8 next;

	// new character select
	char displayname[SKINNAMESIZE+1];
	SINT8 skinnum[2];
	UINT16 oppositecolor;
	char nametag[8];
	patch_t *namepic;
	UINT16 tagtextcolor;
	UINT16 tagoutlinecolor;
} description_t;

// level select platter
typedef struct
{
	char header[22+5]; // mapheader_t lvltttl max length + " ZONE"
	INT32 maplist[3];
	char mapnames[3][17+1];
	boolean mapavailable[4]; // mapavailable[3] == wide or not
} levelselectrow_t;

typedef struct
{
	UINT8 numrows;
	levelselectrow_t *rows;
} levelselect_t;
// experimental level select end

// descriptions for gametype select screen
typedef struct
{
	UINT8 col[2];
	char notes[441];
} gtdesc_t;
extern gtdesc_t gametypedesc[NUMGAMETYPES];

// mode descriptions for video mode menu
typedef struct
{
	INT32 modenum; // video mode number in the vidmodes list
	const char *desc;  // XXXxYYY
	UINT8 goodratio; // aspect correct if 1
} modedesc_t;

// savegame struct for save game menu
typedef struct
{
	char levelname[32];
	UINT8 skinnum;
	UINT8 botskin;
	UINT8 numemeralds;
	UINT8 numgameovers;
	INT32 lives;
	INT32 continuescore;
	INT32 gamemap;
} saveinfo_t;

extern description_t description[MAXSKINS];

extern consvar_t cv_showfocuslost;
extern consvar_t cv_newgametype, cv_nextmap, cv_chooseskin, cv_serversort;
extern CV_PossibleValue_t gametype_cons_t[];

extern INT16 startmap;
extern INT32 ultimate_selectable;
extern INT16 char_on, startchar;

#define MAXSAVEGAMES 31
#define NOSAVESLOT 0 //slot where Play Without Saving appears
#define MARATHONSLOT 420 // just has to be nonzero, but let's use one that'll show up as an obvious error if something goes wrong while not using our existing saves

#define BwehHehHe() S_StartSound(NULL, sfx_bewar1+M_RandomKey(4)) // Bweh heh he

void M_TutorialSaveControlResponse(INT32 ch);

void M_ForceSaveSlotSelected(INT32 sslot);

void M_CheatActivationResponder(INT32 ch);

void M_ModeAttackRetry(INT32 choice);

// Level select updating
void Nextmap_OnChange(void);

// Screenshot menu updating
void Moviemode_mode_Onchange(void);
void Screenshot_option_Onchange(void);

// Addons menu updating
void Addons_option_Onchange(void);

// Moviemode menu updating
void Moviemode_option_Onchange(void);

// Player Setup menu colors linked list
typedef struct menucolor_s {
	struct menucolor_s *next;
	struct menucolor_s *prev;
	UINT16 color;
} menucolor_t;

extern menucolor_t *menucolorhead, *menucolortail;

void M_AddMenuColor(UINT16 color);
void M_MoveColorBefore(UINT16 color, UINT16 targ);
void M_MoveColorAfter(UINT16 color, UINT16 targ);
UINT16 M_GetColorBefore(UINT16 color);
UINT16 M_GetColorAfter(UINT16 color);
void M_InitPlayerSetupColors(void);
void M_FreePlayerSetupColors(void);

// These defines make it a little easier to make menus
#define DEFAULTMENUSTYLE(id, header, source, prev, x, y)\
{\
	id,\
	header,\
	sizeof(source)/sizeof(menuitem_t),\
	prev,\
	source,\
	M_DrawGenericMenu,\
	x, y,\
	0,\
	NULL\
}

#define DEFAULTSCROLLMENUSTYLE(id, header, source, prev, x, y)\
{\
	id,\
	header,\
	sizeof(source)/sizeof(menuitem_t),\
	prev,\
	source,\
	M_DrawGenericScrollMenu,\
	x, y,\
	0,\
	NULL\
}

#define PAUSEMENUSTYLE(source, x, y)\
{\
	MN_SPECIAL,\
	NULL,\
	sizeof(source)/sizeof(menuitem_t),\
	NULL,\
	source,\
	M_DrawPauseMenu,\
	x, y,\
	0,\
	NULL\
}

#define CENTERMENUSTYLE(id, header, source, prev, y)\
{\
	id,\
	header,\
	sizeof(source)/sizeof(menuitem_t),\
	prev,\
	source,\
	M_DrawCenteredMenu,\
	BASEVIDWIDTH/2, y,\
	0,\
	NULL\
}

#define MAPPLATTERMENUSTYLE(id, header, source)\
{\
	id,\
	header,\
	sizeof (source)/sizeof (menuitem_t),\
	&MainDef,\
	source,\
	M_DrawLevelPlatterMenu,\
	0,0,\
	0,\
	NULL\
}

#define CONTROLMENUSTYLE(id, source, prev)\
{\
	id,\
	"M_CONTRO",\
	sizeof (source)/sizeof (menuitem_t),\
	prev,\
	source,\
	M_DrawControl,\
	24, 40,\
	0,\
	NULL\
}

#define IMAGEDEF(source)\
{\
	MN_SPECIAL,\
	NULL,\
	sizeof (source)/sizeof (menuitem_t),\
	NULL,\
	source,\
	M_DrawImageDef,\
	0, 0,\
	0,\
	NULL\
}

#endif //__X_MENU__

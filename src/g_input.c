// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2023 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  g_input.c
/// \brief handle mouse/keyboard/joystick inputs,
///        maps inputs to game controls (forward, spin, jump...)

#include "doomdef.h"
#include "doomstat.h"
#include "g_input.h"
#include "keys.h"
#include "hu_stuff.h" // need HUFONT start & end
#include "d_net.h"
#include "console.h"

#define MAXMOUSESENSITIVITY 100 // sensitivity steps

static CV_PossibleValue_t mousesens_cons_t[] = {{1, "MIN"}, {MAXMOUSESENSITIVITY, "MAX"}, {0, NULL}};
static CV_PossibleValue_t onecontrolperkey_cons_t[] = {{1, "One"}, {2, "Several"}, {0, NULL}};

// mouse values are used once
consvar_t cv_mousesens = CVAR_INIT ("mousesens", "20", CV_SAVE, mousesens_cons_t, NULL);
consvar_t cv_mousesens2 = CVAR_INIT ("mousesens2", "20", CV_SAVE, mousesens_cons_t, NULL);
consvar_t cv_mouseysens = CVAR_INIT ("mouseysens", "20", CV_SAVE, mousesens_cons_t, NULL);
consvar_t cv_mouseysens2 = CVAR_INIT ("mouseysens2", "20", CV_SAVE, mousesens_cons_t, NULL);
consvar_t cv_controlperkey = CVAR_INIT ("controlperkey", "One", CV_SAVE, onecontrolperkey_cons_t, NULL);

mouse_t mouse;
mouse_t mouse2;

// joystick values are repeated
INT32 joyxmove[JOYAXISSET], joyymove[JOYAXISSET], joy2xmove[JOYAXISSET], joy2ymove[JOYAXISSET];

// current state of the keys: true if pushed
UINT8 gamekeydown[NUMINPUTS];

// two key codes (or virtual key) per game control
INT32 gamecontrol[NUM_GAMECONTROLS][2];
INT32 gamecontrolbis[NUM_GAMECONTROLS][2]; // secondary splitscreen player
INT32 gamecontroldefault[num_gamecontrolschemes][NUM_GAMECONTROLS][2]; // default control storage, use 0 (gcs_custom) for memory retention
INT32 gamecontrolbisdefault[num_gamecontrolschemes][NUM_GAMECONTROLS][2];

// lists of GC codes for selective operation
const INT32 gcl_tutorial_check[num_gcl_tutorial_check] = {
	GC_FORWARD, GC_BACKWARD, GC_STRAFELEFT, GC_STRAFERIGHT,
	GC_TURNLEFT, GC_TURNRIGHT
};

const INT32 gcl_tutorial_used[num_gcl_tutorial_used] = {
	GC_FORWARD, GC_BACKWARD, GC_STRAFELEFT, GC_STRAFERIGHT,
	GC_TURNLEFT, GC_TURNRIGHT,
	GC_JUMP, GC_SPIN
};

const INT32 gcl_tutorial_full[num_gcl_tutorial_full] = {
	GC_FORWARD, GC_BACKWARD, GC_STRAFELEFT, GC_STRAFERIGHT,
	GC_LOOKUP, GC_LOOKDOWN, GC_TURNLEFT, GC_TURNRIGHT, GC_CENTERVIEW,
	GC_JUMP, GC_SPIN,
	GC_FIRE, GC_FIRENORMAL
};

const INT32 gcl_movement[num_gcl_movement] = {
	GC_FORWARD, GC_BACKWARD, GC_STRAFELEFT, GC_STRAFERIGHT
};

const INT32 gcl_camera[num_gcl_camera] = {
	GC_TURNLEFT, GC_TURNRIGHT
};

const INT32 gcl_movement_camera[num_gcl_movement_camera] = {
	GC_FORWARD, GC_BACKWARD, GC_STRAFELEFT, GC_STRAFERIGHT,
	GC_TURNLEFT, GC_TURNRIGHT
};

const INT32 gcl_jump[num_gcl_jump] = { GC_JUMP };

const INT32 gcl_spin[num_gcl_spin] = { GC_SPIN };

const INT32 gcl_jump_spin[num_gcl_jump_spin] = {
	GC_JUMP, GC_SPIN
};

typedef struct
{
	UINT8 time;
	UINT8 state;
	UINT8 clicks;
} dclick_t;
static dclick_t mousedclicks[MOUSEBUTTONS];
static dclick_t joydclicks[JOYBUTTONS + JOYHATS*4];
static dclick_t mouse2dclicks[MOUSEBUTTONS];
static dclick_t joy2dclicks[JOYBUTTONS + JOYHATS*4];

// protos
static UINT8 G_CheckDoubleClick(UINT8 state, dclick_t *dt);

//
// Remaps the inputs to game controls.
//
// A game control can be triggered by one or more keys/buttons.
//
// Each key/mousebutton/joybutton triggers ONLY ONE game control.
//
void G_MapEventsToControls(event_t *ev)
{
	INT32 i;
	UINT8 flag;

	switch (ev->type)
	{
		case ev_keydown:
			if (ev->key < NUMINPUTS)
				gamekeydown[ev->key] = 1;
#ifdef PARANOIA
			else
			{
				CONS_Debug(DBG_GAMELOGIC, "Bad downkey input %d\n",ev->key);
			}

#endif
			break;

		case ev_keyup:
			if (ev->key < NUMINPUTS)
				gamekeydown[ev->key] = 0;
#ifdef PARANOIA
			else
			{
				CONS_Debug(DBG_GAMELOGIC, "Bad upkey input %d\n",ev->key);
			}
#endif
			break;

		case ev_mouse: // buttons are virtual keys
			mouse.rdx = ev->x;
			mouse.rdy = ev->y;
			break;

		case ev_joystick: // buttons are virtual keys
			i = ev->key;
			if (i >= JOYAXISSET || menuactive || CON_Ready() || chat_on)
				break;
			if (ev->x != INT32_MAX) joyxmove[i] = ev->x;
			if (ev->y != INT32_MAX) joyymove[i] = ev->y;
			break;

		case ev_joystick2: // buttons are virtual keys
			i = ev->key;
			if (i >= JOYAXISSET || menuactive || CON_Ready() || chat_on)
				break;
			if (ev->x != INT32_MAX) joy2xmove[i] = ev->x;
			if (ev->y != INT32_MAX) joy2ymove[i] = ev->y;
			break;

		case ev_mouse2: // buttons are virtual keys
			if (menuactive || CON_Ready() || chat_on)
				break;
			mouse2.rdx = ev->x;
			mouse2.rdy = ev->y;
			break;

		default:
			break;
	}

	// ALWAYS check for mouse & joystick double-clicks even if no mouse event
	for (i = 0; i < MOUSEBUTTONS; i++)
	{
		flag = G_CheckDoubleClick(gamekeydown[KEY_MOUSE1+i], &mousedclicks[i]);
		gamekeydown[KEY_DBLMOUSE1+i] = flag;
	}

	for (i = 0; i < JOYBUTTONS + JOYHATS*4; i++)
	{
		flag = G_CheckDoubleClick(gamekeydown[KEY_JOY1+i], &joydclicks[i]);
		gamekeydown[KEY_DBLJOY1+i] = flag;
	}

	for (i = 0; i < MOUSEBUTTONS; i++)
	{
		flag = G_CheckDoubleClick(gamekeydown[KEY_2MOUSE1+i], &mouse2dclicks[i]);
		gamekeydown[KEY_DBL2MOUSE1+i] = flag;
	}

	for (i = 0; i < JOYBUTTONS + JOYHATS*4; i++)
	{
		flag = G_CheckDoubleClick(gamekeydown[KEY_2JOY1+i], &joy2dclicks[i]);
		gamekeydown[KEY_DBL2JOY1+i] = flag;
	}
}

//
// General double-click detection routine for any kind of input.
//
static UINT8 G_CheckDoubleClick(UINT8 state, dclick_t *dt)
{
	if (state != dt->state && dt->time > 1)
	{
		dt->state = state;
		if (state)
			dt->clicks++;
		if (dt->clicks == 2)
		{
			dt->clicks = 0;
			return true;
		}
		else
			dt->time = 0;
	}
	else
	{
		dt->time++;
		if (dt->time > 20)
		{
			dt->clicks = 0;
			dt->state = 0;
		}
	}
	return false;
}

typedef struct
{
	INT32 keynum;
	const char *name;
} keyname_t;

static keyname_t keynames[] =
{
	{KEY_SPACE, "space"},
	{KEY_CAPSLOCK, "caps lock"},
	{KEY_ENTER, "enter"},
	{KEY_TAB, "tab"},
	{KEY_ESCAPE, "escape"},
	{KEY_BACKSPACE, "backspace"},

	{KEY_NUMLOCK, "numlock"},
	{KEY_SCROLLLOCK, "scrolllock"},

	// bill gates keys
	{KEY_LEFTWIN, "leftwin"},
	{KEY_RIGHTWIN, "rightwin"},
	{KEY_MENU, "menu"},

	{KEY_LSHIFT, "lshift"},
	{KEY_RSHIFT, "rshift"},
	{KEY_LSHIFT, "shift"},
	{KEY_LCTRL, "lctrl"},
	{KEY_RCTRL, "rctrl"},
	{KEY_LCTRL, "ctrl"},
	{KEY_LALT, "lalt"},
	{KEY_RALT, "ralt"},
	{KEY_LALT, "alt"},

	// keypad keys
	{KEY_KPADSLASH, "keypad /"},
	{KEY_KEYPAD7, "keypad 7"},
	{KEY_KEYPAD8, "keypad 8"},
	{KEY_KEYPAD9, "keypad 9"},
	{KEY_MINUSPAD, "keypad -"},
	{KEY_KEYPAD4, "keypad 4"},
	{KEY_KEYPAD5, "keypad 5"},
	{KEY_KEYPAD6, "keypad 6"},
	{KEY_PLUSPAD, "keypad +"},
	{KEY_KEYPAD1, "keypad 1"},
	{KEY_KEYPAD2, "keypad 2"},
	{KEY_KEYPAD3, "keypad 3"},
	{KEY_KEYPAD0, "keypad 0"},
	{KEY_KPADDEL, "keypad ."},

	// extended keys (not keypad)
	{KEY_HOME, "home"},
	{KEY_UPARROW, "up arrow"},
	{KEY_PGUP, "pgup"},
	{KEY_LEFTARROW, "left arrow"},
	{KEY_RIGHTARROW, "right arrow"},
	{KEY_END, "end"},
	{KEY_DOWNARROW, "down arrow"},
	{KEY_PGDN, "pgdn"},
	{KEY_INS, "ins"},
	{KEY_DEL, "del"},

	// other keys
	{KEY_F1, "f1"},
	{KEY_F2, "f2"},
	{KEY_F3, "f3"},
	{KEY_F4, "f4"},
	{KEY_F5, "f5"},
	{KEY_F6, "f6"},
	{KEY_F7, "f7"},
	{KEY_F8, "f8"},
	{KEY_F9, "f9"},
	{KEY_F10, "f10"},
	{KEY_F11, "f11"},
	{KEY_F12, "f12"},

	// KEY_CONSOLE has an exception in the keyname code
	{'`', "TILDE"},
	{KEY_PAUSE, "pause/break"},

	// virtual keys for mouse buttons and joystick buttons
	{KEY_MOUSE1+0,"mouse1"},
	{KEY_MOUSE1+1,"mouse2"},
	{KEY_MOUSE1+2,"mouse3"},
	{KEY_MOUSE1+3,"mouse4"},
	{KEY_MOUSE1+4,"mouse5"},
	{KEY_MOUSE1+5,"mouse6"},
	{KEY_MOUSE1+6,"mouse7"},
	{KEY_MOUSE1+7,"mouse8"},
	{KEY_2MOUSE1+0,"sec_mouse2"}, // BP: sorry my mouse handler swap button 1 and 2
	{KEY_2MOUSE1+1,"sec_mouse1"},
	{KEY_2MOUSE1+2,"sec_mouse3"},
	{KEY_2MOUSE1+3,"sec_mouse4"},
	{KEY_2MOUSE1+4,"sec_mouse5"},
	{KEY_2MOUSE1+5,"sec_mouse6"},
	{KEY_2MOUSE1+6,"sec_mouse7"},
	{KEY_2MOUSE1+7,"sec_mouse8"},
	{KEY_MOUSEWHEELUP, "wheel 1 up"},
	{KEY_MOUSEWHEELDOWN, "wheel 1 down"},
	{KEY_2MOUSEWHEELUP, "wheel 2 up"},
	{KEY_2MOUSEWHEELDOWN, "wheel 2 down"},

	{KEY_JOY1+0, "joy1"},
	{KEY_JOY1+1, "joy2"},
	{KEY_JOY1+2, "joy3"},
	{KEY_JOY1+3, "joy4"},
	{KEY_JOY1+4, "joy5"},
	{KEY_JOY1+5, "joy6"},
	{KEY_JOY1+6, "joy7"},
	{KEY_JOY1+7, "joy8"},
	{KEY_JOY1+8, "joy9"},
#if !defined (NOMOREJOYBTN_1S)
	// we use up to 32 buttons in DirectInput
	{KEY_JOY1+9, "joy10"},
	{KEY_JOY1+10, "joy11"},
	{KEY_JOY1+11, "joy12"},
	{KEY_JOY1+12, "joy13"},
	{KEY_JOY1+13, "joy14"},
	{KEY_JOY1+14, "joy15"},
	{KEY_JOY1+15, "joy16"},
	{KEY_JOY1+16, "joy17"},
	{KEY_JOY1+17, "joy18"},
	{KEY_JOY1+18, "joy19"},
	{KEY_JOY1+19, "joy20"},
	{KEY_JOY1+20, "joy21"},
	{KEY_JOY1+21, "joy22"},
	{KEY_JOY1+22, "joy23"},
	{KEY_JOY1+23, "joy24"},
	{KEY_JOY1+24, "joy25"},
	{KEY_JOY1+25, "joy26"},
	{KEY_JOY1+26, "joy27"},
	{KEY_JOY1+27, "joy28"},
	{KEY_JOY1+28, "joy29"},
	{KEY_JOY1+29, "joy30"},
	{KEY_JOY1+30, "joy31"},
	{KEY_JOY1+31, "joy32"},
#endif
	// the DOS version uses Allegro's joystick support
	{KEY_HAT1+0, "hatup"},
	{KEY_HAT1+1, "hatdown"},
	{KEY_HAT1+2, "hatleft"},
	{KEY_HAT1+3, "hatright"},
	{KEY_HAT1+4, "hatup2"},
	{KEY_HAT1+5, "hatdown2"},
	{KEY_HAT1+6, "hatleft2"},
	{KEY_HAT1+7, "hatright2"},
	{KEY_HAT1+8, "hatup3"},
	{KEY_HAT1+9, "hatdown3"},
	{KEY_HAT1+10, "hatleft3"},
	{KEY_HAT1+11, "hatright3"},
	{KEY_HAT1+12, "hatup4"},
	{KEY_HAT1+13, "hatdown4"},
	{KEY_HAT1+14, "hatleft4"},
	{KEY_HAT1+15, "hatright4"},

	{KEY_DBLMOUSE1+0, "dblmouse1"},
	{KEY_DBLMOUSE1+1, "dblmouse2"},
	{KEY_DBLMOUSE1+2, "dblmouse3"},
	{KEY_DBLMOUSE1+3, "dblmouse4"},
	{KEY_DBLMOUSE1+4, "dblmouse5"},
	{KEY_DBLMOUSE1+5, "dblmouse6"},
	{KEY_DBLMOUSE1+6, "dblmouse7"},
	{KEY_DBLMOUSE1+7, "dblmouse8"},
	{KEY_DBL2MOUSE1+0, "dblsec_mouse2"}, // BP: sorry my mouse handler swap button 1 and 2
	{KEY_DBL2MOUSE1+1, "dblsec_mouse1"},
	{KEY_DBL2MOUSE1+2, "dblsec_mouse3"},
	{KEY_DBL2MOUSE1+3, "dblsec_mouse4"},
	{KEY_DBL2MOUSE1+4, "dblsec_mouse5"},
	{KEY_DBL2MOUSE1+5, "dblsec_mouse6"},
	{KEY_DBL2MOUSE1+6, "dblsec_mouse7"},
	{KEY_DBL2MOUSE1+7, "dblsec_mouse8"},

	{KEY_DBLJOY1+0, "dbljoy1"},
	{KEY_DBLJOY1+1, "dbljoy2"},
	{KEY_DBLJOY1+2, "dbljoy3"},
	{KEY_DBLJOY1+3, "dbljoy4"},
	{KEY_DBLJOY1+4, "dbljoy5"},
	{KEY_DBLJOY1+5, "dbljoy6"},
	{KEY_DBLJOY1+6, "dbljoy7"},
	{KEY_DBLJOY1+7, "dbljoy8"},
#if !defined (NOMOREJOYBTN_1DBL)
	{KEY_DBLJOY1+8, "dbljoy9"},
	{KEY_DBLJOY1+9, "dbljoy10"},
	{KEY_DBLJOY1+10, "dbljoy11"},
	{KEY_DBLJOY1+11, "dbljoy12"},
	{KEY_DBLJOY1+12, "dbljoy13"},
	{KEY_DBLJOY1+13, "dbljoy14"},
	{KEY_DBLJOY1+14, "dbljoy15"},
	{KEY_DBLJOY1+15, "dbljoy16"},
	{KEY_DBLJOY1+16, "dbljoy17"},
	{KEY_DBLJOY1+17, "dbljoy18"},
	{KEY_DBLJOY1+18, "dbljoy19"},
	{KEY_DBLJOY1+19, "dbljoy20"},
	{KEY_DBLJOY1+20, "dbljoy21"},
	{KEY_DBLJOY1+21, "dbljoy22"},
	{KEY_DBLJOY1+22, "dbljoy23"},
	{KEY_DBLJOY1+23, "dbljoy24"},
	{KEY_DBLJOY1+24, "dbljoy25"},
	{KEY_DBLJOY1+25, "dbljoy26"},
	{KEY_DBLJOY1+26, "dbljoy27"},
	{KEY_DBLJOY1+27, "dbljoy28"},
	{KEY_DBLJOY1+28, "dbljoy29"},
	{KEY_DBLJOY1+29, "dbljoy30"},
	{KEY_DBLJOY1+30, "dbljoy31"},
	{KEY_DBLJOY1+31, "dbljoy32"},
#endif
	{KEY_DBLHAT1+0, "dblhatup"},
	{KEY_DBLHAT1+1, "dblhatdown"},
	{KEY_DBLHAT1+2, "dblhatleft"},
	{KEY_DBLHAT1+3, "dblhatright"},
	{KEY_DBLHAT1+4, "dblhatup2"},
	{KEY_DBLHAT1+5, "dblhatdown2"},
	{KEY_DBLHAT1+6, "dblhatleft2"},
	{KEY_DBLHAT1+7, "dblhatright2"},
	{KEY_DBLHAT1+8, "dblhatup3"},
	{KEY_DBLHAT1+9, "dblhatdown3"},
	{KEY_DBLHAT1+10, "dblhatleft3"},
	{KEY_DBLHAT1+11, "dblhatright3"},
	{KEY_DBLHAT1+12, "dblhatup4"},
	{KEY_DBLHAT1+13, "dblhatdown4"},
	{KEY_DBLHAT1+14, "dblhatleft4"},
	{KEY_DBLHAT1+15, "dblhatright4"},

	{KEY_2JOY1+0, "sec_joy1"},
	{KEY_2JOY1+1, "sec_joy2"},
	{KEY_2JOY1+2, "sec_joy3"},
	{KEY_2JOY1+3, "sec_joy4"},
	{KEY_2JOY1+4, "sec_joy5"},
	{KEY_2JOY1+5, "sec_joy6"},
	{KEY_2JOY1+6, "sec_joy7"},
	{KEY_2JOY1+7, "sec_joy8"},
#if !defined (NOMOREJOYBTN_2S)
	// we use up to 32 buttons in DirectInput
	{KEY_2JOY1+8, "sec_joy9"},
	{KEY_2JOY1+9, "sec_joy10"},
	{KEY_2JOY1+10, "sec_joy11"},
	{KEY_2JOY1+11, "sec_joy12"},
	{KEY_2JOY1+12, "sec_joy13"},
	{KEY_2JOY1+13, "sec_joy14"},
	{KEY_2JOY1+14, "sec_joy15"},
	{KEY_2JOY1+15, "sec_joy16"},
	{KEY_2JOY1+16, "sec_joy17"},
	{KEY_2JOY1+17, "sec_joy18"},
	{KEY_2JOY1+18, "sec_joy19"},
	{KEY_2JOY1+19, "sec_joy20"},
	{KEY_2JOY1+20, "sec_joy21"},
	{KEY_2JOY1+21, "sec_joy22"},
	{KEY_2JOY1+22, "sec_joy23"},
	{KEY_2JOY1+23, "sec_joy24"},
	{KEY_2JOY1+24, "sec_joy25"},
	{KEY_2JOY1+25, "sec_joy26"},
	{KEY_2JOY1+26, "sec_joy27"},
	{KEY_2JOY1+27, "sec_joy28"},
	{KEY_2JOY1+28, "sec_joy29"},
	{KEY_2JOY1+29, "sec_joy30"},
	{KEY_2JOY1+30, "sec_joy31"},
	{KEY_2JOY1+31, "sec_joy32"},
#endif
	// the DOS version uses Allegro's joystick support
	{KEY_2HAT1+0,  "sec_hatup"},
	{KEY_2HAT1+1,  "sec_hatdown"},
	{KEY_2HAT1+2,  "sec_hatleft"},
	{KEY_2HAT1+3,  "sec_hatright"},
	{KEY_2HAT1+4, "sec_hatup2"},
	{KEY_2HAT1+5, "sec_hatdown2"},
	{KEY_2HAT1+6, "sec_hatleft2"},
	{KEY_2HAT1+7, "sec_hatright2"},
	{KEY_2HAT1+8, "sec_hatup3"},
	{KEY_2HAT1+9, "sec_hatdown3"},
	{KEY_2HAT1+10, "sec_hatleft3"},
	{KEY_2HAT1+11, "sec_hatright3"},
	{KEY_2HAT1+12, "sec_hatup4"},
	{KEY_2HAT1+13, "sec_hatdown4"},
	{KEY_2HAT1+14, "sec_hatleft4"},
	{KEY_2HAT1+15, "sec_hatright4"},

	{KEY_DBL2JOY1+0, "dblsec_joy1"},
	{KEY_DBL2JOY1+1, "dblsec_joy2"},
	{KEY_DBL2JOY1+2, "dblsec_joy3"},
	{KEY_DBL2JOY1+3, "dblsec_joy4"},
	{KEY_DBL2JOY1+4, "dblsec_joy5"},
	{KEY_DBL2JOY1+5, "dblsec_joy6"},
	{KEY_DBL2JOY1+6, "dblsec_joy7"},
	{KEY_DBL2JOY1+7, "dblsec_joy8"},
#if !defined (NOMOREJOYBTN_2DBL)
	{KEY_DBL2JOY1+8, "dblsec_joy9"},
	{KEY_DBL2JOY1+9, "dblsec_joy10"},
	{KEY_DBL2JOY1+10, "dblsec_joy11"},
	{KEY_DBL2JOY1+11, "dblsec_joy12"},
	{KEY_DBL2JOY1+12, "dblsec_joy13"},
	{KEY_DBL2JOY1+13, "dblsec_joy14"},
	{KEY_DBL2JOY1+14, "dblsec_joy15"},
	{KEY_DBL2JOY1+15, "dblsec_joy16"},
	{KEY_DBL2JOY1+16, "dblsec_joy17"},
	{KEY_DBL2JOY1+17, "dblsec_joy18"},
	{KEY_DBL2JOY1+18, "dblsec_joy19"},
	{KEY_DBL2JOY1+19, "dblsec_joy20"},
	{KEY_DBL2JOY1+20, "dblsec_joy21"},
	{KEY_DBL2JOY1+21, "dblsec_joy22"},
	{KEY_DBL2JOY1+22, "dblsec_joy23"},
	{KEY_DBL2JOY1+23, "dblsec_joy24"},
	{KEY_DBL2JOY1+24, "dblsec_joy25"},
	{KEY_DBL2JOY1+25, "dblsec_joy26"},
	{KEY_DBL2JOY1+26, "dblsec_joy27"},
	{KEY_DBL2JOY1+27, "dblsec_joy28"},
	{KEY_DBL2JOY1+28, "dblsec_joy29"},
	{KEY_DBL2JOY1+29, "dblsec_joy30"},
	{KEY_DBL2JOY1+30, "dblsec_joy31"},
	{KEY_DBL2JOY1+31, "dblsec_joy32"},
#endif
	{KEY_DBL2HAT1+0, "dblsec_hatup"},
	{KEY_DBL2HAT1+1, "dblsec_hatdown"},
	{KEY_DBL2HAT1+2, "dblsec_hatleft"},
	{KEY_DBL2HAT1+3, "dblsec_hatright"},
	{KEY_DBL2HAT1+4, "dblsec_hatup2"},
	{KEY_DBL2HAT1+5, "dblsec_hatdown2"},
	{KEY_DBL2HAT1+6, "dblsec_hatleft2"},
	{KEY_DBL2HAT1+7, "dblsec_hatright2"},
	{KEY_DBL2HAT1+8, "dblsec_hatup3"},
	{KEY_DBL2HAT1+9, "dblsec_hatdown3"},
	{KEY_DBL2HAT1+10, "dblsec_hatleft3"},
	{KEY_DBL2HAT1+11, "dblsec_hatright3"},
	{KEY_DBL2HAT1+12, "dblsec_hatup4"},
	{KEY_DBL2HAT1+13, "dblsec_hatdown4"},
	{KEY_DBL2HAT1+14, "dblsec_hatleft4"},
	{KEY_DBL2HAT1+15, "dblsec_hatright4"},

};

static const char *gamecontrolname[NUM_GAMECONTROLS] =
{
	"nothing", // a key/button mapped to GC_NULL has no effect
	"forward",
	"backward",
	"strafeleft",
	"straferight",
	"turnleft",
	"turnright",
	"weaponnext",
	"weaponprev",
	"weapon1",
	"weapon2",
	"weapon3",
	"weapon4",
	"weapon5",
	"weapon6",
	"weapon7",
	"weapon8",
	"weapon9",
	"weapon10",
	"fire",
	"firenormal",
	"tossflag",
	"spin",
	"camtoggle",
	"camreset",
	"lookup",
	"lookdown",
	"centerview",
	"mouseaiming",
	"talkkey",
	"teamtalkkey",
	"scores",
	"jump",
	"console",
	"pause",
	"systemmenu",
	"screenshot",
	"recordgif",
	"viewpoint", // Rename this to "viewpointnext" for the next major version
	"viewpointprev",
	"custom1",
	"custom2",
	"custom3",
};

#define NUMKEYNAMES (sizeof (keynames)/sizeof (keyname_t))

//
// Detach any keys associated to the given game control
// - pass the pointer to the gamecontrol table for the player being edited
void G_ClearControlKeys(INT32 (*setupcontrols)[2], INT32 control)
{
	setupcontrols[control][0] = KEY_NULL;
	setupcontrols[control][1] = KEY_NULL;
}

void G_ClearAllControlKeys(void)
{
	INT32 i;
	for (i = 0; i < NUM_GAMECONTROLS; i++)
	{
		G_ClearControlKeys(gamecontrol, i);
		G_ClearControlKeys(gamecontrolbis, i);
	}
}

//
// Returns the name of a key (or virtual key for mouse and joy)
// the input value being an keynum
//
const char *G_KeyNumToName(INT32 keynum)
{
	static char keynamestr[8];

	UINT32 j;

	// return a string with the ascii char if displayable
	if (keynum > ' ' && keynum <= 'z' && keynum != KEY_CONSOLE)
	{
		keynamestr[0] = (char)keynum;
		keynamestr[1] = '\0';
		return keynamestr;
	}

	// find a description for special keys
	for (j = 0; j < NUMKEYNAMES; j++)
		if (keynames[j].keynum == keynum)
			return keynames[j].name;

	// create a name for unknown keys
	sprintf(keynamestr, "KEY%d", keynum);
	return keynamestr;
}

INT32 G_KeyNameToNum(const char *keystr)
{
	UINT32 j;

	if (!keystr[1] && keystr[0] > ' ' && keystr[0] <= 'z')
		return keystr[0];

	if (!strncmp(keystr, "KEY", 3) && keystr[3] >= '0' && keystr[3] <= '9')
	{
		/* what if we out of range bruh? */
		j = atoi(&keystr[3]);
		if (j < NUMINPUTS)
			return j;
		return 0;
	}

	for (j = 0; j < NUMKEYNAMES; j++)
		if (!stricmp(keynames[j].name, keystr))
			return keynames[j].keynum;

	return 0;
}

void G_DefineDefaultControls(void)
{
	INT32 i;

	// FPS game controls (WASD)
	gamecontroldefault[gcs_fps][GC_FORWARD    ][0] = 'w';
	gamecontroldefault[gcs_fps][GC_BACKWARD   ][0] = 's';
	gamecontroldefault[gcs_fps][GC_STRAFELEFT ][0] = 'a';
	gamecontroldefault[gcs_fps][GC_STRAFERIGHT][0] = 'd';
	gamecontroldefault[gcs_fps][GC_LOOKUP     ][0] = KEY_UPARROW;
	gamecontroldefault[gcs_fps][GC_LOOKDOWN   ][0] = KEY_DOWNARROW;
	gamecontroldefault[gcs_fps][GC_TURNLEFT   ][0] = KEY_LEFTARROW;
	gamecontroldefault[gcs_fps][GC_TURNRIGHT  ][0] = KEY_RIGHTARROW;
	gamecontroldefault[gcs_fps][GC_CENTERVIEW ][0] = KEY_LCTRL;
	gamecontroldefault[gcs_fps][GC_JUMP       ][0] = KEY_SPACE;
	gamecontroldefault[gcs_fps][GC_SPIN       ][0] = KEY_LSHIFT;
	gamecontroldefault[gcs_fps][GC_FIRE       ][0] = KEY_RCTRL;
	gamecontroldefault[gcs_fps][GC_FIRE       ][1] = KEY_MOUSE1+0;
	gamecontroldefault[gcs_fps][GC_FIRENORMAL ][0] = KEY_RALT;
	gamecontroldefault[gcs_fps][GC_FIRENORMAL ][1] = KEY_MOUSE1+1;
	gamecontroldefault[gcs_fps][GC_CUSTOM1    ][0] = 'z';
	gamecontroldefault[gcs_fps][GC_CUSTOM2    ][0] = 'x';
	gamecontroldefault[gcs_fps][GC_CUSTOM3    ][0] = 'c';

	// Platform game controls (arrow keys), currently unused
	gamecontroldefault[gcs_platform][GC_FORWARD    ][0] = KEY_UPARROW;
	gamecontroldefault[gcs_platform][GC_BACKWARD   ][0] = KEY_DOWNARROW;
	gamecontroldefault[gcs_platform][GC_STRAFELEFT ][0] = 'a';
	gamecontroldefault[gcs_platform][GC_STRAFERIGHT][0] = 'd';
	gamecontroldefault[gcs_platform][GC_LOOKUP     ][0] = KEY_PGUP;
	gamecontroldefault[gcs_platform][GC_LOOKDOWN   ][0] = KEY_PGDN;
	gamecontroldefault[gcs_platform][GC_TURNLEFT   ][0] = KEY_LEFTARROW;
	gamecontroldefault[gcs_platform][GC_TURNRIGHT  ][0] = KEY_RIGHTARROW;
	gamecontroldefault[gcs_platform][GC_CENTERVIEW ][0] = KEY_END;
	gamecontroldefault[gcs_platform][GC_JUMP       ][0] = KEY_SPACE;
	gamecontroldefault[gcs_platform][GC_SPIN       ][0] = KEY_LSHIFT;
	gamecontroldefault[gcs_platform][GC_FIRE       ][0] = 's';
	gamecontroldefault[gcs_platform][GC_FIRE       ][1] = KEY_MOUSE1+0;
	gamecontroldefault[gcs_platform][GC_FIRENORMAL ][0] = 'w';

	for (i = 1; i < num_gamecontrolschemes; i++) // skip gcs_custom (0)
	{
		gamecontroldefault[i][GC_WEAPONNEXT   ][0] = KEY_MOUSEWHEELUP+0;
		gamecontroldefault[i][GC_WEAPONPREV   ][0] = KEY_MOUSEWHEELDOWN+0;
		gamecontroldefault[i][GC_WEPSLOT1     ][0] = '1';
		gamecontroldefault[i][GC_WEPSLOT2     ][0] = '2';
		gamecontroldefault[i][GC_WEPSLOT3     ][0] = '3';
		gamecontroldefault[i][GC_WEPSLOT4     ][0] = '4';
		gamecontroldefault[i][GC_WEPSLOT5     ][0] = '5';
		gamecontroldefault[i][GC_WEPSLOT6     ][0] = '6';
		gamecontroldefault[i][GC_WEPSLOT7     ][0] = '7';
		gamecontroldefault[i][GC_WEPSLOT8     ][0] = '8';
		gamecontroldefault[i][GC_WEPSLOT9     ][0] = '9';
		gamecontroldefault[i][GC_WEPSLOT10    ][0] = '0';
		gamecontroldefault[i][GC_TOSSFLAG     ][0] = '\'';
		gamecontroldefault[i][GC_CAMTOGGLE    ][0] = 'v';
		gamecontroldefault[i][GC_CAMRESET     ][0] = 'r';
		gamecontroldefault[i][GC_TALKKEY      ][0] = 't';
		gamecontroldefault[i][GC_TEAMKEY      ][0] = 'y';
		gamecontroldefault[i][GC_SCORES       ][0] = KEY_TAB;
		gamecontroldefault[i][GC_CONSOLE      ][0] = KEY_CONSOLE;
		gamecontroldefault[i][GC_PAUSE        ][0] = 'p';
		gamecontroldefault[i][GC_SCREENSHOT   ][0] = KEY_F8;
		gamecontroldefault[i][GC_RECORDGIF    ][0] = KEY_F9;
		gamecontroldefault[i][GC_VIEWPOINTNEXT][0] = KEY_F12;

		// Gamepad controls -- same for both schemes
		gamecontroldefault[i][GC_JUMP         ][1] = KEY_JOY1+0; // A
		gamecontroldefault[i][GC_SPIN         ][1] = KEY_JOY1+2; // X
		gamecontroldefault[i][GC_CUSTOM1      ][1] = KEY_JOY1+1; // B
		gamecontroldefault[i][GC_CUSTOM2      ][1] = KEY_JOY1+3; // Y
		gamecontroldefault[i][GC_CUSTOM3      ][1] = KEY_JOY1+8; // Left Stick
		gamecontroldefault[i][GC_CAMTOGGLE    ][1] = KEY_JOY1+4; // LB
		gamecontroldefault[i][GC_CENTERVIEW   ][1] = KEY_JOY1+5; // RB
		gamecontroldefault[i][GC_SCREENSHOT   ][1] = KEY_JOY1+6; // Back
		gamecontroldefault[i][GC_SYSTEMMENU   ][0] = KEY_JOY1+7; // Start
		gamecontroldefault[i][GC_WEAPONPREV   ][1] = KEY_HAT1+2; // D-Pad Left
		gamecontroldefault[i][GC_WEAPONNEXT   ][1] = KEY_HAT1+3; // D-Pad Right
		gamecontroldefault[i][GC_VIEWPOINTNEXT][1] = KEY_JOY1+9; // Right Stick
		gamecontroldefault[i][GC_TOSSFLAG     ][1] = KEY_HAT1+0; // D-Pad Up
		gamecontroldefault[i][GC_SCORES       ][1] = KEY_HAT1+1; // D-Pad Down

		// Second player controls only have joypad defaults
		gamecontrolbisdefault[i][GC_JUMP         ][1] = KEY_2JOY1+0; // A
		gamecontrolbisdefault[i][GC_SPIN         ][1] = KEY_2JOY1+2; // X
		gamecontrolbisdefault[i][GC_CUSTOM1      ][1] = KEY_2JOY1+1; // B
		gamecontrolbisdefault[i][GC_CUSTOM2      ][1] = KEY_2JOY1+3; // Y
		gamecontrolbisdefault[i][GC_CUSTOM3      ][1] = KEY_2JOY1+8; // Left Stick
		gamecontrolbisdefault[i][GC_CAMTOGGLE    ][1] = KEY_2JOY1+4; // LB
		gamecontrolbisdefault[i][GC_CENTERVIEW   ][1] = KEY_2JOY1+5; // RB
		gamecontrolbisdefault[i][GC_SCREENSHOT   ][1] = KEY_2JOY1+6; // Back
		//gamecontrolbisdefault[i][GC_SYSTEMMENU   ][0] = KEY_2JOY1+7; // Start
		gamecontrolbisdefault[i][GC_WEAPONPREV   ][1] = KEY_2HAT1+2; // D-Pad Left
		gamecontrolbisdefault[i][GC_WEAPONNEXT   ][1] = KEY_2HAT1+3; // D-Pad Right
		gamecontrolbisdefault[i][GC_VIEWPOINTNEXT][1] = KEY_2JOY1+9; // Right Stick
		gamecontrolbisdefault[i][GC_TOSSFLAG     ][1] = KEY_2HAT1+0; // D-Pad Up
		//gamecontrolbisdefault[i][GC_SCORES       ][1] = KEY_2HAT1+1; // D-Pad Down
	}
}

INT32 G_GetControlScheme(INT32 (*fromcontrols)[2], const INT32 *gclist, INT32 gclen)
{
	INT32 i, j, gc;
	boolean skipscheme;

	for (i = 1; i < num_gamecontrolschemes; i++) // skip gcs_custom (0)
	{
		skipscheme = false;
		for (j = 0; j < (gclist && gclen ? gclen : NUM_GAMECONTROLS); j++)
		{
			gc = (gclist && gclen) ? gclist[j] : j;
			if (((fromcontrols[gc][0] && gamecontroldefault[i][gc][0]) ? fromcontrols[gc][0] != gamecontroldefault[i][gc][0] : true) &&
				((fromcontrols[gc][0] && gamecontroldefault[i][gc][1]) ? fromcontrols[gc][0] != gamecontroldefault[i][gc][1] : true) &&
				((fromcontrols[gc][1] && gamecontroldefault[i][gc][0]) ? fromcontrols[gc][1] != gamecontroldefault[i][gc][0] : true) &&
				((fromcontrols[gc][1] && gamecontroldefault[i][gc][1]) ? fromcontrols[gc][1] != gamecontroldefault[i][gc][1] : true))
			{
				skipscheme = true;
				break;
			}
		}
		if (!skipscheme)
			return i;
	}

	return gcs_custom;
}

void G_CopyControls(INT32 (*setupcontrols)[2], INT32 (*fromcontrols)[2], const INT32 *gclist, INT32 gclen)
{
	INT32 i, gc;

	for (i = 0; i < (gclist && gclen ? gclen : NUM_GAMECONTROLS); i++)
	{
		gc = (gclist && gclen) ? gclist[i] : i;
		setupcontrols[gc][0] = fromcontrols[gc][0];
		setupcontrols[gc][1] = fromcontrols[gc][1];
	}
}

void G_SaveKeySetting(FILE *f, INT32 (*fromcontrols)[2], INT32 (*fromcontrolsbis)[2])
{
	INT32 i;

	for (i = 1; i < NUM_GAMECONTROLS; i++)
	{
		fprintf(f, "setcontrol \"%s\" \"%s\"", gamecontrolname[i],
			G_KeyNumToName(fromcontrols[i][0]));

		if (fromcontrols[i][1])
			fprintf(f, " \"%s\"\n", G_KeyNumToName(fromcontrols[i][1]));
		else
			fprintf(f, "\n");
	}

	for (i = 1; i < NUM_GAMECONTROLS; i++)
	{
		fprintf(f, "setcontrol2 \"%s\" \"%s\"", gamecontrolname[i],
			G_KeyNumToName(fromcontrolsbis[i][0]));

		if (fromcontrolsbis[i][1])
			fprintf(f, " \"%s\"\n", G_KeyNumToName(fromcontrolsbis[i][1]));
		else
			fprintf(f, "\n");
	}
}

INT32 G_CheckDoubleUsage(INT32 keynum, boolean modify)
{
	INT32 result = GC_NULL;
	if (cv_controlperkey.value == 1)
	{
		INT32 i;
		for (i = 0; i < NUM_GAMECONTROLS; i++)
		{
			if (gamecontrol[i][0] == keynum)
			{
				result = i;
				if (modify) gamecontrol[i][0] = KEY_NULL;
			}
			if (gamecontrol[i][1] == keynum)
			{
				result = i;
				if (modify) gamecontrol[i][1] = KEY_NULL;
			}
			if (gamecontrolbis[i][0] == keynum)
			{
				result = i;
				if (modify) gamecontrolbis[i][0] = KEY_NULL;
			}
			if (gamecontrolbis[i][1] == keynum)
			{
				result = i;
				if (modify) gamecontrolbis[i][1] = KEY_NULL;
			}
			if (result && !modify)
				return result;
		}
	}
	return result;
}

static INT32 G_FilterKeyByVersion(INT32 numctrl, INT32 keyidx, INT32 player, INT32 *keynum1, INT32 *keynum2, boolean *nestedoverride)
{
	// Special case: ignore KEY_PAUSE because it's hardcoded
	if (keyidx == 0 && *keynum1 == KEY_PAUSE)
	{
		if (*keynum2 != KEY_PAUSE)
		{
			*keynum1 = *keynum2; // shift down keynum2 and continue
			*keynum2 = 0;
		}
		else
			return -1; // skip setting control
	}
	else if (keyidx == 1 && *keynum2 == KEY_PAUSE)
		return -1; // skip setting control

	if (GETMAJOREXECVERSION(cv_execversion.value) < 27 && ( // v2.1.22
		numctrl == GC_WEAPONNEXT || numctrl == GC_WEAPONPREV || numctrl == GC_TOSSFLAG ||
		numctrl == GC_SPIN || numctrl == GC_CAMRESET || numctrl == GC_JUMP ||
		numctrl == GC_PAUSE || numctrl == GC_SYSTEMMENU || numctrl == GC_CAMTOGGLE ||
		numctrl == GC_SCREENSHOT || numctrl == GC_TALKKEY || numctrl == GC_SCORES ||
		numctrl == GC_CENTERVIEW
	))
	{
		INT32 keynum = 0, existingctrl = 0;
		INT32 defaultkey;
		boolean defaultoverride = false;

		// get the default gamecontrol
		if (player == 0 && numctrl == GC_SYSTEMMENU)
			defaultkey = gamecontrol[numctrl][0];
		else
			defaultkey = (player == 1 ? gamecontrolbis[numctrl][0] : gamecontrol[numctrl][1]);

		// Assign joypad button defaults if there is an open slot.
		// At this point, gamecontrol/bis should have the default controls
		// (unless LOADCONFIG is being run)
		//
		// If the player runs SETCONTROL in-game, this block should not be reached
		// because EXECVERSION is locked onto the latest version.
		if (keyidx == 0 && !*keynum1)
		{
			if (*keynum2) // push keynum2 down; this is an edge case
			{
				*keynum1 = *keynum2;
				*keynum2 = 0;
				keynum = *keynum1;
			}
			else
			{
				keynum = defaultkey;
				defaultoverride = true;
			}
		}
		else if (keyidx == 1 && (!*keynum2 || (!*keynum1 && *keynum2))) // last one is the same edge case as above
		{
			keynum = defaultkey;
			defaultoverride = true;
		}
		else // default to the specified keynum
			keynum = (keyidx == 1 ? *keynum2 : *keynum1);

		// Did our last call override keynum2?
		if (*nestedoverride)
		{
			defaultoverride = true;
			*nestedoverride = false;
		}

		// Fill keynum2 with the default control
		if (keyidx == 0 && !*keynum2)
		{
			*keynum2 = defaultkey;
			// Tell the next call that this is an override
			*nestedoverride = true;

			// if keynum2 already matches keynum1, we probably recursed
			// so unset it
			if (*keynum1 == *keynum2)
			{
				*keynum2 = 0;
				*nestedoverride = false;
		}
		}

		// check if the key is being used somewhere else before passing it
		// pass it through if it's the same numctrl. This is an edge case -- when using
		// LOADCONFIG, gamecontrol is not reset with default.
		//
		// Also, only check if we're actually overriding, to preserve behavior where
		// config'd keys overwrite default keys.
		if (defaultoverride)
			existingctrl = G_CheckDoubleUsage(keynum, false);

		if (keynum && (!existingctrl || existingctrl == numctrl))
			return keynum;
		else if (keyidx == 0 && *keynum2)
		{
			// try it again and push down keynum2
			*keynum1 = *keynum2;
			*keynum2 = 0;
			return G_FilterKeyByVersion(numctrl, keyidx, player, keynum1, keynum2, nestedoverride);
			// recursion *should* be safe because we only assign keynum2 to a joy default
			// and then clear it if we find that keynum1 already has the joy default.
		}
		else
			return 0;
	}

	// All's good, so pass the keynum as-is
	if (keyidx == 1)
		return *keynum2;
	else //if (keyidx == 0)
		return *keynum1;
}

static void setcontrol(INT32 (*gc)[2])
{
	INT32 numctrl;
	const char *namectrl;
	INT32 keynum, keynum1, keynum2 = 0;
	INT32 player = ((void*)gc == (void*)&gamecontrolbis ? 1 : 0);
	boolean nestedoverride = false;

	// Update me for 2.3
	namectrl = (stricmp(COM_Argv(1), "use")) ? COM_Argv(1) : "spin";

	for (numctrl = 0; numctrl < NUM_GAMECONTROLS && stricmp(namectrl, gamecontrolname[numctrl]);
		numctrl++)
		;
	if (numctrl == NUM_GAMECONTROLS)
	{
		CONS_Printf(M_GetText("Control '%s' unknown\n"), namectrl);
		return;
	}
	keynum1 = G_KeyNameToNum(COM_Argv(2));
	if (COM_Argc() > 3)
		keynum2 = G_KeyNameToNum(COM_Argv(3));
	keynum = G_FilterKeyByVersion(numctrl, 0, player, &keynum1, &keynum2, &nestedoverride);

	if (keynum >= 0)
	{
		(void)G_CheckDoubleUsage(keynum, true);

		// if keynum was rejected, try it again with keynum2
		if (!keynum && keynum2)
		{
			keynum1 = keynum2; // push down keynum2
			keynum2 = 0;
			keynum = G_FilterKeyByVersion(numctrl, 0, player, &keynum1, &keynum2, &nestedoverride);
			if (keynum >= 0)
				(void)G_CheckDoubleUsage(keynum, true);
		}
	}

	if (keynum >= 0)
		gc[numctrl][0] = keynum;

	if (keynum2)
	{
		keynum = G_FilterKeyByVersion(numctrl, 1, player, &keynum1, &keynum2, &nestedoverride);
		if (keynum >= 0)
		{
			if (keynum != gc[numctrl][0])
				gc[numctrl][1] = keynum;
			else
				gc[numctrl][1] = 0;
		}
	}
	else
		gc[numctrl][1] = 0;
}

void Command_Setcontrol_f(void)
{
	INT32 na;

	na = (INT32)COM_Argc();

	if (na != 3 && na != 4)
	{
		CONS_Printf(M_GetText("setcontrol <controlname> <keyname> [<2nd keyname>]: set controls for player 1\n"));
		return;
	}

	setcontrol(gamecontrol);
}

void Command_Setcontrol2_f(void)
{
	INT32 na;

	na = (INT32)COM_Argc();

	if (na != 3 && na != 4)
	{
		CONS_Printf(M_GetText("setcontrol2 <controlname> <keyname> [<2nd keyname>]: set controls for player 2\n"));
		return;
	}

	setcontrol(gamecontrolbis);
}

void G_SetMouseDeltas(INT32 dx, INT32 dy, UINT8 ssplayer)
{
	mouse_t *m = ssplayer == 1 ? &mouse : &mouse2;
	consvar_t *cvsens, *cvysens;

	cvsens = ssplayer == 1 ? &cv_mousesens : &cv_mousesens2;
	cvysens = ssplayer == 1 ? &cv_mouseysens : &cv_mouseysens2;
	m->rdx = dx;
	m->rdy = dy;
	m->dx = (INT32)(m->rdx*((cvsens->value*cvsens->value)/110.0f + 0.1f));
	m->dy = (INT32)(m->rdy*((cvsens->value*cvsens->value)/110.0f + 0.1f));
	m->mlookdy = (INT32)(m->rdy*((cvysens->value*cvsens->value)/110.0f + 0.1f));
}

// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2022 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  g_input.h
/// \brief handle mouse/keyboard/gamepad inputs,
///        maps inputs to game controls (forward, spin, jump...)

#ifndef __G_INPUT__
#define __G_INPUT__

#include "d_event.h"
#include "keys.h"
#include "command.h"
#include "m_fixed.h"

// number of total 'button' inputs, include keyboard keys, plus virtual
// keys (mousebuttons and joybuttons becomes keys)
#define NUMKEYS 256

// Max gamepads that can be used by every player
#define NUM_GAMEPADS 2

// Max gamepads that can be detected
#define MAX_CONNECTED_GAMEPADS 4

// Max mouse buttons
#define MOUSEBUTTONS 8

typedef enum
{
	GAMEPAD_TYPE_UNKNOWN,

	GAMEPAD_TYPE_XBOX360,
	GAMEPAD_TYPE_XBOXONE,
	GAMEPAD_TYPE_XBOX_SERIES_XS,
	GAMEPAD_TYPE_XBOX_ELITE,

	GAMEPAD_TYPE_PS3,
	GAMEPAD_TYPE_PS4,
	GAMEPAD_TYPE_PS5,

	GAMEPAD_TYPE_NINTENDO_SWITCH_PRO,
	GAMEPAD_TYPE_NINTENDO_SWITCH_JOY_CON_GRIP,
	GAMEPAD_TYPE_NINTENDO_SWITCH_JOY_CON_LEFT,
	GAMEPAD_TYPE_NINTENDO_SWITCH_JOY_CON_RIGHT,

	GAMEPAD_TYPE_GOOGLE_STADIA,
	GAMEPAD_TYPE_AMAZON_LUNA,
	GAMEPAD_TYPE_STEAM_CONTROLLER,

	GAMEPAD_TYPE_VIRTUAL
} gamepadtype_e;

boolean G_GamepadTypeIsXbox(gamepadtype_e type);
boolean G_GamepadTypeIsPlayStation(gamepadtype_e type);
boolean G_GamepadTypeIsNintendoSwitch(gamepadtype_e type);
boolean G_GamepadTypeIsJoyCon(gamepadtype_e type);

const char *G_GamepadTypeToString(gamepadtype_e type);

typedef enum
{
	GAMEPAD_BUTTON_A,
	GAMEPAD_BUTTON_B,
	GAMEPAD_BUTTON_X,
	GAMEPAD_BUTTON_Y,
	GAMEPAD_BUTTON_BACK,
	GAMEPAD_BUTTON_GUIDE,
	GAMEPAD_BUTTON_START,
	GAMEPAD_BUTTON_LEFTSTICK,
	GAMEPAD_BUTTON_RIGHTSTICK,
	GAMEPAD_BUTTON_LEFTSHOULDER,
	GAMEPAD_BUTTON_RIGHTSHOULDER,
	GAMEPAD_BUTTON_DPAD_UP,
	GAMEPAD_BUTTON_DPAD_DOWN,
	GAMEPAD_BUTTON_DPAD_LEFT,
	GAMEPAD_BUTTON_DPAD_RIGHT,

	// According to SDL, this button can be:
	// the Xbox Series X|S share button
	// the PS5 microphone button
	// the Nintendo Switch (Pro or Joy-Con) capture button
	// the Amazon Luna microphone button
	GAMEPAD_BUTTON_MISC1,

	// Xbox Elite paddles
	GAMEPAD_BUTTON_PADDLE1,
	GAMEPAD_BUTTON_PADDLE2,
	GAMEPAD_BUTTON_PADDLE3,
	GAMEPAD_BUTTON_PADDLE4,

	// PS4/PS5 touchpad button
	GAMEPAD_BUTTON_TOUCHPAD,

	NUM_GAMEPAD_BUTTONS
} gamepad_button_e;

typedef enum
{
	GAMEPAD_AXIS_LEFTX,
	GAMEPAD_AXIS_LEFTY,
	GAMEPAD_AXIS_RIGHTX,
	GAMEPAD_AXIS_RIGHTY,
	GAMEPAD_AXIS_TRIGGERLEFT,
	GAMEPAD_AXIS_TRIGGERRIGHT,

	NUM_GAMEPAD_AXES
} gamepad_axis_e;

extern const char *const gamepad_button_names[NUM_GAMEPAD_BUTTONS + 1];
extern const char *const gamepad_axis_names[NUM_GAMEPAD_AXES + 1];

// Haptic effects
typedef struct
{
	fixed_t large_magnitude; // Magnitude of the large motor
	fixed_t small_magnitude; // Magnitude of the small motor
	tic_t duration;          // The total duration of the effect, in tics
} haptic_t;

// Gamepad info for each player on the system
typedef struct
{
	// Gamepad index
	UINT8 num;

	// Gamepad is connected and being used by a player
	boolean connected;

	// What kind of controller this is (Xbox 360, DualShock, Joy-Con, etc.)
	gamepadtype_e type;

	// Treat this gamepad's axes as if it they were buttons
	boolean digital;

	struct {
		boolean supported; // Gamepad can rumble
		boolean active;    // Rumble is active
		boolean paused;    // Rumble is paused
		haptic_t data;     // Current haptic effect status
	} rumble;

	UINT8 buttons[NUM_GAMEPAD_BUTTONS]; // Current state of all buttons
	INT16 axes[NUM_GAMEPAD_AXES]; // Current state of all axes
} gamepad_t;

void G_InitGamepads(void);

typedef enum
{
	GAMEPAD_STRING_DEFAULT, // A
	GAMEPAD_STRING_MENU1,   // A Button
	GAMEPAD_STRING_MENU2    // the A Button
} gamepad_string_e;

const char *G_GetGamepadButtonString(gamepadtype_e type, gamepad_button_e button, gamepad_string_e strtype);
const char *G_GetGamepadAxisString(gamepadtype_e type, gamepad_axis_e button, gamepad_string_e strtype, boolean inv);

extern gamepad_t gamepads[NUM_GAMEPADS];

//
// mouse and gamepad buttons are handled as 'virtual' keys
//
typedef enum
{
	KEY_MOUSE1 = NUMKEYS,
	KEY_GAMEPAD = KEY_MOUSE1 + MOUSEBUTTONS,
	KEY_AXES = KEY_GAMEPAD + NUM_GAMEPAD_BUTTONS, // Sure, why not.
	KEY_INV_AXES = KEY_AXES + NUM_GAMEPAD_AXES,

	KEY_DBLMOUSE1 = KEY_INV_AXES + NUM_GAMEPAD_AXES, // double clicks

	KEY_2MOUSE1 = KEY_DBLMOUSE1 + MOUSEBUTTONS,
	KEY_DBL2MOUSE1 = KEY_2MOUSE1 + MOUSEBUTTONS,

	KEY_MOUSEWHEELUP = KEY_DBL2MOUSE1 + MOUSEBUTTONS,
	KEY_MOUSEWHEELDOWN,
	KEY_2MOUSEWHEELUP,
	KEY_2MOUSEWHEELDOWN,

	NUMINPUTS
} key_input_e;

#define GAMEPAD_KEY(key) (KEY_GAMEPAD + GAMEPAD_BUTTON_##key)
#define GAMEPAD_AXIS(key) (KEY_AXES + GAMEPAD_AXIS_##key)

typedef enum
{
	GC_NULL = 0, // a key/button mapped to GC_NULL has no effect
	GC_FORWARD,
	GC_BACKWARD,
	GC_STRAFELEFT,
	GC_STRAFERIGHT,
	GC_TURNLEFT,
	GC_TURNRIGHT,
	GC_WEAPONNEXT,
	GC_WEAPONPREV,
	GC_WEPSLOT1,
	GC_WEPSLOT2,
	GC_WEPSLOT3,
	GC_WEPSLOT4,
	GC_WEPSLOT5,
	GC_WEPSLOT6,
	GC_WEPSLOT7,
	GC_WEPSLOT8,
	GC_WEPSLOT9,
	GC_WEPSLOT10,
	GC_FIRE,
	GC_FIRENORMAL,
	GC_TOSSFLAG,
	GC_SPIN,
	GC_CAMTOGGLE,
	GC_CAMRESET,
	GC_LOOKUP,
	GC_LOOKDOWN,
	GC_CENTERVIEW,
	GC_MOUSEAIMING, // mouse aiming is momentary (toggleable in the menu)
	GC_TALKKEY,
	GC_TEAMKEY,
	GC_SCORES,
	GC_JUMP,
	GC_CONSOLE,
	GC_PAUSE,
	GC_SYSTEMMENU,
	GC_SCREENSHOT,
	GC_RECORDGIF,
	GC_VIEWPOINTNEXT,
	GC_VIEWPOINTPREV,
	GC_CUSTOM1, // Lua scriptable
	GC_CUSTOM2, // Lua scriptable
	GC_CUSTOM3, // Lua scriptable
	NUM_GAMECONTROLS
} gamecontrols_e;

typedef enum
{
	gcs_custom,
	gcs_fps,
	gcs_platform,
	num_gamecontrolschemes
} gamecontrolschemes_e;

// mouse values are used once
extern consvar_t cv_mousesens, cv_mouseysens;
extern consvar_t cv_mousesens2, cv_mouseysens2;
extern consvar_t cv_controlperkey;

typedef struct
{
	INT32 dx; // deltas with mousemove sensitivity
	INT32 dy;
	INT32 mlookdy; // dy with mouselook sensitivity
	INT32 rdx; // deltas without sensitivity
	INT32 rdy;
	UINT16 buttons;
} mouse_t;

#define MB_BUTTON1    0x0001
#define MB_BUTTON2    0x0002
#define MB_BUTTON3    0x0004
#define MB_BUTTON4    0x0008
#define MB_BUTTON5    0x0010
#define MB_BUTTON6    0x0020
#define MB_BUTTON7    0x0040
#define MB_BUTTON8    0x0080
#define MB_SCROLLUP   0x0100
#define MB_SCROLLDOWN 0x0200

extern mouse_t mouse;
extern mouse_t mouse2;

// current state of the keys: true if pushed
extern UINT8 gamekeydown[NUMINPUTS];

// two key codes (or virtual key) per game control
extern INT32 gamecontrol[NUM_GAMECONTROLS][2];
extern INT32 gamecontrolbis[NUM_GAMECONTROLS][2]; // secondary splitscreen player

// default control storage, use 0 (gcs_custom) for memory retention
extern INT32 gamecontroldefault[num_gamecontrolschemes][NUM_GAMECONTROLS][2];
extern INT32 gamecontrolbisdefault[num_gamecontrolschemes][NUM_GAMECONTROLS][2];

boolean G_PlayerInputDown(UINT8 which, gamecontrols_e gc);
boolean G_CheckDigitalPlayerInput(UINT8 which, gamecontrols_e gc);

SINT8 G_PlayerInputIsAnalog(UINT8 which, gamecontrols_e gc, UINT8 settings);
INT16 G_GetAnalogPlayerInput(UINT8 which, gamecontrols_e gc, UINT8 settings);

#define num_gcl_tutorial_check 6
#define num_gcl_tutorial_used 8
#define num_gcl_tutorial_full 13
#define num_gcl_movement 4
#define num_gcl_camera 2
#define num_gcl_movement_camera 6
#define num_gcl_jump 1
#define num_gcl_spin 1
#define num_gcl_jump_spin 2

extern const INT32 gcl_tutorial_check[num_gcl_tutorial_check];
extern const INT32 gcl_tutorial_used[num_gcl_tutorial_used];
extern const INT32 gcl_tutorial_full[num_gcl_tutorial_full];
extern const INT32 gcl_movement[num_gcl_movement];
extern const INT32 gcl_camera[num_gcl_camera];
extern const INT32 gcl_movement_camera[num_gcl_movement_camera];
extern const INT32 gcl_jump[num_gcl_jump];
extern const INT32 gcl_spin[num_gcl_spin];
extern const INT32 gcl_jump_spin[num_gcl_jump_spin];

// peace to my little coder fingers!
// check a gamecontrol being active or not

// remaps the input event to a game control.
void G_MapEventsToControls(event_t *ev);

boolean G_RumbleSupported(UINT8 which);
boolean G_RumbleGamepad(UINT8 which, fixed_t large_magnitude, fixed_t small_magnitude, tic_t duration);
void G_StopGamepadRumble(UINT8 which);

fixed_t G_GetLargeMotorFreq(UINT8 which);
fixed_t G_GetSmallMotorFreq(UINT8 which);
boolean G_GetGamepadRumblePaused(UINT8 which);
boolean G_SetLargeMotorFreq(UINT8 which, fixed_t freq);
boolean G_SetSmallMotorFreq(UINT8 which, fixed_t freq);
void G_SetGamepadRumblePaused(UINT8 which, boolean pause);

INT16 G_GamepadAxisEventValue(UINT8 which, INT16 value);
INT16 G_GetGamepadAxisValue(UINT8 which, gamepad_axis_e axis);
fixed_t G_GetAdjustedGamepadAxis(UINT8 which, gamepad_axis_e axis, boolean applyDeadzone);

UINT16 G_GetGamepadDeadZone(UINT8 which);
UINT16 G_GetGamepadDigitalDeadZone(UINT8 which);
INT32 G_BasicDeadZoneCalculation(INT32 magnitude, const UINT16 jdeadzone);

INT32 G_RemapGamepadEvent(event_t *event, INT32 *type);

// returns the name of a key
const char *G_KeyNumToName(INT32 keynum);
INT32 G_KeyNameToNum(const char *keystr);

const char *G_GetDisplayNameForKey(INT32 keynum);

// detach any keys associated to the given game control
void G_ClearControlKeys(INT32 (*setupcontrols)[2], INT32 control);
void G_ClearAllControlKeys(void);

void Command_Setcontrol_f(void);
void Command_Setcontrol2_f(void);
void G_DefineDefaultControls(void);
INT32 G_GetControlScheme(INT32 (*fromcontrols)[2], const INT32 *gclist, INT32 gclen);
void G_CopyControls(INT32 (*setupcontrols)[2], INT32 (*fromcontrols)[2], const INT32 *gclist, INT32 gclen);
void G_SaveKeySetting(FILE *f, INT32 (*fromcontrols)[2], INT32 (*fromcontrolsbis)[2]);
INT32 G_CheckDoubleUsage(INT32 keynum, boolean modify);

// sets the members of a mouse_t given position deltas
void G_SetMouseDeltas(INT32 dx, INT32 dy, UINT8 ssplayer);

#endif

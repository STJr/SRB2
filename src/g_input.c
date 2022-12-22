// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2022 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  g_input.c
/// \brief handle mouse/keyboard/gamepad inputs,
///        maps inputs to game controls (forward, spin, jump...)

#include "doomdef.h"
#include "doomstat.h"
#include "g_game.h"
#include "g_input.h"
#include "i_gamepad.h"
#include "keys.h"
#include "hu_stuff.h" // need HUFONT start & end
#include "d_net.h"
#include "console.h"

#define MAXMOUSESENSITIVITY 100 // sensitivity steps

static CV_PossibleValue_t mousesens_cons_t[] = {{1, "MIN"}, {MAXMOUSESENSITIVITY, "MAX"}, {0, NULL}};

// mouse values are used once
consvar_t cv_mousesens = CVAR_INIT ("mousesens", "20", CV_SAVE, mousesens_cons_t, NULL);
consvar_t cv_mousesens2 = CVAR_INIT ("mousesens2", "20", CV_SAVE, mousesens_cons_t, NULL);
consvar_t cv_mouseysens = CVAR_INIT ("mouseysens", "20", CV_SAVE, mousesens_cons_t, NULL);
consvar_t cv_mouseysens2 = CVAR_INIT ("mouseysens2", "20", CV_SAVE, mousesens_cons_t, NULL);

mouse_t mouse;
mouse_t mouse2;

gamepad_t gamepads[NUM_GAMEPADS];

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

static boolean CheckInputDown(UINT8 which, gamecontrols_e gc, boolean checkaxes)
{
	INT32 (*controls)[2] = which == 0 ? gamecontrol : gamecontrolbis;

	for (unsigned i = 0; i < 2; i++)
	{
		INT32 key = controls[gc][i];

		if (key >= KEY_GAMEPAD && key < KEY_AXES)
		{
			if (gamepads[which].buttons[key - KEY_GAMEPAD])
				return true;
		}
		else if (checkaxes && (key >= KEY_AXES && key < KEY_INV_AXES + NUM_GAMEPAD_AXES))
		{
			const UINT16 jdeadzone = G_GetGamepadDigitalDeadZone(which);
			const INT16 value = G_GetGamepadAxisValue(which, (key - KEY_AXES) % NUM_GAMEPAD_AXES);

			if (abs(value) > jdeadzone)
				return true;
		}
		else if (gamekeydown[key])
			return true;
	}

	return false;
}

boolean G_PlayerInputDown(UINT8 which, gamecontrols_e gc)
{
	return CheckInputDown(which, gc, true);
}

boolean G_CheckDigitalPlayerInput(UINT8 which, gamecontrols_e gc)
{
	return CheckInputDown(which, gc, false);
}

SINT8 G_PlayerInputIsAnalog(UINT8 which, gamecontrols_e gc, UINT8 settings)
{
	INT32 (*controls)[2] = which == 0 ? gamecontrol : gamecontrolbis;
	INT32 key = controls[gc][settings];

	if (key >= KEY_AXES && key < KEY_AXES + NUM_GAMEPAD_AXES)
		return 1;
	else if (key >= KEY_INV_AXES && key < KEY_INV_AXES + NUM_GAMEPAD_AXES)
		return -1;

	return 0;
}

INT16 G_GetAnalogPlayerInput(UINT8 which, gamecontrols_e gc, UINT8 settings)
{
	INT32 (*controls)[2] = which == 0 ? gamecontrol : gamecontrolbis;
	INT32 key = controls[gc][settings];

	if (key >= KEY_AXES && key < KEY_INV_AXES + NUM_GAMEPAD_AXES)
		return G_GetGamepadAxisValue(which, (key - KEY_AXES) % NUM_GAMEPAD_AXES);

	return 0;
}

typedef struct
{
	UINT8 time;
	UINT8 state;
	UINT8 clicks;
} dclick_t;

static dclick_t mousedclicks[MOUSEBUTTONS];
static dclick_t mouse2dclicks[MOUSEBUTTONS];

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

//
// Remaps the inputs to game controls.
//
// A game control can be triggered by one or more keys/buttons.
//
// Each key/mouse button/gamepad button triggers ONLY ONE game control.
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
				CONS_Debug(DBG_GAMELOGIC, "Bad downkey input %d\n",ev->key);

#endif
			break;

		case ev_keyup:
			if (ev->key < NUMINPUTS)
				gamekeydown[ev->key] = 0;
#ifdef PARANOIA
			else
				CONS_Debug(DBG_GAMELOGIC, "Bad upkey input %d\n",ev->key);
#endif
			break;

		case ev_gamepad_down:
		case ev_gamepad_up:
#ifdef PARANOIA
			if (ev->which < NUM_GAMEPADS)
#endif
				gamepads[ev->which].buttons[ev->key] = ev->type == ev_gamepad_down ? 1 : 0;
			break;

		case ev_gamepad_axis:
#ifdef PARANOIA
			if (ev->which < NUM_GAMEPADS)
#endif
				gamepads[ev->which].axes[ev->key] = ev->x;
			break;

		case ev_mouse:
			mouse.rdx = ev->x;
			mouse.rdy = ev->y;
			break;

		case ev_mouse2:
			mouse2.rdx = ev->x;
			mouse2.rdy = ev->y;
			break;

		default:
			break;
	}

	// ALWAYS check for mouse double-clicks even if there were no such events
	for (i = 0; i < MOUSEBUTTONS; i++)
	{
		flag = G_CheckDoubleClick(gamekeydown[KEY_MOUSE1+i], &mousedclicks[i]);
		gamekeydown[KEY_DBLMOUSE1+i] = flag;
	}

	for (i = 0; i < MOUSEBUTTONS; i++)
	{
		flag = G_CheckDoubleClick(gamekeydown[KEY_2MOUSE1+i], &mouse2dclicks[i]);
		gamekeydown[KEY_DBL2MOUSE1+i] = flag;
	}
}

const char *const gamepad_button_names[NUM_GAMEPAD_BUTTONS + 1] = {
	"a",
	"b",
	"x",
	"y",
	"back",
	"guide",
	"start",
	"left-stick",
	"right-stick",
	"left-shoulder",
	"right-shoulder",
	"dpad-up",
	"dpad-down",
	"dpad-left",
	"dpad-right",
	"misc1",
	"paddle1",
	"paddle2",
	"paddle3",
	"paddle4",
	"touchpad",
	NULL};

const char *const gamepad_axis_names[NUM_GAMEPAD_AXES + 1] = {
	"left-x",
	"left-y",
	"right-x",
	"right-y",
	"trigger-left",
	"trigger-right",
	NULL};

boolean G_GamepadTypeIsXbox(gamepadtype_e type)
{
	switch (type)
	{
	case GAMEPAD_TYPE_XBOX360:
	case GAMEPAD_TYPE_XBOXONE:
	case GAMEPAD_TYPE_XBOX_SERIES_XS:
	case GAMEPAD_TYPE_XBOX_ELITE:
		return true;
	default:
		return false;
	}
}

boolean G_GamepadTypeIsPlayStation(gamepadtype_e type)
{
	switch (type)
	{
	case GAMEPAD_TYPE_PS3:
	case GAMEPAD_TYPE_PS4:
	case GAMEPAD_TYPE_PS5:
		return true;
	default:
		return false;
	}
}

boolean G_GamepadTypeIsNintendoSwitch(gamepadtype_e type)
{
	switch (type)
	{
	case GAMEPAD_TYPE_NINTENDO_SWITCH_PRO:
	case GAMEPAD_TYPE_NINTENDO_SWITCH_JOY_CON_GRIP:
		return true;
	default:
		return G_GamepadTypeIsJoyCon(type);
	}
}

boolean G_GamepadTypeIsJoyCon(gamepadtype_e type)
{
	switch (type)
	{
	case GAMEPAD_TYPE_NINTENDO_SWITCH_JOY_CON_LEFT:
	case GAMEPAD_TYPE_NINTENDO_SWITCH_JOY_CON_RIGHT:
		return true;
	default:
		return false;
	}
}

boolean G_RumbleSupported(UINT8 which)
{
	if (!I_RumbleSupported() || which >= NUM_GAMEPADS)
		return 0;

	return I_GetGamepadRumbleSupported(which);
}

boolean G_RumbleGamepad(UINT8 which, fixed_t large_magnitude, fixed_t small_magnitude, tic_t duration)
{
	haptic_t effect;

	if (!G_RumbleSupported(which))
		return false;

	effect.large_magnitude = large_magnitude;
	effect.small_magnitude = small_magnitude;
	effect.duration = duration;

	return I_RumbleGamepad(which, &effect);
}

void G_StopGamepadRumble(UINT8 which)
{
	if (G_RumbleSupported(which))
		I_StopGamepadRumble(which);
}

fixed_t G_GetLargeMotorFreq(UINT8 which)
{
	if (!G_RumbleSupported(which) || which >= NUM_GAMEPADS)
		return 0;

	gamepad_t *gamepad = &gamepads[which];
	return gamepad->rumble.data.large_magnitude;
}

fixed_t G_GetSmallMotorFreq(UINT8 which)
{
	if (!G_RumbleSupported(which) || which >= NUM_GAMEPADS)
		return 0;

	gamepad_t *gamepad = &gamepads[which];
	return gamepad->rumble.data.small_magnitude;
}

boolean G_GetGamepadRumblePaused(UINT8 which)
{
	return I_GetGamepadRumblePaused(which);
}

boolean G_SetLargeMotorFreq(UINT8 which, fixed_t freq)
{
	return I_SetGamepadLargeMotorFreq(which, freq);
}

boolean G_SetSmallMotorFreq(UINT8 which, fixed_t freq)
{
	return I_SetGamepadSmallMotorFreq(which, freq);
}

void G_SetGamepadRumblePaused(UINT8 which, boolean pause)
{
	if (G_RumbleSupported(which))
		I_SetGamepadRumblePaused(which, pause);
}

// Obtains the value of an axis, and makes it digital if needed
INT16 G_GamepadAxisEventValue(UINT8 which, INT16 value)
{
	gamepad_t *gamepad = &gamepads[which];

	if (gamepad->digital)
	{
		const UINT16 jdeadzone = G_GetGamepadDigitalDeadZone(which);

		if (value < -jdeadzone)
			value = -JOYAXISRANGE - 1;
		else if (value > jdeadzone)
			value = JOYAXISRANGE;
		else
			value = 0;
	}

	return value;
}

INT16 G_GetGamepadAxisValue(UINT8 which, gamepad_axis_e axis)
{
	gamepad_t *gamepad = &gamepads[which];

	if (axis >= NUM_GAMEPAD_AXES)
		return 0;

	return G_GamepadAxisEventValue(which, gamepad->axes[axis]);
}

fixed_t G_GetAdjustedGamepadAxis(UINT8 which, gamepad_axis_e axis, boolean applyDeadzone)
{
	gamepad_t *gamepad = &gamepads[which];

	if (axis >= NUM_GAMEPAD_AXES)
		return 0;

	INT32 value = gamepad->axes[axis];

	if (applyDeadzone && gamepad->digital)
	{
		INT16 deadzone = G_GetGamepadDigitalDeadZone(which);

		if (value < -deadzone)
			value = -JOYAXISRANGE;
		else if (value > deadzone)
			value = JOYAXISRANGE;
		else
			value = 0;
	}
	else if (applyDeadzone)
	{
		INT32 sign = value < 0 ? -1 : 1;
		INT16 deadzone = G_GetGamepadDeadZone(which);
		INT32 magnitude = value * value;
		INT32 nAxis = magnitude / JOYAXISRANGE;
		INT32 nMagnitude = G_BasicDeadZoneCalculation(magnitude, deadzone);

		value = (nAxis * nMagnitude) / JOYAXISRANGE;
		value = min(value * sign, JOYAXISRANGE);
		value = max(value, -JOYAXISRANGE);
	}

	return (value / 32767.0) * FRACUNIT;
}

static UINT16 CalcGamepadDeadZone(fixed_t deadzone)
{
	INT32 value = (JOYAXISRANGE * deadzone) / FRACUNIT;

	if (value < 0)
		value = 0;
	else if (value > JOYAXISRANGE)
		value = JOYAXISRANGE;

	return value;
}

UINT16 G_GetGamepadDeadZone(UINT8 which)
{
	return CalcGamepadDeadZone(cv_deadzone[which].value);
}

UINT16 G_GetGamepadDigitalDeadZone(UINT8 which)
{
	return CalcGamepadDeadZone(cv_digitaldeadzone[which].value);
}

// Take a magnitude of two axes, and adjust it to take out the deadzone
// Will return a value between 0 and JOYAXISRANGE
INT32 G_BasicDeadZoneCalculation(INT32 magnitude, const UINT16 jdeadzone)
{
	INT32 deadzoneAppliedValue = 0;
	INT32 adjustedMagnitude = abs(magnitude);

	if (jdeadzone >= JOYAXISRANGE && adjustedMagnitude >= JOYAXISRANGE) // If the deadzone and magnitude are both 100%...
		return JOYAXISRANGE; // ...return 100% input directly, to avoid dividing by 0
	else if (adjustedMagnitude > jdeadzone) // Otherwise, calculate how much the magnitude exceeds the deadzone
	{
		adjustedMagnitude = min(adjustedMagnitude, JOYAXISRANGE);

		adjustedMagnitude -= jdeadzone;

		deadzoneAppliedValue = (adjustedMagnitude * JOYAXISRANGE) / (JOYAXISRANGE - jdeadzone);
	}

	return deadzoneAppliedValue;
}

INT32 G_RemapGamepadEvent(event_t *event, INT32 *type)
{
	if (event->type == ev_gamepad_down)
	{
		*type = ev_keydown;
		return KEY_GAMEPAD + event->key;
	}
	else if (event->type == ev_gamepad_up)
	{
		*type = ev_keyup;
		return KEY_GAMEPAD + event->key;
	}
	else if (event->type == ev_gamepad_axis)
	{
		const UINT16 jdeadzone = G_GetGamepadDigitalDeadZone(event->which);
		const INT16 value = G_GetGamepadAxisValue(event->which, event->key);

		if (value < -jdeadzone || value > jdeadzone)
			*type = ev_keyup;
		else
			*type = ev_keydown;

		if (value < -jdeadzone)
			return KEY_INV_AXES + event->key;
		else
			return KEY_AXES + event->key;
	}

	return event->key;
}

typedef struct
{
	const char *name;
	const char *menu1;
	const char *menu2;
} button_strings_t;

#define DEF_NAME_BUTTON(str) {.name = str, .menu1 = str " Button", .menu2 = "the " str " Button"}
#define DEF_NAME_SIMPLE(str) {.name = str, .menu1 = NULL, .menu2 = "the " str}
#define DEF_NAME_DPAD(a, b)  {.name = "D-Pad " a, .menu1 = "D-Pad " b, .menu2 = a}

#define PARTIAL_DEF_START [GAMEPAD_BUTTON_A] = { NULL }
#define PARTIAL_DEF_END [NUM_GAMEPAD_BUTTONS - 1] = { NULL }

static const char *GetStringFromButtonList(const button_strings_t *names, gamepad_button_e button, gamepad_string_e type)
{
	switch (type)
	{
	case GAMEPAD_STRING_DEFAULT:
		return names[button].name;
	case GAMEPAD_STRING_MENU1:
		if (names[button].menu1)
			return names[button].menu1;
		else
			return names[button].name;
	case GAMEPAD_STRING_MENU2:
		if (names[button].menu2)
			return names[button].menu2;
		else
			return names[button].name;
	}

	return NULL;
}

const char *G_GetGamepadButtonString(gamepadtype_e type, gamepad_button_e button, gamepad_string_e strtype)
{
	static const button_strings_t base_names[] = {
		[GAMEPAD_BUTTON_A]             = DEF_NAME_BUTTON("A"),
		[GAMEPAD_BUTTON_B]             = DEF_NAME_BUTTON("B"),
		[GAMEPAD_BUTTON_X]             = DEF_NAME_BUTTON("X"),
		[GAMEPAD_BUTTON_Y]             = DEF_NAME_BUTTON("Y"),
		[GAMEPAD_BUTTON_BACK]          = DEF_NAME_BUTTON("Back"),
		[GAMEPAD_BUTTON_GUIDE]         = DEF_NAME_BUTTON("Guide"),
		[GAMEPAD_BUTTON_START]         = DEF_NAME_BUTTON("Start"),
		[GAMEPAD_BUTTON_LEFTSTICK]     = DEF_NAME_SIMPLE("Left Stick"),
		[GAMEPAD_BUTTON_RIGHTSTICK]    = DEF_NAME_SIMPLE("Right Stick"),
		[GAMEPAD_BUTTON_LEFTSHOULDER]  = DEF_NAME_SIMPLE("Left Shoulder"),
		[GAMEPAD_BUTTON_RIGHTSHOULDER] = DEF_NAME_SIMPLE("Right Shoulder"),
		[GAMEPAD_BUTTON_DPAD_UP]       = DEF_NAME_DPAD("Up", "\x1A"),
		[GAMEPAD_BUTTON_DPAD_DOWN]     = DEF_NAME_DPAD("Down", "\x1B"),
		[GAMEPAD_BUTTON_DPAD_LEFT]     = DEF_NAME_DPAD("Left", "\x1C"),
		[GAMEPAD_BUTTON_DPAD_RIGHT]    = DEF_NAME_DPAD("Right", "\x1D"),
		[GAMEPAD_BUTTON_PADDLE1]       = DEF_NAME_SIMPLE("Paddle 1"),
		[GAMEPAD_BUTTON_PADDLE2]       = DEF_NAME_SIMPLE("Paddle 2"),
		[GAMEPAD_BUTTON_PADDLE3]       = DEF_NAME_SIMPLE("Paddle 3"),
		[GAMEPAD_BUTTON_PADDLE4]       = DEF_NAME_SIMPLE("Paddle 4"),
		[GAMEPAD_BUTTON_TOUCHPAD]      = DEF_NAME_SIMPLE("Touchpad"),

		// This one's a bit weird
		// Suffix the numbers in the event SDL adds more misc buttons
		[GAMEPAD_BUTTON_MISC1] = {
			.name  = "Misc. Button",
			.menu1 = "Gamepad Misc.",
			.menu2 = "the Misc. Button"
		},
	};

	button_strings_t const *names = NULL;

	if (G_GamepadTypeIsXbox(type))
	{
		#define BASE_XBOX_NAMES \
			[GAMEPAD_BUTTON_LEFTSHOULDER]  = DEF_NAME_SIMPLE("Left Bumper"), \
			[GAMEPAD_BUTTON_RIGHTSHOULDER] = DEF_NAME_SIMPLE("Right Bumper")

		static const button_strings_t xbox_names[] = { PARTIAL_DEF_START,
			BASE_XBOX_NAMES,
		PARTIAL_DEF_END };

		static const button_strings_t series_xs_names[] = { PARTIAL_DEF_START,
			BASE_XBOX_NAMES,
			[GAMEPAD_BUTTON_MISC1]         = DEF_NAME_BUTTON("Share"),
		PARTIAL_DEF_END };

		static const button_strings_t elite_names[] = { PARTIAL_DEF_START,
			BASE_XBOX_NAMES,
			[GAMEPAD_BUTTON_PADDLE1]       = DEF_NAME_SIMPLE("P1 Paddle"),
			[GAMEPAD_BUTTON_PADDLE2]       = DEF_NAME_SIMPLE("P2 Paddle"),
			[GAMEPAD_BUTTON_PADDLE3]       = DEF_NAME_SIMPLE("P3 Paddle"),
			[GAMEPAD_BUTTON_PADDLE4]       = DEF_NAME_SIMPLE("P4 Paddle"),
		PARTIAL_DEF_END };

		if (type == GAMEPAD_TYPE_XBOX_SERIES_XS) // X|S controllers have a Share button
			names = series_xs_names;
		else if (type == GAMEPAD_TYPE_XBOX_ELITE) // Elite controller has paddles
			names = elite_names;
		else
			names = xbox_names;

		#undef BASE_XBOX_NAMES
	}
	else if (G_GamepadTypeIsPlayStation(type))
	{
		#define BASE_PS_NAMES \
			[GAMEPAD_BUTTON_A]             = DEF_NAME_BUTTON("Cross"), \
			[GAMEPAD_BUTTON_B]             = DEF_NAME_BUTTON("Circle"), \
			[GAMEPAD_BUTTON_X]             = DEF_NAME_BUTTON("Square"), \
			[GAMEPAD_BUTTON_Y]             = DEF_NAME_BUTTON("Triangle"), \
			[GAMEPAD_BUTTON_BACK]          = DEF_NAME_BUTTON("Select"), \
			[GAMEPAD_BUTTON_GUIDE]         = DEF_NAME_BUTTON("PS"), \
			[GAMEPAD_BUTTON_LEFTSTICK]     = DEF_NAME_BUTTON("L3"), \
			[GAMEPAD_BUTTON_RIGHTSTICK]    = DEF_NAME_BUTTON("R3"), \
			[GAMEPAD_BUTTON_LEFTSHOULDER]  = DEF_NAME_BUTTON("L1"), \
			[GAMEPAD_BUTTON_RIGHTSHOULDER] = DEF_NAME_BUTTON("R1")

		static const button_strings_t ps_names[] = {
			BASE_PS_NAMES,
		PARTIAL_DEF_END };

		static const button_strings_t ps5_names[] = {
			BASE_PS_NAMES,
			[GAMEPAD_BUTTON_MISC1] = DEF_NAME_BUTTON("Microphone"),
		PARTIAL_DEF_END };

		names = type == GAMEPAD_TYPE_PS5 ? ps5_names : ps_names;
		#undef BASE_PS_NAMES
	}
	else if (G_GamepadTypeIsNintendoSwitch(type))
	{
		static const button_strings_t switch_names[] = { PARTIAL_DEF_START,
			[GAMEPAD_BUTTON_BACK]          = DEF_NAME_BUTTON("-"),
			[GAMEPAD_BUTTON_GUIDE]         = DEF_NAME_BUTTON("HOME"),
			[GAMEPAD_BUTTON_START]         = DEF_NAME_BUTTON("+"),
			[GAMEPAD_BUTTON_LEFTSHOULDER]  = DEF_NAME_BUTTON("L"),
			[GAMEPAD_BUTTON_RIGHTSHOULDER] = DEF_NAME_BUTTON("R"),
			[GAMEPAD_BUTTON_MISC1]         = DEF_NAME_BUTTON("Capture"),
		PARTIAL_DEF_END };

		names = switch_names;
	}
	else if (type == GAMEPAD_TYPE_AMAZON_LUNA)
	{
		static const button_strings_t luna_names[] = { PARTIAL_DEF_START,
			[GAMEPAD_BUTTON_MISC1] = DEF_NAME_BUTTON("Microphone"),
		PARTIAL_DEF_END };

		names = luna_names;
	}

	const char *str = NULL;

	if (names)
		str = GetStringFromButtonList(names, button, strtype);
	if (str == NULL)
		str = GetStringFromButtonList(base_names, button, strtype);
	if (str)
		return str;

	return "Unknown";
}

#undef DEF_NAME_BUTTON
#undef DEF_NAME_SIMPLE
#undef DEF_NAME_DPAD

#undef PARTIAL_DEF_START
#undef PARTIAL_DEF_END

typedef struct
{
	const char *name;
	const char *menu1;
	const char *menu2;
	const char *name_inv;
	const char *menu1_inv;
	const char *menu2_inv;
} axis_strings_t;

#define DEF_NAME_AXIS(str, a, inv_a, b, inv_b) {\
	str " " a, str " " b, "the " str " " b, \
	str " " inv_a, str " " inv_b, "the " str " " inv_b}
#define DEF_NAME_TRIGGER(str) {str, NULL, "the " str, NULL, NULL, NULL}
#define DEF_NAME_BUTTON(str) {str, str " Button", "the " str " Button", NULL, NULL, NULL}

#define PARTIAL_DEF_START [GAMEPAD_AXIS_LEFTX] = { NULL }

static const char *GetStringFromAxisList(const axis_strings_t *names, gamepad_axis_e axis, gamepad_string_e type, boolean inv)
{
	switch (type)
	{
	case GAMEPAD_STRING_DEFAULT:
		if (inv && names[axis].name_inv)
			return names[axis].name_inv;
		else
			return names[axis].name;
		break;
	case GAMEPAD_STRING_MENU1:
		if (inv && names[axis].menu1_inv)
			return names[axis].menu1_inv;
		if (names[axis].menu1)
			return names[axis].menu1;
		else
			return names[axis].name;
		break;
	case GAMEPAD_STRING_MENU2:
		if (inv && names[axis].menu2_inv)
			return names[axis].menu2_inv;
		if (names[axis].menu2)
			return names[axis].menu2;
		else
			return names[axis].name;
		break;
	}

	return NULL;
}

const char *G_GetGamepadAxisString(gamepadtype_e type, gamepad_axis_e axis, gamepad_string_e strtype, boolean inv)
{
	static const axis_strings_t base_names[] = {
		[GAMEPAD_AXIS_LEFTX]        = DEF_NAME_AXIS("Left Stick",  "X", "X-", "\x1D", "\x1C"),
		[GAMEPAD_AXIS_LEFTY]        = DEF_NAME_AXIS("Left Stick",  "Y", "Y-", "\x1B", "\x1A"),
		[GAMEPAD_AXIS_RIGHTX]       = DEF_NAME_AXIS("Right Stick", "X", "X-", "\x1D", "\x1C"),
		[GAMEPAD_AXIS_RIGHTY]       = DEF_NAME_AXIS("Right Stick", "Y", "Y-", "\x1B", "\x1A"),
		[GAMEPAD_AXIS_TRIGGERLEFT]  = DEF_NAME_TRIGGER("Left Trigger"),
		[GAMEPAD_AXIS_TRIGGERRIGHT] = DEF_NAME_TRIGGER("Right Trigger")
	};

	axis_strings_t const *names = NULL;

	if (G_GamepadTypeIsPlayStation(type))
	{
		static const axis_strings_t ps_names[] = { PARTIAL_DEF_START,
			[GAMEPAD_AXIS_TRIGGERLEFT]  = DEF_NAME_BUTTON("L2"),
			[GAMEPAD_AXIS_TRIGGERRIGHT] = DEF_NAME_BUTTON("R2"),
		};

		names = ps_names;
	}
	else if (G_GamepadTypeIsNintendoSwitch(type))
	{
		static const axis_strings_t switch_names[] = { PARTIAL_DEF_START,
			[GAMEPAD_AXIS_TRIGGERLEFT]  = DEF_NAME_BUTTON("ZL"),
			[GAMEPAD_AXIS_TRIGGERRIGHT] = DEF_NAME_BUTTON("ZR"),
		};

		names = switch_names;
	}

	const char *str = NULL;

	if (names)
		str = GetStringFromAxisList(names, axis, strtype, inv);
	if (str == NULL)
		str = GetStringFromAxisList(base_names, axis, strtype, inv);
	if (str)
		return str;

	return "Unknown";
}

#undef DEF_NAME_AXIS
#undef DEF_NAME_TRIGGER
#undef DEF_NAME_BUTTON

#undef PARTIAL_DEF_START

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

	// satya nadella keys
	{KEY_LEFTWIN, "leftwin"},
	{KEY_RIGHTWIN, "rightwin"},
	{KEY_MENU, "menu"},

	{KEY_LSHIFT, "lshift"},
	{KEY_RSHIFT, "rshift"},
	{KEY_LCTRL, "lctrl"},
	{KEY_RCTRL, "rctrl"},
	{KEY_LALT, "lalt"},
	{KEY_RALT, "ralt"},

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

	// virtual keys for mouse buttons and gamepad buttons
	{KEY_MOUSE1+0,"mouse1"},
	{KEY_MOUSE1+1,"mouse2"},
	{KEY_MOUSE1+2,"mouse3"},
	{KEY_MOUSE1+3,"mouse4"},
	{KEY_MOUSE1+4,"mouse5"},
	{KEY_MOUSE1+5,"mouse6"},
	{KEY_MOUSE1+6,"mouse7"},
	{KEY_MOUSE1+7,"mouse8"},
	{KEY_2MOUSE1+0,"sec_mouse1"},
	{KEY_2MOUSE1+1,"sec_mouse2"},
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

#define DEF_GAMEPAD_NAME(btn, name) {KEY_GAMEPAD+GAMEPAD_BUTTON_##btn, name}
#define DEF_GAMEPAD_AXIS(ax, name) \
	{KEY_AXES+GAMEPAD_AXIS_##ax, name}, \
	{KEY_INV_AXES+GAMEPAD_AXIS_##ax, name "-"}

	DEF_GAMEPAD_NAME(A, "a button"),
	DEF_GAMEPAD_NAME(B, "b button"),
	DEF_GAMEPAD_NAME(X, "x button"),
	DEF_GAMEPAD_NAME(Y, "y button"),

	DEF_GAMEPAD_NAME(BACK, "back button"),
	DEF_GAMEPAD_NAME(GUIDE, "guide button"),
	DEF_GAMEPAD_NAME(START, "start button"),
	DEF_GAMEPAD_NAME(LEFTSTICK, "left stick"),
	DEF_GAMEPAD_NAME(RIGHTSTICK, "right stick"),

	DEF_GAMEPAD_NAME(LEFTSHOULDER, "left shoulder"),
	DEF_GAMEPAD_NAME(RIGHTSHOULDER, "right shoulder"),

	DEF_GAMEPAD_NAME(DPAD_UP, "d-pad up"),
	DEF_GAMEPAD_NAME(DPAD_DOWN, "d-pad down"),
	DEF_GAMEPAD_NAME(DPAD_LEFT, "d-pad left"),
	DEF_GAMEPAD_NAME(DPAD_RIGHT, "d-pad right"),

	DEF_GAMEPAD_NAME(MISC1, "gamepad misc 1"),
	DEF_GAMEPAD_NAME(PADDLE1, "paddle 1"),
	DEF_GAMEPAD_NAME(PADDLE2, "paddle 2"),
	DEF_GAMEPAD_NAME(PADDLE3, "paddle 3"),
	DEF_GAMEPAD_NAME(PADDLE4, "paddle 4"),
	DEF_GAMEPAD_NAME(TOUCHPAD, "touchpad"),

	DEF_GAMEPAD_AXIS(LEFTX, "left stick x"),
	DEF_GAMEPAD_AXIS(LEFTY, "left stick y"),

	DEF_GAMEPAD_AXIS(RIGHTX, "right stick x"),
	DEF_GAMEPAD_AXIS(RIGHTY, "right stick y"),

	DEF_GAMEPAD_AXIS(TRIGGERLEFT, "left trigger"),
	DEF_GAMEPAD_AXIS(TRIGGERRIGHT, "right trigger"),

#undef DEF_GAMEPAD_NAME
#undef DEF_GAMEPAD_AXIS

	{KEY_DBLMOUSE1+0, "dblmouse1"},
	{KEY_DBLMOUSE1+1, "dblmouse2"},
	{KEY_DBLMOUSE1+2, "dblmouse3"},
	{KEY_DBLMOUSE1+3, "dblmouse4"},
	{KEY_DBLMOUSE1+4, "dblmouse5"},
	{KEY_DBLMOUSE1+5, "dblmouse6"},
	{KEY_DBLMOUSE1+6, "dblmouse7"},
	{KEY_DBLMOUSE1+7, "dblmouse8"},
	{KEY_DBL2MOUSE1+0, "dblsec_mouse1"},
	{KEY_DBL2MOUSE1+1, "dblsec_mouse2"},
	{KEY_DBL2MOUSE1+2, "dblsec_mouse3"},
	{KEY_DBL2MOUSE1+3, "dblsec_mouse4"},
	{KEY_DBL2MOUSE1+4, "dblsec_mouse5"},
	{KEY_DBL2MOUSE1+5, "dblsec_mouse6"},
	{KEY_DBL2MOUSE1+6, "dblsec_mouse7"},
	{KEY_DBL2MOUSE1+7, "dblsec_mouse8"}
};

#define NUMKEYNAMES (sizeof(keynames) / sizeof(keyname_t))

static keyname_t displaykeynames[] =
{
	{KEY_SPACE, "Space Bar"},
	{KEY_CAPSLOCK, "Caps Lock"},
	{KEY_ENTER, "Enter"},
	{KEY_TAB, "Tab"},
	{KEY_ESCAPE, "Escape"},
	{KEY_BACKSPACE, "Backspace"},

	{KEY_NUMLOCK, "Num Lock"},
	{KEY_SCROLLLOCK, "Scroll Lock"},

#ifdef _WIN32
	{KEY_LEFTWIN, "Left Windows"},
	{KEY_RIGHTWIN, "Right Windows"},
#else
	{KEY_LEFTWIN, "Left Super"},
	{KEY_RIGHTWIN, "Right Super"},
#endif

	{KEY_MENU, "Menu"},

	{KEY_LSHIFT, "Left Shift"},
	{KEY_RSHIFT, "Right Shift"},
	{KEY_LCTRL, "Left Ctrl"},
	{KEY_RCTRL, "Right Ctrl"},
	{KEY_LALT, "Left Alt"},
	{KEY_RALT, "Right Alt"},

	{KEY_KEYPAD0, "Keypad 0"},
	{KEY_KEYPAD1, "Keypad 1"},
	{KEY_KEYPAD2, "Keypad 2"},
	{KEY_KEYPAD3, "Keypad 3"},
	{KEY_KEYPAD4, "Keypad 4"},
	{KEY_KEYPAD5, "Keypad 5"},
	{KEY_KEYPAD6, "Keypad 6"},
	{KEY_KEYPAD7, "Keypad 7"},
	{KEY_KEYPAD8, "Keypad 8"},
	{KEY_KEYPAD9, "Keypad 9"},
	{KEY_PLUSPAD, "Keypad +"},
	{KEY_MINUSPAD, "Keypad -"},
	{KEY_KPADSLASH, "Keypad /"},
	{KEY_KPADDEL, "Keypad ."},

	{KEY_UPARROW, "Up Arrow"},
	{KEY_DOWNARROW, "Down Arrow"},
	{KEY_LEFTARROW, "Left Arrow"},
	{KEY_RIGHTARROW, "Right Arrow"},

	{KEY_HOME, "Home"},
	{KEY_END, "End"},
	{KEY_PGUP, "Page Up"},
	{KEY_PGDN, "Page Down"},
	{KEY_INS, "Insert"},
	{KEY_DEL, "Delete"},

	{KEY_F1, "F1"},
	{KEY_F2, "F2"},
	{KEY_F3, "F3"},
	{KEY_F4, "F4"},
	{KEY_F5, "F5"},
	{KEY_F6, "F6"},
	{KEY_F7, "F7"},
	{KEY_F8, "F8"},
	{KEY_F9, "F9"},
	{KEY_F10, "F10"},
	{KEY_F11, "F11"},
	{KEY_F12, "F12"},

	{'`', "Tilde"},
	{KEY_PAUSE, "Pause/Break"},

	{KEY_MOUSE1+0, "Left Mouse Button"},
	{KEY_MOUSE1+1, "Right Mouse Button"},
	{KEY_MOUSE1+2, "Middle Mouse Button"},
	{KEY_MOUSE1+3, "X1 Mouse Button"},
	{KEY_MOUSE1+4, "X2 Mouse Button"},

	{KEY_2MOUSE1+0, "Sec. Mouse Left Button"},
	{KEY_2MOUSE1+1, "Sec. Mouse Right Button"},
	{KEY_2MOUSE1+2, "Sec. Mouse Middle Button"},
	{KEY_2MOUSE1+3, "Sec. Mouse X1 Button"},
	{KEY_2MOUSE1+4, "Sec. Mouse X2 Button"},

	{KEY_MOUSEWHEELUP, "Mouse Wheel Up"},
	{KEY_MOUSEWHEELDOWN, "Mouse Wheel Down"},
	{KEY_2MOUSEWHEELUP, "Sec. Mouse Wheel Up"},
	{KEY_2MOUSEWHEELDOWN, "Sec. Mouse Wheel Down"},

#define DEF_GAMEPAD_NAME(btn, name) {KEY_GAMEPAD+GAMEPAD_BUTTON_##btn, name}
#define DEF_GAMEPAD_AXIS(ax, name) {KEY_AXES+GAMEPAD_AXIS_##ax, name}

	DEF_GAMEPAD_NAME(A, "A Button"),
	DEF_GAMEPAD_NAME(B, "B Button"),
	DEF_GAMEPAD_NAME(X, "X Button"),
	DEF_GAMEPAD_NAME(Y, "Y Button"),

	DEF_GAMEPAD_NAME(BACK, "Back Button"),
	DEF_GAMEPAD_NAME(GUIDE, "Guide Button"),
	DEF_GAMEPAD_NAME(START, "Start Button"),
	DEF_GAMEPAD_NAME(LEFTSTICK, "Left Stick Button"),
	DEF_GAMEPAD_NAME(RIGHTSTICK, "Right Stick Button"),

	DEF_GAMEPAD_NAME(LEFTSHOULDER, "Left Shoulder"),
	DEF_GAMEPAD_NAME(RIGHTSHOULDER, "Right Shoulder"),

	DEF_GAMEPAD_NAME(DPAD_UP, "D-Pad Up"),
	DEF_GAMEPAD_NAME(DPAD_DOWN, "D-Pad Down"),
	DEF_GAMEPAD_NAME(DPAD_LEFT, "D-Pad Left"),
	DEF_GAMEPAD_NAME(DPAD_RIGHT, "D-Pad Right"),

	DEF_GAMEPAD_NAME(MISC1, "Gamepad Misc. 1"),
	DEF_GAMEPAD_NAME(PADDLE1, "Paddle 1"),
	DEF_GAMEPAD_NAME(PADDLE2, "Paddle 2"),
	DEF_GAMEPAD_NAME(PADDLE3, "Paddle 3"),
	DEF_GAMEPAD_NAME(PADDLE4, "Paddle 4"),
	DEF_GAMEPAD_NAME(TOUCHPAD, "Touchpad"),

	{KEY_INV_AXES + GAMEPAD_AXIS_LEFTX, "Left Stick \x1C"},
	{KEY_AXES     + GAMEPAD_AXIS_LEFTX, "Left Stick \x1D"},
	{KEY_INV_AXES + GAMEPAD_AXIS_LEFTY, "Left Stick \x1A"},
	{KEY_AXES     + GAMEPAD_AXIS_LEFTY, "Left Stick \x1B"},
	{KEY_INV_AXES + GAMEPAD_AXIS_RIGHTX, "Right Stick \x1C"},
	{KEY_AXES     + GAMEPAD_AXIS_RIGHTX, "Right Stick \x1D"},
	{KEY_INV_AXES + GAMEPAD_AXIS_RIGHTY, "Right Stick \x1A"},
	{KEY_AXES     + GAMEPAD_AXIS_RIGHTY, "Right Stick \x1B"},

	DEF_GAMEPAD_AXIS(TRIGGERLEFT, "Left Trigger"),
	DEF_GAMEPAD_AXIS(TRIGGERRIGHT, "Right Trigger"),

#undef DEF_GAMEPAD_NAME
#undef DEF_GAMEPAD_AXIS
};

#define NUMDISPLAYKEYNAMES (sizeof(displaykeynames) / sizeof(keyname_t))

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
// Returns the name of a key (or virtual key for mouse and gamepad)
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
	snprintf(keynamestr, sizeof keynamestr, "KEY%d", keynum);
	return keynamestr;
}

const char *G_GetDisplayNameForKey(INT32 keynum)
{
	static char keynamestr[32];

	UINT32 j;

	// find a description for special keys
	for (j = 0; j < NUMDISPLAYKEYNAMES; j++)
		if (displaykeynames[j].keynum == keynum)
			return displaykeynames[j].name;

	// return a string with the ascii char if displayable
	if (keynum > ' ' && keynum <= 'z' && keynum != KEY_CONSOLE)
	{
		snprintf(keynamestr, sizeof keynamestr, "%c Key", toupper((char)keynum));
		return keynamestr;
	}

	// unnamed mouse buttons
	if (keynum >= KEY_MOUSE1 && keynum <= KEY_MOUSE1+7)
	{
		j = (keynum - KEY_MOUSE1) + 1;
		snprintf(keynamestr, sizeof keynamestr, "Mouse Button #%d", j);
		return keynamestr;
	}
	else if (keynum >= KEY_2MOUSE1 && keynum <= KEY_2MOUSE1+7)
	{
		j = (keynum - KEY_2MOUSE1) + 1;
		snprintf(keynamestr, sizeof keynamestr, "Sec. Mouse Button #%d", j);
		return keynamestr;
	}

	// create a name for unknown keys
	snprintf(keynamestr, sizeof keynamestr, "Unknown Key %d", keynum);
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

const char *G_GamepadTypeToString(gamepadtype_e type)
{
	static const char *names[] = {
		"xbox-360",
		"xbox-one",
		"xbox-series-xs",
		"xbox-elite",
		"ps3",
		"ps4",
		"ps5",
		"switch-pro",
		"switch-joy-con-grip",
		"switch-joy-con-left",
		"switch-joy-con-right",
		"stadia",
		"amazon-luna",
		"steam-controller",
		"virtual",
		"unknown"
	};

	return names[type];
}

void G_InitGamepads(void)
{
	for (UINT8 i = 0; i < NUM_GAMEPADS; i++)
		gamepads[i].num = i;
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
		gamecontroldefault[i][GC_JUMP       ][1] = GAMEPAD_KEY(A); // A
		gamecontroldefault[i][GC_SPIN       ][1] = GAMEPAD_KEY(X); // X
		gamecontroldefault[i][GC_CUSTOM1    ][1] = GAMEPAD_KEY(B); // B
		gamecontroldefault[i][GC_CUSTOM2    ][1] = GAMEPAD_KEY(Y); // Y
		gamecontroldefault[i][GC_CUSTOM3    ][1] = GAMEPAD_KEY(LEFTSTICK); // Left Stick
		gamecontroldefault[i][GC_CENTERVIEW ][1] = GAMEPAD_KEY(RIGHTSHOULDER); // R1
		gamecontroldefault[i][GC_CAMTOGGLE  ][1] = GAMEPAD_KEY(LEFTSHOULDER); // L1
		gamecontroldefault[i][GC_SCREENSHOT ][1] = GAMEPAD_KEY(BACK); // Back
		gamecontroldefault[i][GC_SYSTEMMENU ][0] = GAMEPAD_KEY(START); // Start
		gamecontroldefault[i][GC_TOSSFLAG   ][1] = GAMEPAD_KEY(DPAD_UP); // D-Pad Up
		gamecontroldefault[i][GC_WEAPONPREV ][1] = GAMEPAD_KEY(DPAD_LEFT); // D-Pad Left
		gamecontroldefault[i][GC_WEAPONNEXT ][1] = GAMEPAD_KEY(DPAD_RIGHT); // D-Pad Right
		gamecontroldefault[i][GC_SCORES     ][1] = GAMEPAD_KEY(DPAD_DOWN); // D-Pad Down
		gamecontroldefault[i][GC_VIEWPOINTNEXT][1] = GAMEPAD_KEY(RIGHTSTICK); // Right Stick
		gamecontroldefault[i][GC_FIRE       ][1] = GAMEPAD_AXIS(TRIGGERRIGHT); // R2
		gamecontroldefault[i][GC_FIRENORMAL ][1] = GAMEPAD_AXIS(TRIGGERLEFT); // L2

		// Second player only has gamepad defaults
		gamecontrolbisdefault[i][GC_JUMP       ][1] = GAMEPAD_KEY(A); // A
		gamecontrolbisdefault[i][GC_SPIN       ][1] = GAMEPAD_KEY(X); // X
		gamecontrolbisdefault[i][GC_CUSTOM1    ][1] = GAMEPAD_KEY(B); // B
		gamecontrolbisdefault[i][GC_CUSTOM2    ][1] = GAMEPAD_KEY(Y); // Y
		gamecontrolbisdefault[i][GC_CUSTOM3    ][1] = GAMEPAD_KEY(LEFTSTICK); // Left Stick
		gamecontrolbisdefault[i][GC_CENTERVIEW ][1] = GAMEPAD_KEY(RIGHTSHOULDER); // R1
		gamecontrolbisdefault[i][GC_CAMTOGGLE  ][1] = GAMEPAD_KEY(LEFTSHOULDER); // L1
		gamecontrolbisdefault[i][GC_SCREENSHOT ][1] = GAMEPAD_KEY(BACK); // Back
		//gamecontrolbisdefault[i][GC_SYSTEMMENU ][0] = GAMEPAD_KEY(START); // Start
		gamecontrolbisdefault[i][GC_TOSSFLAG   ][1] = GAMEPAD_KEY(DPAD_UP); // D-Pad Up
		gamecontrolbisdefault[i][GC_WEAPONPREV ][1] = GAMEPAD_KEY(DPAD_LEFT); // D-Pad Left
		gamecontrolbisdefault[i][GC_WEAPONNEXT ][1] = GAMEPAD_KEY(DPAD_RIGHT); // D-Pad Right
		//gamecontrolbisdefault[i][GC_SCORES     ][1] = GAMEPAD_KEY(DPAD_DOWN); // D-Pad Down
		gamecontrolbisdefault[i][GC_VIEWPOINTNEXT][1] = GAMEPAD_KEY(RIGHTSTICK); // Right Stick
		gamecontrolbisdefault[i][GC_FIRE       ][1] = GAMEPAD_AXIS(TRIGGERRIGHT); // R2
		gamecontrolbisdefault[i][GC_FIRENORMAL ][1] = GAMEPAD_AXIS(TRIGGERLEFT); // L2
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

INT32 G_CheckDoubleUsage(INT32 keynum, boolean modify, UINT8 player)
{
	INT32 result = GC_NULL;
	INT32 i;
	for (i = 0; i < NUM_GAMECONTROLS; i++)
	{
		if (gamecontrol[i][0] == keynum && player != 2)
		{
			result = i;
			if (modify) gamecontrol[i][0] = KEY_NULL;
		}
		if (gamecontrol[i][1] == keynum && player != 2)
		{
			result = i;
			if (modify) gamecontrol[i][1] = KEY_NULL;
		}
		if (gamecontrolbis[i][0] == keynum && player != 1)
		{
			result = i;
			if (modify) gamecontrolbis[i][0] = KEY_NULL;
		}
		if (gamecontrolbis[i][1] == keynum && player != 1)
		{
			result = i;
			if (modify) gamecontrolbis[i][1] = KEY_NULL;
		}
		if (result && !modify)
			return result;
	}
	
	return result;
}

static INT32 G_FilterSpecialKeys(INT32 keyidx, INT32 *keynum1, INT32 *keynum2)
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
	INT32 keynum, keynum1, keynum2;
	INT32 player = ((void*)gc == (void*)&gamecontrolbis ? 1 : 0);

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
	keynum2 = G_KeyNameToNum(COM_Argv(3));
	keynum = G_FilterSpecialKeys(0, &keynum1, &keynum2);

	if (keynum >= 0)
	{
		(void)G_CheckDoubleUsage(keynum, true, player+1);

		// if keynum was rejected, try it again with keynum2
		if (!keynum && keynum2)
		{
			keynum1 = keynum2; // push down keynum2
			keynum2 = 0;
			keynum = G_FilterSpecialKeys(0, &keynum1, &keynum2);
			if (keynum >= 0)
				(void)G_CheckDoubleUsage(keynum, true, player+1);
		}
	}

	if (keynum >= 0)
		gc[numctrl][0] = keynum;

	if (keynum2)
	{
		keynum = G_FilterSpecialKeys(1, &keynum1, &keynum2);
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

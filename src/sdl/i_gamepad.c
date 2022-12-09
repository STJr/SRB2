// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2022 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  i_gamepad.c
/// \brief Gamepads

#ifdef HAVE_SDL
#include "../i_gamepad.h"
#include "../i_system.h"
#include "../doomdef.h"
#include "../d_main.h"
#include "../d_netcmd.h"
#include "../g_game.h"
#include "../m_argv.h"
#include "../m_menu.h"
#include "../z_zone.h"

#include "SDL.h"
#include "SDL_joystick.h"
#include "sdlmain.h"

static void Controller_ChangeDevice(UINT8 num);
static void Controller_Close(UINT8 num);
static void Controller_StopRumble(UINT8 num);

static ControllerInfo controllers[NUM_GAMEPADS];

static boolean rumble_supported = false;
static boolean rumble_paused = false;

// This attempts to initialize the gamepad subsystems
static boolean InitGamepadSubsystems(void)
{
	if (M_CheckParm("-noxinput"))
		SDL_SetHintWithPriority(SDL_HINT_XINPUT_ENABLED, "0", SDL_HINT_OVERRIDE);
	if (M_CheckParm("-nohidapi"))
		SDL_SetHintWithPriority(SDL_HINT_JOYSTICK_HIDAPI, "0", SDL_HINT_OVERRIDE);

	if (SDL_WasInit(GAMEPAD_INIT_FLAGS) == 0)
	{
		if (SDL_InitSubSystem(GAMEPAD_INIT_FLAGS) == -1)
		{
			CONS_Printf(M_GetText("Couldn't initialize game controller subsystems: %s\n"), SDL_GetError());
			return false;
		}
	}

	return true;
}

void I_InitGamepads(void)
{
	if (M_CheckParm("-nojoy"))
		return;

	CONS_Printf("I_InitGamepads()...\n");

	if (!InitGamepadSubsystems())
		return;

	rumble_supported = !M_CheckParm("-norumble");

	for (UINT8 i = 0; i < NUM_GAMEPADS; i++)
		controllers[i].info = &gamepads[i];

	for (UINT8 i = 0; i < NUM_GAMEPADS; i++)
		Controller_ChangeDevice(i);
}

INT32 I_NumGamepads(void)
{
	if (SDL_WasInit(GAMEPAD_INIT_FLAGS) == GAMEPAD_INIT_FLAGS)
		return SDL_NumJoysticks();
	else
		return 0;
}

// From the SDL source code
#define USB_VENDOR_MICROSOFT    0x045e
#define USB_VENDOR_PDP          0x0e6f
#define USB_VENDOR_POWERA_ALT   0x20d6

#define USB_PRODUCT_XBOX_ONE_ELITE_SERIES_1                 0x02e3
#define USB_PRODUCT_XBOX_ONE_ELITE_SERIES_2                 0x0b00
#define USB_PRODUCT_XBOX_ONE_ELITE_SERIES_2_BLUETOOTH       0x0b05
#define USB_PRODUCT_XBOX_SERIES_X                           0x0b12
#define USB_PRODUCT_XBOX_SERIES_X_BLE                       0x0b13
#define USB_PRODUCT_XBOX_SERIES_X_VICTRIX_GAMBIT            0x02d6
#define USB_PRODUCT_XBOX_SERIES_X_PDP_BLUE                  0x02d9
#define USB_PRODUCT_XBOX_SERIES_X_PDP_AFTERGLOW             0x02da
#define USB_PRODUCT_XBOX_SERIES_X_POWERA_FUSION_PRO2        0x4001
#define USB_PRODUCT_XBOX_SERIES_X_POWERA_SPECTRA            0x4002

static boolean IsJoystickXboxOneElite(Uint16 vendor_id, Uint16 product_id)
{
	if (vendor_id == USB_VENDOR_MICROSOFT) {
		if (product_id == USB_PRODUCT_XBOX_ONE_ELITE_SERIES_1 ||
			product_id == USB_PRODUCT_XBOX_ONE_ELITE_SERIES_2 ||
			product_id == USB_PRODUCT_XBOX_ONE_ELITE_SERIES_2_BLUETOOTH) {
			return true;
		}
	}

	return false;
}

static boolean IsJoystickXboxSeriesXS(Uint16 vendor_id, Uint16 product_id)
{
	if (vendor_id == USB_VENDOR_MICROSOFT) {
		if (product_id == USB_PRODUCT_XBOX_SERIES_X ||
			product_id == USB_PRODUCT_XBOX_SERIES_X_BLE) {
			return true;
		}
	}
	else if (vendor_id == USB_VENDOR_PDP) {
		if (product_id == USB_PRODUCT_XBOX_SERIES_X_VICTRIX_GAMBIT ||
			product_id == USB_PRODUCT_XBOX_SERIES_X_PDP_BLUE ||
			product_id == USB_PRODUCT_XBOX_SERIES_X_PDP_AFTERGLOW) {
			return true;
		}
	}
	else if (vendor_id == USB_VENDOR_POWERA_ALT) {
		if ((product_id >= 0x2001 && product_id <= 0x201a) ||
			product_id == USB_PRODUCT_XBOX_SERIES_X_POWERA_FUSION_PRO2 ||
			product_id == USB_PRODUCT_XBOX_SERIES_X_POWERA_SPECTRA) {
			return true;
		}
	}

	return false;
}

// Opens a controller device
static boolean Controller_OpenDevice(UINT8 which, INT32 devindex)
{
	if (SDL_WasInit(GAMEPAD_INIT_FLAGS) == 0)
	{
		CONS_Debug(DBG_GAMELOGIC, M_GetText("Game controller subsystems not started\n"));
		return false;
	}

	if (devindex <= 0)
		return false;

	if (SDL_NumJoysticks() == 0)
	{
		CONS_Debug(DBG_GAMELOGIC, M_GetText("Found no controllers on this system\n"));
		return false;
	}

	devindex--;

	if (!SDL_IsGameController(devindex))
	{
		CONS_Debug(DBG_GAMELOGIC, M_GetText("Device index %d isn't a game controller\n"), devindex);
		return false;
	}

	ControllerInfo *controller = &controllers[which];
	SDL_GameController *newdev = SDL_GameControllerOpen(devindex);

	// Handle the edge case where the device <-> controller index assignment can change due to hotplugging
	// This indexing is SDL's responsibility and there's not much we can do about it.
	//
	// Example:
	// 1. Plug Controller A   -> Index 0 opened
	// 2. Plug Controller B   -> Index 1 opened
	// 3. Unplug Controller A -> Index 0 closed, Index 1 active
	// 4. Unplug Controller B -> Index 0 inactive, Index 1 closed
	// 5. Plug Controller B   -> Index 0 opened
	// 6. Plug Controller A   -> Index 0 REPLACED, opened as Controller A; Index 1 is now Controller B
	if (controller->dev)
	{
		if (controller->dev == newdev // same device, nothing to do
			|| (newdev == NULL && SDL_GameControllerGetAttached(controller->dev))) // we failed, but already have a working device
			return true;

		// Else, we're changing devices, so close the controller
		CONS_Debug(DBG_GAMELOGIC, M_GetText("Controller %d device is changing; closing controller...\n"), which);
		Controller_Close(which);
	}

	if (newdev == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, M_GetText("Controller %d: Couldn't open device - %s\n"), which, SDL_GetError());
		controller->started = false;
	}
	else
	{
		controller->dev = newdev;
		controller->joydev = SDL_GameControllerGetJoystick(controller->dev);
		controller->started = true;

		CONS_Debug(DBG_GAMELOGIC, M_GetText("Controller %d: %s\n"), which, SDL_GameControllerName(controller->dev));

	#define GAMEPAD_TYPE_CASE(ctrl) \
		case SDL_CONTROLLER_TYPE_##ctrl: \
			controller->info->type = GAMEPAD_TYPE_##ctrl; \
			break

		switch (SDL_GameControllerGetType(newdev))
		{
			GAMEPAD_TYPE_CASE(UNKNOWN);
			GAMEPAD_TYPE_CASE(XBOX360);
			GAMEPAD_TYPE_CASE(XBOXONE);
			GAMEPAD_TYPE_CASE(PS3);
			GAMEPAD_TYPE_CASE(PS4);
			GAMEPAD_TYPE_CASE(PS5);
			GAMEPAD_TYPE_CASE(NINTENDO_SWITCH_PRO);
			GAMEPAD_TYPE_CASE(GOOGLE_STADIA);
			GAMEPAD_TYPE_CASE(AMAZON_LUNA);
			GAMEPAD_TYPE_CASE(VIRTUAL);
			default: break;
		}

	#undef GAMEPAD_BUTTON_CASE

		// Check the device vendor and product to find out what controller this actually is
		Uint16 vendor = SDL_JoystickGetDeviceVendor(devindex);
		Uint16 product = SDL_JoystickGetDeviceProduct(devindex);

		if (IsJoystickXboxSeriesXS(vendor, product))
			controller->info->type = GAMEPAD_TYPE_XBOX_SERIES_XS;
		else if (IsJoystickXboxOneElite(vendor, product))
			controller->info->type = GAMEPAD_TYPE_XBOX_ELITE;

		CONS_Debug(DBG_GAMELOGIC, M_GetText("    Type: %s\n"), G_GamepadTypeToString(controller->info->type));

		// Change the ring LEDs on Xbox 360 controllers
		// TODO: Doesn't seem to work?
		SDL_GameControllerSetPlayerIndex(controller->dev, which);

		// Check if rumble is supported
		if (SDL_GameControllerHasRumble(controller->dev) == SDL_TRUE)
		{
			controller->info->rumble.supported = true;
			CONS_Debug(DBG_GAMELOGIC, M_GetText("    Rumble supported: Yes\n"));
		}
		else
		{
			controller->info->rumble.supported = false;
			CONS_Debug(DBG_GAMELOGIC, M_GetText("    Rumble supported: No\n"));;
		}

		controller->info->connected = true;
	}

	return controller->started;
}

// Initializes a controller
static INT32 Controller_Init(SDL_GameController **newcontroller, UINT8 which, INT32 *index)
{
	ControllerInfo *info = &controllers[which];
	SDL_GameController *controller = NULL;
	INT32 device = (*index);

	if (device && SDL_IsGameController(device - 1))
		controller = SDL_GameControllerOpen(device - 1);
	if (newcontroller)
		(*newcontroller) = controller;

	if (controller && info->dev == controller) // don't override an active device
		(*index) = I_GetControllerIndex(info->dev) + 1;
	else if (controller && Controller_OpenDevice(which, device))
	{
		// SDL's device indexes are unstable, so cv_usegamepad may not match
		// the actual device index. So let's cheat a bit and find the device's current index.
		info->lastindex = I_GetControllerIndex(info->dev) + 1;
		return 1;
	}
	else
	{
		(*index) = 0;
		return 0;
	}

	return -1;
}

// Changes a controller's device
static void Controller_ChangeDevice(UINT8 num)
{
	SDL_GameController *newjoy = NULL;

	if (!Controller_Init(&newjoy, num, &cv_usegamepad[num].value) && controllers[num].lastindex)
		Controller_Close(num);

	I_CloseInactiveController(newjoy);
}

static boolean Controller_IsAnyUsingDevice(SDL_GameController *dev)
{
	for (UINT8 i = 0; i < NUM_GAMEPADS; i++)
	{
		if (controllers[i].dev == dev)
			return true;
	}

	return false;
}

static boolean Controller_IsAnyOtherUsingDevice(SDL_GameController *dev, UINT8 thisjoy)
{
	for (UINT8 i = 0; i < NUM_GAMEPADS; i++)
	{
		if (i == thisjoy)
			continue;
		else if (controllers[i].dev == dev)
			return true;
	}

	return false;
}

void I_ControllerDeviceAdded(INT32 which)
{
	if (!SDL_IsGameController(which))
		return;

	SDL_GameController *newjoy = SDL_GameControllerOpen(which);

	CONS_Debug(DBG_GAMELOGIC, "Gamepad device index %d added\n", which + 1);

	// Because SDL's device index is unstable, we're going to cheat here a bit:
	// For the first controller setting that is NOT active:
	// 1. Set cv_usegamepadX.value to the new device index (this does not change what is written to config.cfg)
	// 2. Set OTHERS' cv_usegamepadX.value to THEIR new device index, because it likely changed
	//    * If device doesn't exist, switch cv_usegamepad back to default value (.string)
	//      * BUT: If that default index is being occupied, use ANOTHER cv_usegamepad's default value!
	for (UINT8 this = 0; this < NUM_GAMEPADS && newjoy; this++)
	{
		if ((!controllers[this].dev || !SDL_GameControllerGetAttached(controllers[this].dev))
			&& !Controller_IsAnyOtherUsingDevice(newjoy, this)) // don't override a currently active device
		{
			cv_usegamepad[this].value = which + 1;

			// Go through every other device
			for (UINT8 other = 0; other < NUM_GAMEPADS; other++)
			{
				if (other == this)
				{
					// Don't change this controller's index
					continue;
				}
				else if (controllers[other].dev)
				{
					// Update this controller's index if the device is open
					cv_usegamepad[other].value = I_GetControllerIndex(controllers[other].dev) + 1;
				}
				else if (atoi(cv_usegamepad[other].string) != controllers[this].lastindex
						&& atoi(cv_usegamepad[other].string) != cv_usegamepad[this].value)
				{
					// If the user-set index for the other controller doesn't
					// match this controller's current or former internal index,
					// then use the other controller's internal index
					cv_usegamepad[other].value = atoi(cv_usegamepad[other].string);
				}
				else if (atoi(cv_usegamepad[this].string) != controllers[this].lastindex
						&& atoi(cv_usegamepad[this].string) != cv_usegamepad[this].value)
				{
					// If the user-set index for this controller doesn't match
					// its current or former internal index, then use this
					// controller's internal index
					cv_usegamepad[other].value = atoi(cv_usegamepad[this].string);
				}
				else
				{
					// Try again
					cv_usegamepad[other].value = 0;
					continue;
				}

				break;
			}

			break;
		}
	}

	// Was cv_usegamepad disabled in settings?
	for (UINT8 i = 0; i < NUM_GAMEPADS; i++)
	{
		if (!strcmp(cv_usegamepad[i].string, "0") || !cv_usegamepad[i].value)
			cv_usegamepad[i].value = 0;
		else if (atoi(cv_usegamepad[i].string) <= I_NumGamepads() // don't mess if we intentionally set higher than NumJoys
			     && cv_usegamepad[i].value) // update the cvar ONLY if a device exists
			CV_SetValue(&cv_usegamepad[i], cv_usegamepad[i].value);
	}

	// Update all gamepads' init states
	// This is a little wasteful since cv_usegamepad already calls this, but
	// we need to do this in case CV_SetValue did nothing because the string was already same.
	// if the device is already active, this should do nothing, effectively.
	for (UINT8 i = 0; i < NUM_GAMEPADS; i++)
	{
		Controller_ChangeDevice(i);
		CONS_Debug(DBG_GAMELOGIC, "Controller %d device index: %d\n", i, controllers[i].lastindex);
	}

	if (M_OnGamepadMenu())
		M_UpdateGamepadMenu();

	I_CloseInactiveController(newjoy);
}

void I_ControllerDeviceRemoved(void)
{
	for (UINT8 this = 0; this < NUM_GAMEPADS; this++)
	{
		if (controllers[this].dev && !SDL_GameControllerGetAttached(controllers[this].dev))
		{
			CONS_Debug(DBG_GAMELOGIC, "Controller %d removed, device index: %d\n", this, controllers[this].lastindex);
			G_OnGamepadDisconnect(this);
			Controller_Close(this);
		}

		// Update the device indexes, because they likely changed
		// * If device doesn't exist, switch cv_usegamepad back to default value (.string)
		//   * BUT: If that default index is being occupied, use ANOTHER cv_usegamepad's default value!
		if (controllers[this].dev)
			cv_usegamepad[this].value = controllers[this].lastindex = I_GetControllerIndex(controllers[this].dev) + 1;
		else
		{
			for (UINT8 other = 0; other < NUM_GAMEPADS; other++)
			{
				if (other == this)
					continue;

				if (atoi(cv_usegamepad[this].string) != controllers[other].lastindex)
				{
					// Update this internal index if this user-set index
					// doesn't match the other's former internal index
					cv_usegamepad[this].value = atoi(cv_usegamepad[this].string);
				}
				else if (atoi(cv_usegamepad[other].string) != controllers[other].lastindex)
				{
					// Otherwise, set this internal index to the other's
					// user-set index, if the other user-set index is not the
					// same as the other's former internal index
					cv_usegamepad[this].value = atoi(cv_usegamepad[other].string);
				}
				else
				{
					// Try again
					cv_usegamepad[this].value = 0;
					continue;
				}

				break;
			}
		}

		// Was cv_usegamepad disabled in settings?
		if (!strcmp(cv_usegamepad[this].string, "0"))
			cv_usegamepad[this].value = 0;
		else if (atoi(cv_usegamepad[this].string) <= I_NumGamepads() // don't mess if we intentionally set higher than NumJoys
				 && cv_usegamepad[this].value) // update the cvar ONLY if a device exists
			CV_SetValue(&cv_usegamepad[this], cv_usegamepad[this].value);

		CONS_Debug(DBG_GAMELOGIC, "Controller %d device index: %d\n", this, controllers[this].lastindex);
	}

	if (M_OnGamepadMenu())
		M_UpdateGamepadMenu();
}

// Close the controller device if there isn't any controller using it
void I_CloseInactiveController(SDL_GameController *dev)
{
	if (!Controller_IsAnyUsingDevice(dev))
		SDL_GameControllerClose(dev);
}

// Cheat to get the device index for a game controller handle
INT32 I_GetControllerIndex(SDL_GameController *dev)
{
	INT32 i, count = SDL_NumJoysticks();

	for (i = 0; dev && i < count; i++)
	{
		SDL_GameController *test = SDL_GameControllerOpen(i);
		if (test && test == dev)
			return i;
		else
			I_CloseInactiveController(test);
	}

	return -1;
}

// Changes a gamepad's device
void I_ChangeGamepad(UINT8 which)
{
	if (which >= NUM_GAMEPADS)
		return;

	if (controllers[which].started)
		Controller_StopRumble(which);

	Controller_ChangeDevice(which);
}

// Returns the name of a controller from its index
const char *I_GetGamepadName(INT32 joyindex)
{
	static char joyname[256];
	joyname[0] = '\0';

	if (SDL_WasInit(GAMEPAD_INIT_FLAGS) == GAMEPAD_INIT_FLAGS)
	{
		const char *tempname = SDL_GameControllerNameForIndex(joyindex - 1);
		if (tempname)
			strlcpy(joyname, tempname, sizeof joyname);
	}

	return joyname;
}

// Toggles a gamepad's digital axis setting
void I_SetGamepadDigital(UINT8 which, boolean enable)
{
	if (which >= NUM_GAMEPADS)
		return;

	gamepads[which].digital = enable;
}

static gamepad_t *Controller_GetFromID(SDL_JoystickID which, UINT8 *found)
{
	// Determine the joystick IDs for each current open controller
	for (UINT8 i = 0; i < NUM_GAMEPADS; i++)
	{
		if (which == SDL_JoystickInstanceID(controllers[i].joydev))
		{
			(*found) = i;
			return &gamepads[i];
		}
	}

	(*found) = UINT8_MAX;

	return NULL;
}

void I_HandleControllerButtonEvent(SDL_ControllerButtonEvent evt, Uint32 type)
{
	event_t event;

	gamepad_t *gamepad = Controller_GetFromID(evt.which, &event.which);
	if (gamepad == NULL)
		return;

	if (type == SDL_CONTROLLERBUTTONUP)
		event.type = ev_gamepad_up;
	else if (type == SDL_CONTROLLERBUTTONDOWN)
		event.type = ev_gamepad_down;
	else
		return;

#define GAMEPAD_BUTTON_CASE(btn) \
	case SDL_CONTROLLER_BUTTON_##btn: \
		event.key = GAMEPAD_BUTTON_##btn; \
		break

	switch (evt.button)
	{
		GAMEPAD_BUTTON_CASE(A);
		GAMEPAD_BUTTON_CASE(B);
		GAMEPAD_BUTTON_CASE(X);
		GAMEPAD_BUTTON_CASE(Y);
		GAMEPAD_BUTTON_CASE(BACK);
		GAMEPAD_BUTTON_CASE(GUIDE);
		GAMEPAD_BUTTON_CASE(START);
		GAMEPAD_BUTTON_CASE(LEFTSTICK);
		GAMEPAD_BUTTON_CASE(RIGHTSTICK);
		GAMEPAD_BUTTON_CASE(LEFTSHOULDER);
		GAMEPAD_BUTTON_CASE(RIGHTSHOULDER);
		GAMEPAD_BUTTON_CASE(DPAD_UP);
		GAMEPAD_BUTTON_CASE(DPAD_DOWN);
		GAMEPAD_BUTTON_CASE(DPAD_LEFT);
		GAMEPAD_BUTTON_CASE(DPAD_RIGHT);
		GAMEPAD_BUTTON_CASE(MISC1);
		GAMEPAD_BUTTON_CASE(PADDLE1);
		GAMEPAD_BUTTON_CASE(PADDLE2);
		GAMEPAD_BUTTON_CASE(PADDLE3);
		GAMEPAD_BUTTON_CASE(PADDLE4);
		GAMEPAD_BUTTON_CASE(TOUCHPAD);
		default: return;
	}

#undef GAMEPAD_BUTTON_CASE

	D_PostEvent(&event);
}

void I_HandleControllerAxisEvent(SDL_ControllerAxisEvent evt)
{
	event_t event;

	gamepad_t *gamepad = Controller_GetFromID(evt.which, &event.which);
	if (gamepad == NULL)
		return;

#define GAMEPAD_AXIS_CASE(btn) \
	case SDL_CONTROLLER_AXIS_##btn: \
		event.key = GAMEPAD_AXIS_##btn; \
		break

	switch (evt.axis)
	{
		GAMEPAD_AXIS_CASE(LEFTX);
		GAMEPAD_AXIS_CASE(LEFTY);
		GAMEPAD_AXIS_CASE(RIGHTX);
		GAMEPAD_AXIS_CASE(RIGHTY);
		GAMEPAD_AXIS_CASE(TRIGGERLEFT);
		GAMEPAD_AXIS_CASE(TRIGGERRIGHT);
		default: return;
	}

#undef GAMEPAD_AXIS_CASE

	event.type = ev_gamepad_axis;
	event.x = evt.value;

	D_PostEvent(&event);
}

static void Controller_StopRumble(UINT8 num)
{
	ControllerInfo *controller = &controllers[num];

	controller->rumble.large_magnitude = 0;
	controller->rumble.small_magnitude = 0;
	controller->rumble.time_left = 0;
	controller->rumble.expiration = 0;

	gamepad_t *gamepad = controller->info;

	gamepad->rumble.active = false;
	gamepad->rumble.paused = false;
	gamepad->rumble.data.large_magnitude = 0;
	gamepad->rumble.data.small_magnitude = 0;
	gamepad->rumble.data.duration = 0;

	if (gamepad->rumble.supported)
		SDL_GameControllerRumble(controller->dev, 0, 0, 0);
}

static void Controller_Close(UINT8 num)
{
	ControllerInfo *controller = &controllers[num];

	// Close the game controller device
	if (controller->dev)
	{
		Controller_StopRumble(num);
		SDL_GameControllerClose(controller->dev);
	}

	controller->dev = NULL;
	controller->joydev = NULL;
	controller->lastindex = -1;
	controller->started = false;

	// Reset gamepad info
	gamepad_t *gamepad = controller->info;

	if (gamepad)
	{
		gamepad->type = GAMEPAD_TYPE_UNKNOWN;
		gamepad->connected = false;
		gamepad->digital = false;
		gamepad->rumble.supported = false;

		for (UINT8 i = 0; i < NUM_GAMEPAD_BUTTONS; i++)
			gamepad->buttons[i] = 0;

		for (UINT8 i = 0; i < NUM_GAMEPAD_AXES; i++)
			gamepad->axes[i] = 0;
	}
}

void I_ShutdownGamepads(void)
{
	for (UINT8 i = 0; i < NUM_GAMEPADS; i++)
		Controller_Close(i);
}

boolean I_RumbleSupported(void)
{
	return rumble_supported;
}

static boolean Controller_Rumble(ControllerInfo *c)
{
	if (SDL_GameControllerRumble(c->dev, c->rumble.large_magnitude, c->rumble.small_magnitude, 0) == -1)
		return false;

	return true;
}

void I_ToggleControllerRumble(boolean unpause)
{
	if (!I_RumbleSupported() || rumble_paused == !unpause)
		return;

	rumble_paused = !unpause;

	for (UINT8 i = 0; i < NUM_GAMEPADS; i++)
	{
		ControllerInfo *controller = &controllers[i];
		if (!controller->started || !controller->info->rumble.supported)
			continue;

		if (rumble_paused)
			SDL_GameControllerRumble(controller->dev, 0, 0, 0);
		else if (!controller->info->rumble.paused)
		{
			if (!Controller_Rumble(controller))
				controller->rumble.expiration = controller->rumble.time_left = 0;
		}
	}
}

void I_UpdateControllers(void)
{
	if (SDL_WasInit(GAMEPAD_INIT_FLAGS) != GAMEPAD_INIT_FLAGS)
		return;

	for (UINT8 i = 0; i < NUM_GAMEPADS; i++)
	{
		ControllerInfo *controller = &controllers[i];
		if (!controller->started || !controller->info->rumble.supported || controller->info->rumble.paused)
			continue;

		if (controller->rumble.expiration &&
			SDL_TICKS_PASSED(SDL_GetTicks(), controller->rumble.expiration))
		{
			// Enough time has passed, so stop the effect
			Controller_StopRumble(i);
		}
	}

	SDL_JoystickUpdate();
}

// Converts duration in tics to milliseconds
#define TICS_TO_MS(tics) ((INT32)(tics * (1000.0f/TICRATE)))

boolean I_RumbleGamepad(UINT8 which, const haptic_t *effect)
{
	if (!I_RumbleSupported() || which >= NUM_GAMEPADS)
		return false;

	ControllerInfo *controller = &controllers[which];
	if (!controller->started || !controller->info->rumble.supported)
		return false;

	UINT16 duration = min(TICS_TO_MS(effect->duration), UINT16_MAX);
	UINT16 large_magnitude = max(0, min(effect->large_magnitude, UINT16_MAX));
	UINT16 small_magnitude = max(0, min(effect->small_magnitude, UINT16_MAX));

	CONS_Debug(DBG_GAMELOGIC, "Starting rumble effect for controller %d:\n", which);
	CONS_Debug(DBG_GAMELOGIC, "    Large motor magnitude: %f\n", large_magnitude / 65535.0f);
	CONS_Debug(DBG_GAMELOGIC, "    Small motor magnitude: %f\n", small_magnitude / 65535.0f);

	if (!duration)
		CONS_Debug(DBG_GAMELOGIC, "    Duration: forever\n");
	else
		CONS_Debug(DBG_GAMELOGIC, "    Duration: %dms\n", duration);

	controller->rumble.large_magnitude = large_magnitude;
	controller->rumble.small_magnitude = small_magnitude;

	if (!rumble_paused && !Controller_Rumble(controller))
	{
		Controller_StopRumble(which);
		return false;
	}

	controller->rumble.time_left = 0;

	if (duration)
		controller->rumble.expiration = SDL_GetTicks() + duration;
	else
		controller->rumble.expiration = 0;

	// Update gamepad rumble info
	gamepad_t *gamepad = controller->info;

	gamepad->rumble.active = true;
	gamepad->rumble.paused = false;
	gamepad->rumble.data.large_magnitude = effect->large_magnitude;
	gamepad->rumble.data.small_magnitude = effect->small_magnitude;
	gamepad->rumble.data.duration = effect->duration;

	return true;
}

#undef TICS_TO_MS

#define SET_MOTOR_FREQ(type) \
	if (!I_RumbleSupported() || which >= NUM_GAMEPADS) \
		return false; \
 \
	ControllerInfo *controller = &controllers[which]; \
	if (!controller->started || !controller->info->rumble.supported) \
		return false; \
 \
	gamepad_t *gamepad = controller->info; \
	if (gamepad->rumble.data.type##_magnitude == freq) \
		return true; \
 \
	UINT16 frequency = max(0, min(freq, UINT16_MAX)); \
 \
	controller->rumble.type##_magnitude = frequency; \
 \
	if (!rumble_paused && !gamepad->rumble.paused && !Controller_Rumble(controller)) \
	{ \
		Controller_StopRumble(which); \
		return false; \
	} \
 \
	gamepad->rumble.data.type##_magnitude = freq; \
	gamepad->rumble.active = true; \
	return true

boolean I_SetGamepadLargeMotorFreq(UINT8 which, fixed_t freq)
{
	SET_MOTOR_FREQ(large);
}

boolean I_SetGamepadSmallMotorFreq(UINT8 which, fixed_t freq)
{
	SET_MOTOR_FREQ(small);
}

void I_SetGamepadRumblePaused(UINT8 which, boolean pause)
{
	if (!I_RumbleSupported() || which >= NUM_GAMEPADS)
		return;

	ControllerInfo *controller = &controllers[which];
	if (!controller->started || !controller->info->rumble.supported)
		return;

	if (pause == controller->info->rumble.paused)
		return;
	else if (pause)
	{
		if (!rumble_paused)
			SDL_GameControllerRumble(controller->dev, 0, 0, 0);

		if (controller->rumble.expiration)
		{
			controller->rumble.time_left = controller->rumble.expiration - SDL_GetTicks();
			controller->rumble.expiration = 0;
		}
	}
	else
	{
		if (!rumble_paused)
			SDL_GameControllerRumble(controller->dev, controller->rumble.large_magnitude, controller->rumble.small_magnitude, 0);

		if (controller->rumble.time_left)
			controller->rumble.expiration = SDL_GetTicks() + controller->rumble.time_left;
	}

	controller->info->rumble.paused = pause;
}

boolean I_GetGamepadRumbleSupported(UINT8 which)
{
	if (!I_RumbleSupported() || which >= NUM_GAMEPADS)
		return false;

	ControllerInfo *controller = &controllers[which];
	if (!controller->started)
		return false;

	return controller->info->rumble.supported;
}

boolean I_GetGamepadRumblePaused(UINT8 which)
{
	if (!I_RumbleSupported() || which >= NUM_GAMEPADS)
		return false;

	ControllerInfo *controller = &controllers[which];
	if (!controller->started || !controller->info->rumble.supported)
		return false;

	return controller->info->rumble.paused;
}

void I_StopGamepadRumble(UINT8 which)
{
	if (!I_RumbleSupported() || which >= NUM_GAMEPADS)
		return;

	ControllerInfo *controller = &controllers[which];
	if (!controller->started || !controller->info->rumble.supported)
		return;

	Controller_StopRumble(which);
}
#endif

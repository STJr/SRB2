#include <nds.h>
#include <fat.h>

#include "../doomdef.h"
#include "../d_main.h"
#include "../i_system.h"
#include "../i_joy.h"

UINT8 graphics_started = 0;

UINT8 keyboard_started = 0;

static volatile tic_t ticcount;


UINT32 I_GetFreeMem(UINT32 *total)
{
	*total = 0;
	return 0;
}

tic_t I_GetTime(void)
{
	return ticcount;
}

void I_Sleep(void){}

void I_GetEvent(void)
{
	// Mappings of DS keys to SRB2 keys
	UINT32 dskeys[] =
	{
		KEY_A,
		KEY_B,
		KEY_X,
		KEY_Y,
		KEY_L,
		KEY_R,
		KEY_START,
		KEY_SELECT
	};

	event_t event;
	UINT32 held, up, down;
	UINT32 i;

	// Check how the state has changed since last time
	scanKeys();

	// For the d-pad, we only care about the current state
	held = keysHeld();
	event.type = ev_joystick;
	event.data1 = 0;	// First (and only) axis set

	if (held & KEY_LEFT) event.data2 = -1;
	else if (held & KEY_RIGHT) event.data2 = 1;
	else event.data2 = 0;

	if (held & KEY_UP) event.data3 = -1;
	else if (held & KEY_DOWN) event.data3 = 1;
	else event.data3 = 0;

	D_PostEvent(&event);

	// For the buttons, we need to report changes in state
	up = keysUp();
	down = keysDown();
	for (i = 0; i < sizeof(dskeys)/sizeof(dskeys[0]); i++)
	{
		// Has this button's state changed?
		if ((up | down) & dskeys[i])
		{
			event.type = (up & dskeys[i]) ? ev_keyup : ev_keydown;
			event.data1 = KEY_JOY1 + i;
			D_PostEvent(&event);
		}
	}
}

void I_OsPolling(void)
{
	I_GetEvent();
}

ticcmd_t *I_BaseTiccmd(void)
{
	static ticcmd_t emptyticcmd;
	return &emptyticcmd;
}

ticcmd_t *I_BaseTiccmd2(void)
{
	static ticcmd_t emptyticcmd2;
	return &emptyticcmd2;
}

void I_Quit(void)
{
	exit(0);
}

void I_Error(const char *error, ...)
{
	va_list argptr;

        va_start(argptr, error);
        viprintf(error, argptr);
        va_end(argptr);

	for(;;);
}

void I_Tactile(FFType Type, const JoyFF_t *Effect)
{
	(void)Type;
	(void)Effect;
}

void I_Tactile2(FFType Type, const JoyFF_t *Effect)
{
	(void)Type;
	(void)Effect;
}

void I_JoyScale(void){}

void I_JoyScale2(void){}

void I_InitJoystick(void)
{
	Joystick.bGamepadStyle = true;
}

void I_InitJoystick2(void){}

INT32 I_NumJoys(void)
{
	return 0;
}

const char *I_GetJoyName(INT32 joyindex)
{
	(void)joyindex;
	return NULL;
}

void I_SetupMumble(void)
{
}

void I_UpdateMumble(const MumblePos_t *MPos)
{
	(void)MPos;
}

void I_OutputMsg(const char *error, ...)
{
	va_list argptr;

	va_start(argptr, error);
	viprintf(error, argptr);
	va_end(argptr);
}

void I_StartupMouse(void){}

void I_StartupMouse2(void){}

void I_StartupKeyboard(void){}

INT32 I_GetKey(void)
{
	return 0;
}

static void NDS_VBlankHandler(void)
{
	ticcount++;
}

void I_StartupTimer(void)
{
	irqSet(IRQ_VBLANK, NDS_VBlankHandler);
}

void I_AddExitFunc(void (*func)())
{
	(void)func;
}

void I_RemoveExitFunc(void (*func)())
{
	(void)func;
}

// Adapted in part from the devkitPro examples.
INT32 I_StartupSystem(void)
{
	lcdMainOnTop();

	videoSetModeSub(MODE_0_2D);
	vramSetBankC(VRAM_C_MAIN_BG);	// Get this mapped *out* of the sub BG
	vramSetBankI(VRAM_I_SUB_BG_0x06208000);

	// The background VRAM that's mapped starts at 0x06208000.
	// The map base is specified in an offset of multiples of 2 KB
	// from 0x06200000, and the tile base in multiples of 16 KB.
	// We put the tiles at the start and the map 2 KB from the end
	// (i.e. 14 KB from the start).
	// The map base is then at 0x0620B800 = 0x06200000 + 16 * 0x800 + 7 * 0x800,
	// and the tile base is at 0x06208000 = 0x06200000 + 2 * 0x4000.
	consoleInit(NULL, 0, BgType_Text4bpp, BgSize_T_256x256, 16+7, 2, false, true);

	// start FAT filesystem code, required for reading SD card
	if(!fatInitDefault())
		I_Error("Couldn't init FAT.");

	return 0;
}

void I_ShutdownSystem(void){}

void I_GetDiskFreeSpace(INT64* freespace)
{
	*freespace = 0;
}

char *I_GetUserName(void)
{
	return NULL;
}

INT32 I_mkdir(const char *dirname, INT32 unixright)
{
	(void)dirname;
	(void)unixright;
	return -1;
}

const CPUInfoFlags *I_CPUInfo(void)
{
	return NULL;
}

const char *I_LocateWad(void)
{
	return NULL;
}

void I_GetJoystickEvents(void){}

void I_GetJoystick2Events(void){}

void I_GetMouseEvents(void){}

char *I_GetEnv(const char *name)
{
	(void)name;
	return NULL;
}

INT32 I_PutEnv(char *variable)
{
	(void)variable;
	return -1;
}

void I_RegisterSysCommands(void) {}

#include "../sdl/dosstr.c"

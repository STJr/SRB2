#define LOG_TAG "SRB2"

#include "../doomdef.h"
#include "../i_system.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <utils/Log.h>

#define MEMINFO_FILE "/proc/meminfo"
#define MEMTOTAL "MemTotal:"
#define MEMFREE "MemFree:"

UINT8 graphics_started = 0;

UINT8 keyboard_started = 0;

static INT64 start_time; // as microseconds since the epoch

// I should probably return how much memory is remaining
// for this process, considering Android's process memory limit.
UINT32 I_GetFreeMem(UINT32 *total)
{
  // what the heck?  sysinfo() is partially missing in bionic?
  /* struct sysinfo si; */
  /* if(sysinfo(&si) != 0) { */
  /*   I_Error("Couldn't invoke sysinfo()...?"); */
  /* } */
  /* return si.freeram; */
  char buf[1024];
  char *memTag;
  UINT32 freeKBytes;
  UINT32 totalKBytes;
  INT32 n;
  INT32 meminfo_fd = -1;

  meminfo_fd = open(MEMINFO_FILE, O_RDONLY);
  n = read(meminfo_fd, buf, 1023);
  close(meminfo_fd);

  if (n < 0)
    {
      // Error
      *total = 0L;
      return 0;
    }

  buf[n] = '\0';
  if (NULL == (memTag = strstr(buf, MEMTOTAL)))
    {
      // Error
      *total = 0L;
      return 0;
    }

  memTag += sizeof (MEMTOTAL);
  totalKBytes = atoi(memTag);

  if (NULL == (memTag = strstr(buf, MEMFREE)))
    {
      // Error
      *total = 0L;
      return 0;
    }

  memTag += sizeof (MEMFREE);
  freeKBytes = atoi(memTag);

  if (total)
    *total = totalKBytes << 10;
  return freeKBytes << 10;
}

INT64 current_time_in_ps() {
  struct timeval t;
  gettimeofday(&t, NULL);
  return (t.tv_sec * (INT64)1000000) + t.tv_usec;
}

tic_t I_GetTime(void)
{
  INT64 since_start = current_time_in_ps() - start_time;
  return (since_start*TICRATE)/1000000;
}

void I_Sleep(void){}

void I_GetEvent(void){}

void I_OsPolling(void){}

ticcmd_t *I_BaseTiccmd(void)
{
  return NULL;
}

ticcmd_t *I_BaseTiccmd2(void)
{
  return NULL;
}

void I_Quit(void)
{
  LOGD("SRB2 quitting!");
  exit(0);
}

void I_Error(const char *error, ...)
{
  va_list argptr;
  char logbuf[8192];

  va_start(argptr, error);
  vsprintf(logbuf, error, argptr);
  va_end(argptr);

  LOGE(logbuf);
  exit(-1);
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

void I_InitJoystick(void){}

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

void I_OutputMsg(const char *fmt, ...)
{
  va_list argptr;
  char logbuf[8192];

  va_start(argptr, fmt);
  vsprintf(logbuf, fmt, argptr);
  va_end(argptr);

  LOGD(logbuf);
}

void I_StartupMouse(void){}

void I_StartupMouse2(void){}

void I_StartupKeyboard(void){}

INT32 I_GetKey(void)
{
  return 0;
}

void I_StartupTimer(void) {
  struct timeval t;
  gettimeofday(&t, NULL);
  start_time = (t.tv_sec * 1000000) + t.tv_usec;
}

void I_AddExitFunc(void (*func)())
{
  (void)func;
}

void I_RemoveExitFunc(void (*func)())
{
  (void)func;
}

INT32 I_StartupSystem(void)
{
  return -1;
}

void I_ShutdownSystem(void){}

void I_GetDiskFreeSpace(INT64* freespace)
{
  *freespace = 0;
}

char *I_GetUserName(void)
{
  return "Android";
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
  return "/sdcard/srb2";
}

void I_GetJoystickEvents(void){}

void I_GetJoystick2Events(void){}

void I_GetMouseEvents(void){}

char *I_GetEnv(const char *name)
{
  LOGW("I_GetEnv() called?!");
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

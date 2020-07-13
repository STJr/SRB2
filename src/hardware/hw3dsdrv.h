// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2001 by DooM Legacy Team.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file hw3dsdrv.h
/// \brief 3D sound import/export prototypes for low-level hardware interface

#ifndef __HW_3DS_DRV_H__
#define __HW_3DS_DRV_H__

// Use standart hardware API
#include "hw_dll.h"
#include "hws_data.h"

#if defined (HAVE_SDL) || !defined (HWD)
EXPORT void HWRAPI(Shutdown) (void);
#endif

// Use standart Init and Shutdown functions

EXPORT INT32    HWRAPI (Startup) (I_Error_t FatalErrorFunction, snddev_t *snd_dev);
EXPORT u_int  HWRAPI (AddSfx) (sfx_data_t *sfx);
EXPORT INT32    HWRAPI (AddSource) (source3D_data_t *src, u_int sfxhandle);
EXPORT INT32    HWRAPI (StartSource) (INT32 handle);
EXPORT void   HWRAPI (StopSource) (INT32 handle);
EXPORT INT32    HWRAPI (GetHW3DSVersion) (void);
EXPORT void   HWRAPI (BeginFrameUpdate) (void);
EXPORT void   HWRAPI (EndFrameUpdate) (void);
EXPORT INT32    HWRAPI (IsPlaying) (INT32 handle);
EXPORT void   HWRAPI (UpdateListener) (listener_data_t *data, INT32 num);
EXPORT void   HWRAPI (UpdateSourceParms) (INT32 handle, INT32 vol, INT32 sep);
EXPORT void   HWRAPI (SetGlobalSfxVolume) (INT32 volume);
EXPORT INT32    HWRAPI (SetCone) (INT32 handle, cone_def_t *cone_def);
EXPORT void   HWRAPI (Update3DSource) (INT32 handle, source3D_pos_t *data);
EXPORT INT32    HWRAPI (ReloadSource) (INT32 handle, u_int sfxhandle);
EXPORT void   HWRAPI (KillSource) (INT32 handle);
EXPORT void   HWRAPI (KillSfx) (u_int sfxhandle);
EXPORT void   HWRAPI (GetHW3DSTitle) (char *buf, size_t size);


#if !defined (_CREATE_DLL_)

struct hardware3ds_s
{
	Startup             pfnStartup;
	AddSfx              pfnAddSfx;
	AddSource           pfnAddSource;
	StartSource         pfnStartSource;
	StopSource          pfnStopSource;
	GetHW3DSVersion     pfnGetHW3DSVersion;
	BeginFrameUpdate    pfnBeginFrameUpdate;
	EndFrameUpdate      pfnEndFrameUpdate;
	IsPlaying           pfnIsPlaying;
	UpdateListener      pfnUpdateListener;
	UpdateSourceParms   pfnUpdateSourceParms;
	SetGlobalSfxVolume  pfnSetGlobalSfxVolume;
	SetCone             pfnSetCone;
	Update3DSource      pfnUpdate3DSource;
	ReloadSource        pfnReloadSource;
	KillSource          pfnKillSource;
	KillSfx             pfnKillSfx;
	Shutdown            pfnShutdown;
	GetHW3DSTitle       pfnGetHW3DSTitle;
};

extern struct hardware3ds_s hw3ds_driver;

#define HW3DS hw3ds_driver


#endif  // _CREATE_DLL_

#endif // __HW_3DS_DRV_H__

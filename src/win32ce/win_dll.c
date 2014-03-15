// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 1998-2000 by DooM Legacy Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//-----------------------------------------------------------------------------
/// \file
/// \brief load and initialise the 3D driver DLL

#include "../doomdef.h"
#ifdef HWRENDER
#include "../hardware/hw_drv.h"        // get the standard 3D Driver DLL exports prototypes
#endif

#ifdef HW3SOUND
#include "../hardware/hw3dsdrv.h"      // get the 3D sound driver DLL export prototypes
#endif

#include "win_dll.h"
#include "win_main.h"       // I_GetLastErrorMsgBox()

#if defined(HWRENDER) || defined(HW3SOUND)
typedef struct loadfunc_s {
	LPCSTR fnName;
	LPVOID fnPointer;
} loadfunc_t;

// --------------------------------------------------------------------------
// Load a DLL, returns the HMODULE handle or NULL
// --------------------------------------------------------------------------
static inline HMODULE LoadDLL (LPCSTR dllName, loadfunc_t *funcTable)
{
	LPVOID      funcPtr;
	loadfunc_t *loadfunc;
	HMODULE     hModule;

	if ((hModule = LoadLibraryA(dllName)) != NULL)
	{
		// get function pointers for all functions we use
		for (loadfunc = funcTable; loadfunc->fnName != NULL; loadfunc++)
		{
			funcPtr = GetProcAddress(hModule, loadfunc->fnName);
			if (!funcPtr) {
				//I_GetLastErrorMsgBox ();
				MessageBoxA(NULL, va("The '%s' haven't the good specification (function %s missing)\n\n"
				           "You must use dll from the same zip of this exe\n", dllName, loadfunc->fnName),
				           "Error", MB_OK|MB_ICONINFORMATION);
				return FALSE;
			}
			// store function address
			*((LPVOID*)loadfunc->fnPointer) = funcPtr;
		}
	}
	else
	{
		MessageBoxA(NULL, va("LoadLibrary() FAILED : couldn't load '%s'\r\n", dllName), "Warning", MB_OK|MB_ICONINFORMATION);
		//I_GetLastErrorMsgBox ();
	}

	return hModule;
}


// --------------------------------------------------------------------------
// Unload the DLL
// --------------------------------------------------------------------------
static inline VOID UnloadDLL (HMODULE* pModule)
{
	if (FreeLibrary(*pModule))
		*pModule = NULL;
	else
		I_GetLastErrorMsgBox ();
}
#endif

// ==========================================================================
// STANDARD 3D DRIVER DLL FOR DOOM LEGACY
// ==========================================================================

// note : the 3D driver loading should be put somewhere else..

#ifdef HWRENDER
static HMODULE hwdModule = NULL;

static loadfunc_t hwdFuncTable[] = {
	{"_Init@4",            &hwdriver.pfnInit},
	{"_Shutdown@0",        &hwdriver.pfnShutdown},
	{"_GetModeList@8",     &hwdriver.pfnGetModeList},
	{"_SetPalette@8",      &hwdriver.pfnSetPalette},
	{"_FinishUpdate@4",    &hwdriver.pfnFinishUpdate},
	{"_Draw2DLine@12",     &hwdriver.pfnDraw2DLine},
	{"_DrawPolygon@16",    &hwdriver.pfnDrawPolygon},
	{"_SetBlend@4",        &hwdriver.pfnSetBlend},
	{"_ClearBuffer@12",    &hwdriver.pfnClearBuffer},
	{"_SetTexture@4",      &hwdriver.pfnSetTexture},
	{"_ReadRect@24",       &hwdriver.pfnReadRect},
	{"_GClipRect@20",      &hwdriver.pfnGClipRect},
	{"_ClearMipMapCache@0",&hwdriver.pfnClearMipMapCache},
	{"_SetSpecialState@8", &hwdriver.pfnSetSpecialState},
	{"_DrawMD2@16",        &hwdriver.pfnDrawMD2},
	{"_SetTransform@4",    &hwdriver.pfnSetTransform},
	{"_GetTextureUsed@0",  &hwdriver.pfnGetTextureUsed},
	{"_GetRenderVersion@0",&hwdriver.pfnGetRenderVersion},
	{NULL,NULL}
};

BOOL Init3DDriver (LPCSTR dllName)
{
	hwdModule = LoadDLL(dllName, hwdFuncTable);
	return (hwdModule != NULL);
}

VOID Shutdown3DDriver (VOID)
{
	UnloadDLL(&hwdModule);
}
#endif

#ifdef HW3SOUND
static HMODULE hwsModule = NULL;

static loadfunc_t hwsFuncTable[] = {
	{"_Startup@8",              &hw3ds_driver.pfnStartup},
	{"_Shutdown@0",             &hw3ds_driver.pfnShutdown},
	{"_AddSfx@4",               &hw3ds_driver.pfnAddSfx},
	{"_AddSource@8",            &hw3ds_driver.pfnAddSource},
	{"_StartSource@4",          &hw3ds_driver.pfnStartSource},
	{"_StopSource@4",           &hw3ds_driver.pfnStopSource},
	{"_GetHW3DSVersion@0",      &hw3ds_driver.pfnGetHW3DSVersion},
	{"_BeginFrameUpdate@0",     &hw3ds_driver.pfnBeginFrameUpdate},
	{"_EndFrameUpdate@0",       &hw3ds_driver.pfnEndFrameUpdate},
	{"_IsPlaying@4",            &hw3ds_driver.pfnIsPlaying},
	{"_UpdateListener@8",       &hw3ds_driver.pfnUpdateListener},
	{"_UpdateSourceParms@12",   &hw3ds_driver.pfnUpdateSourceParms},
	{"_SetCone@8",              &hw3ds_driver.pfnSetCone},
	{"_SetGlobalSfxVolume@4",   &hw3ds_driver.pfnSetGlobalSfxVolume},
	{"_Update3DSource@8",       &hw3ds_driver.pfnUpdate3DSource},
	{"_ReloadSource@8",         &hw3ds_driver.pfnReloadSource},
	{"_KillSource@4",           &hw3ds_driver.pfnKillSource},
	{"_KillSfx@4",              &hw3ds_driver.pfnKillSfx},
	{"_GetHW3DSTitle@8",        &hw3ds_driver.pfnGetHW3DSTitle},
	{NULL, NULL}
};

BOOL Init3DSDriver(LPCSTR dllName)
{
	hwsModule = LoadDLL(dllName, hwsFuncTable);
	return (hwsModule != NULL);
}

VOID Shutdown3DSDriver (VOID)
{
	UnloadDLL(&hwsModule);
}
#endif

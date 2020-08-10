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

#ifdef _WINDOWS

#include "win_dll.h"
#include "win_main.h"       // I_ShowLastError()

#if defined(HWRENDER) || defined(HW3SOUND)
typedef struct loadfunc_s {
	LPCSTR fnName;
	LPVOID fnPointer;
} loadfunc_t;

// --------------------------------------------------------------------------
// Load a DLL, returns the HMODULE handle or NULL
// --------------------------------------------------------------------------
static HMODULE LoadDLL (LPCSTR dllName, loadfunc_t *funcTable)
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
				I_ShowLastError(FALSE);
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
		I_ShowLastError(FALSE);
		MessageBoxA(NULL, va("LoadLibrary() FAILED : couldn't load '%s'\r\n", dllName), "Warning", MB_OK|MB_ICONINFORMATION);
	}

	return hModule;
}


// --------------------------------------------------------------------------
// Unload the DLL
// --------------------------------------------------------------------------
static VOID UnloadDLL (HMODULE* pModule)
{
	if (FreeLibrary(*pModule))
		*pModule = NULL;
	else
		I_ShowLastError(TRUE);
}
#endif

// ==========================================================================
// STANDARD 3D DRIVER DLL FOR DOOM LEGACY
// ==========================================================================

// note : the 3D driver loading should be put somewhere else..

#ifdef HWRENDER
static HMODULE hwdModule = NULL;

static loadfunc_t hwdFuncTable[] = {
#ifdef _X86_
	{"Init@4",              &hwdriver.pfnInit},
	{"Shutdown@0",          &hwdriver.pfnShutdown},
	{"GetModeList@8",       &hwdriver.pfnGetModeList},
	{"SetPalette@4",        &hwdriver.pfnSetPalette},
	{"FinishUpdate@4",      &hwdriver.pfnFinishUpdate},
	{"Draw2DLine@12",       &hwdriver.pfnDraw2DLine},
	{"DrawPolygon@16",      &hwdriver.pfnDrawPolygon},
	{"RenderSkyDome@4",     &hwdriver.pfnRenderSkyDome},
	{"SetBlend@4",          &hwdriver.pfnSetBlend},
	{"ClearBuffer@12",      &hwdriver.pfnClearBuffer},
	{"SetTexture@4",        &hwdriver.pfnSetTexture},
	{"UpdateTexture@4",     &hwdriver.pfnUpdateTexture},
	{"ReadRect@24",         &hwdriver.pfnReadRect},
	{"GClipRect@20",        &hwdriver.pfnGClipRect},
	{"ClearMipMapCache@0",  &hwdriver.pfnClearMipMapCache},
	{"SetSpecialState@8",   &hwdriver.pfnSetSpecialState},
	{"DrawModel@16",        &hwdriver.pfnDrawModel},
	{"SetTransform@4",      &hwdriver.pfnSetTransform},
	{"GetTextureUsed@0",    &hwdriver.pfnGetTextureUsed},
	{"GetRenderVersion@0",  &hwdriver.pfnGetRenderVersion},
#ifdef SHUFFLE
	{"PostImgRedraw@4",     &hwdriver.pfnPostImgRedraw},
#endif
	{"FlushScreenTextures@0",&hwdriver.pfnFlushScreenTextures},
	{"StartScreenWipe@0",   &hwdriver.pfnStartScreenWipe},
	{"EndScreenWipe@0",     &hwdriver.pfnEndScreenWipe},
	{"DoScreenWipe@0",      &hwdriver.pfnDoScreenWipe},
	{"DrawIntermissionBG@0",&hwdriver.pfnDrawIntermissionBG},
	{"MakeScreenTexture@0", &hwdriver.pfnMakeScreenTexture},
	{"MakeScreenFinalTexture@0", &hwdriver.pfnMakeScreenFinalTexture},
	{"DrawScreenFinalTexture@8", &hwdriver.pfnDrawScreenFinalTexture},
#else
	{"Init",                &hwdriver.pfnInit},
	{"Shutdown",            &hwdriver.pfnShutdown},
	{"GetModeList",         &hwdriver.pfnGetModeList},
	{"SetPalette",          &hwdriver.pfnSetPalette},
	{"FinishUpdate",        &hwdriver.pfnFinishUpdate},
	{"Draw2DLine",          &hwdriver.pfnDraw2DLine},
	{"DrawPolygon",         &hwdriver.pfnDrawPolygon},
	{"RenderSkyDome",       &hwdriver.pfnRenderSkyDome},
	{"SetBlend",            &hwdriver.pfnSetBlend},
	{"ClearBuffer",         &hwdriver.pfnClearBuffer},
	{"SetTexture",          &hwdriver.pfnSetTexture},
	{"UpdateTexture",       &hwdriver.pfnUpdateTexture},
	{"ReadRect",            &hwdriver.pfnReadRect},
	{"GClipRect",           &hwdriver.pfnGClipRect},
	{"ClearMipMapCache",    &hwdriver.pfnClearMipMapCache},
	{"SetSpecialState",     &hwdriver.pfnSetSpecialState},
	{"DrawModel",           &hwdriver.pfnDrawModel},
	{"SetTransform",        &hwdriver.pfnSetTransform},
	{"GetTextureUsed",      &hwdriver.pfnGetTextureUsed},
	{"GetRenderVersion",    &hwdriver.pfnGetRenderVersion},
#ifdef SHUFFLE
	{"PostImgRedraw",       &hwdriver.pfnPostImgRedraw},
#endif
	{"FlushScreenTextures", &hwdriver.pfnFlushScreenTextures},
	{"StartScreenWipe",     &hwdriver.pfnStartScreenWipe},
	{"EndScreenWipe",       &hwdriver.pfnEndScreenWipe},
	{"DoScreenWipe",        &hwdriver.pfnDoScreenWipe},
	{"DrawIntermissionBG",  &hwdriver.pfnDrawIntermissionBG},
	{"MakeScreenTexture",   &hwdriver.pfnMakeScreenTexture},
	{"MakeScreenFinalTexture", &hwdriver.pfnMakeScreenFinalTexture},
	{"DrawScreenFinalTexture", &hwdriver.pfnDrawScreenFinalTexture},
#endif
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
#ifdef _X86_
	{"Startup@8",              &hw3ds_driver.pfnStartup},
	{"Shutdown@0",             &hw3ds_driver.pfnShutdown},
	{"AddSfx@4",               &hw3ds_driver.pfnAddSfx},
	{"AddSource@8",            &hw3ds_driver.pfnAddSource},
	{"StartSource@4",          &hw3ds_driver.pfnStartSource},
	{"StopSource@4",           &hw3ds_driver.pfnStopSource},
	{"GetHW3DSVersion@0",      &hw3ds_driver.pfnGetHW3DSVersion},
	{"BeginFrameUpdate@0",     &hw3ds_driver.pfnBeginFrameUpdate},
	{"EndFrameUpdate@0",       &hw3ds_driver.pfnEndFrameUpdate},
	{"IsPlaying@4",            &hw3ds_driver.pfnIsPlaying},
	{"UpdateListener@8",       &hw3ds_driver.pfnUpdateListener},
	{"UpdateSourceParms@12",   &hw3ds_driver.pfnUpdateSourceParms},
	{"SetCone@8",              &hw3ds_driver.pfnSetCone},
	{"SetGlobalSfxVolume@4",   &hw3ds_driver.pfnSetGlobalSfxVolume},
	{"Update3DSource@8",       &hw3ds_driver.pfnUpdate3DSource},
	{"ReloadSource@8",         &hw3ds_driver.pfnReloadSource},
	{"KillSource@4",           &hw3ds_driver.pfnKillSource},
	{"KillSfx@4",              &hw3ds_driver.pfnKillSfx},
	{"GetHW3DSTitle@8",        &hw3ds_driver.pfnGetHW3DSTitle},
#else
	{"Startup",                &hw3ds_driver.pfnStartup},
	{"Shutdown",               &hw3ds_driver.pfnShutdown},
	{"AddSfx",                 &hw3ds_driver.pfnAddSfx},
	{"AddSource",              &hw3ds_driver.pfnAddSource},
	{"StartSource",            &hw3ds_driver.pfnStartSource},
	{"StopSource",             &hw3ds_driver.pfnStopSource},
	{"GetHW3DSVersion",        &hw3ds_driver.pfnGetHW3DSVersion},
	{"BeginFrameUpdate",       &hw3ds_driver.pfnBeginFrameUpdate},
	{"EndFrameUpdate",         &hw3ds_driver.pfnEndFrameUpdate},
	{"IsPlaying",              &hw3ds_driver.pfnIsPlaying},
	{"UpdateListener",         &hw3ds_driver.pfnUpdateListener},
	{"UpdateSourceParms",      &hw3ds_driver.pfnUpdateSourceParms},
	{"SetCone",                &hw3ds_driver.pfnSetCone},
	{"SetGlobalSfxVolume",     &hw3ds_driver.pfnSetGlobalSfxVolume},
	{"Update3DSource",         &hw3ds_driver.pfnUpdate3DSource},
	{"ReloadSource",           &hw3ds_driver.pfnReloadSource},
	{"KillSource",             &hw3ds_driver.pfnKillSource},
	{"KillSfx",                &hw3ds_driver.pfnKillSfx},
	{"GetHW3DSTitle",          &hw3ds_driver.pfnGetHW3DSTitle},
#endif
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
#endif //_WINDOWS

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

/// \file
/// \brief Tool for dynamic referencing of hardware rendering functions
///
///	Declaration and definition of the HW rendering
///	functions do have the same name. Originally, the
///	implementation was stored in a separate library.
///	For SDL, we need some function to return the addresses,
///	otherwise we have a conflict with the compiler.

#include "hwsym_sdl.h"
#include "../doomdef.h"

#ifdef _MSC_VER
#pragma warning(disable : 4214 4244)
#endif

#ifdef HAVE_SDL

#include "SDL.h"

#ifdef _MSC_VER
#pragma warning(default : 4214 4244)
#endif

#ifndef NOLOADSO
#include "SDL_loadso.h"
#endif

#define  _CREATE_DLL_  // necessary for Unix AND Windows

#ifdef HWRENDER
#include "../hardware/hw_drv.h"
#include "ogl_sdl.h"
#ifdef STATIC_OPENGL
#include "../hardware/r_opengl/r_opengl.h"
#endif
#endif

#ifdef HW3SOUND
#include "../hardware/hw3dsdrv.h"
#endif

#define GETFUNC(func) \
	else if (0 == strcmp(#func, funcName)) \
		funcPointer = &func \
//
//
/**	\brief	The *hwSym function

	Stupid function to return function addresses

	\param	funcName	the name of the function
	\param	handle	an object to look in(NULL for self)

	\return	void
*/
//
void *hwSym(const char *funcName,void *handle)
{
	void *funcPointer = NULL;
#ifdef HWRENDER
	if (0 == strcmp("SetPalette", funcName))
		funcPointer = &OglSdlSetPalette;

	GETFUNC(Init);
	GETFUNC(Draw2DLine);
	GETFUNC(DrawPolygon);
	GETFUNC(DrawIndexedTriangles);
	GETFUNC(RenderSkyDome);
	GETFUNC(SetBlend);
	GETFUNC(ClearBuffer);
	GETFUNC(SetTexture);
	GETFUNC(UpdateTexture);
	GETFUNC(ReadRect);
	GETFUNC(GClipRect);
	GETFUNC(ClearMipMapCache);
	GETFUNC(SetSpecialState);
	GETFUNC(GetTextureUsed);
	GETFUNC(DrawModel);
	GETFUNC(CreateModelVBOs);
	GETFUNC(SetTransform);
	GETFUNC(PostImgRedraw);
	GETFUNC(FlushScreenTextures);
	GETFUNC(StartScreenWipe);
	GETFUNC(EndScreenWipe);
	GETFUNC(DoScreenWipe);
	GETFUNC(DrawIntermissionBG);
	GETFUNC(MakeScreenTexture);
	GETFUNC(MakeScreenFinalTexture);
	GETFUNC(DrawScreenFinalTexture);

	GETFUNC(LoadShaders);
	GETFUNC(KillShaders);
	GETFUNC(SetShader);
	GETFUNC(UnSetShader);

	GETFUNC(SetShaderInfo);
	GETFUNC(LoadCustomShader);
	GETFUNC(InitCustomShaders);

#else //HWRENDER
	if (0 == strcmp("FinishUpdate", funcName))
		return funcPointer; //&FinishUpdate;
#endif //!HWRENDER
#ifdef STATIC3DS
	GETFUNC(Startup);
	GETFUNC(AddSfx);
	GETFUNC(AddSource);
	GETFUNC(StartSource);
	GETFUNC(StopSource);
	GETFUNC(GetHW3DSVersion);
	GETFUNC(BeginFrameUpdate);
	GETFUNC(EndFrameUpdate);
	GETFUNC(IsPlaying);
	GETFUNC(UpdateListener);
	GETFUNC(UpdateSourceParms);
	GETFUNC(SetGlobalSfxVolume);
	GETFUNC(SetCone);
	GETFUNC(Update3DSource);
	GETFUNC(ReloadSource);
	GETFUNC(KillSource);
	GETFUNC(Shutdown);
	GETFUNC(GetHW3DSTitle);
#endif
#ifdef NOLOADSO
	else
		funcPointer = handle;
#else
	else if (handle)
		funcPointer = SDL_LoadFunction(handle,funcName);
#endif
	if (!funcPointer)
		I_OutputMsg("hwSym for %s: %s\n", funcName, SDL_GetError());
	return funcPointer;
}

/**	\brief	The *hwOpen function

	\param	hwfile	Open a handle to the SO

	\return	Handle to SO


*/

void *hwOpen(const char *hwfile)
{
#ifdef NOLOADSO
	(void)hwfile;
	return NULL;
#else
	void *tempso = NULL;
	tempso = SDL_LoadObject(hwfile);
	if (!tempso) I_OutputMsg("hwOpen of %s: %s\n", hwfile, SDL_GetError());
	return tempso;
#endif
}

/**	\brief	The hwClose function

	\param	handle	Close the handle of the SO

	\return	void


*/

void hwClose(void *handle)
{
#ifdef NOLOADSO
	(void)handle;
#else
	SDL_UnloadObject(handle);
#endif
}
#endif

// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 2005 by SRB2 Jr. Team.
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
/// \brief Win32 DLL and Shared Objects API definitions

#ifndef __HWR_DLL_H__
#define __HWR_DLL_H__

// Function declaration for exports from the DLL :
// EXPORT <return-type> HWRAPI(<function-name>) (<arguments>);
// If _CREATE_DLL_ is defined the above declaration translates to :
// __declspec(dllexport) <return-type> WINAPI <function-name> (<arguments>);
// If _CREATE_DLL_ is NOT DEFINED the above declaration translates to :
// __declspec(dllexport) <return->type> (WINAPI *<function-name>) (<arguments>);

#ifdef _CREATE_DLL_
 #ifdef _WINDOWS
  #ifdef __cplusplus
   #define EXPORT  extern "C" __declspec(dllexport)
  #else
   #define EXPORT  __declspec(dllexport)
  #endif
 #else
  #ifdef __cplusplus
   #define EXPORT  extern "C"
  #else
   #define EXPORT
  #endif
 #endif
 #ifdef _WIN32
  #define HWRAPI(fn)  WINAPI fn
 #else
  #define HWRAPI(fn)  fn
 #endif
#else // _CREATE_DLL_
 #define EXPORT      typedef
 #ifdef _WIN32
  #define HWRAPI(fn)  (WINAPI *fn)
 #else
  #define HWRAPI(fn)  (*fn)
 #endif
#endif

typedef void (*I_Error_t) (const char *error, ...) FUNCIERROR;

// ==========================================================================
//                                                                      MATHS
// ==========================================================================

// Constants
#ifndef M_PIl
#define M_PIl 3.1415926535897932384626433832795029L
#endif
#define DEGREE (0.017453292519943295769236907684883l) // 2*PI/360

void DBG_Printf(const char *lpFmt, ...) /*FUNCPRINTF*/;

#ifdef _WINDOWS
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved);
#elif defined (__CYGWIN__)
void _init() __attribute__((constructor));
void _fini() __attribute__((destructor));
#else
void _init();
void _fini();
#endif

#endif

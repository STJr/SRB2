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
/// \brief transform an unreadable DirectX error code
///	into a meaningful error message.

#ifndef __DX_ERROR_H__
#define __DX_ERROR_H__

//#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

// Displays a message box containing the given formatted string.
//VOID DXErrorMessageBox (LPSTR fmt, ...);

// Returns a pointer to a string describing the given DD, D3D or D3DRM error code.
LPCSTR DXErrorToString (HRESULT error);

#ifdef __cplusplus
};
#endif
#endif // __DX_ERROR_H__

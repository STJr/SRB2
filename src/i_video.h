// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2023 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  i_video.h
/// \brief System specific interface stuff.

#ifndef __I_VIDEO__
#define __I_VIDEO__

#include "doomtype.h"

#ifdef __GNUG__
#pragma interface
#endif

typedef enum
{
	/// Software
	render_soft = 1,

	/// OpenGL
	render_opengl = 2,

	/// Dedicated
	render_none = 3  // for dedicated server
} rendermode_t;

/**	\brief current render mode
*/
extern rendermode_t rendermode;

/**	\brief render mode set by command line arguments
*/
extern rendermode_t chosenrendermode;

/**	\brief setup video mode
*/
void I_StartupGraphics(void);

/**	\brief shutdown video mode
*/
void I_ShutdownGraphics(void);

/**	\brief	The I_SetPalette function

	\param	palette	Takes full 8 bit values

	\return	void
*/
void I_SetPalette(RGBA_t *palette);

/**	\brief Changes the current resolution
*/
void VID_SetSize(INT32 width, INT32 height);

/**	\brief Checks the render state
	\return	true if the renderer changed
*/
boolean VID_CheckRenderer(void);

/**	\brief Load OpenGL mode
*/
void VID_StartupOpenGL(void);

/**	\brief Checks if OpenGL loaded
*/
void VID_CheckGLLoaded(rendermode_t oldrender);

/**	\brief Returns true if the window is maximized, and false if not
*/
boolean VID_IsMaximized(void);

/**	\brief Restores the window
*/
void VID_RestoreWindow(void);

/**	\brief Gets the current display's size; returns true if it succeeded, and false if not
*/
boolean VID_GetNativeResolution(INT32 *width, INT32 *height);

/**	\brief can video system do fullscreen
*/
extern boolean allow_fullscreen;

/**	\brief Update video system without updating frame
*/
void I_UpdateNoBlit(void);

/**	\brief Update video system with updating frame
*/
void I_FinishUpdate(void);

/**	\brief I_FinishUpdate(), but vsync disabled
*/
void I_UpdateNoVsync(void);

/**	\brief	Wait for vertical retrace or pause a bit.

	\param	count	max wait

	\return	void
*/
void I_WaitVBL(INT32 count);

/**	\brief	The I_ReadScreen function

	\param	scr	buffer to copy screen to

	\return	void
*/
void I_ReadScreen(UINT8 *scr);

/**	\brief Start disk icon
*/
void I_BeginRead(void);

/**	\brief Stop disk icon
*/
void I_EndRead(void);

UINT32 I_GetRefreshRate(void);

#endif

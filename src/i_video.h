// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2020 by Sonic Team Junior.
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

typedef enum rendermode_e
{
	// No renderer chosen. Used on dedicated mode.
	render_none = 0,

	// First renderer choice.
	// Must start from 1.
	render_first = 1,

	// Main renderer choices.
	render_soft = render_first,
	render_opengl,

	// Last renderer choice.
	render_last,

	// Renderer count.
	num_renderers = (render_last - 1),
} rendermode_t;

/**	\brief current render mode
*/
extern rendermode_t rendermode;

/**	\brief render mode set by command line arguments
*/
extern rendermode_t chosenrendermode;

/**	\brief use highcolor modes if true
*/
extern boolean highcolor;

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

/**	\brief return the number of video modes
*/
INT32 I_NumVideoModes(void);

/**	\brief	The I_GetVideoModeForSize function

	\param	w	width
	\param	h	height

	\return	vidmode closest to w : h
*/
INT32 I_GetVideoModeForSize(INT32 w, INT32 h);


/**	\brief	The I_SetVideoMode function

	Set the video mode right now,
	the video mode change is delayed until the start of the next refresh
	by setting the setmodeneeded to a value >0
	setup a video mode, this is to be called from the menu


	\param	modenum	video mode to set to

	\return	current video mode
*/
INT32 I_SetVideoMode(INT32 modenum);

/**	\brief	The I_GetVideoModeName function

	\param	modenum	video mode number

	\return	name of video mode
*/
const char *I_GetVideoModeName(INT32 modenum);

/**	\brief Checks the render state

	\return	true if the renderer changed
*/
boolean I_CheckRenderer(void);

/**	\brief Load OpenGL mode
*/
void I_StartupOpenGL(void);

/**	\brief Checks if OpenGL loaded
*/
void I_CheckGLLoaded(rendermode_t oldrender);

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

#endif

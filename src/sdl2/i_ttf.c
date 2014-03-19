// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 2011 by Callum Dickinson.
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
/// \brief SDL_ttf interface code. Necessary for platforms with no framebuffer console systems.

#if defined(SDL) && defined(HAVE_TTF)
#include "SDL.h"
#include "SDL_ttf.h"
#include "../doomdef.h"
#include "../doomstat.h"
#include "../d_netfil.h"
#include "../filesrch.h"
#include "i_ttf.h"

// Search directories to find aforementioned TTF file.
#ifdef _PS3
#include <sysutil/video.h>
#define FONTSEARCHPATH1 "/dev_hdd0/game/SRB2-PS3_/USRDIR/etc"
#elif defined (__unix__) || defined(__APPLE__) || defined (UNIXCOMMON)
#define FONTSEARCHPATH1 "/usr/share/fonts"
#define FONTSEARCHPATH2 "/usr/local/share/fonts"
#define FONTSEARCHPATH3 "/usr/games/SRB2"
#define FONTSEARCHPATH4 "/usr/local/games/SRB2"
#define FONTSEARCHPATH5 "/usr/local/share/games/SRB2"
#else
#define FONTSEARCHPATH1 "."
#endif

#define FONTHANDLE -1

// Renduring surfaces.
SDL_Surface *TTFSurface = NULL;
SDL_Surface *TTFRendSurface = NULL;
// Text box.
SDL_Rect TTFRect;
// Temporary storage for the new TTFRect, used to check for
// line wrapping.
SDL_Rect TTFRectCheck;
// Text rendering resolution.
videoResolution res;
// Text storage buffer, the contents get printed to the SDL surface.
char textbuffer[8192];

// look for default ttf file in given directory
static char *searchFont(const char *fontsearchDir)
{
	static char tempsw[256] = "";
	filestatus_t fstemp;

	strcpy(tempsw, FONTFILE);
	fstemp = filesearch(tempsw, fontsearchDir, NULL, true, 20);
	if (fstemp == FS_FOUND)
	{
		return tempsw;
	}
	return NULL;
}

// Load TTF font from file.
INT32 I_TTFLoadFont(const char *file, UINT32 ptsize)
{
	TTF_Font *tmpfont = NULL;
	float fontsize;

	// If a font is currently loaded, unload it.
	if (currentfont)
	{
		TTF_CloseFont(currentfont);
	}

	// Scale the specified font point size for the current resolution.
	fontsize = (ptsize * 0.005f) * (res.width - res.height);

	tmpfont = TTF_OpenFont(file, fontsize);

	if (!tmpfont)
		return FONTHANDLE;

	// set pointer for current font
	currentfont = tmpfont;

	// set current font point size
	currentfontpoint = ptsize;

	// get font properties, and set them
	currentfontstyle = TTF_GetFontStyle(currentfont);
	TTF_SetFontStyle(currentfont, currentfontstyle);

	// these functions only exist in SDL_ttf 2.0.10 onwards
#if SDL_TTF_VERSION_ATLEAST(2,0,10)
	currentfontkerning = TTF_GetFontKerning(currentfont);
	TTF_SetFontKerning(currentfont, currentfontkerning);

	currentfonthinting = TTF_GetFontHinting(currentfont);
	TTF_SetFontHinting(currentfont, currentfonthinting);

	currentfontoutline = TTF_GetFontOutline(currentfont);
	TTF_SetFontOutline(currentfont, currentfontoutline);
#endif

	return 0;
}

static void I_TTFRendSurface(const char *textmsg, TTF_Font *font, TextQuality quality, SDL_Color fontfgcolor, SDL_Color fontbgcolor)
{
	// Print text in the buffer.
	// SDL_ttf has three modes to draw text.
	// Solid rendering is quick, but dirty. Use it if you need speed more than quality.
	switch (quality)
	{
		case solid:
			TTFRendSurface = TTF_RenderText_Solid(font, textmsg, fontfgcolor);
			break;
		// Shaded rendering adds a background to the rendered text. Because of this, I_TTFDrawText
		// takes an extra color more than the other styles to be a background color.
		// Shaded is supposedly as fast as solid rendering and about as good quality as blended.
		case shaded:
			TTFRendSurface = TTF_RenderText_Shaded(font, textmsg, fontfgcolor, fontbgcolor);
			break;
		// Blended rendering is the opposite of solid. Good quality, but slow.
		case blended:
			TTFRendSurface = TTF_RenderText_Blended(font, textmsg, fontfgcolor);
			break;
	}

	// Get SDL to update the main surface.
	SDL_BlitSurface(TTFRendSurface, NULL, TTFSurface, &TTFRect);
	SDL_Flip(TTFSurface);
}

// Draw text to screen. It will accept four colour vales (red, green, blue and alpha)
// with foreground for draw modes Solid and Blended, and an extra four values for background
// colour with draw type Shaded.
void I_TTFDrawText(TTF_Font *font, TextQuality quality, INT32 fgR, INT32 fgG, INT32 fgB, INT32 fgA, INT32 bgR, INT32 bgG, INT32 bgB, INT32 bgA, const char *textmsg)
{
	// Temporary small buffer to store character to process.
	// NULL pointer to prevc to kill warning
	char c, prevc = 0x0;
	// hack to allow TTF_SizeText to work properly.
	char linebuffer[2];
	// Don't need h, but TTF_SizeText needs a height parameter
	INT32 w, h;

	// Globally declare foreground and background text colours,
	// text drawing mode and the font to draw.
	SDL_Color fontfgcolor = {fgR, fgG, fgB, fgA};
	SDL_Color fontbgcolor = {bgR, bgG, bgB, bgA};

	// Keep on processing until the null terminator in the text buffer is reached.
	while (*textmsg != '\0')
	{
		// Copy pointer for current character into the temporary buffer.
		c = *textmsg;
		// If c is a newline, move to the next available line.
		if (c == '\n')
		{
			TTFRectCheck.x = 0;
			TTFRectCheck.y += (currentfontpoint + 1);
		}
		// Otherwise...
		else
		{
			// If the previous character was a newline, actually move to the next line.
			if (prevc == '\n')
			{
				if (textbuffer != NULL)
				{
					// Render cached text to the SDL surface.
					I_TTFRendSurface(textbuffer, font, quality, fontfgcolor, fontbgcolor);
					// Empty text buffer.
					memset(textbuffer, '\0', 1);
				}
				TTFRect.x = TTFRectCheck.x;
				TTFRect.y = TTFRectCheck.y;
			}
			// Copy the character to the text buffer.
			sprintf(textbuffer, "%s%c", textbuffer, c);
			// Hack to allow TTF_SizeText to work properly.
			sprintf(linebuffer, "%c", c);
			// If we have reached the end of the screen, move to the next available line.
			TTF_SizeText(currentfont, linebuffer, &w, &h);
			TTFRectCheck.x += w;
			if (TTFRectCheck.x >= res.width)
			{
				// Render cached text to the SDL surface.
				I_TTFRendSurface(textbuffer, font, quality, fontfgcolor, fontbgcolor);
				// Empty text buffer.
				memset(textbuffer, '\0', 1);
				// Move to the next line.
				TTFRectCheck.x = 0;
				TTFRectCheck.y += (currentfontpoint + 1);
				// Set stored co-ordinates for next line.
				TTFRect.x = TTFRectCheck.x;
				TTFRect.y = TTFRectCheck.y;
			}
		}
		// Add 1 to the pointer reference for the character to process.
		textmsg++;
		// Copy contents of the now-old buffer to somewhere else, so it can be referenced in next loop.
		prevc = c;
	}

	// If the buffer was previously emptied by a line wrapping operation and
	// no text came after that, don't print anything. Otherwise, print everything
	// still in the buffer.
	if (textbuffer != NULL)
	{
		// Render cached text to the SDL surface.
		I_TTFRendSurface(textbuffer, font, quality, fontfgcolor, fontbgcolor);
		// Empty text buffer.
		memset(textbuffer, '\0', 1);
		// Set stored co-ordinates for next line.
		TTFRect.x = TTFRectCheck.x;
		TTFRect.y = TTFRectCheck.y;
	}
}

// Initialise SDL_ttf.
void I_StartupTTF(UINT32 fontpointsize, Uint32 initflags, Uint32 vidmodeflags)
{
	char *fontpath = NULL;
	INT32 fontstatus = -1;
#ifdef _PS3
	videoState state;
	videoGetState(0, 0, &state);
	videoGetResolution(state.displayMode.resolution, &res);
	bitsperpixel = 24;
#else
	res.width = 320;
	res.height = 200;
	bitsperpixel = 8;
#endif

	// what's the point of trying to display an error?
	// SDL_ttf is not started, can't display anything to screen (presumably)...
	if (SDL_InitSubSystem(initflags) < 0)
		I_Error("Couldn't initialize SDL: %s\n", SDL_GetError());

	TTFSurface = SDL_SetVideoMode(res.width, res.height, bitsperpixel, vidmodeflags);
	if (!TTFSurface)
		I_Error("Couldn't set SDL Video resolution: %s\n", SDL_GetError());

	if (TTF_Init() < 0)
		I_Error("Couldn't start SDL_ttf: %s\n", TTF_GetError());

	// look for default font in many directories
#ifdef FONTSEARCHPATH1
	fontpath = searchFont(FONTSEARCHPATH1);
	if (fontpath) fontstatus = I_TTFLoadFont(fontpath, fontpointsize);
#endif
#ifdef FONTSEARCHPATH2
	if (fontstatus < 0)
	{
		fontpath = searchFont(FONTSEARCHPATH2);
		if (fontpath) fontstatus = I_TTFLoadFont(fontpath, fontpointsize);
	}
#endif
#ifdef FONTSEARCHPATH3
	if (fontstatus < 0)
	{
		fontpath = searchFont(FONTSEARCHPATH3);
		if (fontpath) fontstatus = I_TTFLoadFont(fontpath, fontpointsize);
	}
#endif
#ifdef FONTSEARCHPATH4
	if (fontstatus < 0)
	{
		fontpath = searchFont(FONTSEARCHPATH4);
		if (fontpath) fontstatus = I_TTFLoadFont(fontpath, fontpointsize);
	}
#endif
#ifdef FONTSEARCHPATH5
	if (fontstatus < 0)
	{
		fontpath = searchFont(FONTSEARCHPATH5);
		if (fontpath) fontstatus = I_TTFLoadFont(fontpath, fontpointsize);
	}
#endif
#ifdef FONTSEARCHPATH6
	if (fontstatus < 0)
	{
		fontpath = searchFont(FONTSEARCHPATH6);
		if (fontpath) fontstatus = I_TTFLoadFont(fontpath, fontpointsize);
	}
#endif
#ifdef FONTSEARCHPATH7
	if (fontstatus < 0)
	{
		fontpath = searchFont(FONTSEARCHPATH7);
		if (fontpath) fontstatus = I_TTFLoadFont(fontpath, fontpointsize);
	}
#endif
	// argh! no font file found! disable SDL_ttf code
	if (fontstatus < 0)
	{
		I_ShutdownTTF();
		CONS_Printf("Unable to find default font files! Not loading SDL_ttf\n");
	}
	else
	{
		// Get SDL_ttf compiled and linked version
		SDL_version TTFcompiled;
		const SDL_version *TTFlinked;

		SDL_TTF_VERSION(&TTFcompiled);
		TTFlinked = TTF_Linked_Version();

		// Display it on screen
		CONS_Printf("Compiled for SDL_ttf version: %d.%d.%d\n",
			    TTFcompiled.major, TTFcompiled.minor, TTFcompiled.patch);
		CONS_Printf("Linked with SDL_ttf version: %d.%d.%d\n",
			    TTFlinked->major, TTFlinked->minor, TTFlinked->patch);
	}
}

void I_ShutdownTTF(void)
{
	// close current font
	TTF_CloseFont(currentfont);
	// shutdown SDL_ttf
	TTF_Quit();

	// Free TTF rendering surfaces.
	SDL_FreeSurface(TTFSurface);
	SDL_FreeSurface(TTFRendSurface);
}
#endif

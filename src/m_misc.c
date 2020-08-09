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
/// \file  m_misc.h
/// \brief Commonly used routines
///        Default config file, PCX screenshots, file i/o

#ifdef __GNUC__

#if (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3)
// Ignore "argument might be clobbered by longjmp" warning in GCC
// (if libpng is compiled with setjmp error handling)
#pragma GCC diagnostic ignored "-Wclobbered"
#endif

#include <unistd.h>
#endif

#include <errno.h>

// Extended map support.
#include <ctype.h>

#include "doomdef.h"
#include "g_game.h"
#include "m_misc.h"
#include "hu_stuff.h"
#include "st_stuff.h"
#include "v_video.h"
#include "z_zone.h"
#include "g_input.h"
#include "i_video.h"
#include "d_main.h"
#include "m_argv.h"
#include "i_system.h"
#include "command.h" // cv_execversion

#include "m_anigif.h"

// So that the screenshot menu auto-updates...
#include "m_menu.h"

#ifdef HWRENDER
#include "hardware/hw_main.h"
#endif

#ifdef HAVE_SDL
#include "sdl/hwsym_sdl.h"
#ifdef __linux__
#ifndef _LARGEFILE64_SOURCE
typedef off_t off64_t;
#endif
#endif
#endif

#if defined(__MINGW32__) && ((__GNUC__ > 7) || (__GNUC__ == 6 && __GNUC_MINOR__ >= 3)) && (__GNUC__ < 8)
#define PRIdS "u"
#elif defined (_WIN32)
#define PRIdS "Iu"
#elif defined (DJGPP)
#define PRIdS "u"
#else
#define PRIdS "zu"
#endif

#ifdef HAVE_PNG

#ifndef _MSC_VER
#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#endif
#endif

#ifndef _LFS64_LARGEFILE
#define _LFS64_LARGEFILE
#endif

#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 0
#endif

 #include "zlib.h"
 #include "png.h"
 #if (PNG_LIBPNG_VER_MAJOR > 1) || (PNG_LIBPNG_VER_MAJOR == 1 && PNG_LIBPNG_VER_MINOR >= 4)
  #define NO_PNG_DEBUG // 1.4.0 move png_debug to pngpriv.h
 #endif
 #ifdef PNG_WRITE_SUPPORTED
  #define USE_PNG // Only actually use PNG if write is supported.
  #if defined (PNG_WRITE_APNG_SUPPORTED) //|| !defined(PNG_STATIC)
    #include "apng.h"
    #define USE_APNG
  #endif
  // See hardware/hw_draw.c for a similar check to this one.
 #endif
#endif

static CV_PossibleValue_t screenshot_cons_t[] = {{0, "Default"}, {1, "HOME"}, {2, "SRB2"}, {3, "CUSTOM"}, {0, NULL}};
consvar_t cv_screenshot_option = {"screenshot_option", "Default", CV_SAVE|CV_CALL, screenshot_cons_t, Screenshot_option_Onchange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_screenshot_folder = {"screenshot_folder", "", CV_SAVE, NULL, NULL, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_screenshot_colorprofile = {"screenshot_colorprofile", "Yes", CV_SAVE, CV_YesNo, NULL, 0, NULL, NULL, 0, 0, NULL};

static CV_PossibleValue_t moviemode_cons_t[] = {{MM_GIF, "GIF"}, {MM_APNG, "aPNG"}, {MM_SCREENSHOT, "Screenshots"}, {0, NULL}};
consvar_t cv_moviemode = {"moviemode_mode", "GIF", CV_SAVE|CV_CALL, moviemode_cons_t, Moviemode_mode_Onchange, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_movie_option = {"movie_option", "Default", CV_SAVE|CV_CALL, screenshot_cons_t, Moviemode_option_Onchange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_movie_folder = {"movie_folder", "", CV_SAVE, NULL, NULL, 0, NULL, NULL, 0, 0, NULL};

static CV_PossibleValue_t zlib_mem_level_t[] = {
	{1, "(Min Memory) 1"},
	{2, "2"}, {3, "3"}, {4, "4"}, {5, "5"}, {6, "6"}, {7, "7"},
	{8, "(Optimal) 8"}, //libpng Default
	{9, "(Max Memory) 9"}, {0, NULL}};

static CV_PossibleValue_t zlib_level_t[] = {
	{0, "No Compression"},  //Z_NO_COMPRESSION
	{1, "(Fastest) 1"}, //Z_BEST_SPEED
	{2, "2"}, {3, "3"}, {4, "4"}, {5, "5"},
	{6, "(Optimal) 6"}, //Zlib Default
	{7, "7"}, {8, "8"},
	{9, "(Maximum) 9"}, //Z_BEST_COMPRESSION
	{0, NULL}};

static CV_PossibleValue_t zlib_strategy_t[] = {
	{0, "Normal"}, //Z_DEFAULT_STRATEGY
	{1, "Filtered"}, //Z_FILTERED
	{2, "Huffman Only"}, //Z_HUFFMAN_ONLY
	{3, "RLE"}, //Z_RLE
	{4, "Fixed"}, //Z_FIXED
	{0, NULL}};

static CV_PossibleValue_t zlib_window_bits_t[] = {
#ifdef WBITS_8_OK
	{8, "256"},
#endif
	{9, "512"}, {10, "1k"}, {11, "2k"}, {12, "4k"}, {13, "8k"},
	{14, "16k"}, {15, "32k"},
	{0, NULL}};

static CV_PossibleValue_t apng_delay_t[] = {
	{1, "1x"},
	{2, "1/2x"},
	{3, "1/3x"},
	{4, "1/4x"},
	{0, NULL}};

// zlib memory usage is as follows:
// (1 << (zlib_window_bits+2)) +  (1 << (zlib_level+9))
consvar_t cv_zlib_memory = {"png_memory_level", "7", CV_SAVE, zlib_mem_level_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_zlib_level = {"png_compress_level", "(Optimal) 6", CV_SAVE, zlib_level_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_zlib_strategy = {"png_strategy", "Normal", CV_SAVE, zlib_strategy_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_zlib_window_bits = {"png_window_size", "32k", CV_SAVE, zlib_window_bits_t, NULL, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_zlib_memorya = {"apng_memory_level", "(Max Memory) 9", CV_SAVE, zlib_mem_level_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_zlib_levela = {"apng_compress_level", "4", CV_SAVE, zlib_level_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_zlib_strategya = {"apng_strategy", "RLE", CV_SAVE, zlib_strategy_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_zlib_window_bitsa = {"apng_window_size", "32k", CV_SAVE, zlib_window_bits_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_apng_delay = {"apng_speed", "1x", CV_SAVE, apng_delay_t, NULL, 0, NULL, NULL, 0, 0, NULL};

boolean takescreenshot = false; // Take a screenshot this tic

moviemode_t moviemode = MM_OFF;

/** Returns the map number for a map identified by the last two characters in
  * its name.
  *
  * \param first  The first character after MAP.
  * \param second The second character after MAP.
  * \return The map number, or 0 if no map corresponds to these characters.
  * \sa G_BuildMapName
  */
INT32 M_MapNumber(char first, char second)
{
	if (isdigit(first))
	{
		if (isdigit(second))
			return ((INT32)first - '0') * 10 + ((INT32)second - '0');
		return 0;
	}

	if (!isalpha(first))
		return 0;
	if (!isalnum(second))
		return 0;

	return 100 + ((INT32)tolower(first) - 'a') * 36 + (isdigit(second) ? ((INT32)second - '0') :
		((INT32)tolower(second) - 'a') + 10);
}

// ==========================================================================
//                         FILE INPUT / OUTPUT
// ==========================================================================

// some libcs has no access function, make our own
#if 0
int access(const char *path, int amode)
{
	int accesshandle = -1;
	FILE *handle = NULL;
	if (amode == 6) // W_OK|R_OK
		handle = fopen(path, "r+");
	else if (amode == 4) // R_OK
		handle = fopen(path, "r");
	else if (amode == 2) // W_OK
		handle = fopen(path, "a+");
	else if (amode == 0) //F_OK
		handle = fopen(path, "rb");
	if (handle)
	{
		accesshandle = 0;
		fclose(handle);
	}
	return accesshandle;
}
#endif


//
// FIL_WriteFile
//
#ifndef O_BINARY
#define O_BINARY 0
#endif

/** Writes out a file.
  *
  * \param name   Name of the file to write.
  * \param source Memory location to write from.
  * \param length How many bytes to write.
  * \return True on success, false on failure.
  */
boolean FIL_WriteFile(char const *name, const void *source, size_t length)
{
	FILE *handle = NULL;
	size_t count;

	//if (FIL_WriteFileOK(name))
		handle = fopen(name, "w+b");

	if (!handle)
		return false;

	count = fwrite(source, 1, length, handle);
	fclose(handle);

	if (count < length)
		return false;

	return true;
}

/** Reads in a file, appending a zero byte at the end.
  *
  * \param name   Filename to read.
  * \param buffer Pointer to a pointer, which will be set to the location of a
  *               newly allocated buffer holding the file's contents.
  * \return Number of bytes read, not counting the zero byte added to the end,
  *         or 0 on error.
  */
size_t FIL_ReadFileTag(char const *name, UINT8 **buffer, INT32 tag)
{
	FILE *handle = NULL;
	size_t count, length;
	UINT8 *buf;

	if (FIL_ReadFileOK(name))
		handle = fopen(name, "rb");

	if (!handle)
		return 0;

	fseek(handle,0,SEEK_END);
	length = ftell(handle);
	fseek(handle,0,SEEK_SET);

	buf = Z_Malloc(length + 1, tag, NULL);
	count = fread(buf, 1, length, handle);
	fclose(handle);

	if (count < length)
	{
		Z_Free(buf);
		return 0;
	}

	// append 0 byte for script text files
	buf[length] = 0;

	*buffer = buf;
	return length;
}

/** Makes a copy of a text file with all newlines converted into LF newlines.
  *
  * \param textfilename The name of the source file
  * \param binfilename The name of the destination file
  */
boolean FIL_ConvertTextFileToBinary(const char *textfilename, const char *binfilename)
{
	FILE *textfile;
	FILE *binfile;
	UINT8 buffer[1024];
	size_t count;
	boolean success;

	textfile = fopen(textfilename, "r");
	if (!textfile)
		return false;

	binfile = fopen(binfilename, "wb");
	if (!binfile)
	{
		fclose(textfile);
		return false;
	}

	do
	{
		count = fread(buffer, 1, sizeof(buffer), textfile);
		fwrite(buffer, 1, count, binfile);
	} while (count);

	success = !(ferror(textfile) || ferror(binfile));

	fclose(textfile);
	fclose(binfile);

	return success;
}

/** Check if the filename exists
  *
  * \param name   Filename to check.
  * \return true if file exists, false if it doesn't.
  */
boolean FIL_FileExists(char const *name)
{
	return access(name,0)+1; //F_OK
}


/** Check if the filename OK to write
  *
  * \param name   Filename to check.
  * \return true if file write-able, false if it doesn't.
  */
boolean FIL_WriteFileOK(char const *name)
{
	return access(name,2)+1; //W_OK
}


/** Check if the filename OK to read
  *
  * \param name   Filename to check.
  * \return true if file read-able, false if it doesn't.
  */
boolean FIL_ReadFileOK(char const *name)
{
	return access(name,4)+1; //R_OK
}

/** Check if the filename OK to read/write
  *
  * \param name   Filename to check.
  * \return true if file (read/write)-able, false if it doesn't.
  */
boolean FIL_FileOK(char const *name)
{
	return access(name,6)+1; //R_OK|W_OK
}


/** Checks if a pathname has a file extension and adds the extension provided
  * if not.
  *
  * \param path      Pathname to check.
  * \param extension Extension to add if no extension is there.
  */
void FIL_DefaultExtension(char *path, const char *extension)
{
	char *src;

	// search for '.' from end to begin, add .EXT only when not found
	src = path + strlen(path) - 1;

	while (*src != '/' && src != path)
	{
		if (*src == '.')
			return; // it has an extension
		src--;
	}

	strcat(path, extension);
}

void FIL_ForceExtension(char *path, const char *extension)
{
	char *src;

	// search for '.' from end to begin, add .EXT only when not found
	src = path + strlen(path) - 1;

	while (*src != '/' && src != path)
	{
		if (*src == '.')
		{
			*src = '\0';
			break; // it has an extension
		}
		src--;
	}

	strcat(path, extension);
}

/** Checks if a filename extension is found.
  * Lump names do not contain dots.
  *
  * \param in String to check.
  * \return True if an extension is found, otherwise false.
  */
boolean FIL_CheckExtension(const char *in)
{
	while (*in++)
		if (*in == '.')
			return true;

	return false;
}

// ==========================================================================
//                        CONFIGURATION FILE
// ==========================================================================

//
// DEFAULTS
//

char configfile[MAX_WADPATH];

// ==========================================================================
//                          CONFIGURATION
// ==========================================================================
static boolean gameconfig_loaded = false; // true once config.cfg loaded AND executed

/** Saves a player's config, possibly to a particular file.
  *
  * \sa Command_LoadConfig_f
  */
void Command_SaveConfig_f(void)
{
	char tmpstr[MAX_WADPATH];

	if (COM_Argc() < 2)
	{
		CONS_Printf(M_GetText("saveconfig <filename[.cfg]> [-silent] : save config to a file\n"));
		return;
	}
	strcpy(tmpstr, COM_Argv(1));
	FIL_ForceExtension(tmpstr, ".cfg");

	M_SaveConfig(tmpstr);
	if (stricmp(COM_Argv(2), "-silent"))
		CONS_Printf(M_GetText("config saved as %s\n"), configfile);
}

/** Loads a game config, possibly from a particular file.
  *
  * \sa Command_SaveConfig_f, Command_ChangeConfig_f
  */
void Command_LoadConfig_f(void)
{
	if (COM_Argc() != 2)
	{
		CONS_Printf(M_GetText("loadconfig <filename[.cfg]> : load config from a file\n"));
		return;
	}

	strcpy(configfile, COM_Argv(1));
	FIL_ForceExtension(configfile, ".cfg");

	// load default control
	G_ClearAllControlKeys();
	G_CopyControls(gamecontrol, gamecontroldefault[gcs_fps], NULL, 0);
	G_CopyControls(gamecontrolbis, gamecontrolbisdefault[gcs_fps], NULL, 0);

	// temporarily reset execversion to default
	CV_ToggleExecVersion(true);
	COM_BufInsertText(va("%s \"%s\"\n", cv_execversion.name, cv_execversion.defaultvalue));
	CV_InitFilterVar();

	// exec the config
	COM_BufInsertText(va("exec \"%s\"\n", configfile));

	// don't filter anymore vars and don't let this convsvar be changed
	COM_BufInsertText(va("%s \"%d\"\n", cv_execversion.name, EXECVERSION));
	CV_ToggleExecVersion(false);
}

/** Saves the current configuration and loads another.
  *
  * \sa Command_LoadConfig_f, Command_SaveConfig_f
  */
void Command_ChangeConfig_f(void)
{
	if (COM_Argc() != 2)
	{
		CONS_Printf(M_GetText("changeconfig <filename[.cfg]> : save current config and load another\n"));
		return;
	}

	COM_BufAddText(va("saveconfig \"%s\"\n", configfile));
	COM_BufAddText(va("loadconfig \"%s\"\n", COM_Argv(1)));
}

/** Loads the default config file.
  *
  * \sa Command_LoadConfig_f
  */
void M_FirstLoadConfig(void)
{
	// configfile is initialised by d_main when searching for the wad?

	// check for a custom config file
	if (M_CheckParm("-config") && M_IsNextParm())
	{
		strcpy(configfile, M_GetNextParm());
		CONS_Printf(M_GetText("config file: %s\n"), configfile);
	}

	// load default control
	G_DefineDefaultControls();
	G_CopyControls(gamecontrol, gamecontroldefault[gcs_fps], NULL, 0);
	G_CopyControls(gamecontrolbis, gamecontrolbisdefault[gcs_fps], NULL, 0);

	// register execversion here before we load any configs
	CV_RegisterVar(&cv_execversion);

	// temporarily reset execversion to default
	// we shouldn't need to do this, but JUST in case...
	CV_ToggleExecVersion(true);
	COM_BufInsertText(va("%s \"%s\"\n", cv_execversion.name, cv_execversion.defaultvalue));
	CV_InitFilterVar();

	// load config, make sure those commands doesnt require the screen...
	COM_BufInsertText(va("exec \"%s\"\n", configfile));
	// no COM_BufExecute() needed; that does it right away

	// don't filter anymore vars and don't let this convsvar be changed
	COM_BufInsertText(va("%s \"%d\"\n", cv_execversion.name, EXECVERSION));
	CV_ToggleExecVersion(false);

	// make sure I_Quit() will write back the correct config
	// (do not write back the config if it crash before)
	gameconfig_loaded = true;

	// reset to default player stuff
	COM_BufAddText (va("%s \"%s\"\n",cv_skin.name,cv_defaultskin.string));
	COM_BufAddText (va("%s \"%s\"\n",cv_playercolor.name,cv_defaultplayercolor.string));
	COM_BufAddText (va("%s \"%s\"\n",cv_skin2.name,cv_defaultskin2.string));
	COM_BufAddText (va("%s \"%s\"\n",cv_playercolor2.name,cv_defaultplayercolor2.string));
}

/** Saves the game configuration.
  *
  * \sa Command_SaveConfig_f
  */
void M_SaveConfig(const char *filename)
{
	FILE *f;
	char *filepath;

	// make sure not to write back the config until it's been correctly loaded
	if (!gameconfig_loaded)
		return;

	// can change the file name
	if (filename)
	{
		if (!strstr(filename, ".cfg"))
		{
			CONS_Alert(CONS_NOTICE, M_GetText("Config filename must be .cfg\n"));
			return;
		}

		// append srb2home to beginning of filename
		// but check if srb2home isn't already there, first
		if (!strstr(filename, srb2home))
			filepath = va(pandf,srb2home, filename);
		else
			filepath = Z_StrDup(filename);

		f = fopen(filepath, "w");
		// change it only if valid
		if (f)
			strcpy(configfile, filepath);
		else
		{
			CONS_Alert(CONS_ERROR, M_GetText("Couldn't save game config file %s\n"), filepath);
			return;
		}
	}
	else
	{
		if (!strstr(configfile, ".cfg"))
		{
			CONS_Alert(CONS_NOTICE, M_GetText("Config filename must be .cfg\n"));
			return;
		}

		f = fopen(configfile, "w");
		if (!f)
		{
			CONS_Alert(CONS_ERROR, M_GetText("Couldn't save game config file %s\n"), configfile);
			return;
		}
	}

	// header message
	fprintf(f, "// SRB2 configuration file.\n");

	// print execversion FIRST, because subsequent consvars need to be filtered
	// always print current EXECVERSION
	fprintf(f, "%s \"%d\"\n", cv_execversion.name, EXECVERSION);

	// FIXME: save key aliases if ever implemented..

	if (tutorialmode && tutorialgcs)
	{
		CV_SetValue(&cv_usemouse, tutorialusemouse);
		CV_SetValue(&cv_alwaysfreelook, tutorialfreelook);
		CV_SetValue(&cv_mousemove, tutorialmousemove);
		CV_SetValue(&cv_analog[0], tutorialanalog);
		CV_SaveVariables(f);
		CV_Set(&cv_usemouse, cv_usemouse.defaultvalue);
		CV_Set(&cv_alwaysfreelook, cv_alwaysfreelook.defaultvalue);
		CV_Set(&cv_mousemove, cv_mousemove.defaultvalue);
		CV_Set(&cv_analog[0], cv_analog[0].defaultvalue);
	}
	else
		CV_SaveVariables(f);

	if (!dedicated)
	{
		if (tutorialmode && tutorialgcs)
			G_SaveKeySetting(f, gamecontroldefault[gcs_custom], gamecontrolbis); // using gcs_custom as temp storage
		else
			G_SaveKeySetting(f, gamecontrol, gamecontrolbis);
	}

	fclose(f);
}

// ==========================================================================
//                              SCREENSHOTS
// ==========================================================================
static UINT8 screenshot_palette[768];
static void M_CreateScreenShotPalette(void)
{
	size_t i, j;
	for (i = 0, j = 0; i < 768; i += 3, j++)
	{
		RGBA_t locpal = ((cv_screenshot_colorprofile.value)
		? pLocalPalette[(max(st_palette,0)*256)+j]
		: pMasterPalette[(max(st_palette,0)*256)+j]);
		screenshot_palette[i] = locpal.s.red;
		screenshot_palette[i+1] = locpal.s.green;
		screenshot_palette[i+2] = locpal.s.blue;
	}
}

#if NUMSCREENS > 2
static const char *Newsnapshotfile(const char *pathname, const char *ext)
{
	static char freename[13] = "srb2XXXX.ext";
	int i = 5000; // start in the middle: num screenshots divided by 2
	int add = i; // how much to add or subtract if wrong; gets divided by 2 each time
	int result; // -1 = guess too high, 0 = correct, 1 = guess too low

	// find a file name to save it to
	strcpy(freename+9,ext);

	for (;;)
	{
		freename[4] = (char)('0' + (char)(i/1000));
		freename[5] = (char)('0' + (char)((i/100)%10));
		freename[6] = (char)('0' + (char)((i/10)%10));
		freename[7] = (char)('0' + (char)(i%10));

		if (FIL_WriteFileOK(va(pandf,pathname,freename))) // access succeeds
			result = 1; // too low
		else // access fails: equal or too high
		{
			if (!i)
				break; // not too high, so it must be equal! YAY!

			freename[4] = (char)('0' + (char)((i-1)/1000));
			freename[5] = (char)('0' + (char)(((i-1)/100)%10));
			freename[6] = (char)('0' + (char)(((i-1)/10)%10));
			freename[7] = (char)('0' + (char)((i-1)%10));
			if (!FIL_WriteFileOK(va(pandf,pathname,freename))) // access fails
				result = -1; // too high
			else
				break; // not too high, so equal, YAY!
		}

		add /= 2;

		if (!add) // don't get stuck at 5 due to truncation!
			add = 1;

		i += add * result;

		if (i < 0 || i > 9999)
			return NULL;
	}

	freename[4] = (char)('0' + (char)(i/1000));
	freename[5] = (char)('0' + (char)((i/100)%10));
	freename[6] = (char)('0' + (char)((i/10)%10));
	freename[7] = (char)('0' + (char)(i%10));

	return freename;
}
#endif

#ifdef HAVE_PNG
FUNCNORETURN static void PNG_error(png_structp PNG, png_const_charp pngtext)
{
	//CONS_Debug(DBG_RENDER, "libpng error at %p: %s", PNG, pngtext);
	I_Error("libpng error at %p: %s", PNG, pngtext);
}

static void PNG_warn(png_structp PNG, png_const_charp pngtext)
{
	CONS_Debug(DBG_RENDER, "libpng warning at %p: %s", PNG, pngtext);
}

static void M_PNGhdr(png_structp png_ptr, png_infop png_info_ptr, PNG_CONST png_uint_32 width, PNG_CONST png_uint_32 height, PNG_CONST png_byte *palette)
{
	const png_byte png_interlace = PNG_INTERLACE_NONE; //PNG_INTERLACE_ADAM7
	if (palette)
	{
		png_colorp png_PLTE = png_malloc(png_ptr, sizeof(png_color)*256); //palette
		const png_byte *pal = palette;
		png_uint_16 i;
		for (i = 0; i < 256; i++)
		{
			png_PLTE[i].red   = *pal; pal++;
			png_PLTE[i].green = *pal; pal++;
			png_PLTE[i].blue  = *pal; pal++;
		}
		png_set_IHDR(png_ptr, png_info_ptr, width, height, 8, PNG_COLOR_TYPE_PALETTE,
		 png_interlace, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
		png_write_info_before_PLTE(png_ptr, png_info_ptr);
		png_set_PLTE(png_ptr, png_info_ptr, png_PLTE, 256);
		png_free(png_ptr, (png_voidp)png_PLTE); // safe in libpng-1.2.1+
		png_set_filter(png_ptr, PNG_FILTER_TYPE_BASE, PNG_FILTER_NONE);
		png_set_compression_strategy(png_ptr, Z_DEFAULT_STRATEGY);
	}
	else
	{
		png_set_IHDR(png_ptr, png_info_ptr, width, height, 8, PNG_COLOR_TYPE_RGB,
		 png_interlace, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
		png_write_info_before_PLTE(png_ptr, png_info_ptr);
		png_set_compression_strategy(png_ptr, Z_FILTERED);
	}
}

static void M_PNGText(png_structp png_ptr, png_infop png_info_ptr, PNG_CONST png_byte movie)
{
#ifdef PNG_TEXT_SUPPORTED
#define SRB2PNGTXT 11 //PNG_KEYWORD_MAX_LENGTH(79) is the max
	png_text png_infotext[SRB2PNGTXT];
	char keytxt[SRB2PNGTXT][12] = {
	"Title", "Description", "Playername", "Mapnum", "Mapname",
	"Location", "Interface", "Render Mode", "Revision", "Build Date", "Build Time"};
	char titletxt[] = "Sonic Robo Blast 2 " VERSIONSTRING;
	png_charp playertxt =  cv_playername.zstring;
	char desctxt[] = "SRB2 Screenshot";
	char Movietxt[] = "SRB2 Movie";
	size_t i;
	char interfacetxt[] =
#ifdef HAVE_SDL
	 "SDL";
#elif defined (_WINDOWS)
	 "DirectX";
#else
	 "Unknown";
#endif
	char rendermodetxt[9];
	char maptext[8];
	char lvlttltext[48];
	char locationtxt[40];
	char ctrevision[40];
	char ctdate[40];
	char cttime[40];

	switch (rendermode)
	{
		case render_soft:
			strcpy(rendermodetxt, "Software");
			break;
		case render_opengl:
			strcpy(rendermodetxt, "OpenGL");
			break;
		default: // Just in case
			strcpy(rendermodetxt, "None");
			break;
	}

	if (gamestate == GS_LEVEL)
		snprintf(maptext, 8, "%s", G_BuildMapName(gamemap));
	else
		snprintf(maptext, 8, "Unknown");

	if (gamestate == GS_LEVEL && mapheaderinfo[gamemap-1]->lvlttl[0] != '\0')
		snprintf(lvlttltext, 48, "%s%s%s",
			mapheaderinfo[gamemap-1]->lvlttl,
			(mapheaderinfo[gamemap-1]->levelflags & LF_NOZONE) ? "" : " Zone",
			(mapheaderinfo[gamemap-1]->actnum > 0) ? va(" %d",mapheaderinfo[gamemap-1]->actnum) : "");
	else
		snprintf(lvlttltext, 48, "Unknown");

	if (gamestate == GS_LEVEL && &players[displayplayer] && players[displayplayer].mo)
		snprintf(locationtxt, 40, "X:%d Y:%d Z:%d A:%d",
			players[displayplayer].mo->x>>FRACBITS,
			players[displayplayer].mo->y>>FRACBITS,
			players[displayplayer].mo->z>>FRACBITS,
			FixedInt(AngleFixed(players[displayplayer].mo->angle)));
	else
		snprintf(locationtxt, 40, "Unknown");

	memset(png_infotext,0x00,sizeof (png_infotext));

	for (i = 0; i < SRB2PNGTXT; i++)
		png_infotext[i].key  = keytxt[i];

	png_infotext[0].text = titletxt;
	if (movie)
		png_infotext[1].text = Movietxt;
	else
		png_infotext[1].text = desctxt;
	png_infotext[2].text = playertxt;
	png_infotext[3].text = maptext;
	png_infotext[4].text = lvlttltext;
	png_infotext[5].text = locationtxt;
	png_infotext[6].text = interfacetxt;
	png_infotext[7].text = rendermodetxt;
	png_infotext[8].text = strncpy(ctrevision, comprevision, sizeof(ctrevision)-1);
	png_infotext[9].text = strncpy(ctdate, compdate, sizeof(ctdate)-1);
	png_infotext[10].text = strncpy(cttime, comptime, sizeof(cttime)-1);

	png_set_text(png_ptr, png_info_ptr, png_infotext, SRB2PNGTXT);
#undef SRB2PNGTXT
#endif
}

static inline void M_PNGImage(png_structp png_ptr, png_infop png_info_ptr, PNG_CONST png_uint_32 height, png_bytep png_buf)
{
	png_uint_32 pitch = png_get_rowbytes(png_ptr, png_info_ptr);
	png_bytepp row_pointers = png_malloc(png_ptr, height* sizeof (png_bytep));
	png_uint_32 y;
	for (y = 0; y < height; y++)
	{
		row_pointers[y] = png_buf;
		png_buf += pitch;
	}
	png_write_image(png_ptr, row_pointers);
	png_free(png_ptr, (png_voidp)row_pointers);
}

#ifdef USE_APNG
static png_structp apng_ptr = NULL;
static png_infop   apng_info_ptr = NULL;
static apng_infop  apng_ainfo_ptr = NULL;
static png_FILE_p  apng_FILE = NULL;
static png_uint_32 apng_frames = 0;
#ifdef PNG_STATIC // Win32 build have static libpng
#define aPNG_set_acTL png_set_acTL
#define aPNG_write_frame_head png_write_frame_head
#define aPNG_write_frame_tail png_write_frame_tail
#else // outside libpng may not have apng support

#ifndef PNG_WRITE_APNG_SUPPORTED // libpng header may not have apng patch

#ifndef PNG_INFO_acTL
#define PNG_INFO_acTL 0x10000L
#endif
#ifndef PNG_INFO_fcTL
#define PNG_INFO_fcTL 0x20000L
#endif
#ifndef PNG_FIRST_FRAME_HIDDEN
#define PNG_FIRST_FRAME_HIDDEN       0x0001
#endif
#ifndef PNG_DISPOSE_OP_NONE
#define PNG_DISPOSE_OP_NONE        0x00
#endif
#ifndef PNG_DISPOSE_OP_BACKGROUND
#define PNG_DISPOSE_OP_BACKGROUND  0x01
#endif
#ifndef PNG_DISPOSE_OP_PREVIOUS
#define PNG_DISPOSE_OP_PREVIOUS    0x02
#endif
#ifndef PNG_BLEND_OP_SOURCE
#define PNG_BLEND_OP_SOURCE        0x00
#endif
#ifndef PNG_BLEND_OP_OVER
#define PNG_BLEND_OP_OVER          0x01
#endif
#ifndef PNG_HAVE_acTL
#define PNG_HAVE_acTL             0x4000
#endif
#ifndef PNG_HAVE_fcTL
#define PNG_HAVE_fcTL             0x8000L
#endif

#endif
typedef png_uint_32 (*P_png_set_acTL) (png_structp png_ptr,
   png_infop info_ptr, png_uint_32 num_frames, png_uint_32 num_plays);
typedef void (*P_png_write_frame_head) (png_structp png_ptr,
   png_infop info_ptr, png_bytepp row_pointers,
   png_uint_32 width, png_uint_32 height,
   png_uint_32 x_offset, png_uint_32 y_offset,
   png_uint_16 delay_num, png_uint_16 delay_den, png_byte dispose_op,
   png_byte blend_op);

typedef void (*P_png_write_frame_tail) (png_structp png_ptr,
   png_infop info_ptr);
static P_png_set_acTL aPNG_set_acTL = NULL;
static P_png_write_frame_head aPNG_write_frame_head = NULL;
static P_png_write_frame_tail aPNG_write_frame_tail = NULL;
#endif

static inline boolean M_PNGLib(void)
{
#ifdef PNG_STATIC // Win32 build have static libpng
	return true;
#else
	static void *pnglib = NULL;
	if (aPNG_set_acTL && aPNG_write_frame_head && aPNG_write_frame_tail)
		return true;
	if (pnglib)
		return false;
#ifdef _WIN32
	pnglib = GetModuleHandleA("libpng.dll");
	if (!pnglib)
		pnglib = GetModuleHandleA("libpng12.dll");
	if (!pnglib)
		pnglib = GetModuleHandleA("libpng13.dll");
#elif defined (HAVE_SDL)
#ifdef __APPLE__
	pnglib = hwOpen("libpng.dylib");
#else
	pnglib = hwOpen("libpng.so");
#endif
#endif
	if (!pnglib)
		return false;
#ifdef HAVE_SDL
	aPNG_set_acTL = hwSym("png_set_acTL", pnglib);
	aPNG_write_frame_head = hwSym("png_write_frame_head", pnglib);
	aPNG_write_frame_tail = hwSym("png_write_frame_tail", pnglib);
#endif
#ifdef _WIN32
	aPNG_set_acTL = GetProcAddress("png_set_acTL", pnglib);
	aPNG_write_frame_head = GetProcAddress("png_write_frame_head", pnglib);
	aPNG_write_frame_tail = GetProcAddress("png_write_frame_tail", pnglib);
#endif
	return (aPNG_set_acTL && aPNG_write_frame_head && aPNG_write_frame_tail);
#endif
}

static void M_PNGFrame(png_structp png_ptr, png_infop png_info_ptr, png_bytep png_buf)
{
	png_uint_32 pitch = png_get_rowbytes(png_ptr, png_info_ptr);
	PNG_CONST png_uint_32 height = vid.height;
	png_bytepp row_pointers = png_malloc(png_ptr, height* sizeof (png_bytep));
	png_uint_32 y;
	png_uint_16 framedelay = (png_uint_16)cv_apng_delay.value;

	apng_frames++;

	for (y = 0; y < height; y++)
	{
		row_pointers[y] = png_buf;
		png_buf += pitch;
	}

#ifndef PNG_STATIC
	if (aPNG_write_frame_head)
#endif
		aPNG_write_frame_head(apng_ptr, apng_info_ptr, row_pointers,
			vid.width, /* width */
			height,    /* height */
			0,         /* x offset */
			0,         /* y offset */
			framedelay, TICRATE,/* delay numerator and denominator */
			PNG_DISPOSE_OP_BACKGROUND, /* dispose */
			PNG_BLEND_OP_SOURCE        /* blend */
		                     );

	png_write_image(png_ptr, row_pointers);

#ifndef PNG_STATIC
	if (aPNG_write_frame_tail)
#endif
		aPNG_write_frame_tail(apng_ptr, apng_info_ptr);

	png_free(png_ptr, (png_voidp)row_pointers);
}

static void M_PNGfix_acTL(png_structp png_ptr, png_infop png_info_ptr,
		apng_infop png_ainfo_ptr)
{
	apng_set_acTL(png_ptr, png_info_ptr, png_ainfo_ptr, apng_frames, 0);

#ifndef NO_PNG_DEBUG
	png_debug(1, "in png_write_acTL\n");
#endif
}

static boolean M_SetupaPNG(png_const_charp filename, png_bytep pal)
{
	apng_FILE = fopen(filename,"wb+"); // + mode for reading
	if (!apng_FILE)
	{
		CONS_Debug(DBG_RENDER, "M_StartMovie: Error on opening %s for write\n", filename);
		return false;
	}

	apng_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL,
	 PNG_error, PNG_warn);
	if (!apng_ptr)
	{
		CONS_Debug(DBG_RENDER, "M_StartMovie: Error on initialize libpng\n");
		fclose(apng_FILE);
		remove(filename);
		return false;
	}

	apng_info_ptr = png_create_info_struct(apng_ptr);
	if (!apng_info_ptr)
	{
		CONS_Debug(DBG_RENDER, "M_StartMovie: Error on allocate for libpng\n");
		png_destroy_write_struct(&apng_ptr,  NULL);
		fclose(apng_FILE);
		remove(filename);
		return false;
	}

	apng_ainfo_ptr = apng_create_info_struct(apng_ptr);
	if (!apng_ainfo_ptr)
	{
		CONS_Debug(DBG_RENDER, "M_StartMovie: Error on allocate for apng\n");
		png_destroy_write_struct(&apng_ptr, &apng_info_ptr);
		fclose(apng_FILE);
		remove(filename);
		return false;
	}

	png_init_io(apng_ptr, apng_FILE);

#ifdef PNG_SET_USER_LIMITS_SUPPORTED
	png_set_user_limits(apng_ptr, MAXVIDWIDTH, MAXVIDHEIGHT);
#endif

	//png_set_filter(apng_ptr, 0, PNG_ALL_FILTERS);

	png_set_compression_level(apng_ptr, cv_zlib_levela.value);
	png_set_compression_mem_level(apng_ptr, cv_zlib_memorya.value);
	png_set_compression_strategy(apng_ptr, cv_zlib_strategya.value);
	png_set_compression_window_bits(apng_ptr, cv_zlib_window_bitsa.value);

	M_PNGhdr(apng_ptr, apng_info_ptr, vid.width, vid.height, pal);

	M_PNGText(apng_ptr, apng_info_ptr, true);

	apng_set_set_acTL_fn(apng_ptr, apng_ainfo_ptr, aPNG_set_acTL);

	apng_set_acTL(apng_ptr, apng_info_ptr, apng_ainfo_ptr, PNG_UINT_31_MAX, 0);

	apng_write_info(apng_ptr, apng_info_ptr, apng_ainfo_ptr);

	apng_frames = 0;

	return true;
}
#endif
#endif

// ==========================================================================
//                             MOVIE MODE
// ==========================================================================
#if NUMSCREENS > 2
static inline moviemode_t M_StartMovieAPNG(const char *pathname)
{
#ifdef USE_APNG
	UINT8 *palette = NULL;
	const char *freename = NULL;
	boolean ret = false;

	if (!M_PNGLib())
	{
		CONS_Alert(CONS_ERROR, "Couldn't create aPNG: libpng not found\n");
		return MM_OFF;
	}

	if (!(freename = Newsnapshotfile(pathname,"png")))
	{
		CONS_Alert(CONS_ERROR, "Couldn't create aPNG: no slots open in %s\n", pathname);
		return MM_OFF;
	}

	if (rendermode == render_soft)
	{
		M_CreateScreenShotPalette();
		palette = screenshot_palette;
	}

	ret = M_SetupaPNG(va(pandf,pathname,freename), palette);

	if (!ret)
	{
		CONS_Alert(CONS_ERROR, "Couldn't create aPNG: error creating %s in %s\n", freename, pathname);
		return MM_OFF;
	}
	return MM_APNG;
#else
	// no APNG support exists
	(void)pathname;
	CONS_Alert(CONS_ERROR, "Couldn't create aPNG: this build lacks aPNG support\n");
	return MM_OFF;
#endif
}

static inline moviemode_t M_StartMovieGIF(const char *pathname)
{
#ifdef HAVE_ANIGIF
	const char *freename;

	if (!(freename = Newsnapshotfile(pathname,"gif")))
	{
		CONS_Alert(CONS_ERROR, "Couldn't create GIF: no slots open in %s\n", pathname);
		return MM_OFF;
	}

	if (!GIF_open(va(pandf,pathname,freename)))
	{
		CONS_Alert(CONS_ERROR, "Couldn't create GIF: error creating %s in %s\n", freename, pathname);
		return MM_OFF;
	}
	return MM_GIF;
#else
	// no GIF support exists
	(void)pathname;
	CONS_Alert(CONS_ERROR, "Couldn't create GIF: this build lacks GIF support\n");
	return MM_OFF;
#endif
}
#endif

void M_StartMovie(void)
{
#if NUMSCREENS > 2
	char pathname[MAX_WADPATH];

	if (moviemode)
		return;

	if (cv_movie_option.value == 0)
		strcpy(pathname, usehome ? srb2home : srb2path);
	else if (cv_movie_option.value == 1)
		strcpy(pathname, srb2home);
	else if (cv_movie_option.value == 2)
		strcpy(pathname, srb2path);
	else if (cv_movie_option.value == 3 && *cv_movie_folder.string != '\0')
		strcpy(pathname, cv_movie_folder.string);

	if (cv_movie_option.value != 3)
	{
		strcat(pathname, PATHSEP"movies"PATHSEP);
		I_mkdir(pathname, 0755);
	}

	if (rendermode == render_none)
		I_Error("Can't make a movie without a render system\n");

	switch (cv_moviemode.value)
	{
		case MM_GIF:
			moviemode = M_StartMovieGIF(pathname);
			break;
		case MM_APNG:
			moviemode = M_StartMovieAPNG(pathname);
			break;
		case MM_SCREENSHOT:
			moviemode = MM_SCREENSHOT;
			break;
		default: //???
			return;
	}

	if (moviemode == MM_APNG)
		CONS_Printf(M_GetText("Movie mode enabled (%s).\n"), "aPNG");
	else if (moviemode == MM_GIF)
		CONS_Printf(M_GetText("Movie mode enabled (%s).\n"), "GIF");
	else if (moviemode == MM_SCREENSHOT)
		CONS_Printf(M_GetText("Movie mode enabled (%s).\n"), "screenshots");

	//singletics = (moviemode != MM_OFF);
#endif
}

void M_SaveFrame(void)
{
#if NUMSCREENS > 2
	// paranoia: should be unnecessary without singletics
	static tic_t oldtic = 0;

	if (oldtic == I_GetTime())
		return;
	else
		oldtic = I_GetTime();

	switch (moviemode)
	{
		case MM_SCREENSHOT:
			takescreenshot = true;
			return;
		case MM_GIF:
			GIF_frame();
			return;
		case MM_APNG:
#ifdef USE_APNG
			{
				UINT8 *linear = NULL;
				if (!apng_FILE) // should not happen!!
				{
					moviemode = MM_OFF;
					return;
				}

				if (rendermode == render_soft)
				{
					// munge planar buffer to linear
					linear = screens[2];
					I_ReadScreen(linear);
				}
#ifdef HWRENDER
				else
					linear = HWR_GetScreenshot();
#endif
				M_PNGFrame(apng_ptr, apng_info_ptr, (png_bytep)linear);
#ifdef HWRENDER
				if (rendermode != render_soft && linear)
					free(linear);
#endif

				if (apng_frames == PNG_UINT_31_MAX)
				{
					CONS_Alert(CONS_NOTICE, M_GetText("Max movie size reached\n"));
					M_StopMovie();
				}
			}
#else
			moviemode = MM_OFF;
#endif
			return;
		default:
			return;
	}
#endif
}

void M_StopMovie(void)
{
#if NUMSCREENS > 2
	switch (moviemode)
	{
		case MM_GIF:
			if (!GIF_close())
				return;
			break;
		case MM_APNG:
#ifdef USE_APNG
			if (!apng_FILE)
				return;

			if (apng_frames)
			{
				M_PNGfix_acTL(apng_ptr, apng_info_ptr, apng_ainfo_ptr);
				apng_write_end(apng_ptr, apng_info_ptr, apng_ainfo_ptr);
			}

			png_destroy_write_struct(&apng_ptr, &apng_info_ptr);

			fclose(apng_FILE);
			apng_FILE = NULL;
			CONS_Printf("aPNG closed; wrote %u frames\n", (UINT32)apng_frames);
			apng_frames = 0;
			break;
#else
			return;
#endif
		case MM_SCREENSHOT:
			break;
		default:
			return;
	}
	moviemode = MM_OFF;
	CONS_Printf(M_GetText("Movie mode disabled.\n"));
#endif
}

// ==========================================================================
//                            SCREEN SHOTS
// ==========================================================================
#ifdef USE_PNG
/** Writes a PNG file to disk.
  *
  * \param filename Filename to write to.
  * \param data     The image data.
  * \param width    Width of the picture.
  * \param height   Height of the picture.
  * \param palette  Palette of image data.
  *  \note if palette is NULL, BGR888 format
  */
boolean M_SavePNG(const char *filename, void *data, int width, int height, const UINT8 *palette)
{
	png_structp png_ptr;
	png_infop png_info_ptr;
	PNG_CONST png_byte *PLTE = (const png_byte *)palette;
#ifdef PNG_SETJMP_SUPPORTED
#ifdef USE_FAR_KEYWORD
	jmp_buf jmpbuf;
#endif
#endif
	png_FILE_p png_FILE;

	png_FILE = fopen(filename,"wb");
	if (!png_FILE)
	{
		CONS_Debug(DBG_RENDER, "M_SavePNG: Error on opening %s for write\n", filename);
		return false;
	}

	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, PNG_error, PNG_warn);
	if (!png_ptr)
	{
		CONS_Debug(DBG_RENDER, "M_SavePNG: Error on initialize libpng\n");
		fclose(png_FILE);
		remove(filename);
		return false;
	}

	png_info_ptr = png_create_info_struct(png_ptr);
	if (!png_info_ptr)
	{
		CONS_Debug(DBG_RENDER, "M_SavePNG: Error on allocate for libpng\n");
		png_destroy_write_struct(&png_ptr,  NULL);
		fclose(png_FILE);
		remove(filename);
		return false;
	}

#ifdef USE_FAR_KEYWORD
	if (setjmp(jmpbuf))
#else
	if (setjmp(png_jmpbuf(png_ptr)))
#endif
	{
		//CONS_Debug(DBG_RENDER, "libpng write error on %s\n", filename);
		png_destroy_write_struct(&png_ptr, &png_info_ptr);
		fclose(png_FILE);
		remove(filename);
		return false;
	}
#ifdef USE_FAR_KEYWORD
	png_memcpy(png_jmpbuf(png_ptr),jmpbuf, sizeof (jmp_buf));
#endif
	png_init_io(png_ptr, png_FILE);

#ifdef PNG_SET_USER_LIMITS_SUPPORTED
	png_set_user_limits(png_ptr, MAXVIDWIDTH, MAXVIDHEIGHT);
#endif

	//png_set_filter(png_ptr, 0, PNG_ALL_FILTERS);

	png_set_compression_level(png_ptr, cv_zlib_level.value);
	png_set_compression_mem_level(png_ptr, cv_zlib_memory.value);
	png_set_compression_strategy(png_ptr, cv_zlib_strategy.value);
	png_set_compression_window_bits(png_ptr, cv_zlib_window_bits.value);

	M_PNGhdr(png_ptr, png_info_ptr, width, height, PLTE);

	M_PNGText(png_ptr, png_info_ptr, false);

	png_write_info(png_ptr, png_info_ptr);

	M_PNGImage(png_ptr, png_info_ptr, height, data);

	png_write_end(png_ptr, png_info_ptr);
	png_destroy_write_struct(&png_ptr, &png_info_ptr);

	fclose(png_FILE);
	return true;
}
#else
/** PCX file structure.
  */
typedef struct
{
	UINT8 manufacturer;
	UINT8 version;
	UINT8 encoding;
	UINT8 bits_per_pixel;

	UINT16 xmin, ymin;
	UINT16 xmax, ymax;
	UINT16 hres, vres;
	UINT8  palette[48];

	UINT8 reserved;
	UINT8 color_planes;
	UINT16 bytes_per_line;
	UINT16 palette_type;

	char filler[58];
	UINT8 data; ///< Unbounded; used for all picture data.
} pcx_t;

/** Writes a PCX file to disk.
  *
  * \param filename Filename to write to.
  * \param data     The image data.
  * \param width    Width of the picture.
  * \param height   Height of the picture.
  * \param palette  Palette of image data
  */
#if NUMSCREENS > 2
static boolean WritePCXfile(const char *filename, const UINT8 *data, int width, int height, const UINT8 *pal)
{
	int i;
	size_t length;
	pcx_t *pcx;
	UINT8 *pack;

	pcx = Z_Malloc(width*height*2 + 1000, PU_STATIC, NULL);

	pcx->manufacturer = 0x0a; // PCX id
	pcx->version = 5; // 256 color
	pcx->encoding = 1; // uncompressed
	pcx->bits_per_pixel = 8; // 256 color
	pcx->xmin = pcx->ymin = 0;
	pcx->xmax = SHORT(width - 1);
	pcx->ymax = SHORT(height - 1);
	pcx->hres = SHORT(width);
	pcx->vres = SHORT(height);
	memset(pcx->palette, 0, sizeof (pcx->palette));
	pcx->reserved = 0;
	pcx->color_planes = 1; // chunky image
	pcx->bytes_per_line = SHORT(width);
	pcx->palette_type = SHORT(1); // not a grey scale
	memset(pcx->filler, 0, sizeof (pcx->filler));

	// pack the image
	pack = &pcx->data;

	for (i = 0; i < width*height; i++)
	{
		if ((*data & 0xc0) != 0xc0)
			*pack++ = *data++;
		else
		{
			*pack++ = 0xc1;
			*pack++ = *data++;
		}
	}

	// write the palette
	*pack++ = 0x0c; // palette ID byte

	// write color table
	{
		for (i = 0; i < 256; i++)
		{
			*pack++ = *pal; pal++;
			*pack++ = *pal; pal++;
			*pack++ = *pal; pal++;
		}
	}

	// write output file
	length = pack - (UINT8 *)pcx;
	i = FIL_WriteFile(filename, pcx, length);

	Z_Free(pcx);
	return i;
}
#endif
#endif

void M_ScreenShot(void)
{
	takescreenshot = true;
}

/** Takes a screenshot.
  * The screenshot is saved as "srb2xxxx.png" where xxxx is the lowest
  * four-digit number for which a file does not already exist.
  *
  * \sa HWR_ScreenShot
  */
void M_DoScreenShot(void)
{
#if NUMSCREENS > 2
	const char *freename = NULL;
	char pathname[MAX_WADPATH];
	boolean ret = false;
	UINT8 *linear = NULL;

	// Don't take multiple screenshots, obviously
	takescreenshot = false;

	// how does one take a screenshot without a render system?
	if (rendermode == render_none)
		return;

	if (cv_screenshot_option.value == 0)
		strcpy(pathname, usehome ? srb2home : srb2path);
	else if (cv_screenshot_option.value == 1)
		strcpy(pathname, srb2home);
	else if (cv_screenshot_option.value == 2)
		strcpy(pathname, srb2path);
	else if (cv_screenshot_option.value == 3 && *cv_screenshot_folder.string != '\0')
		strcpy(pathname, cv_screenshot_folder.string);

	if (cv_screenshot_option.value != 3)
	{
		strcat(pathname, PATHSEP"screenshots"PATHSEP);
		I_mkdir(pathname, 0755);
	}

#ifdef USE_PNG
	freename = Newsnapshotfile(pathname,"png");
#else
	if (rendermode == render_soft)
		freename = Newsnapshotfile(pathname,"pcx");
	else if (rendermode == render_opengl)
		freename = Newsnapshotfile(pathname,"tga");
#endif

	if (rendermode == render_soft)
	{
		// munge planar buffer to linear
		linear = screens[2];
		I_ReadScreen(linear);
	}

	if (!freename)
		goto failure;

	// save the pcx file
#ifdef HWRENDER
	if (rendermode == render_opengl)
		ret = HWR_Screenshot(va(pandf,pathname,freename));
	else
#endif
	{
		M_CreateScreenShotPalette();
#ifdef USE_PNG
		ret = M_SavePNG(va(pandf,pathname,freename), linear, vid.width, vid.height, screenshot_palette);
#else
		ret = WritePCXfile(va(pandf,pathname,freename), linear, vid.width, vid.height, screenshot_palette);
#endif
	}

failure:
	if (ret)
	{
		if (moviemode != MM_SCREENSHOT)
			CONS_Printf(M_GetText("Screen shot %s saved in %s\n"), freename, pathname);
	}
	else
	{
		if (freename)
			CONS_Alert(CONS_ERROR, M_GetText("Couldn't create screen shot %s in %s\n"), freename, pathname);
		else
			CONS_Alert(CONS_ERROR, M_GetText("Couldn't create screen shot in %s (all 10000 slots used!)\n"), pathname);

		if (moviemode == MM_SCREENSHOT)
			M_StopMovie();
	}
#endif
}

boolean M_ScreenshotResponder(event_t *ev)
{
	INT32 ch = -1;
	if (dedicated || ev->type != ev_keydown)
		return false;

	ch = ev->data1;

	if (ch >= KEY_MOUSE1 && menuactive) // If it's not a keyboard key, then don't allow it in the menus!
		return false;

	if (ch == KEY_F8 || ch == gamecontrol[gc_screenshot][0] || ch == gamecontrol[gc_screenshot][1]) // remappable F8
		M_ScreenShot();
	else if (ch == KEY_F9 || ch == gamecontrol[gc_recordgif][0] || ch == gamecontrol[gc_recordgif][1]) // remappable F9
		((moviemode) ? M_StopMovie : M_StartMovie)();
	else
		return false;
	return true;
}

// ==========================================================================
//                       TRANSLATION FUNCTIONS
// ==========================================================================

// M_StartupLocale.
// Sets up gettext to translate SRB2's strings.
#ifdef GETTEXT
	#if defined (__unix__) || defined(__APPLE__) || defined (UNIXCOMMON)
		#define GETTEXTDOMAIN1 "/usr/share/locale"
		#define GETTEXTDOMAIN2 "/usr/local/share/locale"
	#elif defined (_WIN32)
		#define GETTEXTDOMAIN1 "."
	#endif
#endif // GETTEXT

void M_StartupLocale(void)
{
#ifdef GETTEXT
	char *textdomhandle = NULL;
#endif //GETTEXT

	CONS_Printf("M_StartupLocale...\n");

	setlocale(LC_ALL, "");

	// Do not set numeric locale as that affects atof
	setlocale(LC_NUMERIC, "C");

#ifdef GETTEXT
	// FIXME: global name define anywhere?
#ifdef GETTEXTDOMAIN1
	textdomhandle = bindtextdomain("srb2", GETTEXTDOMAIN1);
#endif
#ifdef GETTEXTDOMAIN2
	if (!textdomhandle)
		textdomhandle = bindtextdomain("srb2", GETTEXTDOMAIN2);
#endif
#ifdef GETTEXTDOMAIN3
	if (!textdomhandle)
		textdomhandle = bindtextdomain("srb2", GETTEXTDOMAIN3);
#endif
#ifdef GETTEXTDOMAIN4
	if (!textdomhandle)
		textdomhandle = bindtextdomain("srb2", GETTEXTDOMAIN4);
#endif
	if (textdomhandle)
		textdomain("srb2");
	else
		CONS_Printf("Could not find locale text domain!\n");
#endif //GETTEXT
}

// ==========================================================================
//                        MISC STRING FUNCTIONS
// ==========================================================================

/** Returns a temporary string made out of varargs.
  * For use with CONS_Printf().
  *
  * \param format Format string.
  * \return Pointer to a static buffer of 1024 characters, containing the
  *         resulting string.
  */
char *va(const char *format, ...)
{
	va_list argptr;
	static char string[1024];

	va_start(argptr, format);
	vsprintf(string, format, argptr);
	va_end(argptr);

	return string;
}

/** Creates a string in the first argument that is the second argument followed
  * by the third argument followed by the first argument.
  * Useful for making filenames with full path. s1 = s2+s3+s1
  *
  * \param s1 First string, suffix, and destination.
  * \param s2 Second string. Ends up first in the result.
  * \param s3 Third string. Ends up second in the result.
  */
void strcatbf(char *s1, const char *s2, const char *s3)
{
	char tmp[1024];

	strcpy(tmp, s1);
	strcpy(s1, s2);
	strcat(s1, s3);
	strcat(s1, tmp);
}

/** Converts an ASCII Hex string into an integer. Thanks, Borland!
  * <Inuyasha> I don't know if this belongs here specifically, but it sure
  *            doesn't belong in p_spec.c, that's for sure
  *
  * \param hexStg Hexadecimal string.
  * \return an Integer based off the contents of the string.
  */
INT32 axtoi(const char *hexStg)
{
	INT32 n = 0;
	INT32 m = 0;
	INT32 count;
	INT32 intValue = 0;
	INT32 digit[8];
	while (n < 8)
	{
		if (hexStg[n] == '\0')
			break;
		if (hexStg[n] >= '0' && hexStg[n] <= '9') // 0-9
			digit[n] = (hexStg[n] & 0x0f);
		else if (hexStg[n] >= 'a' && hexStg[n] <= 'f') // a-f
			digit[n] = (hexStg[n] & 0x0f) + 9;
		else if (hexStg[n] >= 'A' && hexStg[n] <= 'F') // A-F
			digit[n] = (hexStg[n] & 0x0f) + 9;
		else
			break;
		n++;
	}
	count = n;
	m = n - 1;
	n = 0;
	while (n < count)
	{
		intValue = intValue | (digit[n] << (m << 2));
		m--;
		n++;
	}
	return intValue;
}

// Token parser variables

static UINT32 oldendPos = 0; // old value of endPos, used by M_UnGetToken
static UINT32 endPos = 0; // now external to M_GetToken, but still static

/** Token parser for TEXTURES, ANIMDEFS, and potentially other lumps later down the line.
  * Was originally R_GetTexturesToken when I was coding up the TEXTURES parser, until I realized I needed it for ANIMDEFS too.
  * Parses up to the next whitespace character or comma. When finding the start of the next token, whitespace is skipped.
  * Commas are not; if a comma is encountered, then THAT'S returned as the token.
  * -Shadow Hog
  *
  * \param inputString The string to be parsed. If NULL is supplied instead of a string, it will continue parsing the last supplied one.
  * The pointer to the last string supplied is stored as a static variable, so be careful not to free it while this function is still using it!
  * \return A pointer to a string, containing the fetched token. This is in freshly allocated memory, so be sure to Z_Free() it as appropriate.
*/
char *M_GetToken(const char *inputString)
{
	static const char *stringToUse = NULL; // Populated if inputString != NULL; used otherwise
	static UINT32 startPos = 0;
//	static UINT32 endPos = 0;
	static UINT32 stringLength = 0;
	static UINT8 inComment = 0; // 0 = not in comment, 1 = // Single-line, 2 = /* Multi-line */
	char *texturesToken = NULL;
	UINT32 texturesTokenLength = 0;

	if (inputString != NULL)
	{
		stringToUse = inputString;
		startPos = 0;
		oldendPos = endPos = 0;
		stringLength = strlen(inputString);
	}
	else
	{
		startPos = oldendPos = endPos;
	}
	if (stringToUse == NULL)
		return NULL;

	// Try to detect comments now, in case we're pointing right at one
	if (startPos < stringLength - 1
		&& inComment == 0)
	{
		if (stringToUse[startPos] == '/'
			&& stringToUse[startPos+1] == '/')
		{
			//Single-line comment start
			inComment = 1;
		}
		else if (stringToUse[startPos] == '/'
			&& stringToUse[startPos+1] == '*')
		{
			//Multi-line comment start
			inComment = 2;
		}
	}

	// Find the first non-whitespace char, or else the end of the string trying
	while ((stringToUse[startPos] == ' '
			|| stringToUse[startPos] == '\t'
			|| stringToUse[startPos] == '\r'
			|| stringToUse[startPos] == '\n'
			|| stringToUse[startPos] == '\0'
			|| stringToUse[startPos] == '=' || stringToUse[startPos] == ';' // UDMF TEXTMAP.
			|| inComment != 0)
			&& startPos < stringLength)
	{
		// Try to detect comment endings now
		if (inComment == 1
			&& stringToUse[startPos] == '\n')
		{
			// End of line for a single-line comment
			inComment = 0;
		}
		else if (inComment == 2
			&& startPos < stringLength - 1
			&& stringToUse[startPos] == '*'
			&& stringToUse[startPos+1] == '/')
		{
			// End of multi-line comment
			inComment = 0;
			startPos++; // Make damn well sure we're out of the comment ending at the end of it all
		}

		startPos++;

		// Try to detect comment starts now
		if (startPos < stringLength - 1
			&& inComment == 0)
		{
			if (stringToUse[startPos] == '/'
				&& stringToUse[startPos+1] == '/')
			{
				//Single-line comment start
				inComment = 1;
			}
			else if (stringToUse[startPos] == '/'
				&& stringToUse[startPos+1] == '*')
			{
				//Multi-line comment start
				inComment = 2;
			}
		}
	}

	// If the end of the string is reached, no token is to be read
	if (startPos == stringLength) {
		endPos = stringLength;
		return NULL;
	}
	// Else, if it's one of these three symbols, capture only this one character
	else if (stringToUse[startPos] == ','
			|| stringToUse[startPos] == '{'
			|| stringToUse[startPos] == '}')
	{
		endPos = startPos + 1;
		texturesToken = (char *)Z_Malloc(2*sizeof(char),PU_STATIC,NULL);
		texturesToken[0] = stringToUse[startPos];
		texturesToken[1] = '\0';
		return texturesToken;
	}
	// Return entire string within quotes, except without the quotes.
	else if (stringToUse[startPos] == '"')
	{
		endPos = ++startPos;
		while (stringToUse[endPos] != '"' && endPos < stringLength)
			endPos++;

		texturesTokenLength = endPos++ - startPos;
		// Assign the memory. Don't forget an extra byte for the end of the string!
		texturesToken = (char *)Z_Malloc((texturesTokenLength+1)*sizeof(char),PU_STATIC,NULL);
		// Copy the string.
		M_Memcpy(texturesToken, stringToUse+startPos, (size_t)texturesTokenLength);
		// Make the final character NUL.
		texturesToken[texturesTokenLength] = '\0';

		return texturesToken;
	}

	// Now find the end of the token. This includes several additional characters that are okay to capture as one character, but not trailing at the end of another token.
	endPos = startPos + 1;
	while ((stringToUse[endPos] != ' '
			&& stringToUse[endPos] != '\t'
			&& stringToUse[endPos] != '\r'
			&& stringToUse[endPos] != '\n'
			&& stringToUse[endPos] != ','
			&& stringToUse[endPos] != '{'
			&& stringToUse[endPos] != '}'
			&& stringToUse[endPos] != '=' && stringToUse[endPos] != ';' // UDMF TEXTMAP.
			&& inComment == 0)
			&& endPos < stringLength)
	{
		endPos++;
		// Try to detect comment starts now; if it's in a comment, we don't want it in this token
		if (endPos < stringLength - 1
			&& inComment == 0)
		{
			if (stringToUse[endPos] == '/'
				&& stringToUse[endPos+1] == '/')
			{
				//Single-line comment start
				inComment = 1;
			}
			else if (stringToUse[endPos] == '/'
				&& stringToUse[endPos+1] == '*')
			{
				//Multi-line comment start
				inComment = 2;
			}
		}
	}
	texturesTokenLength = endPos - startPos;

	// Assign the memory. Don't forget an extra byte for the end of the string!
	texturesToken = (char *)Z_Malloc((texturesTokenLength+1)*sizeof(char),PU_STATIC,NULL);
	// Copy the string.
	M_Memcpy(texturesToken, stringToUse+startPos, (size_t)texturesTokenLength);
	// Make the final character NUL.
	texturesToken[texturesTokenLength] = '\0';
	return texturesToken;
}

/** Undoes the last M_GetToken call
  * The current position along the string being parsed is reset to the last saved position.
  * This exists mostly because of R_ParseTexture/R_ParsePatch honestly, but could be useful elsewhere?
  * -Monster Iestyn (22/10/16)
 */
void M_UnGetToken(void)
{
	endPos = oldendPos;
}

/** Returns the current token's position.
 */
UINT32 M_GetTokenPos(void)
{
	return endPos;
}

/** Sets the current token's position.
 */
void M_SetTokenPos(UINT32 newPos)
{
	endPos = newPos;
}

/** Count bits in a number.
  */
UINT8 M_CountBits(UINT32 num, UINT8 size)
{
	UINT8 i, sum = 0;

	for (i = 0; i < size; ++i)
		if (num & (1 << i))
			++sum;
	return sum;
}

const char *GetRevisionString(void)
{
	static char rev[9] = {0};
	if (rev[0])
		return rev;

	if (comprevision[0] == 'r')
		strncpy(rev, comprevision, 7);
	else
		snprintf(rev, 7, "r%s", comprevision);
	rev[7] = '\0';

	return rev;
}

// Vector/matrix math
TVector *VectorMatrixMultiply(TVector v, TMatrix m)
{
	static TVector ret;

	ret[0] = FixedMul(v[0],m[0][0]) + FixedMul(v[1],m[1][0]) + FixedMul(v[2],m[2][0]) + FixedMul(v[3],m[3][0]);
	ret[1] = FixedMul(v[0],m[0][1]) + FixedMul(v[1],m[1][1]) + FixedMul(v[2],m[2][1]) + FixedMul(v[3],m[3][1]);
	ret[2] = FixedMul(v[0],m[0][2]) + FixedMul(v[1],m[1][2]) + FixedMul(v[2],m[2][2]) + FixedMul(v[3],m[3][2]);
	ret[3] = FixedMul(v[0],m[0][3]) + FixedMul(v[1],m[1][3]) + FixedMul(v[2],m[2][3]) + FixedMul(v[3],m[3][3]);

	return &ret;
}

TMatrix *RotateXMatrix(angle_t rad)
{
	static TMatrix ret;
	const angle_t fa = rad>>ANGLETOFINESHIFT;
	const fixed_t cosrad = FINECOSINE(fa), sinrad = FINESINE(fa);

	ret[0][0] = FRACUNIT; ret[0][1] =       0; ret[0][2] = 0;        ret[0][3] = 0;
	ret[1][0] =        0; ret[1][1] =  cosrad; ret[1][2] = sinrad;   ret[1][3] = 0;
	ret[2][0] =        0; ret[2][1] = -sinrad; ret[2][2] = cosrad;   ret[2][3] = 0;
	ret[3][0] =        0; ret[3][1] =       0; ret[3][2] = 0;        ret[3][3] = FRACUNIT;

	return &ret;
}

#if 0
TMatrix *RotateYMatrix(angle_t rad)
{
	static TMatrix ret;
	const angle_t fa = rad>>ANGLETOFINESHIFT;
	const fixed_t cosrad = FINECOSINE(fa), sinrad = FINESINE(fa);

	ret[0][0] = cosrad;   ret[0][1] =        0; ret[0][2] = -sinrad;   ret[0][3] = 0;
	ret[1][0] = 0;        ret[1][1] = FRACUNIT; ret[1][2] = 0;         ret[1][3] = 0;
	ret[2][0] = sinrad;   ret[2][1] =        0; ret[2][2] = cosrad;    ret[2][3] = 0;
	ret[3][0] = 0;        ret[3][1] =        0; ret[3][2] = 0;         ret[3][3] = FRACUNIT;

	return &ret;
}
#endif

TMatrix *RotateZMatrix(angle_t rad)
{
	static TMatrix ret;
	const angle_t fa = rad>>ANGLETOFINESHIFT;
	const fixed_t cosrad = FINECOSINE(fa), sinrad = FINESINE(fa);

	ret[0][0] = cosrad;    ret[0][1] = sinrad;   ret[0][2] =        0; ret[0][3] = 0;
	ret[1][0] = -sinrad;   ret[1][1] = cosrad;   ret[1][2] =        0; ret[1][3] = 0;
	ret[2][0] = 0;         ret[2][1] = 0;        ret[2][2] = FRACUNIT; ret[2][3] = 0;
	ret[3][0] = 0;         ret[3][1] = 0;        ret[3][2] =        0; ret[3][3] = FRACUNIT;

	return &ret;
}

/** Set of functions to take in a size_t as an argument,
  * put the argument in a character buffer, and return the
  * pointer to that buffer.
  * This is to eliminate usage of PRIdS, so gettext can work
  * with *all* of SRB2's strings.
  */
char *sizeu1(size_t num)
{
	static char sizeu1_buf[28];
	sprintf(sizeu1_buf, "%"PRIdS, num);
	return sizeu1_buf;
}

char *sizeu2(size_t num)
{
	static char sizeu2_buf[28];
	sprintf(sizeu2_buf, "%"PRIdS, num);
	return sizeu2_buf;
}

char *sizeu3(size_t num)
{
	static char sizeu3_buf[28];
	sprintf(sizeu3_buf, "%"PRIdS, num);
	return sizeu3_buf;
}

char *sizeu4(size_t num)
{
	static char sizeu4_buf[28];
	sprintf(sizeu4_buf, "%"PRIdS, num);
	return sizeu4_buf;
}

char *sizeu5(size_t num)
{
	static char sizeu5_buf[28];
	sprintf(sizeu5_buf, "%"PRIdS, num);
	return sizeu5_buf;
}

#if defined (__GNUC__) && defined (__i386__) // from libkwave, under GPL
// Alam: note libkwave memcpy code comes from mplayer's libvo/aclib_template.c, r699

/* for small memory blocks (<256 bytes) this version is faster */
#define small_memcpy(dest,src,n)\
{\
register unsigned long int dummy;\
__asm__ __volatile__(\
	"cld\n\t"\
	"rep; movsb"\
	:"=&D"(dest), "=&S"(src), "=&c"(dummy)\
	:"0" (dest), "1" (src),"2" (n)\
	: "memory", "cc");\
}
/* linux kernel __memcpy (from: /include/asm/string.h) */
ATTRINLINE static FUNCINLINE void *__memcpy (void *dest, const void * src, size_t n)
{
	int d0, d1, d2;

	if ( n < 4 )
	{
		small_memcpy(dest, src, n);
	}
	else
	{
		__asm__ __volatile__ (
			"rep ; movsl;"
			"testb $2,%b4;"
			"je 1f;"
			"movsw;"
			"1:\ttestb $1,%b4;"
			"je 2f;"
			"movsb;"
			"2:"
		: "=&c" (d0), "=&D" (d1), "=&S" (d2)
		:"0" (n/4), "q" (n),"1" ((long) dest),"2" ((long) src)
		: "memory");
	}

	return dest;
}

#define SSE_MMREG_SIZE 16
#define MMX_MMREG_SIZE 8

#define MMX1_MIN_LEN 0x800  /* 2K blocks */
#define MIN_LEN 0x40  /* 64-byte blocks */

/* SSE note: i tried to move 128 bytes a time instead of 64 but it
didn't make any measureable difference. i'm using 64 for the sake of
simplicity. [MF] */
static /*FUNCTARGET("sse2")*/ void *sse_cpy(void * dest, const void * src, size_t n)
{
	void *retval = dest;
	size_t i;

	/* PREFETCH has effect even for MOVSB instruction ;) */
	__asm__ __volatile__ (
		"prefetchnta (%0);"
		"prefetchnta 32(%0);"
		"prefetchnta 64(%0);"
		"prefetchnta 96(%0);"
		"prefetchnta 128(%0);"
		"prefetchnta 160(%0);"
		"prefetchnta 192(%0);"
		"prefetchnta 224(%0);"
		"prefetchnta 256(%0);"
		"prefetchnta 288(%0);"
		: : "r" (src) );

	if (n >= MIN_LEN)
	{
		register unsigned long int delta;
		/* Align destinition to MMREG_SIZE -boundary */
		delta = ((unsigned long int)dest)&(SSE_MMREG_SIZE-1);
		if (delta)
		{
			delta=SSE_MMREG_SIZE-delta;
			n -= delta;
			small_memcpy(dest, src, delta);
		}
		i = n >> 6; /* n/64 */
		n&=63;
		if (((unsigned long)src) & 15)
		/* if SRC is misaligned */
		 for (; i>0; i--)
		 {
			__asm__ __volatile__ (
				"prefetchnta 320(%0);"
				"prefetchnta 352(%0);"
				"movups (%0), %%xmm0;"
				"movups 16(%0), %%xmm1;"
				"movups 32(%0), %%xmm2;"
				"movups 48(%0), %%xmm3;"
				"movntps %%xmm0, (%1);"
				"movntps %%xmm1, 16(%1);"
				"movntps %%xmm2, 32(%1);"
				"movntps %%xmm3, 48(%1);"
			:: "r" (src), "r" (dest) : "memory");
			src = (const unsigned char *)src + 64;
			dest = (unsigned char *)dest + 64;
		}
		else
			/*
			   Only if SRC is aligned on 16-byte boundary.
			   It allows to use movaps instead of movups, which required data
			   to be aligned or a general-protection exception (#GP) is generated.
			*/
		 for (; i>0; i--)
		 {
			__asm__ __volatile__ (
				"prefetchnta 320(%0);"
				"prefetchnta 352(%0);"
				"movaps (%0), %%xmm0;"
				"movaps 16(%0), %%xmm1;"
				"movaps 32(%0), %%xmm2;"
				"movaps 48(%0), %%xmm3;"
				"movntps %%xmm0, (%1);"
				"movntps %%xmm1, 16(%1);"
				"movntps %%xmm2, 32(%1);"
				"movntps %%xmm3, 48(%1);"
			:: "r" (src), "r" (dest) : "memory");
			src = ((const unsigned char *)src) + 64;
			dest = ((unsigned char *)dest) + 64;
		}
		/* since movntq is weakly-ordered, a "sfence"
		 * is needed to become ordered again. */
		__asm__ __volatile__ ("sfence":::"memory");
		/* enables to use FPU */
		__asm__ __volatile__ ("emms":::"memory");
	}
	/*
	 *	Now do the tail of the block
	 */
	if (n) __memcpy(dest, src, n);
	return retval;
}

static FUNCTARGET("mmx") void *mmx2_cpy(void *dest, const void *src, size_t n)
{
	void *retval = dest;
	size_t i;

	/* PREFETCH has effect even for MOVSB instruction ;) */
	__asm__ __volatile__ (
		"prefetchnta (%0);"
		"prefetchnta 32(%0);"
		"prefetchnta 64(%0);"
		"prefetchnta 96(%0);"
		"prefetchnta 128(%0);"
		"prefetchnta 160(%0);"
		"prefetchnta 192(%0);"
		"prefetchnta 224(%0);"
		"prefetchnta 256(%0);"
		"prefetchnta 288(%0);"
	: : "r" (src));

	if (n >= MIN_LEN)
	{
		register unsigned long int delta;
		/* Align destinition to MMREG_SIZE -boundary */
		delta = ((unsigned long int)dest)&(MMX_MMREG_SIZE-1);
		if (delta)
		{
			delta=MMX_MMREG_SIZE-delta;
			n -= delta;
			small_memcpy(dest, src, delta);
		}
		i = n >> 6; /* n/64 */
		n&=63;
		for (; i>0; i--)
		{
			__asm__ __volatile__ (
				"prefetchnta 320(%0);"
				"prefetchnta 352(%0);"
				"movq (%0), %%mm0;"
				"movq 8(%0), %%mm1;"
				"movq 16(%0), %%mm2;"
				"movq 24(%0), %%mm3;"
				"movq 32(%0), %%mm4;"
				"movq 40(%0), %%mm5;"
				"movq 48(%0), %%mm6;"
				"movq 56(%0), %%mm7;"
				"movntq %%mm0, (%1);"
				"movntq %%mm1, 8(%1);"
				"movntq %%mm2, 16(%1);"
				"movntq %%mm3, 24(%1);"
				"movntq %%mm4, 32(%1);"
				"movntq %%mm5, 40(%1);"
				"movntq %%mm6, 48(%1);"
				"movntq %%mm7, 56(%1);"
			:: "r" (src), "r" (dest) : "memory");
			src = ((const unsigned char *)src) + 64;
			dest = ((unsigned char *)dest) + 64;
		}
		/* since movntq is weakly-ordered, a "sfence"
		* is needed to become ordered again. */
		__asm__ __volatile__ ("sfence":::"memory");
		__asm__ __volatile__ ("emms":::"memory");
	}
	/*
	 *	Now do the tail of the block
	 */
	if (n) __memcpy(dest, src, n);
	return retval;
}

static FUNCTARGET("mmx") void *mmx1_cpy(void *dest, const void *src, size_t n) //3DNOW
{
	void *retval = dest;
	size_t i;

	/* PREFETCH has effect even for MOVSB instruction ;) */
	__asm__ __volatile__ (
		"prefetch (%0);"
		"prefetch 32(%0);"
		"prefetch 64(%0);"
		"prefetch 96(%0);"
		"prefetch 128(%0);"
		"prefetch 160(%0);"
		"prefetch 192(%0);"
		"prefetch 224(%0);"
		"prefetch 256(%0);"
		"prefetch 288(%0);"
	: : "r" (src));

	if (n >= MMX1_MIN_LEN)
	{
		register unsigned long int delta;
		/* Align destinition to MMREG_SIZE -boundary */
		delta = ((unsigned long int)dest)&(MMX_MMREG_SIZE-1);
		if (delta)
		{
			delta=MMX_MMREG_SIZE-delta;
			n -= delta;
			small_memcpy(dest, src, delta);
		}
		i = n >> 6; /* n/64 */
		n&=63;
		for (; i>0; i--)
		{
			__asm__ __volatile__ (
				"prefetch 320(%0);"
				"prefetch 352(%0);"
				"movq (%0), %%mm0;"
				"movq 8(%0), %%mm1;"
				"movq 16(%0), %%mm2;"
				"movq 24(%0), %%mm3;"
				"movq 32(%0), %%mm4;"
				"movq 40(%0), %%mm5;"
				"movq 48(%0), %%mm6;"
				"movq 56(%0), %%mm7;"
				"movq %%mm0, (%1);"
				"movq %%mm1, 8(%1);"
				"movq %%mm2, 16(%1);"
				"movq %%mm3, 24(%1);"
				"movq %%mm4, 32(%1);"
				"movq %%mm5, 40(%1);"
				"movq %%mm6, 48(%1);"
				"movq %%mm7, 56(%1);"
			:: "r" (src), "r" (dest) : "memory");
			src = ((const unsigned char *)src) + 64;
			dest = ((unsigned char *)dest) + 64;
		}
		__asm__ __volatile__ ("femms":::"memory"); // same as mmx_cpy() but with a femms
	}
	/*
	 *	Now do the tail of the block
	 */
	if (n) __memcpy(dest, src, n);
	return retval;
}
#endif

// Alam: why? memcpy may be __cdecl/_System and our code may be not the same type
static void *cpu_cpy(void *dest, const void *src, size_t n)
{
	if (src == NULL)
	{
		CONS_Debug(DBG_MEMORY, "Memcpy from 0x0?!: %p %p %s\n", dest, src, sizeu1(n));
		return dest;
	}

	if(dest == NULL)
	{
		CONS_Debug(DBG_MEMORY, "Memcpy to 0x0?!: %p %p %s\n", dest, src, sizeu1(n));
		return dest;
	}

	return memcpy(dest, src, n);
}

static /*FUNCTARGET("mmx")*/ void *mmx_cpy(void *dest, const void *src, size_t n)
{
#if defined (_MSC_VER) && defined (_X86_)
	_asm
	{
		mov ecx, [n]
		mov esi, [src]
		mov edi, [dest]
		shr ecx, 6 // mit mmx: 64bytes per iteration
		jz lower_64 // if lower than 64 bytes
		loop_64: // MMX transfers multiples of 64bytes
		movq mm0,  0[ESI] // read sources
		movq mm1,  8[ESI]
		movq mm2, 16[ESI]
		movq mm3, 24[ESI]
		movq mm4, 32[ESI]
		movq mm5, 40[ESI]
		movq mm6, 48[ESI]
		movq mm7, 56[ESI]

		movq  0[EDI], mm0 // write destination
		movq  8[EDI], mm1
		movq 16[EDI], mm2
		movq 24[EDI], mm3
		movq 32[EDI], mm4
		movq 40[EDI], mm5
		movq 48[EDI], mm6
		movq 56[EDI], mm7

		add esi, 64
		add edi, 64
		dec ecx
		jnz loop_64
		emms // close mmx operation
		lower_64:// transfer rest of buffer
		mov ebx,esi
		sub ebx,src
		mov ecx,[n]
		sub ecx,ebx
		shr ecx, 3 // multiples of 8 bytes
		jz lower_8
		loop_8:
		movq  mm0, [esi] // read source
		movq [edi], mm0 // write destination
		add esi, 8
		add edi, 8
		dec ecx
		jnz loop_8
		emms // close mmx operation
		lower_8:
		mov ebx,esi
		sub ebx,src
		mov ecx,[n]
		sub ecx,ebx
		rep movsb
		mov eax, [dest] // return dest
	}
#elif defined (__GNUC__) && defined (__i386__)
	void *retval = dest;
	size_t i;

	if (n >= MMX1_MIN_LEN)
	{
		register unsigned long int delta;
		/* Align destinition to MMREG_SIZE -boundary */
		delta = ((unsigned long int)dest)&(MMX_MMREG_SIZE-1);
		if (delta)
		{
			delta=MMX_MMREG_SIZE-delta;
			n -= delta;
			small_memcpy(dest, src, delta);
		}
		i = n >> 6; /* n/64 */
		n&=63;
		for (; i>0; i--)
		{
			__asm__ __volatile__ (
				"movq (%0), %%mm0;"
				"movq 8(%0), %%mm1;"
				"movq 16(%0), %%mm2;"
				"movq 24(%0), %%mm3;"
				"movq 32(%0), %%mm4;"
				"movq 40(%0), %%mm5;"
				"movq 48(%0), %%mm6;"
				"movq 56(%0), %%mm7;"
				"movq %%mm0, (%1);"
				"movq %%mm1, 8(%1);"
				"movq %%mm2, 16(%1);"
				"movq %%mm3, 24(%1);"
				"movq %%mm4, 32(%1);"
				"movq %%mm5, 40(%1);"
				"movq %%mm6, 48(%1);"
				"movq %%mm7, 56(%1);"
			:: "r" (src), "r" (dest) : "memory");
			src = ((const unsigned char *)src) + 64;
			dest = ((unsigned char *)dest) + 64;
		}
		__asm__ __volatile__ ("emms":::"memory");
	}
	/*
	 *	Now do the tail of the block
	 */
	if (n) __memcpy(dest, src, n);
	return retval;
#else
	return cpu_cpy(dest, src, n);
#endif
}

void *(*M_Memcpy)(void* dest, const void* src, size_t n) = cpu_cpy;

/** Memcpy that uses MMX, 3DNow, MMXExt or even SSE
  * Do not use on overlapped memory, use memmove for that
  */
void M_SetupMemcpy(void)
{
#if defined (__GNUC__) && defined (__i386__)
	if (R_SSE2)
		M_Memcpy = sse_cpy;
	else if (R_MMXExt)
		M_Memcpy = mmx2_cpy;
	else if (R_3DNow)
		M_Memcpy = mmx1_cpy;
	else
#endif
	if (R_MMX)
		M_Memcpy = mmx_cpy;
#if 0
	M_Memcpy = cpu_cpy;
#endif
}

/** Return the appropriate message for a file error or end of file.
*/
const char *M_FileError(FILE *fp)
{
	if (ferror(fp))
		return strerror(errno);
	else
		return "end-of-file";
}

/** Return the number of parts of this path.
*/
int M_PathParts(const char *path)
{
	int n;
	const char *p;
	const char *t;
	if (path == NULL)
		return 0;
	for (n = 0, p = path ;; ++n)
	{
		t = p;
		if (( p = strchr(p, PATHSEP[0]) ))
			p += strspn(p, PATHSEP);
		else
		{
			if (*t)/* there is something after the final delimiter */
				n++;
			break;
		}
	}
	return n;
}

/** Check whether a path is an absolute path.
*/
boolean M_IsPathAbsolute(const char *path)
{
#ifdef _WIN32
	return ( strncmp(&path[1], ":\\", 2) == 0 );
#else
	return ( path[0] == '/' );
#endif
}

/** I_mkdir for each part of the path.
*/
void M_MkdirEachUntil(const char *cpath, int start, int end, int mode)
{
	char path[MAX_WADPATH];
	char *p;
	char *t;

	if (end > 0 && end <= start)
		return;

	strlcpy(path, cpath, sizeof path);
#ifdef _WIN32
	if (strncmp(&path[1], ":\\", 2) == 0)
		p = &path[3];
	else
#endif
		p = path;

	if (end > 0)
		end -= start;

	for (; start > 0; --start)
	{
		p += strspn(p, PATHSEP);
		if (!( p = strchr(p, PATHSEP[0]) ))
			return;
	}
	p += strspn(p, PATHSEP);
	for (;;)
	{
		if (end > 0 && !--end)
			break;

		t = p;
		if (( p = strchr(p, PATHSEP[0]) ))
		{
			*p = '\0';
			I_mkdir(path, mode);
			*p = PATHSEP[0];
			p += strspn(p, PATHSEP);
		}
		else
		{
			if (*t)
				I_mkdir(path, mode);
			break;
		}
	}
}

void M_MkdirEach(const char *path, int start, int mode)
{
	M_MkdirEachUntil(path, start, -1, mode);
}

int M_JumpWord(const char *line)
{
	int c;

	c = line[0];

	if (isspace(c))
		return strspn(line, " ");
	else if (ispunct(c))
		return strspn(line, PUNCTUATION);
	else
	{
		if (isspace(line[1]))
			return 1 + strspn(&line[1], " ");
		else
			return strcspn(line, " "PUNCTUATION);
	}
}

int M_JumpWordReverse(const char *line, int offset)
{
	int (*is)(int);
	int c;
	c = line[--offset];
	if (isspace(c))
		is = isspace;
	else if (ispunct(c))
		is = ispunct;
	else
		is = isalnum;
	c = (*is)(line[offset]);
	while (offset > 0 &&
			(*is)(line[offset - 1]) == c)
		offset--;
	return offset;
}

const char * M_Ftrim (double f)
{
	static char dig[9];/* "0." + 6 digits (6 is printf's default) */
	int i;
	/* I know I said it's the default, but just in case... */
	sprintf(dig, "%.6f", fabs(modf(f, &f)));
	/* trim trailing zeroes */
	for (i = strlen(dig)-1; dig[i] == '0'; --i)
		;
	if (dig[i] == '.')/* :NOTHING: */
		return "";
	else
	{
		dig[i + 1] = '\0';
		return &dig[1];/* skip the 0 */
	}
}

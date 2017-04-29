/// \file
/// \brief Support to find files

#ifndef __FILESRCH_H__
#define __FILESRCH_H__

#include "doomdef.h"
#include "d_netfil.h"

/**	\brief	The filesearch function

	This function search files, manly WADs and return back the status of the file

	\param	filename	the file to look for
	\param	startpath	where to start look from
	\param	wantedmd5sum	want to check with MD5
	\param	completepath	want to return the complete path of the file?
	\param	maxsearchdepth	the max depth to search for the file

	\return	filestatus_t


*/

filestatus_t filesearch(char *filename, const char *startpath, const UINT8 *wantedmd5sum,
	boolean completepath, int maxsearchdepth);

#define menudepth 20

extern char menupath[1024];
extern size_t menupathindex[menudepth];
extern size_t menudepthleft;

extern char **dirmenu;
extern size_t sizedirmenu;
extern size_t dir_on[menudepth];
extern UINT8 refreshdirmenu;

extern size_t packetsizetally;

typedef enum
{
	EXT_FOLDER = 0,
	EXT_UP,
	EXT_START,
	EXT_TXT = EXT_START,
	EXT_CFG,
	EXT_MD5,
	EXT_WAD = EXT_MD5,
	EXT_SOC,
	EXT_LUA, // allowed even if not HAVE_BLUA so that we can yell on load attempt
	NUM_EXT,
	NUM_EXT_TABLE = NUM_EXT-EXT_START,
	EXT_LOADED = 0x80
	/*
	obviously there can only be 0x7F supported extensions in
	addons menu because we're cramming this into a char out of
	laziness/easy memory allocation (what's the difference?)
	and have stolen a bit to show whether it's loaded or not
	in practice the size of the data type is probably overkill
	toast 02/05/17
	*/
} ext_enum;

typedef enum
{
	DIR_TYPE = 0,
	DIR_LEN,
	DIR_STRING
} dirname_enum;

typedef enum
{
	REFRESHDIR_NORMAL = 1,
	REFRESHDIR_ADDFILE = 2,
	REFRESHDIR_WARNING = 4,
	REFRESHDIR_ERROR = 8,
	REFRESHDIR_MAX = 16
} refreshdir_enum;

boolean preparefilemenu(boolean samemenu);

#endif // __FILESRCH_H__

/// \file
/// \brief Support to find files
///
///
///	 filesearch:
///
///	 ATTENTION : make sure there is enouth space in filename to put a full path (255 or 512)
///	 if needmd5check == 0 there is no md5 check
///	 if completepath then filename will be change with the full path and name
///	 maxsearchdepth == 0 only search given directory, no subdirs
///	 return FS_NOTFOUND
///	        FS_MD5SUMBAD;
///	        FS_FOUND

#include <stdio.h>
#ifdef __GNUC__
#include <dirent.h>
#endif
#ifdef _WIN32
//#define WIN32_LEAN_AND_MEAN
#define RPC_NO_WINDOWS_H
#include <windows.h>
#endif
#include <sys/stat.h>
#include <string.h>

#include "filesrch.h"
#include "d_netfil.h"
#include "m_misc.h"
#include "z_zone.h"
#include "m_menu.h" // Addons_option_Onchange
#include "w_wad.h"

#if defined (_WIN32) && defined (_MSC_VER)

#include <errno.h>
#include <io.h>
#include <tchar.h>

#define SUFFIX	"*"
#define	SLASH	"\\"
#define	S_ISDIR(m)	(((m) & S_IFMT) == S_IFDIR)

#ifndef INVALID_FILE_ATTRIBUTES
#define INVALID_FILE_ATTRIBUTES	((DWORD)-1)
#endif

struct dirent
{
	long		d_ino;		/* Always zero. */
	unsigned short	d_reclen;	/* Always zero. */
	unsigned short	d_namlen;	/* Length of name in d_name. */
	char		d_name[FILENAME_MAX]; /* File name. */
};

/*
 * This is an internal data structure. Good programmers will not use it
 * except as an argument to one of the functions below.
 * dd_stat field is now int (was short in older versions).
 */
typedef struct
{
	/* disk transfer area for this dir */
	struct _finddata_t	dd_dta;

	/* dirent struct to return from dir (NOTE: this makes this thread
	 * safe as long as only one thread uses a particular DIR struct at
	 * a time) */
	struct dirent		dd_dir;

	/* _findnext handle */
#if _MSC_VER > 1200
	intptr_t    dd_handle;
#else
	long        dd_handle;
#endif

	/*
	 * Status of search:
	 *   0 = not started yet (next entry to read is first entry)
	 *  -1 = off the end
	 *   positive = 0 based index of next entry
	 */
	int			dd_stat;

	/* given path for dir with search pattern (struct is extended) */
	CHAR			dd_name[1];
} DIR;

/*
 * opendir
 *
 * Returns a pointer to a DIR structure appropriately filled in to begin
 * searching a directory.
 */

DIR *
opendir (const CHAR *szPath)
{
  DIR *nd;
  DWORD rc;
  CHAR szFullPath[MAX_PATH];

  errno = 0;

  if (!szPath)
    {
      errno = EFAULT;
      return (DIR *) 0;
    }

  if (szPath[0] == '\0')
    {
      errno = ENOTDIR;
      return (DIR *) 0;
    }

  /* Attempt to determine if the given path really is a directory. */
  rc = GetFileAttributesA(szPath);
  if (rc == INVALID_FILE_ATTRIBUTES)
    {
      /* call GetLastError for more error info */
      errno = ENOENT;
      return (DIR *) 0;
    }
  if (!(rc & FILE_ATTRIBUTE_DIRECTORY))
    {
      /* Error, entry exists but not a directory. */
      errno = ENOTDIR;
      return (DIR *) 0;
    }

  /* Make an absolute pathname.  */
  _fullpath (szFullPath, szPath, MAX_PATH);

  /* Allocate enough space to store DIR structure and the complete
   * directory path given. */
  nd = (DIR *) malloc (sizeof (DIR) + (strlen(szFullPath) + strlen (SLASH) +
			 strlen(SUFFIX) + 1) * sizeof (CHAR));

  if (!nd)
    {
      /* Error, out of memory. */
      errno = ENOMEM;
      return (DIR *) 0;
    }

  /* Create the search expression. */
  strcpy (nd->dd_name, szFullPath);

  /* Add on a slash if the path does not end with one. */
  if (nd->dd_name[0] != '\0' &&
      nd->dd_name[strlen (nd->dd_name) - 1] != '/' &&
      nd->dd_name[strlen (nd->dd_name) - 1] != '\\')
    {
      strcat (nd->dd_name, SLASH);
    }

  /* Add on the search pattern */
  strcat (nd->dd_name, SUFFIX);

  /* Initialize handle to -1 so that a premature closedir doesn't try
   * to call _findclose on it. */
  nd->dd_handle = -1;

  /* Initialize the status. */
  nd->dd_stat = 0;

  /* Initialize the dirent structure. ino and reclen are invalid under
   * Win32, and name simply points at the appropriate part of the
   * findfirst_t structure. */
  nd->dd_dir.d_ino = 0;
  nd->dd_dir.d_reclen = 0;
  nd->dd_dir.d_namlen = 0;
  ZeroMemory(nd->dd_dir.d_name, FILENAME_MAX);

  return nd;
}

/*
 * readdir
 *
 * Return a pointer to a dirent structure filled with the information on the
 * next entry in the directory.
 */
struct dirent *
readdir (DIR * dirp)
{
  errno = 0;

  /* Check for valid DIR struct. */
  if (!dirp)
    {
      errno = EFAULT;
      return (struct dirent *) 0;
    }

  if (dirp->dd_stat < 0)
    {
      /* We have already returned all files in the directory
       * (or the structure has an invalid dd_stat). */
      return (struct dirent *) 0;
    }
  else if (dirp->dd_stat == 0)
    {
      /* We haven't started the search yet. */
      /* Start the search */
      dirp->dd_handle = _findfirst (dirp->dd_name, &(dirp->dd_dta));

	  if (dirp->dd_handle == -1)
	{
	  /* Whoops! Seems there are no files in that
	   * directory. */
	  dirp->dd_stat = -1;
	}
      else
	{
	  dirp->dd_stat = 1;
	}
    }
  else
    {
      /* Get the next search entry. */
      if (_findnext (dirp->dd_handle, &(dirp->dd_dta)))
	{
	  /* We are off the end or otherwise error.
	     _findnext sets errno to ENOENT if no more file
	     Undo this. */
	  DWORD winerr = GetLastError();
	  if (winerr == ERROR_NO_MORE_FILES)
	    errno = 0;
	  _findclose (dirp->dd_handle);
	  dirp->dd_handle = -1;
	  dirp->dd_stat = -1;
	}
      else
	{
	  /* Update the status to indicate the correct
	   * number. */
	  dirp->dd_stat++;
	}
    }

  if (dirp->dd_stat > 0)
    {
      /* Successfully got an entry. Everything about the file is
       * already appropriately filled in except the length of the
       * file name. */
      dirp->dd_dir.d_namlen = (unsigned short)strlen (dirp->dd_dta.name);
      strcpy (dirp->dd_dir.d_name, dirp->dd_dta.name);
      return &dirp->dd_dir;
    }

  return (struct dirent *) 0;
}

/*
 * rewinddir
 *
 * Makes the next readdir start from the beginning.
 */
int
rewinddir (DIR * dirp)
{
  errno = 0;

  /* Check for valid DIR struct. */
  if (!dirp)
    {
      errno = EFAULT;
      return -1;
    }

  dirp->dd_stat = 0;

  return 0;
}

/*
 * closedir
 *
 * Frees up resources allocated by opendir.
 */
int
closedir (DIR * dirp)
{
  int rc;

  errno = 0;
  rc = 0;

  if (!dirp)
    {
      errno = EFAULT;
      return -1;
    }

  if (dirp->dd_handle != -1)
    {
      rc = _findclose (dirp->dd_handle);
    }

  /* Delete the dir structure. */
  free (dirp);

  return rc;
}
#endif

static CV_PossibleValue_t addons_cons_t[] = {{0, "Default"},
#if 1
												{1, "HOME"}, {2, "SRB2"},
#endif
													{3, "CUSTOM"}, {0, NULL}};

consvar_t cv_addons_option = CVAR_INIT ("addons_option", "Default", CV_SAVE|CV_CALL, addons_cons_t, Addons_option_Onchange);
consvar_t cv_addons_folder = CVAR_INIT ("addons_folder", "", CV_SAVE, NULL, NULL);

static CV_PossibleValue_t addons_md5_cons_t[] = {{0, "Name"}, {1, "Contents"}, {0, NULL}};
consvar_t cv_addons_md5 = CVAR_INIT ("addons_md5", "Name", CV_SAVE, addons_md5_cons_t, NULL);

consvar_t cv_addons_showall = CVAR_INIT ("addons_showall", "No", CV_SAVE, CV_YesNo, NULL);

consvar_t cv_addons_search_case = CVAR_INIT ("addons_search_case", "No", CV_SAVE, CV_YesNo, NULL);

static CV_PossibleValue_t addons_search_type_cons_t[] = {{0, "Start"}, {1, "Anywhere"}, {0, NULL}};
consvar_t cv_addons_search_type = CVAR_INIT ("addons_search_type", "Anywhere", CV_SAVE, addons_search_type_cons_t, NULL);

char menupath[1024];
size_t menupathindex[menudepth];
size_t menudepthleft = menudepth;

char menusearch[MAXSTRINGLENGTH+1];

char **dirmenu, **coredirmenu; // core only local for this file
size_t sizedirmenu, sizecoredirmenu; // ditto
size_t dir_on[menudepth];
UINT8 refreshdirmenu = 0;
char *refreshdirname = NULL;

size_t packetsizetally = 0;
size_t mainwadstally = 0;

#define folderpathlen 1024
#define maxfolderdepth 48

#define isuptree(dirent) ((dirent)[0]=='.' && ((dirent)[1]=='\0' || ((dirent)[1]=='.' && (dirent)[2]=='\0')))

filestatus_t filesearch(char *filename, const char *startpath, const UINT8 *wantedmd5sum, boolean completepath, int maxsearchdepth)
{
	filestatus_t retval = FS_NOTFOUND;
	DIR **dirhandle;
	struct dirent *dent;
	struct stat fsstat;
	int found = 0;
	char *searchname = strdup(filename);
	int depthleft = maxsearchdepth;
	char searchpath[1024];
	size_t *searchpathindex;

	dirhandle = (DIR**) malloc(maxsearchdepth * sizeof (DIR*));
	searchpathindex = (size_t *) malloc(maxsearchdepth * sizeof (size_t));

	strcpy(searchpath,startpath);
	searchpathindex[--depthleft] = strlen(searchpath) + 1;

	dirhandle[depthleft] = opendir(searchpath);

	if (dirhandle[depthleft] == NULL)
	{
		free(searchname);
		free(dirhandle);
		free(searchpathindex);
		return FS_NOTFOUND;
	}

	if (searchpath[searchpathindex[depthleft]-2] != PATHSEP[0])
	{
		searchpath[searchpathindex[depthleft]-1] = PATHSEP[0];
		searchpath[searchpathindex[depthleft]] = 0;
	}
	else
		searchpathindex[depthleft]--;

	while ((!found) && (depthleft < maxsearchdepth))
	{
		searchpath[searchpathindex[depthleft]]=0;
		dent = readdir(dirhandle[depthleft]);

		if (!dent)
		{
			closedir(dirhandle[depthleft++]);
			continue;
		}

		if (isuptree(dent->d_name))
		{
			// we don't want to scan uptree
			continue;
		}

		// okay, now we actually want searchpath to incorporate d_name
		strcpy(&searchpath[searchpathindex[depthleft]],dent->d_name);

		if (stat(searchpath,&fsstat) < 0) // do we want to follow symlinks? if not: change it to lstat
			; // was the file (re)moved? can't stat it
		else if (S_ISDIR(fsstat.st_mode) && depthleft)
		{
			searchpathindex[--depthleft] = strlen(searchpath) + 1;
			dirhandle[depthleft] = opendir(searchpath);
			if (!dirhandle[depthleft])
			{
					// can't open it... maybe no read-permissions
					// go back to previous dir
					depthleft++;
			}

			searchpath[searchpathindex[depthleft]-1]=PATHSEP[0];
			searchpath[searchpathindex[depthleft]]=0;
		}
		else if (!strcasecmp(searchname, dent->d_name))
		{
			switch (checkfilemd5(searchpath, wantedmd5sum))
			{
				case FS_FOUND:
					if (completepath)
						strcpy(filename,searchpath);
					else
						strcpy(filename,dent->d_name);
					retval = FS_FOUND;
					found = 1;
					break;
				case FS_MD5SUMBAD:
					retval = FS_MD5SUMBAD;
					break;
				default: // prevent some compiler warnings
					break;
			}
		}
	}

	for (; depthleft < maxsearchdepth; closedir(dirhandle[depthleft++]));

	free(searchname);
	free(searchpathindex);
	free(dirhandle);

	return retval;
}

// Called from findfolder and ResGetLumpsFolder in w_wad.c.
// Call with cleanup true if the path has to be verified.
boolean checkfolderpath(const char *path, const char *startpath, boolean cleanup)
{
	char folderpath[folderpathlen], basepath[folderpathlen], *fn = NULL;
	DIR *dirhandle;

	// Remove path separators from the filename, and don't try adding "/".
	// See also the same code in W_InitFolder.
	if (cleanup)
	{
		const char *p = path + strlen(path);
		size_t len;

		--p;
		while (*p == '\\' || *p == '/' || *p == ':')
		{
			p--;
			if (p < path)
				return false;
		}
		++p;

		// Allocate the new path name.
		len = (p - path) + 1;
		fn = ZZ_Alloc(len);
		strlcpy(fn, path, len);
	}

	if (startpath)
	{
		snprintf(basepath, sizeof basepath, "%s" PATHSEP, startpath);

		if (cleanup)
		{
			snprintf(folderpath, sizeof folderpath, "%s%s", basepath, fn);
			Z_Free(fn); // Don't need this anymore.
		}
		else
			snprintf(folderpath, sizeof folderpath, "%s%s", basepath, path);

		// Home path and folder path are the same? Not valid.
		if (!strcmp(basepath, folderpath))
			return false;
	}
	else if (cleanup)
	{
		snprintf(folderpath, sizeof folderpath, "%s", fn);
		Z_Free(fn); // Don't need this anymore.
	}
	else
		snprintf(folderpath, sizeof folderpath, "%s", path);

	dirhandle = opendir(folderpath);
	if (dirhandle == NULL)
		return false;
	else
		closedir(dirhandle);

	return true;
}

INT32 pathisfolder(const char *path)
{
	struct stat fsstat;

	if (stat(path, &fsstat) < 0)
		return -1;
	else if (S_ISDIR(fsstat.st_mode))
		return 1;

	return 0;
}

INT32 samepaths(const char *path1, const char *path2)
{
	struct stat stat1;
	struct stat stat2;

	if (stat(path1, &stat1) < 0)
		return -1;
	if (stat(path2, &stat2) < 0)
		return -1;

	if (stat1.st_dev == stat2.st_dev)
	{
#if !defined(_WIN32)
		return (stat1.st_ino == stat2.st_ino);
#else
		HANDLE file1 = CreateFileA(path1, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
		HANDLE file2 = CreateFileA(path2, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
		BY_HANDLE_FILE_INFORMATION file1info, file2info;
		boolean ok = false;

		if (file1 != INVALID_HANDLE_VALUE && file2 != INVALID_HANDLE_VALUE)
		{
			if (GetFileInformationByHandle(file1, &file1info) && GetFileInformationByHandle(file2, &file2info))
			{
				if (file1info.dwVolumeSerialNumber == file2info.dwVolumeSerialNumber
				&& file1info.nFileIndexLow == file2info.nFileIndexLow
				&& file1info.nFileIndexHigh == file2info.nFileIndexHigh)
					ok = true;
			}
		}

		if (file1 != INVALID_HANDLE_VALUE)
			CloseHandle(file1);
		if (file2 != INVALID_HANDLE_VALUE)
			CloseHandle(file2);

		return ok;
#endif
	}

	return false;
}

//
// Folder loading
//

static void initfolderpath(char *folderpath, size_t *folderpathindex, int depthleft)
{
	folderpathindex[depthleft] = strlen(folderpath) + 1;

	if (folderpath[folderpathindex[depthleft]-2] != PATHSEP[0])
	{
		folderpath[folderpathindex[depthleft]-1] = PATHSEP[0];
		folderpath[folderpathindex[depthleft]] = 0;
	}
	else
		folderpathindex[depthleft]--;
}

lumpinfo_t *getfolderfiles(const char *path, UINT16 *nlmp, UINT16 *nfiles, UINT16 *nfolders)
{
	DIR **dirhandle;
	struct dirent *dent;
	struct stat fsstat;

	int rootfolder = (maxfolderdepth - 1);
	int depthleft = rootfolder;

	char folderpath[folderpathlen];
	size_t *folderpathindex;

	lumpinfo_t *lumpinfo, *lump_p;
	UINT16 i = 0, numlumps = (*nlmp);

	dirhandle = (DIR **)malloc(maxfolderdepth * sizeof (DIR*));
	folderpathindex = (size_t *)malloc(maxfolderdepth * sizeof(size_t));

	// Open the root directory
	strlcpy(folderpath, path, folderpathlen);
	dirhandle[depthleft] = opendir(folderpath);

	if (dirhandle[depthleft] == NULL)
	{
		free(dirhandle);
		free(folderpathindex);
		return NULL;
	}

	initfolderpath(folderpath, folderpathindex, depthleft);
	(*nfiles) = 0;
	(*nfolders) = 0;

	// Count files and directories
	while (depthleft < maxfolderdepth)
	{
		folderpath[folderpathindex[depthleft]] = 0;
		dent = readdir(dirhandle[depthleft]);

		if (!dent)
		{
			if (depthleft != rootfolder) // Don't close the root directory
				closedir(dirhandle[depthleft]);
			depthleft++;
			continue;
		}
		else if (isuptree(dent->d_name))
			continue;

		strcpy(&folderpath[folderpathindex[depthleft]], dent->d_name);

		if (stat(folderpath, &fsstat) < 0)
			;
		else if (S_ISDIR(fsstat.st_mode) && depthleft)
		{
			folderpathindex[--depthleft] = strlen(folderpath) + 1;
			dirhandle[depthleft] = opendir(folderpath);

			if (dirhandle[depthleft])
			{
				numlumps++;
				(*nfolders)++;
			}
			else
				depthleft++;

			folderpath[folderpathindex[depthleft]-1] = '/';
			folderpath[folderpathindex[depthleft]] = 0;
		}
		else
		{
			numlumps++;
			(*nfiles)++;
		}

		if (numlumps == (UINT16_MAX-1))
			break;
	}

	// Failure: No files have been found.
	if (!(*nfiles))
	{
		(*nfiles) = UINT16_MAX;
		free(folderpathindex);
		free(dirhandle);
		for (; depthleft < maxfolderdepth; closedir(dirhandle[depthleft++])); // Close any open directories.
		return NULL;
	}

	// Create the files and directories as lump entries
	// It's possible to create lumps and count files at the same time,
	// but I didn't to constantly have to reallocate memory for every lump.
	rewinddir(dirhandle[rootfolder]);
	depthleft = rootfolder;

	strlcpy(folderpath, path, folderpathlen);
	initfolderpath(folderpath, folderpathindex, depthleft);

	lump_p = lumpinfo = Z_Calloc(numlumps * sizeof(lumpinfo_t), PU_STATIC, NULL);

	while (depthleft < maxfolderdepth)
	{
		char *fullname, *trimname;

		folderpath[folderpathindex[depthleft]] = 0;
		dent = readdir(dirhandle[depthleft]);

		if (!dent)
		{
			closedir(dirhandle[depthleft++]);
			continue;
		}
		else if (isuptree(dent->d_name))
			continue;

		strcpy(&folderpath[folderpathindex[depthleft]], dent->d_name);

		if (stat(folderpath, &fsstat) < 0)
			continue;
		else if (S_ISDIR(fsstat.st_mode) && depthleft)
		{
			folderpathindex[--depthleft] = strlen(folderpath) + 1;
			dirhandle[depthleft] = opendir(folderpath);

			if (!dirhandle[depthleft])
			{
				depthleft++;
				continue;
			}

			folderpath[folderpathindex[depthleft]-1] = '/';
			folderpath[folderpathindex[depthleft]] = 0;
		}

		lump_p->diskpath = Z_StrDup(folderpath); // Path in the filesystem to the file
		lump_p->compression = CM_NOCOMPRESSION; // Lump is uncompressed

		// Remove the folder path.
		fullname = lump_p->diskpath;
		if (strstr(fullname, path))
			fullname += strlen(path) + 1;

		// Get the 8-character long lump name.
		trimname = strrchr(fullname, '/');
		if (trimname)
			trimname++;
		else
			trimname = fullname;

		if (trimname[0])
		{
			char *dotpos = strrchr(trimname, '.');
			if (dotpos == NULL)
				dotpos = fullname + strlen(fullname);

			strncpy(lump_p->name, trimname, min(8, dotpos - trimname));

			// The name of the file, without the extension.
			lump_p->longname = Z_Calloc(dotpos - trimname + 1, PU_STATIC, NULL);
			strlcpy(lump_p->longname, trimname, dotpos - trimname + 1);
		}
		else
			lump_p->longname = Z_Calloc(1, PU_STATIC, NULL);

		// The complete name of the file, with its extension,
		// excluding the path of the folder where it resides.
		lump_p->fullname = Z_StrDup(fullname);

		lump_p++;
		i++;

		if (i > numlumps || i == (UINT16_MAX-1))
		{
			for (; depthleft < maxfolderdepth; closedir(dirhandle[depthleft++])); // Close any open directories.
			break;
		}
	}

	free(folderpathindex);
	free(dirhandle);

	(*nlmp) = numlumps;
	return lumpinfo;
}

//
// Addons menu
//

char exttable[NUM_EXT_TABLE][7] = { // maximum extension length (currently 4) plus 3 (null terminator, stop, and length including previous two)
	"\5.txt", "\5.cfg", // exec
	"\5.wad",
#ifdef USE_KART
	"\6.kart",
#endif
	"\5.pk3", "\5.soc", "\5.lua"}; // addfile

char filenamebuf[MAX_WADFILES][MAX_WADPATH];

static boolean filemenucmp(char *haystack, char *needle)
{
	static char localhaystack[128];
	strlcpy(localhaystack, haystack, 128);
	if (!cv_addons_search_case.value)
		strupr(localhaystack);
	if (cv_addons_search_type.value)
		return (strstr(localhaystack, needle) != 0);
	return (!strncmp(localhaystack, needle, menusearch[0]));
}

void closefilemenu(boolean validsize)
{
	// search
	if (dirmenu)
	{
		if (dirmenu != coredirmenu)
		{
			if (dirmenu[0] && ((UINT8)(dirmenu[0][DIR_TYPE]) == EXT_NORESULTS))
			{
				Z_Free(dirmenu[0]);
				dirmenu[0] = NULL;
			}
			Z_Free(dirmenu);
		}
		dirmenu = NULL;
		sizedirmenu = 0;
	}

	if (coredirmenu)
	{
		// core
		if (validsize)
		{
			for (; sizecoredirmenu > 0; sizecoredirmenu--)
			{
				Z_Free(coredirmenu[sizecoredirmenu-1]);
				coredirmenu[sizecoredirmenu-1] = NULL;
			}
		}
		else
			sizecoredirmenu = 0;

		Z_Free(coredirmenu);
		coredirmenu = NULL;
	}

	if (refreshdirname)
		Z_Free(refreshdirname);
	refreshdirname = NULL;
}

void searchfilemenu(char *tempname)
{
	size_t i, first;
	char localmenusearch[MAXSTRINGLENGTH] = "";

	if (dirmenu)
	{
		if (dirmenu != coredirmenu)
		{
			if (dirmenu[0] && ((UINT8)(dirmenu[0][DIR_TYPE]) == EXT_NORESULTS))
			{
				Z_Free(dirmenu[0]);
				dirmenu[0] = NULL;
			}
			//Z_Free(dirmenu); -- Z_Realloc later tho...
		}
		else
			dirmenu = NULL;
	}

	first = (((UINT8)(coredirmenu[0][DIR_TYPE]) == EXT_UP) ? 1 : 0); // skip UP...

	if (!menusearch[0])
	{
		if (dirmenu)
			Z_Free(dirmenu);
		dirmenu = coredirmenu;
		sizedirmenu = sizecoredirmenu;

		if (tempname)
		{
			for (i = first; i < sizedirmenu; i++)
			{
				if (!strcmp(dirmenu[i]+DIR_STRING, tempname))
				{
					dir_on[menudepthleft] = i;
					break;
				}
			}

			if (i == sizedirmenu)
				dir_on[menudepthleft] = first;

			Z_Free(tempname);
		}

		return;
	}

	strcpy(localmenusearch, menusearch+1);
	if (!cv_addons_search_case.value)
		strupr(localmenusearch);

	sizedirmenu = 0;
	for (i = first; i < sizecoredirmenu; i++)
	{
		if (filemenucmp(coredirmenu[i]+DIR_STRING, localmenusearch))
			sizedirmenu++;
	}

	if (!sizedirmenu) // no results...
	{
		if ((!(dirmenu = Z_Realloc(dirmenu, sizeof(char *), PU_STATIC, NULL)))
			|| !(dirmenu[0] = Z_StrDup(va("%c\13No results...", EXT_NORESULTS))))
				I_Error("searchfilemenu(): could not create \"No results...\".");
		sizedirmenu = 1;
		dir_on[menudepthleft] = 0;
		if (tempname)
			Z_Free(tempname);
		return;
	}

	if (!(dirmenu = Z_Realloc(dirmenu, sizedirmenu*sizeof(char *), PU_STATIC, NULL)))
		I_Error("searchfilemenu(): could not reallocate dirmenu.");

	sizedirmenu = 0;
	for (i = first; i < sizecoredirmenu; i++)
	{
		if (filemenucmp(coredirmenu[i]+DIR_STRING, localmenusearch))
		{
			if (tempname && !strcmp(coredirmenu[i]+DIR_STRING, tempname))
			{
				dir_on[menudepthleft] = sizedirmenu;
				Z_Free(tempname);
				tempname = NULL;
			}
			dirmenu[sizedirmenu++] = coredirmenu[i]; // pointer reuse
		}
	}

	if (tempname)
	{
		dir_on[menudepthleft] = 0; //first; -- can't be first, causes problems
		Z_Free(tempname);
	}
}

boolean preparefilemenu(boolean samedepth)
{
	DIR *dirhandle;
	struct dirent *dent;
	struct stat fsstat;
	size_t pos = 0, folderpos = 0, numfolders = 0;
	char *tempname = NULL;

	if (samedepth)
	{
		if (dirmenu && dirmenu[dir_on[menudepthleft]])
			tempname = Z_StrDup(dirmenu[dir_on[menudepthleft]]+DIR_STRING); // don't need to I_Error if can't make - not important, just QoL
	}
	else
		menusearch[0] = menusearch[1] = 0; // clear search

	if (!(dirhandle = opendir(menupath))) // get directory
	{
		closefilemenu(true);
		return false;
	}

	for (; sizecoredirmenu > 0; sizecoredirmenu--) // clear out existing items
	{
		Z_Free(coredirmenu[sizecoredirmenu-1]);
		coredirmenu[sizecoredirmenu-1] = NULL;
	}

	while (true)
	{
		menupath[menupathindex[menudepthleft]] = 0;
		dent = readdir(dirhandle);

		if (!dent)
			break;
		else if (isuptree(dent->d_name))
			continue; // we don't want to scan uptree

		strcpy(&menupath[menupathindex[menudepthleft]],dent->d_name);

		if (stat(menupath,&fsstat) < 0) // do we want to follow symlinks? if not: change it to lstat
			; // was the file (re)moved? can't stat it
		else // is a file or directory
		{
			if (!S_ISDIR(fsstat.st_mode)) // file
			{
				if (!cv_addons_showall.value)
				{
					size_t len = strlen(dent->d_name)+1;
					UINT8 ext;
					for (ext = 0; ext < NUM_EXT_TABLE; ext++)
						if (!strcasecmp(exttable[ext]+1, dent->d_name+len-(exttable[ext][0]))) break; // extension comparison
					if (ext == NUM_EXT_TABLE) continue; // not an addfile-able (or exec-able) file
				}
			}
			else // directory
				numfolders++;

			sizecoredirmenu++;
		}
	}

	if (!sizecoredirmenu)
	{
		closedir(dirhandle);
		closefilemenu(false);
		if (tempname)
			Z_Free(tempname);
		return false;
	}

	if (menudepthleft != menudepth-1) // Make room for UP...
	{
		sizecoredirmenu++;
		numfolders++;
		folderpos++;
	}

	if (dirmenu && dirmenu == coredirmenu)
		dirmenu = NULL;

	if (!(coredirmenu = Z_Realloc(coredirmenu, sizecoredirmenu*sizeof(char *), PU_STATIC, NULL)))
	{
		closedir(dirhandle); // just in case
		I_Error("preparefilemenu(): could not reallocate coredirmenu.");
	}

	rewinddir(dirhandle);

	while ((pos+folderpos) < sizecoredirmenu)
	{
		menupath[menupathindex[menudepthleft]] = 0;
		dent = readdir(dirhandle);

		if (!dent)
			break;
		else if (isuptree(dent->d_name))
			continue; // we don't want to scan uptree

		strcpy(&menupath[menupathindex[menudepthleft]],dent->d_name);

		if (stat(menupath,&fsstat) < 0) // do we want to follow symlinks? if not: change it to lstat
			; // was the file (re)moved? can't stat it
		else // is a file or directory
		{
			char *temp;
			size_t len = strlen(dent->d_name)+1;
			UINT8 ext = EXT_FOLDER;
			UINT8 folder;

			if (!S_ISDIR(fsstat.st_mode)) // file
			{
				if (!((numfolders+pos) < sizecoredirmenu)) continue; // crash prevention
				for (; ext < NUM_EXT_TABLE; ext++)
					if (!strcasecmp(exttable[ext]+1, dent->d_name+len-(exttable[ext][0]))) break; // extension comparison
				if (ext == NUM_EXT_TABLE && !cv_addons_showall.value) continue; // not an addfile-able (or exec-able) file
				ext += EXT_START; // moving to be appropriate position

				if (ext >= EXT_LOADSTART)
				{
					size_t i;
					for (i = 0; i < numwadfiles; i++)
					{
						if (!filenamebuf[i][0])
						{
							strncpy(filenamebuf[i], wadfiles[i]->filename, MAX_WADPATH);
							filenamebuf[i][MAX_WADPATH - 1] = '\0';
							nameonly(filenamebuf[i]);
						}

						if (strcmp(dent->d_name, filenamebuf[i]))
							continue;
						if (cv_addons_md5.value && !checkfilemd5(menupath, wadfiles[i]->md5sum))
							continue;

						ext |= EXT_LOADED;
					}
				}
				else if (ext == EXT_TXT)
				{
					if (!strncmp(dent->d_name, "log-", 4) || !strcmp(dent->d_name, "errorlog.txt"))
						ext |= EXT_LOADED;
				}

				if (!strcmp(dent->d_name, configfile))
					ext |= EXT_LOADED;

				folder = 0;
			}
			else // directory
				len += (folder = 1);

			if (len > 255)
				len = 255;

			if (!(temp = Z_Malloc((len+DIR_STRING+folder) * sizeof (char), PU_STATIC, NULL)))
				I_Error("preparefilemenu(): could not create file entry.");
			temp[DIR_TYPE] = ext;
			temp[DIR_LEN] = (UINT8)(len);
			strlcpy(temp+DIR_STRING, dent->d_name, len);
			if (folder)
			{
				strcpy(temp+len, PATHSEP);
				coredirmenu[folderpos++] = temp;
			}
			else
				coredirmenu[numfolders + pos++] = temp;
		}
	}

	closedir(dirhandle);

	if ((menudepthleft != menudepth-1) // now for UP... entry
		&& !(coredirmenu[0] = Z_StrDup(va("%c\5UP...", EXT_UP))))
			I_Error("preparefilemenu(): could not create \"UP...\".");

	menupath[menupathindex[menudepthleft]] = 0;
	sizecoredirmenu = (numfolders+pos); // just in case things shrink between opening and rewind

	if (!sizecoredirmenu)
	{
		dir_on[menudepthleft] = 0;
		closefilemenu(false);
		return false;
	}

	searchfilemenu(tempname);

	return true;
}

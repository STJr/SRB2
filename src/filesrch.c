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
#if defined (_WIN32) && !defined (_XBOX)
//#define WIN32_LEAN_AND_MEAN
#define RPC_NO_WINDOWS_H
#include <windows.h>
#endif
#ifdef _WIN32_WCE
#include "sdl/SRB2CE/cehelp.h"
#else
#include <sys/stat.h>
#endif
#include <string.h>

#include "filesrch.h"
#include "d_netfil.h"
#include "m_misc.h"
#include "z_zone.h"

#if (defined (_WIN32) && !defined (_WIN32_WCE)) && defined (_MSC_VER) && !defined (_XBOX)

#include <errno.h>
#include <io.h>
#include <tchar.h>

#define SUFFIX	"*"
#define	SLASH	PATHSEP
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

char menupath[1024];
size_t menupathindex[20];
size_t menudepthleft = 20;

char **dirmenu;
size_t sizedirmenu;
size_t dir_on;

#if defined (_XBOX) && defined (_MSC_VER)
filestatus_t filesearch(char *filename, const char *startpath, const UINT8 *wantedmd5sum,
	boolean completepath, int maxsearchdepth)
{
//NONE?
	startpath = filename = NULL;
	wantedmd5sum = NULL;
	maxsearchdepth = 0;
	completepath = false;
	return FS_NOTFOUND;
}

boolean preparefilemenu(void)
{
	return false;
}

#elif defined (_WIN32_WCE)
filestatus_t filesearch(char *filename, const char *startpath, const UINT8 *wantedmd5sum,
	boolean completepath, int maxsearchdepth)
{
#ifdef __GNUC__
//NONE?
	startpath = filename = NULL;
	wantedmd5sum = NULL;
	maxsearchdepth = 0;
	completepath = false;
#else
	WIN32_FIND_DATA dta;
	HANDLE searchhandle = INVALID_HANDLE_VALUE;
	const wchar_t wm[4] = L"*.*";

	//if (startpath) SetCurrentDirectory(startpath);
	if (FIL_ReadFileOK(filename))
	{
		// checkfilemd5 returns an FS_* value, either FS_FOUND or FS_MD5SUMBAD
		return checkfilemd5(filename, wantedmd5sum);
	}
	ZeroMemory(&dta,sizeof (dta));
	if (maxsearchdepth)
		searchhandle = FindFirstFile(wm,&dta);
	if (searchhandle != INVALID_HANDLE_VALUE)
	{
		do
		{
			if ((dta.cFileName[0]!='.') && (dta.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				//if (SetCurrentDirectory(dta.cFileName))
				{ // can fail if we haven't the right
					filestatus_t found;
					found = filesearch(filename,NULL,wantedmd5sum,completepath,maxsearchdepth-1);
					//SetCurrentDirectory("..");
					if (found == FS_FOUND || found == FS_MD5SUMBAD)
					{
						if (completepath)
							strcatbf(filename,(char *)dta.cFileName,"\\");
						FindClose(searchhandle);
						return found;
					}
				}
			}
		} while (FindNextFile(searchhandle,&dta)==0);
		FindClose(searchhandle);
	}
#endif
	return FS_NOTFOUND;
}

boolean preparefilemenu(void)
{
	return false;
}
#else
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

	if (searchpath[searchpathindex[depthleft]-2] != '/')
	{
		searchpath[searchpathindex[depthleft]-1] = '/';
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

		if (dent->d_name[0]=='.' &&
				(dent->d_name[1]=='\0' ||
					(dent->d_name[1]=='.' &&
						dent->d_name[2]=='\0')))
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

			searchpath[searchpathindex[depthleft]-1]='/';
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

#define MAXEXT 5
char ext[MAXEXT][5] = {
	".txt", ".cfg", // exec
	".wad", ".soc", ".lua"}; // addfile

boolean preparefilemenu(void)
{
	DIR *dirhandle;
	struct dirent *dent;
	struct stat fsstat;
	size_t pos, folderpos = 0, numfolders = 0;

	for (pos = 0; pos < sizedirmenu; pos++)
	{
		Z_Free(dirmenu[pos]);
		dirmenu[pos] = NULL;
	}

	sizedirmenu = dir_on = pos = 0;

	dirhandle = opendir(menupath);

	if (dirhandle == NULL)
		return false;

	while (true)
	{
		menupath[menupathindex[menudepthleft]] = 0;
		dent = readdir(dirhandle);

		if (!dent)
			break;
		else if (dent->d_name[0]=='.' &&
				(dent->d_name[1]=='\0' ||
					(dent->d_name[1]=='.' &&
						dent->d_name[2]=='\0')))
			continue; // we don't want to scan uptree

		strcpy(&menupath[menupathindex[menudepthleft]],dent->d_name);

		if (stat(menupath,&fsstat) < 0) // do we want to follow symlinks? if not: change it to lstat
			; // was the file (re)moved? can't stat it
		else // is a file or directory
		{
			if (!S_ISDIR(fsstat.st_mode))
			{
				size_t len = strlen(dent->d_name)+1;
				UINT8 i;
				for (i = 0; i < MAXEXT; i++)
					if (!strcasecmp(ext[i], dent->d_name+len-5)) break;
				if (i == MAXEXT) continue; // not an addfile-able (or exec-able) file
			}
			else
				numfolders++;
			sizedirmenu++;
		}
	}

	closedir(dirhandle); // I don't know how to go back to the start of the folder without just opening and closing... if there's a way, it doesn't appear to be easily manipulatable

	if (!sizedirmenu)
		return false;

	if (!(dirmenu = Z_Realloc(dirmenu, sizedirmenu*sizeof(char *), PU_STATIC, NULL)))
		I_Error("Ran out of memory whilst preparing add-ons menu");

	dirhandle = opendir(menupath);

	while ((pos+folderpos) < sizedirmenu)
	{
		menupath[menupathindex[menudepthleft]] = 0;
		dent = readdir(dirhandle);

		if (!dent)
			break;
		else if (dent->d_name[0]=='.' &&
				(dent->d_name[1]=='\0' ||
					(dent->d_name[1]=='.' &&
						dent->d_name[2]=='\0')))
			continue; // we don't want to scan uptree

		strcpy(&menupath[menupathindex[menudepthleft]],dent->d_name);

		if (stat(menupath,&fsstat) < 0) // do we want to follow symlinks? if not: change it to lstat
			; // was the file (re)moved? can't stat it
		else // is a file or directory
		{
			char *temp;
			size_t len = strlen(dent->d_name)+1;
			UINT8 i = 0;
			size_t folder;

			if (!S_ISDIR(fsstat.st_mode)) // file
			{
				for (; i < MAXEXT; i++)
					if (!strcasecmp(ext[i], dent->d_name+len-5)) break;
				if (i == MAXEXT) continue; // not an addfile-able (or exec-able) file
				i++; // i goes up so zero-index is directory instead of .txt
				folder = 0;
			}
			else
				len += (folder = 1);

			if (len > 255)
				len = 255;

			if (!(temp = Z_Malloc((len+2+folder) * sizeof (char), PU_STATIC, NULL)))
				I_Error("Ran out of memory whilst preparing add-ons menu");
			temp[0] = i;
			temp[1] = (UINT8)(len);
			strlcpy(temp+2, dent->d_name, len);
			if (folder)
			{
				strcpy(temp+len, "/");
				dirmenu[folderpos++] = temp;
			}
			else
				dirmenu[numfolders + pos++] = temp;
		}
	}

	menupath[menupathindex[menudepthleft]] = 0;
	sizedirmenu = (pos+folderpos); // crash prevention if things change between openings somehow

	closedir(dirhandle);
	return true;
}
#endif

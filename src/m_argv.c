// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2021 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  m_argv.c
/// \brief Command line arguments

#include <string.h>

#include "doomdef.h"
#include "command.h"
#include "m_argv.h"
#include "m_misc.h"

/**	\brief number of arg
*/
INT32 myargc;

/**	\brief string table
*/
char **myargv;

/** \brief did we alloc myargv ourselves?
*/
boolean myargmalloc = false;

/**	\brief founded the parm
*/
static INT32 found;

/**	\brief Parses a server URL (such as srb2://127.0.0.1) as may be passed to the game via a web browser, etc.

	\return the contents of the URL after the protocol (a server to join), or NULL if not found
*/
const char *M_GetUrlProtocolArg(void)
{
	INT32 i;
	const size_t len = strlen(SERVER_URL_PROTOCOL);

	for (i = 1; i < myargc; i++)
	{
		if (strlen(myargv[i]) > len && !strnicmp(myargv[i], SERVER_URL_PROTOCOL, len))
		{
			return &myargv[i][len];
		}
	}

	return NULL;
}

/**	\brief	The M_CheckParm function

	\param	check Checks for the given parameter in the program's command line arguments.

	\return	number (1 to argc-1) or 0 if not present


*/
INT32 M_CheckParm(const char *check)
{
	INT32 i;

	for (i = 1; i < myargc; i++)
	{
		if (!strcasecmp(check, myargv[i]))
		{
			found = i;
			return i;
		}
	}
	found = 0;
	return 0;
}

/**	\brief the M_IsNextParm

  \return  true if there is an available next parameter
*/

boolean M_IsNextParm(void)
{
	if (found > 0 && found + 1 < myargc && myargv[found+1][0] != '-' && myargv[found+1][0] != '+')
		return true;
	return false;
}

/**	\brief the M_GetNextParm function

  \return the next parameter after a M_CheckParm, NULL if not found
	use M_IsNextParm to verify there is a parameter
*/
const char *M_GetNextParm(void)
{
	if (M_IsNextParm())
	{
		found++;
		return myargv[found];
	}
	return NULL;
}

/**	\brief the  M_PushSpecialParameters function
	push all parameters beginning with '+'
*/
void M_PushSpecialParameters(void)
{
	INT32 i;
	for (i = 1; i < myargc; i++)
	{
		if (myargv[i][0] == '+')
		{
			COM_BufAddText(&myargv[i][1]);
			i++;

			// get the parameters of the command too
			for (; i < myargc && myargv[i][0] != '+' && myargv[i][0] != '-'; i++)
			{
				COM_BufAddText(va(" \"%s\"", myargv[i]));
			}

			// push it
			COM_BufAddText("\n");
			i--;
		}
	}
}

/// \brief max args
#define MAXARGVS 256

/**	\brief the M_FindResponseFile function
	Find a response file
*/
void M_FindResponseFile(void)
{
	INT32 i;

	for (i = 1; i < myargc; i++)
		if (myargv[i][0] == '@')
		{
			FILE *handle;
			INT32 k, pindex, indexinfile;
			long size;
			boolean inquote = false;
			UINT8 *infile;
			char *file;
			char *moreargs[20];
			char *firstargv;

			// read the response file into memory
			handle = fopen(&myargv[i][1], "rb");
			if (!handle)
				I_Error("Response file %s not found", &myargv[i][1]);

			CONS_Printf(M_GetText("Found response file %s\n"), &myargv[i][1]);
			fseek(handle, 0, SEEK_END);
			size = ftell(handle);
			fseek(handle, 0, SEEK_SET);
			file = malloc(size);
			if (!file)
				I_Error("No more free memory for the response file");
			if (fread(file, size, 1, handle) != 1)
				I_Error("Couldn't read response file because %s", M_FileError(handle));
			fclose(handle);

			// keep all the command line arguments following @responsefile
			for (pindex = 0, k = i+1; k < myargc; k++)
				moreargs[pindex++] = myargv[k];

			firstargv = myargv[0];
			myargv = malloc(sizeof (char *) * MAXARGVS);
			if (!myargv)
			{
				free(file);
				I_Error("Not enough memory to read response file");
			}
			myargmalloc = true; // mark as having been allocated by us
			memset(myargv, 0, sizeof (char *) * MAXARGVS);
			myargv[0] = firstargv;

			infile = (UINT8 *)file;
			indexinfile = k = 0;
			indexinfile++; // skip past argv[0]
			do
			{
				inquote = infile[k] == '"';
				if (inquote) // strip enclosing double-quote
					k++;
				myargv[indexinfile++] = (char *)&infile[k];
				while (k < size && ((inquote && infile[k] != '"')
					|| (!inquote && infile[k] > ' ')))
				{
					k++;
				}
				infile[k] = 0;
				while (k < size && (infile[k] <= ' '))
					k++;
			} while (k < size);

			for (k = 0; k < pindex; k++)
				myargv[indexinfile++] = moreargs[k];
			myargc = indexinfile;

			// display arguments
			CONS_Printf(M_GetText("%d command-line args:\n"), myargc-1); // -1 so @ don't actually get counted for
			for (k = 1; k < myargc; k++)
				CONS_Printf("%s\n", myargv[k]);

			break;
		}
}

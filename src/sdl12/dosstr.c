// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// This file is in the public domain.
// (Re)written by Graue in 2006.
//
//-----------------------------------------------------------------------------
/// \file
/// \brief String uppercasing/lowercasing functions for non-DOS non-Win32
///        systems

#include "../doomtype.h"

#ifndef HAVE_DOSSTR_FUNCS

#include <ctype.h>

int strupr(char *n)
{
	while (*n != '\0')
	{
		*n = toupper(*n);
		n++;
	}
	return 1;
}

int strlwr(char *n)
{
	while (*n != '\0')
	{
		*n = tolower(*n);
		n++;
	}
	return 1;
}

#endif

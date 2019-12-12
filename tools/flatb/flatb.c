#define HELP \
"Usage: flatb WAD-file list-file"                                         "\n"\
"Replace flats and textures by name in a DOOM WAD."                       "\n"\
"\n"\
"list-file may have the following format:"                                "\n"\
"\n"\
"GFZFLR01 GFZFLR02"                                                       "\n"\
"# Comment"                                                               "\n"\
"GFZROCK GFZBLOCK"                                                        "\n"\
"\n"\
"The first name and second name may be delimited by any whitespace."       "\n"\
"\n"\
"Copyright 2019 James R."                                                 "\n"\
"All rights reserved."                                                    "\n"

/*
Copyright 2019 James R.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#define cchar const char
#define cvoid const void

#define LONG int32_t

#define va_inline( __ap,__last, ... )\
(\
		va_start (__ap,__last),\
		__VA_ARGS__,\
		va_end   (__ap)\
)

#define DELIM "\t\n\r "

typedef struct
{
	FILE  *       fp;
	cchar * filename;
}
File;

int (*le32)(cvoid *);

void
Pexit (int c, cchar *s, ...)
{
	va_list ap;
	va_inline (ap, s,

			vfprintf(stderr, s, ap)

	);
	exit(c);
}

void
Prexit (cchar *pr, ...)
{
	va_list ap;
	va_inline (ap, pr,

			vfprintf(stderr, pr, ap)

	);
	perror("");
	exit(-1);
}

void
Fopen (File *f, cchar *filename, const char *mode)
{
	FILE *fp;
	if (!( fp = fopen(filename, mode) ))
		Prexit("%s", filename);
	f->filename = filename;
	f->fp = fp;
}

void
Ferr (File *f)
{
	if (ferror(f->fp))
		Prexit("%s", f->filename);
}

char *
Fgets (File *f, int b, char *p)
{
	if (!( p = fgets(p, b, f->fp) ))
		Ferr(f);
	return p;
}

void
Fread (File *f, int b, void *p)
{
	if (fread(p, 1, b, f->fp) < b)
		Ferr(f);
}

void
Fwrite (File *f, int b, cvoid *s)
{
	if (fwrite(s, 1, b, f->fp) < b)
		Ferr(f);
}

void
Fseek (File *f, long o)
{
	if (fseek(f->fp, o, SEEK_SET) == -1)
		Prexit("%s", f->filename);
}

void *
Malloc (int b)
{
	void *p;
	if (!( p = malloc(b) ))
		Prexit("%d", b);
	return p;
}

void *
Calloc (int c, int b)
{
	void *p;
	if (!( p = calloc(c, b) ))
		Prexit("(%d)%d", c, b);
	return p;
}

void
Reallocp (void *pp, int b)
{
	void *p;
	if (!( p = realloc((*(void **)pp), b) ))
		Prexit("%d", b);
	(*(void **)pp) = p;
}

void
strucpy (char *p, cchar *s, int n)
{
	int c;
	int i;
	for (i = 0; i < n && ( c = s[i] ); ++i)
		p[i] = toupper(c);
}

int
e32 (cvoid *s)
{
	unsigned int c;
	c = *(LONG *)s;
	return (
			 ( c >> 24 )            |
			(( c >>  8 )& 0x00FF00 )|
			(( c <<  8 )& 0xFF0000 )|
			 ( c << 24 )
	);
}

int
n32 (cvoid *s)
{
	return *(LONG *)s;
}

void
Ie ()
{
	int c;
	c = 1;
	if (*(char *)&c == 1)
		le32 = n32;
	else
		le32 = e32;
}

File         wad_file;
File        list_file;

int         list_c;
char ***    list_v;

char   * directory;
char   *  lump;
int       lumpsize;

char   * sectors;
int      sectors_c;

char   *   sides;
int        sides_c;

int      st_floors;
int      st_ceilings;
int      st_sectors;

int      st_sides;
int      st_uppers;
int      st_mids;
int      st_lowers;

/* this is horseshit */
char   * old;
char   * new;
int      did;

void
Itable ()
{
	char a[1024];

	char ***ttt;
	char ***ppp;

	char  **pp;

	int c;

	while (Fgets(&list_file, sizeof a, a))
	{
		c = a[0];
		if (!(
					c == '\n' ||
					c == '#'
		))
		{
			list_c++;
		}
	}

	rewind(list_file.fp);

	list_v = Calloc(list_c, sizeof (char **));
	for (
			ttt = ( ppp = list_v ) + list_c;
			ppp < ttt;
			++ppp
	)
	{
		(*ppp) = pp = Calloc(2, sizeof (char *));
		pp[0] = Malloc(9);
		pp[1] = Malloc(9);
	}
}

void
Iwad ()
{
	char  buf[12];

	char *  t;
	char *  p;
	int   map;

	char *sector_p;
	char *  side_p;

	int n;
	int h;

	Fread(&wad_file, 12, buf);
	if (
			memcmp(buf, "IWAD", 4) != 0 &&
			memcmp(buf, "PWAD", 4) != 0
	)
	{
		Pexit(-1,"%s: Not a WAD\n", wad_file.filename);
	}

	Fseek(&wad_file, (*le32)(&buf[8]));

	n         = (*le32)(&buf[4]) * 8;
	h         = n / 9;
	n        *= 2;
	directory = Malloc(n);
	/* minimum number of lumps for a map */
	sectors   = Malloc(h);
	sides     = Malloc(h);

	Fread(&wad_file, n, directory);

	sector_p = sectors;
	side_p   = sides;
	map = 3;
	for (t = ( p = directory ) + n; p < t; p += 16)
	{
		/* looking for SECTORS? Hopefully order doesn't matter in real world. */
		/* also search for fucking SIDES MY SIDES AAAAAAAAAA */
		switch (map)
		{
			case 0:
			case 2:
				if (strncmp(&p[8], "SECTORS", 8) == 0)
				{
					/* copy file offset and size */
					memcpy(sector_p, p, 8);
					sector_p += 8;
					sectors_c++;
					map |= 1;
				}
			case 1:
				if (strncmp(&p[8], "SIDEDEFS", 8) == 0)
				{
					memcpy(side_p, p, 8);
					side_p += 8;
					sides_c++;
					map |= 2;
				}
		}
		if (map == 3)
		{
			/* MAP marker */
			if (p[13] == '\0' && strncmp(&p[8], "MAP", 3) == 0)
				map = 0;
		}
	}
}

void
Fuckyou (char *p, int f, int *st)
{
	if (strncmp(p, old, 8) == 0)
	{
		strncpy(p, new, 8);
		(*st)++;
		did |= f;
	}
}

void
Epic (char *p, char *t)
{
	char *top;
	char *bot;
	int i;
	/* oh hi magic number! */
	for (; p < t; p += 26)
	{
		bot = &p [4];
		top = &p[12];
		did = 0;
		for (i = 0; i < list_c; ++i)
		{
			old = list_v[i][0];
			new = list_v[i][1];
			switch (did)
			{
				case 0:
				case 2:
					Fuckyou(bot, 1, &st_floors);
				case 1:
					Fuckyou(top, 2, &st_ceilings);
			}
			if (did == 3)
				break;
		}
		if (did)
			st_sectors++;
	}
}

void
Epic2 (char *p, char *t)
{
	char *top;
	char *mid;
	char *bot;
	int i;
	for (; p < t; p += 30)
	{
		top = &p [4];
		bot = &p[12];
		mid = &p[20];
		did = 0;
		for (i = 0; i < list_c; ++i)
		{
			old = list_v[i][0];
			new = list_v[i][1];
			switch (did)
			{
				case 0:
				case 2:
				case 4:
				case 6:
					Fuckyou(top, 1, &st_uppers);
				case 1:
				case 5:
					Fuckyou(mid, 2, &st_mids);
				case 3:
					Fuckyou(bot, 4, &st_lowers);
			}
			if (did == 7)
				break;
		}
		if (did)
			st_sides++;
	}
}

void
Fuck (char *p, int c, void (*fn)(char *,char *))
{
	char *t;
	int offs;
	int size;
	for (t = p + c * 8; p < t; p += 8)
	{
		offs = (*le32)(p);
		size = (*le32)(p + 4);
		if (lumpsize < size)
		{
			Reallocp(&lump, size);
			lumpsize = size;
		}
		Fseek(&wad_file, offs);
		Fread(&wad_file, size, lump);
		(*fn)(lump, lump + size);
		Fseek(&wad_file, offs);
		Fwrite(&wad_file, size, lump);
	}
}

void
Awad ()
{
	Fuck (sectors, sectors_c, Epic);
	Fuck   (sides,   sides_c, Epic2);
}

void
Readtable ()
{
	char    a[1024];

	int     s;
	char *old;
	char *new;

	int c;

	s = 0;

	while (Fgets(&list_file, sizeof a, a))
	{
		c = a[0];
		if (!(
				c == '\n' ||
				c == '#'
		))
		{
			if (
					( old = strtok(a, DELIM) ) &&
					( new = strtok(0, DELIM) )
			)
			{
				strucpy(list_v[s][0], old, 8);
				strucpy(list_v[s][1], new, 8);
				++s;
			}
		}
	}
}

void
Cleanup ()
{
	char ***ttt;
	char ***ppp;

	char  **pp;

	free(lump);
	free(sides);
	free(sectors);
	free(directory);

	if (list_v)
	{
		for (
				ttt = ( ppp = list_v ) + list_c;
				ppp < ttt && ( pp = (*ppp) );
				++ppp
		)
		{
			free(pp[0]);
			free(pp[1]);
			free(pp);
		}
		free(list_v);
	}
}

int
main (int ac, char **av)
{
	int n;

	if (ac < 3)
		Pexit(0,HELP);

	Fopen (& wad_file, av[1], "rb+");
	Fopen (&list_file, av[2], "r");

	if (atexit(Cleanup) != 0)
		Pexit(-1,"Failed to register cleanup function.\n");

	Itable();
	Readtable();

	Ie();

	Iwad();
	Awad();

	printf(
			"%5d sectors changed.\n"
			"%5d floors.\n"
			"%5d ceilings.\n"
			"\n"
			"%5d sides.\n"
			"%5d upper textures.\n"
			"%5d mid textures.\n"
			"%5d lower textures.\n",

			st_sectors,

			st_floors,
			st_ceilings,

			st_sides,

			st_uppers,
			st_mids,
			st_lowers);

	return 0;
}

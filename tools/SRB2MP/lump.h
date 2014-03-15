/*
    LumpMod v0.21, a command-line utility for working with lumps in wad files.
    Copyright (C) 2003 Thunder Palace Entertainment.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    lump.h: Defines constants, structures, and functions used in lump.c
*/

#ifndef __LUMP_H
#define __LUMP_H

/* Entries in the wadfile directory are 16 bytes */
#define DIRENTRYLEN 16

/* Lumps and associated info */
struct lump {
    long len;
    unsigned char *data;
    char name[8];
};

/* Linked list of lumps */
struct lumplist {
    struct lump *cl;        /* actual content of the lump */
    struct lumplist *next;
};

/* Structure to contain all wadfile data */
struct wadfile {
    char id[4];             /* IWAD or PWAD */
    unsigned long numlumps;          /* 32-bit integer */
    struct lumplist *head;  /* points to first entry */
};

/* Function declarations */
struct wadfile *read_wadfile(FILE *);
void free_wadfile(struct wadfile *);
int write_wadfile(FILE *, struct wadfile *);
char *get_lump_name(struct lump *);
struct lumplist *find_previous_lump(struct lumplist *, struct lumplist *, char *);
void remove_next_lump(struct wadfile *, struct lumplist *);
int add_lump(struct wadfile *, struct lumplist *, char *, long, unsigned char *);
int rename_lump(struct lump *, char *);
struct lumplist *find_last_lump(struct wadfile *);
struct lumplist *find_last_lump_between(struct lumplist *, struct lumplist *);
struct lumplist *find_next_section_lump(struct lumplist *);

#endif

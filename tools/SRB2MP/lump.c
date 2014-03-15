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

    lump.c: Provides functions for dealing with lumps
*/

#include "StdAfx.h"
#include <string.h>
#include <ctype.h>
#include "lump.h"

/* Read contents of a wad file and store them in memory.
 * fpoint is the file to read, opened with "rb" mode.
 * A pointer to a new wadfile struct will be returned, or NULL on error.
 */
struct wadfile *read_wadfile(FILE *fpoint) {
    struct wadfile *wfptr;
    struct lumplist *curlump;
    unsigned long diroffset, filelen;
    unsigned long count;

    /* Allocate memory for wadfile struct */
    wfptr = (struct wadfile*)malloc(sizeof(struct wadfile));
    if(wfptr == NULL) return NULL;

    /* Read first four characters (PWAD or IWAD) */
    if(fread(wfptr->id, 4, 1, fpoint) < 1) {
        free(wfptr);
        return NULL;
    }

    /* Read number of lumps */
    if(fread(&(wfptr->numlumps), 4, 1, fpoint) < 1) {
        free(wfptr);
        return NULL;
    }

    /* If number of lumps is zero, nothing more needs to be done */
    if(wfptr->numlumps == 0) {
        wfptr->head = NULL;
        return wfptr;
    }

    /* Read offset of directory */
    if(fread(&diroffset, 4, 1, fpoint) < 1) {
        free(wfptr);
        return NULL;
    }

    /* Verify that the directory as long as it needs to be */
    fseek(fpoint, 0, SEEK_END);
    filelen = ftell(fpoint);
    if((filelen - diroffset) / DIRENTRYLEN < wfptr->numlumps) {
        free(wfptr);
        return NULL;
    }

    /* Allocate memory for head lumplist item and set head pointer */
    curlump = (struct lumplist*)malloc(sizeof(struct lumplist));
    if(curlump == NULL) {
        free(wfptr);
        return NULL;
    }
    wfptr->head = curlump;
    curlump->cl = NULL;

    /* Read directory entries and lumps */
    for(count = 0; count < wfptr->numlumps; count++) {
        long lumpdataoffset;

        /* Advance to a new list item */
        curlump->next = (struct lumplist*)malloc(sizeof(struct lumplist));
        if(curlump->next == NULL) {
            free_wadfile(wfptr);
            return NULL;
        }
        curlump = curlump->next;
        curlump->next = NULL;

        /* Allocate memory for the lump info */
        curlump->cl = (struct lump*)malloc(sizeof(struct lump));
        if(curlump->cl == NULL) {
            free_wadfile(wfptr);
            return NULL;
        }

        /* Seek to the proper position in the file */
        if(fseek(fpoint, diroffset + (count * DIRENTRYLEN), SEEK_SET) != 0) {
            free_wadfile(wfptr);
            return NULL;
        }

        /* Read offset of lump data */
        if(fread(&lumpdataoffset, 4, 1, fpoint) < 1) {
            free_wadfile(wfptr);
            return NULL;
        }

        /* Read size of lump in bytes */
        if(fread(&(curlump->cl->len), 4, 1, fpoint) < 1) {
            free_wadfile(wfptr);
            return NULL;
        }

        /* Read lump name */
        if(fread(curlump->cl->name, 8, 1, fpoint) < 1) {
            free_wadfile(wfptr);
            return NULL;
        }

        /* Read actual lump data, unless lump size is 0 */
        if(curlump->cl->len > 0) {
            if(fseek(fpoint, lumpdataoffset, SEEK_SET) != 0) {
                free_wadfile(wfptr);
                return NULL;
            }

            /* Allocate memory for data */
            curlump->cl->data = (unsigned char*)malloc(curlump->cl->len);
            if(curlump->cl->data == NULL) {
                free_wadfile(wfptr);
                return NULL;
            }

            /* Fill the data buffer */
            if(fread(curlump->cl->data, curlump->cl->len, 1, fpoint) < 1) {
                free_wadfile(wfptr);
                return NULL;
            }
        } else curlump->cl->data = NULL;
    } /* End of directory reading loop */

    return wfptr;
}

/* Free a wadfile from memory as well as all related structures.
 */
void free_wadfile(struct wadfile *wfptr) {
    struct lumplist *curlump, *nextlump;

    if(wfptr == NULL) return;
    curlump = wfptr->head;

    /* Free items in the lump list */
    while(curlump != NULL) {

        /* Free the actual lump and its data, if necessary */
        if(curlump->cl != NULL) {
            if(curlump->cl->data != NULL) free(curlump->cl->data);
            free(curlump->cl);
        }

        /* Advance to next lump and free this one */
        nextlump = curlump->next;
        free(curlump);
        curlump = nextlump;
    }

    free(wfptr);
}

/* Write complete wadfile to a file stream, opened with "wb" mode.
 * fpoint is the stream to write to.
 * wfptr is a pointer to the wadfile structure to use.
 * Return zero on success, nonzero on failure.
 */
int write_wadfile(FILE *fpoint, struct wadfile *wfptr) {
    struct lumplist *curlump;
    long lumpdataoffset, diroffset;

    if(wfptr == NULL) return 1;

    /* Write four-character ID ("PWAD" or "IWAD") */
    if(fwrite(wfptr->id, 4, 1, fpoint) < 1) return 2;

    /* Write number of lumps */
    if(fwrite(&(wfptr->numlumps), 4, 1, fpoint) < 1) return 3;

    /* Offset of directory is not known yet. For now, write number of lumps
     * again, just to fill the space.
     */
    if(fwrite(&(wfptr->numlumps), 4, 1, fpoint) < 1) return 4;

    /* Loop through lump list, writing lump data */
    for(curlump = wfptr->head; curlump != NULL; curlump = curlump->next) {

        /* Don't write anything for the head of the lump list or for lumps of
           zero length */
        if(curlump->cl == NULL || curlump->cl->data == NULL) continue;

        /* Write the data */
        if(fwrite(curlump->cl->data, curlump->cl->len, 1, fpoint) < 1)
            return 5;
    }

    /* Current position is where directory will start */
    diroffset = ftell(fpoint);

    /* Offset for the first lump's data is always 12 */
    lumpdataoffset = 12;

    /* Loop through lump list again, this time writing directory entries */
    for(curlump = wfptr->head; curlump != NULL; curlump = curlump->next) {

        /* Don't write anything for the head of the lump list */
        if(curlump->cl == NULL) continue;

        /* Write offset for lump data */
        if(fwrite(&lumpdataoffset, 4, 1, fpoint) < 1) return 6;

        /* Write size of lump data */
        if(fwrite(&(curlump->cl->len), 4, 1, fpoint) < 1) return 7;

        /* Write lump name */
        if(fwrite(curlump->cl->name, 8, 1, fpoint) < 1) return 8;

        /* Increment lumpdataoffset variable as appropriate */
        lumpdataoffset += curlump->cl->len;
    }

    /* Go back to header and write the proper directory offset */
    fseek(fpoint, 8, SEEK_SET);
    if(fwrite(&diroffset, 4, 1, fpoint) < 1) return 9;

    return 0;
}

/* Get the name of a lump, as a null-terminated string.
 * item is a pointer to the lump (not lumplist) whose name will be obtained.
 * Return NULL on error.
 */
char *get_lump_name(struct lump *item) {
    char convname[9], *retname;

    if(item == NULL) return NULL;
    memcpy(convname, item->name, 8);
    convname[8] = '\0';

    retname = (char*)malloc(strlen(convname) + 1);
    if(retname != NULL) strcpy(retname, convname);
    return retname;
}

/* Find the lump after start and before end having a certain name.
 * Return a pointer to the list item for that lump, or return NULL if no lump
 * by that name is found or lumpname is too long.
 * lumpname is a null-terminated string.
 * If end parameter is NULL, search to the end of the entire list.
 */
struct lumplist *find_previous_lump(struct lumplist *start, struct lumplist
        *end, char *lumpname) {
    struct lumplist *curlump, *lastlump;
    char *curname;
    int found = 0;

    /* Verify that parameters are valid */
    if(start==NULL || start==end || lumpname==NULL || strlen(lumpname) > 8)
        return NULL;

    /* Loop through the list from start parameter */
    lastlump = start;
    for(curlump = start->next; curlump != end && curlump != NULL;
            curlump = curlump->next) {

        /* Skip header lump */
        if(curlump->cl == NULL) continue;

        /* Find name of this lump */
        curname = get_lump_name(curlump->cl);
        if(curname == NULL) continue;

        /* Compare names to see if this is the lump we want */
        if(strcmp(curname, lumpname) == 0) {
            found = 1;
            break;
        }

        /* Free memory allocated to curname */
        free(curname);

        lastlump = curlump;
    }

    if(found) return lastlump;
    return NULL;
}

/* Remove a lump from the list, free it, and free its data.
 * before is the lump immediately preceding the lump to be removed.
 * wfptr is a pointer to the wadfile structure to which the removed lump
 * belongs, so that numlumps can be decreased.
 */
void remove_next_lump(struct wadfile *wfptr, struct lumplist *before) {
    struct lumplist *removed;

    /* Verify that parameters are valid */
    if(before == NULL || before->next == NULL || wfptr == NULL) return;

    /* Update linked list to omit removed lump */
    removed = before->next;
    before->next = removed->next;

    /* Free lump info and data if necessary */
    if(removed->cl != NULL) {
        if(removed->cl->data != NULL) free(removed->cl->data);
        free(removed->cl);
    }

    free(removed);

    /* Decrement numlumps */
    wfptr->numlumps--;
}

/* Add a lump.
 * The lump will follow prev in the list and be named name, with a data size
 * of len.
 * A copy will be made of the data.
 * Return zero on success or nonzero on failure.
 */
int add_lump(struct wadfile *wfptr, struct lumplist *prev, char *name, long
        len, unsigned char *data) {
    struct lump *newlump;
    struct lumplist *newlumplist;
    unsigned char *copydata;

    /* Verify that parameters are valid */
    if(wfptr == NULL || prev == NULL || name == NULL || strlen(name) > 8)
        return 1;

    /* Allocate space for newlump and newlumplist */
    newlump = (struct lump*)malloc(sizeof(struct lump));
    newlumplist = (struct lumplist*)malloc(sizeof(struct lumplist));
    if(newlump == NULL || newlumplist == NULL) return 2;

    /* Copy lump data and set up newlump */
    if(len == 0 || data == NULL) {
        newlump->len = 0;
        newlump->data = NULL;
    } else {
        newlump->len = len;
        copydata = (unsigned char*)malloc(len);
        if(copydata == NULL) return 3;
        memcpy(copydata, data, len);
        newlump->data = copydata;
    }

    /* Set name of newlump */
    memset(newlump->name, '\0', 8);
    if(strlen(name) == 8) memcpy(newlump->name, name, 8);
    else strcpy(newlump->name, name);

    /* Set up newlumplist and alter prev appropriately */
    newlumplist->cl = newlump;
    newlumplist->next = prev->next;
    prev->next = newlumplist;

    /* Increment numlumps */
    wfptr->numlumps++;

    return 0;
}

/* Rename a lump.
 * renamed is a pointer to the lump (not lumplist) that needs renaming.
 * newname is a null-terminated string with the new name.
 * Return zero on success or nonzero on failure.
 */
int rename_lump(struct lump *renamed, char *newname) {

    /* Verify that parameters are valid. */
    if(newname == NULL || renamed == NULL || strlen(newname) > 8) return 1;

    /* Do the renaming. */
    memset(renamed->name, '\0', 8);
    if(strlen(newname) == 8) memcpy(renamed->name, newname, 8);
    else strcpy(renamed->name, newname);

    return 0;
}

/* Find the last lump in a wadfile structure.
 * Return this lump or NULL on failure.
 */
struct lumplist *find_last_lump(struct wadfile *wfptr) {
    struct lumplist *curlump;

    if(wfptr == NULL || wfptr->head == NULL) return NULL;
    curlump = wfptr->head;

    while(curlump->next != NULL) curlump = curlump->next;
    return curlump;
}

/* Find the last lump between start and end.
 * Return this lump or NULL on failure.
 */
struct lumplist *find_last_lump_between(struct lumplist *start, struct
        lumplist *end) {
    struct lumplist *curlump;

    if(start == NULL) return NULL;
    curlump = start;

    while(curlump->next != end) {
        curlump = curlump->next;
        if(curlump == NULL) break;
    }

    return curlump;
}

/* Find the next section lump. A section lump is MAPxx (0 <= x <= 9), ExMy
 * (0 <= x <= 9, 0 <= y <= 9), or any lump whose name ends in _START or _END.
 * Return NULL if there are no section lumps after start.
 */
struct lumplist *find_next_section_lump(struct lumplist *start) {
    struct lumplist *curlump, *found = NULL;
    char *curname;

    /* Verify that parameter is valid */
    if(start == NULL || start->next == NULL) return NULL;

    /* Loop through the list from start parameter */
    for(curlump = start->next; curlump != NULL && found == NULL;
            curlump = curlump->next) {

        /* Skip header lump */
        if(curlump->cl == NULL) continue;

        /* Find name of this lump */
        curname = get_lump_name(curlump->cl);
        if(curname == NULL) continue;

        /* Check to see if this is a section lump */
        if(strlen(curname) == 5 && strncmp("MAP", curname, 3) == 0 &&
                isdigit(curname[3]) && isdigit(curname[4]))
            found = curlump;
        else if(strlen(curname) == 4 && curname[0] == 'E' && curname[2] ==
                'M' && isdigit(curname[1]) && isdigit(curname[3]))
            found = curlump;
        else if(strlen(curname) == 7 && strcmp("_START", &curname[1]) == 0)
            found = curlump;
        else if(strlen(curname) == 8 && strcmp("_START", &curname[2]) == 0)
            found = curlump;
        else if(strlen(curname) == 5 && strcmp("_END", &curname[1]) == 0)
            found = curlump;
        else if(strlen(curname) == 6 && strcmp("_END", &curname[2]) == 0)
            found = curlump;

        /* Free memory allocated to curname */
        free(curname);
    }

    return found;
}

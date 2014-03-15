/*
    LumpMod v0.2.1, a command-line utility for working with lumps in wad
                    files.
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

    lumpmod.c: Provides program functionality; depends on lump.c and lump.h
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lump.h"

int main(int argc, char *argv[]) {
    enum commands { C_ADD, C_ADDSECT, C_DELETE, C_DELSECT, C_EXTRACT, C_LIST,
            C_RENAME, C_UPDATE } cmd;
    struct wadfile *wfptr;
    struct lumplist *startitem, *enditem = NULL;
    FILE *fpoint;
    char *progname, **params;
    char *startname = NULL, *endname = NULL;
    int numargs, numparams, insection = 0;

    progname = argv[0];
    numargs = argc - 1;

    /* Verify that there are enough arguments */
    if(numargs < 2) {
        fprintf(stderr, "%s: not enough arguments\n", progname);
        return EXIT_FAILURE;
    }

    /* Identify the command used */
         if(strcmp(argv[2], "add"    ) == 0) cmd = C_ADD;
    else if(strcmp(argv[2], "addsect") == 0) cmd = C_ADDSECT;
    else if(strcmp(argv[2], "delete" ) == 0) cmd = C_DELETE;
    else if(strcmp(argv[2], "delsect") == 0) cmd = C_DELSECT;
    else if(strcmp(argv[2], "extract") == 0) cmd = C_EXTRACT;
    else if(strcmp(argv[2], "list"   ) == 0) cmd = C_LIST;
    else if(strcmp(argv[2], "rename" ) == 0) cmd = C_RENAME;
    else if(strcmp(argv[2], "update" ) == 0) cmd = C_UPDATE;
    else {
        fprintf(stderr, "%s: invalid command %s\n", progname, argv[2]);
        return EXIT_FAILURE;
    }

    /* Check for -s option */
    if(numargs >= 4 && strcmp(argv[3], "-s") == 0) {
        if(cmd == C_ADDSECT || cmd == C_DELSECT) {
            fprintf(stderr, "%s: no option -s for command %s\n", progname, argv[2]);
            return EXIT_FAILURE;
        }
        insection = 1;
        params = &argv[5];
        numparams = numargs - 4;

        /* Assume a map name if length > 2 */
        if(strlen(argv[4]) > 2) startname = argv[4];
        else {
            startname = malloc(strlen(argv[4]) + 7);
            endname = malloc(strlen(argv[4]) + 5);
            if(startname == NULL || endname == NULL) {
                fprintf(stderr, "%s: out of memory\n", progname);
                return EXIT_FAILURE;
            }
            sprintf(startname, "%s_START", argv[4]);
            sprintf(endname, "%s_END", argv[4]);
        }
    } else {
        params = &argv[3];
        numparams = numargs - 2;
    } /* end of check for -s option */

    /* Load the wadfile into memory, since all commands require this */
    fpoint = fopen(argv[1], "rb");
    if(fpoint == NULL) {
        fprintf(stderr, "%s: unable to open file %s\n", progname, argv[1]);
        return EXIT_FAILURE;
    }
    wfptr = read_wadfile(fpoint);
    fclose(fpoint);
    if(wfptr == NULL) {
        fprintf(stderr, "%s: %s is not a valid wadfile\n", progname, argv[1]);
        return EXIT_FAILURE;
    }

    /* Find startitem and enditem, using startname and endname */
    if(startname == NULL) startitem = wfptr->head;
    else {
        startitem = find_previous_lump(wfptr->head, NULL, startname);
        if(startitem == NULL) {
            fprintf(stderr, "%s: can't find lump %s in %s", progname,
                    startname, argv[1]);
            return EXIT_FAILURE;
        }
        startitem = startitem->next;
        if(endname == NULL) {
            if(startitem->next != NULL) {
                char *itemname;

                enditem = startitem->next;
                itemname = get_lump_name(enditem->cl);
                while(  strcmp(itemname, "THINGS"  ) == 0 ||
                        strcmp(itemname, "LINEDEFS") == 0 ||
                        strcmp(itemname, "SIDEDEFS") == 0 ||
                        strcmp(itemname, "VERTEXES") == 0 ||
                        strcmp(itemname, "SEGS"    ) == 0 ||
                        strcmp(itemname, "SSECTORS") == 0 ||
                        strcmp(itemname, "NODES"   ) == 0 ||
                        strcmp(itemname, "SECTORS" ) == 0 ||
                        strcmp(itemname, "REJECT"  ) == 0 ||
                        strcmp(itemname, "BLOCKMAP") == 0) {
                    enditem = enditem->next;
                    if(enditem == NULL) break;
                    free(itemname);
                    itemname = get_lump_name(enditem->cl);
                }
                free(itemname);
                if(enditem != NULL) enditem = enditem->next;
            } else enditem = NULL;
        } else {
            enditem = find_previous_lump(startitem, NULL, endname);
            if(enditem == NULL) {
                fprintf(stderr, "%s: can't find lump %s in %s", progname,
                        endname, argv[1]);
                return EXIT_FAILURE;
            }
            enditem = enditem->next;
        }
    } /* end of finding startitem and enditem */

    /* Do stuff specific to each command */
    switch(cmd) {
        case C_ADD: {
            struct lumplist *startitem2, *before, *existing;
            int overwrite = 1, firstentry = 0, canduplicate = 0;
            char *startname2 = NULL;

            /* Parse options: -a, -b, -d, -n */
            while(numparams > 2) {
                if(params[0][0] != '-') break;
                if(strcmp(params[0], "-n") == 0) overwrite = 0;
                else if(strcmp(params[0], "-d") == 0) canduplicate = 1;
                else if(strcmp(params[0], "-b") == 0) firstentry = 1;
                else if(strcmp(params[0], "-a") == 0) {
                    params = &params[1];
                    numparams--;
                    startname2 = params[0];
                } else {
                    fprintf(stderr, "%s: no option %s for command %s",
                            progname, params[0], argv[2]);
                    return EXIT_FAILURE;
                }
                params = &params[1];
                numparams--;
            }

            if(numparams < 2) {
                fprintf(stderr, "%s: not enough parameters for %s", progname,
                        argv[2]);
                return EXIT_FAILURE;
            }

            /* Find the lump after which to add */
            if(firstentry) before = startitem;
            else if(startname2 != NULL) {
                before = find_previous_lump(startitem, enditem, startname2);
                if(before == NULL) {
                    fprintf(stderr, "%s: can't find lump %s in %s", progname,
                            startname2, argv[1]);
                    return EXIT_FAILURE;
                }
                before = before->next;
            } else {
                if(insection) {
                    before = find_last_lump_between(startitem, enditem);
                    if(before == NULL) before = startitem;
                } else before = find_last_lump(wfptr);
            }
            startitem2 = before;

            /* Add LUMPNAME FILENAME pairs */
            printf("Adding lumps in %s...\n", argv[1]);
            for(;;) {
                long newlumpsize;
                unsigned char *newlumpdata;

                /* Check whether the lump already exists, unless -d is used */
                if(canduplicate) existing = NULL;
                else existing = find_previous_lump(startitem, enditem, params[0]);
                if(existing != NULL) existing = existing->next;
                if(existing != NULL && overwrite == 0) {
                    printf("Lump %s already exists. Taking no action.\n", params[0]);
                    numparams -= 2;
                    if(numparams < 2) break;
                    params = &params[2];
                    continue;
                }

                /* Read the file with new lump data */
                fpoint = fopen(params[1], "rb");
                if(fpoint == NULL) {
                    fprintf(stderr, "%s: can't open file %s\n", progname,
                            params[1]);
                    return EXIT_FAILURE;
                }

                /* Find size of lump data */
                fseek(fpoint, 0, SEEK_END);
                newlumpsize = ftell(fpoint);
                fseek(fpoint, 0, SEEK_SET);

                /* Copy lump data to memory */
                if(newlumpsize == 0) newlumpdata = NULL;
                else {
                    newlumpdata = malloc(newlumpsize);
                    if(newlumpdata == NULL) {
                        fprintf(stderr, "%s: out of memory\n", progname);
                        return EXIT_FAILURE;
                    }
                    if(fread(newlumpdata, newlumpsize, 1, fpoint) < 1) {
                        fprintf(stderr, "%s: unable to read file %s\n",
                                progname, params[1]);
                        return EXIT_FAILURE;
                    }
                }

                /* Close the file */
                fclose(fpoint);

                /* Add or update lump */
                if(existing == NULL) {
                    if(add_lump(wfptr, before, params[0], newlumpsize,
                            newlumpdata) != 0) {
                        fprintf(stderr, "%s: unable to add lump %s\n",
                                progname, params[0]);
                        return EXIT_FAILURE;
                    }
                    printf("Lump %s added.\n", params[0]);

                    /* Other lumps will be added after the new one */
                    before = before->next;
                } else {
                    existing->cl->len = newlumpsize;
                    free(existing->cl->data);
                    existing->cl->data = malloc(newlumpsize);
                    if(existing->cl->data == NULL) {
                        fprintf(stderr, "%s: out of memory\n", progname);
                        return EXIT_FAILURE;
                    }
                    memcpy(existing->cl->data, newlumpdata, newlumpsize);
                    printf("Lump %s updated.\n", params[0]);
                }

                /* Advance to the next pair, if there is a next pair */
                numparams -= 2;
                if(numparams < 2) break;
                params = &params[2];
            } /* end of adding LUMPNAME FILENAME pairs */

            /* Save the modified wadfile */
            fpoint = fopen(argv[1], "wb");
            if(fpoint == NULL) {
                fprintf(stderr, "%s: unable to open file %s for writing\n",
                        progname, argv[1]);
                return EXIT_FAILURE;
            }
            if(write_wadfile(fpoint, wfptr) != 0) {
                fprintf(stderr, "%s: unable to write wadfile to disk\n",
                        progname);
                return EXIT_FAILURE;
            }
            fclose(fpoint);
            printf("File %s successfully updated.\n", argv[1]);
        } /* end of C_ADD */
        break;

        case C_UPDATE: {
            struct lumplist *existing;

            /* Parse options (none) */
            if(numparams > 2 && params[0][0] == '-') {
                fprintf(stderr, "%s: no option %s for command %s",
                        progname, params[0], argv[2]);
                return EXIT_FAILURE;
            }

            if(numparams < 2) {
                fprintf(stderr, "%s: not enough parameters for %s", progname,
                        argv[2]);
                return EXIT_FAILURE;
            }

            /* Update LUMPNAME FILENAME pairs */
            printf("Updating lumps in %s...\n", argv[1]);
            for(;;) {
                long newlumpsize;
                unsigned char *newlumpdata;

                /* Check whether the lump already exists */
                existing = find_previous_lump(startitem, enditem, params[0]);
                if(existing == NULL) {
                    printf("Lump %s does not exist. Taking no action.\n",
                            params[0]);
                    numparams -= 2;
                    if(numparams < 2) break;
                    params = &params[2];
                    continue;
                }
                existing = existing->next;

                /* Read the file with new lump data */
                fpoint = fopen(params[1], "rb");
                if(fpoint == NULL) {
                    fprintf(stderr, "%s: can't open file %s\n", progname,
                            params[1]);
                    return EXIT_FAILURE;
                }

                /* Find size of lump data */
                fseek(fpoint, 0, SEEK_END);
                newlumpsize = ftell(fpoint);
                fseek(fpoint, 0, SEEK_SET);

                /* Copy lump data to memory */
                if(newlumpsize == 0) newlumpdata = NULL;
                else {
                    newlumpdata = malloc(newlumpsize);
                    if(newlumpdata == NULL) {
                        fprintf(stderr, "%s: out of memory\n", progname);
                        return EXIT_FAILURE;
                    }
                    if(fread(newlumpdata, newlumpsize, 1, fpoint) < 1) {
                        fprintf(stderr, "%s: unable to read file %s\n",
                                progname, params[1]);
                        return EXIT_FAILURE;
                    }
                }

                /* Close the file */
                fclose(fpoint);

                /* Update lump */
                existing->cl->len = newlumpsize;
                free(existing->cl->data);
                existing->cl->data = malloc(newlumpsize);
                if(existing->cl->data == NULL) {
                    fprintf(stderr, "%s: out of memory\n", progname);
                    return EXIT_FAILURE;
                }
                memcpy(existing->cl->data, newlumpdata, newlumpsize);
                printf("Lump %s updated.\n", params[0]);

                /* Advance to the next pair, if there is a next pair */
                numparams -= 2;
                if(numparams < 2) break;
                params = &params[2];
            } /* end of updating LUMPNAME FILENAME pairs */

            /* Save the modified wadfile */
            fpoint = fopen(argv[1], "wb");
            if(fpoint == NULL) {
                fprintf(stderr, "%s: unable to open file %s for writing\n",
                        progname, argv[1]);
                return EXIT_FAILURE;
            }
            if(write_wadfile(fpoint, wfptr) != 0) {
                fprintf(stderr, "%s: unable to write wadfile to disk\n",
                        progname);
                return EXIT_FAILURE;
            }
            fclose(fpoint);
            printf("File %s successfully updated.\n", argv[1]);
        } /* end of C_UPDATE */
        break;

        case C_DELETE: {
            struct lumplist *before;

            /* Parse options (none) */
            if(numparams > 1 && params[0][0] == '-') {
                fprintf(stderr, "%s: no option %s for command %s",
                        progname, params[0], argv[2]);
                return EXIT_FAILURE;
            }

            if(numparams < 1) {
                fprintf(stderr, "%s: not enough parameters for %s", progname,
                        argv[2]);
                return EXIT_FAILURE;
            }

            /* Delete LUMPNAME lumps */
            printf("Deleting lumps in %s...\n", argv[1]);
            for(;;) {

                /* Find the lump to delete */
                before = find_previous_lump(startitem, enditem, params[0]);
                if(before == NULL) {
                    printf("Lump %s does not exist. Taking no action.\n",
                            params[0]);
                    numparams--;
                    if(numparams < 1) break;
                    params = &params[1];
                    continue;
                }

                /* Delete it */
                remove_next_lump(wfptr, before);
                printf("Lump %s deleted.\n", params[0]);

                /* Advance to the next item to delete, if there is one */
                numparams--;
                if(numparams < 1) break;
                params = &params[1];
            } /* end of deleting LUMPNAME lumps */

            /* Save the modified wadfile */
            fpoint = fopen(argv[1], "wb");
            if(fpoint == NULL) {
                fprintf(stderr, "%s: unable to open file %s for writing\n",
                        progname, argv[1]);
                return EXIT_FAILURE;
            }
            if(write_wadfile(fpoint, wfptr) != 0) {
                fprintf(stderr, "%s: unable to write wadfile to disk\n",
                        progname);
                return EXIT_FAILURE;
            }
            fclose(fpoint);
            printf("File %s successfully updated.\n", argv[1]);
        } /* end of C_DELETE */
        break;

        case C_RENAME: {
            struct lumplist *renamed;

            /* Parse options (none) */
            if(numparams > 2 && params[0][0] == '-') {
                fprintf(stderr, "%s: no option %s for command %s",
                        progname, params[0], argv[2]);
                return EXIT_FAILURE;
            }

            if(numparams < 2) {
                fprintf(stderr, "%s: not enough parameters for %s", progname,
                        argv[2]);
                return EXIT_FAILURE;
            }

            /* Rename OLDNAME NEWNAME pairs */
            printf("Renaming lumps in %s...\n", argv[1]);
            for(;;) {

                /* Find the lump to rename */
                renamed = find_previous_lump(startitem, enditem, params[0]);
                if(renamed == NULL) {
                    printf("Lump %s does not exist. Taking no action.\n",
                            params[0]);
                    numparams -= 2;
                    if(numparams < 2) break;
                    params = &params[2];
                    continue;
                }
                renamed = renamed->next;

                /* Rename lump */
                memset(renamed->cl->name, '\0', 8);
                if(strlen(params[1]) >= 8) memcpy(renamed->cl->name,
                        params[1], 8);
                else strcpy(renamed->cl->name, params[1]);

                printf("Lump %s renamed to %s.\n", params[0], params[1]);

                /* Advance to the next pair, if there is a next pair */
                numparams -= 2;
                if(numparams < 2) break;
                params = &params[2];
            } /* end of renaming OLDNAME NEWNAME pairs */

            /* Save the modified wadfile */
            fpoint = fopen(argv[1], "wb");
            if(fpoint == NULL) {
                fprintf(stderr, "%s: unable to open file %s for writing\n",
                        progname, argv[1]);
                return EXIT_FAILURE;
            }
            if(write_wadfile(fpoint, wfptr) != 0) {
                fprintf(stderr, "%s: unable to write wadfile to disk\n",
                        progname);
                return EXIT_FAILURE;
            }
            fclose(fpoint);
            printf("File %s successfully updated.\n", argv[1]);
        } /* end of C_RENAME */
        break;

        case C_EXTRACT: {
            struct lumplist *extracted;

            /* Parse options (none) */
            if(numparams > 2 && params[0][0] == '-') {
                fprintf(stderr, "%s: no option %s for command %s",
                        progname, params[0], argv[2]);
                return EXIT_FAILURE;
            }

            if(numparams < 2) {
                fprintf(stderr, "%s: not enough parameters for %s", progname,
                        argv[2]);
                return EXIT_FAILURE;
            }

            /* Extract LUMPNAME FILENAME pairs */
            printf("Extracting lumps from %s...\n", argv[1]);
            for(;;) {

                /* Find the lump to extract */
                extracted = find_previous_lump(startitem, enditem, params[0]);
                if(extracted == NULL) {
                    printf("Lump %s does not exist. Taking no action.\n",
                            params[0]);
                    numparams -= 2;
                    if(numparams < 2) break;
                    params = &params[2];
                    continue;
                }
                extracted = extracted->next;

                /* Open the file to extract to */
                fpoint = fopen(params[1], "wb");
                if(fpoint == NULL) {
                    fprintf(stderr, "%s: can't open file %s for writing\n",
                            progname, params[1]);
                    return EXIT_FAILURE;
                }

                /* Extract lump */
                if(fwrite(extracted->cl->data, extracted->cl->len, 1, fpoint)
                        < 1) {
                    fprintf(stderr, "%s: unable to write lump %s to disk\n",
                            progname, params[0]);
                    return EXIT_FAILURE;
                }

                /* Close the file */
                fclose(fpoint);
                printf("Lump %s saved as %s.\n", params[0], params[1]);

                /* Advance to the next pair, if there is a next pair */
                numparams -= 2;
                if(numparams < 2) break;
                params = &params[2];
            } /* end of extracting LUMPNAME FILENAME pairs */

            printf("Finished extracting lumps from file %s.\n", argv[1]);
        } /* end of C_EXTRACT */
        break;

        case C_LIST: {
            struct lumplist *curlump;
            int verbose = 0, i = 0;

            /* Parse options: -v */
            while(numparams > 0) {
                if(params[0][0] != '-') break;
                if(strcmp(params[0], "-v") == 0) verbose = 1;
                else {
                    fprintf(stderr, "%s: no option %s for command %s",
                            progname, params[0], argv[2]);
                    return EXIT_FAILURE;
                }
                params = &params[1];
                numparams--;
            }

            /* Loop through the lump list, printing lump info */
            for(curlump = startitem->next; curlump != enditem; curlump =
                    curlump->next) {
                i++;
                if(verbose) printf("%5i %-8s %7li\n", i,
                        get_lump_name(curlump->cl), curlump->cl->len);
                else printf("%s\n", get_lump_name(curlump->cl));
            }
        } /* end of C_LIST */
        break;

        case C_DELSECT: {

            /* Parse options (none) */
            if(numparams > 1 && params[0][0] == '-') {
                fprintf(stderr, "%s: no option %s for command %s",
                        progname, params[0], argv[2]);
                return EXIT_FAILURE;
            }

            if(numparams < 1) {
                fprintf(stderr, "%s: not enough parameters for %s", progname,
                        argv[2]);
                return EXIT_FAILURE;
            }

            /* Delete sections */
            printf("Deleting sections in %s...\n", argv[1]);
            for(;;) {
                struct lumplist *curlump;

                /* Assume a map name if length > 2 */
                if(strlen(params[0]) > 2) startname = params[0];
                else {
                    startname = malloc(strlen(params[0]) + 7);
                    endname = malloc(strlen(params[0]) + 5);
                    if(startname == NULL || endname == NULL) {
                        fprintf(stderr, "%s: out of memory\n", progname);
                        return EXIT_FAILURE;
                    }
                    sprintf(startname, "%s_START", params[0]);
                    sprintf(endname, "%s_END", params[0]);
                }

                /* Find startitem and enditem, using startname and endname */
                startitem = find_previous_lump(wfptr->head, NULL, startname);
                if(startitem == NULL) {
                    fprintf(stderr, "%s: can't find lump %s in %s", progname,
                            startname, argv[1]);
                    return EXIT_FAILURE;
                }
                if(endname == NULL && startitem->next != NULL) {
                    char *itemname;

                    enditem = startitem->next;
                    itemname = get_lump_name(enditem->cl);
                    do {
                        enditem = enditem->next;
                        if(enditem == NULL) break;
                        free(itemname);
                        itemname = get_lump_name(enditem->cl);
                    } while(strcmp(itemname, "THINGS"  ) == 0 ||
                            strcmp(itemname, "LINEDEFS") == 0 ||
                            strcmp(itemname, "SIDEDEFS") == 0 ||
                            strcmp(itemname, "VERTEXES") == 0 ||
                            strcmp(itemname, "SEGS"    ) == 0 ||
                            strcmp(itemname, "SSECTORS") == 0 ||
                            strcmp(itemname, "NODES"   ) == 0 ||
                            strcmp(itemname, "SECTORS" ) == 0 ||
                            strcmp(itemname, "REJECT"  ) == 0 ||
                            strcmp(itemname, "BLOCKMAP") == 0);
                    free(itemname);
                }
                else {
                    enditem = find_previous_lump(startitem, NULL, endname);
                    if(enditem == NULL) {
                        fprintf(stderr, "%s: can't find lump %s in %s",
                                progname, endname, argv[1]);
                        return EXIT_FAILURE;
                    }
                    enditem = enditem->next->next;
                } /* end of finding startitem and enditem */

                /* Loop through the section lumps, deleting them */
                curlump = startitem;
                while(curlump->next != enditem)
                    remove_next_lump(wfptr, curlump);
                printf("Deleted section %s.\n", params[0]);

                /* Move to next parameter, if it exists */
                numparams--;
                if(numparams < 1) break;
                params = &params[1];
            } /* end of deleting sections */

            /* Save the modified wadfile */
            fpoint = fopen(argv[1], "wb");
            if(fpoint == NULL) {
                fprintf(stderr, "%s: unable to open file %s for writing\n",
                        progname, argv[1]);
                return EXIT_FAILURE;
            }
            if(write_wadfile(fpoint, wfptr) != 0) {
                fprintf(stderr, "%s: unable to write wadfile to disk\n",
                        progname);
                return EXIT_FAILURE;
            }
            fclose(fpoint);
            printf("File %s successfully updated.\n", argv[1]);
        } /* end of C_DELSECT */
        break;

        case C_ADDSECT: {

            /* Parse options (none) */
            if(numparams > 1 && params[0][0] == '-') {
                fprintf(stderr, "%s: no option %s for command %s",
                        progname, params[0], argv[2]);
                return EXIT_FAILURE;
            }

            if(numparams < 1) {
                fprintf(stderr, "%s: not enough parameters for %s", progname,
                        argv[2]);
                return EXIT_FAILURE;
            }

            /* Add sections */
            printf("Adding sections in %s...\n", argv[1]);
            for(;;) {
                struct lumplist *curlump;

                /* Assume a map name if length > 2 */
                if(strlen(params[0]) > 2) startname = params[0];
                else {
                    startname = malloc(strlen(params[0]) + 7);
                    endname = malloc(strlen(params[0]) + 5);
                    if(startname == NULL || endname == NULL) {
                        fprintf(stderr, "%s: out of memory\n", progname);
                        return EXIT_FAILURE;
                    }
                    sprintf(startname, "%s_START", params[0]);
                    sprintf(endname, "%s_END", params[0]);
                }

                /* Add section, unless it already exists */
                if(find_previous_lump(wfptr->head, NULL, startname) == NULL) {
                    struct lumplist *last;

                    last = find_last_lump(wfptr);
                    if(add_lump(wfptr, last, startname, 0, NULL) != 0) {
                        fprintf(stderr, "%s: unable to add lump %s\n",
                            progname, startname);
                        return EXIT_FAILURE;
                    }

                    if(endname != NULL) {
                        last = last->next;
                        if(add_lump(wfptr, last, endname, 0, NULL) != 0) {
                            fprintf(stderr, "%s: unable to add lump %s\n",
                                progname, endname);
                            return EXIT_FAILURE;
                        }
                    }

                    printf("Added section %s.\n", params[0]);
                } else
                    printf("Section %s already exists. Taking no action.\n",
                        params[0]);

                /* Move to next parameter, if it exists */
                numparams--;
                if(numparams < 1) break;
                params = &params[1];
            } /* end of adding sections */

            /* Save the modified wadfile */
            fpoint = fopen(argv[1], "wb");
            if(fpoint == NULL) {
                fprintf(stderr, "%s: unable to open file %s for writing\n",
                        progname, argv[1]);
                return EXIT_FAILURE;
            }
            if(write_wadfile(fpoint, wfptr) != 0) {
                fprintf(stderr, "%s: unable to write wadfile to disk\n",
                        progname);
                return EXIT_FAILURE;
            }
            fclose(fpoint);
            printf("File %s successfully updated.\n", argv[1]);
        } /* end of C_ADDSECT */
    } /* end of command-specific stuff */

    return EXIT_SUCCESS;
}

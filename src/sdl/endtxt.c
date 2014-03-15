/*
 * Function to write the SRB2 end message text
 *
 * Copyright (C) 1998 by Udo Munk <udo@umserver.umnet.de>
 *
 * This code is provided AS IS and there are no guarantees, none.
 * Feel free to share and modify.
 */
//-----------------------------------------------------------------------------
/// \file
/// \brief Support to show ENDOOM text
///
/// Loads the lump ENDOOM, set up the console to print
/// out the colors and text

#include <stdio.h>
#include <stdlib.h>

// need this 19990118 by Kin
#include "../doomdef.h"
#include "../w_wad.h"
#include "../z_zone.h"
#include "endtxt.h"
/**	\brief	The ShowEndTxt function


  Prints out the ENDOOM the way DOOM.EXE/DOOM2.EXE did for Win32 or Linux/GNU

	\return	void


*/

void ShowEndTxt(void)
{
#if !(defined (_WIN32_WCE) || defined (_XBOX) || defined (_arch_dreamcast))
	INT32 i;
	UINT16 j, att = 0;
	INT32 nlflag = 1;
#ifdef _WIN32
	HANDLE co = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD mode, bytesWritten;
	CONSOLE_SCREEN_BUFFER_INFO backupcon;
	COORD resizewin = {80,-1};
	CHAR let = 0;
#endif
	UINT16 *ptext;
	void *data;
	lumpnum_t endoomnum = W_GetNumForName("ENDOOM");
	//char *col;

	/* if the xterm has more then 80 columns we need to add nl's */
	/* doesn't work, COLUMNS is not in the environment at this time ???
	col = I_getenv("COLUMNS");
	if (col) {
		if (atoi(col) > 80)
			nlflag++;
	}
	*/

	/* get the lump with the text */
	data = ptext = W_CacheLumpNum(endoomnum, PU_CACHE);

#ifdef _WIN32
	if (co == INVALID_HANDLE_VALUE || GetFileType(co) != FILE_TYPE_CHAR || !GetConsoleMode(co, &mode)) // test if it a good handle
	{
		Z_Free(data);
		return;
	}

	backupcon.wAttributes = FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE; // Just in case
	GetConsoleScreenBufferInfo(co, &backupcon); //Store old state
	resizewin.Y = backupcon.dwSize.Y;
	if (backupcon.dwSize.X < resizewin.X)
		SetConsoleScreenBufferSize(co, resizewin);

	for (i=1; i<=80*25; i++) // print 80x25 text and deal with the attributes too
	{
		j = (UINT16)(*ptext >> 8); // attribute first
		let = (char)(*ptext & 0xff); // text second
		if (j != att) // attribute changed?
		{
			att = j; // save current attribute
			SetConsoleTextAttribute(co, j); //set fg and bg color for buffer
		}

		WriteConsoleA(co, &let,  1, &bytesWritten, NULL); // now the text

		if (nlflag && !(i % 80) && backupcon.dwSize.X > resizewin.X) // do we need a nl?
		{
			att = backupcon.wAttributes;
			SetConsoleTextAttribute(co, att); // all attributes off
			WriteConsoleA(co, "\n",  1, &bytesWritten, NULL); // newline to console
		}
		ptext++;
	}
	SetConsoleTextAttribute(co, backupcon.wAttributes); // all attributes off
#else
	/* print 80x25 text and deal with the attributes too */
	for (i=1; i<=80*25; i++) {
		/* attribute first */
		/* attribute changed? */
		if ((j = *ptext >> 8) != att) {
			/* save current attribute */
			att = j;
			/* set new attribute, forground color first */
			printf("\033[");
			switch (j & 0x0f) {
			case 0:		/* black */
				printf("30");
				break;
			case 1:		/* blue */
				printf("34");
				break;
			case 2:		/* green */
				printf("32");
				break;
			case 3:		/* cyan */
				printf("36");
				break;
			case 4:		/* red */
				printf("31");
				break;
			case 5:		/* magenta */
				printf("35");
				break;
			case 6:		/* brown */
				printf("33");
				break;
			case 7:		/* bright grey */
				printf("37");
				break;
			case 8:		/* dark grey */
				printf("1;30");
				break;
			case 9:		/* bright blue */
				printf("1;34");
				break;
			case 10:	/* bright green */
				printf("1;32");
				break;
			case 11:	/* bright cyan */
				printf("1;36");
				break;
			case 12:	/* bright red */
				printf("1;31");
				break;
			case 13:	/* bright magenta */
				printf("1;35");
				break;
			case 14:	/* yellow */
				printf("1;33");
				break;
			case 15:	/* white */
				printf("1;37");
				break;
			}
			printf("m");
			/* now background color */
			printf("\033[");
			switch ((j >> 4) & 0x0f) {
			case 0:		/* black */
				printf("40");
				break;
			case 1:		/* blue */
				printf("44");
				break;
			case 2:		/* green */
				printf("42");
				break;
			case 3:		/* cyan */
				printf("46");
				break;
			case 4:		/* red */
				printf("41");
				break;
			case 5:		/* magenta */
				printf("45");
				break;
			case 6:		/* brown */
				printf("43");
				break;
			case 7:		/* bright grey */
				printf("47");
				break;
			case 8:		/* dark grey */
				printf("1;40");
				break;
			case 9:		/* bright blue */
				printf("1;44");
				break;
			case 10:	/* bright green */
				printf("1;42");
				break;
			case 11:	/* bright cyan */
				printf("1;46");
				break;
			case 12:	/* bright red */
				printf("1;41");
				break;
			case 13:	/* bright magenta */
				printf("1;45");
				break;
			case 14:	/* yellow */
				printf("1;43");
				break;
			case 15:	/* white */
				printf("1;47");
				break;
			}
			printf("m");
		}

		/* now the text */
		printf("%c",*ptext++ & 0xff);

		/* do we need a nl? */
		if (nlflag)
		{
			if (!(i % 80))
			{
				printf("\033[0m");
				att = 0;
				printf("\n");
			}
		}
	}
	/* all attributes off */
	printf("\033[0m");
#endif
	if (nlflag)
		printf("\n");

	Z_Free(data);
#endif
}

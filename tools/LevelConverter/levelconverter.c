//
// Level Converter
//
// Converts thing types from 1.09.4 to 1.1
//
// By A.J. Freda
//
// Modified from Graue's "Heightedit"
// and uses Graue's "LumpMod" utility.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#define STARPOSTS_ONLY

#define TABLE_SIZE 16384
int conversion[TABLE_SIZE]; // Conversion table.
FILE *lvlconvfile;

static inline void InitializeThingConversionTable(void)
{
	memset(conversion, 0, sizeof(conversion));

	// Player starts
	conversion[1] = 1;
	conversion[2] = 2;
	conversion[3] = 3;
	conversion[4] = 4;
	conversion[4001] = 5;
	conversion[4002] = 6;
	conversion[4003] = 7;
	conversion[4004] = 8;
	conversion[4005] = 9;
	conversion[4006] = 10;
	conversion[4007] = 11;
	conversion[4008] = 12;
	conversion[4009] = 13;
	conversion[4010] = 14;
	conversion[4011] = 15;
	conversion[4012] = 16;
	conversion[4013] = 17;
	conversion[4014] = 18;
	conversion[4015] = 19;
	conversion[4016] = 20;
	conversion[4017] = 21;
	conversion[4018] = 22;
	conversion[4019] = 23;
	conversion[4020] = 24;
	conversion[4021] = 25;
	conversion[4022] = 26;
	conversion[4023] = 27;
	conversion[4024] = 28;
	conversion[4025] = 29;
	conversion[4026] = 30;
	conversion[4027] = 31;
	conversion[4028] = 32;
	conversion[11] = 33;
	conversion[87] = 34;
	conversion[89] = 35;

	// Enemies
	conversion[3004] = 100;
	conversion[9] = 101;
	conversion[58] = 102;
	conversion[5005] = 103;
	conversion[5006] = 104;
	conversion[3005] = 105;
	conversion[22] = 106;
	conversion[21] = 107;
	conversion[71] = 108;
	conversion[56] = 109;
	conversion[2004] = 110;
	conversion[42] = 111;

	// Bosses
	conversion[16] = 200;
	conversion[2008] = 201;
	conversion[17] = 290;
	conversion[2049] = 291;

	// Collectibles
	conversion[2014] = 300;
	conversion[69] = 301;
	conversion[3003] = 302;
	conversion[80] = 303;
	conversion[26] = 304;
	conversion[54] = 305;
	conversion[31] = 310;
	conversion[34] = 311;
	conversion[2013] = 312;
	conversion[420] = 313;
	conversion[421] = 314;
	conversion[422] = 315;
	conversion[423] = 316;
	conversion[424] = 317;
	conversion[425] = 318;
	conversion[426] = 319;
	conversion[64] = 320;
	conversion[3002] = 320;
	conversion[3001] = 320;

	// Boxes
	conversion[2011] = 400;
	conversion[2012] = 400;
	conversion[48] = 402;
	conversion[2002] = 403;
	conversion[2018] = 404;
	conversion[35] = 405;
	conversion[2028] = 406;
	conversion[25] = 407;
	conversion[2022] = 408;
	conversion[41] = 409;
	conversion[2005] = 410;
	conversion[78] = 411;
	conversion[3000] = 412;

	// Interactive Objects
	conversion[33] = 500;
	conversion[86] = 501;
	conversion[3006] = 502;
//	conversion[?] = 520;
	conversion[23] = 521;
	conversion[67] = 522;
	conversion[68] = 523;
	conversion[32] = 540;
	conversion[30] = 541;
	conversion[28] = 550;
	conversion[79] = 551;
	conversion[5004] = 552;
	conversion[65] = 553;
	conversion[66] = 554;
	conversion[2015] = 555;
	conversion[38] = 556;
	conversion[20] = 557;
	conversion[39] = 558;

	// Special Placement Patterns
	conversion[84] = 600;
	conversion[44] = 601;
	conversion[76] = 602;
	conversion[77] = 603;
	conversion[47] = 604;
	conversion[2007] = 605;
	conversion[2048] = 606;
	conversion[2010] = 607;
	conversion[2046] = 608;
	conversion[2047] = 609;

	// Powerup Indicators/Environmental Effects/Miscellany
	conversion[2026] = 700;
	conversion[2024] = 701;
	conversion[2023] = 702;
	conversion[2045] = 703;
	conversion[83] = 704;
	conversion[2019] = 705;
	conversion[2025] = 706;
	conversion[27] = 707;
	conversion[14] = 708;
	conversion[43] = 709;
	conversion[8] = 750;
	conversion[5003] = 751;
	conversion[5007] = 752;
	conversion[18] = 753;
	conversion[5001] = 754;
	conversion[5002] = 755;
	conversion[2003] = 756;

	// Greenflower Scenery
	conversion[36] = 800;
	conversion[70] = 801;
	conversion[73] = 802;
	conversion[74] = 804;
	conversion[75] = 805;

	// Techno Hill Scenery
	conversion[2035] = 900;
	conversion[2006] = 901;

	// Deep Sea Scenery
	conversion[81] = 1000;

	// Castle Eggman Scenery
	conversion[49] = 1100;
	conversion[24] = 1101;
	conversion[52] = 1102;
	conversion[2001] = 1103;

	// NiGHTS Items
	conversion[72] = 1700;
	conversion[61] = 1701;
	conversion[46] = 1702;
	conversion[60] = 1703;
	conversion[82] = 1704;
	conversion[57] = 1705;
	conversion[37] = 1706;
	conversion[3007] = 1707;
	conversion[3008] = 1708;
	conversion[3009] = 1709;
	conversion[40] = 1710;

	// Mario Items
	conversion[10005] = 1800;
	conversion[10000] = 1801;
	conversion[10001] = 1802;
	conversion[50] = 1803;
	conversion[10] = 1804;
	conversion[29] = 1805;
	conversion[19] = 1806;
	conversion[12] = 1807;
	conversion[10002] = 1808;
	conversion[10003] = 1809;
	conversion[10004] = 1810;

	// Christmas Items
	conversion[5] = 1850;
	conversion[13] = 1851;
	conversion[6] = 1852;
}

static inline void InitializeSectorConversionTable(void)
{
	memset(conversion, 0, sizeof(conversion));

	// Player starts
	conversion[690] = -1;
	conversion[691] = -1;
	conversion[692] = -1;
	conversion[693] = -1;
	conversion[694] = -1;
	conversion[695] = -1;
	conversion[696] = -1;
	conversion[697] = -1;
	conversion[698] = -1;
	conversion[699] = -1;
	conversion[700] = -1;
	conversion[701] = -1;
	conversion[702] = -1;
	conversion[703] = -1;
	conversion[704] = -1;
	conversion[705] = -1;
	conversion[706] = -1;
	conversion[707] = -1;
	conversion[708] = -1;
	conversion[709] = -1;
	conversion[710] = -1;
	conversion[711] = -1;
	conversion[981] = -1;
	conversion[986] = -1;
	conversion[8] = -1;
	conversion[17] = -1;
	conversion[519] = 3 + (512 << 8);
	conversion[984] = 2 + (512 << 8);

	// Section 1
	conversion[11] = 1;
	conversion[983] = 2;
	conversion[7] = 3;
	conversion[18] = 4;
	conversion[4] = 5;
	conversion[16] = 6;
	conversion[5] = 7;
	conversion[10] = 8;
	conversion[978] = 9;
	conversion[980] = 10;
	conversion[9] = 11;
	conversion[6] = 12;
	conversion[992] = 13;
	conversion[996] = 14;
	conversion[14] = 15;

	// Section 2
	conversion[971] = 1 << 4;
	conversion[972] = 2 << 4;
	conversion[973] = 3 << 4;
	conversion[974] = 4 << 4;
	conversion[975] = 5 << 4;
	conversion[967] = 6 << 4;
	conversion[968] = 7 << 4;
	conversion[970] = 8 << 4;
	conversion[666] = 9 << 4;
	conversion[990] = 10 << 4;
	conversion[991] = 11 << 4;

	// Section 3
	conversion[256] = 1 << 8;
	conversion[512] = 2 << 8;
	conversion[768] = 3 << 8;
	conversion[985] = 4 << 8;
	conversion[976] = 5 << 8;
	conversion[977] = 6 << 8;
	conversion[1500] = 7 << 8;
	conversion[1501] = 8 << 8;
	conversion[1502] = 9 << 8;
	conversion[1503] = 10 << 8;
	conversion[1504] = 11 << 8;
	conversion[1505] = 12 << 8;
	conversion[1506] = 13 << 8;
	conversion[1507] = 14 << 8;
	conversion[1508] = 15 << 8;

	// Section 4
	conversion[993] = 1 << 12;
	conversion[33] = 2 << 12;
	conversion[982] = 2 << 12;
	conversion[987] = 2 << 12;
	conversion[995] = 2 << 12;
	conversion[988] = 3 << 12;
	conversion[989] = 4 << 12;
	conversion[997] = 5 << 12;
	conversion[969] = 6 << 12;
	conversion[979] = 7 << 12;
	conversion[998] = 8 << 12;
	conversion[999] = 9 << 12;
	conversion[994] = 10 << 12;
}

static inline void InitializeLinedefConversionTable(void)
{
	memset(conversion, 0, sizeof(conversion));

	// Old Water Removed
	conversion[14] = -1;

	// Level Parameters/Misc:
	conversion[64] = 1;
	conversion[71] = 2;
	conversion[18] = 3;
	conversion[65] = 4;
	conversion[63] = 5;
	conversion[73] = 6;
	conversion[66] = 7;

	// Level-Load Effects:
	conversion[26] = 50;
	conversion[24] = 51;
	conversion[88] = 52;
	conversion[2] = 53;
	conversion[3] = 54;
	conversion[4] = 55;
	conversion[6] = 56;
	conversion[7] = 57;
	conversion[8] = 58;
	conversion[232] = 59;
	conversion[233] = 60;
	conversion[43] = 61;
	conversion[50] = 62;
	conversion[242] = 63;

	// Floor-Over-Floors:
	conversion[25] = 100;
	conversion[33] = 101;
	conversion[44] = 102;
	conversion[69] = 103;
	conversion[51] = 104;
	conversion[57] = 105;
	conversion[48] = 120;
	conversion[45] = 121;
	conversion[75] = 122;
	conversion[74] = 123;
	conversion[59] = 140;
	conversion[81] = 141;
	conversion[77] = 142;
	conversion[38] = 150;
	conversion[68] = 151;
	conversion[72] = 152;
	conversion[34] = 160;
	conversion[36] = 170;
	conversion[35] = 171;
	conversion[79] = 172;
	conversion[80] = 173;
	conversion[82] = 174;
	conversion[83] = 175;
	conversion[39] = 176;
	conversion[1] = 177;
	conversion[37] = 178;
	conversion[42] = 179;
	conversion[40] = 180;
	conversion[89] = 190;
	conversion[90] = 191;
	conversion[91] = 192;
	conversion[94] = 193;
	conversion[92] = 194;
	conversion[93] = 195;
	conversion[49] = 200;
	conversion[47] = 201;
	conversion[46] = 202;
	conversion[62] = 220;
	conversion[52] = 221;
	conversion[67] = 222;
	conversion[58] = 223;
	conversion[41] = 250;
	conversion[54] = 251;
	conversion[76] = 252;
	conversion[86] = 253;
	conversion[55] = 254;
	conversion[78] = 255;
	conversion[84] = 256;
	conversion[56] = 257;
	conversion[53] = 258;
	conversion[87] = 259;

	// Linedef Executor Triggers:
	conversion[96] = 300;
	conversion[97] = 301;
	conversion[98] = 302;
	conversion[95] = 303;
	conversion[99] = 304;
	conversion[19] = 305;
	conversion[20] = 306;
	conversion[21] = 307;
	conversion[9] = 308;
	conversion[10] = 309;
	conversion[11] = 310;
	conversion[12] = 311;
	conversion[13] = 312;
	conversion[15] = 313;

	// Linedef Executor Options:
	conversion[101] = 400;
	conversion[102] = 401;
	conversion[103] = 402;
	conversion[106] = 403;
	conversion[107] = 404;
	conversion[108] = 405;
	conversion[109] = 406;
	conversion[110] = 407;
	conversion[111] = 408;
	conversion[112] = 409;
	conversion[114] = 410;
	conversion[116] = 411;
	conversion[104] = 412;
	conversion[105] = 413;
	conversion[115] = 414;
	conversion[113] = 415;
	conversion[119] = 416;
	conversion[120] = 417;
	conversion[117] = 420;
	conversion[118] = 421;
	conversion[121] = 422;
	conversion[123] = 423;
	conversion[124] = 424;
	conversion[125] = 425;
	conversion[122] = 426;
	conversion[126] = 427;
	conversion[127] = 428;

	// Scrollers/Pushers
	conversion[100] = 500;
	conversion[85] = 501;
	conversion[254] = 502;
	conversion[218] = 503;
	conversion[249] = 504;
	conversion[255] = 505;
	conversion[251] = 510;
	conversion[215] = 511;
	conversion[246] = 512;
	conversion[250] = 513;
	conversion[214] = 514;
	conversion[245] = 515;
	conversion[252] = 520;
	conversion[216] = 521;
	conversion[247] = 522;
	conversion[203] = 523;
	conversion[205] = 524;
	conversion[201] = 525;
	conversion[253] = 530;
	conversion[217] = 531;
	conversion[248] = 532;
	conversion[202] = 533;
	conversion[204] = 534;
	conversion[200] = 535;
	conversion[223] = 540;
	conversion[224] = 541;
	conversion[229] = 542;
	conversion[230] = 543;
	conversion[225] = 544;
	conversion[227] = 545;
	conversion[228] = 546;
	conversion[226] = 547;

	// Lighting
	conversion[213] = 600;
	
	conversion[5] = 601;
	conversion[60] = 602;
	conversion[61] = 603;
	conversion[16] = 606;

	// Translucency
	conversion[284] = 900;
	conversion[285] = 901;
	conversion[286] = 902;
	conversion[287] = 903;
	conversion[288] = 904;
	conversion[283] = 905;

	// Extended numbers
	conversion[281] = conversion[25];
	conversion[289] = conversion[33];
	conversion[282] = conversion[26];
}

#ifndef _WIN32
#include <ctype.h>
static inline const char* _strupr(char *n)
{
	const char *m = n;
        while (*n != '\0')
        {
                *n = toupper(*n);
                n++;
        }
        return m;
}
#define strupr _strupr
#endif

static inline int ConvertStarposts(char *argv[])
{
	char syscmd[100];
	InitializeThingConversionTable();

	sprintf(syscmd, "lumpmod %s extract -s MAP%s THINGS lvlconv.tmp",
		argv[1], strupr(argv[2]));
	system(syscmd);

	lvlconvfile = fopen("lvlconv.tmp", "r+b");
	if(lvlconvfile == NULL) {
		puts("Failed to open lvlconv.tmp.");
		return EXIT_FAILURE;
	}
	for(;;) {
		int c;
		short x, y, angle, type, options;

		fseek(lvlconvfile, 1, SEEK_CUR);
		fseek(lvlconvfile, -1, SEEK_CUR);

		c = fgetc(lvlconvfile);
		if(c == EOF) break;

		fseek(lvlconvfile, -1, SEEK_CUR);
		if(fread(&x, sizeof(short), 1, lvlconvfile) < 1) {
			puts("I CANNOT READ THE X");
			return EXIT_FAILURE;
		}
		if(fread(&y, sizeof(short), 1, lvlconvfile) < 1) {
			puts("I CAN'T READ THE Y");
			return EXIT_FAILURE;
		}
		if(fread(&angle, sizeof(short), 1, lvlconvfile) < 1) {
			puts("I CAN'T READ THE ANGLE");
			return EXIT_FAILURE;
		}
		if(fread(&type, sizeof(short), 1, lvlconvfile) < 1) {
			puts("I CAN'T READ THE TYPE");
			return EXIT_FAILURE;
		}
		if(fread(&options, sizeof(short), 1, lvlconvfile) < 1) {
			puts("I CAN'T READ THE FLAGS");
			return EXIT_FAILURE;
		}

		fseek(lvlconvfile, -6, SEEK_CUR);

		/* CHECK FOR MATCH */
		if(type == 502)
		{
			int numpost = options & 31;
			angle %= 360;
			angle += numpost * 360;
			options = 0;
		}

		if(fwrite(&angle, sizeof(short), 1, lvlconvfile) < 1) {
			puts("I CANNOT WRITE THE ANGLE");
			return EXIT_FAILURE;
		}

		if(fwrite(&type, sizeof(short), 1, lvlconvfile) < 1) {
			puts("I CANNOT WRITE THE TYPE");
			return EXIT_FAILURE;
		}

		if (fwrite(&options, sizeof(short), 1, lvlconvfile) < 1) {
			puts("I CANNOT WRITE THE FLAGS");
			return EXIT_FAILURE;
		}
	}
	fclose(lvlconvfile);
	puts("DONE, Man!.. Inserting into WAD.\n");
	sprintf(syscmd, "lumpmod %s update -s MAP%s THINGS lvlconv.tmp",
		argv[1], strupr(argv[2]));
	system(syscmd);
	remove("lvlconv.tmp");
	return 1;
}

static inline int ConvertThings(char *argv[])
{
	char syscmd[100];
	InitializeThingConversionTable();

	printf("NOTE: Skill settings not supported in 1.1. All of your objects\nhave been converted to appear regardless of skill level.\n");

	sprintf(syscmd, "lumpmod %s extract -s MAP%s THINGS lvlconv.tmp",
		argv[1], strupr(argv[2]));
	system(syscmd);

	lvlconvfile = fopen("lvlconv.tmp", "r+b");
	if(lvlconvfile == NULL) {
		puts("Failed to open lvlconv.tmp.");
		return EXIT_FAILURE;
	}
	for(;;) {
		int c;
		short x, y, angle, type, options;

		fseek(lvlconvfile, 1, SEEK_CUR);
		fseek(lvlconvfile, -1, SEEK_CUR);

		c = fgetc(lvlconvfile);
		if(c == EOF) break;

		fseek(lvlconvfile, -1, SEEK_CUR);
		if(fread(&x, sizeof(short), 1, lvlconvfile) < 1) {
			puts("I CANNOT READ THE X");
			return EXIT_FAILURE;
		}
		if(fread(&y, sizeof(short), 1, lvlconvfile) < 1) {
			puts("I CAN'T READ THE Y");
			return EXIT_FAILURE;
		}
		if(fread(&angle, sizeof(short), 1, lvlconvfile) < 1) {
			puts("I CAN'T READ THE ANGLE");
			return EXIT_FAILURE;
		}
		if(fread(&type, sizeof(short), 1, lvlconvfile) < 1) {
			puts("I CAN'T READ THE TYPE");
			return EXIT_FAILURE;
		}
		if(fread(&options, sizeof(short), 1, lvlconvfile) < 1) {
			puts("I CAN'T READ THE FLAGS");
			return EXIT_FAILURE;
		}

		fseek(lvlconvfile, -6, SEEK_CUR);

		/* CHECK FOR MATCH */
		if(type != 32000 && type != 32001) // !@#$%^ Doom Builder...
		{
			if(type < TABLE_SIZE && conversion[type] < 0)
				printf("\nThing type %d no longer supported. You will have to manually change this.", type);
			else if(type < TABLE_SIZE && conversion[type] != 0)
				type = conversion[type];
			else
				printf("\nUnknown thing type %d!", type);

			if (type != 502 && type != 1700 && type != 1701 && type != 1702 && type != 1703 && type != 1704 && type != 1705)
			{
				if (options == 1 || options == 4)
				{
					printf("\nThing type %d on Easy or Hard only, renamed to thing type 4242.", type);
					type = 4242;
				}

				options &= ~7; // Strip skill settings
			}
		}

		if (type == 502)
		{
			int numpost = options & 31;
			angle %= 360;
			angle += numpost * 360;
			options = 0;
		}

		if(fwrite(&angle, sizeof(short), 1, lvlconvfile) < 1) {
			puts("I CANNOT WRITE THE ANGLE");
			return EXIT_FAILURE;
		}

		if(fwrite(&type, sizeof(short), 1, lvlconvfile) < 1) {
			puts("I CANNOT WRITE THE TYPE");
			return EXIT_FAILURE;
		}

		if (fwrite(&options, sizeof(short), 1, lvlconvfile) < 1) {
			puts("I CANNOT WRITE THE FLAGS");
			return EXIT_FAILURE;
		}
	}
	fclose(lvlconvfile);
	puts("DONE, Man!.. Inserting into WAD.\n");
	sprintf(syscmd, "lumpmod %s update -s MAP%s THINGS lvlconv.tmp",
		argv[1], strupr(argv[2]));
	system(syscmd);
	remove("lvlconv.tmp");
	return 1;
}

static void ConvertFlat(char *garbage)
{
		/* CHECK FOR MATCH */
		if(!memcmp(garbage, "FLOOR0_3\0\0\0\0", 8))
			strncpy(garbage, "GFZFLR01\0\0\0\0", 8);
		else if(!memcmp(garbage, "FLOOR0_6\0\0\0", 8))
			strncpy(garbage, "GFZFLR02\0\0\0", 8);
		else if(!memcmp(garbage, "CEIL3_3\0\0\0\0\0", 8) || !memcmp(garbage, "GATE1\0\0\0\0\0\0", 8))
			strncpy(garbage, "GFZFLR03\0\0\0\0\0\0\0", 8);
		else if(!memcmp(garbage, "CEIL3_4\0\0\0\0\0", 8))
			strncpy(garbage, "GFZFLR04\0\0\0\0\0\0\0", 8);
		else if(!memcmp(garbage, "FLOOR1_1\0\0\0\0\0", 8))
			strncpy(garbage, "GFZFLR05\0\0\0\0\0\0\0", 8);
		else if(!memcmp(garbage, "FLOOR1_7\0\0\0\0\0", 8))
			strncpy(garbage, "GFZFLR06\0\0\0\0\0\0\0", 8);
		else if(!memcmp(garbage, "FLOOR5_3\0\0\0\0\0", 8))
			strncpy(garbage, "GFZFLR07\0\0\0\0\0\0\0", 8);
		else if(!memcmp(garbage, "OLDROCK\0\0\0\0\0\0", 8))
			strncpy(garbage, "GFZFLR08\0\0\0\0\0\0\0", 8);
		else if(!memcmp(garbage, "SFLR5_0\0\0\0\0\0", 8))
			strncpy(garbage, "GFZFLR09\0\0\0\0\0\0", 8);
		else if(!memcmp(garbage, "SFLR6_1\0\0\0\0\0", 8)
			|| !memcmp(garbage, "SFLR6_4\0\0\0\0\0", 8)
			|| !memcmp(garbage, "SFLR7_1\0\0\0\0\0", 8)
			|| !memcmp(garbage, "SFLR7_4\0\0\0\0\0", 8))
		{
			printf("Flat pic %s no longer supported. Use GFZFLR09 with\r\nlinedef special #7 to achieve same effect.\r\nFlat converted to GFZFLR09 for your convenience.\r\n", garbage);
			strncpy(garbage, "GFZFLR09\0\0\0\0\0\0", 8);
		}
		else if(!memcmp(garbage, "DEM1_4\0\0\0\0\0\0", 8))
			strncpy(garbage, "GFZFLR10\0\0\0\0", 8);

		else if(!memcmp(garbage, "FLOOR3_3\0\0\0\0\0", 8))
			strncpy(garbage, "THZFLR30\0\0\0\0\0\0\0", 8);
		else if(!memcmp(garbage, "FLOOR4_5\0\0\0\0\0", 8))
			strncpy(garbage, "THZFLR31\0\0\0\0\0\0\0", 8);
		else if(!memcmp(garbage, "OTHZW1\0\0\0\0\0", 8))
			strncpy(garbage, "THZFLR32\0\0\0\0\0\0\0", 8);
		else if(!memcmp(garbage, "OTHZW2\0\0\0\0\0", 8))
			strncpy(garbage, "THZFLR33\0\0\0\0\0\0\0", 8);

		else if(!memcmp(garbage, "BLOOD1\0\0", 8))
			strncpy(garbage, "LITER1\0\0", 8);
		else if(!memcmp(garbage, "BLOOD2\0\0", 8))
			strncpy(garbage, "LITER2\0\0", 8);
		else if(!memcmp(garbage, "BLOOD3\0\0", 8))
			strncpy(garbage, "LITER3\0\0", 8);

		else if(!memcmp(garbage, "BLUE1\0\0\0", 8))
			strncpy(garbage, "LITEB1\0\0", 8);
		else if(!memcmp(garbage, "BLUE2\0\0\0", 8))
			strncpy(garbage, "LITEB2\0\0", 8);
		else if(!memcmp(garbage, "BLUE3\0\0\0", 8))
			strncpy(garbage, "LITEB3\0\0", 8);

		else if(!memcmp(garbage, "GREY1\0\0\0", 8))
			strncpy(garbage, "LITEN1\0\0", 8);
		else if(!memcmp(garbage, "GREY2\0\0\0", 8))
			strncpy(garbage, "LITEN2\0\0", 8);
		else if(!memcmp(garbage, "GREY3\0\0\0", 8))
			strncpy(garbage, "LITEN3\0\0", 8);

		else if(!memcmp(garbage, "NUKAGE1\0\0\0", 8))
			strncpy(garbage, "LITEY1\0\0", 8);
		else if(!memcmp(garbage, "NUKAGE2\0\0\0", 8))
			strncpy(garbage, "LITEY2\0\0", 8);
		else if(!memcmp(garbage, "NUKAGE3\0\0\0", 8))
			strncpy(garbage, "LITEY3\0\0", 8);

		else if(!memcmp(garbage, "CEIL1_3\0\0\0\0\0", 8))
			strncpy(garbage, "CEZFLR09\0\0\0\0\0\0\0", 8);
		else if(!memcmp(garbage, "CEIL1_1\0\0\0\0\0", 8))
			strncpy(garbage, "CEZFLR10\0\0\0\0\0\0\0", 8);
		else if(!memcmp(garbage, "CEIL1_2\0\0\0\0\0", 8))
			strncpy(garbage, "CEZFLR11\0\0\0\0\0\0\0", 8);

		else if(!memcmp(garbage, "CONS1_1\0\0\0\0\0", 8))
			strncpy(garbage, "SFLR07\0\0\0\0\0\0\0", 8);
		else if(!memcmp(garbage, "CONS1_5\0\0\0\0\0", 8))
			strncpy(garbage, "SFLR08\0\0\0\0\0\0\0", 8);
		else if(!memcmp(garbage, "CONS1_7\0\0\0\0\0", 8))
			strncpy(garbage, "SFLR09\0\0\0\0\0\0\0", 8);
		else if(!memcmp(garbage, "FLOOR0_5\0\0\0\0\0", 8))
			strncpy(garbage, "SFLR10\0\0\0\0\0\0\0", 8);
		else if(!memcmp(garbage, "FLOOR0_7\0\0\0\0\0", 8))
			strncpy(garbage, "SFLR11\0\0\0\0\0\0\0", 8);
		else if(!memcmp(garbage, "FLOOR1_6\0\0\0\0\0", 8))
			strncpy(garbage, "SFLR12\0\0\0\0\0\0\0", 8);
		else if(!memcmp(garbage, "SLIME01\0\0\0\0\0", 8))
			strncpy(garbage, "SFLR13\0\0\0\0\0\0\0", 8);
		else if(!memcmp(garbage, "SLIME02\0\0\0\0\0", 8))
			strncpy(garbage, "SFLR14\0\0\0\0\0\0\0", 8);
		else if(!memcmp(garbage, "SLIME03\0\0\0\0\0", 8))
			strncpy(garbage, "SFLR15\0\0\0\0\0\0\0", 8);
		else if(!memcmp(garbage, "SLIME04\0\0\0\0\0", 8))
			strncpy(garbage, "SFLR12\0\0\0\0\0\0\0", 8);

		else if(!memcmp(garbage, "DCFLR1\0\0\0\0\0", 8))
			strncpy(garbage, "DCZFLR01\0\0\0\0\0\0\0", 8);
		else if(!memcmp(garbage, "DCCEIL1\0\0\0\0\0", 8))
			strncpy(garbage, "DCZFLR02\0\0\0\0\0\0\0", 8);
		else if(!memcmp(garbage, "DCPAVE\0\0\0\0\0", 8))
			strncpy(garbage, "DCZFLR03\0\0\0\0\0\0\0", 8);
		else if(!memcmp(garbage, "DCMANHO\0\0\0\0\0", 8))
			strncpy(garbage, "DCZFLR04\0\0\0\0\0\0\0", 8);
		else if(!memcmp(garbage, "DCTEX1\0\0\0\0\0", 8))
			strncpy(garbage, "DCZFLR05\0\0\0\0\0\0\0", 8);
		else if(!memcmp(garbage, "DCTEX2\0\0\0\0\0", 8))
			strncpy(garbage, "DCZFLR06\0\0\0\0\0\0\0", 8);
		else if(!memcmp(garbage, "DCTEX3\0\0\0\0\0", 8))
			strncpy(garbage, "DCZFLR07\0\0\0\0\0\0\0", 8);
		else if(!memcmp(garbage, "DCTEX4\0\0\0\0\0", 8))
			strncpy(garbage, "DCZFLR08\0\0\0\0\0\0\0", 8);
		else if(!memcmp(garbage, "DCWHITE\0\0\0\0\0", 8))
			strncpy(garbage, "DCZFLR09\0\0\0\0\0\0\0", 8);
		else if(!memcmp(garbage, "DCYELLOW\0\0\0\0\0", 8))
			strncpy(garbage, "DCZFLR10\0\0\0\0\0\0\0", 8);
		else if(!memcmp(garbage, "DENTFLR\0\0\0\0\0", 8))
			strncpy(garbage, "DCZFLR11\0\0\0\0\0\0\0", 8);
		else if(!memcmp(garbage, "DENTMETL\0\0\0\0\0", 8))
			strncpy(garbage, "DCZFLR12\0\0\0\0\0\0\0", 8); // Intentionally
		else if(!memcmp(garbage, "FLAT14\0\0\0\0\0\0", 8))
			strncpy(garbage, "DCZFLR12\0\0\0\0\0", 8); // The same!
		else if(!memcmp(garbage, "MFLR8_1\0\0\0\0\0", 8))
			strncpy(garbage, "DCZFLR13\0\0\0\0\0\0", 8);
		else if(!memcmp(garbage, "STEP2\0\0\0\0\0", 8))
			strncpy(garbage, "DCZFLR14\0\0\0\0\0", 8);


		else if(!memcmp(garbage, "DEEPSEA2\0\0\0\0\0", 8))
			strncpy(garbage, "DSZFLR04\0\0\0\0\0\0\0", 8);
		else if(!memcmp(garbage, "DEEPSEA3\0\0\0\0\0", 8))
			strncpy(garbage, "DSZFLR05\0\0\0\0\0\0\0", 8);
		else if(!memcmp(garbage, "DEEPSEA4\0\0\0\0\0", 8))
			strncpy(garbage, "DSZFLR06\0\0\0\0\0\0\0", 8);
		else if(!memcmp(garbage, "FLOOR4_6\0\0\0\0\0", 8))
			strncpy(garbage, "DSZFLR07\0\0\0\0\0\0\0", 8);
		else if(!memcmp(garbage, "SLIME14\0\0\0\0\0", 8))
			strncpy(garbage, "DSZFLR08\0\0\0\0\0\0\0", 8);
		else if(!memcmp(garbage, "SLIME15\0\0\0\0\0", 8))
			strncpy(garbage, "DSZFLR09\0\0\0\0\0\0\0", 8);
		else if(!memcmp(garbage, "SLIME16\0\0\0\0\0", 8))
			strncpy(garbage, "DSZFLR10\0\0\0\0\0\0\0", 8);

		else if(!memcmp(garbage, "ERROCK\0\0\0\0\0\0", 8))
			strncpy(garbage, "ERZFLR01\0\0\0\0\0", 8);
		else if(!memcmp(garbage, "FLAT18\0\0\0\0\0\0", 8))
			strncpy(garbage, "ERZFLR02\0\0\0\0\0", 8);
		else if(!memcmp(garbage, "FLAT20\0\0\0\0\0\0", 8))
			strncpy(garbage, "ERZFLR03\0\0\0\0\0", 8);
		else if(!memcmp(garbage, "FLAT22\0\0\0\0\0\0", 8))
			strncpy(garbage, "ERZFLR04\0\0\0\0\0", 8);
		else if(!memcmp(garbage, "FLAT23\0\0\0\0\0\0", 8))
			strncpy(garbage, "ERZFLR05\0\0\0\0\0", 8);
		else if(!memcmp(garbage, "FLAT5_4\0\0\0\0", 8))
			strncpy(garbage, "ERZFLR06\0\0\0\0\0", 8);

		else if(!memcmp(garbage, "FLAT10\0\0\0\0\0\0", 8))
			strncpy(garbage, "REDFLR\0\0\0\0\0", 8);
		else if(!memcmp(garbage, "FLAT5\0\0\0\0\0\0", 8))
			strncpy(garbage, "ORGFLR\0\0\0\0\0", 8);

		else if(!memcmp(garbage, "MFLR8_2\0\0\0\0\0\0", 8))
			strncpy(garbage, "GREYFLR\0\0\0\0\0", 8);
		else if(!memcmp(garbage, "MFLR8_4\0\0\0\0\0\0", 8))
			strncpy(garbage, "GREYFLR\0\0\0\0\0", 8);
}

static inline int ConvertSectors(char *argv[])
{
	char syscmd[100];
	InitializeSectorConversionTable();

	sprintf(syscmd, "lumpmod %s extract -s MAP%s SECTORS lvlconv.tmp",
		argv[1], strupr(argv[2]));
	system(syscmd);

	lvlconvfile = fopen("lvlconv.tmp", "r+b");
	if(lvlconvfile == NULL) {
		puts("Failed to open lvlconv.tmp.");
		return EXIT_FAILURE;
	}
	for(;;) {
		int c;
		char garbage[11];
		short type;

		c = fgetc(lvlconvfile);
		if(c == EOF) break;

		fseek(lvlconvfile, -1, SEEK_CUR);
		if(fread(&garbage, sizeof(short)*2, 1, lvlconvfile) < 1) {
			puts("I CANNOT READ THE DATA BEFORE SECTOR FLATS");
			return EXIT_FAILURE;
		}

		// Read floor flat first.
		if(fread(&garbage, 8, 1, lvlconvfile) < 1) {
			puts("I CAN'T READ THE SECTOR FLOOR FLAT");
			return EXIT_FAILURE;
		}

		fseek(lvlconvfile, -8, SEEK_CUR);

		garbage[8] = 0;

		ConvertFlat(garbage);

		if(fwrite(&garbage, 8, 1, lvlconvfile) < 1) {
			puts("I CANNOT WRITE THE FLOOR FLAT");
			return EXIT_FAILURE;
		}

		fseek(lvlconvfile, 0, SEEK_CUR);

		memset(garbage, 0, sizeof(garbage));

		// Now do ceiling flat.
		if(fread(&garbage, 8, 1, lvlconvfile) < 1) {
			puts("I CAN'T READ THE SECTOR CEILING FLAT");
			return EXIT_FAILURE;
		}

		fseek(lvlconvfile, -8, SEEK_CUR);

		garbage[8] = 0;
		ConvertFlat(garbage);

		if(fwrite(&garbage, 8, 1, lvlconvfile) < 1) {
			puts("I CANNOT WRITE THE CEILING FLAT");
			return EXIT_FAILURE;
		}

		// Skip lightlevel
		fseek(lvlconvfile, 1 * sizeof(short), SEEK_CUR);

		// Now do sector special.
		if(fread(&type, sizeof(short), 1, lvlconvfile) < 1) {
			puts("I CAN'T READ THE SECTOR SPECIAL");
			return EXIT_FAILURE;
		}

		fseek(lvlconvfile, -1 * (int)sizeof(short), SEEK_CUR);

		/* CHECK FOR MATCH */
		if(conversion[type] < 0)
			printf("\nSector type %d no longer supported. You will have to manually change this.", type);
		else if(conversion[type] != 0)
			type = conversion[type];
		else if (type != 0)
			printf("\nUnknown sector type %d!", type);

		if(fwrite(&type, sizeof(short), 1, lvlconvfile) < 1) {
			puts("I CANNOT WRITE THE TYPE");
			return EXIT_FAILURE;
		}

		/* skip 1 short of TOTAL CRAP */
		fseek(lvlconvfile, 1 * sizeof(short), SEEK_CUR);
	}
	fclose(lvlconvfile);
	puts("DONE, Man!.. Inserting back into WAD.\n");
	sprintf(syscmd, "lumpmod %s update -s MAP%s SECTORS lvlconv.tmp",
		argv[1], strupr(argv[2]));
	system(syscmd);
	remove("lvlconv.tmp");
	return 1;
}

static inline int ConvertLinedefs(char *argv[])
{
	char syscmd[100];
	InitializeLinedefConversionTable();

	sprintf(syscmd, "lumpmod %s extract -s MAP%s LINEDEFS lvlconv.tmp",
		argv[1], strupr(argv[2]));
	system(syscmd);

	lvlconvfile = fopen("lvlconv.tmp", "r+b");
	if(lvlconvfile == NULL) {
		puts("Failed to open lvlconv.tmp.");
		return EXIT_FAILURE;
	}
	for(;;) {
		int c;
		short garbage[3], type;

		c = fgetc(lvlconvfile);
		if(c == EOF) break;

		fseek(lvlconvfile, -1, SEEK_CUR);
		if(fread(&garbage, sizeof(short)*3, 1, lvlconvfile) < 1) {
			puts("I CANNOT READ THE DATA BEFORE THE LINEDEF SPECIAL");
			return EXIT_FAILURE;
		}
		if(fread(&type, sizeof(short), 1, lvlconvfile) < 1) {
			puts("I CAN'T READ THE LINEDEF TYPE");
			return EXIT_FAILURE;
		}

		fseek(lvlconvfile, -1 * (int)sizeof(short), SEEK_CUR);

		/* CHECK FOR MATCH */
		if(conversion[type] < 0)
			printf("\nLinedef type %d no longer supported. You will have to manually change this.", type);
		else if(conversion[type] != 0)
		{
			if(type == 65)
				printf("\nThis level contains a speed pad linedef. You will have to flip the direction of the line for it to work properly.");

			type = conversion[type];
		}
		else if (type != 0)
			printf("\nUnknown linedef type %d!", type);

		if(fwrite(&type, sizeof(short), 1, lvlconvfile) < 1) {
			puts("I CANNOT WRITE THE TYPE");
			return EXIT_FAILURE;
		}

		/* skip 3 shorts of TOTAL CRAP */
		fseek(lvlconvfile, 3 * sizeof(short), SEEK_CUR);
	}
	fclose(lvlconvfile);
	puts("DONE, Man!.. Inserting back into WAD.\n");
	sprintf(syscmd, "lumpmod %s update -s MAP%s LINEDEFS lvlconv.tmp",
		argv[1], strupr(argv[2]));
	system(syscmd);
	remove("lvlconv.tmp");
	return 1;
}

int main(int argc, char *argv[]) {
	if(argc != 3) {
		puts("Usage: lvlconv [filename] [mapnumber]");
		puts("Example: lvlconv wadfile.wad 01");
		return EXIT_FAILURE; /* YOU FAIL IT! */
	}

	if(strlen(argv[2]) != 2 || strlen(argv[1]) > 32) {
		puts("nonono you screwed it up");
		return EXIT_FAILURE; /* YOU LIKEWISE FAIL IT! */
	}
#ifdef STARPOSTS_ONLY
	ConvertStarposts(argv);
#else
	puts("\nConverting Things...");
	ConvertThings(argv);
	puts("\nConverting Sectors...");
	ConvertSectors(argv);
	puts("\nConverting Linedefs...");
	ConvertLinedefs(argv);
#endif

	puts("\nOkay, DONE!! Enjoy the NEWLY MODIFIED WAD FILE!!! HAVE A GOOD DAY!");
	return EXIT_SUCCESS; /* YOU SUCCEED AT IT! :) */
}

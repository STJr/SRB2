// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2016 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  f_finale.c
/// \brief Title screen, intro, game evaluation, and credits.

#include "doomdef.h"
#include "doomstat.h"
#include "d_main.h"
#include "f_finale.h"
#include "g_game.h"
#include "hu_stuff.h"
#include "r_local.h"
#include "s_sound.h"
#include "i_video.h"
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"
#include "i_system.h"
#include "m_menu.h"
#include "dehacked.h"
#include "g_input.h"
#include "console.h"
#include "m_random.h"
#include "y_inter.h"
#include "m_cond.h"

// Stage of animation:
// 0 = text, 1 = art screen
static INT32 finalecount;
INT32 titlescrollspeed = 80;

static INT32 timetonext; // Delay between screen changes
static INT32 continuetime; // Short delay when continuing

static tic_t animtimer; // Used for some animation timings
static INT32 roidtics; // Asteroid spinning

static INT32 deplete;
static tic_t stoptimer;

static boolean keypressed = false;

// (no longer) De-Demo'd Title Screen
static UINT8  curDemo = 0;
static UINT32 demoDelayLeft;
static UINT32 demoIdleLeft;

static patch_t *ttbanner; // white banner with "robo blast" and "2"
static patch_t *ttwing; // wing background
static patch_t *ttsonic; // "SONIC"
static patch_t *ttswave1; // Title Sonics
static patch_t *ttswave2;
static patch_t *ttswip1;
static patch_t *ttsprep1;
static patch_t *ttsprep2;
static patch_t *ttspop1;
static patch_t *ttspop2;
static patch_t *ttspop3;
static patch_t *ttspop4;
static patch_t *ttspop5;
static patch_t *ttspop6;
static patch_t *ttspop7;

static void F_SkyScroll(INT32 scrollspeed);

//
// CUTSCENE TEXT WRITING
//
static const char *cutscene_basetext = NULL;
static char cutscene_disptext[1024];
static INT32 cutscene_baseptr = 0;
static INT32 cutscene_writeptr = 0;
static INT32 cutscene_textcount = 0;
static INT32 cutscene_textspeed = 0;
static UINT8 cutscene_boostspeed = 0;
static tic_t cutscene_lasttextwrite = 0;

//
// This alters the text string cutscene_disptext.
// Use the typical string drawing functions to display it.
// Returns 0 if \0 is reached (end of input)
//
static UINT8 F_WriteText(void)
{
	INT32 numtowrite = 1;
	const char *c;
	tic_t ltw = I_GetTime();

	if (cutscene_lasttextwrite == ltw)
		return 1; // singletics prevention
	cutscene_lasttextwrite = ltw;

	if (cutscene_boostspeed)
	{
		// for custom cutscene speedup mode
		numtowrite = 8;
	}
	else
	{
		// Don't draw any characters if the count was 1 or more when we started
		if (--cutscene_textcount >= 0)
			return 1;

		if (cutscene_textspeed < 7)
			numtowrite = 8 - cutscene_textspeed;
	}

	for (;numtowrite > 0;++cutscene_baseptr)
	{
		c = &cutscene_basetext[cutscene_baseptr];
		if (!c || !*c || *c=='#')
			return 0;

		// \xA0 - \xAF = change text speed
		if ((UINT8)*c >= 0xA0 && (UINT8)*c <= 0xAF)
		{
			cutscene_textspeed = (INT32)((UINT8)*c - 0xA0);
			continue;
		}
		// \xB0 - \xD2 = delay character for up to one second (35 tics)
		else if ((UINT8)*c >= 0xB0 && (UINT8)*c <= (0xB0+TICRATE-1))
		{
			cutscene_textcount = (INT32)((UINT8)*c - 0xAF);
			numtowrite = 0;
			continue;
		}

		cutscene_disptext[cutscene_writeptr++] = *c;

		// Ignore other control codes (color)
		if ((UINT8)*c < 0x80)
			--numtowrite;
	}
	// Reset textcount for next tic based on speed
	// if it wasn't already set by a delay.
	if (cutscene_textcount < 0)
	{
		cutscene_textcount = 0;
		if (cutscene_textspeed > 7)
			cutscene_textcount = cutscene_textspeed - 7;
	}
	return 1;
}

static void F_NewCutscene(const char *basetext)
{
	cutscene_basetext = basetext;
	memset(cutscene_disptext,0,sizeof(cutscene_disptext));
	cutscene_writeptr = cutscene_baseptr = 0;
	cutscene_textspeed = 9;
	cutscene_textcount = TICRATE/2;
}

//
// F_DrawPatchCol
//
static void F_DrawPatchCol(INT32 x, patch_t *patch, INT32 col)
{
	const column_t *column;
	const UINT8 *source;
	UINT8 *desttop, *dest = NULL;
	const UINT8 *deststop, *destbottom;
	size_t count;

	desttop = screens[0] + x*vid.dupx;
	deststop = screens[0] + vid.rowbytes * vid.height;
	destbottom = desttop + vid.height*vid.width;

	do {
		INT32 topdelta, prevdelta = -1;
		column = (column_t *)((UINT8 *)patch + LONG(patch->columnofs[col]));

		// step through the posts in a column
		while (column->topdelta != 0xff)
		{
			topdelta = column->topdelta;
			if (topdelta <= prevdelta)
				topdelta += prevdelta;
			prevdelta = topdelta;
			source = (const UINT8 *)column + 3;
			dest = desttop + topdelta*vid.width;
			count = column->length;

			while (count--)
			{
				INT32 dupycount = vid.dupy;

				while (dupycount-- && dest < destbottom)
				{
					INT32 dupxcount = vid.dupx;
					while (dupxcount-- && dest <= deststop)
						*dest++ = *source;

					dest += (vid.width - vid.dupx);
				}
				source++;
			}
			column = (const column_t *)((const UINT8 *)column + column->length + 4);
		}

		desttop += SHORT(patch->height)*vid.dupy*vid.width;
	} while(dest < destbottom);
}

//
// F_SkyScroll
//
static void F_SkyScroll(INT32 scrollspeed)
{
	INT32 scrolled, x, mx, fakedwidth;
	patch_t *pat;

	pat = W_CachePatchName("TITLESKY", PU_CACHE);

	animtimer = ((finalecount*scrollspeed)/16) % SHORT(pat->width);

	fakedwidth = vid.width / vid.dupx;

	if (rendermode == render_soft)
	{ // if only hardware rendering could be this elegant and complete
		scrolled = (SHORT(pat->width) - animtimer) - 1;
		for (x = 0, mx = scrolled; x < fakedwidth; x++, mx = (mx+1)%SHORT(pat->width))
			F_DrawPatchCol(x, pat, mx);
	}
#ifdef HWRENDER
	else if (rendermode != render_none)
	{ // if only software rendering could be this simple and retarded
		INT32 dupz = (vid.dupx < vid.dupy ? vid.dupx : vid.dupy);
		INT32 y, pw = SHORT(pat->width) * dupz, ph = SHORT(pat->height) * dupz;
		scrolled = animtimer * dupz;
		for (x = 0; x < vid.width; x += pw)
		{
			for (y = 0; y < vid.height; y += ph)
			{
				if (scrolled > 0)
					V_DrawScaledPatch(scrolled - pw, y, V_NOSCALESTART, pat);

				V_DrawScaledPatch(x + scrolled, y, V_NOSCALESTART, pat);
			}
		}
	}
#endif

	W_UnlockCachedPatch(pat);
}

// =============
//  INTRO SCENE
// =============
#define NUMINTROSCENES 16
INT32 intro_scenenum = 0;
INT32 intro_curtime = 0;

const char *introtext[NUMINTROSCENES];

static tic_t introscenetime[NUMINTROSCENES] =
{
	 7*TICRATE + (TICRATE/2),	// STJr Presents
	11*TICRATE + (TICRATE/2),	// Two months had passed since...
	15*TICRATE + (TICRATE/2),	// As it was about to drain the rings...
	14*TICRATE,					// What Sonic, Tails, and Knuckles...
	18*TICRATE,					// About once every year, a strange...
	19*TICRATE + (TICRATE/2),	// Curses! Eggman yelled. That ridiculous...
	19*TICRATE + (TICRATE/4),	// It was only later that he had an idea...
	10*TICRATE + (TICRATE/2),	// Before beginning his scheme, Eggman decided to give Sonic...
	16*TICRATE,					// We're ready to fire in 15 seconds, the robot said...
	16*TICRATE,					// Meanwhile, Sonic was tearing across the zones...
	16*TICRATE + (TICRATE/2),	// Sonic knew he was getting closer to the city...
	17*TICRATE,					// Greenflower City was gone...
	16*TICRATE + (TICRATE/2),	// You're not quite as dead as we thought, huh?...
	18*TICRATE + (TICRATE/2),	// Eggman took this as his cue and blasted off...
	16*TICRATE,					// Easy! We go find Eggman and stop his...
	25*TICRATE,					// I'm just finding what mission obje...
};

// custom intros
void F_StartCustomCutscene(INT32 cutscenenum, boolean precutscene, boolean resetplayer);

void F_StartIntro(void)
{
	if (introtoplay)
	{
		if (!cutscenes[introtoplay - 1])
			D_StartTitle();
		else
			F_StartCustomCutscene(introtoplay - 1, false, false);
		return;
	}

	introtext[0] = " #";

	introtext[1] = M_GetText(
	"Two months had passed since Dr. Eggman\n"
	"tried to take over the world using his\n"
	"Ring Satellite.\n#");

	introtext[2] = M_GetText(
	"As it was about to drain the rings\n"
	"away from the planet, Sonic burst into\n"
	"the Satellite and for what he thought\n"
	"would be the last time,\xB4 defeated\n"
	"Dr. Eggman.\n#");

	introtext[3] = M_GetText(
	"\nWhat Sonic, Tails, and Knuckles had\n"
	"not anticipated was that Eggman would\n"
	"return,\xB8 bringing an all new threat.\n#");

	introtext[4] = M_GetText(
	"\xA8""About once every year, a strange asteroid\n"
	"hovers around the planet.\xBF It suddenly\n"
	"appears from nowhere, circles around, and\n"
	"\xB6- just as mysteriously as it arrives -\xB6\n"
	"vanishes after about two months.\xBF\n"
	"No one knows why it appears, or how.\n#");

	introtext[5] = M_GetText(
	"\xA7\"Curses!\"\xA9\xBA Eggman yelled. \xA7\"That hedgehog\n"
	"and his ridiculous friends will pay\n"
	"dearly for this!\"\xA9\xC8 Just then his scanner\n"
	"blipped as the Black Rock made its\n"
	"appearance from nowhere.\xBF Eggman looked at\n"
	"the screen, and just shrugged it off.\n#");

	introtext[6] = M_GetText(
	"It was only later\n"
	"that he had an\n"
	"idea. \xBF\xA7\"The Black\n"
	"Rock usually has a\n"
	"lot of energy\n"
	"within it\xAC...\xA7\xBF\n"
	"If I can somehow\n"
	"harness this,\xB8 I\n"
	"can turn it into\n"
	"the ultimate\n"
	"battle station\xAC...\xA7\xBF\n"
	"And every last\n"
	"person will be\n"
	"begging for mercy,\xB8\xA8\n"
	"including Sonic!\"\n#");

	introtext[7] = M_GetText(
	"\xA8\nBefore beginning his scheme,\n"
	"Eggman decided to give Sonic\n"
	"a reunion party...\n#");

	introtext[8] = M_GetText(
	"\xA5\"We're\xB6 ready\xB6 to\xB4 fire\xB6 in\xB6 15\xB6 seconds!\"\xA8\xB8\n"
	"The robot said, his voice crackling a\n"
	"little down the com-link. \xBF\xA7\"Good!\"\xA8\xB8\n"
	"Eggman sat back in his Egg-Mobile and\n"
	"began to count down as he saw the\n"
	"GreenFlower city on the main monitor.\n#");

	introtext[9] = M_GetText(
	"\xA5\"10...\xD2""9...\xD2""8...\"\xA8\xD2\n"
	"Meanwhile, Sonic was tearing across the\n"
	"zones. Everything became a blur as he\n"
	"ran around loops, skimmed over water,\n"
	"and catapulted himself off rocks with\n"
	"his phenomenal speed.\n#");

	introtext[10] = M_GetText(
	"\xA5\"6...\xD2""5...\xD2""4...\"\xA8\xD2\n"
	"Sonic knew he was getting closer to the\n"
	"City, and pushed himself harder.\xB4 Finally,\n"
	"the city appeared in the horizon.\xD2\xD2\n"
	"\xA5\"3...\xD2""2...\xD2""1...\xD2""Zero.\"\n#");

	introtext[11] = M_GetText(
	"GreenFlower City was gone.\xC4\n"
	"Sonic arrived just in time to see what\n"
	"little of the 'ruins' were left.\n"
	"Everyone and everything in the city\n"
	"had been obliterated.\n#");

	introtext[12] = M_GetText(
	"\xA7\"You're not quite as dead as we thought,\n"
	"huh?\xBF Are you going to tell us your plan as\n"
	"usual or will I \xA8\xB4'have to work it out'\xA7 or\n"
	"something?\"\xD2\xD2\n"
	"\"We'll see\xAA...\xA7\xBF let's give you a quick warm\n"
	"up, Sonic!\xA6\xC4 JETTYSYNS!\xA7\xBD Open fire!\"\n#");

	introtext[13] = M_GetText(
	"Eggman took this\n"
	"as his cue and\n"
	"blasted off,\n"
	"leaving Sonic\n"
	"and Tails behind.\xB6\n"
	"Tails looked at\n"
	"the ruins of the\n"
	"Greenflower City\n"
	"with a grim face\n"
	"and sighed.\xC6\n"
	"\xA7\"Now\xB6 what do we\n"
	"do?\",\xA9 he asked.\n#");

	introtext[14] = M_GetText(
	"\xA7\"Easy!\xBF We go\n"
	"find Eggman\n"
	"and stop his\n"
	"latest\n"
	"insane plan.\xBF\n"
	"Just like\n"
	"we've always\n"
	"done,\xBA right?\xD2\n\n"
	"\xAE...\xA9\xD2\n\n"
	"\"Tails, what\n"
	"\xAA*ARE*\xA9 you\n"
	"doing?\"\n#");

	introtext[15] = M_GetText(
	"\xA8\"I'm just finding what mission obje\xAC\xB1...\xBF\n"
	"\xA6""a-\xB8""ha!\xBF Here it is!\xA8\xBF This will only give us\n"
	"the robot's primary objective.\xBF It says\xAC\xB1...\"\n"
	"\xD2\xA3\x83"
	"* LOCATE  AND  RETRIEVE:  CHAOS  EMERALDS *"
	"\xBF\n"
	"*  CLOSEST  LOCATION:  GREENFLOWER  ZONE  *"
	"\x80\n\xA9\xD2\xD2"
	"\"All right, then\xAF... \xD2\xD2\xA7let's go!\"\n#");

/*
	"What are we waiting for? The first emerald is ours!" Sonic was about to
	run, when he saw a shadow pass over him, he recognized the silhouette
	instantly.
	"Knuckles!" Sonic said. The echidna stopped his glide and landed
	facing Sonic. "What are you doing here?"
	He replied, "This crisis affects the Floating Island,
	if that explosion I saw is anything to go by."
	If you're willing to help then... let's go!"
*/

	G_SetGamestate(GS_INTRO);
	gameaction = ga_nothing;
	playerdeadview = false;
	paused = false;
	CON_ToggleOff();
	CON_ClearHUD();
	F_NewCutscene(introtext[0]);

	intro_scenenum = 0;
	finalecount = animtimer = stoptimer = 0;
	roidtics = BASEVIDWIDTH - 64;
	timetonext = introscenetime[intro_scenenum];
}

//
// F_IntroDrawScene
//
static void F_IntroDrawScene(void)
{
	boolean highres = false;
	INT32 cx = 8, cy = 128;
	patch_t *background = NULL;
	INT32 bgxoffs = 0;
	void *patch;

	// DRAW A FULL PIC INSTEAD OF FLAT!
	if (intro_scenenum == 0);
	else if (intro_scenenum == 1)
		background = W_CachePatchName("INTRO1", PU_CACHE);
	else if (intro_scenenum == 2)
	{
		background = W_CachePatchName("INTRO2", PU_CACHE);
		highres = true;
	}
	else if (intro_scenenum == 3)
		background = W_CachePatchName("INTRO3", PU_CACHE);
	else if (intro_scenenum == 4)
		background = W_CachePatchName("INTRO4", PU_CACHE);
	else if (intro_scenenum == 5)
	{
		if (intro_curtime >= 5*TICRATE)
			background = W_CachePatchName("RADAR", PU_CACHE);
		else
		{
			background = W_CachePatchName("DRAT", PU_CACHE);
			highres = true;
		}
	}
	else if (intro_scenenum == 6)
	{
		background = W_CachePatchName("INTRO6", PU_CACHE);
		cx = 180;
		cy = 8;
	}
	else if (intro_scenenum == 7)
	{
		if (intro_curtime >= 6*TICRATE)
			background = W_CachePatchName("SGRASS5", PU_CACHE);
		else
			background = W_CachePatchName("SGRASS1", PU_CACHE);
	}
	else if (intro_scenenum == 8)
	{
		background = W_CachePatchName("WATCHING", PU_CACHE);
		highres = true;
	}
	else if (intro_scenenum == 9)
	{
		background = W_CachePatchName("ZOOMING", PU_CACHE);
		highres = true;
	}
	else if (intro_scenenum == 10);
	else if (intro_scenenum == 11)
		background = W_CachePatchName("INTRO5", PU_CACHE);
	else if (intro_scenenum == 12)
	{
		if (intro_curtime >= 7*TICRATE)
			background = W_CachePatchName("CONFRONT", PU_CACHE);
		else
			background = W_CachePatchName("REVENGE", PU_CACHE);
		highres = true;
	}
	else if (intro_scenenum == 13)
	{
		background = W_CachePatchName("TAILSSAD", PU_CACHE);
		highres = true;
		bgxoffs = 144;
		cx = 8;
		cy = 8;
	}
	else if (intro_scenenum == 14)
	{
		if (intro_curtime >= 7*TICRATE)
			background = W_CachePatchName("SONICDO2", PU_CACHE);
		else
			background = W_CachePatchName("SONICDO1", PU_CACHE);
		highres = true;
		cx = 224;
		cy = 8;
	}
	else if (intro_scenenum == 15)
	{
		background = W_CachePatchName("INTRO7", PU_CACHE);
		highres = true;
	}

	V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 31);

	if (background)
	{
		if (highres)
			V_DrawSmallScaledPatch(bgxoffs, 0, 0, background);
		else
			V_DrawScaledPatch(bgxoffs, 0, 0, background);
	}
	else if (intro_scenenum == 0) // STJr presents
	{
		// "Waaaaaaah" intro
		if (finalecount-TICRATE/2 < 4*TICRATE+23) {
			// aspect is FRACUNIT/2 for 4:3 (source) resolutions, smaller for 16:10 (SRB2) resolutions
			fixed_t aspect = (FRACUNIT + (FRACUNIT*4/3 - FRACUNIT*vid.width/vid.height)/2)>>1;
			fixed_t x,y;
			V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 2);
			if (finalecount < 30) { // Cry!
				if (finalecount < 4)
					S_StopMusic();
				if (finalecount == 4)
					S_ChangeMusicInternal("stjr", false);
				x = (BASEVIDWIDTH<<FRACBITS)/2 - FixedMul(334<<FRACBITS, aspect)/2;
				y = (BASEVIDHEIGHT<<FRACBITS)/2 - FixedMul(358<<FRACBITS, aspect)/2;
				V_DrawSciencePatch(x, y, 0, (patch = W_CachePatchName("WAHH1", PU_CACHE)), aspect);
				W_UnlockCachedPatch(patch);
				if (finalecount > 6) {
					V_DrawSciencePatch(x, y, 0, (patch = W_CachePatchName("WAHH2", PU_CACHE)), aspect);
					W_UnlockCachedPatch(patch);
				}
				if (finalecount > 10) {
					V_DrawSciencePatch(x, y, 0, (patch = W_CachePatchName("WAHH3", PU_CACHE)), aspect);
					W_UnlockCachedPatch(patch);
				}
				if (finalecount > 14) {
					V_DrawSciencePatch(x, y, 0, (patch = W_CachePatchName("WAHH4", PU_CACHE)), aspect);
					W_UnlockCachedPatch(patch);
				}
			}
			else if (finalecount-30 < 20) { // Big eggy
				background = W_CachePatchName("FEEDIN", PU_CACHE);
				x = (BASEVIDWIDTH<<FRACBITS)/2 - FixedMul(560<<FRACBITS, aspect)/2;
				y = (BASEVIDHEIGHT<<FRACBITS) - FixedMul(477<<FRACBITS, aspect);
				V_DrawSciencePatch(x, y, V_SNAPTOBOTTOM, background, aspect);
			}
			else if (finalecount-50 < 30) { // Zoom out
				fixed_t scale = FixedDiv(aspect, FixedDiv((finalecount-50)<<FRACBITS, (15<<FRACBITS))+FRACUNIT);
				background = W_CachePatchName("FEEDIN", PU_CACHE);
				x = (BASEVIDWIDTH<<FRACBITS)/2 - FixedMul(560<<FRACBITS, aspect)/2 + (FixedMul(560<<FRACBITS, aspect) - FixedMul(560<<FRACBITS, scale));
				y = (BASEVIDHEIGHT<<FRACBITS) - FixedMul(477<<FRACBITS, scale);
				V_DrawSciencePatch(x, y, V_SNAPTOBOTTOM, background, scale);
			}
			else
			{
				{
					// Draw tiny eggy
					fixed_t scale = FixedMul(FRACUNIT/3, aspect);
					background = W_CachePatchName("FEEDIN", PU_CACHE);
					x = (BASEVIDWIDTH<<FRACBITS)/2 - FixedMul(560<<FRACBITS, aspect)/2 + (FixedMul(560<<FRACBITS, aspect) - FixedMul(560<<FRACBITS, scale));
					y = (BASEVIDHEIGHT<<FRACBITS) - FixedMul(477<<FRACBITS, scale);
					V_DrawSciencePatch(x, y, V_SNAPTOBOTTOM, background, scale);
				}

				if (finalecount-84 < 58) { // Pure Fat is driving up!
					int ftime = (finalecount-84);
					x = (-189*FRACUNIT) + (FixedMul((6<<FRACBITS)+FRACUNIT/3, ftime<<FRACBITS) - FixedMul((6<<FRACBITS)+FRACUNIT/3, FixedDiv(FixedMul(ftime<<FRACBITS, ftime<<FRACBITS), 120<<FRACBITS)));
					y = (BASEVIDHEIGHT<<FRACBITS) - FixedMul(417<<FRACBITS, aspect);
					// Draw the body
					V_DrawSciencePatch(x, y, V_SNAPTOLEFT|V_SNAPTOBOTTOM, (patch = W_CachePatchName("PUREFAT1", PU_CACHE)), aspect);
					W_UnlockCachedPatch(patch);
					// Draw the door
					V_DrawSciencePatch(x+FixedMul(344<<FRACBITS, aspect), y+FixedMul(292<<FRACBITS, aspect), V_SNAPTOLEFT|V_SNAPTOBOTTOM, (patch = W_CachePatchName("PUREFAT2", PU_CACHE)), aspect);
					W_UnlockCachedPatch(patch);
					// Draw the wheel
					V_DrawSciencePatch(x+FixedMul(178<<FRACBITS, aspect), y+FixedMul(344<<FRACBITS, aspect), V_SNAPTOLEFT|V_SNAPTOBOTTOM, (patch = W_CachePatchName(va("TYRE%02u",(abs(finalecount-144)/3)%16), PU_CACHE)), aspect);
					W_UnlockCachedPatch(patch);
					// Draw the wheel cover
					V_DrawSciencePatch(x+FixedMul(88<<FRACBITS, aspect), y+FixedMul(238<<FRACBITS, aspect), V_SNAPTOLEFT|V_SNAPTOBOTTOM, (patch = W_CachePatchName("PUREFAT3", PU_CACHE)), aspect);
					W_UnlockCachedPatch(patch);
				} else { // Pure Fat has stopped!
					y = (BASEVIDHEIGHT<<FRACBITS) - FixedMul(417<<FRACBITS, aspect);
					// Draw the body
					V_DrawSciencePatch(0, y, V_SNAPTOLEFT|V_SNAPTOBOTTOM, (patch = W_CachePatchName("PUREFAT1", PU_CACHE)), aspect);
					W_UnlockCachedPatch(patch);
					// Draw the wheel
					V_DrawSciencePatch(FixedMul(178<<FRACBITS, aspect), y+FixedMul(344<<FRACBITS, aspect), V_SNAPTOLEFT|V_SNAPTOBOTTOM, (patch = W_CachePatchName("TYRE00", PU_CACHE)), aspect);
					W_UnlockCachedPatch(patch);
					// Draw the wheel cover
					V_DrawSciencePatch(FixedMul(88<<FRACBITS, aspect), y+FixedMul(238<<FRACBITS, aspect), V_SNAPTOLEFT|V_SNAPTOBOTTOM, (patch = W_CachePatchName("PUREFAT3", PU_CACHE)), aspect);
					W_UnlockCachedPatch(patch);
					// Draw the door
					if (finalecount-TICRATE/2 > 4*TICRATE) { // Door is being raised!
						int ftime = (finalecount-TICRATE/2-4*TICRATE);
						y -= FixedDiv((ftime*ftime)<<FRACBITS, 23<<FRACBITS);
					}
					V_DrawSciencePatch(FixedMul(344<<FRACBITS, aspect), y+FixedMul(292<<FRACBITS, aspect), V_SNAPTOLEFT|V_SNAPTOBOTTOM, (patch = W_CachePatchName("PUREFAT2", PU_CACHE)), aspect);
					W_UnlockCachedPatch(patch);
				}
			}
		} else {
			V_DrawCreditString((160 - V_CreditStringWidth("SONIC TEAM JR")/2)<<FRACBITS, 80<<FRACBITS, 0, "SONIC TEAM JR");
			V_DrawCreditString((160 - V_CreditStringWidth("PRESENTS")/2)<<FRACBITS, 96<<FRACBITS, 0, "PRESENTS");
		}
	}
	else if (intro_scenenum == 10) // Sky Runner
	{
		if (timetonext > 5*TICRATE && timetonext < 6*TICRATE)
		{
			if (!(finalecount & 3))
				background = W_CachePatchName("BRITEGG1", PU_CACHE);
			else
				background = W_CachePatchName("DARKEGG1", PU_CACHE);

			V_DrawScaledPatch(0, 0, 0, background);
		}
		else if (timetonext > 3*TICRATE && timetonext < 4*TICRATE)
		{
			if (!(finalecount & 3))
				background = W_CachePatchName("BRITEGG2", PU_CACHE);
			else
				background = W_CachePatchName("DARKEGG2", PU_CACHE);

			V_DrawScaledPatch(0, 0, 0, background);
		}
		else if (timetonext > 1*TICRATE && timetonext < 2*TICRATE)
		{
			if (!(finalecount & 3))
				background = W_CachePatchName("BRITEGG3", PU_CACHE);
			else
				background = W_CachePatchName("DARKEGG3", PU_CACHE);

			V_DrawScaledPatch(0, 0, 0, background);
		}
		else
		{
			F_SkyScroll(80*4);
			if (timetonext == 6)
			{
				stoptimer = finalecount;
				animtimer = finalecount % 16;
			}
			else if (timetonext >= 0 && timetonext < 6)
			{
				animtimer = stoptimer;
				deplete -= 32;
			}
			else
			{
				animtimer = finalecount % 16;
				deplete = 160;
			}

			if (finalecount & 1)
			{
				V_DrawScaledPatch(deplete, 8, 0, (patch = W_CachePatchName("RUN2", PU_CACHE)));
				W_UnlockCachedPatch(patch);
				V_DrawScaledPatch(deplete, 72, 0, (patch = W_CachePatchName("PEELOUT2", PU_CACHE)));
				W_UnlockCachedPatch(patch);
			}
			else
			{
				V_DrawScaledPatch(deplete, 8, 0, (patch = W_CachePatchName("RUN1", PU_CACHE)));
				W_UnlockCachedPatch(patch);
				V_DrawScaledPatch(deplete, 72, 0, (patch = W_CachePatchName("PEELOUT1", PU_CACHE)));
				W_UnlockCachedPatch(patch);
			}

			{ // Fixing up the black box rendering to look right in resolutions <16:10 -Red
				INT32 y = 112;
				INT32 h = BASEVIDHEIGHT - 112;
				if (vid.height != BASEVIDHEIGHT * vid.dupy)
				{
					INT32 adjust = (vid.height/vid.dupy)-200;
					adjust /= 2;
					y += adjust;
					h += adjust;
					V_DrawFill(0, 0, BASEVIDWIDTH, adjust, 31); // Render a black bar on top so it keeps the "cinematic" windowboxing... I just prefer it this way. -Red
				}
				V_DrawFill(0, y, BASEVIDWIDTH, h, 31);
			}
		}
	}

	W_UnlockCachedPatch(background);

	if (intro_scenenum == 4) // The asteroid SPINS!
	{
		if (roidtics >= 0)
		{
			V_DrawScaledPatch(roidtics, 24, 0,
				(patch = W_CachePatchName(va("ROID00%.2d", intro_curtime%35), PU_CACHE)));
			W_UnlockCachedPatch(patch);
		}
	}

	if (animtimer)
		animtimer--;

	if (intro_scenenum == 7 && intro_curtime > 7*TICRATE)
	{
		patch_t *sgrass;

		if (intro_curtime >= 7*TICRATE + ((TICRATE/7)*2))
			sgrass = W_CachePatchName("SGRASS4", PU_CACHE);
		else if (intro_curtime >= 7*TICRATE + (TICRATE/7))
			sgrass = W_CachePatchName("SGRASS3", PU_CACHE);
		else
			sgrass = W_CachePatchName("SGRASS2", PU_CACHE);
		V_DrawScaledPatch(123, 4, 0, sgrass);

		W_UnlockCachedPatch(sgrass);
	}

	V_DrawString(cx, cy, 0, cutscene_disptext);
}

//
// F_IntroDrawer
//
void F_IntroDrawer(void)
{
	if (timetonext <= 0)
	{
		if (intro_scenenum == 0)
		{
			if (rendermode != render_none)
			{
				F_WipeStartScreen();
				V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 31);
				F_WipeEndScreen();
				F_RunWipe(99,true);
			}

			S_ChangeMusicInternal("read_m", false);
		}
		else if (intro_scenenum == 3)
			roidtics = BASEVIDWIDTH - 64;
		else if (intro_scenenum == 10)
		{
			// The only fade to white in the entire damn game.
			if (rendermode != render_none)
			{
				F_WipeStartScreen();
				V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 0);
				F_WipeEndScreen();
				F_RunWipe(99,true);
			}
		}
		else if (intro_scenenum == 15)
		{
			if (rendermode != render_none)
			{
				F_WipeStartScreen();
				V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 31);
				F_WipeEndScreen();
				F_RunWipe(99,true);
			}

			// Stay on black for a bit. =)
			{
				tic_t quittime;
				quittime = I_GetTime() + NEWTICRATE*2; // Shortened the quit time, used to be 2 seconds
				while (quittime > I_GetTime())
				{
					I_OsPolling();
					I_UpdateNoBlit();
					M_Drawer(); // menu is drawn even on top of wipes
					I_FinishUpdate(); // Update the screen with the image Tails 06-19-2001
				}
			}

			D_StartTitle();
			return;
		}
		F_NewCutscene(introtext[++intro_scenenum]);
		timetonext = introscenetime[intro_scenenum];

		F_WipeStartScreen();
		wipegamestate = -1;
		animtimer = stoptimer = 0;
	}

	intro_curtime = introscenetime[intro_scenenum] - timetonext;

	if (rendermode != render_none)
	{
		if (intro_scenenum == 5 && intro_curtime == 5*TICRATE)
		{
			patch_t *radar = W_CachePatchName("RADAR", PU_CACHE);

			F_WipeStartScreen();
			V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 31);
			V_DrawScaledPatch(0, 0, 0, radar);
			W_UnlockCachedPatch(radar);
			V_DrawString(8, 128, 0, cutscene_disptext);

			F_WipeEndScreen();
			F_RunWipe(99,true);
		}
		else if (intro_scenenum == 7 && intro_curtime == 6*TICRATE) // Force a wipe here
		{
			patch_t *grass = W_CachePatchName("SGRASS5", PU_CACHE);

			F_WipeStartScreen();
			V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 31);
			V_DrawScaledPatch(0, 0, 0, grass);
			W_UnlockCachedPatch(grass);
			V_DrawString(8, 128, 0, cutscene_disptext);

			F_WipeEndScreen();
			F_RunWipe(99,true);
		}
		else if (intro_scenenum == 12 && intro_curtime == 7*TICRATE)
		{
			patch_t *confront = W_CachePatchName("CONFRONT", PU_CACHE);

			F_WipeStartScreen();
			V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 31);
			V_DrawSmallScaledPatch(0, 0, 0, confront);
			W_UnlockCachedPatch(confront);
			V_DrawString(8, 128, 0, cutscene_disptext);

			F_WipeEndScreen();
			F_RunWipe(99,true);
		}
		if (intro_scenenum == 14 && intro_curtime == 7*TICRATE)
		{
			patch_t *sdo = W_CachePatchName("SONICDO2", PU_CACHE);

			F_WipeStartScreen();
			V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 31);
			V_DrawSmallScaledPatch(0, 0, 0, sdo);
			W_UnlockCachedPatch(sdo);
			V_DrawString(224, 8, 0, cutscene_disptext);

			F_WipeEndScreen();
			F_RunWipe(99,true);
		}
	}

	F_IntroDrawScene();
}

//
// F_IntroTicker
//
void F_IntroTicker(void)
{
	// advance animation
	finalecount++;

	if (finalecount % 3 == 0)
		roidtics--;

	timetonext--;

	F_WriteText();

	// check for skipping
	if (keypressed)
		keypressed = false;
}

//
// F_IntroResponder
//
boolean F_IntroResponder(event_t *event)
{
	INT32 key = event->data1;

	// remap virtual keys (mouse & joystick buttons)
	switch (key)
	{
		case KEY_MOUSE1:
			key = KEY_ENTER;
			break;
		case KEY_MOUSE1 + 1:
			key = KEY_BACKSPACE;
			break;
		case KEY_JOY1:
		case KEY_JOY1 + 2:
			key = KEY_ENTER;
			break;
		case KEY_JOY1 + 3:
			key = 'n';
			break;
		case KEY_JOY1 + 1:
			key = KEY_BACKSPACE;
			break;
		case KEY_HAT1:
			key = KEY_UPARROW;
			break;
		case KEY_HAT1 + 1:
			key = KEY_DOWNARROW;
			break;
		case KEY_HAT1 + 2:
			key = KEY_LEFTARROW;
			break;
		case KEY_HAT1 + 3:
			key = KEY_RIGHTARROW;
			break;
	}

	if (event->type != ev_keydown && key != 301)
		return false;

	if (key != 27 && key != KEY_ENTER && key != KEY_SPACE && key != KEY_BACKSPACE)
		return false;

	if (keypressed)
		return false;

	keypressed = true;
	return true;
}

// =========
//  CREDITS
// =========
static const char *credits[] = {
	"\1Sonic Robo Blast II",
	"\1Credits",
	"",
	"\1Game Design",
	"Ben \"Mystic\" Geyer",
	"\"SSNTails\"",
	"Johnny \"Sonikku\" Wallbank",
	"",
	"\1Programming",
	"Alam \"GBC\" Arias",
	"Logan \"GBA\" Arias",
	"Callum Dickinson",
	"Scott \"Graue\" Feeney",
	"Nathan \"Jazz\" Giroux",
	"Thomas \"Shadow Hog\" Igoe",
	"Iestyn \"Monster Iestyn\" Jealous",
	"Ronald \"Furyhunter\" Kinard", // The SDL2 port
	"John \"JTE\" Muniz",
	"Ehab \"Wolfy\" Saeed",
	"\"Kaito Sinclaire\"",
	"\"SSNTails\"",
	"",
	"\1Programming",
	"\1Assistance",
	"\"chi.miru\"", // helped port slope drawing code from ZDoom
	"Andrew \"orospakr\" Clunis",
	"Gregor \"Oogaland\" Dick",
	"Louis-Antoine \"LJSonic\" de Moulins", // for fixing 2.1's netcode (de Rochefort doesn't quite fit on the screen sorry lol)
	"Vivian \"toaster\" Grannell",
	"Julio \"Chaos Zero 64\" Guir",
	"\"Kalaron\"", // Coded some of Sryder13's collection of OpenGL fixes, especially fog
	"Matthew \"Shuffle\" Marsalko",
	"Steven \"StroggOnMeth\" McGranahan",
	"\"Morph\"", // For SRB2Morphed stuff
	"Colin \"Sonict\" Pfaff",
	"Sean \"Sryder13\" Ryder",
	"Ben \"Cue\" Woodford",
	"",
	"\1Sprite Artists",
	"Odi \"Iceman404\" Atunzu",
	"Victor \"VAdaPEGA\" Ara\x1Fjo", // Araújo -- sorry for our limited font! D:
	"Jim \"MotorRoach\" DeMello",
	"Desmond \"Blade\" DesJardins",
	"Sherman \"CoatRack\" DesJardins",
	"Andrew \"Senku Niola\" Moran",
	"David \"Instant Sonic\" Spencer Jr.",
	"\"SSNTails\"",
	"",
	"\1Texture Artists",
	"Ryan \"Blaze Hedgehog\" Bloom",
	"Buddy \"KinkaJoy\" Fischer",
	"Vivian \"toaster\" Grannell",
	"Kepa \"Nev3r\" Iceta",
	"Jarrett \"JEV3\" Voight",
	"",
	"\1Music and Sound",
	"\1Production",
	"Malcolm \"RedXVI\" Brown",
	"Dave \"DemonTomatoDave\" Bulmer",
	"Paul \"Boinciel\" Clempson",
	"Cyan Helkaraxe",
	"Kepa \"Nev3r\" Iceta",
	"Iestyn \"Monster Iestyn\" Jealous",
	"Jarel \"Arrow\" Jones",
	"Stefan \"Stuf\" Rimalia",
	"Shane Mychal Sexton",
	"\"Spazzo\"",
	"David \"Big Wave Dave\" Spencer Sr.",
	"David \"Instant Sonic\" Spencer Jr.",
	"\"SSNTails\"",
	"",
	"\1Level Design",
	"Matthew \"Fawfulfan\" Chapman",
	"Paul \"Boinciel\" Clempson",
	"Desmond \"Blade\" DesJardins",
	"Sherman \"CoatRack\" DesJardins",
	"Ben \"Mystic\" Geyer",
	"Nathan \"Jazz\" Giroux",
	"Dan \"Blitzzo\" Hagerstrand",
	"Kepa \"Nev3r\" Iceta",
	"Thomas \"Shadow Hog\" Igoe",
	"Erik \"Torgo\" Nielsen",
	"\"Kaito Sinclaire\"",
	"Wessel \"Spherallic\" Smit",
	"\"Spazzo\"",
	"\"SSNTails\"",
	"Rob Tisdell",
	"Jarrett \"JEV3\" Voight",
	"Johnny \"Sonikku\" Wallbank",
	"Marco \"Digiku\" Zafra",
	"",
	"\1Boss Design",
	"Ben \"Mystic\" Geyer",
	"Thomas \"Shadow Hog\" Igoe",
	"John \"JTE\" Muniz",
	"Samuel \"Prime 2.0\" Peters",
	"\"SSNTails\"",
	"Johnny \"Sonikku\" Wallbank",
	"",
	"\1Testing",
	"Hank \"FuriousFox\" Brannock",
	"Cody \"SRB2 Playah\" Koester",
	"Skye \"OmegaVelocity\" Meredith",
	"Stephen \"HEDGESMFG\" Moellering",
	"Nick \"ST218\" Molina",
	"Samuel \"Prime 2.0\" Peters",
	"Colin \"Sonict\" Pfaff",
	"Bill \"Tets\" Reed",
	"",
	"\1Special Thanks",
	"Doom Legacy Project",
	"iD Software",
	"Alex \"MistaED\" Fuller",
	"FreeDoom Project", // Used some of the mancubus and rocket launcher sprites for Brak
	"Randi Heit (<!>)", // For their MSPaint <!> sprite that we nicked
	"",
	"\1Produced By",
	"Sonic Team Junior",
	"",
	"\1Published By",
	"A 28K dialup modem",
	"",
	"\1Thank you",
	"\1for playing!",
	NULL
};

static struct {
	UINT32 x, y;
	const char *patch;
} credits_pics[] = {
	{  8, 80+200* 1, "CREDIT01"},
	{  4, 80+200* 2, "CREDIT13"},
	{250, 80+200* 3, "CREDIT12"},
	{  8, 80+200* 4, "CREDIT03"},
	{248, 80+200* 5, "CREDIT11"},
	{  8, 80+200* 6, "CREDIT04"},
	{112, 80+200* 7, "CREDIT10"},
	{240, 80+200* 8, "CREDIT05"},
	{120, 80+200* 9, "CREDIT06"},
	{  8, 80+200*10, "CREDIT07"},
	{  8, 80+200*11, "CREDIT08"},
	{112, 80+200*12, "CREDIT09"},
	{0, 0, NULL}
};

void F_StartCredits(void)
{
	G_SetGamestate(GS_CREDITS);

	// Just in case they're open ... somehow
	M_ClearMenus(true);

	// Save the second we enter the credits
	if ((!modifiedgame || savemoddata) && !(netgame || multiplayer) && cursaveslot >= 0)
		G_SaveGame((UINT32)cursaveslot);

	if (creditscutscene)
	{
		F_StartCustomCutscene(creditscutscene - 1, false, false);
		return;
	}

	gameaction = ga_nothing;
	playerdeadview = false;
	paused = false;
	CON_ToggleOff();
	CON_ClearHUD();
	S_StopMusic();

	S_ChangeMusicInternal("credit", false);

	finalecount = 0;
	animtimer = 0;
	timetonext = 2*TICRATE;
}

void F_CreditDrawer(void)
{
	UINT16 i;
	fixed_t y = (80<<FRACBITS) - 5*(animtimer<<FRACBITS)/8;

	V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 31);

	// Draw background pictures first
	for (i = 0; credits_pics[i].patch; i++)
		V_DrawSciencePatch(credits_pics[i].x<<FRACBITS, (credits_pics[i].y<<FRACBITS) - 4*(animtimer<<FRACBITS)/5, 0, W_CachePatchName(credits_pics[i].patch, PU_CACHE), FRACUNIT>>1);

	// Draw credits text on top
	for (i = 0; credits[i]; i++)
	{
		switch(credits[i][0])
		{
		case 0:
			y += 80<<FRACBITS;
			break;
		case 1:
			if (y>>FRACBITS > -20)
				V_DrawCreditString((160 - (V_CreditStringWidth(&credits[i][1])>>1))<<FRACBITS, y, 0, &credits[i][1]);
			y += 30<<FRACBITS;
			break;
		default:
			if (y>>FRACBITS > -10)
				V_DrawStringAtFixed(32<<FRACBITS, y, V_ALLOWLOWERCASE, credits[i]);
			y += 12<<FRACBITS;
			break;
		}
		if (FixedMul(y,vid.dupy) > vid.height)
			break;
	}

	if (!credits[i] && y <= 120<<FRACBITS && !finalecount)
	{
		timetonext = 5*TICRATE+1;
		finalecount = 5*TICRATE;
	}
}

void F_CreditTicker(void)
{
	if (timetonext)
		timetonext--;
	else
		animtimer++;

	if (finalecount && --finalecount == 0)
		F_StartGameEvaluation();
}

boolean F_CreditResponder(event_t *event)
{
	INT32 key = event->data1;

	// remap virtual keys (mouse & joystick buttons)
	switch (key)
	{
		case KEY_MOUSE1:
			key = KEY_ENTER;
			break;
		case KEY_MOUSE1 + 1:
			key = KEY_BACKSPACE;
			break;
		case KEY_JOY1:
		case KEY_JOY1 + 2:
			key = KEY_ENTER;
			break;
		case KEY_JOY1 + 3:
			key = 'n';
			break;
		case KEY_JOY1 + 1:
			key = KEY_BACKSPACE;
			break;
		case KEY_HAT1:
			key = KEY_UPARROW;
			break;
		case KEY_HAT1 + 1:
			key = KEY_DOWNARROW;
			break;
		case KEY_HAT1 + 2:
			key = KEY_LEFTARROW;
			break;
		case KEY_HAT1 + 3:
			key = KEY_RIGHTARROW;
			break;
	}

	if (!(timesBeaten) && !(netgame || multiplayer))
		return false;

	if (event->type != ev_keydown)
		return false;

	if (key != KEY_ESCAPE && key != KEY_ENTER && key != KEY_SPACE && key != KEY_BACKSPACE)
		return false;

	if (keypressed)
		return true;

	keypressed = true;
	return true;
}

// ============
//  EVALUATION
// ============
#define INTERVAL 50
#define TRANSLEVEL V_80TRANS
static INT32 eemeralds_start;
static boolean drawemblem = false, drawchaosemblem = false;

void F_StartGameEvaluation(void)
{
	// Credits option in secrets menu
	if (cursaveslot == -2)
	{
		F_StartGameEnd();
		return;
	}

	G_SetGamestate(GS_EVALUATION);

	// Just in case they're open ... somehow
	M_ClearMenus(true);

	// Save the second we enter the evaluation
	// We need to do this again!  Remember, it's possible a mod designed skipped
	// the credits sequence!
	if ((!modifiedgame || savemoddata) && !(netgame || multiplayer) && cursaveslot >= 0)
		G_SaveGame((UINT32)cursaveslot);

	gameaction = ga_nothing;
	playerdeadview = false;
	paused = false;
	CON_ToggleOff();
	CON_ClearHUD();

	finalecount = 0;
}

void F_GameEvaluationDrawer(void)
{
	INT32 x, y, i;
	const fixed_t radius = 48*FRACUNIT;
	angle_t fa;
	INT32 eemeralds_cur;
	char patchname[7] = "CEMGx0";

	V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 31);

	// Draw all the good crap here.
	if (ALL7EMERALDS(emeralds))
		V_DrawString(114, 16, 0, "GOT THEM ALL!");
	else
		V_DrawString(124, 16, 0, "TRY AGAIN!");

	eemeralds_start++;
	eemeralds_cur = eemeralds_start;

	for (i = 0; i < 7; ++i)
	{
		fa = (FixedAngle(eemeralds_cur*FRACUNIT)>>ANGLETOFINESHIFT) & FINEMASK;
		x = 160 + FixedInt(FixedMul(FINECOSINE(fa),radius));
		y = 100 + FixedInt(FixedMul(FINESINE(fa),radius));

		patchname[4] = 'A'+(char)i;
		if (emeralds & (1<<i))
			V_DrawScaledPatch(x, y, 0, W_CachePatchName(patchname, PU_CACHE));
		else
			V_DrawTranslucentPatch(x, y, TRANSLEVEL, W_CachePatchName(patchname, PU_CACHE));

		eemeralds_cur += INTERVAL;
	}
	if (eemeralds_start >= 360)
		eemeralds_start -= 360;

	if (finalecount == 5*TICRATE)
	{
		if ((!modifiedgame || savemoddata) && !(netgame || multiplayer))
		{
			++timesBeaten;

			if (ALL7EMERALDS(emeralds))
				++timesBeatenWithEmeralds;

			if (ultimatemode)
				++timesBeatenUltimate;

			if (M_UpdateUnlockablesAndExtraEmblems())
				S_StartSound(NULL, sfx_ncitem);

			G_SaveGameData();
		}
	}

	if (finalecount >= 5*TICRATE)
	{
		if (drawemblem)
			V_DrawScaledPatch(120, 192, 0, W_CachePatchName("NWNGA0", PU_CACHE));

		if (drawchaosemblem)
			V_DrawScaledPatch(200, 192, 0, W_CachePatchName("NWNGA0", PU_CACHE));

		V_DrawString(8, 16, V_YELLOWMAP, "Unlocked:");

		if (!(netgame) && (!modifiedgame || savemoddata))
		{
			INT32 startcoord = 32;

			for (i = 0; i < MAXUNLOCKABLES; i++)
			{
				if (unlockables[i].conditionset && unlockables[i].conditionset < MAXCONDITIONSETS
					&& unlockables[i].type && !unlockables[i].nocecho)
				{
					if (unlockables[i].unlocked)
						V_DrawString(8, startcoord, 0, unlockables[i].name);
					startcoord += 8;
				}
			}
		}
		else if (netgame)
			V_DrawString(8, 96, V_YELLOWMAP, "Prizes only\nawarded in\nsingle player!");
		else
			V_DrawString(8, 96, V_YELLOWMAP, "Prizes not\nawarded in\nmodified games!");
	}
}

void F_GameEvaluationTicker(void)
{
	finalecount++;

	if (finalecount > 10*TICRATE)
		F_StartGameEnd();
}

// ==========
//  GAME END
// ==========
void F_StartGameEnd(void)
{
	G_SetGamestate(GS_GAMEEND);

	gameaction = ga_nothing;
	playerdeadview = false;
	paused = false;
	CON_ToggleOff();
	CON_ClearHUD();
	S_StopMusic();

	// In case menus are still up?!!
	M_ClearMenus(true);

	timetonext = TICRATE;
}

//
// F_GameEndDrawer
//
void F_GameEndDrawer(void)
{
}

//
// F_GameEndTicker
//
void F_GameEndTicker(void)
{
	if (timetonext > 0)
		timetonext--;
	else
		D_StartTitle();
}


// ==============
//  TITLE SCREEN
// ==============
void F_StartTitleScreen(void)
{
	if (gamestate != GS_TITLESCREEN && gamestate != GS_WAITINGPLAYERS)
		finalecount = 0;
	else
		wipegamestate = GS_TITLESCREEN;
	G_SetGamestate(GS_TITLESCREEN);
	CON_ClearHUD();

	// IWAD dependent stuff.

	S_ChangeMusicInternal("titles", looptitle);

	animtimer = 0;

	demoDelayLeft = demoDelayTime;
	demoIdleLeft = demoIdleTime;

	ttbanner = W_CachePatchName("TTBANNER", PU_LEVEL);
	ttwing = W_CachePatchName("TTWING", PU_LEVEL);
	ttsonic = W_CachePatchName("TTSONIC", PU_LEVEL);
	ttswave1 = W_CachePatchName("TTSWAVE1", PU_LEVEL);
	ttswave2 = W_CachePatchName("TTSWAVE2", PU_LEVEL);
	ttswip1 = W_CachePatchName("TTSWIP1", PU_LEVEL);
	ttsprep1 = W_CachePatchName("TTSPREP1", PU_LEVEL);
	ttsprep2 = W_CachePatchName("TTSPREP2", PU_LEVEL);
	ttspop1 = W_CachePatchName("TTSPOP1", PU_LEVEL);
	ttspop2 = W_CachePatchName("TTSPOP2", PU_LEVEL);
	ttspop3 = W_CachePatchName("TTSPOP3", PU_LEVEL);
	ttspop4 = W_CachePatchName("TTSPOP4", PU_LEVEL);
	ttspop5 = W_CachePatchName("TTSPOP5", PU_LEVEL);
	ttspop6 = W_CachePatchName("TTSPOP6", PU_LEVEL);
	ttspop7 = W_CachePatchName("TTSPOP7", PU_LEVEL);
}

// (no longer) De-Demo'd Title Screen
void F_TitleScreenDrawer(void)
{
	if (modeattacking)
		return; // We likely came here from retrying. Don't do a damn thing.

	// Draw that sky!
	F_SkyScroll(titlescrollspeed);

	// Don't draw outside of the title screewn, or if the patch isn't there.
	if (!ttwing || (gamestate != GS_TITLESCREEN && gamestate != GS_WAITINGPLAYERS))
		return;

	V_DrawScaledPatch(30, 14, 0, ttwing);

	if (finalecount < 57)
	{
		if (finalecount == 35)
			V_DrawScaledPatch(115, 15, 0, ttspop1);
		else if (finalecount == 36)
			V_DrawScaledPatch(114, 15, 0,ttspop2);
		else if (finalecount == 37)
			V_DrawScaledPatch(113, 15, 0,ttspop3);
		else if (finalecount == 38)
			V_DrawScaledPatch(112, 15, 0,ttspop4);
		else if (finalecount == 39)
			V_DrawScaledPatch(111, 15, 0,ttspop5);
		else if (finalecount == 40)
			V_DrawScaledPatch(110, 15, 0, ttspop6);
		else if (finalecount >= 41 && finalecount <= 44)
			V_DrawScaledPatch(109, 15, 0, ttspop7);
		else if (finalecount >= 45 && finalecount <= 48)
			V_DrawScaledPatch(108, 12, 0, ttsprep1);
		else if (finalecount >= 49 && finalecount <= 52)
			V_DrawScaledPatch(107, 9, 0, ttsprep2);
		else if (finalecount >= 53 && finalecount <= 56)
			V_DrawScaledPatch(106, 6, 0, ttswip1);
		V_DrawScaledPatch(93, 106, 0, ttsonic);
	}
	else
	{
		V_DrawScaledPatch(93, 106, 0,ttsonic);
		if (finalecount/5 & 1)
			V_DrawScaledPatch(100, 3, 0,ttswave1);
		else
			V_DrawScaledPatch(100,3, 0,ttswave2);
	}

	V_DrawScaledPatch(48, 142, 0,ttbanner);
}

// (no longer) De-Demo'd Title Screen
void F_TitleScreenTicker(boolean run)
{
	if (run)
		finalecount++;

	// don't trigger if doing anything besides idling on title
	if (gameaction != ga_nothing || gamestate != GS_TITLESCREEN)
		return;

	// no demos to play? or, are they disabled?
	if (!cv_rollingdemos.value || !numDemos)
		return;

	// Wait for a while (for the music to finish, preferably)
	// before starting demos
	if (demoDelayLeft)
	{
		--demoDelayLeft;
		return;
	}

	// Hold up for a bit if menu or console active
	if (menuactive || CON_Ready())
	{
		demoIdleLeft = demoIdleTime;
		return;
	}

	// is it time?
	if (!(--demoIdleLeft))
	{
		char dname[9];
		lumpnum_t l;

		// prevent console spam if failed
		demoIdleLeft = demoIdleTime;

		// Replay intro when done cycling through demos
		if (curDemo == numDemos)
		{
			curDemo = 0;
			F_StartIntro();
			return;
		}

		// Setup demo name
		snprintf(dname, 9, "DEMO_%03u", ++curDemo);

		if ((l = W_CheckNumForName(dname)) == LUMPERROR)
		{
			CONS_Alert(CONS_ERROR, M_GetText("Demo lump \"%s\" doesn't exist\n"), dname);
			F_StartIntro();
			return;
		}

		titledemo = true;
		G_DoPlayDemo(dname);
	}
}

void F_TitleDemoTicker(void)
{
	keypressed = false;
}

// ==========
//  CONTINUE
// ==========
void F_StartContinue(void)
{
	I_Assert(!netgame && !multiplayer);

	if (players[consoleplayer].continues <= 0)
	{
		Command_ExitGame_f();
		return;
	}

	G_SetGamestate(GS_CONTINUING);
	gameaction = ga_nothing;

	keypressed = false;
	playerdeadview = false;
	paused = false;
	CON_ToggleOff();
	CON_ClearHUD();

	// In case menus are still up?!!
	M_ClearMenus(true);

	S_ChangeMusicInternal("contsc", false);
	S_StopSounds();

	timetonext = TICRATE*11;
}

//
// F_ContinueDrawer
// Moved continuing out of the HUD (hack removal!!)
//
void F_ContinueDrawer(void)
{
	patch_t *contsonic;
	INT32 i, x = (BASEVIDWIDTH/2) + 4, ncontinues = players[consoleplayer].continues;
	if (ncontinues > 20)
		ncontinues = 20;

	if (imcontinuing)
		contsonic = W_CachePatchName("CONT2", PU_CACHE);
	else
		contsonic = W_CachePatchName("CONT1", PU_CACHE);

	V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 31);
	V_DrawCenteredString(BASEVIDWIDTH/2, 100, 0, "CONTINUE?");

	// Draw a Sonic!
	V_DrawScaledPatch((BASEVIDWIDTH - SHORT(contsonic->width))/2, 32, 0, contsonic);

	// Draw the continue markers! Show continues minus one.
	x -= ncontinues * 6;
	for (i = 0; i < ncontinues; ++i)
		V_DrawContinueIcon(x + (i*12), 140, 0, players[consoleplayer].skin, players[consoleplayer].skincolor);

	V_DrawCenteredString(BASEVIDWIDTH/2, 168, 0, va("\x82*\x80" " %02d " "\x82*\x80", timetonext/TICRATE));
}

void F_ContinueTicker(void)
{
	if (!imcontinuing)
	{
		// note the setup to prevent 2x reloading
		if (timetonext >= 0)
			timetonext--;
		if (timetonext == 0)
			Command_ExitGame_f();
	}
	else
	{
		// note the setup to prevent 2x reloading
		if (continuetime >= 0)
			continuetime--;
		if (continuetime == 0)
			G_Continue();
	}
}

boolean F_ContinueResponder(event_t *event)
{
	INT32 key = event->data1;

	if (keypressed)
		return true;

	if (timetonext >= 21*TICRATE/2)
		return false;
	if (event->type != ev_keydown)
		return false;

	// remap virtual keys (mouse & joystick buttons)
	switch (key)
	{
		case KEY_ENTER:
		case KEY_SPACE:
		case KEY_MOUSE1:
		case KEY_JOY1:
		case KEY_JOY1 + 2:
			break;
		default:
			return false;
	}

	keypressed = true;
	imcontinuing = true;
	continuetime = TICRATE;
	S_StartSound(0, sfx_itemup);
	return true;
}

// ==================
//  CUSTOM CUTSCENES
// ==================
static INT32 scenenum, cutnum;
static INT32 picxpos, picypos, picnum, pictime;
static INT32 textxpos, textypos;
static boolean dofadenow = false, cutsceneover = false;
static boolean runningprecutscene = false, precutresetplayer = false;

static void F_AdvanceToNextScene(void)
{
	// Don't increment until after endcutscene check
	// (possible overflow / bad patch names from the one tic drawn before the fade)
	if (scenenum+1 >= cutscenes[cutnum]->numscenes)
	{
		F_EndCutScene();
		return;
	}
	++scenenum;

	timetonext = 0;
	stoptimer = 0;
	picnum = 0;
	picxpos = cutscenes[cutnum]->scene[scenenum].xcoord[picnum];
	picypos = cutscenes[cutnum]->scene[scenenum].ycoord[picnum];

	if (cutscenes[cutnum]->scene[scenenum].musswitch[0])
		S_ChangeMusic(cutscenes[cutnum]->scene[scenenum].musswitch,
			cutscenes[cutnum]->scene[scenenum].musswitchflags,
			cutscenes[cutnum]->scene[scenenum].musicloop);

	// Fade to the next
	dofadenow = true;
	F_NewCutscene(cutscenes[cutnum]->scene[scenenum].text);

	picnum = 0;
	picxpos = cutscenes[cutnum]->scene[scenenum].xcoord[picnum];
	picypos = cutscenes[cutnum]->scene[scenenum].ycoord[picnum];
	textxpos = cutscenes[cutnum]->scene[scenenum].textxpos;
	textypos = cutscenes[cutnum]->scene[scenenum].textypos;

	animtimer = pictime = cutscenes[cutnum]->scene[scenenum].picduration[picnum];
}

void F_EndCutScene(void)
{
	cutsceneover = true; // do this first, just in case Y_EndGame or something wants to turn it back false later
	if (runningprecutscene)
	{
		if (server)
			D_MapChange(gamemap, gametype, ultimatemode, precutresetplayer, 0, true, false);
	}
	else
	{
		if (cutnum == creditscutscene-1)
			F_StartGameEvaluation();
		else if (cutnum == introtoplay-1)
			D_StartTitle();
		else if (nextmap < 1100-1)
			G_NextLevel();
		else
			Y_EndGame();
	}
}

void F_StartCustomCutscene(INT32 cutscenenum, boolean precutscene, boolean resetplayer)
{
	if (!cutscenes[cutscenenum])
		return;

	G_SetGamestate(GS_CUTSCENE);

	gameaction = ga_nothing;
	playerdeadview = false;
	paused = false;
	CON_ToggleOff();

	F_NewCutscene(cutscenes[cutscenenum]->scene[0].text);

	CON_ClearHUD();

	cutsceneover = false;
	runningprecutscene = precutscene;
	precutresetplayer = resetplayer;

	scenenum = picnum = 0;
	cutnum = cutscenenum;
	picxpos = cutscenes[cutnum]->scene[0].xcoord[0];
	picypos = cutscenes[cutnum]->scene[0].ycoord[0];
	textxpos = cutscenes[cutnum]->scene[0].textxpos;
	textypos = cutscenes[cutnum]->scene[0].textypos;

	pictime = cutscenes[cutnum]->scene[0].picduration[0];

	keypressed = false;
	finalecount = 0;
	timetonext = 0;
	animtimer = cutscenes[cutnum]->scene[0].picduration[0]; // Picture duration
	stoptimer = 0;

	if (cutscenes[cutnum]->scene[0].musswitch[0])
		S_ChangeMusic(cutscenes[cutnum]->scene[0].musswitch,
			cutscenes[cutnum]->scene[0].musswitchflags,
			cutscenes[cutnum]->scene[0].musicloop);
	else
		S_StopMusic();
}

//
// F_CutsceneDrawer
//
void F_CutsceneDrawer(void)
{
	if (dofadenow && rendermode != render_none)
	{
		F_WipeStartScreen();

		// Fade to any palette color you want.
		if (cutscenes[cutnum]->scene[scenenum].fadecolor)
		{
			V_DrawFill(0,0,BASEVIDWIDTH,BASEVIDHEIGHT,cutscenes[cutnum]->scene[scenenum].fadecolor);

			F_WipeEndScreen();
			F_RunWipe(cutscenes[cutnum]->scene[scenenum].fadeinid, true);

			F_WipeStartScreen();
		}
	}
	V_DrawFill(0,0, BASEVIDWIDTH, BASEVIDHEIGHT, 31);

	if (cutscenes[cutnum]->scene[scenenum].picname[picnum][0] != '\0')
	{
		if (cutscenes[cutnum]->scene[scenenum].pichires[picnum])
			V_DrawSmallScaledPatch(picxpos, picypos, 0,
				W_CachePatchName(cutscenes[cutnum]->scene[scenenum].picname[picnum], PU_CACHE));
		else
			V_DrawScaledPatch(picxpos,picypos, 0,
				W_CachePatchName(cutscenes[cutnum]->scene[scenenum].picname[picnum], PU_CACHE));
	}

	if (dofadenow && rendermode != render_none)
	{
		F_WipeEndScreen();
		F_RunWipe(cutscenes[cutnum]->scene[scenenum].fadeoutid, true);
	}

	V_DrawString(textxpos, textypos, 0, cutscene_disptext);
}

void F_CutsceneTicker(void)
{
	INT32 i;

	// Online clients tend not to instantly get the map change, so just wait
	// and don't send 30 of them.
	if (cutsceneover)
		return;

	// advance animation
	finalecount++;
	cutscene_boostspeed = 0;

	dofadenow = false;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (netgame && i != serverplayer && i != adminplayer)
			continue;

		if (players[i].cmd.buttons & BT_USE)
		{
			keypressed = false;
			cutscene_boostspeed = 1;
			if (timetonext)
				timetonext = 2;
		}
	}

	if (animtimer)
	{
		animtimer--;
		if (animtimer <= 0)
		{
			if (picnum < 7 && cutscenes[cutnum]->scene[scenenum].picname[picnum+1][0] != '\0')
			{
				picnum++;
				picxpos = cutscenes[cutnum]->scene[scenenum].xcoord[picnum];
				picypos = cutscenes[cutnum]->scene[scenenum].ycoord[picnum];
				pictime = cutscenes[cutnum]->scene[scenenum].picduration[picnum];
				animtimer = pictime;
			}
			else
				timetonext = 2;
		}
	}

	if (timetonext)
		--timetonext;

	if (++stoptimer > 2 && timetonext == 1)
		F_AdvanceToNextScene();
	else if (!timetonext && !F_WriteText())
		timetonext = 5*TICRATE + 1;
}

boolean F_CutsceneResponder(event_t *event)
{
	if (cutnum == introtoplay-1)
		return F_IntroResponder(event);

	return false;
}

// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2018 by Sonic Team Junior.
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
#include "d_netcmd.h"
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
#include "m_misc.h" // moviemode functionality
#include "y_inter.h"
#include "m_cond.h"
#include "p_local.h"
#include "p_setup.h"
#include "st_stuff.h" // hud hiding
#include "fastcmp.h"
#include "console.h"

#ifdef HAVE_BLUA
#include "lua_hud.h"
#endif

// Stage of animation:
// 0 = text, 1 = art screen
static INT32 finalecount;
INT32 titlescrollxspeed = 20;
INT32 titlescrollyspeed = 0;
UINT8 titlemapinaction = TITLEMAP_OFF;

static INT32 timetonext; // Delay between screen changes
static INT32 continuetime; // Short delay when continuing

static tic_t animtimer; // Used for some animation timings
static INT16 skullAnimCounter; // Prompts: Chevron animation

static INT32 deplete;
static tic_t stoptimer;

static boolean keypressed = false;

// (no longer) De-Demo'd Title Screen
static tic_t xscrolltimer;
static tic_t yscrolltimer;
static INT32 menuanimtimer; // Title screen: background animation timing
mobj_t *titlemapcameraref = NULL;

// menu presentation state
char curbgname[9];
SINT8 curfadevalue;
boolean curhidepics;
INT32 curbgcolor;
INT32 curbgxspeed;
INT32 curbgyspeed;
boolean curbghide;
boolean hidetitlemap;		// WARNING: set to false by M_SetupNextMenu and M_ClearMenus

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

static boolean goodending;
static patch_t *endbrdr[2]; // border - blue, white, pink - where have i seen those colours before?
static patch_t *endbgsp[3]; // nebula, sun, planet
static patch_t *endegrk[2]; // eggrock - replaced midway through good ending
static patch_t *endfwrk[3]; // firework - replaced with skin when good ending
static patch_t *endspkl[3]; // sparkle
static patch_t *endglow[2]; // glow aura - replaced with black rock's midway through good ending
static patch_t *endxpld[4]; // mini explosion
static patch_t *endescp[5]; // escape pod + flame
static INT32 sparkloffs[3][2]; // eggrock explosions/blackrock sparkles
static INT32 sparklloop;

//
// PROMPT STATE
//
boolean promptactive = false;
static mobj_t *promptmo;
static INT16 promptpostexectag;
static boolean promptblockcontrols;
static char *promptpagetext = NULL;
static INT32 callpromptnum = INT32_MAX;
static INT32 callpagenum = INT32_MAX;
static INT32 callplayer = INT32_MAX;

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
	S_StopMusic();
	S_StopSounds();

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
	"the control room and for what he thought\n"
	"would be the last time,\xB4 defeated\n"
	"Dr. Eggman.\n#");

	introtext[3] = M_GetText(
	"\nWhat Sonic, Tails, and Knuckles had\n"
	"not anticipated was that Eggman would\n"
	"return,\xB8 bringing an all new threat.\n#");

	introtext[4] = M_GetText(
	"\xA8""About every five years, a strange asteroid\n"
	"hovers around the planet.\xBF It suddenly\n"
	"appears from nowhere, circles around, and\n"
	"\xB6- just as mysteriously as it arrives -\xB6\n"
	"vanishes after only one week.\xBF\n"
	"No one knows why it appears, or how.\n#");

	introtext[5] = M_GetText(
	"\xA7\"Curses!\"\xA9\xBA Eggman yelled. \xA7\"That hedgehog\n"
	"and his ridiculous friends will pay\n"
	"dearly for this!\"\xA9\xC8 Just then his scanner\n"
	"blipped as the Black Rock made its\n"
	"appearance from nowhere.\xBF Eggman looked at\n"
	"the screen, and just shrugged it off.\n#");

	introtext[6] = M_GetText(
	"It was hours later\n"
	"that he had an\n"
	"idea. \xBF\xA7\"The Black\n"
	"Rock has a large\n"
	"amount of energy\n"
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
	"\xA5\"PRE-""\xB6""PARING-""\xB6""TO-""\xB4""FIRE-\xB6IN-""\xB6""15-""\xB6""SECONDS!\"\xA8\xB8\n"
	"his targeting system crackled\n"
	"robotically down the com-link. \xBF\xA7\"Good!\"\xA8\xB8\n"
	"Eggman sat back in his eggmobile and\n"
	"began to count down as he saw the\n"
	"Greenflower mountain on the monitor.\n#");

	introtext[9] = M_GetText(
	"\xA5\"10...\xD2""9...\xD2""8...\"\xA8\xD2\n"
	"Meanwhile, Sonic was tearing across the\n"
	"zones. Everything became a blur as he\n"
	"ran up slopes, skimmed over water,\n"
	"and catapulted himself off rocks with\n"
	"his phenomenal speed.\n#");

	introtext[10] = M_GetText(
	"\xA5\"6...\xD2""5...\xD2""4...\"\xA8\xD2\n"
	"Sonic knew he was getting closer to the\n"
	"zone, and pushed himself harder.\xB4 Finally,\n"
	"the mountain appeared on the horizon.\xD2\xD2\n"
	"\xA5\"3...\xD2""2...\xD2""1...\xD2""Zero.\"\n#");

	introtext[11] = M_GetText(
	"Greenflower Mountain was no more.\xC4\n"
	"Sonic arrived just in time to see what\n"
	"little of the 'ruins' were left.\n"
	"The natural beauty of the zone\n"
	"had been obliterated.\n#");

	introtext[12] = M_GetText(
	"\xA7\"You're not quite as gone as we thought,\n"
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
	"the once-perfect\n"
	"mountainside\n"
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
	paused = false;
	CON_ToggleOff();
	F_NewCutscene(introtext[0]);

	intro_scenenum = 0;
	finalecount = animtimer = skullAnimCounter = stoptimer = 0;
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
					S_ChangeMusicInternal("_stjr", false);
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
			menuanimtimer = animtimer; // Reusing this variable for the intro to fix the scrolling sky, better than changing the function around.
			F_SkyScroll(80*4, 0, "TITLESKY");
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
		if (intro_curtime > 1)
		{
			INT32 worktics = intro_curtime - 1;
			INT32 scale = FRACUNIT;
			patch_t *rockpat;
			UINT8 *colormap = NULL;
			patch_t *glow;
			INT32 trans = 0;

			INT32 x = ((BASEVIDWIDTH - 64)<<FRACBITS) - ((intro_curtime*FRACUNIT)/3);
			INT32 y = 24<<FRACBITS;

			if (worktics < 5)
			{
				scale = (worktics<<(FRACBITS-2));
				x += (30*(FRACUNIT-scale));
				y += (30*(FRACUNIT-scale));
			}

			rockpat = W_CachePatchName(va("ROID00%.2d", 34 - (worktics % 35)), PU_LEVEL);
			glow = W_CachePatchName(va("ENDGLOW%.1d", 2+(worktics & 1)), PU_LEVEL);

			if (worktics >= 5)
				trans = (worktics-5)>>1;
			if (trans < 10)
				V_DrawFixedPatch(x, y, scale, trans<<V_ALPHASHIFT, glow, NULL);

			trans = (15-worktics);
			if (trans < 0)
				trans = -trans;

			if (finalecount < 15)
				colormap = R_GetTranslationColormap(TC_ALLWHITE, 0, GTC_CACHE);
			V_DrawFixedPatch(x, y, scale, 0, rockpat, colormap);
			if (trans < 10)
				V_DrawFixedPatch(x, y, scale, trans<<V_ALPHASHIFT, rockpat, R_GetTranslationColormap(TC_BLINK, SKINCOLOR_AQUA, GTC_CACHE));
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

	V_DrawString(cx, cy, V_ALLOWLOWERCASE, cutscene_disptext);
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

			S_ChangeMusicInternal("_intro", false);
		}
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
				tic_t nowtime, quittime, lasttime;
				nowtime = lasttime = I_GetTime();
				quittime = nowtime + NEWTICRATE*2; // Shortened the quit time, used to be 2 seconds
				while (quittime > nowtime)
				{
					while (!((nowtime = I_GetTime()) - lasttime))
						I_Sleep();
					lasttime = nowtime;

					I_OsPolling();
					I_UpdateNoBlit();
					M_Drawer(); // menu is drawn even on top of wipes
					I_FinishUpdate(); // Update the screen with the image Tails 06-19-2001

					if (moviemode) // make sure we save frames for the white hold too
						M_SaveFrame();
				}
			}

			D_StartTitle();
			wipegamestate = GS_INTRO;
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
	"Vivian \"toaster\" Grannell",
	"Kepa \"Nev3r\" Iceta",
	"Thomas \"Shadow Hog\" Igoe",
	"Iestyn \"Monster Iestyn\" Jealous",
	"Ronald \"Furyhunter\" Kinard", // The SDL2 port
	"John \"JTE\" Muniz",
	"Ehab \"Wolfy\" Saeed",
	"\"Kaito Sinclaire\"",
	"\"SSNTails\"",
	"Marco \"mazmazz\" Zafra",
	"",
	"\1Programming",
	"\1Assistance",
	"\"chi.miru\"", // helped port slope drawing code from ZDoom
	"Andrew \"orospakr\" Clunis",
	"Sally \"TehRealSalt\" Cochenour",
	"Gregor \"Oogaland\" Dick",
	"Louis-Antoine \"LJSonic\" de Moulins", // for fixing 2.1's netcode (de Rochefort doesn't quite fit on the screen sorry lol)
	"Victor \"Steel Titanium\" Fuentes",
	"Julio \"Chaos Zero 64\" Guir",
	"\"Jimita\"",
	"\"Kalaron\"", // Coded some of Sryder13's collection of OpenGL fixes, especially fog
	"\"Lat'\"", // SRB2-CHAT, the chat window from Kart
	"Matthew \"Shuffle\" Marsalko",
	"Steven \"StroggOnMeth\" McGranahan",
	"\"Morph\"", // For SRB2Morphed stuff
	"Colin \"Sonict\" Pfaff",
	"Sean \"Sryder13\" Ryder",
	"Tasos \"tatokis\" Sahanidis", // Corrected C FixedMul, making 64-bit builds netplay compatible
	"Ben \"Cue\" Woodford",
	// Git contributors with 5+ approved merges of substantive quality,
	// or contributors with at least one groundbreaking merge, may be named.
	// Everyone else is acknowledged under "Special Thanks > SRB2 Community Contributors".
	"",
	"\1Sprite Artists",
	"\"Iceman404\"",
	"Victor \"VAdaPEGA\" Ara\x1Fjo", // Araújo -- sorry for our limited font! D:
	"Sally \"TehRealSalt\" Cochenour",
	"Jim \"MotorRoach\" DeMello",
	"Desmond \"Blade\" DesJardins",
	"Sherman \"CoatRack\" DesJardins",
	"Vivian \"toaster\" Grannell",
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
	"Vivian \"toaster\" Grannell",
	"Dan \"Blitzzo\" Hagerstrand",
	"Kepa \"Nev3r\" Iceta",
	"Thomas \"Shadow Hog\" Igoe",
	"\"Kaito Sinclaire\"",
	"Wessel \"sphere\" Smit",
	"\"Spazzo\"",
	"\"SSNTails\"",
	"Rob Tisdell",
	"\"Torgo\"",
	"Jarrett \"JEV3\" Voight",
	"Johnny \"Sonikku\" Wallbank",
	"Marco \"mazmazz\" Zafra",
	"",
	"\1Boss Design",
	"Ben \"Mystic\" Geyer",
	"Vivian \"toaster\" Grannell",
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
	"iD Software",
	"Doom Legacy Project",
	"FreeDoom Project", // Used some of the mancubus and rocket launcher sprites for Brak
	"Alex \"MistaED\" Fuller",
	"Pascal \"CodeImp\" vd Heiden", // Doom Builder developer
	"Randi Heit (<!>)", // For their MSPaint <!> sprite that we nicked
	"Simon \"sirjuddington\" Judd", // SLADE developer
	// Acknowledged here are the following:
	// Minor merge request authors, see guideline above
	// - Golden - Expanded thin font
	// Creators of small quantities of sprite/texture assets
	// - Arietty - New Green Hill-styled textures
	// - Scizor300 - the only other contributor to the 2.0 SRB2 Asset Pack
	"SRB2 Community Contributors",
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
	if ((!modifiedgame || savemoddata) && !(netgame || multiplayer) && cursaveslot > 0)
		G_SaveGame((UINT32)cursaveslot);

	if (creditscutscene)
	{
		F_StartCustomCutscene(creditscutscene - 1, false, false);
		return;
	}

	gameaction = ga_nothing;
	paused = false;
	CON_ToggleOff();
	S_StopMusic();
	S_StopSounds();

	S_ChangeMusicInternal("_creds", false);

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

	// Dim the background
	V_DrawFadeScreen(0xFF00, 16);

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
}

void F_CreditTicker(void)
{
	// "Simulate" the drawing of the credits so that dedicated mode doesn't get stuck
	UINT16 i;
	fixed_t y = (80<<FRACBITS) - 5*(animtimer<<FRACBITS)/8;

	// Draw credits text on top
	for (i = 0; credits[i]; i++)
	{
		switch(credits[i][0])
		{
			case 0: y += 80<<FRACBITS; break;
			case 1: y += 30<<FRACBITS; break;
			default: y += 12<<FRACBITS; break;
		}
		if (FixedMul(y,vid.dupy) > vid.height)
			break;
	}

	// Do this here rather than in the drawer you doofus! (this is why dedicated mode broke at credits)
	if (!credits[i] && y <= 120<<FRACBITS && !finalecount)
	{
		timetonext = 5*TICRATE+1;
		finalecount = 5*TICRATE;
	}

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

	if (!(timesBeaten) && !(netgame || multiplayer) && !cv_debug)
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

#define SPARKLLOOPTIME 7 // must be odd

void F_StartGameEvaluation(void)
{
	// Credits option in secrets menu
	if (cursaveslot == -1)
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
	if ((!modifiedgame || savemoddata) && !(netgame || multiplayer) && cursaveslot > 0)
		G_SaveGame((UINT32)cursaveslot);

	goodending = (ALL7EMERALDS(emeralds));

	gameaction = ga_nothing;
	paused = false;
	CON_ToggleOff();

	finalecount = -1;
	sparklloop = 0;
}

void F_GameEvaluationDrawer(void)
{
	INT32 x, y, i;
	angle_t fa;
	INT32 eemeralds_cur;
	char patchname[7] = "CEMGx0";
	const char* endingtext = (goodending ? "CONGRATULATIONS!" : "TRY AGAIN...");

	V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 31);

	// Draw all the good crap here.

	if (finalecount > 0)
	{
		INT32 scale = FRACUNIT;
		patch_t *rockpat;
		UINT8 *colormap[2] = {NULL, NULL};
		patch_t *glow;
		INT32 trans = 0;

		x = (((BASEVIDWIDTH-82)/2)+11)<<FRACBITS;
		y = (((BASEVIDHEIGHT-82)/2)+12)<<FRACBITS;

		if (finalecount < 5)
		{
			scale = (finalecount<<(FRACBITS-2));
			x += (30*(FRACUNIT-scale));
			y += (30*(FRACUNIT-scale));
		}

		if (goodending)
		{
			rockpat = W_CachePatchName(va("ROID00%.2d", 34 - (finalecount % 35)), PU_LEVEL);
			glow = W_CachePatchName(va("ENDGLOW%.1d", 2+(finalecount & 1)), PU_LEVEL);
			x -= FRACUNIT;
		}
		else
		{
			rockpat = W_CachePatchName("ROID0000", PU_LEVEL);
			glow = W_CachePatchName(va("ENDGLOW%.1d", (finalecount & 1)), PU_LEVEL);
		}

		if (finalecount >= 5)
			trans = (finalecount-5)>>1;
		if (trans < 10)
			V_DrawFixedPatch(x, y, scale, trans<<V_ALPHASHIFT, glow, NULL);

		trans = (15-finalecount);
		if (trans < 0)
			trans = -trans;

		if (finalecount < 15)
			colormap[0] = R_GetTranslationColormap(TC_ALLWHITE, 0, GTC_CACHE);
		V_DrawFixedPatch(x, y, scale, 0, rockpat, colormap[0]);
		if (trans < 10)
		{
			colormap[1] = R_GetTranslationColormap(TC_BLINK, SKINCOLOR_AQUA, GTC_CACHE);
			V_DrawFixedPatch(x, y, scale, trans<<V_ALPHASHIFT, rockpat, colormap[1]);
		}
		if (goodending)
		{
			INT32 j = (sparklloop & 1) ? 2 : 3;
			if (j > (finalecount/SPARKLLOOPTIME))
				j = (finalecount/SPARKLLOOPTIME);
			while (j)
			{
				if (j > 1 || sparklloop >= 2)
				{
					// if j == 0 - alternate between 0 and 1
					//         1 -                   1 and 2
					//         2 -                   2 and not rendered
					V_DrawFixedPatch(x+sparkloffs[j-1][0], y+sparkloffs[j-1][1], FRACUNIT, 0, W_CachePatchName(va("ENDSPKL%.1d", (j - ((sparklloop & 1) ? 0 : 1))), PU_LEVEL), R_GetTranslationColormap(TC_DEFAULT, SKINCOLOR_AQUA, GTC_CACHE));
				}
				j--;
			}
		}
		else
		{
			patch_t *eggrock = W_CachePatchName("ENDEGRK5", PU_LEVEL);
			V_DrawFixedPatch(x, y, scale, 0, eggrock, colormap[0]);
			if (trans < 10)
				V_DrawFixedPatch(x, y, scale, trans<<V_ALPHASHIFT, eggrock, colormap[1]);
			else if (sparklloop)
				V_DrawFixedPatch(x, y, scale, (10-sparklloop)<<V_ALPHASHIFT,
					W_CachePatchName("ENDEGRK0", PU_LEVEL), colormap[1]);
		}
	}

	eemeralds_cur = (finalecount % 360)<<FRACBITS;

	for (i = 0; i < 7; ++i)
	{
		fa = (FixedAngle(eemeralds_cur)>>ANGLETOFINESHIFT) & FINEMASK;
		x = (BASEVIDWIDTH<<(FRACBITS-1)) + (60*FINECOSINE(fa));
		y = ((BASEVIDHEIGHT+16)<<(FRACBITS-1)) + (60*FINESINE(fa));
		eemeralds_cur += (360<<FRACBITS)/7;

		patchname[4] = 'A'+(char)i;
		V_DrawFixedPatch(x, y, FRACUNIT, ((emeralds & (1<<i)) ? 0 : V_80TRANS), W_CachePatchName(patchname, PU_LEVEL), NULL);
	}

	V_DrawCreditString((BASEVIDWIDTH - V_CreditStringWidth(endingtext))<<(FRACBITS-1), (BASEVIDHEIGHT-100)<<(FRACBITS-1), 0, endingtext);

#if 0 // the following looks like hot garbage the more unlockables we add, and we now have a lot of unlockables
	if (finalecount >= 5*TICRATE)
	{
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
#endif
}

void F_GameEvaluationTicker(void)
{
	if (++finalecount > 10*TICRATE)
	{
		F_StartGameEnd();
		return;
	}

	if (!goodending)
	{
		if (sparklloop)
			sparklloop--;

		if (finalecount == (5*TICRATE)/2
			|| finalecount == (7*TICRATE)/2
			|| finalecount == ((7*TICRATE)/2)+5)
		{
			S_StartSound(NULL, sfx_s3k5c);
			sparklloop = 10;
		}
	}
	else if (++sparklloop == SPARKLLOOPTIME) // time to roll the randomisation again
	{
		angle_t workingangle = FixedAngle((M_RandomKey(360))<<FRACBITS)>>ANGLETOFINESHIFT;
		fixed_t workingradius = M_RandomKey(26);

		sparkloffs[2][0] = sparkloffs[1][0];
		sparkloffs[2][1] = sparkloffs[1][1];
		sparkloffs[1][0] = sparkloffs[0][0];
		sparkloffs[1][1] = sparkloffs[0][1];

		sparkloffs[0][0] = (30<<FRACBITS) + workingradius*FINECOSINE(workingangle);
		sparkloffs[0][1] = (30<<FRACBITS) + workingradius*FINESINE(workingangle);
		sparklloop = 0;
	}

	if (finalecount == 5*TICRATE)
	{
		if (netgame || multiplayer) // modify this when we finally allow unlocking stuff in 2P
		{
			HU_SetCEchoFlags(V_YELLOWMAP|V_RETURN8);
			HU_SetCEchoDuration(6);
			HU_DoCEcho("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\Prizes only awarded in singleplayer!");
			S_StartSound(NULL, sfx_s3k68);
		}
		else if (!modifiedgame || savemoddata)
		{
			++timesBeaten;

			if (ALL7EMERALDS(emeralds))
				++timesBeatenWithEmeralds;

			if (ultimatemode)
				++timesBeatenUltimate;

			if (M_UpdateUnlockablesAndExtraEmblems())
				S_StartSound(NULL, sfx_s3k68);

			G_SaveGameData();
		}
		else
		{
			HU_SetCEchoFlags(V_YELLOWMAP|V_RETURN8);
			HU_SetCEchoDuration(6);
			HU_DoCEcho("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\Prizes not awarded in modified games!");
			S_StartSound(NULL, sfx_s3k68);
		}
	}
}

#undef SPARKLLOOPTIME

// ==========
//   ENDING
// ==========

#define INFLECTIONPOINT (6*TICRATE)
#define SPARKLLOOPTIME 15 // must be odd

void F_StartEnding(void)
{
	G_SetGamestate(GS_ENDING);
	wipetypepost = INT16_MAX;

	// Just in case they're open ... somehow
	M_ClearMenus(true);

	// Save before the credits sequence.
	if ((!modifiedgame || savemoddata) && !(netgame || multiplayer) && cursaveslot > 0)
		G_SaveGame((UINT32)cursaveslot);

	gameaction = ga_nothing;
	paused = false;
	CON_ToggleOff();
	S_StopMusic(); // todo: placeholder
	S_StopSounds();

	finalecount = -10; // what? this totally isn't a hack. why are you asking?

	memset(sparkloffs, 0, sizeof(INT32)*3*2);
	sparklloop = 0;

	endbrdr[1] = W_CachePatchName("ENDBRDR1", PU_LEVEL);

	endegrk[0] = W_CachePatchName("ENDEGRK0", PU_LEVEL);
	endegrk[1] = W_CachePatchName("ENDEGRK1", PU_LEVEL);

	endglow[0] = W_CachePatchName("ENDGLOW0", PU_LEVEL);
	endglow[1] = W_CachePatchName("ENDGLOW1", PU_LEVEL);

	endbgsp[0] = W_CachePatchName("ENDBGSP0", PU_LEVEL);
	endbgsp[1] = W_CachePatchName("ENDBGSP1", PU_LEVEL);
	endbgsp[2] = W_CachePatchName("ENDBGSP2", PU_LEVEL);

	endspkl[0] = W_CachePatchName("ENDSPKL0", PU_LEVEL);
	endspkl[1] = W_CachePatchName("ENDSPKL1", PU_LEVEL);
	endspkl[2] = W_CachePatchName("ENDSPKL2", PU_LEVEL);

	endxpld[0] = W_CachePatchName("ENDXPLD0", PU_LEVEL);
	endxpld[1] = W_CachePatchName("ENDXPLD1", PU_LEVEL);
	endxpld[2] = W_CachePatchName("ENDXPLD2", PU_LEVEL);
	endxpld[3] = W_CachePatchName("ENDXPLD3", PU_LEVEL);

	endescp[0] = W_CachePatchName("ENDESCP0", PU_LEVEL);
	endescp[1] = W_CachePatchName("ENDESCP1", PU_LEVEL);
	endescp[2] = W_CachePatchName("ENDESCP2", PU_LEVEL);
	endescp[3] = W_CachePatchName("ENDESCP3", PU_LEVEL);
	endescp[4] = W_CachePatchName("ENDESCP4", PU_LEVEL);

	// so we only need to check once
	if ((goodending = ALL7EMERALDS(emeralds)))
	{
		UINT8 skinnum = players[consoleplayer].skin;
		spritedef_t *sprdef;
		spriteframe_t *sprframe;
		if (skins[skinnum].sprites[SPR2_XTRA].numframes >= (XTRA_ENDING+2)+1)
		{
			sprdef = &skins[skinnum].sprites[SPR2_XTRA];
			// character head, skin specific
			sprframe = &sprdef->spriteframes[XTRA_ENDING];
			endfwrk[0] = W_CachePatchNum(sprframe->lumppat[0], PU_LEVEL);
			sprframe = &sprdef->spriteframes[XTRA_ENDING+1];
			endfwrk[1] = W_CachePatchNum(sprframe->lumppat[0], PU_LEVEL);
			sprframe = &sprdef->spriteframes[XTRA_ENDING+2];
			endfwrk[2] = W_CachePatchNum(sprframe->lumppat[0], PU_LEVEL);
		}
		else // Show a star if your character doesn't have an ending firework display. (Basically the MISSINGs for this)
		{
			endfwrk[0] = W_CachePatchName("ENDFWRK3", PU_LEVEL);
			endfwrk[1] = W_CachePatchName("ENDFWRK4", PU_LEVEL);
			endfwrk[2] = W_CachePatchName("ENDFWRK5", PU_LEVEL);
		}

		endbrdr[0] = W_CachePatchName("ENDBRDR2", PU_LEVEL);
	}
	else
	{
		// eggman, skin nonspecific
		endfwrk[0] = W_CachePatchName("ENDFWRK0", PU_LEVEL);
		endfwrk[1] = W_CachePatchName("ENDFWRK1", PU_LEVEL);
		endfwrk[2] = W_CachePatchName("ENDFWRK2", PU_LEVEL);

		endbrdr[0] = W_CachePatchName("ENDBRDR0", PU_LEVEL);
	}
}

void F_EndingTicker(void)
{
	if (++finalecount > INFLECTIONPOINT*2)
	{
		F_StartCredits();
		wipetypepre = INT16_MAX;
		return;
	}

	if (goodending && finalecount == INFLECTIONPOINT) // time to swap some assets
	{
		endegrk[0] = W_CachePatchName("ENDEGRK2", PU_LEVEL);
		endegrk[1] = W_CachePatchName("ENDEGRK3", PU_LEVEL);

		endglow[0] = W_CachePatchName("ENDGLOW2", PU_LEVEL);
		endglow[1] = W_CachePatchName("ENDGLOW3", PU_LEVEL);

		endxpld[0] = W_CachePatchName("ENDEGRK4", PU_LEVEL);
	}

	if (++sparklloop == SPARKLLOOPTIME) // time to roll the randomisation again
	{
		angle_t workingangle = FixedAngle((M_RandomRange(-170, 80))<<FRACBITS)>>ANGLETOFINESHIFT;
		fixed_t workingradius = M_RandomKey(26);

		sparkloffs[0][0] = (30<<FRACBITS) + workingradius*FINECOSINE(workingangle);
		sparkloffs[0][1] = (30<<FRACBITS) + workingradius*FINESINE(workingangle);

		sparklloop = 0;
	}
}

void F_EndingDrawer(void)
{
	INT32 x, y, i, j, parallaxticker;
	patch_t *rockpat;

	if (!goodending || finalecount < INFLECTIONPOINT)
		rockpat = W_CachePatchName("ROID0000", PU_LEVEL);
	else
		rockpat = W_CachePatchName(va("ROID00%.2d", 34 - ((finalecount - INFLECTIONPOINT) % 35)), PU_LEVEL);

	V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 31);

	parallaxticker = finalecount - INFLECTIONPOINT;
	x = -((parallaxticker*20)<<FRACBITS)/INFLECTIONPOINT;
	y = ((parallaxticker*7)<<FRACBITS)/INFLECTIONPOINT;
	i = (((BASEVIDWIDTH-82)/2)+11)<<FRACBITS;
	j = (((BASEVIDHEIGHT-82)/2)+12)<<FRACBITS;

	if (finalecount <= -10)
		;
	else if (finalecount < 0)
		V_DrawFadeFill(24, 24, BASEVIDWIDTH-48, BASEVIDHEIGHT-48, 0, 0, 10+finalecount);
	else if (finalecount <= 20)
	{
		V_DrawFill(24, 24, BASEVIDWIDTH-48, BASEVIDHEIGHT-48, 0);
		if (finalecount && finalecount < 20)
		{
			INT32 trans = (10-finalecount);
			if (trans < 0)
			{
				trans = -trans;
				V_DrawScaledPatch(BASEVIDWIDTH/2, BASEVIDHEIGHT/2, 0, endbrdr[0]);
			}
			V_DrawScaledPatch(BASEVIDWIDTH/2, BASEVIDHEIGHT/2, trans<<V_ALPHASHIFT, endbrdr[1]);
		}
		else if (finalecount == 20)
			V_DrawScaledPatch(BASEVIDWIDTH/2, BASEVIDHEIGHT/2, 0, endbrdr[0]);
	}
	else if (goodending && (parallaxticker == -2 || !parallaxticker))
	{
		V_DrawFill(24, 24, BASEVIDWIDTH-48, BASEVIDHEIGHT-48, 0);
		V_DrawFixedPatch(x+i, y+j, FRACUNIT, 0, endegrk[0],
			R_GetTranslationColormap(TC_BLINK, SKINCOLOR_BLACK, GTC_CACHE));
		//V_DrawScaledPatch(BASEVIDWIDTH/2, BASEVIDHEIGHT/2, 0, endbrdr[1]);
	}
	else if (goodending && parallaxticker == -1)
	{
		V_DrawFixedPatch(x+i, y+j, FRACUNIT, 0, rockpat,
			R_GetTranslationColormap(TC_ALLWHITE, 0, GTC_CACHE));
		V_DrawScaledPatch(BASEVIDWIDTH/2, BASEVIDHEIGHT/2, 0, endbrdr[1]);
	}
	else
	{
		boolean doexplosions = false;
		boolean borderstuff = false;
		INT32 tweakx = 0, tweaky = 0;

		if (parallaxticker < 75) // f background's supposed to be visible
		{
			V_DrawFixedPatch(-(x/10), -(y/10), FRACUNIT, 0, endbgsp[0], NULL); // nebula
			V_DrawFixedPatch(-(x/5),  -(y/5),  FRACUNIT, 0, endbgsp[1], NULL); // sun
			V_DrawFixedPatch(     0,  -(y/2),  FRACUNIT, 0, endbgsp[2], NULL); // planet

			// player's escape pod
			V_DrawFixedPatch((200<<FRACBITS)+(finalecount<<(FRACBITS-2)),
				(100<<FRACBITS)+(finalecount<<(FRACBITS-2)),
				FRACUNIT, 0, endescp[4], NULL);
			if (parallaxticker > -19)
			{
				INT32 trans = (-parallaxticker)>>1;
				if (trans < 0)
					trans = 0;
				V_DrawFixedPatch((200<<FRACBITS)+(finalecount<<(FRACBITS-2)),
					(100<<FRACBITS)+(finalecount<<(FRACBITS-2)),
					FRACUNIT, trans<<V_ALPHASHIFT, endescp[(finalecount/2)&3], NULL);
			}

			if (goodending && parallaxticker > 0) // gunchedrock
			{
				INT32 scale = FRACUNIT + ((parallaxticker-10)<<7);
				INT32 trans = parallaxticker>>2;
				UINT8 *colormap = R_GetTranslationColormap(TC_RAINBOW, SKINCOLOR_JET, GTC_CACHE);

				if (parallaxticker < 10)
				{
					tweakx = parallaxticker<<FRACBITS;
					tweaky = ((7*parallaxticker)<<(FRACBITS-2))/5;
				}
				else
				{
					tweakx = 10<<FRACBITS;
					tweaky = 7<<(FRACBITS-1);
				}
				i += tweakx;
				j -= tweaky;

				x <<= 1;
				y <<= 1;

				// center detritrus
				V_DrawFixedPatch(i-x, j-y, FRACUNIT, 0, endegrk[0], colormap);
				if (trans < 10)
					V_DrawFixedPatch(i-x, j-y, FRACUNIT, trans<<V_ALPHASHIFT, endegrk[0], NULL);

				 // ring detritrus
				V_DrawFixedPatch((30*(FRACUNIT-scale))+i-(2*x), (30*(FRACUNIT-scale))+j-(2*y) - ((7<<FRACBITS)/2), scale, 0, endegrk[1], colormap);
				if (trans < 10)
					V_DrawFixedPatch((30*(FRACUNIT-scale))+i-(2*x), (30*(FRACUNIT-scale))+j-(2*y), scale, trans<<V_ALPHASHIFT, endegrk[1], NULL);

				scale += ((parallaxticker-10)<<7);

				 // shard detritrus
				V_DrawFixedPatch((30*(FRACUNIT-scale))+i-(x/2), (30*(FRACUNIT-scale))+j-(y/2) - ((7<<FRACBITS)/2), scale, 0, endxpld[0], colormap);
				if (trans < 10)
					V_DrawFixedPatch((30*(FRACUNIT-scale))+i-(x/2), (30*(FRACUNIT-scale))+j-(y/2), scale, trans<<V_ALPHASHIFT, endxpld[0], NULL);
			}
		}
		else if (goodending)
		{
			tweakx = 10<<FRACBITS;
			tweaky = 7<<(FRACBITS-1);
			i += tweakx;
			j += tweaky;
			x <<= 1;
			y <<= 1;
		}

		if (goodending && parallaxticker > 0)
		{
			i -= (3+(tweakx<<1));
			j += tweaky<<2;
		}

		if (parallaxticker <= 70) // eggrock/blackrock
		{
			INT32 trans;
			fixed_t scale = FRACUNIT;
			UINT8 *colormap[2] = {NULL, NULL};

			x += i;
			y += j;

			if (parallaxticker > 66)
			{
				scale = ((70 - parallaxticker)<<(FRACBITS-2));
				x += (30*(FRACUNIT-scale));
				y += (30*(FRACUNIT-scale));
			}
			else if ((parallaxticker > 60) || (goodending && parallaxticker > 0))
				;
			else
			{
				doexplosions = true;
				if (!sparklloop)
				{
					x += ((sparkloffs[0][0] < 30<<FRACBITS) ? FRACUNIT : -FRACUNIT);
					y += ((sparkloffs[0][1] < 30<<FRACBITS) ? FRACUNIT : -FRACUNIT);
				}
			}

			if (goodending && finalecount > INFLECTIONPOINT)
				parallaxticker -= 40;

			if ((-parallaxticker/4) < 5)
			{
				trans = (-parallaxticker/4) + 5;
				if (trans < 0)
					trans = 0;
				V_DrawFixedPatch(x, y, scale, trans<<V_ALPHASHIFT, endglow[(finalecount & 1) ? 0 : 1], NULL);
			}

			if (goodending && finalecount > INFLECTIONPOINT)
			{
				if (finalecount < INFLECTIONPOINT+10)
					V_DrawFadeFill(24, 24, BASEVIDWIDTH-48, BASEVIDHEIGHT-48, 0, 0, INFLECTIONPOINT+10-finalecount);
				parallaxticker -= 30;
			}

			if ((parallaxticker/2) > -15)
				colormap[0] = R_GetTranslationColormap(TC_ALLWHITE, 0, GTC_CACHE);
			V_DrawFixedPatch(x, y, scale, 0, rockpat, colormap[0]);
			if ((parallaxticker/2) > -25)
			{
				trans = (parallaxticker/2) + 15;
				if (trans < 0)
					trans = -trans;
				if (trans < 10)
					V_DrawFixedPatch(x, y, scale, trans<<V_ALPHASHIFT, rockpat,
						R_GetTranslationColormap(TC_BLINK, SKINCOLOR_AQUA, GTC_CACHE));
			}

			if (goodending && finalecount > INFLECTIONPOINT)
			{
				if (finalecount < INFLECTIONPOINT+10)
					V_DrawFixedPatch(x, y, scale, (finalecount-INFLECTIONPOINT)<<V_ALPHASHIFT, rockpat,
						R_GetTranslationColormap(TC_BLINK, SKINCOLOR_BLACK, GTC_CACHE));
			}
			else
			{
				if ((-parallaxticker/2) < -5)
					colormap[1] = R_GetTranslationColormap(TC_ALLWHITE, 0, GTC_CACHE);

				V_DrawFixedPatch(x, y, scale, 0, endegrk[0], colormap[1]);

				if ((-parallaxticker/2) < 5)
				{
					trans = (-parallaxticker/2) + 5;
					if (trans < 0)
						trans = -trans;
					if (trans < 10)
						V_DrawFixedPatch(x, y, scale, trans<<V_ALPHASHIFT, endegrk[1], NULL);
				}
			}
		}
		else // firework
		{
			fixed_t scale = FRACUNIT;
			INT32 frame;
			UINT8 *colormap = NULL;
			parallaxticker -= 70;
			x += ((BASEVIDWIDTH-3)<<(FRACBITS-1)) - tweakx;
			y += (BASEVIDHEIGHT<<(FRACBITS-1)) + tweaky;
			borderstuff = true;

			if (parallaxticker < 5)
			{
				scale = (parallaxticker<<FRACBITS)/4;
				V_DrawFadeFill(24, 24, BASEVIDWIDTH-48, BASEVIDHEIGHT-48, 0, 31, parallaxticker*2);
			}
			else
				scale += (parallaxticker-4)<<5;

			if (goodending)
				colormap = R_GetTranslationColormap(players[consoleplayer].skin, players[consoleplayer].skincolor, GTC_CACHE);

			if ((frame = ((parallaxticker & 1) ? 1 : 0) + (parallaxticker/TICRATE)) < 3)
				V_DrawFixedPatch(x, y, scale, 0, endfwrk[frame], colormap);
		}

		// explosions
		if (sparklloop >= 3 && doexplosions)
		{
			INT32 boomtime = parallaxticker - sparklloop;

			x = ((((BASEVIDWIDTH-82)/2)+11)<<FRACBITS) - ((boomtime*20)<<FRACBITS)/INFLECTIONPOINT;
			y = ((((BASEVIDHEIGHT-82)/2)+12)<<FRACBITS) + ((boomtime*7)<<FRACBITS)/INFLECTIONPOINT;

			V_DrawFixedPatch(x + sparkloffs[0][0], y + sparkloffs[0][1],
				FRACUNIT, 0, endxpld[sparklloop/4], NULL);
		}

		// initial fade
		if (finalecount < 30)
			V_DrawFadeFill(24, 24, BASEVIDWIDTH-48, BASEVIDHEIGHT-48, 0, 0, 30-finalecount);

		// border - only emeralds can exist outside it
		{
			INT32 trans = 0;
			if (borderstuff)
				trans = (10*parallaxticker)/(3*TICRATE);
			if (trans < 10)
				V_DrawScaledPatch(BASEVIDWIDTH/2, BASEVIDHEIGHT/2, trans<<V_ALPHASHIFT, endbrdr[0]);
			if (borderstuff && parallaxticker < 11)
				V_DrawScaledPatch(BASEVIDWIDTH/2, BASEVIDHEIGHT/2, (parallaxticker-1)<<V_ALPHASHIFT, endbrdr[1]);
			else if (goodending && finalecount > INFLECTIONPOINT && finalecount < INFLECTIONPOINT+10)
				V_DrawScaledPatch(BASEVIDWIDTH/2, BASEVIDHEIGHT/2, (finalecount-INFLECTIONPOINT)<<V_ALPHASHIFT, endbrdr[1]);
		}

		// emeralds and emerald accessories
		if (goodending && finalecount >= TICRATE && finalecount < INFLECTIONPOINT)
		{
			INT32 workingtime = finalecount - TICRATE;
			fixed_t radius = ((vid.width/vid.dupx)*(INFLECTIONPOINT - TICRATE - workingtime))/(INFLECTIONPOINT - TICRATE);
			angle_t fa;
			INT32 eemeralds_cur[4];
			char patchname[7] = "CEMGx0";

			radius <<= FRACBITS;

			for (i = 0; i < 4; ++i)
			{
				if (i == 1)
					workingtime -= sparklloop;
				else if (i)
					workingtime -= SPARKLLOOPTIME;
				eemeralds_cur[i] = (workingtime % 360)<<FRACBITS;
			}

			// sparkles
			for (i = 0; i < 7; ++i)
			{
				UINT8* colormap;
				skincolors_t col = SKINCOLOR_GREEN;
				switch (i)
				{
					case 1:
						col = SKINCOLOR_MAGENTA;
						break;
					case 2:
						col = SKINCOLOR_BLUE;
						break;
					case 3:
						col = SKINCOLOR_SKY;
						break;
					case 4:
						col = SKINCOLOR_ORANGE;
						break;
					case 5:
						col = SKINCOLOR_RED;
						break;
					case 6:
						col = SKINCOLOR_GREY;
					default:
					case 0:
						break;
				}

				colormap = R_GetTranslationColormap(TC_DEFAULT, col, GTC_CACHE);

				j = (sparklloop & 1) ? 2 : 3;
				while (j)
				{
					fa = (FixedAngle(eemeralds_cur[j])>>ANGLETOFINESHIFT) & FINEMASK;
					x =  (BASEVIDWIDTH<<(FRACBITS-1)) + FixedMul(FINECOSINE(fa),radius);
					y = (BASEVIDHEIGHT<<(FRACBITS-1)) + FixedMul(FINESINE(fa),radius);
					eemeralds_cur[j] += (360<<FRACBITS)/7;

					// if j == 0 - alternate between 0 and 1
					//         1 -                   1 and 2
					//         2 -                   2 and not rendered
					V_DrawFixedPatch(x, y, FRACUNIT, 0, endspkl[(j - ((sparklloop & 1) ? 0 : 1))], colormap);

					j--;
				}
			}

			// ...then emeralds themselves
			for (i = 0; i < 7; ++i)
			{
				fa = (FixedAngle(eemeralds_cur[0])>>ANGLETOFINESHIFT) & FINEMASK;
				x = (BASEVIDWIDTH<<(FRACBITS-1)) + FixedMul(FINECOSINE(fa),radius);
				y = ((BASEVIDHEIGHT+16)<<(FRACBITS-1)) + FixedMul(FINESINE(fa),radius);
				eemeralds_cur[0] += (360<<FRACBITS)/7;

				patchname[4] = 'A'+(char)i;
				V_DrawFixedPatch(x, y, FRACUNIT, 0, W_CachePatchName(patchname, PU_LEVEL), NULL);
			}
		} // if (goodending...
	} // (finalecount > 20)

	// look, i make an ending for you last-minute, the least you could do is let me have this
	if (cv_soundtest.value == 413)
	{
		INT32 trans = 0;
		boolean donttouch = false;
		const char *str;
		if (goodending)
			str = va("[S] %s: Engage.", skins[players[consoleplayer].skin].realname);
		else
			str = "[S] Eggman: Abscond.";

		if (finalecount < 10)
			trans = (10-finalecount)/2;
		else if (finalecount > (2*INFLECTIONPOINT) - 20)
		{
			trans = 10 + (finalecount/2) - INFLECTIONPOINT;
			donttouch = true;
		}

		if (trans != 10)
		{
			//colset(linkmap,  164, 165, 169); -- the ideal purple colour to represent a clicked in-game link, but not worth it just for a soundtest-controlled secret
			V_DrawCenteredString(BASEVIDWIDTH/2, 8, V_ALLOWLOWERCASE|(trans<<V_ALPHASHIFT), str);
			V_DrawCharacter(32, BASEVIDHEIGHT-16, '>'|(trans<<V_ALPHASHIFT), false);
			V_DrawString(40, ((finalecount == (2*INFLECTIONPOINT)-(20+TICRATE)) ? 1 : 0)+BASEVIDHEIGHT-16, ((timesBeaten || finalecount >= (2*INFLECTIONPOINT)-TICRATE) ? V_PURPLEMAP : V_BLUEMAP)|(trans<<V_ALPHASHIFT), " [S] ===>");
		}

		if (finalecount > (2*INFLECTIONPOINT)-(20+(2*TICRATE)))
		{
			INT32 trans2 = abs((5*FINECOSINE((FixedAngle((finalecount*5)<<FRACBITS)>>ANGLETOFINESHIFT & FINEMASK)))>>FRACBITS)+2;
			if (!donttouch)
			{
				trans = 10 + ((2*INFLECTIONPOINT)-(20+(2*TICRATE))) - finalecount;
				if (trans > trans2)
					trans2 = trans;
			}
			else
				trans2 += 2*trans;
			if (trans2 < 10)
				V_DrawCharacter(26, BASEVIDHEIGHT-33, '\x1C'|(trans2<<V_ALPHASHIFT), false);
		}
	}
}

#undef SPARKLLOOPTIME

// ==========
//  GAME END
// ==========
void F_StartGameEnd(void)
{
	G_SetGamestate(GS_GAMEEND);

	gameaction = ga_nothing;
	paused = false;
	CON_ToggleOff();
	S_StopMusic();
	S_StopSounds();

	// In case menus are still up?!!
	M_ClearMenus(true);

	timetonext = TICRATE;
}

//
// F_GameEndDrawer
//
void F_GameEndDrawer(void)
{
	// this function does nothing
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

void F_InitMenuPresValues(void)
{
	menuanimtimer = 0;
	prevMenuId = 0;
	activeMenuId = MainDef.menuid;

	// Set defaults for presentation values
	strncpy(curbgname, "TITLESKY", 9);
	curfadevalue = 16;
	curhidepics = hidetitlepics;
	curbgcolor = -1;
	curbgxspeed = (gamestate == GS_TIMEATTACK) ? 0 : titlescrollxspeed;
	curbgyspeed = (gamestate == GS_TIMEATTACK) ? 22 : titlescrollyspeed;
	curbghide = (gamestate == GS_TIMEATTACK) ? false : true;

	// Find current presentation values
	M_SetMenuCurBackground((gamestate == GS_TIMEATTACK) ? "RECATTBG" : "TITLESKY");
	M_SetMenuCurFadeValue(16);
	M_SetMenuCurHideTitlePics();
}

//
// F_SkyScroll
//
void F_SkyScroll(INT32 scrollxspeed, INT32 scrollyspeed, const char *patchname)
{
	INT32 xscrolled, x, xneg = (scrollxspeed > 0) - (scrollxspeed < 0), tilex;
	INT32 yscrolled, y, yneg = (scrollyspeed > 0) - (scrollyspeed < 0), tiley;
	boolean xispos = (scrollxspeed >= 0), yispos = (scrollyspeed >= 0);
	INT32 dupz = (vid.dupx < vid.dupy ? vid.dupx : vid.dupy);
	INT16 patwidth, patheight;
	INT32 pw, ph; // scaled by dupz
	patch_t *pat;
	INT32 i, j;

	if (rendermode == render_none)
		return;

	if (!patchname || !patchname[0])
	{
		V_DrawFill(0, 0, vid.width, vid.height, 31);
		return;
	}

	if (!scrollxspeed && !scrollyspeed)
	{
		V_DrawPatchFill(W_CachePatchName(patchname, PU_CACHE));
		return;
	}

	pat = W_CachePatchName(patchname, PU_CACHE);

	patwidth = SHORT(pat->width);
	patheight = SHORT(pat->height);
	pw = patwidth * dupz;
	ph = patheight * dupz;

	tilex = max(FixedCeil(FixedDiv(vid.width, pw)) >> FRACBITS, 1)+2; // one tile on both sides of center
	tiley = max(FixedCeil(FixedDiv(vid.height, ph)) >> FRACBITS, 1)+2;

	xscrolltimer = ((menuanimtimer*scrollxspeed)/16 + patwidth*xneg) % (patwidth);
	yscrolltimer = ((menuanimtimer*scrollyspeed)/16 + patheight*yneg) % (patheight);

	// coordinate offsets
	xscrolled = xscrolltimer * dupz;
	yscrolled = yscrolltimer * dupz;

	for (x = (xispos) ? -pw*(tilex-1)+pw : 0, i = 0;
		i < tilex;
		x += pw, i++)
	{
		for (y = (yispos) ? -ph*(tiley-1)+ph : 0, j = 0;
			j < tiley;
			y += ph, j++)
		{
			V_DrawScaledPatch(
				(xispos) ? xscrolled - x : x + xscrolled,
				(yispos) ? yscrolled - y : y + yscrolled,
				V_NOSCALESTART, pat);
		}
	}

	W_UnlockCachedPatch(pat);
}

void F_StartTitleScreen(void)
{
	if (menupres[MN_MAIN].musname[0])
		S_ChangeMusic(menupres[MN_MAIN].musname, menupres[MN_MAIN].mustrack, menupres[MN_MAIN].muslooping);
	else
		S_ChangeMusicInternal("_title", looptitle);

	if (gamestate != GS_TITLESCREEN && gamestate != GS_WAITINGPLAYERS)
	{
		finalecount = 0;
		wipetypepost = menupres[MN_MAIN].enterwipe;
	}
	else
		wipegamestate = GS_TITLESCREEN;

	if (titlemap)
	{
		mapthing_t *startpos;

		gamestate_t prevwipegamestate = wipegamestate;
		titlemapinaction = TITLEMAP_LOADING;
		titlemapcameraref = NULL;
		gamemap = titlemap;

		if (!mapheaderinfo[gamemap-1])
			P_AllocMapHeader(gamemap-1);

		maptol = mapheaderinfo[gamemap-1]->typeoflevel;
		globalweather = mapheaderinfo[gamemap-1]->weather;

		G_DoLoadLevel(true);
		if (!titlemap)
			return;

		players[displayplayer].playerstate = PST_DEAD; // Don't spawn the player in dummy (I'm still a filthy cheater)

		// Set Default Position
		if (playerstarts[0])
			startpos = playerstarts[0];
		else if (deathmatchstarts[0])
			startpos = deathmatchstarts[0];
		else
			startpos = NULL;

		if (startpos)
		{
			camera.x = startpos->x << FRACBITS;
			camera.y = startpos->y << FRACBITS;
			camera.subsector = R_PointInSubsector(camera.x, camera.y);
			camera.z = camera.subsector->sector->floorheight + ((startpos->options >> ZSHIFT) << FRACBITS);
			camera.angle = (startpos->angle % 360)*ANG1;
			camera.aiming = 0;
		}
		else
		{
			camera.x = camera.y = camera.z = camera.angle = camera.aiming = 0;
			camera.subsector = NULL; // toast is filthy too
		}

		camera.chase = true;
		camera.height = 0;

		// Run enter linedef exec for MN_MAIN, since this is where we start
		if (menupres[MN_MAIN].entertag)
			P_LinedefExecute(menupres[MN_MAIN].entertag, players[displayplayer].mo, NULL);

		wipegamestate = prevwipegamestate;
	}
	else
	{
		titlemapinaction = TITLEMAP_OFF;
		gamemap = 1; // g_game.c
		CON_ClearHUD();
	}

	G_SetGamestate(GS_TITLESCREEN);

	// IWAD dependent stuff.

	animtimer = skullAnimCounter = 0;

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
	boolean hidepics;

	if (modeattacking)
		return; // We likely came here from retrying. Don't do a damn thing.

	// Draw that sky!
	if (curbgcolor >= 0)
		V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, curbgcolor);
	else if (titlemapinaction && curbghide && ! hidetitlemap)
		D_Render();
	else
		F_SkyScroll(curbgxspeed, curbgyspeed, curbgname);

	// Don't draw outside of the title screen, or if the patch isn't there.
	if (!ttwing || (gamestate != GS_TITLESCREEN && gamestate != GS_WAITINGPLAYERS))
		return;

	// rei|miru: use title pics?
	hidepics = curhidepics;
	if (hidepics)
#ifdef HAVE_BLUA
		goto luahook;
#else
		return;
#endif

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

#ifdef HAVE_BLUA
luahook:
	LUAh_TitleHUD();
#endif
}

// separate animation timer for backgrounds, since we also count
// during GS_TIMEATTACK
void F_MenuPresTicker(boolean run)
{
	if (run)
		menuanimtimer++;
}

// (no longer) De-Demo'd Title Screen
void F_TitleScreenTicker(boolean run)
{
	if (run)
		finalecount++;

	// Execute the titlemap camera settings
	if (titlemapinaction)
	{
		thinker_t *th;
		mobj_t *mo2;
		mobj_t *cameraref = NULL;

		// If there's a Line 422 Switch Cut-Away view, don't force us.
		if (!titlemapcameraref || titlemapcameraref->type != MT_ALTVIEWMAN)
		{
			for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
			{
				if (th->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
					continue;

				mo2 = (mobj_t *)th;

				if (!mo2)
					continue;

				if (mo2->type != MT_ALTVIEWMAN)
					continue;

				cameraref = titlemapcameraref = mo2;
				break;
			}
		}
		else
			cameraref = titlemapcameraref;

		if (cameraref)
		{
			camera.x = cameraref->x;
			camera.y = cameraref->y;
			camera.z = cameraref->z;
			camera.angle = cameraref->angle;
			camera.aiming = cameraref->cusval;
			camera.subsector = cameraref->subsector;
		}
		else
		{
			// Default behavior: Do a lil' camera spin if a title map is loaded;
			camera.angle += titlescrollxspeed*ANG1/64;
		}
	}

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

static skin_t *contskins[2];
static UINT8 cont_spr2[2][6];
static UINT8 *contcolormaps[2];

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
	paused = false;
	CON_ToggleOff();

	// In case menus are still up?!!
	M_ClearMenus(true);

	S_ChangeMusicInternal("_conti", false);
	S_StopSounds();

	contskins[0] = &skins[players[consoleplayer].skin];
	cont_spr2[0][0] = P_GetSkinSprite2(contskins[0], SPR2_CNT1, NULL);
	cont_spr2[0][2] = contskins[0]->contangle & 7;
	contcolormaps[0] = R_GetTranslationColormap(players[consoleplayer].skin, players[consoleplayer].skincolor, GTC_CACHE);
	cont_spr2[0][4] = contskins[0]->sprites[cont_spr2[0][0]].numframes;
	cont_spr2[0][5] = max(1, contskins[0]->contspeed);

	if (botskin)
	{
		INT32 secondplaya;

		if (secondarydisplayplayer != consoleplayer)
			secondplaya = secondarydisplayplayer;
		else // HACK
			secondplaya = 1;

		contskins[1] = &skins[players[secondplaya].skin];
		cont_spr2[1][0] = P_GetSkinSprite2(contskins[1], SPR2_CNT4, NULL);
		cont_spr2[1][2] = (contskins[1]->contangle >> 3) & 7;
		contcolormaps[1] = R_GetTranslationColormap(players[secondplaya].skin, players[secondplaya].skincolor, GTC_CACHE);
		cont_spr2[1][4] = contskins[1]->sprites[cont_spr2[1][0]].numframes;
		if (cont_spr2[1][0] == SPR2_CNT4)
			cont_spr2[1][5] = 4; // sorry, this one is hardcoded
		else
			cont_spr2[1][5] = max(1, contskins[1]->contspeed);
	}
	else
	{
		contskins[1] = NULL;
		contcolormaps[1] = NULL;
		cont_spr2[1][0] = cont_spr2[1][2] = cont_spr2[1][4] = cont_spr2[1][5] = 0;
	}

	cont_spr2[0][1] = cont_spr2[0][3] =\
	cont_spr2[1][1] = cont_spr2[1][3] = 0;

	timetonext = (11*TICRATE)+11;
	continuetime = 0;
}

//
// F_ContinueDrawer
// Moved continuing out of the HUD (hack removal!!)
//
void F_ContinueDrawer(void)
{
	spritedef_t *sprdef;
	spriteframe_t *sprframe;
	patch_t *patch;
	INT32 i, x = (BASEVIDWIDTH>>1), ncontinues = players[consoleplayer].continues;
	char numbuf[9] = "CONTNUM*";
	tic_t timeleft = (timetonext/TICRATE);
	INT32 offsx = 0, offsy = 0, lift[2] = {0, 0};

	if (continuetime >= 3*TICRATE)
	{
		V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 0);
		return;
	}

	V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 31);

	if (timetonext >= (11*TICRATE)+10)
		return;

	V_DrawLevelTitle(x - (V_LevelNameWidth("CONTINUE")>>1), 16, 0, "CONTINUE");

	// Two stars...
	patch = W_CachePatchName("CONTSTAR", PU_CACHE);
	V_DrawScaledPatch(x-32, 160, 0, patch);
	V_DrawScaledPatch(x+32, 160, 0, patch);

	// Time left!
	if (timeleft > 9)
	{
		numbuf[7] = '1';
		V_DrawScaledPatch(x - 10, 160, 0, W_CachePatchName(numbuf, PU_CACHE));
		numbuf[7] = '0';
		V_DrawScaledPatch(x + 10, 160, 0, W_CachePatchName(numbuf, PU_CACHE));
	}
	else
	{
		numbuf[7] = '0'+timeleft;
		V_DrawScaledPatch(x, 160, 0, W_CachePatchName(numbuf, PU_CACHE));
	}

	// Draw the continue markers! Show continues.
	if (ncontinues > 10)
	{
		if (!(continuetime & 1) || continuetime > 17)
			V_DrawContinueIcon(x, 68, 0, players[consoleplayer].skin, players[consoleplayer].skincolor);
		V_DrawScaledPatch(x+12, 68-2, 0, stlivex);
		V_DrawRightAlignedString(x+36, 69-5, 0,
			va("%d",(imcontinuing ? ncontinues-1 : ncontinues)));
	}
	else
	{
		x += (ncontinues/2) * 30;
		if (!(ncontinues & 1))
			x -= 15;
		for (i = 0; i < ncontinues; ++i)
		{
			if (i == (ncontinues/2) && ((continuetime & 1) || continuetime > 17))
				continue;
			V_DrawContinueIcon(x - (i*30), 68, 0, players[consoleplayer].skin, players[consoleplayer].skincolor);
		}
		x = BASEVIDWIDTH>>1;
	}

	// Spotlight
	V_DrawScaledPatch(x, 140, 0, W_CachePatchName("CONTSPOT", PU_CACHE));

	// warping laser
	if (continuetime)
	{
		INT32 w = min(continuetime, 28), brightness = (continuetime>>1) & 7;
		if (brightness > 3)
			brightness = 8-brightness;
		V_DrawFadeFill(x-w, 0, w<<1, 140, 0, 0, (3+brightness));
	}

	if (contskins[1])
	{
		if (continuetime > 15)
		{
			angle_t work = FixedAngle((10*(continuetime-15))<<FRACBITS)>>ANGLETOFINESHIFT;
			offsy = FINESINE(work)<<1;
			offsx = (27*FINECOSINE(work))>>1;
		}
		else
			offsx = 27<<(FRACBITS-1);
		lift[1] = continuetime-10;
		if (lift[1] < 0)
			lift[1] = 0;
		else if (lift[1] > TICRATE+5)
			lift[1] = TICRATE+5;
	}

	lift[0] = continuetime-5;
	if (lift[0] < 0)
		lift[0] = 0;
	else if (lift[0] > TICRATE+5)
		lift[0] = TICRATE+5;

#define drawchar(dx, dy, n)	{\
								sprdef = &contskins[n]->sprites[cont_spr2[n][0]];\
								sprframe = &sprdef->spriteframes[cont_spr2[n][1]];\
								patch = W_CachePatchNum(sprframe->lumppat[cont_spr2[n][2]], PU_CACHE);\
								V_DrawFixedPatch((dx), (dy), FRACUNIT, (sprframe->flip & (1<<cont_spr2[n][2])) ? V_FLIP : 0, patch, contcolormaps[n]);\
							}

	if (offsy < 0)
		drawchar((BASEVIDWIDTH<<(FRACBITS-1))-offsx, ((140-lift[0])<<FRACBITS)-offsy, 0);
	if (contskins[1])
		drawchar((BASEVIDWIDTH<<(FRACBITS-1))+offsx, ((140-lift[1])<<FRACBITS)+offsy, 1);
	if (offsy >= 0)
		drawchar((BASEVIDWIDTH<<(FRACBITS-1))-offsx, ((140-lift[0])<<FRACBITS)-offsy, 0);

#undef drawchar

	if (timetonext > (11*TICRATE))
		V_DrawFadeScreen(31, timetonext-(11*TICRATE));
	if (continuetime > ((3*TICRATE) - 10))
		V_DrawFadeScreen(0, (continuetime - ((3*TICRATE) - 10)));
}

void F_ContinueTicker(void)
{
	if (!imcontinuing)
	{
		if (timetonext > 0)
		{
			if (!(--timetonext))
			{
				Command_ExitGame_f();
				return;
			}
		}
	}
	else
	{
		if (++continuetime == 3*TICRATE)
		{
			G_Continue();
			return;
		}

		if (continuetime > 5 && ((continuetime & 1) || continuetime > TICRATE) && (++cont_spr2[0][2]) >= 8)
			cont_spr2[0][2] = 0;

		if (continuetime > 10 && (!(continuetime & 1) || continuetime > TICRATE+5) && (++cont_spr2[1][2]) >= 8)
			cont_spr2[1][2] = 0;

		if (continuetime == (3*TICRATE)-10)
			S_StartSound(NULL, sfx_cdfm56); // or 31
		else if (continuetime == 5)
		{
			cont_spr2[0][0] = P_GetSkinSprite2(contskins[0], SPR2_CNT2, NULL);
			cont_spr2[0][4] = contskins[0]->sprites[cont_spr2[0][0]].numframes;
			cont_spr2[0][1] = cont_spr2[0][3] = 0;
			cont_spr2[0][5] = 2;
		}
		else if (continuetime == TICRATE)
		{
			cont_spr2[0][0] = P_GetSkinSprite2(contskins[0], SPR2_CNT3, NULL);
			cont_spr2[0][4] = contskins[0]->sprites[cont_spr2[0][0]].numframes;
			cont_spr2[0][1] = cont_spr2[0][3] = 0;
		}
		else if (contskins[1])
		{
			if (continuetime == 10)
			{
				cont_spr2[1][0] = P_GetSkinSprite2(contskins[1], SPR2_CNT2, NULL);
				cont_spr2[1][4] = contskins[1]->sprites[cont_spr2[1][0]].numframes;
				cont_spr2[1][1] = cont_spr2[1][3] = 0;
				cont_spr2[1][5] = 2;
			}
			else if (continuetime == TICRATE+5)
			{
				cont_spr2[1][0] = P_GetSkinSprite2(contskins[1], SPR2_CNT3, NULL);
				cont_spr2[1][4] = contskins[1]->sprites[cont_spr2[1][0]].numframes;
				cont_spr2[1][1] = cont_spr2[1][3] = 0;
			}
		}
	}

	if ((++cont_spr2[0][3]) >= cont_spr2[0][5])
	{
		cont_spr2[0][3] = 0;
		if (++cont_spr2[0][1] >= cont_spr2[0][4])
			cont_spr2[0][1] = 0;
	}

	if (contskins[1] && (++cont_spr2[1][3]) >= cont_spr2[1][5])
	{
		cont_spr2[1][3] = 0;
		if (++cont_spr2[1][1] >= cont_spr2[1][4])
			cont_spr2[1][1] = 0;
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
	S_StartSound(NULL, sfx_kc6b);
	I_FadeSong(0, MUSICRATE, &S_StopMusic);

	return true;
}

// ==================
//  CUSTOM CUTSCENES
// ==================
static INT32 scenenum, cutnum;
static INT32 picxpos, picypos, picnum, pictime, picmode, numpics, pictoloop;
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
		S_ChangeMusicEx(cutscenes[cutnum]->scene[scenenum].musswitch,
			cutscenes[cutnum]->scene[scenenum].musswitchflags,
			cutscenes[cutnum]->scene[scenenum].musicloop,
			cutscenes[cutnum]->scene[scenenum].musswitchposition, 0, 0);

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
	cutsceneover = true; // do this first, just in case G_EndGame or something wants to turn it back false later
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
			G_EndGame();
	}
}

void F_StartCustomCutscene(INT32 cutscenenum, boolean precutscene, boolean resetplayer)
{
	if (!cutscenes[cutscenenum])
		return;

	G_SetGamestate(GS_CUTSCENE);

	if (wipegamestate == GS_CUTSCENE)
		wipegamestate = -1;

	gameaction = ga_nothing;
	paused = false;
	CON_ToggleOff();

	F_NewCutscene(cutscenes[cutscenenum]->scene[0].text);

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
		S_ChangeMusicEx(cutscenes[cutnum]->scene[0].musswitch,
			cutscenes[cutnum]->scene[0].musswitchflags,
			cutscenes[cutnum]->scene[0].musicloop,
			cutscenes[cutnum]->scene[scenenum].musswitchposition, 0, 0);
	else
		S_StopMusic();
	S_StopSounds();
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

	V_DrawString(textxpos, textypos, V_ALLOWLOWERCASE, cutscene_disptext);
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
		if (netgame && i != serverplayer && !IsPlayerAdmin(i))
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

// ==================
//  TEXT PROMPTS
// ==================

static void F_GetPageTextGeometry(UINT8 *pagelines, boolean *rightside, INT32 *boxh, INT32 *texth, INT32 *texty, INT32 *namey, INT32 *chevrony, INT32 *textx, INT32 *textr)
{
	// reuse:
	// cutnum -> promptnum
	// scenenum -> pagenum
	lumpnum_t iconlump = W_CheckNumForName(textprompts[cutnum]->page[scenenum].iconname);

	*pagelines = textprompts[cutnum]->page[scenenum].lines ? textprompts[cutnum]->page[scenenum].lines : 4;
	*rightside = (iconlump != LUMPERROR && textprompts[cutnum]->page[scenenum].rightside);

	// Vertical calculations
	*boxh = *pagelines*2;
	*texth = textprompts[cutnum]->page[scenenum].name[0] ? (*pagelines-1)*2 : *pagelines*2; // name takes up first line if it exists
	*texty = BASEVIDHEIGHT - ((*texth * 4) + (*texth/2)*4);
	*namey = BASEVIDHEIGHT - ((*boxh * 4) + (*boxh/2)*4);
	*chevrony = BASEVIDHEIGHT - (((1*2) * 4) + ((1*2)/2)*4); // force on last line

	// Horizontal calculations
	// Shift text to the right if we have a character icon on the left side
	// Add 4 margin against icon
	*textx = (iconlump != LUMPERROR && !*rightside) ? ((*boxh * 4) + (*boxh/2)*4) + 4 : 4;
	*textr = *rightside ? BASEVIDWIDTH - (((*boxh * 4) + (*boxh/2)*4) + 4) : BASEVIDWIDTH-4;
}

static fixed_t F_GetPromptHideHudBound(void)
{
	UINT8 pagelines;
	boolean rightside;
	INT32 boxh, texth, texty, namey, chevrony;
	INT32 textx, textr;

	if (cutnum == INT32_MAX || scenenum == INT32_MAX || !textprompts[cutnum] || scenenum >= textprompts[cutnum]->numpages ||
		!textprompts[cutnum]->page[scenenum].hidehud ||
		(splitscreen && textprompts[cutnum]->page[scenenum].hidehud != 2)) // don't hide on splitscreen, unless hide all is forced
		return 0;
	else if (textprompts[cutnum]->page[scenenum].hidehud == 2) // hide all
		return BASEVIDHEIGHT;

	F_GetPageTextGeometry(&pagelines, &rightside, &boxh, &texth, &texty, &namey, &chevrony, &textx, &textr);

	// calc boxheight (see V_DrawPromptBack)
	boxh *= vid.dupy;
	boxh = (boxh * 4) + (boxh/2)*5; // 4 lines of space plus gaps between and some leeway

	// return a coordinate to check
	// if negative: don't show hud elements below this coordinate (visually)
	// if positive: don't show hud elements above this coordinate (visually)
	return 0 - boxh; // \todo: if prompt at top of screen (someday), make this return positive
}

boolean F_GetPromptHideHudAll(void)
{
	if (cutnum == INT32_MAX || scenenum == INT32_MAX || !textprompts[cutnum] || scenenum >= textprompts[cutnum]->numpages ||
		!textprompts[cutnum]->page[scenenum].hidehud ||
		(splitscreen && textprompts[cutnum]->page[scenenum].hidehud != 2)) // don't hide on splitscreen, unless hide all is forced
		return false;
	else if (textprompts[cutnum]->page[scenenum].hidehud == 2) // hide all
		return true;
	else
		return false;
}

boolean F_GetPromptHideHud(fixed_t y)
{
	INT32 ybound;
	boolean fromtop;
	fixed_t ytest;

	if (!promptactive)
		return false;

	ybound = F_GetPromptHideHudBound();
	fromtop = (ybound >= 0);
	ytest = (fromtop ? ybound : BASEVIDHEIGHT + ybound);

	return (fromtop ? y < ytest : y >= ytest); // true means hide
}

static void F_PreparePageText(char *pagetext)
{
	UINT8 pagelines;
	boolean rightside;
	INT32 boxh, texth, texty, namey, chevrony;
	INT32 textx, textr;

	F_GetPageTextGeometry(&pagelines, &rightside, &boxh, &texth, &texty, &namey, &chevrony, &textx, &textr);

	if (promptpagetext)
		Z_Free(promptpagetext);
	promptpagetext = (pagetext && pagetext[0]) ? V_WordWrap(textx, textr, 0, pagetext) : Z_StrDup("");

	F_NewCutscene(promptpagetext);
	cutscene_textspeed = textprompts[cutnum]->page[scenenum].textspeed ? textprompts[cutnum]->page[scenenum].textspeed : TICRATE/5;
	cutscene_textcount = 0; // no delay in beginning
	cutscene_boostspeed = 0; // don't print 8 characters to start

	// \todo update control hot strings on re-config
	// and somehow don't reset cutscene text counters
}

static void F_AdvanceToNextPage(void)
{
	INT32 nextprompt = textprompts[cutnum]->page[scenenum].nextprompt ? textprompts[cutnum]->page[scenenum].nextprompt - 1 : INT32_MAX,
		nextpage = textprompts[cutnum]->page[scenenum].nextpage ? textprompts[cutnum]->page[scenenum].nextpage - 1 : INT32_MAX,
		oldcutnum = cutnum;

	if (textprompts[cutnum]->page[scenenum].nexttag[0])
		F_GetPromptPageByNamedTag(textprompts[cutnum]->page[scenenum].nexttag, &nextprompt, &nextpage);

	// determine next prompt
	if (nextprompt != INT32_MAX)
	{
		if (nextprompt <= MAX_PROMPTS && textprompts[nextprompt])
			cutnum = nextprompt;
		else
			cutnum = INT32_MAX;
	}

	// determine next page
	if (nextpage != INT32_MAX)
	{
		if (cutnum != INT32_MAX)
		{
			scenenum = nextpage;
			if (scenenum >= MAX_PAGES || scenenum > textprompts[cutnum]->numpages-1)
				scenenum = INT32_MAX;
		}
	}
	else
	{
		if (cutnum != oldcutnum)
			scenenum = 0;
		else if (scenenum + 1 < MAX_PAGES && scenenum < textprompts[cutnum]->numpages-1)
			scenenum++;
		else
			scenenum = INT32_MAX;
	}

	// close the prompt if either num is invalid
	if (cutnum == INT32_MAX || scenenum == INT32_MAX)
		F_EndTextPrompt(false, false);
	else
	{
		// on page mode, number of tics before allowing boost
		// on timer mode, number of tics until page advances
		timetonext = textprompts[cutnum]->page[scenenum].timetonext ? textprompts[cutnum]->page[scenenum].timetonext : TICRATE/10;
		F_PreparePageText(textprompts[cutnum]->page[scenenum].text);

		// gfx
		picnum = textprompts[cutnum]->page[scenenum].pictostart;
		numpics = textprompts[cutnum]->page[scenenum].numpics;
		picmode = textprompts[cutnum]->page[scenenum].picmode;
		pictoloop = textprompts[cutnum]->page[scenenum].pictoloop > 0 ? textprompts[cutnum]->page[scenenum].pictoloop - 1 : 0;
		picxpos = textprompts[cutnum]->page[scenenum].xcoord[picnum];
		picypos = textprompts[cutnum]->page[scenenum].ycoord[picnum];
		animtimer = pictime = textprompts[cutnum]->page[scenenum].picduration[picnum];

		// music change
		if (textprompts[cutnum]->page[scenenum].musswitch[0])
			S_ChangeMusic(textprompts[cutnum]->page[scenenum].musswitch,
				textprompts[cutnum]->page[scenenum].musswitchflags,
				textprompts[cutnum]->page[scenenum].musicloop);
	}
}

void F_EndTextPrompt(boolean forceexec, boolean noexec)
{
	boolean promptwasactive = promptactive;
	promptactive = false;
	callpromptnum = callpagenum = callplayer = INT32_MAX;

	if (promptwasactive)
	{
		if (promptmo && promptmo->player && promptblockcontrols)
			promptmo->reactiontime = TICRATE/4; // prevent jumping right away // \todo account freeze realtime for this)
		// \todo reset frozen realtime?
	}

	// \todo net safety, maybe loop all player thinkers?
	if ((promptwasactive || forceexec) && !noexec && promptpostexectag)
	{
		if (tmthing) // edge case where starting an invalid prompt immediately on level load will make P_MapStart fail
			P_LinedefExecute(promptpostexectag, promptmo, NULL);
		else
		{
			P_MapStart();
			P_LinedefExecute(promptpostexectag, promptmo, NULL);
			P_MapEnd();
		}
	}
}

void F_StartTextPrompt(INT32 promptnum, INT32 pagenum, mobj_t *mo, UINT16 postexectag, boolean blockcontrols, boolean freezerealtime)
{
	INT32 i;

	// if splitscreen and we already have a prompt active, ignore.
	// \todo Proper per-player splitscreen support (individual prompts)
	if (promptactive && splitscreen && promptnum == callpromptnum && pagenum == callpagenum)
		return;

	// \todo proper netgame support
	if (netgame)
	{
		F_EndTextPrompt(true, false); // run the post-effects immediately
		return;
	}

	// We share vars, so no starting text prompts over cutscenes or title screens!
	keypressed = false;
	finalecount = 0;
	timetonext = 0;
	animtimer = 0;
	stoptimer = 0;
	skullAnimCounter = 0;

	// Set up state
	promptmo = mo;
	promptpostexectag = postexectag;
	promptblockcontrols = blockcontrols;
	(void)freezerealtime; // \todo freeze player->realtime, maybe this needs to cycle through player thinkers

	// Initialize current prompt and scene
	callpromptnum = promptnum;
	callpagenum = pagenum;
	cutnum = (promptnum < MAX_PROMPTS && textprompts[promptnum]) ? promptnum : INT32_MAX;
	scenenum = (cutnum != INT32_MAX && pagenum < MAX_PAGES && pagenum <= textprompts[cutnum]->numpages-1) ? pagenum : INT32_MAX;
	promptactive = (cutnum != INT32_MAX && scenenum != INT32_MAX);

	if (promptactive)
	{
		// on page mode, number of tics before allowing boost
		// on timer mode, number of tics until page advances
		timetonext = textprompts[cutnum]->page[scenenum].timetonext ? textprompts[cutnum]->page[scenenum].timetonext : TICRATE/10;
		F_PreparePageText(textprompts[cutnum]->page[scenenum].text);

		// gfx
		picnum = textprompts[cutnum]->page[scenenum].pictostart;
		numpics = textprompts[cutnum]->page[scenenum].numpics;
		picmode = textprompts[cutnum]->page[scenenum].picmode;
		pictoloop = textprompts[cutnum]->page[scenenum].pictoloop > 0 ? textprompts[cutnum]->page[scenenum].pictoloop - 1 : 0;
		picxpos = textprompts[cutnum]->page[scenenum].xcoord[picnum];
		picypos = textprompts[cutnum]->page[scenenum].ycoord[picnum];
		animtimer = pictime = textprompts[cutnum]->page[scenenum].picduration[picnum];

		// music change
		if (textprompts[cutnum]->page[scenenum].musswitch[0])
			S_ChangeMusic(textprompts[cutnum]->page[scenenum].musswitch,
				textprompts[cutnum]->page[scenenum].musswitchflags,
				textprompts[cutnum]->page[scenenum].musicloop);

		// get the calling player
		if (promptblockcontrols && mo && mo->player)
		{
			for (i = 0; i < MAXPLAYERS; i++)
			{
				if (players[i].mo == mo)
				{
					callplayer = i;
					break;
				}
			}
		}
	}
	else
		F_EndTextPrompt(true, false); // run the post-effects immediately
}

static boolean F_GetTextPromptTutorialTag(char *tag, INT32 length)
{
	INT32 gcs = gcs_custom;
	boolean suffixed = true;

	if (!tag || !tag[0] || !tutorialmode)
		return false;

	if (!strncmp(tag, "TAM", 3)) // Movement
		gcs = G_GetControlScheme(gamecontrol, gcl_movement, num_gcl_movement);
	else if (!strncmp(tag, "TAC", 3)) // Camera
	{
		// Check for gcl_movement so we can differentiate between FPS and Platform schemes.
		gcs = G_GetControlScheme(gamecontrol, gcl_movement, num_gcl_movement);
		if (gcs == gcs_custom) // try again, maybe we'll get a match
			gcs = G_GetControlScheme(gamecontrol, gcl_camera, num_gcl_camera);
		if (gcs == gcs_fps && !cv_usemouse.value)
			gcs = gcs_platform; // Platform (arrow) scheme is stand-in for no mouse
	}
	else if (!strncmp(tag, "TAD", 3)) // Movement and Camera
		gcs = G_GetControlScheme(gamecontrol, gcl_movement_camera, num_gcl_movement_camera);
	else if (!strncmp(tag, "TAJ", 3)) // Jump
		gcs = G_GetControlScheme(gamecontrol, gcl_jump, num_gcl_jump);
	else if (!strncmp(tag, "TAS", 3)) // Spin
		gcs = G_GetControlScheme(gamecontrol, gcl_use, num_gcl_use);
	else if (!strncmp(tag, "TAA", 3)) // Char ability
		gcs = G_GetControlScheme(gamecontrol, gcl_jump, num_gcl_jump);
	else if (!strncmp(tag, "TAW", 3)) // Shield ability
		gcs = G_GetControlScheme(gamecontrol, gcl_jump_use, num_gcl_jump_use);
	else
		gcs = G_GetControlScheme(gamecontrol, gcl_tutorial_used, num_gcl_tutorial_used);

	switch (gcs)
	{
		case gcs_fps:
			// strncat(tag, "FPS", length);
			suffixed = false;
			break;

		case gcs_platform:
			strncat(tag, "PLATFORM", length);
			break;

		default:
			strncat(tag, "CUSTOM", length);
			break;
	}

	return suffixed;
}

void F_GetPromptPageByNamedTag(const char *tag, INT32 *promptnum, INT32 *pagenum)
{
	INT32 nosuffixpromptnum = INT32_MAX, nosuffixpagenum = INT32_MAX;
	INT32 tutorialpromptnum = (tutorialmode) ? TUTORIAL_PROMPT-1 : 0;
	boolean suffixed = false, found = false;
	char suffixedtag[33];

	*promptnum = *pagenum = INT32_MAX;

	if (!tag || !tag[0])
		return;

	strncpy(suffixedtag, tag, 33);
	suffixedtag[32] = 0;

	if (tutorialmode)
		suffixed = F_GetTextPromptTutorialTag(suffixedtag, 33);

	for (*promptnum = 0 + tutorialpromptnum; *promptnum < MAX_PROMPTS; (*promptnum)++)
	{
		if (!textprompts[*promptnum])
			continue;

		for (*pagenum = 0; *pagenum < textprompts[*promptnum]->numpages && *pagenum < MAX_PAGES; (*pagenum)++)
		{
			if (suffixed && fastcmp(suffixedtag, textprompts[*promptnum]->page[*pagenum].tag))
			{
				// this goes first because fastcmp ends early if first string is shorter
				found = true;
				break;
			}
			else if (nosuffixpromptnum == INT32_MAX && nosuffixpagenum == INT32_MAX && fastcmp(tag, textprompts[*promptnum]->page[*pagenum].tag))
			{
				if (suffixed)
				{
					nosuffixpromptnum = *promptnum;
					nosuffixpagenum = *pagenum;
					// continue searching for the suffixed tag
				}
				else
				{
					found = true;
					break;
				}
			}
		}

		if (found)
			break;
	}

	if (suffixed && !found && nosuffixpromptnum != INT32_MAX && nosuffixpagenum != INT32_MAX)
	{
		found = true;
		*promptnum = nosuffixpromptnum;
		*pagenum = nosuffixpagenum;
	}

	if (!found)
		CONS_Debug(DBG_GAMELOGIC, "Text prompt: Can't find a page with named tag %s or suffixed tag %s\n", tag, suffixedtag);
}

void F_TextPromptDrawer(void)
{
	// reuse:
	// cutnum -> promptnum
	// scenenum -> pagenum
	lumpnum_t iconlump;
	UINT8 pagelines;
	boolean rightside;
	INT32 boxh, texth, texty, namey, chevrony;
	INT32 textx, textr;

	// Data
	patch_t *patch;

	if (!promptactive)
		return;

	iconlump = W_CheckNumForName(textprompts[cutnum]->page[scenenum].iconname);
	F_GetPageTextGeometry(&pagelines, &rightside, &boxh, &texth, &texty, &namey, &chevrony, &textx, &textr);

	// Draw gfx first
	if (picnum >= 0 && picnum < numpics && textprompts[cutnum]->page[scenenum].picname[picnum][0] != '\0')
	{
		if (textprompts[cutnum]->page[scenenum].pichires[picnum])
			V_DrawSmallScaledPatch(picxpos, picypos, 0,
				W_CachePatchName(textprompts[cutnum]->page[scenenum].picname[picnum], PU_CACHE));
		else
			V_DrawScaledPatch(picxpos,picypos, 0,
				W_CachePatchName(textprompts[cutnum]->page[scenenum].picname[picnum], PU_CACHE));
	}

	// Draw background
	V_DrawPromptBack(boxh, textprompts[cutnum]->page[scenenum].backcolor);

	// Draw narrator icon
	if (iconlump != LUMPERROR)
	{
		INT32 iconx, icony, scale, scaledsize;
		patch = W_CachePatchName(textprompts[cutnum]->page[scenenum].iconname, PU_CACHE);

		// scale and center
		if (patch->width > patch->height)
		{
			scale = FixedDiv(((boxh * 4) + (boxh/2)*4) - 4, patch->width);
			scaledsize = FixedMul(patch->height, scale);
			iconx = (rightside ? BASEVIDWIDTH - (((boxh * 4) + (boxh/2)*4)) : 4) << FRACBITS;
			icony = ((namey-4) << FRACBITS) + FixedDiv(BASEVIDHEIGHT - namey + 4 - scaledsize, 2); // account for 4 margin
		}
		else if (patch->height > patch->width)
		{
			scale = FixedDiv(((boxh * 4) + (boxh/2)*4) - 4, patch->height);
			scaledsize = FixedMul(patch->width, scale);
			iconx = (rightside ? BASEVIDWIDTH - (((boxh * 4) + (boxh/2)*4)) : 4) << FRACBITS;
			icony = namey << FRACBITS;
			iconx += FixedDiv(FixedMul(patch->height, scale) - scaledsize, 2);
		}
		else
		{
			scale = FixedDiv(((boxh * 4) + (boxh/2)*4) - 4, patch->width);
			iconx = (rightside ? BASEVIDWIDTH - (((boxh * 4) + (boxh/2)*4)) : 4) << FRACBITS;
			icony = namey << FRACBITS;
		}

		if (textprompts[cutnum]->page[scenenum].iconflip)
			iconx += FixedMul(patch->width, scale) << FRACBITS;

		V_DrawFixedPatch(iconx, icony, scale, (V_SNAPTOBOTTOM|(textprompts[cutnum]->page[scenenum].iconflip ? V_FLIP : 0)), patch, NULL);
		W_UnlockCachedPatch(patch);
	}

	// Draw text
	V_DrawString(textx, texty, (V_SNAPTOBOTTOM|V_ALLOWLOWERCASE), cutscene_disptext);

	// Draw name
	// Don't use V_YELLOWMAP here so that the name color can be changed with control codes
	if (textprompts[cutnum]->page[scenenum].name[0])
		V_DrawString(textx, namey, (V_SNAPTOBOTTOM|V_ALLOWLOWERCASE), textprompts[cutnum]->page[scenenum].name);

	// Draw chevron
	if (promptblockcontrols && !timetonext)
		V_DrawString(textr-8, chevrony + (skullAnimCounter/5), (V_SNAPTOBOTTOM|V_YELLOWMAP), "\x1B"); // down arrow
}

void F_TextPromptTicker(void)
{
	INT32 i;

	if (!promptactive || paused || P_AutoPause())
		return;

	// advance animation
	finalecount++;
	cutscene_boostspeed = 0;

	// for the chevron
	if (--skullAnimCounter <= 0)
		skullAnimCounter = 8;

	// button handling
	if (textprompts[cutnum]->page[scenenum].timetonext)
	{
		if (promptblockcontrols) // same procedure as below, just without the button handling
		{
			for (i = 0; i < MAXPLAYERS; i++)
			{
				if (netgame && i != serverplayer && !IsPlayerAdmin(i))
					continue;
				else if (splitscreen) {
					// Both players' controls are locked,
					// But only consoleplayer can advance the prompt.
					// \todo Proper per-player splitscreen support (individual prompts)
					if (i == consoleplayer || i == secondarydisplayplayer)
						players[i].powers[pw_nocontrol] = 1;
				}
				else if (i == consoleplayer)
					players[i].powers[pw_nocontrol] = 1;

				if (!splitscreen)
					break;
			}
		}

		if (timetonext >= 1)
			timetonext--;

		if (!timetonext)
			F_AdvanceToNextPage();

		F_WriteText();
	}
	else
	{
		if (promptblockcontrols)
		{
			for (i = 0; i < MAXPLAYERS; i++)
			{
				if (netgame && i != serverplayer && !IsPlayerAdmin(i))
					continue;
				else if (splitscreen) {
					// Both players' controls are locked,
					// But only the triggering player can advance the prompt.
					if (i == consoleplayer || i == secondarydisplayplayer)
					{
						players[i].powers[pw_nocontrol] = 1;

						if (callplayer == consoleplayer || callplayer == secondarydisplayplayer)
						{
							if (i != callplayer)
								continue;
						}
						else if (i != consoleplayer)
							continue;
					}
					else
						continue;
				}
				else if (i == consoleplayer)
					players[i].powers[pw_nocontrol] = 1;
				else
					continue;

				if ((players[i].cmd.buttons & BT_USE) || (players[i].cmd.buttons & BT_JUMP))
				{
					if (timetonext > 1)
						timetonext--;
					else if (cutscene_baseptr) // don't set boost if we just reset the string
						cutscene_boostspeed = 1; // only after a slight delay

					if (keypressed)
					{
						if (!splitscreen)
							break;
						else
							continue;
					}

					if (!timetonext) // is 0 when finished generating text
					{
						F_AdvanceToNextPage();
						if (promptactive)
							S_StartSound(NULL, sfx_menu1);
					}
					keypressed = true; // prevent repeat events
				}
				else if (!(players[i].cmd.buttons & BT_USE) && !(players[i].cmd.buttons & BT_JUMP))
					keypressed = false;

				if (!splitscreen)
					break;
			}
		}

		// generate letter-by-letter text
		if (scenenum >= MAX_PAGES ||
			!textprompts[cutnum]->page[scenenum].text ||
			!textprompts[cutnum]->page[scenenum].text[0] ||
			!F_WriteText())
			timetonext = !promptblockcontrols; // never show the chevron if we can't toggle pages
	}

	// gfx
	if (picnum >= 0 && picnum < numpics)
	{
		if (animtimer <= 0)
		{
			boolean persistanimtimer = false;

			if (picnum < numpics-1 && textprompts[cutnum]->page[scenenum].picname[picnum+1][0] != '\0')
				picnum++;
			else if (picmode == PROMPT_PIC_LOOP)
				picnum = pictoloop;
			else if (picmode == PROMPT_PIC_DESTROY)
				picnum = -1;
			else // if (picmode == PROMPT_PIC_PERSIST)
				persistanimtimer = true;

			if (!persistanimtimer && picnum >= 0)
			{
				picxpos = textprompts[cutnum]->page[scenenum].xcoord[picnum];
				picypos = textprompts[cutnum]->page[scenenum].ycoord[picnum];
				pictime = textprompts[cutnum]->page[scenenum].picduration[picnum];
				animtimer = pictime;
			}
		}
		else
			animtimer--;
	}
}

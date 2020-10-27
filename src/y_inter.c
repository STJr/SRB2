// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2004-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  y_inter.c
/// \brief Tally screens, or "Intermissions" as they were formally called in Doom

#include "doomdef.h"
#include "doomstat.h"
#include "d_main.h"
#include "f_finale.h"
#include "g_game.h"
#include "hu_stuff.h"
#include "i_net.h"
#include "i_video.h"
#include "p_tick.h"
#include "r_defs.h"
#include "r_skins.h"
#include "s_sound.h"
#include "st_stuff.h"
#include "v_video.h"
#include "w_wad.h"
#include "y_inter.h"
#include "z_zone.h"
#include "m_menu.h"
#include "m_misc.h"
#include "i_system.h"
#include "p_setup.h"

#include "r_local.h"
#include "p_local.h"

#include "m_cond.h" // condition sets
#include "lua_hook.h" // IntermissionThinker hook

#include "lua_hud.h"

#ifdef HWRENDER
#include "hardware/hw_main.h"
#endif

typedef struct
{
	char patch[9];
	 INT32 points;
	UINT8 display;
} y_bonus_t;

typedef union
{
	struct
	{
		char passed1[21]; // KNUCKLES GOT    / CRAWLA HONCHO
		char passed2[16]; // THROUGH THE ACT / PASSED THE ACT
		INT32 passedx1;
		INT32 passedx2;

		y_bonus_t bonuses[4];
		patch_t *bonuspatches[4];

		SINT8 gotperfbonus; // Used for visitation flags.

		UINT32 score, total; // fake score, total
		UINT32 tics; // time

		UINT8 actnum; // act number being displayed
		patch_t *ptotal; // TOTAL
		UINT8 gotlife; // Number of extra lives obtained
	} coop;

	struct
	{
		char passed1[29];             // KNUCKLES GOT     / CRAWLA HONCHO
		char passed2[17];             // A CHAOS EMERALD? / GOT THEM ALL!
		char passed3[15];             //                   CAN NOW BECOME
		char passed4[SKINNAMESIZE+7]; //                   SUPER CRAWLA HONCHO
		INT32 passedx1;
		INT32 passedx2;
		INT32 passedx3;
		INT32 passedx4;

		y_bonus_t bonuses[2];
		patch_t *bonuspatches[2];

		patch_t *pscore; // SCORE
		UINT32 score; // fake score

		// Continues
		UINT8 continues;
		patch_t *pcontinues;
		INT32 *playerchar; // Continue HUD
		UINT16 *playercolor;

		UINT8 gotlife; // Number of extra lives obtained
	} spec;

	struct
	{
		UINT32 scores[MAXPLAYERS]; // Winner's score
		UINT16 *color[MAXPLAYERS]; // Winner's color #
		boolean spectator[MAXPLAYERS]; // Spectator list
		INT32 *character[MAXPLAYERS]; // Winner's character #
		INT32 num[MAXPLAYERS]; // Winner's player #
		char *name[MAXPLAYERS]; // Winner's name
		patch_t *result; // RESULT
		patch_t *blueflag;
		patch_t *redflag; // int_ctf uses this struct too.
		INT32 numplayers; // Number of players being displayed
		char levelstring[40]; // holds levelnames up to 32 characters
	} match;

	struct
	{
		UINT16 *color[MAXPLAYERS]; // Winner's color #
		INT32 *character[MAXPLAYERS]; // Winner's character #
		INT32 num[MAXPLAYERS]; // Winner's player #
		char name[MAXPLAYERS][9]; // Winner's name
		UINT32 times[MAXPLAYERS];
		UINT32 rings[MAXPLAYERS];
		UINT32 maxrings[MAXPLAYERS];
		UINT32 monitors[MAXPLAYERS];
		UINT32 scores[MAXPLAYERS];
		UINT32 points[MAXPLAYERS];
		INT32 numplayers; // Number of players being displayed
		char levelstring[40]; // holds levelnames up to 32 characters
	} competition;

} y_data;

static y_data data;

// graphics
static patch_t *bgpatch = NULL;     // INTERSCR
static patch_t *bgtile = NULL;      // SPECTILE/SRB2BACK
static patch_t *interpic = NULL;    // custom picture defined in map header
static boolean usetile;
static INT32 timer;

typedef struct
{
	INT32 source_width, source_height;
	INT32 source_bpp, source_rowbytes;
	UINT8 *source_picture;
	INT32 target_width, target_height;
	INT32 target_bpp, target_rowbytes;
	UINT8 *target_picture;
} y_buffer_t;

boolean usebuffer = false;
static boolean useinterpic;
static boolean safetorender = true;
static y_buffer_t *y_buffer;

static INT32 intertic;
static INT32 tallydonetic = -1;
static INT32 endtic = -1;

intertype_t intertype = int_none;
intertype_t intermissiontypes[NUMGAMETYPES];

static void Y_RescaleScreenBuffer(void);
static void Y_AwardCoopBonuses(void);
static void Y_AwardSpecialStageBonus(void);
static void Y_CalculateCompetitionWinners(void);
static void Y_CalculateTimeRaceWinners(void);
static void Y_CalculateMatchWinners(void);
static void Y_UnloadData(void);
static void Y_CleanupData(void);

// Stuff copy+pasted from st_stuff.c
#define ST_DrawNumFromHud(h,n)        V_DrawTallNum(hudinfo[h].x, hudinfo[h].y, hudinfo[h].f, n)
#define ST_DrawPadNumFromHud(h,n,q)   V_DrawPaddedTallNum(hudinfo[h].x, hudinfo[h].y, hudinfo[h].f, n, q)
#define ST_DrawPatchFromHud(h,p)      V_DrawScaledPatch(hudinfo[h].x, hudinfo[h].y, hudinfo[h].f, p)

static void Y_IntermissionTokenDrawer(void)
{
	INT32 y, offs, lowy, calc;
	UINT32 tokencount;
	INT16 temp;
	UINT8 em;

	offs = 0;
	lowy = BASEVIDHEIGHT - 32 - 8;
	temp = SHORT(tokenicon->height)/2;

	em = 0;
	while (emeralds & (1 << em))
		if (++em == 7)
			return;

	if (tallydonetic != -1)
	{
		offs = (intertic - tallydonetic)*2;
		if (offs > 10)
			offs = 8;
	}

	V_DrawSmallScaledPatch(32, lowy-1, 0, emeraldpics[2][em]); // coinbox

	y = (lowy + offs + 1) - (temp + (token + 1)*8);

	for (tokencount = token; tokencount; tokencount--)
	{
		if (y >= -temp)
			V_DrawSmallScaledPatch(32, y, 0, tokenicon);
		y += 8;
	}

	y += (offs*(temp - 1)/8);
	calc = (lowy - y)*2;

	if (calc > 0)
		V_DrawCroppedPatch(32<<FRACBITS, y<<FRACBITS, FRACUNIT/2, 0, tokenicon, 0, 0, SHORT(tokenicon->width), calc);
}

//
// Y_ConsiderScreenBuffer
//
// Can we copy the current screen to a buffer?
//
void Y_ConsiderScreenBuffer(void)
{
	if (gameaction != ga_completed)
		return;

	if (y_buffer == NULL)
		y_buffer = Z_Calloc(sizeof(y_buffer_t), PU_STATIC, NULL);
	else
		return;

	y_buffer->source_width = vid.width;
	y_buffer->source_height = vid.height;
	y_buffer->source_bpp = vid.bpp;
	y_buffer->source_rowbytes = vid.rowbytes;
	y_buffer->source_picture = ZZ_Alloc(y_buffer->source_width*vid.bpp * y_buffer->source_height);
	VID_BlitLinearScreen(screens[1], y_buffer->source_picture, vid.width*vid.bpp, vid.height, vid.width*vid.bpp, vid.rowbytes);

	// Make the rescaled screen buffer
	Y_RescaleScreenBuffer();
}

//
// Y_RescaleScreenBuffer
//
// Write the rescaled source picture, to the destination picture that has the current screen's resolutions.
//
static void Y_RescaleScreenBuffer(void)
{
	INT32 sx, sy; // source
	INT32 dx, dy; // dest
	fixed_t scalefac, yscalefac;
	fixed_t rowfrac, colfrac;
	UINT8 *dest;

	// Who knows?
	if (y_buffer == NULL)
		return;

	if (y_buffer->target_picture)
		Z_Free(y_buffer->target_picture);

	y_buffer->target_width = vid.width;
	y_buffer->target_height = vid.height;
	y_buffer->target_rowbytes = vid.rowbytes;
	y_buffer->target_bpp = vid.bpp;
	y_buffer->target_picture = ZZ_Alloc(y_buffer->target_width*vid.bpp * y_buffer->target_height);
	dest = y_buffer->target_picture;

	scalefac = FixedDiv(y_buffer->target_width*FRACUNIT, y_buffer->source_width*FRACUNIT);
	yscalefac = FixedDiv(y_buffer->target_height*FRACUNIT, y_buffer->source_height*FRACUNIT);

	rowfrac = FixedDiv(FRACUNIT, yscalefac);
	colfrac = FixedDiv(FRACUNIT, scalefac);

	for (sy = 0, dy = 0; sy < (y_buffer->source_height << FRACBITS) && dy < y_buffer->target_height; sy += rowfrac, dy++)
		for (sx = 0, dx = 0; sx < (y_buffer->source_width << FRACBITS) && dx < y_buffer->target_width; sx += colfrac, dx += y_buffer->target_bpp)
			dest[(dy * y_buffer->target_rowbytes) + dx] = y_buffer->source_picture[((sy>>FRACBITS) * y_buffer->source_width) + (sx>>FRACBITS)];
}

//
// Y_CleanupScreenBuffer
//
// Free all related memory.
//
void Y_CleanupScreenBuffer(void)
{
	// Who knows?
	if (y_buffer == NULL)
		return;

	if (y_buffer->target_picture)
		Z_Free(y_buffer->target_picture);

	if (y_buffer->source_picture)
		Z_Free(y_buffer->source_picture);

	Z_Free(y_buffer);
	y_buffer = NULL;
}

//
// Y_IntermissionDrawer
//
// Called by D_Display. Nothing is modified here; all it does is draw.
// Neat concept, huh?
//
void Y_IntermissionDrawer(void)
{
	// Bonus loops
	INT32 i;

	if (intertype == int_none || rendermode == render_none)
		return;

	// Lactozilla: Renderer switching
	if (needpatchrecache)
	{
		Y_CleanupData();
		safetorender = false;
	}

	if (!safetorender)
		V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 31);

	if (!safetorender)
		goto dontdrawbg;

	if (useinterpic)
		V_DrawScaledPatch(0, 0, 0, interpic);
	else if (!usetile)
	{
		if (rendermode == render_soft && usebuffer)
		{
			// no y_buffer
			if (y_buffer == NULL)
				VID_BlitLinearScreen(screens[1], screens[0], vid.width*vid.bpp, vid.height, vid.width*vid.bpp, vid.rowbytes);
			else
			{
				// Maybe the resolution changed?
				if ((y_buffer->target_width != vid.width) || (y_buffer->target_height != vid.height))
					Y_RescaleScreenBuffer();

				// Blit the already-scaled screen buffer to the current screen
				VID_BlitLinearScreen(y_buffer->target_picture, screens[0], vid.width*vid.bpp, vid.height, vid.width*vid.bpp, vid.rowbytes);
			}
		}
#ifdef HWRENDER
		else if (rendermode != render_soft && usebuffer)
			HWR_DrawIntermissionBG();
#endif
		else if (bgpatch)
		{
			fixed_t hs = vid.width  * FRACUNIT / BASEVIDWIDTH;
			fixed_t vs = vid.height * FRACUNIT / BASEVIDHEIGHT;
			V_DrawStretchyFixedPatch(0, 0, hs, vs, V_NOSCALEPATCH, bgpatch, NULL);
		}
	}
	else if (bgtile)
		V_DrawPatchFill(bgtile);

	LUAh_IntermissionHUD();
	if (!LUA_HudEnabled(hud_intermissiontally))
		goto skiptallydrawer;

dontdrawbg:
	if (intertype == int_coop)
	{
		INT32 bonusy;

		if (gottoken) // first to be behind everything else
			Y_IntermissionTokenDrawer();

		if (!splitscreen)  // there's not enough room in splitscreen, don't even bother trying!
		{
			// draw score
			ST_DrawPatchFromHud(HUD_SCORE, sboscore);
			ST_DrawNumFromHud(HUD_SCORENUM, data.coop.score);

			// draw time
			ST_DrawPatchFromHud(HUD_TIME, sbotime);
			if (cv_timetic.value == 3)
				ST_DrawNumFromHud(HUD_SECONDS, data.coop.tics);
			else
			{
				INT32 seconds, minutes, tictrn;

				seconds = G_TicsToSeconds(data.coop.tics);
				minutes = G_TicsToMinutes(data.coop.tics, true);
				tictrn  = G_TicsToCentiseconds(data.coop.tics);

				ST_DrawNumFromHud(HUD_MINUTES, minutes); // Minutes
				ST_DrawPatchFromHud(HUD_TIMECOLON, sbocolon); // Colon
				ST_DrawPadNumFromHud(HUD_SECONDS, seconds, 2); // Seconds

				if (cv_timetic.value == 1 || cv_timetic.value == 2 || modeattacking || marathonmode)
				{
					ST_DrawPatchFromHud(HUD_TIMETICCOLON, sboperiod); // Period
					ST_DrawPadNumFromHud(HUD_TICS, tictrn, 2); // Tics
				}
			}
		}

		// draw the "got through act" lines and act number
		V_DrawLevelTitle(data.coop.passedx1, 49, 0, data.coop.passed1);
		{
			INT32 h = V_LevelNameHeight(data.coop.passed2);
			V_DrawLevelTitle(data.coop.passedx2, 49+h+2, 0, data.coop.passed2);

			if (data.coop.actnum)
				V_DrawLevelActNum(244, 42+h, 0, data.coop.actnum);
		}

		bonusy = 150;
		// Total
		if (safetorender)
		{
			V_DrawScaledPatch(152, bonusy, 0, data.coop.ptotal);
			V_DrawTallNum(BASEVIDWIDTH - 68, bonusy + 1, 0, data.coop.total);
		}
		bonusy -= (3*SHORT(tallnum[0]->height)/2) + 1;

		// Draw bonuses
		for (i = 3; i >= 0; --i)
		{
			if (data.coop.bonuses[i].display && safetorender)
			{
				V_DrawScaledPatch(152, bonusy, 0, data.coop.bonuspatches[i]);
				V_DrawTallNum(BASEVIDWIDTH - 68, bonusy + 1, 0, data.coop.bonuses[i].points);
			}
			bonusy -= (3*SHORT(tallnum[0]->height)/2) + 1;
		}
	}
	else if (intertype == int_spec)
	{
		static tic_t animatetic = 0;
		INT32 ttheight = 16;
		INT32 xoffset1 = 0; // Line 1 x offset
		INT32 xoffset2 = 0; // Line 2 x offset
		INT32 xoffset3 = 0; // Line 3 x offset
		INT32 xoffset4 = 0; // Line 4 x offset
		INT32 xoffset5 = 0; // Line 5 x offset
		INT32 xoffset6 = 0; // Line 6 x offset
		UINT8 drawsection = 0;

		if (gottoken) // first to be behind everything else
			Y_IntermissionTokenDrawer();

		// draw the header
		if (intertic <= 2*TICRATE)
			animatetic = 0;
		else if (!animatetic && data.spec.bonuses[0].points == 0 && data.spec.bonuses[1].points == 0 && data.spec.passed3[0] != '\0')
			animatetic = intertic + TICRATE;

		if (animatetic && (tic_t)intertic >= animatetic)
		{
			const INT32 scradjust = (vid.width/vid.dupx)>>3; // 40 for BASEVIDWIDTH
			INT32 animatetimer = (intertic - animatetic);
			if (animatetimer <= 16)
			{
				xoffset1 = -(animatetimer      * scradjust);
				xoffset2 = -((animatetimer- 2) * scradjust);
				xoffset3 = -((animatetimer- 4) * scradjust);
				xoffset4 = -((animatetimer- 6) * scradjust);
				xoffset5 = -((animatetimer- 8) * scradjust);
				xoffset6 = -((animatetimer-10) * scradjust);
				if (xoffset2 > 0) xoffset2 = 0;
				if (xoffset3 > 0) xoffset3 = 0;
				if (xoffset4 > 0) xoffset4 = 0;
				if (xoffset5 > 0) xoffset5 = 0;
				if (xoffset6 > 0) xoffset6 = 0;
			}
			else if (animatetimer < 34)
			{
				drawsection = 1;
				xoffset1 = (24-animatetimer) * scradjust;
				xoffset2 = (26-animatetimer) * scradjust;
				xoffset3 = (28-animatetimer) * scradjust;
				xoffset4 = (30-animatetimer) * scradjust;
				xoffset5 = (32-animatetimer) * scradjust;
				xoffset6 = (34-animatetimer) * scradjust;
				if (xoffset1 < 0) xoffset1 = 0;
				if (xoffset2 < 0) xoffset2 = 0;
				if (xoffset3 < 0) xoffset3 = 0;
				if (xoffset4 < 0) xoffset4 = 0;
				if (xoffset5 < 0) xoffset5 = 0;
			}
			else
			{
				drawsection = 1;
				if (animatetimer == 32)
					S_StartSound(NULL, sfx_s3k68);
			}
		}

		if (drawsection == 1)
		{
			const char *ringtext = "\x82" "50 rings, no shield";
			const char *tut1text = "\x82" "press " "\x80" "spin";
			const char *tut2text = "\x82" "mid-" "\x80" "jump";
			ttheight = 8;
			V_DrawLevelTitle(data.spec.passedx1 + xoffset1, ttheight, 0, data.spec.passed1);
			ttheight += V_LevelNameHeight(data.spec.passed3) + 2;
			V_DrawLevelTitle(data.spec.passedx3 + xoffset2, ttheight, 0, data.spec.passed3);
			ttheight += V_LevelNameHeight(data.spec.passed4) + 2;
			V_DrawLevelTitle(data.spec.passedx4 + xoffset3, ttheight, 0, data.spec.passed4);

			ttheight = 108;
			V_DrawLevelTitle(BASEVIDWIDTH/2 + xoffset4 - (V_LevelNameWidth(ringtext)/2), ttheight, 0, ringtext);
			ttheight += V_LevelNameHeight(tut1text) + 2;
			V_DrawLevelTitle(BASEVIDWIDTH/2 + xoffset5 - (V_LevelNameWidth(tut1text)/2), ttheight, 0, tut1text);
			ttheight += V_LevelNameHeight(tut2text) + 2;
			V_DrawLevelTitle(BASEVIDWIDTH/2 + xoffset6 - (V_LevelNameWidth(tut2text)/2), ttheight, 0, tut2text);
		}
		else
		{
			INT32 yoffset = 0;
			if (data.spec.passed1[0] != '\0')
			{
				ttheight = 24;
				V_DrawLevelTitle(data.spec.passedx1 + xoffset1, ttheight, 0, data.spec.passed1);
				ttheight += V_LevelNameHeight(data.spec.passed2) + 2;
				V_DrawLevelTitle(data.spec.passedx2 + xoffset2, ttheight, 0, data.spec.passed2);
			}
			else
			{
				ttheight = 24 + (V_LevelNameHeight(data.spec.passed2)/2) + 2;
				V_DrawLevelTitle(data.spec.passedx2 + xoffset1, ttheight, 0, data.spec.passed2);
			}

			V_DrawScaledPatch(152 + xoffset3, 108, 0, data.spec.bonuspatches[0]);
			V_DrawTallNum(BASEVIDWIDTH + xoffset3 - 68, 109, 0, data.spec.bonuses[0].points);
			if (data.spec.bonuses[1].display)
			{
				V_DrawScaledPatch(152 + xoffset4, 124, 0, data.spec.bonuspatches[1]);
				V_DrawTallNum(BASEVIDWIDTH + xoffset4 - 68, 125, 0, data.spec.bonuses[1].points);
				yoffset = 16;
				// hack; pass the buck along...
				xoffset4 = xoffset5;
				xoffset5 = xoffset6;
			}
			V_DrawScaledPatch(152 + xoffset4, 124+yoffset, 0, data.spec.pscore);
			V_DrawTallNum(BASEVIDWIDTH + xoffset4 - 68, 125+yoffset, 0, data.spec.score);

			// Draw continues!
			if (continuesInSession /* && (data.spec.continues & 0x80) */) // Always draw when continues are a thing
			{
				UINT8 continues = data.spec.continues & 0x7F;

				V_DrawScaledPatch(152 + xoffset5, 150+yoffset, 0, data.spec.pcontinues);
				if (continues > 5)
				{
					INT32 leftx = (continues >= 10) ? 216 : 224;
					V_DrawContinueIcon(leftx + xoffset5, 162+yoffset, 0, *data.spec.playerchar, *data.spec.playercolor);
					V_DrawScaledPatch(leftx + xoffset5 + 12, 160+yoffset, 0, stlivex);
					if (!((data.spec.continues & 0x80) && !(endtic < 0 || intertic%20 < 10)))
						V_DrawRightAlignedString(252 + xoffset5, 158+yoffset, 0,
							va("%d",(((data.spec.continues & 0x80) && (endtic < 0)) ? continues-1 : continues)));
				}
				else
				{
					for (i = 0; i < continues; ++i)
					{
						if ((data.spec.continues & 0x80) && i == continues-1 && (endtic < 0 || intertic%20 < 10))
							break;
						V_DrawContinueIcon(246 + xoffset5 - (i*20), 162+yoffset, 0, *data.spec.playerchar, *data.spec.playercolor);
					}
				}
			}
		}

		// draw the emeralds
		//if (intertic & 1)
		{
			boolean drawthistic = !(ALL7EMERALDS(emeralds) && (intertic & 1));
			INT32 emeraldx = 152 - 3*28;
			INT32 em = P_GetNextEmerald();

			if (em == 7)
			{
				if (!stagefailed)
				{
					fixed_t adjust = 2*(FINESINE(FixedAngle((intertic + 1)<<(FRACBITS-4)) & FINEMASK));
					V_DrawFixedPatch(152<<FRACBITS, (74<<FRACBITS) - adjust, FRACUNIT, 0, emeraldpics[0][em], NULL);
				}
			}
			else if (em < 7)
			{
				static UINT8 emeraldbounces = 0;
				static INT32 emeraldmomy = 20;
				static INT32 emeraldy = -40;

				if (drawthistic)
					for (i = 0; i < 7; ++i)
					{
						if ((i != em) && (emeralds & (1 << i)))
							V_DrawScaledPatch(emeraldx, 74, 0, emeraldpics[0][i]);
						emeraldx += 28;
					}

				emeraldx = 152 + (em-3)*28;

				if (intertic <= 1)
				{
					emeraldbounces = 0;
					emeraldmomy = 20;
					emeraldy = -40;
				}
				else
				{
					if (!stagefailed)
					{
						if (emeraldbounces < 3)
						{
							emeraldy += (++emeraldmomy);
							if (emeraldy > 74)
							{
								S_StartSound(NULL, sfx_tink); // tink
								emeraldbounces++;
								emeraldmomy = -(emeraldmomy/2);
								emeraldy = 74;
							}
						}
					}
					else
					{
						if (emeraldy < (vid.height/vid.dupy)+16)
						{
							emeraldy += (++emeraldmomy);
							emeraldx += intertic - 6;
						}
						if (emeraldbounces < 1 && emeraldy > 74)
						{
							S_StartSound(NULL, sfx_shldls); // nope
							emeraldbounces++;
							emeraldmomy = -(emeraldmomy/2);
							emeraldy = 74;
						}
					}
					if (drawthistic)
						V_DrawScaledPatch(emeraldx, emeraldy, 0, emeraldpics[0][em]);
				}
			}
		}
	}
	else if (intertype == int_match || intertype == int_race)
	{
		INT32 j = 0;
		INT32 x = 4;
		INT32 y = 48;
		char name[MAXPLAYERNAME+1];
		char strtime[10];

		// draw the header
		if (safetorender)
			V_DrawScaledPatch(112, 2, 0, data.match.result);

		// draw the level name
		V_DrawCenteredString(BASEVIDWIDTH/2, 20, 0, data.match.levelstring);
		V_DrawFill(4, 42, 312, 1, 0);

		if (data.match.numplayers > 9)
		{
			V_DrawFill(160, 32, 1, 152, 0);

			if (intertype == int_race)
				V_DrawRightAlignedString(x+152, 32, V_YELLOWMAP, "TIME");
			else
				V_DrawRightAlignedString(x+152, 32, V_YELLOWMAP, "SCORE");

			V_DrawCenteredString(x+(BASEVIDWIDTH/2)+6, 32, V_YELLOWMAP, "#");
			V_DrawString(x+(BASEVIDWIDTH/2)+36, 32, V_YELLOWMAP, "NAME");
		}

		V_DrawCenteredString(x+6, 32, V_YELLOWMAP, "#");
		V_DrawString(x+36, 32, V_YELLOWMAP, "NAME");

		if (intertype == int_race)
			V_DrawRightAlignedString(x+(BASEVIDWIDTH/2)+152, 32, V_YELLOWMAP, "TIME");
		else
			V_DrawRightAlignedString(x+(BASEVIDWIDTH/2)+152, 32, V_YELLOWMAP, "SCORE");

		for (i = 0; i < data.match.numplayers; i++)
		{
			if (data.match.spectator[i])
				continue; //Ignore spectators.

			V_DrawCenteredString(x+6, y, 0, va("%d", j+1));
			j++; //We skip spectators, but not their number.

			if (playeringame[data.match.num[i]])
			{
				// Draw the back sprite, it looks ugly if we don't
				V_DrawSmallScaledPatch(x+16, y-4, 0, livesback);

				if (data.match.color[i] == 0)
					V_DrawSmallScaledPatch(x+16, y-4, 0,faceprefix[*data.match.character[i]]);
				else
				{
					UINT8 *colormap = R_GetTranslationColormap(*data.match.character[i], *data.match.color[i], GTC_CACHE);
					V_DrawSmallMappedPatch(x+16, y-4, 0,faceprefix[*data.match.character[i]], colormap);
				}

				if (data.match.numplayers > 9)
				{
					if (intertype == int_race)
						strlcpy(name, data.match.name[i], 8);
					else
						strlcpy(name, data.match.name[i], 9);
				}
				else
					STRBUFCPY(name, data.match.name[i]);

				V_DrawString(x+36, y, V_ALLOWLOWERCASE, name);

				if (data.match.numplayers > 9)
				{
					if (intertype == int_match)
						V_DrawRightAlignedString(x+152, y, 0, va("%i", data.match.scores[i]));
					else if (intertype == int_race)
					{
						if (players[data.match.num[i]].pflags & PF_GAMETYPEOVER)
							snprintf(strtime, sizeof strtime, "DNF");
						else
							snprintf(strtime, sizeof strtime,
								"%i:%02i.%02i",
								G_TicsToMinutes(data.match.scores[i], true),
								G_TicsToSeconds(data.match.scores[i]), G_TicsToCentiseconds(data.match.scores[i]));

						strtime[sizeof strtime - 1] = '\0';
						V_DrawRightAlignedString(x+152, y, 0, strtime);
					}
				}
				else
				{
					if (intertype == int_match)
						V_DrawRightAlignedString(x+152+BASEVIDWIDTH/2, y, 0, va("%u", data.match.scores[i]));
					else if (intertype == int_race)
					{
						if (players[data.match.num[i]].pflags & PF_GAMETYPEOVER)
							snprintf(strtime, sizeof strtime, "DNF");
						else
							snprintf(strtime, sizeof strtime, "%i:%02i.%02i", G_TicsToMinutes(data.match.scores[i], true),
									G_TicsToSeconds(data.match.scores[i]), G_TicsToCentiseconds(data.match.scores[i]));

						strtime[sizeof strtime - 1] = '\0';

						V_DrawRightAlignedString(x+152+BASEVIDWIDTH/2, y, 0, strtime);
					}
				}
			}

			y += 16;

			if (y > 176)
			{
				y = 48;
				x += BASEVIDWIDTH/2;
			}
		}
	}
	else if (intertype == int_ctf || intertype == int_teammatch)
	{
		INT32 x = 4, y = 0;
		INT32 redplayers = 0, blueplayers = 0;
		char name[MAXPLAYERNAME+1];

		// Show the team flags and the team score at the top instead of "RESULTS"
		V_DrawSmallScaledPatch(128 - SHORT(data.match.blueflag->width)/4, 2, 0, data.match.blueflag);
		V_DrawCenteredString(128, 16, 0, va("%u", bluescore));

		V_DrawSmallScaledPatch(192 - SHORT(data.match.redflag->width)/4, 2, 0, data.match.redflag);
		V_DrawCenteredString(192, 16, 0, va("%u", redscore));

		// draw the level name
		V_DrawCenteredString(BASEVIDWIDTH/2, 24, 0, data.match.levelstring);
		V_DrawFill(4, 42, 312, 1, 0);

		//vert. line
		V_DrawFill(160, 32, 1, 152, 0);

		//strings at the top of the list
		V_DrawCenteredString(x+6, 32, V_YELLOWMAP, "#");
		V_DrawCenteredString(x+(BASEVIDWIDTH/2)+6, 32, V_YELLOWMAP, "#");

		V_DrawString(x+36, 32, V_YELLOWMAP, "NAME");
		V_DrawString(x+(BASEVIDWIDTH/2)+36, 32, V_YELLOWMAP, "NAME");

		V_DrawRightAlignedString(x+152, 32, V_YELLOWMAP, "SCORE");
		V_DrawRightAlignedString(x+(BASEVIDWIDTH/2)+152, 32, V_YELLOWMAP, "SCORE");

		for (i = 0; i < data.match.numplayers; i++)
		{
			if (playeringame[data.match.num[i]] && !(data.match.spectator[i]))
			{
				UINT8 *colormap = R_GetTranslationColormap(*data.match.character[i], *data.match.color[i], GTC_CACHE);

				if (*data.match.color[i] == SKINCOLOR_RED) //red
				{
					if (redplayers++ > 9)
						continue;
					x = 4 + (BASEVIDWIDTH/2);
					y = (redplayers * 16) + 32;
					V_DrawCenteredString(x+6, y, 0, va("%d", redplayers));
				}
				else if (*data.match.color[i] == SKINCOLOR_BLUE) //blue
				{
					if (blueplayers++ > 9)
						continue;
					x = 4;
					y = (blueplayers * 16) + 32;
					V_DrawCenteredString(x+6, y, 0, va("%d", blueplayers));
				}
				else
					continue;

				// Draw the back sprite, it looks ugly if we don't
				V_DrawSmallScaledPatch(x+16, y-4, 0, livesback);

				//color is ALWAYS going to be 6/7 here, no need to check if it's nonzero.
				V_DrawSmallMappedPatch(x+16, y-4, 0,faceprefix[*data.match.character[i]], colormap);

				strlcpy(name, data.match.name[i], 9);

				V_DrawString(x+36, y, V_ALLOWLOWERCASE, name);

				V_DrawRightAlignedString(x+152, y, 0, va("%u", data.match.scores[i]));
			}
		}
	}
	else if (intertype == int_comp)
	{
		INT32 x = 4;
		INT32 y = 48;
		UINT32 ptime, pring, pmaxring, pmonitor, pscore;
		char sstrtime[10];

		// draw the level name
		V_DrawCenteredString(BASEVIDWIDTH/2, 8, 0, data.competition.levelstring);
		V_DrawFill(4, 42, 312, 1, 0);

		V_DrawCenteredString(x+6, 32, V_YELLOWMAP, "#");
		V_DrawString(x+36, 32, V_YELLOWMAP, "NAME");
		// Time
		V_DrawRightAlignedString(x+160, 32, V_YELLOWMAP, "TIME");

		// Rings
		V_DrawThinString(x+168, 32, V_YELLOWMAP, "RING");

		// Total rings
		V_DrawThinString(x+191, 24, V_YELLOWMAP, "TOTAL");
		V_DrawThinString(x+196, 32, V_YELLOWMAP, "RING");

		// Monitors
		V_DrawThinString(x+223, 24, V_YELLOWMAP, "ITEM");
		V_DrawThinString(x+229, 32, V_YELLOWMAP, "BOX");

		// Score
		V_DrawRightAlignedString(x+288, 32, V_YELLOWMAP, "SCORE");

		// Points
		V_DrawRightAlignedString(x+312, 32, V_YELLOWMAP, "PT");

		for (i = 0; i < data.competition.numplayers; i++)
		{
			ptime = (data.competition.times[i] & ~0x80000000);
			pring = (data.competition.rings[i] & ~0x80000000);
			pmaxring = (data.competition.maxrings[i] & ~0x80000000);
			pmonitor = (data.competition.monitors[i] & ~0x80000000);
			pscore = (data.competition.scores[i] & ~0x80000000);

			V_DrawCenteredString(x+6, y, 0, va("%d", i+1));

			if (playeringame[data.competition.num[i]])
			{
				// Draw the back sprite, it looks ugly if we don't
				V_DrawSmallScaledPatch(x+16, y-4, 0, livesback);

				if (data.competition.color[i] == 0)
					V_DrawSmallScaledPatch(x+16, y-4, 0,faceprefix[*data.competition.character[i]]);
				else
				{
					UINT8 *colormap = R_GetTranslationColormap(*data.competition.character[i], *data.competition.color[i], GTC_CACHE);
					V_DrawSmallMappedPatch(x+16, y-4, 0,faceprefix[*data.competition.character[i]], colormap);
				}

				// already constrained to 8 characters
				V_DrawString(x+36, y, V_ALLOWLOWERCASE, data.competition.name[i]);

				if (players[data.competition.num[i]].pflags & PF_GAMETYPEOVER)
					snprintf(sstrtime, sizeof sstrtime, "Time Over");
				else if (players[data.competition.num[i]].lives <= 0)
					snprintf(sstrtime, sizeof sstrtime, "Game Over");
				else
					snprintf(sstrtime, sizeof sstrtime, "%i:%02i.%02i", G_TicsToMinutes(ptime, true),
							G_TicsToSeconds(ptime), G_TicsToCentiseconds(ptime));

				sstrtime[sizeof sstrtime - 1] = '\0';
				// Time
				V_DrawRightAlignedThinString(x+160, y, ((data.competition.times[i] & 0x80000000) ? V_YELLOWMAP : 0), sstrtime);
				// Rings
				V_DrawRightAlignedThinString(x+188, y, V_MONOSPACE|((data.competition.rings[i] & 0x80000000) ? V_YELLOWMAP : 0), va("%u", pring));
				// Total rings
				V_DrawRightAlignedThinString(x+216, y, V_MONOSPACE|((data.competition.maxrings[i] & 0x80000000) ? V_YELLOWMAP : 0), va("%u", pmaxring));
				// Monitors
				V_DrawRightAlignedThinString(x+244, y, V_MONOSPACE|((data.competition.monitors[i] & 0x80000000) ? V_YELLOWMAP : 0), va("%u", pmonitor));
				// Score
				V_DrawRightAlignedThinString(x+288, y, V_MONOSPACE|((data.competition.scores[i] & 0x80000000) ? V_YELLOWMAP : 0), va("%u", pscore));
				// Final Points
				V_DrawRightAlignedString(x+312, y, V_YELLOWMAP, va("%d", data.competition.points[i]));
			}

			y += 16;

			if (y > 176)
				break;
		}
	}

skiptallydrawer:
	if (!LUA_HudEnabled(hud_intermissionmessages))
		return;

	if (timer)
		V_DrawCenteredString(BASEVIDWIDTH/2, 188, V_YELLOWMAP,
			va("start in %d seconds", timer/TICRATE));

	// Make it obvious that scrambling is happening next round.
	if (cv_scrambleonchange.value && cv_teamscramble.value && (intertic/TICRATE % 2 == 0))
		V_DrawCenteredString(BASEVIDWIDTH/2, BASEVIDHEIGHT/2, V_YELLOWMAP, M_GetText("Teams will be scrambled next round!"));
}

//
// Y_Ticker
//
// Manages fake score tally for single player end of act, and decides when intermission is over.
//
void Y_Ticker(void)
{
	if (intertype == int_none)
		return;

	// Check for pause or menu up in single player
	if (paused || P_AutoPause())
		return;

	LUAh_IntermissionThinker();

	intertic++;

	// Team scramble code for team match and CTF.
	// Don't do this if we're going to automatically scramble teams next round.
	if (G_GametypeHasTeams() && cv_teamscramble.value && !cv_scrambleonchange.value && server)
	{
		// If we run out of time in intermission, the beauty is that
		// the P_Ticker() team scramble code will pick it up.
		if ((intertic % (TICRATE/7)) == 0)
			P_DoTeamscrambling();
	}

	// multiplayer uses timer (based on cv_inttime)
	if (timer)
	{
		if (!--timer)
		{
			Y_EndIntermission();
			G_AfterIntermission();
			return;
		}
	}
	// single player is hardcoded to go away after awhile
	else if (intertic == endtic)
	{
		Y_EndIntermission();
		G_AfterIntermission();
		return;
	}

	if (endtic != -1)
		return; // tally is done

	if (intertype == int_coop) // coop or single player, normal level
	{
		INT32 i;
		UINT32 oldscore = data.coop.score;
		boolean skip = (marathonmode) ? true : false;
		boolean anybonuses = false;

		if (!intertic) // first time only
		{
			if (mapheaderinfo[gamemap-1]->musinterfadeout
#ifdef _WIN32
				// can't fade midi due to win32 volume hack
				&& S_MusicType() != MU_MID
#endif
			)
				S_FadeOutStopMusic(mapheaderinfo[gamemap-1]->musinterfadeout);
			else if (mapheaderinfo[gamemap-1]->musintername[0] && S_MusicExists(mapheaderinfo[gamemap-1]->musintername, !midi_disabled, !digital_disabled))
				S_ChangeMusicInternal(mapheaderinfo[gamemap-1]->musintername, false); // don't loop it
			else
				S_ChangeMusicInternal("_clear", false); // don't loop it
			tallydonetic = -1;
		}

		if (intertic < TICRATE) // one second pause before tally begins
			return;

		for (i = 0; i < MAXPLAYERS; i++)
			if (playeringame[i] && (players[i].cmd.buttons & BT_SPIN))
				skip = true;

		// bonuses count down by 222 each tic
		for (i = 0; i < 4; ++i)
		{
			if (!data.coop.bonuses[i].points)
				continue;

			data.coop.bonuses[i].points -= 222;
			data.coop.total += 222;
			data.coop.score += 222;
			if (data.coop.bonuses[i].points < 0 || skip == true) // too far?
			{
				data.coop.score += data.coop.bonuses[i].points;
				data.coop.total += data.coop.bonuses[i].points;
				data.coop.bonuses[i].points = 0;
			}
			if (data.coop.score > MAXSCORE)
				data.coop.score = MAXSCORE;
			if (data.coop.bonuses[i].points > 0)
				anybonuses = true;
		}

		if (!anybonuses)
		{
			tallydonetic = intertic;
			endtic = intertic + 3*TICRATE; // 3 second pause after end of tally
			S_StartSound(NULL, (gottoken ? sfx_token : sfx_chchng)); // cha-ching!

			// Update when done with tally
			if ((!modifiedgame || savemoddata) && !(netgame || multiplayer) && !demoplayback)
			{
				if (M_UpdateUnlockablesAndExtraEmblems())
					S_StartSound(NULL, sfx_s3k68);

				G_SaveGameData();
			}
		}
		else if (!(intertic & 1))
			S_StartSound(NULL, sfx_ptally); // tally sound effect

		if (data.coop.gotlife > 0 && (skip == true || data.coop.score % 50000 < oldscore % 50000)) // just passed a 50000 point mark
		{
			// lives are already added since tally is fake, but play the music
			P_PlayLivesJingle(NULL);
			--data.coop.gotlife;
		}
	}
	else if (intertype == int_spec) // coop or single player, special stage
	{
		INT32 i;
		UINT32 oldscore = data.spec.score;
		boolean skip = (marathonmode) ? true : false, super = false, anybonuses = false;

		if (!intertic) // first time only
		{
			if (mapheaderinfo[gamemap-1]->musinterfadeout
#ifdef _WIN32
				// can't fade midi due to win32 volume hack
				&& S_MusicType() != MU_MID
#endif
			)
				S_FadeOutStopMusic(mapheaderinfo[gamemap-1]->musinterfadeout);
			else if (mapheaderinfo[gamemap-1]->musintername[0] && S_MusicExists(mapheaderinfo[gamemap-1]->musintername, !midi_disabled, !digital_disabled))
				S_ChangeMusicInternal(mapheaderinfo[gamemap-1]->musintername, false); // don't loop it
			else
				S_ChangeMusicInternal("_clear", false); // don't loop it
			tallydonetic = -1;
		}

		if (intertic < 2*TICRATE) // TWO second pause before tally begins, thank you mazmazz
			return;

		for (i = 0; i < MAXPLAYERS; i++)
			if (playeringame[i])
			{
				if (players[i].cmd.buttons & BT_SPIN)
					skip = true;
				if (players[i].charflags & SF_SUPER)
					super = true;
			}

		if (tallydonetic != -1 && ((data.spec.continues & 0x80) || (super && ALL7EMERALDS(emeralds))))
		{
			if ((intertic - tallydonetic) > (3*TICRATE)/2)
			{
				endtic = intertic + 4*TICRATE; // 4 second pause after end of tally
				if (data.spec.continues & 0x80)
					S_StartSound(NULL, sfx_s3kac); // bingly-bingly-bing!

			}
			return;
		}

		// bonuses count down by 222 each tic
		for (i = 0; i < 2; ++i)
		{
			if (!data.spec.bonuses[i].points)
				continue;

			data.spec.bonuses[i].points -= 222;
			data.spec.score += 222;
			if (data.spec.bonuses[i].points < 0 || skip == true) // too far?
			{
				data.spec.score += data.spec.bonuses[i].points;
				data.spec.bonuses[i].points = 0;
			}
			if (data.spec.score > MAXSCORE)
				data.spec.score = MAXSCORE;
			if (data.spec.bonuses[i].points > 0)
				anybonuses = true;
		}

		if (!anybonuses)
		{
			tallydonetic = intertic;
			if (!((data.spec.continues & 0x80) || (super && ALL7EMERALDS(emeralds)))) // don't set endtic yet!
				endtic = intertic + 4*TICRATE; // 4 second pause after end of tally

			S_StartSound(NULL, (gottoken ? sfx_token : sfx_chchng)); // cha-ching!

			// Update when done with tally
			if ((!modifiedgame || savemoddata) && !(netgame || multiplayer) && !demoplayback)
			{
				if (M_UpdateUnlockablesAndExtraEmblems())
					S_StartSound(NULL, sfx_s3k68);

				G_SaveGameData();
			}
		}
		else if (!(intertic & 1))
			S_StartSound(NULL, sfx_ptally); // tally sound effect

		if (data.spec.gotlife > 0 && (skip == true || data.spec.score % 50000 < oldscore % 50000)) // just passed a 50000 point mark
		{
			// lives are already added since tally is fake, but play the music
			P_PlayLivesJingle(NULL);
			--data.spec.gotlife;
		}
	}
	else if (intertype == int_match || intertype == int_ctf || intertype == int_teammatch) // match
	{
		if (!intertic) // first time only
			S_ChangeMusicInternal("_inter", true); // loop it

		// If a player has left or joined, recalculate scores.
		if (data.match.numplayers != D_NumPlayers())
			Y_CalculateMatchWinners();
	}
	else if (intertype == int_race || intertype == int_comp) // race
	{
		if (!intertic) // first time only
			S_ChangeMusicInternal("_inter", true); // loop it

		// Don't bother recalcing for race. It doesn't make as much sense.
	}
}

//
// Y_DetermineIntermissionType
//
// Determines the intermission type from the current gametype.
//
void Y_DetermineIntermissionType(void)
{
	// set to int_none initially
	intertype = int_none;

	if (intermissiontypes[gametype] != int_none)
		intertype = intermissiontypes[gametype];
	else if (gametype == GT_COOP)
		intertype = (G_IsSpecialStage(gamemap)) ? int_spec : int_coop;
	else if (gametype == GT_TEAMMATCH)
		intertype = int_teammatch;
	else if (gametype == GT_MATCH
	 || gametype == GT_TAG
	 || gametype == GT_HIDEANDSEEK)
		intertype = int_match;
	else if (gametype == GT_RACE)
		intertype = int_race;
	else if (gametype == GT_COMPETITION)
		intertype = int_comp;
	else if (gametype == GT_CTF)
		intertype = int_ctf;
}

//
// Y_StartIntermission
//
// Called by G_DoCompleted. Sets up data for intermission drawer/ticker.
//
void Y_StartIntermission(void)
{
	INT32 i;

	intertic = -1;

#ifdef PARANOIA
	if (endtic != -1)
		I_Error("endtic is dirty");
#endif

	safetorender = true;

	if (!multiplayer)
	{
		timer = 0;
		intertype = (G_IsSpecialStage(gamemap)) ? int_spec : int_coop;
	}
	else
	{
		if (cv_inttime.value == 0 && ((intertype == int_coop) || (intertype == int_spec)))
			timer = 0;
		else
		{
			timer = cv_inttime.value*TICRATE;

			if (!timer)
				timer = 1;
		}
	}

	// We couldn't display the intermission even if we wanted to.
	// But we still need to give the players their score bonuses, dummy.
	//if (dedicated) return;

	// This should always exist, but just in case...
	if(!mapheaderinfo[prevmap])
		P_AllocMapHeader(prevmap);

	switch (intertype)
	{
		case int_coop: // coop or single player, normal level
		{
			// award time and ring bonuses
			Y_AwardCoopBonuses();

			// setup time data
			data.coop.tics = players[consoleplayer].realtime;

			for (i = 0; i < 4; ++i)
				data.coop.bonuspatches[i] = W_CachePatchName(data.coop.bonuses[i].patch, PU_PATCH);
			data.coop.ptotal = W_CachePatchName("YB_TOTAL", PU_PATCH);

			// get act number
			data.coop.actnum = mapheaderinfo[gamemap-1]->actnum;

			// get background patches
			bgpatch = W_CachePatchName("INTERSCR", PU_PATCH);

			// grab an interscreen if appropriate
			if (mapheaderinfo[gamemap-1]->interscreen[0] != '#')
			{
				interpic = W_CachePatchName(mapheaderinfo[gamemap-1]->interscreen, PU_PATCH);
				useinterpic = true;
				usebuffer = false;
			}
			else
			{
				useinterpic = false;
#ifdef HWRENDER
				if (rendermode == render_opengl)
					usebuffer = true; // This needs to be here for OpenGL, otherwise usebuffer is never set to true for it, and thus there's no screenshot in the intermission
#endif
			}
			usetile = false;

			// set up the "got through act" message according to skin name
			// too long so just show "YOU GOT THROUGH THE ACT"
			if (strlen(skins[players[consoleplayer].skin].realname) > 13)
			{
				strcpy(data.coop.passed1, "you got");
				strcpy(data.coop.passed2, (mapheaderinfo[gamemap-1]->actnum) ? "through act" : "through the act");
			}
			// long enough that "X GOT" won't fit so use "X PASSED THE ACT"
			else if (strlen(skins[players[consoleplayer].skin].realname) > 8)
			{
				strcpy(data.coop.passed1, skins[players[consoleplayer].skin].realname);
				strcpy(data.coop.passed2, (mapheaderinfo[gamemap-1]->actnum) ? "passed act" : "passed the act");
			}
			// length is okay for normal use
			else
			{
				snprintf(data.coop.passed1, sizeof data.coop.passed1, "%s got",
					skins[players[consoleplayer].skin].realname);
				strcpy(data.coop.passed2, (mapheaderinfo[gamemap-1]->actnum) ? "through act" : "through the act");
			}

			// set X positions
			if (mapheaderinfo[gamemap-1]->actnum)
			{
				data.coop.passedx1 = 62 + (176 - V_LevelNameWidth(data.coop.passed1))/2;
				data.coop.passedx2 = 62 + (176 - V_LevelNameWidth(data.coop.passed2))/2;
			}
			else
			{
				data.coop.passedx1 = (BASEVIDWIDTH - V_LevelNameWidth(data.coop.passed1))/2;
				data.coop.passedx2 = (BASEVIDWIDTH - V_LevelNameWidth(data.coop.passed2))/2;
			}
			// The above value is not precalculated because it needs only be computed once
			// at the start of intermission, and precalculating it would preclude mods
			// changing the font to one of a slightly different width.
			break;
		}

		case int_spec: // coop or single player, special stage
		{
			// give out ring bonuses
			Y_AwardSpecialStageBonus();

			for (i = 0; i < 2; ++i)
				data.spec.bonuspatches[i] = W_CachePatchName(data.spec.bonuses[i].patch, PU_PATCH);

			data.spec.pscore = W_CachePatchName("YB_SCORE", PU_PATCH);
			data.spec.pcontinues = W_CachePatchName("YB_CONTI", PU_PATCH);

			// get background tile
			bgtile = W_CachePatchName("SPECTILE", PU_PATCH);

			// grab an interscreen if appropriate
			if (mapheaderinfo[gamemap-1]->interscreen[0] != '#')
			{
				interpic = W_CachePatchName(mapheaderinfo[gamemap-1]->interscreen, PU_PATCH);
				useinterpic = true;
			}
			else
				useinterpic = false;

			// tile if using the default background
			usetile = !useinterpic;

			// get special stage specific patches
/*			if (!stagefailed && ALL7EMERALDS(emeralds))
			{
				data.spec.cemerald = W_CachePatchName("GOTEMALL", PU_PATCH);
				data.spec.headx = 70;
				data.spec.nowsuper = players[consoleplayer].skin
					? NULL : W_CachePatchName("NOWSUPER", PU_PATCH);
			}
			else
			{
				data.spec.cemerald = W_CachePatchName("CEMERALD", PU_PATCH);
				data.spec.headx = 48;
				data.spec.nowsuper = NULL;
			} */

			// Super form stuff (normally blank)
			data.spec.passed3[0] = '\0';
			data.spec.passed4[0] = '\0';

			// set up the "got through act" message according to skin name
			if (stagefailed)
			{
				strcpy(data.spec.passed2, "Special Stage");
				data.spec.passed1[0] = '\0';
			}
			else if (ALL7EMERALDS(emeralds))
			{
				snprintf(data.spec.passed1,
					sizeof data.spec.passed1, "%s",
					skins[players[consoleplayer].skin].realname);
				data.spec.passed1[sizeof data.spec.passed1 - 1] = '\0';
				strcpy(data.spec.passed2, "got them all!");

				if (players[consoleplayer].charflags & SF_SUPER)
				{
					strcpy(data.spec.passed3, "can now become");
					snprintf(data.spec.passed4,
						sizeof data.spec.passed4, "Super %s",
						skins[players[consoleplayer].skin].realname);
					data.spec.passed4[sizeof data.spec.passed4 - 1] = '\0';
				}
			}
			else
			{
				if (strlen(skins[players[consoleplayer].skin].realname) <= SKINNAMESIZE-5)
				{
					snprintf(data.spec.passed1,
						sizeof data.spec.passed1, "%s got",
						skins[players[consoleplayer].skin].realname);
					data.spec.passed1[sizeof data.spec.passed1 - 1] = '\0';
				}
				else
					strcpy(data.spec.passed1, "You got");
				strcpy(data.spec.passed2, "a Chaos Emerald");
				if (P_GetNextEmerald() > 6)
				{
					data.spec.passed2[15] = '?';
					data.spec.passed2[16] = '\0';
				}
			}
			data.spec.passedx1 = (BASEVIDWIDTH - V_LevelNameWidth(data.spec.passed1))/2;
			data.spec.passedx2 = (BASEVIDWIDTH - V_LevelNameWidth(data.spec.passed2))/2;
			data.spec.passedx3 = (BASEVIDWIDTH - V_LevelNameWidth(data.spec.passed3))/2;
			data.spec.passedx4 = (BASEVIDWIDTH - V_LevelNameWidth(data.spec.passed4))/2;
			break;
		}

		case int_match:
		{
			// Calculate who won
			Y_CalculateMatchWinners();

			// set up the levelstring
			if (mapheaderinfo[prevmap]->actnum)
				snprintf(data.match.levelstring,
					sizeof data.match.levelstring,
					"%.32s * %d *",
					mapheaderinfo[prevmap]->lvlttl, mapheaderinfo[prevmap]->actnum);
			else
				snprintf(data.match.levelstring,
					sizeof data.match.levelstring,
					"* %.32s *",
					mapheaderinfo[prevmap]->lvlttl);

			data.match.levelstring[sizeof data.match.levelstring - 1] = '\0';

			// get RESULT header
			data.match.result =
				W_CachePatchName("RESULT", PU_PATCH);

			bgtile = W_CachePatchName("SRB2BACK", PU_PATCH);
			usetile = true;
			useinterpic = false;
			break;
		}

		case int_race: // (time-only race)
		{
			// Calculate who won
			Y_CalculateTimeRaceWinners();

			// set up the levelstring
			if (mapheaderinfo[prevmap]->actnum)
				snprintf(data.match.levelstring,
					sizeof data.match.levelstring,
					"%.32s * %d *",
					mapheaderinfo[prevmap]->lvlttl, mapheaderinfo[prevmap]->actnum);
			else
				snprintf(data.match.levelstring,
					sizeof data.match.levelstring,
					"* %.32s *",
					mapheaderinfo[prevmap]->lvlttl);

			data.match.levelstring[sizeof data.match.levelstring - 1] = '\0';

			// get RESULT header
			data.match.result = W_CachePatchName("RESULT", PU_PATCH);

			bgtile = W_CachePatchName("SRB2BACK", PU_PATCH);
			usetile = true;
			useinterpic = false;
			break;
		}

		case int_teammatch:
		case int_ctf:
		{
			// Calculate who won
			Y_CalculateMatchWinners();

			// set up the levelstring
			if (mapheaderinfo[prevmap]->actnum)
				snprintf(data.match.levelstring,
					sizeof data.match.levelstring,
					"%.32s * %d *",
					mapheaderinfo[prevmap]->lvlttl, mapheaderinfo[prevmap]->actnum);
			else
				snprintf(data.match.levelstring,
					sizeof data.match.levelstring,
					"* %.32s *",
					mapheaderinfo[prevmap]->lvlttl);

			data.match.levelstring[sizeof data.match.levelstring - 1] = '\0';

			if (intertype == int_ctf)
			{
				data.match.redflag = rflagico;
				data.match.blueflag = bflagico;
			}
			else // team match
			{
				data.match.redflag = rmatcico;
				data.match.blueflag = bmatcico;
			}

			bgtile = W_CachePatchName("SRB2BACK", PU_PATCH);
			usetile = true;
			useinterpic = false;
			break;
		}

		case int_comp: // classic (full race)
		{
			// find out who won
			Y_CalculateCompetitionWinners();

			// set up the levelstring
			if (mapheaderinfo[prevmap]->actnum)
				snprintf(data.competition.levelstring,
					sizeof data.competition.levelstring,
					"%.32s * %d *",
					mapheaderinfo[prevmap]->lvlttl, mapheaderinfo[prevmap]->actnum);
			else
				snprintf(data.competition.levelstring,
					sizeof data.competition.levelstring,
					"* %.32s *",
					mapheaderinfo[prevmap]->lvlttl);

			data.competition.levelstring[sizeof data.competition.levelstring - 1] = '\0';

			// get background tile
			bgtile = W_CachePatchName("SRB2BACK", PU_PATCH);
			usetile = true;
			useinterpic = false;
			break;
		}

		case int_none:
		default:
			break;
	}
}

//
// Y_CalculateMatchWinners
//
static void Y_CalculateMatchWinners(void)
{
	INT32 i, j;
	boolean completed[MAXPLAYERS];

	// Initialize variables
	memset(data.match.scores, 0, sizeof (data.match.scores));
	memset(data.match.color, 0, sizeof (data.match.color));
	memset(data.match.character, 0, sizeof (data.match.character));
	memset(data.match.spectator, 0, sizeof (data.match.spectator));
	memset(completed, 0, sizeof (completed));
	data.match.numplayers = 0;
	i = j = 0;

	for (j = 0; j < MAXPLAYERS; j++)
	{
		if (!playeringame[j])
			continue;

		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (!playeringame[i])
				continue;

			if (players[i].score >= data.match.scores[data.match.numplayers] && completed[i] == false)
			{
				data.match.scores[data.match.numplayers] = players[i].score;
				data.match.color[data.match.numplayers] = &players[i].skincolor;
				data.match.character[data.match.numplayers] = &players[i].skin;
				data.match.name[data.match.numplayers] = player_names[i];
				data.match.spectator[data.match.numplayers] = players[i].spectator;
				data.match.num[data.match.numplayers] = i;
			}
		}
		completed[data.match.num[data.match.numplayers]] = true;
		data.match.numplayers++;
	}
}

//
// Y_CalculateTimeRaceWinners
//
static void Y_CalculateTimeRaceWinners(void)
{
	INT32 i, j;
	boolean completed[MAXPLAYERS];

	// Initialize variables

	for (i = 0; i < MAXPLAYERS; i++)
		data.match.scores[i] = INT32_MAX;

	memset(data.match.color, 0, sizeof (data.match.color));
	memset(data.match.character, 0, sizeof (data.match.character));
	memset(data.match.spectator, 0, sizeof (data.match.spectator));
	memset(completed, 0, sizeof (completed));
	data.match.numplayers = 0;
	i = j = 0;

	for (j = 0; j < MAXPLAYERS; j++)
	{
		if (!playeringame[j])
			continue;

		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (!playeringame[i])
				continue;

			if (players[i].realtime <= data.match.scores[data.match.numplayers] && completed[i] == false)
			{
				data.match.scores[data.match.numplayers] = players[i].realtime;
				data.match.color[data.match.numplayers] = &players[i].skincolor;
				data.match.character[data.match.numplayers] = &players[i].skin;
				data.match.name[data.match.numplayers] = player_names[i];
				data.match.num[data.match.numplayers] = i;
			}
		}
		completed[data.match.num[data.match.numplayers]] = true;
		data.match.numplayers++;
	}
}

//
// Y_CalculateCompetitionWinners
//
static void Y_CalculateCompetitionWinners(void)
{
	INT32 i, j;
	boolean bestat[5];
	boolean completed[MAXPLAYERS];
	INT32 winner; // shortcut

	UINT32 points[MAXPLAYERS];
	UINT32 times[MAXPLAYERS];
	UINT32 rings[MAXPLAYERS];
	UINT32 maxrings[MAXPLAYERS];
	UINT32 monitors[MAXPLAYERS];
	UINT32 scores[MAXPLAYERS];
	char tempname[9];

	memset(data.competition.points, 0, sizeof (data.competition.points));
	memset(points, 0, sizeof (points));
	memset(completed, 0, sizeof (completed));

	// Award points.
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i])
			continue;

		for (j = 0; j < 5; j++)
			bestat[j] = true;

		if ((players[i].pflags & PF_GAMETYPEOVER) || players[i].lives <= 0)
			players[i].rings = 0;

		times[i]    = players[i].realtime;
		rings[i]    = (UINT32)max(players[i].rings, 0);
		maxrings[i] = (UINT32)players[i].totalring;
		monitors[i] = (UINT32)players[i].numboxes;
		scores[i]   = (UINT32)min(players[i].score, MAXSCORE);

		for (j = 0; j < MAXPLAYERS; j++)
		{
			if (!playeringame[j] || j == i)
				continue;

			if (players[i].realtime <= players[j].realtime)
				points[i]++;
			else
				bestat[0] = false;

			if (max(players[i].rings, 0) >= max(players[j].rings, 0))
				points[i]++;
			else
				bestat[1] = false;

			if (players[i].totalring >= players[j].totalring)
				points[i]++;
			else
				bestat[2] = false;

			if (players[i].numboxes >= players[j].numboxes)
				points[i]++;
			else
				bestat[3] = false;

			if (players[i].score >= players[j].score)
				points[i]++;
			else
				bestat[4] = false;
		}

		// Highlight best scores
		if (bestat[0])
			times[i] |= 0x80000000;
		if (bestat[1])
			rings[i] |= 0x80000000;
		if (bestat[2])
			maxrings[i] |= 0x80000000;
		if (bestat[3])
			monitors[i] |= 0x80000000;
		if (bestat[4])
			scores[i] |= 0x80000000;
	}

	// Now we go through and set the data.competition struct properly
	data.competition.numplayers = 0;
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i])
			continue;

		winner = 0;

		for (j = 0; j < MAXPLAYERS; j++)
		{
			if (!playeringame[j])
				continue;

			if (points[j] >= data.competition.points[data.competition.numplayers] && completed[j] == false)
			{
				data.competition.points[data.competition.numplayers] = points[j];
				data.competition.num[data.competition.numplayers] = winner = j;
			}
		}
		// We know this person won this spot, now let's set everything appropriately
		data.competition.times[data.competition.numplayers] = times[winner];
		data.competition.rings[data.competition.numplayers] = rings[winner];
		data.competition.maxrings[data.competition.numplayers] = maxrings[winner];
		data.competition.monitors[data.competition.numplayers] = monitors[winner];
		data.competition.scores[data.competition.numplayers] = scores[winner];

		strncpy(tempname, player_names[winner], 8);
		tempname[8] = '\0';
		strncpy(data.competition.name[data.competition.numplayers], tempname, 9);

		data.competition.color[data.competition.numplayers] = &players[winner].skincolor;
		data.competition.character[data.competition.numplayers] = &players[winner].skin;

		completed[winner] = true;
		data.competition.numplayers++;
	}
}

// ============
// COOP BONUSES
// ============

//
// Y_SetNullBonus
// No bonus in this slot, but we need to set some things anyway.
//
static void Y_SetNullBonus(player_t *player, y_bonus_t *bstruct)
{
	(void)player;
	memset(bstruct, 0, sizeof(y_bonus_t));
	strncpy(bstruct->patch, "MISSING", sizeof(bstruct->patch));
}

//
// Y_SetTimeBonus
//
static void Y_SetTimeBonus(player_t *player, y_bonus_t *bstruct)
{
	INT32 secs, bonus;

	strncpy(bstruct->patch, "YB_TIME", sizeof(bstruct->patch));
	bstruct->display = true;

	// calculate time bonus
	secs = player->realtime / TICRATE;
	if      (secs <  30) /*   :30 */ bonus = 50000;
	else if (secs <  60) /*  1:00 */ bonus = 10000;
	else if (secs <  90) /*  1:30 */ bonus = 5000;
	else if (secs < 120) /*  2:00 */ bonus = 4000;
	else if (secs < 180) /*  3:00 */ bonus = 3000;
	else if (secs < 240) /*  4:00 */ bonus = 2000;
	else if (secs < 300) /*  5:00 */ bonus = 1000;
	else if (secs < 360) /*  6:00 */ bonus = 500;
	else if (secs < 420) /*  7:00 */ bonus = 400;
	else if (secs < 480) /*  8:00 */ bonus = 300;
	else if (secs < 540) /*  9:00 */ bonus = 200;
	else if (secs < 600) /* 10:00 */ bonus = 100;
	else  /* TIME TAKEN: TOO LONG */ bonus = 0;
	bstruct->points = bonus;
}

//
// Y_SetRingBonus
//
static void Y_SetRingBonus(player_t *player, y_bonus_t *bstruct)
{
	strncpy(bstruct->patch, "YB_RING", sizeof(bstruct->patch));
	bstruct->display = true;
	bstruct->points = max(0, (player->rings) * 100);
}

//
// Y_SetNightsBonus
//
static void Y_SetNightsBonus(player_t *player, y_bonus_t *bstruct)
{
	strncpy(bstruct->patch, "YB_NIGHT", sizeof(bstruct->patch));
	bstruct->display = true;
	bstruct->points = player->totalmarescore;
}

//
// Y_SetLapBonus
//
static void Y_SetLapBonus(player_t *player, y_bonus_t *bstruct)
{
	strncpy(bstruct->patch, "YB_LAP", sizeof(bstruct->patch));
	bstruct->display = true;
	bstruct->points = max(0, player->totalmarebonuslap * 1000);
}

//
// Y_SetLinkBonus
//
static void Y_SetLinkBonus(player_t *player, y_bonus_t *bstruct)
{
	strncpy(bstruct->patch, "YB_LINK", sizeof(bstruct->patch));
	bstruct->display = true;
	bstruct->points = max(0, (player->maxlink - 1) * 100);
}

//
// Y_SetGuardBonus
//
static void Y_SetGuardBonus(player_t *player, y_bonus_t *bstruct)
{
	INT32 bonus;
	strncpy(bstruct->patch, "YB_GUARD", sizeof(bstruct->patch));
	bstruct->display = true;

	if      (player->timeshit == 0) bonus = 10000;
	else if (player->timeshit == 1) bonus = 5000;
	else if (player->timeshit == 2) bonus = 1000;
	else if (player->timeshit == 3) bonus = 500;
	else if (player->timeshit == 4) bonus = 100;
	else                            bonus = 0;
	bstruct->points = bonus;
}

//
// Y_SetPerfectBonus
//
static void Y_SetPerfectBonus(player_t *player, y_bonus_t *bstruct)
{
	INT32 i;

	(void)player;
	memset(bstruct, 0, sizeof(y_bonus_t));
	strncpy(bstruct->patch, "YB_PERFE", sizeof(bstruct->patch));

	if (intertype != int_coop || data.coop.gotperfbonus == -1)
	{
		INT32 sharedringtotal = 0;
		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (!playeringame[i]) continue;
			sharedringtotal += players[i].rings;
		}
		if (!sharedringtotal || nummaprings == -1 || sharedringtotal < nummaprings)
			bstruct->display = false;
		else
		{
			bstruct->display = true;
			bstruct->points = 50000;
		}
	}
	if (intertype != int_coop)
		return;

	data.coop.gotperfbonus = (bstruct->display ? 1 : 0);
}

static void Y_SetSpecialRingBonus(player_t *player, y_bonus_t *bstruct)
{
	INT32 i, sharedringtotal = 0;

	(void)player;
	strncpy(bstruct->patch, "YB_RING", sizeof(bstruct->patch));
	bstruct->display = true;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i]) continue;
		sharedringtotal += players[i].rings;
	}
	bstruct->points = max(0, (sharedringtotal) * 100);
}

// This list can be extended in the future with SOC/Lua, perhaps.
typedef void (*bonus_f)(player_t *, y_bonus_t *);
bonus_f bonuses_list[6][4] = {
	{
		Y_SetNullBonus,
		Y_SetNullBonus,
		Y_SetNullBonus,
		Y_SetNullBonus,
	},
	{
		Y_SetNullBonus,
		Y_SetTimeBonus,
		Y_SetRingBonus,
		Y_SetPerfectBonus,
	},
	{
		Y_SetNullBonus,
		Y_SetGuardBonus,
		Y_SetRingBonus,
		Y_SetNullBonus,
	},
	{
		Y_SetNullBonus,
		Y_SetGuardBonus,
		Y_SetRingBonus,
		Y_SetPerfectBonus,
	},
	{
		Y_SetNullBonus,
		Y_SetNightsBonus,
		Y_SetLapBonus,
		Y_SetNullBonus,
	},
	{
		Y_SetNullBonus,
		Y_SetLinkBonus,
		Y_SetLapBonus,
		Y_SetNullBonus,
	},
};



//
// Y_AwardCoopBonuses
//
// Awards the time and ring bonuses.
//
static void Y_AwardCoopBonuses(void)
{
	INT32 i, j, bonusnum, oldscore, ptlives;
	y_bonus_t localbonuses[4];

	// set score/total first
	data.coop.total = 0;
	data.coop.score = players[consoleplayer].score;
	data.coop.gotperfbonus = -1;
	memset(data.coop.bonuses, 0, sizeof(data.coop.bonuses));
	memset(data.coop.bonuspatches, 0, sizeof(data.coop.bonuspatches));

	for (i = 0; i < MAXPLAYERS; ++i)
	{
		if (!playeringame[i] || players[i].lives < 1) // not active or game over
			bonusnum = 0; // all null
		else
			bonusnum = mapheaderinfo[prevmap]->bonustype + 1; // -1 is none

		oldscore = players[i].score;

		for (j = 0; j < 4; ++j) // Set bonuses
		{
			(bonuses_list[bonusnum][j])(&players[i], &localbonuses[j]);
			players[i].score += localbonuses[j].points;
			if (players[i].score > MAXSCORE)
				players[i].score = MAXSCORE;
		}

		ptlives = min(
			(INT32)((!ultimatemode && !modeattacking && players[i].lives != INFLIVES) ? max((INT32)((players[i].score/50000) - (oldscore/50000)), (INT32)0) : 0),
			(INT32)(mapheaderinfo[prevmap]->maxbonuslives < 0 ? INT32_MAX : mapheaderinfo[prevmap]->maxbonuslives));
		if (ptlives)
			P_GivePlayerLives(&players[i], ptlives);

		if (i == consoleplayer)
		{
			data.coop.gotlife = (((netgame || multiplayer) && G_GametypeUsesCoopLives() && cv_cooplives.value == 0) ? 0 : ptlives);
			M_Memcpy(&data.coop.bonuses, &localbonuses, sizeof(data.coop.bonuses));
		}
	}

	// Just in case the perfect bonus wasn't checked.
	if (data.coop.gotperfbonus < 0)
		data.coop.gotperfbonus = 0;
}

//
// Y_AwardSpecialStageBonus
//
// Gives a ring bonus only.
static void Y_AwardSpecialStageBonus(void)
{
	INT32 i, oldscore, ptlives;
	y_bonus_t localbonuses[2];

	data.spec.score = players[consoleplayer].score;
	memset(data.spec.bonuses, 0, sizeof(data.spec.bonuses));
	memset(data.spec.bonuspatches, 0, sizeof(data.spec.bonuspatches));

	for (i = 0; i < MAXPLAYERS; i++)
	{
		oldscore = players[i].score;

		if (!playeringame[i] || players[i].lives < 1) // not active or game over
		{
			Y_SetNullBonus(&players[i], &localbonuses[0]);
			Y_SetNullBonus(&players[i], &localbonuses[1]);
		}
		else if (maptol & TOL_NIGHTS) // NiGHTS bonus score instead of Rings
		{
			Y_SetNightsBonus(&players[i], &localbonuses[0]);
			Y_SetNullBonus(&players[i], &localbonuses[1]);
		}
		else
		{
			Y_SetSpecialRingBonus(&players[i], &localbonuses[0]);
			Y_SetPerfectBonus(&players[i], &localbonuses[1]);
		}
		players[i].score += localbonuses[0].points;
		players[i].score += localbonuses[1].points;
		if (players[i].score > MAXSCORE)
			players[i].score = MAXSCORE;

		// grant extra lives right away since tally is faked
		ptlives = min(
			(INT32)((!ultimatemode && !modeattacking && players[i].lives != INFLIVES) ? max((INT32)((players[i].score/50000) - (oldscore/50000)), (INT32)0) : 0),
			(INT32)(mapheaderinfo[prevmap]->maxbonuslives < 0 ? INT32_MAX : mapheaderinfo[prevmap]->maxbonuslives));
		P_GivePlayerLives(&players[i], ptlives);

		if (i == consoleplayer)
		{
			data.spec.gotlife = (((netgame || multiplayer) && G_GametypeUsesCoopLives() && cv_cooplives.value == 0) ? 0 : ptlives);
			M_Memcpy(&data.spec.bonuses, &localbonuses, sizeof(data.spec.bonuses));

			// Continues related
			data.spec.continues = min(players[i].continues, 8);
			if (players[i].gotcontinue)
				data.spec.continues |= 0x80;
			data.spec.playercolor = &players[i].skincolor;
			data.spec.playerchar = &players[i].skin;
		}
	}
}

// ======

//
// Y_EndIntermission
//
void Y_EndIntermission(void)
{
	Y_UnloadData();

	endtic = -1;
	intertype = int_none;
	usebuffer = false;
}

#define UNLOAD(x) if (x) {Z_ChangeTag(x, PU_CACHE);} x = NULL;
#define CLEANUP(x) x = NULL;

//
// Y_UnloadData
//
static void Y_UnloadData(void)
{
	// In hardware mode, don't Z_ChangeTag a pointer returned by W_CachePatchName().
	// It doesn't work and is unnecessary.
	if (rendermode != render_soft)
		return;

	// unload the background patches
	UNLOAD(bgpatch);
	UNLOAD(bgtile);
	UNLOAD(interpic);

	switch (intertype)
	{
		case int_coop:
			// unload the coop and single player patches
			UNLOAD(data.coop.bonuspatches[3]);
			UNLOAD(data.coop.bonuspatches[2]);
			UNLOAD(data.coop.bonuspatches[1]);
			UNLOAD(data.coop.bonuspatches[0]);
			UNLOAD(data.coop.ptotal);
			break;
		case int_spec:
			// unload the special stage patches
			//UNLOAD(data.spec.cemerald);
			//UNLOAD(data.spec.nowsuper);
			UNLOAD(data.spec.bonuspatches[1]);
			UNLOAD(data.spec.bonuspatches[0]);
			UNLOAD(data.spec.pscore);
			UNLOAD(data.spec.pcontinues);
			break;
		case int_match:
		case int_race:
			UNLOAD(data.match.result);
			break;
		case int_ctf:
			UNLOAD(data.match.blueflag);
			UNLOAD(data.match.redflag);
			break;
		default:
			//without this default,
			//int_none, int_tag, int_chaos, and int_comp
			//are not handled
			break;
	}
}

static void Y_CleanupData(void)
{
	// unload the background patches
	CLEANUP(bgpatch);
	CLEANUP(bgtile);
	CLEANUP(interpic);

	switch (intertype)
	{
		case int_coop:
			// unload the coop and single player patches
			CLEANUP(data.coop.bonuspatches[3]);
			CLEANUP(data.coop.bonuspatches[2]);
			CLEANUP(data.coop.bonuspatches[1]);
			CLEANUP(data.coop.bonuspatches[0]);
			CLEANUP(data.coop.ptotal);
			break;
		case int_spec:
			// unload the special stage patches
			//CLEANUP(data.spec.cemerald);
			//CLEANUP(data.spec.nowsuper);
			CLEANUP(data.spec.bonuspatches[1]);
			CLEANUP(data.spec.bonuspatches[0]);
			CLEANUP(data.spec.pscore);
			CLEANUP(data.spec.pcontinues);
			break;
		case int_match:
		case int_race:
			CLEANUP(data.match.result);
			break;
		case int_ctf:
			CLEANUP(data.match.blueflag);
			CLEANUP(data.match.redflag);
			break;
		default:
			//without this default,
			//int_none, int_tag, int_chaos, and int_classicrace
			//are not handled
			break;
	}
}

// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2023-2023 by Louis-Antoine de Moulins de Rochefort.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  snake.c
/// \brief Snake minigame for the download screen.

#include "snake.h"
#include "g_input.h"
#include "m_random.h"
#include "s_sound.h"
#include "screen.h"
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"

#define SNAKE_SPEED 5

#define SNAKE_NUM_BLOCKS_X 20
#define SNAKE_NUM_BLOCKS_Y 10
#define SNAKE_BLOCK_SIZE 12
#define SNAKE_BORDER_SIZE 12

#define SNAKE_MAP_WIDTH  (SNAKE_NUM_BLOCKS_X * SNAKE_BLOCK_SIZE)
#define SNAKE_MAP_HEIGHT (SNAKE_NUM_BLOCKS_Y * SNAKE_BLOCK_SIZE)

#define SNAKE_LEFT_X ((BASEVIDWIDTH - SNAKE_MAP_WIDTH) / 2 - SNAKE_BORDER_SIZE)
#define SNAKE_RIGHT_X (SNAKE_LEFT_X + SNAKE_MAP_WIDTH + SNAKE_BORDER_SIZE * 2 - 1)
#define SNAKE_BOTTOM_Y (BASEVIDHEIGHT - 48)
#define SNAKE_TOP_Y (SNAKE_BOTTOM_Y - SNAKE_MAP_HEIGHT - SNAKE_BORDER_SIZE * 2 + 1)

enum snake_bonustype_s {
	SNAKE_BONUS_NONE = 0,
	SNAKE_BONUS_SLOW,
	SNAKE_BONUS_FAST,
	SNAKE_BONUS_GHOST,
	SNAKE_BONUS_NUKE,
	SNAKE_BONUS_SCISSORS,
	SNAKE_BONUS_REVERSE,
	SNAKE_BONUS_EGGMAN,
	SNAKE_NUM_BONUSES,
};

typedef struct snake_s
{
	boolean paused;
	boolean pausepressed;
	tic_t time;
	tic_t nextupdate;
	boolean gameover;
	UINT8 background;

	UINT16 snakelength;
	enum snake_bonustype_s snakebonus;
	tic_t snakebonustime;
	UINT8 snakex[SNAKE_NUM_BLOCKS_X * SNAKE_NUM_BLOCKS_Y];
	UINT8 snakey[SNAKE_NUM_BLOCKS_X * SNAKE_NUM_BLOCKS_Y];
	UINT8 snakedir[SNAKE_NUM_BLOCKS_X * SNAKE_NUM_BLOCKS_Y];

	UINT8 applex;
	UINT8 appley;

	enum snake_bonustype_s bonustype;
	UINT8 bonusx;
	UINT8 bonusy;
} snake_t;

static const char *snake_bonuspatches[] = {
	NULL,
	"DL_SLOW",
	"TVSSC0",
	"TVIVC0",
	"TVARC0",
	"DL_SCISSORS",
	"TVRCC0",
	"TVEGC0",
};

static const char *snake_backgrounds[] = {
	"RVPUMICF",
	"FRSTRCKF",
	"TAR",
	"MMFLRB4",
	"RVDARKF1",
	"RVZWALF1",
	"RVZWALF4",
	"RVZWALF5",
	"RVZGRS02",
	"RVZGRS04",
};

static void Snake_Initialise(snake_t *snake)
{
	snake->paused = false;
	snake->pausepressed = false;
	snake->time = 0;
	snake->nextupdate = SNAKE_SPEED;
	snake->gameover = false;
	snake->background = M_RandomKey(sizeof(snake_backgrounds) / sizeof(*snake_backgrounds));

	snake->snakelength = 1;
	snake->snakebonus = SNAKE_BONUS_NONE;
	snake->snakex[0] = M_RandomKey(SNAKE_NUM_BLOCKS_X);
	snake->snakey[0] = M_RandomKey(SNAKE_NUM_BLOCKS_Y);
	snake->snakedir[0] = 0;
	snake->snakedir[1] = 0;

	snake->applex = M_RandomKey(SNAKE_NUM_BLOCKS_X);
	snake->appley = M_RandomKey(SNAKE_NUM_BLOCKS_Y);

	snake->bonustype = SNAKE_BONUS_NONE;
}

static UINT8 Snake_GetOppositeDir(UINT8 dir)
{
	if (dir == 1 || dir == 3)
		return dir + 1;
	else if (dir == 2 || dir == 4)
		return dir - 1;
	else
		return 12 + 5 - dir;
}

static void Snake_FindFreeSlot(snake_t *snake, UINT8 *freex, UINT8 *freey, UINT8 headx, UINT8 heady)
{
	UINT8 x, y;
	UINT16 i;

	do
	{
		x = M_RandomKey(SNAKE_NUM_BLOCKS_X);
		y = M_RandomKey(SNAKE_NUM_BLOCKS_Y);

		for (i = 0; i < snake->snakelength; i++)
			if (x == snake->snakex[i] && y == snake->snakey[i])
				break;
	} while (i < snake->snakelength || (x == headx && y == heady)
		|| (x == snake->applex && y == snake->appley)
		|| (snake->bonustype != SNAKE_BONUS_NONE && x == snake->bonusx && y == snake->bonusy));

	*freex = x;
	*freey = y;
}

void Snake_Allocate(void **opaque)
{
	if (*opaque)
		Snake_Free(opaque);
	*opaque = malloc(sizeof(snake_t));
	Snake_Initialise(*opaque);
}

void Snake_Update(void *opaque)
{
	UINT8 x, y;
	UINT8 oldx, oldy;
	UINT16 i;
	UINT16 joystate = 0;

	snake_t *snake = opaque;

	// Handle retry
	if (snake->gameover && (G_PlayerInputDown(0, GC_JUMP) || gamekeydown[KEY_ENTER]))
	{
		Snake_Initialise(snake);
		snake->pausepressed = true; // Avoid accidental pause on respawn
	}

	// Handle pause
	if (G_PlayerInputDown(0, GC_PAUSE) || gamekeydown[KEY_ENTER])
	{
		if (!snake->pausepressed)
			snake->paused = !snake->paused;
		snake->pausepressed = true;
	}
	else
		snake->pausepressed = false;

	if (snake->paused)
		return;

	snake->time++;

	x = snake->snakex[0];
	y = snake->snakey[0];
	oldx = snake->snakex[1];
	oldy = snake->snakey[1];

	// Update direction
	if (G_PlayerInputDown(0, GC_STRAFELEFT) || gamekeydown[KEY_LEFTARROW] || joystate == 3)
	{
		if (snake->snakelength < 2 || x <= oldx)
			snake->snakedir[0] = 1;
	}
	else if (G_PlayerInputDown(0, GC_STRAFERIGHT) || gamekeydown[KEY_RIGHTARROW] || joystate == 4)
	{
		if (snake->snakelength < 2 || x >= oldx)
			snake->snakedir[0] = 2;
	}
	else if (G_PlayerInputDown(0, GC_FORWARD) || gamekeydown[KEY_UPARROW] || joystate == 1)
	{
		if (snake->snakelength < 2 || y <= oldy)
			snake->snakedir[0] = 3;
	}
	else if (G_PlayerInputDown(0, GC_BACKWARD) || gamekeydown[KEY_DOWNARROW] || joystate == 2)
	{
		if (snake->snakelength < 2 || y >= oldy)
			snake->snakedir[0] = 4;
	}

	if (snake->snakebonustime)
	{
		snake->snakebonustime--;
		if (!snake->snakebonustime)
			snake->snakebonus = SNAKE_BONUS_NONE;
	}

	snake->nextupdate--;
	if (snake->nextupdate)
		return;
	if (snake->snakebonus == SNAKE_BONUS_SLOW)
		snake->nextupdate = SNAKE_SPEED * 2;
	else if (snake->snakebonus == SNAKE_BONUS_FAST)
		snake->nextupdate = SNAKE_SPEED * 2 / 3;
	else
		snake->nextupdate = SNAKE_SPEED;

	if (snake->gameover)
		return;

	// Find new position
	switch (snake->snakedir[0])
	{
		case 1:
			if (x > 0)
				x--;
			else
				snake->gameover = true;
			break;
		case 2:
			if (x < SNAKE_NUM_BLOCKS_X - 1)
				x++;
			else
				snake->gameover = true;
			break;
		case 3:
			if (y > 0)
				y--;
			else
				snake->gameover = true;
			break;
		case 4:
			if (y < SNAKE_NUM_BLOCKS_Y - 1)
				y++;
			else
				snake->gameover = true;
			break;
	}

	// Check collision with snake
	if (snake->snakebonus != SNAKE_BONUS_GHOST)
		for (i = 1; i < snake->snakelength - 1; i++)
			if (x == snake->snakex[i] && y == snake->snakey[i])
			{
				if (snake->snakebonus == SNAKE_BONUS_SCISSORS)
				{
					snake->snakebonus = SNAKE_BONUS_NONE;
					snake->snakelength = i;
					S_StartSound(NULL, sfx_adderr);
				}
				else
					snake->gameover = true;
			}

	if (snake->gameover)
	{
		S_StartSound(NULL, sfx_lose);
		return;
	}

	// Check collision with apple
	if (x == snake->applex && y == snake->appley)
	{
		if (snake->snakelength + 3 < SNAKE_NUM_BLOCKS_X * SNAKE_NUM_BLOCKS_Y)
		{
			snake->snakelength++;
			snake->snakex  [snake->snakelength - 1] = snake->snakex  [snake->snakelength - 2];
			snake->snakey  [snake->snakelength - 1] = snake->snakey  [snake->snakelength - 2];
			snake->snakedir[snake->snakelength - 1] = snake->snakedir[snake->snakelength - 2];
		}

		// Spawn new apple
		Snake_FindFreeSlot(snake, &snake->applex, &snake->appley, x, y);

		// Spawn new bonus
		if (!(snake->snakelength % 5))
		{
			do
			{
				snake->bonustype = M_RandomKey(SNAKE_NUM_BONUSES - 1) + 1;
			} while (snake->snakelength > SNAKE_NUM_BLOCKS_X * SNAKE_NUM_BLOCKS_Y * 3 / 4
				&& (snake->bonustype == SNAKE_BONUS_EGGMAN || snake->bonustype == SNAKE_BONUS_FAST || snake->bonustype == SNAKE_BONUS_REVERSE));

			Snake_FindFreeSlot(snake, &snake->bonusx, &snake->bonusy, x, y);
		}

		S_StartSound(NULL, sfx_s3k6b);
	}

	if (snake->snakelength > 1 && snake->snakedir[0])
	{
		UINT8 dir = snake->snakedir[0];

		oldx = snake->snakex[1];
		oldy = snake->snakey[1];

		// Move
		for (i = snake->snakelength - 1; i > 0; i--)
		{
			snake->snakex[i] = snake->snakex[i - 1];
			snake->snakey[i] = snake->snakey[i - 1];
			snake->snakedir[i] = snake->snakedir[i - 1];
		}

		// Handle corners
		if      (x < oldx && dir == 3)
			dir = 5;
		else if (x > oldx && dir == 3)
			dir = 6;
		else if (x < oldx && dir == 4)
			dir = 7;
		else if (x > oldx && dir == 4)
			dir = 8;
		else if (y < oldy && dir == 1)
			dir = 9;
		else if (y < oldy && dir == 2)
			dir = 10;
		else if (y > oldy && dir == 1)
			dir = 11;
		else if (y > oldy && dir == 2)
			dir = 12;
		snake->snakedir[1] = dir;
	}

	snake->snakex[0] = x;
	snake->snakey[0] = y;

	// Check collision with bonus
	if (snake->bonustype != SNAKE_BONUS_NONE && x == snake->bonusx && y == snake->bonusy)
	{
		S_StartSound(NULL, sfx_ncchip);

		switch (snake->bonustype)
		{
		case SNAKE_BONUS_SLOW:
			snake->snakebonus = SNAKE_BONUS_SLOW;
			snake->snakebonustime = 20 * TICRATE;
			break;
		case SNAKE_BONUS_FAST:
			snake->snakebonus = SNAKE_BONUS_FAST;
			snake->snakebonustime = 20 * TICRATE;
			break;
		case SNAKE_BONUS_GHOST:
			snake->snakebonus = SNAKE_BONUS_GHOST;
			snake->snakebonustime = 10 * TICRATE;
			break;
		case SNAKE_BONUS_NUKE:
			for (i = 0; i < snake->snakelength; i++)
			{
				snake->snakex  [i] = snake->snakex  [0];
				snake->snakey  [i] = snake->snakey  [0];
				snake->snakedir[i] = snake->snakedir[0];
			}

			S_StartSound(NULL, sfx_bkpoof);
			break;
		case SNAKE_BONUS_SCISSORS:
			snake->snakebonus = SNAKE_BONUS_SCISSORS;
			snake->snakebonustime = 60 * TICRATE;
			break;
		case SNAKE_BONUS_REVERSE:
			for (i = 0; i < (snake->snakelength + 1) / 2; i++)
			{
				UINT16 i2 = snake->snakelength - 1 - i;
				UINT8 tmpx   = snake->snakex  [i];
				UINT8 tmpy   = snake->snakey  [i];
				UINT8 tmpdir = snake->snakedir[i];

				// Swap first segment with last segment
				snake->snakex  [i] = snake->snakex  [i2];
				snake->snakey  [i] = snake->snakey  [i2];
				snake->snakedir[i] = Snake_GetOppositeDir(snake->snakedir[i2]);
				snake->snakex  [i2] = tmpx;
				snake->snakey  [i2] = tmpy;
				snake->snakedir[i2] = Snake_GetOppositeDir(tmpdir);
			}

			snake->snakedir[0] = 0;

			S_StartSound(NULL, sfx_gravch);
			break;
		default:
			if (snake->snakebonus != SNAKE_BONUS_GHOST)
			{
				snake->gameover = true;
				S_StartSound(NULL, sfx_lose);
			}
		}

		snake->bonustype = SNAKE_BONUS_NONE;
	}
}

void Snake_Draw(void *opaque)
{
	INT16 i;

	snake_t *snake = opaque;

	// Background
	V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 31);

	V_DrawFlatFill(
		SNAKE_LEFT_X + SNAKE_BORDER_SIZE,
		SNAKE_TOP_Y  + SNAKE_BORDER_SIZE,
		SNAKE_MAP_WIDTH,
		SNAKE_MAP_HEIGHT,
		W_GetNumForName(snake_backgrounds[snake->background])
	);

	// Borders
	V_DrawFill(SNAKE_LEFT_X, SNAKE_TOP_Y, SNAKE_BORDER_SIZE + SNAKE_MAP_WIDTH, SNAKE_BORDER_SIZE, 242); // Top
	V_DrawFill(SNAKE_LEFT_X + SNAKE_BORDER_SIZE + SNAKE_MAP_WIDTH, SNAKE_TOP_Y, SNAKE_BORDER_SIZE, SNAKE_BORDER_SIZE + SNAKE_MAP_HEIGHT, 242); // Right
	V_DrawFill(SNAKE_LEFT_X + SNAKE_BORDER_SIZE, SNAKE_TOP_Y + SNAKE_BORDER_SIZE + SNAKE_MAP_HEIGHT, SNAKE_BORDER_SIZE + SNAKE_MAP_WIDTH, SNAKE_BORDER_SIZE, 242); // Bottom
	V_DrawFill(SNAKE_LEFT_X, SNAKE_TOP_Y + SNAKE_BORDER_SIZE, SNAKE_BORDER_SIZE, SNAKE_BORDER_SIZE + SNAKE_MAP_HEIGHT, 242); // Left

	// Apple
	V_DrawFixedPatch(
		(SNAKE_LEFT_X + SNAKE_BORDER_SIZE + snake->applex * SNAKE_BLOCK_SIZE + SNAKE_BLOCK_SIZE / 2) * FRACUNIT,
		(SNAKE_TOP_Y  + SNAKE_BORDER_SIZE + snake->appley * SNAKE_BLOCK_SIZE + SNAKE_BLOCK_SIZE / 2) * FRACUNIT,
		FRACUNIT / 4,
		0,
		W_CachePatchLongName("DL_APPLE", PU_HUDGFX),
		NULL
	);

	// Bonus
	if (snake->bonustype != SNAKE_BONUS_NONE)
		V_DrawFixedPatch(
			(SNAKE_LEFT_X + SNAKE_BORDER_SIZE + snake->bonusx * SNAKE_BLOCK_SIZE + SNAKE_BLOCK_SIZE / 2    ) * FRACUNIT,
			(SNAKE_TOP_Y  + SNAKE_BORDER_SIZE + snake->bonusy * SNAKE_BLOCK_SIZE + SNAKE_BLOCK_SIZE / 2 + 4) * FRACUNIT,
			FRACUNIT / 2,
			0,
			W_CachePatchLongName(snake_bonuspatches[snake->bonustype], PU_HUDGFX),
			NULL
		);

	// Snake
	if (!snake->gameover || snake->time % 8 < 8 / 2) // Blink if game over
	{
		for (i = snake->snakelength - 1; i >= 0; i--)
		{
			const char *patchname;
			UINT8 dir = snake->snakedir[i];

			if (i == 0) // Head
			{
				switch (dir)
				{
					case  1: patchname = "DL_SNAKEHEAD_L"; break;
					case  2: patchname = "DL_SNAKEHEAD_R"; break;
					case  3: patchname = "DL_SNAKEHEAD_T"; break;
					case  4: patchname = "DL_SNAKEHEAD_B"; break;
					default: patchname = "DL_SNAKEHEAD_M";
				}
			}
			else // Body
			{
				switch (dir)
				{
					case  1: patchname = "DL_SNAKEBODY_L"; break;
					case  2: patchname = "DL_SNAKEBODY_R"; break;
					case  3: patchname = "DL_SNAKEBODY_T"; break;
					case  4: patchname = "DL_SNAKEBODY_B"; break;
					case  5: patchname = "DL_SNAKEBODY_LT"; break;
					case  6: patchname = "DL_SNAKEBODY_RT"; break;
					case  7: patchname = "DL_SNAKEBODY_LB"; break;
					case  8: patchname = "DL_SNAKEBODY_RB"; break;
					case  9: patchname = "DL_SNAKEBODY_TL"; break;
					case 10: patchname = "DL_SNAKEBODY_TR"; break;
					case 11: patchname = "DL_SNAKEBODY_BL"; break;
					case 12: patchname = "DL_SNAKEBODY_BR"; break;
					default: patchname = "DL_SNAKEBODY_B";
				}
			}

			V_DrawFixedPatch(
				(SNAKE_LEFT_X + SNAKE_BORDER_SIZE + snake->snakex[i] * SNAKE_BLOCK_SIZE + SNAKE_BLOCK_SIZE / 2) * FRACUNIT,
				(SNAKE_TOP_Y  + SNAKE_BORDER_SIZE + snake->snakey[i] * SNAKE_BLOCK_SIZE + SNAKE_BLOCK_SIZE / 2) * FRACUNIT,
				i == 0 && dir == 0 ? FRACUNIT / 5 : FRACUNIT / 2,
				snake->snakebonus == SNAKE_BONUS_GHOST ? V_TRANSLUCENT : 0,
				W_CachePatchLongName(patchname, PU_HUDGFX),
				NULL
			);
		}
	}

	// Length
	V_DrawString(SNAKE_RIGHT_X + 4, SNAKE_TOP_Y, V_MONOSPACE, va("%u", snake->snakelength));

	// Bonus
	if (snake->snakebonus != SNAKE_BONUS_NONE
	&& (snake->snakebonustime >= 3 * TICRATE || snake->time % 4 < 4 / 2))
		V_DrawFixedPatch(
			(SNAKE_RIGHT_X + 10) * FRACUNIT,
			(SNAKE_TOP_Y + 24) * FRACUNIT,
			FRACUNIT / 2,
			0,
			W_CachePatchLongName(snake_bonuspatches[snake->snakebonus], PU_HUDGFX),
			NULL
		);
}

void Snake_Free(void **opaque)
{
	if (*opaque)
	{
		free(opaque);
		*opaque = NULL;
	}
}

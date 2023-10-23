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
#include "g_game.h"
#include "i_joy.h"
#include "m_random.h"
#include "s_sound.h"
#include "screen.h"
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"

#define SPEED 5

#define NUM_BLOCKS_X 20
#define NUM_BLOCKS_Y 10
#define BLOCK_SIZE 12
#define BORDER_SIZE 12

#define MAP_WIDTH  (NUM_BLOCKS_X * BLOCK_SIZE)
#define MAP_HEIGHT (NUM_BLOCKS_Y * BLOCK_SIZE)

#define LEFT_X ((BASEVIDWIDTH - MAP_WIDTH) / 2 - BORDER_SIZE)
#define RIGHT_X (LEFT_X + MAP_WIDTH + BORDER_SIZE * 2 - 1)
#define BOTTOM_Y (BASEVIDHEIGHT - 48)
#define TOP_Y (BOTTOM_Y - MAP_HEIGHT - BORDER_SIZE * 2 + 1)

enum bonustype_s {
	BONUS_NONE = 0,
	BONUS_SLOW,
	BONUS_FAST,
	BONUS_GHOST,
	BONUS_NUKE,
	BONUS_SCISSORS,
	BONUS_REVERSE,
	BONUS_EGGMAN,
	NUM_BONUSES,
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
	enum bonustype_s snakebonus;
	tic_t snakebonustime;
	UINT8 snakex[NUM_BLOCKS_X * NUM_BLOCKS_Y];
	UINT8 snakey[NUM_BLOCKS_X * NUM_BLOCKS_Y];
	UINT8 snakedir[NUM_BLOCKS_X * NUM_BLOCKS_Y];

	UINT8 applex;
	UINT8 appley;

	enum bonustype_s bonustype;
	UINT8 bonusx;
	UINT8 bonusy;

	event_t *joyevents[MAXEVENTS];
	UINT16 joyeventcount;
} snake_t;

static const char *bonuspatches[] = {
	NULL,
	"DL_SLOW",
	"TVSSC0",
	"TVIVC0",
	"TVARC0",
	"DL_SCISSORS",
	"TVRCC0",
	"TVEGC0",
};

static const char *backgrounds[] = {
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

static void Initialise(snake_t *snake)
{
	snake->paused = false;
	snake->pausepressed = false;
	snake->time = 0;
	snake->nextupdate = SPEED;
	snake->gameover = false;
	snake->background = M_RandomKey(sizeof(backgrounds) / sizeof(*backgrounds));

	snake->snakelength = 1;
	snake->snakebonus = BONUS_NONE;
	snake->snakex[0] = M_RandomKey(NUM_BLOCKS_X);
	snake->snakey[0] = M_RandomKey(NUM_BLOCKS_Y);
	snake->snakedir[0] = 0;
	snake->snakedir[1] = 0;

	snake->applex = M_RandomKey(NUM_BLOCKS_X);
	snake->appley = M_RandomKey(NUM_BLOCKS_Y);

	snake->bonustype = BONUS_NONE;

	snake->joyeventcount = 0;
}

static UINT8 GetOppositeDir(UINT8 dir)
{
	if (dir == 1 || dir == 3)
		return dir + 1;
	else if (dir == 2 || dir == 4)
		return dir - 1;
	else
		return 12 + 5 - dir;
}

static void FindFreeSlot(snake_t *snake, UINT8 *freex, UINT8 *freey, UINT8 headx, UINT8 heady)
{
	UINT8 x, y;
	UINT16 i;

	do
	{
		x = M_RandomKey(NUM_BLOCKS_X);
		y = M_RandomKey(NUM_BLOCKS_Y);

		for (i = 0; i < snake->snakelength; i++)
			if (x == snake->snakex[i] && y == snake->snakey[i])
				break;
	} while (i < snake->snakelength || (x == headx && y == heady)
		|| (x == snake->applex && y == snake->appley)
		|| (snake->bonustype != BONUS_NONE && x == snake->bonusx && y == snake->bonusy));

	*freex = x;
	*freey = y;
}

void Snake_Allocate(void **opaque)
{
	if (*opaque)
		Snake_Free(opaque);
	*opaque = malloc(sizeof(snake_t));
	Initialise(*opaque);
}

void Snake_Update(void *opaque)
{
	UINT8 x, y;
	UINT8 oldx, oldy;
	UINT16 i;
	UINT16 joystate = 0;
	static INT32 pjoyx = 0, pjoyy = 0;

	snake_t *snake = opaque;

	// Handle retry
	if (snake->gameover && (PLAYER1INPUTDOWN(GC_JUMP) || gamekeydown[KEY_ENTER]))
	{
		Initialise(snake);
		snake->pausepressed = true; // Avoid accidental pause on respawn
	}

	// Handle pause
	if (PLAYER1INPUTDOWN(GC_PAUSE) || gamekeydown[KEY_ENTER])
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

	// Process the input events in here dear lord
	for (UINT16 j = 0; j < snake->joyeventcount; j++)
	{
		event_t *ev = snake->joyevents[j];
		const INT32 jdeadzone = (JOYAXISRANGE * cv_digitaldeadzone.value) / FRACUNIT;
		if (ev->y != INT32_MAX)
		{
			if (Joystick.bGamepadStyle || abs(ev->y) > jdeadzone)
			{
				if (ev->y < 0 && pjoyy >= 0)
					joystate = 1;
				else if (ev->y > 0 && pjoyy <= 0)
					joystate = 2;
				pjoyy = ev->y;
			}
			else
				pjoyy = 0;
		}

		if (ev->x != INT32_MAX)
		{
			if (Joystick.bGamepadStyle || abs(ev->x) > jdeadzone)
			{
				if (ev->x < 0 && pjoyx >= 0)
					joystate = 3;
				else if (ev->x > 0 && pjoyx <= 0)
					joystate = 4;
				pjoyx = ev->x;
			}
			else
				pjoyx = 0;
		}
	}
	snake->joyeventcount = 0;

	// Update direction
	if (PLAYER1INPUTDOWN(GC_STRAFELEFT) || gamekeydown[KEY_LEFTARROW] || joystate == 3)
	{
		if (snake->snakelength < 2 || x <= oldx)
			snake->snakedir[0] = 1;
	}
	else if (PLAYER1INPUTDOWN(GC_STRAFERIGHT) || gamekeydown[KEY_RIGHTARROW] || joystate == 4)
	{
		if (snake->snakelength < 2 || x >= oldx)
			snake->snakedir[0] = 2;
	}
	else if (PLAYER1INPUTDOWN(GC_FORWARD) || gamekeydown[KEY_UPARROW] || joystate == 1)
	{
		if (snake->snakelength < 2 || y <= oldy)
			snake->snakedir[0] = 3;
	}
	else if (PLAYER1INPUTDOWN(GC_BACKWARD) || gamekeydown[KEY_DOWNARROW] || joystate == 2)
	{
		if (snake->snakelength < 2 || y >= oldy)
			snake->snakedir[0] = 4;
	}

	if (snake->snakebonustime)
	{
		snake->snakebonustime--;
		if (!snake->snakebonustime)
			snake->snakebonus = BONUS_NONE;
	}

	snake->nextupdate--;
	if (snake->nextupdate)
		return;
	if (snake->snakebonus == BONUS_SLOW)
		snake->nextupdate = SPEED * 2;
	else if (snake->snakebonus == BONUS_FAST)
		snake->nextupdate = SPEED * 2 / 3;
	else
		snake->nextupdate = SPEED;

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
			if (x < NUM_BLOCKS_X - 1)
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
			if (y < NUM_BLOCKS_Y - 1)
				y++;
			else
				snake->gameover = true;
			break;
	}

	// Check collision with snake
	if (snake->snakebonus != BONUS_GHOST)
		for (i = 1; i < snake->snakelength - 1; i++)
			if (x == snake->snakex[i] && y == snake->snakey[i])
			{
				if (snake->snakebonus == BONUS_SCISSORS)
				{
					snake->snakebonus = BONUS_NONE;
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
		if (snake->snakelength + 3 < NUM_BLOCKS_X * NUM_BLOCKS_Y)
		{
			snake->snakelength++;
			snake->snakex  [snake->snakelength - 1] = snake->snakex  [snake->snakelength - 2];
			snake->snakey  [snake->snakelength - 1] = snake->snakey  [snake->snakelength - 2];
			snake->snakedir[snake->snakelength - 1] = snake->snakedir[snake->snakelength - 2];
		}

		// Spawn new apple
		FindFreeSlot(snake, &snake->applex, &snake->appley, x, y);

		// Spawn new bonus
		if (!(snake->snakelength % 5))
		{
			do
			{
				snake->bonustype = M_RandomKey(NUM_BONUSES - 1) + 1;
			} while (snake->snakelength > NUM_BLOCKS_X * NUM_BLOCKS_Y * 3 / 4
				&& (snake->bonustype == BONUS_EGGMAN || snake->bonustype == BONUS_FAST || snake->bonustype == BONUS_REVERSE));

			FindFreeSlot(snake, &snake->bonusx, &snake->bonusy, x, y);
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
	if (snake->bonustype != BONUS_NONE && x == snake->bonusx && y == snake->bonusy)
	{
		S_StartSound(NULL, sfx_ncchip);

		switch (snake->bonustype)
		{
		case BONUS_SLOW:
			snake->snakebonus = BONUS_SLOW;
			snake->snakebonustime = 20 * TICRATE;
			break;
		case BONUS_FAST:
			snake->snakebonus = BONUS_FAST;
			snake->snakebonustime = 20 * TICRATE;
			break;
		case BONUS_GHOST:
			snake->snakebonus = BONUS_GHOST;
			snake->snakebonustime = 10 * TICRATE;
			break;
		case BONUS_NUKE:
			for (i = 0; i < snake->snakelength; i++)
			{
				snake->snakex  [i] = snake->snakex  [0];
				snake->snakey  [i] = snake->snakey  [0];
				snake->snakedir[i] = snake->snakedir[0];
			}

			S_StartSound(NULL, sfx_bkpoof);
			break;
		case BONUS_SCISSORS:
			snake->snakebonus = BONUS_SCISSORS;
			snake->snakebonustime = 60 * TICRATE;
			break;
		case BONUS_REVERSE:
			for (i = 0; i < (snake->snakelength + 1) / 2; i++)
			{
				UINT16 i2 = snake->snakelength - 1 - i;
				UINT8 tmpx   = snake->snakex  [i];
				UINT8 tmpy   = snake->snakey  [i];
				UINT8 tmpdir = snake->snakedir[i];

				// Swap first segment with last segment
				snake->snakex  [i] = snake->snakex  [i2];
				snake->snakey  [i] = snake->snakey  [i2];
				snake->snakedir[i] = GetOppositeDir(snake->snakedir[i2]);
				snake->snakex  [i2] = tmpx;
				snake->snakey  [i2] = tmpy;
				snake->snakedir[i2] = GetOppositeDir(tmpdir);
			}

			snake->snakedir[0] = 0;

			S_StartSound(NULL, sfx_gravch);
			break;
		default:
			if (snake->snakebonus != BONUS_GHOST)
			{
				snake->gameover = true;
				S_StartSound(NULL, sfx_lose);
			}
		}

		snake->bonustype = BONUS_NONE;
	}
}

void Snake_Draw(void *opaque)
{
	INT16 i;

	snake_t *snake = opaque;

	// Background
	V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 31);

	V_DrawFlatFill(
		LEFT_X + BORDER_SIZE,
		TOP_Y  + BORDER_SIZE,
		MAP_WIDTH,
		MAP_HEIGHT,
		W_GetNumForName(backgrounds[snake->background])
	);

	// Borders
	V_DrawFill(LEFT_X, TOP_Y, BORDER_SIZE + MAP_WIDTH, BORDER_SIZE, 242); // Top
	V_DrawFill(LEFT_X + BORDER_SIZE + MAP_WIDTH, TOP_Y, BORDER_SIZE, BORDER_SIZE + MAP_HEIGHT, 242); // Right
	V_DrawFill(LEFT_X + BORDER_SIZE, TOP_Y + BORDER_SIZE + MAP_HEIGHT, BORDER_SIZE + MAP_WIDTH, BORDER_SIZE, 242); // Bottom
	V_DrawFill(LEFT_X, TOP_Y + BORDER_SIZE, BORDER_SIZE, BORDER_SIZE + MAP_HEIGHT, 242); // Left

	// Apple
	V_DrawFixedPatch(
		(LEFT_X + BORDER_SIZE + snake->applex * BLOCK_SIZE + BLOCK_SIZE / 2) * FRACUNIT,
		(TOP_Y  + BORDER_SIZE + snake->appley * BLOCK_SIZE + BLOCK_SIZE / 2) * FRACUNIT,
		FRACUNIT / 4,
		0,
		W_CachePatchLongName("DL_APPLE", PU_HUDGFX),
		NULL
	);

	// Bonus
	if (snake->bonustype != BONUS_NONE)
		V_DrawFixedPatch(
			(LEFT_X + BORDER_SIZE + snake->bonusx * BLOCK_SIZE + BLOCK_SIZE / 2    ) * FRACUNIT,
			(TOP_Y  + BORDER_SIZE + snake->bonusy * BLOCK_SIZE + BLOCK_SIZE / 2 + 4) * FRACUNIT,
			FRACUNIT / 2,
			0,
			W_CachePatchLongName(bonuspatches[snake->bonustype], PU_HUDGFX),
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
				(LEFT_X + BORDER_SIZE + snake->snakex[i] * BLOCK_SIZE + BLOCK_SIZE / 2) * FRACUNIT,
				(TOP_Y  + BORDER_SIZE + snake->snakey[i] * BLOCK_SIZE + BLOCK_SIZE / 2) * FRACUNIT,
				i == 0 && dir == 0 ? FRACUNIT / 5 : FRACUNIT / 2,
				snake->snakebonus == BONUS_GHOST ? V_TRANSLUCENT : 0,
				W_CachePatchLongName(patchname, PU_HUDGFX),
				NULL
			);
		}
	}

	// Length
	V_DrawString(RIGHT_X + 4, TOP_Y, V_MONOSPACE, va("%u", snake->snakelength));

	// Bonus
	if (snake->snakebonus != BONUS_NONE
	&& (snake->snakebonustime >= 3 * TICRATE || snake->time % 4 < 4 / 2))
		V_DrawFixedPatch(
			(RIGHT_X + 10) * FRACUNIT,
			(TOP_Y + 24) * FRACUNIT,
			FRACUNIT / 2,
			0,
			W_CachePatchLongName(bonuspatches[snake->snakebonus], PU_HUDGFX),
			NULL
		);
}

void Snake_Free(void **opaque)
{
	if (*opaque)
	{
		free(*opaque);
		*opaque = NULL;
	}
}

// I'm screaming the hack is clean - ashi
boolean Snake_JoyGrabber(void *opaque, event_t *ev)
{
	snake_t *snake = opaque;

	if (ev->type == ev_joystick  && ev->key == 0)
	{
		snake->joyevents[snake->joyeventcount] = ev;
		snake->joyeventcount++;
		return true;
	}
	else
		return false;
}

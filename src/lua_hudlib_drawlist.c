// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2014-2016 by John "JTE" Muniz.
// Copyright (C) 2014-2023 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  lua_hudlib_drawlist.c
/// \brief a data structure for managing cached drawlists for the Lua hud lib

#include "lua_hudlib_drawlist.h"

#include <string.h>

#include "v_video.h"
#include "z_zone.h"

enum drawitem_e {
	DI_Draw = 0,
	DI_DrawScaled,
	DI_DrawStretched,
	DI_DrawCropped,
	DI_DrawNum,
	DI_DrawPaddedNum,
	DI_DrawFill,
	DI_DrawString,
	DI_DrawNameTag,
	DI_DrawScaledNameTag,
	DI_DrawLevelTitle,
	DI_FadeScreen,
	DI_MAX,
};

// A single draw item with all possible arguments needed for a draw call.
typedef struct drawitem_s {
	enum drawitem_e type;
	fixed_t x;
	fixed_t y;
	fixed_t w;
	fixed_t h;
	INT32 c;
	fixed_t scale;
	fixed_t hscale;
	fixed_t vscale;
	patch_t *patch;
	INT32 flags;
	UINT16 basecolor;
	UINT16 outlinecolor;
	UINT8 *colormap;
	UINT8 *basecolormap;
	UINT8 *outlinecolormap;
	fixed_t sx;
	fixed_t sy;
	INT32 num;
	INT32 digits;
	const char *str;
	UINT16 color;
	UINT8 strength;
	INT32 align;
} drawitem_t;

// The internal structure of a drawlist.
struct huddrawlist_s {
	drawitem_t *items;
	size_t items_capacity;
	size_t items_len;
	char *strbuf;
	size_t strbuf_capacity;
	size_t strbuf_len;
};

// alignment types for v.drawString
enum align {
	align_left = 0,
	align_center,
	align_right,
	align_fixed,
	align_fixedcenter,
	align_fixedright,
	align_small,
	align_smallfixed,
	align_smallfixedcenter,
	align_smallfixedright,
	align_smallcenter,
	align_smallright,
	align_smallthin,
	align_smallthincenter,
	align_smallthinright,
	align_smallthinfixed,
	align_smallthinfixedcenter,
	align_smallthinfixedright,
	align_thin,
	align_thinfixed,
	align_thinfixedcenter,
	align_thinfixedright,
	align_thincenter,
	align_thinright
};

huddrawlist_h LUA_HUD_CreateDrawList(void)
{
	huddrawlist_h drawlist;

	drawlist = (huddrawlist_h) Z_CallocAlign(sizeof(struct huddrawlist_s), PU_STATIC, NULL, 64);
	drawlist->items = NULL;
	drawlist->items_capacity = 0;
	drawlist->items_len = 0;
	drawlist->strbuf = NULL;
	drawlist->strbuf_capacity = 0;
	drawlist->strbuf_len = 0;

	return drawlist;
}

void LUA_HUD_ClearDrawList(huddrawlist_h list)
{
	// rather than deallocate, we'll just save the existing allocation and empty
	// it out for reuse

	// this memset probably isn't necessary
	if (list->items)
	{
		memset(list->items, 0, sizeof(drawitem_t) * list->items_capacity);
	}

	list->items_len = 0;

	if (list->strbuf)
	{
		list->strbuf[0] = 0;
	}
	list->strbuf_len = 0;
}

void LUA_HUD_DestroyDrawList(huddrawlist_h list)
{
	if (list == NULL) return;

	if (list->items)
	{
		Z_Free(list->items);
	}
	Z_Free(list);
}

boolean LUA_HUD_IsDrawListValid(huddrawlist_h list)
{
	if (!list) return false;

	// that's all we can really do to check the validity of the handle right now
	return true;
}

static size_t AllocateDrawItem(huddrawlist_h list)
{
	if (!list) I_Error("can't allocate draw item: invalid list");
	if (list->items_capacity <= list->items_len + 1)
	{
		if (list->items_capacity == 0) list->items_capacity = 128;
		else list->items_capacity *= 2;
		list->items = (drawitem_t *) Z_ReallocAlign(list->items, sizeof(struct drawitem_s) * list->items_capacity, PU_STATIC, NULL, 64);
	}

	return list->items_len++;
}

// copy string to list's internal string buffer
// lua can deallocate the string before we get to use it, so it's important to
// keep our own copy
static const char *CopyString(huddrawlist_h list, const char* str)
{
	size_t lenstr;

	if (!list) I_Error("can't allocate string; invalid list");
	lenstr = strlen(str);
	if (list->strbuf_capacity <= list->strbuf_len + lenstr + 1)
	{
		if (list->strbuf_capacity == 0) list->strbuf_capacity = 256;
		else list->strbuf_capacity *= 2;
		list->strbuf = (char*) Z_ReallocAlign(list->strbuf, sizeof(char) * list->strbuf_capacity, PU_STATIC, NULL, 8);
	}
	const char *result = (const char *) &list->strbuf[list->strbuf_len];
	strncpy(&list->strbuf[list->strbuf_len], str, lenstr + 1);
	list->strbuf_len += lenstr + 1;
	return result;
}

void LUA_HUD_AddDraw(
	huddrawlist_h list,
	INT32 x,
	INT32 y,
	patch_t *patch,
	INT32 flags,
	UINT8 *colormap
)
{
	size_t i = AllocateDrawItem(list);
	drawitem_t *item = &list->items[i];
	item->type = DI_Draw;
	item->x = x;
	item->y = y;
	item->patch = patch;
	item->flags = flags;
	item->colormap = colormap;
}

void LUA_HUD_AddDrawScaled(
	huddrawlist_h list,
	fixed_t x,
	fixed_t y,
	fixed_t scale,
	patch_t *patch,
	INT32 flags,
	UINT8 *colormap
)
{
	size_t i = AllocateDrawItem(list);
	drawitem_t *item = &list->items[i];
	item->type = DI_DrawScaled;
	item->x = x;
	item->y = y;
	item->scale = scale;
	item->patch = patch;
	item->flags = flags;
	item->colormap = colormap;
}

void LUA_HUD_AddDrawStretched(
	huddrawlist_h list,
	fixed_t x,
	fixed_t y,
	fixed_t hscale,
	fixed_t vscale,
	patch_t *patch,
	INT32 flags,
	UINT8 *colormap
)
{
	size_t i = AllocateDrawItem(list);
	drawitem_t *item = &list->items[i];
	item->type = DI_DrawStretched;
	item->x = x;
	item->y = y;
	item->hscale = hscale;
	item->vscale = vscale;
	item->patch = patch;
	item->flags = flags;
	item->colormap = colormap;
}

void LUA_HUD_AddDrawCropped(
	huddrawlist_h list,
	fixed_t x,
	fixed_t y,
	fixed_t hscale,
	fixed_t vscale,
	patch_t *patch,
	INT32 flags,
	UINT8 *colormap,
	fixed_t sx,
	fixed_t sy,
	fixed_t w,
	fixed_t h
)
{
	size_t i = AllocateDrawItem(list);
	drawitem_t *item = &list->items[i];
	item->type = DI_DrawCropped;
	item->x = x;
	item->y = y;
	item->hscale = hscale;
	item->vscale = vscale;
	item->patch = patch;
	item->flags = flags;
	item->colormap = colormap;
	item->sx = sx;
	item->sy = sy;
	item->w = w;
	item->h = h;
}

void LUA_HUD_AddDrawNum(
	huddrawlist_h list,
	INT32 x,
	INT32 y,
	INT32 num,
	INT32 flags
)
{
	size_t i = AllocateDrawItem(list);
	drawitem_t *item = &list->items[i];
	item->type = DI_DrawNum;
	item->x = x;
	item->y = y;
	item->num = num;
	item->flags = flags;
}

void LUA_HUD_AddDrawPaddedNum(
	huddrawlist_h list,
	INT32 x,
	INT32 y,
	INT32 num,
	INT32 digits,
	INT32 flags
)
{
	size_t i = AllocateDrawItem(list);
	drawitem_t *item = &list->items[i];
	item->type = DI_DrawPaddedNum;
	item->x = x;
	item->y = y;
	item->num = num;
	item->digits = digits;
	item->flags = flags;
}

void LUA_HUD_AddDrawFill(
	huddrawlist_h list,
	INT32 x,
	INT32 y,
	INT32 w,
	INT32 h,
	INT32 c
)
{
	size_t i = AllocateDrawItem(list);
	drawitem_t *item = &list->items[i];
	item->type = DI_DrawFill;
	item->x = x;
	item->y = y;
	item->w = w;
	item->h = h;
	item->c = c;
}

void LUA_HUD_AddDrawString(
	huddrawlist_h list,
	fixed_t x,
	fixed_t y,
	const char *str,
	INT32 flags,
	INT32 align
)
{
	size_t i = AllocateDrawItem(list);
	drawitem_t *item = &list->items[i];
	item->type = DI_DrawString;
	item->x = x;
	item->y = y;
	item->str = CopyString(list, str);
	item->flags = flags;
	item->align = align;
}

void LUA_HUD_AddDrawNameTag(
	huddrawlist_h list,
	INT32 x,
	INT32 y,
	const char *str,
	INT32 flags,
	UINT16 basecolor,
	UINT16 outlinecolor,
	UINT8 *basecolormap,
	UINT8 *outlinecolormap
)
{
	size_t i = AllocateDrawItem(list);
	drawitem_t *item = &list->items[i];
	item->type = DI_DrawNameTag;
	item->x = x;
	item->y = y;
	item->str = CopyString(list, str);
	item->flags = flags;
	item->basecolor = basecolor;
	item->outlinecolor = outlinecolor;
	item->basecolormap = basecolormap;
	item->outlinecolormap = outlinecolormap;
}

void LUA_HUD_AddDrawScaledNameTag(
	huddrawlist_h list,
	fixed_t x,
	fixed_t y,
	const char *str,
	INT32 flags,
	fixed_t scale,
	UINT16 basecolor,
	UINT16 outlinecolor,
	UINT8 *basecolormap,
	UINT8 *outlinecolormap
)
{
	size_t i = AllocateDrawItem(list);
	drawitem_t *item = &list->items[i];
	item->type = DI_DrawScaledNameTag;
	item->x = x;
	item->y = y;
	item->str = CopyString(list, str);
	item->flags = flags;
	item->scale = scale;
	item->basecolor = basecolor;
	item->outlinecolor = outlinecolor;
	item->basecolormap = basecolormap;
	item->outlinecolormap = outlinecolormap;
}

void LUA_HUD_AddDrawLevelTitle(
	huddrawlist_h list,
	INT32 x,
	INT32 y,
	const char *str,
	INT32 flags
)
{
	size_t i = AllocateDrawItem(list);
	drawitem_t *item = &list->items[i];
	item->type = DI_DrawLevelTitle;
	item->x = x;
	item->y = y;
	item->str = CopyString(list, str);
	item->flags = flags;
}

void LUA_HUD_AddFadeScreen(
	huddrawlist_h list,
	UINT16 color,
	UINT8 strength
)
{
	size_t i = AllocateDrawItem(list);
	drawitem_t *item = &list->items[i];
	item->type = DI_FadeScreen;
	item->color = color;
	item->strength = strength;
}

void LUA_HUD_DrawList(huddrawlist_h list)
{
	size_t i;

	if (!list) I_Error("HUD drawlist invalid");
	if (list->items_len <= 0) return;
	if (!list->items) I_Error("HUD drawlist->items invalid");

	for (i = 0; i < list->items_len; i++)
	{
		drawitem_t *item = &list->items[i];

		switch (item->type)
		{
			case DI_Draw:
				V_DrawFixedPatch(item->x<<FRACBITS, item->y<<FRACBITS, FRACUNIT, item->flags, item->patch, item->colormap);
				break;
			case DI_DrawScaled:
				V_DrawFixedPatch(item->x, item->y, item->scale, item->flags, item->patch, item->colormap);
				break;
			case DI_DrawStretched:
				V_DrawStretchyFixedPatch(item->x, item->y, item->hscale, item->vscale, item->flags, item->patch, item->colormap);
				break;
			case DI_DrawCropped:
				V_DrawCroppedPatch(item->x, item->y, item->hscale, item->vscale, item->flags, item->patch, item->colormap, item->sx, item->sy, item->w, item->h);
				break;
			case DI_DrawNum:
				V_DrawTallNum(item->x, item->y, item->flags, item->num);
				break;
			case DI_DrawPaddedNum:
				V_DrawPaddedTallNum(item->x, item->y, item->flags, item->num, item->digits);
				break;
			case DI_DrawFill:
				V_DrawFill(item->x, item->y, item->w, item->h, item->c);
				break;
			case DI_DrawString:
				switch(item->align)
				{
				// hu_font
				case align_left:
					V_DrawString(item->x, item->y, item->flags, item->str);
					break;
				case align_center:
					V_DrawCenteredString(item->x, item->y, item->flags, item->str);
					break;
				case align_right:
					V_DrawRightAlignedString(item->x, item->y, item->flags, item->str);
					break;
				case align_fixed:
					V_DrawStringAtFixed(item->x, item->y, item->flags, item->str);
					break;
				case align_fixedcenter:
					V_DrawCenteredStringAtFixed(item->x, item->y, item->flags, item->str);
					break;
				case align_fixedright:
					V_DrawRightAlignedStringAtFixed(item->x, item->y, item->flags, item->str);
					break;
				// hu_font, 0.5x scale
				case align_small:
					V_DrawSmallString(item->x, item->y, item->flags, item->str);
					break;
				case align_smallfixed:
					V_DrawSmallStringAtFixed(item->x, item->y, item->flags, item->str);
					break;
				case align_smallfixedcenter:
					V_DrawCenteredSmallStringAtFixed(item->x, item->y, item->flags, item->str);
					break;
				case align_smallfixedright:
					V_DrawRightAlignedSmallStringAtFixed(item->x, item->y, item->flags, item->str);
					break;
				case align_smallcenter:
					V_DrawCenteredSmallString(item->x, item->y, item->flags, item->str);
					break;
				case align_smallright:
					V_DrawRightAlignedSmallString(item->x, item->y, item->flags, item->str);
					break;
				case align_smallthin:
					V_DrawSmallThinString(item->x, item->y, item->flags, item->str);
					break;
				case align_smallthincenter:
					V_DrawCenteredSmallThinString(item->x, item->y, item->flags, item->str);
					break;
				case align_smallthinright:
					V_DrawRightAlignedSmallThinString(item->x, item->y, item->flags, item->str);
					break;
				case align_smallthinfixed:
					V_DrawSmallThinStringAtFixed(item->x, item->y, item->flags, item->str);
					break;
				case align_smallthinfixedcenter:
					V_DrawCenteredSmallThinStringAtFixed(item->x, item->y, item->flags, item->str);
					break;
				case align_smallthinfixedright:
					V_DrawRightAlignedSmallThinStringAtFixed(item->x, item->y, item->flags, item->str);
					break;
				// tny_font
				case align_thin:
					V_DrawThinString(item->x, item->y, item->flags, item->str);
					break;
				case align_thincenter:
					V_DrawCenteredThinString(item->x, item->y, item->flags, item->str);
					break;
				case align_thinright:
					V_DrawRightAlignedThinString(item->x, item->y, item->flags, item->str);
					break;
				case align_thinfixed:
					V_DrawThinStringAtFixed(item->x, item->y, item->flags, item->str);
					break;
				case align_thinfixedcenter:
					V_DrawCenteredThinStringAtFixed(item->x, item->y, item->flags, item->str);
					break;
				case align_thinfixedright:
					V_DrawRightAlignedThinStringAtFixed(item->x, item->y, item->flags, item->str);
					break;
				}
				break;
			case DI_DrawNameTag:
				V_DrawNameTag(item->x, item->y, item->flags, FRACUNIT, item->basecolormap, item->outlinecolormap, item->str);
				break;
			case DI_DrawScaledNameTag:
				V_DrawNameTag(FixedInt(item->x), FixedInt(item->y), item->flags, item->scale, item->basecolormap, item->outlinecolormap, item->str);
				break;
			case DI_DrawLevelTitle:
				V_DrawLevelTitle(item->x, item->y, item->flags, item->str);
				break;
			case DI_FadeScreen:
				V_DrawFadeScreen(item->color, item->strength);
				break;
			default:
				I_Error("can't draw draw list item: invalid draw list item type");
				continue;
		}
	}
}

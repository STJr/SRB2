// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2006 by Randy Heit.
// Copyright (C) 2023 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  r_translation.c
/// \brief Translation table handling

#include "r_translation.h"
#include "r_data.h"
#include "r_draw.h"
#include "v_video.h" // pMasterPalette
#include "z_zone.h"
#include "w_wad.h"
#include "m_tokenizer.h"
#include "m_misc.h"

#include <errno.h>

static remaptable_t **paletteremaps = NULL;
static unsigned numpaletteremaps = 0;

static int allWhiteRemap = 0;
static int dashModeRemap = 0;

static void MakeDashModeRemap(void);

static boolean PaletteRemap_AddIndexRange(remaptable_t *tr, int start, int end, int pal1, int pal2);
static boolean PaletteRemap_AddColorRange(remaptable_t *tr, int start, int end, int r1i, int g1i, int b1i, int r2i, int g2i, int b2i);
static boolean PaletteRemap_AddDesaturation(remaptable_t *tr, int start, int end, double r1, double g1, double b1, double r2, double g2, double b2);
static boolean PaletteRemap_AddColourisation(remaptable_t *tr, int start, int end, int r, int g, int b);
static boolean PaletteRemap_AddTint(remaptable_t *tr, int start, int end, int r, int g, int b, int amount);
static boolean PaletteRemap_AddInvert(remaptable_t *tr, int start, int end);

enum PaletteRemapType
{
	REMAP_ADD_INDEXRANGE,
	REMAP_ADD_COLORRANGE,
	REMAP_ADD_COLOURISATION,
	REMAP_ADD_DESATURATION,
	REMAP_ADD_TINT
};

struct PaletteRemapParseResult
{
	int start, end;
	enum PaletteRemapType type;
	union
	{
		struct
		{
			int pal1, pal2;
		} indexRange;
		struct
		{
			int r1, g1, b1;
			int r2, g2, b2;
		} colorRange;
		struct
		{
			double r1, g1, b1;
			double r2, g2, b2;
		} desaturation;
		struct
		{
			int r, g, b;
		} colourisation;
		struct
		{
			int r, g, b, amount;
		} tint;
	};

	char *error;
};

void PaletteRemap_Init(void)
{
	// First translation must be the identity one.
	remaptable_t *base = PaletteRemap_New();
	PaletteRemap_SetIdentity(base);
	PaletteRemap_Add(base);

	// Grayscale translation
	remaptable_t *grayscale = PaletteRemap_New();
	PaletteRemap_SetIdentity(grayscale);
	PaletteRemap_AddDesaturation(grayscale, 0, 255, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0);
	R_AddCustomTranslation("Grayscale", PaletteRemap_Add(grayscale));

	// All white (TC_ALLWHITE)
	remaptable_t *allWhite = PaletteRemap_New();
	memset(allWhite->remap, 0, NUM_PALETTE_ENTRIES * sizeof(UINT8));
	allWhiteRemap = PaletteRemap_Add(allWhite);
	R_AddCustomTranslation("AllWhite", allWhiteRemap);

	// All black
	remaptable_t *allBlack = PaletteRemap_New();
	memset(allBlack->remap, 31, NUM_PALETTE_ENTRIES * sizeof(UINT8));
	R_AddCustomTranslation("AllBlack", PaletteRemap_Add(allBlack));

	// Invert
	remaptable_t *invertRemap = PaletteRemap_New();
	PaletteRemap_SetIdentity(invertRemap);
	PaletteRemap_AddInvert(invertRemap, 0, 255);
	R_AddCustomTranslation("Invert", PaletteRemap_Add(invertRemap));

	// Dash mode (TC_DASHMODE)
	MakeDashModeRemap();
}

remaptable_t *PaletteRemap_New(void)
{
	remaptable_t *tr = Z_Calloc(sizeof(remaptable_t), PU_STATIC, NULL);
	tr->num_entries = NUM_PALETTE_ENTRIES;
	return tr;
}

remaptable_t *PaletteRemap_Copy(remaptable_t *tr)
{
	remaptable_t *copy = Z_Malloc(sizeof(remaptable_t), PU_STATIC, NULL);
	memcpy(copy, tr, sizeof(remaptable_t));
	return copy;
}

boolean PaletteRemap_Equal(remaptable_t *a, remaptable_t *b)
{
	if (a->num_entries != b->num_entries)
		return false;

	return memcmp(a->remap, b->remap, a->num_entries) == 0;
}

void PaletteRemap_SetIdentity(remaptable_t *tr)
{
	for (unsigned i = 0; i < tr->num_entries; i++)
	{
		tr->remap[i] = i;
	}
}

boolean PaletteRemap_IsIdentity(remaptable_t *tr)
{
	for (unsigned i = 0; i < NUM_PALETTE_ENTRIES; i++)
	{
		if (tr->remap[i] != i)
			return false;
	}

	return true;
}

unsigned PaletteRemap_Add(remaptable_t *tr)
{
	numpaletteremaps++;
	paletteremaps = Z_Realloc(paletteremaps, sizeof(remaptable_t *) * numpaletteremaps, PU_STATIC, NULL);
	paletteremaps[numpaletteremaps - 1] = tr;

	return numpaletteremaps - 1;
}

// This is a long one, because MotorRoach basically hand-picked the indices
static void MakeDashModeRemap(void)
{
	remaptable_t *dashmode = PaletteRemap_New();

	PaletteRemap_SetIdentity(dashmode);

	UINT8 *dest_colormap = dashmode->remap;

	// greens -> ketchups
	dest_colormap[96] = dest_colormap[97] = 48;
	dest_colormap[98] = 49;
	dest_colormap[99] = 51;
	dest_colormap[100] = 52;
	dest_colormap[101] = dest_colormap[102] = 54;
	dest_colormap[103] = 34;
	dest_colormap[104] = 37;
	dest_colormap[105] = 39;
	dest_colormap[106] = 41;
	for (unsigned i = 0; i < 5; i++)
		dest_colormap[107 + i] = 43 + i;

	// reds -> steel blues
	dest_colormap[32] = 146;
	dest_colormap[33] = 147;
	dest_colormap[34] = dest_colormap[35] = 170;
	dest_colormap[36] = 171;
	dest_colormap[37] = dest_colormap[38] = 172;
	dest_colormap[39] = dest_colormap[40] = dest_colormap[41] = 173;
	dest_colormap[42] = dest_colormap[43] = dest_colormap[44] = 174;
	dest_colormap[45] = dest_colormap[46] = dest_colormap[47] = 175;
	dest_colormap[71] = 139;

	// steel blues -> oranges
	dest_colormap[170] = 52;
	dest_colormap[171] = 54;
	dest_colormap[172] = 56;
	dest_colormap[173] = 42;
	dest_colormap[174] = 45;
	dest_colormap[175] = 47;

	dashModeRemap = PaletteRemap_Add(dashmode);

	R_AddCustomTranslation("DashMode", dashModeRemap);
}

static boolean PalIndexOutOfRange(int color)
{
	return color < 0 || color > 255;
}

static boolean IndicesOutOfRange(int start, int end)
{
	return PalIndexOutOfRange(start) || PalIndexOutOfRange(end);
}

static boolean IndicesOutOfRange2(int start1, int end1, int start2, int end2)
{
	return IndicesOutOfRange(start1, end1) || IndicesOutOfRange(start2, end2);
}

#define SWAP(a, b, t) { \
	t swap = a; \
	a = b; \
	b = swap; \
}

static boolean PaletteRemap_AddIndexRange(remaptable_t *tr, int start, int end, int pal1, int pal2)
{
	if (IndicesOutOfRange2(start, end, pal1, pal2))
		return false;

	if (start > end)
	{
		SWAP(start, end, int);
		SWAP(pal1, pal2, int);
	}
	else if (start == end)
	{
		tr->remap[start] = pal1;
		return true;
	}

	double palcol = pal1;
	double palstep = (pal2 - palcol) / (end - start);

	for (int i = start; i <= end; palcol += palstep, ++i)
	{
		double idx = round(palcol);
		tr->remap[i] = (int)idx;
	}

	return true;
}

static boolean PaletteRemap_AddColorRange(remaptable_t *tr, int start, int end, int r1i, int g1i, int b1i, int r2i, int g2i, int b2i)
{
	if (IndicesOutOfRange(start, end))
		return false;

	double r1 = r1i;
	double g1 = g1i;
	double b1 = b1i;
	double r2 = r2i;
	double g2 = g2i;
	double b2 = b2i;
	double r, g, b;
	double rs, gs, bs;

	if (start > end)
	{
		SWAP(start, end, int);

		r = r2;
		g = g2;
		b = b2;
		rs = r1 - r2;
		gs = g1 - g2;
		bs = b1 - b2;
	}
	else
	{
		r = r1;
		g = g1;
		b = b1;
		rs = r2 - r1;
		gs = g2 - g1;
		bs = b2 - b1;
	}

	if (start == end)
	{
		tr->remap[start] = NearestColor(r, g, b);
	}
	else
	{
		rs /= (end - start);
		gs /= (end - start);
		bs /= (end - start);

		for (int i = start; i <= end; ++i)
		{
			tr->remap[i] = NearestColor(r, g, b);
			r += rs;
			g += gs;
			b += bs;
		}
	}

	return true;
}

#define CLAMP(val, minval, maxval) max(min(val, maxval), minval)

static boolean PaletteRemap_AddDesaturation(remaptable_t *tr, int start, int end, double r1, double g1, double b1, double r2, double g2, double b2)
{
	if (IndicesOutOfRange(start, end))
		return false;

	r1 = CLAMP(r1, 0.0, 2.0);
	g1 = CLAMP(g1, 0.0, 2.0);
	b1 = CLAMP(b1, 0.0, 2.0);
	r2 = CLAMP(r2, 0.0, 2.0);
	g2 = CLAMP(g2, 0.0, 2.0);
	b2 = CLAMP(b2, 0.0, 2.0);

	if (start > end)
	{
		SWAP(start, end, int);
		SWAP(r1, r2, double);
		SWAP(g1, g2, double);
		SWAP(b1, b2, double);
	}

	r2 -= r1;
	g2 -= g1;
	b2 -= b1;
	r1 *= 255;
	g1 *= 255;
	b1 *= 255;

	for (int c = start; c <= end; c++)
	{
		double intensity = (pMasterPalette[c].s.red * 77 + pMasterPalette[c].s.green * 143 + pMasterPalette[c].s.blue * 37) / 255.0;

		tr->remap[c] = NearestColor(
		    min(255, max(0, (int)(r1 + intensity*r2))),
		    min(255, max(0, (int)(g1 + intensity*g2))),
		    min(255, max(0, (int)(b1 + intensity*b2)))
		);
	}

	return true;
}

#undef CLAMP

#undef SWAP

static boolean PaletteRemap_AddColourisation(remaptable_t *tr, int start, int end, int r, int g, int b)
{
	if (IndicesOutOfRange(start, end))
		return false;

	for (int i = start; i < end; ++i)
	{
		double br = pMasterPalette[i].s.red;
		double bg = pMasterPalette[i].s.green;
		double bb = pMasterPalette[i].s.blue;
		double grey = (br * 0.299 + bg * 0.587 + bb * 0.114) / 255.0f;
		if (grey > 1.0)
			grey = 1.0;

		br = r * grey;
		bg = g * grey;
		bb = b * grey;

		tr->remap[i] = NearestColor(
		    (int)br,
		    (int)bg,
		    (int)bb
		);
	}

	return true;
}

static boolean PaletteRemap_AddTint(remaptable_t *tr, int start, int end, int r, int g, int b, int amount)
{
	if (IndicesOutOfRange(start, end))
		return false;

	for (int i = start; i < end; ++i)
	{
		float br = pMasterPalette[i].s.red;
		float bg = pMasterPalette[i].s.green;
		float bb = pMasterPalette[i].s.blue;
		float a = amount * 0.01f;
		float ia = 1.0f - a;

		br = br * ia + r * a;
		bg = bg * ia + g * a;
		bb = bb * ia + b * a;

		tr->remap[i] = NearestColor(
		    (int)br,
		    (int)bg,
		    (int)bb
		);
	}

	return true;
}

static boolean PaletteRemap_AddInvert(remaptable_t *tr, int start, int end)
{
	if (IndicesOutOfRange(start, end))
		return false;

	for (int i = start; i < end; ++i)
	{
		tr->remap[i] = NearestColor(
		    255 - tr->remap[pMasterPalette[i].s.red],
		    255 - tr->remap[pMasterPalette[i].s.green],
		    255 - tr->remap[pMasterPalette[i].s.blue]
		);
	}

	return true;
}

struct ParsedTranslation
{
	struct ParsedTranslation *next;
	remaptable_t *remap;
	remaptable_t *baseTranslation;
	struct PaletteRemapParseResult *data;
};

static struct ParsedTranslation *parsedTranslationListHead = NULL;
static struct ParsedTranslation *parsedTranslationListTail = NULL;

static void AddParsedTranslation(remaptable_t *remap, remaptable_t *base_translation, struct PaletteRemapParseResult *data)
{
	struct ParsedTranslation *node = Z_Calloc(sizeof(struct ParsedTranslation), PU_STATIC, NULL);

	node->remap = remap;
	node->data = data;
	node->baseTranslation = base_translation;

	if (parsedTranslationListHead == NULL)
		parsedTranslationListHead = parsedTranslationListTail = node;
	else
	{
		parsedTranslationListTail->next = node;
		parsedTranslationListTail = node;
	}
}

static void PaletteRemap_ApplyResult(remaptable_t *tr, struct PaletteRemapParseResult *data)
{
	int start = data->start;
	int end = data->end;

	switch (data->type)
	{
	case REMAP_ADD_INDEXRANGE:
		PaletteRemap_AddIndexRange(tr, start, end, data->indexRange.pal1, data->indexRange.pal2);
		break;
	case REMAP_ADD_COLORRANGE:
		PaletteRemap_AddColorRange(tr, start, end,
			data->colorRange.r1, data->colorRange.g1, data->colorRange.b1,
			data->colorRange.r2, data->colorRange.g2, data->colorRange.b2);
		break;
	case REMAP_ADD_COLOURISATION:
		PaletteRemap_AddColourisation(tr, start, end,
			data->colourisation.r, data->colourisation.g, data->colourisation.b);
		break;
	case REMAP_ADD_DESATURATION:
		PaletteRemap_AddDesaturation(tr, start, end,
			data->desaturation.r1, data->desaturation.g1, data->desaturation.b1,
			data->desaturation.r2, data->desaturation.g2, data->desaturation.b2);
		break;
	case REMAP_ADD_TINT:
		PaletteRemap_AddTint(tr, start, end, data->tint.r, data->tint.g, data->tint.b, data->tint.amount);
		break;
	}
}

void R_LoadParsedTranslations(void)
{
	struct ParsedTranslation *node = parsedTranslationListHead;
	while (node)
	{
		struct PaletteRemapParseResult *result = node->data;
		struct ParsedTranslation *next = node->next;

		remaptable_t *tr = node->remap;
		if (tr)
		{
			if (tr->num_entries == 0)
			{
				tr->num_entries = NUM_PALETTE_ENTRIES;

				PaletteRemap_SetIdentity(tr);

				if (node->baseTranslation)
					memcpy(tr, node->baseTranslation, sizeof(remaptable_t));
			}

			PaletteRemap_ApplyResult(tr, result);
		}

		Z_Free(result);
		Z_Free(node);

		node = next;
	}

	parsedTranslationListHead = parsedTranslationListTail = NULL;
}

static boolean ExpectToken(tokenizer_t *sc, const char *expect)
{
	const char *tkn = sc->get(sc, 0);
	if (!tkn)
		return false;
	return strcmp(tkn, expect) == 0;
}

static boolean ParseNumber(tokenizer_t *sc, int *out)
{
	const char *tkn = sc->get(sc, 0);
	if (!tkn)
		return false;
	return M_StringToNumber(tkn, out);
}

static boolean ParseDecimal(tokenizer_t *sc, double *out)
{
	const char *tkn = sc->get(sc, 0);
	if (!tkn)
		return false;
	return M_StringToDecimal(tkn, out);
}

static struct PaletteRemapParseResult *ThrowError(const char *format, ...)
{
	const size_t err_size = 512 * sizeof(char);

	struct PaletteRemapParseResult *err = Z_Calloc(sizeof(struct PaletteRemapParseResult), PU_STATIC, NULL);

	err->error = Z_Calloc(err_size, PU_STATIC, NULL);

	va_list argptr;
	va_start(argptr, format);
	vsnprintf(err->error, err_size, format, argptr);
	va_end(argptr);

	return err;
}

static struct PaletteRemapParseResult *MakeResult(enum PaletteRemapType type, int start, int end)
{
	struct PaletteRemapParseResult *tr = Z_Calloc(sizeof(struct PaletteRemapParseResult), PU_STATIC, NULL);
	tr->type = type;
	tr->start = start;
	tr->end = end;
	return tr;
}

static struct PaletteRemapParseResult *PaletteRemap_ParseString(tokenizer_t *sc)
{
	int start, end;

	if (!ParseNumber(sc, &start))
		return ThrowError("expected a number for start range");
	if (!ExpectToken(sc, ":"))
		return ThrowError("expected ':'");
	if (!ParseNumber(sc, &end))
		return ThrowError("expected a number for end range");

	if (start < 0 || start > 255 || end < 0 || end > 255)
		return ThrowError("palette indices out of range");

	if (!ExpectToken(sc, "="))
		return ThrowError("expected '='");

	const char *tkn = sc->get(sc, 0);
	if (tkn == NULL)
		return ThrowError("unexpected EOF");

	if (strcmp(tkn, "[") == 0)
	{
		// translation using RGB values
		int r1, g1, b1;
		int r2, g2, b2;

		// start
		if (!ParseNumber(sc, &r1))
			return ThrowError("expected a number for starting red");
		if (!ExpectToken(sc, ","))
			return ThrowError("expected ','");

		if (!ParseNumber(sc, &g1))
			return ThrowError("expected a number for starting green");
		if (!ExpectToken(sc, ","))
			return ThrowError("expected ','");

		if (!ParseNumber(sc, &b1))
			return ThrowError("expected a number for starting blue");
		if (!ExpectToken(sc, ","))
			return ThrowError("expected ','");

		if (!ExpectToken(sc, "]"))
			return ThrowError("expected ']'");
		if (!ExpectToken(sc, ":"))
			return ThrowError("expected ':'");
		if (!ExpectToken(sc, "["))
			return ThrowError("expected '[");

		// end
		if (!ParseNumber(sc, &r2))
			return ThrowError("expected a number for ending red");
		if (!ExpectToken(sc, ","))
			return ThrowError("expected ','");

		if (!ParseNumber(sc, &g2))
			return ThrowError("expected a number for ending green");
		if (!ExpectToken(sc, ","))
			return ThrowError("expected ','");

		if (!ParseNumber(sc, &b2))
			return ThrowError("expected a number for ending blue");
		if (!ExpectToken(sc, "]"))
			return ThrowError("expected ']'");

		struct PaletteRemapParseResult *tr = MakeResult(REMAP_ADD_COLORRANGE, start, end);
		tr->colorRange.r1 = r1;
		tr->colorRange.g1 = g1;
		tr->colorRange.b1 = b1;
		tr->colorRange.r2 = r2;
		tr->colorRange.g2 = g2;
		tr->colorRange.b2 = b2;
		return tr;
	}
	else if (strcmp(tkn, "%") == 0)
	{
		// translation using RGB values (desaturation)
		double r1, g1, b1;
		double r2, g2, b2;

		if (!ExpectToken(sc, "["))
			return ThrowError("expected '[");

		// start
		if (!ParseDecimal(sc, &r1))
			return ThrowError("expected a number for starting red");
		if (!ExpectToken(sc, ","))
			return ThrowError("expected ','");

		if (!ParseDecimal(sc, &g1))
			return ThrowError("expected a number for starting green");
		if (!ExpectToken(sc, ","))
			return ThrowError("expected ','");

		if (!ParseDecimal(sc, &b1))
			return ThrowError("expected a number for starting blue");
		if (!ExpectToken(sc, "]"))
			return ThrowError("expected ']'");

		if (!ExpectToken(sc, ":"))
			return ThrowError("expected ':'");
		if (!ExpectToken(sc, "["))
			return ThrowError("expected '[");

		// end
		if (!ParseDecimal(sc, &r2))
			return ThrowError("expected a number for ending red");
		if (!ExpectToken(sc, ","))
			return ThrowError("expected ','");

		if (!ParseDecimal(sc, &g2))
			return ThrowError("expected a number for ending green");
		if (!ExpectToken(sc, ","))
			return ThrowError("expected ','");

		if (!ParseDecimal(sc, &b2))
			return ThrowError("expected a number for ending blue");
		if (!ExpectToken(sc, "]"))
			return ThrowError("expected ']'");

		struct PaletteRemapParseResult *tr = MakeResult(REMAP_ADD_DESATURATION, start, end);
		tr->desaturation.r1 = r1;
		tr->desaturation.g1 = g1;
		tr->desaturation.b1 = b1;
		tr->desaturation.r2 = r2;
		tr->desaturation.g2 = g2;
		tr->desaturation.b2 = b2;
		return tr;
	}
	else if (strcmp(tkn, "#") == 0)
	{
		// Colourise translation
		int r, g, b;

		if (!ExpectToken(sc, "["))
			return ThrowError("expected '[");
		if (!ParseNumber(sc, &r))
			return ThrowError("expected a number for red");
		if (!ExpectToken(sc, ","))
			return ThrowError("expected ','");
		if (!ParseNumber(sc, &g))
			return ThrowError("expected a number for green");
		if (!ExpectToken(sc, ","))
			return ThrowError("expected ','");
		if (!ParseNumber(sc, &b))
			return ThrowError("expected a number for blue");
		if (!ExpectToken(sc, "]"))
			return ThrowError("expected ']'");

		struct PaletteRemapParseResult *tr = MakeResult(REMAP_ADD_COLOURISATION, start, end);
		tr->colourisation.r = r;
		tr->colourisation.g = g;
		tr->colourisation.b = b;
		return tr;
	}
	else if (strcmp(tkn, "@") == 0)
	{
		// Tint translation
		int a, r, g, b;

		if (!ExpectToken(sc, "["))
			return ThrowError("expected '[");
		if (!ParseNumber(sc, &a))
			return ThrowError("expected a number for amount");
		if (!ExpectToken(sc, ","))
			return ThrowError("expected ','");
		if (!ParseNumber(sc, &r))
			return ThrowError("expected a number for red");
		if (!ExpectToken(sc, ","))
			return ThrowError("expected ','");
		if (!ParseNumber(sc, &g))
			return ThrowError("expected a number for green");
		if (!ExpectToken(sc, ","))
			return ThrowError("expected ','");
		if (!ParseNumber(sc, &b))
			return ThrowError("expected a number for blue");
		if (!ExpectToken(sc, "]"))
			return ThrowError("expected ']'");

		struct PaletteRemapParseResult *tr = MakeResult(REMAP_ADD_TINT, start, end);
		tr->tint.r = r;
		tr->tint.g = g;
		tr->tint.b = b;
		tr->tint.amount = a;
		return tr;
	}
	else
	{
		int pal1, pal2;

		if (!M_StringToNumber(tkn, &pal1))
			return ThrowError("expected a number for starting index");
		if (!ExpectToken(sc, ":"))
			return ThrowError("expected ':'");
		if (!ParseNumber(sc, &pal2))
			return ThrowError("expected a number for ending index");

		struct PaletteRemapParseResult *tr = MakeResult(REMAP_ADD_INDEXRANGE, start, end);
		tr->indexRange.pal1 = pal1;
		tr->indexRange.pal2 = pal2;
		return tr;
	}

	return NULL;
}

static struct PaletteRemapParseResult *PaletteRemap_ParseTranslation(const char *translation)
{
	tokenizer_t *sc = Tokenizer_Open(translation, 1);
	struct PaletteRemapParseResult *result = PaletteRemap_ParseString(sc);
	Tokenizer_Close(sc);
	return result;
}

static void PrintError(const char *name, const char *format, ...)
{
	char error[256];

	va_list argptr;
	va_start(argptr, format);
	vsnprintf(error, sizeof error, format, argptr);
	va_end(argptr);

	CONS_Alert(CONS_ERROR, "Error parsing translation '%s': %s\n", name, error);
}

#define CHECK_EOF() \
	if (!tkn) \
	{ \
		PrintError(name, "Unexpected EOF"); \
		goto fail; \
	}

struct NewTranslation
{
	int id;
	char *name;
	char *base_translation_name;
	struct PaletteRemapParseResult **results;
	size_t num_results;
};

static void AddNewTranslation(struct NewTranslation **list_p, size_t *num, char *name, int id, char *base_translation_name, struct PaletteRemapParseResult *parse_result)
{
	struct NewTranslation *list = *list_p;

	size_t count = *num;

	for (size_t i = 0; i < count; i++)
	{
		struct NewTranslation *entry = &list[i];
		if (entry->id == id && strcmp(entry->name, name) == 0)
		{
			if (entry->base_translation_name && base_translation_name
			&& strcmp(entry->base_translation_name, base_translation_name) != 0)
				continue;
			entry->num_results++;
			entry->results = Z_Realloc(entry->results,
				entry->num_results * sizeof(struct PaletteRemapParseResult **), PU_STATIC, NULL);
			entry->results[entry->num_results - 1] = parse_result;
			return;
		}
	}

	size_t i = count;

	count++;
	list = Z_Realloc(list, count * sizeof(struct NewTranslation), PU_STATIC, NULL);

	struct NewTranslation *entry = &list[i];
	entry->name = name;
	entry->id = id;
	entry->base_translation_name = base_translation_name;
	entry->num_results = 1;
	entry->results = Z_Realloc(entry->results, 1 * sizeof(struct PaletteRemapParseResult **), PU_STATIC, NULL);
	entry->results[0] = parse_result;

	*list_p = list;
	*num = count;
}

static void PrepareNewTranslations(struct NewTranslation *list, size_t count)
{
	if (!list)
		return;

	for (size_t i = 0; i < count; i++)
	{
		struct NewTranslation *entry = &list[i];

		remaptable_t *tr = R_GetTranslationByID(entry->id);
		if (tr == NULL)
		{
			tr = PaletteRemap_New();
			R_AddCustomTranslation(entry->name, PaletteRemap_Add(tr));
		}

		remaptable_t *base_translation = NULL;
		char *base_translation_name = entry->base_translation_name;
		if (base_translation_name)
		{
			int base_translation_id = R_FindCustomTranslation(base_translation_name);
			if (base_translation_id == -1)
				PrintError(entry->name, "No translation named '%s'", base_translation_name);
			else
				base_translation = R_GetTranslationByID(base_translation_id);
		}

		// The translation is not generated until later, because the palette may not have been loaded.
		// We store the result for when it's needed.
		for (size_t j = 0; j < entry->num_results; j++)
			AddParsedTranslation(tr, base_translation, entry->results[j]);

		tr->num_entries = 0;

		Z_Free(base_translation_name);
		Z_Free(entry->results);
		Z_Free(entry->name);
	}

	Z_Free(list);
}

void R_ParseTrnslate(INT32 wadNum, UINT16 lumpnum)
{
	tokenizer_t *sc = NULL;
	const char *tkn = NULL;
	char *lumpData = (char *)W_CacheLumpNumPwad(wadNum, lumpnum, PU_STATIC);
	size_t lumpLength = W_LumpLengthPwad(wadNum, lumpnum);
	char *text = (char *)Z_Malloc((lumpLength + 1), PU_STATIC, NULL);
	memmove(text, lumpData, lumpLength);
	text[lumpLength] = '\0';
	Z_Free(lumpData);

	sc = Tokenizer_Open(text, 1);
	tkn = sc->get(sc, 0);

	struct NewTranslation *list = NULL;
	size_t list_count = 0;

	while (tkn != NULL)
	{
		char *name = Z_StrDup(tkn);

		char *base_translation_name = NULL;

		tkn = sc->get(sc, 0);
		CHECK_EOF();
		if (strcmp(tkn, ":") == 0)
		{
			tkn = sc->get(sc, 0);
			CHECK_EOF();

			base_translation_name = Z_StrDup(tkn);

			tkn = sc->get(sc, 0);
			CHECK_EOF();
		}

		if (strcmp(tkn, "=") != 0)
		{
			PrintError(name, "Expected '=', got '%s'", tkn);
			goto fail;
		}
		tkn = sc->get(sc, 0);
		CHECK_EOF();

		if (strcmp(tkn, "\"") != 0)
		{
			PrintError(name, "Expected '\"', got '%s'", tkn);
			goto fail;
		}
		tkn = sc->get(sc, 0);
		CHECK_EOF();

		int existing_id = R_FindCustomTranslation(name);

		// Parse all of the translations
		do {
			struct PaletteRemapParseResult *parse_result = PaletteRemap_ParseTranslation(tkn);
			if (parse_result->error)
			{
				PrintError(name, "%s", parse_result->error);
				Z_Free(parse_result->error);
				goto fail;
			}
			else
			{
				AddNewTranslation(&list, &list_count, name, existing_id, base_translation_name, parse_result);
			}

			tkn = sc->get(sc, 0);
			if (!tkn || strcmp(tkn, "\"") != 0)
			{
				if (tkn)
					PrintError(name, "Expected '\"', got '%s'", tkn);
				else
					PrintError(name, "Expected '\"', got EOF");
				goto fail;
			}

			// Get ',' or parse the next line
			tkn = sc->get(sc, 0);
			if (!tkn || strcmp(tkn, ",") != 0)
				break;

			// Get '"'
			tkn = sc->get(sc, 0);
			if (!tkn || strcmp(tkn, "\"") != 0)
			{
				if (!tkn)
					PrintError(name, "Expected '\"', got EOF");
				else
					PrintError(name, "Expected '\"', got '%s'", tkn);
				goto fail;
			}
			tkn = sc->get(sc, 0);
			CHECK_EOF();
		} while (true);
	}

fail:
	// Now add all of the new translations
	if (list)
		PrepareNewTranslations(list, list_count);

	Tokenizer_Close(sc);

	Z_Free(text);
}

#undef CHECK_EOF

typedef struct CustomTranslation
{
	char *name;
	unsigned id;
	UINT32 hash;
} customtranslation_t;

static customtranslation_t *customtranslations = NULL;
static unsigned numcustomtranslations = 0;

int R_FindCustomTranslation(const char *name)
{
	UINT32 hash = quickncasehash(name, strlen(name));

	for (unsigned i = 0; i < numcustomtranslations; i++)
	{
		if (hash == customtranslations[i].hash && strcmp(name, customtranslations[i].name) == 0)
			return (int)customtranslations[i].id;
	}

	return -1;
}

// This is needed for SOC (which is case insensitive)
int R_FindCustomTranslation_CaseInsensitive(const char *name)
{
	for (unsigned i = 0; i < numcustomtranslations; i++)
	{
		if (stricmp(name, customtranslations[i].name) == 0)
			return (int)customtranslations[i].id;
	}

	return -1;
}

void R_AddCustomTranslation(const char *name, int trnum)
{
	customtranslation_t *tr = NULL;
	UINT32 hash = quickncasehash(name, strlen(name));

	for (unsigned i = 0; i < numcustomtranslations; i++)
	{
		customtranslation_t *lookup = &customtranslations[i];
		if (hash == lookup->hash && strcmp(name, lookup->name) == 0)
		{
			tr = lookup;
			break;
		}
	}

	if (tr == NULL)
	{
		numcustomtranslations++;
		customtranslations = Z_Realloc(customtranslations, sizeof(customtranslation_t) * numcustomtranslations, PU_STATIC, NULL);
		tr = &customtranslations[numcustomtranslations - 1];
	}

	tr->id = trnum;
	tr->name = Z_StrDup(name);
	tr->hash = quickncasehash(name, strlen(name));
}

const char *R_GetCustomTranslationName(unsigned id)
{
	for (unsigned i = 0; i < numcustomtranslations; i++)
	{
		if (id == customtranslations[i].id)
			return customtranslations[i].name;
	}

	return NULL;
}

unsigned R_NumCustomTranslations(void)
{
	return numcustomtranslations;
}

remaptable_t *R_GetTranslationByID(int id)
{
	if (!R_TranslationIsValid(id))
		return NULL;

	return paletteremaps[id];
}

boolean R_TranslationIsValid(int id)
{
	if (id < 0 || id >= (signed)numpaletteremaps)
		return false;

	return true;
}

remaptable_t *R_GetBuiltInTranslation(SINT8 tc)
{
	switch (tc)
	{
	case TC_ALLWHITE:
		return R_GetTranslationByID(allWhiteRemap);
	case TC_DASHMODE:
		return R_GetTranslationByID(dashModeRemap);
	}
	return NULL;
}

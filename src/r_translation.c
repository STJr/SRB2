// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2023 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  r_translation.c
/// \brief Translations

#include "r_translation.h"
#include "r_data.h"
#include "v_video.h" // pMasterPalette
#include "z_zone.h"
#include "w_wad.h"

#include <errno.h>

remaptable_t **paletteremaps = NULL;
unsigned numpaletteremaps = 0;

void PaletteRemap_Init(void)
{
	remaptable_t *base = PaletteRemap_New();
	PaletteRemap_SetIdentity(base);
	PaletteRemap_Add(base);
}

remaptable_t *PaletteRemap_New(void)
{
	remaptable_t *tr = Z_Calloc(sizeof(remaptable_t), PU_STATIC, NULL);
	tr->num_entries = 256;
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
	for (unsigned i = 0; i < 256; i++)
	{
		if (tr->remap[i] != i)
			return false;
	}

	return true;
}

unsigned PaletteRemap_Add(remaptable_t *tr)
{
#if 0
	for (unsigned i = 0; i < numpaletteremaps; i++)
	{
		if (PaletteRemap_Equal(tr, paletteremaps[i]))
			return i;
	}
#endif

	numpaletteremaps++;
	paletteremaps = Z_Realloc(paletteremaps, sizeof(remaptable_t *) * numpaletteremaps, PU_STATIC, NULL);
	paletteremaps[numpaletteremaps - 1] = tr;

	return numpaletteremaps - 1;
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

boolean PaletteRemap_AddIndexRange(remaptable_t *tr, int start, int end, int pal1, int pal2)
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

boolean PaletteRemap_AddColorRange(remaptable_t *tr, int start, int end, int _r1,int _g1, int _b1, int _r2, int _g2, int _b2)
{
	if (IndicesOutOfRange(start, end))
		return false;

	double r1 = _r1;
	double g1 = _g1;
	double b1 = _b1;
	double r2 = _r2;
	double g2 = _g2;
	double b2 = _b2;
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

#define clamp(val, minval, maxval) max(min(val, maxval), minval)

boolean PaletteRemap_AddDesaturation(remaptable_t *tr, int start, int end, double r1, double g1, double b1, double r2, double g2, double b2)
{
	if (IndicesOutOfRange(start, end))
		return false;

	r1 = clamp(r1, 0.0, 2.0);
	g1 = clamp(g1, 0.0, 2.0);
	b1 = clamp(b1, 0.0, 2.0);
	r2 = clamp(r2, 0.0, 2.0);
	g2 = clamp(g2, 0.0, 2.0);
	b2 = clamp(b2, 0.0, 2.0);

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
		    min(255, (int)(r1 + intensity*r2)),
		    min(255, (int)(g1 + intensity*g2)),
		    min(255, (int)(b1 + intensity*b2))
		);
	}

	return true;
}

boolean PaletteRemap_AddColourisation(remaptable_t *tr, int start, int end, int r, int g, int b)
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

boolean PaletteRemap_AddTint(remaptable_t *tr, int start, int end, int r, int g, int b, int amount)
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

static boolean ExpectToken(const char *expect)
{
	return strcmp(M_TokenizerReadZDoom(0), expect) == 0;
}

static boolean StringToNumber(const char *tkn, int *out)
{
	char *endPos = NULL;

#ifndef AVOID_ERRNO
	errno = 0;
#endif

	int result = strtol(tkn, &endPos, 10);
	if (endPos == tkn || *endPos != '\0')
		return false;

#ifndef AVOID_ERRNO
	if (errno == ERANGE)
		return false;
#endif

	*out = result;

	return true;
}

static boolean ParseNumber(int *out)
{
	return StringToNumber(M_TokenizerReadZDoom(0), out);
}

static boolean ParseDecimal(double *out)
{
	const char *tkn = M_TokenizerReadZDoom(0);

	char *endPos = NULL;

#ifndef AVOID_ERRNO
	errno = 0;
#endif

	double result = strtod(tkn, &endPos);
	if (endPos == tkn || *endPos != '\0')
		return false;

#ifndef AVOID_ERRNO
	if (errno == ERANGE)
		return false;
#endif

	*out = result;

	return true;
}

static struct PaletteRemapParseResult *ThrowError(const char *format, ...)
{
	struct PaletteRemapParseResult *err = Z_Calloc(sizeof(struct PaletteRemapParseResult), PU_STATIC, NULL);

	va_list argptr;
	va_start(argptr, format);
	vsprintf(err->error, format, argptr);
	va_end(argptr);

	return err;
}

struct PaletteRemapParseResult *PaletteRemap_ParseString(remaptable_t *tr, char *translation)
{
	int start, end;

	M_TokenizerOpen(translation);

	if (!ParseNumber(&start))
		return ThrowError("expected a number for start range");
	if (!ExpectToken(":"))
		return ThrowError("expected ':'");
	if (!ParseNumber(&end))
		return ThrowError("expected a number for end range");

	if (start < 0 || start > 255 || end < 0 || end > 255)
		return ThrowError("palette indices out of range");

	if (!ExpectToken("="))
		return ThrowError("expected '='");

	const char *tkn = M_TokenizerReadZDoom(0);
	if (strcmp(tkn, "[") == 0)
	{
		// translation using RGB values
		int r1, g1, b1;
		int r2, g2, b2;

		// start
		if (!ParseNumber(&r1))
			return ThrowError("expected a number for starting red");
		if (!ExpectToken(","))
			return ThrowError("expected ','");

		if (!ParseNumber(&g1))
			return ThrowError("expected a number for starting green");
		if (!ExpectToken(","))
			return ThrowError("expected ','");

		if (!ParseNumber(&b1))
			return ThrowError("expected a number for starting blue");
		if (!ExpectToken(","))
			return ThrowError("expected ','");

		if (!ExpectToken("]"))
			return ThrowError("expected ']'");
		if (!ExpectToken(":"))
			return ThrowError("expected ':'");
		if (!ExpectToken("["))
			return ThrowError("expected '[");

		// end
		if (!ParseNumber(&r2))
			return ThrowError("expected a number for ending red");
		if (!ExpectToken(","))
			return ThrowError("expected ','");

		if (!ParseNumber(&g2))
			return ThrowError("expected a number for ending green");
		if (!ExpectToken(","))
			return ThrowError("expected ','");

		if (!ParseNumber(&b2))
			return ThrowError("expected a number for ending blue");
		if (!ExpectToken("]"))
			return ThrowError("expected ']'");

		PaletteRemap_AddColorRange(tr, start, end, r1, g1, b1, r2, g2, b2);
	}
	else if (strcmp(tkn, "%") == 0)
	{
		// translation using RGB values (desaturation)
		double r1, g1, b1;
		double r2, g2, b2;

		if (!ExpectToken("["))
			return ThrowError("expected '[");

		// start
		if (!ParseDecimal(&r1))
			return ThrowError("expected a number for starting red");
		if (!ExpectToken(","))
			return ThrowError("expected ','");

		if (!ParseDecimal(&g1))
			return ThrowError("expected a number for starting green");
		if (!ExpectToken(","))
			return ThrowError("expected ','");

		if (!ParseDecimal(&b1))
			return ThrowError("expected a number for starting blue");
		if (!ExpectToken("]"))
			return ThrowError("expected ']'");

		if (!ExpectToken(":"))
			return ThrowError("expected ':'");

		if (!ExpectToken("["))
			return ThrowError("expected '[");

		// end
		if (!ParseDecimal(&r2))
			return ThrowError("expected a number for ending red");
		if (!ExpectToken(","))
			return ThrowError("expected ','");

		if (!ParseDecimal(&g2))
			return ThrowError("expected a number for ending green");
		if (!ExpectToken(","))
			return ThrowError("expected ','");

		if (!ParseDecimal(&b2))
			return ThrowError("expected a number for ending blue");
		if (!ExpectToken("]"))
			return ThrowError("expected ']'");

		PaletteRemap_AddDesaturation(tr, start, end, r1, g1, b1, r2, g2, b2);
	}
	else if (strcmp(tkn, "#") == 0)
	{
		// Colourise translation
		int r, g, b;

		if (!ExpectToken("["))
			return ThrowError("expected '[");
		if (!ParseNumber(&r))
			return ThrowError("expected a number for red");
		if (!ExpectToken(","))
			return ThrowError("expected ','");
		if (!ParseNumber(&g))
			return ThrowError("expected a number for green");
		if (!ExpectToken(","))
			return ThrowError("expected ','");
		if (!ParseNumber(&b))
			return ThrowError("expected a number for blue");
		if (!ExpectToken("]"))
			return ThrowError("expected ']'");

		PaletteRemap_AddColourisation(tr, start, end, r, g, b);
	}
	else if (strcmp(tkn, "@") == 0)
	{
		// Tint translation
		int a, r, g, b;

		if (!ExpectToken("["))
			return ThrowError("expected '[");
		if (!ParseNumber(&a))
			return ThrowError("expected a number for amount");
		if (!ExpectToken(","))
			return ThrowError("expected ','");
		if (!ParseNumber(&r))
			return ThrowError("expected a number for red");
		if (!ExpectToken(","))
			return ThrowError("expected ','");
		if (!ParseNumber(&g))
			return ThrowError("expected a number for green");
		if (!ExpectToken(","))
			return ThrowError("expected ','");
		if (!ParseNumber(&b))
			return ThrowError("expected a number for blue");
		if (!ExpectToken("]"))
			return ThrowError("expected ']'");

		PaletteRemap_AddTint(tr, start, end, r, g, b, a);
	}
	else
	{
		int pal1, pal2;

		if (!StringToNumber(tkn, &pal1))
			return ThrowError("expected a number for starting index");
		if (!ExpectToken(":"))
			return ThrowError("expected ':'");
		if (!ParseNumber(&pal2))
			return ThrowError("expected a number for ending index");

		PaletteRemap_AddIndexRange(tr, start, end, pal1, pal2);
	}

	M_TokenizerClose();

	return NULL;
}

static void P_ParseTrnslate(INT32 wadNum, UINT16 lumpnum)
{
	char *lumpData = (char *)W_CacheLumpNumPwad(wadNum, lumpnum, PU_STATIC);
	size_t lumpLength = W_LumpLengthPwad(wadNum, lumpnum);
	char *text = (char *)Z_Malloc((lumpLength + 1), PU_STATIC, NULL);
	memmove(text, lumpData, lumpLength);
	text[lumpLength] = '\0';
	Z_Free(lumpData);

	char *p = text;
	char *tkn = M_GetToken(p);
	while (tkn != NULL)
	{
		remaptable_t *tr = NULL;

		char *name = tkn;

		tkn = M_GetToken(NULL);
		if (strcmp(tkn, ":") == 0)
		{
			Z_Free(tkn);
			tkn = M_GetToken(NULL);

			remaptable_t *tbl = R_GetTranslationByID(R_FindCustomTranslation(tkn));
			if (tbl)
				tr = PaletteRemap_Copy(tbl);
			else
			{
				CONS_Alert(CONS_ERROR, "Error parsing translation '%s': No translation named '%s'\n", name, tkn);
				goto fail;
			}

			Z_Free(tkn);
			tkn = M_GetToken(NULL);
		}
		else
		{
			tr = PaletteRemap_New();
			PaletteRemap_SetIdentity(tr);
		}

#if 0
		tkn = M_GetToken(NULL);
		if (strcmp(tkn, "=") != 0)
		{
			CONS_Alert(CONS_ERROR, "Error parsing translation '%s': Expected '=', got '%s'\n", name, tkn);
			goto fail;
		}
		Z_Free(tkn);
#endif

		do {
			struct PaletteRemapParseResult *error = PaletteRemap_ParseString(tr, tkn);
			if (error)
			{
				CONS_Alert(CONS_ERROR, "Error parsing translation '%s': %s\n", name, error->error);
				Z_Free(error);
				goto fail;
			}

			Z_Free(tkn);
			tkn = M_GetToken(NULL);
			if (!tkn)
				break;

			if (strcmp(tkn, ",") != 0)
				break;

			Z_Free(tkn);
			tkn = M_GetToken(NULL);
		} while (true);

		// add it
		unsigned id = PaletteRemap_Add(tr);
		R_AddCustomTranslation(name, id);
		Z_Free(name);
	}

fail:
	Z_Free(tkn);
	Z_Free((void *)text);
}

void R_LoadTrnslateLumps(void)
{
	for (INT32 w = numwadfiles-1; w >= 0; w--)
	{
		UINT16 lump = W_CheckNumForNamePwad("TRNSLATE", w, 0);

		while (lump != INT16_MAX)
		{
			P_ParseTrnslate(w, lump);
			lump = W_CheckNumForNamePwad("TRNSLATE", (UINT16)w, lump + 1);
		}
	}
}

struct CustomTranslation
{
	char *name;
	unsigned id;
	UINT32 hash;
};

static struct CustomTranslation *customtranslations = NULL;
static unsigned numcustomtranslations = 0;

int R_FindCustomTranslation(const char *name)
{
	UINT32 hash = quickncasehash(name, strlen(name));

	for (unsigned i = 0; i < numcustomtranslations; i++)
	{
		if (hash == customtranslations[i].hash
		&& strcmp(name, customtranslations[i].name) == 0)
			return (int)customtranslations[i].id;
	}

	return -1;
}

void R_AddCustomTranslation(const char *name, int trnum)
{
	struct CustomTranslation *tr = NULL;
	UINT32 hash = quickncasehash(name, strlen(name));

	for (unsigned i = 0; i < numcustomtranslations; i++)
	{
		tr = &customtranslations[i];
		if (hash == tr->hash
		&& strcmp(name, tr->name) == 0)
		{
			break;
		}
	}

	if (tr == NULL)
	{
		numcustomtranslations++;
		customtranslations = Z_Realloc(customtranslations, sizeof(struct CustomTranslation) * numcustomtranslations, PU_STATIC, NULL);
		tr = &customtranslations[numcustomtranslations - 1];
	}

	tr->id = trnum;
	tr->name = Z_StrDup(name);
	tr->hash = quickncasehash(name, strlen(name));
}

remaptable_t *R_GetTranslationByID(int id)
{
	if (id < 0 || id >= (signed)numpaletteremaps)
		return NULL;

	return paletteremaps[id];
}

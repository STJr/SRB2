// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2013-2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  m_tokenizer.h
/// \brief Tokenizer

#ifndef __M_TOKENIZER__
#define __M_TOKENIZER__

#include "doomdef.h"

typedef struct Tokenizer
{
	char *zdup;
	const char *input;
	unsigned numTokens;
	UINT32 *capacity;
	char **token;
	UINT32 startPos;
	UINT32 endPos;
	UINT32 inputLength;
	UINT8 inComment; // 0 = not in comment, 1 = // Single-line, 2 = /* Multi-line */
	UINT8 inString; // 0 = not in string, 1 = in string, 2 = just left string
	int line;
	const char *(*get)(struct Tokenizer*, UINT32);
} tokenizer_t;

tokenizer_t *Tokenizer_Open(const char *inputString, size_t len, unsigned numTokens);
void Tokenizer_Close(tokenizer_t *tokenizer);

const char *Tokenizer_Read(tokenizer_t *tokenizer, UINT32 i);
const char *Tokenizer_SRB2Read(tokenizer_t *tokenizer, UINT32 i);
UINT32 Tokenizer_GetEndPos(tokenizer_t *tokenizer);
void Tokenizer_SetEndPos(tokenizer_t *tokenizer, UINT32 newPos);

#endif

// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2013-2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  m_tokenizer.c
/// \brief Tokenizer

#include "m_tokenizer.h"
#include "z_zone.h"

tokenizer_t *Tokenizer_Open(const char *inputString, size_t len, unsigned numTokens)
{
	tokenizer_t *tokenizer = Z_Malloc(sizeof(tokenizer_t), PU_STATIC, NULL);
	const size_t lenpan = 2;

	tokenizer->zdup = malloc(len+lenpan);
	for (size_t i = 0; i < lenpan; i++)
	{
		tokenizer->zdup[len+i] = 0x00;
	}

	tokenizer->input = M_Memcpy(tokenizer->zdup, inputString, len);
	tokenizer->startPos = 0;
	tokenizer->endPos = 0;
	tokenizer->inputLength = 0;
	tokenizer->inComment = 0;
	tokenizer->inString = 0;
	tokenizer->get = Tokenizer_Read;

	if (numTokens < 1)
		numTokens = 1;

	tokenizer->numTokens = numTokens;
	tokenizer->capacity = Z_Malloc(sizeof(UINT32) * numTokens, PU_STATIC, NULL);
	tokenizer->token = Z_Malloc(sizeof(char*) * numTokens, PU_STATIC, NULL);

	for (size_t i = 0; i < numTokens; i++)
	{
		tokenizer->capacity[i] = 1024;
		tokenizer->token[i] = (char*)Z_Malloc(tokenizer->capacity[i] * sizeof(char), PU_STATIC, NULL);
	}

	tokenizer->inputLength = strlen(tokenizer->input);

	return tokenizer;
}

void Tokenizer_Close(tokenizer_t *tokenizer)
{
	if (!tokenizer)
		return;

	for (size_t i = 0; i < tokenizer->numTokens; i++)
		Z_Free(tokenizer->token[i]);
	Z_Free(tokenizer->capacity);
	Z_Free(tokenizer->token);
	free(tokenizer->zdup);
	Z_Free(tokenizer);
}

static boolean DetectLineBreak(tokenizer_t *tokenizer, size_t pos)
{
	if (tokenizer->input[pos] == '\n')
	{
		tokenizer->line++;
		return true;
	}

	return false;
}

static void DetectComment(tokenizer_t *tokenizer, UINT32 *pos)
{
	if (tokenizer->inComment)
		return;

	if (*pos >= tokenizer->inputLength - 1)
		return;

	if (tokenizer->input[*pos] != '/')
		return;

	// Single-line comment start
	if (tokenizer->input[*pos + 1] == '/')
		tokenizer->inComment = 1;
	// Multi-line comment start
	else if (tokenizer->input[*pos + 1] == '*')
		tokenizer->inComment = 2;
}

static void Tokenizer_ReadTokenString(tokenizer_t *tokenizer, UINT32 i)
{
	UINT32 tokenLength = tokenizer->endPos - tokenizer->startPos;
	if (tokenLength + 1 > tokenizer->capacity[i])
	{
		tokenizer->capacity[i] = tokenLength + 1;
		// Assign the memory. Don't forget an extra byte for the end of the string!
		tokenizer->token[i] = (char *)Z_Malloc(tokenizer->capacity[i] * sizeof(char), PU_STATIC, NULL);
	}
	// Copy the string.
	M_Memcpy(tokenizer->token[i], tokenizer->input + tokenizer->startPos, (size_t)tokenLength);
	// Make the final character NUL.
	tokenizer->token[i][tokenLength] = '\0';
}

const char *Tokenizer_Read(tokenizer_t *tokenizer, UINT32 i)
{
	if (!tokenizer->input)
		return NULL;

	tokenizer->startPos = tokenizer->endPos;

	// If in a string, return the entire string within quotes, except without the quotes.
	if (tokenizer->inString == 1)
	{
		while (tokenizer->input[tokenizer->endPos] != '"' && tokenizer->endPos < tokenizer->inputLength)
		{
			DetectLineBreak(tokenizer, tokenizer->endPos);
			tokenizer->endPos++;
		}

		Tokenizer_ReadTokenString(tokenizer, i);
		tokenizer->inString = 2;
		return tokenizer->token[i];
	}
	// If just ended a string, return only a quotation mark.
	else if (tokenizer->inString == 2)
	{
		tokenizer->endPos = tokenizer->startPos + 1;
		tokenizer->token[i][0] = tokenizer->input[tokenizer->startPos];
		tokenizer->token[i][1] = '\0';
		tokenizer->inString = 0;
		return tokenizer->token[i];
	}

	// Try to detect comments now, in case we're pointing right at one
	DetectComment(tokenizer, &tokenizer->startPos);

	// Find the first non-whitespace char, or else the end of the string trying
	while ((tokenizer->input[tokenizer->startPos] == ' '
			|| tokenizer->input[tokenizer->startPos] == '\t'
			|| tokenizer->input[tokenizer->startPos] == '\r'
			|| tokenizer->input[tokenizer->startPos] == '\n'
			|| tokenizer->input[tokenizer->startPos] == '\0'
			|| tokenizer->inComment != 0)
			&& tokenizer->startPos < tokenizer->inputLength)
	{
		boolean inLineBreak = DetectLineBreak(tokenizer, tokenizer->startPos);

		// Try to detect comment endings now
		if (tokenizer->inComment == 1 && inLineBreak)
			tokenizer->inComment = 0; // End of line for a single-line comment
		else if (tokenizer->inComment == 2
			&& tokenizer->startPos < tokenizer->inputLength - 1
			&& tokenizer->input[tokenizer->startPos] == '*'
			&& tokenizer->input[tokenizer->startPos+1] == '/')
		{
			// End of multi-line comment
			tokenizer->inComment = 0;
			tokenizer->startPos++; // Make damn well sure we're out of the comment ending at the end of it all
		}

		tokenizer->startPos++;
		DetectComment(tokenizer, &tokenizer->startPos);
	}

	// If the end of the string is reached, no token is to be read
	if (tokenizer->startPos == tokenizer->inputLength)
	{
		tokenizer->endPos = tokenizer->inputLength;
		return NULL;
	}
	// Else, if it's one of these three symbols, capture only this one character
	else if (tokenizer->input[tokenizer->startPos] == ','
			|| tokenizer->input[tokenizer->startPos] == '{'
			|| tokenizer->input[tokenizer->startPos] == '}'
			|| tokenizer->input[tokenizer->startPos] == '['
			|| tokenizer->input[tokenizer->startPos] == ']'
			|| tokenizer->input[tokenizer->startPos] == '='
			|| tokenizer->input[tokenizer->startPos] == ':'
			|| tokenizer->input[tokenizer->startPos] == '%'
			|| tokenizer->input[tokenizer->startPos] == '@'
			|| tokenizer->input[tokenizer->startPos] == '"')
	{
		tokenizer->endPos = tokenizer->startPos + 1;
		tokenizer->token[i][0] = tokenizer->input[tokenizer->startPos];
		tokenizer->token[i][1] = '\0';
		if (tokenizer->input[tokenizer->startPos] == '"')
			tokenizer->inString = 1;

		return tokenizer->token[i];
	}

	// Now find the end of the token. This includes several additional characters that are okay to capture as one character, but not trailing at the end of another token.
	tokenizer->endPos = tokenizer->startPos + 1;
	while ((tokenizer->input[tokenizer->endPos] != ' '
			&& tokenizer->input[tokenizer->endPos] != '\t'
			&& tokenizer->input[tokenizer->endPos] != '\r'
			&& tokenizer->input[tokenizer->endPos] != '\n'
			&& tokenizer->input[tokenizer->endPos] != ','
			&& tokenizer->input[tokenizer->endPos] != '{'
			&& tokenizer->input[tokenizer->endPos] != '}'
			&& tokenizer->input[tokenizer->endPos] != '['
			&& tokenizer->input[tokenizer->endPos] != ']'
			&& tokenizer->input[tokenizer->endPos] != '='
			&& tokenizer->input[tokenizer->endPos] != ':'
			&& tokenizer->input[tokenizer->endPos] != '%'
			&& tokenizer->input[tokenizer->endPos] != '@'
			&& tokenizer->input[tokenizer->endPos] != ';'
			&& tokenizer->inComment == 0)
			&& tokenizer->endPos < tokenizer->inputLength)
	{
		tokenizer->endPos++;
		// Try to detect comment starts now; if it's in a comment, we don't want it in this token
		DetectComment(tokenizer, &tokenizer->endPos);
	}

	Tokenizer_ReadTokenString(tokenizer, i);
	return tokenizer->token[i];
}

const char *Tokenizer_SRB2Read(tokenizer_t *tokenizer, UINT32 i)
{
	if (!tokenizer->input)
		return NULL;

	tokenizer->startPos = tokenizer->endPos;

	// Try to detect comments now, in case we're pointing right at one
	DetectComment(tokenizer, &tokenizer->startPos);

	// Find the first non-whitespace char, or else the end of the string trying
	while ((tokenizer->input[tokenizer->startPos] == ' '
			|| tokenizer->input[tokenizer->startPos] == '\t'
			|| tokenizer->input[tokenizer->startPos] == '\r'
			|| tokenizer->input[tokenizer->startPos] == '\n'
			|| tokenizer->input[tokenizer->startPos] == '\0'
			|| tokenizer->input[tokenizer->startPos] == '=' || tokenizer->input[tokenizer->startPos] == ';' // UDMF TEXTMAP.
			|| tokenizer->inComment != 0)
			&& tokenizer->startPos < tokenizer->inputLength)
	{
		boolean inLineBreak = DetectLineBreak(tokenizer, tokenizer->startPos);

		// Try to detect comment endings now
		if (tokenizer->inComment == 1 && inLineBreak)
			tokenizer->inComment = 0; // End of line for a single-line comment
		else if (tokenizer->inComment == 2
			&& tokenizer->startPos < tokenizer->inputLength - 1
			&& tokenizer->input[tokenizer->startPos] == '*'
			&& tokenizer->input[tokenizer->startPos+1] == '/')
		{
			// End of multi-line comment
			tokenizer->inComment = 0;
			tokenizer->startPos++; // Make damn well sure we're out of the comment ending at the end of it all
		}

		tokenizer->startPos++;
		DetectComment(tokenizer, &tokenizer->startPos);
	}

	// If the end of the string is reached, no token is to be read
	if (tokenizer->startPos == tokenizer->inputLength) {
		tokenizer->endPos = tokenizer->inputLength;
		return NULL;
	}
	// Else, if it's one of these three symbols, capture only this one character
	else if (tokenizer->input[tokenizer->startPos] == ','
			|| tokenizer->input[tokenizer->startPos] == '{'
			|| tokenizer->input[tokenizer->startPos] == '}')
	{
		tokenizer->endPos = tokenizer->startPos + 1;
		tokenizer->token[i][0] = tokenizer->input[tokenizer->startPos];
		tokenizer->token[i][1] = '\0';
		return tokenizer->token[i];
	}
	// Return entire string within quotes, except without the quotes.
	else if (tokenizer->input[tokenizer->startPos] == '"')
	{
		tokenizer->endPos = ++tokenizer->startPos;
		while (tokenizer->input[tokenizer->endPos] != '"' && tokenizer->endPos < tokenizer->inputLength)
		{
			DetectLineBreak(tokenizer, tokenizer->endPos);
			tokenizer->endPos++;
		}

		Tokenizer_ReadTokenString(tokenizer, i);
		tokenizer->endPos++;
		return tokenizer->token[i];
	}

	// Now find the end of the token. This includes several additional characters that are okay to capture as one character, but not trailing at the end of another token.
	tokenizer->endPos = tokenizer->startPos + 1;
	while ((tokenizer->input[tokenizer->endPos] != ' '
			&& tokenizer->input[tokenizer->endPos] != '\t'
			&& tokenizer->input[tokenizer->endPos] != '\r'
			&& tokenizer->input[tokenizer->endPos] != '\n'
			&& tokenizer->input[tokenizer->endPos] != ','
			&& tokenizer->input[tokenizer->endPos] != '{'
			&& tokenizer->input[tokenizer->endPos] != '}'
			&& tokenizer->input[tokenizer->endPos] != '=' && tokenizer->input[tokenizer->endPos] != ';' // UDMF TEXTMAP.
			&& tokenizer->inComment == 0)
			&& tokenizer->endPos < tokenizer->inputLength)
	{
		tokenizer->endPos++;
		// Try to detect comment starts now; if it's in a comment, we don't want it in this token
		DetectComment(tokenizer, &tokenizer->endPos);
	}

	Tokenizer_ReadTokenString(tokenizer, i);
	return tokenizer->token[i];
}

UINT32 Tokenizer_GetEndPos(tokenizer_t *tokenizer)
{
	return tokenizer->endPos;
}

void Tokenizer_SetEndPos(tokenizer_t *tokenizer, UINT32 newPos)
{
	tokenizer->endPos = newPos;
}

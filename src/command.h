// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  command.h
/// \brief Deals with commands from console input, scripts, and remote server

#ifndef __COMMAND_H__
#define __COMMAND_H__

#include <stdio.h>
#include "doomdef.h"

//===================================
// Command buffer & command execution
//===================================

/* Lua command registration flags. */
enum
{
	COM_ADMIN       = 1,
	COM_SPLITSCREEN = 2,
	COM_LOCAL       = 4,
};

/* Command buffer flags. */
enum
{
	COM_SAFE = 1,
};

typedef void (*com_func_t)(void);

void COM_AddCommand(const char *name, com_func_t func);
int COM_AddLuaCommand(const char *name);

size_t COM_Argc(void);
const char *COM_Argv(size_t arg); // if argv > argc, returns empty string
char *COM_Args(void);
size_t COM_CheckParm(const char *check); // like M_CheckParm :)
size_t COM_CheckPartialParm(const char *check);
size_t COM_FirstOption(void);

// match existing command or NULL
const char *COM_CompleteCommand(const char *partial, INT32 skips);

const char *COM_CompleteAlias(const char *partial, INT32 skips);

// insert at queu (at end of other command)
#define COM_BufAddText(s) COM_BufAddTextEx(s, 0)
void COM_BufAddTextEx(const char *btext, int flags);

// insert in head (before other command)
#define COM_BufInsertText(s) COM_BufInsertTextEx(s, 0)
void COM_BufInsertTextEx(const char *btext, int flags);

// don't bother inserting, just do immediately
void COM_ImmedExecute(const char *ptext);

// Execute commands in buffer, flush them
void COM_BufExecute(void);

// As above; and progress the wait timer.
void COM_BufTicker(void);

// setup command buffer, at game tartup
void COM_Init(void);

// ======================
// Variable sized buffers
// ======================

typedef struct vsbuf_s
{
	boolean allowoverflow; // if false, do a I_Error
	boolean overflowed; // set to true if the buffer size failed
	UINT8 *data;
	size_t maxsize;
	size_t cursize;
} vsbuf_t;

void VS_Alloc(vsbuf_t *buf, size_t initsize);
void VS_Free(vsbuf_t *buf);
void VS_Clear(vsbuf_t *buf);
void *VS_GetSpace(vsbuf_t *buf, size_t length);
void VS_Write(vsbuf_t *buf, const void *data, size_t length);
void VS_WriteEx(vsbuf_t *buf, const void *data, size_t length, int flags);
void VS_Print(vsbuf_t *buf, const char *data); // strcats onto the sizebuf

//==================
// Console variables
//==================
// console vars are variables that can be changed through code or console,
// at RUN TIME. They can also act as simplified commands, because a func-
// tion can be attached to a console var, which is called whenever the
// variable is modified (using flag CV_CALL).

// flags for console vars

typedef enum
{
	CV_SAVE = 1,   // save to config when quit game
	CV_CALL = 2,   // call function on change
	CV_NETVAR = 4, // send it when change (see logboris.txt at 12-4-2000)
	CV_NOINIT = 8, // dont call function when var is registered (1st set)
	CV_FLOAT = 16, // the value is fixed 16 : 16, where unit is FRACUNIT
	               // (allow user to enter 0.45 for ex)
	               // WARNING: currently only supports set with CV_Set()
	CV_NOTINNET = 32,    // some varaiable can't be changed in network but is not netvar (ex: splitscreen)
	CV_MODIFIED = 64,    // this bit is set when cvar is modified
	CV_SHOWMODIF = 128,  // say something when modified
	CV_SHOWMODIFONETIME = 256, // same but will be reset to 0 when modified, set in toggle
	CV_NOSHOWHELP = 512, // Don't show variable in the HELP list Tails 08-13-2002
	CV_HIDEN = 1024, // variable is not part of the cvar list so cannot be accessed by the console
	                 // can only be set when we have the pointer to it
                   // used on menus
	CV_CHEAT = 2048, // Don't let this be used in multiplayer unless cheats are on.
	CV_NOLUA = 4096,/* don't let this be called from Lua */
} cvflags_t;

typedef struct CV_PossibleValue_s
{
	INT32 value;
	const char *strvalue;
} CV_PossibleValue_t;

typedef struct consvar_s //NULL, NULL, 0, NULL, NULL |, 0, NULL, NULL, 0, 0, NULL
{
	const char *name;
	const char *defaultvalue;
	INT32 flags;            // flags see cvflags_t above
	CV_PossibleValue_t *PossibleValue; // table of possible values
	void (*func)(void);   // called on change, if CV_CALL set
	INT32 value;            // for INT32 and fixed_t
	const char *string;   // value in string
	char *zstring;        // Either NULL or same as string.
	                      // If non-NULL, must be Z_Free'd later.
	struct
	{
		char allocated; // whether to Z_Free
		union
		{
			char       * string;
			const char * const_munge;
		} v;
	} revert;             // value of netvar before joining netgame

	UINT16 netid; // used internaly : netid for send end receive
	                      // used only with CV_NETVAR
	char changed;         // has variable been changed by the user? 0 = no, 1 = yes
	struct consvar_s *next;
} consvar_t;

/* name, defaultvalue, flags, PossibleValue, func */
#define CVAR_INIT( ... ) \
{ __VA_ARGS__, 0, NULL, NULL, {0, {NULL}}, 0U, (char)0, NULL }

#ifdef OLD22DEMOCOMPAT
typedef struct old_demo_var old_demo_var_t;

struct old_demo_var
{
	UINT16  checksum;
	boolean collides;/* this var is a collision of multiple hashes */

	consvar_t      *cvar;
	old_demo_var_t *next;
};
#endif/*OLD22DEMOCOMPAT*/

extern CV_PossibleValue_t CV_OnOff[];
extern CV_PossibleValue_t CV_YesNo[];
extern CV_PossibleValue_t CV_Unsigned[];
extern CV_PossibleValue_t CV_Natural[];

// Filter consvars by version
extern consvar_t cv_execversion;

void CV_InitFilterVar(void);
void CV_ToggleExecVersion(boolean enable);

// register a variable for use at the console
void CV_RegisterVar(consvar_t *variable);

// returns a console variable by name
consvar_t *CV_FindVar(const char *name);

// sets changed to 0 for every console variable
void CV_ClearChangedFlags(void);

// returns the name of the nearest console variable name found
const char *CV_CompleteVar(char *partial, INT32 skips);

// equivalent to "<varname> <value>" typed at the console
void CV_Set(consvar_t *var, const char *value);

// expands value to a string and calls CV_Set
void CV_SetValue(consvar_t *var, INT32 value);

// avoids calling the function if it is CV_CALL
void CV_StealthSetValue(consvar_t *var, INT32 value);
void CV_StealthSet(consvar_t *var, const char *value);

// it a setvalue but with a modulo at the maximum
void CV_AddValue(consvar_t *var, INT32 increment);

// write all CV_SAVE variables to config file
void CV_SaveVariables(FILE *f);

// load/save gamesate (load and save option and for network join in game)
void CV_SaveVars(UINT8 **p, boolean in_demo);

#define CV_SaveNetVars(p) CV_SaveVars(p, false)
void CV_LoadNetVars(UINT8 **p);

// then revert after leaving a netgame
void CV_RevertNetVars(void);

#define CV_SaveDemoVars(p) CV_SaveVars(p, true)
void CV_LoadDemoVars(UINT8 **p);

#ifdef OLD22DEMOCOMPAT
void CV_LoadOldDemoVars(UINT8 **p);
#endif

// reset cheat netvars after cheats is deactivated
void CV_ResetCheatNetVars(void);

boolean CV_IsSetToDefault(consvar_t *v);
UINT8 CV_CheatsEnabled(void);

#endif // __COMMAND_H__

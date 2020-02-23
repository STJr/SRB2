// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  asm_defs.inc
/// \brief must match the C structures

#ifndef __ASM_DEFS__
#define __ASM_DEFS__

// this makes variables more noticable,
// and make the label match with C code

// Linux, unlike DOS, has no "_" 19990119 by Kin
// and nasm needs .data code segs under linux 20010210 by metzgermeister
// FIXME: nasm ignores these settings, so I put the macros into the makefile
#ifdef __ELF__
#define C(label) label
#define CODE_SEG .data
#else
#define C(label) _##label
#define CODE_SEG .text
#endif

/* This is a more readable way to access the arguments passed from C code   */
/* PLEASE NOTE: it is supposed that all arguments passed from C code are    */
/*              32bit integer (INT32, long, and most *pointers)               */
#define ARG1      8(%ebp)
#define ARG2      12(%ebp)
#define ARG3      16(%ebp)
#define ARG4      20(%ebp)
#define ARG5      24(%ebp)
#define ARG6      28(%ebp)
#define ARG7      32(%ebp)
#define ARG8      36(%ebp)
#define ARG9      40(%ebp)      //(c)tm ... Allegro by Shawn Hargreaves.

#endif

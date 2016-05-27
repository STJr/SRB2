// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1995-1996 by the Free Software Foundation, Inc.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  md5.h
/// \brief Functions to compute MD5 message digest of files or memory blocks
///        according to the definition of MD5 in RFC 1321 from April 1992

/* NOTE: The canonical source of this file is maintained with the GNU C
   Library.  Bugs can be reported to bug-glibc@prep.ai.mit.edu. */

#ifndef _MD5_H
#define _MD5_H 1

#include <stdio.h>

#if defined (HAVE_LIMITS_H) || (defined (_LIBC) && _LIBC) || defined (_WIN32)
# include <limits.h>
#endif

/* The following contortions are an attempt to use the C preprocessor
   to determine an unsigned integral type that is 32 bits wide.  An
   alternative approach is to use autoconf's AC_CHECK_SIZEOF macro, but
   doing that would require that the configure script compile and *run*
   the resulting executable.  Locally running cross-compiled executables
   is usually not possible.  */

#ifdef _LIBC
# include <sys/types.h>
typedef u_int32_t md5_uint32;
#else
# if defined __STDC__ && __STDC__
#  define UINT_MAX_32_BITS 4294967295U
# else
#  define UINT_MAX_32_BITS 0xFFFFFFFF
# endif

/* If UINT_MAX isn't defined, assume it's a 32-bit type.
   This should be valid for all systems GNU cares about because
   that doesn't include 16-bit systems, and only modern systems
   (that certainly have <limits.h>) have 64+-bit integral types.  */

# ifndef UINT_MAX
#  define UINT_MAX UINT_MAX_32_BITS
# endif

# if UINT_MAX == UINT_MAX_32_BITS
   typedef unsigned int md5_uint32;
# else
#  if USHRT_MAX == UINT_MAX_32_BITS
    typedef unsigned short md5_uint32;
#  else
#   if ULONG_MAX == UINT_MAX_32_BITS
     typedef unsigned long md5_uint32;
#   else
     /* The following line is intended to evoke an error.
        Using #error is not portable enough.  */
     "Cannot determine unsigned 32-bit data type."
#   endif
#  endif
# endif
#endif

#undef __P
#if defined (__STDC__) && __STDC__
#define	__P(x) x
#else
#define	__P(x) ()
#endif

/*
 * The following three functions are build up the low level used in
 * the functions `md5_stream' and `md5_buffer'.
 */
#if 0
/* Initialize structure containing state of computation.
   (RFC 1321, 3.3: Step 3)  */
extern void md5_init_ctx __P ((struct md5_ctx *ctx));

/* Starting with the result of former calls of this function (or the
   initialization function update the context for the next LEN bytes
   starting at BUFFER.
   It is necessary that LEN is a multiple of 64!!! */
extern void md5_process_block __P ((const void *buffer, size_t len,
                                   struct md5_ctx *ctx));

/* Starting with the result of former calls of this function (or the
   initialization function update the context for the next LEN bytes
   starting at BUFFER.
   It is NOT required that LEN is a multiple of 64.  */
extern void md5_process_bytes __P ((const void *buffer, size_t len,
                                    struct md5_ctx *ctx));

/* Process the remaining bytes in the buffer and put result from CTX
   in first 16 bytes following RESBUF.  The result is always in little
   endian byte order, so that a byte-wise output yields to the wanted
   ASCII representation of the message digest.

   IMPORTANT: On some systems it is required that RESBUF is correctly
   aligned for a 32 bits value.  */
extern void *md5_finish_ctx __P ((struct md5_ctx *ctx, void *resbuf));


/* Put result from CTX in first 16 bytes following RESBUF.  The result is
   always in little endian byte order, so that a byte-wise output yields
   to the wanted ASCII representation of the message digest.

   IMPORTANT: On some systems it is required that RESBUF is correctly
   aligned for a 32 bits value.  */
extern void *md5_read_ctx __P ((const struct md5_ctx *ctx, void *resbuf));

#endif

/* Compute MD5 message digest for bytes read from STREAM.  The
   resulting message digest number will be written into the 16 bytes
   beginning at RESBLOCK.  */
int md5_stream __P ((FILE *stream, void *resblock));

/* Compute MD5 message digest for LEN bytes beginning at BUFFER.  The
   result is always in little endian byte order, so that a byte-wise
   output yields to the wanted ASCII representation of the message
   digest.  */
extern void *md5_buffer __P ((const char *buffer, size_t len, void *resblock));

#endif

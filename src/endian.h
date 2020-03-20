// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2014-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  endian.h
/// \brief Endian detection

#ifndef __ENDIAN__
#define __ENDIAN__

#if defined(SRB2_BIG_ENDIAN) || defined(SRB2_LITTLE_ENDIAN)
  // defined externally
#else
  #if defined(__FreeBSD__)
    // on FreeBSD, _BIG_ENDIAN is a constant to compare
	// _BYTE_ORDER to, not a big-endianess flag
    #include <sys/endian.h>
    #if _BYTE_ORDER == _BIG_ENDIAN
      #define SRB2_BIG_ENDIAN
    #else
      #define SRB2_LITTLE_ENDIAN
    #endif
  #elif defined(__BYTE_ORDER__)
    // defined by at least gcc and clang
    #if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
      #define SRB2_BIG_ENDIAN
    #else
      #define SRB2_LITTLE_ENDIAN
    #endif
  #else
    // check used in vanilla SRB2 (may work incorrectly if
	// _BIG_ENDIAN is used as on FreeBSD)
    #if defined(_BIG_ENDIAN)
      #define SRB2_BIG_ENDIAN
    #else
      #define SRB2_LITTLE_ENDIAN
    #endif
  #endif
#endif

#endif //__ENDIAN__

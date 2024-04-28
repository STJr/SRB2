//-----------------------------------------------------------------------------
//
// Copyright (C) 2017 David Hill
//
// See COPYING for license information.
//
//-----------------------------------------------------------------------------
//
// Numeric identifiers.
//
//-----------------------------------------------------------------------------

#ifndef ACSVM__ID_H__
#define ACSVM__ID_H__

#include "Types.hpp"


//----------------------------------------------------------------------------|
// Extern Functions                                                           |
//

namespace ACSVM
{
   constexpr std::uint32_t MakeID(char c0, char c1, char c2, char c3);
   constexpr std::uint32_t MakeID(char const (&s)[5]);

   //
   // MakeID
   //
   constexpr std::uint32_t MakeID(char c0, char c1, char c2, char c3)
   {
      return
         (static_cast<std::uint32_t>(c0) <<  0) |
         (static_cast<std::uint32_t>(c1) <<  8) |
         (static_cast<std::uint32_t>(c2) << 16) |
         (static_cast<std::uint32_t>(c3) << 24);
   }

   //
   // MakeID
   //
   constexpr std::uint32_t MakeID(char const (&s)[5])
   {
      return MakeID(s[0], s[1], s[2], s[3]);
   }
}

#endif//ACSVM__ID_H__


//-----------------------------------------------------------------------------
//
// Copyright (C) 2015 David Hill
//
// See COPYING for license information.
//
//-----------------------------------------------------------------------------
//
// Jump class.
//
//-----------------------------------------------------------------------------

#ifndef ACSVM__Jump_H__
#define ACSVM__Jump_H__

#include "HashMapFixed.hpp"


//----------------------------------------------------------------------------|
// Types                                                                      |
//

namespace ACSVM
{
   //
   // Jump
   //
   // Dynamic jump target.
   //
   class Jump
   {
   public:
      Word codeIdx;
   };

   //
   // JumpMap
   //
   class JumpMap
   {
   public:
      void loadJumps(Byte const *data, std::size_t count);

      HashMapFixed<Word, Word> table;
   };
}

#endif//ACSVM__Jump_H__


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

#include "Jump.hpp"

#include "BinaryIO.hpp"


//----------------------------------------------------------------------------|
// Extern Functions                                                           |
//

namespace ACSVM
{
   //
   // JumpMap::loadJumps
   //
   void JumpMap::loadJumps(Byte const *data, std::size_t count)
   {
      table.alloc(count);
      std::size_t iter = 0;

      for(auto &jump : table)
      {
         Word caseVal = ReadLE4(data + iter); iter += 4;
         Word codeIdx = ReadLE4(data + iter); iter += 4;
         new(&jump) HashMapFixed<Word, Word>::Elem{caseVal, codeIdx, nullptr};
      }

      table.build();
   }
}

// EOF


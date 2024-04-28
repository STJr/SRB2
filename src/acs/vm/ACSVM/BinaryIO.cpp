//-----------------------------------------------------------------------------
//
// Copyright (C) 2017 David Hill
//
// See COPYING for license information.
//
//-----------------------------------------------------------------------------
//
// Binary data reading/writing primitives.
//
//-----------------------------------------------------------------------------

#include "BinaryIO.hpp"


//----------------------------------------------------------------------------|
// Extern Functions                                                           |
//

namespace ACSVM
{
   //
   // ReadLE4
   //
   std::uint_fast32_t ReadLE4(std::istream &in)
   {
      Byte buf[4];
      for(auto &b : buf) b = in.get();
      return ReadLE4(buf);
   }

   //
   // WriteLE4
   //
   void WriteLE4(std::ostream &out, std::uint_fast32_t in)
   {
      Byte buf[4];
      WriteLE4(buf, in);
      for(auto b : buf) out.put(b);
   }
}

// EOF


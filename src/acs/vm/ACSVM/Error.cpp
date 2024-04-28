//-----------------------------------------------------------------------------
//
// Copyright (C) 2017 David Hill
//
// See COPYING for license information.
//
//-----------------------------------------------------------------------------
//
// Error classes.
//
//-----------------------------------------------------------------------------

#include "Error.hpp"

#include "BinaryIO.hpp"

#include <cctype>
#include <cstdio>


//----------------------------------------------------------------------------|
// Extern Functions                                                           |
//

namespace ACSVM
{
   //
   // SerialSignError constructor
   //
   SerialSignError::SerialSignError(Signature sig_, Signature got_)
   {
      auto sig = static_cast<std::uint32_t>(sig_);
      auto got = static_cast<std::uint32_t>(got_);

      WriteLE4(reinterpret_cast<Byte *>(buf + SigS), sig);
      WriteLE4(reinterpret_cast<Byte *>(buf + GotS), got);
      for(auto i : {SigS+0, SigS+1, SigS+2, SigS+3, GotS+0, GotS+1, GotS+2, GotS+3})
         if(!std::isprint(buf[i]) && !std::isprint(buf[i] = ~buf[i])) buf[i] = ' ';

      for(int i = 8; i--;) buf[Sig + i] = "0123456789ABCDEF"[sig & 0xF], sig >>= 4;
      for(int i = 8; i--;) buf[Got + i] = "0123456789ABCDEF"[got & 0xF], got >>= 4;

      msg = buf;
   }

   //
   // SerialSignError destructor
   //
   SerialSignError::~SerialSignError()
   {
      delete[] msg;
   }
}

// EOF


//-----------------------------------------------------------------------------
//
// Copyright (C) 2017 David Hill
//
// See COPYING for license information.
//
//-----------------------------------------------------------------------------
//
// Serialization.
//
//-----------------------------------------------------------------------------

#include "Serial.hpp"

#include "BinaryIO.hpp"
#include "Error.hpp"

#include <cstring>


//----------------------------------------------------------------------------|
// Extern Functions                                                           |
//

namespace ACSVM
{
   //
   // Serial::loadHead
   //
   void Serial::loadHead()
   {
      char buf[6] = {};
      in->read(buf, 6);

      if(std::memcmp(buf, "ACSVM\0", 6))
         throw SerialError{"invalid file signature"};

      version = ReadVLN<unsigned int>(*in);

      auto flags = ReadVLN<std::uint_fast32_t>(*in);
      signs = flags & 0x0001;
   }

   //
   // Serial::loadTail
   //
   void Serial::loadTail()
   {
      readSign(~Signature::Serial);
   }

   //
   // Serial::readSign
   //
   void Serial::readSign(Signature sign)
   {
      if(!signs) return;

      auto got = static_cast<Signature>(ReadLE4(*in));

      if(sign != got)
         throw SerialSignError{sign, got};
   }

   //
   // Serial::saveHead
   //
   void Serial::saveHead()
   {
      out->write("ACSVM\0", 6);
      WriteVLN(*out, 0);

      std::uint_fast32_t flags = 0;
      if(signs) flags |= 0x0001;
      WriteVLN(*out, flags);
   }

   //
   // Serial::saveTail
   //
   void Serial::saveTail()
   {
      writeSign(~Signature::Serial);
   }

   //
   // Serial::writeSign
   //
   void Serial::writeSign(Signature sign)
   {
      if(!signs) return;

      WriteLE4(*out, static_cast<std::uint32_t>(sign));
   }
}

// EOF


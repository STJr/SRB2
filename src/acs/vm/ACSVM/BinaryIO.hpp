//-----------------------------------------------------------------------------
//
// Copyright (C) 2015-2017 David Hill
//
// See COPYING for license information.
//
//-----------------------------------------------------------------------------
//
// Binary data reading/writing primitives.
//
//-----------------------------------------------------------------------------

#ifndef ACSVM__BinaryIO_H__
#define ACSVM__BinaryIO_H__

#include "Types.hpp"

#include <istream>
#include <ostream>
#include <climits>


//----------------------------------------------------------------------------|
// Extern Functions                                                           |
//

namespace ACSVM
{
   std::uint_fast8_t  ReadLE1(Byte const *data);
   std::uint_fast16_t ReadLE2(Byte const *data);
   std::uint_fast32_t ReadLE4(Byte const *data);
   std::uint_fast32_t ReadLE4(std::istream &in);

   template<typename T>
   T ReadVLN(std::istream &in);

   void WriteLE4(Byte *out, std::uint_fast32_t in);
   void WriteLE4(std::ostream &out, std::uint_fast32_t in);

   template<typename T>
   void WriteVLN(std::ostream &out, T in);

   //
   // ReadLE1
   //
   inline std::uint_fast8_t ReadLE1(Byte const *data)
   {
      return static_cast<std::uint_fast8_t>(data[0]);
   }

   //
   // ReadLE2
   //
   inline std::uint_fast16_t ReadLE2(Byte const *data)
   {
      return
         (static_cast<std::uint_fast16_t>(data[0]) << 0) |
         (static_cast<std::uint_fast16_t>(data[1]) << 8);
   }

   //
   // ReadLE4
   //
   inline std::uint_fast32_t ReadLE4(Byte const *data)
   {
      return
         (static_cast<std::uint_fast32_t>(data[0]) <<  0) |
         (static_cast<std::uint_fast32_t>(data[1]) <<  8) |
         (static_cast<std::uint_fast32_t>(data[2]) << 16) |
         (static_cast<std::uint_fast32_t>(data[3]) << 24);
   }

   //
   // ReadVLN
   //
   template<typename T>
   T ReadVLN(std::istream &in)
   {
      T out{0};

      unsigned char c;
      while(((c = in.get()) & 0x80) && in)
         out = (out << 7) + (c & 0x7F);
      out = (out << 7) + c;

      return out;
   }

   //
   // WriteLE4
   //
   inline void WriteLE4(Byte *out, std::uint_fast32_t in)
   {
      out[0] = (in >>  0) & 0xFF;
      out[1] = (in >>  8) & 0xFF;
      out[2] = (in >> 16) & 0xFF;
      out[3] = (in >> 24) & 0xFF;
   }

   //
   // WriteVLN
   //
   template<typename T>
   void WriteVLN(std::ostream &out, T in)
   {
      constexpr std::size_t len = (sizeof(T) * CHAR_BIT + 6) / 7;
      char buf[len], *ptr = buf + len;

      *--ptr = static_cast<char>(in & 0x7F);
      while((in >>= 7))
         *--ptr = static_cast<char>(in & 0x7F) | 0x80;

      out.write(ptr, (buf + len) - ptr);
   }
}

#endif//ACSVM__BinaryIO_H__


//-----------------------------------------------------------------------------
//
// Copyright (C) 2015 David Hill
//
// See COPYING for license information.
//
//-----------------------------------------------------------------------------
//
// PrintBuf class.
//
//-----------------------------------------------------------------------------

#include "PrintBuf.hpp"

#include "BinaryIO.hpp"

#include <cstdio>
#include <cstdlib>
#include <new>


//----------------------------------------------------------------------------|
// Extern Functions                                                           |
//

namespace ACSVM
{
   //
   // PrintBuf constructor
   //
   PrintBuf::PrintBuf() :
      buffer{nullptr},
      bufEnd{nullptr},
      bufBeg{nullptr},
      bufPtr{nullptr}
   {
   }

   //
   // PrintBuf destructor
   //
   PrintBuf::~PrintBuf()
   {
      std::free(buffer);
   }

   //
   // PrintBuf::drop
   //
   void PrintBuf::drop()
   {
      if(bufBeg != buffer)
      {
         bufPtr = bufBeg - 4;
         bufBeg = bufPtr - ReadLE4(reinterpret_cast<Byte *>(bufPtr));
      }
      else
         bufPtr = bufBeg;
   }

   //
   // PrintBuf::format
   //
   void PrintBuf::format(char const *fmt, ...)
   {
      va_list arg;
      va_start(arg, fmt);
      formatv(fmt, arg);
      va_end(arg);
   }

   //
   // PrintBuf::formatv
   //
   void PrintBuf::formatv(char const *fmt, va_list arg)
   {
      bufPtr += std::vsprintf(bufPtr, fmt, arg);
   }

   //
   // PrintBuf::getLoadBuf
   //
   char *PrintBuf::getLoadBuf(std::size_t countFull, std::size_t count)
   {
      if(static_cast<std::size_t>(bufEnd - buffer) <= countFull)
      {
         char *bufNew;
         if(!(bufNew = static_cast<char *>(std::realloc(buffer, countFull + 1))))
            throw std::bad_alloc();

         buffer = bufNew;
         bufEnd = buffer + countFull + 1;
      }

      bufPtr = buffer + countFull;
      bufBeg = bufPtr - count;

      return buffer;
   }

   //
   // PrintBuf::push
   //
   void PrintBuf::push()
   {
      reserve(4);
      WriteLE4(reinterpret_cast<Byte *>(bufPtr), bufPtr - bufBeg);
      bufBeg = bufPtr += 4;
   }

   //
   // PrintBuf::reserve
   //
   void PrintBuf::reserve(std::size_t count)
   {
      if(static_cast<std::size_t>(bufEnd - bufPtr) > count)
         return;

      // Allocate extra to anticipate further reserves. +1 for null.
      count = count * 2 + 1;

      std::size_t idxEnd = bufEnd - buffer;
      std::size_t idxBeg = bufBeg - buffer;
      std::size_t idxPtr = bufPtr - buffer;

      // Check for size overflow.
      if(SIZE_MAX - idxEnd < count)
         throw std::bad_alloc();

      // Check that the current segment won't pass the push limit.
      if(UINT32_MAX - (idxEnd - idxBeg) < count)
         throw std::bad_alloc();

      idxEnd += count;

      char *bufNew;
      if(!(bufNew = static_cast<char *>(std::realloc(buffer, idxEnd))))
         throw std::bad_alloc();

      buffer = bufNew;
      bufEnd = buffer + idxEnd;
      bufBeg = buffer + idxBeg;
      bufPtr = buffer + idxPtr;
   }
}

// EOF


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

#ifndef ACSVM__PrintBuf_H__
#define ACSVM__PrintBuf_H__

#include "Types.hpp"

#include <cstdarg>


//----------------------------------------------------------------------------|
// Types                                                                      |
//

namespace ACSVM
{
   //
   // PrintBuf
   //
   class PrintBuf
   {
   public:
      PrintBuf();
      ~PrintBuf();

      void clear() {bufBeg = bufPtr = buffer;}

      char const *data() const {return *bufPtr = '\0', bufBeg;}
      char const *dataFull() const {return buffer;}

      void drop();

      // Formats using sprintf. Does not reserve space.
      void format(char const *fmt, ...);
      void formatv(char const *fmt, std::va_list arg);

      // Returns a pointer to count chars to write into. The caller must write
      // to the entire returned buffer. Does not reserve space.
      char *getBuf(std::size_t count)
         {char *s = bufPtr; bufPtr += count; return s;}

      // Prepares the buffer to be deserialized.
      char *getLoadBuf(std::size_t countFull, std::size_t count);

      void push();

      // Writes literal characters. Does not reserve space.
      void put(char c) {*bufPtr++ = c;}
      void put(char const *s) {while(*s) *bufPtr++ = *s++;}
      void put(char const *s, std::size_t n) {while(n--) *bufPtr++ = *s++;}

      // Ensures at least count chars are available for writing into.
      void reserve(std::size_t count);

      std::size_t size() const {return bufPtr - bufBeg;}
      std::size_t sizeFull() const {return bufPtr - buffer;}

   private:
      char *buffer, *bufEnd, *bufBeg, *bufPtr;
   };
}

#endif//ACSVM__PrintBuf_H__


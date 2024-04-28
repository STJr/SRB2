//-----------------------------------------------------------------------------
//
// Copyright (C) 2015-2017 David Hill
//
// See COPYING for license information.
//
//-----------------------------------------------------------------------------
//
// Module class bytecode reading.
//
//-----------------------------------------------------------------------------

#include "Module.hpp"

#include "BinaryIO.hpp"
#include "Environment.hpp"
#include "Error.hpp"
#include "Jump.hpp"
#include "Script.hpp"
#include "Tracer.hpp"


//----------------------------------------------------------------------------|
// Extern Functions                                                           |
//

namespace ACSVM
{
   //
   // Module::reaBytecode
   //
   void Module::readBytecode(Byte const *data, std::size_t size)
   {
      try
      {
         if(size < 4) throw ReadError();

         switch(ReadLE4(data))
         {
         case MakeID("ACS\0"):
            readBytecodeACS0(data, size);
            break;

         case MakeID("ACSE"):
            readBytecodeACSE(data, size, false);
            break;

         case MakeID("ACSe"):
            readBytecodeACSE(data, size, true);
            break;
         }
      }
      catch(...)
      {
         // If an exception occurs before module is fully loaded, reset it.
         if(!loaded)
            reset();

         throw;
      }
   }

   //
   // Module::readBytecodeACS0
   //
   void Module::readBytecodeACS0(Byte const *data, std::size_t size)
   {
      std::size_t iter;

      // Read table index.
      if(size < 8) throw ReadError();
      iter = ReadLE4(data + 4);
      if(iter > size) throw ReadError();

      // Check for ACSE header behind indicated table.
      if(iter >= 8)
      {
         switch(ReadLE4(data + (iter - 4)))
         {
         case MakeID("ACSE"):
            return readBytecodeACSE(data, size, false, iter - 8);

         case MakeID("ACSe"):
            return readBytecodeACSE(data, size, true, iter - 8);
         }
      }

      // Mark as ACS0.
      isACS0 = true;

      // Read script table.

      // Read script count.
      if(size - iter < 4) throw ReadError();
      scriptV.alloc(ReadLE4(data + iter), this); iter += 4;

      // Read scripts.
      if(size - iter < scriptV.size() * 12) throw ReadError();
      for(Script &scr : scriptV)
      {
         scr.name.i  = ReadLE4(data + iter); iter += 4;
         scr.codeIdx = ReadLE4(data + iter); iter += 4;
         scr.argC    = ReadLE4(data + iter); iter += 4;

         std::tie(scr.type, scr.name.i) = env->getScriptTypeACS0(scr.name.i);
      }

      // Read string table.

      // Read string count.
      if(size - iter < 4) throw ReadError();
      stringV.alloc(ReadLE4(data + iter)); iter += 4;

      // Read strings.
      if(size - iter < stringV.size() * 4) throw ReadError();
      for(String *&str : stringV)
      {
         str = readStringACS0(data, size, ReadLE4(data + iter)); iter += 4;
      }

      // Read code.
      readCodeACS0(data, size, false);

      loaded = true;
   }

   //
   // Module::readCodeACS0
   //
   void Module::readCodeACS0(Byte const *data, std::size_t size, bool compressed)
   {
      TracerACS0 tracer{env, data, size, compressed};

      // Trace code paths from this module.
      tracer.trace(this);

      codeV.alloc(tracer.codeC);
      jumpMapV.alloc(tracer.jumpMapC);

      tracer.translate(this);
   }

   //
   // Module::readStringACS0
   //
   String *Module::readStringACS0(Byte const *data, std::size_t size, std::size_t iter)
   {
      Byte const *begin, *end;
      std::size_t len;
      std::tie(begin, end, len) = ScanStringACS0(data, size, iter);

      // If result length is same as input length, no processing is needed.
      if(static_cast<std::size_t>(end - begin) == len)
      {
         // Byte is always unsigned char, which is allowed to alias with char.
         return env->getString(
            reinterpret_cast<char const *>(begin),
            reinterpret_cast<char const *>(end));
      }
      else
         return env->getString(ParseStringACS0(begin, end, len).get(), len);
   }

   //
   // Module::ParseStringACS0
   //
   std::unique_ptr<char[]> Module::ParseStringACS0(Byte const *first,
      Byte const *last, std::size_t len)
   {
      std::unique_ptr<char[]> buf{new char[len + 1]};
      char                   *bufItr = buf.get();

      for(Byte const *s = first; s != last;)
      {
         if(*s == '\\')
         {
            if(++s == last)
               break;

            switch(*s)
            {
            case 'a': *bufItr++ += '\a';   ++s; break;
            case 'b': *bufItr++ += '\b';   ++s; break;
            case 'c': *bufItr++ += '\x1C'; ++s; break; // ZDoom color escape
            case 'f': *bufItr++ += '\f';   ++s; break;
            case 'r': *bufItr++ += '\r';   ++s; break;
            case 'n': *bufItr++ += '\n';   ++s; break;
            case 't': *bufItr++ += '\t';   ++s; break;
            case 'v': *bufItr++ += '\v';   ++s; break;

            case '0': case '1': case '2': case '3':
            case '4': case '5': case '6': case '7':
               for(unsigned int i = 3, c = 0; i-- && s != last; ++s)
               {
                  switch(*s)
                  {
                  case '0': c = c * 8 + 00; continue;
                  case '1': c = c * 8 + 01; continue;
                  case '2': c = c * 8 + 02; continue;
                  case '3': c = c * 8 + 03; continue;
                  case '4': c = c * 8 + 04; continue;
                  case '5': c = c * 8 + 05; continue;
                  case '6': c = c * 8 + 06; continue;
                  case '7': c = c * 8 + 07; continue;
                  }

                  *bufItr++ = c;
                  break;
               }
               break;

            case 'X': case 'x':
               ++s;
               for(unsigned int i = 2, c = 0; i-- && s != last; ++s)
               {
                  switch(*s)
                  {
                  case '0': c = c * 16 + 0x0; continue;
                  case '1': c = c * 16 + 0x1; continue;
                  case '2': c = c * 16 + 0x2; continue;
                  case '3': c = c * 16 + 0x3; continue;
                  case '4': c = c * 16 + 0x4; continue;
                  case '5': c = c * 16 + 0x5; continue;
                  case '6': c = c * 16 + 0x6; continue;
                  case '7': c = c * 16 + 0x7; continue;
                  case '8': c = c * 16 + 0x8; continue;
                  case '9': c = c * 16 + 0x9; continue;
                  case 'A': c = c * 16 + 0xA; continue;
                  case 'B': c = c * 16 + 0xB; continue;
                  case 'C': c = c * 16 + 0xC; continue;
                  case 'D': c = c * 16 + 0xD; continue;
                  case 'E': c = c * 16 + 0xE; continue;
                  case 'F': c = c * 16 + 0xF; continue;
                  case 'a': c = c * 16 + 0xa; continue;
                  case 'b': c = c * 16 + 0xb; continue;
                  case 'c': c = c * 16 + 0xc; continue;
                  case 'd': c = c * 16 + 0xd; continue;
                  case 'e': c = c * 16 + 0xe; continue;
                  case 'f': c = c * 16 + 0xf; continue;
                  }

                  *bufItr++ = c;
                  break;
               }
               break;

            default:
               *bufItr++ = *s++;
               break;
            }
         }
         else
            *bufItr++ = *s++;
      }

      *bufItr++ = '\0';

      return buf;
   }

   //
   // Module::ScanStringACS0
   //
   std::tuple<
      Byte const * /*begin*/,
      Byte const * /*end*/,
      std::size_t  /*len*/>
   Module::ScanStringACS0(Byte const *data, std::size_t size, std::size_t iter)
   {
      if(iter > size) throw ReadError();

      Byte const *begin = data + iter;
      Byte const *end   = data + size;
      Byte const *s     = begin;
      std::size_t len   = 0;

      while(s != end && *s)
      {
         if(*s++ == '\\')
         {
            if(s == end || !*s)
               break;

            switch(*s++)
            {
            case '0': case '1': case '2': case '3':
            case '4': case '5': case '6': case '7':
               for(int i = 2; i-- && s != end; ++s)
               {
                  switch(*s)
                  {
                  case '0': case '1': case '2': case '3':
                  case '4': case '5': case '6': case '7':
                     continue;
                  }

                  break;
               }
               break;

            case 'X': case 'x':
               for(int i = 2; i-- && s != end; ++s)
               {
                  switch(*s)
                  {
                  case '0': case '1': case '2': case '3': case '4':
                  case '5': case '6': case '7': case '8': case '9':
                  case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
                  case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
                     continue;
                  }

                  break;
               }
               break;

            default:
               break;
            }
         }

         ++len;
      }

      // If not terminated by a null, string is malformed.
      if(s == end) throw ReadError();

      return std::make_tuple(begin, s, len);
   }
}

// EOF


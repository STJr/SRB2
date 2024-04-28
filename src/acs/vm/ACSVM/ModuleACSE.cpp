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

#include "Array.hpp"
#include "BinaryIO.hpp"
#include "Environment.hpp"
#include "Error.hpp"
#include "Function.hpp"
#include "Init.hpp"
#include "Jump.hpp"
#include "Script.hpp"

#include <algorithm>


//----------------------------------------------------------------------------|
// Extern Functions                                                           |
//

namespace ACSVM
{
   //
   // Module::chunkIterACSE
   //
   bool Module::chunkIterACSE(Byte const *data, std::size_t size,
      bool (Module::*chunker)(Byte const *, std::size_t, Word))
   {
      std::size_t iter = 0;

      while(iter != size)
      {
         // Need space for header.
         if(size - iter < 8) throw ReadError();

         // Read header.
         Word chunkName = ReadLE4(data + iter + 0);
         Word chunkSize = ReadLE4(data + iter + 4);

         // Consume header.
         iter += 8;

         // Need space for payload.
         if(size - iter < chunkSize) throw ReadError();

         // Read payload.
         if((this->*chunker)(data + iter, chunkSize, chunkName))
            return true;

         // Consume payload.
         iter += chunkSize;
      }

      return false;
   }

   //
   // Module::chunkStrTabACSE
   //
   void Module::chunkStrTabACSE(Vector<String *> &strV,
      Byte const *data, std::size_t size, bool junk)
   {
      std::size_t iter = 0;

      if(junk)
      {
         if(size < 12) throw ReadError();

         /*junk   = ReadLE4(data + iter);*/ iter += 4;
         strV.alloc(ReadLE4(data + iter));  iter += 4;
         /*junk   = ReadLE4(data + iter);*/ iter += 4;
      }
      else
      {
         if(size < 4) throw ReadError();

         strV.alloc(ReadLE4(data + iter)); iter += 4;
      }

      if(size - iter < strV.size() * 4) throw ReadError();
      for(String *&str : strV)
      {
         str = readStringACS0(data, size, ReadLE4(data + iter)); iter += 4;
      }
   }

   //
   // Module::chunkerACSE_AIMP
   //
   bool Module::chunkerACSE_AIMP(Byte const *data, std::size_t size, Word chunkName)
   {
      if(chunkName != MakeID("AIMP")) return false;

      if(size < 4) throw ReadError();

      // Chunk starts with a number of entries. However, that is redundant with
      // just checking for the end of the chunk as in MIMP, so do that.

      // Determine highest index.
      Word arrC = 0;
      for(std::size_t iter = 4; iter != size;)
      {
         if(size - iter < 8) throw ReadError();

         Word idx = ReadLE4(data + iter);   iter += 4;
         /*   len = LeadLE4(data + iter);*/ iter += 4;

         arrC = std::max<Word>(arrC, idx + 1);

         Byte const *next;
         std::tie(std::ignore, next, std::ignore) = ScanStringACS0(data, size, iter);

         iter = next - data + 1;
      }

      // Read imports.
      arrImpV.alloc(arrC);
      for(std::size_t iter = 4; iter != size;)
      {
         Word idx = ReadLE4(data + iter);   iter += 4;
         /*   len = LeadLE4(data + iter);*/ iter += 4;

         Byte const *next;
         std::size_t len;
         std::tie(std::ignore, next, len) = ScanStringACS0(data, size, iter);

         std::unique_ptr<char[]> str = ParseStringACS0(data + iter, next, len);

         arrImpV[idx] = env->getString(str.get(), len);

         iter = next - data + 1;
      }

      return true;
   }

   //
   // Module::chunkerACSE_AINI
   //
   bool Module::chunkerACSE_AINI(Byte const *data, std::size_t size, Word chunkName)
   {
      if(chunkName != MakeID("AINI")) return false;

      if(size < 4 || size % 4) throw ReadError();

      Word idx = ReadLE4(data);

      // Silently ignore out of bounds initializers.
      if(idx >= arrInitV.size()) return false;

      auto &init = arrInitV[idx];
      for(std::size_t iter = 4; iter != size; iter += 4)
         init.setVal(iter / 4 - 1, ReadLE4(data + iter));

      return false;
   }

   //
   // Module::chunkerACSE_ARAY
   //
   bool Module::chunkerACSE_ARAY(Byte const *data, std::size_t size, Word chunkName)
   {
      if(chunkName != MakeID("ARAY")) return false;

      if(size % 8) throw ReadError();

      Word arrC = 0;

      // Determine highest index.
      for(std::size_t iter = 0; iter != size; iter += 8)
         arrC = std::max<Word>(arrC, ReadLE4(data + iter) + 1);

      arrNameV.alloc(arrC);
      arrInitV.alloc(arrC);
      arrSizeV.alloc(arrC);

      for(std::size_t iter = 0; iter != size;)
      {
         Word idx = ReadLE4(data + iter); iter += 4;
         Word len = ReadLE4(data + iter); iter += 4;

         arrInitV[idx].reserve(len);
         arrSizeV[idx] = len;

         // Use names from MEXP.
         if(idx < regNameV.size())
         {
            arrNameV[idx] = regNameV[idx];
            regNameV[idx] = nullptr;
         }
      }

      return true;
   }

   //
   // Module::chunkerACSE_ASTR
   //
   bool Module::chunkerACSE_ASTR(Byte const *data, std::size_t size, Word chunkName)
   {
      if(chunkName != MakeID("ASTR")) return false;

      if(size % 4) throw ReadError();

      for(std::size_t iter = 0; iter != size;)
      {
         Word idx = ReadLE4(data + iter); iter += 4;

         // Silently ignore out of bounds initializers.
         if(idx >= arrInitV.size()) continue;

         auto &init = arrInitV[idx];
         for(Word i = 0, e = arrSizeV[idx]; i != e; ++i)
            init.setTag(i, InitTag::String);
      }

      return false;
   }

   //
   // Module::chunkerACSE_ATAG
   //
   bool Module::chunkerACSE_ATAG(Byte const *data, std::size_t size, Word chunkName)
   {
      if(chunkName != MakeID("ATAG")) return false;

      if(size < 5 || data[0]) throw ReadError();

      Word idx = ReadLE4(data + 1);

      // Silently ignore out of bounds initializers.
      if(idx >= arrInitV.size()) return false;

      auto &init = arrInitV[idx];
      for(std::size_t iter = 5; iter != size; ++iter)
      {
         switch(data[iter])
         {
         case 0: init.setTag(iter - 5, InitTag::Integer);  break;
         case 1: init.setTag(iter - 5, InitTag::String);   break;
         case 2: init.setTag(iter - 5, InitTag::Function); break;
         }
      }

      return false;
   }

   //
   // Module::chunkerACSE_FARY
   //
   bool Module::chunkerACSE_FARY(Byte const *data, std::size_t size, Word chunkName)
   {
      if(chunkName != MakeID("FARY")) return false;

      if(size < 2 || (size - 2) % 4) throw ReadError();

      Word        idx  = ReadLE2(data);
      std::size_t arrC = (size - 2) / 4;

      if(idx < functionV.size() && functionV[idx])
         functionV[idx]->locArrC = arrC;

      return false;
   }

   //
   // Module::chunkerACSE_FNAM
   //
   bool Module::chunkerACSE_FNAM(Byte const *data, std::size_t size, Word chunkName)
   {
      if(chunkName != MakeID("FNAM")) return false;

      chunkStrTabACSE(funcNameV, data, size, false);

      return true;
   }

   //
   // Module::chunkerACSE_FUNC
   //
   bool Module::chunkerACSE_FUNC(Byte const *data, std::size_t size, Word chunkName)
   {
      if(chunkName != MakeID("FUNC")) return false;

      if(size % 8) throw ReadError();

      // Read functions.
      functionV.alloc(size / 8);

      std::size_t iter = 0;
      for(Function *&func : functionV)
      {
         Word idx     = iter / 8;
         Word argC    = ReadLE1(data + iter); iter += 1;
         Word locRegC = ReadLE1(data + iter); iter += 1;
         Word flags   = ReadLE2(data + iter); iter += 2;
         Word codeIdx = ReadLE4(data + iter); iter += 4;

         // Ignore undefined functions for now.
         if(!codeIdx) continue;

         String   *funcName = idx < funcNameV.size() ? funcNameV[idx] : nullptr;
         Function *function = env->getFunction(this, funcName);

         function->argC    = argC;
         function->locRegC = locRegC;
         function->flagRet = flags & 0x0001;
         function->codeIdx = codeIdx;

         func = function;
      }

      return true;
   }

   //
   // Module::chunkerACSE_JUMP
   //
   bool Module::chunkerACSE_JUMP(Byte const *data, std::size_t size, Word chunkName)
   {
      if(chunkName != MakeID("JUMP")) return false;

      if(size % 4) throw ReadError();

      // Read jumps.
      jumpV.alloc(size / 4);

      std::size_t iter = 0;
      for(Jump &jump : jumpV)
      {
         jump.codeIdx = ReadLE4(data + iter); iter += 4;
      }

      return true;
   }

   //
   // Module::chunkerACSE_LOAD
   //
   bool Module::chunkerACSE_LOAD(Byte const *data, std::size_t size, Word chunkName)
   {
      if(chunkName != MakeID("LOAD")) return false;

      // Count imports.
      std::size_t importC = 0;
      for(Byte const *iter = data, *end = data + size; iter != end; ++ iter)
         if(!*iter) ++importC;

      importV.alloc(importC);

      for(std::size_t iter = 0, i = 0; iter != size;)
      {
         Byte const *next;
         std::size_t len;
         std::tie(std::ignore, next, len) = ScanStringACS0(data, size, iter);

         std::unique_ptr<char[]> str = ParseStringACS0(data + iter, next, len);

         auto loadName = env->getModuleName(str.get(), len);
         if(loadName != name)
            importV[i++] = env->getModule(std::move(loadName));
         else
            importV[i++] = this;

         iter = next - data + 1;
      }

      return true;
   }

   //
   // Module::chunkerACSE_MEXP
   //
   bool Module::chunkerACSE_MEXP(Byte const *data, std::size_t size, Word chunkName)
   {
      if(chunkName != MakeID("MEXP")) return false;

      chunkStrTabACSE(regNameV, data, size, false);

      return true;
   }

   //
   // Module::chunkerACSE_MIMP
   //
   bool Module::chunkerACSE_MIMP(Byte const *data, std::size_t size, Word chunkName)
   {
      if(chunkName != MakeID("MIMP")) return false;

      // Determine highest index.
      Word regC = 0;
      for(std::size_t iter = 0; iter != size;)
      {
         if(size - iter < 4) throw ReadError();

         Word idx = ReadLE4(data + iter); iter += 4;

         regC = std::max<Word>(regC, idx + 1);

         Byte const *next;
         std::tie(std::ignore, next, std::ignore) = ScanStringACS0(data, size, iter);

         iter = next - data + 1;
      }

      // Read imports.
      regImpV.alloc(regC);
      for(std::size_t iter = 0; iter != size;)
      {
         Word idx = ReadLE4(data + iter); iter += 4;

         Byte const *next;
         std::size_t len;
         std::tie(std::ignore, next, len) = ScanStringACS0(data, size, iter);

         std::unique_ptr<char[]> str = ParseStringACS0(data + iter, next, len);

         regImpV[idx] = env->getString(str.get(), len);

         iter = next - data + 1;
      }

      return true;
   }

   //
   // Module::chunkerACSE_MINI
   //
   bool Module::chunkerACSE_MINI(Byte const *data, std::size_t size, Word chunkName)
   {
      if(chunkName != MakeID("MINI")) return false;

      if(size % 4 || size < 4) throw ReadError("bad MINI size");

      Word idx  = ReadLE4(data);
      Word regC = idx + size / 4 - 1;

      if(regC > regInitV.size())
         regInitV.realloc(regC);

      for(std::size_t iter = 4; iter != size; iter += 4)
         regInitV[idx++] = ReadLE4(data + iter);

      return true;
   }

   //
   // Module::chunkerACSE_MSTR
   //
   bool Module::chunkerACSE_MSTR(Byte const *data, std::size_t size, Word chunkName)
   {
      if(chunkName != MakeID("MSTR")) return false;

      if(size % 4) throw ReadError();

      for(std::size_t iter = 0; iter != size;)
      {
         Word idx = ReadLE4(data + iter); iter += 4;

         // Silently ignore out of bounds initializers.
         if(idx < regInitV.size())
            regInitV[idx].tag = InitTag::String;
      }

      return false;
   }

   //
   // Module::chunkerACSE_SARY
   //
   bool Module::chunkerACSE_SARY(Byte const *data, std::size_t size, Word chunkName)
   {
      if(chunkName != MakeID("SARY")) return false;

      if(size < 2 || (size - 2) % 4) throw ReadError();

      Word        nameInt = ReadLE2(data);
      std::size_t arrC    = (size - 2) / 4;

      if(nameInt & 0x8000) nameInt |= 0xFFFF0000;

      for(Script &scr : scriptV)
         if(scr.name.i == nameInt) scr.locArrC = arrC;

      return false;
   }

   //
   // Module::chunkerACSE_SFLG
   //
   bool Module::chunkerACSE_SFLG(Byte const *data, std::size_t size, Word chunkName)
   {
      if(chunkName != MakeID("SFLG")) return false;

      if(size % 4) throw ReadError();

      for(std::size_t iter = 0; iter != size;)
      {
         Word nameInt = ReadLE2(data + iter); iter += 2;
         Word flags   = ReadLE2(data + iter); iter += 2;

         bool flagNet    = !!(flags & 0x0001);
         bool flagClient = !!(flags & 0x0002);

         if(nameInt & 0x8000) nameInt |= 0xFFFF0000;

         for(Script &scr : scriptV)
         {
            if(scr.name.i == nameInt)
            {
               scr.flagClient = flagClient;
               scr.flagNet    = flagNet;
            }
         }
      }

      return false;
   }

   //
   // Module::chunkerACSE_SNAM
   //
   bool Module::chunkerACSE_SNAM(Byte const *data, std::size_t size, Word chunkName)
   {
      if(chunkName != MakeID("SNAM")) return false;

      chunkStrTabACSE(scrNameV, data, size, false);

      return true;
   }

   //
   // Module::chunkerACSE_SPTR8
   //
   // Reads 8-byte SPTR chunk.
   //
   bool Module::chunkerACSE_SPTR8(Byte const *data, std::size_t size, Word chunkName)
   {
      if(chunkName != MakeID("SPTR")) return false;

      if(size % 8) throw ReadError();

      // Read scripts.
      scriptV.alloc(size / 8, this);

      std::size_t iter = 0;
      for(Script &scr : scriptV)
      {
         Word nameInt = ReadLE2(data + iter); iter += 2;
         Word type    = ReadLE1(data + iter); iter += 1;
         scr.argC     = ReadLE1(data + iter); iter += 1;
         scr.codeIdx  = ReadLE4(data + iter); iter += 4;

         if(nameInt & 0x8000) nameInt |= 0xFFFF0000;
         setScriptNameTypeACSE(&scr, nameInt, type);
      }

      return true;
   }

   //
   // Module::chunkerACSE_SPTR12
   //
   // Reads 12-byte SPTR chunk.
   //
   bool Module::chunkerACSE_SPTR12(Byte const *data, std::size_t size, Word chunkName)
   {
      if(chunkName != MakeID("SPTR")) return false;

      if(size % 12) throw ReadError();

      // Read scripts.
      scriptV.alloc(size / 12, this);

      std::size_t iter = 0;
      for(Script &scr : scriptV)
      {
         Word nameInt = ReadLE2(data + iter); iter += 2;
         Word type    = ReadLE2(data + iter); iter += 2;
         scr.codeIdx  = ReadLE4(data + iter); iter += 4;
         scr.argC     = ReadLE4(data + iter); iter += 4;

         if(nameInt & 0x8000) nameInt |= 0xFFFF0000;
         setScriptNameTypeACSE(&scr, nameInt, type);
      }

      return true;
   }

   //
   // Module::chunkerACSE_STRE
   //
   bool Module::chunkerACSE_STRE(Byte const *data, std::size_t size, Word chunkName)
   {
      if(chunkName != MakeID("STRE")) return false;

      std::size_t iter = 0;

      if(size < 12) throw ReadError();

      /*junk      = ReadLE4(data + iter);*/ iter += 4;
      stringV.alloc(ReadLE4(data + iter));  iter += 4;
      /*junk      = ReadLE4(data + iter);*/ iter += 4;

      if(size - iter < stringV.size() * 4) throw ReadError();
      for(String *&str : stringV)
      {
         std::size_t offset = ReadLE4(data + iter); iter += 4;

         // Decrypt string.
         std::unique_ptr<Byte[]> buf;
         std::size_t             len;
         std::tie(buf, len) = DecryptStringACSE(data, size, offset);

         // Scan string.
         Byte const *bufEnd;
         std::tie(std::ignore, bufEnd, len) = ScanStringACS0(buf.get(), len, 0);

         // Parse string.
         str = env->getString(ParseStringACS0(buf.get(), bufEnd, len).get(), len);
      }

      return true;
   }

   //
   // Module::chunkerACSE_STRL
   //
   bool Module::chunkerACSE_STRL(Byte const *data, std::size_t size, Word chunkName)
   {
      if(chunkName != MakeID("STRL")) return false;

      chunkStrTabACSE(stringV, data, size, true);

      return true;
   }

   //
   // Module::chunkerACSE_SVCT
   //
   bool Module::chunkerACSE_SVCT(Byte const *data, std::size_t size, Word chunkName)
   {
      if(chunkName != MakeID("SVCT")) return false;

      if(size % 4) throw ReadError();

      for(std::size_t iter = 0; iter != size;)
      {
         Word nameInt = ReadLE2(data + iter); iter += 2;
         Word regC    = ReadLE2(data + iter); iter += 2;

         if(nameInt & 0x8000) nameInt |= 0xFFFF0000;

         for(Script &scr : scriptV)
         {
            if(scr.name.i == nameInt)
               scr.locRegC = regC;
         }
      }

      return false;
   }

   //
   // Module::readBytecodeACSE
   //
   void Module::readBytecodeACSE(Byte const *data, std::size_t size,
      bool compressed, std::size_t offset)
   {
      std::size_t iter = offset;

      // Find table start.
      if(iter > size || size - iter < 4) throw ReadError();
      iter = ReadLE4(data + iter);
      if(iter > size) throw ReadError();

      // Read chunks.
      if(offset == 4)
      {
         readChunksACSE(data + iter, size - iter, false);
      }
      else
      {
         if(iter <= offset)
            readChunksACSE(data + iter, offset - iter, true);
         else
            readChunksACSE(data + iter, size - iter, true);
      }

      // Read code.
      readCodeACS0(data, size, compressed);

      loaded = true;
   }

   //
   // Module::readChunksACSE
   //
   void Module::readChunksACSE(Byte const *data, std::size_t size, bool fakeACS0)
   {
      // MEXP - Module Variable/Array Export
      chunkIterACSE(data, size, &Module::chunkerACSE_MEXP);

      // ARAY - Module Arrays
      chunkIterACSE(data, size, &Module::chunkerACSE_ARAY);

      // AINI - Module Array Init
      chunkIterACSE(data, size, &Module::chunkerACSE_AINI);

      // FNAM - Function Names
      chunkIterACSE(data, size, &Module::chunkerACSE_FNAM);

      // FUNC - Functions
      chunkIterACSE(data, size, &Module::chunkerACSE_FUNC);

      // FARY - Function Arrays
      chunkIterACSE(data, size, &Module::chunkerACSE_FARY);

      // JUMP - Dynamic Jump Targets
      chunkIterACSE(data, size, &Module::chunkerACSE_JUMP);

      // MINI - Module Variable Init
      chunkIterACSE(data, size, &Module::chunkerACSE_MINI);

      // SNAM - Script Names
      chunkIterACSE(data, size, &Module::chunkerACSE_SNAM);

      // SPTR - Script Pointers
      if(fakeACS0)
         chunkIterACSE(data, size, &Module::chunkerACSE_SPTR8);
      else
         chunkIterACSE(data, size, &Module::chunkerACSE_SPTR12);

      // SARY - Script Arrays
      chunkIterACSE(data, size, &Module::chunkerACSE_SARY);

      // SFLG - Script Flags
      chunkIterACSE(data, size, &Module::chunkerACSE_SFLG);

      // SVCT - Script Variable Count
      chunkIterACSE(data, size, &Module::chunkerACSE_SVCT);

      // STRE - Encrypted String Literals
      if(!chunkIterACSE(data, size, &Module::chunkerACSE_STRE))
      {
         // STRL - String Literals
         chunkIterACSE(data, size, &Module::chunkerACSE_STRL);
      }

      // LOAD - Library Loading
      chunkIterACSE(data, size, &Module::chunkerACSE_LOAD);

      // Process function imports.
      for(auto &func : functionV)
      {
         if(func) continue;

         std::size_t idx = &func - functionV.data();

         if(idx >= funcNameV.size()) continue;

         auto &funcName = funcNameV[idx];

         if(!funcName) continue;

         for(auto &import : importV)
         {
            for(auto &funcImp : import->functionV)
            {
               if(funcImp && funcImp->name == funcName)
               {
                  func = funcImp;
                  goto func_found;
               }
            }
         }

      func_found:;
      }

      // AIMP - Module Array Import
      chunkIterACSE(data, size, &Module::chunkerACSE_AIMP);

      // MIMP - Module Variable Import
      chunkIterACSE(data, size, &Module::chunkerACSE_MIMP);

      // ASTR - Module Array Strings
      chunkIterACSE(data, size, &Module::chunkerACSE_ASTR);

      // ATAG - Module Array Tagging
      chunkIterACSE(data, size, &Module::chunkerACSE_ATAG);

      // MSTR - Module Variable Strings
      chunkIterACSE(data, size, &Module::chunkerACSE_MSTR);

      for(auto &init : arrInitV)
         init.finish();
   }

   //
   // Module::setScriptNameTypeACSE
   //
   void Module::setScriptNameTypeACSE(Script *scr, Word nameInt, Word type)
   {
      // If high bit is set, script is named.
      if((scr->name.i = nameInt) & 0x80000000)
      {
         // Fetch name.
         Word nameIdx = ~scr->name.i;
         if(nameIdx < scrNameV.size())
            scr->name.s = scrNameV[nameIdx];
      }

      scr->type = env->getScriptTypeACSE(type);
   }

   //
   // Module::DecryptStringACSE
   //
   std::pair<
      std::unique_ptr<Byte[]> /*data*/,
      std::size_t             /*size*/>
   Module::DecryptStringACSE(Byte const *data, std::size_t size, std::size_t iter)
   {
      Word const key = iter * 157135;

      // Calculate length. Start at 1 for null terminator.
      std::size_t len = 1;
      for(std::size_t i = iter, n = 0;; ++i, ++n, ++len)
      {
         if(i == size) throw ReadError();

         Byte c = static_cast<Byte>(data[i] ^ (n / 2 + key));
         if(!c) break;
      }

      // Decrypt data.
      std::unique_ptr<Byte[]> buf{new Byte[len]};
      for(std::size_t i = iter, n = 0;; ++i, ++n)
      {
         Byte c = static_cast<Byte>(data[i] ^ (n / 2 + key));
         if(!(buf[n] = c)) break;
      }

      return {std::move(buf), len};
   }
}

// EOF


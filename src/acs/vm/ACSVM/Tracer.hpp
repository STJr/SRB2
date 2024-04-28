//-----------------------------------------------------------------------------
//
// Copyright (C) 2015 David Hill
//
// See COPYING for license information.
//
//-----------------------------------------------------------------------------
//
// Tracer classes.
//
//-----------------------------------------------------------------------------

#ifndef ACSVM__Tracer_H__
#define ACSVM__Tracer_H__

#include "Types.hpp"

#include <memory>


//----------------------------------------------------------------------------|
// Types                                                                      |
//

namespace ACSVM
{
   //
   // TracerACS0
   //
   // Traces input bytecode in ACS0 format for code paths, and then
   // translates discovered codes.
   //
   class TracerACS0
   {
   public:
      TracerACS0(Environment *env, Byte const *data, std::size_t size, bool compressed);
      ~TracerACS0();

      void trace(Module *module);

      void translate(Module *module);

      Environment *env;

      std::unique_ptr<Byte[]> codeFound;
      std::unique_ptr<Word[]> codeIndex;
      std::size_t             codeC;

      std::size_t jumpC;

      std::size_t jumpMapC;

   private:
      std::size_t getArgBytes(CodeDataACS0 const *opData, std::size_t iter);

      std::pair<Word /*argc*/, Word /*func*/> readCallFunc(std::size_t iter);

      std::tuple<
         Word                 /*opCode*/,
         CodeDataACS0 const * /*opData*/,
         std::size_t          /*opSize*/>
      readOpACS0(std::size_t iter);

      bool setFound(std::size_t first, std::size_t last);

      void trace(std::size_t iter);

      // Bytecode information.
      Byte const *data;
      std::size_t size;
      bool        compressed;
   };
}

#endif//ACSVM__Tracer_H__


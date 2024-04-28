//-----------------------------------------------------------------------------
//
// Copyright (C) 2015-2017 David Hill
//
// See COPYING for license information.
//
//-----------------------------------------------------------------------------
//
// Script class.
//
//-----------------------------------------------------------------------------

#ifndef ACSVM__Script_H__
#define ACSVM__Script_H__

#include "Types.hpp"


//----------------------------------------------------------------------------|
// Types                                                                      |
//

namespace ACSVM
{
   //
   // ScriptName
   //
   class ScriptName
   {
   public:
      ScriptName() : s{nullptr}, i{0} {}
      ScriptName(String *s_) : s{s_}, i{0} {}
      ScriptName(String *s_, Word i_) : s{s_}, i{i_} {}
      ScriptName(Word i_) : s{nullptr}, i{i_} {}

      String *s;
      Word    i;
   };

   //
   // Script
   //
   class Script
   {
   public:
      explicit Script(Module *module);
      ~Script();

      Module *const module;

      ScriptName name;

      Word argC;
      Word codeIdx;
      Word flags;
      Word locArrC;
      Word locRegC;
      Word type;

      bool flagClient : 1;
      bool flagNet    : 1;
   };
}

#endif//ACSVM__Script_H__


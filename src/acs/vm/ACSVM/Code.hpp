//-----------------------------------------------------------------------------
//
// Copyright (C) 2015 David Hill
//
// See COPYING for license information.
//
//-----------------------------------------------------------------------------
//
// Code classes.
//
//-----------------------------------------------------------------------------

#ifndef ACSVM__Code_H__
#define ACSVM__Code_H__

#include "Types.hpp"


//----------------------------------------------------------------------------|
// Types                                                                      |
//

namespace ACSVM
{
   //
   // Code
   //
   // Internal codes.
   //
   enum class Code
   {
      #define ACSVM_CodeList(name, ...) name,
      #include "CodeList.hpp"

      None
   };

   //
   // CodeACS0
   //
   // ACS0 codes.
   //
   enum class CodeACS0
   {
      #define ACSVM_CodeListACS0(name, idx, ...) name = idx,
      #include "CodeList.hpp"

      None
   };

   //
   // Func
   //
   // Internal CallFunc indexes.
   //
   enum class Func
   {
      #define ACSVM_FuncList(name) name,
      #include "CodeList.hpp"

      None
   };

   //
   // FuncACS0
   //
   // ACS0 CallFunc indexes.
   //
   enum class FuncACS0
   {
      #define ACSVM_FuncListACS0(name, idx, ...) name = idx,
      #include "CodeList.hpp"

      None
   };

   //
   // KillType
   //
   enum class KillType
   {
      None,
      OutOfBounds,
      UnknownCode,
      UnknownFunc,
      BranchLimit,
   };
}

#endif//ACSVM__Code_H__


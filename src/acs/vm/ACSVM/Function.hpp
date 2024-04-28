//-----------------------------------------------------------------------------
//
// Copyright (C) 2015 David Hill
//
// See COPYING for license information.
//
//-----------------------------------------------------------------------------
//
// Function class.
//
//-----------------------------------------------------------------------------

#ifndef ACSVM__Function_H__
#define ACSVM__Function_H__

#include "Types.hpp"


//----------------------------------------------------------------------------|
// Types                                                                      |
//

namespace ACSVM
{
   //
   // Function
   //
   class Function
   {
   public:
      Function(Module *module, String *name, Word idx);
      ~Function();

      Module *module;
      String *name;
      Word    idx;

      Word    argC;
      Word    codeIdx;
      Word    locArrC;
      Word    locRegC;

      bool flagRet : 1;
   };
}

#endif//ACSVM__Function_H__


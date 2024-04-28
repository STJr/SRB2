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

#include "Function.hpp"


//----------------------------------------------------------------------------|
// Extern Functions                                                           |
//

namespace ACSVM
{
   //
   // Function constructor
   //
   Function::Function(Module *module_, String *name_, Word idx_) :
      module{module_},
      name  {name_},
      idx   {idx_},

      argC   {0},
      codeIdx{0},
      locArrC{0},
      locRegC{0},

      flagRet{false}
   {
   }

   //
   // Function destructor
   //
   Function::~Function()
   {
   }
}

// EOF


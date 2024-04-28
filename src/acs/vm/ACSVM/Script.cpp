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

#include "Script.hpp"

#include "Environment.hpp"
#include "Module.hpp"


//----------------------------------------------------------------------------|
// Extern Fumnctions                                                          |
//

namespace ACSVM
{
   //
   // Script constructor
   //
   Script::Script(Module *module_) :
      module{module_},

      name{},

      argC   {0},
      codeIdx{0},
      flags  {0},
      locArrC{0},
      locRegC{module->env->scriptLocRegC},
      type   {0},

      flagClient{false},
      flagNet   {false}
   {
   }

   //
   // Script destructor
   //
   Script::~Script()
   {
   }
}

// EOF


//-----------------------------------------------------------------------------
//
// Copyright (C) 2015 David Hill
//
// See COPYING for license information.
//
//-----------------------------------------------------------------------------
//
// Initializer handling.
//
//-----------------------------------------------------------------------------

#include "Init.hpp"

#include "Array.hpp"
#include "Function.hpp"
#include "Module.hpp"
#include "String.hpp"

#include <vector>


//----------------------------------------------------------------------------|
// Types                                                                      |
//

namespace ACSVM
{
   //
   // ArrayInit::PrivData
   //
   struct ArrayInit::PrivData
   {
      std::vector<WordInit> initV;
   };
}


//----------------------------------------------------------------------------|
// Extern Functions                                                           |
//

namespace ACSVM
{

   //
   // ArrayInit constructor
   //
   ArrayInit::ArrayInit() :
      pd{new PrivData}
   {
   }

   //
   // ArrayInit destructor
   //
   ArrayInit::~ArrayInit()
   {
      delete pd;
   }

   //
   // ArrayInit::apply
   //
   void ArrayInit::apply(Array &arr, Module *module)
   {
      Word idx = 0;
      for(WordInit &init : pd->initV)
      {
         Word value = init.getValue(module);
         if(value) arr[idx] = value;
         ++idx;
      }
   }

   //
   // ArrayInit::finish
   //
   void ArrayInit::finish()
   {
      // Clear out trailing zeroes.
      while(!pd->initV.empty() && !pd->initV.back())
         pd->initV.pop_back();

      // Shrink vector.
      pd->initV.shrink_to_fit();

      // TODO: Break up initialization data into nonzero ranges.
   }

   //
   // ArrayInit::reserve
   //
   void ArrayInit::reserve(Word count)
   {
      pd->initV.resize(count, 0);
   }

   //
   // ArrayInit::setTag
   //
   void ArrayInit::setTag(Word idx, InitTag tag)
   {
      if(idx >= pd->initV.size())
         pd->initV.resize(idx + 1, 0);

      pd->initV[idx].tag = tag;
   }

   //
   // ArrayInit::setVal
   //
   void ArrayInit::setVal(Word idx, Word val)
   {
      if(idx >= pd->initV.size())
         pd->initV.resize(idx + 1, 0);

      pd->initV[idx].val = val;
   }

   //
   // WordInit::getValue
   //
   Word WordInit::getValue(Module *module) const
   {
      switch(tag)
      {
      case InitTag::Integer:
         return val;

      case InitTag::Function:
         if(val < module->functionV.size() && module->functionV[val])
            return module->functionV[val]->idx;
         else
            return val;

      case InitTag::String:
         if(val < module->stringV.size())
            return ~module->stringV[val]->idx;
         else
            return val;
      }

      return val;
   }
}

// EOF


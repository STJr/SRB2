//-----------------------------------------------------------------------------
//
// Copyright (C) 2015 David Hill
//
// See COPYING for license information.
//
//-----------------------------------------------------------------------------
//
// Module class.
//
//-----------------------------------------------------------------------------

#include "Module.hpp"

#include "Array.hpp"
#include "Environment.hpp"
#include "Function.hpp"
#include "Init.hpp"
#include "Jump.hpp"
#include "Script.hpp"


//----------------------------------------------------------------------------|
// Extern Functions                                                           |
//

namespace ACSVM
{
   //
   // ModuleName::hash
   //
   std::size_t ModuleName::hash() const
   {
      return s->hash + std::hash<void*>()(p) + i;
   }

   //
   // Module constructor
   //
   Module::Module(Environment *env_, ModuleName const &name_) :
      env{env_},
      name{name_},

      hashLink{this},

      isACS0{false},
      loaded{false}
   {
   }

   //
   // Module destructor
   //
   Module::~Module()
   {
      reset();
   }

   //
   // Module::refStrings
   //
   void Module::refStrings() const
   {
      if(name.s) name.s->ref = true;

      for(auto &s : arrImpV)   if(s) s->ref = true;
      for(auto &s : arrNameV)  if(s) s->ref = true;
      for(auto &s : funcNameV) if(s) s->ref = true;
      for(auto &s : regImpV)   if(s) s->ref = true;
      for(auto &s : regNameV)  if(s) s->ref = true;
      for(auto &s : scrNameV)  if(s) s->ref = true;
      for(auto &s : stringV)   if(s) s->ref = true;

      for(auto &func : functionV)
         if(func && func->name) func->name->ref = true;

      for(auto &scr : scriptV)
         if(scr.name.s) scr.name.s->ref = true;
   }

   //
   // Module::reset
   //
   void Module::reset()
   {
      // Unload locally defined functions from env.
      for(Function *&func : functionV)
      {
         if(func && func->module == this)
            env->freeFunction(func);
      }

      arrImpV.free();
      arrInitV.free();
      arrNameV.free();
      arrSizeV.free();
      codeV.free();
      funcNameV.free();
      functionV.free();
      importV.free();
      jumpV.free();
      jumpMapV.free();
      regImpV.free();
      regInitV.free();
      regNameV.free();
      scrNameV.free();
      scriptV.free();
      stringV.free();

      isACS0 = false;
      loaded = false;
   }

   //
   // Module::resetStrings
   //
   void Module::resetStrings()
   {
      name.s = env->getString(name.s);

      for(auto &s : arrImpV)   s = env->getString(s);
      for(auto &s : arrNameV)  s = env->getString(s);
      for(auto &s : funcNameV) s = env->getString(s);
      for(auto &s : regImpV)   s = env->getString(s);
      for(auto &s : regNameV)  s = env->getString(s);
      for(auto &s : scrNameV)  s = env->getString(s);
      for(auto &s : stringV)   s = env->getString(s);

      for(auto &func : functionV)
         if(func) func->name = env->getString(func->name);

      for(auto &scr : scriptV)
         scr.name.s = env->getString(scr.name.s);
   }
}


// EOF


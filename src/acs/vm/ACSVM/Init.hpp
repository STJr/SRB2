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

#ifndef ACSVM__Init_H__
#define ACSVM__Init_H__

#include "Types.hpp"


//----------------------------------------------------------------------------|
// Types                                                                      |
//

namespace ACSVM
{
   //
   // InitTag
   //
   enum class InitTag
   {
      Integer,
      Function,
      String,
   };

   //
   // ArrayInit
   //
   class ArrayInit
   {
   public:
      ArrayInit();
      ~ArrayInit();

      void apply(Array &arr, Module *module);

      void finish();

      void reserve(Word count);

      void setTag(Word idx, InitTag tag);
      void setVal(Word idx, Word    val);

   private:
      struct PrivData;

      PrivData *pd;
   };

   //
   // WordInit
   //
   class WordInit
   {
   public:
      WordInit() = default;
      WordInit(Word val_) : val{val_}, tag{InitTag::Integer} {}
      WordInit(Word val_, InitTag tag_) : val{val_}, tag{tag_} {}

      explicit operator bool () const
         {return val || tag != InitTag::Integer;}

      Word getValue(Module *module) const;

      Word    val;
      InitTag tag;
   };
}

#endif//ACSVM__Init_H__


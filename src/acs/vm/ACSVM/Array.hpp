//-----------------------------------------------------------------------------
//
// Copyright (C) 2015-2017 David Hill
//
// See COPYING for license information.
//
//-----------------------------------------------------------------------------
//
// Array class.
//
//-----------------------------------------------------------------------------

#ifndef ACSVM__Array_H__
#define ACSVM__Array_H__

#include "Types.hpp"


//----------------------------------------------------------------------------|
// Types                                                                      |
//

namespace ACSVM
{
   //
   // Array
   //
   // Sparse-allocation array of 2**32 Words.
   //
   class Array
   {
   public:
      Array() : data{nullptr} {}
      Array(Array const &) = delete;
      Array(Array &&array) : data{array.data} {array.data = nullptr;}
      ~Array() {clear();}

      Word &operator [] (Word idx);

      void clear();

      // If idx is allocated, returns that Word. Otherwise, returns 0.
      Word find(Word idx) const;

      void loadState(Serial &in);

      void lockStrings(Environment *env) const;

      void refStrings(Environment *env) const;

      void saveState(Serial &out) const;

      void unlockStrings(Environment *env) const;

   private:
      static constexpr std::size_t PageSize = 256;
      static constexpr std::size_t SegmSize = 256;
      static constexpr std::size_t BankSize = 256;
      static constexpr std::size_t DataSize = 256;

      using Page = Word [PageSize];
      using Segm = Page*[SegmSize];
      using Bank = Segm*[BankSize];
      using Data = Bank*[DataSize];

      Data *data;
   };
}

#endif//ACSVM__Array_H__


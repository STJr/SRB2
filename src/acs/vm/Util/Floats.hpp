//----------------------------------------------------------------------------
//
// Copyright (C) 2015 David Hill
//
// See COPYING for license information.
//
//----------------------------------------------------------------------------
//
// Floating-point utilities.
//
//----------------------------------------------------------------------------

#ifndef ACSVM__Util__Floats_H__
#define ACSVM__Util__Floats_H__

#include "../ACSVM/Types.hpp"

#include <array>
#include <cmath>


//----------------------------------------------------------------------------|
// Extern Functions                                                           |
//

namespace ACSVM
{
   bool CF_AddF_W1(Thread *thread, Word const *argV, Word argC);
   bool CF_DivF_W1(Thread *thread, Word const *argV, Word argC);
   bool CF_MulF_W1(Thread *thread, Word const *argV, Word argC);
   bool CF_SubF_W1(Thread *thread, Word const *argV, Word argC);
   bool CF_AddF_W2(Thread *thread, Word const *argV, Word argC);
   bool CF_DivF_W2(Thread *thread, Word const *argV, Word argC);
   bool CF_MulF_W2(Thread *thread, Word const *argV, Word argC);
   bool CF_SubF_W2(Thread *thread, Word const *argV, Word argC);

   bool CF_PrintDouble(Thread *thread, Word const *argV, Word argC);
   bool CF_PrintFloat(Thread *thread, Word const *argV, Word argC);

   //
   // FloatExpBitDefault
   //
   template<std::size_t N>
   constexpr std::size_t FloatExpBitDefault();

   template<> constexpr std::size_t FloatExpBitDefault<1>() {return  8;}
   template<> constexpr std::size_t FloatExpBitDefault<2>() {return 11;}
   template<> constexpr std::size_t FloatExpBitDefault<4>() {return 15;}

   //
   // FloatToWords
   //
   template<std::size_t N, std::size_t ExpBit = FloatExpBitDefault<N>(), typename FltT>
   std::array<Word, N> FloatToWords(FltT const &f)
   {
      static_assert(N >= 1, "N must be at least 1.");
      static_assert(ExpBit >= 2, "ExpBit must be at least 2.");
      static_assert(ExpBit <= 30, "ExpBit must be at most 30.");


      constexpr int ExpMax = (1 << (ExpBit - 1)) - 1;
      constexpr int ExpMin = -ExpMax - 1;
      constexpr int ExpOff = ExpMax - 1;


      std::array<Word, N> w{};
      Word &wHi = std::get<N - 1>(w);
      Word &wLo = std::get<0>(w);

      // Convert sign.
      bool sigRaw = std::signbit(f);
      wHi = static_cast<Word>(sigRaw) << 31;

      // Convert special values.
      switch(std::fpclassify(f))
      {
      case FP_INFINITE:
         // Convert to +/-INFINITY.
         wHi |= (0xFFFFFFFFu << (31 - ExpBit)) & 0x7FFFFFFF;
         return w;

      case FP_NAN:
         // Convert to NAN.
         wHi |= 0x7FFFFFFF;
         for(auto itr = &wLo, end = &wHi; itr != end; ++itr)
            *itr = 0xFFFFFFFF;
         return w;

      case FP_SUBNORMAL:
         // TODO: Subnormals.
      case FP_ZERO:
         // Convert to +/-0.
         return w;
      }

      int  expRaw = 0;
      FltT manRaw = std::ldexp(std::frexp(std::fabs(f), &expRaw), N * 32 - ExpBit);

      // Check for exponent overflow.
      if(expRaw > ExpMax)
      {
         // Overflow to +/-INFINITY.
         wHi |= (0xFFFFFFFFu << (32 - ExpBit)) & 0x7FFFFFFF;
         return w;
      }

      // Check for exponent underflow.
      if(expRaw < ExpMin)
      {
         // Underflow to +/-0.
         return w;
      }

      // Convert exponent.
      wHi |= static_cast<Word>(expRaw + ExpOff) << (32 - ExpBit - 1);

      // Convert mantissa.
      for(int i = 0, e = N - 1; i != e; ++i)
         w[i] = static_cast<Word>(std::fmod(std::ldexp(manRaw, -i * 32), 4294967296.0));
      wHi |= static_cast<Word>(std::ldexp(manRaw, -static_cast<int>(N - 1) * 32))
         & ~(0xFFFFFFFFu << (31 - ExpBit));

      return w;
   }

   //
   // WordsToFloat
   //
   template<typename FltT, std::size_t N, std::size_t ExpBit = FloatExpBitDefault<N>()>
   FltT WordsToFloat(std::array<Word, N> const &w)
   {
      static_assert(N >= 1, "N must be at least 1.");
      static_assert(ExpBit >= 2, "ExpBit must be at least 2.");
      static_assert(ExpBit <= 30, "ExpBit must be at most 30.");


      constexpr int ExpMax = (1 << (ExpBit - 1)) - 1;
      constexpr int ExpMin = -ExpMax - 1;
      constexpr int ExpOff = ExpMax;

      constexpr Word ManMask = 0xFFFFFFFFu >> (ExpBit + 1);


      Word const &wHi = std::get<N - 1>(w);
      Word const &wLo = std::get<0>(w);

      bool sig = !!(wHi & 0x80000000);
      int  exp = static_cast<int>((wHi & 0x7FFFFFFF) >> (31 - ExpBit)) - ExpOff;

      // INFINITY or NAN.
      if(exp > ExpMax)
      {
         // Check for NAN.
         for(auto itr = &wLo, end = &wHi; itr != end; ++itr)
            if(*itr) return NAN;
         if(wHi & ManMask) return NAN;

         return sig ? -INFINITY : +INFINITY;
      }

      // Zero or subnormal.
      if(exp < ExpMin)
      {
         // TODO: Subnormals.

         return sig ? -0.0f : +0.0f;
      }

      // Convert mantissa.
      FltT f = 0;
      for(auto itr = &wHi, end = &wLo; itr != end;)
         f = ldexp(f + *--itr, -32);
      f = ldexp(f + (wHi & ManMask) + (ManMask + 1), -static_cast<int>(31 - ExpBit));

      // Convert exponent.
      f = ldexp(f, exp);

      // Convert sign.
      f = sig ? -f : +f;

      return f;
   }
}

#endif//ACSVM__Util__Floats_H__


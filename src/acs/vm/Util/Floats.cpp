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

#include "Floats.hpp"

#include "ACSVM/Thread.hpp"

#include <cstdio>


//----------------------------------------------------------------------------|
// Macros                                                                     |
//

//
// DoubleOp
//
#define DoubleOp(name, op) \
   bool CF_##name##F_W2(Thread *thread, Word const *argV, Word) \
   { \
      double l = WordsToFloat<double, 2>({{argV[0], argV[1]}}); \
      double r = WordsToFloat<double, 2>({{argV[2], argV[3]}}); \
      for(auto w : FloatToWords<2>(l op r)) thread->dataStk.push(w); \
      return false; \
   }

//
// FloatOp
//
#define FloatOp(name, op) \
   bool CF_##name##F_W1(Thread *thread, Word const *argV, Word) \
   { \
      float l = WordsToFloat<float, 1>({{argV[0]}}); \
      float r = WordsToFloat<float, 1>({{argV[1]}}); \
      for(auto w : FloatToWords<1>(l op r)) thread->dataStk.push(w); \
      return false; \
   }


//----------------------------------------------------------------------------|
// Extern Functions                                                           |
//

namespace ACSVM
{
   FloatOp(Add, +);
   FloatOp(Div, /);
   FloatOp(Mul, *);
   FloatOp(Sub, -);

   DoubleOp(Add, +);
   DoubleOp(Div, /);
   DoubleOp(Mul, *);
   DoubleOp(Sub, -);

   //
   // void PrintDouble(double f)
   //
   bool CF_PrintDouble(Thread *thread, Word const *argV, Word)
   {
      double f = WordsToFloat<double, 2>({{argV[0], argV[1]}});
      thread->printBuf.reserve(std::snprintf(nullptr, 0, "%f", f));
      thread->printBuf.format("%f", f);
      return false;
   }

   //
   // void PrintFloat(float f)
   //
   bool CF_PrintFloat(Thread *thread, Word const *argV, Word)
   {
      float f = WordsToFloat<float, 1>({{argV[0]}});
      thread->printBuf.reserve(std::snprintf(nullptr, 0, "%f", f));
      thread->printBuf.format("%f", f);
      return false;
   }
}

// EOF


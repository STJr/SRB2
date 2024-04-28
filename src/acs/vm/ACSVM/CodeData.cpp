//-----------------------------------------------------------------------------
//
// Copyright (C) 2015 David Hill
//
// See COPYING for license information.
//
//-----------------------------------------------------------------------------
//
// CodeData classes.
//
//-----------------------------------------------------------------------------

#include "CodeData.hpp"

#include "Code.hpp"

#include <algorithm>
#include <cstring>


//----------------------------------------------------------------------------|
// Extern Functions                                                           |
//

namespace ACSVM
{
   //
   // CodeDataACS0 constructor
   //
   CodeDataACS0::CodeDataACS0(char const *args_, Code transCode_,
      Word stackArgC_, Word transFunc_) :
      code     {CodeACS0::None},
      args     {args_},
      argc     {CountArgs(args_)},
      stackArgC{stackArgC_},
      transCode{transCode_},
      transFunc{transFunc_}
   {
   }

   //
   // CodeDataACS0 constructor
   //
   CodeDataACS0::CodeDataACS0(char const *args_, Word stackArgC_, Word transFunc_) :
      code     {CodeACS0::None},
      args     {args_},
      argc     {CountArgs(args_)},
      stackArgC{stackArgC_},
      transCode{argc ? Code::CallFunc_Lit : Code::CallFunc},
      transFunc{transFunc_}
   {
   }

   //
   // CodeDataACS0 constructor
   //
   CodeDataACS0::CodeDataACS0(CodeACS0 code_, char const *args_,
      Code transCode_, Word stackArgC_, Func transFunc_) :
      code     {code_},
      args     {args_},
      argc     {CountArgs(args_)},
      stackArgC{stackArgC_},
      transCode{transCode_},
      transFunc{transFunc_ != Func::None ? static_cast<Word>(transFunc_) : 0}
   {
   }

   //
   // CodeDataACS0::CountArgs
   //
   std::size_t CodeDataACS0::CountArgs(char const *args)
   {
      std::size_t argc = 0;

      for(; *args; ++args) switch(*args)
      {
      case 'B':
      case 'H':
      case 'W':
      case 'b':
      case 'h':
         ++argc;
         break;
      }

      return argc;
   }

   //
   // FuncDataACS0 copy constructor
   //
   FuncDataACS0::FuncDataACS0(FuncDataACS0 const &data) :
      transFunc{data.transFunc},

      transCodeV{new TransCode[data.transCodeC]},
      transCodeC{data.transCodeC}
   {
      std::copy(data.transCodeV, data.transCodeV + transCodeC, transCodeV);
   }

   //
   // FuncDataACS0 move constructor
   //
   FuncDataACS0::FuncDataACS0(FuncDataACS0 &&data) :
      transFunc{data.transFunc},

      transCodeV{data.transCodeV},
      transCodeC{data.transCodeC}
   {
      data.transCodeV = nullptr;
      data.transCodeC = 0;
   }

   //
   // FuncDataACS0 constructor
   //
   FuncDataACS0::FuncDataACS0(FuncACS0 func_, Func transFunc_,
      std::initializer_list<TransCode> transCodes) :
      func{func_},

      transFunc{transFunc_ != Func::None ? static_cast<Word>(transFunc_) : 0},

      transCodeV{new TransCode[transCodes.size()]},
      transCodeC{transCodes.size()}
   {
      std::copy(transCodes.begin(), transCodes.end(), transCodeV);
   }

   //
   // FuncDataACS0 constructor
   //
   FuncDataACS0::FuncDataACS0(Word transFunc_) :
      func{FuncACS0::None},

      transFunc{transFunc_},

      transCodeV{nullptr},
      transCodeC{0}
   {
   }

   //
   // FuncDataACS0 constructor
   //
   FuncDataACS0::FuncDataACS0(Word transFunc_,
      std::initializer_list<TransCode> transCodes) :
      func{FuncACS0::None},

      transFunc{transFunc_},

      transCodeV{new TransCode[transCodes.size()]},
      transCodeC{transCodes.size()}
   {
      std::copy(transCodes.begin(), transCodes.end(), transCodeV);
   }

   //
   // FuncDataACS0 constructor
   //
   FuncDataACS0::FuncDataACS0(Word transFunc_,
      std::unique_ptr<TransCode[]> &&transCodeV_, std::size_t transCodeC_) :
      func{FuncACS0::None},

      transFunc{transFunc_},

      transCodeV{transCodeV_.release()},
      transCodeC{transCodeC_}
   {
   }

   //
   // FuncDataACS0 destructor
   //
   FuncDataACS0::~FuncDataACS0()
   {
      delete[] transCodeV;
   }

   //
   // FuncDataACS0::operator = FuncDataACS0
   //
   FuncDataACS0 &FuncDataACS0::operator = (FuncDataACS0 &&data)
   {
      std::swap(transFunc, data.transFunc);

      std::swap(transCodeV, data.transCodeV);
      std::swap(transCodeC, data.transCodeC);

      return *this;
   }

   //
   // FuncDataACS0::getTransCode
   //
   Code FuncDataACS0::getTransCode(Word argc) const
   {
      for(auto itr = transCodeV, end = itr + transCodeC; itr != end; ++itr)
         if(itr->first == argc) return itr->second;

      return Code::CallFunc;
   }
}

// EOF


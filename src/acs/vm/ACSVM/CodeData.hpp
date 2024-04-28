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

#ifndef ACSVM__CodeData_H__
#define ACSVM__CodeData_H__

#include "Types.hpp"

#include <initializer_list>
#include <memory>
#include <utility>


//----------------------------------------------------------------------------|
// Types                                                                      |
//

namespace ACSVM
{
   //
   // CodeData
   //
   // Internal code description.
   //
   class CodeData
   {
   public:
      Code code;

      Word argc;
   };

   //
   // CodeDataACS0
   //
   // ACS0 code description.
   //
   class CodeDataACS0
   {
   public:
      CodeDataACS0(char const *args, Code transCode, Word stackArgC,
         Word transFunc = 0);
      CodeDataACS0(char const *args, Word stackArgC, Word transFunc);
      CodeDataACS0(CodeACS0 code, char const *args, Code transCode,
         Word stackArgC, Func transFunc);

      // Code index. If not an internally recognized code, is set to None.
      CodeACS0 code;

      // String describing the code's arguments.
      //    A - Previous value is MapReg index.
      //    a - Previous Value is MapArr index.
      //    B - Single byte.
      //    b - Single byte if compressed, full word otherwise.
      //    G - Previous value is GblReg index.
      //    g - Previous Value is GblArr index.
      //    H - Half word.
      //    h - Half word if compressed, full word otherwise.
      //    J - Previous value is jump index.
      //    L - Previous value is LocReg index.
      //    l - Previous Value is LocArr index.
      //    O - Previous value is ModReg index.
      //    o - Previous Value is ModArr index.
      //    S - Previous value is string index.
      //    U - Previous value is HubReg index.
      //    u - Previous Value is HubArr index.
      //    W - Full word.
      char const *args;
      std::size_t argc;

      // Stack argument count.
      Word stackArgC;

      // Internal code to translate to.
      Code transCode;

      // CallFunc index to translate to.
      Word transFunc;

   private:
      static std::size_t CountArgs(char const *args);
   };

   //
   // FuncDataACS0
   //
   // ACS0 CallFunc description.
   //
   class FuncDataACS0
   {
   public:
      using TransCode = std::pair<Word, Code>;

      FuncDataACS0(FuncDataACS0 const &);
      FuncDataACS0(FuncDataACS0 &&data);
      FuncDataACS0(FuncACS0 func, Func transFunc,
         std::initializer_list<TransCode> transCodes);
      FuncDataACS0(Word transFunc);
      FuncDataACS0(Word transFunc, std::initializer_list<TransCode> transCodes);
      FuncDataACS0(Word transFunc, std::unique_ptr<TransCode[]> &&transCodeV,
         std::size_t transCodeC);
      ~FuncDataACS0();

      FuncDataACS0 &operator = (FuncDataACS0 const &) = delete;
      FuncDataACS0 &operator = (FuncDataACS0 &&data);

      // Internal code to translate to.
      Code getTransCode(Word argc) const;

      // CallFunc index. If not internally recognized, is set to None.
      FuncACS0 func;

      // CallFunc index to translate to.
      Word transFunc;

   private:
      TransCode  *transCodeV;
      std::size_t transCodeC;
   };
}

#endif//ACSVM__CodeData_H__


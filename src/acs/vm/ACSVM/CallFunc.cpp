//-----------------------------------------------------------------------------
//
// Copyright (C) 2015 David Hill
//
// See COPYING for license information.
//
//-----------------------------------------------------------------------------
//
// Internal CallFunc functions.
//
//-----------------------------------------------------------------------------

#include "CallFunc.hpp"

#include "Action.hpp"
#include "Code.hpp"
#include "Environment.hpp"
#include "Module.hpp"
#include "Scope.hpp"
#include "Thread.hpp"

#include <cctype>
#include <cinttypes>


//----------------------------------------------------------------------------|
// Static Functions                                                           |
//

namespace ACSVM
{
   //
   // PrintArray
   //
   static void PrintArray(Thread *thread, Word const *argv, Word argc, Array const &arr)
   {
      Word idx = argv[0] + (argc > 2 ? argv[2] : 0);
      Word len = argc > 3 ? argv[3] : -1;

      thread->env->printArray(thread->printBuf, arr, idx, len);
   }

   //
   // StrCaseCmp
   //
   static int StrCaseCmp(String *l, String *r, Word n)
   {
      for(char const *ls = l->str, *rs = r->str;; ++ls, ++rs)
      {
         char lc = std::toupper(*ls), rc = std::toupper(*rs);
         if(lc != rc) return lc < rc ? -1 : 1;
         if(!lc || !n--) return 0;
      }
   }

   //
   // StrCmp
   //
   static int StrCmp(String *l, String *r, Word n)
   {
      for(char const *ls = l->str, *rs = r->str;; ++ls, ++rs)
      {
         char lc = *ls, rc = *rs;
         if(lc != rc) return lc < rc ? -1 : 1;
         if(!lc || !n--) return 0;
      }
   }

   //
   // StrCpyArray
   //
   static bool StrCpyArray(Thread *thread, Word const *argv, Array &dst)
   {
      Word    dstOff = argv[0] + argv[2];
      Word    dstLen = argv[3];
      String *src = thread->scopeMap->getString(argv[4]);
      Word    srcIdx = argv[5];

      if(srcIdx > src->len) return false;

      for(Word dstIdx = dstOff;;)
      {
         if(dstIdx - dstOff == dstLen) return false;
         if(!(dst[dstIdx++] = src->str[srcIdx++])) return true;
      }
   }
}


//----------------------------------------------------------------------------|
// Extern Functions                                                           |
//

namespace ACSVM
{
   //
   // void Nop()
   //
   bool CallFunc_Func_Nop(Thread *, Word const *, Word)
   {
      return false;
   }

   //
   // [[noreturn]] void Kill()
   //
   bool CallFunc_Func_Kill(Thread *thread, Word const *, Word)
   {
      thread->env->printKill(thread, static_cast<Word>(KillType::UnknownFunc), 0);
      thread->state = ThreadState::Stopped;
      return true;
   }

   //======================================================
   // Printing Functions
   //

   //
   // void PrintChar(char c)
   //
   bool CallFunc_Func_PrintChar(Thread *thread, Word const *argv, Word)
   {
      thread->printBuf.reserve(1);
      thread->printBuf.put(static_cast<char>(argv[0]));
      return false;
   }

   //
   // str PrintEndStr()
   //
   bool CallFunc_Func_PrintEndStr(Thread *thread, Word const *, Word)
   {
      char const *data = thread->printBuf.data();
      std::size_t size = thread->printBuf.size();
      String     *str  = thread->env->getString(data, size);
      thread->printBuf.drop();
      thread->dataStk.push(~str->idx);
      return false;
   }

   //
   // void PrintFixD(fixed d)
   //
   bool CallFunc_Func_PrintFixD(Thread *thread, Word const *argv, Word)
   {
      // %E worst case: -3.276800e+04 == 13
      // %F worst case: -32767.999985 == 13
      // %G worst case: -1.52588e-05  == 12
      // %G should be maximally P+6 + extra exponent digits.
      thread->printBuf.reserve(12);
      thread->printBuf.format("%G", static_cast<std::int32_t>(argv[0]) / 65536.0);
      return false;
   }

   //
   // void PrintGblArr(int idx, int arr, int off = 0, int len = -1)
   //
   bool CallFunc_Func_PrintGblArr(Thread *thread, Word const *argv, Word argc)
   {
      PrintArray(thread, argv, argc, thread->scopeGbl->arrV[argv[1]]);
      return false;
   }

   //
   // void PrintHubArr(int idx, int arr, int off = 0, int len = -1)
   //
   bool CallFunc_Func_PrintHubArr(Thread *thread, Word const *argv, Word argc)
   {
      PrintArray(thread, argv, argc, thread->scopeHub->arrV[argv[1]]);
      return false;
   }

   //
   // void PrintIntB(int b)
   //
   bool CallFunc_Func_PrintIntB(Thread *thread, Word const *argv, Word)
   {
      // %B worst case: 11111111111111111111111111111111 == 32
      char buf[32], *end = buf+32, *itr = end;
      for(Word b = argv[0]; b; b >>= 1) *--itr = '0' + (b & 1);
      thread->printBuf.reserve(end - itr);
      thread->printBuf.put(itr, end - itr);
      return false;
   }

   //
   // void PrintIntD(int d)
   //
   bool CallFunc_Func_PrintIntD(Thread *thread, Word const *argv, Word)
   {
      // %d worst case: -2147483648 == 11
      thread->printBuf.reserve(11);
      thread->printBuf.format("%" PRId32, static_cast<std::int32_t>(argv[0]));
      return false;
   }

   //
   // void PrintIntX(int x)
   //
   bool CallFunc_Func_PrintIntX(Thread *thread, Word const *argv, Word)
   {
      // %d worst case: FFFFFFFF == 8
      thread->printBuf.reserve(8);
      thread->printBuf.format("%" PRIX32, static_cast<std::uint32_t>(argv[0]));
      return false;
   }

   //
   // void PrintLocArr(int idx, int arr, int off = 0, int len = -1)
   //
   bool CallFunc_Func_PrintLocArr(Thread *thread, Word const *argv, Word argc)
   {
      PrintArray(thread, argv, argc, thread->localArr[argv[1]]);
      return false;
   }

   //
   // void PrintModArr(int idx, int arr, int off = 0, int len = -1)
   //
   bool CallFunc_Func_PrintModArr(Thread *thread, Word const *argv, Word argc)
   {
      PrintArray(thread, argv, argc, *thread->scopeMod->arrV[argv[1]]);
      return false;
   }

   //
   // void PrintPush()
   //
   bool CallFunc_Func_PrintPush(Thread *thread, Word const *, Word)
   {
      thread->printBuf.push();
      return false;
   }

   //
   // void PrintString(str s)
   //
   bool CallFunc_Func_PrintString(Thread *thread, Word const *argv, Word)
   {
      String *s = thread->scopeMap->getString(argv[0]);
      thread->printBuf.reserve(s->len0);
      thread->printBuf.put(s->str, s->len0);
      return false;
   }

   //======================================================
   // Script Functions
   //

   //
   // int ScrPauseS(str name, int map)
   //
   bool CallFunc_Func_ScrPauseS(Thread *thread, Word const *argV, Word)
   {
      String *name = thread->scopeMap->getString(argV[0]);
      ScopeID scope{thread->scopeGbl->id, thread->scopeHub->id, argV[1]};
      if(!scope.map) scope.map = thread->scopeMap->id;

      thread->dataStk.push(thread->scopeMap->scriptPause(name, scope));
      return false;
   }

   //
   // int ScrStartS(str name, int map, ...)
   //
   bool CallFunc_Func_ScrStartS(Thread *thread, Word const *argV, Word argC)
   {
      String *name = thread->scopeMap->getString(argV[0]);
      ScopeID scope{thread->scopeGbl->id, thread->scopeHub->id, argV[1]};
      if(!scope.map) scope.map = thread->scopeMap->id;

      thread->dataStk.push(thread->scopeMap->scriptStart(name, scope, {argV+2, argC-2}));
      return false;
   }

   //
   // int ScrStartSD(str name, int map, int arg0, int arg1, int lock)
   //
   bool CallFunc_Func_ScrStartSD(Thread *thread, Word const *argV, Word)
   {
      if(!thread->env->checkLock(thread, argV[4], true))
      {
         thread->dataStk.push(0);
         return false;
      }

      return CallFunc_Func_ScrStartS(thread, argV, 4);
   }

   //
   // int ScrStartSF(str name, int map, ...)
   //
   bool CallFunc_Func_ScrStartSF(Thread *thread, Word const *argV, Word argC)
   {
      String *name = thread->scopeMap->getString(argV[0]);
      ScopeID scope{thread->scopeGbl->id, thread->scopeHub->id, argV[1]};
      if(!scope.map) scope.map = thread->scopeMap->id;

      thread->dataStk.push(thread->scopeMap->scriptStartForced(name, scope, {argV+2, argC-2}));
      return false;
   }

   //
   // int ScrStartSL(str name, int map, int arg0, int arg1, int lock)
   //
   bool CallFunc_Func_ScrStartSL(Thread *thread, Word const *argV, Word)
   {
      if(!thread->env->checkLock(thread, argV[4], false))
      {
         thread->dataStk.push(0);
         return false;
      }

      return CallFunc_Func_ScrStartS(thread, argV, 4);
   }

   //
   // int ScrStartSR(str name, ...)
   //
   bool CallFunc_Func_ScrStartSR(Thread *thread, Word const *argV, Word argC)
   {
      String *name = thread->scopeMap->getString(argV[0]);

      thread->dataStk.push(thread->scopeMap->scriptStartResult(name, {argV+1, argC-1}));
      return false;
   }

   //
   // int ScrStopS(str name, int map)
   //
   bool CallFunc_Func_ScrStopS(Thread *thread, Word const *argV, Word)
   {
      String *name = thread->scopeMap->getString(argV[0]);
      ScopeID scope{thread->scopeGbl->id, thread->scopeHub->id, argV[1]};
      if(!scope.map) scope.map = thread->scopeMap->id;

      thread->dataStk.push(thread->scopeMap->scriptStop(name, scope));
      return false;
   }

   //======================================================
   // String Functions
   //

   //
   // int GetChar(str s, int i)
   //
   bool CallFunc_Func_GetChar(Thread *thread, Word const *argv, Word)
   {
      thread->dataStk.push(thread->scopeMap->getString(argv[0])->get(argv[1]));
      return false;
   }

   //
   // int StrCaseCmp(str l, str r, int n = -1)
   //
   bool CallFunc_Func_StrCaseCmp(Thread *thread, Word const *argv, Word argc)
   {
      String *l = thread->scopeMap->getString(argv[0]);
      String *r = thread->scopeMap->getString(argv[1]);
      Word    n = argc > 2 ? argv[2] : -1;

      thread->dataStk.push(StrCaseCmp(l, r, n));
      return false;
   }

   //
   // int StrCmp(str l, str r, int n = -1)
   //
   bool CallFunc_Func_StrCmp(Thread *thread, Word const *argv, Word argc)
   {
      String *l = thread->scopeMap->getString(argv[0]);
      String *r = thread->scopeMap->getString(argv[1]);
      Word    n = argc > 2 ? argv[2] : -1;

      thread->dataStk.push(StrCmp(l, r, n));
      return false;
   }

   //
   // int StrCpyGblArr(int idx, int dst, int dstOff, int dstLen, str src, int srcOff)
   //
   bool CallFunc_Func_StrCpyGblArr(Thread *thread, Word const *argv, Word)
   {
      thread->dataStk.push(StrCpyArray(thread, argv, thread->scopeGbl->arrV[argv[1]]));
      return false;
   }

   //
   // int StrCpyHubArr(int idx, int dst, int dstOff, int dstLen, str src, int srcOff)
   //
   bool CallFunc_Func_StrCpyHubArr(Thread *thread, Word const *argv, Word)
   {
      thread->dataStk.push(StrCpyArray(thread, argv, thread->scopeHub->arrV[argv[1]]));
      return false;
   }

   //
   // int StrCpyLocArr(int idx, int dst, int dstOff, int dstLen, str src, int srcOff)
   //
   bool CallFunc_Func_StrCpyLocArr(Thread *thread, Word const *argv, Word)
   {
      thread->dataStk.push(StrCpyArray(thread, argv, thread->localArr[argv[1]]));
      return false;
   }

   //
   // int StrCpyModArr(int idx, int dst, int dstOff, int dstLen, str src, int srcOff)
   //
   bool CallFunc_Func_StrCpyModArr(Thread *thread, Word const *argv, Word)
   {
      thread->dataStk.push(StrCpyArray(thread, argv, *thread->scopeMod->arrV[argv[1]]));
      return false;
   }

   //
   // str StrLeft(str s, int len)
   //
   bool CallFunc_Func_StrLeft(Thread *thread, Word const *argv, Word)
   {
      String *str = thread->scopeMap->getString(argv[0]);
      Word    len = argv[1];

      if(len < str->len)
         str = thread->env->getString(str->str, len);

      thread->dataStk.push(~str->idx);
      return false;
   }

   //
   // int StrLen(str s)
   //
   bool CallFunc_Func_StrLen(Thread *thread, Word const *argv, Word)
   {
      thread->dataStk.push(thread->scopeMap->getString(argv[0])->len0);
      return false;
   }

   //
   // str StrMid(str s, int idx, int len)
   //
   bool CallFunc_Func_StrMid(Thread *thread, Word const *argv, Word)
   {
      String *str = thread->scopeMap->getString(argv[0]);
      Word    idx = argv[1];
      Word    len = argv[2];

      if(idx < str->len)
      {
         if(len < str->len - idx)
            str = thread->env->getString(str->str + idx, len);
         else
            str = thread->env->getString(str->str + idx, str->len - idx);
      }
      else
         str = thread->env->getString("", static_cast<std::size_t>(0));

      thread->dataStk.push(~str->idx);
      return false;
   }

   //
   // str StrRight(str s, int len)
   //
   bool CallFunc_Func_StrRight(Thread *thread, Word const *argv, Word)
   {
      String *str = thread->scopeMap->getString(argv[0]);
      Word    len = argv[1];

      if(len < str->len)
         str = thread->env->getString(str->str + str->len - len, len);

      thread->dataStk.push(~str->idx);
      return false;
   }
}

// EOF


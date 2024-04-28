//-----------------------------------------------------------------------------
//
// Copyright (C) 2015-2017 David Hill
//
// See COPYING for license information.
//
//-----------------------------------------------------------------------------
//
// Thread execution.
//
//-----------------------------------------------------------------------------

#include "Thread.hpp"

#include "Array.hpp"
#include "Code.hpp"
#include "Environment.hpp"
#include "Function.hpp"
#include "Jump.hpp"
#include "Module.hpp"
#include "Scope.hpp"
#include "Script.hpp"


//----------------------------------------------------------------------------|
// Macros                                                                     |
//

//
// ACSVM_DynamicGoto
//
// If nonzero, enables use of dynamic goto labels in core interpreter loop.
// Currently, only gcc syntax is supported.
//
#ifndef ACSVM_DynamicGoto
#if defined(__GNUC__)
#define ACSVM_DynamicGoto 1
#else
#define ACSVM_DynamicGoto 0
#endif
#endif

//
// BranchTo
//
#define BranchTo(target) \
   do \
   { \
      codePtr = &module->codeV[(target)]; \
      CountBranch(); \
   } \
   while(0)

//
// CountBranch
//
// Used to limit the number of branches to prevent infinite no-delay loops.
//
#define CountBranch() \
   if(branches && !--branches) \
   { \
      env->printKill(this, static_cast<Word>(KillType::BranchLimit), 0); \
      goto thread_stop; \
   } \
   else \
      ((void)0)

//
// DeclCase
//
#if ACSVM_DynamicGoto
#define DeclCase(name) case_Code##name
#else
#define DeclCase(name) case static_cast<Word>(Code::name)
#endif

//
// NextCase
//
#if ACSVM_DynamicGoto
#define NextCase() goto *cases[*codePtr++]
#else
#define NextCase() goto next_case
#endif

//
// Op_*
//

#define Op_AddU(lop) (dataStk.drop(), (lop) += dataStk[0])
#define Op_AndU(lop) (dataStk.drop(), (lop) &= dataStk[0])
#define Op_CmpI_GE(lop) (dataStk.drop(), OpFunc_CmpI_GE(lop, dataStk[0]))
#define Op_CmpI_GT(lop) (dataStk.drop(), OpFunc_CmpI_GT(lop, dataStk[0]))
#define Op_CmpI_LE(lop) (dataStk.drop(), OpFunc_CmpI_LE(lop, dataStk[0]))
#define Op_CmpI_LT(lop) (dataStk.drop(), OpFunc_CmpI_LT(lop, dataStk[0]))
#define Op_CmpU_EQ(lop) (dataStk.drop(), OpFunc_CmpU_EQ(lop, dataStk[0]))
#define Op_CmpU_NE(lop) (dataStk.drop(), OpFunc_CmpU_NE(lop, dataStk[0]))
#define Op_DecU(lop) (--(lop))
#define Op_DivI(lop) (dataStk.drop(), OpFunc_DivI(lop, dataStk[0]))
#define Op_DivX(lop) (dataStk.drop(), OpFunc_DivX(lop, dataStk[0]))
#define Op_Drop(lop) (dataStk.drop(), (lop) = dataStk[0])
#define Op_IncU(lop) (++(lop))
#define Op_LAnd(lop) (dataStk.drop(), OpFunc_LAnd(lop, dataStk[0]))
#define Op_LOrI(lop) (dataStk.drop(), OpFunc_LOrI(lop, dataStk[0]))
#define Op_ModI(lop) (dataStk.drop(), OpFunc_ModI(lop, dataStk[0]))
#define Op_MulU(lop) (dataStk.drop(), (lop) *= dataStk[0])
#define Op_MulX(lop) (dataStk.drop(), OpFunc_MulX(lop, dataStk[0]))
#define Op_OrIU(lop) (dataStk.drop(), (lop) |= dataStk[0])
#define Op_OrXU(lop) (dataStk.drop(), (lop) ^= dataStk[0])
#define Op_ShLU(lop) (dataStk.drop(), (lop) <<= dataStk[0] & 31)
#define Op_ShRI(lop) (dataStk.drop(), OpFunc_ShRI(lop, dataStk[0]))
#define Op_SubU(lop) (dataStk.drop(), (lop) -= dataStk[0])

//
// OpSet
//
#define OpSet(op) \
   DeclCase(op##_GblArr): \
      Op_##op(scopeGbl->arrV[*codePtr++][dataStk[1]]); dataStk.drop(); \
      NextCase(); \
   DeclCase(op##_GblReg): \
      Op_##op(scopeGbl->regV[*codePtr++]); \
      NextCase(); \
   DeclCase(op##_HubArr): \
      Op_##op(scopeHub->arrV[*codePtr++][dataStk[1]]); dataStk.drop(); \
      NextCase(); \
   DeclCase(op##_HubReg): \
      Op_##op(scopeHub->regV[*codePtr++]); \
      NextCase(); \
   DeclCase(op##_LocArr): \
      Op_##op(localArr[*codePtr++][dataStk[1]]); dataStk.drop(); \
      NextCase(); \
   DeclCase(op##_LocReg): \
      Op_##op(localReg[*codePtr++]); \
      NextCase(); \
   DeclCase(op##_ModArr): \
      Op_##op((*scopeMod->arrV[*codePtr++])[dataStk[1]]); dataStk.drop(); \
      NextCase(); \
   DeclCase(op##_ModReg): \
      Op_##op(*scopeMod->regV[*codePtr++]); \
      NextCase()


//----------------------------------------------------------------------------|
// Static Functions                                                           |
//

namespace ACSVM
{
   //
   // OpFunc_CmpI_GE
   //
   static inline void OpFunc_CmpI_GE(Word &lop, Word rop)
   {
      lop = static_cast<SWord>(lop) >= static_cast<SWord>(rop);
   }

   //
   // OpFunc_CmpI_GT
   //
   static inline void OpFunc_CmpI_GT(Word &lop, Word rop)
   {
      lop = static_cast<SWord>(lop) > static_cast<SWord>(rop);
   }

   //
   // OpFunc_CmpI_LE
   //
   static inline void OpFunc_CmpI_LE(Word &lop, Word rop)
   {
      lop = static_cast<SWord>(lop) <= static_cast<SWord>(rop);
   }

   //
   // OpFunc_CmpI_LT
   //
   static inline void OpFunc_CmpI_LT(Word &lop, Word rop)
   {
      lop = static_cast<SWord>(lop) < static_cast<SWord>(rop);
   }

   //
   // OpFunc_CmpU_EQ
   //
   static inline void OpFunc_CmpU_EQ(Word &lop, Word rop)
   {
      lop = lop == rop;
   }

   //
   // OpFunc_CmpU_NE
   //
   static inline void OpFunc_CmpU_NE(Word &lop, Word rop)
   {
      lop = lop != rop;
   }

   //
   // OpFunc_DivI
   //
   static inline void OpFunc_DivI(Word &lop, Word rop)
   {
      lop = rop ? static_cast<SWord>(lop) / static_cast<SWord>(rop) : 0;
   }

   //
   // OpFunc_DivX
   //
   static inline void OpFunc_DivX(Word &lop, Word rop)
   {
      if(rop)
         lop = (SDWord(SWord(lop)) << 16) / SWord(rop);
      else
         lop = 0;
   }

   //
   // OpFunc_LAnd
   //
   static inline void OpFunc_LAnd(Word &lop, Word rop)
   {
      lop = lop && rop;
   }

   //
   // OpFunc_LOrI
   //
   static inline void OpFunc_LOrI(Word &lop, Word rop)
   {
      lop = lop || rop;
   }

   //
   // OpFunc_ModI
   //
   static inline void OpFunc_ModI(Word &lop, Word rop)
   {
      lop = rop ? static_cast<SWord>(lop) % static_cast<SWord>(rop) : 0;
   }

   //
   // OpFunc_MulX
   //
   static inline void OpFunc_MulX(Word &lop, Word rop)
   {
      lop = DWord(SDWord(SWord(lop)) * SWord(rop)) >> 16;
   }

   //
   // OpFunc_ShRI
   //
   static inline void OpFunc_ShRI(Word &lop, Word rop)
   {
      // TODO: Implement this without relying on sign-extending shift.
      lop = static_cast<SWord>(lop) >> (rop & 31);
   }
}


//----------------------------------------------------------------------------|
// Extern Functions                                                           |
//

namespace ACSVM
{
   //
   // Thread::exec
   //
   void Thread::exec()
   {
      if(delay && --delay)
         return;

      auto branches = env->branchLimit;

   exec_intr:
      switch(state.state)
      {
      case ThreadState::Inactive: return;
      case ThreadState::Stopped:  goto thread_stop;
      case ThreadState::Paused:   return;

      case ThreadState::Running:
         if(delay)
            return;
         break;

      case ThreadState::WaitScrI:
         if(scopeMap->isScriptActive(scopeMap->findScript(state.data)))
            return;
         state = ThreadState::Running;
         break;

      case ThreadState::WaitScrS:
         if(scopeMap->isScriptActive(scopeMap->findScript(scopeMap->getString(state.data))))
            return;
         state = ThreadState::Running;
         break;

      case ThreadState::WaitTag:
         if(!module->env->checkTag(state.type, state.data))
            return;
         state = ThreadState::Running;
         break;
      }

      #if ACSVM_DynamicGoto
      static void const *const cases[] =
      {
         #define ACSVM_CodeList(name, ...) &&case_Code##name,
         #include "CodeList.hpp"
      };
      #endif

      #if ACSVM_DynamicGoto
      NextCase();
      #else
      next_case: switch(*codePtr++)
      #endif
      {
      DeclCase(Nop):
         NextCase();

      DeclCase(Kill):
         module->env->printKill(this, codePtr[0], codePtr[1]);
         goto thread_stop;

         //================================================
         // Binary operator codes.
         //

         OpSet(AddU);
         OpSet(AndU);
         OpSet(DivI);
         OpSet(ModI);
         OpSet(MulU);
         OpSet(OrIU);
         OpSet(OrXU);
         OpSet(ShLU);
         OpSet(ShRI);
         OpSet(SubU);

      DeclCase(AddU): Op_AddU(dataStk[1]); NextCase();
      DeclCase(AndU): Op_AndU(dataStk[1]); NextCase();
      DeclCase(CmpI_GE): Op_CmpI_GE(dataStk[1]); NextCase();
      DeclCase(CmpI_GT): Op_CmpI_GT(dataStk[1]); NextCase();
      DeclCase(CmpI_LE): Op_CmpI_LE(dataStk[1]); NextCase();
      DeclCase(CmpI_LT): Op_CmpI_LT(dataStk[1]); NextCase();
      DeclCase(CmpU_EQ): Op_CmpU_EQ(dataStk[1]); NextCase();
      DeclCase(CmpU_NE): Op_CmpU_NE(dataStk[1]); NextCase();
      DeclCase(DivI): Op_DivI(dataStk[1]); NextCase();
      DeclCase(DivX): Op_DivX(dataStk[1]); NextCase();
      DeclCase(LAnd): Op_LAnd(dataStk[1]); NextCase();
      DeclCase(LOrI): Op_LOrI(dataStk[1]); NextCase();
      DeclCase(ModI): Op_ModI(dataStk[1]); NextCase();
      DeclCase(MulU): Op_MulU(dataStk[1]); NextCase();
      DeclCase(MulX): Op_MulX(dataStk[1]); NextCase();
      DeclCase(OrIU): Op_OrIU(dataStk[1]); NextCase();
      DeclCase(OrXU): Op_OrXU(dataStk[1]); NextCase();
      DeclCase(ShLU): Op_ShLU(dataStk[1]); NextCase();
      DeclCase(ShRI): Op_ShRI(dataStk[1]); NextCase();
      DeclCase(SubU): Op_SubU(dataStk[1]); NextCase();

         //================================================
         // Call codes.
         //

      DeclCase(Call_Lit):
         {
            Function *func;

            func = *codePtr < module->functionV.size() ? module->functionV[*codePtr] : nullptr;
            ++codePtr;

         do_call:
            if(!func) {BranchTo(0); NextCase();}

            // Reserve stack space.
            callStk.reserve(CallStkSize);
            dataStk.reserve(DataStkSize);

            // Push call frame.
            callStk.push({codePtr, module, scopeMod, localArr.size(), localReg.size()});

            // Apply function data.
            codePtr      = &func->module->codeV[func->codeIdx];
            module       = func->module;
            scopeMod     = scopeMap->getModuleScope(module);
            localArr.alloc(func->locArrC);
            localReg.alloc(func->locRegC);

            // Read arguments.
            dataStk.drop(func->argC);
            memcpy(&localReg[0], &dataStk[0], func->argC * sizeof(Word));

            NextCase();

      DeclCase(Call_Stk):
            dataStk.drop();
            func = env->getFunction(dataStk[0]);
            goto do_call;
         }

      DeclCase(CallFunc):
         {
            Word argc = *codePtr++;
            Word func = *codePtr++;
            dataStk.drop(argc);
            if(env->callFunc(this, func, &dataStk[0], argc))
               goto exec_intr;
         }
         NextCase();

      DeclCase(CallFunc_Lit):
         {
            Word        argc = *codePtr++;
            Word        func = *codePtr++;
            Word const *argv =  codePtr;
            codePtr += argc;
            if(env->callFunc(this, func, argv, argc))
               goto exec_intr;
         }
         NextCase();

      DeclCase(CallSpec):
         {
            Word argc = *codePtr++;
            Word spec = *codePtr++;
            dataStk.drop(argc);
            env->callSpec(this, spec, &dataStk[0], argc);
         }
         NextCase();

      DeclCase(CallSpec_Lit):
         {
            Word        argc = *codePtr++;
            Word        spec = *codePtr++;
            Word const *argv =  codePtr;
            codePtr += argc;
            env->callSpec(this, spec, argv, argc);
         }
         NextCase();

      DeclCase(CallSpec_R1):
         {
            Word argc = *codePtr++;
            Word spec = *codePtr++;
            dataStk.drop(argc);
            dataStk.push(env->callSpec(this, spec, &dataStk[0], argc));
         }
         NextCase();

      DeclCase(Retn):
         // If no call frames left, terminate the thread.
         if(callStk.empty())
            goto thread_stop;

         // Apply call frame.
         codePtr     = callStk[1].codePtr;
         module      = callStk[1].module;
         scopeMod    = callStk[1].scopeMod;
         localArr.free(callStk[1].locArrC);
         localReg.free(callStk[1].locRegC);

         // Drop call frame.
         callStk.drop();

         NextCase();

         //================================================
         // Drop codes.
         //

         OpSet(Drop);

      DeclCase(Drop_Nul):
        dataStk.drop();
        NextCase();

      DeclCase(Drop_ScrRet):
        dataStk.drop();
        result = dataStk[0];
        NextCase();

         //================================================
         // Jump codes.
         //

      DeclCase(Jcnd_Lit):
        if(dataStk[1] == *codePtr++)
        {
           dataStk.drop();
           BranchTo(*codePtr);
        }
        else
           ++codePtr;
        NextCase();

      DeclCase(Jcnd_Nil):
         if(dataStk.drop(), dataStk[0])
            ++codePtr;
         else
            BranchTo(*codePtr);
         NextCase();

      DeclCase(Jcnd_Tab):
         if(auto jump = module->jumpMapV[*codePtr++].table.find(dataStk[1]))
         {
            dataStk.drop();
            BranchTo(*jump);
         }
         NextCase();

      DeclCase(Jcnd_Tru):
         if(dataStk.drop(), dataStk[0])
            BranchTo(*codePtr);
         else
            ++codePtr;
         NextCase();

      DeclCase(Jump_Lit):
        BranchTo(*codePtr);
        NextCase();

      DeclCase(Jump_Stk):
         dataStk.drop();
         BranchTo(dataStk[0] < module->jumpV.size() ? module->jumpV[dataStk[0]].codeIdx : 0);
         NextCase();

         //================================================
         // Push codes.
         //

      DeclCase(Pfun_Lit):
         if(*codePtr < module->functionV.size())
            dataStk.push(module->functionV[*codePtr]->idx);
         else
            dataStk.push(0);
         ++codePtr;
         NextCase();

      DeclCase(Pstr_Stk):
         if(dataStk[1] < module->stringV.size())
            dataStk[1] = ~module->stringV[dataStk[1]]->idx;
         NextCase();

      DeclCase(Push_GblArr): dataStk[1] = scopeGbl->arrV[*codePtr++].find(dataStk[1]); NextCase();
      DeclCase(Push_GblReg): dataStk.push(scopeGbl->regV[*codePtr++]); NextCase();
      DeclCase(Push_HubArr): dataStk[1] = scopeHub->arrV[*codePtr++].find(dataStk[1]); NextCase();
      DeclCase(Push_HubReg): dataStk.push(scopeHub->regV[*codePtr++]); NextCase();
      DeclCase(Push_Lit):    dataStk.push(*codePtr++); NextCase();
      DeclCase(Push_LitArr): for(auto i = *codePtr++; i--;) dataStk.push(*codePtr++); NextCase();
      DeclCase(Push_LocArr): dataStk[1] = localArr[*codePtr++].find(dataStk[1]); NextCase();
      DeclCase(Push_LocReg): dataStk.push(localReg[*codePtr++]); NextCase();
      DeclCase(Push_ModArr): dataStk[1] = scopeMod->arrV[*codePtr++]->find(dataStk[1]); NextCase();
      DeclCase(Push_ModReg): dataStk.push(*scopeMod->regV[*codePtr++]); NextCase();

      DeclCase(Push_StrArs):
         dataStk.drop();
         dataStk[1] = scopeMap->getString(dataStk[1])->get(dataStk[0]);
         NextCase();

         //================================================
         // Script control codes.
         //

      DeclCase(ScrDelay):
         dataStk.drop();
         delay = dataStk[0];
         goto exec_intr;

      DeclCase(ScrDelay_Lit):
         delay = *codePtr++;
         goto exec_intr;

      DeclCase(ScrHalt):
         state = ThreadState::Paused;
         goto exec_intr;

      DeclCase(ScrRestart):
         BranchTo(script->codeIdx);
         NextCase();

      DeclCase(ScrTerm):
         goto thread_stop;

      DeclCase(ScrWaitI):
         dataStk.drop();
         state = {ThreadState::WaitScrI, dataStk[0]};
         goto exec_intr;

      DeclCase(ScrWaitI_Lit):
         state = {ThreadState::WaitScrI, *codePtr++};
         goto exec_intr;

      DeclCase(ScrWaitS):
         dataStk.drop();
         state = {ThreadState::WaitScrS, dataStk[0]};
         goto exec_intr;

      DeclCase(ScrWaitS_Lit):
         state = {ThreadState::WaitScrS, *codePtr++};
         goto exec_intr;

         //================================================
         // Stack control codes.
         //

      DeclCase(Copy):
         {auto temp = dataStk[1]; dataStk.push(temp);}
         NextCase();

      DeclCase(Swap):
         std::swap(dataStk[2], dataStk[1]);
         NextCase();

         //================================================
         // Unary operator codes.
         //

         OpSet(DecU);
         OpSet(IncU);

      DeclCase(InvU):
         dataStk[1] = ~dataStk[1];
         NextCase();

      DeclCase(NegI):
         dataStk[1] = ~dataStk[1] + 1;
         NextCase();

      DeclCase(NotU):
         dataStk[1] = !dataStk[1];
         NextCase();
      }

   thread_stop:
      stop();
   }
}

// EOF


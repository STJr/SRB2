//-----------------------------------------------------------------------------
//
// Copyright (C) 2015 David Hill
//
// See COPYING for license information.
//
//-----------------------------------------------------------------------------
//
// Tracer classes.
//
//-----------------------------------------------------------------------------

#include "Tracer.hpp"

#include "BinaryIO.hpp"
#include "Code.hpp"
#include "CodeData.hpp"
#include "Environment.hpp"
#include "Error.hpp"
#include "Function.hpp"
#include "Jump.hpp"
#include "Module.hpp"
#include "Script.hpp"


//----------------------------------------------------------------------------|
// Extern Functions                                                           |
//

namespace ACSVM
{
   //
   // TracerACS0 constructor
   //
   TracerACS0::TracerACS0(Environment *env_, Byte const *data_,
      std::size_t size_, bool compressed_) :
      env       {env_},
      codeFound {new Byte[size_]{}},
      codeIndex {new Word[size_]{}},
      codeC     {0},
      jumpC     {0},
      jumpMapC  {0},
      data      {data_},
      size      {size_},
      compressed{compressed_}
   {
   }

   //
   // TracerACS0 destructor
   //
   TracerACS0::~TracerACS0()
   {
   }

   //
   // TracerACS0::getArgBytes
   //
   std::size_t TracerACS0::getArgBytes(CodeDataACS0 const *opData, std::size_t iter)
   {
      std::size_t argBytes;

      switch(opData->code)
      {
      case CodeACS0::Push_LitArrB:
         if(size - iter < 1) throw ReadError();

         return data[iter] + 1;

      case CodeACS0::Jcnd_Tab:
         // Calculate alignment.
         argBytes = ((iter + 3) & ~static_cast<std::size_t>(3)) - iter;
         if(size - iter < argBytes) throw ReadError();

         // Read the number of cases.
         if(size - iter - argBytes < 4) throw ReadError();
         argBytes += ReadLE4(data + iter + argBytes) * 8 + 4;

         return argBytes;

      default:
         argBytes = 0;
         for(char const *s = opData->args; *s; ++s) switch(*s)
         {
         case 'B': argBytes += 1; break;
         case 'H': argBytes += 2; break;
         case 'W': argBytes += 4; break;
         case 'b': argBytes += compressed ? 1 : 4; break;
         case 'h': argBytes += compressed ? 2 : 4; break;
         }
         return argBytes;
      }
   }

   //
   // TracerACS0::readCallFunc
   //
   std::pair<Word /*argc*/, Word /*func*/> TracerACS0::readCallFunc(std::size_t iter)
   {
      Word argc, func;
      if(compressed)
      {
         argc = ReadLE1(data + iter + 0);
         func = ReadLE2(data + iter + 1);
      }
      else
      {
         argc = ReadLE4(data + iter + 0);
         func = ReadLE4(data + iter + 4);
      }
      return {argc, func};
   }

   //
   // TracerACS0::readOpACS0
   //
   std::tuple<
      Word                 /*opCode*/,
      CodeDataACS0 const * /*opData*/,
      std::size_t          /*opSize*/>
   TracerACS0::readOpACS0(std::size_t iter)
   {
      if(compressed)
      {
         if(size - iter < 1) throw ReadError();

         std::size_t opSize = 1;
         Word        opCode = data[iter];

         if(opCode >= 240)
         {
            if(size - iter < 2) throw ReadError();
            ++opSize;
            opCode = 240 + ((opCode - 240) << 8) + data[iter + 1];
         }

         return std::make_tuple(opCode, env->findCodeDataACS0(opCode), opSize);
      }
      else
      {
         if(size - iter < 4) throw ReadError();

         Word opCode = ReadLE4(data + iter);

         return std::make_tuple(opCode, env->findCodeDataACS0(opCode), 4);
      }
   }

   //
   // TracerACS0::setFound
   //
   bool TracerACS0::setFound(std::size_t first, std::size_t last)
   {
      Byte *begin = &codeFound[first];
      Byte *end   = &codeFound[last];

      std::size_t found = 0;

      for(Byte *itr = begin; itr != end; ++itr)
         found += *itr;

      if(found)
      {
         if(found != last - first) throw ReadError();
         return false;
      }

      for(Byte *itr = begin; itr != end; ++itr)
         *itr = true;

      return true;
   }

   //
   // TracerACS0::trace
   //
   void TracerACS0::trace(Module *module)
   {
      // Add Kill to catch branches to zero.
      codeC += 1 + env->getCodeData(Code::Kill)->argc;

      // Trace from entry points.

      for(Function *&func : module->functionV)
         if(func && func->module == module) trace(func->codeIdx);

      for(Jump &jump : module->jumpV)
         trace(jump.codeIdx);

      for(Script &scr : module->scriptV)
         trace(scr.codeIdx);

      // Add Kill to catch execution past end.
      codeC += 1 + env->getCodeData(Code::Kill)->argc;
   }

   //
   // TracerACS0::trace
   //
   void TracerACS0::trace(std::size_t iter)
   {
      for(std::size_t next;; iter = next)
      {
         // If at the end of the file, terminate tracer. Reaching here will
         // result in a Kill, but the bytecode is otherwise well formed.
         if(iter == size) return;

         // Whereas if iter is out of bounds, bytecode is malformed.
         if(iter > size) throw ReadError();

         // Read op.
         CodeDataACS0 const *opData;
         std::size_t         opSize;
         std::tie(std::ignore, opData, opSize) = readOpACS0(iter);

         // If no translation available, terminate trace.
         if(!opData)
         {
            // Mark as found, so that the translator generates a KILL.
            setFound(iter, iter + opSize);
            codeC += 1 + env->getCodeData(Code::Kill)->argc;
            return;
         }

         std::size_t opSizeFull = opSize + getArgBytes(opData, iter + opSize);

         // If this op goes out of bounds, bytecode is malformed.
         if(size - iter < opSizeFull) throw ReadError();

         next = iter + opSizeFull;

         // If this op already found, terminate trace.
         if(!setFound(iter, next))
            return;

         // Get data for translated op.
         CodeData const *opTran = env->getCodeData(opData->transCode);

         // Count internal size of op.
         switch(opData->code)
         {
            // -> Call(F) Drop_Nul()
         case CodeACS0::Call_Nul:
            codeC += 3;
            break;

         case CodeACS0::CallSpec_1L:
         case CodeACS0::CallSpec_2L:
         case CodeACS0::CallSpec_3L:
         case CodeACS0::CallSpec_4L:
         case CodeACS0::CallSpec_5L:
         case CodeACS0::CallSpec_6L:
         case CodeACS0::CallSpec_7L:
         case CodeACS0::CallSpec_8L:
         case CodeACS0::CallSpec_9L:
         case CodeACS0::CallSpec_10L:
         case CodeACS0::CallSpec_1LB:
         case CodeACS0::CallSpec_2LB:
         case CodeACS0::CallSpec_3LB:
         case CodeACS0::CallSpec_4LB:
         case CodeACS0::CallSpec_5LB:
         case CodeACS0::CallSpec_6LB:
         case CodeACS0::CallSpec_7LB:
         case CodeACS0::CallSpec_8LB:
         case CodeACS0::CallSpec_9LB:
         case CodeACS0::CallSpec_10LB:
         case CodeACS0::Push_Lit2B:
         case CodeACS0::Push_Lit3B:
         case CodeACS0::Push_Lit4B:
         case CodeACS0::Push_Lit5B:
         case CodeACS0::Push_Lit6B:
         case CodeACS0::Push_Lit7B:
         case CodeACS0::Push_Lit8B:
         case CodeACS0::Push_Lit9B:
         case CodeACS0::Push_Lit10B:
            codeC += opData->argc + 2;
            break;

         case CodeACS0::Push_LitArrB:
            codeC += data[iter + opSize] + 2;
            break;

            // -> Push_Lit(0) Retn()
         case CodeACS0::Retn_Nul:
            codeC += 3;
            break;

         case CodeACS0::CallFunc:
            {
               Word argc, func;
               std::tie(argc, func) = readCallFunc(iter + opSize);

               FuncDataACS0 const *opFunc = env->findFuncDataACS0(func);

               if(!opFunc)
               {
                  codeC += 1 + env->getCodeData(Code::Kill)->argc;
                  return;
               }

               opTran = env->getCodeData(opFunc->getTransCode(argc));
            }
            /* FALLTHRU */
         default:
            if(opTran->code == Code::CallFunc_Lit)
               codeC += opData->argc + 1 + 2;
            else
               codeC += opTran->argc + 1;

            if(opTran->code == Code::Kill)
               return;

            break;
         }

         // Special handling for branching ops.
         switch(opData->code)
         {
         case CodeACS0::Jcnd_Nil:
         case CodeACS0::Jcnd_Tru:
            ++jumpC;
            trace(ReadLE4(data + iter + opSize));
            break;

         case CodeACS0::Jcnd_Lit:
            ++jumpC;
            trace(ReadLE4(data + iter + opSize + 4));
            break;

         case CodeACS0::Jcnd_Tab:
            {
               std::size_t count, jumpIter;

               jumpIter = (iter + opSize + 3) & ~static_cast<std::size_t>(3);
               count = ReadLE4(data + jumpIter); jumpIter += 4;

               ++jumpMapC;

               // Trace all of the jump targets.
               for(; count--; jumpIter += 8)
                  trace(ReadLE4(data + jumpIter + 4));
            }
            break;

         case CodeACS0::Jump_Lit:
            ++jumpC;
            next = ReadLE4(data + iter + opSize);
            break;

         case CodeACS0::Jump_Stk:
            // The target of this jump will get traced when the dynamic jump
            // targets get traced.
            return;

         case CodeACS0::Retn_Stk:
         case CodeACS0::Retn_Nul:
         case CodeACS0::ScrTerm:
            return;

         default:
            break;
         }
      }
   }

   //
   // TracerACS0::translate
   //
   void TracerACS0::translate(Module *module)
   {
      std::unique_ptr<Word*[]> jumps{new uint32_t *[jumpC]};

      Word  *codeItr    = module->codeV.data();
      Word **jumpItr    = jumps.get();
      auto   jumpMapItr = module->jumpMapV.data();

      // Add Kill to catch branches to zero.
      *codeItr++ = static_cast<Word>(Code::Kill);
      *codeItr++ = static_cast<Word>(KillType::OutOfBounds);
      *codeItr++ = 0;

      for(std::size_t iter = 0, next; iter != size; iter = next)
      {
         // If no code at this index, skip it.
         if(!codeFound[iter])
         {
            next = iter + 1;
            continue;
         }

         // Record jump target.
         codeIndex[iter] = codeItr - module->codeV.data();

         // Read op.
         Word                opCode;
         CodeDataACS0 const *opData;
         std::size_t         opSize;
         std::tie(opCode, opData, opSize) = readOpACS0(iter);

         // If no translation available, generate Kill.
         if(!opData)
         {
            *codeItr++ = static_cast<Word>(Code::Kill);
            *codeItr++ = static_cast<Word>(KillType::UnknownCode);
            *codeItr++ = opCode;
            next = iter + opSize;
            continue;
         }

         // Calculate next index.
         next = iter + opSize + getArgBytes(opData, iter + opSize);

         // Get data for translated op.
         CodeData const *opTran = env->getCodeData(opData->transCode);

         // Generate internal op.
         switch(opData->code)
         {
         case CodeACS0::Call_Nul:
            *codeItr++ = static_cast<Word>(opData->transCode);
            if(compressed)
               *codeItr++ = ReadLE1(data + iter + opSize);
            else
               *codeItr++ = ReadLE4(data + iter + opSize);
            *codeItr++ = static_cast<Word>(Code::Drop_Nul);
            break;

         case CodeACS0::CallSpec_1:
         case CodeACS0::CallSpec_2:
         case CodeACS0::CallSpec_3:
         case CodeACS0::CallSpec_4:
         case CodeACS0::CallSpec_5:
         case CodeACS0::CallSpec_5Ex:
         case CodeACS0::CallSpec_6:
         case CodeACS0::CallSpec_7:
         case CodeACS0::CallSpec_8:
         case CodeACS0::CallSpec_9:
         case CodeACS0::CallSpec_10:
         case CodeACS0::CallSpec_5R1:
         case CodeACS0::CallSpec_10R1:
            *codeItr++ = static_cast<Word>(opData->transCode);
            *codeItr++ = opData->stackArgC;
            goto trans_args;

         case CodeACS0::CallSpec_1L:
         case CodeACS0::CallSpec_1LB:
         case CodeACS0::CallSpec_2L:
         case CodeACS0::CallSpec_2LB:
         case CodeACS0::CallSpec_3L:
         case CodeACS0::CallSpec_3LB:
         case CodeACS0::CallSpec_4L:
         case CodeACS0::CallSpec_4LB:
         case CodeACS0::CallSpec_5L:
         case CodeACS0::CallSpec_5LB:
         case CodeACS0::CallSpec_6L:
         case CodeACS0::CallSpec_6LB:
         case CodeACS0::CallSpec_7L:
         case CodeACS0::CallSpec_7LB:
         case CodeACS0::CallSpec_8L:
         case CodeACS0::CallSpec_8LB:
         case CodeACS0::CallSpec_9L:
         case CodeACS0::CallSpec_9LB:
         case CodeACS0::CallSpec_10L:
         case CodeACS0::CallSpec_10LB:
            *codeItr++ = static_cast<Word>(opData->transCode);
            *codeItr++ = opData->argc - 1;
            goto trans_args;

         case CodeACS0::Jcnd_Tab:
            {
               std::size_t count, jumpIter;

               jumpIter = (iter + opSize + 3) & ~static_cast<std::size_t>(3);
               count = ReadLE4(data + jumpIter); jumpIter += 4;

               *codeItr++ = static_cast<Word>(opData->transCode);
               *codeItr++ = jumpMapItr - module->jumpMapV.data();

               (jumpMapItr++)->loadJumps(data + jumpIter, count);
            }
            break;

         case CodeACS0::Push_LitArrB:
            *codeItr++ = static_cast<Word>(opData->transCode);
            iter += opSize;
            for(std::size_t n = *codeItr++ = data[iter++]; n--;)
               *codeItr++ = data[iter++];
            break;

         case CodeACS0::Push_Lit2B:
         case CodeACS0::Push_Lit3B:
         case CodeACS0::Push_Lit4B:
         case CodeACS0::Push_Lit5B:
         case CodeACS0::Push_Lit6B:
         case CodeACS0::Push_Lit7B:
         case CodeACS0::Push_Lit8B:
         case CodeACS0::Push_Lit9B:
         case CodeACS0::Push_Lit10B:
            *codeItr++ = static_cast<Word>(opData->transCode);
            *codeItr++ = opData->argc;
            goto trans_args;

         case CodeACS0::Retn_Nul:
            *codeItr++ = static_cast<Word>(Code::Push_Lit);
            *codeItr++ = static_cast<Word>(0);
            *codeItr++ = static_cast<Word>(opData->transCode);
            break;

         case CodeACS0::CallFunc:
            {
               Word argc, func;
               std::tie(argc, func) = readCallFunc(iter + opSize);

               FuncDataACS0 const *opFunc = env->findFuncDataACS0(func);

               if(!opFunc)
               {
                  *codeItr++ = static_cast<Word>(Code::Kill);
                  *codeItr++ = static_cast<Word>(KillType::UnknownFunc);
                  *codeItr++ = func;
                  continue;
               }

               opTran = env->getCodeData(opFunc->getTransCode(argc));

               *codeItr++ = static_cast<Word>(opTran->code);
               if(opTran->code == Code::Kill)
               {
                  *codeItr++ = static_cast<Word>(KillType::UnknownFunc);
                  *codeItr++ = func;
                  continue;
               }
               else if(opTran->code == Code::CallFunc)
               {
                  *codeItr++ = argc;
                  *codeItr++ = opFunc->transFunc;
               }
            }
            break;

         default:
            *codeItr++ = static_cast<Word>(opData->transCode);
            if(opTran->code == Code::Kill)
            {
               *codeItr++ = static_cast<Word>(KillType::UnknownCode);
               *codeItr++ = opCode;
               continue;
            }
            else if(opTran->code == Code::CallFunc)
            {
               *codeItr++ = opData->stackArgC;
               *codeItr++ = opData->transFunc;
            }
            else if(opTran->code == Code::CallFunc_Lit)
            {
               *codeItr++ = opData->argc;
               *codeItr++ = opData->transFunc;
            }

         trans_args:
            // Convert arguments.
            iter += opSize;
            for(char const *a = opData->args; *a; ++a) switch(*a)
            {
            case 'B': *codeItr++ = ReadLE1(data + iter); iter += 1; break;
            case 'H': *codeItr++ = ReadLE2(data + iter); iter += 2; break;
            case 'W': *codeItr++ = ReadLE4(data + iter); iter += 4; break;

            case 'J':
               *jumpItr++ = codeItr - 1;
               break;

            case 'S':
               if(*(codeItr - 1) < module->stringV.size())
                  *(codeItr - 1) = ~module->stringV[*(codeItr - 1)]->idx;
               break;

            case 'b':
               if(compressed)
                  {*codeItr++ = ReadLE1(data + iter); iter += 1;}
               else
                  {*codeItr++ = ReadLE4(data + iter); iter += 4;}
               break;

            case 'h':
               if(compressed)
                  {*codeItr++ = ReadLE2(data + iter); iter += 2;}
               else
                  {*codeItr++ = ReadLE4(data + iter); iter += 4;}
               break;
            }

            break;
         }
      }

      // Add Kill to catch execution past end.
      *codeItr++ = static_cast<Word>(Code::Kill);
      *codeItr++ = static_cast<Word>(KillType::OutOfBounds);
      *codeItr++ = 1;

      // Translate jumps. Has to be done after code in order to jump forward.
      while(jumpItr != jumps.get())
      {
         codeItr = *--jumpItr;

         if(*codeItr < size)
            *codeItr = codeIndex[*codeItr];
         else
            *codeItr = 0;
      }

      // Translate entry points.

      for(Function *&func : module->functionV)
      {
         if(func && func->module == module)
            func->codeIdx = func->codeIdx < size ? codeIndex[func->codeIdx] : 0;
      }

      for(Jump &jump : module->jumpV)
         jump.codeIdx = jump.codeIdx < size ? codeIndex[jump.codeIdx] : 0;

      for(JumpMap &jumpMap : module->jumpMapV)
      {
         for(auto &jump : jumpMap.table)
            jump.val = jump.val < size ? codeIndex[jump.val] : 0;
      }

      for(Script &scr : module->scriptV)
         scr.codeIdx = scr.codeIdx < size ? codeIndex[scr.codeIdx] : 0;
   }
}

// EOF


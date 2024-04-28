//-----------------------------------------------------------------------------
//
// Copyright (C) 2015-2017 David Hill
//
// See COPYING for license information.
//
//-----------------------------------------------------------------------------
//
// Thread classes.
//
//-----------------------------------------------------------------------------

#include "Thread.hpp"

#include "Array.hpp"
#include "BinaryIO.hpp"
#include "Environment.hpp"
#include "Module.hpp"
#include "Scope.hpp"
#include "Script.hpp"
#include "Serial.hpp"

#include <algorithm>


//----------------------------------------------------------------------------|
// Extern Functions                                                           |
//

namespace ACSVM
{
   //
   // Thread constructor
   //
   Thread::Thread(Environment *env_) :
      env{env_},

      link{this},

      codePtr {nullptr},
      module  {nullptr},
      scopeGbl{nullptr},
      scopeHub{nullptr},
      scopeMap{nullptr},
      scopeMod{nullptr},
      script  {nullptr},
      delay   {0},
      result  {0}
   {
   }

   //
   // Thread destructor
   //
   Thread::~Thread()
   {
   }

   //
   // Thread::getInfo
   //
   ThreadInfo const *Thread::getInfo() const
   {
      return nullptr;
   }

   //
   // Thread::loadState
   //
   void Thread::loadState(Serial &in)
   {
      std::size_t count, countFull;

      in.readSign(Signature::Thread);

      module   = env->getModule(env->readModuleName(in));
      codePtr  = &module->codeV[ReadVLN<std::size_t>(in)];
      scopeGbl = env->getGlobalScope(ReadVLN<Word>(in));
      scopeHub = scopeGbl->getHubScope(ReadVLN<Word>(in));
      scopeMap = scopeHub->getMapScope(ReadVLN<Word>(in));
      scopeMod = scopeMap->getModuleScope(module);
      script   = env->readScript(in);
      delay    = ReadVLN<Word>(in);
      result   = ReadVLN<Word>(in);

      count = ReadVLN<std::size_t>(in);
      callStk.clear(); callStk.reserve(count + CallStkSize);
      while(count--)
         callStk.push(readCallFrame(in));

      count = ReadVLN<std::size_t>(in);
      dataStk.clear(); dataStk.reserve(count + DataStkSize);
      while(count--)
         dataStk.push(ReadVLN<Word>(in));

      countFull = ReadVLN<std::size_t>(in);
      count     = ReadVLN<std::size_t>(in);
      localArr.allocLoad(countFull, count);
      for(auto itr = localArr.beginFull(), end = localArr.end(); itr != end; ++itr)
         itr->loadState(in);

      countFull = ReadVLN<std::size_t>(in);
      count     = ReadVLN<std::size_t>(in);
      localReg.allocLoad(countFull, count);
      for(auto itr = localReg.beginFull(), end = localReg.end(); itr != end; ++itr)
         *itr = ReadVLN<Word>(in);

      countFull = ReadVLN<std::size_t>(in);
      count     = ReadVLN<std::size_t>(in);
      in.in->read(printBuf.getLoadBuf(countFull, count), countFull);

      state.state = static_cast<ThreadState::State>(ReadVLN<int>(in));
      state.data = ReadVLN<Word>(in);
      state.type = ReadVLN<Word>(in);

      in.readSign(~Signature::Thread);
   }

   //
   // Thread::lockStrings
   //
   void Thread::lockStrings() const
   {
      for(auto &data : dataStk)
         ++env->getString(data)->lock;

      for(auto arr = localArr.beginFull(), end = localArr.end(); arr != end; ++arr)
         arr->lockStrings(env);

      for(auto reg = localReg.beginFull(), end = localReg.end(); reg != end; ++reg)
         ++env->getString(*reg)->lock;

      if(state == ThreadState::WaitScrS)
         ++env->getString(state.data)->lock;
   }

   //
   // Thread::readCallFrame
   //
   CallFrame Thread::readCallFrame(Serial &in) const
   {
      CallFrame out;

      out.module   = env->getModule(env->readModuleName(in));
      out.scopeMod = scopeMap->getModuleScope(out.module);
      out.codePtr  = &out.module->codeV[ReadVLN<std::size_t>(in)];
      out.locArrC  = ReadVLN<std::size_t>(in);
      out.locRegC  = ReadVLN<std::size_t>(in);

      return out;
   }

   //
   // Thread::refStrings
   //
   void Thread::refStrings() const
   {
      for(auto &data : dataStk)
         env->getString(data)->ref = true;

      for(auto arr = localArr.beginFull(), end = localArr.end(); arr != end; ++arr)
         arr->refStrings(env);

      for(auto reg = localReg.beginFull(), end = localReg.end(); reg != end; ++reg)
         env->getString(*reg)->ref = true;

      if(state == ThreadState::WaitScrS)
         env->getString(state.data)->ref = true;
   }

   //
   // Thread::saveState
   //
   void Thread::saveState(Serial &out) const
   {
      out.writeSign(Signature::Thread);

      env->writeModuleName(out, module->name);
      WriteVLN(out, codePtr - module->codeV.data());
      WriteVLN(out, scopeGbl->id);
      WriteVLN(out, scopeHub->id);
      WriteVLN(out, scopeMap->id);
      env->writeScript(out, script);
      WriteVLN(out, delay);
      WriteVLN(out, result);

      WriteVLN(out, callStk.size());
      for(auto &call : callStk)
         writeCallFrame(out, call);

      WriteVLN(out, dataStk.size());
      for(auto &data : dataStk)
         WriteVLN(out, data);

      WriteVLN(out, localArr.sizeFull());
      WriteVLN(out, localArr.size());
      for(auto itr = localArr.beginFull(), end = localArr.end(); itr != end; ++itr)
         itr->saveState(out);

      WriteVLN(out, localReg.sizeFull());
      WriteVLN(out, localReg.size());
      for(auto itr = localReg.beginFull(), end = localReg.end(); itr != end; ++itr)
         WriteVLN(out, *itr);

      WriteVLN(out, printBuf.sizeFull());
      WriteVLN(out, printBuf.size());
      out.out->write(printBuf.dataFull(), printBuf.sizeFull());

      WriteVLN<int>(out, state.state);
      WriteVLN(out, state.data);
      WriteVLN(out, state.type);

      out.writeSign(~Signature::Thread);
   }

   //
   // Thread::start
   //
   void Thread::start(Script *script_, MapScope *map, ThreadInfo const *,
      Word const *argV, Word argC)
   {
      link.insert(&map->threadActive);

      script  = script_;
      module  = script->module;
      codePtr = &module->codeV[script->codeIdx];

      scopeMod = map->getModuleScope(module);
      scopeMap = map;
      scopeHub = scopeMap->hub;
      scopeGbl = scopeHub->global;

      callStk.reserve(CallStkSize);
      dataStk.reserve(DataStkSize);
      localArr.alloc(script->locArrC);
      localReg.alloc(script->locRegC);

      std::copy(argV, argV + std::min<Word>(argC, script->argC), &localReg[0]);

      delay  = 0;
      result = 0;
      state  = ThreadState::Running;
   }

   //
   // Thread::stop
   //
   void Thread::stop()
   {
      // Release execution resources.
      callStk.clear();
      dataStk.clear();
      localArr.clear();
      localReg.clear();
      printBuf.clear();

      // Set state.
      state = ThreadState::Inactive;
   }

   //
   // Thread::unlockStrings
   //
   void Thread::unlockStrings() const
   {
      for(auto &data : dataStk)
         --env->getString(data)->lock;

      for(auto arr = localArr.beginFull(), end = localArr.end(); arr != end; ++arr)
         arr->unlockStrings(env);

      for(auto reg = localReg.beginFull(), end = localReg.end(); reg != end; ++reg)
         --env->getString(*reg)->lock;

      if(state == ThreadState::WaitScrS)
         --env->getString(state.data)->lock;
   }

   //
   // Thread::writeCallFrame
   //
   void Thread::writeCallFrame(Serial &out, CallFrame const &in) const
   {
      env->writeModuleName(out, in.module->name);
      WriteVLN(out, in.codePtr - in.module->codeV.data());
      WriteVLN(out, in.locArrC);
      WriteVLN(out, in.locRegC);
   }
}

// EOF


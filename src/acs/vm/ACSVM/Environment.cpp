//-----------------------------------------------------------------------------
//
// Copyright (C) 2015-2017 David Hill
//
// See COPYING for license information.
//
//-----------------------------------------------------------------------------
//
// Environment class.
//
//-----------------------------------------------------------------------------

#include "Environment.hpp"

#include "Action.hpp"
#include "BinaryIO.hpp"
#include "CallFunc.hpp"
#include "Code.hpp"
#include "CodeData.hpp"
#include "Function.hpp"
#include "HashMap.hpp"
#include "Module.hpp"
#include "PrintBuf.hpp"
#include "Scope.hpp"
#include "Script.hpp"
#include "Serial.hpp"
#include "Thread.hpp"

#include <iostream>
#include <list>
#include <unordered_map>
#include <vector>


//----------------------------------------------------------------------------|
// Types                                                                      |
//

namespace ACSVM
{
   //
   // Environment::PrivData
   //
   struct Environment::PrivData
   {
      using FuncName = std::pair<ModuleName, String *>;
      using FuncElem = HashMapElem<FuncName, Word>;

      struct NameEqual
      {
         bool operator () (ModuleName const *l, ModuleName const *r) const
            {return *l == *r;}
      };

      struct NameHash
      {
         std::size_t operator () (ModuleName const *name) const
            {return name->hash();}
      };

      struct FuncNameHash
      {
         std::size_t operator () (FuncName const &name) const
            {return name.first.hash() + name.second->hash;}
      };


      // Reserve index 0 as no function.
      std::vector<Function *> functionByIdx{nullptr};

      HashMapKeyExt<FuncName, Word, FuncNameHash> functionByName{16, 16};

      HashMapKeyMem<ModuleName, Module, &Module::name, &Module::hashLink> modules;

      HashMapKeyMem<Word, GlobalScope, &GlobalScope::id, &GlobalScope::hashLink> scopes;

      std::vector<CallFunc> tableCallFunc
      {
         #define ACSVM_FuncList(name) \
            CallFunc_Func_##name,
         #include "CodeList.hpp"

         CallFunc_Func_Nop
      };

      std::unordered_map<Word, CodeDataACS0> tableCodeDataACS0
      {
         #define ACSVM_CodeListACS0(name, code, args, transCode, stackArgC, transFunc) \
            {code, {CodeACS0::name, args, Code::transCode, stackArgC, Func::transFunc}},
         #include "CodeList.hpp"
      };

      std::unordered_map<Word, FuncDataACS0> tableFuncDataACS0
      {
         #define ACSVM_FuncListACS0(name, func, transFunc, ...) \
            {func, {FuncACS0::name, Func::transFunc, __VA_ARGS__}},
         #include "CodeList.hpp"
      };
   };
}


//----------------------------------------------------------------------------|
// Extern Functions                                                           |
//

namespace ACSVM
{
   //
   // Environment constructor
   //
   Environment::Environment() :
      branchLimit  {0},
      scriptLocRegC{ScriptLocRegCDefault},

      funcV{nullptr},
      funcC{0},

      pd{new PrivData}
   {
      funcV = pd->functionByIdx.data();
      funcC = pd->functionByIdx.size();
   }

   //
   // Environment destructor
   //
   Environment::~Environment()
   {
      pd->functionByName.free();
      pd->modules.free();
      pd->scopes.free();

      while(scriptAction.next->obj)
         delete scriptAction.next->obj;

      delete pd;

      // Deallocate threads. Do this after scopes have been destructed.
      while(threadFree.next->obj)
         delete threadFree.next->obj;
   }

   //
   // Environment::addCallFunc
   //
   Word Environment::addCallFunc(CallFunc func)
   {
      pd->tableCallFunc.push_back(func);
      return pd->tableCallFunc.size() - 1;
   }

   //
   // Environment::addCodeDataACS0
   //
   void Environment::addCodeDataACS0(Word code, CodeDataACS0 &&data)
   {
      auto itr = pd->tableCodeDataACS0.find(code);
      if(itr == pd->tableCodeDataACS0.end())
         pd->tableCodeDataACS0.emplace(code, std::move(data));
      else
         itr->second = std::move(data);
   }

   //
   // Environment::addFuncDataACS0
   //
   void Environment::addFuncDataACS0(Word func, FuncDataACS0 &&data)
   {
      auto itr = pd->tableFuncDataACS0.find(func);
      if(itr == pd->tableFuncDataACS0.end())
         pd->tableFuncDataACS0.emplace(func, std::move(data));
      else
         itr->second = std::move(data);
   }

   //
   // Environment::allocThread
   //
   Thread *Environment::allocThread()
   {
      return new Thread(this);
   }

   //
   // Environment::callFunc
   //
   bool Environment::callFunc(Thread *thread, Word func, Word const *argV, Word argC)
   {
      return pd->tableCallFunc[func](thread, argV, argC);
   }

   //
   // Environment::callSpec
   //
   Word Environment::callSpec(Thread *thread, Word spec, Word const *argV, Word argC)
   {
      if(thread->scopeMap->clampCallSpec && thread->module->isACS0)
      {
         Vector<Word> argTmp{argV, argC};
         for(auto &arg : argTmp) arg &= 0xFF;
         return callSpecImpl(thread, spec, argTmp.data(), argTmp.size());
      }
      else
         return callSpecImpl(thread, spec, argV, argC);
   }

   //
   // Environment::callSpecImpl
   //
   Word Environment::callSpecImpl(Thread *, Word, Word const *, Word)
   {
      return 0;
   }

   //
   // Environment::checkLock
   //
   bool Environment::checkLock(Thread *, Word, bool)
   {
      return false;
   }

   //
   // Environment::checkTag
   //
   bool Environment::checkTag(Word, Word)
   {
      return false;
   }

   //
   // Environment::collectStrings
   //
   void Environment::collectStrings()
   {
      stringTable.collectBegin();
      refStrings();
      stringTable.collectEnd();
   }

   //
   // Environment::countActiveThread
   //
   std::size_t Environment::countActiveThread() const
   {
      std::size_t n = 0;

      for(auto &scope : pd->scopes)
      {
         if(scope.active)
            n += scope.countActiveThread();
      }

      return n;
   }

   //
   // Environment::deferAction
   //
   void Environment::deferAction(ScriptAction &&action)
   {
      (new ScriptAction(std::move(action)))->link.insert(&scriptAction);
   }

   //
   // Environment::exec
   //
   void Environment::exec()
   {
      // Delegate deferred script actions.
      for(auto itr = scriptAction.begin(), end = scriptAction.end(); itr != end;)
      {
         auto scope = pd->scopes.find(itr->id.global);
         if(scope && scope->active)
            itr++->link.relink(&scope->scriptAction);
         else
            ++itr;
      }

      for(auto &scope : pd->scopes)
      {
         if(scope.active)
            scope.exec();
      }
   }

   //
   // Environment::findCodeDataACS0
   //
   CodeDataACS0 const *Environment::findCodeDataACS0(Word code)
   {
      auto itr = pd->tableCodeDataACS0.find(code);
      return itr == pd->tableCodeDataACS0.end() ? nullptr : &itr->second;
   }

   //
   // Environment::findFuncDataACS0
   //
   FuncDataACS0 const *Environment::findFuncDataACS0(Word func)
   {
      auto itr = pd->tableFuncDataACS0.find(func);
      return itr == pd->tableFuncDataACS0.end() ? nullptr : &itr->second;
   }

   //
   // Environment::findModule
   //
   Module *Environment::findModule(ModuleName const &name) const
   {
      return pd->modules.find(name);
   }

   //
   // Environment::freeFunction
   //
   void Environment::freeFunction(Function *func)
   {
      // Null every reference to this function in every Module.
      // O(N*M) is not very nice, but that can be fixed if/when it comes up.
      for(auto &module : pd->modules)
      {
         for(Function *&funcItr : module.functionV)
         {
            if(funcItr == func)
               funcItr = nullptr;
         }
      }

      pd->functionByIdx[func->idx] = nullptr;
      delete func;
   }

   //
   // Environment::freeGlobalScope
   //
   void Environment::freeGlobalScope(GlobalScope *scope)
   {
      pd->scopes.unlink(scope);
      delete scope;
   }

   //
   // Environment::freeModule
   //
   void Environment::freeModule(Module *module)
   {
      pd->modules.unlink(module);
      delete module;
   }

   //
   // Environment::freeThread
   //
   void Environment::freeThread(Thread *thread)
   {
      thread->link.relink(&threadFree);
   }

   //
   // Environment::getCodeData
   //
   CodeData const *Environment::getCodeData(Code code)
   {
      switch(code)
      {
         #define ACSVM_CodeList(name, argc) case Code::name: \
            {static CodeData const data{Code::name, argc}; return &data;}
         #include "CodeList.hpp"

      default:
      case Code::None:
         static CodeData const dataNone{Code::None, 0};
         return &dataNone;
      }
   }

   //
   // Environment::getFreeThread
   //
   Thread *Environment::getFreeThread()
   {
      if(threadFree.next->obj)
      {
         Thread *thread = threadFree.next->obj;
         thread->link.unlink();
         return thread;
      }
      else
         return allocThread();
   }

   //
   // Environment::getFunction
   //
   Function *Environment::getFunction(Module *module, String *funcName)
   {
      if(funcName)
      {
         PrivData::FuncName namePair{module->name, funcName};
         auto idx = pd->functionByName.find(namePair);

         if(!idx)
         {
            #if SIZE_MAX > UINT32_MAX
            if(pd->functionByIdx.size() > UINT32_MAX)
               throw std::bad_alloc();
            #endif

            idx = new PrivData::FuncElem{std::move(namePair),
               static_cast<Word>(pd->functionByIdx.size())};
            pd->functionByName.insert(idx);

            pd->functionByIdx.emplace_back();
            funcV = pd->functionByIdx.data();
            funcC = pd->functionByIdx.size();
         }

         auto &ptr = pd->functionByIdx[idx->val];

         if(!ptr)
            ptr = new Function{module, funcName, idx->val};

         return ptr;
      }
      else
         return new Function{module, nullptr, 0};
   }

   //
   // Environment::getGlobalScope
   //
   GlobalScope *Environment::getGlobalScope(Word id)
   {
      if(auto *scope = pd->scopes.find(id))
         return scope;

      auto scope = new GlobalScope(this, id);
      pd->scopes.insert(scope);
      return scope;
   }

   //
   // Environment::getModule
   //
   Module *Environment::getModule(ModuleName const &name)
   {
      auto module = pd->modules.find(name);

      if(!module)
      {
         module = new Module{this, name};
         pd->modules.insert(module);
         loadModule(module);
      }
      else
      {
         if(!module->loaded)
            loadModule(module);
      }

      return module;
   }

   //
   // Environment::getModuleName
   //
   ModuleName Environment::getModuleName(char const *str)
   {
      return getModuleName(str, std::strlen(str));
   }

   //
   // Environment::getModuleName
   //
   ModuleName Environment::getModuleName(char const *str, std::size_t len)
   {
      return {getString(str, len), nullptr, 0};
   }

   //
   // Environment::hasActiveThread
   //
   bool Environment::hasActiveThread() const
   {
      for(auto &scope : pd->scopes)
      {
         if(scope.active && scope.hasActiveThread())
            return true;
      }

      return false;
   }

   //
   // Environment::loadFunctions
   //
   void Environment::loadFunctions(Serial &in)
   {
      // Function index map.
      pd->functionByName.free();
      for(std::size_t n = ReadVLN<std::size_t>(in); n--;)
      {
         ModuleName name = readModuleName(in);
         String    *str  = &stringTable[ReadVLN<Word>(in)];
         Word       idx  = ReadVLN<Word>(in);

         pd->functionByName.insert(new PrivData::FuncElem{{name, str}, idx});
      }

      // Function vector.
      auto oldTable = pd->functionByIdx;

      pd->functionByIdx.clear();
      pd->functionByIdx.resize(ReadVLN<std::size_t>(in), nullptr);
      funcV = pd->functionByIdx.data();
      funcC = pd->functionByIdx.size();

      // Reset function indexes.
      for(Function *&func : oldTable)
      {
         if(func)
         {
            auto idx = pd->functionByName.find({func->module->name, func->name});
            func->idx = idx ? idx->val : 0;
            pd->functionByIdx[func->idx] = func;
         }
      }
   }

   //
   // Environment::loadGlobalScopes
   //
   void Environment::loadGlobalScopes(Serial &in)
   {
      // Clear existing scopes.
      pd->scopes.free();

      for(auto n = ReadVLN<std::size_t>(in); n--;)
         getGlobalScope(ReadVLN<Word>(in))->loadState(in);
   }

   //
   // Environment::loadScriptActions
   //
   void Environment::loadScriptActions(Serial &in)
   {
      readScriptActions(in, scriptAction);
   }

   //
   // Environment::loadState
   //
   void Environment::loadState(Serial &in)
   {
      in.readSign(Signature::Environment);

      loadStringTable(in);
      loadFunctions(in);
      loadGlobalScopes(in);
      loadScriptActions(in);

      in.readSign(~Signature::Environment);
   }

   //
   // Environment::loadStringTable
   //
   void Environment::loadStringTable(Serial &in)
   {
      StringTable oldTable{std::move(stringTable)};
      stringTable.loadState(in);
      resetStrings();
   }

   //
   // Environment::printArray
   //
   void Environment::printArray(PrintBuf &buf, Array const &array, Word index, Word limit)
   {
      PrintArrayChar(buf, array, index, limit);
   }

   //
   // Environment::printKill
   //
   void Environment::printKill(Thread *thread, Word type, Word data)
   {
      std::cerr << "ACSVM ERROR: Kill " << type << ':' << data
         << " at " << (thread->codePtr - thread->module->codeV.data() - 1) << '\n';
   }

   //
   // Environment::readModuleName
   //
   ModuleName Environment::readModuleName(Serial &in) const
   {
      auto s = readString(in);
      auto i = ReadVLN<std::size_t>(in);

      return {s, nullptr, i};
   }

   //
   // Environment::readScript
   //
   Script *Environment::readScript(Serial &in) const
   {
      auto idx = ReadVLN<std::size_t>(in);
      return &findModule(readModuleName(in))->scriptV[idx];
   }

   //
   // Environment::readScriptAction
   //
   ScriptAction *Environment::readScriptAction(Serial &in) const
   {
      auto action = static_cast<ScriptAction::Action>(ReadVLN<int>(in));

      Vector<Word> argV;
      argV.alloc(ReadVLN<std::size_t>(in));
      for(auto &arg : argV)
         arg = ReadVLN<Word>(in);

      ScopeID id;
      id.global = ReadVLN<Word>(in);
      id.hub    = ReadVLN<Word>(in);
      id.map    = ReadVLN<Word>(in);

      ScriptName name = readScriptName(in);

      return new ScriptAction{id, name, action, std::move(argV)};
   }

   //
   // Environment::readScriptActions
   //
   void Environment::readScriptActions(Serial &in, ListLink<ScriptAction> &out) const
   {
      // Clear existing actions.
      while(out.next->obj)
         delete out.next->obj;

      for(auto n = ReadVLN<std::size_t>(in); n--;)
         readScriptAction(in)->link.insert(&out);
   }

   //
   // Environment::readScriptName
   //
   ScriptName Environment::readScriptName(Serial &in) const
   {
      String *s = in.in->get() ? &stringTable[ReadVLN<Word>(in)] : nullptr;
      Word    i = ReadVLN<Word>(in);
      return {s, i};
   }

   //
   // Environment::readString
   //
   String *Environment::readString(Serial &in) const
   {
      if(auto idx = ReadVLN<std::size_t>(in))
         return &stringTable[idx - 1];
      else
         return nullptr;
   }

   //
   // Environment::refStrings
   //
   void Environment::refStrings()
   {
      for(auto &action : scriptAction)
         action.refStrings(this);

      for(auto &funcIdx : pd->functionByName)
      {
         funcIdx.key.first.s->ref = true;
         funcIdx.key.second->ref  = true;
      }

      for(auto &module : pd->modules)
         module.refStrings();

      for(auto &scope : pd->scopes)
         scope.refStrings();
   }

   //
   // Environment::resetStrings
   //
   void Environment::resetStrings()
   {
      for(auto &funcIdx : pd->functionByName)
      {
         funcIdx.key.first.s = getString(funcIdx.key.first.s);
         funcIdx.key.second  = getString(funcIdx.key.second);
      }

      for(auto &module : pd->modules)
         module.resetStrings();
   }

   //
   // Environment::saveFunctions
   //
   void Environment::saveFunctions(Serial &out) const
   {
      WriteVLN(out, pd->functionByName.size());
      for(auto &funcIdx : pd->functionByName)
      {
         writeModuleName(out, funcIdx.key.first);
         WriteVLN(out, funcIdx.key.second->idx);
         WriteVLN(out, funcIdx.val);
      }

      WriteVLN(out, pd->functionByIdx.size());
   }

   //
   // Environment::saveGlobalScopes
   //
   void Environment::saveGlobalScopes(Serial &out) const
   {
      WriteVLN(out, pd->scopes.size());
      for(auto &scope : pd->scopes)
      {
         WriteVLN(out, scope.id);
         scope.saveState(out);
      }
   }

   //
   // Environment::saveScriptActions
   //
   void Environment::saveScriptActions(Serial &out) const
   {
      writeScriptActions(out, scriptAction);
   }

   //
   // Environment::saveState
   //
   void Environment::saveState(Serial &out) const
   {
      out.writeSign(Signature::Environment);

      saveStringTable(out);
      saveFunctions(out);
      saveGlobalScopes(out);
      saveScriptActions(out);

      out.writeSign(~Signature::Environment);
   }

   //
   // Environment::saveStringTable
   //
   void Environment::saveStringTable(Serial &out) const
   {
      stringTable.saveState(out);
   }

   //
   // Environment::writeModuleName
   //
   void Environment::writeModuleName(Serial &out, ModuleName const &in) const
   {
      writeString(out, in.s);
      WriteVLN(out, in.i);
   }

   //
   // Environment::writeScript
   //
   void Environment::writeScript(Serial &out, Script *in) const
   {
      WriteVLN(out, in - in->module->scriptV.data());
      writeModuleName(out, in->module->name);
   }

   //
   // Environment::writeScriptAction
   //
   void Environment::writeScriptAction(Serial &out, ScriptAction const *in) const
   {
      WriteVLN<int>(out, in->action);

      WriteVLN(out, in->argV.size());
      for(auto &arg : in->argV)
         WriteVLN(out, arg);

      WriteVLN(out, in->id.global);
      WriteVLN(out, in->id.hub);
      WriteVLN(out, in->id.map);

      writeScriptName(out, in->name);
   }

   //
   // Environment::writeScriptActions
   //
   void Environment::writeScriptActions(Serial &out,
      ListLink<ScriptAction> const &in) const
   {
      WriteVLN(out, in.size());

      for(auto &action : in)
         writeScriptAction(out, &action);
   }

   //
   // Environment::writeScriptName
   //
   void Environment::writeScriptName(Serial &out, ScriptName const &in) const
   {
      if(in.s)
      {
         out.out->put('\1');
         WriteVLN(out, in.s->idx);
      }
      else
         out.out->put('\0');

      WriteVLN(out, in.i);
   }

   //
   // Environment::writeString
   //
   void Environment::writeString(Serial &out, String const *in) const
   {
      if(in)
         WriteVLN<std::size_t>(out, in->idx + 1);
      else
         WriteVLN<std::size_t>(out, 0);
   }

   //
   // Environment::PrintArrayChar
   //
   void Environment::PrintArrayChar(PrintBuf &buf, Array const &array, Word index, Word limit)
   {
      // Calculate output length and end index.
      std::size_t len = 0;
      Word        end;
      for(Word &itr = end = index; itr - index != limit; ++itr)
      {
         Word c = array.find(itr);
         if(!c) break;
         ++len;
      }

      // Acquire output buffer.
      buf.reserve(len);
      char *s = buf.getBuf(len);

      // Truncate elements to char.
      for(Word itr = index; itr != end; ++itr)
         *s++ = array.find(itr);
   }

   //
   // Environment::PrintArrayUTF8
   //
   void Environment::PrintArrayUTF8(PrintBuf &buf, Array const &array, Word index, Word limit)
   {
      // Calculate output length and end index.
      std::size_t len = 0;
      Word        end;
      for(Word &itr = end = index; itr - index != limit; ++itr)
      {
         Word c = array.find(itr);
         if(!c) break;
         if(c > 0x10FFFF) c = 0xFFFD;

              if(c <= 0x007F) len += 1;
         else if(c <= 0x07FF) len += 2;
         else if(c <= 0xFFFF) len += 3;
         else                 len += 4;
      }

      // Acquire output buffer.
      buf.reserve(len);
      char *s = buf.getBuf(len);

      // Convert UTF-32 sequence to UTF-8.
      for(Word itr = index; itr != end; ++itr)
      {
         Word c = array.find(itr);
         if(c > 0x10FFFF) c = 0xFFFD;

         if(c <= 0x7F)   {*s++ = 0x00 | (c >>  0); goto put0;}
         if(c <= 0x7FF)  {*s++ = 0xC0 | (c >>  6); goto put1;}
         if(c <= 0xFFFF) {*s++ = 0xE0 | (c >> 12); goto put2;}
                         {*s++ = 0xF0 | (c >> 18); goto put3;}

         put3: *s++ = 0x80 | ((c >> 12) & 0x3F);
         put2: *s++ = 0x80 | ((c >>  6) & 0x3F);
         put1: *s++ = 0x80 | ((c >>  0) & 0x3F);
         put0:;
      }
   }
}

// EOF


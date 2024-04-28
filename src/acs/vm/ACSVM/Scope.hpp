//-----------------------------------------------------------------------------
//
// Copyright (C) 2015-2017 David Hill
//
// See COPYING for license information.
//
//-----------------------------------------------------------------------------
//
// Scope classes.
//
//-----------------------------------------------------------------------------

#ifndef ACSVM__Scope_H__
#define ACSVM__Scope_H__

#include "Array.hpp"
#include "List.hpp"


//----------------------------------------------------------------------------|
// Types                                                                      |
//

namespace ACSVM
{
   extern "C" using MapScope_ScriptStartFuncC = void (*)(void *);

   //
   // GlobalScope
   //
   class GlobalScope
   {
   public:
      static constexpr std::size_t ArrC = 256;
      static constexpr std::size_t RegC = 256;


      GlobalScope(GlobalScope const &) = delete;
      GlobalScope(Environment *env, Word id);
      ~GlobalScope();

      std::size_t countActiveThread() const;

      void exec();

      void freeHubScope(HubScope *scope);

      HubScope *getHubScope(Word id);

      bool hasActiveThread() const;

      void lockStrings() const;

      void loadState(Serial &in);

      void refStrings() const;

      void reset();

      void saveState(Serial &out) const;

      void unlockStrings() const;

      Environment *const env;
      Word         const id;

      Array arrV[ArrC];
      Word  regV[RegC];

      ListLink<GlobalScope>  hashLink;
      ListLink<ScriptAction> scriptAction;

      bool active;

   private:
      struct PrivData;

      PrivData *pd;
   };

   //
   // HubScope
   //
   class HubScope
   {
   public:
      static constexpr std::size_t ArrC = 256;
      static constexpr std::size_t RegC = 256;


      HubScope(HubScope const &) = delete;
      HubScope(GlobalScope *global, Word id);
      ~HubScope();

      std::size_t countActiveThread() const;

      void exec();

      void freeMapScope(MapScope *scope);

      MapScope *getMapScope(Word id);

      bool hasActiveThread() const;

      void lockStrings() const;

      void loadState(Serial &in);

      void refStrings() const;

      void reset();

      void saveState(Serial &out) const;

      void unlockStrings() const;

      Environment *const env;
      GlobalScope *const global;
      Word         const id;

      Array arrV[ArrC];
      Word  regV[RegC];

      ListLink<HubScope>     hashLink;
      ListLink<ScriptAction> scriptAction;

      bool active;

   private:
      struct PrivData;

      PrivData *pd;
   };

   //
   // MapScope
   //
   class MapScope
   {
   public:
      using ScriptStartFunc = void (*)(Thread *);
      using ScriptStartFuncC = MapScope_ScriptStartFuncC;

      //
      // ScriptStartInfo
      //
      class ScriptStartInfo
      {
      public:
         ScriptStartInfo() :
            argV{nullptr}, func{nullptr}, funcc{nullptr}, info{nullptr}, argC{0} {}
         ScriptStartInfo(Word const *argV_, std::size_t argC_,
            ThreadInfo const *info_ = nullptr, ScriptStartFunc func_ = nullptr) :
            argV{argV_}, func{func_}, funcc{nullptr}, info{info_}, argC{argC_} {}
         ScriptStartInfo(Word const *argV_, std::size_t argC_,
            ThreadInfo const *info_, ScriptStartFuncC func_) :
            argV{argV_}, func{nullptr}, funcc{func_}, info{info_}, argC{argC_} {}

         Word       const *argV;
         ScriptStartFunc   func;
         ScriptStartFuncC  funcc;
         ThreadInfo const *info;
         std::size_t       argC;
      };


      MapScope(MapScope const &) = delete;
      MapScope(HubScope *hub, Word id);
      ~MapScope();

      void addModules(Module *const *moduleV, std::size_t moduleC);

      std::size_t countActiveThread() const;

      void exec();

      Script *findScript(ScriptName name);
      Script *findScript(String *name);
      Script *findScript(Word name);

      ModuleScope *getModuleScope(Module *module);

      String *getString(Word idx) const;

      bool hasActiveThread() const;

      bool hasModules() const;

      bool isScriptActive(Script *script);

      void loadState(Serial &in);

      void lockStrings() const;

      void refStrings() const;

      void reset();

      void saveState(Serial &out) const;

      bool scriptPause(Script *script);
      bool scriptPause(ScriptName name, ScopeID scope);
      bool scriptStart(Script *script, ScriptStartInfo const &info);
      bool scriptStart(ScriptName name, ScopeID scope, ScriptStartInfo const &info);
      bool scriptStartForced(Script *script, ScriptStartInfo const &info);
      bool scriptStartForced(ScriptName name, ScopeID scope, ScriptStartInfo const &info);
      Word scriptStartResult(Script *script, ScriptStartInfo const &info);
      Word scriptStartResult(ScriptName name, ScriptStartInfo const &info);
      Word scriptStartType(Word type, ScriptStartInfo const &info);
      Word scriptStartTypeForced(Word type, ScriptStartInfo const &info);
      bool scriptStop(Script *script);
      bool scriptStop(ScriptName name, ScopeID scope);

      void unlockStrings() const;

      Environment *const env;
      HubScope    *const hub;
      Word         const id;

      ListLink<MapScope>     hashLink;
      ListLink<ScriptAction> scriptAction;
      ListLink<Thread>       threadActive;

      // Used for untagged string lookup.
      Module *module0;

      bool active;
      bool clampCallSpec;

   protected:
      void freeThread(Thread *thread);

   private:
      struct PrivData;

      void loadModules(Serial &in);
      void loadThreads(Serial &in);

      void saveModules(Serial &out) const;
      void saveThreads(Serial &out) const;

      PrivData *pd;
   };

   //
   // ModuleScope
   //
   class ModuleScope
   {
   public:
      static constexpr std::size_t ArrC = 256;
      static constexpr std::size_t RegC = 256;


      ModuleScope(ModuleScope const &) = delete;
      ModuleScope(MapScope *map, Module *module);
      ~ModuleScope();

      void import();

      void loadState(Serial &in);

      void lockStrings() const;

      void refStrings() const;

      void saveState(Serial &out) const;

      void unlockStrings() const;

      Environment *const env;
      MapScope    *const map;
      Module      *const module;

      Array *arrV[ArrC];
      Word  *regV[RegC];

   private:
      Array selfArrV[ArrC];
      Word  selfRegV[RegC];
   };
}

#endif//ACSVM__Scope_H__


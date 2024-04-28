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

#ifndef ACSVM__Environment_H__
#define ACSVM__Environment_H__

#include "List.hpp"
#include "String.hpp"


//----------------------------------------------------------------------------|
// Types                                                                      |
//

namespace ACSVM
{
   //
   // Environment
   //
   // Represents an entire ACS environment.
   //
   class Environment
   {
   public:
      Environment();
      virtual ~Environment();

      Word addCallFunc(CallFunc func);

      void addCodeDataACS0(Word code, CodeDataACS0 &&data);
      void addFuncDataACS0(Word func, FuncDataACS0 &&data);

      virtual bool callFunc(Thread *thread, Word func, Word const *argV, Word argC);
      Word callSpec(Thread *thread, Word spec, Word const *argV, Word argC);

      // Function to check if a lock can be opened. Default behavior is to
      // always return false.
      virtual bool checkLock(Thread *thread, Word lock, bool door);

      // Function to check tags. Must return true to indicate script should
      // continue. Default behavior is to always return false.
      virtual bool checkTag(Word type, Word tag);

      void collectStrings();

      std::size_t countActiveThread() const;

      void deferAction(ScriptAction &&action);

      virtual void exec();

      CodeDataACS0 const *findCodeDataACS0(Word code);
      FuncDataACS0 const *findFuncDataACS0(Word func);

      Module *findModule(ModuleName const &name) const;

      // Used by Module when unloading.
      void freeFunction(Function *func);

      void freeGlobalScope(GlobalScope *scope);

      void freeModule(Module *module);

      void freeThread(Thread *thread);

      CodeData const *getCodeData(Code code);

      Thread *getFreeThread();

      Function *getFunction(Word idx) {return idx < funcC ? funcV[idx] : nullptr;}

      Function *getFunction(Module *module, String *name);

      GlobalScope *getGlobalScope(Word id);

      // Gets the named module, loading it if needed.
      Module *getModule(ModuleName const &name);

      ModuleName getModuleName(char const *str);
      virtual ModuleName getModuleName(char const *str, std::size_t len);

      // Called to translate script type from ACS0 script number.
      // Default behavior is to modulus 1000 the name.
      virtual std::pair<Word /*type*/, Word /*name*/> getScriptTypeACS0(Word name)
         {return {name / 1000, name % 1000};}

      // Called to translate script type from ACSE SPTR.
      // Default behavior is to return the type as-is.
      virtual Word getScriptTypeACSE(Word type) {return type;}

      String *getString(Word idx) {return &stringTable[~idx];}

      String *getString(char const *first, char const *last)
         {return &stringTable[{first, last}];}

      String *getString(char const *str)
         {return getString(str, std::strlen(str));}

      String *getString(char const *str, std::size_t len)
         {return &stringTable[{str, len}];}

      String *getString(StringData const *data)
         {return data ? &stringTable[*data] : nullptr;}

      // Returns true if any contained scope is active and has an active thread.
      bool hasActiveThread() const;

      virtual void loadState(Serial &in);

      // Prints an array to a print buffer. Default behavior is PrintArrayChar.
      virtual void printArray(PrintBuf &buf, Array const &array, Word index, Word limit);

      // Function to print Kill instructions. Default behavior is to print
      // message to stderr.
      virtual void printKill(Thread *thread, Word type, Word data);

      // Deserializes a ModuleName. Default behavior is to load s and i.
      virtual ModuleName readModuleName(Serial &in) const;

      Script *readScript(Serial &in) const;
      ScriptAction *readScriptAction(Serial &in) const;
      void readScriptActions(Serial &in, ListLink<ScriptAction> &out) const;
      ScriptName readScriptName(Serial &in) const;
      String *readString(Serial &in) const;

      virtual void refStrings();

      virtual void resetStrings();

      virtual void saveState(Serial &out) const;

      // Serializes a ModuleName. Default behavior is to save s and i.
      virtual void writeModuleName(Serial &out, ModuleName const &name) const;

      void writeScript(Serial &out, Script *in) const;
      void writeScriptAction(Serial &out, ScriptAction const *in) const;
      void writeScriptActions(Serial &out, ListLink<ScriptAction> const &in) const;
      void writeScriptName(Serial &out, ScriptName const &in) const;
      void writeString(Serial &out, String const *in) const;

      StringTable stringTable;

      // Number of branches allowed per call to Thread::exec. Default of 0
      // means no limit.
      Word branchLimit;

      // Default number of script variables. Default is 20.
      Word scriptLocRegC;


      // Prints an array to a print buffer, truncating elements of the array to
      // fit char.
      static void PrintArrayChar(PrintBuf &buf, Array const &array, Word index, Word limit);

      // Prints an array to a print buffer, converting the array as a UTF-32
      // sequence into a UTF-8 sequence.
      static void PrintArrayUTF8(PrintBuf &buf, Array const &array, Word index, Word limit);

      static constexpr Word ScriptLocRegCDefault = 20;

   protected:
      virtual Thread *allocThread();

      // Called by callSpec after processing arguments. Default behavior is to
      // do nothing and return 0.
      virtual Word callSpecImpl(Thread *thread, Word spec, Word const *argV, Word argC);

      virtual void loadModule(Module *module) = 0;

      ListLink<ScriptAction> scriptAction;
      ListLink<Thread>       threadFree;

      Function  **funcV;
      std::size_t funcC;

   private:
      struct PrivData;

      void loadFunctions(Serial &in);
      void loadGlobalScopes(Serial &in);
      void loadScriptActions(Serial &in);
      void loadStringTable(Serial &in);

      void saveFunctions(Serial &out) const;
      void saveGlobalScopes(Serial &out) const;
      void saveScriptActions(Serial &out) const;
      void saveStringTable(Serial &out) const;

      PrivData *pd;
   };
}

#endif//ACSVM__Environment_H__


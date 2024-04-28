//-----------------------------------------------------------------------------
//
// Copyright (C) 2015-2017 David Hill
//
// See COPYING for license information.
//
//-----------------------------------------------------------------------------
//
// Module class.
//
//-----------------------------------------------------------------------------

#ifndef ACSVM__Module_H__
#define ACSVM__Module_H__

#include "ID.hpp"
#include "List.hpp"
#include "Vector.hpp"

#include <functional>
#include <memory>


//----------------------------------------------------------------------------|
// Types                                                                      |
//

namespace ACSVM
{
   //
   // ModuleName
   //
   // Stores a Module's name. Name semantics are user-defined and must provide
   // a (user-defined) mapping from name to bytecode data. The names are used
   // internally only for determining if a specific module has already been
   // loaded. That is, two ModuleNames should compare equal if and only if they
   // designate the same bytecode data.
   //
   class ModuleName
   {
   public:
      ModuleName(String *s_, void *p_, std::size_t i_) : s{s_}, p{p_}, i{i_} {}

      bool operator == (ModuleName const &name) const
         {return s == name.s && p == name.p && i == name.i;}
      bool operator != (ModuleName const &name) const
         {return s != name.s || p != name.p || i != name.i;}

      std::size_t hash() const;

      // String value. May be null.
      String *s;

      // Arbitrary pointer value.
      void *p;

      // Arbitrary integer value.
      std::size_t i;
   };

   //
   // Module
   //
   // Represents an ACS bytecode module.
   //
   class Module
   {
   public:
      Module(Environment *env, ModuleName const &name);
      ~Module();

      void readBytecode(Byte const *data, std::size_t size);

      void refStrings() const;

      void reset();

      void resetStrings();

      Environment *env;
      ModuleName   name;

      Vector<String *>   arrImpV;
      Vector<ArrayInit>  arrInitV;
      Vector<String *>   arrNameV;
      Vector<Word>       arrSizeV;
      Vector<Word>       codeV;
      Vector<String *>   funcNameV;
      Vector<Function *> functionV;
      Vector<Module *>   importV;
      Vector<Jump>       jumpV;
      Vector<JumpMap>    jumpMapV;
      Vector<String *>   regImpV;
      Vector<WordInit>   regInitV;
      Vector<String *>   regNameV;
      Vector<String *>   scrNameV;
      Vector<Script>     scriptV;
      Vector<String *>   stringV;

      ListLink<Module> hashLink;

      bool isACS0;
      bool loaded;


      static std::pair<
         std::unique_ptr<Byte[]> /*data*/,
         std::size_t             /*size*/>
      DecryptStringACSE(Byte const *data, std::size_t size, std::size_t iter);

      static std::unique_ptr<char[]> ParseStringACS0(Byte const *first,
         Byte const *last, std::size_t len);

      static std::tuple<
         Byte const * /*begin*/,
         Byte const * /*end*/,
         std::size_t  /*len*/>
      ScanStringACS0(Byte const *data, std::size_t size, std::size_t iter);

   private:
      bool chunkIterACSE(Byte const *data, std::size_t size,
         bool (Module::*chunker)(Byte const *, std::size_t, Word));

      void chunkStrTabACSE(Vector<String *> &strV,
         Byte const *data, std::size_t size, bool junk);

      bool chunkerACSE_AIMP(Byte const *data, std::size_t size, Word chunkName);
      bool chunkerACSE_AINI(Byte const *data, std::size_t size, Word chunkName);
      bool chunkerACSE_ARAY(Byte const *data, std::size_t size, Word chunkName);
      bool chunkerACSE_ASTR(Byte const *data, std::size_t size, Word chunkName);
      bool chunkerACSE_ATAG(Byte const *data, std::size_t size, Word chunkName);
      bool chunkerACSE_FARY(Byte const *data, std::size_t size, Word chunkName);
      bool chunkerACSE_FNAM(Byte const *data, std::size_t size, Word chunkName);
      bool chunkerACSE_FUNC(Byte const *data, std::size_t size, Word chunkName);
      bool chunkerACSE_JUMP(Byte const *data, std::size_t size, Word chunkName);
      bool chunkerACSE_LOAD(Byte const *data, std::size_t size, Word chunkName);
      bool chunkerACSE_MEXP(Byte const *data, std::size_t size, Word chunkName);
      bool chunkerACSE_MIMP(Byte const *data, std::size_t size, Word chunkName);
      bool chunkerACSE_MINI(Byte const *data, std::size_t size, Word chunkName);
      bool chunkerACSE_MSTR(Byte const *data, std::size_t size, Word chunkName);
      bool chunkerACSE_SARY(Byte const *data, std::size_t size, Word chunkName);
      bool chunkerACSE_SFLG(Byte const *data, std::size_t size, Word chunkName);
      bool chunkerACSE_SNAM(Byte const *data, std::size_t size, Word chunkName);
      bool chunkerACSE_SPTR8(Byte const *data, std::size_t size, Word chunkName);
      bool chunkerACSE_SPTR12(Byte const *data, std::size_t size, Word chunkName);
      bool chunkerACSE_STRE(Byte const *data, std::size_t size, Word chunkName);
      bool chunkerACSE_STRL(Byte const *data, std::size_t size, Word chunkName);
      bool chunkerACSE_SVCT(Byte const *data, std::size_t size, Word chunkName);

      void readBytecodeACS0(Byte const *data, std::size_t size);
      void readBytecodeACSE(Byte const *data, std::size_t size,
         bool compressed, std::size_t iter = 4);

      void readChunksACSE(Byte const *data, std::size_t size, bool fakeACS0);

      void readCodeACS0(Byte const *data, std::size_t size, bool compressed);

      String *readStringACS0(Byte const *data, std::size_t size, std::size_t iter);

      void setScriptNameTypeACSE(Script *scr, Word nameInt, Word type);
   };
}

namespace std
{
   //
   // hash<::ACSVM::ModuleName>
   //
   template<>
   struct hash<::ACSVM::ModuleName>
   {
      size_t operator () (::ACSVM::ModuleName const &name) const
         {return name.hash();}
   };
}

#endif//ACSVM__Module_H__


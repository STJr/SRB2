//-----------------------------------------------------------------------------
//
// Copyright (C) 2015 David Hill
//
// See COPYING for license information.
//
//-----------------------------------------------------------------------------
//
// Deferred Action classes.
//
//-----------------------------------------------------------------------------

#ifndef ACSVM__Action_H__
#define ACSVM__Action_H__

#include "List.hpp"
#include "Script.hpp"
#include "Vector.hpp"


//----------------------------------------------------------------------------|
// Types                                                                      |
//

namespace ACSVM
{
   //
   // ScopeID
   //
   class ScopeID
   {
   public:
      ScopeID() = default;
      ScopeID(Word global_, Word hub_, Word map_) :
         global{global_}, hub{hub_}, map{map_} {}

      bool operator == (ScopeID const &id) const
         {return global == id.global && hub == id.hub && map == id.map;}
      bool operator != (ScopeID const &id) const
         {return global != id.global || hub != id.hub || map != id.map;}

      Word global;
      Word hub;
      Word map;
   };

   //
   // ScriptAction
   //
   // Represents a deferred Script action.
   //
   class ScriptAction
   {
   public:
      enum Action
      {
         Start,
         StartForced,
         Stop,
         Pause,
      };


      ScriptAction(ScriptAction &&action);
      ScriptAction(ScopeID id, ScriptName name, Action action, Vector<Word> &&argV);
      ~ScriptAction();

      void lockStrings(Environment *env) const;

      void refStrings(Environment *env) const;

      void unlockStrings(Environment *env) const;

      Action                 action;
      Vector<Word>           argV;
      ScopeID                id;
      ListLink<ScriptAction> link;
      ScriptName             name;
   };
}

#endif//ACSVM__Action_H__


//-----------------------------------------------------------------------------
//
// Copyright (C) 2017 David Hill
//
// See COPYING for license information.
//
//-----------------------------------------------------------------------------
//
// Serialization.
//
//-----------------------------------------------------------------------------

#ifndef ACSVM__Serial_H__
#define ACSVM__Serial_H__

#include "ID.hpp"

#include <istream>
#include <ostream>


//----------------------------------------------------------------------------|
// Types                                                                      |
//

namespace ACSVM
{
   //
   // Signature
   //
   enum class Signature : std::uint32_t
   {
      Array       = MakeID("ARAY"),
      Environment = MakeID("ENVI"),
      GlobalScope = MakeID("GBLs"),
      HubScope    = MakeID("HUBs"),
      MapScope    = MakeID("MAPs"),
      ModuleScope = MakeID("MODs"),
      Serial      = MakeID("SERI"),
      Thread      = MakeID("THRD"),
   };

   //
   // Serial
   //
   class Serial
   {
   public:
      Serial(std::istream &in_) : in{&in_} {}
      Serial(std::ostream &out_) : out{&out_},
         version{0}, signs{false} {}

      operator std::istream & () {return *in;}
      operator std::ostream & () {return *out;}

      void loadHead();
      void loadTail();

      void readSign(Signature sign);

      void saveHead();
      void saveTail();

      void writeSign(Signature sign);

      union
      {
         std::istream *const in;
         std::ostream *const out;
      };

      unsigned int version;
      bool         signs;
   };
}


//----------------------------------------------------------------------------|
// Extern Functions                                                           |
//

namespace ACSVM
{
   constexpr Signature operator ~ (Signature sign)
      {return static_cast<Signature>(~static_cast<std::uint32_t>(sign));}
}

#endif//ACSVM__Serial_H__


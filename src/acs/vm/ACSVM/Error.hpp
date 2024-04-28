//-----------------------------------------------------------------------------
//
// Copyright (C) 2015-2017 David Hill
//
// See COPYING for license information.
//
//-----------------------------------------------------------------------------
//
// Error classes.
//
//-----------------------------------------------------------------------------

#ifndef ACSVM__Error_H__
#define ACSVM__Error_H__

#include "Types.hpp"

#include <stdexcept>


//----------------------------------------------------------------------------|
// Types                                                                      |
//

namespace ACSVM
{
   //
   // ReadError
   //
   // Generic exception class for errors occurring during bytecode reading.
   //
   class ReadError : public std::exception
   {
   public:
      ReadError(char const *msg_ = "ACSVM::ReadError") : msg{msg_} {}

      virtual char const *what() const noexcept {return msg;}

      char const *const msg;
   };

   //
   // SerialError
   //
   // Generic exception for errors during serialization.
   //
   class SerialError : public std::exception
   {
   public:
      SerialError(char const *msg_) : msg{const_cast<char *>(msg_)} {}

      virtual char const *what() const noexcept {return msg;}

   protected:
      SerialError() : msg{nullptr} {}

      char *msg;
   };

   //
   // SerialSignError
   //
   // Thrown due to signature mismatch.
   //
   class SerialSignError : public SerialError
   {
   public:
      SerialSignError(Signature sig, Signature got);
      ~SerialSignError();

   private:
      static constexpr std::size_t Sig = 29, SigS = 39;
      static constexpr std::size_t Got = 49, GotS = 59;

      char buf[sizeof("signature mismatch: expected XXXXXXXX (XXXX) got XXXXXXXX (XXXX)")] =
                      "signature mismatch: expected XXXXXXXX (XXXX) got XXXXXXXX (XXXX)";
   };
}

#endif//ACSVM__Error_H__


//-----------------------------------------------------------------------------
//
// Copyright (C) 2015 David Hill
//
// See COPYING for license information.
//
//-----------------------------------------------------------------------------
//
// Internal CallFunc functions.
//
//-----------------------------------------------------------------------------

#ifndef ACSVM__CallFunc_H__
#define ACSVM__CallFunc_H__

#include "Types.hpp"


//----------------------------------------------------------------------------|
// Extern Functions                                                           |
//

namespace ACSVM
{
   #define ACSVM_FuncList(name) \
      bool (CallFunc_Func_##name)(Thread *thread, Word const *argv, Word argc);
   #include "CodeList.hpp"
}

#endif//ACSVM__CallFunc_H__


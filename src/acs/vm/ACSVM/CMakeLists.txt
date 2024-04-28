##-----------------------------------------------------------------------------
##
## Copyright (C) 2015-2017 David Hill
##
## See COPYING for license information.
##
##-----------------------------------------------------------------------------
##
## CMake file for acsvm.
##
##-----------------------------------------------------------------------------


##----------------------------------------------------------------------------|
## Environment Configuration                                                  |
##

include_directories(.)


##----------------------------------------------------------------------------|
## Targets                                                                    |
##

##
## acsvm
##
add_library(acsvm ${ACSVM_SHARED_DECL}
   Action.cpp
   Action.hpp
   Array.cpp
   Array.hpp
   BinaryIO.cpp
   BinaryIO.hpp
   CallFunc.cpp
   CallFunc.hpp
   Code.hpp
   CodeData.cpp
   CodeData.hpp
   CodeList.hpp
   Environment.cpp
   Environment.hpp
   Error.cpp
   Error.hpp
   Function.cpp
   Function.hpp
   HashMap.hpp
   HashMapFixed.hpp
   ID.hpp
   Init.cpp
   Init.hpp
   Jump.cpp
   Jump.hpp
   Module.cpp
   Module.hpp
   ModuleACS0.cpp
   ModuleACSE.cpp
   PrintBuf.cpp
   PrintBuf.hpp
   Scope.cpp
   Scope.hpp
   Script.cpp
   Script.hpp
   Serial.cpp
   Serial.hpp
   Stack.hpp
   Store.hpp
   String.cpp
   String.hpp
   Thread.cpp
   Thread.hpp
   ThreadExec.cpp
   Tracer.cpp
   Tracer.hpp
   Types.hpp
   Vector.hpp
)

ACSVM_INSTALL_LIB(acsvm)

## EOF


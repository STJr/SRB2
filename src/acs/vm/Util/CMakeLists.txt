##-----------------------------------------------------------------------------
##
## Copyright (C) 2015 David Hill
##
## See COPYING for license information.
##
##-----------------------------------------------------------------------------
##
## CMake file for acsvm-util.
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
## acsvm-capi
##
add_library(acsvm-util ${ACSVM_SHARED_DECL}
   Floats.cpp
   Floats.hpp
)

target_link_libraries(acsvm-util acsvm)

ACSVM_INSTALL_LIB(acsvm-util)

## EOF


##-----------------------------------------------------------------------------
##
## Copyright (C) 2015-2016 David Hill
##
## See COPYING for license information.
##
##-----------------------------------------------------------------------------
##
## Root CMake file.
##
##-----------------------------------------------------------------------------

cmake_minimum_required(VERSION 2.6)

cmake_policy(SET CMP0017 NEW)

project(acsvm)

include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)


##----------------------------------------------------------------------------|
## Functions                                                                  |
##

##
## ACSVM_INSTALL_EXE
##
function(ACSVM_INSTALL_EXE name)
   if(ACSVM_INSTALL_EXE)
      install(TARGETS ${name}
         RUNTIME DESTINATION bin
         LIBRARY DESTINATION lib
         ARCHIVE DESTINATION lib
      )
   endif()
endfunction()

##
## ACSVM_INSTALL_LIB
##
function(ACSVM_INSTALL_LIB name)
   if(ACSVM_INSTALL_LIB)
      if(ACSVM_INSTALL_API)
         install(TARGETS ${name}
            RUNTIME DESTINATION bin
            LIBRARY DESTINATION lib
            ARCHIVE DESTINATION lib
         )
      elseif(ACSVM_SHARED)
         install(TARGETS ${name}
            RUNTIME DESTINATION bin
            LIBRARY DESTINATION lib
         )
      endif()
   endif()
endfunction()

##
## ACSVM_TRY_C_FLAG
##
function(ACSVM_TRY_C_FLAG flag name)
   CHECK_C_COMPILER_FLAG(${flag} ACSVM_C_FLAG_${name})

   if(ACSVM_C_FLAG_${name})
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${flag}" PARENT_SCOPE)
   endif()
endfunction()

##
## ACSVM_TRY_CXX_FLAG
##
function(ACSVM_TRY_CXX_FLAG flag name)
   CHECK_CXX_COMPILER_FLAG(${flag} ACSVM_CXX_FLAG_${name})

   if(ACSVM_CXX_FLAG_${name})
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}" PARENT_SCOPE)
   endif()
endfunction()


##----------------------------------------------------------------------------|
## Environment Detection                                                      |
##

set(ACSVM_SHARED_DEFAULT ON)

if(NOT ACSVM_NOFLAGS)
   if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
      ACSVM_TRY_C_FLAG(-Wall    Wall)
      ACSVM_TRY_C_FLAG(-Wextra  Wextra)

      ACSVM_TRY_C_FLAG(-std=c11 STD_C)
   endif()

   if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
      ACSVM_TRY_CXX_FLAG(-Wall    Wall)
      ACSVM_TRY_CXX_FLAG(-Wextra  Wextra)

      ACSVM_TRY_CXX_FLAG(-std=c++11 STD_CXX)
   endif()
endif()

if(MSVC)
   # Disable shared by default, as the source does not contain the needed
   # declaration annotations to make that work under MSVC.
   set(ACSVM_SHARED_DEFAULT OFF)
endif()


##----------------------------------------------------------------------------|
## Variables                                                                  |
##

##
## ACSVM_INSTALL_API
##
if(NOT DEFINED ACSVM_INSTALL_API)
   set(ACSVM_INSTALL_API ON CACHE BOOL "Install ACSVM headers.")
endif()

##
## ACSVM_INSTALL_DOC
##
if(NOT DEFINED ACSVM_INSTALL_DOC)
   set(ACSVM_INSTALL_DOC ON CACHE BOOL "Install ACSVM documentation.")
endif()

##
## ACSVM_INSTALL_EXE
##
if(NOT DEFINED ACSVM_INSTALL_EXE)
   set(ACSVM_INSTALL_EXE ON CACHE BOOL "Install ACSVM executables.")
endif()

##
## ACSVM_INSTALL_LIB
##
if(NOT DEFINED ACSVM_INSTALL_LIB)
   set(ACSVM_INSTALL_LIB ON CACHE BOOL "Install ACSVM libraries.")
endif()

##
## ACSVM_SHARED
##
## If true (or equiavalent), libraries will be built as SHARED. Otherwise,
## they are built as STATIC.
##
if(NOT DEFINED ACSVM_SHARED)
   set(ACSVM_SHARED ${ACSVM_SHARED_DEFAULT} CACHE BOOL
      "Build libraries as shared objects.")
endif()

##
## ACSVM_SHARED_DECL
##
## Used internally for convenience in add_library commands.
##
if(ACSVM_SHARED)
   set(ACSVM_SHARED_DECL SHARED)
else()
   set(ACSVM_SHARED_DECL STATIC)
endif()


##----------------------------------------------------------------------------|
## Environment Configuration                                                  |
##

include_directories(.)


##----------------------------------------------------------------------------|
## Targets                                                                    |
##

add_subdirectory(ACSVM)

if(EXISTS "${CMAKE_SOURCE_DIR}/CAPI")
   add_subdirectory(CAPI)
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/Exec")
   add_subdirectory(Exec)
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/Util")
   add_subdirectory(Util)
endif()

## EOF



#=============================================================================
# Copyright 2010 Kitware, Inc.
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distribute this file outside of CMake, substitute the full
#  License text for the above reference.)

# support for the yasm assembler

set(CMAKE_ASM_YASM_SOURCE_FILE_EXTENSIONS nasm yasm asm)

if(NOT CMAKE_ASM_YASM_OBJECT_FORMAT)
  if(WIN32)
    if(CMAKE_C_SIZEOF_DATA_PTR EQUAL 8)
      set(CMAKE_ASM_YASM_OBJECT_FORMAT win64)
    else()
      set(CMAKE_ASM_YASM_OBJECT_FORMAT win32)
    endif()
  elseif(APPLE)
    if(CMAKE_C_SIZEOF_DATA_PTR EQUAL 8)
      set(CMAKE_ASM_YASM_OBJECT_FORMAT macho64)
    else()
      set(CMAKE_ASM_YASM_OBJECT_FORMAT macho)
    endif()
  else()
    if(CMAKE_C_SIZEOF_DATA_PTR EQUAL 8)
      set(CMAKE_ASM_YASM_OBJECT_FORMAT elf64)
    else()
      set(CMAKE_ASM_YASM_OBJECT_FORMAT elf)
    endif()
  endif()
endif()

set(CMAKE_ASM_YASM_COMPILE_OBJECT "<CMAKE_ASM_YASM_COMPILER> <FLAGS> -f ${CMAKE_ASM_YASM_OBJECT_FORMAT} -o <OBJECT> <SOURCE>")

# Load the generic ASMInformation file:
set(ASM_DIALECT "_YASM")
include(CMakeASMInformation)
set(ASM_DIALECT)

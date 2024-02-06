/*
 * GIFs2PNG, a free tool for merge GIF images with his masks and save into PNG
 * This is a part of the Platformer Game Engine by Wohlstand, a free platform for game making
 * Copyright (c) 2017-2023 Vitaly Novichkov <admin@wohlnet.ru>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "../pge_version.h" //Global Project version file

#pragma once
#ifndef EDITOR_VERSION_H
#define EDITOR_VERSION_H

#define IDI_ICON1 30001
#define IDI_ICON2 30002

//Version of this program

#ifdef MUSPLAY_VERSION_1
#   define V_VF1 MUSPLAY_VERSION_1
#else
#   define V_VF1 0
#endif

#ifdef MUSPLAY_VERSION_2
#   define V_VF2 MUSPLAY_VERSION_2
#else
#   define V_VF2 0
#endif

#ifdef MUSPLAY_VERSION_3
#   define V_VF3 MUSPLAY_VERSION_3
#else
#   define V_VF3 0
#endif

#ifdef MUSPLAY_VERSION_4
#   define V_VF4 MUSPLAY_VERSION_4
#else
#   define V_VF4 0
#endif

#ifdef MUSPLAY_VERSION_REL
#   define V_FILE_RELEASE STR_VALUE(MUSPLAY_VERSION_REL)
#else
#   define V_FILE_RELEASE "-unk" //"-alpha","-beta","-dev", or "" aka "release"
#endif


#define V_VF1_s STR_VALUE(V_VF1)
#define V_VF2_s STR_VALUE(V_VF2)
#define V_VF3_s STR_VALUE(V_VF3)
#define V_VF4_s STR_VALUE(V_VF4)
#if V_VF4 == 0
    #if V_VF3 == 0
        #define V_FILE_VERSION_NUM GEN_VERSION_NUMBER_2(V_VF1_s, V_VF2_s)
    #else
        #define V_FILE_VERSION_NUM GEN_VERSION_NUMBER_3(V_VF1_s, V_VF2_s, V_VF3_s)
    #endif
#else
    #define V_FILE_VERSION_NUM GEN_VERSION_NUMBER_4(V_VF1_s, V_VF2_s, V_VF3_s, V_VF4_s)
#endif

//Version of this program
#define V_FILE_VERSION V_FILE_VERSION_NUM

#define V_FILE_DESC "Simple SDL2 Mixer X Music player"

#define V_INTERNAL_NAME "pge_musplay"

#ifdef _WIN32
    #define V_ORIGINAL_NAME "pge_musplay.exe" // for Windows platforms
#else
    #define V_ORIGINAL_NAME "pge_musplay" // for any other platforms, however impossible, because RC is Windows-only file :P
#endif

//Uncomment this for enable detal logging
//#define DEBUG

#endif


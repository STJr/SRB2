// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2021 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  vid_copy.s
/// \brief code for updating the linear frame buffer screen.

#include "asm_defs.inc"           // structures, must match the C structures!

// DJGPPv2 is as fast as this one, but then someone may compile with a less
// good version of DJGPP than mine, so this little asm will do the trick!

#define srcptr          4+16
#define destptr         8+16
#define width           12+16
#define height          16+16
#define srcrowbytes     20+16
#define destrowbytes    24+16

// VID_BlitLinearScreen( src, dest, width, height, srcwidth, destwidth );
//         width is given as BYTES

#ifdef __i386__

.globl C(VID_BlitLinearScreen_ASM)
C(VID_BlitLinearScreen_ASM):
    pushl   %ebp                // preserve caller's stack frame
    pushl   %edi
    pushl   %esi                // preserve register variables
    pushl   %ebx

    cld
    movl    srcptr(%esp),%esi
    movl    destptr(%esp),%edi
    movl    width(%esp),%ebx
    movl    srcrowbytes(%esp),%eax
    subl    %ebx,%eax
    movl    destrowbytes(%esp),%edx
    subl    %ebx,%edx
    shrl    $2,%ebx
    movl    height(%esp),%ebp
LLRowLoop:
    movl    %ebx,%ecx
    rep/movsl   (%esi),(%edi)
    addl    %eax,%esi
    addl    %edx,%edi
    decl    %ebp
    jnz     LLRowLoop

    popl    %ebx                // restore register variables
    popl    %esi
    popl    %edi
    popl    %ebp                // restore the caller's stack frame

    ret
#endif

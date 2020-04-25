// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  tmap_asm.s
/// \brief ???

//.comm _dc_colormap,4
//.comm _dc_x,4
//.comm _dc_yl,4
//.comm _dc_yh,4
//.comm _dc_iscale,4
//.comm _dc_texturemid,4
//.comm _dc_source,4
//.comm _ylookup,4
//.comm _columnofs,4
//.comm _loopcount,4
//.comm _pixelcount,4
.data
_pixelcount:
.long 0x00000000
_loopcount:
.long 0x00000000
.align 8
_mmxcomm:
.long 0x00000000
.text

        .align 4
.globl _R_DrawColumn8_NOMMX
_R_DrawColumn8_NOMMX:
   pushl %ebp
   pushl %esi
   pushl %edi
   pushl %ebx
	movl _dc_yl,%edx
	movl _dc_yh,%eax
	subl %edx,%eax
	leal 1(%eax),%ebx
	testl %ebx,%ebx
	jle rdc8ndone
	movl _dc_x,%eax
        movl _ylookup, %edi
	movl (%edi,%edx,4),%esi
	movl _columnofs, %edi
	addl (%edi,%eax,4),%esi
	movl _dc_iscale,%edi
	movl %edx,%eax
	imull %edi,%eax
	movl _dc_texturemid,%ecx
	addl %eax,%ecx

	movl _dc_source,%ebp
   xorl %edx, %edx
   subl $0x12345678, %esi
.globl rdc8nwidth1
rdc8nwidth1:
	.align 4,0x90
rdc8nloop:
	movl %ecx,%eax
	shrl $16,%eax
	addl %edi,%ecx
	andl $127,%eax
	addl $0x12345678,%esi
.globl rdc8nwidth2
rdc8nwidth2:
	movb (%eax,%ebp),%dl
	movl _dc_colormap,%eax
	movb (%eax,%edx),%al
	movb %al,(%esi)
	decl %ebx
	jne rdc8nloop
rdc8ndone:
   popl %ebx
   popl %edi
   popl %esi
   popl %ebp
   ret

//
// Optimised specifically for P54C/P55C (aka Pentium with/without MMX)
// By ES 1998/08/01
//

.globl _R_DrawColumn_8_Pentium
_R_DrawColumn_8_Pentium:
	pushl %ebp
        pushl %ebx
	pushl %esi
        pushl %edi
	movl _dc_yl,%eax        // Top pixel
	movl _dc_yh,%ebx        // Bottom pixel
        movl _ylookup, %edi
	movl (%edi,%ebx,4),%ecx
	subl %eax,%ebx          // ebx=number of pixels-1
	jl rdc8pdone            // no pixel to draw, done
	jnz rdc8pmany
	movl _dc_x,%edx         // Special case: only one pixel
        movl _columnofs, %edi
	addl (%edi,%edx,4),%ecx // dest pixel at (%ecx)
	movl _dc_iscale,%esi
	imull %esi,%eax
	movl _dc_texturemid,%edi
	addl %eax,%edi          // texture index in edi
	movl _dc_colormap,%edx
   	shrl $16, %edi
   	movl _dc_source,%ebp
	andl $127,%edi
	movb (%edi,%ebp),%dl    // read texture pixel
	movb (%edx),%al	        // lookup for light
	movb %al,0(%ecx) 	// write it
	jmp rdc8pdone		// done!
.align 4, 0x90
rdc8pmany:			// draw >1 pixel
	movl _dc_x,%edx
        movl _columnofs, %edi
	movl (%edi,%edx,4),%edx
	leal 0x12345678(%edx, %ecx), %edi  // edi = two pixels above bottom
.globl rdc8pwidth5
rdc8pwidth5:  // DeadBeef = -2*SCREENWIDTH
        movl _dc_iscale,%edx	// edx = fracstep
	imull %edx,%eax
   	shll $9, %edx           // fixme: Should get 7.25 fix as input
	movl _dc_texturemid,%ecx
	addl %eax,%ecx          // ecx = frac
	movl _dc_colormap,%eax  // eax = lighting/special effects LUT
   	shll $9, %ecx
   	movl _dc_source,%esi    // esi = source ptr

	imull $0x12345678, %ebx // ebx = negative offset to pixel
.globl rdc8pwidth6
rdc8pwidth6:  // DeadBeef = -SCREENWIDTH

// Begin the calculation of the two first pixels
        leal (%ecx, %edx), %ebp
	shrl $25, %ecx
	movb (%esi, %ecx), %al
	leal (%edx, %ebp), %ecx
	shrl $25, %ebp
        movb (%eax), %dl

// The main loop
rdc8ploop:
	movb (%esi,%ebp), %al		// load 1
        leal (%ecx, %edx), %ebp         // calc frac 3

	shrl $25, %ecx                  // shift frac 2
        movb %dl, 0x12345678(%edi, %ebx)// store 0
.globl rdc8pwidth1
rdc8pwidth1:  // DeadBeef = 2*SCREENWIDTH

        movb (%eax), %al                // lookup 1

        movb %al, 0x12345678(%edi, %ebx)// store 1
.globl rdc8pwidth2
rdc8pwidth2:  // DeadBeef = 3*SCREENWIDTH
        movb (%esi, %ecx), %al          // load 2

        leal (%ebp, %edx), %ecx         // calc frac 4

        shrl $25, %ebp                  // shift frac 3
        movb (%eax), %dl                // lookup 2

        addl $0x12345678, %ebx          // counter
.globl rdc8pwidth3
rdc8pwidth3:  // DeadBeef = 2*SCREENWIDTH
        jl rdc8ploop                    // loop

// End of loop. Write extra pixel or just exit.
        jnz rdc8pdone
        movb %dl, 0x12345678(%edi, %ebx)// Write odd pixel
.globl rdc8pwidth4
rdc8pwidth4:  // DeadBeef = 2*SCREENWIDTH

rdc8pdone:

        popl %edi
	popl %esi
        popl %ebx
	popl %ebp
        ret

//
// MMX asm version, optimised for K6
// By ES 1998/07/05
//

.globl _R_DrawColumn_8_K6_MMX
_R_DrawColumn_8_K6_MMX:
	pushl %ebp
        pushl %ebx
	pushl %esi
        pushl %edi

        movl %esp, %eax // Push 8 or 12, so that (%esp) gets aligned by 8
        andl $7,%eax
        addl $8,%eax
        movl %eax, _mmxcomm // Temp storage in mmxcomm: (%esp) is used instead
        subl %eax,%esp

	movl _dc_yl,%edx        // Top pixel
	movl _dc_yh,%ebx        // Bottom pixel
        movl _ylookup, %edi
	movl (%edi,%ebx,4),%ecx
	subl %edx,%ebx         // ebx=number of pixels-1
	jl 0x12345678            // no pixel to draw, done
.globl rdc8moffs1
rdc8moffs1:
	jnz rdc8mmany
	movl _dc_x,%eax         // Special case: only one pixel
        movl _columnofs, %edi
	addl (%edi,%eax,4),%ecx  // dest pixel at (%ecx)
	movl _dc_iscale,%esi
	imull %esi,%edx
	movl _dc_texturemid,%edi
	addl %edx,%edi         // texture index in edi
	movl _dc_colormap,%edx
   	shrl $16, %edi
   	movl _dc_source,%ebp
	andl $127,%edi
	movb (%edi,%ebp),%dl  // read texture pixel
	movb (%edx),%al	 // lookup for light
	movb %al,0(%ecx) 	 // write it
	jmp rdc8mdone		 // done!
.globl rdc8moffs2
rdc8moffs2:
.align 4, 0x90
rdc8mmany:			 // draw >1 pixel
	movl _dc_x,%eax
        movl _columnofs, %edi
	movl (%edi,%eax,4),%eax
	leal 0x12345678(%eax, %ecx), %esi  // esi = two pixels above bottom
.globl rdc8mwidth3
rdc8mwidth3:  // DeadBeef = -2*SCREENWIDTH
        movl _dc_iscale,%ecx	 // ecx = fracstep
	imull %ecx,%edx
   	shll $9, %ecx           // fixme: Should get 7.25 fix as input
	movl _dc_texturemid,%eax
	addl %edx,%eax         // eax = frac
	movl _dc_colormap,%edx  // edx = lighting/special effects LUT
   	shll $9, %eax
	leal (%ecx, %ecx), %edi
   	movl _dc_source,%ebp    // ebp = source ptr
	movl %edi, 0(%esp)     // Start moving frac and fracstep to MMX regs

	imull $0x12345678, %ebx  // ebx = negative offset to pixel
.globl rdc8mwidth5
rdc8mwidth5:  // DeadBeef = -SCREENWIDTH

	movl %edi, 4(%esp)
	leal (%eax, %ecx), %edi
	movq 0(%esp), %mm1     // fracstep:fracstep in mm1
	movl %eax, 0(%esp)
	shrl $25, %eax
	movl %edi, 4(%esp)
	movzbl (%ebp, %eax), %eax
	movq 0(%esp), %mm0     // frac:frac in mm0

	paddd %mm1, %mm0
	shrl $25, %edi
	movq %mm0, %mm2
	psrld $25, %mm2         // texture index in mm2
	paddd %mm1, %mm0
	movq %mm2, 0(%esp)

.globl rdc8mloop
rdc8mloop:                      		// The main loop
	movq %mm0, %mm2                    // move 4-5 to temp reg
	movzbl (%ebp, %edi), %edi 		// read 1

	psrld $25, %mm2 			// shift 4-5
	movb (%edx,%eax), %cl 		// lookup 0

	movl 0(%esp), %eax 			// load 2
	addl $0x12345678, %ebx 		// counter
.globl rdc8mwidth2
rdc8mwidth2:  // DeadBeef = 2*SCREENWIDTH

	movb %cl, (%esi, %ebx)		// write 0
	movb (%edx,%edi), %ch 		// lookup 1

	movb %ch, 0x12345678(%esi, %ebx) 	// write 1
.globl rdc8mwidth1
rdc8mwidth1:  // DeadBeef = SCREENWIDTH
	movl 4(%esp), %edi			// load 3

	paddd %mm1, %mm0 			// frac 6-7
	movzbl (%ebp, %eax), %eax 		// lookup 2

	movq %mm2, 0(%esp) 		     // store texture index 4-5
	jl rdc8mloop

	jnz rdc8mno_odd
	movb (%edx,%eax), %cl  // write the last odd pixel
	movb %cl, 0x12345678(%esi)
.globl rdc8mwidth4
rdc8mwidth4:  // DeadBeef = 2*SCREENWIDTH
rdc8mno_odd:

.globl rdc8mdone
rdc8mdone:
        emms

        addl _mmxcomm, %esp
        popl %edi
	popl %esi
        popl %ebx
	popl %ebp
        ret

// Need some extra space to align run-time
.globl R_DrawColumn_8_K6_MMX_end
R_DrawColumn_8_K6_MMX_end:
nop;nop;nop;nop;nop;nop;nop;nop;
nop;nop;nop;nop;nop;nop;nop;nop;
nop;nop;nop;nop;nop;nop;nop;nop;
nop;nop;nop;nop;nop;nop;nop;

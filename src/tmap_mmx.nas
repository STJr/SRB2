;; SONIC ROBO BLAST 2
;;-----------------------------------------------------------------------------
;; Copyright (C) 1998-2000 by DOSDOOM.
;; Copyright (C) 2010-2020 by Sonic Team Junior.
;;
;; This program is free software distributed under the
;; terms of the GNU General Public License, version 2.
;; See the 'LICENSE' file for more details.
;;-----------------------------------------------------------------------------
;; FILE:
;;      tmap_mmx.nas
;; DESCRIPTION:
;;      Assembler optimised rendering code for software mode, using SIMD
;;      instructions.
;;      Draw wall columns.


[BITS 32]

%define FRACBITS 16
%define TRANSPARENTPIXEL 255

%ifdef LINUX
%macro cextern 1
[extern %1]
%endmacro

%macro cglobal 1
[global %1]
%endmacro

%else
%macro cextern 1
%define %1 _%1
[extern %1]
%endmacro

%macro cglobal 1
%define %1 _%1
[global %1]
%endmacro

%endif


; The viddef_s structure. We only need the width field.
struc viddef_s
		resb 12
.width: resb 4
		resb 44
endstruc


;; externs
;; columns
cextern dc_colormap
cextern dc_x
cextern dc_yl
cextern dc_yh
cextern dc_iscale
cextern dc_texturemid
cextern dc_texheight
cextern dc_source
cextern dc_hires
cextern centery
cextern centeryfrac
cextern dc_transmap

cextern R_DrawColumn_8_ASM
cextern R_Draw2sMultiPatchColumn_8_ASM

;; spans
cextern nflatshiftup
cextern nflatxshift
cextern nflatyshift
cextern nflatmask
cextern ds_xfrac
cextern ds_yfrac
cextern ds_xstep
cextern ds_ystep
cextern ds_x1
cextern ds_x2
cextern ds_y
cextern ds_source
cextern ds_colormap

cextern ylookup
cextern columnofs
cextern vid

[SECTION .data]

nflatmask64		dq		0


[SECTION .text]

;;----------------------------------------------------------------------
;;
;; R_DrawColumn : 8bpp column drawer
;;
;; MMX column drawer.
;;
;;----------------------------------------------------------------------
;; eax = accumulator
;; ebx = colormap
;; ecx = count
;; edx = accumulator
;; esi = source
;; edi = dest
;; ebp = vid.width
;; mm0 = accumulator
;; mm1 = heightmask, twice
;; mm2 = 2 * fracstep, twice
;; mm3 = pair of consecutive fracs
;;----------------------------------------------------------------------


cglobal R_DrawColumn_8_MMX
R_DrawColumn_8_MMX:
		push		ebp						;; preserve caller's stack frame pointer
		push		esi						;; preserve register variables
		push		edi
		push		ebx

;;
;; Our algorithm requires that the texture height be a power of two.
;; If not, fall back to the non-MMX drawer.
;;
.texheightcheck:
		mov			edx, [dc_texheight]
		sub			edx, 1					;; edx = heightmask
		test		edx, [dc_texheight]
		jnz			near .usenonMMX

		mov			ebp, edx				;; Keep a copy of heightmask in a
											;; GPR for the time being.

;;
;; Fill mm1 with heightmask
;;
		movd		mm1, edx				;; low dword = heightmask
		punpckldq	mm1, mm1				;; copy low dword to high dword

;;
;; dest = ylookup[dc_yl] + columnofs[dc_x];
;;
		mov			eax, [dc_yl]
		mov			edi, [ylookup+eax*4]
		mov			ebx, [dc_x]
		add			edi, [columnofs+ebx*4]	;; edi = dest


;;
;; pixelcount = yh - yl + 1
;;
		mov			ecx, [dc_yh]
		add			ecx, 1
		sub			ecx, eax				;; pixel count
		jle			near .done				;; nothing to scale

;;
;; fracstep = dc_iscale;
;;
		movd		mm2, [dc_iscale]		;; fracstep in low dword
		punpckldq	mm2, mm2				;; copy to high dword

		mov			ebx, [dc_colormap]
		mov			esi, [dc_source]

;;
;; frac = (dc_texturemid + FixedMul((dc_yl << FRACBITS) - centeryfrac, fracstep));
;;
											;; eax == dc_yl already
		shl			eax, FRACBITS
		sub			eax, [centeryfrac]
		imul		dword [dc_iscale]
		shrd		eax, edx, FRACBITS
		add			eax, [dc_texturemid]

;;
;; if (dc_hires) frac = 0;
;;
		test		byte [dc_hires], 0x01
		jz			.mod2
		xor			eax, eax


;;
;; Do mod-2 pixel.
;;
.mod2:
		test		ecx, 1
		jz			.pairprepare
		mov			edx, eax				;; edx = frac
		add			eax, [dc_iscale]		;; eax += fracstep
		sar			edx, FRACBITS
		and			edx, ebp				;; edx &= heightmask
		movzx		edx, byte [esi + edx]
		movzx		edx, byte [ebx + edx]
		mov			[edi], dl

		add			edi, [vid + viddef_s.width]
		sub			ecx, 1
		jz			.done

.pairprepare:
;;
;; Prepare for the main loop.
;;
		movd		mm3, eax				;; Low dword = frac
		movq		mm4, mm3				;; Copy to intermediate register
		paddd		mm4, mm2				;; dwords of mm4 += fracstep
		punpckldq	mm3, mm4				;; Low dword = first frac, high = second
		pslld		mm2, 1					;; fracstep *= 2

;;
;; ebp = vid.width
;;
		mov			ebp, [vid + viddef_s.width]

		align		16
.pairloop:
		movq		mm0, mm3				;; 3B 1u.
		psrad		mm0, FRACBITS			;; 4B 1u.
		pand		mm0, mm1				;; 3B 1u. frac &= heightmask
		paddd		mm3, mm2				;; 3B 1u. frac += fracstep

		movd		eax, mm0				;; 3B 1u. Get first frac
;; IFETCH boundary
		movzx		eax, byte [esi + eax]	;; 4B 1u. Texture map
		movzx		eax, byte [ebx + eax]	;; 4B 1u. Colormap

		punpckhdq	mm0, mm0				;; 3B 1(2)u. low dword = high dword
		movd		edx, mm0				;; 3B 1u. Get second frac
		mov			[edi], al				;; 2B 1(2)u. First pixel
;; IFETCH boundary

		movzx		edx, byte [esi + edx]	;; 4B 1u. Texture map
		movzx		edx, byte [ebx + edx]	;; 4B 1u. Colormap
		mov			[edi + 1*ebp], dl		;; 3B 1(2)u. Second pixel

		lea			edi, [edi + 2*ebp]		;; 3B 1u. edi += 2 * vid.width
;; IFETCH boundary
		sub			ecx, 2					;; 3B 1u. count -= 2
		jnz			.pairloop				;; 2B 1u. if(count != 0) goto .pairloop


.done:
;;
;; Clear MMX state, or else FPU operations will go badly awry.
;;
		emms

		pop			ebx
		pop			edi
		pop			esi
		pop			ebp
		ret

.usenonMMX:
		call		R_DrawColumn_8_ASM
		jmp			.done


;;----------------------------------------------------------------------
;;
;; R_Draw2sMultiPatchColumn : Like R_DrawColumn, but omits transparent
;;                            pixels.
;;
;; MMX column drawer.
;;
;;----------------------------------------------------------------------
;; eax = accumulator
;; ebx = colormap
;; ecx = count
;; edx = accumulator
;; esi = source
;; edi = dest
;; ebp = vid.width
;; mm0 = accumulator
;; mm1 = heightmask, twice
;; mm2 = 2 * fracstep, twice
;; mm3 = pair of consecutive fracs
;;----------------------------------------------------------------------


cglobal R_Draw2sMultiPatchColumn_8_MMX
R_Draw2sMultiPatchColumn_8_MMX:
		push		ebp						;; preserve caller's stack frame pointer
		push		esi						;; preserve register variables
		push		edi
		push		ebx

;;
;; Our algorithm requires that the texture height be a power of two.
;; If not, fall back to the non-MMX drawer.
;;
.texheightcheck:
		mov			edx, [dc_texheight]
		sub			edx, 1					;; edx = heightmask
		test		edx, [dc_texheight]
		jnz			near .usenonMMX

		mov			ebp, edx				;; Keep a copy of heightmask in a
											;; GPR for the time being.

;;
;; Fill mm1 with heightmask
;;
		movd		mm1, edx				;; low dword = heightmask
		punpckldq	mm1, mm1				;; copy low dword to high dword

;;
;; dest = ylookup[dc_yl] + columnofs[dc_x];
;;
		mov			eax, [dc_yl]
		mov			edi, [ylookup+eax*4]
		mov			ebx, [dc_x]
		add			edi, [columnofs+ebx*4]	;; edi = dest


;;
;; pixelcount = yh - yl + 1
;;
		mov			ecx, [dc_yh]
		add			ecx, 1
		sub			ecx, eax				;; pixel count
		jle			near .done				;; nothing to scale
;;
;; fracstep = dc_iscale;
;;
		movd		mm2, [dc_iscale]		;; fracstep in low dword
		punpckldq	mm2, mm2				;; copy to high dword

		mov			ebx, [dc_colormap]
		mov			esi, [dc_source]

;;
;; frac = (dc_texturemid + FixedMul((dc_yl << FRACBITS) - centeryfrac, fracstep));
;;
											;; eax == dc_yl already
		shl			eax, FRACBITS
		sub			eax, [centeryfrac]
		imul		dword [dc_iscale]
		shrd		eax, edx, FRACBITS
		add			eax, [dc_texturemid]

;;
;; if (dc_hires) frac = 0;
;;
		test		byte [dc_hires], 0x01
		jz			.mod2
		xor			eax, eax


;;
;; Do mod-2 pixel.
;;
.mod2:
		test		ecx, 1
		jz			.pairprepare
		mov			edx, eax				;; edx = frac
		add			eax, [dc_iscale]		;; eax += fracstep
		sar			edx, FRACBITS
		and			edx, ebp				;; edx &= heightmask
		movzx		edx, byte [esi + edx]
		cmp			dl, TRANSPARENTPIXEL
		je			.nextmod2
		movzx		edx, byte [ebx + edx]
		mov			[edi], dl

.nextmod2:
		add			edi, [vid + viddef_s.width]
		sub			ecx, 1
		jz			.done

.pairprepare:
;;
;; Prepare for the main loop.
;;
		movd		mm3, eax				;; Low dword = frac
		movq		mm4, mm3				;; Copy to intermediate register
		paddd		mm4, mm2				;; dwords of mm4 += fracstep
		punpckldq	mm3, mm4				;; Low dword = first frac, high = second
		pslld		mm2, 1					;; fracstep *= 2

;;
;; ebp = vid.width
;;
		mov			ebp, [vid + viddef_s.width]

		align		16
.pairloop:
		movq		mm0, mm3				;; 3B 1u.
		psrad		mm0, FRACBITS			;; 4B 1u.
		pand		mm0, mm1				;; 3B 1u. frac &= heightmask
		paddd		mm3, mm2				;; 3B 1u. frac += fracstep

		movd		eax, mm0				;; 3B 1u. Get first frac
;; IFETCH boundary
		movzx		eax, byte [esi + eax]	;; 4B 1u. Texture map
		punpckhdq	mm0, mm0				;; 3B 1(2)u. low dword = high dword
		movd		edx, mm0				;; 3B 1u. Get second frac
		cmp			al, TRANSPARENTPIXEL	;; 2B 1u.
		je			.secondinpair			;; 2B 1u.
;; IFETCH boundary
		movzx		eax, byte [ebx + eax]	;; 4B 1u. Colormap
		mov			[edi], al				;; 2B 1(2)u. First pixel

.secondinpair:
		movzx		edx, byte [esi + edx]	;; 4B 1u. Texture map
		cmp			dl, TRANSPARENTPIXEL	;; 2B 1u.
		je			.nextpair				;; 2B 1u.
;; IFETCH boundary
		movzx		edx, byte [ebx + edx]	;; 4B 1u. Colormap
		mov			[edi + 1*ebp], dl		;; 3B 1(2)u. Second pixel

.nextpair:
		lea			edi, [edi + 2*ebp]		;; 3B 1u. edi += 2 * vid.width
		sub			ecx, 2					;; 3B 1u. count -= 2
		jnz			.pairloop				;; 2B 1u. if(count != 0) goto .pairloop


.done:
;;
;; Clear MMX state, or else FPU operations will go badly awry.
;;
		emms

		pop			ebx
		pop			edi
		pop			esi
		pop			ebp
		ret

.usenonMMX:
		call		R_Draw2sMultiPatchColumn_8_ASM
		jmp			.done


;;----------------------------------------------------------------------
;;
;; R_DrawSpan : 8bpp span drawer
;;
;; MMX span drawer.
;;
;;----------------------------------------------------------------------
;; eax = accumulator
;; ebx = colormap
;; ecx = count
;; edx = accumulator
;; esi = source
;; edi = dest
;; ebp = two pixels
;; mm0 = accumulator
;; mm1 = xposition
;; mm2 = yposition
;; mm3 = 2 * xstep
;; mm4 = 2 * ystep
;; mm5 = nflatxshift
;; mm6 = nflatyshift
;; mm7 = accumulator
;;----------------------------------------------------------------------

cglobal R_DrawSpan_8_MMX
R_DrawSpan_8_MMX:
		push		ebp						;; preserve caller's stack frame pointer
		push		esi						;; preserve register variables
		push		edi
		push		ebx

;;
;; esi = ds_source
;; ebx = ds_colormap
;;
		mov			esi, [ds_source]
		mov			ebx, [ds_colormap]

;;
;; edi = ylookup[ds_y] + columnofs[ds_x1]
;;
		mov			eax, [ds_y]
		mov			edi, [ylookup + eax*4]
		mov			edx, [ds_x1]
		add			edi, [columnofs + edx*4]

;;
;; ecx = ds_x2 - ds_x1 + 1
;;
		mov			ecx, [ds_x2]
		sub			ecx, edx
		add			ecx, 1

;;
;; Needed for fracs and steps
;;
		movd		mm7, [nflatshiftup]

;;
;; mm3 = xstep
;;
		movd		mm3, [ds_xstep]
		pslld		mm3, mm7
		punpckldq	mm3, mm3

;;
;; mm4 = ystep
;;
		movd		mm4, [ds_ystep]
		pslld		mm4, mm7
		punpckldq	mm4, mm4

;;
;; mm1 = pair of consecutive xpositions
;;
		movd		mm1, [ds_xfrac]
		pslld		mm1, mm7
		movq		mm6, mm1
		paddd		mm6, mm3
		punpckldq	mm1, mm6

;;
;; mm2 = pair of consecutive ypositions
;;
		movd		mm2, [ds_yfrac]
		pslld		mm2, mm7
		movq		mm6, mm2
		paddd		mm6, mm4
		punpckldq	mm2, mm6

;;
;; mm5 = nflatxshift
;; mm6 = nflatyshift
;;
		movd		mm5, [nflatxshift]
		movd		mm6, [nflatyshift]

;;
;; Mask is in memory due to lack of registers.
;;
		mov			eax, [nflatmask]
		mov			[nflatmask64], eax
		mov			[nflatmask64 + 4], eax


;;
;; Go until we reach a dword boundary.
;;
.unaligned:
		test		edi, 3
		jz			.alignedprep
.stragglers:
		cmp			ecx, 0
		je			.done					;; If ecx == 0, we're finished.

;;
;; eax = ((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift)
;;
		movq		mm0, mm1				;; mm0 = xposition
		movq		mm7, mm2				;; mm7 = yposition
		paddd		mm1, mm3				;; xposition += xstep (once!)
		paddd		mm2, mm4				;; yposition += ystep (once!)
		psrld		mm0, mm5				;; shift
		psrld		mm7, mm6				;; shift
		pand		mm7, [nflatmask64]		;; mask
		por			mm0, mm7				;; or x and y together

		movd		eax, mm0				;; eax = index of first pixel
		movzx		eax, byte [esi + eax]	;; al = source[eax]
		movzx		eax, byte [ebx + eax]	;; al = colormap[al]

		mov			[edi], al
		add			edi, 1

		sub			ecx, 1
		jmp			.unaligned


.alignedprep:
;;
;; We can double the steps now.
;;
		pslld		mm3, 1
		pslld		mm4, 1


;;
;; Generate chunks of four pixels.
;;
.alignedloop:

;;
;; Make sure we have at least four pixels.
;;
		cmp			ecx, 4
		jl			.prestragglers

;;
;; First two pixels.
;;
		movq		mm0, mm1				;; mm0 = xposition
		movq		mm7, mm2				;; mm7 = yposition
		paddd		mm1, mm3				;; xposition += xstep
		paddd		mm2, mm4				;; yposition += ystep
		psrld		mm0, mm5				;; shift
		psrld		mm7, mm6				;; shift
		pand		mm7, [nflatmask64]		;; mask
		por			mm0, mm7				;; or x and y together

		movd		eax, mm0				;; eax = index of first pixel
		movzx		eax, byte [esi + eax]	;; al = source[eax]
		movzx		ebp, byte [ebx + eax]	;; ebp = colormap[al]

		punpckhdq	mm0, mm0				;; both dwords = high dword
		movd		eax, mm0				;; eax = index of second pixel
		movzx		eax, byte [esi + eax]	;; al = source[eax]
		movzx		eax, byte [ebx + eax]	;; al = colormap[al]
		shl			eax, 8					;; get pixel in right byte
		or			ebp, eax				;; put pixel in ebp

;;
;; Next two pixels.
;;
		movq		mm0, mm1				;; mm0 = xposition
		movq		mm7, mm2				;; mm7 = yposition
		paddd		mm1, mm3				;; xposition += xstep
		paddd		mm2, mm4				;; yposition += ystep
		psrld		mm0, mm5				;; shift
		psrld		mm7, mm6				;; shift
		pand		mm7, [nflatmask64]		;; mask
		por			mm0, mm7				;; or x and y together

		movd		eax, mm0				;; eax = index of third pixel
		movzx		eax, byte [esi + eax]	;; al = source[eax]
		movzx		eax, byte [ebx + eax]	;; al = colormap[al]
		shl			eax, 16					;; get pixel in right byte
		or			ebp, eax				;; put pixel in ebp

		punpckhdq	mm0, mm0				;; both dwords = high dword
		movd		eax, mm0				;; eax = index of second pixel
		movzx		eax, byte [esi + eax]	;; al = source[eax]
		movzx		eax, byte [ebx + eax]	;; al = colormap[al]
		shl			eax, 24					;; get pixel in right byte
		or			ebp, eax				;; put pixel in ebp

;;
;; Write pixels.
;;
		mov			[edi], ebp
		add			edi, 4

		sub			ecx, 4
		jmp			.alignedloop

.prestragglers:
;;
;; Back to one step at a time.
;;
		psrad		mm3, 1
		psrad		mm4, 1
		jmp			.stragglers

.done:
;;
;; Clear MMX state, or else FPU operations will go badly awry.
;;
		emms

		pop			ebx
		pop			edi
		pop			esi
		pop			ebp
		ret

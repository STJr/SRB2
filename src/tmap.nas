;; SONIC ROBO BLAST 2
;;-----------------------------------------------------------------------------
;; Copyright (C) 1998-2000 by DooM Legacy Team.
;; Copyright (C) 1999-2020 by Sonic Team Junior.
;;
;; This program is free software distributed under the
;; terms of the GNU General Public License, version 2.
;; See the 'LICENSE' file for more details.
;;-----------------------------------------------------------------------------
;; FILE:
;;      tmap.nas
;; DESCRIPTION:
;;      Assembler optimised rendering code for software mode.
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
cextern dc_x
cextern dc_yl
cextern dc_yh
cextern ylookup
cextern columnofs
cextern dc_source
cextern dc_texturemid
cextern dc_texheight
cextern dc_iscale
cextern dc_hires
cextern centery
cextern centeryfrac
cextern dc_colormap
cextern dc_transmap
cextern colormaps
cextern vid
cextern topleft

; DELME
cextern R_DrawColumn_8

; polygon edge rasterizer
cextern prastertab

[SECTION .data]

;;.align        4
loopcount       dd      0
pixelcount      dd      0
tystep          dd      0

[SECTION .text]

;;----------------------------------------------------------------------
;;
;; R_DrawColumn : 8bpp column drawer
;;
;; New  optimised version 10-01-1998 by D.Fabrice and P.Boris
;; Revised by G. Dick July 2010 to support the intervening twelve years'
;; worth of changes to the renderer. Since I only vaguely know what I'm
;; doing, this is probably rather suboptimal. Help appreciated!
;;
;;----------------------------------------------------------------------
;; fracstep, vid.width in memory
;; eax = accumulator
;; ebx = colormap
;; ecx = count
;; edx = heightmask
;; esi = source
;; edi = dest
;; ebp = frac
;;----------------------------------------------------------------------

cglobal R_DrawColumn_8_ASM
;       align   16
R_DrawColumn_8_ASM:
        push    ebp                     ;; preserve caller's stack frame pointer
        push    esi                     ;; preserve register variables
        push    edi
        push    ebx
;;
;; dest = ylookup[dc_yl] + columnofs[dc_x];
;;
        mov     ebp,[dc_yl]
        mov     edi,[ylookup+ebp*4]
        mov     ebx,[dc_x]
        add     edi,[columnofs+ebx*4]  ;; edi = dest
;;
;; pixelcount = yh - yl + 1
;;
        mov     ecx,[dc_yh]
        add     ecx,1
        sub     ecx,ebp                 ;; pixel count
        jle     near .done              ;; nothing to scale
;;
;; fracstep = dc_iscale;	// But we just use [dc_iscale]
;; frac = (dc_texturemid + FixedMul((dc_yl << FRACBITS) - centeryfrac, fracstep));
;;
        mov     eax,ebp                 ;; dc_yl
        shl     eax,FRACBITS
        sub     eax,[centeryfrac]
        imul    dword [dc_iscale]
        shrd    eax,edx,FRACBITS
        add     eax,[dc_texturemid]
        mov     ebp,eax                 ;; ebp = frac

        mov     ebx,[dc_colormap]

        mov     esi,[dc_source]
;;
;; if (dc_hires) frac = 0;
;;
        test    byte [dc_hires],0x01
        jz      .texheightcheck
        xor     ebp,ebp

;;
;; Check for power of two
;;
.texheightcheck:
        mov     edx,[dc_texheight]
        sub     edx,1                   ;; edx = heightmask
        test    edx,[dc_texheight]
        jnz     .notpowertwo

        test    ecx,0x01                ;; Test for odd no. pixels
        jnz     .odd

;;
;; Texture height is a power of two, so we get modular arithmetic by
;; masking
;;
.powertwo:
        mov     eax,ebp                 ;; eax = frac
        sar     eax,FRACBITS            ;; Integer part
        and     eax,edx                 ;; eax &= heightmask
        movzx   eax,byte [esi + eax]    ;; eax = texel
        add     ebp,[dc_iscale]         ;; frac += fracstep
        movzx   eax,byte [ebx+eax]      ;; Map through colormap
        mov     [edi],al                ;; Write pixel
                                        ;; dest += vid.width
        add     edi,[vid + viddef_s.width]

.odd:
        mov     eax,ebp                 ;; eax = frac
        sar     eax,FRACBITS            ;; Integer part
        and     eax,edx                 ;; eax &= heightmask
        movzx   eax,byte [esi + eax]    ;; eax = texel
        add     ebp,[dc_iscale]         ;; frac += fracstep
        movzx   eax,byte [ebx+eax]      ;; Map through colormap
        mov     [edi],al                ;; Write pixel
                                        ;; dest += vid.width
        add     edi,[vid + viddef_s.width]


        sub     ecx,2                   ;; count -= 2
        jg      .powertwo

        jmp     .done

.notpowertwo:
        add     edx,1
        shl     edx,FRACBITS
        test    ebp,ebp
        jns     .notpowtwoloop

.makefracpos:
        add     ebp,edx                 ;; frac is negative; make it positive
        js      .makefracpos

.notpowtwoloop:
        cmp     ebp,edx                 ;; Reduce mod height
        jl      .writenonpowtwo
        sub     ebp,edx
        jmp     .notpowtwoloop

.writenonpowtwo:
        mov     eax,ebp                 ;; eax = frac
        sar     eax,FRACBITS            ;; Integer part.
        mov     bl,[esi + eax]          ;; ebx = colormap + texel
        add     ebp,[dc_iscale]         ;; frac += fracstep
        movzx   eax,byte [ebx]          ;; Map through colormap
        mov     [edi],al                ;; Write pixel
                                        ;; dest += vid.width
        add     edi,[vid + viddef_s.width]

        sub     ecx,1
        jnz     .notpowtwoloop

;;

.done:
        pop     ebx                     ;; restore register variables
        pop     edi
        pop     esi
        pop     ebp                     ;; restore caller's stack frame pointer
        ret


;;----------------------------------------------------------------------
;;
;; R_Draw2sMultiPatchColumn : Like R_DrawColumn, but omits transparent
;;                            pixels.
;;
;; New  optimised version 10-01-1998 by D.Fabrice and P.Boris
;; Revised by G. Dick July 2010 to support the intervening twelve years'
;; worth of changes to the renderer. Since I only vaguely know what I'm
;; doing, this is probably rather suboptimal. Help appreciated!
;;
;;----------------------------------------------------------------------
;; fracstep, vid.width in memory
;; eax = accumulator
;; ebx = colormap
;; ecx = count
;; edx = heightmask
;; esi = source
;; edi = dest
;; ebp = frac
;;----------------------------------------------------------------------

cglobal R_Draw2sMultiPatchColumn_8_ASM
;       align   16
R_Draw2sMultiPatchColumn_8_ASM:
        push    ebp                     ;; preserve caller's stack frame pointer
        push    esi                     ;; preserve register variables
        push    edi
        push    ebx
;;
;; dest = ylookup[dc_yl] + columnofs[dc_x];
;;
        mov     ebp,[dc_yl]
        mov     edi,[ylookup+ebp*4]
        mov     ebx,[dc_x]
        add     edi,[columnofs+ebx*4]  ;; edi = dest
;;
;; pixelcount = yh - yl + 1
;;
        mov     ecx,[dc_yh]
        add     ecx,1
        sub     ecx,ebp                 ;; pixel count
        jle     near .done              ;; nothing to scale
;;
;; fracstep = dc_iscale;	// But we just use [dc_iscale]
;; frac = (dc_texturemid + FixedMul((dc_yl << FRACBITS) - centeryfrac, fracstep));
;;
        mov     eax,ebp                 ;; dc_yl
        shl     eax,FRACBITS
        sub     eax,[centeryfrac]
        imul    dword [dc_iscale]
        shrd    eax,edx,FRACBITS
        add     eax,[dc_texturemid]
        mov     ebp,eax                 ;; ebp = frac

        mov     ebx,[dc_colormap]

        mov     esi,[dc_source]
;;
;; if (dc_hires) frac = 0;
;;
        test    byte [dc_hires],0x01
        jz      .texheightcheck
        xor     ebp,ebp

;;
;; Check for power of two
;;
.texheightcheck:
        mov     edx,[dc_texheight]
        sub     edx,1                   ;; edx = heightmask
        test    edx,[dc_texheight]
        jnz     .notpowertwo

        test    ecx,0x01                ;; Test for odd no. pixels
        jnz     .odd

;;
;; Texture height is a power of two, so we get modular arithmetic by
;; masking
;;
.powertwo:
        mov     eax,ebp                 ;; eax = frac
        sar     eax,FRACBITS            ;; Integer part
        and     eax,edx                 ;; eax &= heightmask
        movzx   eax,byte [esi + eax]    ;; eax = texel
        add     ebp,[dc_iscale]         ;; frac += fracstep
        cmp     al,TRANSPARENTPIXEL     ;; Is pixel transparent?
        je      .nextpowtwoeven         ;; If so, advance.
        movzx   eax,byte [ebx+eax]      ;; Map through colormap
        mov	    [edi],al                ;; Write pixel
.nextpowtwoeven:
                                        ;; dest += vid.width
        add     edi,[vid + viddef_s.width]

.odd:
        mov     eax,ebp                 ;; eax = frac
        sar     eax,FRACBITS            ;; Integer part
        and     eax,edx                 ;; eax &= heightmask
        movzx   eax,byte [esi + eax]    ;; eax = texel
        add     ebp,[dc_iscale]         ;; frac += fracstep
        cmp     al,TRANSPARENTPIXEL     ;; Is pixel transparent?
        je      .nextpowtwoodd          ;; If so, advance.
        movzx   eax,byte [ebx+eax]      ;; Map through colormap
        mov     [edi],al                ;; Write pixel
.nextpowtwoodd:
                                        ;; dest += vid.width
        add     edi,[vid + viddef_s.width]


        sub     ecx,2                   ;; count -= 2
        jg      .powertwo

        jmp     .done

.notpowertwo:
        add     edx,1
        shl     edx,FRACBITS
        test    ebp,ebp
        jns     .notpowtwoloop

.makefracpos:
        add     ebp,edx                 ;; frac is negative; make it positive
        js      .makefracpos

.notpowtwoloop:
        cmp     ebp,edx                 ;; Reduce mod height
        jl      .writenonpowtwo
        sub     ebp,edx
        jmp     .notpowtwoloop

.writenonpowtwo:
        mov     eax,ebp                 ;; eax = frac
        sar     eax,FRACBITS            ;; Integer part.
        mov     bl,[esi + eax]          ;; ebx = colormap + texel
        add     ebp,[dc_iscale]         ;; frac += fracstep
        cmp     bl,TRANSPARENTPIXEL     ;; Is pixel transparent?
        je      .nextnonpowtwo          ;; If so, advance.
        movzx   eax,byte [ebx]          ;; Map through colormap
        mov     [edi],al                ;; Write pixel
.nextnonpowtwo:
                                        ;; dest += vid.width
        add     edi,[vid + viddef_s.width]

        sub     ecx,1
        jnz     .notpowtwoloop

;;

.done:
        pop     ebx                     ;; restore register variables
        pop     edi
        pop     esi
        pop     ebp                     ;; restore caller's stack frame pointer
        ret

;;----------------------------------------------------------------------
;; R_DrawTranslucentColumnA_8
;;
;; Vertical column texture drawer, with transparency. Replaces Doom2's
;; 'fuzz' effect, which was not so beautiful.
;; Transparency is always impressive in some way, don't know why...
;;----------------------------------------------------------------------

cglobal R_DrawTranslucentColumn_8_ASM
R_DrawTranslucentColumn_8_ASM:
        push    ebp                     ;; preserve caller's stack frame pointer
        push    esi                     ;; preserve register variables
        push    edi
        push    ebx
;;
;; dest = ylookup[dc_yl] + columnofs[dc_x];
;;
        mov     ebp,[dc_yl]
        mov     ebx,ebp
        mov     edi,[ylookup+ebx*4]
        mov     ebx,[dc_x]
        add     edi,[columnofs+ebx*4]   ;; edi = dest
;;
;; pixelcount = yh - yl + 1
;;
        mov     eax,[dc_yh]
        inc     eax
        sub     eax,ebp                 ;; pixel count
        mov     [pixelcount],eax        ;; save for final pixel
        jle     near    vtdone         ;; nothing to scale
;;
;; frac = dc_texturemid - (centery-dc_yl)*fracstep;
;;
        mov     ecx,[dc_iscale]        ;; fracstep
        mov     eax,[centery]
        sub     eax,ebp
        imul    eax,ecx
        mov     edx,[dc_texturemid]
        sub     edx,eax
        mov     ebx,edx

        shr     ebx,16                  ;; frac int.
        and     ebx,0x7f
        shl     edx,16                  ;; y frac up

        mov     ebp,ecx
        shl     ebp,16                  ;; fracstep f. up
        shr     ecx,16                  ;; fracstep i. ->cl
        and     cl,0x7f
        push    cx
        mov     ecx,edx
        pop     cx
        mov     edx,[dc_colormap]
        mov     esi,[dc_source]
;;
;; lets rock :) !
;;
        mov     eax,[pixelcount]
        shr     eax,0x2
        test    byte [pixelcount],0x3
        mov     ch,al                   ;; quad count
        mov     eax,[dc_transmap]
        je      vt4quadloop
;;
;;  do un-even pixel
;;
        test    byte [pixelcount],0x1
        je      trf2

        mov     ah,[esi+ebx]            ;; fetch texel : colormap number
        add     ecx,ebp
        adc     bl,cl
        mov     al,[edi]                ;; fetch dest  : index into colormap
        and     bl,0x7f
        mov     dl,[eax]
        mov     dl,[edx]
        mov     [edi],dl
pf:     add     edi,0x12345678
;;
;;  do two non-quad-aligned pixels
;;
trf2:    test    byte [pixelcount],0x2
        je      trf3

        mov     ah,[esi+ebx]            ;; fetch texel : colormap number
        add     ecx,ebp
        adc     bl,cl
        mov     al,[edi]                ;; fetch dest  : index into colormap
        and     bl,0x7f
        mov     dl,[eax]
        mov     dl,[edx]
        mov     [edi],dl
pg:     add     edi,0x12345678

        mov     ah,[esi+ebx]            ;; fetch texel : colormap number
        add     ecx,ebp
        adc     bl,cl
        mov     al,[edi]                ;; fetch dest  : index into colormap
        and     bl,0x7f
        mov     dl,[eax]
        mov     dl,[edx]
        mov     [edi],dl
ph:     add     edi,0x12345678
;;
;;  test if there was at least 4 pixels
;;
trf3:   test    ch,0xff                 ;; test quad count
        je near vtdone

;;
;; ebp : ystep frac. upper 24 bits
;; edx : y     frac. upper 24 bits
;; ebx : y     i.    lower 7 bits,  masked for index
;; ecx : ch = counter, cl = y step i.
;; eax : colormap aligned 256
;; esi : source texture column
;; edi : dest screen
;;
vt4quadloop:
        mov     ah,[esi+ebx]            ;; fetch texel : colormap number
        mov     [tystep],ebp
pi:     add     edi,0x12345678
        mov     al,[edi]                ;; fetch dest  : index into colormap
pj:     sub     edi,0x12345678
        mov     ebp,edi
pk:     sub     edi,0x12345678
        jmp short inloop
align 4
vtquadloop:
        add     ecx,[tystep]
        adc     bl,cl
q1:     add     ebp,0x23456789
        and     bl,0x7f
        mov     dl,[eax]
        mov     ah,[esi+ebx]            ;; fetch texel : colormap number
        mov     dl,[edx]
        mov     [edi],dl
        mov     al,[ebp]                ;; fetch dest   : index into colormap
inloop:
        add     ecx,[tystep]
        adc     bl,cl
q2:     add     edi,0x23456789
        and     bl,0x7f
        mov     dl,[eax]
        mov     ah,[esi+ebx]            ;; fetch texel : colormap number
        mov     dl,[edx]
        mov     [ebp+0x0],dl
        mov     al,[edi]                ;; fetch dest   : index into colormap

        add     ecx,[tystep]
        adc     bl,cl
q3:     add     ebp,0x23456789
        and     bl,0x7f
        mov     dl,[eax]
        mov     ah,[esi+ebx]            ;; fetch texel : colormap number
        mov     dl,[edx]
        mov     [edi],dl
        mov     al,[ebp]                ;; fetch dest   : index into colormap

        add     ecx,[tystep]
        adc     bl,cl
q4:     add     edi,0x23456789
        and     bl,0x7f
        mov     dl,[eax]
        mov     ah,[esi+ebx]            ;; fetch texel : colormap number
        mov     dl,[edx]
        mov     [ebp],dl
        mov     al,[edi]                ;; fetch dest   : index into colormap

        dec     ch
        jne     vtquadloop
vtdone:
        pop     ebx
        pop     edi
        pop     esi
        pop     ebp
        ret

;;----------------------------------------------------------------------
;; R_DrawShadeColumn
;;
;;   for smoke..etc.. test.
;;----------------------------------------------------------------------
cglobal R_DrawShadeColumn_8_ASM
R_DrawShadeColumn_8_ASM:
        push    ebp                     ;; preserve caller's stack frame pointer
        push    esi                     ;; preserve register variables
        push    edi
        push    ebx

;;
;; dest = ylookup[dc_yl] + columnofs[dc_x];
;;
        mov     ebp,[dc_yl]
        mov     ebx,ebp
        mov     edi,[ylookup+ebx*4]
        mov     ebx,[dc_x]
        add     edi,[columnofs+ebx*4]  ;; edi = dest
;;
;; pixelcount = yh - yl + 1
;;
        mov     eax,[dc_yh]
        inc     eax
        sub     eax,ebp                 ;; pixel count
        mov     [pixelcount],eax       ;; save for final pixel
        jle near shdone                ;; nothing to scale
;;
;; frac = dc_texturemid - (centery-dc_yl)*fracstep;
;;
        mov     ecx,[dc_iscale]        ;; fracstep
        mov     eax,[centery]
        sub     eax,ebp
        imul    eax,ecx
        mov     edx,[dc_texturemid]
        sub     edx,eax
        mov     ebx,edx
        shr     ebx,16                  ;; frac int.
        and     ebx,byte +0x7f
        shl     edx,16                  ;; y frac up

        mov     ebp,ecx
        shl     ebp,16                  ;; fracstep f. up
        shr     ecx,16                  ;; fracstep i. ->cl
        and     cl,0x7f

        mov     esi,[dc_source]
;;
;; lets rock :) !
;;
        mov     eax,[pixelcount]
        mov     dh,al
        shr     eax,2
        mov     ch,al                   ;; quad count
        mov     eax,[colormaps]
        test    dh,3
        je      sh4quadloop
;;
;;  do un-even pixel
;;
        test    dh,0x1
        je      shf2

        mov     ah,[esi+ebx]            ;; fetch texel : colormap number
        add     edx,ebp
        adc     bl,cl
        mov     al,[edi]                ;; fetch dest  : index into colormap
        and     bl,0x7f
        mov     dl,[eax]
        mov     [edi],dl
pl:     add     edi,0x12345678
;;
;;  do two non-quad-aligned pixels
;;
shf2:
        test    dh,0x2
        je      shf3

        mov     ah,[esi+ebx]            ;; fetch texel : colormap number
        add     edx,ebp
        adc     bl,cl
        mov     al,[edi]                ;; fetch dest  : index into colormap
        and     bl,0x7f
        mov     dl,[eax]
        mov     [edi],dl
pm:     add     edi,0x12345678

        mov     ah,[esi+ebx]            ;; fetch texel : colormap number
        add     edx,ebp
        adc     bl,cl
        mov     al,[edi]                ;; fetch dest  : index into colormap
        and     bl,0x7f
        mov     dl,[eax]
        mov     [edi],dl
pn:     add     edi,0x12345678
;;
;;  test if there was at least 4 pixels
;;
shf3:
        test    ch,0xff                 ;; test quad count
        je near shdone

;;
;; ebp : ystep frac. upper 24 bits
;; edx : y     frac. upper 24 bits
;; ebx : y     i.    lower 7 bits,  masked for index
;; ecx : ch = counter, cl = y step i.
;; eax : colormap aligned 256
;; esi : source texture column
;; edi : dest screen
;;
sh4quadloop:
        mov     dh,0x7f                 ;; prep mask
        mov     ah,[esi+ebx]            ;; fetch texel : colormap number
        mov     [tystep],ebp
po:     add     edi,0x12345678
        mov     al,[edi]                ;; fetch dest  : index into colormap
pp:     sub     edi,0x12345678
        mov     ebp,edi
pq:     sub     edi,0x12345678
        jmp short shinloop

align  4
shquadloop:
        add     edx,[tystep]
        adc     bl,cl
        and     bl,dh
q5:     add     ebp,0x12345678
        mov     dl,[eax]
        mov     ah,[esi+ebx]            ;; fetch texel : colormap number
        mov     [edi],dl
        mov     al,[ebp]                ;; fetch dest : index into colormap
shinloop:
        add     edx,[tystep]
        adc     bl,cl
        and     bl,dh
q6:     add     edi,0x12345678
        mov     dl,[eax]
        mov     ah,[esi+ebx]            ;; fetch texel : colormap number
        mov     [ebp],dl
        mov     al,[edi]                ;; fetch dest : index into colormap

        add     edx,[tystep]
        adc     bl,cl
        and     bl,dh
q7:     add     ebp,0x12345678
        mov     dl,[eax]
        mov     ah,[esi+ebx]            ;; fetch texel : colormap number
        mov     [edi],dl
        mov     al,[ebp]                ;; fetch dest : index into colormap

        add     edx,[tystep]
        adc     bl,cl
        and     bl,dh
q8:     add     edi,0x12345678
        mov     dl,[eax]
        mov     ah,[esi+ebx]            ;; fetch texel : colormap number
        mov     [ebp],dl
        mov     al,[edi]                ;; fetch dest : index into colormap

        dec     ch
        jne     shquadloop

shdone:
        pop     ebx                     ;; restore register variables
        pop     edi
        pop     esi
        pop     ebp                     ;; restore caller's stack frame pointer
        ret


;; ========================================================================
;;  Rasterization of the segments of a LINEAR polygne textur of manire.
;;  It is thus a question of interpolating coordinate them at the edges of texture in
;;  the time that the X-coordinates minx/maxx for each line.
;;  the argument ' dir' indicates which edges of texture are Interpol?:
;;    0:  segments associs at edge TOP? and BOTTOM? (constant TY)
;;    1:  segments associs at the LEFT and RIGHT edge (constant TX)
;; ========================================================================
;;
;;  void   rasterize_segment_tex( LONG x1, LONG y1, LONG x2, LONG y2, LONG tv1, LONG tv2, LONG tc, LONG dir );
;;                                   ARG1     ARG2     ARG3     ARG4      ARG5      ARG6     ARG7       ARG8
;;
;;  Pour dir = 0, (tv1,tv2) = (tX1,tX2), tc = tY, en effet TY est constant.
;;
;;  Pour dir = 1, (tv1,tv2) = (tY1,tY2), tc = tX, en effet TX est constant.
;;
;;
;;  Uses:  extern struct rastery *_rastertab;
;;

MINX            EQU    0
MAXX            EQU    4
TX1             EQU    8
TY1             EQU    12
TX2             EQU    16
TY2             EQU    20
RASTERY_SIZEOF  EQU    24

cglobal rasterize_segment_tex_asm
rasterize_segment_tex_asm:
        push    ebp
        mov     ebp,esp

        sub     esp,byte +0x8           ;; allocate the local variables

        push    ebx
        push    esi
        push    edi
        o16 mov ax,es
        push    eax

;;        #define DX       [ebp-4]
;;        #define TD       [ebp-8]

        mov     eax,[ebp+0xc]           ;; y1
        mov     ebx,[ebp+0x14]          ;; y2
        cmp     ebx,eax
        je near .L_finished             ;; special (y1==y2) segment horizontal, exit!

        jg near .L_rasterize_right

;;rasterize_left:       ;; one rasterize a segment LEFT of the polygne

        mov     ecx,eax
        sub     ecx,ebx
        inc     ecx                     ;; y1-y2+1

        mov     eax,RASTERY_SIZEOF
        mul     ebx                     ;; * y2
        mov     esi,[prastertab]
        add     esi,eax                 ;; point into rastertab[y2]

        mov     eax,[ebp+0x8]           ;; ARG1
        sub     eax,[ebp+0x10]          ;; ARG3
        shl     eax,0x10                ;;     ((x1-x2)<<PRE) ...
        cdq
        idiv    ecx                     ;; dx =     ...        / (y1-y2+1)
        mov     [ebp-0x4],eax           ;; DX

        mov     eax,[ebp+0x18]          ;; ARG5
        sub     eax,[ebp+0x1c]          ;; ARG6
        shl     eax,0x10
        cdq
        idiv    ecx                     ;;      tdx =((tx1-tx2)<<PRE) / (y1-y2+1)
        mov     [ebp-0x8],eax           ;; idem tdy =((ty1-ty2)<<PRE) / (y1-y2+1)

        mov     eax,[ebp+0x10]          ;; ARG3
        shl     eax,0x10                ;; x = x2<<PRE

        mov     ebx,[ebp+0x1c]          ;; ARG6
        shl     ebx,0x10                ;; tx = tx2<<PRE    d0
                                        ;; ty = ty2<<PRE    d1
        mov     edx,[ebp+0x20]          ;; ARG7
        shl     edx,0x10                ;; ty = ty<<PRE     d0
                                        ;; tx = tx<<PRE     d1
        push    ebp
        mov     edi,[ebp-0x4]           ;; DX
        cmp     dword [ebp+0x24],byte +0x0      ;; ARG8   direction ?

        mov     ebp,[ebp-0x8]           ;; TD
        je      .L_rleft_h_loop
;;
;; TY varies, TX is constant
;;
.L_rleft_v_loop:
        mov     [esi+MINX],eax           ;; rastertab[y].minx = x
          add     ebx,ebp
        mov     [esi+TX1],edx           ;;             .tx1  = tx
          add     eax,edi
        mov     [esi+TY1],ebx           ;;             .ty1  = ty

        ;;addl    DX, %eax        // x     += dx
        ;;addl    TD, %ebx        // ty    += tdy

        add     esi,RASTERY_SIZEOF      ;; next raster line into rastertab[]
        dec     ecx
        jne     .L_rleft_v_loop
        pop     ebp
        jmp     .L_finished
;;
;; TX varies, TY is constant
;;
.L_rleft_h_loop:
        mov     [esi+MINX],eax           ;; rastertab[y].minx = x
          add     eax,edi
        mov     [esi+TX1],ebx           ;;             .tx1  = tx
          add     ebx,ebp
        mov     [esi+TY1],edx           ;;             .ty1  = ty

        ;;addl    DX, %eax        // x     += dx
        ;;addl    TD, %ebx        // tx    += tdx

        add     esi,RASTERY_SIZEOF      ;; next raster line into rastertab[]
        dec     ecx
        jne     .L_rleft_h_loop
        pop     ebp
        jmp     .L_finished
;;
;; one rasterize a segment LINE of the polygne
;;
.L_rasterize_right:
        mov     ecx,ebx
        sub     ecx,eax
        inc     ecx                     ;; y2-y1+1

        mov     ebx,RASTERY_SIZEOF
        mul     ebx                     ;;   * y1
        mov     esi,[prastertab]
        add     esi,eax                 ;;  point into rastertab[y1]

        mov     eax,[ebp+0x10]          ;; ARG3
        sub     eax,[ebp+0x8]           ;; ARG1
        shl     eax,0x10                ;; ((x2-x1)<<PRE) ...
        cdq
        idiv    ecx                     ;;  dx =     ...        / (y2-y1+1)
        mov     [ebp-0x4],eax           ;; DX

        mov     eax,[ebp+0x1c]          ;; ARG6
        sub     eax,[ebp+0x18]          ;; ARG5
        shl     eax,0x10
        cdq
        idiv    ecx                     ;;       tdx =((tx2-tx1)<<PRE) / (y2-y1+1)
        mov     [ebp-0x8],eax           ;;  idem tdy =((ty2-ty1)<<PRE) / (y2-y1+1)

        mov     eax,[ebp+0x8]           ;; ARG1
        shl     eax,0x10                ;; x  = x1<<PRE

        mov     ebx,[ebp+0x18]          ;; ARG5
        shl     ebx,0x10                ;; tx = tx1<<PRE    d0
                                        ;; ty = ty1<<PRE    d1
        mov     edx,[ebp+0x20]          ;; ARG7
        shl     edx,0x10                ;; ty = ty<<PRE     d0
                                        ;; tx = tx<<PRE     d1
        push    ebp
        mov     edi,[ebp-0x4]           ;; DX

        cmp     dword [ebp+0x24], 0     ;; direction ?

         mov     ebp,[ebp-0x8]          ;; TD
        je      .L_rright_h_loop
;;
;; TY varies, TX is constant
;;
.L_rright_v_loop:

        mov     [esi+MAXX],eax           ;; rastertab[y].maxx = x
          add     ebx,ebp
        mov     [esi+TX2],edx          ;;             .tx2  = tx
          add     eax,edi
        mov     [esi+TY2],ebx          ;;             .ty2  = ty

        ;;addl    DX, %eax        // x     += dx
        ;;addl    TD, %ebx        // ty    += tdy

        add     esi,RASTERY_SIZEOF
        dec     ecx
        jne     .L_rright_v_loop

        pop     ebp

        jmp     short .L_finished
;;
;; TX varies, TY is constant
;;
.L_rright_h_loop:
        mov     [esi+MAXX],eax           ;; rastertab[y].maxx = x
          add     eax,edi
        mov     [esi+TX2],ebx          ;;             .tx2  = tx
          add     ebx,ebp
        mov     [esi+TY2],edx          ;;             .ty2  = ty

        ;;addl    DX, %eax        // x     += dx
        ;;addl    TD, %ebx        // tx    += tdx

        add     esi,RASTERY_SIZEOF
        dec     ecx
        jne     .L_rright_h_loop

        pop     ebp

.L_finished:
        pop     eax
        o16 mov es,ax
        pop     edi
        pop     esi
        pop     ebx

        mov     esp,ebp
        pop     ebp
        ret

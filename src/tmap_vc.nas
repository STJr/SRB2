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
;;      tmap_vc.nas
;; DESCRIPTION:
;;      Assembler optimised math code for Visual C++.


[BITS 32]

%macro cglobal 1
%define %1 _%1
[global %1]
%endmacro

[SECTION .text write]

;----------------------------------------------------------------------------
;fixed_t FixedMul (fixed_t a, fixed_t b)
;----------------------------------------------------------------------------
cglobal FixedMul
;       align   16
FixedMul:
        mov     eax,[esp+4]
        imul    dword [esp+8]
        shrd    eax,edx,16
        ret

;----------------------------------------------------------------------------
;fixed_t FixedDiv2 (fixed_t a, fixed_t b);
;----------------------------------------------------------------------------
cglobal FixedDiv2
;       align   16
FixedDiv2:
        mov     eax,[esp+4]
        mov     edx,eax                 ;; these two instructions allow the next
        sar     edx,31                  ;; two to pair, on the Pentium processor.
        shld    edx,eax,16
        sal     eax,16
        idiv    dword [esp+8]
        ret

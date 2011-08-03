;*****************************************************************************
;* sad16-a.asm: x86 high depth sad functions
;*****************************************************************************
;* Copyright (C) 2010-2011 x264 project
;*
;* Authors: Oskar Arvidsson <oskar@irock.se>
;*
;* This program is free software; you can redistribute it and/or modify
;* it under the terms of the GNU General Public License as published by
;* the Free Software Foundation; either version 2 of the License, or
;* (at your option) any later version.
;*
;* This program is distributed in the hope that it will be useful,
;* but WITHOUT ANY WARRANTY; without even the implied warranty of
;* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;* GNU General Public License for more details.
;*
;* You should have received a copy of the GNU General Public License
;* along with this program; if not, write to the Free Software
;* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111, USA.
;*
;* This program is also available under a commercial proprietary license.
;* For more information, contact us at licensing@x264.com.
;*****************************************************************************

%include "x86inc.asm"
%include "x86util.asm"

SECTION .text

cextern pw_1

;=============================================================================
; SAD MMX
;=============================================================================

%macro SAD_INC_1x16P_MMX 0
    movu    m1, [r0+ 0]
    movu    m2, [r0+ 8]
    movu    m3, [r0+16]
    movu    m4, [r0+24]
    psubw   m1, [r2+ 0]
    psubw   m2, [r2+ 8]
    psubw   m3, [r2+16]
    psubw   m4, [r2+24]
    ABSW2   m1, m2, m1, m2, m5, m6
    ABSW2   m3, m4, m3, m4, m7, m5
    lea     r0, [r0+2*r1]
    lea     r2, [r2+2*r3]
    paddw   m1, m2
    paddw   m3, m4
    paddw   m0, m1
    paddw   m0, m3
%endmacro

%macro SAD_INC_2x8P_MMX 0
    movu    m1, [r0+0]
    movu    m2, [r0+8]
    movu    m3, [r0+2*r1+0]
    movu    m4, [r0+2*r1+8]
    psubw   m1, [r2+0]
    psubw   m2, [r2+8]
    psubw   m3, [r2+2*r3+0]
    psubw   m4, [r2+2*r3+8]
    ABSW2   m1, m2, m1, m2, m5, m6
    ABSW2   m3, m4, m3, m4, m7, m5
    lea     r0, [r0+4*r1]
    lea     r2, [r2+4*r3]
    paddw   m1, m2
    paddw   m3, m4
    paddw   m0, m1
    paddw   m0, m3
%endmacro

%macro SAD_INC_2x4P_MMX 0
    movu    m1, [r0]
    movu    m2, [r0+2*r1]
    psubw   m1, [r2]
    psubw   m2, [r2+2*r3]
    ABSW2   m1, m2, m1, m2, m3, m4
    lea     r0, [r0+4*r1]
    lea     r2, [r2+4*r3]
    paddw   m0, m1
    paddw   m0, m2
%endmacro

;-----------------------------------------------------------------------------
; int pixel_sad_NxM( uint16_t *, int, uint16_t *, int )
;-----------------------------------------------------------------------------
%macro SAD_MMX 3
cglobal pixel_sad_%1x%2, 4,4
    pxor    m0, m0
%rep %2/%3
    SAD_INC_%3x%1P_MMX
%endrep
%if %1*%2 == 256
    HADDUW  m0, m1
%else
    HADDW   m0, m1
%endif
    movd   eax, m0
    RET
%endmacro

INIT_MMX mmx2
SAD_MMX 16, 16, 1
SAD_MMX 16,  8, 1
SAD_MMX  8, 16, 2
SAD_MMX  8,  8, 2
SAD_MMX  8,  4, 2
SAD_MMX  4,  8, 2
SAD_MMX  4,  4, 2
INIT_MMX ssse3
SAD_MMX  4,  8, 2
SAD_MMX  4,  4, 2

;=============================================================================
; SAD XMM
;=============================================================================

%macro SAD_INC_2x16P_XMM 0
    movu    m1, [r2+ 0]
    movu    m2, [r2+16]
    movu    m3, [r2+2*r3+ 0]
    movu    m4, [r2+2*r3+16]
    psubw   m1, [r0+ 0]
    psubw   m2, [r0+16]
    psubw   m3, [r0+2*r1+ 0]
    psubw   m4, [r0+2*r1+16]
    ABSW2   m1, m2, m1, m2, m5, m6
    lea     r0, [r0+4*r1]
    lea     r2, [r2+4*r3]
    ABSW2   m3, m4, m3, m4, m7, m5
    paddw   m1, m2
    paddw   m3, m4
    paddw   m0, m1
    paddw   m0, m3
%endmacro

%macro SAD_INC_2x8P_XMM 0
    movu    m1, [r2]
    movu    m2, [r2+2*r3]
    psubw   m1, [r0]
    psubw   m2, [r0+2*r1]
    ABSW2   m1, m2, m1, m2, m3, m4
    lea     r0, [r0+4*r1]
    lea     r2, [r2+4*r3]
    paddw   m0, m1
    paddw   m0, m2
%endmacro

;-----------------------------------------------------------------------------
; int pixel_sad_NxM( uint16_t *, int, uint16_t *, int )
;-----------------------------------------------------------------------------
%macro SAD_XMM 2
cglobal pixel_sad_%1x%2, 4,4,8
    pxor    m0, m0
%rep %2/2
    SAD_INC_2x%1P_XMM
%endrep
    HADDW   m0, m1
    movd   eax, m0
    RET
%endmacro

INIT_XMM sse2
SAD_XMM 16, 16
SAD_XMM 16,  8
SAD_XMM  8, 16
SAD_XMM  8,  8
SAD_XMM  8,  4
INIT_XMM sse2, aligned
SAD_XMM 16, 16
SAD_XMM 16,  8
SAD_XMM  8, 16
SAD_XMM  8,  8
INIT_XMM ssse3
SAD_XMM 16, 16
SAD_XMM 16,  8
SAD_XMM  8, 16
SAD_XMM  8,  8
SAD_XMM  8,  4
INIT_XMM ssse3, aligned
SAD_XMM 16, 16
SAD_XMM 16,  8
SAD_XMM  8, 16
SAD_XMM  8,  8

;=============================================================================
; SAD x3/x4
;=============================================================================

%macro SAD_X3_INC_P 0
    add     r0, 4*FENC_STRIDE
    lea     r1, [r1+4*r4]
    lea     r2, [r2+4*r4]
    lea     r3, [r3+4*r4]
%endmacro

%macro SAD_X3_ONE_START 0
    mova    m3, [r0]
    movu    m0, [r1]
    movu    m1, [r2]
    movu    m2, [r3]
    psubw   m0, m3
    psubw   m1, m3
    psubw   m2, m3
    ABSW2   m0, m1, m0, m1, m4, m5
    ABSW    m2, m2, m6
%endmacro

%macro SAD_X3_ONE 2
    mova    m6, [r0+%1]
    movu    m3, [r1+%2]
    movu    m4, [r2+%2]
    movu    m5, [r3+%2]
    psubw   m3, m6
    psubw   m4, m6
    psubw   m5, m6
    ABSW2   m3, m4, m3, m4, m7, m6
    ABSW    m5, m5, m6
    paddw   m0, m3
    paddw   m1, m4
    paddw   m2, m5
%endmacro

%macro SAD_X3_END 2
%if mmsize == 8 && %1*%2 == 256
    HADDUW   m0, m3
    HADDUW   m1, m4
    HADDUW   m2, m5
%else
    HADDW    m0, m3
    HADDW    m1, m4
    HADDW    m2, m5
%endif
%ifdef UNIX64
    movd [r5+0], m0
    movd [r5+4], m1
    movd [r5+8], m2
%else
    mov      r0, r5mp
    movd [r0+0], m0
    movd [r0+4], m1
    movd [r0+8], m2
%endif
    RET
%endmacro

%macro SAD_X4_INC_P 0
    add     r0, 4*FENC_STRIDE
    lea     r1, [r1+4*r5]
    lea     r2, [r2+4*r5]
    lea     r3, [r3+4*r5]
    lea     r4, [r4+4*r5]
%endmacro

%macro SAD_X4_ONE_START 0
    mova    m4, [r0]
    movu    m0, [r1]
    movu    m1, [r2]
    movu    m2, [r3]
    movu    m3, [r4]
    psubw   m0, m4
    psubw   m1, m4
    psubw   m2, m4
    psubw   m3, m4
    ABSW2   m0, m1, m0, m1, m5, m6
    ABSW2   m2, m3, m2, m3, m4, m7
%endmacro

%macro SAD_X4_ONE 2
    mova    m4, [r0+%1]
    movu    m5, [r1+%2]
    movu    m6, [r2+%2]
%if num_mmregs > 8
    movu    m7, [r3+%2]
    movu    m8, [r4+%2]
    psubw   m5, m4
    psubw   m6, m4
    psubw   m7, m4
    psubw   m8, m4
    ABSW2   m5, m6, m5, m6, m9, m10
    ABSW2   m7, m8, m7, m8, m9, m10
    paddw   m0, m5
    paddw   m1, m6
    paddw   m2, m7
    paddw   m3, m8
%elif cpuflag(ssse3)
    movu    m7, [r3+%2]
    psubw   m5, m4
    psubw   m6, m4
    psubw   m7, m4
    movu    m4, [r4+%2]
    pabsw   m5, m5
    psubw   m4, [r0+%1]
    pabsw   m6, m6
    pabsw   m7, m7
    pabsw   m4, m4
    paddw   m0, m5
    paddw   m1, m6
    paddw   m2, m7
    paddw   m3, m4
%else ; num_mmregs == 8 && !ssse3
    psubw   m5, m4
    psubw   m6, m4
    ABSW    m5, m5, m7
    ABSW    m6, m6, m7
    paddw   m0, m5
    paddw   m1, m6
    movu    m5, [r3+%2]
    movu    m6, [r4+%2]
    psubw   m5, m4
    psubw   m6, m4
    ABSW2   m5, m6, m5, m6, m7, m4
    paddw   m2, m5
    paddw   m3, m6
%endif
%endmacro

%macro SAD_X4_END 2
%if mmsize == 8 && %1*%2 == 256
    HADDUW    m0, m4
    HADDUW    m1, m5
    HADDUW    m2, m6
    HADDUW    m3, m7
%else
    HADDW     m0, m4
    HADDW     m1, m5
    HADDW     m2, m6
    HADDW     m3, m7
%endif
    mov       r0, r6mp
    movd [r0+ 0], m0
    movd [r0+ 4], m1
    movd [r0+ 8], m2
    movd [r0+12], m3
    RET
%endmacro

%macro SAD_X_2xNP 4
    %assign x %3
%rep %4
    SAD_X%1_ONE x*mmsize, x*mmsize
    SAD_X%1_ONE 2*FENC_STRIDE+x*mmsize, 2*%2+x*mmsize
    %assign x x+1
%endrep
%endmacro

;-----------------------------------------------------------------------------
; void pixel_sad_xK_MxN( uint16_t *fenc, uint16_t *pix0, uint16_t *pix1,
;                        uint16_t *pix2, int i_stride, int scores[3] )
;-----------------------------------------------------------------------------
%macro SAD_X 3
cglobal pixel_sad_x%1_%2x%3, 6,7,XMM_REGS
    %assign regnum %1+1
    %xdefine STRIDE r %+ regnum
%ifdef WIN64
    movsxd STRIDE, STRIDE %+ d
%endif
    mov     r6, %3/2-1
    SAD_X%1_ONE_START
    SAD_X%1_ONE 2*FENC_STRIDE, 2*STRIDE
    SAD_X_2xNP %1, STRIDE, 1, %2/(mmsize/2)-1
.loop:
    SAD_X%1_INC_P
    dec     r6
    SAD_X_2xNP %1, STRIDE, 0, %2/(mmsize/2)
    jg .loop
%if %1 == 4
    mov     r6, r6m
%endif
    SAD_X%1_END %2, %3
%endmacro

INIT_MMX mmx2
%define XMM_REGS 0
SAD_X 3, 16, 16
SAD_X 3, 16,  8
SAD_X 3,  8, 16
SAD_X 3,  8,  8
SAD_X 3,  8,  4
SAD_X 3,  4,  8
SAD_X 3,  4,  4
SAD_X 4, 16, 16
SAD_X 4, 16,  8
SAD_X 4,  8, 16
SAD_X 4,  8,  8
SAD_X 4,  8,  4
SAD_X 4,  4,  8
SAD_X 4,  4,  4
INIT_MMX ssse3
SAD_X 3,  4,  8
SAD_X 3,  4,  4
SAD_X 4,  4,  8
SAD_X 4,  4,  4
INIT_XMM ssse3
%define XMM_REGS 9
SAD_X 3, 16, 16
SAD_X 3, 16,  8
SAD_X 3,  8, 16
SAD_X 3,  8,  8
SAD_X 3,  8,  4
SAD_X 4, 16, 16
SAD_X 4, 16,  8
SAD_X 4,  8, 16
SAD_X 4,  8,  8
SAD_X 4,  8,  4
INIT_XMM sse2
%define XMM_REGS 11
SAD_X 3, 16, 16
SAD_X 3, 16,  8
SAD_X 3,  8, 16
SAD_X 3,  8,  8
SAD_X 3,  8,  4
SAD_X 4, 16, 16
SAD_X 4, 16,  8
SAD_X 4,  8, 16
SAD_X 4,  8,  8
SAD_X 4,  8,  4

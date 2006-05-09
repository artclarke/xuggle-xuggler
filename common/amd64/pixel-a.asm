;*****************************************************************************
;* pixel.asm: h264 encoder library
;*****************************************************************************
;* Copyright (C) 2003 x264 project
;* $Id: pixel.asm,v 1.1 2004/06/03 19:27:07 fenrir Exp $
;*
;* Authors: Laurent Aimar <fenrir@via.ecp.fr>
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
;* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111, USA.
;*****************************************************************************

BITS 64

;=============================================================================
; Macros and other preprocessor constants
;=============================================================================

%include "amd64inc.asm"

; sad

%macro SAD_INC_2x16P 0
    movq    mm1,    [parm1q]
    movq    mm2,    [parm1q+8]
    movq    mm3,    [parm1q+parm2q]
    movq    mm4,    [parm1q+parm2q+8]
    psadbw  mm1,    [parm3q]
    psadbw  mm2,    [parm3q+8]
    psadbw  mm3,    [parm3q+parm4q]
    psadbw  mm4,    [parm3q+parm4q+8]
    lea     parm1q, [parm1q+2*parm2q]
    paddw   mm1,    mm2
    paddw   mm3,    mm4
    lea     parm3q, [parm3q+2*parm4q]
    paddw   mm0,    mm1
    paddw   mm0,    mm3
%endmacro

%macro SAD_INC_2x8P 0
    movq    mm1,    [parm1q]
    movq    mm2,    [parm1q+parm2q]
    psadbw  mm1,    [parm3q]
    psadbw  mm2,    [parm3q+parm4q]
    lea     parm1q,    [parm1q+2*parm2q]
    paddw   mm0,    mm1
    paddw   mm0,    mm2
    lea     parm3q, [parm3q+2*parm4q]
%endmacro

%macro SAD_INC_2x4P 0
    movd    mm1,    [parm1q]
    movd    mm2,    [parm3q]
    movd    mm3,    [parm1q+parm2q]
    movd    mm4,    [parm3q+parm4q]

    psadbw  mm1,    mm2
    psadbw  mm3,    mm4
    paddw   mm0,    mm1
    paddw   mm0,    mm3

    lea     parm1q, [parm1q+2*parm2q]
    lea     parm3q, [parm3q+2*parm4q]
%endmacro

; sad x3 / x4

%macro SAD_X3_START_1x8P 1
    mov%1   mm3,    [parm1q]
    mov%1   mm0,    [parm2q]
    mov%1   mm1,    [parm3q]
    mov%1   mm2,    [parm4q]
    psadbw  mm0,    mm3
    psadbw  mm1,    mm3
    psadbw  mm2,    mm3
%endmacro

%macro SAD_X3_1x8P 3
    mov%1   mm3,    [parm1q+%2]
    mov%1   mm4,    [parm2q+%3]
    mov%1   mm5,    [parm3q+%3]
    mov%1   mm6,    [parm4q+%3]
    psadbw  mm4,    mm3
    psadbw  mm5,    mm3
    psadbw  mm6,    mm3
    paddw   mm0,    mm4
    paddw   mm1,    mm5
    paddw   mm2,    mm6
%endmacro

%macro SAD_X3_2x16P 1
%if %1
    SAD_X3_START_1x8P q
%else
    SAD_X3_1x8P q, 0, 0
%endif
    SAD_X3_1x8P q, 8, 8
    SAD_X3_1x8P q, FENC_STRIDE, parm5q
    SAD_X3_1x8P q, FENC_STRIDE+8, parm5q+8
    add     parm1q, 2*FENC_STRIDE
    lea     parm2q, [parm2q+2*parm5q]
    lea     parm3q, [parm3q+2*parm5q]
    lea     parm4q, [parm4q+2*parm5q]
%endmacro

%macro SAD_X3_2x8P 1
%if %1
    SAD_X3_START_1x8P q
%else
    SAD_X3_1x8P q, 0, 0
%endif
    SAD_X3_1x8P q, FENC_STRIDE, parm5q
    add     parm1q, 2*FENC_STRIDE
    lea     parm2q, [parm2q+2*parm5q]
    lea     parm3q, [parm3q+2*parm5q]
    lea     parm4q, [parm4q+2*parm5q]
%endmacro

%macro SAD_X3_2x4P 1
%if %1
    SAD_X3_START_1x8P d
%else
    SAD_X3_1x8P d, 0, 0
%endif
    SAD_X3_1x8P d, FENC_STRIDE, parm5q
    add     parm1q, 2*FENC_STRIDE
    lea     parm2q, [parm2q+2*parm5q]
    lea     parm3q, [parm3q+2*parm5q]
    lea     parm4q, [parm4q+2*parm5q]
%endmacro

%macro SAD_X4_START_1x8P 1
    mov%1   mm7,    [parm1q]
    mov%1   mm0,    [parm2q]
    mov%1   mm1,    [parm3q]
    mov%1   mm2,    [parm4q]
    mov%1   mm3,    [parm5q]
    psadbw  mm0,    mm7
    psadbw  mm1,    mm7
    psadbw  mm2,    mm7
    psadbw  mm3,    mm7
%endmacro

%macro SAD_X4_1x8P 2
    movq    mm7,    [parm1q+%1]
    movq    mm4,    [parm2q+%2]
    movq    mm5,    [parm3q+%2]
    movq    mm6,    [parm4q+%2]
    psadbw  mm4,    mm7
    psadbw  mm5,    mm7
    psadbw  mm6,    mm7
    psadbw  mm7,    [parm5q+%2]
    paddw   mm0,    mm4
    paddw   mm1,    mm5
    paddw   mm2,    mm6
    paddw   mm3,    mm7
%endmacro

%macro SAD_X4_1x4P 2
    movd    mm7,    [parm1q+%1]
    movd    mm4,    [parm2q+%2]
    movd    mm5,    [parm3q+%2]
    movd    mm6,    [parm4q+%2]
    psadbw  mm4,    mm7
    psadbw  mm5,    mm7
    paddw   mm0,    mm4
    psadbw  mm6,    mm7
    movd    mm4,    [parm5q+%2]
    paddw   mm1,    mm5
    psadbw  mm4,    mm7
    paddw   mm2,    mm6
    paddw   mm3,    mm4
%endmacro

%macro SAD_X4_2x16P 1
%if %1
    SAD_X4_START_1x8P q
%else
    SAD_X4_1x8P 0, 0
%endif
    SAD_X4_1x8P 8, 8
    SAD_X4_1x8P FENC_STRIDE, parm6q
    SAD_X4_1x8P FENC_STRIDE+8, parm6q+8
    add     parm1q, 2*FENC_STRIDE
    lea     parm2q, [parm2q+2*parm6q]
    lea     parm3q, [parm3q+2*parm6q]
    lea     parm4q, [parm4q+2*parm6q]
    lea     parm5q, [parm5q+2*parm6q]
%endmacro

%macro SAD_X4_2x8P 1
%if %1
    SAD_X4_START_1x8P q
%else
    SAD_X4_1x8P 0, 0
%endif
    SAD_X4_1x8P FENC_STRIDE, parm6q
    add     parm1q, 2*FENC_STRIDE
    lea     parm2q, [parm2q+2*parm6q]
    lea     parm3q, [parm3q+2*parm6q]
    lea     parm4q, [parm4q+2*parm6q]
    lea     parm5q, [parm5q+2*parm6q]
%endmacro

%macro SAD_X4_2x4P 1
%if %1
    SAD_X4_START_1x8P d
%else
    SAD_X4_1x4P 0, 0
%endif
    SAD_X4_1x4P FENC_STRIDE, parm6q
    add     parm1q, 2*FENC_STRIDE
    lea     parm2q, [parm2q+2*parm6q]
    lea     parm3q, [parm3q+2*parm6q]
    lea     parm4q, [parm4q+2*parm6q]
    lea     parm5q, [parm5q+2*parm6q]
%endmacro

%macro SAD_X3_END 0
    movd    [parm6q+0], mm0
    movd    [parm6q+4], mm1
    movd    [parm6q+8], mm2
    ret
%endmacro

%macro SAD_X4_END 0
    mov     rax, parm7q
    movd    [rax+0], mm0
    movd    [rax+4], mm1
    movd    [rax+8], mm2
    movd    [rax+12], mm3
    ret
%endmacro

; ssd

%macro SSD_INC_1x16P 0
    movq    mm1,    [parm1q]
    movq    mm2,    [parm3q]
    movq    mm3,    [parm1q+8]
    movq    mm4,    [parm3q+8]

    movq    mm5,    mm2
    movq    mm6,    mm4
    psubusb mm2,    mm1
    psubusb mm4,    mm3
    psubusb mm1,    mm5
    psubusb mm3,    mm6
    por     mm1,    mm2
    por     mm3,    mm4

    movq    mm2,    mm1
    movq    mm4,    mm3
    punpcklbw mm1,  mm7
    punpcklbw mm3,  mm7
    punpckhbw mm2,  mm7
    punpckhbw mm4,  mm7
    pmaddwd mm1,    mm1
    pmaddwd mm2,    mm2
    pmaddwd mm3,    mm3
    pmaddwd mm4,    mm4

    add     parm1q, parm2q
    add     parm3q, parm4q
    paddd   mm0,    mm1
    paddd   mm0,    mm2
    paddd   mm0,    mm3
    paddd   mm0,    mm4
%endmacro

%macro SSD_INC_1x8P 0
    movq    mm1,    [parm1q]
    movq    mm2,    [parm3q]

    movq    mm5,    mm2
    psubusb mm2,    mm1
    psubusb mm1,    mm5
    por     mm1,    mm2         ; mm1 = 8bit abs diff

    movq    mm2,    mm1
    punpcklbw mm1,  mm7
    punpckhbw mm2,  mm7         ; (mm1,mm2) = 16bit abs diff
    pmaddwd mm1,    mm1
    pmaddwd mm2,    mm2

    add     parm1q, parm2q
    add     parm3q, parm4q
    paddd   mm0,    mm1
    paddd   mm0,    mm2
%endmacro

%macro SSD_INC_1x4P 0
    movd    mm1,    [parm1q]
    movd    mm2,    [parm3q]

    movq    mm5,    mm2
    psubusb mm2,    mm1
    psubusb mm1,    mm5
    por     mm1,    mm2
    punpcklbw mm1,  mm7
    pmaddwd mm1,    mm1

    add     parm1q, parm2q
    add     parm3q, parm4q
    paddd   mm0,    mm1
%endmacro

; satd

%macro LOAD_DIFF_4P 4  ; MMP, MMT, [pix1], [pix2]
    movd        %1, %3
    movd        %2, %4
    punpcklbw   %1, %2
    punpcklbw   %2, %2
    psubw       %1, %2
%endmacro

%macro HADAMARD4_SUB_BADC 4
    paddw %1,   %2
    paddw %3,   %4
    paddw %2,   %2
    paddw %4,   %4
    psubw %2,   %1
    psubw %4,   %3
%endmacro

%macro HADAMARD4x4 4
    HADAMARD4_SUB_BADC %1, %2, %3, %4
    HADAMARD4_SUB_BADC %1, %3, %2, %4
%endmacro

%macro SBUTTERFLYwd 3
    movq        %3, %1
    punpcklwd   %1, %2
    punpckhwd   %3, %2
%endmacro

%macro SBUTTERFLYdq 3
    movq        %3, %1
    punpckldq   %1, %2
    punpckhdq   %3, %2
%endmacro

%macro TRANSPOSE4x4 5   ; abcd-t -> adtc
    SBUTTERFLYwd %1, %2, %5
    SBUTTERFLYwd %3, %4, %2
    SBUTTERFLYdq %1, %3, %4
    SBUTTERFLYdq %5, %2, %3
%endmacro

%macro MMX_ABS 2    ; mma, tmp
    pxor    %2, %2
    psubw   %2, %1
    pmaxsw  %1, %2
%endmacro

%macro MMX_ABS_TWO 4    ; mma, mmb, tmp0, tmp1
    pxor    %3, %3
    pxor    %4, %4
    psubw   %3, %1
    psubw   %4, %2
    pmaxsw  %1, %3
    pmaxsw  %2, %4
%endmacro

%macro HADAMARD4x4_SUM 1    ; %1 = dest (row sum of one block)
    HADAMARD4x4 mm4, mm5, mm6, mm7
    TRANSPOSE4x4 mm4, mm5, mm6, mm7, %1
    HADAMARD4x4 mm4, mm7, %1, mm6
    MMX_ABS_TWO mm4, mm7, mm3, mm5
    MMX_ABS_TWO %1,  mm6, mm3, mm5
    paddw       %1,  mm4
    paddw       mm6, mm7
    pavgw       %1,  mm6
%endmacro

; in: r10=3*stride1, r11=3*stride2
; in: %2 = horizontal offset
; in: %3 = whether we need to increment pix1 and pix2
; clobber: mm3..mm7
; out: %1 = satd
%macro LOAD_DIFF_HADAMARD_SUM 3
    LOAD_DIFF_4P mm4, mm3, [parm1q+%2],          [parm3q+%2]
    LOAD_DIFF_4P mm5, mm3, [parm1q+parm2q+%2],   [parm3q+parm4q+%2]
    LOAD_DIFF_4P mm6, mm3, [parm1q+2*parm2q+%2], [parm3q+2*parm4q+%2]
    LOAD_DIFF_4P mm7, mm3, [parm1q+r10+%2],      [parm3q+r11+%2]
%if %3
    lea  parm1q, [parm1q+4*parm2q]
    lea  parm3q, [parm3q+4*parm4q]
%endif
    HADAMARD4x4_SUM %1
%endmacro

;=============================================================================
; Code
;=============================================================================

SECTION .text

cglobal x264_pixel_sad_16x16_mmxext
cglobal x264_pixel_sad_16x8_mmxext
cglobal x264_pixel_sad_8x16_mmxext
cglobal x264_pixel_sad_8x8_mmxext
cglobal x264_pixel_sad_8x4_mmxext
cglobal x264_pixel_sad_4x8_mmxext
cglobal x264_pixel_sad_4x4_mmxext

cglobal x264_pixel_sad_x3_16x16_mmxext
cglobal x264_pixel_sad_x3_16x8_mmxext
cglobal x264_pixel_sad_x3_8x16_mmxext
cglobal x264_pixel_sad_x3_8x8_mmxext
cglobal x264_pixel_sad_x3_8x4_mmxext
cglobal x264_pixel_sad_x3_4x8_mmxext
cglobal x264_pixel_sad_x3_4x4_mmxext

cglobal x264_pixel_sad_x4_16x16_mmxext
cglobal x264_pixel_sad_x4_16x8_mmxext
cglobal x264_pixel_sad_x4_8x16_mmxext
cglobal x264_pixel_sad_x4_8x8_mmxext
cglobal x264_pixel_sad_x4_8x4_mmxext
cglobal x264_pixel_sad_x4_4x8_mmxext
cglobal x264_pixel_sad_x4_4x4_mmxext

cglobal x264_pixel_sad_pde_16x16_mmxext
cglobal x264_pixel_sad_pde_16x8_mmxext
cglobal x264_pixel_sad_pde_8x16_mmxext

cglobal x264_pixel_ssd_16x16_mmx
cglobal x264_pixel_ssd_16x8_mmx
cglobal x264_pixel_ssd_8x16_mmx
cglobal x264_pixel_ssd_8x8_mmx
cglobal x264_pixel_ssd_8x4_mmx
cglobal x264_pixel_ssd_4x8_mmx
cglobal x264_pixel_ssd_4x4_mmx

cglobal x264_pixel_satd_4x4_mmxext
cglobal x264_pixel_satd_4x8_mmxext
cglobal x264_pixel_satd_8x4_mmxext
cglobal x264_pixel_satd_8x8_mmxext
cglobal x264_pixel_satd_16x8_mmxext
cglobal x264_pixel_satd_8x16_mmxext
cglobal x264_pixel_satd_16x16_mmxext

cglobal x264_intra_satd_x3_4x4_mmxext
cglobal x264_intra_satd_x3_8x8c_mmxext
cglobal x264_intra_satd_x3_16x16_mmxext


%macro SAD_START 0
    pxor    mm0, mm0
%endmacro

%macro SAD_END 0
    movd    eax, mm0
    ret
%endmacro

;-----------------------------------------------------------------------------
;   int x264_pixel_sad_16x16_mmxext (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
%macro SAD 2
ALIGN 16
x264_pixel_sad_%1x%2_mmxext:
    SAD_START
%rep %2/2
    SAD_INC_2x%1P
%endrep 
    SAD_END
%endmacro

SAD 16, 16
SAD 16,  8
SAD  8, 16
SAD  8,  8
SAD  8,  4
SAD  4,  8
SAD  4,  4

;-----------------------------------------------------------------------------
;  void x264_pixel_sad_x3_16x16_mmxext( uint8_t *fenc, uint8_t *pix0, uint8_t *pix1,
;                                       uint8_t *pix2, int i_stride, int scores[3] )
;-----------------------------------------------------------------------------
%macro SAD_X 3
ALIGN 16
x264_pixel_sad_x%1_%2x%3_mmxext:
    SAD_X%1_2x%2P 1
%rep %3/2-1
    SAD_X%1_2x%2P 0
%endrep
    SAD_X%1_END
%endmacro

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


%macro PDE_CHECK 0
    movd eax, mm0
    cmp  eax, parm5d ; prev_score
    jl   .continue
    ret
ALIGN 4
.continue:
%endmacro

;-----------------------------------------------------------------------------
;   int x264_pixel_sad_pde_16x16_mmxext (uint8_t *, int, uint8_t *, int, int )
;-----------------------------------------------------------------------------
%macro SAD_PDE 2    
ALIGN 16
x264_pixel_sad_pde_%1x%2_mmxext:
    SAD_START
%rep %2/4
    SAD_INC_2x%1P
%endrep

    movd eax, mm0
    cmp  eax, parm5d ; prev_score
    jl   .continue
    ret
ALIGN 4
.continue:

%rep %2/4
    SAD_INC_2x%1P
%endrep  
    SAD_END
%endmacro

SAD_PDE 16, 16
SAD_PDE 16 , 8
SAD_PDE  8, 16



%macro SSD_START 0
    pxor    mm7,    mm7         ; zero
    pxor    mm0,    mm0         ; mm0 holds the sum
%endmacro

%macro SSD_END 0
    movq    mm1,    mm0
    psrlq   mm1,    32
    paddd   mm0,    mm1
    movd    eax,    mm0
    ret
%endmacro

;-----------------------------------------------------------------------------
;   int x264_pixel_ssd_16x16_mmx (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
%macro SSD 2
ALIGN 16
x264_pixel_ssd_%1x%2_mmx:
    SSD_START
%rep %2
    SSD_INC_1x%1P
%endrep
    SSD_END
%endmacro

SSD 16, 16
SSD 16,  8
SSD  8, 16
SSD  8,  8
SSD  8,  4
SSD  4,  8
SSD  4,  4



%macro SATD_START 0
    lea  r10, [3*parm2q] ; 3*stride1
    lea  r11, [3*parm4q] ; 3*stride2
%endmacro

%macro SATD_END 0
    pshufw      mm1, mm0, 01001110b
    paddw       mm0, mm1
    pshufw      mm1, mm0, 10110001b
    paddw       mm0, mm1
    movd        eax, mm0
    and         eax, 0xffff
    ret
%endmacro

ALIGN 16
;-----------------------------------------------------------------------------
;   int x264_pixel_satd_4x4_mmxext (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
x264_pixel_satd_4x4_mmxext:
    SATD_START
    LOAD_DIFF_HADAMARD_SUM mm0, 0, 0
    SATD_END

ALIGN 16
;-----------------------------------------------------------------------------
;   int x264_pixel_satd_4x8_mmxext (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
x264_pixel_satd_4x8_mmxext:
    SATD_START
    LOAD_DIFF_HADAMARD_SUM mm0, 0, 1
    LOAD_DIFF_HADAMARD_SUM mm1, 0, 0
    paddw       mm0, mm1
    SATD_END

ALIGN 16
;-----------------------------------------------------------------------------
;   int x264_pixel_satd_8x4_mmxext (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
x264_pixel_satd_8x4_mmxext:
    SATD_START
    LOAD_DIFF_HADAMARD_SUM mm0, 0, 0
    LOAD_DIFF_HADAMARD_SUM mm1, 4, 0
    paddw       mm0, mm1
    SATD_END

ALIGN 16
;-----------------------------------------------------------------------------
;   int x264_pixel_satd_8x8_mmxext (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
x264_pixel_satd_8x8_mmxext:
    SATD_START
    LOAD_DIFF_HADAMARD_SUM mm0, 0, 0
    LOAD_DIFF_HADAMARD_SUM mm1, 4, 1
    LOAD_DIFF_HADAMARD_SUM mm2, 0, 0
    paddw       mm0, mm1
    LOAD_DIFF_HADAMARD_SUM mm1, 4, 0
    paddw       mm0, mm2
    paddw       mm0, mm1
    SATD_END

ALIGN 16
;-----------------------------------------------------------------------------
;   int x264_pixel_satd_16x8_mmxext (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
x264_pixel_satd_16x8_mmxext:
    SATD_START
    LOAD_DIFF_HADAMARD_SUM mm0,  0, 0
    LOAD_DIFF_HADAMARD_SUM mm1,  4, 0
    LOAD_DIFF_HADAMARD_SUM mm2,  8, 0
    paddw       mm0, mm1
    LOAD_DIFF_HADAMARD_SUM mm1, 12, 1
    paddw       mm0, mm2

    LOAD_DIFF_HADAMARD_SUM mm2,  0, 0
    paddw       mm0, mm1
    LOAD_DIFF_HADAMARD_SUM mm1,  4, 0
    paddw       mm0, mm2
    LOAD_DIFF_HADAMARD_SUM mm2,  8, 0
    paddw       mm0, mm1
    LOAD_DIFF_HADAMARD_SUM mm1, 12, 1
    paddw       mm0, mm2
    paddw       mm0, mm1
    SATD_END

ALIGN 16
;-----------------------------------------------------------------------------
;   int x264_pixel_satd_8x16_mmxext (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
x264_pixel_satd_8x16_mmxext:
    SATD_START
    LOAD_DIFF_HADAMARD_SUM mm0,  0, 0
    LOAD_DIFF_HADAMARD_SUM mm1,  4, 1
    LOAD_DIFF_HADAMARD_SUM mm2,  0, 0
    paddw       mm0, mm1
    LOAD_DIFF_HADAMARD_SUM mm1,  4, 1
    paddw       mm0, mm2

    LOAD_DIFF_HADAMARD_SUM mm2,  0, 0
    paddw       mm0, mm1
    LOAD_DIFF_HADAMARD_SUM mm1,  4, 1
    paddw       mm1, mm2
    LOAD_DIFF_HADAMARD_SUM mm2,  0, 0
    paddw       mm0, mm1
    LOAD_DIFF_HADAMARD_SUM mm1,  4, 1
    paddw       mm0, mm2
    paddw       mm0, mm1
    SATD_END

ALIGN 16
;-----------------------------------------------------------------------------
;   int x264_pixel_satd_16x16_mmxext (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
x264_pixel_satd_16x16_mmxext:
    SATD_START
    LOAD_DIFF_HADAMARD_SUM mm0,  0, 0
    LOAD_DIFF_HADAMARD_SUM mm1,  4, 0
    LOAD_DIFF_HADAMARD_SUM mm2,  8, 0
    paddw       mm0, mm1
    LOAD_DIFF_HADAMARD_SUM mm1, 12, 1
    paddw       mm0, mm2

    LOAD_DIFF_HADAMARD_SUM mm2,  0, 0
    paddw       mm0, mm1
    LOAD_DIFF_HADAMARD_SUM mm1,  4, 0
    paddw       mm0, mm2
    LOAD_DIFF_HADAMARD_SUM mm2,  8, 0
    paddw       mm0, mm1
    LOAD_DIFF_HADAMARD_SUM mm1, 12, 1
    paddw       mm0, mm2

    LOAD_DIFF_HADAMARD_SUM mm2,  0, 0
    paddw       mm0, mm1
    LOAD_DIFF_HADAMARD_SUM mm1,  4, 0
    paddw       mm0, mm2
    LOAD_DIFF_HADAMARD_SUM mm2,  8, 0
    paddw       mm0, mm1
    LOAD_DIFF_HADAMARD_SUM mm1, 12, 1
    paddw       mm0, mm2

    LOAD_DIFF_HADAMARD_SUM mm2,  0, 0
    paddw       mm0, mm1
    LOAD_DIFF_HADAMARD_SUM mm1,  4, 0
    paddw       mm0, mm2
    LOAD_DIFF_HADAMARD_SUM mm2,  8, 0
    paddw       mm0, mm1
    LOAD_DIFF_HADAMARD_SUM mm1, 12, 0
    paddw       mm0, mm2
    paddw       mm0, mm1

    pxor        mm3, mm3
    pshufw      mm1, mm0, 01001110b
    paddw       mm0, mm1
    punpcklwd   mm0, mm3
    pshufw      mm1, mm0, 01001110b
    paddd       mm0, mm1
    movd        eax, mm0
    ret


; in: parm1 = fenc
; out: mm0..mm3 = hadamard coefs
ALIGN 16
load_hadamard:
    pxor        mm7, mm7
    movd        mm0, [parm1q+0*FENC_STRIDE]
    movd        mm4, [parm1q+1*FENC_STRIDE]
    movd        mm3, [parm1q+2*FENC_STRIDE]
    movd        mm1, [parm1q+3*FENC_STRIDE]
    punpcklbw   mm0, mm7
    punpcklbw   mm4, mm7
    punpcklbw   mm3, mm7
    punpcklbw   mm1, mm7
    HADAMARD4x4 mm0, mm4, mm3, mm1
    TRANSPOSE4x4 mm0, mm4, mm3, mm1, mm2
    HADAMARD4x4 mm0, mm1, mm2, mm3
    ret

%macro SCALAR_SUMSUB 4
    add %1, %2
    add %3, %4
    add %2, %2
    add %4, %4
    sub %2, %1
    sub %4, %3
%endmacro

%macro SUM_MM_X3 8 ; 3x sum, 4x tmp, op
    pxor        %7, %7
    pshufw      %4, %1, 01001110b
    pshufw      %5, %2, 01001110b
    pshufw      %6, %3, 01001110b
    paddw       %1, %4
    paddw       %2, %5
    paddw       %3, %6
    punpcklwd   %1, %7
    punpcklwd   %2, %7
    punpcklwd   %3, %7
    pshufw      %4, %1, 01001110b
    pshufw      %5, %2, 01001110b
    pshufw      %6, %3, 01001110b
    %8          %1, %4
    %8          %2, %5
    %8          %3, %6
%endmacro

ALIGN 16
;-----------------------------------------------------------------------------
;  void x264_intra_satd_x3_4x4_mmxext( uint8_t *fenc, uint8_t *fdec, int *res )
;-----------------------------------------------------------------------------
x264_intra_satd_x3_4x4_mmxext:
%define  top_1d  rsp-8  ; +8
%define  left_1d rsp-16 ; +8
    call load_hadamard

    movzx       r8d,  byte [parm2q-1+0*FDEC_STRIDE]
    movzx       r9d,  byte [parm2q-1+1*FDEC_STRIDE]
    movzx       r10d, byte [parm2q-1+2*FDEC_STRIDE]
    movzx       r11d, byte [parm2q-1+3*FDEC_STRIDE]
    SCALAR_SUMSUB r8d, r9d, r10d, r11d
    SCALAR_SUMSUB r8d, r10d, r9d, r11d ; 1x4 hadamard
    mov         [left_1d+0], r8w
    mov         [left_1d+2], r9w
    mov         [left_1d+4], r10w
    mov         [left_1d+6], r11w
    mov         eax, r8d ; dc

    movzx       r8d,  byte [parm2q-FDEC_STRIDE+0]
    movzx       r9d,  byte [parm2q-FDEC_STRIDE+1]
    movzx       r10d, byte [parm2q-FDEC_STRIDE+2]
    movzx       r11d, byte [parm2q-FDEC_STRIDE+3]
    SCALAR_SUMSUB r8d, r9d, r10d, r11d
    SCALAR_SUMSUB r8d, r10d, r9d, r11d ; 4x1 hadamard
    lea         rax, [rax + r8 + 4] ; dc
    mov         [top_1d+0], r8w
    mov         [top_1d+2], r9w
    mov         [top_1d+4], r10w
    mov         [top_1d+6], r11w
    and         eax, -8
    shl         eax, 1

    movq        mm4, mm1
    movq        mm5, mm2
    MMX_ABS_TWO mm4, mm5, mm6, mm7
    movq        mm7, mm3
    paddw       mm4, mm5
    MMX_ABS     mm7, mm6
    paddw       mm7, mm4 ; 3x4 sum

    movq        mm4, [left_1d]
    movd        mm5, eax
    psllw       mm4, 2
    psubw       mm4, mm0
    psubw       mm5, mm0
    punpcklwd   mm0, mm1
    punpcklwd   mm2, mm3
    punpckldq   mm0, mm2 ; transpose
    movq        mm1, [top_1d]
    psllw       mm1, 2
    psubw       mm0, mm1
    MMX_ABS     mm4, mm3 ; 1x4 sum
    MMX_ABS     mm5, mm2 ; 1x4 sum
    MMX_ABS     mm0, mm1 ; 4x1 sum
    paddw       mm4, mm7
    paddw       mm5, mm7
    movq        mm1, mm5
    psrlq       mm1, 16  ; 4x3 sum
    paddw       mm0, mm1

    SUM_MM_X3   mm0, mm4, mm5, mm1, mm2, mm3, mm6, pavgw
    movd        [parm3q+0], mm0 ; i4x4_v satd
    movd        [parm3q+4], mm4 ; i4x4_h satd
    movd        [parm3q+8], mm5 ; i4x4_dc satd
    ret

ALIGN 16
;-----------------------------------------------------------------------------
;  void x264_intra_satd_x3_16x16_mmxext( uint8_t *fenc, uint8_t *fdec, int *res )
;-----------------------------------------------------------------------------
x264_intra_satd_x3_16x16_mmxext:
%define  sums    rsp-32 ; +24
%define  top_1d  rsp-64 ; +32
%define  left_1d rsp-96 ; +32

    mov   qword [sums+0], 0
    mov   qword [sums+8], 0
    mov   qword [sums+16], 0

    ; 1D hadamards
    xor         ecx, ecx
    mov         eax, 12
.loop_edge:
    ; left
    shl         eax,  5 ; log(FDEC_STRIDE)
    movzx       r8d,  byte [parm2q+rax-1+0*FDEC_STRIDE]
    movzx       r9d,  byte [parm2q+rax-1+1*FDEC_STRIDE]
    movzx       r10d, byte [parm2q+rax-1+2*FDEC_STRIDE]
    movzx       r11d, byte [parm2q+rax-1+3*FDEC_STRIDE]
    shr         eax,  5
    SCALAR_SUMSUB r8d, r9d, r10d, r11d
    SCALAR_SUMSUB r8d, r10d, r9d, r11d
    add         ecx, r8d
    mov         [left_1d+2*rax+0], r8w
    mov         [left_1d+2*rax+2], r9w
    mov         [left_1d+2*rax+4], r10w
    mov         [left_1d+2*rax+6], r11w

    ; top
    movzx       r8d,  byte [parm2q+rax-FDEC_STRIDE+0]
    movzx       r9d,  byte [parm2q+rax-FDEC_STRIDE+1]
    movzx       r10d, byte [parm2q+rax-FDEC_STRIDE+2]
    movzx       r11d, byte [parm2q+rax-FDEC_STRIDE+3]
    SCALAR_SUMSUB r8d, r9d, r10d, r11d
    SCALAR_SUMSUB r8d, r10d, r9d, r11d
    add         ecx, r8d
    mov         [top_1d+2*rax+0], r8w
    mov         [top_1d+2*rax+2], r9w
    mov         [top_1d+2*rax+4], r10w
    mov         [top_1d+2*rax+6], r11w
    sub         eax, 4
    jge .loop_edge

    ; dc
    shr         ecx, 1
    add         ecx, 8
    and         ecx, -16

    ; 2D hadamards
    xor         eax, eax
.loop_y:
    xor         esi, esi
.loop_x:
    call load_hadamard

    movq        mm4, mm1
    movq        mm5, mm2
    MMX_ABS_TWO mm4, mm5, mm6, mm7
    movq        mm7, mm3
    paddw       mm4, mm5
    MMX_ABS     mm7, mm6
    paddw       mm7, mm4 ; 3x4 sum

    movq        mm4, [left_1d+8*rax]
    movd        mm5, ecx
    psllw       mm4, 2
    psubw       mm4, mm0
    psubw       mm5, mm0
    punpcklwd   mm0, mm1
    punpcklwd   mm2, mm3
    punpckldq   mm0, mm2 ; transpose
    movq        mm1, [top_1d+8*rsi]
    psllw       mm1, 2
    psubw       mm0, mm1
    MMX_ABS     mm4, mm3 ; 1x4 sum
    MMX_ABS     mm5, mm2 ; 1x4 sum
    MMX_ABS     mm0, mm1 ; 4x1 sum
    pavgw       mm4, mm7
    pavgw       mm5, mm7
    paddw       mm0, [sums+0]  ; i4x4_v satd
    paddw       mm4, [sums+8]  ; i4x4_h satd
    paddw       mm5, [sums+16] ; i4x4_dc satd
    movq        [sums+0], mm0
    movq        [sums+8], mm4
    movq        [sums+16], mm5

    add         parm1q, 4
    inc         esi
    cmp         esi, 4
    jl  .loop_x
    add         parm1q, 4*FENC_STRIDE-16
    inc         eax
    cmp         eax, 4
    jl  .loop_y

; horizontal sum
    movq        mm2, [sums+16]
    movq        mm1, [sums+8]
    movq        mm0, [sums+0]
    movq        mm7, mm2
    SUM_MM_X3   mm0, mm1, mm2, mm3, mm4, mm5, mm6, paddd
    psrld       mm0, 1
    pslld       mm7, 16
    psrld       mm7, 16
    paddd       mm0, mm2
    psubd       mm0, mm7
    movd        [parm3q+8], mm2 ; i16x16_dc satd
    movd        [parm3q+4], mm1 ; i16x16_h satd
    movd        [parm3q+0], mm0 ; i16x16_v satd
    ret

ALIGN 16
;-----------------------------------------------------------------------------
;  void x264_intra_satd_x3_8x8c_mmxext( uint8_t *fenc, uint8_t *fdec, int *res )
;-----------------------------------------------------------------------------
x264_intra_satd_x3_8x8c_mmxext:
%define  sums    rsp-32 ; +24
%define  top_1d  rsp-48 ; +16
%define  left_1d rsp-64 ; +16

    mov   qword [sums+0], 0
    mov   qword [sums+8], 0
    mov   qword [sums+16], 0

    ; 1D hadamards
    mov         eax, 4
.loop_edge:
    ; left
    shl         eax,  5 ; log(FDEC_STRIDE)
    movzx       r8d,  byte [parm2q+rax-1+0*FDEC_STRIDE]
    movzx       r9d,  byte [parm2q+rax-1+1*FDEC_STRIDE]
    movzx       r10d, byte [parm2q+rax-1+2*FDEC_STRIDE]
    movzx       r11d, byte [parm2q+rax-1+3*FDEC_STRIDE]
    shr         eax,  5
    SCALAR_SUMSUB r8d, r9d, r10d, r11d
    SCALAR_SUMSUB r8d, r10d, r9d, r11d
    mov         [left_1d+2*rax+0], r8w
    mov         [left_1d+2*rax+2], r9w
    mov         [left_1d+2*rax+4], r10w
    mov         [left_1d+2*rax+6], r11w

    ; top
    movzx       r8d,  byte [parm2q+rax-FDEC_STRIDE+0]
    movzx       r9d,  byte [parm2q+rax-FDEC_STRIDE+1]
    movzx       r10d, byte [parm2q+rax-FDEC_STRIDE+2]
    movzx       r11d, byte [parm2q+rax-FDEC_STRIDE+3]
    SCALAR_SUMSUB r8d, r9d, r10d, r11d
    SCALAR_SUMSUB r8d, r10d, r9d, r11d
    mov         [top_1d+2*rax+0], r8w
    mov         [top_1d+2*rax+2], r9w
    mov         [top_1d+2*rax+4], r10w
    mov         [top_1d+2*rax+6], r11w
    sub         eax, 4
    jge .loop_edge

    ; dc
    movzx       r8d,  word [left_1d+0]
    movzx       r9d,  word [top_1d+0]
    movzx       r10d, word [left_1d+8]
    movzx       r11d, word [top_1d+8]
    add         r8d,  r9d
    lea         r9,  [r10 + r11]
    lea         r8,  [2*r8 + 8]
    lea         r9,  [2*r9 + 8]
    lea         r10, [4*r10 + 8]
    lea         r11, [4*r11 + 8]
    and         r8d,  -16 ; tl
    and         r9d,  -16 ; br
    and         r10d, -16 ; bl
    and         r11d, -16 ; tr
    shl         r9,   16
    mov         r9w,  r10w
    shl         r9,   16
    mov         r9w,  r11w
    shl         r9,   16
    mov         r9w,  r8w

    ; 2D hadamards
    xor         eax, eax
.loop_y:
    xor         esi, esi
.loop_x:
    call load_hadamard

    movq        mm4, mm1
    movq        mm5, mm2
    MMX_ABS_TWO mm4, mm5, mm6, mm7
    movq        mm7, mm3
    paddw       mm4, mm5
    MMX_ABS     mm7, mm6
    paddw       mm7, mm4 ; 3x4 sum

    movq        mm4, [left_1d+8*rax]
    movzx       ecx, r9w
    shr         r9,  16
    movd        mm5, ecx
    psllw       mm4, 2
    psubw       mm4, mm0
    psubw       mm5, mm0
    punpcklwd   mm0, mm1
    punpcklwd   mm2, mm3
    punpckldq   mm0, mm2 ; transpose
    movq        mm1, [top_1d+8*rsi]
    psllw       mm1, 2
    psubw       mm0, mm1
    MMX_ABS     mm4, mm3 ; 1x4 sum
    MMX_ABS     mm5, mm2 ; 1x4 sum
    MMX_ABS     mm0, mm1 ; 4x1 sum
    pavgw       mm4, mm7
    pavgw       mm5, mm7
    paddw       mm0, [sums+16] ; i4x4_v satd
    paddw       mm4, [sums+8]  ; i4x4_h satd
    paddw       mm5, [sums+0]  ; i4x4_dc satd
    movq        [sums+16], mm0
    movq        [sums+8], mm4
    movq        [sums+0], mm5

    add         parm1q, 4
    inc         esi
    cmp         esi, 2
    jl  .loop_x
    add         parm1q, 4*FENC_STRIDE-8
    inc         eax
    cmp         eax, 2
    jl  .loop_y

; horizontal sum
    movq        mm0, [sums+0]
    movq        mm1, [sums+8]
    movq        mm2, [sums+16]
    movq        mm7, mm0
    psrlq       mm7, 15
    paddw       mm2, mm7
    SUM_MM_X3   mm0, mm1, mm2, mm3, mm4, mm5, mm6, paddd
    psrld       mm2, 1
    movd        [parm3q+0], mm0 ; i8x8c_dc satd
    movd        [parm3q+4], mm1 ; i8x8c_h satd
    movd        [parm3q+8], mm2 ; i8x8c_v satd
    ret

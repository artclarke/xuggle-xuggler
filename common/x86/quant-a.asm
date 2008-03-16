;*****************************************************************************
;* quant-a.asm: h264 encoder library
;*****************************************************************************
;* Copyright (C) 2005-2008 x264 project
;*
;* Authors: Loren Merritt <lorenm@u.washington.edu>
;*          Christian Heine <sennindemokrit@gmx.net>
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

%include "x86inc.asm"

SECTION_RODATA
pd_1:  times 2 dd 1

SECTION .text

%macro MMX_QUANT_DC_START 0
    movd       mm6, r1m     ; mf
    movd       mm7, r2m     ; bias
    pshufw     mm6, mm6, 0
    pshufw     mm7, mm7, 0
%endmacro

%macro SSE2_QUANT_DC_START 0
    movd       xmm6, r1m     ; mf
    movd       xmm7, r2m     ; bias
    pshuflw    xmm6, xmm6, 0
    pshuflw    xmm7, xmm7, 0
    punpcklqdq xmm6, xmm6
    punpcklqdq xmm7, xmm7
%endmacro

%macro QUANT_ONE 5
;;; %1      (m64)       dct[y][x]
;;; %2      (m64/mmx)   mf[y][x] or mf[0][0] (as uint16_t)
;;; %3      (m64/mmx)   bias[y][x] or bias[0][0] (as uint16_t)

    mov%1      %2m0, %3     ; load dct coeffs
    pxor       %2m1, %2m1
    pcmpgtw    %2m1, %2m0   ; sign(coeff)
    pxor       %2m0, %2m1
    psubw      %2m0, %2m1   ; abs(coeff)
    paddusw    %2m0, %5     ; round
    pmulhuw    %2m0, %4     ; divide
    pxor       %2m0, %2m1   ; restore sign
    psubw      %2m0, %2m1
    mov%1        %3, %2m0   ; store
%endmacro
%macro MMX_QUANT_1x4 3
    QUANT_ONE q, m, %1, %2, %3
%endmacro
%macro SSE2_QUANT_1x8 3
    QUANT_ONE dqa, xm, %1, %2, %3
%endmacro

%macro SSSE3_QUANT_1x8 3
    movdqa     xmm1, %1     ; load dct coeffs
    pabsw      xmm0, xmm1
    paddusw    xmm0, %3     ; round
    pmulhuw    xmm0, %2     ; divide
    psignw     xmm0, xmm1   ; restore sign
    movdqa       %1, xmm0   ; store
%endmacro

;-----------------------------------------------------------------------------
; void x264_quant_2x2_dc_mmxext( int16_t dct[4], int mf, int bias )
;-----------------------------------------------------------------------------
cglobal x264_quant_2x2_dc_mmxext, 1,1
    MMX_QUANT_DC_START
    MMX_QUANT_1x4 [r0], mm6, mm7
    RET

;-----------------------------------------------------------------------------
; void x264_quant_4x4_dc_mmxext( int16_t dct[16], int mf, int bias )
;-----------------------------------------------------------------------------
%macro QUANT_DC 6
cglobal %1, 1,1
    %2
%assign x 0
%rep %5
    %3 [r0+x], %4m6, %4m7
%assign x x+%6
%endrep
    RET
%endmacro

;-----------------------------------------------------------------------------
; void x264_quant_4x4_mmx( int16_t dct[16], uint16_t mf[16], uint16_t bias[16] )
;-----------------------------------------------------------------------------
%macro QUANT_AC 4
cglobal %1, 3,3
%assign x 0
%rep %3
    %2 [r0+x], [r1+x], [r2+x]
%assign x x+%4
%endrep
    RET
%endmacro

%ifndef ARCH_X86_64 ; not needed because sse2 is faster
QUANT_DC x264_quant_4x4_dc_mmxext, MMX_QUANT_DC_START, MMX_QUANT_1x4, m, 4, 8
QUANT_AC x264_quant_4x4_mmx, MMX_QUANT_1x4, 4, 8
QUANT_AC x264_quant_8x8_mmx, MMX_QUANT_1x4, 16, 8
%endif

QUANT_DC x264_quant_4x4_dc_sse2, SSE2_QUANT_DC_START, SSE2_QUANT_1x8, xm, 2, 16
QUANT_AC x264_quant_4x4_sse2, SSE2_QUANT_1x8, 2, 16
QUANT_AC x264_quant_8x8_sse2, SSE2_QUANT_1x8, 8, 16

%ifdef HAVE_SSE3
QUANT_DC x264_quant_4x4_dc_ssse3, SSE2_QUANT_DC_START, SSSE3_QUANT_1x8, xm, 2, 16
QUANT_AC x264_quant_4x4_ssse3, SSSE3_QUANT_1x8, 2, 16
QUANT_AC x264_quant_8x8_ssse3, SSSE3_QUANT_1x8, 8, 16
%endif



;=============================================================================
; dequant
;=============================================================================

%macro DEQUANT16_L_1x4 3
;;; %1      dct[y][x]
;;; %2,%3   dequant_mf[i_mf][y][x]
;;; mm5     i_qbits

    movq     mm1, %2
    movq     mm2, %3
    movq     mm0, %1
    packssdw mm1, mm2
    pmullw   mm0, mm1
    psllw    mm0, mm5
    movq     %1,  mm0
%endmacro

%macro DEQUANT32_R_1x4 3
;;; %1      dct[y][x]
;;; %2,%3   dequant_mf[i_mf][y][x]
;;; mm5     -i_qbits
;;; mm6     f as dwords
;;; mm7     0

    movq      mm0, %1
    movq      mm1, mm0
    punpcklwd mm0, mm0
    punpckhwd mm1, mm1

    movq      mm2, mm0
    movq      mm3, mm1
    pmulhw    mm0, %2
    pmulhw    mm1, %3
    pmullw    mm2, %2
    pmullw    mm3, %3
    pslld     mm0, 16
    pslld     mm1, 16
    paddd     mm0, mm2
    paddd     mm1, mm3

    paddd     mm0, mm6
    paddd     mm1, mm6
    psrad     mm0, mm5
    psrad     mm1, mm5

    packssdw  mm0, mm1
    movq      %1,  mm0
%endmacro

%macro DEQUANT_LOOP 2
    mov  t0d, 8*(%2-2)
%%loop:
    %1 [r0+t0+8], [r1+t0*2+16], [r1+t0*2+24]
    %1 [r0+t0  ], [r1+t0*2   ], [r1+t0*2+ 8]
    sub  t0d, 16
    jge  %%loop
    rep ret
%endmacro

;-----------------------------------------------------------------------------
; void x264_dequant_4x4_mmx( int16_t dct[4][4], int dequant_mf[6][4][4], int i_qp )
;-----------------------------------------------------------------------------
%macro DEQUANT_WxH 3
cglobal %1, 0,3
%ifdef ARCH_X86_64
    %define t0  r4
    %define t0d r4d
    imul r4d, r2d, 0x2b
    shr  r4d, 8     ; i_qbits = i_qp / 6
    lea  r3d, [r4d*3]
    sub  r2, r3
    sub  r2, r3     ; i_mf = i_qp % 6
    shl  r2, %3+2
    add  r1, r2     ; dequant_mf[i_mf]
%else
    %define t0  r2
    %define t0d r2d
    mov  r1, r2m    ; i_qp
    imul r2, r1, 0x2b
    shr  r2, 8      ; i_qbits = i_qp / 6
    lea  r0, [r2*3]
    sub  r1, r0
    sub  r1, r0     ; i_mf = i_qp % 6
    shl  r1, %3+2
    add  r1, r1m    ; dequant_mf[i_mf]
    mov  r0, r0m    ; dct
%endif

    sub  t0d, %3
    jl   .rshift32  ; negative qbits => rightshift

.lshift:
    movd mm5, t0d
    DEQUANT_LOOP DEQUANT16_L_1x4, %2

.rshift32:
    neg   t0d
    movd  mm5, t0d
    picgetgot t0d
    movq  mm6, [pd_1 GLOBAL]
    pxor  mm7, mm7
    pslld mm6, mm5
    psrld mm6, 1
    DEQUANT_LOOP DEQUANT32_R_1x4, %2
%endmacro

DEQUANT_WxH x264_dequant_4x4_mmx, 4, 4
DEQUANT_WxH x264_dequant_8x8_mmx, 16, 6

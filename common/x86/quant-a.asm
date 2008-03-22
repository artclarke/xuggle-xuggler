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
pw_1:     times 8 dw 1
pd_1:     times 4 dd 1

%macro DQM4 3
    dw %1, %2, %1, %2, %2, %3, %2, %3
%endmacro
%macro DQM8 6
    dw %1, %4, %5, %4, %1, %4, %5, %4
    dw %4, %2, %6, %2, %4, %2, %6, %2
    dw %5, %6, %3, %6, %5, %6, %3, %6
    ; last line not used, just padding for power-of-2 stride
    times 8 dw 0
%endmacro

dequant4_scale:
    DQM4 10, 13, 16
    DQM4 11, 14, 18
    DQM4 13, 16, 20
    DQM4 14, 18, 23
    DQM4 16, 20, 25
    DQM4 18, 23, 29

dequant8_scale:
    DQM8 20, 18, 32, 19, 25, 24
    DQM8 22, 19, 35, 21, 28, 26
    DQM8 26, 23, 42, 24, 33, 31
    DQM8 28, 25, 45, 26, 35, 33
    DQM8 32, 28, 51, 30, 40, 38
    DQM8 36, 32, 58, 34, 46, 43

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

%macro DEQUANT16_L 3
;;; %1      dct[y][x]
;;; %2,%3   dequant_mf[i_mf][y][x]
;;; m5      i_qbits

    movq     m0, %2
    packssdw m0, %3
    pmullw   m0, %1
    psllw    m0, m5
    movq     %1, m0
%endmacro

%macro DEQUANT32_R 3
;;; %1      dct[y][x]
;;; %2,%3   dequant_mf[i_mf][y][x]
;;; m5      -i_qbits
;;; m6      f
;;; m7      0

    movq      m0, %1
    movq      m1, m0
    punpcklwd m0, m7
    punpckhwd m1, m7
    pmaddwd   m0, %2
    pmaddwd   m1, %3
    paddd     m0, m6
    paddd     m1, m6
    psrad     m0, m5
    psrad     m1, m5
    packssdw  m0, m1
    movq      %1, m0
%endmacro

%macro DEQUANT_LOOP 3
%if 8*(%2-2*%3)
    mov t0d, 8*(%2-2*%3)
%%loop:
    %1 [r0+t0+8*%3], [r1+t0*2+16*%3], [r1+t0*2+24*%3]
    %1 [r0+t0     ], [r1+t0*2      ], [r1+t0*2+ 8*%3]
    sub t0d, 16*%3
    jge %%loop
    rep ret
%else
    %1 [r0+8*%3], [r1+16*%3], [r1+24*%3]
    %1 [r0     ], [r1      ], [r1+ 8*%3]
    ret
%endif
%endmacro

%macro DEQUANT16_FLAT 2-8
    movq   m0, %1
%assign i %0-2
%rep %0-1
%if i
    movq   m %+ i, [r0+%2]
    pmullw m %+ i, m0
%else
    pmullw m0, [r0+%2]
%endif
    psllw  m %+ i, m7
    movq   [r0+%2], m %+ i
    %assign i i-1
    %rotate 1
%endrep
%endmacro

%ifdef ARCH_X86_64
    %define t0  r4
    %define t0d r4d
    %define t1  r3
    %define t1d r3d
    %define t2  r2
    %define t2d r2d
%else
    %define t0  r2
    %define t0d r2d
    %define t1  r0
    %define t1d r0d
    %define t2  r1
    %define t2d r1d
%endif

;-----------------------------------------------------------------------------
; void x264_dequant_4x4_mmx( int16_t dct[4][4], int dequant_mf[6][4][4], int i_qp )
;-----------------------------------------------------------------------------
%macro DEQUANT 4
cglobal x264_dequant_%2x%2_%1, 0,3
    movifnidn t2d, r2m
    imul t0d, t2d, 0x2b
    shr  t0d, 8     ; i_qbits = i_qp / 6
    lea  t1, [t0*3]
    sub  t2d, t1d
    sub  t2d, t1d   ; i_mf = i_qp % 6
    shl  t2d, %3+2
%ifdef ARCH_X86_64
    add  r1, t2     ; dequant_mf[i_mf]
%else
    add  r1, r1m    ; dequant_mf[i_mf]
    mov  r0, r0m    ; dct
%endif
    sub  t0d, %3
    jl   .rshift32  ; negative qbits => rightshift

.lshift:
    movd m5, t0d
    DEQUANT_LOOP DEQUANT16_L, %2*%2/4, %4

.rshift32:
    neg   t0d
    movd  m5, t0d
    picgetgot t0d
    movq  m6, [pd_1 GLOBAL]
    pxor  m7, m7
    pslld m6, m5
    psrld m6, 1
    DEQUANT_LOOP DEQUANT32_R, %2*%2/4, %4

cglobal x264_dequant_%2x%2_flat16_%1, 0,3
    movifnidn t2d, r2m
%if %2 == 8
    cmp  t2d, 12
    jl x264_dequant_%2x%2_%1
    sub  t2d, 12
%endif
    imul t0d, t2d, 0x2b
    shr  t0d, 8     ; i_qbits = i_qp / 6
    lea  t1, [t0*3]
    sub  t2d, t1d
    sub  t2d, t1d   ; i_mf = i_qp % 6
    shl  t2d, %3
%ifdef PIC64
    lea  r1, [dequant%2_scale GLOBAL]
    add  r1, t2
%else
    picgetgot r0
    lea  r1, [t2 + dequant%2_scale GLOBAL]
%endif
    movifnidn r0d, r0m
    movd m7, t0d
%if %2 == 4
%ifidn %1, mmx
    DEQUANT16_FLAT [r1], 0, 16
    DEQUANT16_FLAT [r1+8], 8, 24
%else
    DEQUANT16_FLAT [r1], 0, 16
%endif
%elifidn %1, mmx
    DEQUANT16_FLAT [r1], 0, 8, 64, 72
    DEQUANT16_FLAT [r1+16], 16, 24, 48, 56
    DEQUANT16_FLAT [r1+16], 80, 88, 112, 120
    DEQUANT16_FLAT [r1+32], 32, 40, 96, 104
%else
    DEQUANT16_FLAT [r1], 0, 64
    DEQUANT16_FLAT [r1+16], 16, 48, 80, 112
    DEQUANT16_FLAT [r1+32], 32, 96
%endif
    ret
%endmacro ; DEQUANT

%ifndef ARCH_X86_64
INIT_MMX
DEQUANT mmx, 4, 4, 1
DEQUANT mmx, 8, 6, 1
%endif
INIT_XMM
DEQUANT sse2, 4, 4, 2
DEQUANT sse2, 8, 6, 2


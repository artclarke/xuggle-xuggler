;*****************************************************************************
;* quant-a.asm: h264 encoder library
;*****************************************************************************
;* Copyright (C) 2005 x264 project
;*
;* Authors: Loren Merritt <lorenm@u.washington.edu>
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

%include "amd64inc.asm"

SECTION_RODATA
pd_1:  times 2 dd 1

SECTION .text

%macro MMX_QUANT_DC_START 0
    movd       mm6, parm2d     ; mf
    movd       mm7, parm3d     ; bias
    pshufw     mm6, mm6, 0
    pshufw     mm7, mm7, 0
%endmacro

%macro SSE2_QUANT_DC_START 0
    movd       xmm6, parm2d     ; mf
    movd       xmm7, parm3d     ; bias
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
cglobal x264_quant_2x2_dc_mmxext
    MMX_QUANT_DC_START
    MMX_QUANT_1x4 [parm1q], mm6, mm7
    ret

%macro QUANT_SSE 1
;-----------------------------------------------------------------------------
; void x264_quant_4x4_dc_sse2( int16_t dct[16], int mf, int bias )
;-----------------------------------------------------------------------------
cglobal x264_quant_4x4_dc_%1
    SSE2_QUANT_DC_START
%assign x 0
%rep 2
    QUANT_1x8 [parm1q+x], xmm6, xmm7
%assign x (x+16)
%endrep
    ret

;-----------------------------------------------------------------------------
; void x264_quant_4x4_sse2( int16_t dct[16], uint16_t mf[16], uint16_t bias[16] )
;-----------------------------------------------------------------------------
cglobal x264_quant_4x4_%1
%assign x 0
%rep 2
    QUANT_1x8 [parm1q+x], [parm2q+x], [parm3q+x]
%assign x (x+16)
%endrep
    ret

;-----------------------------------------------------------------------------
; void x264_quant_8x8_sse2( int16_t dct[64], uint16_t mf[64], uint16_t bias[64] )
;-----------------------------------------------------------------------------
cglobal x264_quant_8x8_%1
%assign x 0
%rep 8
    QUANT_1x8 [parm1q+x], [parm2q+x], [parm3q+x]
%assign x (x+16)
%endrep
    ret
%endmacro

%define QUANT_1x8 SSE2_QUANT_1x8
QUANT_SSE sse2
%ifdef HAVE_SSE3
%define QUANT_1x8 SSSE3_QUANT_1x8
QUANT_SSE ssse3
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

;-----------------------------------------------------------------------------
; void x264_dequant_4x4_mmx( int16_t dct[4][4], int dequant_mf[6][4][4], int i_qp )
;-----------------------------------------------------------------------------
%macro DEQUANT_WxH 3
cglobal %1
;   mov  rdi, rdi   ; dct
;   mov  rsi, rsi   ; dequant_mf
;   mov  edx, edx   ; i_qp

    imul eax, edx, 0x2b
    shr  eax, 8     ; i_qbits = i_qp / 6
    lea  ecx, [eax+eax*2]
    sub  edx, ecx
    sub  edx, ecx   ; i_mf = i_qp % 6
    shl  edx, %3+2
    movsxd rdx, edx
    add  rsi, rdx   ; dequant_mf[i_mf]

    sub  eax, %3
    jl   .rshift32  ; negative qbits => rightshift

.lshift:
    movd mm5, eax

%rep %2
    DEQUANT16_L_1x4 [rdi], [rsi], [rsi+8]
    add  rsi, byte 16
    add  rdi, byte 8
%endrep

    ret

.rshift32:
    neg   eax
    movd  mm5, eax
    movq  mm6, [pd_1 GLOBAL]
    pxor  mm7, mm7
    pslld mm6, mm5
    psrld mm6, 1

%rep %2
    DEQUANT32_R_1x4 [rdi], [rsi], [rsi+8]
    add  rsi, byte 16
    add  rdi, byte 8
%endrep

    ret
%endmacro

DEQUANT_WxH x264_dequant_4x4_mmx, 4, 4
DEQUANT_WxH x264_dequant_8x8_mmx, 16, 6

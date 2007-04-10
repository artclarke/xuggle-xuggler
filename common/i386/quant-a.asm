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

BITS 32

%include "i386inc.asm"

SECTION_RODATA
pd_1:  times 2 dd 1

SECTION .text

%macro QUANT_AC_START 0
    mov         eax, [esp+ 4]   ; dct
    mov         ecx, [esp+ 8]   ; mf
    mov         edx, [esp+12]   ; bias
%endmacro

%macro MMX_QUANT_DC_START 0
    mov         eax, [esp+ 4]   ; dct
    movd        mm6, [esp+ 8]   ; mf
    movd        mm7, [esp+12]   ; bias
    pshufw      mm6, mm6, 0
    pshufw      mm7, mm7, 0
%endmacro

%macro SSE2_QUANT_DC_START 0
    mov         eax, [esp+ 4]   ; dct
    movd       xmm6, [esp+ 8]   ; mf
    movd       xmm7, [esp+12]   ; bias
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
    MMX_QUANT_1x4 [eax], mm6, mm7
    ret

;-----------------------------------------------------------------------------
; void x264_quant_4x4_dc_mmxext( int16_t dct[16], int mf, int bias )
;-----------------------------------------------------------------------------
cglobal x264_quant_4x4_dc_mmxext
    MMX_QUANT_DC_START
%assign x 0
%rep 4
    MMX_QUANT_1x4 [eax+x], mm6, mm7
%assign x (x+8)
%endrep
    ret

;-----------------------------------------------------------------------------
; void x264_quant_4x4_mmx( int16_t dct[16], uint16_t mf[16], uint16_t bias[16] )
;-----------------------------------------------------------------------------
cglobal x264_quant_4x4_mmx
    QUANT_AC_START
%assign x 0
%rep 4
    MMX_QUANT_1x4 [eax+x], [ecx+x], [edx+x]
%assign x (x+8)
%endrep
    ret

;-----------------------------------------------------------------------------
; void x264_quant_8x8_mmx( int16_t dct[64], uint16_t mf[64], uint16_t bias[64] )
;-----------------------------------------------------------------------------
cglobal x264_quant_8x8_mmx
    QUANT_AC_START
%assign x 0
%rep 16
    MMX_QUANT_1x4 [eax+x], [ecx+x], [edx+x]
%assign x (x+8)
%endrep
    ret

%macro QUANT_SSE 1
;-----------------------------------------------------------------------------
; void x264_quant_4x4_dc_sse2( int16_t dct[16], int mf, int bias )
;-----------------------------------------------------------------------------
cglobal x264_quant_4x4_dc_%1
    SSE2_QUANT_DC_START
%assign x 0
%rep 2
    QUANT_1x8 [eax+x], xmm6, xmm7
%assign x (x+16)
%endrep
    ret

;-----------------------------------------------------------------------------
; void x264_quant_4x4_sse2( int16_t dct[16], uint16_t mf[16], uint16_t bias[16] )
;-----------------------------------------------------------------------------
cglobal x264_quant_4x4_%1
    QUANT_AC_START
%assign x 0
%rep 2
    QUANT_1x8 [eax+x], [ecx+x], [edx+x]
%assign x (x+16)
%endrep
    ret

;-----------------------------------------------------------------------------
; void x264_quant_8x8_sse2( int16_t dct[64], uint16_t mf[64], uint16_t bias[64] )
;-----------------------------------------------------------------------------
cglobal x264_quant_8x8_%1
    QUANT_AC_START
%assign x 0
%rep 8
    QUANT_1x8 [eax+x], [ecx+x], [edx+x]
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

%macro DEQUANT16_R_1x4 3
;;; %1      dct[y][x]
;;; %2,%3   dequant_mf[i_mf][y][x]
;;; mm5     -i_qbits
;;; mm6     f as words

    movq     mm1, %2
    movq     mm2, %3
    movq     mm0, %1
    packssdw mm1, mm2
    pmullw   mm0, mm1
    paddw    mm0, mm6
    psraw    mm0, mm5
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
    mov  edx, [esp+12] ; i_qp
    imul eax, edx, 0x2b
    shr  eax, 8       ; i_qbits = i_qp / 6
    lea  ecx, [eax+eax*2]
    sub  edx, ecx
    sub  edx, ecx     ; i_mf = i_qp % 6
    shl  edx, %3+2
    add  edx, [esp+8] ; dequant_mf[i_mf]
    mov  ecx, [esp+4] ; dct

    sub  eax, %3
    jl   .rshift32    ; negative qbits => rightshift

.lshift:
    movd mm5, eax

    mov  eax, 8*(%2-1)
.loopl16
%rep 2
    DEQUANT16_L_1x4 [ecx+eax], [edx+eax*2], [edx+eax*2+8]
    sub  eax, byte 8
%endrep
    jge  .loopl16

    nop
    ret

.rshift32:
    neg   eax
    picpush ebx
    picgetgot ebx
    movq  mm6, [pd_1 GOT_ebx]
    picpop ebx
    movd  mm5, eax
    pxor  mm7, mm7
    pslld mm6, mm5
    psrld mm6, 1

    mov  eax, 8*(%2-1)
.loopr32
%rep 2
    DEQUANT32_R_1x4 [ecx+eax], [edx+eax*2], [edx+eax*2+8]
    sub  eax, byte 8
%endrep
    jge  .loopr32

    nop
    ret
%endmacro

DEQUANT_WxH x264_dequant_4x4_mmx, 4, 4
DEQUANT_WxH x264_dequant_8x8_mmx, 16, 6

;*****************************************************************************
;* dct.asm: h264 encoder library
;*****************************************************************************
;* Copyright (C) 2003 x264 project
;* $Id: dct.asm,v 1.1 2004/06/03 19:27:07 fenrir Exp $
;*
;* Authors: Laurent Aimar <fenrir@via.ecp.fr> (initial version)
;*          Min Chen <chenm001.163.com> (converted to nasm)
;*          Loren Merritt <lorenm@u.washington.edu> (dct8)
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

;*****************************************************************************
;*                                                                           *
;*  Revision history:                                                        *
;*                                                                           *
;*  2004.04.28  portab all 4x4 function to nasm (CM)                         *
;*                                                                           *
;*****************************************************************************

BITS 64

;=============================================================================
; Macros and other preprocessor constants
;=============================================================================

%include "amd64inc.asm"

%macro MMX_ZERO 1
    pxor    %1, %1
%endmacro

%macro MMX_LOAD_DIFF_4P 5
    movd        %1, %4
    punpcklbw   %1, %3
    movd        %2, %5
    punpcklbw   %2, %3
    psubw       %1, %2
%endmacro

%macro MMX_LOAD_DIFF_8P 5
    movq        %1, %4
    punpcklbw   %1, %3
    movq        %2, %5
    punpcklbw   %2, %3
    psubw       %1, %2
%endmacro

%macro MMX_SUMSUB_BA 2
    paddw   %1, %2
    paddw   %2, %2
    psubw   %2, %1
%endmacro

%macro MMX_SUMSUB_BADC 4
    paddw   %1, %2
    paddw   %3, %4
    paddw   %2, %2
    paddw   %4, %4
    psubw   %2, %1
    psubw   %4, %3
%endmacro

%macro MMX_SUMSUB2_AB 3
    movq    %3, %1
    paddw   %1, %1
    paddw   %1, %2
    psubw   %3, %2
    psubw   %3, %2
%endmacro

%macro MMX_SUMSUBD2_AB 4
    movq    %4, %1
    movq    %3, %2
    psraw   %2, 1
    psraw   %4, 1
    paddw   %1, %2
    psubw   %4, %3
%endmacro

%macro SBUTTERFLY 5
    mov%1       %5, %3
    punpckl%2   %3, %4
    punpckh%2   %5, %4
%endmacro

;-----------------------------------------------------------------------------
; input ABCD output ADTC
;-----------------------------------------------------------------------------
%macro MMX_TRANSPOSE 5
    SBUTTERFLY q, wd, %1, %2, %5
    SBUTTERFLY q, wd, %3, %4, %2
    SBUTTERFLY q, dq, %1, %3, %4
    SBUTTERFLY q, dq, %5, %2, %3
%endmacro

;-----------------------------------------------------------------------------
; input ABCDEFGH output AFHDTECB 
;-----------------------------------------------------------------------------
%macro SSE2_TRANSPOSE8x8 9
    SBUTTERFLY dqa, wd, %1, %2, %9
    SBUTTERFLY dqa, wd, %3, %4, %2
    SBUTTERFLY dqa, wd, %5, %6, %4
    SBUTTERFLY dqa, wd, %7, %8, %6
    SBUTTERFLY dqa, dq, %1, %3, %8
    SBUTTERFLY dqa, dq, %9, %2, %3
    SBUTTERFLY dqa, dq, %5, %7, %2
    SBUTTERFLY dqa, dq, %4, %6, %7
    SBUTTERFLY dqa, qdq, %1, %5, %6
    SBUTTERFLY dqa, qdq, %9, %4, %5
    SBUTTERFLY dqa, qdq, %8, %2, %4
    SBUTTERFLY dqa, qdq, %3, %7, %2
%endmacro

%macro MMX_STORE_DIFF_4P 5
    paddw       %1, %3
    psraw       %1, 6
    movd        %2, %5
    punpcklbw   %2, %4
    paddsw      %1, %2
    packuswb    %1, %1
    movd        %5, %1
%endmacro

%macro MMX_STORE_DIFF_8P 4
    psraw       %1, 6
    movq        %2, %4
    punpcklbw   %2, %3
    paddsw      %1, %2
    packuswb    %1, %1  
    movq        %4, %1
%endmacro

;=============================================================================
; Constants
;=============================================================================

SECTION_RODATA
pw_1:  times 8 dw 1
pw_32: times 8 dw 32

;=============================================================================
; Code
;=============================================================================

SECTION .text

;-----------------------------------------------------------------------------
;   void x264_dct4x4dc_mmx( int16_t d[4][4] )
;-----------------------------------------------------------------------------
cglobal x264_dct4x4dc_mmx
    movq    mm0,        [parm1q+ 0]
    movq    mm1,        [parm1q+ 8]
    movq    mm2,        [parm1q+16]
    movq    mm3,        [parm1q+24]

    MMX_SUMSUB_BADC     mm1, mm0, mm3, mm2          ; mm1=s01  mm0=d01  mm3=s23  mm2=d23
    MMX_SUMSUB_BADC     mm3, mm1, mm2, mm0          ; mm3=s01+s23  mm1=s01-s23  mm2=d01+d23  mm0=d01-d23

    MMX_TRANSPOSE       mm3, mm1, mm0, mm2, mm4     ; in: mm3, mm1, mm0, mm2  out: mm3, mm2, mm4, mm0 

    MMX_SUMSUB_BADC     mm2, mm3, mm0, mm4          ; mm2=s01  mm3=d01  mm0=s23  mm4=d23
    MMX_SUMSUB_BADC     mm0, mm2, mm4, mm3          ; mm0=s01+s23  mm2=s01-s23  mm4=d01+d23  mm3=d01-d23

    movq    mm6,        [pw_1 GLOBAL]
    paddw   mm0,        mm6
    paddw   mm2,        mm6
    psraw   mm0,        1
    movq    [parm1q+ 0],mm0
    psraw   mm2,        1
    movq    [parm1q+ 8],mm2
    paddw   mm3,        mm6
    paddw   mm4,        mm6
    psraw   mm3,        1
    movq    [parm1q+16],mm3
    psraw   mm4,        1
    movq    [parm1q+24],mm4
    ret

;-----------------------------------------------------------------------------
;   void x264_idct4x4dc_mmx( int16_t d[4][4] )
;-----------------------------------------------------------------------------
cglobal x264_idct4x4dc_mmx
    movq    mm0, [parm1q+ 0]
    movq    mm1, [parm1q+ 8]
    movq    mm2, [parm1q+16]
    movq    mm3, [parm1q+24]

    MMX_SUMSUB_BADC     mm1, mm0, mm3, mm2          ; mm1=s01  mm0=d01  mm3=s23  mm2=d23
    MMX_SUMSUB_BADC     mm3, mm1, mm2, mm0          ; mm3=s01+s23 mm1=s01-s23 mm2=d01+d23 mm0=d01-d23

    MMX_TRANSPOSE       mm3, mm1, mm0, mm2, mm4     ; in: mm3, mm1, mm0, mm2  out: mm3, mm2, mm4, mm0 

    MMX_SUMSUB_BADC     mm2, mm3, mm0, mm4          ; mm2=s01  mm3=d01  mm0=s23  mm4=d23
    MMX_SUMSUB_BADC     mm0, mm2, mm4, mm3          ; mm0=s01+s23  mm2=s01-s23  mm4=d01+d23  mm3=d01-d23

    movq    [parm1q+ 0], mm0
    movq    [parm1q+ 8], mm2
    movq    [parm1q+16], mm3
    movq    [parm1q+24], mm4
    ret

;-----------------------------------------------------------------------------
;   void x264_sub4x4_dct_mmx( int16_t dct[4][4], uint8_t *pix1, uint8_t *pix2 )
;-----------------------------------------------------------------------------
cglobal x264_sub4x4_dct_mmx
    MMX_ZERO    mm7

    ; Load 4 lines
    MMX_LOAD_DIFF_4P    mm0, mm6, mm7, [parm2q+0*FENC_STRIDE], [parm3q+0*FDEC_STRIDE]
    MMX_LOAD_DIFF_4P    mm1, mm6, mm7, [parm2q+1*FENC_STRIDE], [parm3q+1*FDEC_STRIDE]
    MMX_LOAD_DIFF_4P    mm2, mm6, mm7, [parm2q+2*FENC_STRIDE], [parm3q+2*FDEC_STRIDE]
    MMX_LOAD_DIFF_4P    mm3, mm6, mm7, [parm2q+3*FENC_STRIDE], [parm3q+3*FDEC_STRIDE]

    MMX_SUMSUB_BADC     mm3, mm0, mm2, mm1          ; mm3=s03  mm0=d03  mm2=s12  mm1=d12

    MMX_SUMSUB_BA       mm2, mm3                    ; mm2=s03+s12      mm3=s03-s12
    MMX_SUMSUB2_AB      mm0, mm1, mm4               ; mm0=2.d03+d12    mm4=d03-2.d12

    ; transpose in: mm2, mm0, mm3, mm4, out: mm2, mm4, mm1, mm3
    MMX_TRANSPOSE       mm2, mm0, mm3, mm4, mm1

    MMX_SUMSUB_BADC     mm3, mm2, mm1, mm4          ; mm3=s03  mm2=d03  mm1=s12  mm4=d12

    MMX_SUMSUB_BA       mm1, mm3                    ; mm1=s03+s12      mm3=s03-s12
    MMX_SUMSUB2_AB      mm2, mm4, mm0               ; mm2=2.d03+d12    mm0=d03-2.d12

    movq    [parm1q+ 0], mm1
    movq    [parm1q+ 8], mm2
    movq    [parm1q+16], mm3
    movq    [parm1q+24], mm0
    ret

;-----------------------------------------------------------------------------
;   void x264_add4x4_idct_mmx( uint8_t *p_dst, int16_t dct[4][4] )
;-----------------------------------------------------------------------------
cglobal x264_add4x4_idct_mmx
    ; Load dct coeffs
    movq    mm0, [parm2q+ 0] ; dct
    movq    mm1, [parm2q+ 8]
    movq    mm2, [parm2q+16]
    movq    mm3, [parm2q+24]
    
    MMX_SUMSUB_BA       mm2, mm0                        ; mm2=s02  mm0=d02
    MMX_SUMSUBD2_AB     mm1, mm3, mm5, mm4              ; mm1=s13  mm4=d13 ( well 1 + 3>>1 and 1>>1 + 3)

    MMX_SUMSUB_BADC     mm1, mm2, mm4, mm0              ; mm1=s02+s13  mm2=s02-s13  mm4=d02+d13  mm0=d02-d13

    ; in: mm1, mm4, mm0, mm2  out: mm1, mm2, mm3, mm0
    MMX_TRANSPOSE       mm1, mm4, mm0, mm2, mm3

    MMX_SUMSUB_BA       mm3, mm1                        ; mm3=s02  mm1=d02
    MMX_SUMSUBD2_AB     mm2, mm0, mm5, mm4              ; mm2=s13  mm4=d13 ( well 1 + 3>>1 and 1>>1 + 3)

    MMX_SUMSUB_BADC     mm2, mm3, mm4, mm1              ; mm2=s02+s13  mm3=s02-s13  mm4=d02+d13  mm1=d02-d13

    MMX_ZERO            mm7
    movq                mm6, [pw_32 GLOBAL]
    
    MMX_STORE_DIFF_4P   mm2, mm0, mm6, mm7, [parm1q+0*FDEC_STRIDE]
    MMX_STORE_DIFF_4P   mm4, mm0, mm6, mm7, [parm1q+1*FDEC_STRIDE]
    MMX_STORE_DIFF_4P   mm1, mm0, mm6, mm7, [parm1q+2*FDEC_STRIDE]
    MMX_STORE_DIFF_4P   mm3, mm0, mm6, mm7, [parm1q+3*FDEC_STRIDE]

    ret



; =============================================================================
; 8x8 Transform
; =============================================================================

; in:  ABCDEFGH
; out: FBCGEDHI
%macro DCT8_1D 10
    MMX_SUMSUB_BA  %8, %1 ; %8=s07, %1=d07
    MMX_SUMSUB_BA  %7, %2 ; %7=s16, %2=d16
    MMX_SUMSUB_BA  %6, %3 ; %6=s25, %3=d25
    MMX_SUMSUB_BA  %5, %4 ; %5=s34, %4=d34

    MMX_SUMSUB_BA  %5, %8 ; %5=a0, %8=a2
    MMX_SUMSUB_BA  %6, %7 ; %6=a1, %7=a3

    movdqa  %9, %1
    psraw   %9, 1
    paddw   %9, %1
    paddw   %9, %2
    paddw   %9, %3 ; %9=a4

    movdqa  %10, %4
    psraw   %10, 1
    paddw   %10, %4
    paddw   %10, %2
    psubw   %10, %3 ; %10=a7

    MMX_SUMSUB_BA  %4, %1
    psubw   %1, %3
    psubw   %4, %2
    psraw   %3, 1
    psraw   %2, 1
    psubw   %1, %3 ; %1=a5
    psubw   %4, %2 ; %4=a6

    MMX_SUMSUB_BA  %6, %5 ; %6=b0, %5=b4

    movdqa  %2, %10
    psraw   %2, 2
    paddw   %2, %9 ; %2=b1
    psraw   %9, 2
    psubw   %9, %10 ; %9=b7

    movdqa  %3, %7
    psraw   %3, 1
    paddw   %3, %8 ; %3=b2
    psraw   %8, 1
    psubw   %8, %7 ; %8=b6

    movdqa  %7, %4
    psraw   %7, 2
    paddw   %7, %1 ; %7=b3
    psraw   %1, 2
    psubw   %4, %1 ; %4=b5
%endmacro

;-----------------------------------------------------------------------------
;   void __cdecl x264_sub8x8_dct8_sse2( int16_t dct[8][8], uint8_t *pix1, uint8_t *pix2 )
;-----------------------------------------------------------------------------
cglobal x264_sub8x8_dct8_sse2
    MMX_ZERO  xmm9

    MMX_LOAD_DIFF_8P  xmm0, xmm8, xmm9, [parm2q+0*FENC_STRIDE], [parm3q+0*FDEC_STRIDE]
    MMX_LOAD_DIFF_8P  xmm1, xmm8, xmm9, [parm2q+1*FENC_STRIDE], [parm3q+1*FDEC_STRIDE]
    MMX_LOAD_DIFF_8P  xmm2, xmm8, xmm9, [parm2q+2*FENC_STRIDE], [parm3q+2*FDEC_STRIDE]
    MMX_LOAD_DIFF_8P  xmm3, xmm8, xmm9, [parm2q+3*FENC_STRIDE], [parm3q+3*FDEC_STRIDE]
    MMX_LOAD_DIFF_8P  xmm4, xmm8, xmm9, [parm2q+4*FENC_STRIDE], [parm3q+4*FDEC_STRIDE]
    MMX_LOAD_DIFF_8P  xmm5, xmm8, xmm9, [parm2q+5*FENC_STRIDE], [parm3q+5*FDEC_STRIDE]
    MMX_LOAD_DIFF_8P  xmm6, xmm8, xmm9, [parm2q+6*FENC_STRIDE], [parm3q+6*FDEC_STRIDE]
    MMX_LOAD_DIFF_8P  xmm7, xmm8, xmm9, [parm2q+7*FENC_STRIDE], [parm3q+7*FDEC_STRIDE]

    DCT8_1D           xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm8, xmm9
    SSE2_TRANSPOSE8x8 xmm5, xmm1, xmm2, xmm6, xmm4, xmm3, xmm7, xmm8, xmm0
    DCT8_1D           xmm5, xmm3, xmm8, xmm6, xmm0, xmm4, xmm2, xmm1, xmm7, xmm9

    movdqa  [parm1q+0x00], xmm4
    movdqa  [parm1q+0x10], xmm3
    movdqa  [parm1q+0x20], xmm8
    movdqa  [parm1q+0x30], xmm2
    movdqa  [parm1q+0x40], xmm0
    movdqa  [parm1q+0x50], xmm6
    movdqa  [parm1q+0x60], xmm1
    movdqa  [parm1q+0x70], xmm7

    ret


; in:  ABCDEFGH
; out: IBHDEACG
%macro IDCT8_1D 10
    MMX_SUMSUB_BA  %5, %1 ; %5=a0, %1=a2
    movdqa  %10, %3
    psraw   %3, 1
    psubw   %3, %7 ; %3=a4
    psraw   %7, 1
    paddw   %7, %10 ; %7=a6

    movdqa  %9, %2
    psraw   %9, 1
    paddw   %9, %2
    paddw   %9, %4
    paddw   %9, %6 ; %9=a7
    
    movdqa  %10, %6
    psraw   %10, 1
    paddw   %10, %6
    paddw   %10, %8
    psubw   %10, %2 ; %10=a5

    psubw   %2, %4
    psubw   %6, %4
    paddw   %2, %8
    psubw   %6, %8
    psraw   %4, 1
    psraw   %8, 1
    psubw   %2, %4 ; %2=a3
    psubw   %6, %8 ; %6=a1

    MMX_SUMSUB_BA %7, %5 ; %7=b0, %5=b6
    MMX_SUMSUB_BA %3, %1 ; %3=b2, %1=b4

    movdqa  %4, %9
    psraw   %4, 2
    paddw   %4, %6 ; %4=b1
    psraw   %6, 2
    psubw   %9, %6 ; %9=b7

    movdqa  %8, %10
    psraw   %8, 2
    paddw   %8, %2 ; %8=b3
    psraw   %2, 2
    psubw   %2, %10 ; %2=b5

    MMX_SUMSUB_BA %9, %7 ; %9=c0, %7=c7
    MMX_SUMSUB_BA %2, %3 ; %2=c1, %3=c6
    MMX_SUMSUB_BA %8, %1 ; %8=c2, %1=c5
    MMX_SUMSUB_BA %4, %5 ; %4=c3, %5=c4
%endmacro

;-----------------------------------------------------------------------------
;   void __cdecl x264_add8x8_idct8_sse2( uint8_t *p_dst, int16_t dct[8][8] )
;-----------------------------------------------------------------------------
cglobal x264_add8x8_idct8_sse2
    movdqa  xmm0, [parm2q+0x00]
    movdqa  xmm1, [parm2q+0x10]
    movdqa  xmm2, [parm2q+0x20]
    movdqa  xmm3, [parm2q+0x30]
    movdqa  xmm4, [parm2q+0x40]
    movdqa  xmm5, [parm2q+0x50]
    movdqa  xmm6, [parm2q+0x60]
    movdqa  xmm7, [parm2q+0x70]

    IDCT8_1D          xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm9, xmm8
    SSE2_TRANSPOSE8x8 xmm9, xmm1, xmm7, xmm3, xmm4, xmm0, xmm2, xmm6, xmm5
    paddw             xmm9, [pw_32 GLOBAL] ; rounding for the >>6 at the end
    IDCT8_1D          xmm9, xmm0, xmm6, xmm3, xmm5, xmm4, xmm7, xmm1, xmm8, xmm2
 
    MMX_ZERO  xmm15
    MMX_STORE_DIFF_8P   xmm8, xmm14, xmm15, [parm1q+0*FDEC_STRIDE]
    MMX_STORE_DIFF_8P   xmm0, xmm14, xmm15, [parm1q+1*FDEC_STRIDE]
    MMX_STORE_DIFF_8P   xmm1, xmm14, xmm15, [parm1q+2*FDEC_STRIDE]
    MMX_STORE_DIFF_8P   xmm3, xmm14, xmm15, [parm1q+3*FDEC_STRIDE]
    MMX_STORE_DIFF_8P   xmm5, xmm14, xmm15, [parm1q+4*FDEC_STRIDE]
    MMX_STORE_DIFF_8P   xmm9, xmm14, xmm15, [parm1q+5*FDEC_STRIDE]
    MMX_STORE_DIFF_8P   xmm6, xmm14, xmm15, [parm1q+6*FDEC_STRIDE]
    MMX_STORE_DIFF_8P   xmm7, xmm14, xmm15, [parm1q+7*FDEC_STRIDE]

    ret


;-----------------------------------------------------------------------------
;   void __cdecl x264_sub8x8_dct_mmx( int16_t dct[4][4][4],
;                                     uint8_t *pix1, uint8_t *pix2 )
;-----------------------------------------------------------------------------
%macro SUB_NxN_DCT 6
cglobal %1
    call %2
    add  parm1q, %3
    add  parm2q, %4-%5*FENC_STRIDE
    add  parm3q, %4-%5*FDEC_STRIDE
    call %2
    add  parm1q, %3
    add  parm2q, %4*FENC_STRIDE-%6
    add  parm3q, %4*FDEC_STRIDE-%6
    call %2
    add  parm1q, %3
    add  parm2q, %4-%5*FENC_STRIDE
    add  parm3q, %4-%5*FDEC_STRIDE
    jmp  %2
%endmacro

;-----------------------------------------------------------------------------
;   void __cdecl x264_add8x8_idct_mmx( uint8_t *pix, int16_t dct[4][4][4] )
;-----------------------------------------------------------------------------
%macro ADD_NxN_IDCT 6
cglobal %1
    call %2
    add  parm1q, %4-%5*FDEC_STRIDE
    add  parm2q, %3
    call %2
    add  parm1q, %4*FDEC_STRIDE-%6
    add  parm2q, %3
    call %2
    add  parm1q, %4-%5*FDEC_STRIDE
    add  parm2q, %3
    jmp  %2
%endmacro

SUB_NxN_DCT  x264_sub8x8_dct_mmx,      x264_sub4x4_dct_mmx,     32, 4, 0,  4
ADD_NxN_IDCT x264_add8x8_idct_mmx,     x264_add4x4_idct_mmx,    32, 4, 0,  4

SUB_NxN_DCT  x264_sub16x16_dct_mmx,    x264_sub8x8_dct_mmx,     32, 4, 4, 12
ADD_NxN_IDCT x264_add16x16_idct_mmx,   x264_add8x8_idct_mmx,    32, 4, 4, 12

SUB_NxN_DCT  x264_sub16x16_dct8_sse2,  x264_sub8x8_dct8_sse2,  128, 8, 0,  8
ADD_NxN_IDCT x264_add16x16_idct8_sse2, x264_add8x8_idct8_sse2, 128, 8, 0,  8


;-----------------------------------------------------------------------------
; void __cdecl x264_zigzag_scan_4x4_field_sse2( int level[16], int16_t dct[4][4] )
;-----------------------------------------------------------------------------
cglobal x264_zigzag_scan_4x4_field_sse2
    punpcklwd xmm0, [parm2q]
    punpckhwd xmm1, [parm2q]
    punpcklwd xmm2, [parm2q+16]
    punpckhwd xmm3, [parm2q+16]
    psrad     xmm0, 16
    psrad     xmm1, 16
    psrad     xmm2, 16
    psrad     xmm3, 16
    movq      [parm1q   ], xmm0
    movdqa    [parm1q+16], xmm1
    movdqa    [parm1q+32], xmm2
    movhlps   xmm0, xmm0
    movdqa    [parm1q+48], xmm3
    movq      [parm1q+12], xmm0
    movd      [parm1q+ 8], xmm1
    ret


;*****************************************************************************
;* dct-a.asm: h264 encoder library
;*****************************************************************************
;* Copyright (C) 2003-2008 x264 project
;*
;* Authors: Laurent Aimar <fenrir@via.ecp.fr>
;*          Min Chen <chenm001.163.com>
;*          Loren Merritt <lorenm@u.washington.edu>
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
pw_1:  times 8 dw 1
pw_32: times 8 dw 32

SECTION .text

%macro LOAD_DIFF_4P 5
    movd        %1, %4
    punpcklbw   %1, %3
    movd        %2, %5
    punpcklbw   %2, %3
    psubw       %1, %2
%endmacro

%macro SUMSUB_BA 2
    paddw   %1, %2
    paddw   %2, %2
    psubw   %2, %1
%endmacro

%macro SUMSUB_BADC 4
    paddw   %1, %2
    paddw   %3, %4
    paddw   %2, %2
    paddw   %4, %4
    psubw   %2, %1
    psubw   %4, %3
%endmacro

%macro SUMSUB2_AB 3
    movq    %3, %1
    paddw   %1, %1
    paddw   %1, %2
    psubw   %3, %2
    psubw   %3, %2
%endmacro

%macro SUMSUBD2_AB 4
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
%macro TRANSPOSE4x4W 5
    SBUTTERFLY q, wd, %1, %2, %5
    SBUTTERFLY q, wd, %3, %4, %2
    SBUTTERFLY q, dq, %1, %3, %4
    SBUTTERFLY q, dq, %5, %2, %3
%endmacro

%macro STORE_DIFF_4P 5
    paddw       %1, %3
    psraw       %1, 6
    movd        %2, %5
    punpcklbw   %2, %4
    paddsw      %1, %2
    packuswb    %1, %1
    movd        %5, %1
%endmacro

;-----------------------------------------------------------------------------
; void x264_dct4x4dc_mmx( int16_t d[4][4] )
;-----------------------------------------------------------------------------
cglobal x264_dct4x4dc_mmx, 1,1,1
    movq      mm0, [r0+ 0]
    movq      mm1, [r0+ 8]
    movq      mm2, [r0+16]
    movq      mm3, [r0+24]

    SUMSUB_BADC    mm1, mm0, mm3, mm2          ; mm1=s01  mm0=d01  mm3=s23  mm2=d23
    SUMSUB_BADC    mm3, mm1, mm2, mm0          ; mm3=s01+s23  mm1=s01-s23  mm2=d01+d23  mm0=d01-d23

    TRANSPOSE4x4W  mm3, mm1, mm0, mm2, mm4     ; in: mm3, mm1, mm0, mm2  out: mm3, mm2, mm4, mm0 

    SUMSUB_BADC    mm2, mm3, mm0, mm4          ; mm2=s01  mm3=d01  mm0=s23  mm4=d23
    SUMSUB_BADC    mm0, mm2, mm4, mm3          ; mm0=s01+s23  mm2=s01-s23  mm4=d01+d23  mm3=d01-d23

    movq      mm6, [pw_1 GLOBAL]
    paddw     mm0, mm6
    paddw     mm2, mm6
    psraw     mm0, 1
    movq  [r0+ 0], mm0
    psraw     mm2, 1
    movq  [r0+ 8], mm2
    paddw     mm3, mm6
    paddw     mm4, mm6
    psraw     mm3, 1
    movq  [r0+16], mm3
    psraw     mm4, 1
    movq  [r0+24], mm4
    RET

;-----------------------------------------------------------------------------
; void x264_idct4x4dc_mmx( int16_t d[4][4] )
;-----------------------------------------------------------------------------
cglobal x264_idct4x4dc_mmx, 1,1
    movq    mm0, [r0+ 0]
    movq    mm1, [r0+ 8]
    movq    mm2, [r0+16]
    movq    mm3, [r0+24]

    SUMSUB_BADC    mm1, mm0, mm3, mm2          ; mm1=s01  mm0=d01  mm3=s23  mm2=d23
    SUMSUB_BADC    mm3, mm1, mm2, mm0          ; mm3=s01+s23 mm1=s01-s23 mm2=d01+d23 mm0=d01-d23

    TRANSPOSE4x4W  mm3, mm1, mm0, mm2, mm4     ; in: mm3, mm1, mm0, mm2  out: mm3, mm2, mm4, mm0 

    SUMSUB_BADC    mm2, mm3, mm0, mm4          ; mm2=s01  mm3=d01  mm0=s23  mm4=d23
    SUMSUB_BADC    mm0, mm2, mm4, mm3          ; mm0=s01+s23  mm2=s01-s23  mm4=d01+d23  mm3=d01-d23

    movq    [r0+ 0], mm0
    movq    [r0+ 8], mm2
    movq    [r0+16], mm3
    movq    [r0+24], mm4
    RET

;-----------------------------------------------------------------------------
; void x264_sub4x4_dct_mmx( int16_t dct[4][4], uint8_t *pix1, uint8_t *pix2 )
;-----------------------------------------------------------------------------
cglobal x264_sub4x4_dct_mmx, 3,3
.skip_prologue:
    pxor mm7, mm7

    ; Load 4 lines
    LOAD_DIFF_4P   mm0, mm6, mm7, [r1+0*FENC_STRIDE], [r2+0*FDEC_STRIDE]
    LOAD_DIFF_4P   mm1, mm6, mm7, [r1+1*FENC_STRIDE], [r2+1*FDEC_STRIDE]
    LOAD_DIFF_4P   mm2, mm6, mm7, [r1+2*FENC_STRIDE], [r2+2*FDEC_STRIDE]
    LOAD_DIFF_4P   mm3, mm6, mm7, [r1+3*FENC_STRIDE], [r2+3*FDEC_STRIDE]

    SUMSUB_BADC    mm3, mm0, mm2, mm1          ; mm3=s03  mm0=d03  mm2=s12  mm1=d12

    SUMSUB_BA      mm2, mm3                    ; mm2=s03+s12      mm3=s03-s12
    SUMSUB2_AB     mm0, mm1, mm4               ; mm0=2.d03+d12    mm4=d03-2.d12

    ; transpose in: mm2, mm0, mm3, mm4, out: mm2, mm4, mm1, mm3
    TRANSPOSE4x4W  mm2, mm0, mm3, mm4, mm1

    SUMSUB_BADC    mm3, mm2, mm1, mm4          ; mm3=s03  mm2=d03  mm1=s12  mm4=d12

    SUMSUB_BA      mm1, mm3                    ; mm1=s03+s12      mm3=s03-s12
    SUMSUB2_AB     mm2, mm4, mm0               ; mm2=2.d03+d12    mm0=d03-2.d12

    movq    [r0+ 0], mm1
    movq    [r0+ 8], mm2
    movq    [r0+16], mm3
    movq    [r0+24], mm0
    RET

;-----------------------------------------------------------------------------
; void x264_add4x4_idct_mmx( uint8_t *p_dst, int16_t dct[4][4] )
;-----------------------------------------------------------------------------
cglobal x264_add4x4_idct_mmx, 2,2,1
.skip_prologue:
    ; Load dct coeffs
    movq    mm0, [r1+ 0] ; dct
    movq    mm1, [r1+ 8]
    movq    mm2, [r1+16]
    movq    mm3, [r1+24]
    
    SUMSUB_BA      mm2, mm0                        ; mm2=s02  mm0=d02
    SUMSUBD2_AB    mm1, mm3, mm5, mm4              ; mm1=s13  mm4=d13 ( well 1 + 3>>1 and 1>>1 + 3)

    SUMSUB_BADC    mm1, mm2, mm4, mm0              ; mm1=s02+s13  mm2=s02-s13  mm4=d02+d13  mm0=d02-d13

    ; in: mm1, mm4, mm0, mm2  out: mm1, mm2, mm3, mm0
    TRANSPOSE4x4W  mm1, mm4, mm0, mm2, mm3

    SUMSUB_BA      mm3, mm1                        ; mm3=s02  mm1=d02
    SUMSUBD2_AB    mm2, mm0, mm5, mm4              ; mm2=s13  mm4=d13 ( well 1 + 3>>1 and 1>>1 + 3)

    SUMSUB_BADC    mm2, mm3, mm4, mm1              ; mm2=s02+s13  mm3=s02-s13  mm4=d02+d13  mm1=d02-d13

    pxor mm7, mm7
    movq           mm6, [pw_32 GLOBAL]
    
    STORE_DIFF_4P  mm2, mm0, mm6, mm7, [r0+0*FDEC_STRIDE]
    STORE_DIFF_4P  mm4, mm0, mm6, mm7, [r0+1*FDEC_STRIDE]
    STORE_DIFF_4P  mm1, mm0, mm6, mm7, [r0+2*FDEC_STRIDE]
    STORE_DIFF_4P  mm3, mm0, mm6, mm7, [r0+3*FDEC_STRIDE]

    RET



;-----------------------------------------------------------------------------
; void x264_sub8x8_dct_mmx( int16_t dct[4][4][4], uint8_t *pix1, uint8_t *pix2 )
;-----------------------------------------------------------------------------
%macro SUB_NxN_DCT 6
cglobal %1, 3,3
.skip_prologue:
    call %2
    add  r0, %3
    add  r1, %4-%5*FENC_STRIDE
    add  r2, %4-%5*FDEC_STRIDE
    call %2
    add  r0, %3
    add  r1, %4*FENC_STRIDE-%6
    add  r2, %4*FDEC_STRIDE-%6
    call %2
    add  r0, %3
    add  r1, %4-%5*FENC_STRIDE
    add  r2, %4-%5*FDEC_STRIDE
    jmp  %2
%endmacro

;-----------------------------------------------------------------------------
; void x264_add8x8_idct_mmx( uint8_t *pix, int16_t dct[4][4][4] )
;-----------------------------------------------------------------------------
%macro ADD_NxN_IDCT 6
cglobal %1, 2,2,1
.skip_prologue:
    call %2
    add  r0, %4-%5*FDEC_STRIDE
    add  r1, %3
    call %2
    add  r0, %4*FDEC_STRIDE-%6
    add  r1, %3
    call %2
    add  r0, %4-%5*FDEC_STRIDE
    add  r1, %3
    jmp  %2
%endmacro

SUB_NxN_DCT  x264_sub8x8_dct_mmx,    x264_sub4x4_dct_mmx  %+ .skip_prologue, 32, 4, 0,  4
ADD_NxN_IDCT x264_add8x8_idct_mmx,   x264_add4x4_idct_mmx %+ .skip_prologue, 32, 4, 0,  4

SUB_NxN_DCT  x264_sub16x16_dct_mmx,  x264_sub8x8_dct_mmx  %+ .skip_prologue, 32, 4, 4, 12
ADD_NxN_IDCT x264_add16x16_idct_mmx, x264_add8x8_idct_mmx %+ .skip_prologue, 32, 4, 4, 12

%ifdef ARCH_X86_64
cextern x264_sub8x8_dct8_sse2
cextern x264_add8x8_idct8_sse2
SUB_NxN_DCT  x264_sub16x16_dct8_sse2,  x264_sub8x8_dct8_sse2,  128, 8, 0,  8
ADD_NxN_IDCT x264_add16x16_idct8_sse2, x264_add8x8_idct8_sse2, 128, 8, 0,  8
%endif


;-----------------------------------------------------------------------------
; void x264_zigzag_scan_4x4_field_sse2( int level[16], int16_t dct[4][4] )
;-----------------------------------------------------------------------------
cglobal x264_zigzag_scan_4x4_field_sse2, 2,2
    punpcklwd xmm0, [r1]
    punpckhwd xmm1, [r1]
    punpcklwd xmm2, [r1+16]
    punpckhwd xmm3, [r1+16]
    psrad     xmm0, 16
    psrad     xmm1, 16
    psrad     xmm2, 16
    psrad     xmm3, 16
    movq   [r0   ], xmm0
    movdqa [r0+16], xmm1
    movdqa [r0+32], xmm2
    movhlps   xmm0, xmm0
    movdqa [r0+48], xmm3
    movq   [r0+12], xmm0
    movd   [r0+ 8], xmm1
    RET


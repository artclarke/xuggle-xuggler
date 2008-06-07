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
pb_zigzag4: db 0,1,4,8,5,2,3,6,9,12,13,10,7,11,14,15

SECTION .text

%macro LOAD_DIFF_4P 5
    movh        %1, %4
    punpcklbw   %1, %3
    movh        %2, %5
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
    mova    %3, %1
    paddw   %1, %1
    paddw   %1, %2
    psubw   %3, %2
    psubw   %3, %2
%endmacro

%macro SUMSUBD2_AB 4
    mova    %4, %1
    mova    %3, %2
    psraw   %2, 1
    psraw   %4, 1
    paddw   %1, %2
    psubw   %4, %3
%endmacro

%macro SBUTTERFLY 4
    mova       m%4, m%2
    punpckl%1  m%2, m%3
    punpckh%1  m%4, m%3
    SWAP %3, %4
%endmacro

%macro TRANSPOSE4x4W 5
    SBUTTERFLY wd, %1, %2, %5
    SBUTTERFLY wd, %3, %4, %5
    SBUTTERFLY dq, %1, %3, %5
    SBUTTERFLY dq, %2, %4, %5
    SWAP %2, %3
%endmacro

%macro TRANSPOSE2x4x4W 5
    SBUTTERFLY wd, %1, %2, %5
    SBUTTERFLY wd, %3, %4, %5
    SBUTTERFLY dq, %1, %3, %5
    SBUTTERFLY dq, %2, %4, %5
    SBUTTERFLY qdq, %1, %2, %5
    SBUTTERFLY qdq, %3, %4, %5
%endmacro

%macro STORE_DIFF_4P 4
    psraw       %1, 6
    movh        %2, %4
    punpcklbw   %2, %3
    paddsw      %1, %2
    packuswb    %1, %1
    movh        %4, %1
%endmacro

%macro HADAMARD4_1D 4
    SUMSUB_BADC m%2, m%1, m%4, m%3
    SUMSUB_BADC m%4, m%2, m%3, m%1
    SWAP %1, %4, %3
%endmacro

;-----------------------------------------------------------------------------
; void x264_dct4x4dc_mmx( int16_t d[4][4] )
;-----------------------------------------------------------------------------
cglobal x264_dct4x4dc_mmx, 1,1,1
    movq   m0, [r0+ 0]
    movq   m1, [r0+ 8]
    movq   m2, [r0+16]
    movq   m3, [r0+24]
    HADAMARD4_1D  0,1,2,3
    TRANSPOSE4x4W 0,1,2,3,4
    HADAMARD4_1D  0,1,2,3
    movq   m6, [pw_1 GLOBAL]
    paddw  m0, m6
    paddw  m1, m6
    paddw  m2, m6
    paddw  m3, m6
    psraw  m0, 1
    psraw  m1, 1
    psraw  m2, 1
    psraw  m3, 1
    movq  [r0+0], m0
    movq  [r0+8], m1
    movq [r0+16], m2
    movq [r0+24], m3
    RET

;-----------------------------------------------------------------------------
; void x264_idct4x4dc_mmx( int16_t d[4][4] )
;-----------------------------------------------------------------------------
cglobal x264_idct4x4dc_mmx, 1,1
    movq  m0, [r0+ 0]
    movq  m1, [r0+ 8]
    movq  m2, [r0+16]
    movq  m3, [r0+24]
    HADAMARD4_1D  0,1,2,3
    TRANSPOSE4x4W 0,1,2,3,4
    HADAMARD4_1D  0,1,2,3
    movq  [r0+ 0], m0
    movq  [r0+ 8], m1
    movq  [r0+16], m2
    movq  [r0+24], m3
    RET

%macro DCT4_1D 5
    SUMSUB_BADC m%4, m%1, m%3, m%2
    SUMSUB_BA   m%3, m%4
    SUMSUB2_AB  m%1, m%2, m%5
    SWAP %1, %3, %4, %5, %2
%endmacro

%macro IDCT4_1D 6
    SUMSUB_BA   m%3, m%1
    SUMSUBD2_AB m%2, m%4, m%6, m%5
    SUMSUB_BADC m%2, m%3, m%5, m%1
    SWAP %1, %2, %5, %4, %3
%endmacro

;-----------------------------------------------------------------------------
; void x264_sub4x4_dct_mmx( int16_t dct[4][4], uint8_t *pix1, uint8_t *pix2 )
;-----------------------------------------------------------------------------
cglobal x264_sub4x4_dct_mmx, 3,3
.skip_prologue:
%macro SUB_DCT4 1
    LOAD_DIFF_4P  m0, m6, m7, [r1+0*FENC_STRIDE], [r2+0*FDEC_STRIDE]
    LOAD_DIFF_4P  m1, m6, m7, [r1+1*FENC_STRIDE], [r2+1*FDEC_STRIDE]
    LOAD_DIFF_4P  m2, m6, m7, [r1+2*FENC_STRIDE], [r2+2*FDEC_STRIDE]
    LOAD_DIFF_4P  m3, m6, m7, [r1+3*FENC_STRIDE], [r2+3*FDEC_STRIDE]
    DCT4_1D 0,1,2,3,4
    TRANSPOSE%1 0,1,2,3,4
    DCT4_1D 0,1,2,3,4
    movq  [r0+ 0], m0
    movq  [r0+ 8], m1
    movq  [r0+16], m2
    movq  [r0+24], m3
%endmacro
    SUB_DCT4 4x4W
    RET

;-----------------------------------------------------------------------------
; void x264_add4x4_idct_mmx( uint8_t *p_dst, int16_t dct[4][4] )
;-----------------------------------------------------------------------------
cglobal x264_add4x4_idct_mmx, 2,2,1
.skip_prologue:
    movq  m0, [r1+ 0]
    movq  m1, [r1+ 8]
    movq  m2, [r1+16]
    movq  m3, [r1+24]
%macro ADD_IDCT4 1
    IDCT4_1D 0,1,2,3,4,5
    TRANSPOSE%1 0,1,2,3,4
    paddw m0, [pw_32 GLOBAL]
    IDCT4_1D 0,1,2,3,4,5
    pxor  m7, m7
    STORE_DIFF_4P  m0, m4, m7, [r0+0*FDEC_STRIDE]
    STORE_DIFF_4P  m1, m4, m7, [r0+1*FDEC_STRIDE]
    STORE_DIFF_4P  m2, m4, m7, [r0+2*FDEC_STRIDE]
    STORE_DIFF_4P  m3, m4, m7, [r0+3*FDEC_STRIDE]
%endmacro
    ADD_IDCT4 4x4W
    RET

INIT_XMM

cglobal x264_sub8x8_dct_sse2, 3,3
.skip_prologue:
    call .8x4
    add  r0, 64
    add  r1, 4*FENC_STRIDE
    add  r2, 4*FDEC_STRIDE
.8x4:
    SUB_DCT4 2x4x4W
    movhps [r0+32], m0
    movhps [r0+40], m1
    movhps [r0+48], m2
    movhps [r0+56], m3
    ret

cglobal x264_add8x8_idct_sse2, 2,2,1
.skip_prologue:
    call .8x4
    add  r1, 64
    add  r0, 4*FDEC_STRIDE
.8x4:
    movq   m0, [r1+ 0]
    movq   m1, [r1+ 8]
    movq   m2, [r1+16]
    movq   m3, [r1+24]
    movhps m0, [r1+32]
    movhps m1, [r1+40]
    movhps m2, [r1+48]
    movhps m3, [r1+56]
    ADD_IDCT4 2x4x4W
    ret

;-----------------------------------------------------------------------------
; void x264_sub8x8_dct_mmx( int16_t dct[4][4][4], uint8_t *pix1, uint8_t *pix2 )
;-----------------------------------------------------------------------------
%macro SUB_NxN_DCT 6
cglobal %1, 3,3
.skip_prologue:
    call %2
    add  r0, %3
    add  r1, %4-%5-%6*FENC_STRIDE
    add  r2, %4-%5-%6*FDEC_STRIDE
    call %2
    add  r0, %3
    add  r1, (%4-%6)*FENC_STRIDE-%5-%4
    add  r2, (%4-%6)*FDEC_STRIDE-%5-%4
    call %2
    add  r0, %3
    add  r1, %4-%5-%6*FENC_STRIDE
    add  r2, %4-%5-%6*FDEC_STRIDE
    jmp  %2
%endmacro

;-----------------------------------------------------------------------------
; void x264_add8x8_idct_mmx( uint8_t *pix, int16_t dct[4][4][4] )
;-----------------------------------------------------------------------------
%macro ADD_NxN_IDCT 6
cglobal %1, 2,2,1
.skip_prologue:
    call %2
    add  r0, %4-%5-%6*FDEC_STRIDE
    add  r1, %3
    call %2
    add  r0, (%4-%6)*FDEC_STRIDE-%5-%4
    add  r1, %3
    call %2
    add  r0, %4-%5-%6*FDEC_STRIDE
    add  r1, %3
    jmp  %2
%endmacro

%ifndef ARCH_X86_64
SUB_NxN_DCT  x264_sub8x8_dct_mmx,    x264_sub4x4_dct_mmx  %+ .skip_prologue, 32, 4, 0, 0
ADD_NxN_IDCT x264_add8x8_idct_mmx,   x264_add4x4_idct_mmx %+ .skip_prologue, 32, 4, 0, 0
SUB_NxN_DCT  x264_sub16x16_dct_mmx,  x264_sub8x8_dct_mmx  %+ .skip_prologue, 32, 8, 4, 4
ADD_NxN_IDCT x264_add16x16_idct_mmx, x264_add8x8_idct_mmx %+ .skip_prologue, 32, 8, 4, 4

cextern x264_sub8x8_dct8_mmx.skip_prologue
cextern x264_add8x8_idct8_mmx.skip_prologue
SUB_NxN_DCT  x264_sub16x16_dct8_mmx,  x264_sub8x8_dct8_mmx  %+ .skip_prologue, 128, 8, 0, 0
ADD_NxN_IDCT x264_add16x16_idct8_mmx, x264_add8x8_idct8_mmx %+ .skip_prologue, 128, 8, 0, 0
%define x264_sub8x8_dct8_sse2 x264_sub8x8_dct8_sse2.skip_prologue
%define x264_add8x8_idct8_sse2 x264_add8x8_idct8_sse2.skip_prologue
%endif

SUB_NxN_DCT  x264_sub16x16_dct_sse2,  x264_sub8x8_dct_sse2  %+ .skip_prologue, 64, 8, 0, 4
ADD_NxN_IDCT x264_add16x16_idct_sse2, x264_add8x8_idct_sse2 %+ .skip_prologue, 64, 8, 0, 4

cextern x264_sub8x8_dct8_sse2
cextern x264_add8x8_idct8_sse2
SUB_NxN_DCT  x264_sub16x16_dct8_sse2,  x264_sub8x8_dct8_sse2,  128, 8, 0, 0
ADD_NxN_IDCT x264_add16x16_idct8_sse2, x264_add8x8_idct8_sse2, 128, 8, 0, 0



;-----------------------------------------------------------------------------
; void x264_zigzag_scan_4x4_field_mmxext( int16_t level[16], int16_t dct[4][4] )
;-----------------------------------------------------------------------------
; sse2 is only 1 cycle faster, and ssse3/pshufb is slower on core2
cglobal x264_zigzag_scan_4x4_field_mmxext, 2,3
    pshufw     mm0, [r1+4], 0xd2
    movq       mm1, [r1+16]
    movq       mm2, [r1+24]
    movq    [r0+4], mm0
    movq   [r0+16], mm1
    movq   [r0+24], mm2
    mov        r2d, [r1]
    mov       [r0], r2d
    mov        r2d, [r1+12]
    mov    [r0+12], r2d
    RET

;-----------------------------------------------------------------------------
; void x264_zigzag_sub_4x4_frame_ssse3( int16_t level[16], const uint8_t *src, uint8_t *dst )
;-----------------------------------------------------------------------------
cglobal x264_zigzag_sub_4x4_frame_ssse3, 3,3
    movd      xmm0, [r1+0*FENC_STRIDE]
    movd      xmm1, [r1+1*FENC_STRIDE]
    movd      xmm2, [r1+2*FENC_STRIDE]
    movd      xmm3, [r1+3*FENC_STRIDE]
    movd      xmm4, [r2+0*FDEC_STRIDE]
    movd      xmm5, [r2+1*FDEC_STRIDE]
    movd      xmm6, [r2+2*FDEC_STRIDE]
    movd      xmm7, [r2+3*FDEC_STRIDE]
    movd      [r2+0*FDEC_STRIDE], xmm0
    movd      [r2+1*FDEC_STRIDE], xmm1
    movd      [r2+2*FDEC_STRIDE], xmm2
    movd      [r2+3*FDEC_STRIDE], xmm3
    picgetgot r1
    punpckldq xmm0, xmm1
    punpckldq xmm2, xmm3
    punpckldq xmm4, xmm5
    punpckldq xmm6, xmm7
    movlhps   xmm0, xmm2
    movlhps   xmm4, xmm6
    movdqa    xmm7, [pb_zigzag4 GLOBAL]
    pshufb    xmm0, xmm7
    pshufb    xmm4, xmm7
    pxor      xmm6, xmm6
    movdqa    xmm1, xmm0
    movdqa    xmm5, xmm4
    punpcklbw xmm0, xmm6
    punpckhbw xmm1, xmm6
    punpcklbw xmm4, xmm6
    punpckhbw xmm5, xmm6
    psubw     xmm0, xmm4
    psubw     xmm1, xmm5
    movdqa    [r0], xmm0
    movdqa [r0+16], xmm1
    RET

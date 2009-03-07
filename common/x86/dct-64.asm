;*****************************************************************************
;* dct-64.asm: h264 encoder library
;*****************************************************************************
;* Copyright (C) 2003-2008 x264 project
;*
;* Authors: Loren Merritt <lorenm@u.washington.edu>
;*          Holger Lubitz <holger@lubitz.org>
;*          Laurent Aimar <fenrir@via.ecp.fr>
;*          Min Chen <chenm001.163.com>
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
;*****************************************************************************

%include "x86inc.asm"
%include "x86util.asm"

SECTION_RODATA
pw_32: times 8 dw 32
hsub_mul: times 8 db 1, -1

SECTION .text
INIT_XMM

%macro DCT8_1D 10
    SUMSUB_BA  m%5, m%4 ; %5=s34, %4=d34
    SUMSUB_BA  m%6, m%3 ; %6=s25, %3=d25
    SUMSUB_BA  m%7, m%2 ; %7=s16, %2=d16
    SUMSUB_BA  m%8, m%1 ; %8=s07, %1=d07

    SUMSUB_BA  m%6, m%7, m%10 ; %6=a1, %7=a3
    SUMSUB_BA  m%5, m%8, m%10 ; %5=a0, %8=a2

    movdqa  m%9, m%1
    psraw   m%9, 1
    paddw   m%9, m%1
    paddw   m%9, m%2
    paddw   m%9, m%3 ; %9=a4

    movdqa  m%10, m%4
    psraw   m%10, 1
    paddw   m%10, m%4
    paddw   m%10, m%2
    psubw   m%10, m%3 ; %10=a7

    SUMSUB_BA  m%4, m%1
    psubw   m%1, m%3
    psubw   m%4, m%2
    psraw   m%3, 1
    psraw   m%2, 1
    psubw   m%1, m%3 ; %1=a5
    psubw   m%4, m%2 ; %4=a6

    movdqa  m%2, m%10
    psraw   m%2, 2
    paddw   m%2, m%9 ; %2=b1
    psraw   m%9, 2
    psubw   m%9, m%10 ; %9=b7

    SUMSUB_BA  m%6, m%5, m%10 ; %6=b0, %5=b4

    movdqa  m%3, m%7
    psraw   m%3, 1
    paddw   m%3, m%8 ; %3=b2
    psraw   m%8, 1
    psubw   m%8, m%7 ; %8=b6

    movdqa  m%7, m%4
    psraw   m%7, 2
    paddw   m%7, m%1 ; %7=b3
    psraw   m%1, 2
    psubw   m%4, m%1 ; %4=b5

    SWAP %1, %6, %4, %7, %8, %9
%endmacro

%macro IDCT8_1D 10
    SUMSUB_BA  m%5, m%1, m%9 ; %5=a0, %1=a2

    movdqa  m%9, m%2
    psraw   m%9, 1
    paddw   m%9, m%2
    paddw   m%9, m%4
    paddw   m%9, m%6 ; %9=a7

    movdqa  m%10, m%3
    psraw   m%3, 1
    psubw   m%3, m%7 ; %3=a4
    psraw   m%7, 1
    paddw   m%7, m%10 ; %7=a6

    movdqa  m%10, m%6
    psraw   m%10, 1
    paddw   m%10, m%6
    paddw   m%10, m%8
    psubw   m%10, m%2 ; %10=a5

    psubw   m%2, m%4
    psubw   m%6, m%4
    paddw   m%2, m%8
    psubw   m%6, m%8
    psraw   m%4, 1
    psraw   m%8, 1
    psubw   m%2, m%4 ; %2=a3
    psubw   m%6, m%8 ; %6=a1

    movdqa  m%4, m%9
    psraw   m%4, 2
    paddw   m%4, m%6 ; %4=b1
    psraw   m%6, 2
    psubw   m%9, m%6 ; %9=b7

    SUMSUB_BA m%7, m%5, m%6 ; %7=b0, %5=b6
    SUMSUB_BA m%3, m%1, m%6; %3=b2, %1=b4

    movdqa  m%8, m%10
    psraw   m%8, 2
    paddw   m%8, m%2 ; %8=b3
    psraw   m%2, 2
    psubw   m%2, m%10 ; %2=b5

    SUMSUB_BA m%9, m%7, m%6 ; %9=c0, %7=c7
    SUMSUB_BA m%2, m%3, m%6 ; %2=c1, %3=c6
    SUMSUB_BA m%8, m%1, m%6 ; %8=c2, %1=c5
    SUMSUB_BA m%4, m%5, m%6 ; %4=c3, %5=c4

    SWAP %1, %9, %6
    SWAP %3, %8, %7
%endmacro

%macro DCT_SUB8 1
cglobal x264_sub8x8_dct_%1, 3,3,11
    add r2, 4*FDEC_STRIDE
%ifnidn %1, sse2
    mova m7, [hsub_mul GLOBAL]
%endif
%ifdef WIN64
    call .skip_prologue
    RET
%endif
global x264_sub8x8_dct_%1.skip_prologue
.skip_prologue:
    SWAP 7, 9
    LOAD_DIFF8x4 0, 1, 2, 3, 8, 9, r1, r2-4*FDEC_STRIDE
    LOAD_DIFF8x4 4, 5, 6, 7, 8, 9, r1, r2-4*FDEC_STRIDE
    DCT4_1D 0, 1, 2, 3, 8
    TRANSPOSE2x4x4W 0, 1, 2, 3, 8
    DCT4_1D 4, 5, 6, 7, 8
    TRANSPOSE2x4x4W 4, 5, 6, 7, 8
    DCT4_1D 0, 1, 2, 3, 8
    STORE_DCT 0, 1, 2, 3, r0, 0
    DCT4_1D 4, 5, 6, 7, 8
    STORE_DCT 4, 5, 6, 7, r0, 64
    ret

;-----------------------------------------------------------------------------
; void x264_sub8x8_dct8_sse2( int16_t dct[8][8], uint8_t *pix1, uint8_t *pix2 )
;-----------------------------------------------------------------------------
cglobal x264_sub8x8_dct8_%1, 3,3,11
    add r2, 4*FDEC_STRIDE
%ifnidn %1, sse2
    mova m7, [hsub_mul GLOBAL]
%endif
%ifdef WIN64
    call .skip_prologue
    RET
%endif
global x264_sub8x8_dct8_%1.skip_prologue
.skip_prologue:
    SWAP 7, 10
    LOAD_DIFF8x4  0, 1, 2, 3, 4, 10, r1, r2-4*FDEC_STRIDE
    LOAD_DIFF8x4  4, 5, 6, 7, 8, 10, r1, r2-4*FDEC_STRIDE
    DCT8_1D       0,1,2,3,4,5,6,7,8,9
    TRANSPOSE8x8W 0,1,2,3,4,5,6,7,8
    DCT8_1D       0,1,2,3,4,5,6,7,8,9
    movdqa  [r0+0x00], m0
    movdqa  [r0+0x10], m1
    movdqa  [r0+0x20], m2
    movdqa  [r0+0x30], m3
    movdqa  [r0+0x40], m4
    movdqa  [r0+0x50], m5
    movdqa  [r0+0x60], m6
    movdqa  [r0+0x70], m7
    ret
%endmacro

%define LOAD_DIFF8x4 LOAD_DIFF8x4_SSE2
%define movdqa movaps
%define punpcklqdq movlhps
DCT_SUB8 sse2
%undef movdqa
%undef punpcklqdq
%define LOAD_DIFF8x4 LOAD_DIFF8x4_SSSE3
DCT_SUB8 ssse3

;-----------------------------------------------------------------------------
; void x264_add8x8_idct8_sse2( uint8_t *p_dst, int16_t dct[8][8] )
;-----------------------------------------------------------------------------
cglobal x264_add8x8_idct8_sse2, 2,2,11
    add r0, 4*FDEC_STRIDE
    pxor m7, m7
%ifdef WIN64
    call .skip_prologue
    RET
%endif
global x264_add8x8_idct8_sse2.skip_prologue
.skip_prologue:
    SWAP 7, 9
    movdqa  m0, [r1+0x00]
    movdqa  m1, [r1+0x10]
    movdqa  m2, [r1+0x20]
    movdqa  m3, [r1+0x30]
    movdqa  m4, [r1+0x40]
    movdqa  m5, [r1+0x50]
    movdqa  m6, [r1+0x60]
    movdqa  m7, [r1+0x70]
    IDCT8_1D      0,1,2,3,4,5,6,7,8,10
    TRANSPOSE8x8W 0,1,2,3,4,5,6,7,8
    paddw         m0, [pw_32 GLOBAL] ; rounding for the >>6 at the end
    IDCT8_1D      0,1,2,3,4,5,6,7,8,10
    DIFFx2 m0, m1, m8, m9, [r0-4*FDEC_STRIDE], [r0-3*FDEC_STRIDE]
    DIFFx2 m2, m3, m8, m9, [r0-2*FDEC_STRIDE], [r0-1*FDEC_STRIDE]
    DIFFx2 m4, m5, m8, m9, [r0+0*FDEC_STRIDE], [r0+1*FDEC_STRIDE]
    DIFFx2 m6, m7, m8, m9, [r0+2*FDEC_STRIDE], [r0+3*FDEC_STRIDE]
    STORE_IDCT m1, m3, m5, m7
    ret

;-----------------------------------------------------------------------------
; void x264_add8x8_idct_sse2( uint8_t *pix, int16_t dct[4][4][4] )
;-----------------------------------------------------------------------------
cglobal x264_add8x8_idct_sse2, 2,2,11
    add  r0, 4*FDEC_STRIDE
    pxor m7, m7
%ifdef WIN64
    call .skip_prologue
    RET
%endif
global x264_add8x8_idct_sse2.skip_prologue
.skip_prologue:
    SWAP 7, 9
    mova   m0, [r1+ 0]
    mova   m2, [r1+16]
    mova   m1, [r1+32]
    mova   m3, [r1+48]
    SBUTTERFLY qdq, 0, 1, 4
    SBUTTERFLY qdq, 2, 3, 4
    mova   m4, [r1+64]
    mova   m6, [r1+80]
    mova   m5, [r1+96]
    mova   m7, [r1+112]
    SBUTTERFLY qdq, 4, 5, 8
    SBUTTERFLY qdq, 6, 7, 8
    IDCT4_1D 0,1,2,3,8,10
    TRANSPOSE2x4x4W 0,1,2,3,8
    IDCT4_1D 4,5,6,7,8,10
    TRANSPOSE2x4x4W 4,5,6,7,8
    paddw m0, [pw_32 GLOBAL]
    IDCT4_1D 0,1,2,3,8,10
    paddw m4, [pw_32 GLOBAL]
    IDCT4_1D 4,5,6,7,8,10
    DIFFx2 m0, m1, m8, m9, [r0-4*FDEC_STRIDE], [r0-3*FDEC_STRIDE]
    DIFFx2 m2, m3, m8, m9, [r0-2*FDEC_STRIDE], [r0-1*FDEC_STRIDE]
    DIFFx2 m4, m5, m8, m9, [r0+0*FDEC_STRIDE], [r0+1*FDEC_STRIDE]
    DIFFx2 m6, m7, m8, m9, [r0+2*FDEC_STRIDE], [r0+3*FDEC_STRIDE]
    STORE_IDCT m1, m3, m5, m7
    ret

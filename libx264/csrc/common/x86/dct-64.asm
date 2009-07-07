;*****************************************************************************
;* dct-64.asm: h264 encoder library
;*****************************************************************************
;* Copyright (C) 2003-2008 x264 project
;*
;* Authors: Laurent Aimar <fenrir@via.ecp.fr> (initial version)
;*          Loren Merritt <lorenm@u.washington.edu> (dct8, misc)
;*          Min Chen <chenm001.163.com> (converted to nasm)
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

SECTION .text
INIT_XMM

%macro DCT8_1D 10
    SUMSUB_BA  m%8, m%1 ; %8=s07, %1=d07
    SUMSUB_BA  m%7, m%2 ; %7=s16, %2=d16
    SUMSUB_BA  m%6, m%3 ; %6=s25, %3=d25
    SUMSUB_BA  m%5, m%4 ; %5=s34, %4=d34

    SUMSUB_BA  m%5, m%8 ; %5=a0, %8=a2
    SUMSUB_BA  m%6, m%7 ; %6=a1, %7=a3

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

    SUMSUB_BA  m%6, m%5 ; %6=b0, %5=b4

    movdqa  m%2, m%10
    psraw   m%2, 2
    paddw   m%2, m%9 ; %2=b1
    psraw   m%9, 2
    psubw   m%9, m%10 ; %9=b7

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

;-----------------------------------------------------------------------------
; void x264_sub8x8_dct8_sse2( int16_t dct[8][8], uint8_t *pix1, uint8_t *pix2 )
;-----------------------------------------------------------------------------
cglobal x264_sub8x8_dct8_sse2
    LOAD_DIFF  m0, m8, m9, [r1+0*FENC_STRIDE], [r2+0*FDEC_STRIDE]
    LOAD_DIFF  m1, m8, m9, [r1+1*FENC_STRIDE], [r2+1*FDEC_STRIDE]
    LOAD_DIFF  m2, m8, m9, [r1+2*FENC_STRIDE], [r2+2*FDEC_STRIDE]
    LOAD_DIFF  m3, m8, m9, [r1+3*FENC_STRIDE], [r2+3*FDEC_STRIDE]
    LOAD_DIFF  m4, m8, m9, [r1+4*FENC_STRIDE], [r2+4*FDEC_STRIDE]
    LOAD_DIFF  m5, m8, m9, [r1+5*FENC_STRIDE], [r2+5*FDEC_STRIDE]
    LOAD_DIFF  m6, m8, m9, [r1+6*FENC_STRIDE], [r2+6*FDEC_STRIDE]
    LOAD_DIFF  m7, m8, m9, [r1+7*FENC_STRIDE], [r2+7*FDEC_STRIDE]

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


%macro IDCT8_1D 10
    SUMSUB_BA  m%5, m%1 ; %5=a0, %1=a2
    movdqa  m%10, m%3
    psraw   m%3, 1
    psubw   m%3, m%7 ; %3=a4
    psraw   m%7, 1
    paddw   m%7, m%10 ; %7=a6

    movdqa  m%9, m%2
    psraw   m%9, 1
    paddw   m%9, m%2
    paddw   m%9, m%4
    paddw   m%9, m%6 ; %9=a7

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

    SUMSUB_BA m%7, m%5 ; %7=b0, %5=b6
    SUMSUB_BA m%3, m%1 ; %3=b2, %1=b4

    movdqa  m%4, m%9
    psraw   m%4, 2
    paddw   m%4, m%6 ; %4=b1
    psraw   m%6, 2
    psubw   m%9, m%6 ; %9=b7

    movdqa  m%8, m%10
    psraw   m%8, 2
    paddw   m%8, m%2 ; %8=b3
    psraw   m%2, 2
    psubw   m%2, m%10 ; %2=b5

    SUMSUB_BA m%9, m%7 ; %9=c0, %7=c7
    SUMSUB_BA m%2, m%3 ; %2=c1, %3=c6
    SUMSUB_BA m%8, m%1 ; %8=c2, %1=c5
    SUMSUB_BA m%4, m%5 ; %4=c3, %5=c4

    SWAP %1, %9, %6
    SWAP %3, %8, %7
%endmacro

;-----------------------------------------------------------------------------
; void x264_add8x8_idct8_sse2( uint8_t *p_dst, int16_t dct[8][8] )
;-----------------------------------------------------------------------------
cglobal x264_add8x8_idct8_sse2
    movdqa  m0, [r1+0x00]
    movdqa  m1, [r1+0x10]
    movdqa  m2, [r1+0x20]
    movdqa  m3, [r1+0x30]
    movdqa  m4, [r1+0x40]
    movdqa  m5, [r1+0x50]
    movdqa  m6, [r1+0x60]
    movdqa  m7, [r1+0x70]

    IDCT8_1D      0,1,2,3,4,5,6,7,8,9
    TRANSPOSE8x8W 0,1,2,3,4,5,6,7,8
    paddw         m0, [pw_32 GLOBAL] ; rounding for the >>6 at the end
    IDCT8_1D      0,1,2,3,4,5,6,7,8,9

    pxor  m9, m9
    STORE_DIFF m0, m8, m9, [r0+0*FDEC_STRIDE]
    STORE_DIFF m1, m8, m9, [r0+1*FDEC_STRIDE]
    STORE_DIFF m2, m8, m9, [r0+2*FDEC_STRIDE]
    STORE_DIFF m3, m8, m9, [r0+3*FDEC_STRIDE]
    STORE_DIFF m4, m8, m9, [r0+4*FDEC_STRIDE]
    STORE_DIFF m5, m8, m9, [r0+5*FDEC_STRIDE]
    STORE_DIFF m6, m8, m9, [r0+6*FDEC_STRIDE]
    STORE_DIFF m7, m8, m9, [r0+7*FDEC_STRIDE]
    ret



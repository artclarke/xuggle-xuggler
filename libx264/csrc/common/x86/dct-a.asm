;*****************************************************************************
;* dct-a.asm: h264 encoder library
;*****************************************************************************
;* Copyright (C) 2003-2008 x264 project
;*
;* Authors: Laurent Aimar <fenrir@via.ecp.fr>
;*          Loren Merritt <lorenm@u.washington.edu>
;*          Holger Lubitz <hal@duncan.ol.sub.de>
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
pw_8000: times 8 dw 0x8000
pb_sub4frame:   db 0,1,4,8,5,2,3,6,9,12,13,10,7,11,14,15
pb_scan4framea: db 12,13,6,7,14,15,0,1,8,9,2,3,4,5,10,11
pb_scan4frameb: db 0,1,8,9,2,3,4,5,10,11,12,13,6,7,14,15

SECTION .text

%macro HADAMARD4_1D 4
    SUMSUB_BADC m%2, m%1, m%4, m%3
    SUMSUB_BADC m%4, m%2, m%3, m%1
    SWAP %1, %4, %3
%endmacro

%macro SUMSUB_17BIT 4 ; a, b, tmp, 0x8000
    movq  m%3, m%4
    paddw m%1, m%4
    psubw m%3, m%2
    paddw m%2, m%4
    pavgw m%3, m%1
    pavgw m%2, m%1
    psubw m%3, m%4
    psubw m%2, m%4
    SWAP %1, %2, %3
%endmacro

;-----------------------------------------------------------------------------
; void x264_dct4x4dc_mmx( int16_t d[4][4] )
;-----------------------------------------------------------------------------
cglobal x264_dct4x4dc_mmx, 1,1
    movq   m0, [r0+ 0]
    movq   m1, [r0+ 8]
    movq   m2, [r0+16]
    movq   m3, [r0+24]
    movq   m7, [pw_8000 GLOBAL] ; convert to unsigned and back, so that pavgw works
    HADAMARD4_1D  0,1,2,3
    TRANSPOSE4x4W 0,1,2,3,4
    SUMSUB_BADC m1, m0, m3, m2
    SWAP 0,1
    SWAP 2,3
    SUMSUB_17BIT 0,2,4,7
    SUMSUB_17BIT 1,3,5,7
    movq  [r0+0], m0
    movq  [r0+8], m2
    movq [r0+16], m3
    movq [r0+24], m1
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
    LOAD_DIFF  m0, m6, m7, [r1+0*FENC_STRIDE], [r2+0*FDEC_STRIDE]
    LOAD_DIFF  m1, m6, m7, [r1+1*FENC_STRIDE], [r2+1*FDEC_STRIDE]
    LOAD_DIFF  m2, m6, m7, [r1+2*FENC_STRIDE], [r2+2*FDEC_STRIDE]
    LOAD_DIFF  m3, m6, m7, [r1+3*FENC_STRIDE], [r2+3*FDEC_STRIDE]
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
cglobal x264_add4x4_idct_mmx, 2,2
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
    STORE_DIFF  m0, m4, m7, [r0+0*FDEC_STRIDE]
    STORE_DIFF  m1, m4, m7, [r0+1*FDEC_STRIDE]
    STORE_DIFF  m2, m4, m7, [r0+2*FDEC_STRIDE]
    STORE_DIFF  m3, m4, m7, [r0+3*FDEC_STRIDE]
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

cglobal x264_add8x8_idct_sse2, 2,2
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
cglobal %1, 2,2
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
; void x264_zigzag_scan_8x8_frame_ssse3( int16_t level[64], int16_t dct[8][8] )
;-----------------------------------------------------------------------------
%macro SCAN_8x8 1
cglobal x264_zigzag_scan_8x8_frame_%1, 2,2
    movdqa    xmm0, [r1]
    movdqa    xmm1, [r1+16]
    movdq2q    mm0, xmm0
    PALIGNR   xmm1, xmm1, 14, xmm2
    movdq2q    mm1, xmm1

    movdqa    xmm2, [r1+32]
    movdqa    xmm3, [r1+48]
    PALIGNR   xmm2, xmm2, 12, xmm4
    movdq2q    mm2, xmm2
    PALIGNR   xmm3, xmm3, 10, xmm4
    movdq2q    mm3, xmm3

    punpckhwd xmm0, xmm1
    punpckhwd xmm2, xmm3

    movq       mm4, mm1
    movq       mm5, mm1
    movq       mm6, mm2
    movq       mm7, mm3
    punpckhwd  mm1, mm0
    psllq      mm0, 16
    psrlq      mm3, 16
    punpckhdq  mm1, mm1
    punpckhdq  mm2, mm0
    punpcklwd  mm0, mm4
    punpckhwd  mm4, mm3
    punpcklwd  mm4, mm2
    punpckhdq  mm0, mm2
    punpcklwd  mm6, mm3
    punpcklwd  mm5, mm7
    punpcklwd  mm5, mm6

    movdqa    xmm4, [r1+64]
    movdqa    xmm5, [r1+80]
    movdqa    xmm6, [r1+96]
    movdqa    xmm7, [r1+112]

    movq [r0+2*00], mm0
    movq [r0+2*04], mm4
    movd [r0+2*08], mm1
    movq [r0+2*36], mm5
    movq [r0+2*46], mm6

    PALIGNR   xmm4, xmm4, 14, xmm3
    movdq2q    mm4, xmm4
    PALIGNR   xmm5, xmm5, 12, xmm3
    movdq2q    mm5, xmm5
    PALIGNR   xmm6, xmm6, 10, xmm3
    movdq2q    mm6, xmm6
%ifidn %1, ssse3
    PALIGNR   xmm7, xmm7, 8, xmm3
    movdq2q    mm7, xmm7
%else
    movhlps   xmm3, xmm7
    punpcklqdq xmm7, xmm7
    movdq2q    mm7, xmm3
%endif

    punpckhwd xmm4, xmm5
    punpckhwd xmm6, xmm7

    movq       mm0, mm4
    movq       mm1, mm5
    movq       mm3, mm7
    punpcklwd  mm7, mm6
    psrlq      mm6, 16
    punpcklwd  mm4, mm6
    punpcklwd  mm5, mm4
    punpckhdq  mm4, mm3
    punpcklwd  mm3, mm6
    punpckhwd  mm3, mm4
    punpckhwd  mm0, mm1
    punpckldq  mm4, mm0
    punpckhdq  mm0, mm6
    pshufw     mm4, mm4, 0x6c

    movq [r0+2*14], mm4
    movq [r0+2*25], mm0
    movd [r0+2*54], mm7
    movq [r0+2*56], mm5
    movq [r0+2*60], mm3

    movdqa    xmm3, xmm0
    movdqa    xmm7, xmm4
    punpckldq xmm0, xmm2
    punpckldq xmm4, xmm6
    punpckhdq xmm3, xmm2
    punpckhdq xmm7, xmm6
    pshufhw   xmm0, xmm0, 0x1b
    pshuflw   xmm4, xmm4, 0x1b
    pshufhw   xmm3, xmm3, 0x1b
    pshuflw   xmm7, xmm7, 0x1b

    movlps [r0+2*10], xmm0
    movhps [r0+2*17], xmm0
    movlps [r0+2*21], xmm3
    movlps [r0+2*28], xmm4
    movhps [r0+2*32], xmm3
    movhps [r0+2*39], xmm4
    movlps [r0+2*43], xmm7
    movhps [r0+2*50], xmm7

    RET
%endmacro

INIT_XMM
%define PALIGNR PALIGNR_MMX
SCAN_8x8 sse2
%define PALIGNR PALIGNR_SSSE3
SCAN_8x8 ssse3

;-----------------------------------------------------------------------------
; void x264_zigzag_scan_8x8_frame_mmxext( int16_t level[64], int16_t dct[8][8] )
;-----------------------------------------------------------------------------
cglobal x264_zigzag_scan_8x8_frame_mmxext, 2,2
    movq       mm0, [r1]
    movq       mm1, [r1+2*8]
    movq       mm2, [r1+2*14]
    movq       mm3, [r1+2*21]
    movq       mm4, [r1+2*28]
    movq       mm5, mm0
    movq       mm6, mm1
    psrlq      mm0, 16
    punpckldq  mm1, mm1
    punpcklwd  mm5, mm6
    punpckhwd  mm1, mm3
    punpckhwd  mm6, mm0
    punpckldq  mm5, mm0
    movq       mm7, [r1+2*52]
    movq       mm0, [r1+2*60]
    punpckhwd  mm1, mm2
    punpcklwd  mm2, mm4
    punpckhwd  mm4, mm3
    punpckldq  mm3, mm3
    punpckhwd  mm3, mm2
    movq      [r0], mm5
    movq  [r0+2*4], mm1
    movq  [r0+2*8], mm6
    punpcklwd  mm6, mm0
    punpcklwd  mm6, mm7
    movq       mm1, [r1+2*32]
    movq       mm5, [r1+2*39]
    movq       mm2, [r1+2*46]
    movq [r0+2*35], mm3
    movq [r0+2*47], mm4
    punpckhwd  mm7, mm0
    psllq      mm0, 16
    movq       mm3, mm5
    punpcklwd  mm5, mm1
    punpckhwd  mm1, mm2
    punpckhdq  mm3, mm3
    movq [r0+2*52], mm6
    movq [r0+2*13], mm5
    movq       mm4, [r1+2*11]
    movq       mm6, [r1+2*25]
    punpcklwd  mm5, mm7
    punpcklwd  mm1, mm3
    punpckhdq  mm0, mm7
    movq       mm3, [r1+2*4]
    movq       mm7, [r1+2*18]
    punpcklwd  mm2, mm5
    movq [r0+2*25], mm1
    movq       mm1, mm4
    movq       mm5, mm6
    punpcklwd  mm4, mm3
    punpcklwd  mm6, mm7
    punpckhwd  mm1, mm3
    punpckhwd  mm5, mm7
    movq       mm3, mm6
    movq       mm7, mm5
    punpckldq  mm6, mm4
    punpckldq  mm5, mm1
    punpckhdq  mm3, mm4
    punpckhdq  mm7, mm1
    movq       mm4, [r1+2*35]
    movq       mm1, [r1+2*49]
    pshufw     mm6, mm6, 0x1b
    pshufw     mm5, mm5, 0x1b
    movq [r0+2*60], mm0
    movq [r0+2*56], mm2
    movq       mm0, [r1+2*42]
    movq       mm2, [r1+2*56]
    movq [r0+2*17], mm3
    movq [r0+2*32], mm7
    movq [r0+2*10], mm6
    movq [r0+2*21], mm5
    movq       mm3, mm0
    movq       mm7, mm2
    punpcklwd  mm0, mm4
    punpcklwd  mm2, mm1
    punpckhwd  mm3, mm4
    punpckhwd  mm7, mm1
    movq       mm4, mm2
    movq       mm1, mm7
    punpckhdq  mm2, mm0
    punpckhdq  mm7, mm3
    punpckldq  mm4, mm0
    punpckldq  mm1, mm3
    pshufw     mm2, mm2, 0x1b
    pshufw     mm7, mm7, 0x1b
    movq [r0+2*28], mm4
    movq [r0+2*43], mm1
    movq [r0+2*39], mm2
    movq [r0+2*50], mm7
    RET

;-----------------------------------------------------------------------------
; void x264_zigzag_scan_4x4_frame_mmx( int16_t level[16], int16_t dct[4][4] )
;-----------------------------------------------------------------------------
cglobal x264_zigzag_scan_4x4_frame_mmx, 2,2
    movq       mm0, [r1]
    movq       mm1, [r1+8]
    movq       mm2, [r1+16]
    movq       mm3, [r1+24]
    movq       mm4, mm0
    movq       mm5, mm1
    movq       mm6, mm2
    movq       mm7, mm3
    psllq      mm3, 16
    psrlq      mm0, 16
    punpckldq  mm2, mm2
    punpckhdq  mm1, mm1
    punpcklwd  mm4, mm5
    punpcklwd  mm5, mm3
    punpckldq  mm4, mm0
    punpckhwd  mm5, mm2
    punpckhwd  mm0, mm6
    punpckhwd  mm6, mm7
    punpcklwd  mm1, mm0
    punpckhdq  mm3, mm6
    movq      [r0], mm4
    movq    [r0+8], mm5
    movq   [r0+16], mm1
    movq   [r0+24], mm3
    RET

;-----------------------------------------------------------------------------
; void x264_zigzag_scan_4x4_frame_ssse3( int16_t level[16], int16_t dct[4][4] )
;-----------------------------------------------------------------------------
cglobal x264_zigzag_scan_4x4_frame_ssse3, 2,2
    movdqa    xmm1, [r1+16]
    movdqa    xmm0, [r1]
    pshufb    xmm1, [pb_scan4frameb GLOBAL]
    pshufb    xmm0, [pb_scan4framea GLOBAL]
    movdqa    xmm2, xmm1
    psrldq    xmm1, 6
    palignr   xmm2, xmm0, 6
    pslldq    xmm0, 10
    palignr   xmm1, xmm0, 10
    movdqa    [r0], xmm2
    movdqa [r0+16], xmm1
    RET

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
    punpckldq xmm0, xmm1
    punpckldq xmm2, xmm3
    punpckldq xmm4, xmm5
    punpckldq xmm6, xmm7
    punpcklqdq xmm0, xmm2
    punpcklqdq xmm4, xmm6
    movdqa    xmm7, [pb_sub4frame GLOBAL]
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

INIT_MMX
cglobal x264_zigzag_interleave_8x8_cavlc_mmx, 2,3
    mov    r2d, 24
.loop:
    movq   m0, [r1+r2*4+ 0]
    movq   m1, [r1+r2*4+ 8]
    movq   m2, [r1+r2*4+16]
    movq   m3, [r1+r2*4+24]
    TRANSPOSE4x4W 0,1,2,3,4
    movq   [r0+r2+ 0], m0
    movq   [r0+r2+32], m1
    movq   [r0+r2+64], m2
    movq   [r0+r2+96], m3
    sub    r2d, 8
    jge .loop
    REP_RET

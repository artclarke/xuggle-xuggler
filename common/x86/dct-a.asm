;*****************************************************************************
;* dct-a.asm: x86 transform and zigzag
;*****************************************************************************
;* Copyright (C) 2003-2010 x264 project
;*
;* Authors: Holger Lubitz <holger@lubitz.org>
;*          Loren Merritt <lorenm@u.washington.edu>
;*          Laurent Aimar <fenrir@via.ecp.fr>
;*          Min Chen <chenm001.163.com>
;*          Jason Garrett-Glaser <darkshikari@gmail.com>
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
;*
;* This program is also available under a commercial proprietary license.
;* For more information, contact us at licensing@x264.com.
;*****************************************************************************

%include "x86inc.asm"
%include "x86util.asm"

%macro SHUFFLE_16BIT 8
    %rep 8
        db %1*2
        db %1*2+1
        %rotate 1
    %endrep
%endmacro

SECTION_RODATA
pb_sub4frame:   db 0,1,4,8,5,2,3,6,9,12,13,10,7,11,14,15
pb_sub4field:   db 0,4,1,8,12,5,9,13,2,6,10,14,3,7,11,15
pb_subacmask:   dw 0,-1,-1,-1,-1,-1,-1,-1
pb_scan4framea: SHUFFLE_16BIT 6,3,7,0,4,1,2,5
pb_scan4frameb: SHUFFLE_16BIT 0,4,1,2,5,6,3,7
pb_idctdc_unpack: db 0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3
pb_idctdc_unpack2: db 4,4,4,4,5,5,5,5,6,6,6,6,7,7,7,7

SECTION .text

cextern pw_32_0
cextern pw_32
cextern pw_8000
cextern pw_pixel_max
cextern hsub_mul
cextern pb_1
cextern pw_1
cextern pd_1
cextern pd_32

%macro WALSH4_1D 6
    SUMSUB_BADC %1, m%5, m%4, m%3, m%2, m%6
    SUMSUB_BADC %1, m%5, m%3, m%4, m%2, m%6
    SWAP %2, %5, %4
%endmacro

%macro SUMSUB_17BIT 4 ; a, b, tmp, 0x8000
    movq  m%3, m%4
    pxor  m%1, m%4
    psubw m%3, m%2
    pxor  m%2, m%4
    pavgw m%3, m%1
    pavgw m%2, m%1
    pxor  m%3, m%4
    pxor  m%2, m%4
    SWAP %1, %2, %3
%endmacro

%ifdef HIGH_BIT_DEPTH
INIT_XMM
;-----------------------------------------------------------------------------
; void dct4x4dc( dctcoef d[4][4] )
;-----------------------------------------------------------------------------
cglobal dct4x4dc_sse2, 1,1,5
    mova   m0, [r0+ 0]
    mova   m1, [r0+16]
    mova   m2, [r0+32]
    mova   m3, [r0+48]
    WALSH4_1D  d, 0,1,2,3,4
    TRANSPOSE4x4D 0,1,2,3,4
    paddd  m0, [pd_1]
    WALSH4_1D  d, 0,1,2,3,4
    psrad  m0, 1
    psrad  m1, 1
    psrad  m2, 1
    psrad  m3, 1
    mova [r0+ 0], m0
    mova [r0+16], m1
    mova [r0+32], m2
    mova [r0+48], m3
    RET
%else

INIT_MMX
cglobal dct4x4dc_mmx, 1,1
    movq   m3, [r0+24]
    movq   m2, [r0+16]
    movq   m1, [r0+ 8]
    movq   m0, [r0+ 0]
    movq   m7, [pw_8000] ; convert to unsigned and back, so that pavgw works
    WALSH4_1D  w, 0,1,2,3,4
    TRANSPOSE4x4W 0,1,2,3,4
    SUMSUB_BADC w, m1, m0, m3, m2, m4
    SWAP 0, 1
    SWAP 2, 3
    SUMSUB_17BIT 0,2,4,7
    SUMSUB_17BIT 1,3,5,7
    movq  [r0+0], m0
    movq  [r0+8], m2
    movq [r0+16], m3
    movq [r0+24], m1
    RET
%endif ; HIGH_BIT_DEPTH

%ifdef HIGH_BIT_DEPTH
;-----------------------------------------------------------------------------
; void idct4x4dc( int32_t d[4][4] )
;-----------------------------------------------------------------------------
INIT_XMM
cglobal idct4x4dc_sse2, 1,1
    mova   m3, [r0+48]
    mova   m2, [r0+32]
    mova   m1, [r0+16]
    mova   m0, [r0+ 0]
    WALSH4_1D  d,0,1,2,3,4
    TRANSPOSE4x4D 0,1,2,3,4
    WALSH4_1D  d,0,1,2,3,4
    mova  [r0+ 0], m0
    mova  [r0+16], m1
    mova  [r0+32], m2
    mova  [r0+48], m3
    RET
%else

INIT_MMX
;-----------------------------------------------------------------------------
; void idct4x4dc( int16_t d[4][4] )
;-----------------------------------------------------------------------------
cglobal idct4x4dc_mmx, 1,1
    movq   m3, [r0+24]
    movq   m2, [r0+16]
    movq   m1, [r0+ 8]
    movq   m0, [r0+ 0]
    WALSH4_1D  w,0,1,2,3,4
    TRANSPOSE4x4W 0,1,2,3,4
    WALSH4_1D  w,0,1,2,3,4
    movq  [r0+ 0], m0
    movq  [r0+ 8], m1
    movq  [r0+16], m2
    movq  [r0+24], m3
    RET
%endif ; HIGH_BIT_DEPTH

INIT_MMX
%ifdef HIGH_BIT_DEPTH
;-----------------------------------------------------------------------------
; void sub4x4_dct( dctcoef dct[4][4], pixel *pix1, pixel *pix2 )
;-----------------------------------------------------------------------------
cglobal sub4x4_dct_mmx, 3,3
.skip_prologue:
    LOAD_DIFF  m0, m4, none, [r1+0*FENC_STRIDE], [r2+0*FDEC_STRIDE]
    LOAD_DIFF  m3, m4, none, [r1+6*FENC_STRIDE], [r2+6*FDEC_STRIDE]
    LOAD_DIFF  m1, m4, none, [r1+2*FENC_STRIDE], [r2+2*FDEC_STRIDE]
    LOAD_DIFF  m2, m4, none, [r1+4*FENC_STRIDE], [r2+4*FDEC_STRIDE]
    DCT4_1D 0,1,2,3,4
    TRANSPOSE4x4W 0,1,2,3,4
    DCT4_1D 0,1,2,3,4
    STORE_DIFF m0, m4, m5, [r0+ 0], [r0+ 8]
    STORE_DIFF m1, m4, m5, [r0+16], [r0+24]
    STORE_DIFF m2, m4, m5, [r0+32], [r0+40]
    STORE_DIFF m3, m4, m5, [r0+48], [r0+56]
    RET
%else

%macro SUB_DCT4 1
cglobal sub4x4_dct_%1, 3,3
%ifidn %1, mmx
.skip_prologue:
    LOAD_DIFF  m0, m4, m5, [r1+0*FENC_STRIDE], [r2+0*FDEC_STRIDE]
    LOAD_DIFF  m3, m4, m5, [r1+3*FENC_STRIDE], [r2+3*FDEC_STRIDE]
    LOAD_DIFF  m1, m4, m5, [r1+1*FENC_STRIDE], [r2+1*FDEC_STRIDE]
    LOAD_DIFF  m2, m4, m5, [r1+2*FENC_STRIDE], [r2+2*FDEC_STRIDE]
%else
    mova m5, [hsub_mul]
    LOAD_DIFF8x4_SSSE3 0, 3, 1, 2, 4, 5, r1, r2
%endif
    DCT4_1D 0,1,2,3,4
    TRANSPOSE4x4W 0,1,2,3,4
    DCT4_1D 0,1,2,3,4
    movq  [r0+ 0], m0
    movq  [r0+ 8], m1
    movq  [r0+16], m2
    movq  [r0+24], m3
    RET
%endmacro

SUB_DCT4 mmx
SUB_DCT4 ssse3
%endif ; HIGH_BIT_DEPTH

%ifdef HIGH_BIT_DEPTH
;-----------------------------------------------------------------------------
; void add4x4_idct( pixel *p_dst, dctcoef dct[4][4] )
;-----------------------------------------------------------------------------
%macro STORE_DIFFx2 6
    psrad     %1, 6
    psrad     %2, 6
    packssdw  %1, %2
    movq      %3, %5
    movhps    %3, %6
    paddsw    %1, %3
    pxor      %4, %4
    CLIPW     %1, %4, [pw_pixel_max]
    movq      %5, %1
    movhps    %6, %1
%endmacro

INIT_XMM
cglobal add4x4_idct_sse2, 2,2,7
    pxor  m6, m6
.skip_prologue:
    mova  m1, [r1+16]
    mova  m3, [r1+48]
    mova  m2, [r1+32]
    mova  m0, [r1+ 0]
    IDCT4_1D d,0,1,2,3,4,5
    TRANSPOSE4x4D 0,1,2,3,4
    paddd m0, [pd_32]
    IDCT4_1D d,0,1,2,3,4,5
    STORE_DIFFx2 m0, m1, m4, m6, [r0+0*FDEC_STRIDE], [r0+2*FDEC_STRIDE]
    STORE_DIFFx2 m2, m3, m4, m6, [r0+4*FDEC_STRIDE], [r0+6*FDEC_STRIDE]
    RET
%else

cglobal add4x4_idct_mmx, 2,2
    pxor m7, m7
.skip_prologue:
    movq  m1, [r1+ 8]
    movq  m3, [r1+24]
    movq  m2, [r1+16]
    movq  m0, [r1+ 0]
    IDCT4_1D w,0,1,2,3,4,5
    TRANSPOSE4x4W 0,1,2,3,4
    paddw m0, [pw_32]
    IDCT4_1D w,0,1,2,3,4,5
    STORE_DIFF  m0, m4, m7, [r0+0*FDEC_STRIDE]
    STORE_DIFF  m1, m4, m7, [r0+1*FDEC_STRIDE]
    STORE_DIFF  m2, m4, m7, [r0+2*FDEC_STRIDE]
    STORE_DIFF  m3, m4, m7, [r0+3*FDEC_STRIDE]
    RET

INIT_XMM
cglobal add4x4_idct_sse4, 2,2,6
    mova      m0, [r1+0x00]     ; row1/row0
    mova      m2, [r1+0x10]     ; row3/row2
    mova      m1, m0            ; row1/row0
    psraw     m0, 1             ; row1>>1/...
    mova      m3, m2            ; row3/row2
    psraw     m2, 1             ; row3>>1/...
    movsd     m0, m1            ; row1>>1/row0
    movsd     m2, m3            ; row3>>1/row2
    psubw     m0, m3            ; row1>>1-row3/row0-2
    paddw     m2, m1            ; row3>>1+row1/row0+2
    SBUTTERFLY2 wd, 0, 2, 1
    SUMSUB_BA w, m2, m0, m1
    pshuflw   m1, m2, 10110001b
    pshufhw   m2, m2, 10110001b
    punpckldq m1, m0
    punpckhdq m2, m0
    SWAP 0, 1

    mova      m1, [pw_32_0]
    paddw     m1, m0            ; row1/row0 corrected
    psraw     m0, 1             ; row1>>1/...
    mova      m3, m2            ; row3/row2
    psraw     m2, 1             ; row3>>1/...
    movsd     m0, m1            ; row1>>1/row0
    movsd     m2, m3            ; row3>>1/row2
    psubw     m0, m3            ; row1>>1-row3/row0-2
    paddw     m2, m1            ; row3>>1+row1/row0+2
    SBUTTERFLY2 qdq, 0, 2, 1
    SUMSUB_BA w, m2, m0, m1

    movd      m4, [r0+FDEC_STRIDE*0]
    movd      m1, [r0+FDEC_STRIDE*1]
    movd      m3, [r0+FDEC_STRIDE*2]
    movd      m5, [r0+FDEC_STRIDE*3]
    punpckldq m1, m4            ; row0/row1
    pxor      m4, m4
    punpckldq m3, m5            ; row3/row2
    punpcklbw m1, m4
    psraw     m2, 6
    punpcklbw m3, m4
    psraw     m0, 6
    paddsw    m2, m1
    paddsw    m0, m3
    packuswb  m0, m2            ; row0/row1/row3/row2
    pextrd   [r0+FDEC_STRIDE*0], m0, 3
    pextrd   [r0+FDEC_STRIDE*1], m0, 2
    movd     [r0+FDEC_STRIDE*2], m0
    pextrd   [r0+FDEC_STRIDE*3], m0, 1
    RET
%endif ; HIGH_BIT_DEPTH

INIT_MMX
;-----------------------------------------------------------------------------
; void sub8x8_dct( int16_t dct[4][4][4], uint8_t *pix1, uint8_t *pix2 )
;-----------------------------------------------------------------------------
%macro SUB_NxN_DCT 6
cglobal %1, 3,3,11*(mmsize/16)
%ifndef HIGH_BIT_DEPTH
%if mmsize == 8
    pxor m7, m7
%else
    add r2, 4*FDEC_STRIDE
    mova m7, [hsub_mul]
%endif
%endif ; !HIGH_BIT_DEPTH
.skip_prologue:
%ifdef WIN64
    sub  rsp, 8
%endif
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
%ifdef WIN64
    add  rsp, 8
    call %2
    RET
%else
    jmp  %2
%endif
%endmacro

;-----------------------------------------------------------------------------
; void add8x8_idct( uint8_t *pix, int16_t dct[4][4][4] )
;-----------------------------------------------------------------------------
%macro ADD_NxN_IDCT 6-7
cglobal %1, 2,2,11*(mmsize/16)
    pxor m7, m7
%if mmsize==16
    add  r0, 4*FDEC_STRIDE
%endif
.skip_prologue:
%ifdef WIN64
    sub  rsp, 8
%endif
    call %2
    add  r0, %4-%5-%6*FDEC_STRIDE
    add  r1, %3
    call %2
    add  r0, (%4-%6)*FDEC_STRIDE-%5-%4
    add  r1, %3
    call %2
    add  r0, %4-%5-%6*FDEC_STRIDE
    add  r1, %3
%ifdef WIN64
    add  rsp, 8
    call %2
    RET
%else
    jmp  %2
%endif
%endmacro

%ifdef HIGH_BIT_DEPTH
INIT_MMX
SUB_NxN_DCT  sub8x8_dct_mmx,    sub4x4_dct_mmx.skip_prologue,  64,  8, 0, 0
SUB_NxN_DCT  sub16x16_dct_mmx,  sub8x8_dct_mmx.skip_prologue,  64, 16, 8, 8
%else ; !HIGH_BIT_DEPTH
%ifndef ARCH_X86_64
SUB_NxN_DCT  sub8x8_dct_mmx,    sub4x4_dct_mmx.skip_prologue,  32, 4, 0, 0
ADD_NxN_IDCT add8x8_idct_mmx,   add4x4_idct_mmx.skip_prologue, 32, 4, 0, 0
SUB_NxN_DCT  sub16x16_dct_mmx,  sub8x8_dct_mmx.skip_prologue,  32, 8, 4, 4
ADD_NxN_IDCT add16x16_idct_mmx, add8x8_idct_mmx.skip_prologue, 32, 8, 4, 4

cextern sub8x8_dct8_mmx.skip_prologue
cextern add8x8_idct8_mmx.skip_prologue
SUB_NxN_DCT  sub16x16_dct8_mmx,  sub8x8_dct8_mmx.skip_prologue,  128, 8, 0, 0
ADD_NxN_IDCT add16x16_idct8_mmx, add8x8_idct8_mmx.skip_prologue, 128, 8, 0, 0
%endif

INIT_XMM

cextern sub8x8_dct_sse2.skip_prologue
cextern sub8x8_dct_ssse3.skip_prologue
SUB_NxN_DCT  sub16x16_dct_sse2,  sub8x8_dct_sse2.skip_prologue,  128, 8, 0, 0
SUB_NxN_DCT  sub16x16_dct_ssse3, sub8x8_dct_ssse3.skip_prologue, 128, 8, 0, 0
cextern add8x8_idct_sse2.skip_prologue
ADD_NxN_IDCT add16x16_idct_sse2, add8x8_idct_sse2.skip_prologue, 2*64, 8, 0, 0

cextern sub8x8_dct8_sse2.skip_prologue
cextern add8x8_idct8_sse2.skip_prologue
SUB_NxN_DCT  sub16x16_dct8_sse2,  sub8x8_dct8_sse2.skip_prologue,  128, 8, 0, 0
ADD_NxN_IDCT add16x16_idct8_sse2, add8x8_idct8_sse2.skip_prologue, 128, 8, 0, 0

cextern sub8x8_dct8_ssse3.skip_prologue
SUB_NxN_DCT  sub16x16_dct8_ssse3, sub8x8_dct8_ssse3.skip_prologue, 128, 8, 0, 0
%endif ; HIGH_BIT_DEPTH


;-----------------------------------------------------------------------------
; void add8x8_idct_dc( uint8_t *p_dst, int16_t *dct2x2 )
;-----------------------------------------------------------------------------

%macro ADD_DC 3
    movq      mm4, [%3+FDEC_STRIDE*0]
    movq      mm5, [%3+FDEC_STRIDE*1]
    movq      mm6, [%3+FDEC_STRIDE*2]
    paddusb   mm4, %1
    paddusb   mm5, %1
    paddusb   mm6, %1
    paddusb    %1, [%3+FDEC_STRIDE*3]
    psubusb   mm4, %2
    psubusb   mm5, %2
    psubusb   mm6, %2
    psubusb    %1, %2
    movq      [%3+FDEC_STRIDE*0], mm4
    movq      [%3+FDEC_STRIDE*1], mm5
    movq      [%3+FDEC_STRIDE*2], mm6
    movq      [%3+FDEC_STRIDE*3], %1
%endmacro

cglobal add8x8_idct_dc_mmx, 2,2
    movq      mm0, [r1]
    pxor      mm1, mm1
    add        r0, FDEC_STRIDE*4
    paddw     mm0, [pw_32]
    psraw     mm0, 6
    psubw     mm1, mm0
    packuswb  mm0, mm0
    packuswb  mm1, mm1
    punpcklbw mm0, mm0
    punpcklbw mm1, mm1
    pshufw    mm2, mm0, 0xFA
    pshufw    mm3, mm1, 0xFA
    punpcklbw mm0, mm0
    punpcklbw mm1, mm1
    ADD_DC    mm0, mm1, r0-FDEC_STRIDE*4
    ADD_DC    mm2, mm3, r0
    RET

cglobal add8x8_idct_dc_ssse3, 2,2
    movq      xmm0, [r1]
    pxor      xmm1, xmm1
    add         r0, FDEC_STRIDE*4
    paddw     xmm0, [pw_32]
    psraw     xmm0, 6
    psubw     xmm1, xmm0
    movdqa    xmm5, [pb_idctdc_unpack]
    packuswb  xmm0, xmm0
    packuswb  xmm1, xmm1
    pshufb    xmm0, xmm5
    pshufb    xmm1, xmm5
    movq      xmm2, [r0+FDEC_STRIDE*-4]
    movq      xmm3, [r0+FDEC_STRIDE*-3]
    movq      xmm4, [r0+FDEC_STRIDE*-2]
    movq      xmm5, [r0+FDEC_STRIDE*-1]
    movhps    xmm2, [r0+FDEC_STRIDE* 0]
    movhps    xmm3, [r0+FDEC_STRIDE* 1]
    movhps    xmm4, [r0+FDEC_STRIDE* 2]
    movhps    xmm5, [r0+FDEC_STRIDE* 3]
    paddusb   xmm2, xmm0
    paddusb   xmm3, xmm0
    paddusb   xmm4, xmm0
    paddusb   xmm5, xmm0
    psubusb   xmm2, xmm1
    psubusb   xmm3, xmm1
    psubusb   xmm4, xmm1
    psubusb   xmm5, xmm1
    movq      [r0+FDEC_STRIDE*-4], xmm2
    movq      [r0+FDEC_STRIDE*-3], xmm3
    movq      [r0+FDEC_STRIDE*-2], xmm4
    movq      [r0+FDEC_STRIDE*-1], xmm5
    movhps    [r0+FDEC_STRIDE* 0], xmm2
    movhps    [r0+FDEC_STRIDE* 1], xmm3
    movhps    [r0+FDEC_STRIDE* 2], xmm4
    movhps    [r0+FDEC_STRIDE* 3], xmm5
    RET

cglobal add16x16_idct_dc_mmx, 2,3
    mov       r2, 4
.loop:
    movq      mm0, [r1]
    pxor      mm1, mm1
    paddw     mm0, [pw_32]
    psraw     mm0, 6
    psubw     mm1, mm0
    packuswb  mm0, mm0
    packuswb  mm1, mm1
    punpcklbw mm0, mm0
    punpcklbw mm1, mm1
    pshufw    mm2, mm0, 0xFA
    pshufw    mm3, mm1, 0xFA
    punpcklbw mm0, mm0
    punpcklbw mm1, mm1
    ADD_DC    mm0, mm1, r0
    ADD_DC    mm2, mm3, r0+8
    add       r1, 8
    add       r0, FDEC_STRIDE*4
    dec       r2
    jg .loop
    REP_RET

%macro IDCT_DC_STORE 3
    movdqa    xmm4, [r0+%1+FDEC_STRIDE*0]
    movdqa    xmm5, [r0+%1+FDEC_STRIDE*1]
    movdqa    xmm6, [r0+%1+FDEC_STRIDE*2]
    movdqa    xmm7, [r0+%1+FDEC_STRIDE*3]
    paddusb   xmm4, %2
    paddusb   xmm5, %2
    paddusb   xmm6, %2
    paddusb   xmm7, %2
    psubusb   xmm4, %3
    psubusb   xmm5, %3
    psubusb   xmm6, %3
    psubusb   xmm7, %3
    movdqa    [r0+%1+FDEC_STRIDE*0], xmm4
    movdqa    [r0+%1+FDEC_STRIDE*1], xmm5
    movdqa    [r0+%1+FDEC_STRIDE*2], xmm6
    movdqa    [r0+%1+FDEC_STRIDE*3], xmm7
%endmacro

cglobal add16x16_idct_dc_sse2, 2,2,8
    call .loop
    add       r0, FDEC_STRIDE*4
%ifdef WIN64
    call .loop
    RET
%endif
.loop:
    add       r0, FDEC_STRIDE*4
    movq      xmm0, [r1+0]
    movq      xmm2, [r1+8]
    add       r1, 16
    punpcklwd xmm0, xmm0
    punpcklwd xmm2, xmm2
    pxor      xmm1, xmm1
    pxor      xmm3, xmm3
    paddw     xmm0, [pw_32]
    paddw     xmm2, [pw_32]
    psraw     xmm0, 6
    psraw     xmm2, 6
    psubw     xmm1, xmm0
    psubw     xmm3, xmm2
    packuswb  xmm0, xmm1
    packuswb  xmm2, xmm3
    movdqa    xmm1, xmm0
    movdqa    xmm3, xmm2
    punpcklbw xmm0, xmm0
    punpcklbw xmm2, xmm2
    punpckhbw xmm1, xmm1
    punpckhbw xmm3, xmm3
    IDCT_DC_STORE FDEC_STRIDE*-4, xmm0, xmm1
    IDCT_DC_STORE 0, xmm2, xmm3
    ret

cglobal add16x16_idct_dc_ssse3, 2,2,8
    call .loop
    add       r0, FDEC_STRIDE*4
%ifdef WIN64
    call .loop
    RET
%endif
.loop:
    add       r0, FDEC_STRIDE*4
    movdqa    xmm0, [r1]
    add       r1, 16
    pxor      xmm1, xmm1
    paddw     xmm0, [pw_32]
    psraw     xmm0, 6
    psubw     xmm1, xmm0
    movdqa    xmm5, [ pb_idctdc_unpack]
    movdqa    xmm6, [pb_idctdc_unpack2]
    packuswb  xmm0, xmm0
    packuswb  xmm1, xmm1
    movdqa    xmm2, xmm0
    movdqa    xmm3, xmm1
    pshufb    xmm0, xmm5
    pshufb    xmm2, xmm6
    pshufb    xmm1, xmm5
    pshufb    xmm3, xmm6
    IDCT_DC_STORE FDEC_STRIDE*-4, xmm0, xmm1
    IDCT_DC_STORE 0, xmm2, xmm3
    ret

;-----------------------------------------------------------------------------
; void sub8x8_dct_dc( int16_t dct[2][2], uint8_t *pix1, uint8_t *pix2 )
;-----------------------------------------------------------------------------

%macro DCTDC_2ROW_MMX 3
    movq      %1, [r1+FENC_STRIDE*(0+%3)]
    movq      m1, [r1+FENC_STRIDE*(1+%3)]
    movq      m2, [r2+FDEC_STRIDE*(0+%3)]
    movq      m3, [r2+FDEC_STRIDE*(1+%3)]
    movq      %2, %1
    punpckldq %1, m1
    punpckhdq %2, m1
    movq      m1, m2
    punpckldq m2, m3
    punpckhdq m1, m3
    pxor      m3, m3
    psadbw    %1, m3
    psadbw    %2, m3
    psadbw    m2, m3
    psadbw    m1, m3
    psubw     %1, m2
    psubw     %2, m1
%endmacro

%macro DCT2x2 2 ; reg s1/s0 (!=m1), reg s3/s2
    pshufw    mm1, %1, 10100000b  ;  s1  s1  s0  s0
    pshufw    mm0, %2, 10110001b  ;  s3  __  s2  __
    paddw     mm1, %2             ;  s1 s13  s0 s02
    psubw     mm1, mm0            ; d13 s13 d02 s02
    pshufw    mm0, mm1, 01000100b ; d02 s02 d02 s02
    psrlq     mm1, 32             ;  __  __ d13 s13
    paddw     mm0, mm1            ; d02 s02 d02+d13 s02+s13
    psllq     mm1, 32             ; d13 s13
    psubw     mm0, mm1            ; d02-d13 s02-s13 d02+d13 s02+s13
%endmacro

INIT_MMX
cglobal sub8x8_dct_dc_mmxext, 3,3
    DCTDC_2ROW_MMX m0, m4, 0
    DCTDC_2ROW_MMX m5, m6, 2
    paddw     m0, m5
    paddw     m4, m6
    punpckldq m0, m4
    add       r1, FENC_STRIDE*4
    add       r2, FDEC_STRIDE*4
    DCTDC_2ROW_MMX m7, m4, 0
    DCTDC_2ROW_MMX m5, m6, 2
    paddw     m7, m5
    paddw     m4, m6
    punpckldq m7, m4
    DCT2x2    m0, m7
    movq    [r0], m0
    ret

INIT_XMM
%macro DCTDC_2ROW_SSE2 3
    movq      m0, [r1+FENC_STRIDE*(0+%1)]
    movq      m1, [r1+FENC_STRIDE*(1+%1)]
    movq      m2, [r2+FDEC_STRIDE*(0+%1)]
    movq      m3, [r2+FDEC_STRIDE*(1+%1)]
    punpckldq m0, m1
    punpckldq m2, m3
    psadbw    m0, m7
    psadbw    m2, m7
%if %2
    paddw     %3, m0
    paddw     m6, m2
%else
    SWAP      %3, m0
    SWAP      m6, m2
%endif
%endmacro

cglobal sub8x8_dct_dc_sse2, 3,3,8
    pxor     m7, m7
    DCTDC_2ROW_SSE2 0, 0, m4
    DCTDC_2ROW_SSE2 2, 1, m4
    add      r1, FENC_STRIDE*4
    add      r2, FDEC_STRIDE*4
    psubd    m4, m6
    DCTDC_2ROW_SSE2 0, 0, m5
    DCTDC_2ROW_SSE2 2, 1, m5
    psubd    m5, m6
    packssdw m4, m5
    movhlps  m5, m4
    movdq2q mm0, m4
    movdq2q mm7, m5
    DCT2x2  mm0, mm7
    movq   [r0], mm0
    RET

;-----------------------------------------------------------------------------
; void zigzag_scan_8x8_frame( int16_t level[64], int16_t dct[8][8] )
;-----------------------------------------------------------------------------
%macro SCAN_8x8 1
cglobal zigzag_scan_8x8_frame_%1, 2,2,8
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
; void zigzag_scan_8x8_frame( int16_t level[64], int16_t dct[8][8] )
;-----------------------------------------------------------------------------
cglobal zigzag_scan_8x8_frame_mmxext, 2,2
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
; void zigzag_scan_4x4_frame( int16_t level[16], int16_t dct[4][4] )
;-----------------------------------------------------------------------------
cglobal zigzag_scan_4x4_frame_mmx, 2,2
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
; void zigzag_scan_4x4_frame( int16_t level[16], int16_t dct[4][4] )
;-----------------------------------------------------------------------------
cglobal zigzag_scan_4x4_frame_ssse3, 2,2
    movdqa    xmm1, [r1+16]
    movdqa    xmm0, [r1]
    pshufb    xmm1, [pb_scan4frameb]
    pshufb    xmm0, [pb_scan4framea]
    movdqa    xmm2, xmm1
    psrldq    xmm1, 6
    palignr   xmm2, xmm0, 6
    pslldq    xmm0, 10
    palignr   xmm1, xmm0, 10
    movdqa    [r0], xmm2
    movdqa [r0+16], xmm1
    RET

;-----------------------------------------------------------------------------
; void zigzag_scan_4x4_field( int16_t level[16], int16_t dct[4][4] )
;-----------------------------------------------------------------------------
; sse2 is only 1 cycle faster, and ssse3/pshufb is slower on core2
cglobal zigzag_scan_4x4_field_mmxext, 2,3
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
; void zigzag_scan_8x8_field( int16_t level[64], int16_t dct[8][8] )
;-----------------------------------------------------------------------------

; Output order:
;  0  1  2  8  9  3  4 10
; 16 11  5  6  7 12 17 24
; 18 13 14 15 19 25 32 26
; 20 21 22 23 27 33 40 34
; 28 29 30 31 35 41 48 42
; 36 37 38 39 43 49 50 44
; 45 46 47 51 56 57 52 53
; 54 55 58 59 60 61 62 63

cglobal zigzag_scan_8x8_field_mmxext, 2,3
    movq       mm0, [r1+2*0]        ; 03 02 01 00
    movq       mm1, [r1+2*4]        ; 07 06 05 04
    movq       mm2, [r1+2*8]        ; 11 10 09 08
    pshufw     mm3, mm0, 011111111b ; 03 03 03 03
    movd        r2, mm2             ; 09 08
    pshufw     mm2, mm2, 000111001b ; 08 11 10 09
    punpcklwd  mm3, mm1             ; 05 03 04 03
    pinsrw     mm0, r2, 3           ; 08 02 01 00
    movq       mm4, mm2
    punpcklwd  mm2, mm3             ; 04 10 03 09
    pshufw     mm2, mm2, 010110100b ; 10 04 03 09
    movq  [r0+2*0], mm0             ; 08 02 01 00
    movq  [r0+2*4], mm2             ; 10 04 03 09
    movq       mm3, [r1+2*12]       ; 15 14 13 12
    movq       mm5, [r1+2*16]       ; 19 18 17 16
    punpckldq  mm6, mm5             ; 17 16 XX XX
    psrlq      mm1, 16              ; XX 07 06 05
    punpckhwd  mm6, mm4             ; 08 17 11 16
    punpckldq  mm6, mm1             ; 06 05 11 16
    movq  [r0+2*8], mm6             ; 06 05 11 16
    psrlq      mm1, 16              ; XX XX 07 06
    punpcklwd  mm1, mm5             ; 17 07 16 06
    movq       mm0, [r1+2*20]       ; 23 22 21 20
    movq       mm2, [r1+2*24]       ; 27 26 25 24
    movq       mm6, mm3
    punpckhdq  mm1, mm1             ; 17 07 17 07
    punpcklwd  mm6, mm2             ; 25 13 24 12
    pextrw      r2, mm5, 2
    movq [r0+2*24], mm0             ; 23 22 21 20
    punpcklwd  mm1, mm6             ; 24 17 12 07
    movq [r0+2*12], mm1
    pinsrw     mm3, r2, 0           ; 15 14 13 18
    movq [r0+2*16], mm3             ; 15 14 13 18
    movq       mm7, [r1+2*28]
    movq       mm0, [r1+2*32]       ; 35 34 33 32
    psrlq      mm5, 48              ; XX XX XX 19
    pshufw     mm1, mm2, 011111001b ; 27 27 26 25
    punpcklwd  mm5, mm0             ; 33 XX 32 19
    psrlq      mm2, 48              ; XX XX XX 27
    punpcklwd  mm5, mm1             ; 26 32 25 19
    movq [r0+2*32], mm7
    movq [r0+2*20], mm5             ; 26 32 25 19
    movq       mm7, [r1+2*36]
    movq       mm1, [r1+2*40]       ; 43 42 41 40
    pshufw     mm3, mm0, 011111001b ; 35 35 34 33
    punpcklwd  mm2, mm1             ; 41 XX 40 27
    movq [r0+2*40], mm7
    punpcklwd  mm2, mm3             ; 34 40 33 27
    movq [r0+2*28], mm2
    movq       mm7, [r1+2*44]       ; 47 46 45 44
    movq       mm2, [r1+2*48]       ; 51 50 49 48
    psrlq      mm0, 48              ; XX XX XX 35
    punpcklwd  mm0, mm2             ; 49 XX 48 35
    pshufw     mm3, mm1, 011111001b ; 43 43 42 41
    punpcklwd  mm0, mm3             ; 42 48 41 35
    movq [r0+2*36], mm0
    pextrw      r2, mm2, 3          ; 51
    psrlq      mm1, 48              ; XX XX XX 43
    punpcklwd  mm1, mm7             ; 45 XX 44 43
    psrlq      mm2, 16              ; XX 51 50 49
    punpcklwd  mm1, mm2             ; 50 44 49 43
    pshufw     mm1, mm1, 010110100b ; 44 50 49 43
    movq [r0+2*44], mm1
    psrlq      mm7, 16              ; XX 47 46 45
    pinsrw     mm7, r2, 3           ; 51 47 46 45
    movq [r0+2*48], mm7
    movq       mm0, [r1+2*56]       ; 59 58 57 56
    movq       mm1, [r1+2*52]       ; 55 54 53 52
    movq       mm2, mm0
    movq       mm7, [r1+2*60]
    punpckldq  mm2, mm1             ; 53 52 57 56
    punpckhdq  mm1, mm0             ; 59 58 55 54
    movq [r0+2*52], mm2
    movq [r0+2*56], mm1
    movq [r0+2*60], mm7
    RET

;-----------------------------------------------------------------------------
; void zigzag_sub_4x4_frame( int16_t level[16], const uint8_t *src, uint8_t *dst )
;-----------------------------------------------------------------------------
%macro ZIGZAG_SUB_4x4 2
%ifidn %1, ac
cglobal zigzag_sub_4x4%1_%2_ssse3, 4,4,8
%else
cglobal zigzag_sub_4x4%1_%2_ssse3, 3,3,8
%endif
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
%ifidn %2, frame
    movdqa    xmm7, [pb_sub4frame]
%else
    movdqa    xmm7, [pb_sub4field]
%endif
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
%ifidn %1, ac
    movd       r2d, xmm0
    pand      xmm0, [pb_subacmask]
%endif
    movdqa    [r0], xmm0
    pxor      xmm2, xmm2
    movdqa [r0+16], xmm1
    por       xmm0, xmm1
    pcmpeqb   xmm0, xmm2
    pmovmskb   eax, xmm0
%ifidn %1, ac
    mov       [r3], r2w
%endif
    sub        eax, 0xffff
    shr        eax, 31
    RET
%endmacro

ZIGZAG_SUB_4x4   , frame
ZIGZAG_SUB_4x4 ac, frame
ZIGZAG_SUB_4x4   , field
ZIGZAG_SUB_4x4 ac, field

;-----------------------------------------------------------------------------
; void zigzag_interleave_8x8_cavlc( int16_t *dst, int16_t *src, uint8_t *nnz )
;-----------------------------------------------------------------------------

%macro INTERLEAVE 1
    movq   m0, [r1+%1*4+ 0]
    movq   m1, [r1+%1*4+ 8]
    movq   m2, [r1+%1*4+16]
    movq   m3, [r1+%1*4+24]
    TRANSPOSE4x4W 0,1,2,3,4
    movq   [r0+%1+ 0], m0
    movq   [r0+%1+32], m1
    movq   [r0+%1+64], m2
    movq   [r0+%1+96], m3
%if %1
    packsswb m0, m1
    por    m6, m2
    por    m7, m3
    por    m5, m0
%else
    packsswb m0, m1
    SWAP   m5, m0
    SWAP   m6, m2
    SWAP   m7, m3
%endif
%endmacro

INIT_MMX
cglobal zigzag_interleave_8x8_cavlc_mmx, 3,3
    INTERLEAVE  0
    INTERLEAVE  8
    INTERLEAVE 16
    INTERLEAVE 24
    packsswb m6, m7
    packsswb m5, m6
    packsswb m5, m5
    pxor     m0, m0
    pcmpeqb  m5, m0
    paddb    m5, [pb_1]
    movd    r0d, m5
    mov  [r2+0], r0w
    shr     r0d, 16
    mov  [r2+8], r0w
    RET

%macro INTERLEAVE_XMM 1
    mova   m0, [r1+%1*4+ 0]
    mova   m1, [r1+%1*4+16]
    mova   m4, [r1+%1*4+32]
    mova   m5, [r1+%1*4+48]
    SBUTTERFLY wd, 0, 1, 6
    SBUTTERFLY wd, 4, 5, 7
    SBUTTERFLY wd, 0, 1, 6
    SBUTTERFLY wd, 4, 5, 7
    movq   [r0+%1+  0], m0
    movhps [r0+%1+ 32], m0
    movq   [r0+%1+ 64], m1
    movhps [r0+%1+ 96], m1
    movq   [r0+%1+  8], m4
    movhps [r0+%1+ 40], m4
    movq   [r0+%1+ 72], m5
    movhps [r0+%1+104], m5
%if %1
    por    m2, m0
    por    m3, m1
    por    m2, m4
    por    m3, m5
%else
    SWAP 0,2
    SWAP 3,1
    por    m2, m4
    por    m3, m5
%endif
%endmacro

INIT_XMM
cglobal zigzag_interleave_8x8_cavlc_sse2, 3,3,8
    INTERLEAVE_XMM  0
    INTERLEAVE_XMM 16
    packsswb m2, m3
    pxor     m5, m5
    packsswb m2, m2
    packsswb m2, m2
    pcmpeqb  m5, m2
    paddb    m5, [pb_1]
    movd    r0d, m5
    mov  [r2+0], r0w
    shr     r0d, 16
    mov  [r2+8], r0w
    RET


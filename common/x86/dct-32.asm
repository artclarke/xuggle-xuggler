;*****************************************************************************
;* dct-32.asm: h264 encoder library
;*****************************************************************************
;* Copyright (C) 2003-2008 x264 project
;*
;* Authors: Laurent Aimar <fenrir@via.ecp.fr> (initial version)
;*          Min Chen <chenm001.163.com> (converted to nasm)
;*          Christian Heine <sennindemokrit@gmx.net> (dct8/idct8 functions)
;*          Loren Merritt <lorenm@u.washington.edu> (misc)
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

pw_32: times 8 dw 32

SECTION .text

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

; input 2x8 unsigned bytes (%5,%6), zero (%7) output: difference (%1,%2)
%macro LOAD_DIFF_8P 7
    movq       %1, %5
    movq       %2, %1
    punpcklbw  %1, %7
    punpckhbw  %2, %7
    movq       %3, %6
    movq       %4, %3
    punpcklbw  %3, %7
    punpckhbw  %4, %7
    psubw      %1, %3
    psubw      %2, %4
%endmacro
 
%macro LOADSUMSUB 4     ; returns %1=%3+%4, %2=%3-%4
    movq       %2, %3
    movq       %1, %4
    SUMSUB_BA  %1, %2
%endmacro

%macro STORE_DIFF_8P 4
    psraw      %1, 6
    movq       %3, %2
    punpcklbw  %3, %4
    paddsw     %1, %3
    packuswb   %1, %1
    movq       %2, %1
%endmacro


;-----------------------------------------------------------------------------
; void x264_pixel_sub_8x8_mmx( int16_t *diff, uint8_t *pix1, uint8_t *pix2 );
;-----------------------------------------------------------------------------
ALIGN 16
x264_pixel_sub_8x8_mmx:
    pxor mm7, mm7
    %assign i 0
    %rep  8
    LOAD_DIFF_8P mm0, mm1, mm2, mm3, [r1], [r2], mm7
    movq  [r0+i], mm0
    movq  [r0+i+8], mm1
    add   r1, FENC_STRIDE
    add   r2, FDEC_STRIDE
    %assign i i+16
    %endrep
    ret

;-----------------------------------------------------------------------------
; void x264_ydct8_mmx( int16_t dest[8][8] );
;-----------------------------------------------------------------------------
ALIGN 16
x264_ydct8_mmx:
    ;-------------------------------------------------------------------------
    ; vertical dct ( compute 4 columns at a time -> 2 loops )
    ;-------------------------------------------------------------------------
    %assign i 0
    %rep 2
    
    LOADSUMSUB  mm2, mm3, [r0+i+0*16], [r0+i+7*16] ; mm2 = s07, mm3 = d07
    LOADSUMSUB  mm1, mm5, [r0+i+1*16], [r0+i+6*16] ; mm1 = s16, mm5 = d16
    LOADSUMSUB  mm0, mm6, [r0+i+2*16], [r0+i+5*16] ; mm0 = s25, mm6 = d25
    LOADSUMSUB  mm4, mm7, [r0+i+3*16], [r0+i+4*16] ; mm4 = s34, mm7 = d34

    SUMSUB_BA   mm4, mm2        ; mm4 = a0, mm2 = a2
    SUMSUB_BA   mm0, mm1        ; mm0 = a1, mm1 = a3
    SUMSUB_BA   mm0, mm4        ; mm0 = dst0, mm1 = dst4

    movq    [r0+i+0*16], mm0
    movq    [r0+i+4*16], mm4

    movq    mm0, mm1         ; a3
    psraw   mm0, 1           ; a3>>1
    paddw   mm0, mm2         ; a2 + (a3>>1)
    psraw   mm2, 1           ; a2>>1
    psubw   mm2, mm1         ; (a2>>1) - a3

    movq    [r0+i+2*16], mm0
    movq    [r0+i+6*16], mm2

    movq    mm0, mm6
    psraw   mm0, 1
    paddw   mm0, mm6         ; d25+(d25>>1)
    movq    mm1, mm3
    psubw   mm1, mm7         ; a5 = d07-d34-(d25+(d25>>1))
    psubw   mm1, mm0

    movq    mm0, mm5
    psraw   mm0, 1
    paddw   mm0, mm5         ; d16+(d16>>1)
    movq    mm2, mm3
    paddw   mm2, mm7         ; a6 = d07+d34-(d16+(d16>>1))
    psubw   mm2, mm0

    movq    mm0, mm3
    psraw   mm0, 1
    paddw   mm0, mm3         ; d07+(d07>>1)
    paddw   mm0, mm5
    paddw   mm0, mm6         ; a4 = d16+d25+(d07+(d07>>1))

    movq    mm3, mm7
    psraw   mm3, 1
    paddw   mm3, mm7         ; d34+(d34>>1)
    paddw   mm3, mm5
    psubw   mm3, mm6         ; a7 = d16-d25+(d34+(d34>>1))

    movq    mm7, mm3
    psraw   mm7, 2
    paddw   mm7, mm0         ; a4 + (a7>>2)

    movq    mm6, mm2
    psraw   mm6, 2
    paddw   mm6, mm1         ; a5 + (a6>>2)

    psraw   mm0, 2
    psraw   mm1, 2
    psubw   mm0, mm3         ; (a4>>2) - a7
    psubw   mm2, mm1         ; a6 - (a5>>2)

    movq    [r0+i+1*16], mm7
    movq    [r0+i+3*16], mm6
    movq    [r0+i+5*16], mm2
    movq    [r0+i+7*16], mm0

    %assign i i+8
    %endrep
    ret

;-----------------------------------------------------------------------------
; void x264_yidct8_mmx( int16_t dest[8][8] );
;-----------------------------------------------------------------------------
ALIGN 16
x264_yidct8_mmx:
    ;-------------------------------------------------------------------------
    ; vertical idct ( compute 4 columns at a time -> 2 loops )
    ;-------------------------------------------------------------------------
    %assign i 0
    %rep 2

    movq        mm1, [r0+i+1*16]        ; mm1 = d1
    movq        mm3, [r0+i+3*16]        ; mm3 = d3
    movq        mm5, [r0+i+5*16]        ; mm5 = d5
    movq        mm7, [r0+i+7*16]        ; mm7 = d7

    movq        mm4, mm7
    psraw       mm4, 1
    movq        mm0, mm5
    psubw       mm0, mm7
    psubw       mm0, mm4
    psubw       mm0, mm3                ; mm0 = e1

    movq        mm6, mm3
    psraw       mm6, 1
    movq        mm2, mm7
    psubw       mm2, mm6
    psubw       mm2, mm3
    paddw       mm2, mm1                ; mm2 = e3

    movq        mm4, mm5
    psraw       mm4, 1
    paddw       mm4, mm5
    paddw       mm4, mm7
    psubw       mm4, mm1                ; mm4 = e5

    movq        mm6, mm1
    psraw       mm6, 1
    paddw       mm6, mm1
    paddw       mm6, mm5
    paddw       mm6, mm3                ; mm6 = e7

    movq        mm1, mm0
    movq        mm3, mm4
    movq        mm5, mm2
    movq        mm7, mm6
    psraw       mm6, 2
    psraw       mm3, 2
    psraw       mm5, 2
    psraw       mm0, 2
    paddw       mm1, mm6                ; mm1 = f1
    paddw       mm3, mm2                ; mm3 = f3
    psubw       mm5, mm4                ; mm5 = f5
    psubw       mm7, mm0                ; mm7 = f7

    movq        mm2, [r0+i+2*16]        ; mm2 = d2
    movq        mm6, [r0+i+6*16]        ; mm6 = d6
    movq        mm4, mm2
    movq        mm0, mm6
    psraw       mm4, 1
    psraw       mm6, 1
    psubw       mm4, mm0                ; mm4 = a4
    paddw       mm6, mm2                ; mm6 = a6

    movq        mm2, [r0+i+0*16]        ; mm2 = d0
    movq        mm0, [r0+i+4*16]        ; mm0 = d4
    SUMSUB_BA   mm0, mm2                ; mm0 = a0, mm2 = a2

    SUMSUB_BADC mm6, mm0, mm4, mm2      ; mm6 = f0, mm0 = f6
                                        ; mm4 = f2, mm2 = f4

    SUMSUB_BADC mm7, mm6, mm5, mm4      ; mm7 = g0, mm6 = g7
                                        ; mm5 = g1, mm4 = g6
    SUMSUB_BADC mm3, mm2, mm1, mm0      ; mm3 = g2, mm2 = g5
                                        ; mm1 = g3, mm0 = g4

    movq        [r0+i+0*16], mm7
    movq        [r0+i+1*16], mm5
    movq        [r0+i+2*16], mm3
    movq        [r0+i+3*16], mm1
    movq        [r0+i+4*16], mm0
    movq        [r0+i+5*16], mm2
    movq        [r0+i+6*16], mm4
    movq        [r0+i+7*16], mm6

    %assign i i+8
    %endrep
    ret

;-----------------------------------------------------------------------------
; void x264_pixel_add_8x8_mmx( uint8_t *dst, int16_t src[8][8] );
;-----------------------------------------------------------------------------
ALIGN 16
x264_pixel_add_8x8_mmx:
    pxor mm7, mm7
    %assign i 0
    %rep 8
    movq        mm0, [r0]
    movq        mm2, [r1+i]
    movq        mm3, [r1+i+8]
    movq        mm1, mm0
    psraw       mm2, 6
    psraw       mm3, 6
    punpcklbw   mm0, mm7
    punpckhbw   mm1, mm7
    paddw       mm0, mm2
    paddw       mm1, mm3
    packuswb    mm0, mm1
    movq       [r0], mm0
    add          r0, FDEC_STRIDE
    %assign i i+16
    %endrep
    ret

;-----------------------------------------------------------------------------
; void x264_transpose_8x8_mmx( int16_t src[8][8] );
;-----------------------------------------------------------------------------
ALIGN 16
x264_transpose_8x8_mmx:
    movq  m0, [r0    ]
    movq  m1, [r0+ 16]
    movq  m2, [r0+ 32]
    movq  m3, [r0+ 48]
    TRANSPOSE4x4W 0,1,2,3,4
    movq  [r0    ], m0
    movq  [r0+ 16], m1
    movq  [r0+ 32], m2
    movq  [r0+ 48], m3

    movq  m0, [r0+ 72]
    movq  m1, [r0+ 88]
    movq  m2, [r0+104]
    movq  m3, [r0+120]
    TRANSPOSE4x4W 0,1,2,3,4
    movq  [r0+ 72], m0
    movq  [r0+ 88], m1
    movq  [r0+104], m2
    movq  [r0+120], m3

    movq  m0, [r0+  8]
    movq  m1, [r0+ 24]
    movq  m2, [r0+ 40]
    movq  m3, [r0+ 56]
    TRANSPOSE4x4W 0,1,2,3,4
    movq  m4, [r0+ 64]
    movq  m5, [r0+ 80]
    movq  m6, [r0+ 96]
    movq  m7, [r0+112]

    movq  [r0+ 64], m0
    movq  [r0+ 80], m1
    movq  [r0+ 96], m2
    movq  [r0+112], m3
    TRANSPOSE4x4W 4,5,6,7,0
    movq  [r0+  8], m4
    movq  [r0+ 24], m5
    movq  [r0+ 40], m6
    movq  [r0+ 56], m7
    ret

;-----------------------------------------------------------------------------
; void x264_sub8x8_dct8_mmx( int16_t dct[8][8], uint8_t *pix1, uint8_t *pix2 )
;-----------------------------------------------------------------------------
cglobal x264_sub8x8_dct8_mmx, 3,3
    call x264_pixel_sub_8x8_mmx
    call x264_ydct8_mmx
    call x264_transpose_8x8_mmx
    jmp  x264_ydct8_mmx

;-----------------------------------------------------------------------------
; void x264_add8x8_idct8_mmx( uint8_t *dst, int16_t dct[8][8] )
;-----------------------------------------------------------------------------
cglobal x264_add8x8_idct8_mmx, 0,1
    mov  r0, r1m
    add  word [r0], 32
    call x264_yidct8_mmx
    call x264_transpose_8x8_mmx
    call x264_yidct8_mmx
    mov  r1, r0
    mov  r0, r0m
    jmp  x264_pixel_add_8x8_mmx

INIT_XMM

%macro IDCT8_1D 8
    movdqa    m%1, m%3
    movdqa    m%5, m%7
    psraw     m%3, 1
    psraw     m%7, 1
    psubw     m%3, m%5
    paddw     m%7, m%1
    movdqa    m%5, m%2
    psraw     m%5, 1
    paddw     m%5, m%2
    paddw     m%5, m%4
    paddw     m%5, m%6
    movdqa    m%1, m%6
    psraw     m%1, 1
    paddw     m%1, m%6
    paddw     m%1, m%8
    psubw     m%1, m%2
    psubw     m%2, m%4
    psubw     m%6, m%4
    paddw     m%2, m%8
    psubw     m%6, m%8
    psraw     m%4, 1
    psraw     m%8, 1
    psubw     m%2, m%4
    psubw     m%6, m%8
    movdqa    m%4, m%5
    movdqa    m%8, m%1
    psraw     m%4, 2
    psraw     m%8, 2
    paddw     m%4, m%6
    paddw     m%8, m%2
    psraw     m%6, 2
    psraw     m%2, 2
    psubw     m%5, m%6
    psubw     m%2, m%1
    movdqa    m%1, [r1+0x00]
    movdqa    m%6, [r1+0x40]
    SUMSUB_BA m%6, m%1
    SUMSUB_BA m%7, m%6
    SUMSUB_BA m%3, m%1
    SUMSUB_BA m%5, m%7
    SUMSUB_BA m%2, m%3
    SUMSUB_BA m%8, m%1
    SUMSUB_BA m%4, m%6
    SWAP %1, %5, %6
    SWAP %3, %8, %7
%endmacro

; in: m0..m7
; out: all except m4, which is in [%9+0x40]
%macro TRANSPOSE8x8W 9
    movdqa [%9], m%8
    SBUTTERFLY wd, %1, %2, %8
    movdqa [%9+16], m%2
    movdqa m%8, [%9]
    SBUTTERFLY wd, %3, %4, %2
    SBUTTERFLY wd, %5, %6, %2
    SBUTTERFLY wd, %7, %8, %2
    SBUTTERFLY dq, %1, %3, %2
    movdqa [%9], m%3
    movdqa m%2, [16+%9]
    SBUTTERFLY dq, %2, %4, %3
    SBUTTERFLY dq, %5, %7, %3
    SBUTTERFLY dq, %6, %8, %3
    SBUTTERFLY qdq, %1, %5, %3
    SBUTTERFLY qdq, %2, %6, %3
    movdqa [%9+0x40], m%2
    movdqa m%3, [%9]
    SBUTTERFLY qdq, %3, %7, %2
    SBUTTERFLY qdq, %4, %8, %2
    SWAP %2, %5
    SWAP %4, %7
%endmacro

;-----------------------------------------------------------------------------
; void x264_add8x8_idct8_sse2( uint8_t *p_dst, int16_t dct[8][8] )
;-----------------------------------------------------------------------------
cglobal x264_add8x8_idct8_sse2, 2,2
    movdqa     m1, [r1+0x10]
    movdqa     m2, [r1+0x20]
    movdqa     m3, [r1+0x30]
    movdqa     m5, [r1+0x50]
    movdqa     m6, [r1+0x60]
    movdqa     m7, [r1+0x70]
    IDCT8_1D   0,1,2,3,4,5,6,7
    TRANSPOSE8x8W 0,1,2,3,4,5,6,7,r1
    picgetgot  edx
    paddw      m0, [pw_32 GLOBAL]
    movdqa     [r1+0x00], m0
;   movdqa     [r1+0x40], m4  ; still there from transpose
    IDCT8_1D   0,1,2,3,4,5,6,7
    movdqa     [r1+0x60], m6
    movdqa     [r1+0x70], m7
    pxor       m7, m7
    STORE_DIFF_8P m0, [r0+FDEC_STRIDE*0], m6, m7
    STORE_DIFF_8P m1, [r0+FDEC_STRIDE*1], m6, m7
    STORE_DIFF_8P m2, [r0+FDEC_STRIDE*2], m6, m7
    STORE_DIFF_8P m3, [r0+FDEC_STRIDE*3], m6, m7
    STORE_DIFF_8P m4, [r0+FDEC_STRIDE*4], m6, m7
    STORE_DIFF_8P m5, [r0+FDEC_STRIDE*5], m6, m7
    movdqa     m0, [r1+0x60]
    movdqa     m1, [r1+0x70]
    STORE_DIFF_8P m0, [r0+FDEC_STRIDE*6], m6, m7
    STORE_DIFF_8P m1, [r0+FDEC_STRIDE*7], m6, m7
    ret

;-----------------------------------------------------------------------------
; void x264_sub8x8_dct_mmx( int16_t dct[4][4][4], uint8_t *pix1, uint8_t *pix2 )
;-----------------------------------------------------------------------------
%macro SUB_NxN_DCT 4
cglobal %1, 3,3
    add  r2, %4
    add  r1, %4
    add  r0, %3
    push r2
    push r1
    push r0
    call %2
    add  dword [esp+0], %3
    add  dword [esp+4], %4*FENC_STRIDE-%4
    add  dword [esp+8], %4*FDEC_STRIDE-%4
    call %2
    add  dword [esp+0], %3
    add  dword [esp+4], %4
    add  dword [esp+8], %4
    call %2
    add  esp, 12
    jmp  %2
%endmacro

;-----------------------------------------------------------------------------
; void x264_add8x8_idct_mmx( uint8_t *pix, int16_t dct[4][4][4] )
;-----------------------------------------------------------------------------
%macro ADD_NxN_IDCT 4
cglobal %1, 2,2
    add  r1, %3
    add  r0, %4
    push r1
    push r0
    call %2
    add  dword [esp+0], %4*FDEC_STRIDE-%4
    add  dword [esp+4], %3
    call %2
    add  dword [esp+0], %4
    add  dword [esp+4], %3
    call %2
    add  esp, 8
    jmp  %2
%endmacro

SUB_NxN_DCT  x264_sub16x16_dct8_mmx,  x264_sub8x8_dct8_mmx,  128, 8
ADD_NxN_IDCT x264_add16x16_idct8_mmx, x264_add8x8_idct8_mmx, 128, 8

ADD_NxN_IDCT x264_add16x16_idct8_sse2, x264_add8x8_idct8_sse2, 128, 8


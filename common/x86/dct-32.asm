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

%macro SBUTTERFLY 5
    mov%1     %5, %3
    punpckl%2 %3, %4
    punpckh%2 %5, %4
%endmacro

; input ABCD output ADTC
%macro TRANSPOSE4x4W 5
    SBUTTERFLY q, wd, %1, %2, %5
    SBUTTERFLY q, wd, %3, %4, %2
    SBUTTERFLY q, dq, %1, %3, %4
    SBUTTERFLY q, dq, %5, %2, %3
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
    movq  mm0, [r0    ]
    movq  mm1, [r0+ 16]
    movq  mm2, [r0+ 32]
    movq  mm3, [r0+ 48]
    TRANSPOSE4x4W  mm0, mm1, mm2, mm3, mm4
    movq  [r0    ], mm0
    movq  [r0+ 16], mm3
    movq  [r0+ 32], mm4
    movq  [r0+ 48], mm2

    movq  mm0, [r0+ 72]
    movq  mm1, [r0+ 88]
    movq  mm2, [r0+104]
    movq  mm3, [r0+120]
    TRANSPOSE4x4W  mm0, mm1, mm2, mm3, mm4
    movq  [r0+ 72], mm0
    movq  [r0+ 88], mm3
    movq  [r0+104], mm4
    movq  [r0+120], mm2

    movq  mm0, [r0+  8]
    movq  mm1, [r0+ 24]
    movq  mm2, [r0+ 40]
    movq  mm3, [r0+ 56]
    TRANSPOSE4x4W  mm0, mm1, mm2, mm3, mm4
    movq  mm1, [r0+ 64]
    movq  mm5, [r0+ 80]
    movq  mm6, [r0+ 96]
    movq  mm7, [r0+112]

    movq  [r0+ 64], mm0
    movq  [r0+ 80], mm3
    movq  [r0+ 96], mm4
    movq  [r0+112], mm2
    TRANSPOSE4x4W  mm1, mm5, mm6, mm7, mm4
    movq  [r0+  8], mm1
    movq  [r0+ 24], mm7
    movq  [r0+ 40], mm4
    movq  [r0+ 56], mm6
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

%macro IDCT8_1D 8
    movdqa     %1, %3
    movdqa     %5, %7
    psraw      %3, 1
    psraw      %7, 1
    psubw      %3, %5
    paddw      %7, %1
    movdqa     %5, %2
    psraw      %5, 1
    paddw      %5, %2
    paddw      %5, %4
    paddw      %5, %6
    movdqa     %1, %6
    psraw      %1, 1
    paddw      %1, %6
    paddw      %1, %8
    psubw      %1, %2
    psubw      %2, %4
    psubw      %6, %4
    paddw      %2, %8
    psubw      %6, %8
    psraw      %4, 1
    psraw      %8, 1
    psubw      %2, %4
    psubw      %6, %8
    movdqa     %4, %5
    movdqa     %8, %1
    psraw      %4, 2
    psraw      %8, 2
    paddw      %4, %6
    paddw      %8, %2
    psraw      %6, 2
    psraw      %2, 2
    psubw      %5, %6
    psubw      %2, %1
    movdqa     %1, [eax+0x00]
    movdqa     %6, [eax+0x40]
    SUMSUB_BA  %6, %1
    SUMSUB_BA  %7, %6
    SUMSUB_BA  %3, %1
    SUMSUB_BA  %5, %7
    SUMSUB_BA  %2, %3
    SUMSUB_BA  %8, %1
    SUMSUB_BA  %4, %6
%endmacro

%macro TRANSPOSE8 9
    movdqa [%9], %8
    SBUTTERFLY dqa, wd, %1, %2, %8
    movdqa [%9+16], %8
    movdqa %8, [%9]
    SBUTTERFLY dqa, wd, %3, %4, %2
    SBUTTERFLY dqa, wd, %5, %6, %4
    SBUTTERFLY dqa, wd, %7, %8, %6
    SBUTTERFLY dqa, dq, %1, %3, %8
    movdqa [%9], %8
    movdqa %8, [16+%9]
    SBUTTERFLY dqa, dq, %8, %2, %3
    SBUTTERFLY dqa, dq, %5, %7, %2
    SBUTTERFLY dqa, dq, %4, %6, %7
    SBUTTERFLY dqa, qdq, %1, %5, %6
    SBUTTERFLY dqa, qdq, %8, %4, %5
    movdqa [%9+16], %8
    movdqa %8, [%9]
    SBUTTERFLY dqa, qdq, %8, %2, %4
    SBUTTERFLY dqa, qdq, %3, %7, %2
    movdqa %7, [%9+16]
%endmacro

;-----------------------------------------------------------------------------
; void x264_add8x8_idct8_sse2( uint8_t *p_dst, int16_t dct[8][8] )
;-----------------------------------------------------------------------------
cglobal x264_add8x8_idct8_sse2
    mov ecx, [esp+4]
    mov eax, [esp+8]
    movdqa     xmm1, [eax+0x10]
    movdqa     xmm2, [eax+0x20]
    movdqa     xmm3, [eax+0x30]
    movdqa     xmm5, [eax+0x50]
    movdqa     xmm6, [eax+0x60]
    movdqa     xmm7, [eax+0x70]
    IDCT8_1D   xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7
    TRANSPOSE8 xmm4, xmm1, xmm7, xmm3, xmm5, xmm0, xmm2, xmm6, eax
    picgetgot  edx
    paddw      xmm4, [pw_32 GLOBAL]
    movdqa     [eax+0x00], xmm4
    movdqa     [eax+0x40], xmm2
    IDCT8_1D   xmm4, xmm0, xmm6, xmm3, xmm2, xmm5, xmm7, xmm1
    movdqa     [eax+0x60], xmm6
    movdqa     [eax+0x70], xmm7
    pxor       xmm7, xmm7
    STORE_DIFF_8P xmm2, [ecx+FDEC_STRIDE*0], xmm6, xmm7
    STORE_DIFF_8P xmm0, [ecx+FDEC_STRIDE*1], xmm6, xmm7
    STORE_DIFF_8P xmm1, [ecx+FDEC_STRIDE*2], xmm6, xmm7
    STORE_DIFF_8P xmm3, [ecx+FDEC_STRIDE*3], xmm6, xmm7
    STORE_DIFF_8P xmm5, [ecx+FDEC_STRIDE*4], xmm6, xmm7
    STORE_DIFF_8P xmm4, [ecx+FDEC_STRIDE*5], xmm6, xmm7
    movdqa     xmm0, [eax+0x60]
    movdqa     xmm1, [eax+0x70]
    STORE_DIFF_8P xmm0, [ecx+FDEC_STRIDE*6], xmm6, xmm7
    STORE_DIFF_8P xmm1, [ecx+FDEC_STRIDE*7], xmm6, xmm7
    ret

;-----------------------------------------------------------------------------
; void x264_sub8x8_dct_mmx( int16_t dct[4][4][4], uint8_t *pix1, uint8_t *pix2 )
;-----------------------------------------------------------------------------
%macro SUB_NxN_DCT 4
cglobal %1
    mov  edx, [esp+12]
    mov  ecx, [esp+ 8]
    mov  eax, [esp+ 4]
    add  edx, %4
    add  ecx, %4
    add  eax, %3
    push edx
    push ecx
    push eax
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
cglobal %1
    mov  ecx, [esp+8]
    mov  eax, [esp+4]
    add  ecx, %3
    add  eax, %4
    push ecx
    push eax
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


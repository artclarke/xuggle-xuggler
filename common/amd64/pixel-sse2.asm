;*****************************************************************************
;* pixel-sse2.asm: h264 encoder library
;*****************************************************************************
;* Copyright (C) 2005 x264 project
;*
;* Authors: Alex Izvorski <aizvorksi@gmail.com>
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

BITS 64

;=============================================================================
; Macros and other preprocessor constants
;=============================================================================

%include "amd64inc.asm"

SECTION .rodata align=16

pd_0000ffff: times 4 dd 0x0000ffff
pb_1: times 16 db 1


SECTION .text


cglobal x264_pixel_sad_16x16_sse2
cglobal x264_pixel_sad_16x8_sse2
cglobal x264_pixel_ssd_16x16_sse2
cglobal x264_pixel_ssd_16x8_sse2
cglobal x264_pixel_satd_8x4_sse2
cglobal x264_pixel_satd_8x8_sse2
cglobal x264_pixel_satd_16x8_sse2
cglobal x264_pixel_satd_8x16_sse2
cglobal x264_pixel_satd_16x16_sse2
cglobal x264_pixel_sa8d_8x8_sse2
cglobal x264_pixel_sa8d_16x16_sse2
cglobal x264_intra_sa8d_x3_8x8_core_sse2

%macro SAD_INC_4x16P_SSE2 0
    movdqu  xmm1,   [rdx]
    movdqu  xmm2,   [rdx+rcx]
    lea     rdx,    [rdx+2*rcx]
    movdqu  xmm3,   [rdx]
    movdqu  xmm4,   [rdx+rcx]
    psadbw  xmm1,   [rdi]
    psadbw  xmm2,   [rdi+rsi]
    lea     rdi,    [rdi+2*rsi]
    psadbw  xmm3,   [rdi]
    psadbw  xmm4,   [rdi+rsi]
    lea     rdi,    [rdi+2*rsi]
    lea     rdx,    [rdx+2*rcx]
    paddw   xmm1,   xmm2
    paddw   xmm3,   xmm4
    paddw   xmm0,   xmm1
    paddw   xmm0,   xmm3
%endmacro

%macro SAD_START_SSE2 0
;   mov     rdi, rdi            ; pix1
    movsxd  rsi, esi            ; stride1
;   mov     rdx, rdx            ; pix2
    movsxd  rcx, ecx            ; stride2
%endmacro

%macro SAD_END_SSE2 0
    movdqa  xmm1, xmm0
    psrldq  xmm0,  8
    paddw   xmm0, xmm1
    movd    eax,  xmm0
    ret
%endmacro

ALIGN 16
;-----------------------------------------------------------------------------
;   int x264_pixel_sad_16x16_sse2 (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
x264_pixel_sad_16x16_sse2:
    SAD_START_SSE2
    movdqu xmm0, [rdx]
    movdqu xmm1, [rdx+rcx]
    lea    rdx,  [rdx+2*rcx]
    movdqu xmm2, [rdx]
    movdqu xmm3, [rdx+rcx]
    lea    rdx,  [rdx+2*rcx]
    psadbw xmm0, [rdi]
    psadbw xmm1, [rdi+rsi]
    lea    rdi,  [rdi+2*rsi]
    movdqu xmm4, [rdx]
    paddw  xmm0, xmm1
    psadbw xmm2, [rdi]
    psadbw xmm3, [rdi+rsi]
    lea    rdi,  [rdi+2*rsi]
    movdqu xmm5, [rdx+rcx]
    lea    rdx,  [rdx+2*rcx]
    paddw  xmm2, xmm3
    movdqu xmm6, [rdx]
    movdqu xmm7, [rdx+rcx]
    lea    rdx,  [rdx+2*rcx]
    paddw  xmm0, xmm2
    psadbw xmm4, [rdi]
    psadbw xmm5, [rdi+rsi]
    lea    rdi,  [rdi+2*rsi]
    movdqu xmm1, [rdx]
    paddw  xmm4, xmm5
    psadbw xmm6, [rdi]
    psadbw xmm7, [rdi+rsi]
    lea    rdi,  [rdi+2*rsi]
    movdqu xmm2, [rdx+rcx]
    lea    rdx,  [rdx+2*rcx]
    paddw  xmm6, xmm7
    movdqu xmm3, [rdx]
    paddw  xmm0, xmm4
    movdqu xmm4, [rdx+rcx]
    lea    rdx,  [rdx+2*rcx]
    paddw  xmm0, xmm6
    psadbw xmm1, [rdi]
    psadbw xmm2, [rdi+rsi]
    lea    rdi,  [rdi+2*rsi]
    movdqu xmm5, [rdx]
    paddw  xmm1, xmm2
    psadbw xmm3, [rdi]
    psadbw xmm4, [rdi+rsi]
    lea    rdi,  [rdi+2*rsi]
    movdqu xmm6, [rdx+rcx]
    lea    rdx,  [rdx+2*rcx]
    paddw  xmm3, xmm4
    movdqu xmm7, [rdx]
    paddw  xmm0, xmm1
    movdqu xmm1, [rdx+rcx]
    paddw  xmm0, xmm3
    psadbw xmm5, [rdi]
    psadbw xmm6, [rdi+rsi]
    lea    rdi,  [rdi+2*rsi]
    paddw  xmm5, xmm6
    psadbw xmm7, [rdi]
    psadbw xmm1, [rdi+rsi]
    paddw  xmm7, xmm1
    paddw  xmm0, xmm5
    paddw  xmm0, xmm7
    SAD_END_SSE2

ALIGN 16
;-----------------------------------------------------------------------------
;   int x264_pixel_sad_16x8_sse2 (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
x264_pixel_sad_16x8_sse2:
    SAD_START_SSE2
    pxor    xmm0,   xmm0
    SAD_INC_4x16P_SSE2
    SAD_INC_4x16P_SSE2
    SAD_END_SSE2

%macro SSD_INC_2x16P_SSE2 0
    movdqu  xmm1,   [rdi]
    movdqu  xmm2,   [rdx]
    movdqu  xmm3,   [rdi+rsi]
    movdqu  xmm4,   [rdx+rcx]

    movdqa  xmm5,   xmm1
    movdqa  xmm6,   xmm3
    psubusb xmm1,   xmm2
    psubusb xmm3,   xmm4
    psubusb xmm2,   xmm5
    psubusb xmm4,   xmm6
    por     xmm1,   xmm2
    por     xmm3,   xmm4

    movdqa  xmm2,   xmm1
    movdqa  xmm4,   xmm3
    punpcklbw xmm1, xmm7
    punpckhbw xmm2, xmm7
    punpcklbw xmm3, xmm7
    punpckhbw xmm4, xmm7
    pmaddwd xmm1,   xmm1
    pmaddwd xmm2,   xmm2
    pmaddwd xmm3,   xmm3
    pmaddwd xmm4,   xmm4

    lea     rdi,    [rdi+2*rsi]
    lea     rdx,    [rdx+2*rcx]

    paddd   xmm1,   xmm2
    paddd   xmm3,   xmm4
    paddd   xmm0,   xmm1
    paddd   xmm0,   xmm3
%endmacro

%macro SSD_INC_8x16P_SSE2 0
    SSD_INC_2x16P_SSE2
    SSD_INC_2x16P_SSE2
    SSD_INC_2x16P_SSE2
    SSD_INC_2x16P_SSE2
%endmacro

%macro SSD_START_SSE2 0
;   mov     rdi, rdi            ; pix1
    movsxd  rsi, esi            ; stride1
;   mov     rdx, rdx            ; pix2
    movsxd  rcx, ecx            ; stride2

    pxor    xmm7,   xmm7        ; zero
    pxor    xmm0,   xmm0        ; mm0 holds the sum
%endmacro

%macro SSD_END_SSE2 0
    movdqa  xmm1,   xmm0
    psrldq  xmm1,    8
    paddd   xmm0,   xmm1

    movdqa  xmm1,   xmm0
    psrldq  xmm1,    4
    paddd   xmm0,   xmm1

    movd    eax,    xmm0
    ret
%endmacro

ALIGN 16
;-----------------------------------------------------------------------------
;   int x264_pixel_ssd_16x16_sse2 (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
x264_pixel_ssd_16x16_sse2:
    SSD_START_SSE2
    SSD_INC_8x16P_SSE2
    SSD_INC_8x16P_SSE2
    SSD_END_SSE2

ALIGN 16
;-----------------------------------------------------------------------------
;   int x264_pixel_ssd_16x8_sse2 (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
x264_pixel_ssd_16x8_sse2:
    SSD_START_SSE2
    SSD_INC_8x16P_SSE2
    SSD_END_SSE2

; %1=(row2, row0) %2=(row3, row1) %3=junk
; output in %1=(row3, row0) and %3=(row2, row1)
%macro HADAMARD4x4_SSE2 3
    movdqa     %3, %1
    paddw      %1, %2
    psubw      %3, %2
    movdqa     %2, %1
    punpcklqdq %1, %3
    punpckhqdq %2, %3
    movdqa     %3, %1
    paddw      %1, %2
    psubw      %3, %2
%endmacro

;;; two HADAMARD4x4_SSE2 running side-by-side
%macro HADAMARD4x4_TWO_SSE2 6    ; a02 a13 junk1 b02 b13 junk2 (1=4 2=5 3=6)
    movdqa     %3, %1
    movdqa     %6, %4
    paddw      %1, %2
    paddw      %4, %5
    psubw      %3, %2
    psubw      %6, %5
    movdqa     %2, %1
    movdqa     %5, %4
    punpcklqdq %1, %3
    punpcklqdq %4, %6
    punpckhqdq %2, %3
    punpckhqdq %5, %6
    movdqa     %3, %1
    movdqa     %6, %4
    paddw      %1, %2
    paddw      %4, %5
    psubw      %3, %2
    psubw      %6, %5
%endmacro

%macro TRANSPOSE4x4_TWIST_SSE2 3    ; %1=(row3, row0) %2=(row2, row1) %3=junk, output in %1 and %2
    movdqa     %3, %1
    punpcklwd  %1, %2
    punpckhwd  %2, %3             ; backwards because the high quadwords are already swapped

    movdqa     %3, %1
    punpckldq  %1, %2
    punpckhdq  %3, %2

    movdqa     %2, %1
    punpcklqdq %1, %3
    punpckhqdq %2, %3
%endmacro

;;; two TRANSPOSE4x4_TWIST_SSE2 running side-by-side
%macro TRANSPOSE4x4_TWIST_TWO_SSE2 6    ; a02 a13 junk1 b02 b13 junk2 (1=4 2=5 3=6)
    movdqa     %3, %1
    movdqa     %6, %4
    punpcklwd  %1, %2
    punpcklwd  %4, %5
    punpckhwd  %2, %3
    punpckhwd  %5, %6
    movdqa     %3, %1
    movdqa     %6, %4
    punpckldq  %1, %2
    punpckldq  %4, %5
    punpckhdq  %3, %2
    punpckhdq  %6, %5
    movdqa     %2, %1
    movdqa     %5, %4
    punpcklqdq %1, %3
    punpcklqdq %4, %6
    punpckhqdq %2, %3
    punpckhqdq %5, %6
%endmacro

;;; loads the difference of two 4x4 blocks into xmm0,xmm1 and xmm4,xmm5 in interleaved-row order
;;; destroys xmm2, 3
;;; the value in xmm7 doesn't matter: it's only subtracted from itself
%macro LOAD4x8_DIFF_SSE2 0
    movq      xmm0, [rdi]
    movq      xmm4, [rdx]
    punpcklbw xmm0, xmm7
    punpcklbw xmm4, xmm7
    psubw     xmm0, xmm4

    movq      xmm1, [rdi+rsi]
    movq      xmm5, [rdx+rcx]
    lea       rdi,  [rdi+2*rsi]
    lea       rdx,  [rdx+2*rcx]
    punpcklbw xmm1, xmm7
    punpcklbw xmm5, xmm7
    psubw     xmm1, xmm5

    movq       xmm2, [rdi]
    movq       xmm4, [rdx]
    punpcklbw  xmm2, xmm7
    punpcklbw  xmm4, xmm7
    psubw      xmm2, xmm4
    movdqa     xmm4, xmm0
    punpcklqdq xmm0, xmm2        ; rows 0 and 2
    punpckhqdq xmm4, xmm2        ; next 4x4 rows 0 and 2

    movq       xmm3, [rdi+rsi]
    movq       xmm5, [rdx+rcx]
    lea        rdi,  [rdi+2*rsi]
    lea        rdx,  [rdx+2*rcx]
    punpcklbw  xmm3, xmm7
    punpcklbw  xmm5, xmm7
    psubw      xmm3, xmm5
    movdqa     xmm5, xmm1
    punpcklqdq xmm1, xmm3        ; rows 1 and 3
    punpckhqdq xmm5, xmm3        ; next 4x4 rows 1 and 3
%endmacro

%macro SUM1x8_SSE2 3    ; 01 junk sum
    pxor    %2, %2
    psubw   %2, %1
    pmaxsw  %1, %2
    paddusw %3, %1
%endmacro

%macro SUM4x4_SSE2 4    ; 02 13 junk sum
    pxor    %3, %3
    psubw   %3, %1
    pmaxsw  %1, %3

    pxor    %3, %3
    psubw   %3, %2
    pmaxsw  %2, %3

    paddusw %4, %1
    paddusw %4, %2
%endmacro

;;; two SUM4x4_SSE2 running side-by-side
%macro SUM4x4_TWO_SSE2 7    ; a02 a13 junk1 b02 b13 junk2 (1=4 2=5 3=6) sum
    pxor    %3, %3
    pxor    %6, %6
    psubw   %3, %1
    psubw   %6, %4
    pmaxsw  %1, %3
    pmaxsw  %4, %6
    pxor    %3, %3
    pxor    %6, %6
    psubw   %3, %2
    psubw   %6, %5
    pmaxsw  %2, %3
    pmaxsw  %5, %6
    paddusw %1, %2
    paddusw %4, %5
    paddusw %7, %1
    paddusw %7, %4
%endmacro

%macro SUM_MM_SSE2 2    ; sum junk
    movdqa  %2, %1
    psrldq  %1, 2
    paddusw %1, %2
    pand    %1, [pd_0000ffff GLOBAL]
    movdqa  %2, %1
    psrldq  %1, 4
    paddd   %1, %2
    movdqa  %2, %1
    psrldq  %1, 8
    paddd   %1, %2
    movd    eax,%1
%endmacro

%macro SATD_TWO_SSE2 0
    LOAD4x8_DIFF_SSE2
    HADAMARD4x4_TWO_SSE2        xmm0, xmm1, xmm2, xmm4, xmm5, xmm3
    TRANSPOSE4x4_TWIST_TWO_SSE2 xmm0, xmm2, xmm1, xmm4, xmm3, xmm5
    HADAMARD4x4_TWO_SSE2        xmm0, xmm2, xmm1, xmm4, xmm3, xmm5
    SUM4x4_TWO_SSE2             xmm0, xmm1, xmm2, xmm4, xmm5, xmm3, xmm6
%endmacro

%macro SATD_START 0
;   mov     rdi, rdi            ; pix1
    movsxd  rsi, esi            ; stride1
;   mov     rdx, rdx            ; pix2
    movsxd  rcx, ecx            ; stride2
    pxor    xmm6, xmm6
%endmacro

%macro SATD_END 0
    psrlw        xmm6, 1
    SUM_MM_SSE2  xmm6, xmm7
    ret
%endmacro

ALIGN 16
;-----------------------------------------------------------------------------
;   int x264_pixel_satd_16x16_sse2 (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
x264_pixel_satd_16x16_sse2:
    SATD_START
    mov     r8,  rdi
    mov     r9,  rdx

    SATD_TWO_SSE2
    SATD_TWO_SSE2
    SATD_TWO_SSE2
    SATD_TWO_SSE2

    lea     rdi, [r8+8]
    lea     rdx, [r9+8]

    SATD_TWO_SSE2
    SATD_TWO_SSE2
    SATD_TWO_SSE2
    SATD_TWO_SSE2

    SATD_END

ALIGN 16
;-----------------------------------------------------------------------------
;   int x264_pixel_satd_8x16_sse2 (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
x264_pixel_satd_8x16_sse2:
    SATD_START

    SATD_TWO_SSE2
    SATD_TWO_SSE2
    SATD_TWO_SSE2
    SATD_TWO_SSE2

    SATD_END

ALIGN 16
;-----------------------------------------------------------------------------
;   int x264_pixel_satd_16x8_sse2 (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
x264_pixel_satd_16x8_sse2:
    SATD_START
    mov     r8,  rdi
    mov     r9,  rdx

    SATD_TWO_SSE2
    SATD_TWO_SSE2

    lea     rdi, [r8+8]
    lea     rdx, [r9+8]

    SATD_TWO_SSE2
    SATD_TWO_SSE2

    SATD_END

ALIGN 16
;-----------------------------------------------------------------------------
;   int x264_pixel_satd_8x8_sse2 (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
x264_pixel_satd_8x8_sse2:
    SATD_START

    SATD_TWO_SSE2
    SATD_TWO_SSE2

    SATD_END

ALIGN 16
;-----------------------------------------------------------------------------
;   int x264_pixel_satd_8x4_sse2 (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
x264_pixel_satd_8x4_sse2:
    SATD_START

    SATD_TWO_SSE2

    SATD_END


%macro LOAD_DIFF_8P 4  ; MMP, MMT, [pix1], [pix2]
    movq        %1, %3
    movq        %2, %4
    punpcklbw   %1, %2
    punpcklbw   %2, %2
    psubw       %1, %2
%endmacro

%macro SBUTTERFLY 5
    mov%1       %5, %3
    punpckl%2   %3, %4
    punpckh%2   %5, %4
%endmacro

;-----------------------------------------------------------------------------
; input ABCDEFGH output AFHDTECB 
;-----------------------------------------------------------------------------
%macro TRANSPOSE8x8 9
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

%macro SUMSUB_BADC 4
    paddw   %1, %2
    paddw   %3, %4
    paddw   %2, %2
    paddw   %4, %4
    psubw   %2, %1
    psubw   %4, %3
%endmacro

%macro HADAMARD1x8 8
    SUMSUB_BADC %1, %5, %2, %6
    SUMSUB_BADC %3, %7, %4, %8
    SUMSUB_BADC %1, %3, %2, %4
    SUMSUB_BADC %5, %7, %6, %8
    SUMSUB_BADC %1, %2, %3, %4
    SUMSUB_BADC %5, %6, %7, %8
%endmacro

ALIGN 16
;-----------------------------------------------------------------------------
;   int x264_pixel_sa8d_8x8_sse2( uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
x264_pixel_sa8d_8x8_sse2:
    lea  r10, [3*parm2q]
    lea  r11, [3*parm4q]
    LOAD_DIFF_8P xmm0, xmm8, [parm1q],          [parm3q]
    LOAD_DIFF_8P xmm1, xmm9, [parm1q+parm2q],   [parm3q+parm4q]
    LOAD_DIFF_8P xmm2, xmm8, [parm1q+2*parm2q], [parm3q+2*parm4q]
    LOAD_DIFF_8P xmm3, xmm9, [parm1q+r10],      [parm3q+r11]
    lea  parm1q, [parm1q+4*parm2q]
    lea  parm3q, [parm3q+4*parm4q]
    LOAD_DIFF_8P xmm4, xmm8, [parm1q],          [parm3q]
    LOAD_DIFF_8P xmm5, xmm9, [parm1q+parm2q],   [parm3q+parm4q]
    LOAD_DIFF_8P xmm6, xmm8, [parm1q+2*parm2q], [parm3q+2*parm4q]
    LOAD_DIFF_8P xmm7, xmm9, [parm1q+r10],      [parm3q+r11]
    
    HADAMARD1x8  xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7
    TRANSPOSE8x8 xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm8
    HADAMARD1x8  xmm0, xmm5, xmm7, xmm3, xmm8, xmm4, xmm2, xmm1

    pxor            xmm10, xmm10
    SUM4x4_TWO_SSE2 xmm0, xmm1, xmm6, xmm2, xmm3, xmm9, xmm10
    SUM4x4_TWO_SSE2 xmm4, xmm5, xmm6, xmm7, xmm8, xmm9, xmm10
    psrlw           xmm10, 1
    SUM_MM_SSE2     xmm10, xmm0
    add r8d, eax ; preserve rounding for 16x16
    add eax, 1
    shr eax, 1
    ret

ALIGN 16
;-----------------------------------------------------------------------------
;   int x264_pixel_sa8d_16x16_sse2( uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
;; violates calling convention
x264_pixel_sa8d_16x16_sse2:
    xor  r8d, r8d
    call x264_pixel_sa8d_8x8_sse2 ; pix[0]
    lea  parm1q, [parm1q+4*parm2q]
    lea  parm3q, [parm3q+4*parm4q]
    call x264_pixel_sa8d_8x8_sse2 ; pix[8*stride]
    lea  r10, [3*parm2q-2]
    lea  r11, [3*parm4q-2]
    shl  r10, 2
    shl  r11, 2
    sub  parm1q, r10
    sub  parm3q, r11
    call x264_pixel_sa8d_8x8_sse2 ; pix[8]
    lea  parm1q, [parm1q+4*parm2q]
    lea  parm3q, [parm3q+4*parm4q]
    call x264_pixel_sa8d_8x8_sse2 ; pix[8*stride+8]
    mov  eax, r8d
    add  eax, 1
    shr  eax, 1
    ret



ALIGN 16
;-----------------------------------------------------------------------------
;  void x264_intra_sa8d_x3_8x8_core_sse2( uint8_t *fenc, int16_t edges[2][8], int *res )
;-----------------------------------------------------------------------------
x264_intra_sa8d_x3_8x8_core_sse2:
    ; 8x8 hadamard
    pxor        xmm4, xmm4
    movq        xmm0, [parm1q+0*FENC_STRIDE]
    movq        xmm7, [parm1q+1*FENC_STRIDE]
    movq        xmm6, [parm1q+2*FENC_STRIDE]
    movq        xmm3, [parm1q+3*FENC_STRIDE]
    movq        xmm5, [parm1q+4*FENC_STRIDE]
    movq        xmm1, [parm1q+5*FENC_STRIDE]
    movq        xmm8, [parm1q+6*FENC_STRIDE]
    movq        xmm2, [parm1q+7*FENC_STRIDE]
    punpcklbw   xmm0, xmm4
    punpcklbw   xmm7, xmm4
    punpcklbw   xmm6, xmm4
    punpcklbw   xmm3, xmm4
    punpcklbw   xmm5, xmm4
    punpcklbw   xmm1, xmm4
    punpcklbw   xmm8, xmm4
    punpcklbw   xmm2, xmm4
    HADAMARD1x8 xmm0, xmm7, xmm6, xmm3, xmm5, xmm1, xmm8, xmm2
    TRANSPOSE8x8 xmm0, xmm7, xmm6, xmm3, xmm5, xmm1, xmm8, xmm2, xmm4
    HADAMARD1x8 xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7

    ; dc
    movzx       edi, word [parm2q+0]
    add          di, word [parm2q+16]
    add         edi, 8
    and         edi, -16
    shl         edi, 2

    pxor        xmm15, xmm15
    movdqa      xmm8, xmm2
    movdqa      xmm9, xmm3
    movdqa      xmm10, xmm4
    movdqa      xmm11, xmm5
    SUM4x4_TWO_SSE2 xmm8, xmm9, xmm12, xmm10, xmm11, xmm13, xmm15
    movdqa      xmm8, xmm6
    movdqa      xmm9, xmm7
    SUM4x4_SSE2 xmm8, xmm9, xmm10, xmm15
    movdqa      xmm8, xmm1
    SUM1x8_SSE2 xmm8, xmm10, xmm15
    movdqa      xmm14, xmm15 ; 7x8 sum

    movdqa      xmm8, [parm2q+0] ; left edge
    movd        xmm9, edi
    psllw       xmm8, 3
    psubw       xmm8, xmm0
    psubw       xmm9, xmm0
    SUM1x8_SSE2 xmm8, xmm10, xmm14
    SUM1x8_SSE2 xmm9, xmm11, xmm15 ; 1x8 sum
    punpcklwd   xmm0, xmm1
    punpcklwd   xmm2, xmm3
    punpcklwd   xmm4, xmm5
    punpcklwd   xmm6, xmm7
    punpckldq   xmm0, xmm2
    punpckldq   xmm4, xmm6
    punpcklqdq  xmm0, xmm4 ; transpose
    movdqa      xmm1, [parm2q+16] ; top edge
    movdqa      xmm2, xmm15
    psllw       xmm1, 3
    psrldq      xmm2, 2     ; 8x7 sum
    psubw       xmm0, xmm1  ; 8x1 sum
    SUM1x8_SSE2 xmm0, xmm1, xmm2

    SUM_MM_SSE2 xmm14, xmm3
    add         eax, 2
    shr         eax, 2
    mov         [parm3q+4], eax ; i8x8_h sa8d
    SUM_MM_SSE2 xmm15, xmm4
    add         eax, 2
    shr         eax, 2
    mov         [parm3q+8], eax ; i8x8_dc sa8d
    SUM_MM_SSE2 xmm2, xmm5
    add         eax, 2
    shr         eax, 2
    mov         [parm3q+0], eax ; i8x8_v sa8d

    ret

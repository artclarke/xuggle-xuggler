;*****************************************************************************
;* pixel.asm: h264 encoder library
;*****************************************************************************
;* Copyright (C) 2003-2008 x264 project
;*
;* Authors: Loren Merritt <lorenm@u.washington.edu>
;*          Holger Lubitz <holger@lubitz.org>
;*          Laurent Aimar <fenrir@via.ecp.fr>
;*          Alex Izvorski <aizvorksi@gmail.com>
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
;*****************************************************************************

%include "x86inc.asm"
%include "x86util.asm"

SECTION_RODATA
pw_1:      times 8 dw 1
pw_00ff:   times 8 dw 0xff
ssim_c1:   times 4 dd 416    ; .01*.01*255*255*64
ssim_c2:   times 4 dd 235963 ; .03*.03*255*255*64*63
mask_ff:   times 16 db 0xff
           times 16 db 0
mask_ac4:  dw 0, -1, -1, -1, 0, -1, -1, -1
mask_ac4b: dw 0, -1, 0, -1, -1, -1, -1, -1
mask_ac8:  dw 0, -1, -1, -1, -1, -1, -1, -1
hsub_mul:  times 8 db 1, -1
hmul_4p:   times 2 db 1, 1, 1, 1, 1, -1, 1, -1
hmul_8p:   times 8 db 1
           times 4 db 1, -1
mask_10:   times 4 dw 0, -1
mask_1100: times 2 dd 0, -1

SECTION .text

%macro HADDD 2 ; sum junk
%if mmsize == 16
    movhlps %2, %1
    paddd   %1, %2
    pshuflw %2, %1, 0xE
    paddd   %1, %2
%else
    pshufw  %2, %1, 0xE
    paddd   %1, %2
%endif
%endmacro

%macro HADDW 2
    pmaddwd %1, [pw_1 GLOBAL]
    HADDD   %1, %2
%endmacro

%macro HADDUW 2
    mova  %2, %1
    pslld %1, 16
    psrld %2, 16
    psrld %1, 16
    paddd %1, %2
    HADDD %1, %2
%endmacro

;=============================================================================
; SSD
;=============================================================================

%macro SSD_LOAD_FULL 5
    mova      m1, [r0+%1]
    mova      m2, [r2+%2]
    mova      m3, [r0+%3]
    mova      m4, [r2+%4]
%if %5
    lea       r0, [r0+2*r1]
    lea       r2, [r2+2*r3]
%endif
%endmacro

%macro LOAD 5
    movh      m%1, %3
    movh      m%2, %4
%if %5
    lea       r0, [r0+2*r1]
%endif
%endmacro

%macro JOIN 7
    movh      m%3, %5
    movh      m%4, %6
%if %7
    lea       r2, [r2+2*r3]
%endif
    punpcklbw m%1, m7
    punpcklbw m%3, m7
    psubw     m%1, m%3
    punpcklbw m%2, m7
    punpcklbw m%4, m7
    psubw     m%2, m%4
%endmacro

%macro JOIN_SSE2 7
    movh      m%3, %5
    movh      m%4, %6
%if %7
    lea       r2, [r2+2*r3]
%endif
    punpcklqdq m%1, m%2
    punpcklqdq m%3, m%4
    DEINTB %2, %1, %4, %3, 7
    psubw m%2, m%4
    psubw m%1, m%3
%endmacro

%macro JOIN_SSSE3 7
    movh      m%3, %5
    movh      m%4, %6
%if %7
    lea       r2, [r2+2*r3]
%endif
    punpcklbw m%1, m%3
    punpcklbw m%2, m%4
%endmacro

%macro SSD_LOAD_HALF 5
    LOAD      1, 2, [r0+%1], [r0+%3], 1
    JOIN      1, 2, 3, 4, [r2+%2], [r2+%4], 1
    LOAD      3, 4, [r0+%1], [r0+%3], %5
    JOIN      3, 4, 5, 6, [r2+%2], [r2+%4], %5
%endmacro

%macro SSD_CORE 7-8
%ifidn %8, FULL
    mova      m%6, m%2
    mova      m%7, m%4
    psubusb   m%2, m%1
    psubusb   m%4, m%3
    psubusb   m%1, m%6
    psubusb   m%3, m%7
    por       m%1, m%2
    por       m%3, m%4
    mova      m%2, m%1
    mova      m%4, m%3
    punpckhbw m%1, m%5
    punpckhbw m%3, m%5
    punpcklbw m%2, m%5
    punpcklbw m%4, m%5
%endif
    pmaddwd   m%1, m%1
    pmaddwd   m%2, m%2
    pmaddwd   m%3, m%3
    pmaddwd   m%4, m%4
%endmacro

%macro SSD_CORE_SSE2 7-8
%ifidn %8, FULL
    DEINTB %6, %1, %7, %2, %5
    psubw m%6, m%7
    psubw m%1, m%2
    SWAP %2, %6
    DEINTB %6, %3, %7, %4, %5
    psubw m%6, m%7
    psubw m%3, m%4
    SWAP %4, %6
%endif
    pmaddwd   m%1, m%1
    pmaddwd   m%2, m%2
    pmaddwd   m%3, m%3
    pmaddwd   m%4, m%4
%endmacro

%macro SSD_CORE_SSSE3 7-8
%ifidn %8, FULL
    mova      m%6, m%1
    mova      m%7, m%3
    punpcklbw m%1, m%2
    punpcklbw m%3, m%4
    punpckhbw m%6, m%2
    punpckhbw m%7, m%4
    SWAP %6, %2
    SWAP %7, %4
%endif
    pmaddubsw m%1, m%5
    pmaddubsw m%2, m%5
    pmaddubsw m%3, m%5
    pmaddubsw m%4, m%5
    pmaddwd   m%1, m%1
    pmaddwd   m%2, m%2
    pmaddwd   m%3, m%3
    pmaddwd   m%4, m%4
%endmacro

%macro SSD_END 1
    paddd     m1, m2
    paddd     m3, m4
%if %1
    paddd     m0, m1
%else
    SWAP      0, 1
%endif
    paddd     m0, m3
%endmacro

%macro SSD_ITER 7
    SSD_LOAD_%1 %2,%3,%4,%5,%7
    SSD_CORE  1, 2, 3, 4, 7, 5, 6, %1
    SSD_END  %6
%endmacro

;-----------------------------------------------------------------------------
; int x264_pixel_ssd_16x16_mmx( uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
%macro SSD 3-4 0
cglobal x264_pixel_ssd_%1x%2_%3, 4,4,%4
%ifidn %3, ssse3
    mova    m7, [hsub_mul GLOBAL]
%elifidn %3, sse2
    mova    m7, [pw_00ff GLOBAL]
%elif %1 >= mmsize
    pxor    m7, m7
%endif
%assign i 0
%rep %2/4
%if %1 > mmsize
    SSD_ITER FULL, 0,  0,     mmsize,    mmsize, i, 0
    SSD_ITER FULL, r1, r3, r1+mmsize, r3+mmsize, 1, 1
    SSD_ITER FULL, 0,  0,     mmsize,    mmsize, 1, 0
    SSD_ITER FULL, r1, r3, r1+mmsize, r3+mmsize, 1, i<%2/4-1
%elif %1 == mmsize
    SSD_ITER FULL, 0, 0, r1, r3, i, 1
    SSD_ITER FULL, 0, 0, r1, r3, 1, i<%2/4-1
%else
    SSD_ITER HALF, 0, 0, r1, r3, i, i<%2/4-1
%endif
%assign i i+1
%endrep
    HADDD   m0, m1
    movd   eax, m0
    RET
%endmacro

INIT_MMX
SSD 16, 16, mmx
SSD 16,  8, mmx
SSD  8, 16, mmx
SSD  8,  8, mmx
SSD  8,  4, mmx
SSD  4,  8, mmx
SSD  4,  4, mmx
INIT_XMM
SSD 16, 16, sse2slow, 8
SSD 16,  8, sse2slow, 8
SSD  8, 16, sse2slow, 8
SSD  8,  8, sse2slow, 8
SSD  8,  4, sse2slow, 8
%define SSD_CORE SSD_CORE_SSE2
%define JOIN JOIN_SSE2
SSD 16, 16, sse2, 8
SSD 16,  8, sse2, 8
SSD  8, 16, sse2, 8
SSD  8,  8, sse2, 8
SSD  8,  4, sse2, 8
%define SSD_CORE SSD_CORE_SSSE3
%define JOIN JOIN_SSSE3
SSD 16, 16, ssse3, 8
SSD 16,  8, ssse3, 8
SSD  8, 16, ssse3, 8
SSD  8,  8, ssse3, 8
SSD  8,  4, ssse3, 8
INIT_MMX
SSD  4,  8, ssse3
SSD  4,  4, ssse3

;=============================================================================
; variance
;=============================================================================

%macro VAR_START 1
    pxor  m5, m5    ; sum
    pxor  m6, m6    ; sum squared
%if %1
    mova  m7, [pw_00ff GLOBAL]
%else
    pxor  m7, m7    ; zero
%endif
%endmacro

%macro VAR_END 1
    HADDW   m5, m7
    movd   r1d, m5
    imul   r1d, r1d
    HADDD   m6, m1
    shr    r1d, %1
    movd   eax, m6
    sub    eax, r1d  ; sqr - (sum * sum >> shift)
    RET
%endmacro

%macro VAR_CORE 0
    paddw     m5, m0
    paddw     m5, m3
    paddw     m5, m1
    paddw     m5, m4
    pmaddwd   m0, m0
    pmaddwd   m3, m3
    pmaddwd   m1, m1
    pmaddwd   m4, m4
    paddd     m6, m0
    paddd     m6, m3
    paddd     m6, m1
    paddd     m6, m4
%endmacro

%macro VAR_2ROW 2
    mov      r2d, %2
.loop:
    mova      m0, [r0]
    mova      m1, m0
    mova      m3, [r0+%1]
    mova      m4, m3
    punpcklbw m0, m7
    punpckhbw m1, m7
%ifidn %1, r1
    lea       r0, [r0+%1*2]
%else
    add       r0, r1
%endif
    punpcklbw m3, m7
    punpckhbw m4, m7
    dec r2d
    VAR_CORE
    jg .loop
%endmacro

;-----------------------------------------------------------------------------
; int x264_pixel_var_wxh_mmxext( uint8_t *, int )
;-----------------------------------------------------------------------------
INIT_MMX
cglobal x264_pixel_var_16x16_mmxext, 2,3
    VAR_START 0
    VAR_2ROW 8, 16
    VAR_END 8

cglobal x264_pixel_var_8x8_mmxext, 2,3
    VAR_START 0
    VAR_2ROW r1, 4
    VAR_END 6

INIT_XMM
cglobal x264_pixel_var_16x16_sse2, 2,3,8
    VAR_START 1
    mov      r2d, 8
.loop:
    mova      m0, [r0]
    mova      m3, [r0+r1]
    DEINTB    1, 0, 4, 3, 7
    lea       r0, [r0+r1*2]
    VAR_CORE
    dec r2d
    jg .loop
    VAR_END 8

cglobal x264_pixel_var_8x8_sse2, 2,4,8
    VAR_START 1
    mov      r2d, 2
    lea       r3, [r1*3]
.loop:
    movh      m0, [r0]
    movh      m3, [r0+r1]
    movhps    m0, [r0+r1*2]
    movhps    m3, [r0+r3]
    DEINTB    1, 0, 4, 3, 7
    lea       r0, [r0+r1*4]
    VAR_CORE
    dec r2d
    jg .loop
    VAR_END 6


;=============================================================================
; SATD
;=============================================================================

%macro TRANS_SSE2 5-6
; TRANSPOSE2x2
; %1: transpose width (d/q) - use SBUTTERFLY qdq for dq
; %2: ord/unord (for compat with sse4, unused)
; %3/%4: source regs
; %5/%6: tmp regs
%ifidn %1, d
%define mask [mask_10 GLOBAL]
%define shift 16
%elifidn %1, q
%define mask [mask_1100 GLOBAL]
%define shift 32
%endif
%if %0==6 ; less dependency if we have two tmp
    mova   m%5, mask   ; ff00
    mova   m%6, m%4    ; x5x4
    psll%1 m%4, shift  ; x4..
    pand   m%6, m%5    ; x5..
    pandn  m%5, m%3    ; ..x0
    psrl%1 m%3, shift  ; ..x1
    por    m%4, m%5    ; x4x0
    por    m%3, m%6    ; x5x1
%else ; more dependency, one insn less. sometimes faster, sometimes not
    mova   m%5, m%4    ; x5x4
    psll%1 m%4, shift  ; x4..
    pxor   m%4, m%3    ; (x4^x1)x0
    pand   m%4, mask   ; (x4^x1)..
    pxor   m%3, m%4    ; x4x0
    psrl%1 m%4, shift  ; ..(x1^x4)
    pxor   m%5, m%4    ; x5x1
    SWAP   %4, %3, %5
%endif
%endmacro

%define TRANS TRANS_SSE2

%macro TRANS_SSE4 5-6 ; see above
%ifidn %1, d
%define mask 10101010b
%define shift 16
%elifidn %1, q
%define mask 11001100b
%define shift 32
%endif
    mova   m%5, m%3
%ifidn %2, ord
    psrl%1 m%3, shift
%endif
    pblendw m%3, m%4, mask
    psll%1 m%4, shift
%ifidn %2, ord
    pblendw m%4, m%5, 255^mask
%else
    psrl%1 m%5, shift
    por    m%4, m%5
%endif
%endmacro

%macro JDUP_SSE2 2
    punpckldq %1, %2
    ; doesn't need to dup. sse2 does things by zero extending to words and full h_2d
%endmacro

%macro JDUP_CONROE 2
    ; join 2x 32 bit and duplicate them
    ; emulating shufps is faster on conroe
    punpcklqdq %1, %2
    movsldup %1, %1
%endmacro

%macro JDUP_PENRYN 2
    ; just use shufps on anything post conroe
    shufps %1, %2, 0
%endmacro

%macro HSUMSUB 5
    pmaddubsw m%2, m%5
    pmaddubsw m%1, m%5
    pmaddubsw m%4, m%5
    pmaddubsw m%3, m%5
%endmacro

%macro DIFF_UNPACK_SSE2 5
    punpcklbw m%1, m%5
    punpcklbw m%2, m%5
    punpcklbw m%3, m%5
    punpcklbw m%4, m%5
    psubw m%1, m%2
    psubw m%3, m%4
%endmacro

%macro DIFF_SUMSUB_SSSE3 5
    HSUMSUB %1, %2, %3, %4, %5
    psubw m%1, m%2
    psubw m%3, m%4
%endmacro

%macro LOAD_DUP_2x4P 4 ; dst, tmp, 2* pointer
    movd %1, %3
    movd %2, %4
    JDUP %1, %2
%endmacro

%macro LOAD_DUP_4x8P_CONROE 8 ; 4*dst, 4*pointer
    movddup m%3, %6
    movddup m%4, %8
    movddup m%1, %5
    movddup m%2, %7
%endmacro

%macro LOAD_DUP_4x8P_PENRYN 8
    ; penryn and nehalem run punpcklqdq and movddup in different units
    movh m%3, %6
    movh m%4, %8
    punpcklqdq m%3, m%3
    movddup m%1, %5
    punpcklqdq m%4, m%4
    movddup m%2, %7
%endmacro

%macro LOAD_SUMSUB_8x2P 9
    LOAD_DUP_4x8P %1, %2, %3, %4, %6, %7, %8, %9
    DIFF_SUMSUB_SSSE3 %1, %3, %2, %4, %5
%endmacro

%macro LOAD_SUMSUB_8x4P_SSSE3 7-10 r0, r2, 0
; 4x dest, 2x tmp, 1x mul, [2* ptr], [increment?]
    LOAD_SUMSUB_8x2P %1, %2, %5, %6, %7, [%8], [%9], [%8+r1], [%9+r3]
    LOAD_SUMSUB_8x2P %3, %4, %5, %6, %7, [%8+2*r1], [%9+2*r3], [%8+r4], [%9+r5]
%if %10
    lea %8, [%8+4*r1]
    lea %9, [%9+4*r3]
%endif
%endmacro

%macro LOAD_SUMSUB_16P_SSSE3 7 ; 2*dst, 2*tmp, mul, 2*ptr
    movddup m%1, [%7]
    movddup m%2, [%7+8]
    mova m%4, [%6]
    movddup m%3, m%4
    punpckhqdq m%4, m%4
    DIFF_SUMSUB_SSSE3 %1, %3, %2, %4, %5
%endmacro

%macro LOAD_SUMSUB_16P_SSE2 7 ; 2*dst, 2*tmp, mask, 2*ptr
    movu  m%4, [%7]
    mova  m%2, [%6]
    DEINTB %1, %2, %3, %4, %5
    psubw m%1, m%3
    psubw m%2, m%4
    SUMSUB_BA m%1, m%2, m%3
%endmacro

%macro LOAD_SUMSUB_16x4P 10-13 r0, r2, none
; 8x dest, 1x tmp, 1x mul, [2* ptr] [2nd tmp]
    LOAD_SUMSUB_16P %1, %5, %2, %3, %10, %11, %12
    LOAD_SUMSUB_16P %2, %6, %3, %4, %10, %11+r1, %12+r3
    LOAD_SUMSUB_16P %3, %7, %4, %9, %10, %11+2*r1, %12+2*r3
    LOAD_SUMSUB_16P %4, %8, %13, %9, %10, %11+r4, %12+r5
%endmacro

; in: r4=3*stride1, r5=3*stride2
; in: %2 = horizontal offset
; in: %3 = whether we need to increment pix1 and pix2
; clobber: m3..m7
; out: %1 = satd
%macro SATD_4x4_MMX 3
    %xdefine %%n n%1
    LOAD_DIFF m4, m3, none, [r0+%2],      [r2+%2]
    LOAD_DIFF m5, m3, none, [r0+r1+%2],   [r2+r3+%2]
    LOAD_DIFF m6, m3, none, [r0+2*r1+%2], [r2+2*r3+%2]
    LOAD_DIFF m7, m3, none, [r0+r4+%2],   [r2+r5+%2]
%if %3
    lea  r0, [r0+4*r1]
    lea  r2, [r2+4*r3]
%endif
    HADAMARD4_2D 4, 5, 6, 7, 3, %%n
    paddw m4, m6
    SWAP %%n, 4
%endmacro

%macro SATD_8x4_SSE 8-9
%ifidn %1, sse2
    HADAMARD4_2D_SSE %2, %3, %4, %5, %6, amax
%else
    HADAMARD4_V m%2, m%3, m%4, m%5, m%6
    ; doing the abs first is a slight advantage
    ABS4 m%2, m%4, m%3, m%5, m%6, m%7
    HADAMARD 1, max, %2, %4, %6, %7
%endif
%ifnidn %9, swap
    paddw m%8, m%2
%else
    SWAP %8, %2
%endif
%ifidn %1, sse2
    paddw m%8, m%4
%else
    HADAMARD 1, max, %3, %5, %6, %7
    paddw m%8, m%3
%endif
%endmacro

%macro SATD_START_MMX 0
    lea  r4, [3*r1] ; 3*stride1
    lea  r5, [3*r3] ; 3*stride2
%endmacro

%macro SATD_END_MMX 0
    pshufw      m1, m0, 01001110b
    paddw       m0, m1
    pshufw      m1, m0, 10110001b
    paddw       m0, m1
    movd       eax, m0
    and        eax, 0xffff
    RET
%endmacro

; FIXME avoid the spilling of regs to hold 3*stride.
; for small blocks on x86_32, modify pixel pointer instead.

;-----------------------------------------------------------------------------
; int x264_pixel_satd_16x16_mmxext (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
INIT_MMX
cglobal x264_pixel_satd_16x4_internal_mmxext
    SATD_4x4_MMX m2,  0, 0
    SATD_4x4_MMX m1,  4, 0
    paddw        m0, m2
    SATD_4x4_MMX m2,  8, 0
    paddw        m0, m1
    SATD_4x4_MMX m1, 12, 0
    paddw        m0, m2
    paddw        m0, m1
    ret

cglobal x264_pixel_satd_8x8_internal_mmxext
    SATD_4x4_MMX m2,  0, 0
    SATD_4x4_MMX m1,  4, 1
    paddw        m0, m2
    paddw        m0, m1
x264_pixel_satd_8x4_internal_mmxext:
    SATD_4x4_MMX m2,  0, 0
    SATD_4x4_MMX m1,  4, 0
    paddw        m0, m2
    paddw        m0, m1
    ret

cglobal x264_pixel_satd_16x16_mmxext, 4,6
    SATD_START_MMX
    pxor   m0, m0
%rep 3
    call x264_pixel_satd_16x4_internal_mmxext
    lea  r0, [r0+4*r1]
    lea  r2, [r2+4*r3]
%endrep
    call x264_pixel_satd_16x4_internal_mmxext
    HADDUW m0, m1
    movd  eax, m0
    RET

cglobal x264_pixel_satd_16x8_mmxext, 4,6
    SATD_START_MMX
    pxor   m0, m0
    call x264_pixel_satd_16x4_internal_mmxext
    lea  r0, [r0+4*r1]
    lea  r2, [r2+4*r3]
    call x264_pixel_satd_16x4_internal_mmxext
    SATD_END_MMX

cglobal x264_pixel_satd_8x16_mmxext, 4,6
    SATD_START_MMX
    pxor   m0, m0
    call x264_pixel_satd_8x8_internal_mmxext
    lea  r0, [r0+4*r1]
    lea  r2, [r2+4*r3]
    call x264_pixel_satd_8x8_internal_mmxext
    SATD_END_MMX

cglobal x264_pixel_satd_8x8_mmxext, 4,6
    SATD_START_MMX
    pxor   m0, m0
    call x264_pixel_satd_8x8_internal_mmxext
    SATD_END_MMX

cglobal x264_pixel_satd_8x4_mmxext, 4,6
    SATD_START_MMX
    pxor   m0, m0
    call x264_pixel_satd_8x4_internal_mmxext
    SATD_END_MMX

cglobal x264_pixel_satd_4x8_mmxext, 4,6
    SATD_START_MMX
    SATD_4x4_MMX m0, 0, 1
    SATD_4x4_MMX m1, 0, 0
    paddw  m0, m1
    SATD_END_MMX

cglobal x264_pixel_satd_4x4_mmxext, 4,6
    SATD_START_MMX
    SATD_4x4_MMX m0, 0, 0
    SATD_END_MMX

%macro SATD_START_SSE2 3
%ifnidn %1, sse2
    mova    %3, [hmul_8p GLOBAL]
%endif
    lea     r4, [3*r1]
    lea     r5, [3*r3]
    pxor    %2, %2
%endmacro

%macro SATD_END_SSE2 2
    HADDW   %2, m7
    movd   eax, %2
    RET
%endmacro

%macro BACKUP_POINTERS 0
%ifdef ARCH_X86_64
    mov    r10, r0
    mov    r11, r2
%endif
%endmacro

%macro RESTORE_AND_INC_POINTERS 0
%ifdef ARCH_X86_64
    lea     r0, [r10+8]
    lea     r2, [r11+8]
%else
    mov     r0, r0mp
    mov     r2, r2mp
    add     r0, 8
    add     r2, 8
%endif
%endmacro

;-----------------------------------------------------------------------------
; int x264_pixel_satd_8x4_sse2 (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
%macro SATDS_SSE2 1
INIT_XMM
%ifnidn %1, sse2
cglobal x264_pixel_satd_4x4_%1, 4, 6, 6
    SATD_START_MMX
    mova m4, [hmul_4p GLOBAL]
    LOAD_DUP_2x4P m2, m5, [r2], [r2+r3]
    LOAD_DUP_2x4P m3, m5, [r2+2*r3], [r2+r5]
    LOAD_DUP_2x4P m0, m5, [r0], [r0+r1]
    LOAD_DUP_2x4P m1, m5, [r0+2*r1], [r0+r4]
    DIFF_SUMSUB_SSSE3 0, 2, 1, 3, 4
    HADAMARD 0, sumsub, 0, 1, 2, 3
    HADAMARD 4, sumsub, 0, 1, 2, 3
    HADAMARD 1, amax, 0, 1, 2, 3
    HADDW m0, m1
    movd eax, m0
    RET
%endif

cglobal x264_pixel_satd_4x8_%1, 4, 6, 8
    SATD_START_MMX
%ifnidn %1, sse2
    mova m7, [hmul_4p GLOBAL]
%endif
    movd m4, [r2]
    movd m5, [r2+r3]
    movd m6, [r2+2*r3]
    add r2, r5
    movd m0, [r0]
    movd m1, [r0+r1]
    movd m2, [r0+2*r1]
    add r0, r4
    movd m3, [r2+r3]
    JDUP m4, m3
    movd m3, [r0+r1]
    JDUP m0, m3
    movd m3, [r2+2*r3]
    JDUP m5, m3
    movd m3, [r0+2*r1]
    JDUP m1, m3
    DIFFOP 0, 4, 1, 5, 7
    movd m5, [r2]
    add r2, r5
    movd m3, [r0]
    add r0, r4
    movd m4, [r2]
    JDUP m6, m4
    movd m4, [r0]
    JDUP m2, m4
    movd m4, [r2+r3]
    JDUP m5, m4
    movd m4, [r0+r1]
    JDUP m3, m4
    DIFFOP 2, 6, 3, 5, 7
    SATD_8x4_SSE %1, 0, 1, 2, 3, 4, 5, 6, swap
    HADDW m6, m1
    movd eax, m6
    RET

cglobal x264_pixel_satd_8x8_internal_%1
    LOAD_SUMSUB_8x4P 0, 1, 2, 3, 4, 5, 7, r0, r2, 1
    SATD_8x4_SSE %1, 0, 1, 2, 3, 4, 5, 6
x264_pixel_satd_8x4_internal_%1:
    LOAD_SUMSUB_8x4P 0, 1, 2, 3, 4, 5, 7, r0, r2, 1
    SATD_8x4_SSE %1, 0, 1, 2, 3, 4, 5, 6
    ret

%ifdef UNIX64 ; 16x8 regresses on phenom win64, 16x16 is almost the same
cglobal x264_pixel_satd_16x4_internal_%1
    LOAD_SUMSUB_16x4P 0, 1, 2, 3, 4, 8, 5, 9, 6, 7, r0, r2, 11
    lea  r2, [r2+4*r3]
    lea  r0, [r0+4*r1]
    SATD_8x4_SSE ssse3, 0, 1, 2, 3, 6, 11, 10
    SATD_8x4_SSE ssse3, 4, 8, 5, 9, 6, 3, 10
    ret

cglobal x264_pixel_satd_16x8_%1, 4,6,12
    SATD_START_SSE2 %1, m10, m7
%ifidn %1, sse2
    mova m7, [pw_00ff GLOBAL]
%endif
    jmp x264_pixel_satd_16x8_internal_%1

cglobal x264_pixel_satd_16x16_%1, 4,6,12
    SATD_START_SSE2 %1, m10, m7
%ifidn %1, sse2
    mova m7, [pw_00ff GLOBAL]
%endif
    call x264_pixel_satd_16x4_internal_%1
    call x264_pixel_satd_16x4_internal_%1
x264_pixel_satd_16x8_internal_%1:
    call x264_pixel_satd_16x4_internal_%1
    call x264_pixel_satd_16x4_internal_%1
    SATD_END_SSE2 %1, m10
%else
cglobal x264_pixel_satd_16x8_%1, 4,6,8
    SATD_START_SSE2 %1, m6, m7
    BACKUP_POINTERS
    call x264_pixel_satd_8x8_internal_%1
    RESTORE_AND_INC_POINTERS
    call x264_pixel_satd_8x8_internal_%1
    SATD_END_SSE2 %1, m6

cglobal x264_pixel_satd_16x16_%1, 4,6,8
    SATD_START_SSE2 %1, m6, m7
    BACKUP_POINTERS
    call x264_pixel_satd_8x8_internal_%1
    call x264_pixel_satd_8x8_internal_%1
    RESTORE_AND_INC_POINTERS
    call x264_pixel_satd_8x8_internal_%1
    call x264_pixel_satd_8x8_internal_%1
    SATD_END_SSE2 %1, m6
%endif

cglobal x264_pixel_satd_8x16_%1, 4,6,8
    SATD_START_SSE2 %1, m6, m7
    call x264_pixel_satd_8x8_internal_%1
    call x264_pixel_satd_8x8_internal_%1
    SATD_END_SSE2 %1, m6

cglobal x264_pixel_satd_8x8_%1, 4,6,8
    SATD_START_SSE2 %1, m6, m7
    call x264_pixel_satd_8x8_internal_%1
    SATD_END_SSE2 %1, m6

cglobal x264_pixel_satd_8x4_%1, 4,6,8
    SATD_START_SSE2 %1, m6, m7
    call x264_pixel_satd_8x4_internal_%1
    SATD_END_SSE2 %1, m6
%endmacro ; SATDS_SSE2

%macro SA8D 1
%ifdef ARCH_X86_64
;-----------------------------------------------------------------------------
; int x264_pixel_sa8d_8x8_sse2( uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
cglobal x264_pixel_sa8d_8x8_internal_%1
    lea  r10, [r0+4*r1]
    lea  r11, [r2+4*r3]
    LOAD_SUMSUB_8x4P 0, 1, 2, 8, 5, 6, 7, r0, r2
    LOAD_SUMSUB_8x4P 4, 5, 3, 9, 11, 6, 7, r10, r11
%ifidn %1, sse2 ; sse2 doesn't seem to like the horizontal way of doing things
    HADAMARD8_2D 0, 1, 2, 8, 4, 5, 3, 9, 6, amax
%else ; non-sse2
    HADAMARD4_V m0, m1, m2, m8, m6
    HADAMARD4_V m4, m5, m3, m9, m6
    SUMSUB_BADC m0, m4, m1, m5, m6
    HADAMARD 2, sumsub, 0, 4, 6, 11
    HADAMARD 2, sumsub, 1, 5, 6, 11
    SUMSUB_BADC m2, m3, m8, m9, m6
    HADAMARD 2, sumsub, 2, 3, 6, 11
    HADAMARD 2, sumsub, 8, 9, 6, 11
    HADAMARD 1, amax, 0, 4, 6, 11
    HADAMARD 1, amax, 1, 5, 6, 4
    HADAMARD 1, amax, 2, 3, 6, 4
    HADAMARD 1, amax, 8, 9, 6, 4
%endif
    paddw m0, m1
    paddw m0, m2
    paddw m0, m8
    SAVE_MM_PERMUTATION x264_pixel_sa8d_8x8_internal_%1
    ret

cglobal x264_pixel_sa8d_8x8_%1, 4,6,12
    lea  r4, [3*r1]
    lea  r5, [3*r3]
%ifnidn %1, sse2
    mova m7, [hmul_8p GLOBAL]
%endif
    call x264_pixel_sa8d_8x8_internal_%1
    HADDW m0, m1
    movd eax, m0
    add eax, 1
    shr eax, 1
    RET

cglobal x264_pixel_sa8d_16x16_%1, 4,6,12
    lea  r4, [3*r1]
    lea  r5, [3*r3]
%ifnidn %1, sse2
    mova m7, [hmul_8p GLOBAL]
%endif
    call x264_pixel_sa8d_8x8_internal_%1 ; pix[0]
    add  r2, 8
    add  r0, 8
    mova m10, m0
    call x264_pixel_sa8d_8x8_internal_%1 ; pix[8]
    lea  r2, [r2+8*r3]
    lea  r0, [r0+8*r1]
    paddusw m10, m0
    call x264_pixel_sa8d_8x8_internal_%1 ; pix[8*stride+8]
    sub  r2, 8
    sub  r0, 8
    paddusw m10, m0
    call x264_pixel_sa8d_8x8_internal_%1 ; pix[8*stride]
    paddusw m0, m10
    HADDUW m0, m1
    movd eax, m0
    add  eax, 1
    shr  eax, 1
    RET

%else ; ARCH_X86_32
%ifnidn %1, mmxext
cglobal x264_pixel_sa8d_8x8_internal_%1
    %define spill0 [esp+4]
    %define spill1 [esp+20]
    %define spill2 [esp+36]
%ifidn %1, sse2
    LOAD_DIFF_8x4P 0, 1, 2, 3, 4, 5, 6, r0, r2, 1
    HADAMARD4_2D 0, 1, 2, 3, 4
    movdqa spill0, m3
    LOAD_DIFF_8x4P 4, 5, 6, 7, 3, 3, 2, r0, r2, 1
    HADAMARD4_2D 4, 5, 6, 7, 3
    HADAMARD2_2D 0, 4, 1, 5, 3, qdq, amax
    movdqa m3, spill0
    paddw m0, m1
    HADAMARD2_2D 2, 6, 3, 7, 5, qdq, amax
%else ; non-sse2
    mova m7, [hmul_8p GLOBAL]
    LOAD_SUMSUB_8x4P 0, 1, 2, 3, 5, 6, 7, r0, r2, 1
    ; could do first HADAMARD4_V here to save spilling later
    ; surprisingly, not a win on conroe or even p4
    mova spill0, m2
    mova spill1, m3
    mova spill2, m1
    SWAP 1, 7
    LOAD_SUMSUB_8x4P 4, 5, 6, 7, 2, 3, 1, r0, r2, 1
    HADAMARD4_V m4, m5, m6, m7, m3
    mova m1, spill2
    mova m2, spill0
    mova m3, spill1
    mova spill0, m6
    mova spill1, m7
    HADAMARD4_V m0, m1, m2, m3, m7
    SUMSUB_BADC m0, m4, m1, m5, m7
    HADAMARD 2, sumsub, 0, 4, 7, 6
    HADAMARD 2, sumsub, 1, 5, 7, 6
    HADAMARD 1, amax, 0, 4, 7, 6
    HADAMARD 1, amax, 1, 5, 7, 6
    mova m6, spill0
    mova m7, spill1
    paddw m0, m1
    SUMSUB_BADC m2, m6, m3, m7, m4
    HADAMARD 2, sumsub, 2, 6, 4, 5
    HADAMARD 2, sumsub, 3, 7, 4, 5
    HADAMARD 1, amax, 2, 6, 4, 5
    HADAMARD 1, amax, 3, 7, 4, 5
%endif ; sse2/non-sse2
    paddw m0, m2
    paddw m0, m3
    ret
%endif ; ifndef mmxext

cglobal x264_pixel_sa8d_8x8_%1, 4,7
    mov  r6, esp
    and  esp, ~15
    sub  esp, 48
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    call x264_pixel_sa8d_8x8_internal_%1
    HADDW m0, m1
    movd eax, m0
    add  eax, 1
    shr  eax, 1
    mov  esp, r6
    RET

cglobal x264_pixel_sa8d_16x16_%1, 4,7
    mov  r6, esp
    and  esp, ~15
    sub  esp, 64
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    call x264_pixel_sa8d_8x8_internal_%1
%ifidn %1, mmxext
    lea  r0, [r0+4*r1]
    lea  r2, [r2+4*r3]
%endif
    mova [esp+48], m0
    call x264_pixel_sa8d_8x8_internal_%1
    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 8
    add  r2, 8
    paddusw m0, [esp+48]
    mova [esp+48], m0
    call x264_pixel_sa8d_8x8_internal_%1
%ifidn %1, mmxext
    lea  r0, [r0+4*r1]
    lea  r2, [r2+4*r3]
%endif
%if mmsize == 16
    paddusw m0, [esp+48]
%endif
    mova [esp+64-mmsize], m0
    call x264_pixel_sa8d_8x8_internal_%1
    paddusw m0, [esp+64-mmsize]
%if mmsize == 16
    HADDUW m0, m1
%else
    mova m2, [esp+48]
    pxor m7, m7
    mova m1, m0
    mova m3, m2
    punpcklwd m0, m7
    punpckhwd m1, m7
    punpcklwd m2, m7
    punpckhwd m3, m7
    paddd m0, m1
    paddd m2, m3
    paddd m0, m2
    HADDD m0, m1
%endif
    movd eax, m0
    add  eax, 1
    shr  eax, 1
    mov  esp, r6
    RET
%endif ; !ARCH_X86_64
%endmacro ; SA8D

;=============================================================================
; INTRA SATD
;=============================================================================

%macro INTRA_SA8D_SSE2 1
%ifdef ARCH_X86_64
INIT_XMM
;-----------------------------------------------------------------------------
; void x264_intra_sa8d_x3_8x8_core_sse2( uint8_t *fenc, int16_t edges[2][8], int *res )
;-----------------------------------------------------------------------------
cglobal x264_intra_sa8d_x3_8x8_core_%1, 3,3,16
    ; 8x8 hadamard
    pxor        m8, m8
    movq        m0, [r0+0*FENC_STRIDE]
    movq        m1, [r0+1*FENC_STRIDE]
    movq        m2, [r0+2*FENC_STRIDE]
    movq        m3, [r0+3*FENC_STRIDE]
    movq        m4, [r0+4*FENC_STRIDE]
    movq        m5, [r0+5*FENC_STRIDE]
    movq        m6, [r0+6*FENC_STRIDE]
    movq        m7, [r0+7*FENC_STRIDE]
    punpcklbw   m0, m8
    punpcklbw   m1, m8
    punpcklbw   m2, m8
    punpcklbw   m3, m8
    punpcklbw   m4, m8
    punpcklbw   m5, m8
    punpcklbw   m6, m8
    punpcklbw   m7, m8

    HADAMARD8_2D 0,  1,  2,  3,  4,  5,  6,  7,  8

    ; dc
    movzx       r0d, word [r1+0]
    add         r0w, word [r1+16]
    add         r0d, 8
    and         r0d, -16
    shl         r0d, 2

    pxor        m15, m15
    movdqa      m8,  m2
    movdqa      m9,  m3
    movdqa      m10, m4
    movdqa      m11, m5
    ABS4        m8, m9, m10, m11, m12, m13
    paddusw     m8,  m10
    paddusw     m9,  m11
%ifidn %1, ssse3
    pabsw       m10, m6
    pabsw       m11, m7
    pabsw       m15, m1
%else
    movdqa      m10, m6
    movdqa      m11, m7
    movdqa      m15, m1
    ABS2        m10, m11, m13, m14
    ABS1        m15, m13
%endif
    paddusw     m10, m11
    paddusw     m8,  m9
    paddusw     m15, m10
    paddusw     m15, m8
    movdqa      m14, m15 ; 7x8 sum

    movdqa      m8,  [r1+0] ; left edge
    movd        m9,  r0d
    psllw       m8,  3
    psubw       m8,  m0
    psubw       m9,  m0
    ABS1        m8,  m10
    ABS1        m9,  m11 ; 1x8 sum
    paddusw     m14, m8
    paddusw     m15, m9
    punpcklwd   m0,  m1
    punpcklwd   m2,  m3
    punpcklwd   m4,  m5
    punpcklwd   m6,  m7
    punpckldq   m0,  m2
    punpckldq   m4,  m6
    punpcklqdq  m0,  m4 ; transpose
    movdqa      m1,  [r1+16] ; top edge
    movdqa      m2,  m15
    psllw       m1,  3
    psrldq      m2,  2     ; 8x7 sum
    psubw       m0,  m1  ; 8x1 sum
    ABS1        m0,  m1
    paddusw     m2,  m0

    ; 3x HADDW
    movdqa      m7,  [pw_1 GLOBAL]
    pmaddwd     m2,  m7
    pmaddwd     m14, m7
    pmaddwd     m15, m7
    movdqa      m3,  m2
    punpckldq   m2,  m14
    punpckhdq   m3,  m14
    pshufd      m5,  m15, 0xf5
    paddd       m2,  m3
    paddd       m5,  m15
    movdqa      m3,  m2
    punpcklqdq  m2,  m5
    punpckhqdq  m3,  m5
    pavgw       m3,  m2
    pxor        m0,  m0
    pavgw       m3,  m0
    movq      [r2],  m3 ; i8x8_v, i8x8_h
    psrldq      m3,  8
    movd    [r2+8],  m3 ; i8x8_dc
    RET
%endif ; ARCH_X86_64
%endmacro ; INTRA_SA8D_SSE2

; in: r0 = fenc
; out: m0..m3 = hadamard coefs
INIT_MMX
ALIGN 16
load_hadamard:
    pxor        m7, m7
    movd        m0, [r0+0*FENC_STRIDE]
    movd        m1, [r0+1*FENC_STRIDE]
    movd        m2, [r0+2*FENC_STRIDE]
    movd        m3, [r0+3*FENC_STRIDE]
    punpcklbw   m0, m7
    punpcklbw   m1, m7
    punpcklbw   m2, m7
    punpcklbw   m3, m7
    HADAMARD4_2D 0, 1, 2, 3, 4
    SAVE_MM_PERMUTATION load_hadamard
    ret

%macro SCALAR_SUMSUB 4
    add %1, %2
    add %3, %4
    add %2, %2
    add %4, %4
    sub %2, %1
    sub %4, %3
%endmacro

%macro SCALAR_HADAMARD_LEFT 5 ; y, 4x tmp
%ifnidn %1, 0
    shl         %1d, 5 ; log(FDEC_STRIDE)
%endif
    movzx       %2d, byte [r1+%1-1+0*FDEC_STRIDE]
    movzx       %3d, byte [r1+%1-1+1*FDEC_STRIDE]
    movzx       %4d, byte [r1+%1-1+2*FDEC_STRIDE]
    movzx       %5d, byte [r1+%1-1+3*FDEC_STRIDE]
%ifnidn %1, 0
    shr         %1d, 5
%endif
    SCALAR_SUMSUB %2d, %3d, %4d, %5d
    SCALAR_SUMSUB %2d, %4d, %3d, %5d
    mov         [left_1d+2*%1+0], %2w
    mov         [left_1d+2*%1+2], %3w
    mov         [left_1d+2*%1+4], %4w
    mov         [left_1d+2*%1+6], %5w
%endmacro

%macro SCALAR_HADAMARD_TOP 5 ; x, 4x tmp
    movzx       %2d, byte [r1+%1-FDEC_STRIDE+0]
    movzx       %3d, byte [r1+%1-FDEC_STRIDE+1]
    movzx       %4d, byte [r1+%1-FDEC_STRIDE+2]
    movzx       %5d, byte [r1+%1-FDEC_STRIDE+3]
    SCALAR_SUMSUB %2d, %3d, %4d, %5d
    SCALAR_SUMSUB %2d, %4d, %3d, %5d
    mov         [top_1d+2*%1+0], %2w
    mov         [top_1d+2*%1+2], %3w
    mov         [top_1d+2*%1+4], %4w
    mov         [top_1d+2*%1+6], %5w
%endmacro

%macro SUM_MM_X3 8 ; 3x sum, 4x tmp, op
    pxor        %7, %7
    pshufw      %4, %1, 01001110b
    pshufw      %5, %2, 01001110b
    pshufw      %6, %3, 01001110b
    paddw       %1, %4
    paddw       %2, %5
    paddw       %3, %6
    punpcklwd   %1, %7
    punpcklwd   %2, %7
    punpcklwd   %3, %7
    pshufw      %4, %1, 01001110b
    pshufw      %5, %2, 01001110b
    pshufw      %6, %3, 01001110b
    %8          %1, %4
    %8          %2, %5
    %8          %3, %6
%endmacro

%macro CLEAR_SUMS 0
%ifdef ARCH_X86_64
    mov   qword [sums+0], 0
    mov   qword [sums+8], 0
    mov   qword [sums+16], 0
%else
    pxor  m7, m7
    movq  [sums+0], m7
    movq  [sums+8], m7
    movq  [sums+16], m7
%endif
%endmacro

; in: m1..m3
; out: m7
; clobber: m4..m6
%macro SUM3x4 1
%ifidn %1, ssse3
    pabsw       m4, m1
    pabsw       m5, m2
    pabsw       m7, m3
    paddw       m4, m5
%else
    movq        m4, m1
    movq        m5, m2
    ABS2        m4, m5, m6, m7
    movq        m7, m3
    paddw       m4, m5
    ABS1        m7, m6
%endif
    paddw       m7, m4
%endmacro

; in: m0..m3 (4x4), m7 (3x4)
; out: m0 v, m4 h, m5 dc
; clobber: m6
%macro SUM4x3 3 ; dc, left, top
    movq        m4, %2
    movd        m5, %1
    psllw       m4, 2
    psubw       m4, m0
    psubw       m5, m0
    punpcklwd   m0, m1
    punpcklwd   m2, m3
    punpckldq   m0, m2 ; transpose
    movq        m1, %3
    psllw       m1, 2
    psubw       m0, m1
    ABS2        m4, m5, m2, m3 ; 1x4 sum
    ABS1        m0, m1 ; 4x1 sum
%endmacro

%macro INTRA_SATDS_MMX 1
INIT_MMX
;-----------------------------------------------------------------------------
; void x264_intra_satd_x3_4x4_mmxext( uint8_t *fenc, uint8_t *fdec, int *res )
;-----------------------------------------------------------------------------
cglobal x264_intra_satd_x3_4x4_%1, 2,6
%ifdef ARCH_X86_64
    ; stack is 16 byte aligned because abi says so
    %define  top_1d  rsp-8  ; size 8
    %define  left_1d rsp-16 ; size 8
    %define  t0 r10
%else
    ; stack is 16 byte aligned at least in gcc, and we've pushed 3 regs + return address, so it's still aligned
    SUB         esp, 16
    %define  top_1d  esp+8
    %define  left_1d esp
    %define  t0 r2
%endif

    call load_hadamard
    SCALAR_HADAMARD_LEFT 0, r0, r3, r4, r5
    mov         t0d, r0d
    SCALAR_HADAMARD_TOP  0, r0, r3, r4, r5
    lea         t0d, [t0d + r0d + 4]
    and         t0d, -8
    shl         t0d, 1 ; dc

    SUM3x4 %1
    SUM4x3 t0d, [left_1d], [top_1d]
    paddw       m4, m7
    paddw       m5, m7
    movq        m1, m5
    psrlq       m1, 16  ; 4x3 sum
    paddw       m0, m1

    SUM_MM_X3   m0, m4, m5, m1, m2, m3, m6, pavgw
%ifndef ARCH_X86_64
    mov         r2,  r2mp
%endif
    movd        [r2+0], m0 ; i4x4_v satd
    movd        [r2+4], m4 ; i4x4_h satd
    movd        [r2+8], m5 ; i4x4_dc satd
%ifndef ARCH_X86_64
    ADD         esp, 16
%endif
    RET

%ifdef ARCH_X86_64
    %define  t0 r10
    %define  t2 r11
%else
    %define  t0 r0
    %define  t2 r2
%endif

;-----------------------------------------------------------------------------
; void x264_intra_satd_x3_16x16_mmxext( uint8_t *fenc, uint8_t *fdec, int *res )
;-----------------------------------------------------------------------------
cglobal x264_intra_satd_x3_16x16_%1, 0,7
%ifdef ARCH_X86_64
    %assign  stack_pad  88
%else
    %assign  stack_pad  88 + ((stack_offset+88+4)&15)
%endif
    ; not really needed on x86_64, just shuts up valgrind about storing data below the stack across a function call
    SUB         rsp, stack_pad
%define sums    rsp+64 ; size 24
%define top_1d  rsp+32 ; size 32
%define left_1d rsp    ; size 32
    movifnidn   r1,  r1mp
    CLEAR_SUMS

    ; 1D hadamards
    xor         t2d, t2d
    mov         t0d, 12
.loop_edge:
    SCALAR_HADAMARD_LEFT t0, r3, r4, r5, r6
    add         t2d, r3d
    SCALAR_HADAMARD_TOP  t0, r3, r4, r5, r6
    add         t2d, r3d
    sub         t0d, 4
    jge .loop_edge
    shr         t2d, 1
    add         t2d, 8
    and         t2d, -16 ; dc

    ; 2D hadamards
    movifnidn   r0,  r0mp
    xor         r3d, r3d
.loop_y:
    xor         r4d, r4d
.loop_x:
    call load_hadamard

    SUM3x4 %1
    SUM4x3 t2d, [left_1d+8*r3], [top_1d+8*r4]
    pavgw       m4, m7
    pavgw       m5, m7
    paddw       m0, [sums+0]  ; i16x16_v satd
    paddw       m4, [sums+8]  ; i16x16_h satd
    paddw       m5, [sums+16] ; i16x16_dc satd
    movq        [sums+0], m0
    movq        [sums+8], m4
    movq        [sums+16], m5

    add         r0, 4
    inc         r4d
    cmp         r4d, 4
    jl  .loop_x
    add         r0, 4*FENC_STRIDE-16
    inc         r3d
    cmp         r3d, 4
    jl  .loop_y

; horizontal sum
    movifnidn   r2, r2mp
    movq        m2, [sums+16]
    movq        m1, [sums+8]
    movq        m0, [sums+0]
    movq        m7, m2
    SUM_MM_X3   m0, m1, m2, m3, m4, m5, m6, paddd
    psrld       m0, 1
    pslld       m7, 16
    psrld       m7, 16
    paddd       m0, m2
    psubd       m0, m7
    movd        [r2+8], m2 ; i16x16_dc satd
    movd        [r2+4], m1 ; i16x16_h satd
    movd        [r2+0], m0 ; i16x16_v satd
    ADD         rsp, stack_pad
    RET

;-----------------------------------------------------------------------------
; void x264_intra_satd_x3_8x8c_mmxext( uint8_t *fenc, uint8_t *fdec, int *res )
;-----------------------------------------------------------------------------
cglobal x264_intra_satd_x3_8x8c_%1, 0,6
    ; not really needed on x86_64, just shuts up valgrind about storing data below the stack across a function call
    SUB          rsp, 72
%define  sums    rsp+48 ; size 24
%define  dc_1d   rsp+32 ; size 16
%define  top_1d  rsp+16 ; size 16
%define  left_1d rsp    ; size 16
    movifnidn   r1,  r1mp
    CLEAR_SUMS

    ; 1D hadamards
    mov         t0d, 4
.loop_edge:
    SCALAR_HADAMARD_LEFT t0, t2, r3, r4, r5
    SCALAR_HADAMARD_TOP  t0, t2, r3, r4, r5
    sub         t0d, 4
    jge .loop_edge

    ; dc
    movzx       t2d, word [left_1d+0]
    movzx       r3d, word [top_1d+0]
    movzx       r4d, word [left_1d+8]
    movzx       r5d, word [top_1d+8]
    add         t2d, r3d
    lea         r3, [r4 + r5]
    lea         t2, [2*t2 + 8]
    lea         r3, [2*r3 + 8]
    lea         r4, [4*r4 + 8]
    lea         r5, [4*r5 + 8]
    and         t2d, -16 ; tl
    and         r3d, -16 ; br
    and         r4d, -16 ; bl
    and         r5d, -16 ; tr
    mov         [dc_1d+ 0], t2d ; tl
    mov         [dc_1d+ 4], r5d ; tr
    mov         [dc_1d+ 8], r4d ; bl
    mov         [dc_1d+12], r3d ; br
    lea         r5, [dc_1d]

    ; 2D hadamards
    movifnidn   r0,  r0mp
    movifnidn   r2,  r2mp
    xor         r3d, r3d
.loop_y:
    xor         r4d, r4d
.loop_x:
    call load_hadamard

    SUM3x4 %1
    SUM4x3 [r5+4*r4], [left_1d+8*r3], [top_1d+8*r4]
    pavgw       m4, m7
    pavgw       m5, m7
    paddw       m0, [sums+16] ; i4x4_v satd
    paddw       m4, [sums+8]  ; i4x4_h satd
    paddw       m5, [sums+0]  ; i4x4_dc satd
    movq        [sums+16], m0
    movq        [sums+8], m4
    movq        [sums+0], m5

    add         r0, 4
    inc         r4d
    cmp         r4d, 2
    jl  .loop_x
    add         r0, 4*FENC_STRIDE-8
    add         r5, 8
    inc         r3d
    cmp         r3d, 2
    jl  .loop_y

; horizontal sum
    movq        m0, [sums+0]
    movq        m1, [sums+8]
    movq        m2, [sums+16]
    movq        m7, m0
    psrlq       m7, 15
    paddw       m2, m7
    SUM_MM_X3   m0, m1, m2, m3, m4, m5, m6, paddd
    psrld       m2, 1
    movd        [r2+0], m0 ; i8x8c_dc satd
    movd        [r2+4], m1 ; i8x8c_h satd
    movd        [r2+8], m2 ; i8x8c_v satd
    ADD         rsp, 72
    RET
%endmacro ; INTRA_SATDS_MMX


%macro ABS_MOV_SSSE3 2
    pabsw   %1, %2
%endmacro

%macro ABS_MOV_MMX 2
    pxor    %1, %1
    psubw   %1, %2
    pmaxsw  %1, %2
%endmacro

%define ABS_MOV ABS_MOV_MMX

; in:  r0=pix, r1=stride, r2=stride*3, r3=tmp, m6=mask_ac4, m7=0
; out: [tmp]=hadamard4, m0=satd
cglobal x264_hadamard_ac_4x4_mmxext
    movh      m0, [r0]
    movh      m1, [r0+r1]
    movh      m2, [r0+r1*2]
    movh      m3, [r0+r2]
    punpcklbw m0, m7
    punpcklbw m1, m7
    punpcklbw m2, m7
    punpcklbw m3, m7
    HADAMARD4_2D 0, 1, 2, 3, 4
    mova [r3],    m0
    mova [r3+8],  m1
    mova [r3+16], m2
    mova [r3+24], m3
    ABS1      m0, m4
    ABS1      m1, m4
    pand      m0, m6
    ABS1      m2, m4
    ABS1      m3, m4
    paddw     m0, m1
    paddw     m2, m3
    paddw     m0, m2
    SAVE_MM_PERMUTATION x264_hadamard_ac_4x4_mmxext
    ret

cglobal x264_hadamard_ac_2x2max_mmxext
    mova      m0, [r3+0x00]
    mova      m1, [r3+0x20]
    mova      m2, [r3+0x40]
    mova      m3, [r3+0x60]
    sub       r3, 8
    SUMSUB_BADC m0, m1, m2, m3, m4
    ABS4 m0, m2, m1, m3, m4, m5
    HADAMARD 0, max, 0, 2, 4, 5
    HADAMARD 0, max, 1, 3, 4, 5
    paddw     m7, m0
    paddw     m7, m1
    SAVE_MM_PERMUTATION x264_hadamard_ac_2x2max_mmxext
    ret

cglobal x264_hadamard_ac_8x8_mmxext
    mova      m6, [mask_ac4 GLOBAL]
    pxor      m7, m7
    call x264_hadamard_ac_4x4_mmxext
    add       r0, 4
    add       r3, 32
    mova      m5, m0
    call x264_hadamard_ac_4x4_mmxext
    lea       r0, [r0+4*r1]
    add       r3, 64
    paddw     m5, m0
    call x264_hadamard_ac_4x4_mmxext
    sub       r0, 4
    sub       r3, 32
    paddw     m5, m0
    call x264_hadamard_ac_4x4_mmxext
    paddw     m5, m0
    sub       r3, 40
    mova [rsp+gprsize+8], m5 ; save satd
%rep 3
    call x264_hadamard_ac_2x2max_mmxext
%endrep
    mova      m0, [r3+0x00]
    mova      m1, [r3+0x20]
    mova      m2, [r3+0x40]
    mova      m3, [r3+0x60]
    SUMSUB_BADC m0, m1, m2, m3, m4
    HADAMARD 0, sumsub, 0, 2, 4, 5
    ABS4 m1, m3, m0, m2, m4, m5
    HADAMARD 0, max, 1, 3, 4, 5
    pand      m6, m0
    paddw     m7, m1
    paddw     m6, m2
    paddw     m7, m7
    paddw     m6, m7
    mova [rsp+gprsize], m6 ; save sa8d
    SWAP      m0, m6
    SAVE_MM_PERMUTATION x264_hadamard_ac_8x8_mmxext
    ret

%macro HADAMARD_AC_WXH_MMX 2
cglobal x264_pixel_hadamard_ac_%1x%2_mmxext, 2,4
    %assign pad 16-gprsize-(stack_offset&15)
    %define ysub r1
    sub  rsp, 16+128+pad
    lea  r2, [r1*3]
    lea  r3, [rsp+16]
    call x264_hadamard_ac_8x8_mmxext
%if %2==16
    %define ysub r2
    lea  r0, [r0+r1*4]
    sub  rsp, 16
    call x264_hadamard_ac_8x8_mmxext
%endif
%if %1==16
    neg  ysub
    sub  rsp, 16
    lea  r0, [r0+ysub*4+8]
    neg  ysub
    call x264_hadamard_ac_8x8_mmxext
%if %2==16
    lea  r0, [r0+r1*4]
    sub  rsp, 16
    call x264_hadamard_ac_8x8_mmxext
%endif
%endif
    mova    m1, [rsp+0x08]
%if %1*%2 >= 128
    paddusw m0, [rsp+0x10]
    paddusw m1, [rsp+0x18]
%endif
%if %1*%2 == 256
    mova    m2, [rsp+0x20]
    paddusw m1, [rsp+0x28]
    paddusw m2, [rsp+0x30]
    mova    m3, m0
    paddusw m1, [rsp+0x38]
    pxor    m3, m2
    pand    m3, [pw_1 GLOBAL]
    pavgw   m0, m2
    psubusw m0, m3
    HADDUW  m0, m2
%else
    psrlw m0, 1
    HADDW m0, m2
%endif
    psrlw m1, 1
    HADDW m1, m3
    movd edx, m0
    movd eax, m1
    shr  edx, 1
%ifdef ARCH_X86_64
    shl  rdx, 32
    add  rax, rdx
%endif
    add  rsp, 128+%1*%2/4+pad
    RET
%endmacro ; HADAMARD_AC_WXH_MMX

HADAMARD_AC_WXH_MMX 16, 16
HADAMARD_AC_WXH_MMX  8, 16
HADAMARD_AC_WXH_MMX 16,  8
HADAMARD_AC_WXH_MMX  8,  8

%macro LOAD_INC_8x4W_SSE2 5
    movh      m%1, [r0]
    movh      m%2, [r0+r1]
    movh      m%3, [r0+r1*2]
    movh      m%4, [r0+r2]
%ifidn %1, 0
    lea       r0, [r0+r1*4]
%endif
    punpcklbw m%1, m%5
    punpcklbw m%2, m%5
    punpcklbw m%3, m%5
    punpcklbw m%4, m%5
%endmacro

%macro LOAD_INC_8x4W_SSSE3 5
    LOAD_DUP_4x8P %3, %4, %1, %2, [r0+r1*2], [r0+r2], [r0], [r0+r1]
%ifidn %1, 0
    lea       r0, [r0+r1*4]
%endif
    HSUMSUB %1, %2, %3, %4, %5
%endmacro

%macro HADAMARD_AC_SSE2 1
INIT_XMM
; in:  r0=pix, r1=stride, r2=stride*3
; out: [esp+16]=sa8d, [esp+32]=satd, r0+=stride*4
cglobal x264_hadamard_ac_8x8_%1
%ifdef ARCH_X86_64
    %define spill0 m8
    %define spill1 m9
    %define spill2 m10
%else
    %define spill0 [rsp+gprsize]
    %define spill1 [rsp+gprsize+16]
    %define spill2 [rsp+gprsize+32]
%endif
%ifnidn %1, sse2
    ;LOAD_INC loads sumsubs
    mova      m7, [hmul_8p GLOBAL]
%else
    ;LOAD_INC only unpacks to words
    pxor      m7, m7
%endif
    LOAD_INC_8x4W 0, 1, 2, 3, 7
%ifidn %1, sse2
    HADAMARD4_2D_SSE 0, 1, 2, 3, 4
%else
    HADAMARD4_V m0, m1, m2, m3, m4
%endif
    mova  spill0, m1
    SWAP 1, 7
    LOAD_INC_8x4W 4, 5, 6, 7, 1
%ifidn %1, sse2
    HADAMARD4_2D_SSE 4, 5, 6, 7, 1
%else
    HADAMARD4_V m4, m5, m6, m7, m1
%endif

%ifnidn %1, sse2
    mova      m1, spill0
    mova      spill0, m6
    mova      spill1, m7
    HADAMARD 1, sumsub, 0, 1, 6, 7
    HADAMARD 1, sumsub, 2, 3, 6, 7
    mova      m6, spill0
    mova      m7, spill1
    mova      spill0, m1
    mova      spill1, m0
    HADAMARD 1, sumsub, 4, 5, 1, 0
    HADAMARD 1, sumsub, 6, 7, 1, 0
    mova      m0, spill1
%endif

    mova  spill1, m2
    mova  spill2, m3
    ABS_MOV   m1, m0
    ABS_MOV   m2, m4
    ABS_MOV   m3, m5
    paddw     m1, m2
    SUMSUB_BA m0, m4; m2
%ifnidn %1, sse2
    pand      m1, [mask_ac4b GLOBAL]
%else
    pand      m1, [mask_ac4 GLOBAL]
%endif
    ABS_MOV   m2, spill0
    paddw     m1, m3
    ABS_MOV   m3, spill1
    paddw     m1, m2
    ABS_MOV   m2, spill2
    paddw     m1, m3
    ABS_MOV   m3, m6
    paddw     m1, m2
    ABS_MOV   m2, m7
    paddw     m1, m3
    mova      m3, m7
    paddw     m1, m2
    mova      m2, m6
    psubw     m7, spill2
    paddw     m3, spill2
    mova  [rsp+gprsize+32], m1 ; save satd
    mova      m1, m5
    psubw     m6, spill1
    paddw     m2, spill1
    psubw     m5, spill0
    paddw     m1, spill0
%ifnidn %1, sse2
    mova  spill1, m4
    HADAMARD 2, amax, 3, 7, 4
    HADAMARD 2, amax, 2, 6, 7, 4
    mova m4, spill1
    HADAMARD 2, amax, 1, 5, 6, 7
    HADAMARD 2, sumsub, 0, 4, 5, 6
%else
    mova  spill1, m4
    HADAMARD 4, amax, 3, 7, 4
    HADAMARD 4, amax, 2, 6, 7, 4
    mova m4, spill1
    HADAMARD 4, amax, 1, 5, 6, 7
    HADAMARD 4, sumsub, 0, 4, 5, 6
%endif
    paddw m2, m3
    paddw m2, m1
    paddw m2, m2
    ABS1      m4, m7
    pand      m0, [mask_ac8 GLOBAL]
    ABS1      m0, m7
    paddw m2, m4
    paddw m0, m2
    mova  [rsp+gprsize+16], m0 ; save sa8d
    SAVE_MM_PERMUTATION x264_hadamard_ac_8x8_%1
    ret

HADAMARD_AC_WXH_SSE2 16, 16, %1
HADAMARD_AC_WXH_SSE2  8, 16, %1
HADAMARD_AC_WXH_SSE2 16,  8, %1
HADAMARD_AC_WXH_SSE2  8,  8, %1
%endmacro ; HADAMARD_AC_SSE2

; struct { int satd, int sa8d; } x264_pixel_hadamard_ac_16x16( uint8_t *pix, int stride )
%macro HADAMARD_AC_WXH_SSE2 3
cglobal x264_pixel_hadamard_ac_%1x%2_%3, 2,3,11
    %assign pad 16-gprsize-(stack_offset&15)
    %define ysub r1
    sub  rsp, 48+pad
    lea  r2, [r1*3]
    call x264_hadamard_ac_8x8_%3
%if %2==16
    %define ysub r2
    lea  r0, [r0+r1*4]
    sub  rsp, 32
    call x264_hadamard_ac_8x8_%3
%endif
%if %1==16
    neg  ysub
    sub  rsp, 32
    lea  r0, [r0+ysub*4+8]
    neg  ysub
    call x264_hadamard_ac_8x8_%3
%if %2==16
    lea  r0, [r0+r1*4]
    sub  rsp, 32
    call x264_hadamard_ac_8x8_%3
%endif
%endif
    mova    m1, [rsp+0x20]
%if %1*%2 >= 128
    paddusw m0, [rsp+0x30]
    paddusw m1, [rsp+0x40]
%endif
%if %1*%2 == 256
    paddusw m0, [rsp+0x50]
    paddusw m1, [rsp+0x60]
    paddusw m0, [rsp+0x70]
    paddusw m1, [rsp+0x80]
    psrlw m0, 1
%endif
    HADDW m0, m2
    HADDW m1, m3
    movd edx, m0
    movd eax, m1
    shr  edx, 2 - (%1*%2 >> 8)
    shr  eax, 1
%ifdef ARCH_X86_64
    shl  rdx, 32
    add  rax, rdx
%endif
    add  rsp, 16+%1*%2/2+pad
    RET
%endmacro ; HADAMARD_AC_WXH_SSE2

; instantiate satds

%ifndef ARCH_X86_64
cextern x264_pixel_sa8d_8x8_internal_mmxext
SA8D mmxext
%endif

%define TRANS TRANS_SSE2
%define ABS1 ABS1_MMX
%define ABS2 ABS2_MMX
%define DIFFOP DIFF_UNPACK_SSE2
%define JDUP JDUP_SSE2
%define LOAD_INC_8x4W LOAD_INC_8x4W_SSE2
%define LOAD_SUMSUB_8x4P LOAD_DIFF_8x4P
%define LOAD_SUMSUB_16P  LOAD_SUMSUB_16P_SSE2
%define movdqa movaps ; doesn't hurt pre-nehalem, might as well save size
%define movdqu movups
%define punpcklqdq movlhps
INIT_XMM
SA8D sse2
SATDS_SSE2 sse2
INTRA_SA8D_SSE2 sse2
INTRA_SATDS_MMX mmxext
HADAMARD_AC_SSE2 sse2

%define ABS1 ABS1_SSSE3
%define ABS2 ABS2_SSSE3
%define ABS_MOV ABS_MOV_SSSE3
%define DIFFOP DIFF_SUMSUB_SSSE3
%define JDUP JDUP_CONROE
%define LOAD_DUP_4x8P LOAD_DUP_4x8P_CONROE
%define LOAD_INC_8x4W LOAD_INC_8x4W_SSSE3
%define LOAD_SUMSUB_8x4P LOAD_SUMSUB_8x4P_SSSE3
%define LOAD_SUMSUB_16P  LOAD_SUMSUB_16P_SSSE3
SATDS_SSE2 ssse3
SA8D ssse3
HADAMARD_AC_SSE2 ssse3
%undef movdqa ; nehalem doesn't like movaps
%undef movdqu ; movups
%undef punpcklqdq ; or movlhps
INTRA_SA8D_SSE2 ssse3
INTRA_SATDS_MMX ssse3

%define TRANS TRANS_SSE4
%define JDUP JDUP_PENRYN
%define LOAD_DUP_4x8P LOAD_DUP_4x8P_PENRYN
SATDS_SSE2 sse4
SA8D sse4
HADAMARD_AC_SSE2 sse4

;=============================================================================
; SSIM
;=============================================================================

;-----------------------------------------------------------------------------
; void x264_pixel_ssim_4x4x2_core_sse2( const uint8_t *pix1, int stride1,
;                                       const uint8_t *pix2, int stride2, int sums[2][4] )
;-----------------------------------------------------------------------------
cglobal x264_pixel_ssim_4x4x2_core_sse2, 4,4,8
    pxor      m0, m0
    pxor      m1, m1
    pxor      m2, m2
    pxor      m3, m3
    pxor      m4, m4
%rep 4
    movq      m5, [r0]
    movq      m6, [r2]
    punpcklbw m5, m0
    punpcklbw m6, m0
    paddw     m1, m5
    paddw     m2, m6
    movdqa    m7, m5
    pmaddwd   m5, m5
    pmaddwd   m7, m6
    pmaddwd   m6, m6
    paddd     m3, m5
    paddd     m4, m7
    paddd     m3, m6
    add       r0, r1
    add       r2, r3
%endrep
    ; PHADDW m1, m2
    ; PHADDD m3, m4
    movdqa    m7, [pw_1 GLOBAL]
    pshufd    m5, m3, 0xb1
    pmaddwd   m1, m7
    pmaddwd   m2, m7
    pshufd    m6, m4, 0xb1
    packssdw  m1, m2
    paddd     m3, m5
    pshufd    m1, m1, 0xd8
    paddd     m4, m6
    pmaddwd   m1, m7
    movdqa    m5, m3
    punpckldq m3, m4
    punpckhdq m5, m4

%ifdef UNIX64
    %define t0 r4
%else
    %define t0 rax
    mov t0, r4mp
%endif

    movq      [t0+ 0], m1
    movq      [t0+ 8], m3
    psrldq    m1, 8
    movq      [t0+16], m1
    movq      [t0+24], m5
    RET

;-----------------------------------------------------------------------------
; float x264_pixel_ssim_end_sse2( int sum0[5][4], int sum1[5][4], int width )
;-----------------------------------------------------------------------------
cglobal x264_pixel_ssim_end4_sse2, 3,3,7
    movdqa    m0, [r0+ 0]
    movdqa    m1, [r0+16]
    movdqa    m2, [r0+32]
    movdqa    m3, [r0+48]
    movdqa    m4, [r0+64]
    paddd     m0, [r1+ 0]
    paddd     m1, [r1+16]
    paddd     m2, [r1+32]
    paddd     m3, [r1+48]
    paddd     m4, [r1+64]
    paddd     m0, m1
    paddd     m1, m2
    paddd     m2, m3
    paddd     m3, m4
    movdqa    m5, [ssim_c1 GLOBAL]
    movdqa    m6, [ssim_c2 GLOBAL]
    TRANSPOSE4x4D  0, 1, 2, 3, 4

;   s1=m0, s2=m1, ss=m2, s12=m3
    movdqa    m4, m1
    pslld     m1, 16
    pmaddwd   m4, m0  ; s1*s2
    por       m0, m1
    pmaddwd   m0, m0  ; s1*s1 + s2*s2
    pslld     m4, 1
    pslld     m3, 7
    pslld     m2, 6
    psubd     m3, m4  ; covar*2
    psubd     m2, m0  ; vars
    paddd     m0, m5
    paddd     m4, m5
    paddd     m3, m6
    paddd     m2, m6
    cvtdq2ps  m0, m0  ; (float)(s1*s1 + s2*s2 + ssim_c1)
    cvtdq2ps  m4, m4  ; (float)(s1*s2*2 + ssim_c1)
    cvtdq2ps  m3, m3  ; (float)(covar*2 + ssim_c2)
    cvtdq2ps  m2, m2  ; (float)(vars + ssim_c2)
    mulps     m4, m3
    mulps     m0, m2
    divps     m4, m0  ; ssim

    cmp       r2d, 4
    je .skip ; faster only if this is the common case; remove branch if we use ssim on a macroblock level
    neg       r2
%ifdef PIC
    lea       r3, [mask_ff + 16 GLOBAL]
    movdqu    m1, [r3 + r2*4]
%else
    movdqu    m1, [mask_ff + r2*4 + 16 GLOBAL]
%endif
    pand      m4, m1
.skip:
    movhlps   m0, m4
    addps     m0, m4
    pshuflw   m4, m0, 0xE
    addss     m0, m4
%ifndef ARCH_X86_64
    movd     r0m, m0
    fld     dword r0m
%endif
    RET



;=============================================================================
; Successive Elimination ADS
;=============================================================================

%macro ADS_START 1 ; unroll_size
%ifdef ARCH_X86_64
    %define t0 r6
%ifdef WIN64
    mov     r4,  r4mp
    movsxd  r5,  dword r5m
%endif
    mov     r10, rsp
%else
    %define t0 r4
    mov     rbp, rsp
%endif
    mov     r0d, r5m
    sub     rsp, r0
    sub     rsp, %1*4-1
    and     rsp, ~15
    mov     t0,  rsp
    shl     r2d,  1
%endmacro

%macro ADS_END 1
    add     r1, 8*%1
    add     r3, 8*%1
    add     t0, 4*%1
    sub     r0d, 4*%1
    jg .loop
%ifdef WIN64
    RESTORE_XMM r10
%endif
    jmp ads_mvs
%endmacro

%define ABS1 ABS1_MMX

;-----------------------------------------------------------------------------
; int x264_pixel_ads4_mmxext( int enc_dc[4], uint16_t *sums, int delta,
;                             uint16_t *cost_mvx, int16_t *mvs, int width, int thresh )
;-----------------------------------------------------------------------------
cglobal x264_pixel_ads4_mmxext, 4,7
    movq    mm6, [r0]
    movq    mm4, [r0+8]
    pshufw  mm7, mm6, 0
    pshufw  mm6, mm6, 0xAA
    pshufw  mm5, mm4, 0
    pshufw  mm4, mm4, 0xAA
    ADS_START 1
.loop:
    movq    mm0, [r1]
    movq    mm1, [r1+16]
    psubw   mm0, mm7
    psubw   mm1, mm6
    ABS1    mm0, mm2
    ABS1    mm1, mm3
    movq    mm2, [r1+r2]
    movq    mm3, [r1+r2+16]
    psubw   mm2, mm5
    psubw   mm3, mm4
    paddw   mm0, mm1
    ABS1    mm2, mm1
    ABS1    mm3, mm1
    paddw   mm0, mm2
    paddw   mm0, mm3
%ifdef WIN64
    pshufw  mm1, [r10+stack_offset+56], 0
%elifdef ARCH_X86_64
    pshufw  mm1, [r10+8], 0
%else
    pshufw  mm1, [ebp+stack_offset+28], 0
%endif
    paddusw mm0, [r3]
    psubusw mm1, mm0
    packsswb mm1, mm1
    movd    [t0], mm1
    ADS_END 1

cglobal x264_pixel_ads2_mmxext, 4,7
    movq    mm6, [r0]
    pshufw  mm5, r6m, 0
    pshufw  mm7, mm6, 0
    pshufw  mm6, mm6, 0xAA
    ADS_START 1
.loop:
    movq    mm0, [r1]
    movq    mm1, [r1+r2]
    psubw   mm0, mm7
    psubw   mm1, mm6
    ABS1    mm0, mm2
    ABS1    mm1, mm3
    paddw   mm0, mm1
    paddusw mm0, [r3]
    movq    mm4, mm5
    psubusw mm4, mm0
    packsswb mm4, mm4
    movd    [t0], mm4
    ADS_END 1

cglobal x264_pixel_ads1_mmxext, 4,7
    pshufw  mm7, [r0], 0
    pshufw  mm6, r6m, 0
    ADS_START 2
.loop:
    movq    mm0, [r1]
    movq    mm1, [r1+8]
    psubw   mm0, mm7
    psubw   mm1, mm7
    ABS1    mm0, mm2
    ABS1    mm1, mm3
    paddusw mm0, [r3]
    paddusw mm1, [r3+8]
    movq    mm4, mm6
    movq    mm5, mm6
    psubusw mm4, mm0
    psubusw mm5, mm1
    packsswb mm4, mm5
    movq    [t0], mm4
    ADS_END 2

%macro ADS_SSE2 1
cglobal x264_pixel_ads4_%1, 4,7,12
    movdqa  xmm4, [r0]
    pshuflw xmm7, xmm4, 0
    pshuflw xmm6, xmm4, 0xAA
    pshufhw xmm5, xmm4, 0
    pshufhw xmm4, xmm4, 0xAA
    punpcklqdq xmm7, xmm7
    punpcklqdq xmm6, xmm6
    punpckhqdq xmm5, xmm5
    punpckhqdq xmm4, xmm4
%ifdef ARCH_X86_64
    pshuflw xmm8, r6m, 0
    punpcklqdq xmm8, xmm8
    ADS_START 2
    movdqu  xmm10, [r1]
    movdqu  xmm11, [r1+r2]
.loop:
    movdqa  xmm0, xmm10
    movdqu  xmm1, [r1+16]
    movdqa  xmm10, xmm1
    psubw   xmm0, xmm7
    psubw   xmm1, xmm6
    ABS1    xmm0, xmm2
    ABS1    xmm1, xmm3
    movdqa  xmm2, xmm11
    movdqu  xmm3, [r1+r2+16]
    movdqa  xmm11, xmm3
    psubw   xmm2, xmm5
    psubw   xmm3, xmm4
    paddw   xmm0, xmm1
    movdqu  xmm9, [r3]
    ABS1    xmm2, xmm1
    ABS1    xmm3, xmm1
    paddw   xmm0, xmm2
    paddw   xmm0, xmm3
    paddusw xmm0, xmm9
    movdqa  xmm1, xmm8
    psubusw xmm1, xmm0
    packsswb xmm1, xmm1
    movq    [t0], xmm1
%else
    ADS_START 2
.loop:
    movdqu  xmm0, [r1]
    movdqu  xmm1, [r1+16]
    psubw   xmm0, xmm7
    psubw   xmm1, xmm6
    ABS1    xmm0, xmm2
    ABS1    xmm1, xmm3
    movdqu  xmm2, [r1+r2]
    movdqu  xmm3, [r1+r2+16]
    psubw   xmm2, xmm5
    psubw   xmm3, xmm4
    paddw   xmm0, xmm1
    ABS1    xmm2, xmm1
    ABS1    xmm3, xmm1
    paddw   xmm0, xmm2
    paddw   xmm0, xmm3
    movd    xmm1, [ebp+stack_offset+28]
    movdqu  xmm2, [r3]
    pshuflw xmm1, xmm1, 0
    punpcklqdq xmm1, xmm1
    paddusw xmm0, xmm2
    psubusw xmm1, xmm0
    packsswb xmm1, xmm1
    movq    [t0], xmm1
%endif ; ARCH
    ADS_END 2

cglobal x264_pixel_ads2_%1, 4,7,8
    movq    xmm6, [r0]
    movd    xmm5, r6m
    pshuflw xmm7, xmm6, 0
    pshuflw xmm6, xmm6, 0xAA
    pshuflw xmm5, xmm5, 0
    punpcklqdq xmm7, xmm7
    punpcklqdq xmm6, xmm6
    punpcklqdq xmm5, xmm5
    ADS_START 2
.loop:
    movdqu  xmm0, [r1]
    movdqu  xmm1, [r1+r2]
    psubw   xmm0, xmm7
    psubw   xmm1, xmm6
    movdqu  xmm4, [r3]
    ABS1    xmm0, xmm2
    ABS1    xmm1, xmm3
    paddw   xmm0, xmm1
    paddusw xmm0, xmm4
    movdqa  xmm1, xmm5
    psubusw xmm1, xmm0
    packsswb xmm1, xmm1
    movq    [t0], xmm1
    ADS_END 2

cglobal x264_pixel_ads1_%1, 4,7,8
    movd    xmm7, [r0]
    movd    xmm6, r6m
    pshuflw xmm7, xmm7, 0
    pshuflw xmm6, xmm6, 0
    punpcklqdq xmm7, xmm7
    punpcklqdq xmm6, xmm6
    ADS_START 4
.loop:
    movdqu  xmm0, [r1]
    movdqu  xmm1, [r1+16]
    psubw   xmm0, xmm7
    psubw   xmm1, xmm7
    movdqu  xmm2, [r3]
    movdqu  xmm3, [r3+16]
    ABS1    xmm0, xmm4
    ABS1    xmm1, xmm5
    paddusw xmm0, xmm2
    paddusw xmm1, xmm3
    movdqa  xmm4, xmm6
    movdqa  xmm5, xmm6
    psubusw xmm4, xmm0
    psubusw xmm5, xmm1
    packsswb xmm4, xmm5
    movdqa  [t0], xmm4
    ADS_END 4
%endmacro

ADS_SSE2 sse2
%define ABS1 ABS1_SSSE3
ADS_SSE2 ssse3

; int x264_pixel_ads_mvs( int16_t *mvs, uint8_t *masks, int width )
; {
;     int nmv=0, i, j;
;     *(uint32_t*)(masks+width) = 0;
;     for( i=0; i<width; i+=8 )
;     {
;         uint64_t mask = *(uint64_t*)(masks+i);
;         if( !mask ) continue;
;         for( j=0; j<8; j++ )
;             if( mask & (255<<j*8) )
;                 mvs[nmv++] = i+j;
;     }
;     return nmv;
; }
cglobal x264_pixel_ads_mvs, 0,7,0
ads_mvs:
%ifdef ARCH_X86_64
    ; mvs = r4
    ; masks = rsp
    ; width = r5
    ; clear last block in case width isn't divisible by 8. (assume divisible by 4, so clearing 4 bytes is enough.)
%ifdef WIN64
    mov     r8, r4
    mov     r9, r5
%endif
    xor     eax, eax
    xor     esi, esi
    mov     dword [rsp+r9], 0
    jmp .loopi
.loopi0:
    add     esi, 8
    cmp     esi, r9d
    jge .end
.loopi:
    mov     rdi, [rsp+rsi]
    test    rdi, rdi
    jz .loopi0
    xor     ecx, ecx
%macro TEST 1
    mov     [r8+rax*2], si
    test    edi, 0xff<<(%1*8)
    setne   cl
    add     eax, ecx
    inc     esi
%endmacro
    TEST 0
    TEST 1
    TEST 2
    TEST 3
    shr     rdi, 32
    TEST 0
    TEST 1
    TEST 2
    TEST 3
    cmp     esi, r9d
    jl .loopi
.end:
    mov     rsp, r10
    RET

%else
    xor     eax, eax
    xor     esi, esi
    mov     ebx, [ebp+stack_offset+20] ; mvs
    mov     edi, [ebp+stack_offset+24] ; width
    mov     dword [esp+edi], 0
    push    ebp
    jmp .loopi
.loopi0:
    add     esi, 8
    cmp     esi, edi
    jge .end
.loopi:
    mov     ebp, [esp+esi+4]
    mov     edx, [esp+esi+8]
    mov     ecx, ebp
    or      ecx, edx
    jz .loopi0
    xor     ecx, ecx
%macro TEST 2
    mov     [ebx+eax*2], si
    test    %2, 0xff<<(%1*8)
    setne   cl
    add     eax, ecx
    inc     esi
%endmacro
    TEST 0, ebp
    TEST 1, ebp
    TEST 2, ebp
    TEST 3, ebp
    TEST 0, edx
    TEST 1, edx
    TEST 2, edx
    TEST 3, edx
    cmp     esi, edi
    jl .loopi
.end:
    pop     esp
    RET
%endif ; ARCH


;*****************************************************************************
;* mc-a2.asm: x86 motion compensation
;*****************************************************************************
;* Copyright (C) 2005-2010 x264 project
;*
;* Authors: Loren Merritt <lorenm@u.washington.edu>
;*          Jason Garrett-Glaser <darkshikari@gmail.com>
;*          Holger Lubitz <holger@lubitz.org>
;*          Mathieu Monnier <manao@melix.net>
;*          Oskar Arvidsson <oskar@irock.se>
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

SECTION_RODATA

filt_mul20: times 16 db 20
filt_mul15: times 8 db 1, -5
filt_mul51: times 8 db -5, 1
hpel_shuf: db 0,8,1,9,2,10,3,11,4,12,5,13,6,14,7,15
deinterleave_shuf: db 0,2,4,6,8,10,12,14,1,3,5,7,9,11,13,15

pd_16: times 4 dd 16
pd_0f: times 4 dd 0xffff

pad10: times 8 dw    10*PIXEL_MAX
pad20: times 8 dw    20*PIXEL_MAX
pad30: times 8 dw    30*PIXEL_MAX
depad: times 4 dd 32*20*PIXEL_MAX + 512

tap1: times 4 dw  1, -5
tap2: times 4 dw 20, 20
tap3: times 4 dw -5,  1

SECTION .text

cextern pb_0
cextern pw_1
cextern pw_16
cextern pw_32
cextern pw_00ff
cextern pw_3fff
cextern pw_pixel_max
cextern pd_128

%macro LOAD_ADD 4
    movh       %4, %3
    movh       %1, %2
    punpcklbw  %4, m0
    punpcklbw  %1, m0
    paddw      %1, %4
%endmacro

%macro LOAD_ADD_2 6
    mova       %5, %3
    mova       %1, %4
    mova       %6, %5
    mova       %2, %1
    punpcklbw  %5, m0
    punpcklbw  %1, m0
    punpckhbw  %6, m0
    punpckhbw  %2, m0
    paddw      %1, %5
    paddw      %2, %6
%endmacro

%macro FILT_V2 6
    psubw  %1, %2  ; a-b
    psubw  %4, %5
    psubw  %2, %3  ; b-c
    psubw  %5, %6
    psllw  %2, 2
    psllw  %5, 2
    psubw  %1, %2  ; a-5*b+4*c
    psllw  %3, 4
    psubw  %4, %5
    psllw  %6, 4
    paddw  %1, %3  ; a-5*b+20*c
    paddw  %4, %6
%endmacro

%macro FILT_H 3
    psubw  %1, %2  ; a-b
    psraw  %1, 2   ; (a-b)/4
    psubw  %1, %2  ; (a-b)/4-b
    paddw  %1, %3  ; (a-b)/4-b+c
    psraw  %1, 2   ; ((a-b)/4-b+c)/4
    paddw  %1, %3  ; ((a-b)/4-b+c)/4+c = (a-5*b+20*c)/16
%endmacro

%macro FILT_H2 6
    psubw  %1, %2
    psubw  %4, %5
    psraw  %1, 2
    psraw  %4, 2
    psubw  %1, %2
    psubw  %4, %5
    paddw  %1, %3
    paddw  %4, %6
    psraw  %1, 2
    psraw  %4, 2
    paddw  %1, %3
    paddw  %4, %6
%endmacro

%macro FILT_PACK 4-6 b
    paddw      %1, %4
    paddw      %2, %4
%if %0 == 6
    psubusw    %1, %6
    psubusw    %2, %6
    psrlw      %1, %3
    psrlw      %2, %3
%else
    psraw      %1, %3
    psraw      %2, %3
%endif
%ifnidn w, %5
    packuswb %1, %2
%endif
%endmacro

;The hpel_filter routines use non-temporal writes for output.
;The following defines may be uncommented for testing.
;Doing the hpel_filter temporal may be a win if the last level cache
;is big enough (preliminary benching suggests on the order of 4* framesize).

;%define movntq movq
;%define movntps movaps
;%define sfence

%ifdef HIGH_BIT_DEPTH
;-----------------------------------------------------------------------------
; void hpel_filter_v( uint16_t *dst, uint16_t *src, int16_t *buf, int stride, int width );
;-----------------------------------------------------------------------------
%macro HPEL_FILTER 1
cglobal hpel_filter_v_%1, 5,6,11*(mmsize/16)
%ifdef WIN64
    movsxd     r4, r4d
%endif
    FIX_STRIDES r3, r4
    lea        r5, [r1+r3]
    sub        r1, r3
    sub        r1, r3
%if num_mmregs > 8
    mova       m8, [pad10]
    mova       m9, [pad20]
    mova      m10, [pad30]
    %define s10 m8
    %define s20 m9
    %define s30 m10
%else
    %define s10 [pad10]
    %define s20 [pad20]
    %define s30 [pad30]
%endif
    add        r0, r4
    lea        r2, [r2+r4]
    neg        r4
    mova       m7, [pw_pixel_max]
    pxor       m0, m0
.loop:
    mova       m1, [r1]
    mova       m2, [r1+r3]
    mova       m3, [r1+r3*2]
    mova       m4, [r1+mmsize]
    mova       m5, [r1+r3+mmsize]
    mova       m6, [r1+r3*2+mmsize]
    paddw      m1, [r5+r3*2]
    paddw      m2, [r5+r3]
    paddw      m3, [r5]
    paddw      m4, [r5+r3*2+mmsize]
    paddw      m5, [r5+r3+mmsize]
    paddw      m6, [r5+mmsize]
    add        r1, 2*mmsize
    add        r5, 2*mmsize
    FILT_V2    m1, m2, m3, m4, m5, m6
    mova       m6, [pw_16]
    psubw      m1, s20
    psubw      m4, s20
    mova      [r2+r4], m1
    mova      [r2+r4+mmsize], m4
    paddw      m1, s30
    paddw      m4, s30
    add        r4, 2*mmsize
    FILT_PACK  m1, m4, 5, m6, w, s10
    CLIPW      m1, m0, m7
    CLIPW      m4, m0, m7
    mova      [r0+r4-mmsize*2], m1
    mova      [r0+r4-mmsize*1], m4
    jl .loop
    REP_RET

;-----------------------------------------------------------------------------
; void hpel_filter_c( uint16_t *dst, int16_t *buf, int width );
;-----------------------------------------------------------------------------
cglobal hpel_filter_c_%1, 3,3,10*(mmsize/16)
    add        r2, r2
    add        r0, r2
    lea        r1, [r1+r2]
    neg        r2
    mova       m0, [tap1]
    mova       m7, [tap3]
%if num_mmregs > 8
    mova       m8, [tap2]
    mova       m9, [depad]
    %define s1 m8
    %define s2 m9
%else
    %define s1 [tap2]
    %define s2 [depad]
%endif
.loop:
    movu       m1, [r1+r2-4]
    movu       m2, [r1+r2-2]
    mova       m3, [r1+r2+0]
    movu       m4, [r1+r2+2]
    movu       m5, [r1+r2+4]
    movu       m6, [r1+r2+6]
    pmaddwd    m1, m0
    pmaddwd    m2, m0
    pmaddwd    m3, s1
    pmaddwd    m4, s1
    pmaddwd    m5, m7
    pmaddwd    m6, m7
    paddd      m1, s2
    paddd      m2, s2
    paddd      m3, m5
    paddd      m4, m6
    paddd      m1, m3
    paddd      m2, m4
    psrad      m1, 10
    psrad      m2, 10
    pslld      m2, 16
    pand       m1, [pd_0f]
    por        m1, m2
    CLIPW      m1, [pb_0], [pw_pixel_max]
    mova  [r0+r2], m1
    add        r2, mmsize
    jl .loop
    REP_RET

;-----------------------------------------------------------------------------
; void hpel_filter_h( uint16_t *dst, uint16_t *src, int width );
;-----------------------------------------------------------------------------
cglobal hpel_filter_h_%1, 3,4,8*(mmsize/16)
    %define src r1+r2
    add        r2, r2
    add        r0, r2
    add        r1, r2
    neg        r2
    mova       m0, [pw_pixel_max]
.loop:
    movu       m1, [src-4]
    movu       m2, [src-2]
    mova       m3, [src+0]
    movu       m6, [src+2]
    movu       m4, [src+4]
    movu       m5, [src+6]
    paddw      m3, m6 ; c0
    paddw      m2, m4 ; b0
    paddw      m1, m5 ; a0
%if mmsize == 16
    movu       m4, [src-4+mmsize]
    movu       m5, [src-2+mmsize]
%endif
    movu       m7, [src+4+mmsize]
    movu       m6, [src+6+mmsize]
    paddw      m5, m7 ; b1
    paddw      m4, m6 ; a1
    movu       m7, [src+2+mmsize]
    mova       m6, [src+0+mmsize]
    paddw      m6, m7 ; c1
    FILT_H2    m1, m2, m3, m4, m5, m6
    mova       m7, [pw_1]
    pxor       m2, m2
    add        r2, mmsize*2
    FILT_PACK  m1, m4, 1, m7, w
    CLIPW      m1, m2, m0
    CLIPW      m4, m2, m0
    mova      [r0+r2-mmsize*2], m1
    mova      [r0+r2-mmsize*1], m4
    jl .loop
    REP_RET
%endmacro

INIT_MMX
HPEL_FILTER mmxext
INIT_XMM
HPEL_FILTER sse2
%endif ; HIGH_BIT_DEPTH

%ifndef HIGH_BIT_DEPTH
INIT_MMX

%macro HPEL_V 1-2 0
;-----------------------------------------------------------------------------
; void hpel_filter_v( uint8_t *dst, uint8_t *src, int16_t *buf, int stride, int width );
;-----------------------------------------------------------------------------
cglobal hpel_filter_v_%1, 5,6,%2
%ifdef WIN64
    movsxd   r4, r4d
%endif
    lea r5, [r1+r3]
    sub r1, r3
    sub r1, r3
    add r0, r4
    lea r2, [r2+r4*2]
    neg r4
%ifnidn %1, ssse3
    pxor m0, m0
%else
    mova m0, [filt_mul15]
%endif
.loop:
%ifidn %1, ssse3
    mova m1, [r1]
    mova m4, [r1+r3]
    mova m2, [r5+r3*2]
    mova m5, [r5+r3]
    mova m3, [r1+r3*2]
    mova m6, [r5]
    SBUTTERFLY bw, 1, 4, 7
    SBUTTERFLY bw, 2, 5, 7
    SBUTTERFLY bw, 3, 6, 7
    pmaddubsw m1, m0
    pmaddubsw m4, m0
    pmaddubsw m2, m0
    pmaddubsw m5, m0
    pmaddubsw m3, [filt_mul20]
    pmaddubsw m6, [filt_mul20]
    paddw  m1, m2
    paddw  m4, m5
    paddw  m1, m3
    paddw  m4, m6
%else
    LOAD_ADD_2 m1, m4, [r1     ], [r5+r3*2], m6, m7            ; a0 / a1
    LOAD_ADD_2 m2, m5, [r1+r3  ], [r5+r3  ], m6, m7            ; b0 / b1
    LOAD_ADD   m3,     [r1+r3*2], [r5     ], m7                ; c0
    LOAD_ADD   m6,     [r1+r3*2+mmsize/2], [r5+mmsize/2], m7 ; c1
    FILT_V2 m1, m2, m3, m4, m5, m6
%endif
    mova      m7, [pw_16]
    mova      [r2+r4*2], m1
    mova      [r2+r4*2+mmsize], m4
    FILT_PACK m1, m4, 5, m7
    movnta    [r0+r4], m1
    add r1, mmsize
    add r5, mmsize
    add r4, mmsize
    jl .loop
    REP_RET
%endmacro
HPEL_V mmxext

;-----------------------------------------------------------------------------
; void hpel_filter_c( uint8_t *dst, int16_t *buf, int width );
;-----------------------------------------------------------------------------
cglobal hpel_filter_c_mmxext, 3,3
    add r0, r2
    lea r1, [r1+r2*2]
    neg r2
    %define src r1+r2*2
    movq m7, [pw_32]
.loop:
    movq   m1, [src-4]
    movq   m2, [src-2]
    movq   m3, [src  ]
    movq   m4, [src+4]
    movq   m5, [src+6]
    paddw  m3, [src+2]  ; c0
    paddw  m2, m4       ; b0
    paddw  m1, m5       ; a0
    movq   m6, [src+8]
    paddw  m4, [src+14] ; a1
    paddw  m5, [src+12] ; b1
    paddw  m6, [src+10] ; c1
    FILT_H2 m1, m2, m3, m4, m5, m6
    FILT_PACK m1, m4, 6, m7
    movntq [r0+r2], m1
    add r2, 8
    jl .loop
    REP_RET

;-----------------------------------------------------------------------------
; void hpel_filter_h( uint8_t *dst, uint8_t *src, int width );
;-----------------------------------------------------------------------------
cglobal hpel_filter_h_mmxext, 3,3
    add r0, r2
    add r1, r2
    neg r2
    %define src r1+r2
    pxor m0, m0
.loop:
    movd       m1, [src-2]
    movd       m2, [src-1]
    movd       m3, [src  ]
    movd       m6, [src+1]
    movd       m4, [src+2]
    movd       m5, [src+3]
    punpcklbw  m1, m0
    punpcklbw  m2, m0
    punpcklbw  m3, m0
    punpcklbw  m6, m0
    punpcklbw  m4, m0
    punpcklbw  m5, m0
    paddw      m3, m6 ; c0
    paddw      m2, m4 ; b0
    paddw      m1, m5 ; a0
    movd       m7, [src+7]
    movd       m6, [src+6]
    punpcklbw  m7, m0
    punpcklbw  m6, m0
    paddw      m4, m7 ; c1
    paddw      m5, m6 ; b1
    movd       m7, [src+5]
    movd       m6, [src+4]
    punpcklbw  m7, m0
    punpcklbw  m6, m0
    paddw      m6, m7 ; a1
    movq       m7, [pw_1]
    FILT_H2 m1, m2, m3, m4, m5, m6
    FILT_PACK m1, m4, 1, m7
    movntq     [r0+r2], m1
    add r2, 8
    jl .loop
    REP_RET

INIT_XMM

%macro HPEL_C 1
;-----------------------------------------------------------------------------
; void hpel_filter_c( uint8_t *dst, int16_t *buf, int width );
;-----------------------------------------------------------------------------
cglobal hpel_filter_c_%1, 3,3,9
    add r0, r2
    lea r1, [r1+r2*2]
    neg r2
    %define src r1+r2*2
%ifnidn %1, sse2
    mova    m7, [pw_32]
    %define tpw_32 m7
%elifdef ARCH_X86_64
    mova    m8, [pw_32]
    %define tpw_32 m8
%else
    %define tpw_32 [pw_32]
%endif
%ifidn %1,sse2_misalign
.loop:
    movu    m4, [src-4]
    movu    m5, [src-2]
    mova    m6, [src]
    movu    m3, [src+12]
    movu    m2, [src+14]
    mova    m1, [src+16]
    paddw   m4, [src+6]
    paddw   m5, [src+4]
    paddw   m6, [src+2]
    paddw   m3, [src+22]
    paddw   m2, [src+20]
    paddw   m1, [src+18]
    FILT_H2 m4, m5, m6, m3, m2, m1
%else
    mova      m0, [src-16]
    mova      m1, [src]
.loop:
    mova      m2, [src+16]
    mova      m4, m1
    PALIGNR   m4, m0, 12, m7
    mova      m5, m1
    PALIGNR   m5, m0, 14, m0
    mova      m0, m2
    PALIGNR   m0, m1, 6, m7
    paddw     m4, m0
    mova      m0, m2
    PALIGNR   m0, m1, 4, m7
    paddw     m5, m0
    mova      m6, m2
    PALIGNR   m6, m1, 2, m7
    paddw     m6, m1
    FILT_H    m4, m5, m6

    mova      m0, m2
    mova      m5, m2
    PALIGNR   m2, m1, 12, m7
    PALIGNR   m5, m1, 14, m1
    mova      m1, [src+32]
    mova      m3, m1
    PALIGNR   m3, m0, 6, m7
    paddw     m3, m2
    mova      m6, m1
    PALIGNR   m6, m0, 4, m7
    paddw     m5, m6
    mova      m6, m1
    PALIGNR   m6, m0, 2, m7
    paddw     m6, m0
    FILT_H    m3, m5, m6
%endif
    FILT_PACK m4, m3, 6, tpw_32
    movntps [r0+r2], m4
    add r2, 16
    jl .loop
    REP_RET
%endmacro

;-----------------------------------------------------------------------------
; void hpel_filter_h( uint8_t *dst, uint8_t *src, int width );
;-----------------------------------------------------------------------------
cglobal hpel_filter_h_sse2, 3,3,8
    add r0, r2
    add r1, r2
    neg r2
    %define src r1+r2
    pxor m0, m0
.loop:
    movh       m1, [src-2]
    movh       m2, [src-1]
    movh       m3, [src  ]
    movh       m4, [src+1]
    movh       m5, [src+2]
    movh       m6, [src+3]
    punpcklbw  m1, m0
    punpcklbw  m2, m0
    punpcklbw  m3, m0
    punpcklbw  m4, m0
    punpcklbw  m5, m0
    punpcklbw  m6, m0
    paddw      m3, m4 ; c0
    paddw      m2, m5 ; b0
    paddw      m1, m6 ; a0
    movh       m4, [src+6]
    movh       m5, [src+7]
    movh       m6, [src+10]
    movh       m7, [src+11]
    punpcklbw  m4, m0
    punpcklbw  m5, m0
    punpcklbw  m6, m0
    punpcklbw  m7, m0
    paddw      m5, m6 ; b1
    paddw      m4, m7 ; a1
    movh       m6, [src+8]
    movh       m7, [src+9]
    punpcklbw  m6, m0
    punpcklbw  m7, m0
    paddw      m6, m7 ; c1
    mova       m7, [pw_1] ; FIXME xmm8
    FILT_H2 m1, m2, m3, m4, m5, m6
    FILT_PACK m1, m4, 1, m7
    movntps    [r0+r2], m1
    add r2, 16
    jl .loop
    REP_RET

%ifndef ARCH_X86_64
;-----------------------------------------------------------------------------
; void hpel_filter_h( uint8_t *dst, uint8_t *src, int width );
;-----------------------------------------------------------------------------
cglobal hpel_filter_h_ssse3, 3,3
    add r0, r2
    add r1, r2
    neg r2
    %define src r1+r2
    mova      m0, [src-16]
    mova      m1, [src]
    mova      m7, [pw_16]
.loop:
    mova      m2, [src+16]
    mova      m3, m1
    palignr   m3, m0, 14
    mova      m4, m1
    palignr   m4, m0, 15
    mova      m0, m2
    palignr   m0, m1, 2
    pmaddubsw m3, [filt_mul15]
    pmaddubsw m4, [filt_mul15]
    pmaddubsw m0, [filt_mul51]
    mova      m5, m2
    palignr   m5, m1, 1
    mova      m6, m2
    palignr   m6, m1, 3
    paddw     m3, m0
    mova      m0, m1
    pmaddubsw m1, [filt_mul20]
    pmaddubsw m5, [filt_mul20]
    pmaddubsw m6, [filt_mul51]
    paddw     m3, m1
    paddw     m4, m5
    paddw     m4, m6
    FILT_PACK m3, m4, 5, m7
    pshufb    m3, [hpel_shuf]
    mova      m1, m2
    movntps [r0+r2], m3
    add r2, 16
    jl .loop
    REP_RET
%endif

%define PALIGNR PALIGNR_MMX
%ifndef ARCH_X86_64
HPEL_C sse2
%endif
HPEL_V sse2, 8
HPEL_C sse2_misalign
%define PALIGNR PALIGNR_SSSE3
HPEL_C ssse3
HPEL_V ssse3

%ifdef ARCH_X86_64

%macro DO_FILT_V 6
    ;The optimum prefetch distance is difficult to determine in checkasm:
    ;any prefetch seems slower than not prefetching.
    ;In real use, the prefetch seems to be a slight win.
    ;+16 is picked somewhat arbitrarily here based on the fact that even one
    ;loop iteration is going to take longer than the prefetch.
    prefetcht0 [r1+r2*2+16]
%ifidn %6, ssse3
    mova m1, [r3]
    mova m2, [r3+r2]
    mova %3, [r3+r2*2]
    mova m3, [r1]
    mova %1, [r1+r2]
    mova %2, [r1+r2*2]
    mova m4, m1
    punpcklbw m1, m2
    punpckhbw m4, m2
    mova m2, %1
    punpcklbw %1, %2
    punpckhbw m2, %2
    mova %2, m3
    punpcklbw m3, %3
    punpckhbw %2, %3

    pmaddubsw m1, m12
    pmaddubsw m4, m12
    pmaddubsw %1, m0
    pmaddubsw m2, m0
    pmaddubsw m3, m14
    pmaddubsw %2, m14

    paddw m1, %1
    paddw m4, m2
    paddw m1, m3
    paddw m4, %2
%else
    LOAD_ADD_2 m1, m4, [r3     ], [r1+r2*2], m2, m5            ; a0 / a1
    LOAD_ADD_2 m2, m5, [r3+r2  ], [r1+r2  ], m3, m6            ; b0 / b1
    LOAD_ADD_2 m3, m6, [r3+r2*2], [r1     ], %3, %4            ; c0 / c1
    packuswb %3, %4
    FILT_V2 m1, m2, m3, m4, m5, m6
%endif
    add       r3, 16
    add       r1, 16
    mova      %1, m1
    mova      %2, m4
    FILT_PACK m1, m4, 5, m15
    movntps  [r11+r4+%5], m1
%endmacro

%macro FILT_C 4
    mova      m1, %2
    PALIGNR   m1, %1, 12, m2
    mova      m2, %2
    PALIGNR   m2, %1, 14, %1
    mova      m3, %3
    PALIGNR   m3, %2, 4, %1
    mova      m4, %3
    PALIGNR   m4, %2, 2, %1
    paddw     m3, m2
    mova      %1, %3
    PALIGNR   %3, %2, 6, m2
    paddw     m4, %2
    paddw     %3, m1
    FILT_H    %3, m3, m4
%endmacro

%macro DO_FILT_C 4
    FILT_C %1, %2, %3, 6
    FILT_C %2, %1, %4, 6
    FILT_PACK %3, %4, 6, m15
    movntps   [r5+r4], %3
%endmacro

%macro ADD8TO16 5
    mova      %3, %1
    mova      %4, %2
    punpcklbw %1, %5
    punpckhbw %2, %5
    punpckhbw %3, %5
    punpcklbw %4, %5
    paddw     %2, %3
    paddw     %1, %4
%endmacro

%macro DO_FILT_H 4
    mova      m1, %2
    PALIGNR   m1, %1, 14, m3
    mova      m2, %2
    PALIGNR   m2, %1, 15, m3
    mova      m4, %3
    PALIGNR   m4, %2, 1 , m3
    mova      m5, %3
    PALIGNR   m5, %2, 2 , m3
    mova      m6, %3
    PALIGNR   m6, %2, 3 , m3
    mova      %1, %2
%ifidn %4, sse2
    ADD8TO16  m1, m6, m12, m3, m0 ; a
    ADD8TO16  m2, m5, m12, m3, m0 ; b
    ADD8TO16  %2, m4, m12, m3, m0 ; c
    FILT_V2   m1, m2, %2, m6, m5, m4
    FILT_PACK m1, m6, 5, m15
%else ; ssse3
    pmaddubsw m1, m12
    pmaddubsw m2, m12
    pmaddubsw %2, m14
    pmaddubsw m4, m14
    pmaddubsw m5, m0
    pmaddubsw m6, m0
    paddw     m1, %2
    paddw     m2, m4
    paddw     m1, m5
    paddw     m2, m6
    FILT_PACK m1, m2, 5, m15
    pshufb    m1, [hpel_shuf]
%endif
    movntps [r0+r4], m1
    mova      %2, %3
%endmacro

%macro HPEL 1
;-----------------------------------------------------------------------------
; void hpel_filter( uint8_t *dsth, uint8_t *dstv, uint8_t *dstc,
;                   uint8_t *src, int stride, int width, int height)
;-----------------------------------------------------------------------------
cglobal hpel_filter_%1, 7,7,16
%ifdef WIN64
    movsxd   r4, r4d
    movsxd   r5, r5d
%endif
    mov      r10, r3
    sub       r5, 16
    mov      r11, r1
    and      r10, 15
    sub       r3, r10
    add       r0, r5
    add      r11, r5
    add      r10, r5
    add       r5, r2
    mov       r2, r4
    neg      r10
    lea       r1, [r3+r2]
    sub       r3, r2
    sub       r3, r2
    mov       r4, r10
    mova     m15, [pw_16]
%ifidn %1, sse2
    pxor      m0, m0
%else ; ssse3
    mova      m0, [filt_mul51]
    mova     m12, [filt_mul15]
    mova     m14, [filt_mul20]
%endif
;ALIGN 16
.loopy:
; first filter_v
    DO_FILT_V m8, m7, m13, m12, 0, %1
;ALIGN 16
.loopx:
    DO_FILT_V m6, m5, m11, m12, 16, %1
.lastx:
    paddw   m15, m15 ; pw_32
    DO_FILT_C m9, m8, m7, m6
    psrlw   m15, 1 ; pw_16
    movdqa   m7, m5
    DO_FILT_H m10, m13, m11, %1
    add      r4, 16
    jl .loopx
    cmp      r4, 16
    jl .lastx
; setup regs for next y
    sub      r4, r10
    sub      r4, r2
    sub      r1, r4
    sub      r3, r4
    add      r0, r2
    add     r11, r2
    add      r5, r2
    mov      r4, r10
    sub     r6d, 1
    jg .loopy
    sfence
    RET
%endmacro

%define PALIGNR PALIGNR_MMX
HPEL sse2
%define PALIGNR PALIGNR_SSSE3
HPEL ssse3
%endif

%undef movntq
%undef movntps
%undef sfence
%endif ; !HIGH_BIT_DEPTH

;-----------------------------------------------------------------------------
; void plane_copy_core( uint8_t *dst, int i_dst,
;                       uint8_t *src, int i_src, int w, int h)
;-----------------------------------------------------------------------------
; assumes i_dst and w are multiples of 16, and i_dst>w
cglobal plane_copy_core_mmxext, 6,7
    movsxdifnidn r1, r1d
    movsxdifnidn r3, r3d
    movsxdifnidn r4, r4d
    sub    r1,  r4
    sub    r3,  r4
.loopy:
    mov    r6d, r4d
    sub    r6d, 63
.loopx:
    prefetchnta [r2+256]
    movq   mm0, [r2   ]
    movq   mm1, [r2+ 8]
    movntq [r0   ], mm0
    movntq [r0+ 8], mm1
    movq   mm2, [r2+16]
    movq   mm3, [r2+24]
    movntq [r0+16], mm2
    movntq [r0+24], mm3
    movq   mm4, [r2+32]
    movq   mm5, [r2+40]
    movntq [r0+32], mm4
    movntq [r0+40], mm5
    movq   mm6, [r2+48]
    movq   mm7, [r2+56]
    movntq [r0+48], mm6
    movntq [r0+56], mm7
    add    r2,  64
    add    r0,  64
    sub    r6d, 64
    jg .loopx
    prefetchnta [r2+256]
    add    r6d, 63
    jle .end16
.loop16:
    movq   mm0, [r2  ]
    movq   mm1, [r2+8]
    movntq [r0  ], mm0
    movntq [r0+8], mm1
    add    r2,  16
    add    r0,  16
    sub    r6d, 16
    jg .loop16
.end16:
    add    r0, r1
    add    r2, r3
    dec    r5d
    jg .loopy
    sfence
    emms
    RET


%macro INTERLEAVE 4-5 ; dst, srcu, srcv, is_aligned, nt_hint
    movq   m0, [%2]
%if mmsize==16
%if %4
    punpcklbw m0, [%3]
%else
    movq   m1, [%3]
    punpcklbw m0, m1
%endif
    mov%5a [%1], m0
%else
    movq   m1, [%3]
    mova   m2, m0
    punpcklbw m0, m1
    punpckhbw m2, m1
    mov%5a [%1], m0
    mov%5a [%1+8], m2
%endif
%endmacro

%macro DEINTERLEAVE 6 ; dstu, dstv, src, dstv==dstu+8, cpu, shuffle constant
%if mmsize==16
    mova   m0, [%3]
%ifidn %5, ssse3
    pshufb m0, %6
%else
    mova   m1, m0
    pand   m0, %6
    psrlw  m1, 8
    packuswb m0, m1
%endif
%if %4
    mova   [%1], m0
%else
    movq   [%1], m0
    movhps [%2], m0
%endif
%else
    mova   m0, [%3]
    mova   m1, [%3+8]
    mova   m2, m0
    mova   m3, m1
    pand   m0, %6
    pand   m1, %6
    psrlw  m2, 8
    psrlw  m3, 8
    packuswb m0, m1
    packuswb m2, m3
    mova   [%1], m0
    mova   [%2], m2
%endif
%endmacro

%macro PLANE_INTERLEAVE 1
;-----------------------------------------------------------------------------
; void plane_copy_interleave_core( uint8_t *dst, int i_dst,
;                                  uint8_t *srcu, int i_srcu,
;                                  uint8_t *srcv, int i_srcv, int w, int h )
;-----------------------------------------------------------------------------
; assumes i_dst and w are multiples of 16, and i_dst>2*w
cglobal plane_copy_interleave_core_%1, 6,7
    mov    r6d, r6m
    movsxdifnidn r1, r1d
    movsxdifnidn r3, r3d
    movsxdifnidn r5, r5d
    lea    r0, [r0+r6*2]
    add    r2,  r6
    add    r4,  r6
%ifdef ARCH_X86_64
    DECLARE_REG_TMP 10,11
%else
    DECLARE_REG_TMP 1,3
%endif
    mov  t0d, r7m
    mov  t1d, r1d
    shr  t1d, 1
    sub  t1d, r6d
.loopy:
    mov    r6d, r6m
    neg    r6
.prefetch:
    prefetchnta [r2+r6]
    prefetchnta [r4+r6]
    add    r6, 64
    jl .prefetch
    mov    r6d, r6m
    neg    r6
.loopx:
    INTERLEAVE r0+r6*2,    r2+r6,   r4+r6,   0, nt
    INTERLEAVE r0+r6*2+16, r2+r6+8, r4+r6+8, 0, nt
    add    r6, 16
    jl .loopx
.pad:
%if mmsize==8
    movntq [r0+r6*2], m0
    movntq [r0+r6*2+8], m0
    movntq [r0+r6*2+16], m0
    movntq [r0+r6*2+24], m0
%else
    movntdq [r0+r6*2], m0
    movntdq [r0+r6*2+16], m0
%endif
    add    r6, 16
    cmp    r6, t1
    jl .pad
    add    r0, r1mp
    add    r2, r3mp
    add    r4, r5
    dec    t0d
    jg .loopy
    sfence
    emms
    RET

;-----------------------------------------------------------------------------
; void store_interleave_8x8x2( uint8_t *dst, int i_dst, uint8_t *srcu, uint8_t *srcv )
;-----------------------------------------------------------------------------
cglobal store_interleave_8x8x2_%1, 4,5
    mov    r4d, 4
.loop:
    INTERLEAVE r0, r2, r3, 1
    INTERLEAVE r0+r1, r2+FDEC_STRIDE, r3+FDEC_STRIDE, 1
    add    r2, FDEC_STRIDE*2
    add    r3, FDEC_STRIDE*2
    lea    r0, [r0+r1*2]
    dec    r4d
    jg .loop
    REP_RET
%endmacro ; PLANE_INTERLEAVE

%macro DEINTERLEAVE_START 1
%ifidn %1, ssse3
    mova   m4, [deinterleave_shuf]
%else
    mova   m4, [pw_00ff]
%endif
%endmacro

%macro PLANE_DEINTERLEAVE 1
;-----------------------------------------------------------------------------
; void plane_copy_deinterleave( uint8_t *dstu, int i_dstu,
;                               uint8_t *dstv, int i_dstv,
;                               uint8_t *src, int i_src, int w, int h )
;-----------------------------------------------------------------------------
cglobal plane_copy_deinterleave_%1, 6,7
    DEINTERLEAVE_START %1
    mov    r6d, r6m
    movsxdifnidn r1, r1d
    movsxdifnidn r3, r3d
    movsxdifnidn r5, r5d
    add    r0,  r6
    add    r2,  r6
    lea    r4, [r4+r6*2]
.loopy:
    mov    r6d, r6m
    neg    r6
.loopx:
    DEINTERLEAVE r0+r6,   r2+r6,   r4+r6*2,    0, %1, m4
    DEINTERLEAVE r0+r6+8, r2+r6+8, r4+r6*2+16, 0, %1, m4
    add    r6, 16
    jl .loopx
    add    r0, r1
    add    r2, r3
    add    r4, r5
    dec dword r7m
    jg .loopy
    REP_RET

;-----------------------------------------------------------------------------
; void load_deinterleave_8x8x2_fenc( uint8_t *dst, uint8_t *src, int i_src )
;-----------------------------------------------------------------------------
cglobal load_deinterleave_8x8x2_fenc_%1, 3,4
    DEINTERLEAVE_START %1
    mov    r3d, 4
.loop:
    DEINTERLEAVE r0, r0+FENC_STRIDE/2, r1, 1, %1, m4
    DEINTERLEAVE r0+FENC_STRIDE, r0+FENC_STRIDE*3/2, r1+r2, 1, %1, m4
    add    r0, FENC_STRIDE*2
    lea    r1, [r1+r2*2]
    dec    r3d
    jg .loop
    REP_RET

;-----------------------------------------------------------------------------
; void load_deinterleave_8x8x2_fdec( uint8_t *dst, uint8_t *src, int i_src )
;-----------------------------------------------------------------------------
cglobal load_deinterleave_8x8x2_fdec_%1, 3,4
    DEINTERLEAVE_START %1
    mov    r3d, 4
.loop:
    DEINTERLEAVE r0, r0+FDEC_STRIDE/2, r1, 0, %1, m4
    DEINTERLEAVE r0+FDEC_STRIDE, r0+FDEC_STRIDE*3/2, r1+r2, 0, %1, m4
    add    r0, FDEC_STRIDE*2
    lea    r1, [r1+r2*2]
    dec    r3d
    jg .loop
    REP_RET
%endmacro ; PLANE_DEINTERLEAVE

INIT_MMX
PLANE_INTERLEAVE mmxext
PLANE_DEINTERLEAVE mmx
INIT_XMM
PLANE_INTERLEAVE sse2
PLANE_DEINTERLEAVE sse2
PLANE_DEINTERLEAVE ssse3


; These functions are not general-use; not only do the SSE ones require aligned input,
; but they also will fail if given a non-mod16 size or a size less than 64.
; memzero SSE will fail for non-mod128.

;-----------------------------------------------------------------------------
; void *memcpy_aligned( void *dst, const void *src, size_t n );
;-----------------------------------------------------------------------------
cglobal memcpy_aligned_mmx, 3,3
    test r2d, 16
    jz .copy32
    sub r2d, 16
    movq mm0, [r1 + r2 + 0]
    movq mm1, [r1 + r2 + 8]
    movq [r0 + r2 + 0], mm0
    movq [r0 + r2 + 8], mm1
.copy32:
    sub r2d, 32
    movq mm0, [r1 + r2 +  0]
    movq mm1, [r1 + r2 +  8]
    movq mm2, [r1 + r2 + 16]
    movq mm3, [r1 + r2 + 24]
    movq [r0 + r2 +  0], mm0
    movq [r0 + r2 +  8], mm1
    movq [r0 + r2 + 16], mm2
    movq [r0 + r2 + 24], mm3
    jg .copy32
    REP_RET

;-----------------------------------------------------------------------------
; void *memcpy_aligned( void *dst, const void *src, size_t n );
;-----------------------------------------------------------------------------
cglobal memcpy_aligned_sse2, 3,3
    test r2d, 16
    jz .copy32
    sub r2d, 16
    movdqa xmm0, [r1 + r2]
    movdqa [r0 + r2], xmm0
.copy32:
    test r2d, 32
    jz .copy64
    sub r2d, 32
    movdqa xmm0, [r1 + r2 +  0]
    movdqa [r0 + r2 +  0], xmm0
    movdqa xmm1, [r1 + r2 + 16]
    movdqa [r0 + r2 + 16], xmm1
.copy64:
    sub r2d, 64
    movdqa xmm0, [r1 + r2 +  0]
    movdqa [r0 + r2 +  0], xmm0
    movdqa xmm1, [r1 + r2 + 16]
    movdqa [r0 + r2 + 16], xmm1
    movdqa xmm2, [r1 + r2 + 32]
    movdqa [r0 + r2 + 32], xmm2
    movdqa xmm3, [r1 + r2 + 48]
    movdqa [r0 + r2 + 48], xmm3
    jg .copy64
    REP_RET

;-----------------------------------------------------------------------------
; void *memzero_aligned( void *dst, size_t n );
;-----------------------------------------------------------------------------
%macro MEMZERO 1
cglobal memzero_aligned_%1, 2,2
    add  r0, r1
    neg  r1
    pxor m0, m0
.loop:
%assign i 0
%rep 8
    mova [r0 + r1 + i], m0
%assign i i+mmsize
%endrep
    add r1, mmsize*8
    jl .loop
    REP_RET
%endmacro

INIT_MMX
MEMZERO mmx
INIT_XMM
MEMZERO sse2



;-----------------------------------------------------------------------------
; void integral_init4h( uint16_t *sum, uint8_t *pix, int stride )
;-----------------------------------------------------------------------------
cglobal integral_init4h_sse4, 3,4
    lea     r3, [r0+r2*2]
    add     r1, r2
    neg     r2
    pxor    m4, m4
.loop:
    movdqa  m0, [r1+r2]
    movdqa  m1, [r1+r2+16]
    palignr m1, m0, 8
    mpsadbw m0, m4, 0
    mpsadbw m1, m4, 0
    paddw   m0, [r0+r2*2]
    paddw   m1, [r0+r2*2+16]
    movdqa  [r3+r2*2   ], m0
    movdqa  [r3+r2*2+16], m1
    add     r2, 16
    jl .loop
    REP_RET

cglobal integral_init8h_sse4, 3,4
    lea     r3, [r0+r2*2]
    add     r1, r2
    neg     r2
    pxor    m4, m4
.loop:
    movdqa  m0, [r1+r2]
    movdqa  m1, [r1+r2+16]
    palignr m1, m0, 8
    movdqa  m2, m0
    movdqa  m3, m1
    mpsadbw m0, m4, 0
    mpsadbw m1, m4, 0
    mpsadbw m2, m4, 4
    mpsadbw m3, m4, 4
    paddw   m0, [r0+r2*2]
    paddw   m1, [r0+r2*2+16]
    paddw   m0, m2
    paddw   m1, m3
    movdqa  [r3+r2*2   ], m0
    movdqa  [r3+r2*2+16], m1
    add     r2, 16
    jl .loop
    REP_RET

%macro INTEGRAL_INIT_8V 1
;-----------------------------------------------------------------------------
; void integral_init8v( uint16_t *sum8, int stride )
;-----------------------------------------------------------------------------
cglobal integral_init8v_%1, 3,3
    shl   r1, 1
    add   r0, r1
    lea   r2, [r0+r1*8]
    neg   r1
.loop:
    mova  m0, [r2+r1]
    mova  m1, [r2+r1+mmsize]
    psubw m0, [r0+r1]
    psubw m1, [r0+r1+mmsize]
    mova  [r0+r1], m0
    mova  [r0+r1+mmsize], m1
    add   r1, 2*mmsize
    jl .loop
    REP_RET
%endmacro

INIT_MMX
INTEGRAL_INIT_8V mmx
INIT_XMM
INTEGRAL_INIT_8V sse2

;-----------------------------------------------------------------------------
; void integral_init4v( uint16_t *sum8, uint16_t *sum4, int stride )
;-----------------------------------------------------------------------------
INIT_MMX
cglobal integral_init4v_mmx, 3,5
    shl   r2, 1
    lea   r3, [r0+r2*4]
    lea   r4, [r0+r2*8]
    mova  m0, [r0+r2]
    mova  m4, [r4+r2]
.loop:
    sub   r2, 8
    mova  m1, m4
    psubw m1, m0
    mova  m4, [r4+r2]
    mova  m0, [r0+r2]
    paddw m1, m4
    mova  m3, [r3+r2]
    psubw m1, m0
    psubw m3, m0
    mova  [r0+r2], m1
    mova  [r1+r2], m3
    jge .loop
    REP_RET

INIT_XMM
cglobal integral_init4v_sse2, 3,5
    shl     r2, 1
    add     r0, r2
    add     r1, r2
    lea     r3, [r0+r2*4]
    lea     r4, [r0+r2*8]
    neg     r2
.loop:
    mova    m0, [r0+r2]
    mova    m1, [r4+r2]
    mova    m2, m0
    mova    m4, m1
    shufpd  m0, [r0+r2+16], 1
    shufpd  m1, [r4+r2+16], 1
    paddw   m0, m2
    paddw   m1, m4
    mova    m3, [r3+r2]
    psubw   m1, m0
    psubw   m3, m2
    mova  [r0+r2], m1
    mova  [r1+r2], m3
    add     r2, 16
    jl .loop
    REP_RET

cglobal integral_init4v_ssse3, 3,5
    shl     r2, 1
    add     r0, r2
    add     r1, r2
    lea     r3, [r0+r2*4]
    lea     r4, [r0+r2*8]
    neg     r2
.loop:
    mova    m2, [r0+r2]
    mova    m0, [r0+r2+16]
    mova    m4, [r4+r2]
    mova    m1, [r4+r2+16]
    palignr m0, m2, 8
    palignr m1, m4, 8
    paddw   m0, m2
    paddw   m1, m4
    mova    m3, [r3+r2]
    psubw   m1, m0
    psubw   m3, m2
    mova  [r0+r2], m1
    mova  [r1+r2], m3
    add     r2, 16
    jl .loop
    REP_RET

%macro FILT8x4 7
    mova      %3, [r0+%7]
    mova      %4, [r0+r5+%7]
    pavgb     %3, %4
    pavgb     %4, [r0+r5*2+%7]
    PALIGNR   %1, %3, 1, m6
    PALIGNR   %2, %4, 1, m6
    pavgb     %1, %3
    pavgb     %2, %4
    mova      %5, %1
    mova      %6, %2
    pand      %1, m7
    pand      %2, m7
    psrlw     %5, 8
    psrlw     %6, 8
%endmacro

%macro FILT16x2 4
    mova      m3, [r0+%4+mmsize]
    mova      m2, [r0+%4]
    pavgb     m3, [r0+%4+r5+mmsize]
    pavgb     m2, [r0+%4+r5]
    PALIGNR   %1, m3, 1, m6
    pavgb     %1, m3
    PALIGNR   m3, m2, 1, m6
    pavgb     m3, m2
    mova      m5, m3
    mova      m4, %1
    pand      m3, m7
    pand      %1, m7
    psrlw     m5, 8
    psrlw     m4, 8
    packuswb  m3, %1
    packuswb  m5, m4
    mova    [%2], m3
    mova    [%3], m5
    mova      %1, m2
%endmacro

%macro FILT8x2U 3
    mova      m3, [r0+%3+8]
    mova      m2, [r0+%3]
    pavgb     m3, [r0+%3+r5+8]
    pavgb     m2, [r0+%3+r5]
    mova      m1, [r0+%3+9]
    mova      m0, [r0+%3+1]
    pavgb     m1, [r0+%3+r5+9]
    pavgb     m0, [r0+%3+r5+1]
    pavgb     m1, m3
    pavgb     m0, m2
    mova      m3, m1
    mova      m2, m0
    pand      m1, m7
    pand      m0, m7
    psrlw     m3, 8
    psrlw     m2, 8
    packuswb  m0, m1
    packuswb  m2, m3
    mova    [%1], m0
    mova    [%2], m2
%endmacro

;-----------------------------------------------------------------------------
; void frame_init_lowres_core( uint8_t *src0, uint8_t *dst0, uint8_t *dsth, uint8_t *dstv, uint8_t *dstc,
;                              int src_stride, int dst_stride, int width, int height )
;-----------------------------------------------------------------------------
%macro FRAME_INIT_LOWRES 1-2 0 ; FIXME
cglobal frame_init_lowres_core_%1, 6,7,%2
%ifdef WIN64
    movsxd   r5, r5d
%endif
    ; src += 2*(height-1)*stride + 2*width
    mov      r6d, r8m
    dec      r6d
    imul     r6d, r5d
    add      r6d, r7m
    lea       r0, [r0+r6*2]
    ; dst += (height-1)*stride + width
    mov      r6d, r8m
    dec      r6d
    imul     r6d, r6m
    add      r6d, r7m
    add       r1, r6
    add       r2, r6
    add       r3, r6
    add       r4, r6
    ; gap = stride - width
    mov      r6d, r6m
    sub      r6d, r7m
    PUSH      r6
    %define dst_gap [rsp+gprsize]
    mov      r6d, r5d
    sub      r6d, r7m
    shl      r6d, 1
    PUSH      r6
    %define src_gap [rsp]
%if mmsize == 16
    ; adjust for the odd end case
    mov      r6d, r7m
    and      r6d, 8
    sub       r1, r6
    sub       r2, r6
    sub       r3, r6
    sub       r4, r6
    add  dst_gap, r6d
%endif ; mmsize
    pcmpeqb   m7, m7
    psrlw     m7, 8
.vloop:
    mov      r6d, r7m
%ifnidn %1, mmxext
    mova      m0, [r0]
    mova      m1, [r0+r5]
    pavgb     m0, m1
    pavgb     m1, [r0+r5*2]
%endif
%if mmsize == 16
    test     r6d, 8
    jz .hloop
    sub       r0, 16
    FILT8x4   m0, m1, m2, m3, m4, m5, 0
    packuswb  m0, m4
    packuswb  m1, m5
    movq    [r1], m0
    movhps  [r2], m0
    movq    [r3], m1
    movhps  [r4], m1
    mova      m0, m2
    mova      m1, m3
    sub      r6d, 8
%endif ; mmsize
.hloop:
    sub       r0, mmsize*2
    sub       r1, mmsize
    sub       r2, mmsize
    sub       r3, mmsize
    sub       r4, mmsize
%ifdef m8
    FILT8x4   m0, m1, m2, m3, m10, m11, mmsize
    mova      m8, m0
    mova      m9, m1
    FILT8x4   m2, m3, m0, m1, m4, m5, 0
    packuswb  m2, m8
    packuswb  m3, m9
    packuswb  m4, m10
    packuswb  m5, m11
    mova    [r1], m2
    mova    [r2], m4
    mova    [r3], m3
    mova    [r4], m5
%elifidn %1, mmxext
    FILT8x2U  r1, r2, 0
    FILT8x2U  r3, r4, r5
%else
    FILT16x2  m0, r1, r2, 0
    FILT16x2  m1, r3, r4, r5
%endif
    sub      r6d, mmsize
    jg .hloop
.skip:
    mov       r6, dst_gap
    sub       r0, src_gap
    sub       r1, r6
    sub       r2, r6
    sub       r3, r6
    sub       r4, r6
    dec    dword r8m
    jg .vloop
    ADD      rsp, 2*gprsize
    emms
    RET
%endmacro ; FRAME_INIT_LOWRES

INIT_MMX
%define PALIGNR PALIGNR_MMX
FRAME_INIT_LOWRES mmxext
%ifndef ARCH_X86_64
FRAME_INIT_LOWRES cache32_mmxext
%endif
INIT_XMM
FRAME_INIT_LOWRES sse2, 12
%define PALIGNR PALIGNR_SSSE3
FRAME_INIT_LOWRES ssse3, 12

;-----------------------------------------------------------------------------
; void mbtree_propagate_cost( int *dst, uint16_t *propagate_in, uint16_t *intra_costs,
;                             uint16_t *inter_costs, uint16_t *inv_qscales, int len )
;-----------------------------------------------------------------------------
cglobal mbtree_propagate_cost_sse2, 6,6,7
    shl r5d, 1
    lea r0, [r0+r5*2]
    add r1, r5
    add r2, r5
    add r3, r5
    add r4, r5
    neg r5
    pxor      xmm5, xmm5
    movdqa    xmm6, [pw_3fff]
    movdqa    xmm4, [pd_128]
.loop:
    movq      xmm2, [r2+r5] ; intra
    movq      xmm0, [r4+r5] ; invq
    movq      xmm3, [r3+r5] ; inter
    movq      xmm1, [r1+r5] ; prop
    punpcklwd xmm2, xmm5
    punpcklwd xmm0, xmm5
    pmaddwd   xmm0, xmm2
    pand      xmm3, xmm6
    punpcklwd xmm1, xmm5
    punpcklwd xmm3, xmm5
    paddd     xmm0, xmm4
    psrld     xmm0, 8       ; intra*invq>>8
    paddd     xmm0, xmm1    ; prop + (intra*invq>>8)
    cvtdq2ps  xmm1, xmm2    ; intra
    psubd     xmm2, xmm3    ; intra - inter
    rcpps     xmm3, xmm1    ; 1 / intra 1st approximation
    cvtdq2ps  xmm0, xmm0
    mulps     xmm1, xmm3    ; intra * (1/intra 1st approx)
    cvtdq2ps  xmm2, xmm2
    mulps     xmm1, xmm3    ; intra * (1/intra 1st approx)^2
    mulps     xmm0, xmm2    ; (prop + (intra*invq>>8)) * (intra - inter)
    addps     xmm3, xmm3    ; 2 * (1/intra 1st approx)
    subps     xmm3, xmm1    ; 2nd approximation for 1/intra
    mulps     xmm0, xmm3    ; / intra
    cvttps2dq xmm0, xmm0    ; truncation isn't really desired, but matches the integer implementation
    movdqa [r0+r5*2], xmm0
    add r5, 8
    jl .loop
    REP_RET


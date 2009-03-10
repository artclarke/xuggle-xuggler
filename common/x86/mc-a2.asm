;*****************************************************************************
;* mc-a2.asm: h264 encoder library
;*****************************************************************************
;* Copyright (C) 2005-2008 x264 project
;*
;* Authors: Loren Merritt <lorenm@u.washington.edu>
;*          Jason Garrett-Glaser <darkshikari@gmail.com>
;*          Holger Lubitz <hal@duncan.ol.sub.de>
;*          Mathieu Monnier <manao@melix.net>
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

filt_mul20: times 16 db 20
filt_mul51: times 8 db 1, -5

pw_1:  times 8 dw 1
pw_16: times 8 dw 16
pw_32: times 8 dw 32

SECTION .text

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

%macro FILT_V2 0
    psubw  m1, m2  ; a-b
    psubw  m4, m5
    psubw  m2, m3  ; b-c
    psubw  m5, m6
    psllw  m2, 2
    psllw  m5, 2
    psubw  m1, m2  ; a-5*b+4*c
    psubw  m4, m5
    psllw  m3, 4
    psllw  m6, 4
    paddw  m1, m3  ; a-5*b+20*c
    paddw  m4, m6
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

%macro FILT_PACK 3
    paddw     %1, m7
    paddw     %2, m7
    psraw     %1, %3
    psraw     %2, %3
    packuswb  %1, %2
%endmacro

INIT_MMX

%macro HPEL_V 1-2 0
;-----------------------------------------------------------------------------
; void x264_hpel_filter_v_mmxext( uint8_t *dst, uint8_t *src, int16_t *buf, int stride, int width );
;-----------------------------------------------------------------------------
cglobal x264_hpel_filter_v_%1, 5,6,%2
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
    mova m0, [filt_mul51 GLOBAL]
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
    pmaddubsw m3, [filt_mul20 GLOBAL]
    pmaddubsw m6, [filt_mul20 GLOBAL]
    paddw  m1, m2
    paddw  m4, m5
    paddw  m1, m3
    paddw  m4, m6
%else
    LOAD_ADD_2 m1, m4, [r1     ], [r5+r3*2], m6, m7            ; a0 / a1
    LOAD_ADD_2 m2, m5, [r1+r3  ], [r5+r3  ], m6, m7            ; b0 / b1
    LOAD_ADD   m3,     [r1+r3*2], [r5     ], m7                ; c0
    LOAD_ADD   m6,     [r1+r3*2+mmsize/2], [r5+mmsize/2], m7 ; c1
    FILT_V2
%endif
    mova      m7, [pw_16 GLOBAL]
    mova      [r2+r4*2], m1
    mova      [r2+r4*2+mmsize], m4
    paddw     m1, m7
    paddw     m4, m7
    psraw     m1, 5
    psraw     m4, 5
    packuswb  m1, m4
    mova     [r0+r4], m1
    add r1, mmsize
    add r5, mmsize
    add r4, mmsize
    jl .loop
    REP_RET
%endmacro
HPEL_V mmxext

;-----------------------------------------------------------------------------
; void x264_hpel_filter_c_mmxext( uint8_t *dst, int16_t *buf, int width );
;-----------------------------------------------------------------------------
cglobal x264_hpel_filter_c_mmxext, 3,3
    add r0, r2
    lea r1, [r1+r2*2]
    neg r2
    %define src r1+r2*2
    movq m7, [pw_32 GLOBAL]
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
    FILT_PACK m1, m4, 6
    movntq [r0+r2], m1
    add r2, 8
    jl .loop
    REP_RET

;-----------------------------------------------------------------------------
; void x264_hpel_filter_h_mmxext( uint8_t *dst, uint8_t *src, int width );
;-----------------------------------------------------------------------------
cglobal x264_hpel_filter_h_mmxext, 3,3
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
    movq       m7, [pw_1 GLOBAL]
    FILT_H2 m1, m2, m3, m4, m5, m6
    FILT_PACK m1, m4, 1
    movntq     [r0+r2], m1
    add r2, 8
    jl .loop
    REP_RET

INIT_XMM

%macro HPEL_C 1
;-----------------------------------------------------------------------------
; void x264_hpel_filter_c_sse2( uint8_t *dst, int16_t *buf, int width );
;-----------------------------------------------------------------------------
cglobal x264_hpel_filter_c_%1, 3,3,9
    add r0, r2
    lea r1, [r1+r2*2]
    neg r2
    %define src r1+r2*2
%ifidn %1, ssse3
    mova    m7, [pw_32 GLOBAL]
    %define tpw_32 m7
%elifdef ARCH_X86_64
    mova    m8, [pw_32 GLOBAL]
    %define tpw_32 m8
%else
    %define tpw_32 [pw_32 GLOBAL]
%endif
.loop:
%ifidn %1,sse2_misalign
    movu    m0, [src-4]
    movu    m1, [src-2]
    mova    m2, [src]
    paddw   m0, [src+6]
    paddw   m1, [src+4]
    paddw   m2, [src+2]
%else
    mova    m6, [src-16]
    mova    m2, [src]
    mova    m3, [src+16]
    mova    m0, m2
    mova    m1, m2
    mova    m4, m3
    mova    m5, m3
    PALIGNR m3, m2, 2, m7
    PALIGNR m4, m2, 4, m7
    PALIGNR m5, m2, 6, m7
    PALIGNR m0, m6, 12, m7
    PALIGNR m1, m6, 14, m7
    paddw   m2, m3
    paddw   m1, m4
    paddw   m0, m5
%endif
    FILT_H  m0, m1, m2
    paddw   m0, tpw_32
    psraw   m0, 6
    packuswb m0, m0
    movq [r0+r2], m0
    add r2, 8
    jl .loop
    REP_RET
%endmacro

;-----------------------------------------------------------------------------
; void x264_hpel_filter_h_sse2( uint8_t *dst, uint8_t *src, int width );
;-----------------------------------------------------------------------------
cglobal x264_hpel_filter_h_sse2, 3,3,8
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
    mova       m7, [pw_1 GLOBAL] ; FIXME xmm8
    FILT_H2 m1, m2, m3, m4, m5, m6
    FILT_PACK m1, m4, 1
    movntdq    [r0+r2], m1
    add r2, 16
    jl .loop
    REP_RET

%ifndef ARCH_X86_64
;-----------------------------------------------------------------------------
; void x264_hpel_filter_h_ssse3( uint8_t *dst, uint8_t *src, int width );
;-----------------------------------------------------------------------------
cglobal x264_hpel_filter_h_ssse3, 3,3
    add r0, r2
    add r1, r2
    neg r2
    %define src r1+r2
    pxor m0, m0
    movh m1, [src-8]
    punpcklbw m1, m0         ; 00 -1 00 -2 00 -3 00 -4 00 -5 00 -6 00 -7 00 -8
    movh m2, [src]
    punpcklbw m2, m0
    mova       m7, [pw_1 GLOBAL]
.loop:
    movh       m3, [src+8]
    punpcklbw  m3, m0

    mova       m4, m2
    palignr    m2, m1, 14
    mova       m5, m3
    palignr    m3, m4, 4
    paddw      m3, m2

    mova       m2, m4
    palignr    m4, m1, 12
    mova       m1, m5
    palignr    m5, m2, 6
    paddw      m5, m4

    mova       m4, m1
    palignr    m1, m2, 2
    paddw      m1, m2

    FILT_H     m5, m3, m1

    movh       m1, [src+16]
    punpcklbw  m1, m0

    mova       m3, m4
    palignr    m4, m2, 14
    mova       m6, m1
    palignr    m1, m3, 4
    paddw      m1, m4

    mova       m4, m3
    palignr    m3, m2, 12
    mova       m2, m6
    palignr    m6, m4, 6
    paddw      m6, m3

    mova       m3, m2
    palignr    m2, m4, 2
    paddw      m2, m4

    FILT_H m6, m1, m2
    FILT_PACK m5, m6, 1
    movdqa    [r0+r2], m5

    add r2, 16
    mova      m2, m3
    mova      m1, m4

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
%ifidn %6, ssse3
    mova m1, [r3]
    mova m2, [r3+r2]
    mova %3, [r3+r2*2]
    mova m3, [r1]
    mova %4, [r1+r2]
    mova m0, [r1+r2*2]
    mova %2, [filt_mul51 GLOBAL]
    mova m4, m1
    punpcklbw m1, m2
    punpckhbw m4, m2
    mova m2, m0
    punpcklbw m0, %4
    punpckhbw m2, %4
    mova %1, m3
    punpcklbw m3, %3
    punpckhbw %1, %3
    mova %3, m3
    mova %4, %1
    pmaddubsw m1, %2
    pmaddubsw m4, %2
    pmaddubsw m0, %2
    pmaddubsw m2, %2
    pmaddubsw m3, [filt_mul20 GLOBAL]
    pmaddubsw %1, [filt_mul20 GLOBAL]
    psrlw     %3, 8
    psrlw     %4, 8
    paddw m1, m0
    paddw m4, m2
    paddw m1, m3
    paddw m4, %1
%else
    LOAD_ADD_2 m1, m4, [r3     ], [r1+r2*2], m2, m5            ; a0 / a1
    LOAD_ADD_2 m2, m5, [r3+r2  ], [r1+r2  ], m3, m6            ; b0 / b1
    LOAD_ADD_2 m3, m6, [r3+r2*2], [r1     ], %3, %4            ; c0 / c1
    FILT_V2
%endif
    mova      %1, m1
    mova      %2, m4
    paddw     m1, m15
    paddw     m4, m15
    add       r3, 16
    add       r1, 16
    psraw     m1, 5
    psraw     m4, 5
    packuswb  m1, m4
    movntps  [r11+r4+%5], m1
%endmacro

%macro DO_FILT_H 4
    mova      m1, %2
    PALIGNR   m1, %1, 12, m4
    mova      m2, %2
    PALIGNR   m2, %1, 14, m4
    mova      %1, %3
    PALIGNR   %3, %2, 6, m4
    mova      m3, %1
    PALIGNR   m3, %2, 4, m4
    mova      m4, %1
    paddw     %3, m1
    PALIGNR   m4, %2, 2, m1
    paddw     m3, m2
    paddw     m4, %2
    FILT_H    %3, m3, m4
    paddw     %3, m15
    psraw     %3, %4
%endmacro

%macro DO_FILT_CC 4
    DO_FILT_H %1, %2, %3, 6
    DO_FILT_H %2, %1, %4, 6
    packuswb  %3, %4
    movntps   [r5+r4], %3
%endmacro

%macro DO_FILT_HH 4
    DO_FILT_H %1, %2, %3, 1
    DO_FILT_H %2, %1, %4, 1
    packuswb  %3, %4
    movntps   [r0+r4], %3
%endmacro

%macro DO_FILT_H2 6
    DO_FILT_H %1, %2, %3, 6
    psrlw    m15, 5
    DO_FILT_H %4, %5, %6, 1
    packuswb  %6, %3
%endmacro

%macro HPEL 1
;-----------------------------------------------------------------------------
; void x264_hpel_filter_sse2( uint8_t *dsth, uint8_t *dstv, uint8_t *dstc,
;                             uint8_t *src, int stride, int width, int height)
;-----------------------------------------------------------------------------
cglobal x264_hpel_filter_%1, 7,7,16
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
%ifidn %1, sse2
    pxor      m0, m0
%endif
    pcmpeqw  m15, m15
    psrlw    m15, 15 ; pw_1
    psllw    m15, 4
;ALIGN 16
.loopy:
; first filter_v
; prefetching does not help here! lots of variants tested, all slower
    DO_FILT_V m8, m7, m13, m12, 0, %1
;ALIGN 16
.loopx:
    DO_FILT_V m6, m5, m11, m10, 16, %1
.lastx:
    paddw    m15, m15
    DO_FILT_CC m9, m8, m7, m6
    movdqa   m7, m12        ; not really necessary, but seems free and
    movdqa   m6, m11	     ; gives far shorter code
    psrlw    m15, 5
    DO_FILT_HH m14, m13, m7, m6
    psllw    m15, 4 ; pw_16
    movdqa   m7, m5
    movdqa  m12, m10
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

cglobal x264_sfence
    sfence
    ret

;-----------------------------------------------------------------------------
; void x264_plane_copy_mmxext( uint8_t *dst, int i_dst,
;                              uint8_t *src, int i_src, int w, int h)
;-----------------------------------------------------------------------------
cglobal x264_plane_copy_mmxext, 6,7
    movsxdifnidn r1, r1d
    movsxdifnidn r3, r3d
    add    r4d, 3
    and    r4d, ~3
    mov    r6d, r4d
    and    r6d, ~15
    sub    r1,  r6
    sub    r3,  r6
.loopy:
    mov    r6d, r4d
    sub    r6d, 64
    jl     .endx
.loopx:
    prefetchnta [r2+256]
    movq   mm0, [r2   ]
    movq   mm1, [r2+ 8]
    movq   mm2, [r2+16]
    movq   mm3, [r2+24]
    movq   mm4, [r2+32]
    movq   mm5, [r2+40]
    movq   mm6, [r2+48]
    movq   mm7, [r2+56]
    movntq [r0   ], mm0
    movntq [r0+ 8], mm1
    movntq [r0+16], mm2
    movntq [r0+24], mm3
    movntq [r0+32], mm4
    movntq [r0+40], mm5
    movntq [r0+48], mm6
    movntq [r0+56], mm7
    add    r2,  64
    add    r0,  64
    sub    r6d, 64
    jge    .loopx
.endx:
    prefetchnta [r2+256]
    add    r6d, 48
    jl .end16
.loop16:
    movq   mm0, [r2  ]
    movq   mm1, [r2+8]
    movntq [r0  ], mm0
    movntq [r0+8], mm1
    add    r2,  16
    add    r0,  16
    sub    r6d, 16
    jge    .loop16
.end16:
    add    r6d, 12
    jl .end4
.loop4:
    movd   mm2, [r2+r6]
    movd   [r0+r6], mm2
    sub    r6d, 4
    jge .loop4
.end4:
    add    r2, r3
    add    r0, r1
    dec    r5d
    jg     .loopy
    sfence
    emms
    RET



; These functions are not general-use; not only do the SSE ones require aligned input,
; but they also will fail if given a non-mod16 size or a size less than 64.
; memzero SSE will fail for non-mod128.

;-----------------------------------------------------------------------------
; void *x264_memcpy_aligned_mmx( void *dst, const void *src, size_t n );
;-----------------------------------------------------------------------------
cglobal x264_memcpy_aligned_mmx, 3,3
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
; void *x264_memcpy_aligned_sse2( void *dst, const void *src, size_t n );
;-----------------------------------------------------------------------------
cglobal x264_memcpy_aligned_sse2, 3,3
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
; void *x264_memzero_aligned( void *dst, size_t n );
;-----------------------------------------------------------------------------
%macro MEMZERO 1
cglobal x264_memzero_aligned_%1, 2,2
    pxor m0, m0
.loop:
    sub r1d, mmsize*8
%assign i 0
%rep 8
    mova [r0 + r1 + i], m0
%assign i i+mmsize
%endrep
    jg .loop
    REP_RET
%endmacro

INIT_MMX
MEMZERO mmx
INIT_XMM
MEMZERO sse2



;-----------------------------------------------------------------------------
; void x264_integral_init4h_sse4( uint16_t *sum, uint8_t *pix, int stride )
;-----------------------------------------------------------------------------
cglobal x264_integral_init4h_sse4, 3,4
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

cglobal x264_integral_init8h_sse4, 3,4
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

%macro INTEGRAL_INIT 1
;-----------------------------------------------------------------------------
; void x264_integral_init4v_mmx( uint16_t *sum8, uint16_t *sum4, int stride )
;-----------------------------------------------------------------------------
cglobal x264_integral_init4v_%1, 3,5
    shl   r2, 1
    add   r0, r2
    add   r1, r2
    lea   r3, [r0+r2*4]
    lea   r4, [r0+r2*8]
    neg   r2
.loop:
    movu  m0, [r0+r2+8]
    mova  m2, [r0+r2]
    movu  m1, [r4+r2+8]
    paddw m0, m2
    paddw m1, [r4+r2]
    mova  m3, [r3+r2]
    psubw m1, m0
    psubw m3, m2
    mova  [r0+r2], m1
    mova  [r1+r2], m3
    add   r2, mmsize
    jl .loop
    REP_RET

;-----------------------------------------------------------------------------
; void x264_integral_init8v_mmx( uint16_t *sum8, int stride )
;-----------------------------------------------------------------------------
cglobal x264_integral_init8v_%1, 3,3
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
INTEGRAL_INIT mmx
INIT_XMM
INTEGRAL_INIT sse2



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
cglobal x264_frame_init_lowres_core_%1, 6,7,%2
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

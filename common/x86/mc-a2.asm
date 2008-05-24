;*****************************************************************************
;* mc-a2.asm: h264 encoder library
;*****************************************************************************
;* Copyright (C) 2005-2008 x264 project
;*
;* Authors: Loren Merritt <lorenm@u.washington.edu>
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
;* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111, USA.
;*****************************************************************************

%include "x86inc.asm"

SECTION_RODATA

pw_1:  times 8 dw 1
pw_16: times 8 dw 16
pw_32: times 8 dw 32

SECTION .text

%macro LOAD_ADD 3
    movh       %1, %2
    movh       m7, %3
    punpcklbw  %1, m0
    punpcklbw  m7, m0
    paddw      %1, m7
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

%macro FILT_H2 0
    psubw  m1, m2
    psubw  m4, m5
    psraw  m1, 2
    psraw  m4, 2
    psubw  m1, m2
    psubw  m4, m5
    paddw  m1, m3
    paddw  m4, m6
    psraw  m1, 2
    psraw  m4, 2
    paddw  m1, m3
    paddw  m4, m6
%endmacro

%macro FILT_PACK 1
    paddw     m1, m7
    paddw     m4, m7
    psraw     m1, %1
    psraw     m4, %1
    packuswb  m1, m4
%endmacro

%macro PALIGNR_SSE2 4
    %ifnidn %2, %4
    movdqa %4, %2
    %endif
    pslldq %1, 16-%3
    psrldq %4, %3
    por    %1, %4
%endmacro

%macro PALIGNR_SSSE3 4
    palignr %1, %2, %3
%endmacro

INIT_MMX

%macro HPEL_V 1
;-----------------------------------------------------------------------------
; void x264_hpel_filter_v_mmxext( uint8_t *dst, uint8_t *src, int16_t *buf, int stride, int width );
;-----------------------------------------------------------------------------
cglobal x264_hpel_filter_v_%1, 5,6,1
    lea r5, [r1+r3]
    sub r1, r3
    sub r1, r3
    add r0, r4
    lea r2, [r2+r4*2]
    neg r4
    pxor m0, m0
.loop:
    prefetcht0 [r5+r3*2+64]
    LOAD_ADD  m1, [r1     ], [r5+r3*2] ; a0
    LOAD_ADD  m2, [r1+r3  ], [r5+r3  ] ; b0
    LOAD_ADD  m3, [r1+r3*2], [r5     ] ; c0
    LOAD_ADD  m4, [r1     +regsize/2], [r5+r3*2+regsize/2] ; a1
    LOAD_ADD  m5, [r1+r3  +regsize/2], [r5+r3  +regsize/2] ; b1
    LOAD_ADD  m6, [r1+r3*2+regsize/2], [r5     +regsize/2] ; c1
    FILT_V2
    mova      m7, [pw_16 GLOBAL]
    mova      [r2+r4*2], m1
    mova      [r2+r4*2+regsize], m4
    paddw     m1, m7
    paddw     m4, m7
    psraw     m1, 5
    psraw     m4, 5
    packuswb  m1, m4
    movnt     [r0+r4], m1
    add r1, regsize
    add r5, regsize
    add r4, regsize
    jl .loop
    REP_RET
%endmacro
HPEL_V mmxext

;-----------------------------------------------------------------------------
; void x264_hpel_filter_c_mmxext( uint8_t *dst, int16_t *buf, int width );
;-----------------------------------------------------------------------------
cglobal x264_hpel_filter_c_mmxext, 3,3,1
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
    FILT_H2
    FILT_PACK 6
    movntq [r0+r2], m1
    add r2, 8
    jl .loop
    REP_RET

;-----------------------------------------------------------------------------
; void x264_hpel_filter_h_mmxext( uint8_t *dst, uint8_t *src, int width );
;-----------------------------------------------------------------------------
cglobal x264_hpel_filter_h_mmxext, 3,3,1
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
    FILT_H2
    FILT_PACK 1
    movntq     [r0+r2], m1
    add r2, 8
    jl .loop
    REP_RET

INIT_XMM

%macro HPEL_C 1
;-----------------------------------------------------------------------------
; void x264_hpel_filter_c_sse2( uint8_t *dst, int16_t *buf, int width );
;-----------------------------------------------------------------------------
cglobal x264_hpel_filter_c_%1, 3,3,1
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
cglobal x264_hpel_filter_h_sse2, 3,3,1
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
    FILT_H2
    FILT_PACK 1
    movntdq    [r0+r2], m1
    add r2, 16
    jl .loop
    REP_RET

%define PALIGNR PALIGNR_SSE2
HPEL_V sse2
HPEL_C sse2
%define PALIGNR PALIGNR_SSSE3
HPEL_C ssse3

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
    sub r1d, regsize*8
%assign i 0
%rep 8
    mova [r0 + r1 + i], m0
%assign i i+regsize
%endrep
    jg .loop
    REP_RET
%endmacro

INIT_MMX
MEMZERO mmx
INIT_XMM
MEMZERO sse2

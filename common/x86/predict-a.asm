;*****************************************************************************
;* predict-a.asm: x86 intra prediction
;*****************************************************************************
;* Copyright (C) 2005-2010 x264 project
;*
;* Authors: Loren Merritt <lorenm@u.washington.edu>
;*          Holger Lubitz <holger@lubitz.org>
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

SECTION_RODATA

pw_76543210:
pw_3210:    dw 0, 1, 2, 3, 4, 5, 6, 7
pb_00s_ff:  times 8 db 0
pb_0s_ff:   times 7 db 0
            db 0xff

SECTION .text

cextern pb_0
cextern pb_1
cextern pb_3
cextern pw_1
cextern pw_2
cextern pw_4
cextern pw_8
cextern pw_ff00
cextern pb_reverse

%macro STORE8x8 2
    add r0, 4*FDEC_STRIDEB
    mova        [r0 + -4*FDEC_STRIDEB], %1
    mova        [r0 + -3*FDEC_STRIDEB], %1
    mova        [r0 + -2*FDEC_STRIDEB], %1
    mova        [r0 + -1*FDEC_STRIDEB], %1
    mova        [r0 +  0*FDEC_STRIDEB], %2
    mova        [r0 +  1*FDEC_STRIDEB], %2
    mova        [r0 +  2*FDEC_STRIDEB], %2
    mova        [r0 +  3*FDEC_STRIDEB], %2
%endmacro

%macro STORE16x16 2-4
%ifidn %0, 4
    mov         r1d, 8
.loop:
    mova        [r0 + 0*FDEC_STRIDEB + 0], %1
    mova        [r0 + 1*FDEC_STRIDEB + 0], %1
    mova        [r0 + 0*FDEC_STRIDEB + 8], %2
    mova        [r0 + 1*FDEC_STRIDEB + 8], %2
    mova        [r0 + 0*FDEC_STRIDEB +16], %3
    mova        [r0 + 1*FDEC_STRIDEB +16], %3
    mova        [r0 + 0*FDEC_STRIDEB +24], %4
    mova        [r0 + 1*FDEC_STRIDEB +24], %4
    add         r0, 2*FDEC_STRIDEB
    dec         r1d
    jg          .loop
%else
    mov         r1d, 4
.loop:
    mova        [r0 + 0*FDEC_STRIDE], %1
    mova        [r0 + 1*FDEC_STRIDE], %1
    mova        [r0 + 2*FDEC_STRIDE], %1
    mova        [r0 + 3*FDEC_STRIDE], %1
    mova        [r0 + 0*FDEC_STRIDE + 8], %2
    mova        [r0 + 1*FDEC_STRIDE + 8], %2
    mova        [r0 + 2*FDEC_STRIDE + 8], %2
    mova        [r0 + 3*FDEC_STRIDE + 8], %2
    add         r0, 4*FDEC_STRIDE
    dec         r1d
    jg          .loop
%endif
%endmacro

%macro STORE16x16_SSE2 1-2
%ifidn %0,2
    mov r1d, 4
.loop
    mova      [r0+0*FDEC_STRIDEB+ 0], %1
    mova      [r0+0*FDEC_STRIDEB+16], %2
    mova      [r0+1*FDEC_STRIDEB+ 0], %1
    mova      [r0+1*FDEC_STRIDEB+16], %2
    mova      [r0+2*FDEC_STRIDEB+ 0], %1
    mova      [r0+2*FDEC_STRIDEB+16], %2
    mova      [r0+3*FDEC_STRIDEB+ 0], %1
    mova      [r0+3*FDEC_STRIDEB+16], %2
    add       r0, 4*FDEC_STRIDEB
    dec       r1d
    jg        .loop
%else
    add r0, 4*FDEC_STRIDEB
    mova      [r0 + -4*FDEC_STRIDEB], %1
    mova      [r0 + -3*FDEC_STRIDEB], %1
    mova      [r0 + -2*FDEC_STRIDEB], %1
    mova      [r0 + -1*FDEC_STRIDEB], %1
    mova      [r0 +  0*FDEC_STRIDEB], %1
    mova      [r0 +  1*FDEC_STRIDEB], %1
    mova      [r0 +  2*FDEC_STRIDEB], %1
    mova      [r0 +  3*FDEC_STRIDEB], %1
    add r0, 8*FDEC_STRIDEB
    mova      [r0 + -4*FDEC_STRIDEB], %1
    mova      [r0 + -3*FDEC_STRIDEB], %1
    mova      [r0 + -2*FDEC_STRIDEB], %1
    mova      [r0 + -1*FDEC_STRIDEB], %1
    mova      [r0 +  0*FDEC_STRIDEB], %1
    mova      [r0 +  1*FDEC_STRIDEB], %1
    mova      [r0 +  2*FDEC_STRIDEB], %1
    mova      [r0 +  3*FDEC_STRIDEB], %1
%endif
%endmacro

; dest, left, right, src, tmp
; output: %1 = (t[n-1] + t[n]*2 + t[n+1] + 2) >> 2
%macro PRED8x8_LOWPASS 5-6
%ifidn %1, w
    mova        %2, %5
    paddw       %3, %4
    psrlw       %3, 1
    pavgw       %2, %3
%else
    mova        %6, %3
    pavgb       %3, %4
    pxor        %4, %6
    mova        %2, %5
    pand        %4, [pb_1]
    psubusb     %3, %4
    pavgb       %2, %3
%endif
%endmacro

%macro LOAD_PLANE_ARGS 0
%ifdef ARCH_X86_64
    movd        mm0, r1d
    movd        mm2, r2d
    movd        mm4, r3d
    pshufw      mm0, mm0, 0
    pshufw      mm2, mm2, 0
    pshufw      mm4, mm4, 0
%else
    pshufw      mm0, r1m, 0
    pshufw      mm2, r2m, 0
    pshufw      mm4, r3m, 0
%endif
%endmacro

;-----------------------------------------------------------------------------
; void predict_4x4_ddl( pixel *src )
;-----------------------------------------------------------------------------
%macro PREDICT_4x4_DDL 4
cglobal predict_4x4_ddl_%1, 1,1
    mova    m1, [r0-FDEC_STRIDEB]
    mova    m2, m1
    mova    m3, m1
    mova    m4, m1
    psll%2  m1, %3
    pxor    m2, m1
    psrl%2  m2, %3
    pxor    m3, m2
    PRED8x8_LOWPASS %4, m0, m1, m3, m4, m5

%assign Y 0
%rep 4
    psrl%2      m0, %3
    movh        [r0+Y*FDEC_STRIDEB], m0
%assign Y (Y+1)
%endrep

    RET
%endmacro

%ifdef HIGH_BIT_DEPTH
INIT_XMM
PREDICT_4x4_DDL sse2, dq, 2, w
INIT_MMX
%define PALIGNR PALIGNR_MMX
cglobal predict_4x4_ddl_mmxext, 1,2
    mova    m1, [r0-2*FDEC_STRIDE+4]
    mova    m2, [r0-2*FDEC_STRIDE+0]
    mova    m3, [r0-2*FDEC_STRIDE+2]
    PRED8x8_LOWPASS w, m0, m1, m2, m3
    mova    [r0+0*FDEC_STRIDE], m0

    mova    m5, [r0-2*FDEC_STRIDE+6]
    mova    m6, [r0-2*FDEC_STRIDE+8]
    pshufw  m7, m6, 0xF9
    PRED8x8_LOWPASS w, m4, m7, m5, m6
    mova    [r0+6*FDEC_STRIDE], m4

    psllq   m0, 16
    PALIGNR m4, m0, 6, m1
    mova    [r0+4*FDEC_STRIDE], m4

    psllq   m0, 16
    PALIGNR m4, m0, 6, m0
    mova    [r0+2*FDEC_STRIDE], m4
    RET
%else
INIT_MMX
PREDICT_4x4_DDL mmxext, q , 8, b
%endif

;-----------------------------------------------------------------------------
; void predict_4x4_ddr( pixel *src )
;-----------------------------------------------------------------------------
%macro PREDICT_4x4 7
cglobal predict_4x4_ddr_%1, 1,1
    mova      m1, [r0+1*FDEC_STRIDEB-8*SIZEOF_PIXEL]
    mova      m2, [r0+0*FDEC_STRIDEB-8*SIZEOF_PIXEL]
    punpckh%2 m2, [r0-1*FDEC_STRIDEB-8*SIZEOF_PIXEL]
    movh      m3, [r0-1*FDEC_STRIDEB]
    punpckh%3 m1, m2
    PALIGNR   m3, m1, 5*SIZEOF_PIXEL, m1
    mova      m1, m3
    PALIGNR   m3, [r0+2*FDEC_STRIDEB-8*SIZEOF_PIXEL], 7*SIZEOF_PIXEL, m4
    mova      m2, m3
    PALIGNR   m3, [r0+3*FDEC_STRIDEB-8*SIZEOF_PIXEL], 7*SIZEOF_PIXEL, m4
    PRED8x8_LOWPASS %5, m0, m3, m1, m2, m4
%assign Y 3
    movh      [r0+Y*FDEC_STRIDEB], m0
%rep 3
%assign Y (Y-1)
    psrl%4    m0, %7
    movh      [r0+Y*FDEC_STRIDEB], m0
%endrep
    RET

cglobal predict_4x4_vr_%1, 1,1,6*(mmsize/16)
    movh    m0, [r0-1*FDEC_STRIDEB]                                       ; ........t3t2t1t0
    mova    m5, m0
    PALIGNR m0, [r0-1*FDEC_STRIDEB-8*SIZEOF_PIXEL], 7*SIZEOF_PIXEL, m1    ; ......t3t2t1t0lt
    pavg%5  m5, m0
    PALIGNR m0, [r0+0*FDEC_STRIDEB-8*SIZEOF_PIXEL], 7*SIZEOF_PIXEL, m1    ; ....t3t2t1t0ltl0
    mova    m1, m0
    PALIGNR m0, [r0+1*FDEC_STRIDEB-8*SIZEOF_PIXEL], 7*SIZEOF_PIXEL, m2    ; ..t3t2t1t0ltl0l1
    mova    m2, m0
    PALIGNR m0, [r0+2*FDEC_STRIDEB-8*SIZEOF_PIXEL], 7*SIZEOF_PIXEL, m3    ; t3t2t1t0ltl0l1l2
    PRED8x8_LOWPASS %5, m3, m1, m0, m2, m4
    mova    m1, m3
    psrl%4  m3, %7*2
    psll%4  m1, %7*6
    movh    [r0+0*FDEC_STRIDEB], m5
    movh    [r0+1*FDEC_STRIDEB], m3
    PALIGNR m5, m1, 7*SIZEOF_PIXEL, m2
    psll%4  m1, %7
    movh    [r0+2*FDEC_STRIDEB], m5
    PALIGNR m3, m1, 7*SIZEOF_PIXEL, m1
    movh    [r0+3*FDEC_STRIDEB], m3
    RET

cglobal predict_4x4_hd_%1, 1,1,6*(mmsize/16)
    movh      m0, [r0-1*FDEC_STRIDEB-4*SIZEOF_PIXEL] ; lt ..
    punpckl%6 m0, [r0-1*FDEC_STRIDEB]                ; t3 t2 t1 t0 lt .. .. ..
    psll%4    m0, %7                                 ; t2 t1 t0 lt .. .. .. ..
    mova      m1, [r0+3*FDEC_STRIDEB-8*SIZEOF_PIXEL] ; l3
    punpckh%2 m1, [r0+2*FDEC_STRIDEB-8*SIZEOF_PIXEL] ; l2 l3
    mova      m2, [r0+1*FDEC_STRIDEB-8*SIZEOF_PIXEL] ; l1
    punpckh%2 m2, [r0+0*FDEC_STRIDEB-8*SIZEOF_PIXEL] ; l0 l1
    punpckh%3 m1, m2                                 ; l0 l1 l2 l3
    punpckh%6 m1, m0                                 ; t2 t1 t0 lt l0 l1 l2 l3
    mova      m0, m1
    mova      m2, m1
    mova      m5, m1
    psrl%4    m0, %7*2                               ; .. .. t2 t1 t0 lt l0 l1
    psrl%4    m2, %7                                 ; .. t2 t1 t0 lt l0 l1 l2
    pavg%5    m5, m2
    PRED8x8_LOWPASS %5, m3, m1, m0, m2, m4
    punpckl%2 m5, m3
    psrl%4    m3, %7*4
    PALIGNR   m3, m5, 6*SIZEOF_PIXEL, m4
%assign Y 3
    movh      [r0+Y*FDEC_STRIDEB], m5
%rep 2
%assign Y (Y-1)
    psrl%4    m5, %7*2
    movh      [r0+Y*FDEC_STRIDEB], m5
%endrep
    movh      [r0+0*FDEC_STRIDEB], m3
    RET
%endmacro

%ifdef HIGH_BIT_DEPTH
INIT_XMM
%define PALIGNR PALIGNR_SSSE3
PREDICT_4x4 ssse3 , wd, dq, dq, w, qdq, 2
%else
INIT_MMX
%define PALIGNR PALIGNR_MMX
PREDICT_4x4 mmxext, bw, wd, q , b, dq , 8
%define PALIGNR PALIGNR_SSSE3
PREDICT_4x4 ssse3 , bw, wd, q , b, dq , 8
%endif

;-----------------------------------------------------------------------------
; void predict_4x4_hu( pixel *src )
;-----------------------------------------------------------------------------
%ifdef HIGH_BIT_DEPTH
INIT_XMM
cglobal predict_4x4_hu_sse2, 1,1,6
    movq      mm0, [r0+0*FDEC_STRIDEB-4*2]
    punpckhwd mm0, [r0+1*FDEC_STRIDEB-4*2]
    movq      mm1, [r0+2*FDEC_STRIDEB-4*2]
    punpckhwd mm1, [r0+3*FDEC_STRIDEB-4*2]
    punpckhdq mm0, mm1
    pshufw    mm1, mm1, 0xFF
    movq2dq    m0, mm0
    movq2dq    m1, mm1
    punpcklqdq m0, m1
    mova       m2, m0
    mova       m3, m0
    mova       m1, m0
    psrldq     m2, 4
    psrldq     m3, 2
    pavgw      m1, m3
    PRED8x8_LOWPASS w, m4, m0, m2, m3, m5
    punpcklwd  m1, m4

    movq       [r0+0*FDEC_STRIDEB], m1
    psrldq     m1, 4
    movq       [r0+1*FDEC_STRIDEB], m1
    psrldq     m1, 4
    movq       [r0+2*FDEC_STRIDEB], m1
    movq       [r0+3*FDEC_STRIDEB], mm1
    RET

%else
INIT_MMX
cglobal predict_4x4_hu_mmxext, 1,1
    movq      mm0, [r0+0*FDEC_STRIDE-8]
    punpckhbw mm0, [r0+1*FDEC_STRIDE-8]
    movq      mm1, [r0+2*FDEC_STRIDE-8]
    punpckhbw mm1, [r0+3*FDEC_STRIDE-8]
    punpckhwd mm0, mm1
    movq      mm1, mm0
    punpckhbw mm1, mm1
    pshufw    mm1, mm1, 0xFF
    punpckhdq mm0, mm1
    movq      mm2, mm0
    movq      mm3, mm0
    movq      mm7, mm0
    psrlq     mm2, 16
    psrlq     mm3, 8
    pavgb     mm7, mm3
    PRED8x8_LOWPASS b, mm4, mm0, mm2, mm3, mm5
    punpcklbw mm7, mm4
%assign Y 0
    movd    [r0+Y*FDEC_STRIDE], mm7
%rep 2
%assign Y (Y+1)
    psrlq    mm7, 16
    movd    [r0+Y*FDEC_STRIDE], mm7
%endrep
    movd    [r0+3*FDEC_STRIDE], mm1
    RET
%endif ; HIGH_BIT_DEPTH

;-----------------------------------------------------------------------------
; void predict_4x4_vl( pixel *src )
;-----------------------------------------------------------------------------
%macro PREDICT_4x4_V1 4
cglobal predict_4x4_vl_%1, 1,1,6*(mmsize/16)
    mova        m1, [r0-FDEC_STRIDEB]
    mova        m3, m1
    mova        m2, m1
    psrl%2      m3, %3
    psrl%2      m2, %3*2
    mova        m4, m3
    pavg%4      m4, m1
    PRED8x8_LOWPASS %4, m0, m1, m2, m3, m5

    movh        [r0+0*FDEC_STRIDEB], m4
    movh        [r0+1*FDEC_STRIDEB], m0
    psrl%2      m4, %3
    psrl%2      m0, %3
    movh        [r0+2*FDEC_STRIDEB], m4
    movh        [r0+3*FDEC_STRIDEB], m0
    RET
%endmacro

%ifdef HIGH_BIT_DEPTH
INIT_XMM
PREDICT_4x4_V1 sse2, dq, 2, w

INIT_MMX
%define PALIGNR PALIGNR_MMX
cglobal predict_4x4_vl_mmxext, 1,4
    mova    m1, [r0-FDEC_STRIDEB+0]
    mova    m2, [r0-FDEC_STRIDEB+8]
    mova    m3, m2
    PALIGNR m2, m1, 4, m6
    PALIGNR m3, m1, 2, m5
    mova    m4, m3
    pavgw   m4, m1
    mova    [r0+0*FDEC_STRIDEB], m4
    psrlq   m4, 16
    mova    [r0+2*FDEC_STRIDEB], m4
    PRED8x8_LOWPASS w, m0, m1, m2, m3, m6
    mova    [r0+1*FDEC_STRIDEB], m0
    psrlq   m0, 16
    mova    [r0+3*FDEC_STRIDEB], m0

    movzx   r1d, word [r0-FDEC_STRIDEB+ 8]
    movzx   r2d, word [r0-FDEC_STRIDEB+10]
    movzx   r3d, word [r0-FDEC_STRIDEB+12]
    lea     r1d, [r1+r2+1]
    add     r3d, r2d
    lea     r3d, [r3+r1+1]
    shr     r1d, 1
    shr     r3d, 2
    mov     [r0+2*FDEC_STRIDEB+6], r1w
    mov     [r0+3*FDEC_STRIDEB+6], r3w
    RET
%else
INIT_MMX
PREDICT_4x4_V1 mmxext, q , 8, b
%endif

;-----------------------------------------------------------------------------
; void predict_4x4_dc( pixel *src )
;-----------------------------------------------------------------------------
%ifdef HIGH_BIT_DEPTH
INIT_MMX
cglobal predict_4x4_dc_mmxext, 1,1
    mova   m2, [r0+0*FDEC_STRIDEB-4*SIZEOF_PIXEL]
    paddw  m2, [r0+1*FDEC_STRIDEB-4*SIZEOF_PIXEL]
    paddw  m2, [r0+2*FDEC_STRIDEB-4*SIZEOF_PIXEL]
    paddw  m2, [r0+3*FDEC_STRIDEB-4*SIZEOF_PIXEL]
    psrlq  m2, 48
    mova   m0, [r0-FDEC_STRIDEB]
    HADDW  m0, m1
    paddw  m0, [pw_4]
    paddw  m0, m2
    psrlw  m0, 3
    SPLATW m0, m0
    mova   [r0+0*FDEC_STRIDEB], m0
    mova   [r0+1*FDEC_STRIDEB], m0
    mova   [r0+2*FDEC_STRIDEB], m0
    mova   [r0+3*FDEC_STRIDEB], m0
    RET

%else
INIT_MMX
cglobal predict_4x4_dc_mmxext, 1,4
    pxor   mm7, mm7
    movd   mm0, [r0-FDEC_STRIDE]
    psadbw mm0, mm7
    movd   r3d, mm0
    movzx  r1d, byte [r0-1]
%assign n 1
%rep 3
    movzx  r2d, byte [r0+FDEC_STRIDE*n-1]
    add    r1d, r2d
%assign n n+1
%endrep
    lea    r1d, [r1+r3+4]
    shr    r1d, 3
    imul   r1d, 0x01010101
    mov   [r0+FDEC_STRIDE*0], r1d
    mov   [r0+FDEC_STRIDE*1], r1d
    mov   [r0+FDEC_STRIDE*2], r1d
    mov   [r0+FDEC_STRIDE*3], r1d
    RET
%endif ; HIGH_BIT_DEPTH

%macro PREDICT_FILTER 1
;-----------------------------------------------------------------------------
;void predict_8x8_filter( uint8_t *src, uint8_t edge[33], int i_neighbor, int i_filters )
;-----------------------------------------------------------------------------
INIT_MMX
cglobal predict_8x8_filter_%1, 4,5
    add          r0, 0x58
%define src r0-0x58
%ifndef ARCH_X86_64
    mov          r4, r1
%define t1 r4
%define t4 r1
%else
%define t1 r1
%define t4 r4
%endif
    test        r3b, 0x01
    je .check_top
    movq        mm0, [src+0*FDEC_STRIDE-8]
    punpckhbw   mm0, [src-1*FDEC_STRIDE-8]
    movq        mm1, [src+2*FDEC_STRIDE-8]
    punpckhbw   mm1, [src+1*FDEC_STRIDE-8]
    punpckhwd   mm1, mm0
    movq        mm2, [src+4*FDEC_STRIDE-8]
    punpckhbw   mm2, [src+3*FDEC_STRIDE-8]
    movq        mm3, [src+6*FDEC_STRIDE-8]
    punpckhbw   mm3, [src+5*FDEC_STRIDE-8]
    punpckhwd   mm3, mm2
    punpckhdq   mm3, mm1
    movq        mm0, [src+7*FDEC_STRIDE-8]
    movq        mm1, [src-1*FDEC_STRIDE]
    movq        mm4, mm3
    movq        mm2, mm3
    PALIGNR     mm4, mm0, 7, mm0
    PALIGNR     mm1, mm2, 1, mm2
    test        r2b, 0x08
    je .fix_lt_1
.do_left:
    movq        mm0, mm4
    PRED8x8_LOWPASS b, mm2, mm1, mm4, mm3, mm5
    movq     [t1+8], mm2
    movq        mm4, mm0
    PRED8x8_LOWPASS b, mm1, mm3, mm0, mm4, mm5
    movd         t4, mm1
    mov      [t1+7], t4b
.check_top:
    test        r3b, 0x02
    je .done
    movq        mm0, [src-1*FDEC_STRIDE-8]
    movq        mm3, [src-1*FDEC_STRIDE]
    movq        mm1, [src-1*FDEC_STRIDE+8]
    movq        mm2, mm3
    movq        mm4, mm3
    PALIGNR     mm2, mm0, 7, mm0
    PALIGNR     mm1, mm4, 1, mm4
    test        r2b, 0x08
    je .fix_lt_2
    test        r2b, 0x04
    je .fix_tr_1
.do_top:
    PRED8x8_LOWPASS b, mm4, mm2, mm1, mm3, mm5
    movq    [t1+16], mm4
    test        r3b, 0x04
    je .done
    test        r2b, 0x04
    je .fix_tr_2
    movq        mm0, [src-1*FDEC_STRIDE+8]
    movq        mm5, mm0
    movq        mm2, mm0
    movq        mm4, mm0
    psrlq       mm5, 56
    PALIGNR     mm2, mm3, 7, mm3
    PALIGNR     mm5, mm4, 1, mm4
    PRED8x8_LOWPASS b, mm1, mm2, mm5, mm0, mm4
    jmp .do_topright
.fix_tr_2:
    punpckhbw   mm3, mm3
    pshufw      mm1, mm3, 0xFF
.do_topright:
    movq    [t1+24], mm1
    psrlq       mm1, 56
    movd         t4, mm1
    mov     [t1+32], t4b
.done:
    REP_RET
.fix_lt_1:
    movq        mm5, mm3
    pxor        mm5, mm4
    psrlq       mm5, 56
    psllq       mm5, 48
    pxor        mm1, mm5
    jmp .do_left
.fix_lt_2:
    movq        mm5, mm3
    pxor        mm5, mm2
    psllq       mm5, 56
    psrlq       mm5, 56
    pxor        mm2, mm5
    test        r2b, 0x04
    jne .do_top
.fix_tr_1:
    movq        mm5, mm3
    pxor        mm5, mm1
    psrlq       mm5, 56
    psllq       mm5, 56
    pxor        mm1, mm5
    jmp .do_top
%endmacro

%define PALIGNR PALIGNR_MMX
INIT_MMX
PREDICT_FILTER mmxext
%define PALIGNR PALIGNR_SSSE3
PREDICT_FILTER ssse3

;-----------------------------------------------------------------------------
; void predict_8x8_v( pixel *src, pixel *edge )
;-----------------------------------------------------------------------------
%macro PREDICT_8x8_V 1
cglobal predict_8x8_v_%1, 2,2
    mova        m0, [r1+16*SIZEOF_PIXEL]
    STORE8x8    m0, m0
    RET
%endmacro

%ifdef HIGH_BIT_DEPTH
INIT_XMM
PREDICT_8x8_V sse2
%else
INIT_MMX
PREDICT_8x8_V mmxext
%endif

;-----------------------------------------------------------------------------
; void predict_8x8_h( pixel *src, pixel edge[33] )
;-----------------------------------------------------------------------------
%macro PREDICT_8x8_H 3
cglobal predict_8x8_h_%1, 2,2
    movu      m1, [r1+7*SIZEOF_PIXEL]
    add       r0, 4*FDEC_STRIDEB
    mova      m2, m1
    punpckh%2 m1, m1
    punpckl%2 m2, m2
%assign n 0
%rep 8
%assign i 1+n/4
    SPLAT%3 m0, m %+ i, (3-n)&3
    mova [r0+(n-4)*FDEC_STRIDEB], m0
%assign n n+1
%endrep
    RET
%endmacro

%ifdef HIGH_BIT_DEPTH
INIT_XMM
PREDICT_8x8_H sse2  , wd, D
%else
INIT_MMX
PREDICT_8x8_H mmxext, bw, W
%endif

;-----------------------------------------------------------------------------
; void predict_8x8_dc( pixel *src, pixel *edge );
;-----------------------------------------------------------------------------
%ifdef HIGH_BIT_DEPTH
INIT_XMM
cglobal predict_8x8_dc_sse2, 2,2
    movu        m0, [r1+14]
    paddw       m0, [r1+32]
    HADDW       m0, m1
    paddw       m0, [pw_8]
    psrlw       m0, 4
    SPLATW      m0, m0
    STORE8x8    m0, m0
    REP_RET

%else
INIT_MMX
cglobal predict_8x8_dc_mmxext, 2,2
    pxor        mm0, mm0
    pxor        mm1, mm1
    psadbw      mm0, [r1+7]
    psadbw      mm1, [r1+16]
    paddw       mm0, [pw_8]
    paddw       mm0, mm1
    psrlw       mm0, 4
    pshufw      mm0, mm0, 0
    packuswb    mm0, mm0
    STORE8x8    mm0, mm0
    RET
%endif ; HIGH_BIT_DEPTH

;-----------------------------------------------------------------------------
; void predict_8x8_dc_top ( pixel *src, pixel *edge );
; void predict_8x8_dc_left( pixel *src, pixel *edge );
;-----------------------------------------------------------------------------
%ifdef HIGH_BIT_DEPTH
%macro PRED8x8_DC 3
cglobal %1, 2,2
    %3          m0, [r1+%2]
    HADDW       m0, m1
    paddw       m0, [pw_4]
    psrlw       m0, 3
    SPLATW      m0, m0
    STORE8x8    m0, m0
    RET
%endmacro
INIT_XMM
PRED8x8_DC predict_8x8_dc_top_sse2 , 32, mova
PRED8x8_DC predict_8x8_dc_left_sse2, 14, movu

%else
%macro PRED8x8_DC 2
cglobal %1, 2,2
    pxor        mm0, mm0
    psadbw      mm0, [r1+%2]
    paddw       mm0, [pw_4]
    psrlw       mm0, 3
    pshufw      mm0, mm0, 0
    packuswb    mm0, mm0
    STORE8x8    mm0, mm0
    RET
%endmacro
INIT_MMX
PRED8x8_DC predict_8x8_dc_top_mmxext, 16
PRED8x8_DC predict_8x8_dc_left_mmxext, 7
%endif ; HIGH_BIT_DEPTH

%ifndef ARCH_X86_64
; sse2 is faster even on amd, so there's no sense in spending exe size on these
; functions if we know sse2 is available.
INIT_MMX
;-----------------------------------------------------------------------------
; void predict_8x8_ddl( uint8_t *src, uint8_t *edge )
;-----------------------------------------------------------------------------
cglobal predict_8x8_ddl_mmxext, 2,2
    movq        mm5, [r1+16]
    movq        mm2, [r1+17]
    movq        mm3, [r1+23]
    movq        mm4, [r1+25]
    movq        mm1, mm5
    psllq       mm1, 8
    add          r0, FDEC_STRIDE*4
    PRED8x8_LOWPASS b, mm0, mm1, mm2, mm5, mm7
    PRED8x8_LOWPASS b, mm1, mm3, mm4, [r1+24], mm6
%assign Y 3
%rep 6
    movq        [r0+Y*FDEC_STRIDE], mm1
    movq        mm2, mm0
    psllq       mm1, 8
    psrlq       mm2, 56
    psllq       mm0, 8
    por         mm1, mm2
%assign Y (Y-1)
%endrep
    movq        [r0+Y*FDEC_STRIDE], mm1
    psllq       mm1, 8
    psrlq       mm0, 56
    por         mm1, mm0
%assign Y (Y-1)
    movq        [r0+Y*FDEC_STRIDE], mm1
    RET

;-----------------------------------------------------------------------------
; void predict_8x8_ddr( uint8_t *src, uint8_t *edge )
;-----------------------------------------------------------------------------
cglobal predict_8x8_ddr_mmxext, 2,2
    movq        mm1, [r1+7]
    movq        mm2, [r1+9]
    movq        mm3, [r1+15]
    movq        mm4, [r1+17]
    add          r0, FDEC_STRIDE*4
    PRED8x8_LOWPASS b, mm0, mm1, mm2, [r1+8], mm7
    PRED8x8_LOWPASS b, mm1, mm3, mm4, [r1+16], mm6
%assign Y 3
%rep 6
    movq        [r0+Y*FDEC_STRIDE], mm0
    movq        mm2, mm1
    psrlq       mm0, 8
    psllq       mm2, 56
    psrlq       mm1, 8
    por         mm0, mm2
%assign Y (Y-1)
%endrep
    movq        [r0+Y*FDEC_STRIDE], mm0
    psrlq       mm0, 8
    psllq       mm1, 56
    por         mm0, mm1
%assign Y (Y-1)
    movq        [r0+Y*FDEC_STRIDE], mm0
    RET

;-----------------------------------------------------------------------------
; void predict_8x8_hu( uint8_t *src, uint8_t *edge )
;-----------------------------------------------------------------------------
%define PALIGNR PALIGNR_MMX
cglobal predict_8x8_hu_mmxext, 2,2
    movq    mm1, [r1+7]         ; l0 l1 l2 l3 l4 l5 l6 l7
    add      r0, 4*FDEC_STRIDE
    pshufw  mm0, mm1, 00011011b ; l6 l7 l4 l5 l2 l3 l0 l1
    psllq   mm1, 56             ; l7 .. .. .. .. .. .. ..
    movq    mm2, mm0
    psllw   mm0, 8
    psrlw   mm2, 8
    por     mm2, mm0            ; l7 l6 l5 l4 l3 l2 l1 l0
    movq    mm3, mm2
    movq    mm4, mm2
    movq    mm5, mm2
    psrlq   mm2, 8
    psrlq   mm3, 16
    por     mm2, mm1            ; l7 l7 l6 l5 l4 l3 l2 l1
    punpckhbw mm1, mm1
    por     mm3, mm1            ; l7 l7 l7 l6 l5 l4 l3 l2
    pavgb   mm4, mm2
    PRED8x8_LOWPASS b, mm1, mm3, mm5, mm2, mm6
    movq    mm5, mm4
    punpcklbw mm4, mm1          ; p4 p3 p2 p1
    punpckhbw mm5, mm1          ; p8 p7 p6 p5
    movq    mm6, mm5
    movq    mm7, mm5
    movq    mm0, mm5
    PALIGNR mm5, mm4, 2, mm1
    pshufw  mm1, mm6, 11111001b
    PALIGNR mm6, mm4, 4, mm2
    pshufw  mm2, mm7, 11111110b
    PALIGNR mm7, mm4, 6, mm3
    pshufw  mm3, mm0, 11111111b
    movq   [r0-4*FDEC_STRIDE], mm4
    movq   [r0-3*FDEC_STRIDE], mm5
    movq   [r0-2*FDEC_STRIDE], mm6
    movq   [r0-1*FDEC_STRIDE], mm7
    movq   [r0+0*FDEC_STRIDE], mm0
    movq   [r0+1*FDEC_STRIDE], mm1
    movq   [r0+2*FDEC_STRIDE], mm2
    movq   [r0+3*FDEC_STRIDE], mm3
    RET

;-----------------------------------------------------------------------------
; void predict_8x8_vr_core( uint8_t *src, uint8_t *edge )
;-----------------------------------------------------------------------------

; fills only some pixels:
; f01234567
; 0........
; 1,,,,,,,,
; 2 .......
; 3 ,,,,,,,
; 4  ......
; 5  ,,,,,,
; 6   .....
; 7   ,,,,,

cglobal predict_8x8_vr_core_mmxext, 2,2
    movq        mm2, [r1+16]
    movq        mm3, [r1+15]
    movq        mm1, [r1+14]
    movq        mm4, mm3
    pavgb       mm3, mm2
    add          r0, FDEC_STRIDE*4
    PRED8x8_LOWPASS b, mm0, mm1, mm2, mm4, mm7

%assign Y -4
%rep 3
    movq        [r0+ Y   *FDEC_STRIDE], mm3
    movq        [r0+(Y+1)*FDEC_STRIDE], mm0
    psllq       mm3, 8
    psllq       mm0, 8
%assign Y (Y+2)
%endrep
    movq        [r0+ Y   *FDEC_STRIDE], mm3
    movq        [r0+(Y+1)*FDEC_STRIDE], mm0

    RET

;-----------------------------------------------------------------------------
; void predict_8x8c_p_core( uint8_t *src, int i00, int b, int c )
;-----------------------------------------------------------------------------
cglobal predict_8x8c_p_core_mmxext, 1,2
    LOAD_PLANE_ARGS
    movq        mm1, mm2
    pmullw      mm2, [pw_3210]
    psllw       mm1, 2
    paddsw      mm0, mm2        ; mm0 = {i+0*b, i+1*b, i+2*b, i+3*b}
    paddsw      mm1, mm0        ; mm1 = {i+4*b, i+5*b, i+6*b, i+7*b}

    mov         r1d, 8
ALIGN 4
.loop:
    movq        mm5, mm0
    movq        mm6, mm1
    psraw       mm5, 5
    psraw       mm6, 5
    packuswb    mm5, mm6
    movq        [r0], mm5

    paddsw      mm0, mm4
    paddsw      mm1, mm4
    add         r0, FDEC_STRIDE
    dec         r1d
    jg          .loop
    REP_RET

;-----------------------------------------------------------------------------
; void predict_16x16_p_core( uint8_t *src, int i00, int b, int c )
;-----------------------------------------------------------------------------
cglobal predict_16x16_p_core_mmxext, 1,2
    LOAD_PLANE_ARGS
    movq        mm5, mm2
    movq        mm1, mm2
    pmullw      mm5, [pw_3210]
    psllw       mm2, 3
    psllw       mm1, 2
    movq        mm3, mm2
    paddsw      mm0, mm5        ; mm0 = {i+ 0*b, i+ 1*b, i+ 2*b, i+ 3*b}
    paddsw      mm1, mm0        ; mm1 = {i+ 4*b, i+ 5*b, i+ 6*b, i+ 7*b}
    paddsw      mm2, mm0        ; mm2 = {i+ 8*b, i+ 9*b, i+10*b, i+11*b}
    paddsw      mm3, mm1        ; mm3 = {i+12*b, i+13*b, i+14*b, i+15*b}

    mov         r1d, 16
ALIGN 4
.loop:
    movq        mm5, mm0
    movq        mm6, mm1
    psraw       mm5, 5
    psraw       mm6, 5
    packuswb    mm5, mm6
    movq        [r0], mm5

    movq        mm5, mm2
    movq        mm6, mm3
    psraw       mm5, 5
    psraw       mm6, 5
    packuswb    mm5, mm6
    movq        [r0+8], mm5

    paddsw      mm0, mm4
    paddsw      mm1, mm4
    paddsw      mm2, mm4
    paddsw      mm3, mm4
    add         r0, FDEC_STRIDE
    dec         r1d
    jg          .loop
    REP_RET

%endif ; !ARCH_X86_64

INIT_XMM
;-----------------------------------------------------------------------------
; void predict_8x8_ddl( uint8_t *src, uint8_t *edge )
;-----------------------------------------------------------------------------
cglobal predict_8x8_ddl_sse2, 2,2
    movdqa      xmm3, [r1+16]
    movdqu      xmm2, [r1+17]
    movdqa      xmm1, xmm3
    pslldq      xmm1, 1
    add          r0, FDEC_STRIDE*4
    PRED8x8_LOWPASS b, xmm0, xmm1, xmm2, xmm3, xmm4

%assign Y -4
%rep 8
    psrldq      xmm0, 1
    movq        [r0+Y*FDEC_STRIDE], xmm0
%assign Y (Y+1)
%endrep
    RET

;-----------------------------------------------------------------------------
; void predict_8x8_ddr( uint8_t *src, uint8_t *edge )
;-----------------------------------------------------------------------------
cglobal predict_8x8_ddr_sse2, 2,2
    movdqu      xmm3, [r1+8]
    movdqu      xmm1, [r1+7]
    movdqa      xmm2, xmm3
    psrldq      xmm2, 1
    add           r0, FDEC_STRIDE*4
    PRED8x8_LOWPASS b, xmm0, xmm1, xmm2, xmm3, xmm4

    movdqa      xmm1, xmm0
    psrldq      xmm1, 1
%assign Y 3
%rep 3
    movq        [r0+Y*FDEC_STRIDE], xmm0
    movq        [r0+(Y-1)*FDEC_STRIDE], xmm1
    psrldq      xmm0, 2
    psrldq      xmm1, 2
%assign Y (Y-2)
%endrep
    movq        [r0-3*FDEC_STRIDE], xmm0
    movq        [r0-4*FDEC_STRIDE], xmm1

    RET

;-----------------------------------------------------------------------------
; void predict_8x8_vl( uint8_t *src, uint8_t *edge )
;-----------------------------------------------------------------------------
cglobal predict_8x8_vl_sse2, 2,2
    movdqa      xmm4, [r1+16]
    movdqa      xmm2, xmm4
    movdqa      xmm1, xmm4
    movdqa      xmm3, xmm4
    psrldq      xmm2, 1
    pslldq      xmm1, 1
    pavgb       xmm3, xmm2
    add           r0, FDEC_STRIDE*4
    PRED8x8_LOWPASS b, xmm0, xmm1, xmm2, xmm4, xmm5
; xmm0: (t0 + 2*t1 + t2 + 2) >> 2
; xmm3: (t0 + t1 + 1) >> 1

%assign Y -4
%rep 3
    psrldq      xmm0, 1
    movq        [r0+ Y   *FDEC_STRIDE], xmm3
    movq        [r0+(Y+1)*FDEC_STRIDE], xmm0
    psrldq      xmm3, 1
%assign Y (Y+2)
%endrep
    psrldq      xmm0, 1
    movq        [r0+ Y   *FDEC_STRIDE], xmm3
    movq        [r0+(Y+1)*FDEC_STRIDE], xmm0

    RET

;-----------------------------------------------------------------------------
; void predict_8x8_vr( uint8_t *src, uint8_t *edge )
;-----------------------------------------------------------------------------
cglobal predict_8x8_vr_sse2, 2,2,7
    movdqu      xmm0, [r1+8]
    movdqa      xmm6, [pw_ff00]
    add         r0, 4*FDEC_STRIDE
    movdqa      xmm1, xmm0
    movdqa      xmm2, xmm0
    movdqa      xmm3, xmm0
    pslldq      xmm0, 1
    pslldq      xmm1, 2
    pavgb       xmm2, xmm0
    PRED8x8_LOWPASS b, xmm4, xmm3, xmm1, xmm0, xmm5
    pandn       xmm6, xmm4
    movdqa      xmm5, xmm4
    psrlw       xmm4, 8
    packuswb    xmm6, xmm4
    movhlps     xmm4, xmm6
    movhps [r0-3*FDEC_STRIDE], xmm5
    movhps [r0-4*FDEC_STRIDE], xmm2
    psrldq      xmm5, 4
    movss       xmm5, xmm6
    psrldq      xmm2, 4
    movss       xmm2, xmm4
%assign Y 3
%rep 3
    psrldq      xmm5, 1
    psrldq      xmm2, 1
    movq        [r0+Y*FDEC_STRIDE], xmm5
    movq        [r0+(Y-1)*FDEC_STRIDE], xmm2
%assign Y (Y-2)
%endrep
    RET

INIT_MMX
;-----------------------------------------------------------------------------
; void predict_8x8_hd( uint8_t *src, uint8_t *edge )
;-----------------------------------------------------------------------------
%define PALIGNR PALIGNR_MMX
cglobal predict_8x8_hd_mmxext, 2,2
    add     r0, 4*FDEC_STRIDE
    movq    mm0, [r1]           ; l7 .. .. .. .. .. .. ..
    movq    mm1, [r1+8]         ; lt l0 l1 l2 l3 l4 l5 l6
    movq    mm2, [r1+16]        ; t7 t6 t5 t4 t3 t2 t1 t0
    movq    mm3, mm1            ; lt l0 l1 l2 l3 l4 l5 l6
    movq    mm4, mm2            ; t7 t6 t5 t4 t3 t2 t1 t0
    PALIGNR mm2, mm1, 7, mm5    ; t6 t5 t4 t3 t2 t1 t0 lt
    PALIGNR mm1, mm0, 7, mm6    ; l0 l1 l2 l3 l4 l5 l6 l7
    PALIGNR mm4, mm3, 1, mm7    ; t0 lt l0 l1 l2 l3 l4 l5
    movq    mm5, mm3
    pavgb   mm3, mm1
    PRED8x8_LOWPASS b, mm0, mm4, mm1, mm5, mm7
    movq    mm4, mm2
    movq    mm1, mm2            ; t6 t5 t4 t3 t2 t1 t0 lt
    psrlq   mm4, 16             ; .. .. t6 t5 t4 t3 t2 t1
    psrlq   mm1, 8              ; .. t6 t5 t4 t3 t2 t1 t0
    PRED8x8_LOWPASS b, mm6, mm4, mm2, mm1, mm5
                                ; .. p11 p10 p9
    movq    mm7, mm3
    punpcklbw mm3, mm0          ; p4 p3 p2 p1
    punpckhbw mm7, mm0          ; p8 p7 p6 p5
    movq    mm1, mm7
    movq    mm0, mm7
    movq    mm4, mm7
    movq   [r0+3*FDEC_STRIDE], mm3
    PALIGNR mm7, mm3, 2, mm5
    movq   [r0+2*FDEC_STRIDE], mm7
    PALIGNR mm1, mm3, 4, mm5
    movq   [r0+1*FDEC_STRIDE], mm1
    PALIGNR mm0, mm3, 6, mm3
    movq    [r0+0*FDEC_STRIDE], mm0
    movq    mm2, mm6
    movq    mm3, mm6
    movq   [r0-1*FDEC_STRIDE], mm4
    PALIGNR mm6, mm4, 2, mm5
    movq   [r0-2*FDEC_STRIDE], mm6
    PALIGNR mm2, mm4, 4, mm5
    movq   [r0-3*FDEC_STRIDE], mm2
    PALIGNR mm3, mm4, 6, mm4
    movq   [r0-4*FDEC_STRIDE], mm3
    RET

;-----------------------------------------------------------------------------
; void predict_8x8_hd( uint8_t *src, uint8_t *edge )
;-----------------------------------------------------------------------------
%macro PREDICT_8x8_HD 1
cglobal predict_8x8_hd_%1, 2,2
    add       r0, 4*FDEC_STRIDE
    movdqa  xmm0, [r1]
    movdqa  xmm1, [r1+16]
    movdqa  xmm2, xmm1
    movdqa  xmm3, xmm1
    PALIGNR xmm1, xmm0, 7, xmm4
    PALIGNR xmm2, xmm0, 9, xmm5
    PALIGNR xmm3, xmm0, 8, xmm0
    movdqa  xmm4, xmm1
    pavgb   xmm4, xmm3
    PRED8x8_LOWPASS b, xmm0, xmm1, xmm2, xmm3, xmm5
    punpcklbw xmm4, xmm0
    movhlps xmm0, xmm4

%assign Y 3
%rep 3
    movq   [r0+(Y)*FDEC_STRIDE], xmm4
    movq   [r0+(Y-4)*FDEC_STRIDE], xmm0
    psrldq xmm4, 2
    psrldq xmm0, 2
%assign Y (Y-1)
%endrep
    movq   [r0+(Y)*FDEC_STRIDE], xmm4
    movq   [r0+(Y-4)*FDEC_STRIDE], xmm0
    RET
%endmacro

INIT_XMM
PREDICT_8x8_HD sse2
%define PALIGNR PALIGNR_SSSE3
PREDICT_8x8_HD ssse3
INIT_MMX
%define PALIGNR PALIGNR_MMX

;-----------------------------------------------------------------------------
; void predict_8x8_hu( uint8_t *src, uint8_t *edge )
;-----------------------------------------------------------------------------
%macro PREDICT_8x8_HU 1
cglobal predict_8x8_hu_%1, 2,2
    add        r0, 4*FDEC_STRIDE
%ifidn %1, ssse3
    movq      mm5, [r1+7]
    movq      mm6, [pb_reverse]
    movq      mm1, mm5
    movq      mm2, mm5
    movq      mm3, mm5
    pshufb    mm5, mm6
    psrlq     mm6, 8
    pshufb    mm2, mm6
    psrlq     mm6, 8
    pshufb    mm3, mm6
    movq      mm4, mm5
%else
    movq      mm1, [r1+7]           ; l0 l1 l2 l3 l4 l5 l6 l7
    pshufw    mm0, mm1, 00011011b   ; l6 l7 l4 l5 l2 l3 l0 l1
    movq      mm2, mm0
    psllw     mm0, 8
    psrlw     mm2, 8
    por       mm2, mm0              ; l7 l6 l5 l4 l3 l2 l1 l0
    psllq     mm1, 56               ; l7 .. .. .. .. .. .. ..
    movq      mm3, mm2
    movq      mm4, mm2
    movq      mm5, mm2
    psrlq     mm2, 8
    psrlq     mm3, 16
    por       mm2, mm1              ; l7 l7 l6 l5 l4 l3 l2 l1
    punpckhbw mm1, mm1
    por       mm3, mm1              ; l7 l7 l7 l6 l5 l4 l3 l2
%endif
    pavgb     mm4, mm2
    PRED8x8_LOWPASS b, mm1, mm3, mm5, mm2, mm6

    movq2dq   xmm0, mm4
    movq2dq   xmm1, mm1
    punpcklbw xmm0, xmm1
    punpckhbw  mm4, mm1
%assign Y -4
%rep 3
    movq     [r0+Y*FDEC_STRIDE], xmm0
    psrldq    xmm0, 2
%assign Y (Y+1)
%endrep
    pshufw     mm5, mm4, 11111001b
    pshufw     mm6, mm4, 11111110b
    pshufw     mm7, mm4, 11111111b
    movq     [r0+Y*FDEC_STRIDE], xmm0
    movq     [r0+0*FDEC_STRIDE], mm4
    movq     [r0+1*FDEC_STRIDE], mm5
    movq     [r0+2*FDEC_STRIDE], mm6
    movq     [r0+3*FDEC_STRIDE], mm7
    RET
%endmacro

PREDICT_8x8_HU sse2
PREDICT_8x8_HU ssse3

;-----------------------------------------------------------------------------
; void predict_8x8c_v( uint8_t *src )
;-----------------------------------------------------------------------------
cglobal predict_8x8c_v_mmx, 1,1
    movq        mm0, [r0 - FDEC_STRIDE]
    STORE8x8    mm0, mm0
    RET

;-----------------------------------------------------------------------------
; void predict_8x8c_h( uint8_t *src )
;-----------------------------------------------------------------------------

%macro PRED_8x8C_H 1
cglobal predict_8x8c_h_%1, 1,1
%ifidn %1, ssse3
    mova   m1, [pb_3]
%endif
    add    r0, FDEC_STRIDE*4
%assign n -4
%rep 8
    SPLATB m0, r0+FDEC_STRIDE*n-1, m1
    mova [r0+FDEC_STRIDE*n], m0
%assign n n+1
%endrep
    RET
%endmacro

INIT_MMX
%define SPLATB SPLATB_MMX
PRED_8x8C_H mmxext
%define SPLATB SPLATB_SSSE3
PRED_8x8C_H ssse3

;-----------------------------------------------------------------------------
; void predict_8x8c_dc_core( uint8_t *src, int s2, int s3 )
;-----------------------------------------------------------------------------
cglobal predict_8x8c_dc_core_mmxext, 1,1
    movq        mm0, [r0 - FDEC_STRIDE]
    pxor        mm1, mm1
    pxor        mm2, mm2
    punpckhbw   mm1, mm0
    punpcklbw   mm0, mm2
    psadbw      mm1, mm2        ; s1
    psadbw      mm0, mm2        ; s0

%ifdef ARCH_X86_64
    movd        mm4, r1d
    movd        mm5, r2d
    paddw       mm0, mm4
    pshufw      mm2, mm5, 0
%else
    paddw       mm0, r1m
    pshufw      mm2, r2m, 0
%endif
    psrlw       mm0, 3
    paddw       mm1, [pw_2]
    movq        mm3, mm2
    pshufw      mm1, mm1, 0
    pshufw      mm0, mm0, 0     ; dc0 (w)
    paddw       mm3, mm1
    psrlw       mm3, 3          ; dc3 (w)
    psrlw       mm2, 2          ; dc2 (w)
    psrlw       mm1, 2          ; dc1 (w)

    packuswb    mm0, mm1        ; dc0,dc1 (b)
    packuswb    mm2, mm3        ; dc2,dc3 (b)

    STORE8x8    mm0, mm2
    RET

cglobal predict_8x8c_dc_top_mmxext, 1,1
    movq        mm0, [r0 - FDEC_STRIDE]
    pxor        mm1, mm1
    pxor        mm2, mm2
    punpckhbw   mm1, mm0
    punpcklbw   mm0, mm2
    psadbw      mm1, mm2        ; s1
    psadbw      mm0, mm2        ; s0
    psrlw       mm1, 1
    psrlw       mm0, 1
    pavgw       mm1, mm2
    pavgw       mm0, mm2
    pshufw      mm1, mm1, 0
    pshufw      mm0, mm0, 0     ; dc0 (w)
    packuswb    mm0, mm1        ; dc0,dc1 (b)
    STORE8x8    mm0, mm0
    RET

;-----------------------------------------------------------------------------
; void predict_8x8c_p_core( uint8_t *src, int i00, int b, int c )
;-----------------------------------------------------------------------------

cglobal predict_8x8c_p_core_sse2, 1,1
    movd        xmm0, r1m
    movd        xmm2, r2m
    movd        xmm4, r3m
    pshuflw     xmm0, xmm0, 0
    pshuflw     xmm2, xmm2, 0
    pshuflw     xmm4, xmm4, 0
    punpcklqdq  xmm0, xmm0
    punpcklqdq  xmm2, xmm2
    punpcklqdq  xmm4, xmm4
    pmullw      xmm2, [pw_76543210]
    paddsw      xmm0, xmm2        ; xmm0 = {i+0*b, i+1*b, i+2*b, i+3*b, i+4*b, i+5*b, i+6*b, i+7*b}
    movdqa      xmm3, xmm0
    paddsw      xmm3, xmm4
    paddsw      xmm4, xmm4
call .loop
    add           r0, FDEC_STRIDE*4
.loop:
    movdqa      xmm5, xmm0
    movdqa      xmm1, xmm3
    psraw       xmm0, 5
    psraw       xmm3, 5
    packuswb    xmm0, xmm3
    movq        [r0+FDEC_STRIDE*0], xmm0
    movhps      [r0+FDEC_STRIDE*1], xmm0
    paddsw      xmm5, xmm4
    paddsw      xmm1, xmm4
    movdqa      xmm0, xmm5
    movdqa      xmm3, xmm1
    psraw       xmm5, 5
    psraw       xmm1, 5
    packuswb    xmm5, xmm1
    movq        [r0+FDEC_STRIDE*2], xmm5
    movhps      [r0+FDEC_STRIDE*3], xmm5
    paddsw      xmm0, xmm4
    paddsw      xmm3, xmm4
    RET

;-----------------------------------------------------------------------------
; void predict_16x16_p_core( uint8_t *src, int i00, int b, int c )
;-----------------------------------------------------------------------------
cglobal predict_16x16_p_core_sse2, 1,2,8
    movd        xmm0, r1m
    movd        xmm1, r2m
    movd        xmm2, r3m
    pshuflw     xmm0, xmm0, 0
    pshuflw     xmm1, xmm1, 0
    pshuflw     xmm2, xmm2, 0
    punpcklqdq  xmm0, xmm0
    punpcklqdq  xmm1, xmm1
    punpcklqdq  xmm2, xmm2
    movdqa      xmm3, xmm1
    pmullw      xmm3, [pw_76543210]
    psllw       xmm1, 3
    paddsw      xmm0, xmm3  ; xmm0 = {i+ 0*b, i+ 1*b, i+ 2*b, i+ 3*b, i+ 4*b, i+ 5*b, i+ 6*b, i+ 7*b}
    paddsw      xmm1, xmm0  ; xmm1 = {i+ 8*b, i+ 9*b, i+10*b, i+11*b, i+12*b, i+13*b, i+14*b, i+15*b}
    movdqa      xmm7, xmm2
    paddsw      xmm7, xmm7
    mov         r1d, 8
ALIGN 4
.loop:
    movdqa      xmm3, xmm0
    movdqa      xmm4, xmm1
    movdqa      xmm5, xmm0
    movdqa      xmm6, xmm1
    psraw       xmm3, 5
    psraw       xmm4, 5
    paddsw      xmm5, xmm2
    paddsw      xmm6, xmm2
    psraw       xmm5, 5
    psraw       xmm6, 5
    packuswb    xmm3, xmm4
    packuswb    xmm5, xmm6
    movdqa      [r0+FDEC_STRIDE*0], xmm3
    movdqa      [r0+FDEC_STRIDE*1], xmm5
    paddsw      xmm0, xmm7
    paddsw      xmm1, xmm7
    add         r0, FDEC_STRIDE*2
    dec         r1d
    jg          .loop
    REP_RET

;-----------------------------------------------------------------------------
; void predict_16x16_v( pixel *src )
;-----------------------------------------------------------------------------
%ifdef HIGH_BIT_DEPTH
INIT_MMX
cglobal predict_16x16_v_mmx, 1,2
    mova        m0, [r0 - FDEC_STRIDEB+ 0]
    mova        m1, [r0 - FDEC_STRIDEB+ 8]
    mova        m2, [r0 - FDEC_STRIDEB+16]
    mova        m3, [r0 - FDEC_STRIDEB+24]
    STORE16x16  m0, m1, m2, m3
    REP_RET
INIT_XMM
cglobal predict_16x16_v_sse2, 2,2
    mova      m0, [r0 - FDEC_STRIDEB+ 0]
    mova      m1, [r0 - FDEC_STRIDEB+16]
    STORE16x16_SSE2 m0, m1
    REP_RET
%else
INIT_MMX
cglobal predict_16x16_v_mmx, 1,2
    movq        m0, [r0 - FDEC_STRIDE + 0]
    movq        m1, [r0 - FDEC_STRIDE + 8]
    STORE16x16  m0, m1
    REP_RET
INIT_XMM
cglobal predict_16x16_v_sse2, 1,1
    movdqa      xmm0, [r0 - FDEC_STRIDE]
    STORE16x16_SSE2 xmm0
    RET
%endif

;-----------------------------------------------------------------------------
; void predict_16x16_h( pixel *src )
;-----------------------------------------------------------------------------
%macro PRED_16x16_H 1
cglobal predict_16x16_h_%1, 1,2
    mov r1, 12*FDEC_STRIDEB
%ifdef HIGH_BIT_DEPTH
.vloop:
%assign n 0
%rep 4
    movd        m0, [r0+r1+n*FDEC_STRIDEB-2*SIZEOF_PIXEL]
    SPLATW      m0, m0, 1
    mova [r0+r1+n*FDEC_STRIDEB+ 0], m0
    mova [r0+r1+n*FDEC_STRIDEB+16], m0
%if mmsize==8
    mova [r0+r1+n*FDEC_STRIDEB+ 8], m0
    mova [r0+r1+n*FDEC_STRIDEB+24], m0
%endif
%assign n n+1
%endrep

%else
%ifidn %1, ssse3
    mova   m1, [pb_3]
%endif
.vloop:
%assign n 0
%rep 4
    SPLATB m0, r0+r1+FDEC_STRIDE*n-1, m1
    mova [r0+r1+FDEC_STRIDE*n], m0
%if mmsize==8
    mova [r0+r1+FDEC_STRIDE*n+8], m0
%endif
%assign n n+1
%endrep
%endif ; HIGH_BIT_DEPTH
    sub r1, 4*FDEC_STRIDEB
    jge .vloop
    REP_RET
%endmacro

INIT_MMX
%define SPLATB SPLATB_MMX
PRED_16x16_H mmxext
INIT_XMM
%ifdef HIGH_BIT_DEPTH
PRED_16x16_H sse2
%else
;no SSE2 for 8-bit, it's slower than MMX on all systems that don't support SSSE3
%define SPLATB SPLATB_SSSE3
PRED_16x16_H ssse3
%endif

;-----------------------------------------------------------------------------
; void predict_16x16_dc_core( pixel *src, int i_dc_left )
;-----------------------------------------------------------------------------

%macro PRED16x16_DC 2
%ifdef HIGH_BIT_DEPTH
    mova       m0, [r0 - FDEC_STRIDEB+ 0]
    paddw      m0, [r0 - FDEC_STRIDEB+ 8]
    paddw      m0, [r0 - FDEC_STRIDEB+16]
    paddw      m0, [r0 - FDEC_STRIDEB+24]
    HADDW      m0, m1
    paddw      m0, %1
    psrlw      m0, %2
    SPLATW     m0, m0
    STORE16x16 m0, m0, m0, m0
%else
    pxor        m0, m0
    pxor        m1, m1
    psadbw      m0, [r0 - FDEC_STRIDE]
    psadbw      m1, [r0 - FDEC_STRIDE + 8]
    paddusw     m0, m1
    paddusw     m0, %1
    psrlw       m0, %2                      ; dc
    pshufw      m0, m0, 0
    packuswb    m0, m0                      ; dc in bytes
    STORE16x16  m0, m0
%endif
%endmacro

INIT_MMX
cglobal predict_16x16_dc_core_mmxext, 1,2
%ifdef ARCH_X86_64
    movd         m6, r1d
    PRED16x16_DC m6, 5
%else
    PRED16x16_DC r1m, 5
%endif
    REP_RET

INIT_MMX
cglobal predict_16x16_dc_top_mmxext, 1,2
    PRED16x16_DC [pw_8], 4
    REP_RET

INIT_MMX
%ifdef HIGH_BIT_DEPTH
cglobal predict_16x16_dc_left_core_mmxext, 1,2
    movd       m0, r1m
    SPLATW     m0, m0
    STORE16x16 m0, m0, m0, m0
    REP_RET
%else
cglobal predict_16x16_dc_left_core_mmxext, 1,1
    movd       m0, r1m
    pshufw     m0, m0, 0
    packuswb   m0, m0
    STORE16x16 m0, m0
    REP_RET
%endif

;-----------------------------------------------------------------------------
; void predict_16x16_dc_core( pixel *src, int i_dc_left )
;-----------------------------------------------------------------------------

%macro PRED16x16_DC_SSE2 2
%ifdef HIGH_BIT_DEPTH
    mova       m0, [r0 - FDEC_STRIDEB+ 0]
    paddw      m0, [r0 - FDEC_STRIDEB+16]
    HADDW      m0, m2
    paddw      m0, %1
    psrlw      m0, %2
    SPLATW     m0, m0
    STORE16x16_SSE2 m0, m0
%else
    pxor        m0, m0
    psadbw      m0, [r0 - FDEC_STRIDE]
    movhlps     m1, m0
    paddw       m0, m1
    paddusw     m0, %1
    psrlw       m0, %2              ; dc
    SPLATW      m0, m0
    packuswb    m0, m0              ; dc in bytes
    STORE16x16_SSE2 m0
%endif
%endmacro

INIT_XMM
cglobal predict_16x16_dc_core_sse2, 2,2,4
    movd       m3, r1m
    PRED16x16_DC_SSE2 m3, 5
    REP_RET

cglobal predict_16x16_dc_top_sse2, 1,2
    PRED16x16_DC_SSE2 [pw_8], 4
    REP_RET

INIT_XMM
%ifdef HIGH_BIT_DEPTH
cglobal predict_16x16_dc_left_core_sse2, 1,2
    movd       m0, r1m
    SPLATW     m0, m0
    STORE16x16_SSE2 m0, m0
    REP_RET
%else
cglobal predict_16x16_dc_left_core_sse2, 1,1
    movd       m0, r1m
    SPLATW     m0, m0
    packuswb   m0, m0
    STORE16x16_SSE2 m0
    RET
%endif

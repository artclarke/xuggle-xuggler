;*****************************************************************************
;* predict-a.asm: x86 intra prediction
;*****************************************************************************
;* Copyright (C) 2005-2011 x264 project
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
pw_3210:     dw 0, 1, 2, 3, 4, 5, 6, 7
pw_43210123: dw -3, -2, -1, 0, 1, 2, 3, 4
pw_m3:       times 8 dw -3
pb_00s_ff:   times 8 db 0
pb_0s_ff:    times 7 db 0
             db 0xff

SECTION .text

cextern pb_0
cextern pb_1
cextern pb_3
cextern pw_1
cextern pw_2
cextern pw_4
cextern pw_8
cextern pw_16
cextern pw_ff00
cextern pb_reverse
cextern pw_pixel_max

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
    paddw       %3, %4
    psrlw       %3, 1
    pavgw       %2, %5, %3
%else
    mova        %6, %3
    pavgb       %3, %4
    pxor        %4, %6
    pand        %4, [pb_1]
    psubusb     %3, %4
    pavgb       %2, %5, %3
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
%macro PREDICT_4x4_DDL 3
cglobal predict_4x4_ddl, 1,1
    movu    m1, [r0-FDEC_STRIDEB]
    psll%1  m2, m1, %2
    mova    m3, m1
    mova    m4, m1
    pxor    m1, m2
    psrl%1  m1, %2
    pxor    m3, m1
    PRED8x8_LOWPASS %3, m0, m2, m3, m4, m5

%assign Y 0
%rep 4
    psrl%1      m0, %2
    movh        [r0+Y*FDEC_STRIDEB], m0
%assign Y (Y+1)
%endrep

    RET
%endmacro

%ifdef HIGH_BIT_DEPTH
INIT_XMM sse2
PREDICT_4x4_DDL dq, 2, w
INIT_XMM avx
PREDICT_4x4_DDL dq, 2, w
INIT_MMX mmx2
cglobal predict_4x4_ddl, 1,2
    mova    m1, [r0-2*FDEC_STRIDE+4]
    mova    m2, [r0-2*FDEC_STRIDE+0]
    mova    m3, [r0-2*FDEC_STRIDE+2]
    PRED8x8_LOWPASS w, m0, m1, m2, m3
    mova    [r0+0*FDEC_STRIDE], m0

    mova    m5, [r0-2*FDEC_STRIDE+6]
    mova    m6, [r0-2*FDEC_STRIDE+8]
    pshufw  m7, m6, q3321
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
INIT_MMX mmx2
PREDICT_4x4_DDL q, 8, b
%endif

;-----------------------------------------------------------------------------
; void predict_4x4_ddr( pixel *src )
;-----------------------------------------------------------------------------
%macro PREDICT_4x4 6
cglobal predict_4x4_ddr, 1,1
    movu      m1, [r0+1*FDEC_STRIDEB-8*SIZEOF_PIXEL]
    movq      m2, [r0+0*FDEC_STRIDEB-8]
%ifdef HIGH_BIT_DEPTH
    movh      m4, [r0-1*FDEC_STRIDEB-4*SIZEOF_PIXEL]
    punpckl%1 m2, m4
    movh      m3, [r0-1*FDEC_STRIDEB]
    punpckh%2 m1, m2
    PALIGNR   m3, m1, 5*SIZEOF_PIXEL, m1
    mova      m1, m3
    movhps    m4, [r0+2*FDEC_STRIDEB-4*SIZEOF_PIXEL]
    PALIGNR   m3, m4, 7*SIZEOF_PIXEL, m4
    mova      m2, m3
    movhps    m4, [r0+3*FDEC_STRIDEB-4*SIZEOF_PIXEL]
    PALIGNR   m3, m4, 7*SIZEOF_PIXEL, m4
%else
    punpckh%1 m2, [r0-1*FDEC_STRIDEB-8*SIZEOF_PIXEL]
    movh      m3, [r0-1*FDEC_STRIDEB]
    punpckh%2 m1, m2
    PALIGNR   m3, m1, 5*SIZEOF_PIXEL, m1
    mova      m1, m3
    PALIGNR   m3, [r0+2*FDEC_STRIDEB-8*SIZEOF_PIXEL], 7*SIZEOF_PIXEL, m4
    mova      m2, m3
    PALIGNR   m3, [r0+3*FDEC_STRIDEB-8*SIZEOF_PIXEL], 7*SIZEOF_PIXEL, m4
%endif
    PRED8x8_LOWPASS %4, m0, m3, m1, m2, m4
%assign Y 3
    movh      [r0+Y*FDEC_STRIDEB], m0
%rep 3
%assign Y (Y-1)
    psrl%3    m0, %6
    movh      [r0+Y*FDEC_STRIDEB], m0
%endrep
    RET

cglobal predict_4x4_vr, 1,1,6
    movh    m0, [r0-1*FDEC_STRIDEB]                                       ; ........t3t2t1t0
    mova    m5, m0
%ifdef HIGH_BIT_DEPTH
    movhps  m1, [r0-1*FDEC_STRIDEB-4*SIZEOF_PIXEL]
    PALIGNR m0, m1, 7*SIZEOF_PIXEL, m1                                    ; ......t3t2t1t0lt
    pavg%4  m5, m0
    movhps  m1, [r0+0*FDEC_STRIDEB-4*SIZEOF_PIXEL]
    PALIGNR m0, m1, 7*SIZEOF_PIXEL, m1                                    ; ....t3t2t1t0ltl0
    mova    m1, m0
    movhps  m2, [r0+1*FDEC_STRIDEB-4*SIZEOF_PIXEL]
    PALIGNR m0, m2, 7*SIZEOF_PIXEL, m2                                    ; ..t3t2t1t0ltl0l1
    mova    m2, m0
    movhps  m3, [r0+2*FDEC_STRIDEB-4*SIZEOF_PIXEL]
    PALIGNR m0, m3, 7*SIZEOF_PIXEL, m3                                    ; t3t2t1t0ltl0l1l2
%else
    PALIGNR m0, [r0-1*FDEC_STRIDEB-8*SIZEOF_PIXEL], 7*SIZEOF_PIXEL, m1    ; ......t3t2t1t0lt
    pavg%4  m5, m0
    PALIGNR m0, [r0+0*FDEC_STRIDEB-8*SIZEOF_PIXEL], 7*SIZEOF_PIXEL, m1    ; ....t3t2t1t0ltl0
    mova    m1, m0
    PALIGNR m0, [r0+1*FDEC_STRIDEB-8*SIZEOF_PIXEL], 7*SIZEOF_PIXEL, m2    ; ..t3t2t1t0ltl0l1
    mova    m2, m0
    PALIGNR m0, [r0+2*FDEC_STRIDEB-8*SIZEOF_PIXEL], 7*SIZEOF_PIXEL, m3    ; t3t2t1t0ltl0l1l2
%endif
    PRED8x8_LOWPASS %4, m3, m1, m0, m2, m4
    psll%3  m1, m3, %6*6
    psrl%3  m3, %6*2
    movh    [r0+0*FDEC_STRIDEB], m5
    movh    [r0+1*FDEC_STRIDEB], m3
    PALIGNR m5, m1, 7*SIZEOF_PIXEL, m2
    psll%3  m1, %6
    movh    [r0+2*FDEC_STRIDEB], m5
    PALIGNR m3, m1, 7*SIZEOF_PIXEL, m1
    movh    [r0+3*FDEC_STRIDEB], m3
    RET

cglobal predict_4x4_hd, 1,1,6
    movh      m0, [r0-1*FDEC_STRIDEB-4*SIZEOF_PIXEL] ; lt ..
%ifdef HIGH_BIT_DEPTH
    movh      m1, [r0-1*FDEC_STRIDEB]
    punpckl%5 m0, m1                                 ; t3 t2 t1 t0 lt .. .. ..
    psll%3    m0, %6                                 ; t2 t1 t0 lt .. .. .. ..
    movh      m1, [r0+3*FDEC_STRIDEB-4*SIZEOF_PIXEL] ; l3
    movh      m2, [r0+2*FDEC_STRIDEB-4*SIZEOF_PIXEL]
    punpckl%1 m1, m2                                 ; l2 l3
    movh      m2, [r0+1*FDEC_STRIDEB-4*SIZEOF_PIXEL] ; l1
    movh      m3, [r0+0*FDEC_STRIDEB-4*SIZEOF_PIXEL]
    punpckl%1 m2, m3                                 ; l0 l1
%else
    punpckl%5 m0, [r0-1*FDEC_STRIDEB]                ; t3 t2 t1 t0 lt .. .. ..
    psll%3    m0, %6                                 ; t2 t1 t0 lt .. .. .. ..
    movu      m1, [r0+3*FDEC_STRIDEB-8*SIZEOF_PIXEL] ; l3
    punpckh%1 m1, [r0+2*FDEC_STRIDEB-8*SIZEOF_PIXEL] ; l2 l3
    movu      m2, [r0+1*FDEC_STRIDEB-8*SIZEOF_PIXEL] ; l1
    punpckh%1 m2, [r0+0*FDEC_STRIDEB-8*SIZEOF_PIXEL] ; l0 l1
%endif
    punpckh%2 m1, m2                                 ; l0 l1 l2 l3
    punpckh%5 m1, m0                                 ; t2 t1 t0 lt l0 l1 l2 l3
    psrl%3    m2, m1, %6                             ; .. t2 t1 t0 lt l0 l1 l2
    psrl%3    m0, m1, %6*2                           ; .. .. t2 t1 t0 lt l0 l1
    pavg%4    m5, m1, m2
    PRED8x8_LOWPASS %4, m3, m1, m0, m2, m4
    punpckl%1 m5, m3
    psrl%3    m3, %6*4
    PALIGNR   m3, m5, 6*SIZEOF_PIXEL, m4
%assign Y 3
    movh      [r0+Y*FDEC_STRIDEB], m5
%rep 2
%assign Y (Y-1)
    psrl%3    m5, %6*2
    movh      [r0+Y*FDEC_STRIDEB], m5
%endrep
    movh      [r0+0*FDEC_STRIDEB], m3
    RET
%endmacro

%ifdef HIGH_BIT_DEPTH
INIT_MMX mmx2
cglobal predict_4x4_ddr, 1,1
    movq    m3, [r0+3*FDEC_STRIDEB-8]
    psrlq   m3, 48
    PALIGNR m3, [r0+2*FDEC_STRIDEB-8], 6, m6
    PALIGNR m3, [r0+1*FDEC_STRIDEB-8], 6, m7
    movq    m6, [r0+0*FDEC_STRIDEB-8]
    PALIGNR m3, m6, 6, m5

    movq    m4, [r0-1*FDEC_STRIDEB-8]
    movq    m2, m3
    movq    m1, m3
    PALIGNR m2, m4, 6, m5
    movq    m1, m2
    psllq   m1, 16
    PRED8x8_LOWPASS w, m0, m3, m1, m2
    pshufw  m0, m0, q0123
    movq    [r0+3*FDEC_STRIDEB], m0

    movq    m2, [r0-1*FDEC_STRIDEB-0]
    movq    m5, m2
    PALIGNR m5, m4, 6, m4
    movq    m3, m5
    PALIGNR m5, m6, 6, m6
    PRED8x8_LOWPASS w, m1, m5, m2, m3
    movq    [r0+0*FDEC_STRIDEB], m1

    psllq   m0, 16
    PALIGNR m1, m0, 6, m2
    movq    [r0+1*FDEC_STRIDEB], m1
    psllq   m0, 16
    PALIGNR m1, m0, 6, m0
    movq    [r0+2*FDEC_STRIDEB], m1
    psrlq   m1, 16
    movd    [r0+3*FDEC_STRIDEB+4], m1
    RET

cglobal predict_4x4_hd, 1,1
    mova      m0, [r0+1*FDEC_STRIDEB-8]
    punpckhwd m0, [r0+0*FDEC_STRIDEB-8]
    mova      m1, [r0+3*FDEC_STRIDEB-8]
    punpckhwd m1, [r0+2*FDEC_STRIDEB-8]
    punpckhdq m1, m0
    mova      m0, m1
    mova      m4, m1

    movu      m3, [r0-1*FDEC_STRIDEB-2]
    mova      m7, m3
    punpckhdq m4, [r0-1*FDEC_STRIDEB-6]
    PALIGNR   m3, m1, 2, m2
    PRED8x8_LOWPASS w, m2, m4, m1, m3, m6

    pavgw     m0, m3
    mova      m5, m0
    punpcklwd m5, m2
    mova      m4, m0
    punpckhwd m4, m2
    mova      [r0+3*FDEC_STRIDEB], m5
    mova      [r0+1*FDEC_STRIDEB], m4

    mova      m4, m7
    mova      m6, [r0-1*FDEC_STRIDEB+0]
    PALIGNR   m7, [r0+0*FDEC_STRIDEB-8], 6, m5
    PRED8x8_LOWPASS w, m3, m7, m6, m4, m1

    PALIGNR   m3, m0, 6, m5
    mova      [r0+0*FDEC_STRIDEB], m3
    psrlq     m0, 16
    psrlq     m2, 16
    punpcklwd m0, m2
    mova      [r0+2*FDEC_STRIDEB], m0
    RET

INIT_XMM sse2
PREDICT_4x4 wd, dq, dq, w, qdq, 2
INIT_XMM ssse3
PREDICT_4x4 wd, dq, dq, w, qdq, 2
INIT_XMM avx
PREDICT_4x4 wd, dq, dq, w, qdq, 2
%else
INIT_MMX mmx2
PREDICT_4x4 bw, wd, q , b, dq , 8
INIT_MMX ssse3
PREDICT_4x4 bw, wd, q , b, dq , 8
%endif

;-----------------------------------------------------------------------------
; void predict_4x4_hu( pixel *src )
;-----------------------------------------------------------------------------
%ifdef HIGH_BIT_DEPTH
INIT_MMX
cglobal predict_4x4_hu_mmx2, 1,1
    movq      m0, [r0+0*FDEC_STRIDEB-4*2]
    punpckhwd m0, [r0+1*FDEC_STRIDEB-4*2]
    movq      m1, [r0+2*FDEC_STRIDEB-4*2]
    punpckhwd m1, [r0+3*FDEC_STRIDEB-4*2]
    punpckhdq m0, m1
    pshufw    m1, m1, q3333
    movq      [r0+3*FDEC_STRIDEB], m1
    movd      [r0+2*FDEC_STRIDEB+4], m1
    mova      m2, m0
    psrlq     m2, 16
    pavgw     m2, m0

    pshufw    m1, m0, q3321
    pshufw    m5, m0, q3332
    PRED8x8_LOWPASS w, m3, m0, m5, m1, m7
    movq      m6, m2
    punpcklwd m6, m3
    mova      [r0+0*FDEC_STRIDEB], m6
    psrlq     m2, 16
    psrlq     m3, 16
    punpcklwd m2, m3
    mova      [r0+1*FDEC_STRIDEB], m2
    psrlq     m2, 32
    movd      [r0+2*FDEC_STRIDEB+0], m2
    RET

%else ; !HIGH_BIT_DEPTH
INIT_MMX
cglobal predict_4x4_hu_mmx2, 1,1
    movq      mm0, [r0+0*FDEC_STRIDE-8]
    punpckhbw mm0, [r0+1*FDEC_STRIDE-8]
    movq      mm1, [r0+2*FDEC_STRIDE-8]
    punpckhbw mm1, [r0+3*FDEC_STRIDE-8]
    punpckhwd mm0, mm1
    movq      mm1, mm0
    punpckhbw mm1, mm1
    pshufw    mm1, mm1, q3333
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
%macro PREDICT_4x4_V1 3
cglobal predict_4x4_vl, 1,1,6
    movu        m1, [r0-FDEC_STRIDEB]
    psrl%1      m3, m1, %2
    psrl%1      m2, m1, %2*2
    pavg%3      m4, m3, m1
    PRED8x8_LOWPASS %3, m0, m1, m2, m3, m5

    movh        [r0+0*FDEC_STRIDEB], m4
    movh        [r0+1*FDEC_STRIDEB], m0
    psrl%1      m4, %2
    psrl%1      m0, %2
    movh        [r0+2*FDEC_STRIDEB], m4
    movh        [r0+3*FDEC_STRIDEB], m0
    RET
%endmacro

%ifdef HIGH_BIT_DEPTH
INIT_XMM sse2
PREDICT_4x4_V1 dq, 2, w
%ifdef ARCH_X86_64
INIT_XMM avx
PREDICT_4x4_V1 dq, 2, w
%endif

INIT_MMX mmx2
cglobal predict_4x4_vl, 1,4
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
INIT_MMX mmx2
PREDICT_4x4_V1 q, 8, b
%endif

;-----------------------------------------------------------------------------
; void predict_4x4_dc( pixel *src )
;-----------------------------------------------------------------------------
%ifdef HIGH_BIT_DEPTH
INIT_MMX
cglobal predict_4x4_dc_mmx2, 1,1
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
cglobal predict_4x4_dc_mmx2, 1,4
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

%macro PREDICT_FILTER 5
;-----------------------------------------------------------------------------
;void predict_8x8_filter( pixel *src, pixel edge[33], int i_neighbor, int i_filters )
;-----------------------------------------------------------------------------
cglobal predict_8x8_filter, 4,5,7
    add          r0, 0x58*SIZEOF_PIXEL
%define src r0-0x58*SIZEOF_PIXEL
%ifndef ARCH_X86_64
    mov          r4, r1
%define t1 r4
%define t4 r1
%else
%define t1 r1
%define t4 r4
%endif
    test       r3b, 0x01
    je .check_top
    mova        m0, [src+0*FDEC_STRIDEB-8*SIZEOF_PIXEL]
    punpckh%1%2 m0, [src-1*FDEC_STRIDEB-8*SIZEOF_PIXEL]
    mova        m1, [src+2*FDEC_STRIDEB-8*SIZEOF_PIXEL]
    punpckh%1%2 m1, [src+1*FDEC_STRIDEB-8*SIZEOF_PIXEL]
    punpckh%2%3 m1, m0
    mova        m2, [src+4*FDEC_STRIDEB-8*SIZEOF_PIXEL]
    punpckh%1%2 m2, [src+3*FDEC_STRIDEB-8*SIZEOF_PIXEL]
    mova        m3, [src+6*FDEC_STRIDEB-8*SIZEOF_PIXEL]
    punpckh%1%2 m3, [src+5*FDEC_STRIDEB-8*SIZEOF_PIXEL]
    punpckh%2%3 m3, m2
    punpckh%3%4 m3, m1
    mova        m0, [src+7*FDEC_STRIDEB-8*SIZEOF_PIXEL]
    mova        m1, [src-1*FDEC_STRIDEB]
    mova        m4, m3
    mova        m2, m3
    PALIGNR     m4, m0, 7*SIZEOF_PIXEL, m0
    PALIGNR     m1, m2, 1*SIZEOF_PIXEL, m2
    test       r2b, 0x08
    je .fix_lt_1
.do_left:
    mova        m0, m4
    PRED8x8_LOWPASS %1, m2, m1, m4, m3, m5
    mova        [t1+8*SIZEOF_PIXEL], m2
    mova        m4, m0
    PRED8x8_LOWPASS %1, m1, m3, m0, m4, m5
    movd        t4, m1
    mov         [t1+7*SIZEOF_PIXEL], t4%1
.check_top:
    test        r3b, 0x02
    je .done
    mova        m0, [src-1*FDEC_STRIDEB-8*SIZEOF_PIXEL]
    mova        m3, [src-1*FDEC_STRIDEB]
    mova        m1, [src-1*FDEC_STRIDEB+8*SIZEOF_PIXEL]
    mova        m2, m3
    mova        m4, m3
    PALIGNR     m2, m0, 7*SIZEOF_PIXEL, m0
    PALIGNR     m1, m4, 1*SIZEOF_PIXEL, m4
    test        r2b, 0x08
    je .fix_lt_2
    test        r2b, 0x04
    je .fix_tr_1
.do_top:
    PRED8x8_LOWPASS %1, m4, m2, m1, m3, m5
    mova        [t1+16*SIZEOF_PIXEL], m4
    test        r3b, 0x04
    je .done
    test        r2b, 0x04
    je .fix_tr_2
    mova        m0, [src-1*FDEC_STRIDEB+8*SIZEOF_PIXEL]
    mova        m2, m0
    mova        m4, m0
    psrl%4      m5, m0, 7*%5
    PALIGNR     m2, m3, 7*SIZEOF_PIXEL, m3
    PALIGNR     m5, m4, 1*SIZEOF_PIXEL, m4
    PRED8x8_LOWPASS %1, m1, m2, m5, m0, m4
    jmp .do_topright
.fix_tr_2:
    punpckh%1%2 m3, m3
    pshuf%2     m1, m3, q3333
.do_topright:
    mova        [t1+24*SIZEOF_PIXEL], m1
    psrl%4      m1, 7*%5
    movd        t4, m1
    mov         [t1+32*SIZEOF_PIXEL], t4%1
.done:
    REP_RET
.fix_lt_1:
    pxor        m5, m3, m4
    psrl%4      m5, 7*%5
    psll%4      m5, 6*%5
    pxor        m1, m5
    jmp .do_left
.fix_lt_2:
    pxor        m5, m3, m2
    psll%4      m5, 7*%5
    psrl%4      m5, 7*%5
    pxor        m2, m5
    test       r2b, 0x04
    jne .do_top
.fix_tr_1:
    pxor        m5, m3, m1
    psrl%4      m5, 7*%5
    psll%4      m5, 7*%5
    pxor        m1, m5
    jmp .do_top
%endmacro

%ifdef HIGH_BIT_DEPTH
INIT_XMM sse2
PREDICT_FILTER w, d, q, dq, 2
INIT_XMM ssse3
PREDICT_FILTER w, d, q, dq, 2
INIT_XMM avx
PREDICT_FILTER w, d, q, dq, 2
%else
INIT_MMX mmx2
PREDICT_FILTER b, w, d, q , 8
INIT_MMX ssse3
PREDICT_FILTER b, w, d, q , 8
%endif

;-----------------------------------------------------------------------------
; void predict_8x8_v( pixel *src, pixel *edge )
;-----------------------------------------------------------------------------
%macro PREDICT_8x8_V 0
cglobal predict_8x8_v, 2,2
    mova        m0, [r1+16*SIZEOF_PIXEL]
    STORE8x8    m0, m0
    RET
%endmacro

%ifdef HIGH_BIT_DEPTH
INIT_XMM sse2
PREDICT_8x8_V
%else
INIT_MMX mmx2
PREDICT_8x8_V
%endif

;-----------------------------------------------------------------------------
; void predict_8x8_h( pixel *src, pixel edge[33] )
;-----------------------------------------------------------------------------
%macro PREDICT_8x8_H 2
cglobal predict_8x8_h, 2,2
    movu      m1, [r1+7*SIZEOF_PIXEL]
    add       r0, 4*FDEC_STRIDEB
    punpckl%1 m2, m1, m1
    punpckh%1 m1, m1
%assign n 0
%rep 8
%assign i 1+n/4
    SPLAT%2 m0, m %+ i, (3-n)&3
    mova [r0+(n-4)*FDEC_STRIDEB], m0
%assign n n+1
%endrep
    RET
%endmacro

%ifdef HIGH_BIT_DEPTH
INIT_XMM sse2
PREDICT_8x8_H wd, D
%else
INIT_MMX mmx2
PREDICT_8x8_H bw, W
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
cglobal predict_8x8_dc_mmx2, 2,2
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
%macro PREDICT_8x8_DC 3
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
PREDICT_8x8_DC predict_8x8_dc_top_sse2 , 32, mova
PREDICT_8x8_DC predict_8x8_dc_left_sse2, 14, movu

%else
%macro PREDICT_8x8_DC 2
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
PREDICT_8x8_DC predict_8x8_dc_top_mmx2, 16
PREDICT_8x8_DC predict_8x8_dc_left_mmx2, 7
%endif ; HIGH_BIT_DEPTH

; sse2 is faster even on amd for 8-bit, so there's no sense in spending exe
; size on the 8-bit mmx functions below if we know sse2 is available.
%macro PREDICT_8x8 3
;-----------------------------------------------------------------------------
; void predict_8x8_ddl( pixel *src, pixel *edge )
;-----------------------------------------------------------------------------
cglobal predict_8x8_ddl, 2,2,8
    mova        m5, [r1+16*SIZEOF_PIXEL]
    movu        m2, [r1+17*SIZEOF_PIXEL]
    movu        m3, [r1+23*SIZEOF_PIXEL]
    movu        m4, [r1+25*SIZEOF_PIXEL]
    psll%2      m1, m5, %3
    add         r0, FDEC_STRIDEB*4
    PRED8x8_LOWPASS %1, m0, m1, m2, m5, m7
%assign %%bak avx_enabled
%assign avx_enabled 0
    PRED8x8_LOWPASS %1, m1, m3, m4, [r1+24*SIZEOF_PIXEL], m6
%assign avx_enabled %%bak
%assign Y 3
%rep 6
    mova        [r0+Y*FDEC_STRIDEB], m1
    psll%2      m1, %3
    psrl%2      m2, m0, 7*%3
    psll%2      m0, %3
    por         m1, m2
%assign Y (Y-1)
%endrep
    mova        [r0+Y*FDEC_STRIDEB], m1
    psll%2      m1, %3
    psrl%2      m0, 7*%3
    por         m1, m0
%assign Y (Y-1)
    mova        [r0+Y*FDEC_STRIDEB], m1
    RET

;-----------------------------------------------------------------------------
; void predict_8x8_ddr( pixel *src, pixel *edge )
;-----------------------------------------------------------------------------
%if avx_enabled == 0
cglobal predict_8x8_ddr, 2,2,7
    movu        m1, [r1+ 7*SIZEOF_PIXEL]
    movu        m2, [r1+ 9*SIZEOF_PIXEL]
    movu        m3, [r1+15*SIZEOF_PIXEL]
    movu        m4, [r1+17*SIZEOF_PIXEL]
    add         r0, FDEC_STRIDEB*4
    PRED8x8_LOWPASS %1, m0, m1, m2, [r1+ 8*SIZEOF_PIXEL], m5
    PRED8x8_LOWPASS %1, m1, m3, m4, [r1+16*SIZEOF_PIXEL], m6
%assign Y 3
%rep 6
    mova        [r0+Y*FDEC_STRIDEB], m0
    psrl%2      m0, %3
    psll%2      m2, m1, 7*%3
    psrl%2      m1, %3
    por         m0, m2
%assign Y (Y-1)
%endrep
    mova        [r0+Y*FDEC_STRIDEB], m0
    psrl%2      m0, %3
    psll%2      m1, 7*%3
    por         m0, m1
%assign Y (Y-1)
    mova        [r0+Y*FDEC_STRIDEB], m0
    RET
%endif
%endmacro ; PREDICT_8x8

%ifdef HIGH_BIT_DEPTH
INIT_XMM sse2
PREDICT_8x8 w, dq, 2
INIT_XMM avx
PREDICT_8x8 w, dq, 2
%elifndef ARCH_X86_64
INIT_MMX mmx2
PREDICT_8x8 b, q , 8
%endif

;-----------------------------------------------------------------------------
; void predict_8x8_hu( pixel *src, pixel *edge )
;-----------------------------------------------------------------------------
%macro PREDICT_8x8_HU 5
cglobal predict_8x8_hu, 2,2,8
    movu      m1, [r1+7*SIZEOF_PIXEL] ; l0 l1 l2 l3 l4 l5 l6 l7
    add       r0, 4*FDEC_STRIDEB
    pshuf%3   m0, m1, q0123           ; l6 l7 l4 l5 l2 l3 l0 l1
    psll%2    m1, 7*%5                ; l7 .. .. .. .. .. .. ..
    mova      m2, m0
    psll%3    m0, 8*SIZEOF_PIXEL
    psrl%3    m2, 8*SIZEOF_PIXEL
    por       m2, m0                  ; l7 l6 l5 l4 l3 l2 l1 l0
    mova      m4, m2
    mova      m5, m2
    psrl%2    m3, m2, 2*%5
    psrl%2    m2, %5
    por       m2, m1                  ; l7 l7 l6 l5 l4 l3 l2 l1
    punpckh%4 m1, m1
    por       m3, m1                  ; l7 l7 l7 l6 l5 l4 l3 l2
    pavg%1    m4, m2
    PRED8x8_LOWPASS %1, m1, m3, m5, m2, m6
    punpckh%4 m5, m4, m1              ; p8 p7 p6 p5
    punpckl%4 m4, m1                  ; p4 p3 p2 p1
    mova      m6, m5
    mova      m7, m5
    mova      m0, m5
    PALIGNR   m5, m4, 2*SIZEOF_PIXEL, m1
    pshuf%3   m1, m6, q3321
    PALIGNR   m6, m4, 4*SIZEOF_PIXEL, m2
    pshuf%3   m2, m7, q3332
    PALIGNR   m7, m4, 6*SIZEOF_PIXEL, m3
    pshuf%3   m3, m0, q3333
    mova      [r0-4*FDEC_STRIDEB], m4
    mova      [r0-3*FDEC_STRIDEB], m5
    mova      [r0-2*FDEC_STRIDEB], m6
    mova      [r0-1*FDEC_STRIDEB], m7
    mova      [r0+0*FDEC_STRIDEB], m0
    mova      [r0+1*FDEC_STRIDEB], m1
    mova      [r0+2*FDEC_STRIDEB], m2
    mova      [r0+3*FDEC_STRIDEB], m3
    RET
%endmacro

%ifdef HIGH_BIT_DEPTH
INIT_XMM sse2
PREDICT_8x8_HU w, dq, d, wd, 2
INIT_XMM ssse3
PREDICT_8x8_HU w, dq, d, wd, 2
INIT_XMM avx
PREDICT_8x8_HU w, dq, d, wd, 2
%elifndef ARCH_X86_64
INIT_MMX mmx2
PREDICT_8x8_HU b, q , w, bw, 8
%endif

;-----------------------------------------------------------------------------
; void predict_8x8_vr( pixel *src, pixel *edge )
;-----------------------------------------------------------------------------
%macro PREDICT_8x8_VR 3
cglobal predict_8x8_vr, 2,3,7
    mova        m2, [r1+16*SIZEOF_PIXEL]
    movu        m3, [r1+15*SIZEOF_PIXEL]
    movu        m1, [r1+14*SIZEOF_PIXEL]
    pavg%1      m4, m3, m2
    add         r0, FDEC_STRIDEB*4
    PRED8x8_LOWPASS %1, m0, m1, m2, m3, m5
    mova        [r0-4*FDEC_STRIDEB], m4
    mova        [r0-3*FDEC_STRIDEB], m0
    mova        m5, m0
    mova        m6, m4
    mova        m1, [r1+8*SIZEOF_PIXEL]
    mova        m2, m1
    psll%2      m2, %3
    mova        m3, m1
    psll%2      m3, 2*%3
    PRED8x8_LOWPASS %1, m0, m1, m3, m2, m4

%assign Y -2
%rep 5
    %assign i (5 + ((Y+3)&1))
    PALIGNR     m %+ i, m0, 7*SIZEOF_PIXEL, m2
    mova        [r0+Y*FDEC_STRIDEB], m %+ i
    psll%2      m0, %3
%assign Y (Y+1)
%endrep
    PALIGNR     m5, m0, 7*SIZEOF_PIXEL, m0
    mova        [r0+Y*FDEC_STRIDEB], m5
    RET
%endmacro

%ifdef HIGH_BIT_DEPTH
INIT_XMM sse2
PREDICT_8x8_VR w, dq, 2
INIT_XMM ssse3
PREDICT_8x8_VR w, dq, 2
INIT_XMM avx
PREDICT_8x8_VR w, dq, 2
%elifndef ARCH_X86_64
INIT_MMX mmx2
PREDICT_8x8_VR b, q , 8
%endif

;-----------------------------------------------------------------------------
; void predict_8x8c_p_core( uint8_t *src, int i00, int b, int c )
;-----------------------------------------------------------------------------
%ifndef ARCH_X86_64
INIT_MMX
cglobal predict_8x8c_p_core_mmx2, 1,2
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
%endif ; !ARCH_X86_64

INIT_XMM
%ifdef HIGH_BIT_DEPTH
cglobal predict_8x8c_p_core_sse2, 1,1,7
    movd        m0, r1m
    movd        m2, r2m
    movd        m4, r3m
    mova        m3, [pw_pixel_max]
    pxor        m1, m1
    SPLATW      m0, m0, 0
    SPLATW      m2, m2, 0
    SPLATW      m4, m4, 0
    pmullw      m2, [pw_43210123] ; b
    pmullw      m5, m4, [pw_m3]   ; c
    paddw       m5, [pw_16]
    mov        r1d, 8
.loop:
    paddsw      m6, m2, m5
    paddsw      m6, m0
    psraw       m6, 5
    CLIPW       m6, m1, m3
    mova      [r0], m6
    paddw       m5, m4
    add         r0, FDEC_STRIDEB
    dec r1d
    jg .loop
    REP_RET
%else ; !HIGH_BIT_DEPTH
cglobal predict_8x8c_p_core_sse2, 1,1
    movd        m0, r1m
    movd        m2, r2m
    movd        m4, r3m
    SPLATW      m0, m0, 0
    SPLATW      m2, m2, 0
    SPLATW      m4, m4, 0
    pmullw      m2, [pw_76543210]
    paddsw      m0, m2            ; m0 = {i+0*b, i+1*b, i+2*b, i+3*b, i+4*b, i+5*b, i+6*b, i+7*b}
    paddsw      m3, m0, m4
    paddsw      m4, m4
call .loop
    add         r0, FDEC_STRIDE*4
.loop:
    paddsw      m1, m3, m4
    paddsw      m5, m0, m4
    psraw       m3, 5
    psraw       m0, 5
    packuswb    m0, m3
    movq        [r0+FDEC_STRIDE*0], m0
    movhps      [r0+FDEC_STRIDE*1], m0
    paddsw      m0, m5, m4
    paddsw      m3, m1, m4
    psraw       m5, 5
    psraw       m1, 5
    packuswb    m5, m1
    movq        [r0+FDEC_STRIDE*2], m5
    movhps      [r0+FDEC_STRIDE*3], m5
    RET
%endif ; HIGH_BIT_DEPTH

;-----------------------------------------------------------------------------
; void predict_16x16_p_core( uint8_t *src, int i00, int b, int c )
;-----------------------------------------------------------------------------
%ifndef ARCH_X86_64
cglobal predict_16x16_p_core_mmx2, 1,2
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

%macro PREDICT_16x16_P 0
cglobal predict_16x16_p_core, 1,2,8
    movd     m0, r1m
    movd     m1, r2m
    movd     m2, r3m
    SPLATW   m0, m0, 0
    SPLATW   m1, m1, 0
    SPLATW   m2, m2, 0
    pmullw   m3, m1, [pw_76543210]
    psllw    m1, 3
%ifdef HIGH_BIT_DEPTH
    pxor     m6, m6
    mov     r1d, 16
.loop:
    mova     m4, m0
    mova     m5, m0
    mova     m7, m3
    paddsw   m7, m6
    paddsw   m4, m7
    paddsw   m7, m1
    paddsw   m5, m7
    psraw    m4, 5
    psraw    m5, 5
    CLIPW    m4, [pb_0], [pw_pixel_max]
    CLIPW    m5, [pb_0], [pw_pixel_max]
    mova   [r0], m4
    mova [r0+16], m5
    add      r0, FDEC_STRIDEB
    paddw    m6, m2
    dec      r1d
    jg       .loop
%else ; !HIGH_BIT_DEPTH
    paddsw   m0, m3  ; m0 = {i+ 0*b, i+ 1*b, i+ 2*b, i+ 3*b, i+ 4*b, i+ 5*b, i+ 6*b, i+ 7*b}
    paddsw   m1, m0  ; m1 = {i+ 8*b, i+ 9*b, i+10*b, i+11*b, i+12*b, i+13*b, i+14*b, i+15*b}
    paddsw   m7, m2, m2
    mov     r1d, 8
ALIGN 4
.loop:
    psraw    m3, m0, 5
    psraw    m4, m1, 5
    paddsw   m5, m0, m2
    paddsw   m6, m1, m2
    psraw    m5, 5
    psraw    m6, 5
    packuswb m3, m4
    packuswb m5, m6
    mova [r0+FDEC_STRIDE*0], m3
    mova [r0+FDEC_STRIDE*1], m5
    paddsw   m0, m7
    paddsw   m1, m7
    add      r0, FDEC_STRIDE*2
    dec      r1d
    jg       .loop
%endif ; !HIGH_BIT_DEPTH
    REP_RET
%endmacro ; PREDICT_16x16_P

INIT_XMM sse2
PREDICT_16x16_P
%ifndef HIGH_BIT_DEPTH
INIT_XMM avx
PREDICT_16x16_P
%endif

%ifndef HIGH_BIT_DEPTH
%macro PREDICT_8x8 0
;-----------------------------------------------------------------------------
; void predict_8x8_ddl( uint8_t *src, uint8_t *edge )
;-----------------------------------------------------------------------------
cglobal predict_8x8_ddl, 2,2
    movdqa      xmm3, [r1+16]
    movdqu      xmm2, [r1+17]
    pslldq      xmm1, xmm3, 1
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
cglobal predict_8x8_ddr, 2,2
    movdqu      xmm3, [r1+8]
    movdqu      xmm1, [r1+7]
    psrldq      xmm2, xmm3, 1
    add           r0, FDEC_STRIDE*4
    PRED8x8_LOWPASS b, xmm0, xmm1, xmm2, xmm3, xmm4

    psrldq      xmm1, xmm0, 1
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
cglobal predict_8x8_vl, 2,2
    movdqa      xmm4, [r1+16]
    pslldq      xmm1, xmm4, 1
    psrldq      xmm2, xmm4, 1
    pavgb       xmm3, xmm4, xmm2
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
cglobal predict_8x8_vr, 2,2,7
    movdqu      xmm0, [r1+8]
    movdqa      xmm6, [pw_ff00]
    add         r0, 4*FDEC_STRIDE
    movdqa      xmm2, xmm0
    movdqa      xmm3, xmm0
    pslldq      xmm1, xmm0, 2
    pslldq      xmm0, 1
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
%endmacro ; PREDICT_8x8

INIT_XMM sse2
PREDICT_8x8
INIT_XMM avx
PREDICT_8x8

%endif ; !HIGH_BIT_DEPTH

;-----------------------------------------------------------------------------
; void predict_8x8_hd( pixel *src, pixel *edge )
;-----------------------------------------------------------------------------
%macro PREDICT_8x8_HD 4
cglobal predict_8x8_hd, 2,2,8
    add       r0, 4*FDEC_STRIDEB
    mova      m0, [r1]                     ; l7 .. .. .. .. .. .. ..
    mova      m1, [r1+ 8*SIZEOF_PIXEL]     ; lt l0 l1 l2 l3 l4 l5 l6
    mova      m2, [r1+16*SIZEOF_PIXEL]     ; t7 t6 t5 t4 t3 t2 t1 t0
    mova      m3, m1                       ; lt l0 l1 l2 l3 l4 l5 l6
    mova      m4, m2                       ; t7 t6 t5 t4 t3 t2 t1 t0
    PALIGNR   m2, m1, 7*SIZEOF_PIXEL, m5   ; t6 t5 t4 t3 t2 t1 t0 lt
    PALIGNR   m1, m0, 7*SIZEOF_PIXEL, m6   ; l0 l1 l2 l3 l4 l5 l6 l7
    PALIGNR   m4, m3, 1*SIZEOF_PIXEL, m7   ; t0 lt l0 l1 l2 l3 l4 l5
    mova      m5, m3
    pavg%1    m3, m1
    PRED8x8_LOWPASS %1, m0, m4, m1, m5, m7
    psrl%2    m4, m2, 2*%4                 ; .. .. t6 t5 t4 t3 t2 t1
    psrl%2    m1, m2, %4                   ; .. t6 t5 t4 t3 t2 t1 t0
    PRED8x8_LOWPASS %1, m6, m4, m2, m1, m5
                                           ; .. p11 p10 p9
    punpckh%3 m7, m3, m0                   ; p8 p7 p6 p5
    punpckl%3 m3, m0                       ; p4 p3 p2 p1
    mova      m1, m7
    mova      m0, m7
    mova      m4, m7
    mova      [r0+3*FDEC_STRIDEB], m3
    PALIGNR   m7, m3, 2*SIZEOF_PIXEL, m5
    mova      [r0+2*FDEC_STRIDEB], m7
    PALIGNR   m1, m3, 4*SIZEOF_PIXEL, m5
    mova      [r0+1*FDEC_STRIDEB], m1
    PALIGNR   m0, m3, 6*SIZEOF_PIXEL, m3
    mova      [r0+0*FDEC_STRIDEB], m0
    mova      m2, m6
    mova      m3, m6
    mova      [r0-1*FDEC_STRIDEB], m4
    PALIGNR   m6, m4, 2*SIZEOF_PIXEL, m5
    mova      [r0-2*FDEC_STRIDEB], m6
    PALIGNR   m2, m4, 4*SIZEOF_PIXEL, m5
    mova      [r0-3*FDEC_STRIDEB], m2
    PALIGNR   m3, m4, 6*SIZEOF_PIXEL, m4
    mova      [r0-4*FDEC_STRIDEB], m3
    RET
%endmacro

%ifdef HIGH_BIT_DEPTH
INIT_XMM sse2
PREDICT_8x8_HD w, dq, wd, 2
INIT_XMM ssse3
PREDICT_8x8_HD w, dq, wd, 2
INIT_XMM avx
PREDICT_8x8_HD w, dq, wd, 2
%else
INIT_MMX mmx2
PREDICT_8x8_HD b, q , bw, 8

;-----------------------------------------------------------------------------
; void predict_8x8_hd( uint8_t *src, uint8_t *edge )
;-----------------------------------------------------------------------------
%macro PREDICT_8x8_HD 0
cglobal predict_8x8_hd, 2,2
    add       r0, 4*FDEC_STRIDE
    movdqa  xmm0, [r1]
    movdqa  xmm1, [r1+16]
    movdqa  xmm2, xmm1
    movdqa  xmm3, xmm1
    PALIGNR xmm1, xmm0, 7, xmm4
    PALIGNR xmm2, xmm0, 9, xmm5
    PALIGNR xmm3, xmm0, 8, xmm0
    pavgb   xmm4, xmm1, xmm3
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

INIT_XMM sse2
PREDICT_8x8_HD
INIT_XMM ssse3
PREDICT_8x8_HD
INIT_XMM avx
PREDICT_8x8_HD
%endif ; HIGH_BIT_DEPTH

;-----------------------------------------------------------------------------
; void predict_8x8_hu( uint8_t *src, uint8_t *edge )
;-----------------------------------------------------------------------------
%macro PREDICT_8x8_HU 0
cglobal predict_8x8_hu, 2,2
    add        r0, 4*FDEC_STRIDE
%if cpuflag(ssse3)
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
    pshufw    mm0, mm1, q0123       ; l6 l7 l4 l5 l2 l3 l0 l1
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
    pshufw     mm5, mm4, q3321
    pshufw     mm6, mm4, q3332
    pshufw     mm7, mm4, q3333
    movq     [r0+Y*FDEC_STRIDE], xmm0
    movq     [r0+0*FDEC_STRIDE], mm4
    movq     [r0+1*FDEC_STRIDE], mm5
    movq     [r0+2*FDEC_STRIDE], mm6
    movq     [r0+3*FDEC_STRIDE], mm7
    RET
%endmacro

%ifndef HIGH_BIT_DEPTH
INIT_MMX sse2
PREDICT_8x8_HU
INIT_MMX ssse3
PREDICT_8x8_HU
%endif

;-----------------------------------------------------------------------------
; void predict_8x8c_v( uint8_t *src )
;-----------------------------------------------------------------------------

%macro PREDICT_8x8C_V 0
cglobal predict_8x8c_v, 1,1
    mova        m0, [r0 - FDEC_STRIDEB]
    STORE8x8    m0, m0
    RET
%endmacro

%ifdef HIGH_BIT_DEPTH
INIT_XMM sse2
PREDICT_8x8C_V
%else
INIT_MMX mmx
PREDICT_8x8C_V
%endif

%ifdef HIGH_BIT_DEPTH

INIT_MMX
cglobal predict_8x8c_v_mmx, 1,1
    mova        m0, [r0 - FDEC_STRIDEB]
    mova        m1, [r0 - FDEC_STRIDEB + 8]
%assign n 0
%rep 8
    mova        [r0 + (n&1)*FDEC_STRIDEB], m0
    mova        [r0 + (n&1)*FDEC_STRIDEB + 8], m1
%if (n&1) && (n!=7)
    add         r0, FDEC_STRIDEB*2
%endif
%assign n n+1
%endrep
    RET

%endif

;-----------------------------------------------------------------------------
; void predict_8x8c_h( uint8_t *src )
;-----------------------------------------------------------------------------
%ifdef HIGH_BIT_DEPTH

INIT_XMM sse2
cglobal predict_8x8c_h, 1,1
    add        r0, FDEC_STRIDEB*4
%assign n -4
%rep 8
    movd       m0, [r0+FDEC_STRIDEB*n-SIZEOF_PIXEL*2]
    SPLATW     m0, m0, 1
    mova [r0+FDEC_STRIDEB*n], m0
%assign n n+1
%endrep
    RET

%else

%macro PREDICT_8x8C_H 0
cglobal predict_8x8c_h, 1,1
%if cpuflag(ssse3)
    mova   m1, [pb_3]
%endif
    add    r0, FDEC_STRIDE*4
%assign n -4
%rep 8
    SPLATB_LOAD m0, r0+FDEC_STRIDE*n-1, m1
    mova [r0+FDEC_STRIDE*n], m0
%assign n n+1
%endrep
    RET
%endmacro

INIT_MMX mmx2
PREDICT_8x8C_H
INIT_MMX ssse3
PREDICT_8x8C_H

%endif
;-----------------------------------------------------------------------------
; void predict_8x8c_dc( pixel *src )
;-----------------------------------------------------------------------------

%macro PREDICT_8x8C_DC 0
cglobal predict_8x8c_dc, 1,3
    pxor      m7, m7
%ifdef HIGH_BIT_DEPTH
    movq      m0, [r0-FDEC_STRIDEB+0]
    movq      m1, [r0-FDEC_STRIDEB+8]
    HADDW     m0, m2
    HADDW     m1, m2
%else
    movd      m0, [r0-FDEC_STRIDEB+0]
    movd      m1, [r0-FDEC_STRIDEB+4]
    psadbw    m0, m7            ; s0
    psadbw    m1, m7            ; s1
%endif
    add       r0, FDEC_STRIDEB*4

    movzx    r1d, pixel [r0-FDEC_STRIDEB*4-SIZEOF_PIXEL]
    movzx    r2d, pixel [r0-FDEC_STRIDEB*3-SIZEOF_PIXEL]
    add      r1d, r2d
    movzx    r2d, pixel [r0-FDEC_STRIDEB*2-SIZEOF_PIXEL]
    add      r1d, r2d
    movzx    r2d, pixel [r0-FDEC_STRIDEB*1-SIZEOF_PIXEL]
    add      r1d, r2d
    movd      m2, r1d            ; s2

    movzx    r1d, pixel [r0+FDEC_STRIDEB*0-SIZEOF_PIXEL]
    movzx    r2d, pixel [r0+FDEC_STRIDEB*1-SIZEOF_PIXEL]
    add      r1d, r2d
    movzx    r2d, pixel [r0+FDEC_STRIDEB*2-SIZEOF_PIXEL]
    add      r1d, r2d
    movzx    r2d, pixel [r0+FDEC_STRIDEB*3-SIZEOF_PIXEL]
    add      r1d, r2d
    movd      m3, r1d            ; s3

    punpcklwd m0, m1
    punpcklwd m2, m3
    punpckldq m0, m2            ; s0, s1, s2, s3
    pshufw    m3, m0, q3312     ; s2, s1, s3, s3
    pshufw    m0, m0, q1310     ; s0, s1, s3, s1
    paddw     m0, m3
    psrlw     m0, 2
    pavgw     m0, m7            ; s0+s2, s1, s3, s1+s3
%ifdef HIGH_BIT_DEPTH
%if cpuflag(sse2)
    movq2dq   xmm0, m0
    punpcklwd xmm0, xmm0
    pshufd    xmm1, xmm0, q3322
    punpckldq xmm0, xmm0
%assign n 0
%rep 8
%assign i (0 + (n/4))
    movdqa [r0+FDEC_STRIDEB*(n-4)+0], xmm %+ i
%assign n n+1
%endrep
%else
    pshufw    m1, m0, q0000
    pshufw    m2, m0, q1111
    pshufw    m3, m0, q2222
    pshufw    m4, m0, q3333
%assign n 0
%rep 8
%assign i (1 + (n/4)*2)
%assign j (2 + (n/4)*2)
    movq [r0+FDEC_STRIDEB*(n-4)+0], m %+ i
    movq [r0+FDEC_STRIDEB*(n-4)+8], m %+ j
%assign n n+1
%endrep
%endif
%else
    packuswb  m0, m0
    punpcklbw m0, m0
    movq      m1, m0
    punpcklbw m0, m0
    punpckhbw m1, m1
%assign n 0
%rep 8
%assign i (0 + (n/4))
    movq [r0+FDEC_STRIDEB*(n-4)], m %+ i
%assign n n+1
%endrep
%endif
    RET
%endmacro

INIT_MMX mmx2
PREDICT_8x8C_DC
%ifdef HIGH_BIT_DEPTH
INIT_MMX sse2
PREDICT_8x8C_DC
%endif

%ifdef HIGH_BIT_DEPTH

INIT_XMM
cglobal predict_8x8c_dc_top_sse2, 1,1
    pxor        m2, m2
    mova        m0, [r0 - FDEC_STRIDEB]
    pshufd      m1, m0, q2301
    paddw       m0, m1
    pshuflw     m1, m0, q2301
    pshufhw     m1, m1, q2301
    paddw       m0, m1
    psrlw       m0, 1
    pavgw       m0, m2
    STORE8x8    m0, m0
    RET

%else

INIT_MMX
cglobal predict_8x8c_dc_top_mmx2, 1,1
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

%endif

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
%macro PREDICT_16x16_H 0
cglobal predict_16x16_h, 1,2
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
%if cpuflag(ssse3)
    mova   m1, [pb_3]
%endif
.vloop:
%assign n 0
%rep 4
    SPLATB_LOAD m0, r0+r1+FDEC_STRIDE*n-1, m1
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

INIT_MMX mmx2
PREDICT_16x16_H
INIT_XMM sse2
%ifdef HIGH_BIT_DEPTH
PREDICT_16x16_H
%else
;no SSE2 for 8-bit, it's slower than MMX on all systems that don't support SSSE3
INIT_XMM ssse3
PREDICT_16x16_H
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
cglobal predict_16x16_dc_core_mmx2, 1,2
%ifdef ARCH_X86_64
    movd         m6, r1d
    PRED16x16_DC m6, 5
%else
    PRED16x16_DC r1m, 5
%endif
    REP_RET

INIT_MMX
cglobal predict_16x16_dc_top_mmx2, 1,2
    PRED16x16_DC [pw_8], 4
    REP_RET

INIT_MMX
%ifdef HIGH_BIT_DEPTH
cglobal predict_16x16_dc_left_core_mmx2, 1,2
    movd       m0, r1m
    SPLATW     m0, m0
    STORE16x16 m0, m0, m0, m0
    REP_RET
%else
cglobal predict_16x16_dc_left_core_mmx2, 1,1
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

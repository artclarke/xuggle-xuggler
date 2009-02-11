;*****************************************************************************
;* predict-a.asm: h264 encoder library
;*****************************************************************************
;* Copyright (C) 2005-2008 x264 project
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
;*****************************************************************************

%include "x86inc.asm"
%include "x86util.asm"

%macro STORE8x8 2
    add r0, 4*FDEC_STRIDE
    movq        [r0 + -4*FDEC_STRIDE], %1
    movq        [r0 + -3*FDEC_STRIDE], %1
    movq        [r0 + -2*FDEC_STRIDE], %1
    movq        [r0 + -1*FDEC_STRIDE], %1
    movq        [r0 +  0*FDEC_STRIDE], %2
    movq        [r0 +  1*FDEC_STRIDE], %2
    movq        [r0 +  2*FDEC_STRIDE], %2
    movq        [r0 +  3*FDEC_STRIDE], %2
%endmacro

%macro STORE16x16 2
    mov         r1d, 4
.loop:
    movq        [r0 + 0*FDEC_STRIDE], %1
    movq        [r0 + 1*FDEC_STRIDE], %1
    movq        [r0 + 2*FDEC_STRIDE], %1
    movq        [r0 + 3*FDEC_STRIDE], %1
    movq        [r0 + 0*FDEC_STRIDE + 8], %2
    movq        [r0 + 1*FDEC_STRIDE + 8], %2
    movq        [r0 + 2*FDEC_STRIDE + 8], %2
    movq        [r0 + 3*FDEC_STRIDE + 8], %2
    add         r0, 4*FDEC_STRIDE
    dec         r1d
    jg          .loop
%endmacro

%macro STORE16x16_SSE2 1
    add r0, 4*FDEC_STRIDE
    movdqa      [r0 + -4*FDEC_STRIDE], %1
    movdqa      [r0 + -3*FDEC_STRIDE], %1
    movdqa      [r0 + -2*FDEC_STRIDE], %1
    movdqa      [r0 + -1*FDEC_STRIDE], %1
    movdqa      [r0 +  0*FDEC_STRIDE], %1
    movdqa      [r0 +  1*FDEC_STRIDE], %1
    movdqa      [r0 +  2*FDEC_STRIDE], %1
    movdqa      [r0 +  3*FDEC_STRIDE], %1
    add r0, 8*FDEC_STRIDE
    movdqa      [r0 + -4*FDEC_STRIDE], %1
    movdqa      [r0 + -3*FDEC_STRIDE], %1
    movdqa      [r0 + -2*FDEC_STRIDE], %1
    movdqa      [r0 + -1*FDEC_STRIDE], %1
    movdqa      [r0 +  0*FDEC_STRIDE], %1
    movdqa      [r0 +  1*FDEC_STRIDE], %1
    movdqa      [r0 +  2*FDEC_STRIDE], %1
    movdqa      [r0 +  3*FDEC_STRIDE], %1
%endmacro

SECTION_RODATA

ALIGN 16
pb_1:       times 16 db 1
pb_3:       times 16 db 3
pw_2:       times 4 dw 2
pw_4:       times 4 dw 4
pw_8:       times 8 dw 8
pw_76543210:
pw_3210:    dw 0, 1, 2, 3, 4, 5, 6, 7
pb_00s_ff:  times 8 db 0
pb_0s_ff:   times 7 db 0
            db 0xff
pw_ff00:    times 8 dw 0xff00
pb_reverse: db 7, 6, 5, 4, 3, 2, 1, 0

SECTION .text

; dest, left, right, src, tmp
; output: %1 = (t[n-1] + t[n]*2 + t[n+1] + 2) >> 2
%macro PRED8x8_LOWPASS0 6
    mov%6       %5, %2
    pavgb       %2, %3
    pxor        %3, %5
    mov%6       %1, %4
    pand        %3, [pb_1 GLOBAL]
    psubusb     %2, %3
    pavgb       %1, %2
%endmacro
%macro PRED8x8_LOWPASS 5
    PRED8x8_LOWPASS0 %1, %2, %3, %4, %5, q
%endmacro
%macro PRED8x8_LOWPASS_XMM 5
    PRED8x8_LOWPASS0 %1, %2, %3, %4, %5, dqa
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
; void predict_4x4_ddl_mmxext( uint8_t *src )
;-----------------------------------------------------------------------------
cglobal predict_4x4_ddl_mmxext, 1,1
    movq    mm1, [r0-FDEC_STRIDE]
    movq    mm2, mm1
    movq    mm3, mm1
    movq    mm4, mm1
    psllq   mm1, 8
    pxor    mm2, mm1
    psrlq   mm2, 8
    pxor    mm3, mm2
    PRED8x8_LOWPASS mm0, mm1, mm3, mm4, mm5

%assign Y 0
%rep 4
    psrlq       mm0, 8
    movd        [r0+Y*FDEC_STRIDE], mm0
%assign Y (Y+1)
%endrep

    RET

;-----------------------------------------------------------------------------
; void predict_4x4_ddr_mmxext( uint8_t *src )
;-----------------------------------------------------------------------------
%macro PREDICT_4x4 1
cglobal predict_4x4_ddr_%1, 1,1
    movq      mm1, [r0+1*FDEC_STRIDE-8]
    movq      mm2, [r0+0*FDEC_STRIDE-8]
    punpckhbw mm2, [r0-1*FDEC_STRIDE-8]
    movd      mm3, [r0-1*FDEC_STRIDE]
    punpckhwd mm1, mm2
    PALIGNR   mm3, mm1, 5, mm1
    movq      mm1, mm3
    PALIGNR   mm3, [r0+2*FDEC_STRIDE-8], 7, mm4
    movq      mm2, mm3
    PALIGNR   mm3, [r0+3*FDEC_STRIDE-8], 7, mm4
    PRED8x8_LOWPASS mm0, mm3, mm1, mm2, mm4
%assign Y 3
    movd    [r0+Y*FDEC_STRIDE], mm0
%rep 3
%assign Y (Y-1)
    psrlq    mm0, 8
    movd    [r0+Y*FDEC_STRIDE], mm0
%endrep
    RET

cglobal predict_4x4_vr_%1, 1,1
    movd    mm0, [r0-1*FDEC_STRIDE]              ; ........t3t2t1t0
    movq    mm7, mm0
    PALIGNR mm0, [r0-1*FDEC_STRIDE-8], 7, mm1    ; ......t3t2t1t0lt
    pavgb   mm7, mm0
    PALIGNR mm0, [r0+0*FDEC_STRIDE-8], 7, mm1    ; ....t3t2t1t0ltl0
    movq    mm1, mm0
    PALIGNR mm0, [r0+1*FDEC_STRIDE-8], 7, mm2    ; ..t3t2t1t0ltl0l1
    movq    mm2, mm0
    PALIGNR mm0, [r0+2*FDEC_STRIDE-8], 7, mm3    ; t3t2t1t0ltl0l1l2
    PRED8x8_LOWPASS mm3, mm1, mm0, mm2, mm4
    movq    mm1, mm3
    psrlq   mm3, 16
    psllq   mm1, 48
    movd   [r0+0*FDEC_STRIDE], mm7
    movd   [r0+1*FDEC_STRIDE], mm3
    PALIGNR mm7, mm1, 7, mm2
    psllq   mm1, 8
    movd   [r0+2*FDEC_STRIDE], mm7
    PALIGNR mm3, mm1, 7, mm1
    movd   [r0+3*FDEC_STRIDE], mm3
    RET

cglobal predict_4x4_hd_%1, 1,1
    movd      mm0, [r0-1*FDEC_STRIDE-4] ; lt ..
    punpckldq mm0, [r0-1*FDEC_STRIDE]   ; t3 t2 t1 t0 lt .. .. ..
    psllq     mm0, 8                    ; t2 t1 t0 lt .. .. .. ..
    movq      mm1, [r0+3*FDEC_STRIDE-8] ; l3
    punpckhbw mm1, [r0+2*FDEC_STRIDE-8] ; l2 l3
    movq      mm2, [r0+1*FDEC_STRIDE-8] ; l1
    punpckhbw mm2, [r0+0*FDEC_STRIDE-8] ; l0 l1
    punpckhwd mm1, mm2                  ; l0 l1 l2 l3
    punpckhdq mm1, mm0                  ; t2 t1 t0 lt l0 l1 l2 l3
    movq      mm0, mm1
    movq      mm2, mm1
    movq      mm7, mm1
    psrlq     mm0, 16                   ; .. .. t2 t1 t0 lt l0 l1
    psrlq     mm2, 8                    ; .. t2 t1 t0 lt l0 l1 l2
    pavgb     mm7, mm2
    PRED8x8_LOWPASS mm3, mm1, mm0, mm2, mm4
    punpcklbw mm7, mm3
    psrlq     mm3, 32
    PALIGNR   mm3, mm7, 6, mm6
%assign Y 3
    movd     [r0+Y*FDEC_STRIDE], mm7
%rep 2
%assign Y (Y-1)
    psrlq     mm7, 16
    movd     [r0+Y*FDEC_STRIDE], mm7
%endrep
    movd     [r0+0*FDEC_STRIDE], mm3
    RET
%endmacro

%define PALIGNR PALIGNR_MMX
PREDICT_4x4 mmxext
%define PALIGNR PALIGNR_SSSE3
PREDICT_4x4 ssse3

;-----------------------------------------------------------------------------
; void predict_4x4_hu_mmxext( uint8_t *src )
;-----------------------------------------------------------------------------
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
    PRED8x8_LOWPASS mm4, mm0, mm2, mm3, mm5
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

;-----------------------------------------------------------------------------
; void predict_4x4_vl_mmxext( uint8_t *src )
;-----------------------------------------------------------------------------
cglobal predict_4x4_vl_mmxext, 1,1
    movq        mm1, [r0-FDEC_STRIDE]
    movq        mm3, mm1
    movq        mm2, mm1
    psrlq       mm3, 8
    psrlq       mm2, 16
    movq        mm4, mm3
    pavgb       mm4, mm1

    PRED8x8_LOWPASS mm0, mm1, mm2, mm3, mm5

    movd        [r0+0*FDEC_STRIDE], mm4
    movd        [r0+1*FDEC_STRIDE], mm0
    psrlq       mm4, 8
    psrlq       mm0, 8
    movd        [r0+2*FDEC_STRIDE], mm4
    movd        [r0+3*FDEC_STRIDE], mm0

    RET

;-----------------------------------------------------------------------------
; void predict_4x4_dc( uint8_t *src )
;-----------------------------------------------------------------------------

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

%macro PREDICT_FILTER 1
;-----------------------------------------------------------------------------
;void predict_8x8_filter( uint8_t *src, uint8_t edge[33], int i_neighbor, int i_filters )
;-----------------------------------------------------------------------------

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
    PRED8x8_LOWPASS mm2, mm1, mm4, mm3, mm5
    movq     [t1+8], mm2
    movq        mm4, mm0
    PRED8x8_LOWPASS mm1, mm3, mm0, mm4, mm5
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
    PRED8x8_LOWPASS mm4, mm2, mm1, mm3, mm5
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
    PRED8x8_LOWPASS mm1, mm2, mm5, mm0, mm4
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
PREDICT_FILTER mmxext
%define PALIGNR PALIGNR_SSSE3
PREDICT_FILTER ssse3

;-----------------------------------------------------------------------------
; void predict_8x8_v_mmxext( uint8_t *src, uint8_t *edge )
;-----------------------------------------------------------------------------
cglobal predict_8x8_v_mmxext, 2,2
    movq        mm0, [r1+16]
    STORE8x8    mm0, mm0
    RET

;-----------------------------------------------------------------------------
; void predict_8x8_h_mmxext( uint8_t *src, uint8_t edge[33] )
;-----------------------------------------------------------------------------

INIT_MMX
cglobal predict_8x8_h_mmxext, 2,2
    movu   m3, [r1+7]
    mova   m7, m3
    punpckhbw m3, m3
    punpcklbw m7, m7
    pshufw m0, m3, 0xff
    pshufw m1, m3, 0xaa
    pshufw m2, m3, 0x55
    pshufw m3, m3, 0x00
    pshufw m4, m7, 0xff
    pshufw m5, m7, 0xaa
    pshufw m6, m7, 0x55
    pshufw m7, m7, 0x00
%assign n 0
%rep 8
    mova [r0+n*FDEC_STRIDE], m %+ n
%assign n n+1
%endrep
    RET

;-----------------------------------------------------------------------------
; void predict_8x8_dc_mmxext( uint8_t *src, uint8_t *edge );
;-----------------------------------------------------------------------------
cglobal predict_8x8_dc_mmxext, 2,2
    pxor        mm0, mm0
    pxor        mm1, mm1
    psadbw      mm0, [r1+7]
    psadbw      mm1, [r1+16]
    paddw       mm0, [pw_8 GLOBAL]
    paddw       mm0, mm1
    psrlw       mm0, 4
    pshufw      mm0, mm0, 0
    packuswb    mm0, mm0
    STORE8x8    mm0, mm0
    RET

;-----------------------------------------------------------------------------
; void predict_8x8_dc_top_mmxext( uint8_t *src, uint8_t *edge );
;-----------------------------------------------------------------------------
%macro PRED8x8_DC 2
cglobal %1, 2,2
    pxor        mm0, mm0
    psadbw      mm0, [r1+%2]
    paddw       mm0, [pw_4 GLOBAL]
    psrlw       mm0, 3
    pshufw      mm0, mm0, 0
    packuswb    mm0, mm0
    STORE8x8    mm0, mm0
    RET
%endmacro

PRED8x8_DC predict_8x8_dc_top_mmxext, 16
PRED8x8_DC predict_8x8_dc_left_mmxext, 7

%ifndef ARCH_X86_64
; sse2 is faster even on amd, so there's no sense in spending exe size on these
; functions if we know sse2 is available.

;-----------------------------------------------------------------------------
; void predict_8x8_ddl_mmxext( uint8_t *src, uint8_t *edge )
;-----------------------------------------------------------------------------
cglobal predict_8x8_ddl_mmxext, 2,2
    movq        mm5, [r1+16]
    movq        mm2, [r1+17]
    movq        mm3, [r1+23]
    movq        mm4, [r1+25]
    movq        mm1, mm5
    psllq       mm1, 8
    PRED8x8_LOWPASS mm0, mm1, mm2, mm5, mm7
    PRED8x8_LOWPASS mm1, mm3, mm4, [r1+24], mm6

%assign Y 7
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
; void predict_8x8_ddr_mmxext( uint8_t *src, uint8_t *edge )
;-----------------------------------------------------------------------------
cglobal predict_8x8_ddr_mmxext, 2,2
    movq        mm1, [r1+7]
    movq        mm2, [r1+9]
    movq        mm3, [r1+15]
    movq        mm4, [r1+17]
    PRED8x8_LOWPASS mm0, mm1, mm2, [r1+8], mm7
    PRED8x8_LOWPASS mm1, mm3, mm4, [r1+16], mm6

%assign Y 7
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
; void predict_8x8_hu_mmxext( uint8_t *src, uint8_t *edge )
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
    PRED8x8_LOWPASS mm1, mm3, mm5, mm2, mm6
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
; void predict_8x8_vr_core_mmxext( uint8_t *src, uint8_t *edge )
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
    PRED8x8_LOWPASS mm0, mm1, mm2, mm4, mm7

%assign Y 0
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
; void predict_8x8c_p_core_mmxext( uint8_t *src, int i00, int b, int c )
;-----------------------------------------------------------------------------
cglobal predict_8x8c_p_core_mmxext, 1,2
    LOAD_PLANE_ARGS
    movq        mm1, mm2
    pmullw      mm2, [pw_3210 GLOBAL]
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
; void predict_16x16_p_core_mmxext( uint8_t *src, int i00, int b, int c )
;-----------------------------------------------------------------------------
cglobal predict_16x16_p_core_mmxext, 1,2
    LOAD_PLANE_ARGS
    movq        mm5, mm2
    movq        mm1, mm2
    pmullw      mm5, [pw_3210 GLOBAL]
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

;-----------------------------------------------------------------------------
; void predict_8x8_ddl_sse2( uint8_t *src, uint8_t *edge )
;-----------------------------------------------------------------------------
cglobal predict_8x8_ddl_sse2, 2,2
    movdqa      xmm3, [r1+16]
    movdqu      xmm2, [r1+17]
    movdqa      xmm1, xmm3
    pslldq      xmm1, 1
    PRED8x8_LOWPASS_XMM xmm0, xmm1, xmm2, xmm3, xmm4

%assign Y 0
%rep 8
    psrldq      xmm0, 1
    movq        [r0+Y*FDEC_STRIDE], xmm0
%assign Y (Y+1)
%endrep
    RET

;-----------------------------------------------------------------------------
; void predict_8x8_ddr_sse2( uint8_t *src, uint8_t *edge )
;-----------------------------------------------------------------------------
cglobal predict_8x8_ddr_sse2, 2,2
    movdqu      xmm3, [r1+8]
    movdqu      xmm1, [r1+7]
    movdqa      xmm2, xmm3
    psrldq      xmm2, 1
    PRED8x8_LOWPASS_XMM xmm0, xmm1, xmm2, xmm3, xmm4

    movdqa      xmm1, xmm0
    psrldq      xmm1, 1
%assign Y 7
%rep 3
    movq        [r0+Y*FDEC_STRIDE], xmm0
    movq        [r0+(Y-1)*FDEC_STRIDE], xmm1
    psrldq      xmm0, 2
    psrldq      xmm1, 2
%assign Y (Y-2)
%endrep
    movq        [r0+1*FDEC_STRIDE], xmm0
    movq        [r0+0*FDEC_STRIDE], xmm1

    RET

;-----------------------------------------------------------------------------
; void predict_8x8_vl_sse2( uint8_t *src, uint8_t *edge )
;-----------------------------------------------------------------------------
cglobal predict_8x8_vl_sse2, 2,2
    movdqa      xmm4, [r1+16]
    movdqa      xmm2, xmm4
    movdqa      xmm1, xmm4
    movdqa      xmm3, xmm4
    psrldq      xmm2, 1
    pslldq      xmm1, 1
    pavgb       xmm3, xmm2
    PRED8x8_LOWPASS_XMM xmm0, xmm1, xmm2, xmm4, xmm5
; xmm0: (t0 + 2*t1 + t2 + 2) >> 2
; xmm3: (t0 + t1 + 1) >> 1

%assign Y 0
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
; void predict_8x8_vr_sse2( uint8_t *src, uint8_t *edge )
;-----------------------------------------------------------------------------
cglobal predict_8x8_vr_sse2, 2,2,7
    movdqu      xmm0, [r1+8]
    movdqa      xmm6, [pw_ff00 GLOBAL]
    add         r0, 4*FDEC_STRIDE
    movdqa      xmm1, xmm0
    movdqa      xmm2, xmm0
    movdqa      xmm3, xmm0
    pslldq      xmm0, 1
    pslldq      xmm1, 2
    pavgb       xmm2, xmm0
    PRED8x8_LOWPASS_XMM xmm4, xmm3, xmm1, xmm0, xmm5
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

;-----------------------------------------------------------------------------
; void predict_8x8_hd_mmxext( uint8_t *src, uint8_t *edge )
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
    PRED8x8_LOWPASS mm0, mm4, mm1, mm5, mm7
    movq    mm4, mm2
    movq    mm1, mm2            ; t6 t5 t4 t3 t2 t1 t0 lt
    psrlq   mm4, 16             ; .. .. t6 t5 t4 t3 t2 t1
    psrlq   mm1, 8              ; .. t6 t5 t4 t3 t2 t1 t0
    PRED8x8_LOWPASS mm6, mm4, mm2, mm1, mm5
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
; void predict_8x8_hd_ssse3( uint8_t *src, uint8_t *edge )
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
    PRED8x8_LOWPASS_XMM xmm0, xmm1, xmm2, xmm3, xmm5
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
; void predict_8x8_hu_sse2( uint8_t *src, uint8_t *edge )
;-----------------------------------------------------------------------------
%macro PREDICT_8x8_HU 1
cglobal predict_8x8_hu_%1, 2,2
    add        r0, 4*FDEC_STRIDE
%ifidn %1, ssse3
    movq      mm5, [r1+7]
    movq      mm6, [pb_reverse GLOBAL]
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
    PRED8x8_LOWPASS mm1, mm3, mm5, mm2, mm6

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
; void predict_8x8c_v_mmx( uint8_t *src )
;-----------------------------------------------------------------------------
cglobal predict_8x8c_v_mmx, 1,1
    movq        mm0, [r0 - FDEC_STRIDE]
    STORE8x8    mm0, mm0
    RET

;-----------------------------------------------------------------------------
; void predict_8x8c_h_mmxext( uint8_t *src )
;-----------------------------------------------------------------------------

%macro PRED_8x8C_H 1
cglobal predict_8x8c_h_%1, 1,1
%ifidn %1, ssse3
    mova   m1, [pb_3 GLOBAL]
%endif
%assign n 0
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
; void predict_8x8c_dc_core_mmxext( uint8_t *src, int s2, int s3 )
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
    paddw       mm1, [pw_2 GLOBAL]
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
; void predict_8x8c_p_core_sse2( uint8_t *src, int i00, int b, int c )
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
    pmullw      xmm2, [pw_76543210 GLOBAL]
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
; void predict_16x16_p_core_sse2( uint8_t *src, int i00, int b, int c )
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
    pmullw      xmm3, [pw_76543210 GLOBAL]
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
; void predict_16x16_v_mmx( uint8_t *src )
;-----------------------------------------------------------------------------
cglobal predict_16x16_v_mmx, 1,2
    movq        mm0, [r0 - FDEC_STRIDE]
    movq        mm1, [r0 - FDEC_STRIDE + 8]
    STORE16x16  mm0, mm1
    REP_RET

;-----------------------------------------------------------------------------
; void predict_16x16_v_sse2( uint8_t *src )
;-----------------------------------------------------------------------------
cglobal predict_16x16_v_sse2, 1,1
    movdqa      xmm0, [r0 - FDEC_STRIDE]
    STORE16x16_SSE2 xmm0
    RET

;-----------------------------------------------------------------------------
; void predict_16x16_h_mmxext( uint8_t *src )
;-----------------------------------------------------------------------------

%macro PRED_16x16_H 1
cglobal predict_16x16_h_%1, 1,2
    mov r1, FDEC_STRIDE*12
%ifidn %1, ssse3
    mova   m1, [pb_3 GLOBAL]
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
    add r1, -FDEC_STRIDE*4
    jge .vloop
    REP_RET
%endmacro

;no SSE2, its slower than MMX on all systems that don't support SSSE3
INIT_MMX
%define SPLATB SPLATB_MMX
PRED_16x16_H mmxext
INIT_XMM
%define SPLATB SPLATB_SSSE3
PRED_16x16_H ssse3

;-----------------------------------------------------------------------------
; void predict_16x16_dc_core_mmxext( uint8_t *src, int i_dc_left )
;-----------------------------------------------------------------------------

%macro PRED16x16_DC 2
    pxor        mm0, mm0
    pxor        mm1, mm1
    psadbw      mm0, [r0 - FDEC_STRIDE]
    psadbw      mm1, [r0 - FDEC_STRIDE + 8]
    paddusw     mm0, mm1
    paddusw     mm0, %1
    psrlw       mm0, %2                       ; dc
    pshufw      mm0, mm0, 0
    packuswb    mm0, mm0                      ; dc in bytes
    STORE16x16  mm0, mm0
%endmacro

cglobal predict_16x16_dc_core_mmxext, 1,2
%ifdef ARCH_X86_64
    movd         mm2, r1d
    PRED16x16_DC mm2, 5
%else
    PRED16x16_DC r1m, 5
%endif
    REP_RET

cglobal predict_16x16_dc_top_mmxext, 1,2
    PRED16x16_DC [pw_8 GLOBAL], 4
    REP_RET

cglobal predict_16x16_dc_left_core_mmxext, 1,1
    movd       mm0, r1m
    pshufw     mm0, mm0, 0
    packuswb   mm0, mm0
    STORE16x16 mm0, mm0
    REP_RET

;-----------------------------------------------------------------------------
; void predict_16x16_dc_core_sse2( uint8_t *src, int i_dc_left )
;-----------------------------------------------------------------------------

%macro PRED16x16_DC_SSE2 2
    pxor        xmm0, xmm0
    psadbw      xmm0, [r0 - FDEC_STRIDE]
    movhlps     xmm1, xmm0
    paddw       xmm0, xmm1
    paddusw     xmm0, %1
    psrlw       xmm0, %2                ; dc
    pshuflw     xmm0, xmm0, 0
    punpcklqdq  xmm0, xmm0
    packuswb    xmm0, xmm0              ; dc in bytes
    STORE16x16_SSE2 xmm0
%endmacro

cglobal predict_16x16_dc_core_sse2, 1,1
    movd xmm2, r1m
    PRED16x16_DC_SSE2 xmm2, 5
    RET

cglobal predict_16x16_dc_top_sse2, 1,1
    PRED16x16_DC_SSE2 [pw_8 GLOBAL], 4
    RET

cglobal predict_16x16_dc_left_core_sse2, 1,1
    movd       xmm0, r1m
    pshuflw    xmm0, xmm0, 0
    punpcklqdq xmm0, xmm0
    packuswb   xmm0, xmm0
    STORE16x16_SSE2 xmm0
    RET

;*****************************************************************************
;* predict-a.asm: h264 encoder library
;*****************************************************************************
;* Copyright (C) 2005 x264 project
;*
;* Authors: Loren Merritt <lorenm@u.washington.edu>
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

%macro STORE8x8 2
    movq        [parm1q + 1*FDEC_STRIDE], %1
    movq        [parm1q + 2*FDEC_STRIDE], %1
    movq        [parm1q + 3*FDEC_STRIDE], %1
    movq        [parm1q + 4*FDEC_STRIDE], %1
    movq        [parm1q + 5*FDEC_STRIDE], %2
    movq        [parm1q + 6*FDEC_STRIDE], %2
    movq        [parm1q + 7*FDEC_STRIDE], %2
    movq        [parm1q + 8*FDEC_STRIDE], %2
%endmacro

%macro STORE16x16 2
    mov         eax, 4
ALIGN 4
.loop:
    movq        [parm1q + 1*FDEC_STRIDE], %1
    movq        [parm1q + 2*FDEC_STRIDE], %1
    movq        [parm1q + 3*FDEC_STRIDE], %1
    movq        [parm1q + 4*FDEC_STRIDE], %1
    movq        [parm1q + 1*FDEC_STRIDE + 8], %2
    movq        [parm1q + 2*FDEC_STRIDE + 8], %2
    movq        [parm1q + 3*FDEC_STRIDE + 8], %2
    movq        [parm1q + 4*FDEC_STRIDE + 8], %2
    dec         eax
    lea         parm1q, [parm1q + 4*FDEC_STRIDE]
    jnz         .loop
    nop
%endmacro


SECTION .rodata align=16

ALIGN 8
pw_2: times 4 dw 2
pw_8: times 4 dw 8
pb_1: times 8 db 1
pw_3210:
    dw 0
    dw 1
    dw 2
    dw 3

;=============================================================================
; Code
;=============================================================================

SECTION .text

cglobal predict_8x8_v_mmxext
cglobal predict_8x8_dc_core_mmxext
cglobal predict_8x8c_v_mmx
cglobal predict_8x8c_dc_core_mmxext
cglobal predict_8x8c_p_core_mmx
cglobal predict_16x16_p_core_mmx
cglobal predict_16x16_v_mmx
cglobal predict_16x16_dc_core_mmxext
cglobal predict_16x16_dc_top_mmxext



%macro PRED8x8_LOWPASS 2
    movq        mm3, mm1
    pavgb       mm1, mm2
    pxor        mm2, mm3
    movq        %1 , %2
    pand        mm2, [pb_1 GLOBAL]
    psubusb     mm1, mm2
    pavgb       %1 , mm1     ; %1 = (t[n-1] + t[n]*2 + t[n+1] + 2) >> 2
%endmacro

%macro PRED8x8_LOAD_TOP 0
    sub         parm1q, FDEC_STRIDE

    and         parm2d, 12
    movq        mm1, [parm1q-1]
    movq        mm2, [parm1q+1]

    cmp         parm2d, byte 8
    jge         .have_topleft
    mov         al,  [parm1q]
    mov         ah,  [parm1q]
    pinsrw      mm1, eax, 0
.have_topleft:

    and         parm2d, byte 4
    jne         .have_topright
    mov         al,  [parm1q+7]
    mov         ah,  [parm1q+7]
    pinsrw      mm2, eax, 3
.have_topright:

    PRED8x8_LOWPASS mm0, [parm1q]
%endmacro

;-----------------------------------------------------------------------------
;
; void predict_8x8_v_mmxext( uint8_t *src, int i_neighbors )
;
;-----------------------------------------------------------------------------

ALIGN 16
predict_8x8_v_mmxext:
    PRED8x8_LOAD_TOP
    STORE8x8    mm0, mm0
    ret

;-----------------------------------------------------------------------------
;
; void predict_8x8_dc_core_mmxext( uint8_t *src, int i_neighbors, uint8_t *pix_left );
;
;-----------------------------------------------------------------------------

ALIGN 16
predict_8x8_dc_core_mmxext:
    movq        mm1, [parm3q-1]
    movq        mm2, [parm3q+1]
    PRED8x8_LOWPASS mm4, [parm3q]

    PRED8x8_LOAD_TOP

    pxor        mm1, mm1
    psadbw      mm0, mm1
    psadbw      mm4, mm1
    paddw       mm0, [pw_8 GLOBAL]
    paddw       mm0, mm4
    psrlw       mm0, 4
    pshufw      mm0, mm0, 0
    packuswb    mm0, mm0

    STORE8x8    mm0, mm0
    ret

;-----------------------------------------------------------------------------
;
; void predict_8x8c_v_mmx( uint8_t *src )
;
;-----------------------------------------------------------------------------

ALIGN 16
predict_8x8c_v_mmx :
    sub         parm1q, FDEC_STRIDE
    movq        mm0, [parm1q]
    STORE8x8    mm0, mm0
    ret

;-----------------------------------------------------------------------------
;
; void predict_8x8c_dc_core_mmxext( uint8_t *src, int s2, int s3 )
;
;-----------------------------------------------------------------------------

ALIGN 16
predict_8x8c_dc_core_mmxext:
    sub         parm1q, FDEC_STRIDE

    movq        mm0, [parm1q]
    pxor        mm1, mm1
    pxor        mm2, mm2
    punpckhbw   mm1, mm0
    punpcklbw   mm0, mm2
    psadbw      mm1, mm2        ; s1
    psadbw      mm0, mm2        ; s0

    movd        mm4, parm2d
    movd        mm5, parm3d
    paddw       mm0, mm4
    pshufw      mm2, mm5, 0
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
    ret

;-----------------------------------------------------------------------------
;
; void predict_8x8c_p_core_mmx( uint8_t *src, int i00, int b, int c )
;
;-----------------------------------------------------------------------------

ALIGN 16
predict_8x8c_p_core_mmx:
    movd        mm0, parm2d
    movd        mm2, parm3d
    movd        mm4, parm4d
    pshufw      mm0, mm0, 0
    pshufw      mm2, mm2, 0
    pshufw      mm4, mm4, 0
    movq        mm1, mm2
    pmullw      mm2, [pw_3210 GLOBAL]
    psllw       mm1, 2
    paddsw      mm0, mm2        ; mm0 = {i+0*b, i+1*b, i+2*b, i+3*b}
    paddsw      mm1, mm0        ; mm1 = {i+4*b, i+5*b, i+6*b, i+7*b}
    pxor        mm5, mm5

    mov         eax, 8
ALIGN 4
.loop:
    movq        mm6, mm0
    movq        mm7, mm1
    psraw       mm6, 5
    psraw       mm7, 5
    pmaxsw      mm6, mm5
    pmaxsw      mm7, mm5
    packuswb    mm6, mm7
    movq        [parm1q], mm6

    paddsw      mm0, mm4
    paddsw      mm1, mm4
    add         parm1q, FDEC_STRIDE
    dec         eax
    jg          .loop

    nop
    ret

;-----------------------------------------------------------------------------
;
; void predict_16x16_p_core_mmx( uint8_t *src, int i00, int b, int c )
;
;-----------------------------------------------------------------------------

ALIGN 16
predict_16x16_p_core_mmx:
    movd        mm0, parm2d
    movd        mm2, parm3d
    movd        mm4, parm4d
    pshufw      mm0, mm0, 0
    pshufw      mm2, mm2, 0
    pshufw      mm4, mm4, 0
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
    pxor        mm5, mm5

    mov         eax, 16
ALIGN 4
.loop:
    movq        mm6, mm0
    movq        mm7, mm1
    psraw       mm6, 5
    psraw       mm7, 5
    pmaxsw      mm6, mm5
    pmaxsw      mm7, mm5
    packuswb    mm6, mm7
    movq        [parm1q], mm6

    movq        mm6, mm2
    movq        mm7, mm3
    psraw       mm6, 5
    psraw       mm7, 5
    pmaxsw      mm6, mm5
    pmaxsw      mm7, mm5
    packuswb    mm6, mm7
    movq        [parm1q+8], mm6

    paddsw      mm0, mm4
    paddsw      mm1, mm4
    paddsw      mm2, mm4
    paddsw      mm3, mm4
    add         parm1q, FDEC_STRIDE
    dec         eax
    jg          .loop

    nop
    ret
    
;-----------------------------------------------------------------------------
;
; void predict_16x16_v_mmx( uint8_t *src )
;
;-----------------------------------------------------------------------------

ALIGN 16
predict_16x16_v_mmx :
    sub         parm1q, FDEC_STRIDE
    movq        mm0, [parm1q]
    movq        mm1, [parm1q + 8]
    STORE16x16  mm0, mm1
    ret

;-----------------------------------------------------------------------------
;
; void predict_16x16_dc_core_mmxext( uint8_t *src, int i_dc_left )
;
;-----------------------------------------------------------------------------

%macro PRED16x16_DC 2
    sub         parm1q, FDEC_STRIDE

    pxor        mm0, mm0
    pxor        mm1, mm1
    psadbw      mm0, [parm1q]
    psadbw      mm1, [parm1q + 8]
    paddusw     mm0, mm1
    paddusw     mm0, %1
    psrlw       mm0, %2                       ; dc
    pshufw      mm0, mm0, 0
    packuswb    mm0, mm0                      ; dc in bytes

    STORE16x16  mm0, mm0
%endmacro

ALIGN 16
predict_16x16_dc_core_mmxext:
    movd         mm2, parm2d
    PRED16x16_DC mm2, 5
    ret

ALIGN 16
predict_16x16_dc_top_mmxext:
    PRED16x16_DC [pw_8 GLOBAL], 4
    ret


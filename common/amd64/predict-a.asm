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
    movq        [parm1q + 0*FDEC_STRIDE], %1
    movq        [parm1q + 1*FDEC_STRIDE], %1
    movq        [parm1q + 2*FDEC_STRIDE], %1
    movq        [parm1q + 3*FDEC_STRIDE], %1
    movq        [parm1q + 4*FDEC_STRIDE], %2
    movq        [parm1q + 5*FDEC_STRIDE], %2
    movq        [parm1q + 6*FDEC_STRIDE], %2
    movq        [parm1q + 7*FDEC_STRIDE], %2
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


SECTION_RODATA

pw_2: times 4 dw 2
pw_4: times 4 dw 4
pw_8: times 4 dw 8
pw_3210:
    dw 0
    dw 1
    dw 2
    dw 3
ALIGN 16
pb_1: times 16 db 1
pb_00s_ff:
    times 8 db 0
pb_0s_ff:
    times 7 db 0
    db 0xff

;=============================================================================
; Code
;=============================================================================

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


;-----------------------------------------------------------------------------
; void predict_4x4_ddl_mmxext( uint8_t *src )
;-----------------------------------------------------------------------------
cglobal predict_4x4_ddl_mmxext
    sub         parm1q, FDEC_STRIDE
    movq        mm3, [parm1q]
    movq        mm1, [parm1q-1]
    movq        mm2, mm3
    movq        mm4, [pb_0s_ff GLOBAL]
    psrlq       mm2, 8
    pand        mm4, mm3
    por         mm2, mm4

    PRED8x8_LOWPASS mm0, mm1, mm2, mm3, mm5

%assign Y 1
%rep 4
    psrlq       mm0, 8
    movd        [parm1q+Y*FDEC_STRIDE], mm0
%assign Y (Y+1)
%endrep

    ret

;-----------------------------------------------------------------------------
; void predict_4x4_vl_mmxext( uint8_t *src )
;-----------------------------------------------------------------------------
cglobal predict_4x4_vl_mmxext
    movq        mm1, [parm1q-FDEC_STRIDE]
    movq        mm3, mm1
    movq        mm2, mm1
    psrlq       mm3, 8
    psrlq       mm2, 16
    movq        mm4, mm3
    pavgb       mm4, mm1

    PRED8x8_LOWPASS mm0, mm1, mm2, mm3, mm5

    movd        [parm1q+0*FDEC_STRIDE], mm4
    movd        [parm1q+1*FDEC_STRIDE], mm0
    psrlq       mm4, 8
    psrlq       mm0, 8
    movd        [parm1q+2*FDEC_STRIDE], mm4
    movd        [parm1q+3*FDEC_STRIDE], mm0

    ret

;-----------------------------------------------------------------------------
; void predict_8x8_v_mmxext( uint8_t *src, uint8_t *edge )
;-----------------------------------------------------------------------------
cglobal predict_8x8_v_mmxext
    movq        mm0, [parm2q+16]
    STORE8x8    mm0, mm0
    ret

;-----------------------------------------------------------------------------
; void predict_8x8_dc_mmxext( uint8_t *src, uint8_t *edge );
;-----------------------------------------------------------------------------
cglobal predict_8x8_dc_mmxext
    pxor        mm0, mm0
    pxor        mm1, mm1
    psadbw      mm0, [parm2q+7]
    psadbw      mm1, [parm2q+16]
    paddw       mm0, [pw_8 GLOBAL]
    paddw       mm0, mm1
    psrlw       mm0, 4
    pshufw      mm0, mm0, 0
    packuswb    mm0, mm0
    STORE8x8    mm0, mm0
    ret

;-----------------------------------------------------------------------------
; void predict_8x8_dc_top_mmxext( uint8_t *src, uint8_t *edge );
;-----------------------------------------------------------------------------
cglobal predict_8x8_dc_top_mmxext
    pxor        mm0, mm0
    psadbw      mm0, [parm2q+16]
    paddw       mm0, [pw_4 GLOBAL]
    psrlw       mm0, 3
    pshufw      mm0, mm0, 0
    packuswb    mm0, mm0
    STORE8x8    mm0, mm0
    ret

;-----------------------------------------------------------------------------
; void predict_8x8_dc_left_mmxext( uint8_t *src, uint8_t *edge );
;-----------------------------------------------------------------------------
cglobal predict_8x8_dc_left_mmxext
    pxor        mm0, mm0
    psadbw      mm0, [parm2q+7]
    paddw       mm0, [pw_4 GLOBAL]
    psrlw       mm0, 3
    pshufw      mm0, mm0, 0
    packuswb    mm0, mm0
    STORE8x8    mm0, mm0
    ret

;-----------------------------------------------------------------------------
; void predict_8x8_ddl_mmxext( uint8_t *src, uint8_t *edge )
;-----------------------------------------------------------------------------
cglobal predict_8x8_ddl_mmxext
    movq        mm5, [parm2q+16]
    movq        mm2, [parm2q+17]
    movq        mm3, [parm2q+23]
    movq        mm4, [parm2q+25]
    movq        mm1, mm5
    psllq       mm1, 8
    PRED8x8_LOWPASS mm0, mm1, mm2, mm5, mm7
    PRED8x8_LOWPASS mm1, mm3, mm4, [parm2q+24], mm6

%assign Y 7
%rep 6
    movq        [parm1q+Y*FDEC_STRIDE], mm1
    movq        mm2, mm0
    psllq       mm1, 8
    psrlq       mm2, 56
    psllq       mm0, 8
    por         mm1, mm2
%assign Y (Y-1)
%endrep
    movq        [parm1q+Y*FDEC_STRIDE], mm1
    psllq       mm1, 8
    psrlq       mm0, 56
    por         mm1, mm0
%assign Y (Y-1)
    movq        [parm1q+Y*FDEC_STRIDE], mm1

    ret

;-----------------------------------------------------------------------------
; void predict_8x8_ddl_sse2( uint8_t *src, uint8_t *edge )
;-----------------------------------------------------------------------------
cglobal predict_8x8_ddl_sse2
    movdqa      xmm3, [parm2q+16]
    movdqu      xmm2, [parm2q+17]
    movdqa      xmm1, xmm3
    pslldq      xmm1, 1
    PRED8x8_LOWPASS_XMM xmm0, xmm1, xmm2, xmm3, xmm4

%assign Y 0
%rep 8
    psrldq      xmm0, 1
    movq        [parm1q+Y*FDEC_STRIDE], xmm0
%assign Y (Y+1)
%endrep
    ret

;-----------------------------------------------------------------------------
; void predict_8x8_ddr_sse2( uint8_t *src, uint8_t *edge )
;-----------------------------------------------------------------------------
cglobal predict_8x8_ddr_sse2
    movdqu      xmm3, [parm2q+8]
    movdqu      xmm1, [parm2q+7]
    movdqa      xmm2, xmm3
    psrldq      xmm2, 1
    PRED8x8_LOWPASS_XMM xmm0, xmm1, xmm2, xmm3, xmm4

    movdqa      xmm1, xmm0
    psrldq      xmm1, 1
%assign Y 7
%rep 3
    movq        [parm1q+Y*FDEC_STRIDE], xmm0
    movq        [parm1q+(Y-1)*FDEC_STRIDE], xmm1
    psrldq      xmm0, 2
    psrldq      xmm1, 2
%assign Y (Y-2)
%endrep
    movq        [parm1q+1*FDEC_STRIDE], xmm0
    movq        [parm1q+0*FDEC_STRIDE], xmm1

    ret

;-----------------------------------------------------------------------------
; void predict_8x8_vl_sse2( uint8_t *src, uint8_t *edge )
;-----------------------------------------------------------------------------
cglobal predict_8x8_vl_sse2
    movdqa      xmm4, [parm2q+16]
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
    movq        [parm1q+ Y   *FDEC_STRIDE], xmm3
    movq        [parm1q+(Y+1)*FDEC_STRIDE], xmm0
    psrldq      xmm3, 1
%assign Y (Y+2)
%endrep
    psrldq      xmm0, 1
    movq        [parm1q+ Y   *FDEC_STRIDE], xmm3
    movq        [parm1q+(Y+1)*FDEC_STRIDE], xmm0

    ret

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

cglobal predict_8x8_vr_core_mmxext
    movq        mm2, [parm2q+16]
    movq        mm3, [parm2q+15]
    movq        mm1, [parm2q+14]
    movq        mm4, mm3
    pavgb       mm3, mm2
    PRED8x8_LOWPASS mm0, mm1, mm2, mm4, mm7

%assign Y 0
%rep 3
    movq        [parm1q+ Y   *FDEC_STRIDE], mm3
    movq        [parm1q+(Y+1)*FDEC_STRIDE], mm0
    psllq       mm3, 8
    psllq       mm0, 8
%assign Y (Y+2)
%endrep
    movq        [parm1q+ Y   *FDEC_STRIDE], mm3
    movq        [parm1q+(Y+1)*FDEC_STRIDE], mm0

    ret

;-----------------------------------------------------------------------------
; void predict_8x8c_v_mmx( uint8_t *src )
;-----------------------------------------------------------------------------
cglobal predict_8x8c_v_mmx
    movq        mm0, [parm1q - FDEC_STRIDE]
    STORE8x8    mm0, mm0
    ret

;-----------------------------------------------------------------------------
; void predict_8x8c_dc_core_mmxext( uint8_t *src, int s2, int s3 )
;-----------------------------------------------------------------------------
cglobal predict_8x8c_dc_core_mmxext
    movq        mm0, [parm1q - FDEC_STRIDE]
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
; void predict_8x8c_p_core_mmxext( uint8_t *src, int i00, int b, int c )
;-----------------------------------------------------------------------------
cglobal predict_8x8c_p_core_mmxext
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

    mov         eax, 8
ALIGN 4
.loop:
    movq        mm5, mm0
    movq        mm6, mm1
    psraw       mm5, 5
    psraw       mm6, 5
    packuswb    mm5, mm6
    movq        [parm1q], mm5

    paddsw      mm0, mm4
    paddsw      mm1, mm4
    add         parm1q, FDEC_STRIDE
    dec         eax
    jg          .loop

    nop
    ret

;-----------------------------------------------------------------------------
; void predict_16x16_p_core_mmxext( uint8_t *src, int i00, int b, int c )
;-----------------------------------------------------------------------------
cglobal predict_16x16_p_core_mmxext
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

    mov         eax, 16
ALIGN 4
.loop:
    movq        mm5, mm0
    movq        mm6, mm1
    psraw       mm5, 5
    psraw       mm6, 5
    packuswb    mm5, mm6
    movq        [parm1q], mm5

    movq        mm5, mm2
    movq        mm6, mm3
    psraw       mm5, 5
    psraw       mm6, 5
    packuswb    mm5, mm6
    movq        [parm1q+8], mm5

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
; void predict_16x16_v_mmx( uint8_t *src )
;-----------------------------------------------------------------------------
cglobal predict_16x16_v_mmx
    sub         parm1q, FDEC_STRIDE
    movq        mm0, [parm1q]
    movq        mm1, [parm1q + 8]
    STORE16x16  mm0, mm1
    ret

;-----------------------------------------------------------------------------
; void predict_16x16_dc_core_mmxext( uint8_t *src, int i_dc_left )
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

cglobal predict_16x16_dc_core_mmxext
    movd         mm2, parm2d
    PRED16x16_DC mm2, 5
    ret

cglobal predict_16x16_dc_top_mmxext
    PRED16x16_DC [pw_8 GLOBAL], 4
    ret


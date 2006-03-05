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

ALIGN 16
pw_2: times 4 dw 2
pw_8: times 4 dw 8
pb_1: times 16 db 1
pw_3210:
    dw 0
    dw 1
    dw 2
    dw 3
ALIGN 16
pb_00s_ff:
    times 8 db 0
pb_0s_ff:
    times 7 db 0
    db 0xff

;=============================================================================
; Code
;=============================================================================

SECTION .text

cglobal predict_4x4_ddl_mmxext
cglobal predict_4x4_vl_mmxext
cglobal predict_8x8_v_mmxext
cglobal predict_8x8_ddl_mmxext
cglobal predict_8x8_ddl_sse2
cglobal predict_8x8_ddr_sse2
cglobal predict_8x8_vl_sse2
cglobal predict_8x8_vr_core_mmxext
cglobal predict_8x8_dc_core_mmxext
cglobal predict_8x8c_v_mmx
cglobal predict_8x8c_dc_core_mmxext
cglobal predict_8x8c_p_core_mmxext
cglobal predict_16x16_p_core_mmxext
cglobal predict_16x16_v_mmx
cglobal predict_16x16_dc_core_mmxext
cglobal predict_16x16_dc_top_mmxext


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

; output: mm0 = filtered t0..t7
%macro PRED8x8_LOAD_TOP_FILT 0
    sub         parm1q, FDEC_STRIDE

    and         parm2d, 12
    movq        mm1, [parm1q-1]
    movq        mm2, [parm1q+1]

    cmp         parm2d, byte 8
    jge         .have_topleft
    mov         al,  [parm1q]
    mov         ah,  al
    pinsrw      mm1, eax, 0
.have_topleft:

    and         parm2d, byte 4
    jne         .have_topright
    mov         al,  [parm1q+7]
    mov         ah,  al
    pinsrw      mm2, eax, 3
.have_topright:

    PRED8x8_LOWPASS mm0, mm1, mm2, [parm1q], mm7
%endmacro

; output: xmm0 = unfiltered t0..t15
;         xmm1 = unfiltered t1..t15
;         xmm2 = unfiltered tl..t14
%macro PRED8x8_LOAD_TOP_TOPRIGHT_XMM 0
    sub         parm1q, FDEC_STRIDE

    and         parm2d, 12
    movdqu      xmm1, [parm1q-1]

    cmp         parm2d, byte 8
    jge         .have_topleft
    mov         al,   [parm1q]
    mov         ah,   al
    pinsrw      xmm1, eax, 0

.have_topleft:
    and         parm2d, byte 4
    jne         .have_topright

    mov         al,   [parm1q+7]
    mov         ah,   al
    pinsrw      xmm1, eax, 4
    pshufhw     xmm1, xmm1, 0
    movdqa      xmm0, xmm1
    movdqa      xmm2, xmm1
    psrldq      xmm0, 1
    psrldq      xmm2, 2
    pshufhw     xmm0, xmm0, 0
    pshufhw     xmm2, xmm2, 0
    jmp         .done_topright

.have_topright:
    movdqu      xmm0, [parm1q]
    movdqa      xmm2, xmm0
    psrldq      xmm2, 1
    mov         al,   [parm1q+15]
    mov         ah,   al
    pinsrw      xmm2, eax, 7
.done_topright:
%endmacro

;-----------------------------------------------------------------------------
;
; void predict_4x4_ddl_mmxext( uint8_t *src )
;
;-----------------------------------------------------------------------------

ALIGN 16
predict_4x4_ddl_mmxext:
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
;
; void predict_4x4_vl_mmxext( uint8_t *src )
;
;-----------------------------------------------------------------------------

ALIGN 16
predict_4x4_vl_mmxext:
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
;
; void predict_8x8_v_mmxext( uint8_t *src, int i_neighbors )
;
;-----------------------------------------------------------------------------

ALIGN 16
predict_8x8_v_mmxext:
    PRED8x8_LOAD_TOP_FILT
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
    PRED8x8_LOWPASS mm4, mm1, mm2, [parm3q], mm7

    PRED8x8_LOAD_TOP_FILT

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
; void predict_8x8_ddl_mmxext( uint8_t *src, int i_neighbors )
;
;-----------------------------------------------------------------------------

ALIGN 16
predict_8x8_ddl_mmxext:
    sub         parm1q, FDEC_STRIDE

    and         parm2d, 12
    movq        mm1, [parm1q-1]
    movq        mm2, [parm1q+1]

    cmp         parm2d, byte 8
    jge         .have_topleft
    mov         al,  [parm1q]
    mov         ah,  al
    pinsrw      mm1, eax, 0

.have_topleft:
    and         parm2d, byte 4
    jne         .have_topright

    mov         al,  [parm1q+7]
    mov         ah,  [parm1q+7]
    pinsrw      mm2, eax, 3
    pshufw      mm3, mm2, 0xff
    jmp         .done_topright

.have_topright:
    movq        mm5, [parm1q+9];
    mov         al,  [parm1q+15]
    mov         ah,  al
    pinsrw      mm5, eax, 3
    movq        mm4, [parm1q+7];
    PRED8x8_LOWPASS mm3, mm4, mm5, [parm1q+8], mm7
.done_topright:

;?0123456789abcdeff
; [-mm0--][-mm3--]
;[-mm1--][-mm4--]
;  [-mm2--][-mm5--]

    PRED8x8_LOWPASS mm0, mm1, mm2, [parm1q], mm7
    movq        mm1, mm0
    movq        mm2, mm0
    psllq       mm1, 8
    psrlq       mm2, 8
    movq        mm6, mm3
    movq        mm4, mm3
    psllq       mm6, 56
    movq        mm7, mm0
    por         mm2, mm6
    psllq       mm4, 8
    movq        mm5, mm3
    movq        mm6, mm3
    psrlq       mm5, 8
    pand        mm6, [pb_0s_ff GLOBAL]
    psrlq       mm7, 56
    por         mm5, mm6
    por         mm4, mm7
    PRED8x8_LOWPASS mm6, mm1, mm2, mm0, mm7
    PRED8x8_LOWPASS mm7, mm4, mm5, mm3, mm2

%assign Y 8
%rep 6
    movq        [parm1q+Y*FDEC_STRIDE], mm7
    movq        mm1, mm6
    psllq       mm7, 8
    psrlq       mm1, 56
    psllq       mm6, 8
    por         mm7, mm1
%assign Y (Y-1)
%endrep
    movq        [parm1q+Y*FDEC_STRIDE], mm7
    psllq       mm7, 8
    psrlq       mm6, 56
    por         mm7, mm6
%assign Y (Y-1)
    movq        [parm1q+Y*FDEC_STRIDE], mm7

    ret

;-----------------------------------------------------------------------------
;
; void predict_8x8_ddl_sse2( uint8_t *src, int i_neighbors )
;
;-----------------------------------------------------------------------------

ALIGN 16
predict_8x8_ddl_sse2:
    PRED8x8_LOAD_TOP_TOPRIGHT_XMM

;?0123456789abcdeff
; [-----xmm0-----]
;[-----xmm1-----]
;  [-----xmm2-----]

    movdqa      xmm3, [pb_00s_ff GLOBAL]
    PRED8x8_LOWPASS_XMM xmm4, xmm1, xmm2, xmm0, xmm5
    movdqa      xmm1, xmm4
    movdqa      xmm2, xmm4
    pand        xmm3, xmm4
    psrldq      xmm2, 1
    pslldq      xmm1, 1
    por         xmm2, xmm3
    PRED8x8_LOWPASS_XMM xmm0, xmm1, xmm2, xmm4, xmm5

%assign Y 1
%rep 8
    psrldq      xmm0, 1
    movq        [parm1q+Y*FDEC_STRIDE], xmm0
%assign Y (Y+1)
%endrep

    ret

;-----------------------------------------------------------------------------
;
; void predict_8x8_ddr_sse2( uint8_t *src, int i_neighbors )
;
;-----------------------------------------------------------------------------

ALIGN 16
predict_8x8_ddr_sse2:
    lea         r8, [rsp-24]
    movq        mm0, [parm1q-FDEC_STRIDE]
    movq        [r8+8], mm0

    and         parm2d, byte 4
    mov         al,  [parm1q-FDEC_STRIDE+7]
    cmovnz      ax,  [parm1q-FDEC_STRIDE+8]
    mov         [r8+16], al

    mov         dh,  [parm1q+3*FDEC_STRIDE-1]
    mov         dl,  [parm1q+4*FDEC_STRIDE-1]
    mov         ah,  [parm1q-1*FDEC_STRIDE-1]
    mov         al,  [parm1q+0*FDEC_STRIDE-1]
    shl         edx, 16
    shl         eax, 16
    mov         dh,  [parm1q+5*FDEC_STRIDE-1]
    mov         dl,  [parm1q+6*FDEC_STRIDE-1]
    mov         ah,  [parm1q+1*FDEC_STRIDE-1]
    mov         al,  [parm1q+2*FDEC_STRIDE-1]
    mov         [r8+4], eax
    mov         [r8], edx
    movzx       eax, byte [parm1q+7*FDEC_STRIDE-1]
    movd        xmm4, eax
    movzx       edx, dl
    lea         eax, [rax+2*rax+2]
    add         eax, edx
    shr         eax, 2
    movd        xmm5, eax

; r8 -> {l6 l5 l4 l3 l2 l1 l0 lt t0 t1 t2 t3 t4 t5 t6 t7 t8}

    movdqu      xmm0, [r8]
    movdqu      xmm2, [r8+1]
    movdqa      xmm1, xmm0
    pslldq      xmm1, 1
    por         xmm1, xmm4
    PRED8x8_LOWPASS_XMM xmm3, xmm1, xmm2, xmm0, xmm4
    movdqa      xmm1, xmm3
    movdqa      xmm2, xmm3
    pslldq      xmm1, 1
    psrldq      xmm2, 1
    por         xmm1, xmm5
    PRED8x8_LOWPASS_XMM xmm0, xmm1, xmm2, xmm3, xmm4

    movdqa      xmm1, xmm0
    psrldq      xmm1, 1
%assign Y 7
%rep 3
    movq        [parm1q+Y*FDEC_STRIDE], xmm0
    psrldq      xmm0, 2
    movq        [parm1q+(Y-1)*FDEC_STRIDE], xmm1
    psrldq      xmm1, 2
%assign Y (Y-2)
%endrep
    movq        [parm1q+1*FDEC_STRIDE], xmm0
    movq        [parm1q+0*FDEC_STRIDE], xmm1

    ret

;-----------------------------------------------------------------------------
;
; void predict_8x8_vl_sse2( uint8_t *src, int i_neighbors )
;
;-----------------------------------------------------------------------------

ALIGN 16
predict_8x8_vl_sse2:
    PRED8x8_LOAD_TOP_TOPRIGHT_XMM
    PRED8x8_LOWPASS_XMM xmm4, xmm1, xmm2, xmm0, xmm5
    movdqa      xmm2, xmm4
    movdqa      xmm1, xmm4
    movdqa      xmm3, xmm4
    psrldq      xmm2, 1
    pslldq      xmm1, 1
    pavgb       xmm3, xmm2
    PRED8x8_LOWPASS_XMM xmm0, xmm1, xmm2, xmm4, xmm5
; xmm0: (t0 + 2*t1 + t2 + 2) >> 2
; xmm3: (t0 + t1 + 1) >> 1

%assign Y 1
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
;
; void predict_8x8_vr_core_mmxext( uint8_t *src, int i_neighbors, uint16_t ltt0 )
;
;-----------------------------------------------------------------------------

; fills only some pixels:
; f0123456789abcdef
; 0 .......
; 1  ,,,,,,
; 2  ......
; 3   ,,,,,
; 4   .....
; 5    ,,,,
; 6    ....
; 7     ,,,

ALIGN 16
predict_8x8_vr_core_mmxext:
    sub         parm1q, FDEC_STRIDE

    movq        mm1, [parm1q-1]
    movq        mm2, [parm1q+1]

    and         parm2d, byte 4
    jne         .have_topright
    mov         al,  [parm1q+7]
    mov         ah,  al
    pinsrw      mm2, eax, 3
.have_topright:

    PRED8x8_LOWPASS mm4, mm1, mm2, [parm1q], mm7
    movq        mm1, mm4
    movq        mm2, mm4
    psllq       mm1, 8
    movq        mm3, mm4
    pinsrw      mm1, parm3d, 0
    psrlq       mm2, 8
    pavgb       mm3, mm1
    PRED8x8_LOWPASS mm0, mm1, mm2, mm4, mm5

%assign Y 1
%rep 3
    psllq       mm0, 8
    movq        [parm1q+ Y   *FDEC_STRIDE], mm3
    movq        [parm1q+(Y+1)*FDEC_STRIDE], mm0
    psllq       mm3, 8
%assign Y (Y+2)
%endrep
    psllq       mm0, 8
    movq        [parm1q+ Y   *FDEC_STRIDE], mm3
    movq        [parm1q+(Y+1)*FDEC_STRIDE], mm0

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
; void predict_8x8c_p_core_mmxext( uint8_t *src, int i00, int b, int c )
;
;-----------------------------------------------------------------------------

ALIGN 16
predict_8x8c_p_core_mmxext:
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
;
; void predict_16x16_p_core_mmxext( uint8_t *src, int i00, int b, int c )
;
;-----------------------------------------------------------------------------

ALIGN 16
predict_16x16_p_core_mmxext:
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


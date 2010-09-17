;*****************************************************************************
;* sad-a.asm: x86 sad functions
;*****************************************************************************
;* Copyright (C) 2003-2010 x264 project
;*
;* Authors: Loren Merritt <lorenm@u.washington.edu>
;*          Jason Garrett-Glaser <darkshikari@gmail.com>
;*          Laurent Aimar <fenrir@via.ecp.fr>
;*          Alex Izvorski <aizvorksi@gmail.com>
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

h4x4_pred_shuf: db 3,3,3,3,7,7,7,7,11,11,11,11,15,15,15,15
h4x4_pred_shuf2: db 3,7,11,15,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
h8x8_pred_shuf: times 8 db 1
                times 8 db 0
                times 8 db 3
                times 8 db 2
                times 8 db 5
                times 8 db 4
                times 8 db 7
                times 8 db 6

SECTION .text

cextern pb_3
cextern pb_shuf8x8c
cextern pw_8
cextern sw_64

;=============================================================================
; SAD MMX
;=============================================================================

%macro SAD_INC_2x16P 0
    movq    mm1,    [r0]
    movq    mm2,    [r0+8]
    movq    mm3,    [r0+r1]
    movq    mm4,    [r0+r1+8]
    psadbw  mm1,    [r2]
    psadbw  mm2,    [r2+8]
    psadbw  mm3,    [r2+r3]
    psadbw  mm4,    [r2+r3+8]
    lea     r0,     [r0+2*r1]
    paddw   mm1,    mm2
    paddw   mm3,    mm4
    lea     r2,     [r2+2*r3]
    paddw   mm0,    mm1
    paddw   mm0,    mm3
%endmacro

%macro SAD_INC_2x8P 0
    movq    mm1,    [r0]
    movq    mm2,    [r0+r1]
    psadbw  mm1,    [r2]
    psadbw  mm2,    [r2+r3]
    lea     r0,     [r0+2*r1]
    paddw   mm0,    mm1
    paddw   mm0,    mm2
    lea     r2,     [r2+2*r3]
%endmacro

%macro SAD_INC_2x4P 0
    movd    mm1,    [r0]
    movd    mm2,    [r2]
    punpckldq mm1,  [r0+r1]
    punpckldq mm2,  [r2+r3]
    psadbw  mm1,    mm2
    paddw   mm0,    mm1
    lea     r0,     [r0+2*r1]
    lea     r2,     [r2+2*r3]
%endmacro

;-----------------------------------------------------------------------------
; int pixel_sad_16x16( uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
%macro SAD 2
cglobal pixel_sad_%1x%2_mmxext, 4,4
    pxor    mm0, mm0
%rep %2/2
    SAD_INC_2x%1P
%endrep
    movd    eax, mm0
    RET
%endmacro

SAD 16, 16
SAD 16,  8
SAD  8, 16
SAD  8,  8
SAD  8,  4
SAD  4,  8
SAD  4,  4



;=============================================================================
; SAD XMM
;=============================================================================

%macro SAD_END_SSE2 0
    movhlps m1, m0
    paddw   m0, m1
    movd   eax, m0
    RET
%endmacro

%macro SAD_W16 1
;-----------------------------------------------------------------------------
; int pixel_sad_16x16( uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
cglobal pixel_sad_16x16_%1, 4,4,8
    movdqu  m0, [r2]
    movdqu  m1, [r2+r3]
    lea     r2, [r2+2*r3]
    movdqu  m2, [r2]
    movdqu  m3, [r2+r3]
    lea     r2, [r2+2*r3]
    psadbw  m0, [r0]
    psadbw  m1, [r0+r1]
    lea     r0, [r0+2*r1]
    movdqu  m4, [r2]
    paddw   m0, m1
    psadbw  m2, [r0]
    psadbw  m3, [r0+r1]
    lea     r0, [r0+2*r1]
    movdqu  m5, [r2+r3]
    lea     r2, [r2+2*r3]
    paddw   m2, m3
    movdqu  m6, [r2]
    movdqu  m7, [r2+r3]
    lea     r2, [r2+2*r3]
    paddw   m0, m2
    psadbw  m4, [r0]
    psadbw  m5, [r0+r1]
    lea     r0, [r0+2*r1]
    movdqu  m1, [r2]
    paddw   m4, m5
    psadbw  m6, [r0]
    psadbw  m7, [r0+r1]
    lea     r0, [r0+2*r1]
    movdqu  m2, [r2+r3]
    lea     r2, [r2+2*r3]
    paddw   m6, m7
    movdqu  m3, [r2]
    paddw   m0, m4
    movdqu  m4, [r2+r3]
    lea     r2, [r2+2*r3]
    paddw   m0, m6
    psadbw  m1, [r0]
    psadbw  m2, [r0+r1]
    lea     r0, [r0+2*r1]
    movdqu  m5, [r2]
    paddw   m1, m2
    psadbw  m3, [r0]
    psadbw  m4, [r0+r1]
    lea     r0, [r0+2*r1]
    movdqu  m6, [r2+r3]
    lea     r2, [r2+2*r3]
    paddw   m3, m4
    movdqu  m7, [r2]
    paddw   m0, m1
    movdqu  m1, [r2+r3]
    paddw   m0, m3
    psadbw  m5, [r0]
    psadbw  m6, [r0+r1]
    lea     r0, [r0+2*r1]
    paddw   m5, m6
    psadbw  m7, [r0]
    psadbw  m1, [r0+r1]
    paddw   m7, m1
    paddw   m0, m5
    paddw   m0, m7
    SAD_END_SSE2

;-----------------------------------------------------------------------------
; int pixel_sad_16x8( uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
cglobal pixel_sad_16x8_%1, 4,4
    movdqu  m0, [r2]
    movdqu  m2, [r2+r3]
    lea     r2, [r2+2*r3]
    movdqu  m3, [r2]
    movdqu  m4, [r2+r3]
    psadbw  m0, [r0]
    psadbw  m2, [r0+r1]
    lea     r0, [r0+2*r1]
    psadbw  m3, [r0]
    psadbw  m4, [r0+r1]
    lea     r0, [r0+2*r1]
    lea     r2, [r2+2*r3]
    paddw   m0, m2
    paddw   m3, m4
    paddw   m0, m3
    movdqu  m1, [r2]
    movdqu  m2, [r2+r3]
    lea     r2, [r2+2*r3]
    movdqu  m3, [r2]
    movdqu  m4, [r2+r3]
    psadbw  m1, [r0]
    psadbw  m2, [r0+r1]
    lea     r0, [r0+2*r1]
    psadbw  m3, [r0]
    psadbw  m4, [r0+r1]
    lea     r0, [r0+2*r1]
    lea     r2, [r2+2*r3]
    paddw   m1, m2
    paddw   m3, m4
    paddw   m0, m1
    paddw   m0, m3
    SAD_END_SSE2
%endmacro

INIT_XMM
SAD_W16 sse2
%define movdqu lddqu
SAD_W16 sse3
%define movdqu movdqa
SAD_W16 sse2_aligned
%undef movdqu

%macro SAD_INC_4x8P_SSE 1
    movq    m1, [r0]
    movq    m2, [r0+r1]
    lea     r0, [r0+2*r1]
    movq    m3, [r2]
    movq    m4, [r2+r3]
    lea     r2, [r2+2*r3]
    movhps  m1, [r0]
    movhps  m2, [r0+r1]
    movhps  m3, [r2]
    movhps  m4, [r2+r3]
    lea     r0, [r0+2*r1]
    psadbw  m1, m3
    psadbw  m2, m4
    lea     r2, [r2+2*r3]
%if %1
    paddw   m0, m1
%else
    SWAP    m0, m1
%endif
    paddw   m0, m2
%endmacro

;Even on Nehalem, no sizes other than 8x16 benefit from this method.
cglobal pixel_sad_8x16_sse2, 4,4
    SAD_INC_4x8P_SSE 0
    SAD_INC_4x8P_SSE 1
    SAD_INC_4x8P_SSE 1
    SAD_INC_4x8P_SSE 1
    SAD_END_SSE2
    RET

;-----------------------------------------------------------------------------
; void intra_sad_x3_4x4( uint8_t *fenc, uint8_t *fdec, int res[3] );
;-----------------------------------------------------------------------------

cglobal intra_sad_x3_4x4_mmxext, 3,3
    pxor      mm7, mm7
    movd      mm0, [r1-FDEC_STRIDE]
    movd      mm1, [r0+FENC_STRIDE*0]
    movd      mm2, [r0+FENC_STRIDE*2]
    punpckldq mm0, mm0
    punpckldq mm1, [r0+FENC_STRIDE*1]
    punpckldq mm2, [r0+FENC_STRIDE*3]
    movq      mm6, mm0
    movq      mm3, mm1
    psadbw    mm3, mm0
    psadbw    mm0, mm2
    paddw     mm0, mm3
    movd     [r2], mm0 ;V prediction cost
    movd      mm3, [r1+FDEC_STRIDE*0-4]
    movd      mm0, [r1+FDEC_STRIDE*1-4]
    movd      mm4, [r1+FDEC_STRIDE*2-4]
    movd      mm5, [r1+FDEC_STRIDE*3-4]
    punpcklbw mm3, mm0
    punpcklbw mm4, mm5
    movq      mm5, mm3
    punpckhwd mm5, mm4
    punpckhdq mm5, mm6
    psadbw    mm5, mm7
    punpckhbw mm3, mm3
    punpckhbw mm4, mm4
    punpckhwd mm3, mm3
    punpckhwd mm4, mm4
    psraw     mm5, 2
    pavgw     mm5, mm7
    punpcklbw mm5, mm5
    pshufw    mm5, mm5, 0x0 ;DC prediction
    movq      mm6, mm5
    psadbw    mm5, mm1
    psadbw    mm6, mm2
    psadbw    mm1, mm3
    psadbw    mm2, mm4
    paddw     mm5, mm6
    paddw     mm1, mm2
    movd   [r2+8], mm5 ;DC prediction cost
    movd   [r2+4], mm1 ;H prediction cost
    RET

cglobal intra_sad_x3_4x4_sse4, 3,3
    movd       xmm4, [r1+FDEC_STRIDE*0-4]
    pinsrd     xmm4, [r1+FDEC_STRIDE*1-4], 1
    pinsrd     xmm4, [r1+FDEC_STRIDE*2-4], 2
    pinsrd     xmm4, [r1+FDEC_STRIDE*3-4], 3
    movd       xmm2, [r1-FDEC_STRIDE]
    pxor       xmm3, xmm3
    movdqa     xmm5, xmm4
    pshufb     xmm4, [h4x4_pred_shuf2] ; EFGH
    pshufb     xmm5, [h4x4_pred_shuf]  ; EEEEFFFFGGGGHHHH
    pshufd     xmm0, xmm2, 0           ; ABCDABCDABCDABCD
    punpckldq  xmm2, xmm4              ; ABCDEFGH
    psadbw     xmm2, xmm3
    movd       xmm1, [r0+FENC_STRIDE*0]
    pinsrd     xmm1, [r0+FENC_STRIDE*1], 1
    pinsrd     xmm1, [r0+FENC_STRIDE*2], 2
    pinsrd     xmm1, [r0+FENC_STRIDE*3], 3
    psadbw     xmm0, xmm1
    psadbw     xmm5, xmm1
    psraw      xmm2, 2
    pavgw      xmm2, xmm3
    pshufb     xmm2, xmm3              ; DC prediction
    movdqa     xmm3, xmm0
    punpcklqdq xmm0, xmm5
    punpckhqdq xmm3, xmm5
    psadbw     xmm2, xmm1
    paddw      xmm0, xmm3
    movhlps    xmm4, xmm2
    packusdw   xmm0, xmm0
    paddw      xmm2, xmm4
    movq       [r2], xmm0              ; V/H prediction costs
    movd     [r2+8], xmm2              ; DC prediction cost
    RET

;-----------------------------------------------------------------------------
; void intra_sad_x3_8x8( uint8_t *fenc, uint8_t edge[33], int res[3]);
;-----------------------------------------------------------------------------

;m0 = DC
;m6 = V
;m7 = H
;m1 = DC score
;m2 = V score
;m3 = H score
;m5 = pixel row
;m4 = temp

%macro INTRA_SAD_HVDC_ITER 2
    movq      m5, [r0+FENC_STRIDE*%1]
    movq      m4, m5
    psadbw    m4, m0
%if %1
    paddw     m1, m4
%else
    SWAP      m1, m4
%endif
    movq      m4, m5
    psadbw    m4, m6
%if %1
    paddw     m2, m4
%else
    SWAP      m2, m4
%endif
    pshufw    m4, m7, %2
    psadbw    m5, m4
%if %1
    paddw     m3, m5
%else
    SWAP      m3, m5
%endif
%endmacro

INIT_MMX
cglobal intra_sad_x3_8x8_mmxext, 3,3
    movq      m7, [r1+7]
    pxor      m0, m0
    movq      m6, [r1+16]  ;V prediction
    pxor      m1, m1
    psadbw    m0, m7
    psadbw    m1, m6
    paddw     m0, m1
    paddw     m0, [pw_8]
    psrlw     m0, 4
    punpcklbw m0, m0
    pshufw    m0, m0, 0x0 ;DC prediction
    punpckhbw m7, m7
    INTRA_SAD_HVDC_ITER 0, 0xff
    INTRA_SAD_HVDC_ITER 1, 0xaa
    INTRA_SAD_HVDC_ITER 2, 0x55
    INTRA_SAD_HVDC_ITER 3, 0x00
    movq      m7, [r1+7]
    punpcklbw m7, m7
    INTRA_SAD_HVDC_ITER 4, 0xff
    INTRA_SAD_HVDC_ITER 5, 0xaa
    INTRA_SAD_HVDC_ITER 6, 0x55
    INTRA_SAD_HVDC_ITER 7, 0x00
    movd  [r2+0], m2
    movd  [r2+4], m3
    movd  [r2+8], m1
    RET

INIT_XMM
cglobal intra_sad_x3_8x8_ssse3, 3,4,9
%ifdef PIC
    lea        r11, [h8x8_pred_shuf]
%define shuf r11
%else
%define shuf h8x8_pred_shuf
%endif
    movq       m0, [r1+7]   ; left pixels
    movq       m1, [r1+16]  ; top pixels
    pxor       m2, m2
    pxor       m3, m3
    psadbw     m2, m0
    psadbw     m3, m1
    paddw      m2, m3
    pxor       m3, m3       ; V score accumulator
    psraw      m2, 3
    pavgw      m2, m3
    punpcklqdq m1, m1       ; V prediction
    pshufb     m2, m3       ; DC prediction
    pxor       m4, m4       ; H score accumulator
    pxor       m5, m5       ; DC score accumulator
    mov       r3d, 6
.loop:
    movq        m6, [r0+FENC_STRIDE*0]
    movhps      m6, [r0+FENC_STRIDE*1]
    movdqa      m7, m0
    pshufb      m7, [shuf+r3*8] ; H prediction
%ifdef ARCH_X86_64
    movdqa      m8, m1
    psadbw      m7, m6
    psadbw      m8, m6
    psadbw      m6, m2
    paddw       m4, m7
    paddw       m3, m8
    paddw       m5, m6
%else
    psadbw      m7, m6
    paddw       m4, m7
    movdqa      m7, m1
    psadbw      m7, m6
    psadbw      m6, m2
    paddw       m3, m7
    paddw       m5, m6
%endif
    add         r0, FENC_STRIDE*2
    sub        r3d, 2
    jge .loop

    movhlps     m0, m3
    movhlps     m1, m4
    movhlps     m2, m5
    paddw       m3, m0
    paddw       m4, m1
    paddw       m5, m2
    movd    [r2+0], m3
    movd    [r2+4], m4
    movd    [r2+8], m5
    RET

;-----------------------------------------------------------------------------
; void intra_sad_x3_8x8c( uint8_t *fenc, uint8_t *fdec, int res[3] );
;-----------------------------------------------------------------------------

%macro INTRA_SAD_HV_ITER 2
%ifidn %2, ssse3
    movd        m1, [r1 + FDEC_STRIDE*(%1-4) - 4]
    movd        m3, [r1 + FDEC_STRIDE*(%1-3) - 4]
    pshufb      m1, m7
    pshufb      m3, m7
%else
    movq        m1, [r1 + FDEC_STRIDE*(%1-4) - 8]
    movq        m3, [r1 + FDEC_STRIDE*(%1-3) - 8]
    punpckhbw   m1, m1
    punpckhbw   m3, m3
    pshufw      m1, m1, 0xff
    pshufw      m3, m3, 0xff
%endif
    movq        m4, [r0 + FENC_STRIDE*(%1+0)]
    movq        m5, [r0 + FENC_STRIDE*(%1+1)]
    psadbw      m1, m4
    psadbw      m3, m5
    psadbw      m4, m6
    psadbw      m5, m6
    paddw       m1, m3
    paddw       m4, m5
%if %1
    paddw       m0, m1
    paddw       m2, m4
%else
    SWAP 0,1
    SWAP 2,4
%endif
%endmacro

%macro INTRA_SAD_8x8C 1
cglobal intra_sad_x3_8x8c_%1, 3,3
    movq        m6, [r1 - FDEC_STRIDE]
    add         r1, FDEC_STRIDE*4
%ifidn %1,ssse3
    movq        m7, [pb_3]
%endif
    INTRA_SAD_HV_ITER 0, %1
    INTRA_SAD_HV_ITER 2, %1
    INTRA_SAD_HV_ITER 4, %1
    INTRA_SAD_HV_ITER 6, %1
    movd    [r2+4], m0
    movd    [r2+8], m2
    pxor        m7, m7
    movq        m2, [r1 + FDEC_STRIDE*-4 - 8]
    movq        m4, [r1 + FDEC_STRIDE*-2 - 8]
    movq        m3, [r1 + FDEC_STRIDE* 0 - 8]
    movq        m5, [r1 + FDEC_STRIDE* 2 - 8]
    punpckhbw   m2, [r1 + FDEC_STRIDE*-3 - 8]
    punpckhbw   m4, [r1 + FDEC_STRIDE*-1 - 8]
    punpckhbw   m3, [r1 + FDEC_STRIDE* 1 - 8]
    punpckhbw   m5, [r1 + FDEC_STRIDE* 3 - 8]
    punpckhbw   m2, m4
    punpckhbw   m3, m5
    psrlq       m2, 32
    psrlq       m3, 32
    psadbw      m2, m7 ; s2
    psadbw      m3, m7 ; s3
    movq        m1, m6
    SWAP        0, 6
    punpckldq   m0, m7
    punpckhdq   m1, m7
    psadbw      m0, m7 ; s0
    psadbw      m1, m7 ; s1
    punpcklwd   m0, m1
    punpcklwd   m2, m3
    punpckldq   m0, m2 ;s0 s1 s2 s3
    pshufw      m3, m0, 11110110b ;s2,s1,s3,s3
    pshufw      m0, m0, 01110100b ;s0,s1,s3,s1
    paddw       m0, m3
    psrlw       m0, 2
    pavgw       m0, m7 ; s0+s2, s1, s3, s1+s3
%ifidn %1, ssse3
    movq2dq   xmm0, m0
    pshufb    xmm0, [pb_shuf8x8c]
    movq      xmm1, [r0+FENC_STRIDE*0]
    movq      xmm2, [r0+FENC_STRIDE*1]
    movq      xmm3, [r0+FENC_STRIDE*2]
    movq      xmm4, [r0+FENC_STRIDE*3]
    movhps    xmm1, [r0+FENC_STRIDE*4]
    movhps    xmm2, [r0+FENC_STRIDE*5]
    movhps    xmm3, [r0+FENC_STRIDE*6]
    movhps    xmm4, [r0+FENC_STRIDE*7]
    psadbw    xmm1, xmm0
    psadbw    xmm2, xmm0
    psadbw    xmm3, xmm0
    psadbw    xmm4, xmm0
    paddw     xmm1, xmm2
    paddw     xmm1, xmm3
    paddw     xmm1, xmm4
    movhlps   xmm0, xmm1
    paddw     xmm1, xmm0
    movd      [r2], xmm1
%else
    packuswb    m0, m0
    punpcklbw   m0, m0
    movq        m1, m0
    punpcklbw   m0, m0 ; 4x dc0 4x dc1
    punpckhbw   m1, m1 ; 4x dc2 4x dc3
    movq        m2, [r0+FENC_STRIDE*0]
    movq        m3, [r0+FENC_STRIDE*1]
    movq        m4, [r0+FENC_STRIDE*2]
    movq        m5, [r0+FENC_STRIDE*3]
    movq        m6, [r0+FENC_STRIDE*4]
    movq        m7, [r0+FENC_STRIDE*5]
    psadbw      m2, m0
    psadbw      m3, m0
    psadbw      m4, m0
    psadbw      m5, m0
    movq        m0, [r0+FENC_STRIDE*6]
    psadbw      m6, m1
    psadbw      m7, m1
    psadbw      m0, m1
    psadbw      m1, [r0+FENC_STRIDE*7]
    paddw       m2, m3
    paddw       m4, m5
    paddw       m6, m7
    paddw       m0, m1
    paddw       m2, m4
    paddw       m6, m0
    paddw       m2, m6
    movd      [r2], m2
%endif
    RET
%endmacro

INIT_MMX
INTRA_SAD_8x8C mmxext
INTRA_SAD_8x8C ssse3


;-----------------------------------------------------------------------------
; void intra_sad_x3_16x16( uint8_t *fenc, uint8_t *fdec, int res[3] );
;-----------------------------------------------------------------------------

;xmm7: DC prediction    xmm6: H prediction  xmm5: V prediction
;xmm4: DC pred score    xmm3: H pred score  xmm2: V pred score
%macro INTRA_SAD16 1-2 0
cglobal intra_sad_x3_16x16_%1,3,5,%2
    pxor    mm0, mm0
    pxor    mm1, mm1
    psadbw  mm0, [r1-FDEC_STRIDE+0]
    psadbw  mm1, [r1-FDEC_STRIDE+8]
    paddw   mm0, mm1
    movd    r3d, mm0
%ifidn %1, ssse3
    mova  m1, [pb_3]
%endif
%assign x 0
%rep 16
    movzx   r4d, byte [r1-1+FDEC_STRIDE*(x&3)]
%if (x&3)==3 && x!=15
    add      r1, FDEC_STRIDE*4
%endif
    add     r3d, r4d
%assign x x+1
%endrep
    sub      r1, FDEC_STRIDE*12
    add     r3d, 16
    shr     r3d, 5
    imul    r3d, 0x01010101
    movd    m7, r3d
    mova    m5, [r1-FDEC_STRIDE]
%if mmsize==16
    pshufd  m7, m7, 0
%else
    mova    m1, [r1-FDEC_STRIDE+8]
    punpckldq m7, m7
%endif
    pxor    m4, m4
    pxor    m3, m3
    pxor    m2, m2
    mov     r3d, 15*FENC_STRIDE
.vloop:
    SPLATB  m6, r1+r3*2-1, m1
    mova    m0, [r0+r3]
    psadbw  m0, m7
    paddw   m4, m0
    mova    m0, [r0+r3]
    psadbw  m0, m5
    paddw   m2, m0
%if mmsize==8
    mova    m0, [r0+r3]
    psadbw  m0, m6
    paddw   m3, m0
    mova    m0, [r0+r3+8]
    psadbw  m0, m7
    paddw   m4, m0
    mova    m0, [r0+r3+8]
    psadbw  m0, m1
    paddw   m2, m0
    psadbw  m6, [r0+r3+8]
    paddw   m3, m6
%else
    psadbw  m6, [r0+r3]
    paddw   m3, m6
%endif
    add     r3d, -FENC_STRIDE
    jge .vloop
%if mmsize==16
    pslldq  m3, 4
    por     m3, m2
    movhlps m1, m3
    paddw   m3, m1
    movq  [r2+0], m3
    movhlps m1, m4
    paddw   m4, m1
%else
    movd  [r2+0], m2
    movd  [r2+4], m3
%endif
    movd  [r2+8], m4
    RET
%endmacro

INIT_MMX
%define SPLATB SPLATB_MMX
INTRA_SAD16 mmxext
INIT_XMM
INTRA_SAD16 sse2, 8
%define SPLATB SPLATB_SSSE3
INTRA_SAD16 ssse3, 8



;=============================================================================
; SAD x3/x4 MMX
;=============================================================================

%macro SAD_X3_START_1x8P 0
    movq    mm3,    [r0]
    movq    mm0,    [r1]
    movq    mm1,    [r2]
    movq    mm2,    [r3]
    psadbw  mm0,    mm3
    psadbw  mm1,    mm3
    psadbw  mm2,    mm3
%endmacro

%macro SAD_X3_1x8P 2
    movq    mm3,    [r0+%1]
    movq    mm4,    [r1+%2]
    movq    mm5,    [r2+%2]
    movq    mm6,    [r3+%2]
    psadbw  mm4,    mm3
    psadbw  mm5,    mm3
    psadbw  mm6,    mm3
    paddw   mm0,    mm4
    paddw   mm1,    mm5
    paddw   mm2,    mm6
%endmacro

%macro SAD_X3_START_2x4P 3
    movd      mm3,  [r0]
    movd      %1,   [r1]
    movd      %2,   [r2]
    movd      %3,   [r3]
    punpckldq mm3,  [r0+FENC_STRIDE]
    punpckldq %1,   [r1+r4]
    punpckldq %2,   [r2+r4]
    punpckldq %3,   [r3+r4]
    psadbw    %1,   mm3
    psadbw    %2,   mm3
    psadbw    %3,   mm3
%endmacro

%macro SAD_X3_2x16P 1
%if %1
    SAD_X3_START_1x8P
%else
    SAD_X3_1x8P 0, 0
%endif
    SAD_X3_1x8P 8, 8
    SAD_X3_1x8P FENC_STRIDE, r4
    SAD_X3_1x8P FENC_STRIDE+8, r4+8
    add     r0, 2*FENC_STRIDE
    lea     r1, [r1+2*r4]
    lea     r2, [r2+2*r4]
    lea     r3, [r3+2*r4]
%endmacro

%macro SAD_X3_2x8P 1
%if %1
    SAD_X3_START_1x8P
%else
    SAD_X3_1x8P 0, 0
%endif
    SAD_X3_1x8P FENC_STRIDE, r4
    add     r0, 2*FENC_STRIDE
    lea     r1, [r1+2*r4]
    lea     r2, [r2+2*r4]
    lea     r3, [r3+2*r4]
%endmacro

%macro SAD_X3_2x4P 1
%if %1
    SAD_X3_START_2x4P mm0, mm1, mm2
%else
    SAD_X3_START_2x4P mm4, mm5, mm6
    paddw     mm0,  mm4
    paddw     mm1,  mm5
    paddw     mm2,  mm6
%endif
    add     r0, 2*FENC_STRIDE
    lea     r1, [r1+2*r4]
    lea     r2, [r2+2*r4]
    lea     r3, [r3+2*r4]
%endmacro

%macro SAD_X4_START_1x8P 0
    movq    mm7,    [r0]
    movq    mm0,    [r1]
    movq    mm1,    [r2]
    movq    mm2,    [r3]
    movq    mm3,    [r4]
    psadbw  mm0,    mm7
    psadbw  mm1,    mm7
    psadbw  mm2,    mm7
    psadbw  mm3,    mm7
%endmacro

%macro SAD_X4_1x8P 2
    movq    mm7,    [r0+%1]
    movq    mm4,    [r1+%2]
    movq    mm5,    [r2+%2]
    movq    mm6,    [r3+%2]
    psadbw  mm4,    mm7
    psadbw  mm5,    mm7
    psadbw  mm6,    mm7
    psadbw  mm7,    [r4+%2]
    paddw   mm0,    mm4
    paddw   mm1,    mm5
    paddw   mm2,    mm6
    paddw   mm3,    mm7
%endmacro

%macro SAD_X4_START_2x4P 0
    movd      mm7,  [r0]
    movd      mm0,  [r1]
    movd      mm1,  [r2]
    movd      mm2,  [r3]
    movd      mm3,  [r4]
    punpckldq mm7,  [r0+FENC_STRIDE]
    punpckldq mm0,  [r1+r5]
    punpckldq mm1,  [r2+r5]
    punpckldq mm2,  [r3+r5]
    punpckldq mm3,  [r4+r5]
    psadbw    mm0,  mm7
    psadbw    mm1,  mm7
    psadbw    mm2,  mm7
    psadbw    mm3,  mm7
%endmacro

%macro SAD_X4_INC_2x4P 0
    movd      mm7,  [r0]
    movd      mm4,  [r1]
    movd      mm5,  [r2]
    punpckldq mm7,  [r0+FENC_STRIDE]
    punpckldq mm4,  [r1+r5]
    punpckldq mm5,  [r2+r5]
    psadbw    mm4,  mm7
    psadbw    mm5,  mm7
    paddw     mm0,  mm4
    paddw     mm1,  mm5
    movd      mm4,  [r3]
    movd      mm5,  [r4]
    punpckldq mm4,  [r3+r5]
    punpckldq mm5,  [r4+r5]
    psadbw    mm4,  mm7
    psadbw    mm5,  mm7
    paddw     mm2,  mm4
    paddw     mm3,  mm5
%endmacro

%macro SAD_X4_2x16P 1
%if %1
    SAD_X4_START_1x8P
%else
    SAD_X4_1x8P 0, 0
%endif
    SAD_X4_1x8P 8, 8
    SAD_X4_1x8P FENC_STRIDE, r5
    SAD_X4_1x8P FENC_STRIDE+8, r5+8
    add     r0, 2*FENC_STRIDE
    lea     r1, [r1+2*r5]
    lea     r2, [r2+2*r5]
    lea     r3, [r3+2*r5]
    lea     r4, [r4+2*r5]
%endmacro

%macro SAD_X4_2x8P 1
%if %1
    SAD_X4_START_1x8P
%else
    SAD_X4_1x8P 0, 0
%endif
    SAD_X4_1x8P FENC_STRIDE, r5
    add     r0, 2*FENC_STRIDE
    lea     r1, [r1+2*r5]
    lea     r2, [r2+2*r5]
    lea     r3, [r3+2*r5]
    lea     r4, [r4+2*r5]
%endmacro

%macro SAD_X4_2x4P 1
%if %1
    SAD_X4_START_2x4P
%else
    SAD_X4_INC_2x4P
%endif
    add     r0, 2*FENC_STRIDE
    lea     r1, [r1+2*r5]
    lea     r2, [r2+2*r5]
    lea     r3, [r3+2*r5]
    lea     r4, [r4+2*r5]
%endmacro

%macro SAD_X3_END 0
%ifdef UNIX64
    movd    [r5+0], mm0
    movd    [r5+4], mm1
    movd    [r5+8], mm2
%else
    mov     r0, r5mp
    movd    [r0+0], mm0
    movd    [r0+4], mm1
    movd    [r0+8], mm2
%endif
    RET
%endmacro

%macro SAD_X4_END 0
    mov     r0, r6mp
    movd    [r0+0], mm0
    movd    [r0+4], mm1
    movd    [r0+8], mm2
    movd    [r0+12], mm3
    RET
%endmacro

;-----------------------------------------------------------------------------
; void pixel_sad_x3_16x16( uint8_t *fenc, uint8_t *pix0, uint8_t *pix1,
;                          uint8_t *pix2, int i_stride, int scores[3] )
;-----------------------------------------------------------------------------
%macro SAD_X 3
cglobal pixel_sad_x%1_%2x%3_mmxext, %1+2, %1+2
%ifdef WIN64
    %assign i %1+1
    movsxd r %+ i, r %+ i %+ d
%endif
    SAD_X%1_2x%2P 1
%rep %3/2-1
    SAD_X%1_2x%2P 0
%endrep
    SAD_X%1_END
%endmacro

SAD_X 3, 16, 16
SAD_X 3, 16,  8
SAD_X 3,  8, 16
SAD_X 3,  8,  8
SAD_X 3,  8,  4
SAD_X 3,  4,  8
SAD_X 3,  4,  4
SAD_X 4, 16, 16
SAD_X 4, 16,  8
SAD_X 4,  8, 16
SAD_X 4,  8,  8
SAD_X 4,  8,  4
SAD_X 4,  4,  8
SAD_X 4,  4,  4



;=============================================================================
; SAD x3/x4 XMM
;=============================================================================

%macro SAD_X3_START_1x16P_SSE2 0
    movdqa xmm3, [r0]
    movdqu xmm0, [r1]
    movdqu xmm1, [r2]
    movdqu xmm2, [r3]
    psadbw xmm0, xmm3
    psadbw xmm1, xmm3
    psadbw xmm2, xmm3
%endmacro

%macro SAD_X3_1x16P_SSE2 2
    movdqa xmm3, [r0+%1]
    movdqu xmm4, [r1+%2]
    movdqu xmm5, [r2+%2]
    movdqu xmm6, [r3+%2]
    psadbw xmm4, xmm3
    psadbw xmm5, xmm3
    psadbw xmm6, xmm3
    paddw  xmm0, xmm4
    paddw  xmm1, xmm5
    paddw  xmm2, xmm6
%endmacro

%macro SAD_X3_2x16P_SSE2 1
%if %1
    SAD_X3_START_1x16P_SSE2
%else
    SAD_X3_1x16P_SSE2 0, 0
%endif
    SAD_X3_1x16P_SSE2 FENC_STRIDE, r4
    add  r0, 2*FENC_STRIDE
    lea  r1, [r1+2*r4]
    lea  r2, [r2+2*r4]
    lea  r3, [r3+2*r4]
%endmacro

%macro SAD_X3_START_2x8P_SSE2 0
    movq    xmm7, [r0]
    movq    xmm0, [r1]
    movq    xmm1, [r2]
    movq    xmm2, [r3]
    movhps  xmm7, [r0+FENC_STRIDE]
    movhps  xmm0, [r1+r4]
    movhps  xmm1, [r2+r4]
    movhps  xmm2, [r3+r4]
    psadbw  xmm0, xmm7
    psadbw  xmm1, xmm7
    psadbw  xmm2, xmm7
%endmacro

%macro SAD_X3_2x8P_SSE2 0
    movq    xmm7, [r0]
    movq    xmm3, [r1]
    movq    xmm4, [r2]
    movq    xmm5, [r3]
    movhps  xmm7, [r0+FENC_STRIDE]
    movhps  xmm3, [r1+r4]
    movhps  xmm4, [r2+r4]
    movhps  xmm5, [r3+r4]
    psadbw  xmm3, xmm7
    psadbw  xmm4, xmm7
    psadbw  xmm5, xmm7
    paddw   xmm0, xmm3
    paddw   xmm1, xmm4
    paddw   xmm2, xmm5
%endmacro

%macro SAD_X4_START_2x8P_SSE2 0
    movq    xmm7, [r0]
    movq    xmm0, [r1]
    movq    xmm1, [r2]
    movq    xmm2, [r3]
    movq    xmm3, [r4]
    movhps  xmm7, [r0+FENC_STRIDE]
    movhps  xmm0, [r1+r5]
    movhps  xmm1, [r2+r5]
    movhps  xmm2, [r3+r5]
    movhps  xmm3, [r4+r5]
    psadbw  xmm0, xmm7
    psadbw  xmm1, xmm7
    psadbw  xmm2, xmm7
    psadbw  xmm3, xmm7
%endmacro

%macro SAD_X4_2x8P_SSE2 0
    movq    xmm7, [r0]
    movq    xmm4, [r1]
    movq    xmm5, [r2]
%ifdef ARCH_X86_64
    movq    xmm6, [r3]
    movq    xmm8, [r4]
    movhps  xmm7, [r0+FENC_STRIDE]
    movhps  xmm4, [r1+r5]
    movhps  xmm5, [r2+r5]
    movhps  xmm6, [r3+r5]
    movhps  xmm8, [r4+r5]
    psadbw  xmm4, xmm7
    psadbw  xmm5, xmm7
    psadbw  xmm6, xmm7
    psadbw  xmm8, xmm7
    paddw   xmm0, xmm4
    paddw   xmm1, xmm5
    paddw   xmm2, xmm6
    paddw   xmm3, xmm8
%else
    movhps  xmm7, [r0+FENC_STRIDE]
    movhps  xmm4, [r1+r5]
    movhps  xmm5, [r2+r5]
    psadbw  xmm4, xmm7
    psadbw  xmm5, xmm7
    paddw   xmm0, xmm4
    paddw   xmm1, xmm5
    movq    xmm6, [r3]
    movq    xmm4, [r4]
    movhps  xmm6, [r3+r5]
    movhps  xmm4, [r4+r5]
    psadbw  xmm6, xmm7
    psadbw  xmm4, xmm7
    paddw   xmm2, xmm6
    paddw   xmm3, xmm4
%endif
%endmacro

%macro SAD_X4_START_1x16P_SSE2 0
    movdqa xmm7, [r0]
    movdqu xmm0, [r1]
    movdqu xmm1, [r2]
    movdqu xmm2, [r3]
    movdqu xmm3, [r4]
    psadbw xmm0, xmm7
    psadbw xmm1, xmm7
    psadbw xmm2, xmm7
    psadbw xmm3, xmm7
%endmacro

%macro SAD_X4_1x16P_SSE2 2
    movdqa xmm7, [r0+%1]
    movdqu xmm4, [r1+%2]
    movdqu xmm5, [r2+%2]
    movdqu xmm6, [r3+%2]
%ifdef ARCH_X86_64
    movdqu xmm8, [r4+%2]
    psadbw xmm4, xmm7
    psadbw xmm5, xmm7
    psadbw xmm6, xmm7
    psadbw xmm8, xmm7
    paddw  xmm0, xmm4
    paddw  xmm1, xmm5
    paddw  xmm2, xmm6
    paddw  xmm3, xmm8
%else
    psadbw xmm4, xmm7
    psadbw xmm5, xmm7
    paddw  xmm0, xmm4
    psadbw xmm6, xmm7
    movdqu xmm4, [r4+%2]
    paddw  xmm1, xmm5
    psadbw xmm4, xmm7
    paddw  xmm2, xmm6
    paddw  xmm3, xmm4
%endif
%endmacro

%macro SAD_X4_2x16P_SSE2 1
%if %1
    SAD_X4_START_1x16P_SSE2
%else
    SAD_X4_1x16P_SSE2 0, 0
%endif
    SAD_X4_1x16P_SSE2 FENC_STRIDE, r5
    add  r0, 2*FENC_STRIDE
    lea  r1, [r1+2*r5]
    lea  r2, [r2+2*r5]
    lea  r3, [r3+2*r5]
    lea  r4, [r4+2*r5]
%endmacro

%macro SAD_X3_2x8P_SSE2 1
%if %1
    SAD_X3_START_2x8P_SSE2
%else
    SAD_X3_2x8P_SSE2
%endif
    add  r0, 2*FENC_STRIDE
    lea  r1, [r1+2*r4]
    lea  r2, [r2+2*r4]
    lea  r3, [r3+2*r4]
%endmacro

%macro SAD_X4_2x8P_SSE2 1
%if %1
    SAD_X4_START_2x8P_SSE2
%else
    SAD_X4_2x8P_SSE2
%endif
    add  r0, 2*FENC_STRIDE
    lea  r1, [r1+2*r5]
    lea  r2, [r2+2*r5]
    lea  r3, [r3+2*r5]
    lea  r4, [r4+2*r5]
%endmacro

%macro SAD_X3_END_SSE2 0
    movhlps xmm4, xmm0
    movhlps xmm5, xmm1
    movhlps xmm6, xmm2
    paddw   xmm0, xmm4
    paddw   xmm1, xmm5
    paddw   xmm2, xmm6
%ifdef UNIX64
    movd [r5+0], xmm0
    movd [r5+4], xmm1
    movd [r5+8], xmm2
%else
    mov      r0, r5mp
    movd [r0+0], xmm0
    movd [r0+4], xmm1
    movd [r0+8], xmm2
%endif
    RET
%endmacro

%macro SAD_X4_END_SSE2 0
    mov       r0, r6mp
    psllq   xmm1, 32
    psllq   xmm3, 32
    paddw   xmm0, xmm1
    paddw   xmm2, xmm3
    movhlps xmm1, xmm0
    movhlps xmm3, xmm2
    paddw   xmm0, xmm1
    paddw   xmm2, xmm3
    movq  [r0+0], xmm0
    movq  [r0+8], xmm2
    RET
%endmacro

%macro SAD_X3_START_1x16P_SSE2_MISALIGN 0
    movdqa xmm2, [r0]
    movdqu xmm0, [r1]
    movdqu xmm1, [r2]
    psadbw xmm0, xmm2
    psadbw xmm1, xmm2
    psadbw xmm2, [r3]
%endmacro

%macro SAD_X3_1x16P_SSE2_MISALIGN 2
    movdqa xmm3, [r0+%1]
    movdqu xmm4, [r1+%2]
    movdqu xmm5, [r2+%2]
    psadbw xmm4, xmm3
    psadbw xmm5, xmm3
    psadbw xmm3, [r3+%2]
    paddw  xmm0, xmm4
    paddw  xmm1, xmm5
    paddw  xmm2, xmm3
%endmacro

%macro SAD_X4_START_1x16P_SSE2_MISALIGN 0
    movdqa xmm3, [r0]
    movdqu xmm0, [r1]
    movdqu xmm1, [r2]
    movdqu xmm2, [r3]
    psadbw xmm0, xmm3
    psadbw xmm1, xmm3
    psadbw xmm2, xmm3
    psadbw xmm3, [r4]
%endmacro

%macro SAD_X4_1x16P_SSE2_MISALIGN 2
    movdqa xmm7, [r0+%1]
    movdqu xmm4, [r1+%2]
    movdqu xmm5, [r2+%2]
    movdqu xmm6, [r3+%2]
    psadbw xmm4, xmm7
    psadbw xmm5, xmm7
    psadbw xmm6, xmm7
    psadbw xmm7, [r4+%2]
    paddw  xmm0, xmm4
    paddw  xmm1, xmm5
    paddw  xmm2, xmm6
    paddw  xmm3, xmm7
%endmacro

%macro SAD_X3_2x16P_SSE2_MISALIGN 1
%if %1
    SAD_X3_START_1x16P_SSE2_MISALIGN
%else
    SAD_X3_1x16P_SSE2_MISALIGN 0, 0
%endif
    SAD_X3_1x16P_SSE2_MISALIGN FENC_STRIDE, r4
    add  r0, 2*FENC_STRIDE
    lea  r1, [r1+2*r4]
    lea  r2, [r2+2*r4]
    lea  r3, [r3+2*r4]
%endmacro

%macro SAD_X4_2x16P_SSE2_MISALIGN 1
%if %1
    SAD_X4_START_1x16P_SSE2_MISALIGN
%else
    SAD_X4_1x16P_SSE2_MISALIGN 0, 0
%endif
    SAD_X4_1x16P_SSE2_MISALIGN FENC_STRIDE, r5
    add  r0, 2*FENC_STRIDE
    lea  r1, [r1+2*r5]
    lea  r2, [r2+2*r5]
    lea  r3, [r3+2*r5]
    lea  r4, [r4+2*r5]
%endmacro

;-----------------------------------------------------------------------------
; void pixel_sad_x3_16x16( uint8_t *fenc, uint8_t *pix0, uint8_t *pix1,
;                          uint8_t *pix2, int i_stride, int scores[3] )
;-----------------------------------------------------------------------------
%macro SAD_X_SSE2 4
cglobal pixel_sad_x%1_%2x%3_%4, 2+%1,2+%1,9
%ifdef WIN64
    %assign i %1+1
    movsxd r %+ i, r %+ i %+ d
%endif
    SAD_X%1_2x%2P_SSE2 1
%rep %3/2-1
    SAD_X%1_2x%2P_SSE2 0
%endrep
    SAD_X%1_END_SSE2
%endmacro

%macro SAD_X_SSE2_MISALIGN 4
cglobal pixel_sad_x%1_%2x%3_%4_misalign, 2+%1,2+%1,9
%ifdef WIN64
    %assign i %1+1
    movsxd r %+ i, r %+ i %+ d
%endif
    SAD_X%1_2x%2P_SSE2_MISALIGN 1
%rep %3/2-1
    SAD_X%1_2x%2P_SSE2_MISALIGN 0
%endrep
    SAD_X%1_END_SSE2
%endmacro

SAD_X_SSE2 3, 16, 16, sse2
SAD_X_SSE2 3, 16,  8, sse2
SAD_X_SSE2 3,  8, 16, sse2
SAD_X_SSE2 3,  8,  8, sse2
SAD_X_SSE2 3,  8,  4, sse2
SAD_X_SSE2 4, 16, 16, sse2
SAD_X_SSE2 4, 16,  8, sse2
SAD_X_SSE2 4,  8, 16, sse2
SAD_X_SSE2 4,  8,  8, sse2
SAD_X_SSE2 4,  8,  4, sse2

SAD_X_SSE2_MISALIGN 3, 16, 16, sse2
SAD_X_SSE2_MISALIGN 3, 16,  8, sse2
SAD_X_SSE2_MISALIGN 4, 16, 16, sse2
SAD_X_SSE2_MISALIGN 4, 16,  8, sse2

%define movdqu lddqu
SAD_X_SSE2 3, 16, 16, sse3
SAD_X_SSE2 3, 16,  8, sse3
SAD_X_SSE2 4, 16, 16, sse3
SAD_X_SSE2 4, 16,  8, sse3
%undef movdqu



;=============================================================================
; SAD cacheline split
;=============================================================================

; Core2 (Conroe) can load unaligned data just as quickly as aligned data...
; unless the unaligned data spans the border between 2 cachelines, in which
; case it's really slow. The exact numbers may differ, but all Intel cpus prior
; to Nehalem have a large penalty for cacheline splits.
; (8-byte alignment exactly half way between two cachelines is ok though.)
; LDDQU was supposed to fix this, but it only works on Pentium 4.
; So in the split case we load aligned data and explicitly perform the
; alignment between registers. Like on archs that have only aligned loads,
; except complicated by the fact that PALIGNR takes only an immediate, not
; a variable alignment.
; It is also possible to hoist the realignment to the macroblock level (keep
; 2 copies of the reference frame, offset by 32 bytes), but the extra memory
; needed for that method makes it often slower.

; sad 16x16 costs on Core2:
; good offsets: 49 cycles (50/64 of all mvs)
; cacheline split: 234 cycles (14/64 of all mvs. ammortized: +40 cycles)
; page split: 3600 cycles (14/4096 of all mvs. ammortized: +11.5 cycles)
; cache or page split with palignr: 57 cycles (ammortized: +2 cycles)

; computed jump assumes this loop is exactly 80 bytes
%macro SAD16_CACHELINE_LOOP_SSE2 1 ; alignment
ALIGN 16
sad_w16_align%1_sse2:
    movdqa  xmm1, [r2+16]
    movdqa  xmm2, [r2+r3+16]
    movdqa  xmm3, [r2]
    movdqa  xmm4, [r2+r3]
    pslldq  xmm1, 16-%1
    pslldq  xmm2, 16-%1
    psrldq  xmm3, %1
    psrldq  xmm4, %1
    por     xmm1, xmm3
    por     xmm2, xmm4
    psadbw  xmm1, [r0]
    psadbw  xmm2, [r0+r1]
    paddw   xmm0, xmm1
    paddw   xmm0, xmm2
    lea     r0,   [r0+2*r1]
    lea     r2,   [r2+2*r3]
    dec     r4
    jg sad_w16_align%1_sse2
    ret
%endmacro

; computed jump assumes this loop is exactly 64 bytes
%macro SAD16_CACHELINE_LOOP_SSSE3 1 ; alignment
ALIGN 16
sad_w16_align%1_ssse3:
    movdqa  xmm1, [r2+16]
    movdqa  xmm2, [r2+r3+16]
    palignr xmm1, [r2], %1
    palignr xmm2, [r2+r3], %1
    psadbw  xmm1, [r0]
    psadbw  xmm2, [r0+r1]
    paddw   xmm0, xmm1
    paddw   xmm0, xmm2
    lea     r0,   [r0+2*r1]
    lea     r2,   [r2+2*r3]
    dec     r4
    jg sad_w16_align%1_ssse3
    ret
%endmacro

%macro SAD16_CACHELINE_FUNC 2 ; cpu, height
cglobal pixel_sad_16x%2_cache64_%1
    mov     eax, r2m
    and     eax, 0x37
    cmp     eax, 0x30
    jle pixel_sad_16x%2_sse2
    PROLOGUE 4,6
    mov     r4d, r2d
    and     r4d, 15
%ifidn %1, ssse3
    shl     r4d, 6  ; code size = 64
%else
    lea     r4, [r4*5]
    shl     r4d, 4  ; code size = 80
%endif
%define sad_w16_addr (sad_w16_align1_%1 + (sad_w16_align1_%1 - sad_w16_align2_%1))
%ifdef PIC
    lea     r5, [sad_w16_addr]
    add     r5, r4
%else
    lea     r5, [sad_w16_addr + r4]
%endif
    and     r2, ~15
    mov     r4d, %2/2
    pxor    xmm0, xmm0
    call    r5
    movhlps xmm1, xmm0
    paddw   xmm0, xmm1
    movd    eax,  xmm0
    RET
%endmacro

%macro SAD_CACHELINE_START_MMX2 4 ; width, height, iterations, cacheline
    mov    eax, r2m
    and    eax, 0x17|%1|(%4>>1)
    cmp    eax, 0x10|%1|(%4>>1)
    jle pixel_sad_%1x%2_mmxext
    and    eax, 7
    shl    eax, 3
    movd   mm6, [sw_64]
    movd   mm7, eax
    psubw  mm6, mm7
    PROLOGUE 4,5
    and    r2, ~7
    mov    r4d, %3
    pxor   mm0, mm0
%endmacro

%macro SAD16_CACHELINE_FUNC_MMX2 2 ; height, cacheline
cglobal pixel_sad_16x%1_cache%2_mmxext
    SAD_CACHELINE_START_MMX2 16, %1, %1, %2
.loop:
    movq   mm1, [r2]
    movq   mm2, [r2+8]
    movq   mm3, [r2+16]
    movq   mm4, mm2
    psrlq  mm1, mm7
    psllq  mm2, mm6
    psllq  mm3, mm6
    psrlq  mm4, mm7
    por    mm1, mm2
    por    mm3, mm4
    psadbw mm1, [r0]
    psadbw mm3, [r0+8]
    paddw  mm0, mm1
    paddw  mm0, mm3
    add    r2, r3
    add    r0, r1
    dec    r4
    jg .loop
    movd   eax, mm0
    RET
%endmacro

%macro SAD8_CACHELINE_FUNC_MMX2 2 ; height, cacheline
cglobal pixel_sad_8x%1_cache%2_mmxext
    SAD_CACHELINE_START_MMX2 8, %1, %1/2, %2
.loop:
    movq   mm1, [r2+8]
    movq   mm2, [r2+r3+8]
    movq   mm3, [r2]
    movq   mm4, [r2+r3]
    psllq  mm1, mm6
    psllq  mm2, mm6
    psrlq  mm3, mm7
    psrlq  mm4, mm7
    por    mm1, mm3
    por    mm2, mm4
    psadbw mm1, [r0]
    psadbw mm2, [r0+r1]
    paddw  mm0, mm1
    paddw  mm0, mm2
    lea    r2, [r2+2*r3]
    lea    r0, [r0+2*r1]
    dec    r4
    jg .loop
    movd   eax, mm0
    RET
%endmacro

; sad_x3/x4_cache64: check each mv.
; if they're all within a cacheline, use normal sad_x3/x4.
; otherwise, send them individually to sad_cache64.
%macro CHECK_SPLIT 3 ; pix, width, cacheline
    mov  eax, %1
    and  eax, 0x17|%2|(%3>>1)
    cmp  eax, 0x10|%2|(%3>>1)
    jg .split
%endmacro

%macro SADX3_CACHELINE_FUNC 6 ; width, height, cacheline, normal_ver, split_ver, name
cglobal pixel_sad_x3_%1x%2_cache%3_%6
    CHECK_SPLIT r1m, %1, %3
    CHECK_SPLIT r2m, %1, %3
    CHECK_SPLIT r3m, %1, %3
    jmp pixel_sad_x3_%1x%2_%4
.split:
%ifdef ARCH_X86_64
    PROLOGUE 6,7
%ifdef WIN64
    movsxd r4, r4d
    sub  rsp, 8
%endif
    push r3
    push r2
    mov  r2, r1
    mov  r1, FENC_STRIDE
    mov  r3, r4
    mov  r10, r0
    mov  r11, r5
    call pixel_sad_%1x%2_cache%3_%5
    mov  [r11], eax
%ifdef WIN64
    mov  r2, [rsp]
%else
    pop  r2
%endif
    mov  r0, r10
    call pixel_sad_%1x%2_cache%3_%5
    mov  [r11+4], eax
%ifdef WIN64
    mov  r2, [rsp+8]
%else
    pop  r2
%endif
    mov  r0, r10
    call pixel_sad_%1x%2_cache%3_%5
    mov  [r11+8], eax
%ifdef WIN64
    add  rsp, 24
%endif
    RET
%else
    push edi
    mov  edi, [esp+28]
    push dword [esp+24]
    push dword [esp+16]
    push dword 16
    push dword [esp+20]
    call pixel_sad_%1x%2_cache%3_%5
    mov  ecx, [esp+32]
    mov  [edi], eax
    mov  [esp+8], ecx
    call pixel_sad_%1x%2_cache%3_%5
    mov  ecx, [esp+36]
    mov  [edi+4], eax
    mov  [esp+8], ecx
    call pixel_sad_%1x%2_cache%3_%5
    mov  [edi+8], eax
    add  esp, 16
    pop  edi
    ret
%endif
%endmacro

%macro SADX4_CACHELINE_FUNC 6 ; width, height, cacheline, normal_ver, split_ver, name
cglobal pixel_sad_x4_%1x%2_cache%3_%6
    CHECK_SPLIT r1m, %1, %3
    CHECK_SPLIT r2m, %1, %3
    CHECK_SPLIT r3m, %1, %3
    CHECK_SPLIT r4m, %1, %3
    jmp pixel_sad_x4_%1x%2_%4
.split:
%ifdef ARCH_X86_64
    PROLOGUE 6,7
    mov  r11,  r6mp
%ifdef WIN64
    movsxd r5, r5d
%endif
    push r4
    push r3
    push r2
    mov  r2, r1
    mov  r1, FENC_STRIDE
    mov  r3, r5
    mov  r10, r0
    call pixel_sad_%1x%2_cache%3_%5
    mov  [r11], eax
%ifdef WIN64
    mov  r2, [rsp]
%else
    pop  r2
%endif
    mov  r0, r10
    call pixel_sad_%1x%2_cache%3_%5
    mov  [r11+4], eax
%ifdef WIN64
    mov  r2, [rsp+8]
%else
    pop  r2
%endif
    mov  r0, r10
    call pixel_sad_%1x%2_cache%3_%5
    mov  [r11+8], eax
%ifdef WIN64
    mov  r2, [rsp+16]
%else
    pop  r2
%endif
    mov  r0, r10
    call pixel_sad_%1x%2_cache%3_%5
    mov  [r11+12], eax
%ifdef WIN64
    add  rsp, 24
%endif
    RET
%else
    push edi
    mov  edi, [esp+32]
    push dword [esp+28]
    push dword [esp+16]
    push dword 16
    push dword [esp+20]
    call pixel_sad_%1x%2_cache%3_%5
    mov  ecx, [esp+32]
    mov  [edi], eax
    mov  [esp+8], ecx
    call pixel_sad_%1x%2_cache%3_%5
    mov  ecx, [esp+36]
    mov  [edi+4], eax
    mov  [esp+8], ecx
    call pixel_sad_%1x%2_cache%3_%5
    mov  ecx, [esp+40]
    mov  [edi+8], eax
    mov  [esp+8], ecx
    call pixel_sad_%1x%2_cache%3_%5
    mov  [edi+12], eax
    add  esp, 16
    pop  edi
    ret
%endif
%endmacro

%macro SADX34_CACHELINE_FUNC 1+
    SADX3_CACHELINE_FUNC %1
    SADX4_CACHELINE_FUNC %1
%endmacro


; instantiate the aligned sads

%ifndef ARCH_X86_64
SAD16_CACHELINE_FUNC_MMX2  8, 32
SAD16_CACHELINE_FUNC_MMX2 16, 32
SAD8_CACHELINE_FUNC_MMX2   4, 32
SAD8_CACHELINE_FUNC_MMX2   8, 32
SAD8_CACHELINE_FUNC_MMX2  16, 32
SAD16_CACHELINE_FUNC_MMX2  8, 64
SAD16_CACHELINE_FUNC_MMX2 16, 64
%endif ; !ARCH_X86_64
SAD8_CACHELINE_FUNC_MMX2   4, 64
SAD8_CACHELINE_FUNC_MMX2   8, 64
SAD8_CACHELINE_FUNC_MMX2  16, 64

%ifndef ARCH_X86_64
SADX34_CACHELINE_FUNC 16, 16, 32, mmxext, mmxext, mmxext
SADX34_CACHELINE_FUNC 16,  8, 32, mmxext, mmxext, mmxext
SADX34_CACHELINE_FUNC  8, 16, 32, mmxext, mmxext, mmxext
SADX34_CACHELINE_FUNC  8,  8, 32, mmxext, mmxext, mmxext
SADX34_CACHELINE_FUNC 16, 16, 64, mmxext, mmxext, mmxext
SADX34_CACHELINE_FUNC 16,  8, 64, mmxext, mmxext, mmxext
%endif ; !ARCH_X86_64
SADX34_CACHELINE_FUNC  8, 16, 64, mmxext, mmxext, mmxext
SADX34_CACHELINE_FUNC  8,  8, 64, mmxext, mmxext, mmxext

%ifndef ARCH_X86_64
SAD16_CACHELINE_FUNC sse2, 8
SAD16_CACHELINE_FUNC sse2, 16
%assign i 1
%rep 15
SAD16_CACHELINE_LOOP_SSE2 i
%assign i i+1
%endrep
SADX34_CACHELINE_FUNC 16, 16, 64, sse2, sse2, sse2
SADX34_CACHELINE_FUNC 16,  8, 64, sse2, sse2, sse2
%endif ; !ARCH_X86_64
SADX34_CACHELINE_FUNC  8, 16, 64, sse2, mmxext, sse2

SAD16_CACHELINE_FUNC ssse3, 8
SAD16_CACHELINE_FUNC ssse3, 16
%assign i 1
%rep 15
SAD16_CACHELINE_LOOP_SSSE3 i
%assign i i+1
%endrep
SADX34_CACHELINE_FUNC 16, 16, 64, sse2, ssse3, ssse3
SADX34_CACHELINE_FUNC 16,  8, 64, sse2, ssse3, ssse3


;*****************************************************************************
;* deblock-a.asm: h264 encoder library
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

%include "amd64inc.asm"

SECTION_RODATA
pb_01: times 16 db 0x01
pb_03: times 16 db 0x03
pb_a1: times 16 db 0xa1

SECTION .text

; expands to [base],...,[base+7*stride]
%define PASS8ROWS(base, base3, stride, stride3) \
    [base], [base+stride], [base+stride*2], [base3], \
    [base3+stride], [base3+stride*2], [base3+stride3], [base3+stride*4]

; in: 8 rows of 4 bytes in %1..%8
; out: 4 rows of 8 bytes in mm0..mm3
%macro TRANSPOSE4x8_LOAD 8
    movd       mm0, %1
    movd       mm2, %2
    movd       mm1, %3
    movd       mm3, %4
    punpcklbw  mm0, mm2
    punpcklbw  mm1, mm3
    movq       mm2, mm0
    punpcklwd  mm0, mm1
    punpckhwd  mm2, mm1

    movd       mm4, %5
    movd       mm6, %6
    movd       mm5, %7
    movd       mm7, %8
    punpcklbw  mm4, mm6
    punpcklbw  mm5, mm7
    movq       mm6, mm4
    punpcklwd  mm4, mm5
    punpckhwd  mm6, mm5

    movq       mm1, mm0
    movq       mm3, mm2
    punpckldq  mm0, mm4
    punpckhdq  mm1, mm4
    punpckldq  mm2, mm6
    punpckhdq  mm3, mm6
%endmacro

; in: 4 rows of 8 bytes in mm0..mm3
; out: 8 rows of 4 bytes in %1..%8
%macro TRANSPOSE8x4_STORE 8
    movq       mm4, mm0
    movq       mm5, mm1
    movq       mm6, mm2
    punpckhdq  mm4, mm4
    punpckhdq  mm5, mm5
    punpckhdq  mm6, mm6

    punpcklbw  mm0, mm1
    punpcklbw  mm2, mm3
    movq       mm1, mm0
    punpcklwd  mm0, mm2
    punpckhwd  mm1, mm2
    movd       %1,  mm0
    punpckhdq  mm0, mm0
    movd       %2,  mm0
    movd       %3,  mm1
    punpckhdq  mm1, mm1
    movd       %4,  mm1

    punpckhdq  mm3, mm3
    punpcklbw  mm4, mm5
    punpcklbw  mm6, mm3
    movq       mm5, mm4
    punpcklwd  mm4, mm6
    punpckhwd  mm5, mm6
    movd       %5,  mm4
    punpckhdq  mm4, mm4
    movd       %6,  mm4
    movd       %7,  mm5
    punpckhdq  mm5, mm5
    movd       %8,  mm5
%endmacro

%macro SBUTTERFLY 4
    movq       %4, %2
    punpckl%1  %2, %3
    punpckh%1  %4, %3
%endmacro

; in: 8 rows of 8 (only the middle 6 pels are used) in %1..%8
; out: 6 rows of 8 in [%9+0*16] .. [%9+5*16]
%macro TRANSPOSE6x8_MEM 9
    movq  mm0, %1
    movq  mm1, %3
    movq  mm2, %5
    movq  mm3, %7
    SBUTTERFLY bw, mm0, %2, mm4
    SBUTTERFLY bw, mm1, %4, mm5
    SBUTTERFLY bw, mm2, %6, mm6
    movq  [%9+0x10], mm5
    SBUTTERFLY bw, mm3, %8, mm7
    SBUTTERFLY wd, mm0, mm1, mm5
    SBUTTERFLY wd, mm2, mm3, mm1
    punpckhdq mm0, mm2
    movq  [%9+0x00], mm0
    SBUTTERFLY wd, mm4, [%9+0x10], mm3
    SBUTTERFLY wd, mm6, mm7, mm2
    SBUTTERFLY dq, mm4, mm6, mm0
    SBUTTERFLY dq, mm5, mm1, mm7
    punpckldq mm3, mm2
    movq  [%9+0x10], mm5
    movq  [%9+0x20], mm7
    movq  [%9+0x30], mm4
    movq  [%9+0x40], mm0
    movq  [%9+0x50], mm3
%endmacro

; out: %4 = |%1-%2|>%3
; clobbers: %5
%macro DIFF_GT 6
    mov%1   %6, %3
    mov%1   %5, %2
    psubusb %6, %2
    psubusb %5, %3
    por     %5, %6
    psubusb %5, %4
%endmacro
%macro DIFF_GT_MMX 5
    DIFF_GT q, %1, %2, %3, %4, %5
%endmacro
%macro DIFF_GT_SSE2 5
    DIFF_GT dqa, %1, %2, %3, %4, %5
%endmacro

; out: %4 = |%1-%2|>%3
; clobbers: %5
%macro DIFF_GT2 6
    mov%1   %6, %3
    mov%1   %5, %2
    psubusb %6, %2
    psubusb %5, %3
    psubusb %6, %4
    psubusb %5, %4
    pcmpeqb %5, %6
%endmacro
%macro DIFF_GT2_MMX 5
    DIFF_GT2 q, %1, %2, %3, %4, %5
%endmacro
%macro DIFF_GT2_SSE2 5
    DIFF_GT2 dqa, %1, %2, %3, %4, %5
%endmacro

; in: mm0=p1 mm1=p0 mm2=q0 mm3=q1 %1=alpha-1 %2=beta-1
; out: mm5=beta-1, mm7=mask
; clobbers: mm4,mm6
%macro LOAD_MASK_MMX 2
    movd     mm4, %1
    movd     mm5, %2
    pshufw   mm4, mm4, 0
    pshufw   mm5, mm5, 0
    packuswb mm4, mm4  ; 8x alpha-1
    packuswb mm5, mm5  ; 8x beta-1
    DIFF_GT_MMX  mm1, mm2, mm4, mm7, mm6 ; |p0-q0| > alpha-1
    DIFF_GT_MMX  mm0, mm1, mm5, mm4, mm6 ; |p1-p0| > beta-1
    por      mm7, mm4
    DIFF_GT_MMX  mm3, mm2, mm5, mm4, mm6 ; |q1-q0| > beta-1
    por      mm7, mm4
    pxor     mm6, mm6
    pcmpeqb  mm7, mm6
%endmacro
%macro LOAD_MASK_SSE2 2
    movd     xmm4, %1
    movd     xmm5, %2
    pshuflw  xmm4, xmm4, 0
    pshuflw  xmm5, xmm5, 0
    punpcklqdq xmm4, xmm4
    punpcklqdq xmm5, xmm5
    packuswb xmm4, xmm4  ; 16x alpha-1
    packuswb xmm5, xmm5  ; 16x beta-1
    DIFF_GT_SSE2  xmm1, xmm2, xmm4, xmm7, xmm6 ; |p0-q0| > alpha-1
    DIFF_GT_SSE2  xmm0, xmm1, xmm5, xmm4, xmm6 ; |p1-p0| > beta-1
    por      xmm7, xmm4
    DIFF_GT_SSE2  xmm3, xmm2, xmm5, xmm4, xmm6 ; |q1-q0| > beta-1
    por      xmm7, xmm4
    pxor     xmm6, xmm6
    pcmpeqb  xmm7, xmm6
%endmacro

; in: mm0=p1 mm1=p0 mm2=q0 mm3=q1 mm7=(tc&mask)
; out: mm1=p0' mm2=q0'
; clobbers: mm0,3-6
%macro DEBLOCK_P0_Q0 2
    mov%1   %2m5, %2m1
    pxor    %2m5, %2m2           ; p0^q0
    pand    %2m5, [pb_01 GLOBAL] ; (p0^q0)&1
    pcmpeqb %2m4, %2m4
    pxor    %2m3, %2m4
    pavgb   %2m3, %2m0           ; (p1 - q1 + 256)>>1
    pavgb   %2m3, [pb_03 GLOBAL] ; (((p1 - q1 + 256)>>1)+4)>>1 = 64+2+(p1-q1)>>2
    pxor    %2m4, %2m1
    pavgb   %2m4, %2m2           ; (q0 - p0 + 256)>>1
    pavgb   %2m3, %2m5
    paddusb %2m3, %2m4           ; d+128+33
    mov%1   %2m6, [pb_a1 GLOBAL]
    psubusb %2m6, %2m3
    psubusb %2m3, [pb_a1 GLOBAL]
    pminub  %2m6, %2m7
    pminub  %2m3, %2m7
    psubusb %2m1, %2m6
    psubusb %2m2, %2m3
    paddusb %2m1, %2m3
    paddusb %2m2, %2m6
%endmacro
%macro DEBLOCK_P0_Q0_MMX 0
    DEBLOCK_P0_Q0 q, m
%endmacro
%macro DEBLOCK_P0_Q0_SSE2 0
    DEBLOCK_P0_Q0 dqa, xm
%endmacro

; in: mm1=p0 mm2=q0
;     %1=p1 %2=q2 %3=[q2] %4=[q1] %5=tc0 %6=tmp
; out: [q1] = clip( (q2+((p0+q0+1)>>1))>>1, q1-tc0, q1+tc0 )
; clobbers: q2, tmp, tc0
%macro LUMA_Q1_SSE2 6
    movdqa  %6, xmm1
    pavgb   %6, xmm2
    pavgb   %2, %6             ; avg(p2,avg(p0,q0))
    pxor    %6, %3
    pand    %6, [pb_01 GLOBAL] ; (p2^avg(p0,q0))&1
    psubusb %2, %6             ; (p2+((p0+q0+1)>>1))>>1
    movdqa  %6, %1
    psubusb %6, %5
    paddusb %5, %1
    pmaxub  %2, %6
    pminub  %2, %5
    movdqa  %4, %2
%endmacro


SECTION .text
;-----------------------------------------------------------------------------
;   void x264_deblock_v_luma_sse2( uint8_t *pix, int stride, int alpha, int beta, int8_t *tc0 )
;-----------------------------------------------------------------------------
cglobal x264_deblock_v_luma_sse2
    ; rdi = pix
    movsxd rsi, esi ; stride
    dec    edx      ; alpha-1
    dec    ecx      ; beta-1
    movd   xmm8, [r8] ; tc0
    mov    r8,  rdi
    sub    r8,  rsi
    sub    r8,  rsi
    sub    r8,  rsi ; pix-3*stride

    movdqa  xmm0, [r8+rsi]    ; p1
    movdqa  xmm1, [r8+2*rsi]  ; p0
    movdqa  xmm2, [rdi]       ; q0
    movdqa  xmm3, [rdi+rsi]   ; q1
    LOAD_MASK_SSE2  edx, ecx

    punpcklbw xmm8, xmm8
    punpcklbw xmm8, xmm8 ; xmm8 = 4x tc0[3], 4x tc0[2], 4x tc0[1], 4x tc0[0]
    pcmpeqb xmm9, xmm9
    pcmpeqb xmm9, xmm8
    pandn   xmm9, xmm7
    pand    xmm8, xmm9

    movdqa  xmm3, [r8] ; p2
    DIFF_GT2_SSE2  xmm1, xmm3, xmm5, xmm6, xmm7 ; |p2-p0| > beta-1
    pand    xmm6, xmm9
    movdqa  xmm7, xmm8
    psubb   xmm7, xmm6
    pand    xmm6, xmm8
    LUMA_Q1_SSE2  xmm0, xmm3, [r8], [r8+rsi], xmm6, xmm4

    movdqa  xmm4, [rdi+2*rsi] ; q2
    DIFF_GT2_SSE2  xmm2, xmm4, xmm5, xmm6, xmm3 ; |q2-q0| > beta-1
    pand    xmm6, xmm9
    pand    xmm8, xmm6
    psubb   xmm7, xmm6
    movdqa  xmm3, [rdi+rsi]
    LUMA_Q1_SSE2  xmm3, xmm4, [rdi+2*rsi], [rdi+rsi], xmm8, xmm6

    DEBLOCK_P0_Q0_SSE2
    movdqa  [r8+2*rsi], xmm1
    movdqa  [rdi], xmm2

    ret

;-----------------------------------------------------------------------------
;   void x264_deblock_h_luma_sse2( uint8_t *pix, int stride, int alpha, int beta, int8_t *tc0 )
;-----------------------------------------------------------------------------
cglobal x264_deblock_h_luma_sse2
    movsxd r10, esi
    lea    r11, [r10+r10*2]
    lea    rax, [rdi-4]
    lea    r9,  [rdi-4+r11]
    sub    rsp, 0x68
    %define pix_tmp rsp

    ; transpose 6x16 -> tmp space
    TRANSPOSE6x8_MEM  PASS8ROWS(rax, r9, r10, r11), pix_tmp
    lea    rax, [rax+r10*8]
    lea    r9,  [r9 +r10*8]
    TRANSPOSE6x8_MEM  PASS8ROWS(rax, r9, r10, r11), pix_tmp+8

    ; vertical filter
    ; alpha, beta, tc0 are still in edx, ecx, r8
    ; don't backup rax, r9, r10, r11 because x264_deblock_v_luma_sse2 doesn't use them
    lea    rdi, [pix_tmp+0x30]
    mov    esi, 0x10
    call   x264_deblock_v_luma_sse2

    ; transpose 16x4 -> original space  (only the middle 4 rows were changed by the filter)
    add    rax, 2
    add    r9,  2
    movq   mm0, [pix_tmp+0x18]
    movq   mm1, [pix_tmp+0x28]
    movq   mm2, [pix_tmp+0x38]
    movq   mm3, [pix_tmp+0x48]
    TRANSPOSE8x4_STORE  PASS8ROWS(rax, r9, r10, r11)

    shl    r10, 3
    sub    rax, r10
    sub    r9,  r10
    shr    r10, 3
    movq   mm0, [pix_tmp+0x10]
    movq   mm1, [pix_tmp+0x20]
    movq   mm2, [pix_tmp+0x30]
    movq   mm3, [pix_tmp+0x40]
    TRANSPOSE8x4_STORE  PASS8ROWS(rax, r9, r10, r11)

    add    rsp, 0x68
    ret


%macro CHROMA_V_START 0
    ; rdi = pix
    movsxd rsi, esi ; stride
    dec    edx      ; alpha-1
    dec    ecx      ; beta-1
    mov    rax, rdi
    sub    rax, rsi
    sub    rax, rsi
%endmacro

%macro CHROMA_H_START 0
    movsxd rsi, esi
    dec    edx
    dec    ecx
    sub    rdi, 2
    lea    r9, [rsi+rsi*2]
    mov    rax, rdi
    add    rdi, r9
%endmacro

;-----------------------------------------------------------------------------
;   void x264_deblock_v_chroma_mmxext( uint8_t *pix, int stride, int alpha, int beta, int8_t *tc0 )
;-----------------------------------------------------------------------------
cglobal x264_deblock_v_chroma_mmxext
    CHROMA_V_START

    movq  mm0, [rax]
    movq  mm1, [rax+rsi]
    movq  mm2, [rdi]
    movq  mm3, [rdi+rsi]

    LOAD_MASK_MMX  edx, ecx
    movd       mm6, [r8] ; tc0
    punpcklbw  mm6, mm6
    pand       mm7, mm6
    DEBLOCK_P0_Q0_MMX

    movq  [rax+rsi], mm1
    movq  [rdi], mm2
    ret


;-----------------------------------------------------------------------------
;   void x264_deblock_h_chroma_mmxext( uint8_t *pix, int stride, int alpha, int beta, int8_t *tc0 )
;-----------------------------------------------------------------------------
cglobal x264_deblock_h_chroma_mmxext
    CHROMA_H_START

    TRANSPOSE4x8_LOAD  PASS8ROWS(rax, rdi, rsi, r9)
    movq  [rsp-8], mm0
    movq  [rsp-16], mm3

    LOAD_MASK_MMX  edx, ecx
    movd       mm6, [r8] ; tc0
    punpcklbw  mm6, mm6
    pand       mm7, mm6
    DEBLOCK_P0_Q0_MMX

    movq  mm0, [rsp-8]
    movq  mm3, [rsp-16]
    TRANSPOSE8x4_STORE PASS8ROWS(rax, rdi, rsi, r9)
    ret


; in: %1=p0 %2=p1 %3=q1
; out: p0 = (p0 + q1 + 2*p1 + 2) >> 2
%macro CHROMA_INTRA_P0 3
    movq    mm4, %1
    pxor    mm4, %3
    pand    mm4, [pb_01 GLOBAL] ; mm4 = (p0^q1)&1
    pavgb   %1,  %3
    psubusb %1,  mm4
    pavgb   %1,  %2             ; dst = avg(p1, avg(p0,q1) - ((p0^q1)&1))
%endmacro

%macro CHROMA_INTRA_BODY 0
    LOAD_MASK_MMX edx, ecx
    movq   mm5, mm1
    movq   mm6, mm2
    CHROMA_INTRA_P0  mm1, mm0, mm3
    CHROMA_INTRA_P0  mm2, mm3, mm0
    psubb  mm1, mm5
    psubb  mm2, mm6
    pand   mm1, mm7
    pand   mm2, mm7
    paddb  mm1, mm5
    paddb  mm2, mm6
%endmacro

;-----------------------------------------------------------------------------
;   void x264_deblock_v_chroma_intra_mmxext( uint8_t *pix, int stride, int alpha, int beta )
;-----------------------------------------------------------------------------
cglobal x264_deblock_v_chroma_intra_mmxext
    CHROMA_V_START

    movq  mm0, [rax]
    movq  mm1, [rax+rsi]
    movq  mm2, [rdi]
    movq  mm3, [rdi+rsi]

    CHROMA_INTRA_BODY

    movq  [rax+rsi], mm1
    movq  [rdi], mm2
    ret

;-----------------------------------------------------------------------------
;   void x264_deblock_h_chroma_intra_mmxext( uint8_t *pix, int stride, int alpha, int beta )
;-----------------------------------------------------------------------------
cglobal x264_deblock_h_chroma_intra_mmxext
    CHROMA_H_START
    TRANSPOSE4x8_LOAD  PASS8ROWS(rax, rdi, rsi, r9)
    CHROMA_INTRA_BODY
    TRANSPOSE8x4_STORE PASS8ROWS(rax, rdi, rsi, r9)
    ret


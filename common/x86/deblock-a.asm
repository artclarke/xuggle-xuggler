;*****************************************************************************
;* deblock-a.asm: h264 encoder library
;*****************************************************************************
;* Copyright (C) 2005-2008 x264 project
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

%include "x86inc.asm"

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
; out: 4 rows of 8 bytes in m0..m3
%macro TRANSPOSE4x8_LOAD 8
    movd       m0, %1
    movd       m2, %2
    movd       m1, %3
    movd       m3, %4
    punpcklbw  m0, m2
    punpcklbw  m1, m3
    movq       m2, m0
    punpcklwd  m0, m1
    punpckhwd  m2, m1

    movd       m4, %5
    movd       m6, %6
    movd       m5, %7
    movd       m7, %8
    punpcklbw  m4, m6
    punpcklbw  m5, m7
    movq       m6, m4
    punpcklwd  m4, m5
    punpckhwd  m6, m5

    movq       m1, m0
    movq       m3, m2
    punpckldq  m0, m4
    punpckhdq  m1, m4
    punpckldq  m2, m6
    punpckhdq  m3, m6
%endmacro

; in: 4 rows of 8 bytes in m0..m3
; out: 8 rows of 4 bytes in %1..%8
%macro TRANSPOSE8x4_STORE 8
    movq       m4, m0
    movq       m5, m1
    movq       m6, m2
    punpckhdq  m4, m4
    punpckhdq  m5, m5
    punpckhdq  m6, m6

    punpcklbw  m0, m1
    punpcklbw  m2, m3
    movq       m1, m0
    punpcklwd  m0, m2
    punpckhwd  m1, m2
    movd       %1, m0
    punpckhdq  m0, m0
    movd       %2, m0
    movd       %3, m1
    punpckhdq  m1, m1
    movd       %4, m1

    punpckhdq  m3, m3
    punpcklbw  m4, m5
    punpcklbw  m6, m3
    movq       m5, m4
    punpcklwd  m4, m6
    punpckhwd  m5, m6
    movd       %5, m4
    punpckhdq  m4, m4
    movd       %6, m4
    movd       %7, m5
    punpckhdq  m5, m5
    movd       %8, m5
%endmacro

%macro SBUTTERFLY 4
    movq       %4, %2
    punpckl%1  %2, %3
    punpckh%1  %4, %3
%endmacro

; in: 8 rows of 8 (only the middle 6 pels are used) in %1..%8
; out: 6 rows of 8 in [%9+0*16] .. [%9+5*16]
%macro TRANSPOSE6x8_MEM 9
    movq  m0, %1
    movq  m1, %3
    movq  m2, %5
    movq  m3, %7
    SBUTTERFLY bw, m0, %2, m4
    SBUTTERFLY bw, m1, %4, m5
    SBUTTERFLY bw, m2, %6, m6
    movq  [%9+0x10], m5
    SBUTTERFLY bw, m3, %8, m7
    SBUTTERFLY wd, m0, m1, m5
    SBUTTERFLY wd, m2, m3, m1
    punpckhdq m0, m2
    movq  [%9+0x00], m0
    SBUTTERFLY wd, m4, [%9+0x10], m3
    SBUTTERFLY wd, m6, m7, m2
    SBUTTERFLY dq, m4, m6, m0
    SBUTTERFLY dq, m5, m1, m7
    punpckldq m3, m2
    movq  [%9+0x10], m5
    movq  [%9+0x20], m7
    movq  [%9+0x30], m4
    movq  [%9+0x40], m0
    movq  [%9+0x50], m3
%endmacro

; out: %4 = |%1-%2|>%3
; clobbers: %5
%macro DIFF_GT 5
    movq    %5, %2
    movq    %4, %1
    psubusb %5, %1
    psubusb %4, %2
    por     %4, %5
    psubusb %4, %3
%endmacro

; out: %4 = |%1-%2|>%3
; clobbers: %5
%macro DIFF_GT2 5
    movq    %5, %2
    movq    %4, %1
    psubusb %5, %1
    psubusb %4, %2
    psubusb %5, %3
    psubusb %4, %3
    pcmpeqb %4, %5
%endmacro

%macro SPLATW 1
%ifidn m0, xmm0
    pshuflw  %1, %1, 0
    punpcklqdq %1, %1
%else
    pshufw   %1, %1, 0
%endif
%endmacro

; in: m0=p1 m1=p0 m2=q0 m3=q1 %1=alpha-1 %2=beta-1
; out: m5=beta-1, m7=mask
; clobbers: m4,m6
%macro LOAD_MASK 2
    movd     m4, %1
    movd     m5, %2
    SPLATW   m4
    SPLATW   m5
    packuswb m4, m4  ; 16x alpha-1
    packuswb m5, m5  ; 16x beta-1
    DIFF_GT  m1, m2, m4, m7, m6 ; |p0-q0| > alpha-1
    DIFF_GT  m0, m1, m5, m4, m6 ; |p1-p0| > beta-1
    por      m7, m4
    DIFF_GT  m3, m2, m5, m4, m6 ; |q1-q0| > beta-1
    por      m7, m4
    pxor     m6, m6
    pcmpeqb  m7, m6
%endmacro

; in: m0=p1 m1=p0 m2=q0 m3=q1 m7=(tc&mask)
; out: m1=p0' m2=q0'
; clobbers: m0,3-6
%macro DEBLOCK_P0_Q0 0
    movq    m5, m1
    pxor    m5, m2           ; p0^q0
    pand    m5, [pb_01 GLOBAL] ; (p0^q0)&1
    pcmpeqb m4, m4
    pxor    m3, m4
    pavgb   m3, m0           ; (p1 - q1 + 256)>>1
    pavgb   m3, [pb_03 GLOBAL] ; (((p1 - q1 + 256)>>1)+4)>>1 = 64+2+(p1-q1)>>2
    pxor    m4, m1
    pavgb   m4, m2           ; (q0 - p0 + 256)>>1
    pavgb   m3, m5
    paddusb m3, m4           ; d+128+33
    movq    m6, [pb_a1 GLOBAL]
    psubusb m6, m3
    psubusb m3, [pb_a1 GLOBAL]
    pminub  m6, m7
    pminub  m3, m7
    psubusb m1, m6
    psubusb m2, m3
    paddusb m1, m3
    paddusb m2, m6
%endmacro

; in: m1=p0 m2=q0
;     %1=p1 %2=q2 %3=[q2] %4=[q1] %5=tc0 %6=tmp
; out: [q1] = clip( (q2+((p0+q0+1)>>1))>>1, q1-tc0, q1+tc0 )
; clobbers: q2, tmp, tc0
%macro LUMA_Q1 6
    movq    %6, m1
    pavgb   %6, m2
    pavgb   %2, %6             ; avg(p2,avg(p0,q0))
    pxor    %6, %3
    pand    %6, [pb_01 GLOBAL] ; (p2^avg(p0,q0))&1
    psubusb %2, %6             ; (p2+((p0+q0+1)>>1))>>1
    movq    %6, %1
    psubusb %6, %5
    paddusb %5, %1
    pmaxub  %2, %6
    pminub  %2, %5
    movq    %4, %2
%endmacro

;-----------------------------------------------------------------------------
; void x264_deblock_v_luma_sse2( uint8_t *pix, int stride, int alpha, int beta, int8_t *tc0 )
;-----------------------------------------------------------------------------
%ifdef ARCH_X86_64
INIT_XMM
cglobal x264_deblock_v_luma_sse2
    movd    m8, [r4] ; tc0
    lea     r4, [r1*3]
    dec     r2d        ; alpha-1
    neg     r4
    dec     r3d        ; beta-1
    add     r4, r0     ; pix-3*stride

    movdqa  m0, [r4+r1]   ; p1
    movdqa  m1, [r4+2*r1] ; p0
    movdqa  m2, [r0]      ; q0
    movdqa  m3, [r0+r1]   ; q1
    LOAD_MASK r2d, r3d

    punpcklbw m8, m8
    punpcklbw m8, m8 ; tc = 4x tc0[3], 4x tc0[2], 4x tc0[1], 4x tc0[0]
    pcmpeqb m9, m9
    pcmpeqb m9, m8
    pandn   m9, m7
    pand    m8, m9

    movdqa  m3, [r4] ; p2
    DIFF_GT2 m1, m3, m5, m6, m7 ; |p2-p0| > beta-1
    pand    m6, m9
    movdqa  m7, m8
    psubb   m7, m6
    pand    m6, m8
    LUMA_Q1 m0, m3, [r4], [r4+r1], m6, m4

    movdqa  m4, [r0+2*r1] ; q2
    DIFF_GT2 m2, m4, m5, m6, m3 ; |q2-q0| > beta-1
    pand    m6, m9
    pand    m8, m6
    psubb   m7, m6
    movdqa  m3, [r0+r1]
    LUMA_Q1 m3, m4, [r0+2*r1], [r0+r1], m8, m6

    DEBLOCK_P0_Q0
    movdqa  [r4+2*r1], m1
    movdqa  [r0], m2
    ret

;-----------------------------------------------------------------------------
; void x264_deblock_h_luma_sse2( uint8_t *pix, int stride, int alpha, int beta, int8_t *tc0 )
;-----------------------------------------------------------------------------
INIT_MMX
cglobal x264_deblock_h_luma_sse2
    movsxd r10, esi
    lea    r11, [r10+r10*2]
    lea    rax, [r0-4]
    lea    r9,  [r0-4+r11]
    sub    rsp, 0x68
    %define pix_tmp rsp

    ; transpose 6x16 -> tmp space
    TRANSPOSE6x8_MEM  PASS8ROWS(rax, r9, r10, r11), pix_tmp
    lea    rax, [rax+r10*8]
    lea    r9,  [r9 +r10*8]
    TRANSPOSE6x8_MEM  PASS8ROWS(rax, r9, r10, r11), pix_tmp+8

    ; vertical filter
    ; alpha, beta, tc0 are still in r2d, r3d, r4
    ; don't backup rax, r9, r10, r11 because x264_deblock_v_luma_sse2 doesn't use them
    lea    r0, [pix_tmp+0x30]
    mov    esi, 0x10
    call   x264_deblock_v_luma_sse2

    ; transpose 16x4 -> original space  (only the middle 4 rows were changed by the filter)
    add    rax, 2
    add    r9,  2
    movq   m0, [pix_tmp+0x18]
    movq   m1, [pix_tmp+0x28]
    movq   m2, [pix_tmp+0x38]
    movq   m3, [pix_tmp+0x48]
    TRANSPOSE8x4_STORE  PASS8ROWS(rax, r9, r10, r11)

    shl    r10, 3
    sub    rax, r10
    sub    r9,  r10
    shr    r10, 3
    movq   m0, [pix_tmp+0x10]
    movq   m1, [pix_tmp+0x20]
    movq   m2, [pix_tmp+0x30]
    movq   m3, [pix_tmp+0x40]
    TRANSPOSE8x4_STORE  PASS8ROWS(rax, r9, r10, r11)

    add    rsp, 0x68
    ret

%else

%macro DEBLOCK_LUMA 3
;-----------------------------------------------------------------------------
; void x264_deblock_v8_luma_mmxext( uint8_t *pix, int stride, int alpha, int beta, int8_t *tc0 )
;-----------------------------------------------------------------------------
cglobal x264_deblock_%2_luma_%1, 5,5,1
    lea     r4, [r1*3]
    dec     r2     ; alpha-1
    neg     r4
    dec     r3     ; beta-1
    add     r4, r0 ; pix-3*stride

    movq    m0, [r4+r1]   ; p1
    movq    m1, [r4+2*r1] ; p0
    movq    m2, [r0]      ; q0
    movq    m3, [r0+r1]   ; q1
    LOAD_MASK r2, r3

    mov     r3, r4m
%if %3 == 16
    mov     r2, esp
    and     esp, -16
    sub     esp, 32
%else
    sub     esp, 16
%endif

    movd    m4, [r3] ; tc0
    punpcklbw m4, m4
    punpcklbw m4, m4 ; tc = 4x tc0[3], 4x tc0[2], 4x tc0[1], 4x tc0[0]
    movq   [esp+%3], m4 ; tc
    pcmpeqb m3, m3
    pcmpgtb m4, m3
    pand    m4, m7
    movq   [esp], m4 ; mask

    movq    m3, [r4] ; p2
    DIFF_GT2 m1, m3, m5, m6, m7 ; |p2-p0| > beta-1
    pand    m6, m4
    pand    m4, [esp+%3] ; tc
    movq    m7, m4
    psubb   m7, m6
    pand    m6, m4
    LUMA_Q1 m0, m3, [r4], [r4+r1], m6, m4

    movq    m4, [r0+2*r1] ; q2
    DIFF_GT2 m2, m4, m5, m6, m3 ; |q2-q0| > beta-1
    movq    m5, [esp] ; mask
    pand    m6, m5
    movq    m5, [esp+%3] ; tc
    pand    m5, m6
    psubb   m7, m6
    movq    m3, [r0+r1]
    LUMA_Q1 m3, m4, [r0+2*r1], [r0+r1], m5, m6

    DEBLOCK_P0_Q0
    movq    [r4+2*r1], m1
    movq    [r0], m2

%if %3 == 16
    mov     esp, r2
%else
    add     esp, 16
%endif
    RET

;-----------------------------------------------------------------------------
; void x264_deblock_h_luma_mmxext( uint8_t *pix, int stride, int alpha, int beta, int8_t *tc0 )
;-----------------------------------------------------------------------------
INIT_MMX
cglobal x264_deblock_h_luma_%1, 0,6
    mov    r0, r0m
    mov    r3, r1m
    lea    r4, [r3*3]
    sub    r0, 4
    lea    r1, [r0+r4]
    SUB    esp, 0x6c
    lea    r5, [esp+12]
    and    r5, -16
%define pix_tmp r5

    ; transpose 6x16 -> tmp space
    TRANSPOSE6x8_MEM  PASS8ROWS(r0, r1, r3, r4), pix_tmp
    lea    r0, [r0+r3*8]
    lea    r1, [r1+r3*8]
    TRANSPOSE6x8_MEM  PASS8ROWS(r0, r1, r3, r4), pix_tmp+8

    ; vertical filter
    lea    r0, [pix_tmp+0x30]
    PUSH   dword r4m
    PUSH   dword r3m
    PUSH   dword r2m
    PUSH   dword 16
    PUSH   dword r0
    call   x264_deblock_%2_luma_%1
%ifidn %2, v8
    add    dword [esp   ], 8 ; pix_tmp+0x38
    add    dword [esp+16], 2 ; tc0+2
    call   x264_deblock_%2_luma_%1
%endif
    ADD    esp, 20

    ; transpose 16x4 -> original space  (only the middle 4 rows were changed by the filter)
    mov    r0, r0m
    sub    r0, 2
    lea    r1, [r0+r4]

    movq   m0, [pix_tmp+0x10]
    movq   m1, [pix_tmp+0x20]
    movq   m2, [pix_tmp+0x30]
    movq   m3, [pix_tmp+0x40]
    TRANSPOSE8x4_STORE  PASS8ROWS(r0, r1, r3, r4)

    lea    r0, [r0+r3*8]
    lea    r1, [r1+r3*8]
    movq   m0, [pix_tmp+0x18]
    movq   m1, [pix_tmp+0x28]
    movq   m2, [pix_tmp+0x38]
    movq   m3, [pix_tmp+0x48]
    TRANSPOSE8x4_STORE  PASS8ROWS(r0, r1, r3, r4)

    ADD    esp, 0x6c
    RET
%endmacro ; DEBLOCK_LUMA

INIT_MMX
DEBLOCK_LUMA mmxext, v8, 8
INIT_XMM
DEBLOCK_LUMA sse2, v, 16

%endif ; ARCH



INIT_MMX

%macro CHROMA_V_START 0
    dec    r2d      ; alpha-1
    dec    r3d      ; beta-1
    mov    t5, r0
    sub    t5, r1
    sub    t5, r1
%endmacro

%macro CHROMA_H_START 0
    dec    r2d
    dec    r3d
    sub    r0, 2
    lea    t6, [r1*3]
    mov    t5, r0
    add    r0, t6
%endmacro

%define t5 r5
%define t6 r6

;-----------------------------------------------------------------------------
; void x264_deblock_v_chroma_mmxext( uint8_t *pix, int stride, int alpha, int beta, int8_t *tc0 )
;-----------------------------------------------------------------------------
cglobal x264_deblock_v_chroma_mmxext, 5,6
    CHROMA_V_START

    movq  m0, [t5]
    movq  m1, [t5+r1]
    movq  m2, [r0]
    movq  m3, [r0+r1]

    LOAD_MASK  r2d, r3d
    movd       m6, [r4] ; tc0
    punpcklbw  m6, m6
    pand       m7, m6
    picgetgot r4
    DEBLOCK_P0_Q0

    movq  [t5+r1], m1
    movq  [r0], m2
    RET

;-----------------------------------------------------------------------------
; void x264_deblock_h_chroma_mmxext( uint8_t *pix, int stride, int alpha, int beta, int8_t *tc0 )
;-----------------------------------------------------------------------------
cglobal x264_deblock_h_chroma_mmxext, 5,7
%ifdef ARCH_X86_64
    %define buf0 [rsp-16]
    %define buf1 [rsp-8]
%else
    %define buf0 r0m
    %define buf1 r2m
%endif
    CHROMA_H_START

    TRANSPOSE4x8_LOAD  PASS8ROWS(t5, r0, r1, t6)
    movq  buf0, m0
    movq  buf1, m3

    LOAD_MASK  r2d, r3d
    movd       m6, [r4] ; tc0
    punpcklbw  m6, m6
    pand       m7, m6
    picgetgot r4
    DEBLOCK_P0_Q0

    movq  m0, buf0
    movq  m3, buf1
    TRANSPOSE8x4_STORE PASS8ROWS(t5, r0, r1, t6)
    RET



; in: %1=p0 %2=p1 %3=q1
; out: p0 = (p0 + q1 + 2*p1 + 2) >> 2
%macro CHROMA_INTRA_P0 3
    movq    m4, %1
    pxor    m4, %3
    pand    m4, [pb_01 GLOBAL] ; m4 = (p0^q1)&1
    pavgb   %1, %3
    psubusb %1, m4
    pavgb   %1, %2             ; dst = avg(p1, avg(p0,q1) - ((p0^q1)&1))
%endmacro

%macro CHROMA_INTRA_BODY 0
    LOAD_MASK r2d, r3d
    movq   m5, m1
    movq   m6, m2
    CHROMA_INTRA_P0  m1, m0, m3
    CHROMA_INTRA_P0  m2, m3, m0
    psubb  m1, m5
    psubb  m2, m6
    pand   m1, m7
    pand   m2, m7
    paddb  m1, m5
    paddb  m2, m6
%endmacro

%define t5 r4
%define t6 r5

;-----------------------------------------------------------------------------
; void x264_deblock_v_chroma_intra_mmxext( uint8_t *pix, int stride, int alpha, int beta )
;-----------------------------------------------------------------------------
cglobal x264_deblock_v_chroma_intra_mmxext, 4,5,1
    CHROMA_V_START

    movq  m0, [t5]
    movq  m1, [t5+r1]
    movq  m2, [r0]
    movq  m3, [r0+r1]

    CHROMA_INTRA_BODY

    movq  [t5+r1], m1
    movq  [r0], m2
    RET

;-----------------------------------------------------------------------------
; void x264_deblock_h_chroma_intra_mmxext( uint8_t *pix, int stride, int alpha, int beta )
;-----------------------------------------------------------------------------
cglobal x264_deblock_h_chroma_intra_mmxext, 4,6,1
    CHROMA_H_START
    TRANSPOSE4x8_LOAD  PASS8ROWS(t5, r0, r1, t6)
    CHROMA_INTRA_BODY
    TRANSPOSE8x4_STORE PASS8ROWS(t5, r0, r1, t6)
    RET


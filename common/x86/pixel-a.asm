;*****************************************************************************
;* pixel.asm: h264 encoder library
;*****************************************************************************
;* Copyright (C) 2003-2008 x264 project
;*
;* Authors: Loren Merritt <lorenm@u.washington.edu>
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
;*****************************************************************************

%include "x86inc.asm"

SECTION_RODATA
pw_1:    times 8 dw 1
ssim_c1: times 4 dd 416    ; .01*.01*255*255*64
ssim_c2: times 4 dd 235963 ; .03*.03*255*255*64*63
mask_ff: times 16 db 0xff
         times 16 db 0

SECTION .text

%macro HADDD 2 ; sum junk
    movhlps %2, %1
    paddd   %1, %2
    pshuflw %2, %1, 0xE
    paddd   %1, %2
%endmacro

%macro HADDW 2
    pmaddwd %1, [pw_1 GLOBAL]
    HADDD   %1, %2
%endmacro

;=============================================================================
; SSD
;=============================================================================

%macro SSD_INC_1x16P 0
    movq    mm1,    [r0]
    movq    mm2,    [r2]
    movq    mm3,    [r0+8]
    movq    mm4,    [r2+8]

    movq    mm5,    mm2
    movq    mm6,    mm4
    psubusb mm2,    mm1
    psubusb mm4,    mm3
    psubusb mm1,    mm5
    psubusb mm3,    mm6
    por     mm1,    mm2
    por     mm3,    mm4

    movq    mm2,    mm1
    movq    mm4,    mm3
    punpcklbw mm1,  mm7
    punpcklbw mm3,  mm7
    punpckhbw mm2,  mm7
    punpckhbw mm4,  mm7
    pmaddwd mm1,    mm1
    pmaddwd mm2,    mm2
    pmaddwd mm3,    mm3
    pmaddwd mm4,    mm4

    add     r0,     r1
    add     r2,     r3
    paddd   mm0,    mm1
    paddd   mm0,    mm2
    paddd   mm0,    mm3
    paddd   mm0,    mm4
%endmacro

%macro SSD_INC_2x16P 0
    SSD_INC_1x16P
    SSD_INC_1x16P
%endmacro

%macro SSD_INC_2x8P 0
    movq    mm1,    [r0]
    movq    mm2,    [r2]
    movq    mm3,    [r0+r1]
    movq    mm4,    [r2+r3]

    movq    mm5,    mm2
    movq    mm6,    mm4
    psubusb mm2,    mm1
    psubusb mm4,    mm3
    psubusb mm1,    mm5
    psubusb mm3,    mm6
    por     mm1,    mm2
    por     mm3,    mm4

    movq    mm2,    mm1
    movq    mm4,    mm3
    punpcklbw mm1,  mm7
    punpcklbw mm3,  mm7
    punpckhbw mm2,  mm7
    punpckhbw mm4,  mm7
    pmaddwd mm1,    mm1
    pmaddwd mm2,    mm2
    pmaddwd mm3,    mm3
    pmaddwd mm4,    mm4

    lea     r0,     [r0+2*r1]
    lea     r2,     [r2+2*r3]
    paddd   mm0,    mm1
    paddd   mm0,    mm2
    paddd   mm0,    mm3
    paddd   mm0,    mm4
%endmacro

%macro SSD_INC_2x4P 0
    movd      mm1, [r0]
    movd      mm2, [r2]
    movd      mm3, [r0+r1]
    movd      mm4, [r2+r3]

    punpcklbw mm1, mm7
    punpcklbw mm2, mm7
    punpcklbw mm3, mm7
    punpcklbw mm4, mm7
    psubw     mm1, mm2
    psubw     mm3, mm4
    pmaddwd   mm1, mm1
    pmaddwd   mm3, mm3

    lea       r0,  [r0+2*r1]
    lea       r2,  [r2+2*r3]
    paddd     mm0, mm1
    paddd     mm0, mm3
%endmacro

;-----------------------------------------------------------------------------
; int x264_pixel_ssd_16x16_mmx (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
%macro SSD_MMX 2
cglobal x264_pixel_ssd_%1x%2_mmx, 4,4
    pxor    mm7,    mm7         ; zero
    pxor    mm0,    mm0         ; mm0 holds the sum
%rep %2/2
    SSD_INC_2x%1P
%endrep
    movq    mm1,    mm0
    psrlq   mm1,    32
    paddd   mm0,    mm1
    movd    eax,    mm0
    RET
%endmacro

SSD_MMX 16, 16
SSD_MMX 16,  8
SSD_MMX  8, 16
SSD_MMX  8,  8
SSD_MMX  8,  4
SSD_MMX  4,  8
SSD_MMX  4,  4

%macro SSD_INC_2x16P_SSE2 0
    movdqa  xmm1,   [r0]
    movdqa  xmm2,   [r2]
    movdqa  xmm3,   [r0+r1]
    movdqa  xmm4,   [r2+r3]

    movdqa  xmm5,   xmm1
    movdqa  xmm6,   xmm3
    psubusb xmm1,   xmm2
    psubusb xmm3,   xmm4
    psubusb xmm2,   xmm5
    psubusb xmm4,   xmm6
    por     xmm1,   xmm2
    por     xmm3,   xmm4

    movdqa  xmm2,   xmm1
    movdqa  xmm4,   xmm3
    punpcklbw xmm1, xmm7
    punpckhbw xmm2, xmm7
    punpcklbw xmm3, xmm7
    punpckhbw xmm4, xmm7
    pmaddwd xmm1,   xmm1
    pmaddwd xmm2,   xmm2
    pmaddwd xmm3,   xmm3
    pmaddwd xmm4,   xmm4

    lea     r0,     [r0+2*r1]
    lea     r2,     [r2+2*r3]

    paddd   xmm1,   xmm2
    paddd   xmm3,   xmm4
    paddd   xmm0,   xmm1
    paddd   xmm0,   xmm3
%endmacro

%macro SSD_INC_2x8P_SSE2 0
    movq      xmm1, [r0]
    movq      xmm2, [r2]
    movq      xmm3, [r0+r1]
    movq      xmm4, [r2+r3]
    
    punpcklbw xmm1,xmm7
    punpcklbw xmm2,xmm7
    punpcklbw xmm3,xmm7
    punpcklbw xmm4,xmm7
    psubw     xmm1,xmm2
    psubw     xmm3,xmm4
    pmaddwd   xmm1,xmm1
    pmaddwd   xmm3,xmm3
    
    lea       r0, [r0+r1*2]
    lea       r2, [r2+r3*2]
    paddd     xmm0, xmm1
    paddd     xmm0, xmm3
%endmacro

;-----------------------------------------------------------------------------
; int x264_pixel_ssd_16x16_sse2 (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
%macro SSD_SSE2 2
cglobal x264_pixel_ssd_%1x%2_sse2, 4,4
    pxor    xmm7,   xmm7
    pxor    xmm0,   xmm0
%rep %2/2
    SSD_INC_2x%1P_SSE2
%endrep
    HADDD   xmm0,   xmm1
    movd    eax,    xmm0
    RET
%endmacro

SSD_SSE2 16, 16
SSD_SSE2 16, 8
SSD_SSE2 8, 16
SSD_SSE2 8, 8
SSD_SSE2 8, 4



;=============================================================================
; SATD
;=============================================================================

%macro LOAD_DIFF_4P 4  ; dst, tmp, [pix1], [pix2]
    movd       %1, %3
    movd       %2, %4
    punpcklbw  %1, %2
    punpcklbw  %2, %2
    psubw      %1, %2
%endmacro

%macro LOAD_DIFF_8P 4  ; dst, tmp, [pix1], [pix2]
    movq       %1, %3
    movq       %2, %4
    punpcklbw  %1, %2
    punpcklbw  %2, %2
    psubw      %1, %2
%endmacro

%macro LOAD_DIFF_8x4P 6 ; 4x dest, 2x temp
    LOAD_DIFF_8P %1, %5, [r0],      [r2]
    LOAD_DIFF_8P %2, %6, [r0+r1],   [r2+r3]
    LOAD_DIFF_8P %3, %5, [r0+2*r1], [r2+2*r3]
    LOAD_DIFF_8P %4, %6, [r0+r4],   [r2+r5]
%endmacro

; phaddw is used only in 4x4 hadamard, because in 8x8 it's slower:
; even on Penryn, phaddw has latency 3 while paddw and punpck* have 1.
; 4x4 is special in that 4x4 transpose in xmmregs takes extra munging,
; whereas phaddw-based transform doesn't care what order the coefs end up in.

%macro PHSUMSUB 3
    movdqa  %3, %1
    phaddw  %1, %2
    phsubw  %3, %2
%endmacro

%macro HADAMARD4_ROW_PHADD 5  ; abcd-t -> adtc
    PHSUMSUB  %1, %2, %5
    PHSUMSUB  %3, %4, %2
    PHSUMSUB  %1, %3, %4
    PHSUMSUB  %5, %2, %3
%endmacro

%macro SUMSUB_BADC 4
    paddw  %1, %2
    paddw  %3, %4
    paddw  %2, %2
    paddw  %4, %4
    psubw  %2, %1
    psubw  %4, %3
%endmacro

%macro HADAMARD4_1D 4
    SUMSUB_BADC %1, %2, %3, %4
    SUMSUB_BADC %1, %3, %2, %4
%endmacro

%macro HADAMARD8_1D 8
    SUMSUB_BADC %1, %5, %2, %6
    SUMSUB_BADC %3, %7, %4, %8
    SUMSUB_BADC %1, %3, %2, %4
    SUMSUB_BADC %5, %7, %6, %8
    SUMSUB_BADC %1, %2, %3, %4
    SUMSUB_BADC %5, %6, %7, %8
%endmacro

%macro SBUTTERFLY 5
    mov%1       %5, %3
    punpckl%2   %3, %4
    punpckh%2   %5, %4
%endmacro

%macro SBUTTERFLY2 5  ; not really needed, but allows transpose4x4x2 to not shuffle registers
    mov%1       %5, %3
    punpckh%2   %3, %4
    punpckl%2   %5, %4
%endmacro

%macro TRANSPOSE4x4W 5   ; abcd-t -> adtc
    SBUTTERFLY q, wd, %1, %2, %5
    SBUTTERFLY q, wd, %3, %4, %2
    SBUTTERFLY q, dq, %1, %3, %4
    SBUTTERFLY q, dq, %5, %2, %3
%endmacro

%macro TRANSPOSE4x4D 5   ; abcd-t -> adtc
    SBUTTERFLY dqa, dq,  %1, %2, %5
    SBUTTERFLY dqa, dq,  %3, %4, %2
    SBUTTERFLY dqa, qdq, %1, %3, %4
    SBUTTERFLY dqa, qdq, %5, %2, %3
%endmacro

%macro TRANSPOSE2x4x4W 5   ; abcd-t -> abcd
    SBUTTERFLY  dqa, wd,  %1, %2, %5
    SBUTTERFLY  dqa, wd,  %3, %4, %2
    SBUTTERFLY  dqa, dq,  %1, %3, %4
    SBUTTERFLY2 dqa, dq,  %5, %2, %3
    SBUTTERFLY  dqa, qdq, %1, %3, %2
    SBUTTERFLY2 dqa, qdq, %4, %5, %3
%endmacro

%ifdef ARCH_X86_64
%macro TRANSPOSE8x8W 9   ; abcdefgh-t -> afhdtecb
    SBUTTERFLY dqa, wd,  %1, %2, %9
    SBUTTERFLY dqa, wd,  %3, %4, %2
    SBUTTERFLY dqa, wd,  %5, %6, %4
    SBUTTERFLY dqa, wd,  %7, %8, %6
    SBUTTERFLY dqa, dq,  %1, %3, %8
    SBUTTERFLY dqa, dq,  %9, %2, %3
    SBUTTERFLY dqa, dq,  %5, %7, %2
    SBUTTERFLY dqa, dq,  %4, %6, %7
    SBUTTERFLY dqa, qdq, %1, %5, %6
    SBUTTERFLY dqa, qdq, %9, %4, %5
    SBUTTERFLY dqa, qdq, %8, %2, %4
    SBUTTERFLY dqa, qdq, %3, %7, %2
%endmacro
%else
%macro TRANSPOSE8x8W 9   ; abcdefgh -> afhdgecb
    movdqa [%9], %8
    SBUTTERFLY dqa, wd,  %1, %2, %8
    movdqa [%9+16], %8
    movdqa %8, [%9]
    SBUTTERFLY dqa, wd,  %3, %4, %2
    SBUTTERFLY dqa, wd,  %5, %6, %4
    SBUTTERFLY dqa, wd,  %7, %8, %6
    SBUTTERFLY dqa, dq,  %1, %3, %8
    movdqa [%9], %8
    movdqa %8, [16+%9]
    SBUTTERFLY dqa, dq,  %8, %2, %3
    SBUTTERFLY dqa, dq,  %5, %7, %2
    SBUTTERFLY dqa, dq,  %4, %6, %7
    SBUTTERFLY dqa, qdq, %1, %5, %6
    SBUTTERFLY dqa, qdq, %8, %4, %5
    movdqa [%9+16], %8
    movdqa %8, [%9]
    SBUTTERFLY dqa, qdq, %8, %2, %4
    SBUTTERFLY dqa, qdq, %3, %7, %2
    movdqa %7, [%9+16]
%endmacro
%endif

%macro ABS1_MMX 2    ; a, tmp
    pxor    %2, %2
    psubw   %2, %1
    pmaxsw  %1, %2
%endmacro

%macro ABS2_MMX 4    ; a, b, tmp0, tmp1
    pxor    %3, %3
    pxor    %4, %4
    psubw   %3, %1
    psubw   %4, %2
    pmaxsw  %1, %3
    pmaxsw  %2, %4
%endmacro

%macro ABS1_SSSE3 2
    pabsw   %1, %1
%endmacro

%macro ABS2_SSSE3 4
    pabsw   %1, %1
    pabsw   %2, %2
%endmacro

%define ABS1 ABS1_MMX
%define ABS2 ABS2_MMX

%macro ABS4 6
    ABS2 %1, %2, %5, %6
    ABS2 %3, %4, %5, %6
%endmacro

%macro HADAMARD4x4_SUM 1    ; %1 = dest (row sum of one block)
    HADAMARD4_1D  mm4, mm5, mm6, mm7
    TRANSPOSE4x4W mm4, mm5, mm6, mm7, %1
    HADAMARD4_1D  mm4, mm7, %1,  mm6
    ABS2          mm4, mm7, mm3, mm5
    ABS2          %1,  mm6, mm3, mm5
    paddw         %1,  mm4
    paddw         mm6, mm7
    pavgw         %1,  mm6
%endmacro

; in: r4=3*stride1, r5=3*stride2
; in: %2 = horizontal offset
; in: %3 = whether we need to increment pix1 and pix2
; clobber: mm3..mm7
; out: %1 = satd
%macro SATD_4x4_MMX 3
    LOAD_DIFF_4P mm4, mm3, [r0+%2],      [r2+%2]
    LOAD_DIFF_4P mm5, mm3, [r0+r1+%2],   [r2+r3+%2]
    LOAD_DIFF_4P mm6, mm3, [r0+2*r1+%2], [r2+2*r3+%2]
    LOAD_DIFF_4P mm7, mm3, [r0+r4+%2],   [r2+r5+%2]
%if %3
    lea  r0, [r0+4*r1]
    lea  r2, [r2+4*r3]
%endif
    HADAMARD4x4_SUM %1
%endmacro

%macro SATD_8x4_START 1
    SATD_4x4_MMX mm0, 0, 0
    SATD_4x4_MMX mm1, 4, %1
%endmacro

%macro SATD_8x4_INC 1
    SATD_4x4_MMX mm2, 0, 0
    paddw        mm0, mm1
    SATD_4x4_MMX mm1, 4, %1
    paddw        mm0, mm2
%endmacro

%macro SATD_16x4_START 1
    SATD_4x4_MMX mm0,  0, 0
    SATD_4x4_MMX mm1,  4, 0
    SATD_4x4_MMX mm2,  8, 0
    paddw        mm0, mm1
    SATD_4x4_MMX mm1, 12, %1
    paddw        mm0, mm2
%endmacro

%macro SATD_16x4_INC 1
    SATD_4x4_MMX mm2,  0, 0
    paddw        mm0, mm1
    SATD_4x4_MMX mm1,  4, 0
    paddw        mm0, mm2
    SATD_4x4_MMX mm2,  8, 0
    paddw        mm0, mm1
    SATD_4x4_MMX mm1, 12, %1
    paddw        mm0, mm2
%endmacro

%macro SATD_8x4_SSE2 1
    LOAD_DIFF_8x4P    xmm0, xmm1, xmm2, xmm3, xmm4, xmm5
%if %1
    lea  r0, [r0+4*r1]
    lea  r2, [r2+4*r3]
%endif
    HADAMARD4_1D    xmm0, xmm1, xmm2, xmm3
    TRANSPOSE2x4x4W xmm0, xmm1, xmm2, xmm3, xmm4
    HADAMARD4_1D    xmm0, xmm1, xmm2, xmm3
    ABS4            xmm0, xmm1, xmm2, xmm3, xmm4, xmm5
    paddusw  xmm0, xmm1
    paddusw  xmm2, xmm3
    paddusw  xmm6, xmm0
    paddusw  xmm6, xmm2
%endmacro

%macro SATD_8x4_PHADD 1
    LOAD_DIFF_8x4P    xmm0, xmm1, xmm2, xmm3, xmm4, xmm5
%if %1
    lea  r0, [r0+4*r1]
    lea  r2, [r2+4*r3]
%endif
    HADAMARD4_1D    xmm0, xmm1, xmm2, xmm3
    HADAMARD4_ROW_PHADD xmm0, xmm1, xmm2, xmm3, xmm4
    ABS4            xmm0, xmm3, xmm4, xmm2, xmm1, xmm5
    paddusw  xmm0, xmm3
    paddusw  xmm2, xmm4
    paddusw  xmm6, xmm0
    paddusw  xmm6, xmm2
%endmacro

%macro SATD_START_MMX 0
    lea  r4, [3*r1] ; 3*stride1
    lea  r5, [3*r3] ; 3*stride2
%endmacro

%macro SATD_END_MMX 0
    pshufw      mm1, mm0, 01001110b
    paddw       mm0, mm1
    pshufw      mm1, mm0, 10110001b
    paddw       mm0, mm1
    movd        eax, mm0
    and         eax, 0xffff
    RET
%endmacro

; FIXME avoid the spilling of regs to hold 3*stride.
; for small blocks on x86_32, modify pixel pointer instead.

;-----------------------------------------------------------------------------
; int x264_pixel_satd_16x16_mmxext (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
cglobal x264_pixel_satd_16x16_mmxext, 4,6
    SATD_START_MMX
    SATD_16x4_START 1
    SATD_16x4_INC 1
    SATD_16x4_INC 1
    SATD_16x4_INC 0
    paddw       mm0, mm1
    pxor        mm3, mm3
    pshufw      mm1, mm0, 01001110b
    paddw       mm0, mm1
    punpcklwd   mm0, mm3
    pshufw      mm1, mm0, 01001110b
    paddd       mm0, mm1
    movd        eax, mm0
    RET

cglobal x264_pixel_satd_16x8_mmxext, 4,6
    SATD_START_MMX
    SATD_16x4_START 1
    SATD_16x4_INC 0
    paddw       mm0, mm1
    SATD_END_MMX

cglobal x264_pixel_satd_8x16_mmxext, 4,6
    SATD_START_MMX
    SATD_8x4_START 1
    SATD_8x4_INC 1
    SATD_8x4_INC 1
    SATD_8x4_INC 0
    paddw       mm0, mm1
    SATD_END_MMX

cglobal x264_pixel_satd_8x8_mmxext, 4,6
    SATD_START_MMX
    SATD_8x4_START 1
    SATD_8x4_INC 0
    paddw       mm0, mm1
    SATD_END_MMX

cglobal x264_pixel_satd_8x4_mmxext, 4,6
    SATD_START_MMX
    SATD_8x4_START 0
    paddw       mm0, mm1
    SATD_END_MMX

%macro SATD_W4 1
cglobal x264_pixel_satd_4x8_%1, 4,6
    SATD_START_MMX
    SATD_4x4_MMX mm0, 0, 1
    SATD_4x4_MMX mm1, 0, 0
    paddw       mm0, mm1
    SATD_END_MMX

cglobal x264_pixel_satd_4x4_%1, 4,6
    SATD_START_MMX
    SATD_4x4_MMX mm0, 0, 0
    SATD_END_MMX
%endmacro

SATD_W4 mmxext

%macro SATD_START_SSE2 0
    pxor    xmm6, xmm6
    lea     r4,   [3*r1]
    lea     r5,   [3*r3]
%endmacro

%macro SATD_END_SSE2 0
    picgetgot ebx
    psrlw   xmm6, 1
    HADDW   xmm6, xmm7
    movd    eax,  xmm6
    RET
%endmacro

%macro BACKUP_POINTERS 0
%ifdef ARCH_X86_64
    mov     r10, r0
    mov     r11, r2
%endif
%endmacro

%macro RESTORE_AND_INC_POINTERS 0
%ifdef ARCH_X86_64
    lea     r0, [r10+8]
    lea     r2, [r11+8]
%else
    mov     r0, r0m
    mov     r2, r2m
    add     r0, 8
    add     r2, 8
%endif
%endmacro

;-----------------------------------------------------------------------------
; int x264_pixel_satd_8x4_sse2 (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
%macro SATDS_SSE2 1
cglobal x264_pixel_satd_16x16_%1, 4,6
    SATD_START_SSE2
    BACKUP_POINTERS
    SATD_8x4_SSE2 1
    SATD_8x4_SSE2 1
    SATD_8x4_SSE2 1
    SATD_8x4_SSE2 0
    RESTORE_AND_INC_POINTERS
    SATD_8x4_SSE2 1
    SATD_8x4_SSE2 1
    SATD_8x4_SSE2 1
    SATD_8x4_SSE2 0
    SATD_END_SSE2

cglobal x264_pixel_satd_16x8_%1, 4,6
    SATD_START_SSE2
    BACKUP_POINTERS
    SATD_8x4_SSE2 1
    SATD_8x4_SSE2 0
    RESTORE_AND_INC_POINTERS
    SATD_8x4_SSE2 1
    SATD_8x4_SSE2 0
    SATD_END_SSE2

cglobal x264_pixel_satd_8x16_%1, 4,6
    SATD_START_SSE2
    SATD_8x4_SSE2 1
    SATD_8x4_SSE2 1
    SATD_8x4_SSE2 1
    SATD_8x4_SSE2 0
    SATD_END_SSE2

cglobal x264_pixel_satd_8x8_%1, 4,6
    SATD_START_SSE2
    SATD_8x4_SSE2 1
    SATD_8x4_SSE2 0
    SATD_END_SSE2

cglobal x264_pixel_satd_8x4_%1, 4,6
    SATD_START_SSE2
    SATD_8x4_SSE2 0
    SATD_END_SSE2

%ifdef ARCH_X86_64
;-----------------------------------------------------------------------------
; int x264_pixel_sa8d_8x8_sse2( uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
cglobal x264_pixel_sa8d_8x8_%1
    lea  r4, [3*r1]
    lea  r5, [3*r3]
.skip_lea:
    LOAD_DIFF_8x4P xmm0, xmm1, xmm2, xmm3, xmm8, xmm9
    lea  r0, [r0+4*r1]
    lea  r2, [r2+4*r3]
    LOAD_DIFF_8x4P xmm4, xmm5, xmm6, xmm7, xmm8, xmm9

    HADAMARD8_1D  xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7
    TRANSPOSE8x8W xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm8
    HADAMARD8_1D  xmm0, xmm5, xmm7, xmm3, xmm8, xmm4, xmm2, xmm1

    ABS4 xmm0, xmm1, xmm2, xmm3, xmm6, xmm9
    ABS4 xmm4, xmm5, xmm7, xmm8, xmm6, xmm9
    paddusw  xmm0, xmm1
    paddusw  xmm2, xmm3
    paddusw  xmm4, xmm5
    paddusw  xmm7, xmm8
    paddusw  xmm0, xmm2
    paddusw  xmm4, xmm7
    pavgw    xmm0, xmm4
    HADDW    xmm0, xmm1
    movd eax, xmm0
    add r10d, eax ; preserve rounding for 16x16
    add eax, 1
    shr eax, 1
    ret

cglobal x264_pixel_sa8d_16x16_%1
    xor  r10d, r10d
    call x264_pixel_sa8d_8x8_%1 ; pix[0]
    lea  r0, [r0+4*r1]
    lea  r2, [r2+4*r3]
    call x264_pixel_sa8d_8x8_%1.skip_lea ; pix[8*stride]
    neg  r4 ; it's already r1*3
    neg  r5
    lea  r0, [r0+4*r4+8]
    lea  r2, [r2+4*r5+8]
    call x264_pixel_sa8d_8x8_%1 ; pix[8]
    lea  r0, [r0+4*r1]
    lea  r2, [r2+4*r3]
    call x264_pixel_sa8d_8x8_%1.skip_lea ; pix[8*stride+8]
    mov  eax, r10d
    add  eax, 1
    shr  eax, 1
    ret
%else ; ARCH_X86_32
cglobal x264_pixel_sa8d_8x8_%1, 4,7
    mov  r6, esp
    and  esp, ~15
    sub  esp, 32
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    LOAD_DIFF_8x4P xmm0, xmm1, xmm2, xmm3, xmm6, xmm7
    movdqa [esp], xmm2
    lea  r0, [r0+4*r1]
    lea  r2, [r2+4*r3]
    LOAD_DIFF_8x4P xmm4, xmm5, xmm6, xmm7, xmm2, xmm2
    movdqa xmm2, [esp]

    HADAMARD8_1D  xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7
    TRANSPOSE8x8W xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, esp
    HADAMARD8_1D  xmm0, xmm5, xmm7, xmm3, xmm6, xmm4, xmm2, xmm1

%ifidn %1, sse2
    movdqa [esp], xmm6
    movdqa [esp+16], xmm7
%endif
    ABS2 xmm2, xmm3, xmm6, xmm7
    ABS2 xmm0, xmm1, xmm6, xmm7
    paddusw xmm0, xmm2
    paddusw xmm1, xmm3
%ifidn %1, sse2
    movdqa xmm6, [esp]
    movdqa xmm7, [esp+16]
%endif
    ABS2 xmm4, xmm5, xmm2, xmm3
    ABS2 xmm6, xmm7, xmm2, xmm3
    paddusw xmm4, xmm5
    paddusw xmm6, xmm7
    paddusw xmm0, xmm1
    paddusw xmm4, xmm6
    pavgw   xmm0, xmm4
    picgetgot ebx
    HADDW   xmm0, xmm1
    movd eax, xmm0
    mov  ecx, eax ; preserve rounding for 16x16
    add  eax, 1
    shr  eax, 1
    mov  esp, r6
    RET
%endif ; ARCH
%endmacro ; SATDS_SSE2

%macro SA8D_16x16_32 1
%ifndef ARCH_X86_64
cglobal x264_pixel_sa8d_16x16_%1
    push   ebp
    push   dword [esp+20]   ; stride2
    push   dword [esp+20]   ; pix2
    push   dword [esp+20]   ; stride1
    push   dword [esp+20]   ; pix1
    call x264_pixel_sa8d_8x8_%1
    mov    ebp, ecx
    add    dword [esp+0], 8 ; pix1+8
    add    dword [esp+8], 8 ; pix2+8
    call x264_pixel_sa8d_8x8_%1
    add    ebp, ecx
    mov    eax, [esp+4]
    mov    edx, [esp+12]
    shl    eax, 3
    shl    edx, 3
    add    [esp+0], eax     ; pix1+8*stride1+8
    add    [esp+8], edx     ; pix2+8*stride2+8
    call x264_pixel_sa8d_8x8_%1
    add    ebp, ecx
    sub    dword [esp+0], 8 ; pix1+8*stride1
    sub    dword [esp+8], 8 ; pix2+8*stride2
    call x264_pixel_sa8d_8x8_%1
    lea    eax, [ebp+ecx+1]
    shr    eax, 1
    add    esp, 16
    pop    ebp
    ret
%endif ; !ARCH_X86_64
%endmacro ; SA8D_16x16_32



;=============================================================================
; INTRA SATD
;=============================================================================

%macro INTRA_SA8D_SSE2 1
%ifdef ARCH_X86_64
;-----------------------------------------------------------------------------
; void x264_intra_sa8d_x3_8x8_core_sse2( uint8_t *fenc, int16_t edges[2][8], int *res )
;-----------------------------------------------------------------------------
cglobal x264_intra_sa8d_x3_8x8_core_%1
    ; 8x8 hadamard
    pxor        xmm4, xmm4
    movq        xmm0, [r0+0*FENC_STRIDE]
    movq        xmm7, [r0+1*FENC_STRIDE]
    movq        xmm6, [r0+2*FENC_STRIDE]
    movq        xmm3, [r0+3*FENC_STRIDE]
    movq        xmm5, [r0+4*FENC_STRIDE]
    movq        xmm1, [r0+5*FENC_STRIDE]
    movq        xmm8, [r0+6*FENC_STRIDE]
    movq        xmm2, [r0+7*FENC_STRIDE]
    punpcklbw   xmm0, xmm4
    punpcklbw   xmm7, xmm4
    punpcklbw   xmm6, xmm4
    punpcklbw   xmm3, xmm4
    punpcklbw   xmm5, xmm4
    punpcklbw   xmm1, xmm4
    punpcklbw   xmm8, xmm4
    punpcklbw   xmm2, xmm4
    HADAMARD8_1D  xmm0, xmm7, xmm6, xmm3, xmm5, xmm1, xmm8, xmm2
    TRANSPOSE8x8W xmm0, xmm7, xmm6, xmm3, xmm5, xmm1, xmm8, xmm2, xmm4
    HADAMARD8_1D  xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7

    ; dc
    movzx       edi, word [r1+0]
    add          di, word [r1+16]
    add         edi, 8
    and         edi, -16
    shl         edi, 2

    pxor        xmm15, xmm15
    movdqa      xmm8, xmm2
    movdqa      xmm9, xmm3
    movdqa      xmm10, xmm4
    movdqa      xmm11, xmm5
    ABS4        xmm8, xmm9, xmm10, xmm11, xmm12, xmm13
    paddusw     xmm8, xmm10
    paddusw     xmm9, xmm11
%ifidn %1, ssse3
    pabsw       xmm10, xmm6
    pabsw       xmm11, xmm7
    pabsw       xmm15, xmm1
%else
    movdqa      xmm10, xmm6
    movdqa      xmm11, xmm7
    movdqa      xmm15, xmm1
    ABS2        xmm10, xmm11, xmm13, xmm14
    ABS1        xmm15, xmm13
%endif
    paddusw     xmm10, xmm11
    paddusw     xmm8, xmm9
    paddusw     xmm15, xmm10
    paddusw     xmm15, xmm8
    movdqa      xmm14, xmm15 ; 7x8 sum

    movdqa      xmm8, [r1+0] ; left edge
    movd        xmm9, edi
    psllw       xmm8, 3
    psubw       xmm8, xmm0
    psubw       xmm9, xmm0
    ABS1        xmm8, xmm10
    ABS1        xmm9, xmm11 ; 1x8 sum
    paddusw     xmm14, xmm8
    paddusw     xmm15, xmm9
    punpcklwd   xmm0, xmm1
    punpcklwd   xmm2, xmm3
    punpcklwd   xmm4, xmm5
    punpcklwd   xmm6, xmm7
    punpckldq   xmm0, xmm2
    punpckldq   xmm4, xmm6
    punpcklqdq  xmm0, xmm4 ; transpose
    movdqa      xmm1, [r1+16] ; top edge
    movdqa      xmm2, xmm15
    psllw       xmm1, 3
    psrldq      xmm2, 2     ; 8x7 sum
    psubw       xmm0, xmm1  ; 8x1 sum
    ABS1        xmm0, xmm1
    paddusw     xmm2, xmm0

    ; 3x HADDW
    movdqa      xmm7, [pw_1 GLOBAL]
    pmaddwd     xmm2,  xmm7
    pmaddwd     xmm14, xmm7
    pmaddwd     xmm15, xmm7
    movdqa      xmm3,  xmm2
    punpckldq   xmm2,  xmm14
    punpckhdq   xmm3,  xmm14
    pshufd      xmm5,  xmm15, 0xf5
    paddd       xmm2,  xmm3
    paddd       xmm5,  xmm15
    movdqa      xmm3,  xmm2
    punpcklqdq  xmm2,  xmm5
    punpckhqdq  xmm3,  xmm5
    pavgw       xmm3,  xmm2
    pxor        xmm0,  xmm0
    pavgw       xmm3,  xmm0
    movq        [r2],  xmm3 ; i8x8_v, i8x8_h
    psrldq      xmm3,  8
    movd       [r2+8], xmm3 ; i8x8_dc
    ret
%endif ; ARCH_X86_64
%endmacro ; INTRA_SATDS

; in: r0 = fenc
; out: mm0..mm3 = hadamard coefs
ALIGN 16
load_hadamard:
    pxor        mm7, mm7
    movd        mm0, [r0+0*FENC_STRIDE]
    movd        mm4, [r0+1*FENC_STRIDE]
    movd        mm3, [r0+2*FENC_STRIDE]
    movd        mm1, [r0+3*FENC_STRIDE]
    punpcklbw   mm0, mm7
    punpcklbw   mm4, mm7
    punpcklbw   mm3, mm7
    punpcklbw   mm1, mm7
    HADAMARD4_1D  mm0, mm4, mm3, mm1
    TRANSPOSE4x4W mm0, mm4, mm3, mm1, mm2
    HADAMARD4_1D  mm0, mm1, mm2, mm3
    ret

%macro SCALAR_SUMSUB 4
    add %1, %2
    add %3, %4
    add %2, %2
    add %4, %4
    sub %2, %1
    sub %4, %3
%endmacro

%macro SCALAR_HADAMARD_LEFT 5 ; y, 4x tmp
%ifnidn %1, 0
    shl         %1d, 5 ; log(FDEC_STRIDE)
%endif
    movzx       %2d, byte [r1+%1-1+0*FDEC_STRIDE]
    movzx       %3d, byte [r1+%1-1+1*FDEC_STRIDE]
    movzx       %4d, byte [r1+%1-1+2*FDEC_STRIDE]
    movzx       %5d, byte [r1+%1-1+3*FDEC_STRIDE]
%ifnidn %1, 0
    shr         %1d, 5
%endif
    SCALAR_SUMSUB %2d, %3d, %4d, %5d
    SCALAR_SUMSUB %2d, %4d, %3d, %5d
    mov         [left_1d+2*%1+0], %2w
    mov         [left_1d+2*%1+2], %3w
    mov         [left_1d+2*%1+4], %4w
    mov         [left_1d+2*%1+6], %5w
%endmacro

%macro SCALAR_HADAMARD_TOP 5 ; x, 4x tmp
    movzx       %2d, byte [r1+%1-FDEC_STRIDE+0]
    movzx       %3d, byte [r1+%1-FDEC_STRIDE+1]
    movzx       %4d, byte [r1+%1-FDEC_STRIDE+2]
    movzx       %5d, byte [r1+%1-FDEC_STRIDE+3]
    SCALAR_SUMSUB %2d, %3d, %4d, %5d
    SCALAR_SUMSUB %2d, %4d, %3d, %5d
    mov         [top_1d+2*%1+0], %2w
    mov         [top_1d+2*%1+2], %3w
    mov         [top_1d+2*%1+4], %4w
    mov         [top_1d+2*%1+6], %5w
%endmacro

%macro SUM_MM_X3 8 ; 3x sum, 4x tmp, op
    pxor        %7, %7
    pshufw      %4, %1, 01001110b
    pshufw      %5, %2, 01001110b
    pshufw      %6, %3, 01001110b
    paddw       %1, %4
    paddw       %2, %5
    paddw       %3, %6
    punpcklwd   %1, %7
    punpcklwd   %2, %7
    punpcklwd   %3, %7
    pshufw      %4, %1, 01001110b
    pshufw      %5, %2, 01001110b
    pshufw      %6, %3, 01001110b
    %8          %1, %4
    %8          %2, %5
    %8          %3, %6
%endmacro

%macro CLEAR_SUMS 0
%ifdef ARCH_X86_64
    mov   qword [sums+0], 0
    mov   qword [sums+8], 0
    mov   qword [sums+16], 0
%else
    pxor  mm7, mm7
    movq  [sums+0], mm7
    movq  [sums+8], mm7
    movq  [sums+16], mm7
%endif
%endmacro

; in: mm1..mm3
; out: mm7
; clobber: mm4..mm6
%macro SUM3x4 1
%ifidn %1, ssse3
    pabsw       mm4, mm1
    pabsw       mm5, mm2
    pabsw       mm7, mm3
    paddw       mm4, mm5
%else
    movq        mm4, mm1
    movq        mm5, mm2
    ABS2        mm4, mm5, mm6, mm7
    movq        mm7, mm3
    paddw       mm4, mm5
    ABS1        mm7, mm6
%endif
    paddw       mm7, mm4
%endmacro

; in: mm0..mm3 (4x4), mm7 (3x4)
; out: mm0 v, mm4 h, mm5 dc
; clobber: mm6
%macro SUM4x3 3 ; dc, left, top
    movq        mm4, %2
    movd        mm5, %1
    psllw       mm4, 2
    psubw       mm4, mm0
    psubw       mm5, mm0
    punpcklwd   mm0, mm1
    punpcklwd   mm2, mm3
    punpckldq   mm0, mm2 ; transpose
    movq        mm1, %3
    psllw       mm1, 2
    psubw       mm0, mm1
    ABS2        mm4, mm5, mm2, mm3 ; 1x4 sum
    ABS1        mm0, mm1 ; 4x1 sum
%endmacro

%macro INTRA_SATDS_MMX 1
;-----------------------------------------------------------------------------
; void x264_intra_satd_x3_4x4_mmxext( uint8_t *fenc, uint8_t *fdec, int *res )
;-----------------------------------------------------------------------------
cglobal x264_intra_satd_x3_4x4_%1, 2,6
%ifdef ARCH_X86_64
    ; stack is 16 byte aligned because abi says so
    %define  top_1d  rsp-8  ; size 8
    %define  left_1d rsp-16 ; size 8
    %define  t0  r10
    %define  t0d r10d
%else
    ; stack is 16 byte aligned at least in gcc, and we've pushed 3 regs + return address, so it's still aligned
    SUB         esp, 16
    %define  top_1d  esp+8
    %define  left_1d esp
    %define  t0  r2
    %define  t0d r2d
%endif

    call load_hadamard
    SCALAR_HADAMARD_LEFT 0, r0, r3, r4, r5
    mov         t0d, r0d
    SCALAR_HADAMARD_TOP  0, r0, r3, r4, r5
    lea         t0d, [t0d + r0d + 4]
    and         t0d, -8
    shl         t0d, 1 ; dc

    SUM3x4 %1
    SUM4x3 t0d, [left_1d], [top_1d]
    paddw       mm4, mm7
    paddw       mm5, mm7
    movq        mm1, mm5
    psrlq       mm1, 16  ; 4x3 sum
    paddw       mm0, mm1

    SUM_MM_X3   mm0, mm4, mm5, mm1, mm2, mm3, mm6, pavgw
%ifndef ARCH_X86_64
    mov         r2, r2m
%endif
    movd        [r2+0], mm0 ; i4x4_v satd
    movd        [r2+4], mm4 ; i4x4_h satd
    movd        [r2+8], mm5 ; i4x4_dc satd
%ifndef ARCH_X86_64
    ADD         esp, 16
%endif
    RET

%ifdef ARCH_X86_64
    %define  t0  r10
    %define  t0d r10d
    %define  t2  r11
    %define  t2w r11w
    %define  t2d r11d
%else
    %define  t0  r0
    %define  t0d r0d
    %define  t2  r2
    %define  t2w r2w
    %define  t2d r2d
%endif

;-----------------------------------------------------------------------------
; void x264_intra_satd_x3_16x16_mmxext( uint8_t *fenc, uint8_t *fdec, int *res )
;-----------------------------------------------------------------------------
cglobal x264_intra_satd_x3_16x16_%1, 0,7
%ifdef ARCH_X86_64
    %assign  stack_pad  88
%else
    %assign  stack_pad  88 + ((stack_offset+88+4)&15)
%endif
    ; not really needed on x86_64, just shuts up valgrind about storing data below the stack across a function call
    SUB          rsp, stack_pad
%define  sums    rsp+64 ; size 24
%define  top_1d  rsp+32 ; size 32
%define  left_1d rsp    ; size 32
    movifnidn   r1d, r1m
    CLEAR_SUMS

    ; 1D hadamards
    xor         t2d, t2d
    mov         t0d, 12
.loop_edge:
    SCALAR_HADAMARD_LEFT t0, r3, r4, r5, r6
    add         t2d, r3d
    SCALAR_HADAMARD_TOP  t0, r3, r4, r5, r6
    add         t2d, r3d
    sub         t0d, 4
    jge .loop_edge
    shr         t2d, 1
    add         t2d, 8
    and         t2d, -16 ; dc

    ; 2D hadamards
    movifnidn   r0d, r0m
    xor         r3d, r3d
.loop_y:
    xor         r4d, r4d
.loop_x:
    call load_hadamard

    SUM3x4 %1
    SUM4x3 t2d, [left_1d+8*r3], [top_1d+8*r4]
    pavgw       mm4, mm7
    pavgw       mm5, mm7
    paddw       mm0, [sums+0]  ; i16x16_v satd
    paddw       mm4, [sums+8]  ; i16x16_h satd
    paddw       mm5, [sums+16] ; i16x16_dc satd
    movq        [sums+0], mm0
    movq        [sums+8], mm4
    movq        [sums+16], mm5

    add         r0, 4
    inc         r4d
    cmp         r4d, 4
    jl  .loop_x
    add         r0, 4*FENC_STRIDE-16
    inc         r3d
    cmp         r3d, 4
    jl  .loop_y

; horizontal sum
    movifnidn   r2d, r2m
    movq        mm2, [sums+16]
    movq        mm1, [sums+8]
    movq        mm0, [sums+0]
    movq        mm7, mm2
    SUM_MM_X3   mm0, mm1, mm2, mm3, mm4, mm5, mm6, paddd
    psrld       mm0, 1
    pslld       mm7, 16
    psrld       mm7, 16
    paddd       mm0, mm2
    psubd       mm0, mm7
    movd        [r2+8], mm2 ; i16x16_dc satd
    movd        [r2+4], mm1 ; i16x16_h satd
    movd        [r2+0], mm0 ; i16x16_v satd
    ADD         rsp, stack_pad
    RET

;-----------------------------------------------------------------------------
; void x264_intra_satd_x3_8x8c_mmxext( uint8_t *fenc, uint8_t *fdec, int *res )
;-----------------------------------------------------------------------------
cglobal x264_intra_satd_x3_8x8c_%1, 0,6
    ; not really needed on x86_64, just shuts up valgrind about storing data below the stack across a function call
    SUB          rsp, 72
%define  sums    rsp+48 ; size 24
%define  dc_1d   rsp+32 ; size 16
%define  top_1d  rsp+16 ; size 16
%define  left_1d rsp    ; size 16
    movifnidn   r1d, r1m
    CLEAR_SUMS

    ; 1D hadamards
    mov         t0d, 4
.loop_edge:
    SCALAR_HADAMARD_LEFT t0, t2, r3, r4, r5
    SCALAR_HADAMARD_TOP  t0, t2, r3, r4, r5
    sub         t0d, 4
    jge .loop_edge

    ; dc
    movzx       t2d, word [left_1d+0]
    movzx       r3d, word [top_1d+0]
    movzx       r4d, word [left_1d+8]
    movzx       r5d, word [top_1d+8]
    add         t2d, r3d
    lea         r3, [r4 + r5]
    lea         t2, [2*t2 + 8]
    lea         r3, [2*r3 + 8]
    lea         r4, [4*r4 + 8]
    lea         r5, [4*r5 + 8]
    and         t2d, -16 ; tl
    and         r3d, -16 ; br
    and         r4d, -16 ; bl
    and         r5d, -16 ; tr
    mov         [dc_1d+ 0], t2d ; tl
    mov         [dc_1d+ 4], r5d ; tr
    mov         [dc_1d+ 8], r4d ; bl
    mov         [dc_1d+12], r3d ; br
    lea         r5, [dc_1d]

    ; 2D hadamards
    movifnidn   r0d, r0m
    movifnidn   r2d, r2m
    xor         r3d, r3d
.loop_y:
    xor         r4d, r4d
.loop_x:
    call load_hadamard

    SUM3x4 %1
    SUM4x3 [r5+4*r4], [left_1d+8*r3], [top_1d+8*r4]
    pavgw       mm4, mm7
    pavgw       mm5, mm7
    paddw       mm0, [sums+16] ; i4x4_v satd
    paddw       mm4, [sums+8]  ; i4x4_h satd
    paddw       mm5, [sums+0]  ; i4x4_dc satd
    movq        [sums+16], mm0
    movq        [sums+8], mm4
    movq        [sums+0], mm5

    add         r0, 4
    inc         r4d
    cmp         r4d, 2
    jl  .loop_x
    add         r0, 4*FENC_STRIDE-8
    add         r5, 8
    inc         r3d
    cmp         r3d, 2
    jl  .loop_y

; horizontal sum
    movq        mm0, [sums+0]
    movq        mm1, [sums+8]
    movq        mm2, [sums+16]
    movq        mm7, mm0
    psrlq       mm7, 15
    paddw       mm2, mm7
    SUM_MM_X3   mm0, mm1, mm2, mm3, mm4, mm5, mm6, paddd
    psrld       mm2, 1
    movd        [r2+0], mm0 ; i8x8c_dc satd
    movd        [r2+4], mm1 ; i8x8c_h satd
    movd        [r2+8], mm2 ; i8x8c_v satd
    ADD         rsp, 72
    RET
%endmacro

; instantiate satds

%ifndef ARCH_X86_64
cextern x264_pixel_sa8d_8x8_mmxext
SA8D_16x16_32 mmxext
%endif

%define ABS1 ABS1_MMX
%define ABS2 ABS2_MMX
SATDS_SSE2 sse2
SA8D_16x16_32 sse2
INTRA_SA8D_SSE2 sse2
INTRA_SATDS_MMX mmxext
%define ABS1 ABS1_SSSE3
%define ABS2 ABS2_SSSE3
SATDS_SSE2 ssse3
SA8D_16x16_32 ssse3
INTRA_SA8D_SSE2 ssse3
INTRA_SATDS_MMX ssse3
SATD_W4 ssse3 ; mmx, but uses pabsw from ssse3.
%define SATD_8x4_SSE2 SATD_8x4_PHADD
SATDS_SSE2 ssse3_phadd



;=============================================================================
; SSIM
;=============================================================================

;-----------------------------------------------------------------------------
; void x264_pixel_ssim_4x4x2_core_sse2( const uint8_t *pix1, int stride1,
;                                       const uint8_t *pix2, int stride2, int sums[2][4] )
;-----------------------------------------------------------------------------
cglobal x264_pixel_ssim_4x4x2_core_sse2, 4,4
    pxor      xmm0, xmm0
    pxor      xmm1, xmm1
    pxor      xmm2, xmm2
    pxor      xmm3, xmm3
    pxor      xmm4, xmm4
%rep 4
    movq      xmm5, [r0]
    movq      xmm6, [r2]
    punpcklbw xmm5, xmm0
    punpcklbw xmm6, xmm0
    paddw     xmm1, xmm5
    paddw     xmm2, xmm6
    movdqa    xmm7, xmm5
    pmaddwd   xmm5, xmm5
    pmaddwd   xmm7, xmm6
    pmaddwd   xmm6, xmm6
    paddd     xmm3, xmm5
    paddd     xmm4, xmm7
    paddd     xmm3, xmm6
    add       r0, r1
    add       r2, r3
%endrep
    ; PHADDW xmm1, xmm2
    ; PHADDD xmm3, xmm4
    picgetgot eax
    movdqa    xmm7, [pw_1 GLOBAL]
    pshufd    xmm5, xmm3, 0xb1
    pmaddwd   xmm1, xmm7
    pmaddwd   xmm2, xmm7
    pshufd    xmm6, xmm4, 0xb1
    packssdw  xmm1, xmm2
    paddd     xmm3, xmm5
    pshufd    xmm1, xmm1, 0xd8
    paddd     xmm4, xmm6
    pmaddwd   xmm1, xmm7
    movdqa    xmm5, xmm3
    punpckldq xmm3, xmm4
    punpckhdq xmm5, xmm4

%ifdef ARCH_X86_64
    %define t0 r4
%else
    %define t0 eax
    mov t0, r4m
%endif
%ifnidn r4d, r4m
    mov t0, r4m
%endif
    
    movq      [t0+ 0], xmm1
    movq      [t0+ 8], xmm3
    psrldq    xmm1, 8
    movq      [t0+16], xmm1
    movq      [t0+24], xmm5
    RET

;-----------------------------------------------------------------------------
; float x264_pixel_ssim_end_sse2( int sum0[5][4], int sum1[5][4], int width )
;-----------------------------------------------------------------------------
cglobal x264_pixel_ssim_end4_sse2, 3,3
    movdqa   xmm0, [r0+ 0]
    movdqa   xmm1, [r0+16]
    movdqa   xmm2, [r0+32]
    movdqa   xmm3, [r0+48]
    movdqa   xmm4, [r0+64]
    paddd    xmm0, [r1+ 0]
    paddd    xmm1, [r1+16]
    paddd    xmm2, [r1+32]
    paddd    xmm3, [r1+48]
    paddd    xmm4, [r1+64]
    paddd    xmm0, xmm1
    paddd    xmm1, xmm2
    paddd    xmm2, xmm3
    paddd    xmm3, xmm4
    picgetgot r1
    movdqa   xmm5, [ssim_c1 GLOBAL]
    movdqa   xmm6, [ssim_c2 GLOBAL]
    TRANSPOSE4x4D  xmm0, xmm1, xmm2, xmm3, xmm4

;   s1=mm0, s2=mm3, ss=mm4, s12=mm2
    movdqa   xmm1, xmm3
    pslld    xmm3, 16
    pmaddwd  xmm1, xmm0  ; s1*s2
    por      xmm0, xmm3
    pmaddwd  xmm0, xmm0  ; s1*s1 + s2*s2
    pslld    xmm1, 1
    pslld    xmm2, 7
    pslld    xmm4, 6
    psubd    xmm2, xmm1  ; covar*2
    psubd    xmm4, xmm0  ; vars
    paddd    xmm0, xmm5
    paddd    xmm1, xmm5
    paddd    xmm2, xmm6
    paddd    xmm4, xmm6
    cvtdq2ps xmm0, xmm0  ; (float)(s1*s1 + s2*s2 + ssim_c1)
    cvtdq2ps xmm1, xmm1  ; (float)(s1*s2*2 + ssim_c1)
    cvtdq2ps xmm2, xmm2  ; (float)(covar*2 + ssim_c2)
    cvtdq2ps xmm4, xmm4  ; (float)(vars + ssim_c2)
    mulps    xmm1, xmm2
    mulps    xmm0, xmm4
    divps    xmm1, xmm0  ; ssim

    cmp      r2d, 4
    je .skip ; faster only if this is the common case; remove branch if we use ssim on a macroblock level
    neg      r2
%ifdef PIC64
    lea      r3,   [mask_ff + 16 GLOBAL]
    movdqu   xmm3, [r3 + r2*4]
%else
    movdqu   xmm3, [mask_ff + r2*4 + 16 GLOBAL]
%endif
    pand     xmm1, xmm3
.skip:
    movhlps  xmm0, xmm1
    addps    xmm0, xmm1
    pshuflw  xmm1, xmm0, 0xE
    addss    xmm0, xmm1
%ifndef ARCH_X86_64
    movd     r0m, xmm0
    fld      dword r0m
%endif
    RET



;=============================================================================
; Successive Elimination ADS
;=============================================================================

%macro ADS_START 1 ; unroll_size 
%ifdef ARCH_X86_64
    %define t0  r6
    mov     r10, rsp
%else
    %define t0  r4
    mov     rbp, rsp
%endif
    mov     r0d, r5m
    sub     rsp, r0
    sub     rsp, %1*4-1
    and     rsp, ~15
    mov     t0,  rsp
    shl     r2d,  1
%endmacro   

%macro ADS_END 1
    add     r1, 8*%1
    add     r3, 8*%1
    add     t0, 4*%1
    sub     r0d, 4*%1
    jg .loop
    jmp ads_mvs
%endmacro

%define ABS1 ABS1_MMX

;-----------------------------------------------------------------------------
; int x264_pixel_ads4_mmxext( int enc_dc[4], uint16_t *sums, int delta,
;                             uint16_t *cost_mvx, int16_t *mvs, int width, int thresh )
;-----------------------------------------------------------------------------
cglobal x264_pixel_ads4_mmxext, 4,7
    movq    mm6, [r0]
    movq    mm4, [r0+8]
    pshufw  mm7, mm6, 0
    pshufw  mm6, mm6, 0xAA
    pshufw  mm5, mm4, 0
    pshufw  mm4, mm4, 0xAA
    ADS_START 1
.loop:
    movq    mm0, [r1]
    movq    mm1, [r1+16]
    psubw   mm0, mm7
    psubw   mm1, mm6
    ABS1    mm0, mm2
    ABS1    mm1, mm3
    movq    mm2, [r1+r2]
    movq    mm3, [r1+r2+16]
    psubw   mm2, mm5
    psubw   mm3, mm4
    paddw   mm0, mm1
    ABS1    mm2, mm1
    ABS1    mm3, mm1
    paddw   mm0, mm2
    paddw   mm0, mm3
%ifdef ARCH_X86_64
    pshufw  mm1, [r10+8], 0
%else
    pshufw  mm1, [ebp+stack_offset+28], 0
%endif
    paddusw mm0, [r3]
    psubusw mm1, mm0
    packsswb mm1, mm1
    movd    [t0], mm1
    ADS_END 1

cglobal x264_pixel_ads2_mmxext, 4,7
    movq    mm6, [r0]
    pshufw  mm5, r6m, 0
    pshufw  mm7, mm6, 0
    pshufw  mm6, mm6, 0xAA
    ADS_START 1
.loop:
    movq    mm0, [r1]
    movq    mm1, [r1+r2]
    psubw   mm0, mm7
    psubw   mm1, mm6
    ABS1    mm0, mm2
    ABS1    mm1, mm3
    paddw   mm0, mm1
    paddusw mm0, [r3]
    movq    mm4, mm5
    psubusw mm4, mm0
    packsswb mm4, mm4
    movd    [t0], mm4
    ADS_END 1

cglobal x264_pixel_ads1_mmxext, 4,7
    pshufw  mm7, [r0], 0
    pshufw  mm6, r6m, 0
    ADS_START 2
.loop:
    movq    mm0, [r1]
    movq    mm1, [r1+8]
    psubw   mm0, mm7
    psubw   mm1, mm7
    ABS1    mm0, mm2
    ABS1    mm1, mm3
    paddusw mm0, [r3]
    paddusw mm1, [r3+8]
    movq    mm4, mm6
    movq    mm5, mm6
    psubusw mm4, mm0
    psubusw mm5, mm1
    packsswb mm4, mm5
    movq    [t0], mm4
    ADS_END 2

%macro ADS_SSE2 1
cglobal x264_pixel_ads4_%1, 4,7
    movdqa  xmm4, [r0]
    pshuflw xmm7, xmm4, 0
    pshuflw xmm6, xmm4, 0xAA
    pshufhw xmm5, xmm4, 0
    pshufhw xmm4, xmm4, 0xAA
    punpcklqdq xmm7, xmm7
    punpcklqdq xmm6, xmm6
    punpckhqdq xmm5, xmm5
    punpckhqdq xmm4, xmm4
%ifdef ARCH_X86_64
    pshuflw xmm8, r6m, 0
    punpcklqdq xmm8, xmm8
    ADS_START 2
    movdqu  xmm10, [r1]
    movdqu  xmm11, [r1+r2]
.loop:
    movdqa  xmm0, xmm10
    movdqu  xmm1, [r1+16]
    movdqa  xmm10, xmm1
    psubw   xmm0, xmm7
    psubw   xmm1, xmm6
    ABS1    xmm0, xmm2
    ABS1    xmm1, xmm3
    movdqa  xmm2, xmm11
    movdqu  xmm3, [r1+r2+16]
    movdqa  xmm11, xmm3
    psubw   xmm2, xmm5
    psubw   xmm3, xmm4
    paddw   xmm0, xmm1
    movdqu  xmm9, [r3]
    ABS1    xmm2, xmm1
    ABS1    xmm3, xmm1
    paddw   xmm0, xmm2
    paddw   xmm0, xmm3
    paddusw xmm0, xmm9
    movdqa  xmm1, xmm8
    psubusw xmm1, xmm0
    packsswb xmm1, xmm1
    movq    [t0], xmm1
%else
    ADS_START 2
.loop:
    movdqu  xmm0, [r1]
    movdqu  xmm1, [r1+16]
    psubw   xmm0, xmm7
    psubw   xmm1, xmm6
    ABS1    xmm0, xmm2
    ABS1    xmm1, xmm3
    movdqu  xmm2, [r1+r2]
    movdqu  xmm3, [r1+r2+16]
    psubw   xmm2, xmm5
    psubw   xmm3, xmm4
    paddw   xmm0, xmm1
    ABS1    xmm2, xmm1
    ABS1    xmm3, xmm1
    paddw   xmm0, xmm2
    paddw   xmm0, xmm3
    movd    xmm1, [ebp+stack_offset+28]
    movdqu  xmm2, [r3]
    pshuflw xmm1, xmm1, 0
    punpcklqdq xmm1, xmm1
    paddusw xmm0, xmm2
    psubusw xmm1, xmm0
    packsswb xmm1, xmm1
    movq    [t0], xmm1
%endif ; ARCH
    ADS_END 2

cglobal x264_pixel_ads2_%1, 4,7
    movq    xmm6, [r0]
    movd    xmm5, r6m
    pshuflw xmm7, xmm6, 0
    pshuflw xmm6, xmm6, 0xAA
    pshuflw xmm5, xmm5, 0
    punpcklqdq xmm7, xmm7
    punpcklqdq xmm6, xmm6
    punpcklqdq xmm5, xmm5
    ADS_START 2
.loop:
    movdqu  xmm0, [r1]
    movdqu  xmm1, [r1+r2]
    psubw   xmm0, xmm7
    psubw   xmm1, xmm6
    movdqu  xmm4, [r3]
    ABS1    xmm0, xmm2
    ABS1    xmm1, xmm3
    paddw   xmm0, xmm1
    paddusw xmm0, xmm4
    movdqa  xmm1, xmm5
    psubusw xmm1, xmm0
    packsswb xmm1, xmm1
    movq    [t0], xmm1
    ADS_END 2

cglobal x264_pixel_ads1_%1, 4,7
    movd    xmm7, [r0]
    movd    xmm6, r6m
    pshuflw xmm7, xmm7, 0
    pshuflw xmm6, xmm6, 0
    punpcklqdq xmm7, xmm7
    punpcklqdq xmm6, xmm6
    ADS_START 4
.loop:
    movdqu  xmm0, [r1]
    movdqu  xmm1, [r1+16]
    psubw   xmm0, xmm7
    psubw   xmm1, xmm7
    movdqu  xmm2, [r3]
    movdqu  xmm3, [r3+16]
    ABS1    xmm0, xmm4
    ABS1    xmm1, xmm5
    paddusw xmm0, xmm2
    paddusw xmm1, xmm3
    movdqa  xmm4, xmm6
    movdqa  xmm5, xmm6
    psubusw xmm4, xmm0
    psubusw xmm5, xmm1
    packsswb xmm4, xmm5
    movdqa  [t0], xmm4
    ADS_END 4
%endmacro

ADS_SSE2 sse2
%define ABS1 ABS1_SSSE3
ADS_SSE2 ssse3

; int x264_pixel_ads_mvs( int16_t *mvs, uint8_t *masks, int width )
; {
;     int nmv=0, i, j;
;     *(uint32_t*)(masks+width) = 0;
;     for( i=0; i<width; i+=8 )
;     {
;         uint64_t mask = *(uint64_t*)(masks+i);
;         if( !mask ) continue;
;         for( j=0; j<8; j++ )
;             if( mask & (255<<j*8) )
;                 mvs[nmv++] = i+j;
;     }
;     return nmv;
; }
cglobal x264_pixel_ads_mvs
ads_mvs:
    xor     eax, eax
    xor     esi, esi
%ifdef ARCH_X86_64
    ; mvs = r4
    ; masks = rsp
    ; width = r5
    ; clear last block in case width isn't divisible by 8. (assume divisible by 4, so clearing 4 bytes is enough.)
    mov     dword [rsp+r5], 0
    jmp .loopi
.loopi0:
    add     esi, 8
    cmp     esi, r5d
    jge .end
.loopi:
    mov     rdi, [rsp+rsi]
    test    rdi, rdi
    jz .loopi0
    xor     ecx, ecx
%macro TEST 1
    mov     [r4+rax*2], si
    test    edi, 0xff<<(%1*8)
    setne   cl
    add     eax, ecx
    inc     esi
%endmacro
    TEST 0
    TEST 1
    TEST 2
    TEST 3
    shr     rdi, 32
    TEST 0
    TEST 1
    TEST 2
    TEST 3
    cmp     esi, r5d
    jl .loopi
.end:
    mov     rsp, r10
    ret

%else
    ; no PROLOGUE, inherit from x264_pixel_ads1
    mov     ebx, [ebp+stack_offset+20] ; mvs
    mov     edi, [ebp+stack_offset+24] ; width
    mov     dword [esp+edi], 0
    push    ebp
    jmp .loopi
.loopi0:
    add     esi, 8
    cmp     esi, edi
    jge .end
.loopi:
    mov     ebp, [esp+esi+4]
    mov     edx, [esp+esi+8]
    mov     ecx, ebp
    or      ecx, edx
    jz .loopi0
    xor     ecx, ecx
%macro TEST 2
    mov     [ebx+eax*2], si
    test    %2, 0xff<<(%1*8)
    setne   cl
    add     eax, ecx
    inc     esi
%endmacro
    TEST 0, ebp
    TEST 1, ebp
    TEST 2, ebp
    TEST 3, ebp
    TEST 0, edx
    TEST 1, edx
    TEST 2, edx
    TEST 3, edx
    cmp     esi, edi
    jl .loopi
.end:
    pop     esp
    RET
%endif ; ARCH


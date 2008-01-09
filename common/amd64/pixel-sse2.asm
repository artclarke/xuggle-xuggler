;*****************************************************************************
;* pixel-sse2.asm: h264 encoder library
;*****************************************************************************
;* Copyright (C) 2005 x264 project
;*
;* Authors: Alex Izvorski <aizvorksi@gmail.com>
;*          Loren Merritt <lorenm@u.washington.edu>
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

SECTION_RODATA

pw_1:    times 8 dw 1
ssim_c1: times 4 dd 416    ; .01*.01*255*255*64
ssim_c2: times 4 dd 235963 ; .03*.03*255*255*64*63
mask_ff: times 16 db 0xff
         times 16 db 0
sw_64:   dq 64

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

%macro SAD_END_SSE2 0
    movhlps xmm1, xmm0
    paddw   xmm0, xmm1
    movd    eax,  xmm0
    ret
%endmacro

%macro SAD_W16 1
;-----------------------------------------------------------------------------
;   int x264_pixel_sad_16x16_sse2 (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
cglobal x264_pixel_sad_16x16_%1
    movdqu xmm0, [rdx]
    movdqu xmm1, [rdx+rcx]
    lea    rdx,  [rdx+2*rcx]
    movdqu xmm2, [rdx]
    movdqu xmm3, [rdx+rcx]
    lea    rdx,  [rdx+2*rcx]
    psadbw xmm0, [rdi]
    psadbw xmm1, [rdi+rsi]
    lea    rdi,  [rdi+2*rsi]
    movdqu xmm4, [rdx]
    paddw  xmm0, xmm1
    psadbw xmm2, [rdi]
    psadbw xmm3, [rdi+rsi]
    lea    rdi,  [rdi+2*rsi]
    movdqu xmm5, [rdx+rcx]
    lea    rdx,  [rdx+2*rcx]
    paddw  xmm2, xmm3
    movdqu xmm6, [rdx]
    movdqu xmm7, [rdx+rcx]
    lea    rdx,  [rdx+2*rcx]
    paddw  xmm0, xmm2
    psadbw xmm4, [rdi]
    psadbw xmm5, [rdi+rsi]
    lea    rdi,  [rdi+2*rsi]
    movdqu xmm1, [rdx]
    paddw  xmm4, xmm5
    psadbw xmm6, [rdi]
    psadbw xmm7, [rdi+rsi]
    lea    rdi,  [rdi+2*rsi]
    movdqu xmm2, [rdx+rcx]
    lea    rdx,  [rdx+2*rcx]
    paddw  xmm6, xmm7
    movdqu xmm3, [rdx]
    paddw  xmm0, xmm4
    movdqu xmm4, [rdx+rcx]
    lea    rdx,  [rdx+2*rcx]
    paddw  xmm0, xmm6
    psadbw xmm1, [rdi]
    psadbw xmm2, [rdi+rsi]
    lea    rdi,  [rdi+2*rsi]
    movdqu xmm5, [rdx]
    paddw  xmm1, xmm2
    psadbw xmm3, [rdi]
    psadbw xmm4, [rdi+rsi]
    lea    rdi,  [rdi+2*rsi]
    movdqu xmm6, [rdx+rcx]
    lea    rdx,  [rdx+2*rcx]
    paddw  xmm3, xmm4
    movdqu xmm7, [rdx]
    paddw  xmm0, xmm1
    movdqu xmm1, [rdx+rcx]
    paddw  xmm0, xmm3
    psadbw xmm5, [rdi]
    psadbw xmm6, [rdi+rsi]
    lea    rdi,  [rdi+2*rsi]
    paddw  xmm5, xmm6
    psadbw xmm7, [rdi]
    psadbw xmm1, [rdi+rsi]
    paddw  xmm7, xmm1
    paddw  xmm0, xmm5
    paddw  xmm0, xmm7
    SAD_END_SSE2

;-----------------------------------------------------------------------------
;   int x264_pixel_sad_16x8_sse2 (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
cglobal x264_pixel_sad_16x8_%1
    movdqu  xmm0,   [rdx]
    movdqu  xmm2,   [rdx+rcx]
    lea     rdx,    [rdx+2*rcx]
    movdqu  xmm3,   [rdx]
    movdqu  xmm4,   [rdx+rcx]
    psadbw  xmm0,   [rdi]
    psadbw  xmm2,   [rdi+rsi]
    lea     rdi,    [rdi+2*rsi]
    psadbw  xmm3,   [rdi]
    psadbw  xmm4,   [rdi+rsi]
    lea     rdi,    [rdi+2*rsi]
    lea     rdx,    [rdx+2*rcx]
    paddw   xmm0,   xmm2
    paddw   xmm3,   xmm4
    paddw   xmm0,   xmm3
    movdqu  xmm1,   [rdx]
    movdqu  xmm2,   [rdx+rcx]
    lea     rdx,    [rdx+2*rcx]
    movdqu  xmm3,   [rdx]
    movdqu  xmm4,   [rdx+rcx]
    psadbw  xmm1,   [rdi]
    psadbw  xmm2,   [rdi+rsi]
    lea     rdi,    [rdi+2*rsi]
    psadbw  xmm3,   [rdi]
    psadbw  xmm4,   [rdi+rsi]
    lea     rdi,    [rdi+2*rsi]
    lea     rdx,    [rdx+2*rcx]
    paddw   xmm1,   xmm2
    paddw   xmm3,   xmm4
    paddw   xmm0,   xmm1
    paddw   xmm0,   xmm3
    SAD_END_SSE2
%endmacro

SAD_W16 sse2
%ifdef HAVE_SSE3
%define movdqu lddqu
SAD_W16 sse3
%undef movdqu
%endif


; sad x3 / x4

%macro SAD_X3_START_1x16P 0
    movdqa xmm3, [parm1q]
    movdqu xmm0, [parm2q]
    movdqu xmm1, [parm3q]
    movdqu xmm2, [parm4q]
    psadbw xmm0, xmm3
    psadbw xmm1, xmm3
    psadbw xmm2, xmm3
%endmacro

%macro SAD_X3_1x16P 2
    movdqa xmm3, [parm1q+%1]
    movdqu xmm4, [parm2q+%2]
    movdqu xmm5, [parm3q+%2]
    movdqu xmm6, [parm4q+%2]
    psadbw xmm4, xmm3
    psadbw xmm5, xmm3
    psadbw xmm6, xmm3
    paddw  xmm0, xmm4
    paddw  xmm1, xmm5
    paddw  xmm2, xmm6
%endmacro

%macro SAD_X3_2x16P 1
%if %1
    SAD_X3_START_1x16P
%else
    SAD_X3_1x16P 0, 0
%endif
    SAD_X3_1x16P FENC_STRIDE, parm5q
    add  parm1q, 2*FENC_STRIDE
    lea  parm2q, [parm2q+2*parm5q]
    lea  parm3q, [parm3q+2*parm5q]
    lea  parm4q, [parm4q+2*parm5q]
%endmacro

%macro SAD_X4_START_1x16P 0
    movdqa xmm7, [parm1q]
    movdqu xmm0, [parm2q]
    movdqu xmm1, [parm3q]
    movdqu xmm2, [parm4q]
    movdqu xmm3, [parm5q]
    psadbw xmm0, xmm7
    psadbw xmm1, xmm7
    psadbw xmm2, xmm7
    psadbw xmm3, xmm7
%endmacro

%macro SAD_X4_1x16P 2
    movdqa xmm7, [parm1q+%1]
    movdqu xmm4, [parm2q+%2]
    movdqu xmm5, [parm3q+%2]
    movdqu xmm6, [parm4q+%2]
    movdqu xmm8, [parm5q+%2]
    psadbw xmm4, xmm7
    psadbw xmm5, xmm7
    psadbw xmm6, xmm7
    psadbw xmm8, xmm7
    paddw  xmm0, xmm4
    paddw  xmm1, xmm5
    paddw  xmm2, xmm6
    paddw  xmm3, xmm8
%endmacro

%macro SAD_X4_2x16P 1
%if %1
    SAD_X4_START_1x16P
%else
    SAD_X4_1x16P 0, 0
%endif
    SAD_X4_1x16P FENC_STRIDE, parm6q
    add  parm1q, 2*FENC_STRIDE
    lea  parm2q, [parm2q+2*parm6q]
    lea  parm3q, [parm3q+2*parm6q]
    lea  parm4q, [parm4q+2*parm6q]
    lea  parm5q, [parm5q+2*parm6q]
%endmacro

%macro SAD_X3_END 0
    movhlps xmm4, xmm0
    movhlps xmm5, xmm1
    movhlps xmm6, xmm2
    paddw   xmm0, xmm4
    paddw   xmm1, xmm5
    paddw   xmm2, xmm6
    movd [parm6q+0], xmm0
    movd [parm6q+4], xmm1
    movd [parm6q+8], xmm2
    ret
%endmacro

%macro SAD_X4_END 0
    mov      rax, parm7q
    psllq   xmm1, 32
    psllq   xmm3, 32
    paddw   xmm0, xmm1
    paddw   xmm2, xmm3
    movhlps xmm1, xmm0
    movhlps xmm3, xmm2
    paddw   xmm0, xmm1
    paddw   xmm2, xmm3
    movq [rax+0], xmm0
    movq [rax+8], xmm2
    ret
%endmacro

;-----------------------------------------------------------------------------
;  void x264_pixel_sad_x3_16x16_sse2( uint8_t *fenc, uint8_t *pix0, uint8_t *pix1,
;                                     uint8_t *pix2, int i_stride, int scores[3] )
;-----------------------------------------------------------------------------
%macro SAD_X 4
cglobal x264_pixel_sad_x%1_%2x%3_%4
    SAD_X%1_2x%2P 1
%rep %3/2-1
    SAD_X%1_2x%2P 0
%endrep
    SAD_X%1_END
%endmacro

SAD_X 3, 16, 16, sse2
SAD_X 3, 16,  8, sse2
SAD_X 4, 16, 16, sse2
SAD_X 4, 16,  8, sse2

%ifdef HAVE_SSE3
%define movdqu lddqu
SAD_X 3, 16, 16, sse3
SAD_X 3, 16,  8, sse3
SAD_X 4, 16, 16, sse3
SAD_X 4, 16,  8, sse3
%undef movdqu
%endif


; Core2 (Conroe) can load unaligned data just as quickly as aligned data...
; unless the unaligned data spans the border between 2 cachelines, in which
; case it's really slow. The exact numbers may differ, but all Intel cpus
; have a large penalty for cacheline splits.
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

; computed jump assumes this loop is exactly 64 bytes
%macro SAD16_CACHELINE_LOOP 1 ; alignment
ALIGN 16
sad_w16_align%1:
    movdqa  xmm1, [rdx+16]
    movdqa  xmm2, [rdx+rcx+16]
    palignr xmm1, [rdx], %1
    palignr xmm2, [rdx+rcx], %1
    psadbw  xmm1, [rdi]
    psadbw  xmm2, [rdi+rsi]
    paddw   xmm0, xmm1
    paddw   xmm0, xmm2
    lea     rdx,  [rdx+2*rcx]
    lea     rdi,  [rdi+2*rsi]
    dec     eax
    jg sad_w16_align%1
    ret
%endmacro

%macro SAD16_CACHELINE_FUNC 1 ; height
cglobal x264_pixel_sad_16x%1_cache64_ssse3
    mov    eax, parm3d
    and    eax, 0x37
    cmp    eax, 0x30
    jle x264_pixel_sad_16x%1_sse2
    mov    eax, parm3d
    and    eax, 15
    shl    eax, 6
%ifdef __PIC__
    lea    r10, [sad_w16_align1 - 64 GLOBAL]
    add    r10, rax
%else
    lea    r10, [sad_w16_align1 - 64 + rax]
%endif
    and parm3q, ~15
    mov    eax, %1/2
    pxor  xmm0, xmm0
    call   r10
    SAD_END_SSE2
%endmacro

%macro SAD8_CACHELINE_FUNC 1 ; height
cglobal x264_pixel_sad_8x%1_cache64_mmxext
    mov    eax, parm3d
    and    eax, 0x3f
    cmp    eax, 0x38
    jle x264_pixel_sad_8x%1_mmxext
    and    eax, 7
    shl    eax, 3
    movd   mm6, [sw_64 GLOBAL]
    movd   mm7, eax
    psubw  mm6, mm7
    and parm3q, ~7
    mov    eax, %1/2
    pxor   mm0, mm0
.loop:
    movq   mm1, [parm3q+8]
    movq   mm2, [parm3q+parm4q+8]
    movq   mm3, [parm3q]
    movq   mm4, [parm3q+parm4q]
    psllq  mm1, mm6
    psllq  mm2, mm6
    psrlq  mm3, mm7
    psrlq  mm4, mm7
    por    mm1, mm3
    por    mm2, mm4
    psadbw mm1, [parm1q]
    psadbw mm2, [parm1q+parm2q]
    paddw  mm0, mm1
    paddw  mm0, mm2
    lea    parm3q, [parm3q+2*parm4q]
    lea    parm1q, [parm1q+2*parm2q]
    dec    eax
    jg .loop
    movd   eax, mm0
    ret
%endmacro


; sad_x3/x4_cache64: check each mv.
; if they're all within a cacheline, use normal sad_x3/x4.
; otherwise, send them individually to sad_cache64.
%macro CHECK_SPLIT 2 ; pix, width
    mov  eax, %1
    and  eax, 0x37|%2
    cmp  eax, 0x30|%2
    jg .split
%endmacro

%macro SADX3_CACHELINE_FUNC 4 ; width, height, normal_ver, split_ver
cglobal x264_pixel_sad_x3_%1x%2_cache64_%4
    CHECK_SPLIT parm2d, %1
    CHECK_SPLIT parm3d, %1
    CHECK_SPLIT parm4d, %1
    jmp x264_pixel_sad_x3_%1x%2_%3
.split:
    push parm4q
    push parm3q
    mov  parm3q, parm2q
    mov  parm2q, FENC_STRIDE
    mov  parm4q, parm5q
    mov  parm5q, parm1q
    call x264_pixel_sad_%1x%2_cache64_%4
    mov  [parm6q], eax
    pop  parm3q
    mov  parm1q, parm5q
    call x264_pixel_sad_%1x%2_cache64_%4
    mov  [parm6q+4], eax
    pop  parm3q
    mov  parm1q, parm5q
    call x264_pixel_sad_%1x%2_cache64_%4
    mov  [parm6q+8], eax
    ret
%endmacro

%macro SADX4_CACHELINE_FUNC 4 ; width, height, normal_ver, split_ver
cglobal x264_pixel_sad_x4_%1x%2_cache64_%4
    CHECK_SPLIT parm2d, %1
    CHECK_SPLIT parm3d, %1
    CHECK_SPLIT parm4d, %1
    CHECK_SPLIT parm5d, %1
    jmp x264_pixel_sad_x4_%1x%2_%3
.split:
    mov  r11, parm7q
    push parm5q
    push parm4q
    push parm3q
    mov  parm3q, parm2q
    mov  parm2q, FENC_STRIDE
    mov  parm4q, parm6q
    mov  parm5q, parm1q
    call x264_pixel_sad_%1x%2_cache64_%4
    mov  [r11], eax
    pop  parm3q
    mov  parm1q, parm5q
    call x264_pixel_sad_%1x%2_cache64_%4
    mov  [r11+4], eax
    pop  parm3q
    mov  parm1q, parm5q
    call x264_pixel_sad_%1x%2_cache64_%4
    mov  [r11+8], eax
    pop  parm3q
    mov  parm1q, parm5q
    call x264_pixel_sad_%1x%2_cache64_%4
    mov  [r11+12], eax
    ret
%endmacro

%macro SADX34_CACHELINE_FUNC 4
    SADX3_CACHELINE_FUNC %1, %2, %3, %4
    SADX4_CACHELINE_FUNC %1, %2, %3, %4
%endmacro

cextern x264_pixel_sad_8x16_mmxext
cextern x264_pixel_sad_8x8_mmxext
cextern x264_pixel_sad_8x4_mmxext
cextern x264_pixel_sad_x3_8x16_mmxext
cextern x264_pixel_sad_x3_8x8_mmxext
cextern x264_pixel_sad_x4_8x16_mmxext
cextern x264_pixel_sad_x4_8x8_mmxext

; instantiate the aligned sads

SAD8_CACHELINE_FUNC 4
SAD8_CACHELINE_FUNC 8
SAD8_CACHELINE_FUNC 16
SADX34_CACHELINE_FUNC 8,  16, mmxext, mmxext
SADX34_CACHELINE_FUNC 8,  8,  mmxext, mmxext

%ifdef HAVE_SSE3

SAD16_CACHELINE_FUNC 8
SAD16_CACHELINE_FUNC 16
%assign i 1
%rep 15
SAD16_CACHELINE_LOOP i
%assign i i+1
%endrep

SADX34_CACHELINE_FUNC 16, 16, sse2, ssse3
SADX34_CACHELINE_FUNC 16, 8,  sse2, ssse3

%endif ; HAVE_SSE3


; ssd

%macro SSD_INC_2x16P_SSE2 0
    movdqu  xmm1,   [rdi]
    movdqu  xmm2,   [rdx]
    movdqu  xmm3,   [rdi+rsi]
    movdqu  xmm4,   [rdx+rcx]

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

    lea     rdi,    [rdi+2*rsi]
    lea     rdx,    [rdx+2*rcx]

    paddd   xmm1,   xmm2
    paddd   xmm3,   xmm4
    paddd   xmm0,   xmm1
    paddd   xmm0,   xmm3
%endmacro

%macro SSD_START_SSE2 0
    pxor    xmm7,   xmm7        ; zero
    pxor    xmm0,   xmm0        ; mm0 holds the sum
%endmacro

%macro SSD_END_SSE2 0
    HADDD   xmm0, xmm1
    movd    eax,  xmm0
    ret
%endmacro

;-----------------------------------------------------------------------------
;   int x264_pixel_ssd_16x16_sse2 (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
cglobal x264_pixel_ssd_16x16_sse2
    SSD_START_SSE2
%rep 8
    SSD_INC_2x16P_SSE2
%endrep
    SSD_END_SSE2

;-----------------------------------------------------------------------------
;   int x264_pixel_ssd_16x8_sse2 (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
cglobal x264_pixel_ssd_16x8_sse2
    SSD_START_SSE2
%rep 4
    SSD_INC_2x16P_SSE2
%endrep
    SSD_END_SSE2



%macro SUMSUB_BADC 4
    paddw   %1, %2
    paddw   %3, %4
    paddw   %2, %2
    paddw   %4, %4
    psubw   %2, %1
    psubw   %4, %3
%endmacro

%macro HADAMARD1x4 4
    SUMSUB_BADC %1, %2, %3, %4
    SUMSUB_BADC %1, %3, %2, %4
%endmacro

%macro HADAMARD1x8 8
    SUMSUB_BADC %1, %5, %2, %6
    SUMSUB_BADC %3, %7, %4, %8
    SUMSUB_BADC %1, %3, %2, %4
    SUMSUB_BADC %5, %7, %6, %8
    SUMSUB_BADC %1, %2, %3, %4
    SUMSUB_BADC %5, %6, %7, %8
%endmacro

;;; row transform not used, because phaddw is much slower than paddw on a Conroe
;%macro PHSUMSUB 3
;    movdqa  %3, %1
;    phaddw  %1, %2
;    phsubw  %3, %2
;%endmacro

;%macro HADAMARD4x1_SSSE3 5  ; ABCD-T -> ADTC
;    PHSUMSUB    %1, %2, %5
;    PHSUMSUB    %3, %4, %2
;    PHSUMSUB    %1, %3, %4
;    PHSUMSUB    %5, %2, %3
;%endmacro

%macro SBUTTERFLY 5
    mov%1       %5, %3
    punpckl%2   %3, %4
    punpckh%2   %5, %4
%endmacro

%macro SBUTTERFLY2 5  ; not really needed, but allows transpose4x4 to not shuffle registers
    mov%1       %5, %3
    punpckh%2   %3, %4
    punpckl%2   %5, %4
%endmacro

%macro TRANSPOSE4x4D 5   ; ABCD-T -> ADTC
    SBUTTERFLY dqa, dq,  %1, %2, %5
    SBUTTERFLY dqa, dq,  %3, %4, %2
    SBUTTERFLY dqa, qdq, %1, %3, %4
    SBUTTERFLY dqa, qdq, %5, %2, %3
%endmacro

%macro TRANSPOSE2x4x4W 5   ; ABCD-T -> ABCD
    SBUTTERFLY  dqa, wd,  %1, %2, %5
    SBUTTERFLY  dqa, wd,  %3, %4, %2
    SBUTTERFLY  dqa, dq,  %1, %3, %4
    SBUTTERFLY2 dqa, dq,  %5, %2, %3
    SBUTTERFLY  dqa, qdq, %1, %3, %2
    SBUTTERFLY2 dqa, qdq, %4, %5, %3
%endmacro

%macro TRANSPOSE8x8 9   ; ABCDEFGH-T -> AFHDTECB
    SBUTTERFLY dqa, wd, %1, %2, %9
    SBUTTERFLY dqa, wd, %3, %4, %2
    SBUTTERFLY dqa, wd, %5, %6, %4
    SBUTTERFLY dqa, wd, %7, %8, %6
    SBUTTERFLY dqa, dq, %1, %3, %8
    SBUTTERFLY dqa, dq, %9, %2, %3
    SBUTTERFLY dqa, dq, %5, %7, %2
    SBUTTERFLY dqa, dq, %4, %6, %7
    SBUTTERFLY dqa, qdq, %1, %5, %6
    SBUTTERFLY dqa, qdq, %9, %4, %5
    SBUTTERFLY dqa, qdq, %8, %2, %4
    SBUTTERFLY dqa, qdq, %3, %7, %2
%endmacro

%macro LOAD_DIFF_8P 4  ; MMP, MMT, [pix1], [pix2]
    movq        %1, %3
    movq        %2, %4
    punpcklbw   %1, %2
    punpcklbw   %2, %2
    psubw       %1, %2
%endmacro

%macro LOAD_DIFF_4x8P 6 ; 4x dest, 2x temp
    LOAD_DIFF_8P %1, %5, [parm1q],          [parm3q]
    LOAD_DIFF_8P %2, %6, [parm1q+parm2q],   [parm3q+parm4q]
    LOAD_DIFF_8P %3, %5, [parm1q+2*parm2q], [parm3q+2*parm4q]
    LOAD_DIFF_8P %4, %6, [parm1q+r10],      [parm3q+r11]
%endmacro

%macro SUM1x8_SSE2 3    ; 01 junk sum
    pxor    %2, %2
    psubw   %2, %1
    pmaxsw  %1, %2
    paddusw %3, %1
%endmacro

%macro SUM4x4_SSE2 4    ; 02 13 junk sum
    pxor    %3, %3
    psubw   %3, %1
    pmaxsw  %1, %3

    pxor    %3, %3
    psubw   %3, %2
    pmaxsw  %2, %3

    paddusw %4, %1
    paddusw %4, %2
%endmacro

%macro SUM8x4_SSE2 7    ; a02 a13 junk1 b02 b13 junk2 (1=4 2=5 3=6) sum
    pxor    %3, %3
    pxor    %6, %6
    psubw   %3, %1
    psubw   %6, %4
    pmaxsw  %1, %3
    pmaxsw  %4, %6
    pxor    %3, %3
    pxor    %6, %6
    psubw   %3, %2
    psubw   %6, %5
    pmaxsw  %2, %3
    pmaxsw  %5, %6
    paddusw %1, %2
    paddusw %4, %5
    paddusw %7, %1
    paddusw %7, %4
%endmacro

%macro SUM8x4_SSSE3 7    ; a02 a13 . b02 b13 . sum
    pabsw   %1, %1
    pabsw   %2, %2
    pabsw   %4, %4
    pabsw   %5, %5
    paddusw %1, %2
    paddusw %4, %5
    paddusw %7, %1
    paddusw %7, %4
%endmacro

%macro SATD_TWO_SSE2 0
    LOAD_DIFF_4x8P    xmm0, xmm1, xmm2, xmm3, xmm4, xmm5
    lea     parm1q, [parm1q+4*parm2q]
    lea     parm3q, [parm3q+4*parm4q]
    HADAMARD1x4       xmm0, xmm1, xmm2, xmm3
    TRANSPOSE2x4x4W   xmm0, xmm1, xmm2, xmm3, xmm4
    HADAMARD1x4       xmm0, xmm1, xmm2, xmm3
    SUM8x4            xmm0, xmm1, xmm4, xmm2, xmm3, xmm5, xmm6
%endmacro

%macro SATD_START 0
    pxor    xmm6, xmm6
    lea     r10,  [3*parm2q]
    lea     r11,  [3*parm4q]
%endmacro

%macro SATD_END 0
    psrlw   xmm6, 1
    HADDW   xmm6, xmm7
    movd    eax,  xmm6
    ret
%endmacro

%macro SATDS 1
;-----------------------------------------------------------------------------
;   int x264_pixel_satd_16x16_sse2 (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
cglobal x264_pixel_satd_16x16_%1
    SATD_START
    mov     r8,  rdi
    mov     r9,  rdx
    SATD_TWO_SSE2
    SATD_TWO_SSE2
    SATD_TWO_SSE2
    SATD_TWO_SSE2
    lea     rdi, [r8+8]
    lea     rdx, [r9+8]
    SATD_TWO_SSE2
    SATD_TWO_SSE2
    SATD_TWO_SSE2
    SATD_TWO_SSE2
    SATD_END

;-----------------------------------------------------------------------------
;   int x264_pixel_satd_8x16_sse2 (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
cglobal x264_pixel_satd_8x16_%1
    SATD_START
    SATD_TWO_SSE2
    SATD_TWO_SSE2
    SATD_TWO_SSE2
    SATD_TWO_SSE2
    SATD_END

;-----------------------------------------------------------------------------
;   int x264_pixel_satd_16x8_sse2 (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
cglobal x264_pixel_satd_16x8_%1
    SATD_START
    mov     r8,  rdi
    mov     r9,  rdx
    SATD_TWO_SSE2
    SATD_TWO_SSE2
    lea     rdi, [r8+8]
    lea     rdx, [r9+8]
    SATD_TWO_SSE2
    SATD_TWO_SSE2
    SATD_END

;-----------------------------------------------------------------------------
;   int x264_pixel_satd_8x8_sse2 (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
cglobal x264_pixel_satd_8x8_%1
    SATD_START
    SATD_TWO_SSE2
    SATD_TWO_SSE2
    SATD_END

;-----------------------------------------------------------------------------
;   int x264_pixel_satd_8x4_sse2 (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
cglobal x264_pixel_satd_8x4_%1
    SATD_START
    SATD_TWO_SSE2
    SATD_END


;-----------------------------------------------------------------------------
;   int x264_pixel_sa8d_8x8_sse2( uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
cglobal x264_pixel_sa8d_8x8_%1
    lea  r10, [3*parm2q]
    lea  r11, [3*parm4q]
    LOAD_DIFF_4x8P xmm0, xmm1, xmm2, xmm3, xmm8, xmm8
    lea  parm1q, [parm1q+4*parm2q]
    lea  parm3q, [parm3q+4*parm4q]
    LOAD_DIFF_4x8P xmm4, xmm5, xmm6, xmm7, xmm8, xmm8

    HADAMARD1x8  xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7
    TRANSPOSE8x8 xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm8
    HADAMARD1x8  xmm0, xmm5, xmm7, xmm3, xmm8, xmm4, xmm2, xmm1

    pxor            xmm10, xmm10
    SUM8x4          xmm0, xmm1, xmm6, xmm2, xmm3, xmm9, xmm10
    SUM8x4          xmm4, xmm5, xmm6, xmm7, xmm8, xmm9, xmm10
    psrlw           xmm10, 1
    HADDW           xmm10, xmm0
    movd eax, xmm10
    add r8d, eax ; preserve rounding for 16x16
    add eax, 1
    shr eax, 1
    ret

;-----------------------------------------------------------------------------
;   int x264_pixel_sa8d_16x16_sse2( uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
;; violates calling convention
cglobal x264_pixel_sa8d_16x16_%1
    xor  r8d, r8d
    call x264_pixel_sa8d_8x8_%1 ; pix[0]
    lea  parm1q, [parm1q+4*parm2q]
    lea  parm3q, [parm3q+4*parm4q]
    call x264_pixel_sa8d_8x8_%1 ; pix[8*stride]
    lea  r10, [3*parm2q-2]
    lea  r11, [3*parm4q-2]
    shl  r10, 2
    shl  r11, 2
    sub  parm1q, r10
    sub  parm3q, r11
    call x264_pixel_sa8d_8x8_%1 ; pix[8]
    lea  parm1q, [parm1q+4*parm2q]
    lea  parm3q, [parm3q+4*parm4q]
    call x264_pixel_sa8d_8x8_%1 ; pix[8*stride+8]
    mov  eax, r8d
    add  eax, 1
    shr  eax, 1
    ret
%endmacro ; SATDS

%define SUM8x4 SUM8x4_SSE2
SATDS sse2
%ifdef HAVE_SSE3
%define SUM8x4 SUM8x4_SSSE3
SATDS ssse3
%endif



;-----------------------------------------------------------------------------
;  void x264_intra_sa8d_x3_8x8_core_sse2( uint8_t *fenc, int16_t edges[2][8], int *res )
;-----------------------------------------------------------------------------
cglobal x264_intra_sa8d_x3_8x8_core_sse2
    ; 8x8 hadamard
    pxor        xmm4, xmm4
    movq        xmm0, [parm1q+0*FENC_STRIDE]
    movq        xmm7, [parm1q+1*FENC_STRIDE]
    movq        xmm6, [parm1q+2*FENC_STRIDE]
    movq        xmm3, [parm1q+3*FENC_STRIDE]
    movq        xmm5, [parm1q+4*FENC_STRIDE]
    movq        xmm1, [parm1q+5*FENC_STRIDE]
    movq        xmm8, [parm1q+6*FENC_STRIDE]
    movq        xmm2, [parm1q+7*FENC_STRIDE]
    punpcklbw   xmm0, xmm4
    punpcklbw   xmm7, xmm4
    punpcklbw   xmm6, xmm4
    punpcklbw   xmm3, xmm4
    punpcklbw   xmm5, xmm4
    punpcklbw   xmm1, xmm4
    punpcklbw   xmm8, xmm4
    punpcklbw   xmm2, xmm4
    HADAMARD1x8 xmm0, xmm7, xmm6, xmm3, xmm5, xmm1, xmm8, xmm2
    TRANSPOSE8x8 xmm0, xmm7, xmm6, xmm3, xmm5, xmm1, xmm8, xmm2, xmm4
    HADAMARD1x8 xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7

    ; dc
    movzx       edi, word [parm2q+0]
    add          di, word [parm2q+16]
    add         edi, 8
    and         edi, -16
    shl         edi, 2

    pxor        xmm15, xmm15
    movdqa      xmm8, xmm2
    movdqa      xmm9, xmm3
    movdqa      xmm10, xmm4
    movdqa      xmm11, xmm5
    SUM8x4_SSE2 xmm8, xmm9, xmm12, xmm10, xmm11, xmm13, xmm15
    movdqa      xmm8, xmm6
    movdqa      xmm9, xmm7
    SUM4x4_SSE2 xmm8, xmm9, xmm10, xmm15
    movdqa      xmm8, xmm1
    SUM1x8_SSE2 xmm8, xmm10, xmm15
    movdqa      xmm14, xmm15 ; 7x8 sum

    movdqa      xmm8, [parm2q+0] ; left edge
    movd        xmm9, edi
    psllw       xmm8, 3
    psubw       xmm8, xmm0
    psubw       xmm9, xmm0
    SUM1x8_SSE2 xmm8, xmm10, xmm14
    SUM1x8_SSE2 xmm9, xmm11, xmm15 ; 1x8 sum
    punpcklwd   xmm0, xmm1
    punpcklwd   xmm2, xmm3
    punpcklwd   xmm4, xmm5
    punpcklwd   xmm6, xmm7
    punpckldq   xmm0, xmm2
    punpckldq   xmm4, xmm6
    punpcklqdq  xmm0, xmm4 ; transpose
    movdqa      xmm1, [parm2q+16] ; top edge
    movdqa      xmm2, xmm15
    psllw       xmm1, 3
    psrldq      xmm2, 2     ; 8x7 sum
    psubw       xmm0, xmm1  ; 8x1 sum
    SUM1x8_SSE2 xmm0, xmm1, xmm2

    HADDW       xmm14, xmm3
    movd        eax, xmm14
    add         eax, 2
    shr         eax, 2
    mov         [parm3q+4], eax ; i8x8_h sa8d
    HADDW       xmm15, xmm4
    movd        eax, xmm15
    add         eax, 2
    shr         eax, 2
    mov         [parm3q+8], eax ; i8x8_dc sa8d
    HADDW       xmm2, xmm5
    movd        eax, xmm2
    add         eax, 2
    shr         eax, 2
    mov         [parm3q+0], eax ; i8x8_v sa8d

    ret



;-----------------------------------------------------------------------------
; void x264_pixel_ssim_4x4x2_core_sse2( const uint8_t *pix1, int stride1,
;                                       const uint8_t *pix2, int stride2, int sums[2][4] )
;-----------------------------------------------------------------------------
cglobal x264_pixel_ssim_4x4x2_core_sse2
    pxor      xmm0, xmm0
    pxor      xmm1, xmm1
    pxor      xmm2, xmm2
    pxor      xmm3, xmm3
    pxor      xmm4, xmm4
    movdqa    xmm8, [pw_1 GLOBAL]
%rep 4
    movq      xmm5, [parm1q]
    movq      xmm6, [parm3q]
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
    add       parm1q, parm2q
    add       parm3q, parm4q
%endrep
    ; PHADDW xmm1, xmm2
    ; PHADDD xmm3, xmm4
    pshufd    xmm5, xmm3, 0xB1
    pmaddwd   xmm1, xmm8
    pmaddwd   xmm2, xmm8
    pshufd    xmm6, xmm4, 0xB1
    packssdw  xmm1, xmm2
    paddd     xmm3, xmm5
    pshufd    xmm1, xmm1, 0xD8
    paddd     xmm4, xmm6
    pmaddwd   xmm1, xmm8
    movdqa    xmm5, xmm3
    punpckldq xmm3, xmm4
    punpckhdq xmm5, xmm4
    movq      [parm5q+ 0], xmm1
    movq      [parm5q+ 8], xmm3
    psrldq    xmm1, 8
    movq      [parm5q+16], xmm1
    movq      [parm5q+24], xmm5
    ret

;-----------------------------------------------------------------------------
; float x264_pixel_ssim_end_sse2( int sum0[5][4], int sum1[5][4], int width )
;-----------------------------------------------------------------------------
cglobal x264_pixel_ssim_end4_sse2
    movdqa   xmm0, [parm1q+ 0]
    movdqa   xmm1, [parm1q+16]
    movdqa   xmm2, [parm1q+32]
    movdqa   xmm3, [parm1q+48]
    movdqa   xmm4, [parm1q+64]
    paddd    xmm0, [parm2q+ 0]
    paddd    xmm1, [parm2q+16]
    paddd    xmm2, [parm2q+32]
    paddd    xmm3, [parm2q+48]
    paddd    xmm4, [parm2q+64]
    paddd    xmm0, xmm1
    paddd    xmm1, xmm2
    paddd    xmm2, xmm3
    paddd    xmm3, xmm4
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

    neg      parm3q
%ifdef __PIC__
    lea      rax,  [mask_ff + 16 GLOBAL]
    movdqu   xmm3, [rax + parm3q*4]
%else
    movdqu   xmm3, [mask_ff + parm3q*4 + 16]
%endif
    pand     xmm1, xmm3
    movhlps  xmm0, xmm1
    addps    xmm0, xmm1
    pshuflw  xmm1, xmm0, 0xE
    addss    xmm0, xmm1
    ret


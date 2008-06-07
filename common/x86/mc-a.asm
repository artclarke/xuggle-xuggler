;*****************************************************************************
;* mc-a.asm: h264 encoder library
;*****************************************************************************
;* Copyright (C) 2003-2008 x264 project
;*
;* Authors: Loren Merritt <lorenm@u.washington.edu>
;*          Laurent Aimar <fenrir@via.ecp.fr>
;*          Min Chen <chenm001.163.com>
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

pw_4:  times 4 dw  4
pw_8:  times 4 dw  8
pw_32: times 8 dw 32
pw_64: times 8 dw 64
sw_64: dd 64

SECTION .text

;=============================================================================
; pixel avg
;=============================================================================

;-----------------------------------------------------------------------------
; void x264_pixel_avg_4x4_mmxext( uint8_t *dst, int dst_stride,
;                                 uint8_t *src, int src_stride );
;-----------------------------------------------------------------------------
%macro AVGH 3
%assign function_align 8 ; the whole function fits in 8 bytes, so a larger align just wastes space
cglobal x264_pixel_avg_%1x%2_%3
    mov eax, %2
    jmp x264_pixel_avg_w%1_%3
%assign function_align 16
%endmacro

;-----------------------------------------------------------------------------
; void x264_pixel_avg_w4_mmxext( uint8_t *dst, int dst_stride,
;                                uint8_t *src, int src_stride,
;                                int height );
;-----------------------------------------------------------------------------
%ifdef ARCH_X86_64
    %define t0 r0
    %define t1 r1
    %define t2 r2
    %define t3 r3
    %macro AVG_START 1
        cglobal %1, 4,5
        .height_loop:
    %endmacro
%else
    %define t0 r1
    %define t1 r2
    %define t2 r3
    %define t3 r4
    %macro AVG_START 1
        cglobal %1, 0,5
        mov t0, r0m
        mov t1, r1m
        mov t2, r2m
        mov t3, r3m
        .height_loop:
    %endmacro
%endif

%macro AVG_END 0
    sub    eax, 2
    lea    t2, [t2+t3*2]
    lea    t0, [t0+t1*2]
    jg     .height_loop
    REP_RET
%endmacro

AVG_START x264_pixel_avg_w4_mmxext
    movd   mm0, [t2]
    movd   mm1, [t2+t3]
    pavgb  mm0, [t0]
    pavgb  mm1, [t0+t1]
    movd   [t0], mm0
    movd   [t0+t1], mm1
AVG_END

AVGH 4, 8, mmxext
AVGH 4, 4, mmxext
AVGH 4, 2, mmxext

AVG_START x264_pixel_avg_w8_mmxext
    movq   mm0, [t2]
    movq   mm1, [t2+t3]
    pavgb  mm0, [t0]
    pavgb  mm1, [t0+t1]
    movq   [t0], mm0
    movq   [t0+t1], mm1
AVG_END

AVGH 8, 16, mmxext
AVGH 8, 8,  mmxext
AVGH 8, 4,  mmxext

AVG_START x264_pixel_avg_w16_mmxext
    movq   mm0, [t2  ]
    movq   mm1, [t2+8]
    movq   mm2, [t2+t3  ]
    movq   mm3, [t2+t3+8]
    pavgb  mm0, [t0  ]
    pavgb  mm1, [t0+8]
    pavgb  mm2, [t0+t1  ]
    pavgb  mm3, [t0+t1+8]
    movq   [t0  ], mm0
    movq   [t0+8], mm1
    movq   [t0+t1  ], mm2
    movq   [t0+t1+8], mm3
AVG_END

AVGH 16, 16, mmxext
AVGH 16, 8,  mmxext

AVG_START x264_pixel_avg_w16_sse2
    movdqu xmm0, [t2]
    movdqu xmm1, [t2+t3]
    pavgb  xmm0, [t0]
    pavgb  xmm1, [t0+t1]
    movdqa [t0], xmm0
    movdqa [t0+t1], xmm1
AVG_END

AVGH 16, 16, sse2
AVGH 16, 8,  sse2



;=============================================================================
; pixel avg2
;=============================================================================

;-----------------------------------------------------------------------------
; void x264_pixel_avg2_w4_mmxext( uint8_t *dst, int dst_stride,
;                                 uint8_t *src1, int src_stride,
;                                 uint8_t *src2, int height );
;-----------------------------------------------------------------------------
%macro AVG2_W8 2
cglobal x264_pixel_avg2_w%1_mmxext, 6,7
    sub    r4, r2
    lea    r6, [r4+r3]
.height_loop:
    %2     mm0, [r2]
    %2     mm1, [r2+r3]
    pavgb  mm0, [r2+r4]
    pavgb  mm1, [r2+r6]
    %2     [r0], mm0
    %2     [r0+r1], mm1
    sub    r5d, 2
    lea    r2, [r2+r3*2]
    lea    r0, [r0+r1*2]
    jg     .height_loop
    REP_RET
%endmacro

AVG2_W8 4, movd
AVG2_W8 8, movq

%macro AVG2_W16 2
cglobal x264_pixel_avg2_w%1_mmxext, 6,7
    sub    r4, r2
    lea    r6, [r4+r3]
.height_loop:
    movq   mm0, [r2]
    %2     mm1, [r2+8]
    movq   mm2, [r2+r3]
    %2     mm3, [r2+r3+8]
    pavgb  mm0, [r2+r4]
    pavgb  mm1, [r2+r4+8]
    pavgb  mm2, [r2+r6]
    pavgb  mm3, [r2+r6+8]
    movq   [r0], mm0
    %2     [r0+8], mm1
    movq   [r0+r1], mm2
    %2     [r0+r1+8], mm3
    lea    r2, [r2+r3*2]
    lea    r0, [r0+r1*2]
    sub    r5d, 2
    jg     .height_loop
    REP_RET
%endmacro

AVG2_W16 12, movd
AVG2_W16 16, movq

cglobal x264_pixel_avg2_w20_mmxext, 6,7
    sub    r4, r2
    lea    r6, [r4+r3]
.height_loop:
    movq   mm0, [r2]
    movq   mm1, [r2+8]
    movd   mm2, [r2+16]
    movq   mm3, [r2+r3]
    movq   mm4, [r2+r3+8]
    movd   mm5, [r2+r3+16]
    pavgb  mm0, [r2+r4]
    pavgb  mm1, [r2+r4+8]
    pavgb  mm2, [r2+r4+16]
    pavgb  mm3, [r2+r6]
    pavgb  mm4, [r2+r6+8]
    pavgb  mm5, [r2+r6+16]
    movq   [r0], mm0
    movq   [r0+8], mm1
    movd   [r0+16], mm2
    movq   [r0+r1], mm3
    movq   [r0+r1+8], mm4
    movd   [r0+r1+16], mm5
    lea    r2, [r2+r3*2]
    lea    r0, [r0+r1*2]
    sub    r5d, 2
    jg     .height_loop
    REP_RET

cglobal x264_pixel_avg2_w16_sse2, 6,7
    sub    r4, r2
    lea    r6, [r4+r3]
.height_loop:
    movdqu xmm0, [r2]
    movdqu xmm2, [r2+r3]
    movdqu xmm1, [r2+r4]
    movdqu xmm3, [r2+r6]
    pavgb  xmm0, xmm1
    pavgb  xmm2, xmm3
    movdqa [r0], xmm0
    movdqa [r0+r1], xmm2
    lea    r2, [r2+r3*2]
    lea    r0, [r0+r1*2]
    sub    r5d, 2
    jg     .height_loop
    REP_RET

cglobal x264_pixel_avg2_w20_sse2, 6,7
    sub    r4, r2
    lea    r6, [r4+r3]
.height_loop:
    movdqu xmm0, [r2]
    movdqu xmm2, [r2+r3]
    movdqu xmm1, [r2+r4]
    movdqu xmm3, [r2+r6]
    movd   mm4,  [r2+16]
    movd   mm5,  [r2+r3+16]
    pavgb  xmm0, xmm1
    pavgb  xmm2, xmm3
    pavgb  mm4,  [r2+r4+16]
    pavgb  mm5,  [r2+r6+16]
    movdqa [r0], xmm0
    movd   [r0+16], mm4
    movdqa [r0+r1], xmm2
    movd   [r0+r1+16], mm5
    lea    r2, [r2+r3*2]
    lea    r0, [r0+r1*2]
    sub    r5d, 2
    jg     .height_loop
    REP_RET

; Cacheline split code for processors with high latencies for loads
; split over cache lines.  See sad-a.asm for a more detailed explanation.
; This particular instance is complicated by the fact that src1 and src2
; can have different alignments.  For simplicity and code size, only the
; MMX cacheline workaround is used.  As a result, in the case of SSE2
; pixel_avg, the cacheline check functions calls the SSE2 version if there
; is no cacheline split, and the MMX workaround if there is.

%macro INIT_SHIFT 2
    and    eax, 7
    shl    eax, 3
%ifdef PIC32
    ; both versions work, but picgetgot is slower than gpr->mmx is slower than mem->mmx
    mov    r2, 64
    sub    r2, eax
    movd   %2, eax
    movd   %1, r2
%else
    movd   %1, [sw_64 GLOBAL]
    movd   %2, eax
    psubw  %1, %2
%endif
%endmacro

%macro AVG_CACHELINE_CHECK 3 ; width, cacheline, instruction set
cglobal x264_pixel_avg2_w%1_cache%2_%3, 0,0
    mov    eax, r2m
    and    eax, 0x1f|(%2>>1)
    cmp    eax, (32-%1)|(%2>>1)
    jle x264_pixel_avg2_w%1_%3
;w12 isn't needed because w16 is just as fast if there's no cacheline split
%if %1 == 12
    jmp x264_pixel_avg2_w16_cache_mmxext
%else
    jmp x264_pixel_avg2_w%1_cache_mmxext
%endif
%endmacro

%macro AVG_CACHELINE_START 0
    %assign stack_offset 0
    INIT_SHIFT mm6, mm7
    mov    eax, r4m
    INIT_SHIFT mm4, mm5
    PROLOGUE 6,6,0
    and    r2, ~7
    and    r4, ~7
    sub    r4, r2
.height_loop:
%endmacro

%macro AVG_CACHELINE_LOOP 2
    movq   mm0, [r2+8+%1]
    movq   mm1, [r2+%1]
    movq   mm2, [r2+r4+8+%1]
    movq   mm3, [r2+r4+%1]
    psllq  mm0, mm6
    psrlq  mm1, mm7
    psllq  mm2, mm4
    psrlq  mm3, mm5
    por    mm0, mm1
    por    mm2, mm3
    pavgb  mm0, mm2
    %2 [r0+%1], mm0
%endmacro

x264_pixel_avg2_w8_cache_mmxext:
    AVG_CACHELINE_START
    AVG_CACHELINE_LOOP 0, movq
    add    r2, r3
    add    r0, r1
    dec    r5d
    jg     .height_loop
    RET

x264_pixel_avg2_w16_cache_mmxext:
    AVG_CACHELINE_START
    AVG_CACHELINE_LOOP 0, movq
    AVG_CACHELINE_LOOP 8, movq
    add    r2, r3
    add    r0, r1
    dec    r5d
    jg .height_loop
    RET

x264_pixel_avg2_w20_cache_mmxext:
    AVG_CACHELINE_START
    AVG_CACHELINE_LOOP 0, movq
    AVG_CACHELINE_LOOP 8, movq
    AVG_CACHELINE_LOOP 16, movd
    add    r2, r3
    add    r0, r1
    dec    r5d
    jg .height_loop
    RET

%ifndef ARCH_X86_64
AVG_CACHELINE_CHECK  8, 32, mmxext
AVG_CACHELINE_CHECK 12, 32, mmxext
AVG_CACHELINE_CHECK 16, 32, mmxext
AVG_CACHELINE_CHECK 20, 32, mmxext
AVG_CACHELINE_CHECK 16, 64, mmxext
AVG_CACHELINE_CHECK 20, 64, mmxext
%endif

AVG_CACHELINE_CHECK  8, 64, mmxext
AVG_CACHELINE_CHECK 12, 64, mmxext
AVG_CACHELINE_CHECK 16, 64, sse2
AVG_CACHELINE_CHECK 20, 64, sse2

;=============================================================================
; pixel copy
;=============================================================================

%macro COPY4 3
    %1  mm0, [r2]
    %1  mm1, [r2+r3]
    %1  mm2, [r2+r3*2]
    %1  mm3, [r2+%3]
    %1  [r0],      mm0
    %1  [r0+r1],   mm1
    %1  [r0+r1*2], mm2
    %1  [r0+%2],   mm3
%endmacro

;-----------------------------------------------------------------------------
; void x264_mc_copy_w4_mmx( uint8_t *dst, int i_dst_stride,
;                           uint8_t *src, int i_src_stride, int i_height )
;-----------------------------------------------------------------------------
cglobal x264_mc_copy_w4_mmx, 4,6
    cmp     dword r4m, 4
    lea     r5, [r3*3]
    lea     r4, [r1*3]
    je .end
    COPY4 movd, r4, r5
    lea     r2, [r2+r3*4]
    lea     r0, [r0+r1*4]
.end:
    COPY4 movd, r4, r5
    RET

cglobal x264_mc_copy_w8_mmx, 5,7
    lea     r6, [r3*3]
    lea     r5, [r1*3]
.height_loop:
    COPY4 movq, r5, r6
    lea     r2, [r2+r3*4]
    lea     r0, [r0+r1*4]
    sub     r4d, 4
    jg      .height_loop
    REP_RET

cglobal x264_mc_copy_w16_mmx, 5,7
    lea     r6, [r3*3]
    lea     r5, [r1*3]
.height_loop:
    movq    mm0, [r2]
    movq    mm1, [r2+8]
    movq    mm2, [r2+r3]
    movq    mm3, [r2+r3+8]
    movq    mm4, [r2+r3*2]
    movq    mm5, [r2+r3*2+8]
    movq    mm6, [r2+r6]
    movq    mm7, [r2+r6+8]
    movq    [r0], mm0
    movq    [r0+8], mm1
    movq    [r0+r1], mm2
    movq    [r0+r1+8], mm3
    movq    [r0+r1*2], mm4
    movq    [r0+r1*2+8], mm5
    movq    [r0+r5], mm6
    movq    [r0+r5+8], mm7
    lea     r2, [r2+r3*4]
    lea     r0, [r0+r1*4]
    sub     r4d, 4
    jg      .height_loop
    REP_RET

%macro COPY_W16_SSE2 2
cglobal %1, 5,7
    lea     r6, [r3*3]
    lea     r5, [r1*3]
.height_loop:
    %2      xmm0, [r2]
    %2      xmm1, [r2+r3]
    %2      xmm2, [r2+r3*2]
    %2      xmm3, [r2+r6]
    movdqa  [r0], xmm0
    movdqa  [r0+r1], xmm1
    movdqa  [r0+r1*2], xmm2
    movdqa  [r0+r5], xmm3
    lea     r2, [r2+r3*4]
    lea     r0, [r0+r1*4]
    sub     r4d, 4
    jg      .height_loop
    REP_RET
%endmacro

COPY_W16_SSE2 x264_mc_copy_w16_sse2, movdqu
; cacheline split with mmx has too much overhead; the speed benefit is near-zero.
; but with SSE3 the overhead is zero, so there's no reason not to include it.
COPY_W16_SSE2 x264_mc_copy_w16_sse3, lddqu
COPY_W16_SSE2 x264_mc_copy_w16_aligned_sse2, movdqa



;=============================================================================
; weighted prediction
;=============================================================================
; implicit bipred only:
; assumes log2_denom = 5, offset = 0, weight1 + weight2 = 64

%macro SPLATW 2
%if regsize==16
    pshuflw  %1, %2, 0
    movlhps  %1, %1
%else
    pshufw   %1, %2, 0
%endif
%endmacro

%macro BIWEIGHT 2
    movh      m0, %1
    movh      m1, %2
    punpcklbw m0, m7
    punpcklbw m1, m7
    pmullw    m0, m4
    pmullw    m1, m5
    paddw     m0, m1
    paddw     m0, m6
    psraw     m0, 6
    pmaxsw    m0, m7
    packuswb  m0, m0
    movh      %1,  m0
%endmacro

%macro BIWEIGHT_START 1
%ifidn r4m, r4d
    movd    m4, r4m
    SPLATW  m4, m4   ; weight_dst
%else
    SPLATW  m4, r4m
%endif
    picgetgot r4
    mova    m5, [pw_64 GLOBAL]
    psubw   m5, m4      ; weight_src
    mova    m6, [pw_32 GLOBAL] ; rounding
    pxor    m7, m7
%if %1
%ifidn r5m, r5d
    %define t0 r5d
%else
    %define t0 r4d
    mov  r4d, r5m
%endif
%endif
.height_loop:
%endmacro

;-----------------------------------------------------------------------------
; int x264_pixel_avg_weight_w16_mmxext( uint8_t *dst, int, uint8_t *src, int, int i_weight, int )
;-----------------------------------------------------------------------------
cglobal x264_pixel_avg_weight_4x4_mmxext, 4,4,1
    BIWEIGHT_START 0
    BIWEIGHT  [r0     ], [r2     ]
    BIWEIGHT  [r0+r1  ], [r2+r3  ]
    BIWEIGHT  [r0+r1*2], [r2+r3*2]
    add  r0, r1
    add  r2, r3
    BIWEIGHT  [r0+r1*2], [r2+r3*2]
    RET

%macro AVG_WEIGHT 2
cglobal x264_pixel_avg_weight_w%2_%1, 4,5
    BIWEIGHT_START 1
%assign x 0
%rep %2*2/regsize
    BIWEIGHT  [r0+x], [r2+x]
%assign x x+regsize/2
%endrep
    add  r0, r1
    add  r2, r3
    dec  t0
    jg   .height_loop
    REP_RET
%endmacro

INIT_MMX
AVG_WEIGHT mmxext, 8
AVG_WEIGHT mmxext, 16
INIT_XMM
AVG_WEIGHT sse2, 8
AVG_WEIGHT sse2, 16



;=============================================================================
; prefetch
;=============================================================================
; FIXME assumes 64 byte cachelines

;-----------------------------------------------------------------------------
; void x264_prefetch_fenc_mmxext( uint8_t *pix_y, int stride_y, 
;                                 uint8_t *pix_uv, int stride_uv, int mb_x )
;-----------------------------------------------------------------------------
%ifdef ARCH_X86_64
cglobal x264_prefetch_fenc_mmxext, 5,5
    mov    eax, r4d
    and    eax, 3
    imul   eax, r1d
    lea    r0,  [r0+rax*4+64]
    prefetcht0  [r0]
    prefetcht0  [r0+r1]
    lea    r0,  [r0+r1*2]
    prefetcht0  [r0]
    prefetcht0  [r0+r1]

    and    r4d, 6
    imul   r4d, r3d
    lea    r2,  [r2+r4+64]
    prefetcht0  [r2]
    prefetcht0  [r2+r3]
    ret

%else
cglobal x264_prefetch_fenc_mmxext
    mov    r2, [esp+20]
    mov    r1, [esp+8]
    mov    r0, [esp+4]
    and    r2, 3
    imul   r2, r1
    lea    r0, [r0+r2*4+64]
    prefetcht0 [r0]
    prefetcht0 [r0+r1]
    lea    r0, [r0+r1*2]
    prefetcht0 [r0]
    prefetcht0 [r0+r1]

    mov    r2, [esp+20]
    mov    r1, [esp+16]
    mov    r0, [esp+12]
    and    r2, 6
    imul   r2, r1
    lea    r0, [r0+r2+64]
    prefetcht0 [r0]
    prefetcht0 [r0+r1]
    ret
%endif ; ARCH_X86_64

;-----------------------------------------------------------------------------
; void x264_prefetch_ref_mmxext( uint8_t *pix, int stride, int parity )
;-----------------------------------------------------------------------------
cglobal x264_prefetch_ref_mmxext, 3,3
    dec    r2d
    and    r2d, r1d
    lea    r0,  [r0+r2*8+64]
    lea    r2,  [r1*3]
    prefetcht0  [r0]
    prefetcht0  [r0+r1]
    prefetcht0  [r0+r1*2]
    prefetcht0  [r0+r2]
    lea    r0,  [r0+r1*4]
    prefetcht0  [r0]
    prefetcht0  [r0+r1]
    prefetcht0  [r0+r1*2]
    prefetcht0  [r0+r2]
    ret



;=============================================================================
; chroma MC
;=============================================================================

;-----------------------------------------------------------------------------
; void x264_mc_chroma_mmxext( uint8_t *dst, int i_dst_stride,
;                             uint8_t *src, int i_src_stride,
;                             int dx, int dy,
;                             int i_width, int i_height )
;-----------------------------------------------------------------------------
cglobal x264_mc_chroma_mmxext, 0,6,1
%ifdef ARCH_X86_64
    %define t0   r10d
%else
    %define t0   r1d
%endif
    movifnidn r2d, r2m
    movifnidn r3d, r3m
    movifnidn r4d, r4m
    movifnidn r5d, r5m
    mov     eax, r5d
    mov     t0,  r4d
    sar     eax, 3
    sar     t0,  3
    imul    eax, r3d
    pxor    mm3, mm3
    add     eax, t0
    movsxdifnidn rax, eax
    add     r2,  rax            ; src += (dx>>3) + (dy>>3) * src_stride
    and     r4d, 7              ; dx &= 7
    je      .mc1d
    and     r5d, 7              ; dy &= 7
    je      .mc1d

    movd    mm0, r4d
    movd    mm1, r5d
    pshufw  mm5, mm0, 0         ; mm5 = dx
    pshufw  mm6, mm1, 0         ; mm6 = dy

    movq    mm4, [pw_8 GLOBAL]
    movq    mm0, mm4
    psubw   mm4, mm5            ; mm4 = 8-dx
    psubw   mm0, mm6            ; mm0 = 8-dy

    movq    mm7, mm5
    pmullw  mm5, mm0            ; mm5 = dx*(8-dy) =     cB
    pmullw  mm7, mm6            ; mm7 = dx*dy =         cD
    pmullw  mm6, mm4            ; mm6 = (8-dx)*dy =     cC
    pmullw  mm4, mm0            ; mm4 = (8-dx)*(8-dy) = cA

    mov     r4d, r7m
%ifdef ARCH_X86_64
    mov     r10, r0
    mov     r11, r2
%else
    mov     r0,  r0m
    mov     r1,  r1m
    mov     r5,  r2
%endif

ALIGN 4
.height_loop:

    movd    mm1, [r2+r3]
    movd    mm0, [r2]
    punpcklbw mm1, mm3          ; 00 px1 | 00 px2 | 00 px3 | 00 px4
    punpcklbw mm0, mm3
    pmullw  mm1, mm6            ; 2nd line * cC
    pmullw  mm0, mm4            ; 1st line * cA
    paddw   mm0, mm1            ; mm0 <- result

    movd    mm2, [r2+1]
    movd    mm1, [r2+r3+1]
    punpcklbw mm2, mm3
    punpcklbw mm1, mm3

    paddw   mm0, [pw_32 GLOBAL]

    pmullw  mm2, mm5            ; line * cB
    pmullw  mm1, mm7            ; line * cD
    paddw   mm0, mm2
    paddw   mm0, mm1
    psrlw   mm0, 6

    packuswb mm0, mm3           ; 00 00 00 00 px1 px2 px3 px4
    movd    [r0], mm0

    add     r2,  r3
    add     r0,  r1             ; i_dst_stride
    dec     r4d
    jnz     .height_loop

    sub dword r6m, 8
    jnz     .finish             ; width != 8 so assume 4

%ifdef ARCH_X86_64
    lea     r0, [r10+4]         ; dst
    lea     r2, [r11+4]         ; src
%else
    mov     r0,  r0m
    lea     r2, [r5+4]
    add     r0,  4
%endif
    mov     r4d, r7m            ; i_height
    jmp     .height_loop

ALIGN 4
.mc1d:
    mov       eax, r4d
    or        eax, r5d
    and       eax, 7
    cmp       r4d, 0
    mov       r5d, 1
    cmove     r5,  r3           ; pel_offset = dx ? 1 : src_stride
    movd      mm6, eax
    movq      mm5, [pw_8 GLOBAL]
    pshufw    mm6, mm6, 0
    movq      mm7, [pw_4 GLOBAL]
    psubw     mm5, mm6

    cmp dword r6m, 8
    movifnidn r0d, r0m
    movifnidn r1d, r1m
    mov       r4d, r7m
    je .height_loop1_w8

ALIGN 4
.height_loop1_w4:
    movd      mm0, [r2+r5]
    movd      mm1, [r2]
    punpcklbw mm0, mm3
    punpcklbw mm1, mm3
    pmullw    mm0, mm6
    pmullw    mm1, mm5
    paddw     mm0, mm7
    paddw     mm0, mm1
    psrlw     mm0, 3
    packuswb  mm0, mm3
    movd     [r0], mm0
    add       r2,  r3
    add       r0,  r1
    dec       r4d
    jnz .height_loop1_w4
.finish:
    REP_RET

ALIGN 4
.height_loop1_w8:
    movq      mm0, [r2+r5]
    movq      mm1, [r2]
    movq      mm2, mm0
    movq      mm4, mm1
    punpcklbw mm0, mm3
    punpcklbw mm1, mm3
    punpckhbw mm2, mm3
    punpckhbw mm4, mm3
    pmullw    mm0, mm6
    pmullw    mm1, mm5
    pmullw    mm2, mm6
    pmullw    mm4, mm5
    paddw     mm0, mm7
    paddw     mm2, mm7
    paddw     mm0, mm1
    paddw     mm2, mm4
    psrlw     mm0, 3
    psrlw     mm2, 3
    packuswb  mm0, mm2
    movq     [r0], mm0
    add       r2,  r3
    add       r0,  r1
    dec       r4d
    jnz .height_loop1_w8
    REP_RET


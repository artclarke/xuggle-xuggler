;*****************************************************************************
;* mc-a.asm: h264 encoder library
;*****************************************************************************
;* Copyright (C) 2003-2008 x264 project
;*
;* Authors: Loren Merritt <lorenm@u.washington.edu>
;*          Jason Garrett-Glaser <darkshikari@gmail.com>
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
;* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111, USA.
;*****************************************************************************

%include "x86inc.asm"

SECTION_RODATA

pw_4:  times 8 dw  4
pw_8:  times 8 dw  8
pw_32: times 8 dw 32
pw_64: times 8 dw 64
sw_64: dd 64

SECTION .text

;=============================================================================
; weighted prediction
;=============================================================================
; implicit bipred only:
; assumes log2_denom = 5, offset = 0, weight1 + weight2 = 64
%ifdef ARCH_X86_64
    DECLARE_REG_TMP 0,1,2,3,4,5,10,11
    %macro AVG_START 0-1 0
        PROLOGUE 6,7,%1
%ifdef WIN64
        movsxd r5, r5d
%endif
        .height_loop:
    %endmacro
%else
    DECLARE_REG_TMP 1,2,3,4,5,6,1,2
    %macro AVG_START 0-1 0
        PROLOGUE 0,7,%1
        mov t0, r0m
        mov t1, r1m
        mov t2, r2m
        mov t3, r3m
        mov t4, r4m
        mov t5, r5m
        .height_loop:
    %endmacro
%endif

%macro SPLATW 2
%if mmsize==16
    pshuflw  %1, %2, 0
    punpcklqdq %1, %1
%else
    pshufw   %1, %2, 0
%endif
%endmacro

%macro BIWEIGHT_MMX 2
    movh      m0, %1
    movh      m1, %2
    punpcklbw m0, m5
    punpcklbw m1, m5
    pmullw    m0, m2
    pmullw    m1, m3
    paddw     m0, m1
    paddw     m0, m4
    psraw     m0, 6
%endmacro

%macro BIWEIGHT_START_MMX 0
    movd    m2, r6m
    SPLATW  m2, m2   ; weight_dst
    mova    m3, [pw_64 GLOBAL]
    psubw   m3, m2   ; weight_src
    mova    m4, [pw_32 GLOBAL] ; rounding
    pxor    m5, m5
%endmacro

%macro BIWEIGHT_SSSE3 2
    movh      m0, %1
    movh      m1, %2
    punpcklbw m0, m1
    pmaddubsw m0, m3
    paddw     m0, m4
    psraw     m0, 6
%endmacro

%macro BIWEIGHT_START_SSSE3 0
    movzx  t6d, byte r6m ; FIXME x86_64
    mov    t7d, 64
    sub    t7d, t6d
    shl    t7d, 8
    add    t6d, t7d
    movd    m3, t6d
    mova    m4, [pw_32 GLOBAL]
    SPLATW  m3, m3   ; weight_dst,src
%endmacro

%macro BIWEIGHT_ROW 4
    BIWEIGHT [%2], [%3]
%if %4==mmsize/2
    packuswb   m0, m0
    movh     [%1], m0
%else
    SWAP 0, 6
    BIWEIGHT [%2+mmsize/2], [%3+mmsize/2]
    packuswb   m6, m0
    mova     [%1], m6
%endif
%endmacro

;-----------------------------------------------------------------------------
; int x264_pixel_avg_weight_w16_mmxext( uint8_t *dst, int, uint8_t *src1, int, uint8_t *src2, int, int i_weight )
;-----------------------------------------------------------------------------
%macro AVG_WEIGHT 2-3 0
cglobal x264_pixel_avg_weight_w%2_%1
    BIWEIGHT_START
    AVG_START %3
%if %2==8 && mmsize==16
    BIWEIGHT [t2], [t4]
    SWAP 0, 6
    BIWEIGHT [t2+t3], [t4+t5]
    packuswb m6, m0
    movlps   [t0], m6
    movhps   [t0+t1], m6
%else
%assign x 0
%rep 1+%2/(mmsize*2)
    BIWEIGHT_ROW t0+x,    t2+x,    t4+x,    %2
    BIWEIGHT_ROW t0+x+t1, t2+x+t3, t4+x+t5, %2
%assign x x+mmsize
%endrep
%endif
    lea  t0, [t0+t1*2]
    lea  t2, [t2+t3*2]
    lea  t4, [t4+t5*2]
    sub  eax, 2
    jg   .height_loop
    REP_RET
%endmacro

%define BIWEIGHT BIWEIGHT_MMX
%define BIWEIGHT_START BIWEIGHT_START_MMX
INIT_MMX
AVG_WEIGHT mmxext, 4
AVG_WEIGHT mmxext, 8
AVG_WEIGHT mmxext, 16
INIT_XMM
%define x264_pixel_avg_weight_w4_sse2 x264_pixel_avg_weight_w4_mmxext
AVG_WEIGHT sse2, 8,  7
AVG_WEIGHT sse2, 16, 7
%define BIWEIGHT BIWEIGHT_SSSE3
%define BIWEIGHT_START BIWEIGHT_START_SSSE3
INIT_MMX
AVG_WEIGHT ssse3, 4
INIT_XMM
AVG_WEIGHT ssse3, 8,  7
AVG_WEIGHT ssse3, 16, 7



;=============================================================================
; pixel avg
;=============================================================================

;-----------------------------------------------------------------------------
; void x264_pixel_avg_4x4_mmxext( uint8_t *dst, int dst_stride,
;                                 uint8_t *src1, int src1_stride, uint8_t *src2, int src2_stride, int weight );
;-----------------------------------------------------------------------------
%macro AVGH 3
cglobal x264_pixel_avg_%1x%2_%3
    mov eax, %2
    cmp dword r6m, 32
    jne x264_pixel_avg_weight_w%1_%3
%if mmsize == 16 && %1 == 16
    test dword r4m, 15
    jz x264_pixel_avg_w%1_sse2
%endif
    jmp x264_pixel_avg_w%1_mmxext
%endmacro

;-----------------------------------------------------------------------------
; void x264_pixel_avg_w4_mmxext( uint8_t *dst, int dst_stride,
;                                uint8_t *src1, int src1_stride, uint8_t *src2, int src2_stride,
;                                int height, int weight );
;-----------------------------------------------------------------------------

%macro AVG_END 0
    sub    eax, 2
    lea    t4, [t4+t5*2]
    lea    t2, [t2+t3*2]
    lea    t0, [t0+t1*2]
    jg     .height_loop
    REP_RET
%endmacro

%macro AVG_FUNC 3
cglobal %1
    AVG_START
    %2     m0, [t2]
    %2     m1, [t2+t3]
    pavgb  m0, [t4]
    pavgb  m1, [t4+t5]
    %3     [t0], m0
    %3     [t0+t1], m1
    AVG_END
%endmacro

INIT_MMX
AVG_FUNC x264_pixel_avg_w4_mmxext, movd, movd
AVGH 4, 8, mmxext
AVGH 4, 4, mmxext
AVGH 4, 2, mmxext

AVG_FUNC x264_pixel_avg_w8_mmxext, movq, movq
AVGH 8, 16, mmxext
AVGH 8, 8,  mmxext
AVGH 8, 4,  mmxext

cglobal x264_pixel_avg_w16_mmxext
    AVG_START
    movq   mm0, [t2  ]
    movq   mm1, [t2+8]
    movq   mm2, [t2+t3  ]
    movq   mm3, [t2+t3+8]
    pavgb  mm0, [t4  ]
    pavgb  mm1, [t4+8]
    pavgb  mm2, [t4+t5  ]
    pavgb  mm3, [t4+t5+8]
    movq   [t0  ], mm0
    movq   [t0+8], mm1
    movq   [t0+t1  ], mm2
    movq   [t0+t1+8], mm3
    AVG_END

AVGH 16, 16, mmxext
AVGH 16, 8,  mmxext

INIT_XMM
AVG_FUNC x264_pixel_avg_w16_sse2, movdqu, movdqa
AVGH 16, 16, sse2
AVGH 16,  8, sse2
AVGH  8, 16, sse2
AVGH  8,  8, sse2
AVGH  8,  4, sse2
AVGH 16, 16, ssse3
AVGH 16,  8, ssse3
AVGH  8, 16, ssse3
AVGH  8,  8, ssse3
AVGH  8,  4, ssse3
INIT_MMX
AVGH  4,  8, ssse3
AVGH  4,  4, ssse3
AVGH  4,  2, ssse3



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

%macro AVG2_W20 1
cglobal x264_pixel_avg2_w20_%1, 6,7
    sub    r4, r2
    lea    r6, [r4+r3]
.height_loop:
    movdqu xmm0, [r2]
    movdqu xmm2, [r2+r3]
    movd   mm4,  [r2+16]
    movd   mm5,  [r2+r3+16]
%ifidn %1, sse2_misalign
    pavgb  xmm0, [r2+r4]
    pavgb  xmm2, [r2+r6]
%else
    movdqu xmm1, [r2+r4]
    movdqu xmm3, [r2+r6]
    pavgb  xmm0, xmm1
    pavgb  xmm2, xmm3
%endif
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
%endmacro

AVG2_W20 sse2
AVG2_W20 sse2_misalign

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
    movd   %1, [sw_64 GLOBAL]
    movd   %2, eax
    psubw  %1, %2
%endmacro

%macro AVG_CACHELINE_CHECK 3 ; width, cacheline, instruction set
cglobal x264_pixel_avg2_w%1_cache%2_%3
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
    PROLOGUE 6,6
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
    REP_RET

x264_pixel_avg2_w16_cache_mmxext:
    AVG_CACHELINE_START
    AVG_CACHELINE_LOOP 0, movq
    AVG_CACHELINE_LOOP 8, movq
    add    r2, r3
    add    r0, r1
    dec    r5d
    jg .height_loop
    REP_RET

x264_pixel_avg2_w20_cache_mmxext:
    AVG_CACHELINE_START
    AVG_CACHELINE_LOOP 0, movq
    AVG_CACHELINE_LOOP 8, movq
    AVG_CACHELINE_LOOP 16, movd
    add    r2, r3
    add    r0, r1
    dec    r5d
    jg .height_loop
    REP_RET

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

%macro COPY4 4
    %2  m0, [r2]
    %2  m1, [r2+r3]
    %2  m2, [r2+r3*2]
    %2  m3, [r2+%4]
    %1  [r0],      m0
    %1  [r0+r1],   m1
    %1  [r0+r1*2], m2
    %1  [r0+%3],   m3
%endmacro

INIT_MMX
;-----------------------------------------------------------------------------
; void x264_mc_copy_w4_mmx( uint8_t *dst, int i_dst_stride,
;                           uint8_t *src, int i_src_stride, int i_height )
;-----------------------------------------------------------------------------
cglobal x264_mc_copy_w4_mmx, 4,6
    cmp     dword r4m, 4
    lea     r5, [r3*3]
    lea     r4, [r1*3]
    je .end
    COPY4 movd, movd, r4, r5
    lea     r2, [r2+r3*4]
    lea     r0, [r0+r1*4]
.end:
    COPY4 movd, movd, r4, r5
    RET

cglobal x264_mc_copy_w8_mmx, 5,7
    lea     r6, [r3*3]
    lea     r5, [r1*3]
.height_loop:
    COPY4 movq, movq, r5, r6
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

INIT_XMM
%macro COPY_W16_SSE2 2
cglobal %1, 5,7
    lea     r6, [r3*3]
    lea     r5, [r1*3]
.height_loop:
    COPY4 movdqa, %2, r5, r6
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
    RET

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
    RET



;=============================================================================
; chroma MC
;=============================================================================

    %define t0 rax
%ifdef ARCH_X86_64
    %define t1 r10
%else
    %define t1 r1
%endif

%macro MC_CHROMA_START 0
    movifnidn r2,  r2mp
    movifnidn r3d, r3m
    movifnidn r4d, r4m
    movifnidn r5d, r5m
    mov       t0d, r5d
    mov       t1d, r4d
    sar       t0d, 3
    sar       t1d, 3
    imul      t0d, r3d
    add       t0d, t1d
    movsxdifnidn t0, t0d
    add       r2,  t0            ; src += (dx>>3) + (dy>>3) * src_stride
%endmacro

;-----------------------------------------------------------------------------
; void x264_mc_chroma_mmxext( uint8_t *dst, int dst_stride,
;                             uint8_t *src, int src_stride,
;                             int dx, int dy,
;                             int width, int height )
;-----------------------------------------------------------------------------
%macro MC_CHROMA 1-2 0
cglobal x264_mc_chroma_%1
%if mmsize == 16
    cmp dword r6m, 4
    jle x264_mc_chroma_mmxext
%endif
    PROLOGUE 0,6,%2
    MC_CHROMA_START
    pxor       m3, m3
    and       r4d, 7         ; dx &= 7
    jz .mc1dy
    and       r5d, 7         ; dy &= 7
    jz .mc1dx

    movd       m5, r4d
    movd       m6, r5d
    SPLATW     m5, m5        ; m5 = dx
    SPLATW     m6, m6        ; m6 = dy

    mova       m4, [pw_8 GLOBAL]
    mova       m0, m4
    psubw      m4, m5        ; m4 = 8-dx
    psubw      m0, m6        ; m0 = 8-dy

    mova       m7, m5
    pmullw     m5, m0        ; m5 = dx*(8-dy) =     cB
    pmullw     m7, m6        ; m7 = dx*dy =         cD
    pmullw     m6, m4        ; m6 = (8-dx)*dy =     cC
    pmullw     m4, m0        ; m4 = (8-dx)*(8-dy) = cA

    mov       r4d, r7m
%ifdef ARCH_X86_64
    mov       r10, r0
    mov       r11, r2
%else
    mov        r0, r0mp
    mov        r1, r1m
    mov        r5, r2
%endif

.loop2d:
    movh       m1, [r2+r3]
    movh       m0, [r2]
    punpcklbw  m1, m3        ; 00 px1 | 00 px2 | 00 px3 | 00 px4
    punpcklbw  m0, m3
    pmullw     m1, m6        ; 2nd line * cC
    pmullw     m0, m4        ; 1st line * cA
    paddw      m0, m1        ; m0 <- result

    movh       m2, [r2+1]
    movh       m1, [r2+r3+1]
    punpcklbw  m2, m3
    punpcklbw  m1, m3

    paddw      m0, [pw_32 GLOBAL]

    pmullw     m2, m5        ; line * cB
    pmullw     m1, m7        ; line * cD
    paddw      m0, m2
    paddw      m0, m1
    psrlw      m0, 6

    packuswb m0, m3          ; 00 00 00 00 px1 px2 px3 px4
    movh       [r0], m0

    add        r2,  r3
    add        r0,  r1       ; dst_stride
    dec        r4d
    jnz .loop2d

%if mmsize == 8
    sub dword r6m, 8
    jnz .finish              ; width != 8 so assume 4
%ifdef ARCH_X86_64
    lea        r0, [r10+4]   ; dst
    lea        r2, [r11+4]   ; src
%else
    mov        r0, r0mp
    lea        r2, [r5+4]
    add        r0, 4
%endif
    mov       r4d, r7m       ; height
    jmp .loop2d
%else
    REP_RET
%endif ; mmsize

.mc1dy:
    and       r5d, 7
    movd       m6, r5d
    mov        r5, r3        ; pel_offset = dx ? 1 : src_stride
    jmp .mc1d
.mc1dx:
    movd       m6, r4d
    mov       r5d, 1
.mc1d:
    mova       m5, [pw_8 GLOBAL]
    SPLATW     m6, m6
    mova       m7, [pw_4 GLOBAL]
    psubw      m5, m6
    movifnidn r0,  r0mp
    movifnidn r1d, r1m
    mov       r4d, r7m
%if mmsize == 8
    cmp dword r6m, 8
    je .loop1d_w8
%endif

.loop1d_w4:
    movh       m0, [r2+r5]
    movh       m1, [r2]
    punpcklbw  m0, m3
    punpcklbw  m1, m3
    pmullw     m0, m6
    pmullw     m1, m5
    paddw      m0, m7
    paddw      m0, m1
    psrlw      m0, 3
    packuswb   m0, m3
    movh     [r0], m0
    add        r2, r3
    add        r0, r1
    dec        r4d
    jnz .loop1d_w4
.finish:
    REP_RET

%if mmsize == 8
.loop1d_w8:
    movu       m0, [r2+r5]
    mova       m1, [r2]
    mova       m2, m0
    mova       m4, m1
    punpcklbw  m0, m3
    punpcklbw  m1, m3
    punpckhbw  m2, m3
    punpckhbw  m4, m3
    pmullw     m0, m6
    pmullw     m1, m5
    pmullw     m2, m6
    pmullw     m4, m5
    paddw      m0, m7
    paddw      m2, m7
    paddw      m0, m1
    paddw      m2, m4
    psrlw      m0, 3
    psrlw      m2, 3
    packuswb   m0, m2
    mova     [r0], m0
    add        r2, r3
    add        r0, r1
    dec        r4d
    jnz .loop1d_w8
    REP_RET
%endif ; mmsize
%endmacro ; MC_CHROMA

INIT_MMX
MC_CHROMA mmxext
INIT_XMM
MC_CHROMA sse2, 8

INIT_MMX
cglobal x264_mc_chroma_ssse3, 0,6,8
    MC_CHROMA_START
    and       r4d, 7
    and       r5d, 7
    mov       t0d, r4d
    shl       t0d, 8
    sub       t0d, r4d
    mov       r4d, 8
    add       t0d, 8
    sub       r4d, r5d
    imul      r5d, t0d ; (x*255+8)*y
    imul      r4d, t0d ; (x*255+8)*(8-y)
    cmp dword r6m, 4
    jg .width8
    mova       m5, [pw_32 GLOBAL]
    movd       m6, r5d
    movd       m7, r4d
    movifnidn r0,  r0mp
    movifnidn r1d, r1m
    movifnidn r4d, r7m
    SPLATW     m6, m6
    SPLATW     m7, m7
    movh       m0, [r2]
    punpcklbw  m0, [r2+1]
    add r2, r3
.loop4:
    movh       m1, [r2]
    movh       m3, [r2+r3]
    punpcklbw  m1, [r2+1]
    punpcklbw  m3, [r2+r3+1]
    lea        r2, [r2+2*r3]
    mova       m2, m1
    mova       m4, m3
    pmaddubsw  m0, m7
    pmaddubsw  m1, m6
    pmaddubsw  m2, m7
    pmaddubsw  m3, m6
    paddw      m0, m5
    paddw      m2, m5
    paddw      m1, m0
    paddw      m3, m2
    mova       m0, m4
    psrlw      m1, 6
    psrlw      m3, 6
    packuswb   m1, m1
    packuswb   m3, m3
    movh     [r0], m1
    movh  [r0+r1], m3
    sub       r4d, 2
    lea        r0, [r0+2*r1]
    jg .loop4
    REP_RET

INIT_XMM
.width8:
    mova       m5, [pw_32 GLOBAL]
    movd       m6, r5d
    movd       m7, r4d
    movifnidn r0,  r0mp
    movifnidn r1d, r1m
    movifnidn r4d, r7m
    SPLATW     m6, m6
    SPLATW     m7, m7
    movh       m0, [r2]
    movh       m1, [r2+1]
    punpcklbw  m0, m1
    add r2, r3
.loop8:
    movh       m1, [r2]
    movh       m2, [r2+1]
    movh       m3, [r2+r3]
    movh       m4, [r2+r3+1]
    punpcklbw  m1, m2
    punpcklbw  m3, m4
    lea        r2, [r2+2*r3]
    mova       m2, m1
    mova       m4, m3
    pmaddubsw  m0, m7
    pmaddubsw  m1, m6
    pmaddubsw  m2, m7
    pmaddubsw  m3, m6
    paddw      m0, m5
    paddw      m2, m5
    paddw      m1, m0
    paddw      m3, m2
    mova       m0, m4
    psrlw      m1, 6
    psrlw      m3, 6
    packuswb   m1, m3
    movh     [r0], m1
    movhps [r0+r1], m1
    sub       r4d, 2
    lea        r0, [r0+2*r1]
    jg .loop8
    REP_RET

; mc_chroma 1d ssse3 is negligibly faster, and definitely not worth the extra code size


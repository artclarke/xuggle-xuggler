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
pw_32: times 4 dw 32
pw_64: times 4 dw 64

SECTION .text

;=============================================================================
; pixel avg
;=============================================================================

;-----------------------------------------------------------------------------
; void x264_pixel_avg_4x4_mmxext( uint8_t *dst, int dst_stride,
;                                 uint8_t *src, int src_stride );
;-----------------------------------------------------------------------------
%macro AVGH 2
%assign function_align 8 ; the whole function fits in 8 bytes, so a larger align just wastes space
cglobal x264_pixel_avg_%1x%2_mmxext
    mov eax, %2
    jmp x264_pixel_avg_w%1_mmxext
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

AVGH 4, 8
AVGH 4, 4
AVGH 4, 2

AVG_START x264_pixel_avg_w8_mmxext
    movq   mm0, [t2]
    movq   mm1, [t2+t3]
    pavgb  mm0, [t0]
    pavgb  mm1, [t0+t1]
    movq   [t0], mm0
    movq   [t0+t1], mm1
AVG_END

AVGH 8, 16
AVGH 8, 8
AVGH 8, 4

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

AVGH 16, 16
AVGH 16, 8

AVG_START x264_pixel_avg_w16_sse2
    movdqu xmm0, [t2]
    movdqu xmm1, [t2+t3]
    pavgb  xmm0, [t0]
    pavgb  xmm1, [t0+t1]
    movdqa [t0], xmm0
    movdqa [t0+t1], xmm1
AVG_END



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
    cmp     r4m, dword 4
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



;=============================================================================
; weighted prediction
;=============================================================================
; implicit bipred only:
; assumes log2_denom = 5, offset = 0, weight1 + weight2 = 64

%macro BIWEIGHT_4P_MMX 2
    movd      mm0, %1
    movd      mm1, %2
    punpcklbw mm0, mm7
    punpcklbw mm1, mm7
    pmullw    mm0, mm4
    pmullw    mm1, mm5
    paddw     mm0, mm1
    paddw     mm0, mm6
    psraw     mm0, 6
    pmaxsw    mm0, mm7
    packuswb  mm0, mm0
    movd      %1,  mm0
%endmacro

%macro BIWEIGHT_START_MMX 1
%ifidn r4m, r4d
    movd    mm4, r4m
    pshufw  mm4, mm4, 0   ; weight_dst
%else
    pshufw  mm4, r4m, 0
%endif
    picgetgot r4
    movq    mm5, [pw_64 GLOBAL]
    psubw   mm5, mm4      ; weight_src
    movq    mm6, [pw_32 GLOBAL] ; rounding
    pxor    mm7, mm7
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
cglobal x264_pixel_avg_weight_w16_mmxext, 4,5
    BIWEIGHT_START_MMX 1
    BIWEIGHT_4P_MMX  [r0   ], [r2   ]
    BIWEIGHT_4P_MMX  [r0+ 4], [r2+ 4]
    BIWEIGHT_4P_MMX  [r0+ 8], [r2+ 8]
    BIWEIGHT_4P_MMX  [r0+12], [r2+12]
    add  r0, r1
    add  r2, r3
    dec  t0
    jg   .height_loop
    REP_RET

;-----------------------------------------------------------------------------
; int x264_pixel_avg_weight_w8_mmxext( uint8_t *, int, uint8_t *, int, int, int )
;-----------------------------------------------------------------------------
cglobal x264_pixel_avg_weight_w8_mmxext, 4,5
    BIWEIGHT_START_MMX 1
    BIWEIGHT_4P_MMX  [r0  ], [r2  ]
    BIWEIGHT_4P_MMX  [r0+4], [r2+4]
    add  r0, r1
    add  r2, r3
    dec  t0
    jg   .height_loop
    REP_RET

;-----------------------------------------------------------------------------
; int x264_pixel_avg_weight_4x4_mmxext( uint8_t *, int, uint8_t *, int, int )
;-----------------------------------------------------------------------------
cglobal x264_pixel_avg_weight_4x4_mmxext, 4,4,1
    BIWEIGHT_START_MMX 0
    BIWEIGHT_4P_MMX  [r0     ], [r2     ]
    BIWEIGHT_4P_MMX  [r0+r1  ], [r2+r3  ]
    BIWEIGHT_4P_MMX  [r0+r1*2], [r2+r3*2]
    add  r0, r1
    add  r2, r3
    BIWEIGHT_4P_MMX  [r0+r1*2], [r2+r3*2]
    RET



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
.height_loop

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

.finish
    REP_RET

ALIGN 4
.mc1d
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
.height_loop1_w4
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
    REP_RET

ALIGN 4
.height_loop1_w8
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


;*****************************************************************************
;* mc.asm: h264 encoder library
;*****************************************************************************
;* Copyright (C) 2003 x264 project
;* $Id: mc.asm,v 1.3 2004/06/18 01:59:58 chenm001 Exp $
;*
;* Authors: Min Chen <chenm001.163.com> (converted to nasm)
;*          Laurent Aimar <fenrir@via.ecp.fr> (init algorithm)
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

;*****************************************************************************
;*                                                                           *
;*  Revision history:                                                        *
;*                                                                           *
;*  2004.05.17 portab mc_copy_w4/8/16 (CM)                                   *
;*                                                                           *
;*****************************************************************************

BITS 64

;=============================================================================
; Macros and other preprocessor constants
;=============================================================================

%include "amd64inc.asm"

;=============================================================================
; Constants
;=============================================================================

SECTION_RODATA

pw_4:  times 4 dw  4
pw_8:  times 4 dw  8
pw_32: times 4 dw 32
pw_64: times 4 dw 64

;=============================================================================
; Code
;=============================================================================

SECTION .text

;=============================================================================
; pixel avg
;=============================================================================

;-----------------------------------------------------------------------------
; void x264_pixel_avg_w4_mmxext( uint8_t *dst, int dst_stride,
;                                uint8_t *src, int src_stride,
;                                int height );
;-----------------------------------------------------------------------------
cglobal x264_pixel_avg_w4_mmxext
.height_loop:
    movd   mm0, [parm3q]
    movd   mm1, [parm3q+parm4q]
    pavgb  mm0, [parm1q]
    pavgb  mm1, [parm1q+parm2q]
    movd   [parm1q], mm0
    movd   [parm1q+parm2q], mm1
    sub    temp1d, 2
    lea    parm3q, [parm3q+parm4q*2]
    lea    parm1q, [parm1q+parm2q*2]
    jg     .height_loop
    rep ret

cglobal x264_pixel_avg_w8_mmxext
.height_loop:
    movq   mm0, [parm3q]
    movq   mm1, [parm3q+parm4q]
    pavgb  mm0, [parm1q]
    pavgb  mm1, [parm1q+parm2q]
    movq   [parm1q], mm0
    movq   [parm1q+parm2q], mm1
    sub    temp1d, 2
    lea    parm3q, [parm3q+parm4q*2]
    lea    parm1q, [parm1q+parm2q*2]
    jg     .height_loop
    rep ret

cglobal x264_pixel_avg_w16_mmxext
.height_loop:
    movq   mm0, [parm3q  ]
    movq   mm1, [parm3q+8]
    movq   mm2, [parm3q+parm4q  ]
    movq   mm3, [parm3q+parm4q+8]
    pavgb  mm0, [parm1q  ]
    pavgb  mm1, [parm1q+8]
    pavgb  mm2, [parm1q+parm2q  ]
    pavgb  mm3, [parm1q+parm2q+8]
    movq   [parm1q  ], mm0
    movq   [parm1q+8], mm1
    movq   [parm1q+parm2q  ], mm2
    movq   [parm1q+parm2q+8], mm3
    sub    temp1d, 2
    lea    parm3q, [parm3q+parm4q*2]
    lea    parm1q, [parm1q+parm2q*2]
    jg     .height_loop
    rep ret

cglobal x264_pixel_avg_w16_sse2
.height_loop:
    movdqu xmm0, [parm3q]
    movdqu xmm1, [parm3q+parm4q]
    pavgb  xmm0, [parm1q]
    pavgb  xmm1, [parm1q+parm2q]
    movdqa [parm1q], xmm0
    movdqa [parm1q+parm2q], xmm1
    sub    temp1d, 2
    lea    parm3q, [parm3q+parm4q*2]
    lea    parm1q, [parm1q+parm2q*2]
    jg     .height_loop
    rep ret

%macro AVGH 2
cglobal x264_pixel_avg_%1x%2_mmxext
    mov temp1d, %2
    jmp x264_pixel_avg_w%1_mmxext
%endmacro

AVGH 16, 16
AVGH 16, 8
AVGH 8, 16
AVGH 8, 8
AVGH 8, 4
AVGH 4, 8
AVGH 4, 4
AVGH 4, 2

;-----------------------------------------------------------------------------
; void x264_pixel_avg2_w4_mmxext( uint8_t *dst, int dst_stride,
;                                 uint8_t *src1, int src_stride,
;                                 uint8_t *src2, int height );
;-----------------------------------------------------------------------------
%macro AVG2_START 0
%ifdef WIN64
    mov    temp1d, parm6d
    mov    temp2q, parm5q
%endif
    sub    parm5q, parm3q
%endmacro

cglobal x264_pixel_avg2_w4_mmxext
    AVG2_START
    lea    r10, [temp2q+parm4q]
.height_loop:
    movd   mm0, [parm3q]
    movd   mm1, [parm3q+parm4q]
    pavgb  mm0, [parm3q+temp2q]
    pavgb  mm1, [parm3q+r10]
    movd   [parm1q], mm0
    movd   [parm1q+parm2q], mm1
    sub    temp1d, 2
    lea    parm3q, [parm3q+parm4q*2]
    lea    parm1q, [parm1q+parm2q*2]
    jg     .height_loop
    rep ret

cglobal x264_pixel_avg2_w8_mmxext
    AVG2_START
    lea    r10, [temp2q+parm4q]
.height_loop:
    movq   mm0, [parm3q]
    movq   mm1, [parm3q+parm4q]
    pavgb  mm0, [parm3q+temp2q]
    pavgb  mm1, [parm3q+r10]
    movq   [parm1q], mm0
    movq   [parm1q+parm2q], mm1
    sub    temp1d, 2
    lea    parm3q, [parm3q+parm4q*2]
    lea    parm1q, [parm1q+parm2q*2]
    jg     .height_loop
    rep ret

cglobal x264_pixel_avg2_w16_mmxext
    AVG2_START
.height_loop:
    movq   mm0, [parm3q]
    movq   mm1, [parm3q+8]
    pavgb  mm0, [parm3q+temp2q]
    pavgb  mm1, [parm3q+temp2q+8]
    movq   [parm1q], mm0
    movq   [parm1q+8], mm1
    add    parm3q, parm4q
    add    parm1q, parm2q
    dec    temp1d
    jg     .height_loop
    rep ret

cglobal x264_pixel_avg2_w20_mmxext
    AVG2_START
.height_loop:
    movq   mm0, [parm3q]
    movq   mm1, [parm3q+8]
    movd   mm2, [parm3q+16]
    pavgb  mm0, [parm3q+temp2q]
    pavgb  mm1, [parm3q+temp2q+8]
    pavgb  mm2, [parm3q+temp2q+16]
    movq   [parm1q], mm0
    movq   [parm1q+8], mm1
    movd   [parm1q+16], mm2
    add    parm3q, parm4q
    add    parm1q, parm2q
    dec    temp1d
    jg     .height_loop
    rep ret



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

%macro BIWEIGHT_START_MMX 0
;   mov     rdi, rdi      ; dst
;   movsxd  rsi, esi      ; i_dst
;   mov     rdx, rdx      ; src
;   movsxd  rcx, ecx      ; i_src
;   movsxd  r8,  r8d      ; i_weight_dst
;   movsxd  r9,  r9d      ; i_height
    mov     r11d, parm6d  ; i_height

    movd    mm4, parm5d
    pshufw  mm4, mm4, 0   ; weight_dst
    movq    mm5, [pw_64 GLOBAL]
    psubw   mm5, mm4      ; weight_src
    movq    mm6, [pw_32 GLOBAL] ; rounding
    pxor    mm7, mm7

    ALIGN 4
    .height_loop
%endmacro

;-----------------------------------------------------------------------------
;   int x264_pixel_avg_weight_w16_mmxext( uint8_t *dst, int, uint8_t *src, int, int i_weight, int )
;-----------------------------------------------------------------------------
cglobal x264_pixel_avg_weight_w16_mmxext
    BIWEIGHT_START_MMX

    BIWEIGHT_4P_MMX  [parm1q   ], [parm3q   ]
    BIWEIGHT_4P_MMX  [parm1q+ 4], [parm3q+ 4]
    BIWEIGHT_4P_MMX  [parm1q+ 8], [parm3q+ 8]
    BIWEIGHT_4P_MMX  [parm1q+12], [parm3q+12]

    add  parm1q, parm2q
    add  parm3q, parm4q
    dec  r11d
    jg   .height_loop
    rep ret

;-----------------------------------------------------------------------------
;   int x264_pixel_avg_weight_w8_mmxext( uint8_t *, int, uint8_t *, int, int, int )
;-----------------------------------------------------------------------------
cglobal x264_pixel_avg_weight_w8_mmxext
    BIWEIGHT_START_MMX

    BIWEIGHT_4P_MMX  [parm1q  ], [parm3q  ]
    BIWEIGHT_4P_MMX  [parm1q+4], [parm3q+4]

    add  parm1q, parm2q
    add  parm3q, parm4q
    dec  r11d
    jg   .height_loop
    rep ret

;-----------------------------------------------------------------------------
;   int x264_pixel_avg_weight_4x4_mmxext( uint8_t *, int, uint8_t *, int, int )
;-----------------------------------------------------------------------------
cglobal x264_pixel_avg_weight_4x4_mmxext
    BIWEIGHT_START_MMX
    BIWEIGHT_4P_MMX  [parm1q         ], [parm3q         ]
    BIWEIGHT_4P_MMX  [parm1q+parm2q  ], [parm3q+parm4q  ]
    BIWEIGHT_4P_MMX  [parm1q+parm2q*2], [parm3q+parm4q*2]
    add  parm1q, parm2q
    add  parm3q, parm4q
    BIWEIGHT_4P_MMX  [parm1q+parm2q*2], [parm3q+parm4q*2]
    ret



;=============================================================================
; pixel copy
;=============================================================================

;-----------------------------------------------------------------------------
;  void x264_mc_copy_w4_mmx( uint8_t *dst, int i_dst_stride,
;                            uint8_t *src, int i_src_stride, int i_height )
;-----------------------------------------------------------------------------
cglobal x264_mc_copy_w4_mmx
    mov     eax, parm5d         ; i_height
    
ALIGN 4
.height_loop
    mov     r10d, [parm3q]
    mov     r11d, [parm3q+parm4q]
    mov     [parm1q], r10d
    mov     [parm1q+parm2q], r11d
    lea     parm3q, [parm3q+parm4q*2]
    lea     parm1q, [parm1q+parm2q*2]
    dec     eax
    dec     eax
    jg      .height_loop
    rep ret

;-----------------------------------------------------------------------------
;   void x264_mc_copy_w8_mmx( uint8_t *dst, int i_dst_stride,
;                             uint8_t *src, int i_src_stride, int i_height )
;-----------------------------------------------------------------------------
cglobal x264_mc_copy_w8_mmx
    mov     eax, parm5d         ; i_height

    lea     r10, [parm4q+parm4q*2] ; 3 * i_src_stride
    lea     r11, [parm2q+parm2q*2] ; 3 * i_dst_stride

ALIGN 4
.height_loop
    movq    mm0, [parm3q]
    movq    mm1, [parm3q+parm4q]
    movq    mm2, [parm3q+parm4q*2]
    movq    mm3, [parm3q+r10]
    movq    [parm1q], mm0
    movq    [parm1q+parm2q], mm1
    movq    [parm1q+parm2q*2], mm2
    movq    [parm1q+r11], mm3
    lea     parm3q, [parm3q+parm4q*4]
    lea     parm1q, [parm1q+parm2q*4]
    
    sub     eax, byte 4
    jg      .height_loop
    rep ret

;-----------------------------------------------------------------------------
;   void x264_mc_copy_w16_mmx( uint8_t *dst, int i_dst_stride,
;                              uint8_t *src, int i_src_stride, int i_height )
;-----------------------------------------------------------------------------
cglobal x264_mc_copy_w16_mmx
    mov     eax, parm5d         ; i_height
    
    lea     r10, [parm4q+parm4q*2] ; 3 * i_src_stride
    lea     r11, [parm2q+parm2q*2] ; 3 * i_dst_stride

ALIGN 4
.height_loop
    movq    mm0, [parm3q]
    movq    mm1, [parm3q+8]
    movq    mm2, [parm3q+parm4q]
    movq    mm3, [parm3q+parm4q+8]
    movq    mm4, [parm3q+parm4q*2]
    movq    mm5, [parm3q+parm4q*2+8]
    movq    mm6, [parm3q+r10]
    movq    mm7, [parm3q+r10+8]
    movq    [parm1q], mm0
    movq    [parm1q+8], mm1
    movq    [parm1q+parm2q], mm2
    movq    [parm1q+parm2q+8], mm3
    movq    [parm1q+parm2q*2], mm4
    movq    [parm1q+parm2q*2+8], mm5
    movq    [parm1q+r11], mm6
    movq    [parm1q+r11+8], mm7
    lea     parm3q, [parm3q+parm4q*4]
    lea     parm1q, [parm1q+parm2q*4]
    sub     eax, byte 4
    jg      .height_loop
    rep ret


;-----------------------------------------------------------------------------
;   void x264_mc_copy_w16_sse2( uint8_t *dst, int i_dst_stride, uint8_t *src, int i_src_stride, int i_height )
;-----------------------------------------------------------------------------
cglobal x264_mc_copy_w16_sse2
    mov     eax, parm5d         ; i_height

ALIGN 4
.height_loop
    movdqu  xmm0, [parm3q]
    movdqu  xmm1, [parm3q+parm4q]
    movdqu  [parm1q], xmm0
    movdqu  [parm1q+parm2q], xmm1
    sub     eax, byte 2
    lea     parm3q, [parm3q+parm4q*2]
    lea     parm1q, [parm1q+parm2q*2]
    jg      .height_loop
    rep ret



;=============================================================================
; chroma MC
;=============================================================================

;-----------------------------------------------------------------------------
;   void x264_mc_chroma_mmxext( uint8_t *dst, int i_dst_stride,
;                               uint8_t *src, int i_src_stride,
;                               int dx, int dy,
;                               int i_width, int i_height )
;-----------------------------------------------------------------------------
cglobal x264_mc_chroma_mmxext
    mov     r10d, parm6d
    mov     r11d, parm5d
    sar     r10d, 3
    sar     r11d, 3
    imul    r10d, parm4d
    pxor    mm3, mm3
    add     r10d, r11d
    movsxd   r10, r10d
    mov     r11d, parm8d
    add   parm3q, r10           ; src += (dx>>3) + (dy>>3) * src_stride
    and   parm5d, 7             ; dx &= 7
    je      .mc1d
    and   parm6d, 7             ; dy &= 7
    je      .mc1d

    movd    mm0, parm5d
    movd    mm1, parm6d

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

    mov     rax, parm3q
    mov     r10, parm1q

ALIGN 4
.height_loop

    movd    mm1, [rax+parm4q]
    movd    mm0, [rax]
    punpcklbw mm1, mm3          ; 00 px1 | 00 px2 | 00 px3 | 00 px4
    punpcklbw mm0, mm3
    pmullw  mm1, mm6            ; 2nd line * cC
    pmullw  mm0, mm4            ; 1st line * cA

    paddw   mm0, mm1            ; mm0 <- result

    movd    mm2, [rax+1]
    movd    mm1, [rax+parm4q+1]
    punpcklbw mm2, mm3
    punpcklbw mm1, mm3

    paddw   mm0, [pw_32 GLOBAL]

    pmullw  mm2, mm5            ; line * cB
    pmullw  mm1, mm7            ; line * cD
    paddw   mm0, mm2
    paddw   mm0, mm1
    psrlw   mm0, 6

    packuswb mm0, mm3           ; 00 00 00 00 px1 px2 px3 px4
    movd    [r10], mm0

    add     rax, parm4q
    add     r10, parm2q         ; i_dst_stride
    dec     r11d
    jnz     .height_loop

    sub     parm7d, 8
    jnz     .finish             ; width != 8 so assume 4

    mov     r10, parm1q         ; dst
    mov     rax, parm3q         ; src
    mov     r11d, parm8d        ; i_height
    add     r10, 4
    add     rax, 4
    jmp     .height_loop

.finish
    rep ret

ALIGN 4
.mc1d
%define pel_offset temp1q
    mov       eax, parm5d
    or        eax, parm6d
    and       eax, 7
    cmp    parm5d, 0
    mov    pel_offset, 1
    cmove  pel_offset, parm4q   ; pel_offset = dx ? 1 : src_stride
    movd      mm6, eax
    movq      mm5, [pw_8 GLOBAL]
    pshufw    mm6, mm6, 0
    movq      mm7, [pw_4 GLOBAL]
    psubw     mm5, mm6

    cmp    parm7d, 8
    je .height_loop1_w8

ALIGN 4
.height_loop1_w4
    movd      mm0, [parm3q+pel_offset]
    movd      mm1, [parm3q]
    punpcklbw mm0, mm3
    punpcklbw mm1, mm3
    pmullw    mm0, mm6
    pmullw    mm1, mm5
    paddw     mm0, mm7
    paddw     mm0, mm1
    psrlw     mm0, 3
    packuswb  mm0, mm3
    movd [parm1q], mm0
    add    parm3q, parm4q
    add    parm1q, parm2q
    dec      r11d
    jnz .height_loop1_w4
    rep ret

ALIGN 4
.height_loop1_w8
    movq      mm0, [parm3q+pel_offset]
    movq      mm1, [parm3q]
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
    movq [parm1q], mm0
    add    parm3q, parm4q
    add    parm1q, parm2q
    dec      r11d
    jnz .height_loop1_w8
    rep ret



;-----------------------------------------------------------------------------
; void x264_prefetch_fenc_mmxext( uint8_t *pix_y, int stride_y, 
;                                 uint8_t *pix_uv, int stride_uv, int mb_x )
;-----------------------------------------------------------------------------
cglobal x264_prefetch_fenc_mmxext
    mov     eax, parm5d
    and     eax, 3
    imul    eax, parm2d
    lea  parm1q, [parm1q+rax*4+64]
    prefetcht0   [parm1q]
    prefetcht0   [parm1q+parm2q]
    lea  parm1q, [parm1q+parm2q*2]
    prefetcht0   [parm1q]
    prefetcht0   [parm1q+parm2q]

    mov     eax, parm5d
    and     eax, 6
    imul    eax, parm4d
    lea  parm3q, [parm3q+rax+64]
    prefetcht0   [parm3q]
    prefetcht0   [parm3q+parm4q]
    ret

;-----------------------------------------------------------------------------
; void x264_prefetch_ref_mmxext( uint8_t *pix, int stride, int parity )
;-----------------------------------------------------------------------------
cglobal x264_prefetch_ref_mmxext
    dec  parm3d
    and  parm3d, parm2d
    lea  parm1q, [parm1q+parm3q*8+64]
    lea     rax, [parm2q*3]
    prefetcht0   [parm1q]
    prefetcht0   [parm1q+parm2q]
    prefetcht0   [parm1q+parm2q*2]
    prefetcht0   [parm1q+rax]
    lea  parm1q, [parm1q+parm2q*4]
    prefetcht0   [parm1q]
    prefetcht0   [parm1q+parm2q]
    prefetcht0   [parm1q+parm2q*2]
    prefetcht0   [parm1q+rax]
    ret

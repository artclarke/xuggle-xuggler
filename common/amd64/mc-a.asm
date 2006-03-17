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

SECTION .rodata

ALIGN 16
pw_4:  times 4 dw  4
pw_8:  times 4 dw  8
pw_32: times 4 dw 32
pw_64: times 4 dw 64

;=============================================================================
; Code
;=============================================================================

SECTION .text

cglobal x264_pixel_avg_w4_mmxext
cglobal x264_pixel_avg_w8_mmxext
cglobal x264_pixel_avg_w16_mmxext
cglobal x264_pixel_avg_w16_sse2

cglobal x264_pixel_avg_weight_4x4_mmxext
cglobal x264_pixel_avg_weight_w8_mmxext
cglobal x264_pixel_avg_weight_w16_mmxext

cglobal x264_mc_copy_w4_mmx
cglobal x264_mc_copy_w8_mmx
cglobal x264_mc_copy_w16_mmx
cglobal x264_mc_copy_w16_sse2

cglobal x264_mc_chroma_mmxext

;=============================================================================
; pixel avg
;=============================================================================

ALIGN 16
;-----------------------------------------------------------------------------
; void x264_pixel_avg_w4_mmxext( uint8_t *dst,  int i_dst_stride,
;                                uint8_t *src1, int i_src1_stride,
;                                uint8_t *src2, int i_src2_stride,
;                                int i_height );
;-----------------------------------------------------------------------------
x264_pixel_avg_w4_mmxext:
    mov         r10, parm5q         ; src2
    movsxd      r11, parm6d         ; i_src2_stride
    movsxd      rax, parm7d         ; i_height

ALIGN 4
.height_loop    
    movd        mm0, [parm3q]
    pavgb       mm0, [r10]
    movd        mm1, [parm3q+parm4q]
    pavgb       mm1, [r10+r11]
    movd        [parm1q], mm0
    movd        [parm1q+parm2q], mm1
    dec         rax
    dec         rax
    lea         parm3q, [parm3q+parm4q*2]
    lea         r10, [r10+r11*2]
    lea         parm1q, [parm1q+parm2q*2]
    jne         .height_loop

    ret

                          

ALIGN 16
;-----------------------------------------------------------------------------
; void x264_pixel_avg_w8_mmxext( uint8_t *dst,  int i_dst_stride,
;                                uint8_t *src1, int i_src1_stride,
;                                uint8_t *src2, int i_src2_stride,
;                                int i_height );
;-----------------------------------------------------------------------------
x264_pixel_avg_w8_mmxext:

    mov         r10, parm5q         ; src2
    movsxd      r11, parm6d         ; i_src2_stride
    movsxd      rax, parm7d         ; i_height

ALIGN 4
.height_loop    
    movq        mm0, [parm3q]
    pavgb       mm0, [r10]
    movq        [parm1q], mm0
    dec         rax
    lea         parm3q, [parm3q+parm4q]
    lea         r10, [r10+r11]
    lea         parm1q, [parm1q+parm2q]
    jne         .height_loop

    ret

ALIGN 16
;-----------------------------------------------------------------------------
; void x264_pixel_avg_w16_mmxext( uint8_t *dst,  int i_dst_stride,
;                                 uint8_t *src1, int i_src1_stride,
;                                 uint8_t *src2, int i_src2_stride,
;                                 int i_height );
;-----------------------------------------------------------------------------
x264_pixel_avg_w16_mmxext:
    mov         r10, parm5q         ; src2
    movsxd      r11, parm6d         ; i_src2_stride
    movsxd      rax, parm7d         ; i_height

ALIGN 4
.height_loop    
    movq        mm0, [parm3q  ]
    movq        mm1, [parm3q+8]
    pavgb       mm0, [r10  ]
    pavgb       mm1, [r10+8]
    movq        [parm1q  ], mm0
    movq        [parm1q+8], mm1
    dec         rax
    lea         parm3q, [parm3q+parm4q]
    lea         r10, [r10+r11]
    lea         parm1q, [parm1q+parm2q]
    jne         .height_loop

    ret

ALIGN 16
;-----------------------------------------------------------------------------
; void x264_pixel_avg_w16_sse2( uint8_t *dst,  int i_dst_stride,
;                               uint8_t *src1, int i_src1_stride,
;                               uint8_t *src2, int i_src2_stride,
;                               int i_height );
;-----------------------------------------------------------------------------
x264_pixel_avg_w16_sse2:
    mov         r10, parm5q         ; src2
    movsxd      r11, parm6d         ; i_src2_stride
    movsxd      rax, parm7d         ; i_height

ALIGN 4
.height_loop    
    movdqu      xmm0, [parm3q]
    pavgb       xmm0, [r10]
    movdqu      [parm1q], xmm0

    dec         rax
    lea         parm3q, [parm3q+parm4q]
    lea         r10, [r10+r11]
    lea         parm1q, [parm1q+parm2q]
    jne         .height_loop

    ret



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

ALIGN 16
;-----------------------------------------------------------------------------
;   int x264_pixel_avg_weight_w16_mmxext( uint8_t *dst, int, uint8_t *src, int, int i_weight, int )
;-----------------------------------------------------------------------------
x264_pixel_avg_weight_w16_mmxext:
    BIWEIGHT_START_MMX

    BIWEIGHT_4P_MMX  [parm1q   ], [parm3q   ]
    BIWEIGHT_4P_MMX  [parm1q+ 4], [parm3q+ 4]
    BIWEIGHT_4P_MMX  [parm1q+ 8], [parm3q+ 8]
    BIWEIGHT_4P_MMX  [parm1q+12], [parm3q+12]

    add  parm1q, parm2q
    add  parm3q, parm4q
    dec  r11d
    jnz  .height_loop
    ret

ALIGN 16
;-----------------------------------------------------------------------------
;   int x264_pixel_avg_weight_w8_mmxext( uint8_t *, int, uint8_t *, int, int, int )
;-----------------------------------------------------------------------------
x264_pixel_avg_weight_w8_mmxext:
    BIWEIGHT_START_MMX

    BIWEIGHT_4P_MMX  [parm1q         ], [parm3q         ]
    BIWEIGHT_4P_MMX  [parm1q+4       ], [parm3q+4       ]
    BIWEIGHT_4P_MMX  [parm1q+parm2q  ], [parm3q+parm4q  ]
    BIWEIGHT_4P_MMX  [parm1q+parm2q+4], [parm3q+parm4q+4]

    lea  parm1q, [parm1q+parm2q*2]
    lea  parm3q, [parm3q+parm4q*2]
    sub  r11d, byte 2
    jnz  .height_loop
    ret

ALIGN 16
;-----------------------------------------------------------------------------
;   int x264_pixel_avg_weight_4x4_mmxext( uint8_t *, int, uint8_t *, int, int )
;-----------------------------------------------------------------------------
x264_pixel_avg_weight_4x4_mmxext:
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

ALIGN 16
;-----------------------------------------------------------------------------
;  void x264_mc_copy_w4_mmx( uint8_t *dst, int i_dst_stride,
;                            uint8_t *src, int i_src_stride, int i_height )
;-----------------------------------------------------------------------------
x264_mc_copy_w4_mmx:
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
    jne     .height_loop

    ret

ALIGN 16
;-----------------------------------------------------------------------------
;   void x264_mc_copy_w8_mmx( uint8_t *dst, int i_dst_stride,
;                             uint8_t *src, int i_src_stride, int i_height )
;-----------------------------------------------------------------------------
x264_mc_copy_w8_mmx:
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
    jnz     .height_loop

    ret

ALIGN 16
;-----------------------------------------------------------------------------
;   void x264_mc_copy_w16_mmx( uint8_t *dst, int i_dst_stride,
;                              uint8_t *src, int i_src_stride, int i_height )
;-----------------------------------------------------------------------------
x264_mc_copy_w16_mmx:
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
    jnz     .height_loop
    
    ret


ALIGN 16
;-----------------------------------------------------------------------------
;   void x264_mc_copy_w16_sse2( uint8_t *dst, int i_dst_stride, uint8_t *src, int i_src_stride, int i_height )
;-----------------------------------------------------------------------------
x264_mc_copy_w16_sse2:
    mov     eax, parm5d         ; i_height

ALIGN 4
.height_loop
    movdqu  xmm0, [parm3q]
    movdqu  xmm1, [parm3q+parm4q]
    movdqu  [parm1q], xmm0
    movdqu  [parm1q+parm2q], xmm1
    dec     eax
    dec     eax
    lea     parm3q, [parm3q+parm4q*2]
    lea     parm1q, [parm1q+parm2q*2]
    jnz     .height_loop
    
    ret



;=============================================================================
; chroma MC
;=============================================================================

ALIGN 16
;-----------------------------------------------------------------------------
;   void x264_mc_chroma_mmxext( uint8_t *src, int i_src_stride,
;                               uint8_t *dst, int i_dst_stride,
;                               int dx, int dy,
;                               int i_width, int i_height )
;-----------------------------------------------------------------------------

x264_mc_chroma_mmxext:
    movd    mm0, parm5d
    movd    mm1, parm6d

    pxor    mm3, mm3

    pshufw  mm5, mm0, 0    ; mm5 - dx
    pshufw  mm6, mm1, 0    ; mm6 - dy

    movq    mm4, [pw_8 GLOBAL]
    movq    mm0, mm4

    psubw   mm4, mm5            ; mm4 - 8-dx
    psubw   mm0, mm6            ; mm0 - 8-dy

    movq    mm7, mm5
    pmullw  mm5, mm0            ; mm5 = dx*(8-dy) =     cB
    pmullw  mm7, mm6            ; mm7 = dx*dy =         cD
    pmullw  mm6, mm4            ; mm6 = (8-dx)*dy =     cC
    pmullw  mm4, mm0            ; mm4 = (8-dx)*(8-dy) = cA

    mov     rax, parm1q
    mov     r10, parm3q
    mov     r11d, parm8d

ALIGN 4
.height_loop

    movd    mm1, [rax+parm2q]
    movd    mm0, [rax]
    punpcklbw mm1, mm3          ; 00 px1 | 00 px2 | 00 px3 | 00 px4
    punpcklbw mm0, mm3
    pmullw  mm1, mm6            ; 2nd line * cC
    pmullw  mm0, mm4            ; 1st line * cA

    paddw   mm0, mm1            ; mm0 <- result

    movd    mm2, [rax+1]
    movd    mm1, [rax+parm2q+1]
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

    add     rax, parm2q
    add     r10, parm4q         ; i_dst_stride

    dec     r11d
    jnz     .height_loop

    sub     parm7d, 8
    jnz     .finish             ; width != 8 so assume 4

    mov     r10, parm3q         ; dst
    mov     rax, parm1q         ; src
    mov     r11d, parm8d        ; i_height
    add     r10, 4
    add     rax, 4
    jmp    .height_loop

.finish
    ret

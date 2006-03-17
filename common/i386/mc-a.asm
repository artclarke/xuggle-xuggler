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

BITS 32

;=============================================================================
; Macros and other preprocessor constants
;=============================================================================

%include "i386inc.asm"

;=============================================================================
; Constants
;=============================================================================

SECTION_RODATA

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
    push        ebp
    push        ebx
    push        esi
    push        edi

    mov         edi, [esp+20]       ; dst
    mov         ebx, [esp+28]       ; src1
    mov         ecx, [esp+36]       ; src2
    mov         esi, [esp+24]       ; i_dst_stride
    mov         eax, [esp+32]       ; i_src1_stride
    mov         edx, [esp+40]       ; i_src2_stride
    mov         ebp, [esp+44]       ; i_height
ALIGN 4
.height_loop    
    movd        mm0, [ebx]
    pavgb       mm0, [ecx]
    movd        mm1, [ebx+eax]
    pavgb       mm1, [ecx+edx]
    movd        [edi], mm0
    movd        [edi+esi], mm1
    dec         ebp
    dec         ebp
    lea         ebx, [ebx+eax*2]
    lea         ecx, [ecx+edx*2]
    lea         edi, [edi+esi*2]
    jne         .height_loop

    pop         edi
    pop         esi
    pop         ebx
    pop         ebp
    ret

                          

ALIGN 16
;-----------------------------------------------------------------------------
; void x264_pixel_avg_w8_mmxext( uint8_t *dst,  int i_dst_stride,
;                                uint8_t *src1, int i_src1_stride,
;                                uint8_t *src2, int i_src2_stride,
;                                int i_height );
;-----------------------------------------------------------------------------
x264_pixel_avg_w8_mmxext:
    push        ebp
    push        ebx
    push        esi
    push        edi

    mov         edi, [esp+20]       ; dst
    mov         ebx, [esp+28]       ; src1
    mov         ecx, [esp+36]       ; src2
    mov         esi, [esp+24]       ; i_dst_stride
    mov         eax, [esp+32]       ; i_src1_stride
    mov         edx, [esp+40]       ; i_src2_stride
    mov         ebp, [esp+44]       ; i_height
ALIGN 4
.height_loop    
    movq        mm0, [ebx]
    pavgb       mm0, [ecx]
    movq        [edi], mm0
    dec         ebp
    lea         ebx, [ebx+eax]
    lea         ecx, [ecx+edx]
    lea         edi, [edi+esi]
    jne         .height_loop

    pop         edi
    pop         esi
    pop         ebx
    pop         ebp
    ret



ALIGN 16
;-----------------------------------------------------------------------------
; void x264_pixel_avg_w16_mmxext( uint8_t *dst,  int i_dst_stride,
;                                 uint8_t *src1, int i_src1_stride,
;                                 uint8_t *src2, int i_src2_stride,
;                                 int i_height );
;-----------------------------------------------------------------------------
x264_pixel_avg_w16_mmxext:
    push        ebp
    push        ebx
    push        esi
    push        edi

    mov         edi, [esp+20]       ; dst
    mov         ebx, [esp+28]       ; src1
    mov         ecx, [esp+36]       ; src2
    mov         esi, [esp+24]       ; i_dst_stride
    mov         eax, [esp+32]       ; i_src1_stride
    mov         edx, [esp+40]       ; i_src2_stride
    mov         ebp, [esp+44]       ; i_height
ALIGN 4
.height_loop    
    movq        mm0, [ebx  ]
    movq        mm1, [ebx+8]
    pavgb       mm0, [ecx  ]
    pavgb       mm1, [ecx+8]
    movq        [edi  ], mm0
    movq        [edi+8], mm1
    dec         ebp
    lea         ebx, [ebx+eax]
    lea         ecx, [ecx+edx]
    lea         edi, [edi+esi]
    jne         .height_loop

    pop         edi
    pop         esi
    pop         ebx
    pop         ebp
    ret

ALIGN 16
;-----------------------------------------------------------------------------
; void x264_pixel_avg_w16_sse2( uint8_t *dst,  int i_dst_stride,
;                               uint8_t *src1, int i_src1_stride,
;                               uint8_t *src2, int i_src2_stride,
;                               int i_height );
;-----------------------------------------------------------------------------
x264_pixel_avg_w16_sse2:
    push        ebp
    push        ebx
    push        esi
    push        edi

    mov         edi, [esp+20]       ; dst
    mov         ebx, [esp+28]       ; src1
    mov         ecx, [esp+36]       ; src2
    mov         esi, [esp+24]       ; i_dst_stride
    mov         eax, [esp+32]       ; i_src1_stride
    mov         edx, [esp+40]       ; i_src2_stride
    mov         ebp, [esp+44]       ; i_height
ALIGN 4
.height_loop    
    movdqu      xmm0, [ebx]
    pavgb       xmm0, [ecx]
    movdqu      [edi], xmm0

    dec         ebp
    lea         ebx, [ebx+eax]
    lea         ecx, [ecx+edx]
    lea         edi, [edi+esi]
    jne         .height_loop

    pop         edi
    pop         esi
    pop         ebx
    pop         ebp
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
    push    edi
    push    esi
    picpush ebx
    picgetgot ebx
    mov     edi, [picesp+12]    ; dst
    mov     esi, [picesp+16]    ; i_dst
    mov     edx, [picesp+20]    ; src
    mov     ecx, [picesp+24]    ; i_src

    pshufw  mm4, [picesp+28], 0  ; weight_dst
    movq    mm5, [pw_64 GOT_ebx]
    psubw   mm5, mm4             ; weight_src
    movq    mm6, [pw_32 GOT_ebx] ; rounding
    pxor    mm7, mm7
%endmacro
%macro BIWEIGHT_END_MMX 0
    picpop  ebx
    pop     esi
    pop     edi
    ret
%endmacro

ALIGN 16
;-----------------------------------------------------------------------------
;   int __cdecl x264_pixel_avg_weight_w16_mmxext( uint8_t *, int, uint8_t *, int, int, int )
;-----------------------------------------------------------------------------
x264_pixel_avg_weight_w16_mmxext:
    BIWEIGHT_START_MMX
    mov     eax, [picesp+32] ; i_height
    ALIGN 4
    .height_loop

    BIWEIGHT_4P_MMX  [edi   ], [edx   ]
    BIWEIGHT_4P_MMX  [edi+ 4], [edx+ 4]
    BIWEIGHT_4P_MMX  [edi+ 8], [edx+ 8]
    BIWEIGHT_4P_MMX  [edi+12], [edx+12]

    add  edi, esi
    add  edx, ecx
    dec  eax
    jnz  .height_loop
    BIWEIGHT_END_MMX

ALIGN 16
;-----------------------------------------------------------------------------
;   int __cdecl x264_pixel_avg_weight_w8_mmxext( uint8_t *, int, uint8_t *, int, int, int )
;-----------------------------------------------------------------------------
x264_pixel_avg_weight_w8_mmxext:
    BIWEIGHT_START_MMX
    mov     eax, [picesp+32]
    ALIGN 4
    .height_loop

    BIWEIGHT_4P_MMX  [edi      ], [edx      ]
    BIWEIGHT_4P_MMX  [edi+4    ], [edx+4    ]
    BIWEIGHT_4P_MMX  [edi+esi  ], [edx+ecx  ]
    BIWEIGHT_4P_MMX  [edi+esi+4], [edx+ecx+4]

    lea  edi, [edi+esi*2]
    lea  edx, [edx+ecx*2]
    sub  eax, byte 2
    jnz  .height_loop
    BIWEIGHT_END_MMX

ALIGN 16
;-----------------------------------------------------------------------------
;   int __cdecl x264_pixel_avg_weight_4x4_mmxext( uint8_t *, int, uint8_t *, int, int )
;-----------------------------------------------------------------------------
x264_pixel_avg_weight_4x4_mmxext:
    BIWEIGHT_START_MMX
    BIWEIGHT_4P_MMX  [edi      ], [edx      ]
    BIWEIGHT_4P_MMX  [edi+esi  ], [edx+ecx  ]
    BIWEIGHT_4P_MMX  [edi+esi*2], [edx+ecx*2]
    add  edi, esi
    add  edx, ecx
    BIWEIGHT_4P_MMX  [edi+esi*2], [edx+ecx*2]
    BIWEIGHT_END_MMX



;=============================================================================
; pixel copy
;=============================================================================

ALIGN 16
;-----------------------------------------------------------------------------
;  void x264_mc_copy_w4_mmx( uint8_t *src, int i_src_stride,
;                            uint8_t *dst, int i_dst_stride, int i_height )
;-----------------------------------------------------------------------------
x264_mc_copy_w4_mmx:
    push    ebx
    push    esi
    push    edi

    mov     esi, [esp+24]       ; src
    mov     edi, [esp+16]       ; dst
    mov     ebx, [esp+28]       ; i_src_stride
    mov     edx, [esp+20]       ; i_dst_stride
    mov     ecx, [esp+32]       ; i_height
ALIGN 4
.height_loop
    mov     eax, [esi]
    mov     [edi], eax
    mov     eax, [esi+ebx]
    mov     [edi+edx], eax
    lea     esi, [esi+ebx*2]
    lea     edi, [edi+edx*2]
    dec     ecx
    dec     ecx
    jne     .height_loop

    pop     edi
    pop     esi
    pop     ebx
    ret

ALIGN 16
;-----------------------------------------------------------------------------
;   void x264_mc_copy_w8_mmx( uint8_t *src, int i_src_stride,
;                             uint8_t *dst, int i_dst_stride, int i_height )
;-----------------------------------------------------------------------------
x264_mc_copy_w8_mmx:
    push    ebx
    push    esi
    push    edi

    mov     esi, [esp+24]       ; src
    mov     edi, [esp+16]       ; dst
    mov     ebx, [esp+28]       ; i_src_stride
    mov     edx, [esp+20]       ; i_dst_stride
    mov     ecx, [esp+32]       ; i_height
ALIGN 4
.height_loop
    movq    mm0, [esi]
    movq    [edi], mm0
    movq    mm1, [esi+ebx]
    movq    [edi+edx], mm1
    movq    mm2, [esi+ebx*2]
    movq    [edi+edx*2], mm2
    lea     esi, [esi+ebx*2]
    lea     edi, [edi+edx*2]
    movq    mm3, [esi+ebx]
    movq    [edi+edx], mm3
    lea     esi, [esi+ebx*2]
    lea     edi, [edi+edx*2]
    
    sub     ecx, byte 4
    jnz     .height_loop

    pop     edi
    pop     esi
    pop     ebx
    ret

ALIGN 16
;-----------------------------------------------------------------------------
;   void x264_mc_copy_w16_mmx( uint8_t *src, int i_src_stride,
;                              uint8_t *dst, int i_dst_stride, int i_height )
;-----------------------------------------------------------------------------
x264_mc_copy_w16_mmx:
    push    ebx
    push    esi
    push    edi

    mov     esi, [esp+24]       ; src
    mov     edi, [esp+16]       ; dst
    mov     ebx, [esp+28]       ; i_src_stride
    mov     edx, [esp+20]       ; i_dst_stride
    mov     ecx, [esp+32]       ; i_height

ALIGN 4
.height_loop
    movq    mm0, [esi]
    movq    mm1, [esi+8]
    movq    [edi], mm0
    movq    [edi+8], mm1
    movq    mm2, [esi+ebx]
    movq    mm3, [esi+ebx+8]
    movq    [edi+edx], mm2
    movq    [edi+edx+8], mm3
    movq    mm4, [esi+ebx*2]
    movq    mm5, [esi+ebx*2+8]
    movq    [edi+edx*2], mm4
    movq    [edi+edx*2+8], mm5
    lea     esi, [esi+ebx*2]
    lea     edi, [edi+edx*2]
    movq    mm6, [esi+ebx]
    movq    mm7, [esi+ebx+8]
    movq    [edi+edx], mm6
    movq    [edi+edx+8], mm7
    lea     esi, [esi+ebx*2]
    lea     edi, [edi+edx*2]
    sub     ecx, byte 4
    jnz     .height_loop
    
    pop     edi
    pop     esi
    pop     ebx
    ret


ALIGN 16
;-----------------------------------------------------------------------------
;   void x264_mc_copy_w16_sse2( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_height )
;-----------------------------------------------------------------------------
x264_mc_copy_w16_sse2:
    push    ebx
    push    esi
    push    edi

    mov     esi, [esp+24]       ; src
    mov     edi, [esp+16]       ; dst
    mov     ebx, [esp+28]       ; i_src_stride
    mov     edx, [esp+20]       ; i_dst_stride
    mov     ecx, [esp+32]       ; i_height

ALIGN 4
.height_loop
    movdqu  xmm0, [esi]
    movdqu  xmm1, [esi+ebx]
    movdqu  [edi], xmm0
    movdqu  [edi+edx], xmm1
    dec     ecx
    dec     ecx
    lea     esi, [esi+ebx*2]
    lea     edi, [edi+edx*2]
    jnz     .height_loop
    
    pop     edi
    pop     esi
    pop     ebx
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

    picpush ebx
    picgetgot ebx

    pxor    mm3, mm3

    pshufw  mm5, [picesp+20], 0    ; mm5 = dx
    pshufw  mm6, [picesp+24], 0    ; mm6 = dy

    movq    mm4, [pw_8 GOT_ebx]
    movq    mm0, mm4

    psubw   mm4, mm5            ; mm4 = 8-dx
    psubw   mm0, mm6            ; mm0 = 8-dy

    movq    mm7, mm5
    pmullw  mm5, mm0            ; mm5 = dx*(8-dy) =     cB
    pmullw  mm7, mm6            ; mm7 = dx*dy =         cD
    pmullw  mm6, mm4            ; mm6 = (8-dx)*dy =     cC
    pmullw  mm4, mm0            ; mm4 = (8-dx)*(8-dy) = cA

    push    edi

    mov     eax, [picesp+4+4]   ; src
    mov     edi, [picesp+4+12]  ; dst
    mov     ecx, [picesp+4+8]   ; i_src_stride
    mov     edx, [picesp+4+32]  ; i_height

ALIGN 4
.height_loop

    movd    mm1, [eax+ecx]
    movd    mm0, [eax]
    punpcklbw mm1, mm3          ; 00 px1 | 00 px2 | 00 px3 | 00 px4
    punpcklbw mm0, mm3
    pmullw  mm1, mm6            ; 2nd line * cC
    pmullw  mm0, mm4            ; 1st line * cA

    paddw   mm0, mm1            ; mm0 <- result

    movd    mm2, [eax+1]
    movd    mm1, [eax+ecx+1]
    punpcklbw mm2, mm3
    punpcklbw mm1, mm3

    paddw   mm0, [pw_32 GOT_ebx]

    pmullw  mm2, mm5            ; line * cB
    pmullw  mm1, mm7            ; line * cD
    paddw   mm0, mm2
    paddw   mm0, mm1

    psrlw   mm0, 6
    packuswb mm0, mm3           ; 00 00 00 00 px1 px2 px3 px4
    movd    [edi], mm0

    add     eax, ecx
    add     edi, [picesp+4+16]

    dec     edx
    jnz     .height_loop

    sub     [picesp+4+28], dword 8
    jnz     .finish            ; width != 8 so assume 4

    mov     edi, [picesp+4+12] ; dst
    mov     eax, [picesp+4+4]  ; src
    mov     edx, [picesp+4+32] ; i_height
    add     edi, 4
    add     eax, 4
    jmp    .height_loop

.finish
    pop     edi
    picpop  ebx
    ret

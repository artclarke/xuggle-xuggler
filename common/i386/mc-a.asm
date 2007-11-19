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

;=============================================================================
; pixel avg
;=============================================================================

;-----------------------------------------------------------------------------
; void x264_pixel_avg_w4_mmxext( uint8_t *dst, int dst_stride,
;                                uint8_t *src, int src_stride,
;                                int height );
;-----------------------------------------------------------------------------
%macro AVG_START 1
cglobal %1
    push   ebx
    mov    eax, [esp+12]  ; dst
    mov    ebx, [esp+16]  ; dst_stride
    mov    ecx, [esp+20]  ; src
    mov    edx, [esp+24]  ; src_stride
    ; esi = height
.height_loop:
%endmacro

%macro AVG_END 0
    sub    esi, 2
    lea    eax, [eax+ebx*2]
    lea    ecx, [ecx+edx*2]
    jg     .height_loop
    pop    ebx
    pop    esi
    ret
%endmacro

AVG_START x264_pixel_avg_w4_mmxext
    movd   mm0, [ecx]
    movd   mm1, [ecx+edx]
    pavgb  mm0, [eax]
    pavgb  mm1, [eax+ebx]
    movd   [eax], mm0
    movd   [eax+ebx], mm1
AVG_END

AVG_START x264_pixel_avg_w8_mmxext
    movq   mm0, [ecx]
    movq   mm1, [ecx+edx]
    pavgb  mm0, [eax]
    pavgb  mm1, [eax+ebx]
    movq   [eax], mm0
    movq   [eax+ebx], mm1
AVG_END

AVG_START x264_pixel_avg_w16_mmxext
    movq   mm0, [ecx]
    movq   mm1, [ecx+8]
    movq   mm2, [ecx+edx]
    movq   mm3, [ecx+edx+8]
    pavgb  mm0, [eax]
    pavgb  mm1, [eax+8]
    pavgb  mm2, [eax+ebx]
    pavgb  mm3, [eax+ebx+8]
    movq   [eax], mm0
    movq   [eax+8], mm1
    movq   [eax+ebx], mm2
    movq   [eax+ebx+8], mm3
AVG_END

AVG_START x264_pixel_avg_w16_sse2
    movdqu xmm0, [ecx]
    movdqu xmm1, [ecx+edx]
    pavgb  xmm0, [eax]
    pavgb  xmm1, [eax+ebx]
    movdqa [eax], xmm0
    movdqa [eax+ebx], xmm1
AVG_END

%macro AVGH 2
cglobal x264_pixel_avg_%1x%2_mmxext
    push esi
    mov esi, %2
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

%macro AVG2_START 1
cglobal %1
    push   ebx
    push   esi
    push   edi
    push   ebp
    mov    eax, [esp+20]  ; dst
    mov    ebx, [esp+24]  ; dst_stride
    mov    ecx, [esp+28]  ; src1
    mov    edx, [esp+32]  ; src_stride
    mov    edi, [esp+36]  ; src2
    mov    esi, [esp+40]  ; height
    sub    edi, ecx
    lea    ebp, [edi+edx]
.height_loop:
%endmacro

%macro AVG2_END 0
    sub    esi, 2
    lea    eax, [eax+ebx*2]
    lea    ecx, [ecx+edx*2]
    jg     .height_loop
    pop    ebp
    pop    edi
    pop    esi
    pop    ebx
    ret
%endmacro

AVG2_START x264_pixel_avg2_w4_mmxext
    movd   mm0, [ecx]
    movd   mm1, [ecx+edx]
    pavgb  mm0, [ecx+edi]
    pavgb  mm1, [ecx+ebp]
    movd   [eax], mm0
    movd   [eax+ebx], mm1
AVG2_END

AVG2_START x264_pixel_avg2_w8_mmxext
    movq   mm0, [ecx]
    movq   mm1, [ecx+edx]
    pavgb  mm0, [ecx+edi]
    pavgb  mm1, [ecx+ebp]
    movq   [eax], mm0
    movq   [eax+ebx], mm1
AVG2_END

AVG2_START x264_pixel_avg2_w16_mmxext
    movq   mm0, [ecx]
    movq   mm1, [ecx+8]
    movq   mm2, [ecx+edx]
    movq   mm3, [ecx+edx+8]
    pavgb  mm0, [ecx+edi]
    pavgb  mm1, [ecx+edi+8]
    pavgb  mm2, [ecx+ebp]
    pavgb  mm3, [ecx+ebp+8]
    movq   [eax], mm0
    movq   [eax+8], mm1
    movq   [eax+ebx], mm2
    movq   [eax+ebx+8], mm3
AVG2_END

AVG2_START x264_pixel_avg2_w20_mmxext
    movq   mm0, [ecx]
    movq   mm1, [ecx+8]
    movd   mm2, [ecx+16]
    movq   mm3, [ecx+edx]
    movq   mm4, [ecx+edx+8]
    movd   mm5, [ecx+edx+16]
    pavgb  mm0, [ecx+edi]
    pavgb  mm1, [ecx+edi+8]
    pavgb  mm2, [ecx+edi+16]
    pavgb  mm3, [ecx+ebp]
    pavgb  mm4, [ecx+ebp+8]
    pavgb  mm5, [ecx+ebp+16]
    movq   [eax], mm0
    movq   [eax+8], mm1
    movd   [eax+16], mm2
    movq   [eax+ebx], mm3
    movq   [eax+ebx+8], mm4
    movd   [eax+ebx+16], mm5
AVG2_END



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

;-----------------------------------------------------------------------------
;   int __cdecl x264_pixel_avg_weight_w16_mmxext( uint8_t *, int, uint8_t *, int, int, int )
;-----------------------------------------------------------------------------
cglobal x264_pixel_avg_weight_w16_mmxext
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
    jg   .height_loop
    BIWEIGHT_END_MMX

;-----------------------------------------------------------------------------
;   int __cdecl x264_pixel_avg_weight_w8_mmxext( uint8_t *, int, uint8_t *, int, int, int )
;-----------------------------------------------------------------------------
cglobal x264_pixel_avg_weight_w8_mmxext
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
    jg   .height_loop
    BIWEIGHT_END_MMX

;-----------------------------------------------------------------------------
;   int __cdecl x264_pixel_avg_weight_4x4_mmxext( uint8_t *, int, uint8_t *, int, int )
;-----------------------------------------------------------------------------
cglobal x264_pixel_avg_weight_4x4_mmxext
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

;-----------------------------------------------------------------------------
;  void x264_mc_copy_w4_mmx( uint8_t *src, int i_src_stride,
;                            uint8_t *dst, int i_dst_stride, int i_height )
;-----------------------------------------------------------------------------
cglobal x264_mc_copy_w4_mmx
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
    jg      .height_loop

    pop     edi
    pop     esi
    pop     ebx
    ret

;-----------------------------------------------------------------------------
;   void x264_mc_copy_w8_mmx( uint8_t *src, int i_src_stride,
;                             uint8_t *dst, int i_dst_stride, int i_height )
;-----------------------------------------------------------------------------
cglobal x264_mc_copy_w8_mmx
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
    jg      .height_loop

    pop     edi
    pop     esi
    pop     ebx
    ret

;-----------------------------------------------------------------------------
;   void x264_mc_copy_w16_mmx( uint8_t *src, int i_src_stride,
;                              uint8_t *dst, int i_dst_stride, int i_height )
;-----------------------------------------------------------------------------
cglobal x264_mc_copy_w16_mmx
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
    jg      .height_loop
    
    pop     edi
    pop     esi
    pop     ebx
    ret


;-----------------------------------------------------------------------------
;   void x264_mc_copy_w16_sse2( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_height )
;-----------------------------------------------------------------------------
cglobal x264_mc_copy_w16_sse2
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
    jg      .height_loop
    
    pop     edi
    pop     esi
    pop     ebx
    ret



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
    picpush ebx
    picgetgot ebx
    push    edi

    mov     ecx, [picesp+4+24]
    mov     edx, [picesp+4+20]
    mov     eax, ecx
    mov     edi, edx
    sar     ecx, 3
    sar     edx, 3
    imul    ecx, [picesp+4+16]
    add     ecx, edx
    add     [picesp+4+12], ecx   ; src += (dx>>3) + (dy>>3) * src_stride

    pxor    mm3, mm3

    and     edi, 7
    and     eax, 7
    movd    mm5, edi
    movd    mm6, eax
    pshufw  mm5, mm5, 0         ; mm5 = dx&7
    pshufw  mm6, mm6, 0         ; mm6 = dy&7

    movq    mm4, [pw_8 GOT_ebx]
    movq    mm0, mm4

    psubw   mm4, mm5            ; mm4 = 8-dx
    psubw   mm0, mm6            ; mm0 = 8-dy

    movq    mm7, mm5
    pmullw  mm5, mm0            ; mm5 = dx*(8-dy) =     cB
    pmullw  mm7, mm6            ; mm7 = dx*dy =         cD
    pmullw  mm6, mm4            ; mm6 = (8-dx)*dy =     cC
    pmullw  mm4, mm0            ; mm4 = (8-dx)*(8-dy) = cA

    mov     eax, [picesp+4+12]  ; src
    mov     edi, [picesp+4+4]   ; dst
    mov     ecx, [picesp+4+16]  ; i_src_stride
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
    add     edi, [picesp+4+8]

    dec     edx
    jnz     .height_loop

    sub     [picesp+4+28], dword 8
    jnz     .finish            ; width != 8 so assume 4

    mov     edi, [picesp+4+4]  ; dst
    mov     eax, [picesp+4+12] ; src
    mov     edx, [picesp+4+32] ; i_height
    add     edi, 4
    add     eax, 4
    jmp    .height_loop

.finish
    pop     edi
    picpop  ebx
    ret



; prefetches tuned for 64 byte cachelines (K7/K8/Core2)
; TODO add 32 and 128 byte versions for P3/P4

;-----------------------------------------------------------------------------
; void x264_prefetch_fenc_mmxext( uint8_t *pix_y, int stride_y, 
;                                 uint8_t *pix_uv, int stride_uv, int mb_x )
;-----------------------------------------------------------------------------
cglobal x264_prefetch_fenc_mmxext
    mov   eax, [esp+20]
    mov   ecx, [esp+8]
    mov   edx, [esp+4]
    and   eax, 3
    imul  eax, ecx
    lea   edx, [edx+eax*4+64]
    prefetcht0 [edx]
    prefetcht0 [edx+ecx]
    lea   edx, [edx+ecx*2]
    prefetcht0 [edx]
    prefetcht0 [edx+ecx]

    mov   eax, [esp+20]
    mov   ecx, [esp+16]
    mov   edx, [esp+12]
    and   eax, 6
    imul  eax, ecx
    lea   edx, [edx+eax+64]
    prefetcht0 [edx]
    prefetcht0 [edx+ecx]
    ret

;-----------------------------------------------------------------------------
; void x264_prefetch_ref_mmxext( uint8_t *pix, int stride, int parity )
;-----------------------------------------------------------------------------
cglobal x264_prefetch_ref_mmxext
    mov   eax, [esp+12]
    mov   ecx, [esp+8]
    mov   edx, [esp+4]
    sub   eax, 1
    and   eax, ecx
    lea   edx, [edx+eax*8+64]
    lea   eax, [ecx*3]
    prefetcht0 [edx]
    prefetcht0 [edx+ecx]
    prefetcht0 [edx+ecx*2]
    prefetcht0 [edx+eax]
    lea   edx, [edx+ecx*4]
    prefetcht0 [edx]
    prefetcht0 [edx+ecx]
    prefetcht0 [edx+ecx*2]
    prefetcht0 [edx+eax]
    ret

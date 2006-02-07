;*****************************************************************************
;* pixel.asm: h264 encoder library
;*****************************************************************************
;* Copyright (C) 2003 x264 project
;* $Id: pixel.asm,v 1.1 2004/06/03 19:27:07 fenrir Exp $
;*
;* Authors: Laurent Aimar <fenrir@via.ecp.fr>
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

BITS 32

;=============================================================================
; Macros and other preprocessor constants
;=============================================================================

%include "i386inc.asm"

%macro SAD_INC_2x16P 0
    movq    mm1,    [eax]
    movq    mm2,    [eax+8]
    movq    mm3,    [eax+ebx]
    movq    mm4,    [eax+ebx+8]
    psadbw  mm1,    [ecx]
    psadbw  mm2,    [ecx+8]
    psadbw  mm3,    [ecx+edx]
    psadbw  mm4,    [ecx+edx+8]
    lea     eax,    [eax+2*ebx]
    paddw   mm1,    mm2
    paddw   mm3,    mm4
    lea     ecx,    [ecx+2*edx]
    paddw   mm0,    mm1
    paddw   mm0,    mm3
%endmacro

%macro SAD_INC_2x8P 0
    movq    mm1,    [eax]
    movq    mm2,    [eax+ebx]
    psadbw  mm1,    [ecx]
    psadbw  mm2,    [ecx+edx]
    lea     eax,    [eax+2*ebx]
    paddw   mm0,    mm1
    paddw   mm0,    mm2
    lea     ecx,    [ecx+2*edx]
%endmacro

%macro SAD_INC_2x4P 0
    movd    mm1,    [eax]
    movd    mm2,    [ecx]
    movd    mm3,    [eax+ebx]
    movd    mm4,    [ecx+edx]

    psadbw  mm1,    mm2
    psadbw  mm3,    mm4
    paddw   mm0,    mm1
    paddw   mm0,    mm3

    lea     eax,    [eax+2*ebx]
    lea     ecx,    [ecx+2*edx]
%endmacro

%macro SSD_INC_1x16P 0
    movq    mm1,    [eax]
    movq    mm2,    [ecx]
    movq    mm3,    [eax+8]
    movq    mm4,    [ecx+8]

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

    add     eax,    ebx
    add     ecx,    edx
    paddd   mm0,    mm1
    paddd   mm0,    mm2
    paddd   mm0,    mm3
    paddd   mm0,    mm4
%endmacro

%macro SSD_INC_1x8P 0
    movq    mm1,    [eax]
    movq    mm2,    [ecx]

    movq    mm5,    mm2
    psubusb mm2,    mm1
    psubusb mm1,    mm5
    por     mm1,    mm2         ; mm1 = 8bit abs diff

    movq    mm2,    mm1
    punpcklbw mm1,  mm7
    punpckhbw mm2,  mm7         ; (mm1,mm2) = 16bit abs diff
    pmaddwd mm1,    mm1
    pmaddwd mm2,    mm2

    add     eax,    ebx
    add     ecx,    edx
    paddd   mm0,    mm1
    paddd   mm0,    mm2
%endmacro

%macro SSD_INC_1x4P 0
    movd    mm1,    [eax]
    movd    mm2,    [ecx]

    movq    mm5,    mm2
    psubusb mm2,    mm1
    psubusb mm1,    mm5
    por     mm1,    mm2
    punpcklbw mm1,  mm7
    pmaddwd mm1,    mm1

    add     eax,    ebx
    add     ecx,    edx
    paddd   mm0,    mm1
%endmacro

%macro SSD_INC_8x16P 0
    SSD_INC_1x16P
    SSD_INC_1x16P
    SSD_INC_1x16P
    SSD_INC_1x16P
    SSD_INC_1x16P
    SSD_INC_1x16P
    SSD_INC_1x16P
    SSD_INC_1x16P
%endmacro

%macro SSD_INC_4x8P 0
    SSD_INC_1x8P
    SSD_INC_1x8P
    SSD_INC_1x8P
    SSD_INC_1x8P
%endmacro

%macro SSD_INC_4x4P 0
    SSD_INC_1x4P
    SSD_INC_1x4P
    SSD_INC_1x4P
    SSD_INC_1x4P
%endmacro

%macro LOAD_DIFF_4P 4  ; MMP, MMT, [pix1], [pix2]
    movd        %1, %3
    movd        %2, %4
    punpcklbw   %1, %2
    punpcklbw   %2, %2
    psubw       %1, %2
%endmacro

%macro LOAD_DIFF_INC_4x4 10 ; p1,p2,p3,p4, t, pix1, i_pix1, pix2, i_pix2, offset
    LOAD_DIFF_4P %1, %5, [%6+%10],    [%8+%10]
    LOAD_DIFF_4P %2, %5, [%6+%7+%10], [%8+%9+%10]
    lea %6, [%6+2*%7]
    lea %8, [%8+2*%9]
    LOAD_DIFF_4P %3, %5, [%6+%10],    [%8+%10]
    LOAD_DIFF_4P %4, %5, [%6+%7+%10], [%8+%9+%10]
    lea %6, [%6+2*%7]
    lea %8, [%8+2*%9]
%endmacro

%macro LOAD_DIFF_4x4 10 ; p1,p2,p3,p4, t, pix1, i_pix1, pix2, i_pix2, offset
    LOAD_DIFF_4P %1, %5, [%6+%10],      [%8+%10]
    LOAD_DIFF_4P %3, %5, [%6+2*%7+%10], [%8+2*%9+%10]
    add     %6, %7
    add     %8, %9
    LOAD_DIFF_4P %2, %5, [%6+%10],      [%8+%10]
    LOAD_DIFF_4P %4, %5, [%6+2*%7+%10], [%8+2*%9+%10]
%endmacro

%macro HADAMARD4_SUB_BADC 4
    paddw %1,   %2
    paddw %3,   %4
    paddw %2,   %2
    paddw %4,   %4
    psubw %2,   %1
    psubw %4,   %3
%endmacro

%macro HADAMARD4x4 4
    HADAMARD4_SUB_BADC %1, %2, %3, %4
    HADAMARD4_SUB_BADC %1, %3, %2, %4
%endmacro

%macro SBUTTERFLYwd 3
    movq        %3, %1
    punpcklwd   %1, %2
    punpckhwd   %3, %2
%endmacro

%macro SBUTTERFLYdq 3
    movq        %3, %1
    punpckldq   %1, %2
    punpckhdq   %3, %2
%endmacro

%macro TRANSPOSE4x4 5   ; abcd-t -> adtc
    SBUTTERFLYwd %1, %2, %5
    SBUTTERFLYwd %3, %4, %2
    SBUTTERFLYdq %1, %3, %4
    SBUTTERFLYdq %5, %2, %3
%endmacro

%macro MMX_ABS 2        ; mma, mmt
    pxor    %2, %2
    psubw   %2, %1
    pmaxsw  %1, %2
%endmacro

%macro HADAMARD4x4_SUM 1    ; %1 - dest (row sum of one block)
    HADAMARD4x4 mm4, mm5, mm6, mm7
    TRANSPOSE4x4 mm4, mm5, mm6, mm7, %1
    HADAMARD4x4 mm4, mm7, %1, mm6
    MMX_ABS     mm4, mm5
    MMX_ABS     mm7, mm5
    MMX_ABS     %1,  mm5
    MMX_ABS     mm6, mm5
    paddw       %1,  mm4
    paddw       mm6, mm7
    pavgw       %1,  mm6
%endmacro

;=============================================================================
; Code
;=============================================================================

SECTION .text

cglobal x264_pixel_sad_16x16_mmxext
cglobal x264_pixel_sad_16x8_mmxext
cglobal x264_pixel_sad_8x16_mmxext
cglobal x264_pixel_sad_8x8_mmxext
cglobal x264_pixel_sad_8x4_mmxext
cglobal x264_pixel_sad_4x8_mmxext
cglobal x264_pixel_sad_4x4_mmxext

cglobal x264_pixel_sad_pde_16x16_mmxext
cglobal x264_pixel_sad_pde_16x8_mmxext
cglobal x264_pixel_sad_pde_8x16_mmxext

cglobal x264_pixel_ssd_16x16_mmxext
cglobal x264_pixel_ssd_16x8_mmxext
cglobal x264_pixel_ssd_8x16_mmxext
cglobal x264_pixel_ssd_8x8_mmxext
cglobal x264_pixel_ssd_8x4_mmxext
cglobal x264_pixel_ssd_4x8_mmxext
cglobal x264_pixel_ssd_4x4_mmxext

cglobal x264_pixel_satd_4x4_mmxext
cglobal x264_pixel_satd_4x8_mmxext
cglobal x264_pixel_satd_8x4_mmxext
cglobal x264_pixel_satd_8x8_mmxext
cglobal x264_pixel_satd_16x8_mmxext
cglobal x264_pixel_satd_8x16_mmxext
cglobal x264_pixel_satd_16x16_mmxext

%macro SAD_START 0
    push    ebx

    mov     eax,    [esp+ 8]    ; pix1
    mov     ebx,    [esp+12]    ; stride1
    mov     ecx,    [esp+16]    ; pix2
    mov     edx,    [esp+20]    ; stride2

    pxor    mm0,    mm0
%endmacro
%macro SAD_END 0
    movd eax,    mm0

    pop ebx
    ret
%endmacro

ALIGN 16
;-----------------------------------------------------------------------------
;   int __cdecl x264_pixel_sad_16x16_mmxext (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
x264_pixel_sad_16x16_mmxext:
    SAD_START
    SAD_INC_2x16P
    SAD_INC_2x16P
    SAD_INC_2x16P
    SAD_INC_2x16P
    SAD_INC_2x16P
    SAD_INC_2x16P
    SAD_INC_2x16P
    SAD_INC_2x16P
    SAD_END

ALIGN 16
;-----------------------------------------------------------------------------
;   int __cdecl x264_pixel_sad_16x8_mmxext (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
x264_pixel_sad_16x8_mmxext:
    SAD_START
    SAD_INC_2x16P
    SAD_INC_2x16P
    SAD_INC_2x16P
    SAD_INC_2x16P
    SAD_END

ALIGN 16
;-----------------------------------------------------------------------------
;   int __cdecl x264_pixel_sad_8x16_mmxext (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
x264_pixel_sad_8x16_mmxext:
    SAD_START
    SAD_INC_2x8P
    SAD_INC_2x8P
    SAD_INC_2x8P
    SAD_INC_2x8P
    SAD_INC_2x8P
    SAD_INC_2x8P
    SAD_INC_2x8P
    SAD_INC_2x8P
    SAD_END

ALIGN 16
;-----------------------------------------------------------------------------
;   int __cdecl x264_pixel_sad_8x8_mmxext (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
x264_pixel_sad_8x8_mmxext:
    SAD_START
    SAD_INC_2x8P
    SAD_INC_2x8P
    SAD_INC_2x8P
    SAD_INC_2x8P
    SAD_END

ALIGN 16
;-----------------------------------------------------------------------------
;   int __cdecl x264_pixel_sad_8x4_mmxext (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
x264_pixel_sad_8x4_mmxext:
    SAD_START
    SAD_INC_2x8P
    SAD_INC_2x8P
    SAD_END

ALIGN 16
;-----------------------------------------------------------------------------
;   int __cdecl x264_pixel_sad_4x8_mmxext (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
x264_pixel_sad_4x8_mmxext:
    SAD_START
    SAD_INC_2x4P
    SAD_INC_2x4P
    SAD_INC_2x4P
    SAD_INC_2x4P
    SAD_END

ALIGN 16
;-----------------------------------------------------------------------------
;   int __cdecl x264_pixel_sad_4x4_mmxext (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
x264_pixel_sad_4x4_mmxext:
    SAD_START
    SAD_INC_2x4P
    SAD_INC_2x4P
    SAD_END


%macro PDE_CHECK 0
    movd ebx, mm0
    cmp  ebx, [esp+24] ; prev_score
    jl   .continue
    pop  ebx
    mov  eax, 0xffff
    ret
ALIGN 4
.continue:
    mov  ebx, [esp+12]
%endmacro

ALIGN 16
;-----------------------------------------------------------------------------
;   int __cdecl x264_pixel_sad_pde_16x16_mmxext (uint8_t *, int, uint8_t *, int, int )
;-----------------------------------------------------------------------------
x264_pixel_sad_pde_16x16_mmxext:
    SAD_START
    SAD_INC_2x16P
    SAD_INC_2x16P
    SAD_INC_2x16P
    SAD_INC_2x16P
    PDE_CHECK
    SAD_INC_2x16P
    SAD_INC_2x16P
    SAD_INC_2x16P
    SAD_INC_2x16P
    SAD_END

ALIGN 16
;-----------------------------------------------------------------------------
;   int __cdecl x264_pixel_sad_pde_16x8_mmxext (uint8_t *, int, uint8_t *, int, int )
;-----------------------------------------------------------------------------
x264_pixel_sad_pde_16x8_mmxext:
    SAD_START
    SAD_INC_2x16P
    SAD_INC_2x16P
    PDE_CHECK
    SAD_INC_2x16P
    SAD_INC_2x16P
    SAD_END

ALIGN 16
;-----------------------------------------------------------------------------
;   int __cdecl x264_pixel_sad_pde_8x16_mmxext (uint8_t *, int, uint8_t *, int, int )
;-----------------------------------------------------------------------------
x264_pixel_sad_pde_8x16_mmxext:
    SAD_START
    SAD_INC_2x8P
    SAD_INC_2x8P
    SAD_INC_2x8P
    SAD_INC_2x8P
    PDE_CHECK
    SAD_INC_2x8P
    SAD_INC_2x8P
    SAD_INC_2x8P
    SAD_INC_2x8P
    SAD_END



%macro SSD_START 0
    push    ebx

    mov     eax,    [esp+ 8]    ; pix1
    mov     ebx,    [esp+12]    ; stride1
    mov     ecx,    [esp+16]    ; pix2
    mov     edx,    [esp+20]    ; stride2

    pxor    mm7,    mm7         ; zero
    pxor    mm0,    mm0         ; mm0 holds the sum
%endmacro

%macro SSD_END 0
    movq    mm1,    mm0
    psrlq   mm1,    32
    paddd   mm0,    mm1
    movd    eax,    mm0

    pop ebx
    ret
%endmacro

ALIGN 16
;-----------------------------------------------------------------------------
;   int __cdecl x264_pixel_ssd_16x16_mmxext (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
x264_pixel_ssd_16x16_mmxext:
    SSD_START
    SSD_INC_8x16P
    SSD_INC_8x16P
    SSD_END

ALIGN 16
x264_pixel_ssd_16x8_mmxext:
    SSD_START
    SSD_INC_8x16P
    SSD_END

ALIGN 16
x264_pixel_ssd_8x16_mmxext:
    SSD_START
    SSD_INC_4x8P
    SSD_INC_4x8P
    SSD_INC_4x8P
    SSD_INC_4x8P
    SSD_END

ALIGN 16
x264_pixel_ssd_8x8_mmxext:
    SSD_START
    SSD_INC_4x8P
    SSD_INC_4x8P
    SSD_END

ALIGN 16
x264_pixel_ssd_8x4_mmxext:
    SSD_START
    SSD_INC_4x8P
    SSD_END

ALIGN 16
x264_pixel_ssd_4x8_mmxext:
    SSD_START
    SSD_INC_4x4P
    SSD_INC_4x4P
    SSD_END

ALIGN 16
x264_pixel_ssd_4x4_mmxext:
    SSD_START
    SSD_INC_4x4P
    SSD_END



%macro SATD_START 0
    push        ebx

    mov         eax, [esp+ 8]       ; pix1
    mov         ebx, [esp+12]       ; stride1
    mov         ecx, [esp+16]       ; pix2
    mov         edx, [esp+20]       ; stride2
%endmacro

%macro SATD_END 0
    pshufw      mm1, mm0, 01001110b
    paddw       mm0, mm1
    pshufw      mm1, mm0, 10110001b
    paddw       mm0, mm1
    movd        eax, mm0
    and         eax, 0xffff
    pop         ebx
    ret
%endmacro

ALIGN 16
;-----------------------------------------------------------------------------
;   int __cdecl x264_pixel_satd_4x4_mmxext (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
x264_pixel_satd_4x4_mmxext:
    SATD_START
    LOAD_DIFF_4x4       mm4, mm5, mm6, mm7, mm0, eax, ebx, ecx, edx, 0
    HADAMARD4x4_SUM     mm0
    SATD_END

ALIGN 16
;-----------------------------------------------------------------------------
;   int __cdecl x264_pixel_satd_4x8_mmxext (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
x264_pixel_satd_4x8_mmxext:
    SATD_START
    LOAD_DIFF_INC_4x4   mm4, mm5, mm6, mm7, mm0, eax, ebx, ecx, edx, 0
    HADAMARD4x4_SUM     mm0
    LOAD_DIFF_4x4       mm4, mm5, mm6, mm7, mm1, eax, ebx, ecx, edx, 0
    HADAMARD4x4_SUM     mm1
    paddw       mm0, mm1
    SATD_END

ALIGN 16
;-----------------------------------------------------------------------------
;   int __cdecl x264_pixel_satd_8x4_mmxext (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
x264_pixel_satd_8x4_mmxext:
    SATD_START
    LOAD_DIFF_4x4       mm4, mm5, mm6, mm7, mm0, eax, ebx, ecx, edx, 0
    HADAMARD4x4_SUM     mm0
    sub         eax, ebx
    sub         ecx, edx
    LOAD_DIFF_4x4       mm4, mm5, mm6, mm7, mm1, eax, ebx, ecx, edx, 4
    HADAMARD4x4_SUM     mm1
    paddw       mm0, mm1
    SATD_END

ALIGN 16
;-----------------------------------------------------------------------------
;   int __cdecl x264_pixel_satd_8x8_mmxext (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
x264_pixel_satd_8x8_mmxext:
    SATD_START
    LOAD_DIFF_INC_4x4   mm4, mm5, mm6, mm7, mm0, eax, ebx, ecx, edx, 0
    HADAMARD4x4_SUM     mm0
    LOAD_DIFF_4x4       mm4, mm5, mm6, mm7, mm1, eax, ebx, ecx, edx, 0
    HADAMARD4x4_SUM     mm1

    mov         eax, [esp+ 8]       ; pix1
    mov         ecx, [esp+16]       ; pix2
    LOAD_DIFF_INC_4x4   mm4, mm5, mm6, mm7, mm2, eax, ebx, ecx, edx, 4
    HADAMARD4x4_SUM     mm2
    LOAD_DIFF_4x4       mm4, mm5, mm6, mm7, mm3, eax, ebx, ecx, edx, 4
    HADAMARD4x4_SUM     mm3
    paddw       mm0, mm1
    paddw       mm2, mm3
    paddw       mm0, mm2
    SATD_END

ALIGN 16
;-----------------------------------------------------------------------------
;   int __cdecl x264_pixel_satd_16x8_mmxext (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
x264_pixel_satd_16x8_mmxext:
    SATD_START
    LOAD_DIFF_INC_4x4   mm4, mm5, mm6, mm7, mm0, eax, ebx, ecx, edx, 0
    HADAMARD4x4_SUM     mm0
    LOAD_DIFF_4x4       mm4, mm5, mm6, mm7, mm1, eax, ebx, ecx, edx, 0
    HADAMARD4x4_SUM     mm1

    mov         eax, [esp+ 8]       ; pix1
    mov         ecx, [esp+16]       ; pix2
    LOAD_DIFF_INC_4x4   mm4, mm5, mm6, mm7, mm2, eax, ebx, ecx, edx, 4
    HADAMARD4x4_SUM     mm2
    LOAD_DIFF_4x4       mm4, mm5, mm6, mm7, mm3, eax, ebx, ecx, edx, 4
    HADAMARD4x4_SUM     mm3
    paddw       mm0, mm1
    paddw       mm2, mm3
    paddw       mm0, mm2

    mov         eax, [esp+ 8]       ; pix1
    mov         ecx, [esp+16]       ; pix2
    LOAD_DIFF_INC_4x4   mm4, mm5, mm6, mm7, mm1, eax, ebx, ecx, edx, 8
    HADAMARD4x4_SUM     mm1
    LOAD_DIFF_4x4       mm4, mm5, mm6, mm7, mm2, eax, ebx, ecx, edx, 8
    HADAMARD4x4_SUM     mm2
    paddw       mm1, mm2

    mov         eax, [esp+ 8]       ; pix1
    mov         ecx, [esp+16]       ; pix2
    LOAD_DIFF_INC_4x4   mm4, mm5, mm6, mm7, mm2, eax, ebx, ecx, edx, 12
    HADAMARD4x4_SUM     mm2
    LOAD_DIFF_4x4       mm4, mm5, mm6, mm7, mm3, eax, ebx, ecx, edx, 12
    HADAMARD4x4_SUM     mm3
    paddw       mm0, mm1
    paddw       mm2, mm3
    paddw       mm0, mm2
    SATD_END

ALIGN 16
;-----------------------------------------------------------------------------
;   int __cdecl x264_pixel_satd_8x16_mmxext (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
x264_pixel_satd_8x16_mmxext:
    SATD_START
    LOAD_DIFF_INC_4x4   mm4, mm5, mm6, mm7, mm0, eax, ebx, ecx, edx, 0
    HADAMARD4x4_SUM     mm0
    LOAD_DIFF_INC_4x4   mm4, mm5, mm6, mm7, mm1, eax, ebx, ecx, edx, 0
    HADAMARD4x4_SUM     mm1
    LOAD_DIFF_INC_4x4   mm4, mm5, mm6, mm7, mm2, eax, ebx, ecx, edx, 0
    HADAMARD4x4_SUM     mm2
    LOAD_DIFF_4x4       mm4, mm5, mm6, mm7, mm3, eax, ebx, ecx, edx, 0
    HADAMARD4x4_SUM     mm3
    paddw       mm0, mm1
    paddw       mm2, mm3
    paddw       mm0, mm2

    mov         eax, [esp+ 8]       ; pix1
    mov         ecx, [esp+16]       ; pix2
    LOAD_DIFF_INC_4x4   mm4, mm5, mm6, mm7, mm1, eax, ebx, ecx, edx, 4
    HADAMARD4x4_SUM     mm1
    LOAD_DIFF_INC_4x4   mm4, mm5, mm6, mm7, mm2, eax, ebx, ecx, edx, 4
    HADAMARD4x4_SUM     mm2
    paddw       mm1, mm2

    LOAD_DIFF_INC_4x4   mm4, mm5, mm6, mm7, mm2, eax, ebx, ecx, edx, 4
    HADAMARD4x4_SUM     mm2
    LOAD_DIFF_4x4       mm4, mm5, mm6, mm7, mm3, eax, ebx, ecx, edx, 4
    HADAMARD4x4_SUM     mm3
    paddw       mm0, mm1
    paddw       mm2, mm3
    paddw       mm0, mm2
    SATD_END

ALIGN 16
;-----------------------------------------------------------------------------
;   int __cdecl x264_pixel_satd_16x16_mmxext (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
x264_pixel_satd_16x16_mmxext:
    SATD_START
    LOAD_DIFF_INC_4x4   mm4, mm5, mm6, mm7, mm0, eax, ebx, ecx, edx, 0
    HADAMARD4x4_SUM     mm0
    LOAD_DIFF_INC_4x4   mm4, mm5, mm6, mm7, mm1, eax, ebx, ecx, edx, 0
    HADAMARD4x4_SUM     mm1
    LOAD_DIFF_INC_4x4   mm4, mm5, mm6, mm7, mm2, eax, ebx, ecx, edx, 0
    HADAMARD4x4_SUM     mm2
    LOAD_DIFF_4x4       mm4, mm5, mm6, mm7, mm3, eax, ebx, ecx, edx, 0
    HADAMARD4x4_SUM     mm3
    paddw       mm0, mm1
    paddw       mm2, mm3
    paddw       mm0, mm2

    mov         eax, [esp+ 8]       ; pix1
    mov         ecx, [esp+16]       ; pix2
    LOAD_DIFF_INC_4x4   mm4, mm5, mm6, mm7, mm1, eax, ebx, ecx, edx, 4
    HADAMARD4x4_SUM     mm1
    LOAD_DIFF_INC_4x4   mm4, mm5, mm6, mm7, mm2, eax, ebx, ecx, edx, 4
    HADAMARD4x4_SUM     mm2
    paddw       mm1, mm2

    LOAD_DIFF_INC_4x4   mm4, mm5, mm6, mm7, mm2, eax, ebx, ecx, edx, 4
    HADAMARD4x4_SUM     mm2
    LOAD_DIFF_4x4       mm4, mm5, mm6, mm7, mm3, eax, ebx, ecx, edx, 4
    HADAMARD4x4_SUM     mm3
    paddw       mm0, mm1
    paddw       mm2, mm3
    paddw       mm0, mm2

    mov         eax, [esp+ 8]       ; pix1
    mov         ecx, [esp+16]       ; pix2
    LOAD_DIFF_INC_4x4   mm4, mm5, mm6, mm7, mm1, eax, ebx, ecx, edx, 8
    HADAMARD4x4_SUM     mm1
    LOAD_DIFF_INC_4x4   mm4, mm5, mm6, mm7, mm2, eax, ebx, ecx, edx, 8
    HADAMARD4x4_SUM     mm2
    paddw       mm1, mm2

    LOAD_DIFF_INC_4x4   mm4, mm5, mm6, mm7, mm2, eax, ebx, ecx, edx, 8
    HADAMARD4x4_SUM     mm2
    LOAD_DIFF_4x4       mm4, mm5, mm6, mm7, mm3, eax, ebx, ecx, edx, 8
    HADAMARD4x4_SUM     mm3
    paddw       mm0, mm1
    paddw       mm2, mm3
    paddw       mm0, mm2

    mov         eax, [esp+ 8]       ; pix1
    mov         ecx, [esp+16]       ; pix2
    LOAD_DIFF_INC_4x4   mm4, mm5, mm6, mm7, mm1, eax, ebx, ecx, edx, 12
    HADAMARD4x4_SUM     mm1
    LOAD_DIFF_INC_4x4   mm4, mm5, mm6, mm7, mm2, eax, ebx, ecx, edx, 12
    HADAMARD4x4_SUM     mm2
    paddw       mm1, mm2

    LOAD_DIFF_INC_4x4   mm4, mm5, mm6, mm7, mm2, eax, ebx, ecx, edx, 12
    HADAMARD4x4_SUM     mm2
    LOAD_DIFF_4x4       mm4, mm5, mm6, mm7, mm3, eax, ebx, ecx, edx, 12
    HADAMARD4x4_SUM     mm3
    paddw       mm0, mm1
    paddw       mm2, mm3
    paddw       mm0, mm2

    pxor        mm3, mm3
    pshufw      mm1, mm0, 01001110b
    paddw       mm0, mm1
    punpcklwd   mm0, mm3
    pshufw      mm1, mm0, 01001110b
    paddd       mm0, mm1
    movd        eax, mm0
    pop         ebx
    ret


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

; sad

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
    punpckldq mm1,  [eax+ebx]
    punpckldq mm2,  [ecx+edx]
    psadbw  mm1,    mm2
    paddw   mm0,    mm1
    lea     eax,    [eax+2*ebx]
    lea     ecx,    [ecx+2*edx]
%endmacro

; sad x3 / x4

%macro SAD_X3_START 0
    push    edi
    push    esi
    mov     edi,    [esp+12]
    mov     eax,    [esp+16]
    mov     ecx,    [esp+20]
    mov     edx,    [esp+24]
    mov     esi,    [esp+28]
%endmacro

%macro SAD_X3_START_1x8P 0
    movq    mm3,    [edi]
    movq    mm0,    [eax]
    movq    mm1,    [ecx]
    movq    mm2,    [edx]
    psadbw  mm0,    mm3
    psadbw  mm1,    mm3
    psadbw  mm2,    mm3
%endmacro

%macro SAD_X3_1x8P 2
    movq    mm3,    [edi+%1]
    movq    mm4,    [eax+%2]
    movq    mm5,    [ecx+%2]
    movq    mm6,    [edx+%2]
    psadbw  mm4,    mm3
    psadbw  mm5,    mm3
    psadbw  mm6,    mm3
    paddw   mm0,    mm4
    paddw   mm1,    mm5
    paddw   mm2,    mm6
%endmacro

%macro SAD_X3_START_2x4P 3
    movd      mm3,  [edi]
    movd      %1,   [eax]
    movd      %2,   [ecx]
    movd      %3,   [edx]
    punpckldq mm3,  [edi+FENC_STRIDE]
    punpckldq %1,   [eax+esi]
    punpckldq %2,   [ecx+esi]
    punpckldq %3,   [edx+esi]
    psadbw    %1,   mm3
    psadbw    %2,   mm3
    psadbw    %3,   mm3
%endmacro

%macro SAD_X3_2x16P 1
%if %1
    SAD_X3_START
    SAD_X3_START_1x8P
%else
    SAD_X3_1x8P 0, 0
%endif
    SAD_X3_1x8P 8, 8
    SAD_X3_1x8P FENC_STRIDE, esi
    SAD_X3_1x8P FENC_STRIDE+8, esi+8
    add     edi, 2*FENC_STRIDE
    lea     eax, [eax+2*esi]
    lea     ecx, [ecx+2*esi]
    lea     edx, [edx+2*esi]
%endmacro

%macro SAD_X3_2x8P 1
%if %1
    SAD_X3_START
    SAD_X3_START_1x8P
%else
    SAD_X3_1x8P 0, 0
%endif
    SAD_X3_1x8P FENC_STRIDE, esi
    add     edi, 2*FENC_STRIDE
    lea     eax, [eax+2*esi]
    lea     ecx, [ecx+2*esi]
    lea     edx, [edx+2*esi]
%endmacro

%macro SAD_X3_2x4P 1
%if %1
    SAD_X3_START
    SAD_X3_START_2x4P mm0, mm1, mm2
%else
    SAD_X3_START_2x4P mm4, mm5, mm6
    paddw   mm0, mm4
    paddw   mm1, mm5
    paddw   mm2, mm6
%endif
    add     edi, 2*FENC_STRIDE
    lea     eax, [eax+2*esi]
    lea     ecx, [ecx+2*esi]
    lea     edx, [edx+2*esi]
%endmacro

%macro SAD_X4_START 0
    push    edi
    push    esi
    push    ebx
    mov     edi,    [esp+16]
    mov     eax,    [esp+20]
    mov     ebx,    [esp+24]
    mov     ecx,    [esp+28]
    mov     edx,    [esp+32]
    mov     esi,    [esp+36]
%endmacro

%macro SAD_X4_START_1x8P 0
    movq    mm7,    [edi]
    movq    mm0,    [eax]
    movq    mm1,    [ebx]
    movq    mm2,    [ecx]
    movq    mm3,    [edx]
    psadbw  mm0,    mm7
    psadbw  mm1,    mm7
    psadbw  mm2,    mm7
    psadbw  mm3,    mm7
%endmacro

%macro SAD_X4_1x8P 2
    movq    mm7,    [edi+%1]
    movq    mm4,    [eax+%2]
    movq    mm5,    [ebx+%2]
    movq    mm6,    [ecx+%2]
    psadbw  mm4,    mm7
    psadbw  mm5,    mm7
    psadbw  mm6,    mm7
    psadbw  mm7,    [edx+%2]
    paddw   mm0,    mm4
    paddw   mm1,    mm5
    paddw   mm2,    mm6
    paddw   mm3,    mm7
%endmacro

%macro SAD_X4_START_2x4P 0
    movd      mm7,  [edi]
    movd      mm0,  [eax]
    movd      mm1,  [ebx]
    movd      mm2,  [ecx]
    movd      mm3,  [edx]
    punpckldq mm7,  [edi+FENC_STRIDE]
    punpckldq mm0,  [eax+esi]
    punpckldq mm1,  [ebx+esi]
    punpckldq mm2,  [ecx+esi]
    punpckldq mm3,  [edx+esi]
    psadbw    mm0,  mm7
    psadbw    mm1,  mm7
    psadbw    mm2,  mm7
    psadbw    mm3,  mm7
%endmacro   
    
%macro SAD_X4_INC_2x4P 0
    movd      mm7,  [edi]
    movd      mm4,  [eax]
    movd      mm5,  [ebx]
    punpckldq mm7,  [edi+FENC_STRIDE]
    punpckldq mm4,  [eax+esi]
    punpckldq mm5,  [ebx+esi]
    psadbw    mm4,  mm7
    psadbw    mm5,  mm7
    paddw     mm0,  mm4
    paddw     mm1,  mm5
    movd      mm4,  [ecx]
    movd      mm5,  [edx]
    punpckldq mm4,  [ecx+esi]
    punpckldq mm5,  [edx+esi]
    psadbw    mm4,  mm7
    psadbw    mm5,  mm7
    paddw     mm2,  mm4
    paddw     mm3,  mm5
%endmacro

%macro SAD_X4_2x16P 1
%if %1
    SAD_X4_START
    SAD_X4_START_1x8P
%else
    SAD_X4_1x8P 0, 0
%endif
    SAD_X4_1x8P 8, 8
    SAD_X4_1x8P FENC_STRIDE, esi
    SAD_X4_1x8P FENC_STRIDE+8, esi+8
    add     edi, 2*FENC_STRIDE
    lea     eax, [eax+2*esi]
    lea     ebx, [ebx+2*esi]
    lea     ecx, [ecx+2*esi]
    lea     edx, [edx+2*esi]
%endmacro

%macro SAD_X4_2x8P 1
%if %1
    SAD_X4_START
    SAD_X4_START_1x8P
%else
    SAD_X4_1x8P 0, 0
%endif
    SAD_X4_1x8P FENC_STRIDE, esi
    add     edi, 2*FENC_STRIDE
    lea     eax, [eax+2*esi]
    lea     ebx, [ebx+2*esi]
    lea     ecx, [ecx+2*esi]
    lea     edx, [edx+2*esi]
%endmacro

%macro SAD_X4_2x4P 1
%if %1
    SAD_X4_START
    SAD_X4_START_2x4P
%else
    SAD_X4_INC_2x4P
%endif
    add     edi, 2*FENC_STRIDE
    lea     eax, [eax+2*esi]
    lea     ebx, [ebx+2*esi]
    lea     ecx, [ecx+2*esi]
    lea     edx, [edx+2*esi]
%endmacro

%macro SAD_X3_END 0
    mov     eax,  [esp+32]
    movd    [eax+0], mm0
    movd    [eax+4], mm1  
    movd    [eax+8], mm2
    pop     esi
    pop     edi
    ret
%endmacro

%macro SAD_X4_END 0
    mov     eax,  [esp+40]
    movd    [eax+0], mm0
    movd    [eax+4], mm1  
    movd    [eax+8], mm2
    movd    [eax+12], mm3
    pop     ebx
    pop     esi
    pop     edi
    ret
%endmacro

; ssd

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

; satd

%macro SUMSUB_BADC 4
    paddw %1,   %2
    paddw %3,   %4
    paddw %2,   %2
    paddw %4,   %4
    psubw %2,   %1
    psubw %4,   %3
%endmacro

%macro HADAMARD4x4 4
    SUMSUB_BADC %1, %2, %3, %4
    SUMSUB_BADC %1, %3, %2, %4
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

%macro MMX_ABS 2        ; mma, tmp
    pxor    %2, %2
    psubw   %2, %1
    pmaxsw  %1, %2
%endmacro

%macro MMX_ABS_TWO 4    ; mma, mmb, tmp0, tmp1
    pxor    %3, %3
    pxor    %4, %4
    psubw   %3, %1
    psubw   %4, %2
    pmaxsw  %1, %3
    pmaxsw  %2, %4
%endmacro

%macro HADAMARD4x4_SUM 1    ; %1 - dest (row sum of one block)
    HADAMARD4x4 mm4, mm5, mm6, mm7
    TRANSPOSE4x4 mm4, mm5, mm6, mm7, %1
    HADAMARD4x4 mm4, mm7, %1, mm6
    MMX_ABS_TWO mm4, mm7, mm3, mm5
    MMX_ABS_TWO %1,  mm6, mm3, mm5
    paddw       %1,  mm4
    paddw       mm6, mm7
    pavgw       %1,  mm6
%endmacro

%macro LOAD_DIFF_4P 4  ; mmp, mmt, dx, dy
    movd        %1, [eax+ebx*%4+%3]
    movd        %2, [ecx+edx*%4+%3]
    punpcklbw   %1, %2
    punpcklbw   %2, %2
    psubw       %1, %2
%endmacro

; in: %2 = horizontal offset
; in: %3 = whether we need to increment pix1 and pix2
; clobber: mm3..mm7
; out: %1 = satd
%macro LOAD_DIFF_HADAMARD_SUM 3
%if %3
    LOAD_DIFF_4P mm4, mm3, %2, 0
    LOAD_DIFF_4P mm5, mm3, %2, 1
    lea  eax, [eax+2*ebx]
    lea  ecx, [ecx+2*edx]
    LOAD_DIFF_4P mm6, mm3, %2, 0
    LOAD_DIFF_4P mm7, mm3, %2, 1
    lea  eax, [eax+2*ebx]
    lea  ecx, [ecx+2*edx]
%else
    LOAD_DIFF_4P mm4, mm3, %2, 0
    LOAD_DIFF_4P mm6, mm3, %2, 2
    add  eax, ebx
    add  ecx, edx
    LOAD_DIFF_4P mm5, mm3, %2, 0
    LOAD_DIFF_4P mm7, mm3, %2, 2
%endif
    HADAMARD4x4_SUM %1
%endmacro

;=============================================================================
; Code
;=============================================================================

SECTION .text

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

;-----------------------------------------------------------------------------
;   int __cdecl x264_pixel_sad_16x16_mmxext (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
%macro SAD 2
cglobal x264_pixel_sad_%1x%2_mmxext
    SAD_START
%rep %2/2
    SAD_INC_2x%1P
%endrep
    SAD_END
%endmacro

SAD 16, 16
SAD 16,  8
SAD  8, 16
SAD  8,  8
SAD  8,  4
SAD  4,  8
SAD  4,  4

;-----------------------------------------------------------------------------
;  void x264_pixel_sad_x3_16x16_mmxext( uint8_t *fenc, uint8_t *pix0, uint8_t *pix1,
;                                       uint8_t *pix2, int i_stride, int scores[3] )
;-----------------------------------------------------------------------------
%macro SAD_X 3
cglobal x264_pixel_sad_x%1_%2x%3_mmxext
    SAD_X%1_2x%2P 1
%rep %3/2-1
    SAD_X%1_2x%2P 0
%endrep
    SAD_X%1_END
%endmacro

SAD_X 3, 16, 16
SAD_X 3, 16,  8
SAD_X 3,  8, 16
SAD_X 3,  8,  8
SAD_X 3,  8,  4
SAD_X 3,  4,  8
SAD_X 3,  4,  4
SAD_X 4, 16, 16
SAD_X 4, 16,  8
SAD_X 4,  8, 16
SAD_X 4,  8,  8
SAD_X 4,  8,  4
SAD_X 4,  4,  8
SAD_X 4,  4,  4


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

;-----------------------------------------------------------------------------
;   int __cdecl x264_pixel_ssd_16x16_mmx (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
%macro SSD 2
cglobal x264_pixel_ssd_%1x%2_mmx
    SSD_START
%rep %2
    SSD_INC_1x%1P
%endrep
    SSD_END
%endmacro

SSD 16, 16
SSD 16,  8
SSD  8, 16
SSD  8,  8
SSD  8,  4
SSD  4,  8
SSD  4,  4



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

;-----------------------------------------------------------------------------
;   int __cdecl x264_pixel_satd_4x4_mmxext (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
cglobal x264_pixel_satd_4x4_mmxext
    SATD_START
    LOAD_DIFF_HADAMARD_SUM mm0, 0, 0
    SATD_END

;-----------------------------------------------------------------------------
;   int __cdecl x264_pixel_satd_4x8_mmxext (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
cglobal x264_pixel_satd_4x8_mmxext
    SATD_START
    LOAD_DIFF_HADAMARD_SUM mm0, 0, 1
    LOAD_DIFF_HADAMARD_SUM mm1, 0, 0
    paddw       mm0, mm1
    SATD_END

;-----------------------------------------------------------------------------
;   int __cdecl x264_pixel_satd_8x4_mmxext (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
cglobal x264_pixel_satd_8x4_mmxext
    SATD_START
    LOAD_DIFF_HADAMARD_SUM mm0, 0, 0
    sub         eax, ebx
    sub         ecx, edx
    LOAD_DIFF_HADAMARD_SUM mm1, 4, 0
    paddw       mm0, mm1
    SATD_END

;-----------------------------------------------------------------------------
;   int __cdecl x264_pixel_satd_8x8_mmxext (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
cglobal x264_pixel_satd_8x8_mmxext
    SATD_START
    LOAD_DIFF_HADAMARD_SUM mm0, 0, 1
    LOAD_DIFF_HADAMARD_SUM mm1, 0, 0

    mov         eax, [esp+ 8]       ; pix1
    mov         ecx, [esp+16]       ; pix2
    LOAD_DIFF_HADAMARD_SUM mm2, 4, 1
    paddw       mm0, mm1
    LOAD_DIFF_HADAMARD_SUM mm1, 4, 0
    paddw       mm0, mm2
    paddw       mm0, mm1
    SATD_END

;-----------------------------------------------------------------------------
;   int __cdecl x264_pixel_satd_16x8_mmxext (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
cglobal x264_pixel_satd_16x8_mmxext
    SATD_START
    LOAD_DIFF_HADAMARD_SUM mm0, 0, 1
    LOAD_DIFF_HADAMARD_SUM mm1, 0, 0

    mov         eax, [esp+ 8]       ; pix1
    mov         ecx, [esp+16]       ; pix2
    LOAD_DIFF_HADAMARD_SUM mm2, 4, 1
    paddw       mm0, mm1
    LOAD_DIFF_HADAMARD_SUM mm1, 4, 0
    paddw       mm0, mm2

    mov         eax, [esp+ 8]       ; pix1
    mov         ecx, [esp+16]       ; pix2
    LOAD_DIFF_HADAMARD_SUM mm2, 8, 1
    paddw       mm0, mm1
    LOAD_DIFF_HADAMARD_SUM mm1, 8, 0
    paddw       mm0, mm2

    mov         eax, [esp+ 8]       ; pix1
    mov         ecx, [esp+16]       ; pix2
    LOAD_DIFF_HADAMARD_SUM mm2, 12, 1
    paddw       mm0, mm1
    LOAD_DIFF_HADAMARD_SUM mm1, 12, 0
    paddw       mm0, mm2
    paddw       mm0, mm1
    SATD_END

;-----------------------------------------------------------------------------
;   int __cdecl x264_pixel_satd_8x16_mmxext (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
cglobal x264_pixel_satd_8x16_mmxext
    SATD_START
    LOAD_DIFF_HADAMARD_SUM mm0, 0, 1
    LOAD_DIFF_HADAMARD_SUM mm1, 0, 1
    LOAD_DIFF_HADAMARD_SUM mm2, 0, 1
    paddw       mm0, mm1
    LOAD_DIFF_HADAMARD_SUM mm1, 0, 0
    paddw       mm0, mm2

    mov         eax, [esp+ 8]       ; pix1
    mov         ecx, [esp+16]       ; pix2
    LOAD_DIFF_HADAMARD_SUM mm2, 4, 1
    paddw       mm0, mm1
    LOAD_DIFF_HADAMARD_SUM mm1, 4, 1
    paddw       mm0, mm2
    LOAD_DIFF_HADAMARD_SUM mm2, 4, 1
    paddw       mm0, mm1
    LOAD_DIFF_HADAMARD_SUM mm1, 4, 0
    paddw       mm0, mm2
    paddw       mm0, mm1
    SATD_END

;-----------------------------------------------------------------------------
;   int __cdecl x264_pixel_satd_16x16_mmxext (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
cglobal x264_pixel_satd_16x16_mmxext
    SATD_START
    LOAD_DIFF_HADAMARD_SUM mm0, 0, 1
    LOAD_DIFF_HADAMARD_SUM mm1, 0, 1
    LOAD_DIFF_HADAMARD_SUM mm2, 0, 1
    paddw       mm0, mm1
    LOAD_DIFF_HADAMARD_SUM mm1, 0, 0
    paddw       mm0, mm2

    mov         eax, [esp+ 8]       ; pix1
    mov         ecx, [esp+16]       ; pix2
    LOAD_DIFF_HADAMARD_SUM mm2, 4, 1
    paddw       mm0, mm1
    LOAD_DIFF_HADAMARD_SUM mm1, 4, 1
    paddw       mm0, mm2
    LOAD_DIFF_HADAMARD_SUM mm2, 4, 1
    paddw       mm0, mm1
    LOAD_DIFF_HADAMARD_SUM mm1, 4, 0
    paddw       mm0, mm2

    mov         eax, [esp+ 8]       ; pix1
    mov         ecx, [esp+16]       ; pix2
    LOAD_DIFF_HADAMARD_SUM mm2, 8, 1
    paddw       mm0, mm1
    LOAD_DIFF_HADAMARD_SUM mm1, 8, 1
    paddw       mm0, mm2
    LOAD_DIFF_HADAMARD_SUM mm2, 8, 1
    paddw       mm0, mm1
    LOAD_DIFF_HADAMARD_SUM mm1, 8, 0
    paddw       mm0, mm2

    mov         eax, [esp+ 8]       ; pix1
    mov         ecx, [esp+16]       ; pix2
    LOAD_DIFF_HADAMARD_SUM mm2, 12, 1
    paddw       mm0, mm1
    LOAD_DIFF_HADAMARD_SUM mm1, 12, 1
    paddw       mm0, mm2
    LOAD_DIFF_HADAMARD_SUM mm2, 12, 1
    paddw       mm0, mm1
    LOAD_DIFF_HADAMARD_SUM mm1, 12, 0
    paddw       mm0, mm2
    paddw       mm0, mm1

    pxor        mm3, mm3
    pshufw      mm1, mm0, 01001110b
    paddw       mm0, mm1
    punpcklwd   mm0, mm3
    pshufw      mm1, mm0, 01001110b
    paddd       mm0, mm1
    movd        eax, mm0
    pop         ebx
    ret


%macro LOAD_DIFF_4x8P 1 ; dx
    LOAD_DIFF_4P  mm0, mm7, %1, 0
    LOAD_DIFF_4P  mm1, mm7, %1, 1
    lea  eax, [eax+2*ebx]
    lea  ecx, [ecx+2*edx]
    LOAD_DIFF_4P  mm2, mm7, %1, 0
    LOAD_DIFF_4P  mm3, mm7, %1, 1
    lea  eax, [eax+2*ebx]
    lea  ecx, [ecx+2*edx]
    LOAD_DIFF_4P  mm4, mm7, %1, 0
    LOAD_DIFF_4P  mm5, mm7, %1, 1
    lea  eax, [eax+2*ebx]
    lea  ecx, [ecx+2*edx]
    LOAD_DIFF_4P  mm6, mm7, %1, 0
    movq [spill], mm6
    LOAD_DIFF_4P  mm7, mm6, %1, 1
    movq mm6, [spill]
%endmacro

%macro HADAMARD1x8 8
    SUMSUB_BADC %1, %5, %2, %6
    SUMSUB_BADC %3, %7, %4, %8
    SUMSUB_BADC %1, %3, %2, %4
    SUMSUB_BADC %5, %7, %6, %8
    SUMSUB_BADC %1, %2, %3, %4
    SUMSUB_BADC %5, %6, %7, %8
%endmacro

%macro SUM4x8_MM 0
    movq [spill],   mm6
    movq [spill+8], mm7
    MMX_ABS_TWO  mm0, mm1, mm6, mm7
    MMX_ABS_TWO  mm2, mm3, mm6, mm7
    paddw    mm0, mm2
    paddw    mm1, mm3
    movq     mm6, [spill]
    movq     mm7, [spill+8]
    MMX_ABS_TWO  mm4, mm5, mm2, mm3
    MMX_ABS_TWO  mm6, mm7, mm2, mm3
    paddw    mm4, mm6
    paddw    mm5, mm7
    paddw    mm0, mm4
    paddw    mm1, mm5
    paddw    mm0, mm1
%endmacro

;-----------------------------------------------------------------------------
;   int __cdecl x264_pixel_sa8d_8x8_mmxext( uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
cglobal x264_pixel_sa8d_8x8_mmxext
    SATD_START
    sub    esp, 0x70
%define args  esp+0x74
%define spill esp+0x60 ; +16
%define trans esp+0    ; +96
    LOAD_DIFF_4x8P 0
    HADAMARD1x8 mm0, mm1, mm2, mm3, mm4, mm5, mm6, mm7

    movq   [spill], mm0
    TRANSPOSE4x4 mm4, mm5, mm6, mm7, mm0
    movq   [trans+0x00], mm4
    movq   [trans+0x08], mm7
    movq   [trans+0x10], mm0
    movq   [trans+0x18], mm6
    movq   mm0, [spill]
    TRANSPOSE4x4 mm0, mm1, mm2, mm3, mm4
    movq   [trans+0x20], mm0
    movq   [trans+0x28], mm3
    movq   [trans+0x30], mm4
    movq   [trans+0x38], mm2

    mov    eax, [args+4]
    mov    ecx, [args+12]
    LOAD_DIFF_4x8P 4
    HADAMARD1x8 mm0, mm1, mm2, mm3, mm4, mm5, mm6, mm7

    movq   [spill], mm7
    TRANSPOSE4x4 mm0, mm1, mm2, mm3, mm7
    movq   [trans+0x40], mm0
    movq   [trans+0x48], mm3
    movq   [trans+0x50], mm7
    movq   [trans+0x58], mm2
    movq   mm7, [spill]
    TRANSPOSE4x4 mm4, mm5, mm6, mm7, mm0
    movq   mm5, [trans+0x00]
    movq   mm1, [trans+0x08]
    movq   mm2, [trans+0x10]
    movq   mm3, [trans+0x18]

    HADAMARD1x8 mm5, mm1, mm2, mm3, mm4, mm7, mm0, mm6
    SUM4x8_MM
    movq   [trans], mm0

    movq   mm0, [trans+0x20]
    movq   mm1, [trans+0x28]
    movq   mm2, [trans+0x30]
    movq   mm3, [trans+0x38]
    movq   mm4, [trans+0x40]
    movq   mm5, [trans+0x48]
    movq   mm6, [trans+0x50]
    movq   mm7, [trans+0x58]
    
    HADAMARD1x8 mm0, mm1, mm2, mm3, mm4, mm5, mm6, mm7
    SUM4x8_MM

    pavgw  mm0, [esp]
    pshufw mm1, mm0, 01001110b
    paddw  mm0, mm1
    pshufw mm1, mm0, 10110001b
    paddw  mm0, mm1
    movd   eax, mm0
    and    eax, 0xffff
    mov    ecx, eax ; preserve rounding for 16x16
    add    eax, 1
    shr    eax, 1
    add    esp, 0x70
    pop    ebx
    ret
%undef args
%undef spill
%undef trans

;-----------------------------------------------------------------------------
;   int __cdecl x264_pixel_sa8d_16x16_mmxext( uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
;; violates calling convention
cglobal x264_pixel_sa8d_16x16_mmxext
    push   esi
    push   edi
    push   ebp
    mov    esi, [esp+28]    ; stride2
    mov    edi, [esp+20]    ; stride1
    push   esi
    push   dword [esp+28]   ; pix2
    push   edi
    push   dword [esp+28]   ; pix1
    call x264_pixel_sa8d_8x8_mmxext
    mov    ebp, ecx
    shl    edi, 3
    shl    esi, 3
    add    [esp+0], edi     ; pix1+8*stride1
    add    [esp+8], esi     ; pix2+8*stride2
    call x264_pixel_sa8d_8x8_mmxext
    add    ebp, ecx
    add    dword [esp+0], 8 ; pix1+8*stride1+8
    add    dword [esp+8], 8 ; pix2+8*stride2+8
    call x264_pixel_sa8d_8x8_mmxext
    add    ebp, ecx
    sub    [esp+0], edi     ; pix1+8
    sub    [esp+8], esi     ; pix2+8
    call x264_pixel_sa8d_8x8_mmxext
    lea    eax, [ebp+ecx+1]
    shr    eax, 1
    add    esp, 16
    pop    ebp
    pop    edi
    pop    esi
    ret


; in: fenc
; out: mm0..mm3 = hadamard coefs
%macro LOAD_HADAMARD 1
    pxor        mm7, mm7
    movd        mm0, [%1+0*FENC_STRIDE]
    movd        mm4, [%1+1*FENC_STRIDE]
    movd        mm3, [%1+2*FENC_STRIDE]
    movd        mm1, [%1+3*FENC_STRIDE]
    punpcklbw   mm0, mm7
    punpcklbw   mm4, mm7
    punpcklbw   mm3, mm7
    punpcklbw   mm1, mm7
    HADAMARD4x4 mm0, mm4, mm3, mm1
    TRANSPOSE4x4 mm0, mm4, mm3, mm1, mm2
    HADAMARD4x4 mm0, mm1, mm2, mm3
%endmacro

%macro SCALAR_SUMSUB 4
    add %1, %2
    add %3, %4
    add %2, %2
    add %4, %4
    sub %2, %1
    sub %4, %3
%endmacro

%macro SUM_MM_X3 8 ; 3x sum, 4x tmp, op
    pxor        %7, %7
    pshufw      %4, %1, 01001110b
    pshufw      %5, %2, 01001110b
    pshufw      %6, %3, 01001110b
    paddusw     %1, %4
    paddusw     %2, %5
    paddusw     %3, %6
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

;-----------------------------------------------------------------------------
;  void x264_intra_satd_x3_4x4_mmxext( uint8_t *fenc, uint8_t *fdec, int *res )
;-----------------------------------------------------------------------------
cglobal x264_intra_satd_x3_4x4_mmxext
    push ebx
    push edi
    push esi
    sub  esp, 16
%define  args esp+32
%define  top_1d  esp+8 ; +8
%define  left_1d esp+0 ; +8

    mov  eax, [args+0] ; fenc
    LOAD_HADAMARD eax

    mov  edi, [args+4] ; fdec
    movzx       eax, byte [edi-1+0*FDEC_STRIDE]
    movzx       ebx, byte [edi-1+1*FDEC_STRIDE]
    movzx       ecx, byte [edi-1+2*FDEC_STRIDE]
    movzx       edx, byte [edi-1+3*FDEC_STRIDE]
    SCALAR_SUMSUB eax, ebx, ecx, edx
    SCALAR_SUMSUB eax, ecx, ebx, edx ; 1x4 hadamard
    mov         [left_1d+0], ax
    mov         [left_1d+2], bx
    mov         [left_1d+4], cx
    mov         [left_1d+6], dx
    mov         esi, eax ; dc

    movzx       eax, byte [edi-FDEC_STRIDE+0]
    movzx       ebx, byte [edi-FDEC_STRIDE+1]
    movzx       ecx, byte [edi-FDEC_STRIDE+2]
    movzx       edx, byte [edi-FDEC_STRIDE+3]
    SCALAR_SUMSUB eax, ebx, ecx, edx
    SCALAR_SUMSUB eax, ecx, ebx, edx ; 4x1 hadamard
    mov         [top_1d+0], ax
    mov         [top_1d+2], bx
    mov         [top_1d+4], cx
    mov         [top_1d+6], dx
    lea         esi, [esi + eax + 4] ; dc
    and         esi, -8
    shl         esi, 1

    movq        mm4, mm1
    movq        mm5, mm2
    MMX_ABS_TWO mm4, mm5, mm6, mm7
    movq        mm7, mm3
    paddw       mm4, mm5
    MMX_ABS     mm7, mm6
    paddw       mm7, mm4 ; 3x4 sum

    movq        mm4, [left_1d]
    movd        mm5, esi
    psllw       mm4, 2
    psubw       mm4, mm0
    psubw       mm5, mm0
    punpcklwd   mm0, mm1
    punpcklwd   mm2, mm3
    punpckldq   mm0, mm2 ; transpose
    movq        mm1, [top_1d]
    psllw       mm1, 2
    psubw       mm0, mm1
    MMX_ABS     mm4, mm3 ; 1x4 sum
    MMX_ABS     mm5, mm2 ; 1x4 sum
    MMX_ABS     mm0, mm1 ; 4x1 sum
    paddw       mm4, mm7
    paddw       mm5, mm7
    movq        mm1, mm5
    psrlq       mm1, 16  ; 4x3 sum
    paddw       mm0, mm1

    SUM_MM_X3   mm0, mm4, mm5, mm1, mm2, mm3, mm6, pavgw
    mov         eax, [args+8] ; res
    movd        [eax+0], mm0 ; i4x4_v satd
    movd        [eax+4], mm4 ; i4x4_h satd
    movd        [eax+8], mm5 ; i4x4_dc satd

    add  esp, 16
    pop  esi
    pop  edi
    pop  ebx
    ret

;-----------------------------------------------------------------------------
;  void x264_intra_satd_x3_16x16_mmxext( uint8_t *fenc, uint8_t *fdec, int *res )
;-----------------------------------------------------------------------------
cglobal x264_intra_satd_x3_16x16_mmxext
    push ebx
    push ebp
    push edi
    push esi
    sub  esp, 88
%define  args    esp+108
%define  sums    esp+64 ; +24
%define  top_1d  esp+32 ; +32
%define  left_1d esp+0  ; +32

    pxor        mm0, mm0
    movq        [sums+0], mm0
    movq        [sums+8], mm0
    movq        [sums+16], mm0

    ; 1D hadamards
    mov         edi, [args+4] ; fdec
    xor         ebp, ebp
    mov         esi, 12
.loop_edge:
    ; left
    shl         esi, 5 ; log(FDEC_STRIDE)
    movzx       eax, byte [edi+esi-1+0*FDEC_STRIDE]
    movzx       ebx, byte [edi+esi-1+1*FDEC_STRIDE]
    movzx       ecx, byte [edi+esi-1+2*FDEC_STRIDE]
    movzx       edx, byte [edi+esi-1+3*FDEC_STRIDE]
    shr         esi, 5
    SCALAR_SUMSUB eax, ebx, ecx, edx
    SCALAR_SUMSUB eax, ecx, ebx, edx
    add         ebp, eax
    mov         [left_1d+2*esi+0], ax
    mov         [left_1d+2*esi+2], bx
    mov         [left_1d+2*esi+4], cx
    mov         [left_1d+2*esi+6], dx

    ; top
    movzx       eax, byte [edi+esi-FDEC_STRIDE+0]
    movzx       ebx, byte [edi+esi-FDEC_STRIDE+1]
    movzx       ecx, byte [edi+esi-FDEC_STRIDE+2]
    movzx       edx, byte [edi+esi-FDEC_STRIDE+3]
    SCALAR_SUMSUB eax, ebx, ecx, edx
    SCALAR_SUMSUB eax, ecx, ebx, edx
    add         ebp, eax
    mov         [top_1d+2*esi+0], ax
    mov         [top_1d+2*esi+2], bx
    mov         [top_1d+2*esi+4], cx
    mov         [top_1d+2*esi+6], dx
    sub         esi, 4
    jge .loop_edge

    ; dc
    shr         ebp, 1
    add         ebp, 8
    and         ebp, -16

    ; 2D hadamards
    mov         eax, [args+0] ; fenc
    xor         edi, edi
.loop_y:
    xor         esi, esi
.loop_x:
    LOAD_HADAMARD eax

    movq        mm4, mm1
    movq        mm5, mm2
    MMX_ABS_TWO mm4, mm5, mm6, mm7
    movq        mm7, mm3
    paddw       mm4, mm5
    MMX_ABS     mm7, mm6
    paddw       mm7, mm4 ; 3x4 sum

    movq        mm4, [left_1d+8*edi]
    movd        mm5, ebp
    psllw       mm4, 2
    psubw       mm4, mm0
    psubw       mm5, mm0
    punpcklwd   mm0, mm1
    punpcklwd   mm2, mm3
    punpckldq   mm0, mm2 ; transpose
    movq        mm1, [top_1d+8*esi]
    psllw       mm1, 2
    psubw       mm0, mm1
    MMX_ABS     mm4, mm3 ; 1x4 sum
    MMX_ABS     mm5, mm2 ; 1x4 sum
    MMX_ABS     mm0, mm1 ; 4x1 sum
    pavgw       mm4, mm7
    pavgw       mm5, mm7
    paddw       mm0, [sums+0]  ; i4x4_v satd
    paddw       mm4, [sums+8]  ; i4x4_h satd
    paddw       mm5, [sums+16] ; i4x4_dc satd
    movq        [sums+0], mm0
    movq        [sums+8], mm4
    movq        [sums+16], mm5

    add         eax, 4
    inc         esi
    cmp         esi, 4
    jl  .loop_x
    add         eax, 4*FENC_STRIDE-16
    inc         edi
    cmp         edi, 4
    jl  .loop_y

; horizontal sum
    movq        mm2, [sums+16]
    movq        mm0, [sums+0]
    movq        mm1, [sums+8]
    movq        mm7, mm2
    SUM_MM_X3   mm0, mm1, mm2, mm3, mm4, mm5, mm6, paddd
    psrld       mm0, 1
    pslld       mm7, 16
    psrld       mm7, 16
    paddd       mm0, mm2
    psubd       mm0, mm7
    mov         eax, [args+8] ; res
    movd        [eax+0], mm0 ; i16x16_v satd
    movd        [eax+4], mm1 ; i16x16_h satd
    movd        [eax+8], mm2 ; i16x16_dc satd

    add  esp, 88
    pop  esi
    pop  edi
    pop  ebp
    pop  ebx
    ret

;-----------------------------------------------------------------------------
;  void x264_intra_satd_x3_8x8c_mmxext( uint8_t *fenc, uint8_t *fdec, int *res )
;-----------------------------------------------------------------------------
cglobal x264_intra_satd_x3_8x8c_mmxext
    push ebx
    push ebp
    push edi
    push esi
    sub  esp, 72
%define  args    esp+92
%define  sums    esp+48 ; +24
%define  dc_1d   esp+32 ; +16
%define  top_1d  esp+16 ; +16
%define  left_1d esp+0  ; +16

    pxor        mm0, mm0
    movq        [sums+0], mm0
    movq        [sums+8], mm0
    movq        [sums+16], mm0

    ; 1D hadamards
    mov         edi, [args+4] ; fdec
    xor         ebp, ebp
    mov         esi, 12
.loop_edge:
    ; left
    shl         esi, 5 ; log(FDEC_STRIDE)
    movzx       eax, byte [edi+esi-1+0*FDEC_STRIDE]
    movzx       ebx, byte [edi+esi-1+1*FDEC_STRIDE]
    movzx       ecx, byte [edi+esi-1+2*FDEC_STRIDE]
    movzx       edx, byte [edi+esi-1+3*FDEC_STRIDE]
    shr         esi, 5
    SCALAR_SUMSUB eax, ebx, ecx, edx
    SCALAR_SUMSUB eax, ecx, ebx, edx
    mov         [left_1d+2*esi+0], ax
    mov         [left_1d+2*esi+2], bx
    mov         [left_1d+2*esi+4], cx
    mov         [left_1d+2*esi+6], dx

    ; top
    movzx       eax, byte [edi+esi-FDEC_STRIDE+0]
    movzx       ebx, byte [edi+esi-FDEC_STRIDE+1]
    movzx       ecx, byte [edi+esi-FDEC_STRIDE+2]
    movzx       edx, byte [edi+esi-FDEC_STRIDE+3]
    SCALAR_SUMSUB eax, ebx, ecx, edx
    SCALAR_SUMSUB eax, ecx, ebx, edx
    mov         [top_1d+2*esi+0], ax
    mov         [top_1d+2*esi+2], bx
    mov         [top_1d+2*esi+4], cx
    mov         [top_1d+2*esi+6], dx
    sub         esi, 4
    jge .loop_edge

    ; dc
    movzx       eax, word [left_1d+0]
    movzx       ebx, word [top_1d+0]
    movzx       ecx, word [left_1d+8]
    movzx       edx, word [top_1d+8]
    add         eax, ebx
    lea         ebx, [ecx + edx]
    lea         eax, [2*eax + 8]
    lea         ebx, [2*ebx + 8]
    lea         ecx, [4*ecx + 8]
    lea         edx, [4*edx + 8]
    and         eax, -16
    and         ebx, -16
    and         ecx, -16
    and         edx, -16
    mov         [dc_1d+ 0], eax ; tl
    mov         [dc_1d+ 4], edx ; tr
    mov         [dc_1d+ 8], ecx ; bl
    mov         [dc_1d+12], ebx ; br
    lea         ebp, [dc_1d]

    ; 2D hadamards
    mov         eax, [args+0] ; fenc
    xor         edi, edi
.loop_y:
    xor         esi, esi
.loop_x:
    LOAD_HADAMARD eax

    movq        mm4, mm1
    movq        mm5, mm2
    MMX_ABS_TWO mm4, mm5, mm6, mm7
    movq        mm7, mm3
    paddw       mm4, mm5
    MMX_ABS     mm7, mm6
    paddw       mm7, mm4 ; 3x4 sum

    movq        mm4, [left_1d+8*edi]
    movd        mm5, [ebp]
    psllw       mm4, 2
    psubw       mm4, mm0
    psubw       mm5, mm0
    punpcklwd   mm0, mm1
    punpcklwd   mm2, mm3
    punpckldq   mm0, mm2 ; transpose
    movq        mm1, [top_1d+8*esi]
    psllw       mm1, 2
    psubw       mm0, mm1
    MMX_ABS     mm4, mm3 ; 1x4 sum
    MMX_ABS     mm5, mm2 ; 1x4 sum
    MMX_ABS     mm0, mm1 ; 4x1 sum
    pavgw       mm4, mm7
    pavgw       mm5, mm7
    paddw       mm0, [sums+16] ; i4x4_v satd
    paddw       mm4, [sums+8]  ; i4x4_h satd
    paddw       mm5, [sums+0]  ; i4x4_dc satd
    movq        [sums+16], mm0
    movq        [sums+8], mm4
    movq        [sums+0], mm5

    add         eax, 4
    add         ebp, 4
    inc         esi
    cmp         esi, 2
    jl  .loop_x
    add         eax, 4*FENC_STRIDE-8
    inc         edi
    cmp         edi, 2
    jl  .loop_y

; horizontal sum
    movq        mm0, [sums+0]
    movq        mm1, [sums+8]
    movq        mm2, [sums+16]
    movq        mm6, mm0
    psrlq       mm6, 15
    paddw       mm2, mm6
    SUM_MM_X3   mm0, mm1, mm2, mm3, mm4, mm5, mm7, paddd
    psrld       mm2, 1
    mov         eax, [args+8] ; res
    movd        [eax+0], mm0 ; i8x8c_dc satd
    movd        [eax+4], mm1 ; i8x8c_h satd
    movd        [eax+8], mm2 ; i8x8c_v satd

    add  esp, 72
    pop  esi
    pop  edi
    pop  ebp
    pop  ebx
    ret

%macro LOAD_4x8P 1 ; dx
    pxor        mm7, mm7
    movd        mm6, [eax+%1+7*FENC_STRIDE]
    movd        mm0, [eax+%1+0*FENC_STRIDE]
    movd        mm1, [eax+%1+1*FENC_STRIDE]
    movd        mm2, [eax+%1+2*FENC_STRIDE]
    movd        mm3, [eax+%1+3*FENC_STRIDE]
    movd        mm4, [eax+%1+4*FENC_STRIDE]
    movd        mm5, [eax+%1+5*FENC_STRIDE]
    punpcklbw   mm6, mm7
    punpcklbw   mm0, mm7
    punpcklbw   mm1, mm7
    movq    [spill], mm6
    punpcklbw   mm2, mm7
    punpcklbw   mm3, mm7
    movd        mm6, [eax+%1+6*FENC_STRIDE]
    punpcklbw   mm4, mm7
    punpcklbw   mm5, mm7
    punpcklbw   mm6, mm7
    movq        mm7, [spill]
%endmacro

;-----------------------------------------------------------------------------
;  void x264_intra_sa8d_x3_8x8_core_mmxext( uint8_t *fenc, int16_t edges[2][8], int *res )
;-----------------------------------------------------------------------------
cglobal x264_intra_sa8d_x3_8x8_core_mmxext
    mov    eax, [esp+4]
    mov    ecx, [esp+8]
    sub    esp, 0x70
%define args  esp+0x74
%define spill esp+0x60 ; +16
%define trans esp+0    ; +96
%define sum   esp+0    ; +32
    LOAD_4x8P 0
    HADAMARD1x8 mm0, mm1, mm2, mm3, mm4, mm5, mm6, mm7

    movq   [spill], mm0
    TRANSPOSE4x4 mm4, mm5, mm6, mm7, mm0
    movq   [trans+0x00], mm4
    movq   [trans+0x08], mm7
    movq   [trans+0x10], mm0
    movq   [trans+0x18], mm6
    movq   mm0, [spill]
    TRANSPOSE4x4 mm0, mm1, mm2, mm3, mm4
    movq   [trans+0x20], mm0
    movq   [trans+0x28], mm3
    movq   [trans+0x30], mm4
    movq   [trans+0x38], mm2

    LOAD_4x8P 4
    HADAMARD1x8 mm0, mm1, mm2, mm3, mm4, mm5, mm6, mm7

    movq   [spill], mm7
    TRANSPOSE4x4 mm0, mm1, mm2, mm3, mm7
    movq   [trans+0x40], mm0
    movq   [trans+0x48], mm3
    movq   [trans+0x50], mm7
    movq   [trans+0x58], mm2
    movq   mm7, [spill]
    TRANSPOSE4x4 mm4, mm5, mm6, mm7, mm0
    movq   mm5, [trans+0x00]
    movq   mm1, [trans+0x08]
    movq   mm2, [trans+0x10]
    movq   mm3, [trans+0x18]

    HADAMARD1x8 mm5, mm1, mm2, mm3, mm4, mm7, mm0, mm6

    movq [spill+0], mm5
    movq [spill+8], mm7
    MMX_ABS_TWO  mm0, mm1, mm5, mm7
    MMX_ABS_TWO  mm2, mm3, mm5, mm7
    paddw    mm0, mm2
    paddw    mm1, mm3
    paddw    mm0, mm1
    MMX_ABS_TWO  mm4, mm6, mm2, mm3
    movq     mm5, [spill+0]
    movq     mm7, [spill+8]
    paddw    mm0, mm4
    paddw    mm0, mm6
    MMX_ABS  mm7, mm1
    paddw    mm0, mm7 ; 7x4 sum
    movq     mm6, mm5
    movq     mm7, [ecx+8] ; left bottom
    psllw    mm7, 3
    psubw    mm6, mm7
    MMX_ABS_TWO  mm5, mm6, mm2, mm3
    paddw    mm5, mm0
    paddw    mm6, mm0
    movq [sum+0], mm5 ; dc
    movq [sum+8], mm6 ; left

    movq   mm0, [trans+0x20]
    movq   mm1, [trans+0x28]
    movq   mm2, [trans+0x30]
    movq   mm3, [trans+0x38]
    movq   mm4, [trans+0x40]
    movq   mm5, [trans+0x48]
    movq   mm6, [trans+0x50]
    movq   mm7, [trans+0x58]
    
    HADAMARD1x8 mm0, mm1, mm2, mm3, mm4, mm5, mm6, mm7

    movd   [sum+0x10], mm0
    movd   [sum+0x12], mm1
    movd   [sum+0x14], mm2
    movd   [sum+0x16], mm3
    movd   [sum+0x18], mm4
    movd   [sum+0x1a], mm5
    movd   [sum+0x1c], mm6
    movd   [sum+0x1e], mm7

    movq [spill],   mm0
    movq [spill+8], mm1
    MMX_ABS_TWO  mm2, mm3, mm0, mm1
    MMX_ABS_TWO  mm4, mm5, mm0, mm1
    paddw    mm2, mm3
    paddw    mm4, mm5
    paddw    mm2, mm4
    movq     mm0, [spill]
    movq     mm1, [spill+8]
    MMX_ABS_TWO  mm6, mm7, mm4, mm5
    MMX_ABS  mm1, mm4
    paddw    mm2, mm7
    paddw    mm1, mm6
    paddw    mm2, mm1 ; 7x4 sum
    movq     mm1, mm0

    movq     mm7, [ecx+0]
    psllw    mm7, 3   ; left top

    movzx    edx, word [ecx+0]
    add      dx,  [ecx+16]
    lea      edx, [4*edx+32]
    and      edx, -64
    movd     mm6, edx ; dc

    psubw    mm1, mm7
    psubw    mm0, mm6
    MMX_ABS_TWO  mm0, mm1, mm5, mm6
    movq     mm3, [sum+0] ; dc
    paddw    mm0, mm2
    paddw    mm1, mm2
    movq     mm2, mm0
    paddw    mm0, mm3
    paddw    mm1, [sum+8] ; h
    psrlq    mm2, 16
    paddw    mm2, mm3

    movq     mm3, [ecx+16] ; top left
    movq     mm4, [ecx+24] ; top right
    psllw    mm3, 3
    psllw    mm4, 3
    psubw    mm3, [sum+16]
    psubw    mm4, [sum+24]
    MMX_ABS_TWO  mm3, mm4, mm5, mm6
    paddw    mm2, mm3
    paddw    mm2, mm4 ; v

    SUM_MM_X3   mm0, mm1, mm2, mm3, mm4, mm5, mm6, paddd
    mov      eax, [args+8]
    movd     ecx, mm2
    movd     edx, mm1
    add      ecx, 2
    add      edx, 2
    shr      ecx, 2
    shr      edx, 2
    mov      [eax+0], ecx ; i8x8_v satd
    mov      [eax+4], edx ; i8x8_h satd
    movd     ecx, mm0
    add      ecx, 2
    shr      ecx, 2
    mov      [eax+8], ecx ; i8x8_dc satd

    add      esp, 0x70
    ret
%undef args
%undef spill
%undef trans
%undef sum



;-----------------------------------------------------------------------------
; void x264_pixel_ssim_4x4x2_core_mmxext( const uint8_t *pix1, int stride1,
;                                         const uint8_t *pix2, int stride2, int sums[2][4] )
;-----------------------------------------------------------------------------
cglobal x264_pixel_ssim_4x4x2_core_mmxext
    push      ebx
    push      edi
    mov       ebx, [esp+16]
    mov       edx, [esp+24]
    mov       edi, 4
    pxor      mm0, mm0
.loop
    mov       eax, [esp+12]
    mov       ecx, [esp+20]
    add       eax, edi
    add       ecx, edi
    pxor      mm1, mm1
    pxor      mm2, mm2
    pxor      mm3, mm3
    pxor      mm4, mm4
%rep 4
    movd      mm5, [eax]
    movd      mm6, [ecx]
    punpcklbw mm5, mm0
    punpcklbw mm6, mm0
    paddw     mm1, mm5
    paddw     mm2, mm6
    movq      mm7, mm5
    pmaddwd   mm5, mm5
    pmaddwd   mm7, mm6
    pmaddwd   mm6, mm6
    paddd     mm3, mm5
    paddd     mm4, mm7
    paddd     mm3, mm6
    add       eax, ebx
    add       ecx, edx
%endrep
    mov       eax, [esp+28]
    lea       eax, [eax+edi*4]
    pshufw    mm5, mm1, 0xE
    pshufw    mm6, mm2, 0xE
    paddusw   mm1, mm5
    paddusw   mm2, mm6
    punpcklwd mm1, mm2
    pshufw    mm2, mm1, 0xE
    pshufw    mm5, mm3, 0xE
    pshufw    mm6, mm4, 0xE
    paddusw   mm1, mm2
    paddd     mm3, mm5
    paddd     mm4, mm6
    punpcklwd mm1, mm0
    punpckldq mm3, mm4
    movq  [eax+0], mm1
    movq  [eax+8], mm3
    sub       edi, 4
    jge       .loop
    pop       edi
    pop       ebx
    emms
    ret



; int x264_pixel_ads_mvs( int16_t *mvs, uint8_t *masks, int width )
cglobal x264_pixel_ads_mvs
    mov     ebx, [ebp+24] ; mvs
    mov     ecx, esp ; masks
    mov     edi, [ebp+28] ; width
    mov     dword [ecx+edi], 0
    push    esi
    push    ebp
    xor     eax, eax
    xor     esi, esi
.loopi:
    mov     ebp, [ecx+esi]
    mov     edx, [ecx+esi+4]
    or      edx, ebp
    jz .nexti
    xor     edx, edx
%macro TEST 1
    mov     [ebx+eax*2], si
    test    ebp, 0xff<<(%1*8)
    setne   dl
    add     eax, edx
    inc     esi
%endmacro
    TEST 0
    TEST 1
    TEST 2
    TEST 3
    mov     ebp, [ecx+esi]
    TEST 0
    TEST 1
    TEST 2
    TEST 3
    cmp     esi, edi
    jl .loopi
    jmp .end
.nexti:
    add     esi, 8
    cmp     esi, edi
    jl .loopi
.end:
    pop     ebp
    pop     esi
    mov     edi, [ebp-8]
    mov     ebx, [ebp-4]
    leave
    ret

%macro ADS_START 0 
    push    ebp
    mov     ebp, esp
    push    ebx
    push    edi
    mov     eax, [ebp+12] ; sums
    mov     ebx, [ebp+16] ; delta
    mov     ecx, [ebp+20] ; cost_mvx
    mov     edx, [ebp+28] ; width
    sub     esp, edx
    sub     esp, 4
    and     esp, ~15
    mov     edi, esp
    shl     ebx, 1
%endmacro   

%macro ADS_END 1
    add     eax, 8*%1
    add     ecx, 8*%1
    add     edi, 4*%1
    sub     edx, 4*%1
    jg .loop
    jmp x264_pixel_ads_mvs
%endmacro

;-----------------------------------------------------------------------------
; int x264_pixel_ads4_mmxext( int enc_dc[4], uint16_t *sums, int delta,
;                             uint16_t *cost_mvx, int16_t *mvs, int width, int thresh )
;-----------------------------------------------------------------------------
cglobal x264_pixel_ads4_mmxext
    mov     eax, [esp+4]
    movq    mm6, [eax]
    movq    mm4, [eax+8]
    pshufw  mm7, mm6, 0
    pshufw  mm6, mm6, 0xAA
    pshufw  mm5, mm4, 0
    pshufw  mm4, mm4, 0xAA
    ADS_START
.loop:
    movq    mm0, [eax]
    movq    mm1, [eax+16]
    psubw   mm0, mm7
    psubw   mm1, mm6
    MMX_ABS mm0, mm2
    MMX_ABS mm1, mm3
    movq    mm2, [eax+ebx]
    movq    mm3, [eax+ebx+16]
    psubw   mm2, mm5
    psubw   mm3, mm4
    paddw   mm0, mm1
    MMX_ABS mm2, mm1
    MMX_ABS mm3, mm1
    paddw   mm0, mm2
    paddw   mm0, mm3
    pshufw  mm1, [ebp+32], 0
    paddusw mm0, [ecx]
    psubusw mm1, mm0
    packsswb mm1, mm1
    movd    [edi], mm1
    ADS_END 1

cglobal x264_pixel_ads2_mmxext
    mov     eax, [esp+4]
    movq    mm6, [eax]
    pshufw  mm5, [esp+28], 0
    pshufw  mm7, mm6, 0
    pshufw  mm6, mm6, 0xAA
    ADS_START
.loop:
    movq    mm0, [eax]
    movq    mm1, [eax+ebx]
    psubw   mm0, mm7
    psubw   mm1, mm6
    MMX_ABS mm0, mm2
    MMX_ABS mm1, mm3
    paddw   mm0, mm1
    paddusw mm0, [ecx]
    movq    mm4, mm5
    psubusw mm4, mm0
    packsswb mm4, mm4
    movd    [edi], mm4
    ADS_END 1

cglobal x264_pixel_ads1_mmxext
    mov     eax, [esp+4]
    pshufw  mm7, [eax], 0
    pshufw  mm6, [esp+28], 0
    ADS_START
.loop:
    movq    mm0, [eax]
    movq    mm1, [eax+8]
    psubw   mm0, mm7
    psubw   mm1, mm7
    MMX_ABS mm0, mm2
    MMX_ABS mm1, mm3
    paddusw mm0, [ecx]
    paddusw mm1, [ecx+8]
    movq    mm4, mm6
    movq    mm5, mm6
    psubusw mm4, mm0
    psubusw mm5, mm1
    packsswb mm4, mm5
    movq    [edi], mm4
    ADS_END 2

%macro ADS_SSE2 1
cglobal x264_pixel_ads4_%1
    mov     eax, [esp+4] ; enc_dc
    movdqa  xmm4, [eax]
    pshuflw xmm7, xmm4, 0
    pshuflw xmm6, xmm4, 0xAA
    pshufhw xmm5, xmm4, 0
    pshufhw xmm4, xmm4, 0xAA
    punpcklqdq xmm7, xmm7
    punpcklqdq xmm6, xmm6
    punpckhqdq xmm5, xmm5
    punpckhqdq xmm4, xmm4
    ADS_START
.loop:
    movdqu  xmm0, [eax]
    movdqu  xmm1, [eax+16]
    psubw   xmm0, xmm7
    psubw   xmm1, xmm6
    MMX_ABS xmm0, xmm2
    MMX_ABS xmm1, xmm3
    movdqu  xmm2, [eax+ebx]
    movdqu  xmm3, [eax+ebx+16]
    psubw   xmm2, xmm5
    psubw   xmm3, xmm4
    paddw   xmm0, xmm1
    MMX_ABS xmm2, xmm1
    MMX_ABS xmm3, xmm1
    paddw   xmm0, xmm2
    paddw   xmm0, xmm3
    movd    xmm1, [ebp+32] ; thresh
    movdqu  xmm2, [ecx]
    pshuflw xmm1, xmm1, 0
    punpcklqdq xmm1, xmm1
    paddusw xmm0, xmm2
    psubusw xmm1, xmm0
    packsswb xmm1, xmm1
    movq    [edi], xmm1
    ADS_END 2

cglobal x264_pixel_ads2_%1
    mov     eax, [esp+4] ; enc_dc
    movq    xmm6, [eax]
    movd    xmm5, [esp+28] ; thresh
    pshuflw xmm7, xmm6, 0
    pshuflw xmm6, xmm6, 0xAA
    pshuflw xmm5, xmm5, 0
    punpcklqdq xmm7, xmm7
    punpcklqdq xmm6, xmm6
    punpcklqdq xmm5, xmm5
    ADS_START
.loop:
    movdqu  xmm0, [eax]
    movdqu  xmm1, [eax+ebx]
    psubw   xmm0, xmm7
    psubw   xmm1, xmm6
    movdqu  xmm4, [ecx]
    MMX_ABS xmm0, xmm2
    MMX_ABS xmm1, xmm3
    paddw   xmm0, xmm1
    paddusw xmm0, xmm4
    movdqa  xmm1, xmm5
    psubusw xmm1, xmm0
    packsswb xmm1, xmm1
    movq    [edi], xmm1
    ADS_END 2

cglobal x264_pixel_ads1_%1
    mov     eax, [esp+4] ; enc_dc
    movd    xmm7, [eax]
    movd    xmm6, [esp+28] ; thresh
    pshuflw xmm7, xmm7, 0
    pshuflw xmm6, xmm6, 0
    punpcklqdq xmm7, xmm7
    punpcklqdq xmm6, xmm6
    ADS_START
.loop:
    movdqu  xmm0, [eax]
    movdqu  xmm1, [eax+16]
    psubw   xmm0, xmm7
    psubw   xmm1, xmm7
    movdqu  xmm2, [ecx]
    movdqu  xmm3, [ecx+16]
    MMX_ABS xmm0, xmm4
    MMX_ABS xmm1, xmm5
    paddusw xmm0, xmm2
    paddusw xmm1, xmm3
    movdqa  xmm4, xmm6
    movdqa  xmm5, xmm6
    psubusw xmm4, xmm0
    psubusw xmm5, xmm1
    packsswb xmm4, xmm5
    movdqa  [edi], xmm4
    ADS_END 4
%endmacro

ADS_SSE2 sse2
%ifdef HAVE_SSE3
%macro MMX_ABS 2
    pabsw %1, %1
%endmacro
ADS_SSE2 ssse3
%endif

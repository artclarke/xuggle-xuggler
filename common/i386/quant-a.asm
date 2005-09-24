;*****************************************************************************
;* quant-a.asm: h264 encoder library
;*****************************************************************************
;* Copyright (C) 2005 x264 project
;*
;* Authors: Alex Izvorski <aizvorksi@gmail.com>
;*          Christian Heine <sennindemokrit@gmx.net>
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
;*  2005.07.26  quant 4x4 & 8x8 MMX functions (AI)                           *
;*  2005.09.04  quant MMXEXT (added precision) and DC (CH)                   *
;*  2005.09.21  faster MMX and added MMXEXT16 (CH)                           *
;*                                                                           *
;*****************************************************************************

BITS 32

%macro cglobal 1
    %ifdef PREFIX
        global _%1
        %define %1 _%1
    %else
        global %1
    %endif
%endmacro

ALIGN 16

SECTION .text

cglobal x264_quant_2x2_dc_core15_mmx
cglobal x264_quant_4x4_dc_core15_mmx
cglobal x264_quant_4x4_core15_mmx
cglobal x264_quant_8x8_core15_mmx

cglobal x264_quant_2x2_dc_core16_mmxext
cglobal x264_quant_4x4_dc_core16_mmxext
cglobal x264_quant_4x4_core16_mmxext
cglobal x264_quant_8x8_core16_mmxext

cglobal x264_quant_2x2_dc_core32_mmxext
cglobal x264_quant_4x4_dc_core32_mmxext
cglobal x264_quant_4x4_core32_mmxext
cglobal x264_quant_8x8_core32_mmxext

%macro MMX_QUANT_AC_START 0
    mov         eax, [esp+ 4]   ; &dct[0][0]
    mov         ecx, [esp+ 8]   ; &quant_mf[0][0]
    movd        mm6, [esp+12]   ; i_qbits
    movd        mm7, [esp+16]   ; f
    punpckldq   mm7, mm7        ; f in each dword
%endmacro

%macro MMX_QUANT15_DC_START 0
    mov         eax, [esp+ 4]   ; &dct[0][0]
    movd        mm5, [esp+ 8]   ; i_qmf
    movd        mm6, [esp+12]   ; i_qbits
    movd        mm7, [esp+16]   ; f
    punpcklwd   mm5, mm5
    punpcklwd   mm5, mm5        ; i_qmf in each word
    punpckldq   mm7, mm7        ; f in each dword
%endmacro

%macro MMX_QUANT15_1x4 4
;;; %1      (m64)       dct[y][x]
;;; %2      (m64/mmx)   quant_mf[y][x] or quant_mf[0][0] (as int16_t)
;;; %3      (mmx)       i_qbits in the low doubleword
;;; %4      (mmx)       f as doublewords
;;; trashes mm0-mm2,mm4
    movq        mm0, %1     ; load dct coeffs
    pxor        mm4, mm4
    pcmpgtw     mm4, mm0    ; sign(coeff)
    pxor        mm0, mm4
    psubw       mm0, mm4    ; abs(coeff)

    movq        mm2, mm0
    pmullw      mm0, %2
    pmulhw      mm2, %2

    movq        mm1, mm0
    punpcklwd   mm0, mm2
    punpckhwd   mm1, mm2

    paddd       mm0, %4     ; round with f
    paddd       mm1, %4
    psrad       mm0, %3
    psrad       mm1, %3

    packssdw    mm0, mm1    ; pack
    pxor        mm0, mm4    ; restore sign
    psubw       mm0, mm4
    movq        %1, mm0     ; store
%endmacro

ALIGN 16
;-----------------------------------------------------------------------------
;   void __cdecl x264_quant_2x2_dc_core15_mmx( int16_t dct[2][2],
;       int const i_qmf, int const i_qbits, int const f );
;-----------------------------------------------------------------------------
x264_quant_2x2_dc_core15_mmx:
    MMX_QUANT15_DC_START
    MMX_QUANT15_1x4 [eax], mm5, mm6, mm7
    ret

ALIGN 16
;-----------------------------------------------------------------------------
;   void __cdecl x264_quant_4x4_dc_core15_mmx( int16_t dct[4][4],
;       int const i_qmf, int const i_qbits, int const f );
;-----------------------------------------------------------------------------
x264_quant_4x4_dc_core15_mmx:
    MMX_QUANT15_DC_START

%rep 4
    MMX_QUANT15_1x4 [eax], mm5, mm6, mm7
    add         eax, byte 8
%endrep

    ret

ALIGN 16
;-----------------------------------------------------------------------------
;   void __cdecl x264_quant_4x4_core15_mmx( int16_t dct[4][4],
;       int const quant_mf[4][4], int const i_qbits, int const f );
;-----------------------------------------------------------------------------
x264_quant_4x4_core15_mmx:
    MMX_QUANT_AC_START

%rep 4
    movq        mm5, [ecx]
    packssdw    mm5, [ecx+8]
    MMX_QUANT15_1x4 [eax], mm5, mm6, mm7
    add         ecx, byte 16
    add         eax, byte 8
%endrep

    ret

ALIGN 16
;-----------------------------------------------------------------------------
;   void __cdecl x264_quant_8x8_core15_mmx( int16_t dct[8][8],
;       int const quant_mf[8][8], int const i_qbits, int const f );
;-----------------------------------------------------------------------------
x264_quant_8x8_core15_mmx:
    MMX_QUANT_AC_START

%rep 16
    movq        mm5, [ecx]
    packssdw    mm5, [ecx+8]
    MMX_QUANT15_1x4 [eax], mm5, mm6, mm7
    add         ecx, byte 16
    add         eax, byte 8
%endrep

    ret

; ============================================================================

%macro MMXEXT_QUANT16_DC_START 0
    mov         eax, [esp+ 4]   ; &dct[0][0]
    movd        mm5, [esp+ 8]   ; i_qmf
    movd        mm6, [esp+12]   ; i_qbits
    movd        mm7, [esp+16]   ; f
    pshufw      mm5, mm5, 0     ; i_qmf in each word
    punpckldq   mm7, mm7        ; f in each dword
%endmacro

%macro MMXEXT_QUANT16_1x4 4
;;; %1      (m64)       dct[y][x]
;;; %2      (m64/mmx)   quant_mf[y][x] or quant_mf[0][0] (as uint16_t)
;;; %3      (mmx)       i_qbits in the low doubleword
;;; %4      (mmx)       f as doublewords
;;; trashes mm0-mm2,mm4
    movq        mm0, %1     ; load dct coeffs
    pxor        mm4, mm4
    pcmpgtw     mm4, mm0    ; sign(coeff)
    pxor        mm0, mm4
    psubw       mm0, mm4    ; abs(coeff)

    movq        mm2, mm0
    pmullw      mm0, %2
    pmulhuw     mm2, %2

    movq        mm1, mm0
    punpcklwd   mm0, mm2
    punpckhwd   mm1, mm2

    paddd       mm0, %4     ; round with f
    paddd       mm1, %4
    psrad       mm0, %3
    psrad       mm1, %3

    packssdw    mm0, mm1    ; pack
    pxor        mm0, mm4    ; restore sign
    psubw       mm0, mm4
    movq        %1, mm0     ; store
%endmacro

ALIGN 16
;-----------------------------------------------------------------------------
;   void __cdecl x264_quant_2x2_dc_core16_mmxext( int16_t dct[2][2],
;       int const i_qmf, int const i_qbits, int const f );
;-----------------------------------------------------------------------------
x264_quant_2x2_dc_core16_mmxext:
    MMXEXT_QUANT16_DC_START
    MMXEXT_QUANT16_1x4 [eax], mm5, mm6, mm7
    ret

ALIGN 16
;-----------------------------------------------------------------------------
;   void __cdecl x264_quant_4x4_dc_core16_mmxext( int16_t dct[4][4],
;       int const i_qmf, int const i_qbits, int const f );
;-----------------------------------------------------------------------------
x264_quant_4x4_dc_core16_mmxext:
    MMXEXT_QUANT16_DC_START

%rep 4
    MMXEXT_QUANT16_1x4 [eax], mm5, mm6, mm7
    add         eax, byte 8
%endrep

    ret

ALIGN 16
;-----------------------------------------------------------------------------
;   void __cdecl x264_quant_4x4_core16_mmxext( int16_t dct[4][4],
;       int const quant_mf[4][4], int const i_qbits, int const f );
;-----------------------------------------------------------------------------
x264_quant_4x4_core16_mmxext:
    MMX_QUANT_AC_START

%rep 4
    pshufw      mm5, [ecx], 10110001b
    paddw       mm5, [ecx+8]
    pshufw      mm5, mm5, 10001101b
    MMXEXT_QUANT16_1x4 [eax], mm5, mm6, mm7
    add         ecx, byte 16
    add         eax, byte 8
%endrep

    ret

ALIGN 16
;-----------------------------------------------------------------------------
;   void __cdecl x264_quant_8x8_core16_mmxext( int16_t dct[8][8],
;       int const quant_mf[8][8], int const i_qbits, int const f );
;-----------------------------------------------------------------------------
x264_quant_8x8_core16_mmxext:
    MMX_QUANT_AC_START

%rep 16
    pshufw      mm5, [ecx], 10110001b
    paddw       mm5, [ecx+8]
    pshufw      mm5, mm5, 10001101b
    MMXEXT_QUANT16_1x4 [eax], mm5, mm6, mm7
    add         ecx, byte 16
    add         eax, byte 8
%endrep

    ret



%macro MMX_QUANT32_DC_START 0
    mov         eax, [esp+ 4]   ; &dct[0][0]
    movd        mm5, [esp+ 8]   ; i_qmf
    movd        mm6, [esp+12]   ; i_qbits
    movd        mm7, [esp+16]   ; f
    punpckldq   mm5, mm5        ; i_qmf in each dword
    punpckldq   mm7, mm7        ; f in each dword
%endmacro

%macro MMXEXT_QUANT32_1x4 5
;;; %1      (m64)       dct[y][x]
;;; %2,%3   (m64/mmx)   quant_mf[y][x] or quant_mf[0][0] (as int16_t)
;;; %4      (mmx)       i_qbits in the low quadword
;;; %5      (mmx)       f as doublewords
;;; trashes mm0-mm4
    movq        mm0, %1     ; load dct coeffs
    pxor        mm4, mm4
    pcmpgtw     mm4, mm0    ; sign(mm0)
    pxor        mm0, mm4
    psubw       mm0, mm4    ; abs(mm0)
    movq        mm1, mm0
    punpcklwd   mm0, mm0    ; duplicate the words for the upcomming
    punpckhwd   mm1, mm1    ; 32 bit multiplication

    movq        mm2, mm0    ; like in school ...
    movq        mm3, mm1
    pmulhuw     mm0, %2     ; ... multiply the parts ...
    pmulhuw     mm1, %3
    pmullw      mm2, %2
    pmullw      mm3, %3
    pslld       mm0, 16     ; ... shift ...
    pslld       mm1, 16
    paddd       mm0, mm2    ; ... and add them
    paddd       mm1, mm3

    paddd       mm0, %5     ; round with f
    paddd       mm1, %5
    psrad       mm0, %4
    psrad       mm1, %4

    packssdw    mm0, mm1    ; pack to int16_t
    pxor        mm0, mm4    ; restore sign
    psubw       mm0, mm4
    movq        %1, mm0     ; store
%endmacro

ALIGN 16
;-----------------------------------------------------------------------------
;   void __cdecl x264_quant_2x2_dc_core32_mmxext( int16_t dct[2][2],
;       int const i_qmf, int const i_qbits, int const f );
;-----------------------------------------------------------------------------
x264_quant_2x2_dc_core32_mmxext:
    MMX_QUANT32_DC_START
    MMXEXT_QUANT32_1x4 [eax], mm5, mm5, mm6, mm7
    ret

ALIGN 16
;-----------------------------------------------------------------------------
;   void __cdecl x264_quant_4x4_dc_core32_mmxext( int16_t dct[4][4],
;       int const i_qmf, int const i_qbits, int const f );
;-----------------------------------------------------------------------------
x264_quant_4x4_dc_core32_mmxext:
    MMX_QUANT32_DC_START

%rep 4
    MMXEXT_QUANT32_1x4 [eax], mm5, mm5, mm6, mm7
    add         eax, byte 8
%endrep

    ret

ALIGN 16
;-----------------------------------------------------------------------------
;   void __cdecl x264_quant_4x4_core32_mmxext( int16_t dct[4][4],
;       int const quant_mf[4][4], int const i_qbits, int const f );
;-----------------------------------------------------------------------------
x264_quant_4x4_core32_mmxext:
    MMX_QUANT_AC_START

%rep 4
    MMXEXT_QUANT32_1x4 [eax], [ecx], [ecx+8], mm6, mm7
    add         eax, byte 8
    add         ecx, byte 16
%endrep

    ret

ALIGN 16
;-----------------------------------------------------------------------------
;   void __cdecl x264_quant_8x8_core32_mmxext( int16_t dct[8][8],
;       int const quant_mf[8][8], int const i_qbits, int const f );
;-----------------------------------------------------------------------------
x264_quant_8x8_core32_mmxext:
    MMX_QUANT_AC_START

%rep 16
    MMXEXT_QUANT32_1x4 [eax], [ecx], [ecx+8], mm6, mm7
    add         eax, byte 8
    add         ecx, byte 16
%endrep

    ret


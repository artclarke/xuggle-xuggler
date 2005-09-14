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

cglobal x264_quant_8x8_core16_mmx
cglobal x264_quant_4x4_core16_mmx
cglobal x264_quant_8x8_core32_mmx
cglobal x264_quant_4x4_core32_mmx
cglobal x264_quant_4x4_dc_core32_mmx
cglobal x264_quant_2x2_dc_core32_mmx

%macro QUANT_AC_START 0
    mov       eax, [esp+ 4]   ; dct
    mov       ecx, [esp+ 8]   ; quant_mf
    movd      mm6, [esp+12]   ; i_qbits
    movd      mm7, [esp+16]   ; f
    punpckldq mm7, mm7
%endmacro

%macro QUANT_DC_START 0
    mov       eax, [esp+ 4]   ; dct
    movd      mm5, [esp+ 8]   ; i_quant_mf
    movd      mm6, [esp+12]   ; i_qbits
    movd      mm7, [esp+16]   ; f
    punpckldq mm5, mm5
    punpckldq mm7, mm7
%endmacro

%macro QUANT16_1x4 5
;;; %1      dct[y][x]
;;; %2,%3   quant_mf[i_mf][y][x], entries must fit in int16
;;; %4      i_qbits
;;; %5      f as doublewords
;;; trashes mm0-mm5
    movq     mm0, %1
    movq     mm1, %2
    movq     mm2, %3
    packssdw mm1, mm2

    movq     mm4, mm0
    pxor     mm5, mm5
    pcmpgtw  mm4, mm5

    movq     mm2, mm0
    pmullw   mm0, mm1
    pmulhw   mm2, mm1

    movq      mm1, mm0
    punpcklwd mm0, mm2
    punpckhwd mm1, mm2

    movq     mm2, %5
    movq     mm3, %5
    psubd    mm2, mm0
    psubd    mm3, mm1
    paddd    mm0, %5
    paddd    mm1, %5

    psrad    mm0, %4
    psrad    mm1, %4
    psrad    mm2, %4
    psrad    mm3, %4

    packssdw mm0, mm1
    packssdw mm2, mm3
    pxor     mm5, mm5
    psubw    mm5, mm2

    pand     mm0, mm4
    pandn    mm4, mm5

    por      mm0, mm4
    movq     %1,  mm0
%endmacro

%macro QUANT32_1x4 5
;;; %1      dct[y][x]
;;; %2,%3   quant_mf[i_mf][y][x]
;;; %4      i_qbits
;;; %5      f as doublewords
;;; trashes mm0-mm4
    movq        mm0, %1
    pxor        mm4, mm4
    pcmpgtw     mm4, mm0        ; mm4 = sign(mm0)
    pxor        mm0, mm4
    psubw       mm0, mm4        ; mm0 = abs(mm0)
    movq        mm1, mm0
    punpcklwd   mm0, mm0        ; duplicate the words for the upcomming
    punpckhwd   mm1, mm1        ; 32 bit multiplication

    movq        mm2, mm0        ; like in school ...
    movq        mm3, mm1
    pmulhuw     mm0, %2         ; ... multiply the parts ...
    pmulhuw     mm1, %3
    pmullw      mm2, %2
    pmullw      mm3, %3
    pslld       mm0, 16         ; ... shift ...
    pslld       mm1, 16
    paddd       mm0, mm2        ; ... and add them
    paddd       mm1, mm3

    paddd       mm0, %5         ; round with f
    paddd       mm1, %5
    psrad       mm0, %4
    psrad       mm1, %4
    packssdw    mm0, mm1        ; pack & store
    pxor        mm0, mm4
    psubw       mm0, mm4        ; restore sign
    movq        %1, mm0
%endmacro


ALIGN 16
;;; void x264_quant_8x8_core16_mmx( int16_t dct[8][8], int quant_mf[8][8], int i_qbits, int f )
x264_quant_8x8_core16_mmx:
    QUANT_AC_START

%rep 16
    QUANT16_1x4 [eax], [ecx], [ecx+8], mm6, mm7
    add  eax, 8
    add  ecx, 16
%endrep

    ret

ALIGN 16
;;; void x264_quant_4x4_core16_mmx( int16_t dct[4][4], int quant_mf[4][4], int i_qbits, int f )
x264_quant_4x4_core16_mmx:
    QUANT_AC_START

%rep 4
    QUANT16_1x4 [eax], [ecx], [ecx+8], mm6, mm7
    add  eax, 8
    add  ecx, 16
%endrep

    ret

ALIGN 16
;;; void x264_quant_8x8_core32_mmx( int16_t dct[8][8], int quant_mf[8][8], int i_qbits, int f )
x264_quant_8x8_core32_mmx:
    QUANT_AC_START

%rep 16
    QUANT32_1x4 [eax], [ecx], [ecx+8], mm6, mm7
    add  eax, 8
    add  ecx, 16
%endrep

    ret

ALIGN 16
;;; void x264_quant_4x4_core32_mmx( int16_t dct[4][4], int quant_mf[4][4], int i_qbits, int f )
x264_quant_4x4_core32_mmx:
    QUANT_AC_START

%rep 4
    QUANT32_1x4 [eax], [ecx], [ecx+8], mm6, mm7
    add  eax, 8
    add  ecx, 16
%endrep

    ret

ALIGN 16
;;; void x264_quant_4x4_dc_core32_mmx( int16_t dct[4][4], int i_quant_mf, int i_qbits, int f )
x264_quant_4x4_dc_core32_mmx:
    QUANT_DC_START

%rep 4
    QUANT32_1x4 [eax], mm5, mm5, mm6, mm7
    add  eax, 8
%endrep

    ret

ALIGN 16
;;; void x264_quant_2x2_dc_core32_mmx( int16_t dct[2][2], int i_quant_mf, int i_qbits, int f )
x264_quant_2x2_dc_core32_mmx:
    QUANT_DC_START

    QUANT32_1x4 [eax], mm5, mm5, mm6, mm7

    ret


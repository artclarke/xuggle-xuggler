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

%include "i386inc.asm"

SECTION_RODATA
pd_1:  times 2 dd 1

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

cglobal x264_dequant_4x4_mmx
cglobal x264_dequant_8x8_mmx

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


;=============================================================================
; dequant
;=============================================================================

%macro DEQUANT16_L_1x4 3
;;; %1      dct[y][x]
;;; %2,%3   dequant_mf[i_mf][y][x]
;;; mm5     i_qbits

    movq     mm1, %2
    movq     mm2, %3
    movq     mm0, %1
    packssdw mm1, mm2
    pmullw   mm0, mm1
    psllw    mm0, mm5
    movq     %1,  mm0
%endmacro

%macro DEQUANT16_R_1x4 3
;;; %1      dct[y][x]
;;; %2,%3   dequant_mf[i_mf][y][x]
;;; mm5     -i_qbits
;;; mm6     f as words

    movq     mm1, %2
    movq     mm2, %3
    movq     mm0, %1
    packssdw mm1, mm2
    pmullw   mm0, mm1
    paddw    mm0, mm6
    psraw    mm0, mm5
    movq     %1,  mm0
%endmacro

%macro DEQUANT32_R_1x4 3
;;; %1      dct[y][x]
;;; %2,%3   dequant_mf[i_mf][y][x]
;;; mm5     -i_qbits
;;; mm6     f as dwords
;;; mm7     0

    movq      mm0, %1
    movq      mm1, mm0
    punpcklwd mm0, mm0
    punpckhwd mm1, mm1

    movq      mm2, mm0
    movq      mm3, mm1
    pmulhw    mm0, %2
    pmulhw    mm1, %3
    pmullw    mm2, %2
    pmullw    mm3, %3
    pslld     mm0, 16
    pslld     mm1, 16
    paddd     mm0, mm2
    paddd     mm1, mm3

    paddd     mm0, mm6
    paddd     mm1, mm6
    psrad     mm0, mm5
    psrad     mm1, mm5

    packssdw  mm0, mm1
    movq      %1,  mm0
%endmacro

%macro DEQUANT_WxH 3
ALIGN 16
;;; void x264_dequant_4x4_mmx( int16_t dct[4][4], int dequant_mf[6][4][4], int i_qp )
%1:
    mov  edx, [esp+12] ; i_qp
    imul eax, edx, 0x2b
    shr  eax, 8       ; i_qbits = i_qp / 6
    lea  ecx, [eax+eax*2]
    sub  edx, ecx
    sub  edx, ecx     ; i_mf = i_qp % 6
    shl  edx, %3+2
    add  edx, [esp+8] ; dequant_mf[i_mf]
    mov  ecx, [esp+4] ; dct

    sub  eax, %3
    jl   .rshift32    ; negative qbits => rightshift

.lshift:
    movd mm5, eax

    mov  eax, 8*(%2-1)
.loopl16
%rep 2
    DEQUANT16_L_1x4 [ecx+eax], [edx+eax*2], [edx+eax*2+8]
    sub  eax, byte 8
%endrep
    jge  .loopl16

    nop
    ret

.rshift32:
    neg   eax
    picpush ebx
    picgetgot ebx
    movq  mm6, [pd_1 GOT_ebx]
    picpop ebx
    movd  mm5, eax
    pxor  mm7, mm7
    pslld mm6, mm5
    psrld mm6, 1

    mov  eax, 8*(%2-1)
.loopr32
%rep 2
    DEQUANT32_R_1x4 [ecx+eax], [edx+eax*2], [edx+eax*2+8]
    sub  eax, byte 8
%endrep
    jge  .loopr32

    nop
    ret
%endmacro

DEQUANT_WxH x264_dequant_4x4_mmx, 4, 4
DEQUANT_WxH x264_dequant_8x8_mmx, 16, 6

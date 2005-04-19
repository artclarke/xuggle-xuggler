;*****************************************************************************
;* dct.asm: h264 encoder library
;*****************************************************************************
;* Copyright (C) 2003 x264 project
;* $Id: dct.asm,v 1.1 2004/06/03 19:27:07 fenrir Exp $
;*
;* Authors: Min Chen <chenm001.163.com> (converted to nasm)
;*          Laurent Aimar <fenrir@via.ecp.fr> (initial version)
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
;*  2004.04.28  portab all 4x4 function to nasm (CM)                         *
;*                                                                           *
;*****************************************************************************

BITS 64

;=============================================================================
; Macros and other preprocessor constants
;=============================================================================

%macro cglobal 1
	%ifdef PREFIX
		global _%1
		%define %1 _%1
	%else
		global %1
	%endif
%endmacro

%macro MMX_ZERO 1
    pxor    %1, %1
%endmacro

%macro MMX_LOAD_DIFF_4P 5
    movd        %1, %4
    punpcklbw   %1, %3
    movd        %2, %5
    punpcklbw   %2, %3
    psubw       %1, %2
%endmacro

%macro MMX_SUMSUB_BA 2
    paddw   %1, %2
    paddw   %2, %2
    psubw   %2, %1
%endmacro

%macro MMX_SUMSUB_BADC 4
    paddw   %1, %2
    paddw   %3, %4
    paddw   %2, %2
    paddw   %4, %4
    psubw   %2, %1
    psubw   %4, %3
%endmacro

%macro MMX_SUMSUB2_AB 3
    movq    %3, %1
    paddw   %1, %1
    paddw   %1, %2
    psubw   %3, %2
    psubw   %3, %2
%endmacro

%macro MMX_SUMSUBD2_AB 4
    movq    %4, %1
    movq    %3, %2
    psraw   %2, 1
    psraw   %4, 1
    paddw   %1, %2
    psubw   %4, %3
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

;-----------------------------------------------------------------------------
; input ABCD output ADTC
;-----------------------------------------------------------------------------
%macro MMX_TRANSPOSE 5
    SBUTTERFLYwd %1, %2, %5
    SBUTTERFLYwd %3, %4, %2
    SBUTTERFLYdq %1, %3, %4
    SBUTTERFLYdq %5, %2, %3
%endmacro

%macro MMX_STORE_DIFF_4P 5
    paddw       %1, %3
    psraw       %1, 6
    movd        %2, %5
    punpcklbw   %2, %4
    paddsw      %1, %2
    packuswb    %1, %1
    movd        %5, %1
%endmacro

;%macro 
;%endmacro

;=============================================================================
; Local Data (Read Only)
;=============================================================================

%ifdef FORMAT_COFF
SECTION .rodata
%else
SECTION .rodata
%endif

;-----------------------------------------------------------------------------
; Various memory constants (trigonometric values or rounding values)
;-----------------------------------------------------------------------------

ALIGN 16
x264_mmx_1:
  dw 1, 1, 1, 1

x264_mmx_32:
  dw 32, 32, 32, 32

;=============================================================================
; Code
;=============================================================================

SECTION .text

cglobal x264_dct4x4dc_mmxext

ALIGN 16
;-----------------------------------------------------------------------------
;   void __cdecl dct4x4dc( int16_t d[4][4] )
;-----------------------------------------------------------------------------
x264_dct4x4dc_mmxext:
    movq    mm0,        [rdi+ 0]
    movq    mm1,        [rdi+ 8]
    movq    mm2,        [rdi+16]
    movq    mm3,        [rdi+24]

    MMX_SUMSUB_BADC     mm1, mm0, mm3, mm2          ; mm1=s01  mm0=d01  mm3=s23  mm2=d23
    MMX_SUMSUB_BADC     mm3, mm1, mm2, mm0          ; mm3=s01+s23  mm1=s01-s23  mm2=d01+d23  mm0=d01-d23

    MMX_TRANSPOSE       mm3, mm1, mm0, mm2, mm4     ; in: mm3, mm1, mm0, mm2  out: mm3, mm2, mm4, mm0 

    MMX_SUMSUB_BADC     mm2, mm3, mm0, mm4          ; mm2=s01  mm3=d01  mm0=s23  mm4=d23
    MMX_SUMSUB_BADC     mm0, mm2, mm4, mm3          ; mm0=s01+s23  mm2=s01-s23  mm4=d01+d23  mm3=d01-d23

    MMX_TRANSPOSE       mm0, mm2, mm3, mm4, mm1     ; in: mm0, mm2, mm3, mm4  out: mm0, mm4, mm1, mm3

    movq    mm6,        [x264_mmx_1]
    paddw   mm0,        mm6
    paddw   mm4,        mm6
    psraw   mm0,        1
    movq    [rdi+ 0],   mm0
    psraw   mm4,        1
    movq    [rdi+ 8],   mm4
    paddw   mm1,        mm6
    paddw   mm3,        mm6
    psraw   mm1,        1
    movq    [rdi+16],   mm1
    psraw   mm3,        1
    movq    [rdi+24],   mm3
    ret

cglobal x264_idct4x4dc_mmxext

ALIGN 16
;-----------------------------------------------------------------------------
;   void __cdecl x264_idct4x4dc_mmxext( int16_t d[4][4] )
;-----------------------------------------------------------------------------
x264_idct4x4dc_mmxext:
    movq    mm0, [rdi+ 0]
    movq    mm1, [rdi+ 8]
    movq    mm2, [rdi+16]
    movq    mm3, [rdi+24]

    MMX_SUMSUB_BADC     mm1, mm0, mm3, mm2          ; mm1=s01  mm0=d01  mm3=s23  mm2=d23
    MMX_SUMSUB_BADC     mm3, mm1, mm2, mm0          ; mm3=s01+s23 mm1=s01-s23 mm2=d01+d23 mm0=d01-d23

    MMX_TRANSPOSE       mm3, mm1, mm0, mm2, mm4     ; in: mm3, mm1, mm0, mm2  out: mm3, mm2, mm4, mm0 

    MMX_SUMSUB_BADC     mm2, mm3, mm0, mm4          ; mm2=s01  mm3=d01  mm0=s23  mm4=d23
    MMX_SUMSUB_BADC     mm0, mm2, mm4, mm3          ; mm0=s01+s23  mm2=s01-s23  mm4=d01+d23  mm3=d01-d23

    MMX_TRANSPOSE       mm0, mm2, mm3, mm4, mm1     ; in: mm0, mm2, mm3, mm4  out: mm0, mm4, mm1, mm3

    movq    [rdi+ 0],   mm0
    movq    [rdi+ 8],   mm4
    movq    [rdi+16],   mm1
    movq    [rdi+24],   mm3
    ret

cglobal x264_sub4x4_dct_mmxext

ALIGN 16
;-----------------------------------------------------------------------------
;   void __cdecl x264_sub4x4_dct_mmxext( int16_t dct[4][4], uint8_t *pix1, int i_pix1, uint8_t *pix2, int i_pix2 )
;-----------------------------------------------------------------------------
x264_sub4x4_dct_mmxext:
    push    rbx
    mov     rax, rsi        ; pix1
    movsxd  rbx, edx        ; i_pix1
;   mov     rcx, rcx        ; pix2
    movsxd  rdx, r8d        ; i_pix2

    MMX_ZERO    mm7

    ; Load 4 lines
    MMX_LOAD_DIFF_4P    mm0, mm6, mm7, [rax      ], [rcx]
    MMX_LOAD_DIFF_4P    mm1, mm6, mm7, [rax+rbx  ], [rcx+rdx]
    MMX_LOAD_DIFF_4P    mm2, mm6, mm7, [rax+rbx*2], [rcx+rdx*2]
    add     rax, rbx
    add     rcx, rdx
    MMX_LOAD_DIFF_4P    mm3, mm6, mm7, [rax+rbx*2], [rcx+rdx*2]

    MMX_SUMSUB_BADC     mm3, mm0, mm2, mm1          ; mm3=s03  mm0=d03  mm2=s12  mm1=d12

    MMX_SUMSUB_BA       mm2, mm3                    ; mm2=s03+s12      mm3=s03-s12
    MMX_SUMSUB2_AB      mm0, mm1, mm4               ; mm0=2.d03+d12    mm4=d03-2.d12

    ; transpose in: mm2, mm0, mm3, mm4, out: mm2, mm4, mm1, mm3
    MMX_TRANSPOSE       mm2, mm0, mm3, mm4, mm1

    MMX_SUMSUB_BADC     mm3, mm2, mm1, mm4          ; mm3=s03  mm2=d03  mm1=s12  mm4=d12

    MMX_SUMSUB_BA       mm1, mm3                    ; mm1=s03+s12      mm3=s03-s12
    MMX_SUMSUB2_AB      mm2, mm4, mm0               ; mm2=2.d03+d12    mm0=d03-2.d12

    ; transpose in: mm1, mm2, mm3, mm0, out: mm1, mm0, mm4, mm3
    MMX_TRANSPOSE       mm1, mm2, mm3, mm0, mm4

    movq    [rdi+ 0],   mm1 ; dct
    movq    [rdi+ 8],   mm0
    movq    [rdi+16],   mm4
    movq    [rdi+24],   mm3

    pop     rbx
    ret

cglobal x264_add4x4_idct_mmxext

ALIGN 16
;-----------------------------------------------------------------------------
;   void __cdecl x264_add4x4_idct_mmxext( uint8_t *p_dst, int i_dst, int16_t dct[4][4] )
;-----------------------------------------------------------------------------
x264_add4x4_idct_mmxext:
    ; Load dct coeffs
    movq    mm0, [rdx+ 0]   ; dct
    movq    mm4, [rdx+ 8]
    movq    mm3, [rdx+16]
    movq    mm1, [rdx+24]
    
    mov     rax, rdi        ; p_dst
    movsxd  rcx, esi        ; i_dst
    lea     rdx, [rcx+rcx*2]

    ; out:mm0, mm1, mm2, mm3
    MMX_TRANSPOSE       mm0, mm4, mm3, mm1, mm2

    MMX_SUMSUB_BA       mm2, mm0                        ; mm2=s02  mm0=d02
    MMX_SUMSUBD2_AB     mm1, mm3, mm5, mm4              ; mm1=s13  mm4=d13 ( well 1 + 3>>1 and 1>>1 + 3)

    MMX_SUMSUB_BADC     mm1, mm2, mm4, mm0              ; mm1=s02+s13  mm2=s02-s13  mm4=d02+d13  mm0=d02-d13

    ; in: mm1, mm4, mm0, mm2  out: mm1, mm2, mm3, mm0
    MMX_TRANSPOSE       mm1, mm4, mm0, mm2, mm3

    MMX_SUMSUB_BA       mm3, mm1                        ; mm3=s02  mm1=d02
    MMX_SUMSUBD2_AB     mm2, mm0, mm5, mm4              ; mm2=s13  mm4=d13 ( well 1 + 3>>1 and 1>>1 + 3)

    MMX_SUMSUB_BADC     mm2, mm3, mm4, mm1              ; mm2=s02+s13  mm3=s02-s13  mm4=d02+d13  mm1=d02-d13

    MMX_ZERO            mm7
    movq                mm6, [x264_mmx_32]
    
    MMX_STORE_DIFF_4P   mm2, mm0, mm6, mm7, [rax]
    MMX_STORE_DIFF_4P   mm4, mm0, mm6, mm7, [rax+rcx]
    MMX_STORE_DIFF_4P   mm1, mm0, mm6, mm7, [rax+rcx*2]
    MMX_STORE_DIFF_4P   mm3, mm0, mm6, mm7, [rax+rdx]

    ret


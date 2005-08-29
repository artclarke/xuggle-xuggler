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
x264_mmx_1:        dw  1,  1,  1,  1
x264_mmx_32:       dw 32, 32, 32, 32
x264_mmx_PPNN:     dw  1,  1, -1, -1
x264_mmx_PNPN:     dw  1, -1,  1, -1 
x264_mmx_PNNP:     dw  1, -1, -1,  1 
x264_mmx_PPPN:     dw  1,  1,  1, -1 
x264_mmx_PPNP:     dw  1,  1, -1,  1 
x264_mmx_2121:     dw  2,  1,  2,  1 
x264_mmx_p2n2p1p1: dw  2, -2,  1,  1

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



; =============================================================================
; 8x8 Transform
; =============================================================================

; -----------------------------------------------------------------------------
; input 2x8 unsigned bytes (%5,%6), zero (%7) output: difference (%1,%2)
; -----------------------------------------------------------------------------
%macro MMX_LOAD_DIFF_8P 7
    movq            %1, %5
    movq            %2, %1
    punpcklbw       %1, %7
    punpckhbw       %2, %7
    movq            %3, %6
    movq            %4, %3
    punpcklbw       %3, %7
    punpckhbw       %4, %7
    psubw           %1, %3
    psubw           %2, %4
%endmacro
 
%macro MMX_LOADSUMSUB 4     ; returns %1=%3+%4, %2=%3-%4
    movq            %2, %3
    movq            %1, %4
    MMX_SUMSUB_BA   %1, %2
%endmacro

%macro MMX_STORE_DIFF_8P 6
    movq            %1, %3
    movq            %2, %1
    punpcklbw       %1, %6
    punpckhbw       %2, %6
    paddw           %1, %4
    paddw           %2, %5
    packuswb        %1, %2
    movq            %3, %1
%endmacro

cglobal x264_pixel_sub_8x8_mmx
cglobal x264_xdct8_mmxext
cglobal x264_ydct8_mmx
cglobal x264_ydct8_sse2

cglobal x264_xidct8_mmxext
cglobal x264_yidct8_mmx
cglobal x264_yidct8_sse2
cglobal x264_pixel_add_8x8_mmx

ALIGN 16
;-----------------------------------------------------------------------------
;   void __cdecl x264_pixel_sub_8x8_mmx( int16_t *diff, uint8_t *pix1, int i_pix1, uint8_t *pix2, int i_pix2 );
;-----------------------------------------------------------------------------
x264_pixel_sub_8x8_mmx:
;   mov     rdi, rdi        ; diff
;   mov     rsi, rsi        ; pix1
    movsxd  rdx, edx        ; i_pix1
;   mov     rcx, rcx        ; pix2
    movsxd  r8,  r8d        ; i_pix2

    MMX_ZERO    mm7

    %assign disp 0
    %rep  8
    MMX_LOAD_DIFF_8P mm0, mm1, mm2, mm3, [rsi], [rcx], mm7
    movq        [rdi+disp], mm0
    movq        [rdi+disp+8], mm1
    add         rsi, rdx
    add         rcx, r8
    %assign disp disp+16
    %endrep

    ret

ALIGN 16
;-----------------------------------------------------------------------------
;   void __cdecl x264_xdct8_mmxext( int16_t dest[8][8] );
;-----------------------------------------------------------------------------
x264_xdct8_mmxext:

    movq        mm5, [x264_mmx_PPNN]
    movq        mm6, [x264_mmx_PNNP]
    movq        mm4, [x264_mmx_PPPN]
    movq        mm7, [x264_mmx_PPNP]

    ;-------------------------------------------------------------------------
    ; horizontal dct ( compute 1 row at a time -> 8 loops )
    ;-------------------------------------------------------------------------

    %assign disp 0
    %rep 8
    
    movq        mm0, [rdi+disp]
    movq        mm1, [rdi+disp+8]

    pshufw      mm2, mm1, 00011011b
    movq        mm1, mm0
    paddw       mm0, mm2                ; (low)s07/s16/d25/s34(high)
    psubw       mm1, mm2                ; (low)d07/d16/d25/d34(high)

    pshufw      mm2, mm0, 00011011b     ; (low)s34/s25/s16/s07(high)
    pmullw      mm0, mm5                ; (low)s07/s16/-s25/-s34(high)
    paddw       mm0, mm2                ; (low)a0/a1/a3/a2(high)

    movq        mm3, mm1
    psraw       mm1, 1                  ; (low)d07/d16/d25/d34(high) (x>>1)
    pshufw      mm2, mm3, 10110001b     ; (low)d16/d07/d34/d25(high)
    paddw       mm1, mm3                ; (low)d07/d16/d25/d34(high) (x+(x>>1))
    pshufw      mm3, mm2, 00011011b     ; (low)d25/d34/d07/d16(high)
    pmullw      mm2, mm5                ; (low)d16/d07/-d34/-d25(high)
    pmullw      mm1, mm6                ; (low)d07/-d16/-d25/d34(high) (x+(x>>1))
    paddw       mm3, mm2
    paddw       mm1, mm3                ; (low)a4/a6/a5/a7(high)


    pshufw      mm2, mm0, 11001001b     ; (low)a1/a3/a0/a2(high)
    pshufw      mm0, mm0, 10011100b     ; (low)a0/a2/a1/a3(high)
    pmullw      mm2, [x264_mmx_2121]
    pmullw      mm0, mm5                ; (low)a0/a2/-a1/-a3(high)
    psraw       mm2, 1                  ; (low)a1/a3>>1/a0/a2>>1(high)
    paddw       mm0, mm2                ; (low)dst0/dst2/dst4/dst6(high)

    pshufw      mm1, mm1, 00100111b     ; (low)a7/a6/a5/a4(high)
    pshufw      mm2, mm1, 00011011b     ; (low)a4/a5/a6/a7(high)
    psraw       mm1, 2                  ; (low)a7>>2/a6>>2/a5>>2/a4>>2(high)
    pmullw      mm2, mm4                ; (low)a4/a5/a6/-a7(high)
    pmullw      mm1, mm7                ; (low)a7>>2/a6>>2/-a5>>2/a4>>2(high)
    paddw       mm1, mm2                ; (low)dst1/dst3/dst5/dst7(high)

    movq        mm2, mm0
    punpcklwd   mm0, mm1                ; (low)dst0/dst1/dst2/dst3(high)
    punpckhwd   mm2, mm1                ; (low)dst4/dst5/dst6/dst7(high)

    movq        [rdi+disp], mm0
    movq        [rdi+disp+8], mm2

    %assign disp disp+16
    %endrep

    ret

ALIGN 16
;-----------------------------------------------------------------------------
;   void __cdecl x264_ydct8_mmx( int16_t dest[8][8] );
;-----------------------------------------------------------------------------
x264_ydct8_mmx:

    ;-------------------------------------------------------------------------
    ; vertical dct ( compute 4 columns at a time -> 2 loops )
    ;-------------------------------------------------------------------------

    %assign disp 0
    %rep 2
    
    MMX_LOADSUMSUB  mm2, mm3, [rdi+disp+0*16], [rdi+disp+7*16] ; mm2 = s07, mm3 = d07
    MMX_LOADSUMSUB  mm1, mm5, [rdi+disp+1*16], [rdi+disp+6*16] ; mm1 = s16, mm5 = d16
    MMX_LOADSUMSUB  mm0, mm6, [rdi+disp+2*16], [rdi+disp+5*16] ; mm0 = s25, mm6 = d25
    MMX_LOADSUMSUB  mm4, mm7, [rdi+disp+3*16], [rdi+disp+4*16] ; mm4 = s34, mm7 = d34

    MMX_SUMSUB_BA   mm4, mm2        ; mm4 = a0, mm2 = a2
    MMX_SUMSUB_BA   mm0, mm1        ; mm0 = a1, mm1 = a3
    MMX_SUMSUB_BA   mm0, mm4        ; mm0 = dst0, mm1 = dst4

    movq    [rdi+disp+0*16], mm0
    movq    [rdi+disp+4*16], mm4

    movq    mm0, mm1         ; a3
    psraw   mm0, 1           ; a3>>1
    paddw   mm0, mm2         ; a2 + (a3>>1)
    psraw   mm2, 1           ; a2>>1
    psubw   mm2, mm1         ; (a2>>1) - a3

    movq    [rdi+disp+2*16], mm0
    movq    [rdi+disp+6*16], mm2

    movq    mm0, mm6
    psraw   mm0, 1
    paddw   mm0, mm6         ; d25+(d25>>1)
    movq    mm1, mm3
    psubw   mm1, mm7         ; a5 = d07-d34-(d25+(d25>>1))
    psubw   mm1, mm0

    movq    mm0, mm5
    psraw   mm0, 1
    paddw   mm0, mm5         ; d16+(d16>>1)
    movq    mm2, mm3
    paddw   mm2, mm7         ; a6 = d07+d34-(d16+(d16>>1))
    psubw   mm2, mm0

    movq    mm0, mm3
    psraw   mm0, 1
    paddw   mm0, mm3         ; d07+(d07>>1)
    paddw   mm0, mm5
    paddw   mm0, mm6         ; a4 = d16+d25+(d07+(d07>>1))

    movq    mm3, mm7
    psraw   mm3, 1
    paddw   mm3, mm7         ; d34+(d34>>1)
    paddw   mm3, mm5
    psubw   mm3, mm6         ; a7 = d16-d25+(d34+(d34>>1))

    movq    mm7, mm3
    psraw   mm7, 2
    paddw   mm7, mm0         ; a4 + (a7>>2)

    movq    mm6, mm2
    psraw   mm6, 2
    paddw   mm6, mm1         ; a5 + (a6>>2)

    psraw   mm0, 2
    psraw   mm1, 2
    psubw   mm0, mm3         ; (a4>>2) - a7
    psubw   mm2, mm1         ; a6 - (a5>>2)

    movq    [rdi+disp+1*16], mm7
    movq    [rdi+disp+3*16], mm6
    movq    [rdi+disp+5*16], mm2
    movq    [rdi+disp+7*16], mm0

    %assign disp disp+8
    %endrep

    ret

ALIGN 16
;-----------------------------------------------------------------------------
;   void __cdecl x264_xidct8_mmxext( int16_t dest[8][8] );
;-----------------------------------------------------------------------------
x264_xidct8_mmxext:

    movq        mm4, [x264_mmx_PPNN]
    movq        mm5, [x264_mmx_PNPN]
    movq        mm6, [x264_mmx_PPNP]
    movq        mm7, [x264_mmx_PPPN]

    ;-------------------------------------------------------------------------
    ; horizontal idct ( compute 1 row at a time -> 8 loops )
    ;-------------------------------------------------------------------------

    %assign disp 0
    %rep 8

    pshufw      mm0, [rdi+disp], 11011000b      ; (low)d0,d2,d1,d3(high)
    pshufw      mm2, [rdi+disp+8], 11011000b    ; (low)d4,d6,d5,d7(high)
    movq        mm1, mm0
    punpcklwd   mm0, mm2                ; (low)d0,d4,d2,d6(high)
    punpckhwd   mm1, mm2                ; (low)d1,d5,d3,d7(high)

    pshufw      mm2, mm0, 10110001b     ; (low)d4,d0,d6,d2(high)
    pmullw      mm0, [x264_mmx_p2n2p1p1]; (low)2*d0,-2*d4,d2,d6(high)
    pmullw      mm2, mm6                ; (low)d4,d0,-d6,d2(high)
    psraw       mm0, 1                  ; (low)d0,-d4,d2>>1,d6>>1(high)
    paddw       mm0, mm2                ; (low)e0,e2,e4,e6(high)

    movq        mm3, mm1                ; (low)d1,d5,d3,d7(high)
    psraw       mm1, 1                  ; (low)d1>>1,d5>>1,d3>>1,d7>>1(high)
    pshufw      mm2, mm3, 10110001b     ; (low)d5,d1,d7,d3(high)
    paddw       mm1, mm3                ; (low)d1+(d1>>1),d5+(d5>>1),d3+(d3>>1),d7+(d7>>1)(high)
    pshufw      mm3, mm2, 00011011b     ; (low)d3,d7,d1,d5(high)
    pmullw      mm1, mm4                ; (low)d1+(d1>>1),d5+(d5>>1),-d3-(d3>>1),-d7-(d7>>1)(high)
    pmullw      mm2, mm5                ; (low)d5,-d1,d7,-d3(high)
    paddw       mm1, mm3
    paddw       mm1, mm2                ; (low)e7,e5,e3,e1(high)

    pshufw      mm2, mm0, 00011011b     ; (low)e6,e4,e2,e0(high)
    pmullw      mm0, mm4                ; (low)e0,e2,-e4,-e6(high)
    pshufw      mm3, mm1, 00011011b     ; (low)e1,e3,e5,e7(high)
    psraw       mm1, 2                  ; (low)e7>>2,e5>>2,e3>>2,e1>>2(high)
    pmullw      mm3, mm6                ; (low)e1,e3,-e5,e7(high)
    pmullw      mm1, mm7                ; (low)e7>>2,e5>>2,e3>>2,-e1>>2(high)
    paddw       mm0, mm2                ; (low)f0,f2,f4,f6(high)
    paddw       mm1, mm3                ; (low)f1,f3,f5,f7(high)

    pshufw      mm3, mm0, 00011011b     ; (low)f6,f4,f2,f0(high)
    pshufw      mm2, mm1, 00011011b     ; (low)f7,f5,f3,f1(high)
    psubw       mm3, mm1
    paddw       mm0, mm2

    movq        [rdi+disp], mm0
    movq        [rdi+disp+8], mm3

    %assign disp disp+16
    %endrep

    ret

ALIGN 16
;-----------------------------------------------------------------------------
;   void __cdecl x264_yidct8_mmx( int16_t dest[8][8] );
;-----------------------------------------------------------------------------
x264_yidct8_mmx:

    ;-------------------------------------------------------------------------
    ; vertical idct ( compute 4 columns at a time -> 2 loops )
    ;-------------------------------------------------------------------------

    %assign disp 0
    %rep 2

    movq        mm1, [rdi+disp+1*16]    ; mm1 = d1
    movq        mm3, [rdi+disp+3*16]    ; mm3 = d3
    movq        mm5, [rdi+disp+5*16]    ; mm5 = d5
    movq        mm7, [rdi+disp+7*16]    ; mm7 = d7

    movq        mm4, mm7
    psraw       mm4, 1
    movq        mm0, mm5
    psubw       mm0, mm7
    psubw       mm0, mm4
    psubw       mm0, mm3                ; mm0 = e1

    movq        mm6, mm3
    psraw       mm6, 1
    movq        mm2, mm7
    psubw       mm2, mm6
    psubw       mm2, mm3
    paddw       mm2, mm1                ; mm2 = e3

    movq        mm4, mm5
    psraw       mm4, 1
    paddw       mm4, mm5
    paddw       mm4, mm7
    psubw       mm4, mm1                ; mm4 = e5

    movq        mm6, mm1
    psraw       mm6, 1
    paddw       mm6, mm1
    paddw       mm6, mm5
    paddw       mm6, mm3                ; mm6 = e7

    movq        mm1, mm0
    movq        mm3, mm4
    movq        mm5, mm2
    movq        mm7, mm6
    psraw       mm6, 2
    psraw       mm3, 2
    psraw       mm5, 2
    psraw       mm0, 2
    paddw       mm1, mm6                ; mm1 = f1
    paddw       mm3, mm2                ; mm3 = f3
    psubw       mm5, mm4                ; mm5 = f5
    psubw       mm7, mm0                ; mm7 = f7

    movq        mm2, [rdi+disp+2*16]    ; mm2 = d2
    movq        mm6, [rdi+disp+6*16]    ; mm6 = d6
    movq        mm4, mm2
    movq        mm0, mm6
    psraw       mm4, 1
    psraw       mm6, 1
    psubw       mm4, mm0                ; mm4 = a4
    paddw       mm6, mm2                ; mm6 = a6

    movq        mm2, [rdi+disp+0*16]    ; mm2 = d0
    movq        mm0, [rdi+disp+4*16]    ; mm0 = d4
    MMX_SUMSUB_BA   mm0, mm2                ; mm0 = a0, mm2 = a2

    MMX_SUMSUB_BA   mm6, mm0                ; mm6 = f0, mm0 = f6
    MMX_SUMSUB_BA   mm4, mm2                ; mm4 = f2, mm2 = f4

    MMX_SUMSUB_BA   mm7, mm6                ; mm7 = g0, mm6 = g7
    MMX_SUMSUB_BA   mm5, mm4                ; mm5 = g1, mm4 = g6
    MMX_SUMSUB_BA   mm3, mm2                ; mm3 = g2, mm2 = g5
    MMX_SUMSUB_BA   mm1, mm0                ; mm1 = g3, mm0 = g4

    psraw       mm7, 6
    psraw       mm6, 6
    psraw       mm5, 6
    psraw       mm4, 6
    psraw       mm3, 6
    psraw       mm2, 6
    psraw       mm1, 6
    psraw       mm0, 6

    movq        [rdi+disp+0*16], mm7
    movq        [rdi+disp+1*16], mm5
    movq        [rdi+disp+2*16], mm3
    movq        [rdi+disp+3*16], mm1
    movq        [rdi+disp+4*16], mm0
    movq        [rdi+disp+5*16], mm2
    movq        [rdi+disp+6*16], mm4
    movq        [rdi+disp+7*16], mm6

    %assign disp disp+8
    %endrep

    ret

ALIGN 16
;-----------------------------------------------------------------------------
;   void __cdecl x264_pixel_add_8x8_mmx( unit8_t *dst, int i_dst, int16_t src[8][8] );
;-----------------------------------------------------------------------------
x264_pixel_add_8x8_mmx:
;   mov     rdi, rdi        ; dst
    movsxd  rsi, esi        ; i_dst
;   mov     rdx, rdx        ; src

    MMX_ZERO    mm7

    %assign disp 0
    %rep 8
    MMX_STORE_DIFF_8P   mm0, mm1, [rdi], [rdx+disp], [rdx+disp+8], mm7
    add         rdi, rsi
    %assign disp disp+16
    %endrep
    ret


;*****************************************************************************
;* dct.asm: h264 encoder library
;*****************************************************************************
;* Copyright (C) 2003 x264 project
;* $Id: dct.asm,v 1.1 2004/06/03 19:27:07 fenrir Exp $
;*
;* Authors: Laurent Aimar <fenrir@via.ecp.fr> (initial version)
;*          Min Chen <chenm001.163.com> (converted to nasm)
;*          Christian Heine <sennindemokrit@gmx.net> (dct8/idct8 functions)
;*          Loren Merritt <lorenm@u.washington.edu> (misc)
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
;*  2005.08.24  added mmxext optimized dct8/idct8 functions (CH)             *
;*                                                                           *
;*****************************************************************************

BITS 32

;=============================================================================
; Macros and other preprocessor constants
;=============================================================================

%include "i386inc.asm"

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

;=============================================================================
; Local Data (Read Only)
;=============================================================================

SECTION_RODATA

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

;-----------------------------------------------------------------------------
;   void __cdecl x264_dct4x4dc_mmx( int16_t d[4][4] )
;-----------------------------------------------------------------------------
cglobal x264_dct4x4dc_mmx
    mov     eax,        [esp+ 4]
    movq    mm0,        [eax+ 0]
    movq    mm1,        [eax+ 8]
    movq    mm2,        [eax+16]
    movq    mm3,        [eax+24]

    picpush ebx
    picgetgot ebx

    MMX_SUMSUB_BADC     mm1, mm0, mm3, mm2          ; mm1=s01  mm0=d01  mm3=s23  mm2=d23
    MMX_SUMSUB_BADC     mm3, mm1, mm2, mm0          ; mm3=s01+s23  mm1=s01-s23  mm2=d01+d23  mm0=d01-d23

    MMX_TRANSPOSE       mm3, mm1, mm0, mm2, mm4     ; in: mm3, mm1, mm0, mm2  out: mm3, mm2, mm4, mm0 

    MMX_SUMSUB_BADC     mm2, mm3, mm0, mm4          ; mm2=s01  mm3=d01  mm0=s23  mm4=d23
    MMX_SUMSUB_BADC     mm0, mm2, mm4, mm3          ; mm0=s01+s23  mm2=s01-s23  mm4=d01+d23  mm3=d01-d23

    movq    mm6,        [x264_mmx_1 GOT_ebx]
    paddw   mm0,        mm6
    paddw   mm2,        mm6
    psraw   mm0,        1
    movq    [eax+ 0],   mm0
    psraw   mm2,        1
    movq    [eax+ 8],   mm2
    paddw   mm3,        mm6
    paddw   mm4,        mm6
    psraw   mm3,        1
    movq    [eax+16],   mm3
    psraw   mm4,        1
    movq    [eax+24],   mm4
    picpop  ebx
    ret

;-----------------------------------------------------------------------------
;   void __cdecl x264_idct4x4dc_mmx( int16_t d[4][4] )
;-----------------------------------------------------------------------------
cglobal x264_idct4x4dc_mmx
    mov     eax, [esp+ 4]
    movq    mm0, [eax+ 0]
    movq    mm1, [eax+ 8]
    movq    mm2, [eax+16]
    movq    mm3, [eax+24]

    MMX_SUMSUB_BADC     mm1, mm0, mm3, mm2          ; mm1=s01  mm0=d01  mm3=s23  mm2=d23
    MMX_SUMSUB_BADC     mm3, mm1, mm2, mm0          ; mm3=s01+s23 mm1=s01-s23 mm2=d01+d23 mm0=d01-d23

    MMX_TRANSPOSE       mm3, mm1, mm0, mm2, mm4     ; in: mm3, mm1, mm0, mm2  out: mm3, mm2, mm4, mm0 

    MMX_SUMSUB_BADC     mm2, mm3, mm0, mm4          ; mm2=s01  mm3=d01  mm0=s23  mm4=d23
    MMX_SUMSUB_BADC     mm0, mm2, mm4, mm3          ; mm0=s01+s23  mm2=s01-s23  mm4=d01+d23  mm3=d01-d23

    movq    [eax+ 0],   mm0
    movq    [eax+ 8],   mm2
    movq    [eax+16],   mm3
    movq    [eax+24],   mm4
    ret

;-----------------------------------------------------------------------------
;   void __cdecl x264_sub4x4_dct_mmx( int16_t dct[4][4], uint8_t *pix1, uint8_t *pix2 )
;-----------------------------------------------------------------------------
cglobal x264_sub4x4_dct_mmx
    mov     eax, [esp+ 8]   ; pix1
    mov     ecx, [esp+12]   ; pix2

    MMX_ZERO    mm7

    ; Load 4 lines
    MMX_LOAD_DIFF_4P    mm0, mm6, mm7, [eax+0*FENC_STRIDE], [ecx+0*FDEC_STRIDE]
    MMX_LOAD_DIFF_4P    mm1, mm6, mm7, [eax+1*FENC_STRIDE], [ecx+1*FDEC_STRIDE]
    MMX_LOAD_DIFF_4P    mm2, mm6, mm7, [eax+2*FENC_STRIDE], [ecx+2*FDEC_STRIDE]
    MMX_LOAD_DIFF_4P    mm3, mm6, mm7, [eax+3*FENC_STRIDE], [ecx+3*FDEC_STRIDE]

    MMX_SUMSUB_BADC     mm3, mm0, mm2, mm1          ; mm3=s03  mm0=d03  mm2=s12  mm1=d12

    MMX_SUMSUB_BA       mm2, mm3                    ; mm2=s03+s12      mm3=s03-s12
    MMX_SUMSUB2_AB      mm0, mm1, mm4               ; mm0=2.d03+d12    mm4=d03-2.d12

    ; transpose in: mm2, mm0, mm3, mm4, out: mm2, mm4, mm1, mm3
    MMX_TRANSPOSE       mm2, mm0, mm3, mm4, mm1

    MMX_SUMSUB_BADC     mm3, mm2, mm1, mm4          ; mm3=s03  mm2=d03  mm1=s12  mm4=d12

    MMX_SUMSUB_BA       mm1, mm3                    ; mm1=s03+s12      mm3=s03-s12
    MMX_SUMSUB2_AB      mm2, mm4, mm0               ; mm2=2.d03+d12    mm0=d03-2.d12

    mov     eax, [esp+ 4]   ; dct
    movq    [eax+ 0],   mm1
    movq    [eax+ 8],   mm2
    movq    [eax+16],   mm3
    movq    [eax+24],   mm0

    ret

;-----------------------------------------------------------------------------
;   void __cdecl x264_add4x4_idct_mmx( uint8_t *p_dst, int16_t dct[4][4] )
;-----------------------------------------------------------------------------
cglobal x264_add4x4_idct_mmx
    ; Load dct coeffs
    mov     eax, [esp+ 8]   ; dct
    movq    mm0, [eax+ 0]
    movq    mm1, [eax+ 8]
    movq    mm2, [eax+16]
    movq    mm3, [eax+24]
    
    mov     eax, [esp+ 4]   ; p_dst

    picpush ebx
    picgetgot ebx

    MMX_SUMSUB_BA       mm2, mm0                        ; mm2=s02  mm0=d02
    MMX_SUMSUBD2_AB     mm1, mm3, mm5, mm4              ; mm1=s13  mm4=d13 ( well 1 + 3>>1 and 1>>1 + 3)

    MMX_SUMSUB_BADC     mm1, mm2, mm4, mm0              ; mm1=s02+s13  mm2=s02-s13  mm4=d02+d13  mm0=d02-d13

    ; in: mm1, mm4, mm0, mm2  out: mm1, mm2, mm3, mm0
    MMX_TRANSPOSE       mm1, mm4, mm0, mm2, mm3

    MMX_SUMSUB_BA       mm3, mm1                        ; mm3=s02  mm1=d02
    MMX_SUMSUBD2_AB     mm2, mm0, mm5, mm4              ; mm2=s13  mm4=d13 ( well 1 + 3>>1 and 1>>1 + 3)

    MMX_SUMSUB_BADC     mm2, mm3, mm4, mm1              ; mm2=s02+s13  mm3=s02-s13  mm4=d02+d13  mm1=d02-d13

    MMX_ZERO            mm7
    movq                mm6, [x264_mmx_32 GOT_ebx]
    
    MMX_STORE_DIFF_4P   mm2, mm0, mm6, mm7, [eax+0*FDEC_STRIDE]
    MMX_STORE_DIFF_4P   mm4, mm0, mm6, mm7, [eax+1*FDEC_STRIDE]
    MMX_STORE_DIFF_4P   mm1, mm0, mm6, mm7, [eax+2*FDEC_STRIDE]
    MMX_STORE_DIFF_4P   mm3, mm0, mm6, mm7, [eax+3*FDEC_STRIDE]

    picpop  ebx
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

;-----------------------------------------------------------------------------
;   void __cdecl x264_pixel_sub_8x8_mmx( int16_t *diff, uint8_t *pix1, uint8_t *pix2 );
;-----------------------------------------------------------------------------
ALIGN 16
x264_pixel_sub_8x8_mmx:

    mov         edx, [esp+ 4]           ; diff
    mov         eax, [esp+ 8]           ; pix1
    mov         ecx, [esp+12]           ; pix2

    MMX_ZERO    mm7

    %assign disp 0
    %rep  8
    MMX_LOAD_DIFF_8P mm0, mm1, mm2, mm3, [eax], [ecx], mm7
    movq        [edx+disp], mm0
    movq        [edx+disp+8], mm1
    add         eax, FENC_STRIDE
    add         ecx, FDEC_STRIDE
    %assign disp disp+16
    %endrep

    ret

;-----------------------------------------------------------------------------
;   void __cdecl x264_ydct8_mmx( int16_t dest[8][8] );
;-----------------------------------------------------------------------------
ALIGN 16
x264_ydct8_mmx:

    mov         eax, [esp+04]           ; dest

    ;-------------------------------------------------------------------------
    ; vertical dct ( compute 4 columns at a time -> 2 loops )
    ;-------------------------------------------------------------------------

    %assign disp 0
    %rep 2
    
    MMX_LOADSUMSUB  mm2, mm3, [eax+disp+0*16], [eax+disp+7*16] ; mm2 = s07, mm3 = d07
    MMX_LOADSUMSUB  mm1, mm5, [eax+disp+1*16], [eax+disp+6*16] ; mm1 = s16, mm5 = d16
    MMX_LOADSUMSUB  mm0, mm6, [eax+disp+2*16], [eax+disp+5*16] ; mm0 = s25, mm6 = d25
    MMX_LOADSUMSUB  mm4, mm7, [eax+disp+3*16], [eax+disp+4*16] ; mm4 = s34, mm7 = d34

    MMX_SUMSUB_BA   mm4, mm2        ; mm4 = a0, mm2 = a2
    MMX_SUMSUB_BA   mm0, mm1        ; mm0 = a1, mm1 = a3
    MMX_SUMSUB_BA   mm0, mm4        ; mm0 = dst0, mm1 = dst4

    movq    [eax+disp+0*16], mm0
    movq    [eax+disp+4*16], mm4

    movq    mm0, mm1         ; a3
    psraw   mm0, 1           ; a3>>1
    paddw   mm0, mm2         ; a2 + (a3>>1)
    psraw   mm2, 1           ; a2>>1
    psubw   mm2, mm1         ; (a2>>1) - a3

    movq    [eax+disp+2*16], mm0
    movq    [eax+disp+6*16], mm2

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

    movq    [eax+disp+1*16], mm7
    movq    [eax+disp+3*16], mm6
    movq    [eax+disp+5*16], mm2
    movq    [eax+disp+7*16], mm0

    %assign disp disp+8
    %endrep

    ret

;-----------------------------------------------------------------------------
;   void __cdecl x264_yidct8_mmx( int16_t dest[8][8] );
;-----------------------------------------------------------------------------
ALIGN 16
x264_yidct8_mmx:

    mov         eax, [esp+04]           ; dest

    ;-------------------------------------------------------------------------
    ; vertical idct ( compute 4 columns at a time -> 2 loops )
    ;-------------------------------------------------------------------------

    %assign disp 0
    %rep 2

    movq        mm1, [eax+disp+1*16]    ; mm1 = d1
    movq        mm3, [eax+disp+3*16]    ; mm3 = d3
    movq        mm5, [eax+disp+5*16]    ; mm5 = d5
    movq        mm7, [eax+disp+7*16]    ; mm7 = d7

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

    movq        mm2, [eax+disp+2*16]    ; mm2 = d2
    movq        mm6, [eax+disp+6*16]    ; mm6 = d6
    movq        mm4, mm2
    movq        mm0, mm6
    psraw       mm4, 1
    psraw       mm6, 1
    psubw       mm4, mm0                ; mm4 = a4
    paddw       mm6, mm2                ; mm6 = a6

    movq        mm2, [eax+disp+0*16]    ; mm2 = d0
    movq        mm0, [eax+disp+4*16]    ; mm0 = d4
    MMX_SUMSUB_BA   mm0, mm2            ; mm0 = a0, mm2 = a2

    MMX_SUMSUB_BADC mm6, mm0, mm4, mm2  ; mm6 = f0, mm0 = f6
                                        ; mm4 = f2, mm2 = f4

    MMX_SUMSUB_BADC mm7, mm6, mm5, mm4  ; mm7 = g0, mm6 = g7
                                        ; mm5 = g1, mm4 = g6
    MMX_SUMSUB_BADC mm3, mm2, mm1, mm0  ; mm3 = g2, mm2 = g5
                                        ; mm1 = g3, mm0 = g4

    movq        [eax+disp+0*16], mm7
    movq        [eax+disp+1*16], mm5
    movq        [eax+disp+2*16], mm3
    movq        [eax+disp+3*16], mm1
    movq        [eax+disp+4*16], mm0
    movq        [eax+disp+5*16], mm2
    movq        [eax+disp+6*16], mm4
    movq        [eax+disp+7*16], mm6

    %assign disp disp+8
    %endrep

    ret

;-----------------------------------------------------------------------------
;   void __cdecl x264_pixel_add_8x8_mmx( uint8_t *dst, int16_t src[8][8] );
;-----------------------------------------------------------------------------
ALIGN 16
x264_pixel_add_8x8_mmx:
    mov         eax, [esp+4]       ; dst
    mov         edx, [esp+8]       ; src

    MMX_ZERO    mm7

    %assign disp 0
    %rep 8
    movq        mm0, [eax]
    movq        mm2, [edx+disp]
    movq        mm3, [edx+disp+8]
    movq        mm1, mm0
    psraw       mm2, 6
    psraw       mm3, 6
    punpcklbw   mm0, mm7
    punpckhbw   mm1, mm7
    paddw       mm0, mm2
    paddw       mm1, mm3
    packuswb    mm0, mm1
    movq      [eax], mm0
    add         eax, FDEC_STRIDE
    %assign disp disp+16
    %endrep
    ret

;-----------------------------------------------------------------------------
;   void __cdecl x264_transpose_8x8_mmx( int16_t src[8][8] );
;-----------------------------------------------------------------------------
ALIGN 16
x264_transpose_8x8_mmx:
    mov   eax, [esp+4]

    movq  mm0, [eax    ]
    movq  mm1, [eax+ 16]
    movq  mm2, [eax+ 32]
    movq  mm3, [eax+ 48]
    MMX_TRANSPOSE  mm0, mm1, mm2, mm3, mm4
    movq  [eax    ], mm0
    movq  [eax+ 16], mm3
    movq  [eax+ 32], mm4
    movq  [eax+ 48], mm2

    movq  mm0, [eax+ 72]
    movq  mm1, [eax+ 88]
    movq  mm2, [eax+104]
    movq  mm3, [eax+120]
    MMX_TRANSPOSE  mm0, mm1, mm2, mm3, mm4
    movq  [eax+ 72], mm0
    movq  [eax+ 88], mm3
    movq  [eax+104], mm4
    movq  [eax+120], mm2

    movq  mm0, [eax+  8]
    movq  mm1, [eax+ 24]
    movq  mm2, [eax+ 40]
    movq  mm3, [eax+ 56]
    MMX_TRANSPOSE  mm0, mm1, mm2, mm3, mm4
    movq  mm1, [eax+ 64]
    movq  mm5, [eax+ 80]
    movq  mm6, [eax+ 96]
    movq  mm7, [eax+112]

    movq  [eax+ 64], mm0
    movq  [eax+ 80], mm3
    movq  [eax+ 96], mm4
    movq  [eax+112], mm2
    MMX_TRANSPOSE  mm1, mm5, mm6, mm7, mm4
    movq  [eax+  8], mm1
    movq  [eax+ 24], mm7
    movq  [eax+ 40], mm4
    movq  [eax+ 56], mm6

    ret

;-----------------------------------------------------------------------------
;   void __cdecl x264_sub8x8_dct8_mmx( int16_t dct[8][8], uint8_t *pix1, uint8_t *pix2 )
;-----------------------------------------------------------------------------
cglobal x264_sub8x8_dct8_mmx
    push dword [esp+12]
    push dword [esp+12]
    push dword [esp+12]
    call x264_pixel_sub_8x8_mmx
    call x264_ydct8_mmx
    call x264_transpose_8x8_mmx
    add  esp, 12
    jmp  x264_ydct8_mmx

;-----------------------------------------------------------------------------
;   void __cdecl x264_add8x8_idct8_mmx( uint8_t *dst, int16_t dct[8][8] )
;-----------------------------------------------------------------------------
cglobal x264_add8x8_idct8_mmx
    mov  eax, [esp+8]
    add  word [eax], 32
    push eax
    call x264_yidct8_mmx
    call x264_transpose_8x8_mmx
    call x264_yidct8_mmx
    add  esp, 4
    jmp  x264_pixel_add_8x8_mmx

;-----------------------------------------------------------------------------
;   void __cdecl x264_sub8x8_dct_mmx( int16_t dct[4][4][4],
;                                     uint8_t *pix1, uint8_t *pix2 )
;-----------------------------------------------------------------------------
%macro SUB_NxN_DCT 4
cglobal %1
    mov  edx, [esp+12]
    mov  ecx, [esp+ 8]
    mov  eax, [esp+ 4]
    add  edx, %4
    add  ecx, %4
    add  eax, %3
    push edx
    push ecx
    push eax
    call %2
    add  dword [esp+0], %3
    add  dword [esp+4], %4*FENC_STRIDE-%4
    add  dword [esp+8], %4*FDEC_STRIDE-%4
    call %2
    add  dword [esp+0], %3
    add  dword [esp+4], %4
    add  dword [esp+8], %4
    call %2
    add  esp, 12
    jmp  %2
%endmacro

;-----------------------------------------------------------------------------
;   void __cdecl x264_add8x8_idct_mmx( uint8_t *pix, int16_t dct[4][4][4] )
;-----------------------------------------------------------------------------
%macro ADD_NxN_IDCT 4
cglobal %1
    mov  ecx, [esp+8]
    mov  eax, [esp+4]
    add  ecx, %3
    add  eax, %4
    push ecx
    push eax
    call %2
    add  dword [esp+0], %4*FDEC_STRIDE-%4
    add  dword [esp+4], %3
    call %2
    add  dword [esp+0], %4
    add  dword [esp+4], %3
    call %2
    add  esp, 8
    jmp  %2
%endmacro

SUB_NxN_DCT  x264_sub8x8_dct_mmx,     x264_sub4x4_dct_mmx,    32, 4
ADD_NxN_IDCT x264_add8x8_idct_mmx,    x264_add4x4_idct_mmx,   32, 4

SUB_NxN_DCT  x264_sub16x16_dct_mmx,   x264_sub8x8_dct_mmx,   128, 8
ADD_NxN_IDCT x264_add16x16_idct_mmx,  x264_add8x8_idct_mmx,  128, 8

SUB_NxN_DCT  x264_sub16x16_dct8_mmx,  x264_sub8x8_dct8_mmx,  128, 8
ADD_NxN_IDCT x264_add16x16_idct8_mmx, x264_add8x8_idct8_mmx, 128, 8


;-----------------------------------------------------------------------------
; void __cdecl x264_zigzag_scan_4x4_field_mmx( int level[16], int16_t dct[4][4] )
;-----------------------------------------------------------------------------
cglobal x264_zigzag_scan_4x4_field_mmx
    mov       edx, [esp+8]
    mov       ecx, [esp+4]
    punpcklwd mm0, [edx]
    punpckhwd mm1, [edx]
    punpcklwd mm2, [edx+8]
    punpckhwd mm3, [edx+8]
    punpcklwd mm4, [edx+16]
    punpckhwd mm5, [edx+16]
    punpcklwd mm6, [edx+24]
    punpckhwd mm7, [edx+24]
    psrad     mm0, 16
    psrad     mm1, 16
    psrad     mm2, 16
    psrad     mm3, 16
    psrad     mm4, 16
    psrad     mm5, 16
    psrad     mm6, 16
    psrad     mm7, 16
    movq      [ecx   ], mm0
    movq      [ecx+16], mm2
    movq      [ecx+24], mm3
    movq      [ecx+32], mm4
    movq      [ecx+40], mm5
    movq      [ecx+48], mm6
    movq      [ecx+56], mm7
    movq      [ecx+12], mm1
    movd      [ecx+ 8], mm2
    ret

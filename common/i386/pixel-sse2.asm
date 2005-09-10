;*****************************************************************************
;* pixel-sse2.asm: h264 encoder library
;*****************************************************************************
;* Copyright (C) 2005 x264 project
;*
;* Authors: Alex Izvorski <aizvorksi@gmail.com>
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

%macro cglobal 1
    %ifdef PREFIX
        global _%1
        %define %1 _%1
    %else
        global %1
    %endif
%endmacro


%ifdef FORMAT_COFF
SECTION .rodata data
%else
SECTION .rodata data align=16
%endif

pd_0000ffff: times 4 dd 0x0000ffff


SECTION .text


cglobal x264_pixel_sad_16x16_sse2
cglobal x264_pixel_sad_16x8_sse2
cglobal x264_pixel_ssd_16x16_sse2
cglobal x264_pixel_ssd_16x8_sse2
cglobal x264_pixel_satd_8x4_sse2
cglobal x264_pixel_satd_8x8_sse2
cglobal x264_pixel_satd_16x8_sse2
cglobal x264_pixel_satd_8x16_sse2
cglobal x264_pixel_satd_16x16_sse2

%macro SAD_INC_4x16P_SSE2 0
    movdqu  xmm1,   [ecx]
    movdqu  xmm2,   [ecx+edx]
    lea     ecx,    [ecx+2*edx]
    movdqu  xmm3,   [ecx]
    movdqu  xmm4,   [ecx+edx]
    psadbw  xmm1,   [eax]
    psadbw  xmm2,   [eax+ebx]
    lea     eax,    [eax+2*ebx]
    psadbw  xmm3,   [eax]
    psadbw  xmm4,   [eax+ebx]
    lea     eax,    [eax+2*ebx]
    lea     ecx,    [ecx+2*edx]
    paddw   xmm1,   xmm2
    paddw   xmm3,   xmm4
    paddw   xmm0,   xmm1
    paddw   xmm0,   xmm3
%endmacro

%macro SAD_START_SSE2 0
    push    ebx

    mov     eax,    [esp+ 8]    ; pix1
    mov     ebx,    [esp+12]    ; stride1
    mov     ecx,    [esp+16]    ; pix2
    mov     edx,    [esp+20]    ; stride2
%endmacro

%macro SAD_END_SSE2 0
    movdqa  xmm1, xmm0
    psrldq  xmm0,  8
    paddw   xmm0, xmm1
    movd    eax,  xmm0

    pop ebx
    ret
%endmacro

ALIGN 16
;-----------------------------------------------------------------------------
;   int __cdecl x264_pixel_sad_16x16_sse2 (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
x264_pixel_sad_16x16_sse2:
    SAD_START_SSE2
    movdqu xmm0, [ecx]
    movdqu xmm1, [ecx+edx]
    lea    ecx,  [ecx+2*edx]
    movdqu xmm2, [ecx]
    movdqu xmm3, [ecx+edx]
    lea    ecx,  [ecx+2*edx]
    psadbw xmm0, [eax]
    psadbw xmm1, [eax+ebx]
    lea    eax,  [eax+2*ebx]
    movdqu xmm4, [ecx]
    paddw  xmm0, xmm1
    psadbw xmm2, [eax]
    psadbw xmm3, [eax+ebx]
    lea    eax,  [eax+2*ebx]
    movdqu xmm5, [ecx+edx]
    lea    ecx,  [ecx+2*edx]
    paddw  xmm2, xmm3
    movdqu xmm6, [ecx]
    movdqu xmm7, [ecx+edx]
    lea    ecx,  [ecx+2*edx]
    paddw  xmm0, xmm2
    psadbw xmm4, [eax]
    psadbw xmm5, [eax+ebx]
    lea    eax,  [eax+2*ebx]
    movdqu xmm1, [ecx]
    paddw  xmm4, xmm5
    psadbw xmm6, [eax]
    psadbw xmm7, [eax+ebx]
    lea    eax,  [eax+2*ebx]
    movdqu xmm2, [ecx+edx]
    lea    ecx,  [ecx+2*edx]
    paddw  xmm6, xmm7
    movdqu xmm3, [ecx]
    paddw  xmm0, xmm4
    movdqu xmm4, [ecx+edx]
    lea    ecx,  [ecx+2*edx]
    paddw  xmm0, xmm6
    psadbw xmm1, [eax]
    psadbw xmm2, [eax+ebx]
    lea    eax,  [eax+2*ebx]
    movdqu xmm5, [ecx]
    paddw  xmm1, xmm2
    psadbw xmm3, [eax]
    psadbw xmm4, [eax+ebx]
    lea    eax,  [eax+2*ebx]
    movdqu xmm6, [ecx+edx]
    lea    ecx,  [ecx+2*edx]
    paddw  xmm3, xmm4
    movdqu xmm7, [ecx]
    paddw  xmm0, xmm1
    movdqu xmm1, [ecx+edx]
    paddw  xmm0, xmm3
    psadbw xmm5, [eax]
    psadbw xmm6, [eax+ebx]
    lea    eax,  [eax+2*ebx]
    paddw  xmm5, xmm6
    psadbw xmm7, [eax]
    psadbw xmm1, [eax+ebx]
    paddw  xmm7, xmm1
    paddw  xmm0, xmm5
    paddw  xmm0, xmm7
    SAD_END_SSE2

ALIGN 16
;-----------------------------------------------------------------------------
;   int __cdecl x264_pixel_sad_16x8_sse2 (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
x264_pixel_sad_16x8_sse2:
    SAD_START_SSE2
    pxor    xmm0,   xmm0
    SAD_INC_4x16P_SSE2
    SAD_INC_4x16P_SSE2
    SAD_END_SSE2

%macro SSD_INC_2x16P_SSE2 0
    movdqu  xmm1,   [eax]
    movdqu  xmm2,   [ecx]
    movdqu  xmm3,   [eax+ebx]
    movdqu  xmm4,   [ecx+edx]

    movdqa  xmm5,   xmm1
    movdqa  xmm6,   xmm3
    psubusb xmm1,   xmm2
    psubusb xmm3,   xmm4
    psubusb xmm2,   xmm5
    psubusb xmm4,   xmm6
    por     xmm1,   xmm2
    por     xmm3,   xmm4

    movdqa  xmm2,   xmm1
    movdqa  xmm4,   xmm3
    punpcklbw xmm1, xmm7
    punpckhbw xmm2, xmm7
    punpcklbw xmm3, xmm7
    punpckhbw xmm4, xmm7
    pmaddwd xmm1,   xmm1
    pmaddwd xmm2,   xmm2
    pmaddwd xmm3,   xmm3
    pmaddwd xmm4,   xmm4

    lea     eax,    [eax+2*ebx]
    lea     ecx,    [ecx+2*edx]

    paddd   xmm1,   xmm2
    paddd   xmm3,   xmm4
    paddd   xmm0,   xmm1
    paddd   xmm0,   xmm3
%endmacro

%macro SSD_INC_8x16P_SSE2 0
    SSD_INC_2x16P_SSE2
    SSD_INC_2x16P_SSE2
    SSD_INC_2x16P_SSE2
    SSD_INC_2x16P_SSE2
%endmacro

%macro SSD_START_SSE2 0
    push    ebx

    mov     eax,    [esp+ 8]    ; pix1
    mov     ebx,    [esp+12]    ; stride1
    mov     ecx,    [esp+16]    ; pix2
    mov     edx,    [esp+20]    ; stride2

    pxor    xmm7,   xmm7         ; zero
    pxor    xmm0,   xmm0         ; mm0 holds the sum
%endmacro

%macro SSD_END_SSE2 0
    movdqa  xmm1,   xmm0
    psrldq  xmm1,    8
    paddd   xmm0,   xmm1

    movdqa  xmm1,   xmm0
    psrldq  xmm1,    4
    paddd   xmm0,   xmm1

    movd    eax,    xmm0

    pop ebx
    ret
%endmacro

ALIGN 16
;-----------------------------------------------------------------------------
;   int __cdecl x264_pixel_ssd_16x16_sse2 (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
x264_pixel_ssd_16x16_sse2:
    SSD_START_SSE2
    SSD_INC_8x16P_SSE2
    SSD_INC_8x16P_SSE2
    SSD_END_SSE2

ALIGN 16
;-----------------------------------------------------------------------------
;   int __cdecl x264_pixel_ssd_16x8_sse2 (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
x264_pixel_ssd_16x8_sse2:
    SSD_START_SSE2
    SSD_INC_8x16P_SSE2
    SSD_END_SSE2

; %1=(row2, row0) %2=(row3, row1) %3=junk
; output in %1=(row3, row0) and %3=(row2, row1)
%macro HADAMARD4x4_SSE2 3
    movdqa     %3, %1
    paddw      %1, %2
    psubw      %3, %2
    movdqa     %2, %1
    punpcklqdq %1, %3
    punpckhqdq %2, %3
    movdqa     %3, %1
    paddw      %1, %2
    psubw      %3, %2
%endmacro

;;; two HADAMARD4x4_SSE2 running side-by-side
%macro HADAMARD4x4_TWO_SSE2 6    ; a02 a13 junk1 b02 b13 junk2 (1=4 2=5 3=6)
    movdqa     %3, %1
    movdqa     %6, %4
    paddw      %1, %2
    paddw      %4, %5
    psubw      %3, %2
    psubw      %6, %5
    movdqa     %2, %1
    movdqa     %5, %4
    punpcklqdq %1, %3
    punpcklqdq %4, %6
    punpckhqdq %2, %3
    punpckhqdq %5, %6
    movdqa     %3, %1
    movdqa     %6, %4
    paddw      %1, %2
    paddw      %4, %5
    psubw      %3, %2
    psubw      %6, %5
%endmacro

%macro TRANSPOSE4x4_TWIST_SSE2 3    ; %1=(row3, row0) %2=(row2, row1) %3=junk, output in %1 and %2
    movdqa     %3, %1
    punpcklwd  %1, %2
    punpckhwd  %2, %3             ; backwards because the high quadwords are already swapped

    movdqa     %3, %1
    punpckldq  %1, %2
    punpckhdq  %3, %2

    movdqa     %2, %1
    punpcklqdq %1, %3
    punpckhqdq %2, %3
%endmacro

;;; two TRANSPOSE4x4_TWIST_SSE2 running side-by-side
%macro TRANSPOSE4x4_TWIST_TWO_SSE2 6    ; a02 a13 junk1 b02 b13 junk2 (1=4 2=5 3=6)
    movdqa     %3, %1
    movdqa     %6, %4
    punpcklwd  %1, %2
    punpcklwd  %4, %5
    punpckhwd  %2, %3
    punpckhwd  %5, %6
    movdqa     %3, %1
    movdqa     %6, %4
    punpckldq  %1, %2
    punpckldq  %4, %5
    punpckhdq  %3, %2
    punpckhdq  %6, %5
    movdqa     %2, %1
    movdqa     %5, %4
    punpcklqdq %1, %3
    punpcklqdq %4, %6
    punpckhqdq %2, %3
    punpckhqdq %5, %6
%endmacro

;;; loads the difference of two 4x4 blocks into xmm0,xmm1 and xmm4,xmm5 in interleaved-row order
;;; destroys xmm2, 3
;;; the value in xmm7 doesn't matter: it's only subtracted from itself
%macro LOAD4x8_DIFF_SSE2 0
    movq      xmm0, [eax]
    movq      xmm4, [ecx]
    punpcklbw xmm0, xmm7
    punpcklbw xmm4, xmm7
    psubw     xmm0, xmm4

    movq      xmm1, [eax+ebx]
    movq      xmm5, [ecx+edx]
    lea       eax,  [eax+2*ebx]
    lea       ecx,  [ecx+2*edx]
    punpcklbw xmm1, xmm7
    punpcklbw xmm5, xmm7
    psubw     xmm1, xmm5

    movq       xmm2, [eax]
    movq       xmm4, [ecx]
    punpcklbw  xmm2, xmm7
    punpcklbw  xmm4, xmm7
    psubw      xmm2, xmm4
    movdqa     xmm4, xmm0
    punpcklqdq xmm0, xmm2        ; rows 0 and 2
    punpckhqdq xmm4, xmm2        ; next 4x4 rows 0 and 2

    movq       xmm3, [eax+ebx]
    movq       xmm5, [ecx+edx]
    lea        eax,  [eax+2*ebx]
    lea        ecx,  [ecx+2*edx]
    punpcklbw  xmm3, xmm7
    punpcklbw  xmm5, xmm7
    psubw      xmm3, xmm5
    movdqa     xmm5, xmm1
    punpcklqdq xmm1, xmm3        ; rows 1 and 3
    punpckhqdq xmm5, xmm3        ; next 4x4 rows 1 and 3
%endmacro

%macro SUM4x4_SSE2 4    ; 02 13 junk sum
    pxor    %3, %3
    psubw   %3, %1
    pmaxsw  %1, %3

    pxor    %3, %3
    psubw   %3, %2
    pmaxsw  %2, %3

    paddusw %4, %1
    paddusw %4, %2
%endmacro

;;; two SUM4x4_SSE2 running side-by-side
%macro SUM4x4_TWO_SSE2 7    ; a02 a13 junk1 b02 b13 junk2 (1=4 2=5 3=6) sum
    pxor    %3, %3
    pxor    %6, %6
    psubw   %3, %1
    psubw   %6, %4
    pmaxsw  %1, %3
    pmaxsw  %4, %6
    pxor    %3, %3
    pxor    %6, %6
    psubw   %3, %2
    psubw   %6, %5
    pmaxsw  %2, %3
    pmaxsw  %5, %6
    paddusw %1, %2
    paddusw %4, %5
    paddusw %7, %1
    paddusw %7, %4
%endmacro

%macro SUM_MM_SSE2 2    ; sum junk
    ; each column sum of SATD is necessarily even, so we don't lose any precision by shifting first.
    psrlw   %1, 1
    movdqa  %2, %1
    psrldq  %1, 2
    paddusw %1, %2
    pand    %1, [pd_0000ffff]
    movdqa  %2, %1
    psrldq  %1, 4
    paddd   %1, %2
    movdqa  %2, %1
    psrldq  %1, 8
    paddd   %1, %2
    movd    eax,%1
%endmacro

%macro SATD_TWO_SSE2 0
    LOAD4x8_DIFF_SSE2
    HADAMARD4x4_TWO_SSE2        xmm0, xmm1, xmm2, xmm4, xmm5, xmm3
    TRANSPOSE4x4_TWIST_TWO_SSE2 xmm0, xmm2, xmm1, xmm4, xmm3, xmm5
    HADAMARD4x4_TWO_SSE2        xmm0, xmm2, xmm1, xmm4, xmm3, xmm5
    SUM4x4_TWO_SSE2             xmm0, xmm1, xmm2, xmm4, xmm5, xmm3, xmm6
%endmacro

%macro SATD_START 0
    push    ebx

    mov     eax,    [esp+ 8]    ; pix1
    mov     ebx,    [esp+12]    ; stride1
    mov     ecx,    [esp+16]    ; pix2
    mov     edx,    [esp+20]    ; stride2

    pxor    xmm6,    xmm6
%endmacro

%macro SATD_END 0
    SUM_MM_SSE2  xmm6, xmm7

    pop     ebx
    ret
%endmacro

ALIGN 16
;-----------------------------------------------------------------------------
;   int __cdecl x264_pixel_satd_16x16_sse2 (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
x264_pixel_satd_16x16_sse2:
    SATD_START

    SATD_TWO_SSE2
    SATD_TWO_SSE2
    SATD_TWO_SSE2
    SATD_TWO_SSE2

    mov     eax,    [esp+ 8]
    mov     ecx,    [esp+16]
    lea     eax,    [eax+8]
    lea     ecx,    [ecx+8]

    SATD_TWO_SSE2
    SATD_TWO_SSE2
    SATD_TWO_SSE2
    SATD_TWO_SSE2

    SATD_END

ALIGN 16
;-----------------------------------------------------------------------------
;   int __cdecl x264_pixel_satd_8x16_sse2 (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
x264_pixel_satd_8x16_sse2:
    SATD_START

    SATD_TWO_SSE2
    SATD_TWO_SSE2
    SATD_TWO_SSE2
    SATD_TWO_SSE2

    SATD_END

ALIGN 16
;-----------------------------------------------------------------------------
;   int __cdecl x264_pixel_satd_16x8_sse2 (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
x264_pixel_satd_16x8_sse2:
    SATD_START

    SATD_TWO_SSE2
    SATD_TWO_SSE2

    mov     eax,    [esp+ 8]
    mov     ecx,    [esp+16]
    lea     eax,    [eax+8]
    lea     ecx,    [ecx+8]

    SATD_TWO_SSE2
    SATD_TWO_SSE2

    SATD_END

ALIGN 16
;-----------------------------------------------------------------------------
;   int __cdecl x264_pixel_satd_8x8_sse2 (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
x264_pixel_satd_8x8_sse2:
    SATD_START

    SATD_TWO_SSE2
    SATD_TWO_SSE2

    SATD_END

ALIGN 16
;-----------------------------------------------------------------------------
;   int __cdecl x264_pixel_satd_8x4_sse2 (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
x264_pixel_satd_8x4_sse2:
    SATD_START

    SATD_TWO_SSE2

    SATD_END


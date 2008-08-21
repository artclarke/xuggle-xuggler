;*****************************************************************************
;* pixel-32.asm: h264 encoder library
;*****************************************************************************
;* Copyright (C) 2003-2008 x264 project
;*
;* Authors: Laurent Aimar <fenrir@via.ecp.fr>
;*          Loren Merritt <lorenm@u.washington.edu>
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
;* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111, USA.
;*****************************************************************************

%include "x86inc.asm"
%include "x86util.asm"

SECTION .text

%macro SBUTTERFLY 5
    mov%1     %5, %3
    punpckl%2 %3, %4
    punpckh%2 %5, %4
%endmacro

%macro TRANSPOSE4x4W 5   ; abcd-t -> adtc
    SBUTTERFLY q, wd, %1, %2, %5
    SBUTTERFLY q, wd, %3, %4, %2
    SBUTTERFLY q, dq, %1, %3, %4
    SBUTTERFLY q, dq, %5, %2, %3
%endmacro

%macro LOAD_DIFF_4P 4  ; mmp, mmt, dx, dy
    movd        %1, [eax+ebx*%4+%3]
    movd        %2, [ecx+edx*%4+%3]
    punpcklbw   %1, %2
    punpcklbw   %2, %2
    psubw       %1, %2
%endmacro

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

%macro SUM4x8_MM 0
    movq [spill],   mm6
    movq [spill+8], mm7
    ABS2     mm0, mm1, mm6, mm7
    ABS2     mm2, mm3, mm6, mm7
    paddw    mm0, mm2
    paddw    mm1, mm3
    movq     mm6, [spill]
    movq     mm7, [spill+8]
    ABS2     mm4, mm5, mm2, mm3
    ABS2     mm6, mm7, mm2, mm3
    paddw    mm4, mm6
    paddw    mm5, mm7
    paddw    mm0, mm4
    paddw    mm1, mm5
    paddw    mm0, mm1
%endmacro

;-----------------------------------------------------------------------------
; int x264_pixel_sa8d_8x8_mmxext( uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
cglobal x264_pixel_sa8d_8x8_mmxext
    push   ebx
    mov    eax, [esp+ 8]  ; pix1
    mov    ebx, [esp+12]  ; stride1
    mov    ecx, [esp+16]  ; pix2
    mov    edx, [esp+20]  ; stride2
    sub    esp, 0x70
%define args  esp+0x74
%define spill esp+0x60 ; +16
%define trans esp+0    ; +96
    LOAD_DIFF_4x8P 0
    HADAMARD8_1D mm0, mm1, mm2, mm3, mm4, mm5, mm6, mm7

    movq   [spill], mm0
    TRANSPOSE4x4W mm4, mm5, mm6, mm7, mm0
    movq   [trans+0x00], mm4
    movq   [trans+0x08], mm7
    movq   [trans+0x10], mm0
    movq   [trans+0x18], mm6
    movq   mm0, [spill]
    TRANSPOSE4x4W mm0, mm1, mm2, mm3, mm4
    movq   [trans+0x20], mm0
    movq   [trans+0x28], mm3
    movq   [trans+0x30], mm4
    movq   [trans+0x38], mm2

    mov    eax, [args+4]
    mov    ecx, [args+12]
    LOAD_DIFF_4x8P 4
    HADAMARD8_1D mm0, mm1, mm2, mm3, mm4, mm5, mm6, mm7

    movq   [spill], mm7
    TRANSPOSE4x4W mm0, mm1, mm2, mm3, mm7
    movq   [trans+0x40], mm0
    movq   [trans+0x48], mm3
    movq   [trans+0x50], mm7
    movq   [trans+0x58], mm2
    movq   mm7, [spill]
    TRANSPOSE4x4W mm4, mm5, mm6, mm7, mm0
    movq   mm5, [trans+0x00]
    movq   mm1, [trans+0x08]
    movq   mm2, [trans+0x10]
    movq   mm3, [trans+0x18]

    HADAMARD8_1D mm5, mm1, mm2, mm3, mm4, mm7, mm0, mm6
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

    HADAMARD8_1D mm0, mm1, mm2, mm3, mm4, mm5, mm6, mm7
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
; void x264_intra_sa8d_x3_8x8_core_mmxext( uint8_t *fenc, int16_t edges[2][8], int *res )
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
    HADAMARD8_1D mm0, mm1, mm2, mm3, mm4, mm5, mm6, mm7

    movq   [spill], mm0
    TRANSPOSE4x4W mm4, mm5, mm6, mm7, mm0
    movq   [trans+0x00], mm4
    movq   [trans+0x08], mm7
    movq   [trans+0x10], mm0
    movq   [trans+0x18], mm6
    movq   mm0, [spill]
    TRANSPOSE4x4W mm0, mm1, mm2, mm3, mm4
    movq   [trans+0x20], mm0
    movq   [trans+0x28], mm3
    movq   [trans+0x30], mm4
    movq   [trans+0x38], mm2

    LOAD_4x8P 4
    HADAMARD8_1D mm0, mm1, mm2, mm3, mm4, mm5, mm6, mm7

    movq   [spill], mm7
    TRANSPOSE4x4W mm0, mm1, mm2, mm3, mm7
    movq   [trans+0x40], mm0
    movq   [trans+0x48], mm3
    movq   [trans+0x50], mm7
    movq   [trans+0x58], mm2
    movq   mm7, [spill]
    TRANSPOSE4x4W mm4, mm5, mm6, mm7, mm0
    movq   mm5, [trans+0x00]
    movq   mm1, [trans+0x08]
    movq   mm2, [trans+0x10]
    movq   mm3, [trans+0x18]

    HADAMARD8_1D mm5, mm1, mm2, mm3, mm4, mm7, mm0, mm6

    movq [spill+0], mm5
    movq [spill+8], mm7
    ABS2     mm0, mm1, mm5, mm7
    ABS2     mm2, mm3, mm5, mm7
    paddw    mm0, mm2
    paddw    mm1, mm3
    paddw    mm0, mm1
    ABS2     mm4, mm6, mm2, mm3
    movq     mm5, [spill+0]
    movq     mm7, [spill+8]
    paddw    mm0, mm4
    paddw    mm0, mm6
    ABS1     mm7, mm1
    paddw    mm0, mm7 ; 7x4 sum
    movq     mm6, mm5
    movq     mm7, [ecx+8] ; left bottom
    psllw    mm7, 3
    psubw    mm6, mm7
    ABS2     mm5, mm6, mm2, mm3
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

    HADAMARD8_1D mm0, mm1, mm2, mm3, mm4, mm5, mm6, mm7

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
    ABS2     mm2, mm3, mm0, mm1
    ABS2     mm4, mm5, mm0, mm1
    paddw    mm2, mm3
    paddw    mm4, mm5
    paddw    mm2, mm4
    movq     mm0, [spill]
    movq     mm1, [spill+8]
    ABS2     mm6, mm7, mm4, mm5
    ABS1     mm1, mm4
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
    ABS2     mm0, mm1, mm5, mm6
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
    ABS2     mm3, mm4, mm5, mm6
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
.loop:
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


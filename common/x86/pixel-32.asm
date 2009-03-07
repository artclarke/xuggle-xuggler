;*****************************************************************************
;* pixel-32.asm: h264 encoder library
;*****************************************************************************
;* Copyright (C) 2003-2008 x264 project
;*
;* Authors: Loren Merritt <lorenm@u.washington.edu>
;*          Laurent Aimar <fenrir@via.ecp.fr>
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
INIT_MMX

%macro LOAD_DIFF_4x8P 1 ; dx
    LOAD_DIFF  m0, m7, none, [r0+%1],      [r2+%1]
    LOAD_DIFF  m1, m6, none, [r0+%1+r1],   [r2+%1+r3]
    LOAD_DIFF  m2, m7, none, [r0+%1+r1*2], [r2+%1+r3*2]
    LOAD_DIFF  m3, m6, none, [r0+%1+r4],   [r2+%1+r5]
    lea  r0, [r0+4*r1]
    lea  r2, [r2+4*r3]
    LOAD_DIFF  m4, m7, none, [r0+%1],      [r2+%1]
    LOAD_DIFF  m5, m6, none, [r0+%1+r1],   [r2+%1+r3]
    LOAD_DIFF  m6, m7, none, [r0+%1+r1*2], [r2+%1+r3*2]
    movq [spill], m5
    LOAD_DIFF  m7, m5, none, [r0+%1+r4],   [r2+%1+r5]
    movq m5, [spill]
%endmacro

%macro SUM4x8_MM 0
    movq [spill],   m6
    movq [spill+8], m7
    ABS2     m0, m1, m6, m7
    ABS2     m2, m3, m6, m7
    paddw    m0, m2
    paddw    m1, m3
    movq     m6, [spill]
    movq     m7, [spill+8]
    ABS2     m4, m5, m2, m3
    ABS2     m6, m7, m2, m3
    paddw    m4, m6
    paddw    m5, m7
    paddw    m0, m4
    paddw    m1, m5
    paddw    m0, m1
%endmacro

;-----------------------------------------------------------------------------
; int x264_pixel_sa8d_8x8_mmxext( uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
cglobal x264_pixel_sa8d_8x8_internal_mmxext
    push   r0
    push   r2
    sub    esp, 0x74
%define args  esp+0x74
%define spill esp+0x60 ; +16
%define trans esp+0    ; +96
    LOAD_DIFF_4x8P 0
    HADAMARD8_V m0, m1, m2, m3, m4, m5, m6, m7

    movq   [spill], m1
    TRANSPOSE4x4W 4, 5, 6, 7, 1
    movq   [trans+0x00], m4
    movq   [trans+0x08], m5
    movq   [trans+0x10], m6
    movq   [trans+0x18], m7
    movq   m1, [spill]
    TRANSPOSE4x4W 0, 1, 2, 3, 4
    movq   [trans+0x20], m0
    movq   [trans+0x28], m1
    movq   [trans+0x30], m2
    movq   [trans+0x38], m3

    mov    r0, [args+4]
    mov    r2, [args]
    LOAD_DIFF_4x8P 4
    HADAMARD8_V m0, m1, m2, m3, m4, m5, m6, m7

    movq   [spill], m7
    TRANSPOSE4x4W 0, 1, 2, 3, 7
    movq   [trans+0x40], m0
    movq   [trans+0x48], m1
    movq   [trans+0x50], m2
    movq   [trans+0x58], m3
    movq   m7, [spill]
    TRANSPOSE4x4W 4, 5, 6, 7, 1
    movq   m0, [trans+0x00]
    movq   m1, [trans+0x08]
    movq   m2, [trans+0x10]
    movq   m3, [trans+0x18]

    HADAMARD8_V m0, m1, m2, m3, m4, m5, m6, m7
    SUM4x8_MM
    movq   [trans], m0

    movq   m0, [trans+0x20]
    movq   m1, [trans+0x28]
    movq   m2, [trans+0x30]
    movq   m3, [trans+0x38]
    movq   m4, [trans+0x40]
    movq   m5, [trans+0x48]
    movq   m6, [trans+0x50]
    movq   m7, [trans+0x58]

    HADAMARD8_V m0, m1, m2, m3, m4, m5, m6, m7
    SUM4x8_MM

    pavgw  m0, [trans]
    add   esp, 0x7c
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
    pxor        m7, m7
    movd        m6, [eax+%1+7*FENC_STRIDE]
    movd        m0, [eax+%1+0*FENC_STRIDE]
    movd        m1, [eax+%1+1*FENC_STRIDE]
    movd        m2, [eax+%1+2*FENC_STRIDE]
    movd        m3, [eax+%1+3*FENC_STRIDE]
    movd        m4, [eax+%1+4*FENC_STRIDE]
    movd        m5, [eax+%1+5*FENC_STRIDE]
    punpcklbw   m6, m7
    punpcklbw   m0, m7
    punpcklbw   m1, m7
    movq   [spill], m6
    punpcklbw   m2, m7
    punpcklbw   m3, m7
    movd        m6, [eax+%1+6*FENC_STRIDE]
    punpcklbw   m4, m7
    punpcklbw   m5, m7
    punpcklbw   m6, m7
    movq        m7, [spill]
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
    HADAMARD8_V m0, m1, m2, m3, m4, m5, m6, m7

    movq   [spill], m0
    TRANSPOSE4x4W 4, 5, 6, 7, 0
    movq   [trans+0x00], m4
    movq   [trans+0x08], m5
    movq   [trans+0x10], m6
    movq   [trans+0x18], m7
    movq   m0, [spill]
    TRANSPOSE4x4W 0, 1, 2, 3, 4
    movq   [trans+0x20], m0
    movq   [trans+0x28], m1
    movq   [trans+0x30], m2
    movq   [trans+0x38], m3

    LOAD_4x8P 4
    HADAMARD8_V m0, m1, m2, m3, m4, m5, m6, m7

    movq   [spill], m7
    TRANSPOSE4x4W 0, 1, 2, 3, 7
    movq   [trans+0x40], m0
    movq   [trans+0x48], m1
    movq   [trans+0x50], m2
    movq   [trans+0x58], m3
    movq   m7, [spill]
    TRANSPOSE4x4W 4, 5, 6, 7, 0
    movq   m0, [trans+0x00]
    movq   m1, [trans+0x08]
    movq   m2, [trans+0x10]
    movq   m3, [trans+0x18]

    HADAMARD8_V m0, m1, m2, m3, m4, m5, m6, m7

    movq [spill+0], m0
    movq [spill+8], m1
    ABS2     m2, m3, m0, m1
    ABS2     m4, m5, m0, m1
    paddw    m2, m4
    paddw    m3, m5
    ABS2     m6, m7, m4, m5
    movq     m0, [spill+0]
    movq     m1, [spill+8]
    paddw    m2, m6
    paddw    m3, m7
    paddw    m2, m3
    ABS1     m1, m4
    paddw    m2, m1 ; 7x4 sum
    movq     m7, m0
    movq     m1, [ecx+8] ; left bottom
    psllw    m1, 3
    psubw    m7, m1
    ABS2     m0, m7, m5, m3
    paddw    m0, m2
    paddw    m7, m2
    movq [sum+0], m0 ; dc
    movq [sum+8], m7 ; left

    movq   m0, [trans+0x20]
    movq   m1, [trans+0x28]
    movq   m2, [trans+0x30]
    movq   m3, [trans+0x38]
    movq   m4, [trans+0x40]
    movq   m5, [trans+0x48]
    movq   m6, [trans+0x50]
    movq   m7, [trans+0x58]

    HADAMARD8_V m0, m1, m2, m3, m4, m5, m6, m7

    movd   [sum+0x10], m0
    movd   [sum+0x12], m1
    movd   [sum+0x14], m2
    movd   [sum+0x16], m3
    movd   [sum+0x18], m4
    movd   [sum+0x1a], m5
    movd   [sum+0x1c], m6
    movd   [sum+0x1e], m7

    movq [spill],   m0
    movq [spill+8], m1
    ABS2     m2, m3, m0, m1
    ABS2     m4, m5, m0, m1
    paddw    m2, m4
    paddw    m3, m5
    paddw    m2, m3
    movq     m0, [spill]
    movq     m1, [spill+8]
    ABS2     m6, m7, m4, m5
    ABS1     m1, m3
    paddw    m2, m7
    paddw    m1, m6
    paddw    m2, m1 ; 7x4 sum
    movq     m1, m0

    movq     m7, [ecx+0]
    psllw    m7, 3   ; left top

    movzx   edx, word [ecx+0]
    add      dx,  [ecx+16]
    lea     edx, [4*edx+32]
    and     edx, -64
    movd     m6, edx ; dc

    psubw    m1, m7
    psubw    m0, m6
    ABS2     m0, m1, m5, m6
    movq     m3, [sum+0] ; dc
    paddw    m0, m2
    paddw    m1, m2
    movq     m2, m0
    paddw    m0, m3
    paddw    m1, [sum+8] ; h
    psrlq    m2, 16
    paddw    m2, m3

    movq     m3, [ecx+16] ; top left
    movq     m4, [ecx+24] ; top right
    psllw    m3, 3
    psllw    m4, 3
    psubw    m3, [sum+16]
    psubw    m4, [sum+24]
    ABS2     m3, m4, m5, m6
    paddw    m2, m3
    paddw    m2, m4 ; v

    SUM_MM_X3 m0, m1, m2, m3, m4, m5, m6, paddd
    mov     eax, [args+8]
    movd    ecx, m2
    movd    edx, m1
    add     ecx, 2
    add     edx, 2
    shr     ecx, 2
    shr     edx, 2
    mov [eax+0], ecx ; i8x8_v satd
    mov [eax+4], edx ; i8x8_h satd
    movd    ecx, m0
    add     ecx, 2
    shr     ecx, 2
    mov [eax+8], ecx ; i8x8_dc satd

    add     esp, 0x70
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
    push     ebx
    push     edi
    mov      ebx, [esp+16]
    mov      edx, [esp+24]
    mov      edi, 4
    pxor      m0, m0
.loop:
    mov      eax, [esp+12]
    mov      ecx, [esp+20]
    add      eax, edi
    add      ecx, edi
    pxor      m1, m1
    pxor      m2, m2
    pxor      m3, m3
    pxor      m4, m4
%rep 4
    movd      m5, [eax]
    movd      m6, [ecx]
    punpcklbw m5, m0
    punpcklbw m6, m0
    paddw     m1, m5
    paddw     m2, m6
    movq      m7, m5
    pmaddwd   m5, m5
    pmaddwd   m7, m6
    pmaddwd   m6, m6
    paddd     m3, m5
    paddd     m4, m7
    paddd     m3, m6
    add      eax, ebx
    add      ecx, edx
%endrep
    mov      eax, [esp+28]
    lea      eax, [eax+edi*4]
    pshufw    m5, m1, 0xE
    pshufw    m6, m2, 0xE
    paddusw   m1, m5
    paddusw   m2, m6
    punpcklwd m1, m2
    pshufw    m2, m1, 0xE
    pshufw    m5, m3, 0xE
    pshufw    m6, m4, 0xE
    paddusw   m1, m2
    paddd     m3, m5
    paddd     m4, m6
    punpcklwd m1, m0
    punpckldq m3, m4
    movq [eax+0], m1
    movq [eax+8], m3
    sub      edi, 4
    jge .loop
    pop       edi
    pop       ebx
    emms
    ret


;*****************************************************************************
;* dct-32.asm: h264 encoder library
;*****************************************************************************
;* Copyright (C) 2003-2008 x264 project
;*
;* Authors: Loren Merritt <lorenm@u.washington.edu>
;*          Holger Lubitz <holger@lubitz.org>
;*          Laurent Aimar <fenrir@via.ecp.fr>
;*          Min Chen <chenm001.163.com>
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
;* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111, USA.
;*****************************************************************************

%include "x86inc.asm"
%include "x86util.asm"

SECTION_RODATA

pw_32: times 8 dw 32
hsub_mul: times 8 db 1, -1

SECTION .text

; in: m0..m7
; out: 0,4,6 in mem, rest in regs
%macro DCT8_1D 9
    SUMSUB_BA  m%8, m%1      ; %8 = s07, %1 = d07
    SUMSUB_BA  m%7, m%2      ; %7 = s16, %2 = d16
    SUMSUB_BA  m%6, m%3      ; %6 = s25, %3 = d25
    SUMSUB_BA  m%5, m%4      ; %5 = s34, %4 = d34
    SUMSUB_BA  m%5, m%8      ; %5 = a0,  %8 = a2
    SUMSUB_BA  m%6, m%7      ; %6 = a1,  %7 = a3
    SUMSUB_BA  m%6, m%5      ; %6 = dst0, %5 = dst4
    mova    [%9+0x00], m%6
    mova    [%9+0x40], m%5
    mova    m%6, m%7         ; a3
    psraw   m%6, 1           ; a3>>1
    paddw   m%6, m%8         ; a2 + (a3>>1)
    psraw   m%8, 1           ; a2>>1
    psubw   m%8, m%7         ; (a2>>1) - a3
    mova    [%9+0x60], m%8
    mova    m%5, m%3
    psraw   m%5, 1
    paddw   m%5, m%3         ; d25+(d25>>1)
    mova    m%7, m%1
    psubw   m%7, m%4         ; a5 = d07-d34-(d25+(d25>>1))
    psubw   m%7, m%5
    mova    m%5, m%2
    psraw   m%5, 1
    paddw   m%5, m%2         ; d16+(d16>>1)
    mova    m%8, m%1
    paddw   m%8, m%4
    psubw   m%8, m%5         ; a6 = d07+d34-(d16+(d16>>1))
    mova    m%5, m%1
    psraw   m%5, 1
    paddw   m%5, m%1         ; d07+(d07>>1)
    paddw   m%5, m%2
    paddw   m%5, m%3         ; a4 = d16+d25+(d07+(d07>>1))
    mova    m%1, m%4
    psraw   m%1, 1
    paddw   m%1, m%4         ; d34+(d34>>1)
    paddw   m%1, m%2
    psubw   m%1, m%3         ; a7 = d16-d25+(d34+(d34>>1))
    mova    m%4, m%1
    psraw   m%4, 2
    paddw   m%4, m%5         ; a4 + (a7>>2)
    mova    m%3, m%8
    psraw   m%3, 2
    paddw   m%3, m%7         ; a5 + (a6>>2)
    psraw   m%5, 2
    psraw   m%7, 2
    psubw   m%5, m%1         ; (a4>>2) - a7
    psubw   m%8, m%7         ; a6 - (a5>>2)
    SWAP %2, %4, %3, %6, %8, %5
%endmacro

; in: 0,4 in mem, rest in regs
; out: m0..m7
%macro IDCT8_1D 9
    mova      m%1, m%3
    mova      m%5, m%7
    psraw     m%3, 1
    psraw     m%7, 1
    psubw     m%3, m%5
    paddw     m%7, m%1
    mova      m%5, m%2
    psraw     m%5, 1
    paddw     m%5, m%2
    paddw     m%5, m%4
    paddw     m%5, m%6
    mova      m%1, m%6
    psraw     m%1, 1
    paddw     m%1, m%6
    paddw     m%1, m%8
    psubw     m%1, m%2
    psubw     m%2, m%4
    psubw     m%6, m%4
    paddw     m%2, m%8
    psubw     m%6, m%8
    psraw     m%4, 1
    psraw     m%8, 1
    psubw     m%2, m%4
    psubw     m%6, m%8
    mova      m%4, m%5
    mova      m%8, m%1
    psraw     m%4, 2
    psraw     m%8, 2
    paddw     m%4, m%6
    paddw     m%8, m%2
    psraw     m%6, 2
    psraw     m%2, 2
    psubw     m%5, m%6
    psubw     m%2, m%1
    mova      m%1, [%9+0x00]
    mova      m%6, [%9+0x40]
    SUMSUB_BA m%6, m%1
    SUMSUB_BA m%7, m%6
    SUMSUB_BA m%3, m%1
    SUMSUB_BA m%5, m%7
    SUMSUB_BA m%2, m%3
    SUMSUB_BA m%8, m%1
    SUMSUB_BA m%4, m%6
    SWAP %1, %5, %6
    SWAP %3, %8, %7
%endmacro

INIT_MMX
ALIGN 16
load_diff_4x8_mmx:
    LOAD_DIFF m0, m7, none, [r1+0*FENC_STRIDE], [r2+0*FDEC_STRIDE]
    LOAD_DIFF m1, m7, none, [r1+1*FENC_STRIDE], [r2+1*FDEC_STRIDE]
    LOAD_DIFF m2, m7, none, [r1+2*FENC_STRIDE], [r2+2*FDEC_STRIDE]
    LOAD_DIFF m3, m7, none, [r1+3*FENC_STRIDE], [r2+3*FDEC_STRIDE]
    LOAD_DIFF m4, m7, none, [r1+4*FENC_STRIDE], [r2+4*FDEC_STRIDE]
    LOAD_DIFF m5, m7, none, [r1+5*FENC_STRIDE], [r2+5*FDEC_STRIDE]
    movq  [r0], m0
    LOAD_DIFF m6, m7, none, [r1+6*FENC_STRIDE], [r2+6*FDEC_STRIDE]
    LOAD_DIFF m7, m0, none, [r1+7*FENC_STRIDE], [r2+7*FDEC_STRIDE]
    movq  m0, [r0]
    ret

INIT_MMX
ALIGN 16
dct8_mmx:
    DCT8_1D 0,1,2,3,4,5,6,7,r0
    SAVE_MM_PERMUTATION dct8_mmx
    ret

%macro SPILL_SHUFFLE 3-* ; ptr, list of regs, list of memory offsets
    %xdefine %%base %1
    %rep %0/2
    %xdefine %%tmp m%2
    %rotate %0/2
    mova [%%base + %2*16], %%tmp
    %rotate 1-%0/2
    %endrep
%endmacro

%macro UNSPILL_SHUFFLE 3-*
    %xdefine %%base %1
    %rep %0/2
    %xdefine %%tmp m%2
    %rotate %0/2
    mova %%tmp, [%%base + %2*16]
    %rotate 1-%0/2
    %endrep
%endmacro

%macro SPILL 2+ ; assume offsets are the same as reg numbers
    SPILL_SHUFFLE %1, %2, %2
%endmacro

%macro UNSPILL 2+
    UNSPILL_SHUFFLE %1, %2, %2
%endmacro

;-----------------------------------------------------------------------------
; void x264_sub8x8_dct8_mmx( int16_t dct[8][8], uint8_t *pix1, uint8_t *pix2 )
;-----------------------------------------------------------------------------
cglobal x264_sub8x8_dct8_mmx, 3,3
global x264_sub8x8_dct8_mmx.skip_prologue
.skip_prologue:
    INIT_MMX
    call load_diff_4x8_mmx
    call dct8_mmx
    UNSPILL r0, 0
    TRANSPOSE4x4W 0,1,2,3,4
    SPILL r0, 0,1,2,3
    UNSPILL r0, 4,6
    TRANSPOSE4x4W 4,5,6,7,0
    SPILL r0, 4,5,6,7
    INIT_MMX
    add   r1, 4
    add   r2, 4
    add   r0, 8
    call load_diff_4x8_mmx
    sub   r1, 4
    sub   r2, 4
    call dct8_mmx
    sub   r0, 8
    UNSPILL r0+8, 4,6
    TRANSPOSE4x4W 4,5,6,7,0
    SPILL r0+8, 4,5,6,7
    UNSPILL r0+8, 0
    TRANSPOSE4x4W 0,1,2,3,5
    UNSPILL r0, 4,5,6,7
    SPILL_SHUFFLE r0, 0,1,2,3, 4,5,6,7
    movq  mm4, m6 ; depends on the permutation to not produce conflicts
    movq  mm0, m4
    movq  mm1, m5
    movq  mm2, mm4
    movq  mm3, m7
    INIT_MMX
    UNSPILL r0+8, 4,5,6,7
    add   r0, 8
    call dct8_mmx
    sub   r0, 8
    SPILL r0+8, 1,2,3,5,7
    INIT_MMX
    UNSPILL r0, 0,1,2,3,4,5,6,7
    call dct8_mmx
    SPILL r0, 1,2,3,5,7
    ret

INIT_MMX
ALIGN 16
idct8_mmx:
    IDCT8_1D 0,1,2,3,4,5,6,7,r1
    SAVE_MM_PERMUTATION idct8_mmx
    ret

%macro ADD_STORE_ROW 3
    movq  m1, [r0+%1*FDEC_STRIDE]
    movq  m2, m1
    punpcklbw m1, m0
    punpckhbw m2, m0
    paddw m1, %2
    paddw m2, %3
    packuswb m1, m2
    movq  [r0+%1*FDEC_STRIDE], m1
%endmacro

;-----------------------------------------------------------------------------
; void x264_add8x8_idct8_mmx( uint8_t *dst, int16_t dct[8][8] )
;-----------------------------------------------------------------------------
cglobal x264_add8x8_idct8_mmx, 2,2
global x264_add8x8_idct8_mmx.skip_prologue
.skip_prologue:
    INIT_MMX
    add word [r1], 32
    UNSPILL r1, 1,2,3,5,6,7
    call idct8_mmx
    SPILL r1, 7
    TRANSPOSE4x4W 0,1,2,3,7
    SPILL r1, 0,1,2,3
    UNSPILL r1, 7
    TRANSPOSE4x4W 4,5,6,7,0
    SPILL r1, 4,5,6,7
    INIT_MMX
    UNSPILL r1+8, 1,2,3,5,6,7
    add r1, 8
    call idct8_mmx
    sub r1, 8
    SPILL r1+8, 7
    TRANSPOSE4x4W 0,1,2,3,7
    SPILL r1+8, 0,1,2,3
    UNSPILL r1+8, 7
    TRANSPOSE4x4W 4,5,6,7,0
    SPILL r1+8, 4,5,6,7
    INIT_MMX
    movq  m3, [r1+0x08]
    movq  m0, [r1+0x40]
    movq  [r1+0x40], m3
    movq  [r1+0x08], m0
    ; memory layout at this time:
    ; A0------ A1------
    ; B0------ F0------
    ; C0------ G0------
    ; D0------ H0------
    ; E0------ E1------
    ; B1------ F1------
    ; C1------ G1------
    ; D1------ H1------
    UNSPILL_SHUFFLE r1, 1,2,3, 5,6,7
    UNSPILL r1+8, 5,6,7
    add r1, 8
    call idct8_mmx
    sub r1, 8
    psraw m0, 6
    psraw m1, 6
    psraw m2, 6
    psraw m3, 6
    psraw m4, 6
    psraw m5, 6
    psraw m6, 6
    psraw m7, 6
    movq  [r1+0x08], m0 ; mm4
    movq  [r1+0x48], m4 ; mm5
    movq  [r1+0x58], m5 ; mm0
    movq  [r1+0x68], m6 ; mm2
    movq  [r1+0x78], m7 ; mm6
    movq  mm5, [r1+0x18]
    movq  mm6, [r1+0x28]
    movq  [r1+0x18], m1 ; mm1
    movq  [r1+0x28], m2 ; mm7
    movq  mm7, [r1+0x38]
    movq  [r1+0x38], m3 ; mm3
    movq  mm1, [r1+0x10]
    movq  mm2, [r1+0x20]
    movq  mm3, [r1+0x30]
    call idct8_mmx
    psraw m0, 6
    psraw m1, 6
    psraw m2, 6
    psraw m3, 6
    psraw m4, 6
    psraw m5, 6
    psraw m6, 6
    psraw m7, 6
    SPILL r1, 0,1,2
    pxor  m0, m0
    ADD_STORE_ROW 0, [r1+0x00], [r1+0x08]
    ADD_STORE_ROW 1, [r1+0x10], [r1+0x18]
    ADD_STORE_ROW 2, [r1+0x20], [r1+0x28]
    ADD_STORE_ROW 3, m3, [r1+0x38]
    ADD_STORE_ROW 4, m4, [r1+0x48]
    ADD_STORE_ROW 5, m5, [r1+0x58]
    ADD_STORE_ROW 6, m6, [r1+0x68]
    ADD_STORE_ROW 7, m7, [r1+0x78]
    ret

INIT_XMM
%macro DCT_SUB8 1
cglobal x264_sub8x8_dct_%1, 3,3
    add r2, 4*FDEC_STRIDE
global x264_sub8x8_dct_%1.skip_prologue
.skip_prologue:
%ifnidn %1, sse2
    mova m7, [hsub_mul GLOBAL]
%endif
    LOAD_DIFF8x4 0, 1, 2, 3, 6, 7, r1, r2-4*FDEC_STRIDE
    SPILL r0, 1,2
    SWAP 2, 7
    LOAD_DIFF8x4 4, 5, 6, 7, 1, 2, r1, r2-4*FDEC_STRIDE
    UNSPILL r0, 1
    SPILL r0, 7
    SWAP 2, 7
    UNSPILL r0, 2
    DCT4_1D 0, 1, 2, 3, 7
    TRANSPOSE2x4x4W 0, 1, 2, 3, 7
    UNSPILL r0, 7
    SPILL r0, 2
    DCT4_1D 4, 5, 6, 7, 2
    TRANSPOSE2x4x4W 4, 5, 6, 7, 2
    UNSPILL r0, 2
    SPILL r0, 6
    DCT4_1D 0, 1, 2, 3, 6
    UNSPILL r0, 6
    STORE_DCT 0, 1, 2, 3, r0, 0
    DCT4_1D 4, 5, 6, 7, 3
    STORE_DCT 4, 5, 6, 7, r0, 64
    ret

;-----------------------------------------------------------------------------
; void x264_sub8x8_dct8_sse2( int16_t dct[8][8], uint8_t *pix1, uint8_t *pix2 )
;-----------------------------------------------------------------------------
cglobal x264_sub8x8_dct8_%1, 3,3
    add r2, 4*FDEC_STRIDE
global x264_sub8x8_dct8_%1.skip_prologue
.skip_prologue:
%ifidn %1, sse2
    LOAD_DIFF m0, m7, none, [r1+0*FENC_STRIDE], [r2-4*FDEC_STRIDE]
    LOAD_DIFF m1, m7, none, [r1+1*FENC_STRIDE], [r2-3*FDEC_STRIDE]
    LOAD_DIFF m2, m7, none, [r1+2*FENC_STRIDE], [r2-2*FDEC_STRIDE]
    LOAD_DIFF m3, m7, none, [r1+3*FENC_STRIDE], [r2-1*FDEC_STRIDE]
    LOAD_DIFF m4, m7, none, [r1+4*FENC_STRIDE], [r2+0*FDEC_STRIDE]
    LOAD_DIFF m5, m7, none, [r1+5*FENC_STRIDE], [r2+1*FDEC_STRIDE]
    SPILL r0, 0
    LOAD_DIFF m6, m7, none, [r1+6*FENC_STRIDE], [r2+2*FDEC_STRIDE]
    LOAD_DIFF m7, m0, none, [r1+7*FENC_STRIDE], [r2+3*FDEC_STRIDE]
    UNSPILL r0, 0
%else
    mova m7, [hsub_mul GLOBAL]
    LOAD_DIFF8x4 0, 1, 2, 3, 4, 7, r1, r2-4*FDEC_STRIDE
    SPILL r0, 0,1
    SWAP 1, 7
    LOAD_DIFF8x4 4, 5, 6, 7, 0, 1, r1, r2-4*FDEC_STRIDE
    UNSPILL r0, 0,1
%endif
    DCT8_1D 0,1,2,3,4,5,6,7,r0
    UNSPILL r0, 0,4
    TRANSPOSE8x8W 0,1,2,3,4,5,6,7,[r0+0x60],[r0+0x40],1
    UNSPILL r0, 4
    DCT8_1D 0,1,2,3,4,5,6,7,r0
    SPILL r0, 1,2,3,5,7
    ret
%endmacro

%define LOAD_DIFF8x4 LOAD_DIFF8x4_SSE2
%define movdqa movaps
%define punpcklqdq movlhps
DCT_SUB8 sse2
%undef movdqa
%undef punpcklqdq
%define LOAD_DIFF8x4 LOAD_DIFF8x4_SSSE3
DCT_SUB8 ssse3

;-----------------------------------------------------------------------------
; void x264_add8x8_idct_sse2( uint8_t *pix, int16_t dct[4][4][4] )
;-----------------------------------------------------------------------------
cglobal x264_add8x8_idct_sse2, 2,2
    add r0, 4*FDEC_STRIDE
global x264_add8x8_idct_sse2.skip_prologue
.skip_prologue:
    UNSPILL_SHUFFLE r1, 0,2,1,3, 0,1,2,3
    SBUTTERFLY qdq, 0, 1, 4
    SBUTTERFLY qdq, 2, 3, 4
    UNSPILL_SHUFFLE r1, 4,6,5,7, 4,5,6,7
    SPILL r1, 0
    SBUTTERFLY qdq, 4, 5, 0
    SBUTTERFLY qdq, 6, 7, 0
    UNSPILL r1,0
    IDCT4_1D 0,1,2,3,r1
    SPILL r1, 4
    TRANSPOSE2x4x4W 0,1,2,3,4
    UNSPILL r1, 4
    IDCT4_1D 4,5,6,7,r1
    SPILL r1, 0
    TRANSPOSE2x4x4W 4,5,6,7,0
    UNSPILL r1, 0
    paddw m0, [pw_32 GLOBAL]
    IDCT4_1D 0,1,2,3,r1
    paddw m4, [pw_32 GLOBAL]
    IDCT4_1D 4,5,6,7,r1
    SPILL r1, 6,7
    pxor m7, m7
    DIFFx2 m0, m1, m6, m7, [r0-4*FDEC_STRIDE], [r0-3*FDEC_STRIDE]; m5
    DIFFx2 m2, m3, m6, m7, [r0-2*FDEC_STRIDE], [r0-1*FDEC_STRIDE]; m5
    UNSPILL_SHUFFLE r1, 0,2, 6,7
    DIFFx2 m4, m5, m6, m7, [r0+0*FDEC_STRIDE], [r0+1*FDEC_STRIDE]; m5
    DIFFx2 m0, m2, m6, m7, [r0+2*FDEC_STRIDE], [r0+3*FDEC_STRIDE]; m5
    STORE_IDCT m1, m3, m5, m2
    ret

;-----------------------------------------------------------------------------
; void x264_add8x8_idct8_sse2( uint8_t *p_dst, int16_t dct[8][8] )
;-----------------------------------------------------------------------------
cglobal x264_add8x8_idct8_sse2, 2,2
    add r0, 4*FDEC_STRIDE
global x264_add8x8_idct8_sse2.skip_prologue
.skip_prologue:
    UNSPILL r1, 1,2,3,5,6,7
    IDCT8_1D   0,1,2,3,4,5,6,7,r1
    SPILL r1, 6
    TRANSPOSE8x8W 0,1,2,3,4,5,6,7,[r1+0x60],[r1+0x40],1
    paddw      m0, [pw_32 GLOBAL]
    SPILL r1, 0
    IDCT8_1D   0,1,2,3,4,5,6,7,r1
    SPILL r1, 6,7
    pxor       m7, m7
    DIFFx2 m0, m1, m6, m7, [r0-4*FDEC_STRIDE], [r0-3*FDEC_STRIDE]; m5
    DIFFx2 m2, m3, m6, m7, [r0-2*FDEC_STRIDE], [r0-1*FDEC_STRIDE]; m5
    UNSPILL_SHUFFLE r1, 0,2, 6,7
    DIFFx2 m4, m5, m6, m7, [r0+0*FDEC_STRIDE], [r0+1*FDEC_STRIDE]; m5
    DIFFx2 m0, m2, m6, m7, [r0+2*FDEC_STRIDE], [r0+3*FDEC_STRIDE]; m5
    STORE_IDCT m1, m3, m5, m2
    ret

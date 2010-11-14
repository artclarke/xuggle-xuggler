;*****************************************************************************
;* quant-a.asm: x86 quantization and level-run
;*****************************************************************************
;* Copyright (C) 2005-2010 x264 project
;*
;* Authors: Loren Merritt <lorenm@u.washington.edu>
;*          Jason Garrett-Glaser <darkshikari@gmail.com>
;*          Christian Heine <sennindemokrit@gmx.net>
;*          Oskar Arvidsson <oskar@irock.se>
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
;*
;* This program is also available under a commercial proprietary license.
;* For more information, contact us at licensing@x264.com.
;*****************************************************************************

%include "x86inc.asm"
%include "x86util.asm"

SECTION_RODATA

%macro DQM4 3
    dw %1, %2, %1, %2, %2, %3, %2, %3
%endmacro
%macro DQM8 6
    dw %1, %4, %5, %4, %1, %4, %5, %4
    dw %4, %2, %6, %2, %4, %2, %6, %2
    dw %5, %6, %3, %6, %5, %6, %3, %6
    ; last line not used, just padding for power-of-2 stride
    times 8 dw 0
%endmacro

dequant4_scale:
    DQM4 10, 13, 16
    DQM4 11, 14, 18
    DQM4 13, 16, 20
    DQM4 14, 18, 23
    DQM4 16, 20, 25
    DQM4 18, 23, 29

dequant8_scale:
    DQM8 20, 18, 32, 19, 25, 24
    DQM8 22, 19, 35, 21, 28, 26
    DQM8 26, 23, 42, 24, 33, 31
    DQM8 28, 25, 45, 26, 35, 33
    DQM8 32, 28, 51, 30, 40, 38
    DQM8 36, 32, 58, 34, 46, 43

decimate_mask_table4:
    db  0,3,2,6,2,5,5,9,1,5,4,8,5,8,8,12,1,4,4,8,4,7,7,11,4,8,7,11,8,11,11,15,1,4
    db  3,7,4,7,7,11,3,7,6,10,7,10,10,14,4,7,7,11,7,10,10,14,7,11,10,14,11,14,14
    db 18,0,4,3,7,3,6,6,10,3,7,6,10,7,10,10,14,3,6,6,10,6,9,9,13,6,10,9,13,10,13
    db 13,17,4,7,6,10,7,10,10,14,6,10,9,13,10,13,13,17,7,10,10,14,10,13,13,17,10
    db 14,13,17,14,17,17,21,0,3,3,7,3,6,6,10,2,6,5,9,6,9,9,13,3,6,6,10,6,9,9,13
    db  6,10,9,13,10,13,13,17,3,6,5,9,6,9,9,13,5,9,8,12,9,12,12,16,6,9,9,13,9,12
    db 12,16,9,13,12,16,13,16,16,20,3,7,6,10,6,9,9,13,6,10,9,13,10,13,13,17,6,9
    db  9,13,9,12,12,16,9,13,12,16,13,16,16,20,7,10,9,13,10,13,13,17,9,13,12,16
    db 13,16,16,20,10,13,13,17,13,16,16,20,13,17,16,20,17,20,20,24

SECTION .text

cextern pb_1
cextern pw_1
cextern pd_1
cextern pb_01

%macro QUANT_DC_START_MMX 0
    movd       m6, r1m     ; mf
    movd       m7, r2m     ; bias
%ifdef HIGH_BIT_DEPTH
    SPLATD     m6, m6
    SPLATD     m7, m7
%else
    SPLATW     m6, m6
    SPLATW     m7, m7
%endif ; HIGH_BIT_DEPTH
%endmacro

%macro QUANT_DC_START_SSSE3 0
    movdqa     m5, [pb_01]
    movd       m6, r1m     ; mf
    movd       m7, r2m     ; bias
    pshufb     m6, m5
    pshufb     m7, m5
%endmacro

%macro PABSW_MMX 2
    pxor       %1, %1
    pcmpgtw    %1, %2
    pxor       %2, %1
    psubw      %2, %1
    SWAP       %1, %2
%endmacro

%macro PSIGNW_MMX 2
    pxor       %1, %2
    psubw      %1, %2
%endmacro

%macro PABSW_SSSE3 2
    pabsw      %1, %2
%endmacro

%macro PSIGNW_SSSE3 2
    psignw     %1, %2
%endmacro

%macro PSIGND_MMX 2
    pxor        %1, %2
    psubd       %1, %2
%endmacro

%macro PSIGND_SSSE3 2
    psignd      %1, %2
%endmacro

%macro PABSD_MMX 2
    pxor        %1, %1
    pcmpgtd     %1, %2
    pxor        %2, %1
    psubd       %2, %1
    SWAP        %1, %2
%endmacro

%macro PABSD_SSSE3 2
    pabsd       %1, %2
%endmacro

%macro QUANT_END_MMX 0
    xor      eax, eax
%ifdef ARCH_X86_64
%if mmsize == 16
    packsswb  m5, m5
%endif
    movq     rcx, m5
    test     rcx, rcx
%else
%if mmsize == 16
    pxor      m4, m4
    pcmpeqb   m5, m4
    pmovmskb ecx, m5
    cmp      ecx, (1<<mmsize)-1
%else
    packsswb  m5, m5
    movd     ecx, m5
    test     ecx, ecx
%endif
%endif
    setne     al
%endmacro

%macro QUANT_END_SSE4 0
    xor      eax, eax
    ptest     m5, m5
    setne     al
%endmacro

%ifdef HIGH_BIT_DEPTH
%macro QUANT_ONE_DC_MMX 4
    mova        m0, [%1]
    PABSD       m1, m0
    paddd       m1, %3
    mova        m2, m1
    psrlq       m2, 32
    pmuludq     m1, %2
    pmuludq     m2, %2
    psllq       m2, 32
    paddd       m1, m2
    psrld       m1, 16
    PSIGND      m1, m0
    mova      [%1], m1
%if %4
    por         m5, m1
%else
    SWAP        m5, m1
%endif
%endmacro

%macro QUANT_TWO_DC_MMX 4
    QUANT_ONE_DC_MMX %1, %2, %3, %4
    QUANT_ONE_DC_MMX %1+mmsize, %2, %3, %4+mmsize
%endmacro

%macro QUANT_ONE_DC_SSE4 4
    mova        m0, [%1]
    PABSD       m1, m0
    paddd       m1, %3
    pmulld      m1, %2
    psrad       m1, 16
    PSIGND      m1, m0
    mova      [%1], m1
%if %4
    por         m5, m1
%else
    SWAP        m5, m1
%endif
%endmacro

%macro QUANT_TWO_DC_SSE4 4
    mova        m0, [%1]
    mova        m1, [%1+mmsize]
    PABSD       m2, m0
    PABSD       m3, m1
    paddd       m2, %3
    paddd       m3, %3
    pmulld      m2, %2
    pmulld      m3, %2
    psrad       m2, 16
    psrad       m3, 16
    PSIGND      m2, m0
    PSIGND      m3, m1
    mova      [%1], m2
    mova      [%1+mmsize], m3
%if %4
    por         m5, m2
%else
    SWAP        m5, m2
%endif
    por         m5, m3
%endmacro

%macro QUANT_ONE_AC_MMX 4
    mova        m0, [%1]
    mova        m2, [%2]
    PABSD       m1, m0
    mova        m4, m2
    paddd       m1, [%3]
    mova        m3, m1
    psrlq       m4, 32
    psrlq       m3, 32
    pmuludq     m1, m2
    pmuludq     m3, m4
    psllq       m3, 32
    paddd       m1, m3
    psrad       m1, 16
    PSIGND      m1, m0
    mova      [%1], m1
%if %4
    por         m5, m1
%else
    SWAP        m5, m1
%endif
%endmacro

%macro QUANT_TWO_AC_MMX 4
    QUANT_ONE_AC_MMX %1, %2, %3, %4
    QUANT_ONE_AC_MMX %1+mmsize, %2+mmsize, %3+mmsize, %4+mmsize
%endmacro

%macro QUANT_TWO_AC_SSE4 4
    mova        m0, [%1]
    mova        m1, [%1+mmsize]
    PABSD       m2, m0
    PABSD       m3, m1
    paddd       m2, [%3]
    paddd       m3, [%3+mmsize]
    pmulld      m2, [%2]
    pmulld      m3, [%2+mmsize]
    psrad       m2, 16
    psrad       m3, 16
    PSIGND      m2, m0
    PSIGND      m3, m1
    mova      [%1], m2
    mova      [%1+mmsize], m3
%if %4
    por         m5, m2
%else
    SWAP        m5, m2
%endif
    por         m5, m3
%endmacro

;-----------------------------------------------------------------------------
; int quant_2x2( int32_t dct[M*N], int mf, int bias )
;-----------------------------------------------------------------------------
%macro QUANT_DC 3
cglobal quant_%1x%2_dc_%3, 3,3,8*(mmsize/16)
    QUANT_DC_START_MMX
%if %1*%2 <= mmsize/4
    QUANT_ONE_DC r0, m6, m7, 0
%else
%assign x 0
%rep %1*%2/(mmsize/2)
    QUANT_TWO_DC r0+x, m6, m7, x
%assign x x+mmsize*2
%endrep
%endif
    QUANT_END
    RET
%endmacro

;-----------------------------------------------------------------------------
; int quant_MxN( int32_t dct[M*N], uint32_t mf[M*N], uint32_t bias[M*N] )
;-----------------------------------------------------------------------------
%macro QUANT_AC 3
cglobal quant_%1x%2_%3, 3,3,8*(mmsize/16)
%assign x 0
%rep %1*%2/(mmsize/2)
    QUANT_TWO_AC r0+x, r1+x, r2+x, x
%assign x x+mmsize*2
%endrep
    QUANT_END
    RET
%endmacro

%define QUANT_TWO_AC QUANT_TWO_AC_MMX
%define QUANT_ONE_DC QUANT_ONE_DC_MMX
%define QUANT_TWO_DC QUANT_TWO_DC_MMX
%define QUANT_END QUANT_END_MMX
%define PABSD PABSD_MMX
%define PSIGND PSIGND_MMX
INIT_MMX
QUANT_DC 2, 2, mmxext
QUANT_DC 4, 4, mmxext
QUANT_AC 4, 4, mmx
QUANT_AC 8, 8, mmx
INIT_XMM
QUANT_DC 2, 2, sse2
QUANT_DC 4, 4, sse2
QUANT_AC 4, 4, sse2
QUANT_AC 8, 8, sse2

%define PABSD PABSD_SSSE3
%define PSIGND PSIGND_SSSE3
QUANT_DC 2, 2, ssse3
QUANT_DC 4, 4, ssse3
QUANT_AC 4, 4, ssse3
QUANT_AC 8, 8, ssse3

%define QUANT_TWO_AC QUANT_TWO_AC_SSE4
%define QUANT_ONE_DC QUANT_ONE_DC_SSE4
%define QUANT_TWO_DC QUANT_TWO_DC_SSE4
%define QUANT_END QUANT_END_SSE4
QUANT_DC 2, 2, sse4
QUANT_DC 4, 4, sse4
QUANT_AC 4, 4, sse4
QUANT_AC 8, 8, sse4

%undef SIGND
%undef PABSD
%undef QUANT_END
%undef QUANT_TWO_AC
%undef QUANT_ONE_DC
%undef QUANT_TWO_DC
%endif ; HIGH_BIT_DEPTH

%ifndef HIGH_BIT_DEPTH
%macro QUANT_ONE 4
;;; %1      (m64)       dct[y][x]
;;; %2      (m64/mmx)   mf[y][x] or mf[0][0] (as uint16_t)
;;; %3      (m64/mmx)   bias[y][x] or bias[0][0] (as uint16_t)
    mova       m1, %1   ; load dct coeffs
    PABSW      m0, m1
    paddusw    m0, %3   ; round
    pmulhuw    m0, %2   ; divide
    PSIGNW     m0, m1   ; restore sign
    mova       %1, m0   ; store
%if %4
    por        m5, m0
%else
    SWAP       m5, m0
%endif
%endmacro

%macro QUANT_TWO 7
    mova       m1, %1
    mova       m3, %2
    PABSW      m0, m1
    PABSW      m2, m3
    paddusw    m0, %5
    paddusw    m2, %6
    pmulhuw    m0, %3
    pmulhuw    m2, %4
    PSIGNW     m0, m1
    PSIGNW     m2, m3
    mova       %1, m0
    mova       %2, m2
%if %7
    por        m5, m0
    por        m5, m2
%else
    SWAP       m5, m0
    por        m5, m2
%endif
%endmacro

;-----------------------------------------------------------------------------
; void quant_4x4_dc( int16_t dct[16], int mf, int bias )
;-----------------------------------------------------------------------------
%macro QUANT_DC 2-3 0
cglobal %1, 1,1,%3
    QUANT_DC_START
%if %2==1
    QUANT_ONE [r0], m6, m7, 0
%else
%assign x 0
%rep %2/2
    QUANT_TWO [r0+x], [r0+x+mmsize], m6, m6, m7, m7, x
%assign x x+mmsize*2
%endrep
%endif
    QUANT_END
    RET
%endmacro

;-----------------------------------------------------------------------------
; int quant_4x4( int16_t dct[16], uint16_t mf[16], uint16_t bias[16] )
;-----------------------------------------------------------------------------
%macro QUANT_AC 2
cglobal %1, 3,3
%assign x 0
%rep %2/2
    QUANT_TWO [r0+x], [r0+x+mmsize], [r1+x], [r1+x+mmsize], [r2+x], [r2+x+mmsize], x
%assign x x+mmsize*2
%endrep
    QUANT_END
    RET
%endmacro

INIT_MMX
%define QUANT_END QUANT_END_MMX
%define PABSW PABSW_MMX
%define PSIGNW PSIGNW_MMX
%define QUANT_DC_START QUANT_DC_START_MMX
QUANT_DC quant_2x2_dc_mmxext, 1
%ifndef ARCH_X86_64 ; not needed because sse2 is faster
QUANT_DC quant_4x4_dc_mmxext, 4
QUANT_AC quant_4x4_mmx, 4
QUANT_AC quant_8x8_mmx, 16
%endif

INIT_XMM
QUANT_DC quant_4x4_dc_sse2, 2, 8
QUANT_AC quant_4x4_sse2, 2
QUANT_AC quant_8x8_sse2, 8

%define PABSW PABSW_SSSE3
%define PSIGNW PSIGNW_SSSE3
QUANT_DC quant_4x4_dc_ssse3, 2, 8
QUANT_AC quant_4x4_ssse3, 2
QUANT_AC quant_8x8_ssse3, 8

INIT_MMX
QUANT_DC quant_2x2_dc_ssse3, 1
%define QUANT_END QUANT_END_SSE4
;Not faster on Conroe, so only used in SSE4 versions
%define QUANT_DC_START QUANT_DC_START_SSSE3
INIT_XMM
QUANT_DC quant_4x4_dc_sse4, 2, 8
QUANT_AC quant_4x4_sse4, 2
QUANT_AC quant_8x8_sse4, 8
%endif ; !HIGH_BIT_DEPTH



;=============================================================================
; dequant
;=============================================================================

%macro DEQUANT16_L 3
;;; %1      dct[y][x]
;;; %2,%3   dequant_mf[i_mf][y][x]
;;; m2      i_qbits

    mova     m0, %2
    packssdw m0, %3
    pmullw   m0, %1
    psllw    m0, m2
    mova     %1, m0
%endmacro

%macro DEQUANT32_R 3
;;; %1      dct[y][x]
;;; %2,%3   dequant_mf[i_mf][y][x]
;;; m2      -i_qbits
;;; m3      f
;;; m4      0

    mova      m0, %1
    mova      m1, m0
    punpcklwd m0, m4
    punpckhwd m1, m4
    pmaddwd   m0, %2
    pmaddwd   m1, %3
    paddd     m0, m3
    paddd     m1, m3
    psrad     m0, m2
    psrad     m1, m2
    packssdw  m0, m1
    mova      %1, m0
%endmacro

%macro DEQUANT_LOOP 3
%if 8*(%2-2*%3)
    mov t0d, 8*(%2-2*%3)
%%loop:
    %1 [r0+t0+8*%3], [r1+t0*2+16*%3], [r1+t0*2+24*%3]
    %1 [r0+t0     ], [r1+t0*2      ], [r1+t0*2+ 8*%3]
    sub t0d, 16*%3
    jge %%loop
    REP_RET
%else
    %1 [r0+8*%3], [r1+16*%3], [r1+24*%3]
    %1 [r0     ], [r1      ], [r1+ 8*%3]
    RET
%endif
%endmacro

%macro DEQUANT16_FLAT 2-5
    mova   m0, %1
    psllw  m0, m4
%assign i %0-2
%rep %0-1
%if i
    mova   m %+ i, [r0+%2]
    pmullw m %+ i, m0
%else
    pmullw m0, [r0+%2]
%endif
    mova   [r0+%2], m %+ i
    %assign i i-1
    %rotate 1
%endrep
%endmacro

%ifdef WIN64
    DECLARE_REG_TMP 6,3,2
%elifdef ARCH_X86_64
    DECLARE_REG_TMP 4,3,2
%else
    DECLARE_REG_TMP 2,0,1
%endif

%macro DEQUANT_START 2
    movifnidn t2d, r2m
    imul t0d, t2d, 0x2b
    shr  t0d, 8     ; i_qbits = i_qp / 6
    lea  t1, [t0*3]
    sub  t2d, t1d
    sub  t2d, t1d   ; i_mf = i_qp % 6
    shl  t2d, %1
%ifdef ARCH_X86_64
    add  r1, t2     ; dequant_mf[i_mf]
%else
    add  r1, r1mp   ; dequant_mf[i_mf]
    mov  r0, r0mp   ; dct
%endif
    sub  t0d, %2
    jl   .rshift32  ; negative qbits => rightshift
%endmacro

;-----------------------------------------------------------------------------
; void dequant_4x4( int16_t dct[4][4], int dequant_mf[6][4][4], int i_qp )
;-----------------------------------------------------------------------------
%macro DEQUANT 4
cglobal dequant_%2x%2_%1, 0,3
.skip_prologue:
    DEQUANT_START %3+2, %3

.lshift:
    movd m2, t0d
    DEQUANT_LOOP DEQUANT16_L, %2*%2/4, %4

.rshift32:
    neg   t0d
    movd  m2, t0d
    mova  m3, [pd_1]
    pxor  m4, m4
    pslld m3, m2
    psrld m3, 1
    DEQUANT_LOOP DEQUANT32_R, %2*%2/4, %4

cglobal dequant_%2x%2_flat16_%1, 0,3
    movifnidn t2d, r2m
%if %2 == 8
    cmp  t2d, 12
    jl dequant_%2x%2_%1.skip_prologue
    sub  t2d, 12
%endif
    imul t0d, t2d, 0x2b
    shr  t0d, 8     ; i_qbits = i_qp / 6
    lea  t1, [t0*3]
    sub  t2d, t1d
    sub  t2d, t1d   ; i_mf = i_qp % 6
    shl  t2d, %3
%ifdef PIC
    lea  r1, [dequant%2_scale]
    add  r1, t2
%else
    lea  r1, [dequant%2_scale + t2]
%endif
    movifnidn r0, r0mp
    movd m4, t0d
%if %2 == 4
%ifidn %1, mmx
    DEQUANT16_FLAT [r1], 0, 16
    DEQUANT16_FLAT [r1+8], 8, 24
%else
    DEQUANT16_FLAT [r1], 0, 16
%endif
%elifidn %1, mmx
    DEQUANT16_FLAT [r1], 0, 8, 64, 72
    DEQUANT16_FLAT [r1+16], 16, 24, 48, 56
    DEQUANT16_FLAT [r1+16], 80, 88, 112, 120
    DEQUANT16_FLAT [r1+32], 32, 40, 96, 104
%else
    DEQUANT16_FLAT [r1], 0, 64
    DEQUANT16_FLAT [r1+16], 16, 48, 80, 112
    DEQUANT16_FLAT [r1+32], 32, 96
%endif
    RET
%endmacro ; DEQUANT

%ifndef ARCH_X86_64
INIT_MMX
DEQUANT mmx, 4, 4, 1
DEQUANT mmx, 8, 6, 1
%endif
INIT_XMM
DEQUANT sse2, 4, 4, 2
DEQUANT sse2, 8, 6, 2

%macro DEQUANT_DC 1
cglobal dequant_4x4dc_%1, 0,3
    DEQUANT_START 6, 6

.lshift:
    movd   m3, [r1]
    movd   m2, t0d
    pslld  m3, m2
%if mmsize==16
    pshuflw  m3, m3, 0
    punpcklqdq m3, m3
%else
    pshufw   m3, m3, 0
%endif
%assign x 0
%rep 16/mmsize
    mova     m0, [r0+mmsize*0+x]
    mova     m1, [r0+mmsize*1+x]
    pmullw   m0, m3
    pmullw   m1, m3
    mova     [r0+mmsize*0+x], m0
    mova     [r0+mmsize*1+x], m1
%assign x x+mmsize*2
%endrep
    RET

.rshift32:
    neg   t0d
    movd  m3, t0d
    mova  m4, [pw_1]
    mova  m5, m4
    pslld m4, m3
    psrld m4, 1
    movd  m2, [r1]
%if mmsize==8
    punpcklwd m2, m2
%else
    pshuflw m2, m2, 0
%endif
    punpcklwd m2, m4
%assign x 0
%rep 32/mmsize
    mova      m0, [r0+x]
    mova      m1, m0
    punpcklwd m0, m5
    punpckhwd m1, m5
    pmaddwd   m0, m2
    pmaddwd   m1, m2
    psrad     m0, m3
    psrad     m1, m3
    packssdw  m0, m1
    mova      [r0+x], m0
%assign x x+mmsize
%endrep
    RET
%endmacro

INIT_MMX
DEQUANT_DC mmxext
INIT_XMM
DEQUANT_DC sse2

%ifdef HIGH_BIT_DEPTH
;-----------------------------------------------------------------------------
; void denoise_dct( int32_t *dct, uint32_t *sum, uint32_t *offset, int size )
;-----------------------------------------------------------------------------
%macro DENOISE_DCT 1-2 0
cglobal denoise_dct_%1, 4,5,%2
    mov       r4d, [r0] ; backup DC coefficient
    pxor      m6, m6
.loop:
    sub       r3, mmsize/2
    mova      m2, [r0+r3*4+0*mmsize]
    mova      m3, [r0+r3*4+1*mmsize]
    PABSD     m0, m2
    PABSD     m1, m3
    mova      m4, m0
    mova      m5, m1
    psubd     m0, [r2+r3*4+0*mmsize]
    psubd     m1, [r2+r3*4+1*mmsize]
    mova      m7, m0
    pcmpgtd   m7, m6
    pand      m0, m7
    mova      m7, m1
    pcmpgtd   m7, m6
    pand      m1, m7
    PSIGND    m0, m2
    PSIGND    m1, m3
    mova      [r0+r3*4+0*mmsize], m0
    mova      [r0+r3*4+1*mmsize], m1
    paddd     m4, [r1+r3*4+0*mmsize]
    paddd     m5, [r1+r3*4+1*mmsize]
    mova      [r1+r3*4+0*mmsize], m4
    mova      [r1+r3*4+1*mmsize], m5
    jg .loop
    mov       [r0], r4d ; restore DC coefficient
    RET
%endmacro

%define PABSD PABSD_MMX
%define PSIGND PSIGND_MMX
%ifndef ARCH_X86_64
INIT_MMX
DENOISE_DCT mmx
%endif
INIT_XMM
DENOISE_DCT sse2, 8
%define PABSD PABSD_SSSE3
%define PSIGND PSIGND_SSSE3
DENOISE_DCT ssse3, 8

%else ; !HIGH_BIT_DEPTH

;-----------------------------------------------------------------------------
; void denoise_dct( int16_t *dct, uint32_t *sum, uint16_t *offset, int size )
;-----------------------------------------------------------------------------
%macro DENOISE_DCT 1-2 0
cglobal denoise_dct_%1, 4,5,%2
    movzx     r4d, word [r0]
    pxor      m6, m6
.loop:
    sub       r3, mmsize
    mova      m2, [r0+r3*2+0*mmsize]
    mova      m3, [r0+r3*2+1*mmsize]
    PABSW     m0, m2
    PABSW     m1, m3
    mova      m4, m0
    mova      m5, m1
    psubusw   m0, [r2+r3*2+0*mmsize]
    psubusw   m1, [r2+r3*2+1*mmsize]
    PSIGNW    m0, m2
    PSIGNW    m1, m3
    mova      [r0+r3*2+0*mmsize], m0
    mova      [r0+r3*2+1*mmsize], m1
    mova      m2, m4
    mova      m3, m5
    punpcklwd m4, m6
    punpckhwd m2, m6
    punpcklwd m5, m6
    punpckhwd m3, m6
    paddd     m4, [r1+r3*4+0*mmsize]
    paddd     m2, [r1+r3*4+1*mmsize]
    paddd     m5, [r1+r3*4+2*mmsize]
    paddd     m3, [r1+r3*4+3*mmsize]
    mova      [r1+r3*4+0*mmsize], m4
    mova      [r1+r3*4+1*mmsize], m2
    mova      [r1+r3*4+2*mmsize], m5
    mova      [r1+r3*4+3*mmsize], m3
    jg .loop
    mov       [r0], r4w
    RET
%endmacro

%define PABSW PABSW_MMX
%define PSIGNW PSIGNW_MMX
%ifndef ARCH_X86_64
INIT_MMX
DENOISE_DCT mmx
%endif
INIT_XMM
DENOISE_DCT sse2, 7
%define PABSW PABSW_SSSE3
%define PSIGNW PSIGNW_SSSE3
DENOISE_DCT ssse3, 7

%endif ; !HIGH_BIT_DEPTH

;-----------------------------------------------------------------------------
; int decimate_score( dctcoef *dct )
;-----------------------------------------------------------------------------

%macro DECIMATE_MASK_SSE2 7
%ifdef HIGH_BIT_DEPTH
    movdqa   xmm0, [%3+ 0]
    movdqa   xmm1, [%3+32]
    packssdw xmm0, [%3+16]
    packssdw xmm1, [%3+48]
%if %7
    pabsw    xmm0, xmm0
    pabsw    xmm1, xmm1
%else
    ABS2_MMX xmm0, xmm1, xmm3, xmm4
%endif
%else
%if %7
    pabsw    xmm0, [%3+ 0]
    pabsw    xmm1, [%3+16]
%else
    movdqa   xmm0, [%3+ 0]
    movdqa   xmm1, [%3+16]
    ABS2_MMX xmm0, xmm1, xmm3, xmm4
%endif
%endif
    packsswb xmm0, xmm1
    pxor     xmm2, xmm2
    pcmpeqb  xmm2, xmm0
    pcmpgtb  xmm0, %4
    pmovmskb %1, xmm2
    pmovmskb %2, xmm0
%endmacro

%macro DECIMATE_MASK_MMX 7
%ifdef HIGH_BIT_DEPTH
    movq      mm0, [%3+ 0]
    movq      mm1, [%3+16]
    movq      mm2, [%3+32]
    movq      mm3, [%3+48]
    packssdw  mm0, [%3+ 8]
    packssdw  mm1, [%3+24]
    packssdw  mm2, [%3+40]
    packssdw  mm3, [%3+56]
%else
    movq      mm0, [%3+ 0]
    movq      mm1, [%3+ 8]
    movq      mm2, [%3+16]
    movq      mm3, [%3+24]
%endif
    ABS2_MMX  mm0, mm1, mm6, mm7
    ABS2_MMX  mm2, mm3, mm6, mm7
    packsswb  mm0, mm1
    packsswb  mm2, mm3
    pxor      mm4, mm4
    pxor      mm6, mm6
    pcmpeqb   mm4, mm0
    pcmpeqb   mm6, mm2
    pcmpgtb   mm0, %4
    pcmpgtb   mm2, %4
    pmovmskb   %6, mm4
    pmovmskb   %1, mm6
    shl        %1, 8
    or         %1, %6
    pmovmskb   %6, mm0
    pmovmskb   %2, mm2
    shl        %2, 8
    or         %2, %6
%endmacro

cextern decimate_table4
cextern decimate_table8

%macro DECIMATE4x4 4

;A LUT is faster than bsf on AMD processors.
;This is not true for score64.
cglobal decimate_score%1_%2, 1,3
%ifdef PIC
    lea r10, [decimate_table4]
    lea r11, [decimate_mask_table4]
    %define table r10
    %define mask_table r11
%else
    %define table decimate_table4
    %define mask_table decimate_mask_table4
%endif
    DECIMATE_MASK edx, eax, r0, [pb_1], %2, ecx, %4
    xor   edx, 0xffff
    je   .ret
    test  eax, eax
    jne  .ret9
%if %1==15
    shr   edx, 1
%endif
%if %3==1
    movzx ecx, dl
    movzx eax, byte [mask_table + rcx]
    cmp   edx, ecx
    je   .ret
    bsr   ecx, ecx
    shr   edx, 1
    shr   edx, cl
    bsf   ecx, edx
    shr   edx, 1
    shr   edx, cl
    add    al, byte [table + rcx]
    add    al, byte [mask_table + rdx]
%else
.loop:
    bsf   ecx, edx
    shr   edx, cl
    add    al, byte [table + rcx]
    shr   edx, 1
    jne  .loop
%endif
.ret:
    RET
.ret9:
    mov   eax, 9
    RET

%endmacro

%ifndef ARCH_X86_64
%define DECIMATE_MASK DECIMATE_MASK_MMX
DECIMATE4x4 15, mmxext, 0, 0
DECIMATE4x4 16, mmxext, 0, 0
DECIMATE4x4 15, mmxext_slowctz, 1, 0
DECIMATE4x4 16, mmxext_slowctz, 1, 0
%endif
%define DECIMATE_MASK DECIMATE_MASK_SSE2
DECIMATE4x4 15, sse2, 0, 0
DECIMATE4x4 16, sse2, 0, 0
DECIMATE4x4 15, sse2_slowctz, 1, 0
DECIMATE4x4 16, sse2_slowctz, 1, 0
DECIMATE4x4 15, ssse3, 0, 1
DECIMATE4x4 16, ssse3, 0, 1
DECIMATE4x4 15, ssse3_slowctz, 1, 1
DECIMATE4x4 16, ssse3_slowctz, 1, 1

%macro DECIMATE8x8 2

%ifdef ARCH_X86_64
cglobal decimate_score64_%1, 1,4
%ifdef PIC
    lea r10, [decimate_table8]
    %define table r10
%else
    %define table decimate_table8
%endif
    mova  m5, [pb_1]
    DECIMATE_MASK r1d, eax, r0+SIZEOF_DCTCOEF* 0, m5, %1, null, %2
    test  eax, eax
    jne  .ret9
    DECIMATE_MASK r2d, eax, r0+SIZEOF_DCTCOEF*16, m5, %1, null, %2
    shl   r2d, 16
    or    r1d, r2d
    DECIMATE_MASK r2d, r3d, r0+SIZEOF_DCTCOEF*32, m5, %1, null, %2
    shl   r2, 32
    or    eax, r3d
    or    r1, r2
    DECIMATE_MASK r2d, r3d, r0+SIZEOF_DCTCOEF*48, m5, %1, null, %2
    shl   r2, 48
    or    r1, r2
    xor   r1, -1
    je   .ret
    or    eax, r3d
    jne  .ret9
.loop:
    bsf   rcx, r1
    shr   r1, cl
    add   al, byte [table + rcx]
    shr   r1, 1
    jne  .loop
.ret:
    REP_RET
.ret9:
    mov   eax, 9
    RET

%else ; ARCH
%ifidn %1, mmxext
cglobal decimate_score64_%1, 1,6
%else
cglobal decimate_score64_%1, 1,5
%endif
    mova  m5, [pb_1]
    DECIMATE_MASK r3, r2, r0+SIZEOF_DCTCOEF* 0, m5, %1, r5, %2
    test  r2, r2
    jne  .ret9
    DECIMATE_MASK r4, r2, r0+SIZEOF_DCTCOEF*16, m5, %1, r5, %2
    shl   r4, 16
    or    r3, r4
    DECIMATE_MASK r4, r1, r0+SIZEOF_DCTCOEF*32, m5, %1, r5, %2
    or    r2, r1
    DECIMATE_MASK r1, r0, r0+SIZEOF_DCTCOEF*48, m5, %1, r5, %2
    shl   r1, 16
    or    r4, r1
    xor   r3, -1
    je   .tryret
    xor   r4, -1
.cont:
    or    r0, r2
    jne  .ret9      ;r0 is zero at this point, so we don't need to zero it
.loop:
    bsf   ecx, r3
    test  r3, r3
    je   .largerun
    shrd  r3, r4, cl
    shr   r4, cl
    add   r0b, byte [decimate_table8 + ecx]
    shrd  r3, r4, 1
    shr   r4, 1
    cmp   r0, 6     ;score64's threshold is never higher than 6
    jge  .ret9      ;this early termination is only useful on 32-bit because it can be done in the latency after shrd
    test  r3, r3
    jne  .loop
    test  r4, r4
    jne  .loop
.ret:
    REP_RET
.tryret:
    xor   r4, -1
    jne  .cont
    REP_RET
.ret9:
    mov   eax, 9
    RET
.largerun:
    mov   r3, r4
    xor   r4, r4
    bsf   ecx, r3
    shr   r3, cl
    shr   r3, 1
    jne  .loop
    REP_RET
%endif ; ARCH

%endmacro

%ifndef ARCH_X86_64
INIT_MMX
%define DECIMATE_MASK DECIMATE_MASK_MMX
DECIMATE8x8 mmxext, 0
%endif
INIT_XMM
%define DECIMATE_MASK DECIMATE_MASK_SSE2
DECIMATE8x8 sse2, 0
DECIMATE8x8 ssse3, 1

;-----------------------------------------------------------------------------
; int coeff_last( dctcoef *dct )
;-----------------------------------------------------------------------------

%macro LAST_X86 3
    bsr %1, %2
%endmacro

%macro LAST_SSE4A 3
    lzcnt %1, %2
    xor %1, %3
%endmacro

%ifdef HIGH_BIT_DEPTH
%macro LAST_MASK4_MMX 2-3
    movq     mm0, [%2]
    packssdw mm0, [%2+8]
    packsswb mm0, mm0
    pcmpeqb  mm0, mm2
    pmovmskb  %1, mm0
%endmacro

%macro LAST_MASK_SSE2 2-3
    movdqa   xmm0, [%2+ 0]
    movdqa   xmm1, [%2+32]
    packssdw xmm0, [%2+16]
    packssdw xmm1, [%2+48]
    packsswb xmm0, xmm1
    pcmpeqb  xmm0, xmm2
    pmovmskb   %1, xmm0
%endmacro

%macro LAST_MASK_MMX 3
    movq     mm0, [%2+ 0]
    movq     mm1, [%2+16]
    packssdw mm0, [%2+ 8]
    packssdw mm1, [%2+24]
    movq     mm3, [%2+32]
    movq     mm4, [%2+48]
    packssdw mm3, [%2+40]
    packssdw mm4, [%2+56]
    packsswb mm0, mm1
    packsswb mm3, mm4
    pcmpeqb  mm0, mm2
    pcmpeqb  mm3, mm2
    pmovmskb  %1, mm0
    pmovmskb  %3, mm3
    shl       %3, 8
    or        %1, %3
%endmacro
%else ; !HIGH_BIT_DEPTH
%macro LAST_MASK4_MMX 2-3
    movq     mm0, [%2]
    packsswb mm0, mm0
    pcmpeqb  mm0, mm2
    pmovmskb  %1, mm0
%endmacro

%macro LAST_MASK_SSE2 2-3
    movdqa   xmm0, [%2+ 0]
    packsswb xmm0, [%2+16]
    pcmpeqb  xmm0, xmm2
    pmovmskb   %1, xmm0
%endmacro

%macro LAST_MASK_MMX 3
    movq     mm0, [%2+ 0]
    movq     mm1, [%2+16]
    packsswb mm0, [%2+ 8]
    packsswb mm1, [%2+24]
    pcmpeqb  mm0, mm2
    pcmpeqb  mm1, mm2
    pmovmskb  %1, mm0
    pmovmskb  %3, mm1
    shl       %3, 8
    or        %1, %3
%endmacro

%macro COEFF_LAST4 1
%ifdef ARCH_X86_64
cglobal coeff_last4_%1, 1,1
    LAST rax, [r0], 0x3f
    shr  eax, 4
    RET
%else
cglobal coeff_last4_%1, 0,3
    mov   edx, r0mp
    mov   eax, [edx+4]
    xor   ecx, ecx
    test  eax, eax
    cmovz eax, [edx]
    setnz cl
    LAST  eax, eax, 0x1f
    shr   eax, 4
    lea   eax, [eax+ecx*2]
    RET
%endif
%endmacro

%define LAST LAST_X86
COEFF_LAST4 mmxext
%define LAST LAST_SSE4A
COEFF_LAST4 mmxext_lzcnt
%endif ; HIGH_BIT_DEPTH

%macro COEFF_LAST 1
cglobal coeff_last15_%1, 1,3
    pxor m2, m2
    LAST_MASK r1d, r0-SIZEOF_DCTCOEF, r2d
    xor r1d, 0xffff
    LAST eax, r1d, 0x1f
    dec eax
    RET

cglobal coeff_last16_%1, 1,3
    pxor m2, m2
    LAST_MASK r1d, r0, r2d
    xor r1d, 0xffff
    LAST eax, r1d, 0x1f
    RET

%ifndef ARCH_X86_64
cglobal coeff_last64_%1, 1, 5-mmsize/16
    pxor m2, m2
    LAST_MASK r2d, r0+SIZEOF_DCTCOEF* 32, r4d
    LAST_MASK r3d, r0+SIZEOF_DCTCOEF* 48, r4d
    shl r3d, 16
    or  r2d, r3d
    xor r2d, -1
    jne .secondhalf
    LAST_MASK r1d, r0+SIZEOF_DCTCOEF* 0, r4d
    LAST_MASK r3d, r0+SIZEOF_DCTCOEF*16, r4d
    shl r3d, 16
    or  r1d, r3d
    not r1d
    LAST eax, r1d, 0x1f
    RET
.secondhalf:
    LAST eax, r2d, 0x1f
    add eax, 32
    RET
%else
cglobal coeff_last64_%1, 1,4
    pxor m2, m2
    LAST_MASK_SSE2 r1d, r0+SIZEOF_DCTCOEF* 0
    LAST_MASK_SSE2 r2d, r0+SIZEOF_DCTCOEF*16
    LAST_MASK_SSE2 r3d, r0+SIZEOF_DCTCOEF*32
    LAST_MASK_SSE2 r0d, r0+SIZEOF_DCTCOEF*48
    shl r2d, 16
    shl r0d, 16
    or  r1d, r2d
    or  r3d, r0d
    shl r3,  32
    or  r1,  r3
    not r1
    LAST rax, r1, 0x3f
    RET
%endif
%endmacro

%define LAST LAST_X86
%ifndef ARCH_X86_64
INIT_MMX
%define LAST_MASK LAST_MASK_MMX
COEFF_LAST mmxext
%endif
INIT_XMM
%define LAST_MASK LAST_MASK_SSE2
COEFF_LAST sse2
%define LAST LAST_SSE4A
COEFF_LAST sse2_lzcnt

;-----------------------------------------------------------------------------
; int coeff_level_run( dctcoef *dct, run_level_t *runlevel )
;-----------------------------------------------------------------------------

%macro LZCOUNT_X86 3
    bsr %1, %2
    xor %1, %3
%endmacro

%macro LZCOUNT_SSE4A 3
    lzcnt %1, %2
%endmacro

; t6 = eax for return, t3 = ecx for shift, t[01] = r[01] for x86_64 args
%ifdef WIN64
    DECLARE_REG_TMP 3,1,2,0,4,5,6
%elifdef ARCH_X86_64
    DECLARE_REG_TMP 0,1,2,3,4,5,6
%else
    DECLARE_REG_TMP 6,3,2,1,4,5,0
%endif

%macro COEFF_LEVELRUN 2
cglobal coeff_level_run%2_%1,0,7
    movifnidn t0, r0mp
    movifnidn t1, r1mp
    pxor    m2, m2
    LAST_MASK t5d, t0-(%2&1)*SIZEOF_DCTCOEF, t4d
    not    t5d
    shl    t5d, 32-((%2+1)&~1)
    mov    t4d, %2-1
    LZCOUNT t3d, t5d, 0x1f
    xor    t6d, t6d
    add    t5d, t5d
    sub    t4d, t3d
    shl    t5d, t3b
    mov   [t1], t4d
.loop:
    LZCOUNT t3d, t5d, 0x1f
%ifdef HIGH_BIT_DEPTH
    mov    t2d, [t0+t4*4]
    mov   [t1+t6  +4+16*4], t3b
    mov   [t1+t6*4+ 4], t2d
%else
    mov    t2w, [t0+t4*2]
    mov   [t1+t6  +4+16*2], t3b
    mov   [t1+t6*2+ 4], t2w
%endif
    inc    t3d
    shl    t5d, t3b
    inc    t6d
    sub    t4d, t3d
    jge .loop
    REP_RET
%endmacro

INIT_MMX
%define LZCOUNT LZCOUNT_X86
%ifndef ARCH_X86_64
%define LAST_MASK LAST_MASK_MMX
COEFF_LEVELRUN mmxext, 15
COEFF_LEVELRUN mmxext, 16
%endif
%define LAST_MASK LAST_MASK4_MMX
COEFF_LEVELRUN mmxext, 4
INIT_XMM
%define LAST_MASK LAST_MASK_SSE2
COEFF_LEVELRUN sse2, 15
COEFF_LEVELRUN sse2, 16
%define LZCOUNT LZCOUNT_SSE4A
COEFF_LEVELRUN sse2_lzcnt, 15
COEFF_LEVELRUN sse2_lzcnt, 16
INIT_MMX
%define LAST_MASK LAST_MASK4_MMX
COEFF_LEVELRUN mmxext_lzcnt, 4

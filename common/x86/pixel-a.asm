;*****************************************************************************
;* pixel.asm: x86 pixel metrics
;*****************************************************************************
;* Copyright (C) 2003-2011 x264 project
;*
;* Authors: Loren Merritt <lorenm@u.washington.edu>
;*          Holger Lubitz <holger@lubitz.org>
;*          Laurent Aimar <fenrir@via.ecp.fr>
;*          Alex Izvorski <aizvorksi@gmail.com>
;*          Jason Garrett-Glaser <darkshikari@gmail.com>
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

SECTION_RODATA 32
mask_ff:   times 16 db 0xff
           times 16 db 0
%if BIT_DEPTH == 10
ssim_c1:   times 4 dd 6697.7856    ; .01*.01*1023*1023*64
ssim_c2:   times 4 dd 3797644.4352 ; .03*.03*1023*1023*64*63
pf_64:     times 4 dd 64.0
pf_128:    times 4 dd 128.0
%elif BIT_DEPTH == 9
ssim_c1:   times 4 dd 1671         ; .01*.01*511*511*64
ssim_c2:   times 4 dd 947556       ; .03*.03*511*511*64*63
%else ; 8-bit
ssim_c1:   times 4 dd 416          ; .01*.01*255*255*64
ssim_c2:   times 4 dd 235963       ; .03*.03*255*255*64*63
%endif
mask_ac4:  dw 0, -1, -1, -1, 0, -1, -1, -1
mask_ac4b: dw 0, -1, 0, -1, -1, -1, -1, -1
mask_ac8:  dw 0, -1, -1, -1, -1, -1, -1, -1
hmul_4p:   times 2 db 1, 1, 1, 1, 1, -1, 1, -1
hmul_8p:   times 8 db 1
           times 4 db 1, -1
mask_10:   times 4 dw 0, -1
mask_1100: times 2 dd 0, -1
deinterleave_shuf: db 0, 2, 4, 6, 8, 10, 12, 14, 1, 3, 5, 7, 9, 11, 13, 15

pd_f0:     times 4 dd 0xffff0000
sq_0f:     times 1 dq 0xffffffff

SECTION .text

cextern pw_1
cextern pw_00ff

cextern hsub_mul

;=============================================================================
; SSD
;=============================================================================

%ifdef HIGH_BIT_DEPTH
;-----------------------------------------------------------------------------
; int pixel_ssd_MxN( uint16_t *, int, uint16_t *, int )
;-----------------------------------------------------------------------------
%macro SSD_ONE 2
cglobal pixel_ssd_%1x%2, 4,5,6
    mov     r4, %1*%2/mmsize
    pxor    m0, m0
.loop
    mova    m1, [r0]
%if %1 <= mmsize/2
    mova    m3, [r0+r1*2]
    %define offset r3*2
    %define num_rows 2
%else
    mova    m3, [r0+mmsize]
    %define offset mmsize
    %define num_rows 1
%endif
    psubw   m1, [r2]
    psubw   m3, [r2+offset]
    pmaddwd m1, m1
    pmaddwd m3, m3
    dec     r4
    lea     r0, [r0+r1*2*num_rows]
    lea     r2, [r2+r3*2*num_rows]
    paddd   m0, m1
    paddd   m0, m3
    jg .loop
    HADDD   m0, m5
    movd   eax, m0
    RET
%endmacro

%macro SSD_16_MMX 2
cglobal pixel_ssd_%1x%2, 4,5
    mov     r4, %1*%2/mmsize/2
    pxor    m0, m0
.loop
    mova    m1, [r0]
    mova    m2, [r2]
    mova    m3, [r0+mmsize]
    mova    m4, [r2+mmsize]
    mova    m5, [r0+mmsize*2]
    mova    m6, [r2+mmsize*2]
    mova    m7, [r0+mmsize*3]
    psubw   m1, m2
    psubw   m3, m4
    mova    m2, [r2+mmsize*3]
    psubw   m5, m6
    pmaddwd m1, m1
    psubw   m7, m2
    pmaddwd m3, m3
    pmaddwd m5, m5
    dec     r4
    lea     r0, [r0+r1*2]
    lea     r2, [r2+r3*2]
    pmaddwd m7, m7
    paddd   m1, m3
    paddd   m5, m7
    paddd   m0, m1
    paddd   m0, m5
    jg .loop
    HADDD   m0, m7
    movd   eax, m0
    RET
%endmacro

INIT_MMX mmx2
SSD_ONE     4,  4
SSD_ONE     4,  8
SSD_ONE     8,  4
SSD_ONE     8,  8
SSD_ONE     8, 16
SSD_16_MMX 16,  8
SSD_16_MMX 16, 16
INIT_XMM sse2
SSD_ONE     8,  4
SSD_ONE     8,  8
SSD_ONE     8, 16
SSD_ONE    16,  8
SSD_ONE    16, 16
%endif ; HIGH_BIT_DEPTH

%ifndef HIGH_BIT_DEPTH
%macro SSD_LOAD_FULL 5
    mova      m1, [t0+%1]
    mova      m2, [t2+%2]
    mova      m3, [t0+%3]
    mova      m4, [t2+%4]
%if %5==1
    add       t0, t1
    add       t2, t3
%elif %5==2
    lea       t0, [t0+2*t1]
    lea       t2, [t2+2*t3]
%endif
%endmacro

%macro LOAD 5
    movh      m%1, %3
    movh      m%2, %4
%if %5
    lea       t0, [t0+2*t1]
%endif
%endmacro

%macro JOIN 7
    movh      m%3, %5
    movh      m%4, %6
%if %7
    lea       t2, [t2+2*t3]
%endif
    punpcklbw m%1, m7
    punpcklbw m%3, m7
    psubw     m%1, m%3
    punpcklbw m%2, m7
    punpcklbw m%4, m7
    psubw     m%2, m%4
%endmacro

%macro JOIN_SSE2 7
    movh      m%3, %5
    movh      m%4, %6
%if %7
    lea       t2, [t2+2*t3]
%endif
    punpcklqdq m%1, m%2
    punpcklqdq m%3, m%4
    DEINTB %2, %1, %4, %3, 7
    psubw m%2, m%4
    psubw m%1, m%3
%endmacro

%macro JOIN_SSSE3 7
    movh      m%3, %5
    movh      m%4, %6
%if %7
    lea       t2, [t2+2*t3]
%endif
    punpcklbw m%1, m%3
    punpcklbw m%2, m%4
%endmacro

%macro SSD_LOAD_HALF 5
    LOAD      1, 2, [t0+%1], [t0+%3], 1
    JOIN      1, 2, 3, 4, [t2+%2], [t2+%4], 1
    LOAD      3, 4, [t0+%1], [t0+%3], %5
    JOIN      3, 4, 5, 6, [t2+%2], [t2+%4], %5
%endmacro

%macro SSD_CORE 7-8
%ifidn %8, FULL
    mova      m%6, m%2
    mova      m%7, m%4
    psubusb   m%2, m%1
    psubusb   m%4, m%3
    psubusb   m%1, m%6
    psubusb   m%3, m%7
    por       m%1, m%2
    por       m%3, m%4
    punpcklbw m%2, m%1, m%5
    punpckhbw m%1, m%5
    punpcklbw m%4, m%3, m%5
    punpckhbw m%3, m%5
%endif
    pmaddwd   m%1, m%1
    pmaddwd   m%2, m%2
    pmaddwd   m%3, m%3
    pmaddwd   m%4, m%4
%endmacro

%macro SSD_CORE_SSE2 7-8
%ifidn %8, FULL
    DEINTB %6, %1, %7, %2, %5
    psubw m%6, m%7
    psubw m%1, m%2
    SWAP %6, %2, %1
    DEINTB %6, %3, %7, %4, %5
    psubw m%6, m%7
    psubw m%3, m%4
    SWAP %6, %4, %3
%endif
    pmaddwd   m%1, m%1
    pmaddwd   m%2, m%2
    pmaddwd   m%3, m%3
    pmaddwd   m%4, m%4
%endmacro

%macro SSD_CORE_SSSE3 7-8
%ifidn %8, FULL
    punpckhbw m%6, m%1, m%2
    punpckhbw m%7, m%3, m%4
    punpcklbw m%1, m%2
    punpcklbw m%3, m%4
    SWAP %6, %2, %3
    SWAP %7, %4
%endif
    pmaddubsw m%1, m%5
    pmaddubsw m%2, m%5
    pmaddubsw m%3, m%5
    pmaddubsw m%4, m%5
    pmaddwd   m%1, m%1
    pmaddwd   m%2, m%2
    pmaddwd   m%3, m%3
    pmaddwd   m%4, m%4
%endmacro

%macro SSD_ITER 6
    SSD_LOAD_%1 %2,%3,%4,%5,%6
    SSD_CORE  1, 2, 3, 4, 7, 5, 6, %1
    paddd     m1, m2
    paddd     m3, m4
    paddd     m0, m1
    paddd     m0, m3
%endmacro

;-----------------------------------------------------------------------------
; int pixel_ssd_16x16( uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
%macro SSD 2
%if %1 != %2
    %assign function_align 8
%else
    %assign function_align 16
%endif
cglobal pixel_ssd_%1x%2, 0,0,0
    mov     al, %1*%2/mmsize/2

%if %1 != %2
    jmp mangle(x264_pixel_ssd_%1x%1 %+ SUFFIX %+ .startloop)
%else

.startloop:
%ifdef ARCH_X86_64
    DECLARE_REG_TMP 0,1,2,3
    PROLOGUE 0,0,8
%else
    PROLOGUE 0,5
    DECLARE_REG_TMP 1,2,3,4
    mov t0, r0m
    mov t1, r1m
    mov t2, r2m
    mov t3, r3m
%endif

%if cpuflag(ssse3)
    mova    m7, [hsub_mul]
%elifidn cpuname, sse2
    mova    m7, [pw_00ff]
%elif %1 >= mmsize
    pxor    m7, m7
%endif
    pxor    m0, m0

ALIGN 16
.loop:
%if %1 > mmsize
    SSD_ITER FULL, 0, 0, mmsize, mmsize, 1
%elif %1 == mmsize
    SSD_ITER FULL, 0, 0, t1, t3, 2
%else
    SSD_ITER HALF, 0, 0, t1, t3, 2
%endif
    dec     al
    jg .loop
    HADDD   m0, m1
    movd   eax, m0
    RET
%endif
%endmacro

INIT_MMX mmx
SSD 16, 16
SSD 16,  8
SSD  8,  8
SSD  8, 16
SSD  4,  4
SSD  8,  4
SSD  4,  8
INIT_XMM sse2slow
SSD 16, 16
SSD  8,  8
SSD 16,  8
SSD  8, 16
SSD  8,  4
INIT_XMM sse2
%define SSD_CORE SSD_CORE_SSE2
%define JOIN JOIN_SSE2
SSD 16, 16
SSD  8,  8
SSD 16,  8
SSD  8, 16
SSD  8,  4
INIT_XMM ssse3
%define SSD_CORE SSD_CORE_SSSE3
%define JOIN JOIN_SSSE3
SSD 16, 16
SSD  8,  8
SSD 16,  8
SSD  8, 16
SSD  8,  4
INIT_XMM avx
SSD 16, 16
SSD  8,  8
SSD 16,  8
SSD  8, 16
SSD  8,  4
INIT_MMX ssse3
SSD  4,  4
SSD  4,  8
%assign function_align 16
%endif ; !HIGH_BIT_DEPTH

;-----------------------------------------------------------------------------
; void pixel_ssd_nv12_core( uint16_t *pixuv1, int stride1, uint16_t *pixuv2, int stride2,
;                           int width, int height, uint64_t *ssd_u, uint64_t *ssd_v )
;
; The maximum width this function can handle without risk of overflow is given
; in the following equation: (mmsize in bits)
;
;   2 * mmsize/32 * (2^32 - 1) / (2^BIT_DEPTH - 1)^2
;
; For 10-bit MMX this means width >= 16416 and for XMM >= 32832. At sane
; distortion levels it will take much more than that though.
;-----------------------------------------------------------------------------
%ifdef HIGH_BIT_DEPTH
%macro SSD_NV12 0
cglobal pixel_ssd_nv12_core, 6,7,7
    shl        r4d, 2
    FIX_STRIDES r1, r3
    add         r0, r4
    add         r2, r4
    xor         r6, r6
    pxor        m4, m4
    pxor        m5, m5
    pxor        m6, m6
.loopy:
    mov         r6, r4
    neg         r6
    pxor        m2, m2
    pxor        m3, m3
.loopx:
    mova        m0, [r0+r6]
    mova        m1, [r0+r6+mmsize]
    psubw       m0, [r2+r6]
    psubw       m1, [r2+r6+mmsize]
    PSHUFLW     m0, m0, q3120
    PSHUFLW     m1, m1, q3120
%if mmsize==16
    pshufhw     m0, m0, q3120
    pshufhw     m1, m1, q3120
%endif
    pmaddwd     m0, m0
    pmaddwd     m1, m1
    paddd       m2, m0
    paddd       m3, m1
    add         r6, 2*mmsize
    jl .loopx
%if mmsize==16 ; using HADDD would remove the mmsize/32 part from the
               ; equation above, putting the width limit at 8208
    punpckhdq   m0, m2, m6
    punpckhdq   m1, m3, m6
    punpckldq   m2, m6
    punpckldq   m3, m6
    paddq       m3, m2
    paddq       m1, m0
    paddq       m4, m3
    paddq       m4, m1
%else ; unfortunately paddq is sse2
      ; emulate 48 bit precision for mmx2 instead
    mova        m0, m2
    mova        m1, m3
    punpcklwd   m2, m6
    punpcklwd   m3, m6
    punpckhwd   m0, m6
    punpckhwd   m1, m6
    paddd       m3, m2
    paddd       m1, m0
    paddd       m4, m3
    paddd       m5, m1
%endif
    add         r0, r1
    add         r2, r3
    dec        r5d
    jg .loopy
    mov         r3, r6m
    mov         r4, r7m
%if mmsize==16
    movq      [r3], m4
    movhps    [r4], m4
%else ; fixup for mmx2
    SBUTTERFLY dq, 4, 5, 0
    mova        m0, m4
    psrld       m4, 16
    paddd       m5, m4
    pslld       m0, 16
    SBUTTERFLY dq, 0, 5, 4
    psrlq       m0, 16
    psrlq       m5, 16
    movq      [r3], m0
    movq      [r4], m5
%endif
    RET
%endmacro ; SSD_NV12
%endif ; HIGH_BIT_DEPTH

%ifndef HIGH_BIT_DEPTH
;-----------------------------------------------------------------------------
; void pixel_ssd_nv12_core( uint8_t *pixuv1, int stride1, uint8_t *pixuv2, int stride2,
;                           int width, int height, uint64_t *ssd_u, uint64_t *ssd_v )
;
; This implementation can potentially overflow on image widths >= 11008 (or
; 6604 if interlaced), since it is called on blocks of height up to 12 (resp
; 20). At sane distortion levels it will take much more than that though.
;-----------------------------------------------------------------------------
%macro SSD_NV12 0
cglobal pixel_ssd_nv12_core, 6,7
    shl    r4d, 1
    add     r0, r4
    add     r2, r4
    pxor    m3, m3
    pxor    m4, m4
    mova    m5, [pw_00ff]
.loopy:
    mov     r6, r4
    neg     r6
.loopx:
    mova    m0, [r0+r6]
    mova    m1, [r2+r6]
    psubusb m0, m1
    psubusb m1, [r0+r6]
    por     m0, m1
    psrlw   m2, m0, 8
    add     r6, mmsize
    pand    m0, m5
    pmaddwd m2, m2
    pmaddwd m0, m0
    paddd   m3, m0
    paddd   m4, m2
    jl .loopx
    add     r0, r1
    add     r2, r3
    dec    r5d
    jg .loopy
    mov     r3, r6m
    mov     r4, r7m
    mova    m5, [sq_0f]
    HADDD   m3, m0
    HADDD   m4, m0
    pand    m3, m5
    pand    m4, m5
    movq  [r3], m3
    movq  [r4], m4
    RET
%endmacro ; SSD_NV12
%endif ; !HIGH_BIT_DEPTH

INIT_MMX mmx2
SSD_NV12
INIT_XMM sse2
SSD_NV12
INIT_XMM avx
SSD_NV12

;=============================================================================
; variance
;=============================================================================

%macro VAR_START 1
    pxor  m5, m5    ; sum
    pxor  m6, m6    ; sum squared
%ifndef HIGH_BIT_DEPTH
%if %1
    mova  m7, [pw_00ff]
%else
    pxor  m7, m7    ; zero
%endif
%endif ; !HIGH_BIT_DEPTH
%endmacro

%macro VAR_END 2
%ifdef HIGH_BIT_DEPTH
%if mmsize == 8 && %1*%2 == 256
    HADDUW  m5, m2
%else
    HADDW   m5, m2
%endif
%else ; !HIGH_BIT_DEPTH
    HADDW   m5, m2
%endif ; HIGH_BIT_DEPTH
    movd   eax, m5
    HADDD   m6, m1
    movd   edx, m6
%ifdef ARCH_X86_64
    shl    rdx, 32
    add    rax, rdx
%endif
    RET
%endmacro

%macro VAR_CORE 0
    paddw     m5, m0
    paddw     m5, m3
    paddw     m5, m1
    paddw     m5, m4
    pmaddwd   m0, m0
    pmaddwd   m3, m3
    pmaddwd   m1, m1
    pmaddwd   m4, m4
    paddd     m6, m0
    paddd     m6, m3
    paddd     m6, m1
    paddd     m6, m4
%endmacro

%macro VAR_2ROW 2
    mov      r2d, %2
.loop:
%ifdef HIGH_BIT_DEPTH
    mova      m0, [r0]
    mova      m1, [r0+mmsize]
    mova      m3, [r0+%1]
    mova      m4, [r0+%1+mmsize]
%else ; !HIGH_BIT_DEPTH
    mova      m0, [r0]
    punpckhbw m1, m0, m7
    mova      m3, [r0+%1]
    mova      m4, m3
    punpcklbw m0, m7
%endif ; HIGH_BIT_DEPTH
%ifidn %1, r1
    lea       r0, [r0+%1*2]
%else
    add       r0, r1
%endif
%ifndef HIGH_BIT_DEPTH
    punpcklbw m3, m7
    punpckhbw m4, m7
%endif ; !HIGH_BIT_DEPTH
    dec r2d
    VAR_CORE
    jg .loop
%endmacro

;-----------------------------------------------------------------------------
; int pixel_var_wxh( uint8_t *, int )
;-----------------------------------------------------------------------------
INIT_MMX
cglobal pixel_var_16x16_mmx2, 2,3
    FIX_STRIDES r1
    VAR_START 0
    VAR_2ROW 8*SIZEOF_PIXEL, 16
    VAR_END 16, 16

cglobal pixel_var_8x8_mmx2, 2,3
    FIX_STRIDES r1
    VAR_START 0
    VAR_2ROW r1, 4
    VAR_END 8, 8

%ifdef HIGH_BIT_DEPTH
%macro VAR 0
cglobal pixel_var_16x16, 2,3,8
    FIX_STRIDES r1
    VAR_START 0
    VAR_2ROW r1, 8
    VAR_END 16, 16

cglobal pixel_var_8x8, 2,3,8
    lea       r2, [r1*3]
    VAR_START 0
    mova      m0, [r0]
    mova      m1, [r0+r1*2]
    mova      m3, [r0+r1*4]
    mova      m4, [r0+r2*2]
    lea       r0, [r0+r1*8]
    VAR_CORE
    mova      m0, [r0]
    mova      m1, [r0+r1*2]
    mova      m3, [r0+r1*4]
    mova      m4, [r0+r2*2]
    VAR_CORE
    VAR_END 8, 8
%endmacro ; VAR

INIT_XMM sse2
VAR
INIT_XMM avx
VAR
%endif ; HIGH_BIT_DEPTH

%ifndef HIGH_BIT_DEPTH
%macro VAR 0
cglobal pixel_var_16x16, 2,3,8
    VAR_START 1
    mov      r2d, 8
.loop:
    mova      m0, [r0]
    mova      m3, [r0+r1]
    DEINTB    1, 0, 4, 3, 7
    lea       r0, [r0+r1*2]
    VAR_CORE
    dec r2d
    jg .loop
    VAR_END 16, 16

cglobal pixel_var_8x8, 2,4,8
    VAR_START 1
    mov      r2d, 2
    lea       r3, [r1*3]
.loop:
    movh      m0, [r0]
    movh      m3, [r0+r1]
    movhps    m0, [r0+r1*2]
    movhps    m3, [r0+r3]
    DEINTB    1, 0, 4, 3, 7
    lea       r0, [r0+r1*4]
    VAR_CORE
    dec r2d
    jg .loop
    VAR_END 8, 8
%endmacro ; VAR

INIT_XMM sse2
VAR
INIT_XMM avx
VAR
%endif ; !HIGH_BIT_DEPTH

%macro VAR2_END 0
    HADDW   m5, m7
    movd   r1d, m5
    imul   r1d, r1d
    HADDD   m6, m1
    shr    r1d, 6
    movd   eax, m6
    mov   [r4], eax
    sub    eax, r1d  ; sqr - (sum * sum >> shift)
    RET
%endmacro

;-----------------------------------------------------------------------------
; int pixel_var2_8x8( pixel *, int, pixel *, int, int * )
;-----------------------------------------------------------------------------
INIT_MMX
cglobal pixel_var2_8x8_mmx2, 5,6
    FIX_STRIDES r1, r3
    VAR_START 0
    mov      r5d, 8
.loop:
%ifdef HIGH_BIT_DEPTH
    mova      m0, [r0]
    mova      m1, [r0+mmsize]
    psubw     m0, [r2]
    psubw     m1, [r2+mmsize]
%else ; !HIGH_BIT_DEPTH
    movq      m0, [r0]
    movq      m1, m0
    movq      m2, [r2]
    movq      m3, m2
    punpcklbw m0, m7
    punpckhbw m1, m7
    punpcklbw m2, m7
    punpckhbw m3, m7
    psubw     m0, m2
    psubw     m1, m3
%endif ; HIGH_BIT_DEPTH
    paddw     m5, m0
    paddw     m5, m1
    pmaddwd   m0, m0
    pmaddwd   m1, m1
    paddd     m6, m0
    paddd     m6, m1
    add       r0, r1
    add       r2, r3
    dec       r5d
    jg .loop
    VAR2_END
    RET

INIT_XMM
cglobal pixel_var2_8x8_sse2, 5,6,8
    VAR_START 1
    mov      r5d, 4
.loop:
%ifdef HIGH_BIT_DEPTH
    mova      m0, [r0]
    mova      m1, [r0+r1*2]
    mova      m2, [r2]
    mova      m3, [r2+r3*2]
%else ; !HIGH_BIT_DEPTH
    movq      m1, [r0]
    movhps    m1, [r0+r1]
    movq      m3, [r2]
    movhps    m3, [r2+r3]
    DEINTB    0, 1, 2, 3, 7
%endif ; HIGH_BIT_DEPTH
    psubw     m0, m2
    psubw     m1, m3
    paddw     m5, m0
    paddw     m5, m1
    pmaddwd   m0, m0
    pmaddwd   m1, m1
    paddd     m6, m0
    paddd     m6, m1
    lea       r0, [r0+r1*2*SIZEOF_PIXEL]
    lea       r2, [r2+r3*2*SIZEOF_PIXEL]
    dec      r5d
    jg .loop
    VAR2_END
    RET

%ifndef HIGH_BIT_DEPTH
cglobal pixel_var2_8x8_ssse3, 5,6,8
    pxor      m5, m5    ; sum
    pxor      m6, m6    ; sum squared
    mova      m7, [hsub_mul]
    mov      r5d, 2
.loop:
    movq      m0, [r0]
    movq      m2, [r2]
    movq      m1, [r0+r1]
    movq      m3, [r2+r3]
    lea       r0, [r0+r1*2]
    lea       r2, [r2+r3*2]
    punpcklbw m0, m2
    punpcklbw m1, m3
    movq      m2, [r0]
    movq      m3, [r2]
    punpcklbw m2, m3
    movq      m3, [r0+r1]
    movq      m4, [r2+r3]
    punpcklbw m3, m4
    pmaddubsw m0, m7
    pmaddubsw m1, m7
    pmaddubsw m2, m7
    pmaddubsw m3, m7
    paddw     m5, m0
    paddw     m5, m1
    paddw     m5, m2
    paddw     m5, m3
    pmaddwd   m0, m0
    pmaddwd   m1, m1
    pmaddwd   m2, m2
    pmaddwd   m3, m3
    paddd     m6, m0
    paddd     m6, m1
    paddd     m6, m2
    paddd     m6, m3
    lea       r0, [r0+r1*2]
    lea       r2, [r2+r3*2]
    dec      r5d
    jg .loop
    VAR2_END
    RET
%endif ; !HIGH_BIT_DEPTH

;=============================================================================
; SATD
;=============================================================================

%define TRANS TRANS_SSE2

%macro JDUP 2
%if cpuflag(sse4)
    ; just use shufps on anything post conroe
    shufps %1, %2, 0
%elif cpuflag(ssse3)
    ; join 2x 32 bit and duplicate them
    ; emulating shufps is faster on conroe
    punpcklqdq %1, %2
    movsldup %1, %1
%else
    ; doesn't need to dup. sse2 does things by zero extending to words and full h_2d
    punpckldq %1, %2
%endif
%endmacro

%macro HSUMSUB 5
    pmaddubsw m%2, m%5
    pmaddubsw m%1, m%5
    pmaddubsw m%4, m%5
    pmaddubsw m%3, m%5
%endmacro

%macro DIFF_UNPACK_SSE2 5
    punpcklbw m%1, m%5
    punpcklbw m%2, m%5
    punpcklbw m%3, m%5
    punpcklbw m%4, m%5
    psubw m%1, m%2
    psubw m%3, m%4
%endmacro

%macro DIFF_SUMSUB_SSSE3 5
    HSUMSUB %1, %2, %3, %4, %5
    psubw m%1, m%2
    psubw m%3, m%4
%endmacro

%macro LOAD_DUP_2x4P 4 ; dst, tmp, 2* pointer
    movd %1, %3
    movd %2, %4
    JDUP %1, %2
%endmacro

%macro LOAD_DUP_4x8P_CONROE 8 ; 4*dst, 4*pointer
    movddup m%3, %6
    movddup m%4, %8
    movddup m%1, %5
    movddup m%2, %7
%endmacro

%macro LOAD_DUP_4x8P_PENRYN 8
    ; penryn and nehalem run punpcklqdq and movddup in different units
    movh m%3, %6
    movh m%4, %8
    punpcklqdq m%3, m%3
    movddup m%1, %5
    punpcklqdq m%4, m%4
    movddup m%2, %7
%endmacro

%macro LOAD_SUMSUB_8x2P 9
    LOAD_DUP_4x8P %1, %2, %3, %4, %6, %7, %8, %9
    DIFF_SUMSUB_SSSE3 %1, %3, %2, %4, %5
%endmacro

%macro LOAD_SUMSUB_8x4P_SSSE3 7-10 r0, r2, 0
; 4x dest, 2x tmp, 1x mul, [2* ptr], [increment?]
    LOAD_SUMSUB_8x2P %1, %2, %5, %6, %7, [%8], [%9], [%8+r1], [%9+r3]
    LOAD_SUMSUB_8x2P %3, %4, %5, %6, %7, [%8+2*r1], [%9+2*r3], [%8+r4], [%9+r5]
%if %10
    lea %8, [%8+4*r1]
    lea %9, [%9+4*r3]
%endif
%endmacro

%macro LOAD_SUMSUB_16P_SSSE3 7 ; 2*dst, 2*tmp, mul, 2*ptr
    movddup m%1, [%7]
    movddup m%2, [%7+8]
    mova m%4, [%6]
    movddup m%3, m%4
    punpckhqdq m%4, m%4
    DIFF_SUMSUB_SSSE3 %1, %3, %2, %4, %5
%endmacro

%macro LOAD_SUMSUB_16P_SSE2 7 ; 2*dst, 2*tmp, mask, 2*ptr
    movu  m%4, [%7]
    mova  m%2, [%6]
    DEINTB %1, %2, %3, %4, %5
    psubw m%1, m%3
    psubw m%2, m%4
    SUMSUB_BA w, %1, %2, %3
%endmacro

%macro LOAD_SUMSUB_16x4P 10-13 r0, r2, none
; 8x dest, 1x tmp, 1x mul, [2* ptr] [2nd tmp]
    LOAD_SUMSUB_16P %1, %5, %2, %3, %10, %11, %12
    LOAD_SUMSUB_16P %2, %6, %3, %4, %10, %11+r1, %12+r3
    LOAD_SUMSUB_16P %3, %7, %4, %9, %10, %11+2*r1, %12+2*r3
    LOAD_SUMSUB_16P %4, %8, %13, %9, %10, %11+r4, %12+r5
%endmacro

; in: r4=3*stride1, r5=3*stride2
; in: %2 = horizontal offset
; in: %3 = whether we need to increment pix1 and pix2
; clobber: m3..m7
; out: %1 = satd
%macro SATD_4x4_MMX 3
    %xdefine %%n n%1
    %assign offset %2*SIZEOF_PIXEL
    LOAD_DIFF m4, m3, none, [r0+     offset], [r2+     offset]
    LOAD_DIFF m5, m3, none, [r0+  r1+offset], [r2+  r3+offset]
    LOAD_DIFF m6, m3, none, [r0+2*r1+offset], [r2+2*r3+offset]
    LOAD_DIFF m7, m3, none, [r0+  r4+offset], [r2+  r5+offset]
%if %3
    lea  r0, [r0+4*r1]
    lea  r2, [r2+4*r3]
%endif
    HADAMARD4_2D 4, 5, 6, 7, 3, %%n
    paddw m4, m6
    SWAP %%n, 4
%endmacro

%macro SATD_8x4_SSE 8-9
%ifidn %1, sse2
    HADAMARD4_2D_SSE %2, %3, %4, %5, %6, amax
%else
    HADAMARD4_V %2, %3, %4, %5, %6
    ; doing the abs first is a slight advantage
    ABSW2 m%2, m%4, m%2, m%4, m%6, m%7
    ABSW2 m%3, m%5, m%3, m%5, m%6, m%7
    HADAMARD 1, max, %2, %4, %6, %7
%endif
%ifnidn %9, swap
    paddw m%8, m%2
%else
    SWAP %8, %2
%endif
%ifidn %1, sse2
    paddw m%8, m%4
%else
    HADAMARD 1, max, %3, %5, %6, %7
    paddw m%8, m%3
%endif
%endmacro

%macro SATD_START_MMX 0
    FIX_STRIDES r1, r3
    lea  r4, [3*r1] ; 3*stride1
    lea  r5, [3*r3] ; 3*stride2
%endmacro

%macro SATD_END_MMX 0
%ifdef HIGH_BIT_DEPTH
    HADDUW      m0, m1
    movd       eax, m0
%else ; !HIGH_BIT_DEPTH
    pshufw      m1, m0, q1032
    paddw       m0, m1
    pshufw      m1, m0, q2301
    paddw       m0, m1
    movd       eax, m0
    and        eax, 0xffff
%endif ; HIGH_BIT_DEPTH
    RET
%endmacro

; FIXME avoid the spilling of regs to hold 3*stride.
; for small blocks on x86_32, modify pixel pointer instead.

;-----------------------------------------------------------------------------
; int pixel_satd_16x16( uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
INIT_MMX mmx2
cglobal pixel_satd_16x4_internal
    SATD_4x4_MMX m2,  0, 0
    SATD_4x4_MMX m1,  4, 0
    paddw        m0, m2
    SATD_4x4_MMX m2,  8, 0
    paddw        m0, m1
    SATD_4x4_MMX m1, 12, 0
    paddw        m0, m2
    paddw        m0, m1
    ret

cglobal pixel_satd_8x8_internal
    SATD_4x4_MMX m2,  0, 0
    SATD_4x4_MMX m1,  4, 1
    paddw        m0, m2
    paddw        m0, m1
pixel_satd_8x4_internal_mmx2:
    SATD_4x4_MMX m2,  0, 0
    SATD_4x4_MMX m1,  4, 0
    paddw        m0, m2
    paddw        m0, m1
    ret

%ifdef HIGH_BIT_DEPTH
%macro SATD_MxN_MMX 3
cglobal pixel_satd_%1x%2, 4,7
    SATD_START_MMX
    pxor   m0, m0
    call pixel_satd_%1x%3_internal_mmx2
    HADDUW m0, m1
    movd  r6d, m0
%rep %2/%3-1
    pxor   m0, m0
    lea    r0, [r0+4*r1]
    lea    r2, [r2+4*r3]
    call pixel_satd_%1x%3_internal_mmx2
    movd   m2, r4
    HADDUW m0, m1
    movd   r4, m0
    add    r6, r4
    movd   r4, m2
%endrep
    movifnidn eax, r6d
    RET
%endmacro

SATD_MxN_MMX 16, 16, 4
SATD_MxN_MMX 16,  8, 4
SATD_MxN_MMX  8, 16, 8
%endif ; HIGH_BIT_DEPTH

%ifndef HIGH_BIT_DEPTH
cglobal pixel_satd_16x16, 4,6
    SATD_START_MMX
    pxor   m0, m0
%rep 3
    call pixel_satd_16x4_internal_mmx2
    lea  r0, [r0+4*r1]
    lea  r2, [r2+4*r3]
%endrep
    call pixel_satd_16x4_internal_mmx2
    HADDUW m0, m1
    movd  eax, m0
    RET

cglobal pixel_satd_16x8, 4,6
    SATD_START_MMX
    pxor   m0, m0
    call pixel_satd_16x4_internal_mmx2
    lea  r0, [r0+4*r1]
    lea  r2, [r2+4*r3]
    call pixel_satd_16x4_internal_mmx2
    SATD_END_MMX

cglobal pixel_satd_8x16, 4,6
    SATD_START_MMX
    pxor   m0, m0
    call pixel_satd_8x8_internal_mmx2
    lea  r0, [r0+4*r1]
    lea  r2, [r2+4*r3]
    call pixel_satd_8x8_internal_mmx2
    SATD_END_MMX
%endif ; !HIGH_BIT_DEPTH

cglobal pixel_satd_8x8, 4,6
    SATD_START_MMX
    pxor   m0, m0
    call pixel_satd_8x8_internal_mmx2
    SATD_END_MMX

cglobal pixel_satd_8x4, 4,6
    SATD_START_MMX
    pxor   m0, m0
    call pixel_satd_8x4_internal_mmx2
    SATD_END_MMX

cglobal pixel_satd_4x8, 4,6
    SATD_START_MMX
    SATD_4x4_MMX m0, 0, 1
    SATD_4x4_MMX m1, 0, 0
    paddw  m0, m1
    SATD_END_MMX

cglobal pixel_satd_4x4, 4,6
    SATD_START_MMX
    SATD_4x4_MMX m0, 0, 0
    SATD_END_MMX

%macro SATD_START_SSE2 2
%if cpuflag(ssse3)
    mova    %2, [hmul_8p]
%endif
    lea     r4, [3*r1]
    lea     r5, [3*r3]
    pxor    %1, %1
%endmacro

%macro SATD_END_SSE2 1
    HADDW   %1, m7
    movd   eax, %1
    RET
%endmacro

%macro BACKUP_POINTERS 0
%ifdef ARCH_X86_64
    mov    r10, r0
    mov    r11, r2
%endif
%endmacro

%macro RESTORE_AND_INC_POINTERS 0
%ifdef ARCH_X86_64
    lea     r0, [r10+8]
    lea     r2, [r11+8]
%else
    mov     r0, r0mp
    mov     r2, r2mp
    add     r0, 8
    add     r2, 8
%endif
%endmacro

;-----------------------------------------------------------------------------
; int pixel_satd_8x4( uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
%macro SATDS_SSE2 0
%if cpuflag(ssse3)
cglobal pixel_satd_4x4, 4, 6, 6
    SATD_START_MMX
    mova m4, [hmul_4p]
    LOAD_DUP_2x4P m2, m5, [r2], [r2+r3]
    LOAD_DUP_2x4P m3, m5, [r2+2*r3], [r2+r5]
    LOAD_DUP_2x4P m0, m5, [r0], [r0+r1]
    LOAD_DUP_2x4P m1, m5, [r0+2*r1], [r0+r4]
    DIFF_SUMSUB_SSSE3 0, 2, 1, 3, 4
    HADAMARD 0, sumsub, 0, 1, 2, 3
    HADAMARD 4, sumsub, 0, 1, 2, 3
    HADAMARD 1, amax, 0, 1, 2, 3
    HADDW m0, m1
    movd eax, m0
    RET
%endif

cglobal pixel_satd_4x8, 4, 6, 8
    SATD_START_MMX
%if cpuflag(ssse3)
    mova m7, [hmul_4p]
%endif
    movd m4, [r2]
    movd m5, [r2+r3]
    movd m6, [r2+2*r3]
    add r2, r5
    movd m0, [r0]
    movd m1, [r0+r1]
    movd m2, [r0+2*r1]
    add r0, r4
    movd m3, [r2+r3]
    JDUP m4, m3
    movd m3, [r0+r1]
    JDUP m0, m3
    movd m3, [r2+2*r3]
    JDUP m5, m3
    movd m3, [r0+2*r1]
    JDUP m1, m3
    DIFFOP 0, 4, 1, 5, 7
    movd m5, [r2]
    add r2, r5
    movd m3, [r0]
    add r0, r4
    movd m4, [r2]
    JDUP m6, m4
    movd m4, [r0]
    JDUP m2, m4
    movd m4, [r2+r3]
    JDUP m5, m4
    movd m4, [r0+r1]
    JDUP m3, m4
    DIFFOP 2, 6, 3, 5, 7
    SATD_8x4_SSE cpuname, 0, 1, 2, 3, 4, 5, 6, swap
    HADDW m6, m1
    movd eax, m6
    RET

cglobal pixel_satd_8x8_internal
    LOAD_SUMSUB_8x4P 0, 1, 2, 3, 4, 5, 7, r0, r2, 1
    SATD_8x4_SSE cpuname, 0, 1, 2, 3, 4, 5, 6
%%pixel_satd_8x4_internal:
    LOAD_SUMSUB_8x4P 0, 1, 2, 3, 4, 5, 7, r0, r2, 1
    SATD_8x4_SSE cpuname, 0, 1, 2, 3, 4, 5, 6
    ret

%ifdef UNIX64 ; 16x8 regresses on phenom win64, 16x16 is almost the same
cglobal pixel_satd_16x4_internal
    LOAD_SUMSUB_16x4P 0, 1, 2, 3, 4, 8, 5, 9, 6, 7, r0, r2, 11
    lea  r2, [r2+4*r3]
    lea  r0, [r0+4*r1]
    ; FIXME: this doesn't really mean ssse3, but rather selects between two different behaviors implemented with sse2?
    SATD_8x4_SSE ssse3, 0, 1, 2, 3, 6, 11, 10
    SATD_8x4_SSE ssse3, 4, 8, 5, 9, 6, 3, 10
    ret

cglobal pixel_satd_16x8, 4,6,12
    SATD_START_SSE2 m10, m7
%if notcpuflag(ssse3)
    mova m7, [pw_00ff]
%endif
    jmp %%pixel_satd_16x8_internal

cglobal pixel_satd_16x16, 4,6,12
    SATD_START_SSE2 m10, m7
%if notcpuflag(ssse3)
    mova m7, [pw_00ff]
%endif
    call pixel_satd_16x4_internal
    call pixel_satd_16x4_internal
%%pixel_satd_16x8_internal:
    call pixel_satd_16x4_internal
    call pixel_satd_16x4_internal
    SATD_END_SSE2 m10
%else
cglobal pixel_satd_16x8, 4,6,8
    SATD_START_SSE2 m6, m7
    BACKUP_POINTERS
    call pixel_satd_8x8_internal
    RESTORE_AND_INC_POINTERS
    call pixel_satd_8x8_internal
    SATD_END_SSE2 m6

cglobal pixel_satd_16x16, 4,6,8
    SATD_START_SSE2 m6, m7
    BACKUP_POINTERS
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    RESTORE_AND_INC_POINTERS
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    SATD_END_SSE2 m6
%endif

cglobal pixel_satd_8x16, 4,6,8
    SATD_START_SSE2 m6, m7
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    SATD_END_SSE2 m6

cglobal pixel_satd_8x8, 4,6,8
    SATD_START_SSE2 m6, m7
    call pixel_satd_8x8_internal
    SATD_END_SSE2 m6

cglobal pixel_satd_8x4, 4,6,8
    SATD_START_SSE2 m6, m7
    call %%pixel_satd_8x4_internal
    SATD_END_SSE2 m6
%endmacro ; SATDS_SSE2

%macro SA8D_INTER 0
%ifdef ARCH_X86_64
    %define lh m10
    %define rh m0
%else
    %define lh m0
    %define rh [esp+48]
%endif
%ifdef HIGH_BIT_DEPTH
    HADDUW  m0, m1
    paddd   lh, rh
%else
    paddusw lh, rh
%endif ; HIGH_BIT_DEPTH
%endmacro

%macro SA8D 0
%ifdef HIGH_BIT_DEPTH
    %define vertical 1
%else ; sse2 doesn't seem to like the horizontal way of doing things
    %define vertical (cpuflags == cpuflags_sse2)
%endif

%ifdef ARCH_X86_64
;-----------------------------------------------------------------------------
; int pixel_sa8d_8x8( uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
cglobal pixel_sa8d_8x8_internal
    lea  r10, [r0+4*r1]
    lea  r11, [r2+4*r3]
    LOAD_SUMSUB_8x4P 0, 1, 2, 8, 5, 6, 7, r0, r2
    LOAD_SUMSUB_8x4P 4, 5, 3, 9, 11, 6, 7, r10, r11
%if vertical
    HADAMARD8_2D 0, 1, 2, 8, 4, 5, 3, 9, 6, amax
%else ; non-sse2
    HADAMARD4_V 0, 1, 2, 8, 6
    HADAMARD4_V 4, 5, 3, 9, 6
    SUMSUB_BADC w, 0, 4, 1, 5, 6
    HADAMARD 2, sumsub, 0, 4, 6, 11
    HADAMARD 2, sumsub, 1, 5, 6, 11
    SUMSUB_BADC w, 2, 3, 8, 9, 6
    HADAMARD 2, sumsub, 2, 3, 6, 11
    HADAMARD 2, sumsub, 8, 9, 6, 11
    HADAMARD 1, amax, 0, 4, 6, 11
    HADAMARD 1, amax, 1, 5, 6, 4
    HADAMARD 1, amax, 2, 3, 6, 4
    HADAMARD 1, amax, 8, 9, 6, 4
%endif
    paddw m0, m1
    paddw m0, m2
    paddw m0, m8
    SAVE_MM_PERMUTATION
    ret

cglobal pixel_sa8d_8x8, 4,6,12
    FIX_STRIDES r1, r3
    lea  r4, [3*r1]
    lea  r5, [3*r3]
%if vertical == 0
    mova m7, [hmul_8p]
%endif
    call pixel_sa8d_8x8_internal
%ifdef HIGH_BIT_DEPTH
    HADDUW m0, m1
%else
    HADDW m0, m1
%endif ; HIGH_BIT_DEPTH
    movd eax, m0
    add eax, 1
    shr eax, 1
    RET

cglobal pixel_sa8d_16x16, 4,6,12
    FIX_STRIDES r1, r3
    lea  r4, [3*r1]
    lea  r5, [3*r3]
%if vertical == 0
    mova m7, [hmul_8p]
%endif
    call pixel_sa8d_8x8_internal ; pix[0]
    add  r2, 8*SIZEOF_PIXEL
    add  r0, 8*SIZEOF_PIXEL
%ifdef HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova m10, m0
    call pixel_sa8d_8x8_internal ; pix[8]
    lea  r2, [r2+8*r3]
    lea  r0, [r0+8*r1]
    SA8D_INTER
    call pixel_sa8d_8x8_internal ; pix[8*stride+8]
    sub  r2, 8*SIZEOF_PIXEL
    sub  r0, 8*SIZEOF_PIXEL
    SA8D_INTER
    call pixel_sa8d_8x8_internal ; pix[8*stride]
    SA8D_INTER
    SWAP 0, 10
%ifndef HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    movd eax, m0
    add  eax, 1
    shr  eax, 1
    RET

%else ; ARCH_X86_32
%if mmsize == 16
cglobal pixel_sa8d_8x8_internal
    %define spill0 [esp+4]
    %define spill1 [esp+20]
    %define spill2 [esp+36]
%if vertical
    LOAD_DIFF_8x4P 0, 1, 2, 3, 4, 5, 6, r0, r2, 1
    HADAMARD4_2D 0, 1, 2, 3, 4
    movdqa spill0, m3
    LOAD_DIFF_8x4P 4, 5, 6, 7, 3, 3, 2, r0, r2, 1
    HADAMARD4_2D 4, 5, 6, 7, 3
    HADAMARD2_2D 0, 4, 1, 5, 3, qdq, amax
    movdqa m3, spill0
    paddw m0, m1
    HADAMARD2_2D 2, 6, 3, 7, 5, qdq, amax
%else ; mmsize == 8
    mova m7, [hmul_8p]
    LOAD_SUMSUB_8x4P 0, 1, 2, 3, 5, 6, 7, r0, r2, 1
    ; could do first HADAMARD4_V here to save spilling later
    ; surprisingly, not a win on conroe or even p4
    mova spill0, m2
    mova spill1, m3
    mova spill2, m1
    SWAP 1, 7
    LOAD_SUMSUB_8x4P 4, 5, 6, 7, 2, 3, 1, r0, r2, 1
    HADAMARD4_V 4, 5, 6, 7, 3
    mova m1, spill2
    mova m2, spill0
    mova m3, spill1
    mova spill0, m6
    mova spill1, m7
    HADAMARD4_V 0, 1, 2, 3, 7
    SUMSUB_BADC w, 0, 4, 1, 5, 7
    HADAMARD 2, sumsub, 0, 4, 7, 6
    HADAMARD 2, sumsub, 1, 5, 7, 6
    HADAMARD 1, amax, 0, 4, 7, 6
    HADAMARD 1, amax, 1, 5, 7, 6
    mova m6, spill0
    mova m7, spill1
    paddw m0, m1
    SUMSUB_BADC w, 2, 6, 3, 7, 4
    HADAMARD 2, sumsub, 2, 6, 4, 5
    HADAMARD 2, sumsub, 3, 7, 4, 5
    HADAMARD 1, amax, 2, 6, 4, 5
    HADAMARD 1, amax, 3, 7, 4, 5
%endif ; sse2/non-sse2
    paddw m0, m2
    paddw m0, m3
    SAVE_MM_PERMUTATION
    ret
%endif ; ifndef mmx2

cglobal pixel_sa8d_8x8, 4,7
    FIX_STRIDES r1, r3
    mov    r6, esp
    and   esp, ~15
    sub   esp, 48
    lea    r4, [3*r1]
    lea    r5, [3*r3]
    call pixel_sa8d_8x8_internal
%ifdef HIGH_BIT_DEPTH
    HADDUW m0, m1
%else
    HADDW  m0, m1
%endif ; HIGH_BIT_DEPTH
    movd  eax, m0
    add   eax, 1
    shr   eax, 1
    mov   esp, r6
    RET

cglobal pixel_sa8d_16x16, 4,7
    FIX_STRIDES r1, r3
    mov  r6, esp
    and  esp, ~15
    sub  esp, 64
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    call pixel_sa8d_8x8_internal
%if mmsize == 8
    lea  r0, [r0+4*r1]
    lea  r2, [r2+4*r3]
%endif
%ifdef HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal
    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 8*SIZEOF_PIXEL
    add  r2, 8*SIZEOF_PIXEL
    SA8D_INTER
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal
%if mmsize == 8
    lea  r0, [r0+4*r1]
    lea  r2, [r2+4*r3]
%else
    SA8D_INTER
%endif
    mova [esp+64-mmsize], m0
    call pixel_sa8d_8x8_internal
%ifdef HIGH_BIT_DEPTH
    SA8D_INTER
%else ; !HIGH_BIT_DEPTH
    paddusw m0, [esp+64-mmsize]
%if mmsize == 16
    HADDUW m0, m1
%else
    mova m2, [esp+48]
    pxor m7, m7
    mova m1, m0
    mova m3, m2
    punpcklwd m0, m7
    punpckhwd m1, m7
    punpcklwd m2, m7
    punpckhwd m3, m7
    paddd m0, m1
    paddd m2, m3
    paddd m0, m2
    HADDD m0, m1
%endif
%endif ; HIGH_BIT_DEPTH
    movd eax, m0
    add  eax, 1
    shr  eax, 1
    mov  esp, r6
    RET
%endif ; !ARCH_X86_64
%endmacro ; SA8D

;=============================================================================
; INTRA SATD
;=============================================================================

%macro INTRA_SA8D_SSE2 0
%ifdef ARCH_X86_64
;-----------------------------------------------------------------------------
; void intra_sa8d_x3_8x8_core( uint8_t *fenc, int16_t edges[2][8], int *res )
;-----------------------------------------------------------------------------
cglobal intra_sa8d_x3_8x8_core, 3,3,16
    ; 8x8 hadamard
    pxor        m8, m8
    movq        m0, [r0+0*FENC_STRIDE]
    movq        m1, [r0+1*FENC_STRIDE]
    movq        m2, [r0+2*FENC_STRIDE]
    movq        m3, [r0+3*FENC_STRIDE]
    movq        m4, [r0+4*FENC_STRIDE]
    movq        m5, [r0+5*FENC_STRIDE]
    movq        m6, [r0+6*FENC_STRIDE]
    movq        m7, [r0+7*FENC_STRIDE]
    punpcklbw   m0, m8
    punpcklbw   m1, m8
    punpcklbw   m2, m8
    punpcklbw   m3, m8
    punpcklbw   m4, m8
    punpcklbw   m5, m8
    punpcklbw   m6, m8
    punpcklbw   m7, m8

    HADAMARD8_2D 0,  1,  2,  3,  4,  5,  6,  7,  8

    ; dc
    movzx       r0d, word [r1+0]
    add         r0w, word [r1+16]
    add         r0d, 8
    and         r0d, -16
    shl         r0d, 2

    pxor        m15, m15
    movdqa      m8,  m2
    movdqa      m9,  m3
    movdqa      m10, m4
    movdqa      m11, m5
    ABSW2       m8,  m9,  m8,  m9,  m12, m13
    ABSW2       m10, m11, m10, m11, m12, m13
    paddusw     m8,  m10
    paddusw     m9,  m11
    ABSW2       m10, m11, m6,  m7,  m6,  m7
    ABSW        m15, m1,  m1
    paddusw     m10, m11
    paddusw     m8,  m9
    paddusw     m15, m10
    paddusw     m15, m8

    movdqa      m8,  [r1+0] ; left edge
    movd        m9,  r0d
    psllw       m8,  3
    psubw       m8,  m0
    psubw       m9,  m0
    ABSW2       m8,  m9,  m8,  m9,  m10, m11 ; 1x8 sum
    paddusw     m14, m15, m8
    paddusw     m15, m9
    punpcklwd   m0,  m1
    punpcklwd   m2,  m3
    punpcklwd   m4,  m5
    punpcklwd   m6,  m7
    punpckldq   m0,  m2
    punpckldq   m4,  m6
    punpcklqdq  m0,  m4 ; transpose
    movdqa      m1,  [r1+16] ; top edge
    psllw       m1,  3
    psrldq      m2,  m15, 2  ; 8x7 sum
    psubw       m0,  m1  ; 8x1 sum
    ABSW        m0,  m0,  m1
    paddusw     m2,  m0

    ; 3x HADDW
    movdqa      m7,  [pw_1]
    pmaddwd     m2,  m7
    pmaddwd     m14, m7
    pmaddwd     m15, m7
    punpckhdq   m3,  m2, m14
    punpckldq   m2,  m14
    pshufd      m5,  m15, q3311
    paddd       m2,  m3
    paddd       m5,  m15
    punpckhqdq  m3,  m2, m5
    punpcklqdq  m2,  m5
    pavgw       m3,  m2
    pxor        m0,  m0
    pavgw       m3,  m0
    movq      [r2],  m3 ; i8x8_v, i8x8_h
    psrldq      m3,  8
    movd    [r2+8],  m3 ; i8x8_dc
    RET
%endif ; ARCH_X86_64
%endmacro ; INTRA_SA8D_SSE2

; in: r0 = fenc
; out: m0..m3 = hadamard coefs
INIT_MMX
cglobal hadamard_load
; not really a global, but otherwise cycles get attributed to the wrong function in profiling
    pxor        m7, m7
    movd        m0, [r0+0*FENC_STRIDE]
    movd        m1, [r0+1*FENC_STRIDE]
    movd        m2, [r0+2*FENC_STRIDE]
    movd        m3, [r0+3*FENC_STRIDE]
    punpcklbw   m0, m7
    punpcklbw   m1, m7
    punpcklbw   m2, m7
    punpcklbw   m3, m7
    HADAMARD4_2D 0, 1, 2, 3, 4
    SAVE_MM_PERMUTATION
    ret

%macro SCALAR_SUMSUB 4
    add %1, %2
    add %3, %4
    add %2, %2
    add %4, %4
    sub %2, %1
    sub %4, %3
%endmacro

%macro SCALAR_HADAMARD_LEFT 5 ; y, 4x tmp
%ifnidn %1, 0
    shl         %1d, 5 ; log(FDEC_STRIDE)
%endif
    movzx       %2d, byte [r1+%1-1+0*FDEC_STRIDE]
    movzx       %3d, byte [r1+%1-1+1*FDEC_STRIDE]
    movzx       %4d, byte [r1+%1-1+2*FDEC_STRIDE]
    movzx       %5d, byte [r1+%1-1+3*FDEC_STRIDE]
%ifnidn %1, 0
    shr         %1d, 5
%endif
    SCALAR_SUMSUB %2d, %3d, %4d, %5d
    SCALAR_SUMSUB %2d, %4d, %3d, %5d
    mov         [left_1d+2*%1+0], %2w
    mov         [left_1d+2*%1+2], %3w
    mov         [left_1d+2*%1+4], %4w
    mov         [left_1d+2*%1+6], %5w
%endmacro

%macro SCALAR_HADAMARD_TOP 5 ; x, 4x tmp
    movzx       %2d, byte [r1+%1-FDEC_STRIDE+0]
    movzx       %3d, byte [r1+%1-FDEC_STRIDE+1]
    movzx       %4d, byte [r1+%1-FDEC_STRIDE+2]
    movzx       %5d, byte [r1+%1-FDEC_STRIDE+3]
    SCALAR_SUMSUB %2d, %3d, %4d, %5d
    SCALAR_SUMSUB %2d, %4d, %3d, %5d
    mov         [top_1d+2*%1+0], %2w
    mov         [top_1d+2*%1+2], %3w
    mov         [top_1d+2*%1+4], %4w
    mov         [top_1d+2*%1+6], %5w
%endmacro

%macro SUM_MM_X3 8 ; 3x sum, 4x tmp, op
    pxor        %7, %7
    pshufw      %4, %1, q1032
    pshufw      %5, %2, q1032
    pshufw      %6, %3, q1032
    paddw       %1, %4
    paddw       %2, %5
    paddw       %3, %6
    punpcklwd   %1, %7
    punpcklwd   %2, %7
    punpcklwd   %3, %7
    pshufw      %4, %1, q1032
    pshufw      %5, %2, q1032
    pshufw      %6, %3, q1032
    %8          %1, %4
    %8          %2, %5
    %8          %3, %6
%endmacro

%macro CLEAR_SUMS 0
%ifdef ARCH_X86_64
    mov   qword [sums+0], 0
    mov   qword [sums+8], 0
    mov   qword [sums+16], 0
%else
    pxor  m7, m7
    movq  [sums+0], m7
    movq  [sums+8], m7
    movq  [sums+16], m7
%endif
%endmacro

; in: m1..m3
; out: m7
; clobber: m4..m6
%macro SUM3x4 0
    ABSW2       m4, m5, m1, m2, m1, m2
    ABSW        m7, m3, m3
    paddw       m4, m5
    paddw       m7, m4
%endmacro

; in: m0..m3 (4x4), m7 (3x4)
; out: m0 v, m4 h, m5 dc
; clobber: m6
%macro SUM4x3 3 ; dc, left, top
    movq        m4, %2
    movd        m5, %1
    psllw       m4, 2
    psubw       m4, m0
    psubw       m5, m0
    punpcklwd   m0, m1
    punpcklwd   m2, m3
    punpckldq   m0, m2 ; transpose
    movq        m1, %3
    psllw       m1, 2
    psubw       m0, m1
    ABSW2       m4, m5, m4, m5, m2, m3 ; 1x4 sum
    ABSW        m0, m0, m1 ; 4x1 sum
%endmacro

%macro INTRA_SATDS_MMX 0
;-----------------------------------------------------------------------------
; void intra_satd_x3_4x4( uint8_t *fenc, uint8_t *fdec, int *res )
;-----------------------------------------------------------------------------
cglobal intra_satd_x3_4x4, 2,6
%ifdef ARCH_X86_64
    ; stack is 16 byte aligned because abi says so
    %define  top_1d  rsp-8  ; size 8
    %define  left_1d rsp-16 ; size 8
    %define  t0 r10
%else
    ; stack is 16 byte aligned at least in gcc, and we've pushed 3 regs + return address, so it's still aligned
    SUB         esp, 16
    %define  top_1d  esp+8
    %define  left_1d esp
    %define  t0 r2
%endif

    call hadamard_load
    SCALAR_HADAMARD_LEFT 0, r0, r3, r4, r5
    mov         t0d, r0d
    SCALAR_HADAMARD_TOP  0, r0, r3, r4, r5
    lea         t0d, [t0d + r0d + 4]
    and         t0d, -8
    shl         t0d, 1 ; dc

    SUM3x4
    SUM4x3 t0d, [left_1d], [top_1d]
    paddw       m4, m7
    paddw       m5, m7
    movq        m1, m5
    psrlq       m1, 16  ; 4x3 sum
    paddw       m0, m1

    SUM_MM_X3   m0, m4, m5, m1, m2, m3, m6, pavgw
%ifndef ARCH_X86_64
    mov         r2,  r2mp
%endif
    movd        [r2+0], m0 ; i4x4_v satd
    movd        [r2+4], m4 ; i4x4_h satd
    movd        [r2+8], m5 ; i4x4_dc satd
%ifndef ARCH_X86_64
    ADD         esp, 16
%endif
    RET

%ifdef ARCH_X86_64
    %define  t0 r10
    %define  t2 r11
%else
    %define  t0 r0
    %define  t2 r2
%endif

;-----------------------------------------------------------------------------
; void intra_satd_x3_16x16( uint8_t *fenc, uint8_t *fdec, int *res )
;-----------------------------------------------------------------------------
cglobal intra_satd_x3_16x16, 0,7
%ifdef ARCH_X86_64
    %assign  stack_pad  88
%else
    %assign  stack_pad  88 + ((stack_offset+88+4)&15)
%endif
    ; not really needed on x86_64, just shuts up valgrind about storing data below the stack across a function call
    SUB         rsp, stack_pad
%define sums    rsp+64 ; size 24
%define top_1d  rsp+32 ; size 32
%define left_1d rsp    ; size 32
    movifnidn   r1,  r1mp
    CLEAR_SUMS

    ; 1D hadamards
    xor         t2d, t2d
    mov         t0d, 12
.loop_edge:
    SCALAR_HADAMARD_LEFT t0, r3, r4, r5, r6
    add         t2d, r3d
    SCALAR_HADAMARD_TOP  t0, r3, r4, r5, r6
    add         t2d, r3d
    sub         t0d, 4
    jge .loop_edge
    shr         t2d, 1
    add         t2d, 8
    and         t2d, -16 ; dc

    ; 2D hadamards
    movifnidn   r0,  r0mp
    xor         r3d, r3d
.loop_y:
    xor         r4d, r4d
.loop_x:
    call hadamard_load

    SUM3x4
    SUM4x3 t2d, [left_1d+8*r3], [top_1d+8*r4]
    pavgw       m4, m7
    pavgw       m5, m7
    paddw       m0, [sums+0]  ; i16x16_v satd
    paddw       m4, [sums+8]  ; i16x16_h satd
    paddw       m5, [sums+16] ; i16x16_dc satd
    movq        [sums+0], m0
    movq        [sums+8], m4
    movq        [sums+16], m5

    add         r0, 4
    inc         r4d
    cmp         r4d, 4
    jl  .loop_x
    add         r0, 4*FENC_STRIDE-16
    inc         r3d
    cmp         r3d, 4
    jl  .loop_y

; horizontal sum
    movifnidn   r2, r2mp
    movq        m2, [sums+16]
    movq        m1, [sums+8]
    movq        m0, [sums+0]
    movq        m7, m2
    SUM_MM_X3   m0, m1, m2, m3, m4, m5, m6, paddd
    psrld       m0, 1
    pslld       m7, 16
    psrld       m7, 16
    paddd       m0, m2
    psubd       m0, m7
    movd        [r2+8], m2 ; i16x16_dc satd
    movd        [r2+4], m1 ; i16x16_h satd
    movd        [r2+0], m0 ; i16x16_v satd
    ADD         rsp, stack_pad
    RET

;-----------------------------------------------------------------------------
; void intra_satd_x3_8x8c( uint8_t *fenc, uint8_t *fdec, int *res )
;-----------------------------------------------------------------------------
cglobal intra_satd_x3_8x8c, 0,6
    ; not really needed on x86_64, just shuts up valgrind about storing data below the stack across a function call
    SUB          rsp, 72
%define  sums    rsp+48 ; size 24
%define  dc_1d   rsp+32 ; size 16
%define  top_1d  rsp+16 ; size 16
%define  left_1d rsp    ; size 16
    movifnidn   r1,  r1mp
    CLEAR_SUMS

    ; 1D hadamards
    mov         t0d, 4
.loop_edge:
    SCALAR_HADAMARD_LEFT t0, t2, r3, r4, r5
    SCALAR_HADAMARD_TOP  t0, t2, r3, r4, r5
    sub         t0d, 4
    jge .loop_edge

    ; dc
    movzx       t2d, word [left_1d+0]
    movzx       r3d, word [top_1d+0]
    movzx       r4d, word [left_1d+8]
    movzx       r5d, word [top_1d+8]
    add         t2d, r3d
    lea         r3, [r4 + r5]
    lea         t2, [2*t2 + 8]
    lea         r3, [2*r3 + 8]
    lea         r4, [4*r4 + 8]
    lea         r5, [4*r5 + 8]
    and         t2d, -16 ; tl
    and         r3d, -16 ; br
    and         r4d, -16 ; bl
    and         r5d, -16 ; tr
    mov         [dc_1d+ 0], t2d ; tl
    mov         [dc_1d+ 4], r5d ; tr
    mov         [dc_1d+ 8], r4d ; bl
    mov         [dc_1d+12], r3d ; br
    lea         r5, [dc_1d]

    ; 2D hadamards
    movifnidn   r0,  r0mp
    movifnidn   r2,  r2mp
    xor         r3d, r3d
.loop_y:
    xor         r4d, r4d
.loop_x:
    call hadamard_load

    SUM3x4
    SUM4x3 [r5+4*r4], [left_1d+8*r3], [top_1d+8*r4]
    pavgw       m4, m7
    pavgw       m5, m7
    paddw       m0, [sums+16] ; i4x4_v satd
    paddw       m4, [sums+8]  ; i4x4_h satd
    paddw       m5, [sums+0]  ; i4x4_dc satd
    movq        [sums+16], m0
    movq        [sums+8], m4
    movq        [sums+0], m5

    add         r0, 4
    inc         r4d
    cmp         r4d, 2
    jl  .loop_x
    add         r0, 4*FENC_STRIDE-8
    add         r5, 8
    inc         r3d
    cmp         r3d, 2
    jl  .loop_y

; horizontal sum
    movq        m0, [sums+0]
    movq        m1, [sums+8]
    movq        m2, [sums+16]
    movq        m7, m0
    psrlq       m7, 15
    paddw       m2, m7
    SUM_MM_X3   m0, m1, m2, m3, m4, m5, m6, paddd
    psrld       m2, 1
    movd        [r2+0], m0 ; i8x8c_dc satd
    movd        [r2+4], m1 ; i8x8c_h satd
    movd        [r2+8], m2 ; i8x8c_v satd
    ADD         rsp, 72
    RET
%endmacro ; INTRA_SATDS_MMX


; in:  r0=pix, r1=stride, r2=stride*3, r3=tmp, m6=mask_ac4, m7=0
; out: [tmp]=hadamard4, m0=satd
INIT_MMX mmx2
cglobal hadamard_ac_4x4
%ifdef HIGH_BIT_DEPTH
    mova      m0, [r0]
    mova      m1, [r0+r1]
    mova      m2, [r0+r1*2]
    mova      m3, [r0+r2]
%else ; !HIGH_BIT_DEPTH
    movh      m0, [r0]
    movh      m1, [r0+r1]
    movh      m2, [r0+r1*2]
    movh      m3, [r0+r2]
    punpcklbw m0, m7
    punpcklbw m1, m7
    punpcklbw m2, m7
    punpcklbw m3, m7
%endif ; HIGH_BIT_DEPTH
    HADAMARD4_2D 0, 1, 2, 3, 4
    mova [r3],    m0
    mova [r3+8],  m1
    mova [r3+16], m2
    mova [r3+24], m3
    ABSW      m0, m0, m4
    ABSW      m1, m1, m4
    pand      m0, m6
    ABSW      m2, m2, m4
    ABSW      m3, m3, m4
    paddw     m0, m1
    paddw     m2, m3
    paddw     m0, m2
    SAVE_MM_PERMUTATION
    ret

cglobal hadamard_ac_2x2max
    mova      m0, [r3+0x00]
    mova      m1, [r3+0x20]
    mova      m2, [r3+0x40]
    mova      m3, [r3+0x60]
    sub       r3, 8
    SUMSUB_BADC w, 0, 1, 2, 3, 4
    ABSW2 m0, m2, m0, m2, m4, m5
    ABSW2 m1, m3, m1, m3, m4, m5
    HADAMARD 0, max, 0, 2, 4, 5
    HADAMARD 0, max, 1, 3, 4, 5
%ifdef HIGH_BIT_DEPTH
    pmaddwd   m0, m7
    pmaddwd   m1, m7
    paddd     m6, m0
    paddd     m6, m1
%else ; !HIGH_BIT_DEPTH
    paddw     m7, m0
    paddw     m7, m1
%endif ; HIGH_BIT_DEPTH
    SAVE_MM_PERMUTATION
    ret

%macro AC_PREP 2
%ifdef HIGH_BIT_DEPTH
    pmaddwd %1, %2
%endif
%endmacro

%macro AC_PADD 3
%ifdef HIGH_BIT_DEPTH
    AC_PREP %2, %3
    paddd   %1, %2
%else
    paddw   %1, %2
%endif ; HIGH_BIT_DEPTH
%endmacro

cglobal hadamard_ac_8x8
    mova      m6, [mask_ac4]
%ifdef HIGH_BIT_DEPTH
    mova      m7, [pw_1]
%else
    pxor      m7, m7
%endif ; HIGH_BIT_DEPTH
    call hadamard_ac_4x4_mmx2
    add       r0, 4*SIZEOF_PIXEL
    add       r3, 32
    mova      m5, m0
    AC_PREP   m5, m7
    call hadamard_ac_4x4_mmx2
    lea       r0, [r0+4*r1]
    add       r3, 64
    AC_PADD   m5, m0, m7
    call hadamard_ac_4x4_mmx2
    sub       r0, 4*SIZEOF_PIXEL
    sub       r3, 32
    AC_PADD   m5, m0, m7
    call hadamard_ac_4x4_mmx2
    AC_PADD   m5, m0, m7
    sub       r3, 40
    mova [rsp+gprsize+8], m5 ; save satd
%ifdef HIGH_BIT_DEPTH
    pxor      m6, m6
%endif
%rep 3
    call hadamard_ac_2x2max_mmx2
%endrep
    mova      m0, [r3+0x00]
    mova      m1, [r3+0x20]
    mova      m2, [r3+0x40]
    mova      m3, [r3+0x60]
    SUMSUB_BADC w, 0, 1, 2, 3, 4
    HADAMARD 0, sumsub, 0, 2, 4, 5
    ABSW2 m1, m3, m1, m3, m4, m5
    ABSW2 m0, m2, m0, m2, m4, m5
    HADAMARD 0, max, 1, 3, 4, 5
%ifdef HIGH_BIT_DEPTH
    pand      m0, [mask_ac4]
    pmaddwd   m1, m7
    pmaddwd   m0, m7
    pmaddwd   m2, m7
    paddd     m6, m1
    paddd     m0, m2
    paddd     m6, m6
    paddd     m0, m6
    SWAP       0,  6
%else ; !HIGH_BIT_DEPTH
    pand      m6, m0
    paddw     m7, m1
    paddw     m6, m2
    paddw     m7, m7
    paddw     m6, m7
%endif ; HIGH_BIT_DEPTH
    mova [rsp+gprsize], m6 ; save sa8d
    SWAP       0,  6
    SAVE_MM_PERMUTATION
    ret

%macro HADAMARD_AC_WXH_SUM_MMX 2
    mova    m1, [rsp+1*mmsize]
%ifdef HIGH_BIT_DEPTH
%if %1*%2 >= 128
    paddd   m0, [rsp+2*mmsize]
    paddd   m1, [rsp+3*mmsize]
%endif
%if %1*%2 == 256
    mova    m2, [rsp+4*mmsize]
    paddd   m1, [rsp+5*mmsize]
    paddd   m2, [rsp+6*mmsize]
    mova    m3, m0
    paddd   m1, [rsp+7*mmsize]
    paddd   m0, m2
%endif
    psrld   m0, 1
    HADDD   m0, m2
    psrld   m1, 1
    HADDD   m1, m3
%else ; !HIGH_BIT_DEPTH
%if %1*%2 >= 128
    paddusw m0, [rsp+2*mmsize]
    paddusw m1, [rsp+3*mmsize]
%endif
%if %1*%2 == 256
    mova    m2, [rsp+4*mmsize]
    paddusw m1, [rsp+5*mmsize]
    paddusw m2, [rsp+6*mmsize]
    mova    m3, m0
    paddusw m1, [rsp+7*mmsize]
    pxor    m3, m2
    pand    m3, [pw_1]
    pavgw   m0, m2
    psubusw m0, m3
    HADDUW  m0, m2
%else
    psrlw   m0, 1
    HADDW   m0, m2
%endif
    psrlw   m1, 1
    HADDW   m1, m3
%endif ; HIGH_BIT_DEPTH
%endmacro

%macro HADAMARD_AC_WXH_MMX 2
cglobal pixel_hadamard_ac_%1x%2, 2,4
    %assign pad 16-gprsize-(stack_offset&15)
    %define ysub r1
    FIX_STRIDES r1
    sub  rsp, 16+128+pad
    lea  r2, [r1*3]
    lea  r3, [rsp+16]
    call hadamard_ac_8x8_mmx2
%if %2==16
    %define ysub r2
    lea  r0, [r0+r1*4]
    sub  rsp, 16
    call hadamard_ac_8x8_mmx2
%endif
%if %1==16
    neg  ysub
    sub  rsp, 16
    lea  r0, [r0+ysub*4+8*SIZEOF_PIXEL]
    neg  ysub
    call hadamard_ac_8x8_mmx2
%if %2==16
    lea  r0, [r0+r1*4]
    sub  rsp, 16
    call hadamard_ac_8x8_mmx2
%endif
%endif
    HADAMARD_AC_WXH_SUM_MMX %1, %2
    movd edx, m0
    movd eax, m1
    shr  edx, 1
%ifdef ARCH_X86_64
    shl  rdx, 32
    add  rax, rdx
%endif
    add  rsp, 128+%1*%2/4+pad
    RET
%endmacro ; HADAMARD_AC_WXH_MMX

HADAMARD_AC_WXH_MMX 16, 16
HADAMARD_AC_WXH_MMX  8, 16
HADAMARD_AC_WXH_MMX 16,  8
HADAMARD_AC_WXH_MMX  8,  8

%macro LOAD_INC_8x4W_SSE2 5
%ifdef HIGH_BIT_DEPTH
    movu      m%1, [r0]
    movu      m%2, [r0+r1]
    movu      m%3, [r0+r1*2]
    movu      m%4, [r0+r2]
%ifidn %1, 0
    lea       r0, [r0+r1*4]
%endif
%else ; !HIGH_BIT_DEPTH
    movh      m%1, [r0]
    movh      m%2, [r0+r1]
    movh      m%3, [r0+r1*2]
    movh      m%4, [r0+r2]
%ifidn %1, 0
    lea       r0, [r0+r1*4]
%endif
    punpcklbw m%1, m%5
    punpcklbw m%2, m%5
    punpcklbw m%3, m%5
    punpcklbw m%4, m%5
%endif ; HIGH_BIT_DEPTH
%endmacro

%macro LOAD_INC_8x4W_SSSE3 5
    LOAD_DUP_4x8P %3, %4, %1, %2, [r0+r1*2], [r0+r2], [r0], [r0+r1]
%ifidn %1, 0
    lea       r0, [r0+r1*4]
%endif
    HSUMSUB %1, %2, %3, %4, %5
%endmacro

%macro HADAMARD_AC_SSE2 0
; in:  r0=pix, r1=stride, r2=stride*3
; out: [esp+16]=sa8d, [esp+32]=satd, r0+=stride*4
cglobal hadamard_ac_8x8
%ifdef ARCH_X86_64
    %define spill0 m8
    %define spill1 m9
    %define spill2 m10
%else
    %define spill0 [rsp+gprsize]
    %define spill1 [rsp+gprsize+16]
    %define spill2 [rsp+gprsize+32]
%endif
%ifdef HIGH_BIT_DEPTH
    %define vertical 1
%elif cpuflag(ssse3)
    %define vertical 0
    ;LOAD_INC loads sumsubs
    mova      m7, [hmul_8p]
%else
    %define vertical 1
    ;LOAD_INC only unpacks to words
    pxor      m7, m7
%endif
    LOAD_INC_8x4W 0, 1, 2, 3, 7
%if vertical
    HADAMARD4_2D_SSE 0, 1, 2, 3, 4
%else
    HADAMARD4_V 0, 1, 2, 3, 4
%endif
    mova  spill0, m1
    SWAP 1, 7
    LOAD_INC_8x4W 4, 5, 6, 7, 1
%if vertical
    HADAMARD4_2D_SSE 4, 5, 6, 7, 1
%else
    HADAMARD4_V 4, 5, 6, 7, 1
    ; FIXME SWAP
    mova      m1, spill0
    mova      spill0, m6
    mova      spill1, m7
    HADAMARD 1, sumsub, 0, 1, 6, 7
    HADAMARD 1, sumsub, 2, 3, 6, 7
    mova      m6, spill0
    mova      m7, spill1
    mova      spill0, m1
    mova      spill1, m0
    HADAMARD 1, sumsub, 4, 5, 1, 0
    HADAMARD 1, sumsub, 6, 7, 1, 0
    mova      m0, spill1
%endif
    mova  spill1, m2
    mova  spill2, m3
    ABSW      m1, m0, m0
    ABSW      m2, m4, m4
    ABSW      m3, m5, m5
    paddw     m1, m2
    SUMSUB_BA w, 0, 4
%if vertical
    pand      m1, [mask_ac4]
%else
    pand      m1, [mask_ac4b]
%endif
    AC_PREP   m1, [pw_1]
    ABSW      m2, spill0
    AC_PADD   m1, m3, [pw_1]
    ABSW      m3, spill1
    AC_PADD   m1, m2, [pw_1]
    ABSW      m2, spill2
    AC_PADD   m1, m3, [pw_1]
    ABSW      m3, m6, m6
    AC_PADD   m1, m2, [pw_1]
    ABSW      m2, m7, m7
    AC_PADD   m1, m3, [pw_1]
    mova      m3, m7
    AC_PADD   m1, m2, [pw_1]
    mova      m2, m6
    psubw     m7, spill2
    paddw     m3, spill2
    mova  [rsp+gprsize+32], m1 ; save satd
    mova      m1, m5
    psubw     m6, spill1
    paddw     m2, spill1
    psubw     m5, spill0
    paddw     m1, spill0
    %assign %%x 2
%if vertical
    %assign %%x 4
%endif
    mova  spill1, m4
    HADAMARD %%x, amax, 3, 7, 4
    HADAMARD %%x, amax, 2, 6, 7, 4
    mova      m4, spill1
    HADAMARD %%x, amax, 1, 5, 6, 7
    HADAMARD %%x, sumsub, 0, 4, 5, 6
    AC_PREP   m2, [pw_1]
    AC_PADD   m2, m3, [pw_1]
    AC_PADD   m2, m1, [pw_1]
%ifdef HIGH_BIT_DEPTH
    paddd     m2, m2
%else
    paddw     m2, m2
%endif ; HIGH_BIT_DEPTH
    ABSW      m4, m4, m7
    pand      m0, [mask_ac8]
    ABSW      m0, m0, m7
    AC_PADD   m2, m4, [pw_1]
    AC_PADD   m2, m0, [pw_1]
    mova [rsp+gprsize+16], m2 ; save sa8d
    SWAP       0, 2
    SAVE_MM_PERMUTATION
    ret

HADAMARD_AC_WXH_SSE2 16, 16
HADAMARD_AC_WXH_SSE2  8, 16
HADAMARD_AC_WXH_SSE2 16,  8
HADAMARD_AC_WXH_SSE2  8,  8
%endmacro ; HADAMARD_AC_SSE2

%macro HADAMARD_AC_WXH_SUM_SSE2 2
    mova    m1, [rsp+2*mmsize]
%ifdef HIGH_BIT_DEPTH
%if %1*%2 >= 128
    paddd   m0, [rsp+3*mmsize]
    paddd   m1, [rsp+4*mmsize]
%endif
%if %1*%2 == 256
    paddd   m0, [rsp+5*mmsize]
    paddd   m1, [rsp+6*mmsize]
    paddd   m0, [rsp+7*mmsize]
    paddd   m1, [rsp+8*mmsize]
    psrld   m0, 1
%endif
    HADDD   m0, m2
    HADDD   m1, m3
%else ; !HIGH_BIT_DEPTH
%if %1*%2 >= 128
    paddusw m0, [rsp+3*mmsize]
    paddusw m1, [rsp+4*mmsize]
%endif
%if %1*%2 == 256
    paddusw m0, [rsp+5*mmsize]
    paddusw m1, [rsp+6*mmsize]
    paddusw m0, [rsp+7*mmsize]
    paddusw m1, [rsp+8*mmsize]
    psrlw   m0, 1
%endif
    HADDUW  m0, m2
    HADDW   m1, m3
%endif ; HIGH_BIT_DEPTH
%endmacro

; struct { int satd, int sa8d; } pixel_hadamard_ac_16x16( uint8_t *pix, int stride )
%macro HADAMARD_AC_WXH_SSE2 2
cglobal pixel_hadamard_ac_%1x%2, 2,3,11
    %assign pad 16-gprsize-(stack_offset&15)
    %define ysub r1
    FIX_STRIDES r1
    sub  rsp, 48+pad
    lea  r2, [r1*3]
    call hadamard_ac_8x8
%if %2==16
    %define ysub r2
    lea  r0, [r0+r1*4]
    sub  rsp, 32
    call hadamard_ac_8x8
%endif
%if %1==16
    neg  ysub
    sub  rsp, 32
    lea  r0, [r0+ysub*4+8*SIZEOF_PIXEL]
    neg  ysub
    call hadamard_ac_8x8
%if %2==16
    lea  r0, [r0+r1*4]
    sub  rsp, 32
    call hadamard_ac_8x8
%endif
%endif
    HADAMARD_AC_WXH_SUM_SSE2 %1, %2
    movd edx, m0
    movd eax, m1
    shr  edx, 2 - (%1*%2 >> 8)
    shr  eax, 1
%ifdef ARCH_X86_64
    shl  rdx, 32
    add  rax, rdx
%endif
    add  rsp, 16+%1*%2/2+pad
    RET
%endmacro ; HADAMARD_AC_WXH_SSE2

; instantiate satds

%ifndef ARCH_X86_64
cextern pixel_sa8d_8x8_internal_mmx2
INIT_MMX mmx2
SA8D
%endif

%define TRANS TRANS_SSE2
%define DIFFOP DIFF_UNPACK_SSE2
%define LOAD_INC_8x4W LOAD_INC_8x4W_SSE2
%define LOAD_SUMSUB_8x4P LOAD_DIFF_8x4P
%define LOAD_SUMSUB_16P  LOAD_SUMSUB_16P_SSE2
%define movdqa movaps ; doesn't hurt pre-nehalem, might as well save size
%define movdqu movups
%define punpcklqdq movlhps
INIT_XMM sse2
SA8D
SATDS_SSE2
INTRA_SA8D_SSE2
%ifndef HIGH_BIT_DEPTH
INIT_MMX mmx2
INTRA_SATDS_MMX
%endif
INIT_XMM sse2
HADAMARD_AC_SSE2

%define DIFFOP DIFF_SUMSUB_SSSE3
%define LOAD_DUP_4x8P LOAD_DUP_4x8P_CONROE
%ifndef HIGH_BIT_DEPTH
%define LOAD_INC_8x4W LOAD_INC_8x4W_SSSE3
%define LOAD_SUMSUB_8x4P LOAD_SUMSUB_8x4P_SSSE3
%define LOAD_SUMSUB_16P  LOAD_SUMSUB_16P_SSSE3
%endif
INIT_XMM ssse3
SATDS_SSE2
SA8D
HADAMARD_AC_SSE2
%undef movdqa ; nehalem doesn't like movaps
%undef movdqu ; movups
%undef punpcklqdq ; or movlhps
INTRA_SA8D_SSE2
INIT_MMX ssse3
INTRA_SATDS_MMX

%define TRANS TRANS_SSE4
%define LOAD_DUP_4x8P LOAD_DUP_4x8P_PENRYN
INIT_XMM sse4
SATDS_SSE2
SA8D
HADAMARD_AC_SSE2

INIT_XMM avx
SATDS_SSE2
SA8D
INTRA_SA8D_SSE2
HADAMARD_AC_SSE2

;=============================================================================
; SSIM
;=============================================================================

;-----------------------------------------------------------------------------
; void pixel_ssim_4x4x2_core( const uint8_t *pix1, int stride1,
;                             const uint8_t *pix2, int stride2, int sums[2][4] )
;-----------------------------------------------------------------------------
%macro SSIM_ITER 1
%ifdef HIGH_BIT_DEPTH
    movdqu    m5, [r0+(%1&1)*r1]
    movdqu    m6, [r2+(%1&1)*r3]
%else
    movq      m5, [r0+(%1&1)*r1]
    movq      m6, [r2+(%1&1)*r3]
    punpcklbw m5, m0
    punpcklbw m6, m0
%endif
%if %1==1
    lea       r0, [r0+r1*2]
    lea       r2, [r2+r3*2]
%endif
%if %1==0
    movdqa    m1, m5
    movdqa    m2, m6
%else
    paddw     m1, m5
    paddw     m2, m6
%endif
    pmaddwd   m7, m5, m6
    pmaddwd   m5, m5
    pmaddwd   m6, m6
%if %1==0
    SWAP       3,  5
    SWAP       4,  7
%else
    paddd     m3, m5
    paddd     m4, m7
%endif
    paddd     m3, m6
%endmacro

%macro SSIM 0
cglobal pixel_ssim_4x4x2_core, 4,4,8
    FIX_STRIDES r1, r3
    pxor      m0, m0
    SSIM_ITER 0
    SSIM_ITER 1
    SSIM_ITER 2
    SSIM_ITER 3
    ; PHADDW m1, m2
    ; PHADDD m3, m4
    movdqa    m7, [pw_1]
    pshufd    m5, m3, q2301
    pmaddwd   m1, m7
    pmaddwd   m2, m7
    pshufd    m6, m4, q2301
    packssdw  m1, m2
    paddd     m3, m5
    pshufd    m1, m1, q3120
    paddd     m4, m6
    pmaddwd   m1, m7
    punpckhdq m5, m3, m4
    punpckldq m3, m4

%ifdef UNIX64
    %define t0 r4
%else
    %define t0 rax
    mov t0, r4mp
%endif

    movq      [t0+ 0], m1
    movq      [t0+ 8], m3
    movhps    [t0+16], m1
    movq      [t0+24], m5
    RET

;-----------------------------------------------------------------------------
; float pixel_ssim_end( int sum0[5][4], int sum1[5][4], int width )
;-----------------------------------------------------------------------------
cglobal pixel_ssim_end4, 3,3,7
    movdqa    m0, [r0+ 0]
    movdqa    m1, [r0+16]
    movdqa    m2, [r0+32]
    movdqa    m3, [r0+48]
    movdqa    m4, [r0+64]
    paddd     m0, [r1+ 0]
    paddd     m1, [r1+16]
    paddd     m2, [r1+32]
    paddd     m3, [r1+48]
    paddd     m4, [r1+64]
    paddd     m0, m1
    paddd     m1, m2
    paddd     m2, m3
    paddd     m3, m4
    movdqa    m5, [ssim_c1]
    movdqa    m6, [ssim_c2]
    TRANSPOSE4x4D  0, 1, 2, 3, 4

;   s1=m0, s2=m1, ss=m2, s12=m3
%if BIT_DEPTH == 10
    cvtdq2ps  m0, m0
    cvtdq2ps  m1, m1
    cvtdq2ps  m2, m2
    cvtdq2ps  m3, m3
    mulps     m2, [pf_64] ; ss*64
    mulps     m3, [pf_128] ; s12*128
    movdqa    m4, m1
    mulps     m4, m0      ; s1*s2
    mulps     m1, m1      ; s2*s2
    mulps     m0, m0      ; s1*s1
    addps     m4, m4      ; s1*s2*2
    addps     m0, m1      ; s1*s1 + s2*s2
    subps     m2, m0      ; vars
    subps     m3, m4      ; covar*2
    addps     m4, m5      ; s1*s2*2 + ssim_c1
    addps     m0, m5      ; s1*s1 + s2*s2 + ssim_c1
    addps     m2, m6      ; vars + ssim_c2
    addps     m3, m6      ; covar*2 + ssim_c2
%else
    pmaddwd   m4, m1, m0  ; s1*s2
    pslld     m1, 16
    por       m0, m1
    pmaddwd   m0, m0  ; s1*s1 + s2*s2
    pslld     m4, 1
    pslld     m3, 7
    pslld     m2, 6
    psubd     m3, m4  ; covar*2
    psubd     m2, m0  ; vars
    paddd     m0, m5
    paddd     m4, m5
    paddd     m3, m6
    paddd     m2, m6
    cvtdq2ps  m0, m0  ; (float)(s1*s1 + s2*s2 + ssim_c1)
    cvtdq2ps  m4, m4  ; (float)(s1*s2*2 + ssim_c1)
    cvtdq2ps  m3, m3  ; (float)(covar*2 + ssim_c2)
    cvtdq2ps  m2, m2  ; (float)(vars + ssim_c2)
%endif
    mulps     m4, m3
    mulps     m0, m2
    divps     m4, m0  ; ssim

    cmp       r2d, 4
    je .skip ; faster only if this is the common case; remove branch if we use ssim on a macroblock level
    neg       r2
%ifdef PIC
    lea       r3, [mask_ff + 16]
    movdqu    m1, [r3 + r2*4]
%else
    movdqu    m1, [mask_ff + r2*4 + 16]
%endif
    pand      m4, m1
.skip:
    movhlps   m0, m4
    addps     m0, m4
    pshuflw   m4, m0, q0032
    addss     m0, m4
%ifndef ARCH_X86_64
    movd     r0m, m0
    fld     dword r0m
%endif
    RET
%endmacro ; SSIM

INIT_XMM sse2
SSIM
INIT_XMM avx
SSIM

;=============================================================================
; Successive Elimination ADS
;=============================================================================

%macro ADS_START 0
%ifdef WIN64
    movsxd  r5,  r5d
%endif
    mov     r0d, r5d
    lea     r6,  [r4+r5+15]
    and     r6,  ~15;
    shl     r2d,  1
%endmacro

%macro ADS_END 1 ; unroll_size
    add     r1, 8*%1
    add     r3, 8*%1
    add     r6, 4*%1
    sub     r0d, 4*%1
    jg .loop
    WIN64_RESTORE_XMM rsp
    jmp ads_mvs
%endmacro

;-----------------------------------------------------------------------------
; int pixel_ads4( int enc_dc[4], uint16_t *sums, int delta,
;                 uint16_t *cost_mvx, int16_t *mvs, int width, int thresh )
;-----------------------------------------------------------------------------
INIT_MMX mmx2
cglobal pixel_ads4, 6,7
    movq    mm6, [r0]
    movq    mm4, [r0+8]
    pshufw  mm7, mm6, 0
    pshufw  mm6, mm6, q2222
    pshufw  mm5, mm4, 0
    pshufw  mm4, mm4, q2222
    ADS_START
.loop:
    movq    mm0, [r1]
    movq    mm1, [r1+16]
    psubw   mm0, mm7
    psubw   mm1, mm6
    ABSW    mm0, mm0, mm2
    ABSW    mm1, mm1, mm3
    movq    mm2, [r1+r2]
    movq    mm3, [r1+r2+16]
    psubw   mm2, mm5
    psubw   mm3, mm4
    paddw   mm0, mm1
    ABSW    mm2, mm2, mm1
    ABSW    mm3, mm3, mm1
    paddw   mm0, mm2
    paddw   mm0, mm3
    pshufw  mm1, r6m, 0
    paddusw mm0, [r3]
    psubusw mm1, mm0
    packsswb mm1, mm1
    movd    [r6], mm1
    ADS_END 1

cglobal pixel_ads2, 6,7
    movq    mm6, [r0]
    pshufw  mm5, r6m, 0
    pshufw  mm7, mm6, 0
    pshufw  mm6, mm6, q2222
    ADS_START
.loop:
    movq    mm0, [r1]
    movq    mm1, [r1+r2]
    psubw   mm0, mm7
    psubw   mm1, mm6
    ABSW    mm0, mm0, mm2
    ABSW    mm1, mm1, mm3
    paddw   mm0, mm1
    paddusw mm0, [r3]
    movq    mm4, mm5
    psubusw mm4, mm0
    packsswb mm4, mm4
    movd    [r6], mm4
    ADS_END 1

cglobal pixel_ads1, 6,7
    pshufw  mm7, [r0], 0
    pshufw  mm6, r6m, 0
    ADS_START
.loop:
    movq    mm0, [r1]
    movq    mm1, [r1+8]
    psubw   mm0, mm7
    psubw   mm1, mm7
    ABSW    mm0, mm0, mm2
    ABSW    mm1, mm1, mm3
    paddusw mm0, [r3]
    paddusw mm1, [r3+8]
    movq    mm4, mm6
    movq    mm5, mm6
    psubusw mm4, mm0
    psubusw mm5, mm1
    packsswb mm4, mm5
    movq    [r6], mm4
    ADS_END 2

%macro ADS_XMM 0
cglobal pixel_ads4, 6,7,12
    movdqa  xmm4, [r0]
    pshuflw xmm7, xmm4, 0
    pshuflw xmm6, xmm4, q2222
    pshufhw xmm5, xmm4, 0
    pshufhw xmm4, xmm4, q2222
    punpcklqdq xmm7, xmm7
    punpcklqdq xmm6, xmm6
    punpckhqdq xmm5, xmm5
    punpckhqdq xmm4, xmm4
%ifdef ARCH_X86_64
    pshuflw xmm8, r6m, 0
    punpcklqdq xmm8, xmm8
    ADS_START
    movdqu  xmm10, [r1]
    movdqu  xmm11, [r1+r2]
.loop:
    psubw   xmm0, xmm10, xmm7
    movdqu xmm10, [r1+16]
    psubw   xmm1, xmm10, xmm6
    ABSW    xmm0, xmm0, xmm2
    ABSW    xmm1, xmm1, xmm3
    psubw   xmm2, xmm11, xmm5
    movdqu xmm11, [r1+r2+16]
    paddw   xmm0, xmm1
    psubw   xmm3, xmm11, xmm4
    movdqu  xmm9, [r3]
    ABSW    xmm2, xmm2, xmm1
    ABSW    xmm3, xmm3, xmm1
    paddw   xmm0, xmm2
    paddw   xmm0, xmm3
    paddusw xmm0, xmm9
    psubusw xmm1, xmm8, xmm0
    packsswb xmm1, xmm1
    movq    [r6], xmm1
%else
    ADS_START
.loop:
    movdqu  xmm0, [r1]
    movdqu  xmm1, [r1+16]
    psubw   xmm0, xmm7
    psubw   xmm1, xmm6
    ABSW    xmm0, xmm0, xmm2
    ABSW    xmm1, xmm1, xmm3
    movdqu  xmm2, [r1+r2]
    movdqu  xmm3, [r1+r2+16]
    psubw   xmm2, xmm5
    psubw   xmm3, xmm4
    paddw   xmm0, xmm1
    ABSW    xmm2, xmm2, xmm1
    ABSW    xmm3, xmm3, xmm1
    paddw   xmm0, xmm2
    paddw   xmm0, xmm3
    movd    xmm1, r6m
    movdqu  xmm2, [r3]
    pshuflw xmm1, xmm1, 0
    punpcklqdq xmm1, xmm1
    paddusw xmm0, xmm2
    psubusw xmm1, xmm0
    packsswb xmm1, xmm1
    movq    [r6], xmm1
%endif ; ARCH
    ADS_END 2

cglobal pixel_ads2, 6,7,8
    movq    xmm6, [r0]
    movd    xmm5, r6m
    pshuflw xmm7, xmm6, 0
    pshuflw xmm6, xmm6, q2222
    pshuflw xmm5, xmm5, 0
    punpcklqdq xmm7, xmm7
    punpcklqdq xmm6, xmm6
    punpcklqdq xmm5, xmm5
    ADS_START
.loop:
    movdqu  xmm0, [r1]
    movdqu  xmm1, [r1+r2]
    psubw   xmm0, xmm7
    psubw   xmm1, xmm6
    movdqu  xmm4, [r3]
    ABSW    xmm0, xmm0, xmm2
    ABSW    xmm1, xmm1, xmm3
    paddw   xmm0, xmm1
    paddusw xmm0, xmm4
    psubusw xmm1, xmm5, xmm0
    packsswb xmm1, xmm1
    movq    [r6], xmm1
    ADS_END 2

cglobal pixel_ads1, 6,7,8
    movd    xmm7, [r0]
    movd    xmm6, r6m
    pshuflw xmm7, xmm7, 0
    pshuflw xmm6, xmm6, 0
    punpcklqdq xmm7, xmm7
    punpcklqdq xmm6, xmm6
    ADS_START
.loop:
    movdqu  xmm0, [r1]
    movdqu  xmm1, [r1+16]
    psubw   xmm0, xmm7
    psubw   xmm1, xmm7
    movdqu  xmm2, [r3]
    movdqu  xmm3, [r3+16]
    ABSW    xmm0, xmm0, xmm4
    ABSW    xmm1, xmm1, xmm5
    paddusw xmm0, xmm2
    paddusw xmm1, xmm3
    psubusw xmm4, xmm6, xmm0
    psubusw xmm5, xmm6, xmm1
    packsswb xmm4, xmm5
    movdqa  [r6], xmm4
    ADS_END 4
%endmacro

INIT_XMM sse2
ADS_XMM
INIT_XMM ssse3
ADS_XMM
INIT_XMM avx
ADS_XMM

; int pixel_ads_mvs( int16_t *mvs, uint8_t *masks, int width )
; {
;     int nmv=0, i, j;
;     *(uint32_t*)(masks+width) = 0;
;     for( i=0; i<width; i+=8 )
;     {
;         uint64_t mask = *(uint64_t*)(masks+i);
;         if( !mask ) continue;
;         for( j=0; j<8; j++ )
;             if( mask & (255<<j*8) )
;                 mvs[nmv++] = i+j;
;     }
;     return nmv;
; }

%macro TEST 1
    mov     [r4+r0*2], r1w
    test    r2d, 0xff<<(%1*8)
    setne   r3b
    add     r0d, r3d
    inc     r1d
%endmacro

INIT_MMX
cglobal pixel_ads_mvs, 0,7,0
ads_mvs:
    lea     r6,  [r4+r5+15]
    and     r6,  ~15;
    ; mvs = r4
    ; masks = r6
    ; width = r5
    ; clear last block in case width isn't divisible by 8. (assume divisible by 4, so clearing 4 bytes is enough.)
    xor     r0d, r0d
    xor     r1d, r1d
    mov     [r6+r5], r0d
    jmp .loopi
ALIGN 16
.loopi0:
    add     r1d, 8
    cmp     r1d, r5d
    jge .end
.loopi:
    mov     r2,  [r6+r1]
%ifdef ARCH_X86_64
    test    r2,  r2
%else
    mov     r3,  r2
    or      r3d, [r6+r1+4]
%endif
    jz .loopi0
    xor     r3d, r3d
    TEST 0
    TEST 1
    TEST 2
    TEST 3
%ifdef ARCH_X86_64
    shr     r2,  32
%else
    mov     r2d, [r6+r1]
%endif
    TEST 0
    TEST 1
    TEST 2
    TEST 3
    cmp     r1d, r5d
    jl .loopi
.end:
    movifnidn eax, r0d
    RET

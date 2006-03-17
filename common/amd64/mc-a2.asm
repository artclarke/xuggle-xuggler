;*****************************************************************************
;* mc-a2.asm: h264 encoder library
;*****************************************************************************
;* Copyright (C) 2005 x264 project
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

BITS 64

;=============================================================================
; Macros and other preprocessor constants
;=============================================================================

%include "amd64inc.asm"

;=============================================================================
; Read only data
;=============================================================================

SECTION .rodata

ALIGN 16
mmx_dw_one:
    times 4 dw 16
mmx_dd_one:
    times 2 dd 512
mmx_dw_20:
    times 4 dw 20
mmx_dw_5:
    times 4 dw -5

%assign tbuffer 0

;=============================================================================
; Macros
;=============================================================================

%macro LOAD_4 9
    movd %1, %5
    movd %2, %6
    movd %3, %7
    movd %4, %8
    punpcklbw %1, %9
    punpcklbw %2, %9
    punpcklbw %3, %9
    punpcklbw %4, %9
%endmacro

%macro FILT_2 2
    psubw %1, %2
    psllw %2, 2
    psubw %1, %2
%endmacro

%macro FILT_4 3
    paddw %2, %3
    psllw %2, 2
    paddw %1, %2
    psllw %2, 2
    paddw %1, %2
%endmacro

%macro FILT_6 4
    psubw %1, %2
    psllw %2, 2
    psubw %1, %2
    paddw %1, %3
    paddw %1, %4
    psraw %1, 5
%endmacro

%macro FILT_ALL 1
    LOAD_4      mm1, mm2, mm3, mm4, [%1], [%1 + rcx], [%1 + 2 * rcx], [%1 + rbx], mm0
    FILT_2      mm1, mm2
    movd        mm5, [%1 + 4 * rcx]
    movd        mm6, [%1 + rdx]
    FILT_4      mm1, mm3, mm4
    punpcklbw   mm5, mm0
    punpcklbw   mm6, mm0
    psubw       mm1, mm5
    psllw       mm5, 2
    psubw       mm1, mm5
    paddw       mm1, mm6
%endmacro




;=============================================================================
; Code
;=============================================================================

SECTION .text

cglobal x264_horizontal_filter_mmxext
cglobal x264_center_filter_mmxext

;-----------------------------------------------------------------------------
;
; void x264_center_filter_mmxext( uint8_t *dst1, int i_dst1_stride,
;                                 uint8_t *dst2, int i_dst2_stride,
;                                  uint8_t *src, int i_src_stride,
;                                  int i_width, int i_height );
;
;-----------------------------------------------------------------------------

ALIGN 16
x264_center_filter_mmxext :

    push        r15
    pushreg     r15
%ifdef WIN64
    push        rdi
    pushreg     rdi
    push        rsi
    pushreg     rsi
%endif

    push        rbp
    pushreg     rbp
    push        rbx
    pushreg     rbx
    push        r12
    pushreg     r12
    push        r13
    pushreg     r13
    push        r14
    pushreg     r14
    lea         rbp,    [rsp]
    setframe    rbp, 0
    endprolog

%ifdef WIN64
    movsxd      r13,    dword [rsp+64+48]   ; src_stride
    mov         r12,    [rsp+64+40]         ; src
%else
    movsxd      r13,    r9d                 ; src_stride
    mov         r12,    r8                  ; src
%endif
    sub         r12,    r13
    sub         r12,    r13                 ; tsrc = src - 2 * src_stride

    ; use 24 instead of 18 (used in i386/mc-a2.asm) to keep rsp aligned
    lea         rax,    [r13 + r13 + 24 + tbuffer]
    sub         rsp,    rax

    mov         r10,    parm3q                 ; dst2
    movsxd      r11,    parm4d                 ; dst2_stride
    mov         r8,     parm1q                 ; dst1
    movsxd      r9,     parm2d                 ; dst1_stride
%ifdef WIN64
    movsxd      r14,    dword [rbp + 64 + 56]  ; width
    movsxd      r15,    dword [rbp + 64 + 64]  ; height
%else
    movsxd      r14,    dword [rbp + 56]    ; width
    movsxd      r15,    dword [rbp + 64]    ; height
%endif

    mov         rcx,    r13                 ; src_stride
    lea         rbx,    [r13 + r13 * 2]     ; 3 * src_stride
    lea         rdx,    [r13 + r13 * 4]     ; 5 * src_stride

    pxor        mm0,    mm0                 ; 0 ---> mm0
    movq        mm7,    [mmx_dd_one GLOBAL] ; for rounding

.loopcy:

    xor         rax,    rax
    mov         rsi,    r12             ; tsrc

    FILT_ALL    rsi

    pshufw      mm2,    mm1, 0
    movq        [rsp + tbuffer],  mm2
    movq        [rsp + tbuffer + 8],  mm1
    paddw       mm1,    [mmx_dw_one GLOBAL]
    psraw       mm1,    5

    packuswb    mm1,    mm1
    movd        [r8],   mm1             ; dst1[0] = mm1

    add         rax,    8
    add         rsi,    4
    lea         rdi,    [r8 - 4]        ; rdi = dst1 - 4

.loopcx1:

    FILT_ALL    rsi

    movq        [rsp + tbuffer + 2 * rax],  mm1
    paddw       mm1,    [mmx_dw_one GLOBAL]
    psraw       mm1,    5
    packuswb    mm1,    mm1
    movd        [rdi + rax],  mm1   ; dst1[rax - 4] = mm1

    add         rsi,    4
    add         rax,    4
    cmp         rax,    r14         ; cmp rax, width
    jnz         .loopcx1

    FILT_ALL    rsi

    pshufw      mm2,    mm1,  7
    movq        [rsp + tbuffer + 2 * rax],  mm1
    movq        [rsp + tbuffer + 2 * rax + 8],  mm2
    paddw       mm1,    [mmx_dw_one GLOBAL]
    psraw       mm1,    5
    packuswb    mm1,    mm1
    movd        [rdi + rax],  mm1   ; dst1[rax - 4] = mm1

    add         r12,    r13         ; tsrc = tsrc + src_stride

    add         r8,     r9          ; dst1 = dst1 + dst1_stride

    xor         rax,    rax

.loopcx2:

    movq        mm2,    [rsp + 2 * rax + 2  + 4 + tbuffer]
    movq        mm3,    [rsp + 2 * rax + 4  + 4 + tbuffer]
    movq        mm4,    [rsp + 2 * rax + 6  + 4 + tbuffer]
    movq        mm5,    [rsp + 2 * rax + 8  + 4 + tbuffer]
    movq        mm1,    [rsp + 2 * rax      + 4 + tbuffer]
    movq        mm6,    [rsp + 2 * rax + 10 + 4 + tbuffer]
    paddw       mm2,    mm5
    paddw       mm3,    mm4
    paddw       mm1,    mm6

    movq        mm5,    [mmx_dw_20 GLOBAL]
    movq        mm4,    [mmx_dw_5 GLOBAL]
    movq        mm6,    mm1
    pxor        mm7,    mm7

    punpckhwd   mm5,    mm2
    punpcklwd   mm4,    mm3
    punpcklwd   mm2,    [mmx_dw_20 GLOBAL]
    punpckhwd   mm3,    [mmx_dw_5 GLOBAL]

    pcmpgtw     mm7,    mm1

    pmaddwd     mm2,    mm4
    pmaddwd     mm3,    mm5

    punpcklwd   mm1,    mm7
    punpckhwd   mm6,    mm7

    paddd       mm2,    mm1
    paddd       mm3,    mm6

    paddd       mm2,    [mmx_dd_one GLOBAL]
    paddd       mm3,    [mmx_dd_one GLOBAL]

    psrad       mm2,    10
    psrad       mm3,    10

    packssdw    mm2,    mm3
    packuswb    mm2,    mm0

    movd        [r10 + rax], mm2    ; dst2[rax] = mm2

    add         rax,    4
    cmp         rax,    r14         ; cmp rax, width
    jnz         .loopcx2

    add         r10,    r11         ; dst2 += dst2_stride
    dec         r15                 ; height
    jnz         .loopcy

    lea         rsp,    [rbp]

    pop         r14
    pop         r13
    pop         r12
    pop         rbx
    pop         rbp
%ifdef WIN64
    pop         rsi
    pop         rdi
%endif
    pop         r15

    ret

;-----------------------------------------------------------------------------
;
; void x264_horizontal_filter_mmxext( uint8_t *dst, int i_dst_stride,
;                                     uint8_t *src, int i_src_stride,
;                                     int i_width, int i_height );
;
;-----------------------------------------------------------------------------

ALIGN 16
x264_horizontal_filter_mmxext :
    movsxd      r10,    parm2d               ; dst_stride
    movsxd      r11,    parm4d               ; src_stride
%ifdef WIN64
    mov         rdx,    r8                   ; src
    mov         r9,     rcx                  ; dst
    movsxd      rcx,    parm6d               ; height
%else
    movsxd      rcx,    parm6d               ; height
    mov         r9,     rdi                  ; dst
%endif
    
    movsxd      r8,     parm5d               ; width

    pxor        mm0,    mm0
    movq        mm7,    [mmx_dw_one GLOBAL]

    sub         rdx,    2

loophy:

    xor         rax,    rax

loophx:

    prefetchnta [rdx + rax + 48]       

    LOAD_4      mm1,    mm2, mm3, mm4, [rdx + rax], [rdx + rax + 1], [rdx + rax + 2], [rdx + rax + 3], mm0
    FILT_2      mm1,    mm2
    movd        mm5,    [rdx + rax + 4]
    movd        mm6,    [rdx + rax + 5]
    FILT_4      mm1,    mm3, mm4
    movd        mm2,    [rdx + rax + 4]
    movd        mm3,    [rdx + rax + 6]
    punpcklbw   mm5,    mm0
    punpcklbw   mm6,    mm0
    FILT_6      mm1,    mm5, mm6, mm7
    movd        mm4,    [rdx + rax + 7]
    movd        mm5,    [rdx + rax + 8]
    punpcklbw   mm2,    mm0
    punpcklbw   mm3,    mm0                  ; mm2(1), mm3(20), mm6(-5) ready
    FILT_2      mm2,    mm6
    movd        mm6,    [rdx + rax + 9]
    punpcklbw   mm4,    mm0
    punpcklbw   mm5,    mm0                  ; mm2(1-5), mm3(20), mm4(20), mm5(-5) ready
    FILT_4      mm2,    mm3, mm4
    punpcklbw   mm6,    mm0
    FILT_6      mm2,    mm5, mm6, mm7

    packuswb    mm1,    mm2
    movq        [r9 + rax],  mm1

    add         rax,    8
    cmp         rax,    r8                   ; cmp rax, width
    jnz         loophx

    add         rdx,    r11                  ; src_pitch
    add         r9,     r10                  ; dst_pitch

    dec         rcx
    jnz         loophy

    ret

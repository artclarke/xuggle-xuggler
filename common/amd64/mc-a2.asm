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
mmx_dw_16:
    times 4 dw 16
mmx_dw_32:
    times 4 dw 32

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
cglobal x264_plane_copy_mmxext

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

.loopcy:

    mov         rax,    4
    mov         rsi,    r12             ; tsrc
    lea         rdi,    [r8 - 4]        ; rdi = dst1 - 4
    movq        mm7,    [mmx_dw_16 GLOBAL]  ; for rounding

.vertical_filter:

    prefetchnta [rsi + rdx + 32]
    FILT_ALL    rsi
    movq        mm7,    mm1
    FILT_ALL    rsi + 4

    movq        mm6,    [mmx_dw_16 GLOBAL]
    movq        [rsp + tbuffer + 2 * rax],  mm7
    movq        [rsp + tbuffer + 2 * rax + 8],  mm1
    paddw       mm7,    mm6
    paddw       mm1,    mm6
    psraw       mm7,    5
    psraw       mm1,    5
    packuswb    mm7,    mm1
    movntq      [rdi + rax],  mm7   ; dst1[rax - 4]

    cmp         rax,    r14         ; cmp rax, width
    lea         rsi,    [rsi + 8]
    lea         rax,    [rax + 8]
    jl          .vertical_filter

    pshufw      mm2, [rsp + tbuffer + 8], 0
    movq        [rsp + tbuffer], mm2 ; pad left
    ; no need to pad right, since loopcx1 already did 4 extra pixels

    add         r12,    r13         ; tsrc = tsrc + src_stride
    add         r8,     r9          ; dst1 = dst1 + dst1_stride
    xor         rax,    rax
    movq        mm7,    [mmx_dw_32 GLOBAL]  ; for rounding

.center_filter:

    movq        mm1,    [rsp + 2 * rax      + 4 + tbuffer]
    movq        mm2,    [rsp + 2 * rax + 2  + 4 + tbuffer]
    movq        mm3,    [rsp + 2 * rax + 4  + 4 + tbuffer]
    movq        mm4,    [rsp + 2 * rax + 8  + 4 + tbuffer]
    movq        mm5,    [rsp + 2 * rax + 10 + 4 + tbuffer]
    paddw       mm3,    [rsp + 2 * rax + 6  + 4 + tbuffer]
    paddw       mm2,    mm4
    paddw       mm1,    mm5
    movq        mm6,    [rsp + 2 * rax + 12 + 4 + tbuffer]
    paddw       mm4,    [rsp + 2 * rax + 18 + 4 + tbuffer]
    paddw       mm5,    [rsp + 2 * rax + 16 + 4 + tbuffer]
    paddw       mm6,    [rsp + 2 * rax + 14 + 4 + tbuffer]

    psubw       mm1,    mm2         ; a-b
    psubw       mm4,    mm5
    psraw       mm1,    2           ; (a-b)/4
    psraw       mm4,    2
    psubw       mm1,    mm2         ; (a-b)/4-b
    psubw       mm4,    mm5
    paddw       mm1,    mm3         ; (a-b)/4-b+c
    paddw       mm4,    mm6
    psraw       mm1,    2           ; ((a-b)/4-b+c)/4
    psraw       mm4,    2
    paddw       mm1,    mm3         ; ((a-b)/4-b+c)/4+c = (a-5*b+20*c)/16
    paddw       mm4,    mm6
    paddw       mm1,    mm7         ; +32
    paddw       mm4,    mm7
    psraw       mm1,    6
    psraw       mm4,    6

    packuswb    mm1,    mm4
    movntq      [r10 + rax], mm1    ; dst2[rax]

    add         rax,    8
    cmp         rax,    r14         ; cmp rax, width
    jnz         .center_filter

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
    movq        mm7,    [mmx_dw_16 GLOBAL]

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


;-----------------------------------------------------------------------------
; void x264_plane_copy_mmxext( uint8_t *dst, int i_dst,
;                              uint8_t *src, int i_src, int w, int h)
;-----------------------------------------------------------------------------
ALIGN 16
x264_plane_copy_mmxext:
    movsxd parm2q, parm2d
    movsxd parm4q, parm4d
    add    parm5d, 3
    and    parm5d, ~3
    sub    parm2q, parm5q
    sub    parm4q, parm5q
    ; shuffle regs because movsd needs dst=rdi, src=rsi, w=ecx
    xchg   rsi, rdx
    mov    rax, parm4q
.loopy:
    mov    ecx, parm5d
    sub    ecx, 64
    jl     .endx
.loopx:
    prefetchnta [rsi+256]
    movq   mm0, [rsi   ]
    movq   mm1, [rsi+ 8]
    movq   mm2, [rsi+16]
    movq   mm3, [rsi+24]
    movq   mm4, [rsi+32]
    movq   mm5, [rsi+40]
    movq   mm6, [rsi+48]
    movq   mm7, [rsi+56]
    movntq [rdi   ], mm0
    movntq [rdi+ 8], mm1
    movntq [rdi+16], mm2
    movntq [rdi+24], mm3
    movntq [rdi+32], mm4
    movntq [rdi+40], mm5
    movntq [rdi+48], mm6
    movntq [rdi+56], mm7
    add    rsi, 64
    add    rdi, 64
    sub    ecx, 64
    jge    .loopx
.endx:
    prefetchnta [rsi+256]
    add    ecx, 64
    shr    ecx, 2
    rep movsd
    add    rdi, rdx
    add    rsi, rax
    sub    parm6d, 1
    jge    .loopy
    rep ret


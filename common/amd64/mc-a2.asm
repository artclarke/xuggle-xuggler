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

SECTION_RODATA

pw_1:  times 4 dw 1
pw_16: times 4 dw 16
pw_32: times 4 dw 32

;=============================================================================
; Macros
;=============================================================================

%macro LOAD_ADD 3
    movd        %1,     %2
    movd        mm7,    %3
    punpcklbw   %1,     mm0
    punpcklbw   mm7,    mm0
    paddw       %1,     mm7
%endmacro

%macro FILT_V 0
    psubw       mm1,    mm2         ; a-b
    psubw       mm4,    mm5
    psubw       mm2,    mm3         ; b-c
    psubw       mm5,    mm6
    psllw       mm2,    2
    psllw       mm5,    2
    psubw       mm1,    mm2         ; a-5*b+4*c
    psubw       mm4,    mm5
    psllw       mm3,    4
    psllw       mm6,    4
    paddw       mm1,    mm3         ; a-5*b+20*c
    paddw       mm4,    mm6
%endmacro

%macro FILT_H 0
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
%endmacro

%macro FILT_PACK 1
    paddw       mm1,    mm7
    paddw       mm4,    mm7
    psraw       mm1,    %1
    psraw       mm4,    %1
    packuswb    mm1,    mm4
%endmacro


;=============================================================================
; Code
;=============================================================================

SECTION .text

;-----------------------------------------------------------------------------
; void x264_hpel_filter_mmxext( uint8_t *dsth, uint8_t *dstv, uint8_t *dstc, uint8_t *src,
;                               int i_stride, int i_width, int i_height );
;-----------------------------------------------------------------------------
cglobal x264_hpel_filter_mmxext

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
    mov         rbp,    rsp
    setframe    rbp, 0
    endprolog

%ifdef WIN64
    mov         rdi,    parm1q
    mov         rsi,    parm2q
    mov         rdx,    parm3q
    mov         rcx,    parm4q
    movsxd      r8,     dword [rbp+72]
    movsxd      r9,     dword [rbp+80]
    mov         ebx,    dword [rbp+88]
%else
    mov         ebx,    dword [rbp+24]
%endif
    %define     dsth    rdi
    %define     dstv    rsi
    %define     dstc    rdx
    %define     src     rcx
    %define     stride  r8
    %define     width   r9
    %define     height  ebx
    %define     stride3 r10
    %define     stride5 r11
    %define     x       rax
    %define     tbuffer rsp + 8

    lea         stride3, [stride*3]
    lea         stride5, [stride*5]
    sub         src,    stride
    sub         src,    stride

    lea         rax,    [stride*2 + 24]
    sub         rsp,    rax

    pxor        mm0,    mm0

.loopy:

    xor         x,      x
ALIGN 16
.vertical_filter:

    prefetcht0  [src + stride5 + 32]

    LOAD_ADD    mm1,    [src               ], [src + stride5     ] ; a0
    LOAD_ADD    mm2,    [src + stride      ], [src + stride*4    ] ; b0
    LOAD_ADD    mm3,    [src + stride*2    ], [src + stride3     ] ; c0
    LOAD_ADD    mm4,    [src            + 4], [src + stride5  + 4] ; a1
    LOAD_ADD    mm5,    [src + stride   + 4], [src + stride*4 + 4] ; b1
    LOAD_ADD    mm6,    [src + stride*2 + 4], [src + stride3  + 4] ; c1

    FILT_V

    movq        mm7,    [pw_16 GLOBAL]
    movq        [tbuffer + x*2],  mm1
    movq        [tbuffer + x*2 + 8],  mm4
    paddw       mm1,    mm7
    paddw       mm4,    mm7
    psraw       mm1,    5
    psraw       mm4,    5
    packuswb    mm1,    mm4
    movntq      [dstv + x], mm1

    add         x,      8
    add         src,    8
    cmp         x,      width
    jle         .vertical_filter

    pshufw      mm2, [tbuffer], 0
    movq        [tbuffer - 8], mm2 ; pad left
    ; no need to pad right, since vertical_filter already did 4 extra pixels

    sub         src,    x
    xor         x,      x
    movq        mm7,    [pw_32 GLOBAL]
.center_filter:

    movq        mm1,    [tbuffer + x*2 - 4 ]
    movq        mm2,    [tbuffer + x*2 - 2 ]
    movq        mm3,    [tbuffer + x*2     ]
    movq        mm4,    [tbuffer + x*2 + 4 ]
    movq        mm5,    [tbuffer + x*2 + 6 ]
    paddw       mm3,    [tbuffer + x*2 + 2 ] ; c0
    paddw       mm2,    mm4                  ; b0
    paddw       mm1,    mm5                  ; a0
    movq        mm6,    [tbuffer + x*2 + 8 ]
    paddw       mm4,    [tbuffer + x*2 + 14] ; a1
    paddw       mm5,    [tbuffer + x*2 + 12] ; b1
    paddw       mm6,    [tbuffer + x*2 + 10] ; c1

    FILT_H
    FILT_PACK 6
    movntq      [dstc + x], mm1

    add         x,      8
    cmp         x,      width
    jl          .center_filter

    lea         src,    [src + stride*2]
    xor         x,      x
.horizontal_filter:

    movd        mm1,    [src + x - 2]
    movd        mm2,    [src + x - 1]
    movd        mm3,    [src + x    ]
    movd        mm6,    [src + x + 1]
    movd        mm4,    [src + x + 2]
    movd        mm5,    [src + x + 3]
    punpcklbw   mm1,    mm0
    punpcklbw   mm2,    mm0
    punpcklbw   mm3,    mm0
    punpcklbw   mm6,    mm0
    punpcklbw   mm4,    mm0
    punpcklbw   mm5,    mm0
    paddw       mm3,    mm6 ; c0
    paddw       mm2,    mm4 ; b0
    paddw       mm1,    mm5 ; a0
    movd        mm7,    [src + x + 7]
    movd        mm6,    [src + x + 6]
    punpcklbw   mm7,    mm0
    punpcklbw   mm6,    mm0
    paddw       mm4,    mm7 ; c1
    paddw       mm5,    mm6 ; b1
    movd        mm7,    [src + x + 5]
    movd        mm6,    [src + x + 4]
    punpcklbw   mm7,    mm0
    punpcklbw   mm6,    mm0
    paddw       mm6,    mm7 ; a1

    movq        mm7,    [pw_1 GLOBAL]
    FILT_H
    FILT_PACK 1
    movntq      [dsth + x], mm1

    add         x,      8
    cmp         x,      width
    jl          .horizontal_filter

    sub         src,    stride
    add         dsth,   stride
    add         dstv,   stride
    add         dstc,   stride
    dec         height
    jg          .loopy

    mov         rsp,    rbp
    pop         rbx
    pop         rbp
%ifdef WIN64
    pop         rsi
    pop         rdi
%endif
    ret



;-----------------------------------------------------------------------------
; void x264_plane_copy_mmxext( uint8_t *dst, int i_dst,
;                              uint8_t *src, int i_src, int w, int h)
;-----------------------------------------------------------------------------
cglobal x264_plane_copy_mmxext
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
    jg     .loopy
    emms
    ret


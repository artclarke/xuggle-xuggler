;*****************************************************************************
;* mc-a2.asm: h264 encoder library
;*****************************************************************************
;* Copyright (C) 2005-2008 x264 project
;*
;* Authors: Loren Merritt <lorenm@u.washington.edu>
;*          Mathieu Monnier <manao@melix.net>
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

%include "x86inc.asm"

SECTION_RODATA

pw_1:  times 4 dw 1
pw_16: times 4 dw 16
pw_32: times 4 dw 32

SECTION .text

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

;-----------------------------------------------------------------------------
; void x264_hpel_filter_mmxext( uint8_t *dsth, uint8_t *dstv, uint8_t *dstc, uint8_t *src,
;                               int i_stride, int i_width, int i_height );
;-----------------------------------------------------------------------------
cglobal x264_hpel_filter_mmxext, 0,7
    %define     x       r0
    %define     xd      r0d
    %define     dsth    r1
    %define     dstv    r1
    %define     dstc    r1
    %define     src     r2
    %define     src3    r3
    %define     stride  r4
    %define     width   r5d
    %define     tbuffer rsp+8

%ifdef ARCH_X86_64
    PUSH        rbp
    PUSH        r12
    PUSH        r13
    PUSH        r14
    %define     tdsth   r10 ; FIXME r8,9
    %define     tdstv   r11
    %define     tdstc   r12
    %define     tsrc    r13
    %define     theight r14d
    mov         tdsth,  r0
    mov         tdstv,  r1
    mov         tdstc,  r2
    mov         tsrc,   r3
    mov         theight, r6m
%else
    %define     tdsth   [rbp + 20]
    %define     tdstv   [rbp + 24]
    %define     tdstc   [rbp + 28]
    %define     tsrc    [rbp + 32]
    %define     theight [rbp + 44]
%endif

    movifnidn   r4d,    r4m
    movifnidn   r5d,    r5m
    mov         rbp,    rsp
    lea         rax,    [stride*2 + 24]
    sub         rsp,    rax
    pxor        mm0,    mm0

    %define     tpw_1   [pw_1 GLOBAL]
    %define     tpw_16  [pw_16 GLOBAL]
    %define     tpw_32  [pw_32 GLOBAL]
%ifdef PIC32
    ; mov globals onto the stack, to free up PIC pointer
    %define     tpw_1   [ebp - 24]
    %define     tpw_16  [ebp - 16]
    %define     tpw_32  [ebp -  8]
    picgetgot   ebx
    sub         esp,    24
    movq        mm1,    [pw_1  GLOBAL]
    movq        mm2,    [pw_16 GLOBAL]
    movq        mm3,    [pw_32 GLOBAL]
    movq        tpw_1,  mm1
    movq        tpw_16, mm2
    movq        tpw_32, mm3
%endif

.loopy:

    mov         src,    tsrc
    mov         dstv,   tdstv
    lea         src3,   [src + stride]
    sub         src,    stride
    sub         src,    stride
    xor         xd,     xd
ALIGN 16
.vertical_filter:

    prefetcht0  [src3 + stride*2 + 32]

    LOAD_ADD    mm1,    [src               ], [src3 + stride*2    ] ; a0
    LOAD_ADD    mm2,    [src + stride      ], [src3 + stride      ] ; b0
    LOAD_ADD    mm3,    [src + stride*2    ], [src3               ] ; c0
    LOAD_ADD    mm4,    [src            + 4], [src3 + stride*2 + 4] ; a1
    LOAD_ADD    mm5,    [src + stride   + 4], [src3 + stride   + 4] ; b1
    LOAD_ADD    mm6,    [src + stride*2 + 4], [src3            + 4] ; c1

    FILT_V

    movq        mm7,    tpw_16
    movq        [tbuffer + x*2],  mm1
    movq        [tbuffer + x*2 + 8],  mm4
    paddw       mm1,    mm7
    paddw       mm4,    mm7
    psraw       mm1,    5
    psraw       mm4,    5
    packuswb    mm1,    mm4
    movntq      [dstv + x], mm1

    add         xd,     8
    add         src,    8
    add         src3,   8
    cmp         xd,     width
    jle         .vertical_filter

    pshufw      mm2, [tbuffer], 0
    movq        [tbuffer - 8], mm2 ; pad left
    ; no need to pad right, since vertical_filter already did 4 extra pixels

    mov         dstc,   tdstc
    xor         xd,     xd
    movq        mm7,    tpw_32
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

    add         xd,     8
    cmp         xd,     width
    jl          .center_filter

    mov         dsth,   tdsth
    mov         src,    tsrc
    xor         xd,     xd
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

    movq        mm7,    tpw_1
    FILT_H
    FILT_PACK 1
    movntq      [dsth + x], mm1

    add         xd,     8
    cmp         xd,     width
    jl          .horizontal_filter

    add         tsrc,  stride
    add         tdsth, stride
    add         tdstv, stride
    add         tdstc, stride
    dec         dword theight
    jg          .loopy

    mov         rsp,    rbp
%ifdef ARCH_X86_64
    pop         r14
    pop         r13
    pop         r12
    pop         rbp
%endif
    RET



;-----------------------------------------------------------------------------
; void x264_plane_copy_mmxext( uint8_t *dst, int i_dst,
;                              uint8_t *src, int i_src, int w, int h)
;-----------------------------------------------------------------------------
cglobal x264_plane_copy_mmxext, 6,7
    movsxdifnidn r1, r1d
    movsxdifnidn r3, r3d
    add    r4d, 3
    and    r4d, ~3
    mov    r6d, r4d
    and    r6d, ~15
    sub    r1,  r6
    sub    r3,  r6
.loopy:
    mov    r6d, r4d
    sub    r6d, 64
    jl     .endx
.loopx:
    prefetchnta [r2+256]
    movq   mm0, [r2   ]
    movq   mm1, [r2+ 8]
    movq   mm2, [r2+16]
    movq   mm3, [r2+24]
    movq   mm4, [r2+32]
    movq   mm5, [r2+40]
    movq   mm6, [r2+48]
    movq   mm7, [r2+56]
    movntq [r0   ], mm0
    movntq [r0+ 8], mm1
    movntq [r0+16], mm2
    movntq [r0+24], mm3
    movntq [r0+32], mm4
    movntq [r0+40], mm5
    movntq [r0+48], mm6
    movntq [r0+56], mm7
    add    r2,  64
    add    r0,  64
    sub    r6d, 64
    jge    .loopx
.endx:
    prefetchnta [r2+256]
    add    r6d, 48
    jl .end16
.loop16:
    movq   mm0, [r2  ]
    movq   mm1, [r2+8]
    movntq [r0  ], mm0
    movntq [r0+8], mm1
    add    r2,  16
    add    r0,  16
    sub    r6d, 16
    jge    .loop16
.end16:
    add    r6d, 12
    jl .end4
.loop4:
    movd   mm2, [r2+r6]
    movd   [r0+r6], mm2
    sub    r6d, 4
    jge .loop4
.end4:
    add    r2, r3
    add    r0, r1
    dec    r5d
    jg     .loopy
    emms
    RET


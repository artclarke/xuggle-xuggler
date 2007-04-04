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

BITS 32

;=============================================================================
; Macros and other preprocessor constants
;=============================================================================

%include "i386inc.asm"

;=============================================================================
; Read only data
;=============================================================================

SECTION_RODATA

ALIGN 16
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
    push        ebp
    mov         ebp,    esp
    push        ebx
    push        esi
    push        edi
    picgetgot   ebx

    %define     tdsth   ebp +  8
    %define     tdstv   ebp + 12
    %define     tdstc   ebp + 16
    %define     tsrc    ebp + 20
    %define     tstride ebp + 24
    %define     twidth  ebp + 28
    %define     theight ebp + 32
    %define     tpw_1   ebp - 36
    %define     tpw_16  ebp - 28
    %define     tpw_32  ebp - 20
    %define     tbuffer esp +  8

    %define     x       eax
    %define     dsth    ebx
    %define     dstv    ebx
    %define     dstc    ebx
    %define     src     ecx
    %define     src3    edx
    %define     stride  esi
    %define     width   edi

    mov         stride, [tstride]
    mov         width,  [twidth]
    lea         eax,    [stride*2 + 24 + 24]
    sub         esp,    eax
    pxor        mm0,    mm0

    ; mov globals onto the stack, to free up ebx
    movq        mm1,    [pw_1  GOT_ebx]
    movq        mm2,    [pw_16 GOT_ebx]
    movq        mm3,    [pw_32 GOT_ebx]
    movq        [tpw_1],  mm1
    movq        [tpw_16], mm2
    movq        [tpw_32], mm3

.loopy:

    mov         src,    [tsrc]
    mov         dstv,   [tdstv]
    lea         src3,   [src + stride]
    sub         src,    stride
    sub         src,    stride
    xor         x,      x
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

    movq        mm7,    [tpw_16]
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
    add         src3,   8
    cmp         x,      width
    jle         .vertical_filter

    pshufw      mm2, [tbuffer], 0
    movq        [tbuffer - 8], mm2 ; pad left
    ; no need to pad right, since vertical_filter already did 4 extra pixels

    mov         dstc,   [tdstc]
    xor         x,      x
    movq        mm7,    [tpw_32]
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

    mov         dsth,   [tdsth]
    mov         src,    [tsrc]
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

    movq        mm7,    [tpw_1]
    FILT_H
    FILT_PACK 1
    movntq      [dsth + x], mm1

    add         x,      8
    cmp         x,      width
    jl          .horizontal_filter

    add         [tsrc],  stride
    add         [tdsth], stride
    add         [tdstv], stride
    add         [tdstc], stride
    dec         dword [theight]
    jg          .loopy

    lea         esp,    [ebp-12]
    pop         edi
    pop         esi
    pop         ebx
    pop         ebp
    ret




;-----------------------------------------------------------------------------
; void x264_plane_copy_mmxext( uint8_t *dst, int i_dst,
;                              uint8_t *src, int i_src, int w, int h)
;-----------------------------------------------------------------------------
cglobal x264_plane_copy_mmxext
    push   edi
    push   esi
    push   ebx
    mov    edi, [esp+16] ; dst
    mov    ebx, [esp+20] ; i_dst
    mov    esi, [esp+24] ; src
    mov    eax, [esp+28] ; i_src
    mov    edx, [esp+32] ; w
    add    edx, 3
    and    edx, ~3
    sub    ebx, edx
    sub    eax, edx
.loopy:
    mov    ecx, edx
    sub    ecx, 64
    jl     .endx
.loopx:
    prefetchnta [esi+256]
    movq   mm0, [esi   ]
    movq   mm1, [esi+ 8]
    movq   mm2, [esi+16]
    movq   mm3, [esi+24]
    movq   mm4, [esi+32]
    movq   mm5, [esi+40]
    movq   mm6, [esi+48]
    movq   mm7, [esi+56]
    movntq [edi   ], mm0
    movntq [edi+ 8], mm1
    movntq [edi+16], mm2
    movntq [edi+24], mm3
    movntq [edi+32], mm4
    movntq [edi+40], mm5
    movntq [edi+48], mm6
    movntq [edi+56], mm7
    add    esi, 64
    add    edi, 64
    sub    ecx, 64
    jge    .loopx
.endx:
    prefetchnta [esi+256]
    add    ecx, 64
    shr    ecx, 2
    rep movsd
    add    edi, ebx
    add    esi, eax
    sub    dword [esp+36], 1
    jg     .loopy
    pop    ebx
    pop    esi
    pop    edi
    emms
    ret


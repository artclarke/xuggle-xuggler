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
mmx_dw_one:
    times 4 dw 16
mmx_dd_one:
    times 2 dd 512
mmx_dw_20:
    times 4 dw 20
mmx_dw_5:
    times 4 dw -5

%assign twidth  0
%assign theight 4
%assign tdstp1  8
%assign tdstp2  12
%assign tdst1   16
%assign tdst2   20
%assign tsrc    24
%assign tsrcp   28
%assign toffset 32
%assign tbuffer 36


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
    LOAD_4      mm1, mm2, mm3, mm4, [%1], [%1 + ecx], [%1 + 2 * ecx], [%1 + ebx], mm0
    FILT_2      mm1, mm2
    movd        mm5, [%1 + 4 * ecx]
    movd        mm6, [%1 + edx]
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

    push        edi
    push        esi
    push        ebx
    push        ebp

    mov         edx,      [esp + 40]         ; src_stride
    lea         edx,      [edx + edx + 18 + tbuffer]
    sub         esp,      edx
    mov         [esp + toffset] ,edx
    
    mov         eax,      [esp + edx + 20]   ; dst1
    mov         [esp + tdst1]   ,eax
    
    mov         eax,      [esp + edx + 28]   ; dst2
    mov         [esp + tdst2]   ,eax
    
    mov         eax,      [esp + edx + 44]   ; width
    mov         [esp + twidth]  ,eax
    
    mov         eax,      [esp + edx + 48]   ; height
    mov         [esp + theight] ,eax
    
    mov         eax,      [esp + edx + 24]   ; dst1_stride
    mov         [esp + tdstp1]  ,eax
    
    mov         eax,      [esp + edx + 32]   ; dst2_stride
    mov         [esp + tdstp2]  ,eax

    mov         ecx,      [esp + edx + 40]   ; src_stride
    mov         [esp + tsrcp]   ,ecx
    
    mov         eax,      [esp + edx + 36]   ; src
    sub         eax,      ecx
    sub         eax,      ecx
    mov         [esp + tsrc]    ,eax         ; src - 2 * src_stride

    lea         ebx,      [ecx + ecx * 2]    ; 3 * src_stride
    lea         edx,      [ecx + ecx * 4]    ; 5 * src_stride

    picpush     ebx
    picgetgot   ebx

    pxor        mm0,      mm0                ; 0 ---> mm0


loopcy:

    mov         edi,    [picesp + tdst1]
    lea         ebp,    [picesp + tbuffer]
    mov         esi,    [picesp + tsrc]
    movq        mm7,    [mmx_dw_one GOT_ebx]

    picpop      ebx

    FILT_ALL    esi

    pshufw      mm2,    mm1, 0
    movq        [ebp + 8],  mm1
    movq        [ebp],  mm2
    paddw       mm1,    mm7
    psraw       mm1,    5

    packuswb    mm1,    mm1
    movd        [edi],  mm1

    mov         eax,    8
    add         esi,    4

loopcx1:

    FILT_ALL    esi

    movq        [ebp + 2 * eax],  mm1
    paddw       mm1,    mm7
    psraw       mm1,    5
    packuswb    mm1,    mm1
    movd        [edi + eax - 4],  mm1

    add         esi,    4
    add         eax,    4
    cmp         eax,    [esp + twidth]
    jnz         loopcx1

    FILT_ALL    esi

    pshufw      mm2,    mm1,  7
    movq        [ebp + 2 * eax],  mm1
    movq        [ebp + 2 * eax + 8],  mm2
    paddw       mm1,    mm7
    psraw       mm1,    5
    packuswb    mm1,    mm1
    movd        [edi + eax - 4],  mm1

    mov         esi,    [esp + tsrc]
    add         esi,    ecx
    mov         [esp + tsrc],  esi

    add         edi,    [esp + tdstp1]
    mov         [esp + tdst1], edi

    mov         edi,    [esp + tdst2]
;   mov         eax,    [esp + twidth]
    sub         eax,    4

    picpush     ebx
    picgetgot   ebx

loopcx2:

    movq        mm2,    [picesp + 2 * eax + 2  + 4 + tbuffer]
    movq        mm3,    [picesp + 2 * eax + 4  + 4 + tbuffer]
    movq        mm4,    [picesp + 2 * eax + 6  + 4 + tbuffer]
    movq        mm5,    [picesp + 2 * eax + 8  + 4 + tbuffer]
    movq        mm1,    [picesp + 2 * eax      + 4 + tbuffer]
    movq        mm6,    [picesp + 2 * eax + 10 + 4 + tbuffer]
    paddw       mm2,    mm5
    paddw       mm3,    mm4
    paddw       mm1,    mm6

    movq        mm5,    [mmx_dw_20 GOT_ebx]
    movq        mm4,    [mmx_dw_5 GOT_ebx]
    movq        mm6,    mm1
    pxor        mm7,    mm7

    punpckhwd   mm5,    mm2
    punpcklwd   mm4,    mm3
    punpcklwd   mm2,    [mmx_dw_20 GOT_ebx]
    punpckhwd   mm3,    [mmx_dw_5 GOT_ebx]

    pcmpgtw     mm7,    mm1

    pmaddwd     mm2,    mm4
    pmaddwd     mm3,    mm5

    punpcklwd   mm1,    mm7
    punpckhwd   mm6,    mm7

    paddd       mm2,    mm1
    paddd       mm3,    mm6

    paddd       mm2,    [mmx_dd_one GOT_ebx]
    paddd       mm3,    [mmx_dd_one GOT_ebx]

    psrad       mm2,    10
    psrad       mm3,    10

    packssdw    mm2,    mm3
    packuswb    mm2,    mm0

    movd        [edi + eax], mm2

    sub         eax,    4
    jge         loopcx2

    add         edi,    [picesp + tdstp2]
    mov         [picesp + tdst2], edi

    dec         dword [picesp + theight]
    jnz         loopcy

    picpop      ebx

    add         esp,    [esp + toffset]

    pop         ebp
    pop         ebx
    pop         esi
    pop         edi

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
    push edi
    push esi

    mov         edi,    [esp + 12]           ; dst
    mov         esi,    [esp + 20]           ; src

    pxor        mm0,    mm0
    picpush     ebx
    picgetgot   ebx
    movq        mm7,    [mmx_dw_one GOT_ebx]
    picpop      ebx

    mov         ecx,    [esp + 32]           ; height

    sub         esi,    2

loophy:

    xor         eax,    eax

loophx:

    prefetchnta [esi + eax + 48]       

    LOAD_4      mm1,    mm2, mm3, mm4, [esi + eax], [esi + eax + 1], [esi + eax + 2], [esi + eax + 3], mm0
    FILT_2      mm1,    mm2
    movd        mm5,    [esi + eax + 4]
    movd        mm6,    [esi + eax + 5]
    FILT_4      mm1,    mm3, mm4
    movd        mm2,    [esi + eax + 4]
    movd        mm3,    [esi + eax + 6]
    punpcklbw   mm5,    mm0
    punpcklbw   mm6,    mm0
    FILT_6      mm1,    mm5, mm6, mm7
    movd        mm4,    [esi + eax + 7]
    movd        mm5,    [esi + eax + 8]
    punpcklbw   mm2,    mm0
    punpcklbw   mm3,    mm0                  ; mm2(1), mm3(20), mm6(-5) ready
    FILT_2      mm2,    mm6
    movd        mm6,    [esi + eax + 9]
    punpcklbw   mm4,    mm0
    punpcklbw   mm5,    mm0                  ; mm2(1-5), mm3(20), mm4(20), mm5(-5) ready
    FILT_4      mm2,    mm3, mm4
    punpcklbw   mm6,    mm0
    FILT_6      mm2,    mm5, mm6, mm7

    packuswb    mm1,    mm2
    movq        [edi + eax],  mm1

    add         eax,    8
    cmp         eax,    [esp + 28]           ; width
    jnz         loophx

    add         esi,    [esp + 24]           ; src_pitch
    add         edi,    [esp + 16]           ; dst_pitch

    dec         ecx
    jnz         loophy

    pop         esi
    pop         edi

    ret

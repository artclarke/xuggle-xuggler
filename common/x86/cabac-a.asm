;*****************************************************************************
;* cabac-a.asm: h264 encoder library
;*****************************************************************************
;* Copyright (C) 2008 x264 project
;*
;* Author: Loren Merritt <lorenm@u.washington.edu>
;*         Jason Garrett-Glaser <darkshikari@gmail.com>
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

SECTION_RODATA

SECTION .text

cextern x264_cabac_range_lps
cextern x264_cabac_transition
cextern x264_cabac_renorm_shift

; t3 must be ecx, since it's used for shift.
%ifdef WIN64
    DECLARE_REG_TMP 3,1,2,0,4,5,6,10
    %define pointer resq
%elifdef ARCH_X86_64
    DECLARE_REG_TMP 0,1,2,3,4,5,6,10
    %define pointer resq
%else
    DECLARE_REG_TMP 0,3,2,1,4,5,6,3
    %define pointer resd
%endif

struc cb
    .low: resd 1
    .range: resd 1
    .queue: resd 1
    .bytes_outstanding: resd 1
    .start: pointer 1
    .p: pointer 1
    .end: pointer 1
    align 16, resb 1
    .bits_encoded: resd 1
    .state: resb 460
endstruc

%macro LOAD_GLOBAL 4
%ifdef PIC
    ; this would be faster if the arrays were declared in asm, so that I didn't have to duplicate the lea
    lea   r11, [%2 GLOBAL]
    %ifnidn %3, 0
    add   r11, %3
    %endif
    movzx %1, byte [r11+%4]
%else
    movzx %1, byte [%2+%3+%4]
%endif
%endmacro

cglobal x264_cabac_encode_decision_asm, 0,7
    movifnidn t0,  r0mp
    movifnidn t1d, r1m
    mov   t5d, [t0+cb.range]
    movzx t3d, byte [t0+cb.state+t1]
    mov   t4d, t5d
    shr   t5d, 6
    and   t5d, 3
    LOAD_GLOBAL t5d, x264_cabac_range_lps, t5, t3*4
    sub   t4d, t5d
    mov   t6d, t3d
    shr   t6d, 6
    movifnidn t2d, r2m
    cmp   t6d, t2d
    mov   t6d, [t0+cb.low]
    lea   t7,  [t6+t4]
    cmovne t4d, t5d
    cmovne t6d, t7d
    LOAD_GLOBAL t3d, x264_cabac_transition, t2, t3*2
    movifnidn t1d, r1m
    mov   [t0+cb.state+t1], t3b
.renorm:
    mov   t3d, t4d
    shr   t3d, 3
    LOAD_GLOBAL t3d, x264_cabac_renorm_shift, 0, t3
    shl   t4d, t3b
    shl   t6d, t3b
    add   t3d, [t0+cb.queue]
    mov   [t0+cb.range], t4d
    mov   [t0+cb.low], t6d
    mov   [t0+cb.queue], t3d
    cmp   t3d, 8
    jge .putbyte
    REP_RET
.putbyte:
    ; alive: t0=cb t3=queue t6=low
    add   t3d, 2
    mov   t1d, 1
    mov   t2d, t6d
    shl   t1d, t3b
    shr   t2d, t3b ; out
    dec   t1d
    sub   t3d, 10
    and   t6d, t1d
    cmp   t2b, 0xff ; FIXME is a 32bit op faster?
    mov   [t0+cb.queue], t3d
    mov   [t0+cb.low], t6d
    mov   t1d, t2d
    mov   t4,  [t0+cb.p]
    je .postpone
    mov   t5d, [t0+cb.bytes_outstanding]
    shr   t1d, 8 ; carry
    add   [t4-1], t1b
    test  t5d, t5d
    jz .no_outstanding
    dec   t1d
.loop_outstanding:
    mov   [t4], t1b
    inc   t4
    dec   t5d
    jg .loop_outstanding
.no_outstanding:
    mov   [t4], t2b
    inc   t4
    mov   [t0+cb.bytes_outstanding], t5d ; is zero, but a reg has smaller opcode than an immediate
    mov   [t0+cb.p], t4
    RET
.postpone:
    inc   dword [t0+cb.bytes_outstanding]
    RET


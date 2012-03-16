;*****************************************************************************
;* bitstream-a.asm: x86 bitstream functions
;*****************************************************************************
;* Copyright (C) 2010-2012 x264 project
;*
;* Authors: Jason Garrett-Glaser <darkshikari@gmail.com>
;*          Henrik Gramner <hengar-6@student.ltu.se>
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

SECTION .text

;-----------------------------------------------------------------------------
; uint8_t *x264_nal_escape( uint8_t *dst, uint8_t *src, uint8_t *end )
;-----------------------------------------------------------------------------

%macro NAL_LOOP 2
%1_escape:
    ; Detect false positive to avoid unneccessary escape loop
    xor      r3d, r3d
    cmp byte [r0+r1-1], 0
    setnz    r3b
    xor      r3d, r4d
    jnz .escape
    jmp %1_continue
ALIGN 16
%1:
    pcmpeqb   m3, m1, m4
    pcmpeqb   m2, m0, m4
    pmovmskb r3d, m3
    %2   [r0+r1], m0
    pmovmskb r4d, m2
    shl      r3d, mmsize
    mova      m0, [r1+r2+2*mmsize]
    or       r4d, r3d
    %2 [r0+r1+mmsize], m1
    lea      r3d, [r4+r4+1]
    mova      m1, [r1+r2+3*mmsize]
    and      r4d, r3d
    jnz %1_escape
%1_continue:
    add       r1, 2*mmsize
    jl %1
%endmacro

%macro NAL_ESCAPE 0

cglobal nal_escape, 3,5
    mov      r3w, [r1]
    sub       r1, r2 ; r1 = offset of current src pointer from end of src
    pxor      m4, m4
    sub       r0, r1 ; r0 = projected end of dst, assuming no more escapes
    mov  [r0+r1], r3w
    add       r1, 2
    jge .ret

    ; Start off by jumping into the escape loop in
    ; case there's an escape at the start.
    ; And do a few more in scalar until src is aligned again.
    jmp .first_escape

    NAL_LOOP .loop_aligned, mova
%if mmsize==16
    jmp .ret
    NAL_LOOP .loop_unaligned, movu
%endif
.ret:
    movifnidn rax, r0
    RET

ALIGN 16
.escape:
    ; Skip bytes that are known to be valid
    and      r4d, r3d
    tzcnt    r3d, r4d
    add       r1, r3
.escape_loop:
    inc       r1
    jge .ret
.first_escape:
    movzx    r3d, byte [r1+r2]
    lea       r4, [r1+r2]
    cmp      r3d, 3
    jna .escape_check
.no_escape:
    mov  [r0+r1], r3b
    test     r4d, mmsize-1 ; Do SIMD when src is aligned
    jnz .escape_loop
    mova      m0, [r4]
    mova      m1, [r4+mmsize]
%if mmsize==16
    lea      r4d, [r0+r1]
    test     r4d, mmsize-1
    jnz .loop_unaligned
%endif
    jmp .loop_aligned

ALIGN 16
.escape_check:
    cmp word [r0+r1-2], 0
    jnz .no_escape
    mov byte [r0+r1], 3
    inc      r0
    jmp .no_escape
%endmacro

INIT_MMX mmx2
NAL_ESCAPE
INIT_XMM sse2
NAL_ESCAPE
INIT_XMM avx
NAL_ESCAPE

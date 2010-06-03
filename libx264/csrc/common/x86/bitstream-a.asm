;*****************************************************************************
;* bitstream-a.asm: h264 encoder library
;*****************************************************************************
;* Copyright (C) 2010 x264 project
;*
;* Authors: Jason Garrett-Glaser <darkshikari@gmail.com>
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
%include "x86util.asm"

SECTION .text

;-----------------------------------------------------------------------------
; uint8_t *x264_nal_escape( uint8_t *dst, uint8_t *src, uint8_t *end )
;-----------------------------------------------------------------------------

%macro NAL_LOOP 2
ALIGN 16
%1:
    mova      m0, [r1+r2]
    mova      m1, m0
%if mmsize == 8
    psllq     m0, 8
%else
    pslldq    m0, 1
%endif
    %2   [r0+r1], m1
    por       m1, m0
    pcmpeqb   m1, m2
    pmovmskb r3d, m1
    test     r3d, r3d
    jnz .escape
    add       r1, mmsize
    jl %1
%endmacro

%macro NAL_ESCAPE 1

cglobal nal_escape_%1, 3,5
    pxor      m2, m2
    sub       r1, r2 ; r1 = offset of current src pointer from end of src
    sub       r0, r1 ; r0 = projected end of dst, assuming no more escapes

    mov      r3b, [r1+r2]
    mov  [r0+r1], r3b
    inc       r1
    jge .ret

    ; Start off by jumping into the escape loop in
    ; case there's an escape at the start.
    ; And do a few more in scalar until src is aligned again.
    lea      r4d, [r1+r2]
    or       r4d, -mmsize
    neg      r4d
    jmp .first_escape

    NAL_LOOP .loop_aligned, mova
%if mmsize==16
    NAL_LOOP .loop_unaligned, movu
%endif

.ret:
    movifnidn rax, r0
    RET
ALIGN 16
.escape:
    mov      r4d, mmsize
.first_escape:
    mov      r3b, [r1+r2]
.escape_loop:
    mov  [r0+r1], r3b
    inc      r1
    jge .ret
    mov      r3b, [r1+r2]
    cmp      r3b, 3
    jna .escape_check
.no_escape:
    dec      r4d
    jg .escape_loop
%if mmsize==16
    lea      r4d, [r0+r1]
    test     r4d, mmsize-1
    jnz .loop_unaligned
%endif
    jmp .loop_aligned
.escape_check:
    cmp word [r0+r1-2], 0
    jnz .no_escape
    mov byte [r0+r1], 3
    inc      r0
    jmp .no_escape
%endmacro

INIT_MMX
NAL_ESCAPE mmxext
INIT_XMM
NAL_ESCAPE sse2

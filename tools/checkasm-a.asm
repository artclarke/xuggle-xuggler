;*****************************************************************************
;* checkasm-a.asm
;*****************************************************************************
;* Copyright (C) 2008 Loren Merritt <lorenm@u.washington.edu>
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

error_message: db "failed to preserve register", 10, 0

SECTION .text

cextern printf

; max number of args used by any x264 asm function.
; (max_args % 4) must equal 3 for stack alignment
%define max_args 11

; just random numbers to reduce the chance of incidental match
%define n3 dword 0x6549315c
%define n4 dword 0xe02f3e23
%define n5 dword 0xb78d0d1d
%define n6 dword 0x33627ba7

%ifndef ARCH_X86_64
;-----------------------------------------------------------------------------
; long x264_checkasm_call( long (*func)(), int *ok, ... )
;-----------------------------------------------------------------------------
cglobal x264_checkasm_call, 1,7
    mov  r3, n3
    mov  r4, n4
    mov  r5, n5
    mov  r6, n6
%rep max_args
    push dword [esp+24+max_args*4]
%endrep
    call r0
    add  esp, max_args*4
    xor  r3, n3
    xor  r4, n4
    xor  r5, n5
    xor  r6, n6
    or   r3, r4
    or   r5, r6
    or   r3, r5
    jz .ok
    mov  r3, eax
    lea  r1, [error_message GLOBAL]
    push r1
    xor  eax, eax
    call printf
    add  esp, 4
    mov  r1, r1m
    mov  dword [r1], 0
    mov  eax, r3
.ok:
    RET
%endif ; ARCH_X86_64

;-----------------------------------------------------------------------------
; int x264_stack_pagealign( int (*func)(), int align )
;-----------------------------------------------------------------------------
cglobal x264_stack_pagealign, 2,2
    push rbp
    mov  rbp, rsp
    and  rsp, ~0xfff
    sub  rsp, r1
    call r0
    leave
    RET


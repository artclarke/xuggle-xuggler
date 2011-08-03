;*****************************************************************************
;* checkasm-a.asm: assembly check tool
;*****************************************************************************
;* Copyright (C) 2008-2011 x264 project
;*
;* Authors: Loren Merritt <lorenm@u.washington.edu>
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

SECTION_RODATA

error_message: db "failed to preserve register", 0

%ifdef WIN64
; just random numbers to reduce the chance of incidental match
ALIGN 16
n4:   dq 0xa77809bf11b239d1
n5:   dq 0x2ba9bf3d2f05b389
x6:  ddq 0x79445c159ce790641a1b2550a612b48c
x7:  ddq 0x86b2536fcd8cf6362eed899d5a28ddcd
x8:  ddq 0x3f2bf84fc0fcca4eb0856806085e7943
x9:  ddq 0xd229e1f5b281303facbd382dcf5b8de2
x10: ddq 0xab63e2e11fa38ed971aeaff20b095fd9
x11: ddq 0x77d410d5c42c882d89b0c0765892729a
x12: ddq 0x24b3c1d2a024048bc45ea11a955d8dd5
x13: ddq 0xdd7b8919edd427862e8ec680de14b47c
x14: ddq 0x11e53e2b2ac655ef135ce6888fa02cbf
x15: ddq 0x6de8f4c914c334d5011ff554472a7a10
%endif

SECTION .text

cextern_naked puts

; max number of args used by any x264 asm function.
; (max_args % 4) must equal 3 for stack alignment
%define max_args 11

%ifdef WIN64

;-----------------------------------------------------------------------------
; intptr_t x264_checkasm_call( intptr_t (*func)(), int *ok, ... )
;-----------------------------------------------------------------------------
INIT_XMM
cglobal checkasm_call, 4,7,16
    sub  rsp, max_args*8
    %assign stack_offset stack_offset+max_args*8
    mov  r6, r0
    mov  [rsp+stack_offset+16], r1
    mov  r0, r2
    mov  r1, r3
    mov r2d, r4m ; FIXME truncates pointer
    mov r3d, r5m ; FIXME truncates pointer
%assign i 4
%rep max_args-4
    mov  r4, [rsp+stack_offset+8+(i+2)*8]
    mov  [rsp+i*8], r4
    %assign i i+1
%endrep
%assign i 6
%rep 16-6
    movdqa xmm %+ i, [x %+ i]
    %assign i i+1
%endrep
    mov  r4, [n4]
    mov  r5, [n5]
    call r6
    xor  r4, [n4]
    xor  r5, [n5]
    or   r4, r5
    pxor xmm5, xmm5
%assign i 6
%rep 16-6
    pxor xmm %+ i, [x %+ i]
    por  xmm5, xmm %+ i
    %assign i i+1
%endrep
    packsswb xmm5, xmm5
    movq r5, xmm5
    or   r4, r5
    jz .ok
    mov  r4, rax
    lea  r0, [error_message]
    call puts
    mov  r1, [rsp+stack_offset+16]
    mov  dword [r1], 0
    mov  rax, r4
.ok:
    add  rsp, max_args*8
    %assign stack_offset stack_offset-max_args*8
    RET

%elifndef ARCH_X86_64

; just random numbers to reduce the chance of incidental match
%define n3 dword 0x6549315c
%define n4 dword 0xe02f3e23
%define n5 dword 0xb78d0d1d
%define n6 dword 0x33627ba7

;-----------------------------------------------------------------------------
; intptr_t x264_checkasm_call( intptr_t (*func)(), int *ok, ... )
;-----------------------------------------------------------------------------
cglobal checkasm_call, 1,7
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
    lea  r1, [error_message]
    push r1
    call puts
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
cglobal stack_pagealign, 2,2
    push rbp
    mov  rbp, rsp
    and  rsp, ~0xfff
    sub  rsp, r1
    call r0
    leave
    RET


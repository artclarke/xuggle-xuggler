;*****************************************************************************
;* x86inc.asm
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
;* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111, USA.
;*****************************************************************************

%ifdef WIN64
%define ARCH_X86_64
%endif

%ifdef ARCH_X86_64
%include "x86inc-64.asm"
%else
%include "x86inc-32.asm"
%endif

; Macros to eliminate most code duplication between x86_32 and x86_64:
; Currently this works only for leaf functions which load all their arguments
; into registers at the start, and make no other use of the stack. Luckily that
; covers most of x264's asm.

; PROLOGUE:
; %1 = number of arguments. loads them from stack if needed.
; %2 = number of registers used, not including PIC. pushes callee-saved regs if needed.
; %3 = whether global constants are used in this function. inits x86_32 PIC if needed.
; PROLOGUE can also be invoked by adding the same options to cglobal

; TODO Some functions can use some args directly from the stack. If they're the
; last args then you can just not declare them, but if they're in the middle
; we need more flexible macro.

; RET:
; Pops anything that was pushed by PROLOGUE

; REP_RET:
; Same, but if it doesn't pop anything it becomes a 2-byte ret, for athlons
; which are slow when a normal ret follows a branch.

%macro DECLARE_REG 5
    %define r%1q %2
    %define r%1d %3
    %define r%1w %4
    ; no r%1b, because some regs don't have a byte form, and anyway x264 doesn't need it
    %define r%1m %5
    %define r%1  r%1q
%endmacro

%macro DECLARE_REG_SIZE 1
    %define r%1q r%1
    %define e%1q r%1
    %define r%1d e%1
    %define e%1d e%1
    %define r%1w %1
    %define e%1w %1
%ifndef ARCH_X86_64
    %define r%1  e%1
%endif
%endmacro

DECLARE_REG_SIZE ax
DECLARE_REG_SIZE bx
DECLARE_REG_SIZE cx
DECLARE_REG_SIZE dx
DECLARE_REG_SIZE si
DECLARE_REG_SIZE di
DECLARE_REG_SIZE bp

%ifdef ARCH_X86_64
    %define push_size 8
%else
    %define push_size 4
%endif

%macro PUSH 1
    push %1
    %assign stack_offset stack_offset+push_size
%endmacro

%macro POP 1
    pop %1
    %assign stack_offset stack_offset-push_size
%endmacro

%macro SUB 2
    sub %1, %2
    %ifidn %1, rsp
        %assign stack_offset stack_offset+(%2)
    %endif
%endmacro

%macro ADD 2
    add %1, %2
    %ifidn %1, rsp
        %assign stack_offset stack_offset-(%2)
    %endif
%endmacro

%macro movifnidn 2
    %ifnidn %1, %2
        mov %1, %2
    %endif
%endmacro

%macro movsxdifnidn 2
    %ifnidn %1, %2
        movsxd %1, %2
    %endif
%endmacro

%macro ASSERT 1
    %if (%1) == 0
        %error assert failed
    %endif
%endmacro

%ifdef WIN64 ;================================================================

DECLARE_REG 0, rcx, ecx, cx,  ecx
DECLARE_REG 1, rdx, edx, dx,  edx
DECLARE_REG 2, r8,  r8d, r8w, r8d
DECLARE_REG 3, r9,  r9d, r9w, r9d
DECLARE_REG 4, rdi, edi, di,  [rsp + stack_offset + 40]
DECLARE_REG 5, rsi, esi, si,  [rsp + stack_offset + 48]
DECLARE_REG 6, rax, eax, ax,  [rsp + stack_offset + 56]
%define r7m [rsp + stack_offset + 64]

%macro LOAD_IF_USED 2 ; reg_id, number_of_args
    %if %1 < %2
        mov r%1, [rsp + 8 + %1*8]
    %endif
%endmacro

%macro PROLOGUE 3
    ASSERT %2 >= %1
    ASSERT %2 <= 7
    %assign stack_offset 0
    LOAD_IF_USED 4, %1
    LOAD_IF_USED 5, %1
    LOAD_IF_USED 6, %1
%endmacro

%macro RET 0
    ret
%endmacro

%macro REP_RET 0
    rep ret
%endmacro

%elifdef ARCH_X86_64 ;========================================================

DECLARE_REG 0, rdi, edi, di,  edi
DECLARE_REG 1, rsi, esi, si,  esi
DECLARE_REG 2, rdx, edx, dx,  edx
DECLARE_REG 3, rcx, ecx, cx,  ecx
DECLARE_REG 4, r8,  r8d, r8w, r8d
DECLARE_REG 5, r9,  r9d, r9w, r9d
DECLARE_REG 6, rax, eax, ax,  [rsp + stack_offset + 8]
%define r7m [rsp + stack_offset + 16]

%macro LOAD_IF_USED 2 ; reg_id, number_of_args
    %if %1 < %2
        mov r%1, [rsp - 40 + %1*8]
    %endif
%endmacro

%macro PROLOGUE 3
    ASSERT %2 >= %1
    ASSERT %2 <= 7
    %assign stack_offset 0
    LOAD_IF_USED 6, %1
%endmacro

%macro RET 0
    ret
%endmacro

%macro REP_RET 0
    rep ret
%endmacro

%else ; X86_32 ;==============================================================

DECLARE_REG 0, eax, eax, ax, [esp + stack_offset + 4]
DECLARE_REG 1, ecx, ecx, cx, [esp + stack_offset + 8]
DECLARE_REG 2, edx, edx, dx, [esp + stack_offset + 12]
DECLARE_REG 3, ebx, ebx, bx, [esp + stack_offset + 16]
DECLARE_REG 4, esi, esi, si, [esp + stack_offset + 20]
DECLARE_REG 5, edi, edi, di, [esp + stack_offset + 24]
DECLARE_REG 6, ebp, ebp, bp, [esp + stack_offset + 28]
%define r7m [esp + stack_offset + 32]
%define rsp esp

%macro PUSH_IF_USED 1 ; reg_id
    %if %1 < regs_used
        push r%1
        %assign stack_offset stack_offset+4
    %endif
%endmacro

%macro POP_IF_USED 1 ; reg_id
    %if %1 < regs_used
        pop r%1
    %endif
%endmacro

%macro LOAD_IF_USED 2 ; reg_id, number_of_args
    %if %1 < %2
        mov r%1, [esp + stack_offset + 4 + %1*4]
    %endif
%endmacro

%macro PROLOGUE 3
    ASSERT %2 >= %1
    %assign stack_offset 0
    %assign regs_used %2
    %ifdef __PIC__
    %if %3
        %assign regs_used regs_used+1
    %endif
    %endif
    ASSERT regs_used <= 7
    PUSH_IF_USED 3
    PUSH_IF_USED 4
    PUSH_IF_USED 5
    PUSH_IF_USED 6
    LOAD_IF_USED 0, %1
    LOAD_IF_USED 1, %1
    LOAD_IF_USED 2, %1
    LOAD_IF_USED 3, %1
    LOAD_IF_USED 4, %1
    LOAD_IF_USED 5, %1
    LOAD_IF_USED 6, %1
    %if %3
        picgetgot r%2
    %endif
%endmacro

%macro RET 0
    POP_IF_USED 6
    POP_IF_USED 5
    POP_IF_USED 4
    POP_IF_USED 3
    ret
%endmacro

%macro REP_RET 0
    %if regs_used > 3
        RET
    %else
        rep ret
    %endif
%endmacro

%endif ;======================================================================



;=============================================================================
; arch-independent part
;=============================================================================

%assign function_align 16

; Symbol prefix for C linkage
%macro cglobal 1
    %ifidn __OUTPUT_FORMAT__,elf
        %ifdef PREFIX
            global _%1:function hidden
            %define %1 _%1
        %else
            global %1:function hidden
        %endif
    %else
        %ifdef PREFIX
            global _%1
            %define %1 _%1
        %else
            global %1
        %endif
    %endif
%ifdef WIN64
    %define %1 pad %1
%endif
    align function_align
    %1:
%endmacro

%macro cglobal 3
    cglobal %1
    PROLOGUE %2, %3, 0
%endmacro

%macro cglobal 4
    cglobal %1
    PROLOGUE %2, %3, %4
%endmacro

%macro cextern 1
    %ifdef PREFIX
        extern _%1
        %define %1 _%1
    %else
        extern %1
    %endif
%endmacro

; This is needed for ELF, otherwise the GNU linker assumes the stack is
; executable by default.
%ifidn __OUTPUT_FORMAT__,elf
SECTION ".note.GNU-stack" noalloc noexec nowrite progbits
%endif

%assign FENC_STRIDE 16
%assign FDEC_STRIDE 32

%macro INIT_MMX 0
    %undef  movq
    %define m0 mm0
    %define m1 mm1
    %define m2 mm2
    %define m3 mm3
    %define m4 mm4
    %define m5 mm5
    %define m6 mm6
    %define m7 mm7
    %undef  m8
    %undef  m9
%endmacro

%macro INIT_XMM 0
    %define movq movdqa
    %define m0 xmm0
    %define m1 xmm1
    %define m2 xmm2
    %define m3 xmm3
    %define m4 xmm4
    %define m5 xmm5
    %define m6 xmm6
    %define m7 xmm7
    %define m8 xmm8
    %define m9 xmm9
%endmacro


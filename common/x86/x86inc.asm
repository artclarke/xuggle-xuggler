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

%macro DECLARE_REG 6
    %define r%1q %2
    %define r%1d %3
    %define r%1w %4
    %define r%1b %5
    %define r%1m %6
    %define r%1  %2
%endmacro

%macro DECLARE_REG_SIZE 2
    %define r%1q r%1
    %define e%1q r%1
    %define r%1d e%1
    %define e%1d e%1
    %define r%1w %1
    %define e%1w %1
    %define r%1b %2
    %define e%1b %2
%ifndef ARCH_X86_64
    %define r%1  e%1
%endif
%endmacro

DECLARE_REG_SIZE ax, al
DECLARE_REG_SIZE bx, bl
DECLARE_REG_SIZE cx, cl
DECLARE_REG_SIZE dx, dl
DECLARE_REG_SIZE si, sil
DECLARE_REG_SIZE di, dil
DECLARE_REG_SIZE bp, bpl

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

%ifdef ARCH_X86_64 ;========================================================

DECLARE_REG 0, rdi, edi, di,  dil, edi
DECLARE_REG 1, rsi, esi, si,  sil, esi
DECLARE_REG 2, rdx, edx, dx,  dl,  edx
DECLARE_REG 3, rcx, ecx, cx,  cl,  ecx
DECLARE_REG 4, r8,  r8d, r8w, r8b, r8d
DECLARE_REG 5, r9,  r9d, r9w, r9b, r9d
DECLARE_REG 6, rax, eax, ax,  al,  [rsp + stack_offset + 8]
%define r7m [rsp + stack_offset + 16]
%define r8m [rsp + stack_offset + 24]

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

DECLARE_REG 0, eax, eax, ax, al,   [esp + stack_offset + 4]
DECLARE_REG 1, ecx, ecx, cx, cl,   [esp + stack_offset + 8]
DECLARE_REG 2, edx, edx, dx, dl,   [esp + stack_offset + 12]
DECLARE_REG 3, ebx, ebx, bx, bl,   [esp + stack_offset + 16]
DECLARE_REG 4, esi, esi, si, null, [esp + stack_offset + 20]
DECLARE_REG 5, edi, edi, di, null, [esp + stack_offset + 24]
DECLARE_REG 6, ebp, ebp, bp, null, [esp + stack_offset + 28]
%define r7m [esp + stack_offset + 32]
%define r8m [esp + stack_offset + 36]
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
    align function_align
    %1:
    RESET_MM_PERMUTATION ; not really needed, but makes disassembly somewhat nicer
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
SECTION .note.GNU-stack noalloc noexec nowrite progbits
%endif

%assign FENC_STRIDE 16
%assign FDEC_STRIDE 32

; merge mmx and sse*

%macro INIT_MMX 0
    %define RESET_MM_PERMUTATION INIT_MMX
    %define regsize 8
    %define mova movq
    %define movu movq
    %define movh movd
    %define movnt movntq
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
    %undef  m10
    %undef  m11
    %undef  m12
    %undef  m13
    %undef  m14
    %undef  m15
%endmacro

%macro INIT_XMM 0
    %define RESET_MM_PERMUTATION INIT_XMM
    %define regsize 16
    %define mova movdqa
    %define movu movdqu
    %define movh movq
    %define movnt movntdq
    %define m0 xmm0
    %define m1 xmm1
    %define m2 xmm2
    %define m3 xmm3
    %define m4 xmm4
    %define m5 xmm5
    %define m6 xmm6
    %define m7 xmm7
    %ifdef ARCH_X86_64
    %define m8 xmm8
    %define m9 xmm9
    %define m10 xmm10
    %define m11 xmm11
    %define m12 xmm12
    %define m13 xmm13
    %define m14 xmm14
    %define m15 xmm15
    %endif
%endmacro

INIT_MMX

; I often want to use macros that permute their arguments. e.g. there's no
; efficient way to implement butterfly or transpose or dct without swapping some
; arguments.
;
; I would like to not have to manually keep track of the permutations:
; If I insert a permutation in the middle of a function, it should automatically
; change everything that follows. For more complex macros I may also have multiple
; implementations, e.g. the SSE2 and SSSE3 versions may have different permutations.
;
; Hence these macros. Insert a PERMUTE or some SWAPs at the end of a macro that
; permutes its arguments. It's equivalent to exchanging the contents of the
; registers, except that this way you exchange the register names instead, so it
; doesn't cost any cycles.

%macro PERMUTE 2-* ; takes a list of pairs to swap
%rep %0/2
    %xdefine tmp%2 m%2
    %rotate 2
%endrep
%rep %0/2
    %xdefine m%1 tmp%2
    %undef tmp%2
    %rotate 2
%endrep
%endmacro

%macro SWAP 2-* ; swaps a single chain (sometimes more concise than pairs)
%rep %0-1
    %xdefine tmp m%1
    %xdefine m%1 m%2
    %xdefine m%2 tmp
    %undef tmp
    %rotate 1
%endrep
%endmacro

%macro SAVE_MM_PERMUTATION 1
    %xdefine %1_m0 m0
    %xdefine %1_m1 m1
    %xdefine %1_m2 m2
    %xdefine %1_m3 m3
    %xdefine %1_m4 m4
    %xdefine %1_m5 m5
    %xdefine %1_m6 m6
    %xdefine %1_m7 m7
    %ifdef ARCH_X86_64
    %xdefine %1_m8 m8
    %xdefine %1_m9 m9
    %xdefine %1_m10 m10
    %xdefine %1_m11 m11
    %xdefine %1_m12 m12
    %xdefine %1_m13 m13
    %xdefine %1_m14 m14
    %xdefine %1_m15 m15
    %endif
%endmacro

%macro LOAD_MM_PERMUTATION 1
    %xdefine m0 %1_m0
    %xdefine m1 %1_m1
    %xdefine m2 %1_m2
    %xdefine m3 %1_m3
    %xdefine m4 %1_m4
    %xdefine m5 %1_m5
    %xdefine m6 %1_m6
    %xdefine m7 %1_m7
    %ifdef ARCH_X86_64
    %xdefine m8 %1_m8
    %xdefine m9 %1_m9
    %xdefine m10 %1_m10
    %xdefine m11 %1_m11
    %xdefine m12 %1_m12
    %xdefine m13 %1_m13
    %xdefine m14 %1_m14
    %xdefine m15 %1_m15
    %endif
%endmacro

%macro call 1
    call %1
    %ifdef %1_m0
        LOAD_MM_PERMUTATION %1
    %endif
%endmacro

; substitutions which are functionally identical but reduce code size
%define movdqa movaps
%define movdqu movups


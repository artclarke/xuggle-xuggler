;*****************************************************************************
;* i386inc.asm: h264 encoder library
;*****************************************************************************
;* Copyright (C) 2006 x264 project
;*
;* Author: Sam Hocevar <sam@zoy.org>
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

; Symbol prefix for C linkage
%macro cglobal 1
    %ifdef PREFIX
        global _%1
        %define %1 _%1
    %else
        global %1
    %endif
    align 16
    %1:
%endmacro

%macro cextern 1
    %ifdef PREFIX
        extern _%1
        %define %1 _%1
    %else
        extern %1
    %endif
%endmacro

; Name of the .rodata section. On OS X we cannot use .rodata because NASM
; is unable to compute address offsets outside of .text so we use the .text
; section instead until NASM is fixed.
%macro SECTION_RODATA 0
    %ifidn __OUTPUT_FORMAT__,macho
        SECTION .text align=16
        fakegot:
    %else
        SECTION .rodata align=16
    %endif
%endmacro

; PIC support macros. All these macros are totally harmless when __PIC__ is
; not defined but can ruin everything if misused in PIC mode. On x86, shared
; objects cannot directly access global variables by address, they need to
; go through the GOT (global offset table). Most OSes do not care about it
; and let you load non-shared .so objects (Linux, Win32...). However, OS X
; requires PIC code in its .dylib objects.
;
; - GOT_* should be used as a suffix for global addressing, eg.
;     picgetgot ebx
;     mov eax, [foo GOT_ebx]
;   instead of
;     mov eax, [foo]
;
; - picgetgot computes the GOT address into the given register in PIC
;   mode, otherwise does nothing. You need to do this before using GOT_*.
;
; - picpush and picpop respectively push and pop the given register
;   in PIC mode, otherwise do nothing. You should always use them around
;   picgetgot except when sure that the register is no longer used and is
;   being restored later by other means.
;
; - picesp is defined to compensate the changing of esp when pushing
;   a register into the stack, eg.
;     mov eax, [esp + 8]
;     pushpic  ebx
;     mov eax, [picesp + 12]
;   instead of
;     mov eax, [esp + 8]
;     pushpic  ebx
;     mov eax, [esp + 12]
;
%ifdef __PIC__
    %ifidn __OUTPUT_FORMAT__,macho
        ; There is no real global offset table on OS X, but we still
        ; need to reference our variables by offset.
        %define GOT_eax - fakegot + eax
        %define GOT_ebx - fakegot + ebx
        %define GOT_ecx - fakegot + ecx
        %define GOT_edx - fakegot + edx
        %macro picgetgot 1
            call %%getgot 
          %%getgot: 
            pop %1 
            add %1, $$ - %%getgot
        %endmacro
    %else
        %ifidn __OUTPUT_FORMAT__,elf
            %define GOT _GLOBAL_OFFSET_TABLE_
        %else ; for a.out
            %define GOT __GLOBAL_OFFSET_TABLE_
        %endif
        extern GOT
        %define GOT_eax + eax wrt ..gotoff
        %define GOT_ebx + ebx wrt ..gotoff
        %define GOT_ecx + ecx wrt ..gotoff
        %define GOT_edx + edx wrt ..gotoff
        %macro picgetgot 1
            call %%getgot 
          %%getgot: 
            pop %1 
            add %1, GOT + $$ - %%getgot wrt ..gotpc 
        %endmacro
    %endif
    %macro picpush 1
        push %1
    %endmacro
    %macro picpop 1
        pop %1
    %endmacro
    %define picesp esp+4
%else
    %define GOT_eax
    %define GOT_ebx
    %define GOT_ecx
    %define GOT_edx
    %macro picgetgot 1
    %endmacro
    %macro picpush 1
    %endmacro
    %macro picpop 1
    %endmacro
    %define picesp esp
%endif

%assign FENC_STRIDE 16
%assign FDEC_STRIDE 32

; This is needed for ELF, otherwise the GNU linker assumes the stack is
; executable by default.
%ifidn __OUTPUT_FORMAT__,elf
SECTION ".note.GNU-stack" noalloc noexec nowrite progbits
%endif


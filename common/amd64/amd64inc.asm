;*****************************************************************************
;* amd64inc.asm: h264 encoder library
;*****************************************************************************
;* Copyright (C) 2005 x264 project
;*
;* Authors: Andrew Dunstan
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

BITS 64

; FIXME: All of the 64bit asm functions that take a stride as an argument
; via register, assume that the high dword of that register is filled with 0.
; This is true in practice (since we never do any 64bit arithmetic on strides),
; but is not guaranteed by the ABI.

%macro cglobal 1
    %ifdef PREFIX
        global _%1:function hidden
        %define %1 _%1
    %else
        global %1:function hidden
    %endif
%ifdef WIN64
    %define %1 pad %1
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

; Name of the .rodata section. On OS X we cannot use .rodata because YASM
; is unable to compute address offsets outside of .text so we use the .text
; section instead until YASM is fixed.
%macro SECTION_RODATA 0
    %ifidn __OUTPUT_FORMAT__,macho64
      SECTION .text align=16
    %else
      SECTION .rodata align=16
    %endif
%endmacro

%macro pad 1
    %undef %1
    %ifdef PREFIX
        %define %1 _%1
    %endif
    %ifdef WIN64
        times 6 nop
        align 16
        %1
        .startfunc
        %assign unwindcount 0
        %assign framereg 0
    %else
        align 16
        %1
    %endif
%endmacro

%ifdef WIN64

%define __PIC__

%define parm1q rcx
%define parm2q rdx
%define parm3q r8
%define parm4q r9
%define parm5q [rsp+40]
%define parm6q [rsp+48]
%define parm7q [rsp+56]
%define parm8q [rsp+64]
%define parm1d ecx
%define parm2d edx
%define parm3d r8d
%define parm4d r9d
%define parm5d dword parm5q
%define parm6d dword parm6q
%define parm7d dword parm7q
%define parm8d dword parm8q

%define temp1q rdi
%define temp2q rsi
%define temp1d edi
%define temp2d esi

%macro firstpush 1
    db 0x48
    push %1
%endmacro

%define unwindcode(count, code) .unwind %+ count EQU code

%define regcoderax 0
%define regcodercx 1
%define regcoderdx 2
%define regcoderbx 3
%define regcodersp 4
%define regcoderbp 5
%define regcodersi 6
%define regcoderdi 7
%define regcoder8 8
%define regcoder9 9
%define regcoder10 10
%define regcoder11 11
%define regcoder12 12
%define regcoder13 13
%define regcoder14 14
%define regcoder15 15
%define regcodexmm0 0
%define regcodexmm1 1
%define regcodexmm2 2
%define regcodexmm3 3
%define regcodexmm4 4
%define regcodexmm5 5
%define regcodexmm6 6
%define regcodexmm7 7
%define regcodexmm8 8
%define regcodexmm9 9
%define regcodexmm10 10
%define regcodexmm11 11
%define regcodexmm12 12
%define regcodexmm13 13
%define regcodexmm14 14
%define regcodexmm15 15

%macro allocstack 1
    %if %1 < 8
        %error Stack Allocation must be at least 8 bytes.
    %elif %1 < 129
        %assign unwindcount unwindcount+1
        unwindcode(unwindcount, $-.startfunc + 0x200 + (((%1-8)/8)<<12))
    %elif %1 < 524288
        %assign unwindcount unwindcount+1
        unwindcode(unwindcount, %1/8)
        %assign unwindcount unwindcount+1
        unwindcode(unwindcount, $-.startfunc + 0x100)
    %else
        %assign unwindcount unwindcount+1
        unwindcode(unwindcount, %1>>16)
        %assign unwindcount unwindcount+1
        unwindcode(unwindcount, %1 & 0x0000FFFF)
        %assign unwindcount unwindcount+1
        unwindcode(unwindcount, $-.startfunc + 0x1100)
    %endif
%endmacro

%macro pushreg 1
    %assign unwindcount unwindcount+1
    unwindcode(unwindcount, $-.startfunc + 0 + (regcode%1 << 12))
%endmacro

%macro setframe 2
    %if ((%2 % 16) | (%2 > 240) | (%2 < 0))
        %error Frame offset must be a multiple of 16 between 0 and 240.
    %endif
    %assign unwindcount unwindcount+1
    unwindcode(unwindcount, $-.startfunc + (3 << 8 )+ (regcode%1 << 12))
    %assign framereg regcode%1 + %2
%endmacro

%macro savereg 2
    %if ((%2 % 8) | (%2 < 0))
        %error Offset must be a positive multiple of 8.
    %endif
    %if (%2 < 64504)
        %assign unwindcount unwindcount+1
        unwindcode(unwindcount, %2/8)
        %assign unwindcount unwindcount +1
        unwindcode(unwindcount, $-.startfunc + (4 << 8) + (regcode%1 << 12))
    %else
        %assign unwindcount unwindcount+1
        unwindcode(unwindcount, %2 >> 16)
        %assign unwindcount unwindcount+1
        unwindcode(unwindcount, %2 & 0x0000FFFF)
        %assign unwindcount unwindcount+1
        unwindcode(unwindcount, $-.startfunc + (5 << 8) + (regcode%1 << 12))
    %endif
%endmacro

%macro savexmm128 2
    %if ((%2 % 16) | (%2 < 0))
        %error Offset must be a positive multiple of 16.
    %endif
    %if (%2 < 64512)
        %assign unwindcount unwindcount+1
        unwindcode(unwindcount, %2/16)
        %assign unwindcount unwindcount +1
        unwindcode(unwindcount, $-.startfunc + (8 << 8) + (regcode%1 << 12))
    %else
        %assign unwindcount unwindcount+1
        unwindcode(unwindcount, %2 >> 16)
        %assign unwindcount unwindcount+1
        unwindcode(unwindcount, %2 & 0x0000FFFF)
        %assign unwindcount unwindcount+1
        unwindcode(unwindcount, $-.startfunc + (9 << 8) + (regcode%1 << 12))
    %endif
%endmacro

%macro endprolog 0
.endprolog
SECTION .xdata
.unwindinfo
    db 0x01
    db .endprolog-.startfunc
    db unwindcount
    db framereg
    %rep unwindcount
        dw .unwind %+ unwindcount
        %assign unwindcount unwindcount-1
    %endrep
align 4,db 0
SECTION .text
%endmacro

%macro endfunc 0
.endfunc
SECTION .pdata
    dd .startfunc
    dd .endfunc
    dd .unwindinfo
SECTION .text
%endmacro

%else ;linux
%define parm1q rdi
%define parm2q rsi
%define parm3q rdx
%define parm4q rcx
%define parm5q r8
%define parm6q r9
%define parm7q [rsp+8]
%define parm8q [rsp+16]
%define parm1d edi
%define parm2d esi
%define parm3d edx
%define parm4d ecx
%define parm5d r8d
%define parm6d r9d
%define parm7d dword parm7q
%define parm8d dword parm8q

%define temp1q r9
%define temp2q r8
%define temp1d r9d
%define temp2d r8d

%macro allocstack 1
%endmacro

%macro firstpush 1
    push %1
%endmacro

%macro pushreg 1
%endmacro

%macro setframe 2
%endmacro

%macro savereg 2
%endmacro

%macro savexmm128 2
%endmacro

%define endprolog
%define endfunc

%endif ;linux

; PIC support macros. On x86_64 we just use RIP-relative addressing, which is
; much simpler than the GOT handling we need to perform on x86.
;
; - GLOBAL should be used as a suffix for global addressing, eg.
;     mov eax, [foo GLOBAL]
;   instead of
;     mov eax, [foo]
;
%ifdef __PIC__
    %define GLOBAL wrt rip
%else
    %define GLOBAL
%endif

%assign FENC_STRIDE 16
%assign FDEC_STRIDE 32

; This is needed for ELF, otherwise the GNU linker assumes the stack is
; executable by default.
%ifidn __YASM_OBJFMT__,elf
section ".note.GNU-stack" noalloc noexec nowrite progbits
%endif

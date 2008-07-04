;*****************************************************************************
;* x86inc-64.asm: h264 encoder library
;*****************************************************************************
;* Copyright (C) 2005-2008 Loren Merritt <lorenm@u.washington.edu>
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

BITS 64

; FIXME: All of the 64bit asm functions that take a stride as an argument
; via register, assume that the high dword of that register is filled with 0.
; This is true in practice (since we never do any 64bit arithmetic on strides,
; and x264's strides are all positive), but is not guaranteed by the ABI.

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
    %define PIC64
%else
    %define GLOBAL
%endif

%macro picgetgot 1
%endmacro

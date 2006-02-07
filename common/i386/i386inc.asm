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

%macro cglobal 1
    %ifdef PREFIX
        global _%1
        %define %1 _%1
    %else
        global %1
    %endif
%endmacro

%ifdef __PIC__
    extern _GLOBAL_OFFSET_TABLE_
    %define GLOBAL wrt ..gotpc
    %macro GET_GOT_IN_EBX_IF_PIC 0 
        call %%getgot 
      %%getgot: 
        pop ebx 
        add ebx, _GLOBAL_OFFSET_TABLE_ + $$ - %%getgot wrt ..gotpc 
    %endmacro
    %macro PUSH_EBX_IF_PIC 0
        push ebx
    %endmacro
    %macro POP_EBX_IF_PIC 0
        pop ebx
    %endmacro
%else
    %define GLOBAL
    %macro GET_GOT_IN_EBX_IF_PIC 0 
    %endmacro
    %macro PUSH_EBX_IF_PIC 0
    %endmacro
    %macro POP_EBX_IF_PIC 0
    %endmacro
%endif


;*****************************************************************************
;* cpu-64.asm: h264 encoder library
;*****************************************************************************
;* Copyright (C) 2003-2008 x264 project
;*
;* Authors: Laurent Aimar <fenrir@via.ecp.fr>
;*          Loren Merritt <lorenm@u.washington.edu>
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

SECTION .text

;-----------------------------------------------------------------------------
; int x264_cpu_cpuid( int op, int *eax, int *ebx, int *ecx, int *edx )
;-----------------------------------------------------------------------------
cglobal x264_cpu_cpuid
    push    rbx
    mov     r10,   r3
    mov     r11,   r2
    mov     r9,    r1
    mov     eax,   r0d
    cpuid
    mov     [r9],  eax
    mov     [r11], ebx
    mov     [r10], ecx
    mov     [r8],  edx
    pop     rbx
    ret

;-----------------------------------------------------------------------------
; void x264_emms( void )
;-----------------------------------------------------------------------------
cglobal x264_emms
    emms
    ret


;*****************************************************************************
;* cpu.asm: h264 encoder library
;*****************************************************************************
;* Copyright (C) 2003 x264 project
;* $Id: cpu.asm,v 1.1 2004/06/03 19:27:07 fenrir Exp $
;*
;* Authors: Laurent Aimar <fenrir@via.ecp.fr>
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

%include "i386inc.asm"

;=============================================================================
; Code
;=============================================================================

SECTION .text

cglobal x264_cpu_cpuid_test
cglobal x264_cpu_cpuid
cglobal x264_emms

ALIGN 16
;-----------------------------------------------------------------------------
;   int __cdecl x264_cpu_cpuid_test( void ) return 0 if unsupported
;-----------------------------------------------------------------------------
x264_cpu_cpuid_test:
    pushfd
    push    ebx
    push    ebp
    push    esi
    push    edi

    pushfd
    pop     eax
    mov     ebx, eax
    xor     eax, 0x200000
    push    eax
    popfd
    pushfd
    pop     eax
    xor     eax, ebx
    
    pop     edi
    pop     esi
    pop     ebp
    pop     ebx
    popfd
    ret

ALIGN 16
;-----------------------------------------------------------------------------
;   int __cdecl x264_cpu_cpuid( int op, int *eax, int *ebx, int *ecx, int *edx )
;-----------------------------------------------------------------------------
x264_cpu_cpuid:

    push    ebp
    mov     ebp,    esp
    push    ebx
    push    esi
    push    edi
    
    mov     eax,    [ebp +  8]
    cpuid

    mov     esi,    [ebp + 12]
    mov     [esi],  eax

    mov     esi,    [ebp + 16]
    mov     [esi],  ebx

    mov     esi,    [ebp + 20]
    mov     [esi],  ecx

    mov     esi,    [ebp + 24]
    mov     [esi],  edx

    pop     edi
    pop     esi
    pop     ebx
    pop     ebp
    ret

ALIGN 16
;-----------------------------------------------------------------------------
;   void __cdecl x264_emms( void )
;-----------------------------------------------------------------------------
x264_emms:
    emms
    ret


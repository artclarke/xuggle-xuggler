;*****************************************************************************
;* mc.asm: h264 encoder library
;*****************************************************************************
;* Copyright (C) 2003 x264 project
;* $Id: mc.asm,v 1.2 2004/06/17 09:01:19 chenm001 Exp $
;*
;* Authors: Min Chen <chenm001.163.com> (converted to nasm)
;*          Laurent Aimar <fenrir@via.ecp.fr> (init algorithm)
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

;*****************************************************************************
;*                                                                           *
;*  Revision history:                                                        *
;*                                                                           *
;*  2004.05.17 portab mc_copy_w4/8/16 (CM)                                   *
;*                                                                           *
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

;=============================================================================
; Local Data (Read Only)
;=============================================================================

%ifdef FORMAT_COFF
SECTION .rodata data
%else
SECTION .rodata data align=16
%endif

;-----------------------------------------------------------------------------
; Various memory constants (trigonometric values or rounding values)
;-----------------------------------------------------------------------------

ALIGN 16

;=============================================================================
; Code
;=============================================================================

SECTION .text

cglobal pixel_avg_w4

ALIGN 16
;-----------------------------------------------------------------------------
; void pixel_avg_w4( uint8_t *dst,  int i_dst_stride,
;                    uint8_t *src1, int i_src1_stride,
;                    uint8_t *src2, int i_src2_stride,
;                    int i_height );
;-----------------------------------------------------------------------------
pixel_avg_w4:
    push        ebp
    push        ebx
    push        esi
    push        edi

    mov         edi, [esp+20]       ; dst
    mov         ebx, [esp+28]       ; src1
    mov         ecx, [esp+36]       ; src2
    mov         esi, [esp+24]       ; i_dst_stride
    mov         eax, [esp+32]       ; i_src1_stride
    mov         edx, [esp+40]       ; i_src2_stride
    mov         ebp, [esp+44]       ; i_height
ALIGN 4
.height_loop    
    movd        mm0, [ebx]
    pavgb       mm0, [ecx]
    movd        mm1, [ebx+eax]
    pavgb       mm1, [ecx+edx]
    movd        [edi], mm0
    movd        [edi+esi], mm1
    dec         ebp
    dec         ebp
    lea         ebx, [ebx+eax*2]
    lea         ecx, [ecx+edx*2]
    lea         edi, [edi+esi*2]
    jne         .height_loop

    pop         edi
    pop         esi
    pop         ebx
    pop         ebp
    ret

                          
cglobal mc_copy_w4

ALIGN 16
;-----------------------------------------------------------------------------
;   void mc_copy_w4( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_height )
;-----------------------------------------------------------------------------
mc_copy_w4:
    push    ebx
    push    esi
    push    edi

    mov     esi, [esp+16]       ; src
    mov     edi, [esp+24]       ; dst
    mov     ebx, [esp+20]       ; i_src_stride
    mov     edx, [esp+28]       ; i_dst_stride
    mov     ecx, [esp+32]       ; i_height
ALIGN 4
.height_loop
    mov     eax, [esi]
    mov     [edi], eax
    mov     eax, [esi+ebx]
    mov     [edi+edx], eax
    lea     esi, [esi+ebx*2]
    lea     edi, [edi+edx*2]
    dec     ecx
    dec     ecx
    jne     .height_loop

    pop     edi
    pop     esi
    pop     ebx
    ret

cglobal mc_copy_w8

ALIGN 16
;-----------------------------------------------------------------------------
;   void mc_copy_w8( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_height )
;-----------------------------------------------------------------------------
mc_copy_w8:
    push    ebx
    push    esi
    push    edi

    mov     esi, [esp+16]       ; src
    mov     edi, [esp+24]       ; dst
    mov     ebx, [esp+20]       ; i_src_stride
    mov     edx, [esp+28]       ; i_dst_stride
    mov     ecx, [esp+32]       ; i_height
ALIGN 4
.height_loop
    movq    mm0, [esi]
    movq    [edi], mm0
    movq    mm1, [esi+ebx]
    movq    [edi+edx], mm1
    movq    mm2, [esi+ebx*2]
    movq    [edi+edx*2], mm2
    lea     esi, [esi+ebx*2]
    lea     edi, [edi+edx*2]
    movq    mm3, [esi+ebx]
    movq    [edi+edx], mm3
    lea     esi, [esi+ebx*2]
    lea     edi, [edi+edx*2]
    
    sub     ecx, byte 4
    jnz     .height_loop

    pop     edi
    pop     esi
    pop     ebx
    ret

cglobal mc_copy_w16

ALIGN 16
;-----------------------------------------------------------------------------
;   void mc_copy_w16( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_height )
;-----------------------------------------------------------------------------
mc_copy_w16:
    push    ebx
    push    esi
    push    edi

    mov     esi, [esp+16]       ; src
    mov     edi, [esp+24]       ; dst
    mov     ebx, [esp+20]       ; i_src_stride
    mov     edx, [esp+28]       ; i_dst_stride
    mov     ecx, [esp+32]       ; i_height
ALIGN 4
.height_loop
    movq    mm0, [esi]
    movq    mm1, [esi+8]
    movq    [edi], mm0
    movq    [edi+8], mm1
    movq    mm2, [esi+ebx]
    movq    mm3, [esi+ebx+8]
    movq    [edi+edx], mm2
    movq    [edi+edx+8], mm3
    movq    mm4, [esi+ebx*2]
    movq    mm5, [esi+ebx*2+8]
    movq    [edi+edx*2], mm4
    movq    [edi+edx*2+8], mm5
    lea     esi, [esi+ebx*2]
    lea     edi, [edi+edx*2]
    movq    mm6, [esi+ebx]
    movq    mm7, [esi+ebx+8]
    movq    [edi+edx], mm6
    movq    [edi+edx+8], mm7
    lea     esi, [esi+ebx*2]
    lea     edi, [edi+edx*2]
    
    sub     ecx, byte 4
    jnz     .height_loop

    pop     edi
    pop     esi
    pop     ebx
    ret

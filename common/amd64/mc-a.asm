;*****************************************************************************
;* mc.asm: h264 encoder library
;*****************************************************************************
;* Copyright (C) 2003 x264 project
;* $Id: mc.asm,v 1.3 2004/06/18 01:59:58 chenm001 Exp $
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

BITS 64

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

;-----------------------------------------------------------------------------
; Various memory constants (trigonometric values or rounding values)
;-----------------------------------------------------------------------------

ALIGN 16

;=============================================================================
; Code
;=============================================================================

SECTION .text

cglobal x264_pixel_avg_w4_mmxext
cglobal x264_pixel_avg_w8_mmxext
cglobal x264_pixel_avg_w16_mmxext
cglobal x264_pixel_avg_w16_sse2

cglobal x264_mc_copy_w4_mmxext
cglobal x264_mc_copy_w8_mmxext
cglobal x264_mc_copy_w16_mmxext
cglobal x264_mc_copy_w16_sse2

cglobal x264_mc_chroma_sse


ALIGN 16
;-----------------------------------------------------------------------------
; void x264_pixel_avg_w4_mmxext( uint8_t *dst,  int i_dst_stride,
;                                uint8_t *src1, int i_src1_stride,
;                                uint8_t *src2, int i_src2_stride,
;                                int i_height );
;-----------------------------------------------------------------------------
x264_pixel_avg_w4_mmxext:
    push        rbp
    mov         rbp, rsp
    push        r12
    push        r13

    mov         r12, r8             ; src2
    movsxd      r13, r9d            ; i_src2_stride
    mov         r10, rdx            ; src1
    movsxd      r11, ecx            ; i_src1_stride
    mov         r8, rdi             ; dst
    movsxd      r9, esi             ; i_dst_stride
    movsxd      rax, dword [rbp+16] ; i_height

ALIGN 4
.height_loop    
    movd        mm0, [r10]
    pavgb       mm0, [r12]
    movd        mm1, [r10+r11]
    pavgb       mm1, [r12+r13]
    movd        [r8], mm0
    movd        [r8+r9], mm1
    dec         rax
    dec         rax
    lea         r10, [r10+r11*2]
    lea         r12, [r12+r13*2]
    lea         r8, [r8+r9*2]
    jne         .height_loop

    pop         r13
    pop         r12
    pop         rbp
    ret

                          

ALIGN 16
;-----------------------------------------------------------------------------
; void x264_pixel_avg_w8_mmxext( uint8_t *dst,  int i_dst_stride,
;                                uint8_t *src1, int i_src1_stride,
;                                uint8_t *src2, int i_src2_stride,
;                                int i_height );
;-----------------------------------------------------------------------------
x264_pixel_avg_w8_mmxext:
    push        rbp
    mov         rbp, rsp
    push        r12
    push        r13

    mov         r12, r8             ; src2
    movsxd      r13, r9d            ; i_src2_stride
    mov         r10, rdx            ; src1
    movsxd      r11, ecx            ; i_src1_stride
    mov         r8, rdi             ; dst
    movsxd      r9, esi             ; i_dst_stride
    movsxd      rax, dword [rbp+16] ; i_height

ALIGN 4
.height_loop    
    movq        mm0, [r10]
    pavgb       mm0, [r12]
    movq        [r8], mm0
    dec         rax
    lea         r10, [r10+r11]
    lea         r12, [r12+r13]
    lea         r8, [r8+r9]
    jne         .height_loop

    pop         r13
    pop         r12
    pop         rbp
    ret



ALIGN 16
;-----------------------------------------------------------------------------
; void x264_pixel_avg_w16_mmxext( uint8_t *dst,  int i_dst_stride,
;                                 uint8_t *src1, int i_src1_stride,
;                                 uint8_t *src2, int i_src2_stride,
;                                 int i_height );
;-----------------------------------------------------------------------------
x264_pixel_avg_w16_mmxext:
    push        rbp
    mov         rbp, rsp
    push        r12
    push        r13

    mov         r12, r8             ; src2
    movsxd      r13, r9d            ; i_src2_stride
    mov         r10, rdx            ; src1
    movsxd      r11, ecx            ; i_src1_stride
    mov         r8, rdi             ; dst
    movsxd      r9, esi             ; i_dst_stride
    movsxd      rax, dword [rbp+16] ; i_height

ALIGN 4
.height_loop    
    movq        mm0, [r10  ]
    movq        mm1, [r10+8]
    pavgb       mm0, [r12  ]
    pavgb       mm1, [r12+8]
    movq        [r8  ], mm0
    movq        [r8+8], mm1
    dec         rax
    lea         r10, [r10+r11]
    lea         r12, [r12+r13]
    lea         r8, [r8+r9]
    jne         .height_loop

    pop         r13
    pop         r12
    pop         rbp
    ret

ALIGN 16
;-----------------------------------------------------------------------------
; void x264_pixel_avg_w16_sse2( uint8_t *dst,  int i_dst_stride,
;                               uint8_t *src1, int i_src1_stride,
;                               uint8_t *src2, int i_src2_stride,
;                               int i_height );
;-----------------------------------------------------------------------------
x264_pixel_avg_w16_sse2:
    push        rbp
    mov         rbp, rsp
    push        r12
    push        r13

    mov         r12, r8             ; src2
    movsxd      r13, r9d            ; i_src2_stride
    mov         r10, rdx            ; src1
    movsxd      r11, ecx            ; i_src1_stride
    mov         r8, rdi             ; dst
    movsxd      r9, esi             ; i_dst_stride
    movsxd      rax, dword [rbp+16] ; i_height

ALIGN 4
.height_loop    
    movdqu      xmm0, [r10]
    pavgb       xmm0, [r12]
    movdqu      [r8], xmm0

    dec         rax
    lea         r10, [r10+r11]
    lea         r12, [r12+r13]
    lea         r8, [r8+r9]
    jne         .height_loop

    pop         r13
    pop         r12
    pop         rbp
    ret



ALIGN 16
;-----------------------------------------------------------------------------
;  void x264_mc_copy_w4_mmxext( uint8_t *src, int i_src_stride,
;                               uint8_t *dst, int i_dst_stride, int i_height )
;-----------------------------------------------------------------------------
x264_mc_copy_w4_mmxext:
    mov     eax, r8d            ; i_height
    mov     r8, rdi             ; src
    movsxd  r9, esi             ; i_src_stride
    mov     r10, rdx            ; dst
    movsxd  r11, ecx            ; i_dst_stride
    
ALIGN 4
.height_loop
    mov     ecx, [r8]
    mov     edx, [r8+r9]
    mov     [r10], ecx
    mov     [r10+r11], edx
    lea     r8, [r8+r9*2]
    lea     r10, [r10+r11*2]
    dec     eax
    dec     eax
    jne     .height_loop

    ret

cglobal mc_copy_w8

ALIGN 16
;-----------------------------------------------------------------------------
;   void x264_mc_copy_w8_mmxext( uint8_t *src, int i_src_stride,
;                                uint8_t *dst, int i_dst_stride, int i_height )
;-----------------------------------------------------------------------------
x264_mc_copy_w8_mmxext:
    mov     eax, r8d            ; i_height
    mov     r8, rdi             ; src
    movsxd  r9, esi             ; i_src_stride
    mov     r10, rdx            ; dst
    movsxd  r11, ecx            ; i_dst_stride

    lea     rcx, [r9+r9*2]      ; 3 * i_src_stride
    lea     rdx, [r11+r11*2]    ; 3 * i_dst_stride

ALIGN 4
.height_loop
    movq    mm0, [r8]
    movq    mm1, [r8+r9]
    movq    mm2, [r8+r9*2]
    movq    mm3, [r8+rcx]
    movq    [r10], mm0
    movq    [r10+r11], mm1
    movq    [r10+r11*2], mm2
    movq    [r10+rdx], mm3
    lea     r8, [r8+r9*4]
    lea     r10, [r10+r11*4]
    
    sub     eax, byte 4
    jnz     .height_loop

    ret

cglobal mc_copy_w16

ALIGN 16
;-----------------------------------------------------------------------------
;   void x264_mc_copy_w16_mmxext( uint8_t *src, int i_src_stride,
;                                 uint8_t *dst, int i_dst_stride, int i_height )
;-----------------------------------------------------------------------------
x264_mc_copy_w16_mmxext:
    mov     eax, r8d            ; i_height
    mov     r8, rdi             ; src
    movsxd  r9, esi             ; i_src_stride
    mov     r10, rdx            ; dst
    movsxd  r11, ecx            ; i_dst_stride
    
    lea     rcx, [r9+r9*2]      ; 3 * i_src_stride
    lea     rdx, [r11+r11*2]    ; 3 * i_dst_stride

ALIGN 4
.height_loop
    movq    mm0, [r8]
    movq    mm1, [r8+8]
    movq    mm2, [r8+r9]
    movq    mm3, [r8+r9+8]
    movq    mm4, [r8+r9*2]
    movq    mm5, [r8+r9*2+8]
    movq    mm6, [r8+rcx]
    movq    mm7, [r8+rcx+8]
    movq    [r10], mm0
    movq    [r10+8], mm1
    movq    [r10+r11], mm2
    movq    [r10+r11+8], mm3
    movq    [r10+r11*2], mm4
    movq    [r10+r11*2+8], mm5
    movq    [r10+rdx], mm6
    movq    [r10+rdx+8], mm7
    lea     r8, [r8+r9*4]
    lea     r10, [r10+r11*4]
    sub     eax, byte 4
    jnz     .height_loop
    
    ret


ALIGN 16
;-----------------------------------------------------------------------------
;   void x264_mc_copy_w16_sse2( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_height )
;-----------------------------------------------------------------------------
x264_mc_copy_w16_sse2:
    mov     eax, r8d            ; i_height
    mov     r8, rdi             ; src
    movsxd  r9, esi             ; i_src_stride
    mov     r10, rdx            ; dst
    movsxd  r11, ecx            ; i_dst_stride

ALIGN 4
.height_loop
    movdqu  xmm0, [r8]
    movdqu  xmm1, [r8+r9]
    movdqu  [r10], xmm0
    movdqu  [r10+r11], xmm1
    dec     eax
    dec     eax
    lea     r8, [r8+r9*2]
    lea     r10, [r10+r11*2]
    jnz     .height_loop
    
    ret


SECTION .rodata

ALIGN 16
eights    times 4   dw 8
thirty2s  times 4   dw 32

SECTION .data
x264_mc_chroma_sse_dx:
    dw  0
x264_mc_chroma_sse_dy:
    dw  0

SECTION .text

ALIGN 16
;-----------------------------------------------------------------------------
;   void x264_mc_chroma_sse( uint8_t *src, int i_src_stride,
;                               uint8_t *dst, int i_dst_stride,
;                               int dx, int dy,
;                               int i_height, int i_width )
;-----------------------------------------------------------------------------

x264_mc_chroma_sse:
    push    r12
    push    r13

    mov     [x264_mc_chroma_sse_dx], r8d
    mov     [x264_mc_chroma_sse_dy], r9d

    pxor    mm3, mm3

    pshufw  mm5, [x264_mc_chroma_sse_dx], 0    ; mm5 - dx
    pshufw  mm6, [x264_mc_chroma_sse_dy], 0    ; mm6 - dy

    movq    mm4, [eights]
    movq    mm0, mm4

    psubw   mm4, mm5            ; mm4 - 8-dx
    psubw   mm0, mm6            ; mm0 - 8-dy

    movq    mm7, mm5
    pmullw  mm5, mm0            ; mm5 = dx*(8-dy) =     cB
    pmullw  mm7, mm6            ; mm7 = dx*dy =         cD
    pmullw  mm6, mm4            ; mm6 = (8-dx)*dy =     cC
    pmullw  mm4, mm0            ; mm4 = (8-dx)*(8-dy) = cA

    mov     r8, rdi             ; src
    movsxd  r9, esi             ; i_src_stride
    mov     r10, rdx            ; dst
    movsxd  r11, ecx            ; i_dst_stride
    movsxd  r12, dword [rsp+24] ; i_height
    movsxd  r13, dword [rsp+32] ; i_width

    mov     rax, r8
    mov     rdi, r10
    mov     rcx, r9
    mov     rdx, r12

ALIGN 4
.height_loop

    movd    mm1, [rax+rcx]
    movd    mm0, [rax]
    punpcklbw mm1, mm3          ; 00 px1 | 00 px2 | 00 px3 | 00 px4
    punpcklbw mm0, mm3
    pmullw  mm1, mm6            ; 2nd line * cC
    pmullw  mm0, mm4            ; 1st line * cA

    paddw   mm0, mm1            ; mm0 <- result

    movd    mm2, [rax+1]
    movd    mm1, [rax+rcx+1]
    punpcklbw mm2, mm3
    punpcklbw mm1, mm3

    paddw   mm0, [thirty2s]

    pmullw  mm2, mm5            ; line * cB
    pmullw  mm1, mm7            ; line * cD
    paddw   mm0, mm2
    paddw   mm0, mm1

    psrlw   mm0, 6
    packuswb mm0, mm3           ; 00 00 00 00 px1 px2 px3 px4
    movd    [rdi], mm0

    add     rax, rcx
    add     rdi, r11            ; i_dst_stride

    dec     rdx
    jnz     .height_loop

    mov     rax, r13            ; i_width
    sub     rax, 8
    jnz     .finish             ; width != 8 so assume 4

    mov     r13, rax            ; i_width
    mov     rdi, r10            ; dst
    mov     rax, r8             ; src
    mov     rdx, r12            ; i_height
    add     rdi, 4
    add     rax, 4
    jmp    .height_loop

.finish
    pop     r13
    pop     r12
    ret

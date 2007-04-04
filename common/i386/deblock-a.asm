;*****************************************************************************
;* deblock-a.asm: h264 encoder library
;*****************************************************************************
;* Copyright (C) 2005 x264 project
;*
;* Authors: Loren Merritt <lorenm@u.washington.edu>
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

%include "i386inc.asm"

SECTION_RODATA
pb_01: times 8 db 0x01
pb_03: times 8 db 0x03
pb_a1: times 8 db 0xa1

SECTION .text

; expands to [base],...,[base+7*stride]
%define PASS8ROWS(base, base3, stride, stride3) \
    [base], [base+stride], [base+stride*2], [base3], \
    [base3+stride], [base3+stride*2], [base3+stride3], [base3+stride*4]

; in: 8 rows of 4 bytes in %1..%8
; out: 4 rows of 8 bytes in mm0..mm3
%macro TRANSPOSE4x8_LOAD 8
    movd       mm0, %1
    movd       mm2, %2
    movd       mm1, %3
    movd       mm3, %4
    punpcklbw  mm0, mm2
    punpcklbw  mm1, mm3
    movq       mm2, mm0
    punpcklwd  mm0, mm1
    punpckhwd  mm2, mm1

    movd       mm4, %5
    movd       mm6, %6
    movd       mm5, %7
    movd       mm7, %8
    punpcklbw  mm4, mm6
    punpcklbw  mm5, mm7
    movq       mm6, mm4
    punpcklwd  mm4, mm5
    punpckhwd  mm6, mm5

    movq       mm1, mm0
    movq       mm3, mm2
    punpckldq  mm0, mm4
    punpckhdq  mm1, mm4
    punpckldq  mm2, mm6
    punpckhdq  mm3, mm6
%endmacro

; in: 4 rows of 8 bytes in mm0..mm3
; out: 8 rows of 4 bytes in %1..%8
%macro TRANSPOSE8x4_STORE 8
    movq       mm4, mm0
    movq       mm5, mm1
    movq       mm6, mm2
    punpckhdq  mm4, mm4
    punpckhdq  mm5, mm5
    punpckhdq  mm6, mm6

    punpcklbw  mm0, mm1
    punpcklbw  mm2, mm3
    movq       mm1, mm0
    punpcklwd  mm0, mm2
    punpckhwd  mm1, mm2
    movd       %1,  mm0
    punpckhdq  mm0, mm0
    movd       %2,  mm0
    movd       %3,  mm1
    punpckhdq  mm1, mm1
    movd       %4,  mm1

    punpckhdq  mm3, mm3
    punpcklbw  mm4, mm5
    punpcklbw  mm6, mm3
    movq       mm5, mm4
    punpcklwd  mm4, mm6
    punpckhwd  mm5, mm6
    movd       %5,  mm4
    punpckhdq  mm4, mm4
    movd       %6,  mm4
    movd       %7,  mm5
    punpckhdq  mm5, mm5
    movd       %8,  mm5
%endmacro

%macro SBUTTERFLY 4
    movq       %4, %2
    punpckl%1  %2, %3
    punpckh%1  %4, %3
%endmacro

; in: 8 rows of 8 (only the middle 6 pels are used) in %1..%8
; out: 6 rows of 8 in [%9+0*16] .. [%9+5*16]
%macro TRANSPOSE6x8_MEM 9
    movq  mm0, %1
    movq  mm1, %3
    movq  mm2, %5
    movq  mm3, %7
    SBUTTERFLY bw, mm0, %2, mm4
    SBUTTERFLY bw, mm1, %4, mm5
    SBUTTERFLY bw, mm2, %6, mm6
    movq  [%9+0x10], mm5
    SBUTTERFLY bw, mm3, %8, mm7
    SBUTTERFLY wd, mm0, mm1, mm5
    SBUTTERFLY wd, mm2, mm3, mm1
    punpckhdq mm0, mm2
    movq  [%9+0x00], mm0
    SBUTTERFLY wd, mm4, [%9+0x10], mm3
    SBUTTERFLY wd, mm6, mm7, mm2
    SBUTTERFLY dq, mm4, mm6, mm0
    SBUTTERFLY dq, mm5, mm1, mm7
    punpckldq mm3, mm2
    movq  [%9+0x10], mm5
    movq  [%9+0x20], mm7
    movq  [%9+0x30], mm4
    movq  [%9+0x40], mm0
    movq  [%9+0x50], mm3
%endmacro

; out: %4 = |%1-%2|>%3
; clobbers: %5
%macro DIFF_GT_MMX 5
    movq    %5, %2
    movq    %4, %1
    psubusb %5, %1
    psubusb %4, %2
    por     %4, %5
    psubusb %4, %3
%endmacro

%macro DIFF_GT2_MMX 5
    movq    %5, %2
    movq    %4, %1
    psubusb %5, %1
    psubusb %4, %2
    psubusb %5, %3
    psubusb %4, %3
    pcmpeqb %4, %5
%endmacro

; in: mm0=p1 mm1=p0 mm2=q0 mm3=q1 %1=alpha-1 %2=beta-1
; out: mm5=beta-1, mm7=mask
; clobbers: mm4,mm6
%macro LOAD_MASK_MMX 2
    movd     mm4, %1
    movd     mm5, %2
    pshufw   mm4, mm4, 0
    pshufw   mm5, mm5, 0
    packuswb mm4, mm4  ; 8x alpha-1
    packuswb mm5, mm5  ; 8x beta-1
    DIFF_GT_MMX  mm1, mm2, mm4, mm7, mm6 ; |p0-q0| > alpha-1
    DIFF_GT_MMX  mm0, mm1, mm5, mm4, mm6 ; |p1-p0| > beta-1
    por      mm7, mm4
    DIFF_GT_MMX  mm3, mm2, mm5, mm4, mm6 ; |q1-q0| > beta-1
    por      mm7, mm4
    pxor     mm6, mm6
    pcmpeqb  mm7, mm6
%endmacro

; in: mm0=p1 mm1=p0 mm2=q0 mm3=q1 mm7=(tc&mask)
; out: mm1=p0' mm2=q0'
; clobbers: mm0,3-6
%macro DEBLOCK_P0_Q0_MMX 0
    movq    mm5, mm1
    pxor    mm5, mm2             ; p0^q0
    pand    mm5, [pb_01 GOT_ebx] ; (p0^q0)&1
    pcmpeqb mm4, mm4
    pxor    mm3, mm4
    pavgb   mm3, mm0             ; (p1 - q1 + 256)>>1
    pavgb   mm3, [pb_03 GOT_ebx] ; (((p1 - q1 + 256)>>1)+4)>>1 = 64+2+(p1-q1)>>2
    pxor    mm4, mm1
    pavgb   mm4, mm2             ; (q0 - p0 + 256)>>1
    pavgb   mm3, mm5
    paddusb mm3, mm4             ; d+128+33
    movq    mm6, [pb_a1 GOT_ebx]
    psubusb mm6, mm3
    psubusb mm3, [pb_a1 GOT_ebx]
    pminub  mm6, mm7
    pminub  mm3, mm7
    psubusb mm1, mm6
    psubusb mm2, mm3
    paddusb mm1, mm3
    paddusb mm2, mm6
%endmacro

; in: mm1=p0 mm2=q0
;     %1=p1 %2=q2 %3=[q2] %4=[q1] %5=tc0 %6=tmp
; out: [q1] = clip( (q2+((p0+q0+1)>>1))>>1, q1-tc0, q1+tc0 )
; clobbers: q2, tmp, tc0
%macro LUMA_Q1_MMX 6
    movq    %6, mm1
    pavgb   %6, mm2
    pavgb   %2, %6              ; avg(p2,avg(p0,q0))
    pxor    %6, %3
    pand    %6, [pb_01 GOT_ebx] ; (p2^avg(p0,q0))&1
    psubusb %2, %6              ; (p2+((p0+q0+1)>>1))>>1
    movq    %6, %1
    psubusb %6, %5
    paddusb %5, %1
    pmaxub  %2, %6
    pminub  %2, %5
    movq    %4, %2
%endmacro


SECTION .text

;-----------------------------------------------------------------------------
;   void x264_deblock_v8_luma_mmxext( uint8_t *pix, int stride, int alpha, int beta, int8_t *tc0 )
;-----------------------------------------------------------------------------
cglobal x264_deblock_v8_luma_mmxext
    picpush ebx
    picgetgot ebx
    push    edi
    push    esi
    mov     edi, [picesp+12] ; pix
    mov     esi, [picesp+16] ; stride
    mov     edx, [picesp+20] ; alpha
    mov     ecx, [picesp+24] ; beta
    dec     edx
    dec     ecx
    mov     eax, edi
    sub     eax, esi
    sub     eax, esi
    sub     eax, esi ; pix-3*stride
    sub     esp, 16

    movq    mm0, [eax+esi]   ; p1
    movq    mm1, [eax+2*esi] ; p0
    movq    mm2, [edi]       ; q0
    movq    mm3, [edi+esi]   ; q1
    LOAD_MASK_MMX  edx, ecx

    mov     ecx, [picesp+44] ; tc0, use only the low 16 bits
    movd    mm4, [ecx]
    punpcklbw mm4, mm4
    punpcklbw mm4, mm4 ; tc = 4x tc0[1], 4x tc0[0]
    movq   [esp+8], mm4 ; tc
    pcmpeqb mm3, mm3
    pcmpgtb mm4, mm3
    pand    mm4, mm7
    movq   [esp+0], mm4 ; mask

    movq    mm3, [eax] ; p2
    DIFF_GT2_MMX  mm1, mm3, mm5, mm6, mm7 ; |p2-p0| > beta-1
    pand    mm6, mm4
    pand    mm4, [esp+8] ; tc
    movq    mm7, mm4
    psubb   mm7, mm6
    pand    mm6, mm4
    LUMA_Q1_MMX  mm0, mm3, [eax], [eax+esi], mm6, mm4

    movq    mm4, [edi+2*esi] ; q2
    DIFF_GT2_MMX  mm2, mm4, mm5, mm6, mm3 ; |q2-q0| > beta-1
    movq    mm5, [esp+0] ; mask
    pand    mm6, mm5
    movq    mm5, [esp+8] ; tc
    pand    mm5, mm6
    psubb   mm7, mm6
    movq    mm3, [edi+esi]
    LUMA_Q1_MMX  mm3, mm4, [edi+2*esi], [edi+esi], mm5, mm6

    DEBLOCK_P0_Q0_MMX ; XXX: make sure ebx has the GOT in PIC mode
    movq    [eax+2*esi], mm1
    movq    [edi], mm2

    add     esp, 16
    pop     esi
    pop     edi
    picpop  ebx
    ret


;-----------------------------------------------------------------------------
;   void x264_deblock_h_luma_mmxext( uint8_t *pix, int stride, int alpha, int beta, int8_t *tc0 )
;-----------------------------------------------------------------------------
cglobal x264_deblock_h_luma_mmxext
    push   ebx
    push   ebp
    mov    eax, [esp+12] ; pix
    mov    ebx, [esp+16] ; stride
    lea    ebp, [ebx+ebx*2]
    sub    eax, 4
    lea    ecx, [eax+ebp]
    sub    esp, 96
%define pix_tmp esp

    ; transpose 6x16 -> tmp space
    TRANSPOSE6x8_MEM  PASS8ROWS(eax, ecx, ebx, ebp), pix_tmp
    lea    eax, [eax+ebx*8]
    lea    ecx, [ecx+ebx*8]
    TRANSPOSE6x8_MEM  PASS8ROWS(eax, ecx, ebx, ebp), pix_tmp+8

    ; vertical filter
    push   dword [esp+124] ; tc0
    push   dword [esp+124] ; beta
    push   dword [esp+124] ; alpha
    push   dword 16
    push   dword pix_tmp
    add    dword [esp], 0x40 ; pix_tmp+0x30
    call   x264_deblock_v8_luma_mmxext

    add    dword [esp   ], 8 ; pix_tmp+0x38
    add    dword [esp+16], 2 ; tc0+2
    call   x264_deblock_v8_luma_mmxext
    add    esp, 20
    
    ; transpose 16x4 -> original space  (only the middle 4 rows were changed by the filter)
    mov    eax, [esp+108] ; pix
    sub    eax, 2
    lea    ecx, [eax+ebp]

    movq   mm0, [pix_tmp+0x10]
    movq   mm1, [pix_tmp+0x20]
    movq   mm2, [pix_tmp+0x30]
    movq   mm3, [pix_tmp+0x40]
    TRANSPOSE8x4_STORE  PASS8ROWS(eax, ecx, ebx, ebp)

    lea    eax, [eax+ebx*8]
    lea    ecx, [ecx+ebx*8]
    movq   mm0, [pix_tmp+0x18]
    movq   mm1, [pix_tmp+0x28]
    movq   mm2, [pix_tmp+0x38]
    movq   mm3, [pix_tmp+0x48]
    TRANSPOSE8x4_STORE  PASS8ROWS(eax, ecx, ebx, ebp)

    add    esp, 96
    pop    ebp
    pop    ebx
    ret


%macro CHROMA_V_START 0
    push  edi
    push  esi
    mov   edi, [esp+12] ; pix
    mov   esi, [esp+16] ; stride
    mov   edx, [esp+20] ; alpha
    mov   ecx, [esp+24] ; beta
    dec   edx
    dec   ecx
    mov   eax, edi
    sub   eax, esi
    sub   eax, esi
%endmacro

%macro CHROMA_H_START 0
    push  edi
    push  esi
    push  ebp
    mov   edi, [esp+16]
    mov   esi, [esp+20]
    mov   edx, [esp+24]
    mov   ecx, [esp+28]
    dec   edx
    dec   ecx
    sub   edi, 2
    mov   ebp, esi
    add   ebp, esi
    add   ebp, esi
    mov   eax, edi
    add   edi, ebp
%endmacro

%macro CHROMA_END 0
    pop  esi
    pop  edi
    ret
%endmacro

;-----------------------------------------------------------------------------
;   void x264_deblock_v_chroma_mmxext( uint8_t *pix, int stride, int alpha, int beta, int8_t *tc0 )
;-----------------------------------------------------------------------------
cglobal x264_deblock_v_chroma_mmxext
    CHROMA_V_START
    push  ebx
    mov   ebx, [esp+32] ; tc0

    movq  mm0, [eax]
    movq  mm1, [eax+esi]
    movq  mm2, [edi]
    movq  mm3, [edi+esi]

    LOAD_MASK_MMX  edx, ecx
    movd       mm6, [ebx]
    punpcklbw  mm6, mm6
    pand       mm7, mm6
    picgetgot  ebx ; no need to push ebx, it's already been done
    DEBLOCK_P0_Q0_MMX ; XXX: make sure ebx has the GOT in PIC mode

    movq  [eax+esi], mm1
    movq  [edi], mm2

    pop  ebx
    CHROMA_END


;-----------------------------------------------------------------------------
;   void x264_deblock_h_chroma_mmxext( uint8_t *pix, int stride, int alpha, int beta, int8_t *tc0 )
;-----------------------------------------------------------------------------
cglobal x264_deblock_h_chroma_mmxext
    CHROMA_H_START
    push  ebx
    mov   ebx, [esp+36] ; tc0
    sub   esp, 16

    TRANSPOSE4x8_LOAD  PASS8ROWS(eax, edi, esi, ebp)
    movq  [esp+8], mm0
    movq  [esp+0], mm3

    LOAD_MASK_MMX  edx, ecx
    movd       mm6, [ebx]
    punpcklbw  mm6, mm6
    pand       mm7, mm6
    picgetgot  ebx ; no need to push ebx, it's already been done
    DEBLOCK_P0_Q0_MMX ; XXX: make sure ebx has the GOT in PIC mode

    movq  mm0, [esp+8]
    movq  mm3, [esp+0]
    TRANSPOSE8x4_STORE PASS8ROWS(eax, edi, esi, ebp)

    add  esp, 16
    pop  ebx
    pop  ebp
    CHROMA_END


; in: %1=p0 %2=p1 %3=q1
; out: p0 = (p0 + q1 + 2*p1 + 2) >> 2
%macro CHROMA_INTRA_P0 3
    movq    mm4, %1
    pxor    mm4, %3
    pand    mm4, [pb_01 GOT_ebx]  ; mm4 = (p0^q1)&1
    pavgb   %1,  %3
    psubusb %1,  mm4
    pavgb   %1,  %2       ; dst = avg(p1, avg(p0,q1) - ((p0^q1)&1))
%endmacro

%macro CHROMA_INTRA_BODY 0
    LOAD_MASK_MMX edx, ecx
    movq   mm5, mm1
    movq   mm6, mm2
    CHROMA_INTRA_P0  mm1, mm0, mm3
    CHROMA_INTRA_P0  mm2, mm3, mm0
    psubb  mm1, mm5
    psubb  mm2, mm6
    pand   mm1, mm7
    pand   mm2, mm7
    paddb  mm1, mm5
    paddb  mm2, mm6
%endmacro

;-----------------------------------------------------------------------------
;   void x264_deblock_v_chroma_intra_mmxext( uint8_t *pix, int stride, int alpha, int beta )
;-----------------------------------------------------------------------------
cglobal x264_deblock_v_chroma_intra_mmxext
    CHROMA_V_START
    picpush ebx
    picgetgot ebx
    movq  mm0, [eax]
    movq  mm1, [eax+esi]
    movq  mm2, [edi]
    movq  mm3, [edi+esi]
    CHROMA_INTRA_BODY ; XXX: make sure ebx has the GOT in PIC mode
    movq  [eax+esi], mm1
    movq  [edi], mm2
    picpop ebx
    CHROMA_END

;-----------------------------------------------------------------------------
;   void x264_deblock_h_chroma_intra_mmxext( uint8_t *pix, int stride, int alpha, int beta )
;-----------------------------------------------------------------------------
cglobal x264_deblock_h_chroma_intra_mmxext
    CHROMA_H_START
    picpush ebx
    picgetgot ebx
    TRANSPOSE4x8_LOAD  PASS8ROWS(eax, edi, esi, ebp)
    CHROMA_INTRA_BODY ; XXX: make sure ebx has the GOT in PIC mode
    TRANSPOSE8x4_STORE PASS8ROWS(eax, edi, esi, ebp)
    picpop ebx
    pop  ebp ; needed because of CHROMA_H_START
    CHROMA_END


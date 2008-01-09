;*****************************************************************************
;* pixel-sse2.asm: h264 encoder library
;*****************************************************************************
;* Copyright (C) 2005 x264 project
;*
;* Authors: Alex Izvorski <aizvorksi@gmail.com>
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
;* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111, USA.
;*****************************************************************************

BITS 32

;=============================================================================
; Macros and other preprocessor constants
;=============================================================================

%include "i386inc.asm"

SECTION_RODATA

pw_1:    times 8 dw 1
ssim_c1: times 4 dd 416    ; .01*.01*255*255*64
ssim_c2: times 4 dd 235963 ; .03*.03*255*255*64*63
mask_ff: times 16 db 0xff
         times 16 db 0


SECTION .text

%macro HADDW 2    ; sum junk
    ; ebx is no longer used at this point, so no push needed
    picgetgot ebx
    pmaddwd %1, [pw_1 GOT_ebx]
    movhlps %2, %1
    paddd   %1, %2
    pshuflw %2, %1, 0xE 
    paddd   %1, %2
%endmacro

%macro SAD_START_SSE2 0
    push    ebx
    mov     eax,    [esp+ 8]    ; pix1
    mov     ebx,    [esp+12]    ; stride1
    mov     ecx,    [esp+16]    ; pix2
    mov     edx,    [esp+20]    ; stride2
%endmacro

%macro SAD_END_SSE2 0
    movhlps xmm1, xmm0
    paddw   xmm0, xmm1
    movd    eax,  xmm0
    pop ebx
    ret
%endmacro

%macro SAD_W16 1
;-----------------------------------------------------------------------------
;   int __cdecl x264_pixel_sad_16x16_sse2 (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
cglobal x264_pixel_sad_16x16_%1
    SAD_START_SSE2
    movdqu xmm0, [ecx]
    movdqu xmm1, [ecx+edx]
    lea    ecx,  [ecx+2*edx]
    movdqu xmm2, [ecx]
    movdqu xmm3, [ecx+edx]
    lea    ecx,  [ecx+2*edx]
    psadbw xmm0, [eax]
    psadbw xmm1, [eax+ebx]
    lea    eax,  [eax+2*ebx]
    movdqu xmm4, [ecx]
    paddw  xmm0, xmm1
    psadbw xmm2, [eax]
    psadbw xmm3, [eax+ebx]
    lea    eax,  [eax+2*ebx]
    movdqu xmm5, [ecx+edx]
    lea    ecx,  [ecx+2*edx]
    paddw  xmm2, xmm3
    movdqu xmm6, [ecx]
    movdqu xmm7, [ecx+edx]
    lea    ecx,  [ecx+2*edx]
    paddw  xmm0, xmm2
    psadbw xmm4, [eax]
    psadbw xmm5, [eax+ebx]
    lea    eax,  [eax+2*ebx]
    movdqu xmm1, [ecx]
    paddw  xmm4, xmm5
    psadbw xmm6, [eax]
    psadbw xmm7, [eax+ebx]
    lea    eax,  [eax+2*ebx]
    movdqu xmm2, [ecx+edx]
    lea    ecx,  [ecx+2*edx]
    paddw  xmm6, xmm7
    movdqu xmm3, [ecx]
    paddw  xmm0, xmm4
    movdqu xmm4, [ecx+edx]
    lea    ecx,  [ecx+2*edx]
    paddw  xmm0, xmm6
    psadbw xmm1, [eax]
    psadbw xmm2, [eax+ebx]
    lea    eax,  [eax+2*ebx]
    movdqu xmm5, [ecx]
    paddw  xmm1, xmm2
    psadbw xmm3, [eax]
    psadbw xmm4, [eax+ebx]
    lea    eax,  [eax+2*ebx]
    movdqu xmm6, [ecx+edx]
    lea    ecx,  [ecx+2*edx]
    paddw  xmm3, xmm4
    movdqu xmm7, [ecx]
    paddw  xmm0, xmm1
    movdqu xmm1, [ecx+edx]
    paddw  xmm0, xmm3
    psadbw xmm5, [eax]
    psadbw xmm6, [eax+ebx]
    lea    eax,  [eax+2*ebx]
    paddw  xmm5, xmm6
    psadbw xmm7, [eax]
    psadbw xmm1, [eax+ebx]
    paddw  xmm7, xmm1
    paddw  xmm0, xmm5
    paddw  xmm0, xmm7
    SAD_END_SSE2

;-----------------------------------------------------------------------------
;   int __cdecl x264_pixel_sad_16x8_sse2 (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
cglobal x264_pixel_sad_16x8_%1
    SAD_START_SSE2
    movdqu  xmm0,   [ecx]
    movdqu  xmm2,   [ecx+edx]
    lea     ecx,    [ecx+2*edx]
    movdqu  xmm3,   [ecx]
    movdqu  xmm4,   [ecx+edx]
    psadbw  xmm0,   [eax]
    psadbw  xmm2,   [eax+ebx]
    lea     eax,    [eax+2*ebx]
    psadbw  xmm3,   [eax]
    psadbw  xmm4,   [eax+ebx]
    lea     eax,    [eax+2*ebx]
    lea     ecx,    [ecx+2*edx]
    paddw   xmm0,   xmm2
    paddw   xmm3,   xmm4
    paddw   xmm0,   xmm3
    movdqu  xmm1,   [ecx]
    movdqu  xmm2,   [ecx+edx]
    lea     ecx,    [ecx+2*edx]
    movdqu  xmm3,   [ecx]
    movdqu  xmm4,   [ecx+edx]
    psadbw  xmm1,   [eax]
    psadbw  xmm2,   [eax+ebx]
    lea     eax,    [eax+2*ebx]
    psadbw  xmm3,   [eax]
    psadbw  xmm4,   [eax+ebx]
    lea     eax,    [eax+2*ebx]
    lea     ecx,    [ecx+2*edx]
    paddw   xmm1,   xmm2
    paddw   xmm3,   xmm4
    paddw   xmm0,   xmm1
    paddw   xmm0,   xmm3
    SAD_END_SSE2
%endmacro

SAD_W16 sse2
%ifdef HAVE_SSE3
%define movdqu lddqu
SAD_W16 sse3
%undef movdqu
%endif


; sad x3 / x4

%macro SAD_X3_START_1x16P 0
    push    edi
    push    esi
    mov     edi,    [esp+12]
    mov     eax,    [esp+16]
    mov     ecx,    [esp+20]
    mov     edx,    [esp+24]
    mov     esi,    [esp+28]
    movdqa  xmm3,   [edi]
    movdqu  xmm0,   [eax]
    movdqu  xmm1,   [ecx]
    movdqu  xmm2,   [edx]
    psadbw  xmm0,   xmm3
    psadbw  xmm1,   xmm3
    psadbw  xmm2,   xmm3
%endmacro

%macro SAD_X3_1x16P 2
    movdqa  xmm3,   [edi+%1]
    movdqu  xmm4,   [eax+%2]
    movdqu  xmm5,   [ecx+%2]
    movdqu  xmm6,   [edx+%2]
    psadbw  xmm4,   xmm3
    psadbw  xmm5,   xmm3
    psadbw  xmm6,   xmm3
    paddw   xmm0,   xmm4
    paddw   xmm1,   xmm5
    paddw   xmm2,   xmm6
%endmacro

%macro SAD_X3_2x16P 1
%if %1
    SAD_X3_START_1x16P
%else
    SAD_X3_1x16P 0, 0
%endif
    SAD_X3_1x16P FENC_STRIDE, esi
    add     edi, 2*FENC_STRIDE
    lea     eax, [eax+2*esi]
    lea     ecx, [ecx+2*esi]
    lea     edx, [edx+2*esi]
%endmacro

%macro SAD_X4_START_1x16P 0
    push    edi
    push    esi
    push    ebx
    mov     edi,    [esp+16]
    mov     eax,    [esp+20]
    mov     ebx,    [esp+24]
    mov     ecx,    [esp+28]
    mov     edx,    [esp+32]
    mov     esi,    [esp+36]
    movdqa  xmm7,   [edi]
    movdqu  xmm0,   [eax]
    movdqu  xmm1,   [ebx]
    movdqu  xmm2,   [ecx]
    movdqu  xmm3,   [edx]
    psadbw  xmm0,   xmm7
    psadbw  xmm1,   xmm7
    psadbw  xmm2,   xmm7
    psadbw  xmm3,   xmm7
%endmacro

%macro SAD_X4_1x16P 2
    movdqa  xmm7,   [edi+%1]
    movdqu  xmm4,   [eax+%2]
    movdqu  xmm5,   [ebx+%2]
    movdqu  xmm6,   [ecx+%2]
    psadbw  xmm4,   xmm7
    psadbw  xmm5,   xmm7
    paddw   xmm0,   xmm4
    psadbw  xmm6,   xmm7
    movdqu  xmm4,   [edx+%2]
    paddw   xmm1,   xmm5
    psadbw  xmm4,   xmm7
    paddw   xmm2,   xmm6
    paddw   xmm3,   xmm4
%endmacro

%macro SAD_X4_2x16P 1
%if %1
    SAD_X4_START_1x16P
%else
    SAD_X4_1x16P 0, 0
%endif
    SAD_X4_1x16P FENC_STRIDE, esi
    add     edi, 2*FENC_STRIDE
    lea     eax, [eax+2*esi]
    lea     ebx, [ebx+2*esi]
    lea     ecx, [ecx+2*esi]
    lea     edx, [edx+2*esi]
%endmacro

%macro SAD_X3_END 0
    mov     eax,  [esp+32]
    pshufd  xmm4, xmm0, 2
    pshufd  xmm5, xmm1, 2
    pshufd  xmm6, xmm2, 2
    paddw   xmm0, xmm4
    paddw   xmm1, xmm5
    paddw   xmm2, xmm6
    movd    [eax+0], xmm0
    movd    [eax+4], xmm1
    movd    [eax+8], xmm2
    pop     esi
    pop     edi
    ret
%endmacro

%macro SAD_X4_END 0
    mov     eax,  [esp+40]
    psllq   xmm1, 32
    psllq   xmm3, 32
    paddw   xmm0, xmm1
    paddw   xmm2, xmm3
    pshufd  xmm1, xmm0, 14
    pshufd  xmm3, xmm2, 14
    paddw   xmm0, xmm1
    paddw   xmm2, xmm3
    movq    [eax+0], xmm0
    movq    [eax+8], xmm2
    pop     ebx
    pop     esi
    pop     edi
    ret
%endmacro

;-----------------------------------------------------------------------------
;  void x264_pixel_sad_x3_16x16_sse2( uint8_t *fenc, uint8_t *pix0, uint8_t *pix1,
;                                     uint8_t *pix2, int i_stride, int scores[3] )
;-----------------------------------------------------------------------------
%macro SAD_X 4
cglobal x264_pixel_sad_x%1_%2x%3_%4
    SAD_X%1_2x%2P 1
%rep %3/2-1
    SAD_X%1_2x%2P 0
%endrep
    SAD_X%1_END
%endmacro

SAD_X 3, 16, 16, sse2
SAD_X 3, 16,  8, sse2
SAD_X 4, 16, 16, sse2
SAD_X 4, 16,  8, sse2

%ifdef HAVE_SSE3
%define movdqu lddqu
SAD_X 3, 16, 16, sse3
SAD_X 3, 16,  8, sse3
SAD_X 4, 16, 16, sse3
SAD_X 4, 16,  8, sse3
%undef movdqu
%endif


; Core2 (Conroe) can load unaligned data just as quickly as aligned data...
; unless the unaligned data spans the border between 2 cachelines, in which
; case it's really slow. The exact numbers may differ, but all Intel cpus
; have a large penalty for cacheline splits.
; (8-byte alignment exactly half way between two cachelines is ok though.)
; LDDQU was supposed to fix this, but it only works on Pentium 4.
; So in the split case we load aligned data and explicitly perform the
; alignment between registers. Like on archs that have only aligned loads,
; except complicated by the fact that PALIGNR takes only an immediate, not
; a variable alignment.

; computed jump assumes this loop is exactly 80 bytes
%macro SAD16_CACHELINE_LOOP_SSE2 1 ; alignment
ALIGN 16
sad_w16_align%1_sse2:
    movdqa  xmm1, [ecx+16]
    movdqa  xmm2, [ecx+edx+16]
    movdqa  xmm3, [ecx]
    movdqa  xmm4, [ecx+edx]
    pslldq  xmm1, 16-%1
    pslldq  xmm2, 16-%1
    psrldq  xmm3, %1
    psrldq  xmm4, %1
    por     xmm1, xmm3
    por     xmm2, xmm4
    psadbw  xmm1, [eax]
    psadbw  xmm2, [eax+ebx]
    paddw   xmm0, xmm1
    paddw   xmm0, xmm2
    lea     eax,  [eax+2*ebx]
    lea     ecx,  [ecx+2*edx]
    dec     esi
    jg sad_w16_align%1_sse2
    ret
%endmacro

; computed jump assumes this loop is exactly 64 bytes
%macro SAD16_CACHELINE_LOOP_SSSE3 1 ; alignment
ALIGN 16
sad_w16_align%1_ssse3:
    movdqa  xmm1, [ecx+16]
    movdqa  xmm2, [ecx+edx+16]
    palignr xmm1, [ecx], %1
    palignr xmm2, [ecx+edx], %1
    psadbw  xmm1, [eax]
    psadbw  xmm2, [eax+ebx]
    paddw   xmm0, xmm1
    paddw   xmm0, xmm2
    lea     eax,  [eax+2*ebx]
    lea     ecx,  [ecx+2*edx]
    dec     esi
    jg sad_w16_align%1_ssse3
    ret
%endmacro

%macro SAD16_CACHELINE_FUNC 2 ; cpu, height
cglobal x264_pixel_sad_16x%2_cache64_%1
    mov    eax, [esp+12]
    and    eax, 0x37
    cmp    eax, 0x30
    jle x264_pixel_sad_16x%2_sse2
    mov    eax, [esp+12]
    push   ebx
    push   edi
    push   esi
    and    eax, 15
%ifidn %1, ssse3
    shl    eax, 6
%else
    lea    eax, [eax*5]
    shl    eax, 4
%endif
    picgetgot ebx
    lea    edi, [sad_w16_align1_%1 + (sad_w16_align1_%1 - sad_w16_align2_%1) + eax GOT_ebx]
    mov    eax, [esp+16]
    mov    ebx, [esp+20]
    mov    ecx, [esp+24]
    mov    edx, [esp+28]
    and    ecx, ~15
    mov    esi, %2/2
    pxor  xmm0, xmm0
    call   edi
    pop    esi
    pop    edi
    SAD_END_SSE2
%endmacro

%macro SAD_CACHELINE_START_MMX2 4 ; width, height, iterations, cacheline
    mov    eax, [esp+12]
    and    eax, 0x17|%2|(%4>>1)
    cmp    eax, 0x10|%2|(%4>>1)
    jle x264_pixel_sad_%1x%2_mmxext
    push   ebx
    push   esi
    and    eax, 7
    shl    eax, 3
    mov    ecx, 64
    sub    ecx, eax
    movd   mm7, eax
    movd   mm6, ecx
    mov    eax, [esp+12]
    mov    ebx, [esp+16]
    mov    ecx, [esp+20]
    mov    edx, [esp+24]
    and    ecx, ~7
    mov    esi, %3
    pxor   mm0, mm0
%endmacro

%macro SAD16_CACHELINE_FUNC_MMX2 2 ; height, cacheline
cglobal x264_pixel_sad_16x%1_cache%2_mmxext
    SAD_CACHELINE_START_MMX2 16, %1, %1, %2
.loop:
    movq   mm1, [ecx]
    movq   mm2, [ecx+8]
    movq   mm3, [ecx+16]
    movq   mm4, mm2
    psrlq  mm1, mm7
    psllq  mm2, mm6
    psllq  mm3, mm6
    psrlq  mm4, mm7
    por    mm1, mm2
    por    mm3, mm4
    psadbw mm1, [eax]
    psadbw mm3, [eax+8]
    paddw  mm0, mm1
    paddw  mm0, mm3
    add    ecx, edx
    add    eax, ebx
    dec    esi
    jg .loop
    pop    esi
    pop    ebx
    movd   eax, mm0
    ret
%endmacro

%macro SAD8_CACHELINE_FUNC_MMX2 2 ; height, cacheline
cglobal x264_pixel_sad_8x%1_cache%2_mmxext
    SAD_CACHELINE_START_MMX2 8, %1, %1/2, %2
.loop:
    movq   mm1, [ecx+8]
    movq   mm2, [ecx+edx+8]
    movq   mm3, [ecx]
    movq   mm4, [ecx+edx]
    psllq  mm1, mm6
    psllq  mm2, mm6
    psrlq  mm3, mm7
    psrlq  mm4, mm7
    por    mm1, mm3
    por    mm2, mm4
    psadbw mm1, [eax]
    psadbw mm2, [eax+ebx]
    paddw  mm0, mm1
    paddw  mm0, mm2
    lea    ecx, [ecx+2*edx]
    lea    eax, [eax+2*ebx]
    dec    esi
    jg .loop
    pop    esi
    pop    ebx
    movd   eax, mm0
    ret
%endmacro


; sad_x3/x4_cache64: check each mv.
; if they're all within a cacheline, use normal sad_x3/x4.
; otherwise, send them individually to sad_cache64.
%macro CHECK_SPLIT 3 ; pix, width, cacheline
    mov  eax, %1
    and  eax, 0x17|%2|(%3>>1)
    cmp  eax, 0x10|%2|(%3>>1)
    jg .split
%endmacro

%macro SADX3_CACHELINE_FUNC 5 ; width, height, cacheline, normal_ver, split_ver
cglobal x264_pixel_sad_x3_%1x%2_cache%3_%5
    CHECK_SPLIT [esp+8], %1, %3
    CHECK_SPLIT [esp+12], %1, %3
    CHECK_SPLIT [esp+16], %1, %3
    jmp x264_pixel_sad_x3_%1x%2_%4
.split:
    push edi
    mov  edi, [esp+28]
    push dword [esp+24]
    push dword [esp+16]
    push dword 16
    push dword [esp+20]
    call x264_pixel_sad_%1x%2_cache%3_%5
    mov  ecx, [esp+32]
    mov  [edi], eax
    mov  [esp+8], ecx
    call x264_pixel_sad_%1x%2_cache%3_%5
    mov  ecx, [esp+36]
    mov  [edi+4], eax
    mov  [esp+8], ecx
    call x264_pixel_sad_%1x%2_cache%3_%5
    mov  [edi+8], eax
    add  esp, 16
    pop  edi
    ret
%endmacro

%macro SADX4_CACHELINE_FUNC 5 ; width, height, cacheline, normal_ver, split_ver
cglobal x264_pixel_sad_x4_%1x%2_cache%3_%5
    CHECK_SPLIT [esp+8], %1, %3
    CHECK_SPLIT [esp+12], %1, %3
    CHECK_SPLIT [esp+16], %1, %3
    CHECK_SPLIT [esp+20], %1, %3
    jmp x264_pixel_sad_x4_%1x%2_%4
.split:
    push edi
    mov  edi, [esp+32]
    push dword [esp+28]
    push dword [esp+16]
    push dword 16
    push dword [esp+20]
    call x264_pixel_sad_%1x%2_cache%3_%5
    mov  ecx, [esp+32]
    mov  [edi], eax
    mov  [esp+8], ecx
    call x264_pixel_sad_%1x%2_cache%3_%5
    mov  ecx, [esp+36]
    mov  [edi+4], eax
    mov  [esp+8], ecx
    call x264_pixel_sad_%1x%2_cache%3_%5
    mov  ecx, [esp+40]
    mov  [edi+8], eax
    mov  [esp+8], ecx
    call x264_pixel_sad_%1x%2_cache%3_%5
    mov  [edi+12], eax
    add  esp, 16
    pop  edi
    ret
%endmacro

%macro SADX34_CACHELINE_FUNC 5
    SADX3_CACHELINE_FUNC %1, %2, %3, %4, %5
    SADX4_CACHELINE_FUNC %1, %2, %3, %4, %5
%endmacro

cextern x264_pixel_sad_16x16_mmxext
cextern x264_pixel_sad_16x8_mmxext
cextern x264_pixel_sad_8x16_mmxext
cextern x264_pixel_sad_8x8_mmxext
cextern x264_pixel_sad_8x4_mmxext
cextern x264_pixel_sad_x3_16x16_mmxext
cextern x264_pixel_sad_x3_16x8_mmxext
cextern x264_pixel_sad_x3_8x16_mmxext
cextern x264_pixel_sad_x3_8x8_mmxext
cextern x264_pixel_sad_x4_16x16_mmxext
cextern x264_pixel_sad_x4_16x8_mmxext
cextern x264_pixel_sad_x4_8x16_mmxext
cextern x264_pixel_sad_x4_8x8_mmxext

; instantiate the aligned sads

SAD16_CACHELINE_FUNC sse2, 8
SAD16_CACHELINE_FUNC sse2, 16
%assign i 1
%rep 15
SAD16_CACHELINE_LOOP_SSE2 i
%assign i i+1
%endrep

SAD16_CACHELINE_FUNC_MMX2 16, 32
SAD8_CACHELINE_FUNC_MMX2   4, 32
SAD8_CACHELINE_FUNC_MMX2   8, 32
SAD8_CACHELINE_FUNC_MMX2  16, 32
SAD16_CACHELINE_FUNC_MMX2  8, 64
SAD16_CACHELINE_FUNC_MMX2 16, 64
SAD8_CACHELINE_FUNC_MMX2   4, 64
SAD8_CACHELINE_FUNC_MMX2   8, 64
SAD8_CACHELINE_FUNC_MMX2  16, 64
SAD16_CACHELINE_FUNC_MMX2  8, 32

SADX34_CACHELINE_FUNC 16, 16, 32, mmxext, mmxext
SADX34_CACHELINE_FUNC 16, 8,  32, mmxext, mmxext
SADX34_CACHELINE_FUNC 8,  16, 32, mmxext, mmxext
SADX34_CACHELINE_FUNC 8,  8,  32, mmxext, mmxext
SADX34_CACHELINE_FUNC 16, 16, 64, mmxext, mmxext
SADX34_CACHELINE_FUNC 16, 8,  64, mmxext, mmxext
SADX34_CACHELINE_FUNC 8,  16, 64, mmxext, mmxext
SADX34_CACHELINE_FUNC 8,  8,  64, mmxext, mmxext
SADX34_CACHELINE_FUNC 16, 16, 64, sse2, sse2
SADX34_CACHELINE_FUNC 16, 8,  64, sse2, sse2

%ifdef HAVE_SSE3

SAD16_CACHELINE_FUNC ssse3, 8
SAD16_CACHELINE_FUNC ssse3, 16
%assign i 1
%rep 15
SAD16_CACHELINE_LOOP_SSSE3 i
%assign i i+1
%endrep

SADX34_CACHELINE_FUNC 16, 16, 64, sse2, ssse3
SADX34_CACHELINE_FUNC 16, 8,  64, sse2, ssse3

%endif ; HAVE_SSE3


%macro SSD_INC_2x16P_SSE2 0
    movdqu  xmm1,   [eax]
    movdqu  xmm2,   [ecx]
    movdqu  xmm3,   [eax+ebx]
    movdqu  xmm4,   [ecx+edx]

    movdqa  xmm5,   xmm1
    movdqa  xmm6,   xmm3
    psubusb xmm1,   xmm2
    psubusb xmm3,   xmm4
    psubusb xmm2,   xmm5
    psubusb xmm4,   xmm6
    por     xmm1,   xmm2
    por     xmm3,   xmm4

    movdqa  xmm2,   xmm1
    movdqa  xmm4,   xmm3
    punpcklbw xmm1, xmm7
    punpckhbw xmm2, xmm7
    punpcklbw xmm3, xmm7
    punpckhbw xmm4, xmm7
    pmaddwd xmm1,   xmm1
    pmaddwd xmm2,   xmm2
    pmaddwd xmm3,   xmm3
    pmaddwd xmm4,   xmm4

    lea     eax,    [eax+2*ebx]
    lea     ecx,    [ecx+2*edx]

    paddd   xmm1,   xmm2
    paddd   xmm3,   xmm4
    paddd   xmm0,   xmm1
    paddd   xmm0,   xmm3
%endmacro

%macro SSD_START_SSE2 0
    push    ebx

    mov     eax,    [esp+ 8]    ; pix1
    mov     ebx,    [esp+12]    ; stride1
    mov     ecx,    [esp+16]    ; pix2
    mov     edx,    [esp+20]    ; stride2

    pxor    xmm7,   xmm7         ; zero
    pxor    xmm0,   xmm0         ; mm0 holds the sum
%endmacro

%macro SSD_END_SSE2 0
    movdqa  xmm1,   xmm0
    psrldq  xmm1,    8
    paddd   xmm0,   xmm1

    movdqa  xmm1,   xmm0
    psrldq  xmm1,    4
    paddd   xmm0,   xmm1

    movd    eax,    xmm0

    pop ebx
    ret
%endmacro

;-----------------------------------------------------------------------------
;   int __cdecl x264_pixel_ssd_16x16_sse2 (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
cglobal x264_pixel_ssd_16x16_sse2
    SSD_START_SSE2
%rep 8
    SSD_INC_2x16P_SSE2
%endrep
    SSD_END_SSE2

;-----------------------------------------------------------------------------
;   int __cdecl x264_pixel_ssd_16x8_sse2 (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
cglobal x264_pixel_ssd_16x8_sse2
    SSD_START_SSE2
%rep 4
    SSD_INC_2x16P_SSE2
%endrep
    SSD_END_SSE2



%macro SUMSUB_BADC 4
    paddw   %1, %2
    paddw   %3, %4
    paddw   %2, %2
    paddw   %4, %4
    psubw   %2, %1
    psubw   %4, %3  
%endmacro
    
%macro HADAMARD1x4 4
    SUMSUB_BADC %1, %2, %3, %4
    SUMSUB_BADC %1, %3, %2, %4
%endmacro
    
%macro SBUTTERFLY 5
    mov%1       %5, %3
    punpckl%2   %3, %4
    punpckh%2   %5, %4
%endmacro
    
%macro SBUTTERFLY2 5  ; not really needed, but allows transpose4x4 to not shuffle registers
    mov%1       %5, %3
    punpckh%2   %3, %4
    punpckl%2   %5, %4
%endmacro

%macro TRANSPOSE4x4D 5   ; ABCD-T -> ADTC
    SBUTTERFLY dqa, dq,  %1, %2, %5
    SBUTTERFLY dqa, dq,  %3, %4, %2
    SBUTTERFLY dqa, qdq, %1, %3, %4
    SBUTTERFLY dqa, qdq, %5, %2, %3
%endmacro

%macro TRANSPOSE2x4x4W 5   ; ABCD-T -> ABCD
    SBUTTERFLY  dqa, wd,  %1, %2, %5
    SBUTTERFLY  dqa, wd,  %3, %4, %2
    SBUTTERFLY  dqa, dq,  %1, %3, %4
    SBUTTERFLY2 dqa, dq,  %5, %2, %3
    SBUTTERFLY  dqa, qdq, %1, %3, %2
    SBUTTERFLY2 dqa, qdq, %4, %5, %3
%endmacro

%macro LOAD_DIFF_8P 4  ; MMP, MMT, [pix1], [pix2]
    movq        %1, %3
    movq        %2, %4
    punpcklbw   %1, %2
    punpcklbw   %2, %2
    psubw       %1, %2
%endmacro

%macro SUM4x4_SSE2 4    ; 02 13 junk sum
    pxor    %3, %3
    psubw   %3, %1
    pmaxsw  %1, %3

    pxor    %3, %3
    psubw   %3, %2
    pmaxsw  %2, %3

    paddusw %4, %1
    paddusw %4, %2
%endmacro

%macro SUM8x4_SSE2 7    ; a02 a13 junk1 b02 b13 junk2 (1=4 2=5 3=6) sum
    pxor    %3, %3
    pxor    %6, %6
    psubw   %3, %1
    psubw   %6, %4
    pmaxsw  %1, %3
    pmaxsw  %4, %6
    pxor    %3, %3
    pxor    %6, %6
    psubw   %3, %2
    psubw   %6, %5
    pmaxsw  %2, %3
    pmaxsw  %5, %6
    paddusw %1, %2
    paddusw %4, %5
    paddusw %7, %1
    paddusw %7, %4
%endmacro

%macro SUM8x4_SSSE3 7    ; a02 a13 . b02 b13 . sum
    pabsw   %1, %1
    pabsw   %2, %2
    pabsw   %4, %4
    pabsw   %5, %5
    paddusw %1, %2
    paddusw %4, %5
    paddusw %7, %1
    paddusw %7, %4
%endmacro

%macro SATD_TWO_SSE2 0
    LOAD_DIFF_8P xmm0, xmm4, [eax], [ecx]
    LOAD_DIFF_8P xmm1, xmm5, [eax+ebx], [ecx+edx]
    lea          eax,  [eax+2*ebx]
    lea          ecx,  [ecx+2*edx]
    LOAD_DIFF_8P xmm2, xmm4, [eax], [ecx]
    LOAD_DIFF_8P xmm3, xmm5, [eax+ebx], [ecx+edx]
    lea          eax,  [eax+2*ebx]
    lea          ecx,  [ecx+2*edx]

    HADAMARD1x4       xmm0, xmm1, xmm2, xmm3
    TRANSPOSE2x4x4W   xmm0, xmm1, xmm2, xmm3, xmm4
    HADAMARD1x4       xmm0, xmm1, xmm2, xmm3
    SUM8x4            xmm0, xmm1, xmm4, xmm2, xmm3, xmm5, xmm6
%endmacro

%macro SATD_START 0
    push    ebx

    mov     eax,    [esp+ 8]    ; pix1
    mov     ebx,    [esp+12]    ; stride1
    mov     ecx,    [esp+16]    ; pix2
    mov     edx,    [esp+20]    ; stride2

    pxor    xmm6,    xmm6
%endmacro

%macro SATD_END 0
    ; each column sum of SATD is necessarily even, so we don't lose any precision by shifting first.
    psrlw   xmm6, 1
    HADDW   xmm6, xmm7
    movd    eax,  xmm6
    pop     ebx
    ret
%endmacro

%macro SATDS 1
;-----------------------------------------------------------------------------
;   int __cdecl x264_pixel_satd_16x16_sse2 (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
cglobal x264_pixel_satd_16x16_%1
    SATD_START
    SATD_TWO_SSE2
    SATD_TWO_SSE2
    SATD_TWO_SSE2
    SATD_TWO_SSE2
    mov     eax,    [esp+ 8]
    mov     ecx,    [esp+16]
    add     eax,    8
    add     ecx,    8
    SATD_TWO_SSE2
    SATD_TWO_SSE2
    SATD_TWO_SSE2
    SATD_TWO_SSE2
    SATD_END

;-----------------------------------------------------------------------------
;   int __cdecl x264_pixel_satd_8x16_sse2 (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
cglobal x264_pixel_satd_8x16_%1
    SATD_START
    SATD_TWO_SSE2
    SATD_TWO_SSE2
    SATD_TWO_SSE2
    SATD_TWO_SSE2
    SATD_END

;-----------------------------------------------------------------------------
;   int __cdecl x264_pixel_satd_16x8_sse2 (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
cglobal x264_pixel_satd_16x8_%1
    SATD_START
    SATD_TWO_SSE2
    SATD_TWO_SSE2
    mov     eax,    [esp+ 8]
    mov     ecx,    [esp+16]
    add     eax,    8
    add     ecx,    8
    SATD_TWO_SSE2
    SATD_TWO_SSE2
    SATD_END

;-----------------------------------------------------------------------------
;   int __cdecl x264_pixel_satd_8x8_sse2 (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
cglobal x264_pixel_satd_8x8_%1
    SATD_START
    SATD_TWO_SSE2
    SATD_TWO_SSE2
    SATD_END

;-----------------------------------------------------------------------------
;   int __cdecl x264_pixel_satd_8x4_sse2 (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
cglobal x264_pixel_satd_8x4_%1
    SATD_START
    SATD_TWO_SSE2
    SATD_END
%endmacro ; SATDS

%define SUM8x4 SUM8x4_SSE2
SATDS sse2
%ifdef HAVE_SSE3
%define SUM8x4 SUM8x4_SSSE3
SATDS ssse3
%endif



;-----------------------------------------------------------------------------
; void x264_pixel_ssim_4x4x2_core_sse2( const uint8_t *pix1, int stride1,
;                                       const uint8_t *pix2, int stride2, int sums[2][4] )
;-----------------------------------------------------------------------------
cglobal x264_pixel_ssim_4x4x2_core_sse2
    push      ebx
    mov       eax,  [esp+ 8]
    mov       ebx,  [esp+12]
    mov       ecx,  [esp+16]
    mov       edx,  [esp+20]
    pxor      xmm0, xmm0
    pxor      xmm1, xmm1
    pxor      xmm2, xmm2
    pxor      xmm3, xmm3
    pxor      xmm4, xmm4
%rep 4
    movq      xmm5, [eax]
    movq      xmm6, [ecx]
    punpcklbw xmm5, xmm0
    punpcklbw xmm6, xmm0
    paddw     xmm1, xmm5
    paddw     xmm2, xmm6
    movdqa    xmm7, xmm5
    pmaddwd   xmm5, xmm5
    pmaddwd   xmm7, xmm6
    pmaddwd   xmm6, xmm6
    paddd     xmm3, xmm5
    paddd     xmm4, xmm7
    paddd     xmm3, xmm6
    add       eax,  ebx
    add       ecx,  edx
%endrep
    ; PHADDW xmm1, xmm2
    ; PHADDD xmm3, xmm4
    mov       eax,  [esp+24]
    picgetgot ebx
    movdqa    xmm7, [pw_1 GOT_ebx]
    pshufd    xmm5, xmm3, 0xB1
    pmaddwd   xmm1, xmm7
    pmaddwd   xmm2, xmm7
    pshufd    xmm6, xmm4, 0xB1
    packssdw  xmm1, xmm2
    paddd     xmm3, xmm5
    pshufd    xmm1, xmm1, 0xD8
    paddd     xmm4, xmm6
    pmaddwd   xmm1, xmm7
    movdqa    xmm5, xmm3
    punpckldq xmm3, xmm4
    punpckhdq xmm5, xmm4
    movq      [eax+ 0], xmm1
    movq      [eax+ 8], xmm3
    psrldq    xmm1, 8
    movq      [eax+16], xmm1
    movq      [eax+24], xmm5
    pop       ebx
    ret

;-----------------------------------------------------------------------------
; float x264_pixel_ssim_end_sse2( int sum0[5][4], int sum1[5][4], int width )
;-----------------------------------------------------------------------------
cglobal x264_pixel_ssim_end4_sse2
    mov      eax,  [esp+ 4]
    mov      ecx,  [esp+ 8]
    mov      edx,  [esp+12]
    picpush  ebx
    picgetgot ebx
    movdqa   xmm0, [eax+ 0]
    movdqa   xmm1, [eax+16]
    movdqa   xmm2, [eax+32]
    movdqa   xmm3, [eax+48]
    movdqa   xmm4, [eax+64]
    paddd    xmm0, [ecx+ 0]
    paddd    xmm1, [ecx+16]
    paddd    xmm2, [ecx+32]
    paddd    xmm3, [ecx+48]
    paddd    xmm4, [ecx+64]
    paddd    xmm0, xmm1
    paddd    xmm1, xmm2
    paddd    xmm2, xmm3
    paddd    xmm3, xmm4
    movdqa   xmm5, [ssim_c1 GOT_ebx]
    movdqa   xmm6, [ssim_c2 GOT_ebx]
    TRANSPOSE4x4D  xmm0, xmm1, xmm2, xmm3, xmm4

;   s1=mm0, s2=mm3, ss=mm4, s12=mm2
    movdqa   xmm1, xmm3
    pslld    xmm3, 16
    pmaddwd  xmm1, xmm0  ; s1*s2
    por      xmm0, xmm3
    pmaddwd  xmm0, xmm0  ; s1*s1 + s2*s2
    pslld    xmm1, 1
    pslld    xmm2, 7
    pslld    xmm4, 6
    psubd    xmm2, xmm1  ; covar*2
    psubd    xmm4, xmm0  ; vars
    paddd    xmm0, xmm5
    paddd    xmm1, xmm5
    paddd    xmm2, xmm6
    paddd    xmm4, xmm6
    cvtdq2ps xmm0, xmm0  ; (float)(s1*s1 + s2*s2 + ssim_c1)
    cvtdq2ps xmm1, xmm1  ; (float)(s1*s2*2 + ssim_c1)
    cvtdq2ps xmm2, xmm2  ; (float)(covar*2 + ssim_c2)
    cvtdq2ps xmm4, xmm4  ; (float)(vars + ssim_c2)
    mulps    xmm1, xmm2
    mulps    xmm0, xmm4
    divps    xmm1, xmm0  ; ssim

    neg      edx
    movdqu   xmm3, [mask_ff + edx*4 + 16 GOT_ebx]
    pand     xmm1, xmm3
    movhlps  xmm0, xmm1
    addps    xmm0, xmm1
    pshuflw  xmm1, xmm0, 0xE
    addss    xmm0, xmm1

    movd     [picesp+4], xmm0
    fld      dword [picesp+4]
    picpop   ebx
    ret


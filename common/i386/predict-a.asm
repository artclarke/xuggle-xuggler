;*****************************************************************************
;* predict-a.asm: h264 encoder library
;*****************************************************************************
;* Copyright (C) 2005 x264 project
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

;=============================================================================
; Read only data
;=============================================================================

SECTION .rodata data align=16

SECTION .data

;=============================================================================
; Macros
;=============================================================================

%macro SAVE_0_1 1
    movq        [%1]         , mm0
    movq        [%1 + 8]     , mm1
%endmacro

;=============================================================================
; Code
;=============================================================================

SECTION .text

cglobal predict_8x8c_v_mmx
cglobal predict_16x16_v_mmx

;-----------------------------------------------------------------------------
;
; void predict_8x8c_v_mmx( uint8_t *src, int i_stride )
;
;-----------------------------------------------------------------------------

ALIGN 16
predict_8x8c_v_mmx :

    ;push       edi
    ;push       esi

    mov         edx             , [esp + 4]
    mov         ecx             , [esp + 8]
    sub         edx             , ecx               ; esi <-- line -1

    movq        mm0             , [edx]
    movq        [edx + ecx]     , mm0               ; 0
    movq        [edx + 2 * ecx] , mm0               ; 1
    movq        [edx + 4 * ecx] , mm0               ; 3
    movq        [edx + 8 * ecx] , mm0               ; 7
    add         edx             , ecx               ; esi <-- line 0
    movq        [edx + 2 * ecx] , mm0               ; 2
    movq        [edx + 4 * ecx] , mm0               ; 4
    lea         edx             , [edx + 4 * ecx]   ; esi <-- line 4
    movq        [edx + ecx]     , mm0               ; 5
    movq        [edx + 2 * ecx] , mm0               ; 6

    ;pop        esi
    ;pop        edi

    ret

;-----------------------------------------------------------------------------
;
; void predict_16x16_v_mmx( uint8_t *src, int i_stride )
;
;-----------------------------------------------------------------------------

ALIGN 16
predict_16x16_v_mmx :

    ;push       edi
    ;push       esi

    mov         edx, [esp + 4]
    mov         ecx, [esp + 8]
    sub         edx, ecx                ; esi <-- line -1

    movq        mm0, [edx]
    movq        mm1, [edx + 8]
    mov         eax, ecx
    shl         eax, 1
    add         eax, ecx                ; eax <-- 3* stride

    SAVE_0_1    (edx + ecx)             ; 0
    SAVE_0_1    (edx + 2 * ecx)         ; 1
    SAVE_0_1    (edx + eax)             ; 2
    SAVE_0_1    (edx + 4 * ecx)         ; 3
    SAVE_0_1    (edx + 2 * eax)         ; 5
    SAVE_0_1    (edx + 8 * ecx)         ; 7
    SAVE_0_1    (edx + 4 * eax)         ; 11
    add         edx, ecx                ; esi <-- line 0
    SAVE_0_1    (edx + 4 * ecx)         ; 4
    SAVE_0_1    (edx + 2 * eax)         ; 6
    SAVE_0_1    (edx + 8 * ecx)         ; 8
    SAVE_0_1    (edx + 4 * eax)         ; 12
    lea         edx, [edx + 8 * ecx]    ; esi <-- line 8
    SAVE_0_1    (edx + ecx)             ; 9
    SAVE_0_1    (edx + 2 * ecx)         ; 10
    lea         edx, [edx + 4 * ecx]    ; esi <-- line 12
    SAVE_0_1    (edx + ecx)             ; 13
    SAVE_0_1    (edx + 2 * ecx)         ; 14
    SAVE_0_1    (edx + eax)             ; 15


    ;pop        esi
    ;pop        edi

    ret

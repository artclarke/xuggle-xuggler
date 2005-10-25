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

BITS 64

;=============================================================================
; Macros and other preprocessor constants
;=============================================================================

%include "amd64inc.asm"

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
    sub         parm1q, parm2q  ; esi <-- line -1

    movq        mm0,                   [parm1q]
    movq        [parm1q + parm2q],     mm0          ; 0
    movq        [parm1q + 2 * parm2q], mm0          ; 1
    movq        [parm1q + 4 * parm2q], mm0          ; 3
    movq        [parm1q + 8 * parm2q], mm0          ; 7
    add         parm1q,                parm2q       ; <-- line 0
    movq        [parm1q + 2 * parm2q], mm0          ; 2
    movq        [parm1q + 4 * parm2q], mm0          ; 4
    lea         parm1q,                [parm1q + 4 * parm2q] ; <-- line 4
    movq        [parm1q + parm2q],     mm0          ; 5
    movq        [parm1q + 2 * parm2q], mm0          ; 6

    ret

;-----------------------------------------------------------------------------
;
; void predict_16x16_v_mmx( uint8_t *src, int i_stride )
;
;-----------------------------------------------------------------------------

ALIGN 16
predict_16x16_v_mmx :
    sub         parm1q, parm2q                 ; line -1

    movq        mm0, [parm1q]
    movq        mm1, [parm1q + 8]
    lea         rax, [parm2q + 2 * parm2q]     ; 3 * stride

    SAVE_0_1    (parm1q + parm2q)              ; 0
    SAVE_0_1    (parm1q + 2 * parm2q)          ; 1
    SAVE_0_1    (parm1q + rax)                 ; 2
    SAVE_0_1    (parm1q + 4 * parm2q)          ; 3
    SAVE_0_1    (parm1q + 2 * rax)             ; 5
    SAVE_0_1    (parm1q + 8 * parm2q)          ; 7
    SAVE_0_1    (parm1q + 4 * rax)             ; 11
    add         parm1q, parm2q                 ; <-- line 0
    SAVE_0_1    (parm1q + 4 * parm2q)          ; 4
    SAVE_0_1    (parm1q + 2 * rax)             ; 6
    SAVE_0_1    (parm1q + 8 * parm2q)          ; 8
    SAVE_0_1    (parm1q + 4 * rax)             ; 12
    lea         parm1q, [parm1q + 8 * parm2q]  ; <-- line 8
    SAVE_0_1    (parm1q + parm2q)              ; 9
    SAVE_0_1    (parm1q + 2 * parm2q)          ; 10
    lea         parm1q, [parm1q + 4 * parm2q]  ; <-- line 12
    SAVE_0_1    (parm1q + parm2q)              ; 13
    SAVE_0_1    (parm1q + 2 * parm2q)          ; 14
    SAVE_0_1    (parm1q + rax)                 ; 15

    ret

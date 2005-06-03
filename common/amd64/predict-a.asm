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

%macro cglobal 1
    %ifdef PREFIX
        global _%1
        %define %1 _%1
    %else
        global %1
    %endif
%endmacro

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
    movsxd      rcx, esi        ; i_stride

    sub         rdi             , rcx               ; esi <-- line -1

    movq        mm0             , [rdi]
    movq        [rdi + rcx]     , mm0               ; 0
    movq        [rdi + 2 * rcx] , mm0               ; 1
    movq        [rdi + 4 * rcx] , mm0               ; 3
    movq        [rdi + 8 * rcx] , mm0               ; 7
    add         rdi             , rcx               ; esi <-- line 0
    movq        [rdi + 2 * rcx] , mm0               ; 2
    movq        [rdi + 4 * rcx] , mm0               ; 4
    lea         rdi             , [rdi + 4 * rcx]   ; esi <-- line 4
    movq        [rdi + rcx]     , mm0               ; 5
    movq        [rdi + 2 * rcx] , mm0               ; 6

    ret

;-----------------------------------------------------------------------------
;
; void predict_16x16_v_mmx( uint8_t *src, int i_stride )
;
;-----------------------------------------------------------------------------

ALIGN 16
predict_16x16_v_mmx :
    movsxd      rcx, esi                ; i_stride

    sub         rdi, rcx                ; esi <-- line -1

    movq        mm0, [rdi]
    movq        mm1, [rdi + 8]
    lea         rax, [rcx + 2 * rcx]    ; rax <-- 3* stride

    SAVE_0_1    (rdi + rcx)             ; 0
    SAVE_0_1    (rdi + 2 * rcx)         ; 1
    SAVE_0_1    (rdi + rax)             ; 2
    SAVE_0_1    (rdi + 4 * rcx)         ; 3
    SAVE_0_1    (rdi + 2 * rax)         ; 5
    SAVE_0_1    (rdi + 8 * rcx)         ; 7
    SAVE_0_1    (rdi + 4 * rax)         ; 11
    add         rdi, rcx                ; esi <-- line 0
    SAVE_0_1    (rdi + 4 * rcx)         ; 4
    SAVE_0_1    (rdi + 2 * rax)         ; 6
    SAVE_0_1    (rdi + 8 * rcx)         ; 8
    SAVE_0_1    (rdi + 4 * rax)         ; 12
    lea         rdi, [rdi + 8 * rcx]    ; esi <-- line 8
    SAVE_0_1    (rdi + rcx)             ; 9
    SAVE_0_1    (rdi + 2 * rcx)         ; 10
    lea         rdi, [rdi + 4 * rcx]    ; esi <-- line 12
    SAVE_0_1    (rdi + rcx)             ; 13
    SAVE_0_1    (rdi + 2 * rcx)         ; 14
    SAVE_0_1    (rdi + rax)             ; 15

    ret

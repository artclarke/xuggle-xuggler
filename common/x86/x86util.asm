;*****************************************************************************
;* x86inc.asm
;*****************************************************************************
;* Copyright (C) 2008 Loren Merritt <lorenm@u.washington.edu>
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

%macro ABS1_MMX 2    ; a, tmp
    pxor    %2, %2
    psubw   %2, %1
    pmaxsw  %1, %2
%endmacro

%macro ABS2_MMX 4    ; a, b, tmp0, tmp1
    pxor    %3, %3
    pxor    %4, %4
    psubw   %3, %1
    psubw   %4, %2
    pmaxsw  %1, %3
    pmaxsw  %2, %4
%endmacro

%macro ABS1_SSSE3 2
    pabsw   %1, %1
%endmacro

%macro ABS2_SSSE3 4
    pabsw   %1, %1
    pabsw   %2, %2
%endmacro

%define ABS1 ABS1_MMX
%define ABS2 ABS2_MMX

%macro ABS4 6
    ABS2 %1, %2, %5, %6
    ABS2 %3, %4, %5, %6
%endmacro

%macro SUMSUB_BA 2
    paddw   %1, %2
    paddw   %2, %2
    psubw   %2, %1
%endmacro

%macro SUMSUB_BADC 4
    paddw   %1, %2
    paddw   %3, %4
    paddw   %2, %2
    paddw   %4, %4
    psubw   %2, %1
    psubw   %4, %3
%endmacro

%macro HADAMARD8_1D 8
    SUMSUB_BADC %1, %5, %2, %6
    SUMSUB_BADC %3, %7, %4, %8
    SUMSUB_BADC %1, %3, %2, %4
    SUMSUB_BADC %5, %7, %6, %8
    SUMSUB_BADC %1, %2, %3, %4
    SUMSUB_BADC %5, %6, %7, %8
%endmacro

%macro SUMSUB2_AB 3
    mova    %3, %1
    paddw   %1, %1
    paddw   %1, %2
    psubw   %3, %2
    psubw   %3, %2
%endmacro

%macro SUMSUBD2_AB 4
    mova    %4, %1
    mova    %3, %2
    psraw   %2, 1
    psraw   %4, 1
    paddw   %1, %2
    psubw   %4, %3
%endmacro

%macro LOAD_DIFF 5
%ifidn %3, none
    movh       %1, %4
    movh       %2, %5
    punpcklbw  %1, %2
    punpcklbw  %2, %2
    psubw      %1, %2
%else
    movh       %1, %4
    punpcklbw  %1, %3
    movh       %2, %5
    punpcklbw  %2, %3
    psubw      %1, %2
%endif
%endmacro

%macro LOAD_DIFF_8x4P 6 ; 4x dest, 2x temp
    LOAD_DIFF %1, %5, none, [r0],      [r2]
    LOAD_DIFF %2, %6, none, [r0+r1],   [r2+r3]
    LOAD_DIFF %3, %5, none, [r0+2*r1], [r2+2*r3]
    LOAD_DIFF %4, %6, none, [r0+r4],   [r2+r5]
%endmacro

%macro STORE_DIFF 4
    psraw      %1, 6
    movh       %2, %4
    punpcklbw  %2, %3
    paddsw     %1, %2
    packuswb   %1, %1
    movh       %4, %1
%endmacro

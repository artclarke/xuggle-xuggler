/*
 * Copyright (c) 2008 Mans Rullgard <mans@mansr.com>
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "config.h"

        .macro require8, val=1
        .eabi_attribute 24, \val
        .endm

        .macro preserve8, val=1
        .eabi_attribute 25, \val
        .endm

        .macro function name, export=0
.if \export
        .global \name
.endif
        .type   \name, %function
        .func   \name
\name:
        .endm

        .macro movrel rd, val
#if defined(HAVE_ARMV6T2) && !defined(PIC)
        movw            \rd, #:lower16:\val
        movt            \rd, #:upper16:\val
#else
        ldr             \rd, =\val
#endif
        .endm

#define FENC_STRIDE 16
#define FDEC_STRIDE 32

.macro HORIZ_ADD dest, a, b
.ifnb \b
    vadd.u16    \a, \a, \b
.endif
    vpaddl.u16  \a, \a
    vpaddl.u32  \dest, \a
.endm

.macro SUMSUB_AB sum, diff, a, b
    vadd.s16    \sum,  \a, \b
    vsub.s16    \diff, \a, \b
.endm

.macro SUMSUB_ABCD s1, d1, s2, d2, a, b, c, d
    SUMSUB_AB   \s1, \d1, \a, \b
    SUMSUB_AB   \s2, \d2, \c, \d
.endm

.macro ABS2 a b
    vabs.s16 \a, \a
    vabs.s16 \b, \b
.endm

// dist = distance in elements (0 for vertical pass, 1/2 for horizontal passes)
// op = sumsub/amax (sum and diff / maximum of absolutes)
// d1/2 = destination registers
// s1/2 = source registers
.macro HADAMARD dist, op, d1, d2, s1, s2
.if \dist == 1
    vtrn.16     \s1, \s2
.else
    vtrn.32     \s1, \s2
.endif
.ifc \op, sumsub
    SUMSUB_AB   \d1, \d2, \s1, \s2
.else
    vabs.s16    \s1, \s1
    vabs.s16    \s2, \s2
    vmax.s16    \d1, \s1, \s2
.endif
.endm

.macro TRANSPOSE4x4_16  d0 d1 d2 d3
    vtrn.32     \d0, \d2
    vtrn.32     \d1, \d3
    vtrn.16     \d0, \d1
    vtrn.16     \d2, \d3
.endm

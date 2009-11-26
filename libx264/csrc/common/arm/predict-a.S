/*****************************************************************************
 * predict_armv6.S: h264 encoder
 *****************************************************************************
 * Copyright (C) 2009 x264 project
 *
 * Authors: David Conrad <lessen42@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111, USA.
 *****************************************************************************/

#include "asm.S"

.fpu neon

.section .rodata
.align 4

pw_76543210: .short 7,6,5,4,3,2,1,0

.text

// because gcc doesn't believe in using the free shift in add
function x264_predict_4x4_h_armv6
    ldrb    r1, [r0, #0*FDEC_STRIDE-1]
    ldrb    r2, [r0, #1*FDEC_STRIDE-1]
    ldrb    r3, [r0, #2*FDEC_STRIDE-1]
    ldrb    ip, [r0, #3*FDEC_STRIDE-1]
    add     r1, r1, r1, lsl #8
    add     r2, r2, r2, lsl #8
    add     r3, r3, r3, lsl #8
    add     ip, ip, ip, lsl #8
    add     r1, r1, r1, lsl #16
    str     r1, [r0, #0*FDEC_STRIDE]
    add     r2, r2, r2, lsl #16
    str     r2, [r0, #1*FDEC_STRIDE]
    add     r3, r3, r3, lsl #16
    str     r3, [r0, #2*FDEC_STRIDE]
    add     ip, ip, ip, lsl #16
    str     ip, [r0, #3*FDEC_STRIDE]
    bx      lr
.endfunc

function x264_predict_4x4_dc_armv6
    mov     ip, #0
    ldr     r1, [r0, #-FDEC_STRIDE]
    ldrb    r2, [r0, #0*FDEC_STRIDE-1]
    ldrb    r3, [r0, #1*FDEC_STRIDE-1]
    usad8   r1, r1, ip
    add     r2, r2, #4
    ldrb    ip, [r0, #2*FDEC_STRIDE-1]
    add     r2, r2, r3
    ldrb    r3, [r0, #3*FDEC_STRIDE-1]
    add     r2, r2, ip
    add     r2, r2, r3
    add     r1, r1, r2
    lsr     r1, r1, #3
    add     r1, r1, r1, lsl #8
    add     r1, r1, r1, lsl #16
    str     r1, [r0, #0*FDEC_STRIDE]
    str     r1, [r0, #1*FDEC_STRIDE]
    str     r1, [r0, #2*FDEC_STRIDE]
    str     r1, [r0, #3*FDEC_STRIDE]
    bx      lr
.endfunc

// return a1 = (a1+2*b1+c1+2)>>2  a2 = (a2+2*b2+c2+2)>>2
.macro PRED4x4_LOWPASS a1 b1 c1 a2 b2 c2 pb_1
    uhadd8  \a1, \a1, \c1
    uhadd8  \a2, \a2, \c2
    uhadd8  \c1, \a1, \b1
    uhadd8  \c2, \a2, \b2
    eor     \a1, \a1, \b1
    eor     \a2, \a2, \b2
    and     \a1, \a1, \pb_1
    and     \a2, \a2, \pb_1
    uadd8   \a1, \a1, \c1
    uadd8   \a2, \a2, \c2
.endm

function x264_predict_4x4_ddr_armv6
    ldr     r1, [r0, # -FDEC_STRIDE]
    ldrb    r2, [r0, # -FDEC_STRIDE-1]
    ldrb    r3, [r0, #0*FDEC_STRIDE-1]
    push    {r4-r6,lr}
    add     r2, r2, r1, lsl #8
    ldrb    r4, [r0, #1*FDEC_STRIDE-1]
    add     r3, r3, r2, lsl #8
    ldrb    r5, [r0, #2*FDEC_STRIDE-1]
    ldrb    r6, [r0, #3*FDEC_STRIDE-1]
    add     r4, r4, r3, lsl #8
    add     r5, r5, r4, lsl #8
    add     r6, r6, r5, lsl #8
    ldr     ip, =0x01010101
    PRED4x4_LOWPASS r1, r2, r3, r4, r5, r6, ip
    str     r1, [r0, #0*FDEC_STRIDE]
    lsl     r2, r1, #8
    lsl     r3, r1, #16
    lsl     r4, r4, #8
    lsl     r5, r1, #24
    add     r2, r2, r4, lsr #24
    str     r2, [r0, #1*FDEC_STRIDE]
    add     r3, r3, r4, lsr #16
    str     r3, [r0, #2*FDEC_STRIDE]
    add     r5, r5, r4, lsr #8
    str     r5, [r0, #3*FDEC_STRIDE]
    pop     {r4-r6,pc}
.endfunc

function x264_predict_4x4_ddl_neon
    sub         r0, #FDEC_STRIDE
    mov         ip, #FDEC_STRIDE
    vld1.64     {d0}, [r0], ip
    vdup.8      d3, d0[7]
    vext.8      d1, d0, d0, #1
    vext.8      d2, d0, d3, #2
    vhadd.u8    d0, d0, d2
    vrhadd.u8   d0, d0, d1
    vst1.32     {d0[0]}, [r0,:32], ip
    vext.8      d1, d0, d0, #1
    vext.8      d2, d0, d0, #2
    vst1.32     {d1[0]}, [r0,:32], ip
    vext.8      d3, d0, d0, #3
    vst1.32     {d2[0]}, [r0,:32], ip
    vst1.32     {d3[0]}, [r0,:32], ip
    bx          lr
.endfunc

function x264_predict_8x8_dc_neon
    mov     ip, #0
    ldrd    r2, [r1, #8]
    push    {r4-r5,lr}
    ldrd    r4, [r1, #16]
    lsl     r3, r3, #8
    ldrb    lr, [r1, #7]
    usad8   r2, r2, ip
    usad8   r3, r3, ip
    usada8  r2, r4, ip, r2
    add     lr, lr, #8
    usada8  r3, r5, ip, r3
    add     r2, r2, lr
    mov     ip, #FDEC_STRIDE
    add     r2, r2, r3
    lsr     r2, r2, #4

    vdup.8   d0, r2
.rept 8
    vst1.64 {d0}, [r0,:64], ip
.endr
    pop    {r4-r5,pc}
.endfunc


function x264_predict_8x8_h_neon
    add         r1, r1, #7
    mov         ip, #FDEC_STRIDE
    vld1.64     {d16}, [r1]
    vdup.8      d0, d16[7]
    vdup.8      d1, d16[6]
    vst1.64     {d0}, [r0,:64], ip
    vdup.8      d2, d16[5]
    vst1.64     {d1}, [r0,:64], ip
    vdup.8      d3, d16[4]
    vst1.64     {d2}, [r0,:64], ip
    vdup.8      d4, d16[3]
    vst1.64     {d3}, [r0,:64], ip
    vdup.8      d5, d16[2]
    vst1.64     {d4}, [r0,:64], ip
    vdup.8      d6, d16[1]
    vst1.64     {d5}, [r0,:64], ip
    vdup.8      d7, d16[0]
    vst1.64     {d6}, [r0,:64], ip
    vst1.64     {d7}, [r0,:64], ip
    bx          lr
.endfunc

function x264_predict_8x8c_h_neon
    sub         r1, r0, #1
    mov         ip, #FDEC_STRIDE
.rept 4
    vld1.8      {d0[]}, [r1], ip
    vld1.8      {d2[]}, [r1], ip
    vst1.64     {d0}, [r0,:64], ip
    vst1.64     {d2}, [r0,:64], ip
.endr
    bx          lr
.endfunc

function x264_predict_8x8c_v_neon
    sub         r0, r0, #FDEC_STRIDE
    mov         ip, #FDEC_STRIDE
    vld1.64     {d0}, [r0,:64], ip
.rept 8
    vst1.64     {d0}, [r0,:64], ip
.endr
    bx          lr
.endfunc


function x264_predict_16x16_dc_neon
    sub         r3, r0, #FDEC_STRIDE
    sub         r0, r0, #1
    vld1.64     {d0-d1}, [r3,:128]
    ldrb        ip, [r0], #FDEC_STRIDE
    vaddl.u8    q0, d0, d1
    ldrb        r1, [r0], #FDEC_STRIDE
    vadd.u16    d0, d0, d1
    vpadd.u16   d0, d0, d0
    vpadd.u16   d0, d0, d0
.rept 4
    ldrb        r2, [r0], #FDEC_STRIDE
    add         ip, ip, r1
    ldrb        r3, [r0], #FDEC_STRIDE
    add         ip, ip, r2
    ldrb        r1, [r0], #FDEC_STRIDE
    add         ip, ip, r3
.endr
    ldrb        r2, [r0], #FDEC_STRIDE
    add         ip, ip, r1
    ldrb        r3, [r0], #FDEC_STRIDE
    add         ip, ip, r2

    sub         r0, r0, #FDEC_STRIDE*16
    add         ip, ip, r3
    vdup.16     d1, ip
    vadd.u16    d0, d0, d1
    mov         ip, #FDEC_STRIDE
    add         r0, r0, #1
    vrshr.u16   d0, d0, #5
    vdup.8      q0, d0[0]
.rept 16
    vst1.64     {d0-d1}, [r0,:64], ip
.endr
    bx          lr
.endfunc

function x264_predict_16x16_h_neon
    sub         r1, r0, #1
    mov         ip, #FDEC_STRIDE
.rept 8
    vld1.8      {d0[]}, [r1], ip
    vmov        d1, d0
    vld1.8      {d2[]}, [r1], ip
    vmov        d3, d2
    vst1.64     {d0-d1}, [r0,:128], ip
    vst1.64     {d2-d3}, [r0,:128], ip
.endr
    bx          lr
.endfunc

function x264_predict_16x16_v_neon
    sub         r0, r0, #FDEC_STRIDE
    mov         ip, #FDEC_STRIDE
    vld1.64     {d0-d1}, [r0,:128], ip
.rept 16
    vst1.64     {d0-d1}, [r0,:128], ip
.endr
    bx          lr
.endfunc

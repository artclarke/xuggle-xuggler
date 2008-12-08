/*****************************************************************************
 * mc.h: h264 encoder library
 *****************************************************************************
 * Copyright (C) 2008 x264 Project
 *
 * Authors: Jason Garrett-Glaser <darkshikari@gmail.com>
 *          Loren Merritt <lorenm@u.washington.edu>
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

#ifndef X264_X86_UTIL_H
#define X264_X86_UTIL_H

#ifdef __GNUC__
#define x264_median_mv x264_median_mv_mmxext
static inline void x264_median_mv_mmxext( int16_t *dst, int16_t *a, int16_t *b, int16_t *c )
{
    asm(
        "movd   %1,    %%mm0 \n"
        "movd   %2,    %%mm1 \n"
        "movq   %%mm0, %%mm3 \n"
        "movd   %3,    %%mm2 \n"
        "pmaxsw %%mm1, %%mm0 \n"
        "pminsw %%mm3, %%mm1 \n"
        "pminsw %%mm2, %%mm0 \n"
        "pmaxsw %%mm1, %%mm0 \n"
        "movd   %%mm0, %0    \n"
        :"=m"(*(uint32_t*)dst)
        :"m"(*(uint32_t*)a), "m"(*(uint32_t*)b), "m"(*(uint32_t*)c)
    );
}
#define x264_predictor_difference x264_predictor_difference_mmxext
static inline int x264_predictor_difference_mmxext( int16_t (*mvc)[2], intptr_t i_mvc )
{
    int sum = 0;
    uint16_t output[4];
    asm(
        "pxor    %%mm4, %%mm4 \n"
        "test    $1, %1       \n"
        "jnz 3f               \n"
        "movd    -8(%2,%1,4), %%mm0 \n"
        "movd    -4(%2,%1,4), %%mm3 \n"
        "psubw   %%mm3, %%mm0 \n"
        "jmp 2f               \n"
        "3:                   \n"
        "sub     $1,    %1    \n"
        "1:                   \n"
        "movq    -8(%2,%1,4), %%mm0 \n"
        "psubw   -4(%2,%1,4), %%mm0 \n"
        "2:                   \n"
        "sub     $2,    %1    \n"
        "pxor    %%mm2, %%mm2 \n"
        "psubw   %%mm0, %%mm2 \n"
        "pmaxsw  %%mm2, %%mm0 \n"
        "paddusw %%mm0, %%mm4 \n"
        "jg 1b                \n"
        "movq    %%mm4, %0    \n"
        :"=m"(output), "+r"(i_mvc)
        :"r"(mvc), "m"(*(struct {int16_t x[4];} *)mvc)
    );
    sum += output[0] + output[1] + output[2] + output[3];
    return sum;
}
#undef array_non_zero_int
#define array_non_zero_int array_non_zero_int_mmx
static ALWAYS_INLINE int array_non_zero_int_mmx( void *v, int i_count )
{
    if(i_count == 128)
    {
        int nonzero = 0;
        asm(
            "movq     (%1),    %%mm0 \n"
            "por      8(%1),   %%mm0 \n"
            "por      16(%1),  %%mm0 \n"
            "por      24(%1),  %%mm0 \n"
            "por      32(%1),  %%mm0 \n"
            "por      40(%1),  %%mm0 \n"
            "por      48(%1),  %%mm0 \n"
            "por      56(%1),  %%mm0 \n"
            "por      64(%1),  %%mm0 \n"
            "por      72(%1),  %%mm0 \n"
            "por      80(%1),  %%mm0 \n"
            "por      88(%1),  %%mm0 \n"
            "por      96(%1),  %%mm0 \n"
            "por      104(%1), %%mm0 \n"
            "por      112(%1), %%mm0 \n"
            "por      120(%1), %%mm0 \n"
            "packsswb %%mm0,   %%mm0 \n"
            "movd     %%mm0,   %0    \n"
            :"=r"(nonzero)
            :"r"(v), "m"(*(struct {int16_t x[64];} *)v)
        );
        return !!nonzero;
    }
    else return array_non_zero_int_c( v, i_count );
}
#endif

#endif

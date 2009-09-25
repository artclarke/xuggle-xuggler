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
#define x264_cabac_amvd_sum x264_cabac_amvd_sum_mmxext
static ALWAYS_INLINE uint32_t x264_cabac_amvd_sum_mmxext(int16_t *mvdleft, int16_t *mvdtop)
{
    static const uint64_t pw_2    = 0x0002000200020002ULL;
    static const uint64_t pw_28   = 0x001C001C001C001CULL;
    static const uint64_t pw_2184 = 0x0888088808880888ULL;
    /* MIN(((x+28)*2184)>>16,2) = (x>2) + (x>32) */
    /* 2184 = fix16(1/30) */
    uint32_t amvd;
    asm(
        "movd      %1, %%mm0 \n"
        "movd      %2, %%mm1 \n"
        "pxor   %%mm2, %%mm2 \n"
        "pxor   %%mm3, %%mm3 \n"
        "psubw  %%mm0, %%mm2 \n"
        "psubw  %%mm1, %%mm3 \n"
        "pmaxsw %%mm2, %%mm0 \n"
        "pmaxsw %%mm3, %%mm1 \n"
        "paddw     %3, %%mm0 \n"
        "paddw  %%mm1, %%mm0 \n"
        "pmulhuw   %4, %%mm0 \n"
        "pminsw    %5, %%mm0 \n"
        "movd   %%mm0, %0    \n"
        :"=r"(amvd)
        :"m"(*(uint32_t*)mvdleft),"m"(*(uint32_t*)mvdtop),
         "m"(pw_28),"m"(pw_2184),"m"(pw_2)
    );
    return amvd;
}
#endif

#endif

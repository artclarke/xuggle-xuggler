/*****************************************************************************
 * predict.c: h264 encoder
 *****************************************************************************
 * Copyright (C) 2007-2008 Guillaume Poirier <gpoirier@mplayerhq.hu>
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

#ifdef SYS_LINUX
#include <altivec.h>
#endif

#include "common/common.h"
#include "predict.h"
#include "pixel.h"
#include "ppccommon.h"

static void predict_16x16_p_altivec( uint8_t *src )
{
    int16_t a, b, c, i;
    int H = 0;
    int V = 0;
    int16_t i00;

    for( i = 1; i <= 8; i++ )
    {
        H += i * ( src[7+i - FDEC_STRIDE ]  - src[7-i - FDEC_STRIDE ] );
        V += i * ( src[(7+i)*FDEC_STRIDE -1] - src[(7-i)*FDEC_STRIDE -1] );
    }

    a = 16 * ( src[15*FDEC_STRIDE -1] + src[15 - FDEC_STRIDE] );
    b = ( 5 * H + 32 ) >> 6;
    c = ( 5 * V + 32 ) >> 6;
    i00 = a - b * 7 - c * 7 + 16;

    vect_sshort_u i00_u, b_u, c_u;
    i00_u.s[0] = i00;
    b_u.s[0]   = b;
    c_u.s[0]   = c;

    vec_u16_t val5_v = vec_splat_u16(5);
    vec_s16_t i00_v, b_v, c_v;
    i00_v = vec_splat(i00_u.v, 0);
    b_v = vec_splat(b_u.v, 0);
    c_v = vec_splat(c_u.v, 0);
    vec_s16_t induc_v  = (vec_s16_t) CV(0,  1,  2,  3,  4,  5,  6,  7);
    vec_s16_t b8_v = vec_sl(b_v, vec_splat_u16(3));
    vec_s32_t mule_b_v = vec_mule(induc_v, b_v);
    vec_s32_t mulo_b_v = vec_mulo(induc_v, b_v);
    vec_s16_t mul_b_induc0_v = vec_pack(vec_mergeh(mule_b_v, mulo_b_v), vec_mergel(mule_b_v, mulo_b_v));
    vec_s16_t add_i0_b_0v = vec_adds(i00_v, mul_b_induc0_v);
    vec_s16_t add_i0_b_8v = vec_adds(b8_v, add_i0_b_0v);

    int y;

    for( y = 0; y < 16; y++ )
    {
        vec_s16_t shift_0_v = vec_sra(add_i0_b_0v, val5_v);
        vec_s16_t shift_8_v = vec_sra(add_i0_b_8v, val5_v);
        vec_u8_t com_sat_v = vec_packsu(shift_0_v, shift_8_v);
        vec_st( com_sat_v, 0, &src[0]);
        src += FDEC_STRIDE;
        i00 += c;
        add_i0_b_0v = vec_adds(add_i0_b_0v, c_v);
        add_i0_b_8v = vec_adds(add_i0_b_8v, c_v);
    }
}

/****************************************************************************
 * Exported functions:
 ****************************************************************************/
void x264_predict_16x16_init_altivec( x264_predict_t pf[7] )
{
    pf[I_PRED_16x16_P]       = predict_16x16_p_altivec;
}

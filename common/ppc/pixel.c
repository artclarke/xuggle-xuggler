/*****************************************************************************
 * pixel.c: h264 encoder
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar
 * $Id: pixel.c,v 1.1 2004/06/03 19:27:07 fenrir Exp $
 *
 * Authors: Eric Petit <titer@m0k.org>
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111, USA.
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>

#ifdef SYS_LINUX
#include <altivec.h>
#endif

#include "x264.h"
#include "common/pixel.h"
#include "pixel.h"
#include "ppccommon.h"

/* sad routines */
#define PIXEL_SAD_ALTIVEC( name, lx, ly, a, b )        \
static int name( uint8_t *pix1, int i_pix1,            \
                 uint8_t *pix2, int i_pix2 )           \
{                                                      \
    int y;                                             \
    DECLARE_ALIGNED( int, sum, 16 );                   \
                                                       \
    LOAD_ZERO;                                         \
    vector_u8_t  pix1v, pix2v;                         \
    vector_s32_t sumv = zero_s32;                      \
    for( y = 0; y < ly; y++ )                          \
    {                                                  \
        LOAD_##lx( pix1, pix1v );                      \
        LOAD_##lx( pix2, pix2v );                      \
        sumv = (vector_s32_t) vec_sum4s(               \
                   vec_sub( vec_max( pix1v, pix2v ),   \
                            vec_min( pix1v, pix2v ) ), \
                   (vector_u32_t) sumv );              \
        pix1 += i_pix1;                                \
        pix2 += i_pix2;                                \
    }                                                  \
    sumv = vec_sum##a( sumv, zero_s32 );               \
    vec_ste( vec_splat( sumv, b ), 0, &sum );          \
    return sum;                                        \
}

PIXEL_SAD_ALTIVEC( pixel_sad_16x16_altivec, 16, 16, s,  3 )
PIXEL_SAD_ALTIVEC( pixel_sad_8x16_altivec,  8,  16, 2s, 1 )
PIXEL_SAD_ALTIVEC( pixel_sad_16x8_altivec,  16, 8,  s,  3 )
PIXEL_SAD_ALTIVEC( pixel_sad_8x8_altivec,   8,  8,  2s, 1 )

/* satd routines */
static inline int pixel_satd_8x8_altivec( uint8_t *pix1, int i_pix1,
                                          uint8_t *pix2, int i_pix2 )
{
    int i;
    DECLARE_ALIGNED( int, i_satd, 16 );

    LOAD_ZERO;
    vector_s32_t satdv = zero_s32;
    vector_u8_t  pix1u8v, pix2u8v;
    vector_s16_t pix1s16v, pix2s16v;
    vector_s16_t diffv[8];
    vector_s16_t tmpv[8];
    vector_s16_t s01v, s23v, d01v, d23v;

    /* Diff 8x8 */
    for( i = 0; i < 8; i++ )
    {
        LOAD_8( pix1, pix1u8v );
        LOAD_8( pix2, pix2u8v );

        /* u8 -> s16 conversion */
        pix1s16v = (vector_s16_t) vec_mergeh( zero_u8, pix1u8v );
        pix2s16v = (vector_s16_t) vec_mergeh( zero_u8, pix2u8v );

        diffv[i] = vec_sub( pix1s16v, pix2s16v );

        pix1 += i_pix1;
        pix2 += i_pix2;
    }

    /* Hadamar H */
    HADAMAR( &diffv[0], &tmpv[0] );
    HADAMAR( &diffv[4], &tmpv[4] );

    /* Transpose */
    TRANSPOSE8x8( tmpv, diffv );

    /* Hadamar V */
    HADAMAR( &diffv[0], &tmpv[0] );
    HADAMAR( &diffv[4], &tmpv[4] );

    /* Sum of absolute values */
    for( i = 0; i < 8; i++ )
    {
        satdv = vec_sum4s( vec_abs( tmpv[i] ), satdv );
    }
    satdv = vec_sums( satdv, zero_s32 );

    /* Done */
    vec_ste( vec_splat( satdv, 3 ), 0, &i_satd );
    return i_satd / 2;
}

static int pixel_satd_16x8_altivec( uint8_t *pix1, int i_pix1,
                                    uint8_t *pix2, int i_pix2 )
{
    return pixel_satd_8x8_altivec( &pix1[0], i_pix1,
                                   &pix2[0], i_pix2 ) +
           pixel_satd_8x8_altivec( &pix1[8], i_pix1,
                                   &pix2[8], i_pix2 );
}
static int pixel_satd_8x16_altivec( uint8_t *pix1, int i_pix1,
                                    uint8_t *pix2, int i_pix2 )
{
    return pixel_satd_8x8_altivec( &pix1[0], i_pix1,
                                   &pix2[0], i_pix2 ) +
           pixel_satd_8x8_altivec( &pix1[8*i_pix1], i_pix1,
                                   &pix2[8*i_pix2], i_pix2 );
}
static int pixel_satd_16x16_altivec( uint8_t *pix1, int i_pix1,
                                     uint8_t *pix2, int i_pix2 )
{
    return pixel_satd_8x8_altivec( &pix1[0], i_pix1,
                                   &pix2[0], i_pix2 ) +
           pixel_satd_8x8_altivec( &pix1[8], i_pix1,
                                   &pix2[8], i_pix2 ) +
           pixel_satd_8x8_altivec( &pix1[8*i_pix1], i_pix1,
                                   &pix2[8*i_pix2], i_pix2 ) +
           pixel_satd_8x8_altivec( &pix1[8*i_pix1+8], i_pix1,
                                   &pix2[8*i_pix2+8], i_pix2 );
}

static inline int pixel_satd_4x4_altivec( uint8_t *pix1, int i_pix1,
                                          uint8_t *pix2, int i_pix2 )
{
    int i;
    DECLARE_ALIGNED( int, i_satd, 16 );

    LOAD_ZERO;
    vector_s32_t satdv = zero_s32;
    vector_u8_t  pix1u8v, pix2u8v;
    vector_s16_t pix1s16v, pix2s16v;
    vector_s16_t diffv[4];
    vector_s16_t tmpv[4];
    vector_s16_t s01v, s23v, d01v, d23v;

    /* Diff 4x8 */
    for( i = 0; i < 4; i++ )
    {
        LOAD_4( pix1, pix1u8v );
        LOAD_4( pix2, pix2u8v );

        /* u8 -> s16 conversion */
        pix1s16v = (vector_s16_t) vec_mergeh( zero_u8, pix1u8v );
        pix2s16v = (vector_s16_t) vec_mergeh( zero_u8, pix2u8v );

        diffv[i] = vec_sub( pix1s16v, pix2s16v );

        pix1 += i_pix1;
        pix2 += i_pix2;
    }

    /* Hadamar H */
    HADAMAR( diffv, tmpv );

    /* Transpose */
    TRANSPOSE4x4( tmpv, diffv );

    /* Hadamar V */
    HADAMAR( diffv, tmpv );

    /* Sum of absolute values */
    for( i = 0; i < 4; i++ )
    {
        satdv = vec_sum4s( vec_abs( tmpv[i] ), satdv );
    }
    satdv = vec_sum2s( satdv, zero_s32 );

    /* Done */
    vec_ste( vec_splat( satdv, 1 ), 0, &i_satd );
    return i_satd / 2;
}

/****************************************************************************
 * x264_pixel_init:
 ****************************************************************************/
void x264_pixel_altivec_init( x264_pixel_function_t *pixf )
{
    pixf->sad[PIXEL_16x16]  = pixel_sad_16x16_altivec;
    pixf->sad[PIXEL_8x16]   = pixel_sad_8x16_altivec;
    pixf->sad[PIXEL_16x8]   = pixel_sad_16x8_altivec;
    pixf->sad[PIXEL_8x8]    = pixel_sad_8x8_altivec;

    pixf->satd[PIXEL_16x16] = pixel_satd_16x16_altivec;
    pixf->satd[PIXEL_8x16]  = pixel_satd_8x16_altivec;
    pixf->satd[PIXEL_16x8]  = pixel_satd_16x8_altivec;
    pixf->satd[PIXEL_8x8]   = pixel_satd_8x8_altivec;
    pixf->satd[PIXEL_4x4]   = pixel_satd_4x4_altivec;
}

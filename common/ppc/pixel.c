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
    PREP_LOAD;                                         \
    vec_u8_t  pix1v, pix2v;                            \
    vec_s32_t sumv = zero_s32v;                        \
    for( y = 0; y < ly; y++ )                          \
    {                                                  \
        VEC_LOAD( pix1, pix1v, lx );                   \
        VEC_LOAD( pix2, pix2v, lx );                   \
        sumv = (vec_s32_t) vec_sum4s(                  \
                   vec_sub( vec_max( pix1v, pix2v ),   \
                            vec_min( pix1v, pix2v ) ), \
                   (vec_u32_t) sumv );                 \
        pix1 += i_pix1;                                \
        pix2 += i_pix2;                                \
    }                                                  \
    sumv = vec_sum##a( sumv, zero_s32v );              \
    vec_ste( vec_splat( sumv, b ), 0, &sum );          \
    return sum;                                        \
}

PIXEL_SAD_ALTIVEC( pixel_sad_16x16_altivec, 16, 16, s,  3 )
PIXEL_SAD_ALTIVEC( pixel_sad_8x16_altivec,  8,  16, 2s, 1 )
PIXEL_SAD_ALTIVEC( pixel_sad_16x8_altivec,  16, 8,  s,  3 )
PIXEL_SAD_ALTIVEC( pixel_sad_8x8_altivec,   8,  8,  2s, 1 )

/* satd routines */

#define DO_DIFF(a,b) \
    VEC_LOAD( pix1, pix1v, b ); \
    pix1v       = vec_u8_to_s16( pix1v ); \
    VEC_LOAD( pix2, pix2v, b ); \
    pix2v       = vec_u8_to_s16( pix2v ); \
    diff##a##v  = vec_sub( pix1v, pix2v ); \
    pix1       += i_pix1; \
    pix2       += i_pix2

#define ADD_ABS_TO_SATD(a,b) \
    temp##a##v = vec_abs( temp##a##v ); \
    satdv      = vec_sum4s( temp##a##v, b )

static inline int pixel_satd_4x4_altivec( uint8_t *pix1, int i_pix1,
                                          uint8_t *pix2, int i_pix2 )
{
    DECLARE_ALIGNED( int, i_satd, 16 );

    LOAD_ZERO;
    PREP_LOAD;
    vec_s16_t pix1v, pix2v;
    vec_s16_t diff0v, diff1v, diff2v, diff3v;
    vec_s16_t temp0v, temp1v, temp2v, temp3v;
    vec_s32_t satdv;

    DO_DIFF( 0, 4 );
    DO_DIFF( 1, 4 );
    DO_DIFF( 2, 4 );
    DO_DIFF( 3, 4 );

    /* Hadamar H */
    vec_hadamar( diff0v, diff1v, diff2v, diff3v,
                 temp0v, temp1v, temp2v, temp3v );
    
    vec_transpose4x4( temp0v, temp1v, temp2v, temp3v,
                      diff0v, diff1v, diff2v, diff3v );
    /* Hadamar V */
    vec_hadamar( diff0v, diff1v, diff2v, diff3v,
                 temp0v, temp1v, temp2v, temp3v );

    ADD_ABS_TO_SATD( 0, zero_s32v );
    ADD_ABS_TO_SATD( 1, satdv );
    ADD_ABS_TO_SATD( 2, satdv );
    ADD_ABS_TO_SATD( 3, satdv );

    satdv = vec_sum2s( satdv, zero_s32v );
    satdv = vec_splat( satdv, 1 );
    vec_ste( satdv, 0, &i_satd );

    return i_satd / 2;
}

static inline int pixel_satd_8x8_altivec( uint8_t *pix1, int i_pix1,
                                          uint8_t *pix2, int i_pix2 )
{
    DECLARE_ALIGNED( int, i_satd, 16 );

    LOAD_ZERO;
    PREP_LOAD;
    vec_s32_t satdv;
    vec_s16_t pix1v, pix2v;
    vec_s16_t diff0v, diff1v, diff2v, diff3v,
              diff4v, diff5v, diff6v, diff7v;
    vec_s16_t temp0v, temp1v, temp2v, temp3v,
              temp4v, temp5v, temp6v, temp7v;

    DO_DIFF( 0, 8 );
    DO_DIFF( 1, 8 );
    DO_DIFF( 2, 8 );
    DO_DIFF( 3, 8 );
    DO_DIFF( 4, 8 );
    DO_DIFF( 5, 8 );
    DO_DIFF( 6, 8 );
    DO_DIFF( 7, 8 );

    /* Hadamar H */
    vec_hadamar( diff0v, diff1v, diff2v, diff3v,
                 temp0v, temp1v, temp2v, temp3v );
    vec_hadamar( diff4v, diff5v, diff6v, diff7v,
                 temp4v, temp5v, temp6v, temp7v );

    vec_transpose8x8( temp0v, temp1v, temp2v, temp3v,
                      temp4v, temp5v, temp6v, temp7v,
                      diff0v, diff1v, diff2v, diff3v,
                      diff4v, diff5v, diff6v, diff7v );

    /* Hadamar V */
    vec_hadamar( diff0v, diff1v, diff2v, diff3v,
                 temp0v, temp1v, temp2v, temp3v );
    vec_hadamar( diff4v, diff5v, diff6v, diff7v,
                 temp4v, temp5v, temp6v, temp7v );

    ADD_ABS_TO_SATD( 0, zero_s32v );
    ADD_ABS_TO_SATD( 1, satdv );
    ADD_ABS_TO_SATD( 2, satdv );
    ADD_ABS_TO_SATD( 3, satdv );
    ADD_ABS_TO_SATD( 4, satdv );
    ADD_ABS_TO_SATD( 5, satdv );
    ADD_ABS_TO_SATD( 6, satdv );
    ADD_ABS_TO_SATD( 7, satdv );

    satdv = vec_sums( satdv, zero_s32v );
    satdv = vec_splat( satdv, 3 );
    vec_ste( satdv, 0, &i_satd );

    return i_satd / 2;
}

static int pixel_satd_8x4_altivec( uint8_t *pix1, int i_pix1,
                                   uint8_t *pix2, int i_pix2 )
{
    return pixel_satd_4x4_altivec( &pix1[0], i_pix1,
                                   &pix2[0], i_pix2 ) +
           pixel_satd_4x4_altivec( &pix1[4], i_pix1,
                                   &pix2[4], i_pix2 );
}

static int pixel_satd_4x8_altivec( uint8_t *pix1, int i_pix1,
                                   uint8_t *pix2, int i_pix2 )
{
    return pixel_satd_4x4_altivec( &pix1[0], i_pix1,
                                   &pix2[0], i_pix2 ) +
           pixel_satd_4x4_altivec( &pix1[4*i_pix1], i_pix1,
                                   &pix2[4*i_pix2], i_pix2 );
}
static inline int pixel_satd_16x8_altivec( uint8_t *pix1, int i_pix1,
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
    return pixel_satd_16x8_altivec( &pix1[0], i_pix1,
                                    &pix2[0], i_pix2 ) +
           pixel_satd_16x8_altivec( &pix1[8*i_pix1], i_pix1,
                                    &pix2[8*i_pix2], i_pix2 );
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
    pixf->satd[PIXEL_8x4]   = pixel_satd_8x4_altivec;
    pixf->satd[PIXEL_4x8]   = pixel_satd_4x8_altivec;
    pixf->satd[PIXEL_4x4]   = pixel_satd_4x4_altivec;
}

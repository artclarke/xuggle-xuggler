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

/***********************************************************************
 * SAD routines
 **********************************************************************/

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
        VEC_LOAD( pix1, pix1v, lx, vec_u8_t );         \
        VEC_LOAD( pix2, pix2v, lx, vec_u8_t );         \
        sumv = (vec_s32_t) vec_sum4s(                  \
                   vec_sub( vec_max( pix1v, pix2v ),   \
                            vec_min( pix1v, pix2v ) ), \
                   (vec_u32_t) sumv );                 \
        pix1 += i_pix1;                                \
        pix2 += i_pix2;                                \
    }                                                  \
    sumv = vec_sum##a( sumv, zero_s32v );              \
    sumv = vec_splat( sumv, b );                       \
    vec_ste( sumv, 0, &sum );                          \
    return sum;                                        \
}

PIXEL_SAD_ALTIVEC( pixel_sad_16x16_altivec, 16, 16, s,  3 )
PIXEL_SAD_ALTIVEC( pixel_sad_8x16_altivec,  8,  16, 2s, 1 )
PIXEL_SAD_ALTIVEC( pixel_sad_16x8_altivec,  16, 8,  s,  3 )
PIXEL_SAD_ALTIVEC( pixel_sad_8x8_altivec,   8,  8,  2s, 1 )

/***********************************************************************
 * SATD routines
 **********************************************************************/

/***********************************************************************
 * VEC_TRANSPOSE_8
 ***********************************************************************
 * Transposes a 8x8 matrix of s16 vectors
 **********************************************************************/
#define VEC_TRANSPOSE_8(a0,a1,a2,a3,a4,a5,a6,a7,b0,b1,b2,b3,b4,b5,b6,b7) \
    b0 = vec_mergeh( a0, a4 ); \
    b1 = vec_mergel( a0, a4 ); \
    b2 = vec_mergeh( a1, a5 ); \
    b3 = vec_mergel( a1, a5 ); \
    b4 = vec_mergeh( a2, a6 ); \
    b5 = vec_mergel( a2, a6 ); \
    b6 = vec_mergeh( a3, a7 ); \
    b7 = vec_mergel( a3, a7 ); \
    a0 = vec_mergeh( b0, b4 ); \
    a1 = vec_mergel( b0, b4 ); \
    a2 = vec_mergeh( b1, b5 ); \
    a3 = vec_mergel( b1, b5 ); \
    a4 = vec_mergeh( b2, b6 ); \
    a5 = vec_mergel( b2, b6 ); \
    a6 = vec_mergeh( b3, b7 ); \
    a7 = vec_mergel( b3, b7 ); \
    b0 = vec_mergeh( a0, a4 ); \
    b1 = vec_mergel( a0, a4 ); \
    b2 = vec_mergeh( a1, a5 ); \
    b3 = vec_mergel( a1, a5 ); \
    b4 = vec_mergeh( a2, a6 ); \
    b5 = vec_mergel( a2, a6 ); \
    b6 = vec_mergeh( a3, a7 ); \
    b7 = vec_mergel( a3, a7 )

/***********************************************************************
 * VEC_TRANSPOSE_4
 ***********************************************************************
 * Transposes a 4x4 matrix of s16 vectors
 **********************************************************************/
#define VEC_TRANSPOSE_4(a0,a1,a2,a3,b0,b1,b2,b3) \
    b0 = vec_mergeh( a0, a0 ); \
    b1 = vec_mergeh( a1, a0 ); \
    b2 = vec_mergeh( a2, a0 ); \
    b3 = vec_mergeh( a3, a0 ); \
    a0 = vec_mergeh( b0, b2 ); \
    a1 = vec_mergel( b0, b2 ); \
    a2 = vec_mergeh( b1, b3 ); \
    a3 = vec_mergel( b1, b3 ); \
    b0 = vec_mergeh( a0, a2 ); \
    b1 = vec_mergel( a0, a2 ); \
    b2 = vec_mergeh( a1, a3 ); \
    b3 = vec_mergel( a1, a3 )

/***********************************************************************
 * VEC_HADAMAR
 ***********************************************************************
 * b[0] = a[0] + a[1] + a[2] + a[3]
 * b[1] = a[0] + a[1] - a[2] - a[3]
 * b[2] = a[0] - a[1] - a[2] + a[3]
 * b[3] = a[0] - a[1] + a[2] - a[3]
 **********************************************************************/
#define VEC_HADAMAR(a0,a1,a2,a3,b0,b1,b2,b3) \
    b2 = vec_add( a0, a1 ); \
    b3 = vec_add( a2, a3 ); \
    a0 = vec_sub( a0, a1 ); \
    a2 = vec_sub( a2, a3 ); \
    b0 = vec_add( b2, b3 ); \
    b1 = vec_sub( b2, b3 ); \
    b2 = vec_sub( a0, a2 ); \
    b3 = vec_add( a0, a2 )

/***********************************************************************
 * VEC_DIFF_H
 ***********************************************************************
 * p1, p2:    u8 *
 * i1, i2, n: int
 * d:         s16v
 *
 * Loads n bytes from p1 and p2, do the diff of the high elements into
 * d, increments p1 and p2 by i1 and i2
 **********************************************************************/
#define PREP_DIFF           \
    LOAD_ZERO;              \
    PREP_LOAD;              \
    vec_s16_t pix1v, pix2v;

#define VEC_DIFF_H(p1,i1,p2,i2,n,d)      \
    VEC_LOAD( p1, pix1v, n, vec_s16_t ); \
    pix1v = vec_u8_to_s16( pix1v );      \
    VEC_LOAD( p2, pix2v, n, vec_s16_t ); \
    pix2v = vec_u8_to_s16( pix2v );      \
    d     = vec_sub( pix1v, pix2v );     \
    p1   += i1;                          \
    p2   += i2

/***********************************************************************
 * VEC_DIFF_HL
 ***********************************************************************
 * p1, p2: u8 *
 * i1, i2: int
 * dh, dl: s16v
 *
 * Loads 16 bytes from p1 and p2, do the diff of the high elements into
 * dh, the diff of the low elements into dl, increments p1 and p2 by i1
 * and i2
 **********************************************************************/
#define VEC_DIFF_HL(p1,i1,p2,i2,dh,dl)    \
    VEC_LOAD( p1, pix1v, 16, vec_s16_t ); \
    temp0v = vec_u8_to_s16_h( pix1v );    \
    temp1v = vec_u8_to_s16_l( pix1v );    \
    VEC_LOAD( p2, pix2v, 16, vec_s16_t ); \
    temp2v = vec_u8_to_s16_h( pix2v );    \
    temp3v = vec_u8_to_s16_l( pix2v );    \
    dh     = vec_sub( temp0v, temp2v );   \
    dl     = vec_sub( temp1v, temp3v );   \
    p1    += i1;                          \
    p2    += i2

/***********************************************************************
 * VEC_ABS
 ***********************************************************************
 * a: s16v
 *
 * a = abs(a)
 * 
 * Call vec_sub()/vec_max() instead of vec_abs() because vec_abs()
 * actually also calls vec_splat(0), but we already have a null vector.
 **********************************************************************/
#define VEC_ABS(a) \
    pix1v = vec_sub( zero_s16v, a ); \
    a     = vec_max( a, pix1v ); \

/***********************************************************************
 * VEC_ADD_ABS
 ***********************************************************************
 * a:    s16v
 * b, c: s32v
 *
 * c[i] = abs(a[2*i]) + abs(a[2*i+1]) + [bi]
 **********************************************************************/
#define VEC_ADD_ABS(a,b,c) \
    VEC_ABS( a ); \
    c = vec_sum4s( a, b )

/***********************************************************************
 * SATD 4x4
 **********************************************************************/
static int pixel_satd_4x4_altivec( uint8_t *pix1, int i_pix1,
                                   uint8_t *pix2, int i_pix2 )
{
    DECLARE_ALIGNED( int, i_satd, 16 );

    PREP_DIFF;
    vec_s16_t diff0v, diff1v, diff2v, diff3v;
    vec_s16_t temp0v, temp1v, temp2v, temp3v;
    vec_s32_t satdv;

    VEC_DIFF_H( pix1, i_pix1, pix2, i_pix2, 4, diff0v );
    VEC_DIFF_H( pix1, i_pix1, pix2, i_pix2, 4, diff1v );
    VEC_DIFF_H( pix1, i_pix1, pix2, i_pix2, 4, diff2v );
    VEC_DIFF_H( pix1, i_pix1, pix2, i_pix2, 4, diff3v );

    /* Hadamar H */
    VEC_HADAMAR( diff0v, diff1v, diff2v, diff3v,
                 temp0v, temp1v, temp2v, temp3v );
    
    VEC_TRANSPOSE_4( temp0v, temp1v, temp2v, temp3v,
                     diff0v, diff1v, diff2v, diff3v );
    /* Hadamar V */
    VEC_HADAMAR( diff0v, diff1v, diff2v, diff3v,
                 temp0v, temp1v, temp2v, temp3v );

    VEC_ADD_ABS( temp0v, zero_s32v, satdv );
    VEC_ADD_ABS( temp1v, satdv,     satdv );
    VEC_ADD_ABS( temp2v, satdv,     satdv );
    VEC_ADD_ABS( temp3v, satdv,     satdv );

    satdv = vec_sum2s( satdv, zero_s32v );
    satdv = vec_splat( satdv, 1 );
    vec_ste( satdv, 0, &i_satd );

    return i_satd / 2;
}

/***********************************************************************
 * SATD 4x8
 **********************************************************************/
static int pixel_satd_4x8_altivec( uint8_t *pix1, int i_pix1,
                                   uint8_t *pix2, int i_pix2 )
{
    DECLARE_ALIGNED( int, i_satd, 16 );

    PREP_DIFF;
    vec_s16_t diff0v, diff1v, diff2v, diff3v;
    vec_s16_t temp0v, temp1v, temp2v, temp3v;
    vec_s32_t satdv;

    VEC_DIFF_H( pix1, i_pix1, pix2, i_pix2, 4, diff0v );
    VEC_DIFF_H( pix1, i_pix1, pix2, i_pix2, 4, diff1v );
    VEC_DIFF_H( pix1, i_pix1, pix2, i_pix2, 4, diff2v );
    VEC_DIFF_H( pix1, i_pix1, pix2, i_pix2, 4, diff3v );
    VEC_HADAMAR( diff0v, diff1v, diff2v, diff3v,
                 temp0v, temp1v, temp2v, temp3v );
    VEC_TRANSPOSE_4( temp0v, temp1v, temp2v, temp3v,
                     diff0v, diff1v, diff2v, diff3v );
    VEC_HADAMAR( diff0v, diff1v, diff2v, diff3v,
                 temp0v, temp1v, temp2v, temp3v );
    VEC_ADD_ABS( temp0v, zero_s32v, satdv );
    VEC_ADD_ABS( temp1v, satdv,     satdv );
    VEC_ADD_ABS( temp2v, satdv,     satdv );
    VEC_ADD_ABS( temp3v, satdv,     satdv );

    VEC_DIFF_H( pix1, i_pix1, pix2, i_pix2, 4, diff0v );
    VEC_DIFF_H( pix1, i_pix1, pix2, i_pix2, 4, diff1v );
    VEC_DIFF_H( pix1, i_pix1, pix2, i_pix2, 4, diff2v );
    VEC_DIFF_H( pix1, i_pix1, pix2, i_pix2, 4, diff3v );
    VEC_HADAMAR( diff0v, diff1v, diff2v, diff3v,
                 temp0v, temp1v, temp2v, temp3v );
    VEC_TRANSPOSE_4( temp0v, temp1v, temp2v, temp3v,
                     diff0v, diff1v, diff2v, diff3v );
    VEC_HADAMAR( diff0v, diff1v, diff2v, diff3v,
                 temp0v, temp1v, temp2v, temp3v );
    VEC_ADD_ABS( temp0v, satdv,     satdv );
    VEC_ADD_ABS( temp1v, satdv,     satdv );
    VEC_ADD_ABS( temp2v, satdv,     satdv );
    VEC_ADD_ABS( temp3v, satdv,     satdv );

    satdv = vec_sum2s( satdv, zero_s32v );
    satdv = vec_splat( satdv, 1 );
    vec_ste( satdv, 0, &i_satd );

    return i_satd / 2;
}

/***********************************************************************
 * SATD 8x4
 **********************************************************************/
static int pixel_satd_8x4_altivec( uint8_t *pix1, int i_pix1,
                                   uint8_t *pix2, int i_pix2 )
{
    DECLARE_ALIGNED( int, i_satd, 16 );

    PREP_DIFF;
    vec_s16_t diff0v, diff1v, diff2v, diff3v,
              diff4v, diff5v, diff6v, diff7v;
    vec_s16_t temp0v, temp1v, temp2v, temp3v,
              temp4v, temp5v, temp6v, temp7v;
    vec_s32_t satdv;

    VEC_DIFF_H( pix1, i_pix1, pix2, i_pix2, 8, diff0v );
    VEC_DIFF_H( pix1, i_pix1, pix2, i_pix2, 8, diff1v );
    VEC_DIFF_H( pix1, i_pix1, pix2, i_pix2, 8, diff2v );
    VEC_DIFF_H( pix1, i_pix1, pix2, i_pix2, 8, diff3v );

    VEC_HADAMAR( diff0v, diff1v, diff2v, diff3v,
                 temp0v, temp1v, temp2v, temp3v );
    /* This causes warnings because temp4v...temp7v haven't be set,
       but we don't care */
    VEC_TRANSPOSE_8( temp0v, temp1v, temp2v, temp3v,
                     temp4v, temp5v, temp6v, temp7v,
                     diff0v, diff1v, diff2v, diff3v,
                     diff4v, diff5v, diff6v, diff7v );
    VEC_HADAMAR( diff0v, diff1v, diff2v, diff3v,
                 temp0v, temp1v, temp2v, temp3v );
    VEC_HADAMAR( diff4v, diff5v, diff6v, diff7v,
                 temp4v, temp5v, temp6v, temp7v );

    VEC_ADD_ABS( temp0v, zero_s32v, satdv );
    VEC_ADD_ABS( temp1v, satdv,     satdv );
    VEC_ADD_ABS( temp2v, satdv,     satdv );
    VEC_ADD_ABS( temp3v, satdv,     satdv );
    VEC_ADD_ABS( temp4v, satdv,     satdv );
    VEC_ADD_ABS( temp5v, satdv,     satdv );
    VEC_ADD_ABS( temp6v, satdv,     satdv );
    VEC_ADD_ABS( temp7v, satdv,     satdv );

    satdv = vec_sum2s( satdv, zero_s32v );
    satdv = vec_splat( satdv, 1 );
    vec_ste( satdv, 0, &i_satd );

    return i_satd / 2;
}

/***********************************************************************
 * SATD 8x8
 **********************************************************************/
static int pixel_satd_8x8_altivec( uint8_t *pix1, int i_pix1,
                                   uint8_t *pix2, int i_pix2 )
{
    DECLARE_ALIGNED( int, i_satd, 16 );

    PREP_DIFF;
    vec_s16_t diff0v, diff1v, diff2v, diff3v,
              diff4v, diff5v, diff6v, diff7v;
    vec_s16_t temp0v, temp1v, temp2v, temp3v,
              temp4v, temp5v, temp6v, temp7v;
    vec_s32_t satdv;

    VEC_DIFF_H( pix1, i_pix1, pix2, i_pix2, 8, diff0v );
    VEC_DIFF_H( pix1, i_pix1, pix2, i_pix2, 8, diff1v );
    VEC_DIFF_H( pix1, i_pix1, pix2, i_pix2, 8, diff2v );
    VEC_DIFF_H( pix1, i_pix1, pix2, i_pix2, 8, diff3v );
    VEC_DIFF_H( pix1, i_pix1, pix2, i_pix2, 8, diff4v );
    VEC_DIFF_H( pix1, i_pix1, pix2, i_pix2, 8, diff5v );
    VEC_DIFF_H( pix1, i_pix1, pix2, i_pix2, 8, diff6v );
    VEC_DIFF_H( pix1, i_pix1, pix2, i_pix2, 8, diff7v );

    VEC_HADAMAR( diff0v, diff1v, diff2v, diff3v,
                 temp0v, temp1v, temp2v, temp3v );
    VEC_HADAMAR( diff4v, diff5v, diff6v, diff7v,
                 temp4v, temp5v, temp6v, temp7v );

    VEC_TRANSPOSE_8( temp0v, temp1v, temp2v, temp3v,
                     temp4v, temp5v, temp6v, temp7v,
                     diff0v, diff1v, diff2v, diff3v,
                     diff4v, diff5v, diff6v, diff7v );

    VEC_HADAMAR( diff0v, diff1v, diff2v, diff3v,
                 temp0v, temp1v, temp2v, temp3v );
    VEC_HADAMAR( diff4v, diff5v, diff6v, diff7v,
                 temp4v, temp5v, temp6v, temp7v );

    VEC_ADD_ABS( temp0v, zero_s32v, satdv );
    VEC_ADD_ABS( temp1v, satdv,     satdv );
    VEC_ADD_ABS( temp2v, satdv,     satdv );
    VEC_ADD_ABS( temp3v, satdv,     satdv );
    VEC_ADD_ABS( temp4v, satdv,     satdv );
    VEC_ADD_ABS( temp5v, satdv,     satdv );
    VEC_ADD_ABS( temp6v, satdv,     satdv );
    VEC_ADD_ABS( temp7v, satdv,     satdv );

    satdv = vec_sums( satdv, zero_s32v );
    satdv = vec_splat( satdv, 3 );
    vec_ste( satdv, 0, &i_satd );

    return i_satd / 2;
}

/***********************************************************************
 * SATD 8x16
 **********************************************************************/
static int pixel_satd_8x16_altivec( uint8_t *pix1, int i_pix1,
                                    uint8_t *pix2, int i_pix2 )
{
    DECLARE_ALIGNED( int, i_satd, 16 );

    PREP_DIFF;
    vec_s16_t diff0v, diff1v, diff2v, diff3v,
              diff4v, diff5v, diff6v, diff7v;
    vec_s16_t temp0v, temp1v, temp2v, temp3v,
              temp4v, temp5v, temp6v, temp7v;
    vec_s32_t satdv;

    VEC_DIFF_H( pix1, i_pix1, pix2, i_pix2, 8, diff0v );
    VEC_DIFF_H( pix1, i_pix1, pix2, i_pix2, 8, diff1v );
    VEC_DIFF_H( pix1, i_pix1, pix2, i_pix2, 8, diff2v );
    VEC_DIFF_H( pix1, i_pix1, pix2, i_pix2, 8, diff3v );
    VEC_DIFF_H( pix1, i_pix1, pix2, i_pix2, 8, diff4v );
    VEC_DIFF_H( pix1, i_pix1, pix2, i_pix2, 8, diff5v );
    VEC_DIFF_H( pix1, i_pix1, pix2, i_pix2, 8, diff6v );
    VEC_DIFF_H( pix1, i_pix1, pix2, i_pix2, 8, diff7v );
    VEC_HADAMAR( diff0v, diff1v, diff2v, diff3v,
                 temp0v, temp1v, temp2v, temp3v );
    VEC_HADAMAR( diff4v, diff5v, diff6v, diff7v,
                 temp4v, temp5v, temp6v, temp7v );
    VEC_TRANSPOSE_8( temp0v, temp1v, temp2v, temp3v,
                     temp4v, temp5v, temp6v, temp7v,
                     diff0v, diff1v, diff2v, diff3v,
                     diff4v, diff5v, diff6v, diff7v );
    VEC_HADAMAR( diff0v, diff1v, diff2v, diff3v,
                 temp0v, temp1v, temp2v, temp3v );
    VEC_HADAMAR( diff4v, diff5v, diff6v, diff7v,
                 temp4v, temp5v, temp6v, temp7v );
    VEC_ADD_ABS( temp0v, zero_s32v, satdv );
    VEC_ADD_ABS( temp1v, satdv,     satdv );
    VEC_ADD_ABS( temp2v, satdv,     satdv );
    VEC_ADD_ABS( temp3v, satdv,     satdv );
    VEC_ADD_ABS( temp4v, satdv,     satdv );
    VEC_ADD_ABS( temp5v, satdv,     satdv );
    VEC_ADD_ABS( temp6v, satdv,     satdv );
    VEC_ADD_ABS( temp7v, satdv,     satdv );

    VEC_DIFF_H( pix1, i_pix1, pix2, i_pix2, 8, diff0v );
    VEC_DIFF_H( pix1, i_pix1, pix2, i_pix2, 8, diff1v );
    VEC_DIFF_H( pix1, i_pix1, pix2, i_pix2, 8, diff2v );
    VEC_DIFF_H( pix1, i_pix1, pix2, i_pix2, 8, diff3v );
    VEC_DIFF_H( pix1, i_pix1, pix2, i_pix2, 8, diff4v );
    VEC_DIFF_H( pix1, i_pix1, pix2, i_pix2, 8, diff5v );
    VEC_DIFF_H( pix1, i_pix1, pix2, i_pix2, 8, diff6v );
    VEC_DIFF_H( pix1, i_pix1, pix2, i_pix2, 8, diff7v );
    VEC_HADAMAR( diff0v, diff1v, diff2v, diff3v,
                 temp0v, temp1v, temp2v, temp3v );
    VEC_HADAMAR( diff4v, diff5v, diff6v, diff7v,
                 temp4v, temp5v, temp6v, temp7v );
    VEC_TRANSPOSE_8( temp0v, temp1v, temp2v, temp3v,
                     temp4v, temp5v, temp6v, temp7v,
                     diff0v, diff1v, diff2v, diff3v,
                     diff4v, diff5v, diff6v, diff7v );
    VEC_HADAMAR( diff0v, diff1v, diff2v, diff3v,
                 temp0v, temp1v, temp2v, temp3v );
    VEC_HADAMAR( diff4v, diff5v, diff6v, diff7v,
                 temp4v, temp5v, temp6v, temp7v );
    VEC_ADD_ABS( temp0v, satdv,     satdv );
    VEC_ADD_ABS( temp1v, satdv,     satdv );
    VEC_ADD_ABS( temp2v, satdv,     satdv );
    VEC_ADD_ABS( temp3v, satdv,     satdv );
    VEC_ADD_ABS( temp4v, satdv,     satdv );
    VEC_ADD_ABS( temp5v, satdv,     satdv );
    VEC_ADD_ABS( temp6v, satdv,     satdv );
    VEC_ADD_ABS( temp7v, satdv,     satdv );

    satdv = vec_sums( satdv, zero_s32v );
    satdv = vec_splat( satdv, 3 );
    vec_ste( satdv, 0, &i_satd );

    return i_satd / 2;
}

/***********************************************************************
 * SATD 16x8
 **********************************************************************/
static int pixel_satd_16x8_altivec( uint8_t *pix1, int i_pix1,
                                    uint8_t *pix2, int i_pix2 )
{
    DECLARE_ALIGNED( int, i_satd, 16 );

    LOAD_ZERO;
    PREP_LOAD;
    vec_s32_t satdv;
    vec_s16_t pix1v, pix2v;
    vec_s16_t diffh0v, diffh1v, diffh2v, diffh3v,
              diffh4v, diffh5v, diffh6v, diffh7v;
    vec_s16_t diffl0v, diffl1v, diffl2v, diffl3v,
              diffl4v, diffl5v, diffl6v, diffl7v;
    vec_s16_t temp0v, temp1v, temp2v, temp3v,
              temp4v, temp5v, temp6v, temp7v;

    VEC_DIFF_HL( pix1, i_pix1, pix2, i_pix2, diffh0v, diffl0v );
    VEC_DIFF_HL( pix1, i_pix1, pix2, i_pix2, diffh1v, diffl1v );
    VEC_DIFF_HL( pix1, i_pix1, pix2, i_pix2, diffh2v, diffl2v );
    VEC_DIFF_HL( pix1, i_pix1, pix2, i_pix2, diffh3v, diffl3v );
    VEC_DIFF_HL( pix1, i_pix1, pix2, i_pix2, diffh4v, diffl4v );
    VEC_DIFF_HL( pix1, i_pix1, pix2, i_pix2, diffh5v, diffl5v );
    VEC_DIFF_HL( pix1, i_pix1, pix2, i_pix2, diffh6v, diffl6v );
    VEC_DIFF_HL( pix1, i_pix1, pix2, i_pix2, diffh7v, diffl7v );

    VEC_HADAMAR( diffh0v, diffh1v, diffh2v, diffh3v,
                 temp0v, temp1v, temp2v, temp3v );
    VEC_HADAMAR( diffh4v, diffh5v, diffh6v, diffh7v,
                 temp4v, temp5v, temp6v, temp7v );

    VEC_TRANSPOSE_8( temp0v, temp1v, temp2v, temp3v,
                     temp4v, temp5v, temp6v, temp7v,
                     diffh0v, diffh1v, diffh2v, diffh3v,
                     diffh4v, diffh5v, diffh6v, diffh7v );

    VEC_HADAMAR( diffh0v, diffh1v, diffh2v, diffh3v,
                 temp0v, temp1v, temp2v, temp3v );
    VEC_HADAMAR( diffh4v, diffh5v, diffh6v, diffh7v,
                 temp4v, temp5v, temp6v, temp7v );

    VEC_ADD_ABS( temp0v, zero_s32v, satdv );
    VEC_ADD_ABS( temp1v, satdv,     satdv );
    VEC_ADD_ABS( temp2v, satdv,     satdv );
    VEC_ADD_ABS( temp3v, satdv,     satdv );
    VEC_ADD_ABS( temp4v, satdv,     satdv );
    VEC_ADD_ABS( temp5v, satdv,     satdv );
    VEC_ADD_ABS( temp6v, satdv,     satdv );
    VEC_ADD_ABS( temp7v, satdv,     satdv );

    VEC_HADAMAR( diffl0v, diffl1v, diffl2v, diffl3v,
                 temp0v, temp1v, temp2v, temp3v );
    VEC_HADAMAR( diffl4v, diffl5v, diffl6v, diffl7v,
                 temp4v, temp5v, temp6v, temp7v );

    VEC_TRANSPOSE_8( temp0v, temp1v, temp2v, temp3v,
                     temp4v, temp5v, temp6v, temp7v,
                     diffl0v, diffl1v, diffl2v, diffl3v,
                     diffl4v, diffl5v, diffl6v, diffl7v );

    VEC_HADAMAR( diffl0v, diffl1v, diffl2v, diffl3v,
                 temp0v, temp1v, temp2v, temp3v );
    VEC_HADAMAR( diffl4v, diffl5v, diffl6v, diffl7v,
                 temp4v, temp5v, temp6v, temp7v );

    VEC_ADD_ABS( temp0v, satdv,     satdv );
    VEC_ADD_ABS( temp1v, satdv,     satdv );
    VEC_ADD_ABS( temp2v, satdv,     satdv );
    VEC_ADD_ABS( temp3v, satdv,     satdv );
    VEC_ADD_ABS( temp4v, satdv,     satdv );
    VEC_ADD_ABS( temp5v, satdv,     satdv );
    VEC_ADD_ABS( temp6v, satdv,     satdv );
    VEC_ADD_ABS( temp7v, satdv,     satdv );

    satdv = vec_sums( satdv, zero_s32v );
    satdv = vec_splat( satdv, 3 );
    vec_ste( satdv, 0, &i_satd );

    return i_satd / 2;
}

/***********************************************************************
 * SATD 16x16
 **********************************************************************/
static int pixel_satd_16x16_altivec( uint8_t *pix1, int i_pix1,
                                     uint8_t *pix2, int i_pix2 )
{
    DECLARE_ALIGNED( int, i_satd, 16 );

    LOAD_ZERO;
    PREP_LOAD;
    vec_s32_t satdv;
    vec_s16_t pix1v, pix2v;
    vec_s16_t diffh0v, diffh1v, diffh2v, diffh3v,
              diffh4v, diffh5v, diffh6v, diffh7v;
    vec_s16_t diffl0v, diffl1v, diffl2v, diffl3v,
              diffl4v, diffl5v, diffl6v, diffl7v;
    vec_s16_t temp0v, temp1v, temp2v, temp3v,
              temp4v, temp5v, temp6v, temp7v;

    VEC_DIFF_HL( pix1, i_pix1, pix2, i_pix2, diffh0v, diffl0v );
    VEC_DIFF_HL( pix1, i_pix1, pix2, i_pix2, diffh1v, diffl1v );
    VEC_DIFF_HL( pix1, i_pix1, pix2, i_pix2, diffh2v, diffl2v );
    VEC_DIFF_HL( pix1, i_pix1, pix2, i_pix2, diffh3v, diffl3v );
    VEC_DIFF_HL( pix1, i_pix1, pix2, i_pix2, diffh4v, diffl4v );
    VEC_DIFF_HL( pix1, i_pix1, pix2, i_pix2, diffh5v, diffl5v );
    VEC_DIFF_HL( pix1, i_pix1, pix2, i_pix2, diffh6v, diffl6v );
    VEC_DIFF_HL( pix1, i_pix1, pix2, i_pix2, diffh7v, diffl7v );
    VEC_HADAMAR( diffh0v, diffh1v, diffh2v, diffh3v,
                 temp0v, temp1v, temp2v, temp3v );
    VEC_HADAMAR( diffh4v, diffh5v, diffh6v, diffh7v,
                 temp4v, temp5v, temp6v, temp7v );
    VEC_TRANSPOSE_8( temp0v, temp1v, temp2v, temp3v,
                     temp4v, temp5v, temp6v, temp7v,
                     diffh0v, diffh1v, diffh2v, diffh3v,
                     diffh4v, diffh5v, diffh6v, diffh7v );
    VEC_HADAMAR( diffh0v, diffh1v, diffh2v, diffh3v,
                 temp0v, temp1v, temp2v, temp3v );
    VEC_HADAMAR( diffh4v, diffh5v, diffh6v, diffh7v,
                 temp4v, temp5v, temp6v, temp7v );
    VEC_ADD_ABS( temp0v, zero_s32v, satdv );
    VEC_ADD_ABS( temp1v, satdv,     satdv );
    VEC_ADD_ABS( temp2v, satdv,     satdv );
    VEC_ADD_ABS( temp3v, satdv,     satdv );
    VEC_ADD_ABS( temp4v, satdv,     satdv );
    VEC_ADD_ABS( temp5v, satdv,     satdv );
    VEC_ADD_ABS( temp6v, satdv,     satdv );
    VEC_ADD_ABS( temp7v, satdv,     satdv );
    VEC_HADAMAR( diffl0v, diffl1v, diffl2v, diffl3v,
                 temp0v, temp1v, temp2v, temp3v );
    VEC_HADAMAR( diffl4v, diffl5v, diffl6v, diffl7v,
                 temp4v, temp5v, temp6v, temp7v );
    VEC_TRANSPOSE_8( temp0v, temp1v, temp2v, temp3v,
                     temp4v, temp5v, temp6v, temp7v,
                     diffl0v, diffl1v, diffl2v, diffl3v,
                     diffl4v, diffl5v, diffl6v, diffl7v );
    VEC_HADAMAR( diffl0v, diffl1v, diffl2v, diffl3v,
                 temp0v, temp1v, temp2v, temp3v );
    VEC_HADAMAR( diffl4v, diffl5v, diffl6v, diffl7v,
                 temp4v, temp5v, temp6v, temp7v );
    VEC_ADD_ABS( temp0v, satdv,     satdv );
    VEC_ADD_ABS( temp1v, satdv,     satdv );
    VEC_ADD_ABS( temp2v, satdv,     satdv );
    VEC_ADD_ABS( temp3v, satdv,     satdv );
    VEC_ADD_ABS( temp4v, satdv,     satdv );
    VEC_ADD_ABS( temp5v, satdv,     satdv );
    VEC_ADD_ABS( temp6v, satdv,     satdv );
    VEC_ADD_ABS( temp7v, satdv,     satdv );

    VEC_DIFF_HL( pix1, i_pix1, pix2, i_pix2, diffh0v, diffl0v );
    VEC_DIFF_HL( pix1, i_pix1, pix2, i_pix2, diffh1v, diffl1v );
    VEC_DIFF_HL( pix1, i_pix1, pix2, i_pix2, diffh2v, diffl2v );
    VEC_DIFF_HL( pix1, i_pix1, pix2, i_pix2, diffh3v, diffl3v );
    VEC_DIFF_HL( pix1, i_pix1, pix2, i_pix2, diffh4v, diffl4v );
    VEC_DIFF_HL( pix1, i_pix1, pix2, i_pix2, diffh5v, diffl5v );
    VEC_DIFF_HL( pix1, i_pix1, pix2, i_pix2, diffh6v, diffl6v );
    VEC_DIFF_HL( pix1, i_pix1, pix2, i_pix2, diffh7v, diffl7v );
    VEC_HADAMAR( diffh0v, diffh1v, diffh2v, diffh3v,
                 temp0v, temp1v, temp2v, temp3v );
    VEC_HADAMAR( diffh4v, diffh5v, diffh6v, diffh7v,
                 temp4v, temp5v, temp6v, temp7v );
    VEC_TRANSPOSE_8( temp0v, temp1v, temp2v, temp3v,
                     temp4v, temp5v, temp6v, temp7v,
                     diffh0v, diffh1v, diffh2v, diffh3v,
                     diffh4v, diffh5v, diffh6v, diffh7v );
    VEC_HADAMAR( diffh0v, diffh1v, diffh2v, diffh3v,
                 temp0v, temp1v, temp2v, temp3v );
    VEC_HADAMAR( diffh4v, diffh5v, diffh6v, diffh7v,
                 temp4v, temp5v, temp6v, temp7v );
    VEC_ADD_ABS( temp0v, satdv,     satdv );
    VEC_ADD_ABS( temp1v, satdv,     satdv );
    VEC_ADD_ABS( temp2v, satdv,     satdv );
    VEC_ADD_ABS( temp3v, satdv,     satdv );
    VEC_ADD_ABS( temp4v, satdv,     satdv );
    VEC_ADD_ABS( temp5v, satdv,     satdv );
    VEC_ADD_ABS( temp6v, satdv,     satdv );
    VEC_ADD_ABS( temp7v, satdv,     satdv );
    VEC_HADAMAR( diffl0v, diffl1v, diffl2v, diffl3v,
                 temp0v, temp1v, temp2v, temp3v );
    VEC_HADAMAR( diffl4v, diffl5v, diffl6v, diffl7v,
                 temp4v, temp5v, temp6v, temp7v );
    VEC_TRANSPOSE_8( temp0v, temp1v, temp2v, temp3v,
                     temp4v, temp5v, temp6v, temp7v,
                     diffl0v, diffl1v, diffl2v, diffl3v,
                     diffl4v, diffl5v, diffl6v, diffl7v );
    VEC_HADAMAR( diffl0v, diffl1v, diffl2v, diffl3v,
                 temp0v, temp1v, temp2v, temp3v );
    VEC_HADAMAR( diffl4v, diffl5v, diffl6v, diffl7v,
                 temp4v, temp5v, temp6v, temp7v );
    VEC_ADD_ABS( temp0v, satdv,     satdv );
    VEC_ADD_ABS( temp1v, satdv,     satdv );
    VEC_ADD_ABS( temp2v, satdv,     satdv );
    VEC_ADD_ABS( temp3v, satdv,     satdv );
    VEC_ADD_ABS( temp4v, satdv,     satdv );
    VEC_ADD_ABS( temp5v, satdv,     satdv );
    VEC_ADD_ABS( temp6v, satdv,     satdv );
    VEC_ADD_ABS( temp7v, satdv,     satdv );

    satdv = vec_sums( satdv, zero_s32v );
    satdv = vec_splat( satdv, 3 );
    vec_ste( satdv, 0, &i_satd );

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
    pixf->satd[PIXEL_8x4]   = pixel_satd_8x4_altivec;
    pixf->satd[PIXEL_4x8]   = pixel_satd_4x8_altivec;
    pixf->satd[PIXEL_4x4]   = pixel_satd_4x4_altivec;
}

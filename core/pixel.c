/*****************************************************************************
 * pixel.c: h264 encoder
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar
 * $Id: pixel.c,v 1.1 2004/06/03 19:27:07 fenrir Exp $
 *
 * Authors: Laurent Aimar <fenrir@via.ecp.fr>
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

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "../x264.h"
#include "pixel.h"

#ifdef HAVE_MMXEXT
#   include "i386/pixel.h"
#endif
#ifdef HAVE_ALTIVEC
#   include "ppc/pixel.h"
#endif


/****************************************************************************
 * pixel_sad_WxH
 ****************************************************************************/
#define PIXEL_SAD_C( name, lx, ly ) \
static int name( uint8_t *pix1, int i_stride_pix1,  \
                 uint8_t *pix2, int i_stride_pix2 ) \
{                                                   \
    int i_sum = 0;                                  \
    int x, y;                                       \
    for( y = 0; y < ly; y++ )                       \
    {                                               \
        for( x = 0; x < lx; x++ )                   \
        {                                           \
            i_sum += abs( pix1[x] - pix2[x] );      \
        }                                           \
        pix1 += i_stride_pix1;                      \
        pix2 += i_stride_pix2;                      \
    }                                               \
    return i_sum;                                   \
}


PIXEL_SAD_C( pixel_sad_16x16, 16, 16 )
PIXEL_SAD_C( pixel_sad_16x8,  16,  8 )
PIXEL_SAD_C( pixel_sad_8x16,   8, 16 )
PIXEL_SAD_C( pixel_sad_8x8,    8,  8 )
PIXEL_SAD_C( pixel_sad_8x4,    8,  4 )
PIXEL_SAD_C( pixel_sad_4x8,    4,  8 )
PIXEL_SAD_C( pixel_sad_4x4,    4,  4 )

static void pixel_sub_4x4( int16_t diff[4][4], uint8_t *pix1, int i_pix1, uint8_t *pix2, int i_pix2 )
{
    int y, x;
    for( y = 0; y < 4; y++ )
    {
        for( x = 0; x < 4; x++ )
        {
            diff[y][x] = pix1[x] - pix2[x];
        }
        pix1 += i_pix1;
        pix2 += i_pix2;
    }
}

static int pixel_satd_wxh( uint8_t *pix1, int i_pix1, uint8_t *pix2, int i_pix2, int i_width, int i_height )
{
    int16_t tmp[4][4];
    int16_t diff[4][4];
    int x, y;
    int i_satd = 0;

    for( y = 0; y < i_height; y += 4 )
    {
        for( x = 0; x < i_width; x += 4 )
        {
            int d;

            pixel_sub_4x4( diff, &pix1[x], i_pix1, &pix2[x], i_pix2 );

            for( d = 0; d < 4; d++ )
            {
                int s01, s23;
                int d01, d23;

                s01 = diff[d][0] + diff[d][1]; s23 = diff[d][2] + diff[d][3];
                d01 = diff[d][0] - diff[d][1]; d23 = diff[d][2] - diff[d][3];

                tmp[d][0] = s01 + s23;
                tmp[d][1] = s01 - s23;
                tmp[d][2] = d01 - d23;
                tmp[d][3] = d01 + d23;
            }
            for( d = 0; d < 4; d++ )
            {
                int s01, s23;
                int d01, d23;

                s01 = tmp[0][d] + tmp[1][d]; s23 = tmp[2][d] + tmp[3][d];
                d01 = tmp[0][d] - tmp[1][d]; d23 = tmp[2][d] - tmp[3][d];

                i_satd += abs( s01 + s23 ) + abs( s01 - s23 ) + abs( d01 - d23 ) + abs( d01 + d23 );
            }

        }
        pix1 += 4 * i_pix1;
        pix2 += 4 * i_pix2;
    }

    return i_satd / 2;
}
#define PIXEL_SATD_C( name, width, height ) \
static int name( uint8_t *pix1, int i_stride_pix1, \
                 uint8_t *pix2, int i_stride_pix2 ) \
{ \
    return pixel_satd_wxh( pix1, i_stride_pix1, pix2, i_stride_pix2, width, height ); \
}
PIXEL_SATD_C( pixel_satd_16x16, 16, 16 )
PIXEL_SATD_C( pixel_satd_16x8,  16, 8 )
PIXEL_SATD_C( pixel_satd_8x16,  8, 16 )
PIXEL_SATD_C( pixel_satd_8x8,   8, 8 )
PIXEL_SATD_C( pixel_satd_8x4,   8, 4 )
PIXEL_SATD_C( pixel_satd_4x8,   4, 8 )
PIXEL_SATD_C( pixel_satd_4x4,   4, 4 )


static inline void pixel_avg_wxh( uint8_t *dst, int i_dst, uint8_t *src, int i_src, int width, int height )
{
    int x, y;
    for( y = 0; y < height; y++ )
    {
        for( x = 0; x < width; x++ )
        {
            dst[x] = ( dst[x] + src[x] + 1 ) >> 1;
        }
        dst += i_dst;
        src += i_src;
    }
}


#define PIXEL_AVG_C( name, width, height ) \
static void name( uint8_t *pix1, int i_stride_pix1, \
                  uint8_t *pix2, int i_stride_pix2 ) \
{ \
    pixel_avg_wxh( pix1, i_stride_pix1, pix2, i_stride_pix2, width, height ); \
}
PIXEL_AVG_C( pixel_avg_16x16, 16, 16 )
PIXEL_AVG_C( pixel_avg_16x8,  16, 8 )
PIXEL_AVG_C( pixel_avg_8x16,  8, 16 )
PIXEL_AVG_C( pixel_avg_8x8,   8, 8 )
PIXEL_AVG_C( pixel_avg_8x4,   8, 4 )
PIXEL_AVG_C( pixel_avg_4x8,   4, 8 )
PIXEL_AVG_C( pixel_avg_4x4,   4, 4 )

/****************************************************************************
 * x264_pixel_init:
 ****************************************************************************/
void x264_pixel_init( int cpu, x264_pixel_function_t *pixf )
{
    pixf->sad[PIXEL_16x16] = pixel_sad_16x16;
    pixf->sad[PIXEL_16x8]  = pixel_sad_16x8;
    pixf->sad[PIXEL_8x16]  = pixel_sad_8x16;
    pixf->sad[PIXEL_8x8]   = pixel_sad_8x8;
    pixf->sad[PIXEL_8x4]   = pixel_sad_8x4;
    pixf->sad[PIXEL_4x8]   = pixel_sad_4x8;
    pixf->sad[PIXEL_4x4]   = pixel_sad_4x4;

    pixf->satd[PIXEL_16x16]= pixel_satd_16x16;
    pixf->satd[PIXEL_16x8] = pixel_satd_16x8;
    pixf->satd[PIXEL_8x16] = pixel_satd_8x16;
    pixf->satd[PIXEL_8x8]  = pixel_satd_8x8;
    pixf->satd[PIXEL_8x4]  = pixel_satd_8x4;
    pixf->satd[PIXEL_4x8]  = pixel_satd_4x8;
    pixf->satd[PIXEL_4x4]  = pixel_satd_4x4;

    pixf->avg[PIXEL_16x16]= pixel_avg_16x16;
    pixf->avg[PIXEL_16x8] = pixel_avg_16x8;
    pixf->avg[PIXEL_8x16] = pixel_avg_8x16;
    pixf->avg[PIXEL_8x8]  = pixel_avg_8x8;
    pixf->avg[PIXEL_8x4]  = pixel_avg_8x4;
    pixf->avg[PIXEL_4x8]  = pixel_avg_4x8;
    pixf->avg[PIXEL_4x4]  = pixel_avg_4x4;
#ifdef HAVE_MMXEXT
    if( cpu&X264_CPU_MMXEXT )
    {
        pixf->sad[PIXEL_16x16] = x264_pixel_sad_16x16_mmxext;
        pixf->sad[PIXEL_16x8 ] = x264_pixel_sad_16x8_mmxext;
        pixf->sad[PIXEL_8x16 ] = x264_pixel_sad_8x16_mmxext;
        pixf->sad[PIXEL_8x8  ] = x264_pixel_sad_8x8_mmxext;
        pixf->sad[PIXEL_8x4  ] = x264_pixel_sad_8x4_mmxext;
        pixf->sad[PIXEL_4x8  ] = x264_pixel_sad_4x8_mmxext;
        pixf->sad[PIXEL_4x4]   = x264_pixel_sad_4x4_mmxext;

        pixf->satd[PIXEL_16x16]= x264_pixel_satd_16x16_mmxext;
        pixf->satd[PIXEL_16x8] = x264_pixel_satd_16x8_mmxext;
        pixf->satd[PIXEL_8x16] = x264_pixel_satd_8x16_mmxext;
        pixf->satd[PIXEL_8x8]  = x264_pixel_satd_8x8_mmxext;
        pixf->satd[PIXEL_8x4]  = x264_pixel_satd_8x4_mmxext;
        pixf->satd[PIXEL_4x8]  = x264_pixel_satd_4x8_mmxext;
        pixf->satd[PIXEL_4x4]  = x264_pixel_satd_4x4_mmxext;
    }
#endif
#ifdef HAVE_ALTIVEC
    if( cpu&X264_CPU_ALTIVEC )
    {
        x264_pixel_altivec_init( pixf );
    }
#endif
}


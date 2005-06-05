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

#ifdef HAVE_STDINT_H
#include <stdint.h>
#else
#include <inttypes.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "x264.h"
#include "pixel.h"
#include "clip1.h"

#ifdef HAVE_MMXEXT
#   include "i386/pixel.h"
#endif
#ifdef ARCH_PPC
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


/****************************************************************************
 * pixel_ssd_WxH
 ****************************************************************************/
#define PIXEL_SSD_C( name, lx, ly ) \
static int name( uint8_t *pix1, int i_stride_pix1,  \
                 uint8_t *pix2, int i_stride_pix2 ) \
{                                                   \
    int i_sum = 0;                                  \
    int x, y;                                       \
    for( y = 0; y < ly; y++ )                       \
    {                                               \
        for( x = 0; x < lx; x++ )                   \
        {                                           \
            int d = pix1[x] - pix2[x];              \
            i_sum += d*d;                           \
        }                                           \
        pix1 += i_stride_pix1;                      \
        pix2 += i_stride_pix2;                      \
    }                                               \
    return i_sum;                                   \
}

PIXEL_SSD_C( pixel_ssd_16x16, 16, 16 )
PIXEL_SSD_C( pixel_ssd_16x8,  16,  8 )
PIXEL_SSD_C( pixel_ssd_8x16,   8, 16 )
PIXEL_SSD_C( pixel_ssd_8x8,    8,  8 )
PIXEL_SSD_C( pixel_ssd_8x4,    8,  4 )
PIXEL_SSD_C( pixel_ssd_4x8,    4,  8 )
PIXEL_SSD_C( pixel_ssd_4x4,    4,  4 )


static inline void pixel_sub_wxh( int16_t *diff, int i_size,
                                  uint8_t *pix1, int i_pix1, uint8_t *pix2, int i_pix2 )
{
    int y, x;
    for( y = 0; y < i_size; y++ )
    {
        for( x = 0; x < i_size; x++ )
        {
            diff[x + y*i_size] = pix1[x] - pix2[x];
        }
        pix1 += i_pix1;
        pix2 += i_pix2;
    }
}


/****************************************************************************
 * pixel_satd_WxH: sum of 4x4 Hadamard transformed differences
 ****************************************************************************/
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

            pixel_sub_wxh( (int16_t*)diff, 4, &pix1[x], i_pix1, &pix2[x], i_pix2 );

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


/****************************************************************************
 * pixel_sa8d_WxH: sum of 8x8 Hadamard transformed differences
 ****************************************************************************/
#define SA8D_1D {\
    const int a0 = SRC(0) + SRC(4);\
    const int a4 = SRC(0) - SRC(4);\
    const int a1 = SRC(1) + SRC(5);\
    const int a5 = SRC(1) - SRC(5);\
    const int a2 = SRC(2) + SRC(6);\
    const int a6 = SRC(2) - SRC(6);\
    const int a3 = SRC(3) + SRC(7);\
    const int a7 = SRC(3) - SRC(7);\
    const int b0 = a0 + a2;\
    const int b2 = a0 - a2;\
    const int b1 = a1 + a3;\
    const int b3 = a1 - a3;\
    const int b4 = a4 + a6;\
    const int b6 = a4 - a6;\
    const int b5 = a5 + a7;\
    const int b7 = a5 - a7;\
    DST(0, b0 + b1);\
    DST(1, b0 - b1);\
    DST(2, b2 + b3);\
    DST(3, b2 - b3);\
    DST(4, b4 + b5);\
    DST(5, b4 - b5);\
    DST(6, b6 + b7);\
    DST(7, b6 - b7);\
}

static inline int pixel_sa8d_wxh( uint8_t *pix1, int i_pix1, uint8_t *pix2, int i_pix2,
                                  int i_width, int i_height )
{
    int16_t diff[8][8];
    int i_satd = 0;
    int x, y;

    for( y = 0; y < i_height; y += 8 )
    {
        for( x = 0; x < i_width; x += 8 )
        {
            int i;
            pixel_sub_wxh( (int16_t*)diff, 8, pix1+x, i_pix1, pix2+x, i_pix2 );

#define SRC(x)     diff[i][x]
#define DST(x,rhs) diff[i][x] = (rhs)
            for( i = 0; i < 8; i++ )
                SA8D_1D
#undef SRC
#undef DST

#define SRC(x)     diff[x][i]
#define DST(x,rhs) i_satd += abs(rhs)
            for( i = 0; i < 8; i++ )
                SA8D_1D
#undef SRC
#undef DST
        }
        pix1 += 8 * i_pix1;
        pix2 += 8 * i_pix2;
    }

    return i_satd;
}

#define PIXEL_SA8D_C( width, height ) \
static int pixel_sa8d_##width##x##height( uint8_t *pix1, int i_stride_pix1, \
                 uint8_t *pix2, int i_stride_pix2 ) \
{ \
    return ( pixel_sa8d_wxh( pix1, i_stride_pix1, pix2, i_stride_pix2, width, height ) + 2 ) >> 2; \
}
PIXEL_SA8D_C( 16, 16 )
PIXEL_SA8D_C( 16, 8 )
PIXEL_SA8D_C( 8, 16 )
PIXEL_SA8D_C( 8, 8 )


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
PIXEL_AVG_C( pixel_avg_4x2,   4, 2 )
PIXEL_AVG_C( pixel_avg_2x4,   2, 4 )
PIXEL_AVG_C( pixel_avg_2x2,   2, 2 )


/* Implicit weighted bipred only:
 * assumes log2_denom = 5, offset = 0, weight1 + weight2 = 64 */
#define op_scale2(x) dst[x] = x264_clip_uint8( (dst[x]*i_weight1 + src[x]*i_weight2 + (1<<5)) >> 6 )
static inline void pixel_avg_weight_wxh( uint8_t *dst, int i_dst, uint8_t *src, int i_src, int width, int height, int i_weight1 ){
    int y;
    const int i_weight2 = 64 - i_weight1;
    for(y=0; y<height; y++, dst += i_dst, src += i_src){
        op_scale2(0);
        op_scale2(1);
        if(width==2) continue;
        op_scale2(2);
        op_scale2(3);
        if(width==4) continue;
        op_scale2(4);
        op_scale2(5);
        op_scale2(6);
        op_scale2(7);
        if(width==8) continue;
        op_scale2(8);
        op_scale2(9);
        op_scale2(10);
        op_scale2(11);
        op_scale2(12);
        op_scale2(13);
        op_scale2(14);
        op_scale2(15);
    }
}

#define PIXEL_AVG_WEIGHT_C( width, height ) \
static void pixel_avg_weight_##width##x##height( \
                uint8_t *pix1, int i_stride_pix1, \
                uint8_t *pix2, int i_stride_pix2, int i_weight1 ) \
{ \
    pixel_avg_weight_wxh( pix1, i_stride_pix1, pix2, i_stride_pix2, width, height, i_weight1 ); \
}

PIXEL_AVG_WEIGHT_C(16,16)
PIXEL_AVG_WEIGHT_C(16,8)
PIXEL_AVG_WEIGHT_C(8,16)
PIXEL_AVG_WEIGHT_C(8,8)
PIXEL_AVG_WEIGHT_C(8,4)
PIXEL_AVG_WEIGHT_C(4,8)
PIXEL_AVG_WEIGHT_C(4,4)
PIXEL_AVG_WEIGHT_C(4,2)
PIXEL_AVG_WEIGHT_C(2,4)
PIXEL_AVG_WEIGHT_C(2,2)
#undef op_scale2
#undef PIXEL_AVG_WEIGHT_C

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

    pixf->ssd[PIXEL_16x16] = pixel_ssd_16x16;
    pixf->ssd[PIXEL_16x8]  = pixel_ssd_16x8;
    pixf->ssd[PIXEL_8x16]  = pixel_ssd_8x16;
    pixf->ssd[PIXEL_8x8]   = pixel_ssd_8x8;
    pixf->ssd[PIXEL_8x4]   = pixel_ssd_8x4;
    pixf->ssd[PIXEL_4x8]   = pixel_ssd_4x8;
    pixf->ssd[PIXEL_4x4]   = pixel_ssd_4x4;

    pixf->satd[PIXEL_16x16]= pixel_satd_16x16;
    pixf->satd[PIXEL_16x8] = pixel_satd_16x8;
    pixf->satd[PIXEL_8x16] = pixel_satd_8x16;
    pixf->satd[PIXEL_8x8]  = pixel_satd_8x8;
    pixf->satd[PIXEL_8x4]  = pixel_satd_8x4;
    pixf->satd[PIXEL_4x8]  = pixel_satd_4x8;
    pixf->satd[PIXEL_4x4]  = pixel_satd_4x4;

    pixf->sa8d[PIXEL_16x16]= pixel_sa8d_16x16;
    pixf->sa8d[PIXEL_16x8] = pixel_sa8d_16x8;
    pixf->sa8d[PIXEL_8x16] = pixel_sa8d_8x16;
    pixf->sa8d[PIXEL_8x8]  = pixel_sa8d_8x8;

    pixf->avg[PIXEL_16x16]= pixel_avg_16x16;
    pixf->avg[PIXEL_16x8] = pixel_avg_16x8;
    pixf->avg[PIXEL_8x16] = pixel_avg_8x16;
    pixf->avg[PIXEL_8x8]  = pixel_avg_8x8;
    pixf->avg[PIXEL_8x4]  = pixel_avg_8x4;
    pixf->avg[PIXEL_4x8]  = pixel_avg_4x8;
    pixf->avg[PIXEL_4x4]  = pixel_avg_4x4;
    pixf->avg[PIXEL_4x2]  = pixel_avg_4x2;
    pixf->avg[PIXEL_2x4]  = pixel_avg_2x4;
    pixf->avg[PIXEL_2x2]  = pixel_avg_2x2;
    
    pixf->avg_weight[PIXEL_16x16]= pixel_avg_weight_16x16;
    pixf->avg_weight[PIXEL_16x8] = pixel_avg_weight_16x8;
    pixf->avg_weight[PIXEL_8x16] = pixel_avg_weight_8x16;
    pixf->avg_weight[PIXEL_8x8]  = pixel_avg_weight_8x8;
    pixf->avg_weight[PIXEL_8x4]  = pixel_avg_weight_8x4;
    pixf->avg_weight[PIXEL_4x8]  = pixel_avg_weight_4x8;
    pixf->avg_weight[PIXEL_4x4]  = pixel_avg_weight_4x4;
    pixf->avg_weight[PIXEL_4x2]  = pixel_avg_weight_4x2;
    pixf->avg_weight[PIXEL_2x4]  = pixel_avg_weight_2x4;
    pixf->avg_weight[PIXEL_2x2]  = pixel_avg_weight_2x2;

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

        pixf->ssd[PIXEL_16x16] = x264_pixel_ssd_16x16_mmxext;
        pixf->ssd[PIXEL_16x8]  = x264_pixel_ssd_16x8_mmxext;
        pixf->ssd[PIXEL_8x16]  = x264_pixel_ssd_8x16_mmxext;
        pixf->ssd[PIXEL_8x8]   = x264_pixel_ssd_8x8_mmxext;
        pixf->ssd[PIXEL_8x4]   = x264_pixel_ssd_8x4_mmxext;
        pixf->ssd[PIXEL_4x8]   = x264_pixel_ssd_4x8_mmxext;
        pixf->ssd[PIXEL_4x4]   = x264_pixel_ssd_4x4_mmxext;
  
        pixf->satd[PIXEL_16x16]= x264_pixel_satd_16x16_mmxext;
        pixf->satd[PIXEL_16x8] = x264_pixel_satd_16x8_mmxext;
        pixf->satd[PIXEL_8x16] = x264_pixel_satd_8x16_mmxext;
        pixf->satd[PIXEL_8x8]  = x264_pixel_satd_8x8_mmxext;
        pixf->satd[PIXEL_8x4]  = x264_pixel_satd_8x4_mmxext;
        pixf->satd[PIXEL_4x8]  = x264_pixel_satd_4x8_mmxext;
        pixf->satd[PIXEL_4x4]  = x264_pixel_satd_4x4_mmxext;
    }
#endif
#ifdef ARCH_PPC
    if( cpu&X264_CPU_ALTIVEC )
    {
        x264_pixel_altivec_init( pixf );
    }
#endif
}


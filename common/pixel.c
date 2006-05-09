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

#include <string.h>

#include "common.h"
#include "clip1.h"

#ifdef HAVE_MMXEXT
#   include "i386/pixel.h"
#endif
#ifdef ARCH_PPC
#   include "ppc/pixel.h"
#endif
#ifdef ARCH_UltraSparc
#   include "sparc/pixel.h"
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


PIXEL_SAD_C( x264_pixel_sad_16x16, 16, 16 )
PIXEL_SAD_C( x264_pixel_sad_16x8,  16,  8 )
PIXEL_SAD_C( x264_pixel_sad_8x16,   8, 16 )
PIXEL_SAD_C( x264_pixel_sad_8x8,    8,  8 )
PIXEL_SAD_C( x264_pixel_sad_8x4,    8,  4 )
PIXEL_SAD_C( x264_pixel_sad_4x8,    4,  8 )
PIXEL_SAD_C( x264_pixel_sad_4x4,    4,  4 )


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

PIXEL_SSD_C( x264_pixel_ssd_16x16, 16, 16 )
PIXEL_SSD_C( x264_pixel_ssd_16x8,  16,  8 )
PIXEL_SSD_C( x264_pixel_ssd_8x16,   8, 16 )
PIXEL_SSD_C( x264_pixel_ssd_8x8,    8,  8 )
PIXEL_SSD_C( x264_pixel_ssd_8x4,    8,  4 )
PIXEL_SSD_C( x264_pixel_ssd_4x8,    4,  8 )
PIXEL_SSD_C( x264_pixel_ssd_4x4,    4,  4 )

int64_t x264_pixel_ssd_wxh( x264_pixel_function_t *pf, uint8_t *pix1, int i_pix1, uint8_t *pix2, int i_pix2, int i_width, int i_height )
{
    int64_t i_ssd = 0;
    int x, y;

#define SSD(size) i_ssd += pf->ssd[size]( pix1 + y*i_pix1 + x, i_pix1, \
                                          pix2 + y*i_pix2 + x, i_pix2 );
    for( y = 0; y < i_height-15; y += 16 )
    {
        for( x = 0; x < i_width-15; x += 16 )
            SSD(PIXEL_16x16);
        if( x < i_width-7 )
            SSD(PIXEL_8x16);
    }
    if( y < i_height-7 )
        for( x = 0; x < i_width-7; x += 8 )
            SSD(PIXEL_8x8);
#undef SSD

#define SSD1 { int d = pix1[y*i_pix1+x] - pix2[y*i_pix2+x]; i_ssd += d*d; }
    if( i_width % 8 != 0 )
    {
        for( y = 0; y < (i_height & ~7); y++ )
            for( x = i_width & ~7; x < i_width; x++ )
                SSD1;
    }
    if( i_height % 8 != 0 )
    {
        for( y = i_height & ~7; y < i_height; y++ )
            for( x = 0; x < i_width; x++ )
                SSD1;
    }
#undef SSD1

    return i_ssd;
}


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
PIXEL_SATD_C( x264_pixel_satd_16x16, 16, 16 )
PIXEL_SATD_C( x264_pixel_satd_16x8,  16, 8 )
PIXEL_SATD_C( x264_pixel_satd_8x16,  8, 16 )
PIXEL_SATD_C( x264_pixel_satd_8x8,   8, 8 )
PIXEL_SATD_C( x264_pixel_satd_8x4,   8, 4 )
PIXEL_SATD_C( x264_pixel_satd_4x8,   4, 8 )
PIXEL_SATD_C( x264_pixel_satd_4x4,   4, 4 )


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
static int x264_pixel_sa8d_##width##x##height( uint8_t *pix1, int i_stride_pix1, \
                                               uint8_t *pix2, int i_stride_pix2 ) \
{ \
    return ( pixel_sa8d_wxh( pix1, i_stride_pix1, pix2, i_stride_pix2, width, height ) + 2 ) >> 2; \
}
PIXEL_SA8D_C( 16, 16 )
PIXEL_SA8D_C( 16, 8 )
PIXEL_SA8D_C( 8, 16 )
PIXEL_SA8D_C( 8, 8 )

#define SAD_X( size ) \
static void x264_pixel_sad_x3_##size( uint8_t *fenc, uint8_t *pix0, uint8_t *pix1, uint8_t *pix2, int i_stride, int scores[3] )\
{\
    scores[0] = x264_pixel_sad_##size( fenc, FENC_STRIDE, pix0, i_stride );\
    scores[1] = x264_pixel_sad_##size( fenc, FENC_STRIDE, pix1, i_stride );\
    scores[2] = x264_pixel_sad_##size( fenc, FENC_STRIDE, pix2, i_stride );\
}\
static void x264_pixel_sad_x4_##size( uint8_t *fenc, uint8_t *pix0, uint8_t *pix1, uint8_t *pix2, uint8_t *pix3, int i_stride, int scores[4] )\
{\
    scores[0] = x264_pixel_sad_##size( fenc, FENC_STRIDE, pix0, i_stride );\
    scores[1] = x264_pixel_sad_##size( fenc, FENC_STRIDE, pix1, i_stride );\
    scores[2] = x264_pixel_sad_##size( fenc, FENC_STRIDE, pix2, i_stride );\
    scores[3] = x264_pixel_sad_##size( fenc, FENC_STRIDE, pix3, i_stride );\
}

SAD_X( 16x16 )
SAD_X( 16x8 )
SAD_X( 8x16 )
SAD_X( 8x8 )
SAD_X( 8x4 )
SAD_X( 4x8 )
SAD_X( 4x4 )

#ifdef ARCH_UltraSparc
SAD_X( 16x16_vis )
SAD_X( 16x8_vis )
SAD_X( 8x16_vis )
SAD_X( 8x8_vis )
#endif

/****************************************************************************
 * x264_pixel_init:
 ****************************************************************************/
void x264_pixel_init( int cpu, x264_pixel_function_t *pixf )
{
    memset( pixf, 0, sizeof(*pixf) );

#define INIT( name, cpu ) \
    pixf->name[PIXEL_16x16] = x264_pixel_##name##_16x16##cpu;\
    pixf->name[PIXEL_16x8]  = x264_pixel_##name##_16x8##cpu;\
    pixf->name[PIXEL_8x16]  = x264_pixel_##name##_8x16##cpu;\
    pixf->name[PIXEL_8x8]   = x264_pixel_##name##_8x8##cpu;\
    pixf->name[PIXEL_8x4]   = x264_pixel_##name##_8x4##cpu;\
    pixf->name[PIXEL_4x8]   = x264_pixel_##name##_4x8##cpu;\
    pixf->name[PIXEL_4x4]   = x264_pixel_##name##_4x4##cpu;

    INIT( sad, );
    INIT( sad_x3, );
    INIT( sad_x4, );
    INIT( ssd, );
    INIT( satd, );

    pixf->sa8d[PIXEL_16x16]= x264_pixel_sa8d_16x16;
    pixf->sa8d[PIXEL_16x8] = x264_pixel_sa8d_16x8;
    pixf->sa8d[PIXEL_8x16] = x264_pixel_sa8d_8x16;
    pixf->sa8d[PIXEL_8x8]  = x264_pixel_sa8d_8x8;

#ifdef HAVE_MMXEXT
    if( cpu&X264_CPU_MMX )
    {
        INIT( ssd, _mmx );
    }

    if( cpu&X264_CPU_MMXEXT )
    {
        INIT( sad, _mmxext );
        INIT( sad_x3, _mmxext );
        INIT( sad_x4, _mmxext );
        INIT( satd, _mmxext );

        pixf->sad_pde[PIXEL_16x16] = x264_pixel_sad_pde_16x16_mmxext;
        pixf->sad_pde[PIXEL_16x8 ] = x264_pixel_sad_pde_16x8_mmxext;
        pixf->sad_pde[PIXEL_8x16 ] = x264_pixel_sad_pde_8x16_mmxext;

#ifdef ARCH_X86
        pixf->sa8d[PIXEL_16x16] = x264_pixel_sa8d_16x16_mmxext;
        pixf->sa8d[PIXEL_8x8]   = x264_pixel_sa8d_8x8_mmxext;
        pixf->intra_sa8d_x3_8x8 = x264_intra_sa8d_x3_8x8_mmxext;
#endif
        pixf->intra_satd_x3_16x16 = x264_intra_satd_x3_16x16_mmxext;
        pixf->intra_satd_x3_8x8c  = x264_intra_satd_x3_8x8c_mmxext;
        pixf->intra_satd_x3_4x4   = x264_intra_satd_x3_4x4_mmxext;
    }
#endif

#ifdef HAVE_SSE2
    // disable on AMD processors since it is slower
    if( (cpu&X264_CPU_SSE2) && !(cpu&X264_CPU_3DNOW) )
    {
        pixf->sad[PIXEL_16x16] = x264_pixel_sad_16x16_sse2;
        pixf->sad[PIXEL_16x8 ] = x264_pixel_sad_16x8_sse2;

        pixf->satd[PIXEL_16x16]= x264_pixel_satd_16x16_sse2;
        pixf->satd[PIXEL_16x8] = x264_pixel_satd_16x8_sse2;
        pixf->satd[PIXEL_8x16] = x264_pixel_satd_8x16_sse2;
        pixf->satd[PIXEL_8x8]  = x264_pixel_satd_8x8_sse2;
        pixf->satd[PIXEL_8x4]  = x264_pixel_satd_8x4_sse2;

#ifdef ARCH_X86
        pixf->sad_x3[PIXEL_16x16] = x264_pixel_sad_x3_16x16_sse2;
        pixf->sad_x3[PIXEL_16x8 ] = x264_pixel_sad_x3_16x8_sse2;

        pixf->sad_x4[PIXEL_16x16] = x264_pixel_sad_x4_16x16_sse2;
        pixf->sad_x4[PIXEL_16x8 ] = x264_pixel_sad_x4_16x8_sse2;
#endif
    }
    // these are faster on both Intel and AMD
    if( cpu&X264_CPU_SSE2 )
    {
        pixf->ssd[PIXEL_16x16] = x264_pixel_ssd_16x16_sse2;
        pixf->ssd[PIXEL_16x8]  = x264_pixel_ssd_16x8_sse2;

#ifdef ARCH_X86_64
        pixf->sa8d[PIXEL_16x16] = x264_pixel_sa8d_16x16_sse2;
        pixf->sa8d[PIXEL_8x8]   = x264_pixel_sa8d_8x8_sse2;
        pixf->intra_sa8d_x3_8x8 = x264_intra_sa8d_x3_8x8_sse2;
#endif
    }
#endif

#ifdef ARCH_PPC
    if( cpu&X264_CPU_ALTIVEC )
    {
        x264_pixel_altivec_init( pixf );
    }
#endif
#ifdef ARCH_UltraSparc
    pixf->sad[PIXEL_8x8]   = x264_pixel_sad_8x8_vis;
    pixf->sad[PIXEL_8x16]  = x264_pixel_sad_8x16_vis;
    pixf->sad[PIXEL_16x8]  = x264_pixel_sad_16x8_vis;
    pixf->sad[PIXEL_16x16] = x264_pixel_sad_16x16_vis;

    pixf->sad_x3[PIXEL_8x8]   = x264_pixel_sad_x3_8x8_vis;
    pixf->sad_x3[PIXEL_8x16]  = x264_pixel_sad_x3_8x16_vis;
    pixf->sad_x3[PIXEL_16x8]  = x264_pixel_sad_x3_16x8_vis;
    pixf->sad_x3[PIXEL_16x16] = x264_pixel_sad_x3_16x16_vis;

    pixf->sad_x4[PIXEL_8x8]   = x264_pixel_sad_x4_8x8_vis;
    pixf->sad_x4[PIXEL_8x16]  = x264_pixel_sad_x4_8x16_vis;
    pixf->sad_x4[PIXEL_16x8]  = x264_pixel_sad_x4_16x8_vis;
    pixf->sad_x4[PIXEL_16x16] = x264_pixel_sad_x4_16x16_vis;
#endif
}


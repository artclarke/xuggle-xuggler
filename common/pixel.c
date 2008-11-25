/*****************************************************************************
 * pixel.c: h264 encoder
 *****************************************************************************
 * Copyright (C) 2003-2008 x264 project
 *
 * Authors: Loren Merritt <lorenm@u.washington.edu>
 *          Laurent Aimar <fenrir@via.ecp.fr>
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

#include "common.h"

#ifdef HAVE_MMX
#   include "x86/pixel.h"
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
    int align = !(((long)pix1 | (long)pix2 | i_pix1 | i_pix2) & 15);

#define SSD(size) i_ssd += pf->ssd[size]( pix1 + y*i_pix1 + x, i_pix1, \
                                          pix2 + y*i_pix2 + x, i_pix2 );
    for( y = 0; y < i_height-15; y += 16 )
    {
        x = 0;
        if( align )
            for( ; x < i_width-15; x += 16 )
                SSD(PIXEL_16x16);
        for( ; x < i_width-7; x += 8 )
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


/****************************************************************************
 * pixel_var_wxh
 ****************************************************************************/
#define PIXEL_VAR_C( name, w, shift ) \
static int name( uint8_t *pix, int i_stride, uint32_t *sad ) \
{                                             \
    uint32_t var = 0, sum = 0, sqr = 0;       \
    int x, y;                                 \
    for( y = 0; y < w; y++ )                  \
    {                                         \
        for( x = 0; x < w; x++ )              \
        {                                     \
            sum += pix[x];                    \
            sqr += pix[x] * pix[x];           \
        }                                     \
        pix += i_stride;                      \
    }                                         \
    var = sqr - (sum * sum >> shift);         \
    *sad = sum;                               \
    return var;                               \
}

PIXEL_VAR_C( x264_pixel_var_16x16, 16, 8 )
PIXEL_VAR_C( x264_pixel_var_8x8,    8, 6 )


#define HADAMARD4(d0,d1,d2,d3,s0,s1,s2,s3) {\
    int t0 = s0 + s1;\
    int t1 = s0 - s1;\
    int t2 = s2 + s3;\
    int t3 = s2 - s3;\
    d0 = t0 + t2;\
    d2 = t0 - t2;\
    d1 = t1 + t3;\
    d3 = t1 - t3;\
}

/****************************************************************************
 * pixel_satd_WxH: sum of 4x4 Hadamard transformed differences
 ****************************************************************************/
static int pixel_satd_wxh( uint8_t *pix1, int i_pix1, uint8_t *pix2, int i_pix2, int i_width, int i_height )
{
    int16_t tmp[4][4];
    int x, y;
    int i_satd = 0;

    for( y = 0; y < i_height; y += 4 )
    {
        for( x = 0; x < i_width; x += 4 )
        {
            int i;
            uint8_t *p1 = pix1+x, *p2 = pix2+x;

            for( i=0; i<4; i++, p1+=i_pix1, p2+=i_pix2 )
            {
                int a0 = p1[0] - p2[0];
                int a1 = p1[1] - p2[1];
                int a2 = p1[2] - p2[2];
                int a3 = p1[3] - p2[3];
                HADAMARD4( tmp[i][0], tmp[i][1], tmp[i][2], tmp[i][3], a0,a1,a2,a3 );
            }
            for( i=0; i<4; i++ )
            {
                int a0,a1,a2,a3;
                HADAMARD4( a0,a1,a2,a3, tmp[0][i], tmp[1][i], tmp[2][i], tmp[3][i] );
                i_satd += abs(a0) + abs(a1) + abs(a2) + abs(a3);
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
    int b0,b1,b2,b3,b4,b5,b6,b7;\
    HADAMARD4( b0,b1,b2,b3, SRC(0), SRC(1), SRC(2), SRC(3) );\
    HADAMARD4( b4,b5,b6,b7, SRC(4), SRC(5), SRC(6), SRC(7) );\
    DST(0, b0 + b4);\
    DST(4, b0 - b4);\
    DST(1, b1 + b5);\
    DST(5, b1 - b5);\
    DST(2, b2 + b6);\
    DST(6, b2 - b6);\
    DST(3, b3 + b7);\
    DST(7, b3 - b7);\
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
            uint8_t *p1 = pix1+x, *p2 = pix2+x;

#define SRC(x)     a##x
#define DST(x,rhs) diff[i][x] = (rhs)
            for( i=0; i<8; i++, p1+=i_pix1, p2+=i_pix2 )
            {
                int a0 = p1[0] - p2[0];
                int a1 = p1[1] - p2[1];
                int a2 = p1[2] - p2[2];
                int a3 = p1[3] - p2[3];
                int a4 = p1[4] - p2[4];
                int a5 = p1[5] - p2[5];
                int a6 = p1[6] - p2[6];
                int a7 = p1[7] - p2[7];
                SA8D_1D
            }
#undef SRC
#undef DST

#define SRC(x)     diff[x][i]
#define DST(x,rhs) i_satd += abs(rhs)
            for( i=0; i<8; i++ )
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


static uint64_t pixel_hadamard_ac( uint8_t *pix, int stride )
{
    int16_t tmp[8][8];
    int sum4=0, sum8=0;
    int i;
    for( i=0; i<8; i++, pix+=stride )
    {
        HADAMARD4( tmp[0][i], tmp[1][i], tmp[2][i], tmp[3][i],
                   pix[0], pix[1], pix[2], pix[3] );
        HADAMARD4( tmp[4][i], tmp[5][i], tmp[6][i], tmp[7][i],
                   pix[4], pix[5], pix[6], pix[7] );
    }
    for( i=0; i<8; i++ )
    {
        int a0,a1,a2,a3,a4,a5,a6,a7;
        HADAMARD4( a0,a1,a2,a3, tmp[i][0], tmp[i][1], tmp[i][2], tmp[i][3] );
        sum4 += abs(a0) + abs(a1) + abs(a2) + abs(a3);
        HADAMARD4( a4,a5,a6,a7, tmp[i][4], tmp[i][5], tmp[i][6], tmp[i][7] );
        sum4 += abs(a4) + abs(a5) + abs(a6) + abs(a7);
        tmp[i][0] = a0 + a4;
        tmp[i][4] = a0 - a4;
        tmp[i][1] = a1 + a5;
        tmp[i][5] = a1 - a5;
        tmp[i][2] = a2 + a6;
        tmp[i][6] = a2 - a6;
        tmp[i][3] = a3 + a7;
        tmp[i][7] = a3 - a7;
    }
    for( i=0; i<8; i++ )
    {
        sum8 += abs( tmp[0][i] + tmp[4][i] )
              + abs( tmp[0][i] - tmp[4][i] )
              + abs( tmp[1][i] + tmp[5][i] )
              + abs( tmp[1][i] - tmp[5][i] )
              + abs( tmp[2][i] + tmp[6][i] )
              + abs( tmp[2][i] - tmp[6][i] )
              + abs( tmp[3][i] + tmp[7][i] )
              + abs( tmp[3][i] - tmp[7][i] );
    }
    sum4 -= tmp[0][0]+tmp[4][0];
    sum8 -= tmp[0][0]+tmp[4][0];
    return ((uint64_t)sum8<<32) + sum4;
}

#define HADAMARD_AC(w,h) \
static uint64_t x264_pixel_hadamard_ac_##w##x##h( uint8_t *pix, int stride )\
{\
    uint64_t sum = pixel_hadamard_ac( pix, stride );\
    if( w==16 )\
        sum += pixel_hadamard_ac( pix+8, stride );\
    if( h==16 )\
        sum += pixel_hadamard_ac( pix+8*stride, stride );\
    if( w==16 && h==16 )\
        sum += pixel_hadamard_ac( pix+8*stride+8, stride );\
    return ((sum>>34)<<32) + ((uint32_t)sum>>1);\
}
HADAMARD_AC( 16, 16 )
HADAMARD_AC( 16, 8 )
HADAMARD_AC( 8, 16 )
HADAMARD_AC( 8, 8 )


/****************************************************************************
 * pixel_sad_x4
 ****************************************************************************/
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
 * pixel_satd_x4
 * no faster than single satd, but needed for satd to be a drop-in replacement for sad
 ****************************************************************************/

#define SATD_X( size, cpu ) \
static void x264_pixel_satd_x3_##size##cpu( uint8_t *fenc, uint8_t *pix0, uint8_t *pix1, uint8_t *pix2, int i_stride, int scores[3] )\
{\
    scores[0] = x264_pixel_satd_##size##cpu( fenc, FENC_STRIDE, pix0, i_stride );\
    scores[1] = x264_pixel_satd_##size##cpu( fenc, FENC_STRIDE, pix1, i_stride );\
    scores[2] = x264_pixel_satd_##size##cpu( fenc, FENC_STRIDE, pix2, i_stride );\
}\
static void x264_pixel_satd_x4_##size##cpu( uint8_t *fenc, uint8_t *pix0, uint8_t *pix1, uint8_t *pix2, uint8_t *pix3, int i_stride, int scores[4] )\
{\
    scores[0] = x264_pixel_satd_##size##cpu( fenc, FENC_STRIDE, pix0, i_stride );\
    scores[1] = x264_pixel_satd_##size##cpu( fenc, FENC_STRIDE, pix1, i_stride );\
    scores[2] = x264_pixel_satd_##size##cpu( fenc, FENC_STRIDE, pix2, i_stride );\
    scores[3] = x264_pixel_satd_##size##cpu( fenc, FENC_STRIDE, pix3, i_stride );\
}
#define SATD_X_DECL6( cpu )\
SATD_X( 16x16, cpu )\
SATD_X( 16x8, cpu )\
SATD_X( 8x16, cpu )\
SATD_X( 8x8, cpu )\
SATD_X( 8x4, cpu )\
SATD_X( 4x8, cpu )
#define SATD_X_DECL7( cpu )\
SATD_X_DECL6( cpu )\
SATD_X( 4x4, cpu )

SATD_X_DECL7()
#ifdef HAVE_MMX
SATD_X_DECL7( _mmxext )
SATD_X_DECL6( _sse2 )
SATD_X_DECL7( _ssse3 )
SATD_X_DECL6( _ssse3_phadd )
#endif

/****************************************************************************
 * structural similarity metric
 ****************************************************************************/
static void ssim_4x4x2_core( const uint8_t *pix1, int stride1,
                             const uint8_t *pix2, int stride2,
                             int sums[2][4])
{
    int x, y, z;
    for(z=0; z<2; z++)
    {
        uint32_t s1=0, s2=0, ss=0, s12=0;
        for(y=0; y<4; y++)
            for(x=0; x<4; x++)
            {
                int a = pix1[x+y*stride1];
                int b = pix2[x+y*stride2];
                s1  += a;
                s2  += b;
                ss  += a*a;
                ss  += b*b;
                s12 += a*b;
            }
        sums[z][0] = s1;
        sums[z][1] = s2;
        sums[z][2] = ss;
        sums[z][3] = s12;
        pix1 += 4;
        pix2 += 4;
    }
}

static float ssim_end1( int s1, int s2, int ss, int s12 )
{
    static const int ssim_c1 = (int)(.01*.01*255*255*64 + .5);
    static const int ssim_c2 = (int)(.03*.03*255*255*64*63 + .5);
    int vars = ss*64 - s1*s1 - s2*s2;
    int covar = s12*64 - s1*s2;
    return (float)(2*s1*s2 + ssim_c1) * (float)(2*covar + ssim_c2)\
           / ((float)(s1*s1 + s2*s2 + ssim_c1) * (float)(vars + ssim_c2));
}

static float ssim_end4( int sum0[5][4], int sum1[5][4], int width )
{
    int i;
    float ssim = 0.0;
    for( i = 0; i < width; i++ )
        ssim += ssim_end1( sum0[i][0] + sum0[i+1][0] + sum1[i][0] + sum1[i+1][0],
                           sum0[i][1] + sum0[i+1][1] + sum1[i][1] + sum1[i+1][1],
                           sum0[i][2] + sum0[i+1][2] + sum1[i][2] + sum1[i+1][2],
                           sum0[i][3] + sum0[i+1][3] + sum1[i][3] + sum1[i+1][3] );
    return ssim;
}

float x264_pixel_ssim_wxh( x264_pixel_function_t *pf,
                           uint8_t *pix1, int stride1,
                           uint8_t *pix2, int stride2,
                           int width, int height )
{
    int x, y, z;
    float ssim = 0.0;
    int (*sum0)[4] = x264_malloc(4 * (width/4+3) * sizeof(int));
    int (*sum1)[4] = x264_malloc(4 * (width/4+3) * sizeof(int));
    width >>= 2;
    height >>= 2;
    z = 0;
    for( y = 1; y < height; y++ )
    {
        for( ; z <= y; z++ )
        {
            XCHG( void*, sum0, sum1 );
            for( x = 0; x < width; x+=2 )
                pf->ssim_4x4x2_core( &pix1[4*(x+z*stride1)], stride1, &pix2[4*(x+z*stride2)], stride2, &sum0[x] );
        }
        for( x = 0; x < width-1; x += 4 )
            ssim += pf->ssim_end4( sum0+x, sum1+x, X264_MIN(4,width-x-1) );
    }
    x264_free(sum0);
    x264_free(sum1);
    return ssim;
}


/****************************************************************************
 * successive elimination
 ****************************************************************************/
static int x264_pixel_ads4( int enc_dc[4], uint16_t *sums, int delta,
                            uint16_t *cost_mvx, int16_t *mvs, int width, int thresh )
{
    int nmv=0, i;
    for( i=0; i<width; i++, sums++ )
    {
        int ads = abs( enc_dc[0] - sums[0] )
                + abs( enc_dc[1] - sums[8] )
                + abs( enc_dc[2] - sums[delta] )
                + abs( enc_dc[3] - sums[delta+8] )
                + cost_mvx[i];
        if( ads < thresh )
            mvs[nmv++] = i;
    }
    return nmv;
}

static int x264_pixel_ads2( int enc_dc[2], uint16_t *sums, int delta,
                            uint16_t *cost_mvx, int16_t *mvs, int width, int thresh )
{
    int nmv=0, i;
    for( i=0; i<width; i++, sums++ )
    {
        int ads = abs( enc_dc[0] - sums[0] )
                + abs( enc_dc[1] - sums[delta] )
                + cost_mvx[i];
        if( ads < thresh )
            mvs[nmv++] = i;
    }
    return nmv;
}

static int x264_pixel_ads1( int enc_dc[1], uint16_t *sums, int delta,
                            uint16_t *cost_mvx, int16_t *mvs, int width, int thresh )
{
    int nmv=0, i;
    for( i=0; i<width; i++, sums++ )
    {
        int ads = abs( enc_dc[0] - sums[0] )
                + cost_mvx[i];
        if( ads < thresh )
            mvs[nmv++] = i;
    }
    return nmv;
}


/****************************************************************************
 * x264_pixel_init:
 ****************************************************************************/
void x264_pixel_init( int cpu, x264_pixel_function_t *pixf )
{
    memset( pixf, 0, sizeof(*pixf) );

#define INIT2_NAME( name1, name2, cpu ) \
    pixf->name1[PIXEL_16x16] = x264_pixel_##name2##_16x16##cpu;\
    pixf->name1[PIXEL_16x8]  = x264_pixel_##name2##_16x8##cpu;
#define INIT4_NAME( name1, name2, cpu ) \
    INIT2_NAME( name1, name2, cpu ) \
    pixf->name1[PIXEL_8x16]  = x264_pixel_##name2##_8x16##cpu;\
    pixf->name1[PIXEL_8x8]   = x264_pixel_##name2##_8x8##cpu;
#define INIT5_NAME( name1, name2, cpu ) \
    INIT4_NAME( name1, name2, cpu ) \
    pixf->name1[PIXEL_8x4]   = x264_pixel_##name2##_8x4##cpu;
#define INIT6_NAME( name1, name2, cpu ) \
    INIT5_NAME( name1, name2, cpu ) \
    pixf->name1[PIXEL_4x8]   = x264_pixel_##name2##_4x8##cpu;
#define INIT7_NAME( name1, name2, cpu ) \
    INIT6_NAME( name1, name2, cpu ) \
    pixf->name1[PIXEL_4x4]   = x264_pixel_##name2##_4x4##cpu;
#define INIT2( name, cpu ) INIT2_NAME( name, name, cpu )
#define INIT4( name, cpu ) INIT4_NAME( name, name, cpu )
#define INIT5( name, cpu ) INIT5_NAME( name, name, cpu )
#define INIT6( name, cpu ) INIT6_NAME( name, name, cpu )
#define INIT7( name, cpu ) INIT7_NAME( name, name, cpu )

#define INIT_ADS( cpu ) \
    pixf->ads[PIXEL_16x16] = x264_pixel_ads4##cpu;\
    pixf->ads[PIXEL_16x8] = x264_pixel_ads2##cpu;\
    pixf->ads[PIXEL_8x8] = x264_pixel_ads1##cpu;

    INIT7( sad, );
    INIT7_NAME( sad_aligned, sad, );
    INIT7( sad_x3, );
    INIT7( sad_x4, );
    INIT7( ssd, );
    INIT7( satd, );
    INIT7( satd_x3, );
    INIT7( satd_x4, );
    INIT4( sa8d, );
    INIT4( hadamard_ac, );
    INIT_ADS( );

    pixf->var[PIXEL_16x16] = x264_pixel_var_16x16;
    pixf->var[PIXEL_8x8]   = x264_pixel_var_8x8;

    pixf->ssim_4x4x2_core = ssim_4x4x2_core;
    pixf->ssim_end4 = ssim_end4;

#ifdef HAVE_MMX
    if( cpu&X264_CPU_MMX )
    {
        INIT7( ssd, _mmx );
    }

    if( cpu&X264_CPU_MMXEXT )
    {
        INIT7( sad, _mmxext );
        INIT7_NAME( sad_aligned, sad, _mmxext );
        INIT7( sad_x3, _mmxext );
        INIT7( sad_x4, _mmxext );
        INIT7( satd, _mmxext );
        INIT7( satd_x3, _mmxext );
        INIT7( satd_x4, _mmxext );
        INIT4( hadamard_ac, _mmxext );
        INIT_ADS( _mmxext );
        pixf->var[PIXEL_16x16] = x264_pixel_var_16x16_mmxext;
        pixf->var[PIXEL_8x8]   = x264_pixel_var_8x8_mmxext;
#ifdef ARCH_X86
        pixf->sa8d[PIXEL_16x16] = x264_pixel_sa8d_16x16_mmxext;
        pixf->sa8d[PIXEL_8x8]   = x264_pixel_sa8d_8x8_mmxext;
        pixf->intra_sa8d_x3_8x8 = x264_intra_sa8d_x3_8x8_mmxext;
        pixf->ssim_4x4x2_core  = x264_pixel_ssim_4x4x2_core_mmxext;

        if( cpu&X264_CPU_CACHELINE_32 )
        {
            INIT5( sad, _cache32_mmxext );
            INIT4( sad_x3, _cache32_mmxext );
            INIT4( sad_x4, _cache32_mmxext );
        }
        else if( cpu&X264_CPU_CACHELINE_64 )
        {
            INIT5( sad, _cache64_mmxext );
            INIT4( sad_x3, _cache64_mmxext );
            INIT4( sad_x4, _cache64_mmxext );
        }
#else
        if( cpu&X264_CPU_CACHELINE_64 )
        {
            pixf->sad[PIXEL_8x16] = x264_pixel_sad_8x16_cache64_mmxext;
            pixf->sad[PIXEL_8x8]  = x264_pixel_sad_8x8_cache64_mmxext;
            pixf->sad[PIXEL_8x4]  = x264_pixel_sad_8x4_cache64_mmxext;
            pixf->sad_x3[PIXEL_8x16] = x264_pixel_sad_x3_8x16_cache64_mmxext;
            pixf->sad_x3[PIXEL_8x8]  = x264_pixel_sad_x3_8x8_cache64_mmxext;
            pixf->sad_x4[PIXEL_8x16] = x264_pixel_sad_x4_8x16_cache64_mmxext;
            pixf->sad_x4[PIXEL_8x8]  = x264_pixel_sad_x4_8x8_cache64_mmxext;
        }
#endif
        pixf->intra_satd_x3_16x16 = x264_intra_satd_x3_16x16_mmxext;
        pixf->intra_sad_x3_16x16 = x264_intra_sad_x3_16x16_mmxext;
        pixf->intra_satd_x3_8x8c  = x264_intra_satd_x3_8x8c_mmxext;
        pixf->intra_satd_x3_4x4   = x264_intra_satd_x3_4x4_mmxext;
    }

    if( (cpu&X264_CPU_SSE2) && !(cpu&X264_CPU_SSE2_IS_SLOW) )
    {
        INIT2( sad, _sse2 );
        INIT2( sad_x3, _sse2 );
        INIT2( sad_x4, _sse2 );
        if( !(cpu&X264_CPU_STACK_MOD4) )
        {
            INIT4( hadamard_ac, _sse2 );
        }
        INIT_ADS( _sse2 );
        pixf->var[PIXEL_8x8] = x264_pixel_var_8x8_sse2;
        pixf->intra_sad_x3_16x16 = x264_intra_sad_x3_16x16_sse2;
#ifdef ARCH_X86
        if( cpu&X264_CPU_CACHELINE_64 )
        {
            INIT2( sad, _cache64_sse2 );
            INIT2( sad_x3, _cache64_sse2 );
            INIT2( sad_x4, _cache64_sse2 );
        }
#endif
        if( cpu&X264_CPU_SSE_MISALIGN )
        {
            INIT2( sad_x3, _sse2_misalign );
            INIT2( sad_x4, _sse2_misalign );
        }
    }
    if( cpu&X264_CPU_SSE2 )
    {
        INIT5( ssd, _sse2 );
        if( cpu&X264_CPU_SSE2_IS_FAST )
        {
            INIT6( satd, _sse2 );
            INIT6( satd_x3, _sse2 );
            INIT6( satd_x4, _sse2 );
        }
        else
        {
            INIT5( satd, _sse2 );
            INIT5( satd_x3, _sse2 );
            INIT5( satd_x4, _sse2 );
        }
        INIT2_NAME( sad_aligned, sad, _sse2_aligned );
        pixf->var[PIXEL_16x16] = x264_pixel_var_16x16_sse2;
        pixf->ssim_4x4x2_core  = x264_pixel_ssim_4x4x2_core_sse2;
        pixf->ssim_end4        = x264_pixel_ssim_end4_sse2;
        pixf->sa8d[PIXEL_16x16] = x264_pixel_sa8d_16x16_sse2;
        pixf->sa8d[PIXEL_8x8]   = x264_pixel_sa8d_8x8_sse2;
#ifdef ARCH_X86_64
        pixf->intra_sa8d_x3_8x8 = x264_intra_sa8d_x3_8x8_sse2;
#endif
    }

    if( cpu&X264_CPU_SSE2_IS_FAST && !(cpu&X264_CPU_CACHELINE_64) )
    {
        pixf->sad_aligned[PIXEL_8x16] = x264_pixel_sad_8x16_sse2;
        pixf->sad[PIXEL_8x16] = x264_pixel_sad_8x16_sse2;
        pixf->sad_x3[PIXEL_8x16] = x264_pixel_sad_x3_8x16_sse2;
        pixf->sad_x3[PIXEL_8x8] = x264_pixel_sad_x3_8x8_sse2;
        pixf->sad_x3[PIXEL_8x4] = x264_pixel_sad_x3_8x4_sse2;
        pixf->sad_x4[PIXEL_8x16] = x264_pixel_sad_x4_8x16_sse2;
        pixf->sad_x4[PIXEL_8x8] = x264_pixel_sad_x4_8x8_sse2;
        pixf->sad_x4[PIXEL_8x4] = x264_pixel_sad_x4_8x4_sse2;
    }

    if( (cpu&X264_CPU_SSE3) && (cpu&X264_CPU_CACHELINE_64) )
    {
        INIT2( sad, _sse3 );
        INIT2( sad_x3, _sse3 );
        INIT2( sad_x4, _sse3 );
    }

    if( cpu&X264_CPU_SSSE3 )
    {
        INIT7( satd, _ssse3 );
        INIT7( satd_x3, _ssse3 );
        INIT7( satd_x4, _ssse3 );
        if( !(cpu&X264_CPU_STACK_MOD4) )
        {
            INIT4( hadamard_ac, _ssse3 );
        }
        INIT_ADS( _ssse3 );
        pixf->sa8d[PIXEL_16x16]= x264_pixel_sa8d_16x16_ssse3;
        pixf->sa8d[PIXEL_8x8]  = x264_pixel_sa8d_8x8_ssse3;
        pixf->intra_satd_x3_16x16 = x264_intra_satd_x3_16x16_ssse3;
        pixf->intra_sad_x3_16x16  = x264_intra_sad_x3_16x16_ssse3;
        pixf->intra_satd_x3_8x8c  = x264_intra_satd_x3_8x8c_ssse3;
        pixf->intra_satd_x3_4x4   = x264_intra_satd_x3_4x4_ssse3;
#ifdef ARCH_X86_64
        pixf->intra_sa8d_x3_8x8 = x264_intra_sa8d_x3_8x8_ssse3;
#endif
        if( cpu&X264_CPU_CACHELINE_64 )
        {
            INIT2( sad, _cache64_ssse3 );
            INIT2( sad_x3, _cache64_ssse3 );
            INIT2( sad_x4, _cache64_ssse3 );
        }
        if( cpu&X264_CPU_PHADD_IS_FAST )
        {
            INIT6( satd, _ssse3_phadd );
            INIT6( satd_x3, _ssse3_phadd );
            INIT6( satd_x4, _ssse3_phadd );
        }
    }

    if( cpu&X264_CPU_SSE4 )
    {
        pixf->ssd[PIXEL_4x8] = x264_pixel_ssd_4x8_sse4;
        pixf->ssd[PIXEL_4x4] = x264_pixel_ssd_4x4_sse4;
    }
#endif //HAVE_MMX

#ifdef ARCH_PPC
    if( cpu&X264_CPU_ALTIVEC )
    {
        x264_pixel_altivec_init( pixf );
    }
#endif
#ifdef ARCH_UltraSparc
    INIT4( sad, _vis );
    INIT4( sad_x3, _vis );
    INIT4( sad_x4, _vis );
#endif

    pixf->ads[PIXEL_8x16] =
    pixf->ads[PIXEL_8x4] =
    pixf->ads[PIXEL_4x8] = pixf->ads[PIXEL_16x8];
    pixf->ads[PIXEL_4x4] = pixf->ads[PIXEL_8x8];
}


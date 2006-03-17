/*****************************************************************************
 * mc.c: h264 encoder library (Motion Compensation)
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar
 * $Id: mc.c,v 1.1 2004/06/03 19:27:07 fenrir Exp $
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

#include <stdio.h>
#include <string.h>

#include "common.h"
#include "clip1.h"

#ifdef HAVE_MMXEXT
#include "i386/mc.h"
#endif
#ifdef ARCH_PPC
#include "ppc/mc.h"
#endif


static inline int x264_tapfilter( uint8_t *pix, int i_pix_next )
{
    return pix[-2*i_pix_next] - 5*pix[-1*i_pix_next] + 20*(pix[0] + pix[1*i_pix_next]) - 5*pix[ 2*i_pix_next] + pix[ 3*i_pix_next];
}
static inline int x264_tapfilter1( uint8_t *pix )
{
    return pix[-2] - 5*pix[-1] + 20*(pix[0] + pix[1]) - 5*pix[ 2] + pix[ 3];
}

static inline void pixel_avg( uint8_t *dst,  int i_dst_stride,
                              uint8_t *src1, int i_src1_stride,
                              uint8_t *src2, int i_src2_stride,
                              int i_width, int i_height )
{
    int x, y;
    for( y = 0; y < i_height; y++ )
    {
        for( x = 0; x < i_width; x++ )
        {
            dst[x] = ( src1[x] + src2[x] + 1 ) >> 1;
        }
        dst  += i_dst_stride;
        src1 += i_src1_stride;
        src2 += i_src2_stride;
    }
}

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

typedef void (*pf_mc_t)(uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_width, int i_height );

static void mc_copy( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_width, int i_height )
{
    int y;

    for( y = 0; y < i_height; y++ )
    {
        memcpy( dst, src, i_width );

        src += i_src_stride;
        dst += i_dst_stride;
    }
}
static inline void mc_hh( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_width, int i_height )
{
    int x, y;

    for( y = 0; y < i_height; y++ )
    {
        for( x = 0; x < i_width; x++ )
        {
            dst[x] = x264_mc_clip1( ( x264_tapfilter1( &src[x] ) + 16 ) >> 5 );
        }
        src += i_src_stride;
        dst += i_dst_stride;
    }
}
static inline void mc_hv( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_width, int i_height )
{
    int x, y;

    for( y = 0; y < i_height; y++ )
    {
        for( x = 0; x < i_width; x++ )
        {
            dst[x] = x264_mc_clip1( ( x264_tapfilter( &src[x], i_src_stride ) + 16 ) >> 5 );
        }
        src += i_src_stride;
        dst += i_dst_stride;
    }
}
static inline void mc_hc( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_width, int i_height )
{
    uint8_t *out;
    uint8_t *pix;
    int x, y;

    for( x = 0; x < i_width; x++ )
    {
        int tap[6];

        pix = &src[x];
        out = &dst[x];

        tap[0] = x264_tapfilter1( &pix[-2*i_src_stride] );
        tap[1] = x264_tapfilter1( &pix[-1*i_src_stride] );
        tap[2] = x264_tapfilter1( &pix[ 0*i_src_stride] );
        tap[3] = x264_tapfilter1( &pix[ 1*i_src_stride] );
        tap[4] = x264_tapfilter1( &pix[ 2*i_src_stride] );

        for( y = 0; y < i_height; y++ )
        {
            tap[5] = x264_tapfilter1( &pix[ 3*i_src_stride] );

            *out = x264_mc_clip1( ( tap[0] - 5*tap[1] + 20 * tap[2] + 20 * tap[3] -5*tap[4] + tap[5] + 512 ) >> 10 );

            /* Next line */
            pix += i_src_stride;
            out += i_dst_stride;
            tap[0] = tap[1];
            tap[1] = tap[2];
            tap[2] = tap[3];
            tap[3] = tap[4];
            tap[4] = tap[5];
        }
    }
}

static const int hpel_ref0[16] = {0,1,1,1,0,1,1,1,2,3,3,3,0,1,1,1};
static const int hpel_ref1[16] = {0,0,0,0,2,2,3,2,2,2,3,2,2,2,3,2};

static void mc_luma( uint8_t *src[4], int i_src_stride,
                     uint8_t *dst,    int i_dst_stride,
                     int mvx,int mvy,
                     int i_width, int i_height )
{
    int qpel_idx = ((mvy&3)<<2) + (mvx&3);
    int offset = (mvy>>2)*i_src_stride + (mvx>>2);
    uint8_t *src1 = src[hpel_ref0[qpel_idx]] + offset + ((mvy&3) == 3) * i_src_stride;

    if( qpel_idx & 5 ) /* qpel interpolation needed */
    {
        uint8_t *src2 = src[hpel_ref1[qpel_idx]] + offset + ((mvx&3) == 3);

        pixel_avg( dst, i_dst_stride, src1, i_src_stride,
                   src2, i_src_stride, i_width, i_height );
    }
    else
    {
        mc_copy( src1, i_src_stride, dst, i_dst_stride, i_width, i_height );
    }
}

static uint8_t *get_ref( uint8_t *src[4], int i_src_stride,
                         uint8_t *dst,    int * i_dst_stride,
                         int mvx,int mvy,
                         int i_width, int i_height )
{
    int qpel_idx = ((mvy&3)<<2) + (mvx&3);
    int offset = (mvy>>2)*i_src_stride + (mvx>>2);
    uint8_t *src1 = src[hpel_ref0[qpel_idx]] + offset + ((mvy&3) == 3) * i_src_stride;

    if( qpel_idx & 5 ) /* qpel interpolation needed */
    {
        uint8_t *src2 = src[hpel_ref1[qpel_idx]] + offset + ((mvx&3) == 3);

        pixel_avg( dst, *i_dst_stride, src1, i_src_stride,
                   src2, i_src_stride, i_width, i_height );

        return dst;
    }
    else
    {
        *i_dst_stride = i_src_stride;
        return src1;
    }
}

/* full chroma mc (ie until 1/8 pixel)*/
static void motion_compensation_chroma( uint8_t *src, int i_src_stride,
                                        uint8_t *dst, int i_dst_stride,
                                        int mvx, int mvy,
                                        int i_width, int i_height )
{
    uint8_t *srcp;
    int x, y;

    const int d8x = mvx&0x07;
    const int d8y = mvy&0x07;

    const int cA = (8-d8x)*(8-d8y);
    const int cB = d8x    *(8-d8y);
    const int cC = (8-d8x)*d8y;
    const int cD = d8x    *d8y;

    src  += (mvy >> 3) * i_src_stride + (mvx >> 3);
    srcp = &src[i_src_stride];

    for( y = 0; y < i_height; y++ )
    {
        for( x = 0; x < i_width; x++ )
        {
            dst[x] = ( cA*src[x]  + cB*src[x+1] +
                       cC*srcp[x] + cD*srcp[x+1] + 32 ) >> 6;
        }
        dst  += i_dst_stride;

        src   = srcp;
        srcp += i_src_stride;
    }
}

#ifdef HAVE_MMXEXT
static void motion_compensation_chroma_mmxext( uint8_t *src, int i_src_stride,
                                        uint8_t *dst, int i_dst_stride,
                                        int mvx, int mvy,
                                        int i_width, int i_height )
{
    if (i_width == 2) {
        motion_compensation_chroma(src, i_src_stride, dst, i_dst_stride,
                                   mvx, mvy, i_width, i_height);
    } else {
        const int d8x = mvx&0x07;
        const int d8y = mvy&0x07;
        
        src  += (mvy >> 3) * i_src_stride + (mvx >> 3);
        
        x264_mc_chroma_mmxext( src, i_src_stride, dst, i_dst_stride,
                               d8x, d8y, i_width, i_height );
    }
}
#endif

#define MC_COPY(W) \
static void mc_copy_w##W( uint8_t *dst, int i_dst, uint8_t *src, int i_src, int i_height ) \
{ \
    mc_copy( src, i_src, dst, i_dst, W, i_height ); \
}
MC_COPY( 16 )
MC_COPY( 8 )
MC_COPY( 4 )

void x264_mc_init( int cpu, x264_mc_functions_t *pf )
{
    pf->mc_luma   = mc_luma;
    pf->get_ref   = get_ref;
    pf->mc_chroma = motion_compensation_chroma;

    pf->avg[PIXEL_16x16]= pixel_avg_16x16;
    pf->avg[PIXEL_16x8] = pixel_avg_16x8;
    pf->avg[PIXEL_8x16] = pixel_avg_8x16;
    pf->avg[PIXEL_8x8]  = pixel_avg_8x8;
    pf->avg[PIXEL_8x4]  = pixel_avg_8x4;
    pf->avg[PIXEL_4x8]  = pixel_avg_4x8;
    pf->avg[PIXEL_4x4]  = pixel_avg_4x4;
    pf->avg[PIXEL_4x2]  = pixel_avg_4x2;
    pf->avg[PIXEL_2x4]  = pixel_avg_2x4;
    pf->avg[PIXEL_2x2]  = pixel_avg_2x2;
    
    pf->avg_weight[PIXEL_16x16]= pixel_avg_weight_16x16;
    pf->avg_weight[PIXEL_16x8] = pixel_avg_weight_16x8;
    pf->avg_weight[PIXEL_8x16] = pixel_avg_weight_8x16;
    pf->avg_weight[PIXEL_8x8]  = pixel_avg_weight_8x8;
    pf->avg_weight[PIXEL_8x4]  = pixel_avg_weight_8x4;
    pf->avg_weight[PIXEL_4x8]  = pixel_avg_weight_4x8;
    pf->avg_weight[PIXEL_4x4]  = pixel_avg_weight_4x4;
    pf->avg_weight[PIXEL_4x2]  = pixel_avg_weight_4x2;
    pf->avg_weight[PIXEL_2x4]  = pixel_avg_weight_2x4;
    pf->avg_weight[PIXEL_2x2]  = pixel_avg_weight_2x2;

    pf->copy[PIXEL_16x16] = mc_copy_w16;
    pf->copy[PIXEL_8x8]   = mc_copy_w8;
    pf->copy[PIXEL_4x4]   = mc_copy_w4;

#ifdef HAVE_MMXEXT
    if( cpu&X264_CPU_MMXEXT ) {
        x264_mc_mmxext_init( pf );
        pf->mc_chroma = motion_compensation_chroma_mmxext;
    }
#endif
#ifdef HAVE_SSE2
    if( cpu&X264_CPU_SSE2 )
        x264_mc_sse2_init( pf );
#endif
#ifdef ARCH_PPC
    if( cpu&X264_CPU_ALTIVEC )
        x264_mc_altivec_init( pf );
#endif
}

extern void x264_horizontal_filter_mmxext( uint8_t *dst, int i_dst_stride,
                                           uint8_t *src, int i_src_stride,
                                           int i_width, int i_height );
extern void x264_center_filter_mmxext( uint8_t *dst1, int i_dst1_stride,
                                       uint8_t *dst2, int i_dst2_stride,
                                       uint8_t *src, int i_src_stride,
                                       int i_width, int i_height );

void x264_frame_filter( int cpu, x264_frame_t *frame )
{
    const int x_inc = 16, y_inc = 16;
    const int stride = frame->i_stride[0];
    int x, y;

    pf_mc_t int_h = mc_hh;
    pf_mc_t int_v = mc_hv;
    pf_mc_t int_hv = mc_hc;

#ifdef HAVE_MMXEXT
    if ( cpu & X264_CPU_MMXEXT )
    {
        x264_horizontal_filter_mmxext(frame->filtered[1] - 8 * stride - 8, stride,
            frame->plane[0] - 8 * stride - 8, stride,
            stride - 48, frame->i_lines[0] + 16);
        x264_center_filter_mmxext(frame->filtered[2] - 8 * stride - 8, stride,
            frame->filtered[3] - 8 * stride - 8, stride,
            frame->plane[0] - 8 * stride - 8, stride,
            stride - 48, frame->i_lines[0] + 16);
    }
    else
#endif
    {
        for( y = -8; y < frame->i_lines[0]+8; y += y_inc )
        {
            uint8_t *p_in = frame->plane[0] + y * stride - 8;
            uint8_t *p_h  = frame->filtered[1] + y * stride - 8;
            uint8_t *p_v  = frame->filtered[2] + y * stride - 8;
            uint8_t *p_hv = frame->filtered[3] + y * stride - 8;
            for( x = -8; x < stride - 64 + 8; x += x_inc )
            {
                int_h(  p_in, stride, p_h,  stride, x_inc, y_inc );
                int_v(  p_in, stride, p_v,  stride, x_inc, y_inc );
                int_hv( p_in, stride, p_hv, stride, x_inc, y_inc );

                p_h += x_inc;
                p_v += x_inc;
                p_hv += x_inc;
                p_in += x_inc;
            }
        }
    }

    /* generate integral image:
     * each entry in frame->integral is the sum of all luma samples above and
     * to the left of its location (inclusive).
     * this allows us to calculate the DC of any rectangle by looking only
     * at the corner entries.
     * individual entries will overflow 16 bits, but that's ok:
     * we only need the differences between entries, and those will be correct
     * as long as we don't try to evaluate a rectangle bigger than 16x16.
     * likewise, we don't really have to init the edges to 0, leaving garbage
     * there wouldn't affect the results.*/

    if( frame->integral )
    {
        memset( frame->integral - 32 * stride - 32, 0, stride * sizeof(uint16_t) );
        for( y = -31; y < frame->i_lines[0] + 32; y++ )
        {
            uint8_t  *ref  = frame->plane[0] + y * stride - 32;
            uint16_t *line = frame->integral + y * stride - 32;
            uint16_t v = line[0] = 0;
            for( x = 1; x < stride; x++ )
                line[x] = v += ref[x] + line[x-stride] - line[x-stride-1];
        }
    }
}

void x264_frame_init_lowres( int cpu, x264_frame_t *frame )
{
    // FIXME: tapfilter?
    const int i_stride = frame->i_stride[0];
    const int i_stride2 = frame->i_stride_lowres;
    const int i_width2 = i_stride2 - 64;
    int x, y, i;
    for( y = 0; y < frame->i_lines_lowres - 1; y++ )
    {
        uint8_t *src0 = &frame->plane[0][2*y*i_stride];
        uint8_t *src1 = src0+i_stride;
        uint8_t *src2 = src1+i_stride;
        uint8_t *dst0 = &frame->lowres[0][y*i_stride2];
        uint8_t *dsth = &frame->lowres[1][y*i_stride2];
        uint8_t *dstv = &frame->lowres[2][y*i_stride2];
        uint8_t *dstc = &frame->lowres[3][y*i_stride2];
        for( x = 0; x < i_width2 - 1; x++ )
        {
            dst0[x] = (src0[2*x  ] + src0[2*x+1] + src1[2*x  ] + src1[2*x+1] + 2) >> 2;
            dsth[x] = (src0[2*x+1] + src0[2*x+2] + src1[2*x+1] + src1[2*x+2] + 2) >> 2;
            dstv[x] = (src1[2*x  ] + src1[2*x+1] + src2[2*x  ] + src2[2*x+1] + 2) >> 2;
            dstc[x] = (src1[2*x+1] + src1[2*x+2] + src2[2*x+1] + src2[2*x+2] + 2) >> 2;
        }
        dst0[x] = (src0[2*x  ] + src0[2*x+1] + src1[2*x  ] + src1[2*x+1] + 2) >> 2;
        dstv[x] = (src1[2*x  ] + src1[2*x+1] + src2[2*x  ] + src2[2*x+1] + 2) >> 2;
        dsth[x] = (src0[2*x+1] + src1[2*x+1] + 1) >> 1;
        dstc[x] = (src1[2*x+1] + src2[2*x+1] + 1) >> 1;
    }
    for( i = 0; i < 4; i++ )
        memcpy( &frame->lowres[i][y*i_stride2], &frame->lowres[i][(y-1)*i_stride2], i_width2 );

    for( y = 0; y < 16; y++ )
        for( x = 0; x < 16; x++ )
            frame->i_cost_est[x][y] = -1;

    x264_frame_expand_border_lowres( frame );
}


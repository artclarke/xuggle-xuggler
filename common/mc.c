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

#ifdef HAVE_STDINT_H
#include <stdint.h>
#else
#include <inttypes.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "x264.h"

#include "mc.h"
#include "clip1.h"
#include "frame.h"

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

#if 0
/* mc I+H */
static void mc_xy10( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_width, int i_height )
{
    uint8_t tmp[16*16];
    mc_hh( src, i_src_stride, tmp, i_width, i_width, i_height );
    pixel_avg( dst, i_dst_stride, src, i_src_stride, tmp, i_width, i_width, i_height );
}
static void mc_xy30( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_width, int i_height )
{
    uint8_t tmp[16*16];
    mc_hh( src, i_src_stride, tmp, i_width, i_width, i_height );
    pixel_avg( dst, i_dst_stride, src+1, i_src_stride, tmp, i_width, i_width, i_height );
}
/* mc I+V */
static void mc_xy01( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_width, int i_height )
{
    uint8_t tmp[16*16];
    mc_hv( src, i_src_stride, tmp, i_width, i_width, i_height );
    pixel_avg( dst, i_dst_stride, src, i_src_stride, tmp, i_width, i_width, i_height );
}
static void mc_xy03( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_width, int i_height )
{
    uint8_t tmp[16*16];
    mc_hv( src, i_src_stride, tmp, i_width, i_width, i_height );
    pixel_avg( dst, i_dst_stride, src+i_src_stride, i_src_stride, tmp, i_width, i_width, i_height );
}
/* H+V */
static void mc_xy11( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_width, int i_height )
{
    uint8_t tmp1[16*16];
    uint8_t tmp2[16*16];

    mc_hv( src, i_src_stride, tmp1, i_width, i_width, i_height );
    mc_hh( src, i_src_stride, tmp2, i_width, i_width, i_height );
    pixel_avg( dst, i_dst_stride, tmp1, i_width, tmp2, i_width, i_width, i_height );
}
static void mc_xy31( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_width, int i_height )
{
    uint8_t tmp1[16*16];
    uint8_t tmp2[16*16];

    mc_hv( src+1, i_src_stride, tmp1, i_width, i_width, i_height );
    mc_hh( src,   i_src_stride, tmp2, i_width, i_width, i_height );
    pixel_avg( dst, i_dst_stride, tmp1, i_width, tmp2, i_width, i_width, i_height );
}
static void mc_xy13( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_width, int i_height )
{
    uint8_t tmp1[16*16];
    uint8_t tmp2[16*16];

    mc_hv( src,              i_src_stride, tmp1, i_width, i_width, i_height );
    mc_hh( src+i_src_stride, i_src_stride, tmp2, i_width, i_width, i_height );
    pixel_avg( dst, i_dst_stride, tmp1, i_width, tmp2, i_width, i_width, i_height );
}
static void mc_xy33( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_width, int i_height )
{
    uint8_t tmp1[16*16];
    uint8_t tmp2[16*16];

    mc_hv( src+1,            i_src_stride, tmp1, i_width, i_width, i_height );
    mc_hh( src+i_src_stride, i_src_stride, tmp2, i_width, i_width, i_height );
    pixel_avg( dst, i_dst_stride, tmp1, i_width, tmp2, i_width, i_width, i_height );
}
static void mc_xy21( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_width, int i_height )
{
    uint8_t tmp1[16*16];
    uint8_t tmp2[16*16];

    mc_hc( src, i_src_stride, tmp1, i_width, i_width, i_height );
    mc_hh( src, i_src_stride, tmp2, i_width, i_width, i_height );
    pixel_avg( dst, i_dst_stride, tmp1, i_width, tmp2, i_width, i_width, i_height );
}
static void mc_xy12( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_width, int i_height )
{
    uint8_t tmp1[16*16];
    uint8_t tmp2[16*16];

    mc_hc( src, i_src_stride, tmp1, i_width, i_width, i_height );
    mc_hv( src, i_src_stride, tmp2, i_width, i_width, i_height );
    pixel_avg( dst, i_dst_stride, tmp1, i_width, tmp2, i_width, i_width, i_height );
}
static void mc_xy32( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_width, int i_height )
{
    uint8_t tmp1[16*16];
    uint8_t tmp2[16*16];

    mc_hc( src,   i_src_stride, tmp1, i_width, i_width, i_height );
    mc_hv( src+1, i_src_stride, tmp2, i_width, i_width, i_height );
    pixel_avg( dst, i_dst_stride, tmp1, i_width, tmp2, i_width, i_width, i_height );
}
static void mc_xy23( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_width, int i_height )
{
    uint8_t tmp1[16*16];
    uint8_t tmp2[16*16];

    mc_hc( src,              i_src_stride, tmp1, i_width, i_width, i_height );
    mc_hh( src+i_src_stride, i_src_stride, tmp2, i_width, i_width, i_height );
    pixel_avg( dst, i_dst_stride, tmp1, i_width, tmp2, i_width, i_width, i_height );
}

static void motion_compensation_luma( uint8_t *src, int i_src_stride,
                                      uint8_t *dst, int i_dst_stride,
                                      int mvx,int mvy,
                                      int i_width, int i_height )
{
    static pf_mc_t pf_mc[4][4] =    /*XXX [dqy][dqx] */
    {
        { mc_copy,  mc_xy10,    mc_hh,      mc_xy30 },
        { mc_xy01,  mc_xy11,    mc_xy21,    mc_xy31 },
        { mc_hv,    mc_xy12,    mc_hc,      mc_xy32 },
        { mc_xy03,  mc_xy13,    mc_xy23,    mc_xy33 },
    };

    src += (mvy >> 2) * i_src_stride + (mvx >> 2);
    pf_mc[mvy&0x03][mvx&0x03]( src, i_src_stride, dst, i_dst_stride, i_width, i_height );
}
#endif

static void mc_luma( uint8_t *src[4], int i_src_stride,
                     uint8_t *dst,    int i_dst_stride,
                     int mvx,int mvy,
                     int i_width, int i_height )
{
    uint8_t *src1, *src2;

    int correction = (mvx&1) && (mvy&1) && ((mvx&2) ^ (mvy&2));
    int hpel1x = mvx>>1;
    int hpel1y = (mvy+1-correction)>>1;
    int filter1 = (hpel1x & 1) + ( (hpel1y & 1) << 1 );

    src1 = src[filter1] + (hpel1y >> 1) * i_src_stride + (hpel1x >> 1);

    if ( (mvx|mvy) & 1 ) /* qpel interpolation needed */
    {
        int hpel2x = (mvx+1)>>1;
        int hpel2y = (mvy+correction)>>1;
        int filter2 = (hpel2x & 1) + ( (hpel2y & 1) <<1 );

        src2 = src[filter2] + (hpel2y >> 1) * i_src_stride + (hpel2x >> 1);
    
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
    uint8_t *src1, *src2;

    int correction = (mvx&1) && (mvy&1) && ((mvx&2) ^ (mvy&2));
    int hpel1x = mvx>>1;
    int hpel1y = (mvy+1-correction)>>1;
    int filter1 = (hpel1x & 1) + ( (hpel1y & 1) << 1 );

    src1 = src[filter1] + (hpel1y >> 1) * i_src_stride + (hpel1x >> 1);

    if ( (mvx|mvy) & 1 ) /* qpel interpolation needed */
    {
        int hpel2x = (mvx+1)>>1;
        int hpel2y = (mvy+correction)>>1;
        int filter2 = (hpel2x & 1) + ( (hpel2y & 1) <<1 );

        src2 = src[filter2] + (hpel2y >> 1) * i_src_stride + (hpel2x >> 1);
    
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
static void motion_compensation_chroma_sse( uint8_t *src, int i_src_stride,
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
        
        x264_mc_chroma_sse(src, i_src_stride, dst, i_dst_stride,
                              d8x, d8y, i_height, i_width);
    }
}
#endif

void x264_mc_init( int cpu, x264_mc_functions_t *pf )
{
    pf->mc_luma   = mc_luma;
    pf->get_ref   = get_ref;
    pf->mc_chroma = motion_compensation_chroma;

#ifdef HAVE_MMXEXT
    if( cpu&X264_CPU_MMXEXT ) {
        x264_mc_mmxext_init( pf );
        pf->mc_chroma = motion_compensation_chroma_sse;
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

#if 0
void get_funcs_mmx(pf_mc_t*, pf_mc_t*, pf_mc_t*);
void get_funcs_sse2(pf_mc_t*, pf_mc_t*, pf_mc_t*);
#endif

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

#if 0
#ifdef HAVE_MMXEXT
    if( cpu&X264_CPU_MMXEXT )
        get_funcs_mmx(&int_h, &int_v, &int_hv);
#endif

#ifdef HAVE_SSE2
    if( cpu&X264_CPU_SSE2 )
        get_funcs_sse2(&int_h, &int_v, &int_hv);
#endif
#endif

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


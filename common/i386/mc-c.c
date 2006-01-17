/*****************************************************************************
 * mc.c: h264 encoder library (Motion Compensation)
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar
 * $Id: mc-c.c,v 1.5 2004/06/18 01:59:58 chenm001 Exp $
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
#include <stdio.h>
#include <string.h>

#include "common/common.h"

/* NASM functions */
extern void x264_pixel_avg_w4_mmxext( uint8_t *,  int, uint8_t *, int, uint8_t *, int, int );
extern void x264_pixel_avg_w8_mmxext( uint8_t *,  int, uint8_t *, int, uint8_t *, int, int );
extern void x264_pixel_avg_w16_mmxext( uint8_t *,  int, uint8_t *, int, uint8_t *, int, int );
extern void x264_pixel_avg_w16_sse2( uint8_t *,  int, uint8_t *, int, uint8_t *, int, int );
extern void x264_pixel_avg_weight_4x4_mmxext( uint8_t *, int, uint8_t *, int, int );
extern void x264_pixel_avg_weight_w8_mmxext( uint8_t *, int, uint8_t *, int, int, int );
extern void x264_pixel_avg_weight_w16_mmxext( uint8_t *, int, uint8_t *, int, int, int );
extern void x264_mc_copy_w4_mmxext( uint8_t *, int, uint8_t *, int, int );
extern void x264_mc_copy_w8_mmxext( uint8_t *, int, uint8_t *, int, int );
extern void x264_mc_copy_w16_mmxext( uint8_t *, int, uint8_t *, int, int );
extern void x264_mc_copy_w16_sse2( uint8_t *, int, uint8_t *, int, int );

#define AVG(W,H) \
static void x264_pixel_avg_ ## W ## x ## H ## _mmxext( uint8_t *dst, int i_dst, uint8_t *src, int i_src ) \
{ \
    x264_pixel_avg_w ## W ## _mmxext( dst, i_dst, dst, i_dst, src, i_src, H ); \
}
AVG(16,16)
AVG(16,8)
AVG(8,16)
AVG(8,8)
AVG(8,4)
AVG(4,8)
AVG(4,4)
AVG(4,2)

#define AVG_WEIGHT(W,H) \
void x264_pixel_avg_weight_ ## W ## x ## H ## _mmxext( uint8_t *dst, int i_dst, uint8_t *src, int i_src, int i_weight_dst ) \
{ \
    x264_pixel_avg_weight_w ## W ## _mmxext( dst, i_dst, src, i_src, i_weight_dst, H ); \
}
AVG_WEIGHT(16,16)
AVG_WEIGHT(16,8)
AVG_WEIGHT(8,16)
AVG_WEIGHT(8,8)
AVG_WEIGHT(8,4)

void mc_luma_mmx( uint8_t *src[4], int i_src_stride,
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

        switch(i_width) {
        case 4:
            x264_pixel_avg_w4_mmxext( dst, i_dst_stride, src1, i_src_stride,
                          src2, i_src_stride, i_height );
            break;
        case 8:
            x264_pixel_avg_w8_mmxext( dst, i_dst_stride, src1, i_src_stride,
                          src2, i_src_stride, i_height );
            break;
        case 16:
        default:
            x264_pixel_avg_w16_mmxext(dst, i_dst_stride, src1, i_src_stride,
                          src2, i_src_stride, i_height );
        }
    }
    else
    {
        switch(i_width) {
        case 4:
            x264_mc_copy_w4_mmxext( src1, i_src_stride, dst, i_dst_stride, i_height );
            break;
        case 8:
            x264_mc_copy_w8_mmxext( src1, i_src_stride, dst, i_dst_stride, i_height );
            break;
        case 16:
            x264_mc_copy_w16_mmxext( src1, i_src_stride, dst, i_dst_stride, i_height );
            break;
        }
    }
}

uint8_t *get_ref_mmx( uint8_t *src[4], int i_src_stride,
                      uint8_t *dst,   int *i_dst_stride,
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
    
        switch(i_width) {
        case 4:
            x264_pixel_avg_w4_mmxext( dst, *i_dst_stride, src1, i_src_stride,
                          src2, i_src_stride, i_height );
            break;
        case 8:
            x264_pixel_avg_w8_mmxext( dst, *i_dst_stride, src1, i_src_stride,
                          src2, i_src_stride, i_height );
            break;
        case 16:
        default:
            x264_pixel_avg_w16_mmxext(dst, *i_dst_stride, src1, i_src_stride,
                          src2, i_src_stride, i_height );
        }
        return dst;

    }
    else
    {
        *i_dst_stride = i_src_stride;
        return src1;
    }
}


void x264_mc_mmxext_init( x264_mc_functions_t *pf )
{
    pf->mc_luma   = mc_luma_mmx;
    pf->get_ref   = get_ref_mmx;

    pf->avg[PIXEL_16x16] = x264_pixel_avg_16x16_mmxext;
    pf->avg[PIXEL_16x8]  = x264_pixel_avg_16x8_mmxext;
    pf->avg[PIXEL_8x16]  = x264_pixel_avg_8x16_mmxext;
    pf->avg[PIXEL_8x8]   = x264_pixel_avg_8x8_mmxext;
    pf->avg[PIXEL_8x4]   = x264_pixel_avg_8x4_mmxext;
    pf->avg[PIXEL_4x8]   = x264_pixel_avg_4x8_mmxext;
    pf->avg[PIXEL_4x4]   = x264_pixel_avg_4x4_mmxext;
    pf->avg[PIXEL_4x2]   = x264_pixel_avg_4x2_mmxext;
    
    pf->avg_weight[PIXEL_16x16] = x264_pixel_avg_weight_16x16_mmxext;
    pf->avg_weight[PIXEL_16x8]  = x264_pixel_avg_weight_16x8_mmxext;
    pf->avg_weight[PIXEL_8x16]  = x264_pixel_avg_weight_8x16_mmxext;
    pf->avg_weight[PIXEL_8x8]   = x264_pixel_avg_weight_8x8_mmxext;
    pf->avg_weight[PIXEL_8x4]   = x264_pixel_avg_weight_8x4_mmxext;
    pf->avg_weight[PIXEL_4x4]   = x264_pixel_avg_weight_4x4_mmxext;
    // avg_weight_4x8 is rare and 4x2 is not used
}
void x264_mc_sse2_init( x264_mc_functions_t *pf )
{
    /* todo: use sse2 */
    pf->mc_luma   = mc_luma_mmx;
    pf->get_ref   = get_ref_mmx;
}

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
extern void x264_pixel_avg_16x16_mmxext( uint8_t *, int, uint8_t *, int );
extern void x264_pixel_avg_16x8_mmxext( uint8_t *, int, uint8_t *, int );
extern void x264_pixel_avg_8x16_mmxext( uint8_t *, int, uint8_t *, int );
extern void x264_pixel_avg_8x8_mmxext( uint8_t *, int, uint8_t *, int );
extern void x264_pixel_avg_8x4_mmxext( uint8_t *, int, uint8_t *, int );
extern void x264_pixel_avg_4x8_mmxext( uint8_t *, int, uint8_t *, int );
extern void x264_pixel_avg_4x4_mmxext( uint8_t *, int, uint8_t *, int );
extern void x264_pixel_avg_4x2_mmxext( uint8_t *, int, uint8_t *, int );
extern void x264_pixel_avg2_w4_mmxext( uint8_t *, int, uint8_t *, int, uint8_t *, int );
extern void x264_pixel_avg2_w8_mmxext( uint8_t *, int, uint8_t *, int, uint8_t *, int );
extern void x264_pixel_avg2_w16_mmxext( uint8_t *, int, uint8_t *, int, uint8_t *, int );
extern void x264_pixel_avg2_w20_mmxext( uint8_t *, int, uint8_t *, int, uint8_t *, int );
extern void x264_pixel_avg_weight_4x4_mmxext( uint8_t *, int, uint8_t *, int, int );
extern void x264_pixel_avg_weight_w8_mmxext( uint8_t *, int, uint8_t *, int, int, int );
extern void x264_pixel_avg_weight_w16_mmxext( uint8_t *, int, uint8_t *, int, int, int );
extern void x264_mc_copy_w4_mmx( uint8_t *, int, uint8_t *, int, int );
extern void x264_mc_copy_w8_mmx( uint8_t *, int, uint8_t *, int, int );
extern void x264_mc_copy_w16_mmx( uint8_t *, int, uint8_t *, int, int );
extern void x264_mc_copy_w16_sse2( uint8_t *, int, uint8_t *, int, int );
extern void x264_plane_copy_mmxext( uint8_t *, int, uint8_t *, int, int w, int h);
extern void x264_prefetch_fenc_mmxext( uint8_t *, int, uint8_t *, int, int );
extern void x264_prefetch_ref_mmxext( uint8_t *, int, int );
extern void x264_hpel_filter_mmxext( uint8_t *dsth, uint8_t *dstv, uint8_t *dstc, uint8_t *src,
                                     int i_stride, int i_width, int i_height );

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

static void (* const x264_pixel_avg_wtab_mmxext[6])( uint8_t *, int, uint8_t *, int, uint8_t *, int ) =
{
    NULL,
    x264_pixel_avg2_w4_mmxext,
    x264_pixel_avg2_w8_mmxext,
    x264_pixel_avg2_w16_mmxext,
    x264_pixel_avg2_w16_mmxext,
    x264_pixel_avg2_w20_mmxext,
};
static void (* const x264_mc_copy_wtab_mmx[5])( uint8_t *, int, uint8_t *, int, int ) =
{
    NULL,
    x264_mc_copy_w4_mmx,
    x264_mc_copy_w8_mmx,
    NULL,
    x264_mc_copy_w16_mmx
};
static const int hpel_ref0[16] = {0,1,1,1,0,1,1,1,2,3,3,3,0,1,1,1};
static const int hpel_ref1[16] = {0,0,0,0,2,2,3,2,2,2,3,2,2,2,3,2};

void mc_luma_mmx( uint8_t *dst,    int i_dst_stride,
                  uint8_t *src[4], int i_src_stride,
                  int mvx, int mvy,
                  int i_width, int i_height )
{
    int qpel_idx = ((mvy&3)<<2) + (mvx&3);
    int offset = (mvy>>2)*i_src_stride + (mvx>>2);
    uint8_t *src1 = src[hpel_ref0[qpel_idx]] + offset + ((mvy&3) == 3) * i_src_stride;

    if( qpel_idx & 5 ) /* qpel interpolation needed */
    {
        uint8_t *src2 = src[hpel_ref1[qpel_idx]] + offset + ((mvx&3) == 3);
        x264_pixel_avg_wtab_mmxext[i_width>>2](
                dst, i_dst_stride, src1, i_src_stride,
                src2, i_height );
    }
    else
    {
        x264_mc_copy_wtab_mmx[i_width>>2](
                dst, i_dst_stride, src1, i_src_stride, i_height );
    }
}

uint8_t *get_ref_mmx( uint8_t *dst,   int *i_dst_stride,
                      uint8_t *src[4], int i_src_stride,
                      int mvx, int mvy,
                      int i_width, int i_height )
{
    int qpel_idx = ((mvy&3)<<2) + (mvx&3);
    int offset = (mvy>>2)*i_src_stride + (mvx>>2);
    uint8_t *src1 = src[hpel_ref0[qpel_idx]] + offset + ((mvy&3) == 3) * i_src_stride;

    if( qpel_idx & 5 ) /* qpel interpolation needed */
    {
        uint8_t *src2 = src[hpel_ref1[qpel_idx]] + offset + ((mvx&3) == 3);
        x264_pixel_avg_wtab_mmxext[i_width>>2](
                dst, *i_dst_stride, src1, i_src_stride,
                src2, i_height );
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
    pf->mc_luma = mc_luma_mmx;
    pf->get_ref = get_ref_mmx;

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

    pf->copy[PIXEL_16x16] = x264_mc_copy_w16_mmx;
    pf->copy[PIXEL_8x8]   = x264_mc_copy_w8_mmx;
    pf->copy[PIXEL_4x4]   = x264_mc_copy_w4_mmx;

    pf->plane_copy = x264_plane_copy_mmxext;
    pf->hpel_filter = x264_hpel_filter_mmxext;

    pf->prefetch_fenc = x264_prefetch_fenc_mmxext;
    pf->prefetch_ref  = x264_prefetch_ref_mmxext;
}
void x264_mc_sse2_init( x264_mc_functions_t *pf )
{
    /* todo: use sse2 */
}

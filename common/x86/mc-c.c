/*****************************************************************************
 * mc-c.c: h264 encoder library (Motion Compensation)
 *****************************************************************************
 * Copyright (C) 2003-2008 x264 project
 *
 * Authors: Laurent Aimar <fenrir@via.ecp.fr>
 *          Loren Merritt <lorenm@u.washington.edu>
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
extern void x264_pixel_avg_16x16_sse2( uint8_t *, int, uint8_t *, int );
extern void x264_pixel_avg_16x8_sse2( uint8_t *, int, uint8_t *, int );
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
extern void x264_pixel_avg2_w12_mmxext( uint8_t *, int, uint8_t *, int, uint8_t *, int );
extern void x264_pixel_avg2_w16_mmxext( uint8_t *, int, uint8_t *, int, uint8_t *, int );
extern void x264_pixel_avg2_w20_mmxext( uint8_t *, int, uint8_t *, int, uint8_t *, int );
extern void x264_pixel_avg2_w16_sse2( uint8_t *, int, uint8_t *, int, uint8_t *, int );
extern void x264_pixel_avg2_w20_sse2( uint8_t *, int, uint8_t *, int, uint8_t *, int );
extern void x264_mc_copy_w4_mmx( uint8_t *, int, uint8_t *, int, int );
extern void x264_mc_copy_w8_mmx( uint8_t *, int, uint8_t *, int, int );
extern void x264_mc_copy_w16_mmx( uint8_t *, int, uint8_t *, int, int );
extern void x264_mc_copy_w16_sse2( uint8_t *, int, uint8_t *, int, int );
extern void x264_pixel_avg_weight_4x4_mmxext( uint8_t *, int, uint8_t *, int, int );
extern void x264_pixel_avg_weight_w8_mmxext( uint8_t *, int, uint8_t *, int, int, int );
extern void x264_pixel_avg_weight_w16_mmxext( uint8_t *, int, uint8_t *, int, int, int );
extern void x264_prefetch_fenc_mmxext( uint8_t *, int, uint8_t *, int, int );
extern void x264_prefetch_ref_mmxext( uint8_t *, int, int );
extern void x264_mc_chroma_mmxext( uint8_t *src, int i_src_stride,
                                   uint8_t *dst, int i_dst_stride,
                                   int dx, int dy, int i_width, int i_height );
extern void x264_plane_copy_mmxext( uint8_t *, int, uint8_t *, int, int w, int h);
extern void x264_hpel_filter_mmxext( uint8_t *dsth, uint8_t *dstv, uint8_t *dstc, uint8_t *src,
                                     int i_stride, int i_width, int i_height );
extern void *x264_memcpy_aligned_mmx( void * dst, const void * src, size_t n );
extern void *x264_memcpy_aligned_sse2( void * dst, const void * src, size_t n );

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
    x264_pixel_avg2_w12_mmxext,
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
static void (* const x264_pixel_avg_wtab_sse2[6])( uint8_t *, int, uint8_t *, int, uint8_t *, int ) =
{
    NULL,
    x264_pixel_avg2_w4_mmxext,
    x264_pixel_avg2_w8_mmxext,
    x264_pixel_avg2_w12_mmxext,
    x264_pixel_avg2_w16_sse2,
    x264_pixel_avg2_w20_sse2,
};
static void (* const x264_mc_copy_wtab_sse2[5])( uint8_t *, int, uint8_t *, int, int ) =
{
    NULL,
    x264_mc_copy_w4_mmx,
    x264_mc_copy_w8_mmx,
    NULL,
    x264_mc_copy_w16_sse2,
};
static const int hpel_ref0[16] = {0,1,1,1,0,1,1,1,2,3,3,3,0,1,1,1};
static const int hpel_ref1[16] = {0,0,0,0,2,2,3,2,2,2,3,2,2,2,3,2};

#define MC_LUMA(name,instr1,instr2)\
void mc_luma_##name( uint8_t *dst,    int i_dst_stride,\
                  uint8_t *src[4], int i_src_stride,\
                  int mvx, int mvy,\
                  int i_width, int i_height )\
{\
    int qpel_idx = ((mvy&3)<<2) + (mvx&3);\
    int offset = (mvy>>2)*i_src_stride + (mvx>>2);\
    uint8_t *src1 = src[hpel_ref0[qpel_idx]] + offset + ((mvy&3) == 3) * i_src_stride;\
    if( qpel_idx & 5 ) /* qpel interpolation needed */\
    {\
        uint8_t *src2 = src[hpel_ref1[qpel_idx]] + offset + ((mvx&3) == 3);\
        x264_pixel_avg_wtab_##instr1[i_width>>2](\
                dst, i_dst_stride, src1, i_src_stride,\
                src2, i_height );\
    }\
    else\
    {\
        x264_mc_copy_wtab_##instr2[i_width>>2](\
                dst, i_dst_stride, src1, i_src_stride, i_height );\
    }\
}

MC_LUMA(mmxext,mmxext,mmx)
MC_LUMA(sse2,sse2,sse2)

#define GET_REF(name)\
uint8_t *get_ref_##name( uint8_t *dst,   int *i_dst_stride,\
                         uint8_t *src[4], int i_src_stride,\
                         int mvx, int mvy,\
                         int i_width, int i_height )\
{\
    int qpel_idx = ((mvy&3)<<2) + (mvx&3);\
    int offset = (mvy>>2)*i_src_stride + (mvx>>2);\
    uint8_t *src1 = src[hpel_ref0[qpel_idx]] + offset + ((mvy&3) == 3) * i_src_stride;\
    if( qpel_idx & 5 ) /* qpel interpolation needed */\
    {\
        uint8_t *src2 = src[hpel_ref1[qpel_idx]] + offset + ((mvx&3) == 3);\
        x264_pixel_avg_wtab_##name[i_width>>2](\
                dst, *i_dst_stride, src1, i_src_stride,\
                src2, i_height );\
        return dst;\
    }\
    else\
    {\
        *i_dst_stride = i_src_stride;\
        return src1;\
    }\
}

GET_REF(mmxext)
GET_REF(sse2)

void x264_mc_init_mmx( int cpu, x264_mc_functions_t *pf )
{
    if( !(cpu&X264_CPU_MMX) )
        return;

    pf->copy[PIXEL_16x16] = x264_mc_copy_w16_mmx;
    pf->copy[PIXEL_8x8]   = x264_mc_copy_w8_mmx;
    pf->copy[PIXEL_4x4]   = x264_mc_copy_w4_mmx;
    pf->memcpy_aligned = x264_memcpy_aligned_mmx;

    if( !(cpu&X264_CPU_MMXEXT) )
        return;

    pf->mc_luma = mc_luma_mmxext;
    pf->get_ref = get_ref_mmxext;
    pf->mc_chroma = x264_mc_chroma_mmxext;

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

    pf->plane_copy = x264_plane_copy_mmxext;
    pf->hpel_filter = x264_hpel_filter_mmxext;

    pf->prefetch_fenc = x264_prefetch_fenc_mmxext;
    pf->prefetch_ref  = x264_prefetch_ref_mmxext;

    if( !(cpu&X264_CPU_SSE2) )
        return;

    pf->memcpy_aligned = x264_memcpy_aligned_sse2;

    // disable on AMD processors since it is slower
    if( cpu&X264_CPU_3DNOW )
        return;

    pf->mc_luma = mc_luma_sse2;
    pf->get_ref = get_ref_sse2;
    pf->avg[PIXEL_16x16] = x264_pixel_avg_16x16_sse2;
    pf->avg[PIXEL_16x8]  = x264_pixel_avg_16x8_sse2;
}

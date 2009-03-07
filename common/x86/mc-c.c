/*****************************************************************************
 * mc-c.c: h264 encoder library (Motion Compensation)
 *****************************************************************************
 * Copyright (C) 2003-2008 x264 project
 *
 * Authors: Laurent Aimar <fenrir@via.ecp.fr>
 *          Loren Merritt <lorenm@u.washington.edu>
 *          Jason Garrett-Glaser <darkshikari@gmail.com>
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "common/common.h"
#include "mc.h"

#define DECL_SUF( func, args )\
    void func##_mmxext args;\
    void func##_sse2 args;\
    void func##_ssse3 args;

DECL_SUF( x264_pixel_avg_16x16, ( uint8_t *, int, uint8_t *, int, uint8_t *, int, int ))
DECL_SUF( x264_pixel_avg_16x8,  ( uint8_t *, int, uint8_t *, int, uint8_t *, int, int ))
DECL_SUF( x264_pixel_avg_8x16,  ( uint8_t *, int, uint8_t *, int, uint8_t *, int, int ))
DECL_SUF( x264_pixel_avg_8x8,   ( uint8_t *, int, uint8_t *, int, uint8_t *, int, int ))
DECL_SUF( x264_pixel_avg_8x4,   ( uint8_t *, int, uint8_t *, int, uint8_t *, int, int ))
DECL_SUF( x264_pixel_avg_4x8,   ( uint8_t *, int, uint8_t *, int, uint8_t *, int, int ))
DECL_SUF( x264_pixel_avg_4x4,   ( uint8_t *, int, uint8_t *, int, uint8_t *, int, int ))
DECL_SUF( x264_pixel_avg_4x2,   ( uint8_t *, int, uint8_t *, int, uint8_t *, int, int ))
extern void x264_mc_copy_w4_mmx( uint8_t *, int, uint8_t *, int, int );
extern void x264_mc_copy_w8_mmx( uint8_t *, int, uint8_t *, int, int );
extern void x264_mc_copy_w16_mmx( uint8_t *, int, uint8_t *, int, int );
extern void x264_mc_copy_w16_sse2( uint8_t *, int, uint8_t *, int, int );
extern void x264_mc_copy_w16_sse3( uint8_t *, int, uint8_t *, int, int );
extern void x264_mc_copy_w16_aligned_sse2( uint8_t *, int, uint8_t *, int, int );
extern void x264_prefetch_fenc_mmxext( uint8_t *, int, uint8_t *, int, int );
extern void x264_prefetch_ref_mmxext( uint8_t *, int, int );
extern void x264_mc_chroma_mmxext( uint8_t *src, int i_src_stride,
                                   uint8_t *dst, int i_dst_stride,
                                   int dx, int dy, int i_width, int i_height );
extern void x264_mc_chroma_sse2( uint8_t *src, int i_src_stride,
                                 uint8_t *dst, int i_dst_stride,
                                 int dx, int dy, int i_width, int i_height );
extern void x264_mc_chroma_ssse3( uint8_t *src, int i_src_stride,
                                  uint8_t *dst, int i_dst_stride,
                                  int dx, int dy, int i_width, int i_height );
extern void x264_plane_copy_mmxext( uint8_t *, int, uint8_t *, int, int w, int h);
extern void *x264_memcpy_aligned_mmx( void * dst, const void * src, size_t n );
extern void *x264_memcpy_aligned_sse2( void * dst, const void * src, size_t n );
extern void x264_memzero_aligned_mmx( void * dst, int n );
extern void x264_memzero_aligned_sse2( void * dst, int n );
extern void x264_integral_init4h_sse4( uint16_t *sum, uint8_t *pix, int stride );
extern void x264_integral_init8h_sse4( uint16_t *sum, uint8_t *pix, int stride );
extern void x264_integral_init4v_mmx( uint16_t *sum8, uint16_t *sum4, int stride );
extern void x264_integral_init4v_sse2( uint16_t *sum8, uint16_t *sum4, int stride );
extern void x264_integral_init8v_mmx( uint16_t *sum8, int stride );
extern void x264_integral_init8v_sse2( uint16_t *sum8, int stride );
#define LOWRES(cpu) \
extern void x264_frame_init_lowres_core_##cpu( uint8_t *src0, uint8_t *dst0, uint8_t *dsth, uint8_t *dstv, uint8_t *dstc,\
                                               int src_stride, int dst_stride, int width, int height );
LOWRES(mmxext)
LOWRES(cache32_mmxext)
LOWRES(sse2)
LOWRES(ssse3)

#define PIXEL_AVG_W(width,cpu)\
extern void x264_pixel_avg2_w##width##_##cpu( uint8_t *, int, uint8_t *, int, uint8_t *, int );
/* This declares some functions that don't exist, but that isn't a problem. */
#define PIXEL_AVG_WALL(cpu)\
PIXEL_AVG_W(4,cpu); PIXEL_AVG_W(8,cpu); PIXEL_AVG_W(12,cpu); PIXEL_AVG_W(16,cpu); PIXEL_AVG_W(20,cpu);

PIXEL_AVG_WALL(mmxext)
PIXEL_AVG_WALL(cache32_mmxext)
PIXEL_AVG_WALL(cache64_mmxext)
PIXEL_AVG_WALL(cache64_sse2)
PIXEL_AVG_WALL(sse2)
PIXEL_AVG_WALL(sse2_misalign)

#define PIXEL_AVG_WTAB(instr, name1, name2, name3, name4, name5)\
static void (* const x264_pixel_avg_wtab_##instr[6])( uint8_t *, int, uint8_t *, int, uint8_t *, int ) =\
{\
    NULL,\
    x264_pixel_avg2_w4_##name1,\
    x264_pixel_avg2_w8_##name2,\
    x264_pixel_avg2_w12_##name3,\
    x264_pixel_avg2_w16_##name4,\
    x264_pixel_avg2_w20_##name5,\
};

/* w16 sse2 is faster than w12 mmx as long as the cacheline issue is resolved */
#define x264_pixel_avg2_w12_cache64_sse2 x264_pixel_avg2_w16_cache64_sse2
#define x264_pixel_avg2_w12_sse3         x264_pixel_avg2_w16_sse3
#define x264_pixel_avg2_w12_sse2         x264_pixel_avg2_w16_sse2

PIXEL_AVG_WTAB(mmxext, mmxext, mmxext, mmxext, mmxext, mmxext)
#ifdef ARCH_X86
PIXEL_AVG_WTAB(cache32_mmxext, mmxext, cache32_mmxext, cache32_mmxext, cache32_mmxext, cache32_mmxext)
PIXEL_AVG_WTAB(cache64_mmxext, mmxext, cache64_mmxext, cache64_mmxext, cache64_mmxext, cache64_mmxext)
#endif
PIXEL_AVG_WTAB(sse2, mmxext, mmxext, sse2, sse2, sse2)
PIXEL_AVG_WTAB(sse2_misalign, mmxext, mmxext, sse2, sse2, sse2_misalign)
PIXEL_AVG_WTAB(cache64_sse2, mmxext, cache64_mmxext, cache64_sse2, cache64_sse2, cache64_sse2)

#define MC_COPY_WTAB(instr, name1, name2, name3)\
static void (* const x264_mc_copy_wtab_##instr[5])( uint8_t *, int, uint8_t *, int, int ) =\
{\
    NULL,\
    x264_mc_copy_w4_##name1,\
    x264_mc_copy_w8_##name2,\
    NULL,\
    x264_mc_copy_w16_##name3,\
};

MC_COPY_WTAB(mmx,mmx,mmx,mmx)
MC_COPY_WTAB(sse2,mmx,mmx,sse2)

static const int hpel_ref0[16] = {0,1,1,1,0,1,1,1,2,3,3,3,0,1,1,1};
static const int hpel_ref1[16] = {0,0,0,0,2,2,3,2,2,2,3,2,2,2,3,2};

#define MC_LUMA(name,instr1,instr2)\
static void mc_luma_##name( uint8_t *dst,    int i_dst_stride,\
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
#ifdef ARCH_X86
MC_LUMA(cache32_mmxext,cache32_mmxext,mmx)
MC_LUMA(cache64_mmxext,cache64_mmxext,mmx)
#endif
MC_LUMA(sse2,sse2,sse2)
MC_LUMA(cache64_sse2,cache64_sse2,sse2)

#define GET_REF(name)\
static uint8_t *get_ref_##name( uint8_t *dst,   int *i_dst_stride,\
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
#ifdef ARCH_X86
GET_REF(cache32_mmxext)
GET_REF(cache64_mmxext)
#endif
GET_REF(sse2)
GET_REF(sse2_misalign)
GET_REF(cache64_sse2)

#define HPEL(align, cpu, cpuv, cpuc, cpuh)\
void x264_hpel_filter_v_##cpuv( uint8_t *dst, uint8_t *src, int16_t *buf, int stride, int width);\
void x264_hpel_filter_c_##cpuc( uint8_t *dst, int16_t *buf, int width );\
void x264_hpel_filter_h_##cpuh( uint8_t *dst, uint8_t *src, int width );\
void x264_sfence( void );\
static void x264_hpel_filter_##cpu( uint8_t *dsth, uint8_t *dstv, uint8_t *dstc, uint8_t *src,\
                             int stride, int width, int height, int16_t *buf )\
{\
    int realign = (intptr_t)src & (align-1);\
    src -= realign;\
    dstv -= realign;\
    dstc -= realign;\
    dsth -= realign;\
    width += realign;\
    while( height-- )\
    {\
        x264_hpel_filter_v_##cpuv( dstv, src, buf+8, stride, width );\
        x264_hpel_filter_c_##cpuc( dstc, buf+8, width );\
        x264_hpel_filter_h_##cpuh( dsth, src, width );\
        dsth += stride;\
        dstv += stride;\
        dstc += stride;\
        src  += stride;\
    }\
    x264_sfence();\
}

HPEL(8, mmxext, mmxext, mmxext, mmxext)
HPEL(16, sse2_amd, mmxext, mmxext, sse2)
#ifdef ARCH_X86_64
void x264_hpel_filter_sse2( uint8_t *dsth, uint8_t *dstv, uint8_t *dstc, uint8_t *src, int stride, int width, int height, int16_t *buf );
void x264_hpel_filter_ssse3( uint8_t *dsth, uint8_t *dstv, uint8_t *dstc, uint8_t *src, int stride, int width, int height, int16_t *buf );
#else
HPEL(16, sse2, sse2, sse2, sse2)
HPEL(16, ssse3, ssse3, ssse3, ssse3)

#endif
HPEL(16, sse2_misalign, sse2, sse2_misalign, sse2)

void x264_mc_init_mmx( int cpu, x264_mc_functions_t *pf )
{
    if( !(cpu&X264_CPU_MMX) )
        return;

    pf->copy_16x16_unaligned = x264_mc_copy_w16_mmx;
    pf->copy[PIXEL_16x16] = x264_mc_copy_w16_mmx;
    pf->copy[PIXEL_8x8]   = x264_mc_copy_w8_mmx;
    pf->copy[PIXEL_4x4]   = x264_mc_copy_w4_mmx;
    pf->memcpy_aligned = x264_memcpy_aligned_mmx;
    pf->memzero_aligned = x264_memzero_aligned_mmx;
    pf->integral_init4v = x264_integral_init4v_mmx;
    pf->integral_init8v = x264_integral_init8v_mmx;

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

    pf->plane_copy = x264_plane_copy_mmxext;
    pf->hpel_filter = x264_hpel_filter_mmxext;
    pf->frame_init_lowres_core = x264_frame_init_lowres_core_mmxext;

    pf->prefetch_fenc = x264_prefetch_fenc_mmxext;
    pf->prefetch_ref  = x264_prefetch_ref_mmxext;

#ifdef ARCH_X86 // all x86_64 cpus with cacheline split issues use sse2 instead
    if( cpu&X264_CPU_CACHELINE_32 )
    {
        pf->mc_luma = mc_luma_cache32_mmxext;
        pf->get_ref = get_ref_cache32_mmxext;
        pf->frame_init_lowres_core = x264_frame_init_lowres_core_cache32_mmxext;
    }
    else if( cpu&X264_CPU_CACHELINE_64 )
    {
        pf->mc_luma = mc_luma_cache64_mmxext;
        pf->get_ref = get_ref_cache64_mmxext;
        pf->frame_init_lowres_core = x264_frame_init_lowres_core_cache32_mmxext;
    }
#endif

    if( !(cpu&X264_CPU_SSE2) )
        return;

    pf->memcpy_aligned = x264_memcpy_aligned_sse2;
    pf->memzero_aligned = x264_memzero_aligned_sse2;
    pf->integral_init4v = x264_integral_init4v_sse2;
    pf->integral_init8v = x264_integral_init8v_sse2;
    pf->hpel_filter = x264_hpel_filter_sse2_amd;

    if( cpu&X264_CPU_SSE2_IS_SLOW )
        return;

    pf->copy[PIXEL_16x16] = x264_mc_copy_w16_aligned_sse2;
    pf->avg[PIXEL_16x16] = x264_pixel_avg_16x16_sse2;
    pf->avg[PIXEL_16x8]  = x264_pixel_avg_16x8_sse2;
    pf->avg[PIXEL_8x16] = x264_pixel_avg_8x16_sse2;
    pf->avg[PIXEL_8x8]  = x264_pixel_avg_8x8_sse2;
    pf->avg[PIXEL_8x4]  = x264_pixel_avg_8x4_sse2;
    pf->hpel_filter = x264_hpel_filter_sse2;
    if( cpu&X264_CPU_SSE_MISALIGN )
        pf->hpel_filter = x264_hpel_filter_sse2_misalign;
    pf->frame_init_lowres_core = x264_frame_init_lowres_core_sse2;
    pf->mc_chroma = x264_mc_chroma_sse2;

    if( cpu&X264_CPU_SSE2_IS_FAST )
    {
        pf->mc_luma = mc_luma_sse2;
        pf->get_ref = get_ref_sse2;
        if( cpu&X264_CPU_CACHELINE_64 )
        {
            pf->mc_luma = mc_luma_cache64_sse2;
            pf->get_ref = get_ref_cache64_sse2;
        }
        if( cpu&X264_CPU_SSE_MISALIGN )
            pf->get_ref = get_ref_sse2_misalign;
    }

    if( !(cpu&X264_CPU_SSSE3) )
        return;

    pf->avg[PIXEL_16x16] = x264_pixel_avg_16x16_ssse3;
    pf->avg[PIXEL_16x8]  = x264_pixel_avg_16x8_ssse3;
    pf->avg[PIXEL_8x16]  = x264_pixel_avg_8x16_ssse3;
    pf->avg[PIXEL_8x8]   = x264_pixel_avg_8x8_ssse3;
    pf->avg[PIXEL_8x4]   = x264_pixel_avg_8x4_ssse3;
    pf->avg[PIXEL_4x8]   = x264_pixel_avg_4x8_ssse3;
    pf->avg[PIXEL_4x4]   = x264_pixel_avg_4x4_ssse3;
    pf->avg[PIXEL_4x2]   = x264_pixel_avg_4x2_ssse3;

    pf->hpel_filter = x264_hpel_filter_ssse3;
    pf->frame_init_lowres_core = x264_frame_init_lowres_core_ssse3;
    pf->mc_chroma = x264_mc_chroma_ssse3;

    if( !(cpu&X264_CPU_SSE4) )
        return;

    pf->integral_init4h = x264_integral_init4h_sse4;
    pf->integral_init8h = x264_integral_init8h_sse4;
}

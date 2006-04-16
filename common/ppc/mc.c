/*****************************************************************************
 * mc.c: h264 encoder library (Motion Compensation)
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar
 * $Id: mc.c,v 1.1 2004/06/03 19:27:07 fenrir Exp $
 *
 * Authors: Eric Petit <titer@m0k.org>
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
#include <stdint.h>
#include <stdarg.h>

#ifdef SYS_LINUX
#include <altivec.h>
#endif

#include "x264.h"
#include "common/mc.h"
#include "common/clip1.h"
#include "mc.h"
#include "ppccommon.h"

typedef void (*pf_mc_t)( uint8_t *src, int i_src,
                         uint8_t *dst, int i_dst, int i_height );

static inline int x264_tapfilter( uint8_t *pix, int i_pix_next )
{
    return pix[-2*i_pix_next] - 5*pix[-1*i_pix_next] + 20*(pix[0] +
           pix[1*i_pix_next]) - 5*pix[ 2*i_pix_next] +
           pix[ 3*i_pix_next];
}
static inline int x264_tapfilter1( uint8_t *pix )
{
    return pix[-2] - 5*pix[-1] + 20*(pix[0] + pix[1]) - 5*pix[ 2] +
           pix[ 3];
}

/* pixel_avg */
static inline void pixel_avg_w4( uint8_t *dst,  int i_dst,
                                 uint8_t *src1, int i_src1,
                                 uint8_t *src2, int i_src2,
                                 int i_height )
{
    int x, y;
    for( y = 0; y < i_height; y++ )
    {
        for( x = 0; x < 4; x++ )
        {
            dst[x] = ( src1[x] + src2[x] + 1 ) >> 1;
        }
        dst  += i_dst;
        src1 += i_src1;
        src2 += i_src2;
    }
}
static inline void pixel_avg_w8( uint8_t *dst,  int i_dst,
                                 uint8_t *src1, int i_src1,
                                 uint8_t *src2, int i_src2,
                                 int i_height )
{
    int y;
    vec_u8_t src1v, src2v;
    LOAD_ZERO;
    PREP_LOAD;
    PREP_STORE8;
    for( y = 0; y < i_height; y++ )
    {
        VEC_LOAD( src1, src1v, 8, vec_u8_t );
        VEC_LOAD( src2, src2v, 8, vec_u8_t );
        src1v = vec_avg( src1v, src2v );
        VEC_STORE8( src1v, dst );

        dst  += i_dst;
        src1 += i_src1;
        src2 += i_src2;
    }
}
static inline void pixel_avg_w16( uint8_t *dst,  int i_dst,
                                  uint8_t *src1, int i_src1,
                                  uint8_t *src2, int i_src2,
                                  int i_height )
{
    int y;
    vec_u8_t src1v, src2v;
    PREP_LOAD;
    PREP_STORE16;
    for( y = 0; y < i_height; y++ )
    {
        VEC_LOAD( src1, src1v, 16, vec_u8_t );
        VEC_LOAD( src2, src2v, 16, vec_u8_t );
        src1v = vec_avg( src1v, src2v );
        VEC_STORE16( src1v, dst );

        dst  += i_dst;
        src1 += i_src1;
        src2 += i_src2;
    }
}

/* mc_copy: plain c */
#define MC_COPY( name, a )                                \
static void name( uint8_t *src, int i_src,                \
                  uint8_t *dst, int i_dst, int i_height ) \
{                                                         \
    int y;                                                \
    for( y = 0; y < i_height; y++ )                       \
    {                                                     \
        memcpy( dst, src, a );                            \
        src += i_src;                                     \
        dst += i_dst;                                     \
    }                                                     \
}
MC_COPY( mc_copy_w4,  4  )
MC_COPY( mc_copy_w8,  8  )
MC_COPY( mc_copy_w16, 16 )

void mc_luma_altivec( uint8_t *src[4], int i_src_stride,
                      uint8_t *dst,    int i_dst_stride,
                      int mvx, int mvy,
                      int i_width, int i_height )
{
    uint8_t *src1, *src2;
    
    /* todo : fixme... */
    int correction = (((mvx&3) == 3 && (mvy&3) == 1) || ((mvx&3) == 1 && (mvy&3) == 3)) ? 1:0;
    
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
            pixel_avg_w4( dst, i_dst_stride, src1, i_src_stride,
                          src2, i_src_stride, i_height );
            break;
        case 8:
            pixel_avg_w8( dst, i_dst_stride, src1, i_src_stride,
                          src2, i_src_stride, i_height );
            break;
        case 16:
        default:
            pixel_avg_w16( dst, i_dst_stride, src1, i_src_stride,
                           src2, i_src_stride, i_height );
        }
        
    }
    else
    {
        switch(i_width) {
        case 4:
            mc_copy_w4( src1, i_src_stride, dst, i_dst_stride, i_height );
            break;
        case 8:
            mc_copy_w8( src1, i_src_stride, dst, i_dst_stride, i_height );
            break;
        case 16:
            mc_copy_w16( src1, i_src_stride, dst, i_dst_stride, i_height );
            break;
        }
        
    }
}

uint8_t *get_ref_altivec( uint8_t *src[4], int i_src_stride,
                          uint8_t *dst,    int * i_dst_stride,
                          int mvx, int mvy,
                          int i_width, int i_height )
{
    uint8_t *src1, *src2;
    
    /* todo : fixme... */
    int correction = (((mvx&3) == 3 && (mvy&3) == 1) || ((mvx&3) == 1 && (mvy&3) == 3)) ? 1:0;
    
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
            pixel_avg_w4( dst, *i_dst_stride, src1, i_src_stride,
                          src2, i_src_stride, i_height );
            break;
        case 8:
            pixel_avg_w8( dst, *i_dst_stride, src1, i_src_stride,
                          src2, i_src_stride, i_height );
            break;
        case 16:
        default:
            pixel_avg_w16( dst, *i_dst_stride, src1, i_src_stride,
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

static void mc_chroma_c( uint8_t *src, int i_src_stride,
                         uint8_t *dst, int i_dst_stride,
                         int mvx, int mvy,
                         int i_width, int i_height )
{
    uint8_t *srcp;
    int x, y;
    int d8x = mvx & 0x07;
    int d8y = mvy & 0x07;

    DECLARE_ALIGNED( uint16_t, coeff[4], 16 );
    coeff[0] = (8-d8x)*(8-d8y);
    coeff[1] = d8x    *(8-d8y);
    coeff[2] = (8-d8x)*d8y;
    coeff[3] = d8x    *d8y;

    src  += (mvy >> 3) * i_src_stride + (mvx >> 3);
    srcp  = &src[i_src_stride];

    /* TODO: optimize */
    for( y = 0; y < i_height; y++ )
    {
        for( x = 0; x < i_width; x++ )
        {
            dst[x] = ( coeff[0]*src[x]  + coeff[1]*src[x+1] +
                       coeff[2]*srcp[x] + coeff[3]*srcp[x+1] + 32 ) >> 6;
        }
        dst  += i_dst_stride;

        src   = srcp;
        srcp += i_src_stride;
    }
}

#define DO_PROCESS(a) \
        src##a##v_16 = vec_u8_to_u16( src##a##v_8 ); \
        src##a##v_16 = vec_mladd( coeff##a##v, src##a##v_16, zero_u16v ); \
        dstv_16      = vec_add( dstv_16, src##a##v_16 )

static void mc_chroma_altivec_4xh( uint8_t *src, int i_src_stride,
                                   uint8_t *dst, int i_dst_stride,
                                   int mvx, int mvy,
                                   int i_height )
{
    uint8_t *srcp;
    int y;
    int d8x = mvx & 0x07;
    int d8y = mvy & 0x07;

    DECLARE_ALIGNED( uint16_t, coeff[4], 16 );
    coeff[0] = (8-d8x)*(8-d8y);
    coeff[1] = d8x    *(8-d8y);
    coeff[2] = (8-d8x)*d8y;
    coeff[3] = d8x    *d8y;

    src  += (mvy >> 3) * i_src_stride + (mvx >> 3);
    srcp  = &src[i_src_stride];
    
    LOAD_ZERO;
    PREP_LOAD;
    PREP_STORE4;
    vec_u16_t   coeff0v, coeff1v, coeff2v, coeff3v;
    vec_u8_t    src0v_8, src1v_8, src2v_8, src3v_8;
    vec_u16_t   src0v_16, src1v_16, src2v_16, src3v_16;
    vec_u8_t    dstv_8;
    vec_u16_t   dstv_16;
    vec_u8_t    permv;
    vec_u16_t   shiftv;
    vec_u16_t   k32v;
    
    coeff0v = vec_ld( 0, coeff );
    coeff3v = vec_splat( coeff0v, 3 );
    coeff2v = vec_splat( coeff0v, 2 );
    coeff1v = vec_splat( coeff0v, 1 );
    coeff0v = vec_splat( coeff0v, 0 );
    k32v    = vec_sl( vec_splat_u16( 1 ), vec_splat_u16( 5 ) );
    permv   = vec_lvsl( 0, (uint8_t *) 1 );
    shiftv  = vec_splat_u16( 6 );

    VEC_LOAD( src, src2v_8, 5, vec_u8_t );
    src3v_8 = vec_perm( src2v_8, src2v_8, permv );

    for( y = 0; y < i_height; y++ )
    {
        src0v_8 = src2v_8;
        src1v_8 = src3v_8;
        VEC_LOAD( srcp, src2v_8, 5, vec_u8_t );
        src3v_8 = vec_perm( src2v_8, src2v_8, permv );

        dstv_16 = k32v;

        DO_PROCESS( 0 );
        DO_PROCESS( 1 );
        DO_PROCESS( 2 );
        DO_PROCESS( 3 );

        dstv_16 = vec_sr( dstv_16, shiftv );
        dstv_8  = vec_u16_to_u8( dstv_16 );
        VEC_STORE4( dstv_8, dst );

        dst  += i_dst_stride;
        srcp += i_src_stride;
    }
}

static void mc_chroma_altivec_8xh( uint8_t *src, int i_src_stride,
                                   uint8_t *dst, int i_dst_stride,
                                   int mvx, int mvy,
                                   int i_height )
{
    uint8_t *srcp;
    int y;
    int d8x = mvx & 0x07;
    int d8y = mvy & 0x07;

    DECLARE_ALIGNED( uint16_t, coeff[4], 16 );
    coeff[0] = (8-d8x)*(8-d8y);
    coeff[1] = d8x    *(8-d8y);
    coeff[2] = (8-d8x)*d8y;
    coeff[3] = d8x    *d8y;

    src  += (mvy >> 3) * i_src_stride + (mvx >> 3);
    srcp  = &src[i_src_stride];
    
    LOAD_ZERO;
    PREP_LOAD;
    PREP_STORE8;
    vec_u16_t   coeff0v, coeff1v, coeff2v, coeff3v;
    vec_u8_t    src0v_8, src1v_8, src2v_8, src3v_8;
    vec_u16_t   src0v_16, src1v_16, src2v_16, src3v_16;
    vec_u8_t    dstv_8;
    vec_u16_t   dstv_16;
    vec_u8_t    permv;
    vec_u16_t   shiftv;
    vec_u16_t   k32v;
    
    coeff0v = vec_ld( 0, coeff );
    coeff3v = vec_splat( coeff0v, 3 );
    coeff2v = vec_splat( coeff0v, 2 );
    coeff1v = vec_splat( coeff0v, 1 );
    coeff0v = vec_splat( coeff0v, 0 );
    k32v    = vec_sl( vec_splat_u16( 1 ), vec_splat_u16( 5 ) );
    permv   = vec_lvsl( 0, (uint8_t *) 1 );
    shiftv  = vec_splat_u16( 6 );

    VEC_LOAD( src, src2v_8, 9, vec_u8_t );
    src3v_8 = vec_perm( src2v_8, src2v_8, permv );

    for( y = 0; y < i_height; y++ )
    {
        src0v_8 = src2v_8;
        src1v_8 = src3v_8;
        VEC_LOAD( srcp, src2v_8, 9, vec_u8_t );
        src3v_8 = vec_perm( src2v_8, src2v_8, permv );

        dstv_16 = k32v;

        DO_PROCESS( 0 );
        DO_PROCESS( 1 );
        DO_PROCESS( 2 );
        DO_PROCESS( 3 );

        dstv_16 = vec_sr( dstv_16, shiftv );
        dstv_8  = vec_u16_to_u8( dstv_16 );
        VEC_STORE8( dstv_8, dst );

        dst  += i_dst_stride;
        srcp += i_src_stride;
    }
}

static void mc_chroma_altivec( uint8_t *src, int i_src_stride,
                               uint8_t *dst, int i_dst_stride,
                               int mvx, int mvy,
                               int i_width, int i_height )
{
    if( i_width == 8 )
    {
        mc_chroma_altivec_8xh( src, i_src_stride, dst, i_dst_stride,
                               mvx, mvy, i_height );
        return;
    }
    if( i_width == 4 )
    {
        mc_chroma_altivec_4xh( src, i_src_stride, dst, i_dst_stride,
                               mvx, mvy, i_height );
        return;
    }

    mc_chroma_c( src, i_src_stride, dst, i_dst_stride,
                 mvx, mvy, i_width, i_height );
}

void x264_mc_altivec_init( x264_mc_functions_t *pf )
{
    pf->mc_luma   = mc_luma_altivec;
    pf->get_ref   = get_ref_altivec;
    pf->mc_chroma = mc_chroma_altivec;
}

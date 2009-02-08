/*****************************************************************************
 * mc.c: h264 encoder library (Motion Compensation)
 *****************************************************************************
 * Copyright (C) 2003-2008 x264 project
 *
 * Authors: Eric Petit <eric.petit@lapsus.org>
 *          Guillaume Poirier <gpoirier@mplayerhq.hu>
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
#include <stdint.h>
#include <stdarg.h>

#include "x264.h"
#include "common/common.h"
#include "common/mc.h"
#include "mc.h"
#include "ppccommon.h"

typedef void (*pf_mc_t)( uint8_t *src, int i_src,
                         uint8_t *dst, int i_dst, int i_height );


static const int hpel_ref0[16] = {0,1,1,1,0,1,1,1,2,3,3,3,0,1,1,1};
static const int hpel_ref1[16] = {0,0,0,0,2,2,3,2,2,2,3,2,2,2,3,2};


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


static inline void x264_pixel_avg2_w4_altivec( uint8_t *dst,  int i_dst,
                                               uint8_t *src1, int i_src1,
                                               uint8_t *src2, int i_height )
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
        src2 += i_src1;
    }
}

static inline void x264_pixel_avg2_w8_altivec( uint8_t *dst,  int i_dst,
                                               uint8_t *src1, int i_src1,
                                               uint8_t *src2, int i_height )
{
    int y;
    vec_u8_t src1v, src2v;
    PREP_LOAD;
    PREP_STORE8;
    PREP_LOAD_SRC( src1 );
    PREP_LOAD_SRC( src2 );

    for( y = 0; y < i_height; y++ )
    {
        VEC_LOAD( src1, src1v, 8, vec_u8_t, src1 );
        VEC_LOAD( src2, src2v, 8, vec_u8_t, src2 );
        src1v = vec_avg( src1v, src2v );
        VEC_STORE8( src1v, dst );

        dst  += i_dst;
        src1 += i_src1;
        src2 += i_src1;
    }
}

static inline void x264_pixel_avg2_w16_altivec( uint8_t *dst,  int i_dst,
                                                uint8_t *src1, int i_src1,
                                                uint8_t *src2, int i_height )
{
    int y;
    vec_u8_t src1v, src2v;
    PREP_LOAD;
    PREP_LOAD_SRC( src1 );
    PREP_LOAD_SRC( src2 );

    for( y = 0; y < i_height; y++ )
    {
        VEC_LOAD( src1, src1v, 16, vec_u8_t, src1 );
        VEC_LOAD( src2, src2v, 16, vec_u8_t, src2 );
        src1v = vec_avg( src1v, src2v );
        vec_st(src1v, 0, dst);

        dst  += i_dst;
        src1 += i_src1;
        src2 += i_src1;
    }
}

static inline void x264_pixel_avg2_w20_altivec( uint8_t *dst,  int i_dst,
                                                uint8_t *src1, int i_src1,
                                                uint8_t *src2, int i_height )
{
    x264_pixel_avg2_w16_altivec(dst, i_dst, src1, i_src1, src2, i_height);
    x264_pixel_avg2_w4_altivec(dst+16, i_dst, src1+16, i_src1, src2+16, i_height);
}

/* mc_copy: plain c */

#define MC_COPY( name, a )                                \
static void name( uint8_t *dst, int i_dst,                \
                  uint8_t *src, int i_src, int i_height ) \
{                                                         \
    int y;                                                \
    for( y = 0; y < i_height; y++ )                       \
    {                                                     \
        memcpy( dst, src, a );                            \
        src += i_src;                                     \
        dst += i_dst;                                     \
    }                                                     \
}
MC_COPY( x264_mc_copy_w4_altivec,  4  )
MC_COPY( x264_mc_copy_w8_altivec,  8  )

static void x264_mc_copy_w16_altivec( uint8_t *dst, int i_dst,
                                      uint8_t *src, int i_src, int i_height )
{
    int y;
    vec_u8_t cpyV;
    PREP_LOAD;
    PREP_LOAD_SRC( src );

    for( y = 0; y < i_height; y++)
    {
        VEC_LOAD( src, cpyV, 16, vec_u8_t, src );
        vec_st(cpyV, 0, dst);

        src += i_src;
        dst += i_dst;
    }
}


static void x264_mc_copy_w16_aligned_altivec( uint8_t *dst, int i_dst,
                                              uint8_t *src, int i_src, int i_height )
{
    int y;

    for( y = 0; y < i_height; ++y)
    {
        vec_u8_t cpyV = vec_ld( 0, src);
        vec_st(cpyV, 0, dst);

        src += i_src;
        dst += i_dst;
    }
}


static void mc_luma_altivec( uint8_t *dst,    int i_dst_stride,
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

        switch(i_width) {
        case 4:
            x264_pixel_avg2_w4_altivec( dst, i_dst_stride, src1, i_src_stride, src2, i_height );
            break;
        case 8:
            x264_pixel_avg2_w8_altivec( dst, i_dst_stride, src1, i_src_stride, src2, i_height );
            break;
        case 16:
        default:
            x264_pixel_avg2_w16_altivec( dst, i_dst_stride, src1, i_src_stride, src2, i_height );
        }

    }
    else
    {
        switch(i_width) {
        case 4:
            x264_mc_copy_w4_altivec( dst, i_dst_stride, src1, i_src_stride, i_height );
            break;
        case 8:
            x264_mc_copy_w8_altivec( dst, i_dst_stride, src1, i_src_stride, i_height );
            break;
        case 16:
            x264_mc_copy_w16_altivec( dst, i_dst_stride, src1, i_src_stride, i_height );
            break;
        }
    }
}



static uint8_t *get_ref_altivec( uint8_t *dst,   int *i_dst_stride,
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
        switch(i_width) {
        case 4:
            x264_pixel_avg2_w4_altivec( dst, *i_dst_stride, src1, i_src_stride, src2, i_height );
            break;
        case 8:
            x264_pixel_avg2_w8_altivec( dst, *i_dst_stride, src1, i_src_stride, src2, i_height );
            break;
        case 12:
        case 16:
        default:
            x264_pixel_avg2_w16_altivec( dst, *i_dst_stride, src1, i_src_stride, src2, i_height );
            break;
        case 20:
            x264_pixel_avg2_w20_altivec( dst, *i_dst_stride, src1, i_src_stride, src2, i_height );
            break;
        }
        return dst;
    }
    else
    {
        *i_dst_stride = i_src_stride;
        return src1;
    }
}

static void mc_chroma_2xh( uint8_t *dst, int i_dst_stride,
                           uint8_t *src, int i_src_stride,
                           int mvx, int mvy,
                           int i_height )
{
    uint8_t *srcp;
    int y;
    int d8x = mvx&0x07;
    int d8y = mvy&0x07;

    const int cA = (8-d8x)*(8-d8y);
    const int cB = d8x    *(8-d8y);
    const int cC = (8-d8x)*d8y;
    const int cD = d8x    *d8y;

    src  += (mvy >> 3) * i_src_stride + (mvx >> 3);
    srcp  = &src[i_src_stride];

    for( y = 0; y < i_height; y++ )
    {
        dst[0] = ( cA*src[0] +  cB*src[0+1] +
                  cC*srcp[0] + cD*srcp[0+1] + 32 ) >> 6;
        dst[1] = ( cA*src[1] +  cB*src[1+1] +
                  cC*srcp[1] + cD*srcp[1+1] + 32 ) >> 6;

        src  += i_src_stride;
        srcp += i_src_stride;
        dst  += i_dst_stride;
    }
 }


#define DO_PROCESS_W4( a )  \
    dstv_16A = vec_mladd( src##a##v_16A, coeff##a##v, dstv_16A );   \
    dstv_16B = vec_mladd( src##a##v_16B, coeff##a##v, dstv_16B )

static void mc_chroma_altivec_4xh( uint8_t *dst, int i_dst_stride,
                                   uint8_t *src, int i_src_stride,
                                   int mvx, int mvy,
                                   int i_height )
{
    uint8_t *srcp;
    int y;
    int d8x = mvx & 0x07;
    int d8y = mvy & 0x07;

    DECLARE_ALIGNED_16( uint16_t coeff[4] );
    coeff[0] = (8-d8x)*(8-d8y);
    coeff[1] = d8x    *(8-d8y);
    coeff[2] = (8-d8x)*d8y;
    coeff[3] = d8x    *d8y;

    src  += (mvy >> 3) * i_src_stride + (mvx >> 3);
    srcp  = &src[i_src_stride];

    LOAD_ZERO;
    PREP_LOAD;
    PREP_LOAD_SRC( src );
    vec_u16_t   coeff0v, coeff1v, coeff2v, coeff3v;
    vec_u8_t    src2v_8A, dstv_8A;
    vec_u8_t    src2v_8B, dstv_8B;
    vec_u16_t   src0v_16A, src1v_16A, src2v_16A, src3v_16A, dstv_16A;
    vec_u16_t   src0v_16B, src1v_16B, src2v_16B, src3v_16B, dstv_16B;
    vec_u16_t   shiftv, k32v;

    coeff0v = vec_ld( 0, coeff );
    coeff3v = vec_splat( coeff0v, 3 );
    coeff2v = vec_splat( coeff0v, 2 );
    coeff1v = vec_splat( coeff0v, 1 );
    coeff0v = vec_splat( coeff0v, 0 );
    k32v    = vec_sl( vec_splat_u16( 1 ), vec_splat_u16( 5 ) );
    shiftv  = vec_splat_u16( 6 );

    VEC_LOAD( src, src2v_8B, 5, vec_u8_t, src );
    src2v_16B = vec_u8_to_u16( src2v_8B );
    src3v_16B = vec_sld( src2v_16B, src2v_16B, 2 );

    for( y = 0; y < i_height; y+=2 )
    {
        src0v_16A = src2v_16B;
        src1v_16A = src3v_16B;

        VEC_LOAD_G( srcp, src2v_8A, 5, vec_u8_t );
        srcp += i_src_stride;
        VEC_LOAD_G( srcp, src2v_8B, 5, vec_u8_t );
        srcp += i_src_stride;
        src2v_16A = vec_u8_to_u16( src2v_8A );
        src2v_16B = vec_u8_to_u16( src2v_8B );
        src3v_16A = vec_sld( src2v_16A, src2v_16A, 2 );
        src3v_16B = vec_sld( src2v_16B, src2v_16B, 2 );

        src0v_16B = src2v_16A;
        src1v_16B = src3v_16A;

        dstv_16A = dstv_16B = k32v;
        DO_PROCESS_W4( 0 );
        DO_PROCESS_W4( 1 );
        DO_PROCESS_W4( 2 );
        DO_PROCESS_W4( 3 );

        dstv_16A = vec_sr( dstv_16A, shiftv );
        dstv_16B = vec_sr( dstv_16B, shiftv );
        dstv_8A  = vec_u16_to_u8( dstv_16A );
        dstv_8B  = vec_u16_to_u8( dstv_16B );
        vec_ste( vec_splat( (vec_u32_t) dstv_8A, 0 ), 0, (uint32_t*) dst );
        dst += i_dst_stride;
        vec_ste( vec_splat( (vec_u32_t) dstv_8B, 0 ), 0, (uint32_t*) dst );
        dst += i_dst_stride;
    }
}

#define DO_PROCESS_W8( a )  \
    src##a##v_16A = vec_u8_to_u16( src##a##v_8A );  \
    src##a##v_16B = vec_u8_to_u16( src##a##v_8B );  \
    dstv_16A = vec_mladd( src##a##v_16A, coeff##a##v, dstv_16A );   \
    dstv_16B = vec_mladd( src##a##v_16B, coeff##a##v, dstv_16B )

static void mc_chroma_altivec_8xh( uint8_t *dst, int i_dst_stride,
                                   uint8_t *src, int i_src_stride,
                                   int mvx, int mvy,
                                   int i_height )
{
    uint8_t *srcp;
    int y;
    int d8x = mvx & 0x07;
    int d8y = mvy & 0x07;

    DECLARE_ALIGNED_16( uint16_t coeff[4] );
    coeff[0] = (8-d8x)*(8-d8y);
    coeff[1] = d8x    *(8-d8y);
    coeff[2] = (8-d8x)*d8y;
    coeff[3] = d8x    *d8y;

    src  += (mvy >> 3) * i_src_stride + (mvx >> 3);
    srcp  = &src[i_src_stride];

    LOAD_ZERO;
    PREP_LOAD;
    PREP_LOAD_SRC( src );
    PREP_STORE8;
    vec_u16_t   coeff0v, coeff1v, coeff2v, coeff3v;
    vec_u8_t    src0v_8A, src1v_8A, src2v_8A, src3v_8A, dstv_8A;
    vec_u8_t    src0v_8B, src1v_8B, src2v_8B, src3v_8B, dstv_8B;
    vec_u16_t   src0v_16A, src1v_16A, src2v_16A, src3v_16A, dstv_16A;
    vec_u16_t   src0v_16B, src1v_16B, src2v_16B, src3v_16B, dstv_16B;
    vec_u16_t   shiftv, k32v;

    coeff0v = vec_ld( 0, coeff );
    coeff3v = vec_splat( coeff0v, 3 );
    coeff2v = vec_splat( coeff0v, 2 );
    coeff1v = vec_splat( coeff0v, 1 );
    coeff0v = vec_splat( coeff0v, 0 );
    k32v    = vec_sl( vec_splat_u16( 1 ), vec_splat_u16( 5 ) );
    shiftv  = vec_splat_u16( 6 );

    VEC_LOAD( src, src2v_8B, 9, vec_u8_t, src );
    src3v_8B = vec_sld( src2v_8B, src2v_8B, 1 );

    for( y = 0; y < i_height; y+=2 )
    {
        src0v_8A = src2v_8B;
        src1v_8A = src3v_8B;

        VEC_LOAD_G( srcp, src2v_8A, 9, vec_u8_t );
        srcp += i_src_stride;
        VEC_LOAD_G( srcp, src2v_8B, 9, vec_u8_t );
        srcp += i_src_stride;
        src3v_8A = vec_sld( src2v_8A, src2v_8A, 1 );
        src3v_8B = vec_sld( src2v_8B, src2v_8B, 1 );

        src0v_8B = src2v_8A;
        src1v_8B = src3v_8A;
        dstv_16A = dstv_16B = k32v;
        DO_PROCESS_W8( 0 );
        DO_PROCESS_W8( 1 );
        DO_PROCESS_W8( 2 );
        DO_PROCESS_W8( 3 );

        dstv_16A = vec_sr( dstv_16A, shiftv );
        dstv_16B = vec_sr( dstv_16B, shiftv );
        dstv_8A  = vec_u16_to_u8( dstv_16A );
        dstv_8B  = vec_u16_to_u8( dstv_16B );
        VEC_STORE8( dstv_8A, dst );
        dst += i_dst_stride;
        VEC_STORE8( dstv_8B, dst );
        dst += i_dst_stride;
    }
}

static void mc_chroma_altivec( uint8_t *dst, int i_dst_stride,
                               uint8_t *src, int i_src_stride,
                               int mvx, int mvy,
                               int i_width, int i_height )
{
    if( i_width == 8 )
    {
        mc_chroma_altivec_8xh( dst, i_dst_stride, src, i_src_stride,
                               mvx, mvy, i_height );
    }
    else if( i_width == 4 )
    {
        mc_chroma_altivec_4xh( dst, i_dst_stride, src, i_src_stride,
                               mvx, mvy, i_height );
    }
    else
    {
        mc_chroma_2xh( dst, i_dst_stride, src, i_src_stride,
                       mvx, mvy, i_height );
    }
}

#define HPEL_FILTER_1( t1v, t2v, t3v, t4v, t5v, t6v ) \
{                                                     \
    t1v = vec_add( t1v, t6v );                        \
    t2v = vec_add( t2v, t5v );                        \
    t3v = vec_add( t3v, t4v );                        \
                                                      \
    t1v = vec_sub( t1v, t2v );   /* (a-b) */          \
    t2v = vec_sub( t2v, t3v );   /* (b-c) */          \
    t2v = vec_sl(  t2v, twov );  /* (b-c)*4 */        \
    t1v = vec_sub( t1v, t2v );   /* a-5*b+4*c */      \
    t3v = vec_sl(  t3v, fourv ); /* 16*c */           \
    t1v = vec_add( t1v, t3v );   /* a-5*b+20*c */     \
}

#define HPEL_FILTER_2( t1v, t2v, t3v, t4v, t5v, t6v ) \
{                                                     \
    t1v = vec_add( t1v, t6v );                        \
    t2v = vec_add( t2v, t5v );                        \
    t3v = vec_add( t3v, t4v );                        \
                                                      \
    t1v = vec_sub( t1v, t2v );  /* (a-b) */           \
    t1v = vec_sra( t1v, twov ); /* (a-b)/4 */         \
    t1v = vec_sub( t1v, t2v );  /* (a-b)/4-b */       \
    t1v = vec_add( t1v, t3v );  /* (a-b)/4-b+c */     \
    t1v = vec_sra( t1v, twov ); /* ((a-b)/4-b+c)/4 */ \
    t1v = vec_add( t1v, t3v );  /* ((a-b)/4-b+c)/4+c = (a-5*b+20*c)/16 */ \
}

#define HPEL_FILTER_HORIZONTAL()                             \
{                                                            \
    VEC_LOAD_G( &src[x- 2+i_stride*y], src1v, 16, vec_u8_t); \
    VEC_LOAD_G( &src[x+14+i_stride*y], src6v, 16, vec_u8_t); \
                                                             \
    src2v = vec_sld( src1v, src6v,  1 );                     \
    src3v = vec_sld( src1v, src6v,  2 );                     \
    src4v = vec_sld( src1v, src6v,  3 );                     \
    src5v = vec_sld( src1v, src6v,  4 );                     \
    src6v = vec_sld( src1v, src6v,  5 );                     \
                                                             \
    temp1v = vec_u8_to_s16_h( src1v );                       \
    temp2v = vec_u8_to_s16_h( src2v );                       \
    temp3v = vec_u8_to_s16_h( src3v );                       \
    temp4v = vec_u8_to_s16_h( src4v );                       \
    temp5v = vec_u8_to_s16_h( src5v );                       \
    temp6v = vec_u8_to_s16_h( src6v );                       \
                                                             \
    HPEL_FILTER_1( temp1v, temp2v, temp3v,                   \
                   temp4v, temp5v, temp6v );                 \
                                                             \
    dest1v = vec_add( temp1v, sixteenv );                    \
    dest1v = vec_sra( dest1v, fivev );                       \
                                                             \
    temp1v = vec_u8_to_s16_l( src1v );                       \
    temp2v = vec_u8_to_s16_l( src2v );                       \
    temp3v = vec_u8_to_s16_l( src3v );                       \
    temp4v = vec_u8_to_s16_l( src4v );                       \
    temp5v = vec_u8_to_s16_l( src5v );                       \
    temp6v = vec_u8_to_s16_l( src6v );                       \
                                                             \
    HPEL_FILTER_1( temp1v, temp2v, temp3v,                   \
                   temp4v, temp5v, temp6v );                 \
                                                             \
    dest2v = vec_add( temp1v, sixteenv );                    \
    dest2v = vec_sra( dest2v, fivev );                       \
                                                             \
    destv = vec_packsu( dest1v, dest2v );                    \
                                                             \
    VEC_STORE16( destv, &dsth[x+i_stride*y], dsth );         \
}

#define HPEL_FILTER_VERTICAL()                                    \
{                                                                 \
    VEC_LOAD( &src[x+i_stride*(y-2)], src1v, 16, vec_u8_t, src ); \
    VEC_LOAD( &src[x+i_stride*(y-1)], src2v, 16, vec_u8_t, src ); \
    VEC_LOAD( &src[x+i_stride*(y-0)], src3v, 16, vec_u8_t, src ); \
    VEC_LOAD( &src[x+i_stride*(y+1)], src4v, 16, vec_u8_t, src ); \
    VEC_LOAD( &src[x+i_stride*(y+2)], src5v, 16, vec_u8_t, src ); \
    VEC_LOAD( &src[x+i_stride*(y+3)], src6v, 16, vec_u8_t, src ); \
                                                                  \
    temp1v = vec_u8_to_s16_h( src1v );                            \
    temp2v = vec_u8_to_s16_h( src2v );                            \
    temp3v = vec_u8_to_s16_h( src3v );                            \
    temp4v = vec_u8_to_s16_h( src4v );                            \
    temp5v = vec_u8_to_s16_h( src5v );                            \
    temp6v = vec_u8_to_s16_h( src6v );                            \
                                                                  \
    HPEL_FILTER_1( temp1v, temp2v, temp3v,                        \
                   temp4v, temp5v, temp6v );                      \
                                                                  \
    dest1v = vec_add( temp1v, sixteenv );                         \
    dest1v = vec_sra( dest1v, fivev );                            \
                                                                  \
    temp4v = vec_u8_to_s16_l( src1v );                            \
    temp5v = vec_u8_to_s16_l( src2v );                            \
    temp6v = vec_u8_to_s16_l( src3v );                            \
    temp7v = vec_u8_to_s16_l( src4v );                            \
    temp8v = vec_u8_to_s16_l( src5v );                            \
    temp9v = vec_u8_to_s16_l( src6v );                            \
                                                                  \
    HPEL_FILTER_1( temp4v, temp5v, temp6v,                        \
                   temp7v, temp8v, temp9v );                      \
                                                                  \
    dest2v = vec_add( temp4v, sixteenv );                         \
    dest2v = vec_sra( dest2v, fivev );                            \
                                                                  \
    destv = vec_packsu( dest1v, dest2v );                         \
                                                                  \
    VEC_STORE16( destv, &dstv[x+i_stride*y], dsth );              \
}

#define HPEL_FILTER_CENTRAL()                           \
{                                                       \
    temp1v = vec_sld( tempav, tempbv, 12 );             \
    temp2v = vec_sld( tempav, tempbv, 14 );             \
    temp3v = tempbv;                                    \
    temp4v = vec_sld( tempbv, tempcv,  2 );             \
    temp5v = vec_sld( tempbv, tempcv,  4 );             \
    temp6v = vec_sld( tempbv, tempcv,  6 );             \
                                                        \
    HPEL_FILTER_2( temp1v, temp2v, temp3v,              \
                   temp4v, temp5v, temp6v );            \
                                                        \
    dest1v = vec_add( temp1v, thirtytwov );             \
    dest1v = vec_sra( dest1v, sixv );                   \
                                                        \
    temp1v = vec_sld( tempbv, tempcv, 12 );             \
    temp2v = vec_sld( tempbv, tempcv, 14 );             \
    temp3v = tempcv;                                    \
    temp4v = vec_sld( tempcv, tempdv,  2 );             \
    temp5v = vec_sld( tempcv, tempdv,  4 );             \
    temp6v = vec_sld( tempcv, tempdv,  6 );             \
                                                        \
    HPEL_FILTER_2( temp1v, temp2v, temp3v,              \
                   temp4v, temp5v, temp6v );            \
                                                        \
    dest2v = vec_add( temp1v, thirtytwov );             \
    dest2v = vec_sra( dest2v, sixv );                   \
                                                        \
    destv = vec_packsu( dest1v, dest2v );               \
                                                        \
    VEC_STORE16( destv, &dstc[x-16+i_stride*y], dsth ); \
}

void x264_hpel_filter_altivec( uint8_t *dsth, uint8_t *dstv, uint8_t *dstc, uint8_t *src,
                               int i_stride, int i_width, int i_height, int16_t *buf )
{
    int x, y;

    vec_u8_t destv;
    vec_u8_t src1v, src2v, src3v, src4v, src5v, src6v;
    vec_s16_t dest1v, dest2v;
    vec_s16_t temp1v, temp2v, temp3v, temp4v, temp5v, temp6v, temp7v, temp8v, temp9v;
    vec_s16_t tempav, tempbv, tempcv, tempdv, tempev;

    PREP_LOAD;
    PREP_LOAD_SRC( src);
    PREP_STORE16;
    PREP_STORE16_DST( dsth );
    LOAD_ZERO;

    vec_u16_t twov, fourv, fivev, sixv;
    vec_s16_t sixteenv, thirtytwov;
    vec_u16_u temp_u;

    temp_u.s[0]=2;
    twov = vec_splat( temp_u.v, 0 );
    temp_u.s[0]=4;
    fourv = vec_splat( temp_u.v, 0 );
    temp_u.s[0]=5;
    fivev = vec_splat( temp_u.v, 0 );
    temp_u.s[0]=6;
    sixv = vec_splat( temp_u.v, 0 );
    temp_u.s[0]=16;
    sixteenv = (vec_s16_t)vec_splat( temp_u.v, 0 );
    temp_u.s[0]=32;
    thirtytwov = (vec_s16_t)vec_splat( temp_u.v, 0 );

    for( y = 0; y < i_height; y++ )
    {
        x = 0;

        /* horizontal_filter */
        HPEL_FILTER_HORIZONTAL();

        /* vertical_filter */
        HPEL_FILTER_VERTICAL();

        /* central_filter */
        tempav = tempcv;
        tempbv = tempdv;
        tempcv = vec_splat( temp1v, 0 ); /* first only */
        tempdv = temp1v;
        tempev = temp4v;

        for( x = 16; x < i_width; x+=16 )
        {
            /* horizontal_filter */
            HPEL_FILTER_HORIZONTAL();

            /* vertical_filter */
            HPEL_FILTER_VERTICAL();

            /* central_filter */
            tempav = tempcv;
            tempbv = tempdv;
            tempcv = tempev;
            tempdv = temp1v;
            tempev = temp4v;

            HPEL_FILTER_CENTRAL();
        }

        /* Partial vertical filter */
        VEC_LOAD_PARTIAL( &src[x+i_stride*(y-2)], src1v, 16, vec_u8_t, src );
        VEC_LOAD_PARTIAL( &src[x+i_stride*(y-1)], src2v, 16, vec_u8_t, src );
        VEC_LOAD_PARTIAL( &src[x+i_stride*(y-0)], src3v, 16, vec_u8_t, src );
        VEC_LOAD_PARTIAL( &src[x+i_stride*(y+1)], src4v, 16, vec_u8_t, src );
        VEC_LOAD_PARTIAL( &src[x+i_stride*(y+2)], src5v, 16, vec_u8_t, src );
        VEC_LOAD_PARTIAL( &src[x+i_stride*(y+3)], src6v, 16, vec_u8_t, src );

        temp1v = vec_u8_to_s16_h( src1v );
        temp2v = vec_u8_to_s16_h( src2v );
        temp3v = vec_u8_to_s16_h( src3v );
        temp4v = vec_u8_to_s16_h( src4v );
        temp5v = vec_u8_to_s16_h( src5v );
        temp6v = vec_u8_to_s16_h( src6v );

        HPEL_FILTER_1( temp1v, temp2v, temp3v,
                       temp4v, temp5v, temp6v );

        /* central_filter */
        tempav = tempcv;
        tempbv = tempdv;
        tempcv = tempev;
        tempdv = temp1v;
        /* tempev is not used */

        HPEL_FILTER_CENTRAL();
    }
}

void x264_mc_altivec_init( x264_mc_functions_t *pf )
{
    pf->mc_luma   = mc_luma_altivec;
    pf->get_ref   = get_ref_altivec;
    pf->mc_chroma = mc_chroma_altivec;

    pf->copy_16x16_unaligned = x264_mc_copy_w16_altivec;
    pf->copy[PIXEL_16x16] = x264_mc_copy_w16_aligned_altivec;

    pf->hpel_filter = x264_hpel_filter_altivec;
}

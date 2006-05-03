/*****************************************************************************
 * frame.c: h264 encoder library
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar
 * $Id: frame.c,v 1.1 2004/06/03 19:27:06 fenrir Exp $
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

x264_frame_t *x264_frame_new( x264_t *h )
{
    x264_frame_t   *frame = x264_malloc( sizeof( x264_frame_t ) );
    int i, j;

    int i_mb_count = h->mb.i_mb_count;
    int i_stride;
    int i_lines;

    memset( frame, 0, sizeof(x264_frame_t) );

    /* allocate frame data (+64 for extra data for me) */
    i_stride = ( ( h->param.i_width  + 15 )&0xfffff0 )+ 64;
    i_lines  = ( ( h->param.i_height + 15 )&0xfffff0 );

    frame->i_plane = 3;
    for( i = 0; i < 3; i++ )
    {
        int i_divh = 1;
        int i_divw = 1;
        if( i > 0 )
        {
            if( h->param.i_csp == X264_CSP_I420 )
                i_divh = i_divw = 2;
            else if( h->param.i_csp == X264_CSP_I422 )
                i_divw = 2;
        }
        frame->i_stride[i] = i_stride / i_divw;
        frame->i_lines[i] = i_lines / i_divh;
        frame->buffer[i] = x264_malloc( frame->i_stride[i] *
                                        ( frame->i_lines[i] + 64 / i_divh ) );

        frame->plane[i] = ((uint8_t*)frame->buffer[i]) +
                          frame->i_stride[i] * 32 / i_divh + 32 / i_divw;
    }
    frame->i_stride[3] = 0;
    frame->i_lines[3] = 0;
    frame->buffer[3] = NULL;
    frame->plane[3] = NULL;

    frame->filtered[0] = frame->plane[0];
    for( i = 0; i < 3; i++ )
    {
        frame->buffer[4+i] = x264_malloc( frame->i_stride[0] *
                                        ( frame->i_lines[0] + 64 ) );

        frame->filtered[i+1] = ((uint8_t*)frame->buffer[4+i]) +
                                frame->i_stride[0] * 32 + 32;
    }

    if( h->frames.b_have_lowres )
    {
        frame->i_stride_lowres = frame->i_stride[0]/2 + 32;
        frame->i_lines_lowres = frame->i_lines[0]/2;
        for( i = 0; i < 4; i++ )
        {
            frame->buffer[7+i] = x264_malloc( frame->i_stride_lowres *
                                            ( frame->i_lines[0]/2 + 64 ) );
            frame->lowres[i] = ((uint8_t*)frame->buffer[7+i]) +
                                frame->i_stride_lowres * 32 + 32;
        }
    }

    if( h->param.analyse.i_me_method == X264_ME_ESA )
    {
        frame->buffer[11] = x264_malloc( frame->i_stride[0] * (frame->i_lines[0] + 64) * sizeof(uint16_t) );
        frame->integral = (uint16_t*)frame->buffer[11] + frame->i_stride[0] * 32 + 32;
    }

    frame->i_poc = -1;
    frame->i_type = X264_TYPE_AUTO;
    frame->i_qpplus1 = 0;
    frame->i_pts = -1;
    frame->i_frame = -1;
    frame->i_frame_num = -1;

    frame->mb_type= x264_malloc( i_mb_count * sizeof( int8_t) );
    frame->mv[0]  = x264_malloc( 2*16 * i_mb_count * sizeof( int16_t ) );
    frame->ref[0] = x264_malloc( 4 * i_mb_count * sizeof( int8_t ) );
    if( h->param.i_bframe )
    {
        frame->mv[1]  = x264_malloc( 2*16 * i_mb_count * sizeof( int16_t ) );
        frame->ref[1] = x264_malloc( 4 * i_mb_count * sizeof( int8_t ) );
    }
    else
    {
        frame->mv[1]  = NULL;
        frame->ref[1] = NULL;
    }

    frame->i_row_bits = x264_malloc( i_lines/16 * sizeof( int ) );
    frame->i_row_qp   = x264_malloc( i_lines/16 * sizeof( int ) );
    for( i = 0; i < h->param.i_bframe + 2; i++ )
        for( j = 0; j < h->param.i_bframe + 2; j++ )
            frame->i_row_satds[i][j] = x264_malloc( i_lines/16 * sizeof( int ) );

    return frame;
}

void x264_frame_delete( x264_frame_t *frame )
{
    int i, j;
    for( i = 0; i < frame->i_plane; i++ )
        x264_free( frame->buffer[i] );
    for( i = 4; i < 12; i++ ) /* filtered planes */
        x264_free( frame->buffer[i] );
    for( i = 0; i < X264_BFRAME_MAX+2; i++ )
        for( j = 0; j < X264_BFRAME_MAX+2; j++ )
            x264_free( frame->i_row_satds[i][j] );
    x264_free( frame->i_row_bits );
    x264_free( frame->i_row_qp );
    x264_free( frame->mb_type );
    x264_free( frame->mv[0] );
    x264_free( frame->mv[1] );
    x264_free( frame->ref[0] );
    x264_free( frame->ref[1] );
    x264_free( frame );
}

void x264_frame_copy_picture( x264_t *h, x264_frame_t *dst, x264_picture_t *src )
{
    dst->i_type     = src->i_type;
    dst->i_qpplus1  = src->i_qpplus1;
    dst->i_pts      = src->i_pts;

    switch( src->img.i_csp & X264_CSP_MASK )
    {
        case X264_CSP_I420:
            h->csp.i420( dst, &src->img, h->param.i_width, h->param.i_height );
            break;
        case X264_CSP_YV12:
            h->csp.yv12( dst, &src->img, h->param.i_width, h->param.i_height );
            break;
        case X264_CSP_I422:
            h->csp.i422( dst, &src->img, h->param.i_width, h->param.i_height );
            break;
        case X264_CSP_I444:
            h->csp.i444( dst, &src->img, h->param.i_width, h->param.i_height );
            break;
        case X264_CSP_YUYV:
            h->csp.yuyv( dst, &src->img, h->param.i_width, h->param.i_height );
            break;
        case X264_CSP_RGB:
            h->csp.rgb( dst, &src->img, h->param.i_width, h->param.i_height );
            break;
        case X264_CSP_BGR:
            h->csp.bgr( dst, &src->img, h->param.i_width, h->param.i_height );
            break;
        case X264_CSP_BGRA:
            h->csp.bgra( dst, &src->img, h->param.i_width, h->param.i_height );
            break;

        default:
            x264_log( h, X264_LOG_ERROR, "Arg invalid CSP\n" );
            break;
    }
}



static void plane_expand_border( uint8_t *pix, int i_stride, int i_height, int i_pad )
{
#define PPIXEL(x, y) ( pix + (x) + (y)*i_stride )
    const int i_width = i_stride - 2*i_pad;
    int y;

    for( y = 0; y < i_height; y++ )
    {
        /* left band */
        memset( PPIXEL(-i_pad, y), PPIXEL(0, y)[0], i_pad );
        /* right band */
        memset( PPIXEL(i_width, y), PPIXEL(i_width-1, y)[0], i_pad );
    }
    /* upper band */
    for( y = 0; y < i_pad; y++ )
        memcpy( PPIXEL(-i_pad, -y-1), PPIXEL(-i_pad, 0), i_stride );
    /* lower band */
    for( y = 0; y < i_pad; y++ )
        memcpy( PPIXEL(-i_pad, i_height+y), PPIXEL(-i_pad, i_height-1), i_stride );
#undef PPIXEL
}

void x264_frame_expand_border( x264_frame_t *frame )
{
    int i;
    for( i = 0; i < frame->i_plane; i++ )
    {
        int i_pad = i ? 16 : 32;
        plane_expand_border( frame->plane[i], frame->i_stride[i], frame->i_lines[i], i_pad );
    }
}

void x264_frame_expand_border_filtered( x264_frame_t *frame )
{
    /* during filtering, 8 extra pixels were filtered on each edge. 
       we want to expand border from the last filtered pixel */
    int i;
    for( i = 1; i < 4; i++ )
        plane_expand_border( frame->filtered[i] - 8*frame->i_stride[0] - 8, frame->i_stride[0], frame->i_lines[0]+2*8, 24 );
}

void x264_frame_expand_border_lowres( x264_frame_t *frame )
{
    int i;
    for( i = 0; i < 4; i++ )
        plane_expand_border( frame->lowres[i], frame->i_stride_lowres, frame->i_lines_lowres, 32 );
}

void x264_frame_expand_border_mod16( x264_t *h, x264_frame_t *frame )
{
    int i, y;
    for( i = 0; i < frame->i_plane; i++ )
    {
        int i_subsample = i ? 1 : 0;
        int i_width = h->param.i_width >> i_subsample;
        int i_height = h->param.i_height >> i_subsample;
        int i_padx = ( h->sps->i_mb_width * 16 - h->param.i_width ) >> i_subsample;
        int i_pady = ( h->sps->i_mb_height * 16 - h->param.i_height ) >> i_subsample;

        if( i_padx )
        {
            for( y = 0; y < i_height; y++ )
                memset( &frame->plane[i][y*frame->i_stride[i] + i_width],
                         frame->plane[i][y*frame->i_stride[i] + i_width - 1],
                         i_padx );
        }
        if( i_pady )
        {
            for( y = i_height; y < i_height + i_pady; y++ )
                memcpy( &frame->plane[i][y*frame->i_stride[i]],
                        &frame->plane[i][(i_height-1)*frame->i_stride[i]],
                        i_width + i_padx );
        }
    }
}


/* Deblocking filter */

static const int i_alpha_table[52] =
{
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  4,  4,  5,  6,
     7,  8,  9, 10, 12, 13, 15, 17, 20, 22,
    25, 28, 32, 36, 40, 45, 50, 56, 63, 71,
    80, 90,101,113,127,144,162,182,203,226,
    255, 255
};
static const int i_beta_table[52] =
{
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  2,  2,  2,  3,
     3,  3,  3,  4,  4,  4,  6,  6,  7,  7,
     8,  8,  9,  9, 10, 10, 11, 11, 12, 12,
    13, 13, 14, 14, 15, 15, 16, 16, 17, 17,
    18, 18
};
static const int i_tc0_table[52][3] =
{
    { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 },
    { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 },
    { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 1 },
    { 0, 0, 1 }, { 0, 0, 1 }, { 0, 0, 1 }, { 0, 1, 1 }, { 0, 1, 1 }, { 1, 1, 1 },
    { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 2 }, { 1, 1, 2 }, { 1, 1, 2 },
    { 1, 1, 2 }, { 1, 2, 3 }, { 1, 2, 3 }, { 2, 2, 3 }, { 2, 2, 4 }, { 2, 3, 4 },
    { 2, 3, 4 }, { 3, 3, 5 }, { 3, 4, 6 }, { 3, 4, 6 }, { 4, 5, 7 }, { 4, 5, 8 },
    { 4, 6, 9 }, { 5, 7,10 }, { 6, 8,11 }, { 6, 8,13 }, { 7,10,14 }, { 8,11,16 },
    { 9,12,18 }, {10,13,20 }, {11,15,23 }, {13,17,25 }
};

/* From ffmpeg */
static inline int clip_uint8( int a )
{
    if (a&(~255))
        return (-a)>>31;
    else
        return a;
}

static inline void deblock_luma_c( uint8_t *pix, int xstride, int ystride, int alpha, int beta, int8_t *tc0 )
{
    int i, d;
    for( i = 0; i < 4; i++ ) {
        if( tc0[i] < 0 ) {
            pix += 4*ystride;
            continue;
        }
        for( d = 0; d < 4; d++ ) {
            const int p2 = pix[-3*xstride];
            const int p1 = pix[-2*xstride];
            const int p0 = pix[-1*xstride];
            const int q0 = pix[ 0*xstride];
            const int q1 = pix[ 1*xstride];
            const int q2 = pix[ 2*xstride];
   
            if( abs( p0 - q0 ) < alpha &&
                abs( p1 - p0 ) < beta &&
                abs( q1 - q0 ) < beta ) {
   
                int tc = tc0[i];
                int delta;
   
                if( abs( p2 - p0 ) < beta ) {
                    pix[-2*xstride] = p1 + x264_clip3( (( p2 + ((p0 + q0 + 1) >> 1)) >> 1) - p1, -tc0[i], tc0[i] );
                    tc++; 
                }
                if( abs( q2 - q0 ) < beta ) {
                    pix[ 1*xstride] = q1 + x264_clip3( (( q2 + ((p0 + q0 + 1) >> 1)) >> 1) - q1, -tc0[i], tc0[i] );
                    tc++;
                }
    
                delta = x264_clip3( (((q0 - p0 ) << 2) + (p1 - q1) + 4) >> 3, -tc, tc );
                pix[-1*xstride] = clip_uint8( p0 + delta );    /* p0' */
                pix[ 0*xstride] = clip_uint8( q0 - delta );    /* q0' */
            }
            pix += ystride;
        }
    }
}
static void deblock_v_luma_c( uint8_t *pix, int stride, int alpha, int beta, int8_t *tc0 )
{
    deblock_luma_c( pix, stride, 1, alpha, beta, tc0 ); 
}
static void deblock_h_luma_c( uint8_t *pix, int stride, int alpha, int beta, int8_t *tc0 )
{
    deblock_luma_c( pix, 1, stride, alpha, beta, tc0 );
}

static inline void deblock_chroma_c( uint8_t *pix, int xstride, int ystride, int alpha, int beta, int8_t *tc0 )
{
    int i, d;
    for( i = 0; i < 4; i++ ) {
        const int tc = tc0[i];
        if( tc <= 0 ) {
            pix += 2*ystride;
            continue;
        }
        for( d = 0; d < 2; d++ ) {
            const int p1 = pix[-2*xstride];
            const int p0 = pix[-1*xstride];
            const int q0 = pix[ 0*xstride];
            const int q1 = pix[ 1*xstride];

            if( abs( p0 - q0 ) < alpha &&
                abs( p1 - p0 ) < beta &&
                abs( q1 - q0 ) < beta ) {

                int delta = x264_clip3( (((q0 - p0 ) << 2) + (p1 - q1) + 4) >> 3, -tc, tc );
                pix[-1*xstride] = clip_uint8( p0 + delta );    /* p0' */
                pix[ 0*xstride] = clip_uint8( q0 - delta );    /* q0' */
            }
            pix += ystride;
        }
    }
}
static void deblock_v_chroma_c( uint8_t *pix, int stride, int alpha, int beta, int8_t *tc0 )
{   
    deblock_chroma_c( pix, stride, 1, alpha, beta, tc0 );
}
static void deblock_h_chroma_c( uint8_t *pix, int stride, int alpha, int beta, int8_t *tc0 )
{   
    deblock_chroma_c( pix, 1, stride, alpha, beta, tc0 );
}

static inline void deblock_luma_intra_c( uint8_t *pix, int xstride, int ystride, int alpha, int beta )
{
    int d;
    for( d = 0; d < 16; d++ ) {
        const int p2 = pix[-3*xstride];
        const int p1 = pix[-2*xstride];
        const int p0 = pix[-1*xstride];
        const int q0 = pix[ 0*xstride];
        const int q1 = pix[ 1*xstride];
        const int q2 = pix[ 2*xstride];

        if( abs( p0 - q0 ) < alpha &&
            abs( p1 - p0 ) < beta &&
            abs( q1 - q0 ) < beta ) {

            if(abs( p0 - q0 ) < ((alpha >> 2) + 2) ){
                if( abs( p2 - p0 ) < beta)
                {
                    const int p3 = pix[-4*xstride];
                    /* p0', p1', p2' */
                    pix[-1*xstride] = ( p2 + 2*p1 + 2*p0 + 2*q0 + q1 + 4 ) >> 3;
                    pix[-2*xstride] = ( p2 + p1 + p0 + q0 + 2 ) >> 2;
                    pix[-3*xstride] = ( 2*p3 + 3*p2 + p1 + p0 + q0 + 4 ) >> 3;
                } else {
                    /* p0' */
                    pix[-1*xstride] = ( 2*p1 + p0 + q1 + 2 ) >> 2;
                }
                if( abs( q2 - q0 ) < beta)
                {
                    const int q3 = pix[3*xstride];
                    /* q0', q1', q2' */
                    pix[0*xstride] = ( p1 + 2*p0 + 2*q0 + 2*q1 + q2 + 4 ) >> 3;
                    pix[1*xstride] = ( p0 + q0 + q1 + q2 + 2 ) >> 2;
                    pix[2*xstride] = ( 2*q3 + 3*q2 + q1 + q0 + p0 + 4 ) >> 3;
                } else {
                    /* q0' */
                    pix[0*xstride] = ( 2*q1 + q0 + p1 + 2 ) >> 2;
                }
            }else{
                /* p0', q0' */
                pix[-1*xstride] = ( 2*p1 + p0 + q1 + 2 ) >> 2;
                pix[ 0*xstride] = ( 2*q1 + q0 + p1 + 2 ) >> 2;
            }
        }
        pix += ystride;
    }
}
static void deblock_v_luma_intra_c( uint8_t *pix, int stride, int alpha, int beta )
{   
    deblock_luma_intra_c( pix, stride, 1, alpha, beta );
}
static void deblock_h_luma_intra_c( uint8_t *pix, int stride, int alpha, int beta )
{   
    deblock_luma_intra_c( pix, 1, stride, alpha, beta );
}

static inline void deblock_chroma_intra_c( uint8_t *pix, int xstride, int ystride, int alpha, int beta )
{   
    int d; 
    for( d = 0; d < 8; d++ ) {
        const int p1 = pix[-2*xstride];
        const int p0 = pix[-1*xstride];
        const int q0 = pix[ 0*xstride];
        const int q1 = pix[ 1*xstride];

        if( abs( p0 - q0 ) < alpha &&
            abs( p1 - p0 ) < beta &&
            abs( q1 - q0 ) < beta ) {

            pix[-1*xstride] = (2*p1 + p0 + q1 + 2) >> 2;   /* p0' */
            pix[ 0*xstride] = (2*q1 + q0 + p1 + 2) >> 2;   /* q0' */
        }

        pix += ystride;
    }
}
static void deblock_v_chroma_intra_c( uint8_t *pix, int stride, int alpha, int beta )
{   
    deblock_chroma_intra_c( pix, stride, 1, alpha, beta );
}
static void deblock_h_chroma_intra_c( uint8_t *pix, int stride, int alpha, int beta )
{   
    deblock_chroma_intra_c( pix, 1, stride, alpha, beta );
}

static inline void deblock_edge( x264_t *h, uint8_t *pix, int i_stride, int bS[4], int i_qp, int b_chroma,
                                 x264_deblock_inter_t pf_inter, x264_deblock_intra_t pf_intra )
{
    int i;
    const int index_a = x264_clip3( i_qp + h->sh.i_alpha_c0_offset, 0, 51 );
    const int alpha = i_alpha_table[index_a];
    const int beta  = i_beta_table[x264_clip3( i_qp + h->sh.i_beta_offset, 0, 51 )];

    if( bS[0] < 4 ) {
        int8_t tc[4]; 
        for(i=0; i<4; i++)
            tc[i] = (bS[i] ? i_tc0_table[index_a][bS[i] - 1] : -1) + b_chroma;
        pf_inter( pix, i_stride, alpha, beta, tc );
    } else {
        pf_intra( pix, i_stride, alpha, beta );
    }
}

void x264_frame_deblocking_filter( x264_t *h, int i_slice_type )
{
    const int s8x8 = 2 * h->mb.i_mb_stride;
    const int s4x4 = 4 * h->mb.i_mb_stride;

    int mb_y, mb_x;

    for( mb_y = 0, mb_x = 0; mb_y < h->sps->i_mb_height; )
    {
        const int mb_xy  = mb_y * h->mb.i_mb_stride + mb_x;
        const int mb_8x8 = 2 * s8x8 * mb_y + 2 * mb_x;
        const int mb_4x4 = 4 * s4x4 * mb_y + 4 * mb_x;
        const int b_8x8_transform = h->mb.mb_transform_size[mb_xy];
        const int i_edge_end = (h->mb.type[mb_xy] == P_SKIP) ? 1 : 4;
        int i_edge, i_dir;

        /* cavlc + 8x8 transform stores nnz per 16 coeffs for the purpose of
         * entropy coding, but per 64 coeffs for the purpose of deblocking */
        if( !h->param.b_cabac && b_8x8_transform )
        {
            uint32_t *nnz = (uint32_t*)h->mb.non_zero_count[mb_xy];
            if( nnz[0] ) nnz[0] = 0x01010101;
            if( nnz[1] ) nnz[1] = 0x01010101;
            if( nnz[2] ) nnz[2] = 0x01010101;
            if( nnz[3] ) nnz[3] = 0x01010101;
        }

        /* i_dir == 0 -> vertical edge
         * i_dir == 1 -> horizontal edge */
        for( i_dir = 0; i_dir < 2; i_dir++ )
        {
            int i_start = (i_dir ? mb_y : mb_x) ? 0 : 1;
            int i_qp, i_qpn;

            for( i_edge = i_start; i_edge < i_edge_end; i_edge++ )
            {
                int mbn_xy, mbn_8x8, mbn_4x4;
                int bS[4];  /* filtering strength */

                if( b_8x8_transform && (i_edge&1) )
                    continue;

                mbn_xy  = i_edge > 0 ? mb_xy  : ( i_dir == 0 ? mb_xy  - 1 : mb_xy - h->mb.i_mb_stride );
                mbn_8x8 = i_edge > 0 ? mb_8x8 : ( i_dir == 0 ? mb_8x8 - 2 : mb_8x8 - 2 * s8x8 );
                mbn_4x4 = i_edge > 0 ? mb_4x4 : ( i_dir == 0 ? mb_4x4 - 4 : mb_4x4 - 4 * s4x4 );

                /* *** Get bS for each 4px for the current edge *** */
                if( IS_INTRA( h->mb.type[mb_xy] ) || IS_INTRA( h->mb.type[mbn_xy] ) )
                {
                    bS[0] = bS[1] = bS[2] = bS[3] = ( i_edge == 0 ? 4 : 3 );
                }
                else
                {
                    int i;
                    for( i = 0; i < 4; i++ )
                    {
                        int x  = i_dir == 0 ? i_edge : i;
                        int y  = i_dir == 0 ? i      : i_edge;
                        int xn = (x - (i_dir == 0 ? 1 : 0 ))&0x03;
                        int yn = (y - (i_dir == 0 ? 0 : 1 ))&0x03;

                        if( h->mb.non_zero_count[mb_xy][block_idx_xy[x][y]] != 0 ||
                            h->mb.non_zero_count[mbn_xy][block_idx_xy[xn][yn]] != 0 )
                        {
                            bS[i] = 2;
                        }
                        else
                        {
                            /* FIXME: A given frame may occupy more than one position in
                             * the reference list. So we should compare the frame numbers,
                             * not the indices in the ref list.
                             * No harm yet, as we don't generate that case.*/

                            int i8p= mb_8x8+(x/2)+(y/2)*s8x8;
                            int i8q= mbn_8x8+(xn/2)+(yn/2)*s8x8;
                            int i4p= mb_4x4+x+y*s4x4;
                            int i4q= mbn_4x4+xn+yn*s4x4;
                            int l;

                            bS[i] = 0;

                            for( l = 0; l < 1 + (i_slice_type == SLICE_TYPE_B); l++ )
                            {
                                if( h->mb.ref[l][i8p] != h->mb.ref[l][i8q] ||
                                    abs( h->mb.mv[l][i4p][0] - h->mb.mv[l][i4q][0] ) >= 4 ||
                                    abs( h->mb.mv[l][i4p][1] - h->mb.mv[l][i4q][1] ) >= 4 )
                                {
                                    bS[i] = 1;
                                    break;
                                }
                            }
                        }
                    }
                }

                /* *** filter *** */
                /* Y plane */
                i_qp = h->mb.qp[mb_xy];
                i_qpn= h->mb.qp[mbn_xy];

                if( i_dir == 0 )
                {
                    /* vertical edge */
                    deblock_edge( h, &h->fdec->plane[0][16*mb_y * h->fdec->i_stride[0] + 16*mb_x + 4*i_edge],
                                  h->fdec->i_stride[0], bS, (i_qp+i_qpn+1) >> 1, 0,
                                  h->loopf.deblock_h_luma, h->loopf.deblock_h_luma_intra );
                    if( !(i_edge & 1) )
                    {
                        /* U/V planes */
                        int i_qpc = ( i_chroma_qp_table[x264_clip3( i_qp + h->pps->i_chroma_qp_index_offset, 0, 51 )] +
                                      i_chroma_qp_table[x264_clip3( i_qpn + h->pps->i_chroma_qp_index_offset, 0, 51 )] + 1 ) >> 1;
                        deblock_edge( h, &h->fdec->plane[1][8*(mb_y*h->fdec->i_stride[1]+mb_x)+2*i_edge],
                                      h->fdec->i_stride[1], bS, i_qpc, 1,
                                      h->loopf.deblock_h_chroma, h->loopf.deblock_h_chroma_intra );
                        deblock_edge( h, &h->fdec->plane[2][8*(mb_y*h->fdec->i_stride[2]+mb_x)+2*i_edge],
                                      h->fdec->i_stride[2], bS, i_qpc, 1,
                                      h->loopf.deblock_h_chroma, h->loopf.deblock_h_chroma_intra );
                    }
                }
                else
                {
                    /* horizontal edge */
                    deblock_edge( h, &h->fdec->plane[0][(16*mb_y + 4*i_edge) * h->fdec->i_stride[0] + 16*mb_x],
                                  h->fdec->i_stride[0], bS, (i_qp+i_qpn+1) >> 1, 0,
                                  h->loopf.deblock_v_luma, h->loopf.deblock_v_luma_intra );
                    /* U/V planes */
                    if( !(i_edge & 1) )
                    {
                        int i_qpc = ( i_chroma_qp_table[x264_clip3( i_qp + h->pps->i_chroma_qp_index_offset, 0, 51 )] +
                                      i_chroma_qp_table[x264_clip3( i_qpn + h->pps->i_chroma_qp_index_offset, 0, 51 )] + 1 ) >> 1;
                        deblock_edge( h, &h->fdec->plane[1][8*(mb_y*h->fdec->i_stride[1]+mb_x)+2*i_edge*h->fdec->i_stride[1]],
                                      h->fdec->i_stride[1], bS, i_qpc, 1,
                                      h->loopf.deblock_v_chroma, h->loopf.deblock_v_chroma_intra );
                        deblock_edge( h, &h->fdec->plane[2][8*(mb_y*h->fdec->i_stride[2]+mb_x)+2*i_edge*h->fdec->i_stride[2]],
                                      h->fdec->i_stride[2], bS, i_qpc, 1,
                                      h->loopf.deblock_v_chroma, h->loopf.deblock_v_chroma_intra );
                    }
                }
            }
        }

        /* newt mb */
        mb_x++;
        if( mb_x >= h->sps->i_mb_width )
        {
            mb_x = 0;
            mb_y++;
        }
    }
}

#ifdef HAVE_MMXEXT
void x264_deblock_v_chroma_mmxext( uint8_t *pix, int stride, int alpha, int beta, int8_t *tc0 );
void x264_deblock_h_chroma_mmxext( uint8_t *pix, int stride, int alpha, int beta, int8_t *tc0 );
void x264_deblock_v_chroma_intra_mmxext( uint8_t *pix, int stride, int alpha, int beta );
void x264_deblock_h_chroma_intra_mmxext( uint8_t *pix, int stride, int alpha, int beta );
#endif

#ifdef ARCH_X86_64
void x264_deblock_v_luma_sse2( uint8_t *pix, int stride, int alpha, int beta, int8_t *tc0 );
void x264_deblock_h_luma_sse2( uint8_t *pix, int stride, int alpha, int beta, int8_t *tc0 );
#elif defined( HAVE_MMXEXT )
void x264_deblock_h_luma_mmxext( uint8_t *pix, int stride, int alpha, int beta, int8_t *tc0 );
void x264_deblock_v8_luma_mmxext( uint8_t *pix, int stride, int alpha, int beta, int8_t *tc0 );

void x264_deblock_v_luma_mmxext( uint8_t *pix, int stride, int alpha, int beta, int8_t *tc0 )
{
    x264_deblock_v8_luma_mmxext( pix,   stride, alpha, beta, tc0   );
    x264_deblock_v8_luma_mmxext( pix+8, stride, alpha, beta, tc0+2 );
}
#endif

void x264_deblock_init( int cpu, x264_deblock_function_t *pf )
{
    pf->deblock_v_luma = deblock_v_luma_c;
    pf->deblock_h_luma = deblock_h_luma_c;
    pf->deblock_v_chroma = deblock_v_chroma_c;
    pf->deblock_h_chroma = deblock_h_chroma_c;
    pf->deblock_v_luma_intra = deblock_v_luma_intra_c;
    pf->deblock_h_luma_intra = deblock_h_luma_intra_c;
    pf->deblock_v_chroma_intra = deblock_v_chroma_intra_c;
    pf->deblock_h_chroma_intra = deblock_h_chroma_intra_c;

#ifdef HAVE_MMXEXT
    if( cpu&X264_CPU_MMXEXT )
    {
        pf->deblock_v_chroma = x264_deblock_v_chroma_mmxext;
        pf->deblock_h_chroma = x264_deblock_h_chroma_mmxext;
        pf->deblock_v_chroma_intra = x264_deblock_v_chroma_intra_mmxext;
        pf->deblock_h_chroma_intra = x264_deblock_h_chroma_intra_mmxext;

#ifdef ARCH_X86_64
        if( cpu&X264_CPU_SSE2 )
        {
            pf->deblock_v_luma = x264_deblock_v_luma_sse2;
            pf->deblock_h_luma = x264_deblock_h_luma_sse2;
        }
#else
        pf->deblock_v_luma = x264_deblock_v_luma_mmxext;
        pf->deblock_h_luma = x264_deblock_h_luma_mmxext;
#endif
    }
#endif
}


/*****************************************************************************
 * deblock.c: h264 encoder library
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

#include "common.h"

/* cavlc + 8x8 transform stores nnz per 16 coeffs for the purpose of
 * entropy coding, but per 64 coeffs for the purpose of deblocking */
static void munge_cavlc_nnz_row( x264_t *h, int mb_y, uint8_t (*buf)[16] )
{
    uint32_t (*src)[6] = (uint32_t(*)[6])h->mb.non_zero_count + mb_y * h->sps->i_mb_width;
    int8_t *transform = h->mb.mb_transform_size + mb_y * h->sps->i_mb_width;
    for( int x = 0; x<h->sps->i_mb_width; x++ )
    {
        memcpy( buf+x, src+x, 16 );
        if( transform[x] )
        {
            int nnz = src[x][0] | src[x][1];
            src[x][0] = src[x][1] = ((uint16_t)nnz ? 0x0101 : 0) + (nnz>>16 ? 0x01010000 : 0);
            nnz = src[x][2] | src[x][3];
            src[x][2] = src[x][3] = ((uint16_t)nnz ? 0x0101 : 0) + (nnz>>16 ? 0x01010000 : 0);
        }
    }
}

static void restore_cavlc_nnz_row( x264_t *h, int mb_y, uint8_t (*buf)[16] )
{
    uint8_t (*dst)[24] = h->mb.non_zero_count + mb_y * h->sps->i_mb_width;
    for( int x = 0; x < h->sps->i_mb_width; x++ )
        memcpy( dst+x, buf+x, 16 );
}

static void munge_cavlc_nnz( x264_t *h, int mb_y, uint8_t (*buf)[16], void (*func)(x264_t*, int, uint8_t (*)[16]) )
{
    func( h, mb_y, buf );
    if( mb_y > 0 )
        func( h, mb_y-1, buf + h->sps->i_mb_width );
    if( h->sh.b_mbaff )
    {
        func( h, mb_y+1, buf + h->sps->i_mb_width * 2 );
        if( mb_y > 0 )
            func( h, mb_y-2, buf + h->sps->i_mb_width * 3 );
    }
}


/* Deblocking filter */
static const uint8_t i_alpha_table[52+12*2] =
{
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  4,  4,  5,  6,
     7,  8,  9, 10, 12, 13, 15, 17, 20, 22,
    25, 28, 32, 36, 40, 45, 50, 56, 63, 71,
    80, 90,101,113,127,144,162,182,203,226,
   255,255,
   255,255,255,255,255,255,255,255,255,255,255,255,
};
static const uint8_t i_beta_table[52+12*2] =
{
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  2,  2,  2,  3,
     3,  3,  3,  4,  4,  4,  6,  6,  7,  7,
     8,  8,  9,  9, 10, 10, 11, 11, 12, 12,
    13, 13, 14, 14, 15, 15, 16, 16, 17, 17,
    18, 18,
    18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
};
static const int8_t i_tc0_table[52+12*2][4] =
{
    {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 },
    {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 },
    {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 },
    {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 },
    {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 1 },
    {-1, 0, 0, 1 }, {-1, 0, 0, 1 }, {-1, 0, 0, 1 }, {-1, 0, 1, 1 }, {-1, 0, 1, 1 }, {-1, 1, 1, 1 },
    {-1, 1, 1, 1 }, {-1, 1, 1, 1 }, {-1, 1, 1, 1 }, {-1, 1, 1, 2 }, {-1, 1, 1, 2 }, {-1, 1, 1, 2 },
    {-1, 1, 1, 2 }, {-1, 1, 2, 3 }, {-1, 1, 2, 3 }, {-1, 2, 2, 3 }, {-1, 2, 2, 4 }, {-1, 2, 3, 4 },
    {-1, 2, 3, 4 }, {-1, 3, 3, 5 }, {-1, 3, 4, 6 }, {-1, 3, 4, 6 }, {-1, 4, 5, 7 }, {-1, 4, 5, 8 },
    {-1, 4, 6, 9 }, {-1, 5, 7,10 }, {-1, 6, 8,11 }, {-1, 6, 8,13 }, {-1, 7,10,14 }, {-1, 8,11,16 },
    {-1, 9,12,18 }, {-1,10,13,20 }, {-1,11,15,23 }, {-1,13,17,25 },
    {-1,13,17,25 }, {-1,13,17,25 }, {-1,13,17,25 }, {-1,13,17,25 }, {-1,13,17,25 }, {-1,13,17,25 },
    {-1,13,17,25 }, {-1,13,17,25 }, {-1,13,17,25 }, {-1,13,17,25 }, {-1,13,17,25 }, {-1,13,17,25 },
};
#define alpha_table(x) i_alpha_table[(x)+12]
#define beta_table(x)  i_beta_table[(x)+12]
#define tc0_table(x)   i_tc0_table[(x)+12]

/* From ffmpeg */
static inline void deblock_luma_c( uint8_t *pix, int xstride, int ystride, int alpha, int beta, int8_t *tc0 )
{
    for( int i = 0; i < 4; i++ )
    {
        if( tc0[i] < 0 )
        {
            pix += 4*ystride;
            continue;
        }
        for( int d = 0; d < 4; d++ )
        {
            int p2 = pix[-3*xstride];
            int p1 = pix[-2*xstride];
            int p0 = pix[-1*xstride];
            int q0 = pix[ 0*xstride];
            int q1 = pix[ 1*xstride];
            int q2 = pix[ 2*xstride];

            if( abs( p0 - q0 ) < alpha && abs( p1 - p0 ) < beta && abs( q1 - q0 ) < beta )
            {
                int tc = tc0[i];
                int delta;
                if( abs( p2 - p0 ) < beta )
                {
                    if( tc0[i] )
                        pix[-2*xstride] = p1 + x264_clip3( (( p2 + ((p0 + q0 + 1) >> 1)) >> 1) - p1, -tc0[i], tc0[i] );
                    tc++;
                }
                if( abs( q2 - q0 ) < beta )
                {
                    if( tc0[i] )
                        pix[ 1*xstride] = q1 + x264_clip3( (( q2 + ((p0 + q0 + 1) >> 1)) >> 1) - q1, -tc0[i], tc0[i] );
                    tc++;
                }

                delta = x264_clip3( (((q0 - p0 ) << 2) + (p1 - q1) + 4) >> 3, -tc, tc );
                pix[-1*xstride] = x264_clip_uint8( p0 + delta );    /* p0' */
                pix[ 0*xstride] = x264_clip_uint8( q0 - delta );    /* q0' */
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
    for( int i = 0; i < 4; i++ )
    {
        int tc = tc0[i];
        if( tc <= 0 )
        {
            pix += 2*ystride;
            continue;
        }
        for( int d = 0; d < 2; d++ )
        {
            int p1 = pix[-2*xstride];
            int p0 = pix[-1*xstride];
            int q0 = pix[ 0*xstride];
            int q1 = pix[ 1*xstride];

            if( abs( p0 - q0 ) < alpha && abs( p1 - p0 ) < beta && abs( q1 - q0 ) < beta )
            {
                int delta = x264_clip3( (((q0 - p0 ) << 2) + (p1 - q1) + 4) >> 3, -tc, tc );
                pix[-1*xstride] = x264_clip_uint8( p0 + delta );    /* p0' */
                pix[ 0*xstride] = x264_clip_uint8( q0 - delta );    /* q0' */
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
    for( int d = 0; d < 16; d++ )
    {
        int p2 = pix[-3*xstride];
        int p1 = pix[-2*xstride];
        int p0 = pix[-1*xstride];
        int q0 = pix[ 0*xstride];
        int q1 = pix[ 1*xstride];
        int q2 = pix[ 2*xstride];

        if( abs( p0 - q0 ) < alpha && abs( p1 - p0 ) < beta && abs( q1 - q0 ) < beta )
        {
            if(abs( p0 - q0 ) < ((alpha >> 2) + 2) )
            {
                if( abs( p2 - p0 ) < beta ) /* p0', p1', p2' */
                {
                    const int p3 = pix[-4*xstride];
                    pix[-1*xstride] = ( p2 + 2*p1 + 2*p0 + 2*q0 + q1 + 4 ) >> 3;
                    pix[-2*xstride] = ( p2 + p1 + p0 + q0 + 2 ) >> 2;
                    pix[-3*xstride] = ( 2*p3 + 3*p2 + p1 + p0 + q0 + 4 ) >> 3;
                }
                else /* p0' */
                    pix[-1*xstride] = ( 2*p1 + p0 + q1 + 2 ) >> 2;
                if( abs( q2 - q0 ) < beta ) /* q0', q1', q2' */
                {
                    const int q3 = pix[3*xstride];
                    pix[0*xstride] = ( p1 + 2*p0 + 2*q0 + 2*q1 + q2 + 4 ) >> 3;
                    pix[1*xstride] = ( p0 + q0 + q1 + q2 + 2 ) >> 2;
                    pix[2*xstride] = ( 2*q3 + 3*q2 + q1 + q0 + p0 + 4 ) >> 3;
                }
                else /* q0' */
                    pix[0*xstride] = ( 2*q1 + q0 + p1 + 2 ) >> 2;
            }
            else /* p0', q0' */
            {
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
    for( int d = 0; d < 8; d++ )
    {
        int p1 = pix[-2*xstride];
        int p0 = pix[-1*xstride];
        int q0 = pix[ 0*xstride];
        int q1 = pix[ 1*xstride];

        if( abs( p0 - q0 ) < alpha && abs( p1 - p0 ) < beta && abs( q1 - q0 ) < beta )
        {
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

static inline void deblock_edge( x264_t *h, uint8_t *pix1, uint8_t *pix2, int i_stride, uint8_t bS[4], int i_qp, int b_chroma, x264_deblock_inter_t pf_inter )
{
    int index_a = i_qp + h->sh.i_alpha_c0_offset;
    int alpha = alpha_table(index_a);
    int beta  = beta_table(i_qp + h->sh.i_beta_offset);
    int8_t tc[4];

    if( !alpha || !beta )
        return;

    tc[0] = tc0_table(index_a)[bS[0]] + b_chroma;
    tc[1] = tc0_table(index_a)[bS[1]] + b_chroma;
    tc[2] = tc0_table(index_a)[bS[2]] + b_chroma;
    tc[3] = tc0_table(index_a)[bS[3]] + b_chroma;

    pf_inter( pix1, i_stride, alpha, beta, tc );
    if( b_chroma )
        pf_inter( pix2, i_stride, alpha, beta, tc );
}

static inline void deblock_edge_intra( x264_t *h, uint8_t *pix1, uint8_t *pix2, int i_stride, uint8_t bS[4], int i_qp, int b_chroma, x264_deblock_intra_t pf_intra )
{
    int alpha = alpha_table(i_qp + h->sh.i_alpha_c0_offset);
    int beta  = beta_table(i_qp + h->sh.i_beta_offset);

    if( !alpha || !beta )
        return;

    pf_intra( pix1, i_stride, alpha, beta );
    if( b_chroma )
        pf_intra( pix2, i_stride, alpha, beta );
}

void x264_frame_deblock_row( x264_t *h, int mb_y )
{
    int s8x8 = 2 * h->mb.i_mb_stride;
    int s4x4 = 4 * h->mb.i_mb_stride;
    int b_interlaced = h->sh.b_mbaff;
    int mvy_limit = 4 >> b_interlaced;
    int qp_thresh = 15 - X264_MIN(h->sh.i_alpha_c0_offset, h->sh.i_beta_offset) - X264_MAX(0, h->param.analyse.i_chroma_qp_offset);
    int no_sub8x8 = !(h->param.analyse.inter & X264_ANALYSE_PSUB8x8);
    int mb_x;
    int stridey   = h->fdec->i_stride[0];
    int stride2y  = stridey << b_interlaced;
    int strideuv  = h->fdec->i_stride[1];
    int stride2uv = strideuv << b_interlaced;
    int deblock_on_slice_edges = h->sh.i_disable_deblocking_filter_idc != 2;
    int ref_table[32+2];
    #define ref_table(x) ref_table[x+2]
    uint8_t (*nnz_backup)[16] = h->scratch_buffer;

    ref_table(-2) = -2;
    ref_table(-1) = -1;
    for( int i = 0; i < (h->i_ref0 << h->sh.b_mbaff); i++ )
    {
        if( !h->mb.b_interlaced )
            ref_table(i) = h->fref0[i]->i_poc;
        else
            ref_table(i) = h->fref0[i>>1]->i_poc + (i&1);
    }

    if( !h->pps->b_cabac && h->pps->b_transform_8x8_mode )
        munge_cavlc_nnz( h, mb_y, nnz_backup, munge_cavlc_nnz_row );

    for( mb_x = 0; mb_x < h->sps->i_mb_width; mb_x += (~b_interlaced | mb_y)&1, mb_y ^= b_interlaced )
    {
        int mb_xy  = mb_y * h->mb.i_mb_stride + mb_x;
        int mb_8x8 = 2 * s8x8 * mb_y + 2 * mb_x;
        int mb_4x4 = 4 * s4x4 * mb_y + 4 * mb_x;
        int b_8x8_transform = h->mb.mb_transform_size[mb_xy];
        int i_qp = h->mb.qp[mb_xy];
        int i_edge_end = (h->mb.type[mb_xy] == P_SKIP) ? 1 : 4;
        uint8_t *pixy = h->fdec->plane[0] + 16*mb_y*stridey  + 16*mb_x;
        uint8_t *pixu = h->fdec->plane[1] +  8*mb_y*strideuv +  8*mb_x;
        uint8_t *pixv = h->fdec->plane[2] +  8*mb_y*strideuv +  8*mb_x;
        if( b_interlaced && (mb_y&1) )
        {
            pixy -= 15*stridey;
            pixu -=  7*strideuv;
            pixv -=  7*strideuv;
        }

        x264_prefetch_fenc( h, h->fdec, mb_x, mb_y );

        if( i_qp <= qp_thresh )
            i_edge_end = 1;

        #define FILTER_DIR(intra, i_dir)\
        {\
            /* Y plane */\
            i_qpn= h->mb.qp[mbn_xy];\
            if( i_dir == 0 )\
            {\
                /* vertical edge */\
                deblock_edge##intra( h, pixy + 4*i_edge, NULL,\
                              stride2y, bS, (i_qp+i_qpn+1) >> 1, 0,\
                              h->loopf.deblock_h_luma##intra );\
                if( !(i_edge & 1) )\
                {\
                    /* U/V planes */\
                    int i_qpc = (h->chroma_qp_table[i_qp] + h->chroma_qp_table[i_qpn] + 1) >> 1;\
                    deblock_edge##intra( h, pixu + 2*i_edge, pixv + 2*i_edge,\
                                  stride2uv, bS, i_qpc, 1,\
                                  h->loopf.deblock_h_chroma##intra );\
                }\
            }\
            else\
            {\
                /* horizontal edge */\
                deblock_edge##intra( h, pixy + 4*i_edge*stride2y, NULL,\
                              stride2y, bS, (i_qp+i_qpn+1) >> 1, 0,\
                              h->loopf.deblock_v_luma##intra );\
                /* U/V planes */\
                if( !(i_edge & 1) )\
                {\
                    int i_qpc = (h->chroma_qp_table[i_qp] + h->chroma_qp_table[i_qpn] + 1) >> 1;\
                    deblock_edge##intra( h, pixu + 2*i_edge*stride2uv, pixv + 2*i_edge*stride2uv,\
                                  stride2uv, bS, i_qpc, 1,\
                                  h->loopf.deblock_v_chroma##intra );\
                }\
            }\
        }

        #define DEBLOCK_STRENGTH(i_dir)\
        {\
            /* *** Get bS for each 4px for the current edge *** */\
            if( IS_INTRA( h->mb.type[mb_xy] ) || IS_INTRA( h->mb.type[mbn_xy]) )\
                M32( bS ) = 0x03030303;\
            else\
            {\
                M32( bS ) = 0x00000000;\
                for( int i = 0; i < 4; i++ )\
                {\
                    int x  = i_dir == 0 ? i_edge : i;\
                    int y  = i_dir == 0 ? i      : i_edge;\
                    int xn = i_dir == 0 ? (x - 1)&0x03 : x;\
                    int yn = i_dir == 0 ? y : (y - 1)&0x03;\
                    if( h->mb.non_zero_count[mb_xy][x+y*4] != 0 ||\
                        h->mb.non_zero_count[mbn_xy][xn+yn*4] != 0 )\
                        bS[i] = 2;\
                    else if(!(i_edge&no_sub8x8))\
                    {\
                        if((i&no_sub8x8) && bS[i-1] != 2)\
                            bS[i] = bS[i-1];\
                        else\
                        {\
                            int i8p= mb_8x8+(x>>1)+(y>>1)*s8x8;\
                            int i8q= mbn_8x8+(xn>>1)+(yn>>1)*s8x8;\
                            int i4p= mb_4x4+x+y*s4x4;\
                            int i4q= mbn_4x4+xn+yn*s4x4;\
                            if((!(ref_table(h->mb.ref[0][i8p]) == ref_table(h->mb.ref[0][i8q])) ||\
                                abs( h->mb.mv[0][i4p][0] - h->mb.mv[0][i4q][0] ) >= 4 ||\
                                abs( h->mb.mv[0][i4p][1] - h->mb.mv[0][i4q][1] ) >= mvy_limit ) ||\
                               (h->sh.i_type == SLICE_TYPE_B &&\
                               (h->mb.ref[1][i8p] != h->mb.ref[1][i8q] ||\
                                abs( h->mb.mv[1][i4p][0] - h->mb.mv[1][i4q][0] ) >= 4 ||\
                                abs( h->mb.mv[1][i4p][1] - h->mb.mv[1][i4q][1] ) >= mvy_limit )))\
                            {\
                                bS[i] = 1;\
                            }\
                        }\
                    }\
                }\
            }\
        }

        /* i_dir == 0 -> vertical edge
         * i_dir == 1 -> horizontal edge */
        #define DEBLOCK_DIR(i_dir)\
        {\
            int i_edge = 0;\
            int i_qpn, mbn_xy, mbn_8x8, mbn_4x4;\
            ALIGNED_4( uint8_t bS[4] );  /* filtering strength */\
            /* We don't have to consider the MBAFF case of a slice breaking in the middle\
             * of a row because x264 doesn't support that case.  If we add support for that,\
             * this will have to become significantly more complex. */\
            if( i_dir == 0 && (mb_x == 0 || (!deblock_on_slice_edges &&\
                h->mb.slice_table[mb_xy] != h->mb.slice_table[mb_xy-1])) )\
                i_edge++;\
            if( i_dir == 1 && (mb_y <= b_interlaced || (!deblock_on_slice_edges &&\
                h->mb.slice_table[mb_xy] != h->mb.slice_table[mb_xy-(h->mb.i_mb_stride<<b_interlaced)])) )\
                i_edge++;\
            if( i_edge )\
                i_edge+= b_8x8_transform;\
            else\
            {\
                mbn_xy  = i_dir == 0 ? mb_xy  - 1 : mb_xy - h->mb.i_mb_stride;\
                mbn_8x8 = i_dir == 0 ? mb_8x8 - 2 : mb_8x8 - 2 * s8x8;\
                mbn_4x4 = i_dir == 0 ? mb_4x4 - 4 : mb_4x4 - 4 * s4x4;\
                if( b_interlaced && i_dir == 1 )\
                {\
                    mbn_xy -= h->mb.i_mb_stride;\
                    mbn_8x8 -= 2 * s8x8;\
                    mbn_4x4 -= 4 * s4x4;\
                }\
                else if( IS_INTRA( h->mb.type[mb_xy] ) || IS_INTRA( h->mb.type[mbn_xy]) )\
                {\
                    FILTER_DIR( _intra, i_dir );\
                    goto end##i_dir;\
                }\
                DEBLOCK_STRENGTH(i_dir);\
                if( M32( bS ) )\
                    FILTER_DIR( , i_dir);\
                end##i_dir:\
                i_edge += b_8x8_transform+1;\
            }\
            mbn_xy  = mb_xy;\
            mbn_8x8 = mb_8x8;\
            mbn_4x4 = mb_4x4;\
            for( ; i_edge < i_edge_end; i_edge+=b_8x8_transform+1 )\
            {\
                DEBLOCK_STRENGTH(i_dir);\
                if( M32( bS ) )\
                    FILTER_DIR( , i_dir);\
            }\
        }

        DEBLOCK_DIR(0);
        DEBLOCK_DIR(1);
    }

    if( !h->pps->b_cabac && h->pps->b_transform_8x8_mode )
        munge_cavlc_nnz( h, mb_y, nnz_backup, restore_cavlc_nnz_row );
}

void x264_frame_deblock( x264_t *h )
{
    for( int mb_y = 0; mb_y < h->sps->i_mb_height; mb_y += 1 + h->sh.b_mbaff )
        x264_frame_deblock_row( h, mb_y );
}

#ifdef HAVE_MMX
void x264_deblock_v_chroma_mmxext( uint8_t *pix, int stride, int alpha, int beta, int8_t *tc0 );
void x264_deblock_h_chroma_mmxext( uint8_t *pix, int stride, int alpha, int beta, int8_t *tc0 );
void x264_deblock_v_chroma_intra_mmxext( uint8_t *pix, int stride, int alpha, int beta );
void x264_deblock_h_chroma_intra_mmxext( uint8_t *pix, int stride, int alpha, int beta );

void x264_deblock_v_luma_sse2( uint8_t *pix, int stride, int alpha, int beta, int8_t *tc0 );
void x264_deblock_h_luma_sse2( uint8_t *pix, int stride, int alpha, int beta, int8_t *tc0 );
void x264_deblock_v_luma_intra_sse2( uint8_t *pix, int stride, int alpha, int beta );
void x264_deblock_h_luma_intra_sse2( uint8_t *pix, int stride, int alpha, int beta );
#ifdef ARCH_X86
void x264_deblock_h_luma_mmxext( uint8_t *pix, int stride, int alpha, int beta, int8_t *tc0 );
void x264_deblock_v8_luma_mmxext( uint8_t *pix, int stride, int alpha, int beta, int8_t *tc0 );
void x264_deblock_h_luma_intra_mmxext( uint8_t *pix, int stride, int alpha, int beta );
void x264_deblock_v8_luma_intra_mmxext( uint8_t *pix, int stride, int alpha, int beta );

static void x264_deblock_v_luma_mmxext( uint8_t *pix, int stride, int alpha, int beta, int8_t *tc0 )
{
    x264_deblock_v8_luma_mmxext( pix,   stride, alpha, beta, tc0   );
    x264_deblock_v8_luma_mmxext( pix+8, stride, alpha, beta, tc0+2 );
}
static void x264_deblock_v_luma_intra_mmxext( uint8_t *pix, int stride, int alpha, int beta )
{
    x264_deblock_v8_luma_intra_mmxext( pix,   stride, alpha, beta );
    x264_deblock_v8_luma_intra_mmxext( pix+8, stride, alpha, beta );
}
#endif
#endif

#ifdef ARCH_PPC
void x264_deblock_v_luma_altivec( uint8_t *pix, int stride, int alpha, int beta, int8_t *tc0 );
void x264_deblock_h_luma_altivec( uint8_t *pix, int stride, int alpha, int beta, int8_t *tc0 );
#endif // ARCH_PPC

#ifdef HAVE_ARMV6
void x264_deblock_v_luma_neon( uint8_t *, int, int, int, int8_t * );
void x264_deblock_h_luma_neon( uint8_t *, int, int, int, int8_t * );
void x264_deblock_v_chroma_neon( uint8_t *, int, int, int, int8_t * );
void x264_deblock_h_chroma_neon( uint8_t *, int, int, int, int8_t * );
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

#ifdef HAVE_MMX
    if( cpu&X264_CPU_MMXEXT )
    {
        pf->deblock_v_chroma = x264_deblock_v_chroma_mmxext;
        pf->deblock_h_chroma = x264_deblock_h_chroma_mmxext;
        pf->deblock_v_chroma_intra = x264_deblock_v_chroma_intra_mmxext;
        pf->deblock_h_chroma_intra = x264_deblock_h_chroma_intra_mmxext;
#ifdef ARCH_X86
        pf->deblock_v_luma = x264_deblock_v_luma_mmxext;
        pf->deblock_h_luma = x264_deblock_h_luma_mmxext;
        pf->deblock_v_luma_intra = x264_deblock_v_luma_intra_mmxext;
        pf->deblock_h_luma_intra = x264_deblock_h_luma_intra_mmxext;
#endif
        if( (cpu&X264_CPU_SSE2) && !(cpu&X264_CPU_STACK_MOD4) )
        {
            pf->deblock_v_luma = x264_deblock_v_luma_sse2;
            pf->deblock_h_luma = x264_deblock_h_luma_sse2;
            pf->deblock_v_luma_intra = x264_deblock_v_luma_intra_sse2;
            pf->deblock_h_luma_intra = x264_deblock_h_luma_intra_sse2;
        }
    }
#endif

#ifdef HAVE_ALTIVEC
    if( cpu&X264_CPU_ALTIVEC )
    {
        pf->deblock_v_luma = x264_deblock_v_luma_altivec;
        pf->deblock_h_luma = x264_deblock_h_luma_altivec;
   }
#endif // HAVE_ALTIVEC

#ifdef HAVE_ARMV6
   if( cpu&X264_CPU_NEON )
   {
        pf->deblock_v_luma   = x264_deblock_v_luma_neon;
        pf->deblock_h_luma   = x264_deblock_h_luma_neon;
        pf->deblock_v_chroma = x264_deblock_v_chroma_neon;
        pf->deblock_h_chroma = x264_deblock_h_chroma_neon;
   }
#endif
}

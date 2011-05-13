/*****************************************************************************
 * deblock.c: deblocking
 *****************************************************************************
 * Copyright (C) 2003-2011 x264 project
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
 *
 * This program is also available under a commercial proprietary license.
 * For more information, contact us at licensing@x264.com.
 *****************************************************************************/

#include "common.h"

/* Deblocking filter */
static const uint8_t i_alpha_table[52+12*3] =
{
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  4,  4,  5,  6,
     7,  8,  9, 10, 12, 13, 15, 17, 20, 22,
    25, 28, 32, 36, 40, 45, 50, 56, 63, 71,
    80, 90,101,113,127,144,162,182,203,226,
   255,255,
   255,255,255,255,255,255,255,255,255,255,255,255,
};
static const uint8_t i_beta_table[52+12*3] =
{
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  2,  2,  2,  3,
     3,  3,  3,  4,  4,  4,  6,  6,  7,  7,
     8,  8,  9,  9, 10, 10, 11, 11, 12, 12,
    13, 13, 14, 14, 15, 15, 16, 16, 17, 17,
    18, 18,
    18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
};
static const int8_t i_tc0_table[52+12*3][4] =
{
    {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 },
    {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 }, {-1, 0, 0, 0 },
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
#define alpha_table(x) i_alpha_table[(x)+24]
#define beta_table(x)  i_beta_table[(x)+24]
#define tc0_table(x)   i_tc0_table[(x)+24]

/* From ffmpeg */
static ALWAYS_INLINE void deblock_edge_luma_c( pixel *pix, int xstride, int alpha, int beta, int8_t tc0 )
{
    int p2 = pix[-3*xstride];
    int p1 = pix[-2*xstride];
    int p0 = pix[-1*xstride];
    int q0 = pix[ 0*xstride];
    int q1 = pix[ 1*xstride];
    int q2 = pix[ 2*xstride];

    if( abs( p0 - q0 ) < alpha && abs( p1 - p0 ) < beta && abs( q1 - q0 ) < beta )
    {
        int tc = tc0;
        int delta;
        if( abs( p2 - p0 ) < beta )
        {
            if( tc0 )
                pix[-2*xstride] = p1 + x264_clip3( (( p2 + ((p0 + q0 + 1) >> 1)) >> 1) - p1, -tc0, tc0 );
            tc++;
        }
        if( abs( q2 - q0 ) < beta )
        {
            if( tc0 )
                pix[ 1*xstride] = q1 + x264_clip3( (( q2 + ((p0 + q0 + 1) >> 1)) >> 1) - q1, -tc0, tc0 );
            tc++;
        }

        delta = x264_clip3( (((q0 - p0 ) << 2) + (p1 - q1) + 4) >> 3, -tc, tc );
        pix[-1*xstride] = x264_clip_pixel( p0 + delta );    /* p0' */
        pix[ 0*xstride] = x264_clip_pixel( q0 - delta );    /* q0' */
    }
}
static inline void deblock_luma_c( pixel *pix, int xstride, int ystride, int alpha, int beta, int8_t *tc0 )
{
    for( int i = 0; i < 4; i++ )
    {
        if( tc0[i] < 0 )
        {
            pix += 4*ystride;
            continue;
        }
        for( int d = 0; d < 4; d++, pix += ystride )
            deblock_edge_luma_c( pix, xstride, alpha, beta, tc0[i] );
    }
}
static inline void deblock_v_luma_mbaff_c( pixel *pix, int stride, int alpha, int beta, int8_t *tc0 )
{
    for( int d = 0; d < 8; d++, pix += stride )
        deblock_edge_luma_c( pix, 1, alpha, beta, tc0[d>>1] );
}
static void deblock_v_luma_c( pixel *pix, int stride, int alpha, int beta, int8_t *tc0 )
{
    deblock_luma_c( pix, stride, 1, alpha, beta, tc0 );
}
static void deblock_h_luma_c( pixel *pix, int stride, int alpha, int beta, int8_t *tc0 )
{
    deblock_luma_c( pix, 1, stride, alpha, beta, tc0 );
}

static ALWAYS_INLINE void deblock_edge_chroma_c( pixel *pix, int xstride, int alpha, int beta, int8_t tc )
{
    int p1 = pix[-2*xstride];
    int p0 = pix[-1*xstride];
    int q0 = pix[ 0*xstride];
    int q1 = pix[ 1*xstride];

    if( abs( p0 - q0 ) < alpha && abs( p1 - p0 ) < beta && abs( q1 - q0 ) < beta )
    {
        int delta = x264_clip3( (((q0 - p0 ) << 2) + (p1 - q1) + 4) >> 3, -tc, tc );
        pix[-1*xstride] = x264_clip_pixel( p0 + delta );    /* p0' */
        pix[ 0*xstride] = x264_clip_pixel( q0 - delta );    /* q0' */
    }
}
static inline void deblock_chroma_c( pixel *pix, int xstride, int ystride, int alpha, int beta, int8_t *tc0 )
{
    for( int i = 0; i < 4; i++ )
    {
        int tc = tc0[i];
        if( tc <= 0 )
        {
            pix += 2*ystride;
            continue;
        }
        for( int d = 0; d < 2; d++, pix += ystride-2 )
        for( int e = 0; e < 2; e++, pix++ )
            deblock_edge_chroma_c( pix, xstride, alpha, beta, tc0[i] );
    }
}
static inline void deblock_v_chroma_mbaff_c( pixel *pix, int stride, int alpha, int beta, int8_t *tc0 )
{
    for( int i = 0; i < 4; i++, pix += stride )
        deblock_edge_chroma_c( pix, 2, alpha, beta, tc0[i] );
}
static void deblock_v_chroma_c( pixel *pix, int stride, int alpha, int beta, int8_t *tc0 )
{
    deblock_chroma_c( pix, stride, 2, alpha, beta, tc0 );
}
static void deblock_h_chroma_c( pixel *pix, int stride, int alpha, int beta, int8_t *tc0 )
{
    deblock_chroma_c( pix, 2, stride, alpha, beta, tc0 );
}

static ALWAYS_INLINE void deblock_edge_luma_intra_c( pixel *pix, int xstride, int alpha, int beta )
{
    int p2 = pix[-3*xstride];
    int p1 = pix[-2*xstride];
    int p0 = pix[-1*xstride];
    int q0 = pix[ 0*xstride];
    int q1 = pix[ 1*xstride];
    int q2 = pix[ 2*xstride];

    if( abs( p0 - q0 ) < alpha && abs( p1 - p0 ) < beta && abs( q1 - q0 ) < beta )
    {
        if( abs( p0 - q0 ) < ((alpha >> 2) + 2) )
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
}
static inline void deblock_luma_intra_c( pixel *pix, int xstride, int ystride, int alpha, int beta )
{
    for( int d = 0; d < 16; d++, pix += ystride )
        deblock_edge_luma_intra_c( pix, xstride, alpha, beta );
}
static inline void deblock_v_luma_intra_mbaff_c( pixel *pix, int ystride, int alpha, int beta )
{
    for( int d = 0; d < 8; d++, pix += ystride )
        deblock_edge_luma_intra_c( pix, 1, alpha, beta );
}
static void deblock_v_luma_intra_c( pixel *pix, int stride, int alpha, int beta )
{
    deblock_luma_intra_c( pix, stride, 1, alpha, beta );
}
static void deblock_h_luma_intra_c( pixel *pix, int stride, int alpha, int beta )
{
    deblock_luma_intra_c( pix, 1, stride, alpha, beta );
}

static ALWAYS_INLINE void deblock_edge_chroma_intra_c( pixel *pix, int xstride, int alpha, int beta )
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
}
static inline void deblock_chroma_intra_c( pixel *pix, int xstride, int ystride, int alpha, int beta, int dir )
{
    for( int d = 0; d < (dir?16:8); d++, pix += ystride-2 )
    for( int e = 0; e < (dir?1:2); e++, pix++ )
        deblock_edge_chroma_intra_c( pix, xstride, alpha, beta );
}
static inline void deblock_v_chroma_intra_mbaff_c( pixel *pix, int stride, int alpha, int beta )
{
    for( int i = 0; i < 4; i++, pix += stride )
        deblock_edge_chroma_intra_c( pix, 2, alpha, beta );
}
static void deblock_v_chroma_intra_c( pixel *pix, int stride, int alpha, int beta )
{
    deblock_chroma_intra_c( pix, stride, 2, alpha, beta, 1 );
}
static void deblock_h_chroma_intra_c( pixel *pix, int stride, int alpha, int beta )
{
    deblock_chroma_intra_c( pix, 2, stride, alpha, beta, 0 );
}

static void deblock_strength_c( uint8_t nnz[X264_SCAN8_SIZE], int8_t ref[2][X264_SCAN8_LUMA_SIZE],
                                int16_t mv[2][X264_SCAN8_LUMA_SIZE][2], uint8_t bs[2][8][4], int mvy_limit,
                                int bframe, x264_t *h )
{
    for( int dir = 0; dir < 2; dir++ )
    {
        int s1 = dir ? 1 : 8;
        int s2 = dir ? 8 : 1;
        for( int edge = 0; edge < 4; edge++ )
            for( int i = 0, loc = X264_SCAN8_0+edge*s2; i < 4; i++, loc += s1 )
            {
                int locn = loc - s2;
                if( nnz[loc] || nnz[locn] )
                    bs[dir][edge][i] = 2;
                else if( ref[0][loc] != ref[0][locn] ||
                         abs( mv[0][loc][0] - mv[0][locn][0] ) >= 4 ||
                         abs( mv[0][loc][1] - mv[0][locn][1] ) >= mvy_limit ||
                        (bframe && (ref[1][loc] != ref[1][locn] ||
                         abs( mv[1][loc][0] - mv[1][locn][0] ) >= 4 ||
                         abs( mv[1][loc][1] - mv[1][locn][1] ) >= mvy_limit )))
                {
                    bs[dir][edge][i] = 1;
                }
                else
                    bs[dir][edge][i] = 0;
            }
    }
}

void deblock_strength_mbaff_c( uint8_t nnz_cache[X264_SCAN8_SIZE], int8_t ref[2][X264_SCAN8_LUMA_SIZE],
                               int16_t mv[2][X264_SCAN8_LUMA_SIZE][2], uint8_t bs[2][8][4],
                               int mvy_limit, int bframe, x264_t *h )
{
    int neighbour_field[2];
    neighbour_field[0] = h->mb.i_mb_left_xy[0] >= 0 && h->mb.field[h->mb.i_mb_left_xy[0]];
    neighbour_field[1] = h->mb.i_mb_top_xy >= 0 && h->mb.field[h->mb.i_mb_top_xy];
    int intra_cur = IS_INTRA( h->mb.i_type );

    if( !intra_cur )
    {
        for( int dir = 0; dir < 2; dir++ )
        {
            int edge_stride = dir ? 8 : 1;
            int part_stride = dir ? 1 : 8;
            for( int edge = 0; edge < 4; edge++ )
            {
                for( int i = 0, q = X264_SCAN8_0+edge*edge_stride; i < 4; i++, q += part_stride )
                {
                    int p = q - edge_stride;
                    if( nnz_cache[q] || nnz_cache[p] )
                    {
                        bs[dir][edge][i] = 2;
                    }
                    else if( (edge == 0 && MB_INTERLACED != neighbour_field[dir]) ||
                             ref[0][q] != ref[0][p] ||
                             abs( mv[0][q][0] - mv[0][p][0] ) >= 4 ||
                             abs( mv[0][q][1] - mv[0][p][1] ) >= mvy_limit ||
                            (bframe && (ref[1][q] != ref[1][p] ||
                             abs( mv[1][q][0] - mv[1][p][0] ) >= 4 ||
                             abs( mv[1][q][1] - mv[1][p][1] ) >= mvy_limit )) )
                    {
                        bs[dir][edge][i] = 1;
                    }
                    else
                        bs[dir][edge][i] = 0;
                }
            }
        }
    }

    if( h->mb.i_neighbour & MB_LEFT )
    {
        if( h->mb.field[h->mb.i_mb_left_xy[0]] != MB_INTERLACED )
        {
            static const uint8_t offset[2][2][8] = {
                {   { 0, 0, 0, 0, 1, 1, 1, 1 },
                    { 2, 2, 2, 2, 3, 3, 3, 3 }, },
                {   { 0, 1, 2, 3, 0, 1, 2, 3 },
                    { 0, 1, 2, 3, 0, 1, 2, 3 }, }
            };
            uint8_t bS[8];

            if( intra_cur )
                memset( bS, 4, 8 );
            else
            {
                const uint8_t *off = offset[MB_INTERLACED][h->mb.i_mb_y&1];
                uint8_t (*nnz)[24] = h->mb.non_zero_count;

                for( int i = 0; i < 8; i++ )
                {
                    int left = h->mb.i_mb_left_xy[MB_INTERLACED ? i>>2 : i&1];
                    int nnz_this = h->mb.cache.non_zero_count[x264_scan8[0]+8*(i>>1)];
                    int nnz_left = nnz[left][3 + 4*off[i]];
                    if( !h->param.b_cabac && h->pps->b_transform_8x8_mode )
                    {
                        int j = off[i]&~1;
                        if( h->mb.mb_transform_size[left] )
                            nnz_left = !!(M16( &nnz[left][2+4*j] ) | M16( &nnz[left][2+4*(1+j)] ));
                    }
                    if( IS_INTRA( h->mb.type[left] ) )
                        bS[i] = 4;
                    else if( nnz_left || nnz_this )
                        bS[i] = 2;
                    else // As left is different interlaced.
                        bS[i] = 1;
                }
            }

            if( MB_INTERLACED )
            {
                for( int i = 0; i < 4; i++ ) bs[0][0][i] = bS[i];
                for( int i = 0; i < 4; i++ ) bs[0][4][i] = bS[4+i];
            }
            else
            {
                for( int i = 0; i < 4; i++ ) bs[0][0][i] = bS[2*i];
                for( int i = 0; i < 4; i++ ) bs[0][4][i] = bS[1+2*i];
            }
        }
    }

    if( h->mb.i_neighbour & MB_TOP )
    {
        if( !(h->mb.i_mb_y&1) && !MB_INTERLACED && h->mb.field[h->mb.i_mb_top_xy] )
        {
            /* Need to filter both fields (even for frame macroblocks).
             * Filter top two rows using the top macroblock of the above
             * pair and then the bottom one. */
            int mbn_xy = h->mb.i_mb_xy - 2 * h->mb.i_mb_stride;
            uint32_t nnz_cur[4];
            nnz_cur[0] = h->mb.cache.non_zero_count[x264_scan8[0]+0];
            nnz_cur[1] = h->mb.cache.non_zero_count[x264_scan8[0]+1];
            nnz_cur[2] = h->mb.cache.non_zero_count[x264_scan8[0]+2];
            nnz_cur[3] = h->mb.cache.non_zero_count[x264_scan8[0]+3];
            /* Munge NNZ for cavlc + 8x8dct */
            if( !h->param.b_cabac && h->pps->b_transform_8x8_mode &&
                h->mb.mb_transform_size[h->mb.i_mb_xy] )
            {
                int nnz0 = M16( &h->mb.cache.non_zero_count[x264_scan8[ 0]] ) | M16( &h->mb.cache.non_zero_count[x264_scan8[ 2]] );
                int nnz1 = M16( &h->mb.cache.non_zero_count[x264_scan8[ 4]] ) | M16( &h->mb.cache.non_zero_count[x264_scan8[ 6]] );
                nnz_cur[0] = nnz_cur[1] = !!nnz0;
                nnz_cur[2] = nnz_cur[3] = !!nnz1;
            }

            for( int j = 0; j < 2; j++, mbn_xy += h->mb.i_mb_stride )
            {
                int mbn_intra = IS_INTRA( h->mb.type[mbn_xy] );
                uint8_t (*nnz)[24] = h->mb.non_zero_count;

                uint32_t nnz_top[4];
                nnz_top[0] = nnz[mbn_xy][3*4+0];
                nnz_top[1] = nnz[mbn_xy][3*4+1];
                nnz_top[2] = nnz[mbn_xy][3*4+2];
                nnz_top[3] = nnz[mbn_xy][3*4+3];

                if( !h->param.b_cabac && h->pps->b_transform_8x8_mode &&
                    (h->mb.i_neighbour & MB_TOP) && h->mb.mb_transform_size[mbn_xy] )
                {
                    int nnz_top0 = M16( &nnz[mbn_xy][8] ) | M16( &nnz[mbn_xy][12] );
                    int nnz_top1 = M16( &nnz[mbn_xy][10] ) | M16( &nnz[mbn_xy][14] );
                    nnz_top[0] = nnz_top[1] = nnz_top0 ? 0x0101 : 0;
                    nnz_top[2] = nnz_top[3] = nnz_top1 ? 0x0101 : 0;
                }

                uint8_t bS[4];
                if( intra_cur || mbn_intra )
                    M32( bS ) = 0x03030303;
                else
                {
                    for( int i = 0; i < 4; i++ )
                    {
                        if( nnz_cur[i] || nnz_top[i] )
                            bS[i] = 2;
                        else
                            bS[i] = 1;
                    }
                }
                for( int i = 0; i < 4; i++ )
                    bs[1][4*j][i] = bS[i];
            }
        }
    }
}

static inline void deblock_edge( x264_t *h, pixel *pix, int i_stride, uint8_t bS[4], int i_qp, int b_chroma, x264_deblock_inter_t pf_inter )
{
    int index_a = i_qp-QP_BD_OFFSET + h->sh.i_alpha_c0_offset;
    int index_b = i_qp-QP_BD_OFFSET + h->sh.i_beta_offset;
    int alpha = alpha_table(index_a) << (BIT_DEPTH-8);
    int beta  = beta_table(index_b) << (BIT_DEPTH-8);
    int8_t tc[4];

    if( !M32(bS) || !alpha || !beta )
        return;

    tc[0] = (tc0_table(index_a)[bS[0]] << (BIT_DEPTH-8)) + b_chroma;
    tc[1] = (tc0_table(index_a)[bS[1]] << (BIT_DEPTH-8)) + b_chroma;
    tc[2] = (tc0_table(index_a)[bS[2]] << (BIT_DEPTH-8)) + b_chroma;
    tc[3] = (tc0_table(index_a)[bS[3]] << (BIT_DEPTH-8)) + b_chroma;

    pf_inter( pix, i_stride, alpha, beta, tc );
}

static inline void deblock_edge_intra( x264_t *h, pixel *pix, int i_stride, uint8_t bS[4], int i_qp, int b_chroma, x264_deblock_intra_t pf_intra )
{
    int index_a = i_qp-QP_BD_OFFSET + h->sh.i_alpha_c0_offset;
    int index_b = i_qp-QP_BD_OFFSET + h->sh.i_beta_offset;
    int alpha = alpha_table(index_a) << (BIT_DEPTH-8);
    int beta  = beta_table(index_b) << (BIT_DEPTH-8);

    if( !alpha || !beta )
        return;

    pf_intra( pix, i_stride, alpha, beta );
}

void x264_frame_deblock_row( x264_t *h, int mb_y )
{
    int b_interlaced = SLICE_MBAFF;
    int qp_thresh = 15 - X264_MIN( h->sh.i_alpha_c0_offset, h->sh.i_beta_offset ) - X264_MAX( 0, h->param.analyse.i_chroma_qp_offset );
    int stridey   = h->fdec->i_stride[0];
    int strideuv  = h->fdec->i_stride[1];

    for( int mb_x = 0; mb_x < h->mb.i_mb_width; mb_x += (~b_interlaced | mb_y)&1, mb_y ^= b_interlaced )
    {
        x264_prefetch_fenc( h, h->fdec, mb_x, mb_y );
        x264_macroblock_cache_load_neighbours_deblock( h, mb_x, mb_y );

        int mb_xy = h->mb.i_mb_xy;
        int transform_8x8 = h->mb.mb_transform_size[h->mb.i_mb_xy];
        int intra_cur = IS_INTRA( h->mb.type[mb_xy] );
        uint8_t (*bs)[8][4] = h->deblock_strength[mb_y&1][mb_x];

        pixel *pixy = h->fdec->plane[0] + 16*mb_y*stridey  + 16*mb_x;
        pixel *pixuv = h->fdec->plane[1] + 8*mb_y*strideuv + 16*mb_x;
        if( mb_y & MB_INTERLACED )
        {
            pixy -= 15*stridey;
            pixuv -= 7*strideuv;
        }

        int stride2y  = stridey << MB_INTERLACED;
        int stride2uv = strideuv << MB_INTERLACED;
        int qp = h->mb.qp[mb_xy];
        int qpc = h->chroma_qp_table[qp];
        int first_edge_only = h->mb.type[mb_xy] == P_SKIP || qp <= qp_thresh;

        #define FILTER( intra, dir, edge, qp, chroma_qp )\
        do\
        {\
            deblock_edge##intra( h, pixy + 4*edge*(dir?stride2y:1),\
                                 stride2y, bs[dir][edge], qp, 0,\
                                 h->loopf.deblock_luma##intra[dir] );\
            if( !(edge & 1) )\
                deblock_edge##intra( h, pixuv + 2*edge*(dir?stride2uv:2),\
                                     stride2uv, bs[dir][edge], chroma_qp, 1,\
                                     h->loopf.deblock_chroma##intra[dir] );\
        } while(0)

        if( h->mb.i_neighbour & MB_LEFT )
        {
            if( b_interlaced && h->mb.field[h->mb.i_mb_left_xy[0]] != MB_INTERLACED )
            {
                int luma_qp[2];
                int chroma_qp[2];
                int left_qp[2];
                int current_qp = h->mb.qp[mb_xy];
                left_qp[0] = h->mb.qp[h->mb.i_mb_left_xy[0]];
                luma_qp[0] = (current_qp + left_qp[0] + 1) >> 1;
                chroma_qp[0] = (h->chroma_qp_table[current_qp] + h->chroma_qp_table[left_qp[0]] + 1) >> 1;
                if( bs[0][0][0] == 4)
                {
                    deblock_edge_intra( h, pixy,      2*stridey,  bs[0][0], luma_qp[0],   0, deblock_v_luma_intra_mbaff_c );
                    deblock_edge_intra( h, pixuv,     2*strideuv, bs[0][0], chroma_qp[0], 1, deblock_v_chroma_intra_mbaff_c );
                    deblock_edge_intra( h, pixuv + 1, 2*strideuv, bs[0][0], chroma_qp[0], 1, deblock_v_chroma_intra_mbaff_c );
                }
                else
                {
                    deblock_edge( h, pixy,      2*stridey,  bs[0][0], luma_qp[0],   0, deblock_v_luma_mbaff_c );
                    deblock_edge( h, pixuv,     2*strideuv, bs[0][0], chroma_qp[0], 1, deblock_v_chroma_mbaff_c );
                    deblock_edge( h, pixuv + 1, 2*strideuv, bs[0][0], chroma_qp[0], 1, deblock_v_chroma_mbaff_c );
                }

                int offy = MB_INTERLACED ? 4 : 0;
                int offuv = MB_INTERLACED ? 3 : 0;
                left_qp[1] = h->mb.qp[h->mb.i_mb_left_xy[1]];
                luma_qp[1] = (current_qp + left_qp[1] + 1) >> 1;
                chroma_qp[1] = (h->chroma_qp_table[current_qp] + h->chroma_qp_table[left_qp[1]] + 1) >> 1;
                if( bs[0][4][0] == 4)
                {
                    deblock_edge_intra( h, pixy      + (stridey<<offy),   2*stridey,  bs[0][4], luma_qp[1],   0, deblock_v_luma_intra_mbaff_c );
                    deblock_edge_intra( h, pixuv     + (strideuv<<offuv), 2*strideuv, bs[0][4], chroma_qp[1], 1, deblock_v_chroma_intra_mbaff_c );
                    deblock_edge_intra( h, pixuv + 1 + (strideuv<<offuv), 2*strideuv, bs[0][4], chroma_qp[1], 1, deblock_v_chroma_intra_mbaff_c );
                }
                else
                {
                    deblock_edge( h, pixy      + (stridey<<offy),   2*stridey,  bs[0][4], luma_qp[1],   0, deblock_v_luma_mbaff_c );
                    deblock_edge( h, pixuv     + (strideuv<<offuv), 2*strideuv, bs[0][4], chroma_qp[1], 1, deblock_v_chroma_mbaff_c );
                    deblock_edge( h, pixuv + 1 + (strideuv<<offuv), 2*strideuv, bs[0][4], chroma_qp[1], 1, deblock_v_chroma_mbaff_c );
                }
            }
            else
            {
                int qpl = h->mb.qp[h->mb.i_mb_xy-1];
                int qp_left = (qp + qpl + 1) >> 1;
                int qpc_left = (h->chroma_qp_table[qp] + h->chroma_qp_table[qpl] + 1) >> 1;
                int intra_left = IS_INTRA( h->mb.type[h->mb.i_mb_xy-1] );

                if( intra_cur || intra_left )
                    FILTER( _intra, 0, 0, qp_left, qpc_left );
                else
                    FILTER(       , 0, 0, qp_left, qpc_left );
            }
        }
        if( !first_edge_only )
        {
            if( !transform_8x8 ) FILTER( , 0, 1, qp, qpc );
                                 FILTER( , 0, 2, qp, qpc );
            if( !transform_8x8 ) FILTER( , 0, 3, qp, qpc );
        }

        if( h->mb.i_neighbour & MB_TOP )
        {
            if( b_interlaced && !(mb_y&1) && !MB_INTERLACED && h->mb.field[h->mb.i_mb_top_xy] )
            {
                int mbn_xy = mb_xy - 2 * h->mb.i_mb_stride;

                for(int j=0; j<2; j++, mbn_xy += h->mb.i_mb_stride)
                {
                    int qpt = h->mb.qp[mbn_xy];
                    int qp_top = (qp + qpt + 1) >> 1;
                    int qpc_top = (h->chroma_qp_table[qp] + h->chroma_qp_table[qpt] + 1) >> 1;

                    // deblock the first horizontal edge of the even rows, then the first horizontal edge of the odd rows
                    deblock_edge( h, pixy      + j*stridey,  2* stridey, bs[1][4*j], qp_top,  0, deblock_v_luma_c );
                    deblock_edge( h, pixuv     + j*strideuv, 2*strideuv, bs[1][4*j], qpc_top, 1, deblock_v_chroma_c );
                }
            }
            else
            {
                int qpt = h->mb.qp[h->mb.i_mb_top_xy];
                int qp_top = (qp + qpt + 1) >> 1;
                int qpc_top = (h->chroma_qp_table[qp] + h->chroma_qp_table[qpt] + 1) >> 1;
                int intra_top = IS_INTRA( h->mb.type[h->mb.i_mb_top_xy] );

                if( (!b_interlaced || (!MB_INTERLACED && !h->mb.field[h->mb.i_mb_top_xy]))
                    && (intra_cur || intra_top) )
                {
                    FILTER( _intra, 1, 0, qp_top, qpc_top );
                }
                else
                {
                    if( intra_top )
                        M32( bs[1][0] ) = 0x03030303;
                    FILTER(       , 1, 0, qp_top, qpc_top );
                }
            }
        }

        if( !first_edge_only )
        {
            if( !transform_8x8 ) FILTER( , 1, 1, qp, qpc );
                                 FILTER( , 1, 2, qp, qpc );
            if( !transform_8x8 ) FILTER( , 1, 3, qp, qpc );
        }

        #undef FILTER
    }
}

/* For deblock-aware RD.
 * TODO:
 *  deblock macroblock edges
 *  support analysis partitions smaller than 16x16
 *  deblock chroma
 *  handle duplicate refs correctly
 *  handle cavlc+8x8dct correctly
 */
void x264_macroblock_deblock( x264_t *h )
{
    int qp_thresh = 15 - X264_MIN( h->sh.i_alpha_c0_offset, h->sh.i_beta_offset ) - X264_MAX( 0, h->pps->i_chroma_qp_index_offset );
    int qp = h->mb.i_qp;
    if( qp <= qp_thresh || h->mb.i_type == P_SKIP )
        return;

    uint8_t (*bs)[8][4] = h->deblock_strength[h->mb.i_mb_y&1][h->mb.i_mb_x];
    if( IS_INTRA( h->mb.i_type ) )
        memset( bs, 3, 2*8*4*sizeof(uint8_t) );
    else
        h->loopf.deblock_strength( h->mb.cache.non_zero_count, h->mb.cache.ref, h->mb.cache.mv,
                                   bs, 4 >> SLICE_MBAFF, h->sh.i_type == SLICE_TYPE_B, h );

    int transform_8x8 = h->mb.b_transform_8x8;
    pixel *fdec = h->mb.pic.p_fdec[0];

    #define FILTER( dir, edge )\
    do\
    {\
        deblock_edge( h, fdec + 4*edge*(dir?FDEC_STRIDE:1),\
                      FDEC_STRIDE, bs[dir][edge], qp, 0,\
                      h->loopf.deblock_luma[dir] );\
    } while(0)

    if( !transform_8x8 ) FILTER( 0, 1 );
                         FILTER( 0, 2 );
    if( !transform_8x8 ) FILTER( 0, 3 );

    if( !transform_8x8 ) FILTER( 1, 1 );
                         FILTER( 1, 2 );
    if( !transform_8x8 ) FILTER( 1, 3 );

    #undef FILTER
}

#if HAVE_MMX
void x264_deblock_v_luma_sse2( pixel *pix, int stride, int alpha, int beta, int8_t *tc0 );
void x264_deblock_v_luma_avx ( pixel *pix, int stride, int alpha, int beta, int8_t *tc0 );
void x264_deblock_h_luma_sse2( pixel *pix, int stride, int alpha, int beta, int8_t *tc0 );
void x264_deblock_h_luma_avx ( pixel *pix, int stride, int alpha, int beta, int8_t *tc0 );
void x264_deblock_v_chroma_sse2( pixel *pix, int stride, int alpha, int beta, int8_t *tc0 );
void x264_deblock_v_chroma_avx ( pixel *pix, int stride, int alpha, int beta, int8_t *tc0 );
void x264_deblock_h_chroma_sse2( pixel *pix, int stride, int alpha, int beta, int8_t *tc0 );
void x264_deblock_h_chroma_avx ( pixel *pix, int stride, int alpha, int beta, int8_t *tc0 );
void x264_deblock_v_luma_intra_sse2( pixel *pix, int stride, int alpha, int beta );
void x264_deblock_v_luma_intra_avx ( pixel *pix, int stride, int alpha, int beta );
void x264_deblock_h_luma_intra_sse2( pixel *pix, int stride, int alpha, int beta );
void x264_deblock_h_luma_intra_avx ( pixel *pix, int stride, int alpha, int beta );
void x264_deblock_v_chroma_intra_sse2( pixel *pix, int stride, int alpha, int beta );
void x264_deblock_v_chroma_intra_avx ( pixel *pix, int stride, int alpha, int beta );
void x264_deblock_h_chroma_intra_sse2( pixel *pix, int stride, int alpha, int beta );
void x264_deblock_h_chroma_intra_avx ( pixel *pix, int stride, int alpha, int beta );
void x264_deblock_strength_mmxext( uint8_t nnz[X264_SCAN8_SIZE], int8_t ref[2][X264_SCAN8_LUMA_SIZE],
                                   int16_t mv[2][X264_SCAN8_LUMA_SIZE][2], uint8_t bs[2][8][4],
                                   int mvy_limit, int bframe, x264_t *h );
void x264_deblock_strength_sse2  ( uint8_t nnz[X264_SCAN8_SIZE], int8_t ref[2][X264_SCAN8_LUMA_SIZE],
                                   int16_t mv[2][X264_SCAN8_LUMA_SIZE][2], uint8_t bs[2][8][4],
                                   int mvy_limit, int bframe, x264_t *h );
void x264_deblock_strength_ssse3 ( uint8_t nnz[X264_SCAN8_SIZE], int8_t ref[2][X264_SCAN8_LUMA_SIZE],
                                   int16_t mv[2][X264_SCAN8_LUMA_SIZE][2], uint8_t bs[2][8][4],
                                   int mvy_limit, int bframe, x264_t *h );
void x264_deblock_strength_avx   ( uint8_t nnz[X264_SCAN8_SIZE], int8_t ref[2][X264_SCAN8_LUMA_SIZE],
                                   int16_t mv[2][X264_SCAN8_LUMA_SIZE][2], uint8_t bs[2][8][4],
                                   int mvy_limit, int bframe, x264_t *h );
#if ARCH_X86
void x264_deblock_h_luma_mmxext( pixel *pix, int stride, int alpha, int beta, int8_t *tc0 );
void x264_deblock_v8_luma_mmxext( uint8_t *pix, int stride, int alpha, int beta, int8_t *tc0 );
void x264_deblock_v_chroma_mmxext( pixel *pix, int stride, int alpha, int beta, int8_t *tc0 );
void x264_deblock_h_chroma_mmxext( pixel *pix, int stride, int alpha, int beta, int8_t *tc0 );
void x264_deblock_h_luma_intra_mmxext( pixel *pix, int stride, int alpha, int beta );
void x264_deblock_v8_luma_intra_mmxext( uint8_t *pix, int stride, int alpha, int beta );
void x264_deblock_v_chroma_intra_mmxext( pixel *pix, int stride, int alpha, int beta );
void x264_deblock_h_chroma_intra_mmxext( pixel *pix, int stride, int alpha, int beta );

#if HIGH_BIT_DEPTH
void x264_deblock_v_luma_mmxext( pixel *pix, int stride, int alpha, int beta, int8_t *tc0 );
void x264_deblock_v_luma_intra_mmxext( pixel *pix, int stride, int alpha, int beta );
#else
// FIXME this wrapper has a significant cpu cost
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
#endif // HIGH_BIT_DEPTH
#endif
#endif

#if ARCH_PPC
void x264_deblock_v_luma_altivec( uint8_t *pix, int stride, int alpha, int beta, int8_t *tc0 );
void x264_deblock_h_luma_altivec( uint8_t *pix, int stride, int alpha, int beta, int8_t *tc0 );
#endif // ARCH_PPC

#if HAVE_ARMV6
void x264_deblock_v_luma_neon( uint8_t *, int, int, int, int8_t * );
void x264_deblock_h_luma_neon( uint8_t *, int, int, int, int8_t * );
void x264_deblock_v_chroma_neon( uint8_t *, int, int, int, int8_t * );
void x264_deblock_h_chroma_neon( uint8_t *, int, int, int, int8_t * );
#endif

void x264_deblock_init( int cpu, x264_deblock_function_t *pf, int b_mbaff )
{
    pf->deblock_luma[1] = deblock_v_luma_c;
    pf->deblock_luma[0] = deblock_h_luma_c;
    pf->deblock_chroma[1] = deblock_v_chroma_c;
    pf->deblock_chroma[0] = deblock_h_chroma_c;
    pf->deblock_luma_intra[1] = deblock_v_luma_intra_c;
    pf->deblock_luma_intra[0] = deblock_h_luma_intra_c;
    pf->deblock_chroma_intra[1] = deblock_v_chroma_intra_c;
    pf->deblock_chroma_intra[0] = deblock_h_chroma_intra_c;
    pf->deblock_strength = deblock_strength_c;

#if HAVE_MMX
    if( cpu&X264_CPU_MMXEXT )
    {
#if ARCH_X86
        pf->deblock_luma[1] = x264_deblock_v_luma_mmxext;
        pf->deblock_luma[0] = x264_deblock_h_luma_mmxext;
        pf->deblock_chroma[1] = x264_deblock_v_chroma_mmxext;
        pf->deblock_chroma[0] = x264_deblock_h_chroma_mmxext;
        pf->deblock_luma_intra[1] = x264_deblock_v_luma_intra_mmxext;
        pf->deblock_luma_intra[0] = x264_deblock_h_luma_intra_mmxext;
        pf->deblock_chroma_intra[1] = x264_deblock_v_chroma_intra_mmxext;
        pf->deblock_chroma_intra[0] = x264_deblock_h_chroma_intra_mmxext;
#endif
        pf->deblock_strength = x264_deblock_strength_mmxext;
        if( cpu&X264_CPU_SSE2 )
        {
            pf->deblock_strength = x264_deblock_strength_sse2;
            if( !(cpu&X264_CPU_STACK_MOD4) )
            {
                pf->deblock_luma[1] = x264_deblock_v_luma_sse2;
                pf->deblock_luma[0] = x264_deblock_h_luma_sse2;
                pf->deblock_chroma[1] = x264_deblock_v_chroma_sse2;
                pf->deblock_chroma[0] = x264_deblock_h_chroma_sse2;
                pf->deblock_luma_intra[1] = x264_deblock_v_luma_intra_sse2;
                pf->deblock_luma_intra[0] = x264_deblock_h_luma_intra_sse2;
                pf->deblock_chroma_intra[1] = x264_deblock_v_chroma_intra_sse2;
                pf->deblock_chroma_intra[0] = x264_deblock_h_chroma_intra_sse2;
            }
        }
        if( cpu&X264_CPU_SSSE3 )
            pf->deblock_strength = x264_deblock_strength_ssse3;
        if( cpu&X264_CPU_AVX )
        {
            pf->deblock_strength = x264_deblock_strength_avx;
            if( !(cpu&X264_CPU_STACK_MOD4) )
            {
                pf->deblock_luma[1] = x264_deblock_v_luma_avx;
                pf->deblock_luma[0] = x264_deblock_h_luma_avx;
                pf->deblock_chroma[1] = x264_deblock_v_chroma_avx;
                pf->deblock_chroma[0] = x264_deblock_h_chroma_avx;
                pf->deblock_luma_intra[1] = x264_deblock_v_luma_intra_avx;
                pf->deblock_luma_intra[0] = x264_deblock_h_luma_intra_avx;
                pf->deblock_chroma_intra[1] = x264_deblock_v_chroma_intra_avx;
                pf->deblock_chroma_intra[0] = x264_deblock_h_chroma_intra_avx;
            }
        }
    }
#endif

#if !HIGH_BIT_DEPTH
#if HAVE_ALTIVEC
    if( cpu&X264_CPU_ALTIVEC )
    {
        pf->deblock_luma[1] = x264_deblock_v_luma_altivec;
        pf->deblock_luma[0] = x264_deblock_h_luma_altivec;
   }
#endif // HAVE_ALTIVEC

#if HAVE_ARMV6
   if( cpu&X264_CPU_NEON )
   {
        pf->deblock_luma[1] = x264_deblock_v_luma_neon;
        pf->deblock_luma[0] = x264_deblock_h_luma_neon;
//      pf->deblock_chroma[1] = x264_deblock_v_chroma_neon;
//      pf->deblock_chroma[0] = x264_deblock_h_chroma_neon;
   }
#endif
#endif // !HIGH_BIT_DEPTH

    if( b_mbaff ) pf->deblock_strength = deblock_strength_mbaff_c;
}

/*****************************************************************************
 * macroblock.c: h264 encoder library
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar
 * $Id: macroblock-dz.c,v 1.1 2004/06/03 19:27:08 fenrir Exp $
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
#include <stdint.h>

#include "../common/common.h"
#include "../common/vlc.h"
#include "macroblock.h"

static const uint8_t intra4x4_cbp_to_golomb[48]=
{
  3, 29, 30, 17, 31, 18, 37,  8, 32, 38, 19,  9, 20, 10, 11,  2,
 16, 33, 34, 21, 35, 22, 39,  4, 36, 40, 23,  5, 24,  6,  7,  1,
 41, 42, 43, 25, 44, 26, 46, 12, 45, 47, 27, 13, 28, 14, 15,  0
};
static const uint8_t inter_cbp_to_golomb[48]=
{
  0,  2,  3,  7,  4,  8, 17, 13,  5, 18,  9, 14, 10, 15, 16, 11,
  1, 32, 33, 36, 34, 37, 44, 40, 35, 45, 38, 41, 39, 42, 43, 19,
  6, 24, 25, 20, 26, 21, 46, 28, 27, 47, 22, 29, 23, 30, 31, 12
};

static const uint8_t block_idx_x[16] =
{
    0, 1, 0, 1, 2, 3, 2, 3, 0, 1, 0, 1, 2, 3, 2, 3
};
static const uint8_t block_idx_y[16] =
{
    0, 0, 1, 1, 0, 0, 1, 1, 2, 2, 3, 3, 2, 2, 3, 3
};
static const uint8_t block_idx_xy[4][4] =
{
    { 0, 2, 8,  10},
    { 1, 3, 9,  11},
    { 4, 6, 12, 14},
    { 5, 7, 13, 15}
};

static const int quant_mf[6][4][4] =
{
    {  { 13107, 8066, 13107, 8066}, {  8066, 5243,  8066, 5243},
       { 13107, 8066, 13107, 8066}, {  8066, 5243,  8066, 5243}  },
    {  { 11916, 7490, 11916, 7490}, {  7490, 4660,  7490, 4660},
       { 11916, 7490, 11916, 7490}, {  7490, 4660,  7490, 4660}  },
    {  { 10082, 6554, 10082, 6554}, {  6554, 4194,  6554, 4194},
       { 10082, 6554, 10082, 6554}, {  6554, 4194,  6554, 4194}  },
    {  {  9362, 5825,  9362, 5825}, {  5825, 3647,  5825, 3647},
       {  9362, 5825,  9362, 5825}, {  5825, 3647,  5825, 3647}  },
    {  {  8192, 5243,  8192, 5243}, {  5243, 3355,  5243, 3355},
       {  8192, 5243,  8192, 5243}, {  5243, 3355,  5243, 3355}  },
    {  {  7282, 4559,  7282, 4559}, {  4559, 2893,  4559, 2893},
       {  7282, 4559,  7282, 4559}, {  4559, 2893,  4559, 2893}  }
};

static const int i_chroma_qp_table[52] =
{
     0,  1,  2,  3,  4,  5,  6,  7,  8,  9,
    10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
    20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
    29, 30, 31, 32, 32, 33, 34, 34, 35, 35,
    36, 36, 37, 37, 37, 38, 38, 38, 39, 39,
    39, 39
};

static const int f_deadzone_intra[4][4][2] = /* [num][den] */
{
    { {1,2}, {3,7}, {2,5}, {1,3} },
    { {3,7}, {2,5}, {1,3}, {1,4} },
    { {2,5}, {1,3}, {1,4}, {1,5} },
    { {1,3}, {1,4}, {1,5}, {1,5} }
};
static const int f_deadzone_inter[4][4][2] = /* [num][den] */
{
    { {1,3}, {2,7}, {4,15},{2,9} },
    { {2,7}, {4,15},{2,9}, {1,6} },
    { {4,15},{2,9}, {1,6}, {1,7} },
    { {2,9}, {1,6}, {1,7}, {2,15} }
};

/****************************************************************************
 * Scan and Quant functions
 ****************************************************************************/
static const int scan_zigzag_x[16]={0, 1, 0, 0, 1, 2, 3, 2, 1, 0, 1, 2, 3, 3, 2, 3};
static const int scan_zigzag_y[16]={0, 0, 1, 2, 1, 0, 0, 1, 2, 3, 3, 2, 1, 2, 3, 3};

static inline void scan_zigzag_4x4full( int level[16], int16_t dct[4][4] )
{
    int i;

    for( i = 0; i < 16; i++ )
    {
        level[i] = dct[scan_zigzag_y[i]][scan_zigzag_x[i]];
    }
}
static inline void scan_zigzag_4x4( int level[15], int16_t dct[4][4] )
{
    int i;

    for( i = 1; i < 16; i++ )
    {
        level[i - 1] = dct[scan_zigzag_y[i]][scan_zigzag_x[i]];
    }
}

static inline void scan_zigzag_2x2_dc( int level[4], int16_t dct[2][2] )
{
    level[0] = dct[0][0];
    level[1] = dct[0][1];
    level[2] = dct[1][0];
    level[3] = dct[1][1];
}

#if 0
static void quant_4x4( int16_t dct[4][4], int i_qscale, int b_intra )
{
    const int i_qbits = 15 + i_qscale / 6;
    const int i_mf = i_qscale % 6;
    const int f = ( 1 << i_qbits ) / ( b_intra ? 3 : 6 );

    int x,y;
    for( y = 0; y < 4; y++ )
    {
        for( x = 0; x < 4; x++ )
        {
            if( dct[y][x] > 0 )
            {
                dct[y][x] =( f + dct[y][x]  * quant_mf[i_mf][y][x] ) >> i_qbits;
            }
            else
            {
                dct[y][x] = - ( ( f - dct[y][x]  * quant_mf[i_mf][y][x] ) >> i_qbits );
            }
        }
    }
}
static void quant_4x4_dc( int16_t dct[4][4], int i_qscale )
{
    const int i_qbits = 15 + i_qscale / 6;
    const int f2 = ( 2 << i_qbits ) / 3;
    const int i_qmf = quant_mf[i_qscale%6][0][0];
    int x,y;

    for( y = 0; y < 4; y++ )
    {
        for( x = 0; x < 4; x++ )
        {
            if( dct[y][x] > 0 )
            {
                dct[y][x] =( f2 + dct[y][x]  * i_qmf) >> ( 1 + i_qbits );
            }
            else
            {
                dct[y][x] = - ( ( f2 - dct[y][x]  * i_qmf ) >> (1 + i_qbits ) );
            }
        }
    }
}
static void quant_2x2_dc( int16_t dct[2][2], int i_qscale, int b_intra )
{
    int const i_qbits = 15 + i_qscale / 6;
    const int f2 = ( 2 << i_qbits ) / ( b_intra ? 3 : 6 );
    const int i_qmf = quant_mf[i_qscale%6][0][0];

    int x,y;
    for( y = 0; y < 2; y++ )
    {
        for( x = 0; x < 2; x++ )
        {
            if( dct[y][x] > 0 )
            {
                dct[y][x] =( f2 + dct[y][x]  * i_qmf) >> ( 1 + i_qbits );
            }
            else
            {
                dct[y][x] = - ( ( f2 - dct[y][x]  * i_qmf ) >> (1 + i_qbits ) );
            }
        }
    }
}
#endif

static void quant_4x4( int16_t dct[4][4], int i_qscale, int b_intra )
{
    const int i_qbits = 15 + i_qscale / 6;
    const int i_mf = i_qscale % 6;

    int x,y;
    for( y = 0; y < 4; y++ )
    {
        for( x = 0; x < 4; x++ )
        {
            const int f = b_intra ?
                          (f_deadzone_intra[y][x][0] * ( 1 << i_qbits ) / f_deadzone_intra[y][x][1])
                          :
                          (f_deadzone_inter[y][x][0] * ( 1 << i_qbits ) / f_deadzone_inter[y][x][1]);

            if( dct[y][x] > 0 )
            {
                dct[y][x] =( f + dct[y][x]  * quant_mf[i_mf][y][x] ) >> i_qbits;
            }
            else
            {
                dct[y][x] = - ( ( f - dct[y][x]  * quant_mf[i_mf][y][x] ) >> i_qbits );
            }
        }
    }
}

static void quant_4x4_dc( int16_t dct[4][4], int i_qscale )
{
    const int i_qbits = 15 + i_qscale / 6;
    const int i_qmf = quant_mf[i_qscale%6][0][0];
    const int f2 = f_deadzone_intra[0][0][0] * ( 2 << i_qbits ) / f_deadzone_intra[0][0][1];
    int x,y;

    for( y = 0; y < 4; y++ )
    {
        for( x = 0; x < 4; x++ )
        {

            if( dct[y][x] > 0 )
            {
                dct[y][x] =( f2 + dct[y][x]  * i_qmf) >> ( 1 + i_qbits );
            }
            else
            {
                dct[y][x] = - ( ( f2 - dct[y][x]  * i_qmf ) >> (1 + i_qbits ) );
            }
        }
    }
}

static void quant_2x2_dc( int16_t dct[2][2], int i_qscale, int b_intra )
{
    int const i_qbits = 15 + i_qscale / 6;
    const int i_qmf = quant_mf[i_qscale%6][0][0];
    const int f2 = b_intra ?
                   (f_deadzone_intra[0][0][0] * ( 2 << i_qbits ) / f_deadzone_intra[0][0][1])
                   :
                   (f_deadzone_inter[0][0][0] * ( 2 << i_qbits ) / f_deadzone_inter[0][0][1]);
    int x,y;
    for( y = 0; y < 2; y++ )
    {
        for( x = 0; x < 2; x++ )
        {
            if( dct[y][x] > 0 )
            {
                dct[y][x] =( f2 + dct[y][x]  * i_qmf) >> ( 1 + i_qbits );
            }
            else
            {
                dct[y][x] = - ( ( f2 - dct[y][x]  * i_qmf ) >> (1 + i_qbits ) );
            }
        }
    }
}

static inline int array_non_zero_count( int *v, int i_count )
{
    int i;
    int i_nz;

    for( i = 0, i_nz = 0; i < i_count; i++ )
    {
        if( v[i] )
        {
            i_nz++;
        }
    }
    return i_nz;
}

void x264_mb_partition_mvd( x264_macroblock_t *mb, int i_list, int i_part, int i_sub, int mvd[2])
{
    int mvp[2];

    int x,  y;
    int w,  h;
    int dx, dy;

    x264_mb_partition_getxy( mb, i_part, i_sub, &x, &y );
    x264_mb_partition_size ( mb, i_part, i_sub, &w, &h );
    x264_mb_predict_mv(  mb, i_list, i_part, i_sub, mvp );

    mvd[0] = mb->partition[x][y].mv[i_list][0] - mvp[0];
    mvd[1] = mb->partition[x][y].mv[i_list][1] - mvp[1];

    for( dx = 0; dx < w; dx++ )
    {
        for( dy = 0; dy < h; dy++ )
        {
            mb->partition[x+dx][y+dy].mvd[i_list][0] = mvd[0];
            mb->partition[x+dx][y+dy].mvd[i_list][1] = mvd[1];
        }
    }
}

/* (ref: JVT-B118)
 * x264_mb_decimate_score: given dct coeffs it returns a score to see if we could empty this dct coeffs
 * to 0 (low score means set it to null)
 * Used in inter macroblock (luma and chroma)
 *  luma: for a 8x8 block: if score < 4 -> null
 *        for the complete mb: if score < 6 -> null
 *  chroma: for the complete mb: if score < 7 -> null
 */
static int x264_mb_decimate_score( int *dct, int i_max )
{
    static const int i_ds_table[16] = { 3, 2, 2, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

    int i_score = 0;
    int idx = i_max - 1;

    while( idx >= 0 && dct[idx] == 0 )
    {
        idx--;
    }

    while( idx >= 0 )
    {
        int i_run;

        if( abs( dct[idx--] ) > 1 )
        {
            return 9;
        }

        i_run = 0;
        while( idx >= 0 && dct[idx] == 0 )
        {
            idx--;
            i_run++;
        }
        i_score += i_ds_table[i_run];
    }

    return i_score;
}

void x264_mb_encode_i4x4( x264_t *h, x264_macroblock_t *mb, int idx, int i_qscale )
{
    x264_mb_context_t *ctx = mb->context;

    uint8_t *p_src = ctx->p_img[0] + 4 * block_idx_x[idx] + 4 * block_idx_y[idx] * ctx->i_img[0];
    int      i_src = ctx->i_img[0];
    uint8_t *p_dst = ctx->p_fdec[0] + 4 * block_idx_x[idx] + 4 * block_idx_y[idx] * ctx->i_fdec[0];
    int      i_dst = ctx->i_fdec[0];

    int16_t luma[4][4];
    int16_t dct4x4[4][4];

    /* we calculate diff */
    h->pixf.sub4x4( luma, p_src, i_src, p_dst, i_dst );

    /* calculate dct coeffs */
    h->dctf.dct4x4( dct4x4, luma );
    quant_4x4( dct4x4, i_qscale, 1 );

    scan_zigzag_4x4full( mb->block[idx].luma4x4, dct4x4 );

    /* output samples to fdec */
    x264_mb_dequant_4x4( dct4x4, i_qscale );
    h->dctf.idct4x4( luma, dct4x4 );

    /* put pixel to fdec */
    h->pixf.add4x4( p_dst, i_dst, luma );
}

static void x264_mb_encode_i16x16( x264_t *h, x264_macroblock_t *mb, int i_qscale )
{
    x264_mb_context_t *ctx = mb->context;

    uint8_t *p_src = ctx->p_img[0];
    int      i_src = ctx->i_img[0];
    uint8_t *p_dst = ctx->p_fdec[0];
    int      i_dst = ctx->i_fdec[0];

    int16_t luma[16][4][4];
    int16_t dct4x4[16+1][4][4];

    int i;

    /* calculate the diff */
    h->pixf.sub16x16( luma, p_src, i_src, p_dst, i_dst );

    /* calculate dct coeffs */
    for( i = 0; i < 16; i++ )
    {
        h->dctf.dct4x4( dct4x4[i+1], luma[i] );

        /* copy dc coeff */
        dct4x4[0][block_idx_y[i]][block_idx_x[i]] = dct4x4[1+i][0][0];

        quant_4x4( dct4x4[1+i], i_qscale, 1 );
        scan_zigzag_4x4( mb->block[i].residual_ac, dct4x4[1+i] );
    }

    h->dctf.dct4x4dc( dct4x4[0], dct4x4[0] );
    quant_4x4_dc( dct4x4[0], i_qscale );
    scan_zigzag_4x4full( mb->luma16x16_dc, dct4x4[0] );

    /* output samples to fdec */
    h->dctf.idct4x4dc( dct4x4[0], dct4x4[0] );
    x264_mb_dequant_4x4_dc( dct4x4[0], i_qscale );  /* XXX not inversed */

    /* calculate dct coeffs */
    for( i = 0; i < 16; i++ )
    {
        x264_mb_dequant_4x4( dct4x4[1+i], i_qscale );

        /* copy dc coeff */
        dct4x4[1+i][0][0] = dct4x4[0][block_idx_y[i]][block_idx_x[i]];

        h->dctf.idct4x4( luma[i], dct4x4[i+1] );
    }
    /* put pixels to fdec */
    h->pixf.add16x16( p_dst, i_dst, luma );
}

static void x264_mb_encode_8x8( x264_t *h, x264_macroblock_t *mb, int b_inter, int i_qscale )
{
    x264_mb_context_t *ctx = mb->context;

    uint8_t *p_src, *p_dst;
    int      i_src, i_dst;

    int i, ch;
    int i_decimate_score = 0;

    for( ch = 0; ch < 2; ch++ )
    {
        int16_t chroma[4][4][4];
        int16_t dct2x2[2][2];
        int16_t dct4x4[4][4][4];

        p_src = ctx->p_img[1+ch];
        i_src = ctx->i_img[1+ch];
        p_dst = ctx->p_fdec[1+ch];
        i_dst = ctx->i_fdec[1+ch];

        /* calculate the diff */
        h->pixf.sub8x8( chroma, p_src, i_src, p_dst, i_dst );

        /* calculate dct coeffs */
        for( i = 0; i < 4; i++ )
        {
            h->dctf.dct4x4( dct4x4[i], chroma[i] );

            /* copy dc coeff */
            dct2x2[block_idx_y[i]][block_idx_x[i]] = dct4x4[i][0][0];

            quant_4x4( dct4x4[i], i_qscale, b_inter ? 0 : 1 );
            scan_zigzag_4x4( mb->block[16+i+ch*4].residual_ac, dct4x4[i] );

            i_decimate_score += x264_mb_decimate_score( mb->block[16+i+ch*4].residual_ac, 15 );
        }

        h->dctf.dct2x2dc( dct2x2, dct2x2 );
        quant_2x2_dc( dct2x2, i_qscale, b_inter ? 0 : 1  );
        scan_zigzag_2x2_dc( mb->chroma_dc[ch], dct2x2 );

        if( i_decimate_score < 7 && b_inter )
        {
            /* Near null chroma 8x8 block so make it null (bits saving) */
            for( i = 0; i < 4; i++ )
            {
                int x, y;
                for( x = 0; x < 15; x++ )
                {
                    mb->block[16+i+ch*4].residual_ac[x] = 0;
                }
                for( x = 0; x < 4; x++ )
                {
                    for( y = 0; y < 4; y++ )
                    {
                        dct4x4[i][x][y] = 0;
                    }
                }
            }
        }

        /* output samples to fdec */
        h->dctf.idct2x2dc( dct2x2, dct2x2 );
        x264_mb_dequant_2x2_dc( dct2x2, i_qscale );  /* XXX not inversed */

        /* calculate dct coeffs */
        for( i = 0; i < 4; i++ )
        {
            x264_mb_dequant_4x4( dct4x4[i], i_qscale );

            /* copy dc coeff */
            dct4x4[i][0][0] = dct2x2[block_idx_y[i]][block_idx_x[i]];

            h->dctf.idct4x4( chroma[i], dct4x4[i] );
        }
        h->pixf.add8x8( p_dst, i_dst, chroma );
    }
}

/*****************************************************************************
 * x264_macroblock_encode:
 *****************************************************************************/
void x264_macroblock_encode( x264_t *h, x264_macroblock_t *mb )
{
    x264_mb_context_t *ctx = mb->context;
    int i;

    int     i_qscale;

    /* quantification scale */
    i_qscale = mb->i_qp;

    if( mb->i_type == I_16x16 )
    {
        /* do the right prediction */
        h->predict_16x16[mb->i_intra16x16_pred_mode]( ctx->p_fdec[0], ctx->i_fdec[0] );

        /* encode the 16x16 macroblock */
        x264_mb_encode_i16x16( h, mb, i_qscale );

        /* fix the pred mode value */
        mb->i_intra16x16_pred_mode = x264_mb_pred_mode16x16_fix[mb->i_intra16x16_pred_mode];
    }
    else if( mb->i_type == I_4x4 )
    {
        for( i = 0; i < 16; i++ )
        {
            uint8_t *p_dst_by;

            /* Do the right prediction */
            p_dst_by = ctx->p_fdec[0] + 4 * block_idx_x[i] + 4 * block_idx_y[i] * ctx->i_fdec[0];
            h->predict_4x4[mb->block[i].i_intra4x4_pred_mode]( p_dst_by, ctx->i_fdec[0] );

            /* encode one 4x4 block */
            x264_mb_encode_i4x4( h, mb, i, i_qscale );

            /* fix the pred mode value */
            mb->block[i].i_intra4x4_pred_mode = x264_mb_pred_mode4x4_fix[mb->block[i].i_intra4x4_pred_mode];
        }
    }
    else    /* Inter MB */
    {
        int16_t dct4x4[16][4][4];

        int i8x8, i4x4, idx;
        int i_decimate_mb = 0;

        /* Motion compensation */
        x264_mb_mc( h, mb );

        for( i8x8 = 0; i8x8 < 4; i8x8++ )
        {
            int16_t luma[4][4];
            int i_decimate_8x8;

            /* encode one 4x4 block */
            i_decimate_8x8 = 0;
            for( i4x4 = 0; i4x4 < 4; i4x4++ )
            {
                uint8_t *p_src, *p_dst;

                idx = i8x8 * 4 + i4x4;

                p_src = ctx->p_img[0] + 4 * block_idx_x[idx] + 4 * block_idx_y[idx] * ctx->i_img[0];
                p_dst = ctx->p_fdec[0] + 4 * block_idx_x[idx] + 4 * block_idx_y[idx] * ctx->i_fdec[0];

                /* we calculate diff */
                h->pixf.sub4x4( luma, p_src, ctx->i_img[0],p_dst, ctx->i_fdec[0] );

                /* calculate dct coeffs */
                h->dctf.dct4x4( dct4x4[idx], luma );
                quant_4x4( dct4x4[idx], i_qscale, 0 );

                scan_zigzag_4x4full( mb->block[idx].luma4x4, dct4x4[idx] );
                i_decimate_8x8 += x264_mb_decimate_score( mb->block[idx].luma4x4, 16 );
            }

            /* decimate this 8x8 block */
            i_decimate_mb += i_decimate_8x8;
            if( i_decimate_8x8 < 4 )
            {
                for( i4x4 = 0; i4x4 < 4; i4x4++ )
                {
                    int x, y;
                    idx = i8x8 * 4 + i4x4;
                    for( i = 0; i < 16; i++ )
                    {
                        mb->block[idx].luma4x4[i] = 0;
                    }
                    for( x = 0; x < 4; x++ )
                    {
                        for( y = 0; y < 4; y++ )
                        {
                            dct4x4[idx][x][y] = 0;
                        }
                    }
                }
            }
        }

        if( i_decimate_mb < 6 )
        {
            for( i8x8 = 0; i8x8 < 4; i8x8++ )
            {
                for( i4x4 = 0; i4x4 < 4; i4x4++ )
                {
                    for( i = 0; i < 16; i++ )
                    {
                        mb->block[i8x8 * 4 + i4x4].luma4x4[i] = 0;
                    }
                }
            }
        }
        else
        {
            for( i8x8 = 0; i8x8 < 4; i8x8++ )
            {
                int16_t luma[4][4];
                /* TODO we could avoid it if we had decimate this 8x8 block */
                /* output samples to fdec */
                for( i4x4 = 0; i4x4 < 4; i4x4++ )
                {
                    uint8_t *p_dst;

                    idx = i8x8 * 4 + i4x4;

                    x264_mb_dequant_4x4( dct4x4[idx], i_qscale );
                    h->dctf.idct4x4( luma, dct4x4[idx] );

                    /* put pixel to fdec */
                    p_dst = ctx->p_fdec[0] + 4 * block_idx_x[idx] + 4 * block_idx_y[idx] * ctx->i_fdec[0];
                    h->pixf.add4x4( p_dst, ctx->i_fdec[0], luma );
                }
            }
        }
    }

    /* encode chroma */
    i_qscale = i_chroma_qp_table[x264_clip3( i_qscale + h->pps->i_chroma_qp_index_offset, 0, 51 )];
    if( IS_INTRA( mb->i_type ) )
    {
        /* do the right prediction */
        h->predict_8x8[mb->i_chroma_pred_mode]( ctx->p_fdec[1], ctx->i_fdec[1] );
        h->predict_8x8[mb->i_chroma_pred_mode]( ctx->p_fdec[2], ctx->i_fdec[2] );
    }

    /* encode the 8x8 blocks */
    x264_mb_encode_8x8( h, mb, !IS_INTRA( mb->i_type ), i_qscale );

    /* fix the pred mode value */
    if( IS_INTRA( mb->i_type ) )
    {
        mb->i_chroma_pred_mode = x264_mb_pred_mode8x8_fix[mb->i_chroma_pred_mode];
    }

    /* Calculate the Luma/Chroma patern and non_zero_count */
    if( mb->i_type == I_16x16 )
    {
        mb->i_cbp_luma = 0x00;
        for( i = 0; i < 16; i++ )
        {
            mb->block[i].i_non_zero_count = array_non_zero_count( mb->block[i].residual_ac, 15 );
            if( mb->block[i].i_non_zero_count > 0 )
            {
                mb->i_cbp_luma = 0x0f;
            }
        }
    }
    else
    {
        mb->i_cbp_luma = 0x00;
        for( i = 0; i < 16; i++ )
        {
            mb->block[i].i_non_zero_count = array_non_zero_count( mb->block[i].luma4x4, 16 );
            if( mb->block[i].i_non_zero_count > 0 )
            {
                mb->i_cbp_luma |= 1 << (i/4);
            }
        }
    }

    /* Calculate the chroma patern */
    mb->i_cbp_chroma = 0x00;
    for( i = 0; i < 8; i++ )
    {
        mb->block[16+i].i_non_zero_count = array_non_zero_count( mb->block[16+i].residual_ac, 15 );
        if( mb->block[16+i].i_non_zero_count > 0 )
        {
            mb->i_cbp_chroma = 0x02;    /* dc+ac (we can't do only ac) */
        }
    }
    if( mb->i_cbp_chroma == 0x00 &&
        ( array_non_zero_count( mb->chroma_dc[0], 4 ) > 0 || array_non_zero_count( mb->chroma_dc[1], 4 ) ) > 0 )
    {
        mb->i_cbp_chroma = 0x01;    /* dc only */
    }

    /* Check for P_SKIP
     * XXX: in the me perhaps we should take x264_mb_predict_mv_pskip into account
     *      (if multiple mv give same result)*/
    if( mb->i_type == P_L0 && mb->i_partition == D_16x16 &&
        mb->i_cbp_luma == 0x00 && mb->i_cbp_chroma == 0x00 )
    {

        int i_ref;
        int mvx, mvy;
        x264_mb_partition_get( mb, 0, 0, 0, &i_ref, &mvx, &mvy );

        if( i_ref == 0 )
        {
            int mvp[2];

            x264_mb_predict_mv_pskip( mb, mvp );
            if( mvp[0] == mvx && mvp[1] == mvy )
            {
                mb->i_type = P_SKIP;
            }
        }
    }
}


#define BLOCK_INDEX_CHROMA_DC   (-1)
#define BLOCK_INDEX_LUMA_DC     (-2)

static inline void bs_write_vlc( bs_t *s, vlc_t v )
{
    bs_write( s, v.i_size, v.i_bits );
}

/****************************************************************************
 * block_residual_write_cavlc:
 ****************************************************************************/
static void block_residual_write_cavlc( x264_t *h, bs_t *s, x264_macroblock_t *mb, int i_idx, int *l, int i_count )
{
    int level[16], run[16];
    int i_total, i_trailing;
    int i_total_zero;
    int i_last;
    unsigned int i_sign;

    int i;
    int i_zero_left;
    int i_suffix_length;

    /* first find i_last */
    i_last = i_count - 1;
    while( i_last >= 0 && l[i_last] == 0 )
    {
        i_last--;
    }

    i_sign = 0;
    i_total = 0;
    i_trailing = 0;
    i_total_zero = 0;

    if( i_last >= 0 )
    {
        int b_trailing = 1;
        int idx = 0;

        /* level and run and total */
        while( i_last >= 0 )
        {
            level[idx] = l[i_last--];

            run[idx] = 0;
            while( i_last >= 0 && l[i_last] == 0 )
            {
                run[idx]++;
                i_last--;
            }

            i_total++;
            i_total_zero += run[idx];

            if( b_trailing && abs( level[idx] ) == 1 && i_trailing < 3 )
            {
                i_sign <<= 1;
                if( level[idx] < 0 )
                {
                    i_sign |= 0x01;
                }

                i_trailing++;
            }
            else
            {
                b_trailing = 0;
            }

            idx++;
        }
    }

    /* total/trailing */
    if( i_idx == BLOCK_INDEX_CHROMA_DC )
    {
        bs_write_vlc( s, x264_coeff_token[4][i_total*4+i_trailing] );
    }
    else
    {
        /* x264_mb_predict_non_zero_code return 0 <-> (16+16+1)>>1 = 16 */
        static const int ct_index[17] = {0,0,1,1,2,2,2,2,3,3,3,3,3,3,3,3,3 };
        int nC;

        if( i_idx == BLOCK_INDEX_LUMA_DC )
        {
            nC = x264_mb_predict_non_zero_code( h, mb, 0 );
        }
        else
        {
            nC = x264_mb_predict_non_zero_code( h, mb, i_idx );
        }

        bs_write_vlc( s, x264_coeff_token[ct_index[nC]][i_total*4+i_trailing] );
    }

    if( i_total <= 0 )
    {
        return;
    }

    i_suffix_length = i_total > 10 && i_trailing < 3 ? 1 : 0;
    if( i_trailing > 0 )
    {
        bs_write( s, i_trailing, i_sign );
    }
    for( i = i_trailing; i < i_total; i++ )
    {
        int i_level_code;

        /* calculate level code */
        if( level[i] < 0 )
        {
            i_level_code = -2*level[i] - 1;
        }
        else /* if( level[i] > 0 ) */
        {
            i_level_code = 2 * level[i] - 2;
        }
        if( i == i_trailing && i_trailing < 3 )
        {
            i_level_code -=2; /* as level[i] can't be 1 for the first one if i_trailing < 3 */
        }

        if( ( i_level_code >> i_suffix_length ) < 14 )
        {
            bs_write_vlc( s, x264_level_prefix[i_level_code >> i_suffix_length] );
            if( i_suffix_length > 0 )
            {
                bs_write( s, i_suffix_length, i_level_code );
            }
        }
        else if( i_suffix_length == 0 && i_level_code < 30 )
        {
            bs_write_vlc( s, x264_level_prefix[14] );
            bs_write( s, 4, i_level_code - 14 );
        }
        else if( i_suffix_length > 0 && ( i_level_code >> i_suffix_length ) == 14 )
        {
            bs_write_vlc( s, x264_level_prefix[14] );
            bs_write( s, i_suffix_length, i_level_code );
        }
        else
        {
            bs_write_vlc( s, x264_level_prefix[15] );
            i_level_code -= 15 << i_suffix_length;
            if( i_suffix_length == 0 )
            {
                i_level_code -= 15;
            }

            if( i_level_code >= ( 1 << 12 ) || i_level_code < 0 )
            {
                fprintf( stderr, "OVERFLOW levelcode=%d\n", i_level_code );
            }

            bs_write( s, 12, i_level_code );    /* check overflow ?? */
        }

        if( i_suffix_length == 0 )
        {
            i_suffix_length++;
        }
        if( abs( level[i] ) > ( 3 << ( i_suffix_length - 1 ) ) && i_suffix_length < 6 )
        {
            i_suffix_length++;
        }
    }

    if( i_total < i_count )
    {
        if( i_idx == BLOCK_INDEX_CHROMA_DC )
        {
            bs_write_vlc( s, x264_total_zeros_dc[i_total-1][i_total_zero] );
        }
        else
        {
            bs_write_vlc( s, x264_total_zeros[i_total-1][i_total_zero] );
        }
    }

    for( i = 0, i_zero_left = i_total_zero; i < i_total - 1; i++ )
    {
        int i_zl;

        if( i_zero_left <= 0 )
        {
            break;
        }

        i_zl = X264_MIN( i_zero_left - 1, 6 );

        bs_write_vlc( s, x264_run_before[i_zl][run[i]] );

        i_zero_left -= run[i];
    }
}

/*****************************************************************************
 * x264_macroblock_write:
 *****************************************************************************/
void x264_macroblock_write_cavlc( x264_t *h, bs_t *s, x264_macroblock_t *mb )
{
    int i_mb_i_offset;
    int i;

    switch( h->sh.i_type )
    {
        case SLICE_TYPE_I:
            i_mb_i_offset = 0;
            break;
        case SLICE_TYPE_P:
            i_mb_i_offset = 5;
            break;
        case SLICE_TYPE_B:
            i_mb_i_offset = 23;
            break;
        default:
            fprintf( stderr, "internal error or slice unsupported\n" );
            return;
    }

    /* Write:
      - type
      - prediction
      - mv */
    if( mb->i_type == I_PCM )
    {
        /* Untested */
        bs_write_ue( s, i_mb_i_offset + 25 );

        bs_align_0( s );
        /* Luma */
        for( i = 0; i < 16*16; i++ )
        {
            bs_write( s, 8, h->picture->plane[0][mb->i_mb_y * 16 * h->picture->i_stride[0] + mb->i_mb_x * 16+i] );
        }
        /* Cb */
        for( i = 0; i < 8*8; i++ )
        {
            bs_write( s, 8, h->picture->plane[1][mb->i_mb_y * 8 * h->picture->i_stride[1] + mb->i_mb_x * 8+i] );
        }
        /* Cr */
        for( i = 0; i < 8*8; i++ )
        {
            bs_write( s, 8, h->picture->plane[2][mb->i_mb_y * 8 * h->picture->i_stride[2] + mb->i_mb_x * 8+i] );
        }

        for( i = 0; i < 16 + 8; i++ )
        {
            /* special case */
            mb->block[i].i_non_zero_count = 16;
        }
        return;
    }
    else if( mb->i_type == I_4x4 )
    {
        bs_write_ue( s, i_mb_i_offset + 0 );

        /* Prediction: Luma */
        for( i = 0; i < 16; i++ )
        {
            int i_predicted_mode = x264_mb_predict_intra4x4_mode( h, mb, i );
            int i_mode = mb->block[i].i_intra4x4_pred_mode;

            if( i_predicted_mode == i_mode)
            {
                bs_write1( s, 1 );  /* b_prev_intra4x4_pred_mode */
            }
            else
            {
                bs_write1( s, 0 );  /* b_prev_intra4x4_pred_mode */
                if( i_mode < i_predicted_mode )
                {
                    bs_write( s, 3, i_mode );
                }
                else
                {
                    bs_write( s, 3, i_mode - 1 );
                }
            }
        }
        /* Prediction: chroma */
        bs_write_ue( s, mb->i_chroma_pred_mode );
    }
    else if( mb->i_type == I_16x16 )
    {
        bs_write_ue( s, i_mb_i_offset + 1 + mb->i_intra16x16_pred_mode +
                                            mb->i_cbp_chroma * 4 +
                                            ( mb->i_cbp_luma == 0 ? 0 : 12 ) );
        /* Prediction: chroma */
        bs_write_ue( s, mb->i_chroma_pred_mode );
    }
    else if( mb->i_type == P_L0 )
    {
        int mvp[2];

        if( mb->i_partition == D_16x16 )
        {
            bs_write_ue( s, 0 );

            if( h->sh.i_num_ref_idx_l0_active > 1 )
            {
                bs_write_te( s, h->sh.i_num_ref_idx_l0_active - 1, mb->partition[0][0].i_ref[0] );
            }
            x264_mb_predict_mv( mb, 0, 0, 0, mvp );
            bs_write_se( s, mb->partition[0][0].mv[0][0] - mvp[0] );
            bs_write_se( s, mb->partition[0][0].mv[0][1] - mvp[1] );
        }
        else if( mb->i_partition == D_16x8 )
        {
            bs_write_ue( s, 1 );
            if( h->sh.i_num_ref_idx_l0_active > 1 )
            {
                bs_write_te( s, h->sh.i_num_ref_idx_l0_active - 1, mb->partition[0][0].i_ref[0] );
                bs_write_te( s, h->sh.i_num_ref_idx_l0_active - 1, mb->partition[0][2].i_ref[0] );
            }

            x264_mb_predict_mv( mb, 0, 0, 0, mvp );
            bs_write_se( s, mb->partition[0][0].mv[0][0] - mvp[0] );
            bs_write_se( s, mb->partition[0][0].mv[0][1] - mvp[1] );

            x264_mb_predict_mv( mb, 0, 1, 0, mvp );
            bs_write_se( s, mb->partition[0][2].mv[0][0] - mvp[0] );
            bs_write_se( s, mb->partition[0][2].mv[0][1] - mvp[1] );
        }
        else if( mb->i_partition == D_8x16 )
        {
            bs_write_ue( s, 2 );
            if( h->sh.i_num_ref_idx_l0_active > 1 )
            {
                bs_write_te( s, h->sh.i_num_ref_idx_l0_active - 1, mb->partition[0][0].i_ref[0] );
                bs_write_te( s, h->sh.i_num_ref_idx_l0_active - 1, mb->partition[2][0].i_ref[0] );
            }

            x264_mb_predict_mv( mb, 0, 0, 0, mvp );
            bs_write_se( s, mb->partition[0][0].mv[0][0] - mvp[0] );
            bs_write_se( s, mb->partition[0][0].mv[0][1] - mvp[1] );

            x264_mb_predict_mv( mb, 0, 1, 0, mvp );
            bs_write_se( s, mb->partition[2][0].mv[0][0] - mvp[0] );
            bs_write_se( s, mb->partition[2][0].mv[0][1] - mvp[1] );
        }
    }
    else if( mb->i_type == P_8x8 )
    {
        int b_sub_ref0;

        if( mb->partition[0][0].i_ref[0] == 0 &&
            mb->partition[0][2].i_ref[0] == 0 &&
            mb->partition[2][0].i_ref[0] == 0 &&
            mb->partition[2][2].i_ref[0] == 0 )
        {
            bs_write_ue( s, 4 );
            b_sub_ref0 = 0;
        }
        else
        {
            bs_write_ue( s, 3 );
            b_sub_ref0 = 1;
        }
        /* sub mb type */
        for( i = 0; i < 4; i++ )
        {
            switch( mb->i_sub_partition[i] )
            {
                case D_L0_8x8:
                    bs_write_ue( s, 0 );
                    break;
                case D_L0_8x4:
                    bs_write_ue( s, 1 );
                    break;
                case D_L0_4x8:
                    bs_write_ue( s, 2 );
                    break;
                case D_L0_4x4:
                    bs_write_ue( s, 3 );
                    break;
            }
        }
        /* ref0 */
        if( h->sh.i_num_ref_idx_l0_active > 1 && b_sub_ref0 )
        {
            bs_write_te( s, h->sh.i_num_ref_idx_l0_active - 1, mb->partition[0][0].i_ref[0] );
            bs_write_te( s, h->sh.i_num_ref_idx_l0_active - 1, mb->partition[2][0].i_ref[0] );
            bs_write_te( s, h->sh.i_num_ref_idx_l0_active - 1, mb->partition[0][2].i_ref[0] );
            bs_write_te( s, h->sh.i_num_ref_idx_l0_active - 1, mb->partition[2][2].i_ref[0] );
        }
        for( i = 0; i < 4; i++ )
        {
            int i_part;
            for( i_part = 0; i_part < x264_mb_partition_count_table[mb->i_sub_partition[i]]; i_part++ )
            {
                int mvx, mvy;
                int mvp[2];

                x264_mb_partition_get( mb, 0, i, i_part, NULL, &mvx, &mvy );
                x264_mb_predict_mv( mb, 0, i, i_part, mvp );

                bs_write_se( s, mvx - mvp[0] );
                bs_write_se( s, mvy - mvp[1]);
            }
        }
    }
    else if( mb->i_type == B_8x8 )
    {
        fprintf( stderr, "invalid/unhandled mb_type (B_8x8)\n" );
        return;
    }
    else if( mb->i_type != B_DIRECT )
    {
        /* All B mode */
        /* Motion Vector */
        int i_part = x264_mb_partition_count_table[mb->i_partition];
        int i_ref;
        int mvx, mvy;
        int mvp[2];

        int b_list0[2];
        int b_list1[2];

        /* init ref list utilisations */
        for( i = 0; i < 2; i++ )
        {
            b_list0[i] = x264_mb_type_list0_table[mb->i_type][i];
            b_list1[i] = x264_mb_type_list1_table[mb->i_type][i];
        }


        if( mb->i_partition == D_16x16 )
        {
            if( b_list0[0] && b_list1[0] )
            {
                bs_write_ue( s, 3 );
            }
            else if( b_list1[0] )
            {
                bs_write_ue( s, 2 );
            }
            else
            {
                bs_write_ue( s, 1 );
            }
        }
        else
        {
            if( mb->i_type == B_BI_BI )
            {
                bs_write_ue( s, 20 + (mb->i_partition == D_16x8 ? 0 : 1 ) );
            }
            else if( b_list0[0] && b_list1[0] )
            {
                /* B_BI_LX* */
                bs_write_ue( s, 16 + (b_list0[1]?0:2) + (mb->i_partition == D_16x8?0:1) );
            }
            else if( b_list0[1] && b_list1[1] )
            {
                /* B_LX_BI */
                bs_write_ue( s, 12 + (b_list0[1]?0:2) + (mb->i_partition == D_16x8?0:1) );
            }
            else if( b_list1[1] )
            {
                /* B_LX_L1 */
                bs_write_ue( s, 6 + (b_list0[0]?2:0) + (mb->i_partition == D_16x8?0:1) );
            }
            else if( b_list0[1] )
            {
                /* B_LX_L0 */
                bs_write_ue( s, 4 + (b_list0[0]?0:6) + (mb->i_partition == D_16x8?0:1) );
            }
        }

        if( h->sh.i_num_ref_idx_l0_active > 1 )
        {
            for( i = 0; i < i_part; i++ )
            {
                if( b_list0[i] )
                {
                    x264_mb_partition_get( mb, 0, i, 0, &i_ref, NULL, NULL );
                    bs_write_te( s, h->sh.i_num_ref_idx_l0_active - 1, i_ref );
                }
            }
        }
        if( h->sh.i_num_ref_idx_l1_active > 1 )
        {
            for( i = 0; i < i_part; i++ )
            {
                if( b_list1[i] )
                {
                    x264_mb_partition_get( mb, 1, i, 0, &i_ref, NULL, NULL );
                    bs_write_te( s, h->sh.i_num_ref_idx_l1_active - 1, i_ref );
                }
            }
        }

        for( i = 0; i < i_part; i++ )
        {
            if( b_list0[i] )
            {
                x264_mb_partition_get( mb, 0, i, 0, NULL, &mvx, &mvy );
                x264_mb_predict_mv( mb, 0, i, 0, mvp );

                bs_write_se( s, mvx - mvp[0] );
                bs_write_se( s, mvy - mvp[1] );
            }
        }
        for( i = 0; i < i_part; i++ )
        {
            if( b_list1[i] )
            {
                x264_mb_partition_get( mb, 1, i, 0, NULL, &mvx, &mvy );
                x264_mb_predict_mv( mb, 1, i, 0, mvp );

                bs_write_se( s, mvx - mvp[0] );
                bs_write_se( s, mvy - mvp[1] );
            }
        }
    }
    else if( mb->i_type == B_DIRECT )
    {
        bs_write_ue( s, 0 );
    }
    else
    {
        fprintf( stderr, "invalid/unhandled mb_type\n" );
        return;
    }

    /* Coded block patern */
    if( mb->i_type == I_4x4 )
    {
        bs_write_ue( s, intra4x4_cbp_to_golomb[( mb->i_cbp_chroma << 4 )|mb->i_cbp_luma] );
    }
    else if( mb->i_type != I_16x16 )
    {
        bs_write_ue( s, inter_cbp_to_golomb[( mb->i_cbp_chroma << 4 )|mb->i_cbp_luma] );
    }

    /* write residual */
    if( mb->i_type == I_16x16 )
    {
        if( mb->i_mb_x > 0 || mb->i_mb_y > 0 )
            bs_write_se( s, mb->i_qp - (mb-1)->i_qp);
        else
            bs_write_se( s, mb->i_qp - h->pps->i_pic_init_qp - h->sh.i_qp_delta );

        /* DC Luma */
        block_residual_write_cavlc( h, s, mb, BLOCK_INDEX_LUMA_DC , mb->luma16x16_dc, 16 );

        if( mb->i_cbp_luma != 0 )
        {
            /* AC Luma */
            for( i = 0; i < 16; i++ )
            {
                block_residual_write_cavlc( h, s, mb, i, mb->block[i].residual_ac, 15 );
            }
        }
    }
    else if( mb->i_cbp_luma != 0 || mb->i_cbp_chroma != 0 )
    {
        bs_write_se( s, mb->i_qp - h->pps->i_pic_init_qp - h->sh.i_qp_delta );

        for( i = 0; i < 16; i++ )
        {
            if( mb->i_cbp_luma & ( 1 << ( i / 4 ) ) )
            {
                block_residual_write_cavlc( h, s, mb, i, mb->block[i].luma4x4, 16 );
            }
        }
    }
    if( mb->i_cbp_chroma != 0 )
    {
        /* Chroma DC residual present */
        block_residual_write_cavlc( h, s, mb, BLOCK_INDEX_CHROMA_DC, mb->chroma_dc[0], 4 );
        block_residual_write_cavlc( h, s, mb, BLOCK_INDEX_CHROMA_DC, mb->chroma_dc[1], 4 );
        if( mb->i_cbp_chroma&0x02 ) /* Chroma AC residual present */
        {
            for( i = 0; i < 8; i++ )
            {
                block_residual_write_cavlc( h, s, mb, 16 + i, mb->block[16+i].residual_ac, 15 );
            }
        }
    }
}

/*****************************************************************************
 *
 * Cabac stuff
 *
 *****************************************************************************/

static void x264_cabac_mb_type( x264_t *h, x264_macroblock_t *mb )
{
    x264_macroblock_t *mba = mb->context->mba;
    x264_macroblock_t *mbb = mb->context->mbb;
    int i_ctxIdxInc    = 0;

    if( h->sh.i_type == SLICE_TYPE_I )
    {

        if( mba != NULL && mba->i_type != I_4x4 )
        {
            i_ctxIdxInc++;
        }
        if( mbb != NULL && mbb->i_type != I_4x4 )
        {
            i_ctxIdxInc++;
        }

        if( mb->i_type == I_4x4 )
        {
            x264_cabac_encode_decision( &h->cabac, 3 + i_ctxIdxInc, 0 );
        }
        else if( mb->i_type == I_PCM )
        {
            x264_cabac_encode_decision( &h->cabac, 3 + i_ctxIdxInc, 1 );
            x264_cabac_encode_terminal( &h->cabac, 1 ); /*ctxIdx == 276 */
        }
        else    /* I_16x16 */
        {
            x264_cabac_encode_decision( &h->cabac, 3 + i_ctxIdxInc, 1 );
            x264_cabac_encode_terminal( &h->cabac, 0 ); /*ctxIdx == 276 */

            x264_cabac_encode_decision( &h->cabac, 3 + 3, ( mb->i_cbp_luma == 0 ? 0 : 1 ));
            if( mb->i_cbp_chroma == 0 )
            {
                x264_cabac_encode_decision( &h->cabac, 3 + 4, 0 );
            }
            else
            {
                x264_cabac_encode_decision( &h->cabac, 3 + 4, 1 );
                x264_cabac_encode_decision( &h->cabac, 3 + 5, ( mb->i_cbp_chroma == 1 ? 0 : 1 ) );
            }
            x264_cabac_encode_decision( &h->cabac, 3 + 6, ( (mb->i_intra16x16_pred_mode / 2) ? 1 : 0 ));
            x264_cabac_encode_decision( &h->cabac, 3 + 7, ( (mb->i_intra16x16_pred_mode % 2) ? 1 : 0 ));
        }
    }
    else if( h->sh.i_type == SLICE_TYPE_P )
    {
        /* prefix: 14, suffix: 17 */
        if( mb->i_type == P_L0 )
        {
            if( mb->i_partition == D_16x16 )
            {
                x264_cabac_encode_decision( &h->cabac, 14, 0 );
                x264_cabac_encode_decision( &h->cabac, 15, 0 );
                x264_cabac_encode_decision( &h->cabac, 16, 0 );
            }
            else if( mb->i_partition == D_16x8 )
            {
                x264_cabac_encode_decision( &h->cabac, 14, 0 );
                x264_cabac_encode_decision( &h->cabac, 15, 1 );
                x264_cabac_encode_decision( &h->cabac, 17, 1 );
            }
            else if( mb->i_partition == D_8x16 )
            {
                x264_cabac_encode_decision( &h->cabac, 14, 0 );
                x264_cabac_encode_decision( &h->cabac, 15, 1 );
                x264_cabac_encode_decision( &h->cabac, 17, 0 );
            }
        }
        else if( mb->i_type == P_8x8 )
        {
            x264_cabac_encode_decision( &h->cabac, 14, 0 );
            x264_cabac_encode_decision( &h->cabac, 15, 0 );
            x264_cabac_encode_decision( &h->cabac, 16, 1 );
        }
        else if( mb->i_type == I_4x4 )
        {
            /* prefix */
            x264_cabac_encode_decision( &h->cabac, 14, 1 );

            x264_cabac_encode_decision( &h->cabac, 17, 0 );
        }
        else if( mb->i_type == I_PCM )
        {
            /* prefix */
            x264_cabac_encode_decision( &h->cabac, 14, 1 );

            x264_cabac_encode_decision( &h->cabac, 17, 1 );
            x264_cabac_encode_terminal( &h->cabac, 1 ); /*ctxIdx == 276 */
        }
        else /* intra 16x16 */
        {
            /* prefix */
            x264_cabac_encode_decision( &h->cabac, 14, 1 );

            /* suffix */
            x264_cabac_encode_decision( &h->cabac, 17, 1 );
            x264_cabac_encode_terminal( &h->cabac, 0 ); /*ctxIdx == 276 */

            x264_cabac_encode_decision( &h->cabac, 17+1, ( mb->i_cbp_luma == 0 ? 0 : 1 ));
            if( mb->i_cbp_chroma == 0 )
            {
                x264_cabac_encode_decision( &h->cabac, 17+2, 0 );
            }
            else
            {
                x264_cabac_encode_decision( &h->cabac, 17+2, 1 );
                x264_cabac_encode_decision( &h->cabac, 17+2, ( mb->i_cbp_chroma == 1 ? 0 : 1 ) );
            }
            x264_cabac_encode_decision( &h->cabac, 17+3, ( (mb->i_intra16x16_pred_mode / 2) ? 1 : 0 ));
            x264_cabac_encode_decision( &h->cabac, 17+3, ( (mb->i_intra16x16_pred_mode % 2) ? 1 : 0 ));
        }
    }
    else
    {
        fprintf( stderr, "SLICE_TYPE_B unsupported in x264_macroblock_write_cabac\n" );
        return;
    }
}

static void x264_cabac_mb_intra4x4_pred_mode( x264_t *h, x264_macroblock_t *mb, int i_pred, int i_mode )
{
    if( i_pred == i_mode )
    {
        /* b_prev_intra4x4_pred_mode */
        x264_cabac_encode_decision( &h->cabac, 68, 1 );
    }
    else
    {
        /* b_prev_intra4x4_pred_mode */
        x264_cabac_encode_decision( &h->cabac, 68, 0 );
        if( i_mode > i_pred  )
        {
            i_mode--;
        }
        x264_cabac_encode_decision( &h->cabac, 69, (i_mode     )&0x01 );
        x264_cabac_encode_decision( &h->cabac, 69, (i_mode >> 1)&0x01 );
        x264_cabac_encode_decision( &h->cabac, 69, (i_mode >> 2)&0x01 );
    }
}
static void x264_cabac_mb_intra8x8_pred_mode( x264_t *h, x264_macroblock_t *mb )
{
    x264_macroblock_t *mba = mb->context->mba;
    x264_macroblock_t *mbb = mb->context->mbb;

    int i_ctxIdxInc    = 0;

    if( mba != NULL && ( mba->i_type == I_4x4 || mba->i_type == I_16x16 ) && mba->i_chroma_pred_mode != 0 )
    {
        i_ctxIdxInc++;
    }
    if( mbb != NULL && ( mbb->i_type == I_4x4 || mbb->i_type == I_16x16 ) && mbb->i_chroma_pred_mode != 0 )
    {
        i_ctxIdxInc++;
    }
    if( mb->i_chroma_pred_mode == 0 )
    {
        x264_cabac_encode_decision( &h->cabac, 64 + i_ctxIdxInc, 0 );
    }
    else
    {
        x264_cabac_encode_decision( &h->cabac, 64 + i_ctxIdxInc, 1 );
        x264_cabac_encode_decision( &h->cabac, 64 + 3, ( mb->i_chroma_pred_mode == 1 ? 0 : 1 ) );
        if( mb->i_chroma_pred_mode > 1 )
        {
            x264_cabac_encode_decision( &h->cabac, 64 + 3, ( mb->i_chroma_pred_mode == 2 ? 0 : 1 ) );
        }
    }
}

static void x264_cabac_mb_cbp_luma( x264_t *h, x264_macroblock_t *mb )
{
    int idx;
    x264_macroblock_t *mba;
    x264_macroblock_t *mbb;

    for( idx = 0;idx < 16; idx+=4 )
    {
        int i_ctxIdxInc;
        int i8x8a, i8x8b;
        int x, y;

        mba = mb->context->block[idx].mba;
        mbb = mb->context->block[idx].mbb;

        x = block_idx_x[idx]; y = block_idx_y[idx];

        i8x8a = block_idx_xy[(x-1)&0x03][y]/4;
        i8x8b = block_idx_xy[x][(y-1)&0x03]/4;

        i_ctxIdxInc = 0;
        if( mba != NULL && mba->i_type != I_PCM &&
           ( IS_SKIP( mba->i_type ) || ((mba->i_cbp_luma >> i8x8a)&0x01) == 0 ) )
        {
            i_ctxIdxInc++;
        }
        if( mbb != NULL && mbb->i_type != I_PCM &&
           ( IS_SKIP( mbb->i_type ) || ((mbb->i_cbp_luma >> i8x8b)&0x01) == 0 ) )
        {
            i_ctxIdxInc += 2;
        }
        x264_cabac_encode_decision( &h->cabac, 73 + i_ctxIdxInc, (mb->i_cbp_luma  >> (idx/4))&0x01 );
    }
}

static void x264_cabac_mb_cbp_chroma( x264_t *h, x264_macroblock_t *mb )
{
    x264_macroblock_t *mba = mb->context->mba;
    x264_macroblock_t *mbb = mb->context->mbb;
    int i_ctxIdxInc = 0;

    if( mba != NULL && !IS_SKIP( mba->i_type ) &&
        ( mba->i_type == I_PCM || mba->i_cbp_chroma != 0 ) )
    {
        i_ctxIdxInc++;
    }
    if( mbb != NULL && !IS_SKIP( mbb->i_type ) &&
        ( mbb->i_type == I_PCM || mbb->i_cbp_chroma != 0 ) )
    {
        i_ctxIdxInc += 2;
    }
    x264_cabac_encode_decision( &h->cabac, 77 + i_ctxIdxInc, (mb->i_cbp_chroma > 0 ? 1 : 0) );
    if( mb->i_cbp_chroma > 0 )
    {
        i_ctxIdxInc = 4;
        if( mba != NULL && !IS_SKIP( mba->i_type ) &&
            ( mba->i_type == I_PCM || mba->i_cbp_chroma == 2 ) )
        {
            i_ctxIdxInc++;
        }
        if( mbb != NULL && !IS_SKIP( mbb->i_type ) &&
            ( mbb->i_type == I_PCM || mbb->i_cbp_chroma == 2 ) )
        {
            i_ctxIdxInc += 2;
        }
        x264_cabac_encode_decision( &h->cabac, 77 + i_ctxIdxInc, (mb->i_cbp_chroma > 1 ? 1 : 0) );
    }
}

/* TODO check it with != qp per mb */
static void x264_cabac_mb_qp_delta( x264_t *h, x264_macroblock_t *mb )
{
    x264_macroblock_t *mbp = NULL;
    int i_slice_qp =  h->pps->i_pic_init_qp + h->sh.i_qp_delta;
    int i_last_dqp = 0;
    int i_ctxIdxInc = 0;
    int val;

    if( mb->i_mb_x > 0 || mb->i_mb_y > 0 )
    {
        mbp = mb - 1;
        if( mbp->i_mb_x > 0 || mbp->i_mb_y > 0 )
        {
            i_last_dqp = mbp->i_qp - (mbp-1)->i_qp;
        }
        else
        {
            i_last_dqp = mbp->i_qp - i_slice_qp;
        }
    }

    if( mbp != NULL &&
        !IS_SKIP( mbp->i_type ) && mbp->i_type != I_PCM &&
        i_last_dqp != 0 &&
        ( mbp->i_type == I_16x16 || mbp->i_cbp_luma != 0 || mbp->i_cbp_chroma != 0 ) )
    {
        i_ctxIdxInc = 1;
    }
    if( mbp )
        val = (mb->i_qp - mbp->i_qp) <= 0 ? (-2*(mb->i_qp - mbp->i_qp)) : (2*(mb->i_qp - mbp->i_qp)-1);
    else
        val = (mb->i_qp - i_slice_qp) <= 0 ? (-2*(mb->i_qp -i_slice_qp)) : (2*(mb->i_qp - i_slice_qp)-1);

    while( val > 0 )
    {
        x264_cabac_encode_decision( &h->cabac,  60 + i_ctxIdxInc, 1 );
        if( i_ctxIdxInc < 2 )
        {
            i_ctxIdxInc = 2;
        }
        else
        {
            i_ctxIdxInc = 3;
        }
        val--;
    }
    x264_cabac_encode_decision( &h->cabac,  60 + i_ctxIdxInc, 0 );
}

static int x264_cabac_mb_cbf_ctxidxinc( x264_macroblock_t *mb, int i_ctxBlockCat, int i_idx )
{
    x264_mb_context_t *ctx = mb->context;
    x264_macroblock_t *a = NULL;
    x264_macroblock_t *b = NULL;
    int i_nza = -1;
    int i_nzb = -1;

    int i_ctxIdxInc = 0;

    if( i_ctxBlockCat == 0 )
    {
        a = ctx->mba;
        b = ctx->mbb;

        if( a !=NULL && a->i_type == I_16x16 )
        {
            i_nza = array_non_zero_count( a->luma16x16_dc, 16 );
        }
        if( b !=NULL && b->i_type == I_16x16 )
        {
            i_nzb = array_non_zero_count( b->luma16x16_dc, 16 );
        }
    }
    else if( i_ctxBlockCat == 1 || i_ctxBlockCat == 2 )
    {
        int i8x8a, i8x8b;
        int x, y;

        a = ctx->block[i_idx].mba;
        b = ctx->block[i_idx].mbb;

        x = block_idx_x[i_idx];
        y = block_idx_y[i_idx];

        i8x8a = block_idx_xy[(x-1)&0x03][y]/4;
        i8x8b = block_idx_xy[x][(y-1)&0x03]/4;

        /* FIXME is &0x01 correct ? */
        if( a != NULL && !IS_SKIP( a->i_type ) && a->i_type != I_PCM &&
            ((a->i_cbp_luma >> i8x8a)) != 0 )
        {
            i_nza = ctx->block[i_idx].bka->i_non_zero_count;
        }
        if( b != NULL && !IS_SKIP( b->i_type ) && b->i_type != I_PCM &&
            ((b->i_cbp_luma >>i8x8b)) != 0 )
        {
            i_nzb = ctx->block[i_idx].bkb->i_non_zero_count;
        }
    }
    else if( i_ctxBlockCat == 3 )
    {
        a = ctx->mba;
        b = ctx->mbb;

        if( a != NULL && !IS_SKIP( a->i_type ) && a->i_type != I_PCM &&
            a->i_cbp_chroma != 0 )
        {
            i_nza = array_non_zero_count( a->chroma_dc[i_idx], 4 );
        }
        if( b != NULL && !IS_SKIP( b->i_type ) && b->i_type != I_PCM &&
            b->i_cbp_chroma != 0 )
        {
            i_nzb = array_non_zero_count( b->chroma_dc[i_idx], 4 );
        }
    }
    else if( i_ctxBlockCat == 4 )
    {
        a = ctx->block[16+i_idx].mba;
        b = ctx->block[16+i_idx].mbb;

        if( a != NULL && !IS_SKIP( a->i_type ) && a->i_type != I_PCM &&
            a->i_cbp_chroma == 2 )
        {
            i_nza = ctx->block[16+i_idx].bka->i_non_zero_count;
        }
        if( b != NULL && !IS_SKIP( b->i_type ) && b->i_type != I_PCM &&
            b->i_cbp_chroma == 2 )
        {
            i_nzb = ctx->block[16+i_idx].bkb->i_non_zero_count;
        }
    }

    if( ( a == NULL && IS_INTRA( mb->i_type ) ) || ( a != NULL && a->i_type == I_PCM ) || i_nza > 0 )
    {
        i_ctxIdxInc++;
    }
    if( ( b == NULL && IS_INTRA( mb->i_type ) ) || ( b != NULL && b->i_type == I_PCM ) || i_nzb > 0 )
    {
        i_ctxIdxInc += 2;
    }

    return i_ctxIdxInc + 4 * i_ctxBlockCat;
}

void x264_cabac_mb_skip( x264_t *h, x264_macroblock_t *mb, int b_skip )
{
    x264_macroblock_t *mba = mb->context->mba;
    x264_macroblock_t *mbb = mb->context->mbb;
    int i_ctxIdxInc = 0;

    if( mba != NULL && !IS_SKIP( mba->i_type ) )
    {
        i_ctxIdxInc++;
    }
    if( mbb != NULL && !IS_SKIP( mbb->i_type ) )
    {
        i_ctxIdxInc++;
    }

    if( h->sh.i_type == SLICE_TYPE_P )
    {
        x264_cabac_encode_decision( &h->cabac, 11 + i_ctxIdxInc, b_skip ? 1 : 0 );
    }
    else /* SLICE_TYPE_B */
    {
        x264_cabac_encode_decision( &h->cabac, 24 + i_ctxIdxInc, b_skip ? 1 : 0 );
    }
}

static void x264_cabac_mb_ref( x264_t *h, x264_macroblock_t *mb, int i_list, int i_part )
{
    x264_macroblock_t *a;
    x264_macroblock_t *b;

    int i_ctxIdxInc = 0;
    int i_ref;
    int i_refa = -1;
    int i_refb = -1;

    int x, y, xn, yn;

    x264_mb_partition_getxy( mb, i_part, 0, &x, &y );
    i_ref = mb->partition[x][y].i_ref[i_list];


    /* Left  pixel (-1,0)*/
    xn = x - 1;
    a = mb;
    if( xn < 0 )
    {
        xn += 4;
        a = mb->context->mba;
    }
    if( a && !IS_INTRA( a->i_type ) )
    {
        i_refa = a->partition[xn][y].i_ref[i_list];
    }

    /* Up ( pixel(0,-1)*/
    yn = y - 1;
    b = mb;
    if( yn < 0 )
    {
        yn += 4;
        b = mb->context->mbb;
    }
    if( b && !IS_INTRA( b->i_type ) )
    {
        i_refb = b->partition[x][yn].i_ref[i_list];
    }

    /* FIXME not complete for B frame (B_DIRECT and B_DIRECT 8x8 sub */
    if( i_refa > 0 && !IS_SKIP( a->i_type ) )
    {
        i_ctxIdxInc++;
    }
    if( i_refb > 0 && !IS_SKIP( b->i_type ) )
    {
        i_ctxIdxInc += 2;
    }

    while( i_ref > 0 )
    {
        x264_cabac_encode_decision( &h->cabac, 54 + i_ctxIdxInc, 1 );
        if( i_ctxIdxInc < 4 )
        {
            i_ctxIdxInc = 4;
        }
        else
        {
            i_ctxIdxInc = 5;
        }
        i_ref--;
    }
    x264_cabac_encode_decision( &h->cabac, 54 + i_ctxIdxInc, 0 );
}

static void  x264_cabac_mb_mvd( x264_t *h, int i_ctx, int i_ctx_inc, int mvd )
{
    int i_abs = abs( mvd );
    int i_prefix = X264_MIN( i_abs, 9 );
    int i;

    for( i = 0; i < i_prefix; i++ )
    {
        x264_cabac_encode_decision( &h->cabac, i_ctx + i_ctx_inc, 1 );
        if( i_ctx_inc < 3 )
        {
            i_ctx_inc = 3;
        }
        else if( i_ctx_inc < 6 )
        {
            i_ctx_inc++;
        }
    }
    if( i_prefix < 9 )
    {
        x264_cabac_encode_decision( &h->cabac, i_ctx + i_ctx_inc, 0 );
    }

    if( i_prefix >= 9 )
    {
        int k = 3;
        int i_suffix = i_abs - 9;

        while( i_suffix >= (1<<k) )
        {
            x264_cabac_encode_bypass( &h->cabac, 1 );
            i_suffix -= 1 << k;
            k++;
        }
        x264_cabac_encode_bypass( &h->cabac, 0 );
        while( k-- )
        {
            x264_cabac_encode_bypass( &h->cabac, (i_suffix >> k)&0x01 );
        }
    }

    /* sign */
    if( mvd > 0 )
    {
        x264_cabac_encode_bypass( &h->cabac, 0 );
    }
    else if( mvd < 0 )
    {
        x264_cabac_encode_bypass( &h->cabac, 1 );
    }
}

static void  x264_cabac_mb_mv( x264_t *h, x264_macroblock_t *mb, int i_list, int i_part, int i_sub )
{
    x264_macroblock_t *mbn;

    int mvd[2];
    int x, y, xn, yn;
    int i_ctxIdxInc;

    int i_absmv0 = 0;
    int i_absmv1 = 0;

    /* get and update mvd */
    x264_mb_partition_mvd( mb, i_list, i_part, i_sub, mvd );

    /* get context */
    x264_mb_partition_getxy( mb, i_part, i_sub, &x, &y );

    /* FIXME not complete for B frame (B_DIRECT and B_DIRECT 8x8 sub */
    /* Left  pixel (-1,0)*/
    xn = x - 1;
    mbn = mb;
    if( xn < 0 )
    {
        xn += 4;
        mbn = mb->context->mba;
    }
    if( mbn && !IS_INTRA( mbn->i_type ) && !IS_SKIP( mbn->i_type) )
    {
        i_absmv0 += abs( mbn->partition[xn][y].mvd[i_list][0] );
        i_absmv1 += abs( mbn->partition[xn][y].mvd[i_list][1] );
    }

    /* Up ( pixel(0,-1)*/
    yn = y - 1;
    mbn = mb;
    if( yn < 0 )
    {
        yn += 4;
        mbn = mb->context->mbb;
    }
    if( mbn && !IS_INTRA( mbn->i_type ) && !IS_SKIP( mbn->i_type) )
    {
        i_absmv0 += abs( mbn->partition[x][yn].mvd[i_list][0] );
        i_absmv1 += abs( mbn->partition[x][yn].mvd[i_list][1] );
    }

    /* x component */
    if( i_absmv0 < 3 )
    {
        i_ctxIdxInc = 0;
    }
    else if( i_absmv0 > 32 )
    {
        i_ctxIdxInc = 2;
    }
    else
    {
        i_ctxIdxInc = 1;
    }

    x264_cabac_mb_mvd( h, 40, i_ctxIdxInc, mvd[0] );

    /* y component */
    if( i_absmv1 < 3 )
    {
        i_ctxIdxInc = 0;
    }
    else if( i_absmv1 > 32 )
    {
        i_ctxIdxInc = 2;
    }
    else
    {
        i_ctxIdxInc = 1;
    }
    x264_cabac_mb_mvd( h, 47, i_ctxIdxInc, mvd[1] );
}
static void x264_cabac_mb_sub_partition( x264_t *h, int i_sub )
{
    switch( i_sub )
    {
        case D_L0_8x8:
            x264_cabac_encode_decision( &h->cabac, 21, 1 );
            break;
        case D_L0_8x4:
            x264_cabac_encode_decision( &h->cabac, 21, 0 );
            x264_cabac_encode_decision( &h->cabac, 22, 0 );
            break;
        case D_L0_4x8:
            x264_cabac_encode_decision( &h->cabac, 21, 0 );
            x264_cabac_encode_decision( &h->cabac, 22, 1 );
            x264_cabac_encode_decision( &h->cabac, 23, 1 );
            break;
        case D_L0_4x4:
            x264_cabac_encode_decision( &h->cabac, 21, 0 );
            x264_cabac_encode_decision( &h->cabac, 22, 1 );
            x264_cabac_encode_decision( &h->cabac, 23, 0 );
            break;
    }
}

static void block_residual_write_cabac( x264_t *h, x264_macroblock_t *mb, int i_ctxBlockCat, int i_idx, int *l, int i_count )
{
    static const int significant_coeff_flag_offset[5] = { 0, 15, 29, 44, 47 };
    static const int last_significant_coeff_flag_offset[5] = { 0, 15, 29, 44, 47 };
    static const int coeff_abs_level_m1_offset[5] = { 0, 10, 20, 30, 39 };

    int i_coeff_abs_m1[16];
    int i_coeff_sign[16];
    int i_coeff = 0;
    int i_last  = 0;

    int i_abslevel1 = 0;
    int i_abslevelgt1 = 0;

    int i;

    /* i_ctxBlockCat: 0-> DC 16x16  i_idx = 0
     *                1-> AC 16x16  i_idx = luma4x4idx
     *                2-> Luma4x4   i_idx = luma4x4idx
     *                3-> DC Chroma i_idx = iCbCr
     *                4-> AC Chroma i_idx = 4 * iCbCr + chroma4x4idx
     */

    //fprintf( stderr, "l[] = " );
    for( i = 0; i < i_count; i++ )
    {
        //fprintf( stderr, "%d ", l[i] );
        if( l[i] != 0 )
        {
            i_coeff_abs_m1[i_coeff] = abs( l[i] ) - 1;
            i_coeff_sign[i_coeff]   = ( l[i] < 0 ? 1 : 0);
            i_coeff++;

            i_last = i;
        }
    }
    //fprintf( stderr, "\n" );

    if( i_coeff == 0 )
    {
        /* codec block flag */
        x264_cabac_encode_decision( &h->cabac,  85 + x264_cabac_mb_cbf_ctxidxinc( mb, i_ctxBlockCat, i_idx ), 0 );
        return;
    }

    /* block coded */
    x264_cabac_encode_decision( &h->cabac,  85 + x264_cabac_mb_cbf_ctxidxinc( mb, i_ctxBlockCat, i_idx ), 1 );
    for( i = 0; i < i_count - 1; i++ )
    {
        int i_ctxIdxInc;

        i_ctxIdxInc = X264_MIN( i, i_count - 2 );

        if( l[i] != 0 )
        {
            x264_cabac_encode_decision( &h->cabac, 105 + significant_coeff_flag_offset[i_ctxBlockCat] + i_ctxIdxInc, 1 );
            x264_cabac_encode_decision( &h->cabac, 166 + last_significant_coeff_flag_offset[i_ctxBlockCat] + i_ctxIdxInc, i == i_last ? 1 : 0 );
        }
        else
        {
            x264_cabac_encode_decision( &h->cabac, 105 + significant_coeff_flag_offset[i_ctxBlockCat] + i_ctxIdxInc, 0 );
        }
        if( i == i_last )
        {
            break;
        }
    }

    for( i = i_coeff - 1; i >= 0; i-- )
    {
        int i_prefix;
        int i_ctxIdxInc;

        /* write coeff_abs - 1 */

        /* prefix */
        i_prefix = X264_MIN( i_coeff_abs_m1[i], 14 );

        i_ctxIdxInc = (i_abslevelgt1 != 0 ? 0 : X264_MIN( 4, i_abslevel1 + 1 )) + coeff_abs_level_m1_offset[i_ctxBlockCat];
        if( i_prefix == 0 )
        {
            x264_cabac_encode_decision( &h->cabac,  227 + i_ctxIdxInc, 0 );
        }
        else
        {
            int j;
            x264_cabac_encode_decision( &h->cabac,  227 + i_ctxIdxInc, 1 );
            i_ctxIdxInc = 5 + X264_MIN( 4, i_abslevelgt1 ) + coeff_abs_level_m1_offset[i_ctxBlockCat];
            for( j = 0; j < i_prefix - 1; j++ )
            {
                x264_cabac_encode_decision( &h->cabac,  227 + i_ctxIdxInc, 1 );
            }
            if( i_prefix < 14 )
            {
                x264_cabac_encode_decision( &h->cabac,  227 + i_ctxIdxInc, 0 );
            }
        }
        /* suffix */
        if( i_coeff_abs_m1[i] >= 14 )
        {
            int k = 0;
            int i_suffix = i_coeff_abs_m1[i] - 14;

            while( i_suffix >= (1<<k) )
            {
                x264_cabac_encode_bypass( &h->cabac, 1 );
                i_suffix -= 1 << k;
                k++;
            }
            x264_cabac_encode_bypass( &h->cabac, 0 );
            while( k-- )
            {
                x264_cabac_encode_bypass( &h->cabac, (i_suffix >> k)&0x01 );
            }
        }

        /* write sign */
        x264_cabac_encode_bypass( &h->cabac, i_coeff_sign[i] );


        if( i_coeff_abs_m1[i] == 0 )
        {
            i_abslevel1++;
        }
        else
        {
            i_abslevelgt1++;
        }
    }
}



void x264_macroblock_write_cabac( x264_t *h, bs_t *s, x264_macroblock_t *mb )
{
    int i;

    /* Write the MB type */
#if 0
    fprintf( stderr, "[%d,%d] type=%d cbp=%d predc=%d\n",
             mb->i_mb_x, mb->i_mb_y,
             1 + mb->i_intra16x16_pred_mode + mb->i_cbp_chroma * 4 + ( mb->i_cbp_luma == 0 ? 0 : 12 ),
             (mb->i_cbp_chroma << 4)|mb->i_cbp_luma,
             mb->i_chroma_pred_mode );
#endif
    x264_cabac_mb_type( h, mb );

    /* PCM special block type UNTESTED */
    if( mb->i_type == I_PCM )
    {
        bs_align_0( s );    /* not sure */
        /* Luma */
        for( i = 0; i < 16*16; i++ )
        {
            bs_write( s, 8, h->picture->plane[0][mb->i_mb_y * 16 * h->picture->i_stride[0] + mb->i_mb_x * 16+i] );
        }
        /* Cb */
        for( i = 0; i < 8*8; i++ )
        {
            bs_write( s, 8, h->picture->plane[1][mb->i_mb_y * 8 * h->picture->i_stride[1] + mb->i_mb_x * 8+i] );
        }
        /* Cr */
        for( i = 0; i < 8*8; i++ )
        {
            bs_write( s, 8, h->picture->plane[2][mb->i_mb_y * 8 * h->picture->i_stride[2] + mb->i_mb_x * 8+i] );
        }

        for( i = 0; i < 16 + 8; i++ )
        {
            /* special case */
            mb->block[i].i_non_zero_count = 16;
        }

        x264_cabac_encode_init( &h->cabac, s );
        return;
    }

    if( IS_INTRA( mb->i_type ) )
    {
        /* Prediction */
        if( mb->i_type == I_4x4 )
        {
            for( i = 0; i < 16; i++ )
            {
                x264_cabac_mb_intra4x4_pred_mode( h, mb,
                                                  x264_mb_predict_intra4x4_mode( h, mb, i ),
                                                  mb->block[i].i_intra4x4_pred_mode );
            }
        }
        x264_cabac_mb_intra8x8_pred_mode( h, mb );
    }
    else if( mb->i_type == P_8x8 )
    {
        /* sub mb type */
        for( i = 0; i < 4; i++ )
        {
            x264_cabac_mb_sub_partition( h, mb->i_sub_partition[i] );
        }
        /* ref 0 */
        if( h->sh.i_num_ref_idx_l0_active > 1 )
        {
            for( i = 0; i < 4; i++ )
            {
                x264_cabac_mb_ref( h, mb, 0, i );
            }
        }

        for( i = 0; i < 4; i++ )
        {
            int i_sub;
            for( i_sub = 0; i_sub < x264_mb_partition_count_table[mb->i_sub_partition[i]]; i_sub++ )
            {
                x264_cabac_mb_mv( h, mb, 0, i, i_sub );
            }
        }
    }
    else if( mb->i_type == B_8x8 )
    {
        /* TODO */
        fprintf( stderr, "Arggg B_8x8\n" );
    }
    else if( mb->i_type != B_DIRECT )
    {
        /* FIXME -> invalid for B frame */

        /* Motion Vector */
        int i_part = x264_mb_partition_count_table[mb->i_partition];

        if( h->sh.i_num_ref_idx_l0_active > 1 )
        {
            for( i = 0; i < i_part; i++ )
            {
                if( mb->i_type == P_L0 )
                {
                    x264_cabac_mb_ref( h, mb, 0, i );
                }
            }
        }

        for( i = 0; i < i_part; i++ )
        {
            if( mb->i_type == P_L0 )
            {
                x264_cabac_mb_mv( h, mb, 0, i, 0 );
            }
        }
    }

    if( mb->i_type != I_16x16 )
    {
        x264_cabac_mb_cbp_luma( h, mb );
        x264_cabac_mb_cbp_chroma( h, mb );
    }

    if( mb->i_cbp_luma > 0 || mb->i_cbp_chroma > 0 || mb->i_type == I_16x16 )
    {
        x264_cabac_mb_qp_delta( h, mb );

        /* write residual */
        if( mb->i_type == I_16x16 )
        {
            /* DC Luma */
            block_residual_write_cabac( h, mb, 0, 0, mb->luma16x16_dc, 16 );

            if( mb->i_cbp_luma != 0 )
            {
                /* AC Luma */
                for( i = 0; i < 16; i++ )
                {
                    block_residual_write_cabac( h, mb, 1, i, mb->block[i].residual_ac, 15 );
                }
            }
        }
        else
        {
            for( i = 0; i < 16; i++ )
            {
                if( mb->i_cbp_luma & ( 1 << ( i / 4 ) ) )
                {
                    block_residual_write_cabac( h, mb, 2, i, mb->block[i].luma4x4, 16 );
                }
            }
        }

        if( mb->i_cbp_chroma &0x03 )    /* Chroma DC residual present */
        {
            block_residual_write_cabac( h, mb, 3, 0, mb->chroma_dc[0], 4 );
            block_residual_write_cabac( h, mb, 3, 1, mb->chroma_dc[1], 4 );
        }
        if( mb->i_cbp_chroma&0x02 ) /* Chroma AC residual present */
        {
            for( i = 0; i < 8; i++ )
            {
                block_residual_write_cabac( h, mb, 4, i, mb->block[16+i].residual_ac, 15 );
            }
        }
    }
}


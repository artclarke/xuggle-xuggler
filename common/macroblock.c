/*****************************************************************************
 * macroblock.c: h264 encoder library
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar
 * $Id: macroblock.c,v 1.1 2004/06/03 19:27:06 fenrir Exp $
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

#include "common.h"
#include "macroblock.h"

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

static const int dequant_mf[6][4][4] =
{
    { {10, 13, 10, 13}, {13, 16, 13, 16}, {10, 13, 10, 13}, {13, 16, 13, 16} },
    { {11, 14, 11, 14}, {14, 18, 14, 18}, {11, 14, 11, 14}, {14, 18, 14, 18} },
    { {13, 16, 13, 16}, {16, 20, 16, 20}, {13, 16, 13, 16}, {16, 20, 16, 20} },
    { {14, 18, 14, 18}, {18, 23, 18, 23}, {14, 18, 14, 18}, {18, 23, 18, 23} },
    { {16, 20, 16, 20}, {20, 25, 20, 25}, {16, 20, 16, 20}, {20, 25, 20, 25} },
    { {18, 23, 18, 23}, {23, 29, 23, 29}, {18, 23, 18, 23}, {23, 29, 23, 29} }
};

static const int dequant8_mf[6][8][8] =
{
  {
    {20, 19, 25, 19, 20, 19, 25, 19},
    {19, 18, 24, 18, 19, 18, 24, 18},
    {25, 24, 32, 24, 25, 24, 32, 24},
    {19, 18, 24, 18, 19, 18, 24, 18},
    {20, 19, 25, 19, 20, 19, 25, 19},
    {19, 18, 24, 18, 19, 18, 24, 18},
    {25, 24, 32, 24, 25, 24, 32, 24},
    {19, 18, 24, 18, 19, 18, 24, 18}
  }, {
    {22, 21, 28, 21, 22, 21, 28, 21},
    {21, 19, 26, 19, 21, 19, 26, 19},
    {28, 26, 35, 26, 28, 26, 35, 26},
    {21, 19, 26, 19, 21, 19, 26, 19},
    {22, 21, 28, 21, 22, 21, 28, 21},
    {21, 19, 26, 19, 21, 19, 26, 19},
    {28, 26, 35, 26, 28, 26, 35, 26},
    {21, 19, 26, 19, 21, 19, 26, 19}
  }, {
    {26, 24, 33, 24, 26, 24, 33, 24},
    {24, 23, 31, 23, 24, 23, 31, 23},
    {33, 31, 42, 31, 33, 31, 42, 31},
    {24, 23, 31, 23, 24, 23, 31, 23},
    {26, 24, 33, 24, 26, 24, 33, 24},
    {24, 23, 31, 23, 24, 23, 31, 23},
    {33, 31, 42, 31, 33, 31, 42, 31},
    {24, 23, 31, 23, 24, 23, 31, 23}
  }, {
    {28, 26, 35, 26, 28, 26, 35, 26},
    {26, 25, 33, 25, 26, 25, 33, 25},
    {35, 33, 45, 33, 35, 33, 45, 33},
    {26, 25, 33, 25, 26, 25, 33, 25},
    {28, 26, 35, 26, 28, 26, 35, 26},
    {26, 25, 33, 25, 26, 25, 33, 25},
    {35, 33, 45, 33, 35, 33, 45, 33},
    {26, 25, 33, 25, 26, 25, 33, 25}
  }, {
    {32, 30, 40, 30, 32, 30, 40, 30},
    {30, 28, 38, 28, 30, 28, 38, 28},
    {40, 38, 51, 38, 40, 38, 51, 38},
    {30, 28, 38, 28, 30, 28, 38, 28},
    {32, 30, 40, 30, 32, 30, 40, 30},
    {30, 28, 38, 28, 30, 28, 38, 28},
    {40, 38, 51, 38, 40, 38, 51, 38},
    {30, 28, 38, 28, 30, 28, 38, 28}
  }, {
    {36, 34, 46, 34, 36, 34, 46, 34},
    {34, 32, 43, 32, 34, 32, 43, 32},
    {46, 43, 58, 43, 46, 43, 58, 43},
    {34, 32, 43, 32, 34, 32, 43, 32},
    {36, 34, 46, 34, 36, 34, 46, 34},
    {34, 32, 43, 32, 34, 32, 43, 32},
    {46, 43, 58, 43, 46, 43, 58, 43},
    {34, 32, 43, 32, 34, 32, 43, 32}
  }
};

int x264_mb_predict_intra4x4_mode( x264_t *h, int idx )
{
    const int ma = h->mb.cache.intra4x4_pred_mode[x264_scan8[idx] - 1];
    const int mb = h->mb.cache.intra4x4_pred_mode[x264_scan8[idx] - 8];
    const int m  = X264_MIN( x264_mb_pred_mode4x4_fix(ma),
                             x264_mb_pred_mode4x4_fix(mb) );

    if( m < 0 )
        return I_PRED_4x4_DC;

    return m;
}

int x264_mb_predict_non_zero_code( x264_t *h, int idx )
{
    const int za = h->mb.cache.non_zero_count[x264_scan8[idx] - 1];
    const int zb = h->mb.cache.non_zero_count[x264_scan8[idx] - 8];

    int i_ret = za + zb;

    if( i_ret < 0x80 )
    {
        i_ret = ( i_ret + 1 ) >> 1;
    }
    return i_ret & 0x7f;
}

int x264_mb_transform_8x8_allowed( x264_t *h )
{
    if( IS_SKIP( h->mb.i_type ) )
        return 0;
    if( h->mb.i_type == P_8x8 || h->mb.i_type == B_8x8 )
    {
        int i;
        for( i = 0; i < 4; i++ )
            if( !IS_SUB8x8(h->mb.i_sub_partition[i])
                || ( h->mb.i_sub_partition[i] == D_DIRECT_8x8 && !h->sps->b_direct8x8_inference ) )
            {
                return 0;
            }
    }
    if( h->mb.i_type == B_DIRECT && !h->sps->b_direct8x8_inference )
        return 0;

    return 1;
}

/****************************************************************************
 * Scan and Quant functions
 ****************************************************************************/
void x264_mb_dequant_2x2_dc( int16_t dct[2][2], int dequant_mf[6][4][4], int i_qscale )
{
    const int i_qbits = i_qscale/6 - 5;

    if( i_qbits >= 0 )
    {
        const int i_dmf = dequant_mf[i_qscale%6][0][0] << i_qbits;

        dct[0][0] *= i_dmf;
        dct[0][1] *= i_dmf;
        dct[1][0] *= i_dmf;
        dct[1][1] *= i_dmf;
    }
    else
    {
        const int i_dmf = dequant_mf[i_qscale%6][0][0];
        // chroma DC is truncated, not rounded

        dct[0][0] = ( dct[0][0] * i_dmf ) >> (-i_qbits);
        dct[0][1] = ( dct[0][1] * i_dmf ) >> (-i_qbits);
        dct[1][0] = ( dct[1][0] * i_dmf ) >> (-i_qbits);
        dct[1][1] = ( dct[1][1] * i_dmf ) >> (-i_qbits);
    }
}

void x264_mb_dequant_4x4_dc( int16_t dct[4][4], int dequant_mf[6][4][4], int i_qscale )
{
    const int i_qbits = i_qscale/6 - 6;
    int y;

    if( i_qbits >= 0 )
    {
        const int i_dmf = dequant_mf[i_qscale%6][0][0] << i_qbits;

        for( y = 0; y < 4; y++ )
        {
            dct[y][0] *= i_dmf;
            dct[y][1] *= i_dmf;
            dct[y][2] *= i_dmf;
            dct[y][3] *= i_dmf;
        }
    }
    else
    {
        const int i_dmf = dequant_mf[i_qscale%6][0][0];
        const int f = 1 << (-i_qbits-1);

        for( y = 0; y < 4; y++ )
        {
            dct[y][0] = ( dct[y][0] * i_dmf + f ) >> (-i_qbits);
            dct[y][1] = ( dct[y][1] * i_dmf + f ) >> (-i_qbits);
            dct[y][2] = ( dct[y][2] * i_dmf + f ) >> (-i_qbits);
            dct[y][3] = ( dct[y][3] * i_dmf + f ) >> (-i_qbits);
        }
    }
}

void x264_mb_dequant_4x4( int16_t dct[4][4], int dequant_mf[6][4][4], int i_qscale )
{
    const int i_mf = i_qscale%6;
    const int i_qbits = i_qscale/6 - 4;
    int y;

    if( i_qbits >= 0 )
    {
        for( y = 0; y < 4; y++ )
        {
            dct[y][0] = ( dct[y][0] * dequant_mf[i_mf][y][0] ) << i_qbits;
            dct[y][1] = ( dct[y][1] * dequant_mf[i_mf][y][1] ) << i_qbits;
            dct[y][2] = ( dct[y][2] * dequant_mf[i_mf][y][2] ) << i_qbits;
            dct[y][3] = ( dct[y][3] * dequant_mf[i_mf][y][3] ) << i_qbits;
        }
    }
    else
    {
        const int f = 1 << (-i_qbits-1);
        for( y = 0; y < 4; y++ )
        {
            dct[y][0] = ( dct[y][0] * dequant_mf[i_mf][y][0] + f ) >> (-i_qbits);
            dct[y][1] = ( dct[y][1] * dequant_mf[i_mf][y][1] + f ) >> (-i_qbits);
            dct[y][2] = ( dct[y][2] * dequant_mf[i_mf][y][2] + f ) >> (-i_qbits);
            dct[y][3] = ( dct[y][3] * dequant_mf[i_mf][y][3] + f ) >> (-i_qbits);
        }
    }
}

void x264_mb_dequant_8x8( int16_t dct[8][8], int dequant_mf[6][8][8], int i_qscale )
{
    const int i_mf = i_qscale%6;
    const int i_qbits = i_qscale/6 - 6;
    int y;

    if( i_qbits >= 0 )
    {
        for( y = 0; y < 8; y++ )
        {
            dct[y][0] = ( dct[y][0] * dequant_mf[i_mf][y][0] ) << i_qbits;
            dct[y][1] = ( dct[y][1] * dequant_mf[i_mf][y][1] ) << i_qbits;
            dct[y][2] = ( dct[y][2] * dequant_mf[i_mf][y][2] ) << i_qbits;
            dct[y][3] = ( dct[y][3] * dequant_mf[i_mf][y][3] ) << i_qbits;
            dct[y][4] = ( dct[y][4] * dequant_mf[i_mf][y][4] ) << i_qbits;
            dct[y][5] = ( dct[y][5] * dequant_mf[i_mf][y][5] ) << i_qbits;
            dct[y][6] = ( dct[y][6] * dequant_mf[i_mf][y][6] ) << i_qbits;
            dct[y][7] = ( dct[y][7] * dequant_mf[i_mf][y][7] ) << i_qbits;
        }
    }
    else
    {
        const int f = 1 << (-i_qbits-1);
        for( y = 0; y < 8; y++ )
        {
            dct[y][0] = ( dct[y][0] * dequant_mf[i_mf][y][0] + f ) >> (-i_qbits);
            dct[y][1] = ( dct[y][1] * dequant_mf[i_mf][y][1] + f ) >> (-i_qbits);
            dct[y][2] = ( dct[y][2] * dequant_mf[i_mf][y][2] + f ) >> (-i_qbits);
            dct[y][3] = ( dct[y][3] * dequant_mf[i_mf][y][3] + f ) >> (-i_qbits);
            dct[y][4] = ( dct[y][4] * dequant_mf[i_mf][y][4] + f ) >> (-i_qbits);
            dct[y][5] = ( dct[y][5] * dequant_mf[i_mf][y][5] + f ) >> (-i_qbits);
            dct[y][6] = ( dct[y][6] * dequant_mf[i_mf][y][6] + f ) >> (-i_qbits);
            dct[y][7] = ( dct[y][7] * dequant_mf[i_mf][y][7] + f ) >> (-i_qbits);
        }
    }
}

void x264_mb_predict_mv( x264_t *h, int i_list, int idx, int i_width, int mvp[2] )
{
    const int i8 = x264_scan8[idx];
    const int i_ref= h->mb.cache.ref[i_list][i8];
    int     i_refa = h->mb.cache.ref[i_list][i8 - 1];
    int16_t *mv_a  = h->mb.cache.mv[i_list][i8 - 1];
    int     i_refb = h->mb.cache.ref[i_list][i8 - 8];
    int16_t *mv_b  = h->mb.cache.mv[i_list][i8 - 8];
    int     i_refc = h->mb.cache.ref[i_list][i8 - 8 + i_width ];
    int16_t *mv_c  = h->mb.cache.mv[i_list][i8 - 8 + i_width];

    int i_count;

    if( (idx&0x03) == 3 || ( i_width == 2 && (idx&0x3) == 2 )|| i_refc == -2 )
    {
        i_refc = h->mb.cache.ref[i_list][i8 - 8 - 1];
        mv_c   = h->mb.cache.mv[i_list][i8 - 8 - 1];
    }

    if( h->mb.i_partition == D_16x8 )
    {
        if( idx == 0 && i_refb == i_ref )
        {
            mvp[0] = mv_b[0];
            mvp[1] = mv_b[1];
            return;
        }
        else if( idx != 0 && i_refa == i_ref )
        {
            mvp[0] = mv_a[0];
            mvp[1] = mv_a[1];
            return;
        }
    }
    else if( h->mb.i_partition == D_8x16 )
    {
        if( idx == 0 && i_refa == i_ref )
        {
            mvp[0] = mv_a[0];
            mvp[1] = mv_a[1];
            return;
        }
        else if( idx != 0 && i_refc == i_ref )
        {
            mvp[0] = mv_c[0];
            mvp[1] = mv_c[1];
            return;
        }
    }

    i_count = 0;
    if( i_refa == i_ref ) i_count++;
    if( i_refb == i_ref ) i_count++;
    if( i_refc == i_ref ) i_count++;

    if( i_count > 1 )
    {
        mvp[0] = x264_median( mv_a[0], mv_b[0], mv_c[0] );
        mvp[1] = x264_median( mv_a[1], mv_b[1], mv_c[1] );
    }
    else if( i_count == 1 )
    {
        if( i_refa == i_ref )
        {
            mvp[0] = mv_a[0];
            mvp[1] = mv_a[1];
        }
        else if( i_refb == i_ref )
        {
            mvp[0] = mv_b[0];
            mvp[1] = mv_b[1];
        }
        else
        {
            mvp[0] = mv_c[0];
            mvp[1] = mv_c[1];
        }
    }
    else if( i_refb == -2 && i_refc == -2 && i_refa != -2 )
    {
        mvp[0] = mv_a[0];
        mvp[1] = mv_a[1];
    }
    else
    {
        mvp[0] = x264_median( mv_a[0], mv_b[0], mv_c[0] );
        mvp[1] = x264_median( mv_a[1], mv_b[1], mv_c[1] );
    }
}

void x264_mb_predict_mv_16x16( x264_t *h, int i_list, int i_ref, int mvp[2] )
{
    int     i_refa = h->mb.cache.ref[i_list][X264_SCAN8_0 - 1];
    int16_t *mv_a  = h->mb.cache.mv[i_list][X264_SCAN8_0 - 1];
    int     i_refb = h->mb.cache.ref[i_list][X264_SCAN8_0 - 8];
    int16_t *mv_b  = h->mb.cache.mv[i_list][X264_SCAN8_0 - 8];
    int     i_refc = h->mb.cache.ref[i_list][X264_SCAN8_0 - 8 + 4];
    int16_t *mv_c  = h->mb.cache.mv[i_list][X264_SCAN8_0 - 8 + 4];

    int i_count;

    if( i_refc == -2 )
    {
        i_refc = h->mb.cache.ref[i_list][X264_SCAN8_0 - 8 - 1];
        mv_c   = h->mb.cache.mv[i_list][X264_SCAN8_0 - 8 - 1];
    }

    i_count = 0;
    if( i_refa == i_ref ) i_count++;
    if( i_refb == i_ref ) i_count++;
    if( i_refc == i_ref ) i_count++;

    if( i_count > 1 )
    {
        mvp[0] = x264_median( mv_a[0], mv_b[0], mv_c[0] );
        mvp[1] = x264_median( mv_a[1], mv_b[1], mv_c[1] );
    }
    else if( i_count == 1 )
    {
        if( i_refa == i_ref )
        {
            mvp[0] = mv_a[0];
            mvp[1] = mv_a[1];
        }
        else if( i_refb == i_ref )
        {
            mvp[0] = mv_b[0];
            mvp[1] = mv_b[1];
        }
        else
        {
            mvp[0] = mv_c[0];
            mvp[1] = mv_c[1];
        }
    }
    else if( i_refb == -2 && i_refc == -2 && i_refa != -2 )
    {
        mvp[0] = mv_a[0];
        mvp[1] = mv_a[1];
    }
    else
    {
        mvp[0] = x264_median( mv_a[0], mv_b[0], mv_c[0] );
        mvp[1] = x264_median( mv_a[1], mv_b[1], mv_c[1] );
    }
}


void x264_mb_predict_mv_pskip( x264_t *h, int mv[2] )
{
    int     i_refa = h->mb.cache.ref[0][X264_SCAN8_0 - 1];
    int     i_refb = h->mb.cache.ref[0][X264_SCAN8_0 - 8];
    int16_t *mv_a  = h->mb.cache.mv[0][X264_SCAN8_0 - 1];
    int16_t *mv_b  = h->mb.cache.mv[0][X264_SCAN8_0 - 8];

    if( i_refa == -2 || i_refb == -2 ||
        ( i_refa == 0 && mv_a[0] == 0 && mv_a[1] == 0 ) ||
        ( i_refb == 0 && mv_b[0] == 0 && mv_b[1] == 0 ) )
    {
        mv[0] = mv[1] = 0;
    }
    else
    {
        x264_mb_predict_mv_16x16( h, 0, 0, mv );
    }
}

static int x264_mb_predict_mv_direct16x16_temporal( x264_t *h )
{
    int i_mb_4x4 = 16 * h->mb.i_mb_stride * h->mb.i_mb_y + 4 * h->mb.i_mb_x;
    int i_mb_8x8 =  4 * h->mb.i_mb_stride * h->mb.i_mb_y + 2 * h->mb.i_mb_x;
    int i;
    
    x264_macroblock_cache_ref( h, 0, 0, 4, 4, 1, 0 );
    
    if( IS_INTRA( h->fref1[0]->mb_type[ h->mb.i_mb_xy ] ) )
    {
        x264_macroblock_cache_ref( h, 0, 0, 4, 4, 0, 0 );
        x264_macroblock_cache_mv(  h, 0, 0, 4, 4, 0, 0, 0 );
        x264_macroblock_cache_mv(  h, 0, 0, 4, 4, 1, 0, 0 );
        return 1;
    }

    /* FIXME: optimize per block size */
    for( i = 0; i < 4; i++ )
    {
        const int x8 = 2*(i%2);
        const int y8 = 2*(i/2);
        const int i_part_8x8 = i_mb_8x8 + x8/2 + y8 * h->mb.i_mb_stride;
        const int i_ref = h->mb.map_col_to_list0[ h->fref1[0]->ref[0][ i_part_8x8 ] ];

        if( i_ref >= 0 )
        {
            const int dist_scale_factor = h->mb.dist_scale_factor[i_ref][0];
            int x4, y4;

            x264_macroblock_cache_ref( h, x8, y8, 2, 2, 0, i_ref );

            for( y4 = y8; y4 < y8+2; y4++ )
                for( x4 = x8; x4 < x8+2; x4++ )
                {
                    const int16_t *mv_col = h->fref1[0]->mv[0][ i_mb_4x4 + x4 + y4 * 4 * h->mb.i_mb_stride ];
                    int mv_l0[2];
                    mv_l0[0] = ( dist_scale_factor * mv_col[0] + 128 ) >> 8;
                    mv_l0[1] = ( dist_scale_factor * mv_col[1] + 128 ) >> 8;
                    x264_macroblock_cache_mv( h, x4, y4, 1, 1, 0, mv_l0[0], mv_l0[1] );
                    x264_macroblock_cache_mv( h, x4, y4, 1, 1, 1, mv_l0[0] - mv_col[0], mv_l0[1] - mv_col[1] );
                }
        }
        else
        {
            /* the colocated ref isn't in the current list0 */
            /* FIXME: we might still be able to use direct_8x8 on some partitions */
            /* FIXME: with B-pyramid + extensive ref list reordering
             *   (not currently used), we would also have to check
             *   l1mv1 like in spatial mode */
            return 0;
        }
    }

    return 1;
}

static int x264_mb_predict_mv_direct16x16_spatial( x264_t *h )
{
    int ref[2];
    int mv[2][2];
    int i_list;
    int i8, i4;
    const int8_t *l1ref0 = &h->fref1[0]->ref[0][ h->mb.i_b8_xy ];
    const int8_t *l1ref1 = &h->fref1[0]->ref[1][ h->mb.i_b8_xy ];
    const int16_t (*l1mv0)[2] = (const int16_t (*)[2]) &h->fref1[0]->mv[0][ h->mb.i_b4_xy ];
    const int16_t (*l1mv1)[2] = (const int16_t (*)[2]) &h->fref1[0]->mv[1][ h->mb.i_b4_xy ];
    const int intra_col = IS_INTRA( h->fref1[0]->mb_type[ h->mb.i_mb_xy ] );

    for( i_list=0; i_list<2; i_list++ )
    {
        int i_refa = h->mb.cache.ref[i_list][X264_SCAN8_0 - 1];
        int i_refb = h->mb.cache.ref[i_list][X264_SCAN8_0 - 8];
        int i_refc = h->mb.cache.ref[i_list][X264_SCAN8_0 - 8 + 4];
        if( i_refc == -2 )
            i_refc = h->mb.cache.ref[i_list][X264_SCAN8_0 - 8 - 1];

        ref[i_list] = i_refa;
        if( ref[i_list] < 0 || ( i_refb < ref[i_list] && i_refb >= 0 ))
            ref[i_list] = i_refb;
        if( ref[i_list] < 0 || ( i_refc < ref[i_list] && i_refc >= 0 ))
            ref[i_list] = i_refc;
        if( ref[i_list] < 0 )
            ref[i_list] = -1;
    }

    if( ref[0] < 0 && ref[1] < 0 )
    {
        ref[0] = 
        ref[1] = 0;
        mv[0][0] = 
        mv[0][1] = 
        mv[1][0] = 
        mv[1][1] = 0;
    }
    else
    {
        for( i_list=0; i_list<2; i_list++ )
        {
            if( ref[i_list] >= 0 )
                x264_mb_predict_mv_16x16( h, i_list, ref[i_list], mv[i_list] );
            else
                mv[i_list][0] = mv[i_list][1] = 0;
        }
    }

    x264_macroblock_cache_ref( h, 0, 0, 4, 4, 0, ref[0] );
    x264_macroblock_cache_ref( h, 0, 0, 4, 4, 1, ref[1] );
    x264_macroblock_cache_mv(  h, 0, 0, 4, 4, 0, mv[0][0], mv[0][1] );
    x264_macroblock_cache_mv(  h, 0, 0, 4, 4, 1, mv[1][0], mv[1][1] );

    /* col_zero_flag */
    for( i8=0; i8<4; i8++ )
    {
        const int x8 = i8%2;
        const int y8 = i8/2;
        const int o8 = x8 + y8 * h->mb.i_b8_stride;
        if( !intra_col && ( l1ref0[o8] == 0 || ( l1ref0[o8] < 0 && l1ref1[o8] == 0 ) ) )
        {
            const int16_t (*l1mv)[2] = (l1ref0[o8] == 0) ? l1mv0 : l1mv1;
            for( i4=0; i4<4; i4++ )
            {
                const int x4 = i4%2 + 2*x8;
                const int y4 = i4/2 + 2*y8;
                const int16_t *mvcol = l1mv[x4 + y4 * h->mb.i_b4_stride];
                if( abs( mvcol[0] ) <= 1 && abs( mvcol[1] ) <= 1 )
                {
                    if( ref[0] == 0 )
                        x264_macroblock_cache_mv( h, x4, y4, 1, 1, 0, 0, 0 );
                    if( ref[1] == 0 )
                        x264_macroblock_cache_mv( h, x4, y4, 1, 1, 1, 0, 0 );
                }
            }
        }
    }

    return 1;
}

int x264_mb_predict_mv_direct16x16( x264_t *h )
{
    int b_available;
    if( h->param.analyse.i_direct_mv_pred == X264_DIRECT_PRED_NONE )
        return 0;
    else if( h->sh.b_direct_spatial_mv_pred )
        b_available = x264_mb_predict_mv_direct16x16_spatial( h );
    else
        b_available = x264_mb_predict_mv_direct16x16_temporal( h );

    /* cache ref & mv */
    if( b_available )
    {
        int i, l;
        for( l = 0; l < 2; l++ )
            for( i = 0; i < 4; i++ )
                h->mb.cache.direct_ref[l][i] = h->mb.cache.ref[l][x264_scan8[i*4]];
        memcpy(h->mb.cache.direct_mv, h->mb.cache.mv, sizeof(h->mb.cache.mv));
    }

    return b_available;
}

void x264_mb_load_mv_direct8x8( x264_t *h, int idx )
{
    const int x = 2*(idx%2);
    const int y = 2*(idx/2);
    int l;
    x264_macroblock_cache_ref( h, x, y, 2, 2, 0, h->mb.cache.direct_ref[0][idx] );
    x264_macroblock_cache_ref( h, x, y, 2, 2, 1, h->mb.cache.direct_ref[1][idx] );
    for( l = 0; l < 2; l++ )
    {
        *(uint64_t*)h->mb.cache.mv[l][x264_scan8[idx*4]] =
        *(uint64_t*)h->mb.cache.direct_mv[l][x264_scan8[idx*4]];
        *(uint64_t*)h->mb.cache.mv[l][x264_scan8[idx*4]+8] =
        *(uint64_t*)h->mb.cache.direct_mv[l][x264_scan8[idx*4]+8];
    }
}

/* This just improves encoder performance, it's not part of the spec */
void x264_mb_predict_mv_ref16x16( x264_t *h, int i_list, int i_ref, int mvc[5][2], int *i_mvc )
{
    int16_t (*mvr)[2] = h->mb.mvr[i_list][i_ref];
    int i = 0;

    /* temporal */
    if( h->sh.i_type == SLICE_TYPE_B )
    {
        if( h->mb.cache.ref[i_list][x264_scan8[12]] == i_ref )
        {
            /* FIXME: use direct_mv to be clearer? */
            int16_t *mvp = h->mb.cache.mv[i_list][x264_scan8[12]];
            mvc[i][0] = mvp[0];
            mvc[i][1] = mvp[1];
            i++;
        }
    }

    /* spatial */
    if( h->mb.i_neighbour & MB_LEFT )
    {
        int i_mb_l = h->mb.i_mb_xy - 1;
        /* skip MBs didn't go through the whole search process, so mvr is undefined */
        if( !IS_SKIP( h->mb.type[i_mb_l] ) )
        {
            mvc[i][0] = mvr[i_mb_l][0];
            mvc[i][1] = mvr[i_mb_l][1];
            i++;
        }
    }
    if( h->mb.i_neighbour & MB_TOP )
    {
        int i_mb_t = h->mb.i_mb_xy - h->mb.i_mb_stride;
        if( !IS_SKIP( h->mb.type[i_mb_t] ) )
        {
            mvc[i][0] = mvr[i_mb_t][0];
            mvc[i][1] = mvr[i_mb_t][1];
            i++;
        }

        if( h->mb.i_neighbour & MB_TOPLEFT && !IS_SKIP( h->mb.type[i_mb_t - 1] ) )
        {
            mvc[i][0] = mvr[i_mb_t - 1][0];
            mvc[i][1] = mvr[i_mb_t - 1][1];
            i++;
        }
        if( h->mb.i_mb_x < h->mb.i_mb_stride - 1 && !IS_SKIP( h->mb.type[i_mb_t + 1] ) )
        {
            mvc[i][0] = mvr[i_mb_t + 1][0];
            mvc[i][1] = mvr[i_mb_t + 1][1];
            i++;
        }
    }
    *i_mvc = i;
}

static inline void x264_mb_mc_0xywh( x264_t *h, int x, int y, int width, int height )
{
    const int i8 = x264_scan8[0]+x+8*y;
    const int i_ref = h->mb.cache.ref[0][i8];
    const int mvx   = x264_clip3( h->mb.cache.mv[0][i8][0], h->mb.mv_min[0], h->mb.mv_max[0] );
    const int mvy   = x264_clip3( h->mb.cache.mv[0][i8][1], h->mb.mv_min[1], h->mb.mv_max[1] );

    h->mc.mc_luma( h->mb.pic.p_fref[0][i_ref], h->mb.pic.i_stride[0],
                    &h->mb.pic.p_fdec[0][4*y * h->mb.pic.i_stride[0]+4*x],           h->mb.pic.i_stride[0],
                    mvx + 4*4*x, mvy + 4*4*y, 4*width, 4*height );

    h->mc.mc_chroma( &h->mb.pic.p_fref[0][i_ref][4][2*y*h->mb.pic.i_stride[1]+2*x], h->mb.pic.i_stride[1],
                      &h->mb.pic.p_fdec[1][2*y*h->mb.pic.i_stride[1]+2*x],           h->mb.pic.i_stride[1],
                      mvx, mvy, 2*width, 2*height );

    h->mc.mc_chroma( &h->mb.pic.p_fref[0][i_ref][5][2*y*h->mb.pic.i_stride[2]+2*x], h->mb.pic.i_stride[2],
                      &h->mb.pic.p_fdec[2][2*y*h->mb.pic.i_stride[2]+2*x],           h->mb.pic.i_stride[2],
                      mvx, mvy, 2*width, 2*height );
}
static inline void x264_mb_mc_1xywh( x264_t *h, int x, int y, int width, int height )
{
    const int i8 = x264_scan8[0]+x+8*y;
    const int i_ref = h->mb.cache.ref[1][i8];
    const int mvx   = x264_clip3( h->mb.cache.mv[1][i8][0], h->mb.mv_min[0], h->mb.mv_max[0] );
    const int mvy   = x264_clip3( h->mb.cache.mv[1][i8][1], h->mb.mv_min[1], h->mb.mv_max[1] );

    h->mc.mc_luma( h->mb.pic.p_fref[1][i_ref], h->mb.pic.i_stride[0],
                    &h->mb.pic.p_fdec[0][4*y *h->mb.pic.i_stride[0]+4*x],            h->mb.pic.i_stride[0],
                    mvx + 4*4*x, mvy + 4*4*y, 4*width, 4*height );

    h->mc.mc_chroma( &h->mb.pic.p_fref[1][i_ref][4][2*y*h->mb.pic.i_stride[1]+2*x], h->mb.pic.i_stride[1],
                      &h->mb.pic.p_fdec[1][2*y*h->mb.pic.i_stride[1]+2*x],           h->mb.pic.i_stride[1],
                      mvx, mvy, 2*width, 2*height );

    h->mc.mc_chroma( &h->mb.pic.p_fref[1][i_ref][5][2*y*h->mb.pic.i_stride[2]+2*x], h->mb.pic.i_stride[2],
                      &h->mb.pic.p_fdec[2][2*y*h->mb.pic.i_stride[2]+2*x],           h->mb.pic.i_stride[2],
                      mvx, mvy, 2*width, 2*height );
}

static inline void x264_mb_mc_01xywh( x264_t *h, int x, int y, int width, int height )
{
    const int i8 = x264_scan8[0]+x+8*y;

    const int i_ref1 = h->mb.cache.ref[1][i8];
    const int mvx1   = x264_clip3( h->mb.cache.mv[1][i8][0], h->mb.mv_min[0], h->mb.mv_max[0] );
    const int mvy1   = x264_clip3( h->mb.cache.mv[1][i8][1], h->mb.mv_min[1], h->mb.mv_max[1] );
    DECLARE_ALIGNED( uint8_t, tmp[16*16], 16 );
    int i_mode = x264_size2pixel[height][width];

    x264_mb_mc_0xywh( h, x, y, width, height );

    h->mc.mc_luma( h->mb.pic.p_fref[1][i_ref1], h->mb.pic.i_stride[0],
                    tmp, 16, mvx1 + 4*4*x, mvy1 + 4*4*y, 4*width, 4*height );

    if( h->param.analyse.b_weighted_bipred )
    {
        const int i_ref0 = h->mb.cache.ref[0][i8];
        const int weight = h->mb.bipred_weight[i_ref0][i_ref1];

        h->pixf.avg_weight[i_mode]( &h->mb.pic.p_fdec[0][4*y *h->mb.pic.i_stride[0]+4*x], h->mb.pic.i_stride[0], tmp, 16, weight );

        h->mc.mc_chroma( &h->mb.pic.p_fref[1][i_ref1][4][2*y*h->mb.pic.i_stride[1]+2*x], h->mb.pic.i_stride[1],
                          tmp, 16, mvx1, mvy1, 2*width, 2*height );
        h->pixf.avg_weight[i_mode+3]( &h->mb.pic.p_fdec[1][2*y*h->mb.pic.i_stride[1]+2*x], h->mb.pic.i_stride[1], tmp, 16, weight );

        h->mc.mc_chroma( &h->mb.pic.p_fref[1][i_ref1][5][2*y*h->mb.pic.i_stride[2]+2*x], h->mb.pic.i_stride[2],
                          tmp, 16, mvx1, mvy1, 2*width, 2*height );
        h->pixf.avg_weight[i_mode+3]( &h->mb.pic.p_fdec[2][2*y*h->mb.pic.i_stride[2]+2*x], h->mb.pic.i_stride[2], tmp, 16, weight );
    }
    else
    {
        h->pixf.avg[i_mode]( &h->mb.pic.p_fdec[0][4*y *h->mb.pic.i_stride[0]+4*x], h->mb.pic.i_stride[0], tmp, 16 );

        h->mc.mc_chroma( &h->mb.pic.p_fref[1][i_ref1][4][2*y*h->mb.pic.i_stride[1]+2*x], h->mb.pic.i_stride[1],
                          tmp, 16, mvx1, mvy1, 2*width, 2*height );
        h->pixf.avg[i_mode+3]( &h->mb.pic.p_fdec[1][2*y*h->mb.pic.i_stride[1]+2*x], h->mb.pic.i_stride[1], tmp, 16 );

        h->mc.mc_chroma( &h->mb.pic.p_fref[1][i_ref1][5][2*y*h->mb.pic.i_stride[2]+2*x], h->mb.pic.i_stride[2],
                          tmp, 16, mvx1, mvy1, 2*width, 2*height );
        h->pixf.avg[i_mode+3]( &h->mb.pic.p_fdec[2][2*y*h->mb.pic.i_stride[2]+2*x], h->mb.pic.i_stride[2], tmp, 16 );
    }
}

static void x264_mb_mc_direct8x8( x264_t *h, int x, int y )
{
    const int i8 = x264_scan8[0] + x + 8*y;

    /* FIXME: optimize based on current block size, not global settings? */
    if( h->sps->b_direct8x8_inference )
    {
        if( h->mb.cache.ref[0][i8] >= 0 )
            if( h->mb.cache.ref[1][i8] >= 0 )
                x264_mb_mc_01xywh( h, x, y, 2, 2 );
            else
                x264_mb_mc_0xywh( h, x, y, 2, 2 );
        else
            x264_mb_mc_1xywh( h, x, y, 2, 2 );
    }
    else
    {
        if( h->mb.cache.ref[0][i8] >= 0 )
        {
            if( h->mb.cache.ref[1][i8] >= 0 )
            {
                x264_mb_mc_01xywh( h, x+0, y+0, 1, 1 );
                x264_mb_mc_01xywh( h, x+1, y+0, 1, 1 );
                x264_mb_mc_01xywh( h, x+0, y+1, 1, 1 );
                x264_mb_mc_01xywh( h, x+1, y+1, 1, 1 );
            }
            else
            {
                x264_mb_mc_0xywh( h, x+0, y+0, 1, 1 );
                x264_mb_mc_0xywh( h, x+1, y+0, 1, 1 );
                x264_mb_mc_0xywh( h, x+0, y+1, 1, 1 );
                x264_mb_mc_0xywh( h, x+1, y+1, 1, 1 );
            }
        }
        else
        {
            x264_mb_mc_1xywh( h, x+0, y+0, 1, 1 );
            x264_mb_mc_1xywh( h, x+1, y+0, 1, 1 );
            x264_mb_mc_1xywh( h, x+0, y+1, 1, 1 );
            x264_mb_mc_1xywh( h, x+1, y+1, 1, 1 );
        }
    }
}

void x264_mb_mc( x264_t *h )
{
    if( h->mb.i_type == P_L0 )
    {
        if( h->mb.i_partition == D_16x16 )
        {
            x264_mb_mc_0xywh( h, 0, 0, 4, 4 );
        }
        else if( h->mb.i_partition == D_16x8 )
        {
            x264_mb_mc_0xywh( h, 0, 0, 4, 2 );
            x264_mb_mc_0xywh( h, 0, 2, 4, 2 );
        }
        else if( h->mb.i_partition == D_8x16 )
        {
            x264_mb_mc_0xywh( h, 0, 0, 2, 4 );
            x264_mb_mc_0xywh( h, 2, 0, 2, 4 );
        }
    }
    else if( h->mb.i_type == P_8x8 || h->mb.i_type == B_8x8 )
    {
        int i;
        for( i = 0; i < 4; i++ )
        {
            const int x = 2*(i%2);
            const int y = 2*(i/2);
            switch( h->mb.i_sub_partition[i] )
            {
                case D_L0_8x8:
                    x264_mb_mc_0xywh( h, x, y, 2, 2 );
                    break;
                case D_L0_8x4:
                    x264_mb_mc_0xywh( h, x, y+0, 2, 1 );
                    x264_mb_mc_0xywh( h, x, y+1, 2, 1 );
                    break;
                case D_L0_4x8:
                    x264_mb_mc_0xywh( h, x+0, y, 1, 2 );
                    x264_mb_mc_0xywh( h, x+1, y, 1, 2 );
                    break;
                case D_L0_4x4:
                    x264_mb_mc_0xywh( h, x+0, y+0, 1, 1 );
                    x264_mb_mc_0xywh( h, x+1, y+0, 1, 1 );
                    x264_mb_mc_0xywh( h, x+0, y+1, 1, 1 );
                    x264_mb_mc_0xywh( h, x+1, y+1, 1, 1 );
                    break;
                case D_L1_8x8:
                    x264_mb_mc_1xywh( h, x, y, 2, 2 );
                    break;
                case D_L1_8x4:
                    x264_mb_mc_1xywh( h, x, y+0, 2, 1 );
                    x264_mb_mc_1xywh( h, x, y+1, 2, 1 );
                    break;
                case D_L1_4x8:
                    x264_mb_mc_1xywh( h, x+0, y, 1, 2 );
                    x264_mb_mc_1xywh( h, x+1, y, 1, 2 );
                    break;
                case D_L1_4x4:
                    x264_mb_mc_1xywh( h, x+0, y+0, 1, 1 );
                    x264_mb_mc_1xywh( h, x+1, y+0, 1, 1 );
                    x264_mb_mc_1xywh( h, x+0, y+1, 1, 1 );
                    x264_mb_mc_1xywh( h, x+1, y+1, 1, 1 );
                    break;
                case D_BI_8x8:
                    x264_mb_mc_01xywh( h, x, y, 2, 2 );
                    break;
                case D_BI_8x4:
                    x264_mb_mc_01xywh( h, x, y+0, 2, 1 );
                    x264_mb_mc_01xywh( h, x, y+1, 2, 1 );
                    break;
                case D_BI_4x8:
                    x264_mb_mc_01xywh( h, x+0, y, 1, 2 );
                    x264_mb_mc_01xywh( h, x+1, y, 1, 2 );
                    break;
                case D_BI_4x4:
                    x264_mb_mc_01xywh( h, x+0, y+0, 1, 1 );
                    x264_mb_mc_01xywh( h, x+1, y+0, 1, 1 );
                    x264_mb_mc_01xywh( h, x+0, y+1, 1, 1 );
                    x264_mb_mc_01xywh( h, x+1, y+1, 1, 1 );
                    break;
                case D_DIRECT_8x8:
                    x264_mb_mc_direct8x8( h, x, y );
                    break;
            }
        }
    }
    else if( h->mb.i_type == B_SKIP || h->mb.i_type == B_DIRECT )
    {
        x264_mb_mc_direct8x8( h, 0, 0 );
        x264_mb_mc_direct8x8( h, 2, 0 );
        x264_mb_mc_direct8x8( h, 0, 2 );
        x264_mb_mc_direct8x8( h, 2, 2 );
    }
    else    /* B_*x* */
    {
        int b_list0[2];
        int b_list1[2];

        int i;

        /* init ref list utilisations */
        for( i = 0; i < 2; i++ )
        {
            b_list0[i] = x264_mb_type_list0_table[h->mb.i_type][i];
            b_list1[i] = x264_mb_type_list1_table[h->mb.i_type][i];
        }
        if( h->mb.i_partition == D_16x16 )
        {
            if( b_list0[0] && b_list1[0] ) x264_mb_mc_01xywh( h, 0, 0, 4, 4 );
            else if( b_list0[0] )          x264_mb_mc_0xywh ( h, 0, 0, 4, 4 );
            else if( b_list1[0] )          x264_mb_mc_1xywh ( h, 0, 0, 4, 4 );
        }
        else if( h->mb.i_partition == D_16x8 )
        {
            if( b_list0[0] && b_list1[0] ) x264_mb_mc_01xywh( h, 0, 0, 4, 2 );
            else if( b_list0[0] )          x264_mb_mc_0xywh ( h, 0, 0, 4, 2 );
            else if( b_list1[0] )          x264_mb_mc_1xywh ( h, 0, 0, 4, 2 );

            if( b_list0[1] && b_list1[1] ) x264_mb_mc_01xywh( h, 0, 2, 4, 2 );
            else if( b_list0[1] )          x264_mb_mc_0xywh ( h, 0, 2, 4, 2 );
            else if( b_list1[1] )          x264_mb_mc_1xywh ( h, 0, 2, 4, 2 );
        }
        else if( h->mb.i_partition == D_8x16 )
        {
            if( b_list0[0] && b_list1[0] ) x264_mb_mc_01xywh( h, 0, 0, 2, 4 );
            else if( b_list0[0] )          x264_mb_mc_0xywh ( h, 0, 0, 2, 4 );
            else if( b_list1[0] )          x264_mb_mc_1xywh ( h, 0, 0, 2, 4 );

            if( b_list0[1] && b_list1[1] ) x264_mb_mc_01xywh( h, 2, 0, 2, 4 );
            else if( b_list0[1] )          x264_mb_mc_0xywh ( h, 2, 0, 2, 4 );
            else if( b_list1[1] )          x264_mb_mc_1xywh ( h, 2, 0, 2, 4 );
        }
    }
}

void x264_macroblock_cache_init( x264_t *h )
{
    int i, j;
    int i_mb_count = h->mb.i_mb_count;

    h->mb.i_mb_stride = h->sps->i_mb_width;
    h->mb.i_b8_stride = h->sps->i_mb_width * 2;
    h->mb.i_b4_stride = h->sps->i_mb_width * 4;

    h->mb.qp  = x264_malloc( i_mb_count * sizeof(int8_t) );
    h->mb.cbp = x264_malloc( i_mb_count * sizeof(int16_t) );
    h->mb.skipbp = x264_malloc( i_mb_count * sizeof(int8_t) );
    h->mb.mb_transform_size = x264_malloc( i_mb_count * sizeof(int8_t) );

    /* 0 -> 3 top(4), 4 -> 6 : left(3) */
    h->mb.intra4x4_pred_mode = x264_malloc( i_mb_count * 7 * sizeof( int8_t ) );

    /* all coeffs */
    h->mb.non_zero_count = x264_malloc( i_mb_count * 24 * sizeof( uint8_t ) );

    if( h->param.b_cabac )
    {
        h->mb.chroma_pred_mode = x264_malloc( i_mb_count * sizeof( int8_t) );
        h->mb.mvd[0] = x264_malloc( 2*16 * i_mb_count * sizeof( int16_t ) );
        h->mb.mvd[1] = x264_malloc( 2*16 * i_mb_count * sizeof( int16_t ) );
    }

    for( i=0; i<2; i++ )
    {
        int i_refs = i ? 1 + h->param.b_bframe_pyramid : h->param.i_frame_reference;
        for( j=0; j < i_refs; j++ )
            h->mb.mvr[i][j] = x264_malloc( 2 * i_mb_count * sizeof( int16_t ) );
    }

    /* init with not avaiable (for top right idx=7,15) */
    memset( h->mb.cache.ref[0], -2, X264_SCAN8_SIZE * sizeof( int8_t ) );
    memset( h->mb.cache.ref[1], -2, X264_SCAN8_SIZE * sizeof( int8_t ) );
}
void x264_macroblock_cache_end( x264_t *h )
{
    int i, j;
    for( i=0; i<2; i++ )
    {
        int i_refs = i ? 1 + h->param.b_bframe_pyramid : h->param.i_frame_reference;
        for( j=0; j < i_refs; j++ )
            x264_free( h->mb.mvr[i][j] );
    }
    if( h->param.b_cabac )
    {
        x264_free( h->mb.chroma_pred_mode );
        x264_free( h->mb.mvd[0] );
        x264_free( h->mb.mvd[1] );
    }
    x264_free( h->mb.intra4x4_pred_mode );
    x264_free( h->mb.non_zero_count );
    x264_free( h->mb.mb_transform_size );
    x264_free( h->mb.skipbp );
    x264_free( h->mb.cbp );
    x264_free( h->mb.qp );
}
void x264_macroblock_slice_init( x264_t *h )
{
    int i, j;

    h->mb.mv[0] = h->fdec->mv[0];
    h->mb.mv[1] = h->fdec->mv[1];
    h->mb.ref[0] = h->fdec->ref[0];
    h->mb.ref[1] = h->fdec->ref[1];
    h->mb.type = h->fdec->mb_type;

    h->fdec->i_ref[0] = h->i_ref0;
    h->fdec->i_ref[1] = h->i_ref1;
    for( i = 0; i < h->i_ref0; i++ )
        h->fdec->ref_poc[0][i] = h->fref0[i]->i_poc;
    if( h->sh.i_type == SLICE_TYPE_B )
    {
        for( i = 0; i < h->i_ref1; i++ )
            h->fdec->ref_poc[1][i] = h->fref1[i]->i_poc;

        h->mb.map_col_to_list0[-1] = -1;
        h->mb.map_col_to_list0[-2] = -2;
        for( i = 0; i < h->fref1[0]->i_ref[0]; i++ )
        {
            int poc = h->fref1[0]->ref_poc[0][i];
            h->mb.map_col_to_list0[i] = -2;
            for( j = 0; j < h->i_ref0; j++ )
                if( h->fref0[j]->i_poc == poc )
                {
                    h->mb.map_col_to_list0[i] = j;
                    break;
                }
        }
    }
    if( h->sh.i_type == SLICE_TYPE_P )
        memset( h->mb.cache.skip, 0, X264_SCAN8_SIZE * sizeof( int8_t ) );
}


void x264_macroblock_cache_load( x264_t *h, int i_mb_x, int i_mb_y )
{
    const int i_mb_4x4 = 4*(i_mb_y * h->mb.i_b4_stride + i_mb_x);
    const int i_mb_8x8 = 2*(i_mb_y * h->mb.i_b8_stride + i_mb_x);

    int i_mb_xy = i_mb_y * h->mb.i_mb_stride + i_mb_x;
    int i_top_xy = i_mb_xy - h->mb.i_mb_stride;
    int i_left_xy = -1;
    int i_top_type = -1;    /* gcc warn */
    int i_left_type= -1;

    int i;

    /* init index */
    h->mb.i_mb_x = i_mb_x;
    h->mb.i_mb_y = i_mb_y;
    h->mb.i_mb_xy = i_mb_xy;
    h->mb.i_b8_xy = i_mb_8x8;
    h->mb.i_b4_xy = i_mb_4x4;
    h->mb.i_neighbour = 0;

    /* load picture pointers */
    for( i = 0; i < 3; i++ )
    {
        const int w = (i == 0 ? 16 : 8);
        const int i_stride = h->fdec->i_stride[i];
        int   j;

        h->mb.pic.i_stride[i] = i_stride;

        h->mb.pic.p_fenc[i] = &h->fenc->plane[i][ w * ( i_mb_x + i_mb_y * i_stride )];
        h->mb.pic.p_fdec[i] = &h->fdec->plane[i][ w * ( i_mb_x + i_mb_y * i_stride )];

        for( j = 0; j < h->i_ref0; j++ )
        {
            h->mb.pic.p_fref[0][j][i==0 ? 0:i+3] = &h->fref0[j]->plane[i][ w * ( i_mb_x + i_mb_y * i_stride )];
            h->mb.pic.p_fref[0][j][i+1] = &h->fref0[j]->filtered[i+1][ 16 * ( i_mb_x + i_mb_y * h->fdec->i_stride[0] )];
        }
        for( j = 0; j < h->i_ref1; j++ )
        {
            h->mb.pic.p_fref[1][j][i==0 ? 0:i+3] = &h->fref1[j]->plane[i][ w * ( i_mb_x + i_mb_y * i_stride )];
            h->mb.pic.p_fref[1][j][i+1] = &h->fref1[j]->filtered[i+1][ 16 * ( i_mb_x + i_mb_y * h->fdec->i_stride[0] )];
        }
    }

    /* load cache */
    if( i_mb_xy >= h->sh.i_first_mb + h->mb.i_mb_stride )
    {
        h->mb.i_mb_type_top =
        i_top_type= h->mb.type[i_top_xy];

        h->mb.i_neighbour |= MB_TOP;

        /* load intra4x4 */
        h->mb.cache.intra4x4_pred_mode[x264_scan8[0] - 8] = h->mb.intra4x4_pred_mode[i_top_xy][0];
        h->mb.cache.intra4x4_pred_mode[x264_scan8[1] - 8] = h->mb.intra4x4_pred_mode[i_top_xy][1];
        h->mb.cache.intra4x4_pred_mode[x264_scan8[4] - 8] = h->mb.intra4x4_pred_mode[i_top_xy][2];
        h->mb.cache.intra4x4_pred_mode[x264_scan8[5] - 8] = h->mb.intra4x4_pred_mode[i_top_xy][3];

        /* load non_zero_count */
        h->mb.cache.non_zero_count[x264_scan8[0] - 8] = h->mb.non_zero_count[i_top_xy][10];
        h->mb.cache.non_zero_count[x264_scan8[1] - 8] = h->mb.non_zero_count[i_top_xy][11];
        h->mb.cache.non_zero_count[x264_scan8[4] - 8] = h->mb.non_zero_count[i_top_xy][14];
        h->mb.cache.non_zero_count[x264_scan8[5] - 8] = h->mb.non_zero_count[i_top_xy][15];

        h->mb.cache.non_zero_count[x264_scan8[16+0] - 8] = h->mb.non_zero_count[i_top_xy][16+2];
        h->mb.cache.non_zero_count[x264_scan8[16+1] - 8] = h->mb.non_zero_count[i_top_xy][16+3];

        h->mb.cache.non_zero_count[x264_scan8[16+4+0] - 8] = h->mb.non_zero_count[i_top_xy][16+4+2];
        h->mb.cache.non_zero_count[x264_scan8[16+4+1] - 8] = h->mb.non_zero_count[i_top_xy][16+4+3];
    }
    else
    {
        h->mb.i_mb_type_top = -1;
        
        /* load intra4x4 */
        h->mb.cache.intra4x4_pred_mode[x264_scan8[0] - 8] =
        h->mb.cache.intra4x4_pred_mode[x264_scan8[1] - 8] =
        h->mb.cache.intra4x4_pred_mode[x264_scan8[4] - 8] =
        h->mb.cache.intra4x4_pred_mode[x264_scan8[5] - 8] = -1;

        /* load non_zero_count */
        h->mb.cache.non_zero_count[x264_scan8[0] - 8] =
        h->mb.cache.non_zero_count[x264_scan8[1] - 8] =
        h->mb.cache.non_zero_count[x264_scan8[4] - 8] =
        h->mb.cache.non_zero_count[x264_scan8[5] - 8] =
        h->mb.cache.non_zero_count[x264_scan8[16+0] - 8] =
        h->mb.cache.non_zero_count[x264_scan8[16+1] - 8] =
        h->mb.cache.non_zero_count[x264_scan8[16+4+0] - 8] =
        h->mb.cache.non_zero_count[x264_scan8[16+4+1] - 8] = 0x80;

    }

    if( i_mb_x > 0 && i_mb_xy > h->sh.i_first_mb )
    {
        i_left_xy = i_mb_xy - 1;
        h->mb.i_mb_type_left =
        i_left_type = h->mb.type[i_left_xy];

        h->mb.i_neighbour |= MB_LEFT;

        /* load intra4x4 */
        h->mb.cache.intra4x4_pred_mode[x264_scan8[0 ] - 1] = h->mb.intra4x4_pred_mode[i_left_xy][4];
        h->mb.cache.intra4x4_pred_mode[x264_scan8[2 ] - 1] = h->mb.intra4x4_pred_mode[i_left_xy][5];
        h->mb.cache.intra4x4_pred_mode[x264_scan8[8 ] - 1] = h->mb.intra4x4_pred_mode[i_left_xy][6];
        h->mb.cache.intra4x4_pred_mode[x264_scan8[10] - 1] = h->mb.intra4x4_pred_mode[i_left_xy][3];

        /* load non_zero_count */
        h->mb.cache.non_zero_count[x264_scan8[0 ] - 1] = h->mb.non_zero_count[i_left_xy][5];
        h->mb.cache.non_zero_count[x264_scan8[2 ] - 1] = h->mb.non_zero_count[i_left_xy][7];
        h->mb.cache.non_zero_count[x264_scan8[8 ] - 1] = h->mb.non_zero_count[i_left_xy][13];
        h->mb.cache.non_zero_count[x264_scan8[10] - 1] = h->mb.non_zero_count[i_left_xy][15];

        h->mb.cache.non_zero_count[x264_scan8[16+0] - 1] = h->mb.non_zero_count[i_left_xy][16+1];
        h->mb.cache.non_zero_count[x264_scan8[16+2] - 1] = h->mb.non_zero_count[i_left_xy][16+3];

        h->mb.cache.non_zero_count[x264_scan8[16+4+0] - 1] = h->mb.non_zero_count[i_left_xy][16+4+1];
        h->mb.cache.non_zero_count[x264_scan8[16+4+2] - 1] = h->mb.non_zero_count[i_left_xy][16+4+3];
    }
    else
    {
        h->mb.i_mb_type_left = -1;

        h->mb.cache.intra4x4_pred_mode[x264_scan8[0 ] - 1] =
        h->mb.cache.intra4x4_pred_mode[x264_scan8[2 ] - 1] =
        h->mb.cache.intra4x4_pred_mode[x264_scan8[8 ] - 1] =
        h->mb.cache.intra4x4_pred_mode[x264_scan8[10] - 1] = -1;

        /* load non_zero_count */
        h->mb.cache.non_zero_count[x264_scan8[0 ] - 1] =
        h->mb.cache.non_zero_count[x264_scan8[2 ] - 1] =
        h->mb.cache.non_zero_count[x264_scan8[8 ] - 1] =
        h->mb.cache.non_zero_count[x264_scan8[10] - 1] =
        h->mb.cache.non_zero_count[x264_scan8[16+0] - 1] =
        h->mb.cache.non_zero_count[x264_scan8[16+2] - 1] =
        h->mb.cache.non_zero_count[x264_scan8[16+4+0] - 1] =
        h->mb.cache.non_zero_count[x264_scan8[16+4+2] - 1] = 0x80;
    }

    if( i_mb_x < h->sps->i_mb_width - 1 && i_top_xy + 1 >= h->sh.i_first_mb )
    {
        h->mb.i_neighbour |= MB_TOPRIGHT;
        h->mb.i_mb_type_topright = h->mb.type[ i_top_xy + 1 ];
    }
    else
        h->mb.i_mb_type_topright = -1;
    if( i_mb_x > 0 && i_top_xy - 1 >= h->sh.i_first_mb )
    {
        h->mb.i_neighbour |= MB_TOPLEFT;
        h->mb.i_mb_type_topleft = h->mb.type[ i_top_xy - 1 ];
    }
    else
        h->mb.i_mb_type_topleft = -1;

    if( h->param.analyse.b_transform_8x8 )
    {
        h->mb.cache.i_neighbour_transform_size =
            ( i_left_type >= 0 && h->mb.mb_transform_size[i_left_xy] )
          + ( i_top_type  >= 0 && h->mb.mb_transform_size[i_top_xy]  );
    }

    /* load ref/mv/mvd */
    if( h->sh.i_type != SLICE_TYPE_I )
    {
        const int s8x8 = h->mb.i_b8_stride;
        const int s4x4 = h->mb.i_b4_stride;

        int i_list;

        for( i_list = 0; i_list < (h->sh.i_type == SLICE_TYPE_B ? 2  : 1 ); i_list++ )
        {
            /*
            h->mb.cache.ref[i_list][x264_scan8[5 ]+1] =
            h->mb.cache.ref[i_list][x264_scan8[7 ]+1] =
            h->mb.cache.ref[i_list][x264_scan8[13]+1] = -2;
            */

            if( h->mb.i_neighbour & MB_TOPLEFT )
            {
                const int i8 = x264_scan8[0] - 1 - 1*8;
                const int ir = i_mb_8x8 - s8x8 - 1;
                const int iv = i_mb_4x4 - s4x4 - 1;
                h->mb.cache.ref[i_list][i8]  = h->mb.ref[i_list][ir];
                h->mb.cache.mv[i_list][i8][0] = h->mb.mv[i_list][iv][0];
                h->mb.cache.mv[i_list][i8][1] = h->mb.mv[i_list][iv][1];
            }
            else
            {
                const int i8 = x264_scan8[0] - 1 - 1*8;
                h->mb.cache.ref[i_list][i8] = -2;
                h->mb.cache.mv[i_list][i8][0] = 0;
                h->mb.cache.mv[i_list][i8][1] = 0;
            }

            if( h->mb.i_neighbour & MB_TOP )
            {
                const int i8 = x264_scan8[0] - 8;
                const int ir = i_mb_8x8 - s8x8;
                const int iv = i_mb_4x4 - s4x4;

                h->mb.cache.ref[i_list][i8+0] =
                h->mb.cache.ref[i_list][i8+1] = h->mb.ref[i_list][ir + 0];
                h->mb.cache.ref[i_list][i8+2] =
                h->mb.cache.ref[i_list][i8+3] = h->mb.ref[i_list][ir + 1];

                for( i = 0; i < 4; i++ )
                {
                    h->mb.cache.mv[i_list][i8+i][0] = h->mb.mv[i_list][iv + i][0];
                    h->mb.cache.mv[i_list][i8+i][1] = h->mb.mv[i_list][iv + i][1];
                }
            }
            else
            {
                const int i8 = x264_scan8[0] - 8;
                for( i = 0; i < 4; i++ )
                {
                    h->mb.cache.ref[i_list][i8+i] = -2;
                    h->mb.cache.mv[i_list][i8+i][0] =
                    h->mb.cache.mv[i_list][i8+i][1] = 0;
                }
            }

            if( h->mb.i_neighbour & MB_TOPRIGHT )
            {
                const int i8 = x264_scan8[0] + 4 - 1*8;
                const int ir = i_mb_8x8 - s8x8 + 2;
                const int iv = i_mb_4x4 - s4x4 + 4;

                h->mb.cache.ref[i_list][i8]  = h->mb.ref[i_list][ir];
                h->mb.cache.mv[i_list][i8][0] = h->mb.mv[i_list][iv][0];
                h->mb.cache.mv[i_list][i8][1] = h->mb.mv[i_list][iv][1];
            }
            else
            {
                const int i8 = x264_scan8[0] + 4 - 1*8;
                h->mb.cache.ref[i_list][i8] = -2;
                h->mb.cache.mv[i_list][i8][0] = 0;
                h->mb.cache.mv[i_list][i8][1] = 0;
            }

            if( h->mb.i_neighbour & MB_LEFT )
            {
                const int i8 = x264_scan8[0] - 1;
                const int ir = i_mb_8x8 - 1;
                const int iv = i_mb_4x4 - 1;

                h->mb.cache.ref[i_list][i8+0*8] =
                h->mb.cache.ref[i_list][i8+1*8] = h->mb.ref[i_list][ir + 0*s8x8];
                h->mb.cache.ref[i_list][i8+2*8] =
                h->mb.cache.ref[i_list][i8+3*8] = h->mb.ref[i_list][ir + 1*s8x8];

                for( i = 0; i < 4; i++ )
                {
                    h->mb.cache.mv[i_list][i8+i*8][0] = h->mb.mv[i_list][iv + i*s4x4][0];
                    h->mb.cache.mv[i_list][i8+i*8][1] = h->mb.mv[i_list][iv + i*s4x4][1];
                }
            }
            else
            {
                const int i8 = x264_scan8[0] - 1;
                for( i = 0; i < 4; i++ )
                {
                    h->mb.cache.ref[i_list][i8+i*8] = -2;
                    h->mb.cache.mv[i_list][i8+i*8][0] =
                    h->mb.cache.mv[i_list][i8+i*8][1] = 0;
                }
            }

            if( h->param.b_cabac )
            {
                if( i_top_type >= 0 )
                {
                    const int i8 = x264_scan8[0] - 8;
                    const int iv = i_mb_4x4 - s4x4;
                    for( i = 0; i < 4; i++ )
                    {
                        h->mb.cache.mvd[i_list][i8+i][0] = h->mb.mvd[i_list][iv + i][0];
                        h->mb.cache.mvd[i_list][i8+i][1] = h->mb.mvd[i_list][iv + i][1];
                    }
                }
                else
                {
                    const int i8 = x264_scan8[0] - 8;
                    for( i = 0; i < 4; i++ )
                    {
                        h->mb.cache.mvd[i_list][i8+i][0] =
                        h->mb.cache.mvd[i_list][i8+i][1] = 0;
                    }
                }

                if( i_left_type >= 0 )
                {
                    const int i8 = x264_scan8[0] - 1;
                    const int iv = i_mb_4x4 - 1;
                    for( i = 0; i < 4; i++ )
                    {
                        h->mb.cache.mvd[i_list][i8+i*8][0] = h->mb.mvd[i_list][iv + i*s4x4][0];
                        h->mb.cache.mvd[i_list][i8+i*8][1] = h->mb.mvd[i_list][iv + i*s4x4][1];
                    }
                }
                else
                {
                    const int i8 = x264_scan8[0] - 1;
                    for( i = 0; i < 4; i++ )
                    {
                        h->mb.cache.mvd[i_list][i8+i*8][0] =
                        h->mb.cache.mvd[i_list][i8+i*8][1] = 0;
                    }
                }
            }
        }

        /* load skip */
        if( h->sh.i_type == SLICE_TYPE_B && h->param.b_cabac )
        {
            memset( h->mb.cache.skip, 0, X264_SCAN8_SIZE * sizeof( int8_t ) );
            if( i_left_type >= 0 )
            {
                h->mb.cache.skip[x264_scan8[0] - 1] = h->mb.skipbp[i_left_xy] & 0x2;
                h->mb.cache.skip[x264_scan8[8] - 1] = h->mb.skipbp[i_left_xy] & 0x8;
            }
            if( i_top_type >= 0 )
            {
                h->mb.cache.skip[x264_scan8[0] - 8] = h->mb.skipbp[i_top_xy] & 0x4;
                h->mb.cache.skip[x264_scan8[4] - 8] = h->mb.skipbp[i_top_xy] & 0x8;
            }
        }
    }

    h->mb.i_neighbour4[0] =
    h->mb.i_neighbour8[0] = (h->mb.i_neighbour & (MB_TOP|MB_LEFT|MB_TOPLEFT))
                            | ((h->mb.i_neighbour & MB_TOP) ? MB_TOPRIGHT : 0);
    h->mb.i_neighbour4[4] =
    h->mb.i_neighbour4[1] = MB_LEFT | ((h->mb.i_neighbour & MB_TOP) ? (MB_TOP|MB_TOPLEFT|MB_TOPRIGHT) : 0);
    h->mb.i_neighbour4[2] =
    h->mb.i_neighbour4[8] =
    h->mb.i_neighbour4[10] =
    h->mb.i_neighbour8[2] = MB_TOP|MB_TOPRIGHT | ((h->mb.i_neighbour & MB_LEFT) ? (MB_LEFT|MB_TOPLEFT) : 0);
    h->mb.i_neighbour4[3] =
    h->mb.i_neighbour4[7] =
    h->mb.i_neighbour4[11] =
    h->mb.i_neighbour4[13] =
    h->mb.i_neighbour4[15] =
    h->mb.i_neighbour8[3] = MB_LEFT|MB_TOP|MB_TOPLEFT;
    h->mb.i_neighbour4[5] =
    h->mb.i_neighbour8[1] = MB_LEFT | (h->mb.i_neighbour & MB_TOPRIGHT)
                            | ((h->mb.i_neighbour & MB_TOP) ? MB_TOP|MB_TOPLEFT : 0);
    h->mb.i_neighbour4[6] =
    h->mb.i_neighbour4[9] =
    h->mb.i_neighbour4[12] =
    h->mb.i_neighbour4[14] = MB_LEFT|MB_TOP|MB_TOPLEFT|MB_TOPRIGHT;
}

void x264_macroblock_cache_save( x264_t *h )
{
    const int i_mb_xy = h->mb.i_mb_xy;
    const int i_mb_type = x264_mb_type_fix[h->mb.i_type];
    const int s8x8 = h->mb.i_b8_stride;
    const int s4x4 = h->mb.i_b4_stride;
    const int i_mb_4x4 = h->mb.i_b4_xy;
    const int i_mb_8x8 = h->mb.i_b8_xy;

    int i;

    h->mb.type[i_mb_xy] = i_mb_type;

    if( IS_SKIP( h->mb.i_type ) )
        h->mb.qp[i_mb_xy] = h->mb.i_last_qp;

    h->mb.i_last_dqp = h->mb.qp[i_mb_xy] - h->mb.i_last_qp;
    h->mb.i_last_qp = h->mb.qp[i_mb_xy];

    /* save intra4x4 */
    if( i_mb_type == I_4x4 )
    {
        h->mb.intra4x4_pred_mode[i_mb_xy][0] = h->mb.cache.intra4x4_pred_mode[x264_scan8[10] ];
        h->mb.intra4x4_pred_mode[i_mb_xy][1] = h->mb.cache.intra4x4_pred_mode[x264_scan8[11] ];
        h->mb.intra4x4_pred_mode[i_mb_xy][2] = h->mb.cache.intra4x4_pred_mode[x264_scan8[14] ];
        h->mb.intra4x4_pred_mode[i_mb_xy][3] = h->mb.cache.intra4x4_pred_mode[x264_scan8[15] ];
        h->mb.intra4x4_pred_mode[i_mb_xy][4] = h->mb.cache.intra4x4_pred_mode[x264_scan8[5] ];
        h->mb.intra4x4_pred_mode[i_mb_xy][5] = h->mb.cache.intra4x4_pred_mode[x264_scan8[7] ];
        h->mb.intra4x4_pred_mode[i_mb_xy][6] = h->mb.cache.intra4x4_pred_mode[x264_scan8[13] ];
    }
    else
    {
        h->mb.intra4x4_pred_mode[i_mb_xy][0] =
        h->mb.intra4x4_pred_mode[i_mb_xy][1] =
        h->mb.intra4x4_pred_mode[i_mb_xy][2] =
        h->mb.intra4x4_pred_mode[i_mb_xy][3] =
        h->mb.intra4x4_pred_mode[i_mb_xy][4] =
        h->mb.intra4x4_pred_mode[i_mb_xy][5] =
        h->mb.intra4x4_pred_mode[i_mb_xy][6] = I_PRED_4x4_DC;
    }

    if( i_mb_type == I_PCM )
    {
        h->mb.cbp[i_mb_xy] = 0x72f;   /* all set */
        for( i = 0; i < 16 + 2*4; i++ )
        {
            h->mb.non_zero_count[i_mb_xy][i] = 16;
        }
    }
    else
    {
        /* save non zero count */
        for( i = 0; i < 16 + 2*4; i++ )
        {
            h->mb.non_zero_count[i_mb_xy][i] = h->mb.cache.non_zero_count[x264_scan8[i]];
        }
    }

    h->mb.mb_transform_size[i_mb_xy] = h->mb.b_transform_8x8;

    if( !IS_INTRA( i_mb_type ) )
    {
        int i_list;
        for( i_list = 0; i_list < (h->sh.i_type == SLICE_TYPE_B ? 2  : 1 ); i_list++ )
        {
            int y,x;

            h->mb.ref[i_list][i_mb_8x8+0+0*s8x8] = h->mb.cache.ref[i_list][x264_scan8[0]];
            h->mb.ref[i_list][i_mb_8x8+1+0*s8x8] = h->mb.cache.ref[i_list][x264_scan8[4]];
            h->mb.ref[i_list][i_mb_8x8+0+1*s8x8] = h->mb.cache.ref[i_list][x264_scan8[8]];
            h->mb.ref[i_list][i_mb_8x8+1+1*s8x8] = h->mb.cache.ref[i_list][x264_scan8[12]];

            for( y = 0; y < 4; y++ )
            {
                for( x = 0; x < 4; x++ )
                {
                    h->mb.mv[i_list][i_mb_4x4+x+y*s4x4][0] = h->mb.cache.mv[i_list][x264_scan8[0]+x+8*y][0];
                    h->mb.mv[i_list][i_mb_4x4+x+y*s4x4][1] = h->mb.cache.mv[i_list][x264_scan8[0]+x+8*y][1];
                }
            }
        }
    }
    else
    {
        int i_list;
        for( i_list = 0; i_list < (h->sh.i_type == SLICE_TYPE_B ? 2  : 1 ); i_list++ )
        {
            int y,x;

            h->mb.ref[i_list][i_mb_8x8+0+0*s8x8] =
            h->mb.ref[i_list][i_mb_8x8+1+0*s8x8] =
            h->mb.ref[i_list][i_mb_8x8+0+1*s8x8] =
            h->mb.ref[i_list][i_mb_8x8+1+1*s8x8] = -1;

            for( y = 0; y < 4; y++ )
            {
                for( x = 0; x < 4; x++ )
                {
                    h->mb.mv[i_list][i_mb_4x4+x+y*s4x4][0] = 0;
                    h->mb.mv[i_list][i_mb_4x4+x+y*s4x4][1] = 0;
                }
            }
        }
    }

    if( h->param.b_cabac )
    {
        if( i_mb_type == I_4x4 || i_mb_type == I_16x16 )
            h->mb.chroma_pred_mode[i_mb_xy] = x264_mb_pred_mode8x8c_fix[ h->mb.i_chroma_pred_mode ];
        else
            h->mb.chroma_pred_mode[i_mb_xy] = I_PRED_CHROMA_DC;

        if( !IS_INTRA( i_mb_type ) && !IS_SKIP( i_mb_type ) && !IS_DIRECT( i_mb_type ) )
        {
            int i_list;
            for( i_list  = 0; i_list < 2; i_list++ )
            {
                const int s4x4 = 4 * h->mb.i_mb_stride;
                int y,x;
                for( y = 0; y < 4; y++ )
                {
                    for( x = 0; x < 4; x++ )
                    {
                        h->mb.mvd[i_list][i_mb_4x4+x+y*s4x4][0] = h->mb.cache.mvd[i_list][x264_scan8[0]+x+8*y][0];
                        h->mb.mvd[i_list][i_mb_4x4+x+y*s4x4][1] = h->mb.cache.mvd[i_list][x264_scan8[0]+x+8*y][1];
                    }
                }
            }
        }
        else
        {
            int i_list;
            for( i_list  = 0; i_list < 2; i_list++ )
            {
                const int s4x4 = 4 * h->mb.i_mb_stride;
                int y,x;
                for( y = 0; y < 4; y++ )
                {
                    for( x = 0; x < 4; x++ )
                    {
                        h->mb.mvd[i_list][i_mb_4x4+x+y*s4x4][0] = 0;
                        h->mb.mvd[i_list][i_mb_4x4+x+y*s4x4][1] = 0;
                    }
                }
            }
        }
        if( h->sh.i_type == SLICE_TYPE_B )
        {
            if( i_mb_type == B_SKIP || i_mb_type == B_DIRECT )
                h->mb.skipbp[i_mb_xy] = 0xf;
            else if( i_mb_type == B_8x8 )
            {
                int skipbp = 0;
                for( i = 0; i < 4; i++ )
                    skipbp |= ( h->mb.i_sub_partition[i] == D_DIRECT_8x8 ) << i;
                h->mb.skipbp[i_mb_xy] = skipbp;
            }
            else
                h->mb.skipbp[i_mb_xy] = 0;
        }
    }
}

void x264_macroblock_bipred_init( x264_t *h )
{
    int i_ref0, i_ref1;
    for( i_ref0 = 0; i_ref0 < h->i_ref0; i_ref0++ )
    {
        int poc0 = h->fref0[i_ref0]->i_poc;
        for( i_ref1 = 0; i_ref1 < h->i_ref1; i_ref1++ )
        {
            int dist_scale_factor;
            int poc1 = h->fref1[i_ref1]->i_poc;
            int td = x264_clip3( poc1 - poc0, -128, 127 );
            if( td == 0 /* || pic0 is a long-term ref */ )
                dist_scale_factor = 256;
            else
            {
                int tb = x264_clip3( h->fdec->i_poc - poc0, -128, 127 );
                int tx = (16384 + (abs(td) >> 1)) / td;
                dist_scale_factor = x264_clip3( (tb * tx + 32) >> 6, -1024, 1023 );
            }
            h->mb.dist_scale_factor[i_ref0][i_ref1] = dist_scale_factor;

            dist_scale_factor >>= 2;
            if( h->param.analyse.b_weighted_bipred
                  && dist_scale_factor >= -64
                  && dist_scale_factor <= 128 )
                h->mb.bipred_weight[i_ref0][i_ref1] = 64 - dist_scale_factor;
            else
                h->mb.bipred_weight[i_ref0][i_ref1] = 32;
        }
    }
}

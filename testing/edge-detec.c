/*****************************************************************************
 * macroblock.c: h264 encoder library
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar
 * $Id: edge-detec.c,v 1.1 2004/06/03 19:27:08 fenrir Exp $
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
#include <math.h>

#include "common.h"
#include "me.h"
#include "vlc.h"

static inline int x264_median( int a, int b, int c )
{
    int min = a, max =a;
    if( b < min )
    {
        min = b;
    }
    else
    {
        max = b;    /* no need to do 'b > max' (more consuming than always doing affectation) */
    }
    if( c < min )
    {
        min = c;
    }
    else if( c > max )
    {
        max = c;
    }

    return a + b + c - min - max;
}

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

static const int dequant_mf[6][4][4] =
{
    { {10, 13, 10, 13}, {13, 16, 13, 16}, {10, 13, 10, 13}, {13, 16, 13, 16} },
    { {11, 14, 11, 14}, {14, 18, 14, 18}, {11, 14, 11, 14}, {14, 18, 14, 18} },
    { {13, 16, 13, 16}, {16, 20, 16, 20}, {13, 16, 13, 16}, {16, 20, 16, 20} },
    { {14, 18, 14, 18}, {18, 23, 18, 23}, {14, 18, 14, 18}, {18, 23, 18, 23} },
    { {16, 20, 16, 20}, {20, 25, 20, 25}, {16, 20, 16, 20}, {20, 25, 20, 25} },
    { {18, 23, 18, 23}, {23, 29, 23, 29}, {18, 23, 18, 23}, {23, 29, 23, 29} }
};


static int predict_pred_intra4x4_mode( x264_t *h, x264_macroblock_t *mb, int idx )
{
    x264_macroblock_t *mba = mb->context->block[idx].mba;
    x264_macroblock_t *mbb = mb->context->block[idx].mbb;

    int i_mode_a = I_PRED_4x4_DC;
    int i_mode_b = I_PRED_4x4_DC;

    if( !mba || !mbb )
    {
        return I_PRED_4x4_DC;
    }

    if( mba->i_type == I_4x4 )
    {
        i_mode_a = mb->context->block[idx].bka->i_intra4x4_pred_mode;
    }
    if( mbb->i_type == I_4x4 )
    {
        i_mode_b = mb->context->block[idx].bkb->i_intra4x4_pred_mode;
    }

    return X264_MIN( i_mode_a, i_mode_b );
}

static int predict_non_zero_code( x264_t *h, x264_macroblock_t *mb, int idx )
{
    x264_macroblock_t *mba = mb->context->block[idx].mba;
    x264_macroblock_t *mbb = mb->context->block[idx].mbb;

    int i_z_a = 0x80, i_z_b = 0x80;
    int i_ret;

    /* none avail -> 0, one avail -> this one, both -> (a+b+1)>>1 */
    if( mba )
    {
        i_z_a = mb->context->block[idx].bka->i_non_zero_count;
    }
    if( mbb )
    {
        i_z_b = mb->context->block[idx].bkb->i_non_zero_count;
    }

    i_ret = i_z_a+i_z_b;
    if( i_ret < 0x80 )
    {
        i_ret = ( i_ret + 1 ) >> 1;
    }
    return i_ret & 0x7f;
}


/*
 * Handle intra mb
 */
/* Max = 4 */
static void predict_16x16_mode_available( x264_macroblock_t *mb, int *mode, int *pi_count )
{
    if( ( mb->i_neighbour & (MB_LEFT|MB_TOP) ) == (MB_LEFT|MB_TOP) )
    {
        /* top and left avaible */
        *mode++ = I_PRED_16x16_DC;
        *mode++ = I_PRED_16x16_V;
        *mode++ = I_PRED_16x16_H;
        *mode++ = I_PRED_16x16_P;
        *pi_count = 4;
    }
    else if( ( mb->i_neighbour & MB_LEFT ) )
    {
        /* left available*/
        *mode++ = I_PRED_16x16_DC_LEFT;
        *mode++ = I_PRED_16x16_H;
        *pi_count = 2;
    }
    else if( ( mb->i_neighbour & MB_TOP ) )
    {
        /* top available*/
        *mode++ = I_PRED_16x16_DC_TOP;
        *mode++ = I_PRED_16x16_V;
        *pi_count = 2;
    }
    else
    {
        /* none avaible */
        *mode = I_PRED_16x16_DC_128;
        *pi_count = 1;
    }
}

/* Max = 4 */
static void predict_8x8_mode_available( x264_macroblock_t *mb, int *mode, int *pi_count )
{
    if( ( mb->i_neighbour & (MB_LEFT|MB_TOP) ) == (MB_LEFT|MB_TOP) )
    {
        /* top and left avaible */
        *mode++ = I_PRED_CHROMA_DC;
        *mode++ = I_PRED_CHROMA_V;
        *mode++ = I_PRED_CHROMA_H;
        *mode++ = I_PRED_CHROMA_P;
        *pi_count = 4;
    }
    else if( ( mb->i_neighbour & MB_LEFT ) )
    {
        /* left available*/
        *mode++ = I_PRED_CHROMA_DC_LEFT;
        *mode++ = I_PRED_CHROMA_H;
        *pi_count = 2;
    }
    else if( ( mb->i_neighbour & MB_TOP ) )
    {
        /* top available*/
        *mode++ = I_PRED_CHROMA_DC_TOP;
        *mode++ = I_PRED_CHROMA_V;
        *pi_count = 2;
    }
    else
    {
        /* none avaible */
        *mode = I_PRED_CHROMA_DC_128;
        *pi_count = 1;
    }
}

/* MAX = 8 */
static void predict_4x4_mode_available( x264_macroblock_t *mb, int idx, int *mode, int *pi_count )
{
    int b_a, b_b, b_c;
    static const int needmb[16] =
    {
        MB_LEFT|MB_TOP, MB_TOP,
        MB_LEFT,        MB_PRIVATE,
        MB_TOP,         MB_TOP|MB_TOPRIGHT,
        0,              MB_PRIVATE,
        MB_LEFT,        0,
        MB_LEFT,        MB_PRIVATE,
        0,              MB_PRIVATE,
        0,              MB_PRIVATE
    };

    /* FIXME even when b_c == 0 there is some case where missing pixels
     * are emulated and thus more mode are available TODO
     * analysis and encode should be fixed too */
    b_a = (needmb[idx]&mb->i_neighbour&MB_LEFT) == (needmb[idx]&MB_LEFT);
    b_b = (needmb[idx]&mb->i_neighbour&MB_TOP) == (needmb[idx]&MB_TOP);
    b_c = (needmb[idx]&mb->i_neighbour&(MB_TOPRIGHT|MB_PRIVATE)) == (needmb[idx]&(MB_TOPRIGHT|MB_PRIVATE));

    if( b_a && b_b )
    {
        *mode++ = I_PRED_4x4_DC;
        *mode++ = I_PRED_4x4_H;
        *mode++ = I_PRED_4x4_V;
        *mode++ = I_PRED_4x4_DDR;
        *mode++ = I_PRED_4x4_VR;
        *mode++ = I_PRED_4x4_HD;
        *mode++ = I_PRED_4x4_HU;

        *pi_count = 7;

        if( b_c )
        {
            *mode++ = I_PRED_4x4_DDL;
            *mode++ = I_PRED_4x4_VL;
            (*pi_count) += 2;
        }
    }
    else if( b_a && !b_b )
    {
        *mode++ = I_PRED_4x4_DC_LEFT;
        *mode++ = I_PRED_4x4_H;
        *pi_count = 2;
    }
    else if( !b_a && b_b )
    {
        *mode++ = I_PRED_4x4_DC_TOP;
        *mode++ = I_PRED_4x4_V;
        *pi_count = 2;
    }
    else
    {
        *mode++ = I_PRED_4x4_DC_128;
        *pi_count = 1;
    }
}

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


static void quant_4x4( int16_t dct[4][4], int i_qscale, int b_intra )
{
    int i_qbits = 15 + i_qscale / 6;
    int i_mf = i_qscale % 6;
    int f = ( 1 << i_qbits ) / ( b_intra ? 3 : 6 );

    int x,y;
    for( y = 0; y < 4; y++ )
    {
        for( x = 0; x < 4; x++ )
        {
            if( dct[y][x] > 0 )
            {
                dct[y][x] =( f + (int64_t)dct[y][x]  * (int64_t)quant_mf[i_mf][y][x] ) >> i_qbits;
            }
            else
            {
                dct[y][x] = - ( ( f - (int64_t)dct[y][x]  * (int64_t)quant_mf[i_mf][y][x] ) >> i_qbits );
            }
        }
    }
}
static void quant_4x4_dc( int16_t dct[4][4], int i_qscale, int b_intra )
{
    int i_qbits = 15 + i_qscale / 6;
    int i_mf = i_qscale % 6;
    int f = ( 1 << i_qbits ) / ( b_intra ? 3 : 6 );

    int x,y;
    for( y = 0; y < 4; y++ )
    {
        for( x = 0; x < 4; x++ )
        {
            if( dct[y][x] > 0 )
            {
                dct[y][x] =( 2*f + (int64_t)dct[y][x]  * (int64_t)quant_mf[i_mf][0][0] ) >> ( 1 + i_qbits );
            }
            else
            {
                dct[y][x] = - ( ( 2*f - (int64_t)dct[y][x]  * (int64_t)quant_mf[i_mf][0][0] ) >> (1 + i_qbits ) );
            }
        }
    }
}
static void quant_2x2_dc( int16_t dct[2][2], int i_qscale, int b_intra )
{
    int i_qbits = 15 + i_qscale / 6;
    int i_mf = i_qscale % 6;
    int f = ( 1 << i_qbits ) / ( b_intra ? 3 : 6 );

    int x,y;
    for( y = 0; y < 2; y++ )
    {
        for( x = 0; x < 2; x++ )
        {
            /* XXX: is int64_t really needed ? */
            if( dct[y][x] > 0 )
            {
                dct[y][x] =( 2*f + (int64_t)dct[y][x]  * (int64_t)quant_mf[i_mf][0][0] ) >> ( 1 + i_qbits );
            }
            else
            {
                dct[y][x] = - ( ( 2*f - (int64_t)dct[y][x]  * (int64_t)quant_mf[i_mf][0][0] ) >> (1 + i_qbits ) );
            }
        }
    }
}

static void dequant_4x4_dc( int16_t dct[4][4], int i_qscale )
{
    int i_mf = i_qscale%6;
    int i_qbits = i_qscale/6;
    int f;
    int x,y;

    if( i_qbits <= 1 )
    {
        f = 1 << ( 1 - i_qbits );
    }
    else
    {
        f = 0;
    }

    for( y = 0; y < 4; y++ )
    {
        for( x = 0; x < 4; x++ )
        {
            if( i_qbits >= 2 )
            {
                dct[y][x] = ( dct[y][x] * dequant_mf[i_mf][0][0] ) << (i_qbits - 2);
            }
            else
            {
                dct[y][x] = ( dct[y][x] * dequant_mf[i_mf][0][0] + f ) >> ( 2 -i_qbits );
            }
        }
    }
}

static void dequant_2x2_dc( int16_t dct[2][2], int i_qscale )
{
    int i_mf = i_qscale%6;
    int i_qbits = i_qscale/6;
    int x,y;

    for( y = 0; y < 2; y++ )
    {
        for( x = 0; x < 2; x++ )
        {
            if( i_qbits >= 1 )
            {
                dct[y][x] = ( dct[y][x] * dequant_mf[i_mf][0][0] ) << (i_qbits - 1);
            }
            else
            {
                dct[y][x] = ( dct[y][x] * dequant_mf[i_mf][0][0] ) >> 1;
            }
        }
    }
}
static void dequant_4x4( int16_t dct[4][4], int i_qscale )
{
    int i_mf = i_qscale%6;
    int i_qbits = i_qscale/6;
    int x,y;

    for( y = 0; y < 4; y++ )
    {
        for( x = 0; x < 4; x++ )
        {
            dct[y][x] = ( dct[y][x] * dequant_mf[i_mf][x][y] ) << i_qbits;
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

/* TODO : use a table instead */
static int mb_partition_count( int i_partition )
{
    switch( i_partition )
    {
        case D_8x8:
            return 4;
        case D_16x8:
        case D_8x16:
            return 2;
        case D_16x16:
            return 1;
        default:
            /* should never occur */
            return 0;
    }
}

static int mb_sub_partition_count( int i_partition )
{
    switch( i_partition )
    {
        case D_L0_4x4:
        case D_L1_4x4:
        case D_BI_4x4:
            return 4;
        case D_L0_4x8:
        case D_L1_4x8:
        case D_BI_4x8:
        case D_L0_8x4:
        case D_L1_8x4:
        case D_BI_8x4:
            return 2;
        case D_L0_8x8:
        case D_L1_8x8:
        case D_BI_8x8:
        case D_DIRECT_8x8:
            return 1;
        default:
            /* should never occur */
            return 0;
    }
}

static inline void x264_macroblock_partition_getxy( x264_macroblock_t *mb, int i_part, int i_sub, int *x, int *y )
{
    if( mb->i_partition == D_16x16 )
    {
        *x  = 0;
        *y  = 0;
    }
    else if( mb->i_partition == D_16x8 )
    {
        *x = 0;
        *y = 2*i_part;
    }
    else if( mb->i_partition == D_8x16 )
    {
        *x = 2*i_part;
        *y = 0;
    }
    else if( mb->i_partition == D_8x8 )
    {
        *x = 2 * (i_part%2);
        *y = 2 * (i_part/2);

        if( IS_SUB4x4( mb->i_sub_partition[i_part] ) )
        {
            (*x) += i_sub%2;
            (*y) += i_sub/2;
        }
        else if( IS_SUB4x8( mb->i_sub_partition[i_part] ) )
        {
            (*x) += i_sub;
        }
        else if( IS_SUB8x4( mb->i_sub_partition[i_part] ) )
        {
            (*y) += i_sub;
        }
    }
}
static inline void x264_macroblock_partition_size( x264_macroblock_t *mb, int i_part, int i_sub, int *w, int *h )
{
    if( mb->i_partition == D_16x16 )
    {
        *w  = 4;
        *h  = 4;
    }
    else if( mb->i_partition == D_16x8 )
    {
        *w = 4;
        *h = 2;
    }
    else if( mb->i_partition == D_8x16 )
    {
        *w = 2;
        *h = 4;
    }
    else if( mb->i_partition == D_8x8 )
    {
        if( IS_SUB4x4( mb->i_sub_partition[i_part] ) )
        {
            *w = 1;
            *h = 1;
        }
        else if( IS_SUB4x8( mb->i_sub_partition[i_part] ) )
        {
            *w = 1;
            *h = 2;
        }
        else if( IS_SUB8x4( mb->i_sub_partition[i_part] ) )
        {
            *w = 2;
            *h = 1;
        }
        else
        {
            *w = 2;
            *h = 2;
        }
    }
}

void x264_macroblock_partition_set( x264_macroblock_t *mb, int i_list, int i_part, int i_sub, int i_ref, int mx, int my )
{
    int x,  y;
    int w,  h;
    int dx, dy;

    x264_macroblock_partition_getxy( mb, i_part, i_sub, &x, &y );
    x264_macroblock_partition_size ( mb, i_part, i_sub, &w, &h );

    for( dx = 0; dx < w; dx++ )
    {
        for( dy = 0; dy < h; dy++ )
        {
            mb->partition[x+dx][y+dy].i_ref[i_list] = i_ref;
            mb->partition[x+dx][y+dy].mv[i_list][0] = mx;
            mb->partition[x+dx][y+dy].mv[i_list][1] = my;
        }
    }
}

void x264_macroblock_partition_get( x264_macroblock_t *mb, int i_list, int i_part, int i_sub, int *pi_ref, int *pi_mx, int *pi_my )
{
    int x,y;

    x264_macroblock_partition_getxy( mb, i_part, i_sub, &x, &y );

    if( pi_ref )
    {
        *pi_ref = mb->partition[x][y].i_ref[i_list];
    }
    if( pi_mx && pi_my )
    {
        *pi_mx  = mb->partition[x][y].mv[i_list][0];
        *pi_my  = mb->partition[x][y].mv[i_list][1];
    }
}

/* ARrrrg so unbeautifull, and unoptimised for common case */
void x264_macroblock_predict_mv( x264_macroblock_t *mb, int i_list, int i_part, int i_subpart, int *mvxp, int *mvyp )
{
    int x, y, xn, yn;
    int w, h;
    int i_ref;

    int i_refa = -1;
    int i_refb = -1;
    int i_refc = -1;

    int mvxa = 0, mvxb = 0, mvxc = 0;
    int mvya = 0, mvyb = 0, mvyc = 0;

    x264_macroblock_t *mbn;


    x264_macroblock_partition_getxy( mb, i_part, i_subpart, &x, &y );
    x264_macroblock_partition_size( mb, i_part, i_subpart, &w, &h );
    i_ref = mb->partition[x][y].i_ref[i_list];

    /* Left  pixel (-1,0)*/
    xn = x - 1;
    mbn = mb;
    if( xn < 0 )
    {
        xn += 4;
        mbn = mb->mba;
    }
    if( mbn )
    {
        i_refa = -2;
        if( !IS_INTRA( mbn->i_type ) )
        {
            i_refa = mbn->partition[xn][y].i_ref[i_list];
            mvxa   = mbn->partition[xn][y].mv[i_list][0];
            mvya   = mbn->partition[xn][y].mv[i_list][1];
        }
    }

    /* Up ( pixel(0,-1)*/
    yn = y - 1;
    mbn = mb;
    if( yn < 0 )
    {
        yn += 4;
        mbn = mb->mbb;
    }
    if( mbn )
    {
        i_refb = -2;
        if( !IS_INTRA( mbn->i_type ) )
        {
            i_refb = mbn->partition[x][yn].i_ref[i_list];
            mvxb   = mbn->partition[x][yn].mv[i_list][0];
            mvyb   = mbn->partition[x][yn].mv[i_list][1];
        }
    }

    /* Up right pixel(width,-1)*/
    xn = x + w;
    yn = y - 1;

    mbn = mb;
    if( yn < 0 && xn >= 4 )
    {
        if( mb->mbc )
        {
            xn -= 4;
            yn += 4;
            mbn = mb->mbc;
        }
        else
        {
            mbn = NULL;
        }
    }
    else if( yn < 0 )
    {
        yn += 4;
        mbn = mb->mbb;
    }
    else if( xn >= 4 || ( xn == 2 && ( yn == 0 || yn == 2 ) ) )
    {
        mbn = NULL; /* not yet decoded */
    }

    if( mbn == NULL )
    {
        /* load top left pixel(-1,-1) */
        xn = x - 1;
        yn = y - 1;

        mbn = mb;
        if( yn < 0 && xn < 0 )
        {
            if( mb->mba && mb->mbb )
            {
                xn += 4;
                yn += 4;
                mbn = mb->mbb - 1;
            }
            else
            {
                mbn = NULL;
            }
        }
        else if( yn < 0 )
        {
            yn += 4;
            mbn = mb->mbb;
        }
        else if( xn < 0 )
        {
            xn += 4;
            mbn = mb->mba;
        }
    }

    if( mbn )
    {
        i_refc = -2;
        if( !IS_INTRA( mbn->i_type ) )
        {
            i_refc = mbn->partition[xn][yn].i_ref[i_list];
            mvxc   = mbn->partition[xn][yn].mv[i_list][0];
            mvyc   = mbn->partition[xn][yn].mv[i_list][1];
        }
    }

    if( mb->i_partition == D_16x8 && i_part == 0 && i_refb == i_ref )
    {
        *mvxp = mvxb;
        *mvyp = mvyb;
    }
    else if( mb->i_partition == D_16x8 && i_part == 1 && i_refa == i_ref )
    {
        *mvxp = mvxa;
        *mvyp = mvya;
    }
    else if( mb->i_partition == D_8x16 && i_part == 0 && i_refa == i_ref )
    {
        *mvxp = mvxa;
        *mvyp = mvya;
    }
    else if( mb->i_partition == D_8x16 && i_part == 1 && i_refc == i_ref )
    {
        *mvxp = mvxc;
        *mvyp = mvyc;
    }
    else
    {
        int i_count;

        i_count = 0;
        if( i_refa == i_ref ) i_count++;
        if( i_refb == i_ref ) i_count++;
        if( i_refc == i_ref ) i_count++;

        if( i_count > 1 )
        {
            *mvxp = x264_median( mvxa, mvxb, mvxc );
            *mvyp = x264_median( mvya, mvyb, mvyc );
        }
        else if( i_count == 1 )
        {
            if( i_refa == i_ref )
            {
                *mvxp = mvxa;
                *mvyp = mvya;
            }
            else if( i_refb == i_ref )
            {
                *mvxp = mvxb;
                *mvyp = mvyb;
            }
            else
            {
                *mvxp = mvxc;
                *mvyp = mvyc;
            }
        }
        else if( i_refb == -1 && i_refc == -1 && i_refa != -1 )
        {
            *mvxp = mvxa;
            *mvyp = mvya;
        }
        else
        {
            *mvxp = x264_median( mvxa, mvxb, mvxc );
            *mvyp = x264_median( mvya, mvyb, mvyc );
        }
    }
}

void x264_macroblock_predict_mv_pskip( x264_macroblock_t *mb, int *mvxp, int *mvyp )
{
    int x, y, xn, yn;

    int i_refa = -1;
    int i_refb = -1;

    int mvxa = 0, mvxb = 0;
    int mvya = 0, mvyb = 0;

    x264_macroblock_t *mbn;


    x264_macroblock_partition_getxy( mb, 0, 0, &x, &y );

    /* Left  pixel (-1,0)*/
    xn = x - 1;
    mbn = mb;
    if( xn < 0 )
    {
        xn += 4;
        mbn = mb->mba;
    }
    if( mbn )
    {
        i_refa = -2;
        if( !IS_INTRA( mbn->i_type ) )
        {
            i_refa = mbn->partition[xn][y].i_ref[0];
            mvxa   = mbn->partition[xn][y].mv[0][0];
            mvya   = mbn->partition[xn][y].mv[0][1];
        }
    }

    /* Up ( pixel(0,-1)*/
    yn = y - 1;
    mbn = mb;
    if( yn < 0 )
    {
        yn += 4;
        mbn = mb->mbb;
    }
    if( mbn )
    {
        i_refb = -2;
        if( !IS_INTRA( mbn->i_type ) )
        {
            i_refb = mbn->partition[x][yn].i_ref[0];
            mvxb   = mbn->partition[x][yn].mv[0][0];
            mvyb   = mbn->partition[x][yn].mv[0][1];
        }
    }

    if( i_refa == -1 || i_refb == -1 ||
        ( i_refa == 0 && mvxa == 0 && mvya == 0 ) ||
        ( i_refb == 0 && mvxb == 0 && mvyb == 0 ) )
    {
        *mvxp = 0;
        *mvyp = 0;
    }
    else
    {
        x264_macroblock_predict_mv( mb, 0, 0, 0, mvxp, mvyp );
    }
}

static const int i_chroma_qp_table[52] =
{
     0,  1,  2,  3,  4,  5,  6,  7,  8,  9,
    10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
    20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
    29, 30, 31, 32, 32, 33, 34, 34, 35, 35,
    36, 36, 37, 37, 37, 38, 38, 38, 39, 39,
    39, 39
};

static void x264_macroblock_mc( x264_t *h, x264_macroblock_t *mb, int b_luma )
{
    x264_mb_context_t *ctx = mb->context;

    int ch;
    int i_ref;
    int mx, my;

    if( mb->i_type == P_L0 )
    {
        int i_part;

        for( i_part = 0; i_part < mb_partition_count( mb->i_partition ); i_part++ )
        {
            int i_width, i_height;
            int x, y;

            x264_macroblock_partition_get( mb, 0, i_part, 0, &i_ref, &mx, &my );
            x264_macroblock_partition_getxy( mb, i_part, 0, &x, &y );
            x264_macroblock_partition_size(  mb, i_part, 0, &i_width, &i_height );

            if( b_luma )
            {
                int     i_src = ctx->i_fref0[i_ref][0];
                uint8_t *p_src= ctx->p_fref0[i_ref][0];
                int     i_dst = ctx->i_fdec[0];
                uint8_t *p_dst= ctx->p_fdec[0];

                h->mc[MC_LUMA]( &p_src[4*(x+y*i_src)], i_src,
                                &p_dst[4*(x+y*i_dst)], i_dst,
                                mx, my, 4*i_width, 4*i_height );
            }
            else
            {
                int     i_src,  i_dst;
                uint8_t *p_src, *p_dst;

                for( ch = 0; ch < 2; ch++ )
                {
                    i_src = ctx->i_fref0[i_ref][1+ch];
                    p_src = ctx->p_fref0[i_ref][1+ch];
                    i_dst = ctx->i_fdec[1+ch];
                    p_dst = ctx->p_fdec[1+ch];

                    h->mc[MC_CHROMA]( &p_src[2*(x+y*i_src)], i_src,
                                      &p_dst[2*(x+y*i_dst)], i_dst,
                                      mx, my, 2*i_width, 2*i_height );
                }
            }
        }
    }
    else if( mb->i_type == P_8x8 )
    {
        int i_part;

        for( i_part = 0; i_part < 4; i_part++ )
        {
            int i_sub;

            for( i_sub = 0; i_sub < mb_sub_partition_count( mb->i_sub_partition[i_part] ); i_sub++ )
            {
                int i_width, i_height;
                int x, y;

                x264_macroblock_partition_get(   mb, 0, i_part, i_sub, &i_ref, &mx, &my );
                x264_macroblock_partition_getxy( mb, i_part, i_sub, &x, &y );
                x264_macroblock_partition_size(  mb, i_part, i_sub, &i_width, &i_height );

                if( b_luma )
                {
                    int     i_src = ctx->i_fref0[i_ref][0];
                    uint8_t *p_src= ctx->p_fref0[i_ref][0];
                    int     i_dst = ctx->i_fdec[0];
                    uint8_t *p_dst= ctx->p_fdec[0];

                    h->mc[MC_LUMA]( &p_src[4*(x+y*i_src)], i_src,
                                    &p_dst[4*(x+y*i_dst)], i_dst,
                                    mx, my, 4*i_width, 4*i_height );
                }
                else
                {
                    int     i_src,  i_dst;
                    uint8_t *p_src, *p_dst;

                    for( ch = 0; ch < 2; ch++ )
                    {
                        i_src = ctx->i_fref0[i_ref][1+ch];
                        p_src = ctx->p_fref0[i_ref][1+ch];
                        i_dst = ctx->i_fdec[1+ch];
                        p_dst = ctx->p_fdec[1+ch];

                        h->mc[MC_CHROMA]( &p_src[2*(x+y*i_src)], i_src,
                                          &p_dst[2*(x+y*i_dst)], i_dst,
                                          mx, my, 2*i_width, 2*i_height );
                    }
                }
            }
        }
    }
}

/*****************************************************************************
 * x264_macroblock_neighbour_load:
 *****************************************************************************/
void x264_macroblock_context_load( x264_t *h, x264_macroblock_t *mb, x264_mb_context_t *context )
{
    int i;
    int x, y;
    x264_macroblock_t *a = NULL;
    x264_macroblock_t *b = NULL;

    if( mb->i_neighbour&MB_LEFT )
    {
        a = mb - 1;
    }
    if( mb->i_neighbour&MB_TOP )
    {
        b = mb - h->sps.i_mb_width;
    }
#define LOAD_PTR( dst, src ) \
    context->p_##dst[0] = (src)->plane[0] + 16 * ( mb->i_mb_x + mb->i_mb_y * (src)->i_stride[0] ); \
    context->p_##dst[1] = (src)->plane[1] +  8 * ( mb->i_mb_x + mb->i_mb_y * (src)->i_stride[1] ); \
    context->p_##dst[2] = (src)->plane[2] +  8 * ( mb->i_mb_x + mb->i_mb_y * (src)->i_stride[2] ); \
    context->i_##dst[0] = (src)->i_stride[0]; \
    context->i_##dst[1] = (src)->i_stride[1]; \
    context->i_##dst[2] = (src)->i_stride[2]

    LOAD_PTR( img,  h->picture );
    LOAD_PTR( fdec, h->fdec );
    for( i = 0; i < h->i_ref0; i++ )
    {
        LOAD_PTR( fref0[i], h->fref0[i] );
    }
    for( i = 0; i < h->i_ref1; i++ )
    {
        LOAD_PTR( fref1[i], h->fref1[i] );
    }
#undef LOAD_PTR

    for( y = 0; y < 4; y++ )
    {
        for( x = 0; x < 4; x++ )
        {
            int idx;
            int xa, yb;
            x264_macroblock_t *mba;
            x264_macroblock_t *mbb;

            idx = block_idx_xy[x][y];
            mba = mb;
            mbb = mb;

            xa = x - 1;
            if (xa < 0 )
            {
                xa += 4;
                mba = a;
            }
            /* up */
            yb = y - 1;
            if (yb < 0 )
            {
                yb += 4;
                mbb = b;
            }

            context->block[idx].mba = mba;
            context->block[idx].mbb = mbb;
            context->block[idx].bka = mba ? &mba->block[block_idx_xy[xa][y]] : NULL;
            context->block[idx].bkb = mbb ? &mbb->block[block_idx_xy[x][yb]] : NULL;

            if( x < 2 && y < 2 )
            {
                int ch;
                if( xa > 1 ) xa -= 2;   /* we have wrap but here step is 2 not 4 */
                if( yb > 1 ) yb -= 2;   /* idem */

                for( ch = 0; ch < 2; ch++ )
                {
                    context->block[16+4*ch+idx].mba = mba;
                    context->block[16+4*ch+idx].mbb = mbb;
                    context->block[16+4*ch+idx].bka = mba ? &mba->block[16+4*ch+block_idx_xy[xa][y]] : NULL;
                    context->block[16+4*ch+idx].bkb = mbb ? &mbb->block[16+4*ch+block_idx_xy[x][yb]] : NULL;
                }
            }
        }
    }

    mb->context = context;
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

static void x264_mb_encode_4x4( x264_t *h, x264_macroblock_t *mb, int idx, int i_qscale )
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
    dequant_4x4( dct4x4, i_qscale );
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
    quant_4x4_dc( dct4x4[0], i_qscale, 1 );
    scan_zigzag_4x4full( mb->luma16x16_dc, dct4x4[0] );

    /* output samples to fdec */
    h->dctf.idct4x4dc( dct4x4[0], dct4x4[0] );
    dequant_4x4_dc( dct4x4[0], i_qscale );  /* XXX not inversed */

    /* calculate dct coeffs */
    for( i = 0; i < 16; i++ )
    {
        dequant_4x4( dct4x4[1+i], i_qscale );

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

            quant_4x4( dct4x4[i], i_qscale, 1 );
            scan_zigzag_4x4( mb->block[16+i+ch*4].residual_ac, dct4x4[i] );

            i_decimate_score += x264_mb_decimate_score( mb->block[16+i+ch*4].residual_ac, 15 );
        }

        h->dctf.dct2x2dc( dct2x2, dct2x2 );
        quant_2x2_dc( dct2x2, i_qscale, 1 );
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
        dequant_2x2_dc( dct2x2, i_qscale );  /* XXX not inversed */

        /* calculate dct coeffs */
        for( i = 0; i < 4; i++ )
        {
            dequant_4x4( dct4x4[i], i_qscale );

            /* copy dc coeff */
            dct4x4[i][0][0] = dct2x2[block_idx_y[i]][block_idx_x[i]];

            h->dctf.idct4x4( chroma[i], dct4x4[i] );
        }
        h->pixf.add8x8( p_dst, i_dst, chroma );
    }
}

static int x264_mb_pred_mode4x4_fix( int i_mode )
{
    if( i_mode == I_PRED_4x4_DC_LEFT || i_mode == I_PRED_4x4_DC_TOP || i_mode == I_PRED_4x4_DC_128 )
    {
        return I_PRED_4x4_DC;
    }
    return i_mode;
}
static int x264_mb_pred_mode16x16_fix( int i_mode )
{
    if( i_mode == I_PRED_16x16_DC_LEFT || i_mode == I_PRED_16x16_DC_TOP || i_mode == I_PRED_16x16_DC_128 )
    {
        return I_PRED_16x16_DC;
    }
    return i_mode;
}
static int x264_mb_pred_mode8x8_fix( int i_mode )
{
    if( i_mode == I_PRED_CHROMA_DC_LEFT || i_mode == I_PRED_CHROMA_DC_TOP || i_mode == I_PRED_CHROMA_DC_128 )
    {
        return I_PRED_CHROMA_DC;
    }
    return i_mode;
}

typedef struct
{
    /* conduct the analysis using this lamda and QP */
    int i_lambda;
    int i_qp;

    /* Edge histogramme (only luma) */
    int i_edge_4x4[4][4][9];    /* mode 2 isn't calculated (DC) */
    int i_edge_16x16[4];        /* mode 2 isn't calculated (DC) */

    /* I: Intra part */
    /* Luma part 16x16 and 4x4 modes stats */
    int i_sad_i16x16;
    int i_predict16x16;

    int i_sad_i4x4;
    int i_predict4x4[4][4];

    /* Chroma part */
    int i_sad_i8x8;
    int i_predict8x8;

    /* II: Inter part */
    int i_sad_p16x16;
    int i_ref_p16x16;
    int i_mv_p16x16[2];

    int i_sad_p16x8;
    int i_ref_p16x8;
    int i_mv_p16x8[2][2];

    int i_sad_p8x16;
    int i_ref_p8x16;
    int i_mv_p8x16[2][2];

    int i_sad_p8x8;
    int i_ref_p8x8;
    int i_sub_partition_p8x8[4];
    int i_mv_p8x8[4][4][2];

} x264_mb_analysis_t;


static const int i_qp0_cost_table[52] =
{
   1, 1, 1, 1, 1, 1, 1, 1,
   1, 1, 1, 1,
   1, 1, 1, 1, 2, 2, 2, 2,
   3, 3, 3, 4, 4, 4, 5, 6,
   6, 7, 8, 9,10,11,13,14,
  16,18,20,23,25,29,32,36,
  40,45,51,57,64,72,81,91
};


static void x264_macroblock_analyse_edge( x264_t *h, x264_macroblock_t *mb, x264_mb_analysis_t *res )
{
    uint8_t *p_img = mb->context->p_img[0];;
    int      i_img = mb->context->i_img[0];

    int dx, dy;
    int x,  y;
    int i;

#define FIX8( f ) ( (int)((f) * 256))
    /* init stats (16x16) */
    for( i = 0; i < 4; i++ )
    {
        res->i_edge_16x16[i] = 0;
    }

    for( y = 0; y < 4; y++ )
    {
        for( x = 0; x < 4; x++ )
        {
            /* init stats (4x4) */
            for( i = 0; i < 9; i++ )
            {
                res->i_edge_4x4[y][x][i] = 0;
            }

            /* FIXME real interval 0-4 except for border mb */
            for( dy = (y==0 ? 1:0); dy < (y==3?3:4); dy++ )
            {
                for( dx = (x==0?1:0); dx < (x==3?3:4); dx++ )
                {
                    uint8_t *pix = &p_img[(y*4+dy)*i_img+(x+dx)];
                    int dgx, dgy;
                    int Ryx;
                    int Ag;
                    int Dg;


                    dgx = (pix[-1*i_img-1]+2*pix[-1*i_img+0]+pix[-1*i_img+1]) -
                          (pix[ 1*i_img-1]+2*pix[ 1*i_img+0]+pix[ 1*i_img+1]);


                    dgy = (pix[-1*i_img+1]+2*pix[ 0*i_img+1]+pix[ 1*i_img+1]) -
                          (pix[-1*i_img-1]+2*pix[ 0*i_img-1]+pix[ 1*i_img-1]);

                    /* XXX angle to test/verify */
                    Ag = abs( dgx ) + abs( dgy );

                    if( dgx == 0 )
                    {
                        Ryx = (4*256)<<8;
                    }
                    else
                    {
                        Ryx = ( dgy << 8 )/ dgx;
                    }

                    if( abs(Ryx) >= FIX8(5.027339) )
                    {
                        Dg = I_PRED_4x4_V;
                    }
                    else if( abs(Ryx) <= FIX8(0.198912) )
                    {
                        Dg = I_PRED_4x4_H;
                    }
                    else if( Ryx > FIX8(0.198912) && Ryx <= FIX8(0.668179) )
                    {
                        Dg = I_PRED_4x4_HD;
                    }
                    else if( Ryx > FIX8(0.668179) && Ryx <= FIX8(1.496606) )
                    {
                        Dg = I_PRED_4x4_DDR;
                    }
                    else if( Ryx > FIX8(1.496606) && Ryx <= FIX8(5.027339) )
                    {
                        Dg = I_PRED_4x4_VR;
                    }
                    else if( Ryx > FIX8(-5.027339) && Ryx <= FIX8(-1.496606) )
                    {
                        Dg = I_PRED_4x4_VL;
                    }
                    else if( Ryx > FIX8(-1.496606) && Ryx <= FIX8(-0.668179) )
                    {
                        Dg = I_PRED_4x4_DDL;
                    }
                    else if( Ryx > FIX8(-0.668179) && Ryx <= FIX8(-0.198912) )
                    {
                        Dg = I_PRED_4x4_HU;
                    }
                    else
                    {
                        /* Should never occur */
                        fprintf( stderr, "mmh bad edge dectection function\n" );
                        Dg = I_PRED_4x4_DC;
                    }
                    res->i_edge_4x4[y][x][Dg] += Ag;

                    if( abs(Ryx) > FIX8(2.414214) )
                    {
                        Dg = I_PRED_16x16_V;
                    }
                    else if( abs(Ryx) < FIX8(0.414214) )
                    {
                        Dg = I_PRED_16x16_H;
                    }
                    else
                    {
                        Dg = I_PRED_16x16_P;
                    }
                    res->i_edge_16x16[Dg] += Ag;
                }
            }
        }
    }
#undef FIX8
}

static void x264_macroblock_analyse_i16x16( x264_t *h, x264_macroblock_t *mb, x264_mb_analysis_t *res )
{
    uint8_t *p_dst = mb->context->p_fdec[0];
    uint8_t *p_src = mb->context->p_img[0];
    int      i_dst = mb->context->i_fdec[0];
    int      i_src = mb->context->i_img[0];

    int i;
    int i_max;
    int predict_mode[4];

    res->i_sad_i16x16 = -1;

    /* 16x16 prediction selection */
    predict_16x16_mode_available( mb, predict_mode, &i_max );
    for( i = 0; i < i_max; i++ )
    {
        int i_sad;
        int i_mode;

        i_mode = predict_mode[i];

        /* we do the prediction */
        h->predict_16x16[i_mode]( p_dst, i_dst );

        /* we calculate the diff and get the square sum of the diff */
        i_sad = h->pixf.satd[PIXEL_16x16]( p_dst, i_dst, p_src, i_src ) +
                res->i_lambda * bs_size_ue( x264_mb_pred_mode16x16_fix(i_mode) );
        /* if i_score is lower it is better */
        if( res->i_sad_i16x16 == -1 || res->i_sad_i16x16 > i_sad )
        {
            res->i_predict16x16 = i_mode;
            res->i_sad_i16x16     = i_sad;
        }
    }
}

static void x264_macroblock_analyse_i4x4( x264_t *h, x264_macroblock_t *mb, x264_mb_analysis_t *res )
{
    int i, idx;

    int i_max;
    int predict_mode[9];

    uint8_t *p_dst = mb->context->p_fdec[0];
    uint8_t *p_src = mb->context->p_img[0];
    int      i_dst = mb->context->i_fdec[0];
    int      i_src = mb->context->i_img[0];

    res->i_sad_i4x4 = 0;

    /* 4x4 prediction selection */
    for( idx = 0; idx < 16; idx++ )
    {
        uint8_t *p_src_by;
        uint8_t *p_dst_by;
        int     i_best;
        int x, y;
        int i_pred_mode;
        int i_th;

        i_pred_mode= predict_pred_intra4x4_mode( h, mb, idx );
        x = block_idx_x[idx];
        y = block_idx_y[idx];

        i_th = res->i_edge_4x4[y][x][0];
        if( i_th < res->i_edge_4x4[y][x][1] ) i_th = res->i_edge_4x4[y][x][1];
        if( i_th < res->i_edge_4x4[y][x][3] ) i_th = res->i_edge_4x4[y][x][3];
        if( i_th < res->i_edge_4x4[y][x][4] ) i_th = res->i_edge_4x4[y][x][4];
        if( i_th < res->i_edge_4x4[y][x][5] ) i_th = res->i_edge_4x4[y][x][5];
        if( i_th < res->i_edge_4x4[y][x][6] ) i_th = res->i_edge_4x4[y][x][6];
        if( i_th < res->i_edge_4x4[y][x][7] ) i_th = res->i_edge_4x4[y][x][7];
        if( i_th < res->i_edge_4x4[y][x][8] ) i_th = res->i_edge_4x4[y][x][8];
        i_th /= 2;

        res->i_edge_4x4[y][x][2] = i_th;

        p_src_by = p_src + 4 * x + 4 * y * i_src;
        p_dst_by = p_dst + 4 * x + 4 * y * i_dst;

        i_best = -1;
        predict_4x4_mode_available( mb, idx, predict_mode, &i_max );
        for( i = 0; i < i_max; i++ )
        {
            int i_sad;
            int i_mode;
            int i_fmode;

            i_mode = predict_mode[i];
            i_fmode = x264_mb_pred_mode4x4_fix( i_mode );

            if( res->i_edge_4x4[y][x][i_fmode] < i_th )
            {
                continue;
            }

            /* we do the prediction */
            h->predict_4x4[i_mode]( p_dst_by, i_dst );

            /* we calculate diff and get the square sum of the diff */
            i_sad = h->pixf.satd[PIXEL_4x4]( p_dst_by, i_dst, p_src_by, i_src );

            i_sad += res->i_lambda * (i_pred_mode == i_fmode ? 1 : 4);

            /* if i_score is lower it is better */
            if( i_best == -1 || i_best > i_sad )
            {
                res->i_predict4x4[x][y] = i_mode;
                i_best = i_sad;
            }
        }
        res->i_sad_i4x4 += i_best;

        /* we need to encode this mb now (for next ones) */
        mb->block[idx].i_intra4x4_pred_mode = res->i_predict4x4[x][y];
        h->predict_4x4[res->i_predict4x4[x][y]]( p_dst_by, i_dst );
        x264_mb_encode_4x4( h, mb, idx, res->i_qp );
    }
    res->i_sad_i4x4 += res->i_lambda * 24;    /* from JVT (SATD0) */
}

static void x264_macroblock_analyse_intra_chroma( x264_t *h, x264_macroblock_t *mb, x264_mb_analysis_t *res )
{
    int i;

    int i_max;
    int predict_mode[9];

    uint8_t *p_dstc[2], *p_srcc[2];
    int      i_dstc[2], i_srcc[2];

    /* 8x8 prediction selection for chroma */
    p_dstc[0] = mb->context->p_fdec[1]; i_dstc[0] = mb->context->i_fdec[1];
    p_dstc[1] = mb->context->p_fdec[2]; i_dstc[1] = mb->context->i_fdec[2];
    p_srcc[0] = mb->context->p_img[1];  i_srcc[0] = mb->context->i_img[1];
    p_srcc[1] = mb->context->p_img[2];  i_srcc[1] = mb->context->i_img[2];

    predict_8x8_mode_available( mb, predict_mode, &i_max );
    res->i_sad_i8x8 = -1;
    for( i = 0; i < i_max; i++ )
    {
        int i_sad;
        int i_mode;

        i_mode = predict_mode[i];

        /* we do the prediction */
        h->predict_8x8[i_mode]( p_dstc[0], i_dstc[0] );
        h->predict_8x8[i_mode]( p_dstc[1], i_dstc[1] );

        /* we calculate the cost */
        i_sad = h->pixf.satd[PIXEL_8x8]( p_dstc[0], i_dstc[0], p_srcc[0], i_srcc[0] ) +
                h->pixf.satd[PIXEL_8x8]( p_dstc[1], i_dstc[1], p_srcc[1], i_srcc[1] ) +
                res->i_lambda * bs_size_ue( x264_mb_pred_mode8x8_fix(i_mode) );

        /* if i_score is lower it is better */
        if( res->i_sad_i8x8 == -1 || res->i_sad_i8x8 > i_sad )
        {
            res->i_predict8x8 = i_mode;
            res->i_sad_i8x8     = i_sad;
        }
    }
}

static void x264_macroblock_analyse_inter_p8x8( x264_t *h, x264_macroblock_t *mb, x264_mb_analysis_t *res )
{
    x264_mb_context_t *ctx = mb->context;
    int i_ref = res->i_ref_p16x16;

    uint8_t *p_fref = ctx->p_fref0[i_ref][0];
    int      i_fref = ctx->i_fref0[i_ref][0];
    uint8_t *p_img  = ctx->p_img[0];
    int      i_img  = ctx->i_img[0];

    int i;

    res->i_ref_p8x8 = i_ref;
    res->i_sad_p8x8 = 0;
    mb->i_partition = D_8x8;

    for( i = 0; i < 4; i++ )
    {
        static const int test8x8_mode[4] = { D_L0_8x8, D_L0_8x4, D_L0_4x8, D_L0_4x4 };
        static const int test8x8_pix[4]  = { PIXEL_8x8, PIXEL_8x4, PIXEL_4x8, PIXEL_4x4 };
        static const int test8x8_pos_x[4][4] = { { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 4, 0, 0 }, { 0, 4, 0, 4 } };
        static const int test8x8_pos_y[4][4] = { { 0, 0, 0, 0 }, { 0, 4, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 4, 4 } };
        int i_test;
        int mvp[4][2];
        int mv[4][2];

        int x, y;
        int i_sub;
        int i_b_satd;

        y = 8 * (i / 2);
        x = 8 * (i % 2);
        i_b_satd = -1;

        i_test = 0;
        /* FIXME as it's tooooooo slow test only 8x8 */
        //for( i_test = 0; i_test < 4; i_test++ )
        {
            int i_satd;

            i_satd = 0;

            mb->i_sub_partition[i] = test8x8_mode[i_test];

            for( i_sub = 0; i_sub < mb_sub_partition_count( test8x8_mode[i_test] ); i_sub++ )
            {
                x264_macroblock_predict_mv( mb, 0, i, i_sub, &mvp[i_sub][0], &mvp[i_sub][1] );
                mv[i_sub][0] = mvp[i_sub][0];
                mv[i_sub][1] = mvp[i_sub][1];

                i_satd += x264_me_p_umhexagons( h,
                                                &p_fref[(y+test8x8_pos_y[i_test][i_sub])*i_fref +x+test8x8_pos_x[i_test][i_sub]], i_fref,
                                                &p_img[(y+test8x8_pos_y[i_test][i_sub])*i_img +x+test8x8_pos_x[i_test][i_sub]], i_img,
                                                test8x8_pix[i_test],
                                                res->i_lambda,
                                                &mv[i_sub][0], &mv[i_sub][1] );
                i_satd += res->i_lambda * ( bs_size_se( mv[i_sub][0] - mvp[i_sub][0] ) +
                                            bs_size_se( mv[i_sub][1] - mvp[i_sub][1] ) );
            }

            switch( test8x8_mode[i_test] )
            {
                case D_L0_8x8:
                    i_satd += res->i_lambda * bs_size_ue( 0 );
                    break;
                case D_L0_8x4:
                    i_satd += res->i_lambda * bs_size_ue( 1 );
                    break;
                case D_L0_4x8:
                    i_satd += res->i_lambda * bs_size_ue( 2 );
                    break;
                case D_L0_4x4:
                    i_satd += res->i_lambda * bs_size_ue( 3 );
                    break;
                default:
                    fprintf( stderr, "internal error (invalid sub type)\n" );
                    break;
            }

            if( i_b_satd == -1 || i_b_satd > i_satd )
            {
                i_b_satd = i_satd;
                res->i_sub_partition_p8x8[i] = test8x8_mode[i_test];;
                for( i_sub = 0; i_sub < mb_sub_partition_count( test8x8_mode[i_test] ); i_sub++ )
                {
                    res->i_mv_p8x8[i][i_sub][0] = mv[i_sub][0];
                    res->i_mv_p8x8[i][i_sub][1] = mv[i_sub][1];
                }
            }
        }

        res->i_sad_p8x8 += i_b_satd;
        /* needed for the next block */
        mb->i_sub_partition[i] = res->i_sub_partition_p8x8[i];
        for( i_sub = 0; i_sub < mb_sub_partition_count( res->i_sub_partition_p8x8[i] ); i_sub++ )
        {
            x264_macroblock_partition_set( mb, 0, i, i_sub,
                                           res->i_ref_p8x8,
                                           res->i_mv_p8x8[i][i_sub][0],
                                           res->i_mv_p8x8[i][i_sub][1] );
        }
    }

    res->i_sad_p8x8 += 4*res->i_lambda * bs_size_te( h->sh.i_num_ref_idx_l0_active - 1, i_ref );
}

static void x264_macroblock_analyse_inter( x264_t *h, x264_macroblock_t *mb, x264_mb_analysis_t *res )
{
    x264_mb_context_t *ctx = mb->context;

    int i_ref;

    /* int res */
    res->i_sad_p16x16 = -1;
    res->i_sad_p16x8  = -1;
    res->i_sad_p8x16  = -1;
    res->i_sad_p8x8   = -1;

    /* 16x16 Search on all ref frame */
    mb->i_type = P_L0;  /* beurk fix that */
    mb->i_partition = D_16x16;
    for( i_ref = 0; i_ref < h->i_ref0; i_ref++ )
    {
        int i_sad;
        int mvxp, mvyp;
        int mvx, mvy;

        /* Get the predicted MV */
        x264_macroblock_partition_set( mb, 0, 0, 0, i_ref, 0, 0 );
        x264_macroblock_predict_mv( mb, 0, 0, 0, &mvxp, &mvyp );

        mvx = mvxp; mvy = mvyp;
        i_sad = x264_me_p_umhexagons( h, ctx->p_fref0[i_ref][0], ctx->i_fref0[i_ref][0],
                                         ctx->p_img[0],         ctx->i_img[0],
                                         PIXEL_16x16, res->i_lambda, &mvx, &mvy );
        if( mvx == mvxp && mvy == mvyp )
        {
            i_sad -= 16 * res->i_lambda;
        }
        else
        {
            i_sad += res->i_lambda * (bs_size_se(mvx - mvxp) + bs_size_se(mvy - mvyp));
        }
        i_sad += res->i_lambda * bs_size_te( h->sh.i_num_ref_idx_l0_active - 1, i_ref );

        if( res->i_sad_p16x16 == -1 || i_sad < res->i_sad_p16x16 )
        {
            res->i_sad_p16x16   = i_sad;
            res->i_ref_p16x16   = i_ref;
            res->i_mv_p16x16[0] = mvx;
            res->i_mv_p16x16[1] = mvy;
        }
    }

    /* Now do the rafinement (using the ref found in 16x16 mode) */
    i_ref = res->i_ref_p16x16;
    x264_macroblock_partition_set( mb, 0, 0, 0, i_ref, 0, 0 );

    /* try 16x8 */
    /* XXX we test i_predict16x16 to try shape with the same direction than edge
     * We should do a better algo of course (the one with edge dectection to be used
     * for intra mode too)
     * */

    if( res->i_predict16x16 != I_PRED_16x16_V )
    {
        int mvp[2][2];

        mb->i_partition = D_16x8;

        res->i_ref_p16x8   = i_ref;
        x264_macroblock_predict_mv( mb, 0, 0, 0, &mvp[0][0], &mvp[0][1] );
        x264_macroblock_predict_mv( mb, 0, 1, 0, &mvp[1][0], &mvp[1][1] );

        res->i_mv_p16x8[0][0] = mvp[0][0]; res->i_mv_p16x8[0][1] = mvp[0][1];
        res->i_mv_p16x8[1][0] = mvp[1][0]; res->i_mv_p16x8[1][1] = mvp[1][1];

        res->i_sad_p16x8 = x264_me_p_umhexagons( h,
                                                 ctx->p_fref0[i_ref][0], ctx->i_fref0[i_ref][0],
                                                 ctx->p_img[0],          ctx->i_img[0],
                                                 PIXEL_16x8,
                                                 res->i_lambda,
                                                 &res->i_mv_p16x8[0][0], &res->i_mv_p16x8[0][1] ) +
                           x264_me_p_umhexagons( h,
                                                 &ctx->p_fref0[i_ref][0][8*ctx->i_fref0[i_ref][0]], ctx->i_fref0[i_ref][0],
                                                 &ctx->p_img[0][8*ctx->i_img[0]],                   ctx->i_img[0],
                                                 PIXEL_16x8,
                                                 res->i_lambda,
                                                 &res->i_mv_p16x8[1][0], &res->i_mv_p16x8[1][1] );

        res->i_sad_p16x8 += res->i_lambda * ( bs_size_se(res->i_mv_p16x8[0][0] - mvp[0][0] ) +
                                              bs_size_se(res->i_mv_p16x8[0][1] - mvp[0][1] ) +
                                              bs_size_se(res->i_mv_p16x8[1][0] - mvp[1][0] ) +
                                              bs_size_se(res->i_mv_p16x8[1][1] - mvp[1][1] ) );

        res->i_sad_p16x8 += 2*res->i_lambda * bs_size_te( h->sh.i_num_ref_idx_l0_active - 1, i_ref );
    }

    /* try 8x16 */
    if( res->i_predict16x16 != I_PRED_16x16_H )
    {
        int mvp[2][2];

        mb->i_partition = D_8x16;

        res->i_ref_p8x16   = i_ref;
        x264_macroblock_predict_mv( mb, 0, 0, 0, &mvp[0][0], &mvp[0][1] );
        x264_macroblock_predict_mv( mb, 0, 1, 0, &mvp[1][0], &mvp[1][1] );

        res->i_mv_p8x16[0][0] = mvp[0][0]; res->i_mv_p8x16[0][1] = mvp[0][1];
        res->i_mv_p8x16[1][0] = mvp[1][0]; res->i_mv_p8x16[1][1] = mvp[1][1];

        res->i_sad_p8x16 = x264_me_p_umhexagons( h,
                                                 ctx->p_fref0[i_ref][0], ctx->i_fref0[i_ref][0],
                                                 ctx->p_img[0],          ctx->i_img[0],
                                                 PIXEL_8x16,
                                                 res->i_lambda,
                                                 &res->i_mv_p8x16[0][0], &res->i_mv_p8x16[0][1] ) +
                           x264_me_p_umhexagons( h,
                                                 &ctx->p_fref0[i_ref][0][8], ctx->i_fref0[i_ref][0],
                                                 &ctx->p_img[0][8],          ctx->i_img[0],
                                                 PIXEL_8x16,
                                                 res->i_lambda,
                                                 &res->i_mv_p8x16[1][0], &res->i_mv_p8x16[1][1] );

        res->i_sad_p8x16 += res->i_lambda * ( bs_size_se(res->i_mv_p8x16[0][0] - mvp[0][0] ) +
                                                bs_size_se(res->i_mv_p8x16[0][1] - mvp[0][1] ) +
                                                bs_size_se(res->i_mv_p8x16[1][0] - mvp[1][0] ) +
                                                bs_size_se(res->i_mv_p8x16[1][1] - mvp[1][1] ) );
        res->i_sad_p8x16 += 2*res->i_lambda * bs_size_te( h->sh.i_num_ref_idx_l0_active - 1, i_ref );
    }

    if( 1 )
    {
    //    x264_macroblock_analyse_inter_p8x8( h,mb, res );
    }
}

/*****************************************************************************
 * x264_macroblock_analyse:
 *****************************************************************************/
void x264_macroblock_analyse( x264_t *h, x264_macroblock_t *mb, int i_slice_type )
{
    x264_mb_analysis_t analysis;
    int i;

    /* qp TODO */
    mb->i_qp_delta = 0;

    /* init analysis */
    analysis.i_qp = x264_clip3( h->pps.i_pic_init_qp + h->sh.i_qp_delta + mb->i_qp_delta, 0, 51 );
    analysis.i_lambda = i_qp0_cost_table[analysis.i_qp];

    x264_macroblock_analyse_edge( h, mb, &analysis );

    /*--------------------------- Do the analysis ---------------------------*/
    x264_macroblock_analyse_i16x16( h, mb, &analysis );
    x264_macroblock_analyse_i4x4  ( h, mb, &analysis );
    if( i_slice_type == SLICE_TYPE_P )
    {
        x264_macroblock_analyse_inter( h, mb, &analysis );
    }

    /*-------------------- Chose the macroblock mode ------------------------*/
    /* Do the MB decision */
    if( i_slice_type == SLICE_TYPE_I )
    {
        mb->i_type = analysis.i_sad_i4x4 < analysis.i_sad_i16x16 ? I_4x4 : I_16x16;
    }
    else
    {
        int i_satd;
#define BEST_TYPE( type, partition, satd ) \
        if( satd != -1 && satd < i_satd ) \
        {   \
            i_satd = satd;  \
            mb->i_type = type; \
            mb->i_partition = partition; \
        }

        i_satd = analysis.i_sad_i4x4;
        mb->i_type = I_4x4;

        BEST_TYPE( I_16x16, -1,    analysis.i_sad_i16x16 );
        BEST_TYPE( P_L0,  D_16x16, analysis.i_sad_p16x16 );
        BEST_TYPE( P_L0,  D_16x8 , analysis.i_sad_p16x8  );
        BEST_TYPE( P_L0,  D_8x16 , analysis.i_sad_p8x16  );
        BEST_TYPE( P_8x8, D_8x8  , analysis.i_sad_p8x8   );

#undef BEST_TYPE
    }

    if( IS_INTRA( mb->i_type ) )
    {
        x264_macroblock_analyse_intra_chroma( h, mb, &analysis );
    }

    /*-------------------- Update MB from the analysis ----------------------*/
    switch( mb->i_type )
    {
        case I_4x4:
            for( i = 0; i < 16; i++ )
            {
                mb->block[i].i_intra4x4_pred_mode = analysis.i_predict4x4[block_idx_x[i]][block_idx_y[i]];
            }
            mb->i_chroma_pred_mode = analysis.i_predict8x8;
            break;
        case I_16x16:
            mb->i_intra16x16_pred_mode = analysis.i_predict16x16;
            mb->i_chroma_pred_mode = analysis.i_predict8x8;
            break;
        case P_L0:
            switch( mb->i_partition )
            {
                case D_16x16:
                    x264_macroblock_partition_set( mb, 0, 0, 0,
                                                   analysis.i_ref_p16x16, analysis.i_mv_p16x16[0], analysis.i_mv_p16x16[1] );
                    break;
                case D_16x8:
                    x264_macroblock_partition_set( mb, 0, 0, 0,
                                                   analysis.i_ref_p16x8, analysis.i_mv_p16x8[0][0], analysis.i_mv_p16x8[0][1] );
                    x264_macroblock_partition_set( mb, 0, 1, 0,
                                                   analysis.i_ref_p16x8, analysis.i_mv_p16x8[1][0], analysis.i_mv_p16x8[1][1] );
                    break;
                case D_8x16:
                    x264_macroblock_partition_set( mb, 0, 0, 0,
                                                   analysis.i_ref_p8x16, analysis.i_mv_p8x16[0][0], analysis.i_mv_p8x16[0][1] );
                    x264_macroblock_partition_set( mb, 0, 1, 0,
                                                   analysis.i_ref_p8x16, analysis.i_mv_p8x16[1][0], analysis.i_mv_p8x16[1][1] );
                    break;
                default:
                    fprintf( stderr, "internal error\n" );
                    break;
            }
            break;

        case P_8x8:
            for( i = 0; i < 4; i++ )
            {
                int i_sub;

                mb->i_sub_partition[i] = analysis.i_sub_partition_p8x8[i];
                for( i_sub = 0; i_sub < mb_sub_partition_count( mb->i_sub_partition[i] ); i_sub++ )
                {
                    x264_macroblock_partition_set( mb, 0, i, i_sub,
                                                   analysis.i_ref_p8x8,
                                                   analysis.i_mv_p8x8[i][i_sub][0],
                                                   analysis.i_mv_p8x8[i][i_sub][1] );
                }
            }
            break;

        default:
            fprintf( stderr, "internal error\n" );
            break;
    }
}



/*****************************************************************************
 * x264_macroblock_encode:
 *****************************************************************************/
void x264_macroblock_encode( x264_t *h, x264_macroblock_t *mb )
{
    int i;

    int     i_qscale;

    /* quantification scale */
    i_qscale = x264_clip3( h->pps.i_pic_init_qp + h->sh.i_qp_delta + mb->i_qp_delta, 0, 51 );

    if( mb->i_type == I_16x16 )
    {
        /* do the right prediction */
        h->predict_16x16[mb->i_intra16x16_pred_mode]( mb->context->p_fdec[0], mb->context->i_fdec[0] );

        /* encode the 16x16 macroblock */
        x264_mb_encode_i16x16( h, mb, i_qscale );

        /* fix the pred mode value */
        mb->i_intra16x16_pred_mode = x264_mb_pred_mode16x16_fix( mb->i_intra16x16_pred_mode );
    }
    else if( mb->i_type == I_4x4 )
    {
        for( i = 0; i < 16; i++ )
        {
            uint8_t *p_dst_by;

            /* Do the right prediction */
            p_dst_by = mb->context->p_fdec[0] + 4 * block_idx_x[i] + 4 * block_idx_y[i] * mb->context->i_fdec[0];
            h->predict_4x4[mb->block[i].i_intra4x4_pred_mode]( p_dst_by, mb->context->i_fdec[0] );

            /* encode one 4x4 block */
            x264_mb_encode_4x4( h, mb, i, i_qscale );

            /* fix the pred mode value */
            mb->block[i].i_intra4x4_pred_mode = x264_mb_pred_mode4x4_fix( mb->block[i].i_intra4x4_pred_mode );
        }
    }
    else    /* Inter MB */
    {
        x264_mb_context_t *ctx = mb->context;
        int16_t dct4x4[16][4][4];

        int i8x8, i4x4, idx;
        int i_decimate_mb = 0;

        /* Motion compensation */
        x264_macroblock_mc( h, mb, 1 );

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
                quant_4x4( dct4x4[idx], i_qscale, 1 );

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

                    dequant_4x4( dct4x4[idx], i_qscale );
                    h->dctf.idct4x4( luma, dct4x4[idx] );

                    /* put pixel to fdec */
                    p_dst = ctx->p_fdec[0] + 4 * block_idx_x[idx] + 4 * block_idx_y[idx] * ctx->i_fdec[0];
                    h->pixf.add4x4( p_dst, ctx->i_fdec[0], luma );
                }
            }
        }
    }

    /* encode chroma */
    i_qscale = i_chroma_qp_table[x264_clip3( i_qscale + h->pps.i_chroma_qp_index_offset, 0, 51 )];
    if( IS_INTRA( mb->i_type ) )
    {
        /* do the right prediction */
        h->predict_8x8[mb->i_chroma_pred_mode]( mb->context->p_fdec[1], mb->context->i_fdec[1] );
        h->predict_8x8[mb->i_chroma_pred_mode]( mb->context->p_fdec[2], mb->context->i_fdec[2] );
    }
    else
    {
        /* Motion compensation */
        x264_macroblock_mc( h, mb, 0 );
    }
    /* encode the 8x8 blocks */
    x264_mb_encode_8x8( h, mb, !IS_INTRA( mb->i_type ), i_qscale );

    /* fix the pred mode value */
    if( IS_INTRA( mb->i_type ) )
    {
        mb->i_chroma_pred_mode = x264_mb_pred_mode8x8_fix( mb->i_chroma_pred_mode );
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
     * XXX: in the me perhaps we should take x264_macroblock_predict_mv_pskip into account
     *      (if multiple mv give same result)*/
    if( mb->i_type == P_L0 && mb->i_partition == D_16x16 &&
        mb->i_cbp_luma == 0x00 && mb->i_cbp_chroma == 0x00 )
    {

        int i_ref;
        int mvx, mvy;
        x264_macroblock_partition_get( mb, 0, 0, 0, &i_ref, &mvx, &mvy );

        if( i_ref == 0 )
        {
            int mvxp, mvyp;

            x264_macroblock_predict_mv_pskip( mb, &mvxp, &mvyp );
            if( mvxp == mvx && mvyp == mvy )
            {
                mb->i_type = P_SKIP;
            }
        }
    }
}


#define BLOCK_INDEX_CHROMA_DC   (-1)
#define BLOCK_INDEX_LUMA_DC     (-2)

/****************************************************************************
 * block_residual_write:
 ****************************************************************************/
static void block_residual_write( x264_t *h, bs_t *s, x264_macroblock_t *mb, int i_idx, int *l, int i_count )
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
        bs_write_vlc( s, x264_coeff_token[4][i_total][i_trailing] );
    }
    else
    {
        /* predict_non_zero_code return 0 <-> (16+16+1)>>1 = 16 */
        static const int ct_index[17] = {0,0,1,1,2,2,2,2,3,3,3,3,3,3,3,3,3 };
        int nC;

        if( i_idx == BLOCK_INDEX_LUMA_DC )
        {
            nC = predict_non_zero_code( h, mb, 0 );
        }
        else
        {
            nC = predict_non_zero_code( h, mb, i_idx );
        }

        bs_write_vlc( s, x264_coeff_token[ct_index[nC]][i_total][i_trailing] );
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
void x264_macroblock_write( x264_t *h, bs_t *s, int i_slice_type, x264_macroblock_t *mb )
{
    int i;
    int i_mb_i_offset;
    int b_sub_ref0 = 1;
    /* int b_sub_ref1 = 1; */

    switch( i_slice_type )
    {
        case SLICE_TYPE_I:
            i_mb_i_offset = 0;
            break;
        case SLICE_TYPE_P:
            i_mb_i_offset = 5;
            break;
        case SLICE_TYPE_B:
            i_mb_i_offset = 23 + 5;
            break;
        default:
            fprintf( stderr, "internal error or slice unsupported\n" );
            return;
    }

    /* PCM special block type UNTESTED */
    if( mb->i_type == I_PCM )
    {
        bs_write_ue( s, i_mb_i_offset + 25 );   /* I_PCM */
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

    if( mb->i_type == I_4x4 )
    {
        bs_write_ue( s, i_mb_i_offset + 0 );    /* I_4x4 */
    }
    else if( mb->i_type == I_16x16 )
    {
        int i_type = 1 + mb->i_intra16x16_pred_mode + mb->i_cbp_chroma * 4 + ( mb->i_cbp_luma == 0 ? 0 : 12 );

        bs_write_ue( s, i_mb_i_offset + i_type );
    }
    else if( mb->i_type == P_L0 )
    {
        if( mb->i_partition == D_16x16 )
        {
            bs_write_ue( s, 0 );
        }
        else if( mb->i_partition == D_16x8 )
        {
            bs_write_ue( s, 1 );
        }
        else if( mb->i_partition == D_8x16 )
        {
            bs_write_ue( s, 2 );
        }
    }
    else if( mb->i_type == P_8x8 )
    {
        if( mb->partition[0][0].i_ref[0] == 0 &&
            mb->partition[0][2].i_ref[0] == 0 &&
            mb->partition[2][0].i_ref[0] == 0 &&
            mb->partition[2][2].i_ref[0] == 0 )
        {
            b_sub_ref0 = 0;
            bs_write_ue( s, 4 );    /* P_8x8ref0 */
        }
        else
        {
            b_sub_ref0 = 1;
            bs_write_ue( s, 3 );
        }
    }
    else
    {
        /* TODO B type */
    }

    if( IS_INTRA( mb->i_type ) )
    {
        /* Prediction */
        if( mb->i_type == I_4x4 )
        {
            for( i = 0; i < 16; i++ )
            {
                int i_predicted_mode = predict_pred_intra4x4_mode( h, mb, i );
                int i_mode = mb->block[i].i_intra4x4_pred_mode;

                if( i_predicted_mode == i_mode)
                {
                    bs_write( s, 1, 1 );  /* b_prev_intra4x4_pred_mode */
                }
                else
                {
                    bs_write( s, 1, 0 );  /* b_prev_intra4x4_pred_mode */
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
        }
        bs_write_ue( s, mb->i_chroma_pred_mode );
    }
    else if( mb->i_type == P_8x8 )
    {
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
            for( i = 0; i < 4; i++ )
            {
                int i_ref;
                x264_macroblock_partition_get( mb, 0, i, 0, &i_ref, NULL, NULL );

                bs_write_te( s, h->sh.i_num_ref_idx_l0_active - 1, i_ref );
            }
        }
        for( i = 0; i < 4; i++ )
        {
            int i_part;
            for( i_part = 0; i_part < mb_sub_partition_count( mb->i_sub_partition[i] ); i_part++ )
            {
                int mvx, mvy;
                int mvxp, mvyp;

                x264_macroblock_partition_get( mb, 0, i, i_part, NULL, &mvx, &mvy );
                x264_macroblock_predict_mv( mb, 0, i, i_part, &mvxp, &mvyp );

                bs_write_se( s, mvx - mvxp );
                bs_write_se( s, mvy - mvyp);
            }
        }
    }
    else if( mb->i_type == B_8x8 )
    {
        /* TODO for B-frame (merge it with P_8x8 ?)*/
    }
    else if( mb->i_type != B_DIRECT )
    {
        /* FIXME -> invalid for B frame */

        /* Motion Vector */
        int i_part = 1 + ( mb->i_partition != D_16x16 ? 1 : 0 );

        if( h->sh.i_num_ref_idx_l0_active > 1 )
        {
            for( i = 0; i < i_part; i++ )
            {
                if( mb->i_type == P_L0 )    /* fixme B-frame */
                {
                    int i_ref;
                    x264_macroblock_partition_get( mb, 0, i, 0, &i_ref, NULL, NULL );
                    bs_write_te( s, h->sh.i_num_ref_idx_l0_active - 1, i_ref ); /* -1 is correct ? */
                }
            }
        }
        if( h->sh.i_num_ref_idx_l1_active > 1 )
        {
            for( i = 0; i < i_part; i++ )
            {
                /* ref idx part L1 TODO when needed */
            }
        }

        for( i = 0; i < i_part; i++ )
        {
            if( mb->i_type == P_L0 )
            {
                int mvx, mvy;
                int mvxp, mvyp;

                x264_macroblock_partition_get( mb, 0, i, 0, NULL, &mvx, &mvy );
                x264_macroblock_predict_mv( mb, 0, i, 0, &mvxp, &mvyp );

                bs_write_se( s, mvx - mvxp );
                bs_write_se( s, mvy - mvyp);
            }
        }
        /* Same for L1 for B frame */
    }

    if( mb->i_type != I_16x16 )
    {
        if( mb->i_type == I_4x4 )
        {
            bs_write_ue( s, intra4x4_cbp_to_golomb[( mb->i_cbp_chroma << 4 )|mb->i_cbp_luma] );
        }
        else
        {
            bs_write_ue( s, inter_cbp_to_golomb[( mb->i_cbp_chroma << 4 )|mb->i_cbp_luma] );
        }
    }

    if( mb->i_cbp_luma > 0 || mb->i_cbp_chroma > 0 || mb->i_type == I_16x16 )
    {
        bs_write_se( s, mb->i_qp_delta );

        /* write residual */
        if( mb->i_type == I_16x16 )
        {
            /* DC Luma */
            block_residual_write( h, s, mb, BLOCK_INDEX_LUMA_DC , mb->luma16x16_dc, 16 );

            if( mb->i_cbp_luma != 0 )
            {
                /* AC Luma */
                for( i = 0; i < 16; i++ )
                {
                    block_residual_write( h, s, mb, i, mb->block[i].residual_ac, 15 );
                }
            }
        }
        else
        {
            for( i = 0; i < 16; i++ )
            {
                if( mb->i_cbp_luma & ( 1 << ( i / 4 ) ) )
                {
                    block_residual_write( h, s, mb, i, mb->block[i].luma4x4, 16 );
                }
            }
        }

        if( mb->i_cbp_chroma &0x03 )    /* Chroma DC residual present */
        {
            block_residual_write( h, s, mb, BLOCK_INDEX_CHROMA_DC, mb->chroma_dc[0], 4 );
            block_residual_write( h, s, mb, BLOCK_INDEX_CHROMA_DC, mb->chroma_dc[1], 4 );
        }
        if( mb->i_cbp_chroma&0x02 ) /* Chroma AC residual present */
        {
            for( i = 0; i < 8; i++ )
            {
                block_residual_write( h, s, mb, 16 + i, mb->block[16+i].residual_ac, 15 );
            }
        }
    }
}


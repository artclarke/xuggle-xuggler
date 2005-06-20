/*****************************************************************************
 * macroblock.c: h264 encoder library
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar
 * $Id: macroblock.c,v 1.1 2004/06/03 19:27:08 fenrir Exp $
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

#include "common/common.h"
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
    { 0, 2, 8,  10 },
    { 1, 3, 9,  11 },
    { 4, 6, 12, 14 },
    { 5, 7, 13, 15 }
};

/* def_quant4_mf only for probe_skip; actual encoding uses matrices from set.c */
/* FIXME this seems to make better decisions with cqm=jvt, but could screw up
 * with general custom matrices. */
static const int def_quant4_mf[6][4][4] =
{
    { { 13107, 8066, 13107, 8066 }, { 8066, 5243, 8066, 5243 },
      { 13107, 8066, 13107, 8066 }, { 8066, 5243, 8066, 5243 } },
    { { 11916, 7490, 11916, 7490 }, { 7490, 4660, 7490, 4660 },
      { 11916, 7490, 11916, 7490 }, { 7490, 4660, 7490, 4660 } },
    { { 10082, 6554, 10082, 6554 }, { 6554, 4194, 6554, 4194 },
      { 10082, 6554, 10082, 6554 }, { 6554, 4194, 6554, 4194 } },
    { {  9362, 5825,  9362, 5825 }, { 5825, 3647, 5825, 3647 },
      {  9362, 5825,  9362, 5825 }, { 5825, 3647, 5825, 3647 } },
    { {  8192, 5243,  8192, 5243 }, { 5243, 3355, 5243, 3355 },
      {  8192, 5243,  8192, 5243 }, { 5243, 3355, 5243, 3355 } },
    { {  7282, 4559,  7282, 4559 }, { 4559, 2893, 4559, 2893 },
      {  7282, 4559,  7282, 4559 }, { 4559, 2893, 4559, 2893 } }
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

/****************************************************************************
 * Scan and Quant functions
 ****************************************************************************/
//static const int scan_zigzag_x[16]={0, 1, 0, 0, 1, 2, 3, 2, 1, 0, 1, 2, 3, 3, 2, 3};
//static const int scan_zigzag_y[16]={0, 0, 1, 2, 1, 0, 0, 1, 2, 3, 3, 2, 1, 2, 3, 3};

#define ZIG(i,y,x) level[i] = dct[y][x];
static inline void scan_zigzag_8x8full( int level[64], int16_t dct[8][8] )
{
    ZIG( 0,0,0) ZIG( 1,0,1) ZIG( 2,1,0) ZIG( 3,2,0)
    ZIG( 4,1,1) ZIG( 5,0,2) ZIG( 6,0,3) ZIG( 7,1,2)
    ZIG( 8,2,1) ZIG( 9,3,0) ZIG(10,4,0) ZIG(11,3,1)
    ZIG(12,2,2) ZIG(13,1,3) ZIG(14,0,4) ZIG(15,0,5)
    ZIG(16,1,4) ZIG(17,2,3) ZIG(18,3,2) ZIG(19,4,1)
    ZIG(20,5,0) ZIG(21,6,0) ZIG(22,5,1) ZIG(23,4,2)
    ZIG(24,3,3) ZIG(25,2,4) ZIG(26,1,5) ZIG(27,0,6)
    ZIG(28,0,7) ZIG(29,1,6) ZIG(30,2,5) ZIG(31,3,4)
    ZIG(32,4,3) ZIG(33,5,2) ZIG(34,6,1) ZIG(35,7,0)
    ZIG(36,7,1) ZIG(37,6,2) ZIG(38,5,3) ZIG(39,4,4)
    ZIG(40,3,5) ZIG(41,2,6) ZIG(42,1,7) ZIG(43,2,7)
    ZIG(44,3,6) ZIG(45,4,5) ZIG(46,5,4) ZIG(47,6,3)
    ZIG(48,7,2) ZIG(49,7,3) ZIG(50,6,4) ZIG(51,5,5)
    ZIG(52,4,6) ZIG(53,3,7) ZIG(54,4,7) ZIG(55,5,6)
    ZIG(56,6,5) ZIG(57,7,4) ZIG(58,7,5) ZIG(59,6,6)
    ZIG(60,5,7) ZIG(61,6,7) ZIG(62,7,6) ZIG(63,7,7)
}
static inline void scan_zigzag_4x4full( int level[16], int16_t dct[4][4] )
{
    ZIG( 0,0,0) ZIG( 1,0,1) ZIG( 2,1,0) ZIG( 3,2,0)
    ZIG( 4,1,1) ZIG( 5,0,2) ZIG( 6,0,3) ZIG( 7,1,2)
    ZIG( 8,2,1) ZIG( 9,3,0) ZIG(10,3,1) ZIG(11,2,2)
    ZIG(12,1,3) ZIG(13,2,3) ZIG(14,3,2) ZIG(15,3,3)
}
static inline void scan_zigzag_4x4( int level[15], int16_t dct[4][4] )
{
                ZIG( 0,0,1) ZIG( 1,1,0) ZIG( 2,2,0)
    ZIG( 3,1,1) ZIG( 4,0,2) ZIG( 5,0,3) ZIG( 6,1,2)
    ZIG( 7,2,1) ZIG( 8,3,0) ZIG( 9,3,1) ZIG(10,2,2)
    ZIG(11,1,3) ZIG(12,2,3) ZIG(13,3,2) ZIG(14,3,3)
}
static inline void scan_zigzag_2x2_dc( int level[4], int16_t dct[2][2] )
{
    ZIG(0,0,0)
    ZIG(1,0,1)
    ZIG(2,1,0)
    ZIG(3,1,1)
}
#undef ZIG

#define ZIG(i,y,x) {\
    int o = x+y*i_stride;\
    level[i] = p_src[o] - p_dst[o];\
    p_dst[o] = p_src[o];\
}
static inline void sub_zigzag_4x4full( int level[16], const uint8_t *p_src, uint8_t *p_dst, int i_stride )
{
    ZIG( 0,0,0) ZIG( 1,0,1) ZIG( 2,1,0) ZIG( 3,2,0)
    ZIG( 4,1,1) ZIG( 5,0,2) ZIG( 6,0,3) ZIG( 7,1,2)
    ZIG( 8,2,1) ZIG( 9,3,0) ZIG(10,3,1) ZIG(11,2,2)
    ZIG(12,1,3) ZIG(13,2,3) ZIG(14,3,2) ZIG(15,3,3)
}
static inline void sub_zigzag_4x4( int level[15], const uint8_t *p_src, uint8_t *p_dst, int i_stride )
{
                ZIG( 0,0,1) ZIG( 1,1,0) ZIG( 2,2,0)
    ZIG( 3,1,1) ZIG( 4,0,2) ZIG( 5,0,3) ZIG( 6,1,2)
    ZIG( 7,2,1) ZIG( 8,3,0) ZIG( 9,3,1) ZIG(10,2,2)
    ZIG(11,1,3) ZIG(12,2,3) ZIG(13,3,2) ZIG(14,3,3)
}
#undef ZIG

static void quant_8x8( int16_t dct[8][8], int quant_mf[6][8][8], int i_qscale, int b_intra )
{
    const int i_qbits = 16 + i_qscale / 6;
    const int i_mf = i_qscale % 6;
    const int f = ( 1 << i_qbits ) / ( b_intra ? 3 : 6 );

    int x,y;
    for( y = 0; y < 8; y++ )
    {
        for( x = 0; x < 8; x++ )
        {
            if( dct[y][x] > 0 )
                dct[y][x] = ( f + dct[y][x] * quant_mf[i_mf][y][x] ) >> i_qbits;
            else
                dct[y][x] = - ( ( f - dct[y][x] * quant_mf[i_mf][y][x] ) >> i_qbits );
        }
    }
}
static void quant_4x4( int16_t dct[4][4], int quant_mf[6][4][4], int i_qscale, int b_intra )
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
                dct[y][x] = ( f + dct[y][x] * quant_mf[i_mf][y][x] ) >> i_qbits;
            else
                dct[y][x] = - ( ( f - dct[y][x] * quant_mf[i_mf][y][x] ) >> i_qbits );
        }
    }
}
static void quant_4x4_dc( int16_t dct[4][4], int quant_mf[6][4][4], int i_qscale )
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
                dct[y][x] =( f2 + dct[y][x]  * i_qmf) >> ( 1 + i_qbits );
            else
                dct[y][x] = - ( ( f2 - dct[y][x]  * i_qmf ) >> (1 + i_qbits ) );
        }
    }
}
static void quant_2x2_dc( int16_t dct[2][2], int quant_mf[6][4][4], int i_qscale, int b_intra )
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
                dct[y][x] =( f2 + dct[y][x]  * i_qmf) >> ( 1 + i_qbits );
            else
                dct[y][x] = - ( ( f2 - dct[y][x]  * i_qmf ) >> (1 + i_qbits ) );
        }
    }
}
#if 0
/* From a JVT doc */
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


static void quant_4x4( int16_t dct[4][4], int i_qscale, int b_intra )
{
    const int(*f_deadzone)[4][4][2] = b_intra ? &f_deadzone_intra : &f_deadzone_inter;
    const int i_qbits = 15 + i_qscale / 6;
    const int i_mf = i_qscale % 6;

    int x,y;
    for( y = 0; y < 4; y++ )
    {
        for( x = 0; x < 4; x++ )
        {
#if 0
            const int f = b_intra ?
                          (f_deadzone_intra[y][x][0] * ( 1 << i_qbits ) / f_deadzone_intra[y][x][1])
                          :
                          (f_deadzone_inter[y][x][0] * ( 1 << i_qbits ) / f_deadzone_inter[y][x][1]);
#else
            const int f = (*f_deadzone)[y][x][0] * ( 1 << i_qbits ) / (*f_deadzone)[y][x][1];
#endif

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


#endif

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
    static const int i_ds_table4[16] = {
        3,2,2,1,1,1,0,0,0,0,0,0,0,0,0,0 };
    static const int i_ds_table8[64] = {
        3,3,3,3,2,2,2,2,2,2,2,2,1,1,1,1,
        1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };

    const int *ds_table = (i_max == 64) ? i_ds_table8 : i_ds_table4;
    int i_score = 0;
    int idx = i_max - 1;

    while( idx >= 0 && dct[idx] == 0 )
        idx--;

    while( idx >= 0 )
    {
        int i_run;

        if( abs( dct[idx--] ) > 1 )
            return 9;

        i_run = 0;
        while( idx >= 0 && dct[idx] == 0 )
        {
            idx--;
            i_run++;
        }
        i_score += ds_table[i_run];
    }

    return i_score;
}

void x264_mb_encode_i4x4( x264_t *h, int idx, int i_qscale )
{
    const int i_stride = h->mb.pic.i_stride[0];
    const int i_offset = 4 * block_idx_x[idx] + 4 * block_idx_y[idx] * i_stride;
    uint8_t *p_src = &h->mb.pic.p_fenc[0][i_offset];
    uint8_t *p_dst = &h->mb.pic.p_fdec[0][i_offset];
    int16_t dct4x4[4][4];

    if( h->mb.b_lossless )
    {
        sub_zigzag_4x4full( h->dct.block[idx].luma4x4, p_src, p_dst, i_stride );
        return;
    }

    h->dctf.sub4x4_dct( dct4x4, p_src, i_stride, p_dst, i_stride );
    quant_4x4( dct4x4, h->quant4_mf[CQM_4IY], i_qscale, 1 );
    scan_zigzag_4x4full( h->dct.block[idx].luma4x4, dct4x4 );
    x264_mb_dequant_4x4( dct4x4, h->dequant4_mf[CQM_4IY], i_qscale );

    /* output samples to fdec */
    h->dctf.add4x4_idct( p_dst, i_stride, dct4x4 );
}

void x264_mb_encode_i8x8( x264_t *h, int idx, int i_qscale )
{
    const int i_stride = h->mb.pic.i_stride[0];
    const int i_offset = 8 * (idx&1) + 8 * (idx>>1) * i_stride;
    uint8_t *p_src = &h->mb.pic.p_fenc[0][i_offset];
    uint8_t *p_dst = &h->mb.pic.p_fdec[0][i_offset];
    int16_t dct8x8[8][8];

    h->dctf.sub8x8_dct8( dct8x8, p_src, i_stride, p_dst, i_stride );
    quant_8x8( dct8x8, h->quant8_mf[CQM_8IY], i_qscale, 1 );
    scan_zigzag_8x8full( h->dct.luma8x8[idx], dct8x8 );
    x264_mb_dequant_8x8( dct8x8, h->dequant8_mf[CQM_8IY], i_qscale );
    h->dctf.add8x8_idct8( p_dst, i_stride, dct8x8 );
}

static void x264_mb_encode_i16x16( x264_t *h, int i_qscale )
{
    const int i_stride = h->mb.pic.i_stride[0];
    uint8_t  *p_src = h->mb.pic.p_fenc[0];
    uint8_t  *p_dst = h->mb.pic.p_fdec[0];

    int16_t dct4x4[16+1][4][4];

    int i;

    if( h->mb.b_lossless )
    {
        for( i = 0; i < 16; i++ )
        {
            int o = block_idx_x[i]*4 + block_idx_y[i]*4*i_stride;
            sub_zigzag_4x4( h->dct.block[i].residual_ac, p_src+o, p_dst+o, i_stride );
            dct4x4[0][block_idx_y[i]][block_idx_x[i]] = p_src[o] - p_dst[o];
            p_dst[o] = p_src[o];
        }
        scan_zigzag_4x4full( h->dct.luma16x16_dc, dct4x4[0] );
        return;
    }

    h->dctf.sub16x16_dct( &dct4x4[1], p_src, i_stride, p_dst, i_stride );
    for( i = 0; i < 16; i++ )
    {
        /* copy dc coeff */
        dct4x4[0][block_idx_y[i]][block_idx_x[i]] = dct4x4[1+i][0][0];

        /* quant/scan/dequant */
        quant_4x4( dct4x4[1+i], h->quant4_mf[CQM_4IY], i_qscale, 1 );
        scan_zigzag_4x4( h->dct.block[i].residual_ac, dct4x4[1+i] );
        x264_mb_dequant_4x4( dct4x4[1+i], h->dequant4_mf[CQM_4IY], i_qscale );
    }

    h->dctf.dct4x4dc( dct4x4[0] );
    quant_4x4_dc( dct4x4[0], h->quant4_mf[CQM_4IY], i_qscale );
    scan_zigzag_4x4full( h->dct.luma16x16_dc, dct4x4[0] );

    /* output samples to fdec */
    h->dctf.idct4x4dc( dct4x4[0] );
    x264_mb_dequant_4x4_dc( dct4x4[0], h->dequant4_mf[CQM_4IY], i_qscale );  /* XXX not inversed */

    /* calculate dct coeffs */
    for( i = 0; i < 16; i++ )
    {
        /* copy dc coeff */
        dct4x4[1+i][0][0] = dct4x4[0][block_idx_y[i]][block_idx_x[i]];
    }
    /* put pixels to fdec */
    h->dctf.add16x16_idct( p_dst, i_stride, &dct4x4[1] );
}

static void x264_mb_encode_8x8_chroma( x264_t *h, int b_inter, int i_qscale )
{
    int i, ch;

    for( ch = 0; ch < 2; ch++ )
    {
        const int i_stride = h->mb.pic.i_stride[1+ch];
        uint8_t  *p_src = h->mb.pic.p_fenc[1+ch];
        uint8_t  *p_dst = h->mb.pic.p_fdec[1+ch];
        int i_decimate_score = 0;

        int16_t dct2x2[2][2];
        int16_t dct4x4[4][4][4];

        if( h->mb.b_lossless )
        {
            for( i = 0; i < 4; i++ )
            {
                int o = block_idx_x[i]*4 + block_idx_y[i]*4*i_stride;
                sub_zigzag_4x4( h->dct.block[16+i+ch*4].residual_ac, p_src+o, p_dst+o, i_stride );
                h->dct.chroma_dc[ch][i] = p_src[o] - p_dst[o];
                p_dst[o] = p_src[o];
            }
            continue;
        }
            
        h->dctf.sub8x8_dct( dct4x4, p_src, i_stride, p_dst, i_stride );
        /* calculate dct coeffs */
        for( i = 0; i < 4; i++ )
        {
            /* copy dc coeff */
            dct2x2[block_idx_y[i]][block_idx_x[i]] = dct4x4[i][0][0];

            quant_4x4( dct4x4[i], h->quant4_mf[CQM_4IC + b_inter], i_qscale, b_inter ? 0 : 1 );
            scan_zigzag_4x4( h->dct.block[16+i+ch*4].residual_ac, dct4x4[i] );
            x264_mb_dequant_4x4( dct4x4[i], h->dequant4_mf[CQM_4IC + b_inter], i_qscale );

            if( b_inter )
            {
                i_decimate_score += x264_mb_decimate_score( h->dct.block[16+i+ch*4].residual_ac, 15 );
            }
        }

        h->dctf.dct2x2dc( dct2x2 );
        quant_2x2_dc( dct2x2, h->quant4_mf[CQM_4IC + b_inter], i_qscale, b_inter ? 0 : 1 );
        scan_zigzag_2x2_dc( h->dct.chroma_dc[ch], dct2x2 );

        /* output samples to fdec */
        h->dctf.idct2x2dc( dct2x2 );
        x264_mb_dequant_2x2_dc( dct2x2, h->dequant4_mf[CQM_4IC + b_inter], i_qscale );  /* XXX not inversed */

        if( b_inter && i_decimate_score < 7 )
        {
            /* Near null chroma 8x8 block so make it null (bits saving) */
            for( i = 0; i < 4; i++ )
            {
                int x, y;
                for( x = 0; x < 15; x++ )
                {
                    h->dct.block[16+i+ch*4].residual_ac[x] = 0;
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

        /* calculate dct coeffs */
        for( i = 0; i < 4; i++ )
        {
            /* copy dc coeff */
            dct4x4[i][0][0] = dct2x2[block_idx_y[i]][block_idx_x[i]];
        }
        h->dctf.add8x8_idct( p_dst, i_stride, dct4x4 );
    }
}

static void x264_macroblock_encode_skip( x264_t *h )
{
    int i;
    h->mb.i_cbp_luma = 0x00;
    h->mb.i_cbp_chroma = 0x00;

    for( i = 0; i < 16+8; i++ )
    {
        h->mb.cache.non_zero_count[x264_scan8[i]] = 0;
    }

    /* store cbp */
    h->mb.cbp[h->mb.i_mb_xy] = 0;
}

/*****************************************************************************
 * x264_macroblock_encode_pskip:
 *  Encode an already marked skip block
 *****************************************************************************/
void x264_macroblock_encode_pskip( x264_t *h )
{
    const int mvx = x264_clip3( h->mb.cache.mv[0][x264_scan8[0]][0],
                                h->mb.mv_min[0], h->mb.mv_max[0] );
    const int mvy = x264_clip3( h->mb.cache.mv[0][x264_scan8[0]][1],
                                h->mb.mv_min[1], h->mb.mv_max[1] );

    /* Motion compensation XXX probably unneeded */
    h->mc.mc_luma( h->mb.pic.p_fref[0][0], h->mb.pic.i_stride[0],
                    h->mb.pic.p_fdec[0],       h->mb.pic.i_stride[0],
                    mvx, mvy, 16, 16 );

    /* Chroma MC */
    h->mc.mc_chroma( h->mb.pic.p_fref[0][0][4], h->mb.pic.i_stride[1],
                      h->mb.pic.p_fdec[1],       h->mb.pic.i_stride[1],
                      mvx, mvy, 8, 8 );

    h->mc.mc_chroma( h->mb.pic.p_fref[0][0][5], h->mb.pic.i_stride[2],
                      h->mb.pic.p_fdec[2],       h->mb.pic.i_stride[2],
                      mvx, mvy, 8, 8 );

    x264_macroblock_encode_skip( h );
}

/*****************************************************************************
 * x264_macroblock_encode:
 *****************************************************************************/
void x264_macroblock_encode( x264_t *h )
{
    int i_cbp_dc = 0;
    int i_qp = h->mb.i_qp;
    int i;

    if( h->mb.i_type == P_SKIP )
    {
        /* A bit special */
        x264_macroblock_encode_pskip( h );
        return;
    }
    if( h->mb.i_type == B_SKIP )
    {
        /* XXX motion compensation is probably unneeded */
        x264_mb_mc( h );
        x264_macroblock_encode_skip( h );
        return;
    }

    if( h->mb.i_type == I_16x16 )
    {
        const int i_mode = h->mb.i_intra16x16_pred_mode;
        h->mb.b_transform_8x8 = 0;
        /* do the right prediction */
        h->predict_16x16[i_mode]( h->mb.pic.p_fdec[0], h->mb.pic.i_stride[0] );

        /* encode the 16x16 macroblock */
        x264_mb_encode_i16x16( h, i_qp );

        /* fix the pred mode value */
        h->mb.i_intra16x16_pred_mode = x264_mb_pred_mode16x16_fix[i_mode];
    }
    else if( h->mb.i_type == I_8x8 )
    {
        h->mb.b_transform_8x8 = 1;
        for( i = 0; i < 4; i++ )
        {
            const int i_dst = h->mb.pic.i_stride[0];
            uint8_t  *p_dst = &h->mb.pic.p_fdec[0][8 * (i&1) + 8 * (i>>1) * i_dst];
            int      i_mode = h->mb.cache.intra4x4_pred_mode[x264_scan8[4*i]];

            h->predict_8x8[i_mode]( p_dst, i_dst, h->mb.i_neighbour8[i] );
            x264_mb_encode_i8x8( h, i, i_qp );
            h->mb.cache.intra4x4_pred_mode[x264_scan8[4*i]] = x264_mb_pred_mode4x4_fix(i_mode);
        }
    }
    else if( h->mb.i_type == I_4x4 )
    {
        h->mb.b_transform_8x8 = 0;
        for( i = 0; i < 16; i++ )
        {
            const int i_dst = h->mb.pic.i_stride[0];
            uint8_t  *p_dst = &h->mb.pic.p_fdec[0][4 * block_idx_x[i] + 4 * block_idx_y[i] * i_dst];
            int      i_mode = h->mb.cache.intra4x4_pred_mode[x264_scan8[i]];

            h->predict_4x4[i_mode]( p_dst, i_dst );
            x264_mb_encode_i4x4( h, i, i_qp );
            h->mb.cache.intra4x4_pred_mode[x264_scan8[i]] = x264_mb_pred_mode4x4_fix(i_mode);
        }
    }
    else    /* Inter MB */
    {
        int i8x8, i4x4, idx;
        int i_decimate_mb = 0;

        /* Motion compensation */
        x264_mb_mc( h );

        if( h->mb.b_lossless )
        {
            for( i4x4 = 0; i4x4 < 16; i4x4++ )
            {
                int o = block_idx_x[i4x4]*4 + block_idx_y[i4x4]*4 * h->mb.pic.i_stride[0];
                sub_zigzag_4x4full( h->dct.block[i4x4].luma4x4, h->mb.pic.p_fenc[0]+o, h->mb.pic.p_fdec[0]+o, h->mb.pic.i_stride[0] );
            }
        }
        else if( h->mb.b_transform_8x8 )
        {
            int16_t dct8x8[4][8][8];
            h->dctf.sub16x16_dct8( dct8x8,
                                   h->mb.pic.p_fenc[0], h->mb.pic.i_stride[0],
                                   h->mb.pic.p_fdec[0], h->mb.pic.i_stride[0] );

            for( idx = 0; idx < 4; idx++ )
            {
                int i_decimate_8x8;

                quant_8x8( dct8x8[idx], h->quant8_mf[CQM_8PY], i_qp, 0 );
                scan_zigzag_8x8full( h->dct.luma8x8[idx], dct8x8[idx] );
                x264_mb_dequant_8x8( dct8x8[idx], h->dequant8_mf[CQM_8PY], i_qp );

                i_decimate_8x8 = x264_mb_decimate_score( h->dct.luma8x8[idx], 64 );
                i_decimate_mb += i_decimate_8x8;
                if( i_decimate_8x8 < 4 )
                {
                    memset( h->dct.luma8x8[idx], 0, sizeof( h->dct.luma8x8[idx] ) );
                    memset( dct8x8[idx], 0, sizeof( dct8x8[idx] ) );
                }
            }

            if( i_decimate_mb < 6 )
                memset( h->dct.luma8x8, 0, sizeof( h->dct.luma8x8 ) );
            else
                h->dctf.add16x16_idct8( h->mb.pic.p_fdec[0], h->mb.pic.i_stride[0], dct8x8 );
        }
        else
        {
            int16_t dct4x4[16][4][4];
            h->dctf.sub16x16_dct( dct4x4,
                                  h->mb.pic.p_fenc[0], h->mb.pic.i_stride[0],
                                  h->mb.pic.p_fdec[0], h->mb.pic.i_stride[0] );

            for( i8x8 = 0; i8x8 < 4; i8x8++ )
            {
                int i_decimate_8x8;

                /* encode one 4x4 block */
                i_decimate_8x8 = 0;
                for( i4x4 = 0; i4x4 < 4; i4x4++ )
                {
                    idx = i8x8 * 4 + i4x4;

                    quant_4x4( dct4x4[idx], h->quant4_mf[CQM_4PY], i_qp, 0 );
                    scan_zigzag_4x4full( h->dct.block[idx].luma4x4, dct4x4[idx] );
                    x264_mb_dequant_4x4( dct4x4[idx], h->dequant4_mf[CQM_4PY], i_qp );

                    i_decimate_8x8 += x264_mb_decimate_score( h->dct.block[idx].luma4x4, 16 );
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
                            h->dct.block[idx].luma4x4[i] = 0;
                        for( x = 0; x < 4; x++ )
                            for( y = 0; y < 4; y++ )
                                dct4x4[idx][x][y] = 0;
                    }
                }
            }

            if( i_decimate_mb < 6 )
                for( idx = 0; idx < 16; idx++ )
                    for( i = 0; i < 16; i++ )
                        h->dct.block[idx].luma4x4[i] = 0;
            else
                h->dctf.add16x16_idct( h->mb.pic.p_fdec[0], h->mb.pic.i_stride[0], dct4x4 );
        }
    }

    /* encode chroma */
    i_qp = i_chroma_qp_table[x264_clip3( i_qp + h->pps->i_chroma_qp_index_offset, 0, 51 )];
    if( IS_INTRA( h->mb.i_type ) )
    {
        const int i_mode = h->mb.i_chroma_pred_mode;
        h->predict_8x8c[i_mode]( h->mb.pic.p_fdec[1], h->mb.pic.i_stride[1] );
        h->predict_8x8c[i_mode]( h->mb.pic.p_fdec[2], h->mb.pic.i_stride[2] );
    }

    /* encode the 8x8 blocks */
    x264_mb_encode_8x8_chroma( h, !IS_INTRA( h->mb.i_type ), i_qp );

    /* Calculate the Luma/Chroma patern and non_zero_count */
    h->mb.i_cbp_luma = 0x00;
    if( h->mb.i_type == I_16x16 )
    {
        for( i = 0; i < 16; i++ )
        {
            const int nz = array_non_zero_count( h->dct.block[i].residual_ac, 15 );
            h->mb.cache.non_zero_count[x264_scan8[i]] = nz;
            if( nz > 0 )
                h->mb.i_cbp_luma = 0x0f;
        }
    }
    else if( h->mb.b_transform_8x8 )
    {
        /* coded_block_flag is enough for CABAC.
         * the full non_zero_count is done only in CAVLC. */
        for( i = 0; i < 4; i++ )
        {
            const int nz = array_non_zero( h->dct.luma8x8[i], 64 );
            int j;
            for( j = 0; j < 4; j++ )
                h->mb.cache.non_zero_count[x264_scan8[4*i+j]] = nz;
            if( nz > 0 )
                h->mb.i_cbp_luma |= 1 << i;
        }
    }
    else
    {
        for( i = 0; i < 16; i++ )
        {
            const int nz = array_non_zero_count( h->dct.block[i].luma4x4, 16 );
            h->mb.cache.non_zero_count[x264_scan8[i]] = nz;
            if( nz > 0 )
                h->mb.i_cbp_luma |= 1 << (i/4);
        }
    }

    /* Calculate the chroma patern */
    h->mb.i_cbp_chroma = 0x00;
    for( i = 0; i < 8; i++ )
    {
        const int nz = array_non_zero_count( h->dct.block[16+i].residual_ac, 15 );
        h->mb.cache.non_zero_count[x264_scan8[16+i]] = nz;
        if( nz > 0 )
        {
            h->mb.i_cbp_chroma = 0x02;    /* dc+ac (we can't do only ac) */
        }
    }
    if( h->mb.i_cbp_chroma == 0x00 &&
        ( array_non_zero_count( h->dct.chroma_dc[0], 4 ) > 0 || array_non_zero_count( h->dct.chroma_dc[1], 4 ) ) > 0 )
    {
        h->mb.i_cbp_chroma = 0x01;    /* dc only */
    }

    if( h->param.b_cabac )
    {
        if( h->mb.i_type == I_16x16 && array_non_zero_count( h->dct.luma16x16_dc, 16 ) > 0 )
            i_cbp_dc = 0x01;
        else
            i_cbp_dc = 0x00;

        if( array_non_zero_count( h->dct.chroma_dc[0], 4 ) > 0 )
            i_cbp_dc |= 0x02;
        if( array_non_zero_count( h->dct.chroma_dc[1], 4 ) > 0 )
            i_cbp_dc |= 0x04;
    }

    /* store cbp */
    h->mb.cbp[h->mb.i_mb_xy] = (i_cbp_dc << 8) | (h->mb.i_cbp_chroma << 4) | h->mb.i_cbp_luma;

    if( h->mb.i_type != I_16x16 && h->mb.i_cbp_luma == 0 && h->mb.i_cbp_chroma == 0 )
    {
        /* It won'y change anything at the decoder side but it is needed else the
         * decoder will fail to read the next QP */
        h->mb.qp[h->mb.i_mb_xy] = h->mb.i_last_qp;
    }


    /* Check for P_SKIP
     * XXX: in the me perhaps we should take x264_mb_predict_mv_pskip into account
     *      (if multiple mv give same result)*/
    if( h->mb.i_type == P_L0 && h->mb.i_partition == D_16x16 &&
        h->mb.i_cbp_luma == 0x00 && h->mb.i_cbp_chroma== 0x00 &&
        h->mb.cache.ref[0][x264_scan8[0]] == 0 )
    {
        int mvp[2];

        x264_mb_predict_mv_pskip( h, mvp );
        if( h->mb.cache.mv[0][x264_scan8[0]][0] == mvp[0] &&
            h->mb.cache.mv[0][x264_scan8[0]][1] == mvp[1] )
        {
            h->mb.i_type = P_SKIP;
            h->mb.qp[h->mb.i_mb_xy] = h->mb.i_last_qp;  /* Needed */
            /* XXX qp reset may have issues when used in RD instead of the real encode */
        }
    }

    /* Check for B_SKIP */
    if( h->mb.i_type == B_DIRECT &&
        h->mb.i_cbp_luma == 0x00 && h->mb.i_cbp_chroma== 0x00 )
    {
        h->mb.i_type = B_SKIP;
        h->mb.qp[h->mb.i_mb_xy] = h->mb.i_last_qp;  /* Needed */
    }

    if( h->mb.i_cbp_luma == 0 && h->mb.i_type != I_8x8 )
        h->mb.b_transform_8x8 = 0;
}

/*****************************************************************************
 * x264_macroblock_probe_skip:
 *  Check if the current MB could be encoded as a [PB]_SKIP (it supposes you use
 *  the previous QP
 *****************************************************************************/
int x264_macroblock_probe_skip( x264_t *h, int b_bidir )
{
    DECLARE_ALIGNED( int16_t, dct4x4[16][4][4], 16 );
    DECLARE_ALIGNED( int16_t, dct2x2[2][2], 16 );
    DECLARE_ALIGNED( int,     dctscan[16], 16 );

    int i_qp = h->mb.i_qp;
    int mvp[2];
    int ch;

    int i8x8, i4x4;
    int i_decimate_mb;

    if( !b_bidir )
    {
        /* Get the MV */
        x264_mb_predict_mv_pskip( h, mvp );
        mvp[0] = x264_clip3( mvp[0], h->mb.mv_min[0], h->mb.mv_max[0] );
        mvp[1] = x264_clip3( mvp[1], h->mb.mv_min[1], h->mb.mv_max[1] );

        /* Motion compensation */
        h->mc.mc_luma( h->mb.pic.p_fref[0][0], h->mb.pic.i_stride[0],
                        h->mb.pic.p_fdec[0],   h->mb.pic.i_stride[0],
                        mvp[0], mvp[1], 16, 16 );
    }

    /* get luma diff */
    h->dctf.sub16x16_dct( dct4x4, h->mb.pic.p_fenc[0], h->mb.pic.i_stride[0],
                                  h->mb.pic.p_fdec[0], h->mb.pic.i_stride[0] );

    for( i8x8 = 0, i_decimate_mb = 0; i8x8 < 4; i8x8++ )
    {
        /* encode one 4x4 block */
        for( i4x4 = 0; i4x4 < 4; i4x4++ )
        {
            const int idx = i8x8 * 4 + i4x4;

            quant_4x4( dct4x4[idx], (int(*)[4][4])def_quant4_mf, i_qp, 0 );
            scan_zigzag_4x4full( dctscan, dct4x4[idx] );

            i_decimate_mb += x264_mb_decimate_score( dctscan, 16 );

            if( i_decimate_mb >= 6 )
            {
                /* not as P_SKIP */
                return 0;
            }
        }
    }

    /* encode chroma */
    i_qp = i_chroma_qp_table[x264_clip3( i_qp + h->pps->i_chroma_qp_index_offset, 0, 51 )];

    for( ch = 0; ch < 2; ch++ )
    {
        const int i_stride = h->mb.pic.i_stride[1+ch];
        uint8_t  *p_src = h->mb.pic.p_fenc[1+ch];
        uint8_t  *p_dst = h->mb.pic.p_fdec[1+ch];

        if( !b_bidir )
        {
            h->mc.mc_chroma( h->mb.pic.p_fref[0][0][4+ch], i_stride,
                              h->mb.pic.p_fdec[1+ch],       i_stride,
                              mvp[0], mvp[1], 8, 8 );
        }

        h->dctf.sub8x8_dct( dct4x4, p_src, i_stride, p_dst, i_stride );

        /* calculate dct DC */
        dct2x2[0][0] = dct4x4[0][0][0];
        dct2x2[0][1] = dct4x4[1][0][0];
        dct2x2[1][0] = dct4x4[2][0][0];
        dct2x2[1][1] = dct4x4[3][0][0];
        h->dctf.dct2x2dc( dct2x2 );
        quant_2x2_dc( dct2x2, (int(*)[4][4])def_quant4_mf, i_qp, 0 );
        if( dct2x2[0][0] || dct2x2[0][1] || dct2x2[1][0] || dct2x2[1][1]  )
        {
            /* can't be */
            return 0;
        }

        /* calculate dct coeffs */
        for( i4x4 = 0, i_decimate_mb = 0; i4x4 < 4; i4x4++ )
        {
            quant_4x4( dct4x4[i4x4], (int(*)[4][4])def_quant4_mf, i_qp, 0 );
            scan_zigzag_4x4( dctscan, dct4x4[i4x4] );

            i_decimate_mb += x264_mb_decimate_score( dctscan, 15 );
            if( i_decimate_mb >= 7 )
            {
                return 0;
            }
        }
    }

    return 1;
}

/*****************************************************************************
 * macroblock.h: h264 encoder library
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar
 * $Id: macroblock.h,v 1.1 2004/06/03 19:27:07 fenrir Exp $
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

#ifndef _MACROBLOCK_H
#define _MACROBLOCK_H 1

enum macroblock_position_e
{
    MB_LEFT     = 0x01,
    MB_TOP      = 0x02,
    MB_TOPRIGHT = 0x04,

    MB_PRIVATE  = 0x10,
};


/* XXX mb_type isn't the one written in the bitstream -> only internal usage */
#define IS_INTRA(type) ( (type) == I_4x4 || (type) == I_16x16 )
#define IS_SKIP(type)  ( (type) == P_SKIP || (type) == B_SKIP )
enum mb_class_e
{
    I_4x4           = 0,
    I_16x16         = 1,
    I_PCM           = 2,

    P_L0            = 3,
    P_8x8           = 4,
    P_SKIP          = 5,

    B_DIRECT        = 6,
    B_L0_L0         = 7,
    B_L0_L1         = 8,
    B_L0_BI         = 9,
    B_L1_L0         = 10,
    B_L1_L1         = 11,
    B_L1_BI         = 12,
    B_BI_L0         = 13,
    B_BI_L1         = 14,
    B_BI_BI         = 15,
    B_8x8           = 16,
    B_SKIP          = 17,
};
static const int x264_mb_type_list0_table[18][2] =
{
    {0,0}, {0,0}, {0,0},    /* INTRA */
    {1,1},                  /* P_L0 */
    {0,0},                  /* P_8x8 */
    {1,1},                  /* P_SKIP */
    {0,0},                  /* B_DIRECT */
    {1,1}, {1,0}, {1,1},    /* B_L0_* */
    {0,1}, {0,0}, {0,1},    /* B_L1_* */
    {1,1}, {1,0}, {1,1},    /* B_BI_* */
    {0,0},                  /* B_8x8 */
    {0,0}                   /* B_SKIP */
};
static const int x264_mb_type_list1_table[18][2] =
{
    {0,0}, {0,0}, {0,0},    /* INTRA */
    {0,0},                  /* P_L0 */
    {0,0},                  /* P_8x8 */
    {0,0},                  /* P_SKIP */
    {0,0},                  /* B_DIRECT */
    {0,0}, {0,1}, {0,1},    /* B_L0_* */
    {1,0}, {1,1}, {1,1},    /* B_L1_* */
    {1,0}, {1,1}, {1,1},    /* B_BI_* */
    {0,0},                  /* B_8x8 */
    {0,0}                   /* B_SKIP */
};

#define IS_SUB4x4(type) ( (type ==D_L0_4x4)||(type ==D_L1_4x4)||(type ==D_BI_4x4))
#define IS_SUB4x8(type) ( (type ==D_L0_4x8)||(type ==D_L1_4x8)||(type ==D_BI_4x8))
#define IS_SUB8x4(type) ( (type ==D_L0_8x4)||(type ==D_L1_8x4)||(type ==D_BI_8x4))
#define IS_SUB8x8(type) ( (type ==D_L0_8x8)||(type ==D_L1_8x8)||(type ==D_BI_8x8)||(type ==D_DIRECT_8x8))
enum mb_partition_e
{
    /* sub partition type for P_8x8 and B_8x8 */
    D_L0_4x4        = 0,
    D_L0_8x4        = 1,
    D_L0_4x8        = 2,
    D_L0_8x8        = 3,

    /* sub partition type for B_8x8 only */
    D_L1_4x4        = 4,
    D_L1_8x4        = 5,
    D_L1_4x8        = 6,
    D_L1_8x8        = 7,

    D_BI_4x4        = 8,
    D_BI_8x4        = 9,
    D_BI_4x8        = 10,
    D_BI_8x8        = 11,
    D_DIRECT_8x8    = 12,

    /* partition */
    D_8x8           = 13,
    D_16x8          = 14,
    D_8x16          = 15,
    D_16x16         = 16,
};

static const int x264_mb_partition_count_table[17] =
{
    /* sub L0 */
    4, 2, 2, 1,
    /* sub L1 */
    4, 2, 2, 1,
    /* sub BI */
    4, 2, 2, 1,
    /* Direct */
    1,
    /* Partition */
    4, 2, 2, 1
};

void x264_macroblock_cache_init( x264_t *h );
void x264_macroblock_cache_load( x264_t *h, int, int );
void x264_macroblock_cache_save( x264_t *h );
void x264_macroblock_cache_end( x264_t *h );

void x264_mb_dequant_4x4_dc( int16_t dct[4][4], int i_qscale );
void x264_mb_dequant_2x2_dc( int16_t dct[2][2], int i_qscale );
void x264_mb_dequant_4x4( int16_t dct[4][4], int i_qscale );

/* x264_mb_predict_mv_16x16:
 *      set mvp with predicted mv for D_16x16 block
 *      h->mb. need only valid values from other blocks */
void x264_mb_predict_mv_16x16( x264_t *h, int i_list, int i_ref, int mvp[2] );
/* x264_mb_predict_mv_pskip:
 *      set mvp with predicted mv for P_SKIP
 *      h->mb. need only valid values from other blocks */
void x264_mb_predict_mv_pskip( x264_t *h, int mv[2] );
/* x264_mb_predict_mv:
 *      set mvp with predicted mv for all blocks except P_SKIP
 *      h->mb. need valid ref/partition/sub of current block to be valid
 *      and valid mv/ref from other blocks . */
void x264_mb_predict_mv( x264_t *h, int i_list, int idx, int i_width, int mvp[2] );
/* x264_mb_predict_mv_ref16x16:
 *      set mvc with D_16x16 prediction.
 *      uses all neighbors, even those that didn't end up using this ref.
 *      need only valid values from other blocks */
void x264_mb_predict_mv_ref16x16( x264_t *h, int i_list, int i_ref, int mvc[4][2], int *i_mvc );


int  x264_mb_predict_intra4x4_mode( x264_t *h, int idx );
int  x264_mb_predict_non_zero_code( x264_t *h, int idx );

void x264_mb_encode_i4x4( x264_t *h, int idx, int i_qscale );

void x264_mb_mc( x264_t *h );


static inline void x264_macroblock_cache_ref( x264_t *h, int x, int y, int width, int height, int i_list, int ref )
{
    int dy, dx;
    for( dy = 0; dy < height; dy++ )
    {
        for( dx = 0; dx < width; dx++ )
        {
            h->mb.cache.ref[i_list][X264_SCAN8_0+x+dx+8*(y+dy)] = ref;
        }
    }
}
static inline void x264_macroblock_cache_mv( x264_t *h, int x, int y, int width, int height, int i_list, int mvx, int mvy )
{
    int dy, dx;
    for( dy = 0; dy < height; dy++ )
    {
        for( dx = 0; dx < width; dx++ )
        {
            h->mb.cache.mv[i_list][X264_SCAN8_0+x+dx+8*(y+dy)][0] = mvx;
            h->mb.cache.mv[i_list][X264_SCAN8_0+x+dx+8*(y+dy)][1] = mvy;
        }
    }
}
static inline void x264_macroblock_cache_mvd( x264_t *h, int x, int y, int width, int height, int i_list, int mdx, int mdy )
{
    int dy, dx;
    for( dy = 0; dy < height; dy++ )
    {
        for( dx = 0; dx < width; dx++ )
        {
            h->mb.cache.mvd[i_list][X264_SCAN8_0+x+dx+8*(y+dy)][0] = mdx;
            h->mb.cache.mvd[i_list][X264_SCAN8_0+x+dx+8*(y+dy)][1] = mdy;
        }
    }
}

#endif


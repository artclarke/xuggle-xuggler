/*****************************************************************************
 * cavlc.c: h264 encoder library
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar
 * $Id: cavlc.c,v 1.1 2004/06/03 19:27:08 fenrir Exp $
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

#include "../core/common.h"
#include "../core/vlc.h"
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

#define BLOCK_INDEX_CHROMA_DC   (-1)
#define BLOCK_INDEX_LUMA_DC     (-2)

static inline void bs_write_vlc( bs_t *s, vlc_t v )
{
    bs_write( s, v.i_size, v.i_bits );
}

/****************************************************************************
 * block_residual_write_cavlc:
 ****************************************************************************/
static void block_residual_write_cavlc( x264_t *h, bs_t *s, int i_idx, int *l, int i_count )
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
            nC = x264_mb_predict_non_zero_code( h, 0 );
        }
        else
        {
            nC = x264_mb_predict_non_zero_code( h, i_idx );
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
void x264_macroblock_write_cavlc( x264_t *h, bs_t *s )
{
    const int i_mb_type = h->mb.i_type;
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
    if( i_mb_type == I_PCM )
    {
        /* Untested */
        bs_write_ue( s, i_mb_i_offset + 25 );

        bs_align_0( s );
        /* Luma */
        for( i = 0; i < 16*16; i++ )
        {
            const int x = 16 * h->mb.i_mb_x + (i % 16);
            const int y = 16 * h->mb.i_mb_y + (i / 16);
            bs_write( s, 8, h->fenc->plane[0][y*h->mb.pic.i_stride[0]+x] );
        }
        /* Cb */
        for( i = 0; i < 8*8; i++ )
        {
            const int x = 8 * h->mb.i_mb_x + (i % 8);
            const int y = 8 * h->mb.i_mb_y + (i / 8);
            bs_write( s, 8, h->fenc->plane[1][y*h->mb.pic.i_stride[1]+x] );
        }
        /* Cr */
        for( i = 0; i < 8*8; i++ )
        {
            const int x = 8 * h->mb.i_mb_x + (i % 8);
            const int y = 8 * h->mb.i_mb_y + (i / 8);
            bs_write( s, 8, h->fenc->plane[2][y*h->mb.pic.i_stride[2]+x] );
        }
        return;
    }
    else if( i_mb_type == I_4x4 )
    {
        bs_write_ue( s, i_mb_i_offset + 0 );

        /* Prediction: Luma */
        for( i = 0; i < 16; i++ )
        {
            int i_pred = x264_mb_predict_intra4x4_mode( h, i );
            int i_mode = h->mb.cache.intra4x4_pred_mode[x264_scan8[i]];

            if( i_pred == i_mode)
            {
                bs_write1( s, 1 );  /* b_prev_intra4x4_pred_mode */
            }
            else
            {
                bs_write1( s, 0 );  /* b_prev_intra4x4_pred_mode */
                if( i_mode < i_pred )
                {
                    bs_write( s, 3, i_mode );
                }
                else
                {
                    bs_write( s, 3, i_mode - 1 );
                }
            }
        }
        bs_write_ue( s, h->mb.i_chroma_pred_mode );
    }
    else if( i_mb_type == I_16x16 )
    {
        bs_write_ue( s, i_mb_i_offset + 1 + h->mb.i_intra16x16_pred_mode +
                        h->mb.i_cbp_chroma * 4 + ( h->mb.i_cbp_luma == 0 ? 0 : 12 ) );
        bs_write_ue( s, h->mb.i_chroma_pred_mode );
    }
    else if( i_mb_type == P_L0 )
    {
        int mvp[2];

        if( h->mb.i_partition == D_16x16 )
        {
            bs_write_ue( s, 0 );

            if( h->sh.i_num_ref_idx_l0_active > 1 )
            {
                bs_write_te( s, h->sh.i_num_ref_idx_l0_active - 1, h->mb.cache.ref[0][x264_scan8[0]] );
            }
            x264_mb_predict_mv( h, 0, 0, 4, mvp );
            bs_write_se( s, h->mb.cache.mv[0][x264_scan8[0]][0] - mvp[0] );
            bs_write_se( s, h->mb.cache.mv[0][x264_scan8[0]][1] - mvp[1] );
        }
        else if( h->mb.i_partition == D_16x8 )
        {
            bs_write_ue( s, 1 );
            if( h->sh.i_num_ref_idx_l0_active > 1 )
            {
                bs_write_te( s, h->sh.i_num_ref_idx_l0_active - 1, h->mb.cache.ref[0][x264_scan8[0]] );
                bs_write_te( s, h->sh.i_num_ref_idx_l0_active - 1, h->mb.cache.ref[0][x264_scan8[8]] );
            }

            x264_mb_predict_mv( h, 0, 0, 4, mvp );
            bs_write_se( s, h->mb.cache.mv[0][x264_scan8[0]][0] - mvp[0] );
            bs_write_se( s, h->mb.cache.mv[0][x264_scan8[0]][1] - mvp[1] );

            x264_mb_predict_mv( h, 0, 8, 4, mvp );
            bs_write_se( s, h->mb.cache.mv[0][x264_scan8[8]][0] - mvp[0] );
            bs_write_se( s, h->mb.cache.mv[0][x264_scan8[8]][1] - mvp[1] );
        }
        else if( h->mb.i_partition == D_8x16 )
        {
            bs_write_ue( s, 2 );
            if( h->sh.i_num_ref_idx_l0_active > 1 )
            {
                bs_write_te( s, h->sh.i_num_ref_idx_l0_active - 1, h->mb.cache.ref[0][x264_scan8[0]] );
                bs_write_te( s, h->sh.i_num_ref_idx_l0_active - 1, h->mb.cache.ref[0][x264_scan8[4]] );
            }

            x264_mb_predict_mv( h, 0, 0, 2, mvp );
            bs_write_se( s, h->mb.cache.mv[0][x264_scan8[0]][0] - mvp[0] );
            bs_write_se( s, h->mb.cache.mv[0][x264_scan8[0]][1] - mvp[1] );

            x264_mb_predict_mv( h, 0, 4, 2, mvp );
            bs_write_se( s, h->mb.cache.mv[0][x264_scan8[4]][0] - mvp[0] );
            bs_write_se( s, h->mb.cache.mv[0][x264_scan8[4]][1] - mvp[1] );
        }
    }
    else if( i_mb_type == P_8x8 )
    {
        int b_sub_ref0;

        if( h->mb.cache.ref[0][x264_scan8[0]] == 0 && h->mb.cache.ref[0][x264_scan8[4]] == 0 &&
            h->mb.cache.ref[0][x264_scan8[8]] == 0 && h->mb.cache.ref[0][x264_scan8[12]] == 0 )
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
            switch( h->mb.i_sub_partition[i] )
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
            bs_write_te( s, h->sh.i_num_ref_idx_l0_active - 1, h->mb.cache.ref[0][x264_scan8[0]] );
            bs_write_te( s, h->sh.i_num_ref_idx_l0_active - 1, h->mb.cache.ref[0][x264_scan8[4]] );
            bs_write_te( s, h->sh.i_num_ref_idx_l0_active - 1, h->mb.cache.ref[0][x264_scan8[8]] );
            bs_write_te( s, h->sh.i_num_ref_idx_l0_active - 1, h->mb.cache.ref[0][x264_scan8[12]] );
        }
        for( i = 0; i < 4; i++ )
        {
            int mvp[2];

            switch( h->mb.i_sub_partition[i] )
            {
                case D_L0_8x8:
                    x264_mb_predict_mv( h, 0, 4*i, 2, mvp );
                    bs_write_se( s, h->mb.cache.mv[0][x264_scan8[4*i]][0] - mvp[0] );
                    bs_write_se( s, h->mb.cache.mv[0][x264_scan8[4*i]][1] - mvp[1] );
                    break;
                case D_L0_8x4:
                    x264_mb_predict_mv( h, 0, 4*i+0, 2, mvp );
                    bs_write_se( s, h->mb.cache.mv[0][x264_scan8[4*i]][0] - mvp[0] );
                    bs_write_se( s, h->mb.cache.mv[0][x264_scan8[4*i]][1] - mvp[1] );

                    x264_mb_predict_mv( h, 0, 4*i+2, 2, mvp );
                    bs_write_se( s, h->mb.cache.mv[0][x264_scan8[4*i+2]][0] - mvp[0] );
                    bs_write_se( s, h->mb.cache.mv[0][x264_scan8[4*i+2]][1] - mvp[1] );
                    break;
                case D_L0_4x8:
                    x264_mb_predict_mv( h, 0, 4*i+0, 1, mvp );
                    bs_write_se( s, h->mb.cache.mv[0][x264_scan8[4*i]][0] - mvp[0] );
                    bs_write_se( s, h->mb.cache.mv[0][x264_scan8[4*i]][1] - mvp[1] );

                    x264_mb_predict_mv( h, 0, 4*i+1, 1, mvp );
                    bs_write_se( s, h->mb.cache.mv[0][x264_scan8[4*i+1]][0] - mvp[0] );
                    bs_write_se( s, h->mb.cache.mv[0][x264_scan8[4*i+1]][1] - mvp[1] );
                    break;
                case D_L0_4x4:
                    x264_mb_predict_mv( h, 0, 4*i+0, 1, mvp );
                    bs_write_se( s, h->mb.cache.mv[0][x264_scan8[4*i]][0] - mvp[0] );
                    bs_write_se( s, h->mb.cache.mv[0][x264_scan8[4*i]][1] - mvp[1] );

                    x264_mb_predict_mv( h, 0, 4*i+1, 1, mvp );
                    bs_write_se( s, h->mb.cache.mv[0][x264_scan8[4*i+1]][0] - mvp[0] );
                    bs_write_se( s, h->mb.cache.mv[0][x264_scan8[4*i+1]][1] - mvp[1] );

                    x264_mb_predict_mv( h, 0, 4*i+2, 1, mvp );
                    bs_write_se( s, h->mb.cache.mv[0][x264_scan8[4*i+2]][0] - mvp[0] );
                    bs_write_se( s, h->mb.cache.mv[0][x264_scan8[4*i+2]][1] - mvp[1] );

                    x264_mb_predict_mv( h, 0, 4*i+3, 1, mvp );
                    bs_write_se( s, h->mb.cache.mv[0][x264_scan8[4*i+3]][0] - mvp[0] );
                    bs_write_se( s, h->mb.cache.mv[0][x264_scan8[4*i+3]][1] - mvp[1] );
                    break;
            }
        }
    }
    else if( i_mb_type == B_8x8 )
    {
        fprintf( stderr, "invalid/unhandled mb_type (B_8x8)\n" );
        return;
    }
    else if( i_mb_type != B_DIRECT )
    {
        /* All B mode */
        /* Motion Vector */
        int i_list;
        int mvp[2];

        int b_list[2][2];

        /* init ref list utilisations */
        for( i = 0; i < 2; i++ )
        {
            b_list[0][i] = x264_mb_type_list0_table[i_mb_type][i];
            b_list[1][i] = x264_mb_type_list1_table[i_mb_type][i];
        }


        if( h->mb.i_partition == D_16x16 )
        {
            if( b_list[0][0] && b_list[1][0] )
            {
                bs_write_ue( s, 3 );
            }
            else if( b_list[1][0] )
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
            if( i_mb_type == B_BI_BI )
            {
                bs_write_ue( s, 20 + (h->mb.i_partition == D_16x8 ? 0 : 1 ) );
            }
            else if( b_list[0][0] && b_list[1][0] )
            {
                /* B_BI_LX* */
                bs_write_ue( s, 16 + (b_list[0][1]?0:2) + (h->mb.i_partition == D_16x8?0:1) );
            }
            else if( b_list[0][1] && b_list[1][1] )
            {
                /* B_LX_BI */
                bs_write_ue( s, 12 + (b_list[0][1]?0:2) + (h->mb.i_partition == D_16x8?0:1) );
            }
            else if( b_list[1][1] )
            {
                /* B_LX_L1 */
                bs_write_ue( s, 6 + (b_list[0][0]?2:0) + (h->mb.i_partition == D_16x8?0:1) );
            }
            else if( b_list[0][1] )
            {
                /* B_LX_L0 */
                bs_write_ue( s, 4 + (b_list[0][0]?0:6) + (h->mb.i_partition == D_16x8?0:1) );
            }
        }

        for( i_list = 0; i_list < 2; i_list++ )
        {
            const int i_ref_max = i_list == 0 ? h->sh.i_num_ref_idx_l0_active : h->sh.i_num_ref_idx_l1_active;

            if( i_ref_max > 1 )
            {
                switch( h->mb.i_partition )
                {
                    case D_16x16:
                        if( b_list[i_list][0] ) bs_write_te( s, i_ref_max - 1, h->mb.cache.ref[i_list][x264_scan8[0]] );
                        break;
                    case D_16x8:
                        if( b_list[i_list][0] ) bs_write_te( s, i_ref_max - 1, h->mb.cache.ref[i_list][x264_scan8[0]] );
                        if( b_list[i_list][1] ) bs_write_te( s, i_ref_max - 1, h->mb.cache.ref[i_list][x264_scan8[8]] );
                        break;
                    case D_8x16:
                        if( b_list[i_list][0] ) bs_write_te( s, i_ref_max - 1, h->mb.cache.ref[i_list][x264_scan8[0]] );
                        if( b_list[i_list][1] ) bs_write_te( s, i_ref_max - 1, h->mb.cache.ref[i_list][x264_scan8[4]] );
                        break;
                }
            }
        }
        for( i_list = 0; i_list < 2; i_list++ )
        {
            switch( h->mb.i_partition )
            {
                case D_16x16:
                    if( b_list[i_list][0] )
                    {
                        x264_mb_predict_mv( h, i_list, 0, 4, mvp );
                        bs_write_se( s, h->mb.cache.mv[i_list][x264_scan8[0]][0] - mvp[0] );
                        bs_write_se( s, h->mb.cache.mv[i_list][x264_scan8[0]][1] - mvp[1] );
                    }
                    break;
                case D_16x8:
                    if( b_list[i_list][0] )
                    {
                        x264_mb_predict_mv( h, i_list, 0, 4, mvp );
                        bs_write_se( s, h->mb.cache.mv[i_list][x264_scan8[0]][0] - mvp[0] );
                        bs_write_se( s, h->mb.cache.mv[i_list][x264_scan8[0]][1] - mvp[1] );
                    }
                    if( b_list[i_list][1] )
                    {
                        x264_mb_predict_mv( h, i_list, 8, 4, mvp );
                        bs_write_se( s, h->mb.cache.mv[i_list][x264_scan8[8]][0] - mvp[0] );
                        bs_write_se( s, h->mb.cache.mv[i_list][x264_scan8[8]][1] - mvp[1] );
                    }
                    break;
                case D_8x16:
                    if( b_list[i_list][0] )
                    {
                        x264_mb_predict_mv( h, i_list, 0, 2, mvp );
                        bs_write_se( s, h->mb.cache.mv[i_list][x264_scan8[0]][0] - mvp[0] );
                        bs_write_se( s, h->mb.cache.mv[i_list][x264_scan8[0]][1] - mvp[1] );
                    }
                    if( b_list[i_list][1] )
                    {
                        x264_mb_predict_mv( h, i_list, 4, 2, mvp );
                        bs_write_se( s, h->mb.cache.mv[i_list][x264_scan8[4]][0] - mvp[0] );
                        bs_write_se( s, h->mb.cache.mv[i_list][x264_scan8[4]][1] - mvp[1] );
                    }
                    break;
            }
        }
    }
    else if( i_mb_type == B_DIRECT )
    {
        bs_write_ue( s, 0 );
    }
    else
    {
        fprintf( stderr, "invalid/unhandled mb_type\n" );
        return;
    }

    /* Coded block patern */
    if( i_mb_type == I_4x4 )
    {
        bs_write_ue( s, intra4x4_cbp_to_golomb[( h->mb.i_cbp_chroma << 4 )|h->mb.i_cbp_luma] );
    }
    else if( i_mb_type != I_16x16 )
    {
        bs_write_ue( s, inter_cbp_to_golomb[( h->mb.i_cbp_chroma << 4 )|h->mb.i_cbp_luma] );
    }

    /* write residual */
    if( i_mb_type == I_16x16 )
    {
        bs_write_se( s, h->mb.qp[h->mb.i_mb_xy] - h->mb.i_last_qp );

        /* DC Luma */
        block_residual_write_cavlc( h, s, BLOCK_INDEX_LUMA_DC , h->dct.luma16x16_dc, 16 );

        if( h->mb.i_cbp_luma != 0 )
        {
            /* AC Luma */
            for( i = 0; i < 16; i++ )
            {
                block_residual_write_cavlc( h, s, i, h->dct.block[i].residual_ac, 15 );
            }
        }
    }
    else if( h->mb.i_cbp_luma != 0 || h->mb.i_cbp_chroma != 0 )
    {
        bs_write_se( s, h->mb.qp[h->mb.i_mb_xy] - h->mb.i_last_qp );

        for( i = 0; i < 16; i++ )
        {
            if( h->mb.i_cbp_luma & ( 1 << ( i / 4 ) ) )
            {
                block_residual_write_cavlc( h, s, i, h->dct.block[i].luma4x4, 16 );
            }
        }
    }
    if( h->mb.i_cbp_chroma != 0 )
    {
        /* Chroma DC residual present */
        block_residual_write_cavlc( h, s, BLOCK_INDEX_CHROMA_DC, h->dct.chroma_dc[0], 4 );
        block_residual_write_cavlc( h, s, BLOCK_INDEX_CHROMA_DC, h->dct.chroma_dc[1], 4 );
        if( h->mb.i_cbp_chroma&0x02 ) /* Chroma AC residual present */
        {
            for( i = 0; i < 8; i++ )
            {
                block_residual_write_cavlc( h, s, 16 + i, h->dct.block[16+i].residual_ac, 15 );
            }
        }
    }
}

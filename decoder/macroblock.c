/*****************************************************************************
 * macroblock.c: h264 decoder library
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar
 * $Id: macroblock.c,v 1.1 2004/06/03 19:27:07 fenrir Exp $
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

#include "common/common.h"
#include "common/vlc.h"
#include "vlc.h"
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

static const int golomb_to_intra4x4_cbp[48]=
{
    47, 31, 15,  0, 23, 27, 29, 30,  7, 11, 13, 14, 39, 43, 45, 46,
    16,  3,  5, 10, 12, 19, 21, 26, 28, 35, 37, 42, 44,  1,  2,  4,
     8, 17, 18, 20, 24,  6,  9, 22, 25, 32, 33, 34, 36, 40, 38, 41
};
static const int golomb_to_inter_cbp[48]=
{
     0, 16,  1,  2,  4,  8, 32,  3,  5, 10, 12, 15, 47,  7, 11, 13,
    14,  6,  9, 31, 35, 37, 42, 44, 33, 34, 36, 40, 39, 43, 45, 46,
    17, 18, 20, 24, 19, 21, 26, 28, 23, 27, 29, 30, 22, 25, 38, 41
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


void x264_mb_partition_ref_set( x264_macroblock_t *mb, int i_list, int i_part, int i_ref )
{
    int x,  y;
    int w,  h;
    int dx, dy;

    x264_mb_partition_getxy( mb, i_part, 0, &x, &y );
    if( mb->i_partition == D_16x16 )
    {
        w = 4; h = 4;
    }
    else if( mb->i_partition == D_16x8 )
    {
        w = 4; h = 2;
    }
    else if( mb->i_partition == D_8x16 )
    {
        w = 2; h = 4;
    }
    else
    {
        /* D_8x8 */
        w = 2; h = 2;
    }

    for( dx = 0; dx < w; dx++ )
    {
        for( dy = 0; dy < h; dy++ )
        {
            mb->partition[x+dx][y+dy].i_ref[i_list] = i_ref;
        }
    }
}

void x264_mb_partition_mv_set( x264_macroblock_t *mb, int i_list, int i_part, int i_sub, int mv[2] )
{
    int x,  y;
    int w,  h;
    int dx, dy;

    x264_mb_partition_getxy( mb, i_part, i_sub, &x, &y );
    x264_mb_partition_size ( mb, i_part, i_sub, &w, &h );

    for( dx = 0; dx < w; dx++ )
    {
        for( dy = 0; dy < h; dy++ )
        {
            mb->partition[x+dx][y+dy].mv[i_list][0] = mv[0];
            mb->partition[x+dx][y+dy].mv[i_list][1] = mv[1];
        }
    }
}


int x264_macroblock_read_cabac( x264_t *h, bs_t *s, x264_macroblock_t *mb )
{
    return -1;
}

static int x264_macroblock_decode_ipcm( x264_t *h, bs_t *s, x264_macroblock_t *mb )
{
    /* TODO */
    return -1;
}


#define BLOCK_INDEX_CHROMA_DC   (-1)
#define BLOCK_INDEX_LUMA_DC     (-2)

static int bs_read_vlc( bs_t *s, x264_vlc_table_t *table )
{
    int i_nb_bits;
    int i_value = 0;
    int i_bits;
    int i_index;
    int i_level = 0;

    i_index = bs_show( s, table->i_lookup_bits );
    if( i_index >= table->i_lookup )
    {
        return( -1 );
    }
    i_value = table->lookup[i_index].i_value;
    i_bits  = table->lookup[i_index].i_size;

    while( i_bits < 0 )
    {
        i_level++;
        if( i_level > 5 )
        {
            return( -1 );        // FIXME what to do ?
        }
        bs_skip( s, table->i_lookup_bits );
        i_nb_bits = -i_bits;

        i_index = bs_show( s, i_nb_bits ) + i_value;
        if( i_index >= table->i_lookup )
        {
            return( -1 );
        }
        i_value = table->lookup[i_index].i_value;
        i_bits  = table->lookup[i_index].i_size;
    }
    bs_skip( s, i_bits );

    return( i_value );
}

static int block_residual_read_cavlc( x264_t *h, bs_t *s, x264_macroblock_t *mb,
                                      int i_idx, int *l, int i_count )
{
    int i;
    int level[16], run[16];
    int i_coeff;

    int i_total, i_trailing;
    int i_suffix_length;
    int i_zero_left;

    for( i = 0; i < i_count; i++ )
    {
        l[i] = 0;
    }

    /* total/trailing */
    if( i_idx == BLOCK_INDEX_CHROMA_DC )
    {
        int i_tt;

        if( ( i_tt = bs_read_vlc( s, h->x264_coeff_token_lookup[4] )) < 0 )
        {
            return -1;
        }

        i_total = i_tt / 4;
        i_trailing = i_tt % 4;
    }
    else
    {
        /* x264_mb_predict_non_zero_code return 0 <-> (16+16+1)>>1 = 16 */
        static const int ct_index[17] = {0,0,1,1,2,2,2,2,3,3,3,3,3,3,3,3,3 };
        int nC;
        int i_tt;

        if( i_idx == BLOCK_INDEX_LUMA_DC )
        {
            nC = x264_mb_predict_non_zero_code( h, mb, 0 );
        }
        else
        {
            nC = x264_mb_predict_non_zero_code( h, mb, i_idx );
        }

        if( ( i_tt = bs_read_vlc( s, h->x264_coeff_token_lookup[ct_index[nC]] ) ) < 0 )
        {
            return -1;
        }

        i_total = i_tt / 4;
        i_trailing = i_tt % 4;
    }

    if( i_idx >= 0 )
    {
        mb->block[i_idx].i_non_zero_count = i_total;
    }

    if( i_total <= 0 )
    {
        return 0;
    }

    i_suffix_length = i_total > 10 && i_trailing < 3 ? 1 : 0;

    for( i = 0; i < i_trailing; i++ )
    {
        level[i] = 1 - 2 * bs_read1( s );
    }

    for( ; i < i_total; i++ )
    {
        int i_prefix;
        int i_level_code;

        i_prefix = bs_read_vlc( s, h->x264_level_prefix_lookup );

        if( i_prefix == -1 )
        {
            return -1;
        }
        else if( i_prefix < 14 )
        {
            if( i_suffix_length > 0 )
            {
                i_level_code = (i_prefix << i_suffix_length) + bs_read( s, i_suffix_length );
            }
            else
            {
                i_level_code = i_prefix;
            }
        }
        else if( i_prefix == 14 )
        {
            if( i_suffix_length > 0 )
            {
                i_level_code = (i_prefix << i_suffix_length) + bs_read( s, i_suffix_length );
            }
            else
            {
                i_level_code = i_prefix + bs_read( s, 4 );
            }
        }
        else /* if( i_prefix == 15 ) */
        {
            i_level_code = (i_prefix << i_suffix_length) + bs_read( s, 12 );
            if( i_suffix_length == 0 )
            {
                i_level_code += 15;
            }
        }
        if( i == i_trailing && i_trailing < 3 )
        {
            i_level_code += 2;
        }
        /* Optimise */
        level[i] = i_level_code&0x01 ? -((i_level_code+1)/2) : (i_level_code+2)/2;

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
            i_zero_left = bs_read_vlc( s, h->x264_total_zeros_dc_lookup[i_total-1] );
        }
        else
        {
            i_zero_left = bs_read_vlc( s, h->x264_total_zeros_lookup[i_total-1] );
        }
        if( i_zero_left < 0 )
        {
            return -1;
        }
    }
    else
    {
        i_zero_left = 0;
    }

    for( i = 0; i < i_total - 1; i++ )
    {
        if( i_zero_left <= 0 )
        {
            break;
        }
        run[i] = bs_read_vlc( s, h->x264_run_before_lookup[X264_MIN( i_zero_left - 1, 6 )] );

        if( run[i] < 0 )
        {
            return -1;
        }
        i_zero_left -= run[i];
    }
    if( i_zero_left < 0 )
    {
        return -1;
    }

    for( ; i < i_total - 1; i++ )
    {
        run[i] = 0;
    }
    run[i_total-1] = i_zero_left;

    i_coeff = -1;
    for( i = i_total - 1; i >= 0; i-- )
    {
        i_coeff += run[i] + 1;
        l[i_coeff] = level[i];
    }

    return 0;
}

static inline void array_zero_set( int *l, int i_count )
{
    int i;

    for( i = 0; i < i_count; i++ )
    {
        l[i] = 0;
    }
}

int x264_macroblock_read_cavlc( x264_t *h, bs_t *s, x264_macroblock_t *mb )
{
    int i_mb_i_offset;
    int i_mb_p_offset;
    int b_sub_ref0 = 0;
    int i_type;
    int i;

    /* read the mb type */
    switch( h->sh.i_type )
    {
        case SLICE_TYPE_I:
            i_mb_p_offset = 0;  /* shut up gcc */
            i_mb_i_offset = 0;
            break;
        case SLICE_TYPE_P:
            i_mb_p_offset = 0;
            i_mb_i_offset = 5;
            break;
        case SLICE_TYPE_B:
            i_mb_p_offset = 23;
            i_mb_i_offset = 23 + 5;
            break;
        default:
            fprintf( stderr, "internal error or slice unsupported\n" );
            return -1;
    }

    i_type = bs_read_ue( s );

    if( i_type < i_mb_i_offset )
    {
        if( i_type < i_mb_p_offset )
        {
            fprintf( stderr, "unsupported mb type(B*)\n" );
            /* TODO for B frame */
            return -1;
        }
        else
        {
            i_type -= i_mb_p_offset;

            if( i_type == 0 )
            {
                mb->i_type = P_L0;
                mb->i_partition = D_16x16;
            }
            else if( i_type == 1 )
            {
                mb->i_type = P_L0;
                mb->i_partition = D_16x8;
            }
            else if( i_type == 2 )
            {
                mb->i_type = P_L0;
                mb->i_partition = D_8x16;
            }
            else if( i_type == 3 || i_type == 4 )
            {
                mb->i_type = P_8x8;
                mb->i_partition = D_8x8;
                b_sub_ref0 = i_type == 4 ? 1 : 0;
            }
            else
            {
                fprintf( stderr, "invalid mb type\n" );
                return -1;
            }
        }
    }
    else
    {
        i_type -= i_mb_i_offset;

        if( i_type == 0 )
        {
            mb->i_type = I_4x4;
        }
        else if( i_type < 25 )
        {
            mb->i_type = I_16x16;

            mb->i_intra16x16_pred_mode = (i_type - 1)%4;
            mb->i_cbp_chroma = ( (i_type-1) / 4 )%3;
            mb->i_cbp_luma   = ((i_type-1) / 12) ? 0x0f : 0x00;
        }
        else if( i_type == 25 )
        {
            mb->i_type = I_PCM;
        }
        else
        {
            fprintf( stderr, "invalid mb type (%d)\n", i_type );
            return -1;
        }
    }

    if( mb->i_type == I_PCM )
    {
        return x264_macroblock_decode_ipcm( h, s, mb );
    }

    if( IS_INTRA( mb->i_type ) )
    {
        if( mb->i_type == I_4x4 )
        {
            for( i = 0; i < 16; i++ )
            {
                int b_coded;

                b_coded = bs_read1( s );

                if( b_coded )
                {
                    mb->block[i].i_intra4x4_pred_mode = x264_mb_predict_intra4x4_mode( h, mb, i );
                }
                else
                {
                    int i_predicted_mode = x264_mb_predict_intra4x4_mode( h, mb, i );
                    int i_mode = bs_read( s, 3 );

                    if( i_mode >= i_predicted_mode )
                    {
                        mb->block[i].i_intra4x4_pred_mode = i_mode + 1;
                    }
                    else
                    {
                        mb->block[i].i_intra4x4_pred_mode = i_mode;
                    }
                }
            }
        }

        mb->i_chroma_pred_mode = bs_read_ue( s );
    }
    else if( mb->i_type == P_8x8 || mb->i_type == B_8x8)
    {
        /* FIXME won't work for B_8x8 */

        for( i = 0; i < 4; i++ )
        {
            int i_sub_partition;

            i_sub_partition = bs_read_ue( s );
            switch( i_sub_partition )
            {
                case 0:
                    mb->i_sub_partition[i] = D_L0_8x8;
                    break;
                case 1:
                    mb->i_sub_partition[i] = D_L0_8x4;
                    break;
                case 2:
                    mb->i_sub_partition[i] = D_L0_4x8;
                    break;
                case 3:
                    mb->i_sub_partition[i] = D_L0_4x4;
                    break;
                default:
                    fprintf( stderr, "invalid i_sub_partition\n" );
                    return -1;
            }
        }
        for( i = 0; i < 4; i++ )
        {
            int i_ref;

            i_ref = b_sub_ref0 ? 0 : bs_read_te( s, h->sh.i_num_ref_idx_l0_active - 1 );
            x264_mb_partition_ref_set( mb, 0, i, i_ref );
        }
        for( i = 0; i < 4; i++ )
        {
            int i_sub;
            int i_ref;

            x264_mb_partition_get( mb, 0, i, 0, &i_ref, NULL, NULL );

            for( i_sub = 0; i_sub < x264_mb_partition_count_table[mb->i_sub_partition[i]]; i_sub++ )
            {
                int mv[2];

                x264_mb_predict_mv( mb, 0, i, i_sub, mv );
                mv[0] += bs_read_se( s );
                mv[1] += bs_read_se( s );

                x264_mb_partition_mv_set( mb, 0, i, i_sub, mv );
            }
        }
    }
    else if( mb->i_type != B_DIRECT )
    {
        /* FIXME will work only for P block */

        /* FIXME using x264_mb_partition_set/x264_mb_partition_get here are too unoptimised
         * I should introduce ref and mv get/set */

        /* Motion Vector */
        int i_part = x264_mb_partition_count_table[mb->i_partition];

        for( i = 0; i < i_part; i++ )
        {
            int i_ref;

            i_ref = bs_read_te( s, h->sh.i_num_ref_idx_l0_active - 1 );

            x264_mb_partition_ref_set( mb, 0, i, i_ref );
        }

        for( i = 0; i < i_part; i++ )
        {
            int mv[2];

            x264_mb_predict_mv( mb, 0, i, 0, mv );

            mv[0] += bs_read_se( s );
            mv[1] += bs_read_se( s );

            x264_mb_partition_mv_set( mb, 0, i, 0, mv );
        }
    }

    if( mb->i_type != I_16x16 )
    {
        int i_cbp;

        i_cbp = bs_read_ue( s );
        if( i_cbp >= 48 )
        {
            fprintf( stderr, "invalid cbp\n" );
            return -1;
        }

        if( mb->i_type == I_4x4 )
        {
            i_cbp = golomb_to_intra4x4_cbp[i_cbp];
        }
        else
        {
            i_cbp = golomb_to_inter_cbp[i_cbp];
        }
        mb->i_cbp_luma   = i_cbp&0x0f;
        mb->i_cbp_chroma = i_cbp >> 4;
    }

    if( mb->i_cbp_luma > 0 || mb->i_cbp_chroma > 0 || mb->i_type == I_16x16 )
    {
        mb->i_qp = bs_read_se( s ) + h->pps->i_pic_init_qp + h->sh.i_qp_delta;

        /* write residual */
        if( mb->i_type == I_16x16 )
        {
            /* DC Luma */
            if( block_residual_read_cavlc( h, s, mb, BLOCK_INDEX_LUMA_DC , mb->luma16x16_dc, 16 ) < 0 )
            {
                return -1;
            }

            if( mb->i_cbp_luma != 0 )
            {
                /* AC Luma */
                for( i = 0; i < 16; i++ )
                {
                    if( block_residual_read_cavlc( h, s, mb, i, mb->block[i].residual_ac, 15 ) < 0 )
                    {
                        return -1;
                    }
                }
            }
            else
            {
                for( i = 0; i < 16; i++ )
                {
                    mb->block[i].i_non_zero_count = 0;
                    array_zero_set( mb->block[i].residual_ac, 15 );
                }
            }
        }
        else
        {
            for( i = 0; i < 16; i++ )
            {
                if( mb->i_cbp_luma & ( 1 << ( i / 4 ) ) )
                {
                    if( block_residual_read_cavlc( h, s, mb, i, mb->block[i].luma4x4, 16 ) < 0 )
                    {
                        return -1;
                    }
                }
                else
                {
                    mb->block[i].i_non_zero_count = 0;
                    array_zero_set( mb->block[i].luma4x4, 16 );
                }
            }
        }

        if( mb->i_cbp_chroma &0x03 )    /* Chroma DC residual present */
        {
            if( block_residual_read_cavlc( h, s, mb, BLOCK_INDEX_CHROMA_DC, mb->chroma_dc[0], 4 ) < 0 ||
                block_residual_read_cavlc( h, s, mb, BLOCK_INDEX_CHROMA_DC, mb->chroma_dc[1], 4 ) < 0 )
            {
                return -1;
            }
        }
        else
        {
            array_zero_set( mb->chroma_dc[0], 4 );
            array_zero_set( mb->chroma_dc[1], 4 );
        }
        if( mb->i_cbp_chroma&0x02 ) /* Chroma AC residual present */
        {
            for( i = 0; i < 8; i++ )
            {
                if( block_residual_read_cavlc( h, s, mb, 16 + i, mb->block[16+i].residual_ac, 15 ) < 0 )
                {
                    return -1;
                }
            }
        }
        else
        {
            for( i = 0; i < 8; i++ )
            {
                mb->block[16+i].i_non_zero_count = 0;
                array_zero_set( mb->block[16+i].residual_ac, 15 );
            }
        }
    }
    else
    {
        mb->i_qp = h->pps->i_pic_init_qp + h->sh.i_qp_delta;
        for( i = 0; i < 16; i++ )
        {
            mb->block[i].i_non_zero_count = 0;
            array_zero_set( mb->block[i].luma4x4, 16 );
        }
        array_zero_set( mb->chroma_dc[0], 4 );
        array_zero_set( mb->chroma_dc[1], 4 );
        for( i = 0; i < 8; i++ )
        {
            array_zero_set( mb->block[16+i].residual_ac, 15 );
            mb->block[16+i].i_non_zero_count = 0;
        }
    }

    //fprintf( stderr, "mb read type=%d\n", mb->i_type );

    return 0;
}




static int x264_mb_pred_mode16x16_valid( x264_macroblock_t *mb, int i_mode )
{
    if( ( mb->i_neighbour & (MB_LEFT|MB_TOP) ) == (MB_LEFT|MB_TOP) )
    {
        return i_mode;
    }
    else if( ( mb->i_neighbour & MB_LEFT ) )
    {
        if( i_mode == I_PRED_16x16_DC )
        {
            return I_PRED_16x16_DC_LEFT;
        }
        else if( i_mode == I_PRED_16x16_H )
        {
            return I_PRED_16x16_H;
        }

        fprintf( stderr, "invalid 16x16 prediction\n" );
        return I_PRED_16x16_DC_LEFT;
    }
    else if( ( mb->i_neighbour & MB_TOP ) )
    {
        if( i_mode == I_PRED_16x16_DC )
        {
            return I_PRED_16x16_DC_TOP;
        }
        else if( i_mode == I_PRED_16x16_V )
        {
            return I_PRED_16x16_V;
        }

        fprintf( stderr, "invalid 16x16 prediction\n" );
        return I_PRED_16x16_DC_TOP;
    }
    else
    {
        return I_PRED_16x16_DC_128;
    }
}

static int x264_mb_pred_mode8x8_valid( x264_macroblock_t *mb, int i_mode )
{
    if( ( mb->i_neighbour & (MB_LEFT|MB_TOP) ) == (MB_LEFT|MB_TOP) )
    {
        return i_mode;
    }
    else if( ( mb->i_neighbour & MB_LEFT ) )
    {
        if( i_mode == I_PRED_CHROMA_DC )
        {
            return I_PRED_CHROMA_DC_LEFT;
        }
        else if( i_mode == I_PRED_CHROMA_H )
        {
            return I_PRED_CHROMA_H;
        }

        fprintf( stderr, "invalid 8x8 prediction\n" );
        return I_PRED_CHROMA_DC_LEFT;
    }
    else if( ( mb->i_neighbour & MB_TOP ) )
    {
        if( i_mode == I_PRED_CHROMA_DC )
        {
            return I_PRED_CHROMA_DC_TOP;
        }
        else if( i_mode == I_PRED_CHROMA_V )
        {
            return I_PRED_CHROMA_V;
        }

        fprintf( stderr, "invalid 8x8 prediction\n" );
        return I_PRED_CHROMA_DC_TOP;
    }
    else
    {
        return I_PRED_CHROMA_DC_128;
    }
}

static int x264_mb_pred_mode4x4_valid( x264_macroblock_t *mb, int idx, int i_mode, int *pb_emu )
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
    int b_emu = 0;

    *pb_emu = 0;

    b_a = (needmb[idx]&mb->i_neighbour&MB_LEFT) == (needmb[idx]&MB_LEFT);
    b_b = (needmb[idx]&mb->i_neighbour&MB_TOP) == (needmb[idx]&MB_TOP);
    b_c = (needmb[idx]&mb->i_neighbour&(MB_TOPRIGHT|MB_PRIVATE)) == (needmb[idx]&(MB_TOPRIGHT|MB_PRIVATE));

    if( b_c == 0 && b_b )
    {
        b_emu = 1;
        b_c = 1;
    }

    /* handle I_PRED_4x4_DC */
    if( i_mode == I_PRED_4x4_DC )
    {
        if( b_a && b_b )
        {
            return I_PRED_4x4_DC;
        }
        else if( b_a )
        {
            return I_PRED_4x4_DC_LEFT;
        }
        else if( b_b )
        {
            return I_PRED_4x4_DC_TOP;
        }
        return I_PRED_4x4_DC_128;
    }

    /* handle 1 dir needed only */
    if( ( b_a && i_mode == I_PRED_4x4_H ) ||
        ( b_b && i_mode == I_PRED_4x4_V ) )
    {
        return i_mode;
    }

    /* handle b_c case (b_b always true) */
    if( b_c && ( i_mode == I_PRED_4x4_DDL || i_mode == I_PRED_4x4_VL ) )
    {
        *pb_emu = b_emu;
        return i_mode;
    }

    if( b_a && b_b )
    {
        /* I_PRED_4x4_DDR, I_PRED_4x4_VR, I_PRED_4x4_HD, I_PRED_4x4_HU */
        return i_mode;
    }

    fprintf( stderr, "invalid 4x4 predict mode(%d, mb:%x-%x idx:%d\n", i_mode, mb->i_mb_x, mb->i_mb_y, idx );
    return I_PRED_CHROMA_DC_128;    /* unefficient */
}

/****************************************************************************
 * UnScan functions
 ****************************************************************************/
static const int scan_zigzag_x[16]={0, 1, 0, 0, 1, 2, 3, 2, 1, 0, 1, 2, 3, 3, 2, 3};
static const int scan_zigzag_y[16]={0, 0, 1, 2, 1, 0, 0, 1, 2, 3, 3, 2, 1, 2, 3, 3};

static inline void unscan_zigzag_4x4full( int16_t dct[4][4], int level[16] )
{
    int i;

    for( i = 0; i < 16; i++ )
    {
        dct[scan_zigzag_y[i]][scan_zigzag_x[i]] = level[i];
    }
}
static inline void unscan_zigzag_4x4( int16_t dct[4][4], int level[15] )
{
    int i;

    for( i = 1; i < 16; i++ )
    {
        dct[scan_zigzag_y[i]][scan_zigzag_x[i]] = level[i - 1];
    }
}

static inline void unscan_zigzag_2x2_dc( int16_t dct[2][2], int level[4] )
{
    dct[0][0] = level[0];
    dct[0][1] = level[1];
    dct[1][0] = level[2];
    dct[1][1] = level[3];
}


int x264_macroblock_decode( x264_t *h, x264_macroblock_t *mb )
{
    x264_mb_context_t *ctx = mb->context;

    int i_qscale;
    int ch;
    int i;

    if( !IS_INTRA(mb->i_type ) )
    {
        /* Motion compensation */
        x264_mb_mc( h, mb );
    }

    /* luma */
    i_qscale = mb->i_qp;
    if( mb->i_type == I_16x16 )
    {
        int     i_mode = x264_mb_pred_mode16x16_valid( mb, mb->i_intra16x16_pred_mode );
        int16_t luma[16][4][4];
        int16_t dct4x4[16+1][4][4];


        /* do the right prediction */
        h->predict_16x16[i_mode]( ctx->p_fdec[0], ctx->i_fdec[0] );

        /* get dc coeffs */
        unscan_zigzag_4x4full( dct4x4[0], mb->luma16x16_dc );
        h->dctf.idct4x4dc( dct4x4[0], dct4x4[0] );
        x264_mb_dequant_4x4_dc( dct4x4[0], i_qscale );

        /* decode the 16x16 macroblock */
        for( i = 0; i < 16; i++ )
        {
            unscan_zigzag_4x4( dct4x4[1+i], mb->block[i].residual_ac );
            x264_mb_dequant_4x4( dct4x4[1+i], i_qscale );

            /* copy dc coeff */
            dct4x4[1+i][0][0] = dct4x4[0][block_idx_y[i]][block_idx_x[i]];

            h->dctf.idct4x4( luma[i], dct4x4[i+1] );
        }
        /* put pixels to fdec */
        h->pixf.add16x16( ctx->p_fdec[0], ctx->i_fdec[0], luma );
    }
    else if( mb->i_type == I_4x4 )
    {
        for( i = 0; i < 16; i++ )
        {
            int16_t luma[4][4];
            int16_t dct4x4[4][4];

            uint8_t *p_dst_by;
            int     i_mode;
            int     b_emu;

            /* Do the right prediction */
            p_dst_by = ctx->p_fdec[0] + 4 * block_idx_x[i] + 4 * block_idx_y[i] * ctx->i_fdec[0];
            i_mode   = x264_mb_pred_mode4x4_valid( mb, i, mb->block[i].i_intra4x4_pred_mode, &b_emu );
            if( b_emu )
            {
                fprintf( stderr, "mmmh b_emu\n" );
                memset( &p_dst_by[4], p_dst_by[3], 4 );
            }
            h->predict_4x4[i_mode]( p_dst_by, ctx->i_fdec[0] );

            if( mb->block[i].i_non_zero_count > 0 )
            {
                /* decode one 4x4 block */
                unscan_zigzag_4x4full( dct4x4, mb->block[i].luma4x4 );

                x264_mb_dequant_4x4( dct4x4, i_qscale );

                h->dctf.idct4x4( luma, dct4x4 );

                h->pixf.add4x4( p_dst_by, ctx->i_fdec[0], luma );
            }
        }
    }
    else /* Inter mb */
    {
        for( i = 0; i < 16; i++ )
        {
            uint8_t *p_dst_by;
            int16_t luma[4][4];
            int16_t dct4x4[4][4];

            if( mb->block[i].i_non_zero_count > 0 )
            {
                unscan_zigzag_4x4full( dct4x4, mb->block[i].luma4x4 );
                x264_mb_dequant_4x4( dct4x4, i_qscale );

                h->dctf.idct4x4( luma, dct4x4 );

                p_dst_by = ctx->p_fdec[0] + 4 * block_idx_x[i] + 4 * block_idx_y[i] * ctx->i_fdec[0];
                h->pixf.add4x4( p_dst_by, ctx->i_fdec[0], luma );
            }
        }
    }

    /* chroma */
    i_qscale = i_chroma_qp_table[x264_clip3( i_qscale + h->pps->i_chroma_qp_index_offset, 0, 51 )];
    if( IS_INTRA( mb->i_type ) )
    {
        int i_mode = x264_mb_pred_mode8x8_valid( mb, mb->i_chroma_pred_mode );
        /* do the right prediction */
        h->predict_8x8[i_mode]( ctx->p_fdec[1], ctx->i_fdec[1] );
        h->predict_8x8[i_mode]( ctx->p_fdec[2], ctx->i_fdec[2] );
    }

    if( mb->i_cbp_chroma != 0 )
    {
        for( ch = 0; ch < 2; ch++ )
        {
            int16_t chroma[4][4][4];
            int16_t dct2x2[2][2];
            int16_t dct4x4[4][4][4];

            /* get dc chroma */
            unscan_zigzag_2x2_dc( dct2x2, mb->chroma_dc[ch] );
            h->dctf.idct2x2dc( dct2x2, dct2x2 );
            x264_mb_dequant_2x2_dc( dct2x2, i_qscale );

            for( i = 0; i < 4; i++ )
            {
                unscan_zigzag_4x4( dct4x4[i], mb->block[16+i+ch*4].residual_ac );
                x264_mb_dequant_4x4( dct4x4[i], i_qscale );

                /* copy dc coeff */
                dct4x4[i][0][0] = dct2x2[block_idx_y[i]][block_idx_x[i]];

                h->dctf.idct4x4( chroma[i], dct4x4[i] );
            }
            h->pixf.add8x8( ctx->p_fdec[1+ch], ctx->i_fdec[1+ch], chroma );
        }
    }

    return 0;
}

void x264_macroblock_decode_skip( x264_t *h, x264_macroblock_t *mb )
{
    int i;
    int x, y;
    int mv[2];

    /* decode it as a 16x16 with no luma/chroma */
    mb->i_type = P_L0;
    mb->i_partition = D_16x16;
    mb->i_cbp_luma = 0;
    mb->i_cbp_chroma = 0;
    for( i = 0; i < 16 + 8; i++ )
    {
        mb->block[i].i_non_zero_count = 0;
    }
    for( i = 0; i < 16; i++ )
    {
        array_zero_set( mb->block[i].luma4x4, 16 );
    }
    array_zero_set( mb->chroma_dc[0], 4 );
    array_zero_set( mb->chroma_dc[1], 4 );
    for( i = 0; i < 8; i++ )
    {
        array_zero_set( mb->block[16+i].residual_ac, 15 );
    }

    /* set ref0 */
    for( x = 0; x < 4; x++ )
    {
        for( y = 0; y < 4; y++ )
        {
            mb->partition[x][y].i_ref[0] = 0;
        }
    }
    /* get mv */
    x264_mb_predict_mv_pskip( mb, mv );

    x264_mb_partition_mv_set( mb, 0, 0, 0, mv );

    /* Motion compensation */
    x264_mb_mc( h, mb );

    mb->i_type = P_SKIP;
}


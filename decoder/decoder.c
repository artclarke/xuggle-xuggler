/*****************************************************************************
 * x264: h264 decoder
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar
 * $Id: decoder.c,v 1.1 2004/06/03 19:27:07 fenrir Exp $
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
#include "common/cpu.h"
#include "common/vlc.h"

#include "macroblock.h"
#include "set.h"
#include "vlc.h"


static void x264_slice_idr( x264_t *h )
{
    int i;

    h->i_poc_msb = 0;
    h->i_poc_lsb = 0;
    h->i_frame_offset = 0;
    h->i_frame_num = 0;

    if( h->sps )
    {
        for( i = 0; i < h->sps->i_num_ref_frames + 1; i++ )
        {
            h->freference[i]->i_poc = -1;
        }

        h->fdec = h->freference[0];
        h->i_ref0 = 0;
        h->i_ref1 = 0;
    }
}

/* The slice reading is split in two part:
 *      - before ref_pic_list_reordering( )
 *      - after  dec_ref_pic_marking( )
 */
static int x264_slice_header_part1_read( bs_t *s,
                                         x264_slice_header_t *sh, x264_sps_t sps_array[32], x264_pps_t pps_array[256], int b_idr )
{
    sh->i_first_mb = bs_read_ue( s );
    sh->i_type = bs_read_ue( s );
    if( sh->i_type >= 5 )
    {
        sh->i_type -= 5;
    }
    sh->i_pps_id = bs_read_ue( s );
    if( bs_eof( s ) || sh->i_pps_id >= 256 || pps_array[sh->i_pps_id].i_id == -1 )
    {
        fprintf( stderr, "invalid pps_id in slice header\n" );
        return -1;
    }

    sh->pps = &pps_array[sh->i_pps_id];
    sh->sps = &sps_array[sh->pps->i_sps_id];    /* valid if pps valid */

    sh->i_frame_num = bs_read( s, sh->sps->i_log2_max_frame_num );
    if( !sh->sps->b_frame_mbs_only )
    {
        sh->b_field_pic = bs_read1( s );
        if( sh->b_field_pic )
        {
            sh->b_bottom_field = bs_read1( s );
        }
    }

    if( b_idr )
    {
        sh->i_idr_pic_id = bs_read_ue( s );
    }
    else
    {
        sh->i_idr_pic_id = 0;
    }

    if( sh->sps->i_poc_type == 0 )
    {
        sh->i_poc_lsb = bs_read( s, sh->sps->i_log2_max_poc_lsb );
        if( sh->pps->b_pic_order && !sh->b_field_pic )
        {
            sh->i_delta_poc_bottom = bs_read_se( s );
        }
    }
    else if( sh->sps->i_poc_type == 1 && !sh->sps->b_delta_pic_order_always_zero )
    {
        sh->i_delta_poc[0] = bs_read_se( s );
        if( sh->pps->b_pic_order && !sh->b_field_pic )
        {
            sh->i_delta_poc[1] = bs_read_se( s );
        }
    }

    if( sh->pps->b_redundant_pic_cnt )
    {
        sh->i_redundant_pic_cnt = bs_read_ue( s );
    }

    if( sh->i_type == SLICE_TYPE_B )
    {
        sh->b_direct_spatial_mv_pred = bs_read1( s );
    }

    if( sh->i_type == SLICE_TYPE_P || sh->i_type == SLICE_TYPE_SP || sh->i_type == SLICE_TYPE_B )
    {
        sh->b_num_ref_idx_override = bs_read1( s );

        sh->i_num_ref_idx_l0_active = sh->pps->i_num_ref_idx_l0_active; /* default */
        sh->i_num_ref_idx_l1_active = sh->pps->i_num_ref_idx_l1_active; /* default */

        if( sh->b_num_ref_idx_override )
        {
            sh->i_num_ref_idx_l0_active = bs_read_ue( s ) + 1;
            if( sh->i_type == SLICE_TYPE_B )
            {
                sh->i_num_ref_idx_l1_active = bs_read_ue( s ) + 1;
            }
        }
    }

    return bs_eof( s ) ? -1 : 0;
}

static int x264_slice_header_part2_read( bs_t *s, x264_slice_header_t *sh )
{
    if( sh->pps->b_cabac && sh->i_type != SLICE_TYPE_I && sh->i_type != SLICE_TYPE_SI )
    {
        sh->i_cabac_init_idc = bs_read_ue( s );
    }
    sh->i_qp_delta = bs_read_se( s );

    if( sh->i_type == SLICE_TYPE_SI || sh->i_type == SLICE_TYPE_SP )
    {
        if( sh->i_type == SLICE_TYPE_SP )
        {
            sh->b_sp_for_swidth = bs_read1( s );
        }
        sh->i_qs_delta = bs_read_se( s );
    }

    if( sh->pps->b_deblocking_filter_control )
    {
        sh->i_disable_deblocking_filter_idc = bs_read_ue( s );
        if( sh->i_disable_deblocking_filter_idc != 1 )
        {
            sh->i_alpha_c0_offset = bs_read_se( s );
            sh->i_beta_offset = bs_read_se( s );
        }
    }
    else
    {
        sh->i_alpha_c0_offset = 0;
        sh->i_beta_offset = 0;
    }

    if( sh->pps->i_num_slice_groups > 1 && sh->pps->i_slice_group_map_type >= 3 && sh->pps->i_slice_group_map_type <= 5 )
    {
        /* FIXME */
        return -1;
    }
    return 0;
}

static int x264_slice_header_ref_pic_reordering( x264_t *h, bs_t *s )
{
    int b_ok;
    int i;

    /* use the no more use frame */
    h->fdec = h->freference[0];
    h->fdec->i_poc = h->i_poc;

    /* build ref list 0/1 */
    h->i_ref0 = 0;
    h->i_ref1 = 0;
    for( i = 1; i < h->sps->i_num_ref_frames + 1; i++ )
    {
        if( h->freference[i]->i_poc >= 0 )
        {
            if( h->freference[i]->i_poc < h->fdec->i_poc )
            {
                h->fref0[h->i_ref0++] = h->freference[i];
            }
            else if( h->freference[i]->i_poc > h->fdec->i_poc )
            {
                h->fref1[h->i_ref1++] = h->freference[i];
            }
        }
    }

    /* Order ref0 from higher to lower poc */
    do
    {
        b_ok = 1;
        for( i = 0; i < h->i_ref0 - 1; i++ )
        {
            if( h->fref0[i]->i_poc < h->fref0[i+1]->i_poc )
            {
                x264_frame_t *tmp = h->fref0[i+1];

                h->fref0[i+1] = h->fref0[i];
                h->fref0[i] = tmp;
                b_ok = 0;
                break;
            }
        }
    } while( !b_ok );
    /* Order ref1 from lower to higher poc (bubble sort) for B-frame */
    do
    {
        b_ok = 1;
        for( i = 0; i < h->i_ref1 - 1; i++ )
        {
            if( h->fref1[i]->i_poc > h->fref1[i+1]->i_poc )
            {
                x264_frame_t *tmp = h->fref1[i+1];

                h->fref1[i+1] = h->fref1[i];
                h->fref1[i] = tmp;
                b_ok = 0;
                break;
            }
        }
    } while( !b_ok );

    if( h->i_ref0 > h->pps->i_num_ref_idx_l0_active )
    {
        h->i_ref0 = h->pps->i_num_ref_idx_l0_active;
    }
    if( h->i_ref1 > h->pps->i_num_ref_idx_l1_active )
    {
        h->i_ref1 = h->pps->i_num_ref_idx_l1_active;
    }

    //fprintf( stderr,"POC:%d ref0=%d POC0=%d\n", h->fdec->i_poc, h->i_ref0, h->i_ref0 > 0 ? h->fref0[0]->i_poc : -1 );


    /* Now parse the stream and change the default order */
    if( h->sh.i_type != SLICE_TYPE_I && h->sh.i_type != SLICE_TYPE_SI )
    {
        int b_reorder = bs_read1( s );

        if( b_reorder )
        {
            /* FIXME */
            return -1;
        }
    }
    if( h->sh.i_type == SLICE_TYPE_B )
    {
        int b_reorder = bs_read1( s );
        if( b_reorder )
        {
            /* FIXME */
            return -1;
        }
    }
    return 0;
}

static int x264_slice_header_pred_weight_table( x264_t *h, bs_t *s )
{
    return -1;
}

static int  x264_slice_header_dec_ref_pic_marking( x264_t *h, bs_t *s, int i_nal_type  )
{
    if( i_nal_type == NAL_SLICE_IDR )
    {
        int b_no_output_of_prior_pics = bs_read1( s );
        int b_long_term_reference_flag = bs_read1( s );

        /* TODO */
        if( b_no_output_of_prior_pics )
        {

        }

        if( b_long_term_reference_flag )
        {

        }
    }
    else
    {
        int b_adaptive_ref_pic_marking_mode = bs_read1( s );
        if( b_adaptive_ref_pic_marking_mode )
        {
            return -1;
        }
    }
    return 0;
}

/****************************************************************************
 * Decode a slice header and setup h for mb decoding.
 ****************************************************************************/
static int x264_slice_header_decode( x264_t *h, bs_t *s, x264_nal_t *nal )
{
    /* read the first part of the slice */
    if( x264_slice_header_part1_read( s, &h->sh,
                                      h->sps_array, h->pps_array,
                                      nal->i_type == NAL_SLICE_IDR ? 1 : 0 ) < 0 )
    {
        fprintf( stderr, "x264_slice_header_part1_read failed\n" );
        return -1;
    }

    /* now reset h if needed for this frame */
    if( h->sps != h->sh.sps || h->pps != h->sh.pps )
    {
        int i;
        /* TODO */

        h->sps = NULL;
        h->pps = NULL;
        if( h->picture->i_width != 0 && h->picture->i_height != 0 )
        {
            for( i = 0; i < h->sps->i_num_ref_frames + 1; i++ )
            {
                x264_frame_delete( h->freference[i]);
            }
            free( h->mb );
        }

        h->picture->i_width = 0;
        h->picture->i_height = 0;
    }

    /* and init if needed */
    if( h->sps == NULL || h->pps == NULL )
    {
        int i;

        h->sps = h->sh.sps;
        h->pps = h->sh.pps;

        h->param.i_width = h->picture->i_width = 16 * h->sps->i_mb_width;
        h->param.i_height= h->picture->i_height= 16 * h->sps->i_mb_height;

        fprintf( stderr, "x264: %dx%d\n", h->picture->i_width, h->picture->i_height );

        h->mb = x264_macroblocks_new( h->sps->i_mb_width, h->sps->i_mb_height );

        for( i = 0; i < h->sps->i_num_ref_frames + 1; i++ )
        {
            h->freference[i] = x264_frame_new( h );
            h->freference[i]->i_poc = -1;
        }
        h->fdec = h->freference[0];
        h->i_ref0 = 0;
        h->i_ref1 = 0;

        h->i_poc_msb = 0;
        h->i_poc_lsb = 0;
        h->i_frame_offset = 0;
        h->i_frame_num = 0;
    }

    /* calculate poc for current frame */
    if( h->sps->i_poc_type == 0 )
    {
        int i_max_poc_lsb = 1 << h->sps->i_log2_max_poc_lsb;

        if( h->sh.i_poc_lsb < h->i_poc_lsb && h->i_poc_lsb - h->sh.i_poc_lsb >= i_max_poc_lsb/2 )
        {
            h->i_poc_msb += i_max_poc_lsb;
        }
        else if( h->sh.i_poc_lsb > h->i_poc_lsb  && h->sh.i_poc_lsb - h->i_poc_lsb > i_max_poc_lsb/2 )
        {
            h->i_poc_msb -= i_max_poc_lsb;
        }
        h->i_poc_lsb = h->sh.i_poc_lsb;

        h->i_poc = h->i_poc_msb + h->sh.i_poc_lsb;
    }
    else if( h->sps->i_poc_type == 1 )
    {
        /* FIXME */
        return -1;
    }
    else
    {
        if( nal->i_type == NAL_SLICE_IDR )
        {
            h->i_frame_offset = 0;
            h->i_poc = 0;
        }
        else
        {
            if( h->sh.i_frame_num < h->i_frame_num )
            {
                h->i_frame_offset += 1 << h->sps->i_log2_max_frame_num;
            }
            if( nal->i_ref_idc > 0 )
            {
                h->i_poc = 2 * ( h->i_frame_offset + h->sh.i_frame_num );
            }
            else
            {
                h->i_poc = 2 * ( h->i_frame_offset + h->sh.i_frame_num ) - 1;
            }
        }
        h->i_frame_num = h->sh.i_frame_num;
    }

    fprintf( stderr, "x264: pic type=%s poc:%d\n",
             h->sh.i_type == SLICE_TYPE_I ? "I" : (h->sh.i_type == SLICE_TYPE_P ? "P" : "B?" ),
             h->i_poc );

    if( h->sh.i_type != SLICE_TYPE_I && h->sh.i_type != SLICE_TYPE_P )
    {
        fprintf( stderr, "only SLICE I/P supported\n" );
        return -1;
    }

    /* read and do the ref pic reordering */
    if( x264_slice_header_ref_pic_reordering( h, s ) < 0 )
    {
        return -1;
    }

    if( ( (h->sh.i_type == SLICE_TYPE_P || h->sh.i_type == SLICE_TYPE_SP) && h->sh.pps->b_weighted_pred  ) ||
        ( h->sh.i_type == SLICE_TYPE_B && h->sh.pps->b_weighted_bipred ) )
    {
        if( x264_slice_header_pred_weight_table( h, s ) < 0 )
        {
            return -1;
        }
    }

    if( nal->i_ref_idc != 0 )
    {
        x264_slice_header_dec_ref_pic_marking( h, s, nal->i_type );
    }

    if( x264_slice_header_part2_read( s, &h->sh ) < 0 )
    {
        return -1;
    }

    return 0;
}

static int x264_slice_data_decode( x264_t *h, bs_t *s )
{
    int mb_xy = h->sh.i_first_mb;
    int i_ret = 0;

    if( h->pps->b_cabac )
    {
        /* TODO: alignement and cabac init */
    }

    /* FIXME field decoding */
    for( ;; )
    {
        x264_mb_context_t context;
        x264_macroblock_t *mb;

        if( mb_xy >= h->sps->i_mb_width * h->sps->i_mb_height )
        {
            break;
        }

        mb = &h->mb[mb_xy];

        /* load neighbour */
        x264_macroblock_context_load( h, mb, &context );


        if( h->pps->b_cabac )
        {
            if( h->sh.i_type != SLICE_TYPE_I && h->sh.i_type != SLICE_TYPE_SI )
            {
                /* TODO */
            }
            i_ret = x264_macroblock_read_cabac( h, s, mb );
        }
        else
        {
            if( h->sh.i_type != SLICE_TYPE_I && h->sh.i_type != SLICE_TYPE_SI )
            {
                int i_skip = bs_read_ue( s );

                while( i_skip > 0 )
                {
                    x264_macroblock_decode_skip( h, mb );

                    /* next macroblock */
                    mb_xy++;
                    if( mb_xy >= h->sps->i_mb_width * h->sps->i_mb_height )
                    {
                        break;
                    }
                    mb++;

                    /* load neighbour */
                    x264_macroblock_context_load( h, mb, &context );

                    i_skip--;
                }
                if( mb_xy >= h->sps->i_mb_width * h->sps->i_mb_height )
                {
                    break;
                }
            }
            i_ret = x264_macroblock_read_cavlc( h, s, mb );
        }

        if( i_ret < 0 )
        {
            fprintf( stderr, "x264_macroblock_read failed [%d,%d]\n", mb->i_mb_x, mb->i_mb_y );
            break;
        }

        if( x264_macroblock_decode( h, mb ) < 0 )
        {
            fprintf( stderr, "x264_macroblock_decode failed\n" );
            /* try to do some error correction ;) */
        }

        mb_xy++;
    }

    if( i_ret >= 0 )
    {
        int i;

        /* expand border for frame reference TODO avoid it when using b-frame */
        x264_frame_expand_border( h->fdec );

        /* apply deblocking filter to the current decoded picture */
        if( !h->pps->b_deblocking_filter_control || h->sh.i_disable_deblocking_filter_idc != 1 )
        {
            x264_frame_deblocking_filter( h, h->sh.i_type );
        }

#if 0
        /* expand border for frame reference TODO avoid it when using b-frame */
        x264_frame_expand_border( h->fdec );
#endif

        h->picture->i_plane = h->fdec->i_plane;
        for( i = 0; i < h->picture->i_plane; i++ )
        {
            h->picture->i_stride[i] = h->fdec->i_stride[i];
            h->picture->plane[i]    = h->fdec->plane[i];
        }

        /* move frame in the buffer FIXME won't work for B-frame */
        h->fdec = h->freference[h->sps->i_num_ref_frames];
        for( i = h->sps->i_num_ref_frames; i > 0; i-- )
        {
            h->freference[i] = h->freference[i-1];
        }
        h->freference[0] = h->fdec;
    }

    return i_ret;
}

/****************************************************************************
 *
 ******************************* x264 libs **********************************
 *
 ****************************************************************************/

/****************************************************************************
 * x264_decoder_open:
 ****************************************************************************/
x264_t *x264_decoder_open   ( x264_param_t *param )
{
    x264_t *h = x264_malloc( sizeof( x264_t ) );
    int i;

    memcpy( &h->param, param, sizeof( x264_param_t ) );

    h->cpu = param->cpu;

    /* no SPS and PPS active yet */
    h->sps = NULL;
    h->pps = NULL;

    for( i = 0; i < 32; i++ )
    {
        h->sps_array[i].i_id = -1;  /* invalidate it */
    }
    for( i = 0; i < 256; i++ )
    {
        h->pps_array[i].i_id = -1;  /* invalidate it */
    }

    h->picture = x264_malloc( sizeof( x264_picture_t ) );
    h->picture->i_width = 0;
    h->picture->i_height= 0;

    /* init predict_XxX */
    x264_predict_16x16_init( h->cpu, h->predict_16x16 );
    x264_predict_8x8_init( h->cpu, h->predict_8x8 );
    x264_predict_4x4_init( h->cpu, h->predict_4x4 );

    x264_pixel_init( h->cpu, &h->pixf );
    x264_dct_init( h->cpu, &h->dctf );

    x264_mc_init( h->cpu, h->mc );

    /* create the vlc table (we could remove it from x264_t but it will need
     * to introduce a x264_init() for global librarie) */
    for( i = 0; i < 5; i++ )
    {
        /* max 2 step */
        h->x264_coeff_token_lookup[i] = x264_vlc_table_lookup_new( x264_coeff_token[i], 17*4, 4 );
    }
    /* max 2 step */
    h->x264_level_prefix_lookup = x264_vlc_table_lookup_new( x264_level_prefix, 16, 8 );

    for( i = 0; i < 15; i++ )
    {
        /* max 1 step */
        h->x264_total_zeros_lookup[i] = x264_vlc_table_lookup_new( x264_total_zeros[i], 16, 9 );
    }
    for( i = 0;i < 3; i++ )
    {
        /* max 1 step */
        h->x264_total_zeros_dc_lookup[i] = x264_vlc_table_lookup_new( x264_total_zeros_dc[i], 4, 3 );
    }
    for( i = 0;i < 7; i++ )
    {
        /* max 2 step */
        h->x264_run_before_lookup[i] = x264_vlc_table_lookup_new( x264_run_before[i], 15, 6 );
    }

    return h;
}

/****************************************************************************
 * x264_decoder_decode: decode one nal unit
 ****************************************************************************/
int     x264_decoder_decode( x264_t *h,
                             x264_picture_t **pp_pic, x264_nal_t *nal )
{
    int i_ret = 0;
    bs_t bs;

    /* no picture */
    *pp_pic = NULL;

    /* init bitstream reader */
    bs_init( &bs, nal->p_payload, nal->i_payload );

    switch( nal->i_type )
    {
        case NAL_SPS:
            if( ( i_ret = x264_sps_read( &bs, h->sps_array ) ) < 0 )
            {
                fprintf( stderr, "x264: x264_sps_read failed\n" );
            }
            break;

        case NAL_PPS:
            if( ( i_ret = x264_pps_read( &bs, h->pps_array ) ) < 0 )
            {
                fprintf( stderr, "x264: x264_pps_read failed\n" );
            }
            break;

        case NAL_SLICE_IDR:
            fprintf( stderr, "x264: NAL_SLICE_IDR\n" );
            x264_slice_idr( h );

        case NAL_SLICE:
            if( ( i_ret = x264_slice_header_decode( h, &bs, nal ) ) < 0 )
            {
                fprintf( stderr, "x264: x264_slice_header_decode failed\n" );
            }
            if( h->sh.i_redundant_pic_cnt == 0 && i_ret == 0 )
            {
                if( ( i_ret = x264_slice_data_decode( h, &bs ) ) < 0 )
                {
                    fprintf( stderr, "x264: x264_slice_data_decode failed\n" );
                }
                else
                {
                    *pp_pic = h->picture;
                }
            }
            break;

        case NAL_SLICE_DPA:
        case NAL_SLICE_DPB:
        case NAL_SLICE_DPC:
            fprintf( stderr, "partitioned stream unsupported\n" );
            i_ret = -1;
            break;

        case NAL_SEI:
        default:
            break;
    }

    /* restore CPU state (before using float again) */
    x264_cpu_restore( h->cpu );

    return i_ret;
}

/****************************************************************************
 * x264_decoder_close:
 ****************************************************************************/
void    x264_decoder_close  ( x264_t *h )
{
    int i;

    if( h->picture->i_width != 0 && h->picture->i_height != 0 )
    {
        for( i = 0; i < h->sps->i_num_ref_frames + 1; i++ )
        {
            x264_frame_delete( h->freference[i]);
        }
        x264_free( h->mb );
    }

    /* free vlc table */
    for( i = 0; i < 5; i++ )
    {
        x264_vlc_table_lookup_delete( h->x264_coeff_token_lookup[i] );
    }
    x264_vlc_table_lookup_delete( h->x264_level_prefix_lookup );

    for( i = 0; i < 15; i++ )
    {
        x264_vlc_table_lookup_delete( h->x264_total_zeros_lookup[i] );
    }
    for( i = 0;i < 3; i++ )
    {
        x264_vlc_table_lookup_delete( h->x264_total_zeros_dc_lookup[i] );
    }
    for( i = 0;i < 7; i++ )
    {
        x264_vlc_table_lookup_delete( h->x264_run_before_lookup[i] );
    }

    x264_free( h->picture );
    x264_free( h );
}


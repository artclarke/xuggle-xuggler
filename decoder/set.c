/*****************************************************************************
 * x264: h264 decoder
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar
 * $Id: set.c,v 1.1 2004/06/03 19:27:07 fenrir Exp $
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
#include "set.h"

/* return -1 if invalid, else the id */
int x264_sps_read( bs_t *s, x264_sps_t sps_array[32] )
{
    x264_sps_t *sps;

    int i_profile_idc;
    int i_level_idc;

    int b_constraint_set0;
    int b_constraint_set1;
    int b_constraint_set2;

    int id;

    i_profile_idc     = bs_read( s, 8 );
    b_constraint_set0 = bs_read( s, 1 );
    b_constraint_set1 = bs_read( s, 1 );
    b_constraint_set2 = bs_read( s, 1 );

    bs_skip( s, 5 );    /* reserved */
    i_level_idc       = bs_read( s, 8 );


    id = bs_read_ue( s );
    if( bs_eof( s ) || id >= 32 )
    {
        /* the sps is invalid, no need to corrupt sps_array[0] */
        return -1;
    }

    sps = &sps_array[id];
    sps->i_id = id;

    /* put pack parsed value */
    sps->i_profile_idc     = i_profile_idc;
    sps->i_level_idc       = i_level_idc;
    sps->b_constraint_set0 = b_constraint_set0;
    sps->b_constraint_set1 = b_constraint_set1;
    sps->b_constraint_set2 = b_constraint_set2;

    sps->i_log2_max_frame_num = bs_read_ue( s ) + 4;

    sps->i_poc_type = bs_read_ue( s );
    if( sps->i_poc_type == 0 )
    {
        sps->i_log2_max_poc_lsb = bs_read_ue( s ) + 4;
    }
    else if( sps->i_poc_type == 1 )
    {
        int i;
        sps->b_delta_pic_order_always_zero = bs_read( s, 1 );
        sps->i_offset_for_non_ref_pic = bs_read_se( s );
        sps->i_offset_for_top_to_bottom_field = bs_read_se( s );
        sps->i_num_ref_frames_in_poc_cycle = bs_read_ue( s );
        if( sps->i_num_ref_frames_in_poc_cycle > 256 )
        {
            /* FIXME what to do */
            sps->i_num_ref_frames_in_poc_cycle = 256;
        }
        for( i = 0; i < sps->i_num_ref_frames_in_poc_cycle; i++ )
        {
            sps->i_offset_for_ref_frame[i] = bs_read_se( s );
        }
    }
    else if( sps->i_poc_type > 2 )
    {
        goto error;
    }

    sps->i_num_ref_frames = bs_read_ue( s );
    sps->b_gaps_in_frame_num_value_allowed = bs_read( s, 1 );
    sps->i_mb_width = bs_read_ue( s ) + 1;
    sps->i_mb_height= bs_read_ue( s ) + 1;
    sps->b_frame_mbs_only = bs_read( s, 1 );
    if( !sps->b_frame_mbs_only )
    {
        sps->b_mb_adaptive_frame_field = bs_read( s, 1 );
    }
    else
    {
        sps->b_mb_adaptive_frame_field = 0;
    }
    sps->b_direct8x8_inference = bs_read( s, 1 );

    sps->b_crop = bs_read( s, 1 );
    if( sps->b_crop )
    {
        sps->crop.i_left  = bs_read_ue( s );
        sps->crop.i_right = bs_read_ue( s );
        sps->crop.i_top   = bs_read_ue( s );
        sps->crop.i_bottom= bs_read_ue( s );
    }
    else
    {
        sps->crop.i_left  = 0;
        sps->crop.i_right = 0;
        sps->crop.i_top   = 0;
        sps->crop.i_bottom= 0;
    }

    sps->b_vui = bs_read( s, 1 );
    if( sps->b_vui )
    {
        /* FIXME */
    }
    else
    {

    }

    if( bs_eof( s ) )
    {
        /* no rbsp trailing */
        fprintf( stderr, "incomplete SPS\n" );
        goto error;
    }

    fprintf( stderr, "x264_sps_read: sps:0x%x profile:%d/%d poc:%d ref:%d %xx%d crop:%d-%d-%d-%d\n",
             sps->i_id,
             sps->i_profile_idc, sps->i_level_idc,
             sps->i_poc_type,
             sps->i_num_ref_frames,
             sps->i_mb_width, sps->i_mb_height,
             sps->crop.i_left, sps->crop.i_right,
             sps->crop.i_top, sps->crop.i_bottom );

    return id;

error:
    /* invalidate this sps */
    sps->i_id = -1;
    return -1;
}

/* return -1 if invalid, else the id */
int x264_pps_read( bs_t *s, x264_pps_t pps_array[256] )
{
    x264_pps_t *pps;
    int id;
    int i;

    id = bs_read_ue( s );
    if( bs_eof( s ) || id >= 256 )
    {
        fprintf( stderr, "id invalid\n" );
        return -1;
    }
    pps = &pps_array[id];
    pps->i_id = id;
    pps->i_sps_id = bs_read_ue( s );
    if( pps->i_sps_id >= 32 )
    {
        goto error;
    }
    pps->b_cabac = bs_read( s, 1 );
    pps->b_pic_order = bs_read( s, 1 );
    pps->i_num_slice_groups = bs_read_ue( s ) + 1;
    if( pps->i_num_slice_groups > 1 )
    {
        fprintf( stderr, "FMO unsupported\n " );

        pps->i_slice_group_map_type  =bs_read_ue( s );
        if( pps->i_slice_group_map_type == 0 )
        {
            for( i = 0; i < pps->i_num_slice_groups; i++ )
            {
                pps->i_run_length[i] = bs_read_ue( s );
            }
        }
        else if( pps->i_slice_group_map_type == 2 )
        {
            for( i = 0; i < pps->i_num_slice_groups; i++ )
            {
                pps->i_top_left[i] = bs_read_ue( s );
                pps->i_bottom_right[i] = bs_read_ue( s );
            }
        }
        else if( pps->i_slice_group_map_type == 3 ||
                 pps->i_slice_group_map_type == 4 ||
                 pps->i_slice_group_map_type == 5 )
        {
            pps->b_slice_group_change_direction = bs_read( s, 1 );
            pps->i_slice_group_change_rate = bs_read_ue( s ) + 1;
        }
        else if( pps->i_slice_group_map_type == 6 )
        {
            pps->i_pic_size_in_map_units = bs_read_ue( s ) + 1;
            for( i = 0; i < pps->i_pic_size_in_map_units; i++ )
            {
               /*  FIXME */
                /* pps->i_slice_group_id = bs_read( s, ceil( log2( pps->i_pic_size_in_map_units +1 ) ) ); */
            }
        }
    }
    pps->i_num_ref_idx_l0_active = bs_read_ue( s ) + 1;
    pps->i_num_ref_idx_l1_active = bs_read_ue( s ) + 1;
    pps->b_weighted_pred = bs_read( s, 1 );
    pps->b_weighted_bipred = bs_read( s, 2 );

    pps->i_pic_init_qp = bs_read_se( s ) + 26;
    pps->i_pic_init_qs = bs_read_se( s ) + 26;

    pps->i_chroma_qp_index_offset = bs_read_se( s );

    pps->b_deblocking_filter_control = bs_read( s, 1 );
    pps->b_constrained_intra_pred = bs_read( s, 1 );
    pps->b_redundant_pic_cnt = bs_read( s, 1 );

    if( bs_eof( s ) )
    {
        /* no rbsp trailing */
        fprintf( stderr, "incomplete PPS\n" );
        goto error;
    }
    fprintf( stderr, "x264_sps_read: pps:0x%x sps:0x%x %s slice_groups=%d ref0:%d ref1:%d QP:%d QS:%d QC=%d DFC:%d CIP:%d RPC:%d\n",
             pps->i_id, pps->i_sps_id,
             pps->b_cabac ? "CABAC" : "CAVLC",
             pps->i_num_slice_groups,
             pps->i_num_ref_idx_l0_active,
             pps->i_num_ref_idx_l1_active,
             pps->i_pic_init_qp, pps->i_pic_init_qs, pps->i_chroma_qp_index_offset,
             pps->b_deblocking_filter_control,
             pps->b_constrained_intra_pred,
             pps->b_redundant_pic_cnt );

    return id;
error:
    pps->i_id = -1;
    return -1;
}


/*****************************************************************************
 * set: h264 encoder (SPS and PPS init and write)
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar
 * $Id: set.c,v 1.1 2004/06/03 19:27:08 fenrir Exp $
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

#ifdef HAVE_STDINT_H
#include <stdint.h>
#else
#include <inttypes.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "../x264.h"
#include "../common/bs.h"
#include "../common/set.h"

void x264_sps_init( x264_sps_t *sps, int i_id, x264_param_t *param )
{
    sps->i_id               = i_id;

    if( param->b_cabac || param->i_bframe > 0 )
        sps->i_profile_idc      = PROFILE_MAIN;
    else
        sps->i_profile_idc      = PROFILE_BASELINE;

    sps->i_level_idc        = 21;               /* FIXME ? */
    sps->b_constraint_set0  = 0;
    sps->b_constraint_set1  = 0;
    sps->b_constraint_set2  = 0;

    sps->i_log2_max_frame_num = 4;  /* at least 4 */
    while( (1 << sps->i_log2_max_frame_num) <= param->i_idrframe * param->i_iframe )
    {
        sps->i_log2_max_frame_num++;
    }
    sps->i_log2_max_frame_num++;    /* just in case */

    sps->i_poc_type = 0;
    if( sps->i_poc_type == 0 )
    {
        sps->i_log2_max_poc_lsb = sps->i_log2_max_frame_num + 1;    /* max poc = 2*frame_num */
    }
    else if( sps->i_poc_type == 1 )
    {
        int i;

        /* FIXME */
        sps->b_delta_pic_order_always_zero = 1;
        sps->i_offset_for_non_ref_pic = 0;
        sps->i_offset_for_top_to_bottom_field = 0;
        sps->i_num_ref_frames_in_poc_cycle = 0;

        for( i = 0; i < sps->i_num_ref_frames_in_poc_cycle; i++ )
        {
            sps->i_offset_for_ref_frame[i] = 0;
        }
    }

    sps->i_num_ref_frames = param->i_frame_reference;
    if( param->i_bframe )
        sps->i_num_ref_frames++; /* for 2 ref in B */
    sps->b_gaps_in_frame_num_value_allowed = 0;
    sps->i_mb_width = ( param->i_width + 15 ) / 16;
    sps->i_mb_height= ( param->i_height + 15 )/ 16;
    sps->b_frame_mbs_only = 1;
    sps->b_mb_adaptive_frame_field = 0;
    sps->b_direct8x8_inference = 0;
    if( sps->b_frame_mbs_only == 0 ||
        !(param->analyse.inter & X264_ANALYSE_PSUB8x8) )
    {
        sps->b_direct8x8_inference = 1;
    }

    if( param->i_width % 16 != 0 || param->i_height % 16 != 0 )
    {
        sps->b_crop = 1;
        sps->crop.i_left    = 0;
        sps->crop.i_right   = ( 16 - param->i_width % 16)/2;
        sps->crop.i_top     = 0;
        sps->crop.i_bottom  = ( 16 - param->i_height % 16)/2;
    }
    else
    {
        sps->b_crop = 0;
        sps->crop.i_left    = 0;
        sps->crop.i_right   = 0;
        sps->crop.i_top     = 0;
        sps->crop.i_bottom  = 0;
    }

    sps->b_vui = 0;
    sps->vui.b_aspect_ratio_info_present = 0;

    if( param->vui.i_sar_width > 0 && param->vui.i_sar_height > 0 )
    {
        sps->vui.b_aspect_ratio_info_present = 1;
        sps->vui.i_sar_width = param->vui.i_sar_width;
        sps->vui.i_sar_height= param->vui.i_sar_height;
    }
    sps->b_vui |= sps->vui.b_aspect_ratio_info_present;

    if( param->i_fps_num > 0 && param->i_fps_den > 0)
    {
        sps->vui.b_timing_info_present = 1;
        /* The standard is confusing me, but this seems to work best
           with other encoders */
        sps->vui.i_num_units_in_tick = param->i_fps_den;
        sps->vui.i_time_scale = param->i_fps_num;
        sps->vui.b_fixed_frame_rate = 1;
    }
    sps->b_vui |= sps->vui.b_timing_info_present;
}


void x264_sps_write( bs_t *s, x264_sps_t *sps )
{
    bs_write( s, 8, sps->i_profile_idc );
    bs_write( s, 1, sps->b_constraint_set0 );
    bs_write( s, 1, sps->b_constraint_set1 );
    bs_write( s, 1, sps->b_constraint_set2 );

    bs_write( s, 5, 0 );    /* reserved */

    bs_write( s, 8, sps->i_level_idc );

    bs_write_ue( s, sps->i_id );
    bs_write_ue( s, sps->i_log2_max_frame_num - 4 );
    bs_write_ue( s, sps->i_poc_type );
    if( sps->i_poc_type == 0 )
    {
        bs_write_ue( s, sps->i_log2_max_poc_lsb - 4 );
    }
    else if( sps->i_poc_type == 1 )
    {
        int i;

        bs_write( s, 1, sps->b_delta_pic_order_always_zero );
        bs_write_se( s, sps->i_offset_for_non_ref_pic );
        bs_write_se( s, sps->i_offset_for_top_to_bottom_field );
        bs_write_ue( s, sps->i_num_ref_frames_in_poc_cycle );

        for( i = 0; i < sps->i_num_ref_frames_in_poc_cycle; i++ )
        {
            bs_write_se( s, sps->i_offset_for_ref_frame[i] );
        }
    }
    bs_write_ue( s, sps->i_num_ref_frames );
    bs_write( s, 1, sps->b_gaps_in_frame_num_value_allowed );
    bs_write_ue( s, sps->i_mb_width - 1 );
    bs_write_ue( s, sps->i_mb_height - 1);
    bs_write( s, 1, sps->b_frame_mbs_only );
    if( !sps->b_frame_mbs_only )
    {
        bs_write( s, 1, sps->b_mb_adaptive_frame_field );
    }
    bs_write( s, 1, sps->b_direct8x8_inference );

    bs_write( s, 1, sps->b_crop );
    if( sps->b_crop )
    {
        bs_write_ue( s, sps->crop.i_left );
        bs_write_ue( s, sps->crop.i_right );
        bs_write_ue( s, sps->crop.i_top );
        bs_write_ue( s, sps->crop.i_bottom );
    }

    bs_write( s, 1, sps->b_vui );
    if( sps->b_vui )
    {
        bs_write1( s, sps->vui.b_aspect_ratio_info_present );
        if( sps->vui.b_aspect_ratio_info_present )
        {
            int i;
            static const struct { int w, h; int sar; } sar[] =
            {
                { 1,   1, 1 }, { 12, 11, 2 }, { 10, 11, 3 }, { 16, 11, 4 },
                { 40, 33, 5 }, { 24, 11, 6 }, { 20, 11, 7 }, { 32, 11, 8 },
                { 80, 33, 9 }, { 18, 11, 10}, { 15, 11, 11}, { 64, 33, 12},
                { 160,99, 13}, { 0, 0, -1 }
            };
            for( i = 0; sar[i].sar != -1; i++ )
            {
                if( sar[i].w == sps->vui.i_sar_width &&
                    sar[i].h == sps->vui.i_sar_height )
                    break;
            }
            if( sar[i].sar != -1 )
            {
                bs_write( s, 8, sar[i].sar );
            }
            else
            {
                bs_write( s, 8, 255);   /* aspect_ration_idc (extented) */
                bs_write( s, 16, sps->vui.i_sar_width );
                bs_write( s, 16, sps->vui.i_sar_height );
            }
        }

        bs_write1( s, 0 );      /* overscan_info_present_flag */

        bs_write1( s, 0 );      /* video_signal_type_present_flag */
#if 0
        bs_write( s, 3, 5 );    /* unspecified video format */
        bs_write1( s, 1 );      /* video full range flag */
        bs_write1( s, 0 );      /* colour description present flag */
#endif
        bs_write1( s, 0 );      /* chroma_loc_info_present_flag */

        bs_write1( s, sps->vui.b_timing_info_present );
        if( sps->vui.b_timing_info_present )
        {
            bs_write( s, 32, sps->vui.i_num_units_in_tick );
            bs_write( s, 32, sps->vui.i_time_scale );
            bs_write1( s, sps->vui.b_fixed_frame_rate );
        }

        bs_write1( s, 0 );      /* nal_hrd_parameters_present_flag */
        bs_write1( s, 0 );      /* vcl_hrd_parameters_present_flag */
        bs_write1( s, 0 );      /* pic_struct_present_flag */
        bs_write1( s, 0 );      /* bitstream_restriction_flag */
    }

    bs_rbsp_trailing( s );
}

void x264_pps_init( x264_pps_t *pps, int i_id, x264_param_t *param, x264_sps_t *sps )
{
    pps->i_id = i_id;
    pps->i_sps_id = sps->i_id;
    pps->b_cabac = param->b_cabac;

    pps->b_pic_order = 0;
    pps->i_num_slice_groups = 1;

    if( pps->i_num_slice_groups > 1 )
    {
        int i;

        pps->i_slice_group_map_type = 0;
        if( pps->i_slice_group_map_type == 0 )
        {
            for( i = 0; i < pps->i_num_slice_groups; i++ )
            {
                pps->i_run_length[i] = 1;
            }
        }
        else if( pps->i_slice_group_map_type == 2 )
        {
            for( i = 0; i < pps->i_num_slice_groups; i++ )
            {
                pps->i_top_left[i] = 0;
                pps->i_bottom_right[i] = 0;
            }
        }
        else if( pps->i_slice_group_map_type >= 3 &&
                 pps->i_slice_group_map_type <= 5 )
        {
            pps->b_slice_group_change_direction = 0;
            pps->i_slice_group_change_rate = 0;
        }
        else if( pps->i_slice_group_map_type == 6 )
        {
            pps->i_pic_size_in_map_units = 1;
            for( i = 0; i < pps->i_pic_size_in_map_units; i++ )
            {
                pps->i_slice_group_id[i] = 0;
            }
        }
    }
    pps->i_num_ref_idx_l0_active = 1;
    pps->i_num_ref_idx_l1_active = 1;

    pps->b_weighted_pred = 0;
    pps->b_weighted_bipred = 0;

    pps->i_pic_init_qp = 26;
    pps->i_pic_init_qs = 26;

    pps->i_chroma_qp_index_offset = 0;
#if 0
    if( !param->b_deblocking_filter )
    {
        pps->b_deblocking_filter_control = 1;
    }
    else
    {
        pps->b_deblocking_filter_control = 1;
    }
#endif
    pps->b_deblocking_filter_control = 1;
    pps->b_constrained_intra_pred = 0;
    pps->b_redundant_pic_cnt = 0;
}

void x264_pps_write( bs_t *s, x264_pps_t *pps )
{
    bs_write_ue( s, pps->i_id );
    bs_write_ue( s, pps->i_sps_id );

    bs_write( s, 1, pps->b_cabac );
    bs_write( s, 1, pps->b_pic_order );
    bs_write_ue( s, pps->i_num_slice_groups - 1 );

    if( pps->i_num_slice_groups > 1 )
    {
        int i;

        bs_write_ue( s, pps->i_slice_group_map_type );
        if( pps->i_slice_group_map_type == 0 )
        {
            for( i = 0; i < pps->i_num_slice_groups; i++ )
            {
                bs_write_ue( s, pps->i_run_length[i] - 1 );
            }
        }
        else if( pps->i_slice_group_map_type == 2 )
        {
            for( i = 0; i < pps->i_num_slice_groups; i++ )
            {
                bs_write_ue( s, pps->i_top_left[i] );
                bs_write_ue( s, pps->i_bottom_right[i] );
            }
        }
        else if( pps->i_slice_group_map_type >= 3 &&
                 pps->i_slice_group_map_type <= 5 )
        {
            bs_write( s, 1, pps->b_slice_group_change_direction );
            bs_write_ue( s, pps->b_slice_group_change_direction - 1 );
        }
        else if( pps->i_slice_group_map_type == 6 )
        {
            bs_write_ue( s, pps->i_pic_size_in_map_units - 1 );
            for( i = 0; i < pps->i_pic_size_in_map_units; i++ )
            {
                /* FIXME */
                /* bs_write( s, ceil( log2( pps->i_pic_size_in_map_units +1 ) ),
                 *              pps->i_slice_group_id[i] );
                 */
            }
        }
    }

    bs_write_ue( s, pps->i_num_ref_idx_l0_active - 1 );
    bs_write_ue( s, pps->i_num_ref_idx_l1_active - 1 );
    bs_write( s, 1, pps->b_weighted_pred );
    bs_write( s, 2, pps->b_weighted_bipred );

    bs_write_se( s, pps->i_pic_init_qp - 26 );
    bs_write_se( s, pps->i_pic_init_qs - 26 );
    bs_write_se( s, pps->i_chroma_qp_index_offset );

    bs_write( s, 1, pps->b_deblocking_filter_control );
    bs_write( s, 1, pps->b_constrained_intra_pred );
    bs_write( s, 1, pps->b_redundant_pic_cnt );

    bs_rbsp_trailing( s );
}


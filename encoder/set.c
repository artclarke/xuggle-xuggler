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
#include <math.h>

#include "x264.h"
#include "common/common.h"
#include "common/macroblock.h"
#ifndef _MSC_VER
#include "config.h"
#endif

static const uint8_t *const x264_cqm_jvt[6] =
{
    x264_cqm_jvt4i, x264_cqm_jvt4p,
    x264_cqm_jvt4i, x264_cqm_jvt4p,
    x264_cqm_jvt8i, x264_cqm_jvt8p
};

static void scaling_list_write( bs_t *s, x264_pps_t *pps, int idx )
{
    const int len = idx<4 ? 16 : 64;
    const int *zigzag = idx<4 ? x264_zigzag_scan4 : x264_zigzag_scan8;
    const uint8_t *list = pps->scaling_list[idx];
    const uint8_t *def_list = (idx==CQM_4IC) ? pps->scaling_list[CQM_4IY]
                            : (idx==CQM_4PC) ? pps->scaling_list[CQM_4PY]
                            : x264_cqm_jvt[idx];
    int j;
    if( memcmp( list, def_list, len ) )
    {
        bs_write( s, 1, 1 ); // scaling_list_present_flag
        for( j = 0; j < len; j++ )
            bs_write_se( s, list[zigzag[j]] - (j>0 ? list[zigzag[j-1]] : 8) ); // delta
    }
    else
        bs_write( s, 1, 0 ); // scaling_list_present_flag
}

void x264_sps_init( x264_sps_t *sps, int i_id, x264_param_t *param )
{
    sps->i_id = i_id;

    sps->b_qpprime_y_zero_transform_bypass = !param->rc.b_cbr && param->rc.i_qp_constant == 0;
    if( sps->b_qpprime_y_zero_transform_bypass )
        sps->i_profile_idc  = PROFILE_HIGH444;
    else if( param->analyse.b_transform_8x8 || param->i_cqm_preset != X264_CQM_FLAT )
        sps->i_profile_idc  = PROFILE_HIGH;
    else if( param->b_cabac || param->i_bframe > 0 )
        sps->i_profile_idc  = PROFILE_MAIN;
    else
        sps->i_profile_idc  = PROFILE_BASELINE;
    sps->i_level_idc = param->i_level_idc;

    sps->b_constraint_set0  = 0;
    sps->b_constraint_set1  = 0;
    sps->b_constraint_set2  = 0;

    sps->i_log2_max_frame_num = 4;  /* at least 4 */
    while( (1 << sps->i_log2_max_frame_num) <= param->i_keyint_max )
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

    sps->vui.i_num_reorder_frames = param->b_bframe_pyramid ? 2 : param->i_bframe ? 1 : 0;
    sps->vui.i_max_dec_frame_buffering =
    sps->i_num_ref_frames = X264_MIN(16, param->i_frame_reference + sps->vui.i_num_reorder_frames);

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

    sps->vui.b_bitstream_restriction = param->i_bframe > 0;
    if( sps->vui.b_bitstream_restriction )
    {
        sps->vui.b_motion_vectors_over_pic_boundaries = 1;
        sps->vui.i_max_bytes_per_pic_denom = 0;
        sps->vui.i_max_bits_per_mb_denom = 0;
        sps->vui.i_log2_max_mv_length_horizontal =
        sps->vui.i_log2_max_mv_length_vertical = (int)(log(param->analyse.i_mv_range*4-1)/log(2)) + 1;
    }
    sps->b_vui |= sps->vui.b_bitstream_restriction;
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

    if( sps->i_profile_idc >= PROFILE_HIGH )
    {
        bs_write_ue( s, 1 ); // chroma_format_idc = 4:2:0
        bs_write_ue( s, 0 ); // bit_depth_luma_minus8
        bs_write_ue( s, 0 ); // bit_depth_chroma_minus8
        bs_write( s, 1, sps->b_qpprime_y_zero_transform_bypass );
        bs_write( s, 1, 0 ); // seq_scaling_matrix_present_flag
    }

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
                bs_write( s, 8, 255);   /* aspect_ratio_idc (extented) */
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
        bs_write1( s, sps->vui.b_bitstream_restriction );
        if( sps->vui.b_bitstream_restriction )
        {
            bs_write1( s, sps->vui.b_motion_vectors_over_pic_boundaries );
            bs_write_ue( s, sps->vui.i_max_bytes_per_pic_denom );
            bs_write_ue( s, sps->vui.i_max_bits_per_mb_denom );
            bs_write_ue( s, sps->vui.i_log2_max_mv_length_horizontal );
            bs_write_ue( s, sps->vui.i_log2_max_mv_length_vertical );
            bs_write_ue( s, sps->vui.i_num_reorder_frames );
            bs_write_ue( s, sps->vui.i_max_dec_frame_buffering );
        }
    }

    bs_rbsp_trailing( s );
}

void x264_pps_init( x264_pps_t *pps, int i_id, x264_param_t *param, x264_sps_t *sps )
{
    int i, j;

    pps->i_id = i_id;
    pps->i_sps_id = sps->i_id;
    pps->b_cabac = param->b_cabac;

    pps->b_pic_order = 0;
    pps->i_num_slice_groups = 1;

#if 0
    if( pps->i_num_slice_groups > 1 )
    {
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
#endif
    pps->i_num_ref_idx_l0_active = 1;
    pps->i_num_ref_idx_l1_active = 1;

    pps->b_weighted_pred = 0;
    pps->b_weighted_bipred = param->analyse.b_weighted_bipred ? 2 : 0;

    pps->i_pic_init_qp = 26;
    pps->i_pic_init_qs = 26;

    pps->i_chroma_qp_index_offset = param->analyse.i_chroma_qp_offset;
    pps->b_deblocking_filter_control = 1;
    pps->b_constrained_intra_pred = 0;
    pps->b_redundant_pic_cnt = 0;

    pps->b_transform_8x8_mode = param->analyse.b_transform_8x8 ? 1 : 0;

    pps->i_cqm_preset = param->i_cqm_preset;
    switch( pps->i_cqm_preset )
    {
    case X264_CQM_FLAT:
        for( i = 0; i < 6; i++ )
            pps->scaling_list[i] = x264_cqm_flat16;
        break;
    case X264_CQM_JVT:
        for( i = 0; i < 6; i++ )
            pps->scaling_list[i] = x264_cqm_jvt[i];
        break;
    case X264_CQM_CUSTOM:
        pps->scaling_list[CQM_4IY] = param->cqm_4iy;
        pps->scaling_list[CQM_4IC] = param->cqm_4ic;
        pps->scaling_list[CQM_4PY] = param->cqm_4py;
        pps->scaling_list[CQM_4PC] = param->cqm_4pc;
        pps->scaling_list[CQM_8IY+4] = param->cqm_8iy;
        pps->scaling_list[CQM_8PY+4] = param->cqm_8py;
        for( i = 0; i < 6; i++ )
            for( j = 0; j < (i<4?16:64); j++ )
                if( pps->scaling_list[i][j] == 0 )
                    pps->scaling_list[i] = x264_cqm_jvt[i];
        break;
    }
}

void x264_pps_write( bs_t *s, x264_pps_t *pps )
{
    bs_write_ue( s, pps->i_id );
    bs_write_ue( s, pps->i_sps_id );

    bs_write( s, 1, pps->b_cabac );
    bs_write( s, 1, pps->b_pic_order );
    bs_write_ue( s, pps->i_num_slice_groups - 1 );

#if 0
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
#endif

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

    if( pps->b_transform_8x8_mode || pps->i_cqm_preset != X264_CQM_FLAT )
    {
        bs_write( s, 1, pps->b_transform_8x8_mode );
        bs_write( s, 1, (pps->i_cqm_preset != X264_CQM_FLAT) );
        if( pps->i_cqm_preset != X264_CQM_FLAT )
        {
            scaling_list_write( s, pps, CQM_4IY );
            scaling_list_write( s, pps, CQM_4IC );
            bs_write( s, 1, 0 ); // Cr = Cb
            scaling_list_write( s, pps, CQM_4PY );
            scaling_list_write( s, pps, CQM_4PC );
            bs_write( s, 1, 0 ); // Cr = Cb
            if( pps->b_transform_8x8_mode )
            {
                scaling_list_write( s, pps, CQM_8IY+4 );
                scaling_list_write( s, pps, CQM_8PY+4 );
            }
        }
        bs_write_se( s, pps->i_chroma_qp_index_offset );
    }

    bs_rbsp_trailing( s );
}

void x264_sei_version_write( bs_t *s )
{
    int i;
    // random ID number generated according to ISO-11578
    const uint8_t uuid[16] = {
        0xdc, 0x45, 0xe9, 0xbd, 0xe6, 0xd9, 0x48, 0xb7,
        0x96, 0x2c, 0xd8, 0x20, 0xd9, 0x23, 0xee, 0xef
    };
    char version[256];
    int length;
    sprintf( version, "x264 - core %d%s - H.264/MPEG-4 AVC codec - Copyleft 2005 - http://www.videolan.org/x264.html",
             X264_BUILD, X264_VERSION );
    length = strlen(version)+1+16;

    bs_write( s, 8, 0x5 ); // payload_type = user_data_unregistered
    while( length > 255 )
        bs_write( s, 8, 255 ), length -= 255;
    bs_write( s, 8, length ); // payload_size

    for( i = 0; i < 16; i++ )
        bs_write( s, 8, uuid[i] );
    for( i = 0; i < length-16; i++ )
        bs_write( s, 8, version[i] );

    bs_rbsp_trailing( s );
}

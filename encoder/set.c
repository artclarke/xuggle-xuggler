/*****************************************************************************
 * set: h264 encoder (SPS and PPS init and write)
 *****************************************************************************
 * Copyright (C) 2003-2008 x264 project
 *
 * Authors: Laurent Aimar <fenrir@via.ecp.fr>
 *          Loren Merritt <lorenm@u.washington.edu>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111, USA.
 *****************************************************************************/

#include <math.h>

#include "common/common.h"
#ifndef _MSC_VER
#include "config.h"
#endif
#include "set.h"

#define bs_write_ue bs_write_ue_big

static void transpose( uint8_t *buf, int w )
{
    int i, j;
    for( i = 0; i < w; i++ )
        for( j = 0; j < i; j++ )
            XCHG( uint8_t, buf[w*i+j], buf[w*j+i] );
}

static void scaling_list_write( bs_t *s, x264_pps_t *pps, int idx )
{
    const int len = idx<4 ? 16 : 64;
    const uint8_t *zigzag = idx<4 ? x264_zigzag_scan4[0] : x264_zigzag_scan8[0];
    const uint8_t *list = pps->scaling_list[idx];
    const uint8_t *def_list = (idx==CQM_4IC) ? pps->scaling_list[CQM_4IY]
                            : (idx==CQM_4PC) ? pps->scaling_list[CQM_4PY]
                            : x264_cqm_jvt[idx];
    if( !memcmp( list, def_list, len ) )
        bs_write( s, 1, 0 ); // scaling_list_present_flag
    else if( !memcmp( list, x264_cqm_jvt[idx], len ) )
    {
        bs_write( s, 1, 1 ); // scaling_list_present_flag
        bs_write_se( s, -8 ); // use jvt list
    }
    else
    {
        int j, run;
        bs_write( s, 1, 1 ); // scaling_list_present_flag

        // try run-length compression of trailing values
        for( run = len; run > 1; run-- )
            if( list[zigzag[run-1]] != list[zigzag[run-2]] )
                break;
        if( run < len && len - run < bs_size_se( (int8_t)-list[zigzag[run]] ) )
            run = len;

        for( j = 0; j < run; j++ )
            bs_write_se( s, (int8_t)(list[zigzag[j]] - (j>0 ? list[zigzag[j-1]] : 8)) ); // delta

        if( run < len )
            bs_write_se( s, (int8_t)-list[zigzag[run]] );
    }
}

void x264_sps_init( x264_sps_t *sps, int i_id, x264_param_t *param )
{
    sps->i_id = i_id;

    sps->b_qpprime_y_zero_transform_bypass = param->rc.i_rc_method == X264_RC_CQP && param->rc.i_qp_constant == 0;
    if( sps->b_qpprime_y_zero_transform_bypass )
        sps->i_profile_idc  = PROFILE_HIGH444_PREDICTIVE;
    else if( param->analyse.b_transform_8x8 || param->i_cqm_preset != X264_CQM_FLAT )
        sps->i_profile_idc  = PROFILE_HIGH;
    else if( param->b_cabac || param->i_bframe > 0 )
        sps->i_profile_idc  = PROFILE_MAIN;
    else
        sps->i_profile_idc  = PROFILE_BASELINE;
    sps->i_level_idc = param->i_level_idc;

    sps->b_constraint_set0  = sps->i_profile_idc == PROFILE_BASELINE;
    /* x264 doesn't support the features that are in Baseline and not in Main,
     * namely arbitrary_slice_order and slice_groups. */
    sps->b_constraint_set1  = sps->i_profile_idc <= PROFILE_MAIN;
    /* Never set constraint_set2, it is not necessary and not used in real world. */
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

    sps->b_vui = 1;

    sps->b_gaps_in_frame_num_value_allowed = 0;
    sps->i_mb_width = ( param->i_width + 15 ) / 16;
    sps->i_mb_height= ( param->i_height + 15 ) / 16;
    if( param->b_interlaced )
        sps->i_mb_height = ( sps->i_mb_height + 1 ) & ~1;
    sps->b_frame_mbs_only = ! param->b_interlaced;
    sps->b_mb_adaptive_frame_field = param->b_interlaced;
    sps->b_direct8x8_inference = 1;

    sps->crop.i_left   = 0;
    sps->crop.i_top    = 0;
    sps->crop.i_right  = sps->i_mb_width*16 - param->i_width;
    sps->crop.i_bottom = (sps->i_mb_height*16 - param->i_height) >> param->b_interlaced;
    sps->b_crop = sps->crop.i_left  || sps->crop.i_top ||
                  sps->crop.i_right || sps->crop.i_bottom;

    sps->vui.b_aspect_ratio_info_present = 0;
    if( param->vui.i_sar_width > 0 && param->vui.i_sar_height > 0 )
    {
        sps->vui.b_aspect_ratio_info_present = 1;
        sps->vui.i_sar_width = param->vui.i_sar_width;
        sps->vui.i_sar_height= param->vui.i_sar_height;
    }

    sps->vui.b_overscan_info_present = ( param->vui.i_overscan ? 1 : 0 );
    if( sps->vui.b_overscan_info_present )
        sps->vui.b_overscan_info = ( param->vui.i_overscan == 2 ? 1 : 0 );

    sps->vui.b_signal_type_present = 0;
    sps->vui.i_vidformat = ( param->vui.i_vidformat <= 5 ? param->vui.i_vidformat : 5 );
    sps->vui.b_fullrange = ( param->vui.b_fullrange ? 1 : 0 );
    sps->vui.b_color_description_present = 0;

    sps->vui.i_colorprim = ( param->vui.i_colorprim <=  9 ? param->vui.i_colorprim : 2 );
    sps->vui.i_transfer  = ( param->vui.i_transfer  <= 11 ? param->vui.i_transfer  : 2 );
    sps->vui.i_colmatrix = ( param->vui.i_colmatrix <=  9 ? param->vui.i_colmatrix : 2 );
    if( sps->vui.i_colorprim != 2 ||
        sps->vui.i_transfer  != 2 ||
        sps->vui.i_colmatrix != 2 )
    {
        sps->vui.b_color_description_present = 1;
    }

    if( sps->vui.i_vidformat != 5 ||
        sps->vui.b_fullrange ||
        sps->vui.b_color_description_present )
    {
        sps->vui.b_signal_type_present = 1;
    }

    /* FIXME: not sufficient for interlaced video */
    sps->vui.b_chroma_loc_info_present = ( param->vui.i_chroma_loc ? 1 : 0 );
    if( sps->vui.b_chroma_loc_info_present )
    {
        sps->vui.i_chroma_loc_top = param->vui.i_chroma_loc;
        sps->vui.i_chroma_loc_bottom = param->vui.i_chroma_loc;
    }

    sps->vui.b_timing_info_present = 0;
    if( param->i_fps_num > 0 && param->i_fps_den > 0)
    {
        sps->vui.b_timing_info_present = 1;
        sps->vui.i_num_units_in_tick = param->i_fps_den;
        sps->vui.i_time_scale = param->i_fps_num * 2;
        sps->vui.b_fixed_frame_rate = 1;
    }

    sps->vui.i_num_reorder_frames = param->b_bframe_pyramid ? 2 : param->i_bframe ? 1 : 0;
    /* extra slot with pyramid so that we don't have to override the
     * order of forgetting old pictures */
    sps->vui.i_max_dec_frame_buffering =
    sps->i_num_ref_frames = X264_MIN(16, X264_MAX(param->i_frame_reference, 1 + sps->vui.i_num_reorder_frames));

    sps->vui.b_bitstream_restriction = 1;
    if( sps->vui.b_bitstream_restriction )
    {
        sps->vui.b_motion_vectors_over_pic_boundaries = 1;
        sps->vui.i_max_bytes_per_pic_denom = 0;
        sps->vui.i_max_bits_per_mb_denom = 0;
        sps->vui.i_log2_max_mv_length_horizontal =
        sps->vui.i_log2_max_mv_length_vertical = (int)(log(param->analyse.i_mv_range*4-1)/log(2)) + 1;
    }
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
    if (sps->b_frame_mbs_only)
    {
        bs_write_ue( s, sps->i_mb_height - 1);
    }
    else // interlaced
    {
        bs_write_ue( s, sps->i_mb_height/2 - 1);
    }
    bs_write( s, 1, sps->b_frame_mbs_only );
    if( !sps->b_frame_mbs_only )
    {
        bs_write( s, 1, sps->b_mb_adaptive_frame_field );
    }
    bs_write( s, 1, sps->b_direct8x8_inference );

    bs_write( s, 1, sps->b_crop );
    if( sps->b_crop )
    {
        bs_write_ue( s, sps->crop.i_left   / 2 );
        bs_write_ue( s, sps->crop.i_right  / 2 );
        bs_write_ue( s, sps->crop.i_top    / 2 );
        bs_write_ue( s, sps->crop.i_bottom / 2 );
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
                bs_write( s, 8, 255);   /* aspect_ratio_idc (extended) */
                bs_write( s, 16, sps->vui.i_sar_width );
                bs_write( s, 16, sps->vui.i_sar_height );
            }
        }

        bs_write1( s, sps->vui.b_overscan_info_present );
        if( sps->vui.b_overscan_info_present )
            bs_write1( s, sps->vui.b_overscan_info );

        bs_write1( s, sps->vui.b_signal_type_present );
        if( sps->vui.b_signal_type_present )
        {
            bs_write( s, 3, sps->vui.i_vidformat );
            bs_write1( s, sps->vui.b_fullrange );
            bs_write1( s, sps->vui.b_color_description_present );
            if( sps->vui.b_color_description_present )
            {
                bs_write( s, 8, sps->vui.i_colorprim );
                bs_write( s, 8, sps->vui.i_transfer );
                bs_write( s, 8, sps->vui.i_colmatrix );
            }
        }

        bs_write1( s, sps->vui.b_chroma_loc_info_present );
        if( sps->vui.b_chroma_loc_info_present )
        {
            bs_write_ue( s, sps->vui.i_chroma_loc_top );
            bs_write_ue( s, sps->vui.i_chroma_loc_bottom );
        }

        bs_write1( s, sps->vui.b_timing_info_present );
        if( sps->vui.b_timing_info_present )
        {
            bs_write32( s, sps->vui.i_num_units_in_tick );
            bs_write32( s, sps->vui.i_time_scale );
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

    pps->i_num_ref_idx_l0_active = 1;
    pps->i_num_ref_idx_l1_active = 1;

    pps->b_weighted_pred = 0;
    pps->b_weighted_bipred = param->analyse.b_weighted_bipred ? 2 : 0;

    pps->i_pic_init_qp = param->rc.i_rc_method == X264_RC_ABR ? 26 : param->rc.i_qp_constant;
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
        /* match the transposed DCT & zigzag */
        transpose( param->cqm_4iy, 4 );
        transpose( param->cqm_4ic, 4 );
        transpose( param->cqm_4py, 4 );
        transpose( param->cqm_4pc, 4 );
        transpose( param->cqm_8iy, 8 );
        transpose( param->cqm_8py, 8 );
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

void x264_sei_version_write( x264_t *h, bs_t *s )
{
    int i;
    // random ID number generated according to ISO-11578
    const uint8_t uuid[16] = {
        0xdc, 0x45, 0xe9, 0xbd, 0xe6, 0xd9, 0x48, 0xb7,
        0x96, 0x2c, 0xd8, 0x20, 0xd9, 0x23, 0xee, 0xef
    };
    char *opts = x264_param2string( &h->param, 0 );
    char *version = x264_malloc( 200 + strlen(opts) );
    int length;

    sprintf( version, "x264 - core %d%s - H.264/MPEG-4 AVC codec - "
             "Copyleft 2003-2009 - http://www.videolan.org/x264.html - options: %s",
             X264_BUILD, X264_VERSION, opts );
    length = strlen(version)+1+16;

    bs_write( s, 8, 0x5 ); // payload_type = user_data_unregistered
    // payload_size
    for( i = 0; i <= length-255; i += 255 )
        bs_write( s, 8, 255 );
    bs_write( s, 8, length-i );

    for( i = 0; i < 16; i++ )
        bs_write( s, 8, uuid[i] );
    for( i = 0; i < length-16; i++ )
        bs_write( s, 8, version[i] );

    bs_rbsp_trailing( s );

    x264_free( opts );
    x264_free( version );
}

const x264_level_t x264_levels[] =
{
    { 10,   1485,    99,   152064,     64,    175,  64, 64,  0, 0, 0, 1 },
//  {"1b",  1485,    99,   152064,    128,    350,  64, 64,  0, 0, 0, 1 },
    { 11,   3000,   396,   345600,    192,    500, 128, 64,  0, 0, 0, 1 },
    { 12,   6000,   396,   912384,    384,   1000, 128, 64,  0, 0, 0, 1 },
    { 13,  11880,   396,   912384,    768,   2000, 128, 64,  0, 0, 0, 1 },
    { 20,  11880,   396,   912384,   2000,   2000, 128, 64,  0, 0, 0, 1 },
    { 21,  19800,   792,  1824768,   4000,   4000, 256, 64,  0, 0, 0, 0 },
    { 22,  20250,  1620,  3110400,   4000,   4000, 256, 64,  0, 0, 0, 0 },
    { 30,  40500,  1620,  3110400,  10000,  10000, 256, 32, 22, 0, 1, 0 },
    { 31, 108000,  3600,  6912000,  14000,  14000, 512, 16, 60, 1, 1, 0 },
    { 32, 216000,  5120,  7864320,  20000,  20000, 512, 16, 60, 1, 1, 0 },
    { 40, 245760,  8192, 12582912,  20000,  25000, 512, 16, 60, 1, 1, 0 },
    { 41, 245760,  8192, 12582912,  50000,  62500, 512, 16, 24, 1, 1, 0 },
    { 42, 522240,  8704, 13369344,  50000,  62500, 512, 16, 24, 1, 1, 1 },
    { 50, 589824, 22080, 42393600, 135000, 135000, 512, 16, 24, 1, 1, 1 },
    { 51, 983040, 36864, 70778880, 240000, 240000, 512, 16, 24, 1, 1, 1 },
    { 0 }
};

#define ERROR(...)\
{\
    if( verbose )\
        x264_log( h, X264_LOG_WARNING, __VA_ARGS__ );\
    ret = 1;\
}

int x264_validate_levels( x264_t *h, int verbose )
{
    int ret = 0;
    int mbs = h->sps->i_mb_width * h->sps->i_mb_height;
    int dpb = mbs * 384 * h->sps->i_num_ref_frames;
    int cbp_factor = h->sps->i_profile_idc==PROFILE_HIGH ? 5 : 4;

    const x264_level_t *l = x264_levels;
    while( l->level_idc != 0 && l->level_idc != h->param.i_level_idc )
        l++;

    if( l->frame_size < mbs
        || l->frame_size*8 < h->sps->i_mb_width * h->sps->i_mb_width
        || l->frame_size*8 < h->sps->i_mb_height * h->sps->i_mb_height )
        ERROR( "frame MB size (%dx%d) > level limit (%d)\n",
               h->sps->i_mb_width, h->sps->i_mb_height, l->frame_size );
    if( dpb > l->dpb )
        ERROR( "DPB size (%d frames, %d bytes) > level limit (%d frames, %d bytes)\n",
                h->sps->i_num_ref_frames, dpb, (int)(l->dpb / (384*mbs)), l->dpb );

#define CHECK( name, limit, val ) \
    if( (val) > (limit) ) \
        ERROR( name " (%d) > level limit (%d)\n", (int)(val), (limit) );

    CHECK( "VBV bitrate", (l->bitrate * cbp_factor) / 4, h->param.rc.i_vbv_max_bitrate );
    CHECK( "VBV buffer", (l->cpb * cbp_factor) / 4, h->param.rc.i_vbv_buffer_size );
    CHECK( "MV range", l->mv_range, h->param.analyse.i_mv_range );
    CHECK( "interlaced", !l->frame_only, h->param.b_interlaced );

    if( h->param.i_fps_den > 0 )
        CHECK( "MB rate", l->mbps, (int64_t)mbs * h->param.i_fps_num / h->param.i_fps_den );

    /* TODO check the rest of the limits */
    return ret;
}

/*****************************************************************************
 * set.h: h264 encoder
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar
 * $Id: set.h,v 1.1 2004/06/03 19:27:07 fenrir Exp $
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

#ifndef _SET_H
#define _SET_H 1

enum profile_e
{
    PROFILE_BASELINE = 66,
    PROFILE_MAIN = 77,
    PROFILE_EXTENTED = 88
};

typedef struct
{
    int i_id;

    int i_profile_idc;
    int i_level_idc;

    int b_constraint_set0;
    int b_constraint_set1;
    int b_constraint_set2;

    int i_log2_max_frame_num;

    int i_poc_type;
    /* poc 0 */
    int i_log2_max_poc_lsb;
    /* poc 1 */
    int b_delta_pic_order_always_zero;
    int i_offset_for_non_ref_pic;
    int i_offset_for_top_to_bottom_field;
    int i_num_ref_frames_in_poc_cycle;
    int i_offset_for_ref_frame[256];

    int i_num_ref_frames;
    int b_gaps_in_frame_num_value_allowed;
    int i_mb_width;
    int i_mb_height;
    int b_frame_mbs_only;
    int b_mb_adaptive_frame_field;
    int b_direct8x8_inference;

    int b_crop;
    struct
    {
        int i_left;
        int i_right;
        int i_top;
        int i_bottom;
    } crop;

    int b_vui;
    struct
    {
        int i_sar_width;
        int i_sar_height;
        /* FIXME to complete */
    } vui;

} x264_sps_t;

typedef struct
{
    int i_id;
    int i_sps_id;

    int b_cabac;

    int b_pic_order;
    int i_num_slice_groups;

    int i_slice_group_map_type;
    /* i_slice_group_map_type == 0 */
    int i_run_length[256];      /* FIXME */
    /* i_slice_group_map_type == 2 */
    int i_top_left[256];        /* FIXME */
    int i_bottom_right[256];    /* FIXME */
    /* i_slice_group_map_type == 3, 4, 5 */
    int b_slice_group_change_direction;
    int i_slice_group_change_rate;
    /* i_slice_group_map_type == 6 */
    int i_pic_size_in_map_units;
    int i_slice_group_id[256];  /* FIXME */

    int i_num_ref_idx_l0_active;
    int i_num_ref_idx_l1_active;

    int b_weighted_pred;
    int b_weighted_bipred;

    int i_pic_init_qp;
    int i_pic_init_qs;

    int i_chroma_qp_index_offset;

    int b_deblocking_filter_control;
    int b_constrained_intra_pred;
    int b_redundant_pic_cnt;
} x264_pps_t;

#endif

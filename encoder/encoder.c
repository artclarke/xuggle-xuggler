/*****************************************************************************
 * x264: h264 encoder
 *****************************************************************************
 * Copyright (C) 2003-2008 x264 project
 *
 * Authors: Laurent Aimar <fenrir@via.ecp.fr>
 *          Loren Merritt <lorenm@u.washington.edu>
 *          Jason Garrett-Glaser <darkshikari@gmail.com>
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
#include "common/cpu.h"

#include "set.h"
#include "analyse.h"
#include "ratecontrol.h"
#include "macroblock.h"

#if VISUALIZE
#include "common/visualize.h"
#endif

//#define DEBUG_MB_TYPE

#define NALU_OVERHEAD 5 // startcode + NAL type costs 5 bytes per frame

#define bs_write_ue bs_write_ue_big

static int x264_encoder_frame_end( x264_t *h, x264_t *thread_current,
                                   x264_nal_t **pp_nal, int *pi_nal,
                                   x264_picture_t *pic_out );

/****************************************************************************
 *
 ******************************* x264 libs **********************************
 *
 ****************************************************************************/
static float x264_psnr( int64_t i_sqe, int64_t i_size )
{
    double f_mse = (double)i_sqe / ((double)65025.0 * (double)i_size);
    if( f_mse <= 0.0000000001 ) /* Max 100dB */
        return 100;

    return (float)(-10.0 * log( f_mse ) / log( 10.0 ));
}

static void x264_frame_dump( x264_t *h )
{
    FILE *f = fopen( h->param.psz_dump_yuv, "r+b" );
    int i, y;
    if( !f )
        return;
    /* Write the frame in display order */
    fseek( f, (uint64_t)h->fdec->i_frame * h->param.i_height * h->param.i_width * 3/2, SEEK_SET );
    for( i = 0; i < h->fdec->i_plane; i++ )
        for( y = 0; y < h->param.i_height >> !!i; y++ )
            fwrite( &h->fdec->plane[i][y*h->fdec->i_stride[i]], 1, h->param.i_width >> !!i, f );
    fclose( f );
}


/* Fill "default" values */
static void x264_slice_header_init( x264_t *h, x264_slice_header_t *sh,
                                    x264_sps_t *sps, x264_pps_t *pps,
                                    int i_idr_pic_id, int i_frame, int i_qp )
{
    x264_param_t *param = &h->param;
    int i;

    /* First we fill all field */
    sh->sps = sps;
    sh->pps = pps;

    sh->i_first_mb  = 0;
    sh->i_last_mb   = h->mb.i_mb_count - 1;
    sh->i_pps_id    = pps->i_id;

    sh->i_frame_num = i_frame;

    sh->b_mbaff = h->param.b_interlaced;
    sh->b_field_pic = 0;    /* no field support for now */
    sh->b_bottom_field = 0; /* not yet used */

    sh->i_idr_pic_id = i_idr_pic_id;

    /* poc stuff, fixed later */
    sh->i_poc_lsb = 0;
    sh->i_delta_poc_bottom = 0;
    sh->i_delta_poc[0] = 0;
    sh->i_delta_poc[1] = 0;

    sh->i_redundant_pic_cnt = 0;

    if( !h->mb.b_direct_auto_read )
    {
        if( h->mb.b_direct_auto_write )
            sh->b_direct_spatial_mv_pred = ( h->stat.i_direct_score[1] > h->stat.i_direct_score[0] );
        else
            sh->b_direct_spatial_mv_pred = ( param->analyse.i_direct_mv_pred == X264_DIRECT_PRED_SPATIAL );
    }
    /* else b_direct_spatial_mv_pred was read from the 2pass statsfile */

    sh->b_num_ref_idx_override = 0;
    sh->i_num_ref_idx_l0_active = 1;
    sh->i_num_ref_idx_l1_active = 1;

    sh->b_ref_pic_list_reordering_l0 = h->b_ref_reorder[0];
    sh->b_ref_pic_list_reordering_l1 = h->b_ref_reorder[1];

    /* If the ref list isn't in the default order, construct reordering header */
    /* List1 reordering isn't needed yet */
    if( sh->b_ref_pic_list_reordering_l0 )
    {
        int pred_frame_num = i_frame;
        for( i = 0; i < h->i_ref0; i++ )
        {
            int diff = h->fref0[i]->i_frame_num - pred_frame_num;
            if( diff == 0 )
                x264_log( h, X264_LOG_ERROR, "diff frame num == 0\n" );
            sh->ref_pic_list_order[0][i].idc = ( diff > 0 );
            sh->ref_pic_list_order[0][i].arg = abs( diff ) - 1;
            pred_frame_num = h->fref0[i]->i_frame_num;
        }
    }

    sh->i_cabac_init_idc = param->i_cabac_init_idc;

    sh->i_qp = i_qp;
    sh->i_qp_delta = i_qp - pps->i_pic_init_qp;
    sh->b_sp_for_swidth = 0;
    sh->i_qs_delta = 0;

    /* If effective qp <= 15, deblocking would have no effect anyway */
    if( param->b_deblocking_filter
        && ( h->mb.b_variable_qp
        || 15 < i_qp + 2 * X264_MIN(param->i_deblocking_filter_alphac0, param->i_deblocking_filter_beta) ) )
    {
        sh->i_disable_deblocking_filter_idc = 0;
    }
    else
    {
        sh->i_disable_deblocking_filter_idc = 1;
    }
    sh->i_alpha_c0_offset = param->i_deblocking_filter_alphac0 << 1;
    sh->i_beta_offset = param->i_deblocking_filter_beta << 1;
}

static void x264_slice_header_write( bs_t *s, x264_slice_header_t *sh, int i_nal_ref_idc )
{
    int i;

    if( sh->b_mbaff )
    {
        assert( sh->i_first_mb % (2*sh->sps->i_mb_width) == 0 );
        bs_write_ue( s, sh->i_first_mb >> 1 );
    }
    else
        bs_write_ue( s, sh->i_first_mb );

    bs_write_ue( s, sh->i_type + 5 );   /* same type things */
    bs_write_ue( s, sh->i_pps_id );
    bs_write( s, sh->sps->i_log2_max_frame_num, sh->i_frame_num & ((1<<sh->sps->i_log2_max_frame_num)-1) );

    if( !sh->sps->b_frame_mbs_only )
    {
        bs_write1( s, sh->b_field_pic );
        if( sh->b_field_pic )
            bs_write1( s, sh->b_bottom_field );
    }

    if( sh->i_idr_pic_id >= 0 ) /* NAL IDR */
    {
        bs_write_ue( s, sh->i_idr_pic_id );
    }

    if( sh->sps->i_poc_type == 0 )
    {
        bs_write( s, sh->sps->i_log2_max_poc_lsb, sh->i_poc_lsb & ((1<<sh->sps->i_log2_max_poc_lsb)-1) );
        if( sh->pps->b_pic_order && !sh->b_field_pic )
        {
            bs_write_se( s, sh->i_delta_poc_bottom );
        }
    }
    else if( sh->sps->i_poc_type == 1 && !sh->sps->b_delta_pic_order_always_zero )
    {
        bs_write_se( s, sh->i_delta_poc[0] );
        if( sh->pps->b_pic_order && !sh->b_field_pic )
        {
            bs_write_se( s, sh->i_delta_poc[1] );
        }
    }

    if( sh->pps->b_redundant_pic_cnt )
    {
        bs_write_ue( s, sh->i_redundant_pic_cnt );
    }

    if( sh->i_type == SLICE_TYPE_B )
    {
        bs_write1( s, sh->b_direct_spatial_mv_pred );
    }
    if( sh->i_type == SLICE_TYPE_P || sh->i_type == SLICE_TYPE_SP || sh->i_type == SLICE_TYPE_B )
    {
        bs_write1( s, sh->b_num_ref_idx_override );
        if( sh->b_num_ref_idx_override )
        {
            bs_write_ue( s, sh->i_num_ref_idx_l0_active - 1 );
            if( sh->i_type == SLICE_TYPE_B )
            {
                bs_write_ue( s, sh->i_num_ref_idx_l1_active - 1 );
            }
        }
    }

    /* ref pic list reordering */
    if( sh->i_type != SLICE_TYPE_I )
    {
        bs_write1( s, sh->b_ref_pic_list_reordering_l0 );
        if( sh->b_ref_pic_list_reordering_l0 )
        {
            for( i = 0; i < sh->i_num_ref_idx_l0_active; i++ )
            {
                bs_write_ue( s, sh->ref_pic_list_order[0][i].idc );
                bs_write_ue( s, sh->ref_pic_list_order[0][i].arg );

            }
            bs_write_ue( s, 3 );
        }
    }
    if( sh->i_type == SLICE_TYPE_B )
    {
        bs_write1( s, sh->b_ref_pic_list_reordering_l1 );
        if( sh->b_ref_pic_list_reordering_l1 )
        {
            for( i = 0; i < sh->i_num_ref_idx_l1_active; i++ )
            {
                bs_write_ue( s, sh->ref_pic_list_order[1][i].idc );
                bs_write_ue( s, sh->ref_pic_list_order[1][i].arg );
            }
            bs_write_ue( s, 3 );
        }
    }

    if( sh->pps->b_weighted_pred && ( sh->i_type == SLICE_TYPE_P || sh->i_type == SLICE_TYPE_SP ) )
    {
        /* pred_weight_table() */
        bs_write_ue( s, sh->weight[0][0].i_denom );
        bs_write_ue( s, sh->weight[0][1].i_denom );
        for( i = 0; i < sh->i_num_ref_idx_l0_active; i++ )
        {
            int luma_weight_l0_flag = !!sh->weight[i][0].weightfn;
            int chroma_weight_l0_flag = !!sh->weight[i][1].weightfn || !!sh->weight[i][2].weightfn;
            bs_write1( s, luma_weight_l0_flag );
            if( luma_weight_l0_flag )
            {
                bs_write_se( s, sh->weight[i][0].i_scale );
                bs_write_se( s, sh->weight[i][0].i_offset );
            }
            bs_write1( s, chroma_weight_l0_flag );
            if( chroma_weight_l0_flag )
            {
                int j;
                for( j = 1; j < 3; j++ )
                {
                    bs_write_se( s, sh->weight[i][j].i_scale );
                    bs_write_se( s, sh->weight[i][j].i_offset );
                }
            }
        }
    }
    else if( sh->pps->b_weighted_bipred == 1 && sh->i_type == SLICE_TYPE_B )
    {
      /* TODO */
    }

    if( i_nal_ref_idc != 0 )
    {
        if( sh->i_idr_pic_id >= 0 )
        {
            bs_write1( s, 0 );  /* no output of prior pics flag */
            bs_write1( s, 0 );  /* long term reference flag */
        }
        else
        {
            bs_write1( s, sh->i_mmco_command_count > 0 ); /* adaptive_ref_pic_marking_mode_flag */
            if( sh->i_mmco_command_count > 0 )
            {
                int i;
                for( i = 0; i < sh->i_mmco_command_count; i++ )
                {
                    bs_write_ue( s, 1 ); /* mark short term ref as unused */
                    bs_write_ue( s, sh->mmco[i].i_difference_of_pic_nums - 1 );
                }
                bs_write_ue( s, 0 ); /* end command list */
            }
        }
    }

    if( sh->pps->b_cabac && sh->i_type != SLICE_TYPE_I )
    {
        bs_write_ue( s, sh->i_cabac_init_idc );
    }
    bs_write_se( s, sh->i_qp_delta );      /* slice qp delta */

    if( sh->pps->b_deblocking_filter_control )
    {
        bs_write_ue( s, sh->i_disable_deblocking_filter_idc );
        if( sh->i_disable_deblocking_filter_idc != 1 )
        {
            bs_write_se( s, sh->i_alpha_c0_offset >> 1 );
            bs_write_se( s, sh->i_beta_offset >> 1 );
        }
    }
}

/* If we are within a reasonable distance of the end of the memory allocated for the bitstream, */
/* reallocate, adding an arbitrary amount of space (100 kilobytes). */
static int x264_bitstream_check_buffer( x264_t *h )
{
    uint8_t *bs_bak = h->out.p_bitstream;
    if( ( h->param.b_cabac && (h->cabac.p_end - h->cabac.p < 2500) )
     || ( h->out.bs.p_end - h->out.bs.p < 2500 ) )
    {
        intptr_t delta;
        int i;

        h->out.i_bitstream += 100000;
        CHECKED_MALLOC( h->out.p_bitstream, h->out.i_bitstream );
        h->mc.memcpy_aligned( h->out.p_bitstream, bs_bak, (h->out.i_bitstream - 100000) & ~15 );
        delta = h->out.p_bitstream - bs_bak;

        h->out.bs.p_start += delta;
        h->out.bs.p += delta;
        h->out.bs.p_end = h->out.p_bitstream + h->out.i_bitstream;

        h->cabac.p_start += delta;
        h->cabac.p += delta;
        h->cabac.p_end = h->out.p_bitstream + h->out.i_bitstream;

        for( i = 0; i <= h->out.i_nal; i++ )
            h->out.nal[i].p_payload += delta;
        x264_free( bs_bak );
    }
    return 0;
fail:
    x264_free( bs_bak );
    return -1;
}

/****************************************************************************
 *
 ****************************************************************************
 ****************************** External API*********************************
 ****************************************************************************
 *
 ****************************************************************************/

static int x264_validate_parameters( x264_t *h )
{
#ifdef HAVE_MMX
    if( !(x264_cpu_detect() & X264_CPU_SSE) )
    {
        x264_log( h, X264_LOG_ERROR, "your cpu does not support SSE1, but x264 was compiled with asm support\n");
        x264_log( h, X264_LOG_ERROR, "to run x264, recompile without asm support (configure --disable-asm)\n");
        return -1;
    }
#endif
    if( h->param.i_width <= 0 || h->param.i_height <= 0 )
    {
        x264_log( h, X264_LOG_ERROR, "invalid width x height (%dx%d)\n",
                  h->param.i_width, h->param.i_height );
        return -1;
    }

    if( h->param.i_width % 2 || h->param.i_height % 2 )
    {
        x264_log( h, X264_LOG_ERROR, "width or height not divisible by 2 (%dx%d)\n",
                  h->param.i_width, h->param.i_height );
        return -1;
    }
    if( h->param.i_csp != X264_CSP_I420 )
    {
        x264_log( h, X264_LOG_ERROR, "invalid CSP (only I420 supported)\n" );
        return -1;
    }

    if( h->param.i_threads == X264_THREADS_AUTO )
        h->param.i_threads = x264_cpu_num_processors() * 3/2;
    h->param.i_threads = x264_clip3( h->param.i_threads, 1, X264_THREAD_MAX );
    if( h->param.i_threads > 1 )
    {
#ifndef HAVE_PTHREAD
        x264_log( h, X264_LOG_WARNING, "not compiled with pthread support!\n");
        h->param.i_threads = 1;
#endif
    }

    if( h->param.b_interlaced )
    {
        if( h->param.analyse.i_me_method >= X264_ME_ESA )
        {
            x264_log( h, X264_LOG_WARNING, "interlace + me=esa is not implemented\n" );
            h->param.analyse.i_me_method = X264_ME_UMH;
        }
        if( h->param.analyse.i_direct_mv_pred > X264_DIRECT_PRED_SPATIAL )
        {
            x264_log( h, X264_LOG_WARNING, "interlace + direct=temporal is not implemented\n" );
            h->param.analyse.i_direct_mv_pred = X264_DIRECT_PRED_SPATIAL;
        }
        if( h->param.analyse.i_weighted_pred > 0 )
        {
            x264_log( h, X264_LOG_WARNING, "interlace + weightp is not implemented\n" );
            h->param.analyse.i_weighted_pred = X264_WEIGHTP_NONE;
        }
    }

    /* Detect default ffmpeg settings and terminate with an error. */
    {
        int score = 0;
        score += h->param.analyse.i_me_range == 0;
        score += h->param.rc.i_qp_step == 3;
        score += h->param.i_keyint_max == 12;
        score += h->param.rc.i_qp_min == 2;
        score += h->param.rc.i_qp_max == 31;
        score += h->param.rc.f_qcompress == 0.5;
        score += fabs(h->param.rc.f_ip_factor - 1.25) < 0.01;
        score += fabs(h->param.rc.f_pb_factor - 1.25) < 0.01;
        score += h->param.analyse.inter == 0 && h->param.analyse.i_subpel_refine == 8;
        if( score >= 5 )
        {
            x264_log( h, X264_LOG_ERROR, "broken ffmpeg default settings detected\n" );
            x264_log( h, X264_LOG_ERROR, "use an encoding preset (vpre)\n" );
            return -1;
        }
    }

    if( h->param.rc.i_rc_method < 0 || h->param.rc.i_rc_method > 2 )
    {
        x264_log( h, X264_LOG_ERROR, "no ratecontrol method specified\n" );
        return -1;
    }
    h->param.rc.f_rf_constant = x264_clip3f( h->param.rc.f_rf_constant, 0, 51 );
    h->param.rc.i_qp_constant = x264_clip3( h->param.rc.i_qp_constant, 0, 51 );
    if( h->param.rc.i_rc_method == X264_RC_CRF )
    {
        h->param.rc.i_qp_constant = h->param.rc.f_rf_constant;
        h->param.rc.i_bitrate = 0;
    }
    if( (h->param.rc.i_rc_method == X264_RC_CQP || h->param.rc.i_rc_method == X264_RC_CRF)
        && h->param.rc.i_qp_constant == 0 )
    {
        h->mb.b_lossless = 1;
        h->param.i_cqm_preset = X264_CQM_FLAT;
        h->param.psz_cqm_file = NULL;
        h->param.rc.i_rc_method = X264_RC_CQP;
        h->param.rc.f_ip_factor = 1;
        h->param.rc.f_pb_factor = 1;
        h->param.analyse.b_psnr = 0;
        h->param.analyse.b_ssim = 0;
        h->param.analyse.i_chroma_qp_offset = 0;
        h->param.analyse.i_trellis = 0;
        h->param.analyse.b_fast_pskip = 0;
        h->param.analyse.i_noise_reduction = 0;
        h->param.analyse.f_psy_rd = 0;
        h->param.i_bframe = 0;
        /* 8x8dct is not useful at all in CAVLC lossless */
        if( !h->param.b_cabac )
            h->param.analyse.b_transform_8x8 = 0;
    }
    if( h->param.rc.i_rc_method == X264_RC_CQP )
    {
        float qp_p = h->param.rc.i_qp_constant;
        float qp_i = qp_p - 6*log(h->param.rc.f_ip_factor)/log(2);
        float qp_b = qp_p + 6*log(h->param.rc.f_pb_factor)/log(2);
        h->param.rc.i_qp_min = x264_clip3( (int)(X264_MIN3( qp_p, qp_i, qp_b )), 0, 51 );
        h->param.rc.i_qp_max = x264_clip3( (int)(X264_MAX3( qp_p, qp_i, qp_b ) + .999), 0, 51 );
        h->param.rc.i_aq_mode = 0;
        h->param.rc.b_mb_tree = 0;
    }
    h->param.rc.i_qp_max = x264_clip3( h->param.rc.i_qp_max, 0, 51 );
    h->param.rc.i_qp_min = x264_clip3( h->param.rc.i_qp_min, 0, h->param.rc.i_qp_max );

    int max_slices = (h->param.i_height+((16<<h->param.b_interlaced)-1))/(16<<h->param.b_interlaced);
    h->param.i_slice_count = x264_clip3( h->param.i_slice_count, 0, max_slices );
    h->param.i_slice_max_size = X264_MAX( h->param.i_slice_max_size, 0 );
    h->param.i_slice_max_mbs = X264_MAX( h->param.i_slice_max_mbs, 0 );
    if( h->param.b_interlaced && h->param.i_slice_max_size )
    {
        x264_log( h, X264_LOG_WARNING, "interlaced + slice-max-size is not implemented\n" );
        h->param.i_slice_max_size = 0;
    }
    if( h->param.b_interlaced && h->param.i_slice_max_mbs )
    {
        x264_log( h, X264_LOG_WARNING, "interlaced + slice-max-mbs is not implemented\n" );
        h->param.i_slice_max_mbs = 0;
    }
    if( h->param.i_slice_max_mbs || h->param.i_slice_max_size )
        h->param.i_slice_count = 0;

    h->param.i_frame_reference = x264_clip3( h->param.i_frame_reference, 1, 16 );
    if( h->param.i_keyint_max <= 0 )
        h->param.i_keyint_max = 1;
    if( h->param.i_scenecut_threshold < 0 )
        h->param.i_scenecut_threshold = 0;
    h->param.i_keyint_min = x264_clip3( h->param.i_keyint_min, 1, h->param.i_keyint_max/2+1 );
    if( !h->param.analyse.i_subpel_refine && h->param.analyse.i_direct_mv_pred > X264_DIRECT_PRED_SPATIAL )
    {
        x264_log( h, X264_LOG_WARNING, "subme=0 + direct=temporal is not supported\n" );
        h->param.analyse.i_direct_mv_pred = X264_DIRECT_PRED_SPATIAL;
    }
    h->param.i_bframe = x264_clip3( h->param.i_bframe, 0, X264_BFRAME_MAX );
    if( h->param.i_keyint_max == 1 )
        h->param.i_bframe = 0;
    h->param.i_bframe_bias = x264_clip3( h->param.i_bframe_bias, -90, 100 );
    if( h->param.i_bframe <= 1 )
        h->param.i_bframe_pyramid = X264_B_PYRAMID_NONE;
    h->param.i_bframe_pyramid = x264_clip3( h->param.i_bframe_pyramid, X264_B_PYRAMID_NONE, X264_B_PYRAMID_NORMAL );
    if( !h->param.i_bframe )
    {
        h->param.i_bframe_adaptive = X264_B_ADAPT_NONE;
        h->param.analyse.i_direct_mv_pred = 0;
        h->param.analyse.b_weighted_bipred = 0;
    }
    h->param.rc.i_lookahead = x264_clip3( h->param.rc.i_lookahead, 0, X264_LOOKAHEAD_MAX );
    {
        int maxrate = X264_MAX( h->param.rc.i_vbv_max_bitrate, h->param.rc.i_bitrate );
        float bufsize = maxrate ? (float)h->param.rc.i_vbv_buffer_size / maxrate : 0;
        float fps = h->param.i_fps_num > 0 && h->param.i_fps_den > 0 ? (float) h->param.i_fps_num / h->param.i_fps_den : 25.0;
        h->param.rc.i_lookahead = X264_MIN( h->param.rc.i_lookahead, X264_MAX( h->param.i_keyint_max, bufsize*fps ) );
    }

    h->param.rc.f_qcompress = x264_clip3f( h->param.rc.f_qcompress, 0.0, 1.0 );
    if( !h->param.rc.i_lookahead || h->param.i_keyint_max == 1 || h->param.rc.f_qcompress == 1 )
        h->param.rc.b_mb_tree = 0;
    if( h->param.rc.b_stat_read )
        h->param.rc.i_lookahead = 0;
#ifdef HAVE_PTHREAD
    if( h->param.i_sync_lookahead )
        h->param.i_sync_lookahead = x264_clip3( h->param.i_sync_lookahead, h->param.i_threads + h->param.i_bframe, X264_LOOKAHEAD_MAX );
    if( h->param.rc.b_stat_read || h->param.i_threads == 1 )
        h->param.i_sync_lookahead = 0;
#else
    h->param.i_sync_lookahead = 0;
#endif

    h->mb.b_direct_auto_write = h->param.analyse.i_direct_mv_pred == X264_DIRECT_PRED_AUTO
                                && h->param.i_bframe
                                && ( h->param.rc.b_stat_write || !h->param.rc.b_stat_read );

    h->param.i_deblocking_filter_alphac0 = x264_clip3( h->param.i_deblocking_filter_alphac0, -6, 6 );
    h->param.i_deblocking_filter_beta    = x264_clip3( h->param.i_deblocking_filter_beta, -6, 6 );
    h->param.analyse.i_luma_deadzone[0] = x264_clip3( h->param.analyse.i_luma_deadzone[0], 0, 32 );
    h->param.analyse.i_luma_deadzone[1] = x264_clip3( h->param.analyse.i_luma_deadzone[1], 0, 32 );

    h->param.i_cabac_init_idc = x264_clip3( h->param.i_cabac_init_idc, 0, 2 );

    if( h->param.i_cqm_preset < X264_CQM_FLAT || h->param.i_cqm_preset > X264_CQM_CUSTOM )
        h->param.i_cqm_preset = X264_CQM_FLAT;

    if( h->param.analyse.i_me_method < X264_ME_DIA ||
        h->param.analyse.i_me_method > X264_ME_TESA )
        h->param.analyse.i_me_method = X264_ME_HEX;
    if( h->param.analyse.i_me_range < 4 )
        h->param.analyse.i_me_range = 4;
    if( h->param.analyse.i_me_range > 16 && h->param.analyse.i_me_method <= X264_ME_HEX )
        h->param.analyse.i_me_range = 16;
    if( h->param.analyse.i_me_method == X264_ME_TESA &&
        (h->mb.b_lossless || h->param.analyse.i_subpel_refine <= 1) )
        h->param.analyse.i_me_method = X264_ME_ESA;
    h->param.analyse.i_subpel_refine = x264_clip3( h->param.analyse.i_subpel_refine, 0, 10 );
    h->param.analyse.b_mixed_references = h->param.analyse.b_mixed_references && h->param.i_frame_reference > 1;
    h->param.analyse.inter &= X264_ANALYSE_PSUB16x16|X264_ANALYSE_PSUB8x8|X264_ANALYSE_BSUB16x16|
                              X264_ANALYSE_I4x4|X264_ANALYSE_I8x8;
    h->param.analyse.intra &= X264_ANALYSE_I4x4|X264_ANALYSE_I8x8;
    if( !(h->param.analyse.inter & X264_ANALYSE_PSUB16x16) )
        h->param.analyse.inter &= ~X264_ANALYSE_PSUB8x8;
    if( !h->param.analyse.b_transform_8x8 )
    {
        h->param.analyse.inter &= ~X264_ANALYSE_I8x8;
        h->param.analyse.intra &= ~X264_ANALYSE_I8x8;
    }
    h->param.analyse.i_chroma_qp_offset = x264_clip3(h->param.analyse.i_chroma_qp_offset, -12, 12);
    if( !h->param.b_cabac )
        h->param.analyse.i_trellis = 0;
    h->param.analyse.i_trellis = x264_clip3( h->param.analyse.i_trellis, 0, 2 );
    if( !h->param.analyse.b_psy )
    {
        h->param.analyse.f_psy_rd = 0;
        h->param.analyse.f_psy_trellis = 0;
    }
    if( !h->param.analyse.i_trellis )
        h->param.analyse.f_psy_trellis = 0;
    h->param.analyse.f_psy_rd = x264_clip3f( h->param.analyse.f_psy_rd, 0, 10 );
    h->param.analyse.f_psy_trellis = x264_clip3f( h->param.analyse.f_psy_trellis, 0, 10 );
    if( h->param.analyse.i_subpel_refine < 6 )
        h->param.analyse.f_psy_rd = 0;
    h->mb.i_psy_rd = FIX8( h->param.analyse.f_psy_rd );
    /* Psy RDO increases overall quantizers to improve the quality of luma--this indirectly hurts chroma quality */
    /* so we lower the chroma QP offset to compensate */
    /* This can be triggered repeatedly on multiple calls to parameter_validate, but since encoding
     * uses the pps chroma qp offset not the param chroma qp offset, this is not a problem. */
    if( h->mb.i_psy_rd )
        h->param.analyse.i_chroma_qp_offset -= h->param.analyse.f_psy_rd < 0.25 ? 1 : 2;
    h->mb.i_psy_trellis = FIX8( h->param.analyse.f_psy_trellis / 4 );
    /* Psy trellis has a similar effect. */
    if( h->mb.i_psy_trellis )
        h->param.analyse.i_chroma_qp_offset -= h->param.analyse.f_psy_trellis < 0.25 ? 1 : 2;
    else
        h->mb.i_psy_trellis = 0;
    h->param.analyse.i_chroma_qp_offset = x264_clip3(h->param.analyse.i_chroma_qp_offset, -12, 12);
    h->param.rc.i_aq_mode = x264_clip3( h->param.rc.i_aq_mode, 0, 2 );
    h->param.rc.f_aq_strength = x264_clip3f( h->param.rc.f_aq_strength, 0, 3 );
    if( h->param.rc.f_aq_strength == 0 )
        h->param.rc.i_aq_mode = 0;
    /* MB-tree requires AQ to be on, even if the strength is zero. */
    if( !h->param.rc.i_aq_mode && h->param.rc.b_mb_tree )
    {
        h->param.rc.i_aq_mode = 1;
        h->param.rc.f_aq_strength = 0;
    }
    if( h->param.rc.b_mb_tree && h->param.i_bframe_pyramid )
    {
        x264_log( h, X264_LOG_WARNING, "b-pyramid + mb-tree is not supported\n" );
        h->param.i_bframe_pyramid = X264_B_PYRAMID_NONE;
    }
    h->param.analyse.i_noise_reduction = x264_clip3( h->param.analyse.i_noise_reduction, 0, 1<<16 );
    if( h->param.analyse.i_subpel_refine == 10 && (h->param.analyse.i_trellis != 2 || !h->param.rc.i_aq_mode) )
        h->param.analyse.i_subpel_refine = 9;

    {
        const x264_level_t *l = x264_levels;
        if( h->param.i_level_idc < 0 )
        {
            int maxrate_bak = h->param.rc.i_vbv_max_bitrate;
            if( h->param.rc.i_rc_method == X264_RC_ABR && h->param.rc.i_vbv_buffer_size <= 0 )
                h->param.rc.i_vbv_max_bitrate = h->param.rc.i_bitrate * 2;
            h->sps = h->sps_array;
            x264_sps_init( h->sps, h->param.i_sps_id, &h->param );
            do h->param.i_level_idc = l->level_idc;
                while( l[1].level_idc && x264_validate_levels( h, 0 ) && l++ );
            h->param.rc.i_vbv_max_bitrate = maxrate_bak;
        }
        else
        {
            while( l->level_idc && l->level_idc != h->param.i_level_idc )
                l++;
            if( l->level_idc == 0 )
            {
                x264_log( h, X264_LOG_ERROR, "invalid level_idc: %d\n", h->param.i_level_idc );
                return -1;
            }
        }
        if( h->param.analyse.i_mv_range <= 0 )
            h->param.analyse.i_mv_range = l->mv_range >> h->param.b_interlaced;
        else
            h->param.analyse.i_mv_range = x264_clip3(h->param.analyse.i_mv_range, 32, 512 >> h->param.b_interlaced);
    }

    h->param.analyse.i_weighted_pred = x264_clip3( h->param.analyse.i_weighted_pred, 0, X264_WEIGHTP_SMART );
    if( !h->param.analyse.i_weighted_pred && h->param.rc.b_mb_tree && h->param.analyse.b_psy && !h->param.b_interlaced )
        h->param.analyse.i_weighted_pred = X264_WEIGHTP_FAKE;

    if( h->param.i_threads > 1 )
    {
        int r = h->param.analyse.i_mv_range_thread;
        int r2;
        if( r <= 0 )
        {
            // half of the available space is reserved and divided evenly among the threads,
            // the rest is allocated to whichever thread is far enough ahead to use it.
            // reserving more space increases quality for some videos, but costs more time
            // in thread synchronization.
            int max_range = (h->param.i_height + X264_THREAD_HEIGHT) / h->param.i_threads - X264_THREAD_HEIGHT;
            r = max_range / 2;
        }
        r = X264_MAX( r, h->param.analyse.i_me_range );
        r = X264_MIN( r, h->param.analyse.i_mv_range );
        // round up to use the whole mb row
        r2 = (r & ~15) + ((-X264_THREAD_HEIGHT) & 15);
        if( r2 < r )
            r2 += 16;
        x264_log( h, X264_LOG_DEBUG, "using mv_range_thread = %d\n", r2 );
        h->param.analyse.i_mv_range_thread = r2;
    }

    if( h->param.rc.f_qblur < 0 )
        h->param.rc.f_qblur = 0;
    if( h->param.rc.f_complexity_blur < 0 )
        h->param.rc.f_complexity_blur = 0;

    h->param.i_sps_id &= 31;

    if( h->param.i_log_level < X264_LOG_INFO )
    {
        h->param.analyse.b_psnr = 0;
        h->param.analyse.b_ssim = 0;
    }

    /* ensure the booleans are 0 or 1 so they can be used in math */
#define BOOLIFY(x) h->param.x = !!h->param.x
    BOOLIFY( b_cabac );
    BOOLIFY( b_deblocking_filter );
    BOOLIFY( b_interlaced );
    BOOLIFY( analyse.b_transform_8x8 );
    BOOLIFY( analyse.b_chroma_me );
    BOOLIFY( analyse.b_fast_pskip );
    BOOLIFY( rc.b_stat_write );
    BOOLIFY( rc.b_stat_read );
    BOOLIFY( rc.b_mb_tree );
#undef BOOLIFY

    return 0;
}

static void mbcmp_init( x264_t *h )
{
    int satd = !h->mb.b_lossless && h->param.analyse.i_subpel_refine > 1;
    memcpy( h->pixf.mbcmp, satd ? h->pixf.satd : h->pixf.sad_aligned, sizeof(h->pixf.mbcmp) );
    memcpy( h->pixf.mbcmp_unaligned, satd ? h->pixf.satd : h->pixf.sad, sizeof(h->pixf.mbcmp_unaligned) );
    h->pixf.intra_mbcmp_x3_16x16 = satd ? h->pixf.intra_satd_x3_16x16 : h->pixf.intra_sad_x3_16x16;
    h->pixf.intra_mbcmp_x3_8x8c = satd ? h->pixf.intra_satd_x3_8x8c : h->pixf.intra_sad_x3_8x8c;
    h->pixf.intra_mbcmp_x3_4x4 = satd ? h->pixf.intra_satd_x3_4x4 : h->pixf.intra_sad_x3_4x4;
    satd &= h->param.analyse.i_me_method == X264_ME_TESA;
    memcpy( h->pixf.fpelcmp, satd ? h->pixf.satd : h->pixf.sad, sizeof(h->pixf.fpelcmp) );
    memcpy( h->pixf.fpelcmp_x3, satd ? h->pixf.satd_x3 : h->pixf.sad_x3, sizeof(h->pixf.fpelcmp_x3) );
    memcpy( h->pixf.fpelcmp_x4, satd ? h->pixf.satd_x4 : h->pixf.sad_x4, sizeof(h->pixf.fpelcmp_x4) );
}

static void x264_set_aspect_ratio( x264_t *h, x264_param_t *param, int initial )
{
    /* VUI */
    if( param->vui.i_sar_width > 0 && param->vui.i_sar_height > 0 )
    {
        int i_w = param->vui.i_sar_width;
        int i_h = param->vui.i_sar_height;
        int old_w = h->param.vui.i_sar_width;
        int old_h = h->param.vui.i_sar_height;

        x264_reduce_fraction( &i_w, &i_h );

        while( i_w > 65535 || i_h > 65535 )
        {
            i_w /= 2;
            i_h /= 2;
        }

        x264_reduce_fraction( &i_w, &i_h );

        if( i_w != old_w || i_h != old_h || initial )
        {
            h->param.vui.i_sar_width = 0;
            h->param.vui.i_sar_height = 0;
            if( i_w == 0 || i_h == 0 )
                x264_log( h, X264_LOG_WARNING, "cannot create valid sample aspect ratio\n" );
            else
            {
                x264_log( h, initial?X264_LOG_INFO:X264_LOG_DEBUG, "using SAR=%d/%d\n", i_w, i_h );
                h->param.vui.i_sar_width = i_w;
                h->param.vui.i_sar_height = i_h;
            }
        }
    }
}

/****************************************************************************
 * x264_encoder_open:
 ****************************************************************************/
x264_t *x264_encoder_open( x264_param_t *param )
{
    x264_t *h;
    char buf[1000], *p;
    int i, qp, i_slicetype_length;

    CHECKED_MALLOCZERO( h, sizeof(x264_t) );

    /* Create a copy of param */
    memcpy( &h->param, param, sizeof(x264_param_t) );

    if( param->param_free )
        param->param_free( param );

    if( x264_validate_parameters( h ) < 0 )
        goto fail;

    if( h->param.psz_cqm_file )
        if( x264_cqm_parse_file( h, h->param.psz_cqm_file ) < 0 )
            goto fail;

    if( h->param.rc.psz_stat_out )
        h->param.rc.psz_stat_out = strdup( h->param.rc.psz_stat_out );
    if( h->param.rc.psz_stat_in )
        h->param.rc.psz_stat_in = strdup( h->param.rc.psz_stat_in );

    x264_set_aspect_ratio( h, param, 1 );

    x264_reduce_fraction( &h->param.i_fps_num, &h->param.i_fps_den );

    /* Init x264_t */
    h->i_frame = -1;
    h->i_frame_num = 0;
    h->i_idr_pic_id = 0;

    h->sps = &h->sps_array[0];
    x264_sps_init( h->sps, h->param.i_sps_id, &h->param );

    h->pps = &h->pps_array[0];
    x264_pps_init( h->pps, h->param.i_sps_id, &h->param, h->sps );

    x264_validate_levels( h, 1 );

    h->chroma_qp_table = i_chroma_qp_table + 12 + h->pps->i_chroma_qp_index_offset;

    if( x264_cqm_init( h ) < 0 )
        goto fail;

    h->mb.i_mb_count = h->sps->i_mb_width * h->sps->i_mb_height;

    /* Init frames. */
    if( h->param.i_bframe_adaptive == X264_B_ADAPT_TRELLIS )
        h->frames.i_delay = X264_MAX(h->param.i_bframe,3)*4;
    else
        h->frames.i_delay = h->param.i_bframe;
    if( h->param.rc.b_mb_tree || h->param.rc.i_vbv_buffer_size )
        h->frames.i_delay = X264_MAX( h->frames.i_delay, h->param.rc.i_lookahead );
    i_slicetype_length = h->frames.i_delay;
    h->frames.i_delay += h->param.i_threads - 1;
    h->frames.i_delay = X264_MIN( h->frames.i_delay, X264_LOOKAHEAD_MAX );
    h->frames.i_delay += h->param.i_sync_lookahead;

    h->frames.i_max_ref0 = h->param.i_frame_reference;
    h->frames.i_max_ref1 = h->sps->vui.i_num_reorder_frames;
    h->frames.i_max_dpb  = h->sps->vui.i_max_dec_frame_buffering;
    h->frames.b_have_lowres = !h->param.rc.b_stat_read
        && ( h->param.rc.i_rc_method == X264_RC_ABR
          || h->param.rc.i_rc_method == X264_RC_CRF
          || h->param.i_bframe_adaptive
          || h->param.i_scenecut_threshold
          || h->param.rc.b_mb_tree );
    h->frames.b_have_lowres |= h->param.rc.b_stat_read && h->param.rc.i_vbv_buffer_size > 0;
    h->frames.b_have_sub8x8_esa = !!(h->param.analyse.inter & X264_ANALYSE_PSUB8x8);

    h->frames.i_last_idr = - h->param.i_keyint_max;
    h->frames.i_input    = 0;

    CHECKED_MALLOCZERO( h->frames.unused[0], (h->frames.i_delay + 3) * sizeof(x264_frame_t *) );
    /* Allocate room for max refs plus a few extra just in case. */
    CHECKED_MALLOCZERO( h->frames.unused[1], (h->param.i_threads + 20) * sizeof(x264_frame_t *) );
    CHECKED_MALLOCZERO( h->frames.current, (h->param.i_sync_lookahead + h->param.i_bframe
                        + h->param.i_threads + 3) * sizeof(x264_frame_t *) );
    if( h->param.analyse.i_weighted_pred > 0 )
        CHECKED_MALLOCZERO( h->frames.blank_unused, h->param.i_threads * 4 * sizeof(x264_frame_t *) );
    h->i_ref0 = 0;
    h->i_ref1 = 0;

    x264_rdo_init();

    /* init CPU functions */
    x264_predict_16x16_init( h->param.cpu, h->predict_16x16 );
    x264_predict_8x8c_init( h->param.cpu, h->predict_8x8c );
    x264_predict_8x8_init( h->param.cpu, h->predict_8x8, &h->predict_8x8_filter );
    x264_predict_4x4_init( h->param.cpu, h->predict_4x4 );
    if( !h->param.b_cabac )
        x264_init_vlc_tables();
    x264_pixel_init( h->param.cpu, &h->pixf );
    x264_dct_init( h->param.cpu, &h->dctf );
    x264_zigzag_init( h->param.cpu, &h->zigzagf, h->param.b_interlaced );
    x264_mc_init( h->param.cpu, &h->mc );
    x264_quant_init( h, h->param.cpu, &h->quantf );
    x264_deblock_init( h->param.cpu, &h->loopf );
    x264_dct_init_weights();

    mbcmp_init( h );

    p = buf + sprintf( buf, "using cpu capabilities:" );
    for( i=0; x264_cpu_names[i].flags; i++ )
    {
        if( !strcmp(x264_cpu_names[i].name, "SSE2")
            && param->cpu & (X264_CPU_SSE2_IS_FAST|X264_CPU_SSE2_IS_SLOW) )
            continue;
        if( !strcmp(x264_cpu_names[i].name, "SSE3")
            && (param->cpu & X264_CPU_SSSE3 || !(param->cpu & X264_CPU_CACHELINE_64)) )
            continue;
        if( !strcmp(x264_cpu_names[i].name, "SSE4.1")
            && (param->cpu & X264_CPU_SSE42) )
            continue;
        if( (param->cpu & x264_cpu_names[i].flags) == x264_cpu_names[i].flags
            && (!i || x264_cpu_names[i].flags != x264_cpu_names[i-1].flags) )
            p += sprintf( p, " %s", x264_cpu_names[i].name );
    }
    if( !param->cpu )
        p += sprintf( p, " none!" );
    x264_log( h, X264_LOG_INFO, "%s\n", buf );

    for( qp = h->param.rc.i_qp_min; qp <= h->param.rc.i_qp_max; qp++ )
        if( x264_analyse_init_costs( h, qp ) )
            goto fail;
    if( x264_analyse_init_costs( h, X264_LOOKAHEAD_QP ) )
        goto fail;
    if( h->cost_mv[1][2013] != 24 )
    {
        x264_log( h, X264_LOG_ERROR, "MV cost test failed: x264 has been miscompiled!\n" );
        goto fail;
    }

    h->out.i_nal = 0;
    h->out.i_bitstream = X264_MAX( 1000000, h->param.i_width * h->param.i_height * 4
        * ( h->param.rc.i_rc_method == X264_RC_ABR ? pow( 0.95, h->param.rc.i_qp_min )
          : pow( 0.95, h->param.rc.i_qp_constant ) * X264_MAX( 1, h->param.rc.f_ip_factor )));

    CHECKED_MALLOC( h->nal_buffer, h->out.i_bitstream * 3/2 + 4 );
    h->nal_buffer_size = h->out.i_bitstream * 3/2 + 4;

    h->thread[0] = h;
    h->i_thread_num = 0;
    for( i = 1; i < h->param.i_threads + !!h->param.i_sync_lookahead; i++ )
        CHECKED_MALLOC( h->thread[i], sizeof(x264_t) );

    for( i = 0; i < h->param.i_threads; i++ )
    {
        if( i > 0 )
            *h->thread[i] = *h;
        h->thread[i]->fdec = x264_frame_pop_unused( h, 1 );
        if( !h->thread[i]->fdec )
            goto fail;
        CHECKED_MALLOC( h->thread[i]->out.p_bitstream, h->out.i_bitstream );
        /* Start each thread with room for 8 NAL units; it'll realloc later if needed. */
        CHECKED_MALLOC( h->thread[i]->out.nal, 8*sizeof(x264_nal_t) );
        h->thread[i]->out.i_nals_allocated = 8;
        if( x264_macroblock_cache_init( h->thread[i] ) < 0 )
            goto fail;
    }

    if( x264_lookahead_init( h, i_slicetype_length ) )
        goto fail;

    if( x264_ratecontrol_new( h ) < 0 )
        goto fail;

    if( h->param.psz_dump_yuv )
    {
        /* create or truncate the reconstructed video file */
        FILE *f = fopen( h->param.psz_dump_yuv, "w" );
        if( !f )
        {
            x264_log( h, X264_LOG_ERROR, "dump_yuv: can't write to %s\n", h->param.psz_dump_yuv );
            goto fail;
        }
        else if( !x264_is_regular_file( f ) )
        {
            x264_log( h, X264_LOG_ERROR, "dump_yuv: incompatible with non-regular file %s\n", h->param.psz_dump_yuv );
            goto fail;
        }
        fclose( f );
    }

    x264_log( h, X264_LOG_INFO, "profile %s, level %d.%d\n",
        h->sps->i_profile_idc == PROFILE_BASELINE ? "Baseline" :
        h->sps->i_profile_idc == PROFILE_MAIN ? "Main" :
        h->sps->i_profile_idc == PROFILE_HIGH ? "High" :
        "High 4:4:4 Predictive", h->sps->i_level_idc/10, h->sps->i_level_idc%10 );

    return h;
fail:
    x264_free( h );
    return NULL;
}

/****************************************************************************
 * x264_encoder_reconfig:
 ****************************************************************************/
int x264_encoder_reconfig( x264_t *h, x264_param_t *param )
{
    h = h->thread[h->i_thread_phase];
    x264_set_aspect_ratio( h, param, 0 );
#define COPY(var) h->param.var = param->var
    COPY( i_frame_reference ); // but never uses more refs than initially specified
    COPY( i_bframe_bias );
    if( h->param.i_scenecut_threshold )
        COPY( i_scenecut_threshold ); // can't turn it on or off, only vary the threshold
    COPY( b_deblocking_filter );
    COPY( i_deblocking_filter_alphac0 );
    COPY( i_deblocking_filter_beta );
    COPY( analyse.intra );
    COPY( analyse.inter );
    COPY( analyse.i_direct_mv_pred );
    /* Scratch buffer prevents me_range from being increased for esa/tesa */
    if( h->param.analyse.i_me_method < X264_ME_ESA || param->analyse.i_me_range < h->param.analyse.i_me_range )
        COPY( analyse.i_me_range );
    COPY( analyse.i_noise_reduction );
    /* We can't switch out of subme=0 during encoding. */
    if( h->param.analyse.i_subpel_refine )
        COPY( analyse.i_subpel_refine );
    COPY( analyse.i_trellis );
    COPY( analyse.b_chroma_me );
    COPY( analyse.b_dct_decimate );
    COPY( analyse.b_fast_pskip );
    COPY( analyse.b_mixed_references );
    COPY( analyse.f_psy_rd );
    COPY( analyse.f_psy_trellis );
    // can only twiddle these if they were enabled to begin with:
    if( h->param.analyse.i_me_method >= X264_ME_ESA || param->analyse.i_me_method < X264_ME_ESA )
        COPY( analyse.i_me_method );
    if( h->param.analyse.i_me_method >= X264_ME_ESA && !h->frames.b_have_sub8x8_esa )
        h->param.analyse.inter &= ~X264_ANALYSE_PSUB8x8;
    if( h->pps->b_transform_8x8_mode )
        COPY( analyse.b_transform_8x8 );
    if( h->frames.i_max_ref1 > 1 )
        COPY( i_bframe_pyramid );
    COPY( i_slice_max_size );
    COPY( i_slice_max_mbs );
    COPY( i_slice_count );
#undef COPY

    mbcmp_init( h );

    return x264_validate_parameters( h );
}

/* internal usage */
static void x264_nal_start( x264_t *h, int i_type, int i_ref_idc )
{
    x264_nal_t *nal = &h->out.nal[h->out.i_nal];

    nal->i_ref_idc = i_ref_idc;
    nal->i_type    = i_type;

    nal->i_payload= 0;
    nal->p_payload= &h->out.p_bitstream[bs_pos( &h->out.bs ) / 8];
}
static int x264_nal_end( x264_t *h )
{
    x264_nal_t *nal = &h->out.nal[h->out.i_nal];
    nal->i_payload = &h->out.p_bitstream[bs_pos( &h->out.bs ) / 8] - nal->p_payload;
    h->out.i_nal++;

    /* if number of allocated nals is not enough, re-allocate a larger one. */
    if( h->out.i_nal >= h->out.i_nals_allocated )
    {
        x264_nal_t *new_out = x264_malloc( sizeof(x264_nal_t) * (h->out.i_nals_allocated*2) );
        if( !new_out )
            return -1;
        memcpy( new_out, h->out.nal, sizeof(x264_nal_t) * (h->out.i_nals_allocated) );
        x264_free( h->out.nal );
        h->out.nal = new_out;
        h->out.i_nals_allocated *= 2;
    }
    return 0;
}

static int x264_encoder_encapsulate_nals( x264_t *h )
{
    int nal_size = 0, i;
    for( i = 0; i < h->out.i_nal; i++ )
        nal_size += h->out.nal[i].i_payload;

    /* Worst-case NAL unit escaping: reallocate the buffer if it's too small. */
    if( h->nal_buffer_size < nal_size * 3/2 + h->out.i_nal * 4 )
    {
        uint8_t *buf = x264_malloc( nal_size * 2 + h->out.i_nal * 4 );
        if( !buf )
            return -1;
        x264_free( h->nal_buffer );
        h->nal_buffer = buf;
    }

    uint8_t *nal_buffer = h->nal_buffer;

    for( i = 0; i < h->out.i_nal; i++ )
    {
        int size = x264_nal_encode( nal_buffer, h->param.b_annexb, &h->out.nal[i] );
        h->out.nal[i].i_payload = size;
        h->out.nal[i].p_payload = nal_buffer;
        nal_buffer += size;
    }

    return nal_buffer - h->nal_buffer;
}

/****************************************************************************
 * x264_encoder_headers:
 ****************************************************************************/
int x264_encoder_headers( x264_t *h, x264_nal_t **pp_nal, int *pi_nal )
{
    int frame_size = 0;
    /* init bitstream context */
    h->out.i_nal = 0;
    bs_init( &h->out.bs, h->out.p_bitstream, h->out.i_bitstream );

    /* Write SEI, SPS and PPS. */
    x264_nal_start( h, NAL_SEI, NAL_PRIORITY_DISPOSABLE );
    if( x264_sei_version_write( h, &h->out.bs ) )
        return -1;
    if( x264_nal_end( h ) )
        return -1;

    /* generate sequence parameters */
    x264_nal_start( h, NAL_SPS, NAL_PRIORITY_HIGHEST );
    x264_sps_write( &h->out.bs, h->sps );
    if( x264_nal_end( h ) )
        return -1;

    /* generate picture parameters */
    x264_nal_start( h, NAL_PPS, NAL_PRIORITY_HIGHEST );
    x264_pps_write( &h->out.bs, h->pps );
    if( x264_nal_end( h ) )
        return -1;
    bs_flush( &h->out.bs );

    frame_size = x264_encoder_encapsulate_nals( h );

    /* now set output*/
    *pi_nal = h->out.i_nal;
    *pp_nal = &h->out.nal[0];
    h->out.i_nal = 0;

    return frame_size;
}

/* Check to see whether we have chosen a reference list ordering different
 * from the standard's default. */
static inline void x264_reference_check_reorder( x264_t *h )
{
    int i;
    for( i = 0; i < h->i_ref0 - 1; i++ )
        /* P and B-frames use different default orders. */
        if( h->sh.i_type == SLICE_TYPE_P ? h->fref0[i]->i_frame_num < h->fref0[i+1]->i_frame_num
                                         : h->fref0[i]->i_poc < h->fref0[i+1]->i_poc )
        {
            h->b_ref_reorder[0] = 1;
            break;
        }
}

/* return -1 on failure, else return the index of the new reference frame */
int x264_weighted_reference_duplicate( x264_t *h, int i_ref, const x264_weight_t *w )
{
    int i = h->i_ref0;
    int j;
    x264_frame_t *newframe;
    if( i <= 1 ) /* empty list, definitely can't duplicate frame */
        return -1;

    /* Find a place to insert the duplicate in the reference list. */
    for( j = 0; j < i; j++ )
        if( h->fref0[i_ref]->i_frame != h->fref0[j]->i_frame )
        {
            /* found a place, after j, make sure there is not already a duplicate there */
            if( j == i-1 || ( h->fref0[j+1] && h->fref0[i_ref]->i_frame != h->fref0[j+1]->i_frame ) )
                break;
        }

    if( j == i ) /* No room in the reference list for the duplicate. */
        return -1;
    j++;

    newframe = x264_frame_pop_blank_unused( h );

    //FIXME: probably don't need to copy everything
    *newframe = *h->fref0[i_ref];
    newframe->i_reference_count = 1;
    newframe->orig = h->fref0[i_ref];
    newframe->b_duplicate = 1;
    memcpy( h->fenc->weight[j], w, sizeof(h->fenc->weight[i]) );

    /* shift the frames to make space for the dupe. */
    h->b_ref_reorder[0] = 1;
    if( h->i_ref0 < 16 )
        ++h->i_ref0;
    h->fref0[15] = NULL;
    x264_frame_unshift( &h->fref0[j], newframe );

    return j;
}

static void x264_weighted_pred_init( x264_t *h )
{
    int i_ref;
    int i;

    /* for now no analysis and set all weights to nothing */
    for( i_ref = 0; i_ref < h->i_ref0; i_ref++ )
        h->fenc->weighted[i_ref] = h->fref0[i_ref]->filtered[0];

    // FIXME: This only supports weighting of one reference frame
    // and duplicates of that frame.
    h->fenc->i_lines_weighted = 0;

    for( i_ref = 0; i_ref < h->i_ref0; i_ref++ )
        for( i = 0; i < 3; i++ )
            h->sh.weight[i_ref][i].weightfn = NULL;


    if( h->sh.i_type != SLICE_TYPE_P || h->param.analyse.i_weighted_pred <= 0 )
        return;

    int i_padv = PADV << h->param.b_interlaced;
    int denom = -1;
    int weightluma = 0;
    int buffer_next = 0;
    int j;
    //FIXME: when chroma support is added, move this into loop
    h->sh.weight[0][1].weightfn = h->sh.weight[0][2].weightfn = NULL;
    h->sh.weight[0][1].i_denom = h->sh.weight[0][2].i_denom = 0;
    for( j = 0; j < h->i_ref0; j++ )
    {
        if( h->fenc->weight[j][0].weightfn )
        {
            h->sh.weight[j][0] = h->fenc->weight[j][0];
            // if weight is useless, don't write it to stream
            if( h->sh.weight[j][0].i_scale == 1<<h->sh.weight[j][0].i_denom && h->sh.weight[j][0].i_offset == 0 )
                h->sh.weight[j][0].weightfn = NULL;
            else
            {
                if( !weightluma )
                {
                    weightluma = 1;
                    h->sh.weight[0][0].i_denom = denom = h->sh.weight[j][0].i_denom;
                }
                assert( h->sh.weight[j][0].i_denom == denom );
                h->fenc->weighted[j] = h->mb.p_weight_buf[buffer_next++] +
                    h->fenc->i_stride[0] * i_padv + PADH;
            }
        }

        //scale full resolution frame
        if( h->sh.weight[j][0].weightfn && h->param.i_threads == 1 )
        {
            uint8_t *src = h->fref0[j]->filtered[0] - h->fref0[j]->i_stride[0]*i_padv - PADH;
            uint8_t *dst = h->fenc->weighted[j] - h->fenc->i_stride[0]*i_padv - PADH;
            int stride = h->fenc->i_stride[0];
            int width = h->fenc->i_width[0] + PADH*2;
            int height = h->fenc->i_lines[0] + i_padv*2;
            x264_weight_scale_plane( h, dst, stride, src, stride, width, height, &h->sh.weight[j][0] );
            h->fenc->i_lines_weighted = height;
        }
    }
    if( !weightluma )
        h->sh.weight[0][0].i_denom = 0;
}

static inline void x264_reference_build_list( x264_t *h, int i_poc )
{
    int i;
    int b_ok;

    /* build ref list 0/1 */
    h->i_ref0 = 0;
    h->i_ref1 = 0;
    for( i = 0; h->frames.reference[i]; i++ )
    {
        if( h->frames.reference[i]->i_poc < i_poc )
        {
            h->fref0[h->i_ref0++] = h->frames.reference[i];
        }
        else if( h->frames.reference[i]->i_poc > i_poc )
        {
            h->fref1[h->i_ref1++] = h->frames.reference[i];
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
                XCHG( x264_frame_t*, h->fref0[i], h->fref0[i+1] );
                b_ok = 0;
                break;
            }
        }
    } while( !b_ok );

    if( h->sh.i_mmco_remove_from_end )
        for( i = h->i_ref0-1; i >= h->i_ref0 - h->sh.i_mmco_remove_from_end; i-- )
        {
            int diff = h->i_frame_num - h->fref0[i]->i_frame_num;
            h->sh.mmco[h->sh.i_mmco_command_count].i_poc = h->fref0[i]->i_poc;
            h->sh.mmco[h->sh.i_mmco_command_count++].i_difference_of_pic_nums = diff;
        }

    /* Order ref1 from lower to higher poc (bubble sort) for B-frame */
    do
    {
        b_ok = 1;
        for( i = 0; i < h->i_ref1 - 1; i++ )
        {
            if( h->fref1[i]->i_poc > h->fref1[i+1]->i_poc )
            {
                XCHG( x264_frame_t*, h->fref1[i], h->fref1[i+1] );
                b_ok = 0;
                break;
            }
        }
    } while( !b_ok );

    x264_reference_check_reorder( h );

    h->i_ref1 = X264_MIN( h->i_ref1, h->frames.i_max_ref1 );
    h->i_ref0 = X264_MIN( h->i_ref0, h->frames.i_max_ref0 );
    h->i_ref0 = X264_MIN( h->i_ref0, h->param.i_frame_reference ); // if reconfig() has lowered the limit

    /* add duplicates */
    if( h->fenc->i_type == X264_TYPE_P )
    {
        if( h->param.analyse.i_weighted_pred == X264_WEIGHTP_SMART )
        {
            x264_weight_t w[3];
            w[1].weightfn = w[2].weightfn = NULL;
            if( h->param.rc.b_stat_read )
                x264_ratecontrol_set_weights( h, h->fenc );
            else if( h->param.i_threads == 1 )
                x264_weights_analyse( h, h->fenc, h->fref0[0], 0, 0 );

            if( !h->fenc->weight[0][0].weightfn )
            {
                h->fenc->weight[0][0].i_denom = 0;
                SET_WEIGHT( w[0], 1, 1, 0, -1 );
                x264_weighted_reference_duplicate( h, 0, w );
            }
            else
            {
                if( h->fenc->weight[0][0].i_scale == 1<<h->fenc->weight[0][0].i_denom )
                {
                    SET_WEIGHT( h->fenc->weight[0][0], 1, 1, 0, h->fenc->weight[0][0].i_offset );
                }
                x264_weighted_reference_duplicate( h, 0, weight_none );
                w[0] = h->fenc->weight[0][0];
                w[0].i_offset--;
                h->mc.weight_cache( h, &w[0] );
                x264_weighted_reference_duplicate( h, 0, w );
            }
        }
        else if( h->param.analyse.i_weighted_pred == X264_WEIGHTP_BLIND )
        {
            //weighted offset=-1
            x264_weight_t w[3];
            SET_WEIGHT( w[0], 1, 1, 0, -1 );
            h->fenc->weight[0][0].i_denom = 0;
            w[1].weightfn = w[2].weightfn = NULL;
            x264_weighted_reference_duplicate( h, 0, w );
        }
    }

    assert( h->i_ref0 + h->i_ref1 <= 16 );
    h->mb.pic.i_fref[0] = h->i_ref0;
    h->mb.pic.i_fref[1] = h->i_ref1;
}

static void x264_fdec_filter_row( x264_t *h, int mb_y )
{
    /* mb_y is the mb to be encoded next, not the mb to be filtered here */
    int b_hpel = h->fdec->b_kept_as_ref;
    int b_deblock = !h->sh.i_disable_deblocking_filter_idc;
    int b_end = mb_y == h->sps->i_mb_height;
    int min_y = mb_y - (1 << h->sh.b_mbaff);
    int max_y = b_end ? h->sps->i_mb_height : mb_y;
    b_deblock &= b_hpel || h->param.psz_dump_yuv;
    if( mb_y & h->sh.b_mbaff )
        return;
    if( min_y < 0 )
        return;

    if( !b_end )
    {
        int i, j;
        for( j=0; j<=h->sh.b_mbaff; j++ )
            for( i=0; i<3; i++ )
            {
                memcpy( h->mb.intra_border_backup[j][i],
                        h->fdec->plane[i] + ((mb_y*16 >> !!i) + j - 1 - h->sh.b_mbaff) * h->fdec->i_stride[i],
                        h->sps->i_mb_width*16 >> !!i );
            }
    }

    if( b_deblock )
    {
        int y;
        for( y = min_y; y < max_y; y += (1 << h->sh.b_mbaff) )
            x264_frame_deblock_row( h, y );
    }

    if( b_hpel )
    {
        x264_frame_expand_border( h, h->fdec, min_y, b_end );
        if( h->param.analyse.i_subpel_refine )
        {
            x264_frame_filter( h, h->fdec, min_y, b_end );
            x264_frame_expand_border_filtered( h, h->fdec, min_y, b_end );
        }
    }

    if( h->param.i_threads > 1 && h->fdec->b_kept_as_ref )
    {
        x264_frame_cond_broadcast( h->fdec, mb_y*16 + (b_end ? 10000 : -(X264_THREAD_HEIGHT << h->sh.b_mbaff)) );
    }

    min_y = X264_MAX( min_y*16-8, 0 );
    max_y = b_end ? h->param.i_height : mb_y*16-8;

    if( h->param.analyse.b_psnr )
    {
        int i;
        for( i=0; i<3; i++ )
            h->stat.frame.i_ssd[i] +=
                x264_pixel_ssd_wxh( &h->pixf,
                    h->fdec->plane[i] + (min_y>>!!i) * h->fdec->i_stride[i], h->fdec->i_stride[i],
                    h->fenc->plane[i] + (min_y>>!!i) * h->fenc->i_stride[i], h->fenc->i_stride[i],
                    h->param.i_width >> !!i, (max_y-min_y) >> !!i );
    }

    if( h->param.analyse.b_ssim )
    {
        x264_emms();
        /* offset by 2 pixels to avoid alignment of ssim blocks with dct blocks,
         * and overlap by 4 */
        min_y += min_y == 0 ? 2 : -6;
        h->stat.frame.f_ssim +=
            x264_pixel_ssim_wxh( &h->pixf,
                h->fdec->plane[0] + 2+min_y*h->fdec->i_stride[0], h->fdec->i_stride[0],
                h->fenc->plane[0] + 2+min_y*h->fenc->i_stride[0], h->fenc->i_stride[0],
                h->param.i_width-2, max_y-min_y, h->scratch_buffer );
    }
}

static inline int x264_reference_update( x264_t *h )
{
    int i, j;
    if( !h->fdec->b_kept_as_ref )
    {
        if( h->param.i_threads > 1 )
        {
            x264_frame_push_unused( h, h->fdec );
            h->fdec = x264_frame_pop_unused( h, 1 );
            if( !h->fdec )
                return -1;
        }
        return 0;
    }

    /* apply mmco from previous frame. */
    for( i = 0; i < h->sh.i_mmco_command_count; i++ )
        for( j = 0; h->frames.reference[j]; j++ )
            if( h->frames.reference[j]->i_poc == h->sh.mmco[i].i_poc )
                x264_frame_push_unused( h, x264_frame_shift( &h->frames.reference[j] ) );

    /* move frame in the buffer */
    x264_frame_push( h->frames.reference, h->fdec );
    if( h->frames.reference[h->sps->i_num_ref_frames] )
        x264_frame_push_unused( h, x264_frame_shift( h->frames.reference ) );
    h->fdec = x264_frame_pop_unused( h, 1 );
    if( !h->fdec )
        return -1;
    return 0;
}

static inline void x264_reference_reset( x264_t *h )
{
    while( h->frames.reference[0] )
        x264_frame_push_unused( h, x264_frame_pop( h->frames.reference ) );
    h->fdec->i_poc =
    h->fenc->i_poc = 0;
}

static inline void x264_reference_hierarchy_reset( x264_t *h )
{
    int i, ref;
    int b_hasdelayframe = 0;
    if( !h->param.i_bframe_pyramid )
        return;

    /* look for delay frames -- chain must only contain frames that are disposable */
    for( i = 0; h->frames.current[i] && IS_DISPOSABLE( h->frames.current[i]->i_type ); i++ )
        b_hasdelayframe |= h->frames.current[i]->i_dts
                        != h->frames.current[i]->i_frame + h->sps->vui.i_num_reorder_frames;

    if( h->param.i_bframe_pyramid != X264_B_PYRAMID_STRICT && !b_hasdelayframe )
        return;

    /* Remove last BREF. There will never be old BREFs in the
     * dpb during a BREF decode when pyramid == STRICT */
    for( ref = 0; h->frames.reference[ref]; ref++ )
    {
        if( h->param.i_bframe_pyramid == X264_B_PYRAMID_STRICT
            && h->frames.reference[ref]->i_type == X264_TYPE_BREF )
        {
            int diff = h->i_frame_num - h->frames.reference[ref]->i_frame_num;
            h->sh.mmco[h->sh.i_mmco_command_count].i_difference_of_pic_nums = diff;
            h->sh.mmco[h->sh.i_mmco_command_count++].i_poc = h->frames.reference[ref]->i_poc;
            x264_frame_push_unused( h, x264_frame_pop( h->frames.reference ) );
            h->b_ref_reorder[0] = 1;
            break;
        }
    }

    /* Prepare to room in the dpb for the delayed display time of the later b-frame's */
    h->sh.i_mmco_remove_from_end = X264_MAX( ref + 2 - h->frames.i_max_dpb, 0 );
}

static inline void x264_slice_init( x264_t *h, int i_nal_type, int i_global_qp )
{
    /* ------------------------ Create slice header  ----------------------- */
    if( i_nal_type == NAL_SLICE_IDR )
    {
        x264_slice_header_init( h, &h->sh, h->sps, h->pps, h->i_idr_pic_id, h->i_frame_num, i_global_qp );

        /* increment id */
        h->i_idr_pic_id = ( h->i_idr_pic_id + 1 ) % 65536;
    }
    else
    {
        x264_slice_header_init( h, &h->sh, h->sps, h->pps, -1, h->i_frame_num, i_global_qp );

        /* always set the real higher num of ref frame used */
        h->sh.b_num_ref_idx_override = 1;
        h->sh.i_num_ref_idx_l0_active = h->i_ref0 <= 0 ? 1 : h->i_ref0;
        h->sh.i_num_ref_idx_l1_active = h->i_ref1 <= 0 ? 1 : h->i_ref1;
    }

    h->fdec->i_frame_num = h->sh.i_frame_num;

    if( h->sps->i_poc_type == 0 )
    {
        h->sh.i_poc_lsb = h->fdec->i_poc & ( (1 << h->sps->i_log2_max_poc_lsb) - 1 );
        h->sh.i_delta_poc_bottom = 0;   /* XXX won't work for field */
    }
    else if( h->sps->i_poc_type == 1 )
    {
        /* FIXME TODO FIXME */
    }
    else
    {
        /* Nothing to do ? */
    }

    x264_macroblock_slice_init( h );
}

static int x264_slice_write( x264_t *h )
{
    int i_skip;
    int mb_xy, i_mb_x, i_mb_y;
    int i, i_list, i_ref, i_skip_bak = 0; /* Shut up GCC. */
    bs_t bs_bak;
    x264_cabac_t cabac_bak;
    uint8_t cabac_prevbyte_bak = 0; /* Shut up GCC. */
    /* Assume no more than 3 bytes of NALU escaping. */
    int slice_max_size = h->param.i_slice_max_size > 0 ? (h->param.i_slice_max_size-3-NALU_OVERHEAD)*8 : INT_MAX;
    int starting_bits = bs_pos(&h->out.bs);

    /* Slice */
    x264_nal_start( h, h->i_nal_type, h->i_nal_ref_idc );

    /* Slice header */
    x264_slice_header_write( &h->out.bs, &h->sh, h->i_nal_ref_idc );
    if( h->param.b_cabac )
    {
        /* alignment needed */
        bs_align_1( &h->out.bs );

        /* init cabac */
        x264_cabac_context_init( &h->cabac, h->sh.i_type, h->sh.i_qp, h->sh.i_cabac_init_idc );
        x264_cabac_encode_init ( &h->cabac, h->out.bs.p, h->out.bs.p_end );
    }
    h->mb.i_last_qp = h->sh.i_qp;
    h->mb.i_last_dqp = 0;

    i_mb_y = h->sh.i_first_mb / h->sps->i_mb_width;
    i_mb_x = h->sh.i_first_mb % h->sps->i_mb_width;
    i_skip = 0;

    while( (mb_xy = i_mb_x + i_mb_y * h->sps->i_mb_width) <= h->sh.i_last_mb )
    {
        int mb_spos = bs_pos(&h->out.bs) + x264_cabac_pos(&h->cabac);
        if( h->param.i_slice_max_size > 0 )
        {
            /* We don't need the contexts because flushing the CABAC encoder has no context
             * dependency and macroblocks are only re-encoded in the case where a slice is
             * ended (and thus the content of all contexts are thrown away). */
            if( h->param.b_cabac )
            {
                memcpy( &cabac_bak, &h->cabac, offsetof(x264_cabac_t, f8_bits_encoded) );
                /* x264's CABAC writer modifies the previous byte during carry, so it has to be
                 * backed up. */
                cabac_prevbyte_bak = h->cabac.p[-1];
            }
            else
            {
                bs_bak = h->out.bs;
                i_skip_bak = i_skip;
            }
        }

        if( i_mb_x == 0 && !h->mb.b_reencode_mb )
            x264_fdec_filter_row( h, i_mb_y );

        /* load cache */
        x264_macroblock_cache_load( h, i_mb_x, i_mb_y );

        x264_macroblock_analyse( h );

        /* encode this macroblock -> be careful it can change the mb type to P_SKIP if needed */
        x264_macroblock_encode( h );

        if( x264_bitstream_check_buffer( h ) )
            return -1;

        if( h->param.b_cabac )
        {
            if( mb_xy > h->sh.i_first_mb && !(h->sh.b_mbaff && (i_mb_y&1)) )
                x264_cabac_encode_terminal( &h->cabac );

            if( IS_SKIP( h->mb.i_type ) )
                x264_cabac_mb_skip( h, 1 );
            else
            {
                if( h->sh.i_type != SLICE_TYPE_I )
                    x264_cabac_mb_skip( h, 0 );
                x264_macroblock_write_cabac( h, &h->cabac );
            }
        }
        else
        {
            if( IS_SKIP( h->mb.i_type ) )
                i_skip++;
            else
            {
                if( h->sh.i_type != SLICE_TYPE_I )
                {
                    bs_write_ue( &h->out.bs, i_skip );  /* skip run */
                    i_skip = 0;
                }
                x264_macroblock_write_cavlc( h, &h->out.bs );
            }
        }

        int total_bits = bs_pos(&h->out.bs) + x264_cabac_pos(&h->cabac);
        int mb_size = total_bits - mb_spos;

        /* We'll just re-encode this last macroblock if we go over the max slice size. */
        if( total_bits - starting_bits > slice_max_size && !h->mb.b_reencode_mb )
        {
            if( mb_xy != h->sh.i_first_mb )
            {
                if( h->param.b_cabac )
                {
                    memcpy( &h->cabac, &cabac_bak, offsetof(x264_cabac_t, f8_bits_encoded) );
                    h->cabac.p[-1] = cabac_prevbyte_bak;
                }
                else
                {
                    h->out.bs = bs_bak;
                    i_skip = i_skip_bak;
                }
                h->mb.b_reencode_mb = 1;
                h->sh.i_last_mb = mb_xy-1;
                break;
            }
            else
            {
                h->sh.i_last_mb = mb_xy;
                h->mb.b_reencode_mb = 0;
            }
        }
        else
            h->mb.b_reencode_mb = 0;

#if VISUALIZE
        if( h->param.b_visualize )
            x264_visualize_mb( h );
#endif

        /* save cache */
        x264_macroblock_cache_save( h );

        /* accumulate mb stats */
        h->stat.frame.i_mb_count[h->mb.i_type]++;

        if( !IS_INTRA(h->mb.i_type) && !IS_SKIP(h->mb.i_type) && !IS_DIRECT(h->mb.i_type) )
        {
            if( h->mb.i_partition != D_8x8 )
                    h->stat.frame.i_mb_partition[h->mb.i_partition] += 4;
                else
                    for( i = 0; i < 4; i++ )
                        h->stat.frame.i_mb_partition[h->mb.i_sub_partition[i]] ++;
            if( h->param.i_frame_reference > 1 )
                for( i_list = 0; i_list <= (h->sh.i_type == SLICE_TYPE_B); i_list++ )
                    for( i = 0; i < 4; i++ )
                    {
                        i_ref = h->mb.cache.ref[i_list][ x264_scan8[4*i] ];
                        if( i_ref >= 0 )
                            h->stat.frame.i_mb_count_ref[i_list][i_ref] ++;
                    }
        }

        if( h->param.i_log_level >= X264_LOG_INFO )
        {
            if( h->mb.i_cbp_luma || h->mb.i_cbp_chroma )
            {
                int cbpsum = (h->mb.i_cbp_luma&1) + ((h->mb.i_cbp_luma>>1)&1)
                           + ((h->mb.i_cbp_luma>>2)&1) + (h->mb.i_cbp_luma>>3);
                int b_intra = IS_INTRA(h->mb.i_type);
                h->stat.frame.i_mb_cbp[!b_intra + 0] += cbpsum;
                h->stat.frame.i_mb_cbp[!b_intra + 2] += h->mb.i_cbp_chroma >= 1;
                h->stat.frame.i_mb_cbp[!b_intra + 4] += h->mb.i_cbp_chroma == 2;
            }
            if( h->mb.i_cbp_luma && !IS_INTRA(h->mb.i_type) )
            {
                h->stat.frame.i_mb_count_8x8dct[0] ++;
                h->stat.frame.i_mb_count_8x8dct[1] += h->mb.b_transform_8x8;
            }
            if( IS_INTRA(h->mb.i_type) && h->mb.i_type != I_PCM )
            {
                if( h->mb.i_type == I_16x16 )
                    h->stat.frame.i_mb_pred_mode[0][h->mb.i_intra16x16_pred_mode]++;
                else if( h->mb.i_type == I_8x8 )
                    for( i = 0; i < 16; i += 4 )
                        h->stat.frame.i_mb_pred_mode[1][h->mb.cache.intra4x4_pred_mode[x264_scan8[i]]]++;
                else //if( h->mb.i_type == I_4x4 )
                    for( i = 0; i < 16; i++ )
                        h->stat.frame.i_mb_pred_mode[2][h->mb.cache.intra4x4_pred_mode[x264_scan8[i]]]++;
            }
        }

        x264_ratecontrol_mb( h, mb_size );

        if( h->sh.b_mbaff )
        {
            i_mb_x += i_mb_y & 1;
            i_mb_y ^= i_mb_x < h->sps->i_mb_width;
        }
        else
            i_mb_x++;
        if( i_mb_x == h->sps->i_mb_width )
        {
            i_mb_y++;
            i_mb_x = 0;
        }
    }

    if( h->param.b_cabac )
    {
        x264_cabac_encode_flush( h, &h->cabac );
        h->out.bs.p = h->cabac.p;
    }
    else
    {
        if( i_skip > 0 )
            bs_write_ue( &h->out.bs, i_skip );  /* last skip run */
        /* rbsp_slice_trailing_bits */
        bs_rbsp_trailing( &h->out.bs );
        bs_flush( &h->out.bs );
    }
    if( x264_nal_end( h ) )
        return -1;

    if( h->sh.i_last_mb == h->mb.i_mb_count-1 )
    {
        h->stat.frame.i_misc_bits = bs_pos( &h->out.bs )
                                  + (h->out.i_nal*NALU_OVERHEAD * 8)
                                  - h->stat.frame.i_tex_bits
                                  - h->stat.frame.i_mv_bits;
        x264_fdec_filter_row( h, h->sps->i_mb_height );
    }

    return 0;
}

static void x264_thread_sync_context( x264_t *dst, x264_t *src )
{
    x264_frame_t **f;
    if( dst == src )
        return;

    // reference counting
    for( f = src->frames.reference; *f; f++ )
        (*f)->i_reference_count++;
    for( f = dst->frames.reference; *f; f++ )
        x264_frame_push_unused( src, *f );
    src->fdec->i_reference_count++;
    x264_frame_push_unused( src, dst->fdec );

    // copy everything except the per-thread pointers and the constants.
    memcpy( &dst->i_frame, &src->i_frame, offsetof(x264_t, mb.type) - offsetof(x264_t, i_frame) );
    dst->param = src->param;
    dst->stat = src->stat;
}

static void x264_thread_sync_stat( x264_t *dst, x264_t *src )
{
    if( dst == src )
        return;
    memcpy( &dst->stat.i_frame_count, &src->stat.i_frame_count, sizeof(dst->stat) - sizeof(dst->stat.frame) );
}

static void *x264_slices_write( x264_t *h )
{
    int i_slice_num = 0;
    if( h->param.i_sync_lookahead )
        x264_lower_thread_priority( 10 );

#ifdef HAVE_MMX
    /* Misalign mask has to be set separately for each thread. */
    if( h->param.cpu&X264_CPU_SSE_MISALIGN )
        x264_cpu_mask_misalign_sse();
#endif

#if VISUALIZE
    if( h->param.b_visualize )
        if( x264_visualize_init( h ) )
            return (void *)-1;
#endif

    /* init stats */
    memset( &h->stat.frame, 0, sizeof(h->stat.frame) );
    h->mb.b_reencode_mb = 0;
    while( h->sh.i_first_mb < h->mb.i_mb_count )
    {
        h->sh.i_last_mb = h->mb.i_mb_count - 1;
        if( h->param.i_slice_max_mbs )
            h->sh.i_last_mb = h->sh.i_first_mb + h->param.i_slice_max_mbs - 1;
        else if( h->param.i_slice_count )
        {
            x264_emms();
            i_slice_num++;
            double height = h->sps->i_mb_height >> h->param.b_interlaced;
            int width = h->sps->i_mb_width << h->param.b_interlaced;
            h->sh.i_last_mb = (int)(height * i_slice_num / h->param.i_slice_count + 0.5) * width - 1;
        }
        h->sh.i_last_mb = X264_MIN( h->sh.i_last_mb, h->mb.i_mb_count - 1 );
        if( x264_stack_align( x264_slice_write, h ) )
            return (void *)-1;
        h->sh.i_first_mb = h->sh.i_last_mb + 1;
    }

#if VISUALIZE
    if( h->param.b_visualize )
    {
        x264_visualize_show( h );
        x264_visualize_close( h );
    }
#endif

    return (void *)0;
}

/****************************************************************************
 * x264_encoder_encode:
 *  XXX: i_poc   : is the poc of the current given picture
 *       i_frame : is the number of the frame being coded
 *  ex:  type frame poc
 *       I      0   2*0
 *       P      1   2*3
 *       B      2   2*1
 *       B      3   2*2
 *       P      4   2*6
 *       B      5   2*4
 *       B      6   2*5
 ****************************************************************************/
int     x264_encoder_encode( x264_t *h,
                             x264_nal_t **pp_nal, int *pi_nal,
                             x264_picture_t *pic_in,
                             x264_picture_t *pic_out )
{
    x264_t *thread_current, *thread_prev, *thread_oldest;
    int     i_nal_type;
    int     i_nal_ref_idc;

    int   i_global_qp;

    if( h->param.i_threads > 1)
    {
        thread_prev    = h->thread[ h->i_thread_phase ];
        h->i_thread_phase = (h->i_thread_phase + 1) % h->param.i_threads;
        thread_current = h->thread[ h->i_thread_phase ];
        thread_oldest  = h->thread[ (h->i_thread_phase + 1) % h->param.i_threads ];
        x264_thread_sync_context( thread_current, thread_prev );
        x264_thread_sync_ratecontrol( thread_current, thread_prev, thread_oldest );
        h = thread_current;
//      fprintf(stderr, "current: %p  prev: %p  oldest: %p \n", thread_current, thread_prev, thread_oldest);
    }
    else
    {
        thread_current =
        thread_oldest  = h;
    }

    // ok to call this before encoding any frames, since the initial values of fdec have b_kept_as_ref=0
    if( x264_reference_update( h ) )
        return -1;
    h->fdec->i_lines_completed = -1;

    /* no data out */
    *pi_nal = 0;
    *pp_nal = NULL;

    /* ------------------- Setup new frame from picture -------------------- */
    if( pic_in != NULL )
    {
        /* 1: Copy the picture to a frame and move it to a buffer */
        x264_frame_t *fenc = x264_frame_pop_unused( h, 0 );
        if( !fenc )
            return -1;

        if( x264_frame_copy_picture( h, fenc, pic_in ) < 0 )
            return -1;

        if( h->param.i_width != 16 * h->sps->i_mb_width ||
            h->param.i_height != 16 * h->sps->i_mb_height )
            x264_frame_expand_border_mod16( h, fenc );

        fenc->i_frame = h->frames.i_input++;

        if( h->frames.b_have_lowres )
            x264_frame_init_lowres( h, fenc );

        if( h->param.rc.b_mb_tree && h->param.rc.b_stat_read )
        {
            if( x264_macroblock_tree_read( h, fenc ) )
                return -1;
        }
        else if( h->param.rc.i_aq_mode )
            x264_adaptive_quant_frame( h, fenc );

        /* 2: Place the frame into the queue for its slice type decision */
        x264_lookahead_put_frame( h, fenc );

        if( h->frames.i_input <= h->frames.i_delay + 1 - h->param.i_threads )
        {
            /* Nothing yet to encode, waiting for filling of buffers */
            pic_out->i_type = X264_TYPE_AUTO;
            return 0;
        }
    }
    else
    {
        /* signal kills for lookahead thread */
        h->lookahead->b_exit_thread = 1;
        x264_pthread_cond_broadcast( &h->lookahead->ifbuf.cv_fill );
    }

    h->i_frame++;
    /* 3: The picture is analyzed in the lookahead */
    if( !h->frames.current[0] )
        x264_lookahead_get_frames( h );

    if( !h->frames.current[0] && x264_lookahead_is_empty( h ) )
        return x264_encoder_frame_end( thread_oldest, thread_current, pp_nal, pi_nal, pic_out );

    /* ------------------- Get frame to be encoded ------------------------- */
    /* 4: get picture to encode */
    h->fenc = x264_frame_shift( h->frames.current );
    if( h->fenc->param )
    {
        x264_encoder_reconfig( h, h->fenc->param );
        if( h->fenc->param->param_free )
            h->fenc->param->param_free( h->fenc->param );
    }

    if( h->fenc->i_type == X264_TYPE_IDR )
    {
        h->frames.i_last_idr = h->fenc->i_frame;
        h->i_frame_num = 0;
    }
    h->sh.i_mmco_command_count = 0;
    h->sh.i_mmco_remove_from_end = 0;
    h->b_ref_reorder[0] =
    h->b_ref_reorder[1] = 0;

    /* ------------------- Setup frame context ----------------------------- */
    /* 5: Init data dependent of frame type */
    if( h->fenc->i_type == X264_TYPE_IDR )
    {
        /* reset ref pictures */
        i_nal_type    = NAL_SLICE_IDR;
        i_nal_ref_idc = NAL_PRIORITY_HIGHEST;
        h->sh.i_type = SLICE_TYPE_I;
        x264_reference_reset( h );
    }
    else if( h->fenc->i_type == X264_TYPE_I )
    {
        i_nal_type    = NAL_SLICE;
        i_nal_ref_idc = NAL_PRIORITY_HIGH; /* Not completely true but for now it is (as all I/P are kept as ref)*/
        h->sh.i_type = SLICE_TYPE_I;
        x264_reference_hierarchy_reset( h );
    }
    else if( h->fenc->i_type == X264_TYPE_P )
    {
        i_nal_type    = NAL_SLICE;
        i_nal_ref_idc = NAL_PRIORITY_HIGH; /* Not completely true but for now it is (as all I/P are kept as ref)*/
        h->sh.i_type = SLICE_TYPE_P;
        x264_reference_hierarchy_reset( h );
    }
    else if( h->fenc->i_type == X264_TYPE_BREF )
    {
        i_nal_type    = NAL_SLICE;
        i_nal_ref_idc = h->param.i_bframe_pyramid == X264_B_PYRAMID_STRICT ? NAL_PRIORITY_LOW : NAL_PRIORITY_HIGH;
        h->sh.i_type = SLICE_TYPE_B;
        x264_reference_hierarchy_reset( h );
    }
    else    /* B frame */
    {
        i_nal_type    = NAL_SLICE;
        i_nal_ref_idc = NAL_PRIORITY_DISPOSABLE;
        h->sh.i_type = SLICE_TYPE_B;
    }

    h->fdec->i_poc =
    h->fenc->i_poc = 2 * (h->fenc->i_frame - h->frames.i_last_idr);
    h->fdec->i_type = h->fenc->i_type;
    h->fdec->i_frame = h->fenc->i_frame;
    h->fenc->b_kept_as_ref =
    h->fdec->b_kept_as_ref = i_nal_ref_idc != NAL_PRIORITY_DISPOSABLE && h->param.i_keyint_max > 1;



    /* ------------------- Init                ----------------------------- */
    /* build ref list 0/1 */
    x264_reference_build_list( h, h->fdec->i_poc );

    /* ---------------------- Write the bitstream -------------------------- */
    /* Init bitstream context */
    h->out.i_nal = 0;
    bs_init( &h->out.bs, h->out.p_bitstream, h->out.i_bitstream );

    if( h->param.b_aud )
    {
        int pic_type;

        if( h->sh.i_type == SLICE_TYPE_I )
            pic_type = 0;
        else if( h->sh.i_type == SLICE_TYPE_P )
            pic_type = 1;
        else if( h->sh.i_type == SLICE_TYPE_B )
            pic_type = 2;
        else
            pic_type = 7;

        x264_nal_start( h, NAL_AUD, NAL_PRIORITY_DISPOSABLE );
        bs_write( &h->out.bs, 3, pic_type );
        bs_rbsp_trailing( &h->out.bs );
        if( x264_nal_end( h ) )
            return -1;
    }

    h->i_nal_type = i_nal_type;
    h->i_nal_ref_idc = i_nal_ref_idc;

    int overhead = NALU_OVERHEAD;

    /* Write SPS and PPS */
    if( i_nal_type == NAL_SLICE_IDR && h->param.b_repeat_headers )
    {
        if( h->fenc->i_frame == 0 )
        {
            /* identify ourself */
            x264_nal_start( h, NAL_SEI, NAL_PRIORITY_DISPOSABLE );
            if( x264_sei_version_write( h, &h->out.bs ) )
                return -1;
            if( x264_nal_end( h ) )
                return -1;
            overhead += h->out.nal[h->out.i_nal-1].i_payload + NALU_OVERHEAD;
        }

        /* generate sequence parameters */
        x264_nal_start( h, NAL_SPS, NAL_PRIORITY_HIGHEST );
        x264_sps_write( &h->out.bs, h->sps );
        if( x264_nal_end( h ) )
            return -1;
        overhead += h->out.nal[h->out.i_nal-1].i_payload + NALU_OVERHEAD;

        /* generate picture parameters */
        x264_nal_start( h, NAL_PPS, NAL_PRIORITY_HIGHEST );
        x264_pps_write( &h->out.bs, h->pps );
        if( x264_nal_end( h ) )
            return -1;
        overhead += h->out.nal[h->out.i_nal-1].i_payload + NALU_OVERHEAD;
    }

    /* Init the rate control */
    /* FIXME: Include slice header bit cost. */
    x264_ratecontrol_start( h, h->fenc->i_qpplus1, overhead*8 );
    i_global_qp = x264_ratecontrol_qp( h );

    pic_out->i_qpplus1 =
    h->fdec->i_qpplus1 = i_global_qp + 1;

    if( h->param.rc.b_stat_read && h->sh.i_type != SLICE_TYPE_I )
    {
        x264_reference_build_list_optimal( h );
        x264_reference_check_reorder( h );
    }

    if( h->sh.i_type == SLICE_TYPE_B )
        x264_macroblock_bipred_init( h );

    /*------------------------- Weights -------------------------------------*/
    x264_weighted_pred_init( h );

    /* ------------------------ Create slice header  ----------------------- */
    x264_slice_init( h, i_nal_type, i_global_qp );

    if( i_nal_ref_idc != NAL_PRIORITY_DISPOSABLE )
        h->i_frame_num++;

    /* Write frame */
    if( h->param.i_threads > 1 )
    {
        if( x264_pthread_create( &h->thread_handle, NULL, (void*)x264_slices_write, h ) )
            return -1;
        h->b_thread_active = 1;
    }
    else
        if( (intptr_t)x264_slices_write( h ) )
            return -1;

    return x264_encoder_frame_end( thread_oldest, thread_current, pp_nal, pi_nal, pic_out );
}

static int x264_encoder_frame_end( x264_t *h, x264_t *thread_current,
                                   x264_nal_t **pp_nal, int *pi_nal,
                                   x264_picture_t *pic_out )
{
    int i, j, i_list, frame_size;
    char psz_message[80];

    if( h->b_thread_active )
    {
        void *ret = NULL;
        x264_pthread_join( h->thread_handle, &ret );
        if( (intptr_t)ret )
            return (intptr_t)ret;
        h->b_thread_active = 0;
    }
    if( !h->out.i_nal )
    {
        pic_out->i_type = X264_TYPE_AUTO;
        return 0;
    }

    x264_frame_push_unused( thread_current, h->fenc );

    /* End bitstream, set output  */
    *pi_nal = h->out.i_nal;
    *pp_nal = h->out.nal;

    frame_size = x264_encoder_encapsulate_nals( h );

    h->out.i_nal = 0;

    /* Set output picture properties */
    if( h->sh.i_type == SLICE_TYPE_I )
        pic_out->i_type = h->i_nal_type == NAL_SLICE_IDR ? X264_TYPE_IDR : X264_TYPE_I;
    else if( h->sh.i_type == SLICE_TYPE_P )
        pic_out->i_type = X264_TYPE_P;
    else
        pic_out->i_type = X264_TYPE_B;
    pic_out->i_pts = h->fenc->i_pts;

    pic_out->img.i_plane = h->fdec->i_plane;
    for(i = 0; i < 3; i++)
    {
        pic_out->img.i_stride[i] = h->fdec->i_stride[i];
        pic_out->img.plane[i] = h->fdec->plane[i];
    }

    /* ---------------------- Update encoder state ------------------------- */

    /* update rc */
    x264_emms();
    if( x264_ratecontrol_end( h, frame_size * 8 ) < 0 )
        return -1;

    x264_noise_reduction_update( thread_current );

    /* ---------------------- Compute/Print statistics --------------------- */
    x264_thread_sync_stat( h, h->thread[0] );

    /* Slice stat */
    h->stat.i_frame_count[h->sh.i_type]++;
    h->stat.i_frame_size[h->sh.i_type] += frame_size;
    h->stat.f_frame_qp[h->sh.i_type] += h->fdec->f_qp_avg_aq;

    for( i = 0; i < X264_MBTYPE_MAX; i++ )
        h->stat.i_mb_count[h->sh.i_type][i] += h->stat.frame.i_mb_count[i];
    for( i = 0; i < X264_PARTTYPE_MAX; i++ )
        h->stat.i_mb_partition[h->sh.i_type][i] += h->stat.frame.i_mb_partition[i];
    for( i = 0; i < 2; i++ )
        h->stat.i_mb_count_8x8dct[i] += h->stat.frame.i_mb_count_8x8dct[i];
    for( i = 0; i < 6; i++ )
        h->stat.i_mb_cbp[i] += h->stat.frame.i_mb_cbp[i];
    for( i = 0; i < 3; i++ )
        for( j = 0; j < 13; j++ )
            h->stat.i_mb_pred_mode[i][j] += h->stat.frame.i_mb_pred_mode[i][j];
    if( h->sh.i_type != SLICE_TYPE_I )
        for( i_list = 0; i_list < 2; i_list++ )
            for( i = 0; i < 32; i++ )
                h->stat.i_mb_count_ref[h->sh.i_type][i_list][i] += h->stat.frame.i_mb_count_ref[i_list][i];
    if( h->sh.i_type == SLICE_TYPE_P )
    {
        h->stat.i_consecutive_bframes[h->fdec->i_frame - h->fref0[0]->i_frame - 1]++;
        if( h->param.analyse.i_weighted_pred == X264_WEIGHTP_SMART )
        {
            for( i = 0; i < 3; i++ )
                for( j = 0; j < h->i_ref0; j++ )
                    if( h->sh.weight[0][i].i_denom != 0 )
                    {
                        h->stat.i_wpred[i]++;
                        break;
                    }
        }
    }
    if( h->sh.i_type == SLICE_TYPE_B )
    {
        h->stat.i_direct_frames[ h->sh.b_direct_spatial_mv_pred ] ++;
        if( h->mb.b_direct_auto_write )
        {
            //FIXME somewhat arbitrary time constants
            if( h->stat.i_direct_score[0] + h->stat.i_direct_score[1] > h->mb.i_mb_count )
            {
                for( i = 0; i < 2; i++ )
                    h->stat.i_direct_score[i] = h->stat.i_direct_score[i] * 9/10;
            }
            for( i = 0; i < 2; i++ )
                h->stat.i_direct_score[i] += h->stat.frame.i_direct_score[i];
        }
    }

    psz_message[0] = '\0';
    if( h->param.analyse.b_psnr )
    {
        int64_t ssd[3] = {
            h->stat.frame.i_ssd[0],
            h->stat.frame.i_ssd[1],
            h->stat.frame.i_ssd[2],
        };

        h->stat.i_ssd_global[h->sh.i_type] += ssd[0] + ssd[1] + ssd[2];
        h->stat.f_psnr_average[h->sh.i_type] += x264_psnr( ssd[0] + ssd[1] + ssd[2], 3 * h->param.i_width * h->param.i_height / 2 );
        h->stat.f_psnr_mean_y[h->sh.i_type] += x264_psnr( ssd[0], h->param.i_width * h->param.i_height );
        h->stat.f_psnr_mean_u[h->sh.i_type] += x264_psnr( ssd[1], h->param.i_width * h->param.i_height / 4 );
        h->stat.f_psnr_mean_v[h->sh.i_type] += x264_psnr( ssd[2], h->param.i_width * h->param.i_height / 4 );

        snprintf( psz_message, 80, " PSNR Y:%5.2f U:%5.2f V:%5.2f",
                  x264_psnr( ssd[0], h->param.i_width * h->param.i_height ),
                  x264_psnr( ssd[1], h->param.i_width * h->param.i_height / 4),
                  x264_psnr( ssd[2], h->param.i_width * h->param.i_height / 4) );
    }

    if( h->param.analyse.b_ssim )
    {
        double ssim_y = h->stat.frame.f_ssim
                      / (((h->param.i_width-6)>>2) * ((h->param.i_height-6)>>2));
        h->stat.f_ssim_mean_y[h->sh.i_type] += ssim_y;
        snprintf( psz_message + strlen(psz_message), 80 - strlen(psz_message),
                  " SSIM Y:%.5f", ssim_y );
    }
    psz_message[79] = '\0';

    x264_log( h, X264_LOG_DEBUG,
                  "frame=%4d QP=%.2f NAL=%d Slice:%c Poc:%-3d I:%-4d P:%-4d SKIP:%-4d size=%d bytes%s\n",
              h->i_frame,
              h->fdec->f_qp_avg_aq,
              h->i_nal_ref_idc,
              h->sh.i_type == SLICE_TYPE_I ? 'I' : (h->sh.i_type == SLICE_TYPE_P ? 'P' : 'B' ),
              h->fdec->i_poc,
              h->stat.frame.i_mb_count_i,
              h->stat.frame.i_mb_count_p,
              h->stat.frame.i_mb_count_skip,
              frame_size,
              psz_message );

    // keep stats all in one place
    x264_thread_sync_stat( h->thread[0], h );
    // for the use of the next frame
    x264_thread_sync_stat( thread_current, h );

#ifdef DEBUG_MB_TYPE
{
    static const char mb_chars[] = { 'i', 'i', 'I', 'C', 'P', '8', 'S',
        'D', '<', 'X', 'B', 'X', '>', 'B', 'B', 'B', 'B', '8', 'S' };
    int mb_xy;
    for( mb_xy = 0; mb_xy < h->sps->i_mb_width * h->sps->i_mb_height; mb_xy++ )
    {
        if( h->mb.type[mb_xy] < X264_MBTYPE_MAX && h->mb.type[mb_xy] >= 0 )
            fprintf( stderr, "%c ", mb_chars[ h->mb.type[mb_xy] ] );
        else
            fprintf( stderr, "? " );

        if( (mb_xy+1) % h->sps->i_mb_width == 0 )
            fprintf( stderr, "\n" );
    }
}
#endif

    /* Remove duplicates, must be done near the end as breaks h->fref0 array
     * by freeing some of its pointers. */
     for( i = 0; i < h->i_ref0; i++ )
         if( h->fref0[i] && h->fref0[i]->b_duplicate )
         {
             x264_frame_push_blank_unused( h, h->fref0[i] );
             h->fref0[i] = 0;
         }

    if( h->param.psz_dump_yuv )
        x264_frame_dump( h );

    return frame_size;
}

static void x264_print_intra( int64_t *i_mb_count, double i_count, int b_print_pcm, char *intra )
{
    intra += sprintf( intra, "I16..4%s: %4.1f%% %4.1f%% %4.1f%%",
        b_print_pcm ? "..PCM" : "",
        i_mb_count[I_16x16]/ i_count,
        i_mb_count[I_8x8]  / i_count,
        i_mb_count[I_4x4]  / i_count );
    if( b_print_pcm )
        sprintf( intra, " %4.1f%%", i_mb_count[I_PCM]  / i_count );
}

/****************************************************************************
 * x264_encoder_close:
 ****************************************************************************/
void    x264_encoder_close  ( x264_t *h )
{
    int64_t i_yuv_size = 3 * h->param.i_width * h->param.i_height / 2;
    int64_t i_mb_count_size[2][7] = {{0}};
    char buf[200];
    int i, j, i_list, i_type;
    int b_print_pcm = h->stat.i_mb_count[SLICE_TYPE_I][I_PCM]
                   || h->stat.i_mb_count[SLICE_TYPE_P][I_PCM]
                   || h->stat.i_mb_count[SLICE_TYPE_B][I_PCM];

    x264_lookahead_delete( h );

    for( i=0; i<h->param.i_threads; i++ )
    {
        // don't strictly have to wait for the other threads, but it's simpler than canceling them
        if( h->thread[i]->b_thread_active )
        {
            x264_pthread_join( h->thread[i]->thread_handle, NULL );
            assert( h->thread[i]->fenc->i_reference_count == 1 );
            x264_frame_delete( h->thread[i]->fenc );
        }
    }

    if( h->param.i_threads > 1 )
    {
        x264_t *thread_prev;

        thread_prev = h->thread[h->i_thread_phase];
        x264_thread_sync_ratecontrol( h, thread_prev, h );
        x264_thread_sync_ratecontrol( thread_prev, thread_prev, h );
        h->i_frame = thread_prev->i_frame + 1 - h->param.i_threads;
    }
    h->i_frame++;

    /* Slices used and PSNR */
    for( i=0; i<5; i++ )
    {
        static const int slice_order[] = { SLICE_TYPE_I, SLICE_TYPE_SI, SLICE_TYPE_P, SLICE_TYPE_SP, SLICE_TYPE_B };
        static const char *slice_name[] = { "P", "B", "I", "SP", "SI" };
        int i_slice = slice_order[i];

        if( h->stat.i_frame_count[i_slice] > 0 )
        {
            const int i_count = h->stat.i_frame_count[i_slice];
            if( h->param.analyse.b_psnr )
            {
                x264_log( h, X264_LOG_INFO,
                          "frame %s:%-5d Avg QP:%5.2f  size:%6.0f  PSNR Mean Y:%5.2f U:%5.2f V:%5.2f Avg:%5.2f Global:%5.2f\n",
                          slice_name[i_slice],
                          i_count,
                          h->stat.f_frame_qp[i_slice] / i_count,
                          (double)h->stat.i_frame_size[i_slice] / i_count,
                          h->stat.f_psnr_mean_y[i_slice] / i_count, h->stat.f_psnr_mean_u[i_slice] / i_count, h->stat.f_psnr_mean_v[i_slice] / i_count,
                          h->stat.f_psnr_average[i_slice] / i_count,
                          x264_psnr( h->stat.i_ssd_global[i_slice], i_count * i_yuv_size ) );
            }
            else
            {
                x264_log( h, X264_LOG_INFO,
                          "frame %s:%-5d Avg QP:%5.2f  size:%6.0f\n",
                          slice_name[i_slice],
                          i_count,
                          h->stat.f_frame_qp[i_slice] / i_count,
                          (double)h->stat.i_frame_size[i_slice] / i_count );
            }
        }
    }
    if( h->param.i_bframe && h->stat.i_frame_count[SLICE_TYPE_P] )
    {
        char *p = buf;
        int den = 0;
        // weight by number of frames (including the P-frame) that are in a sequence of N B-frames
        for( i=0; i<=h->param.i_bframe; i++ )
            den += (i+1) * h->stat.i_consecutive_bframes[i];
        for( i=0; i<=h->param.i_bframe; i++ )
            p += sprintf( p, " %4.1f%%", 100. * (i+1) * h->stat.i_consecutive_bframes[i] / den );
        x264_log( h, X264_LOG_INFO, "consecutive B-frames:%s\n", buf );
    }

    for( i_type = 0; i_type < 2; i_type++ )
        for( i = 0; i < X264_PARTTYPE_MAX; i++ )
        {
            if( i == D_DIRECT_8x8 ) continue; /* direct is counted as its own type */
            i_mb_count_size[i_type][x264_mb_partition_pixel_table[i]] += h->stat.i_mb_partition[i_type][i];
        }

    /* MB types used */
    if( h->stat.i_frame_count[SLICE_TYPE_I] > 0 )
    {
        int64_t *i_mb_count = h->stat.i_mb_count[SLICE_TYPE_I];
        double i_count = h->stat.i_frame_count[SLICE_TYPE_I] * h->mb.i_mb_count / 100.0;
        x264_print_intra( i_mb_count, i_count, b_print_pcm, buf );
        x264_log( h, X264_LOG_INFO, "mb I  %s\n", buf );
    }
    if( h->stat.i_frame_count[SLICE_TYPE_P] > 0 )
    {
        int64_t *i_mb_count = h->stat.i_mb_count[SLICE_TYPE_P];
        double i_count = h->stat.i_frame_count[SLICE_TYPE_P] * h->mb.i_mb_count / 100.0;
        int64_t *i_mb_size = i_mb_count_size[SLICE_TYPE_P];
        x264_print_intra( i_mb_count, i_count, b_print_pcm, buf );
        x264_log( h, X264_LOG_INFO,
                  "mb P  %s  P16..4: %4.1f%% %4.1f%% %4.1f%% %4.1f%% %4.1f%%    skip:%4.1f%%\n",
                  buf,
                  i_mb_size[PIXEL_16x16] / (i_count*4),
                  (i_mb_size[PIXEL_16x8] + i_mb_size[PIXEL_8x16]) / (i_count*4),
                  i_mb_size[PIXEL_8x8] / (i_count*4),
                  (i_mb_size[PIXEL_8x4] + i_mb_size[PIXEL_4x8]) / (i_count*4),
                  i_mb_size[PIXEL_4x4] / (i_count*4),
                  i_mb_count[P_SKIP] / i_count );
    }
    if( h->stat.i_frame_count[SLICE_TYPE_B] > 0 )
    {
        int64_t *i_mb_count = h->stat.i_mb_count[SLICE_TYPE_B];
        double i_count = h->stat.i_frame_count[SLICE_TYPE_B] * h->mb.i_mb_count / 100.0;
        double i_mb_list_count;
        int64_t *i_mb_size = i_mb_count_size[SLICE_TYPE_B];
        int64_t list_count[3] = {0}; /* 0 == L0, 1 == L1, 2 == BI */
        x264_print_intra( i_mb_count, i_count, b_print_pcm, buf );
        for( i = 0; i < X264_PARTTYPE_MAX; i++ )
            for( j = 0; j < 2; j++ )
            {
                int l0 = x264_mb_type_list_table[i][0][j];
                int l1 = x264_mb_type_list_table[i][1][j];
                if( l0 || l1 )
                    list_count[l1+l0*l1] += h->stat.i_mb_count[SLICE_TYPE_B][i] * 2;
            }
        list_count[0] += h->stat.i_mb_partition[SLICE_TYPE_B][D_L0_8x8];
        list_count[1] += h->stat.i_mb_partition[SLICE_TYPE_B][D_L1_8x8];
        list_count[2] += h->stat.i_mb_partition[SLICE_TYPE_B][D_BI_8x8];
        i_mb_count[B_DIRECT] += (h->stat.i_mb_partition[SLICE_TYPE_B][D_DIRECT_8x8]+2)/4;
        i_mb_list_count = (list_count[0] + list_count[1] + list_count[2]) / 100.0;
        x264_log( h, X264_LOG_INFO,
                  "mb B  %s  B16..8: %4.1f%% %4.1f%% %4.1f%%  direct:%4.1f%%  skip:%4.1f%%  L0:%4.1f%% L1:%4.1f%% BI:%4.1f%%\n",
                  buf,
                  i_mb_size[PIXEL_16x16] / (i_count*4),
                  (i_mb_size[PIXEL_16x8] + i_mb_size[PIXEL_8x16]) / (i_count*4),
                  i_mb_size[PIXEL_8x8] / (i_count*4),
                  i_mb_count[B_DIRECT] / i_count,
                  i_mb_count[B_SKIP]   / i_count,
                  list_count[0] / i_mb_list_count,
                  list_count[1] / i_mb_list_count,
                  list_count[2] / i_mb_list_count );
    }

    x264_ratecontrol_summary( h );

    if( h->stat.i_frame_count[SLICE_TYPE_I] + h->stat.i_frame_count[SLICE_TYPE_P] + h->stat.i_frame_count[SLICE_TYPE_B] > 0 )
    {
#define SUM3(p) (p[SLICE_TYPE_I] + p[SLICE_TYPE_P] + p[SLICE_TYPE_B])
#define SUM3b(p,o) (p[SLICE_TYPE_I][o] + p[SLICE_TYPE_P][o] + p[SLICE_TYPE_B][o])
        int64_t i_i8x8 = SUM3b( h->stat.i_mb_count, I_8x8 );
        int64_t i_intra = i_i8x8 + SUM3b( h->stat.i_mb_count, I_4x4 )
                                 + SUM3b( h->stat.i_mb_count, I_16x16 );
        int64_t i_all_intra = i_intra + SUM3b( h->stat.i_mb_count, I_PCM);
        const int i_count = h->stat.i_frame_count[SLICE_TYPE_I] +
                            h->stat.i_frame_count[SLICE_TYPE_P] +
                            h->stat.i_frame_count[SLICE_TYPE_B];
        int64_t i_mb_count = i_count * h->mb.i_mb_count;
        float fps = (float) h->param.i_fps_num / h->param.i_fps_den;
        float f_bitrate = fps * SUM3(h->stat.i_frame_size) / i_count / 125;

        if( h->pps->b_transform_8x8_mode )
        {
            buf[0] = 0;
            if( h->stat.i_mb_count_8x8dct[0] )
                sprintf( buf, " inter:%.1f%%", 100. * h->stat.i_mb_count_8x8dct[1] / h->stat.i_mb_count_8x8dct[0] );
            x264_log( h, X264_LOG_INFO, "8x8 transform intra:%.1f%%%s\n", 100. * i_i8x8 / i_intra, buf );
        }

        if( h->param.analyse.i_direct_mv_pred == X264_DIRECT_PRED_AUTO
            && h->stat.i_frame_count[SLICE_TYPE_B] )
        {
            x264_log( h, X264_LOG_INFO, "direct mvs  spatial:%.1f%% temporal:%.1f%%\n",
                      h->stat.i_direct_frames[1] * 100. / h->stat.i_frame_count[SLICE_TYPE_B],
                      h->stat.i_direct_frames[0] * 100. / h->stat.i_frame_count[SLICE_TYPE_B] );
        }

        buf[0] = 0;
        if( i_mb_count != i_all_intra )
            sprintf( buf, " inter: %.1f%% %.1f%% %.1f%%",
                     h->stat.i_mb_cbp[1] * 100.0 / ((i_mb_count - i_all_intra)*4),
                     h->stat.i_mb_cbp[3] * 100.0 / ((i_mb_count - i_all_intra)  ),
                     h->stat.i_mb_cbp[5] * 100.0 / ((i_mb_count - i_all_intra)) );
        x264_log( h, X264_LOG_INFO, "coded y,uvDC,uvAC intra: %.1f%% %.1f%% %.1f%%%s\n",
                  h->stat.i_mb_cbp[0] * 100.0 / (i_all_intra*4),
                  h->stat.i_mb_cbp[2] * 100.0 / (i_all_intra  ),
                  h->stat.i_mb_cbp[4] * 100.0 / (i_all_intra  ), buf );

        int64_t fixed_pred_modes[3][9] = {{0}};
        int64_t sum_pred_modes[3] = {0};
        for( i = 0; i <= I_PRED_16x16_DC_128; i++ )
        {
            fixed_pred_modes[0][x264_mb_pred_mode16x16_fix[i]] += h->stat.i_mb_pred_mode[0][i];
            sum_pred_modes[0] += h->stat.i_mb_pred_mode[0][i];
        }
        if( sum_pred_modes[0] )
            x264_log( h, X264_LOG_INFO, "i16 v,h,dc,p: %2.0f%% %2.0f%% %2.0f%% %2.0f%%\n",
                      fixed_pred_modes[0][0] * 100.0 / sum_pred_modes[0],
                      fixed_pred_modes[0][1] * 100.0 / sum_pred_modes[0],
                      fixed_pred_modes[0][2] * 100.0 / sum_pred_modes[0],
                      fixed_pred_modes[0][3] * 100.0 / sum_pred_modes[0] );
        for( i = 1; i <= 2; i++ )
        {
            for( j = 0; j <= I_PRED_8x8_DC_128; j++ )
            {
                fixed_pred_modes[i][x264_mb_pred_mode4x4_fix(j)] += h->stat.i_mb_pred_mode[i][j];
                sum_pred_modes[i] += h->stat.i_mb_pred_mode[i][j];
            }
            if( sum_pred_modes[i] )
                x264_log( h, X264_LOG_INFO, "i%d v,h,dc,ddl,ddr,vr,hd,vl,hu: %2.0f%% %2.0f%% %2.0f%% %2.0f%% %2.0f%% %2.0f%% %2.0f%% %2.0f%% %2.0f%%\n", (3-i)*4,
                          fixed_pred_modes[i][0] * 100.0 / sum_pred_modes[i],
                          fixed_pred_modes[i][1] * 100.0 / sum_pred_modes[i],
                          fixed_pred_modes[i][2] * 100.0 / sum_pred_modes[i],
                          fixed_pred_modes[i][3] * 100.0 / sum_pred_modes[i],
                          fixed_pred_modes[i][4] * 100.0 / sum_pred_modes[i],
                          fixed_pred_modes[i][5] * 100.0 / sum_pred_modes[i],
                          fixed_pred_modes[i][6] * 100.0 / sum_pred_modes[i],
                          fixed_pred_modes[i][7] * 100.0 / sum_pred_modes[i],
                          fixed_pred_modes[i][8] * 100.0 / sum_pred_modes[i] );
        }

        if( h->param.analyse.i_weighted_pred == X264_WEIGHTP_SMART )
            x264_log( h, X264_LOG_INFO, "Weighted P-Frames: Y:%.1f%%\n",
                      h->stat.i_wpred[0] * 100.0 / h->stat.i_frame_count[SLICE_TYPE_P] );

        for( i_list = 0; i_list < 2; i_list++ )
        {
            int i_slice;
            for( i_slice = 0; i_slice < 2; i_slice++ )
            {
                char *p = buf;
                int64_t i_den = 0;
                int i_max = 0;
                for( i = 0; i < 32; i++ )
                    if( h->stat.i_mb_count_ref[i_slice][i_list][i] )
                    {
                        i_den += h->stat.i_mb_count_ref[i_slice][i_list][i];
                        i_max = i;
                    }
                if( i_max == 0 )
                    continue;
                for( i = 0; i <= i_max; i++ )
                    p += sprintf( p, " %4.1f%%", 100. * h->stat.i_mb_count_ref[i_slice][i_list][i] / i_den );
                x264_log( h, X264_LOG_INFO, "ref %c L%d:%s\n", "PB"[i_slice], i_list, buf );
            }
        }

        if( h->param.analyse.b_ssim )
        {
            x264_log( h, X264_LOG_INFO,
                      "SSIM Mean Y:%.7f\n",
                      SUM3( h->stat.f_ssim_mean_y ) / i_count );
        }
        if( h->param.analyse.b_psnr )
        {
            x264_log( h, X264_LOG_INFO,
                      "PSNR Mean Y:%6.3f U:%6.3f V:%6.3f Avg:%6.3f Global:%6.3f kb/s:%.2f\n",
                      SUM3( h->stat.f_psnr_mean_y ) / i_count,
                      SUM3( h->stat.f_psnr_mean_u ) / i_count,
                      SUM3( h->stat.f_psnr_mean_v ) / i_count,
                      SUM3( h->stat.f_psnr_average ) / i_count,
                      x264_psnr( SUM3( h->stat.i_ssd_global ), i_count * i_yuv_size ),
                      f_bitrate );
        }
        else
            x264_log( h, X264_LOG_INFO, "kb/s:%.2f\n", f_bitrate );
    }

    /* rc */
    x264_ratecontrol_delete( h );

    /* param */
    if( h->param.rc.psz_stat_out )
        free( h->param.rc.psz_stat_out );
    if( h->param.rc.psz_stat_in )
        free( h->param.rc.psz_stat_in );

    x264_cqm_delete( h );
    x264_free( h->nal_buffer );
    x264_analyse_free_costs( h );

    if( h->param.i_threads > 1)
        h = h->thread[h->i_thread_phase];

    /* frames */
    x264_frame_delete_list( h->frames.unused[0] );
    x264_frame_delete_list( h->frames.unused[1] );
    x264_frame_delete_list( h->frames.current );
    x264_frame_delete_list( h->frames.blank_unused );

    h = h->thread[0];

    for( i = h->param.i_threads - 1; i >= 0; i-- )
    {
        x264_frame_t **frame;

        for( frame = h->thread[i]->frames.reference; *frame; frame++ )
        {
            assert( (*frame)->i_reference_count > 0 );
            (*frame)->i_reference_count--;
            if( (*frame)->i_reference_count == 0 )
                x264_frame_delete( *frame );
        }
        frame = &h->thread[i]->fdec;
        assert( (*frame)->i_reference_count > 0 );
        (*frame)->i_reference_count--;
        if( (*frame)->i_reference_count == 0 )
            x264_frame_delete( *frame );

        x264_macroblock_cache_end( h->thread[i] );
        x264_free( h->thread[i]->out.p_bitstream );
        x264_free( h->thread[i]->out.nal);
        x264_free( h->thread[i] );
    }
}

/****************************************************************************
 * x264_encoder_delayed_frames:
 ****************************************************************************/
int x264_encoder_delayed_frames( x264_t *h )
{
    int delayed_frames = 0;
    int i;
    for( i=0; i<h->param.i_threads; i++ )
        delayed_frames += h->thread[i]->b_thread_active;
    h = h->thread[h->i_thread_phase];
    for( i=0; h->frames.current[i]; i++ )
        delayed_frames++;
    x264_pthread_mutex_lock( &h->lookahead->ofbuf.mutex );
    x264_pthread_mutex_lock( &h->lookahead->ifbuf.mutex );
    x264_pthread_mutex_lock( &h->lookahead->next.mutex );
    delayed_frames += h->lookahead->ifbuf.i_size + h->lookahead->next.i_size + h->lookahead->ofbuf.i_size;
    x264_pthread_mutex_unlock( &h->lookahead->next.mutex );
    x264_pthread_mutex_unlock( &h->lookahead->ifbuf.mutex );
    x264_pthread_mutex_unlock( &h->lookahead->ofbuf.mutex );
    return delayed_frames;
}

/*****************************************************************************
 * x264: h264 encoder
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar
 * $Id: encoder.c,v 1.1 2004/06/03 19:27:08 fenrir Exp $
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

#include <stdio.h>
#include <string.h>
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
//#define DEBUG_DUMP_FRAME
//#define DEBUG_BENCHMARK

#ifdef DEBUG_BENCHMARK
static int64_t i_mtime_encode_frame = 0;
static int64_t i_mtime_analyse = 0;
static int64_t i_mtime_encode = 0;
static int64_t i_mtime_write = 0;
static int64_t i_mtime_filter = 0;
#define TIMER_START( d ) \
    { \
        int64_t d##start = x264_mdate();

#define TIMER_STOP( d ) \
        d += x264_mdate() - d##start;\
    }
#else
#define TIMER_START( d )
#define TIMER_STOP( d )
#endif

#define NALU_OVERHEAD 5 // startcode + NAL type costs 5 bytes per frame

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

#ifdef DEBUG_DUMP_FRAME
static void x264_frame_dump( x264_t *h, x264_frame_t *fr, char *name )
{
    FILE *f = fopen( name, "r+b" );
    int i, y;
    if( !f )
        return;

    /* Write the frame in display order */
    fseek( f, fr->i_frame * h->param.i_height * h->param.i_width * 3 / 2, SEEK_SET );

    for( i = 0; i < fr->i_plane; i++ )
    {
        for( y = 0; y < h->param.i_height / ( i == 0 ? 1 : 2 ); y++ )
        {
            fwrite( &fr->plane[i][y*fr->i_stride[i]], 1, h->param.i_width / ( i == 0 ? 1 : 2 ), f );
        }
    }
    fclose( f );
}
#endif


/* Fill "default" values */
static void x264_slice_header_init( x264_t *h, x264_slice_header_t *sh,
                                    x264_sps_t *sps, x264_pps_t *pps,
                                    int i_type, int i_idr_pic_id, int i_frame, int i_qp )
{
    x264_param_t *param = &h->param;
    int i;

    /* First we fill all field */
    sh->sps = sps;
    sh->pps = pps;

    sh->i_type      = i_type;
    sh->i_first_mb  = 0;
    sh->i_last_mb   = h->sps->i_mb_width * h->sps->i_mb_height;
    sh->i_pps_id    = pps->i_id;

    sh->i_frame_num = i_frame;

    sh->b_field_pic = 0;    /* Not field support for now */
    sh->b_bottom_field = 1; /* not yet used */

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
        || 15 < i_qp + 2 * X264_MAX(param->i_deblocking_filter_alphac0, param->i_deblocking_filter_beta) ) )
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

    bs_write_ue( s, sh->i_first_mb );
    bs_write_ue( s, sh->i_type + 5 );   /* same type things */
    bs_write_ue( s, sh->i_pps_id );
    bs_write( s, sh->sps->i_log2_max_frame_num, sh->i_frame_num );

    if( sh->i_idr_pic_id >= 0 ) /* NAL IDR */
    {
        bs_write_ue( s, sh->i_idr_pic_id );
    }

    if( sh->sps->i_poc_type == 0 )
    {
        bs_write( s, sh->sps->i_log2_max_poc_lsb, sh->i_poc_lsb );
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

    if( ( sh->pps->b_weighted_pred && ( sh->i_type == SLICE_TYPE_P || sh->i_type == SLICE_TYPE_SP ) ) ||
        ( sh->pps->b_weighted_bipred == 1 && sh->i_type == SLICE_TYPE_B ) )
    {
        /* FIXME */
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
            bs_write1( s, 0 );  /* adaptive_ref_pic_marking_mode_flag */
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

/****************************************************************************
 *
 ****************************************************************************
 ****************************** External API*********************************
 ****************************************************************************
 *
 ****************************************************************************/

static int x264_validate_parameters( x264_t *h )
{
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

    h->param.i_threads = x264_clip3( h->param.i_threads, 1, X264_SLICE_MAX );
    h->param.i_threads = X264_MIN( h->param.i_threads, (h->param.i_height + 15) / 16 );
#ifndef HAVE_PTHREAD
    if( h->param.i_threads > 1 )
    {
        x264_log( h, X264_LOG_WARNING, "not compiled with pthread support!\n");
        x264_log( h, X264_LOG_WARNING, "multislicing anyway, but you won't see any speed gain.\n" );
    }
#endif

    if( h->param.rc.b_cbr )
        h->param.rc.i_rf_constant = 0;
    if( h->param.rc.i_rf_constant > 0 )
        h->param.rc.i_qp_constant = h->param.rc.i_rf_constant;
    h->param.rc.i_rf_constant = x264_clip3( h->param.rc.i_rf_constant, 0, 51 );
    h->param.rc.i_qp_constant = x264_clip3( h->param.rc.i_qp_constant, 0, 51 );
    if( !h->param.rc.b_cbr && h->param.rc.i_qp_constant == 0 )
    {
        h->mb.b_lossless = 1;
        h->param.analyse.b_transform_8x8 = 0;
        h->param.i_cqm_preset = X264_CQM_FLAT;
        h->param.psz_cqm_file = NULL;
        h->param.rc.f_ip_factor = 1;
        h->param.rc.f_pb_factor = 1;
        h->param.analyse.b_psnr = 0;
        h->param.analyse.i_chroma_qp_offset = 0;
        h->param.analyse.i_trellis = 0;
        h->param.analyse.b_fast_pskip = 0;
        h->param.analyse.i_noise_reduction = 0;
        h->param.analyse.i_subpel_refine = x264_clip3( h->param.analyse.i_subpel_refine, 1, 6 );
    }

    if( ( h->param.i_width % 16 || h->param.i_height % 16 ) && !h->mb.b_lossless )
    {
        x264_log( h, X264_LOG_WARNING, 
                  "width or height not divisible by 16 (%dx%d), compression will suffer.\n",
                  h->param.i_width, h->param.i_height );
    }

    h->param.i_frame_reference = x264_clip3( h->param.i_frame_reference, 1, 16 );
    if( h->param.i_keyint_max <= 0 )
        h->param.i_keyint_max = 1;
    h->param.i_keyint_min = x264_clip3( h->param.i_keyint_min, 1, h->param.i_keyint_max/2+1 );

    h->param.i_bframe = x264_clip3( h->param.i_bframe, 0, X264_BFRAME_MAX );
    h->param.i_bframe_bias = x264_clip3( h->param.i_bframe_bias, -90, 100 );
    h->param.b_bframe_pyramid = h->param.b_bframe_pyramid && h->param.i_bframe > 1;
    h->param.b_bframe_adaptive = h->param.b_bframe_adaptive && h->param.i_bframe > 0;
    h->param.analyse.b_weighted_bipred = h->param.analyse.b_weighted_bipred && h->param.i_bframe > 0;
    h->mb.b_direct_auto_write = h->param.analyse.i_direct_mv_pred == X264_DIRECT_PRED_AUTO
                                && h->param.i_bframe
                                && ( h->param.rc.b_stat_write || !h->param.rc.b_stat_read );

    h->param.i_deblocking_filter_alphac0 = x264_clip3( h->param.i_deblocking_filter_alphac0, -6, 6 );
    h->param.i_deblocking_filter_beta    = x264_clip3( h->param.i_deblocking_filter_beta, -6, 6 );

    h->param.i_cabac_init_idc = x264_clip3( h->param.i_cabac_init_idc, 0, 2 );

    if( h->param.i_cqm_preset < X264_CQM_FLAT || h->param.i_cqm_preset > X264_CQM_CUSTOM )
        h->param.i_cqm_preset = X264_CQM_FLAT;

    if( h->param.analyse.i_me_method < X264_ME_DIA ||
        h->param.analyse.i_me_method > X264_ME_ESA )
        h->param.analyse.i_me_method = X264_ME_HEX;
    if( h->param.analyse.i_me_range < 4 )
        h->param.analyse.i_me_range = 4;
    if( h->param.analyse.i_me_range > 16 && h->param.analyse.i_me_method <= X264_ME_HEX )
        h->param.analyse.i_me_range = 16;
    h->param.analyse.i_subpel_refine = x264_clip3( h->param.analyse.i_subpel_refine, 1, 7 );
    h->param.analyse.b_bframe_rdo = h->param.analyse.b_bframe_rdo && h->param.analyse.i_subpel_refine >= 6;
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
    h->param.analyse.i_noise_reduction = x264_clip3( h->param.analyse.i_noise_reduction, 0, 1<<16 );

    {
        const x264_level_t *l = x264_levels;
        while( l->level_idc != 0 && l->level_idc != h->param.i_level_idc )
            l++;
        if( l->level_idc == 0 )
        {
            x264_log( h, X264_LOG_ERROR, "invalid level_idc: %d\n", h->param.i_level_idc );
            return -1;
        }
        if( h->param.analyse.i_mv_range <= 0 )
            h->param.analyse.i_mv_range = l->mv_range;
        else
            h->param.analyse.i_mv_range = x264_clip3(h->param.analyse.i_mv_range, 32, 2048);
    }

    if( h->param.rc.f_qblur < 0 )
        h->param.rc.f_qblur = 0;
    if( h->param.rc.f_complexity_blur < 0 )
        h->param.rc.f_complexity_blur = 0;

    h->param.i_sps_id &= 31;

    /* ensure the booleans are 0 or 1 so they can be used in math */
#define BOOLIFY(x) h->param.x = !!h->param.x
    BOOLIFY( b_cabac );
    BOOLIFY( b_deblocking_filter );
    BOOLIFY( analyse.b_transform_8x8 );
    BOOLIFY( analyse.b_bidir_me );
    BOOLIFY( analyse.b_chroma_me );
    BOOLIFY( analyse.b_fast_pskip );
    BOOLIFY( rc.b_cbr );
    BOOLIFY( rc.b_stat_write );
    BOOLIFY( rc.b_stat_read );
#undef BOOLIFY

    return 0;
}

/****************************************************************************
 * x264_encoder_open:
 ****************************************************************************/
x264_t *x264_encoder_open   ( x264_param_t *param )
{
    x264_t *h = x264_malloc( sizeof( x264_t ) );
    int i;

    memset( h, 0, sizeof( x264_t ) );

    /* Create a copy of param */
    memcpy( &h->param, param, sizeof( x264_param_t ) );

    if( x264_validate_parameters( h ) < 0 )
    {
        x264_free( h );
        return NULL;
    }

    if( h->param.psz_cqm_file )
        if( x264_cqm_parse_file( h, h->param.psz_cqm_file ) < 0 )
        {
            x264_free( h );
            return NULL;
        }

    if( h->param.rc.psz_stat_out )
        h->param.rc.psz_stat_out = strdup( h->param.rc.psz_stat_out );
    if( h->param.rc.psz_stat_in )
        h->param.rc.psz_stat_in = strdup( h->param.rc.psz_stat_in );
    if( h->param.rc.psz_rc_eq )
        h->param.rc.psz_rc_eq = strdup( h->param.rc.psz_rc_eq );

    /* VUI */
    if( h->param.vui.i_sar_width > 0 && h->param.vui.i_sar_height > 0 )
    {
        int i_w = param->vui.i_sar_width;
        int i_h = param->vui.i_sar_height;

        x264_reduce_fraction( &i_w, &i_h );

        while( i_w > 65535 || i_h > 65535 )
        {
            i_w /= 2;
            i_h /= 2;
        }

        h->param.vui.i_sar_width = 0;
        h->param.vui.i_sar_height = 0;
        if( i_w == 0 || i_h == 0 )
        {
            x264_log( h, X264_LOG_WARNING, "cannot create valid sample aspect ratio\n" );
        }
        else
        {
            x264_log( h, X264_LOG_INFO, "using SAR=%d/%d\n", i_w, i_h );
            h->param.vui.i_sar_width = i_w;
            h->param.vui.i_sar_height = i_h;
        }
    }

    x264_reduce_fraction( &h->param.i_fps_num, &h->param.i_fps_den );

    /* Init x264_t */
    h->out.i_nal = 0;
    h->out.i_bitstream = X264_MAX( 1000000, h->param.i_width * h->param.i_height * 1.7
        * ( h->param.rc.b_cbr ? pow( 0.5, h->param.rc.i_qp_min )
          : pow( 0.5, h->param.rc.i_qp_constant ) * X264_MAX( 1, h->param.rc.f_ip_factor )));
    h->out.p_bitstream = x264_malloc( h->out.i_bitstream );

    h->i_frame = 0;
    h->i_frame_num = 0;
    h->i_idr_pic_id = 0;

    h->sps = &h->sps_array[0];
    x264_sps_init( h->sps, h->param.i_sps_id, &h->param );

    h->pps = &h->pps_array[0];
    x264_pps_init( h->pps, h->param.i_sps_id, &h->param, h->sps);

    x264_validate_levels( h );

    x264_cqm_init( h );
    
    h->mb.i_mb_count = h->sps->i_mb_width * h->sps->i_mb_height;

    /* Init frames. */
    h->frames.i_delay = h->param.i_bframe;
    h->frames.i_max_ref0 = h->param.i_frame_reference;
    h->frames.i_max_ref1 = h->sps->vui.i_num_reorder_frames;
    h->frames.i_max_dpb  = h->sps->vui.i_max_dec_frame_buffering + 1;
    h->frames.b_have_lowres = !h->param.rc.b_stat_read
        && ( h->param.rc.b_cbr || h->param.rc.i_rf_constant || h->param.b_bframe_adaptive );

    for( i = 0; i < X264_BFRAME_MAX + 3; i++ )
    {
        h->frames.current[i] = NULL;
        h->frames.next[i]    = NULL;
        h->frames.unused[i]  = NULL;
    }
    for( i = 0; i < 1 + h->frames.i_delay; i++ )
    {
        h->frames.unused[i] =  x264_frame_new( h );
    }
    for( i = 0; i < h->frames.i_max_dpb; i++ )
    {
        h->frames.reference[i] = x264_frame_new( h );
    }
    h->frames.reference[h->frames.i_max_dpb] = NULL;
    h->frames.i_last_idr = - h->param.i_keyint_max;
    h->frames.i_input    = 0;
    h->frames.last_nonb  = NULL;

    h->i_ref0 = 0;
    h->i_ref1 = 0;

    h->fdec = h->frames.reference[0];

    x264_macroblock_cache_init( h );
    x264_rdo_init( );

    /* init CPU functions */
    x264_predict_16x16_init( h->param.cpu, h->predict_16x16 );
    x264_predict_8x8c_init( h->param.cpu, h->predict_8x8c );
    x264_predict_8x8_init( h->param.cpu, h->predict_8x8 );
    x264_predict_4x4_init( h->param.cpu, h->predict_4x4 );

    x264_pixel_init( h->param.cpu, &h->pixf );
    x264_dct_init( h->param.cpu, &h->dctf );
    x264_mc_init( h->param.cpu, &h->mc );
    x264_csp_init( h->param.cpu, h->param.i_csp, &h->csp );
    x264_quant_init( h, h->param.cpu, &h->quantf );
    x264_deblock_init( h->param.cpu, &h->loopf );

    memcpy( h->pixf.mbcmp,
            ( h->mb.b_lossless || h->param.analyse.i_subpel_refine <= 1 ) ? h->pixf.sad : h->pixf.satd,
            sizeof(h->pixf.mbcmp) );

    /* rate control */
    if( x264_ratecontrol_new( h ) < 0 )
        return NULL;

    x264_log( h, X264_LOG_INFO, "using cpu capabilities %s%s%s%s%s%s\n",
             param->cpu&X264_CPU_MMX ? "MMX " : "",
             param->cpu&X264_CPU_MMXEXT ? "MMXEXT " : "",
             param->cpu&X264_CPU_SSE ? "SSE " : "",
             param->cpu&X264_CPU_SSE2 ? "SSE2 " : "",
             param->cpu&X264_CPU_3DNOW ? "3DNow! " : "",
             param->cpu&X264_CPU_ALTIVEC ? "Altivec " : "" );

    h->thread[0] = h;
    h->i_thread_num = 0;
    for( i = 1; i < param->i_threads; i++ )
        h->thread[i] = x264_malloc( sizeof(x264_t) );

#ifdef DEBUG_DUMP_FRAME
    {
        /* create or truncate the reconstructed video file */
        FILE *f = fopen( "fdec.yuv", "w" );
        if( f )
            fclose( f );
        else
        {
            x264_log( h, X264_LOG_ERROR, "can't write to fdec.yuv\n" );
            x264_free( h );
            return NULL;
        }
    }
#endif

    return h;
}

/****************************************************************************
 * x264_encoder_reconfig:
 ****************************************************************************/
int x264_encoder_reconfig( x264_t *h, x264_param_t *param )
{
    h->param.i_bframe_bias = param->i_bframe_bias;
    h->param.i_deblocking_filter_alphac0 = param->i_deblocking_filter_alphac0;
    h->param.i_deblocking_filter_beta    = param->i_deblocking_filter_beta;
    h->param.analyse.i_me_method = param->analyse.i_me_method;
    h->param.analyse.i_me_range = param->analyse.i_me_range;
    h->param.analyse.i_subpel_refine = param->analyse.i_subpel_refine;
    h->param.analyse.i_trellis = param->analyse.i_trellis;
    h->param.analyse.intra = param->analyse.intra;
    h->param.analyse.inter = param->analyse.inter;

    memcpy( h->pixf.mbcmp,
            ( h->mb.b_lossless || h->param.analyse.i_subpel_refine <= 1 ) ? h->pixf.sad : h->pixf.satd,
            sizeof(h->pixf.mbcmp) );

    return x264_validate_parameters( h );
}

/* internal usage */
static void x264_nal_start( x264_t *h, int i_type, int i_ref_idc )
{
    x264_nal_t *nal = &h->out.nal[h->out.i_nal];

    nal->i_ref_idc = i_ref_idc;
    nal->i_type    = i_type;

    bs_align_0( &h->out.bs );   /* not needed */

    nal->i_payload= 0;
    nal->p_payload= &h->out.p_bitstream[bs_pos( &h->out.bs) / 8];
}
static void x264_nal_end( x264_t *h )
{
    x264_nal_t *nal = &h->out.nal[h->out.i_nal];

    bs_align_0( &h->out.bs );   /* not needed */

    nal->i_payload = &h->out.p_bitstream[bs_pos( &h->out.bs)/8] - nal->p_payload;

    h->out.i_nal++;
}

/****************************************************************************
 * x264_encoder_headers:
 ****************************************************************************/
int x264_encoder_headers( x264_t *h, x264_nal_t **pp_nal, int *pi_nal )
{
    /* init bitstream context */
    h->out.i_nal = 0;
    bs_init( &h->out.bs, h->out.p_bitstream, h->out.i_bitstream );

    /* Put SPS and PPS */
    if( h->i_frame == 0 )
    {
        /* identify ourself */
        x264_nal_start( h, NAL_SEI, NAL_PRIORITY_DISPOSABLE );
        x264_sei_version_write( h, &h->out.bs );
        x264_nal_end( h );

        /* generate sequence parameters */
        x264_nal_start( h, NAL_SPS, NAL_PRIORITY_HIGHEST );
        x264_sps_write( &h->out.bs, h->sps );
        x264_nal_end( h );

        /* generate picture parameters */
        x264_nal_start( h, NAL_PPS, NAL_PRIORITY_HIGHEST );
        x264_pps_write( &h->out.bs, h->pps );
        x264_nal_end( h );
    }
    /* now set output*/
    *pi_nal = h->out.i_nal;
    *pp_nal = &h->out.nal[0];

    return 0;
}


static void x264_frame_put( x264_frame_t *list[X264_BFRAME_MAX], x264_frame_t *frame )
{
    int i = 0;
    while( list[i] ) i++;
    list[i] = frame;
}

static void x264_frame_push( x264_frame_t *list[X264_BFRAME_MAX], x264_frame_t *frame )
{
    int i = 0;
    while( list[i] ) i++;
    while( i-- )
        list[i+1] = list[i];
    list[0] = frame;
}

static x264_frame_t *x264_frame_get( x264_frame_t *list[X264_BFRAME_MAX+1] )
{
    x264_frame_t *frame = list[0];
    int i;
    for( i = 0; list[i]; i++ )
        list[i] = list[i+1];
    return frame;
}

static void x264_frame_sort( x264_frame_t *list[X264_BFRAME_MAX+1], int b_dts )
{
    int i, b_ok;
    do {
        b_ok = 1;
        for( i = 0; list[i+1]; i++ )
        {
            int dtype = list[i]->i_type - list[i+1]->i_type;
            int dtime = list[i]->i_frame - list[i+1]->i_frame;
            int swap = b_dts ? dtype > 0 || ( dtype == 0 && dtime > 0 )
                             : dtime > 0;
            if( swap )
            {
                XCHG( x264_frame_t*, list[i], list[i+1] );
                b_ok = 0;
            }
        }
    } while( !b_ok );
}
#define x264_frame_sort_dts(list) x264_frame_sort(list, 1)
#define x264_frame_sort_pts(list) x264_frame_sort(list, 0)

static inline void x264_reference_build_list( x264_t *h, int i_poc, int i_slice_type )
{
    int i;
    int b_ok;

    /* build ref list 0/1 */
    h->i_ref0 = 0;
    h->i_ref1 = 0;
    for( i = 1; i < h->frames.i_max_dpb; i++ )
    {
        if( h->frames.reference[i]->i_poc >= 0 )
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

    /* In the standard, a P-frame's ref list is sorted by frame_num.
     * We use POC, but check whether explicit reordering is needed */
    h->b_ref_reorder[0] =
    h->b_ref_reorder[1] = 0;
    if( i_slice_type == SLICE_TYPE_P )
    {
        for( i = 0; i < h->i_ref0 - 1; i++ )
            if( h->fref0[i]->i_frame_num < h->fref0[i+1]->i_frame_num )
            {
                h->b_ref_reorder[0] = 1;
                break;
            }
    }

    h->i_ref1 = X264_MIN( h->i_ref1, h->frames.i_max_ref1 );
    h->i_ref0 = X264_MIN( h->i_ref0, h->frames.i_max_ref0 );
    h->i_ref0 = X264_MIN( h->i_ref0, 16 - h->i_ref1 );
}

static inline void x264_fdec_deblock( x264_t *h )
{
    /* apply deblocking filter to the current decoded picture */
    if( !h->sh.i_disable_deblocking_filter_idc )
    {
        TIMER_START( i_mtime_filter );
        x264_frame_deblocking_filter( h, h->sh.i_type );
        TIMER_STOP( i_mtime_filter );
    }
}

static inline void x264_reference_update( x264_t *h )
{
    int i;

    x264_fdec_deblock( h );

    /* expand border */
    x264_frame_expand_border( h->fdec );

    /* create filtered images */
    x264_frame_filter( h->param.cpu, h->fdec );

    /* expand border of filtered images */
    x264_frame_expand_border_filtered( h->fdec );

    /* move lowres copy of the image to the ref frame */
    for( i = 0; i < 4; i++)
        XCHG( uint8_t*, h->fdec->lowres[i], h->fenc->lowres[i] );

    /* adaptive B decision needs a pointer, since it can't use the ref lists */
    if( h->sh.i_type != SLICE_TYPE_B )
        h->frames.last_nonb = h->fdec;

    /* move frame in the buffer */
    h->fdec = h->frames.reference[h->frames.i_max_dpb-1];
    for( i = h->frames.i_max_dpb-1; i > 0; i-- )
    {
        h->frames.reference[i] = h->frames.reference[i-1];
    }
    h->frames.reference[0] = h->fdec;
}

static inline void x264_reference_reset( x264_t *h )
{
    int i;

    /* reset ref pictures */
    for( i = 1; i < h->frames.i_max_dpb; i++ )
    {
        h->frames.reference[i]->i_poc = -1;
    }
    h->frames.reference[0]->i_poc = 0;
}

static inline void x264_slice_init( x264_t *h, int i_nal_type, int i_slice_type, int i_global_qp )
{
    /* ------------------------ Create slice header  ----------------------- */
    if( i_nal_type == NAL_SLICE_IDR )
    {
        x264_slice_header_init( h, &h->sh, h->sps, h->pps, i_slice_type, h->i_idr_pic_id, h->i_frame_num, i_global_qp );

        /* increment id */
        h->i_idr_pic_id = ( h->i_idr_pic_id + 1 ) % 65536;
    }
    else
    {
        x264_slice_header_init( h, &h->sh, h->sps, h->pps, i_slice_type, -1, h->i_frame_num, i_global_qp );

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
    int mb_xy;
    int i;

    /* init stats */
    memset( &h->stat.frame, 0, sizeof(h->stat.frame) );

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
        x264_cabac_encode_init ( &h->cabac, &h->out.bs );
    }
    h->mb.i_last_qp = h->sh.i_qp;
    h->mb.i_last_dqp = 0;

    for( mb_xy = h->sh.i_first_mb, i_skip = 0; mb_xy < h->sh.i_last_mb; mb_xy++ )
    {
        const int i_mb_y = mb_xy / h->sps->i_mb_width;
        const int i_mb_x = mb_xy % h->sps->i_mb_width;

        int mb_spos = bs_pos(&h->out.bs);

        /* load cache */
        x264_macroblock_cache_load( h, i_mb_x, i_mb_y );

        /* analyse parameters
         * Slice I: choose I_4x4 or I_16x16 mode
         * Slice P: choose between using P mode or intra (4x4 or 16x16)
         * */
        TIMER_START( i_mtime_analyse );
        x264_macroblock_analyse( h );
        TIMER_STOP( i_mtime_analyse );

        /* encode this macrobock -> be carefull it can change the mb type to P_SKIP if needed */
        TIMER_START( i_mtime_encode );
        x264_macroblock_encode( h );
        TIMER_STOP( i_mtime_encode );

        TIMER_START( i_mtime_write );
        if( h->param.b_cabac )
        {
            if( mb_xy > h->sh.i_first_mb )
                x264_cabac_encode_terminal( &h->cabac, 0 );

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
        TIMER_STOP( i_mtime_write );

#if VISUALIZE
        if( h->param.b_visualize )
            x264_visualize_mb( h );
#endif

        /* save cache */
        x264_macroblock_cache_save( h );

        /* accumulate mb stats */
        h->stat.frame.i_mb_count[h->mb.i_type]++;
        if( !IS_SKIP(h->mb.i_type) && !IS_INTRA(h->mb.i_type) && !IS_DIRECT(h->mb.i_type) )
        {
            if( h->mb.i_partition != D_8x8 )
                h->stat.frame.i_mb_count_size[ x264_mb_partition_pixel_table[ h->mb.i_partition ] ] += 4;
            else
                for( i = 0; i < 4; i++ )
                    h->stat.frame.i_mb_count_size[ x264_mb_partition_pixel_table[ h->mb.i_sub_partition[i] ] ] ++;
            if( h->param.i_frame_reference > 1 )
            {
                for( i = 0; i < 4; i++ )
                {
                    int i_ref = h->mb.cache.ref[0][ x264_scan8[4*i] ];
                    if( i_ref >= 0 )
                        h->stat.frame.i_mb_count_ref[i_ref] ++;
                }
            }
        }
        if( h->mb.i_cbp_luma && !IS_INTRA(h->mb.i_type) )
        {
            h->stat.frame.i_mb_count_8x8dct[0] ++;
            h->stat.frame.i_mb_count_8x8dct[1] += h->mb.b_transform_8x8;
        }

        if( h->mb.b_variable_qp )
            x264_ratecontrol_mb(h, bs_pos(&h->out.bs) - mb_spos);
    }

    if( h->param.b_cabac )
    {
        /* end of slice */
        x264_cabac_encode_terminal( &h->cabac, 1 );
    }
    else if( i_skip > 0 )
    {
        bs_write_ue( &h->out.bs, i_skip );  /* last skip run */
    }

    if( h->param.b_cabac )
    {
        x264_cabac_encode_flush( &h->cabac );

    }
    else
    {
        /* rbsp_slice_trailing_bits */
        bs_rbsp_trailing( &h->out.bs );
    }

    x264_nal_end( h );

    /* Compute misc bits */
    h->stat.frame.i_misc_bits = bs_pos( &h->out.bs )
                              + NALU_OVERHEAD * 8
                              - h->stat.frame.i_itex_bits
                              - h->stat.frame.i_ptex_bits
                              - h->stat.frame.i_hdr_bits;

    return 0;
}

static inline int x264_slices_write( x264_t *h )
{
    int i_frame_size;

#if VISUALIZE
    if( h->param.b_visualize )
        x264_visualize_init( h );
#endif

    if( h->param.i_threads == 1 )
    {
        x264_ratecontrol_threads_start( h );
        x264_slice_write( h );
        i_frame_size = h->out.nal[h->out.i_nal-1].i_payload;
    }
    else
    {
        int i_nal = h->out.i_nal;
        int i_bs_size = h->out.i_bitstream / h->param.i_threads;
        int i;
        /* duplicate contexts */
        for( i = 0; i < h->param.i_threads; i++ )
        {
            x264_t *t = h->thread[i];
            if( i > 0 )
            {
                memcpy( t, h, sizeof(x264_t) );
                t->out.p_bitstream += i*i_bs_size;
                bs_init( &t->out.bs, t->out.p_bitstream, i_bs_size );
                t->i_thread_num = i;
            }
            t->sh.i_first_mb = (i    * h->sps->i_mb_height / h->param.i_threads) * h->sps->i_mb_width;
            t->sh.i_last_mb = ((i+1) * h->sps->i_mb_height / h->param.i_threads) * h->sps->i_mb_width;
            t->out.i_nal = i_nal + i;
        }
        x264_ratecontrol_threads_start( h );

        /* dispatch */
#ifdef HAVE_PTHREAD
        {
            pthread_t handles[X264_SLICE_MAX];
            for( i = 0; i < h->param.i_threads; i++ )
                pthread_create( &handles[i], NULL, (void*)x264_slice_write, (void*)h->thread[i] );
            for( i = 0; i < h->param.i_threads; i++ )
                pthread_join( handles[i], NULL );
        }
#else
        for( i = 0; i < h->param.i_threads; i++ )
            x264_slice_write( h->thread[i] );
#endif

        /* merge contexts */
        i_frame_size = h->out.nal[i_nal].i_payload;
        for( i = 1; i < h->param.i_threads; i++ )
        {
            int j;
            x264_t *t = h->thread[i];
            h->out.nal[i_nal+i] = t->out.nal[i_nal+i];
            i_frame_size += t->out.nal[i_nal+i].i_payload;
            // all entries in stat.frame are ints
            for( j = 0; j < sizeof(h->stat.frame) / sizeof(int); j++ )
                ((int*)&h->stat.frame)[j] += ((int*)&t->stat.frame)[j];
        }
        h->out.i_nal = i_nal + h->param.i_threads;
    }

#if VISUALIZE
    if( h->param.b_visualize )
    {
        x264_visualize_show( h );
        x264_visualize_close( h );
    }
#endif

    return i_frame_size;
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
    x264_frame_t   *frame_psnr = h->fdec; /* just to keep the current decoded frame for psnr calculation */
    int     i_nal_type;
    int     i_nal_ref_idc;
    int     i_slice_type;
    int     i_frame_size;

    int i;

    int   i_global_qp;

    char psz_message[80];

    /* no data out */
    *pi_nal = 0;
    *pp_nal = NULL;


    /* ------------------- Setup new frame from picture -------------------- */
    TIMER_START( i_mtime_encode_frame );
    if( pic_in != NULL )
    {
        /* 1: Copy the picture to a frame and move it to a buffer */
        x264_frame_t *fenc = x264_frame_get( h->frames.unused );

        x264_frame_copy_picture( h, fenc, pic_in );

        if( h->param.i_width % 16 || h->param.i_height % 16 )
            x264_frame_expand_border_mod16( h, fenc );

        fenc->i_frame = h->frames.i_input++;

        x264_frame_put( h->frames.next, fenc );

        if( h->frames.b_have_lowres )
            x264_frame_init_lowres( h->param.cpu, fenc );

        if( h->frames.i_input <= h->frames.i_delay )
        {
            /* Nothing yet to encode */
            /* waiting for filling bframe buffer */
            pic_out->i_type = X264_TYPE_AUTO;
            return 0;
        }
    }

    if( h->frames.current[0] == NULL )
    {
        int bframes = 0;
        /* 2: Select frame types */
        if( h->frames.next[0] == NULL )
            return 0;

        x264_slicetype_decide( h );

        /* 3: move some B-frames and 1 non-B to encode queue */
        while( IS_X264_TYPE_B( h->frames.next[bframes]->i_type ) )
            bframes++;
        x264_frame_put( h->frames.current, x264_frame_get( &h->frames.next[bframes] ) );
        /* FIXME: when max B-frames > 3, BREF may no longer be centered after GOP closing */
        if( h->param.b_bframe_pyramid && bframes > 1 )
        {
            x264_frame_t *mid = x264_frame_get( &h->frames.next[bframes/2] );
            mid->i_type = X264_TYPE_BREF;
            x264_frame_put( h->frames.current, mid );
            bframes--;
        }
        while( bframes-- )
            x264_frame_put( h->frames.current, x264_frame_get( h->frames.next ) );
    }
    TIMER_STOP( i_mtime_encode_frame );

    /* ------------------- Get frame to be encoded ------------------------- */
    /* 4: get picture to encode */
    h->fenc = x264_frame_get( h->frames.current );
    if( h->fenc == NULL )
    {
        /* Nothing yet to encode (ex: waiting for I/P with B frames) */
        /* waiting for filling bframe buffer */
        pic_out->i_type = X264_TYPE_AUTO;
        return 0;
    }

do_encode:

    if( h->fenc->i_type == X264_TYPE_IDR )
    {
        h->frames.i_last_idr = h->fenc->i_frame;
    }

    /* ------------------- Setup frame context ----------------------------- */
    /* 5: Init data dependant of frame type */
    TIMER_START( i_mtime_encode_frame );
    if( h->fenc->i_type == X264_TYPE_IDR )
    {
        /* reset ref pictures */
        x264_reference_reset( h );

        i_nal_type    = NAL_SLICE_IDR;
        i_nal_ref_idc = NAL_PRIORITY_HIGHEST;
        i_slice_type = SLICE_TYPE_I;
    }
    else if( h->fenc->i_type == X264_TYPE_I )
    {
        i_nal_type    = NAL_SLICE;
        i_nal_ref_idc = NAL_PRIORITY_HIGH; /* Not completely true but for now it is (as all I/P are kept as ref)*/
        i_slice_type = SLICE_TYPE_I;
    }
    else if( h->fenc->i_type == X264_TYPE_P )
    {
        i_nal_type    = NAL_SLICE;
        i_nal_ref_idc = NAL_PRIORITY_HIGH; /* Not completely true but for now it is (as all I/P are kept as ref)*/
        i_slice_type = SLICE_TYPE_P;
    }
    else if( h->fenc->i_type == X264_TYPE_BREF )
    {
        i_nal_type    = NAL_SLICE;
        i_nal_ref_idc = NAL_PRIORITY_HIGH; /* maybe add MMCO to forget it? -> low */
        i_slice_type = SLICE_TYPE_B;
    }
    else    /* B frame */
    {
        i_nal_type    = NAL_SLICE;
        i_nal_ref_idc = NAL_PRIORITY_DISPOSABLE;
        i_slice_type = SLICE_TYPE_B;
    }

    h->fdec->i_poc =
    h->fenc->i_poc = 2 * (h->fenc->i_frame - h->frames.i_last_idr);
    h->fdec->i_type = h->fenc->i_type;
    h->fdec->i_frame = h->fenc->i_frame;
    h->fenc->b_kept_as_ref =
    h->fdec->b_kept_as_ref = i_nal_ref_idc != NAL_PRIORITY_DISPOSABLE;



    /* ------------------- Init                ----------------------------- */
    /* build ref list 0/1 */
    x264_reference_build_list( h, h->fdec->i_poc, i_slice_type );

    /* Init the rate control */
    x264_ratecontrol_start( h, i_slice_type, h->fenc->i_qpplus1 );
    i_global_qp = x264_ratecontrol_qp( h );

    pic_out->i_qpplus1 =
    h->fdec->i_qpplus1 = i_global_qp + 1;

    if( i_slice_type == SLICE_TYPE_B )
        x264_macroblock_bipred_init( h );

    /* ------------------------ Create slice header  ----------------------- */
    x264_slice_init( h, i_nal_type, i_slice_type, i_global_qp );

    if( h->fenc->b_kept_as_ref )
        h->i_frame_num++;

    /* ---------------------- Write the bitstream -------------------------- */
    /* Init bitstream context */
    h->out.i_nal = 0;
    bs_init( &h->out.bs, h->out.p_bitstream, h->out.i_bitstream );

    if(h->param.b_aud){
        int pic_type;

        if(i_slice_type == SLICE_TYPE_I)
            pic_type = 0;
        else if(i_slice_type == SLICE_TYPE_P)
            pic_type = 1;
        else if(i_slice_type == SLICE_TYPE_B)
            pic_type = 2;
        else
            pic_type = 7;

        x264_nal_start(h, NAL_AUD, NAL_PRIORITY_DISPOSABLE);
        bs_write(&h->out.bs, 3, pic_type);
        bs_rbsp_trailing(&h->out.bs);
        x264_nal_end(h);
    }

    h->i_nal_type = i_nal_type;
    h->i_nal_ref_idc = i_nal_ref_idc;

    /* Write SPS and PPS */
    if( i_nal_type == NAL_SLICE_IDR && h->param.b_repeat_headers )
    {
        if( h->fenc->i_frame == 0 )
        {
            /* identify ourself */
            x264_nal_start( h, NAL_SEI, NAL_PRIORITY_DISPOSABLE );
            x264_sei_version_write( h, &h->out.bs );
            x264_nal_end( h );
        }

        /* generate sequence parameters */
        x264_nal_start( h, NAL_SPS, NAL_PRIORITY_HIGHEST );
        x264_sps_write( &h->out.bs, h->sps );
        x264_nal_end( h );

        /* generate picture parameters */
        x264_nal_start( h, NAL_PPS, NAL_PRIORITY_HIGHEST );
        x264_pps_write( &h->out.bs, h->pps );
        x264_nal_end( h );
    }

    /* Write frame */
    i_frame_size = x264_slices_write( h );

    /* restore CPU state (before using float again) */
    x264_cpu_restore( h->param.cpu );

    if( i_slice_type == SLICE_TYPE_P && !h->param.rc.b_stat_read 
        && h->param.i_scenecut_threshold >= 0 )
    {
        const int *mbs = h->stat.frame.i_mb_count;
        int i_mb_i = mbs[I_16x16] + mbs[I_8x8] + mbs[I_4x4];
        int i_mb_p = mbs[P_L0] + mbs[P_8x8];
        int i_mb_s = mbs[P_SKIP];
        int i_mb   = h->sps->i_mb_width * h->sps->i_mb_height;
        int64_t i_inter_cost = h->stat.frame.i_inter_cost;
        int64_t i_intra_cost = h->stat.frame.i_intra_cost;

        float f_bias;
        int i_gop_size = h->fenc->i_frame - h->frames.i_last_idr;
        float f_thresh_max = h->param.i_scenecut_threshold / 100.0;
        /* magic numbers pulled out of thin air */
        float f_thresh_min = f_thresh_max * h->param.i_keyint_min
                             / ( h->param.i_keyint_max * 4 );
        if( h->param.i_keyint_min == h->param.i_keyint_max )
             f_thresh_min= f_thresh_max;

        /* macroblock_analyse() doesn't further analyse skipped mbs,
         * so we have to guess their cost */
        if( i_mb_s < i_mb )
            i_intra_cost = i_intra_cost * i_mb / (i_mb - i_mb_s);

        if( i_gop_size < h->param.i_keyint_min / 4 )
            f_bias = f_thresh_min / 4;
        else if( i_gop_size <= h->param.i_keyint_min )
            f_bias = f_thresh_min * i_gop_size / h->param.i_keyint_min;
        else
        {
            f_bias = f_thresh_min
                     + ( f_thresh_max - f_thresh_min )
                       * ( i_gop_size - h->param.i_keyint_min )
                       / ( h->param.i_keyint_max - h->param.i_keyint_min );
        }
        f_bias = X264_MIN( f_bias, 1.0 );

        /* Bad P will be reencoded as I */
        if( i_mb_s < i_mb &&
            i_inter_cost >= (1.0 - f_bias) * i_intra_cost )
        {
            int b;

            x264_log( h, X264_LOG_DEBUG, "scene cut at %d Icost:%.0f Pcost:%.0f ratio:%.3f bias=%.3f lastIDR:%d (I:%d P:%d S:%d)\n",
                      h->fenc->i_frame,
                      (double)i_intra_cost, (double)i_inter_cost,
                      (double)i_inter_cost / i_intra_cost,
                      f_bias, i_gop_size,
                      i_mb_i, i_mb_p, i_mb_s );

            /* Restore frame num */
            h->i_frame_num--;

            for( b = 0; h->frames.current[b] && IS_X264_TYPE_B( h->frames.current[b]->i_type ); b++ );
            if( b > 0 )
            {
                /* If using B-frames, force GOP to be closed.
                 * Even if this frame is going to be I and not IDR, forcing a
                 * P-frame before the scenecut will probably help compression.
                 * 
                 * We don't yet know exactly which frame is the scene cut, so
                 * we can't assign an I-frame. Instead, change the previous
                 * B-frame to P, and rearrange coding order. */

                if( h->param.b_bframe_adaptive || b > 1 )
                    h->fenc->i_type = X264_TYPE_AUTO;
                x264_frame_sort_pts( h->frames.current );
                x264_frame_push( h->frames.next, h->fenc );
                h->fenc = h->frames.current[b-1];
                h->frames.current[b-1] = NULL;
                h->fenc->i_type = X264_TYPE_P;
                x264_frame_sort_dts( h->frames.current );
            }
            /* Do IDR if needed */
            else if( i_gop_size >= h->param.i_keyint_min )
            {
                x264_frame_t *tmp;

                /* Reset */
                h->i_frame_num = 0;

                /* Reinit field of fenc */
                h->fenc->i_type = X264_TYPE_IDR;
                h->fenc->i_poc = 0;

                /* Put enqueued frames back in the pool */
                while( (tmp = x264_frame_get( h->frames.current ) ) != NULL )
                    x264_frame_put( h->frames.next, tmp );
                x264_frame_sort_pts( h->frames.next );
            }
            else
            {
                h->fenc->i_type = X264_TYPE_I;
            }
            goto do_encode;
        }
    }

    /* End bitstream, set output  */
    *pi_nal = h->out.i_nal;
    *pp_nal = h->out.nal;

    /* Set output picture properties */
    if( i_slice_type == SLICE_TYPE_I )
        pic_out->i_type = i_nal_type == NAL_SLICE_IDR ? X264_TYPE_IDR : X264_TYPE_I;
    else if( i_slice_type == SLICE_TYPE_P )
        pic_out->i_type = X264_TYPE_P;
    else
        pic_out->i_type = X264_TYPE_B;
    pic_out->i_pts = h->fenc->i_pts;

    pic_out->img.i_plane = h->fdec->i_plane;
    for(i = 0; i < 4; i++){
        pic_out->img.i_stride[i] = h->fdec->i_stride[i];
        pic_out->img.plane[i] = h->fdec->plane[i];
    }

    /* ---------------------- Update encoder state ------------------------- */

    /* update rc */
    x264_cpu_restore( h->param.cpu );
    x264_ratecontrol_end( h, i_frame_size * 8 );

    /* handle references */
    if( i_nal_ref_idc != NAL_PRIORITY_DISPOSABLE )
        x264_reference_update( h );
#ifdef DEBUG_DUMP_FRAME
    else
        x264_fdec_deblock( h );
#endif
    x264_frame_put( h->frames.unused, h->fenc );

    /* increase frame count */
    h->i_frame++;

    /* restore CPU state (before using float again) */
    x264_cpu_restore( h->param.cpu );

    x264_noise_reduction_update( h );

    TIMER_STOP( i_mtime_encode_frame );

    /* ---------------------- Compute/Print statistics --------------------- */
    /* Slice stat */
    h->stat.i_slice_count[i_slice_type]++;
    h->stat.i_slice_size[i_slice_type] += i_frame_size + NALU_OVERHEAD;
    h->stat.i_slice_qp[i_slice_type] += i_global_qp;

    for( i = 0; i < 19; i++ )
        h->stat.i_mb_count[h->sh.i_type][i] += h->stat.frame.i_mb_count[i];
    for( i = 0; i < 2; i++ )
        h->stat.i_mb_count_8x8dct[i] += h->stat.frame.i_mb_count_8x8dct[i];
    if( h->sh.i_type != SLICE_TYPE_I )
    {
        for( i = 0; i < 7; i++ )
            h->stat.i_mb_count_size[h->sh.i_type][i] += h->stat.frame.i_mb_count_size[i];
        for( i = 0; i < 16; i++ )
            h->stat.i_mb_count_ref[h->sh.i_type][i] += h->stat.frame.i_mb_count_ref[i];
    }
    if( i_slice_type == SLICE_TYPE_B )
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

    if( h->param.analyse.b_psnr )
    {
        int64_t i_sqe_y, i_sqe_u, i_sqe_v;

        /* PSNR */
        i_sqe_y = x264_pixel_ssd_wxh( &h->pixf, frame_psnr->plane[0], frame_psnr->i_stride[0], h->fenc->plane[0], h->fenc->i_stride[0], h->param.i_width, h->param.i_height );
        i_sqe_u = x264_pixel_ssd_wxh( &h->pixf, frame_psnr->plane[1], frame_psnr->i_stride[1], h->fenc->plane[1], h->fenc->i_stride[1], h->param.i_width/2, h->param.i_height/2);
        i_sqe_v = x264_pixel_ssd_wxh( &h->pixf, frame_psnr->plane[2], frame_psnr->i_stride[2], h->fenc->plane[2], h->fenc->i_stride[2], h->param.i_width/2, h->param.i_height/2);
        x264_cpu_restore( h->param.cpu );

        h->stat.i_sqe_global[i_slice_type] += i_sqe_y + i_sqe_u + i_sqe_v;
        h->stat.f_psnr_average[i_slice_type] += x264_psnr( i_sqe_y + i_sqe_u + i_sqe_v, 3 * h->param.i_width * h->param.i_height / 2 );
        h->stat.f_psnr_mean_y[i_slice_type] += x264_psnr( i_sqe_y, h->param.i_width * h->param.i_height );
        h->stat.f_psnr_mean_u[i_slice_type] += x264_psnr( i_sqe_u, h->param.i_width * h->param.i_height / 4 );
        h->stat.f_psnr_mean_v[i_slice_type] += x264_psnr( i_sqe_v, h->param.i_width * h->param.i_height / 4 );

        snprintf( psz_message, 80, " PSNR Y:%2.2f U:%2.2f V:%2.2f",
                  x264_psnr( i_sqe_y, h->param.i_width * h->param.i_height ),
                  x264_psnr( i_sqe_u, h->param.i_width * h->param.i_height / 4),
                  x264_psnr( i_sqe_v, h->param.i_width * h->param.i_height / 4) );
        psz_message[79] = '\0';
    }
    else
    {
        psz_message[0] = '\0';
    }
    
    x264_log( h, X264_LOG_DEBUG,
                  "frame=%4d QP=%i NAL=%d Slice:%c Poc:%-3d I:%-4d P:%-4d SKIP:%-4d size=%d bytes%s\n",
              h->i_frame - 1,
              i_global_qp,
              i_nal_ref_idc,
              i_slice_type == SLICE_TYPE_I ? 'I' : (i_slice_type == SLICE_TYPE_P ? 'P' : 'B' ),
              frame_psnr->i_poc,
              h->stat.frame.i_mb_count_i,
              h->stat.frame.i_mb_count_p,
              h->stat.frame.i_mb_count_skip,
              i_frame_size,
              psz_message );


#ifdef DEBUG_MB_TYPE
{
    static const char mb_chars[] = { 'i', 'i', 'I', 'C', 'P', '8', 'S',
        'D', '<', 'X', 'B', 'X', '>', 'B', 'B', 'B', 'B', '8', 'S' };
    int mb_xy;
    for( mb_xy = 0; mb_xy < h->sps->i_mb_width * h->sps->i_mb_height; mb_xy++ )
    {
        if( h->mb.type[mb_xy] < 19 && h->mb.type[mb_xy] >= 0 )
            fprintf( stderr, "%c ", mb_chars[ h->mb.type[mb_xy] ] );
        else
            fprintf( stderr, "? " );

        if( (mb_xy+1) % h->sps->i_mb_width == 0 )
            fprintf( stderr, "\n" );
    }
}
#endif

#ifdef DEBUG_DUMP_FRAME
    /* Dump reconstructed frame */
    x264_frame_dump( h, frame_psnr, "fdec.yuv" );
#endif
    return 0;
}

/****************************************************************************
 * x264_encoder_close:
 ****************************************************************************/
void    x264_encoder_close  ( x264_t *h )
{
#ifdef DEBUG_BENCHMARK
    int64_t i_mtime_total = i_mtime_analyse + i_mtime_encode + i_mtime_write + i_mtime_filter + 1;
#endif
    int64_t i_yuv_size = 3 * h->param.i_width * h->param.i_height / 2;
    int i;

#ifdef DEBUG_BENCHMARK
    x264_log( h, X264_LOG_INFO,
              "analyse=%d(%lldms) encode=%d(%lldms) write=%d(%lldms) filter=%d(%lldms)\n",
              (int)(100*i_mtime_analyse/i_mtime_total), i_mtime_analyse/1000,
              (int)(100*i_mtime_encode/i_mtime_total), i_mtime_encode/1000,
              (int)(100*i_mtime_write/i_mtime_total), i_mtime_write/1000,
              (int)(100*i_mtime_filter/i_mtime_total), i_mtime_filter/1000 );
#endif

    /* Slices used and PSNR */
    for( i=0; i<5; i++ )
    {
        static const int slice_order[] = { SLICE_TYPE_I, SLICE_TYPE_SI, SLICE_TYPE_P, SLICE_TYPE_SP, SLICE_TYPE_B };
        static const char *slice_name[] = { "P", "B", "I", "SP", "SI" };
        int i_slice = slice_order[i];

        if( h->stat.i_slice_count[i_slice] > 0 )
        {
            const int i_count = h->stat.i_slice_count[i_slice];
            if( h->param.analyse.b_psnr )
            {
                x264_log( h, X264_LOG_INFO,
                          "slice %s:%-5d Avg QP:%5.2f  size:%6.0f  PSNR Mean Y:%5.2f U:%5.2f V:%5.2f Avg:%5.2f Global:%5.2f\n",
                          slice_name[i_slice],
                          i_count,
                          (double)h->stat.i_slice_qp[i_slice] / i_count,
                          (double)h->stat.i_slice_size[i_slice] / i_count,
                          h->stat.f_psnr_mean_y[i_slice] / i_count, h->stat.f_psnr_mean_u[i_slice] / i_count, h->stat.f_psnr_mean_v[i_slice] / i_count,
                          h->stat.f_psnr_average[i_slice] / i_count,
                          x264_psnr( h->stat.i_sqe_global[i_slice], i_count * i_yuv_size ) );
            }
            else
            {
                x264_log( h, X264_LOG_INFO,
                          "slice %s:%-5d Avg QP:%5.2f  size:%6.0f\n",
                          slice_name[i_slice],
                          i_count,
                          (double)h->stat.i_slice_qp[i_slice] / i_count,
                          (double)h->stat.i_slice_size[i_slice] / i_count );
            }
        }
    }

    /* MB types used */
    if( h->stat.i_slice_count[SLICE_TYPE_I] > 0 )
    {
        const int64_t *i_mb_count = h->stat.i_mb_count[SLICE_TYPE_I];
        const double i_count = h->stat.i_slice_count[SLICE_TYPE_I] * h->mb.i_mb_count / 100.0;
        x264_log( h, X264_LOG_INFO,
                  "mb I  I16..4: %4.1f%% %4.1f%% %4.1f%%\n",
                  i_mb_count[I_16x16]/ i_count,
                  i_mb_count[I_8x8]  / i_count,
                  i_mb_count[I_4x4]  / i_count );
    }
    if( h->stat.i_slice_count[SLICE_TYPE_P] > 0 )
    {
        const int64_t *i_mb_count = h->stat.i_mb_count[SLICE_TYPE_P];
        const int64_t *i_mb_size = h->stat.i_mb_count_size[SLICE_TYPE_P];
        const double i_count = h->stat.i_slice_count[SLICE_TYPE_P] * h->mb.i_mb_count / 100.0;
        x264_log( h, X264_LOG_INFO,
                  "mb P  I16..4: %4.1f%% %4.1f%% %4.1f%%  P16..4: %4.1f%% %4.1f%% %4.1f%% %4.1f%% %4.1f%%    skip:%4.1f%%\n",
                  i_mb_count[I_16x16]/ i_count,
                  i_mb_count[I_8x8]  / i_count,
                  i_mb_count[I_4x4]  / i_count,
                  i_mb_size[PIXEL_16x16] / (i_count*4),
                  (i_mb_size[PIXEL_16x8] + i_mb_size[PIXEL_8x16]) / (i_count*4),
                  i_mb_size[PIXEL_8x8] / (i_count*4),
                  (i_mb_size[PIXEL_8x4] + i_mb_size[PIXEL_4x8]) / (i_count*4),
                  i_mb_size[PIXEL_4x4] / (i_count*4),
                  i_mb_count[P_SKIP] / i_count );
    }
    if( h->stat.i_slice_count[SLICE_TYPE_B] > 0 )
    {
        const int64_t *i_mb_count = h->stat.i_mb_count[SLICE_TYPE_B];
        const int64_t *i_mb_size = h->stat.i_mb_count_size[SLICE_TYPE_B];
        const double i_count = h->stat.i_slice_count[SLICE_TYPE_B] * h->mb.i_mb_count / 100.0;
        x264_log( h, X264_LOG_INFO,
                  "mb B  I16..4: %4.1f%% %4.1f%% %4.1f%%  B16..8: %4.1f%% %4.1f%% %4.1f%%  direct:%4.1f%%  skip:%4.1f%%\n",
                  i_mb_count[I_16x16]  / i_count,
                  i_mb_count[I_8x8]    / i_count,
                  i_mb_count[I_4x4]    / i_count,
                  i_mb_size[PIXEL_16x16] / (i_count*4),
                  (i_mb_size[PIXEL_16x8] + i_mb_size[PIXEL_8x16]) / (i_count*4),
                  i_mb_size[PIXEL_8x8] / (i_count*4),
                  i_mb_count[B_DIRECT] / i_count,
                  i_mb_count[B_SKIP]   / i_count );
    }

    x264_ratecontrol_summary( h );

    if( h->stat.i_slice_count[SLICE_TYPE_I] + h->stat.i_slice_count[SLICE_TYPE_P] + h->stat.i_slice_count[SLICE_TYPE_B] > 0 )
    {
        const int i_count = h->stat.i_slice_count[SLICE_TYPE_I] +
                            h->stat.i_slice_count[SLICE_TYPE_P] +
                            h->stat.i_slice_count[SLICE_TYPE_B];
        float fps = (float) h->param.i_fps_num / h->param.i_fps_den;
#define SUM3(p) (p[SLICE_TYPE_I] + p[SLICE_TYPE_P] + p[SLICE_TYPE_B])
#define SUM3b(p,o) (p[SLICE_TYPE_I][o] + p[SLICE_TYPE_P][o] + p[SLICE_TYPE_B][o])
        float f_bitrate = fps * SUM3(h->stat.i_slice_size) / i_count / 125;

        if( h->param.analyse.b_transform_8x8 )
        {
            int64_t i_i8x8 = SUM3b( h->stat.i_mb_count, I_8x8 );
            int64_t i_intra = i_i8x8 + SUM3b( h->stat.i_mb_count, I_4x4 )
                                     + SUM3b( h->stat.i_mb_count, I_16x16 );
            x264_log( h, X264_LOG_INFO, "8x8 transform  intra:%.1f%%  inter:%.1f%%\n",
                      100. * i_i8x8 / i_intra,
                      100. * h->stat.i_mb_count_8x8dct[1] / h->stat.i_mb_count_8x8dct[0] );
        }

        if( h->param.analyse.i_direct_mv_pred == X264_DIRECT_PRED_AUTO
            && h->stat.i_slice_count[SLICE_TYPE_B] )
        {
            x264_log( h, X264_LOG_INFO, "direct mvs  spatial:%.1f%%  temporal:%.1f%%\n",
                      h->stat.i_direct_frames[1] * 100. / h->stat.i_slice_count[SLICE_TYPE_B],
                      h->stat.i_direct_frames[0] * 100. / h->stat.i_slice_count[SLICE_TYPE_B] );
        }

        if( h->param.i_frame_reference > 1 )
        {
            int i_slice;
            for( i_slice = 0; i_slice < 2; i_slice++ )
            {
                char buf[200];
                char *p = buf;
                int64_t i_den = 0;
                int i_max = 0;
                for( i = 0; i < h->param.i_frame_reference; i++ )
                    if( h->stat.i_mb_count_ref[i_slice][i] )
                    {
                        i_den += h->stat.i_mb_count_ref[i_slice][i];
                        i_max = i;
                    }
                if( i_max == 0 )
                    continue;
                for( i = 0; i <= i_max; i++ )
                    p += sprintf( p, " %4.1f%%", 100. * h->stat.i_mb_count_ref[i_slice][i] / i_den );
                x264_log( h, X264_LOG_INFO, "ref %c %s\n", i_slice==SLICE_TYPE_P ? 'P' : 'B', buf );
            }
        }

        if( h->param.analyse.b_psnr )
            x264_log( h, X264_LOG_INFO,
                      "PSNR Mean Y:%6.3f U:%6.3f V:%6.3f Avg:%6.3f Global:%6.3f kb/s:%.2f\n",
                      SUM3( h->stat.f_psnr_mean_y ) / i_count,
                      SUM3( h->stat.f_psnr_mean_u ) / i_count,
                      SUM3( h->stat.f_psnr_mean_v ) / i_count,
                      SUM3( h->stat.f_psnr_average ) / i_count,
                      x264_psnr( SUM3( h->stat.i_sqe_global ), i_count * i_yuv_size ),
                      f_bitrate );
        else
            x264_log( h, X264_LOG_INFO, "kb/s:%.1f\n", f_bitrate );
    }

    /* frames */
    for( i = 0; i < X264_BFRAME_MAX + 3; i++ )
    {
        if( h->frames.current[i] ) x264_frame_delete( h->frames.current[i] );
        if( h->frames.next[i] )    x264_frame_delete( h->frames.next[i] );
        if( h->frames.unused[i] )  x264_frame_delete( h->frames.unused[i] );
    }
    /* ref frames */
    for( i = 0; i < h->frames.i_max_dpb; i++ )
    {
        x264_frame_delete( h->frames.reference[i] );
    }

    /* rc */
    x264_ratecontrol_delete( h );

    /* param */
    if( h->param.rc.psz_stat_out )
        free( h->param.rc.psz_stat_out );
    if( h->param.rc.psz_stat_in )
        free( h->param.rc.psz_stat_in );
    if( h->param.rc.psz_rc_eq )
        free( h->param.rc.psz_rc_eq );

    x264_macroblock_cache_end( h );
    x264_free( h->out.p_bitstream );
    for( i = 1; i < h->param.i_threads; i++ )
        x264_free( h->thread[i] );
    x264_free( h );
}


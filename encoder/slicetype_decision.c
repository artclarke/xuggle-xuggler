/*****************************************************************************
 * slicetype_decision.c: h264 encoder library
 *****************************************************************************
 * Copyright (C) 2005 Loren Merritt
 *
 * Authors: Loren Merritt <lorenm@u.washington.edu>
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

#include <string.h>
#include <math.h>
#include <limits.h>

#include "common/common.h"
#include "common/cpu.h"
#include "macroblock.h"
#include "me.h"


static void x264_lowres_context_init( x264_t *h, x264_mb_analysis_t *a )
{
    a->i_qp = 12; // arbitrary, but low because SATD scores are 1/4 normal
    a->i_lambda = i_qp0_cost_table[ a->i_qp ];
    x264_mb_analyse_load_costs( h, a );
    h->mb.i_me_method = X264_MIN( X264_ME_HEX, h->param.analyse.i_me_method ); // maybe dia?
    h->mb.i_subpel_refine = 4; // 3 should be enough, but not tweaking for speed now
    h->mb.b_chroma_me = 0;
}

int x264_slicetype_mb_cost( x264_t *h, x264_mb_analysis_t *a,
                            x264_frame_t **frames, int p0, int p1, int b,
                            int dist_scale_factor )
{
    x264_frame_t *fref0 = frames[p0];
    x264_frame_t *fref1 = frames[p1];
    x264_frame_t *fenc  = frames[b];
    const int b_bidir = (b < p1);
    const int i_mb_x = h->mb.i_mb_x;
    const int i_mb_y = h->mb.i_mb_y;
    const int i_mb_stride = h->sps->i_mb_width;
    const int i_mb_xy = i_mb_x + i_mb_y * i_mb_stride;
    const int i_stride = fenc->i_stride_lowres;
    const int i_pel_offset = 8 * ( i_mb_x + i_mb_y * i_stride );

    uint8_t pix1[9*9], pix2[8*8];
    x264_me_t m[2];
    int mvc[4][2], i_mvc;
    int i_bcost = COST_MAX;
    int i_cost_bak;
    int l, i;

    if( !p0 && !p1 && !b )
        goto lowres_intra_mb;

    // no need for h->mb.mv_min[]
    h->mb.mv_min_fpel[0] = -16*h->mb.i_mb_x - 8;
    h->mb.mv_max_fpel[0] = 16*( h->sps->i_mb_width - h->mb.i_mb_x - 1 ) + 8;
    h->mb.mv_min_spel[0] = 4*( h->mb.mv_min_fpel[0] - 16 );
    h->mb.mv_max_spel[0] = 4*( h->mb.mv_max_fpel[0] + 16 );
    if( h->mb.i_mb_x <= 1)
    {
        h->mb.mv_min_fpel[1] = -16*h->mb.i_mb_y - 8;
        h->mb.mv_max_fpel[1] = 16*( h->sps->i_mb_height - h->mb.i_mb_y - 1 ) + 8;
        h->mb.mv_min_spel[1] = 4*( h->mb.mv_min_fpel[1] - 16 );
        h->mb.mv_max_spel[1] = 4*( h->mb.mv_max_fpel[1] + 16 );
    }

#define LOAD_HPELS_LUMA(dst, src) \
    { \
        (dst)[0] = &(src)[0][i_pel_offset]; \
        (dst)[1] = &(src)[1][i_pel_offset]; \
        (dst)[2] = &(src)[2][i_pel_offset]; \
        (dst)[3] = &(src)[3][i_pel_offset]; \
    }
#define SAVE_MVS( mv0, mv1 ) \
    { \
        fenc->mv[0][i_mb_xy][0] = mv0[0]; \
        fenc->mv[0][i_mb_xy][1] = mv0[1]; \
        if( b_bidir ) \
        { \
            fenc->mv[1][i_mb_xy][0] = mv1[0]; \
            fenc->mv[1][i_mb_xy][1] = mv1[1]; \
        } \
    }
#define TRY_BIDIR( mv0, mv1, penalty ) \
    { \
        int stride2 = 8; \
        uint8_t *src2; \
        int i_cost; \
        h->mc.mc_luma( m[0].p_fref, m[0].i_stride[0], pix1, 8, \
                       (mv0)[0], (mv0)[1], 8, 8 ); \
        src2 = h->mc.get_ref( m[1].p_fref, m[1].i_stride[0], pix2, &stride2, \
                       (mv1)[0], (mv1)[1], 8, 8 ); \
        h->mc.avg[PIXEL_8x8]( pix1, 8, src2, stride2 ); \
        i_cost = penalty + h->pixf.mbcmp[PIXEL_8x8]( \
                           m[0].p_fenc[0], m[0].i_stride[0], pix1, 8 ); \
        if( i_bcost > i_cost ) \
        { \
            i_bcost = i_cost; \
            SAVE_MVS( mv0, mv1 ); \
        } \
    }

    m[0].i_pixel = PIXEL_8x8;
    m[0].p_cost_mv = a->p_cost_mv;
    m[0].i_stride[0] = i_stride;
    m[0].p_fenc[0] = &fenc->lowres[0][ i_pel_offset ];
    LOAD_HPELS_LUMA( m[0].p_fref, fref0->lowres );

    if( b_bidir )
    {
        int16_t *mvr = fref1->mv[0][i_mb_xy];
        int dmv[2][2];
        int mv0[2] = {0,0};

        m[1] = m[0];
        LOAD_HPELS_LUMA( m[1].p_fref, fref1->lowres );

        dmv[0][0] = ( mvr[0] * dist_scale_factor + 128 ) >> 8;
        dmv[0][1] = ( mvr[1] * dist_scale_factor + 128 ) >> 8;
        dmv[1][0] = dmv[0][0] - mvr[0];
        dmv[1][1] = dmv[0][1] - mvr[1];

        TRY_BIDIR( dmv[0], dmv[1], 0 );
        if( dmv[0][0] || dmv[0][1] || dmv[1][0] || dmv[1][1] );
           TRY_BIDIR( mv0, mv0, 0 );
//      if( i_bcost < 60 ) // arbitrary threshold
//          return i_bcost;
    }

    i_cost_bak = i_bcost;
    for( l = 0; l < 1 + b_bidir; l++ )
    {
        int16_t (*fenc_mv)[2] = &fenc->mv[l][i_mb_xy];
        mvc[0][0] = fenc_mv[-1][0];
        mvc[0][1] = fenc_mv[-1][1];
        mvc[1][0] = fenc_mv[-i_mb_stride][0];
        mvc[1][1] = fenc_mv[-i_mb_stride][1];
        mvc[2][0] = fenc_mv[-i_mb_stride+1][0];
        mvc[2][1] = fenc_mv[-i_mb_stride+1][1];
        mvc[3][0] = fenc_mv[-i_mb_stride-1][0];
        mvc[3][1] = fenc_mv[-i_mb_stride-1][1];
        m[l].mvp[0] = x264_median( mvc[0][0], mvc[1][0], mvc[2][0] );
        m[l].mvp[1] = x264_median( mvc[0][1], mvc[1][1], mvc[2][1] );
        i_mvc = 4;

        x264_me_search( h, &m[l], mvc, i_mvc );

        i_bcost = X264_MIN( i_bcost, m[l].cost + 3 );
    }

    if( b_bidir && (m[0].mv[0] || m[0].mv[1] || m[1].mv[0] || m[1].mv[1]) )
        TRY_BIDIR( m[0].mv, m[1].mv, 5 );

    if( i_bcost < i_cost_bak )
        SAVE_MVS( m[0].mv, m[1].mv );

lowres_intra_mb:
    {
        uint8_t *src = &fenc->lowres[0][ i_pel_offset - i_stride - 1 ];
        int intra_penalty = 5 + 10 * b_bidir;
        i_cost_bak = i_bcost;

        memcpy( pix1, src, 9 );
        for( i=1; i<9; i++, src += i_stride )
            pix1[i*9] = src[0];
        src = &fenc->lowres[0][ i_pel_offset ];

        for( i = I_PRED_CHROMA_DC; i <= I_PRED_CHROMA_P; i++ )
        {
            int i_cost;
            h->predict_8x8c[i]( &pix1[10], 9 );
            i_cost = h->pixf.mbcmp[PIXEL_8x8]( &pix1[10], 9, src, i_stride ) + intra_penalty;
            i_bcost = X264_MIN( i_bcost, i_cost );
        }
        if( i_bcost != i_cost_bak )
        {
            if( !b_bidir )
                fenc->i_intra_mbs[b-p0]++;
            if( p1 > p0+1 )
                i_bcost = i_bcost * 9 / 8; // arbitray penalty for I-blocks in and after B-frames
        }
    }

    return i_bcost;
}
#undef TRY_BIDIR
#undef SAVE_MVS

int x264_slicetype_frame_cost( x264_t *h, x264_mb_analysis_t *a,
                               x264_frame_t **frames, int p0, int p1, int b )
{
    int i_score = 0;
    int dist_scale_factor = 128;

    /* Check whether we already evaluated this frame
     * If we have tried this frame as P, then we have also tried
     * the preceding frames as B. (is this still true?) */
    if( frames[b]->i_cost_est[b-p0][p1-b] >= 0 )
        return frames[b]->i_cost_est[b-p0][p1-b];

    /* Init MVs so that we don't have to check edge conditions when loading predictors. */
    /* FIXME: not needed every time */
    memset( frames[b]->mv[0], 0, h->sps->i_mb_height * h->sps->i_mb_width * 2*sizeof(int16_t) );
    if( b != p1 )
        memset( frames[b]->mv[1], 0, h->sps->i_mb_height * h->sps->i_mb_width * 2*sizeof(int16_t) );

    if( b == p1 )
        frames[b]->i_intra_mbs[b-p0] = 0;
    if( p1 != p0 )
        dist_scale_factor = ( ((b-p0) << 8) + ((p1-p0) >> 1) ) / (p1-p0);

    /* Skip the outermost ring of macroblocks, to simplify mv range and intra prediction. */
    for( h->mb.i_mb_y = 1; h->mb.i_mb_y < h->sps->i_mb_height - 1; h->mb.i_mb_y++ )
        for( h->mb.i_mb_x = 1; h->mb.i_mb_x < h->sps->i_mb_width - 1; h->mb.i_mb_x++ )
            i_score += x264_slicetype_mb_cost( h, a, frames, p0, p1, b, dist_scale_factor );

    if( b != p1 )
        i_score = i_score * 100 / (120 + h->param.i_bframe_bias);

    frames[b]->i_cost_est[b-p0][p1-b] = i_score;
//  fprintf( stderr, "frm %d %c(%d,%d): %6d I:%d  \n", frames[b]->i_frame,
//           (p1==0?'I':b<p1?'B':'P'), b-p0, p1-b, i_score, frames[b]->i_intra_mbs[b-p0] );
    x264_cpu_restore( h->param.cpu );
    return i_score;
}

void x264_slicetype_analyse( x264_t *h )
{
    x264_mb_analysis_t a;
    x264_frame_t *frames[X264_BFRAME_MAX+3] = { NULL, };
    int num_frames;
    int keyint_limit;
    int j;
    int i_mb_count = (h->sps->i_mb_width - 2) * (h->sps->i_mb_height - 2);
    int cost1p0, cost2p0, cost1b1, cost2p1;

    if( !h->frames.last_nonb )
        return;
    frames[0] = h->frames.last_nonb;
    for( j = 0; h->frames.next[j]; j++ )
        frames[j+1] = h->frames.next[j];
    keyint_limit = h->param.i_keyint_max - frames[0]->i_frame + h->frames.i_last_idr - 1;
    num_frames = X264_MIN( j, keyint_limit );
    if( num_frames == 0 )
        return;
    if( num_frames == 1 )
    {
no_b_frames:
        frames[1]->i_type = X264_TYPE_P;
        return;
    }

    x264_lowres_context_init( h, &a );

    cost2p1 = x264_slicetype_frame_cost( h, &a, frames, 0, 2, 2 );
    if( frames[2]->i_intra_mbs[2] > i_mb_count / 2 )
        goto no_b_frames;

    cost1b1 = x264_slicetype_frame_cost( h, &a, frames, 0, 2, 1 );
    cost1p0 = x264_slicetype_frame_cost( h, &a, frames, 0, 1, 1 );
    cost2p0 = x264_slicetype_frame_cost( h, &a, frames, 1, 2, 2 );
//  fprintf( stderr, "PP: %d + %d <=> BP: %d + %d \n",
//           cost1p0, cost2p0, cost1b1, cost2p1 );
    if( cost1p0 + cost2p0 < cost1b1 + cost2p1 )
        goto no_b_frames;

// arbitrary and untuned
#define INTER_THRESH 300
#define P_SENS_BIAS (50 - h->param.i_bframe_bias)
    frames[1]->i_type = X264_TYPE_B;

    for( j = 2; j <= X264_MIN( h->param.i_bframe, num_frames-1 ); j++ )
    {
        int pthresh = X264_MAX(INTER_THRESH - P_SENS_BIAS * (j-1), INTER_THRESH/10);
        int pcost = x264_slicetype_frame_cost( h, &a, frames, 0, j+1, j+1 );
//      fprintf( stderr, "frm%d+%d: %d <=> %d, I:%d/%d \n",
//               frames[0]->i_frame, j-1, pthresh, pcost/i_mb_count,
//               frames[j+1]->i_intra_mbs[j+1], i_mb_count );
        if( pcost > pthresh*i_mb_count || frames[j+1]->i_intra_mbs[j+1] > i_mb_count/3 )
        {
            frames[j]->i_type = X264_TYPE_P;
            break;
        }
        else
            frames[j]->i_type = X264_TYPE_B;
    }
}

void x264_slicetype_decide( x264_t *h )
{
    x264_frame_t *frm;
    int bframes;
    int i;

    if( h->frames.next[0] == NULL )
        return;

    if( h->param.rc.b_stat_read )
    {
        /* Use the frame types from the first pass */
        for( i = 0; h->frames.next[i] != NULL; i++ )
            h->frames.next[i]->i_type =
                x264_ratecontrol_slice_type( h, h->frames.next[i]->i_frame );
    }
    else if( h->param.i_bframe && h->param.b_bframe_adaptive )
        x264_slicetype_analyse( h );

    for( bframes = 0;; bframes++ )
    {
        frm = h->frames.next[bframes];

        /* Limit GOP size */
        if( frm->i_frame - h->frames.i_last_idr >= h->param.i_keyint_max )
        {
            if( frm->i_type == X264_TYPE_AUTO )
                frm->i_type = X264_TYPE_IDR;
            if( frm->i_type != X264_TYPE_IDR )
                x264_log( h, X264_LOG_ERROR, "specified frame type (%d) is not compatible with keyframe interval\n", frm->i_type );
        }
        if( frm->i_type == X264_TYPE_IDR )
        {
            /* Close GOP */
            if( bframes > 0 )
            {
                bframes--;
                h->frames.next[bframes]->i_type = X264_TYPE_P;
            }
            else
            {
                h->i_frame_num = 0;
            }
        }

        if( bframes == h->param.i_bframe
            || h->frames.next[bframes+1] == NULL )
        {
            if( IS_X264_TYPE_B( frm->i_type ) )
                x264_log( h, X264_LOG_ERROR, "specified frame type is not compatible with max B-frames\n" );
            if( frm->i_type == X264_TYPE_AUTO
                || IS_X264_TYPE_B( frm->i_type ) )
                frm->i_type = X264_TYPE_P;
        }

        if( frm->i_type != X264_TYPE_AUTO && frm->i_type != X264_TYPE_B && frm->i_type != X264_TYPE_BREF )
            break;

        frm->i_type = X264_TYPE_B;
    }
}

int x264_rc_analyse_slice( x264_t *h )
{
    int p1 = 0;
    x264_mb_analysis_t a;
    x264_frame_t *frames[X264_BFRAME_MAX+2] = { NULL, };

    if( IS_X264_TYPE_I(h->fenc->i_type) )
        return x264_slicetype_frame_cost( h, &a, &h->fenc, 0, 0, 0 );

    while( h->frames.current[p1] && IS_X264_TYPE_B( h->frames.current[p1]->i_type ) )
        p1++;
    p1++;
    if( h->fenc->i_cost_est[p1][0] >= 0 )
        return h->fenc->i_cost_est[p1][0];

    frames[0] = h->fref0[0];
    frames[p1] = h->fenc;
    x264_lowres_context_init( h, &a );

    return x264_slicetype_frame_cost( h, &a, frames, 0, p1, p1 );
}

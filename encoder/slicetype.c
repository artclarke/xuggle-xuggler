/*****************************************************************************
 * slicetype.c: h264 encoder library
 *****************************************************************************
 * Copyright (C) 2005-2008 x264 project
 *
 * Authors: Loren Merritt <lorenm@u.washington.edu>
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
#include <limits.h>

#include "common/common.h"
#include "common/cpu.h"
#include "macroblock.h"
#include "me.h"


static void x264_lowres_context_init( x264_t *h, x264_mb_analysis_t *a )
{
    a->i_qp = 12; // arbitrary, but low because SATD scores are 1/4 normal
    a->i_lambda = x264_lambda_tab[ a->i_qp ];
    x264_mb_analyse_load_costs( h, a );
    h->mb.i_me_method = X264_MIN( X264_ME_HEX, h->param.analyse.i_me_method ); // maybe dia?
    h->mb.i_subpel_refine = 4; // 3 should be enough, but not tweaking for speed now
    h->mb.b_chroma_me = 0;
}

static int x264_slicetype_mb_cost( x264_t *h, x264_mb_analysis_t *a,
                            x264_frame_t **frames, int p0, int p1, int b,
                            int dist_scale_factor, int do_search[2] )
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
    const int i_bipred_weight = h->param.analyse.b_weighted_bipred ? 64 - (dist_scale_factor>>2) : 32;
    int16_t (*fenc_mvs[2])[2] = { &frames[b]->lowres_mvs[0][b-p0-1][i_mb_xy], &frames[b]->lowres_mvs[1][p1-b-1][i_mb_xy] };
    int (*fenc_costs[2]) = { &frames[b]->lowres_mv_costs[0][b-p0-1][i_mb_xy], &frames[b]->lowres_mv_costs[1][p1-b-1][i_mb_xy] };

    DECLARE_ALIGNED_8( uint8_t pix1[9*FDEC_STRIDE] );
    uint8_t *pix2 = pix1+8;
    x264_me_t m[2];
    int i_bcost = COST_MAX;
    int l, i;

    h->mb.pic.p_fenc[0] = h->mb.pic.fenc_buf;
    h->mc.copy[PIXEL_8x8]( h->mb.pic.p_fenc[0], FENC_STRIDE, &fenc->lowres[0][i_pel_offset], i_stride, 8 );

    if( !p0 && !p1 && !b )
        goto lowres_intra_mb;

    // no need for h->mb.mv_min[]
    h->mb.mv_min_fpel[0] = -8*h->mb.i_mb_x - 4;
    h->mb.mv_max_fpel[0] = 8*( h->sps->i_mb_width - h->mb.i_mb_x - 1 ) + 4;
    h->mb.mv_min_spel[0] = 4*( h->mb.mv_min_fpel[0] - 8 );
    h->mb.mv_max_spel[0] = 4*( h->mb.mv_max_fpel[0] + 8 );
    if( h->mb.i_mb_x >= h->sps->i_mb_width - 2 )
    {
        h->mb.mv_min_fpel[1] = -8*h->mb.i_mb_y - 4;
        h->mb.mv_max_fpel[1] = 8*( h->sps->i_mb_height - h->mb.i_mb_y - 1 ) + 4;
        h->mb.mv_min_spel[1] = 4*( h->mb.mv_min_fpel[1] - 8 );
        h->mb.mv_max_spel[1] = 4*( h->mb.mv_max_fpel[1] + 8 );
    }

#define LOAD_HPELS_LUMA(dst, src) \
    { \
        (dst)[0] = &(src)[0][i_pel_offset]; \
        (dst)[1] = &(src)[1][i_pel_offset]; \
        (dst)[2] = &(src)[2][i_pel_offset]; \
        (dst)[3] = &(src)[3][i_pel_offset]; \
    }
#define CLIP_MV( mv ) \
    { \
        mv[0] = x264_clip3( mv[0], h->mb.mv_min_spel[0], h->mb.mv_max_spel[0] ); \
        mv[1] = x264_clip3( mv[1], h->mb.mv_min_spel[1], h->mb.mv_max_spel[1] ); \
    }
#define TRY_BIDIR( mv0, mv1, penalty ) \
    { \
        int stride1 = 16, stride2 = 16; \
        uint8_t *src1, *src2; \
        int i_cost; \
        src1 = h->mc.get_ref( pix1, &stride1, m[0].p_fref, m[0].i_stride[0], \
                              (mv0)[0], (mv0)[1], 8, 8 ); \
        src2 = h->mc.get_ref( pix2, &stride2, m[1].p_fref, m[1].i_stride[0], \
                              (mv1)[0], (mv1)[1], 8, 8 ); \
        h->mc.avg[PIXEL_8x8]( pix1, 16, src1, stride1, src2, stride2, i_bipred_weight ); \
        i_cost = penalty + h->pixf.mbcmp[PIXEL_8x8]( \
                           m[0].p_fenc[0], FENC_STRIDE, pix1, 16 ); \
        if( i_bcost > i_cost ) \
            i_bcost = i_cost; \
    }

    m[0].i_pixel = PIXEL_8x8;
    m[0].p_cost_mv = a->p_cost_mv;
    m[0].i_stride[0] = i_stride;
    m[0].p_fenc[0] = h->mb.pic.p_fenc[0];
    LOAD_HPELS_LUMA( m[0].p_fref, fref0->lowres );

    if( b_bidir )
    {
        int16_t *mvr = fref1->lowres_mvs[0][p1-p0-1][i_mb_xy];
        int dmv[2][2];
        int mv0[2] = {0,0};

        h->mc.memcpy_aligned( &m[1], &m[0], sizeof(x264_me_t) );
        LOAD_HPELS_LUMA( m[1].p_fref, fref1->lowres );

        dmv[0][0] = ( mvr[0] * dist_scale_factor + 128 ) >> 8;
        dmv[0][1] = ( mvr[1] * dist_scale_factor + 128 ) >> 8;
        dmv[1][0] = dmv[0][0] - mvr[0];
        dmv[1][1] = dmv[0][1] - mvr[1];
        CLIP_MV( dmv[0] );
        CLIP_MV( dmv[1] );

        TRY_BIDIR( dmv[0], dmv[1], 0 );
        if( dmv[0][0] | dmv[0][1] | dmv[1][0] | dmv[1][1] )
           TRY_BIDIR( mv0, mv0, 0 );
//      if( i_bcost < 60 ) // arbitrary threshold
//          return i_bcost;
    }

    for( l = 0; l < 1 + b_bidir; l++ )
    {
        DECLARE_ALIGNED_4(int16_t mvc[4][2]) = {{0}};
        int i_mvc = 0;
        int16_t (*fenc_mv)[2] = fenc_mvs[l];

        if( do_search[l] )
        {
            /* Reverse-order MV prediction. */
#define MVC(mv) { *(uint32_t*)mvc[i_mvc] = *(uint32_t*)mv; i_mvc++; }
            if( i_mb_x < h->sps->i_mb_width - 1 )
                MVC(fenc_mv[1]);
            if( i_mb_y < h->sps->i_mb_height - 1 )
            {
                MVC(fenc_mv[i_mb_stride]);
                if( i_mb_x > 0 )
                    MVC(fenc_mv[i_mb_stride-1]);
                if( i_mb_x < h->sps->i_mb_width - 1 )
                    MVC(fenc_mv[i_mb_stride+1]);
            }
#undef MVC
            x264_median_mv( m[l].mvp, mvc[0], mvc[1], mvc[2] );
            x264_me_search( h, &m[l], mvc, i_mvc );

            m[l].cost -= 2; // remove mvcost from skip mbs
            if( *(uint32_t*)m[l].mv )
                m[l].cost += 5;
            *(uint32_t*)fenc_mvs[l] = *(uint32_t*)m[l].mv;
            *fenc_costs[l] = m[l].cost;
        }
        else
        {
            *(uint32_t*)m[l].mv = *(uint32_t*)fenc_mvs[l];
            m[l].cost = *fenc_costs[l];
        }
        i_bcost = X264_MIN( i_bcost, m[l].cost );
    }

    if( b_bidir && ( *(uint32_t*)m[0].mv || *(uint32_t*)m[1].mv ) )
        TRY_BIDIR( m[0].mv, m[1].mv, 5 );

lowres_intra_mb:
    /* forbid intra-mbs in B-frames, because it's rare and not worth checking */
    /* FIXME: Should we still forbid them now that we cache intra scores? */
    if( !b_bidir )
    {
        int i_icost, b_intra;
        if( !fenc->b_intra_calculated )
        {
            DECLARE_ALIGNED_16( uint8_t edge[33] );
            uint8_t *pix = &pix1[8+FDEC_STRIDE - 1];
            uint8_t *src = &fenc->lowres[0][i_pel_offset - 1];
            const int intra_penalty = 5;
            int satds[4];

            memcpy( pix-FDEC_STRIDE, src-i_stride, 17 );
            for( i=0; i<8; i++ )
                pix[i*FDEC_STRIDE] = src[i*i_stride];
            pix++;

            if( h->pixf.intra_satd_x3_8x8c && h->pixf.mbcmp[0] == h->pixf.satd[0] )
            {
                h->pixf.intra_satd_x3_8x8c( h->mb.pic.p_fenc[0], pix, satds );
                h->predict_8x8c[I_PRED_CHROMA_P]( pix );
                satds[I_PRED_CHROMA_P] =
                    h->pixf.satd[PIXEL_8x8]( pix, FDEC_STRIDE, h->mb.pic.p_fenc[0], FENC_STRIDE );
            }
            else
            {
                for( i=0; i<4; i++ )
                {
                    h->predict_8x8c[i]( pix );
                    satds[i] = h->pixf.mbcmp[PIXEL_8x8]( pix, FDEC_STRIDE, h->mb.pic.p_fenc[0], FENC_STRIDE );
                }
            }
            i_icost = X264_MIN4( satds[0], satds[1], satds[2], satds[3] );

            x264_predict_8x8_filter( pix, edge, ALL_NEIGHBORS, ALL_NEIGHBORS );
            for( i=3; i<9; i++ )
            {
                int satd;
                h->predict_8x8[i]( pix, edge );
                satd = h->pixf.mbcmp[PIXEL_8x8]( pix, FDEC_STRIDE, h->mb.pic.p_fenc[0], FENC_STRIDE );
                i_icost = X264_MIN( i_icost, satd );
            }

            i_icost += intra_penalty;
            fenc->i_intra_cost[i_mb_xy] = i_icost;
        }
        else
            i_icost = fenc->i_intra_cost[i_mb_xy];
        b_intra = i_icost < i_bcost;
        if( b_intra )
            i_bcost = i_icost;
        if(   (i_mb_x > 0 && i_mb_x < h->sps->i_mb_width - 1
            && i_mb_y > 0 && i_mb_y < h->sps->i_mb_height - 1)
            || h->sps->i_mb_width <= 2 || h->sps->i_mb_height <= 2 )
        {
            fenc->i_intra_mbs[b-p0] += b_intra;
            fenc->i_cost_est[0][0] += i_icost;
        }
    }

    return i_bcost;
}
#undef TRY_BIDIR

#define NUM_MBS\
   (h->sps->i_mb_width > 2 && h->sps->i_mb_height > 2 ?\
   (h->sps->i_mb_width - 2) * (h->sps->i_mb_height - 2) :\
    h->sps->i_mb_width * h->sps->i_mb_height)

static int x264_slicetype_frame_cost( x264_t *h, x264_mb_analysis_t *a,
                               x264_frame_t **frames, int p0, int p1, int b,
                               int b_intra_penalty )
{
    int i_score = 0;
    /* Don't use the AQ'd scores for slicetype decision. */
    int i_score_aq = 0;
    int do_search[2];

    /* Check whether we already evaluated this frame
     * If we have tried this frame as P, then we have also tried
     * the preceding frames as B. (is this still true?) */
    /* Also check that we already calculated the row SATDs for the current frame. */
    if( frames[b]->i_cost_est[b-p0][p1-b] >= 0 && (!h->param.rc.i_vbv_buffer_size || frames[b]->i_row_satds[b-p0][p1-b][0] != -1) )
    {
        i_score = frames[b]->i_cost_est[b-p0][p1-b];
    }
    else
    {
        int dist_scale_factor = 128;
        int *row_satd = frames[b]->i_row_satds[b-p0][p1-b];

        /* For each list, check to see whether we have lowres motion-searched this reference frame before. */
        do_search[0] = b != p0 && frames[b]->lowres_mvs[0][b-p0-1][0][0] == 0x7FFF;
        do_search[1] = b != p1 && frames[b]->lowres_mvs[1][p1-b-1][0][0] == 0x7FFF;
        if( do_search[0] ) frames[b]->lowres_mvs[0][b-p0-1][0][0] = 0;
        if( do_search[1] ) frames[b]->lowres_mvs[1][p1-b-1][0][0] = 0;

        if( b == p1 )
        {
            frames[b]->i_intra_mbs[b-p0] = 0;
            frames[b]->i_cost_est[0][0] = 0;
        }
        if( p1 != p0 )
            dist_scale_factor = ( ((b-p0) << 8) + ((p1-p0) >> 1) ) / (p1-p0);

        /* Lowres lookahead goes backwards because the MVs are used as predictors in the main encode.
         * This considerably improves MV prediction overall. */

        /* the edge mbs seem to reduce the predictive quality of the
         * whole frame's score, but are needed for a spatial distribution. */
        if( h->param.rc.i_vbv_buffer_size || h->sps->i_mb_width <= 2 || h->sps->i_mb_height <= 2 )
        {
            for( h->mb.i_mb_y = h->sps->i_mb_height - 1; h->mb.i_mb_y >= 0; h->mb.i_mb_y-- )
            {
                row_satd[ h->mb.i_mb_y ] = 0;
                for( h->mb.i_mb_x = h->sps->i_mb_width - 1; h->mb.i_mb_x >= 0; h->mb.i_mb_x-- )
                {
                    int i_mb_cost = x264_slicetype_mb_cost( h, a, frames, p0, p1, b, dist_scale_factor, do_search );
                    int i_mb_cost_aq = i_mb_cost;
                    if( h->param.rc.i_aq_mode )
                        i_mb_cost_aq = (i_mb_cost_aq * frames[b]->i_inv_qscale_factor[h->mb.i_mb_x + h->mb.i_mb_y*h->mb.i_mb_stride] + 128) >> 8;
                    row_satd[ h->mb.i_mb_y ] += i_mb_cost_aq;
                    if( (h->mb.i_mb_y > 0 && h->mb.i_mb_y < h->sps->i_mb_height - 1 &&
                         h->mb.i_mb_x > 0 && h->mb.i_mb_x < h->sps->i_mb_width - 1) ||
                         h->sps->i_mb_width <= 2 || h->sps->i_mb_height <= 2 )
                    {
                        /* Don't use AQ-weighted costs for slicetype decision, only for ratecontrol. */
                        i_score += i_mb_cost;
                        i_score_aq += i_mb_cost_aq;
                    }
                }
            }
        }
        else
        {
            for( h->mb.i_mb_y = h->sps->i_mb_height - 2; h->mb.i_mb_y > 0; h->mb.i_mb_y-- )
                for( h->mb.i_mb_x = h->sps->i_mb_width - 2; h->mb.i_mb_x > 0; h->mb.i_mb_x-- )
                {
                    int i_mb_cost = x264_slicetype_mb_cost( h, a, frames, p0, p1, b, dist_scale_factor, do_search );
                    int i_mb_cost_aq = i_mb_cost;
                    if( h->param.rc.i_aq_mode )
                        i_mb_cost_aq = (i_mb_cost_aq * frames[b]->i_inv_qscale_factor[h->mb.i_mb_x + h->mb.i_mb_y*h->mb.i_mb_stride] + 128) >> 8;
                    i_score += i_mb_cost;
                    i_score_aq += i_mb_cost_aq;
                }
        }

        if( b != p1 )
            i_score = i_score * 100 / (120 + h->param.i_bframe_bias);
        else
            frames[b]->b_intra_calculated = 1;

        frames[b]->i_cost_est[b-p0][p1-b] = i_score;
        frames[b]->i_cost_est_aq[b-p0][p1-b] = i_score_aq;
        x264_emms();
    }

    if( b_intra_penalty )
    {
        // arbitrary penalty for I-blocks after B-frames
        int nmb = NUM_MBS;
        i_score += i_score * frames[b]->i_intra_mbs[b-p0] / (nmb * 8);
    }
    return i_score;
}

#define MAX_LENGTH (X264_BFRAME_MAX*4)

static int x264_slicetype_path_cost( x264_t *h, x264_mb_analysis_t *a, x264_frame_t **frames, char *path, int threshold )
{
    int loc = 1;
    int cost = 0;
    int cur_p = 0;
    path--; /* Since the 1st path element is really the second frame */
    while( path[loc] )
    {
        int next_p = loc;
        int next_b;
        /* Find the location of the next P-frame. */
        while( path[next_p] && path[next_p] != 'P' )
            next_p++;
        /* Return if the path doesn't end on a P-frame. */
        if( path[next_p] != 'P' )
            return cost;

        /* Add the cost of the P-frame found above */
        cost += x264_slicetype_frame_cost( h, a, frames, cur_p, next_p, next_p, 0 );
        /* Early terminate if the cost we have found is larger than the best path cost so far */
        if( cost > threshold )
            break;

        for( next_b = loc; next_b < next_p && cost < threshold; next_b++ )
            cost += x264_slicetype_frame_cost( h, a, frames, cur_p, next_p, next_b, 0 );

        loc = next_p + 1;
        cur_p = next_p;
    }
    return cost;
}

/* Viterbi/trellis slicetype decision algorithm. */
/* Uses strings due to the fact that the speed of the control functions is
   negligable compared to the cost of running slicetype_frame_cost, and because
   it makes debugging easier. */
static void x264_slicetype_path( x264_t *h, x264_mb_analysis_t *a, x264_frame_t **frames, int length, int max_bframes, int buffer_size, char (*best_paths)[MAX_LENGTH] )
{
    char paths[X264_BFRAME_MAX+2][MAX_LENGTH] = {{0}};
    int num_paths = X264_MIN(max_bframes+1, length);
    int suffix_size, loc, path;
    int best_cost = COST_MAX;
    int best_path_index = 0;
    length = X264_MIN(length,MAX_LENGTH);

    /* Iterate over all currently possible paths and add suffixes to each one */
    for( suffix_size = 0; suffix_size < num_paths; suffix_size++ )
    {
        memcpy( paths[suffix_size], best_paths[length - (suffix_size + 1)], length - (suffix_size + 1) );
        for( loc = 0; loc < suffix_size; loc++ )
            strcat( paths[suffix_size], "B" );
        strcat( paths[suffix_size], "P" );
    }

    /* Calculate the actual cost of each of the current paths */
    for( path = 0; path < num_paths; path++ )
    {
        int cost = x264_slicetype_path_cost( h, a, frames, paths[path], best_cost );
        if( cost < best_cost )
        {
            best_cost = cost;
            best_path_index = path;
        }
    }

    /* Store the best path. */
    memcpy( best_paths[length], paths[best_path_index], length );
}

static int x264_slicetype_path_search( x264_t *h, x264_mb_analysis_t *a, x264_frame_t **frames, int length, int bframes, int buffer )
{
    char best_paths[MAX_LENGTH][MAX_LENGTH] = {"","P"};
    int n;
    for( n = 2; n < length-1; n++ )
        x264_slicetype_path( h, a, frames, n, bframes, buffer, best_paths );
    return strspn( best_paths[length-2], "B" );
}

static int scenecut( x264_t *h, x264_mb_analysis_t *a, x264_frame_t **frames, int p0, int p1 )
{
    x264_frame_t *frame = frames[p1];
    x264_slicetype_frame_cost( h, a, frames, p0, p1, p1, 0 );

    int icost = frame->i_cost_est[0][0];
    int pcost = frame->i_cost_est[p1-p0][0];
    float f_bias;
    int i_gop_size = frame->i_frame - h->frames.i_last_idr;
    float f_thresh_max = h->param.i_scenecut_threshold / 100.0;
    /* magic numbers pulled out of thin air */
    float f_thresh_min = f_thresh_max * h->param.i_keyint_min
                         / ( h->param.i_keyint_max * 4 );
    int res;

    if( h->param.i_keyint_min == h->param.i_keyint_max )
        f_thresh_min= f_thresh_max;
    if( i_gop_size < h->param.i_keyint_min / 4 )
        f_bias = f_thresh_min / 4;
    else if( i_gop_size <= h->param.i_keyint_min )
        f_bias = f_thresh_min * i_gop_size / h->param.i_keyint_min;
    else
    {
        f_bias = f_thresh_min
                 + ( f_thresh_max - f_thresh_min )
                    * ( i_gop_size - h->param.i_keyint_min )
                   / ( h->param.i_keyint_max - h->param.i_keyint_min ) ;
    }

    res = pcost >= (1.0 - f_bias) * icost;
    if( res )
    {
        int imb = frame->i_intra_mbs[p1-p0];
        int pmb = NUM_MBS - imb;
        x264_log( h, X264_LOG_DEBUG, "scene cut at %d Icost:%d Pcost:%d ratio:%.4f bias:%.4f gop:%d (imb:%d pmb:%d)\n",
                  frame->i_frame,
                  icost, pcost, 1. - (double)pcost / icost,
                  f_bias, i_gop_size, imb, pmb );
    }
    return res;
}

static void x264_slicetype_analyse( x264_t *h )
{
    x264_mb_analysis_t a;
    x264_frame_t *frames[X264_BFRAME_MAX*4+3] = { NULL, };
    int num_frames;
    int keyint_limit;
    int j;
    int i_mb_count = NUM_MBS;
    int cost1p0, cost2p0, cost1b1, cost2p1;
    int idr_frame_type;

    assert( h->frames.b_have_lowres );

    if( !h->frames.last_nonb )
        return;
    frames[0] = h->frames.last_nonb;
    for( j = 0; h->frames.next[j] && h->frames.next[j]->i_type == X264_TYPE_AUTO; j++ )
        frames[j+1] = h->frames.next[j];
    keyint_limit = h->param.i_keyint_max - frames[0]->i_frame + h->frames.i_last_idr - 1;
    num_frames = X264_MIN( j, keyint_limit );
    if( num_frames == 0 )
        return;

    x264_lowres_context_init( h, &a );
    idr_frame_type = frames[1]->i_frame - h->frames.i_last_idr >= h->param.i_keyint_min ? X264_TYPE_IDR : X264_TYPE_I;

    if( num_frames == 1 )
    {
no_b_frames:
        frames[1]->i_type = X264_TYPE_P;
        if( h->param.i_scenecut_threshold && scenecut( h, &a, frames, 0, 1 ) )
            frames[1]->i_type = idr_frame_type;
        return;
    }

    if( h->param.i_bframe_adaptive == X264_B_ADAPT_TRELLIS )
    {
        int num_bframes;
        int max_bframes = X264_MIN(num_frames-1, h->param.i_bframe);
        if( h->param.i_scenecut_threshold && scenecut( h, &a, frames, 0, 1 ) )
        {
            frames[1]->i_type = idr_frame_type;
            return;
        }
        num_bframes = x264_slicetype_path_search( h, &a, frames, num_frames, max_bframes, num_frames-max_bframes );
        assert(num_bframes < num_frames);

        for( j = 1; j < num_bframes+1; j++ )
        {
            if( h->param.i_scenecut_threshold && scenecut( h, &a, frames, j, j+1 ) )
            {
                frames[j]->i_type = X264_TYPE_P;
                return;
            }
            frames[j]->i_type = X264_TYPE_B;
        }
        frames[num_bframes+1]->i_type = X264_TYPE_P;
    }
    else if( h->param.i_bframe_adaptive == X264_B_ADAPT_FAST )
    {
        cost2p1 = x264_slicetype_frame_cost( h, &a, frames, 0, 2, 2, 1 );
        if( frames[2]->i_intra_mbs[2] > i_mb_count / 2 )
            goto no_b_frames;

        cost1b1 = x264_slicetype_frame_cost( h, &a, frames, 0, 2, 1, 0 );
        cost1p0 = x264_slicetype_frame_cost( h, &a, frames, 0, 1, 1, 0 );
        cost2p0 = x264_slicetype_frame_cost( h, &a, frames, 1, 2, 2, 0 );

        if( cost1p0 + cost2p0 < cost1b1 + cost2p1 )
            goto no_b_frames;

        // arbitrary and untuned
        #define INTER_THRESH 300
        #define P_SENS_BIAS (50 - h->param.i_bframe_bias)
        frames[1]->i_type = X264_TYPE_B;

        for( j = 2; j <= X264_MIN( h->param.i_bframe, num_frames-1 ); j++ )
        {
            int pthresh = X264_MAX(INTER_THRESH - P_SENS_BIAS * (j-1), INTER_THRESH/10);
            int pcost = x264_slicetype_frame_cost( h, &a, frames, 0, j+1, j+1, 1 );

            if( pcost > pthresh*i_mb_count || frames[j+1]->i_intra_mbs[j+1] > i_mb_count/3 )
            {
                frames[j]->i_type = X264_TYPE_P;
                break;
            }
            else
                frames[j]->i_type = X264_TYPE_B;
        }
    }
    else
    {
        int max_bframes = X264_MIN(num_frames-1, h->param.i_bframe);
        if( h->param.i_scenecut_threshold && scenecut( h, &a, frames, 0, 1 ) )
        {
            frames[1]->i_type = idr_frame_type;
            return;
        }

        for( j = 1; j < max_bframes+1; j++ )
        {
            if( h->param.i_scenecut_threshold && scenecut( h, &a, frames, j, j+1 ) )
            {
                frames[j]->i_type = X264_TYPE_P;
                return;
            }
            frames[j]->i_type = X264_TYPE_B;
        }
        frames[max_bframes+1]->i_type = X264_TYPE_P;
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
    else if( (h->param.i_bframe && h->param.i_bframe_adaptive)
             || h->param.i_scenecut_threshold )
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
                x264_log( h, X264_LOG_WARNING, "specified frame type (%d) is not compatible with keyframe interval\n", frm->i_type );
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
                x264_log( h, X264_LOG_WARNING, "specified frame type is not compatible with max B-frames\n" );
            if( frm->i_type == X264_TYPE_AUTO
                || IS_X264_TYPE_B( frm->i_type ) )
                frm->i_type = X264_TYPE_P;
        }

        if( frm->i_type == X264_TYPE_AUTO ) frm->i_type = X264_TYPE_B;
        else if( !IS_X264_TYPE_B( frm->i_type ) ) break;
    }
}

int x264_rc_analyse_slice( x264_t *h )
{
    x264_mb_analysis_t a;
    x264_frame_t *frames[X264_BFRAME_MAX*4+2] = { NULL, };
    int p0=0, p1, b;
    int cost;

    x264_lowres_context_init( h, &a );

    if( IS_X264_TYPE_I(h->fenc->i_type) )
    {
        p1 = b = 0;
    }
    else if( X264_TYPE_P == h->fenc->i_type )
    {
        p1 = 0;
        while( h->frames.current[p1] && IS_X264_TYPE_B( h->frames.current[p1]->i_type ) )
            p1++;
        p1++;
        b = p1;
    }
    else //B
    {
        p1 = (h->fref1[0]->i_poc - h->fref0[0]->i_poc)/2;
        b  = (h->fref1[0]->i_poc - h->fenc->i_poc)/2;
        frames[p1] = h->fref1[0];
    }
    frames[p0] = h->fref0[0];
    frames[b] = h->fenc;

    cost = x264_slicetype_frame_cost( h, &a, frames, p0, p1, b, 0 );

    /* In AQ, use the weighted score instead. */
    if( h->param.rc.i_aq_mode )
        cost = frames[b]->i_cost_est[b-p0][p1-b];

    h->fenc->i_row_satd = h->fenc->i_row_satds[b-p0][p1-b];
    h->fdec->i_row_satd = h->fdec->i_row_satds[b-p0][p1-b];
    h->fdec->i_satd = cost;
    memcpy( h->fdec->i_row_satd, h->fenc->i_row_satd, h->sps->i_mb_height * sizeof(int) );
    return cost;
}

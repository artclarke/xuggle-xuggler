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

#include "common/common.h"
#include "common/cpu.h"
#include "macroblock.h"
#include "me.h"


static int x264_lowres_context_init( x264_t *h, x264_mb_analysis_t *a )
{
    a->i_qp = 12; // arbitrary, but low because SATD scores are 1/4 normal
    a->i_lambda = x264_lambda_tab[ a->i_qp ];
    if( x264_mb_analyse_load_costs( h, a ) )
        return -1;
    h->mb.i_me_method = X264_MIN( X264_ME_HEX, h->param.analyse.i_me_method ); // maybe dia?
    h->mb.i_subpel_refine = 4; // 3 should be enough, but not tweaking for speed now
    h->mb.b_chroma_me = 0;
    return 0;
}

int x264_lowres_context_alloc( x264_t *h )
{
    x264_mb_analysis_t a;
    return x264_lowres_context_init( h, &a );
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

    ALIGNED_8( uint8_t pix1[9*FDEC_STRIDE] );
    uint8_t *pix2 = pix1+8;
    x264_me_t m[2];
    int i_bcost = COST_MAX;
    int l, i;
    int list_used = 0;

    h->mb.pic.p_fenc[0] = h->mb.pic.fenc_buf;
    h->mc.copy[PIXEL_8x8]( h->mb.pic.p_fenc[0], FENC_STRIDE, &fenc->lowres[0][i_pel_offset], i_stride, 8 );

    if( p0 == p1 )
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
        COPY2_IF_LT( i_bcost, i_cost, list_used, 3 ); \
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
        {
            int i_cost;
            h->mc.avg[PIXEL_8x8]( pix1, 16, m[0].p_fref[0], m[0].i_stride[0], m[1].p_fref[0], m[1].i_stride[0], i_bipred_weight );
            i_cost = h->pixf.mbcmp[PIXEL_8x8]( m[0].p_fenc[0], FENC_STRIDE, pix1, 16 );
            COPY2_IF_LT( i_bcost, i_cost, list_used, 3 );
        }
    }

    for( l = 0; l < 1 + b_bidir; l++ )
    {
        if( do_search[l] )
        {
            int i_mvc = 0;
            int16_t (*fenc_mv)[2] = fenc_mvs[l];
            ALIGNED_4( int16_t mvc[4][2] );

            /* Reverse-order MV prediction. */
            *(uint32_t*)mvc[0] = 0;
            *(uint32_t*)mvc[1] = 0;
            *(uint32_t*)mvc[2] = 0;
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
        COPY2_IF_LT( i_bcost, m[l].cost, list_used, l+1 );
    }

    if( b_bidir && ( *(uint32_t*)m[0].mv || *(uint32_t*)m[1].mv ) )
        TRY_BIDIR( m[0].mv, m[1].mv, 5 );

    /* Store to width-2 bitfield. */
    frames[b]->lowres_inter_types[b-p0][p1-b][i_mb_xy>>2] &= ~(3<<((i_mb_xy&3)*2));
    frames[b]->lowres_inter_types[b-p0][p1-b][i_mb_xy>>2] |= list_used<<((i_mb_xy&3)*2);

lowres_intra_mb:
    /* forbid intra-mbs in B-frames, because it's rare and not worth checking */
    /* FIXME: Should we still forbid them now that we cache intra scores? */
    if( !b_bidir || h->param.rc.b_mb_tree )
    {
        int i_icost, b_intra;
        if( !fenc->b_intra_calculated )
        {
            ALIGNED_ARRAY_16( uint8_t, edge,[33] );
            uint8_t *pix = &pix1[8+FDEC_STRIDE - 1];
            uint8_t *src = &fenc->lowres[0][i_pel_offset - 1];
            const int intra_penalty = 5;
            int satds[4];

            memcpy( pix-FDEC_STRIDE, src-i_stride, 17 );
            for( i=0; i<8; i++ )
                pix[i*FDEC_STRIDE] = src[i*i_stride];
            pix++;

            if( h->pixf.intra_mbcmp_x3_8x8c )
            {
                h->pixf.intra_mbcmp_x3_8x8c( h->mb.pic.p_fenc[0], pix, satds );
                h->predict_8x8c[I_PRED_CHROMA_P]( pix );
                satds[I_PRED_CHROMA_P] =
                    h->pixf.mbcmp[PIXEL_8x8]( pix, FDEC_STRIDE, h->mb.pic.p_fenc[0], FENC_STRIDE );
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

            h->predict_8x8_filter( pix, edge, ALL_NEIGHBORS, ALL_NEIGHBORS );
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
        if( !b_bidir )
        {
            b_intra = i_icost < i_bcost;
            if( b_intra )
                i_bcost = i_icost;
            if(   (i_mb_x > 0 && i_mb_x < h->sps->i_mb_width - 1
                && i_mb_y > 0 && i_mb_y < h->sps->i_mb_height - 1)
                || h->sps->i_mb_width <= 2 || h->sps->i_mb_height <= 2 )
            {
                fenc->i_intra_mbs[b-p0] += b_intra;
                fenc->i_cost_est[0][0] += i_icost;
                if( h->param.rc.i_aq_mode )
                    fenc->i_cost_est_aq[0][0] += (i_icost * fenc->i_inv_qscale_factor[i_mb_xy] + 128) >> 8;
            }
        }
    }

    fenc->lowres_costs[b-p0][p1-b][i_mb_xy] = i_bcost;

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
            frames[b]->i_cost_est_aq[0][0] = 0;
        }
        if( p1 != p0 )
            dist_scale_factor = ( ((b-p0) << 8) + ((p1-p0) >> 1) ) / (p1-p0);

        /* Lowres lookahead goes backwards because the MVs are used as predictors in the main encode.
         * This considerably improves MV prediction overall. */

        /* the edge mbs seem to reduce the predictive quality of the
         * whole frame's score, but are needed for a spatial distribution. */
        if( h->param.rc.b_mb_tree || h->param.rc.i_vbv_buffer_size ||
            h->sps->i_mb_width <= 2 || h->sps->i_mb_height <= 2 )
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

/* If MB-tree changes the quantizers, we need to recalculate the frame cost without
 * re-running lookahead. */
static int x264_slicetype_frame_cost_recalculate( x264_t *h, x264_frame_t **frames, int p0, int p1, int b )
{
    int i_score = 0;
    int *row_satd = frames[b]->i_row_satds[b-p0][p1-b];
    x264_emms();
    for( h->mb.i_mb_y = h->sps->i_mb_height - 1; h->mb.i_mb_y >= 0; h->mb.i_mb_y-- )
    {
        row_satd[ h->mb.i_mb_y ] = 0;
        for( h->mb.i_mb_x = h->sps->i_mb_width - 1; h->mb.i_mb_x >= 0; h->mb.i_mb_x-- )
        {
            int i_mb_xy = h->mb.i_mb_x + h->mb.i_mb_y*h->mb.i_mb_stride;
            int i_mb_cost = frames[b]->lowres_costs[b-p0][p1-b][i_mb_xy];
            float qp_adj = frames[b]->f_qp_offset[i_mb_xy];
            i_mb_cost = (i_mb_cost * x264_exp2fix8(qp_adj) + 128) >> 8;
            row_satd[ h->mb.i_mb_y ] += i_mb_cost;
            if( (h->mb.i_mb_y > 0 && h->mb.i_mb_y < h->sps->i_mb_height - 1 &&
                 h->mb.i_mb_x > 0 && h->mb.i_mb_x < h->sps->i_mb_width - 1) ||
                 h->sps->i_mb_width <= 2 || h->sps->i_mb_height <= 2 )
            {
                i_score += i_mb_cost;
            }
        }
    }
    return i_score;
}

static void x264_macroblock_tree_finish( x264_t *h, x264_frame_t *frame, int b_bidir )
{
    int mb_index;
    x264_emms();
    if( b_bidir )
        memcpy( frame->f_qp_offset, frame->f_qp_offset_aq, sizeof( frame->f_qp_offset ) );
    else
    {
        /* Allow the strength to be adjusted via qcompress, since the two
         * concepts are very similar. */
        float strength = 5.0f * (1.0f - h->param.rc.f_qcompress);
        for( mb_index = 0; mb_index < h->mb.i_mb_count; mb_index++ )
        {
            int intra_cost = (frame->i_intra_cost[mb_index] * frame->i_inv_qscale_factor[mb_index]+128)>>8;
            if( intra_cost )
            {
                int propagate_cost = frame->i_propagate_cost[mb_index];
                float log2_ratio = x264_log2(intra_cost + propagate_cost) - x264_log2(intra_cost);
                frame->f_qp_offset[mb_index] = frame->f_qp_offset_aq[mb_index] - strength * log2_ratio;
            }
        }
    }
}

static void x264_macroblock_tree_propagate( x264_t *h, x264_frame_t **frames, int p0, int p1, int b )
{
    uint16_t *ref_costs[2] = {frames[p0]->i_propagate_cost,frames[p1]->i_propagate_cost};
    int dist_scale_factor = ( ((b-p0) << 8) + ((p1-p0) >> 1) ) / (p1-p0);
    int i_bipred_weight = h->param.analyse.b_weighted_bipred ? 64 - (dist_scale_factor>>2) : 32;
    int16_t (*mvs[2])[2] = { frames[b]->lowres_mvs[0][b-p0-1], frames[b]->lowres_mvs[1][p1-b-1] };
    int bipred_weights[2] = {i_bipred_weight, 64 - i_bipred_weight};
    int *buf = h->scratch_buffer;

    for( h->mb.i_mb_y = 0; h->mb.i_mb_y < h->sps->i_mb_height; h->mb.i_mb_y++ )
    {
        int mb_index = h->mb.i_mb_y*h->mb.i_mb_stride;
        h->mc.mbtree_propagate_cost( buf, frames[b]->i_propagate_cost+mb_index,
            frames[b]->i_intra_cost+mb_index, frames[b]->lowres_costs[b-p0][p1-b]+mb_index,
            frames[b]->i_inv_qscale_factor+mb_index, h->sps->i_mb_width );
        for( h->mb.i_mb_x = 0; h->mb.i_mb_x < h->sps->i_mb_width; h->mb.i_mb_x++, mb_index++ )
        {
            int propagate_amount = buf[h->mb.i_mb_x];
            /* Don't propagate for an intra block. */
            if( propagate_amount > 0 )
            {
                /* Access width-2 bitfield. */
                int lists_used = (frames[b]->lowres_inter_types[b-p0][p1-b][mb_index>>2] >> ((mb_index&3)*2))&3;
                int list;
                /* Follow the MVs to the previous frame(s). */
                for( list = 0; list < 2; list++ )
                    if( (lists_used >> list)&1 )
                    {
                        int x = mvs[list][mb_index][0];
                        int y = mvs[list][mb_index][1];
                        int listamount = propagate_amount;
                        int mbx = (x>>5)+h->mb.i_mb_x;
                        int mby = (y>>5)+h->mb.i_mb_y;
                        int idx0 = mbx + mby*h->mb.i_mb_stride;
                        int idx1 = idx0 + 1;
                        int idx2 = idx0 + h->mb.i_mb_stride;
                        int idx3 = idx0 + h->mb.i_mb_stride + 1;
                        x &= 31;
                        y &= 31;
                        int idx0weight = (32-y)*(32-x);
                        int idx1weight = (32-y)*x;
                        int idx2weight = y*(32-x);
                        int idx3weight = y*x;

                        /* Apply bipred weighting. */
                        if( lists_used == 3 )
                            listamount = (listamount * bipred_weights[list] + 32) >> 6;

#define CLIP_ADD(s,x) (s) = X264_MIN((s)+(x),(1<<16)-1)

                        /* We could just clip the MVs, but pixels that lie outside the frame probably shouldn't
                         * be counted. */
                        if( mbx < h->sps->i_mb_width-1 && mby < h->sps->i_mb_height-1 && mbx >= 0 && mby >= 0 )
                        {
                            CLIP_ADD( ref_costs[list][idx0], (listamount*idx0weight+512)>>10 );
                            CLIP_ADD( ref_costs[list][idx1], (listamount*idx1weight+512)>>10 );
                            CLIP_ADD( ref_costs[list][idx2], (listamount*idx2weight+512)>>10 );
                            CLIP_ADD( ref_costs[list][idx3], (listamount*idx3weight+512)>>10 );
                        }
                        else /* Check offsets individually */
                        {
                            if( mbx < h->sps->i_mb_width && mby < h->sps->i_mb_height && mbx >= 0 && mby >= 0 )
                                CLIP_ADD( ref_costs[list][idx0], (listamount*idx0weight+512)>>10 );
                            if( mbx+1 < h->sps->i_mb_width && mby < h->sps->i_mb_height && mbx+1 >= 0 && mby >= 0 )
                                CLIP_ADD( ref_costs[list][idx1], (listamount*idx1weight+512)>>10 );
                            if( mbx < h->sps->i_mb_width && mby+1 < h->sps->i_mb_height && mbx >= 0 && mby+1 >= 0 )
                                CLIP_ADD( ref_costs[list][idx2], (listamount*idx2weight+512)>>10 );
                            if( mbx+1 < h->sps->i_mb_width && mby+1 < h->sps->i_mb_height && mbx+1 >= 0 && mby+1 >= 0 )
                                CLIP_ADD( ref_costs[list][idx3], (listamount*idx3weight+512)>>10 );
                        }
                    }
            }
        }
    }

    if( h->param.rc.i_vbv_buffer_size )
        x264_macroblock_tree_finish( h, frames[b], b != p1 );
}

static void x264_macroblock_tree( x264_t *h, x264_mb_analysis_t *a, x264_frame_t **frames, int num_frames, int b_intra )
{
    int i, idx = !b_intra;
    int last_nonb, cur_nonb = 1;
    if( b_intra )
        x264_slicetype_frame_cost( h, a, frames, 0, 0, 0, 0 );

    i = num_frames-1;
    while( i > 0 && frames[i]->i_type == X264_TYPE_B )
        i--;
    last_nonb = i;

    if( last_nonb < 0 )
        return;

    memset( frames[last_nonb]->i_propagate_cost, 0, h->mb.i_mb_count * sizeof(uint16_t) );
    while( i-- > idx )
    {
        cur_nonb = i;
        while( frames[cur_nonb]->i_type == X264_TYPE_B && cur_nonb > 0 )
            cur_nonb--;
        if( cur_nonb < idx )
            break;
        x264_slicetype_frame_cost( h, a, frames, cur_nonb, last_nonb, last_nonb, 0 );
        memset( frames[cur_nonb]->i_propagate_cost, 0, h->mb.i_mb_count * sizeof(uint16_t) );
        x264_macroblock_tree_propagate( h, frames, cur_nonb, last_nonb, last_nonb );
        while( frames[i]->i_type == X264_TYPE_B && i > 0 )
        {
            x264_slicetype_frame_cost( h, a, frames, cur_nonb, last_nonb, i, 0 );
            memset( frames[i]->i_propagate_cost, 0, h->mb.i_mb_count * sizeof(uint16_t) );
            x264_macroblock_tree_propagate( h, frames, cur_nonb, last_nonb, i );
            i--;
        }
        last_nonb = cur_nonb;
    }

    x264_macroblock_tree_finish( h, frames[last_nonb], 0 );
}

static int x264_vbv_frame_cost( x264_t *h, x264_mb_analysis_t *a, x264_frame_t **frames, int p0, int p1, int b )
{
    int cost = x264_slicetype_frame_cost( h, a, frames, p0, p1, b, 0 );
    if( h->param.rc.i_aq_mode )
    {
        if( h->param.rc.b_mb_tree )
            return x264_slicetype_frame_cost_recalculate( h, frames, p0, p1, b );
        else
            return frames[b]->i_cost_est_aq[b-p0][p1-b];
    }
    return cost;
}

static void x264_vbv_lookahead( x264_t *h, x264_mb_analysis_t *a, x264_frame_t **frames, int num_frames, int keyframe )
{
    int last_nonb = 0, cur_nonb = 1, next_nonb, i, idx = 0;
    while( cur_nonb < num_frames && frames[cur_nonb]->i_type == X264_TYPE_B )
        cur_nonb++;
    next_nonb = keyframe ? last_nonb : cur_nonb;

    while( cur_nonb <= num_frames )
    {
        /* P/I cost: This shouldn't include the cost of next_nonb */
        if( next_nonb != cur_nonb )
        {
            int p0 = IS_X264_TYPE_I( frames[cur_nonb]->i_type ) ? cur_nonb : last_nonb;
            frames[next_nonb]->i_planned_satd[idx] = x264_vbv_frame_cost( h, a, frames, p0, cur_nonb, cur_nonb );
            frames[next_nonb]->i_planned_type[idx] = frames[cur_nonb]->i_type;
            idx++;
        }
        /* Handle the B-frames: coded order */
        for( i = last_nonb+1; i < cur_nonb; i++, idx++ )
        {
            frames[next_nonb]->i_planned_satd[idx] = x264_vbv_frame_cost( h, a, frames, last_nonb, cur_nonb, i );
            frames[next_nonb]->i_planned_type[idx] = X264_TYPE_B;
        }
        last_nonb = cur_nonb;
        cur_nonb++;
        while( cur_nonb <= num_frames && frames[cur_nonb]->i_type == X264_TYPE_B )
            cur_nonb++;
    }
    frames[next_nonb]->i_planned_type[idx] = X264_TYPE_AUTO;
}

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
static void x264_slicetype_path( x264_t *h, x264_mb_analysis_t *a, x264_frame_t **frames, int length, int max_bframes, char (*best_paths)[X264_LOOKAHEAD_MAX] )
{
    char paths[X264_BFRAME_MAX+2][X264_LOOKAHEAD_MAX] = {{0}};
    int num_paths = X264_MIN(max_bframes+1, length);
    int suffix_size, loc, path;
    int best_cost = COST_MAX;
    int best_path_index = 0;
    length = X264_MIN(length,X264_LOOKAHEAD_MAX);

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

static int scenecut( x264_t *h, x264_mb_analysis_t *a, x264_frame_t **frames, int p0, int p1 )
{
    x264_frame_t *frame = frames[p1];
    x264_slicetype_frame_cost( h, a, frames, p0, p1, p1, 0 );

    int icost = frame->i_cost_est[0][0];
    int pcost = frame->i_cost_est[p1-p0][0];
    float f_bias;
    int i_gop_size = frame->i_frame - h->lookahead->i_last_idr;
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

void x264_slicetype_analyse( x264_t *h, int keyframe )
{
    x264_mb_analysis_t a;
    x264_frame_t *frames[X264_LOOKAHEAD_MAX+3] = { NULL, };
    int num_frames, keyint_limit, idr_frame_type, i, j;
    int i_mb_count = NUM_MBS;
    int cost1p0, cost2p0, cost1b1, cost2p1;
    int i_max_search = X264_MIN( h->lookahead->next.i_size, X264_LOOKAHEAD_MAX );
    if( h->param.b_deterministic )
        i_max_search = X264_MIN( i_max_search, h->lookahead->i_slicetype_length + !keyframe );

    assert( h->frames.b_have_lowres );

    if( !h->lookahead->last_nonb )
        return;
    frames[0] = h->lookahead->last_nonb;
    for( j = 0; j < i_max_search && h->lookahead->next.list[j]->i_type == X264_TYPE_AUTO; j++ )
        frames[j+1] = h->lookahead->next.list[j];

    if( !j )
        return;

    keyint_limit = h->param.i_keyint_max - frames[0]->i_frame + h->lookahead->i_last_idr - 1;
    num_frames = X264_MIN( j, keyint_limit );

    x264_lowres_context_init( h, &a );
    idr_frame_type = frames[1]->i_frame - h->lookahead->i_last_idr >= h->param.i_keyint_min ? X264_TYPE_IDR : X264_TYPE_I;

    /* This is important psy-wise: if we have a non-scenecut keyframe,
     * there will be significant visual artifacts if the frames just before
     * go down in quality due to being referenced less, despite it being
     * more RD-optimal. */
    if( (h->param.analyse.b_psy && h->param.rc.b_mb_tree) || h->param.rc.i_vbv_buffer_size )
        num_frames = j;
    else if( num_frames == 1 )
    {
        frames[1]->i_type = X264_TYPE_P;
        if( h->param.i_scenecut_threshold && scenecut( h, &a, frames, 0, 1 ) )
            frames[1]->i_type = idr_frame_type;
        return;
    }
    else if( num_frames == 0 )
    {
        frames[1]->i_type = idr_frame_type;
        return;
    }

    char best_paths[X264_LOOKAHEAD_MAX][X264_LOOKAHEAD_MAX] = {"","P"};
    int n;
    int num_bframes = 0;
    int max_bframes = X264_MIN(num_frames-1, h->param.i_bframe);
    int num_analysed_frames = num_frames;
    int reset_start;
    if( h->param.i_scenecut_threshold && scenecut( h, &a, frames, 0, 1 ) )
    {
        frames[1]->i_type = idr_frame_type;
        return;
    }

    if( h->param.i_bframe )
    {
        if( h->param.i_bframe_adaptive == X264_B_ADAPT_TRELLIS )
        {
            /* Perform the frametype analysis. */
            for( n = 2; n < num_frames-1; n++ )
                x264_slicetype_path( h, &a, frames, n, max_bframes, best_paths );
            if( num_frames > 1 )
            {
                num_bframes = strspn( best_paths[num_frames-2], "B" );
                /* Load the results of the analysis into the frame types. */
                for( j = 1; j < num_frames; j++ )
                    frames[j]->i_type = best_paths[num_frames-2][j-1] == 'B' ? X264_TYPE_B : X264_TYPE_P;
            }
            frames[num_frames]->i_type = X264_TYPE_P;
        }
        else if( h->param.i_bframe_adaptive == X264_B_ADAPT_FAST )
        {
            for( i = 0; i < num_frames-(2-!i); )
            {
                cost2p1 = x264_slicetype_frame_cost( h, &a, frames, i+0, i+2, i+2, 1 );
                if( frames[i+2]->i_intra_mbs[2] > i_mb_count / 2 )
                {
                    frames[i+1]->i_type = X264_TYPE_P;
                    frames[i+2]->i_type = X264_TYPE_P;
                    i += 2;
                    continue;
                }

                cost1b1 = x264_slicetype_frame_cost( h, &a, frames, i+0, i+2, i+1, 0 );
                cost1p0 = x264_slicetype_frame_cost( h, &a, frames, i+0, i+1, i+1, 0 );
                cost2p0 = x264_slicetype_frame_cost( h, &a, frames, i+1, i+2, i+2, 0 );

                if( cost1p0 + cost2p0 < cost1b1 + cost2p1 )
                {
                    frames[i+1]->i_type = X264_TYPE_P;
                    frames[i+2]->i_type = X264_TYPE_P;
                    i += 2;
                    continue;
                }

                // arbitrary and untuned
                #define INTER_THRESH 300
                #define P_SENS_BIAS (50 - h->param.i_bframe_bias)
                frames[i+1]->i_type = X264_TYPE_B;
                frames[i+2]->i_type = X264_TYPE_P;

                for( j = i+2; j <= X264_MIN( h->param.i_bframe, num_frames-1 ); j++ )
                {
                    int pthresh = X264_MAX(INTER_THRESH - P_SENS_BIAS * (j-i-1), INTER_THRESH/10);
                    int pcost = x264_slicetype_frame_cost( h, &a, frames, i+0, j+1, j+1, 1 );

                    if( pcost > pthresh*i_mb_count || frames[j+1]->i_intra_mbs[j-i+1] > i_mb_count/3 )
                    {
                        frames[j]->i_type = X264_TYPE_P;
                        break;
                    }
                    else
                        frames[j]->i_type = X264_TYPE_B;
                }
                i = j;
            }
            frames[i+!i]->i_type = X264_TYPE_P;
            num_bframes = 0;
            while( num_bframes < num_frames && frames[num_bframes+1]->i_type == X264_TYPE_B )
                num_bframes++;
        }
        else
        {
            num_bframes = X264_MIN(num_frames-1, h->param.i_bframe);
            for( j = 1; j < num_frames; j++ )
                frames[j]->i_type = (j%(num_bframes+1)) ? X264_TYPE_B : X264_TYPE_P;
            frames[num_frames]->i_type = X264_TYPE_P;
        }

        /* Check scenecut on the first minigop. */
        for( j = 1; j < num_bframes+1; j++ )
            if( h->param.i_scenecut_threshold && scenecut( h, &a, frames, j, j+1 ) )
            {
                frames[j]->i_type = X264_TYPE_P;
                num_analysed_frames = j;
                break;
            }

        reset_start = keyframe ? 1 : X264_MIN( num_bframes+2, num_analysed_frames+1 );
    }
    else
    {
        for( j = 1; j < num_frames; j++ )
            frames[j]->i_type = X264_TYPE_P;
        reset_start = !keyframe + 1;
        num_bframes = 0;
    }

    for( j = 1; j <= num_frames; j++ )
        if( frames[j]->i_type == X264_TYPE_AUTO )
            frames[j]->i_type = X264_TYPE_P;

    /* Perform the actual macroblock tree analysis.
     * Don't go farther than the maximum keyframe interval; this helps in short GOPs. */
    if( h->param.rc.b_mb_tree )
        x264_macroblock_tree( h, &a, frames, X264_MIN(num_frames, h->param.i_keyint_max), keyframe );

    /* Enforce keyframe limit. */
    for( j = 0; j < num_frames; j++ )
    {
        if( ((j-keyint_limit) % h->param.i_keyint_max) == 0 )
        {
            if( j && h->param.i_keyint_max > 1 )
                frames[j]->i_type = X264_TYPE_P;
            frames[j+1]->i_type = X264_TYPE_IDR;
            reset_start = X264_MIN( reset_start, j+2 );
        }
    }

    if( h->param.rc.i_vbv_buffer_size )
        x264_vbv_lookahead( h, &a, frames, num_frames, keyframe );

    /* Restore frametypes for all frames that haven't actually been decided yet. */
    for( j = reset_start; j <= num_frames; j++ )
        frames[j]->i_type = X264_TYPE_AUTO;
}

void x264_slicetype_decide( x264_t *h )
{
    x264_frame_t *frm;
    int bframes;
    int i;

    if( !h->lookahead->next.i_size )
        return;

    if( h->param.rc.b_stat_read )
    {
        /* Use the frame types from the first pass */
        for( i = 0; i < h->lookahead->next.i_size; i++ )
            h->lookahead->next.list[i]->i_type =
                x264_ratecontrol_slice_type( h, h->lookahead->next.list[i]->i_frame );
    }
    else if( (h->param.i_bframe && h->param.i_bframe_adaptive)
             || h->param.i_scenecut_threshold
             || h->param.rc.b_mb_tree
             || (h->param.rc.i_vbv_buffer_size && h->param.rc.i_lookahead) )
        x264_slicetype_analyse( h, 0 );

    for( bframes = 0;; bframes++ )
    {
        frm = h->lookahead->next.list[bframes];

        /* Limit GOP size */
        if( frm->i_frame - h->lookahead->i_last_idr >= h->param.i_keyint_max )
        {
            if( frm->i_type == X264_TYPE_AUTO )
                frm->i_type = X264_TYPE_IDR;
            if( frm->i_type != X264_TYPE_IDR )
                x264_log( h, X264_LOG_WARNING, "specified frame type (%d) is not compatible with keyframe interval\n", frm->i_type );
        }
        if( frm->i_type == X264_TYPE_IDR )
        {
            /* Close GOP */
            h->lookahead->i_last_idr = frm->i_frame;
            if( bframes > 0 )
            {
                bframes--;
                h->lookahead->next.list[bframes]->i_type = X264_TYPE_P;
            }
        }

        if( bframes == h->param.i_bframe ||
            !h->lookahead->next.list[bframes+1] )
        {
            if( IS_X264_TYPE_B( frm->i_type ) )
                x264_log( h, X264_LOG_WARNING, "specified frame type is not compatible with max B-frames\n" );
            if( frm->i_type == X264_TYPE_AUTO
                || IS_X264_TYPE_B( frm->i_type ) )
                frm->i_type = X264_TYPE_P;
        }

        if( frm->i_type == X264_TYPE_AUTO )
            frm->i_type = X264_TYPE_B;

        else if( !IS_X264_TYPE_B( frm->i_type ) ) break;
    }

    if( bframes )
        h->lookahead->next.list[bframes-1]->b_last_minigop_bframe = 1;
    h->lookahead->next.list[bframes]->i_bframes = bframes;

    /* calculate the frame costs ahead of time for x264_rc_analyse_slice while we still have lowres */
    if( h->param.rc.i_rc_method != X264_RC_CQP )
    {
        x264_mb_analysis_t a;
        x264_frame_t *frames[X264_BFRAME_MAX+2] = { NULL, };
        int p0=0, p1, b;

        x264_lowres_context_init( h, &a );

        if( IS_X264_TYPE_I( h->lookahead->next.list[bframes]->i_type ) )
            p1 = b = 0;
        else // P
            p1 = b = bframes + 1;
        frames[p0] = h->lookahead->last_nonb;
        frames[b] = h->lookahead->next.list[bframes];

        x264_slicetype_frame_cost( h, &a, frames, p0, p1, b, 0 );
    }
}

int x264_rc_analyse_slice( x264_t *h )
{
    x264_frame_t *frames[X264_BFRAME_MAX+2] = { NULL, };
    int p0=0, p1, b;
    int cost;

    if( IS_X264_TYPE_I(h->fenc->i_type) )
        p1 = b = 0;
    else // P
        p1 = b = h->fenc->i_bframes + 1;
    frames[p0] = h->fref0[0];
    frames[b] = h->fenc;

    /* cost should have been already calculated by x264_slicetype_decide */
    cost = frames[b]->i_cost_est[b-p0][p1-b];
    assert( cost >= 0 );

    if( h->param.rc.b_mb_tree && !h->param.rc.b_stat_read )
        cost = x264_slicetype_frame_cost_recalculate( h, frames, p0, p1, b );
    /* In AQ, use the weighted score instead. */
    else if( h->param.rc.i_aq_mode )
        cost = frames[b]->i_cost_est_aq[b-p0][p1-b];

    h->fenc->i_row_satd = h->fenc->i_row_satds[b-p0][p1-b];
    h->fdec->i_row_satd = h->fdec->i_row_satds[b-p0][p1-b];
    h->fdec->i_satd = cost;
    memcpy( h->fdec->i_row_satd, h->fenc->i_row_satd, h->sps->i_mb_height * sizeof(int) );
    return cost;
}

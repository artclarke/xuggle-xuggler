/*****************************************************************************
 * analyse.c: h264 encoder library
 *****************************************************************************
 * Copyright (C) 2003 x264 project
 * $Id: analyse.c,v 1.1 2004/06/03 19:27:08 fenrir Exp $
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111, USA.
 *****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <limits.h>

#include "common/common.h"
#include "common/macroblock.h"
#include "macroblock.h"
#include "me.h"
#include "ratecontrol.h"
#include "analyse.h"
#include "rdo.c"

typedef struct
{
    /* 16x16 */
    int i_ref;
    x264_me_t me16x16;

    /* 8x8 */
    int       i_cost8x8;
    x264_me_t me8x8[4];

    /* Sub 4x4 */
    int       i_cost4x4[4]; /* cost per 8x8 partition */
    x264_me_t me4x4[4][4];

    /* Sub 8x4 */
    int       i_cost8x4[4]; /* cost per 8x8 partition */
    x264_me_t me8x4[4][2];

    /* Sub 4x8 */
    int       i_cost4x8[4]; /* cost per 8x8 partition */
    x264_me_t me4x8[4][4];

    /* 16x8 */
    int       i_cost16x8;
    x264_me_t me16x8[2];

    /* 8x16 */
    int       i_cost8x16;
    x264_me_t me8x16[2];

} x264_mb_analysis_list_t;

typedef struct
{
    /* conduct the analysis using this lamda and QP */
    int i_lambda;
    int i_lambda2;
    int i_qp;
    int16_t *p_cost_mv;
    int b_mbrd;


    /* I: Intra part */
    /* Take some shortcuts in intra search if intra is deemed unlikely */
    int b_fast_intra;
    int i_best_satd;

    /* Luma part */
    int i_sad_i16x16;
    int i_predict16x16;

    int i_sad_i8x8;
    int i_predict8x8[2][2];

    int i_sad_i4x4;
    int i_predict4x4[4][4];

    /* Chroma part */
    int i_sad_i8x8chroma;
    int i_predict8x8chroma;

    /* II: Inter part P/B frame */
    x264_mb_analysis_list_t l0;
    x264_mb_analysis_list_t l1;

    int i_cost16x16bi; /* used the same ref and mv as l0 and l1 (at least for now) */
    int i_cost16x16direct;
    int i_cost8x8bi;
    int i_cost8x8direct[4];
    int i_cost16x8bi;
    int i_cost8x16bi;

    int i_mb_partition16x8[2]; /* mb_partition_e */
    int i_mb_partition8x16[2];
    int i_mb_type16x8; /* mb_class_e */
    int i_mb_type8x16;

    int b_direct_available;

} x264_mb_analysis_t;

/* lambda = pow(2,qp/6-2) */
static const int i_qp0_cost_table[52] = {
   1, 1, 1, 1, 1, 1, 1, 1,  /*  0-7 */
   1, 1, 1, 1,              /*  8-11 */
   1, 1, 1, 1, 2, 2, 2, 2,  /* 12-19 */
   3, 3, 3, 4, 4, 4, 5, 6,  /* 20-27 */
   6, 7, 8, 9,10,11,13,14,  /* 28-35 */
  16,18,20,23,25,29,32,36,  /* 36-43 */
  40,45,51,57,64,72,81,91   /* 44-51 */
};

/* pow(lambda,2) * .9 */
static const int i_qp0_cost2_table[52] = {
   1,   1,   1,   1,   1,   1, /*  0-5  */
   1,   1,   1,   1,   1,   1, /*  6-11 */
   1,   1,   1,   2,   2,   3, /* 12-17 */
   4,   5,   6,   7,   9,  11, /* 18-23 */
  14,  18,  23,  29,  36,  46, /* 24-29 */
  58,  73,  91, 115, 145, 183, /* 30-35 */
 230, 290, 366, 461, 581, 731, /* 36-41 */
 922,1161,1463,1843,2322,2926, /* 42-47 */
3686,4645,5852,7373
};

static const uint8_t block_idx_x[16] = {
    0, 1, 0, 1, 2, 3, 2, 3, 0, 1, 0, 1, 2, 3, 2, 3
};
static const uint8_t block_idx_y[16] = {
    0, 0, 1, 1, 0, 0, 1, 1, 2, 2, 3, 3, 2, 2, 3, 3
};

/* TODO: calculate CABAC costs */
static const int i_mb_b_cost_table[19] = {
    9, 9, 9, 9, 0, 0, 0, 1, 3, 7, 7, 7, 3, 7, 7, 7, 5, 9, 0
};
static const int i_mb_b16x8_cost_table[17] = {
    0, 0, 0, 0, 0, 0, 0, 0, 5, 7, 7, 7, 5, 7, 9, 9, 9
};
static const int i_sub_mb_b_cost_table[13] = {
    7, 5, 5, 3, 7, 5, 7, 3, 7, 7, 7, 5, 1
};
static const int i_sub_mb_p_cost_table[4] = {
    5, 3, 3, 1
};

static void x264_analyse_update_cache( x264_t *h, x264_mb_analysis_t *a );

/* initialize an array of lambda*nbits for all possible mvs */
static void x264_mb_analyse_load_costs( x264_t *h, x264_mb_analysis_t *a )
{
    static int16_t *p_cost_mv[52];

    if( !p_cost_mv[a->i_qp] )
    {
        /* could be faster, but isn't called many times */
        /* factor of 4 from qpel, 2 from sign, and 2 because mv can be opposite from mvp */
        int i;
        p_cost_mv[a->i_qp] = x264_malloc( (4*4*h->param.analyse.i_mv_range + 1) * sizeof(int16_t) );
        p_cost_mv[a->i_qp] += 2*4*h->param.analyse.i_mv_range;
        for( i = 0; i <= 2*4*h->param.analyse.i_mv_range; i++ )
        {
            p_cost_mv[a->i_qp][-i] =
            p_cost_mv[a->i_qp][i]  = a->i_lambda * bs_size_se( i );
        }
    }

    a->p_cost_mv = p_cost_mv[a->i_qp];
}

static void x264_mb_analyse_init( x264_t *h, x264_mb_analysis_t *a, int i_qp )
{
    memset( a, 0, sizeof( x264_mb_analysis_t ) );

    /* conduct the analysis using this lamda and QP */
    a->i_qp = i_qp;
    a->i_lambda = i_qp0_cost_table[i_qp];
    a->i_lambda2 = i_qp0_cost2_table[i_qp];
    a->b_mbrd = h->param.analyse.i_subpel_refine >= 6 && h->sh.i_type != SLICE_TYPE_B;

    h->mb.i_me_method = h->param.analyse.i_me_method;
    h->mb.i_subpel_refine = h->param.analyse.i_subpel_refine;
    h->mb.b_chroma_me = h->param.analyse.b_chroma_me && h->sh.i_type == SLICE_TYPE_P
                        && h->mb.i_subpel_refine >= 5;

    h->mb.b_transform_8x8 = 0;

    /* I: Intra part */
    a->i_sad_i16x16 =
    a->i_sad_i8x8   =
    a->i_sad_i4x4   =
    a->i_sad_i8x8chroma = COST_MAX;

    a->b_fast_intra = 0;
    a->i_best_satd = COST_MAX;

    /* II: Inter part P/B frame */
    if( h->sh.i_type != SLICE_TYPE_I )
    {
        int i;
        int i_fmv_range = h->param.analyse.i_mv_range - 16;

        /* Calculate max allowed MV range */
#define CLIP_FMV(mv) x264_clip3( mv, -i_fmv_range, i_fmv_range )
        h->mb.mv_min_fpel[0] = CLIP_FMV( -16*h->mb.i_mb_x - 8 );
        h->mb.mv_max_fpel[0] = CLIP_FMV( 16*( h->sps->i_mb_width - h->mb.i_mb_x ) - 8 );
        h->mb.mv_min[0] = 4*( h->mb.mv_min_fpel[0] - 16 );
        h->mb.mv_max[0] = 4*( h->mb.mv_max_fpel[0] + 16 );
        if( h->mb.i_mb_x == 0)
        {
            h->mb.mv_min_fpel[1] = CLIP_FMV( -16*h->mb.i_mb_y - 8 );
            h->mb.mv_max_fpel[1] = CLIP_FMV( 16*( h->sps->i_mb_height - h->mb.i_mb_y ) - 8 );
            h->mb.mv_min[1] = 4*( h->mb.mv_min_fpel[1] - 16 );
            h->mb.mv_max[1] = 4*( h->mb.mv_max_fpel[1] + 16 );
        }
#undef CLIP_FMV

        a->l0.me16x16.cost =
        a->l0.i_cost8x8    = COST_MAX;

        for( i = 0; i < 4; i++ )
        {
            a->l0.i_cost4x4[i] =
            a->l0.i_cost8x4[i] =
            a->l0.i_cost4x8[i] = COST_MAX;
        }

        a->l0.i_cost16x8   =
        a->l0.i_cost8x16   = COST_MAX;
        if( h->sh.i_type == SLICE_TYPE_B )
        {
            a->l1.me16x16.cost =
            a->l1.i_cost8x8    = COST_MAX;

            for( i = 0; i < 4; i++ )
            {
                a->l1.i_cost4x4[i] =
                a->l1.i_cost8x4[i] =
                a->l1.i_cost4x8[i] =
                a->i_cost8x8direct[i] = COST_MAX;
            }

            a->l1.i_cost16x8   =
            a->l1.i_cost8x16   =

            a->i_cost16x16bi   =
            a->i_cost16x16direct =
            a->i_cost8x8bi     =
            a->i_cost16x8bi    =
            a->i_cost8x16bi    = COST_MAX;
        }

        /* Fast intra decision */
        if( h->mb.i_mb_xy - h->sh.i_first_mb > 4 )
        {
            if( a->b_mbrd
               || IS_INTRA( h->mb.i_mb_type_left )
               || IS_INTRA( h->mb.i_mb_type_top )
               || IS_INTRA( h->mb.i_mb_type_topleft )
               || IS_INTRA( h->mb.i_mb_type_topright )
               || (h->sh.i_type == SLICE_TYPE_P && IS_INTRA( h->fref0[0]->mb_type[h->mb.i_mb_xy] ))
               || (h->mb.i_mb_xy - h->sh.i_first_mb < 3*(h->stat.frame.i_mb_count[I_4x4] + h->stat.frame.i_mb_count[I_8x8] + h->stat.frame.i_mb_count[I_16x16])) )
            { /* intra is likely */ }
            else
            {
                a->b_fast_intra = 1;
            }
        }
    }
}



/*
 * Handle intra mb
 */
/* Max = 4 */
static void predict_16x16_mode_available( unsigned int i_neighbour, int *mode, int *pi_count )
{
    if( i_neighbour & MB_TOPLEFT )
    {
        /* top and left avaible */
        *mode++ = I_PRED_16x16_V;
        *mode++ = I_PRED_16x16_H;
        *mode++ = I_PRED_16x16_DC;
        *mode++ = I_PRED_16x16_P;
        *pi_count = 4;
    }
    else if( i_neighbour & MB_LEFT )
    {
        /* left available*/
        *mode++ = I_PRED_16x16_DC_LEFT;
        *mode++ = I_PRED_16x16_H;
        *pi_count = 2;
    }
    else if( i_neighbour & MB_TOP )
    {
        /* top available*/
        *mode++ = I_PRED_16x16_DC_TOP;
        *mode++ = I_PRED_16x16_V;
        *pi_count = 2;
    }
    else
    {
        /* none avaible */
        *mode = I_PRED_16x16_DC_128;
        *pi_count = 1;
    }
}

/* Max = 4 */
static void predict_8x8chroma_mode_available( unsigned int i_neighbour, int *mode, int *pi_count )
{
    if( i_neighbour & MB_TOPLEFT )
    {
        /* top and left avaible */
        *mode++ = I_PRED_CHROMA_V;
        *mode++ = I_PRED_CHROMA_H;
        *mode++ = I_PRED_CHROMA_DC;
        *mode++ = I_PRED_CHROMA_P;
        *pi_count = 4;
    }
    else if( i_neighbour & MB_LEFT )
    {
        /* left available*/
        *mode++ = I_PRED_CHROMA_DC_LEFT;
        *mode++ = I_PRED_CHROMA_H;
        *pi_count = 2;
    }
    else if( i_neighbour & MB_TOP )
    {
        /* top available*/
        *mode++ = I_PRED_CHROMA_DC_TOP;
        *mode++ = I_PRED_CHROMA_V;
        *pi_count = 2;
    }
    else
    {
        /* none avaible */
        *mode = I_PRED_CHROMA_DC_128;
        *pi_count = 1;
    }
}

/* MAX = 9 */
static void predict_4x4_mode_available( unsigned int i_neighbour,
                                        int *mode, int *pi_count )
{
    /* FIXME even when b_tr == 0 there is some case where missing pixels
     * are emulated and thus more mode are available TODO
     * analysis and encode should be fixed too */
    int b_l = i_neighbour & MB_LEFT;
    int b_t = i_neighbour & MB_TOP;
    int b_tr = i_neighbour & MB_TOPRIGHT;

    if( b_l && b_t )
    {
        *mode++ = I_PRED_4x4_DC;
        *mode++ = I_PRED_4x4_H;
        *mode++ = I_PRED_4x4_V;
        *mode++ = I_PRED_4x4_DDR;
        *mode++ = I_PRED_4x4_VR;
        *mode++ = I_PRED_4x4_HD;
        *mode++ = I_PRED_4x4_HU;
        *pi_count = 7;
    }
    else if( b_l )
    {
        *mode++ = I_PRED_4x4_DC_LEFT;
        *mode++ = I_PRED_4x4_H;
        *mode++ = I_PRED_4x4_HU;
        *pi_count = 3;
    }
    else if( b_t )
    {
        *mode++ = I_PRED_4x4_DC_TOP;
        *mode++ = I_PRED_4x4_V;
        *pi_count = 2;
    }
    else
    {
        *mode++ = I_PRED_4x4_DC_128;
        *pi_count = 1;
    }

    if( b_t && b_tr )
    {
        *mode++ = I_PRED_4x4_DDL;
        *mode++ = I_PRED_4x4_VL;
        (*pi_count) += 2;
    }
}

static void x264_mb_analyse_intra_chroma( x264_t *h, x264_mb_analysis_t *a )
{
    int i;

    int i_max;
    int predict_mode[9];

    uint8_t *p_dstc[2], *p_srcc[2];
    int      i_stride[2];

    if( a->i_sad_i8x8chroma < COST_MAX )
        return;

    /* 8x8 prediction selection for chroma */
    p_dstc[0] = h->mb.pic.p_fdec[1];
    p_dstc[1] = h->mb.pic.p_fdec[2];
    p_srcc[0] = h->mb.pic.p_fenc[1];
    p_srcc[1] = h->mb.pic.p_fenc[2];

    i_stride[0] = h->mb.pic.i_stride[1];
    i_stride[1] = h->mb.pic.i_stride[2];

    predict_8x8chroma_mode_available( h->mb.i_neighbour, predict_mode, &i_max );
    a->i_sad_i8x8chroma = COST_MAX;
    for( i = 0; i < i_max; i++ )
    {
        int i_sad;
        int i_mode;

        i_mode = predict_mode[i];

        /* we do the prediction */
        h->predict_8x8c[i_mode]( p_dstc[0], i_stride[0] );
        h->predict_8x8c[i_mode]( p_dstc[1], i_stride[1] );

        /* we calculate the cost */
        i_sad = h->pixf.mbcmp[PIXEL_8x8]( p_dstc[0], i_stride[0],
                                          p_srcc[0], i_stride[0] ) +
                h->pixf.mbcmp[PIXEL_8x8]( p_dstc[1], i_stride[1],
                                          p_srcc[1], i_stride[1] ) +
                a->i_lambda * bs_size_ue( x264_mb_pred_mode8x8c_fix[i_mode] );

        /* if i_score is lower it is better */
        if( a->i_sad_i8x8chroma > i_sad )
        {
            a->i_predict8x8chroma = i_mode;
            a->i_sad_i8x8chroma   = i_sad;
        }
    }

    h->mb.i_chroma_pred_mode = a->i_predict8x8chroma;
}

static void x264_mb_analyse_intra( x264_t *h, x264_mb_analysis_t *a, int i_cost_inter )
{
    const unsigned int flags = h->sh.i_type == SLICE_TYPE_I ? h->param.analyse.intra : h->param.analyse.inter;
    const int i_stride = h->mb.pic.i_stride[0];
    uint8_t  *p_src = h->mb.pic.p_fenc[0];
    uint8_t  *p_dst = h->mb.pic.p_fdec[0];
    int      f8_satd_rd_ratio = 0;

    int i, idx;
    int i_max;
    int predict_mode[9];

    const int i_satd_thresh = a->i_best_satd * 5/4 + a->i_lambda * 10;

    /*---------------- Try all mode and calculate their score ---------------*/

    /* 16x16 prediction selection */
    predict_16x16_mode_available( h->mb.i_neighbour, predict_mode, &i_max );
    for( i = 0; i < i_max; i++ )
    {
        int i_sad;
        int i_mode;

        i_mode = predict_mode[i];
        h->predict_16x16[i_mode]( p_dst, i_stride );

        i_sad = h->pixf.mbcmp[PIXEL_16x16]( p_dst, i_stride, p_src, i_stride ) +
                a->i_lambda * bs_size_ue( x264_mb_pred_mode16x16_fix[i_mode] );
        if( a->i_sad_i16x16 > i_sad )
        {
            a->i_predict16x16 = i_mode;
            a->i_sad_i16x16   = i_sad;
        }
    }

    if( a->b_mbrd )
    {
        f8_satd_rd_ratio = ((unsigned)i_cost_inter << 8) / a->i_best_satd + 1;
        x264_mb_analyse_intra_chroma( h, a );
        if( h->mb.b_chroma_me )
            a->i_sad_i16x16 += a->i_sad_i8x8chroma;
        if( a->i_sad_i16x16 < i_satd_thresh )
        {
            h->mb.i_type = I_16x16;
            h->mb.i_intra16x16_pred_mode = a->i_predict16x16;
            a->i_sad_i16x16 = x264_rd_cost_mb( h, a->i_lambda2 );
        }
        else
            a->i_sad_i16x16 = a->i_sad_i16x16 * f8_satd_rd_ratio >> 8;
    }
    else
    {
        if( h->sh.i_type == SLICE_TYPE_B )
            /* cavlc mb type prefix */
            a->i_sad_i16x16 += a->i_lambda * i_mb_b_cost_table[I_16x16];
        if( a->b_fast_intra && a->i_sad_i16x16 > 2*i_cost_inter )
            return;
    }

    /* 4x4 prediction selection */
    if( flags & X264_ANALYSE_I4x4 )
    {
        a->i_sad_i4x4 = 0;
        for( idx = 0; idx < 16; idx++ )
        {
            uint8_t *p_src_by;
            uint8_t *p_dst_by;
            int     i_best;
            int x, y;
            int i_pred_mode;

            i_pred_mode= x264_mb_predict_intra4x4_mode( h, idx );
            x = block_idx_x[idx];
            y = block_idx_y[idx];

            p_src_by = p_src + 4 * x + 4 * y * i_stride;
            p_dst_by = p_dst + 4 * x + 4 * y * i_stride;

            i_best = COST_MAX;
            predict_4x4_mode_available( h->mb.i_neighbour4[idx], predict_mode, &i_max );
            for( i = 0; i < i_max; i++ )
            {
                int i_sad;
                int i_mode;

                i_mode = predict_mode[i];
                h->predict_4x4[i_mode]( p_dst_by, i_stride );

                i_sad = h->pixf.mbcmp[PIXEL_4x4]( p_dst_by, i_stride,
                                                  p_src_by, i_stride )
                      + a->i_lambda * (i_pred_mode == x264_mb_pred_mode4x4_fix(i_mode) ? 1 : 4);

                if( i_best > i_sad )
                {
                    a->i_predict4x4[x][y] = i_mode;
                    i_best = i_sad;
                }
            }
            a->i_sad_i4x4 += i_best;

            /* we need to encode this block now (for next ones) */
            h->predict_4x4[a->i_predict4x4[x][y]]( p_dst_by, i_stride );
            x264_mb_encode_i4x4( h, idx, a->i_qp );

            h->mb.cache.intra4x4_pred_mode[x264_scan8[idx]] = a->i_predict4x4[x][y];
        }

        a->i_sad_i4x4 += a->i_lambda * 24;    /* from JVT (SATD0) */
        if( a->b_mbrd )
        {
            if( h->mb.b_chroma_me )
                a->i_sad_i4x4 += a->i_sad_i8x8chroma;
            if( a->i_sad_i4x4 < i_satd_thresh )
            {
                h->mb.i_type = I_4x4;
                a->i_sad_i4x4 = x264_rd_cost_mb( h, a->i_lambda2 );
            }
            else
                a->i_sad_i4x4 = a->i_sad_i4x4 * f8_satd_rd_ratio >> 8;
        }
        else
        {
            if( h->sh.i_type == SLICE_TYPE_B )
                a->i_sad_i4x4 += a->i_lambda * i_mb_b_cost_table[I_4x4];
        }
    }

    /* 8x8 prediction selection */
    if( flags & X264_ANALYSE_I8x8 )
    {
        a->i_sad_i8x8 = 0;
        for( idx = 0; idx < 4; idx++ )
        {
            uint8_t *p_src_by;
            uint8_t *p_dst_by;
            int     i_best;
            int x, y;
            int i_pred_mode;

            i_pred_mode= x264_mb_predict_intra4x4_mode( h, 4*idx );
            x = idx&1;
            y = idx>>1;

            p_src_by = p_src + 8 * x + 8 * y * i_stride;
            p_dst_by = p_dst + 8 * x + 8 * y * i_stride;

            i_best = COST_MAX;
            predict_4x4_mode_available( h->mb.i_neighbour8[idx], predict_mode, &i_max );
            for( i = 0; i < i_max; i++ )
            {
                int i_sad;
                int i_mode;

                i_mode = predict_mode[i];
                h->predict_8x8[i_mode]( p_dst_by, i_stride, h->mb.i_neighbour );

                /* could use sa8d, but it doesn't seem worth the speed cost (without mmx at least) */
                i_sad = h->pixf.mbcmp[PIXEL_8x8]( p_dst_by, i_stride,
                                                  p_src_by, i_stride )
                      + a->i_lambda * (i_pred_mode == x264_mb_pred_mode4x4_fix(i_mode) ? 1 : 4);

                if( i_best > i_sad )
                {
                    a->i_predict8x8[x][y] = i_mode;
                    i_best = i_sad;
                }
            }
            a->i_sad_i8x8 += i_best;

            /* we need to encode this block now (for next ones) */
            h->predict_8x8[a->i_predict8x8[x][y]]( p_dst_by, i_stride, h->mb.i_neighbour );
            x264_mb_encode_i8x8( h, idx, a->i_qp );

            x264_macroblock_cache_intra8x8_pred( h, 2*x, 2*y, a->i_predict8x8[x][y] );
        }

        if( a->b_mbrd )
        {
            if( h->mb.b_chroma_me )
                a->i_sad_i8x8 += a->i_sad_i8x8chroma;
            if( a->i_sad_i8x8 < i_satd_thresh )
            {
                h->mb.i_type = I_8x8;
                a->i_sad_i8x8 = x264_rd_cost_mb( h, a->i_lambda2 );
            }
            else
                a->i_sad_i8x8 = a->i_sad_i8x8 * f8_satd_rd_ratio >> 8;
        }
        else
        {
            // FIXME some bias like in i4x4?
            if( h->sh.i_type == SLICE_TYPE_B )
                a->i_sad_i8x8 += a->i_lambda * i_mb_b_cost_table[I_8x8];
        }
    }
}

#define LOAD_FENC( m, src, xoff, yoff) \
    (m)->i_stride[0] = h->mb.pic.i_stride[0]; \
    (m)->i_stride[1] = h->mb.pic.i_stride[1]; \
    (m)->p_fenc[0] = &(src)[0][(xoff)+(yoff)*(m)->i_stride[0]]; \
    (m)->p_fenc[1] = &(src)[1][((xoff)>>1)+((yoff)>>1)*(m)->i_stride[1]]; \
    (m)->p_fenc[2] = &(src)[2][((xoff)>>1)+((yoff)>>1)*(m)->i_stride[1]];
#define LOAD_HPELS(m, src, xoff, yoff) \
    (m)->p_fref[0] = &(src)[0][(xoff)+(yoff)*(m)->i_stride[0]]; \
    (m)->p_fref[1] = &(src)[1][(xoff)+(yoff)*(m)->i_stride[0]]; \
    (m)->p_fref[2] = &(src)[2][(xoff)+(yoff)*(m)->i_stride[0]]; \
    (m)->p_fref[3] = &(src)[3][(xoff)+(yoff)*(m)->i_stride[0]]; \
    (m)->p_fref[4] = &(src)[4][((xoff)>>1)+((yoff)>>1)*(m)->i_stride[1]]; \
    (m)->p_fref[5] = &(src)[5][((xoff)>>1)+((yoff)>>1)*(m)->i_stride[1]];

static void x264_mb_analyse_inter_p16x16( x264_t *h, x264_mb_analysis_t *a )
{
    x264_me_t m;
    int i_ref;
    int mvc[4][2], i_mvc;
    int i_fullpel_thresh = INT_MAX;
    int *p_fullpel_thresh = h->i_ref0>1 ? &i_fullpel_thresh : NULL;

    /* 16x16 Search on all ref frame */
    m.i_pixel = PIXEL_16x16;
    m.p_cost_mv = a->p_cost_mv;
    LOAD_FENC( &m, h->mb.pic.p_fenc, 0, 0 );

    a->l0.me16x16.cost = INT_MAX;
    for( i_ref = 0; i_ref < h->i_ref0; i_ref++ )
    {
        const int i_ref_cost = a->i_lambda * bs_size_te( h->sh.i_num_ref_idx_l0_active - 1, i_ref );
        i_fullpel_thresh -= i_ref_cost;

        /* search with ref */
        LOAD_HPELS( &m, h->mb.pic.p_fref[0][i_ref], 0, 0 );
        x264_mb_predict_mv_16x16( h, 0, i_ref, m.mvp );
        x264_mb_predict_mv_ref16x16( h, 0, i_ref, mvc, &i_mvc );
        x264_me_search_ref( h, &m, mvc, i_mvc, p_fullpel_thresh );

        m.cost += i_ref_cost;
        i_fullpel_thresh += i_ref_cost;

        if( m.cost < a->l0.me16x16.cost )
        {
            a->l0.i_ref = i_ref;
            a->l0.me16x16 = m;
        }

        /* save mv for predicting neighbors */
        h->mb.mvr[0][i_ref][h->mb.i_mb_xy][0] = m.mv[0];
        h->mb.mvr[0][i_ref][h->mb.i_mb_xy][1] = m.mv[1];
    }

    /* Set global ref, needed for all others modes */
    x264_macroblock_cache_ref( h, 0, 0, 4, 4, 0, a->l0.i_ref );

    if( a->b_mbrd )
    {
        a->i_best_satd = a->l0.me16x16.cost;
        h->mb.i_type = P_L0;
        h->mb.i_partition = D_16x16;
        x264_macroblock_cache_mv ( h, 0, 0, 4, 4, 0, a->l0.me16x16.mv[0], a->l0.me16x16.mv[1] );
        a->l0.me16x16.cost = x264_rd_cost_mb( h, a->i_lambda2 );
    }
    else
    {
        /* subtract ref cost, so we don't have to add it for the other P types */
        a->l0.me16x16.cost -= a->i_lambda * bs_size_te( h->sh.i_num_ref_idx_l0_active - 1, a->l0.i_ref );
    }
}

static void x264_mb_analyse_inter_p8x8( x264_t *h, x264_mb_analysis_t *a )
{
    uint8_t  **p_fref = h->mb.pic.p_fref[0][a->l0.i_ref];
    uint8_t  **p_fenc = h->mb.pic.p_fenc;
    int mvc[5][2], i_mvc;
    int i;

    /* XXX Needed for x264_mb_predict_mv */
    h->mb.i_partition = D_8x8;

    i_mvc = 1;
    mvc[0][0] = a->l0.me16x16.mv[0];
    mvc[0][1] = a->l0.me16x16.mv[1];

    for( i = 0; i < 4; i++ )
    {
        x264_me_t *m = &a->l0.me8x8[i];
        const int x8 = i%2;
        const int y8 = i/2;

        m->i_pixel = PIXEL_8x8;
        m->p_cost_mv = a->p_cost_mv;

        LOAD_FENC( m, p_fenc, 8*x8, 8*y8 );
        LOAD_HPELS( m, p_fref, 8*x8, 8*y8 );

        x264_mb_predict_mv( h, 0, 4*i, 2, m->mvp );
        x264_me_search( h, m, mvc, i_mvc );

        x264_macroblock_cache_mv( h, 2*x8, 2*y8, 2, 2, 0, m->mv[0], m->mv[1] );

        mvc[i_mvc][0] = m->mv[0];
        mvc[i_mvc][1] = m->mv[1];
        i_mvc++;

        /* mb type cost */
        m->cost += a->i_lambda * i_sub_mb_p_cost_table[D_L0_8x8];
    }

    a->l0.i_cost8x8 = a->l0.me8x8[0].cost + a->l0.me8x8[1].cost +
                      a->l0.me8x8[2].cost + a->l0.me8x8[3].cost;
    if( a->b_mbrd )
    {
        if( a->i_best_satd > a->l0.i_cost8x8 )
            a->i_best_satd = a->l0.i_cost8x8;
        h->mb.i_type = P_8x8;
        h->mb.i_sub_partition[0] = h->mb.i_sub_partition[1] =
        h->mb.i_sub_partition[2] = h->mb.i_sub_partition[3] = D_L0_8x8;
        a->l0.i_cost8x8 = x264_rd_cost_mb( h, a->i_lambda2 );
    }
}

static void x264_mb_analyse_inter_p16x8( x264_t *h, x264_mb_analysis_t *a )
{
    uint8_t  **p_fref = h->mb.pic.p_fref[0][a->l0.i_ref];
    uint8_t  **p_fenc = h->mb.pic.p_fenc;
    int mvc[2][2];
    int i;

    /* XXX Needed for x264_mb_predict_mv */
    h->mb.i_partition = D_16x8;

    for( i = 0; i < 2; i++ )
    {
        x264_me_t *m = &a->l0.me16x8[i];

        m->i_pixel = PIXEL_16x8;
        m->p_cost_mv = a->p_cost_mv;

        LOAD_FENC( m, p_fenc, 0, 8*i );
        LOAD_HPELS( m, p_fref, 0, 8*i );

        mvc[0][0] = a->l0.me8x8[2*i].mv[0];
        mvc[0][1] = a->l0.me8x8[2*i].mv[1];
        mvc[1][0] = a->l0.me8x8[2*i+1].mv[0];
        mvc[1][1] = a->l0.me8x8[2*i+1].mv[1];

        x264_mb_predict_mv( h, 0, 8*i, 4, m->mvp );
        x264_me_search( h, m, mvc, 2 );

        x264_macroblock_cache_mv( h, 0, 2*i, 4, 2, 0, m->mv[0], m->mv[1] );
    }

    a->l0.i_cost16x8 = a->l0.me16x8[0].cost + a->l0.me16x8[1].cost;
    if( a->b_mbrd )
    {
        if( a->i_best_satd > a->l0.i_cost16x8 )
            a->i_best_satd = a->l0.i_cost16x8;
        h->mb.i_type = P_L0;
        a->l0.i_cost16x8 = x264_rd_cost_mb( h, a->i_lambda2 );
    }
}

static void x264_mb_analyse_inter_p8x16( x264_t *h, x264_mb_analysis_t *a )
{
    uint8_t  **p_fref = h->mb.pic.p_fref[0][a->l0.i_ref];
    uint8_t  **p_fenc = h->mb.pic.p_fenc;
    int mvc[2][2];
    int i;

    /* XXX Needed for x264_mb_predict_mv */
    h->mb.i_partition = D_8x16;

    for( i = 0; i < 2; i++ )
    {
        x264_me_t *m = &a->l0.me8x16[i];

        m->i_pixel = PIXEL_8x16;
        m->p_cost_mv = a->p_cost_mv;

        LOAD_FENC( m, p_fenc, 8*i, 0 );
        LOAD_HPELS( m, p_fref, 8*i, 0 );

        mvc[0][0] = a->l0.me8x8[i].mv[0];
        mvc[0][1] = a->l0.me8x8[i].mv[1];
        mvc[1][0] = a->l0.me8x8[i+2].mv[0];
        mvc[1][1] = a->l0.me8x8[i+2].mv[1];

        x264_mb_predict_mv( h, 0, 4*i, 2, m->mvp );
        x264_me_search( h, m, mvc, 2 );

        x264_macroblock_cache_mv( h, 2*i, 0, 2, 4, 0, m->mv[0], m->mv[1] );
    }

    a->l0.i_cost8x16 = a->l0.me8x16[0].cost + a->l0.me8x16[1].cost;
    if( a->b_mbrd )
    {
        if( a->i_best_satd > a->l0.i_cost8x16 )
            a->i_best_satd = a->l0.i_cost8x16;
        h->mb.i_type = P_L0;
        a->l0.i_cost8x16 = x264_rd_cost_mb( h, a->i_lambda2 );
    }
}

static int x264_mb_analyse_inter_p4x4_chroma( x264_t *h, x264_mb_analysis_t *a, uint8_t **p_fref, int i8x8, int pixel )
{
    uint8_t pix1[8*8], pix2[8*8];
    const int i_stride = h->mb.pic.i_stride[1];
    const int off = 4*(i8x8&1) + 2*(i8x8&2)*i_stride;

#define CHROMA4x4MC( width, height, me, x, y ) \
    h->mc.mc_chroma( &p_fref[4][off+x+y*i_stride], i_stride, &pix1[x+y*8], 8, (me).mv[0], (me).mv[1], width, height ); \
    h->mc.mc_chroma( &p_fref[5][off+x+y*i_stride], i_stride, &pix2[x+y*8], 8, (me).mv[0], (me).mv[1], width, height );

    if( pixel == PIXEL_4x4 )
    {
        CHROMA4x4MC( 2,2, a->l0.me4x4[i8x8][0], 0,0 );
        CHROMA4x4MC( 2,2, a->l0.me4x4[i8x8][1], 0,2 );
        CHROMA4x4MC( 2,2, a->l0.me4x4[i8x8][2], 2,0 );
        CHROMA4x4MC( 2,2, a->l0.me4x4[i8x8][3], 2,2 );
    }
    else if( pixel == PIXEL_8x4 )
    {
        CHROMA4x4MC( 4,2, a->l0.me8x4[i8x8][0], 0,0 );
        CHROMA4x4MC( 4,2, a->l0.me8x4[i8x8][1], 0,2 );
    }
    else
    {
        CHROMA4x4MC( 2,4, a->l0.me4x8[i8x8][0], 0,0 );
        CHROMA4x4MC( 2,4, a->l0.me4x8[i8x8][1], 2,0 );
    }

    return h->pixf.mbcmp[PIXEL_4x4]( &h->mb.pic.p_fenc[1][off], i_stride, pix1, 8 )
         + h->pixf.mbcmp[PIXEL_4x4]( &h->mb.pic.p_fenc[2][off], i_stride, pix2, 8 );
}

static void x264_mb_analyse_inter_p4x4( x264_t *h, x264_mb_analysis_t *a, int i8x8 )
{
    uint8_t  **p_fref = h->mb.pic.p_fref[0][a->l0.i_ref];
    uint8_t  **p_fenc = h->mb.pic.p_fenc;

    int i4x4;

    /* XXX Needed for x264_mb_predict_mv */
    h->mb.i_partition = D_8x8;

    for( i4x4 = 0; i4x4 < 4; i4x4++ )
    {
        const int idx = 4*i8x8 + i4x4;
        const int x4 = block_idx_x[idx];
        const int y4 = block_idx_y[idx];
        const int i_mvc = (i4x4 == 0);

        x264_me_t *m = &a->l0.me4x4[i8x8][i4x4];

        m->i_pixel = PIXEL_4x4;
        m->p_cost_mv = a->p_cost_mv;

        LOAD_FENC( m, p_fenc, 4*x4, 4*y4 );
        LOAD_HPELS( m, p_fref, 4*x4, 4*y4 );

        x264_mb_predict_mv( h, 0, idx, 1, m->mvp );
        x264_me_search( h, m, &a->l0.me8x8[i8x8].mv, i_mvc );

        x264_macroblock_cache_mv( h, x4, y4, 1, 1, 0, m->mv[0], m->mv[1] );
    }

    a->l0.i_cost4x4[i8x8] = a->l0.me4x4[i8x8][0].cost +
                         a->l0.me4x4[i8x8][1].cost +
                         a->l0.me4x4[i8x8][2].cost +
                         a->l0.me4x4[i8x8][3].cost +
                         a->i_lambda * i_sub_mb_p_cost_table[D_L0_4x4];
    if( h->mb.b_chroma_me )
        a->l0.i_cost4x4[i8x8] += x264_mb_analyse_inter_p4x4_chroma( h, a, p_fref, i8x8, PIXEL_4x4 );
}

static void x264_mb_analyse_inter_p8x4( x264_t *h, x264_mb_analysis_t *a, int i8x8 )
{
    uint8_t  **p_fref = h->mb.pic.p_fref[0][a->l0.i_ref];
    uint8_t  **p_fenc = h->mb.pic.p_fenc;

    int i8x4;

    /* XXX Needed for x264_mb_predict_mv */
    h->mb.i_partition = D_8x8;

    for( i8x4 = 0; i8x4 < 2; i8x4++ )
    {
        const int idx = 4*i8x8 + 2*i8x4;
        const int x4 = block_idx_x[idx];
        const int y4 = block_idx_y[idx];
        const int i_mvc = (i8x4 == 0);

        x264_me_t *m = &a->l0.me8x4[i8x8][i8x4];

        m->i_pixel = PIXEL_8x4;
        m->p_cost_mv = a->p_cost_mv;

        LOAD_FENC( m, p_fenc, 4*x4, 4*y4 );
        LOAD_HPELS( m, p_fref, 4*x4, 4*y4 );

        x264_mb_predict_mv( h, 0, idx, 2, m->mvp );
        x264_me_search( h, m, &a->l0.me4x4[i8x8][0].mv, i_mvc );

        x264_macroblock_cache_mv( h, x4, y4, 2, 1, 0, m->mv[0], m->mv[1] );
    }

    a->l0.i_cost8x4[i8x8] = a->l0.me8x4[i8x8][0].cost + a->l0.me8x4[i8x8][1].cost +
                            a->i_lambda * i_sub_mb_p_cost_table[D_L0_8x4];
    if( h->mb.b_chroma_me )
        a->l0.i_cost8x4[i8x8] += x264_mb_analyse_inter_p4x4_chroma( h, a, p_fref, i8x8, PIXEL_8x4 );
}

static void x264_mb_analyse_inter_p4x8( x264_t *h, x264_mb_analysis_t *a, int i8x8 )
{
    uint8_t  **p_fref = h->mb.pic.p_fref[0][a->l0.i_ref];
    uint8_t  **p_fenc = h->mb.pic.p_fenc;

    int i4x8;

    /* XXX Needed for x264_mb_predict_mv */
    h->mb.i_partition = D_8x8;

    for( i4x8 = 0; i4x8 < 2; i4x8++ )
    {
        const int idx = 4*i8x8 + i4x8;
        const int x4 = block_idx_x[idx];
        const int y4 = block_idx_y[idx];
        const int i_mvc = (i4x8 == 0);

        x264_me_t *m = &a->l0.me4x8[i8x8][i4x8];

        m->i_pixel = PIXEL_4x8;
        m->p_cost_mv = a->p_cost_mv;

        LOAD_FENC( m, p_fenc, 4*x4, 4*y4 );
        LOAD_HPELS( m, p_fref, 4*x4, 4*y4 );

        x264_mb_predict_mv( h, 0, idx, 1, m->mvp );
        x264_me_search( h, m, &a->l0.me4x4[i8x8][0].mv, i_mvc );

        x264_macroblock_cache_mv( h, x4, y4, 1, 2, 0, m->mv[0], m->mv[1] );
    }

    a->l0.i_cost4x8[i8x8] = a->l0.me4x8[i8x8][0].cost + a->l0.me4x8[i8x8][1].cost +
                            a->i_lambda * i_sub_mb_p_cost_table[D_L0_4x8];
    if( h->mb.b_chroma_me )
        a->l0.i_cost4x8[i8x8] += x264_mb_analyse_inter_p4x4_chroma( h, a, p_fref, i8x8, PIXEL_4x8 );
}

static void x264_mb_analyse_inter_direct( x264_t *h, x264_mb_analysis_t *a )
{
    /* Assumes that fdec still contains the results of
     * x264_mb_predict_mv_direct16x16 and x264_mb_mc */

    uint8_t **p_fenc = h->mb.pic.p_fenc;
    uint8_t **p_fdec = h->mb.pic.p_fdec;
    int i_stride= h->mb.pic.i_stride[0];
    int i;

    a->i_cost16x16direct = 0;
    for( i = 0; i < 4; i++ )
    {
        const int x8 = i%2;
        const int y8 = i/2;
        const int off = 8 * x8 + 8 * i_stride * y8;
        a->i_cost16x16direct +=
        a->i_cost8x8direct[i] =
            h->pixf.mbcmp[PIXEL_8x8]( &p_fenc[0][off], i_stride, &p_fdec[0][off], i_stride );

        /* mb type cost */
        a->i_cost8x8direct[i] += a->i_lambda * i_sub_mb_b_cost_table[D_DIRECT_8x8];
    }

    a->i_cost16x16direct += a->i_lambda * i_mb_b_cost_table[B_DIRECT];
}

#define WEIGHTED_AVG( size, pix1, stride1, src2, stride2 ) \
    { \
        if( h->param.analyse.b_weighted_bipred ) \
            h->pixf.avg_weight[size]( pix1, stride1, src2, stride2, \
                    h->mb.bipred_weight[a->l0.i_ref][a->l1.i_ref] ); \
        else \
            h->pixf.avg[size]( pix1, stride1, src2, stride2 ); \
    }

static void x264_mb_analyse_inter_b16x16( x264_t *h, x264_mb_analysis_t *a )
{
    uint8_t pix1[16*16], pix2[16*16];
    uint8_t *src2;
    int stride2 = 16;
    int src2_ref, pix1_ref;

    x264_me_t m;
    int i_ref;
    int mvc[5][2], i_mvc;
    int i_fullpel_thresh = INT_MAX;
    int *p_fullpel_thresh = h->i_ref0>1 ? &i_fullpel_thresh : NULL;

    /* 16x16 Search on all ref frame */
    m.i_pixel = PIXEL_16x16;
    m.p_cost_mv = a->p_cost_mv;
    LOAD_FENC( &m, h->mb.pic.p_fenc, 0, 0 );

    /* ME for List 0 */
    a->l0.me16x16.cost = INT_MAX;
    for( i_ref = 0; i_ref < h->i_ref0; i_ref++ )
    {
        /* search with ref */
        LOAD_HPELS( &m, h->mb.pic.p_fref[0][i_ref], 0, 0 );
        x264_mb_predict_mv_16x16( h, 0, i_ref, m.mvp );
        x264_mb_predict_mv_ref16x16( h, 0, i_ref, mvc, &i_mvc );
        x264_me_search_ref( h, &m, mvc, i_mvc, p_fullpel_thresh );

        /* add ref cost */
        m.cost += a->i_lambda * bs_size_te( h->sh.i_num_ref_idx_l0_active - 1, i_ref );

        if( m.cost < a->l0.me16x16.cost )
        {
            a->l0.i_ref = i_ref;
            a->l0.me16x16 = m;
        }

        /* save mv for predicting neighbors */
        h->mb.mvr[0][i_ref][h->mb.i_mb_xy][0] = m.mv[0];
        h->mb.mvr[0][i_ref][h->mb.i_mb_xy][1] = m.mv[1];
    }
    /* subtract ref cost, so we don't have to add it for the other MB types */
    a->l0.me16x16.cost -= a->i_lambda * bs_size_te( h->sh.i_num_ref_idx_l0_active - 1, a->l0.i_ref );

    /* ME for list 1 */
    i_fullpel_thresh = INT_MAX;
    p_fullpel_thresh = h->i_ref1>1 ? &i_fullpel_thresh : NULL;
    a->l1.me16x16.cost = INT_MAX;
    for( i_ref = 0; i_ref < h->i_ref1; i_ref++ )
    {
        /* search with ref */
        LOAD_HPELS( &m, h->mb.pic.p_fref[1][i_ref], 0, 0 );
        x264_mb_predict_mv_16x16( h, 1, i_ref, m.mvp );
        x264_mb_predict_mv_ref16x16( h, 1, i_ref, mvc, &i_mvc );
        x264_me_search_ref( h, &m, mvc, i_mvc, p_fullpel_thresh );

        /* add ref cost */
        m.cost += a->i_lambda * bs_size_te( h->sh.i_num_ref_idx_l1_active - 1, i_ref );

        if( m.cost < a->l1.me16x16.cost )
        {
            a->l1.i_ref = i_ref;
            a->l1.me16x16 = m;
        }

        /* save mv for predicting neighbors */
        h->mb.mvr[1][i_ref][h->mb.i_mb_xy][0] = m.mv[0];
        h->mb.mvr[1][i_ref][h->mb.i_mb_xy][1] = m.mv[1];
    }
    /* subtract ref cost, so we don't have to add it for the other MB types */
    a->l1.me16x16.cost -= a->i_lambda * bs_size_te( h->sh.i_num_ref_idx_l1_active - 1, a->l1.i_ref );

    /* Set global ref, needed for other modes? */
    x264_macroblock_cache_ref( h, 0, 0, 4, 4, 0, a->l0.i_ref );
    x264_macroblock_cache_ref( h, 0, 0, 4, 4, 1, a->l1.i_ref );

    /* get cost of BI mode */
    if ( ((a->l0.me16x16.mv[0] | a->l0.me16x16.mv[1]) & 1) == 0 )
    {
        /* l0 reference is halfpel, so get_ref on it will make it faster */
        src2 = h->mc.get_ref( h->mb.pic.p_fref[0][a->l0.i_ref], h->mb.pic.i_stride[0],
                        pix2, &stride2,
                        a->l0.me16x16.mv[0], a->l0.me16x16.mv[1],
                        16, 16 );
        h->mc.mc_luma( h->mb.pic.p_fref[1][a->l1.i_ref], h->mb.pic.i_stride[0],
                        pix1, 16,
                        a->l1.me16x16.mv[0], a->l1.me16x16.mv[1],
                        16, 16 );
        src2_ref = a->l0.i_ref;
        pix1_ref = a->l1.i_ref;
    } 
    else
    {
        /* if l0 was qpel, we'll use get_ref on l1 instead */
        h->mc.mc_luma( h->mb.pic.p_fref[0][a->l0.i_ref], h->mb.pic.i_stride[0],
                        pix1, 16,
                        a->l0.me16x16.mv[0], a->l0.me16x16.mv[1],
                        16, 16 );
        src2 = h->mc.get_ref( h->mb.pic.p_fref[1][a->l1.i_ref], h->mb.pic.i_stride[0],
                        pix2, &stride2,
                        a->l1.me16x16.mv[0], a->l1.me16x16.mv[1],
                        16, 16 );
        src2_ref = a->l1.i_ref;
        pix1_ref = a->l0.i_ref;
    }

    if( h->param.analyse.b_weighted_bipred )
        h->pixf.avg_weight[PIXEL_16x16]( pix1, 16, src2, stride2,
                h->mb.bipred_weight[pix1_ref][src2_ref] );
    else
        h->pixf.avg[PIXEL_16x16]( pix1, 16, src2, stride2 );

    a->i_cost16x16bi = h->pixf.mbcmp[PIXEL_16x16]( h->mb.pic.p_fenc[0], h->mb.pic.i_stride[0], pix1, 16 )
                     + a->i_lambda * ( bs_size_te( h->sh.i_num_ref_idx_l0_active - 1, a->l0.i_ref )
                                     + bs_size_te( h->sh.i_num_ref_idx_l1_active - 1, a->l1.i_ref ) )
                     + a->l0.me16x16.cost_mv
                     + a->l1.me16x16.cost_mv;

    /* mb type cost */
    a->i_cost16x16bi   += a->i_lambda * i_mb_b_cost_table[B_BI_BI];
    a->l0.me16x16.cost += a->i_lambda * i_mb_b_cost_table[B_L0_L0];
    a->l1.me16x16.cost += a->i_lambda * i_mb_b_cost_table[B_L1_L1];
}

static inline void x264_mb_cache_mv_p8x8( x264_t *h, x264_mb_analysis_t *a, int i )
{
    const int x = 2*(i%2);
    const int y = 2*(i/2);

    switch( h->mb.i_sub_partition[i] )
    {
        case D_L0_8x8:
            x264_macroblock_cache_mv( h, x, y, 2, 2, 0, a->l0.me8x8[i].mv[0], a->l0.me8x8[i].mv[1] );
            break;
        case D_L0_8x4:
            x264_macroblock_cache_mv( h, x, y+0, 2, 1, 0, a->l0.me8x4[i][0].mv[0], a->l0.me8x4[i][0].mv[1] );
            x264_macroblock_cache_mv( h, x, y+1, 2, 1, 0, a->l0.me8x4[i][1].mv[0], a->l0.me8x4[i][1].mv[1] );
            break;
        case D_L0_4x8:
            x264_macroblock_cache_mv( h, x+0, y, 1, 2, 0, a->l0.me4x8[i][0].mv[0], a->l0.me4x8[i][0].mv[1] );
            x264_macroblock_cache_mv( h, x+1, y, 1, 2, 0, a->l0.me4x8[i][1].mv[0], a->l0.me4x8[i][1].mv[1] );
            break;
        case D_L0_4x4:
            x264_macroblock_cache_mv( h, x+0, y+0, 1, 1, 0, a->l0.me4x4[i][0].mv[0], a->l0.me4x4[i][0].mv[1] );
            x264_macroblock_cache_mv( h, x+1, y+0, 1, 1, 0, a->l0.me4x4[i][1].mv[0], a->l0.me4x4[i][1].mv[1] );
            x264_macroblock_cache_mv( h, x+0, y+1, 1, 1, 0, a->l0.me4x4[i][2].mv[0], a->l0.me4x4[i][2].mv[1] );
            x264_macroblock_cache_mv( h, x+1, y+1, 1, 1, 0, a->l0.me4x4[i][3].mv[0], a->l0.me4x4[i][3].mv[1] );
            break;
        default:
            x264_log( h, X264_LOG_ERROR, "internal error\n" );
            break;
    }
}

#define CACHE_MV_BI(x,y,dx,dy,me0,me1,part) \
    if( x264_mb_partition_listX_table[0][part] ) \
    { \
        x264_macroblock_cache_ref( h, x,y,dx,dy, 0, a->l0.i_ref ); \
        x264_macroblock_cache_mv(  h, x,y,dx,dy, 0, me0.mv[0], me0.mv[1] ); \
    } \
    else \
    { \
        x264_macroblock_cache_ref( h, x,y,dx,dy, 0, -1 ); \
        x264_macroblock_cache_mv(  h, x,y,dx,dy, 0, 0, 0 ); \
        if( b_mvd ) \
            x264_macroblock_cache_mvd( h, x,y,dx,dy, 0, 0, 0 ); \
    } \
    if( x264_mb_partition_listX_table[1][part] ) \
    { \
        x264_macroblock_cache_ref( h, x,y,dx,dy, 1, a->l1.i_ref ); \
        x264_macroblock_cache_mv(  h, x,y,dx,dy, 1, me1.mv[0], me1.mv[1] ); \
    } \
    else \
    { \
        x264_macroblock_cache_ref( h, x,y,dx,dy, 1, -1 ); \
        x264_macroblock_cache_mv(  h, x,y,dx,dy, 1, 0, 0 ); \
        if( b_mvd ) \
            x264_macroblock_cache_mvd( h, x,y,dx,dy, 1, 0, 0 ); \
    }

static inline void x264_mb_cache_mv_b8x8( x264_t *h, x264_mb_analysis_t *a, int i, int b_mvd )
{
    int x = (i%2)*2;
    int y = (i/2)*2;
    if( h->mb.i_sub_partition[i] == D_DIRECT_8x8 )
    {
        x264_mb_load_mv_direct8x8( h, i );
        if( b_mvd )
        {
            x264_macroblock_cache_mvd(  h, x, y, 2, 2, 0, 0, 0 );
            x264_macroblock_cache_mvd(  h, x, y, 2, 2, 1, 0, 0 );
            x264_macroblock_cache_skip( h, x, y, 2, 2, 1 );
        }
    }
    else
    {
        CACHE_MV_BI( x, y, 2, 2, a->l0.me8x8[i], a->l1.me8x8[i], h->mb.i_sub_partition[i] );
    }
}
static inline void x264_mb_cache_mv_b16x8( x264_t *h, x264_mb_analysis_t *a, int i, int b_mvd )
{
    CACHE_MV_BI( 0, 2*i, 4, 2, a->l0.me16x8[i], a->l1.me16x8[i], a->i_mb_partition16x8[i] );
}
static inline void x264_mb_cache_mv_b8x16( x264_t *h, x264_mb_analysis_t *a, int i, int b_mvd )
{
    CACHE_MV_BI( 2*i, 0, 2, 4, a->l0.me8x16[i], a->l1.me8x16[i], a->i_mb_partition8x16[i] );
}
#undef CACHE_MV_BI

static void x264_mb_analyse_inter_b8x8( x264_t *h, x264_mb_analysis_t *a )
{
    uint8_t **p_fref[2] =
        { h->mb.pic.p_fref[0][a->l0.i_ref],
          h->mb.pic.p_fref[1][a->l1.i_ref] };
    uint8_t pix[2][8*8];
    int i, l;

    /* XXX Needed for x264_mb_predict_mv */
    h->mb.i_partition = D_8x8;

    a->i_cost8x8bi = 0;

    for( i = 0; i < 4; i++ )
    {
        const int x8 = i%2;
        const int y8 = i/2;
        int i_part_cost;
        int i_part_cost_bi = 0;

        for( l = 0; l < 2; l++ )
        {
            x264_mb_analysis_list_t *lX = l ? &a->l1 : &a->l0;
            x264_me_t *m = &lX->me8x8[i];

            m->i_pixel = PIXEL_8x8;
            m->p_cost_mv = a->p_cost_mv;

            LOAD_FENC( m, h->mb.pic.p_fenc, 8*x8, 8*y8 );
            LOAD_HPELS( m, p_fref[l], 8*x8, 8*y8 );

            x264_mb_predict_mv( h, l, 4*i, 2, m->mvp );
            x264_me_search( h, m, &lX->me16x16.mv, 1 );

            x264_macroblock_cache_mv( h, 2*x8, 2*y8, 2, 2, l, m->mv[0], m->mv[1] );

            /* BI mode */
            h->mc.mc_luma( m->p_fref, m->i_stride[0], pix[l], 8,
                            m->mv[0], m->mv[1], 8, 8 );
            i_part_cost_bi += m->cost_mv;
            /* FIXME: ref cost */
        }

        WEIGHTED_AVG( PIXEL_8x8, pix[0], 8, pix[1], 8 );
        i_part_cost_bi += h->pixf.mbcmp[PIXEL_8x8]( a->l0.me8x8[i].p_fenc[0], h->mb.pic.i_stride[0], pix[0], 8 )
                        + a->i_lambda * i_sub_mb_b_cost_table[D_BI_8x8];
        a->l0.me8x8[i].cost += a->i_lambda * i_sub_mb_b_cost_table[D_L0_8x8];
        a->l1.me8x8[i].cost += a->i_lambda * i_sub_mb_b_cost_table[D_L1_8x8];

        i_part_cost = a->l0.me8x8[i].cost;
        h->mb.i_sub_partition[i] = D_L0_8x8;
        if( a->l1.me8x8[i].cost < i_part_cost )
        {
            i_part_cost = a->l1.me8x8[i].cost;
            h->mb.i_sub_partition[i] = D_L1_8x8;
        }
        if( i_part_cost_bi < i_part_cost )
        {
            i_part_cost = i_part_cost_bi;
            h->mb.i_sub_partition[i] = D_BI_8x8;
        }
        if( a->i_cost8x8direct[i] < i_part_cost )
        {
            i_part_cost = a->i_cost8x8direct[i];
            h->mb.i_sub_partition[i] = D_DIRECT_8x8;
        }
        a->i_cost8x8bi += i_part_cost;

        /* XXX Needed for x264_mb_predict_mv */
        x264_mb_cache_mv_b8x8( h, a, i, 0 );
    }

    /* mb type cost */
    a->i_cost8x8bi += a->i_lambda * i_mb_b_cost_table[B_8x8];
}

static void x264_mb_analyse_inter_b16x8( x264_t *h, x264_mb_analysis_t *a )
{
    uint8_t **p_fref[2] =
        { h->mb.pic.p_fref[0][a->l0.i_ref],
          h->mb.pic.p_fref[1][a->l1.i_ref] };
    uint8_t pix[2][16*8];
    int mvc[2][2];
    int i, l;

    h->mb.i_partition = D_16x8;
    a->i_cost16x8bi = 0;

    for( i = 0; i < 2; i++ )
    {
        int i_part_cost;
        int i_part_cost_bi = 0;

        /* TODO: check only the list(s) that were used in b8x8? */
        for( l = 0; l < 2; l++ )
        {
            x264_mb_analysis_list_t *lX = l ? &a->l1 : &a->l0;
            x264_me_t *m = &lX->me16x8[i];

            m->i_pixel = PIXEL_16x8;
            m->p_cost_mv = a->p_cost_mv;

            LOAD_FENC( m, h->mb.pic.p_fenc, 0, 8*i );
            LOAD_HPELS( m, p_fref[l], 0, 8*i );

            mvc[0][0] = lX->me8x8[2*i].mv[0];
            mvc[0][1] = lX->me8x8[2*i].mv[1];
            mvc[1][0] = lX->me8x8[2*i+1].mv[0];
            mvc[1][1] = lX->me8x8[2*i+1].mv[1];

            x264_mb_predict_mv( h, 0, 8*i, 2, m->mvp );
            x264_me_search( h, m, mvc, 2 );

            /* BI mode */
            h->mc.mc_luma( m->p_fref, m->i_stride[0], pix[l], 16,
                            m->mv[0], m->mv[1], 16, 8 );
            /* FIXME: ref cost */
            i_part_cost_bi += m->cost_mv;
        }

        WEIGHTED_AVG( PIXEL_16x8, pix[0], 16, pix[1], 16 );
        i_part_cost_bi += h->pixf.mbcmp[PIXEL_16x8]( a->l0.me16x8[i].p_fenc[0], h->mb.pic.i_stride[0], pix[0], 16 );

        i_part_cost = a->l0.me16x8[i].cost;
        a->i_mb_partition16x8[i] = D_L0_8x8; /* not actually 8x8, only the L0 matters */
        if( a->l1.me16x8[i].cost < i_part_cost )
        {
            i_part_cost = a->l1.me16x8[i].cost;
            a->i_mb_partition16x8[i] = D_L1_8x8;
        }
        if( i_part_cost_bi + a->i_lambda * 1 < i_part_cost )
        {
            i_part_cost = i_part_cost_bi;
            a->i_mb_partition16x8[i] = D_BI_8x8;
        }
        a->i_cost16x8bi += i_part_cost;

        if( i == 0 )
            x264_mb_cache_mv_b16x8( h, a, i, 0 );
    }

    /* mb type cost */
    a->i_mb_type16x8 = B_L0_L0
        + (a->i_mb_partition16x8[0]>>2) * 3
        + (a->i_mb_partition16x8[1]>>2);
    a->i_cost16x8bi += a->i_lambda * i_mb_b16x8_cost_table[a->i_mb_type16x8];
}
static void x264_mb_analyse_inter_b8x16( x264_t *h, x264_mb_analysis_t *a )
{
    uint8_t **p_fref[2] =
        { h->mb.pic.p_fref[0][a->l0.i_ref],
          h->mb.pic.p_fref[1][a->l1.i_ref] };
    uint8_t pix[2][8*16];
    int mvc[2][2];
    int i, l;

    h->mb.i_partition = D_8x16;
    a->i_cost8x16bi = 0;

    for( i = 0; i < 2; i++ )
    {
        int i_part_cost;
        int i_part_cost_bi = 0;

        for( l = 0; l < 2; l++ )
        {
            x264_mb_analysis_list_t *lX = l ? &a->l1 : &a->l0;
            x264_me_t *m = &lX->me8x16[i];

            m->i_pixel = PIXEL_8x16;
            m->p_cost_mv = a->p_cost_mv;

            LOAD_FENC( m, h->mb.pic.p_fenc, 8*i, 0 );
            LOAD_HPELS( m, p_fref[l], 8*i, 0 );

            mvc[0][0] = lX->me8x8[i].mv[0];
            mvc[0][1] = lX->me8x8[i].mv[1];
            mvc[1][0] = lX->me8x8[i+2].mv[0];
            mvc[1][1] = lX->me8x8[i+2].mv[1];

            x264_mb_predict_mv( h, 0, 4*i, 2, m->mvp );
            x264_me_search( h, m, mvc, 2 );

            /* BI mode */
            h->mc.mc_luma( m->p_fref, m->i_stride[0], pix[l], 8,
                            m->mv[0], m->mv[1], 8, 16 );
            /* FIXME: ref cost */
            i_part_cost_bi += m->cost_mv;
        }

        WEIGHTED_AVG( PIXEL_8x16, pix[0], 8, pix[1], 8 );
        i_part_cost_bi += h->pixf.mbcmp[PIXEL_8x16]( a->l0.me8x16[i].p_fenc[0], h->mb.pic.i_stride[0], pix[0], 8 );

        i_part_cost = a->l0.me8x16[i].cost;
        a->i_mb_partition8x16[i] = D_L0_8x8;
        if( a->l1.me8x16[i].cost < i_part_cost )
        {
            i_part_cost = a->l1.me8x16[i].cost;
            a->i_mb_partition8x16[i] = D_L1_8x8;
        }
        if( i_part_cost_bi + a->i_lambda * 1 < i_part_cost )
        {
            i_part_cost = i_part_cost_bi;
            a->i_mb_partition8x16[i] = D_BI_8x8;
        }
        a->i_cost8x16bi += i_part_cost;

        if( i == 0 )
            x264_mb_cache_mv_b8x16( h, a, i, 0 );
    }

    /* mb type cost */
    a->i_mb_type8x16 = B_L0_L0
        + (a->i_mb_partition8x16[0]>>2) * 3
        + (a->i_mb_partition8x16[1]>>2);
    a->i_cost8x16bi += a->i_lambda * i_mb_b16x8_cost_table[a->i_mb_type8x16];
}

static inline void x264_mb_analyse_transform( x264_t *h )
{
    h->mb.cache.b_transform_8x8_allowed =
        h->param.analyse.b_transform_8x8
        && !IS_INTRA( h->mb.i_type ) && x264_mb_transform_8x8_allowed( h );

    if( h->mb.cache.b_transform_8x8_allowed )
    {
        int i_cost4, i_cost8;
        /* FIXME only luma mc is needed */
        x264_mb_mc( h );

        i_cost8 = h->pixf.sa8d[PIXEL_16x16]( h->mb.pic.p_fenc[0], h->mb.pic.i_stride[0],
                                             h->mb.pic.p_fdec[0], h->mb.pic.i_stride[0] );
        i_cost4 = h->pixf.satd[PIXEL_16x16]( h->mb.pic.p_fenc[0], h->mb.pic.i_stride[0],
                                             h->mb.pic.p_fdec[0], h->mb.pic.i_stride[0] );

        h->mb.b_transform_8x8 = i_cost8 < i_cost4;
    }
}

static inline void x264_mb_analyse_transform_rd( x264_t *h, x264_mb_analysis_t *a, int *i_cost )
{
    h->mb.cache.b_transform_8x8_allowed =
        h->param.analyse.b_transform_8x8 && x264_mb_transform_8x8_allowed( h );

    if( h->mb.cache.b_transform_8x8_allowed )
    {
        int i_cost8;
        x264_analyse_update_cache( h, a );
        h->mb.b_transform_8x8 = !h->mb.b_transform_8x8;
        /* FIXME only luma is needed, but the score for comparison already includes chroma */
        i_cost8 = x264_rd_cost_mb( h, a->i_lambda2 );

        if( *i_cost >= i_cost8 )
        {
            if( *i_cost > 0 )
                a->i_best_satd = (int64_t)a->i_best_satd * i_cost8 / *i_cost;
            *i_cost = i_cost8;
        }
        else
            h->mb.b_transform_8x8 = !h->mb.b_transform_8x8;
    }
}


/*****************************************************************************
 * x264_macroblock_analyse:
 *****************************************************************************/
void x264_macroblock_analyse( x264_t *h )
{
    x264_mb_analysis_t analysis;
    int i;

    h->mb.i_qp =
    h->mb.qp[h->mb.i_mb_xy] = x264_ratecontrol_qp( h );

    /* init analysis */
    x264_mb_analyse_init( h, &analysis, h->mb.qp[h->mb.i_mb_xy] );

    /*--------------------------- Do the analysis ---------------------------*/
    if( h->sh.i_type == SLICE_TYPE_I )
    {
        int i_cost;
        x264_mb_analyse_intra( h, &analysis, COST_MAX );

        i_cost = analysis.i_sad_i16x16;
        h->mb.i_type = I_16x16;
        if( analysis.i_sad_i4x4 < i_cost )
        {
            i_cost = analysis.i_sad_i4x4;
            h->mb.i_type = I_4x4;
        }
        if( analysis.i_sad_i8x8 < i_cost )
            h->mb.i_type = I_8x8;
    }
    else if( h->sh.i_type == SLICE_TYPE_P )
    {
        int b_skip = 0;
        int i_cost;
        int i_intra_cost, i_intra_type;

        /* Fast P_SKIP detection */
        if( !h->mb.b_lossless &&
           (( h->mb.i_mb_type_left == P_SKIP ) ||
            ( h->mb.i_mb_type_top == P_SKIP ) ||
            ( h->mb.i_mb_type_topleft == P_SKIP ) ||
            ( h->mb.i_mb_type_topright == P_SKIP )))
        {
            b_skip = x264_macroblock_probe_pskip( h );
        }

        if( b_skip )
        {
            h->mb.i_type = P_SKIP;
            h->mb.i_partition = D_16x16;
        }
        else
        {
            const unsigned int flags = h->param.analyse.inter;
            int i_type;
            int i_partition;
            int i_thresh16x8;

            x264_mb_analyse_load_costs( h, &analysis );

            x264_mb_analyse_inter_p16x16( h, &analysis );
            if( flags & X264_ANALYSE_PSUB16x16 )
                x264_mb_analyse_inter_p8x8( h, &analysis );

            /* Select best inter mode */
            i_type = P_L0;
            i_partition = D_16x16;
            i_cost = analysis.l0.me16x16.cost;

            if( ( flags & X264_ANALYSE_PSUB16x16 ) &&
                analysis.l0.i_cost8x8 < analysis.l0.me16x16.cost )
            {
                int i;

                i_type = P_8x8;
                i_partition = D_8x8;
                h->mb.i_sub_partition[0] = h->mb.i_sub_partition[1] =
                h->mb.i_sub_partition[2] = h->mb.i_sub_partition[3] = D_L0_8x8;

                i_cost = analysis.l0.i_cost8x8;

                /* Do sub 8x8 */
                if( flags & X264_ANALYSE_PSUB8x8 )
                {
                    int i_cost_bak = i_cost;
                    int b_sub8x8 = 0;
                    for( i = 0; i < 4; i++ )
                    {
                        x264_mb_analyse_inter_p4x4( h, &analysis, i );
                        if( analysis.l0.i_cost4x4[i] < analysis.l0.me8x8[i].cost )
                        {
                            int i_cost8x8 = analysis.l0.i_cost4x4[i];
                            h->mb.i_sub_partition[i] = D_L0_4x4;

                            x264_mb_analyse_inter_p8x4( h, &analysis, i );
                            if( analysis.l0.i_cost8x4[i] < i_cost8x8 )
                            {
                                h->mb.i_sub_partition[i] = D_L0_8x4;
                                i_cost8x8 = analysis.l0.i_cost8x4[i];
                            }

                            x264_mb_analyse_inter_p4x8( h, &analysis, i );
                            if( analysis.l0.i_cost4x8[i] < i_cost8x8 )
                            {
                                h->mb.i_sub_partition[i] = D_L0_4x8;
                                i_cost8x8 = analysis.l0.i_cost4x8[i];
                            }

                            i_cost += i_cost8x8 - analysis.l0.me8x8[i].cost;
                            b_sub8x8 = 1;
                        }
                        x264_mb_cache_mv_p8x8( h, &analysis, i );
                    }
                    /* TODO: RD per subpartition */
                    if( b_sub8x8 && analysis.b_mbrd )
                    {
                        i_cost = x264_rd_cost_mb( h, analysis.i_lambda2 );
                        if( i_cost > i_cost_bak )
                        {
                            i_cost = i_cost_bak;
                            h->mb.i_sub_partition[0] = h->mb.i_sub_partition[1] =
                            h->mb.i_sub_partition[2] = h->mb.i_sub_partition[3] = D_L0_8x8;
                        }
                    }
                }
            }

            /* Now do 16x8/8x16 */
            i_thresh16x8 = analysis.l0.me8x8[1].cost_mv + analysis.l0.me8x8[2].cost_mv;
            if( analysis.b_mbrd )
                i_thresh16x8 = i_thresh16x8 * analysis.i_lambda2 / analysis.i_lambda;
            if( ( flags & X264_ANALYSE_PSUB16x16 ) &&
                analysis.l0.i_cost8x8 < analysis.l0.me16x16.cost + i_thresh16x8 )
            {
                x264_mb_analyse_inter_p16x8( h, &analysis );
                if( analysis.l0.i_cost16x8 < i_cost )
                {
                    i_type = P_L0;
                    i_partition = D_16x8;
                    i_cost = analysis.l0.i_cost16x8;
                }

                x264_mb_analyse_inter_p8x16( h, &analysis );
                if( analysis.l0.i_cost8x16 < i_cost )
                {
                    i_type = P_L0;
                    i_partition = D_8x16;
                    i_cost = analysis.l0.i_cost8x16;
                }
            }

            h->mb.i_partition = i_partition;

            /* refine qpel */
            //FIXME mb_type costs?
            if( analysis.b_mbrd )
            {
                h->mb.i_type = i_type;
                x264_mb_analyse_transform_rd( h, &analysis, &i_cost );
            }
            else if( i_partition == D_16x16 )
            {
                x264_me_refine_qpel( h, &analysis.l0.me16x16 );
                i_cost = analysis.l0.me16x16.cost;
            }
            else if( i_partition == D_16x8 )
            {
                x264_me_refine_qpel( h, &analysis.l0.me16x8[0] );
                x264_me_refine_qpel( h, &analysis.l0.me16x8[1] );
                i_cost = analysis.l0.me16x8[0].cost + analysis.l0.me16x8[1].cost;
            }
            else if( i_partition == D_8x16 )
            {
                x264_me_refine_qpel( h, &analysis.l0.me8x16[0] );
                x264_me_refine_qpel( h, &analysis.l0.me8x16[1] );
                i_cost = analysis.l0.me8x16[0].cost + analysis.l0.me8x16[1].cost;
            }
            else if( i_partition == D_8x8 )
            {
                int i8x8;
                i_cost = 0;
                for( i8x8 = 0; i8x8 < 4; i8x8++ )
                {
                    switch( h->mb.i_sub_partition[i8x8] )
                    {
                        case D_L0_8x8:
                            x264_me_refine_qpel( h, &analysis.l0.me8x8[i8x8] );
                            i_cost += analysis.l0.me8x8[i8x8].cost;
                            break;
                        case D_L0_8x4:
                            x264_me_refine_qpel( h, &analysis.l0.me8x4[i8x8][0] );
                            x264_me_refine_qpel( h, &analysis.l0.me8x4[i8x8][1] );
                            i_cost += analysis.l0.me8x4[i8x8][0].cost +
                                      analysis.l0.me8x4[i8x8][1].cost;
                            break;
                        case D_L0_4x8:
                            x264_me_refine_qpel( h, &analysis.l0.me4x8[i8x8][0] );
                            x264_me_refine_qpel( h, &analysis.l0.me4x8[i8x8][1] );
                            i_cost += analysis.l0.me4x8[i8x8][0].cost +
                                      analysis.l0.me4x8[i8x8][1].cost;
                            break;

                        case D_L0_4x4:
                            x264_me_refine_qpel( h, &analysis.l0.me4x4[i8x8][0] );
                            x264_me_refine_qpel( h, &analysis.l0.me4x4[i8x8][1] );
                            x264_me_refine_qpel( h, &analysis.l0.me4x4[i8x8][2] );
                            x264_me_refine_qpel( h, &analysis.l0.me4x4[i8x8][3] );
                            i_cost += analysis.l0.me4x4[i8x8][0].cost +
                                      analysis.l0.me4x4[i8x8][1].cost +
                                      analysis.l0.me4x4[i8x8][2].cost +
                                      analysis.l0.me4x4[i8x8][3].cost;
                            break;
                        default:
                            x264_log( h, X264_LOG_ERROR, "internal error (!8x8 && !4x4)\n" );
                            break;
                    }
                }
            }

            x264_mb_analyse_intra( h, &analysis, i_cost );
            if( h->mb.b_chroma_me && !analysis.b_mbrd &&
                ( analysis.i_sad_i16x16 < i_cost
               || analysis.i_sad_i8x8 < i_cost
               || analysis.i_sad_i4x4 < i_cost ))
            {
                x264_mb_analyse_intra_chroma( h, &analysis );
                analysis.i_sad_i16x16 += analysis.i_sad_i8x8chroma;
                analysis.i_sad_i8x8 += analysis.i_sad_i8x8chroma;
                analysis.i_sad_i4x4 += analysis.i_sad_i8x8chroma;
            }

            i_intra_type = I_16x16;
            i_intra_cost = analysis.i_sad_i16x16;

            if( analysis.i_sad_i8x8 < i_intra_cost )
            {
                i_intra_type = I_8x8;
                i_intra_cost = analysis.i_sad_i8x8;
            }
            if( analysis.i_sad_i4x4 < i_intra_cost )
            {
                i_intra_type = I_4x4;
                i_intra_cost = analysis.i_sad_i4x4;
            }

            if( i_intra_cost < i_cost )
            {
                i_type = i_intra_type;
                i_cost = i_intra_cost;
            }

            h->mb.i_type = i_type;
            h->stat.frame.i_intra_cost += i_intra_cost;
            h->stat.frame.i_inter_cost += i_cost;
        }
    }
    else if( h->sh.i_type == SLICE_TYPE_B )
    {
        int b_skip = 0;

        analysis.b_direct_available = x264_mb_predict_mv_direct16x16( h );
        if( analysis.b_direct_available )
        {
            h->mb.i_type = B_SKIP;
            x264_mb_mc( h );

            /* Conditioning the probe on neighboring block types
             * doesn't seem to help speed or quality. */
            b_skip = !h->mb.b_lossless && x264_macroblock_probe_bskip( h );
        }

        if( !b_skip )
        {
            const unsigned int flags = h->param.analyse.inter;
            int i_partition;
            int i_cost;

            x264_mb_analyse_load_costs( h, &analysis );

            /* select best inter mode */
            /* direct must be first */
            if( analysis.b_direct_available )
                x264_mb_analyse_inter_direct( h, &analysis );

            x264_mb_analyse_inter_b16x16( h, &analysis );

            h->mb.i_type = B_L0_L0;
            i_partition = D_16x16;
            i_cost = analysis.l0.me16x16.cost;
            if( analysis.l1.me16x16.cost < i_cost )
            {
                h->mb.i_type = B_L1_L1;
                i_cost = analysis.l1.me16x16.cost;
            }
            if( analysis.i_cost16x16bi < i_cost )
            {
                h->mb.i_type = B_BI_BI;
                i_cost = analysis.i_cost16x16bi;
            }
            if( analysis.i_cost16x16direct < i_cost )
            {
                h->mb.i_type = B_DIRECT;
                i_cost = analysis.i_cost16x16direct;
            }
            
            if( flags & X264_ANALYSE_BSUB16x16 )
            {
                x264_mb_analyse_inter_b8x8( h, &analysis );
                if( analysis.i_cost8x8bi < i_cost )
                {
                    h->mb.i_type = B_8x8;
                    i_partition = D_8x8;
                    i_cost = analysis.i_cost8x8bi;

                    if( h->mb.i_sub_partition[0] == h->mb.i_sub_partition[1] ||
                        h->mb.i_sub_partition[2] == h->mb.i_sub_partition[3] )
                    {
                        x264_mb_analyse_inter_b16x8( h, &analysis );
                        if( analysis.i_cost16x8bi < i_cost )
                        {
                            i_partition = D_16x8;
                            i_cost = analysis.i_cost16x8bi;
                            h->mb.i_type = analysis.i_mb_type16x8;
                        }
                    }
                    if( h->mb.i_sub_partition[0] == h->mb.i_sub_partition[2] ||
                        h->mb.i_sub_partition[1] == h->mb.i_sub_partition[3] )
                    {
                        x264_mb_analyse_inter_b8x16( h, &analysis );
                        if( analysis.i_cost8x16bi < i_cost )
                        {
                            i_partition = D_8x16;
                            i_cost = analysis.i_cost8x16bi;
                            h->mb.i_type = analysis.i_mb_type8x16;
                        }
                    }
                }
            }

            h->mb.i_partition = i_partition;

            /* refine qpel */
            if( i_partition == D_16x16 )
            {
                analysis.l0.me16x16.cost -= analysis.i_lambda * i_mb_b_cost_table[B_L0_L0];
                analysis.l1.me16x16.cost -= analysis.i_lambda * i_mb_b_cost_table[B_L1_L1];
                if( h->mb.i_type == B_L0_L0 )
                {
                    x264_me_refine_qpel( h, &analysis.l0.me16x16 );
                    i_cost = analysis.l0.me16x16.cost
                           + analysis.i_lambda * i_mb_b_cost_table[B_L0_L0];
                }
                else if( h->mb.i_type == B_L1_L1 )
                {
                    x264_me_refine_qpel( h, &analysis.l1.me16x16 );
                    i_cost = analysis.l1.me16x16.cost
                           + analysis.i_lambda * i_mb_b_cost_table[B_L1_L1];
                }
                else if( h->mb.i_type == B_BI_BI )
                {
                    x264_me_refine_qpel( h, &analysis.l0.me16x16 );
                    x264_me_refine_qpel( h, &analysis.l1.me16x16 );
                }
            }
            else if( i_partition == D_16x8 )
            {
                for( i=0; i<2; i++ )
                {
                    if( analysis.i_mb_partition16x8[i] != D_L1_8x8 )
                        x264_me_refine_qpel( h, &analysis.l0.me16x8[i] );
                    if( analysis.i_mb_partition16x8[i] != D_L0_8x8 )
                        x264_me_refine_qpel( h, &analysis.l1.me16x8[i] );
                }
            }
            else if( i_partition == D_8x16 )
            {
                for( i=0; i<2; i++ )
                {
                    if( analysis.i_mb_partition8x16[i] != D_L1_8x8 )
                        x264_me_refine_qpel( h, &analysis.l0.me8x16[i] );
                    if( analysis.i_mb_partition8x16[i] != D_L0_8x8 )
                        x264_me_refine_qpel( h, &analysis.l1.me8x16[i] );
                }
            }
            else if( i_partition == D_8x8 )
            {
                for( i=0; i<4; i++ )
                {
                    x264_me_t *m;
                    int i_part_cost_old;
                    int i_type_cost;
                    int i_part_type = h->mb.i_sub_partition[i];
                    int b_bidir = (i_part_type == D_BI_8x8);

                    if( i_part_type == D_DIRECT_8x8 )
                        continue;
                    if( x264_mb_partition_listX_table[0][i_part_type] )
                    {
                        m = &analysis.l0.me8x8[i];
                        i_part_cost_old = m->cost;
                        i_type_cost = analysis.i_lambda * i_sub_mb_b_cost_table[D_L0_8x8];
                        m->cost -= i_type_cost;
                        x264_me_refine_qpel( h, m );
                        if( !b_bidir )
                            analysis.i_cost8x8bi += m->cost + i_type_cost - i_part_cost_old;
                    }
                    if( x264_mb_partition_listX_table[1][i_part_type] )
                    {
                        m = &analysis.l1.me8x8[i];
                        i_part_cost_old = m->cost;
                        i_type_cost = analysis.i_lambda * i_sub_mb_b_cost_table[D_L1_8x8];
                        m->cost -= i_type_cost;
                        x264_me_refine_qpel( h, m );
                        if( !b_bidir )
                            analysis.i_cost8x8bi += m->cost + i_type_cost - i_part_cost_old;
                    }
                    /* TODO: update mvp? */
                }
            }

            /* best intra mode */
            x264_mb_analyse_intra( h, &analysis, i_cost );

            if( analysis.i_sad_i16x16 < i_cost )
            {
                h->mb.i_type = I_16x16;
                i_cost = analysis.i_sad_i16x16;
            }
            if( analysis.i_sad_i8x8 < i_cost )
            {
                h->mb.i_type = I_8x8;
                i_cost = analysis.i_sad_i8x8;
            }
            if( analysis.i_sad_i4x4 < i_cost )
            {
                h->mb.i_type = I_4x4;
                i_cost = analysis.i_sad_i4x4;
            }
        }
    }

    x264_analyse_update_cache( h, &analysis );

    if( !analysis.b_mbrd )
        x264_mb_analyse_transform( h );
}

/*-------------------- Update MB from the analysis ----------------------*/
static void x264_analyse_update_cache( x264_t *h, x264_mb_analysis_t *a  )
{
    int i;

    switch( h->mb.i_type )
    {
        case I_4x4:
            for( i = 0; i < 16; i++ )
            {
                h->mb.cache.intra4x4_pred_mode[x264_scan8[i]] =
                    a->i_predict4x4[block_idx_x[i]][block_idx_y[i]];
            }

            x264_mb_analyse_intra_chroma( h, a );
            break;
        case I_8x8:
            for( i = 0; i < 4; i++ )
                x264_macroblock_cache_intra8x8_pred( h, 2*(i&1), 2*(i>>1),
                    a->i_predict8x8[i&1][i>>1] );

            x264_mb_analyse_intra_chroma( h, a );
            break;
        case I_16x16:
            h->mb.i_intra16x16_pred_mode = a->i_predict16x16;
            x264_mb_analyse_intra_chroma( h, a );
            break;

        case P_L0:
            x264_macroblock_cache_ref( h, 0, 0, 4, 4, 0, a->l0.i_ref );
            switch( h->mb.i_partition )
            {
                case D_16x16:
                    x264_macroblock_cache_mv ( h, 0, 0, 4, 4, 0, a->l0.me16x16.mv[0], a->l0.me16x16.mv[1] );
                    break;

                case D_16x8:
                    x264_macroblock_cache_mv ( h, 0, 0, 4, 2, 0, a->l0.me16x8[0].mv[0], a->l0.me16x8[0].mv[1] );
                    x264_macroblock_cache_mv ( h, 0, 2, 4, 2, 0, a->l0.me16x8[1].mv[0], a->l0.me16x8[1].mv[1] );
                    break;

                case D_8x16:
                    x264_macroblock_cache_mv ( h, 0, 0, 2, 4, 0, a->l0.me8x16[0].mv[0], a->l0.me8x16[0].mv[1] );
                    x264_macroblock_cache_mv ( h, 2, 0, 2, 4, 0, a->l0.me8x16[1].mv[0], a->l0.me8x16[1].mv[1] );
                    break;

                default:
                    x264_log( h, X264_LOG_ERROR, "internal error P_L0 and partition=%d\n", h->mb.i_partition );
                    break;
            }
            break;

        case P_8x8:
            x264_macroblock_cache_ref( h, 0, 0, 4, 4, 0, a->l0.i_ref );
            for( i = 0; i < 4; i++ )
                x264_mb_cache_mv_p8x8( h, a, i );
            break;

        case P_SKIP:
        {
            int mvp[2];
            x264_mb_predict_mv_pskip( h, mvp );
            /* */
            h->mb.i_partition = D_16x16;
            x264_macroblock_cache_ref( h, 0, 0, 4, 4, 0, 0 );
            x264_macroblock_cache_mv ( h, 0, 0, 4, 4, 0, mvp[0], mvp[1] );
            break;
        }

        case B_SKIP:
            /* nothing has changed since x264_macroblock_probe_bskip */
            break;
        case B_DIRECT:
            x264_mb_load_mv_direct8x8( h, 0 );
            x264_mb_load_mv_direct8x8( h, 1 );
            x264_mb_load_mv_direct8x8( h, 2 );
            x264_mb_load_mv_direct8x8( h, 3 );
            break;

        case B_8x8:
            /* optimize: cache might not need to be rewritten */
            for( i = 0; i < 4; i++ )
                x264_mb_cache_mv_b8x8( h, a, i, 1 );
            break;

        default: /* the rest of the B types */
            switch( h->mb.i_partition )
            {
            case D_16x16:
                switch( h->mb.i_type )
                {
                case B_L0_L0:
                    x264_macroblock_cache_ref( h, 0, 0, 4, 4, 0, a->l0.i_ref );
                    x264_macroblock_cache_mv ( h, 0, 0, 4, 4, 0, a->l0.me16x16.mv[0], a->l0.me16x16.mv[1] );

                    x264_macroblock_cache_ref( h, 0, 0, 4, 4, 1, -1 );
                    x264_macroblock_cache_mv ( h, 0, 0, 4, 4, 1,  0, 0 );
                    x264_macroblock_cache_mvd( h, 0, 0, 4, 4, 1,  0, 0 );
                    break;
                case B_L1_L1:
                    x264_macroblock_cache_ref( h, 0, 0, 4, 4, 0, -1 );
                    x264_macroblock_cache_mv ( h, 0, 0, 4, 4, 0,  0, 0 );
                    x264_macroblock_cache_mvd( h, 0, 0, 4, 4, 0,  0, 0 );

                    x264_macroblock_cache_ref( h, 0, 0, 4, 4, 1, a->l1.i_ref );
                    x264_macroblock_cache_mv ( h, 0, 0, 4, 4, 1, a->l1.me16x16.mv[0], a->l1.me16x16.mv[1] );
                    break;
                case B_BI_BI:
                    x264_macroblock_cache_ref( h, 0, 0, 4, 4, 0, a->l0.i_ref );
                    x264_macroblock_cache_mv ( h, 0, 0, 4, 4, 0, a->l0.me16x16.mv[0], a->l0.me16x16.mv[1] );

                    x264_macroblock_cache_ref( h, 0, 0, 4, 4, 1, a->l1.i_ref );
                    x264_macroblock_cache_mv ( h, 0, 0, 4, 4, 1, a->l1.me16x16.mv[0], a->l1.me16x16.mv[1] );
                    break;
                }
                break;
            case D_16x8:
                x264_mb_cache_mv_b16x8( h, a, 0, 1 );
                x264_mb_cache_mv_b16x8( h, a, 1, 1 );
                break;
            case D_8x16:
                x264_mb_cache_mv_b8x16( h, a, 0, 1 );
                x264_mb_cache_mv_b8x16( h, a, 1, 1 );
                break;
            default:
                x264_log( h, X264_LOG_ERROR, "internal error (invalid MB type)\n" );
                break;
            }
    }
}

#include "slicetype_decision.c"


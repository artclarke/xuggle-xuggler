/*****************************************************************************
 * analyse.c: h264 encoder library
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar
 * $Id: analyse.c,v 1.1 2004/06/03 19:27:08 fenrir Exp $
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

#include "../core/common.h"
#include "../core/macroblock.h"
#include "macroblock.h"
#include "me.h"

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
    int i_qp;


    /* I: Intra part */
    /* Luma part 16x16 and 4x4 modes stats */
    int i_sad_i16x16;
    int i_predict16x16;

    int i_sad_i4x4;
    int i_predict4x4[4][4];

    /* Chroma part */
    int i_sad_i8x8;
    int i_predict8x8;

    /* II: Inter part P/B frame */
    int i_mv_range;

    x264_mb_analysis_list_t l0;
    x264_mb_analysis_list_t l1;

    int i_cost16x16bi; /* used the same ref and mv as l0 and l1 (at least for now) */

} x264_mb_analysis_t;

static const int i_qp0_cost_table[52] = {
   1, 1, 1, 1, 1, 1, 1, 1,  /*  0-7 */
   1, 1, 1, 1,              /*  8-11 */
   1, 1, 1, 1, 2, 2, 2, 2,  /* 12-19 */
   3, 3, 3, 4, 4, 4, 5, 6,  /* 20-27 */
   6, 7, 8, 9,10,11,13,14,  /* 28-35 */
  16,18,20,23,25,29,32,36,  /* 36-43 */
  40,45,51,57,64,72,81,91   /* 44-51 */
};

static const uint8_t block_idx_x[16] = {
    0, 1, 0, 1, 2, 3, 2, 3, 0, 1, 0, 1, 2, 3, 2, 3
};
static const uint8_t block_idx_y[16] = {
    0, 0, 1, 1, 0, 0, 1, 1, 2, 2, 3, 3, 2, 2, 3, 3
};

static void x264_mb_analyse_init( x264_t *h, x264_mb_analysis_t *a, int i_qp )
{
    memset( a, 0, sizeof( x264_mb_analysis_t ) );

    /* conduct the analysis using this lamda and QP */
    a->i_qp = i_qp;
    a->i_lambda = i_qp0_cost_table[i_qp];

    /* I: Intra part */
    a->i_sad_i16x16 = -1;
    a->i_sad_i4x4   = -1;
    a->i_sad_i8x8   = -1;

    /* II: Inter part P/B frame */
    if( h->sh.i_type != SLICE_TYPE_I )
    {
        int dmb;
        int i;

        /* Calculate max start MV range */
        dmb = h->mb.i_mb_x;
        if( h->mb.i_mb_y < dmb )
            dmb = h->mb.i_mb_y;
        if( h->sps->i_mb_width - h->mb.i_mb_x < dmb )
            dmb = h->sps->i_mb_width - h->mb.i_mb_x;
        if( h->sps->i_mb_height - h->mb.i_mb_y < dmb )
            dmb = h->sps->i_mb_height - h->mb.i_mb_y;

        a->i_mv_range = 16*dmb + 8;

        a->l0.me16x16.cost = -1;
        a->l0.i_cost8x8    = -1;

        for( i = 0; i < 4; i++ )
        {
            a->l0.i_cost4x4[i] = -1;
            a->l0.i_cost8x4[i] = -1;
            a->l0.i_cost4x8[i] = -1;
        }

        a->l0.i_cost16x8   = -1;
        a->l0.i_cost8x16   = -1;
        if( h->sh.i_type == SLICE_TYPE_B )
        {
            a->l1.me16x16.cost = -1;
            a->l1.i_cost8x8    = -1;

            for( i = 0; i < 4; i++ )
            {
                a->l1.i_cost4x4[i] = -1;
                a->l1.i_cost8x4[i] = -1;
                a->l1.i_cost4x8[i] = -1;
            }

            a->l1.i_cost16x8   = -1;
            a->l1.i_cost8x16   = -1;

            a->i_cost16x16bi   = -1;
        }
    }
}



/*
 * Handle intra mb
 */
/* Max = 4 */
static void predict_16x16_mode_available( unsigned int i_neighbour, int *mode, int *pi_count )
{
    if( ( i_neighbour & (MB_LEFT|MB_TOP) ) == (MB_LEFT|MB_TOP) )
    {
        /* top and left avaible */
        *mode++ = I_PRED_16x16_V;
        *mode++ = I_PRED_16x16_H;
        *mode++ = I_PRED_16x16_DC;
        *mode++ = I_PRED_16x16_P;
        *pi_count = 4;
    }
    else if( ( i_neighbour & MB_LEFT ) )
    {
        /* left available*/
        *mode++ = I_PRED_16x16_DC_LEFT;
        *mode++ = I_PRED_16x16_H;
        *pi_count = 2;
    }
    else if( ( i_neighbour & MB_TOP ) )
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
static void predict_8x8_mode_available( unsigned int i_neighbour, int *mode, int *pi_count )
{
    if( ( i_neighbour & (MB_LEFT|MB_TOP) ) == (MB_LEFT|MB_TOP) )
    {
        /* top and left avaible */
        *mode++ = I_PRED_CHROMA_V;
        *mode++ = I_PRED_CHROMA_H;
        *mode++ = I_PRED_CHROMA_DC;
        *mode++ = I_PRED_CHROMA_P;
        *pi_count = 4;
    }
    else if( ( i_neighbour & MB_LEFT ) )
    {
        /* left available*/
        *mode++ = I_PRED_CHROMA_DC_LEFT;
        *mode++ = I_PRED_CHROMA_H;
        *pi_count = 2;
    }
    else if( ( i_neighbour & MB_TOP ) )
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

/* MAX = 8 */
static void predict_4x4_mode_available( unsigned int i_neighbour, int idx, int *mode, int *pi_count )
{
    int b_a, b_b, b_c;
    static const unsigned int needmb[16] =
    {
        MB_LEFT|MB_TOP, MB_TOP,
        MB_LEFT,        MB_PRIVATE,
        MB_TOP,         MB_TOP|MB_TOPRIGHT,
        0,              MB_PRIVATE,
        MB_LEFT,        0,
        MB_LEFT,        MB_PRIVATE,
        0,              MB_PRIVATE,
        0,              MB_PRIVATE
    };

    /* FIXME even when b_c == 0 there is some case where missing pixels
     * are emulated and thus more mode are available TODO
     * analysis and encode should be fixed too */
    b_a = (needmb[idx]&i_neighbour&MB_LEFT) == (needmb[idx]&MB_LEFT);
    b_b = (needmb[idx]&i_neighbour&MB_TOP) == (needmb[idx]&MB_TOP);
    b_c = (needmb[idx]&i_neighbour&(MB_TOPRIGHT|MB_PRIVATE)) == (needmb[idx]&(MB_TOPRIGHT|MB_PRIVATE));

    if( b_a && b_b )
    {
        *mode++ = I_PRED_4x4_DC;
        *mode++ = I_PRED_4x4_H;
        *mode++ = I_PRED_4x4_V;
        *mode++ = I_PRED_4x4_DDR;
        *mode++ = I_PRED_4x4_VR;
        *mode++ = I_PRED_4x4_HD;
        *mode++ = I_PRED_4x4_HU;

        *pi_count = 7;

        if( b_c )
        {
            *mode++ = I_PRED_4x4_DDL;
            *mode++ = I_PRED_4x4_VL;
            (*pi_count) += 2;
        }
    }
    else if( b_a && !b_b )
    {
        *mode++ = I_PRED_4x4_DC_LEFT;
        *mode++ = I_PRED_4x4_H;
        *pi_count = 2;
    }
    else if( !b_a && b_b )
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
}

static void x264_mb_analyse_intra( x264_t *h, x264_mb_analysis_t *res )
{
    const unsigned int flags = h->sh.i_type == SLICE_TYPE_I ? h->param.analyse.intra : h->param.analyse.inter;
    const int i_stride = h->mb.pic.i_stride[0];
    uint8_t  *p_src = h->mb.pic.p_fenc[0];
    uint8_t  *p_dst = h->mb.pic.p_fdec[0];

    int i, idx;

    int i_max;
    int predict_mode[9];

    /*---------------- Try all mode and calculate their score ---------------*/

    /* 16x16 prediction selection */
    predict_16x16_mode_available( h->mb.i_neighbour, predict_mode, &i_max );
    for( i = 0; i < i_max; i++ )
    {
        int i_sad;
        int i_mode;

        i_mode = predict_mode[i];

        /* we do the prediction */
        h->predict_16x16[i_mode]( p_dst, i_stride );

        /* we calculate the diff and get the square sum of the diff */
        i_sad = h->pixf.satd[PIXEL_16x16]( p_dst, i_stride, p_src, i_stride ) +
                res->i_lambda * bs_size_ue( x264_mb_pred_mode16x16_fix[i_mode] );
        /* if i_score is lower it is better */
        if( res->i_sad_i16x16 == -1 || res->i_sad_i16x16 > i_sad )
        {
            res->i_predict16x16 = i_mode;
            res->i_sad_i16x16     = i_sad;
        }
    }

    /* 4x4 prediction selection */
    if( flags & X264_ANALYSE_I4x4 )
    {
        res->i_sad_i4x4 = 0;
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

            i_best = -1;
            predict_4x4_mode_available( h->mb.i_neighbour, idx, predict_mode, &i_max );
            for( i = 0; i < i_max; i++ )
            {
                int i_sad;
                int i_mode;

                i_mode = predict_mode[i];

                /* we do the prediction */
                h->predict_4x4[i_mode]( p_dst_by, i_stride );

                /* we calculate diff and get the square sum of the diff */
                i_sad = h->pixf.satd[PIXEL_4x4]( p_dst_by, i_stride,
                                                 p_src_by, i_stride );

                i_sad += res->i_lambda * (i_pred_mode == x264_mb_pred_mode4x4_fix[i_mode] ? 1 : 4);

                /* if i_score is lower it is better */
                if( i_best == -1 || i_best > i_sad )
                {
                    res->i_predict4x4[x][y] = i_mode;
                    i_best = i_sad;
                }
            }
            res->i_sad_i4x4 += i_best;

            /* we need to encode this mb now (for next ones) */
            h->predict_4x4[res->i_predict4x4[x][y]]( p_dst_by, i_stride );
            x264_mb_encode_i4x4( h, idx, res->i_qp );

            /* we need to store the 'fixed' version */
            h->mb.cache.intra4x4_pred_mode[x264_scan8[idx]] =
                x264_mb_pred_mode4x4_fix[res->i_predict4x4[x][y]];
        }
        res->i_sad_i4x4 += res->i_lambda * 24;    /* from JVT (SATD0) */
    }
}

static void x264_mb_analyse_intra_chroma( x264_t *h, x264_mb_analysis_t *res )
{
    int i;

    int i_max;
    int predict_mode[9];

    uint8_t *p_dstc[2], *p_srcc[2];
    int      i_stride[2];

    /* 8x8 prediction selection for chroma */
    p_dstc[0] = h->mb.pic.p_fdec[1];
    p_dstc[1] = h->mb.pic.p_fdec[2];
    p_srcc[0] = h->mb.pic.p_fenc[1];
    p_srcc[1] = h->mb.pic.p_fenc[2];

    i_stride[0] = h->mb.pic.i_stride[1];
    i_stride[1] = h->mb.pic.i_stride[2];

    predict_8x8_mode_available( h->mb.i_neighbour, predict_mode, &i_max );
    res->i_sad_i8x8 = -1;
    for( i = 0; i < i_max; i++ )
    {
        int i_sad;
        int i_mode;

        i_mode = predict_mode[i];

        /* we do the prediction */
        h->predict_8x8[i_mode]( p_dstc[0], i_stride[0] );
        h->predict_8x8[i_mode]( p_dstc[1], i_stride[1] );

        /* we calculate the cost */
        i_sad = h->pixf.satd[PIXEL_8x8]( p_dstc[0], i_stride[0],
                                         p_srcc[0], i_stride[0] ) +
                h->pixf.satd[PIXEL_8x8]( p_dstc[1], i_stride[1],
                                         p_srcc[1], i_stride[1] ) +
                res->i_lambda * bs_size_ue( x264_mb_pred_mode8x8_fix[i_mode] );

        /* if i_score is lower it is better */
        if( res->i_sad_i8x8 == -1 || res->i_sad_i8x8 > i_sad )
        {
            res->i_predict8x8 = i_mode;
            res->i_sad_i8x8     = i_sad;
        }
    }
}

static void x264_mb_analyse_inter_p16x16( x264_t *h, x264_mb_analysis_t *a )
{
    x264_me_t m;
    int i_ref;

    /* 16x16 Search on all ref frame */
    m.i_pixel = PIXEL_16x16;
    m.lm      = a->i_lambda;
    m.p_fenc  = h->mb.pic.p_fenc[0];
    m.i_stride= h->mb.pic.i_stride[0];
    m.i_mv_range = a->i_mv_range;
    m.b_mvc   = 0;
//    m.mvc[0]  = 0;
//    m.mvc[1]  = 0;

    /* ME for ref 0 */
    m.p_fref = h->mb.pic.p_fref[0][0][0];
    x264_mb_predict_mv_16x16( h, 0, 0, m.mvp );
    x264_me_search( h, &m );

    a->l0.i_ref = 0;
    a->l0.me16x16 = m;

    for( i_ref = 1; i_ref < h->i_ref0; i_ref++ )
    {
        /* search with ref */
        m.p_fref = h->mb.pic.p_fref[0][i_ref][0];
        x264_mb_predict_mv_16x16( h, 0, i_ref, m.mvp );
        x264_me_search( h, &m );

        /* add ref cost */
        m.cost += m.lm * bs_size_te( h->sh.i_num_ref_idx_l0_active - 1, i_ref );

        if( m.cost < a->l0.me16x16.cost )
        {
            a->l0.i_ref = i_ref;
            a->l0.me16x16 = m;
        }
    }

    /* Set global ref, needed for all others modes */
    x264_macroblock_cache_ref( h, 0, 0, 4, 4, 0, a->l0.i_ref );
}

static void x264_mb_analyse_inter_p8x8( x264_t *h, x264_mb_analysis_t *a )
{
    uint8_t  *p_fref = h->mb.pic.p_fref[0][a->l0.i_ref][0];
    uint8_t  *p_fenc = h->mb.pic.p_fenc[0];

    int i;

    /* XXX Needed for x264_mb_predict_mv */
    h->mb.i_partition = D_8x8;

    for( i = 0; i < 4; i++ )
    {
        x264_me_t *m = &a->l0.me8x8[i];
        const int x8 = i%2;
        const int y8 = i/2;

        m->i_pixel = PIXEL_8x8;
        m->lm      = a->i_lambda;

        m->p_fenc = &p_fenc[8*(y8*h->mb.pic.i_stride[0]+x8)];
        m->p_fref = &p_fref[8*(y8*h->mb.pic.i_stride[0]+x8)];
        m->i_stride= h->mb.pic.i_stride[0];
        m->i_mv_range = a->i_mv_range;

        if( i == 0 )
        {
            m->b_mvc   = 1;
            m->mvc[0] = a->l0.me16x16.mv[0];
            m->mvc[1] = a->l0.me16x16.mv[1];
        }
        else
        {
            m->b_mvc   = 0;
        }

        x264_mb_predict_mv( h, 0, 4*i, 2, m->mvp );
        x264_me_search( h, m );

        x264_macroblock_cache_mv( h, 2*x8, 2*y8, 2, 2, 0, m->mv[0], m->mv[1] );
    }

    a->l0.i_cost8x8 = a->l0.me8x8[0].cost + a->l0.me8x8[1].cost +
                   a->l0.me8x8[2].cost + a->l0.me8x8[3].cost;
}

static void x264_mb_analyse_inter_p16x8( x264_t *h, x264_mb_analysis_t *a )
{
    uint8_t  *p_fref = h->mb.pic.p_fref[0][a->l0.i_ref][0];
    uint8_t  *p_fenc = h->mb.pic.p_fenc[0];

    int i;

    /* XXX Needed for x264_mb_predict_mv */
    h->mb.i_partition = D_16x8;

    for( i = 0; i < 2; i++ )
    {
        x264_me_t *m = &a->l0.me16x8[i];

        m->i_pixel = PIXEL_16x8;
        m->lm      = a->i_lambda;

        m->p_fenc = &p_fenc[8*i*h->mb.pic.i_stride[0]];
        m->p_fref = &p_fref[8*i*h->mb.pic.i_stride[0]];
        m->i_stride= h->mb.pic.i_stride[0];
        m->i_mv_range = a->i_mv_range;

        m->b_mvc   = 1;
        m->mvc[0] = a->l0.me8x8[2*i].mv[0];
        m->mvc[1] = a->l0.me8x8[2*i].mv[1];

        x264_mb_predict_mv( h, 0, 8*i, 4, m->mvp );
        x264_me_search( h, m );

        x264_macroblock_cache_mv( h, 0, 2*i, 4, 2, 0, m->mv[0], m->mv[1] );
    }

    a->l0.i_cost16x8 = a->l0.me16x8[0].cost + a->l0.me16x8[1].cost;
}

static void x264_mb_analyse_inter_p8x16( x264_t *h, x264_mb_analysis_t *a )
{
    uint8_t  *p_fref = h->mb.pic.p_fref[0][a->l0.i_ref][0];
    uint8_t  *p_fenc = h->mb.pic.p_fenc[0];

    int i;

    /* XXX Needed for x264_mb_predict_mv */
    h->mb.i_partition = D_8x16;

    for( i = 0; i < 2; i++ )
    {
        x264_me_t *m = &a->l0.me8x16[i];

        m->i_pixel = PIXEL_8x16;
        m->lm      = a->i_lambda;

        m->p_fenc  = &p_fenc[8*i];
        m->p_fref  = &p_fref[8*i];
        m->i_stride= h->mb.pic.i_stride[0];
        m->i_mv_range = a->i_mv_range;

        m->b_mvc   = 1;
        m->mvc[0] = a->l0.me8x8[i].mv[0];
        m->mvc[1] = a->l0.me8x8[i].mv[1];

        x264_mb_predict_mv( h, 0, 4*i, 2, m->mvp );
        x264_me_search( h, m );

        x264_macroblock_cache_mv( h, 2*i, 0, 2, 4, 0, m->mv[0], m->mv[1] );
    }

    a->l0.i_cost8x16 = a->l0.me8x16[0].cost + a->l0.me8x16[1].cost;
}

static void x264_mb_analyse_inter_p4x4( x264_t *h, x264_mb_analysis_t *a, int i8x8 )
{
    uint8_t  *p_fref = h->mb.pic.p_fref[0][a->l0.i_ref][0];
    uint8_t  *p_fenc = h->mb.pic.p_fenc[0];

    int i4x4;

    /* XXX Needed for x264_mb_predict_mv */
    h->mb.i_partition = D_8x8;

    for( i4x4 = 0; i4x4 < 4; i4x4++ )
    {
        const int idx = 4*i8x8 + i4x4;
        const int x4 = block_idx_x[idx];
        const int y4 = block_idx_y[idx];

        x264_me_t *m = &a->l0.me4x4[i8x8][i4x4];

        m->i_pixel = PIXEL_4x4;
        m->lm      = a->i_lambda;

        m->p_fenc  = &p_fenc[4*(y4*h->mb.pic.i_stride[0]+x4)];
        m->p_fref  = &p_fref[4*(y4*h->mb.pic.i_stride[0]+x4)];
        m->i_stride= h->mb.pic.i_stride[0];
        m->i_mv_range = a->i_mv_range;

        if( i4x4 == 0 )
        {
            m->b_mvc   = 1;
            m->mvc[0] = a->l0.me8x8[i8x8].mv[0];
            m->mvc[1] = a->l0.me8x8[i8x8].mv[1];
        }
        else
        {
            m->b_mvc   = 0;
        }

        x264_mb_predict_mv( h, 0, idx, 1, m->mvp );
        x264_me_search( h, m );

        x264_macroblock_cache_mv( h, x4, y4, 1, 1, 0, m->mv[0], m->mv[1] );
    }

    a->l0.i_cost4x4[i8x8] = a->l0.me4x4[i8x8][0].cost +
                         a->l0.me4x4[i8x8][1].cost +
                         a->l0.me4x4[i8x8][2].cost +
                         a->l0.me4x4[i8x8][3].cost;
}

static void x264_mb_analyse_inter_p8x4( x264_t *h, x264_mb_analysis_t *a, int i8x8 )
{
    uint8_t  *p_fref = h->mb.pic.p_fref[0][a->l0.i_ref][0];
    uint8_t  *p_fenc = h->mb.pic.p_fenc[0];

    int i8x4;

    /* XXX Needed for x264_mb_predict_mv */
    h->mb.i_partition = D_8x8;

    for( i8x4 = 0; i8x4 < 2; i8x4++ )
    {
        const int idx = 4*i8x8 + 2*i8x4;
        const int x4 = block_idx_x[idx];
        const int y4 = block_idx_y[idx];

        x264_me_t *m = &a->l0.me8x4[i8x8][i8x4];

        m->i_pixel = PIXEL_8x4;
        m->lm      = a->i_lambda;

        m->p_fenc  = &p_fenc[4*(y4*h->mb.pic.i_stride[0]+x4)];
        m->p_fref  = &p_fref[4*(y4*h->mb.pic.i_stride[0]+x4)];
        m->i_stride= h->mb.pic.i_stride[0];
        m->i_mv_range = a->i_mv_range;

        if( i8x4 == 0 )
        {
            m->b_mvc   = 1;
            m->mvc[0] = a->l0.me4x4[i8x8][0].mv[0];
            m->mvc[1] = a->l0.me4x4[i8x8][0].mv[1];
        }
        else
        {
            m->b_mvc   = 0;
        }

        x264_mb_predict_mv( h, 0, idx, 2, m->mvp );
        x264_me_search( h, m );

        x264_macroblock_cache_mv( h, x4, y4, 2, 1, 0, m->mv[0], m->mv[1] );
    }

    a->l0.i_cost8x4[i8x8] = a->l0.me8x4[i8x8][0].cost + a->l0.me8x4[i8x8][1].cost;
}

static void x264_mb_analyse_inter_p4x8( x264_t *h, x264_mb_analysis_t *a, int i8x8 )
{
    uint8_t  *p_fref = h->mb.pic.p_fref[0][a->l0.i_ref][0];
    uint8_t  *p_fenc = h->mb.pic.p_fenc[0];

    int i4x8;

    /* XXX Needed for x264_mb_predict_mv */
    h->mb.i_partition = D_8x8;

    for( i4x8 = 0; i4x8 < 2; i4x8++ )
    {
        const int idx = 4*i8x8 + i4x8;
        const int x4 = block_idx_x[idx];
        const int y4 = block_idx_y[idx];

        x264_me_t *m = &a->l0.me4x8[i8x8][i4x8];

        m->i_pixel = PIXEL_4x8;
        m->lm      = a->i_lambda;

        m->p_fenc  = &p_fenc[4*(y4*h->mb.pic.i_stride[0]+x4)];
        m->p_fref  = &p_fref[4*(y4*h->mb.pic.i_stride[0]+x4)];
        m->i_stride= h->mb.pic.i_stride[0];
        m->i_mv_range = a->i_mv_range;

        if( i4x8 == 0 )
        {
            m->b_mvc   = 1;
            m->mvc[0] = a->l0.me4x4[i8x8][0].mv[0];
            m->mvc[1] = a->l0.me4x4[i8x8][0].mv[1];
        }
        else
        {
            m->b_mvc   = 0;
        }

        x264_mb_predict_mv( h, 0, idx, 1, m->mvp );
        x264_me_search( h, m );

        x264_macroblock_cache_mv( h, x4, y4, 1, 2, 0, m->mv[0], m->mv[1] );
    }

    a->l0.i_cost4x8[i8x8] = a->l0.me4x8[i8x8][0].cost + a->l0.me4x8[i8x8][1].cost;
}


static void x264_mb_analyse_inter_b16x16( x264_t *h, x264_mb_analysis_t *a )
{
    uint8_t pix1[16*16], pix2[16*16];

    x264_me_t m;
    int i_ref;

    /* 16x16 Search on all ref frame */
    m.i_pixel = PIXEL_16x16;
    m.lm      = a->i_lambda;
    m.p_fenc  = h->mb.pic.p_fenc[0];
    m.i_stride= h->mb.pic.i_stride[0];
    m.b_mvc   = 0;
    m.i_mv_range = a->i_mv_range;

    /* ME for List 0 ref 0 */
    m.p_fref = h->mb.pic.p_fref[0][0][0];
    x264_mb_predict_mv_16x16( h, 0, 0, m.mvp );
    x264_me_search( h, &m );

    a->l0.i_ref = 0;
    a->l0.me16x16 = m;

    for( i_ref = 1; i_ref < h->i_ref0; i_ref++ )
    {
        /* search with ref */
        m.p_fref = h->mb.pic.p_fref[0][i_ref][0];
        x264_mb_predict_mv_16x16( h, 0, i_ref, m.mvp );
        x264_me_search( h, &m );

        /* add ref cost */
        m.cost += m.lm * bs_size_te( h->sh.i_num_ref_idx_l0_active - 1, i_ref );

        if( m.cost < a->l0.me16x16.cost )
        {
            a->l0.i_ref = i_ref;
            a->l0.me16x16 = m;
        }
    }

    /* ME for list 1 ref 0 */
    m.p_fref = h->mb.pic.p_fref[1][0][0];
    x264_mb_predict_mv_16x16( h, 1, 0, m.mvp );
    x264_me_search( h, &m );

    a->l1.i_ref = 0;
    a->l1.me16x16 = m;

    for( i_ref = 1; i_ref < h->i_ref1; i_ref++ )
    {
        /* search with ref */
        m.p_fref = h->mb.pic.p_fref[1][i_ref][0];
        x264_mb_predict_mv_16x16( h, 1, i_ref, m.mvp );
        x264_me_search( h, &m );

        /* add ref cost */
        m.cost += m.lm * bs_size_te( h->sh.i_num_ref_idx_l1_active - 1, i_ref );

        if( m.cost < a->l1.me16x16.cost )
        {
            a->l1.i_ref = i_ref;
            a->l1.me16x16 = m;
        }
    }

    /* Set global ref, needed for all others modes FIXME some work for mixed block mode */
    x264_macroblock_cache_ref( h, 0, 0, 4, 4, 0, a->l0.i_ref );
    x264_macroblock_cache_ref( h, 0, 0, 4, 4, 1, a->l1.i_ref );

    /* get cost of BI mode */
    h->mc[MC_LUMA]( h->mb.pic.p_fref[0][a->l0.i_ref][0], h->mb.pic.i_stride[0],
                    pix1, 16,
                    a->l0.me16x16.mv[0], a->l0.me16x16.mv[1],
                    16, 16 );
    h->mc[MC_LUMA]( h->mb.pic.p_fref[1][a->l1.i_ref][0], h->mb.pic.i_stride[0],
                    pix2, 16,
                    a->l1.me16x16.mv[0], a->l1.me16x16.mv[1],
                    16, 16 );
    h->pixf.avg[PIXEL_16x16]( pix1, 16, pix2, 16 );

    a->i_cost16x16bi = h->pixf.satd[PIXEL_16x16]( h->mb.pic.p_fenc[0], h->mb.pic.i_stride[0], pix1, 16 ) +
                       a->i_lambda * ( bs_size_te( h->sh.i_num_ref_idx_l0_active - 1, a->l0.i_ref ) +
                                       bs_size_te( h->sh.i_num_ref_idx_l1_active - 1, a->l1.i_ref ) +
                                       bs_size_se( a->l0.me16x16.mv[0] - a->l0.me16x16.mvp[0] ) +
                                       bs_size_se( a->l0.me16x16.mv[1] - a->l0.me16x16.mvp[1] ) +
                                       bs_size_se( a->l1.me16x16.mv[0] - a->l1.me16x16.mvp[0] ) +
                                       bs_size_se( a->l1.me16x16.mv[1] - a->l1.me16x16.mvp[1] ) );
}

/*****************************************************************************
 * x264_macroblock_analyse:
 *****************************************************************************/
void x264_macroblock_analyse( x264_t *h )
{
    x264_mb_analysis_t analysis;
    int i;

    /* qp TODO implement a nice RC */
    h->mb.qp[h->mb.i_mb_xy] = x264_clip3( h->pps->i_pic_init_qp + h->sh.i_qp_delta + 0, 0, 51 );

    /* FIXME check if it's 12 */
    if( h->mb.qp[h->mb.i_mb_xy] - h->mb.i_last_qp < -12 )
        h->mb.qp[h->mb.i_mb_xy] = h->mb.i_last_qp - 12;
    else if( h->mb.qp[h->mb.i_mb_xy] - h->mb.i_last_qp > 12 )
        h->mb.qp[h->mb.i_mb_xy] = h->mb.i_last_qp + 12;

    /* init analysis */
    x264_mb_analyse_init( h, &analysis, h->mb.qp[h->mb.i_mb_xy] );

    /*--------------------------- Do the analysis ---------------------------*/
    if( h->sh.i_type == SLICE_TYPE_I )
    {
        x264_mb_analyse_intra( h, &analysis );

        if( analysis.i_sad_i4x4 >= 0 &&  analysis.i_sad_i4x4 < analysis.i_sad_i16x16 )
            h->mb.i_type = I_4x4;
        else
            h->mb.i_type = I_16x16;
    }
    else if( h->sh.i_type == SLICE_TYPE_P )
    {
        const unsigned int i_neighbour = h->mb.i_neighbour;

        int b_skip = 0;
        int i_cost;

        /* Fast P_SKIP detection */
        if( ( (i_neighbour&MB_LEFT) && h->mb.type[h->mb.i_mb_xy - 1] == P_SKIP ) ||
            ( (i_neighbour&MB_TOP) && h->mb.type[h->mb.i_mb_xy - h->mb.i_mb_stride] == P_SKIP ) ||
            ( ((i_neighbour&(MB_TOP|MB_LEFT)) == (MB_TOP|MB_LEFT) ) && h->mb.type[h->mb.i_mb_xy - h->mb.i_mb_stride-1 ] == P_SKIP ) ||
            ( (i_neighbour&MB_TOPRIGHT) && h->mb.type[h->mb.i_mb_xy - h->mb.i_mb_stride+1 ] == P_SKIP ) )
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
                h->mb.i_sub_partition[0] = D_L0_8x8;
                h->mb.i_sub_partition[1] = D_L0_8x8;
                h->mb.i_sub_partition[2] = D_L0_8x8;
                h->mb.i_sub_partition[3] = D_L0_8x8;

                i_cost = analysis.l0.i_cost8x8;

                /* Do sub 8x8 */
                if( flags & X264_ANALYSE_PSUB8x8 )
                {
                    for( i = 0; i < 4; i++ )
                    {
                        x264_mb_analyse_inter_p4x4( h, &analysis, i );
                        if( analysis.l0.i_cost4x4[i] < analysis.l0.me8x8[i].cost )
                        {
                            int i_cost8x8;

                            h->mb.i_sub_partition[i] = D_L0_4x4;
                            i_cost8x8 = analysis.l0.i_cost4x4[i];

                            x264_mb_analyse_inter_p8x4( h, &analysis, i );
                            if( analysis.l0.i_cost8x4[i] < analysis.l0.i_cost4x4[i] )
                            {
                                h->mb.i_sub_partition[i] = D_L0_8x4;
                                i_cost8x8 = analysis.l0.i_cost8x4[i];
                            }

                            x264_mb_analyse_inter_p4x8( h, &analysis, i );
                            if( analysis.l0.i_cost4x8[i] < analysis.l0.i_cost4x4[i] )
                            {
                                h->mb.i_sub_partition[i] = D_L0_4x8;
                                i_cost8x8 = analysis.l0.i_cost4x8[i];
                            }

                            i_cost += i_cost8x8 - analysis.l0.me8x8[i].cost;
                        }
                    }
                }

                /* Now do sub 16x8/8x16 */
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

            h->mb.i_type = i_type;
            h->mb.i_partition = i_partition;

            /* refine qpel */
            if( h->mb.i_partition == D_16x16 )
            {
                x264_me_refine_qpel( h, &analysis.l0.me16x16 );
                i_cost = analysis.l0.me16x16.cost;
            }
            else if( h->mb.i_partition == D_16x8 )
            {
                x264_me_refine_qpel( h, &analysis.l0.me16x8[0] );
                x264_me_refine_qpel( h, &analysis.l0.me16x8[1] );
                i_cost = analysis.l0.me16x8[0].cost + analysis.l0.me16x8[1].cost;
            }
            else if( h->mb.i_partition == D_8x16 )
            {
                x264_me_refine_qpel( h, &analysis.l0.me8x16[0] );
                x264_me_refine_qpel( h, &analysis.l0.me8x16[1] );
                i_cost = analysis.l0.me8x16[0].cost + analysis.l0.me8x16[1].cost;
            }
            else if( h->mb.i_partition == D_8x8 )
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
                            fprintf( stderr, "internal error (!8x8 && !4x4)" );
                            break;
                    }
                }
            }

            x264_mb_analyse_intra( h, &analysis );
            if( analysis.i_sad_i16x16 >= 0 && analysis.i_sad_i16x16 < i_cost )
            {
                h->mb.i_type = I_16x16;
                i_cost = analysis.i_sad_i16x16;
            }

            if( analysis.i_sad_i4x4 >=0 && analysis.i_sad_i4x4 < i_cost )
            {
                h->mb.i_type = I_4x4;
                i_cost = analysis.i_sad_i4x4;
            }
        }
    }
    else if( h->sh.i_type == SLICE_TYPE_B )
    {
        int i_cost;

        /* best inter mode */
        x264_mb_analyse_inter_b16x16( h, &analysis );
        h->mb.i_type = B_L0_L0;
        h->mb.i_partition = D_16x16;
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

        /* best intra mode */
        x264_mb_analyse_intra( h, &analysis );
        if( analysis.i_sad_i16x16 >= 0 && analysis.i_sad_i16x16 < i_cost )
        {
            h->mb.i_type = I_16x16;
            i_cost = analysis.i_sad_i16x16;
        }
        if( analysis.i_sad_i4x4 >=0 && analysis.i_sad_i4x4 < i_cost )
        {
            h->mb.i_type = I_4x4;
            i_cost = analysis.i_sad_i4x4;
        }
    }
#undef BEST_TYPE

    /*-------------------- Update MB from the analysis ----------------------*/
    h->mb.type[h->mb.i_mb_xy] = h->mb.i_type;
    switch( h->mb.i_type )
    {
        case I_4x4:
            for( i = 0; i < 16; i++ )
            {
                h->mb.cache.intra4x4_pred_mode[x264_scan8[i]] =
                    analysis.i_predict4x4[block_idx_x[i]][block_idx_y[i]];
            }

            x264_mb_analyse_intra_chroma( h, &analysis );
            h->mb.i_chroma_pred_mode = analysis.i_predict8x8;
            break;
        case I_16x16:
            h->mb.i_intra16x16_pred_mode = analysis.i_predict16x16;

            x264_mb_analyse_intra_chroma( h, &analysis );
            h->mb.i_chroma_pred_mode = analysis.i_predict8x8;
            break;

        case P_L0:
            x264_macroblock_cache_ref( h, 0, 0, 4, 4, 0, analysis.l0.i_ref );
            switch( h->mb.i_partition )
            {
                case D_16x16:
                    x264_macroblock_cache_mv ( h, 0, 0, 4, 4, 0, analysis.l0.me16x16.mv[0], analysis.l0.me16x16.mv[1] );
                    break;

                case D_16x8:
                    x264_macroblock_cache_mv ( h, 0, 0, 4, 2, 0, analysis.l0.me16x8[0].mv[0], analysis.l0.me16x8[0].mv[1] );
                    x264_macroblock_cache_mv ( h, 0, 2, 4, 2, 0, analysis.l0.me16x8[1].mv[0], analysis.l0.me16x8[1].mv[1] );
                    break;

                case D_8x16:
                    x264_macroblock_cache_mv ( h, 0, 0, 2, 4, 0, analysis.l0.me8x16[0].mv[0], analysis.l0.me8x16[0].mv[1] );
                    x264_macroblock_cache_mv ( h, 2, 0, 2, 4, 0, analysis.l0.me8x16[1].mv[0], analysis.l0.me8x16[1].mv[1] );
                    break;

                default:
                    fprintf( stderr, "internal error P_L0 and partition=%d\n", h->mb.i_partition );
                    break;
            }
            break;

        case P_8x8:
            x264_macroblock_cache_ref( h, 0, 0, 4, 4, 0, analysis.l0.i_ref );
            for( i = 0; i < 4; i++ )
            {
                const int x = 2*(i%2);
                const int y = 2*(i/2);

                switch( h->mb.i_sub_partition[i] )
                {
                    case D_L0_8x8:
                        x264_macroblock_cache_mv( h, x, y, 2, 2, 0, analysis.l0.me8x8[i].mv[0], analysis.l0.me8x8[i].mv[1] );
                        break;
                    case D_L0_8x4:
                        x264_macroblock_cache_mv( h, x, y+0, 2, 1, 0, analysis.l0.me8x4[i][0].mv[0], analysis.l0.me8x4[i][0].mv[1] );
                        x264_macroblock_cache_mv( h, x, y+1, 2, 1, 0, analysis.l0.me8x4[i][1].mv[0], analysis.l0.me8x4[i][1].mv[1] );
                        break;
                    case D_L0_4x8:
                        x264_macroblock_cache_mv( h, x+0, y, 1, 2, 0, analysis.l0.me4x8[i][0].mv[0], analysis.l0.me4x8[i][0].mv[1] );
                        x264_macroblock_cache_mv( h, x+1, y, 1, 2, 0, analysis.l0.me4x8[i][1].mv[0], analysis.l0.me4x8[i][1].mv[1] );
                        break;
                    case D_L0_4x4:
                        x264_macroblock_cache_mv( h, x+0, y+0, 1, 1, 0, analysis.l0.me4x4[i][0].mv[0], analysis.l0.me4x4[i][0].mv[1] );
                        x264_macroblock_cache_mv( h, x+1, y+0, 1, 1, 0, analysis.l0.me4x4[i][1].mv[0], analysis.l0.me4x4[i][1].mv[1] );
                        x264_macroblock_cache_mv( h, x+0, y+1, 1, 1, 0, analysis.l0.me4x4[i][2].mv[0], analysis.l0.me4x4[i][2].mv[1] );
                        x264_macroblock_cache_mv( h, x+1, y+1, 1, 1, 0, analysis.l0.me4x4[i][3].mv[0], analysis.l0.me4x4[i][3].mv[1] );
                        break;
                    default:
                        fprintf( stderr, "internal error\n" );
                        break;
                }
            }
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

        case B_L0_L0:
            switch( h->mb.i_partition )
            {
                case D_16x16:
                    x264_macroblock_cache_ref( h, 0, 0, 4, 4, 0, analysis.l0.i_ref );
                    x264_macroblock_cache_mv ( h, 0, 0, 4, 4, 0, analysis.l0.me16x16.mv[0], analysis.l0.me16x16.mv[1] );

                    x264_macroblock_cache_ref( h, 0, 0, 4, 4, 1, -1 );
                    x264_macroblock_cache_mv ( h, 0, 0, 4, 4, 1,  0, 0 );
                    x264_macroblock_cache_mvd( h, 0, 0, 4, 4, 1,  0, 0 );
                    break;
                default:
                    fprintf( stderr, "internal error\n" );
                    break;
            }
            break;
        case B_L1_L1:
            switch( h->mb.i_partition )
            {
                case D_16x16:
                    x264_macroblock_cache_ref( h, 0, 0, 4, 4, 0, -1 );
                    x264_macroblock_cache_mv ( h, 0, 0, 4, 4, 0,  0, 0 );
                    x264_macroblock_cache_mvd( h, 0, 0, 4, 4, 0,  0, 0 );

                    x264_macroblock_cache_ref( h, 0, 0, 4, 4, 1, analysis.l1.i_ref );
                    x264_macroblock_cache_mv ( h, 0, 0, 4, 4, 1, analysis.l1.me16x16.mv[0], analysis.l1.me16x16.mv[1] );
                    break;

                default:
                    fprintf( stderr, "internal error\n" );
                    break;
            }
            break;
        case B_BI_BI:
            switch( h->mb.i_partition )
            {
                case D_16x16:
                    x264_macroblock_cache_ref( h, 0, 0, 4, 4, 0, analysis.l0.i_ref );
                    x264_macroblock_cache_mv ( h, 0, 0, 4, 4, 0, analysis.l0.me16x16.mv[0], analysis.l0.me16x16.mv[1] );

                    x264_macroblock_cache_ref( h, 0, 0, 4, 4, 1, analysis.l1.i_ref );
                    x264_macroblock_cache_mv ( h, 0, 0, 4, 4, 1, analysis.l1.me16x16.mv[0], analysis.l1.me16x16.mv[1] );
                    break;

                default:
                    fprintf( stderr, "internal error\n" );
                    break;
            }
            break;

        default:
            fprintf( stderr, "internal error (invalid MB type)\n" );
            break;
    }
}


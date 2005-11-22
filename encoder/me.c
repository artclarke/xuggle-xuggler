/*****************************************************************************
 * me.c: h264 encoder library (Motion Estimation)
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar
 * $Id: me.c,v 1.1 2004/06/03 19:27:08 fenrir Exp $
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

#include "common/common.h"
#include "me.h"

/* presets selected from good points on the speed-vs-quality curve of several test videos
 * subpel_iters[i_subpel_refine] = { refine_hpel, refine_qpel, me_hpel, me_qpel }
 * where me_* are the number of EPZS iterations run on all candidate block types,
 * and refine_* are run only on the winner. */
static const int subpel_iterations[][4] = 
   {{1,0,0,0},
    {1,1,0,0},
    {0,1,1,0},
    {0,2,1,0},
    {0,2,1,1},
    {0,2,1,2},
    {0,0,2,3}};

static void refine_subpel( x264_t *h, x264_me_t *m, int hpel_iters, int qpel_iters, int *p_halfpel_thresh, int b_refine_qpel );

#define COST_MV_INT( mx, my, bd, d ) \
{ \
    int cost = h->pixf.sad[i_pixel]( m->p_fenc[0], m->i_stride[0],     \
                   &p_fref[(my)*m->i_stride[0]+(mx)], m->i_stride[0] ) \
             + p_cost_mvx[ (mx)<<2 ]  \
             + p_cost_mvy[ (my)<<2 ]; \
    if( cost < bcost ) \
    {                  \
        bcost = cost;  \
        bmx = mx;      \
        bmy = my;      \
        if( bd ) \
            dir = d; \
    } \
}
#define COST_MV( mx, my )         COST_MV_INT( mx, my, 0, 0 )
#define COST_MV_DIR( mx, my, d )  COST_MV_INT( mx, my, 1, d )

#define DIA1_ITER( mx, my )\
    {\
        omx = mx; omy = my;\
        COST_MV( omx  , omy-1 );/*  1  */\
        COST_MV( omx  , omy+1 );/* 101 */\
        COST_MV( omx-1, omy   );/*  1  */\
        COST_MV( omx+1, omy   );\
    }


void x264_me_search_ref( x264_t *h, x264_me_t *m, int (*mvc)[2], int i_mvc, int *p_halfpel_thresh )
{
    const int i_pixel = m->i_pixel;
    const int i_me_range = h->param.analyse.i_me_range;
    int bmx, bmy, bcost;
    int omx, omy, pmx, pmy;
    uint8_t *p_fref = m->p_fref[0];
    int i, j;
    int dir;

    int mv_x_min = h->mb.mv_min_fpel[0];
    int mv_y_min = h->mb.mv_min_fpel[1];
    int mv_x_max = h->mb.mv_max_fpel[0];
    int mv_y_max = h->mb.mv_max_fpel[1];

    const int16_t *p_cost_mvx = m->p_cost_mv - m->mvp[0];
    const int16_t *p_cost_mvy = m->p_cost_mv - m->mvp[1];

    if( h->mb.i_me_method == X264_ME_UMH )
    {
        /* clamp mvp to inside frame+padding, so that we don't have to check it each iteration */
        p_cost_mvx = m->p_cost_mv - x264_clip3( m->mvp[0], h->mb.mv_min[0], h->mb.mv_max[0] );
        p_cost_mvy = m->p_cost_mv - x264_clip3( m->mvp[1], h->mb.mv_min[1], h->mb.mv_max[1] );
    }

    bmx = pmx = x264_clip3( ( m->mvp[0] + 2 ) >> 2, mv_x_min, mv_x_max );
    bmy = pmy = x264_clip3( ( m->mvp[1] + 2 ) >> 2, mv_y_min, mv_y_max );
    bcost = COST_MAX;
    COST_MV( pmx, pmy );
    /* I don't know why this helps */
    bcost -= p_cost_mvx[ bmx<<2 ] + p_cost_mvy[ bmy<<2 ];

    /* try extra predictors if provided */
    for( i = 0; i < i_mvc; i++ )
    {
        const int mx = x264_clip3( ( mvc[i][0] + 2 ) >> 2, mv_x_min, mv_x_max );
        const int my = x264_clip3( ( mvc[i][1] + 2 ) >> 2, mv_y_min, mv_y_max );
        if( mx != bmx || my != bmy )
            COST_MV( mx, my );
    }
    
    COST_MV( 0, 0 );

    mv_x_max += 8;
    mv_y_max += 8;
    mv_x_min -= 8;
    mv_y_min -= 8;

    switch( h->mb.i_me_method )
    {
    case X264_ME_DIA:
        /* diamond search, radius 1 */
        for( i = 0; i < i_me_range; i++ )
        {
            DIA1_ITER( bmx, bmy );
            if( bmx == omx && bmy == omy )
                break;
        }
        break;

    case X264_ME_HEX:
me_hex2:
        /* hexagon search, radius 2 */
#if 0
        for( i = 0; i < i_me_range/2; i++ )
        {
            omx = bmx; omy = bmy;
            COST_MV( omx-2, omy   );
            COST_MV( omx-1, omy+2 );
            COST_MV( omx+1, omy+2 );
            COST_MV( omx+2, omy   );
            COST_MV( omx+1, omy-2 );
            COST_MV( omx-1, omy-2 );
            if( bmx == omx && bmy == omy )
                break;
        }
#else
        /* equivalent to the above, but eliminates duplicate candidates */
        dir = -1;
        omx = bmx; omy = bmy;
        COST_MV_DIR( omx-2, omy,   0 );
        COST_MV_DIR( omx-1, omy+2, 1 );
        COST_MV_DIR( omx+1, omy+2, 2 );
        COST_MV_DIR( omx+2, omy,   3 );
        COST_MV_DIR( omx+1, omy-2, 4 );
        COST_MV_DIR( omx-1, omy-2, 5 );
        if( dir != -1 )
        {
            for( i = 1; i < i_me_range/2; i++ )
            {
                static const int hex2[8][2] = {{-1,-2}, {-2,0}, {-1,2}, {1,2}, {2,0}, {1,-2}, {-1,-2}, {-2,0}};
                static const int mod6[8] = {5,0,1,2,3,4,5,0};
                const int odir = mod6[dir+1];
                omx = bmx; omy = bmy;
                COST_MV_DIR( omx + hex2[odir+0][0], omy + hex2[odir+0][1], odir-1 );
                COST_MV_DIR( omx + hex2[odir+1][0], omy + hex2[odir+1][1], odir   );
                COST_MV_DIR( omx + hex2[odir+2][0], omy + hex2[odir+2][1], odir+1 );
                if( bmx == omx && bmy == omy )
                    break;
            }
        }
#endif
        /* square refine */
        DIA1_ITER( bmx, bmy );
        COST_MV( omx-1, omy-1 );
        COST_MV( omx-1, omy+1 );
        COST_MV( omx+1, omy-1 );
        COST_MV( omx+1, omy+1 );
        break;

    case X264_ME_UMH:
        {
            /* Uneven-cross Multi-Hexagon-grid Search
             * as in JM, except without early termination */

            int ucost;
            int cross_start;

            /* refine predictors */
            DIA1_ITER( pmx, pmy );
            if( pmx || pmy )
                DIA1_ITER( 0, 0 );

            if(i_pixel == PIXEL_4x4)
                goto me_hex2;

            ucost = bcost;
            if( (bmx || bmy) && (bmx!=pmx || bmy!=pmy) )
                DIA1_ITER( bmx, bmy );

            /* cross */
            omx = bmx; omy = bmy;
            cross_start = ( bcost == ucost ) ? 3 : 1;
            for( i = cross_start; i < i_me_range; i+=2 )
            {
                if( omx + i <= mv_x_max )
                    COST_MV( omx + i, omy );
                if( omx - i >= mv_x_min )
                    COST_MV( omx - i, omy );
            }
            for( i = cross_start; i < i_me_range/2; i+=2 )
            {
                if( omy + i <= mv_y_max )
                    COST_MV( omx, omy + i );
                if( omy - i >= mv_y_min )
                    COST_MV( omx, omy - i );
            }

            /* 5x5 ESA */
            omx = bmx; omy = bmy;
            for( i = (bcost == ucost) ? 4 : 0; i < 24; i++ )
            {
                static const int square2[24][2] = {
                    { 1, 0}, { 0, 1}, {-1, 0}, { 0,-1},
                    { 1, 1}, {-1, 1}, {-1,-1}, { 1,-1},
                    { 2,-1}, { 2, 0}, { 2, 1}, { 2, 2},
                    { 1, 2}, { 0, 2}, {-1, 2}, {-2, 2},
                    {-2, 1}, {-2, 0}, {-2,-1}, {-2,-2},
                    {-1,-2}, { 0,-2}, { 1,-2}, { 2,-2}
                };
                COST_MV( omx + square2[i][0], omy + square2[i][1] );
            }
            /* hexagon grid */
            omx = bmx; omy = bmy;
            for( i = 1; i <= i_me_range/4; i++ )
            {
                int bounds_check = 4*i > X264_MIN4( mv_x_max-omx, mv_y_max-omy, omx-mv_x_min, omy-mv_y_min );
                for( j = 0; j < 16; j++ )
                {
                    static const int hex4[16][2] = {
                        {-4, 2}, {-4, 1}, {-4, 0}, {-4,-1}, {-4,-2},
                        { 4,-2}, { 4,-1}, { 4, 0}, { 4, 1}, { 4, 2},
                        { 2, 3}, { 0, 4}, {-2, 3},
                        {-2,-3}, { 0,-4}, { 2,-3},
                    };
                    int mx = omx + hex4[j][0]*i;
                    int my = omy + hex4[j][1]*i;
                    if( !bounds_check || ( mx >= mv_x_min && mx <= mv_x_max
                                        && my >= mv_y_min && my <= mv_y_max ) )
                        COST_MV( mx, my );
                }
            }
            goto me_hex2;
        }

    case X264_ME_ESA:
        {
            const int min_x = X264_MAX( bmx - i_me_range, mv_x_min);
            const int min_y = X264_MAX( bmy - i_me_range, mv_y_min);
            const int max_x = X264_MIN( bmx + i_me_range, mv_x_max);
            const int max_y = X264_MIN( bmy + i_me_range, mv_y_max);
            for( omy = min_y; omy <= max_y; omy++ )
                for( omx = min_x; omx <= max_x; omx++ )
                {
                    COST_MV( omx, omy );
                }
        }
        break;
    }

    /* -> qpel mv */
    m->mv[0] = bmx << 2;
    m->mv[1] = bmy << 2;

    /* compute the real cost */
    m->cost_mv = p_cost_mvx[ m->mv[0] ] + p_cost_mvy[ m->mv[1] ];
    m->cost = bcost;
    if( bmx == pmx && bmy == pmy )
        m->cost += m->cost_mv;
    
    /* subpel refine */
    if( h->mb.i_subpel_refine >= 2 )
    {
        int hpel = subpel_iterations[h->mb.i_subpel_refine][2];
        int qpel = subpel_iterations[h->mb.i_subpel_refine][3];
        refine_subpel( h, m, hpel, qpel, p_halfpel_thresh, 0 );
    }
}
#undef COST_MV

void x264_me_refine_qpel( x264_t *h, x264_me_t *m )
{
    int hpel = subpel_iterations[h->mb.i_subpel_refine][0];
    int qpel = subpel_iterations[h->mb.i_subpel_refine][1];

    if( m->i_pixel <= PIXEL_8x8 && h->sh.i_type == SLICE_TYPE_P )
        m->cost -= m->i_ref_cost;
	
    refine_subpel( h, m, hpel, qpel, NULL, 1 );
}

#define COST_MV_SAD( mx, my ) \
{ \
    int stride = 16; \
    uint8_t *src = h->mc.get_ref( m->p_fref, m->i_stride[0], pix, &stride, mx, my, bw, bh ); \
    int cost = h->pixf.sad[i_pixel]( m->p_fenc[0], m->i_stride[0], src, stride ) \
             + p_cost_mvx[ mx ] + p_cost_mvy[ my ]; \
    if( cost < bcost ) \
    {                  \
        bcost = cost;  \
        bmx = mx;      \
        bmy = my;      \
    } \
}

#define COST_MV_SATD( mx, my, dir ) \
if( b_refine_qpel || (dir^1) != odir ) \
{ \
    int stride = 16; \
    uint8_t *src = h->mc.get_ref( m->p_fref, m->i_stride[0], pix, &stride, mx, my, bw, bh ); \
    int cost = h->pixf.mbcmp[i_pixel]( m->p_fenc[0], m->i_stride[0], src, stride ) \
             + p_cost_mvx[ mx ] + p_cost_mvy[ my ]; \
    if( b_chroma_me && cost < bcost ) \
    { \
        h->mc.mc_chroma( m->p_fref[4], m->i_stride[1], pix, 8, mx, my, bw/2, bh/2 ); \
        cost += h->pixf.mbcmp[i_pixel+3]( m->p_fenc[1], m->i_stride[1], pix, 8 ); \
        if( cost < bcost ) \
        { \
            h->mc.mc_chroma( m->p_fref[5], m->i_stride[1], pix, 8, mx, my, bw/2, bh/2 ); \
            cost += h->pixf.mbcmp[i_pixel+3]( m->p_fenc[2], m->i_stride[1], pix, 8 ); \
        } \
    } \
    if( cost < bcost ) \
    {                  \
        bcost = cost;  \
        bmx = mx;      \
        bmy = my;      \
        bdir = dir;    \
    } \
}

static void refine_subpel( x264_t *h, x264_me_t *m, int hpel_iters, int qpel_iters, int *p_halfpel_thresh, int b_refine_qpel )
{
    const int bw = x264_pixel_size[m->i_pixel].w;
    const int bh = x264_pixel_size[m->i_pixel].h;
    const int16_t *p_cost_mvx = m->p_cost_mv - m->mvp[0];
    const int16_t *p_cost_mvy = m->p_cost_mv - m->mvp[1];
    const int i_pixel = m->i_pixel;
    const int b_chroma_me = h->mb.b_chroma_me && i_pixel <= PIXEL_8x8;

    DECLARE_ALIGNED( uint8_t, pix[16*16], 16 );
    int omx, omy;
    int i;

    int bmx = m->mv[0];
    int bmy = m->mv[1];
    int bcost = m->cost;
    int odir = -1, bdir;


    /* try the subpel component of the predicted mv if it's close to
     * the result of the fullpel search */
    if( hpel_iters )
    {
        int mx = x264_clip3( m->mvp[0], h->mb.mv_min[0], h->mb.mv_max[0] );
        int my = x264_clip3( m->mvp[1], h->mb.mv_min[1], h->mb.mv_max[1] );
        if( mx != bmx || my != bmy )
            COST_MV_SAD( mx, my );
    }
    
    /* hpel search */
    for( i = hpel_iters; i > 0; i-- )
    {
        omx = bmx;
        omy = bmy;
        COST_MV_SAD( omx, omy - 2 );
        COST_MV_SAD( omx, omy + 2 );
        COST_MV_SAD( omx - 2, omy );
        COST_MV_SAD( omx + 2, omy );
        if( bmx == omx && bmy == omy )
            break;
    }
    
    if( !b_refine_qpel )
    {
        bcost = COST_MAX;
        COST_MV_SATD( bmx, bmy, -1 );
    }
    
    /* early termination when examining multiple reference frames */
    if( p_halfpel_thresh )
    {
        if( (bcost*7)>>3 > *p_halfpel_thresh )
        {
            m->cost = bcost;
            m->mv[0] = bmx;
            m->mv[1] = bmy;
            // don't need cost_mv
            return;
        }
        else if( bcost < *p_halfpel_thresh )
            *p_halfpel_thresh = bcost;
    }

    /* qpel search */
    bdir = -1;
    for( i = qpel_iters; i > 0; i-- )
    {
        odir = bdir;
        omx = bmx;
        omy = bmy;
        COST_MV_SATD( omx, omy - 1, 0 );
        COST_MV_SATD( omx, omy + 1, 1 );
        COST_MV_SATD( omx - 1, omy, 2 );
        COST_MV_SATD( omx + 1, omy, 3 );
        if( bmx == omx && bmy == omy )
            break;
    }

    m->cost = bcost;
    m->mv[0] = bmx;
    m->mv[1] = bmy;
    m->cost_mv = p_cost_mvx[ bmx ] + p_cost_mvy[ bmy ];
}


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
    {1,2,0,0},
    {0,2,1,0},
    {0,2,1,1},
    {0,2,1,2}};

static void refine_subpel( x264_t *h, x264_me_t *m, int hpel_iters, int qpel_iters );

#define COST_MV( mx, my ) \
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
    } \
}

void x264_me_search_ref( x264_t *h, x264_me_t *m, int (*mvc)[2], int i_mvc, int *p_fullpel_thresh )
{
    const int i_pixel = m->i_pixel;
    const int b_chroma_me = h->mb.b_chroma_me && i_pixel <= PIXEL_8x8;
    int bmx, bmy, bcost;
    int omx, omy;
    uint8_t *p_fref = m->p_fref[0];
    int i_iter;

    const int mv_x_min = h->mb.mv_min_fpel[0];
    const int mv_y_min = h->mb.mv_min_fpel[1];
    const int mv_x_max = h->mb.mv_max_fpel[0];
    const int mv_y_max = h->mb.mv_max_fpel[1];

    const int16_t *p_cost_mvx = m->p_cost_mv - m->mvp[0];
    const int16_t *p_cost_mvy = m->p_cost_mv - m->mvp[1];


    /* init with mvp */
    /* XXX: We don't need to clamp because the way diamond work, we will
     * never go outside padded picture, and predict mv won't compute vector
     * with componant magnitude greater.
     * XXX: if some vector can go outside, (accelerator, ....) you need to clip
     * them yourself */
    bmx = x264_clip3( ( m->mvp[0] + 2 ) >> 2, mv_x_min, mv_x_max );
    bmy = x264_clip3( ( m->mvp[1] + 2 ) >> 2, mv_y_min, mv_y_max );
    bcost = COST_MAX;
    COST_MV( bmx, bmy );
    /* I don't know why this helps */
    bcost -= p_cost_mvx[ bmx<<2 ] + p_cost_mvy[ bmy<<2 ];

    /* try extra predictors if provided */
    for( i_iter = 0; i_iter < i_mvc; i_iter++ )
    {
        const int mx = x264_clip3( ( mvc[i_iter][0] + 2 ) >> 2, mv_x_min, mv_x_max );
        const int my = x264_clip3( ( mvc[i_iter][1] + 2 ) >> 2, mv_y_min, mv_y_max );
        if( mx != bmx || my != bmy )
            COST_MV( mx, my );
    }
    
    COST_MV( 0, 0 );

    if( h->mb.i_subpel_refine >= 2 )
    {
        /* hexagon search */
        /* Don't need to test mv_range each time, we won't go outside picture+padding */
        omx = bmx;
        omy = bmy;
        for( i_iter = 0; i_iter < 8; i_iter++ )
        {
            COST_MV( omx-2, omy   );
            COST_MV( omx-1, omy+2 );
            COST_MV( omx+1, omy+2 );
            COST_MV( omx+2, omy   );
            COST_MV( omx+1, omy-2 );
            COST_MV( omx-1, omy-2 );

            if( bmx == omx && bmy == omy )
                break;
            omx = bmx;
            omy = bmy;
        }
    
        /* square refine */
        COST_MV( omx-1, omy-1 );
        COST_MV( omx-1, omy   );
        COST_MV( omx-1, omy+1 );
        COST_MV( omx  , omy-1 );
        COST_MV( omx  , omy+1 );
        COST_MV( omx+1, omy-1 );
        COST_MV( omx+1, omy   );
        COST_MV( omx+1, omy+1 );
    }
    else
    {
        /* diamond search */
        for( i_iter = 0; i_iter < 16; i_iter++ )
        {
            omx = bmx;
            omy = bmy;
            COST_MV( omx  , omy-1 );
            COST_MV( omx  , omy+1 );
            COST_MV( omx-1, omy   );
            COST_MV( omx+1, omy   );
            if( bmx == omx && bmy == omy )
                break;
        }
    }

    /* -> qpel mv */
    m->mv[0] = bmx << 2;
    m->mv[1] = bmy << 2;

    /* compute the real cost */
    m->cost_mv = p_cost_mvx[ m->mv[0] ] + p_cost_mvy[ m->mv[1] ];
    m->cost = h->pixf.satd[i_pixel]( m->p_fenc[0], m->i_stride[0],
                    &p_fref[bmy * m->i_stride[0] + bmx], m->i_stride[0] )
            + m->cost_mv;
    if( b_chroma_me )
    {
        const int bw = x264_pixel_size[m->i_pixel].w;
        const int bh = x264_pixel_size[m->i_pixel].h;
        DECLARE_ALIGNED( uint8_t, pix[8*8*2], 16 );
        h->mc.mc_chroma( m->p_fref[4], m->i_stride[1], pix, 8, m->mv[0], m->mv[1], bw/2, bh/2 );
        h->mc.mc_chroma( m->p_fref[5], m->i_stride[1], pix+8*8, 8, m->mv[0], m->mv[1], bw/2, bh/2 );
        m->cost += h->pixf.satd[i_pixel+3]( m->p_fenc[1], m->i_stride[1], pix, 8 )
                 + h->pixf.satd[i_pixel+3]( m->p_fenc[2], m->i_stride[1], pix+8*8, 8 );
    }

    /* subpel refine */
    if( h->mb.i_subpel_refine >= 3 )
    {
        int hpel, qpel;

        /* early termination (when examining multiple reference frames)
         * FIXME: this can update fullpel_thresh even if the match
         *        ref is rejected after subpel refinement */
        if( p_fullpel_thresh )
        {
            if( (m->cost*7)>>3 > *p_fullpel_thresh )
                return;
            else if( m->cost < *p_fullpel_thresh )
                *p_fullpel_thresh = m->cost;
        }

        hpel = subpel_iterations[h->mb.i_subpel_refine][2];
        qpel = subpel_iterations[h->mb.i_subpel_refine][3];
        refine_subpel( h, m, hpel, qpel );
    }
}
#undef COST_MV

void x264_me_refine_qpel( x264_t *h, x264_me_t *m )
{
    int hpel = subpel_iterations[h->mb.i_subpel_refine][0];
    int qpel = subpel_iterations[h->mb.i_subpel_refine][1];
//  if( hpel || qpel )
	refine_subpel( h, m, hpel, qpel );
}

#define COST_MV( mx, my, dir ) \
{ \
    int stride = 16; \
    uint8_t *src = h->mc.get_ref( m->p_fref, m->i_stride[0], pix, &stride, mx, my, bw, bh ); \
    int cost = h->pixf.satd[i_pixel]( m->p_fenc[0], m->i_stride[0], src, stride ) \
             + p_cost_mvx[ mx ] + p_cost_mvy[ my ]; \
    if( b_chroma_me && cost < bcost ) \
    { \
        h->mc.mc_chroma( m->p_fref[4], m->i_stride[1], pix, 8, mx, my, bw/2, bh/2 ); \
        cost += h->pixf.satd[i_pixel+3]( m->p_fenc[1], m->i_stride[1], pix, 8 ); \
        if( cost < bcost ) \
        { \
            h->mc.mc_chroma( m->p_fref[5], m->i_stride[1], pix, 8, mx, my, bw/2, bh/2 ); \
            cost += h->pixf.satd[i_pixel+3]( m->p_fenc[2], m->i_stride[1], pix, 8 ); \
        } \
    } \
    if( cost < bcost ) \
    {                  \
        bcost = cost;  \
        bdir = dir;    \
    } \
}

static void refine_subpel( x264_t *h, x264_me_t *m, int hpel_iters, int qpel_iters )
{
    const int bw = x264_pixel_size[m->i_pixel].w;
    const int bh = x264_pixel_size[m->i_pixel].h;
    const int16_t *p_cost_mvx = m->p_cost_mv - m->mvp[0];
    const int16_t *p_cost_mvy = m->p_cost_mv - m->mvp[1];
    const int i_pixel = m->i_pixel;
    const int b_chroma_me = h->mb.b_chroma_me && i_pixel <= PIXEL_8x8;

    DECLARE_ALIGNED( uint8_t, pix[16*16], 16 );
    int step, i;

    int bmx = m->mv[0];
    int bmy = m->mv[1];

    for( step = 2; step >= 1; step-- )
    {
	for( i = step>1 ? hpel_iters : qpel_iters; i > 0; i-- )
        {
            int bcost = COST_MAX;
            int bdir = 0;
            COST_MV( bmx, bmy - step, 0 );
            COST_MV( bmx, bmy + step, 1 );
            COST_MV( bmx - step, bmy, 2 );
            COST_MV( bmx + step, bmy, 3 );
    
            if( bcost < m->cost )
            {
                m->cost = bcost;
                if( bdir == 0 )      bmy -= step;
                else if( bdir == 1 ) bmy += step;
                else if( bdir == 2 ) bmx -= step;
                else if( bdir == 3 ) bmx += step;
            }
            else break;
	}
    }

    m->mv[0] = bmx;
    m->mv[1] = bmy;
    m->cost_mv = p_cost_mvx[ bmx ] + p_cost_mvy[ bmy ];
}


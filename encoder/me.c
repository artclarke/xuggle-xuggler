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

#include "../common/common.h"
#include "me.h"

/* presets selected from good points on the speed-vs-quality curve of several test videos
 * subpel_iters[i_subpel_refine] = { refine_hpel, refine_qpel, me_hpel, me_qpel }
 * where me_* are the number of EPZS iterations run on all candidate block types,
 * and refine_* are run only on the winner. */
const static int subpel_iterations[][4] = 
   {{1,0,0,0},
    {1,1,0,0},
    {1,2,0,0},
    {0,2,1,0},
    {0,2,1,1},
    {0,2,1,2}};

static void refine_subpel( x264_t *h, x264_me_t *m, int hpel_iters, int qpel_iters );

#define COST_MV( mx, my ) \
{ \
    int cost = h->pixf.sad[i_pixel]( m->p_fenc, m->i_stride,       \
                   &p_fref[(my)*m->i_stride+(mx)], m->i_stride ) + \
               m->lm * ( bs_size_se(((mx)<<2) - m->mvp[0] ) +      \
                         bs_size_se(((my)<<2) - m->mvp[1] ) );     \
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
    int bmx, bmy, bcost;
    int omx, omy;
    uint8_t *p_fref = m->p_fref;
    int i_iter;


    /* init with mvp */
    /* XXX: We don't need to clamp because the way diamond work, we will
     * never go outside padded picture, and predict mv won't compute vector
     * with componant magnitude greater.
     * XXX: if some vector can go outside, (accelerator, ....) you need to clip
     * them yourself */
    bmx = x264_clip3( ( m->mvp[0] + 2 ) >> 2, -m->i_mv_range, m->i_mv_range );
    bmy = x264_clip3( ( m->mvp[1] + 2 ) >> 2, -m->i_mv_range, m->i_mv_range );

    bcost = h->pixf.sad[i_pixel]( m->p_fenc, m->i_stride,
                &p_fref[bmy * m->i_stride + bmx], m->i_stride );

    /* try extra predictors if provided */
    for( i_iter = 0; i_iter < i_mvc; i_iter++ )
    {
        const int mx = x264_clip3( ( mvc[i_iter][0] + 2 ) >> 2, -m->i_mv_range, m->i_mv_range );
        const int my = x264_clip3( ( mvc[i_iter][1] + 2 ) >> 2, -m->i_mv_range, m->i_mv_range );
        if( mx != bmx || my != bmy )
            COST_MV( mx, my );
    }
    
    COST_MV( 0, 0 );

    if( h->param.analyse.i_subpel_refine >= 2 )
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
    m->cost = h->pixf.satd[i_pixel]( m->p_fenc, m->i_stride,
                    &p_fref[bmy * m->i_stride + bmx], m->i_stride ) +
                m->lm * ( bs_size_se( m->mv[0] - m->mvp[0] ) +
                          bs_size_se( m->mv[1] - m->mvp[1] ) );

    /* subpel refine */
    if( h->param.analyse.i_subpel_refine >= 3 )
    {
        int hpel, qpel;

        /* early termination (when examining multiple reference frames) */
        if( p_fullpel_thresh )
        {
            if( (m->cost*7)>>3 > *p_fullpel_thresh )
                return;
            else if( m->cost < *p_fullpel_thresh )
                *p_fullpel_thresh = m->cost;
        }

        hpel = subpel_iterations[h->param.analyse.i_subpel_refine][2];
        qpel = subpel_iterations[h->param.analyse.i_subpel_refine][3];
        refine_subpel( h, m, hpel, qpel );
    }
}
#undef COST_MV

void x264_me_refine_qpel( x264_t *h, x264_me_t *m )
{
    int hpel = subpel_iterations[h->param.analyse.i_subpel_refine][0];
    int qpel = subpel_iterations[h->param.analyse.i_subpel_refine][1];
//  if( hpel || qpel )
	refine_subpel( h, m, hpel, qpel );
}

static void refine_subpel( x264_t *h, x264_me_t *m, int hpel_iters, int qpel_iters )
{
    const int bw = x264_pixel_size[m->i_pixel].w;
    const int bh = x264_pixel_size[m->i_pixel].h;

    DECLARE_ALIGNED( uint8_t, pix[4][16*16], 16 );
    int cost[4];
    int best;
    int step, i;

    int bmx = m->mv[0];
    int bmy = m->mv[1];

    for( step = 2; step >= 1; step-- )
    {
	for( i = step>1 ? hpel_iters : qpel_iters; i > 0; i-- )
        {
            h->mc[MC_LUMA]( m->p_fref, m->i_stride, pix[0], 16, bmx + 0, bmy - step, bw, bh );
            h->mc[MC_LUMA]( m->p_fref, m->i_stride, pix[1], 16, bmx + 0, bmy + step, bw, bh );
            h->mc[MC_LUMA]( m->p_fref, m->i_stride, pix[2], 16, bmx - step, bmy + 0, bw, bh );
            h->mc[MC_LUMA]( m->p_fref, m->i_stride, pix[3], 16, bmx + step, bmy + 0, bw, bh );
    
            cost[0] = h->pixf.satd[m->i_pixel]( m->p_fenc, m->i_stride, pix[0], 16 ) +
                      m->lm * ( bs_size_se( bmx + 0 - m->mvp[0] ) + bs_size_se( bmy - step - m->mvp[1] ) );
            cost[1] = h->pixf.satd[m->i_pixel]( m->p_fenc, m->i_stride, pix[1], 16 ) +
                      m->lm * ( bs_size_se( bmx + 0 - m->mvp[0] ) + bs_size_se( bmy + step - m->mvp[1] ) );
            cost[2] = h->pixf.satd[m->i_pixel]( m->p_fenc, m->i_stride, pix[2], 16 ) +
                      m->lm * ( bs_size_se( bmx - step - m->mvp[0] ) + bs_size_se( bmy + 0 - m->mvp[1] ) );
            cost[3] = h->pixf.satd[m->i_pixel]( m->p_fenc, m->i_stride, pix[3], 16 ) +
                      m->lm * ( bs_size_se( bmx + step - m->mvp[0] ) + bs_size_se( bmy + 0 - m->mvp[1] ) );
    
            best = 0;
            if( cost[1] < cost[0] )    best = 1;
            if( cost[2] < cost[best] ) best = 2;
            if( cost[3] < cost[best] ) best = 3;
    
            if( cost[best] < m->cost )
            {
                m->cost = cost[best];
                if( best == 0 )      bmy -= step;
                else if( best == 1 ) bmy += step;
                else if( best == 2 ) bmx -= step;
                else if( best == 3 ) bmx += step;
            }
            else break;
	}
    }

    m->mv[0] = bmx;
    m->mv[1] = bmy;
}


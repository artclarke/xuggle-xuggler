/*****************************************************************************
 * me.c: h264 encoder library (Motion Estimation)
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar
 * $Id: me.c,v 1.1 2004/06/03 19:27:08 fenrir Exp $
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

void x264_me_search( x264_t *h, x264_me_t *m, int (*mvc)[2], int i_mvc )
{
    const int i_pixel = m->i_pixel;
    int bcost;
    int bmx, bmy;
    uint8_t *p_fref = m->p_fref;
    int i_iter;
    int hpel, qpel;


    /* init with mvp */
    /* XXX: We don't need to clamp because the way diamond work, we will
     * never go outside padded picture, and predict mv won't compute vector
     * with componant magnitude greater.
     * XXX: if some vector can go outside, (accelerator, ....) you need to clip
     * them yourself */
    bmx = x264_clip3( ( m->mvp[0] + 2 ) >> 2, -m->i_mv_range, m->i_mv_range );
    bmy = x264_clip3( ( m->mvp[1] + 2 ) >> 2, -m->i_mv_range, m->i_mv_range );

    p_fref = &m->p_fref[bmy * m->i_stride + bmx];
    bcost = h->pixf.sad[i_pixel]( m->p_fenc, m->i_stride, p_fref, m->i_stride );


    /* try a candidate if provided */
    for( i_iter = 0; i_iter < i_mvc; i_iter++ )
    {
        const int mx = x264_clip3( ( mvc[i_iter][0] + 2 ) >> 2, -m->i_mv_range, m->i_mv_range );
        const int my = x264_clip3( ( mvc[i_iter][1] + 2 ) >> 2, -m->i_mv_range, m->i_mv_range );
        if( mx != bmx || my != bmy )
        {
            uint8_t *p_fref2 = &m->p_fref[my*m->i_stride+mx];
            int cost = h->pixf.sad[i_pixel]( m->p_fenc, m->i_stride, p_fref2, m->i_stride ) +
                       m->lm * ( bs_size_se( mx - m->mvp[0] ) + bs_size_se( my - m->mvp[1] ) );
            if( cost < bcost )
            {
                bmx = mx;
                bmy = my;
                bcost = cost;
                p_fref = p_fref2;
            }
        }
    }

    /* Don't need to test mv_range each time, we won't go outside picture+padding */
    /* diamond */
    for( i_iter = 0; i_iter < 16; i_iter++ )
    {
        int best = 0;
        int cost[4];

#define COST_MV( c, dx, dy ) \
        (c) = h->pixf.sad[i_pixel]( m->p_fenc, m->i_stride,                    \
                               &p_fref[(dy)*m->i_stride+(dx)], m->i_stride ) + \
              m->lm * ( bs_size_se(((bmx+(dx))<<2) - m->mvp[0] ) +         \
                        bs_size_se(((bmy+(dy))<<2) - m->mvp[1] ) )

        COST_MV( cost[0],  0, -1 );
        COST_MV( cost[1],  0,  1 );
        COST_MV( cost[2], -1,  0 );
        COST_MV( cost[3],  1,  0 );
#undef COST_MV

        if( cost[1] < cost[0] )    best = 1;
        if( cost[2] < cost[best] ) best = 2;
        if( cost[3] < cost[best] ) best = 3;

        if( bcost <= cost[best] )
            break;

        bcost = cost[best];

        if( best == 0 ) {
            bmy--;
            p_fref -= m->i_stride;
        } else if( best == 1 ) {
            bmy++;
            p_fref += m->i_stride;
        } else if( best == 2 ) {
            bmx--;
            p_fref--;
        } else if( best == 3 ) {
            bmx++;
            p_fref++;
        }
    }

    /* -> qpel mv */
    m->mv[0] = bmx << 2;
    m->mv[1] = bmy << 2;

    /* compute the real cost */
    m->cost = h->pixf.satd[i_pixel]( m->p_fenc, m->i_stride, p_fref, m->i_stride ) +
                m->lm * ( bs_size_se( m->mv[0] - m->mvp[0] ) +
                          bs_size_se( m->mv[1] - m->mvp[1] ) );

    hpel = subpel_iterations[h->param.analyse.i_subpel_refine][2];
    qpel = subpel_iterations[h->param.analyse.i_subpel_refine][3];
    if( hpel || qpel )
	refine_subpel( h, m, hpel, qpel );
}

void x264_me_refine_qpel( x264_t *h, x264_me_t *m )
{
    int hpel = subpel_iterations[h->param.analyse.i_subpel_refine][0];
    int qpel = subpel_iterations[h->param.analyse.i_subpel_refine][1];
    if( hpel || qpel )
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


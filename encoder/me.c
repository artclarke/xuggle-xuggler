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
#include <stdint.h>

#include "../core/common.h"
#include "me.h"

void x264_me_search( x264_t *h, x264_me_t *m )
{
    const int i_pixel = m->i_pixel;
    int bcost;
    int bmx, bmy;
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

    p_fref = &m->p_fref[bmy * m->i_stride + bmx];
    bcost = h->pixf.sad[i_pixel]( m->p_fenc, m->i_stride, p_fref, m->i_stride );


    /* try a candidate if provided */
    if( m->b_mvc )
    {
        const int mx = x264_clip3( ( m->mvc[0] + 2 ) >> 2, -m->i_mv_range, m->i_mv_range );
        const int my = x264_clip3( ( m->mvc[1] + 2 ) >> 2, -m->i_mv_range, m->i_mv_range );
        uint8_t *p_fref2 = &m->p_fref[my*m->i_stride+mx];
        int cost = h->pixf.sad[i_pixel]( m->p_fenc, m->i_stride, p_fref2, m->i_stride ) +
                   m->lm * ( bs_size_se( m->mvc[0] - m->mvp[0] ) + bs_size_se( m->mvc[1] - m->mvp[1] ) );
        if( cost < bcost )
        {
            bmx = mx;
            bmy = my;
            bcost = cost;
            p_fref = p_fref2;
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
}

void x264_me_refine_qpel( x264_t *h, x264_me_t *m )
{
    const int bw = x264_pixel_size[m->i_pixel].w;
    const int bh = x264_pixel_size[m->i_pixel].h;

    DECLARE_ALIGNED( uint8_t, pix[4][16*16], 16 );
    int cost[4];
    int best;

    int bmx = m->mv[0];
    int bmy = m->mv[1];

    h->mc[MC_LUMA]( m->p_fref, m->i_stride, pix[0], 16, bmx + 0, bmy - 2, bw, bh );
    h->mc[MC_LUMA]( m->p_fref, m->i_stride, pix[1], 16, bmx + 0, bmy + 2, bw, bh );
    h->mc[MC_LUMA]( m->p_fref, m->i_stride, pix[2], 16, bmx - 2, bmy + 0, bw, bh );
    h->mc[MC_LUMA]( m->p_fref, m->i_stride, pix[3], 16, bmx + 2, bmy + 0, bw, bh );

    cost[0] = h->pixf.satd[m->i_pixel]( m->p_fenc, m->i_stride, pix[0], 16 ) +
              m->lm * ( bs_size_se( bmx + 0 - m->mvp[0] ) + bs_size_se( bmy - 2 - m->mvp[1] ) );
    cost[1] = h->pixf.satd[m->i_pixel]( m->p_fenc, m->i_stride, pix[1], 16 ) +
              m->lm * ( bs_size_se( bmx + 0 - m->mvp[0] ) + bs_size_se( bmy + 2 - m->mvp[1] ) );
    cost[2] = h->pixf.satd[m->i_pixel]( m->p_fenc, m->i_stride, pix[2], 16 ) +
              m->lm * ( bs_size_se( bmx - 2 - m->mvp[0] ) + bs_size_se( bmy + 0 - m->mvp[1] ) );
    cost[3] = h->pixf.satd[m->i_pixel]( m->p_fenc, m->i_stride, pix[3], 16 ) +
              m->lm * ( bs_size_se( bmx + 2 - m->mvp[0] ) + bs_size_se( bmy + 0 - m->mvp[1] ) );

    best = 0;
    if( cost[1] < cost[0] )    best = 1;
    if( cost[2] < cost[best] ) best = 2;
    if( cost[3] < cost[best] ) best = 3;

    if( cost[best] < m->cost )
    {
        m->cost = cost[best];
        if( best == 0 )      bmy -= 2;
        else if( best == 1 ) bmy += 2;
        else if( best == 2 ) bmx -= 2;
        else if( best == 3 ) bmx += 2;
    }

    h->mc[MC_LUMA]( m->p_fref, m->i_stride, pix[0], 16, bmx + 0, bmy - 1, bw, bh );
    h->mc[MC_LUMA]( m->p_fref, m->i_stride, pix[1], 16, bmx + 0, bmy + 1, bw, bh );
    h->mc[MC_LUMA]( m->p_fref, m->i_stride, pix[2], 16, bmx - 1, bmy + 0, bw, bh );
    h->mc[MC_LUMA]( m->p_fref, m->i_stride, pix[3], 16, bmx + 1, bmy + 0, bw, bh );

    cost[0] = h->pixf.satd[m->i_pixel]( m->p_fenc, m->i_stride, pix[0], 16 ) +
              m->lm * ( bs_size_se( bmx + 0 - m->mvp[0] ) + bs_size_se( bmy - 1 - m->mvp[1] ) );
    cost[1] = h->pixf.satd[m->i_pixel]( m->p_fenc, m->i_stride, pix[1], 16 ) +
              m->lm * ( bs_size_se( bmx + 0 - m->mvp[0] ) + bs_size_se( bmy + 1 - m->mvp[1] ) );
    cost[2] = h->pixf.satd[m->i_pixel]( m->p_fenc, m->i_stride, pix[2], 16 ) +
              m->lm * ( bs_size_se( bmx - 1 - m->mvp[0] ) + bs_size_se( bmy + 0 - m->mvp[1] ) );
    cost[3] = h->pixf.satd[m->i_pixel]( m->p_fenc, m->i_stride, pix[3], 16 ) +
              m->lm * ( bs_size_se( bmx + 1 - m->mvp[0] ) + bs_size_se( bmy + 0 - m->mvp[1] ) );

    best = 0;
    if( cost[1] < cost[0] )    best = 1;
    if( cost[2] < cost[best] ) best = 2;
    if( cost[3] < cost[best] ) best = 3;

    if( cost[best] < m->cost )
    {
        m->cost = cost[best];
        if( best == 0 )      bmy--;
        else if( best == 1 ) bmy++;
        else if( best == 2 ) bmx--;
        else if( best == 3 ) bmx++;
    }

    m->mv[0] = bmx;
    m->mv[1] = bmy;
}

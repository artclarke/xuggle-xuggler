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
    {0,0,2,2}};

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

#define COST_MV_PDE( mx, my ) \
{ \
    int cost = h->pixf.sad_pde[i_pixel]( m->p_fenc[0], m->i_stride[0], \
                   &p_fref[(my)*m->i_stride[0]+(mx)], m->i_stride[0], \
                   bcost - p_cost_mvx[ (mx)<<2 ] - p_cost_mvy[ (my)<<2 ] ); \
    if( cost < bcost - p_cost_mvx[ (mx)<<2 ] - p_cost_mvy[ (my)<<2 ] ) \
    {                  \
        bcost = cost + p_cost_mvx[ (mx)<<2 ] + p_cost_mvy[ (my)<<2 ];  \
        bmx = mx;      \
        bmy = my;      \
    } \
}

#define DIA1_ITER( mx, my )\
    {\
        omx = mx; omy = my;\
        COST_MV( omx  , omy-1 );/*  1  */\
        COST_MV( omx  , omy+1 );/* 101 */\
        COST_MV( omx-1, omy   );/*  1  */\
        COST_MV( omx+1, omy   );\
    }

#define DIA2 \
    {\
        COST_MV( omx  , omy-2 );\
        COST_MV( omx-1, omy-1 );/*   1   */\
        COST_MV( omx+1, omy-1 );/*  1 1  */\
        COST_MV( omx-2, omy   );/* 1 0 1 */\
        COST_MV( omx+2, omy   );/*  1 1  */\
        COST_MV( omx-1, omy+1 );/*   1   */\
        COST_MV( omx+1, omy+1 );\
        COST_MV( omx  , omy+2 );\
    }\

#define OCT2 \
    {\
        COST_MV( omx-1, omy-2 );\
        COST_MV( omx+1, omy-2 );/*  1 1  */\
        COST_MV( omx-2, omy-1 );/* 1   1 */\
        COST_MV( omx+2, omy-1 );/*   0   */\
        COST_MV( omx-2, omy+1 );/* 1   1 */\
        COST_MV( omx+2, omy+1 );/*  1 1  */\
        COST_MV( omx-1, omy+2 );\
        COST_MV( omx+1, omy+2 );\
    }

#define CROSS( start, x_max, y_max ) \
    { \
        for( i = start; i < x_max; i+=2 ) \
        { \
            if( omx + i <= mv_x_max ) \
                COST_MV( omx + i, omy ); \
            if( omx - i >= mv_x_min ) \
                COST_MV( omx - i, omy ); \
        } \
        for( i = start; i < y_max; i+=2 ) \
        { \
            if( omy + i <= mv_y_max ) \
                COST_MV( omx, omy + i ); \
            if( omy - i >= mv_y_min ) \
                COST_MV( omx, omy - i ); \
        } \
    }


void x264_me_search_ref( x264_t *h, x264_me_t *m, int (*mvc)[2], int i_mvc, int *p_halfpel_thresh )
{
    const int i_pixel = m->i_pixel;
    int i_me_range = h->param.analyse.i_me_range;
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
        p_cost_mvx = m->p_cost_mv - x264_clip3( m->mvp[0], h->mb.mv_min_spel[0], h->mb.mv_max_spel[0] );
        p_cost_mvy = m->p_cost_mv - x264_clip3( m->mvp[1], h->mb.mv_min_spel[1], h->mb.mv_max_spel[1] );
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
             * as in JM, except with different early termination */

            static const int x264_pixel_size_shift[7] = { 0, 1, 1, 2, 3, 3, 4 };

            int ucost1, ucost2;
            int cross_start = 1;

            /* refine predictors */
            ucost1 = bcost;
            DIA1_ITER( pmx, pmy );
            if( pmx || pmy )
                DIA1_ITER( 0, 0 );

            if(i_pixel == PIXEL_4x4)
                goto me_hex2;

            ucost2 = bcost;
            if( (bmx || bmy) && (bmx!=pmx || bmy!=pmy) )
                DIA1_ITER( bmx, bmy );
            if( bcost == ucost2 )
                cross_start = 3;
            omx = bmx; omy = bmy;

            /* early termination */
#define SAD_THRESH(v) ( bcost < ( v >> x264_pixel_size_shift[i_pixel] ) )
            if( bcost == ucost2 && SAD_THRESH(2000) )
            {
                DIA2;
                if( bcost == ucost1 && SAD_THRESH(500) )
                    break;
                if( bcost == ucost2 )
                {
                    int range = (i_me_range>>1) | 1;
                    CROSS( 3, range, range );
                    OCT2;
                    if( bcost == ucost2 )
                        break;
                    cross_start = range + 2;
                }
            }

            /* adaptive search range */
            if( i_mvc ) 
            {
                /* range multipliers based on casual inspection of some statistics of
                 * average distance between current predictor and final mv found by ESA.
                 * these have not been tuned much by actual encoding. */
                static const int range_mul[4][4] =
                {
                    { 3, 3, 4, 4 },
                    { 3, 4, 4, 4 },
                    { 4, 4, 4, 5 },
                    { 4, 4, 5, 6 },
                };
                int mvd;
                int sad_ctx, mvd_ctx;

                if( i_mvc == 1 )
                {
                    if( i_pixel == PIXEL_16x16 )
                        /* mvc is probably the same as mvp, so the difference isn't meaningful.
                         * but prediction usually isn't too bad, so just use medium range */
                        mvd = 25;
                    else
                        mvd = abs( m->mvp[0] - mvc[0][0] )
                            + abs( m->mvp[1] - mvc[0][1] );
                }
                else
                {
                    /* calculate the degree of agreement between predictors. */
                    /* in 16x16, mvc includes all the neighbors used to make mvp,
                     * so don't count mvp separately. */
                    int i_denom = i_mvc - 1;
                    mvd = 0;
                    if( i_pixel != PIXEL_16x16 )
                    {
                        mvd = abs( m->mvp[0] - mvc[0][0] )
                            + abs( m->mvp[1] - mvc[0][1] );
                        i_denom++;
                    }
                    for( i = 0; i < i_mvc-1; i++ )
                        mvd += abs( mvc[i][0] - mvc[i+1][0] )
                             + abs( mvc[i][1] - mvc[i+1][1] );
                    mvd /= i_denom; //FIXME idiv
                }

                sad_ctx = SAD_THRESH(1000) ? 0
                        : SAD_THRESH(2000) ? 1
                        : SAD_THRESH(4000) ? 2 : 3;
                mvd_ctx = mvd < 10 ? 0
                        : mvd < 20 ? 1
                        : mvd < 40 ? 2 : 3;

                i_me_range = i_me_range * range_mul[mvd_ctx][sad_ctx] / 4;
            }

            /* FIXME if the above DIA2/OCT2/CROSS found a new mv, it has not updated omx/omy.
             * we are still centered on the same place as the DIA2. is this desirable? */
            CROSS( cross_start, i_me_range, i_me_range/2 );

            /* 5x5 ESA */
            omx = bmx; omy = bmy;
            for( i = (bcost == ucost2) ? 4 : 0; i < 24; i++ )
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
                static const int hex4[16][2] = {
                    {-4, 2}, {-4, 1}, {-4, 0}, {-4,-1}, {-4,-2},
                    { 4,-2}, { 4,-1}, { 4, 0}, { 4, 1}, { 4, 2},
                    { 2, 3}, { 0, 4}, {-2, 3},
                    {-2,-3}, { 0,-4}, { 2,-3},
                };
                const int bounds_check = 4*i > X264_MIN4( mv_x_max-omx, mv_y_max-omy, omx-mv_x_min, omy-mv_y_min );

                if( h->pixf.sad_pde[i_pixel] )
                {
                    for( j = 0; j < 16; j++ )
                    {
                        int mx = omx + hex4[j][0]*i;
                        int my = omy + hex4[j][1]*i;
                        if( !bounds_check || ( mx >= mv_x_min && mx <= mv_x_max
                                            && my >= mv_y_min && my <= mv_y_max ) )
                            COST_MV_PDE( mx, my );
                    }
                }
                else
                {
                    for( j = 0; j < 16; j++ )
                    {
                        int mx = omx + hex4[j][0]*i;
                        int my = omy + hex4[j][1]*i;
                        if( !bounds_check || ( mx >= mv_x_min && mx <= mv_x_max
                                            && my >= mv_y_min && my <= mv_y_max ) )
                            COST_MV( mx, my );
                    }
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
            int mx, my;
#if 0
            /* plain old exhaustive search */
            for( my = min_y; my <= max_y; my++ )
                for( mx = min_x; mx <= max_x; mx++ )
                    COST_MV( mx, my );
#else
            /* successive elimination by comparing DC before a full SAD,
             * because sum(abs(diff)) >= abs(diff(sum)). */
            const int stride = m->i_stride[0];
            const int dw = x264_pixel_size[i_pixel].w;
            const int dh = x264_pixel_size[i_pixel].h * stride;
            static uint8_t zero[16*16] = {0,};
            const int enc_dc = h->pixf.sad[i_pixel]( m->p_fenc[0], stride, zero, 16 );
            const uint16_t *integral_base = &m->integral[ -1 - 1*stride ];

            if( h->pixf.sad_pde[i_pixel] )
            {
                for( my = min_y; my <= max_y; my++ )
                    for( mx = min_x; mx <= max_x; mx++ )
                    {
                        const uint16_t *integral = &integral_base[ mx + my * stride ];
                        const uint16_t ref_dc = integral[  0 ] + integral[ dh + dw ]
                                              - integral[ dw ] - integral[ dh ];
                        const int bsad = bcost - p_cost_mvx[ (mx)<<2 ] - p_cost_mvy[ (my)<<2 ];
                        if( abs( ref_dc - enc_dc ) < bsad )
                            COST_MV_PDE( mx, my );
                    }
            }
            else
            {
                for( my = min_y; my <= max_y; my++ )
                    for( mx = min_x; mx <= max_x; mx++ )
                    {
                        const uint16_t *integral = &integral_base[ mx + my * stride ];
                        const uint16_t ref_dc = integral[  0 ] + integral[ dh + dw ]
                                              - integral[ dw ] - integral[ dh ];
                        const int bsad = bcost - p_cost_mvx[ (mx)<<2 ] - p_cost_mvy[ (my)<<2 ];
                        if( abs( ref_dc - enc_dc ) < bsad )
                            COST_MV( mx, my );
                    }
            }
#endif
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

#define COST_MV_SAD( mx, my, dir ) \
if( b_refine_qpel || (dir^1) != odir ) \
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
        bdir = dir;    \
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


    /* try the subpel component of the predicted mv */
    if( hpel_iters )
    {
        int mx = x264_clip3( m->mvp[0], h->mb.mv_min_spel[0], h->mb.mv_max_spel[0] );
        int my = x264_clip3( m->mvp[1], h->mb.mv_min_spel[1], h->mb.mv_max_spel[1] );
        if( mx != bmx || my != bmy )
            COST_MV_SAD( mx, my, -1 );
    }
    
    /* hpel search */
    bdir = -1;
    for( i = hpel_iters; i > 0; i-- )
    {
        odir = bdir;
        omx = bmx;
        omy = bmy;
        COST_MV_SAD( omx, omy - 2, 0 );
        COST_MV_SAD( omx, omy + 2, 1 );
        COST_MV_SAD( omx - 2, omy, 2 );
        COST_MV_SAD( omx + 2, omy, 3 );
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

#define BIME_CACHE( dx, dy ) \
{ \
    int i = 4 + 3*dx + dy; \
    h->mc.mc_luma( m0->p_fref, m0->i_stride[0], pix0[i], bw, om0x+dx, om0y+dy, bw, bh ); \
    h->mc.mc_luma( m1->p_fref, m1->i_stride[0], pix1[i], bw, om1x+dx, om1y+dy, bw, bh ); \
}

#define BIME_CACHE2(a,b) \
    BIME_CACHE(a,b) \
    BIME_CACHE(-(a),-(b))

#define COST_BIMV_SATD( m0x, m0y, m1x, m1y ) \
if( pass == 0 || !visited[(m0x)&7][(m0y)&7][(m1x)&7][(m1y)&7] ) \
{ \
    int cost; \
    int i0 = 4 + 3*(m0x-om0x) + (m0y-om0y); \
    int i1 = 4 + 3*(m1x-om1x) + (m1y-om1y); \
    visited[(m0x)&7][(m0y)&7][(m1x)&7][(m1y)&7] = 1; \
    memcpy( pix, pix0[i0], bs ); \
    if( i_weight == 32 ) \
        h->mc.avg[i_pixel]( pix, bw, pix1[i1], bw ); \
    else \
        h->mc.avg_weight[i_pixel]( pix, bw, pix1[i1], bw, i_weight ); \
    cost = h->pixf.mbcmp[i_pixel]( m0->p_fenc[0], m0->i_stride[0], pix, bw ) \
         + p_cost_m0x[ m0x ] + p_cost_m0y[ m0y ] \
         + p_cost_m1x[ m1x ] + p_cost_m1y[ m1y ]; \
    if( cost < bcost ) \
    {                  \
        bcost = cost;  \
        bm0x = m0x;    \
        bm0y = m0y;    \
        bm1x = m1x;    \
        bm1y = m1y;    \
    } \
}

#define CHECK_BIDIR(a,b,c,d) \
    COST_BIMV_SATD(om0x+a, om0y+b, om1x+c, om1y+d)

#define CHECK_BIDIR2(a,b,c,d) \
    CHECK_BIDIR(a,b,c,d) \
    CHECK_BIDIR(-(a),-(b),-(c),-(d))

#define CHECK_BIDIR8(a,b,c,d) \
    CHECK_BIDIR2(a,b,c,d) \
    CHECK_BIDIR2(b,c,d,a) \
    CHECK_BIDIR2(c,d,a,b) \
    CHECK_BIDIR2(d,a,b,c)

int x264_me_refine_bidir( x264_t *h, x264_me_t *m0, x264_me_t *m1, int i_weight )
{
    const int i_pixel = m0->i_pixel;
    const int bw = x264_pixel_size[i_pixel].w;
    const int bh = x264_pixel_size[i_pixel].h;
    const int bs = bw*bh;
    const int16_t *p_cost_m0x = m0->p_cost_mv - x264_clip3( m0->mvp[0], h->mb.mv_min_spel[0], h->mb.mv_max_spel[0] );
    const int16_t *p_cost_m0y = m0->p_cost_mv - x264_clip3( m0->mvp[1], h->mb.mv_min_spel[0], h->mb.mv_max_spel[0] );
    const int16_t *p_cost_m1x = m1->p_cost_mv - x264_clip3( m1->mvp[0], h->mb.mv_min_spel[0], h->mb.mv_max_spel[0] );
    const int16_t *p_cost_m1y = m1->p_cost_mv - x264_clip3( m1->mvp[1], h->mb.mv_min_spel[0], h->mb.mv_max_spel[0] );
    DECLARE_ALIGNED( uint8_t, pix0[9][16*16], 16 );
    DECLARE_ALIGNED( uint8_t, pix1[9][16*16], 16 );
    DECLARE_ALIGNED( uint8_t, pix[16*16], 16 );
    int bm0x = m0->mv[0], om0x = bm0x;
    int bm0y = m0->mv[1], om0y = bm0y;
    int bm1x = m1->mv[0], om1x = bm1x;
    int bm1y = m1->mv[1], om1y = bm1y;
    int bcost = COST_MAX;
    int pass = 0;
    uint8_t visited[8][8][8][8];
    memset( visited, 0, sizeof(visited) );

    BIME_CACHE( 0, 0 );
    CHECK_BIDIR( 0, 0, 0, 0 );

    for( pass = 0; pass < 8; pass++ )
    {
        /* check all mv pairs that differ in at most 2 components from the current mvs. */
        /* doesn't do chroma ME. this probably doesn't matter, as the gains
         * from bidir ME are the same with and without chroma ME. */

        BIME_CACHE2( 1, 0 );
        BIME_CACHE2( 0, 1 );
        BIME_CACHE2( 1, 1 );
        BIME_CACHE2( 1,-1 );

        CHECK_BIDIR8( 0, 0, 0, 1 );
        CHECK_BIDIR8( 0, 0, 1, 1 );
        CHECK_BIDIR2( 0, 1, 0, 1 );
        CHECK_BIDIR2( 1, 0, 1, 0 );
        CHECK_BIDIR8( 0, 0,-1, 1 );
        CHECK_BIDIR2( 0,-1, 0, 1 );
        CHECK_BIDIR2(-1, 0, 1, 0 );

        if( om0x == bm0x && om0y == bm0y && om1x == bm1x && om1y == bm1y )
            break;

        om0x = bm0x;
        om0y = bm0y;
        om1x = bm1x;
        om1y = bm1y;
        BIME_CACHE( 0, 0 );
    }

    m0->mv[0] = bm0x;
    m0->mv[1] = bm0y;
    m1->mv[0] = bm1x;
    m1->mv[1] = bm1y;
    return bcost;
}

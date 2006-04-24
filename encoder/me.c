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
    {0,0,2,2},
    {0,0,2,2}};

static void refine_subpel( x264_t *h, x264_me_t *m, int hpel_iters, int qpel_iters, int *p_halfpel_thresh, int b_refine_qpel );

#define BITS_MVD( mx, my )\
    (p_cost_mvx[(mx)<<2] + p_cost_mvy[(my)<<2])

#define COST_MV( mx, my )\
{\
    int cost = h->pixf.sad[i_pixel]( m->p_fenc[0], FENC_STRIDE,\
                   &p_fref[(my)*m->i_stride[0]+(mx)], m->i_stride[0] )\
             + BITS_MVD(mx,my);\
    COPY3_IF_LT( bcost, cost, bmx, mx, bmy, my );\
}

#define COST_MV_PRED( mx, my ) \
{ \
    int stride = 16; \
    uint8_t *src = h->mc.get_ref( m->p_fref, m->i_stride[0], pix, &stride, mx, my, bw, bh ); \
    int cost = h->pixf.sad[i_pixel]( m->p_fenc[0], FENC_STRIDE, src, stride ) \
             + p_cost_mvx[ mx ] + p_cost_mvy[ my ]; \
    COPY3_IF_LT( bpred_cost, cost, bpred_mx, mx, bpred_my, my ); \
}

#define COST_MV_X3_DIR( m0x, m0y, m1x, m1y, m2x, m2y, costs )\
{\
    uint8_t *pix_base = p_fref + bmx + bmy*m->i_stride[0];\
    h->pixf.sad_x3[i_pixel]( m->p_fenc[0],\
        pix_base + (m0x) + (m0y)*m->i_stride[0],\
        pix_base + (m1x) + (m1y)*m->i_stride[0],\
        pix_base + (m2x) + (m2y)*m->i_stride[0],\
        m->i_stride[0], costs );\
    (costs)[0] += BITS_MVD( bmx+(m0x), bmy+(m0y) );\
    (costs)[1] += BITS_MVD( bmx+(m1x), bmy+(m1y) );\
    (costs)[2] += BITS_MVD( bmx+(m2x), bmy+(m2y) );\
}

#define COST_MV_X4( m0x, m0y, m1x, m1y, m2x, m2y, m3x, m3y )\
{\
    uint8_t *pix_base = p_fref + omx + omy*m->i_stride[0];\
    h->pixf.sad_x4[i_pixel]( m->p_fenc[0],\
        pix_base + (m0x) + (m0y)*m->i_stride[0],\
        pix_base + (m1x) + (m1y)*m->i_stride[0],\
        pix_base + (m2x) + (m2y)*m->i_stride[0],\
        pix_base + (m3x) + (m3y)*m->i_stride[0],\
        m->i_stride[0], costs );\
    costs[0] += BITS_MVD( omx+(m0x), omy+(m0y) );\
    costs[1] += BITS_MVD( omx+(m1x), omy+(m1y) );\
    costs[2] += BITS_MVD( omx+(m2x), omy+(m2y) );\
    costs[3] += BITS_MVD( omx+(m3x), omy+(m3y) );\
    COPY3_IF_LT( bcost, costs[0], bmx, omx+(m0x), bmy, omy+(m0y) );\
    COPY3_IF_LT( bcost, costs[1], bmx, omx+(m1x), bmy, omy+(m1y) );\
    COPY3_IF_LT( bcost, costs[2], bmx, omx+(m2x), bmy, omy+(m2y) );\
    COPY3_IF_LT( bcost, costs[3], bmx, omx+(m3x), bmy, omy+(m3y) );\
}

#define COST_MV_X4_ABS( m0x, m0y, m1x, m1y, m2x, m2y, m3x, m3y )\
{\
    h->pixf.sad_x4[i_pixel]( m->p_fenc[0],\
        p_fref + (m0x) + (m0y)*m->i_stride[0],\
        p_fref + (m1x) + (m1y)*m->i_stride[0],\
        p_fref + (m2x) + (m2y)*m->i_stride[0],\
        p_fref + (m3x) + (m3y)*m->i_stride[0],\
        m->i_stride[0], costs );\
    costs[0] += BITS_MVD( m0x, m0y );\
    costs[1] += BITS_MVD( m1x, m1y );\
    costs[2] += BITS_MVD( m2x, m2y );\
    costs[3] += BITS_MVD( m3x, m3y );\
    COPY3_IF_LT( bcost, costs[0], bmx, m0x, bmy, m0y );\
    COPY3_IF_LT( bcost, costs[1], bmx, m1x, bmy, m1y );\
    COPY3_IF_LT( bcost, costs[2], bmx, m2x, bmy, m2y );\
    COPY3_IF_LT( bcost, costs[3], bmx, m3x, bmy, m3y );\
}

/*  1  */
/* 101 */
/*  1  */
#define DIA1_ITER( mx, my )\
{\
    omx = mx; omy = my;\
    COST_MV_X4( 0,-1, 0,1, -1,0, 1,0 );\
}

#define CROSS( start, x_max, y_max )\
{\
    i = start;\
    if( x_max <= X264_MIN(mv_x_max-omx, omx-mv_x_min) )\
        for( ; i < x_max-2; i+=4 )\
            COST_MV_X4( i,0, -i,0, i+2,0, -i-2,0 );\
    for( ; i < x_max; i+=2 )\
    {\
        if( omx+i <= mv_x_max )\
            COST_MV( omx+i, omy );\
        if( omx-i >= mv_x_min )\
            COST_MV( omx-i, omy );\
    }\
    i = start;\
    if( y_max <= X264_MIN(mv_y_max-omy, omy-mv_y_min) )\
        for( ; i < y_max-2; i+=4 )\
            COST_MV_X4( 0,i, 0,-i, 0,i+2, 0,-i-2 );\
    for( ; i < y_max; i+=2 )\
    {\
        if( omy+i <= mv_y_max )\
            COST_MV( omx, omy+i );\
        if( omy-i >= mv_y_min )\
            COST_MV( omx, omy-i );\
    }\
}

void x264_me_search_ref( x264_t *h, x264_me_t *m, int (*mvc)[2], int i_mvc, int *p_halfpel_thresh )
{
    const int bw = x264_pixel_size[m->i_pixel].w;
    const int bh = x264_pixel_size[m->i_pixel].h;
    const int i_pixel = m->i_pixel;
    int i_me_range = h->param.analyse.i_me_range;
    int bmx, bmy, bcost;
    int bpred_mx = 0, bpred_my = 0, bpred_cost = COST_MAX;
    int omx, omy, pmx, pmy;
    uint8_t *p_fref = m->p_fref[0];
    DECLARE_ALIGNED( uint8_t, pix[16*16], 16 );
    
    int i, j;
    int dir;
    int costs[6];

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

    bmx = x264_clip3( m->mvp[0], mv_x_min*4, mv_x_max*4 );
    bmy = x264_clip3( m->mvp[1], mv_y_min*4, mv_y_max*4 );
    pmx = ( bmx + 2 ) >> 2;
    pmy = ( bmy + 2 ) >> 2;
    bcost = COST_MAX;

    /* try extra predictors if provided */
    if( h->mb.i_subpel_refine >= 3 )
    {
        COST_MV_PRED( bmx, bmy );
        for( i = 0; i < i_mvc; i++ )
        {
             const int mx = x264_clip3( mvc[i][0], mv_x_min*4, mv_x_max*4 );
             const int my = x264_clip3( mvc[i][1], mv_y_min*4, mv_y_max*4 );
             if( mx != bpred_mx || my != bpred_my )
                 COST_MV_PRED( mx, my );
        }
        bmx = ( bpred_mx + 2 ) >> 2;
        bmy = ( bpred_my + 2 ) >> 2;
        COST_MV( bmx, bmy );
    }
    else
    {
        /* check the MVP */
        COST_MV( pmx, pmy );
        /* I don't know why this helps */
        bcost -= BITS_MVD(bmx,bmy);
        
        for( i = 0; i < i_mvc; i++ )
        {
             const int mx = x264_clip3( ( mvc[i][0] + 2 ) >> 2, mv_x_min, mv_x_max );
             const int my = x264_clip3( ( mvc[i][1] + 2 ) >> 2, mv_y_min, mv_y_max );
             if( mx != bmx || my != bmy )
                 COST_MV( mx, my );
        }
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
        dir = -2;

        /* hexagon */
        COST_MV_X3_DIR( -2,0, -1, 2,  1, 2, costs   );
        COST_MV_X3_DIR(  2,0,  1,-2, -1,-2, costs+3 );
        COPY2_IF_LT( bcost, costs[0], dir, 0 );
        COPY2_IF_LT( bcost, costs[1], dir, 1 );
        COPY2_IF_LT( bcost, costs[2], dir, 2 );
        COPY2_IF_LT( bcost, costs[3], dir, 3 );
        COPY2_IF_LT( bcost, costs[4], dir, 4 );
        COPY2_IF_LT( bcost, costs[5], dir, 5 );

        if( dir != -2 )
        {
            static const int hex2[8][2] = {{-1,-2}, {-2,0}, {-1,2}, {1,2}, {2,0}, {1,-2}, {-1,-2}, {-2,0}};
            bmx += hex2[dir+1][0];
            bmy += hex2[dir+1][1];
            /* half hexagon, not overlapping the previous iteration */
            for( i = 1; i < i_me_range/2; i++ )
            {
                static const int mod6[8] = {5,0,1,2,3,4,5,0};
                const int odir = mod6[dir+1];
                COST_MV_X3_DIR( hex2[odir+0][0], hex2[odir+0][1],
                                hex2[odir+1][0], hex2[odir+1][1],
                                hex2[odir+2][0], hex2[odir+2][1],
                                costs );
                dir = -2;
                COPY2_IF_LT( bcost, costs[0], dir, odir-1 );
                COPY2_IF_LT( bcost, costs[1], dir, odir   );
                COPY2_IF_LT( bcost, costs[2], dir, odir+1 );
                if( dir == -2 )
                    break;
                bmx += hex2[dir+1][0];
                bmy += hex2[dir+1][1];
            }
        }
#endif
        /* square refine */
        omx = bmx; omy = bmy;
        COST_MV_X4(  0,-1,  0,1, -1,0, 1,0 );
        COST_MV_X4( -1,-1, -1,1, 1,-1, 1,1 );
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
                COST_MV_X4( 0,-2, -1,-1, 1,-1, -2,0 );
                COST_MV_X4( 2, 0, -1, 1, 1, 1,  0,2 );
                if( bcost == ucost1 && SAD_THRESH(500) )
                    break;
                if( bcost == ucost2 )
                {
                    int range = (i_me_range>>1) | 1;
                    CROSS( 3, range, range );
                    COST_MV_X4( -1,-2, 1,-2, -2,-1, 2,-1 );
                    COST_MV_X4( -2, 1, 2, 1, -1, 2, 1, 2 );
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
            if( bcost != ucost2 )
                COST_MV_X4(  1, 0,  0, 1, -1, 0,  0,-1 );
            COST_MV_X4(  1, 1, -1, 1, -1,-1,  1,-1 );
            COST_MV_X4(  2,-1,  2, 0,  2, 1,  2, 2 );
            COST_MV_X4(  1, 2,  0, 2, -1, 2, -2, 2 );
            COST_MV_X4( -2, 1, -2, 0, -2,-1, -2,-2 );
            COST_MV_X4( -1,-2,  0,-2,  1,-2,  2,-2 );

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

                if( 4*i > X264_MIN4( mv_x_max-omx, omx-mv_x_min,
                                     mv_y_max-omy, omy-mv_y_min ) )
                {
                    for( j = 0; j < 16; j++ )
                    {
                        int mx = omx + hex4[j][0]*i;
                        int my = omy + hex4[j][1]*i;
                        if(    mx >= mv_x_min && mx <= mv_x_max
                            && my >= mv_y_min && my <= mv_y_max )
                            COST_MV( mx, my );
                    }
                }
                else
                {
                    COST_MV_X4( -4*i, 2*i, -4*i, 1*i, -4*i, 0*i, -4*i,-1*i );
                    COST_MV_X4( -4*i,-2*i,  4*i,-2*i,  4*i,-1*i,  4*i, 0*i );
                    COST_MV_X4(  4*i, 1*i,  4*i, 2*i,  2*i, 3*i,  0*i, 4*i );
                    COST_MV_X4( -2*i, 3*i, -2*i,-3*i,  0*i,-4*i,  2*i,-3*i );
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
            const int enc_dc = h->pixf.sad[i_pixel]( m->p_fenc[0], FENC_STRIDE, zero, 16 );
            const uint16_t *integral_base = &m->integral[ -1 - 1*stride ];

            for( my = min_y; my <= max_y; my++ )
            {
                int mvs[3], i_mvs=0;
                for( mx = min_x; mx <= max_x; mx++ )
                {
                    const uint16_t *integral = &integral_base[ mx + my * stride ];
                    const uint16_t ref_dc = integral[  0 ] + integral[ dh + dw ]
                                          - integral[ dw ] - integral[ dh ];
                    const int bsad = bcost - BITS_MVD(mx,my);
                    if( abs( ref_dc - enc_dc ) < bsad )
                    {
                        if( i_mvs == 3 )
                        {
                            COST_MV_X4_ABS( mvs[0],my, mvs[1],my, mvs[2],my, mx,my );
                            i_mvs = 0;
                        }
                        else
                            mvs[i_mvs++] = mx;
                    }
                }
                for( i=0; i<i_mvs; i++ )
                    COST_MV( mvs[i], my );
            }
#endif
        }
        break;
    }

    /* -> qpel mv */
    if( bpred_cost < bcost )
    {
        m->mv[0] = bpred_mx;
        m->mv[1] = bpred_my;
        m->cost = bpred_cost;
    }
    else
    {
        m->mv[0] = bmx << 2;
        m->mv[1] = bmy << 2;
        m->cost = bcost;
    }

    /* compute the real cost */
    m->cost_mv = p_cost_mvx[ m->mv[0] ] + p_cost_mvy[ m->mv[1] ];
    if( bmx == pmx && bmy == pmy && h->mb.i_subpel_refine < 3 )
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
    uint8_t *src = h->mc.get_ref( m->p_fref, m->i_stride[0], pix[0], &stride, mx, my, bw, bh ); \
    int cost = h->pixf.sad[i_pixel]( m->p_fenc[0], FENC_STRIDE, src, stride ) \
             + p_cost_mvx[ mx ] + p_cost_mvy[ my ]; \
    COPY3_IF_LT( bcost, cost, bmx, mx, bmy, my ); \
}

#define COST_MV_SATD( mx, my, dir ) \
if( b_refine_qpel || (dir^1) != odir ) \
{ \
    int stride = 16; \
    uint8_t *src = h->mc.get_ref( m->p_fref, m->i_stride[0], pix[0], &stride, mx, my, bw, bh ); \
    int cost = h->pixf.mbcmp[i_pixel]( m->p_fenc[0], FENC_STRIDE, src, stride ) \
             + p_cost_mvx[ mx ] + p_cost_mvy[ my ]; \
    if( b_chroma_me && cost < bcost ) \
    { \
        h->mc.mc_chroma( m->p_fref[4], m->i_stride[1], pix[0], 8, mx, my, bw/2, bh/2 ); \
        cost += h->pixf.mbcmp[i_pixel+3]( m->p_fenc[1], FENC_STRIDE, pix[0], 8 ); \
        if( cost < bcost ) \
        { \
            h->mc.mc_chroma( m->p_fref[5], m->i_stride[1], pix[0], 8, mx, my, bw/2, bh/2 ); \
            cost += h->pixf.mbcmp[i_pixel+3]( m->p_fenc[2], FENC_STRIDE, pix[0], 8 ); \
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

    DECLARE_ALIGNED( uint8_t, pix[4][16*16], 16 );
    int omx, omy;
    int i;

    int bmx = m->mv[0];
    int bmy = m->mv[1];
    int bcost = m->cost;
    int odir = -1, bdir;


    /* try the subpel component of the predicted mv */
    if( hpel_iters && h->mb.i_subpel_refine < 3 )
    {
        int mx = x264_clip3( m->mvp[0], h->mb.mv_min_spel[0], h->mb.mv_max_spel[0] );
        int my = x264_clip3( m->mvp[1], h->mb.mv_min_spel[1], h->mb.mv_max_spel[1] );
        if( mx != bmx || my != bmy )
            COST_MV_SAD( mx, my );
    }

    /* halfpel diamond search */
    for( i = hpel_iters; i > 0; i-- )
    {
        int omx = bmx, omy = bmy;
        int costs[4];
        int stride = 16; // candidates are either all hpel or all qpel, so one stride is enough
        uint8_t *src0, *src1, *src2, *src3;
        src0 = h->mc.get_ref( m->p_fref, m->i_stride[0], pix[0], &stride, omx, omy-2, bw, bh );
        src2 = h->mc.get_ref( m->p_fref, m->i_stride[0], pix[2], &stride, omx-2, omy, bw, bh );
        if( (omx|omy)&1 )
        {
            src1 = h->mc.get_ref( m->p_fref, m->i_stride[0], pix[1], &stride, omx, omy+2, bw, bh );
            src3 = h->mc.get_ref( m->p_fref, m->i_stride[0], pix[3], &stride, omx+2, omy, bw, bh );
        }
        else
        {
            src1 = src0 + stride;
            src3 = src2 + 1;
        }
        h->pixf.sad_x4[i_pixel]( m->p_fenc[0], src0, src1, src2, src3, stride, costs );
        COPY2_IF_LT( bcost, costs[0] + p_cost_mvx[omx  ] + p_cost_mvy[omy-2], bmy, omy-2 );
        COPY2_IF_LT( bcost, costs[1] + p_cost_mvx[omx  ] + p_cost_mvy[omy+2], bmy, omy+2 );
        COPY3_IF_LT( bcost, costs[2] + p_cost_mvx[omx-2] + p_cost_mvy[omy  ], bmx, omx-2, bmy, omy );
        COPY3_IF_LT( bcost, costs[3] + p_cost_mvx[omx+2] + p_cost_mvy[omy  ], bmx, omx+2, bmy, omy );
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

    /* quarterpel diamond search */
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
    cost = h->pixf.mbcmp[i_pixel]( m0->p_fenc[0], FENC_STRIDE, pix, bw ) \
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

#undef COST_MV_SATD
#define COST_MV_SATD( mx, my, dst ) \
{ \
    int stride = 16; \
    uint8_t *src = h->mc.get_ref( m->p_fref, m->i_stride[0], pix, &stride, mx, my, bw*4, bh*4 ); \
    dst = h->pixf.mbcmp[i_pixel]( m->p_fenc[0], FENC_STRIDE, src, stride ) \
        + p_cost_mvx[mx] + p_cost_mvy[my]; \
    COPY1_IF_LT( bsatd, dst ); \
}

#define COST_MV_RD( mx, my, satd, dir ) \
{ \
    if( satd <= bsatd * SATD_THRESH \
        && (dir^1) != odir \
        && (dir<0 || !p_visited[(mx)+(my)*16]) ) \
    { \
        int cost; \
        cache_mv[0] = cache_mv2[0] = mx; \
        cache_mv[1] = cache_mv2[1] = my; \
        cost = x264_rd_cost_part( h, i_lambda2, i8, m->i_pixel ); \
        COPY3_IF_LT( bcost, cost, bmx, mx, bmy, my ); \
        if(dir>=0) p_visited[(mx)+(my)*16] = 1; \
    } \
}

#define SATD_THRESH 17/16

void x264_me_refine_qpel_rd( x264_t *h, x264_me_t *m, int i_lambda2, int i8 )
{
    // don't have to fill the whole mv cache rectangle
    static const int pixel_mv_offs[] = { 0, 4, 4*8, 0 };
    int16_t *cache_mv = h->mb.cache.mv[0][x264_scan8[i8*4]];
    int16_t *cache_mv2 = cache_mv + pixel_mv_offs[m->i_pixel];
    const int16_t *p_cost_mvx, *p_cost_mvy;
    const int bw = x264_pixel_size[m->i_pixel].w>>2;
    const int bh = x264_pixel_size[m->i_pixel].h>>2;
    const int i_pixel = m->i_pixel;

    DECLARE_ALIGNED( uint8_t, pix[16*16], 16 );
    int bcost = m->i_pixel == PIXEL_16x16 ? m->cost : COST_MAX;
    int bmx = m->mv[0];
    int bmy = m->mv[1];
    int pmx, pmy, omx, omy, i;
    int odir = -1, bdir;
    unsigned bsatd, satds[4];

    int visited[16*13] = {0}; // only need 13x13, but 16 is more convenient
    int *p_visited = &visited[6+6*16];

    if( m->i_pixel != PIXEL_16x16 && i8 != 0 )
        x264_mb_predict_mv( h, 0, i8*4, bw, m->mvp );
    pmx = m->mvp[0];
    pmy = m->mvp[1];
    p_cost_mvx = m->p_cost_mv - pmx;
    p_cost_mvy = m->p_cost_mv - pmy;
    COST_MV_SATD( bmx, bmy, bsatd );
    if( m->i_pixel != PIXEL_16x16 )
        COST_MV_RD( bmx, bmy, 0, -1 );

    /* check the predicted mv */
    if( (bmx != pmx || bmy != pmy)
        && pmx >= h->mb.mv_min_spel[0] && pmx <= h->mb.mv_max_spel[0]
        && pmy >= h->mb.mv_min_spel[1] && pmy <= h->mb.mv_max_spel[1] )
    {
        int satd;
        COST_MV_SATD( pmx, pmy, satd );
        COST_MV_RD( pmx, pmy, satd, -1 );
    }

    /* mark mv and mvp as visited */
    p_visited[0] = 1;
    p_visited -= bmx + bmy*16;
    {
        int mx = bmx ^ m->mv[0] ^ pmx;
        int my = bmy ^ m->mv[1] ^ pmy;
        if( abs(mx-bmx) < 7 && abs(my-bmy) < 7 )
            p_visited[mx + my*16] = 1;
    }

    /* hpel diamond */
    bdir = -1;
    for( i = 0; i < 2; i++ )
    {
         omx = bmx;
         omy = bmy;
         odir = bdir;
         COST_MV_SATD( omx, omy - 2, satds[0] );
         COST_MV_SATD( omx, omy + 2, satds[1] );
         COST_MV_SATD( omx - 2, omy, satds[2] );
         COST_MV_SATD( omx + 2, omy, satds[3] );
         COST_MV_RD( omx, omy - 2, satds[0], 0 );
         COST_MV_RD( omx, omy + 2, satds[1], 1 );
         COST_MV_RD( omx - 2, omy, satds[2], 2 );
         COST_MV_RD( omx + 2, omy, satds[3], 3 );
         if( bmx == omx && bmy == omy )
            break;
    }

    /* qpel diamond */
    bdir = -1;
    for( i = 0; i < 2; i++ )
    {
         omx = bmx;
         omy = bmy;
         odir = bdir;
         COST_MV_SATD( omx, omy - 1, satds[0] );
         COST_MV_SATD( omx, omy + 1, satds[1] );
         COST_MV_SATD( omx - 1, omy, satds[2] );
         COST_MV_SATD( omx + 1, omy, satds[3] );
         COST_MV_RD( omx, omy - 1, satds[0], 0 );
         COST_MV_RD( omx, omy + 1, satds[1], 1 );
         COST_MV_RD( omx - 1, omy, satds[2], 2 );
         COST_MV_RD( omx + 1, omy, satds[3], 3 );
         if( bmx == omx && bmy == omy )
            break;
    }

    m->cost = bcost;
    m->mv[0] = bmx;
    m->mv[1] = bmy;

    x264_macroblock_cache_mv ( h, 2*(i8&1), i8&2, bw, bh, 0, bmx, bmy );
    x264_macroblock_cache_mvd( h, 2*(i8&1), i8&2, bw, bh, 0, bmx - pmx, bmy - pmy );
}


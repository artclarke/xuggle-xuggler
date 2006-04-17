/*****************************************************************************
 * cabac.c: h264 encoder library
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar
 * $Id: cabac.c,v 1.1 2004/06/03 19:27:08 fenrir Exp $
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

#include <stdio.h>
#include <string.h>

#include "common/common.h"
#include "macroblock.h"

static inline void x264_cabac_encode_ue_bypass( x264_cabac_t *cb, int exp_bits, int val )
{
#ifdef RDO_SKIP_BS
    cb->f8_bits_encoded += ( bs_size_ue( val + (1<<exp_bits)-1 ) - exp_bits ) << 8;
#else
    int k;
    for( k = exp_bits; val >= (1<<k); k++ )
    {
        x264_cabac_encode_bypass( cb, 1 );
        val -= 1 << k;
    }
    x264_cabac_encode_bypass( cb, 0 );
    while( k-- )
        x264_cabac_encode_bypass( cb, (val >> k)&0x01 );
#endif
}

static inline void x264_cabac_mb_type_intra( x264_t *h, x264_cabac_t *cb, int i_mb_type,
                    int ctx0, int ctx1, int ctx2, int ctx3, int ctx4, int ctx5 )
{
    if( i_mb_type == I_4x4 || i_mb_type == I_8x8 )
    {
        x264_cabac_encode_decision( cb, ctx0, 0 );
    }
    else if( i_mb_type == I_PCM )
    {
        x264_cabac_encode_decision( cb, ctx0, 1 );
        x264_cabac_encode_terminal( cb,       1 );
        x264_cabac_encode_flush( cb );
    }
    else
    {
        int i_pred = x264_mb_pred_mode16x16_fix[h->mb.i_intra16x16_pred_mode];

        x264_cabac_encode_decision( cb, ctx0, 1 );
        x264_cabac_encode_terminal( cb,       0 );

        x264_cabac_encode_decision( cb, ctx1, ( h->mb.i_cbp_luma == 0 ? 0 : 1 ));
        if( h->mb.i_cbp_chroma == 0 )
        {
            x264_cabac_encode_decision( cb, ctx2, 0 );
        }
        else
        {
            x264_cabac_encode_decision( cb, ctx2, 1 );
            x264_cabac_encode_decision( cb, ctx3, ( h->mb.i_cbp_chroma == 1 ? 0 : 1 ) );
        }
        x264_cabac_encode_decision( cb, ctx4, ( (i_pred / 2) ? 1 : 0 ));
        x264_cabac_encode_decision( cb, ctx5, ( (i_pred % 2) ? 1 : 0 ));
    }
}

static void x264_cabac_mb_type( x264_t *h, x264_cabac_t *cb )
{
    const int i_mb_type = h->mb.i_type;

    if( h->sh.i_type == SLICE_TYPE_I )
    {
        int ctx = 0;
        if( h->mb.i_mb_type_left >= 0 && h->mb.i_mb_type_left != I_4x4 )
        {
            ctx++;
        }
        if( h->mb.i_mb_type_top >= 0 && h->mb.i_mb_type_top != I_4x4 )
        {
            ctx++;
        }

        x264_cabac_mb_type_intra( h, cb, i_mb_type, 3+ctx, 3+3, 3+4, 3+5, 3+6, 3+7 );
    }
    else if( h->sh.i_type == SLICE_TYPE_P )
    {
        /* prefix: 14, suffix: 17 */
        if( i_mb_type == P_L0 )
        {
            if( h->mb.i_partition == D_16x16 )
            {
                x264_cabac_encode_decision( cb, 14, 0 );
                x264_cabac_encode_decision( cb, 15, 0 );
                x264_cabac_encode_decision( cb, 16, 0 );
            }
            else if( h->mb.i_partition == D_16x8 )
            {
                x264_cabac_encode_decision( cb, 14, 0 );
                x264_cabac_encode_decision( cb, 15, 1 );
                x264_cabac_encode_decision( cb, 17, 1 );
            }
            else if( h->mb.i_partition == D_8x16 )
            {
                x264_cabac_encode_decision( cb, 14, 0 );
                x264_cabac_encode_decision( cb, 15, 1 );
                x264_cabac_encode_decision( cb, 17, 0 );
            }
        }
        else if( i_mb_type == P_8x8 )
        {
            x264_cabac_encode_decision( cb, 14, 0 );
            x264_cabac_encode_decision( cb, 15, 0 );
            x264_cabac_encode_decision( cb, 16, 1 );
        }
        else /* intra */
        {
            /* prefix */
            x264_cabac_encode_decision( cb, 14, 1 );

            /* suffix */
            x264_cabac_mb_type_intra( h, cb, i_mb_type, 17+0, 17+1, 17+2, 17+2, 17+3, 17+3 );
        }
    }
    else if( h->sh.i_type == SLICE_TYPE_B )
    {
        int ctx = 0;
        if( h->mb.i_mb_type_left >= 0 && h->mb.i_mb_type_left != B_SKIP && h->mb.i_mb_type_left != B_DIRECT )
        {
            ctx++;
        }
        if( h->mb.i_mb_type_top >= 0 && h->mb.i_mb_type_top != B_SKIP && h->mb.i_mb_type_top != B_DIRECT )
        {
            ctx++;
        }

        if( i_mb_type == B_DIRECT )
        {
            x264_cabac_encode_decision( cb, 27+ctx, 0 );
        }
        else if( i_mb_type == B_8x8 )
        {
            x264_cabac_encode_decision( cb, 27+ctx, 1 );
            x264_cabac_encode_decision( cb, 27+3,   1 );
            x264_cabac_encode_decision( cb, 27+4,   1 );

            x264_cabac_encode_decision( cb, 27+5,   1 );
            x264_cabac_encode_decision( cb, 27+5,   1 );
            x264_cabac_encode_decision( cb, 27+5,   1 );
        }
        else if( IS_INTRA( i_mb_type ) )
        {
            /* prefix */
            x264_cabac_encode_decision( cb, 27+ctx, 1 );
            x264_cabac_encode_decision( cb, 27+3,   1 );
            x264_cabac_encode_decision( cb, 27+4,   1 );

            x264_cabac_encode_decision( cb, 27+5,   1 );
            x264_cabac_encode_decision( cb, 27+5,   0 );
            x264_cabac_encode_decision( cb, 27+5,   1 );

            /* suffix */
            x264_cabac_mb_type_intra( h, cb, i_mb_type, 32+0, 32+1, 32+2, 32+2, 32+3, 32+3 );
        }
        else
        {
            static const int i_mb_len[9*3] =
            {
                6, 6, 3,    /* L0 L0 */
                6, 6, 0,    /* L0 L1 */
                7, 7, 0,    /* L0 BI */
                6, 6, 0,    /* L1 L0 */
                6, 6, 3,    /* L1 L1 */
                7, 7, 0,    /* L1 BI */
                7, 7, 0,    /* BI L0 */
                7, 7, 0,    /* BI L1 */
                7, 7, 6,    /* BI BI */
            };
            static const int i_mb_bits[9*3][7] =
            {
                { 1,1,0,0,0,1   }, { 1,1,0,0,1,0,  }, { 1,0,0 },       /* L0 L0 */
                { 1,1,0,1,0,1   }, { 1,1,0,1,1,0   }, {0},             /* L0 L1 */
                { 1,1,1,0,0,0,0 }, { 1,1,1,0,0,0,1 }, {0},             /* L0 BI */
                { 1,1,0,1,1,1   }, { 1,1,1,1,1,0   }, {0},             /* L1 L0 */
                { 1,1,0,0,1,1   }, { 1,1,0,1,0,0   }, { 1,0,1 },       /* L1 L1 */
                { 1,1,1,0,0,1,0 }, { 1,1,1,0,0,1,1 }, {0},             /* L1 BI */
                { 1,1,1,0,1,0,0 }, { 1,1,1,0,1,0,1 }, {0},             /* BI L0 */
                { 1,1,1,0,1,1,0 }, { 1,1,1,0,1,1,1 }, {0},             /* BI L1 */
                { 1,1,1,1,0,0,0 }, { 1,1,1,1,0,0,1 }, { 1,1,0,0,0,0 }, /* BI BI */
            };

            const int idx = (i_mb_type - B_L0_L0) * 3 + (h->mb.i_partition - D_16x8);
            int i;

            x264_cabac_encode_decision( cb, 27+ctx, i_mb_bits[idx][0] );
            x264_cabac_encode_decision( cb, 27+3,   i_mb_bits[idx][1] );
            x264_cabac_encode_decision( cb, 27+5-i_mb_bits[idx][1], i_mb_bits[idx][2] );
            for( i = 3; i < i_mb_len[idx]; i++ )
                x264_cabac_encode_decision( cb, 27+5, i_mb_bits[idx][i] );
        }
    }
    else
    {
        x264_log(h, X264_LOG_ERROR, "unknown SLICE_TYPE unsupported in x264_macroblock_write_cabac\n" );
    }
}

static void x264_cabac_mb_intra4x4_pred_mode( x264_cabac_t *cb, int i_pred, int i_mode )
{
    if( i_pred == i_mode )
    {
        /* b_prev_intra4x4_pred_mode */
        x264_cabac_encode_decision( cb, 68, 1 );
    }
    else
    {
        /* b_prev_intra4x4_pred_mode */
        x264_cabac_encode_decision( cb, 68, 0 );
        if( i_mode > i_pred  )
        {
            i_mode--;
        }
        x264_cabac_encode_decision( cb, 69, (i_mode     )&0x01 );
        x264_cabac_encode_decision( cb, 69, (i_mode >> 1)&0x01 );
        x264_cabac_encode_decision( cb, 69, (i_mode >> 2)&0x01 );
    }
}

static void x264_cabac_mb_intra_chroma_pred_mode( x264_t *h, x264_cabac_t *cb )
{
    const int i_mode = x264_mb_pred_mode8x8c_fix[ h->mb.i_chroma_pred_mode ];
    int       ctx = 0;

    /* No need to test for I4x4 or I_16x16 as cache_save handle that */
    if( (h->mb.i_neighbour & MB_LEFT) && h->mb.chroma_pred_mode[h->mb.i_mb_xy - 1] != 0 )
    {
        ctx++;
    }
    if( (h->mb.i_neighbour & MB_TOP) && h->mb.chroma_pred_mode[h->mb.i_mb_xy - h->mb.i_mb_stride] != 0 )
    {
        ctx++;
    }

    x264_cabac_encode_decision( cb, 64 + ctx, i_mode > 0 );
    if( i_mode > 0 )
    {
        x264_cabac_encode_decision( cb, 64 + 3, i_mode > 1 );
        if( i_mode > 1 )
        {
            x264_cabac_encode_decision( cb, 64 + 3, i_mode > 2 );
        }
    }
}

static void x264_cabac_mb_cbp_luma( x264_t *h, x264_cabac_t *cb )
{
    /* TODO: clean up and optimize */
    int i8x8;
    for( i8x8 = 0; i8x8 < 4; i8x8++ )
    {
        int i_mba_xy = -1;
        int i_mbb_xy = -1;
        int x = block_idx_x[4*i8x8];
        int y = block_idx_y[4*i8x8];
        int ctx = 0;

        if( x > 0 )
            i_mba_xy = h->mb.i_mb_xy;
        else if( h->mb.i_neighbour & MB_LEFT )
            i_mba_xy = h->mb.i_mb_xy - 1;

        if( y > 0 )
            i_mbb_xy = h->mb.i_mb_xy;
        else if( h->mb.i_neighbour & MB_TOP )
            i_mbb_xy = h->mb.i_mb_xy - h->mb.i_mb_stride;


        /* No need to test for PCM and SKIP */
        if( i_mba_xy >= 0 )
        {
            const int i8x8a = block_idx_xy[(x-1)&0x03][y]/4;
            if( ((h->mb.cbp[i_mba_xy] >> i8x8a)&0x01) == 0 )
            {
                ctx++;
            }
        }

        if( i_mbb_xy >= 0 )
        {
            const int i8x8b = block_idx_xy[x][(y-1)&0x03]/4;
            if( ((h->mb.cbp[i_mbb_xy] >> i8x8b)&0x01) == 0 )
            {
                ctx += 2;
            }
        }

        x264_cabac_encode_decision( cb, 73 + ctx, (h->mb.i_cbp_luma >> i8x8)&0x01 );
    }
}

static void x264_cabac_mb_cbp_chroma( x264_t *h, x264_cabac_t *cb )
{
    int cbp_a = -1;
    int cbp_b = -1;
    int ctx;

    /* No need to test for SKIP/PCM */
    if( h->mb.i_neighbour & MB_LEFT )
    {
        cbp_a = (h->mb.cbp[h->mb.i_mb_xy - 1] >> 4)&0x3;
    }

    if( h->mb.i_neighbour & MB_TOP )
    {
        cbp_b = (h->mb.cbp[h->mb.i_mb_xy - h->mb.i_mb_stride] >> 4)&0x3;
    }

    ctx = 0;
    if( cbp_a > 0 ) ctx++;
    if( cbp_b > 0 ) ctx += 2;
    if( h->mb.i_cbp_chroma == 0 )
    {
        x264_cabac_encode_decision( cb, 77 + ctx, 0 );
    }
    else
    {
        x264_cabac_encode_decision( cb, 77 + ctx, 1 );

        ctx = 4;
        if( cbp_a == 2 ) ctx++;
        if( cbp_b == 2 ) ctx += 2;
        x264_cabac_encode_decision( cb, 77 + ctx, h->mb.i_cbp_chroma > 1 );
    }
}

/* TODO check it with != qp per mb */
static void x264_cabac_mb_qp_delta( x264_t *h, x264_cabac_t *cb )
{
    int i_mbn_xy = h->mb.i_mb_xy - 1;
    int i_dqp = h->mb.i_qp - h->mb.i_last_qp;
    int ctx;

    /* No need to test for PCM / SKIP */
    if( i_mbn_xy >= h->sh.i_first_mb && h->mb.i_last_dqp != 0 &&
        ( h->mb.type[i_mbn_xy] == I_16x16 || (h->mb.cbp[i_mbn_xy]&0x3f) ) )
        ctx = 1;
    else
        ctx = 0;

    if( i_dqp != 0 )
    {
        int val = i_dqp <= 0 ? (-2*i_dqp) : (2*i_dqp - 1);
        /* dqp is interpreted modulo 52 */
        if( val >= 51 && val != 52 )
            val = 103 - val;
        while( val-- )
        {
            x264_cabac_encode_decision( cb, 60 + ctx, 1 );
            if( ctx < 2 )
                ctx = 2;
            else
                ctx = 3;
        }
    }
    x264_cabac_encode_decision( cb, 60 + ctx, 0 );
}

void x264_cabac_mb_skip( x264_t *h, int b_skip )
{
    int ctx = 0;

    if( h->mb.i_mb_type_left >= 0 && !IS_SKIP( h->mb.i_mb_type_left ) )
    {
        ctx++;
    }
    if( h->mb.i_mb_type_top >= 0 && !IS_SKIP( h->mb.i_mb_type_top ) )
    {
        ctx++;
    }

    ctx += (h->sh.i_type == SLICE_TYPE_P) ? 11 : 24;
    x264_cabac_encode_decision( &h->cabac, ctx, b_skip );
}

static inline void x264_cabac_mb_sub_p_partition( x264_cabac_t *cb, int i_sub )
{
    if( i_sub == D_L0_8x8 )
    {
        x264_cabac_encode_decision( cb, 21, 1 );
    }
    else if( i_sub == D_L0_8x4 )
    {
        x264_cabac_encode_decision( cb, 21, 0 );
        x264_cabac_encode_decision( cb, 22, 0 );
    }
    else if( i_sub == D_L0_4x8 )
    {
        x264_cabac_encode_decision( cb, 21, 0 );
        x264_cabac_encode_decision( cb, 22, 1 );
        x264_cabac_encode_decision( cb, 23, 1 );
    }
    else if( i_sub == D_L0_4x4 )
    {
        x264_cabac_encode_decision( cb, 21, 0 );
        x264_cabac_encode_decision( cb, 22, 1 );
        x264_cabac_encode_decision( cb, 23, 0 );
    }
}

static inline void x264_cabac_mb_sub_b_partition( x264_cabac_t *cb, int i_sub )
{
#define WRITE_SUB_3(a,b,c) {\
        x264_cabac_encode_decision( cb, 36, a );\
        x264_cabac_encode_decision( cb, 37, b );\
        x264_cabac_encode_decision( cb, 39, c );\
    }
#define WRITE_SUB_5(a,b,c,d,e) {\
        x264_cabac_encode_decision( cb, 36, a );\
        x264_cabac_encode_decision( cb, 37, b );\
        x264_cabac_encode_decision( cb, 38, c );\
        x264_cabac_encode_decision( cb, 39, d );\
        x264_cabac_encode_decision( cb, 39, e );\
    }
#define WRITE_SUB_6(a,b,c,d,e,f) {\
        WRITE_SUB_5(a,b,c,d,e)\
        x264_cabac_encode_decision( cb, 39, f );\
    }

    switch( i_sub )
    {
        case D_DIRECT_8x8:
            x264_cabac_encode_decision( cb, 36, 0 );
            break;
        case D_L0_8x8: WRITE_SUB_3(1,0,0); break;
        case D_L1_8x8: WRITE_SUB_3(1,0,1); break;
        case D_BI_8x8: WRITE_SUB_5(1,1,0,0,0); break;
        case D_L0_8x4: WRITE_SUB_5(1,1,0,0,1); break;
        case D_L0_4x8: WRITE_SUB_5(1,1,0,1,0); break;
        case D_L1_8x4: WRITE_SUB_5(1,1,0,1,1); break;
        case D_L1_4x8: WRITE_SUB_6(1,1,1,0,0,0); break;
        case D_BI_8x4: WRITE_SUB_6(1,1,1,0,0,1); break;
        case D_BI_4x8: WRITE_SUB_6(1,1,1,0,1,0); break;
        case D_L0_4x4: WRITE_SUB_6(1,1,1,0,1,1); break;
        case D_L1_4x4: WRITE_SUB_5(1,1,1,1,0); break;
        case D_BI_4x4: WRITE_SUB_5(1,1,1,1,1); break;
    }
}

static inline void x264_cabac_mb_transform_size( x264_t *h, x264_cabac_t *cb )
{
    int ctx = 399 + h->mb.cache.i_neighbour_transform_size;
    x264_cabac_encode_decision( cb, ctx, h->mb.b_transform_8x8 );
}

static inline void x264_cabac_mb_ref( x264_t *h, x264_cabac_t *cb, int i_list, int idx )
{
    const int i8 = x264_scan8[idx];
    const int i_refa = h->mb.cache.ref[i_list][i8 - 1];
    const int i_refb = h->mb.cache.ref[i_list][i8 - 8];
    int i_ref  = h->mb.cache.ref[i_list][i8];
    int ctx  = 0;

    if( i_refa > 0 && !h->mb.cache.skip[i8 - 1])
        ctx++;
    if( i_refb > 0 && !h->mb.cache.skip[i8 - 8])
        ctx += 2;

    while( i_ref > 0 )
    {
        x264_cabac_encode_decision( cb, 54 + ctx, 1 );
        if( ctx < 4 )
            ctx = 4;
        else
            ctx = 5;

        i_ref--;
    }
    x264_cabac_encode_decision( cb, 54 + ctx, 0 );
}



static inline void x264_cabac_mb_mvd_cpn( x264_t *h, x264_cabac_t *cb, int i_list, int idx, int l, int mvd )
{
    const int amvd = abs( h->mb.cache.mvd[i_list][x264_scan8[idx] - 1][l] ) +
                     abs( h->mb.cache.mvd[i_list][x264_scan8[idx] - 8][l] );
    const int i_abs = abs( mvd );
    const int i_prefix = X264_MIN( i_abs, 9 );
    const int ctxbase = (l == 0 ? 40 : 47);
    int ctx;
    int i;


    if( amvd < 3 )
        ctx = 0;
    else if( amvd > 32 )
        ctx = 2;
    else
        ctx = 1;

    for( i = 0; i < i_prefix; i++ )
    {
        x264_cabac_encode_decision( cb, ctxbase + ctx, 1 );
        if( ctx < 3 )
            ctx = 3;
        else if( ctx < 6 )
            ctx++;
    }
    if( i_prefix < 9 )
        x264_cabac_encode_decision( cb, ctxbase + ctx, 0 );
    else
        x264_cabac_encode_ue_bypass( cb, 3, i_abs - 9 );

    /* sign */
    if( mvd )
        x264_cabac_encode_bypass( cb, mvd < 0 );
}

static inline void x264_cabac_mb_mvd( x264_t *h, x264_cabac_t *cb, int i_list, int idx, int width, int height )
{
    int mvp[2];
    int mdx, mdy;

    /* Calculate mvd */
    x264_mb_predict_mv( h, i_list, idx, width, mvp );
    mdx = h->mb.cache.mv[i_list][x264_scan8[idx]][0] - mvp[0];
    mdy = h->mb.cache.mv[i_list][x264_scan8[idx]][1] - mvp[1];

    /* encode */
    x264_cabac_mb_mvd_cpn( h, cb, i_list, idx, 0, mdx );
    x264_cabac_mb_mvd_cpn( h, cb, i_list, idx, 1, mdy );

    /* save value */
    x264_macroblock_cache_mvd( h, block_idx_x[idx], block_idx_y[idx], width, height, i_list, mdx, mdy );
}

static inline void x264_cabac_mb8x8_mvd( x264_t *h, x264_cabac_t *cb, int i_list, int i )
{
    if( !x264_mb_partition_listX_table[i_list][ h->mb.i_sub_partition[i] ] )
        return;

    switch( h->mb.i_sub_partition[i] )
    {
        case D_L0_8x8:
        case D_L1_8x8:
        case D_BI_8x8:
            x264_cabac_mb_mvd( h, cb, i_list, 4*i, 2, 2 );
            break;
        case D_L0_8x4:
        case D_L1_8x4:
        case D_BI_8x4:
            x264_cabac_mb_mvd( h, cb, i_list, 4*i+0, 2, 1 );
            x264_cabac_mb_mvd( h, cb, i_list, 4*i+2, 2, 1 );
            break;
        case D_L0_4x8:
        case D_L1_4x8:
        case D_BI_4x8:
            x264_cabac_mb_mvd( h, cb, i_list, 4*i+0, 1, 2 );
            x264_cabac_mb_mvd( h, cb, i_list, 4*i+1, 1, 2 );
            break;
        case D_L0_4x4:
        case D_L1_4x4:
        case D_BI_4x4:
            x264_cabac_mb_mvd( h, cb, i_list, 4*i+0, 1, 1 );
            x264_cabac_mb_mvd( h, cb, i_list, 4*i+1, 1, 1 );
            x264_cabac_mb_mvd( h, cb, i_list, 4*i+2, 1, 1 );
            x264_cabac_mb_mvd( h, cb, i_list, 4*i+3, 1, 1 );
            break;
    }
}

static int x264_cabac_mb_cbf_ctxidxinc( x264_t *h, int i_cat, int i_idx )
{
    int i_mba_xy = -1;
    int i_mbb_xy = -1;
    int i_nza = 0;
    int i_nzb = 0;
    int ctx;

    if( i_cat == DCT_LUMA_DC )
    {
        if( h->mb.i_neighbour & MB_LEFT )
        {
            i_mba_xy = h->mb.i_mb_xy - 1;
            if( h->mb.type[i_mba_xy] == I_16x16 )
                i_nza = h->mb.cbp[i_mba_xy] & 0x100;
        }
        if( h->mb.i_neighbour & MB_TOP )
        {
            i_mbb_xy = h->mb.i_mb_xy - h->mb.i_mb_stride;
            if( h->mb.type[i_mbb_xy] == I_16x16 )
                i_nzb = h->mb.cbp[i_mbb_xy] & 0x100;
        }
    }
    else if( i_cat == DCT_LUMA_AC || i_cat == DCT_LUMA_4x4 )
    {
        if( i_idx & ~10 ) // block_idx_x > 0
            i_mba_xy = h->mb.i_mb_xy;
        else if( h->mb.i_neighbour & MB_LEFT )
            i_mba_xy = h->mb.i_mb_xy - 1;

        if( i_idx & ~5 ) // block_idx_y > 0
            i_mbb_xy = h->mb.i_mb_xy;
        else if( h->mb.i_neighbour & MB_TOP )
            i_mbb_xy = h->mb.i_mb_xy - h->mb.i_mb_stride;

        /* no need to test for skip/pcm */
        if( i_mba_xy >= 0 )
            i_nza = h->mb.cache.non_zero_count[x264_scan8[i_idx] - 1];
        if( i_mbb_xy >= 0 )
            i_nzb = h->mb.cache.non_zero_count[x264_scan8[i_idx] - 8];
    }
    else if( i_cat == DCT_CHROMA_DC )
    {
        /* no need to test skip/pcm */
        if( h->mb.i_neighbour & MB_LEFT )
        {
            i_mba_xy = h->mb.i_mb_xy - 1;
            i_nza = h->mb.cbp[i_mba_xy] & (0x200 << i_idx);
        }
        if( h->mb.i_neighbour & MB_TOP )
        {
            i_mbb_xy = h->mb.i_mb_xy - h->mb.i_mb_stride;
            i_nzb = h->mb.cbp[i_mbb_xy] & (0x200 << i_idx);
        }
    }
    else if( i_cat == DCT_CHROMA_AC )
    {
        if( i_idx & 1 )
            i_mba_xy = h->mb.i_mb_xy;
        else if( h->mb.i_neighbour & MB_LEFT )
            i_mba_xy = h->mb.i_mb_xy - 1;

        if( i_idx & 2 )
            i_mbb_xy = h->mb.i_mb_xy;
        else if( h->mb.i_neighbour & MB_TOP )
            i_mbb_xy = h->mb.i_mb_xy - h->mb.i_mb_stride;

        /* no need to test skip/pcm */
        if( i_mba_xy >= 0 )
            i_nza = h->mb.cache.non_zero_count[x264_scan8[16+i_idx] - 1];
        if( i_mbb_xy >= 0 )
            i_nzb = h->mb.cache.non_zero_count[x264_scan8[16+i_idx] - 8];
    }

    if( IS_INTRA( h->mb.i_type ) )
    {
        if( i_mba_xy < 0 )
            i_nza = 1;
        if( i_mbb_xy < 0 )
            i_nzb = 1;
    }

    ctx = 4 * i_cat;
    if( i_nza )
        ctx += 1;
    if( i_nzb )
        ctx += 2;
    return ctx;
}


static const int significant_coeff_flag_offset[6] = { 105, 120, 134, 149, 152, 402 };
static const int last_coeff_flag_offset[6] = { 166, 181, 195, 210, 213, 417 };
static const int coeff_abs_level_m1_offset[6] = { 227, 237, 247, 257, 266, 426 };
static const int significant_coeff_flag_offset_8x8[63] = {
    0, 1, 2, 3, 4, 5, 5, 4, 4, 3, 3, 4, 4, 4, 5, 5,
    4, 4, 4, 4, 3, 3, 6, 7, 7, 7, 8, 9,10, 9, 8, 7,
    7, 6,11,12,13,11, 6, 7, 8, 9,14,10, 9, 8, 6,11,
   12,13,11, 6, 9,14,10, 9,11,12,13,11,14,10,12
};
static const int last_coeff_flag_offset_8x8[63] = {
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4,
    5, 5, 5, 5, 6, 6, 6, 6, 7, 7, 7, 7, 8, 8, 8
};

static void block_residual_write_cabac( x264_t *h, x264_cabac_t *cb, int i_ctxBlockCat, int i_idx, int *l, int i_count )
{
    const int i_ctx_sig = significant_coeff_flag_offset[i_ctxBlockCat];
    const int i_ctx_last = last_coeff_flag_offset[i_ctxBlockCat];
    const int i_ctx_level = coeff_abs_level_m1_offset[i_ctxBlockCat];

    int i_coeff_abs_m1[64];
    int i_coeff_sign[64];
    int i_coeff = 0;
    int i_last  = 0;
    int i_sigmap_size;

    int i_abslevel1 = 0;
    int i_abslevelgt1 = 0;

    int i;

    /* i_ctxBlockCat: 0-> DC 16x16  i_idx = 0
     *                1-> AC 16x16  i_idx = luma4x4idx
     *                2-> Luma4x4   i_idx = luma4x4idx
     *                3-> DC Chroma i_idx = iCbCr
     *                4-> AC Chroma i_idx = 4 * iCbCr + chroma4x4idx
     *                5-> Luma8x8   i_idx = luma8x8idx
     */

    for( i = 0; i < i_count; i++ )
    {
        if( l[i] != 0 )
        {
            i_coeff_abs_m1[i_coeff] = abs( l[i] ) - 1;
            i_coeff_sign[i_coeff]   = ( l[i] < 0 );
            i_coeff++;

            i_last = i;
        }
    }

    if( i_count != 64 )
    {
        /* coded block flag */
        x264_cabac_encode_decision( cb, 85 + x264_cabac_mb_cbf_ctxidxinc( h, i_ctxBlockCat, i_idx ), i_coeff != 0 );
        if( i_coeff == 0 )
            return;
    }

    i_sigmap_size = X264_MIN( i_last+1, i_count-1 );
    for( i = 0; i < i_sigmap_size; i++ )
    {
        int i_sig_ctxIdxInc;
        int i_last_ctxIdxInc;

        if( i_ctxBlockCat == DCT_LUMA_8x8 )
        {
            i_sig_ctxIdxInc = significant_coeff_flag_offset_8x8[i];
            i_last_ctxIdxInc = last_coeff_flag_offset_8x8[i];
        }
        else
            i_sig_ctxIdxInc = i_last_ctxIdxInc = i;

        x264_cabac_encode_decision( cb, i_ctx_sig + i_sig_ctxIdxInc, l[i] != 0 );
        if( l[i] != 0 )
            x264_cabac_encode_decision( cb, i_ctx_last + i_last_ctxIdxInc, i == i_last );
    }

    for( i = i_coeff - 1; i >= 0; i-- )
    {
        /* write coeff_abs - 1 */
        const int i_prefix = X264_MIN( i_coeff_abs_m1[i], 14 );
        const int i_ctxIdxInc = (i_abslevelgt1 ? 0 : X264_MIN( 4, i_abslevel1 + 1 )) + i_ctx_level;
        x264_cabac_encode_decision( cb, i_ctxIdxInc, i_prefix != 0 );

        if( i_prefix != 0 )
        {
            const int i_ctxIdxInc = 5 + X264_MIN( 4, i_abslevelgt1 ) + i_ctx_level;
            int j;
            for( j = 0; j < i_prefix - 1; j++ )
                x264_cabac_encode_decision( cb, i_ctxIdxInc, 1 );
            if( i_prefix < 14 )
                x264_cabac_encode_decision( cb, i_ctxIdxInc, 0 );
            else /* suffix */
                x264_cabac_encode_ue_bypass( cb, 0, i_coeff_abs_m1[i] - 14 );

            i_abslevelgt1++;
        }
        else
            i_abslevel1++;

        /* write sign */
        x264_cabac_encode_bypass( cb, i_coeff_sign[i] );
    }
}



void x264_macroblock_write_cabac( x264_t *h, x264_cabac_t *cb )
{
    const int i_mb_type = h->mb.i_type;
    int i_list;
    int i;

#ifndef RDO_SKIP_BS
    const int i_mb_pos_start = x264_cabac_pos( cb );
    int       i_mb_pos_tex;
#endif

    /* Write the MB type */
    x264_cabac_mb_type( h, cb );

    /* PCM special block type UNTESTED */
    if( i_mb_type == I_PCM )
    {
#ifdef RDO_SKIP_BS
        cb->f8_bits_encoded += (384*8) << 8;
#else
        bs_t *s = cb->s;
        bs_align_0( s );    /* not sure */
        /* Luma */
        for( i = 0; i < 16*16; i++ )
        {
            const int x = 16 * h->mb.i_mb_x + (i % 16);
            const int y = 16 * h->mb.i_mb_y + (i / 16);
            bs_write( s, 8, h->fenc->plane[0][y*h->mb.pic.i_stride[0]+x] );
        }
        /* Cb */
        for( i = 0; i < 8*8; i++ )
        {
            const int x = 8 * h->mb.i_mb_x + (i % 8);
            const int y = 8 * h->mb.i_mb_y + (i / 8);
            bs_write( s, 8, h->fenc->plane[1][y*h->mb.pic.i_stride[1]+x] );
        }
        /* Cr */
        for( i = 0; i < 8*8; i++ )
        {
            const int x = 8 * h->mb.i_mb_x + (i % 8);
            const int y = 8 * h->mb.i_mb_y + (i / 8);
            bs_write( s, 8, h->fenc->plane[2][y*h->mb.pic.i_stride[2]+x] );
        }
        x264_cabac_encode_init( cb, s );
#endif
        return;
    }

    if( IS_INTRA( i_mb_type ) )
    {
        if( h->pps->b_transform_8x8_mode && i_mb_type != I_16x16 )
            x264_cabac_mb_transform_size( h, cb );

        if( i_mb_type != I_16x16 )
        {
            int di = (i_mb_type == I_8x8) ? 4 : 1;
            for( i = 0; i < 16; i += di )
            {
                const int i_pred = x264_mb_predict_intra4x4_mode( h, i );
                const int i_mode = x264_mb_pred_mode4x4_fix( h->mb.cache.intra4x4_pred_mode[x264_scan8[i]] );
                x264_cabac_mb_intra4x4_pred_mode( cb, i_pred, i_mode );
            }
        }

        x264_cabac_mb_intra_chroma_pred_mode( h, cb );
    }
    else if( i_mb_type == P_L0 )
    {
        if( h->mb.i_partition == D_16x16 )
        {
            if( h->sh.i_num_ref_idx_l0_active > 1 )
            {
                x264_cabac_mb_ref( h, cb, 0, 0 );
            }
            x264_cabac_mb_mvd( h, cb, 0, 0, 4, 4 );
        }
        else if( h->mb.i_partition == D_16x8 )
        {
            if( h->sh.i_num_ref_idx_l0_active > 1 )
            {
                x264_cabac_mb_ref( h, cb, 0, 0 );
                x264_cabac_mb_ref( h, cb, 0, 8 );
            }
            x264_cabac_mb_mvd( h, cb, 0, 0, 4, 2 );
            x264_cabac_mb_mvd( h, cb, 0, 8, 4, 2 );
        }
        else if( h->mb.i_partition == D_8x16 )
        {
            if( h->sh.i_num_ref_idx_l0_active > 1 )
            {
                x264_cabac_mb_ref( h, cb, 0, 0 );
                x264_cabac_mb_ref( h, cb, 0, 4 );
            }
            x264_cabac_mb_mvd( h, cb, 0, 0, 2, 4 );
            x264_cabac_mb_mvd( h, cb, 0, 4, 2, 4 );
        }
    }
    else if( i_mb_type == P_8x8 )
    {
        /* sub mb type */
        x264_cabac_mb_sub_p_partition( cb, h->mb.i_sub_partition[0] );
        x264_cabac_mb_sub_p_partition( cb, h->mb.i_sub_partition[1] );
        x264_cabac_mb_sub_p_partition( cb, h->mb.i_sub_partition[2] );
        x264_cabac_mb_sub_p_partition( cb, h->mb.i_sub_partition[3] );

        /* ref 0 */
        if( h->sh.i_num_ref_idx_l0_active > 1 )
        {
            x264_cabac_mb_ref( h, cb, 0, 0 );
            x264_cabac_mb_ref( h, cb, 0, 4 );
            x264_cabac_mb_ref( h, cb, 0, 8 );
            x264_cabac_mb_ref( h, cb, 0, 12 );
        }

        for( i = 0; i < 4; i++ )
            x264_cabac_mb8x8_mvd( h, cb, 0, i );
    }
    else if( i_mb_type == B_8x8 )
    {
        /* sub mb type */
        x264_cabac_mb_sub_b_partition( cb, h->mb.i_sub_partition[0] );
        x264_cabac_mb_sub_b_partition( cb, h->mb.i_sub_partition[1] );
        x264_cabac_mb_sub_b_partition( cb, h->mb.i_sub_partition[2] );
        x264_cabac_mb_sub_b_partition( cb, h->mb.i_sub_partition[3] );

        /* ref */
        for( i_list = 0; i_list < 2; i_list++ )
        {
            if( ( i_list ? h->sh.i_num_ref_idx_l1_active : h->sh.i_num_ref_idx_l0_active ) == 1 )
                continue;
            for( i = 0; i < 4; i++ )
                if( x264_mb_partition_listX_table[i_list][ h->mb.i_sub_partition[i] ] )
                    x264_cabac_mb_ref( h, cb, i_list, 4*i );
        }

        for( i = 0; i < 4; i++ )
            x264_cabac_mb8x8_mvd( h, cb, 0, i );
        for( i = 0; i < 4; i++ )
            x264_cabac_mb8x8_mvd( h, cb, 1, i );
    }
    else if( i_mb_type != B_DIRECT )
    {
        /* All B mode */
        int b_list[2][2];

        /* init ref list utilisations */
        for( i = 0; i < 2; i++ )
        {
            b_list[0][i] = x264_mb_type_list0_table[i_mb_type][i];
            b_list[1][i] = x264_mb_type_list1_table[i_mb_type][i];
        }

        for( i_list = 0; i_list < 2; i_list++ )
        {
            const int i_ref_max = i_list == 0 ? h->sh.i_num_ref_idx_l0_active : h->sh.i_num_ref_idx_l1_active;

            if( i_ref_max > 1 )
            {
                if( h->mb.i_partition == D_16x16 )
                {
                    if( b_list[i_list][0] ) x264_cabac_mb_ref( h, cb, i_list, 0 );
                }
                else if( h->mb.i_partition == D_16x8 )
                {
                    if( b_list[i_list][0] ) x264_cabac_mb_ref( h, cb, i_list, 0 );
                    if( b_list[i_list][1] ) x264_cabac_mb_ref( h, cb, i_list, 8 );
                }
                else if( h->mb.i_partition == D_8x16 )
                {
                    if( b_list[i_list][0] ) x264_cabac_mb_ref( h, cb, i_list, 0 );
                    if( b_list[i_list][1] ) x264_cabac_mb_ref( h, cb, i_list, 4 );
                }
            }
        }
        for( i_list = 0; i_list < 2; i_list++ )
        {
            if( h->mb.i_partition == D_16x16 )
            {
                if( b_list[i_list][0] ) x264_cabac_mb_mvd( h, cb, i_list, 0, 4, 4 );
            }
            else if( h->mb.i_partition == D_16x8 )
            {
                if( b_list[i_list][0] ) x264_cabac_mb_mvd( h, cb, i_list, 0, 4, 2 );
                if( b_list[i_list][1] ) x264_cabac_mb_mvd( h, cb, i_list, 8, 4, 2 );
            }
            else if( h->mb.i_partition == D_8x16 )
            {
                if( b_list[i_list][0] ) x264_cabac_mb_mvd( h, cb, i_list, 0, 2, 4 );
                if( b_list[i_list][1] ) x264_cabac_mb_mvd( h, cb, i_list, 4, 2, 4 );
            }
        }
    }

#ifndef RDO_SKIP_BS
    i_mb_pos_tex = x264_cabac_pos( cb );
    h->stat.frame.i_hdr_bits += i_mb_pos_tex - i_mb_pos_start;
#endif

    if( i_mb_type != I_16x16 )
    {
        x264_cabac_mb_cbp_luma( h, cb );
        x264_cabac_mb_cbp_chroma( h, cb );
    }

    if( h->mb.cache.b_transform_8x8_allowed && h->mb.i_cbp_luma && !IS_INTRA(i_mb_type) )
    {
        x264_cabac_mb_transform_size( h, cb );
    }

    if( h->mb.i_cbp_luma > 0 || h->mb.i_cbp_chroma > 0 || i_mb_type == I_16x16 )
    {
        x264_cabac_mb_qp_delta( h, cb );

        /* write residual */
        if( i_mb_type == I_16x16 )
        {
            /* DC Luma */
            block_residual_write_cabac( h, cb, DCT_LUMA_DC, 0, h->dct.luma16x16_dc, 16 );

            /* AC Luma */
            if( h->mb.i_cbp_luma != 0 )
                for( i = 0; i < 16; i++ )
                    block_residual_write_cabac( h, cb, DCT_LUMA_AC, i, h->dct.block[i].residual_ac, 15 );
        }
        else if( h->mb.b_transform_8x8 )
        {
            for( i = 0; i < 4; i++ )
                if( h->mb.i_cbp_luma & ( 1 << i ) )
                    block_residual_write_cabac( h, cb, DCT_LUMA_8x8, i, h->dct.luma8x8[i], 64 );
        }
        else
        {
            for( i = 0; i < 16; i++ )
                if( h->mb.i_cbp_luma & ( 1 << ( i / 4 ) ) )
                    block_residual_write_cabac( h, cb, DCT_LUMA_4x4, i, h->dct.block[i].luma4x4, 16 );
        }

        if( h->mb.i_cbp_chroma &0x03 )    /* Chroma DC residual present */
        {
            block_residual_write_cabac( h, cb, DCT_CHROMA_DC, 0, h->dct.chroma_dc[0], 4 );
            block_residual_write_cabac( h, cb, DCT_CHROMA_DC, 1, h->dct.chroma_dc[1], 4 );
        }
        if( h->mb.i_cbp_chroma&0x02 ) /* Chroma AC residual present */
        {
            for( i = 0; i < 8; i++ )
                block_residual_write_cabac( h, cb, DCT_CHROMA_AC, i, h->dct.block[16+i].residual_ac, 15 );
        }
    }

#ifndef RDO_SKIP_BS
    if( IS_INTRA( i_mb_type ) )
        h->stat.frame.i_itex_bits += x264_cabac_pos( cb ) - i_mb_pos_tex;
    else
        h->stat.frame.i_ptex_bits += x264_cabac_pos( cb ) - i_mb_pos_tex;
#endif
}

#ifdef RDO_SKIP_BS
/*****************************************************************************
 * RD only; doesn't generate a valid bitstream
 * doesn't write cbp or chroma dc (I don't know how much this matters)
 * works on all partition sizes except 16x16
 * for sub8x8, call once per 8x8 block
 *****************************************************************************/
void x264_partition_size_cabac( x264_t *h, x264_cabac_t *cb, int i8, int i_pixel )
{
    const int i_mb_type = h->mb.i_type;
    int j;

    if( i_mb_type == P_8x8 )
    {
        x264_cabac_mb_sub_p_partition( cb, h->mb.i_sub_partition[i8] );
        if( h->sh.i_num_ref_idx_l0_active > 1 )
            x264_cabac_mb_ref( h, cb, 0, 4*i8 );
        x264_cabac_mb8x8_mvd( h, cb, 0, i8 );
    }
    else if( i_mb_type == P_L0 )
    {
        if( h->sh.i_num_ref_idx_l0_active > 1 )
            x264_cabac_mb_ref( h, cb, 0, 4*i8 );
        if( h->mb.i_partition == D_16x8 )
            x264_cabac_mb_mvd( h, cb, 0, 4*i8, 4, 2 );
        else //8x16
            x264_cabac_mb_mvd( h, cb, 0, 4*i8, 2, 4 );
    }
    else if( i_mb_type == B_8x8 )
    {
        x264_cabac_mb_sub_b_partition( cb, h->mb.i_sub_partition[i8] );

        if( h->sh.i_num_ref_idx_l0_active > 1
            && x264_mb_partition_listX_table[0][ h->mb.i_sub_partition[i8] ] )
            x264_cabac_mb_ref( h, cb, 0, 4*i8 );
        if( h->sh.i_num_ref_idx_l1_active > 1
            && x264_mb_partition_listX_table[1][ h->mb.i_sub_partition[i8] ] )
            x264_cabac_mb_ref( h, cb, 1, 4*i8 );

        x264_cabac_mb8x8_mvd( h, cb, 0, i8 );
        x264_cabac_mb8x8_mvd( h, cb, 1, i8 );
    }
    else
    {
        x264_log(h, X264_LOG_ERROR, "invalid/unhandled mb_type\n" );
        return;
    }

    for( j = (i_pixel < PIXEL_8x8); j >= 0; j-- )
    {
        if( h->mb.i_cbp_luma & (1 << i8) )
        {
            if( h->mb.b_transform_8x8 )
                block_residual_write_cabac( h, cb, DCT_LUMA_8x8, i8, h->dct.luma8x8[i8], 64 );
            else
            {
                int i4;
                for( i4 = 0; i4 < 4; i4++ )
                    block_residual_write_cabac( h, cb, DCT_LUMA_4x4, i4+i8*4, h->dct.block[i4+i8*4].luma4x4, 16 );
            }
        }

        block_residual_write_cabac( h, cb, DCT_CHROMA_AC, i8,   h->dct.block[16+i8  ].residual_ac, 15 );
        block_residual_write_cabac( h, cb, DCT_CHROMA_AC, i8+4, h->dct.block[16+i8+4].residual_ac, 15 );

        i8 += x264_pixel_size[i_pixel].h >> 3;
    }
}

static void x264_partition_i8x8_size_cabac( x264_t *h, x264_cabac_t *cb, int i8, int i_mode )
{
    const int i_pred = x264_mb_predict_intra4x4_mode( h, 4*i8 );
    i_mode = x264_mb_pred_mode4x4_fix( i_mode );
    x264_cabac_mb_intra4x4_pred_mode( cb, i_pred, i_mode );
    block_residual_write_cabac( h, cb, DCT_LUMA_8x8, 4*i8, h->dct.luma8x8[i8], 64 );
}

static void x264_partition_i4x4_size_cabac( x264_t *h, x264_cabac_t *cb, int i4, int i_mode )
{
    const int i_pred = x264_mb_predict_intra4x4_mode( h, i4 );
    i_mode = x264_mb_pred_mode4x4_fix( i_mode );
    x264_cabac_mb_intra4x4_pred_mode( cb, i_pred, i_mode );
    block_residual_write_cabac( h, cb, DCT_LUMA_4x4, i4, h->dct.block[i4].luma4x4, 16 );
}
#endif

/*****************************************************************************
 * cabac.c: h264 encoder library
 *****************************************************************************
 * Copyright (C) 2003-2008 x264 project
 *
 * Authors: Laurent Aimar <fenrir@via.ecp.fr>
 *          Loren Merritt <lorenm@u.washington.edu>
 *          Jason Garrett-Glaser <darkshikari@gmail.com>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111, USA.
 *****************************************************************************/

#include "common/common.h"
#include "macroblock.h"

#ifndef RDO_SKIP_BS
#define RDO_SKIP_BS 0
#endif

static inline void x264_cabac_mb_type_intra( x264_t *h, x264_cabac_t *cb, int i_mb_type,
                    int ctx0, int ctx1, int ctx2, int ctx3, int ctx4, int ctx5 )
{
    if( i_mb_type == I_4x4 || i_mb_type == I_8x8 )
    {
        x264_cabac_encode_decision_noup( cb, ctx0, 0 );
    }
#if !RDO_SKIP_BS
    else if( i_mb_type == I_PCM )
    {
        x264_cabac_encode_decision_noup( cb, ctx0, 1 );
        x264_cabac_encode_flush( h, cb );
    }
#endif
    else
    {
        int i_pred = x264_mb_pred_mode16x16_fix[h->mb.i_intra16x16_pred_mode];

        x264_cabac_encode_decision_noup( cb, ctx0, 1 );
        x264_cabac_encode_terminal( cb );

        x264_cabac_encode_decision_noup( cb, ctx1, !!h->mb.i_cbp_luma );
        if( h->mb.i_cbp_chroma == 0 )
            x264_cabac_encode_decision_noup( cb, ctx2, 0 );
        else
        {
            x264_cabac_encode_decision( cb, ctx2, 1 );
            x264_cabac_encode_decision_noup( cb, ctx3, h->mb.i_cbp_chroma>>1 );
        }
        x264_cabac_encode_decision( cb, ctx4, i_pred>>1 );
        x264_cabac_encode_decision_noup( cb, ctx5, i_pred&1 );
    }
}

static void x264_cabac_mb_type( x264_t *h, x264_cabac_t *cb )
{
    const int i_mb_type = h->mb.i_type;

    if( h->sh.b_mbaff &&
        (!(h->mb.i_mb_y & 1) || IS_SKIP(h->mb.type[h->mb.i_mb_xy - h->mb.i_mb_stride])) )
    {
        x264_cabac_encode_decision_noup( cb, 70 + h->mb.cache.i_neighbour_interlaced, h->mb.b_interlaced );
    }

    if( h->sh.i_type == SLICE_TYPE_I )
    {
        int ctx = 0;
        if( h->mb.i_mb_type_left >= 0 && h->mb.i_mb_type_left != I_4x4 )
            ctx++;
        if( h->mb.i_mb_type_top >= 0 && h->mb.i_mb_type_top != I_4x4 )
            ctx++;

        x264_cabac_mb_type_intra( h, cb, i_mb_type, 3+ctx, 3+3, 3+4, 3+5, 3+6, 3+7 );
    }
    else if( h->sh.i_type == SLICE_TYPE_P )
    {
        /* prefix: 14, suffix: 17 */
        if( i_mb_type == P_L0 )
        {
            x264_cabac_encode_decision_noup( cb, 14, 0 );
            x264_cabac_encode_decision_noup( cb, 15, h->mb.i_partition != D_16x16 );
            x264_cabac_encode_decision_noup( cb, 17-(h->mb.i_partition == D_16x16), h->mb.i_partition == D_16x8 );
        }
        else if( i_mb_type == P_8x8 )
        {
            x264_cabac_encode_decision_noup( cb, 14, 0 );
            x264_cabac_encode_decision_noup( cb, 15, 0 );
            x264_cabac_encode_decision_noup( cb, 16, 1 );
        }
        else /* intra */
        {
            /* prefix */
            x264_cabac_encode_decision_noup( cb, 14, 1 );

            /* suffix */
            x264_cabac_mb_type_intra( h, cb, i_mb_type, 17+0, 17+1, 17+2, 17+2, 17+3, 17+3 );
        }
    }
    else //if( h->sh.i_type == SLICE_TYPE_B )
    {
        int ctx = 0;
        if( h->mb.i_mb_type_left >= 0 && h->mb.i_mb_type_left != B_SKIP && h->mb.i_mb_type_left != B_DIRECT )
            ctx++;
        if( h->mb.i_mb_type_top >= 0 && h->mb.i_mb_type_top != B_SKIP && h->mb.i_mb_type_top != B_DIRECT )
            ctx++;

        if( i_mb_type == B_DIRECT )
        {
            x264_cabac_encode_decision_noup( cb, 27+ctx, 0 );
            return;
        }
        x264_cabac_encode_decision_noup( cb, 27+ctx, 1 );

        if( i_mb_type == B_8x8 )
        {
            x264_cabac_encode_decision_noup( cb, 27+3,   1 );
            x264_cabac_encode_decision_noup( cb, 27+4,   1 );
            x264_cabac_encode_decision( cb, 27+5,   1 );
            x264_cabac_encode_decision( cb, 27+5,   1 );
            x264_cabac_encode_decision_noup( cb, 27+5,   1 );
        }
        else if( IS_INTRA( i_mb_type ) )
        {
            /* prefix */
            x264_cabac_encode_decision_noup( cb, 27+3,   1 );
            x264_cabac_encode_decision_noup( cb, 27+4,   1 );
            x264_cabac_encode_decision( cb, 27+5,   1 );
            x264_cabac_encode_decision( cb, 27+5,   0 );
            x264_cabac_encode_decision( cb, 27+5,   1 );

            /* suffix */
            x264_cabac_mb_type_intra( h, cb, i_mb_type, 32+0, 32+1, 32+2, 32+2, 32+3, 32+3 );
        }
        else
        {
            static const uint8_t i_mb_bits[9*3][6] =
            {
                { 1,0,0,0,1,2 }, { 1,0,0,1,0,2 }, { 0,0,2,2,2,2 },  /* L0 L0 */
                { 1,0,1,0,1,2 }, { 1,0,1,1,0,2 }, {0},              /* L0 L1 */
                { 1,1,0,0,0,0 }, { 1,1,0,0,0,1 }, {0},              /* L0 BI */
                { 1,0,1,1,1,2 }, { 1,1,1,1,0,2 }, {0},              /* L1 L0 */
                { 1,0,0,1,1,2 }, { 1,0,1,0,0,2 }, { 0,1,2,2,2,2 },  /* L1 L1 */
                { 1,1,0,0,1,0 }, { 1,1,0,0,1,1 }, {0},              /* L1 BI */
                { 1,1,0,1,0,0 }, { 1,1,0,1,0,1 }, {0},              /* BI L0 */
                { 1,1,0,1,1,0 }, { 1,1,0,1,1,1 }, {0},              /* BI L1 */
                { 1,1,1,0,0,0 }, { 1,1,1,0,0,1 }, { 1,0,0,0,0,2 },  /* BI BI */
            };

            const int idx = (i_mb_type - B_L0_L0) * 3 + (h->mb.i_partition - D_16x8);

            x264_cabac_encode_decision_noup( cb, 27+3,   i_mb_bits[idx][0] );
            x264_cabac_encode_decision( cb, 27+5-i_mb_bits[idx][0], i_mb_bits[idx][1] );
            if( i_mb_bits[idx][2] != 2 )
            {
                x264_cabac_encode_decision( cb, 27+5, i_mb_bits[idx][2] );
                x264_cabac_encode_decision( cb, 27+5, i_mb_bits[idx][3] );
                x264_cabac_encode_decision( cb, 27+5, i_mb_bits[idx][4] );
                if( i_mb_bits[idx][5] != 2 )
                    x264_cabac_encode_decision_noup( cb, 27+5, i_mb_bits[idx][5] );
            }
        }
    }
}

static void x264_cabac_mb_intra4x4_pred_mode( x264_cabac_t *cb, int i_pred, int i_mode )
{
    if( i_pred == i_mode )
        x264_cabac_encode_decision( cb, 68, 1 );
    else
    {
        x264_cabac_encode_decision( cb, 68, 0 );
        if( i_mode > i_pred  )
            i_mode--;
        x264_cabac_encode_decision( cb, 69, (i_mode     )&0x01 );
        x264_cabac_encode_decision( cb, 69, (i_mode >> 1)&0x01 );
        x264_cabac_encode_decision( cb, 69, (i_mode >> 2)      );
    }
}

static void x264_cabac_mb_intra_chroma_pred_mode( x264_t *h, x264_cabac_t *cb )
{
    const int i_mode = x264_mb_pred_mode8x8c_fix[ h->mb.i_chroma_pred_mode ];
    int       ctx = 0;

    /* No need to test for I4x4 or I_16x16 as cache_save handle that */
    if( (h->mb.i_neighbour & MB_LEFT) && h->mb.chroma_pred_mode[h->mb.i_mb_xy - 1] != 0 )
        ctx++;
    if( (h->mb.i_neighbour & MB_TOP) && h->mb.chroma_pred_mode[h->mb.i_mb_top_xy] != 0 )
        ctx++;

    x264_cabac_encode_decision_noup( cb, 64 + ctx, i_mode > 0 );
    if( i_mode > 0 )
    {
        x264_cabac_encode_decision( cb, 64 + 3, i_mode > 1 );
        if( i_mode > 1 )
            x264_cabac_encode_decision_noup( cb, 64 + 3, i_mode > 2 );
    }
}

static void x264_cabac_mb_cbp_luma( x264_t *h, x264_cabac_t *cb )
{
    int cbp = h->mb.i_cbp_luma;
    int cbp_l = h->mb.cache.i_cbp_left;
    int cbp_t = h->mb.cache.i_cbp_top;
    x264_cabac_encode_decision     ( cb, 76 - ((cbp_l >> 1) & 1) - ((cbp_t >> 1) & 2), (cbp >> 0) & 1 );
    x264_cabac_encode_decision     ( cb, 76 - ((cbp   >> 0) & 1) - ((cbp_t >> 2) & 2), (cbp >> 1) & 1 );
    x264_cabac_encode_decision     ( cb, 76 - ((cbp_l >> 3) & 1) - ((cbp   << 1) & 2), (cbp >> 2) & 1 );
    x264_cabac_encode_decision_noup( cb, 76 - ((cbp   >> 2) & 1) - ((cbp   >> 0) & 2), (cbp >> 3) & 1 );
}

static void x264_cabac_mb_cbp_chroma( x264_t *h, x264_cabac_t *cb )
{
    int cbp_a = h->mb.cache.i_cbp_left & 0x30;
    int cbp_b = h->mb.cache.i_cbp_top  & 0x30;
    int ctx = 0;

    if( cbp_a && h->mb.cache.i_cbp_left != -1 ) ctx++;
    if( cbp_b && h->mb.cache.i_cbp_top  != -1 ) ctx+=2;
    if( h->mb.i_cbp_chroma == 0 )
        x264_cabac_encode_decision_noup( cb, 77 + ctx, 0 );
    else
    {
        x264_cabac_encode_decision_noup( cb, 77 + ctx, 1 );

        ctx = 4;
        if( cbp_a == 0x20 ) ctx++;
        if( cbp_b == 0x20 ) ctx += 2;
        x264_cabac_encode_decision_noup( cb, 77 + ctx, h->mb.i_cbp_chroma > 1 );
    }
}

static void x264_cabac_mb_qp_delta( x264_t *h, x264_cabac_t *cb )
{
    int i_dqp = h->mb.i_qp - h->mb.i_last_qp;
    int ctx;

    /* Avoid writing a delta quant if we have an empty i16x16 block, e.g. in a completely flat background area */
    if( h->mb.i_type == I_16x16 && !h->mb.cbp[h->mb.i_mb_xy] )
    {
#if !RDO_SKIP_BS
        h->mb.i_qp = h->mb.i_last_qp;
#endif
        i_dqp = 0;
    }

    /* Since, per the above, empty-CBP I16x16 blocks never have delta quants,
     * we don't have to check for them. */
    ctx = h->mb.i_last_dqp && h->mb.cbp[h->mb.i_mb_prev_xy];

    if( i_dqp != 0 )
    {
        int val = i_dqp <= 0 ? (-2*i_dqp) : (2*i_dqp - 1);
        /* dqp is interpreted modulo 52 */
        if( val >= 51 && val != 52 )
            val = 103 - val;
        do
        {
            x264_cabac_encode_decision( cb, 60 + ctx, 1 );
            ctx = 2+(ctx>>1);
        } while( --val );
    }
    x264_cabac_encode_decision_noup( cb, 60 + ctx, 0 );
}

#if !RDO_SKIP_BS
void x264_cabac_mb_skip( x264_t *h, int b_skip )
{
    int ctx = (h->mb.i_mb_type_left >= 0 && !IS_SKIP( h->mb.i_mb_type_left ))
            + (h->mb.i_mb_type_top >= 0 && !IS_SKIP( h->mb.i_mb_type_top ))
            + (h->sh.i_type == SLICE_TYPE_P ? 11 : 24);
    x264_cabac_encode_decision( &h->cabac, ctx, b_skip );
}
#endif

static inline void x264_cabac_mb_sub_p_partition( x264_cabac_t *cb, int i_sub )
{
    if( i_sub == D_L0_8x8 )
    {
        x264_cabac_encode_decision( cb, 21, 1 );
        return;
    }
    x264_cabac_encode_decision( cb, 21, 0 );
    if( i_sub == D_L0_8x4 )
        x264_cabac_encode_decision( cb, 22, 0 );
    else
    {
        x264_cabac_encode_decision( cb, 22, 1 );
        x264_cabac_encode_decision( cb, 23, i_sub == D_L0_4x8 );
    }
}

static inline void x264_cabac_mb_sub_b_partition( x264_cabac_t *cb, int i_sub )
{
    if( i_sub == D_DIRECT_8x8 )
    {
        x264_cabac_encode_decision( cb, 36, 0 );
        return;
    }
    x264_cabac_encode_decision( cb, 36, 1 );
    if( i_sub == D_BI_8x8 )
    {
        x264_cabac_encode_decision( cb, 37, 1 );
        x264_cabac_encode_decision( cb, 38, 0 );
        x264_cabac_encode_decision( cb, 39, 0 );
        x264_cabac_encode_decision( cb, 39, 0 );
        return;
    }
    x264_cabac_encode_decision( cb, 37, 0 );
    x264_cabac_encode_decision( cb, 39, i_sub == D_L1_8x8 );
}

static inline void x264_cabac_mb_transform_size( x264_t *h, x264_cabac_t *cb )
{
    int ctx = 399 + h->mb.cache.i_neighbour_transform_size;
    x264_cabac_encode_decision_noup( cb, ctx, h->mb.b_transform_8x8 );
}

static void x264_cabac_mb_ref( x264_t *h, x264_cabac_t *cb, int i_list, int idx )
{
    const int i8 = x264_scan8[idx];
    const int i_refa = h->mb.cache.ref[i_list][i8 - 1];
    const int i_refb = h->mb.cache.ref[i_list][i8 - 8];
    int i_ref  = h->mb.cache.ref[i_list][i8];
    int ctx  = 0;

    if( i_refa > 0 && !h->mb.cache.skip[i8 - 1] )
        ctx++;
    if( i_refb > 0 && !h->mb.cache.skip[i8 - 8] )
        ctx += 2;

    while( i_ref > 0 )
    {
        x264_cabac_encode_decision( cb, 54 + ctx, 1 );
        ctx = (ctx>>2)+4;
        i_ref--;
    }
    x264_cabac_encode_decision( cb, 54 + ctx, 0 );
}

static inline void x264_cabac_mb_mvd_cpn( x264_t *h, x264_cabac_t *cb, int i_list, int idx, int l, int mvd, int ctx )
{
    const int i_abs = abs( mvd );
    const int ctxbase = l ? 47 : 40;
    int i;
#if RDO_SKIP_BS
    if( i_abs == 0 )
        x264_cabac_encode_decision( cb, ctxbase + ctx, 0 );
    else
    {
        x264_cabac_encode_decision( cb, ctxbase + ctx, 1 );
        if( i_abs <= 3 )
        {
            for( i = 1; i < i_abs; i++ )
                x264_cabac_encode_decision( cb, ctxbase + i + 2, 1 );
            x264_cabac_encode_decision( cb, ctxbase + i_abs + 2, 0 );
            x264_cabac_encode_bypass( cb, mvd < 0 );
        }
        else
        {
            x264_cabac_encode_decision( cb, ctxbase + 3, 1 );
            x264_cabac_encode_decision( cb, ctxbase + 4, 1 );
            x264_cabac_encode_decision( cb, ctxbase + 5, 1 );
            if( i_abs < 9 )
            {
                cb->f8_bits_encoded += cabac_size_unary[i_abs - 3][cb->state[ctxbase+6]];
                cb->state[ctxbase+6] = cabac_transition_unary[i_abs - 3][cb->state[ctxbase+6]];
            }
            else
            {
                cb->f8_bits_encoded += cabac_size_5ones[cb->state[ctxbase+6]];
                cb->state[ctxbase+6] = cabac_transition_5ones[cb->state[ctxbase+6]];
                x264_cabac_encode_ue_bypass( cb, 3, i_abs - 9 );
            }
        }
    }
#else
    static const uint8_t ctxes[8] = { 3,4,5,6,6,6,6,6 };

    if( i_abs == 0 )
        x264_cabac_encode_decision( cb, ctxbase + ctx, 0 );
    else
    {
        x264_cabac_encode_decision( cb, ctxbase + ctx, 1 );
        if( i_abs < 9 )
        {
            for( i = 1; i < i_abs; i++ )
                x264_cabac_encode_decision( cb, ctxbase + ctxes[i-1], 1 );
            x264_cabac_encode_decision( cb, ctxbase + ctxes[i_abs-1], 0 );
        }
        else
        {
            for( i = 1; i < 9; i++ )
                x264_cabac_encode_decision( cb, ctxbase + ctxes[i-1], 1 );
            x264_cabac_encode_ue_bypass( cb, 3, i_abs - 9 );
        }
        x264_cabac_encode_bypass( cb, mvd < 0 );
    }
#endif
}

static NOINLINE uint32_t x264_cabac_mb_mvd( x264_t *h, x264_cabac_t *cb, int i_list, int idx, int width )
{
    ALIGNED_4( int16_t mvp[2] );
    uint32_t amvd;
    int mdx, mdy;

    /* Calculate mvd */
    x264_mb_predict_mv( h, i_list, idx, width, mvp );
    mdx = h->mb.cache.mv[i_list][x264_scan8[idx]][0] - mvp[0];
    mdy = h->mb.cache.mv[i_list][x264_scan8[idx]][1] - mvp[1];
    amvd = x264_cabac_amvd_sum(h->mb.cache.mvd[i_list][x264_scan8[idx] - 1],
                               h->mb.cache.mvd[i_list][x264_scan8[idx] - 8]);

    /* encode */
    x264_cabac_mb_mvd_cpn( h, cb, i_list, idx, 0, mdx, amvd&0xFFFF );
    x264_cabac_mb_mvd_cpn( h, cb, i_list, idx, 1, mdy, amvd>>16 );

    return pack16to32_mask(mdx,mdy);
}

#define x264_cabac_mb_mvd(h,cb,i_list,idx,width,height)\
do\
{\
    uint32_t mvd = x264_cabac_mb_mvd(h,cb,i_list,idx,width);\
    x264_macroblock_cache_mvd( h, block_idx_x[idx], block_idx_y[idx], width, height, i_list, mvd );\
} while(0)

static inline void x264_cabac_mb8x8_mvd( x264_t *h, x264_cabac_t *cb, int i )
{
    switch( h->mb.i_sub_partition[i] )
    {
        case D_L0_8x8:
            x264_cabac_mb_mvd( h, cb, 0, 4*i, 2, 2 );
            break;
        case D_L0_8x4:
            x264_cabac_mb_mvd( h, cb, 0, 4*i+0, 2, 1 );
            x264_cabac_mb_mvd( h, cb, 0, 4*i+2, 2, 1 );
            break;
        case D_L0_4x8:
            x264_cabac_mb_mvd( h, cb, 0, 4*i+0, 1, 2 );
            x264_cabac_mb_mvd( h, cb, 0, 4*i+1, 1, 2 );
            break;
        case D_L0_4x4:
            x264_cabac_mb_mvd( h, cb, 0, 4*i+0, 1, 1 );
            x264_cabac_mb_mvd( h, cb, 0, 4*i+1, 1, 1 );
            x264_cabac_mb_mvd( h, cb, 0, 4*i+2, 1, 1 );
            x264_cabac_mb_mvd( h, cb, 0, 4*i+3, 1, 1 );
            break;
        default:
            assert(0);
    }
}

/* i_ctxBlockCat: 0-> DC 16x16  i_idx = 0
 *                1-> AC 16x16  i_idx = luma4x4idx
 *                2-> Luma4x4   i_idx = luma4x4idx
 *                3-> DC Chroma i_idx = iCbCr
 *                4-> AC Chroma i_idx = 4 * iCbCr + chroma4x4idx
 *                5-> Luma8x8   i_idx = luma8x8idx
 */

static int ALWAYS_INLINE x264_cabac_mb_cbf_ctxidxinc( x264_t *h, int i_cat, int i_idx, int b_intra )
{
    int i_nza;
    int i_nzb;

    switch( i_cat )
    {
        case DCT_LUMA_AC:
        case DCT_LUMA_4x4:
        case DCT_CHROMA_AC:
            /* no need to test for skip/pcm */
            i_nza = h->mb.cache.non_zero_count[x264_scan8[i_idx] - 1];
            i_nzb = h->mb.cache.non_zero_count[x264_scan8[i_idx] - 8];
            if( x264_constant_p(b_intra) && !b_intra )
                return 85 + 4*i_cat + ((2*i_nzb + i_nza)&0x7f);
            else
            {
                i_nza &= 0x7f + (b_intra << 7);
                i_nzb &= 0x7f + (b_intra << 7);
                return 85 + 4*i_cat + 2*!!i_nzb + !!i_nza;
            }
        case DCT_LUMA_DC:
            i_nza = (h->mb.cache.i_cbp_left >> 8) & 1;
            i_nzb = (h->mb.cache.i_cbp_top  >> 8) & 1;
            return 85 + 4*i_cat + 2*i_nzb + i_nza;
        case DCT_CHROMA_DC:
            /* no need to test skip/pcm */
            i_idx -= 25;
            i_nza = h->mb.cache.i_cbp_left != -1 ? (h->mb.cache.i_cbp_left >> (9 + i_idx)) & 1 : b_intra;
            i_nzb = h->mb.cache.i_cbp_top  != -1 ? (h->mb.cache.i_cbp_top  >> (9 + i_idx)) & 1 : b_intra;
            return 85 + 4*i_cat + 2*i_nzb + i_nza;
        default:
            return 0;
    }
}


static const uint16_t significant_coeff_flag_offset[2][6] = {
    { 105, 120, 134, 149, 152, 402 },
    { 277, 292, 306, 321, 324, 436 }
};
static const uint16_t last_coeff_flag_offset[2][6] = {
    { 166, 181, 195, 210, 213, 417 },
    { 338, 353, 367, 382, 385, 451 }
};
static const uint16_t coeff_abs_level_m1_offset[6] =
    { 227, 237, 247, 257, 266, 426 };
static const uint8_t significant_coeff_flag_offset_8x8[2][63] =
{{
    0, 1, 2, 3, 4, 5, 5, 4, 4, 3, 3, 4, 4, 4, 5, 5,
    4, 4, 4, 4, 3, 3, 6, 7, 7, 7, 8, 9,10, 9, 8, 7,
    7, 6,11,12,13,11, 6, 7, 8, 9,14,10, 9, 8, 6,11,
   12,13,11, 6, 9,14,10, 9,11,12,13,11,14,10,12
},{
    0, 1, 1, 2, 2, 3, 3, 4, 5, 6, 7, 7, 7, 8, 4, 5,
    6, 9,10,10, 8,11,12,11, 9, 9,10,10, 8,11,12,11,
    9, 9,10,10, 8,11,12,11, 9, 9,10,10, 8,13,13, 9,
    9,10,10, 8,13,13, 9, 9,10,10,14,14,14,14,14
}};
static const uint8_t last_coeff_flag_offset_8x8[63] = {
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4,
    5, 5, 5, 5, 6, 6, 6, 6, 7, 7, 7, 7, 8, 8, 8
};

// node ctx: 0..3: abslevel1 (with abslevelgt1 == 0).
//           4..7: abslevelgt1 + 3 (and abslevel1 doesn't matter).
/* map node ctx => cabac ctx for level=1 */
static const int coeff_abs_level1_ctx[8] = { 1, 2, 3, 4, 0, 0, 0, 0 };
/* map node ctx => cabac ctx for level>1 */
static const int coeff_abs_levelgt1_ctx[8] = { 5, 5, 5, 5, 6, 7, 8, 9 };
static const uint8_t coeff_abs_level_transition[2][8] = {
/* update node ctx after coding a level=1 */
    { 1, 2, 3, 3, 4, 5, 6, 7 },
/* update node ctx after coding a level>1 */
    { 4, 4, 4, 4, 5, 6, 7, 7 }
};
static const int count_cat_m1[5] = {15, 14, 15, 3, 14};

#if !RDO_SKIP_BS
static void block_residual_write_cabac( x264_t *h, x264_cabac_t *cb, int i_ctxBlockCat, int16_t *l )
{
    const int i_ctx_sig = significant_coeff_flag_offset[h->mb.b_interlaced][i_ctxBlockCat];
    const int i_ctx_last = last_coeff_flag_offset[h->mb.b_interlaced][i_ctxBlockCat];
    const int i_ctx_level = coeff_abs_level_m1_offset[i_ctxBlockCat];
    const uint8_t *significant_coeff_flag_offset = significant_coeff_flag_offset_8x8[h->mb.b_interlaced];
    int i_coeff_abs_m1[64];
    int i_coeff_sign[64];
    int i_coeff = 0;
    int i_last;
    int node_ctx = 0;
    int i = 0;

    i_last = h->quantf.coeff_last[i_ctxBlockCat](l);

#define WRITE_SIGMAP( l8x8 )\
    while(1)\
    {\
        if( l[i] )\
        {\
            i_coeff_abs_m1[i_coeff] = abs(l[i]) - 1;\
            i_coeff_sign[i_coeff] = l[i] < 0;\
            i_coeff++;\
            x264_cabac_encode_decision( cb, i_ctx_sig + (l8x8 ? significant_coeff_flag_offset[i] : i), 1 );\
            if( i == i_last )\
            {\
                x264_cabac_encode_decision( cb, i_ctx_last + (l8x8 ? last_coeff_flag_offset_8x8[i] : i), 1 );\
                break;\
            }\
            else\
                x264_cabac_encode_decision( cb, i_ctx_last + (l8x8 ? last_coeff_flag_offset_8x8[i] : i), 0 );\
        }\
        else\
            x264_cabac_encode_decision( cb, i_ctx_sig + (l8x8 ? significant_coeff_flag_offset[i] : i), 0 );\
        i++;\
        if( i == i_count_m1 )\
        {\
            i_coeff_abs_m1[i_coeff] = abs(l[i]) - 1;\
            i_coeff_sign[i_coeff]   = l[i] < 0;\
            i_coeff++;\
            break;\
        }\
    }

    if( i_ctxBlockCat == DCT_LUMA_8x8 )
    {
        const int i_count_m1 = 63;
        WRITE_SIGMAP( 1 )
    }
    else
    {
        const int i_count_m1 = count_cat_m1[i_ctxBlockCat];
        WRITE_SIGMAP( 0 )
    }

    do
    {
        int i_prefix, ctx;
        i_coeff--;

        /* write coeff_abs - 1 */
        i_prefix = X264_MIN( i_coeff_abs_m1[i_coeff], 14 );
        ctx = coeff_abs_level1_ctx[node_ctx] + i_ctx_level;

        if( i_prefix )
        {
            x264_cabac_encode_decision( cb, ctx, 1 );
            ctx = coeff_abs_levelgt1_ctx[node_ctx] + i_ctx_level;
            for( i = 0; i < i_prefix - 1; i++ )
                x264_cabac_encode_decision( cb, ctx, 1 );
            if( i_prefix < 14 )
                x264_cabac_encode_decision( cb, ctx, 0 );
            else
                x264_cabac_encode_ue_bypass( cb, 0, i_coeff_abs_m1[i_coeff] - 14 );

            node_ctx = coeff_abs_level_transition[1][node_ctx];
        }
        else
        {
            x264_cabac_encode_decision( cb, ctx, 0 );
            node_ctx = coeff_abs_level_transition[0][node_ctx];
        }

        x264_cabac_encode_bypass( cb, i_coeff_sign[i_coeff] );
    } while( i_coeff > 0 );
}
#define block_residual_write_cabac_8x8( h, cb, l ) block_residual_write_cabac( h, cb, DCT_LUMA_8x8, l )

#else

/* Faster RDO by merging sigmap and level coding.  Note that for 8x8dct
 * this is slightly incorrect because the sigmap is not reversible
 * (contexts are repeated).  However, there is nearly no quality penalty
 * for this (~0.001db) and the speed boost (~30%) is worth it. */
static void ALWAYS_INLINE block_residual_write_cabac_internal( x264_t *h, x264_cabac_t *cb, int i_ctxBlockCat, int16_t *l, int b_8x8 )
{
    const int i_ctx_sig = significant_coeff_flag_offset[h->mb.b_interlaced][i_ctxBlockCat];
    const int i_ctx_last = last_coeff_flag_offset[h->mb.b_interlaced][i_ctxBlockCat];
    const int i_ctx_level = coeff_abs_level_m1_offset[i_ctxBlockCat];
    const uint8_t *significant_coeff_flag_offset = significant_coeff_flag_offset_8x8[h->mb.b_interlaced];
    int i_last, i_coeff_abs, ctx, i, node_ctx;

    i_last = h->quantf.coeff_last[i_ctxBlockCat](l);

    i_coeff_abs = abs(l[i_last]);
    ctx = coeff_abs_level1_ctx[0] + i_ctx_level;

    if( i_last != (b_8x8 ? 63 : count_cat_m1[i_ctxBlockCat]) )
    {
        x264_cabac_encode_decision( cb, i_ctx_sig + (b_8x8?significant_coeff_flag_offset[i_last]:i_last), 1 );
        x264_cabac_encode_decision( cb, i_ctx_last + (b_8x8?last_coeff_flag_offset_8x8[i_last]:i_last), 1 );
    }

    if( i_coeff_abs > 1 )
    {
        x264_cabac_encode_decision( cb, ctx, 1 );
        ctx = coeff_abs_levelgt1_ctx[0] + i_ctx_level;
        if( i_coeff_abs < 15 )
        {
            cb->f8_bits_encoded += cabac_size_unary[i_coeff_abs-1][cb->state[ctx]];
            cb->state[ctx] = cabac_transition_unary[i_coeff_abs-1][cb->state[ctx]];
        }
        else
        {
            cb->f8_bits_encoded += cabac_size_unary[14][cb->state[ctx]];
            cb->state[ctx] = cabac_transition_unary[14][cb->state[ctx]];
            x264_cabac_encode_ue_bypass( cb, 0, i_coeff_abs - 15 );
        }
        node_ctx = coeff_abs_level_transition[1][0];
    }
    else
    {
        x264_cabac_encode_decision( cb, ctx, 0 );
        node_ctx = coeff_abs_level_transition[0][0];
        x264_cabac_encode_bypass( cb, 0 ); // sign
    }

    for( i = i_last-1 ; i >= 0; i-- )
    {
        if( l[i] )
        {
            i_coeff_abs = abs(l[i]);
            x264_cabac_encode_decision( cb, i_ctx_sig + (b_8x8?significant_coeff_flag_offset[i]:i), 1 );
            x264_cabac_encode_decision( cb, i_ctx_last + (b_8x8?last_coeff_flag_offset_8x8[i]:i), 0 );
            ctx = coeff_abs_level1_ctx[node_ctx] + i_ctx_level;

            if( i_coeff_abs > 1 )
            {
                x264_cabac_encode_decision( cb, ctx, 1 );
                ctx = coeff_abs_levelgt1_ctx[node_ctx] + i_ctx_level;
                if( i_coeff_abs < 15 )
                {
                    cb->f8_bits_encoded += cabac_size_unary[i_coeff_abs-1][cb->state[ctx]];
                    cb->state[ctx] = cabac_transition_unary[i_coeff_abs-1][cb->state[ctx]];
                }
                else
                {
                    cb->f8_bits_encoded += cabac_size_unary[14][cb->state[ctx]];
                    cb->state[ctx] = cabac_transition_unary[14][cb->state[ctx]];
                    x264_cabac_encode_ue_bypass( cb, 0, i_coeff_abs - 15 );
                }
                node_ctx = coeff_abs_level_transition[1][node_ctx];
            }
            else
            {
                x264_cabac_encode_decision( cb, ctx, 0 );
                node_ctx = coeff_abs_level_transition[0][node_ctx];
                x264_cabac_encode_bypass( cb, 0 );
            }
        }
        else
            x264_cabac_encode_decision( cb, i_ctx_sig + (b_8x8?significant_coeff_flag_offset[i]:i), 0 );
    }
}

static void block_residual_write_cabac_8x8( x264_t *h, x264_cabac_t *cb, int16_t *l )
{
    block_residual_write_cabac_internal( h, cb, DCT_LUMA_8x8, l, 1 );
}
static void block_residual_write_cabac( x264_t *h, x264_cabac_t *cb, int i_ctxBlockCat, int16_t *l )
{
    block_residual_write_cabac_internal( h, cb, i_ctxBlockCat, l, 0 );
}
#endif

#define block_residual_write_cabac_cbf( h, cb, i_ctxBlockCat, i_idx, l, b_intra ) \
{ \
    int ctxidxinc = x264_cabac_mb_cbf_ctxidxinc( h, i_ctxBlockCat, i_idx, b_intra ); \
    if( h->mb.cache.non_zero_count[x264_scan8[i_idx]] )\
    {\
        x264_cabac_encode_decision( cb, ctxidxinc, 1 );\
        block_residual_write_cabac( h, cb, i_ctxBlockCat, l ); \
    }\
    else\
        x264_cabac_encode_decision( cb, ctxidxinc, 0 );\
}

void x264_macroblock_write_cabac( x264_t *h, x264_cabac_t *cb )
{
    const int i_mb_type = h->mb.i_type;
    int i_list;
    int i;

#if !RDO_SKIP_BS
    const int i_mb_pos_start = x264_cabac_pos( cb );
    int       i_mb_pos_tex;
#endif

    /* Write the MB type */
    x264_cabac_mb_type( h, cb );

#if !RDO_SKIP_BS
    if( i_mb_type == I_PCM )
    {
        i_mb_pos_tex = x264_cabac_pos( cb );
        h->stat.frame.i_mv_bits += i_mb_pos_tex - i_mb_pos_start;

        memcpy( cb->p, h->mb.pic.p_fenc[0], 256 );
        cb->p += 256;
        for( i = 0; i < 8; i++ )
            memcpy( cb->p + i*8, h->mb.pic.p_fenc[1] + i*FENC_STRIDE, 8 );
        cb->p += 64;
        for( i = 0; i < 8; i++ )
            memcpy( cb->p + i*8, h->mb.pic.p_fenc[2] + i*FENC_STRIDE, 8 );
        cb->p += 64;

        cb->i_low   = 0;
        cb->i_range = 0x01FE;
        cb->i_queue = -1;
        cb->i_bytes_outstanding = 0;

        /* if PCM is chosen, we need to store reconstructed frame data */
        h->mc.copy[PIXEL_16x16]( h->mb.pic.p_fdec[0], FDEC_STRIDE, h->mb.pic.p_fenc[0], FENC_STRIDE, 16 );
        h->mc.copy[PIXEL_8x8]  ( h->mb.pic.p_fdec[1], FDEC_STRIDE, h->mb.pic.p_fenc[1], FENC_STRIDE, 8 );
        h->mc.copy[PIXEL_8x8]  ( h->mb.pic.p_fdec[2], FDEC_STRIDE, h->mb.pic.p_fenc[2], FENC_STRIDE, 8 );

        h->stat.frame.i_tex_bits += x264_cabac_pos( cb ) - i_mb_pos_tex;
        return;
    }
#endif

    if( IS_INTRA( i_mb_type ) )
    {
        if( h->pps->b_transform_8x8_mode && i_mb_type != I_16x16 )
            x264_cabac_mb_transform_size( h, cb );

        if( i_mb_type != I_16x16 )
        {
            int di = h->mb.b_transform_8x8 ? 4 : 1;
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
            if( h->mb.pic.i_fref[0] > 1 )
            {
                x264_cabac_mb_ref( h, cb, 0, 0 );
            }
            x264_cabac_mb_mvd( h, cb, 0, 0, 4, 4 );
        }
        else if( h->mb.i_partition == D_16x8 )
        {
            if( h->mb.pic.i_fref[0] > 1 )
            {
                x264_cabac_mb_ref( h, cb, 0, 0 );
                x264_cabac_mb_ref( h, cb, 0, 8 );
            }
            x264_cabac_mb_mvd( h, cb, 0, 0, 4, 2 );
            x264_cabac_mb_mvd( h, cb, 0, 8, 4, 2 );
        }
        else //if( h->mb.i_partition == D_8x16 )
        {
            if( h->mb.pic.i_fref[0] > 1 )
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
        for( i = 0; i < 4; i++ )
            x264_cabac_mb_sub_p_partition( cb, h->mb.i_sub_partition[i] );

        /* ref 0 */
        if( h->mb.pic.i_fref[0] > 1 )
        {
            x264_cabac_mb_ref( h, cb, 0, 0 );
            x264_cabac_mb_ref( h, cb, 0, 4 );
            x264_cabac_mb_ref( h, cb, 0, 8 );
            x264_cabac_mb_ref( h, cb, 0, 12 );
        }

        for( i = 0; i < 4; i++ )
            x264_cabac_mb8x8_mvd( h, cb, i );
    }
    else if( i_mb_type == B_8x8 )
    {
        /* sub mb type */
        for( i = 0; i < 4; i++ )
            x264_cabac_mb_sub_b_partition( cb, h->mb.i_sub_partition[i] );

        /* ref */
        if( h->mb.pic.i_fref[0] > 1 )
            for( i = 0; i < 4; i++ )
                if( x264_mb_partition_listX_table[0][ h->mb.i_sub_partition[i] ] )
                    x264_cabac_mb_ref( h, cb, 0, 4*i );

        if( h->mb.pic.i_fref[1] > 1 )
            for( i = 0; i < 4; i++ )
                if( x264_mb_partition_listX_table[1][ h->mb.i_sub_partition[i] ] )
                    x264_cabac_mb_ref( h, cb, 1, 4*i );

        for( i = 0; i < 4; i++ )
            if( x264_mb_partition_listX_table[0][ h->mb.i_sub_partition[i] ] )
                x264_cabac_mb_mvd( h, cb, 0, 4*i, 2, 2 );

        for( i = 0; i < 4; i++ )
            if( x264_mb_partition_listX_table[1][ h->mb.i_sub_partition[i] ] )
                x264_cabac_mb_mvd( h, cb, 1, 4*i, 2, 2 );
    }
    else if( i_mb_type != B_DIRECT )
    {
        /* All B mode */
        const uint8_t (*b_list)[2] = x264_mb_type_list_table[i_mb_type];
        if( h->mb.pic.i_fref[0] > 1 )
        {
            if( b_list[0][0] )
                x264_cabac_mb_ref( h, cb, 0, 0 );
            if( b_list[0][1] && h->mb.i_partition != D_16x16 )
                x264_cabac_mb_ref( h, cb, 0, 8 >> (h->mb.i_partition == D_8x16) );
        }
        if( h->mb.pic.i_fref[1] > 1 )
        {
            if( b_list[1][0] )
                x264_cabac_mb_ref( h, cb, 1, 0 );
            if( b_list[1][1] && h->mb.i_partition != D_16x16 )
                x264_cabac_mb_ref( h, cb, 1, 8 >> (h->mb.i_partition == D_8x16) );
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
            else //if( h->mb.i_partition == D_8x16 )
            {
                if( b_list[i_list][0] ) x264_cabac_mb_mvd( h, cb, i_list, 0, 2, 4 );
                if( b_list[i_list][1] ) x264_cabac_mb_mvd( h, cb, i_list, 4, 2, 4 );
            }
        }
    }

#if !RDO_SKIP_BS
    i_mb_pos_tex = x264_cabac_pos( cb );
    h->stat.frame.i_mv_bits += i_mb_pos_tex - i_mb_pos_start;
#endif

    if( i_mb_type != I_16x16 )
    {
        x264_cabac_mb_cbp_luma( h, cb );
        x264_cabac_mb_cbp_chroma( h, cb );
    }

    if( x264_mb_transform_8x8_allowed( h ) && h->mb.i_cbp_luma )
    {
        x264_cabac_mb_transform_size( h, cb );
    }

    if( h->mb.i_cbp_luma > 0 || h->mb.i_cbp_chroma > 0 || i_mb_type == I_16x16 )
    {
        const int b_intra = IS_INTRA( i_mb_type );
        x264_cabac_mb_qp_delta( h, cb );

        /* write residual */
        if( i_mb_type == I_16x16 )
        {
            /* DC Luma */
            block_residual_write_cabac_cbf( h, cb, DCT_LUMA_DC, 24, h->dct.luma16x16_dc, 1 );

            /* AC Luma */
            if( h->mb.i_cbp_luma != 0 )
                for( i = 0; i < 16; i++ )
                    block_residual_write_cabac_cbf( h, cb, DCT_LUMA_AC, i, h->dct.luma4x4[i]+1, 1 );
        }
        else if( h->mb.b_transform_8x8 )
        {
            for( i = 0; i < 4; i++ )
                if( h->mb.i_cbp_luma & ( 1 << i ) )
                    block_residual_write_cabac_8x8( h, cb, h->dct.luma8x8[i] );
        }
        else
        {
            for( i = 0; i < 16; i++ )
                if( h->mb.i_cbp_luma & ( 1 << ( i / 4 ) ) )
                    block_residual_write_cabac_cbf( h, cb, DCT_LUMA_4x4, i, h->dct.luma4x4[i], b_intra );
        }

        if( h->mb.i_cbp_chroma&0x03 )    /* Chroma DC residual present */
        {
            block_residual_write_cabac_cbf( h, cb, DCT_CHROMA_DC, 25, h->dct.chroma_dc[0], b_intra );
            block_residual_write_cabac_cbf( h, cb, DCT_CHROMA_DC, 26, h->dct.chroma_dc[1], b_intra );
            if( h->mb.i_cbp_chroma&0x02 ) /* Chroma AC residual present */
                for( i = 16; i < 24; i++ )
                    block_residual_write_cabac_cbf( h, cb, DCT_CHROMA_AC, i, h->dct.luma4x4[i]+1, b_intra );
        }
    }

#if !RDO_SKIP_BS
    h->stat.frame.i_tex_bits += x264_cabac_pos( cb ) - i_mb_pos_tex;
#endif
}

#if RDO_SKIP_BS
/*****************************************************************************
 * RD only; doesn't generate a valid bitstream
 * doesn't write cbp or chroma dc (I don't know how much this matters)
 * doesn't write ref (never varies between calls, so no point in doing so)
 * only writes subpartition for p8x8, needed for sub-8x8 mode decision RDO
 * works on all partition sizes except 16x16
 *****************************************************************************/
static void x264_partition_size_cabac( x264_t *h, x264_cabac_t *cb, int i8, int i_pixel )
{
    const int i_mb_type = h->mb.i_type;
    int b_8x16 = h->mb.i_partition == D_8x16;
    int j;

    if( i_mb_type == P_8x8 )
    {
        x264_cabac_mb8x8_mvd( h, cb, i8 );
        x264_cabac_mb_sub_p_partition( cb, h->mb.i_sub_partition[i8] );
    }
    else if( i_mb_type == P_L0 )
        x264_cabac_mb_mvd( h, cb, 0, 4*i8, 4>>b_8x16, 2<<b_8x16 );
    else if( i_mb_type > B_DIRECT && i_mb_type < B_8x8 )
    {
        if( x264_mb_type_list_table[ i_mb_type ][0][!!i8] ) x264_cabac_mb_mvd( h, cb, 0, 4*i8, 4>>b_8x16, 2<<b_8x16 );
        if( x264_mb_type_list_table[ i_mb_type ][1][!!i8] ) x264_cabac_mb_mvd( h, cb, 1, 4*i8, 4>>b_8x16, 2<<b_8x16 );
    }
    else //if( i_mb_type == B_8x8 )
    {
        if( x264_mb_partition_listX_table[0][ h->mb.i_sub_partition[i8] ] )
            x264_cabac_mb_mvd( h, cb, 0, 4*i8, 2, 2 );
        if( x264_mb_partition_listX_table[1][ h->mb.i_sub_partition[i8] ] )
            x264_cabac_mb_mvd( h, cb, 1, 4*i8, 2, 2 );
    }

    for( j = (i_pixel < PIXEL_8x8); j >= 0; j-- )
    {
        if( h->mb.i_cbp_luma & (1 << i8) )
        {
            if( h->mb.b_transform_8x8 )
                block_residual_write_cabac_8x8( h, cb, h->dct.luma8x8[i8] );
            else
            {
                int i4;
                for( i4 = 0; i4 < 4; i4++ )
                    block_residual_write_cabac_cbf( h, cb, DCT_LUMA_4x4, i4+i8*4, h->dct.luma4x4[i4+i8*4], 0 );
            }
        }

        block_residual_write_cabac_cbf( h, cb, DCT_CHROMA_AC, 16+i8, h->dct.luma4x4[16+i8]+1, 0 );
        block_residual_write_cabac_cbf( h, cb, DCT_CHROMA_AC, 20+i8, h->dct.luma4x4[20+i8]+1, 0 );

        i8 += x264_pixel_size[i_pixel].h >> 3;
    }
}

static void x264_subpartition_size_cabac( x264_t *h, x264_cabac_t *cb, int i4, int i_pixel )
{
    int b_8x4 = i_pixel == PIXEL_8x4;
    block_residual_write_cabac_cbf( h, cb, DCT_LUMA_4x4, i4, h->dct.luma4x4[i4], 0 );
    if( i_pixel == PIXEL_4x4 )
    {
        x264_cabac_mb_mvd( h, cb, 0, i4, 1, 1 );
    }
    else
    {
        x264_cabac_mb_mvd( h, cb, 0, i4, 1+b_8x4, 2-b_8x4 );
        block_residual_write_cabac_cbf( h, cb, DCT_LUMA_4x4, i4+2-b_8x4, h->dct.luma4x4[i4+2-b_8x4], 0 );
    }
}

static void x264_partition_i8x8_size_cabac( x264_t *h, x264_cabac_t *cb, int i8, int i_mode )
{
    const int i_pred = x264_mb_predict_intra4x4_mode( h, 4*i8 );
    i_mode = x264_mb_pred_mode4x4_fix( i_mode );
    x264_cabac_mb_intra4x4_pred_mode( cb, i_pred, i_mode );
    x264_cabac_mb_cbp_luma( h, cb );
    if( h->mb.i_cbp_luma & (1 << i8) )
        block_residual_write_cabac_8x8( h, cb, h->dct.luma8x8[i8] );
}

static void x264_partition_i4x4_size_cabac( x264_t *h, x264_cabac_t *cb, int i4, int i_mode )
{
    const int i_pred = x264_mb_predict_intra4x4_mode( h, i4 );
    i_mode = x264_mb_pred_mode4x4_fix( i_mode );
    x264_cabac_mb_intra4x4_pred_mode( cb, i_pred, i_mode );
    block_residual_write_cabac_cbf( h, cb, DCT_LUMA_4x4, i4, h->dct.luma4x4[i4], 1 );
}

static void x264_i8x8_chroma_size_cabac( x264_t *h, x264_cabac_t *cb )
{
    x264_cabac_mb_intra_chroma_pred_mode( h, cb );
    x264_cabac_mb_cbp_chroma( h, cb );
    if( h->mb.i_cbp_chroma > 0 )
    {
        block_residual_write_cabac_cbf( h, cb, DCT_CHROMA_DC, 25, h->dct.chroma_dc[0], 1 );
        block_residual_write_cabac_cbf( h, cb, DCT_CHROMA_DC, 26, h->dct.chroma_dc[1], 1 );

        if( h->mb.i_cbp_chroma == 2 )
        {
            int i;
            for( i = 16; i < 24; i++ )
                block_residual_write_cabac_cbf( h, cb, DCT_CHROMA_AC, i, h->dct.luma4x4[i]+1, 1 );
        }
    }
}
#endif

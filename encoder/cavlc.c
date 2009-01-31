/*****************************************************************************
 * cavlc.c: h264 encoder library
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

static const uint8_t intra4x4_cbp_to_golomb[48]=
{
  3, 29, 30, 17, 31, 18, 37,  8, 32, 38, 19,  9, 20, 10, 11,  2,
 16, 33, 34, 21, 35, 22, 39,  4, 36, 40, 23,  5, 24,  6,  7,  1,
 41, 42, 43, 25, 44, 26, 46, 12, 45, 47, 27, 13, 28, 14, 15,  0
};
static const uint8_t inter_cbp_to_golomb[48]=
{
  0,  2,  3,  7,  4,  8, 17, 13,  5, 18,  9, 14, 10, 15, 16, 11,
  1, 32, 33, 36, 34, 37, 44, 40, 35, 45, 38, 41, 39, 42, 43, 19,
  6, 24, 25, 20, 26, 21, 46, 28, 27, 47, 22, 29, 23, 30, 31, 12
};
static const uint8_t mb_type_b_to_golomb[3][9]=
{
    { 4,  8, 12, 10,  6, 14, 16, 18, 20 }, /* D_16x8 */
    { 5,  9, 13, 11,  7, 15, 17, 19, 21 }, /* D_8x16 */
    { 1, -1, -1, -1,  2, -1, -1, -1,  3 }  /* D_16x16 */
};
static const uint8_t sub_mb_type_p_to_golomb[4]=
{
    3, 1, 2, 0
};
static const uint8_t sub_mb_type_b_to_golomb[13]=
{
    10,  4,  5,  1, 11,  6,  7,  2, 12,  8,  9,  3,  0
};

#define bs_write_vlc(s,v) bs_write( s, (v).i_size, (v).i_bits )

/****************************************************************************
 * block_residual_write_cavlc:
 ****************************************************************************/
static inline int block_residual_write_cavlc_escape( x264_t *h, bs_t *s, int i_suffix_length, int level )
{
    static const uint16_t next_suffix[7] = { 0, 3, 6, 12, 24, 48, 0xffff };
    int i_level_prefix = 15;
    int mask = level >> 15;
    int abs_level = (level^mask)-mask;
    int i_level_code = abs_level*2-mask-2;
    if( ( i_level_code >> i_suffix_length ) < 15 )
    {
        bs_write( s, (i_level_code >> i_suffix_length) + 1 + i_suffix_length,
                 (1<<i_suffix_length) + (i_level_code & ((1<<i_suffix_length)-1)) );
    }
    else
    {
        i_level_code -= 15 << i_suffix_length;
        if( i_suffix_length == 0 )
            i_level_code -= 15;

        /* If the prefix size exceeds 15, High Profile is required. */
        if( i_level_code >= 1<<12 )
        {
            if( h->sps->i_profile_idc >= PROFILE_HIGH )
            {
                while( i_level_code > 1<<(i_level_prefix-3) )
                {
                    i_level_code -= 1<<(i_level_prefix-3);
                    i_level_prefix++;
                }
            }
            else
            {
#if RDO_SKIP_BS
                /* Weight highly against overflows. */
                s->i_bits_encoded += 1000000;
#else
                x264_log(h, X264_LOG_WARNING, "OVERFLOW levelcode=%d is only allowed in High Profile\n", i_level_code );
                /* clip level, preserving sign */
                i_level_code = (1<<12) - 2 + (i_level_code & 1);
#endif
            }
        }
        bs_write( s, i_level_prefix + 1, 1 );
        bs_write( s, i_level_prefix - 3, i_level_code & ((1<<(i_level_prefix-3))-1) );
    }
    if( i_suffix_length == 0 )
        i_suffix_length++;
    if( abs_level > next_suffix[i_suffix_length] )
        i_suffix_length++;
    return i_suffix_length;
}

static void block_residual_write_cavlc( x264_t *h, bs_t *s, int i_ctxBlockCat, int i_idx, int16_t *l, int i_count )
{
    static const uint8_t ct_index[17] = {0,0,1,1,2,2,2,2,3,3,3,3,3,3,3,3,3};
    static const uint8_t ctz_index[8] = {3,0,1,0,2,0,1,0};
    x264_run_level_t runlevel;
    int i_trailing, i_total_zero, i_suffix_length, i;
    int i_total = 0;
    unsigned int i_sign;
    /* x264_mb_predict_non_zero_code return 0 <-> (16+16+1)>>1 = 16 */
    int nC = i_idx >= 25 ? 4 : ct_index[x264_mb_predict_non_zero_code( h, i_idx == 24 ? 0 : i_idx )];

    if( !h->mb.cache.non_zero_count[x264_scan8[i_idx]] )
    {
        bs_write_vlc( s, x264_coeff0_token[nC] );
        return;
    }

    /* level and run and total */
    /* set these to 2 to allow branchless i_trailing calculation */
    runlevel.level[1] = 2;
    runlevel.level[2] = 2;
    i_total = h->quantf.coeff_level_run[i_ctxBlockCat]( l, &runlevel );
    i_total_zero = runlevel.last + 1 - i_total;

    h->mb.cache.non_zero_count[x264_scan8[i_idx]] = i_total;

    i_trailing = ((((runlevel.level[0]+1) | (1-runlevel.level[0])) >> 31) & 1) // abs(runlevel.level[0])>1
               | ((((runlevel.level[1]+1) | (1-runlevel.level[1])) >> 31) & 2)
               | ((((runlevel.level[2]+1) | (1-runlevel.level[2])) >> 31) & 4);
    i_trailing = ctz_index[i_trailing];
    i_sign = ((runlevel.level[2] >> 31) & 1)
           | ((runlevel.level[1] >> 31) & 2)
           | ((runlevel.level[0] >> 31) & 4);
    i_sign >>= 3-i_trailing;

    /* total/trailing */
    bs_write_vlc( s, x264_coeff_token[nC][i_total*4+i_trailing-4] );

    i_suffix_length = i_total > 10 && i_trailing < 3;
    if( i_trailing > 0 || RDO_SKIP_BS )
        bs_write( s, i_trailing, i_sign );

    if( i_trailing < i_total )
    {
        int16_t val = runlevel.level[i_trailing];
        int16_t val_original = runlevel.level[i_trailing]+LEVEL_TABLE_SIZE/2;
        if( i_trailing < 3 )
            val -= (val>>15)|1; /* as runlevel.level[i] can't be 1 for the first one if i_trailing < 3 */
        val += LEVEL_TABLE_SIZE/2;

        if( (unsigned)val_original < LEVEL_TABLE_SIZE )
        {
            bs_write_vlc( s, x264_level_token[i_suffix_length][val] );
            i_suffix_length = x264_level_token[i_suffix_length][val_original].i_next;
        }
        else
            i_suffix_length = block_residual_write_cavlc_escape( h, s, i_suffix_length, val-LEVEL_TABLE_SIZE/2 );
        for( i = i_trailing+1; i < i_total; i++ )
        {
            val = runlevel.level[i] + LEVEL_TABLE_SIZE/2;
            if( (unsigned)val < LEVEL_TABLE_SIZE )
            {
                bs_write_vlc( s, x264_level_token[i_suffix_length][val] );
                i_suffix_length = x264_level_token[i_suffix_length][val].i_next;
            }
            else
                i_suffix_length = block_residual_write_cavlc_escape( h, s, i_suffix_length, val-LEVEL_TABLE_SIZE/2 );
        }
    }

    if( i_total < i_count )
    {
        if( i_idx >= 25 )
            bs_write_vlc( s, x264_total_zeros_dc[i_total-1][i_total_zero] );
        else
            bs_write_vlc( s, x264_total_zeros[i_total-1][i_total_zero] );
    }

    for( i = 0; i < i_total-1 && i_total_zero > 0; i++ )
    {
        int i_zl = X264_MIN( i_total_zero - 1, 6 );
        bs_write_vlc( s, x264_run_before[i_zl][runlevel.run[i]] );
        i_total_zero -= runlevel.run[i];
    }
}

static void cavlc_qp_delta( x264_t *h, bs_t *s )
{
    int i_dqp = h->mb.i_qp - h->mb.i_last_qp;

    /* Avoid writing a delta quant if we have an empty i16x16 block, e.g. in a completely flat background area */
    if( h->mb.i_type == I_16x16 && !(h->mb.i_cbp_luma | h->mb.i_cbp_chroma)
        && !h->mb.cache.non_zero_count[x264_scan8[24]] )
    {
#if !RDO_SKIP_BS
        h->mb.i_qp = h->mb.i_last_qp;
#endif
        i_dqp = 0;
    }

    if( i_dqp )
    {
        if( i_dqp < -26 )
            i_dqp += 52;
        else if( i_dqp > 25 )
            i_dqp -= 52;
    }
    bs_write_se( s, i_dqp );
}

static void cavlc_mb_mvd( x264_t *h, bs_t *s, int i_list, int idx, int width )
{
    DECLARE_ALIGNED_4( int16_t mvp[2] );
    x264_mb_predict_mv( h, i_list, idx, width, mvp );
    bs_write_se( s, h->mb.cache.mv[i_list][x264_scan8[idx]][0] - mvp[0] );
    bs_write_se( s, h->mb.cache.mv[i_list][x264_scan8[idx]][1] - mvp[1] );
}

static void cavlc_mb8x8_mvd( x264_t *h, bs_t *s, int i_list, int i )
{
    if( !x264_mb_partition_listX_table[i_list][ h->mb.i_sub_partition[i] ] )
        return;

    switch( h->mb.i_sub_partition[i] )
    {
        case D_L0_8x8:
        case D_L1_8x8:
        case D_BI_8x8:
            cavlc_mb_mvd( h, s, i_list, 4*i, 2 );
            break;
        case D_L0_8x4:
        case D_L1_8x4:
        case D_BI_8x4:
            cavlc_mb_mvd( h, s, i_list, 4*i+0, 2 );
            cavlc_mb_mvd( h, s, i_list, 4*i+2, 2 );
            break;
        case D_L0_4x8:
        case D_L1_4x8:
        case D_BI_4x8:
            cavlc_mb_mvd( h, s, i_list, 4*i+0, 1 );
            cavlc_mb_mvd( h, s, i_list, 4*i+1, 1 );
            break;
        case D_L0_4x4:
        case D_L1_4x4:
        case D_BI_4x4:
            cavlc_mb_mvd( h, s, i_list, 4*i+0, 1 );
            cavlc_mb_mvd( h, s, i_list, 4*i+1, 1 );
            cavlc_mb_mvd( h, s, i_list, 4*i+2, 1 );
            cavlc_mb_mvd( h, s, i_list, 4*i+3, 1 );
            break;
    }
}

static inline void x264_macroblock_luma_write_cavlc( x264_t *h, bs_t *s, int i8start, int i8end )
{
    int i8, i4;
    if( h->mb.b_transform_8x8 )
    {
        /* shuffle 8x8 dct coeffs into 4x4 lists */
        for( i8 = i8start; i8 <= i8end; i8++ )
            if( h->mb.i_cbp_luma & (1 << i8) )
                h->zigzagf.interleave_8x8_cavlc( h->dct.luma4x4[i8*4], h->dct.luma8x8[i8], &h->mb.cache.non_zero_count[x264_scan8[i8*4]] );
    }

    for( i8 = i8start; i8 <= i8end; i8++ )
        if( h->mb.i_cbp_luma & (1 << i8) )
            for( i4 = 0; i4 < 4; i4++ )
                block_residual_write_cavlc( h, s, DCT_LUMA_4x4, i4+i8*4, h->dct.luma4x4[i4+i8*4], 16 );
}

/*****************************************************************************
 * x264_macroblock_write:
 *****************************************************************************/
void x264_macroblock_write_cavlc( x264_t *h, bs_t *s )
{
    const int i_mb_type = h->mb.i_type;
    int i_mb_i_offset;
    int i;

#if !RDO_SKIP_BS
    const int i_mb_pos_start = bs_pos( s );
    int       i_mb_pos_tex;
#endif

    switch( h->sh.i_type )
    {
        case SLICE_TYPE_I:
            i_mb_i_offset = 0;
            break;
        case SLICE_TYPE_P:
            i_mb_i_offset = 5;
            break;
        case SLICE_TYPE_B:
            i_mb_i_offset = 23;
            break;
        default:
            x264_log(h, X264_LOG_ERROR, "internal error or slice unsupported\n" );
            return;
    }

    if( h->sh.b_mbaff
        && (!(h->mb.i_mb_y & 1) || IS_SKIP(h->mb.type[h->mb.i_mb_xy - h->mb.i_mb_stride])) )
    {
        bs_write1( s, h->mb.b_interlaced );
    }

#if !RDO_SKIP_BS
    if( i_mb_type == I_PCM)
    {
        bs_write_ue( s, i_mb_i_offset + 25 );
        i_mb_pos_tex = bs_pos( s );
        h->stat.frame.i_mv_bits += i_mb_pos_tex - i_mb_pos_start;

        bs_align_0( s );

        memcpy( s->p, h->mb.pic.p_fenc[0], 256 );
        s->p += 256;
        for( i = 0; i < 8; i++ )
            memcpy( s->p + i*8, h->mb.pic.p_fenc[1] + i*FENC_STRIDE, 8 );
        s->p += 64;
        for( i = 0; i < 8; i++ )
            memcpy( s->p + i*8, h->mb.pic.p_fenc[2] + i*FENC_STRIDE, 8 );
        s->p += 64;

        /* if PCM is chosen, we need to store reconstructed frame data */
        h->mc.copy[PIXEL_16x16]( h->mb.pic.p_fdec[0], FDEC_STRIDE, h->mb.pic.p_fenc[0], FENC_STRIDE, 16 );
        h->mc.copy[PIXEL_8x8]  ( h->mb.pic.p_fdec[1], FDEC_STRIDE, h->mb.pic.p_fenc[1], FENC_STRIDE, 8 );
        h->mc.copy[PIXEL_8x8]  ( h->mb.pic.p_fdec[2], FDEC_STRIDE, h->mb.pic.p_fenc[2], FENC_STRIDE, 8 );

        h->stat.frame.i_tex_bits += bs_pos(s) - i_mb_pos_tex;
        return;
    }
#endif

    /* Write:
      - type
      - prediction
      - mv */
    if( i_mb_type == I_4x4 || i_mb_type == I_8x8 )
    {
        int di = i_mb_type == I_8x8 ? 4 : 1;
        bs_write_ue( s, i_mb_i_offset + 0 );
        if( h->pps->b_transform_8x8_mode )
            bs_write1( s, h->mb.b_transform_8x8 );

        /* Prediction: Luma */
        for( i = 0; i < 16; i += di )
        {
            int i_pred = x264_mb_predict_intra4x4_mode( h, i );
            int i_mode = x264_mb_pred_mode4x4_fix( h->mb.cache.intra4x4_pred_mode[x264_scan8[i]] );

            if( i_pred == i_mode )
                bs_write1( s, 1 );  /* b_prev_intra4x4_pred_mode */
            else
                bs_write( s, 4, i_mode - (i_mode > i_pred) );
        }
        bs_write_ue( s, x264_mb_pred_mode8x8c_fix[ h->mb.i_chroma_pred_mode ] );
    }
    else if( i_mb_type == I_16x16 )
    {
        bs_write_ue( s, i_mb_i_offset + 1 + x264_mb_pred_mode16x16_fix[h->mb.i_intra16x16_pred_mode] +
                        h->mb.i_cbp_chroma * 4 + ( h->mb.i_cbp_luma == 0 ? 0 : 12 ) );
        bs_write_ue( s, x264_mb_pred_mode8x8c_fix[ h->mb.i_chroma_pred_mode ] );
    }
    else if( i_mb_type == P_L0 )
    {
        DECLARE_ALIGNED_4( int16_t mvp[2] );

        if( h->mb.i_partition == D_16x16 )
        {
            bs_write_ue( s, 0 );

            if( h->mb.pic.i_fref[0] > 1 )
                bs_write_te( s, h->mb.pic.i_fref[0] - 1, h->mb.cache.ref[0][x264_scan8[0]] );
            x264_mb_predict_mv( h, 0, 0, 4, mvp );
            bs_write_se( s, h->mb.cache.mv[0][x264_scan8[0]][0] - mvp[0] );
            bs_write_se( s, h->mb.cache.mv[0][x264_scan8[0]][1] - mvp[1] );
        }
        else if( h->mb.i_partition == D_16x8 )
        {
            bs_write_ue( s, 1 );
            if( h->mb.pic.i_fref[0] > 1 )
            {
                bs_write_te( s, h->mb.pic.i_fref[0] - 1, h->mb.cache.ref[0][x264_scan8[0]] );
                bs_write_te( s, h->mb.pic.i_fref[0] - 1, h->mb.cache.ref[0][x264_scan8[8]] );
            }

            x264_mb_predict_mv( h, 0, 0, 4, mvp );
            bs_write_se( s, h->mb.cache.mv[0][x264_scan8[0]][0] - mvp[0] );
            bs_write_se( s, h->mb.cache.mv[0][x264_scan8[0]][1] - mvp[1] );

            x264_mb_predict_mv( h, 0, 8, 4, mvp );
            bs_write_se( s, h->mb.cache.mv[0][x264_scan8[8]][0] - mvp[0] );
            bs_write_se( s, h->mb.cache.mv[0][x264_scan8[8]][1] - mvp[1] );
        }
        else if( h->mb.i_partition == D_8x16 )
        {
            bs_write_ue( s, 2 );
            if( h->mb.pic.i_fref[0] > 1 )
            {
                bs_write_te( s, h->mb.pic.i_fref[0] - 1, h->mb.cache.ref[0][x264_scan8[0]] );
                bs_write_te( s, h->mb.pic.i_fref[0] - 1, h->mb.cache.ref[0][x264_scan8[4]] );
            }

            x264_mb_predict_mv( h, 0, 0, 2, mvp );
            bs_write_se( s, h->mb.cache.mv[0][x264_scan8[0]][0] - mvp[0] );
            bs_write_se( s, h->mb.cache.mv[0][x264_scan8[0]][1] - mvp[1] );

            x264_mb_predict_mv( h, 0, 4, 2, mvp );
            bs_write_se( s, h->mb.cache.mv[0][x264_scan8[4]][0] - mvp[0] );
            bs_write_se( s, h->mb.cache.mv[0][x264_scan8[4]][1] - mvp[1] );
        }
    }
    else if( i_mb_type == P_8x8 )
    {
        int b_sub_ref;
        if( (h->mb.cache.ref[0][x264_scan8[0]] | h->mb.cache.ref[0][x264_scan8[ 4]] |
             h->mb.cache.ref[0][x264_scan8[8]] | h->mb.cache.ref[0][x264_scan8[12]]) == 0 )
        {
            bs_write_ue( s, 4 );
            b_sub_ref = 0;
        }
        else
        {
            bs_write_ue( s, 3 );
            b_sub_ref = h->mb.pic.i_fref[0] > 1;
        }

        /* sub mb type */
        if( h->param.analyse.inter & X264_ANALYSE_PSUB8x8 )
            for( i = 0; i < 4; i++ )
                bs_write_ue( s, sub_mb_type_p_to_golomb[ h->mb.i_sub_partition[i] ] );
        else
            bs_write( s, 4, 0xf );

        /* ref0 */
        if( b_sub_ref )
        {
            bs_write_te( s, h->mb.pic.i_fref[0] - 1, h->mb.cache.ref[0][x264_scan8[0]] );
            bs_write_te( s, h->mb.pic.i_fref[0] - 1, h->mb.cache.ref[0][x264_scan8[4]] );
            bs_write_te( s, h->mb.pic.i_fref[0] - 1, h->mb.cache.ref[0][x264_scan8[8]] );
            bs_write_te( s, h->mb.pic.i_fref[0] - 1, h->mb.cache.ref[0][x264_scan8[12]] );
        }

        for( i = 0; i < 4; i++ )
            cavlc_mb8x8_mvd( h, s, 0, i );
    }
    else if( i_mb_type == B_8x8 )
    {
        bs_write_ue( s, 22 );

        /* sub mb type */
        for( i = 0; i < 4; i++ )
            bs_write_ue( s, sub_mb_type_b_to_golomb[ h->mb.i_sub_partition[i] ] );

        /* ref */
        for( i = 0; i < 4; i++ )
            if( x264_mb_partition_listX_table[0][ h->mb.i_sub_partition[i] ] )
                bs_write_te( s, h->mb.pic.i_fref[0] - 1, h->mb.cache.ref[0][x264_scan8[i*4]] );
        for( i = 0; i < 4; i++ )
            if( x264_mb_partition_listX_table[1][ h->mb.i_sub_partition[i] ] )
                bs_write_te( s, h->mb.pic.i_fref[1] - 1, h->mb.cache.ref[1][x264_scan8[i*4]] );

        /* mvd */
        for( i = 0; i < 4; i++ )
            cavlc_mb8x8_mvd( h, s, 0, i );
        for( i = 0; i < 4; i++ )
            cavlc_mb8x8_mvd( h, s, 1, i );
    }
    else if( i_mb_type != B_DIRECT )
    {
        /* All B mode */
        /* Motion Vector */
        int i_list;
        DECLARE_ALIGNED_4( int16_t mvp[2] );
        const uint8_t (*b_list)[2] = x264_mb_type_list_table[i_mb_type];

        bs_write_ue( s, mb_type_b_to_golomb[ h->mb.i_partition - D_16x8 ][ i_mb_type - B_L0_L0 ] );

        for( i_list = 0; i_list < 2; i_list++ )
        {
            const int i_ref_max = (i_list == 0 ? h->mb.pic.i_fref[0] : h->mb.pic.i_fref[1]) - 1;

            if( i_ref_max )
                switch( h->mb.i_partition )
                {
                    case D_16x16:
                        if( b_list[i_list][0] ) bs_write_te( s, i_ref_max, h->mb.cache.ref[i_list][x264_scan8[0]] );
                        break;
                    case D_16x8:
                        if( b_list[i_list][0] ) bs_write_te( s, i_ref_max, h->mb.cache.ref[i_list][x264_scan8[0]] );
                        if( b_list[i_list][1] ) bs_write_te( s, i_ref_max, h->mb.cache.ref[i_list][x264_scan8[8]] );
                        break;
                    case D_8x16:
                        if( b_list[i_list][0] ) bs_write_te( s, i_ref_max, h->mb.cache.ref[i_list][x264_scan8[0]] );
                        if( b_list[i_list][1] ) bs_write_te( s, i_ref_max, h->mb.cache.ref[i_list][x264_scan8[4]] );
                        break;
                }
        }
        for( i_list = 0; i_list < 2; i_list++ )
        {
            switch( h->mb.i_partition )
            {
                case D_16x16:
                    if( b_list[i_list][0] )
                    {
                        x264_mb_predict_mv( h, i_list, 0, 4, mvp );
                        bs_write_se( s, h->mb.cache.mv[i_list][x264_scan8[0]][0] - mvp[0] );
                        bs_write_se( s, h->mb.cache.mv[i_list][x264_scan8[0]][1] - mvp[1] );
                    }
                    break;
                case D_16x8:
                    if( b_list[i_list][0] )
                    {
                        x264_mb_predict_mv( h, i_list, 0, 4, mvp );
                        bs_write_se( s, h->mb.cache.mv[i_list][x264_scan8[0]][0] - mvp[0] );
                        bs_write_se( s, h->mb.cache.mv[i_list][x264_scan8[0]][1] - mvp[1] );
                    }
                    if( b_list[i_list][1] )
                    {
                        x264_mb_predict_mv( h, i_list, 8, 4, mvp );
                        bs_write_se( s, h->mb.cache.mv[i_list][x264_scan8[8]][0] - mvp[0] );
                        bs_write_se( s, h->mb.cache.mv[i_list][x264_scan8[8]][1] - mvp[1] );
                    }
                    break;
                case D_8x16:
                    if( b_list[i_list][0] )
                    {
                        x264_mb_predict_mv( h, i_list, 0, 2, mvp );
                        bs_write_se( s, h->mb.cache.mv[i_list][x264_scan8[0]][0] - mvp[0] );
                        bs_write_se( s, h->mb.cache.mv[i_list][x264_scan8[0]][1] - mvp[1] );
                    }
                    if( b_list[i_list][1] )
                    {
                        x264_mb_predict_mv( h, i_list, 4, 2, mvp );
                        bs_write_se( s, h->mb.cache.mv[i_list][x264_scan8[4]][0] - mvp[0] );
                        bs_write_se( s, h->mb.cache.mv[i_list][x264_scan8[4]][1] - mvp[1] );
                    }
                    break;
            }
        }
    }
    else if( i_mb_type == B_DIRECT )
        bs_write_ue( s, 0 );
    else
    {
        x264_log(h, X264_LOG_ERROR, "invalid/unhandled mb_type\n" );
        return;
    }

#if !RDO_SKIP_BS
    i_mb_pos_tex = bs_pos( s );
    h->stat.frame.i_mv_bits += i_mb_pos_tex - i_mb_pos_start;
#endif

    /* Coded block patern */
    if( i_mb_type == I_4x4 || i_mb_type == I_8x8 )
        bs_write_ue( s, intra4x4_cbp_to_golomb[( h->mb.i_cbp_chroma << 4 )|h->mb.i_cbp_luma] );
    else if( i_mb_type != I_16x16 )
        bs_write_ue( s, inter_cbp_to_golomb[( h->mb.i_cbp_chroma << 4 )|h->mb.i_cbp_luma] );

    /* transform size 8x8 flag */
    if( x264_mb_transform_8x8_allowed( h ) && h->mb.i_cbp_luma )
        bs_write1( s, h->mb.b_transform_8x8 );

    /* write residual */
    if( i_mb_type == I_16x16 )
    {
        cavlc_qp_delta( h, s );

        /* DC Luma */
        block_residual_write_cavlc( h, s, DCT_LUMA_DC, 24 , h->dct.luma16x16_dc, 16 );

        /* AC Luma */
        if( h->mb.i_cbp_luma )
            for( i = 0; i < 16; i++ )
                block_residual_write_cavlc( h, s, DCT_LUMA_AC, i, h->dct.luma4x4[i]+1, 15 );
    }
    else if( h->mb.i_cbp_luma | h->mb.i_cbp_chroma )
    {
        cavlc_qp_delta( h, s );
        x264_macroblock_luma_write_cavlc( h, s, 0, 3 );
    }
    if( h->mb.i_cbp_chroma )
    {
        /* Chroma DC residual present */
        block_residual_write_cavlc( h, s, DCT_CHROMA_DC, 25, h->dct.chroma_dc[0], 4 );
        block_residual_write_cavlc( h, s, DCT_CHROMA_DC, 26, h->dct.chroma_dc[1], 4 );
        if( h->mb.i_cbp_chroma&0x02 ) /* Chroma AC residual present */
            for( i = 16; i < 24; i++ )
                block_residual_write_cavlc( h, s, DCT_CHROMA_AC, i, h->dct.luma4x4[i]+1, 15 );
    }

#if !RDO_SKIP_BS
    h->stat.frame.i_tex_bits += bs_pos(s) - i_mb_pos_tex;
#endif
}

#if RDO_SKIP_BS
/*****************************************************************************
 * RD only; doesn't generate a valid bitstream
 * doesn't write cbp or chroma dc (I don't know how much this matters)
 * doesn't write ref or subpartition (never varies between calls, so no point in doing so)
 * works on all partition sizes except 16x16
 * for sub8x8, call once per 8x8 block
 *****************************************************************************/
static int x264_partition_size_cavlc( x264_t *h, int i8, int i_pixel )
{
    bs_t s;
    const int i_mb_type = h->mb.i_type;
    int b_8x16 = h->mb.i_partition == D_8x16;
    int j;

    s.i_bits_encoded = 0;

    if( i_mb_type == P_8x8 )
        cavlc_mb8x8_mvd( h, &s, 0, i8 );
    else if( i_mb_type == P_L0 )
        cavlc_mb_mvd( h, &s, 0, 4*i8, 4>>b_8x16 );
    else if( i_mb_type > B_DIRECT && i_mb_type < B_8x8 )
    {
        if( x264_mb_type_list_table[ i_mb_type ][0][!!i8] ) cavlc_mb_mvd( h, &s, 0, 4*i8, 4>>b_8x16 );
        if( x264_mb_type_list_table[ i_mb_type ][1][!!i8] ) cavlc_mb_mvd( h, &s, 1, 4*i8, 4>>b_8x16 );
    }
    else if( i_mb_type == B_8x8 )
    {
        cavlc_mb8x8_mvd( h, &s, 0, i8 );
        cavlc_mb8x8_mvd( h, &s, 1, i8 );
    }
    else
    {
        x264_log(h, X264_LOG_ERROR, "invalid/unhandled mb_type\n" );
        return 0;
    }

    for( j = (i_pixel < PIXEL_8x8); j >= 0; j-- )
    {
        x264_macroblock_luma_write_cavlc( h, &s, i8, i8 );
        block_residual_write_cavlc( h, &s, DCT_CHROMA_AC, 16+i8, h->dct.luma4x4[16+i8]+1, 15 );
        block_residual_write_cavlc( h, &s, DCT_CHROMA_AC, 20+i8, h->dct.luma4x4[20+i8]+1, 15 );
        i8 += x264_pixel_size[i_pixel].h >> 3;
    }

    return s.i_bits_encoded;
}

static int x264_subpartition_size_cavlc( x264_t *h, int i4, int i_pixel )
{
    bs_t s;
    int b_8x4 = i_pixel == PIXEL_8x4;
    s.i_bits_encoded = 0;
    cavlc_mb_mvd( h, &s, 0, i4, 1+b_8x4 );
    block_residual_write_cavlc( h, &s, DCT_LUMA_4x4, i4, h->dct.luma4x4[i4], 16 );
    if( i_pixel != PIXEL_4x4 )
    {
        i4 += 2-b_8x4;
        block_residual_write_cavlc( h, &s, DCT_LUMA_4x4, i4, h->dct.luma4x4[i4], 16 );
    }

    return s.i_bits_encoded;
}

static int cavlc_intra4x4_pred_size( x264_t *h, int i4, int i_mode )
{
    if( x264_mb_predict_intra4x4_mode( h, i4 ) == x264_mb_pred_mode4x4_fix( i_mode ) )
        return 1;
    else
        return 4;
}

static int x264_partition_i8x8_size_cavlc( x264_t *h, int i8, int i_mode )
{
    h->out.bs.i_bits_encoded = cavlc_intra4x4_pred_size( h, 4*i8, i_mode );
    bs_write_ue( &h->out.bs, intra4x4_cbp_to_golomb[( h->mb.i_cbp_chroma << 4 )|h->mb.i_cbp_luma] );
    x264_macroblock_luma_write_cavlc( h, &h->out.bs, i8, i8 );
    return h->out.bs.i_bits_encoded;
}

static int x264_partition_i4x4_size_cavlc( x264_t *h, int i4, int i_mode )
{
    h->out.bs.i_bits_encoded = cavlc_intra4x4_pred_size( h, i4, i_mode );
    block_residual_write_cavlc( h, &h->out.bs, DCT_LUMA_4x4, i4, h->dct.luma4x4[i4], 16 );
    return h->out.bs.i_bits_encoded;
}

static int x264_i8x8_chroma_size_cavlc( x264_t *h )
{
    h->out.bs.i_bits_encoded = bs_size_ue( x264_mb_pred_mode8x8c_fix[ h->mb.i_chroma_pred_mode ] );
    if( h->mb.i_cbp_chroma )
    {
        block_residual_write_cavlc( h, &h->out.bs, DCT_CHROMA_DC, 25, h->dct.chroma_dc[0], 4 );
        block_residual_write_cavlc( h, &h->out.bs, DCT_CHROMA_DC, 26, h->dct.chroma_dc[1], 4 );

        if( h->mb.i_cbp_chroma == 2 )
        {
            int i;
            for( i = 16; i < 24; i++ )
                block_residual_write_cavlc( h, &h->out.bs, DCT_CHROMA_AC, i, h->dct.luma4x4[i]+1, 15 );
        }
    }
    return h->out.bs.i_bits_encoded;
}
#endif

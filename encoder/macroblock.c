/*****************************************************************************
 * macroblock.c: macroblock encoding
 *****************************************************************************
 * Copyright (C) 2003-2010 x264 project
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
 *
 * This program is also available under a commercial proprietary license.
 * For more information, contact us at licensing@x264.com.
 *****************************************************************************/

#include "common/common.h"
#include "macroblock.h"

/* These chroma DC functions don't have assembly versions and are only used here. */

#define ZIG(i,y,x) level[i] = dct[x*2+y];
static inline void zigzag_scan_2x2_dc( dctcoef level[4], dctcoef dct[4] )
{
    ZIG(0,0,0)
    ZIG(1,0,1)
    ZIG(2,1,0)
    ZIG(3,1,1)
}
#undef ZIG

#define IDCT_DEQUANT_START \
    int d0 = dct[0] + dct[1]; \
    int d1 = dct[2] + dct[3]; \
    int d2 = dct[0] - dct[1]; \
    int d3 = dct[2] - dct[3]; \
    int dmf = dequant_mf[i_qp%6][0] << i_qp/6;

static inline void idct_dequant_2x2_dc( dctcoef dct[4], dctcoef dct4x4[4][16], int dequant_mf[6][16], int i_qp )
{
    IDCT_DEQUANT_START
    dct4x4[0][0] = (d0 + d1) * dmf >> 5;
    dct4x4[1][0] = (d0 - d1) * dmf >> 5;
    dct4x4[2][0] = (d2 + d3) * dmf >> 5;
    dct4x4[3][0] = (d2 - d3) * dmf >> 5;
}

static inline void idct_dequant_2x2_dconly( dctcoef out[4], dctcoef dct[4], int dequant_mf[6][16], int i_qp )
{
    IDCT_DEQUANT_START
    out[0] = (d0 + d1) * dmf >> 5;
    out[1] = (d0 - d1) * dmf >> 5;
    out[2] = (d2 + d3) * dmf >> 5;
    out[3] = (d2 - d3) * dmf >> 5;
}

static inline void dct2x2dc( dctcoef d[4], dctcoef dct4x4[4][16] )
{
    int d0 = dct4x4[0][0] + dct4x4[1][0];
    int d1 = dct4x4[2][0] + dct4x4[3][0];
    int d2 = dct4x4[0][0] - dct4x4[1][0];
    int d3 = dct4x4[2][0] - dct4x4[3][0];
    d[0] = d0 + d1;
    d[2] = d2 + d3;
    d[1] = d0 - d1;
    d[3] = d2 - d3;
    dct4x4[0][0] = 0;
    dct4x4[1][0] = 0;
    dct4x4[2][0] = 0;
    dct4x4[3][0] = 0;
}

static ALWAYS_INLINE int x264_quant_4x4( x264_t *h, dctcoef dct[16], int i_qp, int ctx_block_cat, int b_intra, int idx )
{
    int i_quant_cat = b_intra ? CQM_4IY : CQM_4PY;
    if( h->mb.b_trellis )
        return x264_quant_4x4_trellis( h, dct, i_quant_cat, i_qp, ctx_block_cat, b_intra, 0, idx );
    else
        return h->quantf.quant_4x4( dct, h->quant4_mf[i_quant_cat][i_qp], h->quant4_bias[i_quant_cat][i_qp] );
}

static ALWAYS_INLINE int x264_quant_8x8( x264_t *h, dctcoef dct[64], int i_qp, int b_intra, int idx )
{
    int i_quant_cat = b_intra ? CQM_8IY : CQM_8PY;
    if( h->mb.b_trellis )
        return x264_quant_8x8_trellis( h, dct, i_quant_cat, i_qp, b_intra, idx );
    else
        return h->quantf.quant_8x8( dct, h->quant8_mf[i_quant_cat][i_qp], h->quant8_bias[i_quant_cat][i_qp] );
}

/* All encoding functions must output the correct CBP and NNZ values.
 * The entropy coding functions will check CBP first, then NNZ, before
 * actually reading the DCT coefficients.  NNZ still must be correct even
 * if CBP is zero because of the use of NNZ values for context selection.
 * "NNZ" need only be 0 or 1 rather than the exact coefficient count because
 * that is only needed in CAVLC, and will be calculated by CAVLC's residual
 * coding and stored as necessary. */

/* This means that decimation can be done merely by adjusting the CBP and NNZ
 * rather than memsetting the coefficients. */

void x264_mb_encode_i4x4( x264_t *h, int idx, int i_qp )
{
    int nz;
    pixel *p_src = &h->mb.pic.p_fenc[0][block_idx_xy_fenc[idx]];
    pixel *p_dst = &h->mb.pic.p_fdec[0][block_idx_xy_fdec[idx]];
    ALIGNED_ARRAY_16( dctcoef, dct4x4,[16] );

    if( h->mb.b_lossless )
    {
        nz = h->zigzagf.sub_4x4( h->dct.luma4x4[idx], p_src, p_dst );
        h->mb.cache.non_zero_count[x264_scan8[idx]] = nz;
        h->mb.i_cbp_luma |= nz<<(idx>>2);
        return;
    }

    h->dctf.sub4x4_dct( dct4x4, p_src, p_dst );

    nz = x264_quant_4x4( h, dct4x4, i_qp, DCT_LUMA_4x4, 1, idx );
    h->mb.cache.non_zero_count[x264_scan8[idx]] = nz;
    if( nz )
    {
        h->mb.i_cbp_luma |= 1<<(idx>>2);
        h->zigzagf.scan_4x4( h->dct.luma4x4[idx], dct4x4 );
        h->quantf.dequant_4x4( dct4x4, h->dequant4_mf[CQM_4IY], i_qp );
        h->dctf.add4x4_idct( p_dst, dct4x4 );
    }
}

#define STORE_8x8_NNZ( s8, nz )\
do\
{\
    M16( &h->mb.cache.non_zero_count[(s8) + 0*8] ) = (nz) * 0x0101;\
    M16( &h->mb.cache.non_zero_count[(s8) + 1*8] ) = (nz) * 0x0101;\
} while(0)

#define CLEAR_16x16_NNZ \
{\
    M32( &h->mb.cache.non_zero_count[x264_scan8[ 0]] ) = 0;\
    M32( &h->mb.cache.non_zero_count[x264_scan8[ 2]] ) = 0;\
    M32( &h->mb.cache.non_zero_count[x264_scan8[ 8]] ) = 0;\
    M32( &h->mb.cache.non_zero_count[x264_scan8[10]] ) = 0;\
}

void x264_mb_encode_i8x8( x264_t *h, int idx, int i_qp )
{
    int x = idx&1;
    int y = idx>>1;
    int s8 = X264_SCAN8_0 + 2*x + 16*y;
    int nz;
    pixel *p_src = &h->mb.pic.p_fenc[0][8*x + 8*y*FENC_STRIDE];
    pixel *p_dst = &h->mb.pic.p_fdec[0][8*x + 8*y*FDEC_STRIDE];
    ALIGNED_ARRAY_16( dctcoef, dct8x8,[64] );

    if( h->mb.b_lossless )
    {
        nz = h->zigzagf.sub_8x8( h->dct.luma8x8[idx], p_src, p_dst );
        STORE_8x8_NNZ( s8, nz );
        h->mb.i_cbp_luma |= nz<<idx;
        return;
    }

    h->dctf.sub8x8_dct8( dct8x8, p_src, p_dst );

    nz = x264_quant_8x8( h, dct8x8, i_qp, 1, idx );
    if( nz )
    {
        h->mb.i_cbp_luma |= 1<<idx;
        h->zigzagf.scan_8x8( h->dct.luma8x8[idx], dct8x8 );
        h->quantf.dequant_8x8( dct8x8, h->dequant8_mf[CQM_8IY], i_qp );
        h->dctf.add8x8_idct8( p_dst, dct8x8 );
        STORE_8x8_NNZ( s8, 1 );
    }
    else
        STORE_8x8_NNZ( s8, 0 );
}

static void x264_mb_encode_i16x16( x264_t *h, int i_qp )
{
    pixel *p_src = h->mb.pic.p_fenc[0];
    pixel *p_dst = h->mb.pic.p_fdec[0];

    ALIGNED_ARRAY_16( dctcoef, dct4x4,[16],[16] );
    ALIGNED_ARRAY_16( dctcoef, dct_dc4x4,[16] );

    int nz;
    int decimate_score = h->mb.b_dct_decimate ? 0 : 9;

    if( h->mb.b_lossless )
    {
        for( int i = 0; i < 16; i++ )
        {
            int oe = block_idx_xy_fenc[i];
            int od = block_idx_xy_fdec[i];
            nz = h->zigzagf.sub_4x4ac( h->dct.luma4x4[i], p_src+oe, p_dst+od, &dct_dc4x4[block_idx_yx_1d[i]] );
            h->mb.cache.non_zero_count[x264_scan8[i]] = nz;
            h->mb.i_cbp_luma |= nz;
        }
        h->mb.i_cbp_luma *= 0xf;
        h->mb.cache.non_zero_count[x264_scan8[24]] = array_non_zero( dct_dc4x4 );
        h->zigzagf.scan_4x4( h->dct.luma16x16_dc, dct_dc4x4 );
        return;
    }

    h->dctf.sub16x16_dct( dct4x4, p_src, p_dst );

    for( int i = 0; i < 16; i++ )
    {
        /* copy dc coeff */
        dct_dc4x4[block_idx_xy_1d[i]] = dct4x4[i][0];
        dct4x4[i][0] = 0;

        /* quant/scan/dequant */
        nz = x264_quant_4x4( h, dct4x4[i], i_qp, DCT_LUMA_AC, 1, i );
        h->mb.cache.non_zero_count[x264_scan8[i]] = nz;
        if( nz )
        {
            h->zigzagf.scan_4x4( h->dct.luma4x4[i], dct4x4[i] );
            h->quantf.dequant_4x4( dct4x4[i], h->dequant4_mf[CQM_4IY], i_qp );
            if( decimate_score < 6 ) decimate_score += h->quantf.decimate_score15( h->dct.luma4x4[i] );
            h->mb.i_cbp_luma = 0xf;
        }
    }

    /* Writing the 16 CBFs in an i16x16 block is quite costly, so decimation can save many bits. */
    /* More useful with CAVLC, but still useful with CABAC. */
    if( decimate_score < 6 )
    {
        h->mb.i_cbp_luma = 0;
        CLEAR_16x16_NNZ
    }

    h->dctf.dct4x4dc( dct_dc4x4 );
    if( h->mb.b_trellis )
        nz = x264_quant_dc_trellis( h, dct_dc4x4, CQM_4IY, i_qp, DCT_LUMA_DC, 1, 0 );
    else
        nz = h->quantf.quant_4x4_dc( dct_dc4x4, h->quant4_mf[CQM_4IY][i_qp][0]>>1, h->quant4_bias[CQM_4IY][i_qp][0]<<1 );

    h->mb.cache.non_zero_count[x264_scan8[24]] = nz;
    if( nz )
    {
        h->zigzagf.scan_4x4( h->dct.luma16x16_dc, dct_dc4x4 );

        /* output samples to fdec */
        h->dctf.idct4x4dc( dct_dc4x4 );
        h->quantf.dequant_4x4_dc( dct_dc4x4, h->dequant4_mf[CQM_4IY], i_qp );  /* XXX not inversed */
        if( h->mb.i_cbp_luma )
            for( int i = 0; i < 16; i++ )
                dct4x4[i][0] = dct_dc4x4[block_idx_xy_1d[i]];
    }

    /* put pixels to fdec */
    if( h->mb.i_cbp_luma )
        h->dctf.add16x16_idct( p_dst, dct4x4 );
    else if( nz )
        h->dctf.add16x16_idct_dc( p_dst, dct_dc4x4 );
}

static inline int idct_dequant_round_2x2_dc( dctcoef ref[4], dctcoef dct[4], int dequant_mf[6][16], int i_qp )
{
    dctcoef out[4];
    idct_dequant_2x2_dconly( out, dct, dequant_mf, i_qp );
    return ((ref[0] ^ (out[0]+32))
          | (ref[1] ^ (out[1]+32))
          | (ref[2] ^ (out[2]+32))
          | (ref[3] ^ (out[3]+32))) >> 6;
}

/* Round down coefficients losslessly in DC-only chroma blocks.
 * Unlike luma blocks, this can't be done with a lookup table or
 * other shortcut technique because of the interdependencies
 * between the coefficients due to the chroma DC transform. */
static inline int x264_mb_optimize_chroma_dc( x264_t *h, int b_inter, int i_qp, dctcoef dct2x2[4] )
{
    dctcoef dct2x2_orig[4];
    int coeff, nz;

    /* If the QP is too high, there's no benefit to rounding optimization. */
    if( h->dequant4_mf[CQM_4IC + b_inter][i_qp%6][0] << (i_qp/6) > 32*64 )
        return 1;

    idct_dequant_2x2_dconly( dct2x2_orig, dct2x2, h->dequant4_mf[CQM_4IC + b_inter], i_qp );
    dct2x2_orig[0] += 32;
    dct2x2_orig[1] += 32;
    dct2x2_orig[2] += 32;
    dct2x2_orig[3] += 32;

    /* If the DC coefficients already round to zero, terminate early. */
    if( !((dct2x2_orig[0]|dct2x2_orig[1]|dct2x2_orig[2]|dct2x2_orig[3])>>6) )
        return 0;

    /* Start with the highest frequency coefficient... is this the best option? */
    for( nz = 0, coeff = h->quantf.coeff_last[DCT_CHROMA_DC]( dct2x2 ); coeff >= 0; coeff-- )
    {
        int level = dct2x2[coeff];
        int sign = level>>31 | 1; /* dct2x2[coeff] < 0 ? -1 : 1 */

        while( level )
        {
            dct2x2[coeff] = level - sign;
            if( idct_dequant_round_2x2_dc( dct2x2_orig, dct2x2, h->dequant4_mf[CQM_4IC + b_inter], i_qp ) )
            {
                nz = 1;
                dct2x2[coeff] = level;
                break;
            }
            level -= sign;
        }
    }

    return nz;
}

void x264_mb_encode_8x8_chroma( x264_t *h, int b_inter, int i_qp )
{
    int nz, nz_dc;
    int b_decimate = b_inter && h->mb.b_dct_decimate;
    ALIGNED_ARRAY_16( dctcoef, dct2x2,[4] );
    h->mb.i_cbp_chroma = 0;

    /* Early termination: check variance of chroma residual before encoding.
     * Don't bother trying early termination at low QPs.
     * Values are experimentally derived. */
    if( b_decimate && i_qp >= (h->mb.b_trellis ? 12 : 18) )
    {
        int thresh = (x264_lambda2_tab[i_qp] + 32) >> 6;
        int ssd[2];
        int score = h->pixf.var2_8x8( h->mb.pic.p_fenc[1], FENC_STRIDE, h->mb.pic.p_fdec[1], FDEC_STRIDE, &ssd[0] );
        if( score < thresh*4 )
            score += h->pixf.var2_8x8( h->mb.pic.p_fenc[2], FENC_STRIDE, h->mb.pic.p_fdec[2], FDEC_STRIDE, &ssd[1] );
        if( score < thresh*4 )
        {
            h->mb.cache.non_zero_count[x264_scan8[16]] = 0;
            h->mb.cache.non_zero_count[x264_scan8[17]] = 0;
            h->mb.cache.non_zero_count[x264_scan8[18]] = 0;
            h->mb.cache.non_zero_count[x264_scan8[19]] = 0;
            h->mb.cache.non_zero_count[x264_scan8[20]] = 0;
            h->mb.cache.non_zero_count[x264_scan8[21]] = 0;
            h->mb.cache.non_zero_count[x264_scan8[22]] = 0;
            h->mb.cache.non_zero_count[x264_scan8[23]] = 0;
            M16( &h->mb.cache.non_zero_count[x264_scan8[25]] ) = 0;

            for( int ch = 0; ch < 2; ch++ )
            {
                if( ssd[ch] > thresh )
                {
                    h->dctf.sub8x8_dct_dc( dct2x2, h->mb.pic.p_fenc[1+ch], h->mb.pic.p_fdec[1+ch] );
                    if( h->mb.b_trellis )
                        nz_dc = x264_quant_dc_trellis( h, dct2x2, CQM_4IC+b_inter, i_qp, DCT_CHROMA_DC, !b_inter, 1 );
                    else
                        nz_dc = h->quantf.quant_2x2_dc( dct2x2, h->quant4_mf[CQM_4IC+b_inter][i_qp][0]>>1, h->quant4_bias[CQM_4IC+b_inter][i_qp][0]<<1 );

                    if( nz_dc )
                    {
                        if( !x264_mb_optimize_chroma_dc( h, b_inter, i_qp, dct2x2 ) )
                            continue;
                        h->mb.cache.non_zero_count[x264_scan8[25]+ch] = 1;
                        zigzag_scan_2x2_dc( h->dct.chroma_dc[ch], dct2x2 );
                        idct_dequant_2x2_dconly( dct2x2, dct2x2, h->dequant4_mf[CQM_4IC + b_inter], i_qp );
                        h->dctf.add8x8_idct_dc( h->mb.pic.p_fdec[1+ch], dct2x2 );
                        h->mb.i_cbp_chroma = 1;
                    }
                }
            }
            return;
        }
    }

    for( int ch = 0; ch < 2; ch++ )
    {
        pixel *p_src = h->mb.pic.p_fenc[1+ch];
        pixel *p_dst = h->mb.pic.p_fdec[1+ch];
        int i_decimate_score = 0;
        int nz_ac = 0;

        ALIGNED_ARRAY_16( dctcoef, dct4x4,[4],[16] );

        if( h->mb.b_lossless )
        {
            for( int i = 0; i < 4; i++ )
            {
                int oe = block_idx_x[i]*4 + block_idx_y[i]*4*FENC_STRIDE;
                int od = block_idx_x[i]*4 + block_idx_y[i]*4*FDEC_STRIDE;
                nz = h->zigzagf.sub_4x4ac( h->dct.luma4x4[16+i+ch*4], p_src+oe, p_dst+od, &h->dct.chroma_dc[ch][i] );
                h->mb.cache.non_zero_count[x264_scan8[16+i+ch*4]] = nz;
                h->mb.i_cbp_chroma |= nz;
            }
            h->mb.cache.non_zero_count[x264_scan8[25]+ch] = array_non_zero( h->dct.chroma_dc[ch] );
            continue;
        }

        h->dctf.sub8x8_dct( dct4x4, p_src, p_dst );
        dct2x2dc( dct2x2, dct4x4 );
        /* calculate dct coeffs */
        for( int i = 0; i < 4; i++ )
        {
            if( h->mb.b_trellis )
                nz = x264_quant_4x4_trellis( h, dct4x4[i], CQM_4IC+b_inter, i_qp, DCT_CHROMA_AC, !b_inter, 1, 0 );
            else
                nz = h->quantf.quant_4x4( dct4x4[i], h->quant4_mf[CQM_4IC+b_inter][i_qp], h->quant4_bias[CQM_4IC+b_inter][i_qp] );
            h->mb.cache.non_zero_count[x264_scan8[16+i+ch*4]] = nz;
            if( nz )
            {
                nz_ac = 1;
                h->zigzagf.scan_4x4( h->dct.luma4x4[16+i+ch*4], dct4x4[i] );
                h->quantf.dequant_4x4( dct4x4[i], h->dequant4_mf[CQM_4IC + b_inter], i_qp );
                if( b_decimate )
                    i_decimate_score += h->quantf.decimate_score15( h->dct.luma4x4[16+i+ch*4] );
            }
        }

        if( h->mb.b_trellis )
            nz_dc = x264_quant_dc_trellis( h, dct2x2, CQM_4IC+b_inter, i_qp, DCT_CHROMA_DC, !b_inter, 1 );
        else
            nz_dc = h->quantf.quant_2x2_dc( dct2x2, h->quant4_mf[CQM_4IC+b_inter][i_qp][0]>>1, h->quant4_bias[CQM_4IC+b_inter][i_qp][0]<<1 );

        h->mb.cache.non_zero_count[x264_scan8[25]+ch] = nz_dc;

        if( (b_decimate && i_decimate_score < 7) || !nz_ac )
        {
            /* Decimate the block */
            h->mb.cache.non_zero_count[x264_scan8[16+0]+24*ch] = 0;
            h->mb.cache.non_zero_count[x264_scan8[16+1]+24*ch] = 0;
            h->mb.cache.non_zero_count[x264_scan8[16+2]+24*ch] = 0;
            h->mb.cache.non_zero_count[x264_scan8[16+3]+24*ch] = 0;
            if( !nz_dc ) /* Whole block is empty */
                continue;
            if( !x264_mb_optimize_chroma_dc( h, b_inter, i_qp, dct2x2 ) )
            {
                h->mb.cache.non_zero_count[x264_scan8[25]+ch] = 0;
                continue;
            }
            /* DC-only */
            zigzag_scan_2x2_dc( h->dct.chroma_dc[ch], dct2x2 );
            idct_dequant_2x2_dconly( dct2x2, dct2x2, h->dequant4_mf[CQM_4IC + b_inter], i_qp );
            h->dctf.add8x8_idct_dc( p_dst, dct2x2 );
        }
        else
        {
            h->mb.i_cbp_chroma = 1;
            if( nz_dc )
            {
                zigzag_scan_2x2_dc( h->dct.chroma_dc[ch], dct2x2 );
                idct_dequant_2x2_dc( dct2x2, dct4x4, h->dequant4_mf[CQM_4IC + b_inter], i_qp );
            }
            h->dctf.add8x8_idct( p_dst, dct4x4 );
        }
    }

    /* 0 = none, 1 = DC only, 2 = DC+AC */
    h->mb.i_cbp_chroma = ((!!M16( &h->mb.cache.non_zero_count[x264_scan8[25]] )) | h->mb.i_cbp_chroma) + h->mb.i_cbp_chroma;
}

static void x264_macroblock_encode_skip( x264_t *h )
{
    M32( &h->mb.cache.non_zero_count[x264_scan8[0]+0*8] ) = 0;
    M32( &h->mb.cache.non_zero_count[x264_scan8[0]+1*8] ) = 0;
    M32( &h->mb.cache.non_zero_count[x264_scan8[0]+2*8] ) = 0;
    M32( &h->mb.cache.non_zero_count[x264_scan8[0]+3*8] ) = 0;
    for( int i = 16; i < 24; i++ )
        h->mb.cache.non_zero_count[x264_scan8[i]] = 0;
    h->mb.i_cbp_luma = 0;
    h->mb.i_cbp_chroma = 0;
    h->mb.cbp[h->mb.i_mb_xy] = 0;
}

/*****************************************************************************
 * x264_macroblock_encode_pskip:
 *  Encode an already marked skip block
 *****************************************************************************/
static void x264_macroblock_encode_pskip( x264_t *h )
{
    /* don't do pskip motion compensation if it was already done in macroblock_analyse */
    if( !h->mb.b_skip_mc )
    {
        int mvx = x264_clip3( h->mb.cache.mv[0][x264_scan8[0]][0],
                              h->mb.mv_min[0], h->mb.mv_max[0] );
        int mvy = x264_clip3( h->mb.cache.mv[0][x264_scan8[0]][1],
                              h->mb.mv_min[1], h->mb.mv_max[1] );

        h->mc.mc_luma( h->mb.pic.p_fdec[0],    FDEC_STRIDE,
                       h->mb.pic.p_fref[0][0], h->mb.pic.i_stride[0],
                       mvx, mvy, 16, 16, &h->sh.weight[0][0] );

        /* Special case for mv0, which is (of course) very common in P-skip mode. */
        if( mvx | mvy )
            h->mc.mc_chroma( h->mb.pic.p_fdec[1], h->mb.pic.p_fdec[2], FDEC_STRIDE,
                             h->mb.pic.p_fref[0][0][4], h->mb.pic.i_stride[1],
                             mvx, mvy, 8, 8 );
        else
            h->mc.load_deinterleave_8x8x2_fdec( h->mb.pic.p_fdec[1], h->mb.pic.p_fref[0][0][4], h->mb.pic.i_stride[1] );

        if( h->sh.weight[0][1].weightfn )
            h->sh.weight[0][1].weightfn[8>>2]( h->mb.pic.p_fdec[1], FDEC_STRIDE,
                                               h->mb.pic.p_fdec[1], FDEC_STRIDE,
                                               &h->sh.weight[0][1], 8 );
        if( h->sh.weight[0][2].weightfn )
            h->sh.weight[0][2].weightfn[8>>2]( h->mb.pic.p_fdec[2], FDEC_STRIDE,
                                               h->mb.pic.p_fdec[2], FDEC_STRIDE,
                                               &h->sh.weight[0][2], 8 );
    }

    x264_macroblock_encode_skip( h );
}

/*****************************************************************************
 * Intra prediction for predictive lossless mode.
 *****************************************************************************/

void x264_predict_lossless_8x8_chroma( x264_t *h, int i_mode )
{
    if( i_mode == I_PRED_CHROMA_V )
    {
        h->mc.copy[PIXEL_8x8]( h->mb.pic.p_fdec[1], FDEC_STRIDE, h->mb.pic.p_fenc[1]-FENC_STRIDE, FENC_STRIDE, 8 );
        h->mc.copy[PIXEL_8x8]( h->mb.pic.p_fdec[2], FDEC_STRIDE, h->mb.pic.p_fenc[2]-FENC_STRIDE, FENC_STRIDE, 8 );
        memcpy( h->mb.pic.p_fdec[1], h->mb.pic.p_fdec[1]-FDEC_STRIDE, 8*sizeof(pixel) );
        memcpy( h->mb.pic.p_fdec[2], h->mb.pic.p_fdec[2]-FDEC_STRIDE, 8*sizeof(pixel) );
    }
    else if( i_mode == I_PRED_CHROMA_H )
    {
        h->mc.copy[PIXEL_8x8]( h->mb.pic.p_fdec[1], FDEC_STRIDE, h->mb.pic.p_fenc[1]-1, FENC_STRIDE, 8 );
        h->mc.copy[PIXEL_8x8]( h->mb.pic.p_fdec[2], FDEC_STRIDE, h->mb.pic.p_fenc[2]-1, FENC_STRIDE, 8 );
        x264_copy_column8( h->mb.pic.p_fdec[1]+4*FDEC_STRIDE, h->mb.pic.p_fdec[1]+4*FDEC_STRIDE-1 );
        x264_copy_column8( h->mb.pic.p_fdec[2]+4*FDEC_STRIDE, h->mb.pic.p_fdec[2]+4*FDEC_STRIDE-1 );
    }
    else
    {
        h->predict_8x8c[i_mode]( h->mb.pic.p_fdec[1] );
        h->predict_8x8c[i_mode]( h->mb.pic.p_fdec[2] );
    }
}

void x264_predict_lossless_4x4( x264_t *h, pixel *p_dst, int idx, int i_mode )
{
    int stride = h->fenc->i_stride[0] << h->mb.b_interlaced;
    pixel *p_src = h->mb.pic.p_fenc_plane[0] + block_idx_x[idx]*4 + block_idx_y[idx]*4 * stride;

    if( i_mode == I_PRED_4x4_V )
        h->mc.copy[PIXEL_4x4]( p_dst, FDEC_STRIDE, p_src-stride, stride, 4 );
    else if( i_mode == I_PRED_4x4_H )
        h->mc.copy[PIXEL_4x4]( p_dst, FDEC_STRIDE, p_src-1, stride, 4 );
    else
        h->predict_4x4[i_mode]( p_dst );
}

void x264_predict_lossless_8x8( x264_t *h, pixel *p_dst, int idx, int i_mode, pixel edge[33] )
{
    int stride = h->fenc->i_stride[0] << h->mb.b_interlaced;
    pixel *p_src = h->mb.pic.p_fenc_plane[0] + (idx&1)*8 + (idx>>1)*8*stride;

    if( i_mode == I_PRED_8x8_V )
        h->mc.copy[PIXEL_8x8]( p_dst, FDEC_STRIDE, p_src-stride, stride, 8 );
    else if( i_mode == I_PRED_8x8_H )
        h->mc.copy[PIXEL_8x8]( p_dst, FDEC_STRIDE, p_src-1, stride, 8 );
    else
        h->predict_8x8[i_mode]( p_dst, edge );
}

void x264_predict_lossless_16x16( x264_t *h, int i_mode )
{
    int stride = h->fenc->i_stride[0] << h->mb.b_interlaced;
    if( i_mode == I_PRED_16x16_V )
        h->mc.copy[PIXEL_16x16]( h->mb.pic.p_fdec[0], FDEC_STRIDE, h->mb.pic.p_fenc_plane[0]-stride, stride, 16 );
    else if( i_mode == I_PRED_16x16_H )
        h->mc.copy_16x16_unaligned( h->mb.pic.p_fdec[0], FDEC_STRIDE, h->mb.pic.p_fenc_plane[0]-1, stride, 16 );
    else
        h->predict_16x16[i_mode]( h->mb.pic.p_fdec[0] );
}

/*****************************************************************************
 * x264_macroblock_encode:
 *****************************************************************************/
void x264_macroblock_encode( x264_t *h )
{
    int i_qp = h->mb.i_qp;
    int b_decimate = h->mb.b_dct_decimate;
    int b_force_no_skip = 0;
    int nz;
    h->mb.i_cbp_luma = 0;
    h->mb.cache.non_zero_count[x264_scan8[24]] = 0;

    if( h->mb.i_type == I_PCM )
    {
        /* if PCM is chosen, we need to store reconstructed frame data */
        h->mc.copy[PIXEL_16x16]( h->mb.pic.p_fdec[0], FDEC_STRIDE, h->mb.pic.p_fenc[0], FENC_STRIDE, 16 );
        h->mc.copy[PIXEL_8x8]  ( h->mb.pic.p_fdec[1], FDEC_STRIDE, h->mb.pic.p_fenc[1], FENC_STRIDE, 8 );
        h->mc.copy[PIXEL_8x8]  ( h->mb.pic.p_fdec[2], FDEC_STRIDE, h->mb.pic.p_fenc[2], FENC_STRIDE, 8 );
        return;
    }

    if( h->sh.b_mbaff
        && h->mb.i_mb_xy == h->sh.i_first_mb + h->mb.i_mb_stride
        && IS_SKIP(h->mb.type[h->sh.i_first_mb]) )
    {
        /* The first skip is predicted to be a frame mb pair.
         * We don't yet support the aff part of mbaff, so force it to non-skip
         * so that we can pick the aff flag. */
        b_force_no_skip = 1;
        if( IS_SKIP(h->mb.i_type) )
        {
            if( h->mb.i_type == P_SKIP )
                h->mb.i_type = P_L0;
            else if( h->mb.i_type == B_SKIP )
                h->mb.i_type = B_DIRECT;
        }
    }

    if( h->mb.i_type == P_SKIP )
    {
        /* A bit special */
        x264_macroblock_encode_pskip( h );
        return;
    }
    if( h->mb.i_type == B_SKIP )
    {
        /* don't do bskip motion compensation if it was already done in macroblock_analyse */
        if( !h->mb.b_skip_mc )
            x264_mb_mc( h );
        x264_macroblock_encode_skip( h );
        return;
    }

    if( h->mb.i_type == I_16x16 )
    {
        const int i_mode = h->mb.i_intra16x16_pred_mode;
        h->mb.b_transform_8x8 = 0;

        if( h->mb.b_lossless )
            x264_predict_lossless_16x16( h, i_mode );
        else
            h->predict_16x16[i_mode]( h->mb.pic.p_fdec[0] );

        /* encode the 16x16 macroblock */
        x264_mb_encode_i16x16( h, i_qp );
    }
    else if( h->mb.i_type == I_8x8 )
    {
        ALIGNED_ARRAY_16( pixel, edge,[33] );
        h->mb.b_transform_8x8 = 1;
        /* If we already encoded 3 of the 4 i8x8 blocks, we don't have to do them again. */
        if( h->mb.i_skip_intra )
        {
            h->mc.copy[PIXEL_16x16]( h->mb.pic.p_fdec[0], FDEC_STRIDE, h->mb.pic.i8x8_fdec_buf, 16, 16 );
            M32( &h->mb.cache.non_zero_count[x264_scan8[ 0]] ) = h->mb.pic.i8x8_nnz_buf[0];
            M32( &h->mb.cache.non_zero_count[x264_scan8[ 2]] ) = h->mb.pic.i8x8_nnz_buf[1];
            M32( &h->mb.cache.non_zero_count[x264_scan8[ 8]] ) = h->mb.pic.i8x8_nnz_buf[2];
            M32( &h->mb.cache.non_zero_count[x264_scan8[10]] ) = h->mb.pic.i8x8_nnz_buf[3];
            h->mb.i_cbp_luma = h->mb.pic.i8x8_cbp;
            /* In RD mode, restore the now-overwritten DCT data. */
            if( h->mb.i_skip_intra == 2 )
                h->mc.memcpy_aligned( h->dct.luma8x8, h->mb.pic.i8x8_dct_buf, sizeof(h->mb.pic.i8x8_dct_buf) );
        }
        for( int i = h->mb.i_skip_intra ? 3 : 0 ; i < 4; i++ )
        {
            pixel *p_dst = &h->mb.pic.p_fdec[0][8 * (i&1) + 8 * (i>>1) * FDEC_STRIDE];
            int i_mode = h->mb.cache.intra4x4_pred_mode[x264_scan8[4*i]];
            h->predict_8x8_filter( p_dst, edge, h->mb.i_neighbour8[i], x264_pred_i4x4_neighbors[i_mode] );

            if( h->mb.b_lossless )
                x264_predict_lossless_8x8( h, p_dst, i, i_mode, edge );
            else
                h->predict_8x8[i_mode]( p_dst, edge );

            x264_mb_encode_i8x8( h, i, i_qp );
        }
    }
    else if( h->mb.i_type == I_4x4 )
    {
        h->mb.b_transform_8x8 = 0;
        /* If we already encoded 15 of the 16 i4x4 blocks, we don't have to do them again. */
        if( h->mb.i_skip_intra )
        {
            h->mc.copy[PIXEL_16x16]( h->mb.pic.p_fdec[0], FDEC_STRIDE, h->mb.pic.i4x4_fdec_buf, 16, 16 );
            M32( &h->mb.cache.non_zero_count[x264_scan8[ 0]] ) = h->mb.pic.i4x4_nnz_buf[0];
            M32( &h->mb.cache.non_zero_count[x264_scan8[ 2]] ) = h->mb.pic.i4x4_nnz_buf[1];
            M32( &h->mb.cache.non_zero_count[x264_scan8[ 8]] ) = h->mb.pic.i4x4_nnz_buf[2];
            M32( &h->mb.cache.non_zero_count[x264_scan8[10]] ) = h->mb.pic.i4x4_nnz_buf[3];
            h->mb.i_cbp_luma = h->mb.pic.i4x4_cbp;
            /* In RD mode, restore the now-overwritten DCT data. */
            if( h->mb.i_skip_intra == 2 )
                h->mc.memcpy_aligned( h->dct.luma4x4, h->mb.pic.i4x4_dct_buf, sizeof(h->mb.pic.i4x4_dct_buf) );
        }
        for( int i = h->mb.i_skip_intra ? 15 : 0 ; i < 16; i++ )
        {
            pixel *p_dst = &h->mb.pic.p_fdec[0][block_idx_xy_fdec[i]];
            int i_mode = h->mb.cache.intra4x4_pred_mode[x264_scan8[i]];

            if( (h->mb.i_neighbour4[i] & (MB_TOPRIGHT|MB_TOP)) == MB_TOP )
                /* emulate missing topright samples */
                MPIXEL_X4( &p_dst[4-FDEC_STRIDE] ) = PIXEL_SPLAT_X4( p_dst[3-FDEC_STRIDE] );

            if( h->mb.b_lossless )
                x264_predict_lossless_4x4( h, p_dst, i, i_mode );
            else
                h->predict_4x4[i_mode]( p_dst );
            x264_mb_encode_i4x4( h, i, i_qp );
        }
    }
    else    /* Inter MB */
    {
        int i_decimate_mb = 0;

        /* Don't repeat motion compensation if it was already done in non-RD transform analysis */
        if( !h->mb.b_skip_mc )
            x264_mb_mc( h );

        if( h->mb.b_lossless )
        {
            if( h->mb.b_transform_8x8 )
                for( int i8x8 = 0; i8x8 < 4; i8x8++ )
                {
                    int x = i8x8&1;
                    int y = i8x8>>1;
                    int s8 = X264_SCAN8_0 + 2*x + 16*y;

                    nz = h->zigzagf.sub_8x8( h->dct.luma8x8[i8x8], h->mb.pic.p_fenc[0] + 8*x + 8*y*FENC_STRIDE,
                                                                   h->mb.pic.p_fdec[0] + 8*x + 8*y*FDEC_STRIDE );
                    STORE_8x8_NNZ( s8, nz );
                    h->mb.i_cbp_luma |= nz << i8x8;
                }
            else
                for( int i4x4 = 0; i4x4 < 16; i4x4++ )
                {
                    nz = h->zigzagf.sub_4x4( h->dct.luma4x4[i4x4],
                                        h->mb.pic.p_fenc[0]+block_idx_xy_fenc[i4x4],
                                        h->mb.pic.p_fdec[0]+block_idx_xy_fdec[i4x4] );
                    h->mb.cache.non_zero_count[x264_scan8[i4x4]] = nz;
                    h->mb.i_cbp_luma |= nz << (i4x4>>2);
                }
        }
        else if( h->mb.b_transform_8x8 )
        {
            ALIGNED_ARRAY_16( dctcoef, dct8x8,[4],[64] );
            b_decimate &= !h->mb.b_trellis || !h->param.b_cabac; // 8x8 trellis is inherently optimal decimation for CABAC
            h->dctf.sub16x16_dct8( dct8x8, h->mb.pic.p_fenc[0], h->mb.pic.p_fdec[0] );
            h->nr_count[1] += h->mb.b_noise_reduction * 4;

            for( int idx = 0; idx < 4; idx++ )
            {
                if( h->mb.b_noise_reduction )
                    h->quantf.denoise_dct( dct8x8[idx], h->nr_residual_sum[1], h->nr_offset[1], 64 );
                nz = x264_quant_8x8( h, dct8x8[idx], i_qp, 0, idx );

                if( nz )
                {
                    h->zigzagf.scan_8x8( h->dct.luma8x8[idx], dct8x8[idx] );
                    if( b_decimate )
                    {
                        int i_decimate_8x8 = h->quantf.decimate_score64( h->dct.luma8x8[idx] );
                        i_decimate_mb += i_decimate_8x8;
                        if( i_decimate_8x8 >= 4 )
                            h->mb.i_cbp_luma |= 1<<idx;
                    }
                    else
                        h->mb.i_cbp_luma |= 1<<idx;
                }
            }

            if( i_decimate_mb < 6 && b_decimate )
            {
                h->mb.i_cbp_luma = 0;
                CLEAR_16x16_NNZ
            }
            else
            {
                for( int idx = 0; idx < 4; idx++ )
                {
                    int x = idx&1;
                    int y = idx>>1;
                    int s8 = X264_SCAN8_0 + 2*x + 16*y;

                    if( h->mb.i_cbp_luma&(1<<idx) )
                    {
                        h->quantf.dequant_8x8( dct8x8[idx], h->dequant8_mf[CQM_8PY], i_qp );
                        h->dctf.add8x8_idct8( &h->mb.pic.p_fdec[0][8*x + 8*y*FDEC_STRIDE], dct8x8[idx] );
                        STORE_8x8_NNZ( s8, 1 );
                    }
                    else
                        STORE_8x8_NNZ( s8, 0 );
                }
            }
        }
        else
        {
            ALIGNED_ARRAY_16( dctcoef, dct4x4,[16],[16] );
            h->dctf.sub16x16_dct( dct4x4, h->mb.pic.p_fenc[0], h->mb.pic.p_fdec[0] );
            h->nr_count[0] += h->mb.b_noise_reduction * 16;

            for( int i8x8 = 0; i8x8 < 4; i8x8++ )
            {
                int i_decimate_8x8 = 0;
                int cbp = 0;

                /* encode one 4x4 block */
                for( int i4x4 = 0; i4x4 < 4; i4x4++ )
                {
                    int idx = i8x8 * 4 + i4x4;

                    if( h->mb.b_noise_reduction )
                        h->quantf.denoise_dct( dct4x4[idx], h->nr_residual_sum[0], h->nr_offset[0], 16 );
                    nz = x264_quant_4x4( h, dct4x4[idx], i_qp, DCT_LUMA_4x4, 0, idx );
                    h->mb.cache.non_zero_count[x264_scan8[idx]] = nz;

                    if( nz )
                    {
                        h->zigzagf.scan_4x4( h->dct.luma4x4[idx], dct4x4[idx] );
                        h->quantf.dequant_4x4( dct4x4[idx], h->dequant4_mf[CQM_4PY], i_qp );
                        if( b_decimate && i_decimate_8x8 < 6 )
                            i_decimate_8x8 += h->quantf.decimate_score16( h->dct.luma4x4[idx] );
                        cbp = 1;
                    }
                }

                int x = i8x8&1;
                int y = i8x8>>1;

                /* decimate this 8x8 block */
                i_decimate_mb += i_decimate_8x8;
                if( b_decimate )
                {
                    if( i_decimate_8x8 < 4 )
                    {
                        int s8 = X264_SCAN8_0 + 2*x + 16*y;
                        STORE_8x8_NNZ( s8, 0 );
                    }
                    else
                        h->mb.i_cbp_luma |= 1<<i8x8;
                }
                else if( cbp )
                {
                    h->dctf.add8x8_idct( &h->mb.pic.p_fdec[0][8*x + 8*y*FDEC_STRIDE], &dct4x4[i8x8*4] );
                    h->mb.i_cbp_luma |= 1<<i8x8;
                }
            }

            if( b_decimate )
            {
                if( i_decimate_mb < 6 )
                {
                    h->mb.i_cbp_luma = 0;
                    CLEAR_16x16_NNZ
                }
                else
                {
                    for( int i8x8 = 0; i8x8 < 4; i8x8++ )
                        if( h->mb.i_cbp_luma&(1<<i8x8) )
                            h->dctf.add8x8_idct( &h->mb.pic.p_fdec[0][(i8x8&1)*8 + (i8x8>>1)*8*FDEC_STRIDE], &dct4x4[i8x8*4] );
                }
            }
        }
    }

    /* encode chroma */
    if( IS_INTRA( h->mb.i_type ) )
    {
        const int i_mode = h->mb.i_chroma_pred_mode;
        if( h->mb.b_lossless )
            x264_predict_lossless_8x8_chroma( h, i_mode );
        else
        {
            h->predict_8x8c[i_mode]( h->mb.pic.p_fdec[1] );
            h->predict_8x8c[i_mode]( h->mb.pic.p_fdec[2] );
        }
    }

    /* encode the 8x8 blocks */
    x264_mb_encode_8x8_chroma( h, !IS_INTRA( h->mb.i_type ), h->mb.i_chroma_qp );

    /* store cbp */
    int cbp = h->mb.i_cbp_chroma << 4 | h->mb.i_cbp_luma;
    if( h->param.b_cabac )
        cbp |= h->mb.cache.non_zero_count[x264_scan8[24]] << 8
            |  h->mb.cache.non_zero_count[x264_scan8[25]] << 9
            |  h->mb.cache.non_zero_count[x264_scan8[26]] << 10;
    h->mb.cbp[h->mb.i_mb_xy] = cbp;

    /* Check for P_SKIP
     * XXX: in the me perhaps we should take x264_mb_predict_mv_pskip into account
     *      (if multiple mv give same result)*/
    if( !b_force_no_skip )
    {
        if( h->mb.i_type == P_L0 && h->mb.i_partition == D_16x16 &&
            !(h->mb.i_cbp_luma | h->mb.i_cbp_chroma) &&
            M32( h->mb.cache.mv[0][x264_scan8[0]] ) == M32( h->mb.cache.pskip_mv )
            && h->mb.cache.ref[0][x264_scan8[0]] == 0 )
        {
            h->mb.i_type = P_SKIP;
        }

        /* Check for B_SKIP */
        if( h->mb.i_type == B_DIRECT && !(h->mb.i_cbp_luma | h->mb.i_cbp_chroma) )
        {
            h->mb.i_type = B_SKIP;
        }
    }
}

/*****************************************************************************
 * x264_macroblock_probe_skip:
 *  Check if the current MB could be encoded as a [PB]_SKIP
 *****************************************************************************/
int x264_macroblock_probe_skip( x264_t *h, int b_bidir )
{
    ALIGNED_ARRAY_16( dctcoef, dct4x4,[4],[16] );
    ALIGNED_ARRAY_16( dctcoef, dct2x2,[4] );
    ALIGNED_ARRAY_16( dctcoef, dctscan,[16] );
    ALIGNED_4( int16_t mvp[2] );

    int i_qp = h->mb.i_qp;
    int thresh, ssd;

    if( !b_bidir )
    {
        /* Get the MV */
        mvp[0] = x264_clip3( h->mb.cache.pskip_mv[0], h->mb.mv_min[0], h->mb.mv_max[0] );
        mvp[1] = x264_clip3( h->mb.cache.pskip_mv[1], h->mb.mv_min[1], h->mb.mv_max[1] );

        /* Motion compensation */
        h->mc.mc_luma( h->mb.pic.p_fdec[0],    FDEC_STRIDE,
                       h->mb.pic.p_fref[0][0], h->mb.pic.i_stride[0],
                       mvp[0], mvp[1], 16, 16, &h->sh.weight[0][0] );
    }

    for( int i8x8 = 0, i_decimate_mb = 0; i8x8 < 4; i8x8++ )
    {
        int fenc_offset = (i8x8&1) * 8 + (i8x8>>1) * FENC_STRIDE * 8;
        int fdec_offset = (i8x8&1) * 8 + (i8x8>>1) * FDEC_STRIDE * 8;
        /* get luma diff */
        h->dctf.sub8x8_dct( dct4x4, h->mb.pic.p_fenc[0] + fenc_offset,
                                    h->mb.pic.p_fdec[0] + fdec_offset );
        /* encode one 4x4 block */
        for( int i4x4 = 0; i4x4 < 4; i4x4++ )
        {
            if( !h->quantf.quant_4x4( dct4x4[i4x4], h->quant4_mf[CQM_4PY][i_qp], h->quant4_bias[CQM_4PY][i_qp] ) )
                continue;
            h->zigzagf.scan_4x4( dctscan, dct4x4[i4x4] );
            i_decimate_mb += h->quantf.decimate_score16( dctscan );
            if( i_decimate_mb >= 6 )
                return 0;
        }
    }

    /* encode chroma */
    i_qp = h->mb.i_chroma_qp;
    thresh = (x264_lambda2_tab[i_qp] + 32) >> 6;

    if( !b_bidir )
    {
        /* Special case for mv0, which is (of course) very common in P-skip mode. */
        if( M32( mvp ) )
            h->mc.mc_chroma( h->mb.pic.p_fdec[1], h->mb.pic.p_fdec[2], FDEC_STRIDE,
                             h->mb.pic.p_fref[0][0][4], h->mb.pic.i_stride[1],
                             mvp[0], mvp[1], 8, 8 );
        else
            h->mc.load_deinterleave_8x8x2_fdec( h->mb.pic.p_fdec[1], h->mb.pic.p_fref[0][0][4], h->mb.pic.i_stride[1] );
    }

    for( int ch = 0; ch < 2; ch++ )
    {
        pixel *p_src = h->mb.pic.p_fenc[1+ch];
        pixel *p_dst = h->mb.pic.p_fdec[1+ch];

        if( !b_bidir && h->sh.weight[0][1+ch].weightfn )
            h->sh.weight[0][1+ch].weightfn[8>>2]( h->mb.pic.p_fdec[1+ch], FDEC_STRIDE,
                                                  h->mb.pic.p_fdec[1+ch], FDEC_STRIDE,
                                                  &h->sh.weight[0][1+ch], 8 );

        /* there is almost never a termination during chroma, but we can't avoid the check entirely */
        /* so instead we check SSD and skip the actual check if the score is low enough. */
        ssd = h->pixf.ssd[PIXEL_8x8]( p_dst, FDEC_STRIDE, p_src, FENC_STRIDE );
        if( ssd < thresh )
            continue;

        /* The vast majority of chroma checks will terminate during the DC check or the higher
         * threshold check, so we can save time by doing a DC-only DCT. */
        h->dctf.sub8x8_dct_dc( dct2x2, p_src, p_dst );

        if( h->quantf.quant_2x2_dc( dct2x2, h->quant4_mf[CQM_4PC][i_qp][0]>>1, h->quant4_bias[CQM_4PC][i_qp][0]<<1 ) )
            return 0;

        /* If there wasn't a termination in DC, we can check against a much higher threshold. */
        if( ssd < thresh*4 )
            continue;

        h->dctf.sub8x8_dct( dct4x4, p_src, p_dst );

        /* calculate dct coeffs */
        for( int i4x4 = 0, i_decimate_mb = 0; i4x4 < 4; i4x4++ )
        {
            dct4x4[i4x4][0] = 0;
            if( !h->quantf.quant_4x4( dct4x4[i4x4], h->quant4_mf[CQM_4PC][i_qp], h->quant4_bias[CQM_4PC][i_qp] ) )
                continue;
            h->zigzagf.scan_4x4( dctscan, dct4x4[i4x4] );
            i_decimate_mb += h->quantf.decimate_score15( dctscan );
            if( i_decimate_mb >= 7 )
                return 0;
        }
    }

    h->mb.b_skip_mc = 1;
    return 1;
}

/****************************************************************************
 * DCT-domain noise reduction / adaptive deadzone
 * from libavcodec
 ****************************************************************************/

void x264_noise_reduction_update( x264_t *h )
{
    for( int cat = 0; cat < 2; cat++ )
    {
        int size = cat ? 64 : 16;
        const uint16_t *weight = cat ? x264_dct8_weight2_tab : x264_dct4_weight2_tab;

        if( h->nr_count[cat] > (cat ? (1<<16) : (1<<18)) )
        {
            for( int i = 0; i < size; i++ )
                h->nr_residual_sum[cat][i] >>= 1;
            h->nr_count[cat] >>= 1;
        }

        for( int i = 0; i < size; i++ )
            h->nr_offset[cat][i] =
                ((uint64_t)h->param.analyse.i_noise_reduction * h->nr_count[cat]
                 + h->nr_residual_sum[cat][i]/2)
              / ((uint64_t)h->nr_residual_sum[cat][i] * weight[i]/256 + 1);
    }
}

/*****************************************************************************
 * RD only; 4 calls to this do not make up for one macroblock_encode.
 * doesn't transform chroma dc.
 *****************************************************************************/
void x264_macroblock_encode_p8x8( x264_t *h, int i8 )
{
    int i_qp = h->mb.i_qp;
    int x = i8&1;
    int y = i8>>1;
    int s8 = X264_SCAN8_0 + 2*x + 16*y;
    pixel *p_fenc = h->mb.pic.p_fenc[0] + 8*x + 8*y*FENC_STRIDE;
    pixel *p_fdec = h->mb.pic.p_fdec[0] + 8*x + 8*y*FDEC_STRIDE;
    int b_decimate = h->mb.b_dct_decimate;
    int nnz8x8 = 0;
    int nz;

    if( !h->mb.b_skip_mc )
        x264_mb_mc_8x8( h, i8 );

    if( h->mb.b_lossless )
    {
        if( h->mb.b_transform_8x8 )
        {
            nnz8x8 = h->zigzagf.sub_8x8( h->dct.luma8x8[i8], p_fenc, p_fdec );
            STORE_8x8_NNZ( s8, nnz8x8 );
        }
        else
        {
            for( int i4 = i8*4; i4 < i8*4+4; i4++ )
            {
                nz = h->zigzagf.sub_4x4( h->dct.luma4x4[i4],
                                    h->mb.pic.p_fenc[0]+block_idx_xy_fenc[i4],
                                    h->mb.pic.p_fdec[0]+block_idx_xy_fdec[i4] );
                h->mb.cache.non_zero_count[x264_scan8[i4]] = nz;
                nnz8x8 |= nz;
            }
        }
        for( int ch = 0; ch < 2; ch++ )
        {
            dctcoef dc;
            p_fenc = h->mb.pic.p_fenc[1+ch] + 4*x + 4*y*FENC_STRIDE;
            p_fdec = h->mb.pic.p_fdec[1+ch] + 4*x + 4*y*FDEC_STRIDE;
            nz = h->zigzagf.sub_4x4ac( h->dct.luma4x4[16+i8+ch*4], p_fenc, p_fdec, &dc );
            h->mb.cache.non_zero_count[x264_scan8[16+i8+ch*4]] = nz;
        }
    }
    else
    {
        if( h->mb.b_transform_8x8 )
        {
            ALIGNED_ARRAY_16( dctcoef, dct8x8,[64] );
            h->dctf.sub8x8_dct8( dct8x8, p_fenc, p_fdec );
            nnz8x8 = x264_quant_8x8( h, dct8x8, i_qp, 0, i8 );
            if( nnz8x8 )
            {
                h->zigzagf.scan_8x8( h->dct.luma8x8[i8], dct8x8 );

                if( b_decimate && !h->mb.b_trellis )
                    nnz8x8 = 4 <= h->quantf.decimate_score64( h->dct.luma8x8[i8] );

                if( nnz8x8 )
                {
                    h->quantf.dequant_8x8( dct8x8, h->dequant8_mf[CQM_8PY], i_qp );
                    h->dctf.add8x8_idct8( p_fdec, dct8x8 );
                    STORE_8x8_NNZ( s8, 1 );
                }
                else
                    STORE_8x8_NNZ( s8, 0 );
            }
            else
                STORE_8x8_NNZ( s8, 0 );
        }
        else
        {
            int i_decimate_8x8 = 0;
            ALIGNED_ARRAY_16( dctcoef, dct4x4,[4],[16] );
            h->dctf.sub8x8_dct( dct4x4, p_fenc, p_fdec );
            for( int i4 = 0; i4 < 4; i4++ )
            {
                nz = x264_quant_4x4( h, dct4x4[i4], i_qp, DCT_LUMA_4x4, 0, i8*4+i4 );
                h->mb.cache.non_zero_count[x264_scan8[i8*4+i4]] = nz;
                if( nz )
                {
                    h->zigzagf.scan_4x4( h->dct.luma4x4[i8*4+i4], dct4x4[i4] );
                    h->quantf.dequant_4x4( dct4x4[i4], h->dequant4_mf[CQM_4PY], i_qp );
                    if( b_decimate )
                        i_decimate_8x8 += h->quantf.decimate_score16( h->dct.luma4x4[i8*4+i4] );
                    nnz8x8 = 1;
                }
            }

            if( b_decimate && i_decimate_8x8 < 4 )
                nnz8x8 = 0;

            if( nnz8x8 )
                h->dctf.add8x8_idct( p_fdec, dct4x4 );
            else
                STORE_8x8_NNZ( s8, 0 );
        }

        i_qp = h->mb.i_chroma_qp;

        for( int ch = 0; ch < 2; ch++ )
        {
            ALIGNED_ARRAY_16( dctcoef, dct4x4,[16] );
            p_fenc = h->mb.pic.p_fenc[1+ch] + 4*x + 4*y*FENC_STRIDE;
            p_fdec = h->mb.pic.p_fdec[1+ch] + 4*x + 4*y*FDEC_STRIDE;

            h->dctf.sub4x4_dct( dct4x4, p_fenc, p_fdec );
            dct4x4[0] = 0;

            if( h->mb.b_trellis )
                nz = x264_quant_4x4_trellis( h, dct4x4, CQM_4PC, i_qp, DCT_CHROMA_AC, 0, 1, 0 );
            else
                nz = h->quantf.quant_4x4( dct4x4, h->quant4_mf[CQM_4PC][i_qp], h->quant4_bias[CQM_4PC][i_qp] );

            h->mb.cache.non_zero_count[x264_scan8[16+i8+ch*4]] = nz;
            if( nz )
            {
                h->zigzagf.scan_4x4( h->dct.luma4x4[16+i8+ch*4], dct4x4 );
                h->quantf.dequant_4x4( dct4x4, h->dequant4_mf[CQM_4PC], i_qp );
                h->dctf.add4x4_idct( p_fdec, dct4x4 );
            }
        }
    }
    h->mb.i_cbp_luma &= ~(1 << i8);
    h->mb.i_cbp_luma |= nnz8x8 << i8;
    h->mb.i_cbp_chroma = 0x02;
}

/*****************************************************************************
 * RD only, luma only
 *****************************************************************************/
void x264_macroblock_encode_p4x4( x264_t *h, int i4 )
{
    int i_qp = h->mb.i_qp;
    pixel *p_fenc = &h->mb.pic.p_fenc[0][block_idx_xy_fenc[i4]];
    pixel *p_fdec = &h->mb.pic.p_fdec[0][block_idx_xy_fdec[i4]];
    int nz;

    /* Don't need motion compensation as this function is only used in qpel-RD, which caches pixel data. */

    if( h->mb.b_lossless )
    {
        nz = h->zigzagf.sub_4x4( h->dct.luma4x4[i4], p_fenc, p_fdec );
        h->mb.cache.non_zero_count[x264_scan8[i4]] = nz;
    }
    else
    {
        ALIGNED_ARRAY_16( dctcoef, dct4x4,[16] );
        h->dctf.sub4x4_dct( dct4x4, p_fenc, p_fdec );
        nz = x264_quant_4x4( h, dct4x4, i_qp, DCT_LUMA_4x4, 0, i4 );
        h->mb.cache.non_zero_count[x264_scan8[i4]] = nz;
        if( nz )
        {
            h->zigzagf.scan_4x4( h->dct.luma4x4[i4], dct4x4 );
            h->quantf.dequant_4x4( dct4x4, h->dequant4_mf[CQM_4PY], i_qp );
            h->dctf.add4x4_idct( p_fdec, dct4x4 );
        }
    }
}

/*****************************************************************************
 * rdo.c: rate-distortion optimization
 *****************************************************************************
 * Copyright (C) 2005-2011 x264 project
 *
 * Authors: Loren Merritt <lorenm@u.washington.edu>
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

/* duplicate all the writer functions, just calculating bit cost
 * instead of writing the bitstream.
 * TODO: use these for fast 1st pass too. */

#define RDO_SKIP_BS 1

/* Transition and size tables for abs<9 MVD and residual coding */
/* Consist of i_prefix-2 1s, one zero, and a bypass sign bit */
static uint8_t cabac_transition_unary[15][128];
static uint16_t cabac_size_unary[15][128];
/* Transition and size tables for abs>9 MVD */
/* Consist of 5 1s and a bypass sign bit */
static uint8_t cabac_transition_5ones[128];
static uint16_t cabac_size_5ones[128];

/* CAVLC: produces exactly the same bit count as a normal encode */
/* this probably still leaves some unnecessary computations */
#define bs_write1(s,v)     ((s)->i_bits_encoded += 1)
#define bs_write(s,n,v)    ((s)->i_bits_encoded += (n))
#define bs_write_ue(s,v)   ((s)->i_bits_encoded += bs_size_ue(v))
#define bs_write_se(s,v)   ((s)->i_bits_encoded += bs_size_se(v))
#define bs_write_te(s,v,l) ((s)->i_bits_encoded += bs_size_te(v,l))
#define x264_macroblock_write_cavlc  static x264_macroblock_size_cavlc
#include "cavlc.c"

/* CABAC: not exactly the same. x264_cabac_size_decision() keeps track of
 * fractional bits, but only finite precision. */
#undef  x264_cabac_encode_decision
#undef  x264_cabac_encode_decision_noup
#undef  x264_cabac_encode_bypass
#undef  x264_cabac_encode_terminal
#define x264_cabac_encode_decision(c,x,v) x264_cabac_size_decision(c,x,v)
#define x264_cabac_encode_decision_noup(c,x,v) x264_cabac_size_decision_noup(c,x,v)
#define x264_cabac_encode_terminal(c)     ((c)->f8_bits_encoded += 7)
#define x264_cabac_encode_bypass(c,v)     ((c)->f8_bits_encoded += 256)
#define x264_cabac_encode_ue_bypass(c,e,v) ((c)->f8_bits_encoded += (bs_size_ue_big(v+(1<<e)-1)-e)<<8)
#define x264_macroblock_write_cabac  static x264_macroblock_size_cabac
#include "cabac.c"

#define COPY_CABAC h->mc.memcpy_aligned( &cabac_tmp.f8_bits_encoded, &h->cabac.f8_bits_encoded, \
        sizeof(x264_cabac_t) - offsetof(x264_cabac_t,f8_bits_encoded) - (CHROMA444 ? 0 : (1024+12)-460) )
#define COPY_CABAC_PART( pos, size )\
        memcpy( &cb->state[pos], &h->cabac.state[pos], size )

static ALWAYS_INLINE uint64_t cached_hadamard( x264_t *h, int size, int x, int y )
{
    static const uint8_t hadamard_shift_x[4] = {4,   4,   3,   3};
    static const uint8_t hadamard_shift_y[4] = {4-0, 3-0, 4-1, 3-1};
    static const uint8_t  hadamard_offset[4] = {0,   1,   3,   5};
    int cache_index = (x >> hadamard_shift_x[size]) + (y >> hadamard_shift_y[size])
                    + hadamard_offset[size];
    uint64_t res = h->mb.pic.fenc_hadamard_cache[cache_index];
    if( res )
        return res - 1;
    else
    {
        pixel *fenc = h->mb.pic.p_fenc[0] + x + y*FENC_STRIDE;
        res = h->pixf.hadamard_ac[size]( fenc, FENC_STRIDE );
        h->mb.pic.fenc_hadamard_cache[cache_index] = res + 1;
        return res;
    }
}

static ALWAYS_INLINE int cached_satd( x264_t *h, int size, int x, int y )
{
    static const uint8_t satd_shift_x[3] = {3,   2,   2};
    static const uint8_t satd_shift_y[3] = {2-1, 3-2, 2-2};
    static const uint8_t  satd_offset[3] = {0,   8,   16};
    ALIGNED_16( static pixel zero[16] ) = {0};
    int cache_index = (x >> satd_shift_x[size - PIXEL_8x4]) + (y >> satd_shift_y[size - PIXEL_8x4])
                    + satd_offset[size - PIXEL_8x4];
    int res = h->mb.pic.fenc_satd_cache[cache_index];
    if( res )
        return res - 1;
    else
    {
        pixel *fenc = h->mb.pic.p_fenc[0] + x + y*FENC_STRIDE;
        int dc = h->pixf.sad[size]( fenc, FENC_STRIDE, zero, 0 ) >> 1;
        res = h->pixf.satd[size]( fenc, FENC_STRIDE, zero, 0 ) - dc;
        h->mb.pic.fenc_satd_cache[cache_index] = res + 1;
        return res;
    }
}

/* Psy RD distortion metric: SSD plus "Absolute Difference of Complexities" */
/* SATD and SA8D are used to measure block complexity. */
/* The difference between SATD and SA8D scores are both used to avoid bias from the DCT size.  Using SATD */
/* only, for example, results in overusage of 8x8dct, while the opposite occurs when using SA8D. */

/* FIXME:  Is there a better metric than averaged SATD/SA8D difference for complexity difference? */
/* Hadamard transform is recursive, so a SATD+SA8D can be done faster by taking advantage of this fact. */
/* This optimization can also be used in non-RD transform decision. */

static inline int ssd_plane( x264_t *h, int size, int p, int x, int y )
{
    ALIGNED_16( static pixel zero[16] ) = {0};
    int satd = 0;
    pixel *fdec = h->mb.pic.p_fdec[p] + x + y*FDEC_STRIDE;
    pixel *fenc = h->mb.pic.p_fenc[p] + x + y*FENC_STRIDE;
    if( p == 0 && h->mb.i_psy_rd )
    {
        /* If the plane is smaller than 8x8, we can't do an SA8D; this probably isn't a big problem. */
        if( size <= PIXEL_8x8 )
        {
            uint64_t fdec_acs = h->pixf.hadamard_ac[size]( fdec, FDEC_STRIDE );
            uint64_t fenc_acs = cached_hadamard( h, size, x, y );
            satd = abs((int32_t)fdec_acs - (int32_t)fenc_acs)
                 + abs((int32_t)(fdec_acs>>32) - (int32_t)(fenc_acs>>32));
            satd >>= 1;
        }
        else
        {
            int dc = h->pixf.sad[size]( fdec, FDEC_STRIDE, zero, 0 ) >> 1;
            satd = abs(h->pixf.satd[size]( fdec, FDEC_STRIDE, zero, 0 ) - dc - cached_satd( h, size, x, y ));
        }
        satd = (satd * h->mb.i_psy_rd * h->mb.i_psy_rd_lambda + 128) >> 8;
    }
    return h->pixf.ssd[size](fenc, FENC_STRIDE, fdec, FDEC_STRIDE) + satd;
}

static inline int ssd_mb( x264_t *h )
{
    int chroma_size = CHROMA444 ? PIXEL_16x16 : PIXEL_8x8;
    int chroma_ssd = ssd_plane(h, chroma_size, 1, 0, 0) + ssd_plane(h, chroma_size, 2, 0, 0);
    chroma_ssd = ((uint64_t)chroma_ssd * h->mb.i_chroma_lambda2_offset + 128) >> 8;
    return ssd_plane(h, PIXEL_16x16, 0, 0, 0) + chroma_ssd;
}

static int x264_rd_cost_mb( x264_t *h, int i_lambda2 )
{
    int b_transform_bak = h->mb.b_transform_8x8;
    int i_ssd;
    int i_bits;
    int type_bak = h->mb.i_type;

    x264_macroblock_encode( h );

    if( h->mb.b_deblock_rdo )
        x264_macroblock_deblock( h );

    i_ssd = ssd_mb( h );

    if( IS_SKIP( h->mb.i_type ) )
    {
        i_bits = (1 * i_lambda2 + 128) >> 8;
    }
    else if( h->param.b_cabac )
    {
        x264_cabac_t cabac_tmp;
        COPY_CABAC;
        x264_macroblock_size_cabac( h, &cabac_tmp );
        i_bits = ( (uint64_t)cabac_tmp.f8_bits_encoded * i_lambda2 + 32768 ) >> 16;
    }
    else
    {
        x264_macroblock_size_cavlc( h );
        i_bits = ( h->out.bs.i_bits_encoded * i_lambda2 + 128 ) >> 8;
    }

    h->mb.b_transform_8x8 = b_transform_bak;
    h->mb.i_type = type_bak;

    return i_ssd + i_bits;
}

/* partition RD functions use 8 bits more precision to avoid large rounding errors at low QPs */

static uint64_t x264_rd_cost_subpart( x264_t *h, int i_lambda2, int i4, int i_pixel )
{
    uint64_t i_ssd, i_bits;

    x264_macroblock_encode_p4x4( h, i4 );
    if( i_pixel == PIXEL_8x4 )
        x264_macroblock_encode_p4x4( h, i4+1 );
    if( i_pixel == PIXEL_4x8 )
        x264_macroblock_encode_p4x4( h, i4+2 );

    i_ssd = ssd_plane( h, i_pixel, 0, block_idx_x[i4]*4, block_idx_y[i4]*4 );
    if( CHROMA444 )
    {
        int chromassd = ssd_plane( h, i_pixel, 1, block_idx_x[i4]*4, block_idx_y[i4]*4 )
                      + ssd_plane( h, i_pixel, 2, block_idx_x[i4]*4, block_idx_y[i4]*4 );
        chromassd = ((uint64_t)chromassd * h->mb.i_chroma_lambda2_offset + 128) >> 8;
        i_ssd += chromassd;
    }

    if( h->param.b_cabac )
    {
        x264_cabac_t cabac_tmp;
        COPY_CABAC;
        x264_subpartition_size_cabac( h, &cabac_tmp, i4, i_pixel );
        i_bits = ( (uint64_t)cabac_tmp.f8_bits_encoded * i_lambda2 + 128 ) >> 8;
    }
    else
        i_bits = x264_subpartition_size_cavlc( h, i4, i_pixel );

    return (i_ssd<<8) + i_bits;
}

uint64_t x264_rd_cost_part( x264_t *h, int i_lambda2, int i4, int i_pixel )
{
    uint64_t i_ssd, i_bits;
    int i8 = i4 >> 2;
    int chromassd;

    if( i_pixel == PIXEL_16x16 )
    {
        int i_cost = x264_rd_cost_mb( h, i_lambda2 );
        return i_cost;
    }

    if( i_pixel > PIXEL_8x8 )
        return x264_rd_cost_subpart( h, i_lambda2, i4, i_pixel );

    h->mb.i_cbp_luma = 0;

    x264_macroblock_encode_p8x8( h, i8 );
    if( i_pixel == PIXEL_16x8 )
        x264_macroblock_encode_p8x8( h, i8+1 );
    if( i_pixel == PIXEL_8x16 )
        x264_macroblock_encode_p8x8( h, i8+2 );

    i_ssd = ssd_plane( h, i_pixel, 0, (i8&1)*8, (i8>>1)*8 );
    if( CHROMA444 )
    {
        chromassd = ssd_plane( h, i_pixel, 1, (i8&1)*8, (i8>>1)*8 )
                  + ssd_plane( h, i_pixel, 2, (i8&1)*8, (i8>>1)*8 );
    }
    else
    {
        chromassd = ssd_plane( h, i_pixel+3, 1, (i8&1)*4, (i8>>1)*4 )
                  + ssd_plane( h, i_pixel+3, 2, (i8&1)*4, (i8>>1)*4 );
    }
    chromassd = ((uint64_t)chromassd * h->mb.i_chroma_lambda2_offset + 128) >> 8;
    i_ssd += chromassd;

    if( h->param.b_cabac )
    {
        x264_cabac_t cabac_tmp;
        COPY_CABAC;
        x264_partition_size_cabac( h, &cabac_tmp, i8, i_pixel );
        i_bits = ( (uint64_t)cabac_tmp.f8_bits_encoded * i_lambda2 + 128 ) >> 8;
    }
    else
        i_bits = x264_partition_size_cavlc( h, i8, i_pixel ) * i_lambda2;

    return (i_ssd<<8) + i_bits;
}

static uint64_t x264_rd_cost_i8x8( x264_t *h, int i_lambda2, int i8, int i_mode, pixel edge[3][48] )
{
    uint64_t i_ssd, i_bits;
    int plane_count = CHROMA444 ? 3 : 1;
    int i_qp = h->mb.i_qp;
    h->mb.i_cbp_luma &= ~(1<<i8);
    h->mb.b_transform_8x8 = 1;

    for( int p = 0; p < plane_count; p++ )
    {
        x264_mb_encode_i8x8( h, p, i8, i_qp, i_mode, edge[p] );
        i_qp = h->mb.i_chroma_qp;
    }

    i_ssd = ssd_plane( h, PIXEL_8x8, 0, (i8&1)*8, (i8>>1)*8 );
    if( CHROMA444 )
    {
        int chromassd = ssd_plane( h, PIXEL_8x8, 1, (i8&1)*8, (i8>>1)*8 )
                      + ssd_plane( h, PIXEL_8x8, 2, (i8&1)*8, (i8>>1)*8 );
        chromassd = ((uint64_t)chromassd * h->mb.i_chroma_lambda2_offset + 128) >> 8;
        i_ssd += chromassd;
    }

    if( h->param.b_cabac )
    {
        x264_cabac_t cabac_tmp;
        COPY_CABAC;
        x264_partition_i8x8_size_cabac( h, &cabac_tmp, i8, i_mode );
        i_bits = ( (uint64_t)cabac_tmp.f8_bits_encoded * i_lambda2 + 128 ) >> 8;
    }
    else
        i_bits = x264_partition_i8x8_size_cavlc( h, i8, i_mode ) * i_lambda2;

    return (i_ssd<<8) + i_bits;
}

static uint64_t x264_rd_cost_i4x4( x264_t *h, int i_lambda2, int i4, int i_mode )
{
    uint64_t i_ssd, i_bits;
    int plane_count = CHROMA444 ? 3 : 1;
    int i_qp = h->mb.i_qp;

    for( int p = 0; p < plane_count; p++ )
    {
        x264_mb_encode_i4x4( h, p, i4, i_qp, i_mode );
        i_qp = h->mb.i_chroma_qp;
    }

    i_ssd = ssd_plane( h, PIXEL_4x4, 0, block_idx_x[i4]*4, block_idx_y[i4]*4 );
    if( CHROMA444 )
    {
        int chromassd = ssd_plane( h, PIXEL_4x4, 1, block_idx_x[i4]*4, block_idx_y[i4]*4 )
                      + ssd_plane( h, PIXEL_4x4, 2, block_idx_x[i4]*4, block_idx_y[i4]*4 );
        chromassd = ((uint64_t)chromassd * h->mb.i_chroma_lambda2_offset + 128) >> 8;
        i_ssd += chromassd;
    }

    if( h->param.b_cabac )
    {
        x264_cabac_t cabac_tmp;
        COPY_CABAC;
        x264_partition_i4x4_size_cabac( h, &cabac_tmp, i4, i_mode );
        i_bits = ( (uint64_t)cabac_tmp.f8_bits_encoded * i_lambda2 + 128 ) >> 8;
    }
    else
        i_bits = x264_partition_i4x4_size_cavlc( h, i4, i_mode ) * i_lambda2;

    return (i_ssd<<8) + i_bits;
}

static uint64_t x264_rd_cost_i8x8_chroma( x264_t *h, int i_lambda2, int i_mode, int b_dct )
{
    uint64_t i_ssd, i_bits;

    if( b_dct )
        x264_mb_encode_8x8_chroma( h, 0, h->mb.i_chroma_qp );
    i_ssd = ssd_plane( h, PIXEL_8x8, 1, 0, 0 ) +
            ssd_plane( h, PIXEL_8x8, 2, 0, 0 );

    h->mb.i_chroma_pred_mode = i_mode;

    if( h->param.b_cabac )
    {
        x264_cabac_t cabac_tmp;
        COPY_CABAC;
        x264_i8x8_chroma_size_cabac( h, &cabac_tmp );
        i_bits = ( (uint64_t)cabac_tmp.f8_bits_encoded * i_lambda2 + 128 ) >> 8;
    }
    else
        i_bits = x264_i8x8_chroma_size_cavlc( h ) * i_lambda2;

    return (i_ssd<<8) + i_bits;
}
/****************************************************************************
 * Trellis RD quantization
 ****************************************************************************/

#define TRELLIS_SCORE_MAX ((uint64_t)1<<50)
#define CABAC_SIZE_BITS 8
#define SSD_WEIGHT_BITS 5
#define LAMBDA_BITS 4

/* precalculate the cost of coding various combinations of bits in a single context */
void x264_rdo_init( void )
{
    for( int i_prefix = 0; i_prefix < 15; i_prefix++ )
    {
        for( int i_ctx = 0; i_ctx < 128; i_ctx++ )
        {
            int f8_bits = 0;
            uint8_t ctx = i_ctx;

            for( int i = 1; i < i_prefix; i++ )
                f8_bits += x264_cabac_size_decision2( &ctx, 1 );
            if( i_prefix > 0 && i_prefix < 14 )
                f8_bits += x264_cabac_size_decision2( &ctx, 0 );
            f8_bits += 1 << CABAC_SIZE_BITS; //sign

            cabac_size_unary[i_prefix][i_ctx] = f8_bits;
            cabac_transition_unary[i_prefix][i_ctx] = ctx;
        }
    }
    for( int i_ctx = 0; i_ctx < 128; i_ctx++ )
    {
        int f8_bits = 0;
        uint8_t ctx = i_ctx;

        for( int i = 0; i < 5; i++ )
            f8_bits += x264_cabac_size_decision2( &ctx, 1 );
        f8_bits += 1 << CABAC_SIZE_BITS; //sign

        cabac_size_5ones[i_ctx] = f8_bits;
        cabac_transition_5ones[i_ctx] = ctx;
    }
}

typedef struct
{
    int64_t score;
    int level_idx; // index into level_tree[]
    uint8_t cabac_state[10]; //just the contexts relevant to coding abs_level_m1
} trellis_node_t;

// TODO:
// save cabac state between blocks?
// use trellis' RD score instead of x264_mb_decimate_score?
// code 8x8 sig/last flags forwards with deadzone and save the contexts at
//   each position?
// change weights when using CQMs?

// possible optimizations:
// make scores fit in 32bit
// save quantized coefs during rd, to avoid a duplicate trellis in the final encode
// if trellissing all MBRD modes, finish SSD calculation so we can skip all of
//   the normal dequant/idct/ssd/cabac

// the unquant_mf here is not the same as dequant_mf:
// in normal operation (dct->quant->dequant->idct) the dct and idct are not
// normalized. quant/dequant absorb those scaling factors.
// in this function, we just do (quant->unquant) and want the output to be
// comparable to the input. so unquant is the direct inverse of quant,
// and uses the dct scaling factors, not the idct ones.

static ALWAYS_INLINE
int quant_trellis_cabac( x264_t *h, dctcoef *dct,
                         const udctcoef *quant_mf, const int *unquant_mf,
                         const uint16_t *coef_weight, const uint8_t *zigzag,
                         int ctx_block_cat, int i_lambda2, int b_ac,
                         int b_chroma, int dc, int i_coefs, int idx )
{
    int abs_coefs[64], signs[64];
    trellis_node_t nodes[2][8];
    trellis_node_t *nodes_cur = nodes[0];
    trellis_node_t *nodes_prev = nodes[1];
    trellis_node_t *bnode;
    const int b_interlaced = MB_INTERLACED;
    uint8_t *cabac_state_sig = &h->cabac.state[ significant_coeff_flag_offset[b_interlaced][ctx_block_cat] ];
    uint8_t *cabac_state_last = &h->cabac.state[ last_coeff_flag_offset[b_interlaced][ctx_block_cat] ];
    const int f = 1 << 15; // no deadzone
    int i_last_nnz;
    int i;

    // (# of coefs) * (# of ctx) * (# of levels tried) = 1024
    // we don't need to keep all of those: (# of coefs) * (# of ctx) would be enough,
    // but it takes more time to remove dead states than you gain in reduced memory.
    struct
    {
        uint16_t abs_level;
        uint16_t next;
    } level_tree[64*8*2];
    int i_levels_used = 1;

    /* init coefs */
    for( i = i_coefs-1; i >= b_ac; i-- )
        if( (unsigned)(dct[zigzag[i]] * (dc?quant_mf[0]>>1:quant_mf[zigzag[i]]) + f-1) >= 2*f )
            break;

    if( i < b_ac )
    {
        /* We only need to zero an empty 4x4 block. 8x8 can be
           implicitly emptied via zero nnz, as can dc. */
        if( i_coefs == 16 && !dc )
            memset( dct, 0, 16 * sizeof(dctcoef) );
        return 0;
    }

    i_last_nnz = i;
    idx &= i_coefs == 64 ? 3 : 15;

    for( ; i >= b_ac; i-- )
    {
        int coef = dct[zigzag[i]];
        abs_coefs[i] = abs(coef);
        signs[i] = coef < 0 ? -1 : 1;
    }

    /* init trellis */
    for( int j = 1; j < 8; j++ )
        nodes_cur[j].score = TRELLIS_SCORE_MAX;
    nodes_cur[0].score = 0;
    nodes_cur[0].level_idx = 0;
    level_tree[0].abs_level = 0;
    level_tree[0].next = 0;

    // coefs are processed in reverse order, because that's how the abs value is coded.
    // last_coef and significant_coef flags are normally coded in forward order, but
    // we have to reverse them to match the levels.
    // in 4x4 blocks, last_coef and significant_coef use a separate context for each
    // position, so the order doesn't matter, and we don't even have to update their contexts.
    // in 8x8 blocks, some positions share contexts, so we'll just have to hope that
    // cabac isn't too sensitive.

    memcpy( nodes_cur[0].cabac_state, &h->cabac.state[ coeff_abs_level_m1_offset[ctx_block_cat] ], 10 );

    for( i = i_last_nnz; i >= b_ac; i-- )
    {
        int i_coef = abs_coefs[i];
        int q = ( f + i_coef * (dc?quant_mf[0]>>1:quant_mf[zigzag[i]]) ) >> 16;
        int cost_sig[2], cost_last[2];
        trellis_node_t n;

        // skip 0s: this doesn't affect the output, but saves some unnecessary computation.
        if( q == 0 )
        {
            // no need to calculate ssd of 0s: it's the same in all nodes.
            // no need to modify level_tree for ctx=0: it starts with an infinite loop of 0s.
            int sigindex = i_coefs == 64 ? significant_coeff_flag_offset_8x8[b_interlaced][i] : i;
            const uint32_t cost_sig0 = x264_cabac_size_decision_noup2( &cabac_state_sig[sigindex], 0 )
                                     * (uint64_t)i_lambda2 >> ( CABAC_SIZE_BITS - LAMBDA_BITS );
            for( int j = 1; j < 8; j++ )
            {
                if( nodes_cur[j].score != TRELLIS_SCORE_MAX )
                {
#define SET_LEVEL(n,l) \
                    level_tree[i_levels_used].abs_level = l; \
                    level_tree[i_levels_used].next = n.level_idx; \
                    n.level_idx = i_levels_used; \
                    i_levels_used++;

                    SET_LEVEL( nodes_cur[j], 0 );
                    nodes_cur[j].score += cost_sig0;
                }
            }
            continue;
        }

        XCHG( trellis_node_t*, nodes_cur, nodes_prev );

        for( int j = 0; j < 8; j++ )
            nodes_cur[j].score = TRELLIS_SCORE_MAX;

        if( i < i_coefs-1 )
        {
            int sigindex = i_coefs == 64 ? significant_coeff_flag_offset_8x8[b_interlaced][i] : i;
            int lastindex = i_coefs == 64 ? last_coeff_flag_offset_8x8[i] : i;
            cost_sig[0] = x264_cabac_size_decision_noup2( &cabac_state_sig[sigindex], 0 );
            cost_sig[1] = x264_cabac_size_decision_noup2( &cabac_state_sig[sigindex], 1 );
            cost_last[0] = x264_cabac_size_decision_noup2( &cabac_state_last[lastindex], 0 );
            cost_last[1] = x264_cabac_size_decision_noup2( &cabac_state_last[lastindex], 1 );
        }
        else
        {
            cost_sig[0] = cost_sig[1] = 0;
            cost_last[0] = cost_last[1] = 0;
        }

        // there are a few cases where increasing the coeff magnitude helps,
        // but it's only around .003 dB, and skipping them ~doubles the speed of trellis.
        // could also try q-2: that sometimes helps, but also sometimes decimates blocks
        // that are better left coded, especially at QP > 40.
        for( int abs_level = q; abs_level >= q-1; abs_level-- )
        {
            int unquant_abs_level = (((dc?unquant_mf[0]<<1:unquant_mf[zigzag[i]]) * abs_level + 128) >> 8);
            int d = i_coef - unquant_abs_level;
            int64_t ssd;
            /* Psy trellis: bias in favor of higher AC coefficients in the reconstructed frame. */
            if( h->mb.i_psy_trellis && i && !dc && !b_chroma )
            {
                int orig_coef = (i_coefs == 64) ? h->mb.pic.fenc_dct8[idx][zigzag[i]] : h->mb.pic.fenc_dct4[idx][zigzag[i]];
                int predicted_coef = orig_coef - i_coef * signs[i];
                int psy_value = h->mb.i_psy_trellis * abs(predicted_coef + unquant_abs_level * signs[i]);
                int psy_weight = (i_coefs == 64) ? x264_dct8_weight_tab[zigzag[i]] : x264_dct4_weight_tab[zigzag[i]];
                ssd = (int64_t)d*d * coef_weight[i] - psy_weight * psy_value;
            }
            else
            /* FIXME: for i16x16 dc is this weight optimal? */
                ssd = (int64_t)d*d * (dc?256:coef_weight[i]);

            for( int j = 0; j < 8; j++ )
            {
                int node_ctx = j;
                if( nodes_prev[j].score == TRELLIS_SCORE_MAX )
                    continue;
                n = nodes_prev[j];

                /* code the proposed level, and count how much entropy it would take */
                if( abs_level || node_ctx )
                {
                    unsigned f8_bits = cost_sig[ abs_level != 0 ];
                    if( abs_level )
                    {
                        const int i_prefix = X264_MIN( abs_level - 1, 14 );
                        f8_bits += cost_last[ node_ctx == 0 ];
                        f8_bits += x264_cabac_size_decision2( &n.cabac_state[coeff_abs_level1_ctx[node_ctx]], i_prefix > 0 );
                        if( i_prefix > 0 )
                        {
                            uint8_t *ctx = &n.cabac_state[coeff_abs_levelgt1_ctx[node_ctx]];
                            f8_bits += cabac_size_unary[i_prefix][*ctx];
                            *ctx = cabac_transition_unary[i_prefix][*ctx];
                            if( abs_level >= 15 )
                                f8_bits += bs_size_ue_big( abs_level - 15 ) << CABAC_SIZE_BITS;
                            node_ctx = coeff_abs_level_transition[1][node_ctx];
                        }
                        else
                        {
                            f8_bits += 1 << CABAC_SIZE_BITS;
                            node_ctx = coeff_abs_level_transition[0][node_ctx];
                        }
                    }
                    n.score += (uint64_t)f8_bits * i_lambda2 >> ( CABAC_SIZE_BITS - LAMBDA_BITS );
                }

                if( j || i || dc )
                    n.score += ssd;
                /* Optimize rounding for DC coefficients in DC-only luma 4x4/8x8 blocks. */
                else
                {
                    d = i_coef * signs[0] - ((unquant_abs_level * signs[0] + 8)&~15);
                    n.score += (int64_t)d*d * coef_weight[i];
                }

                /* save the node if it's better than any existing node with the same cabac ctx */
                if( n.score < nodes_cur[node_ctx].score )
                {
                    SET_LEVEL( n, abs_level );
                    nodes_cur[node_ctx] = n;
                }
            }
        }
    }

    /* output levels from the best path through the trellis */
    bnode = &nodes_cur[0];
    for( int j = 1; j < 8; j++ )
        if( nodes_cur[j].score < bnode->score )
            bnode = &nodes_cur[j];

    if( bnode == &nodes_cur[0] )
    {
        if( i_coefs == 16 && !dc )
            memset( dct, 0, 16 * sizeof(dctcoef) );
        return 0;
    }

    int level = bnode->level_idx;
    for( i = b_ac; level; i++ )
    {
        dct[zigzag[i]] = level_tree[level].abs_level * signs[i];
        level = level_tree[level].next;
    }
    for( ; i < i_coefs; i++ )
        dct[zigzag[i]] = 0;

    return 1;
}

/* FIXME: This is a gigantic hack.  See below.
 *
 * CAVLC is much more difficult to trellis than CABAC.
 *
 * CABAC has only three states to track: significance map, last, and the
 * level state machine.
 * CAVLC, by comparison, has five: coeff_token (trailing + total),
 * total_zeroes, zero_run, and the level state machine.
 *
 * I know of no paper that has managed to design a close-to-optimal trellis
 * that covers all five of these and isn't exponential-time.  As a result, this
 * "trellis" isn't: it's just a QNS search.  Patches welcome for something better.
 * It's actually surprisingly fast, albeit not quite optimal.  It's pretty close
 * though; since CAVLC only has 2^16 possible rounding modes (assuming only two
 * roundings as options), a bruteforce search is feasible.  Testing shows
 * that this QNS is reasonably close to optimal in terms of compression.
 *
 * TODO:
 *  Don't bother changing large coefficients when it wouldn't affect bit cost
 *  (e.g. only affecting bypassed suffix bits).
 *  Don't re-run all parts of CAVLC bit cost calculation when not necessary.
 *  e.g. when changing a coefficient from one non-zero value to another in
 *  such a way that trailing ones and suffix length isn't affected. */
static ALWAYS_INLINE
int quant_trellis_cavlc( x264_t *h, dctcoef *dct,
                         const udctcoef *quant_mf, const int *unquant_mf,
                         const uint16_t *coef_weight, const uint8_t *zigzag,
                         int ctx_block_cat, int i_lambda2, int b_ac,
                         int b_chroma, int dc, int i_coefs, int idx, int b_8x8 )
{
    ALIGNED_16( dctcoef quant_coefs[2][16] );
    ALIGNED_16( dctcoef coefs[16] ) = {0};
    int delta_distortion[16];
    int64_t score = 1ULL<<62;
    int i, j;
    const int f = 1<<15;
    int nC = ctx_block_cat == DCT_CHROMA_DC ? 4 : ct_index[x264_mb_predict_non_zero_code( h, ctx_block_cat == DCT_LUMA_DC ? (idx - LUMA_DC)*16 : idx )];

    /* Code for handling 8x8dct -> 4x4dct CAVLC munging.  Input/output use a different
     * step/start/end than internal processing. */
    int step = 1;
    int start = b_ac;
    int end = i_coefs - 1;
    if( b_8x8 )
    {
        start = idx&3;
        end = 60 + start;
        step = 4;
    }
    idx &= 15;

    i_lambda2 <<= LAMBDA_BITS;

    /* Find last non-zero coefficient. */
    for( i = end; i >= start; i -= step )
        if( (unsigned)(dct[zigzag[i]] * (dc?quant_mf[0]>>1:quant_mf[zigzag[i]]) + f-1) >= 2*f )
            break;

    if( i < start )
        goto zeroblock;

    /* Prepare for QNS search: calculate distortion caused by each DCT coefficient
     * rounding to be searched.
     *
     * We only search two roundings (nearest and nearest-1) like in CABAC trellis,
     * so we just store the difference in distortion between them. */
    int i_last_nnz = b_8x8 ? i >> 2 : i;
    int coef_mask = 0;
    int round_mask = 0;
    for( i = b_ac, j = start; i <= i_last_nnz; i++, j += step )
    {
        int coef = dct[zigzag[j]];
        int abs_coef = abs(coef);
        int sign = coef < 0 ? -1 : 1;
        int nearest_quant = ( f + abs_coef * (dc?quant_mf[0]>>1:quant_mf[zigzag[j]]) ) >> 16;
        quant_coefs[1][i] = quant_coefs[0][i] = sign * nearest_quant;
        coefs[i] = quant_coefs[1][i];
        if( nearest_quant )
        {
            /* We initialize the trellis with a deadzone halfway between nearest rounding
             * and always-round-down.  This gives much better results than initializing to either
             * extreme.
             * FIXME: should we initialize to the deadzones used by deadzone quant? */
            int deadzone_quant = ( f/2 + abs_coef * (dc?quant_mf[0]>>1:quant_mf[zigzag[j]]) ) >> 16;
            int unquant1 = (((dc?unquant_mf[0]<<1:unquant_mf[zigzag[j]]) * (nearest_quant-0) + 128) >> 8);
            int unquant0 = (((dc?unquant_mf[0]<<1:unquant_mf[zigzag[j]]) * (nearest_quant-1) + 128) >> 8);
            int d1 = abs_coef - unquant1;
            int d0 = abs_coef - unquant0;
            delta_distortion[i] = (d0*d0 - d1*d1) * (dc?256:coef_weight[j]);

            /* Psy trellis: bias in favor of higher AC coefficients in the reconstructed frame. */
            if( h->mb.i_psy_trellis && j && !dc && !b_chroma )
            {
                int orig_coef = b_8x8 ? h->mb.pic.fenc_dct8[idx>>2][zigzag[j]] : h->mb.pic.fenc_dct4[idx][zigzag[j]];
                int predicted_coef = orig_coef - coef;
                int psy_weight = b_8x8 ? x264_dct8_weight_tab[zigzag[j]] : x264_dct4_weight_tab[zigzag[j]];
                int psy_value0 = h->mb.i_psy_trellis * abs(predicted_coef + unquant0 * sign);
                int psy_value1 = h->mb.i_psy_trellis * abs(predicted_coef + unquant1 * sign);
                delta_distortion[i] += (psy_value0 - psy_value1) * psy_weight;
            }

            quant_coefs[0][i] = sign * (nearest_quant-1);
            if( deadzone_quant != nearest_quant )
                coefs[i] = quant_coefs[0][i];
            else
                round_mask |= 1 << i;
        }
        else
            delta_distortion[i] = 0;
        coef_mask |= (!!coefs[i]) << i;
    }

    /* Calculate the cost of the starting state. */
    h->out.bs.i_bits_encoded = 0;
    if( !coef_mask )
        bs_write_vlc( &h->out.bs, x264_coeff0_token[nC] );
    else
        block_residual_write_cavlc_internal( h, ctx_block_cat, coefs + b_ac, nC );
    score = (int64_t)h->out.bs.i_bits_encoded * i_lambda2;

    /* QNS loop: pick the change that improves RD the most, apply it, repeat.
     * coef_mask and round_mask are used to simplify tracking of nonzeroness
     * and rounding modes chosen. */
    while( 1 )
    {
        int64_t iter_score = score;
        int iter_distortion_delta = 0;
        int iter_coef = -1;
        int iter_mask = coef_mask;
        int iter_round = round_mask;
        for( i = b_ac; i <= i_last_nnz; i++ )
        {
            if( !delta_distortion[i] )
                continue;

            /* Set up all the variables for this iteration. */
            int cur_round = round_mask ^ (1 << i);
            int round_change = (cur_round >> i)&1;
            int old_coef = coefs[i];
            int new_coef = quant_coefs[round_change][i];
            int cur_mask = (coef_mask&~(1 << i))|(!!new_coef << i);
            int cur_distortion_delta = delta_distortion[i] * (round_change ? -1 : 1);
            int64_t cur_score = cur_distortion_delta;
            coefs[i] = new_coef;

            /* Count up bits. */
            h->out.bs.i_bits_encoded = 0;
            if( !cur_mask )
                bs_write_vlc( &h->out.bs, x264_coeff0_token[nC] );
            else
                block_residual_write_cavlc_internal( h, ctx_block_cat, coefs + b_ac, nC );
            cur_score += (int64_t)h->out.bs.i_bits_encoded * i_lambda2;

            coefs[i] = old_coef;
            if( cur_score < iter_score )
            {
                iter_score = cur_score;
                iter_coef = i;
                iter_mask = cur_mask;
                iter_round = cur_round;
                iter_distortion_delta = cur_distortion_delta;
            }
        }
        if( iter_coef >= 0 )
        {
            score = iter_score - iter_distortion_delta;
            coef_mask = iter_mask;
            round_mask = iter_round;
            coefs[iter_coef] = quant_coefs[((round_mask >> iter_coef)&1)][iter_coef];
            /* Don't try adjusting coefficients we've already adjusted.
             * Testing suggests this doesn't hurt results -- and sometimes actually helps. */
            delta_distortion[iter_coef] = 0;
        }
        else
            break;
    }

    if( coef_mask )
    {
        for( i = b_ac, j = start; i <= i_last_nnz; i++, j += step )
            dct[zigzag[j]] = coefs[i];
        for( ; j <= end; j += step )
            dct[zigzag[j]] = 0;
        return 1;
    }

zeroblock:
    if( !dc )
    {
        if( b_8x8 )
            for( i = start; i <= end; i+=step )
                dct[zigzag[i]] = 0;
        else
            memset( dct, 0, 16*sizeof(dctcoef) );
    }
    return 0;
}

const static uint8_t x264_zigzag_scan2[4] = {0,1,2,3};

int x264_quant_dc_trellis( x264_t *h, dctcoef *dct, int i_quant_cat,
                           int i_qp, int ctx_block_cat, int b_intra, int b_chroma, int idx )
{
    if( h->param.b_cabac )
        return quant_trellis_cabac( h, dct,
            h->quant4_mf[i_quant_cat][i_qp], h->unquant4_mf[i_quant_cat][i_qp],
            NULL, ctx_block_cat==DCT_CHROMA_DC ? x264_zigzag_scan2 : x264_zigzag_scan4[MB_INTERLACED],
            ctx_block_cat, h->mb.i_trellis_lambda2[b_chroma][b_intra], 0, b_chroma, 1, ctx_block_cat==DCT_CHROMA_DC ? 4 : 16, idx );

    if( ctx_block_cat != DCT_CHROMA_DC )
        ctx_block_cat = DCT_LUMA_DC;

    return quant_trellis_cavlc( h, dct,
        h->quant4_mf[i_quant_cat][i_qp], h->unquant4_mf[i_quant_cat][i_qp],
        NULL, ctx_block_cat==DCT_CHROMA_DC ? x264_zigzag_scan2 : x264_zigzag_scan4[MB_INTERLACED],
        ctx_block_cat, h->mb.i_trellis_lambda2[b_chroma][b_intra], 0, b_chroma, 1, ctx_block_cat==DCT_CHROMA_DC ? 4 : 16, idx, 0 );
}

int x264_quant_4x4_trellis( x264_t *h, dctcoef *dct, int i_quant_cat,
                            int i_qp, int ctx_block_cat, int b_intra, int b_chroma, int idx )
{
    static const uint8_t ctx_ac[14] = {0,1,0,0,1,0,0,1,0,0,0,1,0,0};
    int b_ac = ctx_ac[ctx_block_cat];
    if( h->param.b_cabac )
        return quant_trellis_cabac( h, dct,
            h->quant4_mf[i_quant_cat][i_qp], h->unquant4_mf[i_quant_cat][i_qp],
            x264_dct4_weight2_zigzag[MB_INTERLACED],
            x264_zigzag_scan4[MB_INTERLACED],
            ctx_block_cat, h->mb.i_trellis_lambda2[b_chroma][b_intra], b_ac, b_chroma, 0, 16, idx );

    return quant_trellis_cavlc( h, dct,
            h->quant4_mf[i_quant_cat][i_qp], h->unquant4_mf[i_quant_cat][i_qp],
            x264_dct4_weight2_zigzag[MB_INTERLACED],
            x264_zigzag_scan4[MB_INTERLACED],
            ctx_block_cat, h->mb.i_trellis_lambda2[b_chroma][b_intra], b_ac, b_chroma, 0, 16, idx, 0 );
}

int x264_quant_8x8_trellis( x264_t *h, dctcoef *dct, int i_quant_cat,
                            int i_qp, int ctx_block_cat, int b_intra, int b_chroma, int idx )
{
    if( h->param.b_cabac )
    {
        return quant_trellis_cabac( h, dct,
            h->quant8_mf[i_quant_cat][i_qp], h->unquant8_mf[i_quant_cat][i_qp],
            x264_dct8_weight2_zigzag[MB_INTERLACED],
            x264_zigzag_scan8[MB_INTERLACED],
            ctx_block_cat, h->mb.i_trellis_lambda2[b_chroma][b_intra], 0, b_chroma, 0, 64, idx );
    }

    /* 8x8 CAVLC is split into 4 4x4 blocks */
    int nzaccum = 0;
    for( int i = 0; i < 4; i++ )
    {
        int nz = quant_trellis_cavlc( h, dct,
            h->quant8_mf[i_quant_cat][i_qp], h->unquant8_mf[i_quant_cat][i_qp],
            x264_dct8_weight2_zigzag[MB_INTERLACED],
            x264_zigzag_scan8[MB_INTERLACED],
            DCT_LUMA_4x4, h->mb.i_trellis_lambda2[b_chroma][b_intra], 0, b_chroma, 0, 16, idx*4+i, 1 );
        /* Set up nonzero count for future calls */
        h->mb.cache.non_zero_count[x264_scan8[idx*4+i]] = nz;
        nzaccum |= nz;
    }
    return nzaccum;
}

/*****************************************************************************
 * quant.c: quantization and level-run
 *****************************************************************************
 * Copyright (C) 2005-2011 x264 project
 *
 * Authors: Loren Merritt <lorenm@u.washington.edu>
 *          Jason Garrett-Glaser <darkshikari@gmail.com>
 *          Christian Heine <sennindemokrit@gmx.net>
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

#include "common.h"

#if HAVE_MMX
#include "x86/quant.h"
#endif
#if ARCH_PPC
#   include "ppc/quant.h"
#endif
#if ARCH_ARM
#   include "arm/quant.h"
#endif

#define QUANT_ONE( coef, mf, f ) \
{ \
    if( (coef) > 0 ) \
        (coef) = (f + (coef)) * (mf) >> 16; \
    else \
        (coef) = - ((f - (coef)) * (mf) >> 16); \
    nz |= (coef); \
}

static int quant_8x8( dctcoef dct[64], udctcoef mf[64], udctcoef bias[64] )
{
    int nz = 0;
    for( int i = 0; i < 64; i++ )
        QUANT_ONE( dct[i], mf[i], bias[i] );
    return !!nz;
}

static int quant_4x4( dctcoef dct[16], udctcoef mf[16], udctcoef bias[16] )
{
    int nz = 0;
    for( int i = 0; i < 16; i++ )
        QUANT_ONE( dct[i], mf[i], bias[i] );
    return !!nz;
}

static int quant_4x4_dc( dctcoef dct[16], int mf, int bias )
{
    int nz = 0;
    for( int i = 0; i < 16; i++ )
        QUANT_ONE( dct[i], mf, bias );
    return !!nz;
}

static int quant_2x2_dc( dctcoef dct[4], int mf, int bias )
{
    int nz = 0;
    QUANT_ONE( dct[0], mf, bias );
    QUANT_ONE( dct[1], mf, bias );
    QUANT_ONE( dct[2], mf, bias );
    QUANT_ONE( dct[3], mf, bias );
    return !!nz;
}

#define DEQUANT_SHL( x ) \
    dct[x] = ( dct[x] * dequant_mf[i_mf][x] ) << i_qbits

#define DEQUANT_SHR( x ) \
    dct[x] = ( dct[x] * dequant_mf[i_mf][x] + f ) >> (-i_qbits)

static void dequant_4x4( dctcoef dct[16], int dequant_mf[6][16], int i_qp )
{
    const int i_mf = i_qp%6;
    const int i_qbits = i_qp/6 - 4;

    if( i_qbits >= 0 )
    {
        for( int i = 0; i < 16; i++ )
            DEQUANT_SHL( i );
    }
    else
    {
        const int f = 1 << (-i_qbits-1);
        for( int i = 0; i < 16; i++ )
            DEQUANT_SHR( i );
    }
}

static void dequant_8x8( dctcoef dct[64], int dequant_mf[6][64], int i_qp )
{
    const int i_mf = i_qp%6;
    const int i_qbits = i_qp/6 - 6;

    if( i_qbits >= 0 )
    {
        for( int i = 0; i < 64; i++ )
            DEQUANT_SHL( i );
    }
    else
    {
        const int f = 1 << (-i_qbits-1);
        for( int i = 0; i < 64; i++ )
            DEQUANT_SHR( i );
    }
}

static void dequant_4x4_dc( dctcoef dct[16], int dequant_mf[6][16], int i_qp )
{
    const int i_qbits = i_qp/6 - 6;

    if( i_qbits >= 0 )
    {
        const int i_dmf = dequant_mf[i_qp%6][0] << i_qbits;
        for( int i = 0; i < 16; i++ )
            dct[i] *= i_dmf;
    }
    else
    {
        const int i_dmf = dequant_mf[i_qp%6][0];
        const int f = 1 << (-i_qbits-1);
        for( int i = 0; i < 16; i++ )
            dct[i] = ( dct[i] * i_dmf + f ) >> (-i_qbits);
    }
}

static ALWAYS_INLINE void idct_dequant_2x2_dconly( dctcoef out[4], dctcoef dct[4], int dequant_mf )
{
    int d0 = dct[0] + dct[1];
    int d1 = dct[2] + dct[3];
    int d2 = dct[0] - dct[1];
    int d3 = dct[2] - dct[3];
    out[0] = (d0 + d1) * dequant_mf >> 5;
    out[1] = (d0 - d1) * dequant_mf >> 5;
    out[2] = (d2 + d3) * dequant_mf >> 5;
    out[3] = (d2 - d3) * dequant_mf >> 5;
}

static ALWAYS_INLINE int idct_dequant_round_2x2_dc( dctcoef ref[4], dctcoef dct[4], int dequant_mf )
{
    dctcoef out[4];
    idct_dequant_2x2_dconly( out, dct, dequant_mf );
    return ((ref[0] ^ (out[0]+32))
          | (ref[1] ^ (out[1]+32))
          | (ref[2] ^ (out[2]+32))
          | (ref[3] ^ (out[3]+32))) >> 6;
}

static int optimize_chroma_dc( dctcoef dct[4], int dequant_mf )
{
    /* dequant_mf = h->dequant4_mf[CQM_4IC + b_inter][i_qp%6][0] << i_qp/6, max 32*64 */
    dctcoef dct_orig[4];
    int coeff, nz;

    idct_dequant_2x2_dconly( dct_orig, dct, dequant_mf );
    dct_orig[0] += 32;
    dct_orig[1] += 32;
    dct_orig[2] += 32;
    dct_orig[3] += 32;

    /* If the DC coefficients already round to zero, terminate early. */
    if( !((dct_orig[0]|dct_orig[1]|dct_orig[2]|dct_orig[3])>>6) )
        return 0;

    /* Start with the highest frequency coefficient... is this the best option? */
    for( nz = 0, coeff = 3; coeff >= 0; coeff-- )
    {
        int level = dct[coeff];
        int sign = level>>31 | 1; /* dct2x2[coeff] < 0 ? -1 : 1 */

        while( level )
        {
            dct[coeff] = level - sign;
            if( idct_dequant_round_2x2_dc( dct_orig, dct, dequant_mf ) )
            {
                nz = 1;
                dct[coeff] = level;
                break;
            }
            level -= sign;
        }
    }

    return nz;
}

static void x264_denoise_dct( dctcoef *dct, uint32_t *sum, udctcoef *offset, int size )
{
    for( int i = 0; i < size; i++ )
    {
        int level = dct[i];
        int sign = level>>31;
        level = (level+sign)^sign;
        sum[i] += level;
        level -= offset[i];
        dct[i] = level<0 ? 0 : (level^sign)-sign;
    }
}

/* (ref: JVT-B118)
 * x264_mb_decimate_score: given dct coeffs it returns a score to see if we could empty this dct coeffs
 * to 0 (low score means set it to null)
 * Used in inter macroblock (luma and chroma)
 *  luma: for a 8x8 block: if score < 4 -> null
 *        for the complete mb: if score < 6 -> null
 *  chroma: for the complete mb: if score < 7 -> null
 */

const uint8_t x264_decimate_table4[16] =
{
    3,2,2,1,1,1,0,0,0,0,0,0,0,0,0,0
};
const uint8_t x264_decimate_table8[64] =
{
    3,3,3,3,2,2,2,2,2,2,2,2,1,1,1,1,
    1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

static int ALWAYS_INLINE x264_decimate_score_internal( dctcoef *dct, int i_max )
{
    const uint8_t *ds_table = (i_max == 64) ? x264_decimate_table8 : x264_decimate_table4;
    int i_score = 0;
    int idx = i_max - 1;

    while( idx >= 0 && dct[idx] == 0 )
        idx--;
    while( idx >= 0 )
    {
        int i_run;

        if( (unsigned)(dct[idx--] + 1) > 2 )
            return 9;

        i_run = 0;
        while( idx >= 0 && dct[idx] == 0 )
        {
            idx--;
            i_run++;
        }
        i_score += ds_table[i_run];
    }

    return i_score;
}

static int x264_decimate_score15( dctcoef *dct )
{
    return x264_decimate_score_internal( dct+1, 15 );
}
static int x264_decimate_score16( dctcoef *dct )
{
    return x264_decimate_score_internal( dct, 16 );
}
static int x264_decimate_score64( dctcoef *dct )
{
    return x264_decimate_score_internal( dct, 64 );
}

static int ALWAYS_INLINE x264_coeff_last_internal( dctcoef *l, int i_count )
{
    int i_last = i_count-1;
    while( i_last >= 0 && l[i_last] == 0 )
        i_last--;
    return i_last;
}

static int x264_coeff_last4( dctcoef *l )
{
    return x264_coeff_last_internal( l, 4 );
}
static int x264_coeff_last15( dctcoef *l )
{
    return x264_coeff_last_internal( l, 15 );
}
static int x264_coeff_last16( dctcoef *l )
{
    return x264_coeff_last_internal( l, 16 );
}
static int x264_coeff_last64( dctcoef *l )
{
    return x264_coeff_last_internal( l, 64 );
}

#define level_run(num)\
static int x264_coeff_level_run##num( dctcoef *dct, x264_run_level_t *runlevel )\
{\
    int i_last = runlevel->last = x264_coeff_last##num(dct);\
    int i_total = 0;\
    do\
    {\
        int r = 0;\
        runlevel->level[i_total] = dct[i_last];\
        while( --i_last >= 0 && dct[i_last] == 0 )\
            r++;\
        runlevel->run[i_total++] = r;\
    } while( i_last >= 0 );\
    return i_total;\
}

level_run(4)
level_run(15)
level_run(16)


void x264_quant_init( x264_t *h, int cpu, x264_quant_function_t *pf )
{
    pf->quant_8x8 = quant_8x8;
    pf->quant_4x4 = quant_4x4;
    pf->quant_4x4_dc = quant_4x4_dc;
    pf->quant_2x2_dc = quant_2x2_dc;

    pf->dequant_4x4 = dequant_4x4;
    pf->dequant_4x4_dc = dequant_4x4_dc;
    pf->dequant_8x8 = dequant_8x8;

    pf->optimize_chroma_dc = optimize_chroma_dc;

    pf->denoise_dct = x264_denoise_dct;
    pf->decimate_score15 = x264_decimate_score15;
    pf->decimate_score16 = x264_decimate_score16;
    pf->decimate_score64 = x264_decimate_score64;

    pf->coeff_last[DCT_CHROMA_DC] = x264_coeff_last4;
    pf->coeff_last[  DCT_LUMA_AC] = x264_coeff_last15;
    pf->coeff_last[ DCT_LUMA_4x4] = x264_coeff_last16;
    pf->coeff_last[ DCT_LUMA_8x8] = x264_coeff_last64;
    pf->coeff_level_run[DCT_CHROMA_DC] = x264_coeff_level_run4;
    pf->coeff_level_run[  DCT_LUMA_AC] = x264_coeff_level_run15;
    pf->coeff_level_run[ DCT_LUMA_4x4] = x264_coeff_level_run16;

#if HIGH_BIT_DEPTH
#if HAVE_MMX
    if( cpu&X264_CPU_MMX2 )
    {
#if ARCH_X86
        pf->denoise_dct = x264_denoise_dct_mmx;
        pf->decimate_score15 = x264_decimate_score15_mmx2;
        pf->decimate_score16 = x264_decimate_score16_mmx2;
        if( cpu&X264_CPU_SLOW_CTZ )
        {
            pf->decimate_score15 = x264_decimate_score15_mmx2_slowctz;
            pf->decimate_score16 = x264_decimate_score16_mmx2_slowctz;
        }
        pf->decimate_score64 = x264_decimate_score64_mmx2;
        pf->coeff_last[DCT_CHROMA_DC] = x264_coeff_last4_mmx2;
        pf->coeff_last[  DCT_LUMA_AC] = x264_coeff_last15_mmx2;
        pf->coeff_last[ DCT_LUMA_4x4] = x264_coeff_last16_mmx2;
        pf->coeff_last[ DCT_LUMA_8x8] = x264_coeff_last64_mmx2;
        pf->coeff_level_run[  DCT_LUMA_AC] = x264_coeff_level_run15_mmx2;
        pf->coeff_level_run[ DCT_LUMA_4x4] = x264_coeff_level_run16_mmx2;
#endif
        pf->coeff_level_run[DCT_CHROMA_DC] = x264_coeff_level_run4_mmx2;
        if( cpu&X264_CPU_LZCNT )
            pf->coeff_level_run[DCT_CHROMA_DC] = x264_coeff_level_run4_mmx2_lzcnt;
    }
    if( cpu&X264_CPU_SSE2 )
    {
        pf->quant_4x4 = x264_quant_4x4_sse2;
        pf->quant_8x8 = x264_quant_8x8_sse2;
        pf->quant_2x2_dc = x264_quant_2x2_dc_sse2;
        pf->quant_4x4_dc = x264_quant_4x4_dc_sse2;
        pf->dequant_4x4 = x264_dequant_4x4_sse2;
        pf->dequant_8x8 = x264_dequant_8x8_sse2;
        pf->dequant_4x4_dc = x264_dequant_4x4dc_sse2;
        pf->denoise_dct = x264_denoise_dct_sse2;
        pf->decimate_score15 = x264_decimate_score15_sse2;
        pf->decimate_score16 = x264_decimate_score16_sse2;
        pf->decimate_score64 = x264_decimate_score64_sse2;
        if( cpu&X264_CPU_SLOW_CTZ )
        {
            pf->decimate_score15 = x264_decimate_score15_sse2_slowctz;
            pf->decimate_score16 = x264_decimate_score16_sse2_slowctz;
        }
        pf->coeff_last[ DCT_LUMA_AC] = x264_coeff_last15_sse2;
        pf->coeff_last[DCT_LUMA_4x4] = x264_coeff_last16_sse2;
        pf->coeff_last[DCT_LUMA_8x8] = x264_coeff_last64_sse2;
        pf->coeff_level_run[ DCT_LUMA_AC] = x264_coeff_level_run15_sse2;
        pf->coeff_level_run[DCT_LUMA_4x4] = x264_coeff_level_run16_sse2;
        if( cpu&X264_CPU_LZCNT )
        {
            pf->coeff_last[DCT_CHROMA_DC] = x264_coeff_last4_mmx2_lzcnt;
            pf->coeff_last[ DCT_LUMA_AC] = x264_coeff_last15_sse2_lzcnt;
            pf->coeff_last[DCT_LUMA_4x4] = x264_coeff_last16_sse2_lzcnt;
            pf->coeff_last[DCT_LUMA_8x8] = x264_coeff_last64_sse2_lzcnt;
            pf->coeff_level_run[ DCT_LUMA_AC] = x264_coeff_level_run15_sse2_lzcnt;
            pf->coeff_level_run[DCT_LUMA_4x4] = x264_coeff_level_run16_sse2_lzcnt;
        }
    }
    if( cpu&X264_CPU_SSSE3 )
    {
        pf->quant_4x4 = x264_quant_4x4_ssse3;
        pf->quant_8x8 = x264_quant_8x8_ssse3;
        pf->quant_2x2_dc = x264_quant_2x2_dc_ssse3;
        pf->quant_4x4_dc = x264_quant_4x4_dc_ssse3;
        pf->denoise_dct = x264_denoise_dct_ssse3;
        pf->decimate_score15 = x264_decimate_score15_ssse3;
        pf->decimate_score16 = x264_decimate_score16_ssse3;
        if( cpu&X264_CPU_SLOW_CTZ )
        {
            pf->decimate_score15 = x264_decimate_score15_ssse3_slowctz;
            pf->decimate_score16 = x264_decimate_score16_ssse3_slowctz;
        }
        pf->decimate_score64 = x264_decimate_score64_ssse3;
    }
    if( cpu&X264_CPU_SSE4 )
    {
        pf->quant_2x2_dc = x264_quant_2x2_dc_sse4;
        pf->quant_4x4_dc = x264_quant_4x4_dc_sse4;
        pf->quant_4x4 = x264_quant_4x4_sse4;
        pf->quant_8x8 = x264_quant_8x8_sse4;
    }
    if( cpu&X264_CPU_AVX )
    {
        pf->denoise_dct = x264_denoise_dct_avx;
    }
#endif // HAVE_MMX
#else // !HIGH_BIT_DEPTH
#if HAVE_MMX
    if( cpu&X264_CPU_MMX )
    {
#if ARCH_X86
        pf->quant_4x4 = x264_quant_4x4_mmx;
        pf->quant_8x8 = x264_quant_8x8_mmx;
        pf->dequant_4x4 = x264_dequant_4x4_mmx;
        pf->dequant_4x4_dc = x264_dequant_4x4dc_mmx2;
        pf->dequant_8x8 = x264_dequant_8x8_mmx;
        if( h->param.i_cqm_preset == X264_CQM_FLAT )
        {
            pf->dequant_4x4 = x264_dequant_4x4_flat16_mmx;
            pf->dequant_8x8 = x264_dequant_8x8_flat16_mmx;
        }
        pf->denoise_dct = x264_denoise_dct_mmx;
#endif
    }

    if( cpu&X264_CPU_MMX2 )
    {
        pf->quant_2x2_dc = x264_quant_2x2_dc_mmx2;
#if ARCH_X86
        pf->quant_4x4_dc = x264_quant_4x4_dc_mmx2;
        pf->decimate_score15 = x264_decimate_score15_mmx2;
        pf->decimate_score16 = x264_decimate_score16_mmx2;
        if( cpu&X264_CPU_SLOW_CTZ )
        {
            pf->decimate_score15 = x264_decimate_score15_mmx2_slowctz;
            pf->decimate_score16 = x264_decimate_score16_mmx2_slowctz;
        }
        pf->decimate_score64 = x264_decimate_score64_mmx2;
        pf->coeff_last[  DCT_LUMA_AC] = x264_coeff_last15_mmx2;
        pf->coeff_last[ DCT_LUMA_4x4] = x264_coeff_last16_mmx2;
        pf->coeff_last[ DCT_LUMA_8x8] = x264_coeff_last64_mmx2;
        pf->coeff_level_run[  DCT_LUMA_AC] = x264_coeff_level_run15_mmx2;
        pf->coeff_level_run[ DCT_LUMA_4x4] = x264_coeff_level_run16_mmx2;
#endif
        pf->coeff_last[DCT_CHROMA_DC] = x264_coeff_last4_mmx2;
        pf->coeff_level_run[DCT_CHROMA_DC] = x264_coeff_level_run4_mmx2;
        if( cpu&X264_CPU_LZCNT )
        {
            pf->coeff_last[DCT_CHROMA_DC] = x264_coeff_last4_mmx2_lzcnt;
            pf->coeff_level_run[DCT_CHROMA_DC] = x264_coeff_level_run4_mmx2_lzcnt;
        }
    }

    if( cpu&X264_CPU_SSE2 )
    {
        pf->quant_4x4_dc = x264_quant_4x4_dc_sse2;
        pf->quant_4x4 = x264_quant_4x4_sse2;
        pf->quant_8x8 = x264_quant_8x8_sse2;
        pf->dequant_4x4 = x264_dequant_4x4_sse2;
        pf->dequant_4x4_dc = x264_dequant_4x4dc_sse2;
        pf->dequant_8x8 = x264_dequant_8x8_sse2;
        if( h->param.i_cqm_preset == X264_CQM_FLAT )
        {
            pf->dequant_4x4 = x264_dequant_4x4_flat16_sse2;
            pf->dequant_8x8 = x264_dequant_8x8_flat16_sse2;
        }
        pf->optimize_chroma_dc = x264_optimize_chroma_dc_sse2;
        pf->denoise_dct = x264_denoise_dct_sse2;
        pf->decimate_score15 = x264_decimate_score15_sse2;
        pf->decimate_score16 = x264_decimate_score16_sse2;
        pf->decimate_score64 = x264_decimate_score64_sse2;
        if( cpu&X264_CPU_SLOW_CTZ )
        {
            pf->decimate_score15 = x264_decimate_score15_sse2_slowctz;
            pf->decimate_score16 = x264_decimate_score16_sse2_slowctz;
        }
        pf->coeff_last[ DCT_LUMA_AC] = x264_coeff_last15_sse2;
        pf->coeff_last[DCT_LUMA_4x4] = x264_coeff_last16_sse2;
        pf->coeff_last[DCT_LUMA_8x8] = x264_coeff_last64_sse2;
        pf->coeff_level_run[ DCT_LUMA_AC] = x264_coeff_level_run15_sse2;
        pf->coeff_level_run[DCT_LUMA_4x4] = x264_coeff_level_run16_sse2;
        if( cpu&X264_CPU_LZCNT )
        {
            pf->coeff_last[ DCT_LUMA_AC] = x264_coeff_last15_sse2_lzcnt;
            pf->coeff_last[DCT_LUMA_4x4] = x264_coeff_last16_sse2_lzcnt;
            pf->coeff_last[DCT_LUMA_8x8] = x264_coeff_last64_sse2_lzcnt;
            pf->coeff_level_run[ DCT_LUMA_AC] = x264_coeff_level_run15_sse2_lzcnt;
            pf->coeff_level_run[DCT_LUMA_4x4] = x264_coeff_level_run16_sse2_lzcnt;
        }
    }

    if( cpu&X264_CPU_SSSE3 )
    {
        pf->quant_2x2_dc = x264_quant_2x2_dc_ssse3;
        pf->quant_4x4_dc = x264_quant_4x4_dc_ssse3;
        pf->quant_4x4 = x264_quant_4x4_ssse3;
        pf->quant_8x8 = x264_quant_8x8_ssse3;
        pf->optimize_chroma_dc = x264_optimize_chroma_dc_ssse3;
        pf->denoise_dct = x264_denoise_dct_ssse3;
        pf->decimate_score15 = x264_decimate_score15_ssse3;
        pf->decimate_score16 = x264_decimate_score16_ssse3;
        if( cpu&X264_CPU_SLOW_CTZ )
        {
            pf->decimate_score15 = x264_decimate_score15_ssse3_slowctz;
            pf->decimate_score16 = x264_decimate_score16_ssse3_slowctz;
        }
        pf->decimate_score64 = x264_decimate_score64_ssse3;
    }

    if( cpu&X264_CPU_SSE4 )
    {
        pf->quant_4x4_dc = x264_quant_4x4_dc_sse4;
        pf->quant_4x4 = x264_quant_4x4_sse4;
        pf->quant_8x8 = x264_quant_8x8_sse4;
        pf->optimize_chroma_dc = x264_optimize_chroma_dc_sse4;
    }

    if( cpu&X264_CPU_AVX )
    {
        pf->dequant_4x4 = x264_dequant_4x4_avx;
        pf->dequant_8x8 = x264_dequant_8x8_avx;
        pf->dequant_4x4_dc = x264_dequant_4x4dc_avx;
        pf->optimize_chroma_dc = x264_optimize_chroma_dc_avx;
        pf->denoise_dct = x264_denoise_dct_avx;
    }
#endif // HAVE_MMX

#if HAVE_ALTIVEC
    if( cpu&X264_CPU_ALTIVEC ) {
        pf->quant_2x2_dc = x264_quant_2x2_dc_altivec;
        pf->quant_4x4_dc = x264_quant_4x4_dc_altivec;
        pf->quant_4x4 = x264_quant_4x4_altivec;
        pf->quant_8x8 = x264_quant_8x8_altivec;

        pf->dequant_4x4 = x264_dequant_4x4_altivec;
        pf->dequant_8x8 = x264_dequant_8x8_altivec;
    }
#endif

#if HAVE_ARMV6
    if( cpu&X264_CPU_ARMV6 )
        pf->coeff_last[DCT_CHROMA_DC] = x264_coeff_last4_arm;

    if( cpu&X264_CPU_NEON )
    {
        pf->quant_2x2_dc   = x264_quant_2x2_dc_neon;
        pf->quant_4x4      = x264_quant_4x4_neon;
        pf->quant_4x4_dc   = x264_quant_4x4_dc_neon;
        pf->quant_8x8      = x264_quant_8x8_neon;
        pf->dequant_4x4    = x264_dequant_4x4_neon;
        pf->dequant_4x4_dc = x264_dequant_4x4_dc_neon;
        pf->dequant_8x8    = x264_dequant_8x8_neon;
        pf->coeff_last[ DCT_LUMA_AC] = x264_coeff_last15_neon;
        pf->coeff_last[DCT_LUMA_4x4] = x264_coeff_last16_neon;
        pf->coeff_last[DCT_LUMA_8x8] = x264_coeff_last64_neon;
    }
#endif
#endif // HIGH_BIT_DEPTH
    pf->coeff_last[DCT_LUMA_DC]     = pf->coeff_last[DCT_CHROMAU_DC]  = pf->coeff_last[DCT_CHROMAV_DC] =
    pf->coeff_last[DCT_CHROMAU_4x4] = pf->coeff_last[DCT_CHROMAV_4x4] = pf->coeff_last[DCT_LUMA_4x4];
    pf->coeff_last[DCT_CHROMA_AC]   = pf->coeff_last[DCT_CHROMAU_AC]  =
    pf->coeff_last[DCT_CHROMAV_AC]  = pf->coeff_last[DCT_LUMA_AC];
    pf->coeff_last[DCT_CHROMAU_8x8] = pf->coeff_last[DCT_CHROMAV_8x8] = pf->coeff_last[DCT_LUMA_8x8];

    pf->coeff_level_run[DCT_LUMA_DC]     = pf->coeff_level_run[DCT_CHROMAU_DC]  = pf->coeff_level_run[DCT_CHROMAV_DC] =
    pf->coeff_level_run[DCT_CHROMAU_4x4] = pf->coeff_level_run[DCT_CHROMAV_4x4] = pf->coeff_level_run[DCT_LUMA_4x4];
    pf->coeff_level_run[DCT_CHROMA_AC]   = pf->coeff_level_run[DCT_CHROMAU_AC]  =
    pf->coeff_level_run[DCT_CHROMAV_AC]  = pf->coeff_level_run[DCT_LUMA_AC];
}

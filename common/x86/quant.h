/*****************************************************************************
 * quant.h: x86 quantization and level-run
 *****************************************************************************
 * Copyright (C) 2005-2010 x264 project
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

#ifndef X264_I386_QUANT_H
#define X264_I386_QUANT_H

int x264_quant_2x2_dc_mmxext( dctcoef dct[4], int mf, int bias );
int x264_quant_4x4_dc_mmxext( dctcoef dct[16], int mf, int bias );
int x264_quant_4x4_mmx( dctcoef dct[16], udctcoef mf[16], udctcoef bias[16] );
int x264_quant_8x8_mmx( dctcoef dct[64], udctcoef mf[64], udctcoef bias[64] );
int x264_quant_2x2_dc_sse2( dctcoef dct[16], int mf, int bias );
int x264_quant_4x4_dc_sse2( dctcoef dct[16], int mf, int bias );
int x264_quant_4x4_sse2( dctcoef dct[16], udctcoef mf[16], udctcoef bias[16] );
int x264_quant_8x8_sse2( dctcoef dct[64], udctcoef mf[64], udctcoef bias[64] );
int x264_quant_2x2_dc_ssse3( dctcoef dct[4], int mf, int bias );
int x264_quant_4x4_dc_ssse3( dctcoef dct[16], int mf, int bias );
int x264_quant_4x4_ssse3( dctcoef dct[16], udctcoef mf[16], udctcoef bias[16] );
int x264_quant_8x8_ssse3( dctcoef dct[64], udctcoef mf[64], udctcoef bias[64] );
int x264_quant_2x2_dc_sse4( dctcoef dct[16], int mf, int bias );
int x264_quant_4x4_dc_sse4( dctcoef dct[16], int mf, int bias );
int x264_quant_4x4_sse4( dctcoef dct[16], udctcoef mf[16], udctcoef bias[16] );
int x264_quant_8x8_sse4( dctcoef dct[64], udctcoef mf[64], udctcoef bias[64] );
void x264_dequant_4x4_mmx( int16_t dct[16], int dequant_mf[6][16], int i_qp );
void x264_dequant_4x4dc_mmxext( int16_t dct[16], int dequant_mf[6][16], int i_qp );
void x264_dequant_8x8_mmx( int16_t dct[64], int dequant_mf[6][64], int i_qp );
void x264_dequant_4x4_sse2( int16_t dct[16], int dequant_mf[6][16], int i_qp );
void x264_dequant_4x4dc_sse2( int16_t dct[16], int dequant_mf[6][16], int i_qp );
void x264_dequant_8x8_sse2( int16_t dct[64], int dequant_mf[6][64], int i_qp );
void x264_dequant_4x4_flat16_mmx( int16_t dct[16], int dequant_mf[6][16], int i_qp );
void x264_dequant_8x8_flat16_mmx( int16_t dct[64], int dequant_mf[6][64], int i_qp );
void x264_dequant_4x4_flat16_sse2( int16_t dct[16], int dequant_mf[6][16], int i_qp );
void x264_dequant_8x8_flat16_sse2( int16_t dct[64], int dequant_mf[6][64], int i_qp );
void x264_denoise_dct_mmx( dctcoef *dct, uint32_t *sum, udctcoef *offset, int size );
void x264_denoise_dct_sse2( dctcoef *dct, uint32_t *sum, udctcoef *offset, int size );
void x264_denoise_dct_ssse3( dctcoef *dct, uint32_t *sum, udctcoef *offset, int size );
int x264_decimate_score15_mmxext( dctcoef *dct );
int x264_decimate_score15_sse2  ( dctcoef *dct );
int x264_decimate_score15_ssse3 ( dctcoef *dct );
int x264_decimate_score16_mmxext( dctcoef *dct );
int x264_decimate_score16_sse2  ( dctcoef *dct );
int x264_decimate_score16_ssse3 ( dctcoef *dct );
int x264_decimate_score15_mmxext_slowctz( dctcoef *dct );
int x264_decimate_score15_sse2_slowctz  ( dctcoef *dct );
int x264_decimate_score15_ssse3_slowctz ( dctcoef *dct );
int x264_decimate_score16_mmxext_slowctz( dctcoef *dct );
int x264_decimate_score16_sse2_slowctz  ( dctcoef *dct );
int x264_decimate_score16_ssse3_slowctz ( dctcoef *dct );
int x264_decimate_score64_mmxext( dctcoef *dct );
int x264_decimate_score64_sse2  ( dctcoef *dct );
int x264_decimate_score64_ssse3 ( dctcoef *dct );
int x264_coeff_last4_mmxext( dctcoef *dct );
int x264_coeff_last15_mmxext( dctcoef *dct );
int x264_coeff_last16_mmxext( dctcoef *dct );
int x264_coeff_last64_mmxext( dctcoef *dct );
int x264_coeff_last15_sse2( dctcoef *dct );
int x264_coeff_last16_sse2( dctcoef *dct );
int x264_coeff_last64_sse2( dctcoef *dct );
int x264_coeff_last4_mmxext_lzcnt( dctcoef *dct );
int x264_coeff_last15_sse2_lzcnt( dctcoef *dct );
int x264_coeff_last16_sse2_lzcnt( dctcoef *dct );
int x264_coeff_last64_sse2_lzcnt( dctcoef *dct );
int x264_coeff_level_run16_mmxext( dctcoef *dct, x264_run_level_t *runlevel );
int x264_coeff_level_run16_sse2( dctcoef *dct, x264_run_level_t *runlevel );
int x264_coeff_level_run16_sse2_lzcnt( dctcoef *dct, x264_run_level_t *runlevel );
int x264_coeff_level_run15_mmxext( dctcoef *dct, x264_run_level_t *runlevel );
int x264_coeff_level_run15_sse2( dctcoef *dct, x264_run_level_t *runlevel );
int x264_coeff_level_run15_sse2_lzcnt( dctcoef *dct, x264_run_level_t *runlevel );
int x264_coeff_level_run4_mmxext( dctcoef *dct, x264_run_level_t *runlevel );
int x264_coeff_level_run4_mmxext_lzcnt( dctcoef *dct, x264_run_level_t *runlevel );

#endif

/*****************************************************************************
 * quant.h: h264 encoder library
 *****************************************************************************
 * Copyright (C) 2005-2008 x264 project
 *
 * Authors: Christian Heine <sennindemokrit@gmx.net>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111, USA.
 *****************************************************************************/

#ifndef X264_I386_QUANT_H
#define X264_I386_QUANT_H

int x264_quant_2x2_dc_mmxext( int16_t dct[2][2], int mf, int bias );
int x264_quant_4x4_dc_mmxext( int16_t dct[4][4], int mf, int bias );
int x264_quant_4x4_mmx( int16_t dct[4][4], uint16_t mf[16], uint16_t bias[16] );
int x264_quant_8x8_mmx( int16_t dct[8][8], uint16_t mf[64], uint16_t bias[64] );
int x264_quant_4x4_dc_sse2( int16_t dct[4][4], int mf, int bias );
int x264_quant_4x4_sse2( int16_t dct[4][4], uint16_t mf[16], uint16_t bias[16] );
int x264_quant_8x8_sse2( int16_t dct[8][8], uint16_t mf[64], uint16_t bias[64] );
int x264_quant_2x2_dc_ssse3( int16_t dct[2][2], int mf, int bias );
int x264_quant_4x4_dc_ssse3( int16_t dct[4][4], int mf, int bias );
int x264_quant_4x4_ssse3( int16_t dct[4][4], uint16_t mf[16], uint16_t bias[16] );
int x264_quant_8x8_ssse3( int16_t dct[8][8], uint16_t mf[64], uint16_t bias[64] );
int x264_quant_4x4_dc_sse4( int16_t dct[4][4], int mf, int bias );
int x264_quant_4x4_sse4( int16_t dct[4][4], uint16_t mf[16], uint16_t bias[16] );
int x264_quant_8x8_sse4( int16_t dct[8][8], uint16_t mf[64], uint16_t bias[64] );
void x264_dequant_4x4_mmx( int16_t dct[4][4], int dequant_mf[6][4][4], int i_qp );
void x264_dequant_4x4dc_mmxext( int16_t dct[4][4], int dequant_mf[6][4][4], int i_qp );
void x264_dequant_8x8_mmx( int16_t dct[8][8], int dequant_mf[6][8][8], int i_qp );
void x264_dequant_4x4_sse2( int16_t dct[4][4], int dequant_mf[6][4][4], int i_qp );
void x264_dequant_4x4dc_sse2( int16_t dct[4][4], int dequant_mf[6][4][4], int i_qp );
void x264_dequant_8x8_sse2( int16_t dct[8][8], int dequant_mf[6][8][8], int i_qp );
void x264_dequant_4x4_flat16_mmx( int16_t dct[4][4], int dequant_mf[6][4][4], int i_qp );
void x264_dequant_8x8_flat16_mmx( int16_t dct[8][8], int dequant_mf[6][8][8], int i_qp );
void x264_dequant_4x4_flat16_sse2( int16_t dct[4][4], int dequant_mf[6][4][4], int i_qp );
void x264_dequant_8x8_flat16_sse2( int16_t dct[8][8], int dequant_mf[6][8][8], int i_qp );
void x264_denoise_dct_mmx( int16_t *dct, uint32_t *sum, uint16_t *offset, int size );
void x264_denoise_dct_sse2( int16_t *dct, uint32_t *sum, uint16_t *offset, int size );
void x264_denoise_dct_ssse3( int16_t *dct, uint32_t *sum, uint16_t *offset, int size );
int x264_decimate_score15_mmxext( int16_t *dct );
int x264_decimate_score15_sse2  ( int16_t *dct );
int x264_decimate_score15_ssse3 ( int16_t *dct );
int x264_decimate_score16_mmxext( int16_t *dct );
int x264_decimate_score16_sse2  ( int16_t *dct );
int x264_decimate_score16_ssse3 ( int16_t *dct );
int x264_decimate_score64_mmxext( int16_t *dct );
int x264_decimate_score64_sse2  ( int16_t *dct );
int x264_decimate_score64_ssse3 ( int16_t *dct );
int x264_coeff_last4_mmxext( int16_t *dct );
int x264_coeff_last15_mmxext( int16_t *dct );
int x264_coeff_last16_mmxext( int16_t *dct );
int x264_coeff_last64_mmxext( int16_t *dct );
int x264_coeff_last15_sse2( int16_t *dct );
int x264_coeff_last16_sse2( int16_t *dct );
int x264_coeff_last64_sse2( int16_t *dct );
int x264_coeff_last4_mmxext_lzcnt( int16_t *dct );
int x264_coeff_last15_sse2_lzcnt( int16_t *dct );
int x264_coeff_last16_sse2_lzcnt( int16_t *dct );
int x264_coeff_last64_sse2_lzcnt( int16_t *dct );
int x264_coeff_level_run16_mmxext( int16_t *dct, x264_run_level_t *runlevel );
int x264_coeff_level_run16_sse2( int16_t *dct, x264_run_level_t *runlevel );
int x264_coeff_level_run16_sse2_lzcnt( int16_t *dct, x264_run_level_t *runlevel );
int x264_coeff_level_run15_mmxext( int16_t *dct, x264_run_level_t *runlevel );
int x264_coeff_level_run15_sse2( int16_t *dct, x264_run_level_t *runlevel );
int x264_coeff_level_run15_sse2_lzcnt( int16_t *dct, x264_run_level_t *runlevel );
int x264_coeff_level_run4_mmxext( int16_t *dct, x264_run_level_t *runlevel );
int x264_coeff_level_run4_mmxext_lzcnt( int16_t *dct, x264_run_level_t *runlevel );

#endif

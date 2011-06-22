/*****************************************************************************
 * quant.h: quantization and level-run
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

#ifndef X264_QUANT_H
#define X264_QUANT_H

typedef struct
{
    int (*quant_8x8)( dctcoef dct[64], udctcoef mf[64], udctcoef bias[64] );
    int (*quant_4x4)( dctcoef dct[16], udctcoef mf[16], udctcoef bias[16] );
    int (*quant_4x4_dc)( dctcoef dct[16], int mf, int bias );
    int (*quant_2x2_dc)( dctcoef dct[4], int mf, int bias );

    void (*dequant_8x8)( dctcoef dct[64], int dequant_mf[6][64], int i_qp );
    void (*dequant_4x4)( dctcoef dct[16], int dequant_mf[6][16], int i_qp );
    void (*dequant_4x4_dc)( dctcoef dct[16], int dequant_mf[6][16], int i_qp );

    int (*optimize_chroma_dc)( dctcoef dct[4], int dequant_mf );

    void (*denoise_dct)( dctcoef *dct, uint32_t *sum, udctcoef *offset, int size );

    int (*decimate_score15)( dctcoef *dct );
    int (*decimate_score16)( dctcoef *dct );
    int (*decimate_score64)( dctcoef *dct );
    int (*coeff_last[14])( dctcoef *dct );
    int (*coeff_level_run[13])( dctcoef *dct, x264_run_level_t *runlevel );
} x264_quant_function_t;

void x264_quant_init( x264_t *h, int cpu, x264_quant_function_t *pf );

#endif

/*****************************************************************************
 * quant.h: h264 encoder library
 *****************************************************************************
 * Copyright (C) 2005-2008 x264 project
 *
 * Authors: Loren Merritt <lorenm@u.washington.edu>
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

#ifndef X264_QUANT_H
#define X264_QUANT_H

typedef struct
{
    int (*quant_8x8)( int16_t dct[8][8], uint16_t mf[64], uint16_t bias[64] );
    int (*quant_4x4)( int16_t dct[4][4], uint16_t mf[16], uint16_t bias[16] );
    int (*quant_4x4_dc)( int16_t dct[4][4], int mf, int bias );
    int (*quant_2x2_dc)( int16_t dct[2][2], int mf, int bias );

    void (*dequant_8x8)( int16_t dct[8][8], int dequant_mf[6][8][8], int i_qp );
    void (*dequant_4x4)( int16_t dct[4][4], int dequant_mf[6][4][4], int i_qp );
    void (*dequant_4x4_dc)( int16_t dct[4][4], int dequant_mf[6][4][4], int i_qp );

    void (*denoise_dct)( int16_t *dct, uint32_t *sum, uint16_t *offset, int size );

    int (*decimate_score15)( int16_t *dct );
    int (*decimate_score16)( int16_t *dct );
    int (*decimate_score64)( int16_t *dct );
    int (*coeff_last[6])( int16_t *dct );
    int (*coeff_level_run[5])( int16_t *dct, x264_run_level_t *runlevel );
} x264_quant_function_t;

void x264_quant_init( x264_t *h, int cpu, x264_quant_function_t *pf );

#endif

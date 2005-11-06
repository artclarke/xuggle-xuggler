/*****************************************************************************
 * quant.h: h264 encoder library
 *****************************************************************************
 * Copyright (C) 2005 x264 project
 *
 * Authors: Christian Heine <sennindemokrit@gmx.net>
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

#ifndef _I386_QUANT_H
#define _I386_QUANT_H 1

void x264_quant_8x8_core15_mmx( int16_t dct[8][8],
    int quant_mf[8][8], int const i_qbits, int const f );
void x264_quant_4x4_core15_mmx( int16_t dct[4][4],
    int quant_mf[4][4], int const i_qbits, int const f );
void x264_quant_4x4_dc_core15_mmx( int16_t dct[4][4],
    int const i_qmf, int const i_qbits, int const f );
void x264_quant_2x2_dc_core15_mmx( int16_t dct[2][2],
    int const i_qmf, int const i_qbits, int const f );

void x264_quant_8x8_core16_mmxext( int16_t dct[8][8],
    int quant_mf[8][8], int const i_qbits, int const f );
void x264_quant_4x4_core16_mmxext( int16_t dct[4][4],
    int quant_mf[4][4], int const i_qbits, int const f );
void x264_quant_4x4_dc_core16_mmxext( int16_t dct[4][4],
    int const i_qmf, int const i_qbits, int const f );
void x264_quant_2x2_dc_core16_mmxext( int16_t dct[2][2],
    int const i_qmf, int const i_qbits, int const f );

void x264_quant_8x8_core32_mmxext( int16_t dct[8][8],
    int quant_mf[8][8], int const i_qbits, int const f );
void x264_quant_4x4_core32_mmxext( int16_t dct[4][4],
    int quant_mf[4][4], int const i_qbits, int const f );
void x264_quant_4x4_dc_core32_mmxext( int16_t dct[4][4],
    int const i_qmf, int const i_qbits, int const f );
void x264_quant_2x2_dc_core32_mmxext( int16_t dct[2][2],
    int const i_qmf, int const i_qbits, int const f );

void x264_dequant_4x4_mmx( int16_t dct[4][4], int dequant_mf[6][4][4], int i_qp );
void x264_dequant_8x8_mmx( int16_t dct[8][8], int dequant_mf[6][8][8], int i_qp );

#endif

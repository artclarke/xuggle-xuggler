/*****************************************************************************
* quant.h: h264 encoder library
*****************************************************************************
* Copyright (C) 2007 Guillaume Poirier <gpoirier@mplayerhq.hu>
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

#ifndef X264_PPC_QUANT_H
#define X264_PPC_QUANT_H

int x264_quant_4x4_altivec( int16_t dct[4][4], uint16_t mf[16], uint16_t bias[16] );
int x264_quant_8x8_altivec( int16_t dct[8][8], uint16_t mf[64], uint16_t bias[64] );

int x264_quant_4x4_dc_altivec( int16_t dct[4][4], int mf, int bias );
int x264_quant_2x2_dc_altivec( int16_t dct[2][2], int mf, int bias );

void x264_dequant_4x4_altivec( int16_t dct[4][4], int dequant_mf[6][4][4], int i_qp );
void x264_dequant_8x8_altivec( int16_t dct[8][8], int dequant_mf[6][8][8], int i_qp );
#endif

/*****************************************************************************
 * dct.h: h264 encoder library
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar
 * $Id: dct.h,v 1.1 2004/06/03 19:27:07 fenrir Exp $
 *
 * Authors: Laurent Aimar <fenrir@via.ecp.fr>
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

#ifndef _I386_DCT_H
#define _I386_DCT_H 1

void x264_sub4x4_dct_mmx( int16_t dct[4][4],  uint8_t *pix1, uint8_t *pix2 );
void x264_sub8x8_dct_mmx( int16_t dct[4][4][4],  uint8_t *pix1, uint8_t *pix2 );
void x264_sub16x16_dct_mmx( int16_t dct[16][4][4],  uint8_t *pix1, uint8_t *pix2 );

void x264_add4x4_idct_mmx( uint8_t *p_dst, int16_t dct[4][4] );
void x264_add8x8_idct_mmx( uint8_t *p_dst, int16_t dct[4][4][4] );
void x264_add16x16_idct_mmx( uint8_t *p_dst, int16_t dct[16][4][4] );

void x264_dct4x4dc_mmx( int16_t d[4][4] );
void x264_idct4x4dc_mmx( int16_t d[4][4] );

void x264_sub8x8_dct8_mmx( int16_t dct[8][8], uint8_t *pix1, uint8_t *pix2 );
void x264_sub16x16_dct8_mmx( int16_t dct[4][8][8], uint8_t *pix1, uint8_t *pix2 );

void x264_add8x8_idct8_mmx( uint8_t *dst, int16_t dct[8][8] );
void x264_add16x16_idct8_mmx( uint8_t *dst, int16_t dct[4][8][8] );

void x264_sub8x8_dct8_sse2( int16_t dct[8][8],  uint8_t *pix1, uint8_t *pix2 );
void x264_sub16x16_dct8_sse2( int16_t dct[4][8][8], uint8_t *pix1, uint8_t *pix2 );

void x264_add8x8_idct8_sse2( uint8_t *dst, int16_t dct[8][8] );
void x264_add16x16_idct8_sse2( uint8_t *dst, int16_t dct[4][8][8] );

void x264_zigzag_scan_4x4_field_sse2( int level[16], int16_t dct[4][4] );
void x264_zigzag_scan_4x4_field_mmx( int level[16], int16_t dct[4][4] );

#endif

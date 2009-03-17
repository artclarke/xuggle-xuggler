/*****************************************************************************
 * dct.h: h264 encoder library
 *****************************************************************************
 * Copyright (C) 2003-2008 x264 project
 *
 * Authors: Loren Merritt <lorenm@u.washington.edu>
 *          Laurent Aimar <fenrir@via.ecp.fr>
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

#ifndef X264_I386_DCT_H
#define X264_I386_DCT_H

void x264_sub4x4_dct_mmx     ( int16_t dct[ 4][4]   ,  uint8_t *pix1, uint8_t *pix2 );
void x264_sub8x8_dct_mmx     ( int16_t dct[ 4][4][4],  uint8_t *pix1, uint8_t *pix2 );
void x264_sub16x16_dct_mmx   ( int16_t dct[16][4][4],  uint8_t *pix1, uint8_t *pix2 );
void x264_sub8x8_dct_sse2    ( int16_t dct[ 4][4][4],  uint8_t *pix1, uint8_t *pix2 );
void x264_sub16x16_dct_sse2  ( int16_t dct[16][4][4],  uint8_t *pix1, uint8_t *pix2 );
void x264_sub4x4_dct_ssse3   ( int16_t dct[ 4][4]   ,  uint8_t *pix1, uint8_t *pix2 );
void x264_sub8x8_dct_ssse3   ( int16_t dct[ 4][4][4],  uint8_t *pix1, uint8_t *pix2 );
void x264_sub16x16_dct_ssse3 ( int16_t dct[16][4][4],  uint8_t *pix1, uint8_t *pix2 );


void x264_add4x4_idct_mmx    ( uint8_t *p_dst, int16_t dct[ 4][4]    );
void x264_add8x8_idct_mmx    ( uint8_t *p_dst, int16_t dct[ 4][4][4] );
void x264_add8x8_idct_dc_mmx ( uint8_t *p_dst, int16_t dct[2][2] );
void x264_add16x16_idct_mmx  ( uint8_t *p_dst, int16_t dct[16][4][4] );
void x264_add16x16_idct_dc_mmx ( uint8_t *p_dst, int16_t dct[4][4] );
void x264_add8x8_idct_sse2   ( uint8_t *p_dst, int16_t dct[ 4][4][4] );
void x264_add16x16_idct_sse2 ( uint8_t *p_dst, int16_t dct[16][4][4] );
void x264_add16x16_idct_dc_sse2( uint8_t *p_dst, int16_t dct[4][4] );
void x264_add8x8_idct_dc_ssse3( uint8_t *p_dst, int16_t dct[2][2] );
void x264_add16x16_idct_dc_ssse3( uint8_t *p_dst, int16_t dct[4][4] );

void x264_dct4x4dc_mmx       ( int16_t d[4][4] );
void x264_idct4x4dc_mmx      ( int16_t d[4][4] );

void x264_sub8x8_dct8_mmx    ( int16_t dct[8][8]   , uint8_t *pix1, uint8_t *pix2 );
void x264_sub16x16_dct8_mmx  ( int16_t dct[4][8][8], uint8_t *pix1, uint8_t *pix2 );
void x264_sub8x8_dct8_sse2   ( int16_t dct[8][8]   , uint8_t *pix1, uint8_t *pix2 );
void x264_sub16x16_dct8_sse2 ( int16_t dct[4][8][8], uint8_t *pix1, uint8_t *pix2 );
void x264_sub8x8_dct8_ssse3  ( int16_t dct[8][8]   , uint8_t *pix1, uint8_t *pix2 );
void x264_sub16x16_dct8_ssse3( int16_t dct[4][8][8], uint8_t *pix1, uint8_t *pix2 );


void x264_add8x8_idct8_mmx   ( uint8_t *dst, int16_t dct[8][8]    );
void x264_add16x16_idct8_mmx ( uint8_t *dst, int16_t dct[4][8][8] );
void x264_add8x8_idct8_sse2  ( uint8_t *dst, int16_t dct[8][8]    );
void x264_add16x16_idct8_sse2( uint8_t *dst, int16_t dct[4][8][8] );

void x264_zigzag_scan_8x8_frame_ssse3 ( int16_t level[64], int16_t dct[8][8] );
void x264_zigzag_scan_8x8_frame_sse2  ( int16_t level[64], int16_t dct[8][8] );
void x264_zigzag_scan_8x8_frame_mmxext( int16_t level[64], int16_t dct[8][8] );
void x264_zigzag_scan_4x4_frame_ssse3 ( int16_t level[16], int16_t dct[4][4] );
void x264_zigzag_scan_4x4_frame_mmx   ( int16_t level[16], int16_t dct[4][4] );
void x264_zigzag_scan_4x4_field_mmxext( int16_t level[16], int16_t dct[4][4] );
void x264_zigzag_sub_4x4_frame_ssse3  ( int16_t level[16], const uint8_t *src, uint8_t *dst );
void x264_zigzag_interleave_8x8_cavlc_mmx( int16_t *dst, int16_t *src, uint8_t *nnz );
void x264_zigzag_interleave_8x8_cavlc_sse2( int16_t *dst, int16_t *src, uint8_t *nnz );

#endif

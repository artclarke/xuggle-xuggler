/*****************************************************************************
 * dct.h: h264 encoder library
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar
 * $Id: dct.h,v 1.1 2004/06/03 19:27:06 fenrir Exp $
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

#ifndef _DCT_H
#define _DCT_H 1

typedef struct
{
    void (*sub4x4_dct)   ( int16_t dct[4][4],  uint8_t *pix1, int i_pix1, uint8_t *pix2, int i_pix2 );
    void (*add4x4_idct)  ( uint8_t *p_dst, int i_dst, int16_t dct[4][4] );

    void (*sub8x8_dct)   ( int16_t dct[4][4][4],  uint8_t *pix1, int i_pix1, uint8_t *pix2, int i_pix2 );
    void (*add8x8_idct)  ( uint8_t *p_dst, int i_dst, int16_t dct[4][4][4] );

    void (*sub16x16_dct)   ( int16_t dct[16][4][4],  uint8_t *pix1, int i_pix1, uint8_t *pix2, int i_pix2 );
    void (*add16x16_idct)  ( uint8_t *p_dst, int i_dst, int16_t dct[16][4][4] );

    void (*sub8x8_dct8)   ( int16_t dct[8][8],  uint8_t *pix1, int i_pix1, uint8_t *pix2, int i_pix2 );
    void (*add8x8_idct8)  ( uint8_t *p_dst, int i_dst, int16_t dct[8][8] );

    void (*sub16x16_dct8)   ( int16_t dct[4][8][8],  uint8_t *pix1, int i_pix1, uint8_t *pix2, int i_pix2 );
    void (*add16x16_idct8)  ( uint8_t *p_dst, int i_dst, int16_t dct[4][8][8] );

    void (*dct4x4dc) ( int16_t d[4][4] );
    void (*idct4x4dc)( int16_t d[4][4] );

    void (*dct2x2dc) ( int16_t d[2][2] );
    void (*idct2x2dc)( int16_t d[2][2] );

} x264_dct_function_t;

void x264_dct_init( int cpu, x264_dct_function_t *dctf );

#endif

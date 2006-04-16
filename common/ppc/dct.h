/*****************************************************************************
 * dct.h: h264 encoder library
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar
 * $Id$
 *
 * Authors: Eric Petit <titer@m0k.org>
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

#ifndef _PPC_DCT_H
#define _PPC_DCT_H 1

void x264_sub4x4_dct_altivec( int16_t dct[4][4],
        uint8_t *pix1, uint8_t *pix2 );
void x264_sub8x8_dct_altivec( int16_t dct[4][4][4],
        uint8_t *pix1, uint8_t *pix2 );
void x264_sub16x16_dct_altivec( int16_t dct[16][4][4],
        uint8_t *pix1, uint8_t *pix2 );

#endif

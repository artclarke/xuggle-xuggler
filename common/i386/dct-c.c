/*****************************************************************************
 * dct.c: h264 encoder library
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar
 * $Id: dct-c.c,v 1.1 2004/06/03 19:27:07 fenrir Exp $
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

#ifdef HAVE_STDINT_H
#include <stdint.h>
#else
#include <inttypes.h>
#endif
#include <stdlib.h>
#include <stdarg.h>

#include "x264.h"

#include "dct.h"


void x264_sub8x8_dct_mmxext( int16_t dct[4][4][4], uint8_t *pix1, int i_pix1, uint8_t *pix2, int i_pix2 )
{
    x264_sub4x4_dct_mmxext( dct[0], &pix1[0], i_pix1, &pix2[0], i_pix2 );
    x264_sub4x4_dct_mmxext( dct[1], &pix1[4], i_pix1, &pix2[4], i_pix2 );
    x264_sub4x4_dct_mmxext( dct[2], &pix1[4*i_pix1+0], i_pix1, &pix2[4*i_pix2+0], i_pix2 );
    x264_sub4x4_dct_mmxext( dct[3], &pix1[4*i_pix1+4], i_pix1, &pix2[4*i_pix2+4], i_pix2 );
}

void x264_sub16x16_dct_mmxext( int16_t dct[16][4][4], uint8_t *pix1, int i_pix1, uint8_t *pix2, int i_pix2 )
{
    x264_sub8x8_dct_mmxext( &dct[ 0], &pix1[0], i_pix1, &pix2[0], i_pix2 );
    x264_sub8x8_dct_mmxext( &dct[ 4], &pix1[8], i_pix1, &pix2[8], i_pix2 );
    x264_sub8x8_dct_mmxext( &dct[ 8], &pix1[8*i_pix1], i_pix1, &pix2[8*i_pix2], i_pix2 );
    x264_sub8x8_dct_mmxext( &dct[12], &pix1[8*i_pix1+8], i_pix1, &pix2[8*i_pix2+8], i_pix2 );
}



/****************************************************************************
 * addXxX_idct:
 ****************************************************************************/

void x264_add8x8_idct_mmxext( uint8_t *p_dst, int i_dst, int16_t dct[4][4][4] )
{
    x264_add4x4_idct_mmxext( p_dst, i_dst,             dct[0] );
    x264_add4x4_idct_mmxext( &p_dst[4], i_dst,         dct[1] );
    x264_add4x4_idct_mmxext( &p_dst[4*i_dst+0], i_dst, dct[2] );
    x264_add4x4_idct_mmxext( &p_dst[4*i_dst+4], i_dst, dct[3] );
}

void x264_add16x16_idct_mmxext( uint8_t *p_dst, int i_dst, int16_t dct[16][4][4] )
{
    x264_add8x8_idct_mmxext( &p_dst[0], i_dst, &dct[0] );
    x264_add8x8_idct_mmxext( &p_dst[8], i_dst, &dct[4] );
    x264_add8x8_idct_mmxext( &p_dst[8*i_dst], i_dst, &dct[8] );
    x264_add8x8_idct_mmxext( &p_dst[8*i_dst+8], i_dst, &dct[12] );
}

/***********************
 * dct8/idct8 functions
 ***********************/

#ifdef ARCH_X86_64
void x264_sub16x16_dct8_sse2( int16_t dct[4][8][8], uint8_t *pix1, int i_pix1, uint8_t *pix2, int i_pix2 )
{
    x264_sub8x8_dct8_sse2( dct[0], pix1,            i_pix1, pix2,            i_pix2 );
    x264_sub8x8_dct8_sse2( dct[1], pix1+8,          i_pix1, pix2+8,          i_pix2 );
    x264_sub8x8_dct8_sse2( dct[2], pix1+8*i_pix1,   i_pix1, pix2+8*i_pix2,   i_pix2 );
    x264_sub8x8_dct8_sse2( dct[3], pix1+8*i_pix1+8, i_pix1, pix2+8*i_pix2+8, i_pix2 );
}

void x264_add16x16_idct8_sse2( uint8_t *p_dst, int i_dst, int16_t dct[4][8][8] )
{
    x264_add8x8_idct8_sse2( p_dst,           i_dst, dct[0] );
    x264_add8x8_idct8_sse2( p_dst+8,         i_dst, dct[1] );
    x264_add8x8_idct8_sse2( p_dst+8*i_dst,   i_dst, dct[2] );
    x264_add8x8_idct8_sse2( p_dst+8*i_dst+8, i_dst, dct[3] );
}

#else // ARCH_X86

void x264_pixel_sub_8x8_mmx( int16_t *diff, uint8_t *pix1, int i_pix1, uint8_t *pix2, int i_pix2 );
void x264_pixel_add_8x8_mmx( uint8_t *pix, int i_pix, uint16_t *diff );
void x264_xdct8_mmxext( int16_t dct[8][8] );
void x264_xidct8_mmxext( int16_t dct[8][8] );
void x264_ydct8_mmx( int16_t dct[8][8] );
void x264_yidct8_mmx( int16_t dct[8][8] );       // including >>6 at the end

inline void x264_sub8x8_dct8_mmxext( int16_t dct[8][8], uint8_t *pix1, int i_pix1, uint8_t *pix2, int i_pix2 )
{
    x264_pixel_sub_8x8_mmx( (int16_t *)dct, pix1, i_pix1, pix2, i_pix2 );
    x264_xdct8_mmxext( dct );
    x264_ydct8_mmx( dct );
}

void x264_sub16x16_dct8_mmxext( int16_t dct[4][8][8], uint8_t *pix1, int i_pix1, uint8_t *pix2, int i_pix2 )
{
    x264_sub8x8_dct8_mmxext( dct[0], pix1,            i_pix1, pix2,            i_pix2 );
    x264_sub8x8_dct8_mmxext( dct[1], pix1+8,          i_pix1, pix2+8,          i_pix2 );
    x264_sub8x8_dct8_mmxext( dct[2], pix1+8*i_pix1,   i_pix1, pix2+8*i_pix2,   i_pix2 );
    x264_sub8x8_dct8_mmxext( dct[3], pix1+8*i_pix1+8, i_pix1, pix2+8*i_pix2+8, i_pix2 );
}

inline void x264_add8x8_idct8_mmxext( uint8_t *dst, int i_dst, int16_t dct[8][8] )
{
    dct[0][0] += 32;
    x264_xidct8_mmxext( dct );
    x264_yidct8_mmx( dct );
    x264_pixel_add_8x8_mmx( dst, i_dst, (uint16_t *)dct );
}

void x264_add16x16_idct8_mmxext( uint8_t *dst, int i_dst, int16_t dct[4][8][8] )
{
    x264_add8x8_idct8_mmxext( dst,           i_dst, dct[0] );
    x264_add8x8_idct8_mmxext( dst+8,         i_dst, dct[1] );
    x264_add8x8_idct8_mmxext( dst+8*i_dst,   i_dst, dct[2] );
    x264_add8x8_idct8_mmxext( dst+8*i_dst+8, i_dst, dct[3] );
}
#endif

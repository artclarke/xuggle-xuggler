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

#include "dct.h"
#include "common/common.h"


void x264_sub8x8_dct_mmx( int16_t dct[4][4][4], uint8_t *pix1, uint8_t *pix2 )
{
    x264_sub4x4_dct_mmx( dct[0], &pix1[0], &pix2[0] );
    x264_sub4x4_dct_mmx( dct[1], &pix1[4], &pix2[4] );
    x264_sub4x4_dct_mmx( dct[2], &pix1[4*FENC_STRIDE+0], &pix2[4*FDEC_STRIDE+0] );
    x264_sub4x4_dct_mmx( dct[3], &pix1[4*FENC_STRIDE+4], &pix2[4*FDEC_STRIDE+4] );
}

void x264_sub16x16_dct_mmx( int16_t dct[16][4][4], uint8_t *pix1, uint8_t *pix2 )
{
    x264_sub8x8_dct_mmx( &dct[ 0], &pix1[0], &pix2[0] );
    x264_sub8x8_dct_mmx( &dct[ 4], &pix1[8], &pix2[8] );
    x264_sub8x8_dct_mmx( &dct[ 8], &pix1[8*FENC_STRIDE+0], &pix2[8*FDEC_STRIDE+0] );
    x264_sub8x8_dct_mmx( &dct[12], &pix1[8*FENC_STRIDE+8], &pix2[8*FDEC_STRIDE+8] );
}



/****************************************************************************
 * addXxX_idct:
 ****************************************************************************/

void x264_add8x8_idct_mmx( uint8_t *p_dst, int16_t dct[4][4][4] )
{
    x264_add4x4_idct_mmx( p_dst,                   dct[0] );
    x264_add4x4_idct_mmx( &p_dst[4],               dct[1] );
    x264_add4x4_idct_mmx( &p_dst[4*FDEC_STRIDE+0], dct[2] );
    x264_add4x4_idct_mmx( &p_dst[4*FDEC_STRIDE+4], dct[3] );
}

void x264_add16x16_idct_mmx( uint8_t *p_dst, int16_t dct[16][4][4] )
{
    x264_add8x8_idct_mmx( &p_dst[0],               &dct[0] );
    x264_add8x8_idct_mmx( &p_dst[8],               &dct[4] );
    x264_add8x8_idct_mmx( &p_dst[8*FDEC_STRIDE],   &dct[8] );
    x264_add8x8_idct_mmx( &p_dst[8*FDEC_STRIDE+8], &dct[12] );
}

/***********************
 * dct8/idct8 functions
 ***********************/

#ifdef ARCH_X86_64
void x264_sub16x16_dct8_sse2( int16_t dct[4][8][8], uint8_t *pix1, uint8_t *pix2 )
{
    x264_sub8x8_dct8_sse2( dct[0], pix1,                 pix2 );
    x264_sub8x8_dct8_sse2( dct[1], pix1+8,               pix2+8 );
    x264_sub8x8_dct8_sse2( dct[2], pix1+8*FENC_STRIDE,   pix2+8*FDEC_STRIDE );
    x264_sub8x8_dct8_sse2( dct[3], pix1+8*FENC_STRIDE+8, pix2+8*FDEC_STRIDE+8 );
}

void x264_add16x16_idct8_sse2( uint8_t *p_dst, int16_t dct[4][8][8] )
{
    x264_add8x8_idct8_sse2( p_dst,                 dct[0] );
    x264_add8x8_idct8_sse2( p_dst+8,               dct[1] );
    x264_add8x8_idct8_sse2( p_dst+8*FDEC_STRIDE,   dct[2] );
    x264_add8x8_idct8_sse2( p_dst+8*FDEC_STRIDE+8, dct[3] );
}

#else // ARCH_X86

void x264_pixel_sub_8x8_mmx( int16_t *diff, uint8_t *pix1, uint8_t *pix2 );
void x264_pixel_add_8x8_mmx( uint8_t *pix, uint16_t *diff );
void x264_transpose_8x8_mmx( int16_t src[8][8] );
void x264_ydct8_mmx( int16_t dct[8][8] );
void x264_yidct8_mmx( int16_t dct[8][8] );

inline void x264_sub8x8_dct8_mmx( int16_t dct[8][8], uint8_t *pix1, uint8_t *pix2 )
{
    x264_pixel_sub_8x8_mmx( (int16_t *)dct, pix1, pix2 );
    x264_ydct8_mmx( dct );
    x264_transpose_8x8_mmx( dct );
    x264_ydct8_mmx( dct );
}

void x264_sub16x16_dct8_mmx( int16_t dct[4][8][8], uint8_t *pix1, uint8_t *pix2 )
{
    x264_sub8x8_dct8_mmx( dct[0], pix1,                 pix2 );
    x264_sub8x8_dct8_mmx( dct[1], pix1+8,               pix2+8 );
    x264_sub8x8_dct8_mmx( dct[2], pix1+8*FENC_STRIDE,   pix2+8*FDEC_STRIDE );
    x264_sub8x8_dct8_mmx( dct[3], pix1+8*FENC_STRIDE+8, pix2+8*FDEC_STRIDE+8 );
}

inline void x264_add8x8_idct8_mmx( uint8_t *dst, int16_t dct[8][8] )
{
    dct[0][0] += 32;
    x264_yidct8_mmx( dct );
    x264_transpose_8x8_mmx( dct );
    x264_yidct8_mmx( dct );
    x264_pixel_add_8x8_mmx( dst, (uint16_t *)dct ); // including >>6 at the end
}

void x264_add16x16_idct8_mmx( uint8_t *dst, int16_t dct[4][8][8] )
{
    x264_add8x8_idct8_mmx( dst,                 dct[0] );
    x264_add8x8_idct8_mmx( dst+8,               dct[1] );
    x264_add8x8_idct8_mmx( dst+8*FDEC_STRIDE,   dct[2] );
    x264_add8x8_idct8_mmx( dst+8*FDEC_STRIDE+8, dct[3] );
}
#endif

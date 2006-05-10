/*****************************************************************************
 * mc.h: h264 encoder library
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar
 * $Id: pixel.h,v 1.1 2004/06/03 19:27:07 fenrir Exp $
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

#ifndef _I386_PIXEL_H
#define _I386_PIXEL_H 1

int x264_pixel_sad_16x16_mmxext( uint8_t *, int, uint8_t *, int );
int x264_pixel_sad_16x8_mmxext( uint8_t *, int, uint8_t *, int );
int x264_pixel_sad_8x16_mmxext( uint8_t *, int, uint8_t *, int );
int x264_pixel_sad_8x8_mmxext( uint8_t *, int, uint8_t *, int );
int x264_pixel_sad_8x4_mmxext( uint8_t *, int, uint8_t *, int );
int x264_pixel_sad_4x8_mmxext( uint8_t *, int, uint8_t *, int );
int x264_pixel_sad_4x4_mmxext( uint8_t *, int, uint8_t *, int );

void x264_pixel_sad_x3_16x16_mmxext( uint8_t *, uint8_t *, uint8_t *, uint8_t *, int, int * );
void x264_pixel_sad_x3_16x8_mmxext( uint8_t *, uint8_t *, uint8_t *, uint8_t *, int, int * );
void x264_pixel_sad_x3_8x16_mmxext( uint8_t *, uint8_t *, uint8_t *, uint8_t *, int, int * );
void x264_pixel_sad_x3_8x8_mmxext( uint8_t *, uint8_t *, uint8_t *, uint8_t *, int, int * );
void x264_pixel_sad_x3_8x4_mmxext( uint8_t *, uint8_t *, uint8_t *, uint8_t *, int, int * );
void x264_pixel_sad_x3_4x8_mmxext( uint8_t *, uint8_t *, uint8_t *, uint8_t *, int, int * );
void x264_pixel_sad_x3_4x4_mmxext( uint8_t *, uint8_t *, uint8_t *, uint8_t *, int, int * );
void x264_pixel_sad_x4_16x16_mmxext( uint8_t *, uint8_t *, uint8_t *, uint8_t *, uint8_t *, int, int * );
void x264_pixel_sad_x4_16x8_mmxext( uint8_t *, uint8_t *, uint8_t *, uint8_t *, uint8_t *, int, int * );
void x264_pixel_sad_x4_8x16_mmxext( uint8_t *, uint8_t *, uint8_t *, uint8_t *, uint8_t *, int, int * );
void x264_pixel_sad_x4_8x8_mmxext( uint8_t *, uint8_t *, uint8_t *, uint8_t *, uint8_t *, int, int * );
void x264_pixel_sad_x4_8x4_mmxext( uint8_t *, uint8_t *, uint8_t *, uint8_t *, uint8_t *, int, int * );
void x264_pixel_sad_x4_4x8_mmxext( uint8_t *, uint8_t *, uint8_t *, uint8_t *, uint8_t *, int, int * );
void x264_pixel_sad_x4_4x4_mmxext( uint8_t *, uint8_t *, uint8_t *, uint8_t *, uint8_t *, int, int * );

int x264_pixel_sad_pde_16x16_mmxext( uint8_t *, int, uint8_t *, int, int );
int x264_pixel_sad_pde_16x8_mmxext( uint8_t *, int, uint8_t *, int, int );
int x264_pixel_sad_pde_8x16_mmxext( uint8_t *, int, uint8_t *, int, int );

int x264_pixel_ssd_16x16_mmx( uint8_t *, int, uint8_t *, int );
int x264_pixel_ssd_16x8_mmx( uint8_t *, int, uint8_t *, int );
int x264_pixel_ssd_8x16_mmx( uint8_t *, int, uint8_t *, int );
int x264_pixel_ssd_8x8_mmx( uint8_t *, int, uint8_t *, int );
int x264_pixel_ssd_8x4_mmx( uint8_t *, int, uint8_t *, int );
int x264_pixel_ssd_4x8_mmx( uint8_t *, int, uint8_t *, int );
int x264_pixel_ssd_4x4_mmx( uint8_t *, int, uint8_t *, int );

int x264_pixel_satd_16x16_mmxext( uint8_t *, int, uint8_t *, int );
int x264_pixel_satd_16x8_mmxext( uint8_t *, int, uint8_t *, int );
int x264_pixel_satd_8x16_mmxext( uint8_t *, int, uint8_t *, int );
int x264_pixel_satd_8x8_mmxext( uint8_t *, int, uint8_t *, int );
int x264_pixel_satd_8x4_mmxext( uint8_t *, int, uint8_t *, int );
int x264_pixel_satd_4x8_mmxext( uint8_t *, int, uint8_t *, int );
int x264_pixel_satd_4x4_mmxext( uint8_t *, int, uint8_t *, int );

int x264_pixel_sa8d_16x16_mmxext( uint8_t *, int, uint8_t *, int );
int x264_pixel_sa8d_8x8_mmxext( uint8_t *, int, uint8_t *, int );

int x264_pixel_sad_16x16_sse2( uint8_t *, int, uint8_t *, int );
int x264_pixel_sad_16x8_sse2( uint8_t *, int, uint8_t *, int );

void x264_pixel_sad_x3_16x16_sse2( uint8_t *, uint8_t *, uint8_t *, uint8_t *, int, int * );
void x264_pixel_sad_x3_16x8_sse2( uint8_t *, uint8_t *, uint8_t *, uint8_t *, int, int * );
void x264_pixel_sad_x4_16x16_sse2( uint8_t *, uint8_t *, uint8_t *, uint8_t *, uint8_t *, int, int * );
void x264_pixel_sad_x4_16x8_sse2( uint8_t *, uint8_t *, uint8_t *, uint8_t *, uint8_t *, int, int * );

int x264_pixel_ssd_16x16_sse2( uint8_t *, int, uint8_t *, int );
int x264_pixel_ssd_16x8_sse2( uint8_t *, int, uint8_t *, int );

int x264_pixel_satd_16x16_sse2( uint8_t *, int, uint8_t *, int );
int x264_pixel_satd_16x8_sse2( uint8_t *, int, uint8_t *, int );
int x264_pixel_satd_8x16_sse2( uint8_t *, int, uint8_t *, int );
int x264_pixel_satd_8x8_sse2( uint8_t *, int, uint8_t *, int );
int x264_pixel_satd_8x4_sse2( uint8_t *, int, uint8_t *, int );

int x264_pixel_sa8d_16x16_sse2( uint8_t *, int, uint8_t *, int );
int x264_pixel_sa8d_8x8_sse2( uint8_t *, int, uint8_t *, int );

void x264_intra_satd_x3_4x4_mmxext( uint8_t *, uint8_t *, int * );
void x264_intra_satd_x3_8x8c_mmxext( uint8_t *, uint8_t *, int * );
void x264_intra_satd_x3_16x16_mmxext( uint8_t *, uint8_t *, int * );
void x264_intra_sa8d_x3_8x8_sse2( uint8_t *, uint8_t *, int * );
void x264_intra_sa8d_x3_8x8_mmxext( uint8_t *, uint8_t *, int * );
void x264_intra_sa8d_x3_8x8_core_sse2( uint8_t *, int16_t [2][8], int * );
void x264_intra_sa8d_x3_8x8_core_mmxext( uint8_t *, int16_t [2][8], int * );

#endif

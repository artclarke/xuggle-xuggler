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

#define DECL_PIXELS( ret, name, suffix, args ) \
    ret x264_pixel_##name##_16x16_##suffix args;\
    ret x264_pixel_##name##_16x8_##suffix args;\
    ret x264_pixel_##name##_8x16_##suffix args;\
    ret x264_pixel_##name##_8x8_##suffix args;\
    ret x264_pixel_##name##_8x4_##suffix args;\
    ret x264_pixel_##name##_4x8_##suffix args;\
    ret x264_pixel_##name##_4x4_##suffix args;\

#define DECL_X1( name, suffix ) \
    DECL_PIXELS( int, name, suffix, ( uint8_t *, int, uint8_t *, int ) )

#define DECL_X4( name, suffix ) \
    DECL_PIXELS( void, name##_x3, suffix, ( uint8_t *, uint8_t *, uint8_t *, uint8_t *, int, int * ) )\
    DECL_PIXELS( void, name##_x4, suffix, ( uint8_t *, uint8_t *, uint8_t *, uint8_t *, uint8_t *, int, int * ) )

DECL_X1( sad, mmxext )
DECL_X1( sad, sse2 )
DECL_X1( sad, sse3 )
DECL_X4( sad, mmxext )
DECL_X4( sad, sse2 )
DECL_X4( sad, sse3 )
DECL_X1( ssd, mmx )
DECL_X1( ssd, sse2 )
DECL_X1( satd, mmxext )
DECL_X1( satd, sse2 )
DECL_X1( satd, ssse3 )
DECL_X1( sa8d, mmxext )
DECL_X1( sa8d, sse2 )
DECL_X1( sa8d, ssse3 )
DECL_X1( sad, cache32_mmxext );
DECL_X1( sad, cache64_mmxext );
DECL_X1( sad, cache64_sse2 );
DECL_X1( sad, cache64_ssse3 );
DECL_X4( sad, cache32_mmxext );
DECL_X4( sad, cache64_mmxext );
DECL_X4( sad, cache64_sse2 );
DECL_X4( sad, cache64_ssse3 );

#undef DECL_PIXELS
#undef DECL_X1
#undef DECL_X4

void x264_intra_satd_x3_4x4_mmxext( uint8_t *, uint8_t *, int * );
void x264_intra_satd_x3_8x8c_mmxext( uint8_t *, uint8_t *, int * );
void x264_intra_satd_x3_16x16_mmxext( uint8_t *, uint8_t *, int * );
void x264_intra_sa8d_x3_8x8_sse2( uint8_t *, uint8_t *, int * );
void x264_intra_sa8d_x3_8x8_mmxext( uint8_t *, uint8_t *, int * );
void x264_intra_sa8d_x3_8x8_core_sse2( uint8_t *, int16_t [2][8], int * );
void x264_intra_sa8d_x3_8x8_core_mmxext( uint8_t *, int16_t [2][8], int * );

void x264_pixel_ssim_4x4x2_core_mmxext( const uint8_t *pix1, int stride1,
                                        const uint8_t *pix2, int stride2, int sums[2][4] );
void x264_pixel_ssim_4x4x2_core_sse2( const uint8_t *pix1, int stride1,
                                      const uint8_t *pix2, int stride2, int sums[2][4] );
float x264_pixel_ssim_end4_sse2( int sum0[5][4], int sum1[5][4], int width );

void x264_pixel_ads4_mmxext( int enc_dc[4], uint16_t *sums, int delta,
                             uint16_t *res, int width );
void x264_pixel_ads2_mmxext( int enc_dc[2], uint16_t *sums, int delta,
                             uint16_t *res, int width );
void x264_pixel_ads1_mmxext( int enc_dc[1], uint16_t *sums, int delta,
                             uint16_t *res, int width );

#endif

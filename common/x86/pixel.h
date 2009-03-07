/*****************************************************************************
 * pixel.h: h264 encoder library
 *****************************************************************************
 * Copyright (C) 2003-2008 x264 project
 *
 * Authors: Laurent Aimar <fenrir@via.ecp.fr>
 *          Loren Merritt <lorenm@u.washington.edu>
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

#ifndef X264_I386_PIXEL_H
#define X264_I386_PIXEL_H

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
DECL_X4( sad, sse2_misalign )
DECL_X1( sad, sse3 )
DECL_X1( sad, sse2_aligned )
DECL_X4( sad, mmxext )
DECL_X4( sad, sse2 )
DECL_X4( sad, sse3 )
DECL_X1( ssd, mmx )
DECL_X1( ssd, sse2slow )
DECL_X1( ssd, sse2 )
DECL_X1( ssd, ssse3 )
DECL_X1( satd, mmxext )
DECL_X1( satd, sse2 )
DECL_X1( satd, ssse3 )
DECL_X1( satd, sse4 )
DECL_X1( sa8d, mmxext )
DECL_X1( sa8d, sse2 )
DECL_X1( sa8d, ssse3 )
DECL_X1( sa8d, sse4)
DECL_X1( sad, cache32_mmxext );
DECL_X1( sad, cache64_mmxext );
DECL_X1( sad, cache64_sse2 );
DECL_X1( sad, cache64_ssse3 );
DECL_X4( sad, cache32_mmxext );
DECL_X4( sad, cache64_mmxext );
DECL_X4( sad, cache64_sse2 );
DECL_X4( sad, cache64_ssse3 );

DECL_PIXELS( int, var, mmxext, ( uint8_t *pix, int i_stride ))
DECL_PIXELS( int, var, sse2,   ( uint8_t *pix, int i_stride ))
DECL_PIXELS( uint64_t, hadamard_ac, mmxext, ( uint8_t *pix, int i_stride ))
DECL_PIXELS( uint64_t, hadamard_ac, sse2,   ( uint8_t *pix, int i_stride ))
DECL_PIXELS( uint64_t, hadamard_ac, ssse3,  ( uint8_t *pix, int i_stride ))
DECL_PIXELS( uint64_t, hadamard_ac, sse4,   ( uint8_t *pix, int i_stride ))


void x264_intra_satd_x3_4x4_mmxext  ( uint8_t *, uint8_t *, int * );
void x264_intra_satd_x3_4x4_ssse3   ( uint8_t *, uint8_t *, int * );
void x264_intra_satd_x3_8x8c_mmxext ( uint8_t *, uint8_t *, int * );
void x264_intra_satd_x3_8x8c_ssse3  ( uint8_t *, uint8_t *, int * );
void x264_intra_satd_x3_16x16_mmxext( uint8_t *, uint8_t *, int * );
void x264_intra_satd_x3_16x16_ssse3 ( uint8_t *, uint8_t *, int * );
void x264_intra_sad_x3_16x16_mmxext ( uint8_t *, uint8_t *, int * );
void x264_intra_sad_x3_16x16_sse2   ( uint8_t *, uint8_t *, int * );
void x264_intra_sad_x3_16x16_ssse3  ( uint8_t *, uint8_t *, int * );
void x264_intra_sa8d_x3_8x8_mmxext  ( uint8_t *, uint8_t *, int * );
void x264_intra_sa8d_x3_8x8_sse2    ( uint8_t *, uint8_t *, int * );
void x264_intra_sa8d_x3_8x8_ssse3   ( uint8_t *, uint8_t *, int * );
void x264_intra_sa8d_x3_8x8_core_mmxext( uint8_t *, int16_t [2][8], int * );
void x264_intra_sa8d_x3_8x8_core_sse2  ( uint8_t *, int16_t [2][8], int * );
void x264_intra_sa8d_x3_8x8_core_ssse3 ( uint8_t *, int16_t [2][8], int * );

void x264_pixel_ssim_4x4x2_core_mmxext( const uint8_t *pix1, int stride1,
                                        const uint8_t *pix2, int stride2, int sums[2][4] );
void x264_pixel_ssim_4x4x2_core_sse2( const uint8_t *pix1, int stride1,
                                      const uint8_t *pix2, int stride2, int sums[2][4] );
float x264_pixel_ssim_end4_sse2( int sum0[5][4], int sum1[5][4], int width );

#define DECL_ADS( size, suffix ) \
int x264_pixel_ads##size##_##suffix( int enc_dc[size], uint16_t *sums, int delta,\
                                     uint16_t *cost_mvx, int16_t *mvs, int width, int thresh );
DECL_ADS( 4, mmxext )
DECL_ADS( 2, mmxext )
DECL_ADS( 1, mmxext )
DECL_ADS( 4, sse2 )
DECL_ADS( 2, sse2 )
DECL_ADS( 1, sse2 )
DECL_ADS( 4, ssse3 )
DECL_ADS( 2, ssse3 )
DECL_ADS( 1, ssse3 )

#undef DECL_PIXELS
#undef DECL_X1
#undef DECL_X4
#undef DECL_ADS

#endif

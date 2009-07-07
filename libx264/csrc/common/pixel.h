/*****************************************************************************
 * pixel.h: h264 encoder library
 *****************************************************************************
 * Copyright (C) 2004-2008 Loren Merritt <lorenm@u.washington.edu>
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

#ifndef X264_PIXEL_H
#define X264_PIXEL_H

// SSD assumes all args aligned
// other cmp functions assume first arg aligned
typedef int  (*x264_pixel_cmp_t) ( uint8_t *, int, uint8_t *, int );
typedef void (*x264_pixel_cmp_x3_t) ( uint8_t *, uint8_t *, uint8_t *, uint8_t *, int, int[3] );
typedef void (*x264_pixel_cmp_x4_t) ( uint8_t *, uint8_t *, uint8_t *, uint8_t *, uint8_t *, int, int[4] );

enum
{
    PIXEL_16x16 = 0,
    PIXEL_16x8  = 1,
    PIXEL_8x16  = 2,
    PIXEL_8x8   = 3,
    PIXEL_8x4   = 4,
    PIXEL_4x8   = 5,
    PIXEL_4x4   = 6,
    PIXEL_4x2   = 7,
    PIXEL_2x4   = 8,
    PIXEL_2x2   = 9,
};

static const struct {
    int w;
    int h;
} x264_pixel_size[7] = {
    { 16, 16 },
    { 16,  8 }, {  8, 16 },
    {  8,  8 },
    {  8,  4 }, {  4,  8 },
    {  4,  4 }
};

static const uint8_t x264_size2pixel[5][5] = {
    { 0, },
    { 0, PIXEL_4x4, PIXEL_8x4, 0, 0 },
    { 0, PIXEL_4x8, PIXEL_8x8, 0, PIXEL_16x8 },
    { 0, },
    { 0, 0,        PIXEL_8x16, 0, PIXEL_16x16 }
};

typedef struct
{
    x264_pixel_cmp_t  sad[7];
    x264_pixel_cmp_t  ssd[7];
    x264_pixel_cmp_t satd[7];
    x264_pixel_cmp_t ssim[7];
    x264_pixel_cmp_t sa8d[4];
    x264_pixel_cmp_t mbcmp[7]; /* either satd or sad for subpel refine and mode decision */
    x264_pixel_cmp_t mbcmp_unaligned[7]; /* unaligned mbcmp for subpel */
    x264_pixel_cmp_t fpelcmp[7]; /* either satd or sad for fullpel motion search */
    x264_pixel_cmp_x3_t fpelcmp_x3[7];
    x264_pixel_cmp_x4_t fpelcmp_x4[7];
    x264_pixel_cmp_t sad_aligned[7]; /* Aligned SAD for mbcmp */

    int (*var[4])( uint8_t *pix, int stride );
    uint64_t (*hadamard_ac[4])( uint8_t *pix, int stride );

    void (*ssim_4x4x2_core)( const uint8_t *pix1, int stride1,
                             const uint8_t *pix2, int stride2, int sums[2][4] );
    float (*ssim_end4)( int sum0[5][4], int sum1[5][4], int width );

    /* multiple parallel calls to cmp. */
    x264_pixel_cmp_x3_t sad_x3[7];
    x264_pixel_cmp_x4_t sad_x4[7];
    x264_pixel_cmp_x3_t satd_x3[7];
    x264_pixel_cmp_x4_t satd_x4[7];

    /* abs-diff-sum for successive elimination.
     * may round width up to a multiple of 16. */
    int (*ads[7])( int enc_dc[4], uint16_t *sums, int delta,
                   uint16_t *cost_mvx, int16_t *mvs, int width, int thresh );

    /* calculate satd or sad of V, H, and DC modes.
     * may be NULL, in which case just use pred+satd instead. */
    void (*intra_mbcmp_x3_16x16)( uint8_t *fenc, uint8_t *fdec  , int res[3] );
    void (*intra_satd_x3_16x16) ( uint8_t *fenc, uint8_t *fdec  , int res[3] );
    void (*intra_sad_x3_16x16)  ( uint8_t *fenc, uint8_t *fdec  , int res[3] );
    void (*intra_satd_x3_8x8c)  ( uint8_t *fenc, uint8_t *fdec  , int res[3] );
    void (*intra_satd_x3_4x4)   ( uint8_t *fenc, uint8_t *fdec  , int res[3] );
    void (*intra_sa8d_x3_8x8)   ( uint8_t *fenc, uint8_t edge[33], int res[3] );
} x264_pixel_function_t;

void x264_pixel_init( int cpu, x264_pixel_function_t *pixf );
int64_t x264_pixel_ssd_wxh( x264_pixel_function_t *pf, uint8_t *pix1, int i_pix1, uint8_t *pix2, int i_pix2, int i_width, int i_height );
float x264_pixel_ssim_wxh( x264_pixel_function_t *pf, uint8_t *pix1, int i_pix1, uint8_t *pix2, int i_pix2, int i_width, int i_height, void *buf );

#endif

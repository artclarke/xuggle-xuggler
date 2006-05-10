/*****************************************************************************
 * pixel.h: h264 encoder library
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

#ifndef _PIXEL_H
#define _PIXEL_H 1

typedef int  (*x264_pixel_cmp_t) ( uint8_t *, int, uint8_t *, int );
typedef int  (*x264_pixel_cmp_pde_t) ( uint8_t *, int, uint8_t *, int, int );
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

static const int x264_size2pixel[5][5] = {
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
    x264_pixel_cmp_t sa8d[4];
    x264_pixel_cmp_t mbcmp[7]; /* either satd or sad for subpel refine and mode decision */

    /* partial distortion elimination:
     * terminate early if partial score is worse than a threshold.
     * may be NULL, in which case just use sad instead. */
    x264_pixel_cmp_pde_t sad_pde[7];

    /* multiple parallel calls to sad. */
    x264_pixel_cmp_x3_t sad_x3[7];
    x264_pixel_cmp_x4_t sad_x4[7];

    /* calculate satd of V, H, and DC modes.
     * may be NULL, in which case just use pred+satd instead. */
    void (*intra_satd_x3_16x16)( uint8_t *fenc, uint8_t *fdec, int res[3] );
    void (*intra_satd_x3_8x8c)( uint8_t *fenc, uint8_t *fdec, int res[3] );
    void (*intra_satd_x3_4x4)( uint8_t *fenc, uint8_t *fdec, int res[3] );
    void (*intra_sa8d_x3_8x8)( uint8_t *fenc, uint8_t edge[33], int res[3] );
} x264_pixel_function_t;

void x264_pixel_init( int cpu, x264_pixel_function_t *pixf );
int64_t x264_pixel_ssd_wxh( x264_pixel_function_t *pf, uint8_t *pix1, int i_pix1, uint8_t *pix2, int i_pix2, int i_width, int i_height );

#endif

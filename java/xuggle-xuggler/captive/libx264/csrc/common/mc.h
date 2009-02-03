/*****************************************************************************
 * mc.h: h264 encoder library (Motion Compensation)
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

#ifndef X264_MC_H
#define X264_MC_H

/* Do the MC
 * XXX: Only width = 4, 8 or 16 are valid
 * width == 4 -> height == 4 or 8
 * width == 8 -> height == 4 or 8 or 16
 * width == 16-> height == 8 or 16
 * */

typedef struct
{
    void (*mc_luma)(uint8_t *dst, int i_dst, uint8_t **src, int i_src,
                    int mvx, int mvy,
                    int i_width, int i_height );

    /* may round up the dimensions if they're not a power of 2 */
    uint8_t* (*get_ref)(uint8_t *dst, int *i_dst, uint8_t **src, int i_src,
                        int mvx, int mvy,
                        int i_width, int i_height );

    /* mc_chroma may write up to 2 bytes of garbage to the right of dst,
     * so it must be run from left to right. */
    void (*mc_chroma)(uint8_t *dst, int i_dst, uint8_t *src, int i_src,
                      int mvx, int mvy,
                      int i_width, int i_height );

    void (*avg[10])( uint8_t *dst, int, uint8_t *src1, int, uint8_t *src2, int, int i_weight );

    /* only 16x16, 8x8, and 4x4 defined */
    void (*copy[7])( uint8_t *dst, int, uint8_t *src, int, int i_height );
    void (*copy_16x16_unaligned)( uint8_t *dst, int, uint8_t *src, int, int i_height );

    void (*plane_copy)( uint8_t *dst, int i_dst,
                        uint8_t *src, int i_src, int w, int h);

    void (*hpel_filter)( uint8_t *dsth, uint8_t *dstv, uint8_t *dstc, uint8_t *src,
                         int i_stride, int i_width, int i_height, int16_t *buf );

    /* prefetch the next few macroblocks of fenc or fdec */
    void (*prefetch_fenc)( uint8_t *pix_y, int stride_y,
                           uint8_t *pix_uv, int stride_uv, int mb_x );
    /* prefetch the next few macroblocks of a hpel reference frame */
    void (*prefetch_ref)( uint8_t *pix, int stride, int parity );

    void *(*memcpy_aligned)( void *dst, const void *src, size_t n );
    void (*memzero_aligned)( void *dst, int n );

    /* successive elimination prefilter */
    void (*integral_init4h)( uint16_t *sum, uint8_t *pix, int stride );
    void (*integral_init8h)( uint16_t *sum, uint8_t *pix, int stride );
    void (*integral_init4v)( uint16_t *sum8, uint16_t *sum4, int stride );
    void (*integral_init8v)( uint16_t *sum8, int stride );

    void (*frame_init_lowres_core)( uint8_t *src0, uint8_t *dst0, uint8_t *dsth, uint8_t *dstv, uint8_t *dstc,
                                    int src_stride, int dst_stride, int width, int height );
} x264_mc_functions_t;

void x264_mc_init( int cpu, x264_mc_functions_t *pf );

#endif

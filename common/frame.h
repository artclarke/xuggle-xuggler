/*****************************************************************************
 * frame.h: h264 encoder library
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar
 * $Id: frame.h,v 1.1 2004/06/03 19:27:06 fenrir Exp $
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

#ifndef _FRAME_H
#define _FRAME_H 1

typedef struct
{
    /* */
    int     i_poc;
    int     i_type;
    int     i_qpplus1;
    int64_t i_pts;
    int     i_frame;    /* Presentation frame number */
    int     i_frame_num; /* Coded frame number */
    int     b_kept_as_ref;

    /* YUV buffer */
    int     i_plane;
    int     i_stride[4];
    int     i_lines[4];
    int     i_stride_lowres;
    int     i_lines_lowres;
    uint8_t *plane[4];
    uint8_t *filtered[4]; /* plane[0], H, V, HV */
    uint8_t *lowres[4]; /* half-size copy of input frame: Orig, H, V, HV */

    /* for unrestricted mv we allocate more data than needed
     * allocated data are stored in buffer */
    void    *buffer[11];

    /* motion data */
    int8_t  *mb_type;
    int16_t (*mv[2])[2];
    int8_t  *ref[2];
    int     i_ref[2];
    int     ref_poc[2][16];

    /* for adaptive B-frame decision.
     * contains the SATD cost of the lowres frame encoded in various modes
     * FIXME: how big an array do we need? */
    int     i_cost_est[16][16];
    int     i_intra_mbs[16];

} x264_frame_t;

x264_frame_t *x264_frame_new( x264_t *h );
void          x264_frame_delete( x264_frame_t *frame );

void          x264_frame_copy_picture( x264_t *h, x264_frame_t *dst, x264_picture_t *src );

void          x264_frame_expand_border( x264_frame_t *frame );
void          x264_frame_expand_border_filtered( x264_frame_t *frame );
void          x264_frame_expand_border_lowres( x264_frame_t *frame );

void          x264_frame_deblocking_filter( x264_t *h, int i_slice_type );

void          x264_frame_filter( int cpu, x264_frame_t *frame );
void          x264_frame_init_lowres( int cpu, x264_frame_t *frame );

#endif

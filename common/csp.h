/*****************************************************************************
 * csp.h: h264 encoder library
 *****************************************************************************
 * Copyright (C) 2004 Laurent Aimar
 * $Id: csp.h,v 1.1 2004/06/03 19:27:06 fenrir Exp $
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

#ifndef _CSP_H
#define _CSP_H 1

typedef struct
{
    void (*i420)( x264_frame_t *, x264_image_t *, int i_width, int i_height );
    void (*i422)( x264_frame_t *, x264_image_t *, int i_width, int i_height );
    void (*i444)( x264_frame_t *, x264_image_t *, int i_width, int i_height );
    void (*yv12)( x264_frame_t *, x264_image_t *, int i_width, int i_height );
    void (*yuyv)( x264_frame_t *, x264_image_t *, int i_width, int i_height );
    void (*rgb )( x264_frame_t *, x264_image_t *, int i_width, int i_height );
    void (*bgr )( x264_frame_t *, x264_image_t *, int i_width, int i_height );
    void (*bgra)( x264_frame_t *, x264_image_t *, int i_width, int i_height );
} x264_csp_function_t;


void x264_csp_init( int cpu, int i_csp, x264_csp_function_t *pf );

#endif


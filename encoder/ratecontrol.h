/*****************************************************************************
 * ratecontrol.h: h264 encoder library (Rate Control)
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar
 * $Id: ratecontrol.h,v 1.1 2004/06/03 19:27:08 fenrir Exp $
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

#ifndef _RATECONTROL_H
#define _RATECONTROL_H 1

struct x264_ratecontrol_t
{
    float fps;
    int   i_iframe;

    int i_bitrate;
    int i_qp_last;
    int i_qp;

    int i_slice_type;

    int     i_frames;
    int64_t i_size;

};


x264_ratecontrol_t *x264_ratecontrol_new   ( x264_param_t * );
void                x264_ratecontrol_delete( x264_ratecontrol_t * );

void x264_ratecontrol_start( x264_ratecontrol_t *, int i_slice_type );
int  x264_ratecontrol_qp( x264_ratecontrol_t * );
void x264_ratecontrol_end( x264_ratecontrol_t *, int bits );

#endif


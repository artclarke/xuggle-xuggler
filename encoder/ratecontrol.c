/*****************************************************************************
 * ratecontrol.c: h264 encoder library (Rate Control)
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar
 * $Id: ratecontrol.c,v 1.1 2004/06/03 19:27:08 fenrir Exp $
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "../core/common.h"
#include "ratecontrol.h"


x264_ratecontrol_t *x264_ratecontrol_new( x264_param_t *param )
{
    x264_ratecontrol_t *rc = x264_malloc( sizeof( x264_ratecontrol_t ) );

    rc->fps = param->f_fps > 0.1 ? param->f_fps : 25.0f;
    rc->i_iframe = param->i_iframe;
    rc->i_bitrate = param->i_bitrate * 1000;

    rc->i_qp_last = 26;
    rc->i_qp      = param->i_qp_constant;

    rc->i_frames  = 0;
    rc->i_size    = 0;

    return rc;
}

void x264_ratecontrol_delete( x264_ratecontrol_t *rc )
{
    x264_free( rc );
}

void x264_ratecontrol_start( x264_ratecontrol_t *rc, int i_slice_type )
{
    rc->i_slice_type = i_slice_type;
}

int  x264_ratecontrol_qp( x264_ratecontrol_t *rc )
{
    return x264_clip3( rc->i_qp, 1, 51 );
}

void x264_ratecontrol_end( x264_ratecontrol_t *rc, int bits )
{
    return;
#if 0
    int i_avg;
    int i_target = rc->i_bitrate / rc->fps;
    int i_qp = rc->i_qp;

    rc->i_qp_last = rc->i_qp;
    rc->i_frames++;
    rc->i_size += bits / 8;

    i_avg = 8 * rc->i_size / rc->i_frames;

    if( rc->i_slice_type == SLICE_TYPE_I )
    {
        i_target = i_target * 20 / 10;
    }

    if( i_avg > i_target * 11 / 10 )
    {
        i_qp = rc->i_qp + ( i_avg / i_target - 1 );
    }
    else if( i_avg < i_target * 9 / 10 )
    {
        i_qp = rc->i_qp - ( i_target / i_avg - 1 );
    }

    rc->i_qp = x264_clip3( i_qp, rc->i_qp_last - 2, rc->i_qp_last + 2 );
#endif
}


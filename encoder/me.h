/*****************************************************************************
 * me.h: h264 encoder library (Motion Estimation)
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar
 * $Id: me.h,v 1.1 2004/06/03 19:27:08 fenrir Exp $
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

#ifndef _ME_H
#define _ME_H 1

typedef struct
{
    /* input */
    int      i_pixel;   /* PIXEL_WxH */
    int      lm;        /* lambda motion */

    uint8_t *p_fref;
    uint8_t *p_fenc;
    int      i_stride;

    int i_mv_range;

    int mvp[2];

    int b_mvc;
    int mvc[2];

    /* output */
    int cost;           /* satd + lm * nbits */
    int mv[2];
} x264_me_t;

void x264_me_search( x264_t *h, x264_me_t *m );
void x264_me_refine_qpel( x264_t *h, x264_me_t *m );

#endif

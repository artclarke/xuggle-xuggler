/*****************************************************************************
 * ratecontrol.h: h264 encoder library (Rate Control)
 *****************************************************************************
 * Copyright (C) 2003-2008 x264 project
 *
 * Authors: Loren Merritt <lorenm@u.washington.edu>
 *          Laurent Aimar <fenrir@via.ecp.fr>
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

#ifndef X264_RATECONTROL_H
#define X264_RATECONTROL_H

int  x264_ratecontrol_new   ( x264_t * );
void x264_ratecontrol_delete( x264_t * );

void x264_adaptive_quant_frame( x264_t *h, x264_frame_t *frame );
void x264_adaptive_quant( x264_t * );
void x264_thread_sync_ratecontrol( x264_t *cur, x264_t *prev, x264_t *next );
void x264_ratecontrol_start( x264_t *, int i_force_qp );
int  x264_ratecontrol_slice_type( x264_t *, int i_frame );
void x264_ratecontrol_mb( x264_t *, int bits );
int  x264_ratecontrol_qp( x264_t * );
void x264_ratecontrol_end( x264_t *, int bits );
void x264_ratecontrol_summary( x264_t * );
void x264_ratecontrol_set_estimated_size( x264_t *, int bits );
int  x264_ratecontrol_get_estimated_size( x264_t const *);
int  x264_rc_analyse_slice( x264_t *h );

#endif


/*****************************************************************************
 * set.h: h264 encoder
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

#ifndef X264_ENCODER_SET_H
#define X264_ENCODER_SET_H

void x264_sps_init( x264_sps_t *sps, int i_id, x264_param_t *param );
void x264_sps_write( bs_t *s, x264_sps_t *sps );
void x264_pps_init( x264_pps_t *pps, int i_id, x264_param_t *param, x264_sps_t *sps );
void x264_pps_write( bs_t *s, x264_pps_t *pps );
void x264_sei_version_write( x264_t *h, bs_t *s );
int  x264_validate_levels( x264_t *h, int verbose );

#endif

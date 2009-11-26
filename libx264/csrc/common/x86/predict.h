/*****************************************************************************
 * predict.h: h264 encoder library
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

#ifndef X264_I386_PREDICT_H
#define X264_I386_PREDICT_H

void x264_predict_16x16_init_mmx ( int cpu, x264_predict_t pf[7] );
void x264_predict_8x8c_init_mmx  ( int cpu, x264_predict_t pf[7] );
void x264_predict_4x4_init_mmx   ( int cpu, x264_predict_t pf[12] );
void x264_predict_8x8_init_mmx   ( int cpu, x264_predict8x8_t pf[12], x264_predict_8x8_filter_t *predict_8x8_filter );
#endif

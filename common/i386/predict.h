/*****************************************************************************
 * predict.h: h264 encoder library
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar
 * $Id: predict.h,v 1.1 2004/06/03 19:27:07 fenrir Exp $
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

#ifndef _I386_PREDICT_H
#define _I386_PREDICT_H 1

void x264_predict_16x16_init_mmxext ( x264_predict_t pf[7] );
void x264_predict_8x8c_init_mmxext  ( x264_predict_t pf[7] );
void x264_predict_4x4_init_mmxext   ( x264_predict_t pf[12] );
void x264_predict_8x8_init_mmxext   ( x264_predict8x8_t pf[12] );
void x264_predict_8x8_init_sse2     ( x264_predict8x8_t pf[12] );

#endif

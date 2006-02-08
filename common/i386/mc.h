/*****************************************************************************
 * mc.h: h264 encoder library
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar
 * $Id: mc.h,v 1.1 2004/06/03 19:27:07 fenrir Exp $
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

#ifndef _I386_MC_H
#define _I386_MC_H 1

void x264_mc_mmxext_init( x264_mc_functions_t *pf );
void x264_mc_sse2_init( x264_mc_functions_t *pf );

void x264_mc_chroma_mmxext( uint8_t *src, int i_src_stride,
                            uint8_t *dst, int i_dst_stride,
                            int dx, int dy, int i_width, int i_height );
#endif

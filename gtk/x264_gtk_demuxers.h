/*****************************************************************************
 * x264_gtk_demuxers.h: h264 gtk encoder frontend
 *****************************************************************************
 * Copyright (C) 2006 Vincent Torri
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

#ifndef X264_GTK_MUXERS_H
#define X264_GTK_MUXERS_H

#include "../config.h"
#include "../muxers.h"

typedef enum {
  X264_DEMUXER_YUV    = 0,
  X264_DEMUXER_CIF,
  X264_DEMUXER_QCIF,
  X264_DEMUXER_Y4M,
  X264_DEMUXER_AVI,
  X264_DEMUXER_AVS,
  X264_DEMUXER_UNKOWN
} X264_Demuxer_Type;
/* static int X264_Num_Demuxers = (int)X264_DEMUXER_UNKOWN; */

#endif  /* X264_GTK_MUXERS_H */

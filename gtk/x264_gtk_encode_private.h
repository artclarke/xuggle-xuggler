/*****************************************************************************
 * x264_gtk_encode_private.h: h264 gtk encoder frontend
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

#ifndef X264_GTK_ENCODE_PRIVATE_H
#define X264_GTK_ENCODE_PRIVATE_H


#include <common/osdep.h>

#include "x264_gtk_demuxers.h"

typedef struct X264_Thread_Data_ X264_Thread_Data;
typedef struct X264_Pipe_Data_ X264_Pipe_Data;

struct X264_Thread_Data_
{
  GtkWidget         *current_video_frame;
  GtkWidget         *video_data;
  GtkWidget         *video_rendering_rate;
  GtkWidget         *time_elapsed;
  GtkWidget         *time_remaining;
  GtkWidget         *progress;

  GtkWidget         *dialog;
  GtkWidget         *button;
  GtkWidget         *end_button;

  x264_param_t      *param;
  gchar             *file_input;
  X264_Demuxer_Type  in_container;

  gchar             *file_output;
  gint               out_container;

  /* file descriptors */
  GIOChannel        *io_read;  /* use it with read */
  GIOChannel        *io_write; /* use it with write */
};

struct X264_Pipe_Data_
{
  int     frame;
  int     frame_total;
  int     file;
  int64_t elapsed;
};


#endif /* X264_GTK_ENCODE_PRIVATE_H */

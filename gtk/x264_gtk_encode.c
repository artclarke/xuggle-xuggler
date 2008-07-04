/*****************************************************************************
 * x264_gtk_encode.c: h264 gtk encoder frontend
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

#if defined __FreeBSD__ || defined __OpenBSD__ || defined __NetBSD__ || defined __DragonFly__
#  include <inttypes.h>
#else
#  include <stdint.h>
#endif

#include <gtk/gtk.h>

#include "x264_gtk_private.h"
#include "x264_gtk_i18n.h"
#include "x264_gtk_encode_main_window.h"

int
main (int argc, char *argv[])
{
  BIND_X264_TEXTDOMAIN();

  g_thread_init (NULL);

  gtk_init (&argc, &argv);

  x264_gtk_encode_main_window ();

  gtk_main ();

  return 0;
}

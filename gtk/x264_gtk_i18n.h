/*****************************************************************************
 * x264_gtk_i18n.h: h264 gtk encoder frontend
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

#ifndef X264_GTK_I18N_H
#define X264_GTK_I18N_H


#include <libintl.h>


#define _(X) gettext(X)
#define GETTEXT_DOMAIN "x264_gtk"

#ifdef X264_GTK_PRIVATE_H
/* x264_path must be known for this to work */
#  define BIND_X264_TEXTDOMAIN()                        \
  gchar* x264_tmp = x264_gtk_path("locale");                    \
  bindtextdomain(GETTEXT_DOMAIN, x264_tmp);                 \
  g_free(x264_tmp);                                         \
  bind_textdomain_codeset (GETTEXT_DOMAIN, "UTF-8");    \
  textdomain(GETTEXT_DOMAIN)
#else
#  define BIND_X264_TEXTDOMAIN() you_must_include_x64_gtk_private_h_first
#endif


#endif /* X264_GTK_I18N_H */

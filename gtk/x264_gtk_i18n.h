#ifndef X264_GTK_I18N_H
#define X264_GTK_I18N_H


#include <libintl.h>


#define _(X) gettext(X)
#define GETTEXT_DOMAIN "x264_gtk"

#ifdef X264_GTK_PRIVATE_H
/* x264_path must be known for this to work */
#  define BIND_X264_TEXTDOMAIN()                        \
  gchar* _tmp = x264_gtk_path("locale");                    \
  bindtextdomain(GETTEXT_DOMAIN, _tmp);                 \
  g_free(_tmp);                                         \
  bind_textdomain_codeset (GETTEXT_DOMAIN, "UTF-8");    \
  textdomain(GETTEXT_DOMAIN)
#else
#  define BIND_X264_TEXTDOMAIN() you_must_include_x64_gtk_private_h_first
#endif


#endif /* X264_GTK_I18N_H */

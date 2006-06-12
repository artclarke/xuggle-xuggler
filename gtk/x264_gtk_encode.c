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

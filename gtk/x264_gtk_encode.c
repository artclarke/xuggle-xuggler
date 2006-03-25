#include <gtk/gtk.h>

#include "x264_gtk_encode_main_window.h"

int
main (int argc, char *argv[])
{
  g_thread_init (NULL);
  gtk_init (&argc, &argv);

  x264_gtk_encode_main_window ();

  gtk_main ();

  return 0;
}

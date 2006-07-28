#include <stdint.h>
#include <string.h>

#include <gtk/gtk.h>

#define DECLARE_ALIGNED( type, var, n ) type var __attribute__((aligned(n)))

#include "../x264.h"
#include "../common/common.h"

#include "x264_gtk.h"
#include "x264_gtk_private.h"
#include "x264_gtk_i18n.h"


int
main (int argc, char *argv[])
{
  GtkWidget    *window;
  X264_Gtk     *x264_gtk;
  x264_param_t *param;
  x264_param_t  param_default;
  char         *res;
  char         *res_default;

  BIND_X264_TEXTDOMAIN();

  gtk_init (&argc, &argv);

  window = x264_gtk_window_create (NULL);
  x264_gtk_shutdown (window);

  x264_gtk = x264_gtk_load ();
  param = x264_gtk_param_get (x264_gtk);

  /* do what you want with these data */
  /* for example, displaying them and compare with default*/
  res = x264_param2string (param, 0);
  printf ("%s\n", res);

  x264_param_default (&param_default);
  res_default = x264_param2string (&param_default, 0);
  printf ("\n%s\n", res_default);

  if (strcmp (res, res_default) == 0)
    printf (_("\nSame result !\n"));
  else
    printf (_("\nDifferent from default values\n"));

  x264_free (res);
  x264_free (res_default);

  x264_gtk_free (x264_gtk);
  g_free (param);

  return 1;
}

#if defined __FreeBSD__ || defined __OpenBSD__ || defined __NetBSD__ || defined __DragonFly__
#  include <inttypes.h>
#else
#  include <stdint.h>
#endif

#include <gtk/gtk.h>

#include "../x264.h"
#include "x264_gtk_private.h"
#include "x264_gtk_i18n.h"
#include "x264_gtk_enum.h"


/* Callbacks */
static void _cqm_flat_matrix_cb   (GtkToggleButton *togglebutton,
                                   gpointer         user_data);
static void _cqm_jvt_matrix_cb    (GtkToggleButton *togglebutton,
                                   gpointer         user_data);
static void _cqm_custom_matrix_cb (GtkToggleButton *togglebutton,
                                   gpointer         user_data);
static void _cqm_matrix_file_cb   (GtkFileChooser  *filechooser,
                                   gint             response,
                                   gpointer         user_data);


static GtkWidget *_cqm_4x4_page    (X264_Gui_Config *gconfig);
static GtkWidget *_cqm_8x8_iy_page (X264_Gui_Config *gconfig);
static GtkWidget *_cqm_8x8_py_page (X264_Gui_Config *gconfig);

GtkWidget *
_cqm_page (X264_Gui_Config *gconfig)
{
  GtkWidget   *vbox;
  GtkWidget   *table;
  GtkWidget   *notebook;
  GtkWidget   *chooser;
  GtkWidget   *label;
  GtkWidget   *page;
  GtkTooltips *tooltips;

  tooltips = gtk_tooltips_new ();

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);

  table = gtk_table_new (4, 2, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_container_set_border_width (GTK_CONTAINER (table), 6);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, TRUE, 6);
  gtk_widget_show (table);

  gconfig->cqm.radio_flat = gtk_radio_button_new_with_label (NULL, _("Flat matrix"));
  gtk_tooltips_set_tip (tooltips, gconfig->cqm.radio_flat,
                        _("Flat matrix - description"),
                        "");
  gtk_table_attach_defaults (GTK_TABLE (table), gconfig->cqm.radio_flat,
                             0, 1, 0, 1);
  g_signal_connect (G_OBJECT (gconfig->cqm.radio_flat), "toggled",
                    G_CALLBACK (_cqm_flat_matrix_cb), gconfig);
  gtk_widget_show (gconfig->cqm.radio_flat);

  gconfig->cqm.radio_jvt = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (gconfig->cqm.radio_flat), _("JVT matrix"));
  gtk_tooltips_set_tip (tooltips, gconfig->cqm.radio_jvt,
                        _("JVT matrix - description"),
                        "");
  gtk_table_attach_defaults (GTK_TABLE (table), gconfig->cqm.radio_jvt,
                             0, 1, 1, 2);
  g_signal_connect (G_OBJECT (gconfig->cqm.radio_jvt), "toggled",
                    G_CALLBACK (_cqm_jvt_matrix_cb), gconfig);
  gtk_widget_show (gconfig->cqm.radio_jvt);

  gconfig->cqm.radio_custom = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (gconfig->cqm.radio_flat), _("Custom matrix"));
  gtk_tooltips_set_tip (tooltips, gconfig->cqm.radio_custom,
                        _("Custom matrix - description"),
                        "");
  gtk_table_attach_defaults (GTK_TABLE (table), gconfig->cqm.radio_custom,
                             0, 1, 2, 3);
  g_signal_connect (G_OBJECT (gconfig->cqm.radio_custom), "toggled",
                    G_CALLBACK (_cqm_custom_matrix_cb), gconfig);
  gtk_widget_show (gconfig->cqm.radio_custom);

  chooser = gtk_file_chooser_dialog_new(_("Select a file"),
                                        NULL,
                                        GTK_FILE_CHOOSER_ACTION_OPEN,
                                        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                        GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                        NULL);
  g_signal_connect (G_OBJECT (chooser), "response",
                    G_CALLBACK (_cqm_matrix_file_cb), gconfig);
  gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (chooser),
                                       g_get_home_dir ());
  gconfig->cqm.cqm_file = gtk_file_chooser_button_new_with_dialog (chooser);
  gtk_table_attach_defaults (GTK_TABLE (table), gconfig->cqm.cqm_file,
                             1, 2, 2, 3);
  gtk_widget_show (gconfig->cqm.cqm_file);

  notebook = gtk_notebook_new ();
  gtk_table_attach_defaults (GTK_TABLE (table), notebook,
                             0, 2, 3, 4);
  gtk_widget_show (notebook);

  label = gtk_label_new (_("4x4 quant. matrices"));
  gtk_widget_show (label);

  page = _cqm_4x4_page (gconfig);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page, label);
  gtk_widget_show (page);

  label = gtk_label_new (_("8x8 I luma quant. matrices"));
  gtk_widget_show (label);

  page = _cqm_8x8_iy_page (gconfig);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page, label);
  gtk_widget_show (page);

  label = gtk_label_new (_("8x8 P luma quant. matrices"));
  gtk_widget_show (label);

  page = _cqm_8x8_py_page (gconfig);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page, label);
  gtk_widget_show (page);

  return vbox;
}

static GtkWidget *
_cqm_4x4_page   (X264_Gui_Config *gconfig)
{
  GtkWidget     *table;
  GtkWidget     *misc;
  GtkWidget     *frame;
  GtkWidget     *t;
  GtkRequisition size;
  gint           i;

  misc = gtk_entry_new_with_max_length (3);
  gtk_widget_size_request (misc, &size);

  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_container_set_border_width (GTK_CONTAINER (table), 6);

  frame = gtk_frame_new (_("4x4 I luma quant. matrices"));
  gtk_table_attach_defaults (GTK_TABLE (table), frame,
                             0, 1, 0, 1);
  gtk_widget_show (frame);

  t = gtk_table_new (4, 4, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (t), 6);
  gtk_table_set_col_spacings (GTK_TABLE (t), 6);
  gtk_container_set_border_width (GTK_CONTAINER (t), 6);
  gtk_container_add (GTK_CONTAINER (frame), t);
  gtk_widget_show (t);

  for (i = 0; i < 16; i++) {
    gconfig->cqm.cqm_4iy[i] = gtk_entry_new_with_max_length (3);
    gtk_widget_set_size_request (gconfig->cqm.cqm_4iy[i], 25, size.height);
    gtk_table_attach_defaults (GTK_TABLE (t), gconfig->cqm.cqm_4iy[i],
                               i >> 2, (i >> 2) + 1, i & 3, (i & 3) + 1);
    gtk_widget_show (gconfig->cqm.cqm_4iy[i]);
  }

  frame = gtk_frame_new (_("4x4 I chroma quant. matrices"));
  gtk_table_attach_defaults (GTK_TABLE (table), frame,
                             0, 1, 1, 2);
  gtk_widget_show (frame);

  t = gtk_table_new (4, 4, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (t), 6);
  gtk_table_set_col_spacings (GTK_TABLE (t), 6);
  gtk_container_set_border_width (GTK_CONTAINER (t), 6);
  gtk_container_add (GTK_CONTAINER (frame), t);
  gtk_widget_show (t);

  for (i = 0; i < 16; i++) {
    gconfig->cqm.cqm_4ic[i] = gtk_entry_new_with_max_length (3);
    gtk_widget_set_size_request (gconfig->cqm.cqm_4ic[i], 25, size.height);
    gtk_table_attach_defaults (GTK_TABLE (t), gconfig->cqm.cqm_4ic[i],
                               i >> 2, (i >> 2) + 1, i & 3, (i & 3) + 1);
    gtk_widget_show (gconfig->cqm.cqm_4ic[i]);
  }

  frame = gtk_frame_new (_("4x4 P luma quant. matrix"));
  gtk_table_attach_defaults (GTK_TABLE (table), frame,
                             1, 2, 0, 1);
  gtk_widget_show (frame);

  t = gtk_table_new (4, 4, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (t), 6);
  gtk_table_set_col_spacings (GTK_TABLE (t), 6);
  gtk_container_set_border_width (GTK_CONTAINER (t), 6);
  gtk_container_add (GTK_CONTAINER (frame), t);
  gtk_widget_show (t);

  for (i = 0; i < 16; i++) {
    gconfig->cqm.cqm_4py[i] = gtk_entry_new_with_max_length (3);
    gtk_widget_set_size_request (gconfig->cqm.cqm_4py[i], 25, size.height);
    gtk_table_attach_defaults (GTK_TABLE (t), gconfig->cqm.cqm_4py[i],
                               i >> 2, (i >> 2) + 1, i & 3, (i & 3) + 1);
    gtk_widget_show (gconfig->cqm.cqm_4py[i]);
  }

  frame = gtk_frame_new (_("4x4 P chroma quant. matrix"));
  gtk_table_attach_defaults (GTK_TABLE (table), frame,
                             1, 2, 1, 2);
  gtk_widget_show (frame);

  t = gtk_table_new (4, 4, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (t), 6);
  gtk_table_set_col_spacings (GTK_TABLE (t), 6);
  gtk_container_set_border_width (GTK_CONTAINER (t), 6);
  gtk_container_add (GTK_CONTAINER (frame), t);
  gtk_widget_show (t);

  for (i = 0; i < 16; i++) {
    gconfig->cqm.cqm_4pc[i] = gtk_entry_new_with_max_length (3);
    gtk_widget_set_size_request (gconfig->cqm.cqm_4pc[i], 25, size.height);
    gtk_table_attach_defaults (GTK_TABLE (t), gconfig->cqm.cqm_4pc[i],
                               i >> 2, (i >> 2) + 1, i & 3, (i & 3) + 1);
    gtk_widget_show (gconfig->cqm.cqm_4pc[i]);
  }

  return table;
}

static GtkWidget *
_cqm_8x8_iy_page (X264_Gui_Config *gconfig)
{
  GtkWidget     *table;
  GtkWidget     *misc;
  GtkRequisition size;
  gint           i;

  misc = gtk_entry_new_with_max_length (3);
  gtk_widget_size_request (misc, &size);

  table = gtk_table_new (8, 8, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_container_set_border_width (GTK_CONTAINER (table), 6);

  for (i = 0; i < 64; i++) {
    gconfig->cqm.cqm_8iy[i] = gtk_entry_new_with_max_length (3);
    gtk_widget_set_size_request (gconfig->cqm.cqm_8iy[i], 25, size.height);
    gtk_table_attach_defaults (GTK_TABLE (table), gconfig->cqm.cqm_8iy[i],
                               i >> 3, (i >> 3) + 1, i & 7, (i & 7) + 1);
    gtk_widget_show (gconfig->cqm.cqm_8iy[i]);
  }

  return table;
}

static GtkWidget *
_cqm_8x8_py_page (X264_Gui_Config *gconfig)
{
  GtkWidget     *table;
  GtkWidget     *misc;
  GtkRequisition size;
  gint           i;

  misc = gtk_entry_new_with_max_length (3);
  gtk_widget_size_request (misc, &size);

  table = gtk_table_new (8, 8, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_container_set_border_width (GTK_CONTAINER (table), 6);

  for (i = 0; i < 64; i++) {
    gconfig->cqm.cqm_8py[i] = gtk_entry_new_with_max_length (3);
    gtk_widget_set_size_request (gconfig->cqm.cqm_8py[i], 25, size.height);
    gtk_table_attach_defaults (GTK_TABLE (table), gconfig->cqm.cqm_8py[i],
                               i >> 3, (i >> 3) + 1, i & 7, (i & 7) + 1);
    gtk_widget_show (gconfig->cqm.cqm_8py[i]);
  }

  return table;
}

/* Callbacks */
static void
_cqm_flat_matrix_cb (GtkToggleButton *togglebutton,
                     gpointer         user_data)
{
  X264_Gui_Config *gconfig;

  gconfig = (X264_Gui_Config *)user_data;
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (gconfig->cqm.radio_flat))) {
    gint i;

    gtk_widget_set_sensitive (gconfig->cqm.cqm_file, FALSE);
    for (i = 0; i < 16; i++) {
      gtk_widget_set_sensitive (gconfig->cqm.cqm_4iy[i], FALSE);
      gtk_widget_set_sensitive (gconfig->cqm.cqm_4ic[i], FALSE);
      gtk_widget_set_sensitive (gconfig->cqm.cqm_4py[i], FALSE);
      gtk_widget_set_sensitive (gconfig->cqm.cqm_4pc[i], FALSE);
    }
    for (i = 0; i < 64; i++) {
      gtk_widget_set_sensitive (gconfig->cqm.cqm_8iy[i], FALSE);
      gtk_widget_set_sensitive (gconfig->cqm.cqm_8py[i], FALSE);
    }
  }
}

static void
_cqm_jvt_matrix_cb (GtkToggleButton *togglebutton,
                    gpointer         user_data)
{
  X264_Gui_Config *gconfig;

  gconfig = (X264_Gui_Config *)user_data;
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (gconfig->cqm.radio_jvt))) {
    gint i;

    gtk_widget_set_sensitive (gconfig->cqm.cqm_file, FALSE);
    for (i = 0; i < 16; i++) {
      gtk_widget_set_sensitive (gconfig->cqm.cqm_4iy[i], FALSE);
      gtk_widget_set_sensitive (gconfig->cqm.cqm_4ic[i], FALSE);
      gtk_widget_set_sensitive (gconfig->cqm.cqm_4py[i], FALSE);
      gtk_widget_set_sensitive (gconfig->cqm.cqm_4pc[i], FALSE);
    }
    for (i = 0; i < 64; i++) {
      gtk_widget_set_sensitive (gconfig->cqm.cqm_8iy[i], FALSE);
      gtk_widget_set_sensitive (gconfig->cqm.cqm_8py[i], FALSE);
    }
  }
}

static void
_cqm_custom_matrix_cb (GtkToggleButton *togglebutton,
                       gpointer         user_data)
{
  X264_Gui_Config *gconfig;

  gconfig = (X264_Gui_Config *)user_data;
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (gconfig->cqm.radio_custom))) {
    gint i;

    gtk_widget_set_sensitive (gconfig->cqm.cqm_file, TRUE);
    for (i = 0; i < 16; i++) {
      gtk_widget_set_sensitive (gconfig->cqm.cqm_4iy[i], TRUE);
      gtk_widget_set_sensitive (gconfig->cqm.cqm_4ic[i], TRUE);
      gtk_widget_set_sensitive (gconfig->cqm.cqm_4py[i], TRUE);
      gtk_widget_set_sensitive (gconfig->cqm.cqm_4pc[i], TRUE);
    }
    for (i = 0; i < 64; i++) {
      gtk_widget_set_sensitive (gconfig->cqm.cqm_8iy[i], TRUE);
      gtk_widget_set_sensitive (gconfig->cqm.cqm_8py[i], TRUE);
    }
  }
}

static gboolean
_set_coefs (int size, GtkWidget **entries, GIOChannel *file)
{
  gchar *line;
  gsize  length;
  int    i = 0;
  int    offset = 0;

  while (i < size) {
    gchar **coefs;
    int     j;

    if (g_io_channel_read_line (file, &line, &length, NULL, NULL) != G_IO_STATUS_NORMAL) {
      g_print ("Not a JM custom AVC matrix compliant file\n");
      return FALSE;
    }
    if ((line[0] == '\0') || (line[0] == '\n') || (line[0] == '\r') || (line[0] == '#')) {
      g_free (line);
      continue;
    }
    coefs = g_strsplit (line, ",", size + 1);
    for (j = 0; j < size; j++) {
      gtk_entry_set_text (GTK_ENTRY (entries[offset]), coefs[j]);
      offset++;
    }
    g_strfreev (coefs);
    g_free (line);
    i++;
  }
  return TRUE;
}

static void
_cqm_matrix_file_cb (GtkFileChooser  *filechooser,
                     gint             response,
                     gpointer         user_data)
{
  X264_Gui_Config *gconfig;
  GIOChannel      *file;
  GError          *error = NULL;
  gchar           *filename;
  gchar           *line;
  gsize            length;

  if (!user_data)
    return;

  gconfig = (X264_Gui_Config *)user_data;

  filename = gtk_file_chooser_get_filename (filechooser);
  file = g_io_channel_new_file (filename, "r", &error);
  if (error) {
    g_print ("Can not open file %s\n", filename);
    g_free (filename);

    return;
  }

  while ((g_io_channel_read_line (file, &line, &length, NULL, NULL) == G_IO_STATUS_NORMAL)) {
    if (!line) continue;
    if ((line[0] == '\0') || (line[0] == '\n') || (line[0] == '\r') || (line[0] == '#')) {
      g_free (line);
      continue;
    }
    if (g_str_has_prefix (line, "INTRA4X4_LUMA")) {
      g_free (line);
      if (!_set_coefs (4, gconfig->cqm.cqm_4iy, file)) {
        g_free (filename);
        return;
      }
      continue;
    }
    if (g_str_has_prefix (line, "INTRA4X4_CHROMAU")) {
      g_free (line);
      if (!_set_coefs (4, gconfig->cqm.cqm_4ic, file)) {
        g_free (filename);
        return;
      }
      continue;
    }
    if (g_str_has_prefix (line, "INTER4X4_LUMA")) {
      g_free (line);
      if (!_set_coefs (4, gconfig->cqm.cqm_4py, file)) {
        g_free (filename);
        return;
      }
      continue;
    }

    if (g_str_has_prefix (line, "INTER4X4_CHROMAU")) {
      g_free (line);
      if (!_set_coefs (4, gconfig->cqm.cqm_4pc, file)) {
        g_free (filename);
        return;
      }
      continue;
    }
    if (g_str_has_prefix (line, "INTRA8X8_LUMA")) {
      g_free (line);
      if (!_set_coefs (8, gconfig->cqm.cqm_8iy, file)) {
        g_free (filename);
        return;
      }
      continue;
    }
    if (g_str_has_prefix (line, "INTER8X8_LUMA")) {
      g_free (line);
      if (!_set_coefs (8, gconfig->cqm.cqm_8py, file)) {
        g_free (filename);
        return;
      }
      continue;
    }
  }
  g_free (filename);
}

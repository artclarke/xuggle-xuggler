#include <gtk/gtk.h>

#include "x264_gtk_i18n.h"
#include "x264_gtk_private.h"


/* Callbacks */
static void _mb_bframe_pyramid (GtkToggleButton *button,
                                gpointer         user_data);
static void _mb_inter_search_8 (GtkToggleButton *button,
                                gpointer         user_data);
static void _mb_transform_8x8  (GtkToggleButton *button,
                                gpointer         user_data);


GtkWidget *
_mb_page (X264_Gui_Config *config)
{
  GtkWidget   *vbox;
  GtkWidget   *frame;
  GtkWidget   *vbox2;
  GtkWidget   *table;
  GtkWidget   *eb;
  GtkWidget   *label;
  GtkTooltips *tooltips;

  tooltips = gtk_tooltips_new ();

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);

  /* Partitions */
  frame = gtk_frame_new (_("Partitions"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, TRUE, 6);
  gtk_widget_show (frame);

  vbox2 = gtk_vbox_new (FALSE, 6);
  gtk_container_set_border_width (GTK_CONTAINER (vbox2), 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox2);
  gtk_widget_show (vbox2);

  config->mb.partitions.transform_8x8 = gtk_check_button_new_with_label (_("8x8 Transform"));
  gtk_tooltips_set_tip (tooltips, config->mb.partitions.transform_8x8,
                        _("8x8 Transform - description"),
                        "");
  g_signal_connect (G_OBJECT (config->mb.partitions.transform_8x8),
                    "toggled",
                    G_CALLBACK (_mb_transform_8x8), config);
  gtk_box_pack_start (GTK_BOX (vbox2), config->mb.partitions.transform_8x8, FALSE, TRUE, 0);
  gtk_widget_show (config->mb.partitions.transform_8x8);

  config->mb.partitions.pframe_search_8 = gtk_check_button_new_with_label (_("8x16, 16x8 and 8x8 P-frame search"));
  gtk_tooltips_set_tip (tooltips, config->mb.partitions.pframe_search_8,
                        _("8x16, 16x8 and 8x8 P-frame search - description"),
                        "");
  gtk_box_pack_start (GTK_BOX (vbox2), config->mb.partitions.pframe_search_8, FALSE, TRUE, 0);
  gtk_widget_show (config->mb.partitions.pframe_search_8);

  config->mb.partitions.bframe_search_8 = gtk_check_button_new_with_label (_("8x16, 16x8 and 8x8 B-frame search"));
  gtk_tooltips_set_tip (tooltips, config->mb.partitions.bframe_search_8,
                        _("8x16, 16x8 and 8x8 B-frame search - description"),
                        "");
  gtk_box_pack_start (GTK_BOX (vbox2), config->mb.partitions.bframe_search_8, FALSE, TRUE, 0);
  gtk_widget_show (config->mb.partitions.bframe_search_8);

  config->mb.partitions.pframe_search_4 = gtk_check_button_new_with_label (_("4x8, 8x4 and 4x4 P-frame search"));
  gtk_tooltips_set_tip (tooltips, config->mb.partitions.pframe_search_4,
                        _("4x8, 8x4 and 4x4 P-frame search - description"),
                        "");
  gtk_box_pack_start (GTK_BOX (vbox2), config->mb.partitions.pframe_search_4, FALSE, TRUE, 0);
  gtk_widget_show (config->mb.partitions.pframe_search_4);

  config->mb.partitions.inter_search_8 = gtk_check_button_new_with_label (_("8x8 Intra search"));
  gtk_tooltips_set_tip (tooltips, config->mb.partitions.inter_search_8,
                        _("8x8 Intra search - description"),
                        "");
  g_signal_connect (G_OBJECT (config->mb.partitions.inter_search_8),
                    "toggled",
                    G_CALLBACK (_mb_inter_search_8), config);
  gtk_box_pack_start (GTK_BOX (vbox2), config->mb.partitions.inter_search_8, FALSE, TRUE, 0);
  gtk_widget_show (config->mb.partitions.inter_search_8);

  config->mb.partitions.inter_search_4 = gtk_check_button_new_with_label (_("4x4 Intra search"));
  gtk_tooltips_set_tip (tooltips, config->mb.partitions.inter_search_4,
                        _("4x4 Intra search - description"),
                        "");
  gtk_box_pack_start (GTK_BOX (vbox2), config->mb.partitions.inter_search_4, FALSE, TRUE, 0);
  gtk_widget_show (config->mb.partitions.inter_search_4);

  /* B-Frames */
  frame = gtk_frame_new (_("B-Frames"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, TRUE, 6);
  gtk_widget_show (frame);

  table = gtk_table_new (5, 2, TRUE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_container_set_border_width (GTK_CONTAINER (table), 6);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  eb = gtk_event_box_new ();
  gtk_event_box_set_visible_window (GTK_EVENT_BOX (eb), FALSE);
  gtk_tooltips_set_tip (tooltips, eb,
                        _("Max consecutive - description"),
                        "");
  gtk_table_attach_defaults (GTK_TABLE (table), eb,
                             0, 1, 0, 1);
  gtk_widget_show (eb);

  label = gtk_label_new (_("Max consecutive"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_container_add (GTK_CONTAINER (eb), label);
  gtk_widget_show (label);

  config->mb.bframes.bframe = gtk_entry_new_with_max_length (3);
  gtk_table_attach_defaults (GTK_TABLE (table), config->mb.bframes.bframe,
                             1, 2, 0, 1);
  gtk_widget_show (config->mb.bframes.bframe);

  eb = gtk_event_box_new ();
  gtk_event_box_set_visible_window (GTK_EVENT_BOX (eb), FALSE);
  gtk_tooltips_set_tip (tooltips, eb,
                        _("Bias - description"),
                        "");
  gtk_table_attach_defaults (GTK_TABLE (table), eb,
                             0, 1, 1, 2);
  gtk_widget_show (eb);

  label = gtk_label_new (_("Bias"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_container_add (GTK_CONTAINER (eb), label);
  gtk_widget_show (label);

  config->mb.bframes.bframe_bias = gtk_hscale_new_with_range (-100.0, 100.0, 1.0);
  gtk_scale_set_digits (GTK_SCALE (config->mb.bframes.bframe_bias), 0);
  gtk_scale_set_value_pos (GTK_SCALE (config->mb.bframes.bframe_bias), GTK_POS_RIGHT);
  gtk_table_attach_defaults (GTK_TABLE (table), config->mb.bframes.bframe_bias,
                             1, 2, 1, 2);
  gtk_widget_show (config->mb.bframes.bframe_bias);

  config->mb.bframes.bframe_pyramid = gtk_check_button_new_with_label (_("Use as references"));
  gtk_tooltips_set_tip (tooltips, config->mb.bframes.bframe_pyramid,
                        _("Use as references - description"),
                        "");
  g_signal_connect (G_OBJECT (config->mb.bframes.bframe_pyramid),
                    "toggled",
                    G_CALLBACK (_mb_bframe_pyramid), config);
  gtk_table_attach_defaults (GTK_TABLE (table), config->mb.bframes.bframe_pyramid,
                             0, 1, 2, 3);
  gtk_widget_show (config->mb.bframes.bframe_pyramid);

  config->mb.bframes.bidir_me = gtk_check_button_new_with_label (_("Bidirectional ME"));
  gtk_tooltips_set_tip (tooltips, config->mb.bframes.bidir_me,
                        _("Bidirectional ME - description"),
                        "");
  gtk_table_attach_defaults (GTK_TABLE (table), config->mb.bframes.bidir_me,
                             1, 2, 2, 3);
  gtk_widget_show (config->mb.bframes.bidir_me);

  config->mb.bframes.bframe_adaptive = gtk_check_button_new_with_label (_("Adaptive"));
  gtk_tooltips_set_tip (tooltips, config->mb.bframes.bframe_adaptive,
                        _("Adaptive - description"),
                        "");
  gtk_table_attach_defaults (GTK_TABLE (table), config->mb.bframes.bframe_adaptive,
                             0, 1, 3, 4);
  gtk_widget_show (config->mb.bframes.bframe_adaptive);

  config->mb.bframes.weighted_bipred = gtk_check_button_new_with_label (_("Weighted biprediction"));
  gtk_tooltips_set_tip (tooltips, config->mb.bframes.weighted_bipred,
                        _("Weighted biprediction - description"),
                        "");
  gtk_table_attach_defaults (GTK_TABLE (table), config->mb.bframes.weighted_bipred,
                             1, 2, 3, 4);
  gtk_widget_show (config->mb.bframes.weighted_bipred);

  eb = gtk_event_box_new ();
  gtk_event_box_set_visible_window (GTK_EVENT_BOX (eb), FALSE);
  gtk_tooltips_set_tip (tooltips, eb,
                        _("Direct mode - description"),
                        "");
  gtk_table_attach_defaults (GTK_TABLE (table), eb,
                             0, 1, 4, 5);
  gtk_widget_show (eb);

  label = gtk_label_new (_("Direct mode"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_container_add (GTK_CONTAINER (eb), label);
  gtk_widget_show (label);

  config->mb.bframes.direct_mode = gtk_combo_box_new_text ();
  gtk_combo_box_append_text (GTK_COMBO_BOX (config->mb.bframes.direct_mode),
                             _("None"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (config->mb.bframes.direct_mode),
                             _("Spatial"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (config->mb.bframes.direct_mode),
                             _("Temporal"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (config->mb.bframes.direct_mode),
                             _("Auto"));
  gtk_table_attach_defaults (GTK_TABLE (table),
                             config->mb.bframes.direct_mode,
                             1, 2, 4, 5);
  gtk_widget_show (config->mb.bframes.direct_mode);

  return vbox;
}

static void
_mb_bframe_pyramid (GtkToggleButton *button,
                    gpointer         user_data)
{
  X264_Gui_Config *config;

  config = (X264_Gui_Config *)user_data;

  if (gtk_toggle_button_get_active (button)) {
    const gchar *text;
    gint         val;

    text = gtk_entry_get_text (GTK_ENTRY (config->mb.bframes.bframe));
    val = (gint)g_ascii_strtoull (text, NULL, 10);
    if (val < 2)
      gtk_entry_set_text (GTK_ENTRY (config->mb.bframes.bframe), "2");
  }
}

static void
_mb_inter_search_8 (GtkToggleButton *button,
                    gpointer         user_data)
{
  X264_Gui_Config *config;

  config = (X264_Gui_Config *)user_data;

  if (gtk_toggle_button_get_active (button)) {
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (config->mb.partitions.transform_8x8), TRUE);
  }
}

static void
_mb_transform_8x8 (GtkToggleButton *button,
                   gpointer         user_data)
{
  X264_Gui_Config *config;

  config = (X264_Gui_Config *)user_data;

  if (!gtk_toggle_button_get_active (button)) {
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (config->mb.partitions.inter_search_8), FALSE);
  }
}

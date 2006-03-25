#include <gtk/gtk.h>

#include "x264_gtk_private.h"


/* Callbacks */
static void _more_deblocking_filter (GtkToggleButton *button,
                                     gpointer         user_data);
static void _more_cabac             (GtkToggleButton *button,
                                     gpointer         user_data);


GtkWidget *
_more_page (X264_Gui_Config *config)
{
  GtkWidget     *vbox;
  GtkWidget     *frame;
  GtkWidget     *hbox;
  GtkWidget     *table;
  GtkWidget     *label;
  GtkRequisition size;
  GtkRequisition size2;
  GtkRequisition size3;
  GtkRequisition size4;
  GtkRequisition size5;

  label = gtk_entry_new_with_max_length (3);
  gtk_widget_size_request (label, &size);
  gtk_widget_destroy (GTK_WIDGET (label));

  label = gtk_check_button_new_with_label ("Deblocking Filter");
  gtk_widget_size_request (label, &size2);
  gtk_widget_destroy (GTK_WIDGET (label));

  label = gtk_label_new ("Partition decision");
  gtk_widget_size_request (label, &size3);
  gtk_widget_destroy (GTK_WIDGET (label));

  label = gtk_label_new ("Threshold");
  gtk_widget_size_request (label, &size5);
  gtk_widget_destroy (GTK_WIDGET (label));

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);

  /* Motion Estimation */
  frame = gtk_frame_new ("Motion Estimation");
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, TRUE, 6);
  gtk_widget_show (frame);

  table = gtk_table_new (4, 3, TRUE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_container_set_border_width (GTK_CONTAINER (table), 6);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  label = gtk_label_new ("Partition decision");
  gtk_widget_set_size_request (label, size2.width, size3.height);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label,
                             0, 2, 0, 1);
  gtk_widget_show (label);

  config->more.motion_estimation.partition_decision = gtk_combo_box_new_text ();
  gtk_combo_box_append_text (GTK_COMBO_BOX (config->more.motion_estimation.partition_decision),
                             "1 (Fastest)");
  gtk_combo_box_append_text (GTK_COMBO_BOX (config->more.motion_estimation.partition_decision),
                             "2");
  gtk_combo_box_append_text (GTK_COMBO_BOX (config->more.motion_estimation.partition_decision),
                             "3");
  gtk_combo_box_append_text (GTK_COMBO_BOX (config->more.motion_estimation.partition_decision),
                             "4");
  gtk_combo_box_append_text (GTK_COMBO_BOX (config->more.motion_estimation.partition_decision),
                             "5 (High quality)");
  gtk_combo_box_append_text (GTK_COMBO_BOX (config->more.motion_estimation.partition_decision),
                             "6 (RDO)");
  gtk_combo_box_append_text (GTK_COMBO_BOX (config->more.motion_estimation.partition_decision),
                             "6b (RDO on B frames)");
  gtk_table_attach_defaults (GTK_TABLE (table), config->more.motion_estimation.partition_decision,
                             1, 3, 0, 1);
  gtk_widget_show (config->more.motion_estimation.partition_decision);

  label = gtk_label_new ("Method");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label,
                             0, 2, 1, 2);
  gtk_widget_show (label);

  config->more.motion_estimation.method = gtk_combo_box_new_text ();
  gtk_combo_box_append_text (GTK_COMBO_BOX (config->more.motion_estimation.method),
                             "Diamond Search");
  gtk_combo_box_append_text (GTK_COMBO_BOX (config->more.motion_estimation.method),
                             "Hexagonal Search");
  gtk_combo_box_append_text (GTK_COMBO_BOX (config->more.motion_estimation.method),
                             "Uneven Multi-Hexagon");
  gtk_combo_box_append_text (GTK_COMBO_BOX (config->more.motion_estimation.method),
                             "Exhaustive search");
  gtk_table_attach_defaults (GTK_TABLE (table), config->more.motion_estimation.method,
                             1, 3, 1, 2);
  gtk_widget_show (config->more.motion_estimation.method);

  label = gtk_label_new ("Range");
  gtk_widget_size_request (label, &size4);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label,
                             0, 1, 2, 3);
  gtk_widget_show (label);

  config->more.motion_estimation.range = gtk_entry_new_with_max_length (3);
  gtk_widget_set_size_request (config->more.motion_estimation.range,
                               20, size.height);
  gtk_table_attach_defaults (GTK_TABLE (table), config->more.motion_estimation.range,
                             1, 2, 2, 3);
  gtk_widget_show (config->more.motion_estimation.range);

  config->more.motion_estimation.chroma_me = gtk_check_button_new_with_label ("Chroma ME");
  gtk_table_attach_defaults (GTK_TABLE (table), config->more.motion_estimation.chroma_me,
                             2, 3, 2, 3);
  gtk_widget_show (config->more.motion_estimation.chroma_me);

  label = gtk_label_new ("Max Ref. frames");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label,
                             0, 1, 3, 4);
  gtk_widget_show (label);

  config->more.motion_estimation.max_ref_frames = gtk_entry_new_with_max_length (3);
  gtk_widget_set_size_request (config->more.motion_estimation.max_ref_frames,
                               20, size.height);
  gtk_table_attach_defaults (GTK_TABLE (table), config->more.motion_estimation.max_ref_frames,
                             1, 2, 3, 4);
  gtk_widget_show (config->more.motion_estimation.max_ref_frames);

  config->more.motion_estimation.mixed_refs = gtk_check_button_new_with_label ("Mixed Refs");
  gtk_table_attach_defaults (GTK_TABLE (table), config->more.motion_estimation.mixed_refs,
                             2, 3, 3, 4);
  gtk_widget_show (config->more.motion_estimation.mixed_refs);

  /* Misc. Options */
  frame = gtk_frame_new ("Misc. Options");
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, TRUE, 6);
  gtk_widget_show (frame);

  table = gtk_table_new (5, 4, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_container_set_border_width (GTK_CONTAINER (table), 6);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  label = gtk_label_new ("Sample AR");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label,
                             0, 1, 0, 1);
  gtk_widget_show (label);

  hbox = gtk_hbox_new (TRUE, 6);
  gtk_table_attach_defaults (GTK_TABLE (table), hbox,
                             1, 2, 0, 1);
  gtk_widget_show (hbox);

  config->more.misc.sample_ar_x = gtk_entry_new_with_max_length (3);
  gtk_widget_set_size_request (config->more.misc.sample_ar_x, 25, size.height);
  gtk_box_pack_start (GTK_BOX (hbox), config->more.misc.sample_ar_x, FALSE, TRUE, 0);
  gtk_widget_show (config->more.misc.sample_ar_x);

  config->more.misc.sample_ar_y = gtk_entry_new_with_max_length (3);
  gtk_widget_set_size_request (config->more.misc.sample_ar_y, 25, size.height);
  gtk_box_pack_start (GTK_BOX (hbox), config->more.misc.sample_ar_y, FALSE, TRUE, 0);
  gtk_widget_show (config->more.misc.sample_ar_y);

  label = gtk_label_new ("Threads");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label,
                             2, 3, 0, 1);
  gtk_widget_show (label);

  config->more.misc.threads = gtk_entry_new_with_max_length (3);
  gtk_widget_set_size_request (config->more.misc.threads, size5.width, size.height);
  gtk_table_attach_defaults (GTK_TABLE (table),
                             config->more.misc.threads,
                             3, 4, 0, 1);
  gtk_widget_show (config->more.misc.threads);

  config->more.misc.cabac = gtk_check_button_new_with_label ("CABAC");
  gtk_widget_set_size_request (config->more.misc.cabac, size5.width, size.height);
  g_signal_connect (G_OBJECT (config->more.misc.cabac),
                    "toggled",
                    G_CALLBACK (_more_cabac), config);
  gtk_table_attach_defaults (GTK_TABLE (table), config->more.misc.cabac,
                             0, 1, 1, 2);
  gtk_widget_show (config->more.misc.cabac);

  label = gtk_label_new ("Trellis");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label,
                             1, 2, 1, 2);
  gtk_widget_show (label);

  config->more.misc.trellis = gtk_combo_box_new_text ();
  gtk_combo_box_append_text (GTK_COMBO_BOX (config->more.misc.trellis),
                             "Disabled");
  gtk_combo_box_append_text (GTK_COMBO_BOX (config->more.misc.trellis),
                             "Enabled (once)");
  gtk_combo_box_append_text (GTK_COMBO_BOX (config->more.misc.trellis),
                             "Enabled (mode decision)");
  gtk_table_attach_defaults (GTK_TABLE (table), config->more.misc.trellis,
                             2, 4, 1, 2);
  gtk_widget_show (config->more.misc.trellis);

  label = gtk_label_new ("Noise reduction");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label,
                             0, 1, 2, 3);
  gtk_widget_show (label);

  config->more.misc.noise_reduction = gtk_entry_new_with_max_length (3);
  gtk_widget_set_size_request (config->more.misc.noise_reduction, size5.width, size.height);
  gtk_table_attach_defaults (GTK_TABLE (table), config->more.misc.noise_reduction,
                             1, 2, 2, 3);
  gtk_widget_show (config->more.misc.noise_reduction);

  config->more.misc.df.deblocking_filter = gtk_check_button_new_with_label ("Deblocking Filter");
  g_signal_connect (G_OBJECT (config->more.misc.df.deblocking_filter),
                    "toggled",
                    G_CALLBACK (_more_deblocking_filter), config);
  gtk_table_attach_defaults (GTK_TABLE (table), config->more.misc.df.deblocking_filter,
                             0, 1, 3, 4);
  gtk_widget_show (config->more.misc.df.deblocking_filter);

  label = gtk_label_new ("Strength");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_widget_set_size_request (label, size5.width, size4.height);
  gtk_table_attach_defaults (GTK_TABLE (table), label,
                             1, 2, 3, 4);
  gtk_widget_show (label);

  config->more.misc.df.strength = gtk_hscale_new_with_range (-6.0, 6.0, 1.0);
  gtk_widget_size_request (config->more.misc.df.strength, &size4);
  gtk_scale_set_digits (GTK_SCALE (config->more.misc.df.strength), 0);
  gtk_scale_set_value_pos (GTK_SCALE (config->more.misc.df.strength), GTK_POS_RIGHT);
  //  gtk_widget_set_size_request (config->more.misc.df.strength, size5.width, size4.height);
  gtk_table_attach_defaults (GTK_TABLE (table), config->more.misc.df.strength,
                             2, 4, 3, 4);
  gtk_widget_show (config->more.misc.df.strength);

  label = gtk_label_new ("Threshold");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_widget_set_size_request (label, size5.width, size4.height);
  gtk_table_attach_defaults (GTK_TABLE (table), label,
                             1, 2, 4, 5);
  gtk_widget_show (label);

  config->more.misc.df.threshold = gtk_hscale_new_with_range (-6.0, 6.0, 1.0);
  gtk_scale_set_digits (GTK_SCALE (config->more.misc.df.threshold), 0);
  gtk_scale_set_value_pos (GTK_SCALE (config->more.misc.df.threshold), GTK_POS_RIGHT);
  gtk_table_attach_defaults (GTK_TABLE (table), config->more.misc.df.threshold,
                             2, 4, 4, 5);
  gtk_widget_show (config->more.misc.df.threshold);

  /* Debug */
  frame = gtk_frame_new ("Debug");
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, TRUE, 6);
  gtk_widget_show (frame);

  table = gtk_table_new (2, 2, TRUE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_container_set_border_width (GTK_CONTAINER (table), 6);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  label = gtk_label_new ("Log level");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_widget_set_size_request (label, size2.width, size3.height);
  gtk_table_attach_defaults (GTK_TABLE (table), label,
                             0, 1, 0, 1);
  gtk_widget_show (label);

  config->more.debug.log_level = gtk_combo_box_new_text ();
  gtk_combo_box_append_text (GTK_COMBO_BOX (config->more.debug.log_level),
                             "None");
  gtk_combo_box_append_text (GTK_COMBO_BOX (config->more.debug.log_level),
                             "Error");
  gtk_combo_box_append_text (GTK_COMBO_BOX (config->more.debug.log_level),
                             "Warning");
  gtk_combo_box_append_text (GTK_COMBO_BOX (config->more.debug.log_level),
                             "Info");
  gtk_combo_box_append_text (GTK_COMBO_BOX (config->more.debug.log_level),
                             "Debug");
  gtk_table_attach_defaults (GTK_TABLE (table), config->more.debug.log_level,
                             1, 2, 0, 1);
  gtk_widget_show (config->more.debug.log_level);

  label = gtk_label_new ("FourCC");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label,
                             0, 1, 1, 2);
  gtk_widget_show (label);

  config->more.debug.fourcc = gtk_entry_new_with_max_length (4);
  gtk_table_attach_defaults (GTK_TABLE (table),
                             config->more.debug.fourcc,
                             1, 2, 1, 2);
  gtk_widget_show (config->more.debug.fourcc);



  return vbox;
}

/* Callbacks */
static void
_more_deblocking_filter (GtkToggleButton *button,
                         gpointer         user_data)
{
  X264_Gui_Config *config;

  config = (X264_Gui_Config *)user_data;

  if (gtk_toggle_button_get_active (button)) {
    gtk_widget_set_sensitive (config->more.misc.df.strength, TRUE);
    gtk_widget_set_sensitive (config->more.misc.df.threshold, TRUE);
  }
  else {
    gtk_widget_set_sensitive (config->more.misc.df.strength, FALSE);
    gtk_widget_set_sensitive (config->more.misc.df.threshold, FALSE);
  }
}

static void
_more_cabac (GtkToggleButton *button,
             gpointer         user_data)
{
  X264_Gui_Config *config;

  config = (X264_Gui_Config *)user_data;

  if (gtk_toggle_button_get_active (button))
    gtk_widget_set_sensitive (config->more.misc.trellis, TRUE);
  else
    gtk_widget_set_sensitive (config->more.misc.trellis, FALSE);
}

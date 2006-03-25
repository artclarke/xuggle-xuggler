#include <gtk/gtk.h>

#include "x264_gtk_private.h"


GtkWidget *
_mb_page (X264_Gui_Config *config)
{
  GtkWidget   *vbox;
  GtkWidget   *frame;
  GtkWidget   *vbox2;
  GtkWidget   *table;
  GtkWidget   *label;

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);

  /* Partitions */
  frame = gtk_frame_new ("Partitions");
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, TRUE, 6);
  gtk_widget_show (frame);

  vbox2 = gtk_vbox_new (FALSE, 6);
  gtk_container_set_border_width (GTK_CONTAINER (vbox2), 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox2);
  gtk_widget_show (vbox2);

  config->mb.partitions.transform_8x8 = gtk_check_button_new_with_label ("8x8 Transform");
  gtk_box_pack_start (GTK_BOX (vbox2), config->mb.partitions.transform_8x8, FALSE, TRUE, 0);
  gtk_widget_show (config->mb.partitions.transform_8x8);

  config->mb.partitions.pframe_search_8 = gtk_check_button_new_with_label ("8x16, 16x8 and 8x8 P-frame search");
  gtk_box_pack_start (GTK_BOX (vbox2), config->mb.partitions.pframe_search_8, FALSE, TRUE, 0);
  gtk_widget_show (config->mb.partitions.pframe_search_8);

  config->mb.partitions.bframe_search_8 = gtk_check_button_new_with_label ("8x16, 16x8 and 8x8 B-frame search");
  gtk_box_pack_start (GTK_BOX (vbox2), config->mb.partitions.bframe_search_8, FALSE, TRUE, 0);
  gtk_widget_show (config->mb.partitions.bframe_search_8);

  config->mb.partitions.pframe_search_4 = gtk_check_button_new_with_label ("4x8, 8x4 and 4x4 P-frame search");
  gtk_box_pack_start (GTK_BOX (vbox2), config->mb.partitions.pframe_search_4, FALSE, TRUE, 0);
  gtk_widget_show (config->mb.partitions.pframe_search_4);

  config->mb.partitions.inter_search_8 = gtk_check_button_new_with_label ("8x8 Intra search");
  gtk_box_pack_start (GTK_BOX (vbox2), config->mb.partitions.inter_search_8, FALSE, TRUE, 0);
  gtk_widget_show (config->mb.partitions.inter_search_8);

  config->mb.partitions.inter_search_4 = gtk_check_button_new_with_label ("4x4 Intra search");
  gtk_box_pack_start (GTK_BOX (vbox2), config->mb.partitions.inter_search_4, FALSE, TRUE, 0);
  gtk_widget_show (config->mb.partitions.inter_search_4);

  /* B-Frames */
  frame = gtk_frame_new ("B-Frames");
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, TRUE, 6);
  gtk_widget_show (frame);

  table = gtk_table_new (5, 2, TRUE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_container_set_border_width (GTK_CONTAINER (table), 6);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  label = gtk_label_new ("Max consecutive");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label,
                             0, 1, 0, 1);
  gtk_widget_show (label);

  config->mb.bframes.max_consecutive = gtk_entry_new_with_max_length (3);
  gtk_table_attach_defaults (GTK_TABLE (table), config->mb.bframes.max_consecutive,
                             1, 2, 0, 1);
  gtk_widget_show (config->mb.bframes.max_consecutive);

  label = gtk_label_new ("Bias");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label,
                             0, 1, 1, 2);
  gtk_widget_show (label);

  config->mb.bframes.bias = gtk_hscale_new_with_range (-100.0, 100.0, 1.0);
  gtk_scale_set_digits (GTK_SCALE (config->mb.bframes.bias), 0);
  gtk_scale_set_value_pos (GTK_SCALE (config->mb.bframes.bias), GTK_POS_RIGHT);
  gtk_table_attach_defaults (GTK_TABLE (table), config->mb.bframes.bias,
                             1, 2, 1, 2);
  gtk_widget_show (config->mb.bframes.bias);

  config->mb.bframes.use_as_reference = gtk_check_button_new_with_label ("Use as references");
  gtk_table_attach_defaults (GTK_TABLE (table),
                             config->mb.bframes.use_as_reference,
                             0, 1, 2, 3);
  gtk_widget_show (config->mb.bframes.use_as_reference);

  config->mb.bframes.bidir_me = gtk_check_button_new_with_label ("Bidirectional ME");
  gtk_table_attach_defaults (GTK_TABLE (table),
                             config->mb.bframes.bidir_me,
                             1, 2, 2, 3);
  gtk_widget_show (config->mb.bframes.bidir_me);

  config->mb.bframes.adaptive = gtk_check_button_new_with_label ("Adaptive");
  gtk_table_attach_defaults (GTK_TABLE (table),
                             config->mb.bframes.adaptive,
                             0, 1, 3, 4);
  gtk_widget_show (config->mb.bframes.adaptive);

  config->mb.bframes.weighted_biprediction = gtk_check_button_new_with_label ("Weighted biprediction");
  gtk_table_attach_defaults (GTK_TABLE (table),
                             config->mb.bframes.weighted_biprediction,
                             1, 2, 3, 4);
  gtk_widget_show (config->mb.bframes.weighted_biprediction);

  label = gtk_label_new ("Direct mode");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label,
                             0, 1, 4, 5);
  gtk_widget_show (label);

  config->mb.bframes.direct_mode = gtk_combo_box_new_text ();
  gtk_combo_box_append_text (GTK_COMBO_BOX (config->mb.bframes.direct_mode),
                             "None");
  gtk_combo_box_append_text (GTK_COMBO_BOX (config->mb.bframes.direct_mode),
                             "Spatial");
  gtk_combo_box_append_text (GTK_COMBO_BOX (config->mb.bframes.direct_mode),
                             "Temporal");
  gtk_table_attach_defaults (GTK_TABLE (table),
                             config->mb.bframes.direct_mode,
                             1, 2, 4, 5);
  gtk_widget_show (config->mb.bframes.direct_mode);

  return vbox;
}

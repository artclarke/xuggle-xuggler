#include <gtk/gtk.h>

#include "x264_gtk_private.h"


/* Callbacks */
static void _insert_numeric (GtkEditable *editable,
                             const gchar *text,
                             gint         length,
                             gint        *position,
                             gpointer     data);

GtkWidget *
_rate_control_page (X264_Gui_Config *config)
{
  GtkWidget     *vbox;
  GtkWidget     *frame;
  GtkWidget     *table;
  GtkWidget     *label;

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);

  /* bitrate */
  frame = gtk_frame_new ("Bitrate");
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, TRUE, 6);
  gtk_widget_show (frame);

  table = gtk_table_new (3, 2, TRUE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_container_set_border_width (GTK_CONTAINER (table), 6);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  label = gtk_label_new ("Keyframe boost (%)");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label,
                             0, 1, 0, 1);
  gtk_widget_show (label);

  config->rate_control.bitrate.keyframe_boost = gtk_entry_new_with_max_length (3);
  g_signal_connect (G_OBJECT (config->rate_control.bitrate.keyframe_boost),
                    "insert-text",
                    G_CALLBACK (_insert_numeric),
                    NULL);
  gtk_table_attach_defaults (GTK_TABLE (table),
                             config->rate_control.bitrate.keyframe_boost,
                             1, 2, 0, 1);
  gtk_widget_show (config->rate_control.bitrate.keyframe_boost);

  label = gtk_label_new ("B-frames reduction (%)");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label,
                             0, 1, 1, 2);
  gtk_widget_show (label);

  config->rate_control.bitrate.bframes_reduction = gtk_entry_new_with_max_length (5);
  gtk_table_attach_defaults (GTK_TABLE (table),
                             config->rate_control.bitrate.bframes_reduction,
                             1, 2, 1, 2);
  gtk_widget_show (config->rate_control.bitrate.bframes_reduction);

  label = gtk_label_new ("Bitrate variability (%)");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label,
                             0, 1, 2, 3);
  gtk_widget_show (label);

  config->rate_control.bitrate.bitrate_variability = gtk_entry_new_with_max_length (3);
  gtk_table_attach_defaults (GTK_TABLE (table),
                             config->rate_control.bitrate.bitrate_variability,
                             1, 2, 2, 3);
  gtk_widget_show (config->rate_control.bitrate.bitrate_variability);

  /* Quantization limits */
  frame = gtk_frame_new ("Quantization limits");
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, TRUE, 6);
  gtk_widget_show (frame);

  table = gtk_table_new (3, 2, TRUE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_container_set_border_width (GTK_CONTAINER (table), 6);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  label = gtk_label_new ("Min QP");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label,
                             0, 1, 0, 1);
  gtk_widget_show (label);

  config->rate_control.quantization_limits.min_qp = gtk_entry_new_with_max_length (3);
  gtk_table_attach_defaults (GTK_TABLE (table),
                             config->rate_control.quantization_limits.min_qp,
                             1, 2, 0, 1);
  gtk_widget_show (config->rate_control.quantization_limits.min_qp);

  label = gtk_label_new ("Max QP");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label,
                             0, 1, 1, 2);
  gtk_widget_show (label);

  config->rate_control.quantization_limits.max_qp = gtk_entry_new_with_max_length (3);
  gtk_table_attach_defaults (GTK_TABLE (table),
                             config->rate_control.quantization_limits.max_qp,
                             1, 2, 1, 2);
  gtk_widget_show (config->rate_control.quantization_limits.max_qp);

  label = gtk_label_new ("Max QP Step");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label,
                             0, 1, 2, 3);
  gtk_widget_show (label);

  config->rate_control.quantization_limits.max_qp_step = gtk_entry_new_with_max_length (3);
  gtk_table_attach_defaults (GTK_TABLE (table),
                             config->rate_control.quantization_limits.max_qp_step,
                             1, 2, 2, 3);
  gtk_widget_show (config->rate_control.quantization_limits.max_qp_step);

  /* Scene Cuts */
  frame = gtk_frame_new ("Scene Cuts");
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, TRUE, 6);
  gtk_widget_show (frame);

  table = gtk_table_new (3, 2, TRUE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_container_set_border_width (GTK_CONTAINER (table), 6);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  label = gtk_label_new ("Scene Cut Threshold");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label,
                             0, 1, 0, 1);
  gtk_widget_show (label);

  config->rate_control.scene_cuts.scene_cut_threshold = gtk_entry_new_with_max_length (3);
  gtk_table_attach_defaults (GTK_TABLE (table),
                             config->rate_control.scene_cuts.scene_cut_threshold,
                             1, 2, 0, 1);
  gtk_widget_show (config->rate_control.scene_cuts.scene_cut_threshold);

  label = gtk_label_new ("Min IDR-frame interval");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label,
                             0, 1, 1, 2);
  gtk_widget_show (label);

  config->rate_control.scene_cuts.min_idr_frame_interval = gtk_entry_new_with_max_length (3);
  gtk_table_attach_defaults (GTK_TABLE (table),
                             config->rate_control.scene_cuts.min_idr_frame_interval,
                             1, 2, 1, 2);
  gtk_widget_show (config->rate_control.scene_cuts.min_idr_frame_interval);

  label = gtk_label_new ("Max IDR-frame interval");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label,
                             0, 1, 2, 3);
  gtk_widget_show (label);

  config->rate_control.scene_cuts.max_idr_frame_interval = gtk_entry_new_with_max_length (3);
  gtk_table_attach_defaults (GTK_TABLE (table),
                             config->rate_control.scene_cuts.max_idr_frame_interval,
                             1, 2, 2, 3);
  gtk_widget_show (config->rate_control.scene_cuts.max_idr_frame_interval);


  return vbox;
}

static void
_insert_numeric (GtkEditable *editable,
                 const gchar *text,
                 gint         length,
                 gint        *position,
                 gpointer     data)
{
  gint i;
  gint j;
  gchar *result;

  result = (gchar *)g_malloc (sizeof (gchar) * (length + 1));
  if (!result)
    return;

  for (i = 0, j = 0; i < length; i++)
    {
      if (g_ascii_isdigit (text[i]))
        {
          result[j] = text[i];
          j++;
        }
    }
  result[j] = '\0';

  g_signal_handlers_block_by_func (editable,
				   (gpointer) _insert_numeric, data);
  gtk_editable_insert_text (editable, result, j, position);
  g_signal_handlers_unblock_by_func (editable,
                                     (gpointer) _insert_numeric, data);

  g_signal_stop_emission_by_name (editable, "insert-text");

  g_free (result);
}


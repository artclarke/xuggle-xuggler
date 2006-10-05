#if defined __FreeBSD__ || defined __OpenBSD__ || defined __NetBSD__ || defined __DragonFly__
#  include <inttypes.h>
#else
#  include <stdint.h>
#endif

#include <gtk/gtk.h>

#include "../x264.h"
#include "x264_gtk_i18n.h"
#include "x264_gtk_private.h"
#include "x264_gtk_enum.h"


/* Callbacks */
static void     _bitrate_pass     (GtkComboBox     *combo,
                                   gpointer         user_data);
static void     _bitrate_statfile (GtkToggleButton *button,
                                   gpointer         user_data);

GtkWidget *
_bitrate_page (X264_Gui_Config *gconfig)
{
  GtkWidget   *vbox;
  GtkWidget   *frame;
  GtkWidget   *table;
  GtkWidget   *image;
  GtkWidget   *eb;
  GtkWidget   *label;
  GtkTooltips *tooltips;

  tooltips = gtk_tooltips_new ();

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);

  image = gtk_image_new_from_file (X264_DATA_DIR "/x264/x264.png");
  gtk_box_pack_start (GTK_BOX (vbox), image, FALSE, TRUE, 6);
  gtk_widget_show (image);

  frame = gtk_frame_new (_("Main settings"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, TRUE, 6);
  gtk_widget_show (frame);

  table = gtk_table_new (2, 2, TRUE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_container_set_border_width (GTK_CONTAINER (table), 6);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  eb = gtk_event_box_new ();
  gtk_event_box_set_visible_window (GTK_EVENT_BOX (eb), FALSE);
  gtk_tooltips_set_tip (tooltips, eb,
                        _("Encoding type - description"),
                        "");
  gtk_table_attach_defaults (GTK_TABLE (table), eb,
                             0, 1, 0, 1);
  gtk_widget_show (eb);

  label = gtk_label_new (_("Encoding type"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_container_add (GTK_CONTAINER (eb), label);
  gtk_widget_show (label);

  gconfig->bitrate.pass = gtk_combo_box_new_text ();
  gtk_combo_box_append_text (GTK_COMBO_BOX (gconfig->bitrate.pass),
                             _("Single Pass - Bitrate"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (gconfig->bitrate.pass),
                             _("Single Pass - Quantizer"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (gconfig->bitrate.pass),
                             _("Multipass - First Pass"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (gconfig->bitrate.pass),
                             _("Multipass - First Pass (fast)"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (gconfig->bitrate.pass),
                             _("Multipass - Nth Pass"));
  gtk_table_attach_defaults (GTK_TABLE (table), gconfig->bitrate.pass,
                             1, 2, 0, 1);
  g_signal_connect (G_OBJECT (gconfig->bitrate.pass),
                    "changed",
                    G_CALLBACK (_bitrate_pass),
                    gconfig);
  gtk_widget_show (gconfig->bitrate.pass);

  eb = gtk_event_box_new ();
  gtk_event_box_set_visible_window (GTK_EVENT_BOX (eb), FALSE);
  gtk_tooltips_set_tip (tooltips, eb,
                        _("Quantizer - description"),
                        "");
  gtk_table_attach_defaults (GTK_TABLE (table), eb,
                             0, 1, 1, 2);
  gtk_widget_show (eb);

  gconfig->bitrate.label = gtk_label_new (_("Quantizer"));
  gtk_misc_set_alignment (GTK_MISC (gconfig->bitrate.label), 0.0, 0.5);
  gtk_container_add (GTK_CONTAINER (eb), gconfig->bitrate.label);
  gtk_widget_show (gconfig->bitrate.label);

  gconfig->bitrate.w_quantizer = gtk_hscale_new_with_range (0.0, 51.0, 1.0);
  gtk_scale_set_digits (GTK_SCALE (gconfig->bitrate.w_quantizer), 0);
  gtk_scale_set_value_pos (GTK_SCALE (gconfig->bitrate.w_quantizer), GTK_POS_RIGHT);
  gtk_table_attach_defaults (GTK_TABLE (table), gconfig->bitrate.w_quantizer,
                             1, 2, 1, 2);

  gconfig->bitrate.w_average_bitrate = gtk_entry_new_with_max_length (4095);
  gtk_table_attach_defaults (GTK_TABLE (table), gconfig->bitrate.w_average_bitrate,
                             1, 2, 1, 2);

  gconfig->bitrate.w_target_bitrate = gtk_entry_new_with_max_length (4095);
  gtk_table_attach_defaults (GTK_TABLE (table), gconfig->bitrate.w_target_bitrate,
                             1, 2, 1, 2);

  frame = gtk_frame_new (_("Statistic file"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, TRUE, 6);
  gtk_widget_show (frame);

  table = gtk_table_new (2, 2, TRUE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_container_set_border_width (GTK_CONTAINER (table), 6);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  gconfig->bitrate.update_statfile = gtk_check_button_new_with_label (_("Update statistic file"));
  gtk_tooltips_set_tip (tooltips, gconfig->bitrate.update_statfile,
                        _("Update statistic file - description"),
                        "");
  g_signal_connect (G_OBJECT (gconfig->bitrate.update_statfile),
                    "toggled",
                    G_CALLBACK (_bitrate_statfile), gconfig);
  gtk_table_attach_defaults (GTK_TABLE (table), gconfig->bitrate.update_statfile,
                             0, 1, 0, 1);
  gtk_widget_show (gconfig->bitrate.update_statfile);

  label = gtk_label_new (_("Statistic file name"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label,
                             0, 1, 1, 2);
  gtk_widget_show (label);

  gconfig->bitrate.statsfile_name = gtk_entry_new_with_max_length (4095);
  gtk_table_attach_defaults (GTK_TABLE (table), gconfig->bitrate.statsfile_name,
                             1, 2, 1, 2);
  gtk_widget_show (gconfig->bitrate.statsfile_name);

  return vbox;
}

/* Callbacks */
static void
_bitrate_pass (GtkComboBox *combo,
               gpointer    user_data)
{
  X264_Gui_Config *gconfig;

  gconfig = (X264_Gui_Config *)user_data;

  switch (gtk_combo_box_get_active (combo))
    {
    case X264_PASS_SINGLE_BITRATE:
      gtk_label_set_text (GTK_LABEL (gconfig->bitrate.label), _("Average bitrate"));
      if (!GTK_WIDGET_VISIBLE (gconfig->bitrate.w_average_bitrate)) {
        gtk_widget_hide (gconfig->bitrate.w_quantizer);
        gtk_widget_hide (gconfig->bitrate.w_target_bitrate);
        gtk_widget_show (gconfig->bitrate.w_average_bitrate);
      }
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gconfig->bitrate.update_statfile), FALSE);
      gtk_widget_set_sensitive (gconfig->bitrate.update_statfile, FALSE);
      break;
    case X264_PASS_SINGLE_QUANTIZER:
      gtk_label_set_text (GTK_LABEL (gconfig->bitrate.label), _("Quantizer"));
      if (!GTK_WIDGET_VISIBLE (gconfig->bitrate.w_quantizer)) {
        gtk_widget_hide (gconfig->bitrate.w_average_bitrate);
        gtk_widget_hide (gconfig->bitrate.w_target_bitrate);
        gtk_widget_show (gconfig->bitrate.w_quantizer);
      }
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gconfig->bitrate.update_statfile), FALSE);
      gtk_widget_set_sensitive (gconfig->bitrate.update_statfile, FALSE);
      break;
    case X264_PASS_MULTIPASS_1ST:
    case X264_PASS_MULTIPASS_1ST_FAST:
      gtk_label_set_text (GTK_LABEL (gconfig->bitrate.label), _("Target bitrate"));
      if (!GTK_WIDGET_VISIBLE (gconfig->bitrate.w_target_bitrate)) {
        gtk_widget_hide (gconfig->bitrate.w_quantizer);
        gtk_widget_hide (gconfig->bitrate.w_average_bitrate);
        gtk_widget_show (gconfig->bitrate.w_target_bitrate);
      }
      gtk_widget_set_sensitive (gconfig->bitrate.update_statfile, TRUE);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gconfig->bitrate.update_statfile), TRUE);
      break;
    case X264_PASS_MULTIPASS_NTH:
      gtk_label_set_text (GTK_LABEL (gconfig->bitrate.label), _("Target bitrate"));
      if (!GTK_WIDGET_VISIBLE (gconfig->bitrate.w_target_bitrate)) {
        gtk_widget_hide (gconfig->bitrate.w_quantizer);
        gtk_widget_hide (gconfig->bitrate.w_average_bitrate);
        gtk_widget_show (gconfig->bitrate.w_target_bitrate);
      }
      gtk_widget_set_sensitive (gconfig->bitrate.update_statfile, TRUE);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gconfig->bitrate.update_statfile), TRUE);
      break;
    }
}

static void
_bitrate_statfile (GtkToggleButton *button,
                   gpointer         user_data)
{
  X264_Gui_Config *gconfig;

  gconfig = (X264_Gui_Config *)user_data;

  if (gtk_toggle_button_get_active (button))
    gtk_widget_set_sensitive (gconfig->bitrate.statsfile_name, TRUE);
  else
    gtk_widget_set_sensitive (gconfig->bitrate.statsfile_name, FALSE);
}

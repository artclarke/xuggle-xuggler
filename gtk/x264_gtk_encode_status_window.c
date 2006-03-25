#if defined __FreeBSD__ || defined __OpenBSD__ || defined __NetBSD__ || defined __DragonFly__
#  include <inttypes.h>
#else
#  include <stdint.h>
#endif

#include <gtk/gtk.h>

#include "../x264.h"
#include "x264_gtk_encode_private.h"


/* Callbacks */
static gboolean _delete_window_cb    (GtkWidget *widget,
                                      GdkEvent  *event,
                                      gpointer   user_data);
static void     _response_window_cb  (GtkDialog *dialog,
                                      gint       res,
                                      gpointer   user_data);

GtkWidget *
x264_gtk_encode_status_window (X264_Thread_Data *thread_data)
{
  GtkWidget *win_status;
  GtkWidget *table;
  GtkWidget *label;

  if (!thread_data) return NULL;

  win_status = thread_data->dialog = gtk_dialog_new ();
  gtk_window_set_title  (GTK_WINDOW (win_status), "Encoding status");
  thread_data->button = gtk_dialog_add_button (GTK_DIALOG (win_status),
                                               GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
  thread_data->end_button = gtk_dialog_add_button (GTK_DIALOG (thread_data->dialog),
                                                   GTK_STOCK_CLOSE, GTK_RESPONSE_CANCEL);
  gtk_widget_set_sensitive (thread_data->end_button, FALSE);

  g_signal_connect (G_OBJECT (win_status),
                    "delete-event",
                    G_CALLBACK (_delete_window_cb),
                    thread_data);
  g_signal_connect (G_OBJECT (win_status),
                    "response",
                    G_CALLBACK (_response_window_cb),
                    thread_data);

  table = gtk_table_new (5, 2, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_container_set_border_width (GTK_CONTAINER (table), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (win_status)->vbox), table,
                      FALSE, FALSE, 0);
  gtk_widget_show (table);
  
  label = gtk_label_new ("Current video frame:");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 0, 1);
  gtk_widget_show (label);
  
  thread_data->current_video_frame = gtk_entry_new ();
  gtk_table_attach_defaults (GTK_TABLE (table),
                             thread_data->current_video_frame,
                             1, 2, 0, 1);
  gtk_widget_show (thread_data->current_video_frame);
  
  label = gtk_label_new ("Video data:");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 1, 2);
  gtk_widget_show (label);
  
  thread_data->video_data = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (thread_data->video_data), "0KB");
  gtk_table_attach_defaults (GTK_TABLE (table), thread_data->video_data,
                             1, 2, 1, 2);
  gtk_widget_show (thread_data->video_data);
  
  label = gtk_label_new ("Video rendering rate:");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 2, 3);
  gtk_widget_show (label);
  
  thread_data->video_rendering_rate = gtk_entry_new ();
  gtk_table_attach_defaults (GTK_TABLE (table),
                             thread_data->video_rendering_rate,
                             1, 2, 2, 3);
  gtk_widget_show (thread_data->video_rendering_rate);
  
  label = gtk_label_new ("Time elapsed:");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 3, 4);
  gtk_widget_show (label);
  
  thread_data->time_elapsed = gtk_entry_new ();
  gtk_table_attach_defaults (GTK_TABLE (table), thread_data->time_elapsed,
                             1, 2, 3, 4);
  gtk_widget_show (thread_data->time_elapsed);
  
  label = gtk_label_new ("Total time (estimated):");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 4, 5);
  gtk_widget_show (label);
  
  thread_data->time_remaining = gtk_entry_new ();
  gtk_table_attach_defaults (GTK_TABLE (table), thread_data->time_remaining,
                             1, 2, 4, 5);
  gtk_widget_show (thread_data->time_remaining);

  table = gtk_table_new (1, 2, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_container_set_border_width (GTK_CONTAINER (table), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (win_status)->vbox), table,
                      FALSE, FALSE, 0);
  gtk_widget_show (table);
  
  label = gtk_label_new ("Progress:");
  gtk_misc_set_alignment (GTK_MISC (label),
                          0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table),
                             label,
                             0, 1,
                             0, 1);
  gtk_widget_show (label);
  
  thread_data->progress = gtk_progress_bar_new ();
  gtk_table_attach_defaults (GTK_TABLE (table), thread_data->progress,
                             1, 2, 0, 1);
  gtk_widget_show (thread_data->progress);

  return win_status;
}

static void
_thread_data_free (X264_Thread_Data *thread_data)
{
  g_free (thread_data->param);
  g_free (thread_data->file_input);
  g_free (thread_data->file_output);
  g_io_channel_unref (thread_data->io_read);
  g_io_channel_unref (thread_data->io_write);
  g_free (thread_data);
}

static gboolean
_delete_window_cb (GtkWidget *widget,
                   GdkEvent  *event __UNUSED__,
                   gpointer   user_data)
{
  gtk_widget_destroy (widget);
  _thread_data_free ((X264_Thread_Data *)user_data);
  return TRUE;
}

static void
_response_window_cb (GtkDialog *dialog,
                     gint       res,
                     gpointer   user_data)
{
  switch (res) {
  case GTK_RESPONSE_CANCEL:
  default:
    gtk_widget_destroy (GTK_WIDGET (dialog));
    _thread_data_free ((X264_Thread_Data *)user_data);
  }
}

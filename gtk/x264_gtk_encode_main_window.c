#if defined __FreeBSD__ || defined __OpenBSD__ || defined __NetBSD__ || defined __DragonFly__
#  include <inttypes.h>
#else
#  include <stdint.h>
#endif
#include <unistd.h>
#ifdef _WIN32 /* Needed to define _O_BINARY */
#  include <fcntl.h>
#  define strncasecmp _strnicmp
#endif
#include <errno.h>
#include <string.h>
#include <sys/stat.h>  /* For stat */

#include <gtk/gtk.h>

#include "../x264.h"
#include "../config.h"
#include "x264_icon.h"
#include "x264_gtk.h"
#include "x264_gtk_demuxers.h"
#include "x264_gtk_encode_private.h"
#include "x264_gtk_encode_encode.h"
#include "x264_gtk_encode_status_window.h"


typedef struct X264_Gtk_Encode_ X264_Gtk_Encode;

struct X264_Gtk_Encode_
{
  GtkWidget         *main_dialog;

  /* input */
  X264_Demuxer_Type  container;
  guint64            size; /* For YUV formats */
  GtkWidget         *file_input;
  GtkWidget         *width;
  GtkWidget         *height;
  GtkWidget         *fps_num;
  GtkWidget         *fps_den;
  GtkWidget         *frame_count;
  
  /* output */
  GtkWidget         *path_output;
  GtkWidget         *file_output;
  GtkWidget         *combo;
};


/* Callbacks */
static gboolean _delete_window_cb    (GtkWidget *widget,
                                      GdkEvent  *event,
                                      gpointer   user_data);
static void     _configure_window_cb (GtkButton *button,
                                      gpointer   user_data);
static void     _chooser_window_cb   (GtkDialog *dialog,
                                      gint       res,
                                      gpointer   user_data);
static void     _response_window_cb  (GtkDialog *dialog,
                                      gint       res,
                                      gpointer   user_data);
static void     _dimension_entry_cb  (GtkEditable *editable,
                                      gpointer     user_data);

static gboolean _fill_status_window (GIOChannel  *io,
                                     GIOCondition condition,
                                     gpointer     user_data);
/* Code */
guint64
_file_size(const char* name)
{
  struct stat buf;
  memset(&buf, 0, sizeof(struct stat));

  if (stat(name, &buf) < 0)
  {
    fprintf(stderr, "Can't stat file\n");
    return 0;
  }
  return buf.st_size;
}

void
x264_gtk_encode_main_window ()
{
  GtkWidget       *dialog;
  GtkWidget       *frame;
  GtkWidget       *button;
  GtkWidget       *table;
  GtkWidget       *label;
  GtkFileChooser  *chooser;
  GtkFileFilter   *filter;
  GdkPixbuf       *icon;
  X264_Gtk_Encode *encode;

  encode = (X264_Gtk_Encode *)g_malloc0 (sizeof (X264_Gtk_Encode));

  dialog = gtk_dialog_new_with_buttons ("X264 Gtk Encoder",
                                        NULL, 0,
                                        NULL);
  icon = gdk_pixbuf_new_from_inline (-1, x264_icon,
                                        FALSE, NULL);
  gtk_window_set_icon (GTK_WINDOW (dialog), icon);
  g_signal_connect (G_OBJECT (dialog),
                    "delete-event",
                    G_CALLBACK (_delete_window_cb),
                    encode);
  g_signal_connect (G_OBJECT (dialog),
                    "response",
                    G_CALLBACK (_response_window_cb),
                    encode);
  encode->main_dialog = dialog;

  button = gtk_button_new_with_label ("Configure");
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), button, FALSE, TRUE, 6);
  g_signal_connect (G_OBJECT (button),
                    "clicked",
                    G_CALLBACK (_configure_window_cb),
                    dialog);
  gtk_widget_show (button);

  gtk_dialog_add_buttons (GTK_DIALOG (dialog),
                          GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
                          GTK_STOCK_EXECUTE, GTK_RESPONSE_APPLY,
                          NULL);

  /* input */
  frame = gtk_frame_new ("Input file");
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), frame, FALSE, TRUE, 6);
  gtk_widget_show (frame);

  table = gtk_table_new (6, 2, TRUE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_container_set_border_width (GTK_CONTAINER (table), 6);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  label = gtk_label_new ("Input file:");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 0, 1);
  gtk_widget_show (label);

  chooser = (GtkFileChooser*)
      gtk_file_chooser_dialog_new("Select a file",
                                  GTK_WINDOW(dialog),
                                  GTK_FILE_CHOOSER_ACTION_OPEN,
                                  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                  GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                  NULL);
  gtk_file_chooser_set_current_folder (chooser, g_get_home_dir ());
   /* All supported */
  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, "All supported");
  gtk_file_filter_add_pattern (filter, "*.yuv");
  gtk_file_filter_add_pattern (filter, "*.cif");
  gtk_file_filter_add_pattern (filter, "*.qcif");
  gtk_file_filter_add_pattern (filter, "*.y4m");
#ifdef AVIS_INPUT
  gtk_file_filter_add_pattern (filter, "*.avs");
  gtk_file_filter_add_pattern (filter, "*.avi");
#endif
  gtk_file_chooser_add_filter (chooser, filter);
  /* YUV filter */
  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, "YUV sequence");
  gtk_file_filter_add_pattern (filter, "*.yuv");
  gtk_file_chooser_add_filter (chooser, filter);

  /* CIF filter */
  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, "YUV CIF sequence");
  gtk_file_filter_add_pattern (filter, "*.cif");
  gtk_file_chooser_add_filter (chooser, filter);

  /* QCIF filter */
  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, "YUV QCIF sequence");
  gtk_file_filter_add_pattern (filter, "*.qcif");
  gtk_file_chooser_add_filter (chooser, filter);

  /* YUV4MPEG2 filter */
  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, "YUV4MPEG2 sequence");
  gtk_file_filter_add_pattern (filter, "*.y4m");
  gtk_file_chooser_add_filter (chooser, filter);

#ifdef AVIS_INPUT
  /* AVI filter */
  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, "AVI");
  gtk_file_filter_add_pattern (filter, "*.avi");
  gtk_file_chooser_add_filter (chooser, filter);
  /* AVS filter */
  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, "Avisynth Script");
  gtk_file_filter_add_pattern (filter, "*.avs");
  gtk_file_chooser_add_filter (chooser, filter);
#endif
  g_signal_connect_after(G_OBJECT (chooser), "response",
                         G_CALLBACK (_chooser_window_cb),
                         encode);
  encode->file_input = gtk_file_chooser_button_new_with_dialog(GTK_WIDGET(chooser));
  gtk_table_attach_defaults (GTK_TABLE (table), encode->file_input, 1, 2, 0, 1);
  gtk_widget_show (encode->file_input);

  label = gtk_label_new ("Width:");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 1, 2);
  gtk_widget_show (label);

  encode->width = gtk_entry_new_with_max_length (255);
  gtk_entry_set_text (GTK_ENTRY (encode->width), "352");
  g_signal_connect_after(G_OBJECT (encode->width), "changed",
                   G_CALLBACK (_dimension_entry_cb),
                   encode);
  gtk_table_attach_defaults (GTK_TABLE (table), encode->width, 1, 2, 1, 2);
  gtk_widget_show (encode->width);

  label = gtk_label_new ("Height:");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 2, 3);
  gtk_widget_show (label);

  encode->height = gtk_entry_new_with_max_length (255);
  gtk_entry_set_text (GTK_ENTRY (encode->height), "288");
  gtk_table_attach_defaults (GTK_TABLE (table), encode->height, 1, 2, 2, 3);
  g_signal_connect_after(G_OBJECT (encode->height), "changed",
                   G_CALLBACK (_dimension_entry_cb),
                   encode);
  gtk_widget_show (encode->height);

  label = gtk_label_new ("Frame rate num:");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 3, 4);
  gtk_widget_show (label);

  encode->fps_num = gtk_entry_new_with_max_length (255);
  gtk_entry_set_text (GTK_ENTRY (encode->fps_num), "25");
  gtk_table_attach_defaults (GTK_TABLE (table), encode->fps_num, 1, 2, 3, 4);
  gtk_widget_show (encode->fps_num);

  label = gtk_label_new ("Frame rate den:");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 4, 5);
  gtk_widget_show (label);

  encode->fps_den = gtk_entry_new_with_max_length (255);
  gtk_entry_set_text (GTK_ENTRY (encode->fps_den), "1");
  gtk_table_attach_defaults (GTK_TABLE (table), encode->fps_den, 1, 2, 4, 5);
  gtk_widget_show (encode->fps_den);

  label = gtk_label_new ("Frame count:");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 5, 6);
  gtk_widget_show (label);

  encode->frame_count = gtk_entry_new_with_max_length (255);
  gtk_entry_set_text (GTK_ENTRY (encode->frame_count), "0");
  gtk_table_attach_defaults (GTK_TABLE (table), encode->frame_count, 1, 2, 5, 6);
  gtk_widget_show (encode->frame_count);

  /* output */
  frame = gtk_frame_new ("Output file");
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), frame, FALSE, TRUE, 6);
  gtk_widget_show (frame);

  table = gtk_table_new (3, 2, TRUE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_container_set_border_width (GTK_CONTAINER (table), 6);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  label = gtk_label_new ("Output path:");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 0, 1);
  gtk_widget_show (label);

  encode->path_output = gtk_file_chooser_button_new ("Select a path",
                                                     GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
  gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (encode->path_output),
                                       g_get_home_dir ());
  gtk_table_attach_defaults (GTK_TABLE (table), encode->path_output, 1, 2, 0, 1);
  gtk_widget_show (encode->path_output);

  label = gtk_label_new ("Output file (without ext.):");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 1, 2);
  gtk_widget_show (label);

  encode->file_output = gtk_entry_new_with_max_length (4095);
  gtk_table_attach_defaults (GTK_TABLE (table), encode->file_output, 1, 2, 1, 2);
  gtk_widget_show (encode->file_output);

  label = gtk_label_new ("Container:");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 2, 3);
  gtk_widget_show (label);

  encode->combo = gtk_combo_box_new_text ();
  gtk_combo_box_append_text (GTK_COMBO_BOX (encode->combo),
                             "Raw ES");
  gtk_combo_box_append_text (GTK_COMBO_BOX (encode->combo),
                             "Matroska");
#ifdef MP4_OUTPUT
  gtk_combo_box_append_text (GTK_COMBO_BOX (encode->combo),
                             "Mp4");
#endif
  gtk_table_attach_defaults (GTK_TABLE (table), encode->combo, 1, 2, 2, 3);
  gtk_widget_show (encode->combo);

  gtk_combo_box_set_active (GTK_COMBO_BOX (encode->combo), 0);

  gtk_widget_show (dialog);
}

/* Callbacks */

static void
_encode_shutdown (X264_Gtk_Encode *encode)
{
  if (!encode) return;

  g_free (encode);
  encode = NULL;
}

static gboolean
_delete_window_cb (GtkWidget *widget __UNUSED__,
                   GdkEvent  *event __UNUSED__,
                   gpointer   user_data)
{
  gtk_main_quit ();
  _encode_shutdown ((X264_Gtk_Encode *)user_data);

  return TRUE;
}

static void
_chooser_window_cb (GtkDialog *dialog,
                    gint       res,
                    gpointer   user_data)
{
  X264_Gtk_Encode *encode;
  gboolean         sensitivity = TRUE;
  x264_param_t     param;
  hnd_t            hin;
  char            *in;
#define       BUFFER_LENGTH  64
  gchar            buffer[BUFFER_LENGTH];

  /* input interface */
  int              (*p_open_infile)( char *psz_filename, hnd_t *p_handle, x264_param_t *p_param );
  int              (*p_get_frame_total)( hnd_t handle );
  int              (*p_close_infile)( hnd_t handle );

  encode = (X264_Gtk_Encode *)user_data;
  in = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (encode->file_input));
  if (!in) return;

  /* Set defaults */
  p_open_infile = open_file_yuv;
  p_get_frame_total = get_frame_total_yuv;
  p_close_infile = close_file_yuv;
  param.i_width = (gint)(g_ascii_strtod (gtk_entry_get_text (GTK_ENTRY (encode->width)), NULL));
  param.i_height = (gint)(g_ascii_strtod (gtk_entry_get_text (GTK_ENTRY (encode->height)), NULL));
  param.i_fps_num = 25;
  param.i_fps_den = 1;
  param.i_frame_total = 0;

  switch (res) {
  case GTK_RESPONSE_OK:
  case GTK_RESPONSE_ACCEPT:
  case GTK_RESPONSE_APPLY: {
    X264_Gtk_Encode  *encode = (X264_Gtk_Encode *)user_data;
    GSList           *filters;
    GtkFileFilter    *selected;
    int               container;
    
    filters = gtk_file_chooser_list_filters(GTK_FILE_CHOOSER (encode->file_input));
    selected = gtk_file_chooser_get_filter(GTK_FILE_CHOOSER (encode->file_input));
    container = g_slist_index(filters, selected);
    g_slist_free (filters);

    if (container == 0)
    {
      /* All format needed, search for extension */
      const char *ext = strrchr(in, '.');
      if (!strncasecmp(ext, ".y4m", 4))
        encode->container = X264_DEMUXER_Y4M;
      else if (!strncasecmp(ext, ".avs", 4))
        encode->container = X264_DEMUXER_AVS;
      else if (!strncasecmp(ext, ".avi", 4))
        encode->container = X264_DEMUXER_AVI;
      else if (!strncasecmp(ext, ".cif", 4))
        encode->container = X264_DEMUXER_CIF;
      else if (!strncasecmp(ext, ".qcif", 4))
        encode->container = X264_DEMUXER_QCIF;
      else
        encode->container = X264_DEMUXER_YUV;
    }
    else
    {
      /* The all supproted type is 0 => shift of 1 */
      encode->container = (X264_Demuxer_Type)container-1;
    }
        
    switch (encode->container) {
    case X264_DEMUXER_YUV: /* YUV */
      break;
    case X264_DEMUXER_CIF: /* YUV CIF */
      param.i_width = 352;
      param.i_height = 288;
      break;
    case X264_DEMUXER_QCIF: /* YUV QCIF */
      /*   Default input file driver */
      param.i_width = 176;
      param.i_height = 144;
      break;
    case X264_DEMUXER_Y4M: /* YUV4MPEG */
      /*   Default input file driver */
      sensitivity = FALSE;
      p_open_infile = open_file_y4m;
      p_get_frame_total = get_frame_total_y4m;
      p_close_infile = close_file_y4m;
      break;
#ifdef AVIS_INPUT
    case X264_DEMUXER_AVI: /* AVI */
    case X264_DEMUXER_AVS: /* AVS */
      sensitivity = FALSE;
      p_open_infile = open_file_avis;
      p_get_frame_total = get_frame_total_avis;
      p_close_infile = close_file_avis;
    break;
#endif
    default: /* Unknown */
      break;
    }
    break;
  }
  default:
    break;
  }

  /* Modify dialog */
  gtk_widget_set_sensitive(encode->width, sensitivity);
  gtk_widget_set_sensitive(encode->height, sensitivity);
  gtk_widget_set_sensitive(encode->fps_num, sensitivity);
  gtk_widget_set_sensitive(encode->fps_den, sensitivity);
  gtk_widget_set_sensitive(encode->frame_count, sensitivity);

  /* Inquire input format */
  if (param.i_width < 2) param.i_width = 352;
  if (param.i_height < 2) param.i_height = 288;
  if (p_open_infile (in, &hin, &param) >= 0) {
    param.i_frame_total = p_get_frame_total(hin);
    p_close_infile (hin);
  } else {
    GtkWidget *dialog_message;

    dialog_message = gtk_message_dialog_new (GTK_WINDOW (dialog),
                                             GTK_DIALOG_DESTROY_WITH_PARENT,
                                             GTK_MESSAGE_ERROR,
                                             GTK_BUTTONS_CLOSE,
                                             strerror(errno));
    gtk_dialog_run (GTK_DIALOG (dialog_message));
    gtk_widget_destroy (dialog_message);      
  }
  encode->size = _file_size(in);

  if (g_snprintf(buffer, BUFFER_LENGTH, "%i", param.i_width) > 0)
    gtk_entry_set_text (GTK_ENTRY (encode->width), buffer);
  if (g_snprintf(buffer, BUFFER_LENGTH, "%i", param.i_height) > 0)
    gtk_entry_set_text (GTK_ENTRY (encode->height), buffer);
  if (g_snprintf(buffer, BUFFER_LENGTH, "%i", param.i_fps_num) > 0)
    gtk_entry_set_text (GTK_ENTRY (encode->fps_num), buffer);
  if (g_snprintf(buffer, BUFFER_LENGTH, "%i", param.i_fps_den) > 0)
    gtk_entry_set_text (GTK_ENTRY (encode->fps_den), buffer);

  if (g_snprintf(buffer, BUFFER_LENGTH, "%i", param.i_frame_total) > 0)
    gtk_entry_set_text (GTK_ENTRY (encode->frame_count), buffer);
}
static void
_dimension_entry_cb (GtkEditable *editable,
                     gpointer     user_data)
{
  X264_Gtk_Encode *encode = (X264_Gtk_Encode *)user_data;
  char             buffer[32];
  gint             width;
  gint             height;
  gint             frame_size;

  width = (gint)(g_ascii_strtod (gtk_entry_get_text (GTK_ENTRY (encode->width)), NULL));
  height = (gint)(g_ascii_strtod (gtk_entry_get_text (GTK_ENTRY (encode->height)), NULL));
  frame_size = (3*width*height)/2;

  if (frame_size > 0 && encode->container <= X264_DEMUXER_QCIF)
  {
    snprintf(buffer, 32, "%lu", (long unsigned int)((encode->size+frame_size/2)/frame_size));
    gtk_entry_set_text (GTK_ENTRY (encode->frame_count), buffer);
  }
}

static void
_configure_window_cb (GtkButton *button __UNUSED__,
                      gpointer   user_data)
{
  GtkWidget *window;
  
  window = x264_gtk_window_create (GTK_WIDGET (user_data));
  x264_gtk_shutdown (window);
}

static void
_response_window_cb (GtkDialog *dialog,
                     gint       res,
                     gpointer   user_data)
{
  switch (res) {
  case GTK_RESPONSE_APPLY: {
    x264_param_t     *param;
    X264_Gtk         *x264_gtk;
    X264_Gtk_Encode  *encode;
    X264_Thread_Data *thread_data;
    GtkWidget        *win_status;
    GThread          *thread;
    const gchar      *file_input = NULL;
    const gchar      *path_output = NULL;
    const gchar      *filename_output = NULL;
    gchar            *file_output = NULL;
    gchar            *ext;
    gint              fds[2];
    gint              out_container;

    encode = (X264_Gtk_Encode *)user_data;
    file_input = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (encode->file_input));
    path_output = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (encode->path_output));
    filename_output = gtk_entry_get_text (GTK_ENTRY (encode->file_output));

    if (!file_input || 
        (file_input[0] == '\0')) {
      GtkWidget *dialog_message;

      dialog_message = gtk_message_dialog_new (GTK_WINDOW (dialog),
                                               GTK_DIALOG_DESTROY_WITH_PARENT,
                                               GTK_MESSAGE_ERROR,
                                               GTK_BUTTONS_CLOSE,
                                               "Error: input file name is not set");
      gtk_dialog_run (GTK_DIALOG (dialog_message));
      gtk_widget_destroy (dialog_message);
      break;
    }

    if (!filename_output || 
        (filename_output[0] == '\0')) {
      GtkWidget *dialog_message;

      dialog_message = gtk_message_dialog_new (GTK_WINDOW (dialog),
                                       GTK_DIALOG_DESTROY_WITH_PARENT,
                                       GTK_MESSAGE_ERROR,
                                       GTK_BUTTONS_CLOSE,
                                               "Error: output file name is not set");
      gtk_dialog_run (GTK_DIALOG (dialog_message));
      gtk_widget_destroy (dialog_message);
      break;
    }

    out_container = gtk_combo_box_get_active (GTK_COMBO_BOX (encode->combo));

    switch (out_container) {
    case 1:
      ext = ".mkv";
      break;
#ifdef MP4_OUTPUT
    case 2:
      ext = ".mp4";
      break;
#endif
    case 0:
    default:
      ext = ".264";
    }

    file_output = g_strconcat (path_output, "/", filename_output, ext, NULL);
    g_print ("file output : %s\n", file_output);

    x264_gtk = x264_gtk_load ();
    param = x264_gtk_param_get (x264_gtk);
    g_free (x264_gtk);

    {
      gint width;
      gint height;
      gint fps_num;
      gint fps_den;
      gint frame_count;

      width = (gint)(g_ascii_strtod (gtk_entry_get_text (GTK_ENTRY (encode->width)), NULL));
      height = (gint)(g_ascii_strtod (gtk_entry_get_text (GTK_ENTRY (encode->height)), NULL));
      fps_num = (gint)(g_ascii_strtod (gtk_entry_get_text (GTK_ENTRY (encode->fps_num)), NULL));
      fps_den = (gint)(g_ascii_strtod (gtk_entry_get_text (GTK_ENTRY (encode->fps_den)), NULL));
      frame_count = (gint)(g_ascii_strtod (gtk_entry_get_text (GTK_ENTRY (encode->frame_count)), NULL));

      if ((width <= 0) ||
          (height <= 0) ||
          (fps_num <= 0) ||
          (fps_den <= 0) ||
          (frame_count < 0))
        break;

      param->i_width = width;
      param->i_height = height;
      param->i_fps_num = fps_num;
      param->i_fps_den = fps_den;
      param->i_frame_total = frame_count;
    }

    if (pipe (fds) == -1)
      break;

    thread_data = (X264_Thread_Data *)g_malloc0 (sizeof (X264_Thread_Data));
    thread_data->param = param;
    thread_data->file_input = g_strdup (file_input);
    thread_data->file_output = g_strdup (file_output);
    thread_data->in_container = encode->container;
    thread_data->out_container = out_container;
    g_free (file_output);

    thread_data->io_read = g_io_channel_unix_new (fds[0]);
    g_io_channel_set_encoding (thread_data->io_read, NULL, NULL);
    thread_data->io_write = g_io_channel_unix_new (fds[1]);
    g_io_channel_set_encoding (thread_data->io_write, NULL, NULL);

    g_io_add_watch (thread_data->io_read, G_IO_IN,
                    (GIOFunc)_fill_status_window, thread_data);

    win_status = x264_gtk_encode_status_window (thread_data);
    gtk_window_set_transient_for (GTK_WINDOW (win_status), GTK_WINDOW (dialog));
    gtk_window_set_modal (GTK_WINDOW (win_status), TRUE);
    gtk_widget_show (win_status);
    //gtk_widget_hide(thread_data->end_button);

    thread = g_thread_create ((GThreadFunc)x264_gtk_encode_encode, thread_data, FALSE, NULL);

    break;
  }
  case GTK_RESPONSE_CLOSE:
  default:
    gtk_main_quit ();
    _encode_shutdown ((X264_Gtk_Encode *)user_data);
  }
}

static gboolean
_fill_status_window (GIOChannel  *io __UNUSED__,
                     GIOCondition condition __UNUSED__,
                     gpointer     user_data)
{
  gchar             str[128];
  X264_Thread_Data *thread_data;
  X264_Pipe_Data    pipe_data;
  GIOStatus         status;
  gsize             size;
  gint              eta;
  gdouble           progress;
  gdouble           fps;

  thread_data = (X264_Thread_Data *)user_data;
  status = g_io_channel_read_chars (thread_data->io_read,
                                    (gchar *)&pipe_data,
                                    sizeof (X264_Pipe_Data),
                                    &size, NULL);
  if (status != G_IO_STATUS_NORMAL) {
    g_print ("Error ! %d %d %d\n", status, sizeof (X264_Pipe_Data), size);
    return FALSE;
  }

  snprintf (str, 128, "%d/%d", pipe_data.frame, pipe_data.frame_total);
  gtk_entry_set_text (GTK_ENTRY (thread_data->current_video_frame),
                      str);

  snprintf (str, 128, "%dKB",
            pipe_data.file >> 10);
  gtk_entry_set_text (GTK_ENTRY (thread_data->video_data),
                      str);

  fps = pipe_data.elapsed > 0 ? 1000000.0 * (gdouble)pipe_data.frame / (gdouble)pipe_data.elapsed : 0.0;
  snprintf (str, 128, "%.2fKB/s (%.2f fps)",
            (double) pipe_data.file * 8 * thread_data->param->i_fps_num /
            ((double) thread_data->param->i_fps_den * pipe_data.frame * 1000),
            fps);
  gtk_entry_set_text (GTK_ENTRY (thread_data->video_rendering_rate),
                      str);

  snprintf (str, 128, "%lld:%02lld:%02lld",
            (pipe_data.elapsed / 1000000) / 3600,
            ((pipe_data.elapsed / 1000000) / 60) % 60,
            (pipe_data.elapsed / 1000000) % 60);
  gtk_entry_set_text (GTK_ENTRY (thread_data->time_elapsed),
                      str);

  eta = pipe_data.elapsed * (pipe_data.frame_total - pipe_data.frame) / ((int64_t)pipe_data.frame * 1000000);
  snprintf (str, 128, "%d:%02d:%02d", eta / 3600, (eta / 60) % 60, eta % 60);
  gtk_entry_set_text (GTK_ENTRY (thread_data->time_remaining),
                      str);

  progress = (gdouble)pipe_data.frame / (gdouble)pipe_data.frame_total;
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (thread_data->progress),
                                 progress);

  snprintf (str, 128, "%0.1f%%", 100.0 * progress);
  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (thread_data->progress), str);

  return TRUE;
}

#include <string.h>
#if defined __FreeBSD__ || defined __OpenBSD__ || defined __NetBSD__ || defined __DragonFly__
#  include <inttypes.h>
#else
#  include <stdint.h>
#endif

#include <gtk/gtk.h>

#include "../x264.h"
#include "x264_gtk_demuxers.h"
#include "x264_gtk_encode_private.h"


#define DATA_MAX 3000000
uint8_t data[DATA_MAX];

int64_t x264_mdate (void);

/* input interface */
int           (*p_open_infile)( char *psz_filename, hnd_t *p_handle, x264_param_t *p_param );
int           (*p_get_frame_total)( hnd_t handle );
int           (*p_read_frame)( x264_picture_t *p_pic, hnd_t handle, int i_frame );
int           (*p_close_infile)( hnd_t handle );

/* output interface */
static int (*p_open_outfile)      (char *filename, void **handle);
static int (*p_set_outfile_param) (void *handle, x264_param_t *p_param);
static int (*p_write_nalu)        (void *handle, uint8_t *p_nal, int i_size);
static int (*p_set_eop)           (void *handle, x264_picture_t *p_picture);
static int (*p_close_outfile)     (void *handle);

static int _set_drivers  (X264_Demuxer_Type in_container, gint out_container);
static int _encode_frame (x264_t *h, void *handle, x264_picture_t *pic);


gpointer
x264_gtk_encode_encode (X264_Thread_Data *thread_data)
{
  GIOStatus       status;
  gsize           size;
  X264_Pipe_Data  pipe_data;
  x264_param_t   *param;
  x264_picture_t  pic;
  x264_t         *h;
  hnd_t           hin;
  hnd_t           hout;
  int             i_frame;
  int             i_frame_total;
  int64_t         i_start;
  int64_t         i_end;
  int64_t         i_file;
  int             i_frame_size;
  int             i_progress;
  int             err;

  g_print ("encoding...\n");
  param = thread_data->param;
  err = _set_drivers (thread_data->in_container, thread_data->out_container);
  if (err < 0) {
    GtkWidget *no_driver;
    no_driver = gtk_message_dialog_new (GTK_WINDOW(thread_data->dialog),
                                        GTK_DIALOG_DESTROY_WITH_PARENT,
                                        GTK_MESSAGE_ERROR,
                                        GTK_BUTTONS_CLOSE,
                                        (err == -2) ? "Error: unknown output file type"
                                                    : "Error: unknown input file type");
    gtk_dialog_run (GTK_DIALOG (no_driver));
    gtk_widget_destroy (no_driver);
    return NULL;
  }
    
  if (p_open_infile (thread_data->file_input, &hin, param)) {
    fprintf( stderr, "could not open input file '%s'\n", thread_data->file_input );
    return NULL;
  }

  p_open_outfile ((char *)thread_data->file_output, &hout);

  i_frame_total = p_get_frame_total (hin );
  if (((i_frame_total == 0) || (param->i_frame_total < i_frame_total)) &&
      (param->i_frame_total > 0))
    i_frame_total = param->i_frame_total;
  param->i_frame_total = i_frame_total;

  if ((h = x264_encoder_open (param)) == NULL)
    {
      fprintf (stderr, "x264_encoder_open failed\n");
      p_close_infile (hin);
      p_close_outfile (hout);
      g_free (param);

      return NULL;
    }

  if (p_set_outfile_param (hout, param))
    {
      fprintf (stderr, "can't set outfile param\n");
      p_close_infile (hin);
      p_close_outfile (hout);
      g_free (param);

      return NULL;
    }

  /* Create a new pic */
  x264_picture_alloc (&pic, X264_CSP_I420, param->i_width, param->i_height );

  i_start = x264_mdate();

  /* Encode frames */
  for (i_frame = 0, i_file = 0, i_progress = 0;
       ((i_frame < i_frame_total) || (i_frame_total == 0)); )
    {
      if (p_read_frame (&pic, hin, i_frame))
        break;

      pic.i_pts = (int64_t)i_frame * param->i_fps_den;

      i_file += _encode_frame (h, hout, &pic);

      i_frame++;

      /* update status line (up to 1000 times per input file) */
      if (param->i_log_level < X264_LOG_DEBUG && 
          (i_frame_total ? i_frame * 1000 / i_frame_total > i_progress
           : i_frame % 10 == 0))
        {
          int64_t i_elapsed = x264_mdate () - i_start;
          double fps = i_elapsed > 0 ? i_frame * 1000000. / i_elapsed : 0;

          if (i_frame_total)
            {
              pipe_data.frame = i_frame;
              pipe_data.frame_total = i_frame_total;
              pipe_data.file = i_file;
              pipe_data.elapsed = i_elapsed;
              status = g_io_channel_write_chars (thread_data->io_write,
                                                 (const gchar *)&pipe_data,
                                                 sizeof (X264_Pipe_Data),
                                                 &size, NULL);
              if (status != G_IO_STATUS_NORMAL) {
                g_print ("Error ! %d %d %d\n", status, sizeof (X264_Pipe_Data), size);
              }
              else {
                /* we force the GIOChannel to write to the pipeline */
                status = g_io_channel_flush (thread_data->io_write,
                                             NULL);
                if (status != G_IO_STATUS_NORMAL) {
                  g_print ("Error ! %d\n", status);
                }
              }
            }
          else
            fprintf( stderr, "encoded frames: %d, %.2f fps   \r", i_frame, fps );
          fflush( stderr ); /* needed in windows */
        }
    }
  /* Flush delayed B-frames */
  do {
    i_file += i_frame_size = _encode_frame (h, hout, NULL);
  } while (i_frame_size);

  i_end = x264_mdate ();
  x264_picture_clean (&pic);
  x264_encoder_close (h);
  fprintf (stderr, "\n");

  p_close_infile (hin);
  p_close_outfile (hout);

  if (i_frame > 0) {
    double fps = (double)i_frame * (double)1000000 /
      (double)(i_end - i_start);

    fprintf (stderr, "encoded %d frames, %.2f fps, %.2f kb/s\n",
             i_frame, fps,
             (double) i_file * 8 * param->i_fps_num /
             ((double) param->i_fps_den * i_frame * 1000));
  }

  gtk_widget_set_sensitive (thread_data->end_button, TRUE);
  gtk_widget_hide (thread_data->button);
  return NULL;
}

static int
_set_drivers (X264_Demuxer_Type in_container, gint out_container)
{
  switch (in_container) {
  case X264_DEMUXER_YUV:
  case X264_DEMUXER_CIF:
  case X264_DEMUXER_QCIF:
    /*   Default input file driver */
    p_open_infile = open_file_yuv;
    p_get_frame_total = get_frame_total_yuv;
    p_read_frame = read_frame_yuv;
    p_close_infile = close_file_yuv;
    break;
  case X264_DEMUXER_Y4M:
    p_open_infile = open_file_y4m;
    p_get_frame_total = get_frame_total_y4m;
    p_read_frame = read_frame_y4m;
    p_close_infile = close_file_y4m;
    break;
#ifdef AVIS_INPUT
  case X264_DEMUXER_AVI:
  case X264_DEMUXER_AVS:
    p_open_infile = open_file_avis;
    p_get_frame_total = get_frame_total_avis;
    p_read_frame = read_frame_avis;
    p_close_infile = close_file_avis;
    break;
#endif
  default: /* Unknown */
    return -1;
  }

  switch (out_container) {
  case 0:
/*     Raw ES output file driver */
    p_open_outfile = open_file_bsf;
    p_set_outfile_param = set_param_bsf;
    p_write_nalu = write_nalu_bsf;
    p_set_eop = set_eop_bsf;
    p_close_outfile = close_file_bsf;
    break;
  case 1:
/*     Matroska output file driver */
    p_open_outfile = open_file_mkv;
    p_set_outfile_param = set_param_mkv;
    p_write_nalu = write_nalu_mkv;
    p_set_eop = set_eop_mkv;
    p_close_outfile = close_file_mkv;
    break;
#ifdef MP4_OUTPUT
  case 2:
    p_open_outfile = open_file_mp4;
    p_set_outfile_param = set_param_mp4;
    p_write_nalu = write_nalu_mp4;
    p_set_eop = set_eop_mp4;
    p_close_outfile = close_file_mp4;
    break;
#endif
  default:
    return -2;
  }

  return 1;
}

static int
_encode_frame (x264_t *h, void *handle, x264_picture_t *pic)
{
  x264_picture_t pic_out;
  x264_nal_t    *nal;
  int            i_nal;
  int            i;
  int            i_file = 0;

  /* Do not force any parameters */
  if (pic)
    {
      pic->i_type = X264_TYPE_AUTO;
      pic->i_qpplus1 = 0;
    }
  if (x264_encoder_encode (h, &nal, &i_nal, pic, &pic_out) < 0)
    {
      fprintf (stderr, "x264_encoder_encode failed\n");
    }

  for (i = 0; i < i_nal; i++)
    {
      int i_size;
      int i_data;

      i_data = DATA_MAX;
      if ((i_size = x264_nal_encode (data, &i_data, 1, &nal[i])) > 0 )
        {
          i_file += p_write_nalu (handle, data, i_size);
        }
      else if (i_size < 0)
        {
          fprintf (stderr, "need to increase buffer size (size=%d)\n", -i_size);
        }
    }
  if (i_nal)
    p_set_eop (handle, &pic_out);

  return i_file;
}

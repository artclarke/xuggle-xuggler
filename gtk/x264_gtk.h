#ifndef __X264_GTK_H__
#define __X264_GTK_H__


#include "x264_gtk_enum.h"

typedef struct X264_Gtk_ X264_Gtk;

struct X264_Gtk_
{
  /* video properties */
  gint width;
  gint height;
  gint csp;

  /* bitrate */
  X264_Pass pass;
  gint average_bitrate;
  gint target_bitrate;
  gint quantizer;
  gint desired_size;
  gint statsfile_length; /* length of the filename (as returned by strlen) */
  gchar statsfile_name[4095+1];

  /* rc */
  gint keyframe_boost;
  gint bframes_reduction;
  gint bitrate_variability;

  gint min_qp;
  gint max_qp;
  gint max_qp_step;

  gint scene_cut_threshold;
  gint min_idr_frame_interval;
  gint max_idr_frame_interval;

  gint    vbv_max_bitrate;
  gint    vbv_buffer_size;
  gdouble vbv_buffer_init;

  /* mb */

  gint bframe;  /* max consecutive B frames */
  gint bframe_bias;
  X264_Direct_Mode direct_mode;

  /* more */
  X264_Partition_Decision partition_decision;
  X264_Me_Method me_method;
  gint range;
  gint max_ref_frames;

  gint sample_ar_x;
  gint sample_ar_y;
  gint threads;
  guint trellis;
  gint noise_reduction;
  
  gint strength;
  gint threshold;

  X264_Debug_Method debug_method;
  gchar fourcc[4+1];

  /* cqm */
  X264_Cqm_Preset cqm_preset;
  gchar           cqm_file[4095+1];
  guint8          cqm_4iy[16];
  guint8          cqm_4ic[16];
  guint8          cqm_4py[16];
  guint8          cqm_4pc[16];
  guint8          cqm_8iy[64];
  guint8          cqm_8py[64];

  /* bitrate */
  guint stat_write            : 1;
  guint stat_read             : 1;
  guint update_statfile       : 1;
  /* mb - partitions */
  guint transform_8x8         : 1;
  guint pframe_search_8       : 1;
  guint bframe_search_8       : 1;
  guint pframe_search_4       : 1;
  guint inter_search_8        : 1;
  guint inter_search_4        : 1;
  /* mb - bframes */
  guint bframe_pyramid        : 1; /* use as reference */
  guint bidir_me              : 1;
  guint bframe_adaptive       : 1;
  guint weighted_bipred       : 1;
  /* more - me */
  guint bframe_rdo            : 1;
  guint chroma_me             : 1;
  guint mixed_refs            : 1;
  guint fast_pskip            : 1;
  guint dct_decimate          : 1;
  /* more - misc */
  guint cabac                 : 1;
  /* more - misc - df */
  guint deblocking_filter     : 1;
};

x264_param_t *x264_gtk_param_get (X264_Gtk *x264_gtk);
X264_Gtk     *x264_gtk_load (void);
GtkWidget    *x264_gtk_window_create (GtkWidget *parent);
void          x264_gtk_shutdown (GtkWidget *dialog);
void          x264_gtk_free (X264_Gtk *x264_gtk);


#endif /* __X264_GTK_H__ */

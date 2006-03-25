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
  gint statfile_length; /* length of the filename (as returned by strlen) */
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

  /* mb */

  gint max_consecutive;
  gint bias;
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

  /* bitrate */
  guint update_statfile       : 1;
  /* mb - partitions */
  guint transform_8x8         : 1;
  guint pframe_search_8       : 1;
  guint bframe_search_8       : 1;
  guint pframe_search_4       : 1;
  guint inter_search_8        : 1;
  guint inter_search_4        : 1;
  /* mb - bframes */
  guint use_as_reference      : 1;
  guint bidir_me                  : 1;
  guint adaptive              : 1;
  guint weighted_biprediction : 1;
  /* more - me */
  guint bframe_rdo            : 1;
  guint chroma_me             : 1;
  guint mixed_refs            : 1;
  /* more - misc */
  guint cabac                 : 1;
  /* more - misc - df */
  guint deblocking_filter     : 1;
};

x264_param_t *x264_gtk_param_get (X264_Gtk *x264_gtk);
X264_Gtk     *x264_gtk_load (void);
GtkWidget    *x264_gtk_window_create (GtkWidget *parent);
void          x264_gtk_shutdown (GtkWidget *dialog);


#endif /* __X264_GTK_H__ */

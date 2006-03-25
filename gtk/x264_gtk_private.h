#ifndef __X264_GTK_PRIVATE_H__
#define __X264_GTK_PRIVATE_H__


#define __UNUSED__ __attribute__((unused))


typedef struct Bitrate_ Bitrate;
typedef struct Rate_Control_ Rate_Control;
typedef struct MB_ MB;
typedef struct More_ More;
typedef struct Zones_ Zones;
typedef struct X264_Gui_Config_ X264_Gui_Config;
typedef struct X264_Gui_Zone_ X264_Gui_Zone;

struct Bitrate_
{
  GtkWidget *pass;
  GtkWidget *label;
  GtkWidget *w_quantizer;
  GtkWidget *w_average_bitrate;
  GtkWidget *w_target_bitrate;

  GtkWidget *update_statfile;
  GtkWidget *statsfile_name;
};

struct Rate_Control_
{
  /* bitrate */
  struct
  {
    GtkWidget *keyframe_boost;
    GtkWidget *bframes_reduction;
    GtkWidget *bitrate_variability;
  }bitrate;

  /* Quantization Limits */
  struct
  {
    GtkWidget *min_qp;
    GtkWidget *max_qp;
    GtkWidget *max_qp_step;
  }quantization_limits;

  /* Scene Cuts */
  struct
  {
    GtkWidget *scene_cut_threshold;
    GtkWidget *min_idr_frame_interval;
    GtkWidget *max_idr_frame_interval;
  }scene_cuts;

};

struct MB_
{
  /* Partitions */
  struct
  {
    GtkWidget *transform_8x8;
    GtkWidget *pframe_search_8;
    GtkWidget *bframe_search_8;
    GtkWidget *pframe_search_4;
    GtkWidget *inter_search_8;
    GtkWidget *inter_search_4;
  }partitions;

  /* B-frames */
  struct
  {
    GtkWidget *use_as_reference;
    GtkWidget *bidir_me;
    GtkWidget *adaptive;
    GtkWidget *weighted_biprediction;
    GtkWidget *max_consecutive;
    GtkWidget *bias;
    GtkWidget *direct_mode;
  }bframes;

};

struct More_
{
  /* Motion estimation */
  struct
  {
    GtkWidget *partition_decision;
    GtkWidget *method;
    GtkWidget *range;
    GtkWidget *chroma_me;
    GtkWidget *max_ref_frames;
    GtkWidget *mixed_refs;
  }motion_estimation;

  /* Misc. Options */
  struct
  {
    GtkWidget *sample_ar_x;
    GtkWidget *sample_ar_y;
    GtkWidget *threads;
    GtkWidget *cabac;
    GtkWidget *trellis;
    GtkWidget *noise_reduction;

    struct
    {
      GtkWidget *deblocking_filter;
      GtkWidget *strength;
      GtkWidget *threshold;
    }df;

  }misc;

  struct
  {
    GtkWidget *log_level;
    GtkWidget *fourcc;
  }debug;

};

struct Zones_
{
  GtkWidget *list_zones;
};

struct X264_Gui_Config_
{
  Bitrate      bitrate;
  Rate_Control rate_control;
  MB           mb;
  More         more;
  Zones        zones;
};

struct X264_Gui_Zone_
{
  GtkWidget *entry_start_frame;
  GtkWidget *radio_qp;
  GtkWidget *entry_qp;
  GtkWidget *entry_weight;
};


#endif /* __X264_GTK_PRIVATE_H__ */

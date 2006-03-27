#ifndef __X264_GTK_MUXERS_H__
#define __X264_GTK_MUXERS_H__

#include "../config.h"
#include "../muxers.h"

typedef enum {
  X264_DEMUXER_YUV    = 0,
  X264_DEMUXER_CIF,
  X264_DEMUXER_QCIF,
  X264_DEMUXER_Y4M,
  X264_DEMUXER_AVI,
  X264_DEMUXER_AVS,
  X264_DEMUXER_UNKOWN
} X264_Demuxer_Type;
/* static int X264_Num_Demuxers = (int)X264_DEMUXER_UNKOWN; */

#endif  /* __X264_GTK_MUXERS_H__ */

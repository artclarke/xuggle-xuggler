/*****************************************************************************
 * x264_gtk_enum.h: h264 gtk encoder frontend
 *****************************************************************************
 * Copyright (C) 2006 Vincent Torri
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111, USA.
 *****************************************************************************/

#ifndef X264_GTK_ENUM_H
#define X264_GTK_ENUM_H

typedef enum
{
  X264_PASS_SINGLE_BITRATE,
  X264_PASS_SINGLE_QUANTIZER,
  X264_PASS_MULTIPASS_1ST,
  X264_PASS_MULTIPASS_1ST_FAST,
  X264_PASS_MULTIPASS_NTH
}X264_Pass;

typedef enum
{
  X264_NONE     = X264_DIRECT_PRED_NONE,
  X264_SPATIAL  = X264_DIRECT_PRED_SPATIAL,
  X264_TEMPORAL = X264_DIRECT_PRED_TEMPORAL,
  X264_AUTO     = X264_DIRECT_PRED_AUTO
}X264_Direct_Mode;

typedef enum
{
  X264_PD_1,
  X264_PD_2,
  X264_PD_3,
  X264_PD_4,
  X264_PD_5,
  X264_PD_6,
  X264_PD_6b
}X264_Partition_Decision;

typedef enum
{
  X264_ME_METHOD_DIAMOND          = X264_ME_DIA,
  X264_ME_METHOD_HEXAGONAL        = X264_ME_HEX,
  X264_ME_METHOD_UNEVEN_MULTIHEXA = X264_ME_UMH,
  X264_ME_METHOD_EXHAUSTIVE       = X264_ME_ESA
}X264_Me_Method;

typedef enum
{
  X264_DEBUG_METHOD_NONE    = X264_LOG_NONE + 1,
  X264_DEBUG_METHOD_ERROR   = X264_LOG_ERROR + 1,
  X264_DEBUG_METHOD_WARNING = X264_LOG_WARNING + 1,
  X264_DEBUG_METHOD_INFO    = X264_LOG_INFO + 1,
  X264_DEBUG_METHOD_DEBUG   = X264_LOG_DEBUG + 1
}X264_Debug_Method;

typedef enum
{
  X264_CQM_PRESET_FLAT   = X264_CQM_FLAT,
  X264_CQM_PRESET_JVT    = X264_CQM_JVT,
  X264_CQM_PRESET_CUSTOM = X264_CQM_CUSTOM
}X264_Cqm_Preset;


#endif /* X264_GTK_ENUM_H */

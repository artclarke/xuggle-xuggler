/*****************************************************************************
 * output.h: x264 file output modules
 *****************************************************************************
 * Copyright (C) 2003-2009 x264 project
 *
 * Authors: Laurent Aimar <fenrir@via.ecp.fr>
 *          Loren Merritt <lorenm@u.washington.edu>
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

#ifndef X264_OUTPUT_H
#define X264_OUTPUT_H

typedef struct
{
    int (*open_file)( char *psz_filename, hnd_t *p_handle );
    int (*set_param)( hnd_t handle, x264_param_t *p_param );
    int (*write_nalu)( hnd_t handle, uint8_t *p_nal, int i_size );
    int (*set_eop)( hnd_t handle, x264_picture_t *p_picture );
    int (*close_file)( hnd_t handle );
} cli_output_t;

extern cli_output_t raw_output;
extern cli_output_t mkv_output;
extern cli_output_t mp4_output;

#endif

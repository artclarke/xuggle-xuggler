/*****************************************************************************
 * input.h: x264 file input modules
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

#ifndef X264_INPUT_H
#define X264_INPUT_H

typedef struct
{
    int (*open_file)( char *psz_filename, hnd_t *p_handle, x264_param_t *p_param );
    int (*get_frame_total)( hnd_t handle );
    int (*read_frame)( x264_picture_t *p_pic, hnd_t handle, int i_frame );
    int (*close_file)( hnd_t handle );
} cli_input_t;

extern cli_input_t yuv_input;
extern cli_input_t y4m_input;
extern cli_input_t avis_input;
extern cli_input_t thread_input;

#endif

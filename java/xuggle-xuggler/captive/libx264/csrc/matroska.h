/*****************************************************************************
 * matroska.h:
 *****************************************************************************
 * Copyright (C) 2005 Mike Matsnev
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

#ifndef X264_MATROSKA_H
#define X264_MATROSKA_H

typedef struct mk_Writer mk_Writer;

mk_Writer *mk_createWriter( const char *filename );

int  mk_writeHeader( mk_Writer *w, const char *writingApp,
                     const char *codecID,
                     const void *codecPrivate, unsigned codecPrivateSize,
                     int64_t default_frame_duration,
                     int64_t timescale,
                     unsigned width, unsigned height,
                     unsigned d_width, unsigned d_height );

int  mk_startFrame( mk_Writer *w );
int  mk_addFrameData( mk_Writer *w, const void *data, unsigned size );
int  mk_setFrameFlags( mk_Writer *w, int64_t timestamp, int keyframe );
int  mk_close( mk_Writer *w );

#endif

/*****************************************************************************
 * vlc.h: h264 decoder
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar
 * $Id: vlc.h,v 1.1 2004/06/03 19:27:07 fenrir Exp $
 *
 * Authors: Laurent Aimar <fenrir@via.ecp.fr>
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111, USA.
 *****************************************************************************/

#ifndef _DECODER_VLC_H
#define _DECODER_VLC_H 1

typedef struct
{
    int i_value;
    int i_size;
} vlc_lookup_t;

struct x264_vlc_table_t
{
    int          i_lookup_bits;

    int          i_lookup;
    vlc_lookup_t *lookup;
};

x264_vlc_table_t *x264_vlc_table_lookup_new( const vlc_t *vlc, int i_vlc, int i_lookup_bits );

void x264_vlc_table_lookup_delete( x264_vlc_table_t *table );

#endif


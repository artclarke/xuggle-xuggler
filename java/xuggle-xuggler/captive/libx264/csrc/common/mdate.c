/*****************************************************************************
 * mdate.c: h264 encoder
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar <fenrir@via.ecp.fr>
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

#if !(defined(_MSC_VER) || defined(__MINGW32__))
#include <sys/time.h>
#else
#include <sys/types.h>
#include <sys/timeb.h>
#endif
#include <time.h>

#include "common.h"
#include "osdep.h"

int64_t x264_mdate( void )
{
#if !(defined(_MSC_VER) || defined(__MINGW32__))
    struct timeval tv_date;

    gettimeofday( &tv_date, NULL );
    return( (int64_t) tv_date.tv_sec * 1000000 + (int64_t) tv_date.tv_usec );
#else
    struct _timeb tb;
    _ftime(&tb);
    return ((int64_t)tb.time * (1000) + (int64_t)tb.millitm) * (1000);
#endif
}


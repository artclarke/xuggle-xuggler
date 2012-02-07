/*****************************************************************************
 * bitstream.c: bitstream writing
 *****************************************************************************
 * Copyright (C) 2003-2012 x264 project
 *
 * Authors: Laurent Aimar <fenrir@via.ecp.fr>
 *          Jason Garrett-Glaser <darkshikari@gmail.com>
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
 *
 * This program is also available under a commercial proprietary license.
 * For more information, contact us at licensing@x264.com.
 *****************************************************************************/

#include "common.h"

static uint8_t *x264_nal_escape_c( uint8_t *dst, uint8_t *src, uint8_t *end )
{
    if( src < end ) *dst++ = *src++;
    if( src < end ) *dst++ = *src++;
    while( src < end )
    {
        if( src[0] <= 0x03 && !dst[-2] && !dst[-1] )
            *dst++ = 0x03;
        *dst++ = *src++;
    }
    return dst;
}

#if HAVE_MMX
uint8_t *x264_nal_escape_mmx2( uint8_t *dst, uint8_t *src, uint8_t *end );
uint8_t *x264_nal_escape_sse2( uint8_t *dst, uint8_t *src, uint8_t *end );
uint8_t *x264_nal_escape_avx( uint8_t *dst, uint8_t *src, uint8_t *end );
#endif

/****************************************************************************
 * x264_nal_encode:
 ****************************************************************************/
void x264_nal_encode( x264_t *h, uint8_t *dst, x264_nal_t *nal )
{
    uint8_t *src = nal->p_payload;
    uint8_t *end = nal->p_payload + nal->i_payload;
    uint8_t *orig_dst = dst;

    if( h->param.b_annexb )
    {
        if( nal->b_long_startcode )
            *dst++ = 0x00;
        *dst++ = 0x00;
        *dst++ = 0x00;
        *dst++ = 0x01;
    }
    else /* save room for size later */
        dst += 4;

    /* nal header */
    *dst++ = ( 0x00 << 7 ) | ( nal->i_ref_idc << 5 ) | nal->i_type;

    dst = h->bsf.nal_escape( dst, src, end );
    int size = (dst - orig_dst) - 4;

    /* Write the size header for mp4/etc */
    if( !h->param.b_annexb )
    {
        /* Size doesn't include the size of the header we're writing now. */
        orig_dst[0] = size>>24;
        orig_dst[1] = size>>16;
        orig_dst[2] = size>> 8;
        orig_dst[3] = size>> 0;
    }

    nal->i_payload = size+4;
    nal->p_payload = orig_dst;
    x264_emms();
}

void x264_bitstream_init( int cpu, x264_bitstream_function_t *pf )
{
    pf->nal_escape = x264_nal_escape_c;
#if HAVE_MMX
    if( cpu&X264_CPU_MMX2 )
        pf->nal_escape = x264_nal_escape_mmx2;
    if( (cpu&X264_CPU_SSE2) && (cpu&X264_CPU_SSE2_IS_FAST) )
        pf->nal_escape = x264_nal_escape_sse2;
    if( cpu&X264_CPU_AVX )
        pf->nal_escape = x264_nal_escape_avx;
#endif
}

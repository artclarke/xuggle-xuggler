/*****************************************************************************
 * csp.c: h264 encoder library
 *****************************************************************************
 * Copyright (C) 2004 Laurent Aimar
 * $Id: csp.c,v 1.1 2004/06/03 19:27:06 fenrir Exp $
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

#include <stdio.h>
#include <string.h>

#include "common.h"

static inline void plane_copy( uint8_t *dst, int i_dst,
                               uint8_t *src, int i_src, int w, int h)
{
    for( ; h > 0; h-- )
    {
        memcpy( dst, src, w );
        dst += i_dst;
        src += i_src;
    }
}
static inline void plane_copy_vflip( uint8_t *dst, int i_dst,
                                     uint8_t *src, int i_src, int w, int h)
{
    plane_copy( dst, i_dst, src + (h -1)*i_src, -i_src, w, h );
}

static inline void plane_subsamplev2( uint8_t *dst, int i_dst,
                                      uint8_t *src, int i_src, int w, int h)
{
    for( ; h > 0; h-- )
    {
        uint8_t *d = dst;
        uint8_t *s = src;
        int     i;
        for( i = 0; i < w; i++ )
        {
            *d++ = ( s[0] + s[i_src] + 1 ) >> 1;
            s++;
        }
        dst += i_dst;
        src += 2 * i_src;
    }
}

static inline void plane_subsamplev2_vlip( uint8_t *dst, int i_dst,
                                           uint8_t *src, int i_src, int w, int h)
{
    plane_subsamplev2( dst, i_dst, src + (2*h-1)*i_src, -i_src, w, h );
}

static inline void plane_subsamplehv2( uint8_t *dst, int i_dst,
                                       uint8_t *src, int i_src, int w, int h)
{
    for( ; h > 0; h-- )
    {
        uint8_t *d = dst;
        uint8_t *s = src;
        int     i;
        for( i = 0; i < w; i++ )
        {
            *d++ = ( s[0] + s[1] + s[i_src] + s[i_src+1] + 1 ) >> 2;
            s += 2;
        }
        dst += i_dst;
        src += 2 * i_src;
    }
}

static inline void plane_subsamplehv2_vlip( uint8_t *dst, int i_dst,
                                            uint8_t *src, int i_src, int w, int h)
{
    plane_subsamplehv2( dst, i_dst, src + (2*h-1)*i_src, -i_src, w, h );
}

static void i420_to_i420( x264_frame_t *frm, x264_image_t *img,
                          int i_width, int i_height )
{
    if( img->i_csp & X264_CSP_VFLIP )
    {
        plane_copy_vflip( frm->plane[0], frm->i_stride[0],
                          img->plane[0], img->i_stride[0],
                          i_width, i_height );
        plane_copy_vflip( frm->plane[1], frm->i_stride[1],
                          img->plane[1], img->i_stride[1],
                          i_width / 2, i_height / 2 );
        plane_copy_vflip( frm->plane[2], frm->i_stride[2],
                          img->plane[2], img->i_stride[2],
                          i_width / 2, i_height / 2 );
    }
    else
    {
        plane_copy( frm->plane[0], frm->i_stride[0],
                    img->plane[0], img->i_stride[0],
                    i_width, i_height );
        plane_copy( frm->plane[1], frm->i_stride[1],
                    img->plane[1], img->i_stride[1],
                    i_width / 2, i_height / 2 );
        plane_copy( frm->plane[2], frm->i_stride[2],
                    img->plane[2], img->i_stride[2],
                    i_width / 2, i_height / 2 );
    }
}

static void yv12_to_i420( x264_frame_t *frm, x264_image_t *img,
                          int i_width, int i_height )
{
    if( img->i_csp & X264_CSP_VFLIP )
    {
        plane_copy_vflip( frm->plane[0], frm->i_stride[0],
                          img->plane[0], img->i_stride[0],
                          i_width, i_height );
        plane_copy_vflip( frm->plane[2], frm->i_stride[2],
                          img->plane[1], img->i_stride[1],
                          i_width / 2, i_height / 2 );
        plane_copy_vflip( frm->plane[1], frm->i_stride[1],
                          img->plane[2], img->i_stride[2],
                          i_width / 2, i_height / 2 );
    }
    else
    {
        plane_copy( frm->plane[0], frm->i_stride[0],
                    img->plane[0], img->i_stride[0],
                    i_width, i_height );
        plane_copy( frm->plane[2], frm->i_stride[2],
                    img->plane[1], img->i_stride[1],
                    i_width / 2, i_height / 2 );
        plane_copy( frm->plane[1], frm->i_stride[1],
                    img->plane[2], img->i_stride[2],
                    i_width / 2, i_height / 2 );
    }
}

static void i422_to_i420( x264_frame_t *frm, x264_image_t *img,
                          int i_width, int i_height )
{
    if( img->i_csp & X264_CSP_VFLIP )
    {
        plane_copy_vflip( frm->plane[0], frm->i_stride[0],
                          img->plane[0], img->i_stride[0],
                          i_width, i_height );

        plane_subsamplev2_vlip( frm->plane[1], frm->i_stride[1],
                                img->plane[1], img->i_stride[1],
                                i_width / 2, i_height / 2 );
        plane_subsamplev2_vlip( frm->plane[2], frm->i_stride[2],
                                img->plane[2], img->i_stride[2],
                                i_width / 2, i_height / 2 );
    }
    else
    {
        plane_copy( frm->plane[0], frm->i_stride[0],
                    img->plane[0], img->i_stride[0],
                    i_width, i_height );

        plane_subsamplev2( frm->plane[1], frm->i_stride[1],
                           img->plane[1], img->i_stride[1],
                           i_width / 2, i_height / 2 );
        plane_subsamplev2( frm->plane[2], frm->i_stride[2],
                           img->plane[2], img->i_stride[2],
                           i_width / 2, i_height / 2 );
    }
}

static void i444_to_i420( x264_frame_t *frm, x264_image_t *img,
                          int i_width, int i_height )
{
    if( img->i_csp & X264_CSP_VFLIP )
    {
        plane_copy_vflip( frm->plane[0], frm->i_stride[0],
                          img->plane[0], img->i_stride[0],
                          i_width, i_height );

        plane_subsamplehv2_vlip( frm->plane[1], frm->i_stride[1],
                                 img->plane[1], img->i_stride[1],
                                 i_width / 2, i_height / 2 );
        plane_subsamplehv2_vlip( frm->plane[2], frm->i_stride[2],
                                 img->plane[2], img->i_stride[2],
                                 i_width / 2, i_height / 2 );
    }
    else
    {
        plane_copy( frm->plane[0], frm->i_stride[0],
                    img->plane[0], img->i_stride[0],
                    i_width, i_height );

        plane_subsamplehv2( frm->plane[1], frm->i_stride[1],
                            img->plane[1], img->i_stride[1],
                            i_width / 2, i_height / 2 );
        plane_subsamplehv2( frm->plane[2], frm->i_stride[2],
                            img->plane[2], img->i_stride[2],
                            i_width / 2, i_height / 2 );
    }
}
static void yuyv_to_i420( x264_frame_t *frm, x264_image_t *img,
                          int i_width, int i_height )
{
    uint8_t *src = img->plane[0];
    int     i_src= img->i_stride[0];

    uint8_t *y   = frm->plane[0];
    uint8_t *u   = frm->plane[1];
    uint8_t *v   = frm->plane[2];

    if( img->i_csp & X264_CSP_VFLIP )
    {
        src += ( i_height - 1 ) * i_src;
        i_src = -i_src;
    }

    for( ; i_height > 0; i_height -= 2 )
    {
        uint8_t *ss = src;
        uint8_t *yy = y;
        uint8_t *uu = u;
        uint8_t *vv = v;
        int w;

        for( w = i_width; w > 0; w -= 2 )
        {
            *yy++ = ss[0];
            *yy++ = ss[2];

            *uu++ = ( ss[1] + ss[1+i_src] + 1 ) >> 1;
            *vv++ = ( ss[3] + ss[3+i_src] + 1 ) >> 1;

            ss += 4;
        }
        src += i_src;
        y += frm->i_stride[0];
        u += frm->i_stride[1];
        v += frm->i_stride[2];

        ss = src;
        yy = y;
        for( w = i_width; w > 0; w -= 2 )
        {
            *yy++ = ss[0];
            *yy++ = ss[2];
            ss += 4;
        }
        src += i_src;
        y += frm->i_stride[0];
    }
}

/* Same value than in XviD */
#define BITS 8
#define FIX(f) ((int)((f) * (1 << BITS) + 0.5))

#define Y_R   FIX(0.257)
#define Y_G   FIX(0.504)
#define Y_B   FIX(0.098)
#define Y_ADD 16

#define U_R   FIX(0.148)
#define U_G   FIX(0.291)
#define U_B   FIX(0.439)
#define U_ADD 128

#define V_R   FIX(0.439)
#define V_G   FIX(0.368)
#define V_B   FIX(0.071)
#define V_ADD 128
#define RGB_TO_I420( name, POS_R, POS_G, POS_B, S_RGB ) \
static void name( x264_frame_t *frm, x264_image_t *img, \
                  int i_width, int i_height )           \
{                                                       \
    uint8_t *src = img->plane[0];                       \
    int     i_src= img->i_stride[0];                    \
    int     i_y  = frm->i_stride[0];                    \
    uint8_t *y   = frm->plane[0];                       \
    uint8_t *u   = frm->plane[1];                       \
    uint8_t *v   = frm->plane[2];                       \
                                                        \
    if( img->i_csp & X264_CSP_VFLIP )                   \
    {                                                   \
        src += ( i_height - 1 ) * i_src;                \
        i_src = -i_src;                                 \
    }                                                   \
                                                        \
    for(  ; i_height > 0; i_height -= 2 )               \
    {                                                   \
        uint8_t *ss = src;                              \
        uint8_t *yy = y;                                \
        uint8_t *uu = u;                                \
        uint8_t *vv = v;                                \
        int w;                                          \
                                                        \
        for( w = i_width; w > 0; w -= 2 )               \
        {                                               \
            int cr = 0,cg = 0,cb = 0;                   \
            int r, g, b;                                \
                                                        \
            /* Luma */                                  \
            cr = r = ss[POS_R];                         \
            cg = g = ss[POS_G];                         \
            cb = b = ss[POS_B];                         \
                                                        \
            yy[0] = Y_ADD + ((Y_R * r + Y_G * g + Y_B * b) >> BITS);    \
                                                        \
            cr+= r = ss[POS_R+i_src];                   \
            cg+= g = ss[POS_G+i_src];                   \
            cb+= b = ss[POS_B+i_src];                   \
            yy[i_y] = Y_ADD + ((Y_R * r + Y_G * g + Y_B * b) >> BITS);  \
            yy++;                                       \
            ss += S_RGB;                                \
                                                        \
            cr+= r = ss[POS_R];                         \
            cg+= g = ss[POS_G];                         \
            cb+= b = ss[POS_B];                         \
                                                        \
            yy[0] = Y_ADD + ((Y_R * r + Y_G * g + Y_B * b) >> BITS);    \
                                                        \
            cr+= r = ss[POS_R+i_src];                   \
            cg+= g = ss[POS_G+i_src];                   \
            cb+= b = ss[POS_B+i_src];                   \
            yy[i_y] = Y_ADD + ((Y_R * r + Y_G * g + Y_B * b) >> BITS);  \
            yy++;                                       \
            ss += S_RGB;                                \
                                                        \
            /* Chroma */                                \
            *uu++ = (uint8_t)(U_ADD + ((-U_R * cr - U_G * cg + U_B * cb) >> (BITS+2)) ); \
            *vv++ = (uint8_t)(V_ADD + (( V_R * cr - V_G * cg - V_B * cb) >> (BITS+2)) ); \
        }                                               \
                                                        \
        src += 2*i_src;                                   \
        y += 2*frm->i_stride[0];                        \
        u += frm->i_stride[1];                          \
        v += frm->i_stride[2];                          \
    }                                                   \
}

RGB_TO_I420( rgb_to_i420,  0, 1, 2, 3 );
RGB_TO_I420( bgr_to_i420,  2, 1, 0, 3 );
RGB_TO_I420( bgra_to_i420, 2, 1, 0, 4 );

void x264_csp_init( int cpu, int i_csp, x264_csp_function_t *pf )
{
    switch( i_csp )
    {
        case X264_CSP_I420:
            pf->i420 = i420_to_i420;
            pf->i422 = i422_to_i420;
            pf->i444 = i444_to_i420;
            pf->yv12 = yv12_to_i420;
            pf->yuyv = yuyv_to_i420;
            pf->rgb  = rgb_to_i420;
            pf->bgr  = bgr_to_i420;
            pf->bgra = bgra_to_i420;
            break;

        default:
            /* For now, can't happen */
            fprintf( stderr, "arg in x264_csp_init\n" );
            exit( -1 );
            break;
    }
}


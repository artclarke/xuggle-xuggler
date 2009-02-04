/*****************************************************************************
 * predict.c: h264 encoder
 *****************************************************************************
 * Copyright (C) 2003-2008 x264 project
 *
 * Authors: Laurent Aimar <fenrir@via.ecp.fr>
 *          Loren Merritt <lorenm@u.washington.edu>
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
 *****************************************************************************/

/* predict4x4 are inspired from ffmpeg h264 decoder */


#include "common.h"

#ifdef HAVE_MMX
#   include "x86/predict.h"
#endif
#ifdef ARCH_PPC
#   include "ppc/predict.h"
#endif

/****************************************************************************
 * 16x16 prediction for intra luma block
 ****************************************************************************/

#define PREDICT_16x16_DC(v) \
    for( i = 0; i < 16; i++ )\
    {\
        uint32_t *p = (uint32_t*)src;\
        *p++ = v;\
        *p++ = v;\
        *p++ = v;\
        *p++ = v;\
        src += FDEC_STRIDE;\
    }

static void predict_16x16_dc( uint8_t *src )
{
    uint32_t dc = 0;
    int i;

    for( i = 0; i < 16; i++ )
    {
        dc += src[-1 + i * FDEC_STRIDE];
        dc += src[i - FDEC_STRIDE];
    }
    dc = (( dc + 16 ) >> 5) * 0x01010101;

    PREDICT_16x16_DC(dc);
}
static void predict_16x16_dc_left( uint8_t *src )
{
    uint32_t dc = 0;
    int i;

    for( i = 0; i < 16; i++ )
    {
        dc += src[-1 + i * FDEC_STRIDE];
    }
    dc = (( dc + 8 ) >> 4) * 0x01010101;

    PREDICT_16x16_DC(dc);
}
static void predict_16x16_dc_top( uint8_t *src )
{
    uint32_t dc = 0;
    int i;

    for( i = 0; i < 16; i++ )
    {
        dc += src[i - FDEC_STRIDE];
    }
    dc = (( dc + 8 ) >> 4) * 0x01010101;

    PREDICT_16x16_DC(dc);
}
static void predict_16x16_dc_128( uint8_t *src )
{
    int i;
    PREDICT_16x16_DC(0x80808080);
}
static void predict_16x16_h( uint8_t *src )
{
    int i;

    for( i = 0; i < 16; i++ )
    {
        const uint32_t v = 0x01010101 * src[-1];
        uint32_t *p = (uint32_t*)src;

        *p++ = v;
        *p++ = v;
        *p++ = v;
        *p++ = v;

        src += FDEC_STRIDE;

    }
}
static void predict_16x16_v( uint8_t *src )
{
    uint32_t v0 = *(uint32_t*)&src[ 0-FDEC_STRIDE];
    uint32_t v1 = *(uint32_t*)&src[ 4-FDEC_STRIDE];
    uint32_t v2 = *(uint32_t*)&src[ 8-FDEC_STRIDE];
    uint32_t v3 = *(uint32_t*)&src[12-FDEC_STRIDE];
    int i;

    for( i = 0; i < 16; i++ )
    {
        uint32_t *p = (uint32_t*)src;
        *p++ = v0;
        *p++ = v1;
        *p++ = v2;
        *p++ = v3;
        src += FDEC_STRIDE;
    }
}
static void predict_16x16_p( uint8_t *src )
{
    int x, y, i;
    int a, b, c;
    int H = 0;
    int V = 0;
    int i00;

    /* calculate H and V */
    for( i = 0; i <= 7; i++ )
    {
        H += ( i + 1 ) * ( src[ 8 + i - FDEC_STRIDE ] - src[6 -i -FDEC_STRIDE] );
        V += ( i + 1 ) * ( src[-1 + (8+i)*FDEC_STRIDE] - src[-1 + (6-i)*FDEC_STRIDE] );
    }

    a = 16 * ( src[-1 + 15*FDEC_STRIDE] + src[15 - FDEC_STRIDE] );
    b = ( 5 * H + 32 ) >> 6;
    c = ( 5 * V + 32 ) >> 6;

    i00 = a - b * 7 - c * 7 + 16;

    for( y = 0; y < 16; y++ )
    {
        int pix = i00;
        for( x = 0; x < 16; x++ )
        {
            src[x] = x264_clip_uint8( pix>>5 );
            pix += b;
        }
        src += FDEC_STRIDE;
        i00 += c;
    }
}


/****************************************************************************
 * 8x8 prediction for intra chroma block
 ****************************************************************************/

static void predict_8x8c_dc_128( uint8_t *src )
{
    int y;

    for( y = 0; y < 8; y++ )
    {
        uint32_t *p = (uint32_t*)src;
        *p++ = 0x80808080;
        *p++ = 0x80808080;
        src += FDEC_STRIDE;
    }
}
static void predict_8x8c_dc_left( uint8_t *src )
{
    int y;
    uint32_t dc0 = 0, dc1 = 0;

    for( y = 0; y < 4; y++ )
    {
        dc0 += src[y * FDEC_STRIDE     - 1];
        dc1 += src[(y+4) * FDEC_STRIDE - 1];
    }
    dc0 = (( dc0 + 2 ) >> 2)*0x01010101;
    dc1 = (( dc1 + 2 ) >> 2)*0x01010101;

    for( y = 0; y < 4; y++ )
    {
        uint32_t *p = (uint32_t*)src;
        *p++ = dc0;
        *p++ = dc0;
        src += FDEC_STRIDE;
    }
    for( y = 0; y < 4; y++ )
    {
        uint32_t *p = (uint32_t*)src;
        *p++ = dc1;
        *p++ = dc1;
        src += FDEC_STRIDE;
    }

}
static void predict_8x8c_dc_top( uint8_t *src )
{
    int y, x;
    uint32_t dc0 = 0, dc1 = 0;

    for( x = 0; x < 4; x++ )
    {
        dc0 += src[x     - FDEC_STRIDE];
        dc1 += src[x + 4 - FDEC_STRIDE];
    }
    dc0 = (( dc0 + 2 ) >> 2)*0x01010101;
    dc1 = (( dc1 + 2 ) >> 2)*0x01010101;

    for( y = 0; y < 8; y++ )
    {
        uint32_t *p = (uint32_t*)src;
        *p++ = dc0;
        *p++ = dc1;
        src += FDEC_STRIDE;
    }
}
static void predict_8x8c_dc( uint8_t *src )
{
    int y;
    int s0 = 0, s1 = 0, s2 = 0, s3 = 0;
    uint32_t dc0, dc1, dc2, dc3;
    int i;

    /*
          s0 s1
       s2
       s3
    */
    for( i = 0; i < 4; i++ )
    {
        s0 += src[i - FDEC_STRIDE];
        s1 += src[i + 4 - FDEC_STRIDE];
        s2 += src[-1 + i * FDEC_STRIDE];
        s3 += src[-1 + (i+4)*FDEC_STRIDE];
    }
    /*
       dc0 dc1
       dc2 dc3
     */
    dc0 = (( s0 + s2 + 4 ) >> 3)*0x01010101;
    dc1 = (( s1 + 2 ) >> 2)*0x01010101;
    dc2 = (( s3 + 2 ) >> 2)*0x01010101;
    dc3 = (( s1 + s3 + 4 ) >> 3)*0x01010101;

    for( y = 0; y < 4; y++ )
    {
        uint32_t *p = (uint32_t*)src;
        *p++ = dc0;
        *p++ = dc1;
        src += FDEC_STRIDE;
    }

    for( y = 0; y < 4; y++ )
    {
        uint32_t *p = (uint32_t*)src;
        *p++ = dc2;
        *p++ = dc3;
        src += FDEC_STRIDE;
    }
}
static void predict_8x8c_h( uint8_t *src )
{
    int i;

    for( i = 0; i < 8; i++ )
    {
        uint32_t v = 0x01010101 * src[-1];
        uint32_t *p = (uint32_t*)src;
        *p++ = v;
        *p++ = v;
        src += FDEC_STRIDE;
    }
}
static void predict_8x8c_v( uint8_t *src )
{
    uint32_t v0 = *(uint32_t*)&src[0-FDEC_STRIDE];
    uint32_t v1 = *(uint32_t*)&src[4-FDEC_STRIDE];
    int i;

    for( i = 0; i < 8; i++ )
    {
        uint32_t *p = (uint32_t*)src;
        *p++ = v0;
        *p++ = v1;
        src += FDEC_STRIDE;
    }
}
static void predict_8x8c_p( uint8_t *src )
{
    int i;
    int x,y;
    int a, b, c;
    int H = 0;
    int V = 0;
    int i00;

    for( i = 0; i < 4; i++ )
    {
        H += ( i + 1 ) * ( src[4+i - FDEC_STRIDE] - src[2 - i -FDEC_STRIDE] );
        V += ( i + 1 ) * ( src[-1 +(i+4)*FDEC_STRIDE] - src[-1+(2-i)*FDEC_STRIDE] );
    }

    a = 16 * ( src[-1+7*FDEC_STRIDE] + src[7 - FDEC_STRIDE] );
    b = ( 17 * H + 16 ) >> 5;
    c = ( 17 * V + 16 ) >> 5;
    i00 = a -3*b -3*c + 16;

    for( y = 0; y < 8; y++ )
    {
        int pix = i00;
        for( x = 0; x < 8; x++ )
        {
            src[x] = x264_clip_uint8( pix>>5 );
            pix += b;
        }
        src += FDEC_STRIDE;
        i00 += c;
    }
}

/****************************************************************************
 * 4x4 prediction for intra luma block
 ****************************************************************************/

#define SRC(x,y) src[(x)+(y)*FDEC_STRIDE]
#define SRC32(x,y) *(uint32_t*)&SRC(x,y)

#define PREDICT_4x4_DC(v)\
    SRC32(0,0) = SRC32(0,1) = SRC32(0,2) = SRC32(0,3) = v;

static void predict_4x4_dc_128( uint8_t *src )
{
    PREDICT_4x4_DC(0x80808080);
}
static void predict_4x4_dc_left( uint8_t *src )
{
    uint32_t dc = ((SRC(-1,0) + SRC(-1,1) + SRC(-1,2) + SRC(-1,3) + 2) >> 2) * 0x01010101;
    PREDICT_4x4_DC(dc);
}
static void predict_4x4_dc_top( uint8_t *src )
{
    uint32_t dc = ((SRC(0,-1) + SRC(1,-1) + SRC(2,-1) + SRC(3,-1) + 2) >> 2) * 0x01010101;
    PREDICT_4x4_DC(dc);
}
static void predict_4x4_dc( uint8_t *src )
{
    uint32_t dc = ((SRC(-1,0) + SRC(-1,1) + SRC(-1,2) + SRC(-1,3) +
                    SRC(0,-1) + SRC(1,-1) + SRC(2,-1) + SRC(3,-1) + 4) >> 3) * 0x01010101;
    PREDICT_4x4_DC(dc);
}
static void predict_4x4_h( uint8_t *src )
{
    SRC32(0,0) = SRC(-1,0) * 0x01010101;
    SRC32(0,1) = SRC(-1,1) * 0x01010101;
    SRC32(0,2) = SRC(-1,2) * 0x01010101;
    SRC32(0,3) = SRC(-1,3) * 0x01010101;
}
static void predict_4x4_v( uint8_t *src )
{
    PREDICT_4x4_DC(SRC32(0,-1));
}

#define PREDICT_4x4_LOAD_LEFT\
    const int l0 = SRC(-1,0);\
    const int l1 = SRC(-1,1);\
    const int l2 = SRC(-1,2);\
    UNUSED const int l3 = SRC(-1,3);

#define PREDICT_4x4_LOAD_TOP\
    const int t0 = SRC(0,-1);\
    const int t1 = SRC(1,-1);\
    const int t2 = SRC(2,-1);\
    UNUSED const int t3 = SRC(3,-1);

#define PREDICT_4x4_LOAD_TOP_RIGHT\
    const int t4 = SRC(4,-1);\
    const int t5 = SRC(5,-1);\
    const int t6 = SRC(6,-1);\
    UNUSED const int t7 = SRC(7,-1);

#define F1(a,b)   (((a)+(b)+1)>>1)
#define F2(a,b,c) (((a)+2*(b)+(c)+2)>>2)

static void predict_4x4_ddl( uint8_t *src )
{
    PREDICT_4x4_LOAD_TOP
    PREDICT_4x4_LOAD_TOP_RIGHT
    SRC(0,0)= F2(t0,t1,t2);
    SRC(1,0)=SRC(0,1)= F2(t1,t2,t3);
    SRC(2,0)=SRC(1,1)=SRC(0,2)= F2(t2,t3,t4);
    SRC(3,0)=SRC(2,1)=SRC(1,2)=SRC(0,3)= F2(t3,t4,t5);
    SRC(3,1)=SRC(2,2)=SRC(1,3)= F2(t4,t5,t6);
    SRC(3,2)=SRC(2,3)= F2(t5,t6,t7);
    SRC(3,3)= F2(t6,t7,t7);
}
static void predict_4x4_ddr( uint8_t *src )
{
    const int lt = SRC(-1,-1);
    PREDICT_4x4_LOAD_LEFT
    PREDICT_4x4_LOAD_TOP
    SRC(3,0)= F2(t3,t2,t1);
    SRC(2,0)=SRC(3,1)= F2(t2,t1,t0);
    SRC(1,0)=SRC(2,1)=SRC(3,2)= F2(t1,t0,lt);
    SRC(0,0)=SRC(1,1)=SRC(2,2)=SRC(3,3)= F2(t0,lt,l0);
    SRC(0,1)=SRC(1,2)=SRC(2,3)= F2(lt,l0,l1);
    SRC(0,2)=SRC(1,3)= F2(l0,l1,l2);
    SRC(0,3)= F2(l1,l2,l3);
}

static void predict_4x4_vr( uint8_t *src )
{
    const int lt = SRC(-1,-1);
    PREDICT_4x4_LOAD_LEFT
    PREDICT_4x4_LOAD_TOP
    SRC(0,3)= F2(l2,l1,l0);
    SRC(0,2)= F2(l1,l0,lt);
    SRC(0,1)=SRC(1,3)= F2(l0,lt,t0);
    SRC(0,0)=SRC(1,2)= F1(lt,t0);
    SRC(1,1)=SRC(2,3)= F2(lt,t0,t1);
    SRC(1,0)=SRC(2,2)= F1(t0,t1);
    SRC(2,1)=SRC(3,3)= F2(t0,t1,t2);
    SRC(2,0)=SRC(3,2)= F1(t1,t2);
    SRC(3,1)= F2(t1,t2,t3);
    SRC(3,0)= F1(t2,t3);
}

static void predict_4x4_hd( uint8_t *src )
{
    const int lt= SRC(-1,-1);
    PREDICT_4x4_LOAD_LEFT
    PREDICT_4x4_LOAD_TOP
    SRC(0,3)= F1(l2,l3);
    SRC(1,3)= F2(l1,l2,l3);
    SRC(0,2)=SRC(2,3)= F1(l1,l2);
    SRC(1,2)=SRC(3,3)= F2(l0,l1,l2);
    SRC(0,1)=SRC(2,2)= F1(l0,l1);
    SRC(1,1)=SRC(3,2)= F2(lt,l0,l1);
    SRC(0,0)=SRC(2,1)= F1(lt,l0);
    SRC(1,0)=SRC(3,1)= F2(t0,lt,l0);
    SRC(2,0)= F2(t1,t0,lt);
    SRC(3,0)= F2(t2,t1,t0);
}

static void predict_4x4_vl( uint8_t *src )
{
    PREDICT_4x4_LOAD_TOP
    PREDICT_4x4_LOAD_TOP_RIGHT
    SRC(0,0)= F1(t0,t1);
    SRC(0,1)= F2(t0,t1,t2);
    SRC(1,0)=SRC(0,2)= F1(t1,t2);
    SRC(1,1)=SRC(0,3)= F2(t1,t2,t3);
    SRC(2,0)=SRC(1,2)= F1(t2,t3);
    SRC(2,1)=SRC(1,3)= F2(t2,t3,t4);
    SRC(3,0)=SRC(2,2)= F1(t3,t4);
    SRC(3,1)=SRC(2,3)= F2(t3,t4,t5);
    SRC(3,2)= F1(t4,t5);
    SRC(3,3)= F2(t4,t5,t6);
}

static void predict_4x4_hu( uint8_t *src )
{
    PREDICT_4x4_LOAD_LEFT
    SRC(0,0)= F1(l0,l1);
    SRC(1,0)= F2(l0,l1,l2);
    SRC(2,0)=SRC(0,1)= F1(l1,l2);
    SRC(3,0)=SRC(1,1)= F2(l1,l2,l3);
    SRC(2,1)=SRC(0,2)= F1(l2,l3);
    SRC(3,1)=SRC(1,2)= F2(l2,l3,l3);
    SRC(3,2)=SRC(1,3)=SRC(0,3)=
    SRC(2,2)=SRC(2,3)=SRC(3,3)= l3;
}

/****************************************************************************
 * 8x8 prediction for intra luma block
 ****************************************************************************/

#define PL(y) \
    edge[14-y] = F2(SRC(-1,y-1), SRC(-1,y), SRC(-1,y+1));
#define PT(x) \
    edge[16+x] = F2(SRC(x-1,-1), SRC(x,-1), SRC(x+1,-1));

void x264_predict_8x8_filter( uint8_t *src, uint8_t edge[33], int i_neighbor, int i_filters )
{
    /* edge[7..14] = l7..l0
     * edge[15] = lt
     * edge[16..31] = t0 .. t15
     * edge[32] = t15 */

    int have_lt = i_neighbor & MB_TOPLEFT;
    if( i_filters & MB_LEFT )
    {
        edge[15] = (SRC(0,-1) + 2*SRC(-1,-1) + SRC(-1,0) + 2) >> 2;
        edge[14] = ((have_lt ? SRC(-1,-1) : SRC(-1,0))
                    + 2*SRC(-1,0) + SRC(-1,1) + 2) >> 2;
        PL(1) PL(2) PL(3) PL(4) PL(5) PL(6)
        edge[7] = (SRC(-1,6) + 3*SRC(-1,7) + 2) >> 2;
    }

    if( i_filters & MB_TOP )
    {
        int have_tr = i_neighbor & MB_TOPRIGHT;
        edge[16] = ((have_lt ? SRC(-1,-1) : SRC(0,-1))
                    + 2*SRC(0,-1) + SRC(1,-1) + 2) >> 2;
        PT(1) PT(2) PT(3) PT(4) PT(5) PT(6)
        edge[23] = (SRC(6,-1) + 2*SRC(7,-1)
                    + (have_tr ? SRC(8,-1) : SRC(7,-1)) + 2) >> 2;

        if( i_filters & MB_TOPRIGHT )
        {
            if( have_tr )
            {
                PT(8) PT(9) PT(10) PT(11) PT(12) PT(13) PT(14)
                edge[31] =
                edge[32] = (SRC(14,-1) + 3*SRC(15,-1) + 2) >> 2;
            }
            else
            {
                *(uint64_t*)(edge+24) = SRC(7,-1) * 0x0101010101010101ULL;
                edge[32] = SRC(7,-1);
            }
        }
    }
}

#undef PL
#undef PT

#define PL(y) \
    UNUSED const int l##y = edge[14-y];
#define PT(x) \
    UNUSED const int t##x = edge[16+x];
#define PREDICT_8x8_LOAD_TOPLEFT \
    const int lt = edge[15];
#define PREDICT_8x8_LOAD_LEFT \
    PL(0) PL(1) PL(2) PL(3) PL(4) PL(5) PL(6) PL(7)
#define PREDICT_8x8_LOAD_TOP \
    PT(0) PT(1) PT(2) PT(3) PT(4) PT(5) PT(6) PT(7)
#define PREDICT_8x8_LOAD_TOPRIGHT \
    PT(8) PT(9) PT(10) PT(11) PT(12) PT(13) PT(14) PT(15)

#define PREDICT_8x8_DC(v) \
    int y; \
    for( y = 0; y < 8; y++ ) { \
        ((uint32_t*)src)[0] = \
        ((uint32_t*)src)[1] = v; \
        src += FDEC_STRIDE; \
    }

static void predict_8x8_dc_128( uint8_t *src, uint8_t edge[33] )
{
    PREDICT_8x8_DC(0x80808080);
}
static void predict_8x8_dc_left( uint8_t *src, uint8_t edge[33] )
{
    PREDICT_8x8_LOAD_LEFT
    const uint32_t dc = ((l0+l1+l2+l3+l4+l5+l6+l7+4) >> 3) * 0x01010101;
    PREDICT_8x8_DC(dc);
}
static void predict_8x8_dc_top( uint8_t *src, uint8_t edge[33] )
{
    PREDICT_8x8_LOAD_TOP
    const uint32_t dc = ((t0+t1+t2+t3+t4+t5+t6+t7+4) >> 3) * 0x01010101;
    PREDICT_8x8_DC(dc);
}
static void predict_8x8_dc( uint8_t *src, uint8_t edge[33] )
{
    PREDICT_8x8_LOAD_LEFT
    PREDICT_8x8_LOAD_TOP
    const uint32_t dc = ((l0+l1+l2+l3+l4+l5+l6+l7
                         +t0+t1+t2+t3+t4+t5+t6+t7+8) >> 4) * 0x01010101;
    PREDICT_8x8_DC(dc);
}
static void predict_8x8_h( uint8_t *src, uint8_t edge[33] )
{
    PREDICT_8x8_LOAD_LEFT
#define ROW(y) ((uint32_t*)(src+y*FDEC_STRIDE))[0] =\
               ((uint32_t*)(src+y*FDEC_STRIDE))[1] = 0x01010101U * l##y
    ROW(0); ROW(1); ROW(2); ROW(3); ROW(4); ROW(5); ROW(6); ROW(7);
#undef ROW
}
static void predict_8x8_v( uint8_t *src, uint8_t edge[33] )
{
    const uint64_t top = *(uint64_t*)(edge+16);
    int y;
    for( y = 0; y < 8; y++ )
        *(uint64_t*)(src+y*FDEC_STRIDE) = top;
}
static void predict_8x8_ddl( uint8_t *src, uint8_t edge[33] )
{
    PREDICT_8x8_LOAD_TOP
    PREDICT_8x8_LOAD_TOPRIGHT
    SRC(0,0)= F2(t0,t1,t2);
    SRC(0,1)=SRC(1,0)= F2(t1,t2,t3);
    SRC(0,2)=SRC(1,1)=SRC(2,0)= F2(t2,t3,t4);
    SRC(0,3)=SRC(1,2)=SRC(2,1)=SRC(3,0)= F2(t3,t4,t5);
    SRC(0,4)=SRC(1,3)=SRC(2,2)=SRC(3,1)=SRC(4,0)= F2(t4,t5,t6);
    SRC(0,5)=SRC(1,4)=SRC(2,3)=SRC(3,2)=SRC(4,1)=SRC(5,0)= F2(t5,t6,t7);
    SRC(0,6)=SRC(1,5)=SRC(2,4)=SRC(3,3)=SRC(4,2)=SRC(5,1)=SRC(6,0)= F2(t6,t7,t8);
    SRC(0,7)=SRC(1,6)=SRC(2,5)=SRC(3,4)=SRC(4,3)=SRC(5,2)=SRC(6,1)=SRC(7,0)= F2(t7,t8,t9);
    SRC(1,7)=SRC(2,6)=SRC(3,5)=SRC(4,4)=SRC(5,3)=SRC(6,2)=SRC(7,1)= F2(t8,t9,t10);
    SRC(2,7)=SRC(3,6)=SRC(4,5)=SRC(5,4)=SRC(6,3)=SRC(7,2)= F2(t9,t10,t11);
    SRC(3,7)=SRC(4,6)=SRC(5,5)=SRC(6,4)=SRC(7,3)= F2(t10,t11,t12);
    SRC(4,7)=SRC(5,6)=SRC(6,5)=SRC(7,4)= F2(t11,t12,t13);
    SRC(5,7)=SRC(6,6)=SRC(7,5)= F2(t12,t13,t14);
    SRC(6,7)=SRC(7,6)= F2(t13,t14,t15);
    SRC(7,7)= F2(t14,t15,t15);
}
static void predict_8x8_ddr( uint8_t *src, uint8_t edge[33] )
{
    PREDICT_8x8_LOAD_TOP
    PREDICT_8x8_LOAD_LEFT
    PREDICT_8x8_LOAD_TOPLEFT
    SRC(0,7)= F2(l7,l6,l5);
    SRC(0,6)=SRC(1,7)= F2(l6,l5,l4);
    SRC(0,5)=SRC(1,6)=SRC(2,7)= F2(l5,l4,l3);
    SRC(0,4)=SRC(1,5)=SRC(2,6)=SRC(3,7)= F2(l4,l3,l2);
    SRC(0,3)=SRC(1,4)=SRC(2,5)=SRC(3,6)=SRC(4,7)= F2(l3,l2,l1);
    SRC(0,2)=SRC(1,3)=SRC(2,4)=SRC(3,5)=SRC(4,6)=SRC(5,7)= F2(l2,l1,l0);
    SRC(0,1)=SRC(1,2)=SRC(2,3)=SRC(3,4)=SRC(4,5)=SRC(5,6)=SRC(6,7)= F2(l1,l0,lt);
    SRC(0,0)=SRC(1,1)=SRC(2,2)=SRC(3,3)=SRC(4,4)=SRC(5,5)=SRC(6,6)=SRC(7,7)= F2(l0,lt,t0);
    SRC(1,0)=SRC(2,1)=SRC(3,2)=SRC(4,3)=SRC(5,4)=SRC(6,5)=SRC(7,6)= F2(lt,t0,t1);
    SRC(2,0)=SRC(3,1)=SRC(4,2)=SRC(5,3)=SRC(6,4)=SRC(7,5)= F2(t0,t1,t2);
    SRC(3,0)=SRC(4,1)=SRC(5,2)=SRC(6,3)=SRC(7,4)= F2(t1,t2,t3);
    SRC(4,0)=SRC(5,1)=SRC(6,2)=SRC(7,3)= F2(t2,t3,t4);
    SRC(5,0)=SRC(6,1)=SRC(7,2)= F2(t3,t4,t5);
    SRC(6,0)=SRC(7,1)= F2(t4,t5,t6);
    SRC(7,0)= F2(t5,t6,t7);

}
static void predict_8x8_vr( uint8_t *src, uint8_t edge[33] )
{
    PREDICT_8x8_LOAD_TOP
    PREDICT_8x8_LOAD_LEFT
    PREDICT_8x8_LOAD_TOPLEFT
    SRC(0,6)= F2(l5,l4,l3);
    SRC(0,7)= F2(l6,l5,l4);
    SRC(0,4)=SRC(1,6)= F2(l3,l2,l1);
    SRC(0,5)=SRC(1,7)= F2(l4,l3,l2);
    SRC(0,2)=SRC(1,4)=SRC(2,6)= F2(l1,l0,lt);
    SRC(0,3)=SRC(1,5)=SRC(2,7)= F2(l2,l1,l0);
    SRC(0,1)=SRC(1,3)=SRC(2,5)=SRC(3,7)= F2(l0,lt,t0);
    SRC(0,0)=SRC(1,2)=SRC(2,4)=SRC(3,6)= F1(lt,t0);
    SRC(1,1)=SRC(2,3)=SRC(3,5)=SRC(4,7)= F2(lt,t0,t1);
    SRC(1,0)=SRC(2,2)=SRC(3,4)=SRC(4,6)= F1(t0,t1);
    SRC(2,1)=SRC(3,3)=SRC(4,5)=SRC(5,7)= F2(t0,t1,t2);
    SRC(2,0)=SRC(3,2)=SRC(4,4)=SRC(5,6)= F1(t1,t2);
    SRC(3,1)=SRC(4,3)=SRC(5,5)=SRC(6,7)= F2(t1,t2,t3);
    SRC(3,0)=SRC(4,2)=SRC(5,4)=SRC(6,6)= F1(t2,t3);
    SRC(4,1)=SRC(5,3)=SRC(6,5)=SRC(7,7)= F2(t2,t3,t4);
    SRC(4,0)=SRC(5,2)=SRC(6,4)=SRC(7,6)= F1(t3,t4);
    SRC(5,1)=SRC(6,3)=SRC(7,5)= F2(t3,t4,t5);
    SRC(5,0)=SRC(6,2)=SRC(7,4)= F1(t4,t5);
    SRC(6,1)=SRC(7,3)= F2(t4,t5,t6);
    SRC(6,0)=SRC(7,2)= F1(t5,t6);
    SRC(7,1)= F2(t5,t6,t7);
    SRC(7,0)= F1(t6,t7);
}
static void predict_8x8_hd( uint8_t *src, uint8_t edge[33] )
{
    PREDICT_8x8_LOAD_TOP
    PREDICT_8x8_LOAD_LEFT
    PREDICT_8x8_LOAD_TOPLEFT
    int p1 = pack8to16(F1(l6,l7), F2(l5,l6,l7));
    int p2 = pack8to16(F1(l5,l6), F2(l4,l5,l6));
    int p3 = pack8to16(F1(l4,l5), F2(l3,l4,l5));
    int p4 = pack8to16(F1(l3,l4), F2(l2,l3,l4));
    int p5 = pack8to16(F1(l2,l3), F2(l1,l2,l3));
    int p6 = pack8to16(F1(l1,l2), F2(l0,l1,l2));
    int p7 = pack8to16(F1(l0,l1), F2(lt,l0,l1));
    int p8 = pack8to16(F1(lt,l0), F2(l0,lt,t0));
    int p9 = pack8to16(F2(t1,t0,lt), F2(t2,t1,t0));
    int p10 = pack8to16(F2(t3,t2,t1), F2(t4,t3,t2));
    int p11 = pack8to16(F2(t5,t4,t3), F2(t6,t5,t4));
    SRC32(0,7)= pack16to32(p1,p2);
    SRC32(0,6)= pack16to32(p2,p3);
    SRC32(4,7)=SRC32(0,5)= pack16to32(p3,p4);
    SRC32(4,6)=SRC32(0,4)= pack16to32(p4,p5);
    SRC32(4,5)=SRC32(0,3)= pack16to32(p5,p6);
    SRC32(4,4)=SRC32(0,2)= pack16to32(p6,p7);
    SRC32(4,3)=SRC32(0,1)= pack16to32(p7,p8);
    SRC32(4,2)=SRC32(0,0)= pack16to32(p8,p9);
    SRC32(4,1)= pack16to32(p9,p10);
    SRC32(4,0)= pack16to32(p10,p11);
}
static void predict_8x8_vl( uint8_t *src, uint8_t edge[33] )
{
    PREDICT_8x8_LOAD_TOP
    PREDICT_8x8_LOAD_TOPRIGHT
    SRC(0,0)= F1(t0,t1);
    SRC(0,1)= F2(t0,t1,t2);
    SRC(0,2)=SRC(1,0)= F1(t1,t2);
    SRC(0,3)=SRC(1,1)= F2(t1,t2,t3);
    SRC(0,4)=SRC(1,2)=SRC(2,0)= F1(t2,t3);
    SRC(0,5)=SRC(1,3)=SRC(2,1)= F2(t2,t3,t4);
    SRC(0,6)=SRC(1,4)=SRC(2,2)=SRC(3,0)= F1(t3,t4);
    SRC(0,7)=SRC(1,5)=SRC(2,3)=SRC(3,1)= F2(t3,t4,t5);
    SRC(1,6)=SRC(2,4)=SRC(3,2)=SRC(4,0)= F1(t4,t5);
    SRC(1,7)=SRC(2,5)=SRC(3,3)=SRC(4,1)= F2(t4,t5,t6);
    SRC(2,6)=SRC(3,4)=SRC(4,2)=SRC(5,0)= F1(t5,t6);
    SRC(2,7)=SRC(3,5)=SRC(4,3)=SRC(5,1)= F2(t5,t6,t7);
    SRC(3,6)=SRC(4,4)=SRC(5,2)=SRC(6,0)= F1(t6,t7);
    SRC(3,7)=SRC(4,5)=SRC(5,3)=SRC(6,1)= F2(t6,t7,t8);
    SRC(4,6)=SRC(5,4)=SRC(6,2)=SRC(7,0)= F1(t7,t8);
    SRC(4,7)=SRC(5,5)=SRC(6,3)=SRC(7,1)= F2(t7,t8,t9);
    SRC(5,6)=SRC(6,4)=SRC(7,2)= F1(t8,t9);
    SRC(5,7)=SRC(6,5)=SRC(7,3)= F2(t8,t9,t10);
    SRC(6,6)=SRC(7,4)= F1(t9,t10);
    SRC(6,7)=SRC(7,5)= F2(t9,t10,t11);
    SRC(7,6)= F1(t10,t11);
    SRC(7,7)= F2(t10,t11,t12);
}
static void predict_8x8_hu( uint8_t *src, uint8_t edge[33] )
{
    PREDICT_8x8_LOAD_LEFT
    int p1 = pack8to16(F1(l0,l1), F2(l0,l1,l2));
    int p2 = pack8to16(F1(l1,l2), F2(l1,l2,l3));
    int p3 = pack8to16(F1(l2,l3), F2(l2,l3,l4));
    int p4 = pack8to16(F1(l3,l4), F2(l3,l4,l5));
    int p5 = pack8to16(F1(l4,l5), F2(l4,l5,l6));
    int p6 = pack8to16(F1(l5,l6), F2(l5,l6,l7));
    int p7 = pack8to16(F1(l6,l7), F2(l6,l7,l7));
    int p8 = pack8to16(l7,l7);
    SRC32(0,0)= pack16to32(p1,p2);
    SRC32(0,1)= pack16to32(p2,p3);
    SRC32(4,0)=SRC32(0,2)= pack16to32(p3,p4);
    SRC32(4,1)=SRC32(0,3)= pack16to32(p4,p5);
    SRC32(4,2)=SRC32(0,4)= pack16to32(p5,p6);
    SRC32(4,3)=SRC32(0,5)= pack16to32(p6,p7);
    SRC32(4,4)=SRC32(0,6)= pack16to32(p7,p8);
    SRC32(4,5)=SRC32(4,6)= SRC32(0,7) = SRC32(4,7) = pack16to32(p8,p8);
}

/****************************************************************************
 * Exported functions:
 ****************************************************************************/
void x264_predict_16x16_init( int cpu, x264_predict_t pf[7] )
{
    pf[I_PRED_16x16_V ]     = predict_16x16_v;
    pf[I_PRED_16x16_H ]     = predict_16x16_h;
    pf[I_PRED_16x16_DC]     = predict_16x16_dc;
    pf[I_PRED_16x16_P ]     = predict_16x16_p;
    pf[I_PRED_16x16_DC_LEFT]= predict_16x16_dc_left;
    pf[I_PRED_16x16_DC_TOP ]= predict_16x16_dc_top;
    pf[I_PRED_16x16_DC_128 ]= predict_16x16_dc_128;

#ifdef HAVE_MMX
    x264_predict_16x16_init_mmx( cpu, pf );
#endif

#ifdef ARCH_PPC
    if( cpu&X264_CPU_ALTIVEC )
    {
        x264_predict_16x16_init_altivec( pf );
    }
#endif
}

void x264_predict_8x8c_init( int cpu, x264_predict_t pf[7] )
{
    pf[I_PRED_CHROMA_V ]     = predict_8x8c_v;
    pf[I_PRED_CHROMA_H ]     = predict_8x8c_h;
    pf[I_PRED_CHROMA_DC]     = predict_8x8c_dc;
    pf[I_PRED_CHROMA_P ]     = predict_8x8c_p;
    pf[I_PRED_CHROMA_DC_LEFT]= predict_8x8c_dc_left;
    pf[I_PRED_CHROMA_DC_TOP ]= predict_8x8c_dc_top;
    pf[I_PRED_CHROMA_DC_128 ]= predict_8x8c_dc_128;

#ifdef HAVE_MMX
    x264_predict_8x8c_init_mmx( cpu, pf );
#endif

#ifdef ARCH_PPC
    if( cpu&X264_CPU_ALTIVEC )
    {
        x264_predict_8x8c_init_altivec( pf );
    }
#endif
}

void x264_predict_8x8_init( int cpu, x264_predict8x8_t pf[12], x264_predict_8x8_filter_t *predict_8x8_filter )
{
    pf[I_PRED_8x8_V]      = predict_8x8_v;
    pf[I_PRED_8x8_H]      = predict_8x8_h;
    pf[I_PRED_8x8_DC]     = predict_8x8_dc;
    pf[I_PRED_8x8_DDL]    = predict_8x8_ddl;
    pf[I_PRED_8x8_DDR]    = predict_8x8_ddr;
    pf[I_PRED_8x8_VR]     = predict_8x8_vr;
    pf[I_PRED_8x8_HD]     = predict_8x8_hd;
    pf[I_PRED_8x8_VL]     = predict_8x8_vl;
    pf[I_PRED_8x8_HU]     = predict_8x8_hu;
    pf[I_PRED_8x8_DC_LEFT]= predict_8x8_dc_left;
    pf[I_PRED_8x8_DC_TOP] = predict_8x8_dc_top;
    pf[I_PRED_8x8_DC_128] = predict_8x8_dc_128;
    *predict_8x8_filter   = x264_predict_8x8_filter;

#ifdef HAVE_MMX
    x264_predict_8x8_init_mmx( cpu, pf, predict_8x8_filter );
#endif
}

void x264_predict_4x4_init( int cpu, x264_predict_t pf[12] )
{
    pf[I_PRED_4x4_V]      = predict_4x4_v;
    pf[I_PRED_4x4_H]      = predict_4x4_h;
    pf[I_PRED_4x4_DC]     = predict_4x4_dc;
    pf[I_PRED_4x4_DDL]    = predict_4x4_ddl;
    pf[I_PRED_4x4_DDR]    = predict_4x4_ddr;
    pf[I_PRED_4x4_VR]     = predict_4x4_vr;
    pf[I_PRED_4x4_HD]     = predict_4x4_hd;
    pf[I_PRED_4x4_VL]     = predict_4x4_vl;
    pf[I_PRED_4x4_HU]     = predict_4x4_hu;
    pf[I_PRED_4x4_DC_LEFT]= predict_4x4_dc_left;
    pf[I_PRED_4x4_DC_TOP] = predict_4x4_dc_top;
    pf[I_PRED_4x4_DC_128] = predict_4x4_dc_128;

#ifdef HAVE_MMX
    x264_predict_4x4_init_mmx( cpu, pf );
#endif
}


/*****************************************************************************
 * macroblock.h: h264 encoder library
 *****************************************************************************
 * Copyright (C) 2005-2008 x264 project
 *
 * Authors: Loren Merritt <lorenm@u.washington.edu>
 *          Laurent Aimar <fenrir@via.ecp.fr>
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

#ifndef X264_MACROBLOCK_H
#define X264_MACROBLOCK_H

enum macroblock_position_e
{
    MB_LEFT     = 0x01,
    MB_TOP      = 0x02,
    MB_TOPRIGHT = 0x04,
    MB_TOPLEFT  = 0x08,

    MB_PRIVATE  = 0x10,

    ALL_NEIGHBORS = 0xf,
};

static const uint8_t x264_pred_i4x4_neighbors[12] =
{
    MB_TOP,                         // I_PRED_4x4_V
    MB_LEFT,                        // I_PRED_4x4_H
    MB_LEFT | MB_TOP,               // I_PRED_4x4_DC
    MB_TOP  | MB_TOPRIGHT,          // I_PRED_4x4_DDL
    MB_LEFT | MB_TOPLEFT | MB_TOP,  // I_PRED_4x4_DDR
    MB_LEFT | MB_TOPLEFT | MB_TOP,  // I_PRED_4x4_VR
    MB_LEFT | MB_TOPLEFT | MB_TOP,  // I_PRED_4x4_HD
    MB_TOP  | MB_TOPRIGHT,          // I_PRED_4x4_VL
    MB_LEFT,                        // I_PRED_4x4_HU
    MB_LEFT,                        // I_PRED_4x4_DC_LEFT
    MB_TOP,                         // I_PRED_4x4_DC_TOP
    0                               // I_PRED_4x4_DC_128
};


/* XXX mb_type isn't the one written in the bitstream -> only internal usage */
#define IS_INTRA(type) ( (type) == I_4x4 || (type) == I_8x8 || (type) == I_16x16 || (type) == I_PCM )
#define IS_SKIP(type)  ( (type) == P_SKIP || (type) == B_SKIP )
#define IS_DIRECT(type)  ( (type) == B_DIRECT )
enum mb_class_e
{
    I_4x4           = 0,
    I_8x8           = 1,
    I_16x16         = 2,
    I_PCM           = 3,

    P_L0            = 4,
    P_8x8           = 5,
    P_SKIP          = 6,

    B_DIRECT        = 7,
    B_L0_L0         = 8,
    B_L0_L1         = 9,
    B_L0_BI         = 10,
    B_L1_L0         = 11,
    B_L1_L1         = 12,
    B_L1_BI         = 13,
    B_BI_L0         = 14,
    B_BI_L1         = 15,
    B_BI_BI         = 16,
    B_8x8           = 17,
    B_SKIP          = 18,

    X264_MBTYPE_MAX = 19
};
static const uint8_t x264_mb_type_fix[X264_MBTYPE_MAX] =
{
    I_4x4, I_4x4, I_16x16, I_PCM,
    P_L0, P_8x8, P_SKIP,
    B_DIRECT, B_L0_L0, B_L0_L1, B_L0_BI, B_L1_L0, B_L1_L1,
    B_L1_BI, B_BI_L0, B_BI_L1, B_BI_BI, B_8x8, B_SKIP
};
static const uint8_t x264_mb_type_list_table[X264_MBTYPE_MAX][2][2] =
{
    {{0,0},{0,0}}, {{0,0},{0,0}}, {{0,0},{0,0}}, {{0,0},{0,0}}, /* INTRA */
    {{1,1},{0,0}},                                              /* P_L0 */
    {{0,0},{0,0}},                                              /* P_8x8 */
    {{1,1},{0,0}},                                              /* P_SKIP */
    {{0,0},{0,0}},                                              /* B_DIRECT */
    {{1,1},{0,0}}, {{1,0},{0,1}}, {{1,1},{0,1}},                /* B_L0_* */
    {{0,1},{1,0}}, {{0,0},{1,1}}, {{0,1},{1,1}},                /* B_L1_* */
    {{1,1},{1,0}}, {{1,0},{1,1}}, {{1,1},{1,1}},                /* B_BI_* */
    {{0,0},{0,0}},                                              /* B_8x8 */
    {{0,0},{0,0}}                                               /* B_SKIP */
};

#define IS_SUB4x4(type) ( (type ==D_L0_4x4)||(type ==D_L1_4x4)||(type ==D_BI_4x4))
#define IS_SUB4x8(type) ( (type ==D_L0_4x8)||(type ==D_L1_4x8)||(type ==D_BI_4x8))
#define IS_SUB8x4(type) ( (type ==D_L0_8x4)||(type ==D_L1_8x4)||(type ==D_BI_8x4))
#define IS_SUB8x8(type) ( (type ==D_L0_8x8)||(type ==D_L1_8x8)||(type ==D_BI_8x8)||(type ==D_DIRECT_8x8))
enum mb_partition_e
{
    /* sub partition type for P_8x8 and B_8x8 */
    D_L0_4x4          = 0,
    D_L0_8x4          = 1,
    D_L0_4x8          = 2,
    D_L0_8x8          = 3,

    /* sub partition type for B_8x8 only */
    D_L1_4x4          = 4,
    D_L1_8x4          = 5,
    D_L1_4x8          = 6,
    D_L1_8x8          = 7,

    D_BI_4x4          = 8,
    D_BI_8x4          = 9,
    D_BI_4x8          = 10,
    D_BI_8x8          = 11,
    D_DIRECT_8x8      = 12,

    /* partition */
    D_8x8             = 13,
    D_16x8            = 14,
    D_8x16            = 15,
    D_16x16           = 16,
    X264_PARTTYPE_MAX = 17,
};

static const uint8_t x264_mb_partition_listX_table[2][17] =
{{
    1, 1, 1, 1, /* D_L0_* */
    0, 0, 0, 0, /* D_L1_* */
    1, 1, 1, 1, /* D_BI_* */
    0,          /* D_DIRECT_8x8 */
    0, 0, 0, 0  /* 8x8 .. 16x16 */
},
{
    0, 0, 0, 0, /* D_L0_* */
    1, 1, 1, 1, /* D_L1_* */
    1, 1, 1, 1, /* D_BI_* */
    0,          /* D_DIRECT_8x8 */
    0, 0, 0, 0  /* 8x8 .. 16x16 */
}};
static const uint8_t x264_mb_partition_count_table[17] =
{
    /* sub L0 */
    4, 2, 2, 1,
    /* sub L1 */
    4, 2, 2, 1,
    /* sub BI */
    4, 2, 2, 1,
    /* Direct */
    1,
    /* Partition */
    4, 2, 2, 1
};
static const uint8_t x264_mb_partition_pixel_table[17] =
{
    6, 4, 5, 3, 6, 4, 5, 3, 6, 4, 5, 3, 3, 3, 1, 2, 0
};

/* zigzags are transposed with respect to the tables in the standard */
static const uint8_t x264_zigzag_scan4[2][16] =
{{ // frame
    0,  4,  1,  2,  5,  8, 12,  9,  6,  3,  7, 10, 13, 14, 11, 15
},
{  // field
    0,  1,  4,  2,  3,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15
}};
static const uint8_t x264_zigzag_scan8[2][64] =
{{
    0,  8,  1,  2,  9, 16, 24, 17, 10,  3,  4, 11, 18, 25, 32, 40,
   33, 26, 19, 12,  5,  6, 13, 20, 27, 34, 41, 48, 56, 49, 42, 35,
   28, 21, 14,  7, 15, 22, 29, 36, 43, 50, 57, 58, 51, 44, 37, 30,
   23, 31, 38, 45, 52, 59, 60, 53, 46, 39, 47, 54, 61, 62, 55, 63
},
{
    0,  1,  2,  8,  9,  3,  4, 10, 16, 11,  5,  6,  7, 12, 17, 24,
   18, 13, 14, 15, 19, 25, 32, 26, 20, 21, 22, 23, 27, 33, 40, 34,
   28, 29, 30, 31, 35, 41, 48, 42, 36, 37, 38, 39, 43, 49, 50, 44,
   45, 46, 47, 51, 56, 57, 52, 53, 54, 55, 58, 59, 60, 61, 62, 63
}};

static const uint8_t block_idx_x[16] =
{
    0, 1, 0, 1, 2, 3, 2, 3, 0, 1, 0, 1, 2, 3, 2, 3
};
static const uint8_t block_idx_y[16] =
{
    0, 0, 1, 1, 0, 0, 1, 1, 2, 2, 3, 3, 2, 2, 3, 3
};
static const uint8_t block_idx_xy[4][4] =
{
    { 0, 2, 8,  10 },
    { 1, 3, 9,  11 },
    { 4, 6, 12, 14 },
    { 5, 7, 13, 15 }
};
static const uint8_t block_idx_xy_1d[16] =
{
    0, 1, 4, 5, 2, 3, 6, 7, 8, 9, 12, 13, 10, 11, 14, 15
};
static const uint8_t block_idx_yx_1d[16] =
{
    0, 4, 1, 5, 8, 12, 9, 13, 2, 6, 3, 7, 10, 14, 11, 15
};
static const uint8_t block_idx_xy_fenc[16] =
{
    0*4 + 0*4*FENC_STRIDE, 1*4 + 0*4*FENC_STRIDE,
    0*4 + 1*4*FENC_STRIDE, 1*4 + 1*4*FENC_STRIDE,
    2*4 + 0*4*FENC_STRIDE, 3*4 + 0*4*FENC_STRIDE,
    2*4 + 1*4*FENC_STRIDE, 3*4 + 1*4*FENC_STRIDE,
    0*4 + 2*4*FENC_STRIDE, 1*4 + 2*4*FENC_STRIDE,
    0*4 + 3*4*FENC_STRIDE, 1*4 + 3*4*FENC_STRIDE,
    2*4 + 2*4*FENC_STRIDE, 3*4 + 2*4*FENC_STRIDE,
    2*4 + 3*4*FENC_STRIDE, 3*4 + 3*4*FENC_STRIDE
};
static const uint16_t block_idx_xy_fdec[16] =
{
    0*4 + 0*4*FDEC_STRIDE, 1*4 + 0*4*FDEC_STRIDE,
    0*4 + 1*4*FDEC_STRIDE, 1*4 + 1*4*FDEC_STRIDE,
    2*4 + 0*4*FDEC_STRIDE, 3*4 + 0*4*FDEC_STRIDE,
    2*4 + 1*4*FDEC_STRIDE, 3*4 + 1*4*FDEC_STRIDE,
    0*4 + 2*4*FDEC_STRIDE, 1*4 + 2*4*FDEC_STRIDE,
    0*4 + 3*4*FDEC_STRIDE, 1*4 + 3*4*FDEC_STRIDE,
    2*4 + 2*4*FDEC_STRIDE, 3*4 + 2*4*FDEC_STRIDE,
    2*4 + 3*4*FDEC_STRIDE, 3*4 + 3*4*FDEC_STRIDE
};

static const uint8_t i_chroma_qp_table[52+12*2] =
{
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  1,  2,  3,  4,  5,  6,  7,  8,  9,
    10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
    20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
    29, 30, 31, 32, 32, 33, 34, 34, 35, 35,
    36, 36, 37, 37, 37, 38, 38, 38, 39, 39,
    39, 39,
    39, 39, 39, 39, 39, 39, 39, 39, 39, 39, 39, 39,
};

enum cabac_ctx_block_cat_e
{
    DCT_LUMA_DC   = 0,
    DCT_LUMA_AC   = 1,
    DCT_LUMA_4x4  = 2,
    DCT_CHROMA_DC = 3,
    DCT_CHROMA_AC = 4,
    DCT_LUMA_8x8  = 5,
};


int  x264_macroblock_cache_init( x264_t *h );
void x264_macroblock_slice_init( x264_t *h );
void x264_macroblock_cache_load( x264_t *h, int i_mb_x, int i_mb_y );
void x264_macroblock_cache_save( x264_t *h );
void x264_macroblock_cache_end( x264_t *h );

void x264_macroblock_bipred_init( x264_t *h );

void x264_prefetch_fenc( x264_t *h, x264_frame_t *fenc, int i_mb_x, int i_mb_y );

/* x264_mb_predict_mv_16x16:
 *      set mvp with predicted mv for D_16x16 block
 *      h->mb. need only valid values from other blocks */
void x264_mb_predict_mv_16x16( x264_t *h, int i_list, int i_ref, int16_t mvp[2] );
/* x264_mb_predict_mv_pskip:
 *      set mvp with predicted mv for P_SKIP
 *      h->mb. need only valid values from other blocks */
void x264_mb_predict_mv_pskip( x264_t *h, int16_t mv[2] );
/* x264_mb_predict_mv:
 *      set mvp with predicted mv for all blocks except SKIP and DIRECT
 *      h->mb. need valid ref/partition/sub of current block to be valid
 *      and valid mv/ref from other blocks. */
void x264_mb_predict_mv( x264_t *h, int i_list, int idx, int i_width, int16_t mvp[2] );
/* x264_mb_predict_mv_direct16x16:
 *      set h->mb.cache.mv and h->mb.cache.ref for B_SKIP or B_DIRECT
 *      h->mb. need only valid values from other blocks.
 *      return 1 on success, 0 on failure.
 *      if b_changed != NULL, set it to whether refs or mvs differ from
 *      before this functioncall. */
int x264_mb_predict_mv_direct16x16( x264_t *h, int *b_changed );
/* x264_mb_load_mv_direct8x8:
 *      set h->mb.cache.mv and h->mb.cache.ref for B_DIRECT
 *      must be called only after x264_mb_predict_mv_direct16x16 */
void x264_mb_load_mv_direct8x8( x264_t *h, int idx );
/* x264_mb_predict_mv_ref16x16:
 *      set mvc with D_16x16 prediction.
 *      uses all neighbors, even those that didn't end up using this ref.
 *      h->mb. need only valid values from other blocks */
void x264_mb_predict_mv_ref16x16( x264_t *h, int i_list, int i_ref, int16_t mvc[8][2], int *i_mvc );

void x264_mb_mc( x264_t *h );
void x264_mb_mc_8x8( x264_t *h, int i8 );

static ALWAYS_INLINE uint32_t pack16to32( int a, int b )
{
#ifdef WORDS_BIGENDIAN
   return b + (a<<16);
#else
   return a + (b<<16);
#endif
}
static ALWAYS_INLINE uint32_t pack8to16( int a, int b )
{
#ifdef WORDS_BIGENDIAN
   return b + (a<<8);
#else
   return a + (b<<8);
#endif
}
static ALWAYS_INLINE uint32_t pack8to32( int a, int b, int c, int d )
{
#ifdef WORDS_BIGENDIAN
   return d + (c<<8) + (b<<16) + (a<<24);
#else
   return a + (b<<8) + (c<<16) + (d<<24);
#endif
}
static ALWAYS_INLINE uint32_t pack16to32_mask( int a, int b )
{
#ifdef WORDS_BIGENDIAN
   return (b&0xFFFF) + (a<<16);
#else
   return (a&0xFFFF) + (b<<16);
#endif
}
static ALWAYS_INLINE void x264_macroblock_cache_rect1( void *dst, int width, int height, uint8_t val )
{
    if( width == 4 )
    {
        uint32_t val2 = val * 0x01010101;
                          ((uint32_t*)dst)[0] = val2;
        if( height >= 2 ) ((uint32_t*)dst)[2] = val2;
        if( height == 4 ) ((uint32_t*)dst)[4] = val2;
        if( height == 4 ) ((uint32_t*)dst)[6] = val2;
    }
    else // 2
    {
        uint32_t val2 = val * 0x0101;
                          ((uint16_t*)dst)[ 0] = val2;
        if( height >= 2 ) ((uint16_t*)dst)[ 4] = val2;
        if( height == 4 ) ((uint16_t*)dst)[ 8] = val2;
        if( height == 4 ) ((uint16_t*)dst)[12] = val2;
    }
}
static ALWAYS_INLINE void x264_macroblock_cache_rect4( void *dst, int width, int height, uint32_t val )
{
    int dy;
    if( width == 1 || WORD_SIZE < 8 )
    {
        for( dy = 0; dy < height; dy++ )
        {
                             ((uint32_t*)dst)[8*dy+0] = val;
            if( width >= 2 ) ((uint32_t*)dst)[8*dy+1] = val;
            if( width == 4 ) ((uint32_t*)dst)[8*dy+2] = val;
            if( width == 4 ) ((uint32_t*)dst)[8*dy+3] = val;
        }
    }
    else
    {
        uint64_t val64 = val + ((uint64_t)val<<32);
        for( dy = 0; dy < height; dy++ )
        {
                             ((uint64_t*)dst)[4*dy+0] = val64;
            if( width == 4 ) ((uint64_t*)dst)[4*dy+1] = val64;
        }
    }
}
#define x264_macroblock_cache_mv_ptr(a,x,y,w,h,l,mv) x264_macroblock_cache_mv(a,x,y,w,h,l,*(uint32_t*)mv)
static ALWAYS_INLINE void x264_macroblock_cache_mv( x264_t *h, int x, int y, int width, int height, int i_list, uint32_t mv )
{
    x264_macroblock_cache_rect4( &h->mb.cache.mv[i_list][X264_SCAN8_0+x+8*y], width, height, mv );
}
static ALWAYS_INLINE void x264_macroblock_cache_mvd( x264_t *h, int x, int y, int width, int height, int i_list, uint32_t mv )
{
    x264_macroblock_cache_rect4( &h->mb.cache.mvd[i_list][X264_SCAN8_0+x+8*y], width, height, mv );
}
static ALWAYS_INLINE void x264_macroblock_cache_ref( x264_t *h, int x, int y, int width, int height, int i_list, uint8_t ref )
{
    x264_macroblock_cache_rect1( &h->mb.cache.ref[i_list][X264_SCAN8_0+x+8*y], width, height, ref );
}
static ALWAYS_INLINE void x264_macroblock_cache_skip( x264_t *h, int x, int y, int width, int height, int b_skip )
{
    x264_macroblock_cache_rect1( &h->mb.cache.skip[X264_SCAN8_0+x+8*y], width, height, b_skip );
}
static ALWAYS_INLINE void x264_macroblock_cache_intra8x8_pred( x264_t *h, int x, int y, int i_mode )
{
    int8_t *cache = &h->mb.cache.intra4x4_pred_mode[X264_SCAN8_0+x+8*y];
    cache[0] = cache[1] = cache[8] = cache[9] = i_mode;
}
#define array_non_zero(a) array_non_zero_int(a, sizeof(a))
#define array_non_zero_int array_non_zero_int_c
static ALWAYS_INLINE int array_non_zero_int_c( void *v, int i_count )
{
    union {uint16_t s[4]; uint64_t l;} *x = v;
    if(i_count == 8)
        return !!x[0].l;
    else if(i_count == 16)
        return !!(x[0].l|x[1].l);
    else if(i_count == 32)
        return !!(x[0].l|x[1].l|x[2].l|x[3].l);
    else
    {
        int i;
        i_count /= sizeof(uint64_t);
        for( i = 0; i < i_count; i++ )
            if( x[i].l ) return 1;
        return 0;
    }
}
static inline int x264_mb_predict_intra4x4_mode( x264_t *h, int idx )
{
    const int ma = h->mb.cache.intra4x4_pred_mode[x264_scan8[idx] - 1];
    const int mb = h->mb.cache.intra4x4_pred_mode[x264_scan8[idx] - 8];
    const int m  = X264_MIN( x264_mb_pred_mode4x4_fix(ma),
                             x264_mb_pred_mode4x4_fix(mb) );

    if( m < 0 )
        return I_PRED_4x4_DC;

    return m;
}
static inline int x264_mb_predict_non_zero_code( x264_t *h, int idx )
{
    const int za = h->mb.cache.non_zero_count[x264_scan8[idx] - 1];
    const int zb = h->mb.cache.non_zero_count[x264_scan8[idx] - 8];

    int i_ret = za + zb;

    if( i_ret < 0x80 )
    {
        i_ret = ( i_ret + 1 ) >> 1;
    }
    return i_ret & 0x7f;
}
/* x264_mb_transform_8x8_allowed:
 *      check whether any partition is smaller than 8x8 (or at least
 *      might be, according to just partition type.)
 *      doesn't check for cbp */
static inline int x264_mb_transform_8x8_allowed( x264_t *h )
{
    // intra and skip are disallowed
    // large partitions are allowed
    // direct and 8x8 are conditional
    static const uint8_t partition_tab[X264_MBTYPE_MAX] = {
        0,0,0,0,1,2,0,1,1,1,1,1,1,1,1,1,1,1,0,
    };

    if( !h->pps->b_transform_8x8_mode )
        return 0;
    if( h->mb.i_type != P_8x8 )
        return partition_tab[h->mb.i_type];
    return *(uint32_t*)h->mb.i_sub_partition == D_L0_8x8*0x01010101;
}

#endif


/*****************************************************************************
 * macroblock.c: h264 encoder library
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

#include "common.h"
#include "encoder/me.h"

void x264_mb_predict_mv( x264_t *h, int i_list, int idx, int i_width, int16_t mvp[2] )
{
    const int i8 = x264_scan8[idx];
    const int i_ref= h->mb.cache.ref[i_list][i8];
    int     i_refa = h->mb.cache.ref[i_list][i8 - 1];
    int16_t *mv_a  = h->mb.cache.mv[i_list][i8 - 1];
    int     i_refb = h->mb.cache.ref[i_list][i8 - 8];
    int16_t *mv_b  = h->mb.cache.mv[i_list][i8 - 8];
    int     i_refc = h->mb.cache.ref[i_list][i8 - 8 + i_width];
    int16_t *mv_c  = h->mb.cache.mv[i_list][i8 - 8 + i_width];

    if( (idx&3) >= 2 + (i_width&1) || i_refc == -2 )
    {
        i_refc = h->mb.cache.ref[i_list][i8 - 8 - 1];
        mv_c   = h->mb.cache.mv[i_list][i8 - 8 - 1];
    }

    if( h->mb.i_partition == D_16x8 )
    {
        if( idx == 0 )
        {
            if( i_refb == i_ref )
            {
                CP32( mvp, mv_b );
                return;
            }
        }
        else
        {
            if( i_refa == i_ref )
            {
                CP32( mvp, mv_a );
                return;
            }
        }
    }
    else if( h->mb.i_partition == D_8x16 )
    {
        if( idx == 0 )
        {
            if( i_refa == i_ref )
            {
                CP32( mvp, mv_a );
                return;
            }
        }
        else
        {
            if( i_refc == i_ref )
            {
                CP32( mvp, mv_c );
                return;
            }
        }
    }

    int i_count = (i_refa == i_ref) + (i_refb == i_ref) + (i_refc == i_ref);

    if( i_count > 1 )
    {
median:
        x264_median_mv( mvp, mv_a, mv_b, mv_c );
    }
    else if( i_count == 1 )
    {
        if( i_refa == i_ref )
            CP32( mvp, mv_a );
        else if( i_refb == i_ref )
            CP32( mvp, mv_b );
        else
            CP32( mvp, mv_c );
    }
    else if( i_refb == -2 && i_refc == -2 && i_refa != -2 )
        CP32( mvp, mv_a );
    else
        goto median;
}

void x264_mb_predict_mv_16x16( x264_t *h, int i_list, int i_ref, int16_t mvp[2] )
{
    int     i_refa = h->mb.cache.ref[i_list][X264_SCAN8_0 - 1];
    int16_t *mv_a  = h->mb.cache.mv[i_list][X264_SCAN8_0 - 1];
    int     i_refb = h->mb.cache.ref[i_list][X264_SCAN8_0 - 8];
    int16_t *mv_b  = h->mb.cache.mv[i_list][X264_SCAN8_0 - 8];
    int     i_refc = h->mb.cache.ref[i_list][X264_SCAN8_0 - 8 + 4];
    int16_t *mv_c  = h->mb.cache.mv[i_list][X264_SCAN8_0 - 8 + 4];
    if( i_refc == -2 )
    {
        i_refc = h->mb.cache.ref[i_list][X264_SCAN8_0 - 8 - 1];
        mv_c   = h->mb.cache.mv[i_list][X264_SCAN8_0 - 8 - 1];
    }

    int i_count = (i_refa == i_ref) + (i_refb == i_ref) + (i_refc == i_ref);

    if( i_count > 1 )
    {
median:
        x264_median_mv( mvp, mv_a, mv_b, mv_c );
    }
    else if( i_count == 1 )
    {
        if( i_refa == i_ref )
            CP32( mvp, mv_a );
        else if( i_refb == i_ref )
            CP32( mvp, mv_b );
        else
            CP32( mvp, mv_c );
    }
    else if( i_refb == -2 && i_refc == -2 && i_refa != -2 )
        CP32( mvp, mv_a );
    else
        goto median;
}


void x264_mb_predict_mv_pskip( x264_t *h, int16_t mv[2] )
{
    int     i_refa = h->mb.cache.ref[0][X264_SCAN8_0 - 1];
    int     i_refb = h->mb.cache.ref[0][X264_SCAN8_0 - 8];
    int16_t *mv_a  = h->mb.cache.mv[0][X264_SCAN8_0 - 1];
    int16_t *mv_b  = h->mb.cache.mv[0][X264_SCAN8_0 - 8];

    if( i_refa == -2 || i_refb == -2 ||
        !( i_refa | M32( mv_a ) ) ||
        !( i_refb | M32( mv_b ) ) )
    {
        M32( mv ) = 0;
    }
    else
        x264_mb_predict_mv_16x16( h, 0, 0, mv );
}

static int x264_mb_predict_mv_direct16x16_temporal( x264_t *h )
{
    int i_mb_4x4 = 16 * h->mb.i_mb_stride * h->mb.i_mb_y + 4 * h->mb.i_mb_x;
    int i_mb_8x8 =  4 * h->mb.i_mb_stride * h->mb.i_mb_y + 2 * h->mb.i_mb_x;
    const int type_col = h->fref1[0]->mb_type[h->mb.i_mb_xy];
    const int partition_col = h->fref1[0]->mb_partition[h->mb.i_mb_xy];

    x264_macroblock_cache_ref( h, 0, 0, 4, 4, 1, 0 );

    h->mb.i_partition = partition_col;

    if( IS_INTRA( type_col ) )
    {
        x264_macroblock_cache_ref( h, 0, 0, 4, 4, 0, 0 );
        x264_macroblock_cache_mv(  h, 0, 0, 4, 4, 0, 0 );
        x264_macroblock_cache_mv(  h, 0, 0, 4, 4, 1, 0 );
        return 1;
    }

    /* Don't do any checks other than the ones we have to, based
     * on the size of the colocated partitions.
     * Depends on the enum order: D_8x8, D_16x8, D_8x16, D_16x16 */
    int max_i8 = (D_16x16 - partition_col) + 1;
    int step = (partition_col == D_16x8) + 1;
    int width = 4 >> ((D_16x16 - partition_col)&1);
    int height = 4 >> ((D_16x16 - partition_col)>>1);

    for( int i8 = 0; i8 < max_i8; i8 += step )
    {
        int x8 = i8&1;
        int y8 = i8>>1;
        int i_part_8x8 = i_mb_8x8 + x8 + y8 * h->mb.i_b8_stride;
        int i_ref1_ref = h->fref1[0]->ref[0][i_part_8x8];
        int i_ref = (map_col_to_list0(i_ref1_ref>>h->sh.b_mbaff) << h->sh.b_mbaff) + (i_ref1_ref&h->sh.b_mbaff);

        if( i_ref >= 0 )
        {
            int dist_scale_factor = h->mb.dist_scale_factor[i_ref][0];
            int16_t *mv_col = h->fref1[0]->mv[0][i_mb_4x4 + 3*x8 + 3*y8 * h->mb.i_b4_stride];
            int l0x = ( dist_scale_factor * mv_col[0] + 128 ) >> 8;
            int l0y = ( dist_scale_factor * mv_col[1] + 128 ) >> 8;
            if( h->param.i_threads > 1 && (l0y > h->mb.mv_max_spel[1] || l0y-mv_col[1] > h->mb.mv_max_spel[1]) )
                return 0;
            x264_macroblock_cache_ref( h, 2*x8, 2*y8, width, height, 0, i_ref );
            x264_macroblock_cache_mv( h, 2*x8, 2*y8, width, height, 0, pack16to32_mask(l0x, l0y) );
            x264_macroblock_cache_mv( h, 2*x8, 2*y8, width, height, 1, pack16to32_mask(l0x-mv_col[0], l0y-mv_col[1]) );
        }
        else
        {
            /* the collocated ref isn't in the current list0 */
            /* FIXME: we might still be able to use direct_8x8 on some partitions */
            /* FIXME: with B-pyramid + extensive ref list reordering
             *   (not currently used), we would also have to check
             *   l1mv1 like in spatial mode */
            return 0;
        }
    }

    return 1;
}

static int x264_mb_predict_mv_direct16x16_spatial( x264_t *h )
{
    int8_t ref[2];
    ALIGNED_ARRAY_8( int16_t, mv,[2],[2] );
    const int8_t *l1ref0 = &h->fref1[0]->ref[0][h->mb.i_b8_xy];
    const int8_t *l1ref1 = &h->fref1[0]->ref[1][h->mb.i_b8_xy];
    const int16_t (*l1mv[2])[2] = { (const int16_t (*)[2]) &h->fref1[0]->mv[0][h->mb.i_b4_xy],
                                    (const int16_t (*)[2]) &h->fref1[0]->mv[1][h->mb.i_b4_xy] };
    const int type_col = h->fref1[0]->mb_type[h->mb.i_mb_xy];
    const int partition_col = h->fref1[0]->mb_partition[h->mb.i_mb_xy];

    h->mb.i_partition = partition_col;

    for( int i_list = 0; i_list < 2; i_list++ )
    {
        int     i_refa = h->mb.cache.ref[i_list][X264_SCAN8_0 - 1];
        int16_t *mv_a  = h->mb.cache.mv[i_list][X264_SCAN8_0 - 1];
        int     i_refb = h->mb.cache.ref[i_list][X264_SCAN8_0 - 8];
        int16_t *mv_b  = h->mb.cache.mv[i_list][X264_SCAN8_0 - 8];
        int     i_refc = h->mb.cache.ref[i_list][X264_SCAN8_0 - 8 + 4];
        int16_t *mv_c  = h->mb.cache.mv[i_list][X264_SCAN8_0 - 8 + 4];
        if( i_refc == -2 )
        {
            i_refc = h->mb.cache.ref[i_list][X264_SCAN8_0 - 8 - 1];
            mv_c   = h->mb.cache.mv[i_list][X264_SCAN8_0 - 8 - 1];
        }

        int i_ref = X264_MIN3( (unsigned)i_refa, (unsigned)i_refb, (unsigned)i_refc );
        if( i_ref < 0 )
        {
            i_ref = -1;
            M32( mv[i_list] ) = 0;
        }
        else
        {
            /* Same as x264_mb_predict_mv_16x16, but simplified to eliminate cases
             * not relevant to spatial direct. */
            int i_count = (i_refa == i_ref) + (i_refb == i_ref) + (i_refc == i_ref);

            if( i_count > 1 )
                x264_median_mv( mv[i_list], mv_a, mv_b, mv_c );
            else
            {
                if( i_refa == i_ref )
                    CP32( mv[i_list], mv_a );
                else if( i_refb == i_ref )
                    CP32( mv[i_list], mv_b );
                else
                    CP32( mv[i_list], mv_c );
            }
        }

        x264_macroblock_cache_ref( h, 0, 0, 4, 4, i_list, i_ref );
        x264_macroblock_cache_mv_ptr( h, 0, 0, 4, 4, i_list, mv[i_list] );
        ref[i_list] = i_ref;
    }

    if( (M16( ref ) & 0x8080) == 0x8080 ) /* if( ref[0] < 0 && ref[1] < 0 ) */
    {
        x264_macroblock_cache_ref( h, 0, 0, 4, 4, 0, 0 );
        x264_macroblock_cache_ref( h, 0, 0, 4, 4, 1, 0 );
        return 1;
    }

    if( h->param.i_threads > 1
        && ( mv[0][1] > h->mb.mv_max_spel[1]
          || mv[1][1] > h->mb.mv_max_spel[1] ) )
    {
#if 0
        fprintf(stderr, "direct_spatial: (%d,%d) (%d,%d) > %d \n",
                mv[0][0], mv[0][1], mv[1][0], mv[1][1],
                h->mb.mv_max_spel[1]);
#endif
        return 0;
    }

    if( !M64( mv ) || IS_INTRA( type_col ) || (ref[0]&&ref[1]) )
        return 1;

    /* Don't do any checks other than the ones we have to, based
     * on the size of the colocated partitions.
     * Depends on the enum order: D_8x8, D_16x8, D_8x16, D_16x16 */
    int max_i8 = (D_16x16 - partition_col) + 1;
    int step = (partition_col == D_16x8) + 1;
    int width = 4 >> ((D_16x16 - partition_col)&1);
    int height = 4 >> ((D_16x16 - partition_col)>>1);

    /* col_zero_flag */
    for( int i8 = 0; i8 < max_i8; i8 += step )
    {
        const int x8 = i8&1;
        const int y8 = i8>>1;
        const int o8 = x8 + y8 * h->mb.i_b8_stride;
        const int o4 = 3*(x8 + y8 * h->mb.i_b4_stride);
        int idx;
        if( l1ref0[o8] == 0 )
            idx = 0;
        else if( l1ref0[o8] < 0 && l1ref1[o8] == 0 )
            idx = 1;
        else
            continue;

        if( abs( l1mv[idx][o4][0] ) <= 1 && abs( l1mv[idx][o4][1] ) <= 1 )
        {
            if( ref[0] == 0 ) x264_macroblock_cache_mv( h, 2*x8, 2*y8, width, height, 0, 0 );
            if( ref[1] == 0 ) x264_macroblock_cache_mv( h, 2*x8, 2*y8, width, height, 1, 0 );
        }
    }

    return 1;
}

int x264_mb_predict_mv_direct16x16( x264_t *h, int *b_changed )
{
    int b_available;
    if( h->param.analyse.i_direct_mv_pred == X264_DIRECT_PRED_NONE )
        return 0;
    else if( h->sh.b_direct_spatial_mv_pred )
        b_available = x264_mb_predict_mv_direct16x16_spatial( h );
    else
        b_available = x264_mb_predict_mv_direct16x16_temporal( h );

    if( b_changed != NULL && b_available )
    {
        int changed;

        changed  = M32( h->mb.cache.direct_mv[0][0] ) ^ M32( h->mb.cache.mv[0][x264_scan8[0]] );
        changed |= M32( h->mb.cache.direct_mv[1][0] ) ^ M32( h->mb.cache.mv[1][x264_scan8[0]] );
        changed |= h->mb.cache.direct_ref[0][0] ^ h->mb.cache.ref[0][x264_scan8[0]];
        changed |= h->mb.cache.direct_ref[1][0] ^ h->mb.cache.ref[1][x264_scan8[0]];
        if( !changed && h->mb.i_partition != D_16x16 )
        {
            changed |= M32( h->mb.cache.direct_mv[0][3] ) ^ M32( h->mb.cache.mv[0][x264_scan8[12]] );
            changed |= M32( h->mb.cache.direct_mv[1][3] ) ^ M32( h->mb.cache.mv[1][x264_scan8[12]] );
            changed |= h->mb.cache.direct_ref[0][3] ^ h->mb.cache.ref[0][x264_scan8[12]];
            changed |= h->mb.cache.direct_ref[1][3] ^ h->mb.cache.ref[1][x264_scan8[12]];
        }
        if( !changed && h->mb.i_partition == D_8x8 )
        {
            changed |= M32( h->mb.cache.direct_mv[0][1] ) ^ M32( h->mb.cache.mv[0][x264_scan8[4]] );
            changed |= M32( h->mb.cache.direct_mv[1][1] ) ^ M32( h->mb.cache.mv[1][x264_scan8[4]] );
            changed |= M32( h->mb.cache.direct_mv[0][2] ) ^ M32( h->mb.cache.mv[0][x264_scan8[8]] );
            changed |= M32( h->mb.cache.direct_mv[1][2] ) ^ M32( h->mb.cache.mv[1][x264_scan8[8]] );
            changed |= h->mb.cache.direct_ref[0][1] ^ h->mb.cache.ref[0][x264_scan8[4]];
            changed |= h->mb.cache.direct_ref[1][1] ^ h->mb.cache.ref[1][x264_scan8[4]];
            changed |= h->mb.cache.direct_ref[0][2] ^ h->mb.cache.ref[0][x264_scan8[8]];
            changed |= h->mb.cache.direct_ref[1][2] ^ h->mb.cache.ref[1][x264_scan8[8]];
        }
        *b_changed = changed;
        if( !changed )
            return b_available;
    }

    /* cache ref & mv */
    if( b_available )
        for( int l = 0; l < 2; l++ )
        {
            CP32( h->mb.cache.direct_mv[l][0], h->mb.cache.mv[l][x264_scan8[ 0]] );
            CP32( h->mb.cache.direct_mv[l][1], h->mb.cache.mv[l][x264_scan8[ 4]] );
            CP32( h->mb.cache.direct_mv[l][2], h->mb.cache.mv[l][x264_scan8[ 8]] );
            CP32( h->mb.cache.direct_mv[l][3], h->mb.cache.mv[l][x264_scan8[12]] );
            h->mb.cache.direct_ref[l][0] = h->mb.cache.ref[l][x264_scan8[ 0]];
            h->mb.cache.direct_ref[l][1] = h->mb.cache.ref[l][x264_scan8[ 4]];
            h->mb.cache.direct_ref[l][2] = h->mb.cache.ref[l][x264_scan8[ 8]];
            h->mb.cache.direct_ref[l][3] = h->mb.cache.ref[l][x264_scan8[12]];
            h->mb.cache.direct_partition = h->mb.i_partition;
        }

    return b_available;
}

/* This just improves encoder performance, it's not part of the spec */
void x264_mb_predict_mv_ref16x16( x264_t *h, int i_list, int i_ref, int16_t mvc[9][2], int *i_mvc )
{
    int16_t (*mvr)[2] = h->mb.mvr[i_list][i_ref];
    int i = 0;

#define SET_MVP(mvp)\
    { \
        CP32( mvc[i], mvp ); \
        i++; \
    }

    /* b_direct */
    if( h->sh.i_type == SLICE_TYPE_B
        && h->mb.cache.ref[i_list][x264_scan8[12]] == i_ref )
    {
        SET_MVP( h->mb.cache.mv[i_list][x264_scan8[12]] );
    }

    if( i_ref == 0 && h->frames.b_have_lowres )
    {
        int16_t (*lowres_mv)[2] = i_list ? h->fenc->lowres_mvs[1][h->fref1[0]->i_frame-h->fenc->i_frame-1]
                                         : h->fenc->lowres_mvs[0][h->fenc->i_frame-h->fref0[0]->i_frame-1];
        if( lowres_mv[0][0] != 0x7fff )
        {
            M32( mvc[i] ) = (M32( lowres_mv[h->mb.i_mb_xy] )*2)&0xfffeffff;
            i++;
        }
    }

    /* spatial predictors */
    if( h->mb.i_neighbour & MB_LEFT )
    {
        int i_mb_l = h->mb.i_mb_xy - 1;
        SET_MVP( mvr[i_mb_l] );
    }
    if( h->mb.i_neighbour & MB_TOP )
    {
        int i_mb_t = h->mb.i_mb_top_xy;
        SET_MVP( mvr[i_mb_t] );

        if( h->mb.i_neighbour & MB_TOPLEFT )
            SET_MVP( mvr[i_mb_t-1] );
        if( h->mb.i_mb_x < h->mb.i_mb_stride - 1 )
            SET_MVP( mvr[i_mb_t+1] );
    }
#undef SET_MVP

    /* temporal predictors */
    if( h->fref0[0]->i_ref[0] > 0 )
    {
        x264_frame_t *l0 = h->fref0[0];
        x264_frame_t **fref = i_list ? h->fref1 : h->fref0;
        int field = h->mb.i_mb_y&1;
        int curpoc = h->fdec->i_poc + field*h->sh.i_delta_poc_bottom;
        int refpoc = fref[i_ref>>h->sh.b_mbaff]->i_poc;
        if( h->sh.b_mbaff && field^(i_ref&1) )
            refpoc += h->sh.i_delta_poc_bottom;

#define SET_TMVP(dx, dy) { \
            int i_b4 = h->mb.i_b4_xy + dx*4 + dy*4*h->mb.i_b4_stride; \
            int i_b8 = h->mb.i_b8_xy + dx*2 + dy*2*h->mb.i_b8_stride; \
            int ref_col = l0->ref[0][i_b8]; \
            if( ref_col >= 0 ) \
            { \
                int scale = (curpoc - refpoc) * l0->inv_ref_poc[h->mb.b_interlaced&field][ref_col];\
                mvc[i][0] = (l0->mv[0][i_b4][0]*scale + 128) >> 8;\
                mvc[i][1] = (l0->mv[0][i_b4][1]*scale + 128) >> 8;\
                i++; \
            } \
        }

        SET_TMVP(0,0);
        if( h->mb.i_mb_x < h->sps->i_mb_width-1 )
            SET_TMVP(1,0);
        if( h->mb.i_mb_y < h->sps->i_mb_height-1 )
            SET_TMVP(0,1);
#undef SET_TMVP
    }

    *i_mvc = i;
}

/* Set up a lookup table for delta pocs to reduce an IDIV to an IMUL */
static void setup_inverse_delta_pocs( x264_t *h )
{
    for( int field = 0; field <= h->sh.b_mbaff; field++ )
    {
        int curpoc = h->fdec->i_poc + field*h->sh.i_delta_poc_bottom;
        for( int i = 0; i < (h->i_ref0<<h->sh.b_mbaff); i++ )
        {
            int refpoc = h->fref0[i>>h->sh.b_mbaff]->i_poc;
            if( h->sh.b_mbaff && field^(i&1) )
                refpoc += h->sh.i_delta_poc_bottom;
            int delta = curpoc - refpoc;

            h->fdec->inv_ref_poc[field][i] = (256 + delta/2) / delta;
        }
    }
}

static NOINLINE void x264_mb_mc_0xywh( x264_t *h, int x, int y, int width, int height )
{
    int i8    = x264_scan8[0]+x+8*y;
    int i_ref = h->mb.cache.ref[0][i8];
    int mvx   = x264_clip3( h->mb.cache.mv[0][i8][0], h->mb.mv_min[0], h->mb.mv_max[0] ) + 4*4*x;
    int mvy   = x264_clip3( h->mb.cache.mv[0][i8][1], h->mb.mv_min[1], h->mb.mv_max[1] ) + 4*4*y;

    h->mc.mc_luma( &h->mb.pic.p_fdec[0][4*y*FDEC_STRIDE+4*x], FDEC_STRIDE,
                   h->mb.pic.p_fref[0][i_ref], h->mb.pic.i_stride[0],
                   mvx, mvy, 4*width, 4*height, &h->sh.weight[i_ref][0] );

    // chroma is offset if MCing from a field of opposite parity
    if( h->mb.b_interlaced & i_ref )
        mvy += (h->mb.i_mb_y & 1)*4 - 2;

    h->mc.mc_chroma( &h->mb.pic.p_fdec[1][2*y*FDEC_STRIDE+2*x], FDEC_STRIDE,
                     h->mb.pic.p_fref[0][i_ref][4], h->mb.pic.i_stride[1],
                     mvx, mvy, 2*width, 2*height );

    if( h->sh.weight[i_ref][1].weightfn )
        h->sh.weight[i_ref][1].weightfn[width>>1]( &h->mb.pic.p_fdec[1][2*y*FDEC_STRIDE+2*x], FDEC_STRIDE,
                                                   &h->mb.pic.p_fdec[1][2*y*FDEC_STRIDE+2*x], FDEC_STRIDE,
                                                   &h->sh.weight[i_ref][1], height*2 );

    h->mc.mc_chroma( &h->mb.pic.p_fdec[2][2*y*FDEC_STRIDE+2*x], FDEC_STRIDE,
                     h->mb.pic.p_fref[0][i_ref][5], h->mb.pic.i_stride[2],
                     mvx, mvy, 2*width, 2*height );

    if( h->sh.weight[i_ref][2].weightfn )
        h->sh.weight[i_ref][2].weightfn[width>>1]( &h->mb.pic.p_fdec[2][2*y*FDEC_STRIDE+2*x], FDEC_STRIDE,
                                                   &h->mb.pic.p_fdec[2][2*y*FDEC_STRIDE+2*x], FDEC_STRIDE,
                                                   &h->sh.weight[i_ref][2],height*2 );

}
static NOINLINE void x264_mb_mc_1xywh( x264_t *h, int x, int y, int width, int height )
{
    int i8    = x264_scan8[0]+x+8*y;
    int i_ref = h->mb.cache.ref[1][i8];
    int mvx   = x264_clip3( h->mb.cache.mv[1][i8][0], h->mb.mv_min[0], h->mb.mv_max[0] ) + 4*4*x;
    int mvy   = x264_clip3( h->mb.cache.mv[1][i8][1], h->mb.mv_min[1], h->mb.mv_max[1] ) + 4*4*y;

    h->mc.mc_luma( &h->mb.pic.p_fdec[0][4*y*FDEC_STRIDE+4*x], FDEC_STRIDE,
                   h->mb.pic.p_fref[1][i_ref], h->mb.pic.i_stride[0],
                   mvx, mvy, 4*width, 4*height, weight_none );

    if( h->mb.b_interlaced & i_ref )
        mvy += (h->mb.i_mb_y & 1)*4 - 2;

    h->mc.mc_chroma( &h->mb.pic.p_fdec[1][2*y*FDEC_STRIDE+2*x], FDEC_STRIDE,
                     h->mb.pic.p_fref[1][i_ref][4], h->mb.pic.i_stride[1],
                     mvx, mvy, 2*width, 2*height );

    h->mc.mc_chroma( &h->mb.pic.p_fdec[2][2*y*FDEC_STRIDE+2*x], FDEC_STRIDE,
                     h->mb.pic.p_fref[1][i_ref][5], h->mb.pic.i_stride[2],
                     mvx, mvy, 2*width, 2*height );
}

static NOINLINE void x264_mb_mc_01xywh( x264_t *h, int x, int y, int width, int height )
{
    int i8 = x264_scan8[0]+x+8*y;
    int i_ref0 = h->mb.cache.ref[0][i8];
    int i_ref1 = h->mb.cache.ref[1][i8];
    int weight = h->mb.bipred_weight[i_ref0][i_ref1];
    int mvx0   = x264_clip3( h->mb.cache.mv[0][i8][0], h->mb.mv_min[0], h->mb.mv_max[0] ) + 4*4*x;
    int mvx1   = x264_clip3( h->mb.cache.mv[1][i8][0], h->mb.mv_min[0], h->mb.mv_max[0] ) + 4*4*x;
    int mvy0   = x264_clip3( h->mb.cache.mv[0][i8][1], h->mb.mv_min[1], h->mb.mv_max[1] ) + 4*4*y;
    int mvy1   = x264_clip3( h->mb.cache.mv[1][i8][1], h->mb.mv_min[1], h->mb.mv_max[1] ) + 4*4*y;
    int i_mode = x264_size2pixel[height][width];
    int i_stride0 = 16, i_stride1 = 16;
    ALIGNED_ARRAY_16( uint8_t, tmp0,[16*16] );
    ALIGNED_ARRAY_16( uint8_t, tmp1,[16*16] );
    uint8_t *src0, *src1;

    src0 = h->mc.get_ref( tmp0, &i_stride0, h->mb.pic.p_fref[0][i_ref0], h->mb.pic.i_stride[0],
                          mvx0, mvy0, 4*width, 4*height, weight_none );
    src1 = h->mc.get_ref( tmp1, &i_stride1, h->mb.pic.p_fref[1][i_ref1], h->mb.pic.i_stride[0],
                          mvx1, mvy1, 4*width, 4*height, weight_none );
    h->mc.avg[i_mode]( &h->mb.pic.p_fdec[0][4*y*FDEC_STRIDE+4*x], FDEC_STRIDE,
                       src0, i_stride0, src1, i_stride1, weight );

    if( h->mb.b_interlaced & i_ref0 )
        mvy0 += (h->mb.i_mb_y & 1)*4 - 2;
    if( h->mb.b_interlaced & i_ref1 )
        mvy1 += (h->mb.i_mb_y & 1)*4 - 2;

    h->mc.mc_chroma( tmp0, 16, h->mb.pic.p_fref[0][i_ref0][4], h->mb.pic.i_stride[1],
                     mvx0, mvy0, 2*width, 2*height );
    h->mc.mc_chroma( tmp1, 16, h->mb.pic.p_fref[1][i_ref1][4], h->mb.pic.i_stride[1],
                     mvx1, mvy1, 2*width, 2*height );
    h->mc.avg[i_mode+3]( &h->mb.pic.p_fdec[1][2*y*FDEC_STRIDE+2*x], FDEC_STRIDE, tmp0, 16, tmp1, 16, weight );
    h->mc.mc_chroma( tmp0, 16, h->mb.pic.p_fref[0][i_ref0][5], h->mb.pic.i_stride[2],
                     mvx0, mvy0, 2*width, 2*height );
    h->mc.mc_chroma( tmp1, 16, h->mb.pic.p_fref[1][i_ref1][5], h->mb.pic.i_stride[2],
                     mvx1, mvy1, 2*width, 2*height );
    h->mc.avg[i_mode+3]( &h->mb.pic.p_fdec[2][2*y*FDEC_STRIDE+2*x], FDEC_STRIDE, tmp0, 16, tmp1, 16, weight );
}

void x264_mb_mc_8x8( x264_t *h, int i8 )
{
    int x = 2*(i8&1);
    int y = 2*(i8>>1);

    if( h->sh.i_type == SLICE_TYPE_P )
    {
        switch( h->mb.i_sub_partition[i8] )
        {
            case D_L0_8x8:
                x264_mb_mc_0xywh( h, x, y, 2, 2 );
                break;
            case D_L0_8x4:
                x264_mb_mc_0xywh( h, x, y+0, 2, 1 );
                x264_mb_mc_0xywh( h, x, y+1, 2, 1 );
                break;
            case D_L0_4x8:
                x264_mb_mc_0xywh( h, x+0, y, 1, 2 );
                x264_mb_mc_0xywh( h, x+1, y, 1, 2 );
                break;
            case D_L0_4x4:
                x264_mb_mc_0xywh( h, x+0, y+0, 1, 1 );
                x264_mb_mc_0xywh( h, x+1, y+0, 1, 1 );
                x264_mb_mc_0xywh( h, x+0, y+1, 1, 1 );
                x264_mb_mc_0xywh( h, x+1, y+1, 1, 1 );
                break;
        }
    }
    else
    {
        int scan8 = x264_scan8[0] + x + 8*y;

        if( h->mb.cache.ref[0][scan8] >= 0 )
            if( h->mb.cache.ref[1][scan8] >= 0 )
                x264_mb_mc_01xywh( h, x, y, 2, 2 );
            else
                x264_mb_mc_0xywh( h, x, y, 2, 2 );
        else
            x264_mb_mc_1xywh( h, x, y, 2, 2 );
    }
}

void x264_mb_mc( x264_t *h )
{
    if( h->mb.i_partition == D_8x8 )
    {
        for( int i = 0; i < 4; i++ )
            x264_mb_mc_8x8( h, i );
    }
    else
    {
        int ref0a = h->mb.cache.ref[0][x264_scan8[ 0]];
        int ref0b = h->mb.cache.ref[0][x264_scan8[12]];
        int ref1a = h->mb.cache.ref[1][x264_scan8[ 0]];
        int ref1b = h->mb.cache.ref[1][x264_scan8[12]];

        if( h->mb.i_partition == D_16x16 )
        {
            if( ref0a >= 0 )
                if( ref1a >= 0 ) x264_mb_mc_01xywh( h, 0, 0, 4, 4 );
                else             x264_mb_mc_0xywh ( h, 0, 0, 4, 4 );
            else                 x264_mb_mc_1xywh ( h, 0, 0, 4, 4 );
        }
        else if( h->mb.i_partition == D_16x8 )
        {
            if( ref0a >= 0 )
                if( ref1a >= 0 ) x264_mb_mc_01xywh( h, 0, 0, 4, 2 );
                else             x264_mb_mc_0xywh ( h, 0, 0, 4, 2 );
            else                 x264_mb_mc_1xywh ( h, 0, 0, 4, 2 );

            if( ref0b >= 0 )
                if( ref1b >= 0 ) x264_mb_mc_01xywh( h, 0, 2, 4, 2 );
                else             x264_mb_mc_0xywh ( h, 0, 2, 4, 2 );
            else                 x264_mb_mc_1xywh ( h, 0, 2, 4, 2 );
        }
        else if( h->mb.i_partition == D_8x16 )
        {
            if( ref0a >= 0 )
                if( ref1a >= 0 ) x264_mb_mc_01xywh( h, 0, 0, 2, 4 );
                else             x264_mb_mc_0xywh ( h, 0, 0, 2, 4 );
            else                 x264_mb_mc_1xywh ( h, 0, 0, 2, 4 );

            if( ref0b >= 0 )
                if( ref1b >= 0 ) x264_mb_mc_01xywh( h, 2, 0, 2, 4 );
                else             x264_mb_mc_0xywh ( h, 2, 0, 2, 4 );
            else                 x264_mb_mc_1xywh ( h, 2, 0, 2, 4 );
        }
    }
}

int x264_macroblock_cache_init( x264_t *h )
{
    int i_mb_count = h->mb.i_mb_count;

    h->mb.i_mb_stride = h->sps->i_mb_width;
    h->mb.i_b8_stride = h->sps->i_mb_width * 2;
    h->mb.i_b4_stride = h->sps->i_mb_width * 4;

    h->mb.b_interlaced = h->param.b_interlaced;

    CHECKED_MALLOC( h->mb.qp, i_mb_count * sizeof(int8_t) );
    CHECKED_MALLOC( h->mb.cbp, i_mb_count * sizeof(int16_t) );
    CHECKED_MALLOC( h->mb.skipbp, i_mb_count * sizeof(int8_t) );
    CHECKED_MALLOC( h->mb.mb_transform_size, i_mb_count * sizeof(int8_t) );

    /* 0 -> 3 top(4), 4 -> 6 : left(3) */
    CHECKED_MALLOC( h->mb.intra4x4_pred_mode, i_mb_count * 8 * sizeof(int8_t) );

    /* all coeffs */
    CHECKED_MALLOC( h->mb.non_zero_count, i_mb_count * 24 * sizeof(uint8_t) );

    if( h->param.b_cabac )
    {
        CHECKED_MALLOC( h->mb.chroma_pred_mode, i_mb_count * sizeof(int8_t) );
        CHECKED_MALLOC( h->mb.mvd[0], i_mb_count * sizeof( **h->mb.mvd ) );
        CHECKED_MALLOC( h->mb.mvd[1], i_mb_count * sizeof( **h->mb.mvd ) );
    }

    for( int i = 0; i < 2; i++ )
    {
        int i_refs = X264_MIN(16, (i ? 1 + !!h->param.i_bframe_pyramid : h->param.i_frame_reference) ) << h->param.b_interlaced;
        if( h->param.analyse.i_weighted_pred == X264_WEIGHTP_SMART )
            i_refs = X264_MIN(16, i_refs + 2); //smart weights add two duplicate frames
        else if( h->param.analyse.i_weighted_pred == X264_WEIGHTP_BLIND )
            i_refs = X264_MIN(16, i_refs + 1); //blind weights add one duplicate frame

        for( int j = 0; j < i_refs; j++ )
            CHECKED_MALLOC( h->mb.mvr[i][j], 2 * i_mb_count * sizeof(int16_t) );
    }

    if( h->param.analyse.i_weighted_pred )
    {
        int i_padv = PADV << h->param.b_interlaced;
#define ALIGN(x,a) (((x)+((a)-1))&~((a)-1))
        int align = h->param.cpu&X264_CPU_CACHELINE_64 ? 64 : h->param.cpu&X264_CPU_CACHELINE_32 ? 32 : 16;
        int i_stride, luma_plane_size = 0;
        int numweightbuf;

        if( h->param.analyse.i_weighted_pred == X264_WEIGHTP_FAKE )
        {
            // only need buffer for lookahead
            if( !h->param.i_sync_lookahead || h == h->thread[h->param.i_threads] )
            {
                // Fake analysis only works on lowres
                i_stride = ALIGN( h->sps->i_mb_width*8 + 2*PADH, align );
                luma_plane_size = i_stride * (h->sps->i_mb_height*8+2*i_padv);
                // Only need 1 buffer for analysis
                numweightbuf = 1;
            }
            else
                numweightbuf = 0;
        }
        else
        {
            i_stride = ALIGN( h->sps->i_mb_width*16 + 2*PADH, align );
            luma_plane_size = i_stride * (h->sps->i_mb_height*16+2*i_padv);

            if( h->param.analyse.i_weighted_pred == X264_WEIGHTP_SMART )
                //SMART can weight one ref and one offset -1
                numweightbuf = 2;
            else
                //blind only has one weighted copy (offset -1)
                numweightbuf = 1;
        }

        for( int i = 0; i < numweightbuf; i++ )
            CHECKED_MALLOC( h->mb.p_weight_buf[i], luma_plane_size );
#undef ALIGN
    }

    for( int i = 0; i <= h->param.b_interlaced; i++ )
        for( int j = 0; j < 3; j++ )
        {
            /* shouldn't really be initialized, just silences a valgrind false-positive in predict_8x8_filter_mmx */
            CHECKED_MALLOCZERO( h->mb.intra_border_backup[i][j], (h->sps->i_mb_width*16+32)>>!!j );
            h->mb.intra_border_backup[i][j] += 8;
        }

    return 0;
fail: return -1;
}
void x264_macroblock_cache_end( x264_t *h )
{
    for( int i = 0; i <= h->param.b_interlaced; i++ )
        for( int j = 0; j < 3; j++ )
            x264_free( h->mb.intra_border_backup[i][j] - 8 );
    for( int i = 0; i < 2; i++ )
        for( int j = 0; j < 32; j++ )
            x264_free( h->mb.mvr[i][j] );
    for( int i = 0; i < 16; i++ )
        x264_free( h->mb.p_weight_buf[i] );

    if( h->param.b_cabac )
    {
        x264_free( h->mb.chroma_pred_mode );
        x264_free( h->mb.mvd[0] );
        x264_free( h->mb.mvd[1] );
    }
    x264_free( h->mb.intra4x4_pred_mode );
    x264_free( h->mb.non_zero_count );
    x264_free( h->mb.mb_transform_size );
    x264_free( h->mb.skipbp );
    x264_free( h->mb.cbp );
    x264_free( h->mb.qp );
}
void x264_macroblock_slice_init( x264_t *h )
{
    h->mb.mv[0] = h->fdec->mv[0];
    h->mb.mv[1] = h->fdec->mv[1];
    h->mb.ref[0] = h->fdec->ref[0];
    h->mb.ref[1] = h->fdec->ref[1];
    h->mb.type = h->fdec->mb_type;
    h->mb.partition = h->fdec->mb_partition;

    h->fdec->i_ref[0] = h->i_ref0;
    h->fdec->i_ref[1] = h->i_ref1;
    for( int i = 0; i < h->i_ref0; i++ )
        h->fdec->ref_poc[0][i] = h->fref0[i]->i_poc;
    if( h->sh.i_type == SLICE_TYPE_B )
    {
        for( int i = 0; i < h->i_ref1; i++ )
            h->fdec->ref_poc[1][i] = h->fref1[i]->i_poc;

        map_col_to_list0(-1) = -1;
        map_col_to_list0(-2) = -2;
        for( int i = 0; i < h->fref1[0]->i_ref[0]; i++ )
        {
            int poc = h->fref1[0]->ref_poc[0][i];
            map_col_to_list0(i) = -2;
            for( int j = 0; j < h->i_ref0; j++ )
                if( h->fref0[j]->i_poc == poc )
                {
                    map_col_to_list0(i) = j;
                    break;
                }
        }
    }
    if( h->sh.i_type == SLICE_TYPE_P )
        memset( h->mb.cache.skip, 0, sizeof( h->mb.cache.skip ) );

    /* init with not available (for top right idx=7,15) */
    memset( h->mb.cache.ref, -2, sizeof( h->mb.cache.ref ) );

    setup_inverse_delta_pocs( h );

    h->mb.i_neighbour4[6] =
    h->mb.i_neighbour4[9] =
    h->mb.i_neighbour4[12] =
    h->mb.i_neighbour4[14] = MB_LEFT|MB_TOP|MB_TOPLEFT|MB_TOPRIGHT;
    h->mb.i_neighbour4[3] =
    h->mb.i_neighbour4[7] =
    h->mb.i_neighbour4[11] =
    h->mb.i_neighbour4[13] =
    h->mb.i_neighbour4[15] =
    h->mb.i_neighbour8[3] = MB_LEFT|MB_TOP|MB_TOPLEFT;
}

void x264_macroblock_thread_init( x264_t *h )
{
    h->mb.i_me_method = h->param.analyse.i_me_method;
    h->mb.i_subpel_refine = h->param.analyse.i_subpel_refine;
    if( h->sh.i_type == SLICE_TYPE_B && (h->mb.i_subpel_refine == 6 || h->mb.i_subpel_refine == 8) )
        h->mb.i_subpel_refine--;
    h->mb.b_chroma_me = h->param.analyse.b_chroma_me && h->sh.i_type == SLICE_TYPE_P
                        && h->mb.i_subpel_refine >= 5;
    h->mb.b_dct_decimate = h->sh.i_type == SLICE_TYPE_B ||
                          (h->param.analyse.b_dct_decimate && h->sh.i_type != SLICE_TYPE_I);


    /* fdec:      fenc:
     * yyyyyyy
     * yYYYY      YYYY
     * yYYYY      YYYY
     * yYYYY      YYYY
     * yYYYY      YYYY
     * uuu vvv    UUVV
     * uUU vVV    UUVV
     * uUU vVV
     */
    h->mb.pic.p_fenc[0] = h->mb.pic.fenc_buf;
    h->mb.pic.p_fenc[1] = h->mb.pic.fenc_buf + 16*FENC_STRIDE;
    h->mb.pic.p_fenc[2] = h->mb.pic.fenc_buf + 16*FENC_STRIDE + 8;
    h->mb.pic.p_fdec[0] = h->mb.pic.fdec_buf + 2*FDEC_STRIDE;
    h->mb.pic.p_fdec[1] = h->mb.pic.fdec_buf + 19*FDEC_STRIDE;
    h->mb.pic.p_fdec[2] = h->mb.pic.fdec_buf + 19*FDEC_STRIDE + 16;
}

void x264_prefetch_fenc( x264_t *h, x264_frame_t *fenc, int i_mb_x, int i_mb_y )
{
    int stride_y  = fenc->i_stride[0];
    int stride_uv = fenc->i_stride[1];
    int off_y = 16 * (i_mb_x + i_mb_y * stride_y);
    int off_uv = 8 * (i_mb_x + i_mb_y * stride_uv);
    h->mc.prefetch_fenc( fenc->plane[0]+off_y, stride_y,
                         fenc->plane[1+(i_mb_x&1)]+off_uv, stride_uv, i_mb_x );
}

static NOINLINE void copy_column8( uint8_t *dst, uint8_t *src )
{
    // input pointers are offset by 4 rows because that's faster (smaller instruction size on x86)
    for( int i = -4; i < 4; i++ )
        dst[i*FDEC_STRIDE] = src[i*FDEC_STRIDE];
}

static void ALWAYS_INLINE x264_macroblock_load_pic_pointers( x264_t *h, int i_mb_x, int i_mb_y, int i)
{
    const int w = (i == 0 ? 16 : 8);
    const int i_stride = h->fdec->i_stride[!!i];
    const int i_stride2 = i_stride << h->mb.b_interlaced;
    const int i_pix_offset = h->mb.b_interlaced
                           ? w * (i_mb_x + (i_mb_y&~1) * i_stride) + (i_mb_y&1) * i_stride
                           : w * (i_mb_x + i_mb_y * i_stride);
    const uint8_t *plane_fdec = &h->fdec->plane[i][i_pix_offset];
    const uint8_t *intra_fdec = h->param.b_sliced_threads ? plane_fdec-i_stride2 :
                                &h->mb.intra_border_backup[i_mb_y & h->sh.b_mbaff][i][i_mb_x*16>>!!i];
    int ref_pix_offset[2] = { i_pix_offset, i_pix_offset };
    x264_frame_t **fref[2] = { h->fref0, h->fref1 };
    if( h->mb.b_interlaced )
        ref_pix_offset[1] += (1-2*(i_mb_y&1)) * i_stride;
    h->mb.pic.i_stride[i] = i_stride2;
    h->mb.pic.p_fenc_plane[i] = &h->fenc->plane[i][i_pix_offset];
    h->mc.copy[i?PIXEL_8x8:PIXEL_16x16]( h->mb.pic.p_fenc[i], FENC_STRIDE,
        h->mb.pic.p_fenc_plane[i], i_stride2, w );
    if( i_mb_y > 0 )
        memcpy( &h->mb.pic.p_fdec[i][-1-FDEC_STRIDE], intra_fdec-1, w*3/2+1 );
    else
        memset( &h->mb.pic.p_fdec[i][-1-FDEC_STRIDE], 0, w*3/2+1 );
    if( h->mb.b_interlaced )
        for( int j = 0; j < w; j++ )
            h->mb.pic.p_fdec[i][-1+j*FDEC_STRIDE] = plane_fdec[-1+j*i_stride2];
    for( int j = 0; j < h->mb.pic.i_fref[0]; j++ )
    {
        h->mb.pic.p_fref[0][j][i==0 ? 0:i+3] = &fref[0][j >> h->mb.b_interlaced]->plane[i][ref_pix_offset[j&1]];
        if( i == 0 )
        {
            for( int k = 1; k < 4; k++ )
                h->mb.pic.p_fref[0][j][k] = &fref[0][j >> h->mb.b_interlaced]->filtered[k][ref_pix_offset[j&1]];
            if( h->sh.weight[j][0].weightfn )
                h->mb.pic.p_fref_w[j] = &h->fenc->weighted[j >> h->mb.b_interlaced][ref_pix_offset[j&1]];
            else
                h->mb.pic.p_fref_w[j] = h->mb.pic.p_fref[0][j][0];
        }
    }
    if( h->sh.i_type == SLICE_TYPE_B )
        for( int j = 0; j < h->mb.pic.i_fref[1]; j++ )
        {
            h->mb.pic.p_fref[1][j][i==0 ? 0:i+3] = &fref[1][j >> h->mb.b_interlaced]->plane[i][ref_pix_offset[j&1]];
            if( i == 0 )
                for( int k = 1; k < 4; k++ )
                    h->mb.pic.p_fref[1][j][k] = &fref[1][j >> h->mb.b_interlaced]->filtered[k][ref_pix_offset[j&1]];
        }
}

void x264_macroblock_cache_load( x264_t *h, int i_mb_x, int i_mb_y )
{
    int i_mb_xy = i_mb_y * h->mb.i_mb_stride + i_mb_x;
    int i_mb_4x4 = 4*(i_mb_y * h->mb.i_b4_stride + i_mb_x);
    int i_mb_8x8 = 2*(i_mb_y * h->mb.i_b8_stride + i_mb_x);
    int i_top_y = i_mb_y - (1 << h->mb.b_interlaced);
    int i_top_xy = i_top_y * h->mb.i_mb_stride + i_mb_x;
    int i_top_4x4 = (4*i_top_y+3) * h->mb.i_b4_stride + 4*i_mb_x;
    int i_top_8x8 = (2*i_top_y+1) * h->mb.i_b8_stride + 2*i_mb_x;
    int i_left_xy = -1;
    int i_top_type = -1;    /* gcc warn */
    int i_left_type= -1;

    /* init index */
    h->mb.i_mb_x = i_mb_x;
    h->mb.i_mb_y = i_mb_y;
    h->mb.i_mb_xy = i_mb_xy;
    h->mb.i_b8_xy = i_mb_8x8;
    h->mb.i_b4_xy = i_mb_4x4;
    h->mb.i_mb_top_xy = i_top_xy;
    h->mb.i_neighbour = 0;
    h->mb.i_neighbour_intra = 0;

    /* load cache */
    if( i_top_xy >= h->sh.i_first_mb )
    {
        h->mb.i_mb_type_top =
        i_top_type = h->mb.type[i_top_xy];
        h->mb.cache.i_cbp_top = h->mb.cbp[i_top_xy];

        h->mb.i_neighbour |= MB_TOP;

        if( !h->param.b_constrained_intra || IS_INTRA( i_top_type ) )
            h->mb.i_neighbour_intra |= MB_TOP;

        /* load intra4x4 */
        CP32( &h->mb.cache.intra4x4_pred_mode[x264_scan8[0] - 8], &h->mb.intra4x4_pred_mode[i_top_xy][0] );

        /* load non_zero_count */
        CP32( &h->mb.cache.non_zero_count[x264_scan8[0] - 8], &h->mb.non_zero_count[i_top_xy][12] );
        /* shift because x264_scan8[16] is misaligned */
        M32( &h->mb.cache.non_zero_count[x264_scan8[16+0] - 9] ) = M16( &h->mb.non_zero_count[i_top_xy][18] ) << 8;
        M32( &h->mb.cache.non_zero_count[x264_scan8[16+4] - 9] ) = M16( &h->mb.non_zero_count[i_top_xy][22] ) << 8;
    }
    else
    {
        h->mb.i_mb_type_top = -1;
        h->mb.cache.i_cbp_top = -1;

        /* load intra4x4 */
        M32( &h->mb.cache.intra4x4_pred_mode[x264_scan8[0] - 8] ) = 0xFFFFFFFFU;

        /* load non_zero_count */
        M32( &h->mb.cache.non_zero_count[x264_scan8[   0] - 8] ) = 0x80808080U;
        M32( &h->mb.cache.non_zero_count[x264_scan8[16+0] - 9] ) = 0x80808080U;
        M32( &h->mb.cache.non_zero_count[x264_scan8[16+4] - 9] ) = 0x80808080U;
    }

    if( i_mb_x > 0 && i_mb_xy > h->sh.i_first_mb )
    {
        i_left_xy = i_mb_xy - 1;
        h->mb.i_mb_type_left =
        i_left_type = h->mb.type[i_left_xy];
        h->mb.cache.i_cbp_left = h->mb.cbp[h->mb.i_mb_xy - 1];

        h->mb.i_neighbour |= MB_LEFT;

        if( !h->param.b_constrained_intra || IS_INTRA( i_left_type ) )
            h->mb.i_neighbour_intra |= MB_LEFT;

        /* load intra4x4 */
        h->mb.cache.intra4x4_pred_mode[x264_scan8[0 ] - 1] = h->mb.intra4x4_pred_mode[i_left_xy][4];
        h->mb.cache.intra4x4_pred_mode[x264_scan8[2 ] - 1] = h->mb.intra4x4_pred_mode[i_left_xy][5];
        h->mb.cache.intra4x4_pred_mode[x264_scan8[8 ] - 1] = h->mb.intra4x4_pred_mode[i_left_xy][6];
        h->mb.cache.intra4x4_pred_mode[x264_scan8[10] - 1] = h->mb.intra4x4_pred_mode[i_left_xy][3];

        /* load non_zero_count */
        h->mb.cache.non_zero_count[x264_scan8[0 ] - 1] = h->mb.non_zero_count[i_left_xy][3];
        h->mb.cache.non_zero_count[x264_scan8[2 ] - 1] = h->mb.non_zero_count[i_left_xy][7];
        h->mb.cache.non_zero_count[x264_scan8[8 ] - 1] = h->mb.non_zero_count[i_left_xy][11];
        h->mb.cache.non_zero_count[x264_scan8[10] - 1] = h->mb.non_zero_count[i_left_xy][15];

        h->mb.cache.non_zero_count[x264_scan8[16+0] - 1] = h->mb.non_zero_count[i_left_xy][16+1];
        h->mb.cache.non_zero_count[x264_scan8[16+2] - 1] = h->mb.non_zero_count[i_left_xy][16+3];

        h->mb.cache.non_zero_count[x264_scan8[16+4+0] - 1] = h->mb.non_zero_count[i_left_xy][16+4+1];
        h->mb.cache.non_zero_count[x264_scan8[16+4+2] - 1] = h->mb.non_zero_count[i_left_xy][16+4+3];
    }
    else
    {
        h->mb.i_mb_type_left = -1;
        h->mb.cache.i_cbp_left = -1;

        h->mb.cache.intra4x4_pred_mode[x264_scan8[0 ] - 1] =
        h->mb.cache.intra4x4_pred_mode[x264_scan8[2 ] - 1] =
        h->mb.cache.intra4x4_pred_mode[x264_scan8[8 ] - 1] =
        h->mb.cache.intra4x4_pred_mode[x264_scan8[10] - 1] = -1;

        /* load non_zero_count */
        h->mb.cache.non_zero_count[x264_scan8[0 ] - 1] =
        h->mb.cache.non_zero_count[x264_scan8[2 ] - 1] =
        h->mb.cache.non_zero_count[x264_scan8[8 ] - 1] =
        h->mb.cache.non_zero_count[x264_scan8[10] - 1] =
        h->mb.cache.non_zero_count[x264_scan8[16+0] - 1] =
        h->mb.cache.non_zero_count[x264_scan8[16+2] - 1] =
        h->mb.cache.non_zero_count[x264_scan8[16+4+0] - 1] =
        h->mb.cache.non_zero_count[x264_scan8[16+4+2] - 1] = 0x80;
    }

    if( i_mb_x < h->sps->i_mb_width - 1 && i_top_xy + 1 >= h->sh.i_first_mb )
    {
        h->mb.i_neighbour |= MB_TOPRIGHT;
        h->mb.i_mb_type_topright = h->mb.type[ i_top_xy + 1 ];
        if( !h->param.b_constrained_intra || IS_INTRA( h->mb.i_mb_type_topright ) )
            h->mb.i_neighbour_intra |= MB_TOPRIGHT;
    }
    else
        h->mb.i_mb_type_topright = -1;
    if( i_mb_x > 0 && i_top_xy - 1 >= h->sh.i_first_mb )
    {
        h->mb.i_neighbour |= MB_TOPLEFT;
        h->mb.i_mb_type_topleft = h->mb.type[ i_top_xy - 1 ];
        if( !h->param.b_constrained_intra || IS_INTRA( h->mb.i_mb_type_topleft ) )
            h->mb.i_neighbour_intra |= MB_TOPLEFT;
    }
    else
        h->mb.i_mb_type_topleft = -1;

    if( h->pps->b_transform_8x8_mode )
    {
        h->mb.cache.i_neighbour_transform_size =
            ( i_left_type >= 0 && h->mb.mb_transform_size[i_left_xy] )
          + ( i_top_type  >= 0 && h->mb.mb_transform_size[i_top_xy]  );
    }

    if( h->sh.b_mbaff )
    {
        h->mb.pic.i_fref[0] = h->i_ref0 << h->mb.b_interlaced;
        h->mb.pic.i_fref[1] = h->i_ref1 << h->mb.b_interlaced;
        h->mb.cache.i_neighbour_interlaced =
            !!(h->mb.i_neighbour & MB_LEFT)
          + !!(h->mb.i_neighbour & MB_TOP);
    }

    if( !h->mb.b_interlaced )
    {
        copy_column8( h->mb.pic.p_fdec[0]-1+ 4*FDEC_STRIDE, h->mb.pic.p_fdec[0]+15+ 4*FDEC_STRIDE );
        copy_column8( h->mb.pic.p_fdec[0]-1+12*FDEC_STRIDE, h->mb.pic.p_fdec[0]+15+12*FDEC_STRIDE );
        copy_column8( h->mb.pic.p_fdec[1]-1+ 4*FDEC_STRIDE, h->mb.pic.p_fdec[1]+ 7+ 4*FDEC_STRIDE );
        copy_column8( h->mb.pic.p_fdec[2]-1+ 4*FDEC_STRIDE, h->mb.pic.p_fdec[2]+ 7+ 4*FDEC_STRIDE );
    }

    /* load picture pointers */
    x264_macroblock_load_pic_pointers( h, i_mb_x, i_mb_y, 0 );
    x264_macroblock_load_pic_pointers( h, i_mb_x, i_mb_y, 1 );
    x264_macroblock_load_pic_pointers( h, i_mb_x, i_mb_y, 2 );

    if( h->fdec->integral )
    {
        assert( !h->mb.b_interlaced );
        for( int i = 0; i < h->mb.pic.i_fref[0]; i++ )
            h->mb.pic.p_integral[0][i] = &h->fref0[i]->integral[ 16 * ( i_mb_x + i_mb_y * h->fdec->i_stride[0] )];
        for( int i = 0; i < h->mb.pic.i_fref[1]; i++ )
            h->mb.pic.p_integral[1][i] = &h->fref1[i]->integral[ 16 * ( i_mb_x + i_mb_y * h->fdec->i_stride[0] )];
    }

    x264_prefetch_fenc( h, h->fenc, i_mb_x, i_mb_y );

    /* load ref/mv/mvd */
    if( h->sh.i_type != SLICE_TYPE_I )
    {
        const int s8x8 = h->mb.i_b8_stride;
        const int s4x4 = h->mb.i_b4_stride;

        for( int i_list = 0; i_list < (h->sh.i_type == SLICE_TYPE_B ? 2  : 1 ); i_list++ )
        {
            /*
            h->mb.cache.ref[i_list][x264_scan8[5 ]+1] =
            h->mb.cache.ref[i_list][x264_scan8[7 ]+1] =
            h->mb.cache.ref[i_list][x264_scan8[13]+1] = -2;
            */

            if( h->mb.i_neighbour & MB_TOPLEFT )
            {
                const int i8 = x264_scan8[0] - 1 - 1*8;
                const int ir = i_top_8x8 - 1;
                const int iv = i_top_4x4 - 1;
                h->mb.cache.ref[i_list][i8]  = h->mb.ref[i_list][ir];
                CP32( h->mb.cache.mv[i_list][i8], h->mb.mv[i_list][iv] );
            }
            else
            {
                const int i8 = x264_scan8[0] - 1 - 1*8;
                h->mb.cache.ref[i_list][i8] = -2;
                M32( h->mb.cache.mv[i_list][i8] ) = 0;
            }

            if( h->mb.i_neighbour & MB_TOP )
            {
                const int i8 = x264_scan8[0] - 8;
                const int ir = i_top_8x8;
                const int iv = i_top_4x4;
                h->mb.cache.ref[i_list][i8+0] =
                h->mb.cache.ref[i_list][i8+1] = h->mb.ref[i_list][ir + 0];
                h->mb.cache.ref[i_list][i8+2] =
                h->mb.cache.ref[i_list][i8+3] = h->mb.ref[i_list][ir + 1];
                CP64( h->mb.cache.mv[i_list][i8+0], h->mb.mv[i_list][iv+0] );
                CP64( h->mb.cache.mv[i_list][i8+2], h->mb.mv[i_list][iv+2] );
            }
            else
            {
                const int i8 = x264_scan8[0] - 8;
                M64( h->mb.cache.mv[i_list][i8+0] ) = 0;
                M64( h->mb.cache.mv[i_list][i8+2] ) = 0;
                M32( &h->mb.cache.ref[i_list][i8] ) = (uint8_t)(-2) * 0x01010101U;
            }

            if( h->mb.i_neighbour & MB_TOPRIGHT )
            {
                const int i8 = x264_scan8[0] + 4 - 1*8;
                const int ir = i_top_8x8 + 2;
                const int iv = i_top_4x4 + 4;
                h->mb.cache.ref[i_list][i8]  = h->mb.ref[i_list][ir];
                CP32( h->mb.cache.mv[i_list][i8], h->mb.mv[i_list][iv] );
            }
            else
            {
                const int i8 = x264_scan8[0] + 4 - 1*8;
                h->mb.cache.ref[i_list][i8] = -2;
            }

            if( h->mb.i_neighbour & MB_LEFT )
            {
                const int i8 = x264_scan8[0] - 1;
                const int ir = i_mb_8x8 - 1;
                const int iv = i_mb_4x4 - 1;
                h->mb.cache.ref[i_list][i8+0*8] =
                h->mb.cache.ref[i_list][i8+1*8] = h->mb.ref[i_list][ir + 0*s8x8];
                h->mb.cache.ref[i_list][i8+2*8] =
                h->mb.cache.ref[i_list][i8+3*8] = h->mb.ref[i_list][ir + 1*s8x8];

                CP32( h->mb.cache.mv[i_list][i8+0*8], h->mb.mv[i_list][iv + 0*s4x4] );
                CP32( h->mb.cache.mv[i_list][i8+1*8], h->mb.mv[i_list][iv + 1*s4x4] );
                CP32( h->mb.cache.mv[i_list][i8+2*8], h->mb.mv[i_list][iv + 2*s4x4] );
                CP32( h->mb.cache.mv[i_list][i8+3*8], h->mb.mv[i_list][iv + 3*s4x4] );
            }
            else
            {
                const int i8 = x264_scan8[0] - 1;
                for( int i = 0; i < 4; i++ )
                {
                    h->mb.cache.ref[i_list][i8+i*8] = -2;
                    M32( h->mb.cache.mv[i_list][i8+i*8] ) = 0;
                }
            }

            if( h->param.b_cabac )
            {
                if( i_top_type >= 0 )
                    CP64( h->mb.cache.mvd[i_list][x264_scan8[0] - 8], h->mb.mvd[i_list][i_top_xy][0] );
                else
                    M64( h->mb.cache.mvd[i_list][x264_scan8[0] - 8] ) = 0;

                if( i_left_type >= 0 )
                {
                    CP16( h->mb.cache.mvd[i_list][x264_scan8[0 ] - 1], h->mb.mvd[i_list][i_left_xy][4] );
                    CP16( h->mb.cache.mvd[i_list][x264_scan8[2 ] - 1], h->mb.mvd[i_list][i_left_xy][5] );
                    CP16( h->mb.cache.mvd[i_list][x264_scan8[8 ] - 1], h->mb.mvd[i_list][i_left_xy][6] );
                    CP16( h->mb.cache.mvd[i_list][x264_scan8[10] - 1], h->mb.mvd[i_list][i_left_xy][3] );
                }
                else
                    for( int i = 0; i < 4; i++ )
                        M16( h->mb.cache.mvd[i_list][x264_scan8[0]-1+i*8] ) = 0;
            }
        }

        /* load skip */
        if( h->sh.i_type == SLICE_TYPE_B )
        {
            h->mb.bipred_weight = h->mb.bipred_weight_buf[h->mb.b_interlaced&(i_mb_y&1)];
            h->mb.dist_scale_factor = h->mb.dist_scale_factor_buf[h->mb.b_interlaced&(i_mb_y&1)];
            if( h->param.b_cabac )
            {
                uint8_t skipbp;
                x264_macroblock_cache_skip( h, 0, 0, 4, 4, 0 );
                skipbp = i_left_type >= 0 ? h->mb.skipbp[i_left_xy] : 0;
                h->mb.cache.skip[x264_scan8[0] - 1] = skipbp & 0x2;
                h->mb.cache.skip[x264_scan8[8] - 1] = skipbp & 0x8;
                skipbp = i_top_type >= 0 ? h->mb.skipbp[i_top_xy] : 0;
                h->mb.cache.skip[x264_scan8[0] - 8] = skipbp & 0x4;
                h->mb.cache.skip[x264_scan8[4] - 8] = skipbp & 0x8;
            }
        }

        if( h->sh.i_type == SLICE_TYPE_P )
            x264_mb_predict_mv_pskip( h, h->mb.cache.pskip_mv );
    }

    h->mb.i_neighbour4[0] =
    h->mb.i_neighbour8[0] = (h->mb.i_neighbour_intra & (MB_TOP|MB_LEFT|MB_TOPLEFT))
                            | ((h->mb.i_neighbour_intra & MB_TOP) ? MB_TOPRIGHT : 0);
    h->mb.i_neighbour4[4] =
    h->mb.i_neighbour4[1] = MB_LEFT | ((h->mb.i_neighbour_intra & MB_TOP) ? (MB_TOP|MB_TOPLEFT|MB_TOPRIGHT) : 0);
    h->mb.i_neighbour4[2] =
    h->mb.i_neighbour4[8] =
    h->mb.i_neighbour4[10] =
    h->mb.i_neighbour8[2] = MB_TOP|MB_TOPRIGHT | ((h->mb.i_neighbour_intra & MB_LEFT) ? (MB_LEFT|MB_TOPLEFT) : 0);
    h->mb.i_neighbour4[5] =
    h->mb.i_neighbour8[1] = MB_LEFT | (h->mb.i_neighbour_intra & MB_TOPRIGHT)
                            | ((h->mb.i_neighbour_intra & MB_TOP) ? MB_TOP|MB_TOPLEFT : 0);
}

static void ALWAYS_INLINE x264_macroblock_store_pic( x264_t *h, int i )
{
    int w = i ? 8 : 16;
    int i_stride = h->fdec->i_stride[!!i];
    int i_stride2 = i_stride << h->mb.b_interlaced;
    int i_pix_offset = h->mb.b_interlaced
                     ? w * (h->mb.i_mb_x + (h->mb.i_mb_y&~1) * i_stride) + (h->mb.i_mb_y&1) * i_stride
                     : w * (h->mb.i_mb_x + h->mb.i_mb_y * i_stride);
    h->mc.copy[i?PIXEL_8x8:PIXEL_16x16]( &h->fdec->plane[i][i_pix_offset], i_stride2,
                                         h->mb.pic.p_fdec[i], FDEC_STRIDE, w );
}

void x264_macroblock_cache_save( x264_t *h )
{
    const int i_mb_xy = h->mb.i_mb_xy;
    const int i_mb_type = x264_mb_type_fix[h->mb.i_type];
    const int s8x8 = h->mb.i_b8_stride;
    const int s4x4 = h->mb.i_b4_stride;
    const int i_mb_4x4 = h->mb.i_b4_xy;
    const int i_mb_8x8 = h->mb.i_b8_xy;

    /* GCC pessimizes direct stores to heap-allocated 8-bit arrays due to aliasing.*/
    /* By only dereferencing them once, we avoid this issue. */
    int8_t *intra4x4_pred_mode = h->mb.intra4x4_pred_mode[i_mb_xy];
    uint8_t *non_zero_count = h->mb.non_zero_count[i_mb_xy];

    x264_macroblock_store_pic( h, 0 );
    x264_macroblock_store_pic( h, 1 );
    x264_macroblock_store_pic( h, 2 );

    x264_prefetch_fenc( h, h->fdec, h->mb.i_mb_x, h->mb.i_mb_y );

    h->mb.type[i_mb_xy] = i_mb_type;
    h->mb.partition[i_mb_xy] = IS_INTRA( i_mb_type ) ? D_16x16 : h->mb.i_partition;
    h->mb.i_mb_prev_xy = i_mb_xy;

    /* save intra4x4 */
    if( i_mb_type == I_4x4 )
    {
        CP32( &intra4x4_pred_mode[0], &h->mb.cache.intra4x4_pred_mode[x264_scan8[10]] );
        M32( &intra4x4_pred_mode[4] ) = pack8to32(h->mb.cache.intra4x4_pred_mode[x264_scan8[5] ],
                                                  h->mb.cache.intra4x4_pred_mode[x264_scan8[7] ],
                                                  h->mb.cache.intra4x4_pred_mode[x264_scan8[13] ], 0);
    }
    else if( !h->param.b_constrained_intra || IS_INTRA(i_mb_type) )
        M64( intra4x4_pred_mode ) = I_PRED_4x4_DC * 0x0101010101010101ULL;
    else
        M64( intra4x4_pred_mode ) = (uint8_t)(-1) * 0x0101010101010101ULL;


    if( i_mb_type == I_PCM )
    {
        h->mb.qp[i_mb_xy] = 0;
        h->mb.i_last_dqp = 0;
        h->mb.i_cbp_chroma = 2;
        h->mb.i_cbp_luma = 0xf;
        h->mb.cbp[i_mb_xy] = 0x72f;   /* all set */
        h->mb.b_transform_8x8 = 0;
        memset( non_zero_count, 16, sizeof( *h->mb.non_zero_count ) );
    }
    else
    {
        /* save non zero count */
        CP32( &non_zero_count[0*4], &h->mb.cache.non_zero_count[x264_scan8[0]+0*8] );
        CP32( &non_zero_count[1*4], &h->mb.cache.non_zero_count[x264_scan8[0]+1*8] );
        CP32( &non_zero_count[2*4], &h->mb.cache.non_zero_count[x264_scan8[0]+2*8] );
        CP32( &non_zero_count[3*4], &h->mb.cache.non_zero_count[x264_scan8[0]+3*8] );
        M16( &non_zero_count[16+0*2] ) = M32( &h->mb.cache.non_zero_count[x264_scan8[16+0*2]-1] ) >> 8;
        M16( &non_zero_count[16+1*2] ) = M32( &h->mb.cache.non_zero_count[x264_scan8[16+1*2]-1] ) >> 8;
        M16( &non_zero_count[16+2*2] ) = M32( &h->mb.cache.non_zero_count[x264_scan8[16+2*2]-1] ) >> 8;
        M16( &non_zero_count[16+3*2] ) = M32( &h->mb.cache.non_zero_count[x264_scan8[16+3*2]-1] ) >> 8;

        if( h->mb.i_type != I_16x16 && h->mb.i_cbp_luma == 0 && h->mb.i_cbp_chroma == 0 )
            h->mb.i_qp = h->mb.i_last_qp;
        h->mb.qp[i_mb_xy] = h->mb.i_qp;
        h->mb.i_last_dqp = h->mb.i_qp - h->mb.i_last_qp;
        h->mb.i_last_qp = h->mb.i_qp;
    }

    if( h->mb.i_cbp_luma == 0 && h->mb.i_type != I_8x8 )
        h->mb.b_transform_8x8 = 0;
    h->mb.mb_transform_size[i_mb_xy] = h->mb.b_transform_8x8;

    if( h->sh.i_type != SLICE_TYPE_I )
    {
        if( !IS_INTRA( i_mb_type ) )
        {
            h->mb.ref[0][i_mb_8x8+0+0*s8x8] = h->mb.cache.ref[0][x264_scan8[0]];
            h->mb.ref[0][i_mb_8x8+1+0*s8x8] = h->mb.cache.ref[0][x264_scan8[4]];
            h->mb.ref[0][i_mb_8x8+0+1*s8x8] = h->mb.cache.ref[0][x264_scan8[8]];
            h->mb.ref[0][i_mb_8x8+1+1*s8x8] = h->mb.cache.ref[0][x264_scan8[12]];
            for( int y = 0; y < 4; y++ )
            {
                CP64( h->mb.mv[0][i_mb_4x4+y*s4x4+0], h->mb.cache.mv[0][x264_scan8[0]+8*y+0] );
                CP64( h->mb.mv[0][i_mb_4x4+y*s4x4+2], h->mb.cache.mv[0][x264_scan8[0]+8*y+2] );
            }
            if( h->sh.i_type == SLICE_TYPE_B )
            {
                h->mb.ref[1][i_mb_8x8+0+0*s8x8] = h->mb.cache.ref[1][x264_scan8[0]];
                h->mb.ref[1][i_mb_8x8+1+0*s8x8] = h->mb.cache.ref[1][x264_scan8[4]];
                h->mb.ref[1][i_mb_8x8+0+1*s8x8] = h->mb.cache.ref[1][x264_scan8[8]];
                h->mb.ref[1][i_mb_8x8+1+1*s8x8] = h->mb.cache.ref[1][x264_scan8[12]];
                for( int y = 0; y < 4; y++ )
                {
                    CP64( h->mb.mv[1][i_mb_4x4+y*s4x4+0], h->mb.cache.mv[1][x264_scan8[0]+8*y+0] );
                    CP64( h->mb.mv[1][i_mb_4x4+y*s4x4+2], h->mb.cache.mv[1][x264_scan8[0]+8*y+2] );
                }
            }
        }
        else
        {
            for( int i_list = 0; i_list < (h->sh.i_type == SLICE_TYPE_B ? 2  : 1 ); i_list++ )
            {
                M16( &h->mb.ref[i_list][i_mb_8x8+0*s8x8] ) = (uint8_t)(-1) * 0x0101;
                M16( &h->mb.ref[i_list][i_mb_8x8+1*s8x8] ) = (uint8_t)(-1) * 0x0101;
                for( int y = 0; y < 4; y++ )
                {
                    M64( h->mb.mv[i_list][i_mb_4x4+y*s4x4+0] ) = 0;
                    M64( h->mb.mv[i_list][i_mb_4x4+y*s4x4+2] ) = 0;
                }
            }
        }
    }

    if( h->param.b_cabac )
    {
        if( IS_INTRA(i_mb_type) && i_mb_type != I_PCM )
            h->mb.chroma_pred_mode[i_mb_xy] = x264_mb_pred_mode8x8c_fix[ h->mb.i_chroma_pred_mode ];
        else
            h->mb.chroma_pred_mode[i_mb_xy] = I_PRED_CHROMA_DC;

        if( !IS_INTRA( i_mb_type ) && !IS_SKIP( i_mb_type ) && !IS_DIRECT( i_mb_type ) )
        {
            CP64( h->mb.mvd[0][i_mb_xy][0], h->mb.cache.mvd[0][x264_scan8[10]] );
            CP16( h->mb.mvd[0][i_mb_xy][4], h->mb.cache.mvd[0][x264_scan8[5 ]] );
            CP16( h->mb.mvd[0][i_mb_xy][5], h->mb.cache.mvd[0][x264_scan8[7 ]] );
            CP16( h->mb.mvd[0][i_mb_xy][6], h->mb.cache.mvd[0][x264_scan8[13]] );
            if( h->sh.i_type == SLICE_TYPE_B )
            {
                CP64( h->mb.mvd[1][i_mb_xy][0], h->mb.cache.mvd[1][x264_scan8[10]] );
                CP16( h->mb.mvd[1][i_mb_xy][4], h->mb.cache.mvd[1][x264_scan8[5 ]] );
                CP16( h->mb.mvd[1][i_mb_xy][5], h->mb.cache.mvd[1][x264_scan8[7 ]] );
                CP16( h->mb.mvd[1][i_mb_xy][6], h->mb.cache.mvd[1][x264_scan8[13]] );
            }
        }
        else
        {
            M64( h->mb.mvd[0][i_mb_xy][0] ) = 0;
            M64( h->mb.mvd[0][i_mb_xy][4] ) = 0;
            if( h->sh.i_type == SLICE_TYPE_B )
            {
                M64( h->mb.mvd[1][i_mb_xy][0] ) = 0;
                M64( h->mb.mvd[1][i_mb_xy][4] ) = 0;
            }
        }

        if( h->sh.i_type == SLICE_TYPE_B )
        {
            if( i_mb_type == B_SKIP || i_mb_type == B_DIRECT )
                h->mb.skipbp[i_mb_xy] = 0xf;
            else if( i_mb_type == B_8x8 )
            {
                int skipbp = ( h->mb.i_sub_partition[0] == D_DIRECT_8x8 ) << 0;
                skipbp    |= ( h->mb.i_sub_partition[1] == D_DIRECT_8x8 ) << 1;
                skipbp    |= ( h->mb.i_sub_partition[2] == D_DIRECT_8x8 ) << 2;
                skipbp    |= ( h->mb.i_sub_partition[3] == D_DIRECT_8x8 ) << 3;
                h->mb.skipbp[i_mb_xy] = skipbp;
            }
            else
                h->mb.skipbp[i_mb_xy] = 0;
        }
    }
}


void x264_macroblock_bipred_init( x264_t *h )
{
    for( int field = 0; field <= h->sh.b_mbaff; field++ )
        for( int i_ref0 = 0; i_ref0 < (h->i_ref0<<h->sh.b_mbaff); i_ref0++ )
        {
            int poc0 = h->fref0[i_ref0>>h->sh.b_mbaff]->i_poc;
            if( h->sh.b_mbaff && field^(i_ref0&1) )
                poc0 += h->sh.i_delta_poc_bottom;
            for( int i_ref1 = 0; i_ref1 < (h->i_ref1<<h->sh.b_mbaff); i_ref1++ )
            {
                int dist_scale_factor;
                int poc1 = h->fref1[i_ref1>>h->sh.b_mbaff]->i_poc;
                if( h->sh.b_mbaff && field^(i_ref1&1) )
                    poc1 += h->sh.i_delta_poc_bottom;
                int cur_poc = h->fdec->i_poc + field*h->sh.i_delta_poc_bottom;
                int td = x264_clip3( poc1 - poc0, -128, 127 );
                if( td == 0 /* || pic0 is a long-term ref */ )
                    dist_scale_factor = 256;
                else
                {
                    int tb = x264_clip3( cur_poc - poc0, -128, 127 );
                    int tx = (16384 + (abs(td) >> 1)) / td;
                    dist_scale_factor = x264_clip3( (tb * tx + 32) >> 6, -1024, 1023 );
                }

                h->mb.dist_scale_factor_buf[field][i_ref0][i_ref1] = dist_scale_factor;

                dist_scale_factor >>= 2;
                if( h->param.analyse.b_weighted_bipred
                      && dist_scale_factor >= -64
                      && dist_scale_factor <= 128 )
                {
                    h->mb.bipred_weight_buf[field][i_ref0][i_ref1] = 64 - dist_scale_factor;
                    // ssse3 implementation of biweight doesn't support the extrema.
                    // if we ever generate them, we'll have to drop that optimization.
                    assert( dist_scale_factor >= -63 && dist_scale_factor <= 127 );
                }
                else
                    h->mb.bipred_weight_buf[field][i_ref0][i_ref1] = 32;
            }
        }
}


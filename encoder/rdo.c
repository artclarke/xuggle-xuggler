/*****************************************************************************
 * rdo.c: h264 encoder library (rate-distortion optimization)
 *****************************************************************************
 * Copyright (C) 2005 x264 project
 *
 * Authors: Loren Merritt <lorenm@u.washington.edu>
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

/* duplicate all the writer functions, just calculating bit cost
 * instead of writing the bitstream.
 * TODO: use these for fast 1st pass too. */

#define RDO_SKIP_BS

/* CAVLC: produces exactly the same bit count as a normal encode */
/* this probably still leaves some unnecessary computations */
#define bs_write1(s,v)     ((s)->i_bits_encoded += 1)
#define bs_write(s,n,v)    ((s)->i_bits_encoded += (n))
#define bs_write_ue(s,v)   ((s)->i_bits_encoded += bs_size_ue(v))
#define bs_write_se(s,v)   ((s)->i_bits_encoded += bs_size_se(v))
#define bs_write_te(s,v,l) ((s)->i_bits_encoded += bs_size_te(v,l))
#define x264_macroblock_write_cavlc  x264_macroblock_size_cavlc
#include "cavlc.c"

/* CABAC: not exactly the same. x264_cabac_size_decision() keeps track of
 * fractional bits, but only finite precision. */
#define x264_cabac_encode_decision(c,x,v) x264_cabac_size_decision(c,x,v)
#define x264_cabac_encode_terminal(c,v)   x264_cabac_size_decision(c,276,v)
#define x264_cabac_encode_bypass(c,v)     ((c)->f8_bits_encoded += 256)
#define x264_cabac_encode_flush(c)
#define x264_macroblock_write_cabac  x264_macroblock_size_cabac
#define x264_cabac_mb_skip  x264_cabac_mb_size_skip_unused
#include "cabac.c"


static int x264_rd_cost_mb( x264_t *h, int i_lambda2 )
{
    // backup mb_type because x264_macroblock_encode may change it to skip
    int i_type_bak = h->mb.i_type;
    int b_transform_bak = h->mb.b_transform_8x8;
    int i_ssd;
    int i_bits;

    x264_macroblock_encode( h );

    i_ssd = h->pixf.ssd[PIXEL_16x16]( h->mb.pic.p_fenc[0], h->mb.pic.i_stride[0],
                                      h->mb.pic.p_fdec[0], h->mb.pic.i_stride[0] )
          + h->pixf.ssd[PIXEL_8x8](   h->mb.pic.p_fenc[1], h->mb.pic.i_stride[1],
                                      h->mb.pic.p_fdec[1], h->mb.pic.i_stride[1] )
          + h->pixf.ssd[PIXEL_8x8](   h->mb.pic.p_fenc[2], h->mb.pic.i_stride[2],
                                      h->mb.pic.p_fdec[2], h->mb.pic.i_stride[2] );

    if( IS_SKIP( h->mb.i_type ) )
    {
        i_bits = 1 * i_lambda2;
    }
    else if( h->param.b_cabac )
    {
        x264_cabac_t cabac_tmp = h->cabac;
        cabac_tmp.f8_bits_encoded = 0;
        x264_macroblock_size_cabac( h, &cabac_tmp );
        i_bits = ( cabac_tmp.f8_bits_encoded * i_lambda2 + 128 ) >> 8;
    }
    else
    {
        bs_t bs_tmp = h->out.bs;
        bs_tmp.i_bits_encoded = 0;
        x264_macroblock_size_cavlc( h, &bs_tmp );
        i_bits = bs_tmp.i_bits_encoded * i_lambda2;
    }
    h->mb.i_type = i_type_bak;
    h->mb.b_transform_8x8 = b_transform_bak;

    return i_ssd + i_bits;
}

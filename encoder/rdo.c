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
        i_bits = 1;
    }
    else if( h->param.b_cabac )
    {
        x264_cabac_t cabac_tmp = h->cabac;
        bs_t bs_tmp = h->out.bs;
        cabac_tmp.s = &bs_tmp;
        x264_macroblock_write_cabac( h, &cabac_tmp );
        i_bits = x264_cabac_pos( &cabac_tmp ) - x264_cabac_pos( &h->cabac );
    }
    else
    {
        bs_t bs_tmp = h->out.bs;
        x264_macroblock_write_cavlc( h, &bs_tmp );
        i_bits = bs_pos( &bs_tmp ) - bs_pos( &h->out.bs );
    }
    h->mb.i_type = i_type_bak;
    h->mb.b_transform_8x8 = b_transform_bak;

    return i_ssd + i_bits * i_lambda2;
}

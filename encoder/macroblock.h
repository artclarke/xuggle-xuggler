/*****************************************************************************
 * macroblock.h: macroblock encoding
 *****************************************************************************
 * Copyright (C) 2003-2011 x264 project
 *
 * Authors: Loren Merritt <lorenm@u.washington.edu>
 *          Laurent Aimar <fenrir@via.ecp.fr>
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

#ifndef X264_ENCODER_MACROBLOCK_H
#define X264_ENCODER_MACROBLOCK_H

#include "common/macroblock.h"

extern const int x264_lambda2_tab[QP_MAX_MAX+1];
extern const uint16_t x264_lambda_tab[QP_MAX_MAX+1];

void x264_rdo_init( void );

int x264_macroblock_probe_skip( x264_t *h, int b_bidir );

#define x264_macroblock_probe_pskip( h )\
    x264_macroblock_probe_skip( h, 0 )
#define x264_macroblock_probe_bskip( h )\
    x264_macroblock_probe_skip( h, 1 )

void x264_predict_lossless_8x8_chroma( x264_t *h, int i_mode );
void x264_predict_lossless_4x4( x264_t *h, pixel *p_dst, int p, int idx, int i_mode );
void x264_predict_lossless_8x8( x264_t *h, pixel *p_dst, int p, int idx, int i_mode, pixel edge[33] );
void x264_predict_lossless_16x16( x264_t *h, int p, int i_mode );

void x264_macroblock_encode      ( x264_t *h );
void x264_macroblock_write_cabac ( x264_t *h, x264_cabac_t *cb );
void x264_macroblock_write_cavlc ( x264_t *h );

void x264_macroblock_encode_p8x8( x264_t *h, int i8 );
void x264_macroblock_encode_p4x4( x264_t *h, int i4 );
void x264_mb_encode_i4x4( x264_t *h, int p, int idx, int i_qp, int i_mode );
void x264_mb_encode_i8x8( x264_t *h, int p, int idx, int i_qp, int i_mode, pixel *edge );
void x264_mb_encode_8x8_chroma( x264_t *h, int b_inter, int i_qp );

void x264_cabac_mb_skip( x264_t *h, int b_skip );

int x264_quant_dc_trellis( x264_t *h, dctcoef *dct, int i_quant_cat,
                             int i_qp, int ctx_block_cat, int b_intra, int b_chroma, int idx );
int x264_quant_4x4_trellis( x264_t *h, dctcoef *dct, int i_quant_cat,
                             int i_qp, int ctx_block_cat, int b_intra, int b_chroma, int idx );
int x264_quant_8x8_trellis( x264_t *h, dctcoef *dct, int i_quant_cat,
                             int i_qp, int ctx_block_cat, int b_intra, int b_chroma, int idx );

void x264_noise_reduction_update( x264_t *h );

#endif


/*****************************************************************************
 * predict.c: h264 encoder
 *****************************************************************************
 * Copyright (C) 2009 x264 project
 *
 * Authors: David Conrad <lessen42@gmail.com>
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

#include "common/common.h"
#include "predict.h"
#include "pixel.h"

void x264_predict_4x4_dc_armv6( uint8_t *src );
void x264_predict_4x4_h_armv6( uint8_t *src );
void x264_predict_4x4_ddr_armv6( uint8_t *src );
void x264_predict_4x4_ddl_neon( uint8_t *src );

void x264_predict_8x8c_h_neon( uint8_t *src );
void x264_predict_8x8c_v_neon( uint8_t *src );

void x264_predict_8x8_dc_neon( uint8_t *src, uint8_t edge[33] );
void x264_predict_8x8_h_neon( uint8_t *src, uint8_t edge[33] );

void x264_predict_16x16_dc_neon( uint8_t *src );
void x264_predict_16x16_h_neon( uint8_t *src );
void x264_predict_16x16_v_neon( uint8_t *src );

void x264_predict_4x4_init_arm( int cpu, x264_predict_t pf[12] )
{
    if (!(cpu&X264_CPU_ARMV6))
        return;

    pf[I_PRED_4x4_H]   = x264_predict_4x4_h_armv6;
    pf[I_PRED_4x4_DC]  = x264_predict_4x4_dc_armv6;
    pf[I_PRED_4x4_DDR] = x264_predict_4x4_ddr_armv6;

    if (!(cpu&X264_CPU_NEON))
        return;

    pf[I_PRED_4x4_DDL] = x264_predict_4x4_ddl_neon;
}

void x264_predict_8x8c_init_arm( int cpu, x264_predict_t pf[7] )
{
    if (!(cpu&X264_CPU_NEON))
        return;

    pf[I_PRED_CHROMA_H] = x264_predict_8x8c_h_neon;
    pf[I_PRED_CHROMA_V] = x264_predict_8x8c_v_neon;
}

void x264_predict_8x8_init_arm( int cpu, x264_predict8x8_t pf[12], x264_predict_8x8_filter_t *predict_filter )
{
    if (!(cpu&X264_CPU_NEON))
        return;

    pf[I_PRED_8x8_DC]  = x264_predict_8x8_dc_neon;
    pf[I_PRED_8x8_H]   = x264_predict_8x8_h_neon;
}

void x264_predict_16x16_init_arm( int cpu, x264_predict_t pf[7] )
{
    if (!(cpu&X264_CPU_NEON))
        return;

    pf[I_PRED_16x16_DC ]    = x264_predict_16x16_dc_neon;
    pf[I_PRED_16x16_H ]     = x264_predict_16x16_h_neon;
    pf[I_PRED_16x16_V ]     = x264_predict_16x16_v_neon;
}

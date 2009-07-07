/*****************************************************************************
 * cpu.h: h264 encoder library
 *****************************************************************************
 * Copyright (C) 2004-2008 Loren Merritt <lorenm@u.washington.edu>
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

#ifndef X264_CPU_H
#define X264_CPU_H

uint32_t x264_cpu_detect( void );
int      x264_cpu_num_processors( void );
void     x264_emms( void );
void     x264_cpu_mask_misalign_sse( void );

/* kluge:
 * gcc can't give variables any greater alignment than the stack frame has.
 * We need 16 byte alignment for SSE2, so here we make sure that the stack is
 * aligned to 16 bytes.
 * gcc 4.2 introduced __attribute__((force_align_arg_pointer)) to fix this
 * problem, but I don't want to require such a new version.
 * This applies only to x86_32, since other architectures that need alignment
 * also have ABIs that ensure aligned stack. */
#if defined(ARCH_X86) && defined(HAVE_MMX)
int x264_stack_align( void (*func)(x264_t*), x264_t *arg );
#define x264_stack_align(func,arg) x264_stack_align((void (*)(x264_t*))func,arg)
#else
#define x264_stack_align(func,arg) func(arg)
#endif

typedef struct {
    const char name[16];
    int flags;
} x264_cpu_name_t;
extern const x264_cpu_name_t x264_cpu_names[];

#endif

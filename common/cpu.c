/*****************************************************************************
 * cpu.c: h264 encoder library
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar
 * $Id: cpu.c,v 1.1 2004/06/03 19:27:06 fenrir Exp $
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

#if defined(HAVE_PTHREAD) && defined(SYS_LINUX)
#define _GNU_SOURCE
#include <sched.h>
#endif
#ifdef SYS_BEOS
#include <kernel/OS.h>
#endif
#if defined(SYS_MACOSX) || defined(SYS_FREEBSD)
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

#include "common.h"

const struct {
    const char name[8];
    int flags;
} x264_cpu_names[] = {
    {"MMX",     X264_CPU_MMX},
    {"MMX2",    X264_CPU_MMX|X264_CPU_MMXEXT},
    {"MMXEXT",  X264_CPU_MMX|X264_CPU_MMXEXT},
    {"SSE",     X264_CPU_MMX|X264_CPU_MMXEXT|X264_CPU_SSE},
    {"SSE1",    X264_CPU_MMX|X264_CPU_MMXEXT|X264_CPU_SSE},
    {"SSE2",    X264_CPU_MMX|X264_CPU_MMXEXT|X264_CPU_SSE|X264_CPU_SSE2},
    {"SSE3",    X264_CPU_MMX|X264_CPU_MMXEXT|X264_CPU_SSE|X264_CPU_SSE2|X264_CPU_SSE3},
    {"SSSE3",   X264_CPU_MMX|X264_CPU_MMXEXT|X264_CPU_SSE|X264_CPU_SSE2|X264_CPU_SSE3|X264_CPU_SSSE3},
    {"SSE4",    X264_CPU_MMX|X264_CPU_MMXEXT|X264_CPU_SSE|X264_CPU_SSE2|X264_CPU_SSE3|X264_CPU_SSSE3|X264_CPU_SSE4},
    {"3DNow",   X264_CPU_3DNOW},
    {"Altivec", X264_CPU_ALTIVEC},
    {"Cache32", X264_CPU_CACHELINE_SPLIT|X264_CPU_CACHELINE_32},
    {"Cache64", X264_CPU_CACHELINE_SPLIT|X264_CPU_CACHELINE_64},
    {"", 0},
};

#ifdef HAVE_MMX
extern int  x264_cpu_cpuid_test( void );
extern uint32_t  x264_cpu_cpuid( uint32_t op, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx );

uint32_t x264_cpu_detect( void )
{
    uint32_t cpu = 0;
    uint32_t eax, ebx, ecx, edx;
    uint32_t vendor[4] = {0};
    int max_extended_cap;
    int cache;

#ifndef ARCH_X86_64
    if( !x264_cpu_cpuid_test() )
        return 0;
#endif

    x264_cpu_cpuid( 0, &eax, vendor+0, vendor+2, vendor+1 );
    if( eax == 0 )
        return 0;

    x264_cpu_cpuid( 1, &eax, &ebx, &ecx, &edx );
    if( edx&0x00800000 )
        cpu |= X264_CPU_MMX;
    else
        return 0;
    if( edx&0x02000000 )
        cpu |= X264_CPU_MMXEXT|X264_CPU_SSE;
    if( edx&0x04000000 )
        cpu |= X264_CPU_SSE2;
    if( ecx&0x00000001 )
        cpu |= X264_CPU_SSE3;
    if( ecx&0x00000200 )
        cpu |= X264_CPU_SSSE3;
    if( ecx&0x00080000 )
        cpu |= X264_CPU_SSE4;

    x264_cpu_cpuid( 0x80000000, &eax, &ebx, &ecx, &edx );
    max_extended_cap = eax;

    if( !strcmp((char*)vendor, "AuthenticAMD") && max_extended_cap >= 0x80000001 )
    {
        x264_cpu_cpuid( 0x80000001, &eax, &ebx, &ecx, &edx );
        if( edx&0x80000000 )
            cpu |= X264_CPU_3DNOW;
        if( edx&0x00400000 )
            cpu |= X264_CPU_MMXEXT;
    }

    if( !strcmp((char*)vendor, "GenuineIntel") || !strcmp((char*)vendor, "CyrixInstead") )
        cpu |= X264_CPU_CACHELINE_SPLIT;
    /* cacheline size is specified in 3 places, any of which may be missing */
    x264_cpu_cpuid( 1, &eax, &ebx, &ecx, &edx );
    cache = (ebx&0xff00)>>5; // cflush size
    if( !cache && max_extended_cap >= 0x80000006 )
    {
        x264_cpu_cpuid( 0x80000006, &eax, &ebx, &ecx, &edx );
        cache = ecx&0xff; // cacheline size
    }
    if( !cache )
    {
        // Cache and TLB Information
        static const char cache32_ids[] = { 0x0a, 0x0c, 0x41, 0x42, 0x43, 0x44, 0x45, 0x82, 0x83, 0x84, 0x85, 0 };
        static const char cache64_ids[] = { 0x22, 0x23, 0x25, 0x29, 0x2c, 0x46, 0x47, 0x49, 0x60, 0x66, 0x67, 0x68, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7c, 0x7f, 0x86, 0x87, 0 };
        uint32_t buf[4];
        int max, i=0, j;
        do {
            x264_cpu_cpuid( 2, buf+0, buf+1, buf+2, buf+3 );
            max = buf[0]&0xff;
            buf[0] &= ~0xff;
            for(j=0; j<4; j++)
                if( !(buf[j]>>31) )
                    while( buf[j] )
                    {
                        if( strchr( cache32_ids, buf[j]&0xff ) )
                            cache = 32;
                        if( strchr( cache64_ids, buf[j]&0xff ) )
                            cache = 64;
                        buf[j] >>= 8;
                    }
        } while( ++i < max );
    }

    if( cache == 32 )
        cpu |= X264_CPU_CACHELINE_32;
    if( cache == 64 )
        cpu |= X264_CPU_CACHELINE_64;

    return cpu;
}

#elif defined( ARCH_PPC )

#ifdef SYS_MACOSX
#include <sys/sysctl.h>
uint32_t x264_cpu_detect( void )
{
    /* Thank you VLC */
    uint32_t cpu = 0;
    int      selectors[2] = { CTL_HW, HW_VECTORUNIT };
    int      has_altivec = 0;
    size_t   length = sizeof( has_altivec );
    int      error = sysctl( selectors, 2, &has_altivec, &length, NULL, 0 );

    if( error == 0 && has_altivec != 0 )
    {
        cpu |= X264_CPU_ALTIVEC;
    }

    return cpu;
}

#elif defined( SYS_LINUX )
#include <signal.h>
#include <setjmp.h>
static sigjmp_buf jmpbuf;
static volatile sig_atomic_t canjump = 0;

static void sigill_handler( int sig )
{
    if( !canjump )
    {
        signal( sig, SIG_DFL );
        raise( sig );
    }

    canjump = 0;
    siglongjmp( jmpbuf, 1 );
}

uint32_t x264_cpu_detect( void )
{
    static void (* oldsig)( int );

    oldsig = signal( SIGILL, sigill_handler );
    if( sigsetjmp( jmpbuf, 1 ) )
    {
        signal( SIGILL, oldsig );
        return 0;
    }

    canjump = 1;
    asm volatile( "mtspr 256, %0\n\t"
                  "vand 0, 0, 0\n\t"
                  :
                  : "r"(-1) );
    canjump = 0;

    signal( SIGILL, oldsig );

    return X264_CPU_ALTIVEC;
}
#endif

#else

uint32_t x264_cpu_detect( void )
{
    return 0;
}

#endif

#ifndef HAVE_MMX
void x264_emms( void )
{
}
#endif


int x264_cpu_num_processors( void )
{
#if !defined(HAVE_PTHREAD)
    return 1;

#elif defined(_WIN32)
    return pthread_num_processors_np();

#elif defined(SYS_LINUX)
    unsigned int bit;
    int np;
    cpu_set_t p_aff;
    memset( &p_aff, 0, sizeof(p_aff) );
    sched_getaffinity( 0, sizeof(p_aff), &p_aff );
    for( np = 0, bit = 0; bit < sizeof(p_aff); bit++ )
        np += (((uint8_t *)&p_aff)[bit / 8] >> (bit % 8)) & 1;
    return np;

#elif defined(SYS_BEOS)
    system_info info;
    get_system_info( &info );
    return info.cpu_count;

#elif defined(SYS_MACOSX) || defined(SYS_FREEBSD)
    int numberOfCPUs;
    size_t length = sizeof( numberOfCPUs );
    if( sysctlbyname("hw.ncpu", &numberOfCPUs, &length, NULL, 0) )
    {
        numberOfCPUs = 1;
    }
    return numberOfCPUs;

#else
    return 1;
#endif
}

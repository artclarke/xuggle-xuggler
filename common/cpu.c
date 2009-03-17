/*****************************************************************************
 * cpu.c: h264 encoder library
 *****************************************************************************
 * Copyright (C) 2003-2008 x264 project
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
#ifdef SYS_OPENBSD
#include <sys/param.h>
#include <sys/sysctl.h>
#include <machine/cpu.h>
#endif

#include "common.h"
#include "cpu.h"

const x264_cpu_name_t x264_cpu_names[] = {
    {"Altivec", X264_CPU_ALTIVEC},
//  {"MMX",     X264_CPU_MMX}, // we don't support asm on mmx1 cpus anymore
    {"MMX2",    X264_CPU_MMX|X264_CPU_MMXEXT},
    {"MMXEXT",  X264_CPU_MMX|X264_CPU_MMXEXT},
//  {"SSE",     X264_CPU_MMX|X264_CPU_MMXEXT|X264_CPU_SSE}, // there are no sse1 functions in x264
    {"SSE2Slow",X264_CPU_MMX|X264_CPU_MMXEXT|X264_CPU_SSE|X264_CPU_SSE2|X264_CPU_SSE2_IS_SLOW},
    {"SSE2",    X264_CPU_MMX|X264_CPU_MMXEXT|X264_CPU_SSE|X264_CPU_SSE2},
    {"SSE2Fast",X264_CPU_MMX|X264_CPU_MMXEXT|X264_CPU_SSE|X264_CPU_SSE2|X264_CPU_SSE2_IS_FAST},
    {"SSE3",    X264_CPU_MMX|X264_CPU_MMXEXT|X264_CPU_SSE|X264_CPU_SSE2|X264_CPU_SSE3},
    {"SSSE3",   X264_CPU_MMX|X264_CPU_MMXEXT|X264_CPU_SSE|X264_CPU_SSE2|X264_CPU_SSE3|X264_CPU_SSSE3},
    {"FastShuffle",   X264_CPU_MMX|X264_CPU_MMXEXT|X264_CPU_SSE|X264_CPU_SSE2|X264_CPU_SHUFFLE_IS_FAST},
    {"SSE4.1",  X264_CPU_MMX|X264_CPU_MMXEXT|X264_CPU_SSE|X264_CPU_SSE2|X264_CPU_SSE3|X264_CPU_SSSE3|X264_CPU_SSE4},
    {"SSE4.2",  X264_CPU_MMX|X264_CPU_MMXEXT|X264_CPU_SSE|X264_CPU_SSE2|X264_CPU_SSE3|X264_CPU_SSSE3|X264_CPU_SSE4|X264_CPU_SSE42},
    {"Cache32", X264_CPU_CACHELINE_32},
    {"Cache64", X264_CPU_CACHELINE_64},
    {"SSEMisalign", X264_CPU_SSE_MISALIGN},
    {"LZCNT", X264_CPU_LZCNT},
    {"Slow_mod4_stack", X264_CPU_STACK_MOD4},
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
    if( ecx&0x00100000 )
        cpu |= X264_CPU_SSE42;

    if( cpu & X264_CPU_SSSE3 )
        cpu |= X264_CPU_SSE2_IS_FAST;
    if( cpu & X264_CPU_SSE4 )
        cpu |= X264_CPU_SHUFFLE_IS_FAST;

    x264_cpu_cpuid( 0x80000000, &eax, &ebx, &ecx, &edx );
    max_extended_cap = eax;

    if( !strcmp((char*)vendor, "AuthenticAMD") && max_extended_cap >= 0x80000001 )
    {
        x264_cpu_cpuid( 0x80000001, &eax, &ebx, &ecx, &edx );
        if( edx&0x00400000 )
            cpu |= X264_CPU_MMXEXT;
        if( cpu & X264_CPU_SSE2 )
        {
            if( ecx&0x00000040 ) /* SSE4a */
            {
                cpu |= X264_CPU_SSE2_IS_FAST;
                cpu |= X264_CPU_SSE_MISALIGN;
                cpu |= X264_CPU_LZCNT;
                cpu |= X264_CPU_SHUFFLE_IS_FAST;
                x264_cpu_mask_misalign_sse();
            }
            else
                cpu |= X264_CPU_SSE2_IS_SLOW;
        }
    }

    if( !strcmp((char*)vendor, "GenuineIntel") )
    {
        int family, model, stepping;
        x264_cpu_cpuid( 1, &eax, &ebx, &ecx, &edx );
        family = ((eax>>8)&0xf) + ((eax>>20)&0xff);
        model  = ((eax>>4)&0xf) + ((eax>>12)&0xf0);
        stepping = eax&0xf;
        /* 6/9 (pentium-m "banias"), 6/13 (pentium-m "dothan"), and 6/14 (core1 "yonah")
         * theoretically support sse2, but it's significantly slower than mmx for
         * almost all of x264's functions, so let's just pretend they don't. */
        if( family==6 && (model==9 || model==13 || model==14) )
        {
            cpu &= ~(X264_CPU_SSE2|X264_CPU_SSE3);
            assert(!(cpu&(X264_CPU_SSSE3|X264_CPU_SSE4)));
        }
    }

    if( (!strcmp((char*)vendor, "GenuineIntel") || !strcmp((char*)vendor, "CyrixInstead")) && !(cpu&X264_CPU_SSE42))
    {
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
        else if( cache == 64 )
            cpu |= X264_CPU_CACHELINE_64;
        else
            fprintf( stderr, "x264 [warning]: unable to determine cacheline size\n" );
    }

#ifdef BROKEN_STACK_ALIGNMENT
    cpu |= X264_CPU_STACK_MOD4;
#endif

    return cpu;
}

#elif defined( ARCH_PPC )

#if defined(SYS_MACOSX) || defined(SYS_OPENBSD)
#include <sys/sysctl.h>
uint32_t x264_cpu_detect( void )
{
    /* Thank you VLC */
    uint32_t cpu = 0;
#ifdef SYS_OPENBSD
    int      selectors[2] = { CTL_MACHDEP, CPU_ALTIVEC };
#else
    int      selectors[2] = { CTL_HW, HW_VECTORUNIT };
#endif
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

#elif defined(SYS_MACOSX) || defined(SYS_FREEBSD) || defined(SYS_OPENBSD)
    int numberOfCPUs;
    size_t length = sizeof( numberOfCPUs );
#ifdef SYS_OPENBSD
    int mib[2] = { CTL_HW, HW_NCPU };
    if( sysctl(mib, 2, &numberOfCPUs, &length, NULL, 0) )
#else
    if( sysctlbyname("hw.ncpu", &numberOfCPUs, &length, NULL, 0) )
#endif
    {
        numberOfCPUs = 1;
    }
    return numberOfCPUs;

#else
    return 1;
#endif
}

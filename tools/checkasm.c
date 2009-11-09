/*****************************************************************************
 * checkasm.c: assembly check tool
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

#include <ctype.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>

#include "common/common.h"
#include "common/cpu.h"

// GCC doesn't align stack variables on ARM, so use .bss
#ifdef ARCH_ARM
#undef ALIGNED_16
#define ALIGNED_16( var ) DECLARE_ALIGNED( static var, 16 )
#endif

/* buf1, buf2: initialised to random data and shouldn't write into them */
uint8_t * buf1, * buf2;
/* buf3, buf4: used to store output */
uint8_t * buf3, * buf4;

int quiet = 0;

#define report( name ) { \
    if( used_asm && !quiet ) \
        fprintf( stderr, " - %-21s [%s]\n", name, ok ? "OK" : "FAILED" ); \
    if( !ok ) ret = -1; \
}

#define BENCH_RUNS 100  // tradeoff between accuracy and speed
#define BENCH_ALIGNS 16 // number of stack+heap data alignments (another accuracy vs speed tradeoff)
#define MAX_FUNCS 1000  // just has to be big enough to hold all the existing functions
#define MAX_CPUS 10     // number of different combinations of cpu flags

typedef struct {
    void *pointer; // just for detecting duplicates
    uint32_t cpu;
    uint32_t cycles;
    uint32_t den;
} bench_t;

typedef struct {
    char *name;
    bench_t vers[MAX_CPUS];
} bench_func_t;

int do_bench = 0;
int bench_pattern_len = 0;
const char *bench_pattern = "";
char func_name[100];
static bench_func_t benchs[MAX_FUNCS];

static const char *pixel_names[10] = { "16x16", "16x8", "8x16", "8x8", "8x4", "4x8", "4x4", "4x2", "2x4", "2x2" };
static const char *intra_predict_16x16_names[7] = { "v", "h", "dc", "p", "dcl", "dct", "dc8" };
static const char *intra_predict_8x8c_names[7] = { "dc", "h", "v", "p", "dcl", "dct", "dc8" };
static const char *intra_predict_4x4_names[12] = { "v", "h", "dc", "ddl", "ddr", "vr", "hd", "vl", "hu", "dcl", "dct", "dc8" };
static const char **intra_predict_8x8_names = intra_predict_4x4_names;

#define set_func_name(...) snprintf( func_name, sizeof(func_name), __VA_ARGS__ )

static inline uint32_t read_time(void)
{
    uint32_t a = 0;
#if defined(__GNUC__) && (defined(ARCH_X86) || defined(ARCH_X86_64))
    asm volatile( "rdtsc" :"=a"(a) ::"edx" );
#elif defined(ARCH_PPC)
    asm volatile( "mftb %0" : "=r" (a) );
#elif defined(ARCH_ARM)     // ARMv7 only
    asm volatile( "mrc p15, 0, %0, c9, c13, 0" : "=r"(a) );
#endif
    return a;
}

static bench_t* get_bench( const char *name, int cpu )
{
    int i, j;
    for( i=0; benchs[i].name && strcmp(name, benchs[i].name); i++ )
        assert( i < MAX_FUNCS );
    if( !benchs[i].name )
        benchs[i].name = strdup( name );
    if( !cpu )
        return &benchs[i].vers[0];
    for( j=1; benchs[i].vers[j].cpu && benchs[i].vers[j].cpu != cpu; j++ )
        assert( j < MAX_CPUS );
    benchs[i].vers[j].cpu = cpu;
    return &benchs[i].vers[j];
}

static int cmp_nop( const void *a, const void *b )
{
    return *(uint16_t*)a - *(uint16_t*)b;
}

static int cmp_bench( const void *a, const void *b )
{
    // asciibetical sort except preserving numbers
    const char *sa = ((bench_func_t*)a)->name;
    const char *sb = ((bench_func_t*)b)->name;
    for(;; sa++, sb++)
    {
        if( !*sa && !*sb ) return 0;
        if( isdigit(*sa) && isdigit(*sb) && isdigit(sa[1]) != isdigit(sb[1]) )
            return isdigit(sa[1]) - isdigit(sb[1]);
        if( *sa != *sb ) return *sa - *sb;
    }
}

static void print_bench(void)
{
    uint16_t nops[10000] = {0};
    int i, j, k, nfuncs, nop_time=0;

    for( i=0; i<10000; i++ )
    {
        int t = read_time();
        nops[i] = read_time() - t;
    }
    qsort( nops, 10000, sizeof(uint16_t), cmp_nop );
    for( i=500; i<9500; i++ )
        nop_time += nops[i];
    nop_time /= 900;
    printf( "nop: %d\n", nop_time );

    for( i=0; i<MAX_FUNCS && benchs[i].name; i++ );
    nfuncs=i;
    qsort( benchs, nfuncs, sizeof(bench_func_t), cmp_bench );
    for( i=0; i<nfuncs; i++ )
        for( j=0; j<MAX_CPUS && (!j || benchs[i].vers[j].cpu); j++ )
        {
            bench_t *b = &benchs[i].vers[j];
            if( !b->den ) continue;
            for( k=0; k<j && benchs[i].vers[k].pointer != b->pointer; k++ );
            if( k<j ) continue;
            printf( "%s_%s%s: %"PRId64"\n", benchs[i].name,
                    b->cpu&X264_CPU_SSE4 ? "sse4" :
                    b->cpu&X264_CPU_SHUFFLE_IS_FAST ? "fastshuffle" :
                    b->cpu&X264_CPU_SSSE3 ? "ssse3" :
                    b->cpu&X264_CPU_SSE3 ? "sse3" :
                    /* print sse2slow only if there's also a sse2fast version of the same func */
                    b->cpu&X264_CPU_SSE2_IS_SLOW && j<MAX_CPUS && b[1].cpu&X264_CPU_SSE2_IS_FAST && !(b[1].cpu&X264_CPU_SSE3) ? "sse2slow" :
                    b->cpu&X264_CPU_SSE2 ? "sse2" :
                    b->cpu&X264_CPU_MMX ? "mmx" :
                    b->cpu&X264_CPU_ALTIVEC ? "altivec" :
                    b->cpu&X264_CPU_NEON ? "neon" :
                    b->cpu&X264_CPU_ARMV6 ? "armv6" : "c",
                    b->cpu&X264_CPU_CACHELINE_32 ? "_c32" :
                    b->cpu&X264_CPU_CACHELINE_64 ? "_c64" :
                    b->cpu&X264_CPU_SSE_MISALIGN ? "_misalign" :
                    b->cpu&X264_CPU_LZCNT ? "_lzcnt" :
                    b->cpu&X264_CPU_FAST_NEON_MRC ? "_fast_mrc" : "",
                    ((int64_t)10*b->cycles/b->den - nop_time)/4 );
        }
}

#if defined(ARCH_X86) || defined(ARCH_X86_64)
int x264_stack_pagealign( int (*func)(), int align );
#else
#define x264_stack_pagealign( func, align ) func()
#endif

#define call_c1(func,...) func(__VA_ARGS__)

#if defined(ARCH_X86) || defined(_WIN64)
/* detect when callee-saved regs aren't saved.
 * needs an explicit asm check because it only sometimes crashes in normal use. */
intptr_t x264_checkasm_call( intptr_t (*func)(), int *ok, ... );
#define call_a1(func,...) x264_checkasm_call((intptr_t(*)())func, &ok, __VA_ARGS__)
#else
#define call_a1 call_c1
#endif

#define call_bench(func,cpu,...)\
    if( do_bench && !strncmp(func_name, bench_pattern, bench_pattern_len) )\
    {\
        uint32_t tsum = 0;\
        int tcount = 0;\
        int ti;\
        call_a1(func, __VA_ARGS__);\
        for( ti=0; ti<(cpu?BENCH_RUNS:BENCH_RUNS/4); ti++ )\
        {\
            uint32_t t = read_time();\
            func(__VA_ARGS__);\
            func(__VA_ARGS__);\
            func(__VA_ARGS__);\
            func(__VA_ARGS__);\
            t = read_time() - t;\
            if( t*tcount <= tsum*4 && ti > 0 )\
            {\
                tsum += t;\
                tcount++;\
            }\
        }\
        bench_t *b = get_bench( func_name, cpu );\
        b->cycles += tsum;\
        b->den += tcount;\
        b->pointer = func;\
    }

/* for most functions, run benchmark and correctness test at the same time.
 * for those that modify their inputs, run the above macros separately */
#define call_a(func,...) ({ call_a2(func,__VA_ARGS__); call_a1(func,__VA_ARGS__); })
#define call_c(func,...) ({ call_c2(func,__VA_ARGS__); call_c1(func,__VA_ARGS__); })
#define call_a2(func,...) ({ call_bench(func,cpu_new,__VA_ARGS__); })
#define call_c2(func,...) ({ call_bench(func,0,__VA_ARGS__); })


static int check_pixel( int cpu_ref, int cpu_new )
{
    x264_pixel_function_t pixel_c;
    x264_pixel_function_t pixel_ref;
    x264_pixel_function_t pixel_asm;
    x264_predict_t predict_16x16[4+3];
    x264_predict_t predict_8x8c[4+3];
    x264_predict_t predict_4x4[9+3];
    x264_predict8x8_t predict_8x8[9+3];
    x264_predict_8x8_filter_t predict_8x8_filter;
    ALIGNED_16( uint8_t edge[33] );
    uint16_t cost_mv[32];
    int ret = 0, ok, used_asm;
    int i, j;

    x264_pixel_init( 0, &pixel_c );
    x264_pixel_init( cpu_ref, &pixel_ref );
    x264_pixel_init( cpu_new, &pixel_asm );
    x264_predict_16x16_init( 0, predict_16x16 );
    x264_predict_8x8c_init( 0, predict_8x8c );
    x264_predict_8x8_init( 0, predict_8x8, &predict_8x8_filter );
    x264_predict_4x4_init( 0, predict_4x4 );
    predict_8x8_filter( buf2+40, edge, ALL_NEIGHBORS, ALL_NEIGHBORS );

    // maximize sum
    for( i=0; i<256; i++ )
    {
        int z = i|(i>>4);
        z ^= z>>2;
        z ^= z>>1;
        buf3[i] = ~(buf4[i] = -(z&1));
    }
    // random pattern made of maxed pixel differences, in case an intermediate value overflows
    for( ; i<0x1000; i++ )
        buf3[i] = ~(buf4[i] = -(buf1[i&~0x88]&1));

#define TEST_PIXEL( name, align ) \
    for( i = 0, ok = 1, used_asm = 0; i < 7; i++ ) \
    { \
        int res_c, res_asm; \
        if( pixel_asm.name[i] != pixel_ref.name[i] ) \
        { \
            set_func_name( "%s_%s", #name, pixel_names[i] ); \
            used_asm = 1; \
            for( j=0; j<64; j++ ) \
            { \
                res_c   = call_c( pixel_c.name[i], buf1, 16, buf2+j*!align, 64 ); \
                res_asm = call_a( pixel_asm.name[i], buf1, 16, buf2+j*!align, 64 ); \
                if( res_c != res_asm ) \
                { \
                    ok = 0; \
                    fprintf( stderr, #name "[%d]: %d != %d [FAILED]\n", i, res_c, res_asm ); \
                    break; \
                } \
            } \
            for( j=0; j<0x1000 && ok; j+=256 ) \
            { \
                res_c   = pixel_c  .name[i]( buf3+j, 16, buf4+j, 16 ); \
                res_asm = pixel_asm.name[i]( buf3+j, 16, buf4+j, 16 ); \
                if( res_c != res_asm ) \
                { \
                    ok = 0; \
                    fprintf( stderr, #name "[%d]: overflow %d != %d\n", i, res_c, res_asm ); \
                } \
            } \
        } \
    } \
    report( "pixel " #name " :" );

    TEST_PIXEL( sad, 0 );
    TEST_PIXEL( sad_aligned, 1 );
    TEST_PIXEL( ssd, 1 );
    TEST_PIXEL( satd, 0 );
    TEST_PIXEL( sa8d, 1 );

#define TEST_PIXEL_X( N ) \
    for( i = 0, ok = 1, used_asm = 0; i < 7; i++ ) \
    { \
        int res_c[4]={0}, res_asm[4]={0}; \
        if( pixel_asm.sad_x##N[i] && pixel_asm.sad_x##N[i] != pixel_ref.sad_x##N[i] ) \
        { \
            set_func_name( "sad_x%d_%s", N, pixel_names[i] ); \
            used_asm = 1; \
            for( j=0; j<64; j++) \
            { \
                uint8_t *pix2 = buf2+j; \
                res_c[0] = pixel_c.sad[i]( buf1, 16, pix2, 64 ); \
                res_c[1] = pixel_c.sad[i]( buf1, 16, pix2+6, 64 ); \
                res_c[2] = pixel_c.sad[i]( buf1, 16, pix2+1, 64 ); \
                if(N==4) \
                { \
                    res_c[3] = pixel_c.sad[i]( buf1, 16, pix2+10, 64 ); \
                    call_a( pixel_asm.sad_x4[i], buf1, pix2, pix2+6, pix2+1, pix2+10, 64, res_asm ); \
                } \
                else \
                    call_a( pixel_asm.sad_x3[i], buf1, pix2, pix2+6, pix2+1, 64, res_asm ); \
                if( memcmp(res_c, res_asm, sizeof(res_c)) ) \
                { \
                    ok = 0; \
                    fprintf( stderr, "sad_x"#N"[%d]: %d,%d,%d,%d != %d,%d,%d,%d [FAILED]\n", \
                             i, res_c[0], res_c[1], res_c[2], res_c[3], \
                             res_asm[0], res_asm[1], res_asm[2], res_asm[3] ); \
                } \
                if(N==4) \
                    call_c2( pixel_c.sad_x4[i], buf1, pix2, pix2+6, pix2+1, pix2+10, 64, res_asm ); \
                else \
                    call_c2( pixel_c.sad_x3[i], buf1, pix2, pix2+6, pix2+1, 64, res_asm ); \
            } \
        } \
    } \
    report( "pixel sad_x"#N" :" );

    TEST_PIXEL_X(3);
    TEST_PIXEL_X(4);

#define TEST_PIXEL_VAR( i ) \
    if( pixel_asm.var[i] != pixel_ref.var[i] ) \
    { \
        int res_c, res_asm; \
        set_func_name( "%s_%s", "var", pixel_names[i] ); \
        used_asm = 1; \
        res_c   = call_c( pixel_c.var[i], buf1, 16 ); \
        res_asm = call_a( pixel_asm.var[i], buf1, 16 ); \
        if( res_c != res_asm ) \
        { \
            ok = 0; \
            fprintf( stderr, "var[%d]: %d != %d [FAILED]\n", i, res_c, res_asm ); \
        } \
    }

    ok = 1; used_asm = 0;
    TEST_PIXEL_VAR( PIXEL_16x16 );
    TEST_PIXEL_VAR( PIXEL_8x8 );
    report( "pixel var :" );

    ok = 1; used_asm = 0;
    if( pixel_asm.var2_8x8 != pixel_ref.var2_8x8 )
    {
        int res_c, res_asm, ssd_c, ssd_asm;
        set_func_name( "var2_8x8" );
        used_asm = 1;
        res_c   = call_c( pixel_c.var2_8x8, buf1, 16, buf2, 16, &ssd_c );
        res_asm = call_a( pixel_asm.var2_8x8, buf1, 16, buf2, 16, &ssd_asm );
        if( res_c != res_asm || ssd_c != ssd_asm )
        {
            ok = 0;
            fprintf( stderr, "var[%d]: %d != %d or %d != %d [FAILED]\n", i, res_c, res_asm, ssd_c, ssd_asm );
        }
    }

    report( "pixel var2 :" );

    for( i=0, ok=1, used_asm=0; i<4; i++ )
        if( pixel_asm.hadamard_ac[i] != pixel_ref.hadamard_ac[i] )
        {
            set_func_name( "hadamard_ac_%s", pixel_names[i] );
            used_asm = 1;
            for( j=0; j<32; j++ )
            {
                uint8_t *pix = (j&16 ? buf1 : buf3) + (j&15)*256;
                uint64_t rc = pixel_c.hadamard_ac[i]( pix, 16 );
                uint64_t ra = pixel_asm.hadamard_ac[i]( pix, 16 );
                if( rc != ra )
                {
                    ok = 0;
                    fprintf( stderr, "hadamard_ac[%d]: %d,%d != %d,%d\n", i, (int)rc, (int)(rc>>32), (int)ra, (int)(ra>>32) );
                    break;
                }
            }
            call_c2( pixel_c.hadamard_ac[i], buf1, 16 );
            call_a2( pixel_asm.hadamard_ac[i], buf1, 16 );
        }
    report( "pixel hadamard_ac :" );

#define TEST_INTRA_MBCMP( name, pred, satd, i8x8, ... ) \
    if( pixel_asm.name && pixel_asm.name != pixel_ref.name ) \
    { \
        int res_c[3], res_asm[3]; \
        set_func_name( #name );\
        used_asm = 1; \
        memcpy( buf3, buf2, 1024 ); \
        for( i=0; i<3; i++ ) \
        { \
            pred[i]( buf3+48, ##__VA_ARGS__ ); \
            res_c[i] = pixel_c.satd( buf1+48, 16, buf3+48, 32 ); \
        } \
        call_a( pixel_asm.name, buf1+48, i8x8 ? edge : buf3+48, res_asm ); \
        if( memcmp(res_c, res_asm, sizeof(res_c)) ) \
        { \
            ok = 0; \
            fprintf( stderr, #name": %d,%d,%d != %d,%d,%d [FAILED]\n", \
                     res_c[0], res_c[1], res_c[2], \
                     res_asm[0], res_asm[1], res_asm[2] ); \
        } \
    }

    ok = 1; used_asm = 0;
    TEST_INTRA_MBCMP( intra_satd_x3_16x16, predict_16x16, satd[PIXEL_16x16], 0 );
    TEST_INTRA_MBCMP( intra_satd_x3_8x8c , predict_8x8c , satd[PIXEL_8x8]  , 0 );
    TEST_INTRA_MBCMP( intra_satd_x3_4x4  , predict_4x4  , satd[PIXEL_4x4]  , 0 );
    TEST_INTRA_MBCMP( intra_sa8d_x3_8x8  , predict_8x8  , sa8d[PIXEL_8x8]  , 1, edge );
    report( "intra satd_x3 :" );
    TEST_INTRA_MBCMP( intra_sad_x3_16x16 , predict_16x16, sad [PIXEL_16x16], 0 );
    TEST_INTRA_MBCMP( intra_sad_x3_8x8c  , predict_8x8c , sad [PIXEL_8x8]  , 0 );
    TEST_INTRA_MBCMP( intra_sad_x3_8x8   , predict_8x8  , sad [PIXEL_8x8]  , 1, edge );
    TEST_INTRA_MBCMP( intra_sad_x3_4x4   , predict_4x4  , sad [PIXEL_4x4]  , 0 );
    report( "intra sad_x3 :" );

    if( pixel_asm.ssim_4x4x2_core != pixel_ref.ssim_4x4x2_core ||
        pixel_asm.ssim_end4 != pixel_ref.ssim_end4 )
    {
        float res_c, res_a;
        ALIGNED_16( int sums[5][4] ) = {{0}};
        used_asm = ok = 1;
        x264_emms();
        res_c = x264_pixel_ssim_wxh( &pixel_c,   buf1+2, 32, buf2+2, 32, 32, 28, buf3 );
        res_a = x264_pixel_ssim_wxh( &pixel_asm, buf1+2, 32, buf2+2, 32, 32, 28, buf3 );
        if( fabs(res_c - res_a) > 1e-6 )
        {
            ok = 0;
            fprintf( stderr, "ssim: %.7f != %.7f [FAILED]\n", res_c, res_a );
        }
        set_func_name( "ssim_core" );
        call_c2( pixel_c.ssim_4x4x2_core,   buf1+2, 32, buf2+2, 32, sums );
        call_a2( pixel_asm.ssim_4x4x2_core, buf1+2, 32, buf2+2, 32, sums );
        set_func_name( "ssim_end" );
        call_c2( pixel_c.ssim_end4,   sums, sums, 4 );
        call_a2( pixel_asm.ssim_end4, sums, sums, 4 );
        report( "ssim :" );
    }

    ok = 1; used_asm = 0;
    for( i=0; i<32; i++ )
        cost_mv[i] = i*10;
    for( i=0; i<100 && ok; i++ )
        if( pixel_asm.ads[i&3] != pixel_ref.ads[i&3] )
        {
            ALIGNED_16( uint16_t sums[72] );
            ALIGNED_16( int dc[4] );
            int16_t mvs_a[32], mvs_c[32];
            int mvn_a, mvn_c;
            int thresh = rand() & 0x3fff;
            set_func_name( "esa_ads" );
            for( j=0; j<72; j++ )
                sums[j] = rand() & 0x3fff;
            for( j=0; j<4; j++ )
                dc[j] = rand() & 0x3fff;
            used_asm = 1;
            mvn_c = call_c( pixel_c.ads[i&3], dc, sums, 32, cost_mv, mvs_c, 28, thresh );
            mvn_a = call_a( pixel_asm.ads[i&3], dc, sums, 32, cost_mv, mvs_a, 28, thresh );
            if( mvn_c != mvn_a || memcmp( mvs_c, mvs_a, mvn_c*sizeof(*mvs_c) ) )
            {
                ok = 0;
                printf("c%d: ", i&3);
                for(j=0; j<mvn_c; j++)
                    printf("%d ", mvs_c[j]);
                printf("\na%d: ", i&3);
                for(j=0; j<mvn_a; j++)
                    printf("%d ", mvs_a[j]);
                printf("\n\n");
            }
        }
    report( "esa ads:" );

    return ret;
}

static int check_dct( int cpu_ref, int cpu_new )
{
    x264_dct_function_t dct_c;
    x264_dct_function_t dct_ref;
    x264_dct_function_t dct_asm;
    x264_quant_function_t qf;
    int ret = 0, ok, used_asm, i, j, interlace;
    ALIGNED_16( int16_t dct1[16][16] );
    ALIGNED_16( int16_t dct2[16][16] );
    ALIGNED_16( int16_t dct4[16][16] );
    ALIGNED_16( int16_t dct8[4][64] );
    ALIGNED_8( int16_t dctdc[2][4] );
    x264_t h_buf;
    x264_t *h = &h_buf;

    x264_dct_init( 0, &dct_c );
    x264_dct_init( cpu_ref, &dct_ref);
    x264_dct_init( cpu_new, &dct_asm );

    memset( h, 0, sizeof(*h) );
    h->pps = h->pps_array;
    x264_param_default( &h->param );
    h->chroma_qp_table = i_chroma_qp_table + 12;
    h->param.analyse.i_luma_deadzone[0] = 0;
    h->param.analyse.i_luma_deadzone[1] = 0;
    h->param.analyse.b_transform_8x8 = 1;
    for( i=0; i<6; i++ )
        h->pps->scaling_list[i] = x264_cqm_flat16;
    x264_cqm_init( h );
    x264_quant_init( h, 0, &qf );

#define TEST_DCT( name, t1, t2, size ) \
    if( dct_asm.name != dct_ref.name ) \
    { \
        set_func_name( #name );\
        used_asm = 1; \
        call_c( dct_c.name, t1, buf1, buf2 ); \
        call_a( dct_asm.name, t2, buf1, buf2 ); \
        if( memcmp( t1, t2, size ) ) \
        { \
            ok = 0; \
            fprintf( stderr, #name " [FAILED]\n" ); \
        } \
    }
    ok = 1; used_asm = 0;
    TEST_DCT( sub4x4_dct, dct1[0], dct2[0], 16*2 );
    TEST_DCT( sub8x8_dct, dct1, dct2, 16*2*4 );
    TEST_DCT( sub8x8_dct_dc, dctdc[0], dctdc[1], 4*2 );
    TEST_DCT( sub16x16_dct, dct1, dct2, 16*2*16 );
    report( "sub_dct4 :" );

    ok = 1; used_asm = 0;
    TEST_DCT( sub8x8_dct8, (void*)dct1[0], (void*)dct2[0], 64*2 );
    TEST_DCT( sub16x16_dct8, (void*)dct1, (void*)dct2, 64*2*4 );
    report( "sub_dct8 :" );
#undef TEST_DCT

    // fdct and idct are denormalized by different factors, so quant/dequant
    // is needed to force the coefs into the right range.
    dct_c.sub16x16_dct( dct4, buf1, buf2 );
    dct_c.sub16x16_dct8( dct8, buf1, buf2 );
    for( i=0; i<16; i++ )
    {
        qf.quant_4x4( dct4[i], h->quant4_mf[CQM_4IY][20], h->quant4_bias[CQM_4IY][20] );
        qf.dequant_4x4( dct4[i], h->dequant4_mf[CQM_4IY], 20 );
    }
    for( i=0; i<4; i++ )
    {
        qf.quant_8x8( dct8[i], h->quant8_mf[CQM_8IY][20], h->quant8_bias[CQM_8IY][20] );
        qf.dequant_8x8( dct8[i], h->dequant8_mf[CQM_8IY], 20 );
    }

#define TEST_IDCT( name, src ) \
    if( dct_asm.name != dct_ref.name ) \
    { \
        set_func_name( #name );\
        used_asm = 1; \
        memcpy( buf3, buf1, 32*32 ); \
        memcpy( buf4, buf1, 32*32 ); \
        memcpy( dct1, src, 512 ); \
        memcpy( dct2, src, 512 ); \
        call_c1( dct_c.name, buf3, (void*)dct1 ); \
        call_a1( dct_asm.name, buf4, (void*)dct2 ); \
        if( memcmp( buf3, buf4, 32*32 ) ) \
        { \
            ok = 0; \
            fprintf( stderr, #name " [FAILED]\n" ); \
        } \
        call_c2( dct_c.name, buf3, (void*)dct1 ); \
        call_a2( dct_asm.name, buf4, (void*)dct2 ); \
    }
    ok = 1; used_asm = 0;
    TEST_IDCT( add4x4_idct, dct4 );
    TEST_IDCT( add8x8_idct, dct4 );
    TEST_IDCT( add8x8_idct_dc, dct4 );
    TEST_IDCT( add16x16_idct, dct4 );
    TEST_IDCT( add16x16_idct_dc, dct4 );
    report( "add_idct4 :" );

    ok = 1; used_asm = 0;
    TEST_IDCT( add8x8_idct8, dct8 );
    TEST_IDCT( add16x16_idct8, dct8 );
    report( "add_idct8 :" );
#undef TEST_IDCT

#define TEST_DCTDC( name )\
    ok = 1; used_asm = 0;\
    if( dct_asm.name != dct_ref.name )\
    {\
        set_func_name( #name );\
        used_asm = 1;\
        uint16_t *p = (uint16_t*)buf1;\
        for( i=0; i<16 && ok; i++ )\
        {\
            for( j=0; j<16; j++ )\
                dct1[0][j] = !i ? (j^j>>1^j>>2^j>>3)&1 ? 4080 : -4080 /* max dc */\
                           : i<8 ? (*p++)&1 ? 4080 : -4080 /* max elements */\
                           : ((*p++)&0x1fff)-0x1000; /* general case */\
            memcpy( dct2, dct1, 32 );\
            call_c1( dct_c.name, dct1[0] );\
            call_a1( dct_asm.name, dct2[0] );\
            if( memcmp( dct1, dct2, 32 ) )\
                ok = 0;\
        }\
        call_c2( dct_c.name, dct1[0] );\
        call_a2( dct_asm.name, dct2[0] );\
    }\
    report( #name " :" );

    TEST_DCTDC(  dct4x4dc );
    TEST_DCTDC( idct4x4dc );
#undef TEST_DCTDC

    x264_zigzag_function_t zigzag_c;
    x264_zigzag_function_t zigzag_ref;
    x264_zigzag_function_t zigzag_asm;

    ALIGNED_16( int16_t level1[64] );
    ALIGNED_16( int16_t level2[64] );

#define TEST_ZIGZAG_SCAN( name, t1, t2, dct, size )   \
    if( zigzag_asm.name != zigzag_ref.name ) \
    { \
        set_func_name( "zigzag_"#name"_%s", interlace?"field":"frame" );\
        used_asm = 1; \
        memcpy(dct, buf1, size*sizeof(int16_t));\
        call_c( zigzag_c.name, t1, dct ); \
        call_a( zigzag_asm.name, t2, dct ); \
        if( memcmp( t1, t2, size*sizeof(int16_t) ) ) \
        { \
            ok = 0; \
            fprintf( stderr, #name " [FAILED]\n" ); \
        } \
    }

#define TEST_ZIGZAG_SUB( name, t1, t2, size ) \
    if( zigzag_asm.name != zigzag_ref.name ) \
    { \
        int nz_a, nz_c; \
        set_func_name( "zigzag_"#name"_%s", interlace?"field":"frame" );\
        used_asm = 1; \
        memcpy( buf3, buf1, 16*FDEC_STRIDE ); \
        memcpy( buf4, buf1, 16*FDEC_STRIDE ); \
        nz_c = call_c1( zigzag_c.name, t1, buf2, buf3 );  \
        nz_a = call_a1( zigzag_asm.name, t2, buf2, buf4 ); \
        if( memcmp( t1, t2, size*sizeof(int16_t) )|| memcmp( buf3, buf4, 16*FDEC_STRIDE ) || nz_c != nz_a )  \
        { \
            ok = 0; \
            fprintf( stderr, #name " [FAILED]\n" ); \
        } \
        call_c2( zigzag_c.name, t1, buf2, buf3 );  \
        call_a2( zigzag_asm.name, t2, buf2, buf4 ); \
    }

#define TEST_ZIGZAG_SUBAC( name, t1, t2 ) \
    if( zigzag_asm.name != zigzag_ref.name ) \
    { \
        int nz_a, nz_c; \
        int16_t dc_a, dc_c; \
        set_func_name( "zigzag_"#name"_%s", interlace?"field":"frame" );\
        used_asm = 1; \
        for( i = 0; i < 2; i++ ) \
        { \
            memcpy( buf3, buf2, 16*FDEC_STRIDE ); \
            memcpy( buf4, buf2, 16*FDEC_STRIDE ); \
            for( j = 0; j < 4; j++ ) \
            { \
                memcpy( buf3 + j*FDEC_STRIDE, (i?buf1:buf2) + j*FENC_STRIDE, 4 ); \
                memcpy( buf4 + j*FDEC_STRIDE, (i?buf1:buf2) + j*FENC_STRIDE, 4 ); \
            } \
            nz_c = call_c1( zigzag_c.name, t1, buf2, buf3, &dc_c );  \
            nz_a = call_a1( zigzag_asm.name, t2, buf2, buf4, &dc_a ); \
            if( memcmp( t1+1, t2+1, 15*sizeof(int16_t) ) || memcmp( buf3, buf4, 16*FDEC_STRIDE ) || nz_c != nz_a || dc_c != dc_a )  \
            { \
                ok = 0; \
                fprintf( stderr, #name " [FAILED]\n" ); \
                break; \
            } \
        } \
        call_c2( zigzag_c.name, t1, buf2, buf3, &dc_c );  \
        call_a2( zigzag_asm.name, t2, buf2, buf4, &dc_a ); \
    }

#define TEST_INTERLEAVE( name, t1, t2, dct, size )   \
    if( zigzag_asm.name != zigzag_ref.name ) \
    { \
        for( j=0; j<100; j++ ) \
        { \
            set_func_name( "zigzag_"#name"_%s", interlace?"field":"frame" );\
            used_asm = 1; \
            memcpy(dct, buf1, size*sizeof(int16_t));\
            for( i=0; i<size; i++ ) \
                dct[i] = rand()&0x1F ? 0 : dct[i]; \
            memcpy(buf3, buf4, 10*sizeof(uint8_t)); \
            call_c( zigzag_c.name, t1, dct, buf3 ); \
            call_a( zigzag_asm.name, t2, dct, buf4 ); \
            if( memcmp( t1, t2, size*sizeof(int16_t) ) || memcmp( buf3, buf4, 10*sizeof(uint8_t) ) ) \
            { \
                ok = 0; \
            } \
        } \
    }

    interlace = 0;
    x264_zigzag_init( 0, &zigzag_c, 0 );
    x264_zigzag_init( cpu_ref, &zigzag_ref, 0 );
    x264_zigzag_init( cpu_new, &zigzag_asm, 0 );

    ok = 1; used_asm = 0;
    TEST_ZIGZAG_SCAN( scan_8x8, level1, level2, (void*)dct1, 64 );
    TEST_ZIGZAG_SCAN( scan_4x4, level1, level2, dct1[0], 16  );
    TEST_ZIGZAG_SUB( sub_4x4, level1, level2, 16 );
    TEST_ZIGZAG_SUBAC( sub_4x4ac, level1, level2 );
    report( "zigzag_frame :" );

    interlace = 1;
    x264_zigzag_init( 0, &zigzag_c, 1 );
    x264_zigzag_init( cpu_ref, &zigzag_ref, 1 );
    x264_zigzag_init( cpu_new, &zigzag_asm, 1 );

    ok = 1; used_asm = 0;
    TEST_ZIGZAG_SCAN( scan_8x8, level1, level2, (void*)dct1, 64 );
    TEST_ZIGZAG_SCAN( scan_4x4, level1, level2, dct1[0], 16  );
    TEST_ZIGZAG_SUB( sub_4x4, level1, level2, 16 );
    TEST_ZIGZAG_SUBAC( sub_4x4ac, level1, level2 );
    report( "zigzag_field :" );

    ok = 1; used_asm = 0;
    TEST_INTERLEAVE( interleave_8x8_cavlc, level1, level2, dct1[0], 64 );
    report( "zigzag_interleave :" );
#undef TEST_ZIGZAG_SCAN
#undef TEST_ZIGZAG_SUB

    return ret;
}

static int check_mc( int cpu_ref, int cpu_new )
{
    x264_mc_functions_t mc_c;
    x264_mc_functions_t mc_ref;
    x264_mc_functions_t mc_a;
    x264_pixel_function_t pixel;

    uint8_t *src     = &buf1[2*64+2];
    uint8_t *src2[4] = { &buf1[3*64+2], &buf1[5*64+2],
                         &buf1[7*64+2], &buf1[9*64+2] };
    uint8_t *dst1    = buf3;
    uint8_t *dst2    = buf4;

    int dx, dy, i, j, k, w;
    int ret = 0, ok, used_asm;

    x264_mc_init( 0, &mc_c );
    x264_mc_init( cpu_ref, &mc_ref );
    x264_mc_init( cpu_new, &mc_a );
    x264_pixel_init( 0, &pixel );

#define MC_TEST_LUMA( w, h ) \
        if( mc_a.mc_luma != mc_ref.mc_luma && !(w&(w-1)) && h<=16 ) \
        { \
            const x264_weight_t *weight = weight_none; \
            set_func_name( "mc_luma_%dx%d", w, h );\
            used_asm = 1; \
            memset(buf3, 0xCD, 1024); \
            memset(buf4, 0xCD, 1024); \
            call_c( mc_c.mc_luma, dst1, 32, src2, 64, dx, dy, w, h, weight ); \
            call_a( mc_a.mc_luma, dst2, 32, src2, 64, dx, dy, w, h, weight ); \
            if( memcmp( buf3, buf4, 1024 ) ) \
            { \
                fprintf( stderr, "mc_luma[mv(%d,%d) %2dx%-2d]     [FAILED]\n", dx, dy, w, h ); \
                ok = 0; \
            } \
        } \
        if( mc_a.get_ref != mc_ref.get_ref ) \
        { \
            uint8_t *ref = dst2; \
            int ref_stride = 32; \
            const x264_weight_t *weight = weight_none; \
            set_func_name( "get_ref_%dx%d", w, h );\
            used_asm = 1; \
            memset(buf3, 0xCD, 1024); \
            memset(buf4, 0xCD, 1024); \
            call_c( mc_c.mc_luma, dst1, 32, src2, 64, dx, dy, w, h, weight ); \
            ref = (uint8_t*) call_a( mc_a.get_ref, ref, &ref_stride, src2, 64, dx, dy, w, h, weight ); \
            for( i=0; i<h; i++ ) \
                if( memcmp( dst1+i*32, ref+i*ref_stride, w ) ) \
                { \
                    fprintf( stderr, "get_ref[mv(%d,%d) %2dx%-2d]     [FAILED]\n", dx, dy, w, h ); \
                    ok = 0; \
                    break; \
                } \
        }

#define MC_TEST_CHROMA( w, h ) \
        if( mc_a.mc_chroma != mc_ref.mc_chroma ) \
        { \
            set_func_name( "mc_chroma_%dx%d", w, h );\
            used_asm = 1; \
            memset(buf3, 0xCD, 1024); \
            memset(buf4, 0xCD, 1024); \
            call_c( mc_c.mc_chroma, dst1, 16, src, 64, dx, dy, w, h ); \
            call_a( mc_a.mc_chroma, dst2, 16, src, 64, dx, dy, w, h ); \
            /* mc_chroma width=2 may write garbage to the right of dst. ignore that. */\
            for( j=0; j<h; j++ ) \
                for( i=w; i<4; i++ ) \
                    dst2[i+j*16] = dst1[i+j*16]; \
            if( memcmp( buf3, buf4, 1024 ) ) \
            { \
                fprintf( stderr, "mc_chroma[mv(%d,%d) %2dx%-2d]     [FAILED]\n", dx, dy, w, h ); \
                ok = 0; \
            } \
        }
    ok = 1; used_asm = 0;
    for( dy = -8; dy < 8; dy++ )
        for( dx = -128; dx < 128; dx++ )
        {
            if( rand()&15 ) continue; // running all of them is too slow
            MC_TEST_LUMA( 20, 18 );
            MC_TEST_LUMA( 16, 16 );
            MC_TEST_LUMA( 16, 8 );
            MC_TEST_LUMA( 12, 10 );
            MC_TEST_LUMA( 8, 16 );
            MC_TEST_LUMA( 8, 8 );
            MC_TEST_LUMA( 8, 4 );
            MC_TEST_LUMA( 4, 8 );
            MC_TEST_LUMA( 4, 4 );
        }
    report( "mc luma :" );

    ok = 1; used_asm = 0;
    for( dy = -1; dy < 9; dy++ )
        for( dx = -128; dx < 128; dx++ )
        {
            if( rand()&15 ) continue;
            MC_TEST_CHROMA( 8, 8 );
            MC_TEST_CHROMA( 8, 4 );
            MC_TEST_CHROMA( 4, 8 );
            MC_TEST_CHROMA( 4, 4 );
            MC_TEST_CHROMA( 4, 2 );
            MC_TEST_CHROMA( 2, 4 );
            MC_TEST_CHROMA( 2, 2 );
        }
    report( "mc chroma :" );
#undef MC_TEST_LUMA
#undef MC_TEST_CHROMA

#define MC_TEST_AVG( name, weight ) \
    for( i = 0, ok = 1, used_asm = 0; i < 10; i++ ) \
    { \
        memcpy( buf3, buf1+320, 320 ); \
        memcpy( buf4, buf1+320, 320 ); \
        if( mc_a.name[i] != mc_ref.name[i] ) \
        { \
            set_func_name( "%s_%s", #name, pixel_names[i] );\
            used_asm = 1; \
            call_c1( mc_c.name[i], buf3, 16, buf2+1, 16, buf1+18, 16, weight ); \
            call_a1( mc_a.name[i], buf4, 16, buf2+1, 16, buf1+18, 16, weight ); \
            if( memcmp( buf3, buf4, 320 ) ) \
            { \
                ok = 0; \
                fprintf( stderr, #name "[%d]: [FAILED]\n", i ); \
            } \
            call_c2( mc_c.name[i], buf3, 16, buf2+1, 16, buf1+18, 16, weight ); \
            call_a2( mc_a.name[i], buf4, 16, buf2+1, 16, buf1+18, 16, weight ); \
        } \
    }
    ok = 1; used_asm = 0;
    for( w = -63; w <= 127 && ok; w++ )
        MC_TEST_AVG( avg, w );
    report( "mc wpredb :" );

#define MC_TEST_WEIGHT( name, weight, aligned ) \
    int align_off = (aligned ? 0 : rand()%16); \
    for( i = 1, ok = 1, used_asm = 0; i <= 5; i++ ) \
    { \
        ALIGNED_16( uint8_t buffC[640] ); \
        ALIGNED_16( uint8_t buffA[640] ); \
        j = X264_MAX( i*4, 2 ); \
        memset( buffC, 0, 640 ); \
        memset( buffA, 0, 640 ); \
        x264_t ha; \
        ha.mc = mc_a; \
        /* w12 is the same as w16 in some cases */ \
        if( i == 3 && mc_a.name[i] == mc_a.name[i+1] ) \
            continue; \
        if( mc_a.name[i] != mc_ref.name[i] ) \
        { \
            int k; \
            set_func_name( "%s_w%d", #name, j ); \
            used_asm = 1; \
            call_c1( mc_c.weight[i], buffC, 32, buf2+align_off, 32, &weight, 16 ); \
            mc_a.weight_cache(&ha, &weight); \
            call_a1( weight.weightfn[i], buffA, 32, buf2+align_off, 32, &weight, 16 ); \
            for( k = 0; k < 16; k++ ) \
                if( memcmp( &buffC[k*32], &buffA[k*32], j ) ) \
                { \
                    ok = 0; \
                    fprintf( stderr, #name "[%d]: [FAILED] s:%d o:%d d%d\n", i, s, o, d ); \
                    break; \
                } \
            call_c2( mc_c.weight[i], buffC, 32, buf2+align_off, 32, &weight, 16 ); \
            call_a2( weight.weightfn[i], buffA, 32, buf2+align_off, 32, &weight, 16 ); \
        } \
    }

    ok = 1; used_asm = 0;

    int s,o,d;
    int align_cnt = 0;
    for( s = 0; s <= 127 && ok; s++ )
    {
        for( o = -128; o <= 127 && ok; o++ )
        {
            if( rand() & 2047 ) continue;
            for( d = 0; d <= 7 && ok; d++ )
            {
                if( s == 1<<d )
                    continue;
                x264_weight_t weight = { .i_scale = s, .i_denom = d, .i_offset = o };
                MC_TEST_WEIGHT( weight, weight, (align_cnt++ % 4) );
            }
        }

    }
    report( "mc weight :" );

    ok = 1; used_asm = 0;
    s = 1; d = 0;
    for( o = 0; o <= 127 && ok; o++ )
    {
        if( rand() & 15 ) continue;
        x264_weight_t weight = { .i_scale = 1, .i_denom = 0, .i_offset = o };
        MC_TEST_WEIGHT( offsetadd, weight, (align_cnt++ % 4) );
    }
    report( "mc offsetadd :" );
    ok = 1; used_asm = 0;
    for( o = -128; o < 0 && ok; o++ )
    {
        if( rand() & 15 ) continue;
        x264_weight_t weight = { .i_scale = 1, .i_denom = 0, .i_offset = o };
        MC_TEST_WEIGHT( offsetsub, weight, (align_cnt++ % 4) );
    }
    report( "mc offsetsub :" );

    if( mc_a.hpel_filter != mc_ref.hpel_filter )
    {
        uint8_t *src = buf1+8+2*64;
        uint8_t *dstc[3] = { buf3+8, buf3+8+16*64, buf3+8+32*64 };
        uint8_t *dsta[3] = { buf4+8, buf4+8+16*64, buf4+8+32*64 };
        void *tmp = buf3+49*64;
        set_func_name( "hpel_filter" );
        ok = 1; used_asm = 1;
        memset( buf3, 0, 4096 );
        memset( buf4, 0, 4096 );
        call_c( mc_c.hpel_filter, dstc[0], dstc[1], dstc[2], src, 64, 48, 10, tmp );
        call_a( mc_a.hpel_filter, dsta[0], dsta[1], dsta[2], src, 64, 48, 10, tmp );
        for( i=0; i<3; i++ )
            for( j=0; j<10; j++ )
                //FIXME ideally the first pixels would match too, but they aren't actually used
                if( memcmp( dstc[i]+j*64+2, dsta[i]+j*64+2, 43 ) )
                {
                    ok = 0;
                    fprintf( stderr, "hpel filter differs at plane %c line %d\n", "hvc"[i], j );
                    for( k=0; k<48; k++ )
                        printf("%02x%s", dstc[i][j*64+k], (k+1)&3 ? "" : " ");
                    printf("\n");
                    for( k=0; k<48; k++ )
                        printf("%02x%s", dsta[i][j*64+k], (k+1)&3 ? "" : " ");
                    printf("\n");
                    break;
                }
        report( "hpel filter :" );
    }

    if( mc_a.frame_init_lowres_core != mc_ref.frame_init_lowres_core )
    {
        uint8_t *dstc[4] = { buf3, buf3+1024, buf3+2048, buf3+3072 };
        uint8_t *dsta[4] = { buf4, buf4+1024, buf4+2048, buf4+3072 };
        set_func_name( "lowres_init" );
        ok = 1; used_asm = 1;
        for( w=40; w<=48; w+=8 )
        {
            int stride = (w+8)&~15;
            call_c( mc_c.frame_init_lowres_core, buf1, dstc[0], dstc[1], dstc[2], dstc[3], w*2, stride, w, 16 );
            call_a( mc_a.frame_init_lowres_core, buf1, dsta[0], dsta[1], dsta[2], dsta[3], w*2, stride, w, 16 );
            for( i=0; i<16; i++)
            {
                for( j=0; j<4; j++)
                    if( memcmp( dstc[j]+i*stride, dsta[j]+i*stride, w ) )
                    {
                        ok = 0;
                        fprintf( stderr, "frame_init_lowres differs at plane %d line %d\n", j, i );
                        for( k=0; k<w; k++ )
                            printf( "%d ", dstc[j][k+i*stride] );
                        printf("\n");
                        for( k=0; k<w; k++ )
                            printf( "%d ", dsta[j][k+i*stride] );
                        printf("\n");
                        break;
                    }
            }
        }
        report( "lowres init :" );
    }

#define INTEGRAL_INIT( name, size, ... )\
    if( mc_a.name != mc_ref.name )\
    {\
        int stride = 80;\
        set_func_name( #name );\
        used_asm = 1;\
        memcpy( buf3, buf1, size*2*stride );\
        memcpy( buf4, buf1, size*2*stride );\
        uint16_t *sum = (uint16_t*)buf3;\
        call_c1( mc_c.name, __VA_ARGS__ );\
        sum = (uint16_t*)buf4;\
        call_a1( mc_a.name, __VA_ARGS__ );\
        if( memcmp( buf3, buf4, (stride-8)*2 )\
            || (size>9 && memcmp( buf3+18*stride, buf4+18*stride, (stride-8)*2 )))\
            ok = 0;\
        call_c2( mc_c.name, __VA_ARGS__ );\
        call_a2( mc_a.name, __VA_ARGS__ );\
    }
    ok = 1; used_asm = 0;
    INTEGRAL_INIT( integral_init4h, 2, sum+stride, buf2, stride );
    INTEGRAL_INIT( integral_init8h, 2, sum+stride, buf2, stride );
    INTEGRAL_INIT( integral_init4v, 14, sum, sum+9*stride, stride );
    INTEGRAL_INIT( integral_init8v, 9, sum, stride );
    report( "integral init :" );

    if( mc_a.mbtree_propagate_cost != mc_ref.mbtree_propagate_cost )
    {
        ok = 1; used_asm = 1;
        set_func_name( "mbtree_propagate" );
        int *dsta = (int*)buf3;
        int *dstc = dsta+400;
        uint16_t *prop = (uint16_t*)buf1;
        uint16_t *intra = (uint16_t*)buf4;
        uint16_t *inter = intra+400;
        uint16_t *qscale = inter+400;
        uint16_t *rand = (uint16_t*)buf2;
        x264_emms();
        for( i=0; i<400; i++ )
        {
            intra[i]  = *rand++ & 0x7fff;
            intra[i] += !intra[i];
            inter[i]  = *rand++ & 0x7fff;
            qscale[i] = *rand++ & 0x7fff;
        }
        call_c( mc_c.mbtree_propagate_cost, dstc, prop, intra, inter, qscale, 400 );
        call_a( mc_a.mbtree_propagate_cost, dsta, prop, intra, inter, qscale, 400 );
        // I don't care about exact rounding, this is just how close the floating-point implementation happens to be
        x264_emms();
        for( i=0; i<400; i++ )
            ok &= abs(dstc[i]-dsta[i]) <= (abs(dstc[i])>512) || fabs((double)dstc[i]/dsta[i]-1) < 1e-6;
        report( "mbtree propagate :" );
    }

    return ret;
}

static int check_deblock( int cpu_ref, int cpu_new )
{
    x264_deblock_function_t db_c;
    x264_deblock_function_t db_ref;
    x264_deblock_function_t db_a;
    int ret = 0, ok = 1, used_asm = 0;
    int alphas[36], betas[36];
    int8_t tcs[36][4];
    int a, c, i, j;

    x264_deblock_init( 0, &db_c );
    x264_deblock_init( cpu_ref, &db_ref );
    x264_deblock_init( cpu_new, &db_a );

    /* not exactly the real values of a,b,tc but close enough */
    a = 255; c = 250;
    for( i = 35; i >= 0; i-- )
    {
        alphas[i] = a;
        betas[i] = (i+1)/2;
        tcs[i][0] = tcs[i][2] = (c+6)/10;
        tcs[i][1] = tcs[i][3] = (c+9)/20;
        a = a*9/10;
        c = c*9/10;
    }

#define TEST_DEBLOCK( name, align, ... ) \
    for( i = 0; i < 36; i++ ) \
    { \
        int off = 8*32 + (i&15)*4*!align; /* benchmark various alignments of h filter */\
        for( j = 0; j < 1024; j++ ) \
            /* two distributions of random to excersize different failure modes */\
            buf3[j] = rand() & (i&1 ? 0xf : 0xff ); \
        memcpy( buf4, buf3, 1024 ); \
        if( db_a.name != db_ref.name ) \
        { \
            set_func_name( #name );\
            used_asm = 1; \
            call_c1( db_c.name, buf3+off, 32, alphas[i], betas[i], ##__VA_ARGS__ ); \
            call_a1( db_a.name, buf4+off, 32, alphas[i], betas[i], ##__VA_ARGS__ ); \
            if( memcmp( buf3, buf4, 1024 ) ) \
            { \
                ok = 0; \
                fprintf( stderr, #name "(a=%d, b=%d): [FAILED]\n", alphas[i], betas[i] ); \
                break; \
            } \
            call_c2( db_c.name, buf3+off, 32, alphas[i], betas[i], ##__VA_ARGS__ ); \
            call_a2( db_a.name, buf4+off, 32, alphas[i], betas[i], ##__VA_ARGS__ ); \
        } \
    }

    TEST_DEBLOCK( deblock_h_luma, 0, tcs[i] );
    TEST_DEBLOCK( deblock_v_luma, 1, tcs[i] );
    TEST_DEBLOCK( deblock_h_chroma, 0, tcs[i] );
    TEST_DEBLOCK( deblock_v_chroma, 1, tcs[i] );
    TEST_DEBLOCK( deblock_h_luma_intra, 0 );
    TEST_DEBLOCK( deblock_v_luma_intra, 1 );
    TEST_DEBLOCK( deblock_h_chroma_intra, 0 );
    TEST_DEBLOCK( deblock_v_chroma_intra, 1 );

    report( "deblock :" );

    return ret;
}

static int check_quant( int cpu_ref, int cpu_new )
{
    x264_quant_function_t qf_c;
    x264_quant_function_t qf_ref;
    x264_quant_function_t qf_a;
    ALIGNED_16( int16_t dct1[64] );
    ALIGNED_16( int16_t dct2[64] );
    ALIGNED_16( uint8_t cqm_buf[64] );
    int ret = 0, ok, used_asm;
    int oks[2] = {1,1}, used_asms[2] = {0,0};
    int i, j, i_cqm, qp;
    x264_t h_buf;
    x264_t *h = &h_buf;
    memset( h, 0, sizeof(*h) );
    h->pps = h->pps_array;
    x264_param_default( &h->param );
    h->chroma_qp_table = i_chroma_qp_table + 12;
    h->param.rc.i_qp_min = 26;
    h->param.analyse.b_transform_8x8 = 1;

    for( i_cqm = 0; i_cqm < 4; i_cqm++ )
    {
        if( i_cqm == 0 )
        {
            for( i = 0; i < 6; i++ )
                h->pps->scaling_list[i] = x264_cqm_flat16;
            h->param.i_cqm_preset = h->pps->i_cqm_preset = X264_CQM_FLAT;
        }
        else if( i_cqm == 1 )
        {
            for( i = 0; i < 6; i++ )
                h->pps->scaling_list[i] = x264_cqm_jvt[i];
            h->param.i_cqm_preset = h->pps->i_cqm_preset = X264_CQM_JVT;
        }
        else
        {
            if( i_cqm == 2 )
                for( i = 0; i < 64; i++ )
                    cqm_buf[i] = 10 + rand() % 246;
            else
                for( i = 0; i < 64; i++ )
                    cqm_buf[i] = 1;
            for( i = 0; i < 6; i++ )
                h->pps->scaling_list[i] = cqm_buf;
            h->param.i_cqm_preset = h->pps->i_cqm_preset = X264_CQM_CUSTOM;
        }

        x264_cqm_init( h );
        x264_quant_init( h, 0, &qf_c );
        x264_quant_init( h, cpu_ref, &qf_ref );
        x264_quant_init( h, cpu_new, &qf_a );

#define INIT_QUANT8() \
        { \
            static const int scale1d[8] = {32,31,24,31,32,31,24,31}; \
            for( i = 0; i < 64; i++ ) \
            { \
                unsigned int scale = (255*scale1d[i>>3]*scale1d[i&7])/16; \
                dct1[i] = dct2[i] = j ? (rand()%(2*scale+1))-scale : 0; \
            } \
        }

#define INIT_QUANT4() \
        { \
            static const int scale1d[4] = {4,6,4,6}; \
            for( i = 0; i < 16; i++ ) \
            { \
                unsigned int scale = 255*scale1d[i>>2]*scale1d[i&3]; \
                dct1[i] = dct2[i] = j ? (rand()%(2*scale+1))-scale : 0; \
            } \
        }

#define TEST_QUANT_DC( name, cqm ) \
        if( qf_a.name != qf_ref.name ) \
        { \
            set_func_name( #name ); \
            used_asms[0] = 1; \
            for( qp = 51; qp > 0; qp-- ) \
            { \
                for( j = 0; j < 2; j++ ) \
                { \
                    int result_c, result_a; \
                    for( i = 0; i < 16; i++ ) \
                        dct1[i] = dct2[i] = j ? (rand() & 0x1fff) - 0xfff : 0; \
                    result_c = call_c1( qf_c.name, dct1, h->quant4_mf[CQM_4IY][qp][0], h->quant4_bias[CQM_4IY][qp][0] ); \
                    result_a = call_a1( qf_a.name, dct2, h->quant4_mf[CQM_4IY][qp][0], h->quant4_bias[CQM_4IY][qp][0] ); \
                    if( memcmp( dct1, dct2, 16*2 ) || result_c != result_a )       \
                    { \
                        oks[0] = 0; \
                        fprintf( stderr, #name "(cqm=%d): [FAILED]\n", i_cqm ); \
                        break; \
                    } \
                    call_c2( qf_c.name, dct1, h->quant4_mf[CQM_4IY][qp][0], h->quant4_bias[CQM_4IY][qp][0] ); \
                    call_a2( qf_a.name, dct2, h->quant4_mf[CQM_4IY][qp][0], h->quant4_bias[CQM_4IY][qp][0] ); \
                } \
            } \
        }

#define TEST_QUANT( qname, block, w ) \
        if( qf_a.qname != qf_ref.qname ) \
        { \
            set_func_name( #qname ); \
            used_asms[0] = 1; \
            for( qp = 51; qp > 0; qp-- ) \
            { \
                for( j = 0; j < 2; j++ ) \
                { \
                    int result_c, result_a; \
                    INIT_QUANT##w() \
                    result_c = call_c1( qf_c.qname, dct1, h->quant##w##_mf[block][qp], h->quant##w##_bias[block][qp] ); \
                    result_a = call_a1( qf_a.qname, dct2, h->quant##w##_mf[block][qp], h->quant##w##_bias[block][qp] ); \
                    if( memcmp( dct1, dct2, w*w*2 ) || result_c != result_a ) \
                    { \
                        oks[0] = 0; \
                        fprintf( stderr, #qname "(qp=%d, cqm=%d, block="#block"): [FAILED]\n", qp, i_cqm ); \
                        break; \
                    } \
                    call_c2( qf_c.qname, dct1, h->quant##w##_mf[block][qp], h->quant##w##_bias[block][qp] ); \
                    call_a2( qf_a.qname, dct2, h->quant##w##_mf[block][qp], h->quant##w##_bias[block][qp] ); \
                } \
            } \
        }

        TEST_QUANT( quant_8x8, CQM_8IY, 8 );
        TEST_QUANT( quant_8x8, CQM_8PY, 8 );
        TEST_QUANT( quant_4x4, CQM_4IY, 4 );
        TEST_QUANT( quant_4x4, CQM_4PY, 4 );
        TEST_QUANT_DC( quant_4x4_dc, **h->quant4_mf[CQM_4IY] );
        TEST_QUANT_DC( quant_2x2_dc, **h->quant4_mf[CQM_4IC] );

#define TEST_DEQUANT( qname, dqname, block, w ) \
        if( qf_a.dqname != qf_ref.dqname ) \
        { \
            set_func_name( "%s_%s", #dqname, i_cqm?"cqm":"flat" ); \
            used_asms[1] = 1; \
            j = 1; \
            for( qp = 51; qp > 0; qp-- ) \
            { \
                INIT_QUANT##w() \
                call_c1( qf_c.qname, dct1, h->quant##w##_mf[block][qp], h->quant##w##_bias[block][qp] ); \
                memcpy( dct2, dct1, w*w*2 ); \
                call_c1( qf_c.dqname, dct1, h->dequant##w##_mf[block], qp ); \
                call_a1( qf_a.dqname, dct2, h->dequant##w##_mf[block], qp ); \
                if( memcmp( dct1, dct2, w*w*2 ) ) \
                { \
                    oks[1] = 0; \
                    fprintf( stderr, #dqname "(qp=%d, cqm=%d, block="#block"): [FAILED]\n", qp, i_cqm ); \
                    break; \
                } \
                call_c2( qf_c.dqname, dct1, h->dequant##w##_mf[block], qp ); \
                call_a2( qf_a.dqname, dct2, h->dequant##w##_mf[block], qp ); \
            } \
        }

        TEST_DEQUANT( quant_8x8, dequant_8x8, CQM_8IY, 8 );
        TEST_DEQUANT( quant_8x8, dequant_8x8, CQM_8PY, 8 );
        TEST_DEQUANT( quant_4x4, dequant_4x4, CQM_4IY, 4 );
        TEST_DEQUANT( quant_4x4, dequant_4x4, CQM_4PY, 4 );

#define TEST_DEQUANT_DC( qname, dqname, block, w ) \
        if( qf_a.dqname != qf_ref.dqname ) \
        { \
            set_func_name( "%s_%s", #dqname, i_cqm?"cqm":"flat" ); \
            used_asms[1] = 1; \
            for( qp = 51; qp > 0; qp-- ) \
            { \
                for( i = 0; i < 16; i++ ) \
                    dct1[i] = rand(); \
                call_c1( qf_c.qname, dct1, h->quant##w##_mf[block][qp][0]>>1, h->quant##w##_bias[block][qp][0]>>1 ); \
                memcpy( dct2, dct1, w*w*2 ); \
                call_c1( qf_c.dqname, dct1, h->dequant##w##_mf[block], qp ); \
                call_a1( qf_a.dqname, dct2, h->dequant##w##_mf[block], qp ); \
                if( memcmp( dct1, dct2, w*w*2 ) ) \
                { \
                    oks[1] = 0; \
                    fprintf( stderr, #dqname "(qp=%d, cqm=%d, block="#block"): [FAILED]\n", qp, i_cqm ); \
                } \
                call_c2( qf_c.dqname, dct1, h->dequant##w##_mf[block], qp ); \
                call_a2( qf_a.dqname, dct2, h->dequant##w##_mf[block], qp ); \
            } \
        }

        TEST_DEQUANT_DC( quant_4x4_dc, dequant_4x4_dc, CQM_4IY, 4 );

        x264_cqm_delete( h );
    }

    ok = oks[0]; used_asm = used_asms[0];
    report( "quant :" );

    ok = oks[1]; used_asm = used_asms[1];
    report( "dequant :" );

    ok = 1; used_asm = 0;
    if( qf_a.denoise_dct != qf_ref.denoise_dct )
    {
        int size;
        used_asm = 1;
        for( size = 16; size <= 64; size += 48 )
        {
            set_func_name( "denoise_dct" );
            memcpy(dct1, buf1, size*2);
            memcpy(dct2, buf1, size*2);
            memcpy(buf3+256, buf3, 256);
            call_c1( qf_c.denoise_dct, dct1, (uint32_t*)buf3, (uint16_t*)buf2, size );
            call_a1( qf_a.denoise_dct, dct2, (uint32_t*)(buf3+256), (uint16_t*)buf2, size );
            if( memcmp( dct1, dct2, size*2 ) || memcmp( buf3+4, buf3+256+4, (size-1)*sizeof(uint32_t) ) )
                ok = 0;
            call_c2( qf_c.denoise_dct, dct1, (uint32_t*)buf3, (uint16_t*)buf2, size );
            call_a2( qf_a.denoise_dct, dct2, (uint32_t*)(buf3+256), (uint16_t*)buf2, size );
        }
    }
    report( "denoise dct :" );

#define TEST_DECIMATE( decname, w, ac, thresh ) \
    if( qf_a.decname != qf_ref.decname ) \
    { \
        set_func_name( #decname ); \
        used_asm = 1; \
        for( i = 0; i < 100; i++ ) \
        { \
            int result_c, result_a, idx; \
            for( idx = 0; idx < w*w; idx++ ) \
                dct1[idx] = !(rand()&3) + (!(rand()&15))*(rand()&3); \
            if( ac ) \
                dct1[0] = 0; \
            result_c = call_c( qf_c.decname, dct1 ); \
            result_a = call_a( qf_a.decname, dct1 ); \
            if( X264_MIN(result_c,thresh) != X264_MIN(result_a,thresh) ) \
            { \
                ok = 0; \
                fprintf( stderr, #decname ": [FAILED]\n" ); \
                break; \
            } \
        } \
    }

    ok = 1; used_asm = 0;
    TEST_DECIMATE( decimate_score64, 8, 0, 6 );
    TEST_DECIMATE( decimate_score16, 4, 0, 6 );
    TEST_DECIMATE( decimate_score15, 4, 1, 7 );
    report( "decimate_score :" );

#define TEST_LAST( last, lastname, w, ac ) \
    if( qf_a.last != qf_ref.last ) \
    { \
        set_func_name( #lastname ); \
        used_asm = 1; \
        for( i = 0; i < 100; i++ ) \
        { \
            int result_c, result_a, idx, nnz=0; \
            int max = rand() & (w*w-1); \
            memset( dct1, 0, w*w*2 ); \
            for( idx = ac; idx < max; idx++ ) \
                nnz |= dct1[idx] = !(rand()&3) + (!(rand()&15))*rand(); \
            if( !nnz ) \
                dct1[ac] = 1; \
            result_c = call_c( qf_c.last, dct1+ac ); \
            result_a = call_a( qf_a.last, dct1+ac ); \
            if( result_c != result_a ) \
            { \
                ok = 0; \
                fprintf( stderr, #lastname ": [FAILED]\n" ); \
                break; \
            } \
        } \
    }

    ok = 1; used_asm = 0;
    TEST_LAST( coeff_last[DCT_CHROMA_DC],  coeff_last4, 2, 0 );
    TEST_LAST( coeff_last[  DCT_LUMA_AC], coeff_last15, 4, 1 );
    TEST_LAST( coeff_last[ DCT_LUMA_4x4], coeff_last16, 4, 0 );
    TEST_LAST( coeff_last[ DCT_LUMA_8x8], coeff_last64, 8, 0 );
    report( "coeff_last :" );

#define TEST_LEVELRUN( lastname, name, w, ac ) \
    if( qf_a.lastname != qf_ref.lastname ) \
    { \
        set_func_name( #name ); \
        used_asm = 1; \
        for( i = 0; i < 100; i++ ) \
        { \
            x264_run_level_t runlevel_c, runlevel_a; \
            int result_c, result_a, idx, nnz=0; \
            int max = rand() & (w*w-1); \
            memset( dct1, 0, w*w*2 ); \
            memcpy( &runlevel_a, buf1+i, sizeof(x264_run_level_t) ); \
            memcpy( &runlevel_c, buf1+i, sizeof(x264_run_level_t) ); \
            for( idx = ac; idx < max; idx++ ) \
                nnz |= dct1[idx] = !(rand()&3) + (!(rand()&15))*rand(); \
            if( !nnz ) \
                dct1[ac] = 1; \
            result_c = call_c( qf_c.lastname, dct1+ac, &runlevel_c ); \
            result_a = call_a( qf_a.lastname, dct1+ac, &runlevel_a ); \
            if( result_c != result_a || runlevel_c.last != runlevel_a.last || \
                memcmp(runlevel_c.level, runlevel_a.level, sizeof(int16_t)*result_c) || \
                memcmp(runlevel_c.run, runlevel_a.run, sizeof(uint8_t)*(result_c-1)) ) \
            { \
                ok = 0; \
                fprintf( stderr, #name ": [FAILED]\n" ); \
                break; \
            } \
        } \
    }

    ok = 1; used_asm = 0;
    TEST_LEVELRUN( coeff_level_run[DCT_CHROMA_DC],  coeff_level_run4, 2, 0 );
    TEST_LEVELRUN( coeff_level_run[  DCT_LUMA_AC], coeff_level_run15, 4, 1 );
    TEST_LEVELRUN( coeff_level_run[ DCT_LUMA_4x4], coeff_level_run16, 4, 0 );
    report( "coeff_level_run :" );

    return ret;
}

static int check_intra( int cpu_ref, int cpu_new )
{
    int ret = 0, ok = 1, used_asm = 0;
    int i;
    ALIGNED_16( uint8_t edge[33] );
    ALIGNED_16( uint8_t edge2[33] );
    struct
    {
        x264_predict_t      predict_16x16[4+3];
        x264_predict_t      predict_8x8c[4+3];
        x264_predict8x8_t   predict_8x8[9+3];
        x264_predict_t      predict_4x4[9+3];
        x264_predict_8x8_filter_t predict_8x8_filter;
    } ip_c, ip_ref, ip_a;

    x264_predict_16x16_init( 0, ip_c.predict_16x16 );
    x264_predict_8x8c_init( 0, ip_c.predict_8x8c );
    x264_predict_8x8_init( 0, ip_c.predict_8x8, &ip_c.predict_8x8_filter );
    x264_predict_4x4_init( 0, ip_c.predict_4x4 );

    x264_predict_16x16_init( cpu_ref, ip_ref.predict_16x16 );
    x264_predict_8x8c_init( cpu_ref, ip_ref.predict_8x8c );
    x264_predict_8x8_init( cpu_ref, ip_ref.predict_8x8, &ip_ref.predict_8x8_filter );
    x264_predict_4x4_init( cpu_ref, ip_ref.predict_4x4 );

    x264_predict_16x16_init( cpu_new, ip_a.predict_16x16 );
    x264_predict_8x8c_init( cpu_new, ip_a.predict_8x8c );
    x264_predict_8x8_init( cpu_new, ip_a.predict_8x8, &ip_a.predict_8x8_filter );
    x264_predict_4x4_init( cpu_new, ip_a.predict_4x4 );

    ip_c.predict_8x8_filter( buf1+48, edge, ALL_NEIGHBORS, ALL_NEIGHBORS );

#define INTRA_TEST( name, dir, w, ... ) \
    if( ip_a.name[dir] != ip_ref.name[dir] )\
    { \
        set_func_name( "intra_%s_%s", #name, intra_##name##_names[dir] );\
        used_asm = 1; \
        memcpy( buf3, buf1, 32*20 );\
        memcpy( buf4, buf1, 32*20 );\
        call_c( ip_c.name[dir], buf3+48, ##__VA_ARGS__ );\
        call_a( ip_a.name[dir], buf4+48, ##__VA_ARGS__ );\
        if( memcmp( buf3, buf4, 32*20 ) )\
        {\
            fprintf( stderr, #name "[%d] :  [FAILED]\n", dir );\
            ok = 0;\
            int j,k;\
            for(k=-1; k<16; k++)\
                printf("%2x ", edge[16+k]);\
            printf("\n");\
            for(j=0; j<w; j++){\
                printf("%2x ", edge[14-j]);\
                for(k=0; k<w; k++)\
                    printf("%2x ", buf4[48+k+j*32]);\
                printf("\n");\
            }\
            printf("\n");\
            for(j=0; j<w; j++){\
                printf("   ");\
                for(k=0; k<w; k++)\
                    printf("%2x ", buf3[48+k+j*32]);\
                printf("\n");\
            }\
        }\
    }

    for( i = 0; i < 12; i++ )
        INTRA_TEST( predict_4x4, i, 4 );
    for( i = 0; i < 7; i++ )
        INTRA_TEST( predict_8x8c, i, 8 );
    for( i = 0; i < 7; i++ )
        INTRA_TEST( predict_16x16, i, 16 );
    for( i = 0; i < 12; i++ )
        INTRA_TEST( predict_8x8, i, 8, edge );

    set_func_name("intra_predict_8x8_filter");
    if( ip_a.predict_8x8_filter != ip_ref.predict_8x8_filter )
    {
        used_asm = 1;
        for( i = 0; i < 32; i++ )
        {
            memcpy( edge2, edge, 33 );
            call_c(ip_c.predict_8x8_filter, buf1+48, edge, (i&24)>>1, i&7);
            call_a(ip_a.predict_8x8_filter, buf1+48, edge2, (i&24)>>1, i&7);
            if( memcmp( edge, edge2, 33 ) )
            {
                fprintf( stderr, "predict_8x8_filter :  [FAILED] %d %d\n", (i&24)>>1, i&7);
                ok = 0;
            }
        }
    }

    report( "intra pred :" );
    return ret;
}

#define DECL_CABAC(cpu) \
static void run_cabac_##cpu( uint8_t *dst )\
{\
    int i;\
    x264_cabac_t cb;\
    x264_cabac_context_init( &cb, SLICE_TYPE_P, 26, 0 );\
    x264_cabac_encode_init( &cb, dst, dst+0xff0 );\
    for( i=0; i<0x1000; i++ )\
        x264_cabac_encode_decision_##cpu( &cb, buf1[i]>>1, buf1[i]&1 );\
}
DECL_CABAC(c)
#ifdef HAVE_MMX
DECL_CABAC(asm)
#else
#define run_cabac_asm run_cabac_c
#endif

static int check_cabac( int cpu_ref, int cpu_new )
{
    int ret = 0, ok, used_asm = 1;
    if( cpu_ref || run_cabac_c == run_cabac_asm)
        return 0;
    set_func_name( "cabac_encode_decision" );
    memcpy( buf4, buf3, 0x1000 );
    call_c( run_cabac_c, buf3 );
    call_a( run_cabac_asm, buf4 );
    ok = !memcmp( buf3, buf4, 0x1000 );
    report( "cabac :" );
    return ret;
}

static int check_all_funcs( int cpu_ref, int cpu_new )
{
    return check_pixel( cpu_ref, cpu_new )
         + check_dct( cpu_ref, cpu_new )
         + check_mc( cpu_ref, cpu_new )
         + check_intra( cpu_ref, cpu_new )
         + check_deblock( cpu_ref, cpu_new )
         + check_quant( cpu_ref, cpu_new )
         + check_cabac( cpu_ref, cpu_new );
}

static int add_flags( int *cpu_ref, int *cpu_new, int flags, const char *name )
{
    *cpu_ref = *cpu_new;
    *cpu_new |= flags;
    if( *cpu_new & X264_CPU_SSE2_IS_FAST )
        *cpu_new &= ~X264_CPU_SSE2_IS_SLOW;
    if( !quiet )
        fprintf( stderr, "x264: %s\n", name );
    return check_all_funcs( *cpu_ref, *cpu_new );
}

static int check_all_flags( void )
{
    int ret = 0;
    int cpu0 = 0, cpu1 = 0;
#ifdef HAVE_MMX
    if( x264_cpu_detect() & X264_CPU_MMXEXT )
    {
        ret |= add_flags( &cpu0, &cpu1, X264_CPU_MMX | X264_CPU_MMXEXT, "MMX" );
        ret |= add_flags( &cpu0, &cpu1, X264_CPU_CACHELINE_64, "MMX Cache64" );
        cpu1 &= ~X264_CPU_CACHELINE_64;
#ifdef ARCH_X86
        ret |= add_flags( &cpu0, &cpu1, X264_CPU_CACHELINE_32, "MMX Cache32" );
        cpu1 &= ~X264_CPU_CACHELINE_32;
#endif
        if( x264_cpu_detect() & X264_CPU_LZCNT )
        {
            ret |= add_flags( &cpu0, &cpu1, X264_CPU_LZCNT, "MMX_LZCNT" );
            cpu1 &= ~X264_CPU_LZCNT;
        }
    }
    if( x264_cpu_detect() & X264_CPU_SSE2 )
    {
        ret |= add_flags( &cpu0, &cpu1, X264_CPU_SSE | X264_CPU_SSE2 | X264_CPU_SSE2_IS_SLOW, "SSE2Slow" );
        ret |= add_flags( &cpu0, &cpu1, X264_CPU_SSE2_IS_FAST, "SSE2Fast" );
        ret |= add_flags( &cpu0, &cpu1, X264_CPU_CACHELINE_64, "SSE2Fast Cache64" );
        ret |= add_flags( &cpu0, &cpu1, X264_CPU_SHUFFLE_IS_FAST, "SSE2 FastShuffle" );
        cpu1 &= ~X264_CPU_SHUFFLE_IS_FAST;
    }
    if( x264_cpu_detect() & X264_CPU_SSE_MISALIGN )
    {
        cpu1 &= ~X264_CPU_CACHELINE_64;
        ret |= add_flags( &cpu0, &cpu1, X264_CPU_SSE_MISALIGN, "SSE_Misalign" );
        cpu1 &= ~X264_CPU_SSE_MISALIGN;
    }
    if( x264_cpu_detect() & X264_CPU_LZCNT )
    {
        cpu1 &= ~X264_CPU_CACHELINE_64;
        ret |= add_flags( &cpu0, &cpu1, X264_CPU_LZCNT, "SSE_LZCNT" );
        cpu1 &= ~X264_CPU_LZCNT;
    }
    if( x264_cpu_detect() & X264_CPU_SSE3 )
        ret |= add_flags( &cpu0, &cpu1, X264_CPU_SSE3 | X264_CPU_CACHELINE_64, "SSE3" );
    if( x264_cpu_detect() & X264_CPU_SSSE3 )
    {
        cpu1 &= ~X264_CPU_CACHELINE_64;
        ret |= add_flags( &cpu0, &cpu1, X264_CPU_SSSE3, "SSSE3" );
        ret |= add_flags( &cpu0, &cpu1, X264_CPU_CACHELINE_64, "SSSE3 Cache64" );
        ret |= add_flags( &cpu0, &cpu1, X264_CPU_SHUFFLE_IS_FAST, "SSSE3 FastShuffle" );
        cpu1 &= ~X264_CPU_SHUFFLE_IS_FAST;
    }
    if( x264_cpu_detect() & X264_CPU_SSE4 )
    {
        cpu1 &= ~X264_CPU_CACHELINE_64;
        ret |= add_flags( &cpu0, &cpu1, X264_CPU_SSE4, "SSE4" );
    }
#elif ARCH_PPC
    if( x264_cpu_detect() & X264_CPU_ALTIVEC )
    {
        fprintf( stderr, "x264: ALTIVEC against C\n" );
        ret = check_all_funcs( 0, X264_CPU_ALTIVEC );
    }
#elif ARCH_ARM
    if( x264_cpu_detect() & X264_CPU_ARMV6 )
        ret |= add_flags( &cpu0, &cpu1, X264_CPU_ARMV6, "ARMv6" );
    if( x264_cpu_detect() & X264_CPU_NEON )
        ret |= add_flags( &cpu0, &cpu1, X264_CPU_NEON, "NEON" );
    if( x264_cpu_detect() & X264_CPU_FAST_NEON_MRC )
        ret |= add_flags( &cpu0, &cpu1, X264_CPU_FAST_NEON_MRC, "Fast NEON MRC" );
#endif
    return ret;
}

int main(int argc, char *argv[])
{
    int ret = 0;
    int i;

    if( argc > 1 && !strncmp( argv[1], "--bench", 7 ) )
    {
#if !defined(ARCH_X86) && !defined(ARCH_X86_64) && !defined(ARCH_PPC) && !defined(ARCH_ARM)
        fprintf( stderr, "no --bench for your cpu until you port rdtsc\n" );
        return 1;
#endif
        do_bench = 1;
        if( argv[1][7] == '=' )
        {
            bench_pattern = argv[1]+8;
            bench_pattern_len = strlen(bench_pattern);
        }
        argc--;
        argv++;
    }

    i = ( argc > 1 ) ? atoi(argv[1]) : x264_mdate();
    fprintf( stderr, "x264: using random seed %u\n", i );
    srand( i );

    buf1 = x264_malloc( 0x3e00 + 16*BENCH_ALIGNS );
    if( !buf1 )
    {
        fprintf( stderr, "malloc failed, unable to initiate tests!\n" );
        return -1;
    }
    buf2 = buf1 + 0xf00;
    buf3 = buf2 + 0xf00;
    buf4 = buf3 + 0x1000;
    for( i=0; i<0x1e00; i++ )
        buf1[i] = rand() & 0xFF;
    memset( buf1+0x1e00, 0, 0x2000 );

    /* 16-byte alignment is guaranteed whenever it's useful, but some functions also vary in speed depending on %64 */
    if( do_bench )
        for( i=0; i<BENCH_ALIGNS && !ret; i++ )
        {
            buf2 = buf1 + 0xf00;
            buf3 = buf2 + 0xf00;
            buf4 = buf3 + 0x1000;
            ret |= x264_stack_pagealign( check_all_flags, i*16 );
            buf1 += 16;
            quiet = 1;
            fprintf( stderr, "%d/%d\r", i+1, BENCH_ALIGNS );
        }
    else
        ret = check_all_flags();

    if( ret )
    {
        fprintf( stderr, "x264: at least one test has failed. Go and fix that Right Now!\n" );
        return -1;
    }
    fprintf( stderr, "x264: All tests passed Yeah :)\n" );
    if( do_bench )
        print_bench();
    return 0;
}


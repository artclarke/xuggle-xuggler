#ifndef _X264_VFW_H
#define _X264_VFW_H

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <windows.h>
#include <vfw.h>

#include <x264.h>

#include "resource.h"

/* Name */
#define X264_NAME_L     L"x264"
#define X264_DESC_L     L"x264 - H264/AVC encoder"

/* Codec fcc */
#define FOURCC_X264 mmioFOURCC('X','2','6','4')

/* yuv 4:2:0 planar */
#define FOURCC_I420 mmioFOURCC('I','4','2','0')
#define FOURCC_IYUV mmioFOURCC('I','Y','U','V')
#define FOURCC_YV12 mmioFOURCC('Y','V','1','2')

/* yuv 4:2:2 packed */
#define FOURCC_YUY2 mmioFOURCC('Y','U','Y','2')
#define FOURCC_YUYV mmioFOURCC('Y','U','Y','V')

#define X264_WEBSITE    "http://videolan.org/x264.html"

/* CONFIG: vfw config
 */
typedef struct
{
    /********** ATTENTION **********/
    int mode;                   /* Vidomi directly accesses these vars */
    int bitrate;
    int desired_size;           /* please try to avoid modifications here */
    char stats[MAX_PATH];
    /*******************************/
    int i_2passbitrate;
    int i_pass;

    int b_fast1pass;    /* turns off some flags during 1st pass */    
    int b_updatestats;  /* updates the statsfile during 2nd pass */

    int i_frame_total;

    /* Our config */
    int i_refmax;
    int i_keyint_max;
    int i_keyint_min;
    int i_scenecut_threshold;
    int i_qp_min;
    int i_qp_max;
    int i_qp_step;

    int i_qp;
    int b_filter;

    int b_cabac;

    int b_i8x8;
    int b_i4x4;
    int b_psub16x16;
    int b_psub8x8;
    int b_bsub16x16;
    int b_dct8x8;
    int b_mixedref;

    int i_bframe;
    int i_subpel_refine;
    int i_me_method;
    int i_me_range;
    int b_chroma_me;
    int i_direct_mv_pred;
    int i_threads;

    int i_inloop_a;
    int i_inloop_b;

    int b_b_refs;
    int b_b_wpred;
    int i_bframe_bias;
    int b_bframe_adaptive;

    int i_key_boost;
    int i_b_red;
    int i_curve_comp;

    int i_sar_width;
    int i_sar_height;

    int i_log_level;

    /* vfw interface */
    int b_save;
    /* fourcc used */
    char fcc[4+1];
    int i_encoding_type;
    int i_trellis;
    int b_bidir_me;
    int i_noise_reduction;
} CONFIG;

/* CODEC: vfw codec instance
 */
typedef struct
{
    CONFIG config;

    /* handle */
    x264_t *h;

    /* error console handle */
    HWND *hCons;

    /* XXX: needed ? */
    unsigned int fincr;
    unsigned int fbase;
} CODEC;

/* Compress functions */
LRESULT compress_query(CODEC *, BITMAPINFO *, BITMAPINFO *);
LRESULT compress_get_format(CODEC *, BITMAPINFO *, BITMAPINFO *);
LRESULT compress_get_size(CODEC *, BITMAPINFO *, BITMAPINFO *);
LRESULT compress_frames_info(CODEC *, ICCOMPRESSFRAMES *);
LRESULT compress_begin(CODEC *, BITMAPINFO *, BITMAPINFO *);
LRESULT compress_end(CODEC *);
LRESULT compress(CODEC *, ICCOMPRESS *);


/* config functions */
void config_reg_load( CONFIG * config );
void config_reg_save( CONFIG * config );
static void tabs_enable_items( HWND hDlg, CONFIG * config );
static void tabs_update_items( HWND hDlg, CONFIG * config );

/* Dialog callbacks */
BOOL CALLBACK callback_main ( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
BOOL CALLBACK callback_tabs( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
BOOL CALLBACK callback_about( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
BOOL CALLBACK callback_err_console( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );

/* Dll instance */
extern HINSTANCE g_hInst;

#if defined(_DEBUG)
#include <stdio.h> /* vsprintf */
#define DPRINTF_BUF_SZ  1024
static __inline void DPRINTF(char *fmt, ...)
{
    va_list args;
    char buf[DPRINTF_BUF_SZ];

    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    OutputDebugString(buf);
}
#else
static __inline void DPRINTF(char *fmt, ...) { }
#endif


#endif


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

#define X264_WEBSITE	"http://lyra.via.ecp.fr/"

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

    /* Our config */
    int i_refmax;
    int i_idrframe;
    int i_iframe;

    int i_qp;
    int b_filter;

    int b_cabac;

    int b_i4x4;
    int b_psub16x16;
    int b_psub8x8;

    /* vfw interface */
    int b_save;
    /* fourcc used */
    char fcc[4+1];
    int  i_encoding_type;
} CONFIG;

/* CODEC: vfw codec instance
 */
typedef struct
{
    CONFIG config;

    /* handle */
    x264_t *h;

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


/* Dialog callbacks */
BOOL CALLBACK callback_about( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
BOOL CALLBACK callback_main ( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
BOOL CALLBACK callback_advanced( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );

/* Dll instance */
extern HINSTANCE g_hInst;

#endif


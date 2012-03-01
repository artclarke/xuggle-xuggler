/*******************************************************************************
 * Copyright (c) 2008, 2010 Xuggle Inc.  All rights reserved.
 *  
 * This file is part of Xuggle-Xuggler-Main.
 *
 * Xuggle-Xuggler-Main is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Xuggle-Xuggler-Main is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Xuggle-Xuggler-Main.  If not, see <http://www.gnu.org/licenses/>.
 *******************************************************************************/

#include <jni.h>
#include <com/xuggle/xuggler/io/IO.h>

/* Header for class com_xuggle_xuggler_io_FfmpegIO */

#ifndef _FFMPEG_IO_H
#define _FFMPEG_IO_H
#ifdef __cplusplus
extern "C" {
#endif

VS_API_XUGGLER_IO void VS_API_CALL Java_com_xuggle_xuggler_io_FfmpegIO_init(JNIEnv *env, jclass);

/*
 * Class:     com_xuggle_xuggler_io_FfmpegIO
 * Method:    native_registerProtocolHandler
 * Signature: (Ljava/lang/String;Lnet/xuggle/xuggler/io/URLProtocolManager;)I
 */
VS_API_XUGGLER_IO jint VS_API_CALL Java_com_xuggle_xuggler_io_FfmpegIO_native_1registerProtocolHandler
  (JNIEnv *, jclass, jstring, jobject);

/*
 * Class:     com_xuggle_xuggler_io_FfmpegIO
 * Method:    native_url_open
 * Signature: (Lnet/xuggle/xuggler/io/FfmpegIOHandle;Ljava/lang/String;I)I
 */
VS_API_XUGGLER_IO jint VS_API_CALL Java_com_xuggle_xuggler_io_FfmpegIO_native_1url_1open
  (JNIEnv *, jclass, jobject, jstring, jint);

/*
 * Class:     com_xuggle_xuggler_io_FfmpegIO
 * Method:    native_url_read
 * Signature: (Lnet/xuggle/xuggler/io/FfmpegIOHandle;[BI)I
 */
VS_API_XUGGLER_IO jint VS_API_CALL Java_com_xuggle_xuggler_io_FfmpegIO_native_1url_1read
  (JNIEnv *, jclass, jobject, jbyteArray, jint);

/*
 * Class:     com_xuggle_xuggler_io_FfmpegIO
 * Method:    native_url_write
 * Signature: (Lnet/xuggle/xuggler/io/FfmpegIOHandle;[BI)I
 */
VS_API_XUGGLER_IO jint VS_API_CALL Java_com_xuggle_xuggler_io_FfmpegIO_native_1url_1write
  (JNIEnv *, jclass, jobject, jbyteArray, jint);

/*
 * Class:     com_xuggle_xuggler_io_FfmpegIO
 * Method:    native_url_seek
 * Signature: (Lnet/xuggle/xuggler/io/FfmpegIOHandle;JI)J
 */
VS_API_XUGGLER_IO jlong VS_API_CALL Java_com_xuggle_xuggler_io_FfmpegIO_native_1url_1seek
  (JNIEnv *, jclass, jobject, jlong, jint);

/*
 * Class:     com_xuggle_xuggler_io_FfmpegIO
 * Method:    native_url_close
 * Signature: (Lnet/xuggle/xuggler/io/FfmpegIOHandle;)I
 */
VS_API_XUGGLER_IO jint VS_API_CALL Java_com_xuggle_xuggler_io_FfmpegIO_native_1url_1close
  (JNIEnv *, jclass, jobject);

#ifdef __cplusplus
}
#endif
#endif // _FFMPEG_IO_H

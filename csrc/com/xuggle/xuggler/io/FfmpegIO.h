/*
 * This file is part of Xuggler.
 * 
 * Xuggler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 * 
 * Xuggler is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public
 * License along with Xuggler.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <jni.h>
/* Header for class com_xuggle_xuggler_io_FfmpegIO */

#ifndef _FFMPEG_IO_H
#define _FFMPEG_IO_H
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     com_xuggle_xuggler_io_FfmpegIO
 * Method:    native_registerProtocolHandler
 * Signature: (Ljava/lang/String;Lnet/xuggle/xuggler/io/URLProtocolManager;)I
 */
JNIEXPORT jint JNICALL Java_com_xuggle_xuggler_io_FfmpegIO_native_1registerProtocolHandler
  (JNIEnv *, jclass, jstring, jobject);

/*
 * Class:     com_xuggle_xuggler_io_FfmpegIO
 * Method:    native_url_open
 * Signature: (Lnet/xuggle/xuggler/io/FfmpegIOHandle;Ljava/lang/String;I)I
 */
JNIEXPORT jint JNICALL Java_com_xuggle_xuggler_io_FfmpegIO_native_1url_1open
  (JNIEnv *, jclass, jobject, jstring, jint);

/*
 * Class:     com_xuggle_xuggler_io_FfmpegIO
 * Method:    native_url_read
 * Signature: (Lnet/xuggle/xuggler/io/FfmpegIOHandle;[BI)I
 */
JNIEXPORT jint JNICALL Java_com_xuggle_xuggler_io_FfmpegIO_native_1url_1read
  (JNIEnv *, jclass, jobject, jbyteArray, jint);

/*
 * Class:     com_xuggle_xuggler_io_FfmpegIO
 * Method:    native_url_write
 * Signature: (Lnet/xuggle/xuggler/io/FfmpegIOHandle;[BI)I
 */
JNIEXPORT jint JNICALL Java_com_xuggle_xuggler_io_FfmpegIO_native_1url_1write
  (JNIEnv *, jclass, jobject, jbyteArray, jint);

/*
 * Class:     com_xuggle_xuggler_io_FfmpegIO
 * Method:    native_url_seek
 * Signature: (Lnet/xuggle/xuggler/io/FfmpegIOHandle;JI)J
 */
JNIEXPORT jlong JNICALL Java_com_xuggle_xuggler_io_FfmpegIO_native_1url_1seek
  (JNIEnv *, jclass, jobject, jlong, jint);

/*
 * Class:     com_xuggle_xuggler_io_FfmpegIO
 * Method:    native_url_close
 * Signature: (Lnet/xuggle/xuggler/io/FfmpegIOHandle;)I
 */
JNIEXPORT jint JNICALL Java_com_xuggle_xuggler_io_FfmpegIO_native_1url_1close
  (JNIEnv *, jclass, jobject);

#ifdef __cplusplus
}
#endif
#endif // _FFMPEG_IO_H

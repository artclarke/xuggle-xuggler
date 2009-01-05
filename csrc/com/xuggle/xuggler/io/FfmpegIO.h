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

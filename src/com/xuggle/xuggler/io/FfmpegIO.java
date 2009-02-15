/*
 * Copyright (c) 2008-2009 by Xuggle Inc. All rights reserved.
 *
 * It is REQUESTED BUT NOT REQUIRED if you use this library, that you let 
 * us know by sending e-mail to info@xuggle.com telling us briefly how you're
 * using the library and what you like or don't like about it.
 *
 * This library is free software; you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any later
 * version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along
 * with this library; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */
package com.xuggle.xuggler.io;


import com.xuggle.xuggler.io.FfmpegIOHandle;
import com.xuggle.xuggler.io.IURLProtocolHandlerFactory;
import com.xuggle.xuggler.io.URLProtocolManager;
import com.xuggle.xuggler.io.Version;

/**
 * For Internal Use Only.
 * This is the global (static) object that implements the Xuggler IO library.
 * <p>
 * The following methods are not meant to be generally
 * called by code (although they can be).
 * </p></p>
 * You should never need to call this method.  Calling {@link URLProtocolManager#registerFactory(String, IURLProtocolHandlerFactory)}
 * should cause it to be instantiated.
 *</p></p>
 * They are generally 'thread-aware' but not 'thread-safe', meaning if you use a handle
 * on a thread, you are a responsible for making sure you don't
 * reuse it on other threads.
 * </p></p>
 * They forward into ffmpeg's URL read/write/seek capabilities
 * and allow our test scripts to make sure our URLProtocolHandlers
 * (and FFMPEG's native handlers) work as expected.
 * </p></p>
 * Lastly this class, unlike other classes in the library, does not use SWIG to generate
 * the Java objects, so you need to be careful not to change method names as the corresponding
 * native code relies on specific naming and paramaeters.
 * </p>
 * @author aclarke
 *
 */
public class FfmpegIO
{

  static Boolean initialized = false;

  static
  {
    init();
  }

  /**
   * Load the native library we depend on.
   */
  static void init()
  {
    if (!initialized)
    {
      synchronized (initialized)
      {
        if (!initialized)
        {
          com.xuggle.ferry.JNILibraryLoader.loadLibrary(
              "xuggle-xuggler-io",
              new Long(Version.MAJOR_VERSION));
        }
        initialized = true;
        // And force the URLProtocolManager global
        // object to be created.
        URLProtocolManager.init();
      }
    }

  }

  /**
   * This method is called by URLProtocolManager to register itself as a 
   * protocol manager for different FFMPEG protocols.
   * 
   * @param protocol The protocol
   * @param manager The manager for that protocol.
   */
  static void registerProtocolHandler(String protocol,
      URLProtocolManager manager)
  {
    native_registerProtocolHandler(protocol, manager);
  }

  public static int url_open(FfmpegIOHandle handle, String filename, int flags)
  {
    return native_url_open(handle, filename, flags);
  }

  public static int url_read(FfmpegIOHandle handle, byte[] buffer, int length)
  {
    return native_url_read(handle, buffer, length);
  }

  public static int url_close(FfmpegIOHandle handle)
  {
    return native_url_close(handle);
  }

  public static int url_write(FfmpegIOHandle handle, byte[] buffer, int length)
  {
    return native_url_write(handle, buffer, length);
  }

  public static long url_seek(FfmpegIOHandle handle, long position, int whence)
  {
    return native_url_seek(handle, position, whence);
  }

  private static native int native_registerProtocolHandler(
      String urlPrefix, URLProtocolManager proto);

  private static native int native_url_open(FfmpegIOHandle handle,
      String filename, int flags);

  private static native int native_url_read(FfmpegIOHandle handle,
      byte[] buffer, int length);

  private static native int native_url_write(FfmpegIOHandle handle,
      byte[] buffer, int length);

  private static native long native_url_seek(FfmpegIOHandle handle,
      long position, int whence);

  private static native int native_url_close(FfmpegIOHandle handle);

}

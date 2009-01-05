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

import com.xuggle.xuggler.io.FileProtocolHandler;
import com.xuggle.xuggler.io.URLProtocolManager;

/**
 * Interface that FFMPEG native code calls back to for each
 * URL.  It is assumed a new Protocol object is made
 * per URL being read or written to.
 * <p> 
 * You must implement this interface if you want to register a
 * new type of URLProtocolHandler with the {@link URLProtocolManager}.
 * </p>
 * <p>
 * If you throw an exception in your implementation of this handler during
 * a callback from within Xuggler, we will assume your method returned -1 while
 * still in native code.  Once the stack unwinds back into Xuggler we will
 * re-raise your exception.
 * </p>
 * @see FileProtocolHandler
 * 
 * @author aclarke
 *
 */
public interface IURLProtocolHandler
{
  // These are the flags that can be passed to open
  // IMPORTANT: These values must match the corresponding
  // flags in the avio.h header file in libavformat
  
  /**
   * A flag for {@link #seek(long, int)}.  Denotes positions relative to start of file.
   */
  public static final int SEEK_SET = 0;
  /**
   * A flag for {@link #seek(long, int)}.  Denotes positions relative to where the current file pointer is.
   */
  public static final int SEEK_CUR = 1;
  /**
   * A flag for {@link #seek(long, int)}.  Denotes positions relative to the end of file.
   */
  public static final int SEEK_END = 2;
  /**
   * A flag for {@link #seek(long, int)}.  A special hack of FFMPEG, denotes you want to find the total size of the file.
   */
  public static final int SEEK_SIZE = 0x10000;
  /**
   * Open the file in Read Only mode.
   */
  public static final int URL_RDONLY_MODE = 0;
  
  /**
   * Open the file in Write Only mode.
   */
  public static final int URL_WRONLY_MODE = 1;
  
  /**
   * Implement the file in Read/Write mode.
   */
  public static final int URL_RDWR = 2;

  /**
   * This method gets called by FFMPEG when it opens a file.
   * 
   * @param url The URL to open
   * @param flags The flags (e.g. {@link #URL_RDONLY_MODE})
   * @return >= 0 for success; -1 for error.
   * 
   */
  public int open(String url, int flags);

  /**
   * This method gets called by FFMPEG when it tries to read data.
   * <p>
   * Implementators should block until data is available; returning 0 signals
   * to FFMPEG that the file is empty.
   * </p>
   * 
   * @param buf The buffer to write your data to.
   * @param size The number of bytes in buf data available for you to write the data that FFMPEG will read.
   * @return 0 for end of file, else number of bytes you wrote to the buffer, or -1 if error.
   */
  public int read(byte[] buf, int size);

  /**
   * This method gets called by FFMPEG when it tries to write data.
   * <p>
   * Implementators should block until data can be written; returning 0 signals
   * to FFMPEG that the file is empty.
   * </p>
   * 
   * @param buf The data you should write.
   * @param size The number of bytes in buf.
   * @return 0 for end of file, else number of bytes you read from buf, or -1 if error.
   */
  public int write(byte[] buf, int size);

  /**
   * A request from FFMPEG to seek to a position in the stream.
   * 
   * @param offset The offset in bytes.
   * @param whence Where that offset is relative to.  Follow the C stdlib fseek() conventions EXCEPT
   *   {@link #SEEK_SIZE} should return back the size of the stream in bytes if known without adjusting
   *   the seek pointer.
   * @return -1 if not supported, else the position relative to whence
   */
  public long seek(long offset, int whence);

  /**
   * A request to close() from ffmpeg
   * 
   * @return -1 on error; else >= 0
   */
  public int close();

  /**
   * Special callback made by Xuggler in order to determine if your stream supports streaming.
   * 
   * If you return true, FFMPEG may jump around in the stream.  Be warned that FFMPEG cannot write
   * certain formats to streaming URLs (for example, the .MOV container).
   * 
   * @param url The URL that would be passed to {@link #open(String, int)}
   * @param flags The flags that would be passed to {@link #open(String, int)}
   * @return true if you can stream that URL; false if not.
   */
  public boolean isStreamed(String url, int flags);
}

/*
 * Copyright (c) 2008, 2009 by Xuggle Incorporated. All rights reserved.
 * 
 * This file is part of Xuggler.
 * 
 * You can redistribute Xuggler and/or modify it under the terms of the GNU
 * Affero General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 * 
 * Xuggler is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Affero General Public License for more
 * details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with Xuggler. If not, see <http://www.gnu.org/licenses/>.
 */

package com.xuggle.xuggler.io;

import java.io.Closeable;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentMap;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.xuggle.xuggler.IContainer;

/**
 * Allows Xuggler to read from and write to Java {@link InputStream} and
 * {@link OutputStream} objects.
 * <p>
 * Users should call {@link #map(String, InputStream, OutputStream)} to get a
 * string that can be passed to
 * {@link com.xuggle.xuggler.IContainer#open(String, com.xuggle.xuggler.IContainer.Type, com.xuggle.xuggler.IContainerFormat)
 * }
 * </p>
 * <p>
 * 
 * To use for reading (assuming an InputStream object called inputStream):
 * </p>
 * 
 * <pre>
 * IContainer container = IContainer.make();
 * container
 *     .open(StreamIO.map(&quot;yourkey&quot;, inputStream), IContainer.Type.READ, null);
 * </pre>
 * <p>
 * or for writing:
 * </p>
 * 
 * <pre>
 * IContainer container = IContainer.make();
 * container.open(StreamIO.map(&quot;yourkey&quot;, outputStream), IContainer.Type.WRITE,
 *     null);
 * </pre>
 * 
 * <p>
 * If the container is opened for reading, then the inputStream argument will be
 * used. If opened for writing, then the outputStream argument will be used. You
 * can pass any unique string you like for the first argument and should use
 * your own conventions to maintain uniqueness.
 * 
 * </p>
 * <p>
 * 
 * All streams that are mapped in this factory share the same name space, even
 * if registered under different protocols. So, if "exampleone" and "exampletwo"
 * were both registered as protocols for this factory, then
 * "exampleone:filename" is the same as "exampletwo:filename" and will map to
 * the same input and output streams. In reality, they are all mapped to the
 * {@link #DEFAULT_PROTOCOL} protocol.
 * 
 * </p>
 * <p>
 * 
 * By default, if you call {@link #mapIO(String, InputStream, OutputStream)},
 * the mapping will be removed automagically when Xuggler calls back into a
 * registered handler and attempts to open the stream. That means that code like
 * the examples above will not leak old InputStream references. If you prefer to
 * manually unmap your mappings, support is provided for that too.
 * 
 * </p>
 */

public class StreamIO implements IURLProtocolHandlerFactory
{
  /**
   * The default protocol that this factory uses ({@value #DEFAULT_PROTOCOL}).
   * <p>
   * For example, passing {@value #DEFAULT_PROTOCOL} as the protocol part of a
   * URL, and "foo" as the non-protocol part, will open an input handled by this
   * factory, provided {@link #mapIO(String, InputStream, OutputStream)} or
   * {@link #map(String, InputStream, OutputStream)} had been called previously
   * for "foo".
   * </p>
   */

  public final static String DEFAULT_PROTOCOL = "xugglerjavaio";

  /**
   * Do we undo mappings on open by default?
   */
  private final static boolean DEFAULT_UNMAP_URL_ON_OPEN = true;

  /**
   * Do we call {@link Closeable#close()} by default when closing?
   */
  private final static boolean DEFAULT_CLOSE_STREAM_ON_CLOSE = true;

  /**
   * A thread-safe mapping between URLs and registration information
   */
  private ConcurrentMap<String, RegistrationInformation> mURLs = new ConcurrentHashMap<String, RegistrationInformation>();

  /**
   * The singleton Factory object for this class loader.
   */
  private final static StreamIO mFactory = new StreamIO();

  /**
   * The static constructor just registered the singleton factory under the
   * DEFAULT_PROTOCOL protocol.
   */
  static
  {
    registerFactory(DEFAULT_PROTOCOL);
  }

  /**
   * Package level constructor. We don't allow people to create their own
   * version of this factory
   */
  StreamIO()
  {

  }

  /**
   * Register a new protocol name for this factory that Xuggler.IO will use for
   * the given protocol.
   * 
   * <p>
   * 
   * A default factory for the protocol {@value #DEFAULT_PROTOCOL} will already
   * be registered. This just allows you to register the same factory under
   * different strings if you want.
   * 
   * </p>
   * <p>
   * 
   * <strong>NOTE: Protocol can only contain alpha characters.</strong>
   * 
   * </p>
   * 
   * @param protocolPrefix
   *          The protocol (e.g. "yourapphandler").
   * @return The factory registered
   */

  static public StreamIO registerFactory(String protocolPrefix)
  {
    URLProtocolManager manager = URLProtocolManager.getManager();
    manager.registerFactory(protocolPrefix, mFactory);
    return mFactory;
  }

  /**
   * Get the singleton factory object for this class.
   * 
   * @return the factory
   */

  static public StreamIO getFactory()
  {
    return mFactory;
  }

  /**
   * Maps an {@link InputStream} to a URL for use by Xuggler.
   * 
   * Forwards to {@link #getFactory()}.
   * {@link #mapIO(String, InputStream, OutputStream)}
   * 
   * @return Returns a URL that can be passed to Xuggler's {@link IContainer}'s
   *         open method, and will result in IO being performed on the passed in
   *         streams.
   */

  public static String map(String url, InputStream inputStream)
  {
    return map(url, inputStream, null, DEFAULT_UNMAP_URL_ON_OPEN,
        DEFAULT_CLOSE_STREAM_ON_CLOSE);
  }

  /**
   * Maps an {@link OutputStream} to a URL for use by Xuggler.
   * 
   * Forwards to {@link #getFactory()}.
   * {@link #mapIO(String, InputStream, OutputStream)}
   * 
   * @return Returns a URL that can be passed to Xuggler's {@link IContainer}'s
   *         open method, and will result in IO being performed on the passed in
   *         streams.
   */

  public static String map(String url, OutputStream outputStream)
  {
    return map(url, null, outputStream, DEFAULT_UNMAP_URL_ON_OPEN,
        DEFAULT_CLOSE_STREAM_ON_CLOSE);
  }

  /**
   * Maps an {@link InputStream} or {@link OutputStream} to a URL for use by
   * Xuggler.
   * 
   * Forwards to {@link #getFactory()}.
   * {@link #mapIO(String, InputStream, OutputStream)}
   * 
   * @return Returns a URL that can be passed to Xuggler's {@link IContainer}'s
   *         open method, and will result in IO being performed on the passed in
   *         streams.
   */

  public static String map(String url, InputStream inputStream,
      OutputStream outputStream)
  {
    return map(url, inputStream, outputStream, DEFAULT_UNMAP_URL_ON_OPEN,
        DEFAULT_CLOSE_STREAM_ON_CLOSE);
  }

  /**
   * Maps an {@link InputStream} or {@link OutputStream} to a URL for use by
   * Xuggler.
   * 
   * Forwards to {@link #getFactory()}.
   * {@link #mapIO(String, InputStream, OutputStream, boolean, boolean)}
   * 
   * @return Returns a URL that can be passed to Xuggler's {@link IContainer}'s
   *         open method, and will result in IO being performed on the passed in
   *         streams.
   */

  public static String map(String url, InputStream inputStream,
      OutputStream outputStream, boolean unmapOnOpen, boolean closeStreamOnClose)
  {
    RegistrationInformation info = mFactory.mapIO(url, inputStream,
        outputStream, unmapOnOpen, closeStreamOnClose);
    if (info != null)
      throw new RuntimeException("url is already mapped: " + url);
    return DEFAULT_PROTOCOL + ":" + URLProtocolManager.getResourceFromURL(url);
  }

  /**
   * Undoes a URL to {@link InputStream} or {@link OutputStream} mapping.
   * Forwards to {@link #getFactory()}.{@link #unmapIO(String)}
   */
  public static RegistrationInformation unmap(String url)
  {
    return mFactory.unmapIO(url);
  }

  /**
   * Maps the given url or file name to the given {@link InputStream} or
   * {@link OutputStream} so that Xuggler calls to open the URL can use the
   * stream objects, and unmaps itself once Xuggler calls close on the stream.
   * 
   * <p>
   * 
   * When a Stream is mapped using this method, it will have
   * {@link #unmapIO(String)} automatically called when a Xuggler
   * {@link IContainer} opens the registered URL. It will also automatically
   * call {@link Closeable#close()} on the underlying {@link InputStream} or
   * {@link OutputStream} it used when Xuggler closes the {@link IContainer}.
   * 
   * </p>
   * 
   * @param url
   *          A file or URL. If a URL, the protocol will be stripped off and
   *          replaced with {@link #DEFAULT_PROTOCOL} when registering.
   * @param inputStream
   *          An input stream to use for reading data, or null if none.
   * @return The previous registration information for this url, or null if
   *         none.
   * 
   * @throws IllegalArgumentException
   *           if inputStream is null.
   */

  public RegistrationInformation mapIO(String url, InputStream inputStream)
  {
    if (inputStream == null)
      throw new IllegalArgumentException();
    return mapIO(url, inputStream, null, DEFAULT_UNMAP_URL_ON_OPEN,
        DEFAULT_CLOSE_STREAM_ON_CLOSE);
  }

  /**
   * Maps the given url or file name to the given {@link InputStream} or
   * {@link OutputStream} so that Xuggler calls to open the URL can use the
   * stream objects, and unmaps itself once Xuggler calls close on the stream.
   * 
   * <p>
   * 
   * When a Stream is mapped using this method, it will have
   * {@link #unmapIO(String)} automatically called when a Xuggler
   * {@link IContainer} opens the registered URL. It will also automatically
   * call {@link Closeable#close()} on the underlying {@link InputStream} or
   * {@link OutputStream} it used when Xuggler closes the {@link IContainer}.
   * 
   * </p>
   * 
   * @param url
   *          A file or URL. If a URL, the protocol will be stripped off and
   *          replaced with {@link #DEFAULT_PROTOCOL} when registering.
   * @param outputStream
   *          An output stream to use for reading data, or null if none.
   * @return The previous registration information for this url, or null if
   *         none.
   * 
   * @throws IllegalArgumentException
   *           if outputStream is null.
   */

  public RegistrationInformation mapIO(String url, OutputStream outputStream)
  {
    if (outputStream == null)
      throw new IllegalArgumentException();

    return mapIO(url, null, outputStream, DEFAULT_UNMAP_URL_ON_OPEN,
        DEFAULT_CLOSE_STREAM_ON_CLOSE);
  }

  /**
   * Maps the given url or file name to the given {@link InputStream} or
   * {@link OutputStream} so that Xuggler calls to open the URL can use the
   * stream objects, and unmaps itself once Xuggler opens this mapping for
   * reading or writing.
   * 
   * <p>
   * 
   * When a Stream is mapped using this method, it will have
   * {@link #unmapIO(String)} automatically called when a Xuggler
   * {@link IContainer} opens the registered URL. It will also automatically
   * call {@link Closeable#close()} on the underlying {@link InputStream} or
   * {@link OutputStream} it used when Xuggler closes the {@link IContainer}.
   * 
   * </p>
   * <p>
   * 
   * Both inputStream and outputStream may be specified (but at least one must
   * be non null). The factory will decide which to use based on whether it is
   * opened for reading (inputStream) or writing (outputStream).
   * 
   * </p>
   * 
   * @param url
   *          A file or URL. If a URL, the protocol will be stripped off and
   *          replaced with {@link #DEFAULT_PROTOCOL} when registering.
   * @param inputStream
   *          An input stream to use for reading data, or null if none.
   * @param outputStream
   *          An output stream to use for reading data, or null if none.
   * @return The previous registration information for this url, or null if
   *         none.
   * 
   * @throws IllegalArgumentException
   *           if both inputStream and outputStream are null.
   */

  public RegistrationInformation mapIO(String url, InputStream inputStream,
      OutputStream outputStream)
  {
    return mapIO(url, inputStream, outputStream, DEFAULT_UNMAP_URL_ON_OPEN,
        DEFAULT_CLOSE_STREAM_ON_CLOSE);
  }

  /**
   * Maps the given url or file name to the given {@link InputStream} or
   * {@link OutputStream} so that Xuggler calls to open the URL can use the Java
   * stream objects.
   * 
   * <p>
   * 
   * If you set unmapOnOpen to false, or you never actually open this mapping,
   * then you must ensure that you call {@link #unmapIO(String)} at some point
   * in time to remove the mapping, or we will hold onto references to the
   * {@link InputStream} or {@link OutputStream} you passed in.
   * 
   * </p>
   * 
   * @param url
   *          A file or URL. If a URL, the protocol will be stripped off and
   *          replaced with {@link #DEFAULT_PROTOCOL} when registering.
   * @param inputStream
   *          An input stream to use for reading data, or null if none.
   * @param outputStream
   *          An output stream to use for reading data, or null if none.
   * @param unmapOnOpen
   *          If true, the handler will unmap itself after an {@link IContainer}
   *          opens the registered URL. If true, you do not need to call
   *          {@link #unmapIO(String)} for this url.
   * @param closeStreamOnClose
   *          If true, the handler will call {@link Closeable#close()} on the
   *          {@link InputStream} or {@link OutputStream} it was using when
   *          {@link IURLProtocolHandler#close()} is called.
   * @return The previous registration information for this url, or null if
   *         none.
   * 
   * 
   * @throws IllegalArgumentException
   *           if both inputStream and outputStream are null.
   */

  public RegistrationInformation mapIO(String url, InputStream inputStream,
      OutputStream outputStream, boolean unmapOnOpen, boolean closeStreamOnClose)
  {
    if (url == null || url.length() <= 0)
      throw new IllegalArgumentException("must pass in non-zero url");
    if (inputStream == null && outputStream == null)
    {
      throw new IllegalArgumentException(
          "must pass in at least one input or output stream");
    }
    String streamName = URLProtocolManager.getResourceFromURL(url);
    RegistrationInformation tuple = new RegistrationInformation(streamName,
        inputStream, outputStream, unmapOnOpen, closeStreamOnClose);
    return mURLs.putIfAbsent(streamName, tuple);
  }

  /**
   * Unmaps the registration set by
   * {@link #mapIO(String, InputStream, OutputStream)}.
   * <p>
   * If URL contains a protocol it is ignored when trying to find the matching
   * IO stream.
   * </p>
   * 
   * @param url
   *          The stream name to unmap
   * @return the RegistrationInformation that had been registered for that url,
   *         or null if none.
   */
  public RegistrationInformation unmapIO(String url)
  {
    if (url == null || url.length() <= 0)
      throw new IllegalArgumentException("must pass in non-zero url");
    String streamName = URLProtocolManager.getResourceFromURL(url);
    return mURLs.remove(streamName);
  }

  /**
   * {@inheritDoc}
   */

  public IURLProtocolHandler getHandler(String protocol, String url, int flags)
  {
    // Note: We need to remove any protocol markers from the url
    String streamName = URLProtocolManager.getResourceFromURL(url);

    RegistrationInformation tuple = mURLs.get(streamName);
    if (tuple != null)
    {
      return new Handler(tuple);
    }
    return null;
  }

  /**
   * A set of information about a registered handler.
   * 
   * @author aclarke
   * 
   */

  public class RegistrationInformation
  {
    private final String mName;
    private final InputStream mInput;
    private final OutputStream mOutput;
    private final boolean mIsUnmappingOnOpen;
    private final boolean mIsClosingStreamOnClose;

    RegistrationInformation(String streamName, InputStream input,
        OutputStream output, boolean unmapOnOpen, boolean closeStreamOnClose)
    {
      mName = streamName;
      mInput = input;
      mOutput = output;
      mIsUnmappingOnOpen = unmapOnOpen;
      mIsClosingStreamOnClose = closeStreamOnClose;
    }

    /**
     * Get the factory that created this registration.
     * 
     * @return The factory.
     */
    public StreamIO getFactory()
    {
      return StreamIO.this;
    }

    /**
     * Get the input stream, if there is one.
     * 
     * @return the input stream, or null if none.
     */
    public InputStream getInput()
    {
      return mInput;
    }

    /**
     * Get the output stream, if there is one.
     * 
     * @return the output stream, or null if none.
     */
    public OutputStream getOutput()
    {
      return mOutput;
    }

    /**
     * The name of this handler registration, without any protocol.
     * 
     * @return the name
     */
    public String getName()
    {
      return mName;
    }

    /**
     * Should the handler unmap the stream after it is opened?
     * 
     * @return the decision
     */
    public boolean isUnmappingOnOpen()
    {
      return mIsUnmappingOnOpen;
    }

    /**
     * Should the handler call {@link Closeable#close()} when it closes?
     * 
     * @return the decision
     */
    public boolean isClosingStreamOnClose()
    {
      return mIsClosingStreamOnClose;
    }
  }

  /**
   * Implementation of URLProtocolHandler that can read from {@link InputStream}
   * objects or write to {@link OutputStream} objects.
   * 
   * <p>
   * 
   * The {@link IURLProtocolHandler#URL_RDWR} mode is not supported.
   * 
   * </p>
   * <p>
   * 
   * {@link IURLProtocolHandler#isStreamed(String, int)} always return true.
   * 
   * </p>
   * 
   * @author aclarke
   * 
   */

  static class Handler implements IURLProtocolHandler
  {
    private final RegistrationInformation mInfo;
    private final Logger log = LoggerFactory.getLogger(this.getClass());

    private Closeable mOpenStream = null;

    /**
     * Only usable by the package.
     */

    public Handler(RegistrationInformation tuple)
    {
      if (tuple == null)
        throw new IllegalArgumentException();
      log.trace("Initializing handler: {}", tuple.getName());
      mInfo = tuple;
    }

    /**
     * {@inheritDoc}
     */

    public int close()
    {
      log.trace("Closing stream: {}", mInfo.getName());
      int retval = 0;

      if (mOpenStream != null)
      {
        try
        {
          if (mInfo.isClosingStreamOnClose())
            mOpenStream.close();
        }
        catch (IOException e)
        {
          log.error("could not close stream {}: {}", mInfo.getName(), e);
          retval = -1;
        }
      }
      mOpenStream = null;
      return retval;
    }

    /**
     * {@inheritDoc}
     */

    public int open(String url, int flags)
    {
      log.trace("attempting to open {} with flags {}", url == null ? mInfo
          .getName() : url, flags);
      if (mInfo.isUnmappingOnOpen())
        // the unmapIO is an atomic operation
        if (!mInfo.equals(mInfo.getFactory().unmapIO(mInfo.getName())))
        {
          // someone already unmapped this stream
          log
              .error(
                  "stream {} already unmapped meaning it was likely already opened",
                  mInfo.getName());
          return -1;
        }

      if (mOpenStream != null)
      {
        log.debug("attempting to open already open handler: {}", mInfo
            .getName());
        return -1;
      }
      switch (flags)
      {
        case URL_RDWR:
          log.debug("do not support read/write mode for Java IO Handlers");
          return -1;
        case URL_WRONLY_MODE:
          mOpenStream = mInfo.getOutput();
          if (mOpenStream == null)
          {
            log.error("No OutputStream specified for writing: {}", mInfo
                .getName());
            return -1;
          }
          break;
        case URL_RDONLY_MODE:
          mOpenStream = mInfo.getInput();
          if (mOpenStream == null)
          {
            log.error("No InputStream specified for reading: {}", mInfo
                .getName());
            return -1;
          }
          break;
        default:
          log.error("Invalid flag passed to open: {}", mInfo.getName());
          return -1;
      }

      log.debug("Opened file: {}", mInfo.getName());
      return 0;
    }

    /**
     * {@inheritDoc}
     */

    public int read(byte[] buf, int size)
    {
      if (mOpenStream == null || !(mOpenStream instanceof InputStream))
        return -1;

      InputStream stream = (InputStream) mOpenStream;
      try
      {
        int ret = -1;
        ret = stream.read(buf, 0, size);
        // log.debug("Got result for read: {}", ret);
        return ret;
      }
      catch (IOException e)
      {
        log.error("Got IO exception reading from file: {}; {}",
            mInfo.getName(), e);
        return -1;
      }
    }

    /**
     * {@inheritDoc}
     * 
     * This method is not supported on this class and always return -1;
     */

    public long seek(long offset, int whence)
    {
      // not supported
      return -1;
    }

    /**
     * {@inheritDoc}
     */

    public int write(byte[] buf, int size)
    {
      if (mOpenStream == null || !(mOpenStream instanceof OutputStream))
        return -1;

      OutputStream stream = (OutputStream) mOpenStream;
      // log.debug("writing {} bytes to: {}", size, file);
      try
      {
        stream.write(buf, 0, size);
        return size;
      }
      catch (IOException e)
      {
        log.error("Got error writing to file: {}; {}", mInfo.getName(), e);
        return -1;
      }
    }

    /**
     * {@inheritDoc}
     * 
     * Always returns true for this class.
     */

    public boolean isStreamed(String url, int flags)
    {
      return true;
    }
  }
}

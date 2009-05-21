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
import java.io.InputStream;
import java.io.OutputStream;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentMap;

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
 * container.open(StreamProtocolHandlerFactory.map(&quot;yourkey&quot;, inputStream, null),
 *     IContainer.Type.READ, null);
 * </pre>
 * <p>
 * or for writing:
 * </p>
 * 
 * <pre>
 * IContainer container = IContainer.make();
 * container.open(StreamProtocolHandlerFactory.map(&quot;yourkey&quot;, null, outputStream),
 *     IContainer.Type.WRITE, null);
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
 * the mapping will be removed automagically when Xuggler closes the container
 * that opens the file.  That means that code like the examples above will
 * not leak old InputStream references.  If you prefer to manually unmap
 * your mappings, support is provided for that too.
 * 
 * </p>
 */

public class StreamProtocolHandlerFactory implements IURLProtocolHandlerFactory
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

  /** We don't allow people to create their own version of this factory */
  StreamProtocolHandlerFactory()
  {

  }

  /**
   * A set of information about a registered handler.
   * 
   * @author aclarke
   * 
   */

  class RegistrationInformation
  {
    private final String mName;
    private final InputStream mInput;
    private final OutputStream mOutput;
    private final boolean mIsUnmappingOnClose;
    private final boolean mIsClosingStreamOnClose;

    RegistrationInformation(String streamName, InputStream input,
        OutputStream output, boolean unmapOnClose, boolean closeStreamOnClose)
    {
      mName = streamName;
      mInput = input;
      mOutput = output;
      mIsUnmappingOnClose = unmapOnClose;
      mIsClosingStreamOnClose = closeStreamOnClose;
    }
    
    public StreamProtocolHandlerFactory getFactory()
    {
      return StreamProtocolHandlerFactory.this;
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
     * Should the handler call unmap the stream when it closes?
     * 
     * @return the decision
     */
    public boolean isUnmappingOnClose()
    {
      return mIsUnmappingOnClose;
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

  private ConcurrentMap<String, RegistrationInformation> mURLs = new ConcurrentHashMap<String, RegistrationInformation>();

  private final static StreamProtocolHandlerFactory mFactory = new StreamProtocolHandlerFactory();
  static
  {
    registerDefaultFactory();
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

  static public StreamProtocolHandlerFactory registerFactory(
      String protocolPrefix)
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

  static public StreamProtocolHandlerFactory getFactory()
  {
    return mFactory;
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
      return new StreamProtocolHandler(tuple);
    }
    return null;
  }

  /**
   * Forwards to {@link #getFactory()}.
   * {@link #mapIO(String, InputStream, OutputStream)}
   */

  public static String map(String url, InputStream inputStream,
      OutputStream outputStream)
  {
    return mFactory.mapIO(url, inputStream, outputStream);
  }

  public static String map(String url, InputStream inputStream,
      OutputStream outputStream, boolean unmapOnClose,
      boolean closeStreamOnClose)
  {
    return mFactory.mapIO(url, inputStream, outputStream, unmapOnClose,
        closeStreamOnClose);
  }

  /**
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
   * The return value can be passed directly to Xuggler and will be guaranteed
   * to map to the streams you passed in.
   * 
   * </p>
   * <p>
   * 
   * When a Stream is mapped using this method, it will have
   * {@link #unmapIO(String)} automatically called when Xuggler closes the
   * {@link StreamProtocolHandler} it is using. It will also automatically call
   * {@link Closeable#close()} on the underlying {@link InputStream} or
   * {@link OutputStream} it used.
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
   * @return A URL that can be passed directly to Xuggler for opening.
   * 
   * 
   * @throws IllegalArgumentException
   *           if both inputStream and outputStream are null.
   */

  public String mapIO(String url, InputStream inputStream,
      OutputStream outputStream)
  {
    return mapIO(url, inputStream, outputStream, true, true);
  }

  /**
   * Maps the given url or file name to the given {@link InputStream} or
   * {@link OutputStream} so that Xuggler calls to open the URL can use the
   * stream objects.
   * <p>
   * The return value can be passed directly to Xuggler and will be guaranteed
   * to map to the streams you passed in.
   * </p>
   * 
   * @param url
   *          A file or URL. If a URL, the protocol will be stripped off and
   *          replaced with {@link #DEFAULT_PROTOCOL} when registering.
   * @param inputStream
   *          An input stream to use for reading data, or null if none.
   * @param outputStream
   *          An output stream to use for reading data, or null if none.
   * @param unmapOnClose
   *          If true, the handler will unmap itself with
   *          {@link IURLProtocolHandler#close()} is called.
   * @param closeStreamOnClose
   *          If true, the handler will call {@link Closeable#close()} on the
   *          {@link InputStream} or {@link OutputStream} it was using when
   *          {@link IURLProtocolHandler#close()} is called.
   * @return A URL that can be passed directly to Xuggler for opening.
   * 
   * 
   * @throws IllegalArgumentException
   *           if both inputStream and outputStream are null.
   */

  public String mapIO(String url, InputStream inputStream,
      OutputStream outputStream, boolean unmapOnClose,
      boolean closeStreamOnClose)
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
        inputStream, outputStream, unmapOnClose, closeStreamOnClose);
    RegistrationInformation oldTuple = mURLs.putIfAbsent(streamName, tuple);
    if (oldTuple != null)
      throw new RuntimeException(
          "this factory already has a registration for: " + streamName);
    return DEFAULT_PROTOCOL + ":" + streamName;
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

  private static void registerDefaultFactory()
  {
    registerFactory(DEFAULT_PROTOCOL);
  }
}

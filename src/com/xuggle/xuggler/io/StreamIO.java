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
import java.io.DataInput;
import java.io.DataOutput;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.RandomAccessFile;
import java.nio.ByteBuffer;
import java.nio.channels.ByteChannel;
import java.nio.channels.ReadableByteChannel;
import java.nio.channels.WritableByteChannel;
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
   * Maps a {@link DataInput} object to a URL for use by Xuggler.
   * 
   * @param url
   *          the URL to use.
   * @param input
   *          the {@link DataInput}
   * @return a string that can be passed to {@link IContainer}'s open methods.
   */
  public static String map(String url, DataInput input)
  {
    return map(url, input, null, DEFAULT_UNMAP_URL_ON_OPEN);
  }

  /**
   * Maps a {@link DataOutput} object to a URL for use by Xuggler.
   * 
   * @param url
   *          the URL to use.
   * @param output
   *          the {@link DataOutput}
   * @return a string that can be passed to {@link IContainer}'s open methods.
   */
  public static String map(String url, DataOutput output)
  {
    return map(url, null, output, DEFAULT_UNMAP_URL_ON_OPEN);
  }

  /**
   * Maps a {@link RandomAccessFile} object to a URL for use by Xuggler.
   * 
   * @param url
   *          the URL to use.
   * @param file
   *          the {@link RandomAccessFile}
   * @return a string that can be passed to {@link IContainer}'s open methods.
   */
  public static String map(String url, RandomAccessFile file)
  {
    return map(url, file, file, DEFAULT_UNMAP_URL_ON_OPEN);
  }

  /**
   * Maps a {@link ReadableByteChannel} to a URL for use by Xuggler.
   * 
   * @param url
   *          the URL to use.
   * @param channel
   *          the {@link ReadableByteChannel}
   * @return a string that can be passed to {@link IContainer}'s open methods.
   */
  public static String map(String url, ReadableByteChannel channel)
  {
    return map(url, channel, null, DEFAULT_UNMAP_URL_ON_OPEN,
        DEFAULT_CLOSE_STREAM_ON_CLOSE);
  }

  /**
   * Maps a {@link WritableByteChannel} to a URL for use by Xuggler.
   * 
   * @param url
   *          the URL to use.
   * @param channel
   *          the {@link WritableByteChannell}
   * @return a string that can be passed to {@link IContainer}'s open methods.
   */
  public static String map(String url, WritableByteChannel channel)
  {
    return map(url, null, channel, DEFAULT_UNMAP_URL_ON_OPEN,
        DEFAULT_CLOSE_STREAM_ON_CLOSE);
  }

  /**
   * Maps a {@link ByteChannel} to a URL for use by Xuggler.
   * 
   * @param url
   *          the URL to use.
   * @param channel
   *          the {@link ByteChannel}
   * @return a string that can be passed to {@link IContainer}'s open methods.
   */
  public static String map(String url, ByteChannel channel)
  {
    return map(url, channel, channel, DEFAULT_UNMAP_URL_ON_OPEN,
        DEFAULT_CLOSE_STREAM_ON_CLOSE);
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
   * Maps a {@link DataInput} or {@link DataOutput} object to a URL for use by
   * Xuggler.
   * 
   * Forwards to {@link #getFactory()}.
   * {@link #mapIO(String, DataInput, DataOutput, boolean, boolean)}
   * 
   * @return Returns a URL that can be passed to Xuggler's {@link IContainer}'s
   *         open method, and will result in IO being performed on the passed in
   *         streams.
   */

  public static String map(String url, DataInput input, DataOutput output,
      boolean unmapOnOpen)
  {
    RegistrationInformation info = mFactory.mapIO(url, input, output,
        unmapOnOpen);
    if (info != null)
      throw new RuntimeException("url is already mapped: " + url);
    return DEFAULT_PROTOCOL + ":" + URLProtocolManager.getResourceFromURL(url);
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

  public static String map(String url, ReadableByteChannel readChannel,
      WritableByteChannel writeChannel, boolean unmapOnOpen,
      boolean closeChannelOnClose)
  {
    RegistrationInformation info = mFactory.mapIO(url, readChannel,
        writeChannel, unmapOnOpen, closeChannelOnClose);
    if (info != null)
      throw new RuntimeException("url is already mapped: " + url);
    return DEFAULT_PROTOCOL + ":" + URLProtocolManager.getResourceFromURL(url);
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
   * Maps the given url or file name to the given {@link ReadableByteChannel} or
   * {@link WritableByteChannel} so that Xuggler calls to open the URL can use
   * the Java channel objects.
   * 
   * <p>
   * 
   * If you set unmapOnOpen to false, or you never actually open this mapping,
   * then you must ensure that you call {@link #unmapIO(String)} at some point
   * in time to remove the mapping, or Xuggler will hold onto references to the
   * channels passed in.
   * 
   * </p>
   * 
   * @param url
   *          A file or URL. If a URL, the protocol will be stripped off and
   *          replaced with {@link #DEFAULT_PROTOCOL} when registering.
   * @param input
   *          An input channel to use for reading data, or null if none.
   * @param output
   *          An output channel to use for reading data, or null if none.
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


  public RegistrationInformation mapIO(String url, ReadableByteChannel input,
      WritableByteChannel output, boolean unmapOnOpen,
      boolean closeChannelOnClose)
  {
    if (url == null || url.length() <= 0)
      throw new IllegalArgumentException("must pass in non-zero url");
    if (input == null && output == null)
    {
      throw new IllegalArgumentException(
          "must pass in at least one input or one output channel");
    }
    String streamName = URLProtocolManager.getResourceFromURL(url);
    RegistrationInformation tuple = new RegistrationInformation(streamName,
        input, output, unmapOnOpen, closeChannelOnClose);
    return mURLs.putIfAbsent(streamName, tuple);
  }

  /**
   * Maps the given url or file name to the given {@link DataInput} or
   * {@link DataOutput} so that Xuggler calls to open the URL can use
   * the Java channel objects.
   * 
   * <p>
   * 
   * If you set unmapOnOpen to false, or you never actually open this mapping,
   * then you must ensure that you call {@link #unmapIO(String)} at some point
   * in time to remove the mapping, or we will hold onto references to the
   * i/o objects you passed in.
   * 
   * </p>
   * 
   * @param url
   *          A file or URL. If a URL, the protocol will be stripped off and
   *          replaced with {@link #DEFAULT_PROTOCOL} when registering.
   * @param input
   *          An input object to use for reading data, or null if none.
   * @param output
   *          An output objectto use for reading data, or null if none.
   * @param unmapOnOpen
   *          If true, the handler will unmap itself after an {@link IContainer}
   *          opens the registered URL. If true, you do not need to call
   *          {@link #unmapIO(String)} for this url.
   * @return The previous registration information for this url, or null if
   *         none.
   * 
   * 
   * @throws IllegalArgumentException
   *           if both input and output are null.
   */

  public RegistrationInformation mapIO(String url, DataInput input,
      DataOutput output, boolean unmapOnOpen)
  {
    if (url == null || url.length() <= 0)
      throw new IllegalArgumentException("must pass in non-zero url");
    if (input == null && output == null)
    {
      throw new IllegalArgumentException(
          "must pass in at least one input or one output channel");
    }
    String streamName = URLProtocolManager.getResourceFromURL(url);
    RegistrationInformation tuple = new RegistrationInformation(streamName,
        input, output, unmapOnOpen);
    return mURLs.putIfAbsent(streamName, tuple);
  }

  /**
   * Unmaps a registration between a URL and the underlying i/o objects.
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
    private final ReadableByteChannel mInputChannel;
    private final WritableByteChannel mOutputChannel;
    private final DataInput mDataInput;
    private final DataOutput mDataOutput;

    private final boolean mIsUnmappingOnOpen;
    private final boolean mIsClosingStreamOnClose;

    RegistrationInformation(String streamName, InputStream input,
        OutputStream output, boolean unmapOnOpen, boolean closeStreamOnClose)
    {
      mName = streamName;
      mInput = input;
      mOutput = output;
      mInputChannel = null;
      mOutputChannel = null;
      mDataInput = null;
      mDataOutput = null;
      mIsUnmappingOnOpen = unmapOnOpen;
      mIsClosingStreamOnClose = closeStreamOnClose;
    }

    RegistrationInformation(String streamName,
        ReadableByteChannel inputChannel, WritableByteChannel outputChannel,
        boolean unmapOnOpen, boolean closeStreamOnClose)
    {
      mName = streamName;
      mInput = null;
      mOutput = null;
      mInputChannel = inputChannel;
      mOutputChannel = outputChannel;
      mDataInput = null;
      mDataOutput = null;
      mIsUnmappingOnOpen = unmapOnOpen;
      mIsClosingStreamOnClose = closeStreamOnClose;
    }

    public RegistrationInformation(String streamName, DataInput input,
        DataOutput output, boolean unmapOnOpen)
    {
      mName = streamName;
      mInput = null;
      mOutput = null;
      mInputChannel = null;
      mOutputChannel = null;
      mDataInput = input;
      mDataOutput = output;
      mIsUnmappingOnOpen = unmapOnOpen;
      mIsClosingStreamOnClose = false;
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

    public ReadableByteChannel getInputChannel()
    {
      return mInputChannel;
    }

    public WritableByteChannel getOutputChannel()
    {
      return mOutputChannel;
    }

    public DataInput getDataInput()
    {
      return mDataInput;
    }

    public DataOutput getDataOutput()
    {
      return mDataOutput;
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

    private Object mOpenStream = null;

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

      if (mOpenStream != null && mOpenStream instanceof Closeable)
      {
        try
        {
          if (mInfo.isClosingStreamOnClose())
            ((Closeable) mOpenStream).close();
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
          mOpenStream = mInfo.getDataInput();
          if (!(mOpenStream instanceof RandomAccessFile))
            mOpenStream = mInfo.getDataOutput();
          if (!(mOpenStream instanceof RandomAccessFile))
          {
            log.debug("do not support read/write mode for Java IO Handlers");
            return -1;
          }
          break;
        case URL_WRONLY_MODE:
          mOpenStream = mInfo.getOutput();
          if (mOpenStream == null)
            mOpenStream = mInfo.getOutputChannel();
          if (mOpenStream == null)
            mOpenStream = mInfo.getDataOutput();
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
            mOpenStream = mInfo.getInputChannel();
          if (mOpenStream == null)
            mOpenStream = mInfo.getDataInput();
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
      int ret = -1;
      if (mOpenStream == null)
        return -1;

      try
      {
        if (mOpenStream instanceof InputStream)
        {
          InputStream stream = (InputStream) mOpenStream;
          ret = stream.read(buf, 0, size);
          // log.debug("Got result for read: {}", ret);
          return ret;
        }
        else if (mOpenStream instanceof DataInput)
        {
          DataInput input = (DataInput) mOpenStream;
          if (mOpenStream instanceof RandomAccessFile) {
            RandomAccessFile file = (RandomAccessFile) input;
            ret = file.read(buf, 0, size);
            return ret;
          } else {
            input.readFully(buf, 0, size);
            ret = size;
            return ret;
          }
        }
        else if (mOpenStream instanceof ReadableByteChannel)
        {
          ReadableByteChannel channel = (ReadableByteChannel) mOpenStream;
          ByteBuffer buffer = ByteBuffer.allocate(size);
          ret = channel.read(buffer);
          buffer.flip();
          buffer.put(buf, 0, ret);
          return ret;
        }
      }
      catch (IOException e)
      {
        log.error("Got IO exception reading from file: {}; {}",
            mInfo.getName(), e);
        return -1;
      }

      return ret;
    }

    /**
     * {@inheritDoc}
     * 
     * This method is not supported on this class and always return -1;
     */

    public long seek(long offset, int whence)
    {
      final RandomAccessFile file;
      if (mInfo.getDataInput() instanceof RandomAccessFile)
        file = (RandomAccessFile) mInfo.getDataInput();
      else if (mInfo.getDataOutput() instanceof RandomAccessFile)
        file = (RandomAccessFile) mInfo.getDataOutput();
      else
        file = null;
      if (file == null)
        return -1;

      try
      {
        final long seek;
        if (whence == SEEK_SET)
          seek = offset;
        else if (whence == SEEK_CUR)
          seek = file.getFilePointer() + offset;
        else if (whence == SEEK_END)
          seek = file.length() + offset;
        else if (whence == SEEK_SIZE)
          // odd feature of the protocol handler; this request
          // just returns the file size without actually seeking
          return (int) file.length();
        else
        {
          log.error("invalid seek value \"{}\" for file: {}", whence, file);
          return -1;
        }

        file.seek(seek);
        log.debug("seeking to \"{}\" in: {}", seek, file);
        return seek;
      }
      catch (IOException e)
      {
        log.error("got io exception \"{}\" while seeking in: {}", e
            .getMessage(), file);
        e.printStackTrace();
        return -1;
      }
    }

    /**
     * {@inheritDoc}
     */

    public int write(byte[] buf, int size)
    {
      if (mOpenStream == null)
        return -1;

      try
      {
        if (mOpenStream instanceof OutputStream)
        {
          OutputStream stream = (OutputStream) mOpenStream;
          // log.debug("writing {} bytes to: {}", size, file);
          stream.write(buf, 0, size);
          return size;
        }
        else if (mOpenStream instanceof WritableByteChannel)
        {
          WritableByteChannel channel = (WritableByteChannel) mOpenStream;
          ByteBuffer buffer = ByteBuffer.allocate(size);
          buffer.put(buf, 0, size);
          buffer.flip();
          return channel.write(buffer);
        }
        else if (mOpenStream instanceof DataOutput)
        {
          DataOutput output = (DataOutput) mOpenStream;
          output.write(buf, 0, size);
          return size;


        }
      }
      catch (IOException e)
      {
        log.error("Got error writing to file: {}; {}", mInfo.getName(), e);
        return -1;
      }
      return -1;
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

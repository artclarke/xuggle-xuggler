package com.xuggle.xuggler.io;

import com.xuggle.xuggler.IContainer;
import com.xuggle.xuggler.io.IURLProtocolHandler;

/**
 * The NullProtocolHandler implements {@link IURLProtocolHandler}, but discards
 * any data written and always returns 0 for reading.
 * <p>
 * This is useful when you want to set up an {@link IContainer} instance to
 * encode data, but don't want to actually write the data out after encoding (because
 * all you really want are muxed packets)
 * </p>
 * @author aclarke
 *
 */
public class NullProtocolHandler implements IURLProtocolHandler
{

  // package level so other folks can't create it.
  NullProtocolHandler()
  {
  }
  
  public int close()
  {
    // Always succeed
    return 0;
  }

  public boolean isStreamed(String aUrl, int aFlags)
  {
    // We're not streamed because, well, we do nothing.
    return false;
  }

  public int open(String aUrl, int aFlags)
  {
    // Always succeed
    return 0;
  }

  public int read(byte[] aBuf, int aSize)
  {
    // always read zero bytes
    return 0;
  }

  public long seek(long aOffset, int aWhence)
  {
    // always seek to where we're asked to seek to
    return aOffset;
  }

  public int write(byte[] aBuf, int aSize)
  {
    // always write all bytes
    return aSize;
  }

}

package com.xuggle.mediatool;

import com.xuggle.xuggler.IContainer;
import com.xuggle.xuggler.IStreamCoder;

public interface IMediaTool extends IMediaPipe
{

  /** 
   * Get the underlying media {@link IContainer} that the media tool is
   * reading from or writing to.  The returned {@link IContainer} can
   * be further interrogated for media stream details.
   *
   * @return the media container.
   */

  public abstract IContainer getContainer();

  /**
   * The URL from which media is being read or written to.
   * 
   * @return the source or destination URL.
   */

  public abstract String getUrl();

  /** 
   * Open this media tool.  This will open the internal {@link
   * IContainer}.  Typically the tool will open itself at the right
   * time, but there may exist rare cases where the calling context
   * may need to open the tool.
   */

  public abstract void open();

  /**
   * Test if this media tool is open.
   * 
   * @return true if the media tool is open.
   */

  public abstract boolean isOpen();
    
  /** 
   * Close this media tool.  This will close all {@link IStreamCoder}s
   * explicitly opened by tool, then close the internal {@link
   * IContainer}, again only if it was explicitly opened by tool.
   * 
   * <p> Typically the tool will close itself at the right time, but there
   * are instances where the calling context may need to close the
   * tool. </p>
   */

  public abstract void close();
}

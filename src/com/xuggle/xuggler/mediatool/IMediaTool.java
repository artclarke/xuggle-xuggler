package com.xuggle.xuggler.mediatool;

import com.xuggle.xuggler.IContainer;

public interface IMediaTool
{
  /** Add a media reader listener.
  *
  * @param listener the listener to add
  *
  * @return true if this collection changed as a result of the call
  */
 
  public abstract boolean addListener(IMediaListener listener);

  /** Remove a media reader listener.
  *
  * @param listener the listener to remove
  *
  * @return true if this collection changed as a result of the call
  */
 
  public abstract boolean removeListener(IMediaListener listener);

  /** Get the underlying media {@link IContainer} that this reader is
   * using to decode streams.  Listeners can use stream index values to
   * learn more about particular streams they are receiving data from.
   *
   * @return the stream container.
   */

  public abstract IContainer getContainer();

  /** Reset this {@link MediaReader} object, closing all elements we opened.
   * For example, if the {@link MediaReader} opened the {@link IContainer},
   * it closes it now.
   */

  public abstract void close();
}

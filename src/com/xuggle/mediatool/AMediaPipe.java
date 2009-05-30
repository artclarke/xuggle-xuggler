package com.xuggle.mediatool;

import java.util.Collection;
import java.util.Collections;
import java.util.concurrent.CopyOnWriteArrayList;


/**
 * An abstract class that provides an implementation of
 * {@link IMediaPipe}.
 * 
 * <p>
 * This class does not actually declare it implements those
 * methods so that deriving classes can decide which if any
 * they want to expose.
 * </p>
 * 
 * @author aclarke
 *
 */
public abstract class AMediaPipe
// We deliberately don't declare these, but they
// are commented out so you can easily add them to check compliance
//implements IMediaPipeListener, IMediaPipe
{

  private final Collection<IMediaPipeListener> mListeners = new CopyOnWriteArrayList<IMediaPipeListener>();

  /**
   * Create an AMediaPipe
   */
  public AMediaPipe()
  {
    super();
  }

  /**
   * Default implementation for {@link IMediaPipe#addListener(IMediaPipeListener)}
   */
  public boolean addListener(IMediaPipeListener listener)
  {
    return mListeners.add(listener);
  }

  /**
   * Default implementation for {@link IMediaPipe#removeListener(IMediaPipeListener)}
   */
  public boolean removeListener(IMediaPipeListener listener)
  {
    return mListeners.remove(listener);
  }

  /**
   * Default implementation for {@link IMediaPipe#getListeners()}
   */
  public Collection<IMediaPipeListener> getListeners()
  {
    return Collections.unmodifiableCollection(mListeners);
  }

}

package com.xuggle.mediatool;

import java.awt.image.BufferedImage;
import java.util.Collection;
import java.util.Collections;
import java.util.concurrent.CopyOnWriteArrayList;

import com.xuggle.xuggler.IAudioSamples;
import com.xuggle.xuggler.IPacket;
import com.xuggle.xuggler.IVideoPicture;

/**
 * An abstract class that provides implementations of
 * {@link IMediaPipe} and {@link IMediaPipeListener}.
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
abstract class AMediaPipe
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

  /**
   * Default implementation for
   * {@link IMediaPipeListener#onAddStream(IMediaPipe, int)}
   */
  public void onAddStream(IMediaPipe tool, int streamIndex)
  {
    for (IMediaPipeListener listener : mListeners)
      listener.onAddStream(tool, streamIndex);
  }

  /**
   * Default implementation for
   * {@link IMediaPipeListener#onAudioSamples(IMediaPipe, IAudioSamples, int)}
   */
  public void onAudioSamples(IMediaPipe tool, IAudioSamples samples,
      int streamIndex)
  {
    for (IMediaPipeListener listener : mListeners)
      listener.onAudioSamples(tool, samples, streamIndex);
  }

  /**
   * Default implementation for
   * {@link IMediaPipeListener#onClose(IMediaPipe)}
   */
  public void onClose(IMediaPipe tool)
  {
    for (IMediaPipeListener listener : mListeners)
      listener.onClose(tool);
  }

  
  /**
   * Default implementation for
   * {@link IMediaPipeListener#onCloseCoder(IMediaPipe, Integer)}
   */
  public void onCloseCoder(IMediaPipe tool, Integer coderIndex)
  {
    for (IMediaPipeListener listener : mListeners)
      listener.onCloseCoder(tool, coderIndex);
  }

  /**
   * Default implementation for
   * {@link IMediaPipeListener#onFlush(IMediaPipe)}
   */
  public void onFlush(IMediaPipe tool)
  {
    for (IMediaPipeListener listener : mListeners)
      listener.onFlush(tool);
  }

  /**
   * Default implementation for
   * {@link IMediaPipeListener#onOpen(IMediaPipe)}
   */
  public void onOpen(IMediaPipe tool)
  {
    for (IMediaPipeListener listener : mListeners)
      listener.onOpen(tool);
  }

  /**
   * Default implementation for
   * {@link IMediaPipeListener#onOpenCoder(IMediaPipe, Integer)}
   */
  public void onOpenCoder(IMediaPipe tool, Integer coderIndex)
  {
    for (IMediaPipeListener listener : mListeners)
      listener.onOpenCoder(tool, coderIndex);
  }

  /**
   * Default implementation for
   * {@link IMediaPipeListener#onReadPacket(IMediaPipe, IPacket)}
   */
  public void onReadPacket(IMediaPipe tool, IPacket packet)
  {
    for (IMediaPipeListener listener : mListeners)
      listener.onReadPacket(tool, packet);
  }

  /**
   * Default implementation for
   * {@link IMediaPipeListener#onVideoPicture(IMediaPipe, IVideoPicture, BufferedImage, int)}
   */
  public void onVideoPicture(IMediaPipe tool, IVideoPicture picture,
      BufferedImage image, int streamIndex)
  {
    for (IMediaPipeListener listener : mListeners)
      listener.onVideoPicture(tool, picture, image, streamIndex);
  }

  /**
   * Default implementation for
   * {@link IMediaPipeListener#onWriteHeader(IMediaPipe)}
   */
  public void onWriteHeader(IMediaPipe tool)
  {
    for (IMediaPipeListener listener : mListeners)
      listener.onWriteHeader(tool);
  }

  /**
   * Default implementation for
   * {@link IMediaPipeListener#onWritePacket(IMediaPipe, IPacket)}
   */
  public void onWritePacket(IMediaPipe tool, IPacket packet)
  {
    for (IMediaPipeListener listener : mListeners)
      listener.onWritePacket(tool, packet);
  }

  /**
   * Default implementation for
   * {@link IMediaPipeListener#onWriteTrailer(IMediaPipe)}
   */
  public void onWriteTrailer(IMediaPipe tool)
  {
    for (IMediaPipeListener listener : mListeners)
      listener.onWriteTrailer(tool);
  }

}

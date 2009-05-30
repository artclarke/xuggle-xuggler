package com.xuggle.mediatool;

import java.awt.image.BufferedImage;

import com.xuggle.xuggler.IAudioSamples;
import com.xuggle.xuggler.IPacket;
import com.xuggle.xuggler.IVideoPicture;

/**
 * An abstract implementation
 * of {@link IMediaPipe} and of
 * {@link IMediaPipeListener} that just forwards each event
 * to all the registered listeners.
 * @author aclarke
 *
 */
public class AMediaPipeEventGenerator extends AMediaPipe
{

  /**
   * Default implementation for
   * {@link IMediaPipeListener#onAddStream(IMediaPipe, int)}
   */
  public void onAddStream(IMediaPipe tool, int streamIndex)
  {
    for (IMediaPipeListener listener : getListeners())
      listener.onAddStream(tool, streamIndex);
  }

  /**
   * Default implementation for
   * {@link IMediaPipeListener#onAudioSamples(IMediaPipe, IAudioSamples, int)}
   */
  public void onAudioSamples(IMediaPipe tool, IAudioSamples samples, int streamIndex)
  {
    for (IMediaPipeListener listener : getListeners())
      listener.onAudioSamples(tool, samples, streamIndex);
  }

  /**
   * Default implementation for
   * {@link IMediaPipeListener#onClose(IMediaPipe)}
   */
  public void onClose(IMediaPipe tool)
  {
    for (IMediaPipeListener listener : getListeners())
      listener.onClose(tool);
  }

  /**
   * Default implementation for
   * {@link IMediaPipeListener#onCloseCoder(IMediaPipe, Integer)}
   */
  public void onCloseCoder(IMediaPipe tool, Integer coderIndex)
  {
    for (IMediaPipeListener listener : getListeners())
      listener.onCloseCoder(tool, coderIndex);
  }

  /**
   * Default implementation for
   * {@link IMediaPipeListener#onFlush(IMediaPipe)}
   */
  public void onFlush(IMediaPipe tool)
  {
    for (IMediaPipeListener listener : getListeners())
      listener.onFlush(tool);
  }

  /**
   * Default implementation for
   * {@link IMediaPipeListener#onOpen(IMediaPipe)}
   */
  public void onOpen(IMediaPipe tool)
  {
    for (IMediaPipeListener listener : getListeners())
      listener.onOpen(tool);
  }

  /**
   * Default implementation for
   * {@link IMediaPipeListener#onOpenCoder(IMediaPipe, Integer)}
   */
  public void onOpenCoder(IMediaPipe tool, Integer coderIndex)
  {
    for (IMediaPipeListener listener : getListeners())
      listener.onOpenCoder(tool, coderIndex);
  }

  /**
   * Default implementation for
   * {@link IMediaPipeListener#onReadPacket(IMediaPipe, IPacket)}
   */
  public void onReadPacket(IMediaPipe tool, IPacket packet)
  {
    for (IMediaPipeListener listener : getListeners())
      listener.onReadPacket(tool, packet);
  }

  /**
   * Default implementation for
   * {@link IMediaPipeListener#onVideoPicture(IMediaPipe, IVideoPicture, BufferedImage, int)}
   */
  public void onVideoPicture(IMediaPipe tool, IVideoPicture picture, BufferedImage image,
      int streamIndex)
  {
    for (IMediaPipeListener listener : getListeners())
      listener.onVideoPicture(tool, picture, image, streamIndex);
  }

  /**
   * Default implementation for
   * {@link IMediaPipeListener#onWriteHeader(IMediaPipe)}
   */
  public void onWriteHeader(IMediaPipe tool)
  {
    for (IMediaPipeListener listener : getListeners())
      listener.onWriteHeader(tool);
  }

  /**
   * Default implementation for
   * {@link IMediaPipeListener#onWritePacket(IMediaPipe, IPacket)}
   */
  public void onWritePacket(IMediaPipe tool, IPacket packet)
  {
    for (IMediaPipeListener listener : getListeners())
      listener.onWritePacket(tool, packet);
  }

  /**
   * Default implementation for
   * {@link IMediaPipeListener#onWriteTrailer(IMediaPipe)}
   */
  public void onWriteTrailer(IMediaPipe tool)
  {
    for (IMediaPipeListener listener : getListeners())
      listener.onWriteTrailer(tool);
  }

}

package com.xuggle.xuggler.mediatool;

import java.awt.image.BufferedImage;

import com.xuggle.xuggler.IPacket;
import com.xuggle.xuggler.IStream;
import com.xuggle.xuggler.IAudioSamples;
import com.xuggle.xuggler.IVideoPicture;

/** 
 * The IMediaListener receives events which occure in {@link
 * IMediaTool}, specifically {@link MediaReader} and {@link
 * MediaWriter}.
 *
 * <p>
 * 
 * These methods will block continued media processing by operation of
 * calling tool.
 *
 * </p>
 */

public interface IMediaListener
{
  /**
   * Called after a video frame has been decoded by a {@link
   * MediaReader} or encode by a {@link MediaWriter}.  Optionally a
   * BufferedImage version of the frame may be passed.
   *
   * @param tool the tool that generated this event
   * @param picture a raw video picture
   * @param image the buffered image, which may be null
   * @param streamIndex the index of the stream this object was decoded from.
   */

  public void onVideoPicture(IMediaTool tool, IVideoPicture picture,
    BufferedImage image, int streamIndex);
  
  /**
   * Called after audio samples have been decoded by a {@link
   * MediaReader} or encode by a {@link MediaWriter}.
   *
   * @param tool the tool that generated this event
   * @param samples a audio samples
   * @param streamIndex the index of the stream
   */

  public void onAudioSamples(IMediaTool tool, IAudioSamples samples, int streamIndex);

  /**
   * Called after an {@link com.xuggle.xuggler.IContainer} is opened.
   *
   * @param tool the tool that generated this event
   */

  public void onOpen(IMediaTool tool);

  /**
   * Called after an {@link com.xuggle.xuggler.IContainer} is closed.
   * 
   * @param tool the tool that generated this event
   */

  public void onClose(IMediaTool tool);

  /**
   * Called after an {@link com.xuggle.xuggler.IStream} is opened.
   *
   * @param tool the tool that generated this event
   * @param stream the stream opened
   */

  public void onOpenStream(IMediaTool tool, IStream stream);

  /**
   * Called after an {@link com.xuggle.xuggler.IStream} is closed.
   *
   * @param tool the tool that generated this event
   * @param stream the stream just closed
   */

  public void onCloseStream(IMediaTool tool, IStream stream);

  /**
   * Called after a {@link com.xuggle.xuggler.IPacket} has been read by
   * a {@link MediaReader}.
   *
   * @param tool the tool that generated this event
   * @param packet the packet just read
   */

  public void onReadPacket(IMediaTool tool, IPacket packet);

  /**
   * Called after a {@link com.xuggle.xuggler.IPacket} has been written
   * by a {@link MediaWriter}.
   *
   * @param tool the tool that generated this event
   * @param packet the packet just written
   */

  public void onWritePacket(IMediaTool tool, IPacket packet);

  /**
   * Called after a {@link MediaWriter} has flushed its buffers.
   *
   * @param tool the tool that generated this event
   */

  public void onFlush(IMediaTool tool);
}

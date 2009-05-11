package com.xuggle.xuggler.mediatool;

import java.awt.image.BufferedImage;

import com.xuggle.xuggler.IPacket;
import com.xuggle.xuggler.IStream;
import com.xuggle.xuggler.IAudioSamples;
import com.xuggle.xuggler.IVideoPicture;

/** An implementation of {@link IMediaListener} that implements all methods
 * as NO-OP methods.  This can be useful if you only want to override
 * some members of {@link IMediaListener}; instead, just subclass this and
 * override the methods you want.
 */

public class MediaAdapter implements IMediaListener
{
  /** {@inheritDoc} */

  public void onVideoPicture(IMediaTool tool, IVideoPicture picture, 
    BufferedImage image, int streamIndex)
  {
  }
  
  /** {@inheritDoc} */

  public void onAudioSamples(IMediaTool tool, IAudioSamples samples, int streamIndex)
  {
  }

  /** {@inheritDoc} */

  public void onOpen(IMediaTool tool)
  {
  }

  /** {@inheritDoc} */

  public void onClose(IMediaTool tool)
  {
  }

  /** {@inheritDoc} */

  public void onOpenStream(IMediaTool tool, IStream stream)
  {
  }

  /** {@inheritDoc} */

  public void onCloseStream(IMediaTool tool, IStream stream)
  {
  }

  /** {@inheritDoc} */

  public void onReadPacket(IMediaTool tool, IPacket packet)
  {
  }

  /** {@inheritDoc} */

  public void onWritePacket(IMediaTool tool, IPacket packet)
  {
  }

  /** {@inheritDoc} */

  public void onFlush(IMediaTool tool)
  {
  }
}

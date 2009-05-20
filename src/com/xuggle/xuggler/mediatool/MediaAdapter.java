/*
 * This file is part of Xuggler.
 * 
 * Xuggler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 * 
 * Xuggler is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public
 * License along with Xuggler.  If not, see <http://www.gnu.org/licenses/>.
 */

package com.xuggle.xuggler.mediatool;

import java.awt.image.BufferedImage;

import com.xuggle.xuggler.IPacket;
import com.xuggle.xuggler.IStream;
import com.xuggle.xuggler.IAudioSamples;
import com.xuggle.xuggler.IVideoPicture;

/**
 * An implementation of {@link IMediaListener} that implements all methods as
 * NO-OP methods.
 * 
 * <p>
 * 
 * This can be useful if you only want to override some members of
 * {@link IMediaListener}; instead, just subclass this and override the methods
 * you want.
 * 
 * </p>
 */

public class MediaAdapter implements IMediaListener
{
  /** {@inheritDoc} */

  public void onVideoPicture(IMediaTool tool, IVideoPicture picture,
      BufferedImage image, int streamIndex)
  {
  }

  /** {@inheritDoc} */

  public void onAudioSamples(IMediaTool tool, IAudioSamples samples,
      int streamIndex)
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

  public void onAddStream(IMediaTool tool, IStream stream)
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

  public void onWriteHeader(IMediaTool tool)
  {
  }

  /** {@inheritDoc} */

  public void onFlush(IMediaTool tool)
  {
  }

  /** {@inheritDoc} */

  public void onWriteTrailer(IMediaTool tool)
  {
  }
}

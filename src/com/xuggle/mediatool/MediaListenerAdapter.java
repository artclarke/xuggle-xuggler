/*
 * Copyright (c) 2008, 2009 by Xuggle Incorporated.  All rights reserved.
 * 
 * This file is part of Xuggler.
 * 
 * You can redistribute Xuggler and/or modify it under the terms of the GNU
 * Affero General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any
 * later version.
 * 
 * Xuggler is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public
 * License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with Xuggler.  If not, see <http://www.gnu.org/licenses/>.
 */

package com.xuggle.mediatool;

import com.xuggle.mediatool.event.IAddStreamEvent;
import com.xuggle.mediatool.event.IAudioSamplesEvent;
import com.xuggle.mediatool.event.ICloseCoderEvent;
import com.xuggle.mediatool.event.ICloseEvent;
import com.xuggle.mediatool.event.IFlushEvent;
import com.xuggle.mediatool.event.IOpenCoderEvent;
import com.xuggle.mediatool.event.IOpenEvent;
import com.xuggle.mediatool.event.IReadPacketEvent;
import com.xuggle.mediatool.event.IVideoPictureEvent;
import com.xuggle.mediatool.event.IWriteHeaderEvent;
import com.xuggle.mediatool.event.IWritePacketEvent;
import com.xuggle.mediatool.event.IWriteTrailerEvent;



/**
 * An implementation of {@link IMediaListener} that implements all methods as
 * empty methods.
 * 
 * <p>
 * 
 * This can be useful if you only want to override some members of
 * {@link IMediaListener}; instead, just subclass this and override the methods
 * you want, rather than providing an implementation of all event callbacks.
 * 
 * </p>
 */

public abstract class MediaListenerAdapter implements IMediaListener
{
  /** {@inheritDoc} */

  public void onVideoPicture(IVideoPictureEvent event)
  {
  }

  /** {@inheritDoc} */

  public void onAudioSamples(IAudioSamplesEvent event)
  {
  }

  /** {@inheritDoc} */

  public void onOpen(IOpenEvent event)
  {
  }

  /** {@inheritDoc} */

  public void onClose(ICloseEvent event)
  {
  }

  /** {@inheritDoc} */

  public void onAddStream(IAddStreamEvent event)
  {
  }

  /** {@inheritDoc} */

  public void onOpenCoder(IOpenCoderEvent event)
  {
  }

  /** {@inheritDoc} */

  public void onCloseCoder(ICloseCoderEvent event)
  {
  }

  /** {@inheritDoc} */

  public void onReadPacket(IReadPacketEvent event)
  {
  }

  /** {@inheritDoc} */

  public void onWritePacket(IWritePacketEvent event)
  {
  }

  /** {@inheritDoc} */

  public void onWriteHeader(IWriteHeaderEvent event)
  {
  }

  /** {@inheritDoc} */

  public void onFlush(IFlushEvent event)
  {
  }

  /** {@inheritDoc} */

  public void onWriteTrailer(IWriteTrailerEvent event)
  {
  }
}

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

import com.xuggle.mediatool.event.AddStreamEvent;
import com.xuggle.mediatool.event.AudioSamplesEvent;
import com.xuggle.mediatool.event.CloseCoderEvent;
import com.xuggle.mediatool.event.CloseEvent;
import com.xuggle.mediatool.event.FlushEvent;
import com.xuggle.mediatool.event.OpenCoderEvent;
import com.xuggle.mediatool.event.OpenEvent;
import com.xuggle.mediatool.event.ReadPacketEvent;
import com.xuggle.mediatool.event.VideoPictureEvent;
import com.xuggle.mediatool.event.WriteHeaderEvent;
import com.xuggle.mediatool.event.WritePacketEvent;
import com.xuggle.mediatool.event.WriteTrailerEvent;



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

  public void onVideoPicture(VideoPictureEvent event)
  {
  }

  /** {@inheritDoc} */

  public void onAudioSamples(AudioSamplesEvent event)
  {
  }

  /** {@inheritDoc} */

  public void onOpen(OpenEvent event)
  {
  }

  /** {@inheritDoc} */

  public void onClose(CloseEvent event)
  {
  }

  /** {@inheritDoc} */

  public void onAddStream(AddStreamEvent event)
  {
  }

  /** {@inheritDoc} */

  public void onOpenCoder(OpenCoderEvent event)
  {
  }

  /** {@inheritDoc} */

  public void onCloseCoder(CloseCoderEvent event)
  {
  }

  /** {@inheritDoc} */

  public void onReadPacket(ReadPacketEvent event)
  {
  }

  /** {@inheritDoc} */

  public void onWritePacket(WritePacketEvent event)
  {
  }

  /** {@inheritDoc} */

  public void onWriteHeader(WriteHeaderEvent event)
  {
  }

  /** {@inheritDoc} */

  public void onFlush(FlushEvent event)
  {
  }

  /** {@inheritDoc} */

  public void onWriteTrailer(WriteTrailerEvent event)
  {
  }
}

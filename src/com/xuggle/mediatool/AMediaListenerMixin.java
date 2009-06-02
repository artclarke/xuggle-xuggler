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
 * An abstract empty implementation of all
 * {@link IMediaListener} methods, but does not declare {@link IMediaListener}.
 * 
 * <p>
 * This class implements every method on {@link IMediaListener}, but
 * does nothing with the events.
 * </p>
 * 
 * <p>
 * 
 * This can be useful if you only want to override some members of
 * {@link IMediaListener}; instead, just subclass this and override the methods
 * you want, rather than providing an implementation of all event callbacks.
 * 
 * </p>
 * <p>
 * 
 * Mixin classes can be extended by anyone, but the extending class
 * gets to decide which, if any, of the interfaces they actually
 * want to support.
 * 
 * </p>
 * 
 */

public abstract class AMediaListenerMixin
{
  /**
   * Construct an empty {@link AMediaListenerMixin}.
   */
  public AMediaListenerMixin()
  {
    
  }
  
  /** Empty implementation of
   * {@link IMediaListener#onVideoPicture(IVideoPictureEvent)}.
   */

  public void onVideoPicture(IVideoPictureEvent event)
  {
  }

  /** Empty implementation of
   * {@link IMediaListener#onAudioSamples(IAudioSamplesEvent)}.
   */
  
  public void onAudioSamples(IAudioSamplesEvent event)
  {
  }

  /** Empty implementation of
   * {@link IMediaListener#onOpen(IOpenEvent)}.
   */

  public void onOpen(IOpenEvent event)
  {
  }

  /** Empty implementation of
   * {@link IMediaListener#onClose(ICloseEvent)}.
   */

  public void onClose(ICloseEvent event)
  {
  }

  /** Empty implementation of
   * {@link IMediaListener#onAddStream(IAddStreamEvent)}.
   */

  public void onAddStream(IAddStreamEvent event)
  {
  }

  /** Empty implementation of
   * {@link IMediaListener#onOpenCoder(IOpenCoderEvent)}.
   */

  public void onOpenCoder(IOpenCoderEvent event)
  {
  }

  /** Empty implementation of
   * {@link IMediaListener#onCloseCoder(ICloseCoderEvent)}.
   */

  public void onCloseCoder(ICloseCoderEvent event)
  {
  }

  /** Empty implementation of
   * {@link IMediaListener#onReadPacket(IReadPacketEvent)}.
   */

  public void onReadPacket(IReadPacketEvent event)
  {
  }

  /** Empty implementation of
   * {@link IMediaListener#onWritePacket(IWritePacketEvent)}.
   */

  public void onWritePacket(IWritePacketEvent event)
  {
  }

  /** Empty implementation of
   * {@link IMediaListener#onWriteHeader(IWriteHeaderEvent)}.
   */

  public void onWriteHeader(IWriteHeaderEvent event)
  {
  }

  /** Empty implementation of
   * {@link IMediaListener#onFlush(IFlushEvent)}.
   */

  public void onFlush(IFlushEvent event)
  {
  }

  /** Empty implementation of
   * {@link IMediaListener#onWriteTrailer(IWriteTrailerEvent)}.
   */

  public void onWriteTrailer(IWriteTrailerEvent event)
  {
  }
}

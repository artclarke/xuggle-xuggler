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
 * An abstract implementation of all
 * {@link IMediaTool} methods, but does not declare {@link IMediaTool}.
 *
 * <p>
 * 
 * Forwards every call on the {@link IMediaListener} interface methods to all 
 * listeners added on the {@link IMediaGenerator} interface, but
 * does not declare it implements those interfaces.
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
 * @author trebor
 * @author aclarke
 *
 */

public abstract class AMediaToolMixin extends AMediaGeneratorMixin
{
  /**
   * Create an AMediaToolMixin
   */
  public AMediaToolMixin()
  {
    super();
  }

  /**
   * Calls {@link IMediaListener#onAddStream(IAddStreamEvent)} on all
   * registered listeners.
   */
  public void onAddStream(IAddStreamEvent event)
  {
    for (IMediaListener listener : getListeners())
      listener.onAddStream(event);
  }

  /**
   * Calls {@link IMediaListener#onAudioSamples(IAudioSamplesEvent)}
   *  on all
   * registered listeners.
   */
  public void onAudioSamples(IAudioSamplesEvent event)
  {
    for (IMediaListener listener : getListeners())
      listener.onAudioSamples(event);
  }

  /**
   * Calls {@link IMediaListener#onClose(ICloseEvent)}
   *  on all
   * registered listeners.
   */
  public void onClose(ICloseEvent event)
  {
    for (IMediaListener listener : getListeners())
      listener.onClose(event);
  }

  /**
   * Calls {@link IMediaListener#onCloseCoder(ICloseCoderEvent)}
   *  on all
   * registered listeners.
   */
  public void onCloseCoder(ICloseCoderEvent event)
  {
    for (IMediaListener listener : getListeners())
      listener.onCloseCoder(event);
  }

  /**
   * Calls {@link IMediaListener#onFlush(IFlushEvent)}
   *  on all
   * registered listeners.
   */
  public void onFlush(IFlushEvent event)
  {
    for (IMediaListener listener : getListeners())
      listener.onFlush(event);
  }

  /**
   * Calls {@link IMediaListener#onOpen(IOpenEvent)}
   *  on all
   * registered listeners.
   */
  public void onOpen(IOpenEvent event)
  {
    for (IMediaListener listener : getListeners())
      listener.onOpen(event);
  }

  /**
   * Calls {@link IMediaListener#onOpenCoder(IOpenCoderEvent)}
   *  on all
   * registered listeners.
   */
  public void onOpenCoder(IOpenCoderEvent event)
  {
    for (IMediaListener listener : getListeners())
      listener.onOpenCoder(event);
  }

  /**
   * Calls {@link IMediaListener#onReadPacket(IReadPacketEvent)}
   *  on all
   * registered listeners.
   */
  public void onReadPacket(IReadPacketEvent event)
  {
    for (IMediaListener listener : getListeners())
      listener.onReadPacket(event);
  }

  /**
   * Calls {@link IMediaListener#onVideoPicture(IVideoPictureEvent)}
   *  on all
   * registered listeners.
   */
  public void onVideoPicture(IVideoPictureEvent event)
  {
    for (IMediaListener listener : getListeners())
      listener.onVideoPicture(event);
  }

  /**
   * Calls {@link IMediaListener#onWriteHeader(IWriteHeaderEvent)}
   *  on all
   * registered listeners.
   */
  public void onWriteHeader(IWriteHeaderEvent event)
  {
    for (IMediaListener listener : getListeners())
      listener.onWriteHeader(event);
  }

  /**
   * Calls {@link IMediaListener#onWritePacket(IWritePacketEvent)}
   *  on all
   * registered listeners.
   */
  public void onWritePacket(IWritePacketEvent event)
  {
    for (IMediaListener listener : getListeners())
      listener.onWritePacket(event);
  }

  /**
   * Calls {@link IMediaListener#onWriteTrailer(IWriteTrailerEvent)}
   *  on all
   * registered listeners.
   */
  public void onWriteTrailer(IWriteTrailerEvent event)
  {
    for (IMediaListener listener : getListeners())
      listener.onWriteTrailer(event);
  }

}

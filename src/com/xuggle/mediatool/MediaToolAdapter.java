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

import java.util.Collection;

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

public class MediaToolAdapter  extends AMediaToolMixin
implements IMediaTool
{
  /**
   * Implementation note: We declare and forward
   * to our parent every method in IMediaTool so that
   * it shows up obviously in JavaDoc that these are the
   * main methods people might override.
   */

  /**
   * {@inheritDoc}
   */
  public boolean addListener(IMediaListener listener)
  {
    return super.addListener(listener);
  }

  /**
   * {@inheritDoc}
   */
  public Collection<IMediaListener> getListeners()
  {
    return super.getListeners();
  }

  /**
   * {@inheritDoc}
   */
  public boolean removeListener(IMediaListener listener)
  {
    return super.removeListener(listener);
  }

  /**
   * {@inheritDoc}
   */
  public void onAddStream(IAddStreamEvent event)
  {
    super.onAddStream(event);
  }

  /**
   * {@inheritDoc}
   */
  public void onAudioSamples(IAudioSamplesEvent event)
  {
    super.onAudioSamples(event);
  }

  /**
   * {@inheritDoc}
   */
  public void onClose(ICloseEvent event)
  {
    super.onClose(event);
  }

  /**
   * {@inheritDoc}
   */
  public void onCloseCoder(ICloseCoderEvent event)
  {
    super.onCloseCoder(event);
  }

  /**
   * {@inheritDoc}
   */
  public void onFlush(IFlushEvent event)
  {
    super.onFlush(event);
  }

  /**
   * {@inheritDoc}
   */
  public void onOpen(IOpenEvent event)
  {
    super.onOpen(event);
  }

  /**
   * {@inheritDoc}
   */
  public void onOpenCoder(IOpenCoderEvent event)
  {
    super.onOpenCoder(event);
  }

  /**
   * {@inheritDoc}
   */
  public void onReadPacket(IReadPacketEvent event)
  {
   super.onReadPacket(event); 
  }

  /**
   * {@inheritDoc}
   */
  public void onVideoPicture(IVideoPictureEvent event)
  {
    super.onVideoPicture(event);
  }

  /**
   * {@inheritDoc}
   */
  public void onWriteHeader(IWriteHeaderEvent event)
  {
    super.onWriteHeader(event);
  }

  /**
   * {@inheritDoc}
   */
  public void onWritePacket(IWritePacketEvent event)
  {
   super.onWritePacket(event); 
  }

  /**
   * {@inheritDoc}
   */
  public void onWriteTrailer(IWriteTrailerEvent event)
  {
    super.onWriteTrailer(event);
  }

}

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
import java.util.Collections;
import java.util.concurrent.CopyOnWriteArrayList;

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
 * An abstract implementation
 * of {@link IMediaGenerator} and of
 * {@link IMediaListener} that just forwards each event
 * to all the registered listeners.
 * @author aclarke
 *
 */
public class AMediaToolMixin
{
  private final Collection<IMediaListener> mListeners = new CopyOnWriteArrayList<IMediaListener>();

  /**
   * Create an AMediaToolMixin
   */
  public AMediaToolMixin()
  {
    super();
  }

  /**
   * Default implementation for {@link IMediaGenerator#addListener(IMediaListener)}
   */
  public boolean addListener(IMediaListener listener)
  {
    return mListeners.add(listener);
  }

  /**
   * Default implementation for {@link IMediaGenerator#removeListener(IMediaListener)}
   */
  public boolean removeListener(IMediaListener listener)
  {
    return mListeners.remove(listener);
  }

  /**
   * Default implementation for {@link IMediaGenerator#getListeners()}
   */
  public Collection<IMediaListener> getListeners()
  {
    return Collections.unmodifiableCollection(mListeners);
  }

  /**
   * Default implementation for
   * {@link IMediaListener#onAddStream(IAddStreamEvent)}
   * @param event TODO
   */
  public void onAddStream(IAddStreamEvent event)
  {
    for (IMediaListener listener : getListeners())
      listener.onAddStream(event);
  }

  /**
   * Default implementation for
   * {@link IMediaListener#onAudioSamples(IAudioSamplesEvent)}
   * @param event TODO
   */
  public void onAudioSamples(IAudioSamplesEvent event)
  {
    for (IMediaListener listener : getListeners())
      listener.onAudioSamples(event);
  }

  /**
   * Default implementation for
   * {@link IMediaListener#onClose(ICloseEvent)}
   * @param event TODO
   */
  public void onClose(ICloseEvent event)
  {
    for (IMediaListener listener : getListeners())
      listener.onClose(event);
  }

  /**
   * Default implementation for
   * {@link IMediaListener#onCloseCoder(ICloseCoderEvent)}
   * @param event TODO
   */
  public void onCloseCoder(ICloseCoderEvent event)
  {
    for (IMediaListener listener : getListeners())
      listener.onCloseCoder(event);
  }

  /**
   * Default implementation for
   * {@link IMediaListener#onFlush(IFlushEvent)}
   * @param event TODO
   */
  public void onFlush(IFlushEvent event)
  {
    for (IMediaListener listener : getListeners())
      listener.onFlush(event);
  }

  /**
   * Default implementation for
   * {@link IMediaListener#onOpen(IOpenEvent)}
   * @param event TODO
   */
  public void onOpen(IOpenEvent event)
  {
    for (IMediaListener listener : getListeners())
      listener.onOpen(event);
  }

  /**
   * Default implementation for
   * {@link IMediaListener#onOpenCoder(IOpenCoderEvent)}
   * @param event TODO
   */
  public void onOpenCoder(IOpenCoderEvent event)
  {
    for (IMediaListener listener : getListeners())
      listener.onOpenCoder(event);
  }

  /**
   * Default implementation for
   * {@link IMediaListener#onReadPacket(IReadPacketEvent)}
   * @param event TODO
   */
  public void onReadPacket(IReadPacketEvent event)
  {
    for (IMediaListener listener : getListeners())
      listener.onReadPacket(event);
  }

  /**
   * Default implementation for
   * {@link IMediaListener#onVideoPicture(IVideoPictureEvent)}
   */
  public void onVideoPicture(IVideoPictureEvent event)
  {
    for (IMediaListener listener : getListeners())
      listener.onVideoPicture(event);
  }

  /**
   * Default implementation for
   * {@link IMediaListener#onWriteHeader(IWriteHeaderEvent)}
   * @param event TODO
   */
  public void onWriteHeader(IWriteHeaderEvent event)
  {
    for (IMediaListener listener : getListeners())
      listener.onWriteHeader(event);
  }

  /**
   * Default implementation for
   * {@link IMediaListener#onWritePacket(IWritePacketEvent)}
   * @param event TODO
   */
  public void onWritePacket(IWritePacketEvent event)
  {
    for (IMediaListener listener : getListeners())
      listener.onWritePacket(event);
  }

  /**
   * Default implementation for
   * {@link IMediaListener#onWriteTrailer(IWriteTrailerEvent)}
   * @param event TODO
   */
  public void onWriteTrailer(IWriteTrailerEvent event)
  {
    for (IMediaListener listener : getListeners())
      listener.onWriteTrailer(event);
  }

}

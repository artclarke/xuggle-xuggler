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
   * {@link IMediaListener#onAddStream(AddStreamEvent)}
   * @param event TODO
   */
  public void onAddStream(AddStreamEvent event)
  {
    for (IMediaListener listener : getListeners())
      listener.onAddStream(event);
  }

  /**
   * Default implementation for
   * {@link IMediaListener#onAudioSamples(AudioSamplesEvent)}
   * @param event TODO
   */
  public void onAudioSamples(AudioSamplesEvent event)
  {
    for (IMediaListener listener : getListeners())
      listener.onAudioSamples(event);
  }

  /**
   * Default implementation for
   * {@link IMediaListener#onClose(CloseEvent)}
   * @param event TODO
   */
  public void onClose(CloseEvent event)
  {
    for (IMediaListener listener : getListeners())
      listener.onClose(event);
  }

  /**
   * Default implementation for
   * {@link IMediaListener#onCloseCoder(CloseCoderEvent)}
   * @param event TODO
   */
  public void onCloseCoder(CloseCoderEvent event)
  {
    for (IMediaListener listener : getListeners())
      listener.onCloseCoder(event);
  }

  /**
   * Default implementation for
   * {@link IMediaListener#onFlush(FlushEvent)}
   * @param event TODO
   */
  public void onFlush(FlushEvent event)
  {
    for (IMediaListener listener : getListeners())
      listener.onFlush(event);
  }

  /**
   * Default implementation for
   * {@link IMediaListener#onOpen(OpenEvent)}
   * @param event TODO
   */
  public void onOpen(OpenEvent event)
  {
    for (IMediaListener listener : getListeners())
      listener.onOpen(event);
  }

  /**
   * Default implementation for
   * {@link IMediaListener#onOpenCoder(OpenCoderEvent)}
   * @param event TODO
   */
  public void onOpenCoder(OpenCoderEvent event)
  {
    for (IMediaListener listener : getListeners())
      listener.onOpenCoder(event);
  }

  /**
   * Default implementation for
   * {@link IMediaListener#onReadPacket(ReadPacketEvent)}
   * @param event TODO
   */
  public void onReadPacket(ReadPacketEvent event)
  {
    for (IMediaListener listener : getListeners())
      listener.onReadPacket(event);
  }

  /**
   * Default implementation for
   * {@link IMediaListener#onVideoPicture(VideoPictureEvent)}
   */
  public void onVideoPicture(VideoPictureEvent event)
  {
    for (IMediaListener listener : getListeners())
      listener.onVideoPicture(event);
  }

  /**
   * Default implementation for
   * {@link IMediaListener#onWriteHeader(WriteHeaderEvent)}
   * @param event TODO
   */
  public void onWriteHeader(WriteHeaderEvent event)
  {
    for (IMediaListener listener : getListeners())
      listener.onWriteHeader(event);
  }

  /**
   * Default implementation for
   * {@link IMediaListener#onWritePacket(WritePacketEvent)}
   * @param event TODO
   */
  public void onWritePacket(WritePacketEvent event)
  {
    for (IMediaListener listener : getListeners())
      listener.onWritePacket(event);
  }

  /**
   * Default implementation for
   * {@link IMediaListener#onWriteTrailer(WriteTrailerEvent)}
   * @param event TODO
   */
  public void onWriteTrailer(WriteTrailerEvent event)
  {
    for (IMediaListener listener : getListeners())
      listener.onWriteTrailer(event);
  }

}

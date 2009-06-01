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

import com.xuggle.mediatool.IMediaListener.MediaVideoPictureEvent;
import com.xuggle.xuggler.IAudioSamples;
import com.xuggle.xuggler.IPacket;

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
   * {@link IMediaListener#onAddStream(IMediaGenerator, int)}
   */
  public void onAddStream(IMediaGenerator tool, int streamIndex)
  {
    for (IMediaListener listener : getListeners())
      listener.onAddStream(tool, streamIndex);
  }

  /**
   * Default implementation for
   * {@link IMediaListener#onAudioSamples(IMediaGenerator, IAudioSamples, int)}
   */
  public void onAudioSamples(IMediaGenerator tool, IAudioSamples samples, int streamIndex)
  {
    for (IMediaListener listener : getListeners())
      listener.onAudioSamples(tool, samples, streamIndex);
  }

  /**
   * Default implementation for
   * {@link IMediaListener#onClose(IMediaGenerator)}
   */
  public void onClose(IMediaGenerator tool)
  {
    for (IMediaListener listener : getListeners())
      listener.onClose(tool);
  }

  /**
   * Default implementation for
   * {@link IMediaListener#onCloseCoder(IMediaGenerator, Integer)}
   */
  public void onCloseCoder(IMediaGenerator tool, Integer coderIndex)
  {
    for (IMediaListener listener : getListeners())
      listener.onCloseCoder(tool, coderIndex);
  }

  /**
   * Default implementation for
   * {@link IMediaListener#onFlush(IMediaGenerator)}
   */
  public void onFlush(IMediaGenerator tool)
  {
    for (IMediaListener listener : getListeners())
      listener.onFlush(tool);
  }

  /**
   * Default implementation for
   * {@link IMediaListener#onOpen(IMediaGenerator)}
   */
  public void onOpen(IMediaGenerator tool)
  {
    for (IMediaListener listener : getListeners())
      listener.onOpen(tool);
  }

  /**
   * Default implementation for
   * {@link IMediaListener#onOpenCoder(IMediaGenerator, Integer)}
   */
  public void onOpenCoder(IMediaGenerator tool, Integer coderIndex)
  {
    for (IMediaListener listener : getListeners())
      listener.onOpenCoder(tool, coderIndex);
  }

  /**
   * Default implementation for
   * {@link IMediaListener#onReadPacket(IMediaGenerator, IPacket)}
   */
  public void onReadPacket(IMediaGenerator tool, IPacket packet)
  {
    for (IMediaListener listener : getListeners())
      listener.onReadPacket(tool, packet);
  }

  /**
   * Default implementation for
   * {@link IMediaListener#onVideoPicture(MediaVideoPictureEvent)}
   */
  public void onVideoPicture(MediaVideoPictureEvent event)
  {
    for (IMediaListener listener : getListeners())
      listener.onVideoPicture(event);
  }

  /**
   * Default implementation for
   * {@link IMediaListener#onWriteHeader(IMediaGenerator)}
   */
  public void onWriteHeader(IMediaGenerator tool)
  {
    for (IMediaListener listener : getListeners())
      listener.onWriteHeader(tool);
  }

  /**
   * Default implementation for
   * {@link IMediaListener#onWritePacket(IMediaGenerator, IPacket)}
   */
  public void onWritePacket(IMediaGenerator tool, IPacket packet)
  {
    for (IMediaListener listener : getListeners())
      listener.onWritePacket(tool, packet);
  }

  /**
   * Default implementation for
   * {@link IMediaListener#onWriteTrailer(IMediaGenerator)}
   */
  public void onWriteTrailer(IMediaGenerator tool)
  {
    for (IMediaListener listener : getListeners())
      listener.onWriteTrailer(tool);
  }

}

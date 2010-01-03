/*******************************************************************************
 * Copyright (c) 2008, 2010 Xuggle Inc.  All rights reserved.
 *  
 * This file is part of Xuggle-Xuggler-Main.
 *
 * Xuggle-Xuggler-Main is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Xuggle-Xuggler-Main is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Xuggle-Xuggler-Main.  If not, see <http://www.gnu.org/licenses/>.
 *******************************************************************************/

package com.xuggle.mediatool.event;

import java.util.concurrent.TimeUnit;

import com.xuggle.mediatool.IMediaGenerator;
import com.xuggle.xuggler.IAudioSamples;

/**
 * An implementation of {@link IAudioSamplesEvent}.
 * 
 * @author aclarke
 *
 */
public class AudioSamplesEvent extends ARawMediaMixin implements IAudioSamplesEvent
{
  /**
   * Create an {@link AudioSamplesEvent}.
   * @param source the source
   * @param samples the samples (must be non null).
   * @param streamIndex the stream index of the stream that generated
   *   these samples, or null if unknown.
   * @throws IllegalArgumentException if samples is null.
   */
  public AudioSamplesEvent (IMediaGenerator source,
      IAudioSamples samples,
      Integer streamIndex)
  {
    super(source, samples, null, samples.getTimeStamp(), TimeUnit.MICROSECONDS, streamIndex);
  }
  
  /**
   * {@inheritDoc}
   * 
   *  @see com.xuggle.mediatool.event.IAudioSamplesEvent#getMediaData()
   */
  @Override
  public IAudioSamples getMediaData()
  {
    return (IAudioSamples) super.getMediaData();
  }
  
  /**
   * {@inheritDoc}
   * @see com.xuggle.mediatool.event.IAudioSamplesEvent#getAudioSamples()
   */
  public IAudioSamples getAudioSamples()
  {
    return getMediaData();
  }
  
}
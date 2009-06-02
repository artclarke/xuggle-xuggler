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

package com.xuggle.mediatool.event;

import com.xuggle.mediatool.IMediaGenerator;
import com.xuggle.mediatool.IMediaListener;
import com.xuggle.xuggler.IAudioSamples;

/**
 * {@link AEventMixin} for {@link IMediaListener#onAudioSamples(IAudioSamplesEvent)}
 * 
 * @author aclarke
 *
 */
public class AudioSamplesEvent extends ARawMediaMixin implements IAudioSamplesEvent
{
  public AudioSamplesEvent (IMediaGenerator source,
      IAudioSamples samples,
      int streamIndex)
  {
    super(source, samples, null, 0, null, streamIndex);
  }
  
  /* (non-Javadoc)
   * @see com.xuggle.mediatool.event.IAudioSamplesEvent#getMediaData()
   */
  @Override
  public IAudioSamples getMediaData()
  {
    return (IAudioSamples) super.getMediaData();
  }
  
  /* (non-Javadoc)
   * @see com.xuggle.mediatool.event.IAudioSamplesEvent#getAudioSamples()
   */
  public IAudioSamples getAudioSamples()
  {
    return getMediaData();
  }
  
}
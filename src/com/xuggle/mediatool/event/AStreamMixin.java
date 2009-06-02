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

/**
 * An abstract implementation of {@link IStreamEvent}, but does not declare
 * {@link IStreamEvent}.
 * @author aclarke
 *
 */
public abstract class AStreamMixin extends AEventMixin
{
  private final Integer mStreamIndex;

  /**
   * Create an {@link AStreamMixin}.
   * @param source the source.
   * @param streamIndex the stream index, or null if unknown.
   */
  public AStreamMixin(IMediaGenerator source, Integer streamIndex)
  {
    super(source);
    mStreamIndex = streamIndex;
  }

  /**
   * Implementation of {@link IStreamEvent#getStreamIndex()}.
   * @see com.xuggle.mediatool.event.IStreamEvent#getStreamIndex()
   */
  public Integer getStreamIndex()
  {
    return mStreamIndex;
  }
}
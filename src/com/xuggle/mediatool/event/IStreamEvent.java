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

/**
 * An {@link IEvent} that corresponds to a stream
 * in some container.
 * 
 * @author aclarke
 *
 */
public interface IStreamEvent extends IEvent
{

  /**
   * Get the stream index.
   * 
   * The stream index can be null if the 
   * {@link com.xuggle.mediatool.IMediaGenerator} that
   * generated this event has no concept of streams.
   * 
   * @return the stream index if known, or null if not
   */
  public abstract Integer getStreamIndex();

}

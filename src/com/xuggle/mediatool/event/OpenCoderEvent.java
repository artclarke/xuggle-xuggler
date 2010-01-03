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

import com.xuggle.mediatool.IMediaCoder;

/**
 * An implementation of {@link IOpenCoderEvent}.
 * @author aclarke
 *
 */

public class OpenCoderEvent extends AStreamCoderMixin implements IOpenCoderEvent
{
  /**
   * Create an {@link OpenCoderEvent}.
   * @param source the source of this event.
   * @param streamIndex the stream this event occurred on, or null if unknown.
   */
  public OpenCoderEvent(IMediaCoder source, Integer streamIndex)
  {
    super(source, streamIndex);
  }
}
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


/**
 * An abstract class that provides an implementation of
 * {@link IMediaPipe}.
 * 
 * <p>
 * This class does not actually declare it implements those
 * methods so that deriving classes can decide which if any
 * they want to expose.
 * </p>
 * 
 * @author aclarke
 *
 */
public abstract class AMediaPipe
// We deliberately don't declare these, but they
// are commented out so you can easily add them to check compliance
//implements IMediaPipeListener, IMediaPipe
{

  private final Collection<IMediaPipeListener> mListeners = new CopyOnWriteArrayList<IMediaPipeListener>();

  /**
   * Create an AMediaPipe
   */
  public AMediaPipe()
  {
    super();
  }

  /**
   * Default implementation for {@link IMediaPipe#addListener(IMediaPipeListener)}
   */
  public boolean addListener(IMediaPipeListener listener)
  {
    return mListeners.add(listener);
  }

  /**
   * Default implementation for {@link IMediaPipe#removeListener(IMediaPipeListener)}
   */
  public boolean removeListener(IMediaPipeListener listener)
  {
    return mListeners.remove(listener);
  }

  /**
   * Default implementation for {@link IMediaPipe#getListeners()}
   */
  public Collection<IMediaPipeListener> getListeners()
  {
    return Collections.unmodifiableCollection(mListeners);
  }

}

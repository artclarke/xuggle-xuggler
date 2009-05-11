/*
 * Copyright (c) 2008-2009 by Xuggle Inc. All rights reserved.
 *
 * It is REQUESTED BUT NOT REQUIRED if you use this library, that you let 
 * us know by sending e-mail to info@xuggle.com telling us briefly how you're
 * using the library and what you like or don't like about it.
 *
 * This library is free software; you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any later
 * version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along
 * with this library; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

package com.xuggle.xuggler.mediatool;

import java.util.Collection;
import java.util.Collections;
import java.util.concurrent.CopyOnWriteArrayList;

public abstract class AMediaTool implements IMediaTool
{
  // all the media reader listeners

  private final Collection<IMediaListener> mListeners
    = new CopyOnWriteArrayList<IMediaListener>();

  /** {@inheritDoc} */

  public boolean addListener(IMediaListener listener)
  {
    return mListeners.add(listener);
  }

  /** {@inheritDoc} */

  public boolean removeListener(IMediaListener listener)
  {
    return mListeners.remove(listener);
  }

  /** {@inheritDoc} */

  protected Collection<IMediaListener> getListeners()
  {
    return Collections.unmodifiableCollection(mListeners);
  }
}

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

import com.xuggle.xuggler.IContainer;

abstract class AMediaTool implements IMediaTool
{
  // the container to read from or write to
  
  protected final IContainer mContainer;

  // true if this media writer should close the container

  protected boolean mCloseContainer;

  // the URL which is read or written

  private final String mUrl;

  // all the media reader listeners

  private final Collection<IMediaListener> mListeners
    = new CopyOnWriteArrayList<IMediaListener>();

  /**
   * Construct an abstract IMediaTool.
   *
   * @param url the URL which will be read or written to
   * @param the container which be read from or written to
   */
  
  public AMediaTool(String url, IContainer container)
  {
    mUrl = url;
    mContainer = container;

    // it is assuemd that the container should not be closed by the
    // tool, this may change if open() is laster called 

    mCloseContainer = false;
  }

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

  public Collection<IMediaListener> getListeners()
  {
    return Collections.unmodifiableCollection(mListeners);
  } 

  /** {@inheritDoc} */

  public String getUrl()
  {
    return mUrl;
  }

  /** {@inheritDoc} */

  public IContainer getContainer()
  {
    return mContainer;
  }

  /** {@inheritDoc} */
  
  public boolean isOpen()
  {
    return mContainer.isOpened();
  }
}

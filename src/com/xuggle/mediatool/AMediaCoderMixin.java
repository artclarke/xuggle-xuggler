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

package com.xuggle.mediatool;



import com.xuggle.xuggler.IContainer;

/**
 * An abstract implementation of all
 * {@link IMediaCoder} methods, but does not declare {@link IMediaCoder}.
 * 
 * <p>
 * 
 * Mixin classes can be extended by anyone, but the extending class
 * gets to decide which, if any, of the interfaces they actually
 * want to support.
 * 
 * </p>
 * 
 * @author trebor
 * @author aclarke
 */

public abstract class AMediaCoderMixin extends AMediaToolMixin
{
  // the container to read from or write to
  
  private final IContainer mContainer;

  // true if this media writer should close the container

  private boolean mCloseContainer;

  // the URL which is read or written

  private final String mUrl;

  // all the media reader listeners

  /**
   * Construct an {@link AMediaCoderMixin}.
   *
   * @param url the URL which will be read or written to
   * @param container the container which be read from or written to
   */
  
  public AMediaCoderMixin(String url, IContainer container)
  {
    mUrl = url;
    mContainer = container.copyReference();

    // it is assuemd that the container should not be closed by the
    // tool, this may change if open() is laster called 

    setShouldCloseContainer(false);
  }

  /**
   * The URL from which the {@link IContainer} is being read or written to.
   * 
   * @return the source or destination URL.
   */

  public String getUrl()
  {
    return mUrl;
  }

  /** 
   * Get the underlying media {@link IContainer} that the {@link IMediaCoder} is
   * reading from or writing to.  The returned {@link IContainer} can
   * be further interrogated for media stream details.
   *
   * @return the media container.
   */

  public IContainer getContainer()
  {
    return mContainer == null ? null : mContainer.copyReference();
  }

  /**
   * Test if this {@link IMediaCoder} is open.
   * 
   * @return true if the media tool is open.
   */

  public boolean isOpen()
  {
    return mContainer.isOpened();
  }

  /**
   * Should this {@link IMediaCoder} call {@link IContainer#close()}
   * when {@link IMediaCoder#close()} is called.
   * @param value should we close the container
   */
  public void setShouldCloseContainer(boolean value)
  {
    mCloseContainer = value;
  }

  /**
   * Should this {@link IMediaCoder} call {@link IContainer#close()}
   * when {@link IMediaCoder#close()} is called.
   * 
   * @return should we close the container
   */
  public boolean getShouldCloseContainer()
  {
    return mCloseContainer;
  }
}

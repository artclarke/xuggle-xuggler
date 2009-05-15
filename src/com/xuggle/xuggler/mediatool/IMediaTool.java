/*
 * Copyright (c) 2008-2009 by Xuggle Inc. All rights reserved.
 *
 * It is REQUESTED BUT NOT REQUIRED if you use this library, that you
 * let us know by sending e-mail to info@xuggle.com telling us briefly
 * how you're using the library and what you like or don't like about
 * it.
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

package com.xuggle.xuggler.mediatool;

import java.util.Collection;

import com.xuggle.xuggler.IContainer;
import com.xuggle.xuggler.IStreamCoder;


/** 
 * In interface for media tools like {@link MediaReader} and {@link
 * MediaWriter}.
 */

public interface IMediaTool
{
  /**
   * Add a media tool listener.
   *
   * @param listener the listener to add
   *
   * @return true if this collection changed as a result of the call
   */
  
  public abstract boolean addListener(IMediaListener listener);
  
  /**
   * Remove a media tool listener.
   *
   * @param listener the listener to remove
   *
   * @return true if this collection changed as a result of the call
   */
  
  public abstract boolean removeListener(IMediaListener listener);
  
  /**
   * Get the list of existing media tools listeners.
   *
   * @return an unmodifiable collection of listeners
   */
  
  public abstract Collection<IMediaListener> getListeners();
  
  /** 
   * Get the underlying media {@link IContainer} that the media tool is
   * reading from or writing to.  The returned {@link IContainer} can
   * be further interrogated for media stream details.
   *
   * @return the media container.
   */

  public abstract IContainer getContainer();

  /**
   * The URL from which media is being read or written to.
   * 
   * @return the source or destination URL.
   */

  public abstract String getUrl();

  /** 
   * Open this media tool.  This will open the internal {@link
   * IContainer}.  Typically the tool will open itself at the right
   * time, but there may exist rare cases where the calling context
   * may need to open the tool.
   */

  public abstract void open();

  /**
   * Test if this media tool is open.
   * 
   * @return true if the media tool is open.
   */

  public abstract boolean isOpen();
    
  /** 
   * Close this media tool.  This will close all {@link IStreamCoder}s
   * explicitly opened by tool, then close the internal {@link
   * IContainer}, again only if it was explicitly opened by tool.
   * 
   * <p> Typically the tool will close itself at the right time, but there
   * are instances where the calling context may need to close the
   * tool. </p>
   */

  public abstract void close();
}

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

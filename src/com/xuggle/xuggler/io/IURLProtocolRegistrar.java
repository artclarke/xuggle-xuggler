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

package com.xuggle.xuggler.io;

import com.xuggle.xuggler.io.URLProtocolManager;

/**
 * For Internal Use Only.
 * 
 * A signature interface for objects that want to register protocols for
 * Xuggler to use.
 * 
 * @author aclarke
 *
 */
public interface IURLProtocolRegistrar
{
  /**
   * Tells Xuggler that any requests for a given protocol should be
   * redirected to the given manager.
   * <p>
   * NOTE: Protocol can only be alpha-characters; no numbers or punctuation.
   * </p>
   * @param protocol The protocol (alpha-only) without ":" 
   * @param manager The manager object you want to use.
   */
  void registerProtocol(String protocol, URLProtocolManager manager);
}

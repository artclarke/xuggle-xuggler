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

package com.xuggle.test_utils;

import com.xuggle.ferry.AtomicInteger;

/**
 * This is a simple class that calls one JNI_Utils method and could
 * (in theory) be used to test the memory management / ref-counting
 * JNI code.
 * 
 * In theory being that I have yet to get valgrind to start up a
 * JVM successfully, but for now, I'm checking this in and will come
 * back to it later.
 * 
 * @author aclarke
 *
 */
public class Standalone
{

  /**
   * @param args Command line arguments
   */
  public static void main(String[] args)
  {
    AtomicInteger ai = new AtomicInteger(5);
    System.out.println("The test is over... now: " + ai.get());
  }

}

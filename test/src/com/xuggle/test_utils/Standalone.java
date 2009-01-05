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

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

package com.xuggle.ferry;

import com.xuggle.ferry.NativeLogger;

import org.junit.*;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import junit.framework.TestCase;

public class NativeLoggerTest extends TestCase
{
  private final Logger log = LoggerFactory.getLogger(this.getClass());
  private NativeLogger mLog=null;

  @Before
  public void setUp()
  {
    log.debug("Executing test case: {}", this.getName());
    mLog = null;
  }
  
  @Test
  public void testLogger()
  {
    boolean retval = false;
    String loggerName = this.getClass().getName()+".native";
    mLog = NativeLogger.getLogger(loggerName);

    retval = mLog.log(0, "This is an error msg");
    assertTrue(retval);

    log.debug("Class logger name: {}", log.getName());
    log.debug("Native logger name: {}", mLog.getName());

    retval = mLog.log(1, "This is a warn msg");
    assertTrue(retval);
    retval = mLog.log(2, "This is an info msg");
    assertTrue(retval);
    retval = mLog.log(3, "This is a debug msg");
    assertTrue(retval);
    retval = mLog.log(4, "This is a trace msg");
    assertTrue(!retval);
    assertTrue("all tests seemed to pass without crashes", true);
  }

}

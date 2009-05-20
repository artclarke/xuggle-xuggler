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

package com.xuggle.xuggler;
import static org.junit.Assert.*;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;

public class ErrorTest
{

  @Before
  public void setUp() throws Exception
  {
  }

  @After
  public void tearDown() throws Exception
  {
  }

  @Test
  public void testGetType()
  {
    IError err = IError.make(IError.Type.ERROR_EOF);
    assertEquals(IError.Type.ERROR_EOF, err.getType());
  }

  @Test
  public void testGetDescription()
  {
    IError err = IError.make(IError.Type.ERROR_EOF);
    assertNotNull(err.getDescription());
    assertTrue(err.getDescription().length()>0);
  }

  @Test
  public void testGetErrorNumber()
  {
    IError err = IError.make(IError.Type.ERROR_EOF);
    assertTrue(err.getErrorNumber()<0);
  }

  @Test
  public void testMake1()
  {
    assertNull(IError.make(1)); // should fail with positive number
  }
  
  @Test
  public void testMake2()
  {
    assertNotNull(IError.make(IError.Type.ERROR_EOF));
  }

}

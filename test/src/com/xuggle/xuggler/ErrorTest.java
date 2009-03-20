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

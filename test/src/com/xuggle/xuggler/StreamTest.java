/*
 * This file is part of Xuggler.
 * 
 * Xuggler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 * 
 * Xuggler is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public
 * License along with Xuggler.  If not, see <http://www.gnu.org/licenses/>.
 */

package com.xuggle.xuggler;

import org.junit.*;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.xuggle.xuggler.IContainer;
import com.xuggle.xuggler.IRational;
import com.xuggle.xuggler.IStream;
import com.xuggle.xuggler.IStreamCoder;

import junit.framework.TestCase;

public class StreamTest extends TestCase
{
  private final Logger log = LoggerFactory.getLogger(this.getClass());
  private final String sampleFile = "fixtures/testfile.flv";

  private IStream mStream=null;
  private IContainer mContainer=null;

  @Before
  public void setUp()
  {
    log.debug("Executing test case: {}", this.getName());
    if (mContainer != null)
      mContainer.delete();
    mContainer = IContainer.make();
    if (mStream != null)
      mStream.delete();
    mStream = null;
  }
  
  @Test
  public void testGetStream()
  {
    helperGetStream(1);
  }
  
  @Test
  public void testStreamGetters()
  {
    helperGetStream(0);
    IRational rational = null;
    long val=0;
    
    val = mStream.getIndex();
    log.debug("Index: {}", val);
    
    val = mStream.getId();
    log.debug("ID: {}", val);
    
    rational = mStream.getFrameRate();
    log.debug("Frame Rate: {}", rational.getDouble());
    
    rational = mStream.getTimeBase();
    log.debug("Time Base: {}", rational.getDouble());
    
    val = mStream.getStartTime();
    log.debug("Start Time: {}", val);
    val = mStream.getDuration();
    log.debug("Duration: {}", val);
    val = mStream.getCurrentDts();
    log.debug("Current Dts: {}", val);
    val = mStream.getNumFrames();
    log.debug("Number of Frames: {}", val);
    val = mStream.getNumIndexEntries();
    log.debug("Number of Index Entries: {}", val);
    
  }
  
  public void testGetStreamCoder()
  {
    helperGetStream(0);
    
    IStreamCoder coder = null;
    coder = mStream.getStreamCoder();
    assertTrue(coder != null);
  }

  public void testGetSampleAspectRatio()
  {
    helperGetStream(0);
    IRational sample = mStream.getSampleAspectRatio();
    assertNotNull(sample);
    assertEquals(0, sample.getNumerator());
    assertEquals(0, sample.getNumerator());
  }
  
  public void testSetSampleAspectRatio()
  {
    helperGetStream(0);
    
    IRational newVal = IRational.make(3,2);
    mStream.setSampleAspectRatio(newVal);
    
    IRational sample = mStream.getSampleAspectRatio();
    assertNotNull(sample);
    assertEquals(3, sample.getNumerator());
    assertEquals(2, sample.getDenominator());
  }

  public void testSetSampleAspectRatioNullIgnored()
  {
    helperGetStream(0);
    
    IRational newVal = IRational.make(3,2);
    mStream.setSampleAspectRatio(newVal);
    
    IRational sample = mStream.getSampleAspectRatio();
    assertNotNull(sample);
    assertEquals(3, sample.getNumerator());
    assertEquals(2, sample.getDenominator());

    // Now, set to null and make sure it doesn't actually work or crash the JVM
    newVal = null;
    mStream.setSampleAspectRatio(newVal);
    
    sample = mStream.getSampleAspectRatio();
    assertNotNull(sample);
    assertEquals(3, sample.getNumerator());
    assertEquals(2, sample.getDenominator());
  }
  
  public void testGetLanguage()
  {
    helperGetStream(0);
    String lang = mStream.getLanguage();
    // should be null if not set, which is the case for the sample file
    assertNull(lang);
  }
  
  public void testSetLanguage()
  {
    helperGetStream(0);
    String lang = mStream.getLanguage();
    // should be null if not set, which is the case for the sample file
    assertNull(lang);
    
    mStream.setLanguage("jpn");
    assertEquals("jpn", mStream.getLanguage());
  }
  
  public void testSetLanguageNull()
  {
    helperGetStream(0);
    mStream.setLanguage("jpn");
    assertEquals("jpn", mStream.getLanguage());

    mStream.setLanguage(null);
    assertNull(mStream.getLanguage());
  }

  public void testSetLanguageEmptyString()
  {
    helperGetStream(0);
    mStream.setLanguage("jpn");
    assertEquals("jpn", mStream.getLanguage());

    mStream.setLanguage("");
    assertNull(mStream.getLanguage());
  }
  
  public void testSetLanguageFourCharacterString()
  {
    helperGetStream(0);
    mStream.setLanguage("1234");
    assertEquals("1234", mStream.getLanguage());
  }

  public void testSetLanguageFiveCharacterString()
  {
    helperGetStream(0);
    // should truncate to 4
    mStream.setLanguage("12345");
    assertEquals("1234", mStream.getLanguage());
  }


  private void helperGetStream(int index)
  {
    int retval = -1;
    retval = mContainer.open(sampleFile, IContainer.Type.READ, null);
    assertTrue(retval >= 0);
    
    assertTrue(mContainer.getNumStreams() == 2);
    
    mStream = mContainer.getStream(index);
    assertTrue(mStream != null);
    assertTrue(mStream.getIndex() == index);
  }
}

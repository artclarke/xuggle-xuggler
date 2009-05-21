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

import java.util.Set;

import org.junit.*;

import com.xuggle.xuggler.IContainerFormat;

import junit.framework.TestCase;

public class ContainerFormatTest extends TestCase
{
  private final String sampleFile = "fixtures/testfile.flv";

  @Test
  public void testSetOutputFmt()
  {
    IContainerFormat fmt = IContainerFormat.make();
    
    fmt.setOutputFormat("flv", null, null);
    fmt.setOutputFormat("flv", sampleFile, null);
    fmt.setOutputFormat("flv", "file:"+sampleFile, null);

    
    fmt.setOutputFormat("NotAShortName", null, null);
    fmt.setOutputFormat("NotAShortName", "NotAURL", null);
    fmt.setOutputFormat("NotAShortName", "file:"+"NotAURL", null);
    fmt.setOutputFormat("NotAShortName", "NotAProtocol:"+"NotAURL", null);

    
    assertTrue("got to end of test without coredump.  woo hoo", true);
  }
  
  @Test
  public void testSetInputFmt()
  {
    IContainerFormat fmt = IContainerFormat.make();
    
    fmt.setInputFormat("flv");
    fmt.setInputFormat("mov");
    fmt.setInputFormat("NotAShortName");
    assertTrue("got to end of test without coredump.  woo hoo", true);
  }
  
  @Test
  public void testGetInputFlag()
  {
    IContainerFormat fmt = IContainerFormat.make();
    int retval = -1;
    int flags = fmt.getInputFlags();
    assertEquals("should be not set", flags, 0);
    retval = fmt.setInputFormat("s16be");
    assertTrue("should succeed", retval >= 0);
    boolean hasGenericIndex = fmt.getInputFlag(IContainerFormat.Flags.FLAG_GENERIC_INDEX);
    assertTrue("should have global header", hasGenericIndex);
  }
  
  @Test
  public void testGetOutputFlag()
  {
    IContainerFormat fmt = IContainerFormat.make();
    
    int retval = -1;
    int flags = fmt.getOutputFlags();
    assertEquals("should be not set", flags, 0);
    retval = fmt.setOutputFormat("mov", null, null);
    assertTrue("should succeed", retval >= 0);
    boolean hasGlobalHeader = fmt.getOutputFlag(IContainerFormat.Flags.FLAG_GLOBALHEADER);
    assertTrue("should have global header", hasGlobalHeader);
  }
  
  @Test
  public void testSetInputFlag()
  {
    IContainerFormat fmt = IContainerFormat.make();
    int retval = -1;
    int flags = fmt.getInputFlags();
    assertEquals("should be not set", flags, 0);
    retval = fmt.setInputFormat("s16be");
    assertTrue("should succeed", retval >= 0);
    boolean hasGlobalHeader = fmt.getInputFlag(IContainerFormat.Flags.FLAG_GLOBALHEADER);
    assertTrue("should not have global header", !hasGlobalHeader);
    fmt.setInputFlag(IContainerFormat.Flags.FLAG_GLOBALHEADER, true);
    hasGlobalHeader = fmt.getInputFlag(IContainerFormat.Flags.FLAG_GLOBALHEADER);
    assertTrue("should have global header", hasGlobalHeader);

    fmt.setInputFlag(IContainerFormat.Flags.FLAG_GLOBALHEADER, false);
    hasGlobalHeader = fmt.getInputFlag(IContainerFormat.Flags.FLAG_GLOBALHEADER);
    assertTrue("should not have global header", !hasGlobalHeader);
  }
  
  @Test
  public void testSetOutputFlag()
  {
    IContainerFormat fmt = IContainerFormat.make();
    
    int retval = -1;
    int flags = fmt.getOutputFlags();
    assertEquals("should be not set", flags, 0);
    retval = fmt.setOutputFormat("mov", null, null);
    assertTrue("should succeed", retval >= 0);
    boolean hasGlobalHeader = fmt.getOutputFlag(IContainerFormat.Flags.FLAG_GLOBALHEADER);
    assertTrue("should have global header", hasGlobalHeader);
    
    fmt.setOutputFlag(IContainerFormat.Flags.FLAG_GLOBALHEADER, false);
    hasGlobalHeader = fmt.getOutputFlag(IContainerFormat.Flags.FLAG_GLOBALHEADER);
    assertTrue("should not have global header", !hasGlobalHeader);

    fmt.setOutputFlag(IContainerFormat.Flags.FLAG_GLOBALHEADER, true);
    hasGlobalHeader = fmt.getOutputFlag(IContainerFormat.Flags.FLAG_GLOBALHEADER);
    assertTrue("should have global header", hasGlobalHeader);
    

  }
  
  @Test
  public void testGetOutputCodecsSupported()
  {
    IContainerFormat fmt = IContainerFormat.make();
    
    int retval = -1;
    int flags = fmt.getOutputFlags();
    assertEquals("should be not set", flags, 0);
    retval = fmt.setOutputFormat("mov", null, null);
    assertTrue("should succeed", retval >= 0);
    
    Set<ICodec.ID> codecs = fmt.getOutputCodecsSupported();
    assertNotNull(codecs);
//    for(ICodec.ID id : codecs)
//      System.out.println("Codec: "+id);

    assertTrue("should get at least one codec", codecs.size() > 1);
    assertTrue("should have MP3",
        codecs.contains(ICodec.ID.CODEC_ID_MP3));
    assertTrue("Should contain H263",
        codecs.contains(ICodec.ID.CODEC_ID_H263));
  }
}

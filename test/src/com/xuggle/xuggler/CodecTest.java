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

import java.util.Collection;

import org.junit.*;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.xuggle.xuggler.ICodec;

import junit.framework.TestCase;

public class CodecTest extends TestCase
{
  private final Logger log = LoggerFactory.getLogger(this.getClass());

  private ICodec mCodec=null;
  @Before
  public void setUp()
  {
    if (mCodec!= null)
    {
      mCodec.delete();
    }
    mCodec = null;
  }
  @Test
  public void testFindDecodingCodec()
  {
    String name = null;
    ICodec.ID id = ICodec.ID.CODEC_ID_NONE;
    ICodec.Type type = ICodec.Type.CODEC_TYPE_UNKNOWN;
    
    mCodec = ICodec.findDecodingCodec(
        ICodec.ID.CODEC_ID_NELLYMOSER);
    assertTrue("no codec?", mCodec != null);
    id = mCodec.getID();
    type = mCodec.getType();
    
    name = mCodec.getName();
    log.debug("Codec name is: {}", name);
    log.debug("Codec   id is: {}", id);
    log.debug("Codec type is: {}", type);
    
    mCodec.delete();
    mCodec = ICodec.findDecodingCodecByName(name);
    assertTrue("no codec?", mCodec != null);
    assertTrue("different codec?", id == mCodec.getID());
    assertTrue("different codec?", type == mCodec.getType());    

    mCodec = ICodec.findDecodingCodecByName("notbloodylikely");
    assertTrue("that codec exists?", mCodec == null);
    mCodec = ICodec.findDecodingCodec(ICodec.ID.CODEC_ID_NONE);
    assertTrue("that codec exists?", mCodec == null);
  }

  @Test
  public void testFindEncodingCodec()
  {
    String name = null;
    ICodec.ID id = ICodec.ID.CODEC_ID_NONE;
    ICodec.Type type = ICodec.Type.CODEC_TYPE_UNKNOWN;
    
    mCodec = ICodec.findEncodingCodec(
        ICodec.ID.CODEC_ID_MP3);
    assertTrue("no codec?", mCodec != null);
    id = mCodec.getID();
    type = mCodec.getType();
    
    name = mCodec.getName();
    log.debug("Codec name is: {}", name);
    log.debug("Codec   id is: {}", id);
    log.debug("Codec type is: {}", type);
    
    mCodec.delete();
    mCodec = ICodec.findEncodingCodecByName(name);
    assertTrue("no codec?", mCodec != null);
    assertTrue("different codec?", id == mCodec.getID());
    assertTrue("different codec?", type == mCodec.getType());
    
    mCodec = ICodec.findEncodingCodecByName("notbloodylikely");
    assertTrue("that codec exists?", mCodec == null);
    mCodec = ICodec.findEncodingCodec(ICodec.ID.CODEC_ID_NONE);
    assertTrue("that codec exists?", mCodec == null);
    
  }

  @Test
  public void testGetLongName()
  {
    mCodec = ICodec.findEncodingCodec(
        ICodec.ID.CODEC_ID_MP3);
    assertNotNull(mCodec);
    String longname = mCodec.getLongName();
    assertNotNull("no long name", longname);
    assertTrue("no description", longname.length() > 0);
  }
  @Test
  public void testCodecIdsAreEnums()
  {
    ICodec.ID ids[] = {
        ICodec.ID.CODEC_ID_4XM,
        ICodec.ID.CODEC_ID_ADPCM_4XM,
        ICodec.ID.CODEC_ID_DXA
    };
    
    for(ICodec.ID id : ids)
    {
      switch(id)
      {
      case CODEC_ID_4XM:
      case CODEC_ID_ADPCM_4XM:
      case CODEC_ID_DXA:
        // yup; looks like we can do a switch statement;
        break;
      default:
        fail("and should never get here");
      }
    }
  }
  
  @Test
  public void testGetInstalledCodecs()
  {
    Collection<ICodec> installed = ICodec.getInstalledCodecs();
    assertTrue(installed.size() > 0);
    for(ICodec codec : installed)
    {
      assertNotNull(codec);
      assertTrue(codec.getName().length() > 0);
    }
  }
}

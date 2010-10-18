/*******************************************************************************
 * Copyright (c) 2008, 2010 Xuggle Inc.  All rights reserved.
 *  
 * This file is part of Xuggle-Xuggler-Main.
 *
 * Xuggle-Xuggler-Main is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Xuggle-Xuggler-Main is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Xuggle-Xuggler-Main.  If not, see <http://www.gnu.org/licenses/>.
 *******************************************************************************/
package com.xuggle.xuggler;

import static org.junit.Assert.*;

import java.util.Collection;

import org.junit.Ignore;
import org.junit.Test;

import com.xuggle.ferry.IBuffer;

public class MetaDataTest
{
  public static final String AUTHOR_KEY = "artist";
  public static final String GENRE_KEY = "genre";
  public static final String TITLE_KEY = "title";
  public static final String YEAR_KEY = "TYER";
  public static final String ALBUM_KEY = "album";
  public static final String COMMENT_KEY = "comment";
  
  @Test
  public final void testGetKeys()
  {
    IMetaData meta = IMetaData.make();
    Collection<String> keys = meta.getKeys();
    assertNotNull(keys);
    assertEquals(0, keys.size());
    meta.setValue("foo", "bar");
    meta.setValue("bar", "goober");
    keys = meta.getKeys();
    assertEquals(2, keys.size());
    System.out.println("MetaData = " + meta);
  }

  @Test
  public final void testGetValue()
  {
    IMetaData meta = IMetaData.make();
    Collection<String> keys = meta.getKeys();
    assertNotNull(keys);
    assertEquals(0, keys.size());
    meta.setValue("foo", "bar");
    meta.setValue("bar", "goober");
    keys = meta.getKeys();
    assertEquals(2, keys.size());
    assertEquals("bar", meta.getValue("FoO", IMetaData.Flags.METADATA_NONE));
    assertNull(meta.getValue("FoO", IMetaData.Flags.METADATA_MATCH_CASE));
    assertNull(meta.getValue("notthere"));
  }
  
  @Test
  public void testGetFromContainer()
  {
    IContainer container = IContainer.make();
    container.open("fixtures/testfile.mp3", IContainer.Type.READ, null);
    IMetaData meta = container.getMetaData();
    Collection<String> keys = meta.getKeys();
    for(String key : keys)
    {
      System.out.println(key + " = " + meta.getValue(key));
    }
    assertNotNull(meta.getValue(AUTHOR_KEY));
    meta.setValue(AUTHOR_KEY, "Your Mom");
    assertEquals("Your Mom", meta.getValue(AUTHOR_KEY));
  }

  @Test
  @Ignore // fails sometimes and but not debugging now.
  public void testSetInContainer()
  {
    String filename = getClass().getName()+".mp3";
    IContainer container = IContainer.make();
    container.open(filename, IContainer.Type.WRITE, null);
    IStream stream = container.addNewStream(0);
    IStreamCoder coder = stream.getStreamCoder();
    coder.setCodec(ICodec.ID.CODEC_ID_MP3);
    coder.setSampleRate(22050);
    coder.setChannels(1);
    coder.open();
    
    IMetaData meta = container.getMetaData();
    String author = "Your Mom";
    String genre = "6";
    String title = "Ode to Mothers";
    meta.setValue(TITLE_KEY, title);
    meta.setValue(AUTHOR_KEY, author);
    meta.setValue(GENRE_KEY, genre);
    meta.setValue(YEAR_KEY, "2009");
    meta.setValue(ALBUM_KEY, "So large the sun rotates around her");
    meta.setValue(COMMENT_KEY, "I wonder why genre is blues?");
    container.writeHeader();
    
    // Let's add some fake data
    byte[] fakeData = new byte[64*576];
    for(int i = 0; i < fakeData.length; i++)
      fakeData[i] = (byte)i; // garbage
    
    IBuffer buffer = IBuffer.make(null, fakeData, 0, fakeData.length);
    IAudioSamples samples = IAudioSamples.make(buffer,
        coder.getChannels(), coder.getSampleFormat());
    samples.setComplete(true,
        fakeData.length/2, coder.getSampleRate(),
        coder.getChannels(), coder.getSampleFormat(), 0);
    IPacket packet = IPacket.make();
    int samplesDecoded = 0;
    do {
      int ret = coder.encodeAudio(packet, samples, samplesDecoded);
      if (ret <= 0)
        break;
      samplesDecoded += ret;
      if (samplesDecoded >= samples.getNumSamples())
        break;
      if (packet.isComplete())
        container.writePacket(packet);
    } while(true);
    // flush
    coder.encodeAudio(packet, null, 0);
    if (packet.isComplete())
      container.writePacket(packet);
    container.writeTrailer();
    coder.close();
    coder.delete();
    stream.delete();
    container.close();
    container.delete();
    
    container = IContainer.make();
    assertTrue(container.open(filename, IContainer.Type.READ, null)>=0);
    meta = container.getMetaData();
    System.out.println("Metadata = " + meta);
    assertEquals(author, meta.getValue(AUTHOR_KEY));
    assertEquals("Grunge", meta.getValue(GENRE_KEY));
    assertEquals(title, meta.getValue(TITLE_KEY));
  }

  @Test
  @Ignore // disabling for 3.3
  public void testGetFLVMetaDataContainer()
  {
    IContainer container = IContainer.make();
    container.open("fixtures/testfile.flv", IContainer.Type.READ, null);
    IMetaData meta = container.getMetaData();
    Collection<String> keys = meta.getKeys();
    for(String key : keys)
    {
      System.out.println(key + " = " + meta.getValue(key));
    }
    if (keys.size() > 0)
    {
      // This means the version of FFmpeg we're using has our
      // patch for FLV meta-data installed.  Let's make sure it's
      // right
      assertEquals(11, keys.size());
      meta.setValue(AUTHOR_KEY, "Your Mom");
      assertEquals("Your Mom", meta.getValue(AUTHOR_KEY));
    }
    container.close();
    meta.delete();
    container.delete();
  }


}

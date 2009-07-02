package com.xuggle.xuggler;

import static org.junit.Assert.*;

import java.util.Collection;

import org.junit.Test;

import com.xuggle.ferry.IBuffer;

public class MetaDataTest
{

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
    assertNotNull(meta.getValue("author"));
    meta.setValue("author", "Your Mom");
    assertEquals("Your Mom", meta.getValue("author"));
  }

  @Test
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
    meta.setValue("title", title);
    meta.setValue("author", author);
    meta.setValue("genre", genre);
    meta.setValue("year", "2009");
    meta.setValue("album", "So large the sun rotates around her");
    meta.setValue("comment", "I wonder why genre is blues?");
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
    assertEquals(author, meta.getValue("author"));
    assertEquals("Grunge", meta.getValue("genre"));
    assertEquals(title, meta.getValue("title"));
  }

  @Test
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
      meta.setValue("author", "Your Mom");
      assertEquals("Your Mom", meta.getValue("author"));
    }
  }


}

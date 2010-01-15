package com.xuggle.xuggler;

import static org.junit.Assert.*;

import java.util.List;

import org.junit.Test;

public class IndexEntryTest
{

  @Test
  public void testReadIndex()
  {
    IContainer container = IContainer.make();
    int retval;
    retval = container.open("fixtures/testfile_h264_mp4a_tmcd.mov", IContainer.Type.READ, null);
    assertTrue(retval >= 0);
    assertEquals(3, container.getNumStreams());
    IStream stream = container.getStream(0);
    IStreamCoder coder = stream.getStreamCoder();
    assertEquals(ICodec.ID.CODEC_ID_H264, coder.getCodecID());
    assertEquals(2665, stream.getNumIndexEntries());
    List<IIndexEntry> entries = stream.getIndexEntries();
    assertEquals(stream.getNumIndexEntries(), entries.size());
//    for(IIndexEntry entry : entries)
//      System.out.println(entry);

    IIndexEntry lastEntry = entries.get(entries.size()-1);
    assertEquals(11673146, lastEntry.getPosition());
    assertEquals(332875, lastEntry.getTimeStamp());
    assertEquals("Should not be a key frame", 0, lastEntry.getFlags());
    assertEquals(50, lastEntry.getSize());
    assertEquals(96, lastEntry.getMinDistance());
  }
}

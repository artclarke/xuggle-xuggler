package com.xuggle.xuggler;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import java.util.ArrayList;

import org.junit.Ignore;
import org.junit.Test;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;


public class ContainerSeekExhaustiveTest
{
  private final Logger log = LoggerFactory.getLogger(this.getClass());

  @Test
  @Ignore
  public void testSeekKeyFrameCheckIndex()
  {
    Global.setFFmpegLoggingLevel(52);
    IContainer container = IContainer.make();
    
    int retval = -1;
    //final String INPUT_FILE="fixtures/"+this.getClass().getName()+"-testSeekKeyFrameCheckIndex.ogg";
//    final int NUM_STREAMS=2;
//    final ICodec.ID VID_CODEC=ICodec.ID.CODEC_ID_THEORA;
//    final String INPUT_FILE="fixtures/testfile_h264_mp4a_tmcd.mov";
//    final int NUM_STREAMS=3;
//    final ICodec.ID VID_CODEC=ICodec.ID.CODEC_ID_H264;
    final String INPUT_FILE="fixtures/testfile.flv";
    final int NUM_STREAMS=2;
    final ICodec.ID VID_CODEC=ICodec.ID.CODEC_ID_FLV1;
    
    retval = container.open(INPUT_FILE, IContainer.Type.READ, null);
    assertTrue("could not open file", retval >= 0);

    // First, let's get all the key frames
    final int numStreams = container.getNumStreams();
    assertEquals(NUM_STREAMS, numStreams);
    
    final int vidIndex = 0;
    final IStream vidStream = container.getStream(vidIndex);
    final IStreamCoder vidCoder = vidStream.getStreamCoder();
    assertEquals(VID_CODEC, vidCoder.getCodecID());
    vidCoder.delete();
    vidStream.delete();
    
    // now, loop through the entire file and record the index of EACH
    // video key frame
    final IPacket packet = IPacket.make();
    final ArrayList<Long> offsets = new ArrayList<Long>(1024);
    final ArrayList<Long> timestamps = new ArrayList<Long>(1024);
    long numPackets=0;
    while(container.readNextPacket(packet) >= 0)
    {
//      log.debug("Next: {}", container.getByteOffset());
      if (packet.isComplete())
      {
        if (packet.getStreamIndex() != vidIndex)
          continue;
        if (!packet.isKey())
          continue;
//        log.debug("Packet: {}", packet);
        if (numPackets < 5)
        {
          log.debug("First Packet: {}", packet);
        }
        ++numPackets;
        assertTrue(packet.getPosition() >= 0);
        offsets.add(packet.getPosition());
        timestamps.add(packet.getDts());
      }
    }
    log.debug("Num Index Entries; container: {}; test: {}",
        vidStream.getNumIndexEntries(),
        offsets.size());
//    retval = container.close();
//    assertTrue("got negative retval: " + IError.errorNumberToType(retval), retval >= 0);
//    retval = container.open(INPUT_FILE, IContainer.Type.READ, null);
//    assertTrue("got negative retval: " + IError.errorNumberToType(retval), retval >= 0);
    
    // move the seek head backwards
    retval = container.seekKeyFrame(-1,
        Long.MIN_VALUE, 0, Long.MAX_VALUE,
        IContainer.SEEK_FLAG_BACKWARDS);
    assertTrue("got negative retval: " + IError.errorNumberToType(retval), retval >= 0);
    
    // now let's walk through that index and ensure we can seek to each key frame.
    for(int i = 0; i < offsets.size(); i++)
    {
      long index = offsets.get(i);
      retval = container.seekKeyFrame(
          vidIndex,
          index,
          index,
          index,
//          0);
          IContainer.SEEK_FLAG_BYTE);
      assertTrue("got negative retval: " + IError.errorNumberToType(retval), retval >= 0);
      retval = container.readNextPacket(packet);
      log.debug("{}", packet);
      assertTrue("got negative retval: " + IError.errorNumberToType(retval), retval >= 0);
      assertTrue(packet.isComplete());
//      assertEquals(offsets.get(i).longValue(), packet.getPosition());
//      assertEquals(timestamps.get(i).longValue(), packet.getDts());
      assertTrue(packet.isKey());
    }
    container.close();
    
  }

}

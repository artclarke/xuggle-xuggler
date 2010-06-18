package com.xuggle.xuggler;

import java.io.IOException;
import java.net.Socket;
import java.net.UnknownHostException;

import static org.junit.Assert.*;

import org.apache.commons.cli.CommandLine;
import org.apache.commons.cli.Options;
import org.apache.commons.cli.ParseException;
import org.junit.Test;


/**
 * Tests publishing a stream to a RTMP server.
 * If localhost:5080 is not open, then we assume there is no
 * RTMP server on this machine and succeed silently.
 * @author aclarke
 *
 */
public class RTMPPublishingExhaustiveTest
{
  // This assumes a Wowza installation is installed with an application
  // called live.  To verify the test, use a RTMP player to watch the stream
  private static final int SERVER_PORT = 1935;
  private static final String SERVER_NAME = "127.0.0.1";
  private static final String APP_NAME = "live";
  private static final String STREAM_NAME = "test";
  private static final String PROTOCOL_NAME = "rtmp";

  @Test
  public void testRTMPPublishSorenson() throws ParseException
  {
    if (!rtmpPortOpen())
      return;
    final String url =
      PROTOCOL_NAME + "://" + SERVER_NAME + ":" + SERVER_PORT + "/" + APP_NAME + "/" + STREAM_NAME;
    
    String[] args = new String[]{
        "--containerformat",
        "flv",
        "--acodec",
        "libmp3lame",
        "--asamplerate",
        "22050",
        "--achannels",
        "1",
        "--abitrate",
        "64000",
        "--aquality",
        "0",
        "--vcodec",
        "flv",
        "--vscalefactor",
        "1.0",
        "--vbitrate",
        "300000",
        "--vbitratetolerance",
        "12000000",
        "--vquality",
        "0",
        "--realtime", // this will stream out to an RTMP server
        "fixtures/testfile_videoonly_20sec.flv",
        url
    };
    
    Converter converter = new Converter();
    
    Options options = converter.defineOptions();
  
    CommandLine cmdLine = converter.parseOptions(options, args);
    assertTrue("all commandline options successful", cmdLine != null);
    
    final long startTime = System.nanoTime();
    converter.run(cmdLine);
    final long endTime = System.nanoTime();
    final long delta = endTime - startTime;
    // 20 second test file;
    assertTrue("did not take long enough", delta >= 18L*1000*1000*1000);
    System.err.println("Total time taken: " + delta);
    assertTrue("took too long", delta <= 60L*1000*1000*1000);
    
  }
  @Test
  public void testRTMPPublishH264() throws ParseException
  {
    if (!rtmpPortOpen())
      return;
    final String url =
      PROTOCOL_NAME + "://" + SERVER_NAME + ":" + SERVER_PORT + "/" + APP_NAME + "/" + "mp4:"+ STREAM_NAME;
    
    String[] args = new String[]{
        "--containerformat",
        "flv",
        "--acodec",
        "libmp3lame",
        "--asamplerate",
        "22050",
        "--achannels",
        "1",
        "--abitrate",
        "64000",
        "--aquality",
        "0",
        "--vcodec",
        "libx264",
        "--vscalefactor",
        "1.0",
        "--vbitrate",
        "300000",
        "--vbitratetolerance",
        "12000000",
        "--vquality",
        "0",
        "--vpreset",
        "fixtures/"+this.getClass().getName()+"_h264.preset",
        "--realtime", // this will stream out to an RTMP server
        "fixtures/testfile_videoonly_20sec.flv",
        url
    };
    
    Converter converter = new Converter();
    
    Options options = converter.defineOptions();
  
    CommandLine cmdLine = converter.parseOptions(options, args);
    assertTrue("all commandline options successful", cmdLine != null);
    
    final long startTime = System.nanoTime();
    converter.run(cmdLine);
    final long endTime = System.nanoTime();
    final long delta = endTime - startTime;
    // 20 second test file;
    assertTrue("did not take long enough", delta >= 18L*1000*1000*1000);
    System.err.println("Total time taken: " + delta);
    assertTrue("took too long", delta <= 60L*1000*1000*1000);
    
  }

  private static boolean rtmpPortOpen()
  {
    try
    {
      Socket socket = new Socket(SERVER_NAME, SERVER_PORT);
      socket.close();
      return true;
    }
    catch (UnknownHostException e)
    {
      assertTrue("unknown host", false);
    }
    catch (IOException e)
    {
      // no port; just return false
      System.out.println("No RTMP Server on this machine");
    }
    
    return false;
  }

}

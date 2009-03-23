package com.xuggle.xuggler;


import com.xuggle.test_utils.NameAwareTestClassRunner;
import com.xuggle.xuggler.ICodec;
import com.xuggle.xuggler.IContainerFormat;
import com.xuggle.xuggler.IPixelFormat;
import com.xuggle.xuggler.IRational;
import com.xuggle.xuggler.ISimpleMediaFile;
import com.xuggle.xuggler.ITimeValue;
import com.xuggle.xuggler.SimpleMediaFile;

import static org.junit.Assert.*;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

@RunWith(NameAwareTestClassRunner.class)
public class SimpleMediaFileTest
{
  private final Logger log = LoggerFactory.getLogger(this.getClass());
  private String mTestName = null;
  @Before
  public void setUp()
  {
    mTestName = NameAwareTestClassRunner.getTestMethodName();
    log.debug("-----START----- {}", mTestName);
  }
  @After
  public void tearDown()
  {
    log.debug("----- END ----- {}", mTestName);
  }
  
  @Test
  public void testCreation()
  {
    new SimpleMediaFile();
  }
  
  @Test(expected=IllegalArgumentException.class)
  public void testCopyNull()
  {
    new SimpleMediaFile(null);
  }
  
  @Test
  public void testCopy()
  {
    ISimpleMediaFile obj = new SimpleMediaFile();
    // change all the defaults
    obj.setAudioBitRate(123);
    obj.setAudioChannels(2);
    obj.setAudioSampleRate(22050);
    obj.setAudioCodec(ICodec.ID.CODEC_ID_MP3);
    obj.setAudioTimeBase(IRational.make(17, 100));
    obj.setVideoWidth(1);
    obj.setVideoHeight(2);
    obj.setVideoTimeBase(IRational.make(1, 100));
    obj.setVideoCodec(ICodec.ID.CODEC_ID_FLV1);
    obj.setVideoPixelFormat(IPixelFormat.Type.ARGB);
    obj.setVideoNumPicturesInGroupOfPictures(12);
    obj.setVideoFrameRate(IRational.make(2, 15));
    obj.setVideoGlobalQuality(5);
    obj.setDuration(ITimeValue.make(1, ITimeValue.Unit.SECONDS));
    IContainerFormat format = IContainerFormat.make();
    format.setInputFormat("flv");
    obj.setContainerFormat(format);
    obj.setHasVideo(false);
    obj.setHasAudio(false);
    obj.setURL("foo:bar");
    
    ISimpleMediaFile copy = new SimpleMediaFile(obj);
    assertEquals(obj.getAudioBitRate(), copy.getAudioBitRate());
    assertEquals(obj.isAudioBitRateKnown(), copy.isAudioBitRateKnown());
    assertEquals(obj.getAudioChannels(), copy.getAudioChannels());
    assertEquals(obj.isAudioChannelsKnown(), copy.isAudioChannelsKnown());
    assertEquals(obj.getAudioSampleRate(), copy.getAudioSampleRate());
    assertEquals(obj.isAudioSampleRateKnown(), copy.isAudioSampleRateKnown());
    assertEquals(obj.getAudioCodec(), copy.getAudioCodec());
    assertEquals(obj.getAudioTimeBase().getNumerator(), copy.getAudioTimeBase().getNumerator());
    assertEquals(obj.getAudioTimeBase().getDenominator(), copy.getAudioTimeBase().getDenominator());
    assertEquals(obj.getVideoWidth(), copy.getVideoWidth());
    assertEquals(obj.isVideoWidthKnown(), copy.isVideoWidthKnown());
    assertEquals(obj.getVideoHeight(), copy.getVideoHeight());
    assertEquals(obj.isVideoHeightKnown(), copy.isVideoHeightKnown());
    assertEquals(obj.getVideoTimeBase().getNumerator(), copy.getVideoTimeBase().getNumerator());
    assertEquals(obj.getVideoTimeBase().getDenominator(), copy.getVideoTimeBase().getDenominator());
    assertEquals(obj.getVideoCodec(), copy.getVideoCodec());
    assertEquals(obj.getVideoPixelFormat(), copy.getVideoPixelFormat());
    assertEquals(obj.isVideoPixelFormatKnown(), copy.isVideoPixelFormatKnown());
    assertEquals(obj.getVideoNumPicturesInGroupOfPictures(), copy.getVideoNumPicturesInGroupOfPictures());
    assertEquals(obj.isVideoNumPicturesInGroupOfPicturesKnown(), copy.isVideoNumPicturesInGroupOfPicturesKnown());
    assertEquals(obj.getVideoFrameRate().getNumerator(), copy.getVideoFrameRate().getNumerator());
    assertEquals(obj.getVideoFrameRate().getDenominator(), copy.getVideoFrameRate().getDenominator());
    assertEquals(obj.getVideoGlobalQuality(), copy.getVideoGlobalQuality());
    assertEquals(obj.isVideoGlobalQualityKnown(), copy.isVideoGlobalQualityKnown());
    assertEquals(obj.hasVideo(), copy.hasVideo());
    assertEquals(obj.hasAudio(), copy.hasVideo());
    assertEquals(obj.getContainerFormat().getInputFormatLongName(), copy.getContainerFormat().getInputFormatLongName());
    assertEquals(obj.getDuration(), copy.getDuration());
    assertEquals(obj.getURL(), copy.getURL());
  }
  
  @Test
  public void testAudioBitRate()
  {
    int defaultVal = 64000;
    int val=123;
    ISimpleMediaFile obj = new SimpleMediaFile();
    assertEquals("unexpected default", defaultVal, obj.getAudioBitRate());
    obj.setAudioBitRate(val);
    assertEquals("set method failed", val, obj.getAudioBitRate());
  }

  @Test
  public void testAudioChannels()
  {
    int defaultVal = 1;
    int val=2;
    ISimpleMediaFile obj = new SimpleMediaFile();
    assertEquals("unexpected default", defaultVal, obj.getAudioChannels());
    obj.setAudioChannels(val);
    assertEquals("set method failed", val, obj.getAudioChannels());
  }
  
  @Test
  public void testAudioSampleRate()
  {
    int defaultVal = 44100;
    int val=22050;
    ISimpleMediaFile obj = new SimpleMediaFile();
    assertEquals("unexpected default", defaultVal, obj.getAudioSampleRate());
    obj.setAudioSampleRate(val);
    assertEquals("set method failed", val, obj.getAudioSampleRate());
  }
  
  @Test
  public void testAudioCodec()
  {
    ICodec.ID defaultVal = ICodec.ID.CODEC_ID_NONE;
    ICodec.ID val=ICodec.ID.CODEC_ID_AAC;
    assertNotNull("couldn't find codec", val);
    
    ISimpleMediaFile obj = new SimpleMediaFile();
    assertEquals("unexpected default", defaultVal, obj.getAudioCodec());
    obj.setAudioCodec(val);
    assertEquals("set method failed", val, obj.getAudioCodec());
  }

  @Test
  public void testAudioTimeBase()
  {
    IRational val=IRational.make(1, 500);
    ISimpleMediaFile obj = new SimpleMediaFile();
    assertNull(obj.getAudioTimeBase());
    obj.setAudioTimeBase(val);
    assertEquals("set method failed", val.getNumerator(), obj.getAudioTimeBase().getNumerator());
    assertEquals("set method failed", val.getDenominator(), obj.getAudioTimeBase().getDenominator());
  }
  
  @Test
  public void testVideoCodec()
  {
    ICodec.ID defaultVal = ICodec.ID.CODEC_ID_NONE;
    ICodec.ID val=ICodec.ID.CODEC_ID_MPEG4;
    assertNotNull("couldn't find codec", val);
    
    ISimpleMediaFile obj = new SimpleMediaFile();
    assertEquals("unexpected default", defaultVal, obj.getVideoCodec());
    obj.setVideoCodec(val);
    assertEquals("set method failed", val, obj.getVideoCodec());
  }
  
  @Test
  public void testVideoHeight()
  {
    int defaultVal = 1;
    int val=123;
    ISimpleMediaFile obj = new SimpleMediaFile();
    assertEquals("unexpected default", defaultVal, obj.getVideoHeight());
    obj.setVideoHeight(val);
    assertEquals("set method failed", val, obj.getVideoHeight());    
  }
  
  @Test
  public void testVideoWidth()
  {
    int defaultVal = 1;
    int val=123;
    ISimpleMediaFile obj = new SimpleMediaFile();
    assertEquals("unexpected default", defaultVal, obj.getVideoWidth());
    obj.setVideoWidth(val);
    assertEquals("set method failed", val, obj.getVideoWidth());    
  }
  
  @Test
  public void testVideoBitRate()
  {
    int defaultVal = 320000;
    int val=123;
    ISimpleMediaFile obj = new SimpleMediaFile();
    assertEquals("unexpected default", defaultVal, obj.getVideoBitRate());
    obj.setVideoBitRate(val);
    assertEquals("set method failed", val, obj.getVideoBitRate());    
  }
  
  @Test
  public void testVideoTimeBase()
  {
    IRational val=IRational.make(1, 500);
    ISimpleMediaFile obj = new SimpleMediaFile();
    assertNull(obj.getVideoTimeBase());
    obj.setVideoTimeBase(val);
    assertEquals("set method failed", val.getNumerator(), obj.getVideoTimeBase().getNumerator());
    assertEquals("set method failed", val.getDenominator(), obj.getVideoTimeBase().getDenominator());
  }
  
  @Test
  public void testVideoFrameRate()
  {
    IRational val=IRational.make(30, 1);
    ISimpleMediaFile obj = new SimpleMediaFile();
    assertNull(obj.getVideoFrameRate());
    obj.setVideoFrameRate(val);
    assertEquals("set method failed", val.getNumerator(), obj.getVideoFrameRate().getNumerator());
    assertEquals("set method failed", val.getDenominator(), obj.getVideoFrameRate().getDenominator());
  }
  
  @Test
  public void testVideoPixelFormat()
  {
    IPixelFormat.Type defaultVal = IPixelFormat.Type.YUV420P;;
    IPixelFormat.Type val=IPixelFormat.Type.RGB24;
    ISimpleMediaFile obj = new SimpleMediaFile();
    assertEquals("unexpected default", defaultVal, obj.getVideoPixelFormat());
    obj.setVideoPixelFormat(val);
    assertEquals("set method failed", val, obj.getVideoPixelFormat());    
  }
  
  @Test
  public void testVideoNumPicturesInGroupOfPictures()
  {
    int defaultVal = 15;
    int val=123;
    ISimpleMediaFile obj = new SimpleMediaFile();
    assertEquals("unexpected default", defaultVal, obj.getVideoNumPicturesInGroupOfPictures());
    obj.setVideoNumPicturesInGroupOfPictures(val);
    assertEquals("set method failed", val, obj.getVideoNumPicturesInGroupOfPictures());    
  }
  
  @Test
  public void testVideoGlobalQuality()
  {
    int defaultVal = 0;
    int val=123;
    ISimpleMediaFile obj = new SimpleMediaFile();
    assertEquals("unexpected default", defaultVal, obj.getVideoGlobalQuality());
    obj.setVideoGlobalQuality(val);
    assertEquals("set method failed", val, obj.getVideoGlobalQuality());    
  }
  
  @Test
  public void testHasVideo()
  {
    boolean defaultVal = true;
    boolean val = false;
    ISimpleMediaFile obj = new SimpleMediaFile();
    assertEquals("unexpected default", defaultVal, obj.hasVideo());
    obj.setHasVideo(val);
    assertEquals("set method failed", val, obj.hasVideo());    
  }

  @Test
  public void testHasAudio()
  {
    boolean defaultVal = true;
    boolean val = false;
    ISimpleMediaFile obj = new SimpleMediaFile();
    assertEquals("unexpected default", defaultVal, obj.hasAudio());
    obj.setHasAudio(val);
    assertEquals("set method failed", val, obj.hasAudio());    
  }  

  @Test
  public void testContainerFormat()
  {
    IContainerFormat defaultVal = null;
    ISimpleMediaFile obj = new SimpleMediaFile();
    assertEquals("unexpected default", defaultVal, obj.getContainerFormat());

    IContainerFormat format = IContainerFormat.make();
    assertNotNull(format);
    format.setInputFormat("flv");
    assertNotNull(format.getInputFormatLongName());
    assertTrue(format.getInputFormatLongName().length()>0);
    obj.setContainerFormat(format);
    assertEquals(format.getInputFormatLongName(), obj.getContainerFormat().getInputFormatLongName());
  }
  
  @Test
  public void testDuration()
  {
    ITimeValue defaultVal = null;
    ISimpleMediaFile obj = new SimpleMediaFile();
    assertEquals("unexpected default", defaultVal, obj.getDuration());
    
    ITimeValue expected = ITimeValue.make(3, ITimeValue.Unit.SECONDS);
    obj.setDuration(expected);
    assertEquals(expected, obj.getDuration());
  }

  @Test
  public void testURL()
  {
    String defaultVal = null;
    ISimpleMediaFile obj = new SimpleMediaFile();
    assertEquals("unexpected default", defaultVal, obj.getURL());
    
    String expected = "hello:three";
    obj.setURL(expected);
    assertEquals(expected, obj.getURL());
  }

}

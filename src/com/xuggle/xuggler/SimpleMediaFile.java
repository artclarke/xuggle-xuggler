package com.xuggle.xuggler;

import com.xuggle.xuggler.IAudioSamples;
import com.xuggle.xuggler.ICodec;
import com.xuggle.xuggler.IContainerFormat;
import com.xuggle.xuggler.IPixelFormat;
import com.xuggle.xuggler.IRational;
import com.xuggle.xuggler.ITimeValue;
import com.xuggle.xuggler.IAudioSamples.Format;

/**
 * An implementation of {@link ISimpleMediaFile}.
 * 
 * Provides a generic bag of attributes about a SimpleMediaFile.
 */
public class SimpleMediaFile implements ISimpleMediaFile
{
  private String mURL = null;
  private boolean mHasAudio = true;
  private int mAudioBitRate = 64000;
  private boolean mAudioBitRateKnown = false;
  private int mAudioChannels = 1;
  private boolean mAudioChannelsKnown = false;
  private int mAudioSampleRate = 44100;
  private boolean mAudioSampleRateKnown = false;
  private IRational mAudioTimeBase = null;
  private ICodec.ID mAudioCodec = ICodec.ID.CODEC_ID_NONE;
  private IAudioSamples.Format mAudioSamplesFormat = IAudioSamples.Format.FMT_S16;
  
  private boolean mHasVideo = true;
  private ICodec.ID mVideoCodec = ICodec.ID.CODEC_ID_NONE;
  private int mVideoHeight = 1;
  private boolean mVideoHeightKnown = false;
  
  private int mVideoWidth = 1;
  private boolean mVideoWidthKnown = false;
  
  private int mVideoBitRate = 320000;
  private boolean mVideoBitRateKnown = false;
  private IRational mVideoTimeBase = null;
  private IRational mVideoFrameRate = null;
  private int mVideoGOPS = 15;
  private boolean mVideoGOPSKnown=false;
  private int mVideoGlobalQuality = 0;
  private boolean mVideoGlobalQualityKnown=false;
  private IPixelFormat.Type mPixelFormat = IPixelFormat.Type.YUV420P;
  private boolean mVideoPixelFormatKnown = false;
  private IContainerFormat mContainerFormat = null;
  private ITimeValue mDuration = null;


  /**
   * Create a new {@link SimpleMediaFile}.
   */
  public SimpleMediaFile()
  {
  }
  /**
   * Create a new {@link SimpleMediaFile} from an existing {@link SimpleMediaFile}.
   * 
   * All values are shallow copied.
   * 
   * @param src The value to copy.
   * 
   * @throws IllegalArgumentException if src is null
   */
  public SimpleMediaFile(ISimpleMediaFile src)
  {
    if (src == null)
      throw new IllegalArgumentException("cannot pass null src");
    setHasAudio(src.hasAudio());
    setAudioBitRate(src.getAudioBitRate());
    setAudioChannels(src.getAudioChannels());
    setAudioSampleRate(src.getAudioSampleRate());
    setAudioSampleFormat(src.getAudioSampleFormat());
    setHasVideo(src.hasVideo());
    setAudioCodec(src.getAudioCodec());
    setVideoCodec(src.getVideoCodec());
    setVideoHeight(src.getVideoHeight());
    setVideoWidth(src.getVideoWidth());
    IRational srcRational = src.getVideoTimeBase();
    setVideoTimeBase(srcRational);
    srcRational = src.getVideoFrameRate();
    setVideoFrameRate(srcRational);
    srcRational = src.getAudioTimeBase();
    setAudioTimeBase(srcRational);
    setVideoNumPicturesInGroupOfPictures(src.getVideoNumPicturesInGroupOfPictures());
    setVideoGlobalQuality(src.getVideoGlobalQuality());
    setVideoPixelFormat(src.getVideoPixelFormat());
    setContainerFormat(src.getContainerFormat());
    setDuration(src.getDuration());
    
    setURL(src.getURL());
    
    // set the "known" flags last.
    setAudioBitRateKnown(src.isAudioBitRateKnown());
    setAudioChannelsKnown(src.isAudioChannelsKnown());
    setAudioSampleRateKnown(src.isAudioSampleRateKnown());
    
    setVideoHeightKnown(src.isVideoHeightKnown());
    setVideoWidthKnown(src.isVideoWidthKnown());
    setVideoNumPicturesInGroupOfPicturesKnown(src.isVideoNumPicturesInGroupOfPicturesKnown());
    setVideoPixelFormatKnown(src.isVideoPixelFormatKnown());
    setVideoGlobalQualityKnown(src.isVideoGlobalQualityKnown());
    
  }
  
  /**
   * {@inheritDoc}
   */
  public int getAudioBitRate()
  {
    return mAudioBitRate;
  }

  /**
   * {@inheritDoc}
   */
  public int getAudioChannels()
  {
    return mAudioChannels;
  }

  /**
   * {@inheritDoc}
   */
  public int getAudioSampleRate()
  {
    return mAudioSampleRate;
  }

  /**
   * {@inheritDoc}
   */
  public void setAudioBitRate(int bitRate)
  {
    mAudioBitRate = bitRate;
    setAudioBitRateKnown(true);
  }

  /**
   * {@inheritDoc}
   */
  public void setAudioChannels(int numChannels)
  {
    if (numChannels < 1 || numChannels > 2)
      throw new IllegalArgumentException("only supports 1 or 2 channels");
    
    mAudioChannels = numChannels;
    setAudioChannelsKnown(true);
  }

  /**
   * {@inheritDoc}
   */
  public void setAudioSampleRate(int sampleRate)
  {
    mAudioSampleRate = sampleRate;
    setAudioSampleRateKnown(true);
  }

  /**
   * {@inheritDoc}
   */
  public ICodec.ID getAudioCodec()
  {
    return mAudioCodec;
  }

  /**
   * {@inheritDoc}
   */
  public void setAudioCodec(ICodec.ID audioCodec)
  {
    mAudioCodec = audioCodec;
  }
  /**
   * {@inheritDoc}
   */
  public ICodec.ID getVideoCodec()
  {
    return mVideoCodec;
  }
  /**
   * {@inheritDoc}
   */
  public int getVideoHeight()
  {
    return mVideoHeight;
  }
  /**
   * {@inheritDoc}
   */
  public int getVideoWidth()
  {
    return mVideoWidth;
  }
  /**
   * {@inheritDoc}
   */
  public int getVideoBitRate()
  {
    return mVideoBitRate;
  }
  /**
   * {@inheritDoc}
   */
  public void setVideoCodec(ICodec.ID videoCodec)
  {
    mVideoCodec = videoCodec;
  }
  /**
   * {@inheritDoc}
   */
  public void setVideoHeight(int height)
  {
    mVideoHeight = height;
    setVideoHeightKnown(true);
  }
  /**
   * {@inheritDoc}
   */
  public void setVideoWidth(int width)
  {
    mVideoWidth = width;
    setVideoWidthKnown(true);
  }
  /**
   * {@inheritDoc}
   */
  public IRational getVideoTimeBase()
  {
    return mVideoTimeBase ;
  }
  /**
   * {@inheritDoc}
   */
  public void setVideoTimeBase(IRational timeBase)
  {
    mVideoTimeBase = timeBase;
  }
  /**
   * {@inheritDoc}
   */
  public void setVideoBitRate(int bitRate)
  {
    mVideoBitRate = bitRate;
    setVideoBitRateKnown(true);
  }
  /**
   * {@inheritDoc}
   */
  public IPixelFormat.Type getVideoPixelFormat()
  {
    return mPixelFormat;
  }
  /**
   * {@inheritDoc}
   */
  public void setVideoPixelFormat(IPixelFormat.Type pixelFormat)
  {
    mPixelFormat = pixelFormat;
    setVideoPixelFormatKnown(true);
  }
  /**
   * {@inheritDoc}
   */
  public IRational getVideoFrameRate()
  {
    return mVideoFrameRate;
  }

  /**
   * {@inheritDoc}
   */
  public int getVideoGlobalQuality()
  {
    return mVideoGlobalQuality;
  }

  /**
   * {@inheritDoc}
   */
  public int getVideoNumPicturesInGroupOfPictures()
  {
    return mVideoGOPS;
  }

  /**
   * {@inheritDoc}
   */
  public void setVideoFrameRate(IRational frameRate)
  {
    mVideoFrameRate = frameRate;
  }

  /**
   * {@inheritDoc}
   */
  public void setVideoGlobalQuality(int quality)
  {
    mVideoGlobalQuality = quality;
    setVideoGlobalQualityKnown(true);
  }

  /**
   * {@inheritDoc}
   */
  public void setVideoNumPicturesInGroupOfPictures(int gops)
  {
    mVideoGOPS = gops;
    setVideoNumPicturesInGroupOfPicturesKnown(true);
  }

  /**
   * {@inheritDoc}
   */
  public boolean hasAudio()
  {
    return mHasAudio;
  }

  /**
   * {@inheritDoc}
   */
  public boolean hasVideo()
  {
    return mHasVideo;
  }

  /**
   * {@inheritDoc}
   */
  public void setHasAudio(boolean hasAudio)
  {
    mHasAudio = hasAudio;
  }

  /**
   * {@inheritDoc}
   */
  public void setHasVideo(boolean hasVideo)
  {
    mHasVideo = hasVideo;    
  }
  /**
   * {@inheritDoc}
   */
  public IContainerFormat getContainerFormat()
  {
    return mContainerFormat;
  }
  /**
   * {@inheritDoc}
   */
  public void setContainerFormat(IContainerFormat aFormat)
  {
    mContainerFormat = aFormat;
  }
  /**
   * {@inheritDoc}
   */
  public IRational getAudioTimeBase()
  {
    return mAudioTimeBase;
  }
  /**
   * {@inheritDoc}
   */
  public void setAudioTimeBase(IRational aTimeBase)
  {
    mAudioTimeBase = aTimeBase;
  }
  /**
   * {@inheritDoc}
   */
  public void setAudioBitRateKnown(boolean audioBitRateKnown)
  {
    mAudioBitRateKnown = audioBitRateKnown;
  }

  /**
   * {@inheritDoc}
   */
  public boolean isAudioBitRateKnown()
  {
    return mAudioBitRateKnown;
  }
  /**
   * {@inheritDoc}
   */
  public void setAudioChannelsKnown(boolean audioChannelsKnown)
  {
    mAudioChannelsKnown = audioChannelsKnown;
  }
  /**
   * {@inheritDoc}
   */
  public boolean isAudioChannelsKnown()
  {
    return mAudioChannelsKnown;
  }
  /**
   * {@inheritDoc}
   */
  public void setAudioSampleRateKnown(boolean audioSampleRateKnown)
  {
    mAudioSampleRateKnown = audioSampleRateKnown;
  }
  /**
   * {@inheritDoc}
   */
  public boolean isAudioSampleRateKnown()
  {
    return mAudioSampleRateKnown;
  }
  /**
   * {@inheritDoc}
   */
  public void setVideoHeightKnown(boolean videoHeightKnown)
  {
    mVideoHeightKnown = videoHeightKnown;
  }
  /**
   * {@inheritDoc}
   */
  public boolean isVideoHeightKnown()
  {
    return mVideoHeightKnown;
  }
  /**
   * {@inheritDoc}
   */
  public void setVideoWidthKnown(boolean videoWidthKnown)
  {
    mVideoWidthKnown = videoWidthKnown;
  }
  /**
   * {@inheritDoc}
   */
  public boolean isVideoWidthKnown()
  {
    return mVideoWidthKnown;
  }
  /**
   * {@inheritDoc}
   */
  public void setVideoBitRateKnown(boolean videoBitRateKnown)
  {
    mVideoBitRateKnown = videoBitRateKnown;
  }
  /**
   * {@inheritDoc}
   */
  public boolean isVideoBitRateKnown()
  {
    return mVideoBitRateKnown;
  }
  /**
   * {@inheritDoc}
   */
  public void setVideoNumPicturesInGroupOfPicturesKnown(boolean videoGOPSKnown)
  {
    mVideoGOPSKnown = videoGOPSKnown;
  }
  /**
   * {@inheritDoc}
   */
  public boolean isVideoNumPicturesInGroupOfPicturesKnown()
  {
    return mVideoGOPSKnown;
  }
  /**
   * {@inheritDoc}
   */
  public void setVideoGlobalQualityKnown(boolean videoGlobalQualityKnown)
  {
    mVideoGlobalQualityKnown = videoGlobalQualityKnown;
  }
  /**
   * {@inheritDoc}
   */
  public boolean isVideoGlobalQualityKnown()
  {
    return mVideoGlobalQualityKnown;
  }
  /**
   * {@inheritDoc}
   */
  public void setVideoPixelFormatKnown(boolean videoPixelFormatKnown)
  {
    mVideoPixelFormatKnown = videoPixelFormatKnown;
  }
  /**
   * {@inheritDoc}
   */
  public boolean isVideoPixelFormatKnown()
  {
    return mVideoPixelFormatKnown;
  }

  /**
   * {@inheritDoc}
   */
  public void setDuration(ITimeValue duration)
  {
    mDuration = duration;
  }

  /**
   * {@inheritDoc}
   */
  public ITimeValue getDuration()
  {
    return mDuration;
  }
  
  /**
   * {@inheritDoc}
   */
  public void setURL(String uRL)
  {
    mURL = uRL;
  }
  
  /**
   * {@inheritDoc}
   */
  public String getURL()
  {
    return mURL;
  }
  
  /**
   * {@inheritDoc}
   */
  public Format getAudioSampleFormat()
  {
    return mAudioSamplesFormat;
  }
  
  /**
   * {@inheritDoc}
   */
  public void setAudioSampleFormat(Format aFormat)
  {
    if (aFormat == null)
      throw new IllegalArgumentException("cannot set to null format");
    mAudioSamplesFormat = aFormat;
  }

}

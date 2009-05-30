package com.xuggle.mediatool;

public interface IMediaViewer
{
  /**
   * The mode you want to view media in.
   * @author aclarke
   *
   */
  public enum Mode
  {
    /** Play audio & video streams in real-time. */

    AUDIO_VIDEO(true, true, true),

    /** Play only audio streams, in real-time. */

    AUDIO_ONLY(true, false, true),

    /** Play only video streams, in real-time. */

    VIDEO_ONLY(false, true, true),

    /** Play only video, as fast as possible. */

    FAST_VIDEO(false, true, false),

    /** Play neither audio or video, also disables statistics. */

    DISABLED(false, false, false);

    // play audio
    
    private final boolean mPlayAudio;
    
    // show video
    
    private final boolean mShowVideo;

    // show media in real time

    private final boolean mRealtime;

    // construct a mode

    private Mode(boolean playAudio, boolean showVideo, boolean realTime)
    {
      mPlayAudio = playAudio;
      mShowVideo = showVideo;
      mRealtime = realTime;
    }

    /**
     * Does this mode play audio?
     * @return
     */
    public boolean playAudio()
    {
      return mPlayAudio;
    }

    /**
     * Does this mode display video?
     * @return displays video
     */
    public boolean showVideo()
    {
      return mShowVideo;
    }

    /**
     * Does this mode play in real time or in fast time?
     * @return true if real time
     */
    public boolean isRealTime()
    {
      return mRealtime;
    }
  }

  /**
   * Will this viewer show a stats window?
   * @return will this viewer show a stats window?
   */
  public abstract boolean willShowStatsWindow();

  /**
   * Get the default close operation.
   * @return the default close operation
   */
  public abstract int getDefaultCloseOperation();

  /** Get media playback mode.
   *
   * @return the playback mode
   * 
   * @see MediaViewer.Mode
   */

  public abstract Mode getMode();

}

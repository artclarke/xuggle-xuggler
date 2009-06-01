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

package com.xuggle.mediatool;

import javax.sound.sampled.DataLine;

/**
 * An {@link IMediaPipeListener} that plays audio, video or both, while
 * listening to a {@link IMediaPipe} that produces raw media.
 * 
 * <p>
 * 
 * You can use this object to attach to a {@link IMediaReader} or
 * a {@link IMediaWriter} to see the output as they work.
 * 
 * </p>
 * <p>
 * 
 * You can optionally have the {@link IMediaViewer} display statistics
 * on-screen while playing about the contents of the media file, and
 * overlay a clock on the screen while playing.
 * 
 * </p>
 * <p>
 * 
 * Please note that due to limitations in Sun's sound system on Linux
 * there is a lag between audio and video in Linux.  Not much we can do
 * about it, but anyone who knows a fix (the issue is with the precision
 * of {@link DataLine#getMicrosecondPosition()}), please let us know. 
 * 
 * </p>
 */

public interface IMediaViewer extends IMediaPipeListener
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

    FAST_VIDEO_ONLY(false, true, false),

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
     * @return true if we play audio
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

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

import java.lang.Thread;

import java.io.File;

import java.util.Map;
import java.util.Queue;
import java.util.Vector;
import java.util.HashMap;
import java.util.Formatter;
import java.util.LinkedList;
import java.util.Collections;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicLong;
import java.util.concurrent.locks.ReentrantLock;
import java.util.concurrent.locks.Condition;

import java.awt.Color;
import java.awt.Graphics;
import java.awt.Dimension;
import java.awt.Component;
import java.awt.Graphics2D;
import java.awt.RenderingHints;
import java.awt.geom.Rectangle2D;
import java.awt.event.WindowEvent;
import java.awt.image.BufferedImage;
import java.awt.event.WindowAdapter;

import javax.sound.sampled.DataLine;
import javax.sound.sampled.AudioFormat;
import javax.sound.sampled.AudioSystem;
import javax.sound.sampled.SourceDataLine;
import javax.sound.sampled.LineUnavailableException;

import javax.swing.JTable;
import javax.swing.JPanel;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.BoxLayout;
import javax.swing.WindowConstants;
import javax.swing.table.TableModel;
import javax.swing.table.TableColumn;
import javax.swing.table.TableCellRenderer;
import javax.swing.table.AbstractTableModel;

import com.xuggle.mediatool.event.IAddStreamEvent;
import com.xuggle.mediatool.event.IAudioSamplesEvent;
import com.xuggle.mediatool.event.ICloseEvent;
import com.xuggle.mediatool.event.IOpenEvent;
import com.xuggle.mediatool.event.IVideoPictureEvent;
import com.xuggle.xuggler.Global;
import com.xuggle.xuggler.ICodec;
import com.xuggle.xuggler.IStream;
import com.xuggle.xuggler.IMediaData;
import com.xuggle.xuggler.IContainer;
import com.xuggle.xuggler.IStreamCoder;
import com.xuggle.xuggler.IVideoPicture;
import com.xuggle.xuggler.IAudioSamples;
import com.xuggle.xuggler.video.IConverter;
import com.xuggle.xuggler.video.ConverterFactory;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import static java.util.concurrent.TimeUnit.NANOSECONDS;
import static java.util.concurrent.TimeUnit.MILLISECONDS;
import static java.util.concurrent.TimeUnit.MICROSECONDS;

/**
 * An {@link IMediaListener} plays audio, video or both, while listening
 * to a {@link IMediaGenerator} that produces raw media.
 *
 * <p>
 * 
 * You can use this object to attach to a {@link MediaReader} or a
 * {@link MediaWriter} to see the output as they work.
 *
 * </p>
 * <p>
 *
 * You can optionally have the {@link MediaViewer} display statistics
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

class MediaViewer extends MediaListenerAdapter implements IMediaListener, IMediaViewer
{
  private static final Logger log = LoggerFactory.getLogger(MediaViewer.class);

  // the capacity (in time) of media buffers

  private long mVideoQueueCapacity = TIME_UNIT.convert(1000, MILLISECONDS);

  // the capacity (in time) of media buffers

  private long mAudioQueueCapacity = TIME_UNIT.convert(1000, MILLISECONDS);

  // the standard time unit used in the media viewer

  private static final TimeUnit TIME_UNIT = MICROSECONDS;

  /** the stats font size */

  private static final float FONT_SIZE = 20f;

  /** default video early time window, before which video is delayed */

  private static final long DEFAULT_VIDEO_EARLY_WINDOW =
    TIME_UNIT.convert(50, MILLISECONDS);

  /** default video late time window, after which video is dropped */

  private static final long DEFAULT_VIDEO_LATE_WINDOW =
    TIME_UNIT.convert(50, MILLISECONDS);

  /** default audio early time window, before which audio is delayed */

  private static final long DEFAULT_AUDIO_EARLY_WINDOW = 
    TIME_UNIT.convert(Long.MAX_VALUE, MILLISECONDS);

  /** default audio late time window, after which audio is dropped */

  private static final long DEFAULT_AUDIO_LATE_WINDOW =
    TIME_UNIT.convert(Long.MAX_VALUE, MILLISECONDS);

  // video converters

  private final Map<Integer, IConverter> mConverters = 
    new HashMap<Integer, IConverter>();

  private final Map<Integer, MediaFrame> mFrames = 
    new HashMap<Integer, MediaFrame>();

  // video queues

  private final Map<Integer, VideoQueue> mVideoQueues = 
    new HashMap<Integer, VideoQueue>();

  // audio queues

  private final Map<Integer, AudioQueue> mAudioQueues = 
    new HashMap<Integer, AudioQueue>();

  // audio lines

  private final Map<Integer, SourceDataLine> mAudioLines = 
    new HashMap<Integer, SourceDataLine>();

  // video position index

  private final Map<MediaFrame, Integer> mFrameIndex = 
    new HashMap<MediaFrame, Integer>();

  // the container which is to be viewed

  private IContainer mContainer;

  // the statistics frame

  private StatsFrame mStatsFrame;

  // next frame index

  private int mNextFrameIndex = 0;

  // show or hide media statistics

  private final boolean mShowStats;

  // is this viewer in the process of closing

  private boolean mClosing = false;
    
  // display mode

  private Mode mMode;

  // default behavior of windows on close

  private final int mDefaultCloseOperation;

  private final AtomicLong mStartClockTime = new AtomicLong(Global.NO_PTS);

  //  private final AtomicLong mStartContainerTime = new AtomicLong(Global.NO_PTS);

  //  private final AtomicLong mAudioLantency = new AtomicLong(0);

  // the authoratative data line used play media

  private SourceDataLine mDataLine = null;

  /**
   * Construct a default media viewer.
   */

  public MediaViewer()
  {
    this(Mode.AUDIO_VIDEO, false, JFrame.DISPOSE_ON_CLOSE);
  }

  /**
   * Construct a media viewer which plays in the specified mode.
   * 
   * @param mode the play mode of this viewer
   */

  MediaViewer(Mode mode)
  {
    this(mode, false, JFrame.DISPOSE_ON_CLOSE);
  }

  /**
   * Construct a media viewer and optionally show media statistics.
   * 
   * @param showStats display media statistics
   */

  MediaViewer(boolean showStats)
  {
    this(Mode.AUDIO_VIDEO, showStats, JFrame.DISPOSE_ON_CLOSE);
  }

  /**
   * Construct a media viewer which plays in the specified mode and
   * optionally shows media statistics.
   * 
   * @param mode the play mode of this viewer
   * @param showStats display media statistics
   */

  MediaViewer(Mode mode, boolean showStats)
  {
    this(mode, showStats, JFrame.DISPOSE_ON_CLOSE);
  }

  /**
   * Construct a media viewer, optionally show media statistics and
   * specify the default frame close behavior.
   * 
   * @param showStats display media statistics
   * @param defaultCloseOperation what should Swing do if the window is
   *    closed. See the {@link javax.swing.WindowConstants}
   *    documentation for valid values.
   */

  MediaViewer(boolean showStats, int defaultCloseOperation)
  {
    this(Mode.AUDIO_VIDEO, showStats, defaultCloseOperation);
  }

  /**
   * Construct a media viewer which plays in the specified mode, optionally
   * shows media statistics and specifies the default frame close
   * behavior.
   * 
   * @param mode the play mode of this viewer
   * @param showStats display media statistics
   * @param defaultCloseOperation what should Swing do if the window is
   *    closed. See the {@link javax.swing.WindowConstants}
   *    documentation for valid values.
   */

  MediaViewer(Mode mode, boolean showStats,
      int defaultCloseOperation)
  {
    setMode(mode);
    mShowStats = showStats;
    mDefaultCloseOperation = defaultCloseOperation;

    Runtime.getRuntime().addShutdownHook(new Thread()
      {
        public void run()
        {
          mClosing = true;
          for (AudioQueue queue: mAudioQueues.values())
            queue.close();
          for (VideoQueue queue: mVideoQueues.values())
            queue.close();
        }
      });
  }

  /**
   * Will this viewer show a stats window?
   * @return will this viewer show a stats window?
   */
  public boolean willShowStatsWindow()
  {
    return mShowStats;
  }

  /**
   * Get the default close operation.
   * @return the default close operation
   */
  public int getDefaultCloseOperation()
  {
    return mDefaultCloseOperation;
  }
  
  /**
   * Get media time.  This is time used to choose to delay, present, or
   * drop a media frame.
   *
   * @return the current presentation time of the media
   */

  private long getMediaTime()
  {
    // if not in real time mode, then this call is in error

    if (!getMode().isRealTime())
      throw new RuntimeException(
        "requested real time when not in real time mode");

    // if in play audio mode, base media time on audio

    if (getMode().playAudio())
    {
      // if no data line then no time has passed

      if (null == mDataLine)
        return 0;

      // if there is a data line use it's play time to identify media
      // time

      return TIME_UNIT.convert(mDataLine.getMicrosecondPosition(), MICROSECONDS);
    }

    // otherwise base time on clock time

    else
    {
      long now = TIME_UNIT.convert(System.nanoTime(), NANOSECONDS);
      mStartClockTime.compareAndSet(Global.NO_PTS, now);
      return now - mStartClockTime.get();
    }
  }
  
  /** Set media playback mode.
   *
   * @param mode the playback mode
   * 
   * @see MediaViewer.Mode
   */

  private void setMode(Mode mode)
  {
    mMode = mode;
  }

  /** Get media playback mode.
   *
   * @return the playback mode
   * 
   * @see MediaViewer.Mode
   */

  public Mode getMode()
  {
    return mMode;
  }

  /** Internaly Only.  Configure internal parameters of the media viewer. */

  @Override
  public void onAddStream(IAddStreamEvent event)
  {
    // if disabled don't add a stream

    if (getMode() == Mode.DISABLED)
      return;

    // get the coder, and stream index

    IContainer container = event.getSource().getContainer();
    IStream stream = container.getStream(event.getStreamIndex());
    IStreamCoder coder = stream.getStreamCoder();
    int streamIndex = event.getStreamIndex();

    // if video stream and showing video, configure video stream

    if (coder.getCodecType() == ICodec.Type.CODEC_TYPE_VIDEO &&
      getMode().showVideo())
    {
      // create a converter for this video stream

      IConverter converter = mConverters.get(streamIndex);
      if (null == converter)
      {
        converter = ConverterFactory.createConverter(
            ConverterFactory.XUGGLER_BGR_24, coder.getPixelType(), coder
                .getWidth(), coder.getHeight());
        mConverters.put(streamIndex, converter);
      }

      // get a frame for this stream

      MediaFrame frame = mFrames.get(streamIndex);
      if (null == frame)
      {
        frame = new MediaFrame(mDefaultCloseOperation, stream);
        mFrames.put(streamIndex, frame);
        mFrameIndex.put(frame, mNextFrameIndex++);
      }

      // if real time establish video queue

      if (getMode().isRealTime())
        getVideoQueue(streamIndex, frame);
    }

    // if audio stream and playing audio, configure audio stream

    else if (coder.getCodecType() == ICodec.Type.CODEC_TYPE_AUDIO &&
      getMode().playAudio())

    {
      // if real time establish audio queue

      if (getMode().isRealTime())
        getAudioQueue(event.getSource(), streamIndex);
    }
  }

  /** Internaly Only.  {@inheritDoc} */

  @Override
  public void onVideoPicture(IVideoPictureEvent event)
  {
    // if not configured to show video, return now

    if (!getMode().showVideo())
      return;

    // verify container is defined

    if (null == mContainer)
    {
      // if source does not posses a container then throw exception

      if (!(event.getSource() instanceof IMediaCoder))
        throw new UnsupportedOperationException();

      // establish container

      mContainer = ((IMediaCoder)event.getSource()).getContainer();
    }

    // get the frame

    MediaFrame frame = mFrames.get(event.getStreamIndex());

    // if in real time, queue the video frame for viewing

    if (getMode().isRealTime())
      getVideoQueue(event.getStreamIndex(), frame)
        .offerMedia(event.getPicture(), event.getTimeStamp(), MICROSECONDS);

    // otherwise just set the image on the frame

    else
      frame.setVideoImage(event.getPicture(), event.getImage());
  }

  private VideoQueue getVideoQueue(int streamIndex, MediaFrame frame)
  {
    VideoQueue queue = mVideoQueues.get(streamIndex);
    if (null == queue)
    {
      queue = new VideoQueue(mVideoQueueCapacity, TIME_UNIT, frame);
      mVideoQueues.put(streamIndex, queue);
    }
    return queue;
  }

  /** Internaly Only.  {@inheritDoc} */

  @Override
  public void onAudioSamples(IAudioSamplesEvent event)
  {
    // if not configured to play audio, return now

    if (!getMode().playAudio())
      return;

    // verify container is defined

    if (null == mContainer)
    {
      // if source does not posses a container then throw exception

      if (!(event.getSource() instanceof IMediaCoder))
        throw new UnsupportedOperationException();

      // establish container

      mContainer = ((IMediaCoder)event.getSource()).getContainer();
    }

    // establish the audio samples

    final IAudioSamples samples = event.getAudioSamples();

    // if in realtime mode, add samples to queue audio

    if (getMode().isRealTime())
    {
      // establish audio queue

      AudioQueue queue = getAudioQueue(event.getSource(),
          event.getStreamIndex());

      // enqueue the audio samples

      if (queue != null)
        queue.offerMedia(samples, event.getTimeStamp(), event.getTimeUnit());
    }

    // other wise just play the audio directly

    else
    {
      IStream stream = mContainer.getStream(event.getStreamIndex());
      SourceDataLine line = getJavaSoundLine(stream);
      if (line != null)
        playAudio(stream, line, samples);
    }
  }

  /**
   * Play audio samples.
   * 
   * @param stream the source stream of the audio
   * @param line the audio line to play audio samples on
   * @param samples the audio samples
   */

  private void playAudio(IStream stream, SourceDataLine line, 
    IAudioSamples samples)
  {
    if (!mClosing)
    {
      int size = samples.getSize();
      line.write(samples.getData().getByteArray(0, size), 0, size);
      updateStreamStats(stream, samples);
    }
  }

  private AudioQueue getAudioQueue(IMediaGenerator tool, int streamIndex)
  {
    if (!(tool instanceof IMediaCoder))
      throw new UnsupportedOperationException();

    AudioQueue queue = mAudioQueues.get(streamIndex);
    IStream stream = ((IMediaCoder)tool).getContainer().getStream(streamIndex);
    SourceDataLine line = getJavaSoundLine(stream);
    
    // if no queue (and there is a line), create the queue

    if (null == queue && line != null)
    {
      queue = new AudioQueue(mAudioQueueCapacity, TIME_UNIT, stream, line);
      mAudioQueues.put(streamIndex, queue);
    }

    return queue;
  }

  /**
   * Flush all media buffers.
   */

  private void flush()
  {
    // flush all the video queues

    for (VideoQueue queue : mVideoQueues.values())
      queue.flush();

    // flush all the audio queus

    for (AudioQueue queue : mAudioQueues.values())
      queue.flush();

    // wait for all audio lines to close
    for (SourceDataLine line : mAudioLines.values())
    {
      line.drain();
    }
  }

  /**
   * Internal Only.  {@inheritDoc}
   */

  @Override
  public void onOpen(IOpenEvent event)
  {    
    mContainer = event.getSource().getContainer();
  };

  /**
   * Internal Only.  {@inheritDoc} Closes any open windows on screen.
   */

  @Override
  public void onClose(ICloseEvent event)
  {
    flush();
    for (MediaFrame frame : mFrames.values())
      frame.dispose();
    if (null != mStatsFrame)
      mStatsFrame.dispose();
    for (SourceDataLine line : mAudioLines.values())
    {
      line.stop();
      line.close();
    }
  };

  /**
   * Update the statistics for a given media stream.
   *
   * @param stream the stream for which to update the statistics
   * @param mediaData the current media data for this stream
   */

  private void updateStreamStats(IStream stream, IMediaData mediaData)
  {
    if (mShowStats)
    {
      if (null == mStatsFrame)
        mStatsFrame = new StatsFrame(WindowConstants.DISPOSE_ON_CLOSE, this);
      mStatsFrame.update(stream, mediaData);
    }
  }

  /**
   * Show the video time on the video.
   * 
   * @param picture the video picture from which to extract the time
   *        stamp
   * @param image the image on which to draw the time stamp
   */

  private static void drawStats(IVideoPicture picture, BufferedImage image)
  {
    if (image == null)
      throw new RuntimeException("must be used with a IMediaGenerator"
          + " that created BufferedImages");
    Graphics2D g = image.createGraphics();
    g.setRenderingHint(RenderingHints.KEY_ANTIALIASING,
        RenderingHints.VALUE_ANTIALIAS_ON);

    String timeStamp = picture.getFormattedTimeStamp();
    Rectangle2D bounds = g.getFont().getStringBounds(timeStamp,
        g.getFontRenderContext());

    double inset = bounds.getHeight() / 2;
    g.translate(inset, image.getHeight() - inset);

    g.setColor(new Color(255, 255, 255, 128));
    g.fill(bounds);
    g.setColor(Color.BLACK);
    g.drawString(timeStamp, 0, 0);
  }

  /**
   * Open a java audio line out to play the audio samples into.
   * 
   * @param stream the stream we'll be decoding in to this line.
   * @return the line
   */

  private SourceDataLine getJavaSoundLine(IStream stream)
  {
    IStreamCoder audioCoder = stream.getStreamCoder();
    int streamIndex = stream.getIndex();
    SourceDataLine line = mAudioLines.get(streamIndex);
    if (line == null)
    {
      try
      {
        // estabish the audio format, NOTE: xuggler defaults to signed 16 bit
        // samples

        AudioFormat audioFormat = new AudioFormat(audioCoder.getSampleRate(),
          (int) IAudioSamples
          .findSampleBitDepth(audioCoder.getSampleFormat()), audioCoder
          .getChannels(), true, false);
        
        // create the audio line out
        
        DataLine.Info info = new DataLine.Info(SourceDataLine.class,
            audioFormat);
        line = (SourceDataLine) AudioSystem.getLine(info);

        // open the line and start the line

        line.open(audioFormat);
        line.start();
        mAudioLines.put(streamIndex, line);

        // if mDataLine is not yet defined, do so

        if (null == mDataLine)
          mDataLine = line;
      }
      catch (LineUnavailableException lue)
      {
        log.warn("WARINING: No audio line out available: " + lue);
        line = null;
      }
    }
    return line;
  }

  /**
   * Custom debug message
   */

  private static void debug(String format, Object... args)
  {
    Formatter formatter = new Formatter();
    log.debug(formatter.format(format, args).toString());
  }

  /**
   * A queue of audio sampless which automatically plays audio frames at the
   * correct time.
   */

  private class AudioQueue extends SelfServicingMediaQueue
  {
    // removes the warning

    public static final long serialVersionUID = 0;

    // the audio line

    private final SourceDataLine mLine;

    // source audio stream

    private final IStream mStream;

    /**
     * Construct queue and activate it's internal thread.
     * 
     * @param capacity the total duraiton of media stored in the queue
     * @param unit the time unit of the capacity (MILLISECONDS,
     *        MICROSECONDS, etc).
     * @param stream the stream from whence the audio issued forth
     * @param sourceDataLine the swing frame on which samples are
     *          displayed
     */

    public AudioQueue(long capacity, TimeUnit unit, IStream stream,
      SourceDataLine sourceDataLine)
    {
      super(TIME_UNIT.convert(capacity, unit), DEFAULT_AUDIO_EARLY_WINDOW,
          DEFAULT_AUDIO_LATE_WINDOW, TIME_UNIT, Thread.MIN_PRIORITY, 
        "audio stream " + stream.getIndex() + " " + 
        stream.getStreamCoder().getCodec().getLongName());
      mStream = stream;
      mLine = sourceDataLine;
    }

    /** {@inheritDoc} */

    public void dispatch(IMediaData samples, long timeStamp)
    {
      if (samples instanceof IAudioSamples)
        playAudio(mStream, mLine, (IAudioSamples)samples);
    }
  }

  /**
   * A queue of video images which automatically displays video frames at the
   * correct time.
   */

  private class VideoQueue extends SelfServicingMediaQueue
  {
    // removes the warning

    public static final long serialVersionUID = 0;

    // the media frame to display images on

    private final MediaFrame mMediaFrame;

    /**
     * Construct queue and activate it's internal thread.
     * 
     * @param capacity
     *          the total duraiton of media stored in the queue
     * @param unit
     *          the time unit of the capacity (MILLISECONDS, MICROSECONDS, etc).
     * @param mediaFrame
     *          the swing frame on which images are displayed
     */

    public VideoQueue(long capacity, TimeUnit unit, MediaFrame mediaFrame)
    {
      super(TIME_UNIT.convert(capacity, unit), DEFAULT_VIDEO_EARLY_WINDOW,
          DEFAULT_VIDEO_LATE_WINDOW, TIME_UNIT, Thread.MIN_PRIORITY, 
        "video stream " + mediaFrame.mStream.getIndex() + " " + 
        mediaFrame.mStream.getStreamCoder().getCodec().getLongName());
      mMediaFrame = mediaFrame;
    }

    /** {@inheritDoc} */

    public void dispatch(IMediaData picture, long timeStamp)
    {
      if (picture instanceof IVideoPicture)
        mMediaFrame.setVideoImage((IVideoPicture)picture, null);
    }
  }

  /**
   * When created, this queue start a thread which extracts media frames in a
   * timely way and presents them to the analog hole (viewer).
   */

  private abstract class SelfServicingMediaQueue
  {
    /**
     * to make warning go away
     */

    private static final long serialVersionUID = 1L;

    private final Queue<DelayedItem<IMediaData>> mQueue =  new
      LinkedList<DelayedItem<IMediaData>>();
    
    // the lock

    private ReentrantLock mLock = new ReentrantLock(true);

    // the locks condition

    private Condition mCondition = mLock.newCondition();

    // if true the queue terminates it's thread

    private boolean mDone = false;

    // the maximum amount of media which will be stored in the buffer

    private final long mCapacity;

    // the time before which media is delayed

    private final long mEarlyWindow;

    // the time after which media is dropped

    private final long mLateWindow;

    private boolean mIsInitialized = false;

    /**
     * Construct queue and activate it's internal thread.
     * 
     * @param capacity
     *          the total duraiton of media stored in the queue
     * @param earlyWindow
     *          the time before which media is delayed
     * @param lateWindow
     *          the time after which media is dropped
     * @param unit
     *          the time unit for capacity and window values (MILLISECONDS,
     *          MICROSECONDS, etc).
     */

    public SelfServicingMediaQueue(long capacity, long earlyWindow,
        long lateWindow, TimeUnit unit, int priority, String name)
    {
      // record capacity, and window

      mCapacity = TIME_UNIT.convert(capacity, unit);
      mEarlyWindow = TIME_UNIT.convert(earlyWindow, unit);
      mLateWindow = TIME_UNIT.convert(lateWindow, unit);

      // create and start the thread

      Thread t = new Thread()
      {
        public void run()
        {
          boolean isDone = false;
          DelayedItem<IMediaData> delayedItem = null;

          // wait for all the other stream threads to wakeup
          synchronized (SelfServicingMediaQueue.this)
          {
            mIsInitialized = true;
            SelfServicingMediaQueue.this.notifyAll();
          }
          // start processing media

          while (!isDone)
          {
            // synchronized (SelfServicingMediaQueue.this)

            mLock.lock();
            try
            {
              // while not done, and no item, wait for one

              while (!mDone && (delayedItem = mQueue.poll()) == null)
              {
                try
                {
                  mCondition.await();
                }
                catch (InterruptedException e)
                {
                  // interrupt and return
                  Thread.currentThread().interrupt();

                  return;
                }
              }

              // notify the queue that data extracted

              mCondition.signalAll();

              // record "atomic" done

              isDone = mDone;
            }
            finally
            {
              mLock.unlock();
            }

            // if there is an item, dispatch it
            if (null != delayedItem)
            {
              IMediaData item = delayedItem.getItem();
              try {
                do
                {
                  // this is the story of goldilocks testing the the media

                  long now = getMediaTime();
                  long delta = delayedItem.getTimeStamp() - now;

                  // if the media is too new and unripe, goldilocks sleeps
                  // for a bit

                  if (delta >= mEarlyWindow)
                  {
                    //debug("delta: " + delta);
                    try
                    {
                      //sleep(MILLISECONDS.convert(delta - mEarlyWindow, TIME_UNIT));
                      sleep(MILLISECONDS.convert(delta / 3, TIME_UNIT));
                    }
                    catch (InterruptedException e)
                    {
                      // interrupt and return
                      Thread.currentThread().interrupt();
                      return;
                    }
                  }
                  else
                  {
                    // if the media is old and moldy, goldilocks says
                    // "ick" and drops the media on the floor

                    if (delta < -mLateWindow)
                    {
                      debug(
                          "@%5d DROP queue[%2d]: %s[%5d] delta: %d",
                          MILLISECONDS.convert(now, TIME_UNIT),
                          mQueue.size(),
                          (item instanceof IVideoPicture ? "IMAGE"
                              : "sound"), MILLISECONDS.convert(delayedItem
                                  .getTimeStamp(), TIME_UNIT), MILLISECONDS.convert(
                                      delta, TIME_UNIT));
                    }

                    // if the media is just right, goldilocks dispaches it
                    // for presentiation becuse she's a badass bitch

                    else
                    {
                      dispatch(item, delayedItem.getTimeStamp());
                      // debug("%5d show [%2d]: %s[%5d] delta: %d",
                      // MILLISECONDS.convert(getPresentationTime(), TIME_UNIT),
                      // size(),
                      // (delayedItem.getItem() instanceof BufferedImage
                      // ? "IMAGE"
                      // : "sound"),
                      // MILLISECONDS.convert(delayedItem.getTimeStamp(),
                      // TIME_UNIT),
                      // MILLISECONDS.convert(delta, TIME_UNIT));
                    }

                    // and the moral of the story is don't mess with goldilocks

                    break;
                  }
                }
                while (!mDone);
              } finally {
                if (item != null)
                  item.delete();
              }
            }
          }
        }
      };

      t.setPriority(priority);
      t.setName(name);
      synchronized (this)
      {
        t.start();
        try
        {
          while (!mIsInitialized)
            this.wait();
        }
        catch (InterruptedException e)
        {
          // interrupt and return
          Thread.currentThread().interrupt();

          throw new RuntimeException("could not start thread");
        }
      }
      debug("thread started and ready for action");

    }

    /**
     * Block until all data is extracted from the buffer.
     */

    public void flush()
    {
      // synchronized (SelfServicingMediaQueue.this)

      mLock.lock();
      try
      {
        while (!mDone && !mQueue.isEmpty())
        {
          try
          {
            mCondition.await();
          }
          catch (InterruptedException e)
          {
            // interrupt and return
            Thread.currentThread().interrupt();
            return;
          }
        }

        mCondition.signalAll();

      }
      finally
      {
        mLock.unlock();
      }
    }

    /**
     * Dispatch an item just removed from the queue.
     * 
     * @param item
     *          the item just removed from the queue
     * @param timeStamp
     *          the presentation time stamp of the item
     */

    public abstract void dispatch(IMediaData item, long timeStamp);

    /**
     * Place an item onto the queue, if the queue is full, block.
     * 
     * @param item
     *          the media item to be placed on the queue
     * @param timeStamp
     *          the presentation time stamp of the item
     * @param unit
     *          the time unit of the time stamp
     */

    public void offerMedia(IMediaData item, long timeStamp, TimeUnit unit)
    {
      // wait for all the other stream threads to wakeup

      // try {mBarrier.await(250, MILLISECONDS);}
      // catch (InterruptedException ex) { return; }
      // catch (BrokenBarrierException ex) { return; }
      // catch (TimeoutException ex) {}

      // convert time stamp to standar time unit

      long convertedTime = TIME_UNIT.convert(timeStamp, unit);

      // synchronized (SelfServicingMediaQueue.this)

      mLock.lock();
      try
      {
        // while not done, and over the buffer capacity, wait till media
        // is draied below it's capacity

        while (!mDone && !mQueue.isEmpty()
            && (convertedTime - mQueue.peek().getTimeStamp()) > mCapacity)
        {
          try
          {
            // log.debug("Reader blocking; ts:{}; media:{}",
            // timeStamp, item);
            mCondition.await();
          }
          catch (InterruptedException e)
          {
            // interrupt and return
            Thread.currentThread().interrupt();
            return;
          }
        }

        // if not done, put item on the queue

        if (!mDone)
        {
          // debug("1     queue[%2d]: %s[%5d]",
          // size(),
          // (item instanceof BufferedImage ? "IMAGE" : "SOUND"),
          // MILLISECONDS.convert(convertedTime, TIME_UNIT));

          // put a COPY on the queue
          mQueue.offer(new DelayedItem<IMediaData>(item.copyReference(),
              convertedTime));

          // debug("2     queue[%2d]: %s[%5d]",
          // size(),
          // (item instanceof BufferedImage ? "IMAGE" : "SOUND"),
          // MILLISECONDS.convert(convertedTime, TIME_UNIT));
        }

        // notify that things have changed

        mCondition.signalAll();
      }
      finally
      {
        mLock.unlock();
      }

    }

    /** Stipulate that this queue is terminate it's internal thread. */

    public void close()
    {
      mLock.lock();

      try
      {
        mDone = true;
        mCondition.signalAll();
      }
      finally
      {
        mLock.unlock();
      }
    }
  }

  /** A place to put frames which will be delayed. */

  private class DelayedItem<Item>
  {
    // buffered image

    private final Item mItem;

    // time stamp

    private final long mTimeStamp;

    // constrcut a delayed frame

    public DelayedItem(Item item, long timeStamp)
    {
      mItem = item;
      mTimeStamp = timeStamp;
    }

    // get the buffered item

    public Item getItem()
    {
      return mItem;
    }

    // get the timestamp

    public long getTimeStamp()
    {
      return mTimeStamp;
    }

  }

  /** A JFrame which initially positions itself in a smart way */


  private static class PositionFrame extends JFrame
  {
    // removes the warning

    public static final long serialVersionUID = 0;

    // a collection of all know frames

    private static Vector<PositionFrame> mFrames = new Vector<PositionFrame>();

    /** Consruct a self positioning frame.
     * 
     * @param defaultCloseOperation what should Swing do if the window
     *        is closed. See the {@link javax.swing.WindowConstants}
     *        documentation for valid values.
     */
    
    public PositionFrame(int defaultCloseOperation)
    {
      setDefaultCloseOperation(defaultCloseOperation);

      if (mFrames.size() > 0)
        reposition(mFrames.lastElement());

      mFrames.add(this);

      addWindowListener(new WindowAdapter()
        {
          public void windowClosed(WindowEvent e)
          {
            mFrames.remove(PositionFrame.this);
            if (!mFrames.isEmpty())
            {
              PositionFrame frame = mFrames.firstElement();
              frame.setLocation(0,0);
              repositionFrom(frame);
            }
          }
        });
    }

    // repostion this frame

    protected void reposition(PositionFrame other)
    {
      setLocation(other.getX() + other.getWidth(), other.getY());
    }

    // repostion all frames from a given frame
    
    public void repositionFrom(PositionFrame frame)
    {
      if (mFrames.contains(frame))
        for (int i = mFrames.indexOf(frame) + 1; i < mFrames.size(); ++i)
          mFrames.get(i).reposition(mFrames.get(i - 1));
    }

    // resize window to fit frame

    protected void adjustSize()
    {
      pack();
      invalidate();
      repositionFrom(this);
    }
  }

  /** A media viewer frame. */

  private class MediaFrame extends PositionFrame
  {
    // removes the warning

    public static final long serialVersionUID = 0;

    // the video image

    private BufferedImage mImage;

    // the video panel

    private final JPanel mVideoPanel;

    // the stream

    private final IStream mStream;

    // the index of the stream (incase it's closed)

    private final int mStreamIndex;

    /**
     * Construct a media frame.
     * 
     * @param defaultCloseOperation what should Swing do if the window
     *        is closed. See the {@link javax.swing.WindowConstants}
     *        documentation for valid values.
     * @param stream the stream which will appear in this frame
     */

    public MediaFrame(int defaultCloseOperation, IStream stream)
    {
      super(defaultCloseOperation);

      // get stream and set title based it

      // keep our own copy of the stream since we'll live in a separate thread
      mStream = stream.copyReference();
      mStreamIndex = mStream.getIndex();
      setTitle("Stream #" + mStreamIndex + ", " + 
        mStream.getStreamCoder().getCodec().getLongName());

      // the panel which shows the video image

      mVideoPanel = new JPanel()
      {
        public static final long serialVersionUID = 0;

        /** {@inheritDoc} */

        public void paint(Graphics graphics)
        {
          paintPanel((Graphics2D) graphics);
        }
      };

      // add the videoPanel

      getContentPane().add(mVideoPanel);

      // show the frame

      setVisible(true);
    }

    // configure the size of a the video panel

    protected void setVideoSize(Dimension videoSize)
    {
      mVideoPanel.setPreferredSize(videoSize);
      adjustSize();
    }

    // set the video image

    protected void setVideoImage(IVideoPicture picture, BufferedImage image)
    {
      // if the image is null, convert the picture to the image

      if (null == image)
      {
        IConverter converter = mConverters.get(mStreamIndex);
        image = converter.toImage(picture);
      }

      if (mShowStats)
      {
        drawStats(picture, image);
        updateStreamStats(mStream, picture);
      }

      mImage = image;
      if (null != image)
      {
        if (mVideoPanel.getWidth() != mImage.getWidth() ||
          mVideoPanel.getHeight() != mImage.getHeight())
        {
          setVideoSize(new Dimension(mImage.getWidth(), mImage.getHeight()));
        }
        repaint();
      }
    }

    /** paint the video panel */

    protected void paintPanel(Graphics2D graphics)
    {
      // call parent paint

      super.paint(graphics);

      // if the image exists, draw it

      if (mImage != null)
        graphics.drawImage(mImage, 0, 0, null);
    }
  }

  /** A stats frame. */

  private static class StatsFrame extends PositionFrame
  {
    // removes the warning

    public static final long serialVersionUID = 0;

    // the statistics panel

    private final JPanel mStatsPanel;

    // the viewer object

    private final MediaViewer mViewer;

    // the layout

    private final BoxLayout mLayout;

    // the panels for each stream

    private final Map<IStream, StreamPanel> mStreamPanels = 
      new HashMap<IStream, StreamPanel>();

    /**
     * Construct a stats frame.
     * 
     * @param defaultCloseOperation what should Swing do if the window
     *        is closed. See the {@link javax.swing.WindowConstants}
     *        documentation for valid values.
     * @param viewer the parent media viewer
     */

    public StatsFrame(int defaultCloseOperation, MediaViewer viewer)
    {
      super(defaultCloseOperation);

      // get the viewer

      mViewer = viewer;

      // set the title based on the container

      File file = new File(mViewer.mContainer.getURL());
      setTitle("Statistics " + file.getName());

      // the panel which contains the stats

      mStatsPanel = new JPanel();
      mLayout = new BoxLayout(mStatsPanel, BoxLayout.Y_AXIS);
      mStatsPanel.setLayout(mLayout);

      // add the videoPanel

      getContentPane().add(mStatsPanel);
    }

    // update a stream's statistics

    protected void update(IStream stream, IMediaData mediaData)
    {
      if (!isVisible())
        setVisible(true);

      StreamPanel streamPanel = mStreamPanels.get(stream);
      if (streamPanel == null)
      {
        streamPanel = new StreamPanel(stream, this);
        mStreamPanels.put(stream.copyReference(), streamPanel);
        mStatsPanel.add(streamPanel);
        adjustSize();
      }
      
      streamPanel.update(mediaData);
      //mLayout.invalidateLayout(mStatsPanel);
      //adjustSize();
    }

    // a panel for stream stats

    protected static class StreamPanel extends JPanel
    {

      // removes the warning

      public static final long serialVersionUID = 0;

      // the stream

      private final IStream mStream;

      // the frame

      private final StatsFrame mFrame;

      // media data

      private final Object mMediaLock; 
      private IMediaData mMediaData;

      // the table

      private final TableModel mTableModel;

      // the table

      private final JTable mTable;

      // stream background colors

      private final Color[] mColors = 
      {
//         new Color(0x70, 0x70, 0x70), 
//         new Color(0xA0, 0xA0, 0xA0),
        new Color(0xA0, 0xA0, 0xA0),
      };
      
      // the fields of the display panel

      private enum Field
      {
        // stream index

        INDEX("index")
        {
          public Object getValue(StreamPanel streamPanel)
          {
            return isStreamGood(streamPanel)
              ? streamPanel.mStream.getIndex()
              : null;
          }
        },

          // stream id
          
          ID("id")
          {
            public Object getValue(StreamPanel streamPanel)
            {
              return isStreamGood(streamPanel)
                ? streamPanel.mStream.getId()
                : null;
            }
          },

          // stream type

          TYPE("type")
          {
            public Object getValue(StreamPanel streamPanel)
            {
              return isStreamGood(streamPanel)
                ? streamPanel.mStream.getStreamCoder().getCodecType()
                : null;
            }
          },

          // stream type

          NAME("name")
          {
            public Object getValue(StreamPanel streamPanel)
            {
              return isStreamGood(streamPanel)
                ? streamPanel.mStream.getStreamCoder().getCodec().getLongName()
                : null;
            }
          },

          // the stream direction

          DIRECTION("direction")
          {
            public Object getValue(StreamPanel streamPanel)
            {
              return isStreamGood(streamPanel)
                ? streamPanel.mStream.getDirection()
                : null;
            }
          },

          // the stream time

          TIME("time")
          {
            public Object getValue(StreamPanel streamPanel)
            {
              IMediaData data = null;
              synchronized(streamPanel.mMediaLock)
              {
                if (streamPanel.mMediaData != null)
                  data = streamPanel.mMediaData.copyReference();
              }
              String retval = "";
              if (data != null) {
                try {
                  long delta = (streamPanel.mFrame.mViewer.getMode().isRealTime())
                  ? MILLISECONDS.convert( data.getTimeStamp() - 
                      streamPanel.mFrame.mViewer.getMediaTime(), TIME_UNIT)
                      : 0;

                  retval = data.getFormattedTimeStamp() +
                  (delta <= 0 ? " + " : " - ") + Math.abs(delta);

                } finally {
                  data.delete();
                }
              }
              return retval;
            }
          };
        
          // field lable
          
        private final String mLabel;
        
        // last value returned

        private Object mLastValue;

        // construct a field

        Field(String label)
        {
          mLabel = label + " ";
          mLastValue = "-";
        }
        
        private static boolean isStreamGood(StreamPanel streamPanel)
        {
          if (null == streamPanel.mStream)
            return false;
          if (null == streamPanel.mStream.getStreamCoder())
            return false;
          if (!streamPanel.mStream.getStreamCoder().isOpen())
            return false;
          return true;
        }

        public String getLabel()
        {
          return mLabel;
        }
        
        public Object getCell(int col, StreamPanel streamPanel)
        {
          if (0 == col)
            return getLabel();

          Object value = getValue(streamPanel);
          
          return null != value
            ?  mLastValue = value
            : mLastValue;
        }
        
        abstract public Object getValue(StreamPanel streamPanel);
      };

      // construct the panel

      public StreamPanel(IStream stream, StatsFrame frame)
      {
        // get the stream and frame

        mStream = stream.copyReference();
        mFrame = frame;
        mMediaLock = new Object();
        mMediaData = null;
        // set background based in stream index

        setBackground(mColors[mStream.getIndex() % mColors.length]);

        // create the table model

        mTableModel = new AbstractTableModel() 
          {
            public static final long serialVersionUID = 0;

            public int getColumnCount() { return 2; }
            public int getRowCount() { return Field.values().length;}
            public Object getValueAt(int row, int col)
            {
              return Field.values()[row].getCell(col, StreamPanel.this);
            }
          };
        
        // create and add the table

        mTable = new JTable(mTableModel);
        add(mTable);

        // create the table cell renderer

        TableCellRenderer tableCellRenderer = new TableCellRenderer()
          {
            public static final long serialVersionUID = 0;

            private final JLabel mLabel = new JLabel();
            private final JLabel mValue = new JLabel();
            
            private final int[] colWidths = {0, 0};

            {
              mLabel.setVerticalAlignment(JLabel.TOP);
              mLabel.setHorizontalAlignment(JLabel.RIGHT);
              mLabel.setForeground(new Color(128, 128, 128));
              mLabel.setFont(mLabel.getFont().deriveFont(FONT_SIZE * 0.66f));
              mLabel.doLayout();

              mValue.setHorizontalAlignment(JLabel.LEFT);
              mValue.setForeground(new Color(32, 32, 32));
              mValue.setFont(mValue.getFont().deriveFont(FONT_SIZE));
              mLabel.doLayout();
            }
            
            public Component getTableCellRendererComponent(JTable table, Object value,
              boolean isSelected, boolean hasFocus, int row, int col) 
            {
              JLabel cell = col == 0 ? mLabel : mValue;
              cell.setText(null != value ? value.toString() : "NULL");
              
              Dimension cellSize = cell.getPreferredSize();
              
              if (cellSize.getHeight() > mTable.getRowHeight(row))
              {
                mTable.setRowHeight(row, (int)cellSize.getHeight());
                mFrame.adjustSize();
              }
              
              if (cellSize.getWidth() > colWidths[col])
              {
                colWidths[col] = (int)(cellSize.getWidth() * 1.1);
                mTable.getColumnModel().getColumn(col)
                  .setPreferredWidth(colWidths[col]);
                mFrame.adjustSize();
              }

              return cell;
            }
          };

        // configure to use new renderer

        for (TableColumn column: Collections.list(mTable.getColumnModel().getColumns()))
          column.setCellRenderer(tableCellRenderer);
      }

      // update the stream panel with new media data

      public void update(IMediaData mediaData)
      {
        synchronized(mMediaLock) {
          IMediaData oldMedia = mMediaData;
          mMediaData = mediaData.copyReference();
          if (oldMedia != null)
            oldMedia.delete();
        }
        repaint();
      }
    }
  }
}

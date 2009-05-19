/*
 * Copyright (c) 2008-2009 by Xuggle Inc. All rights reserved.
 * 
 * It is REQUESTED BUT NOT REQUIRED if you use this library, that you let us
 * know by sending e-mail to info@xuggle.com telling us briefly how you're using
 * the library and what you like or don't like about it.
 * 
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2.1 of the License, or (at your option)
 * any later version.
 * 
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

package com.xuggle.xuggler.mediatool;

import java.lang.Thread;

import java.util.Map;
import java.util.HashMap;
import java.util.Formatter;
import java.util.LinkedList;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicLong;
import java.util.concurrent.locks.ReentrantLock;
import java.util.concurrent.locks.Condition;

import java.awt.Color;
import java.awt.Graphics;
import java.awt.Graphics2D;
import java.awt.Dimension;
import java.awt.RenderingHints;
import java.awt.geom.Rectangle2D;
import java.awt.image.BufferedImage;

import javax.sound.sampled.DataLine;
import javax.sound.sampled.AudioFormat;
import javax.sound.sampled.AudioSystem;
import javax.sound.sampled.SourceDataLine;
import javax.sound.sampled.LineUnavailableException;

import javax.swing.JPanel;
import javax.swing.JFrame;

import com.xuggle.xuggler.Global;
import com.xuggle.xuggler.ICodec;
import com.xuggle.xuggler.IStream;
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
 * Add this ass a listener to a media tool to monitor the media. Optionally
 * overlay a running clock onto the video.
 */

public class MediaViewer extends MediaAdapter
{
  private static final Logger log = LoggerFactory.getLogger(MediaViewer.class);

  // the standard time unit used in the media viewer

  public static final TimeUnit TIME_UNIT = MICROSECONDS;

  /** default video early time window, before which video is delayed */

  public static final long DEFALUT_VIDEO_EARLY_WINDOW = TIME_UNIT.convert(50,
      MILLISECONDS);

  /** default video late time window, after which video is dropped */

  public static final long DEFALUT_VIDEO_LATE_WINDOW = TIME_UNIT.convert(50,
      MILLISECONDS);

  /** default audio early time window, before which audio is delayed */

  public static final long DEFALUT_AUDIO_EARLY_WINDOW = 
    TIME_UNIT.convert(50, MILLISECONDS);

  /** default audio late time window, after which audio is dropped */

  public static final long DEFALUT_AUDIO_LATE_WINDOW =
    TIME_UNIT.convert(50, MILLISECONDS);

  // video converters

  private final Map<Integer, IConverter> mConverters = new HashMap<Integer, IConverter>();

  // video frames

  private final Map<Integer, MediaFrame> mFrames = new HashMap<Integer, MediaFrame>();

  // video queues

  private final Map<Integer, VideoQueue> mVideoQueues = new HashMap<Integer, VideoQueue>();

  // audio queues

  private final Map<Integer, AudioQueue> mAudioQueues = new HashMap<Integer, AudioQueue>();

  // audio lines
  private final Map<Integer, SourceDataLine> mAudioLines = new HashMap<Integer, SourceDataLine>();

  // video position index

  private final Map<MediaFrame, Integer> mFrameIndex = new HashMap<MediaFrame, Integer>();

  // next frame index

  private int mNextFrameIndex = 0;

  // the capacity (in time) of media buffers

  private long mBuffersCapacity = TIME_UNIT.convert(3000, MILLISECONDS);

  // show or hide media statistics

  private boolean mShowStats;

  // show media in real time

  private boolean mRealtime;

  // play audio

  private boolean mPlayAudio = false;

  // show video

  private boolean mShowVideo = false;

  // default behavior of windows on close

  private final int mDefaultCloseOperation;

  private final AtomicLong mStartClockTime = new AtomicLong(Global.NO_PTS);

  private final AtomicLong mStartContainerTime = new AtomicLong(Global.NO_PTS);

  private final AtomicLong mAudioLantency = new AtomicLong(0);

  /**
   * Construct a media viewer.
   */

  public MediaViewer()
  {
    this(false);
  }

  /**
   * Construct a media viewer, optionally show basic media statistics statistics
   * overlaid on the video.
   * 
   * @param showStats
   *          display media statistics
   */

  public MediaViewer(boolean showStats)
  {
    this(false, showStats, JFrame.DISPOSE_ON_CLOSE);
  }

  /**
   * Construct a media viewer, optionally show basic media statistics statistics
   * overlaid on the video. The close behavior or the window can be specified.
   * 
   * @param showStats
   *          display media statistics
   * @param defaultCloseOperation
   *          what should Swing do if the window is closed. See the
   *          {@link javax.swing.WindowConstants} documentation for valid
   *          values.
   */

  public MediaViewer(boolean showStats, int defaultCloseOperation)
  {
    this(false, showStats, defaultCloseOperation);
  }

  /**
   * Construct a media viewer, optionally will play audio, show basic media
   * statistics statistics overlaid on the video. The close behavior or the
   * window can be specified.
   * 
   * @param playAudio
   *          play audio, if present in media
   * @param showStats
   *          display media statistics
   * @param defaultCloseOperation
   *          what should Swing do if the window is closed. See the
   *          {@link javax.swing.WindowConstants} documentation for valid
   *          values.
   */

  public MediaViewer(boolean playAudio, boolean showStats,
      int defaultCloseOperation)
  {
    mPlayAudio = true; // playAudio;
    mShowVideo = true; // playAudio;
    mShowStats = showStats;
    mDefaultCloseOperation = defaultCloseOperation;
    mRealtime = true; // true; //mPlayAudio;
  }

  /** Configure internal parameters of the media viewer. */

  @Override
  public void onAddStream(IMediaTool tool, IStream stream)
  {
    log.debug("onAddStream: {}", stream);

    IStreamCoder coder = stream.getStreamCoder();
    int streamIndex = stream.getIndex();
    if (coder.getCodecType() == ICodec.Type.CODEC_TYPE_VIDEO)
    {
      IConverter converter = mConverters.get(streamIndex);
      if (null == converter)
      {
        converter = ConverterFactory.createConverter(
            ConverterFactory.XUGGLER_BGR_24, coder.getPixelType(), coder
                .getWidth(), coder.getHeight());
        mConverters.put(streamIndex, converter);
      }
      MediaFrame frame = mFrames.get(streamIndex);
      if (null == frame)
      {
        frame = new MediaFrame(streamIndex, null);
        mFrames.put(streamIndex, frame);
        mFrameIndex.put(frame, mNextFrameIndex++);
        frame.setLocation(coder.getWidth() * mFrameIndex.get(frame),
            (int) frame.getLocation().getY());
        frame.setDefaultCloseOperation(mDefaultCloseOperation);
      }
      if (mRealtime)
      {
        getVideoQueue(streamIndex, frame);
      }
    }
    else if (coder.getCodecType() == ICodec.Type.CODEC_TYPE_AUDIO)
    {
      if (mRealtime)
      {
        getAudioQueue(tool, streamIndex);
      }
    }

  }

  /** {@inheritDoc} */

  @Override
  public void onVideoPicture(IMediaTool tool, IVideoPicture picture,
      BufferedImage image, int streamIndex)
  {
    // if not supposed to play audio, don't

    if (!mShowVideo)
      return;

    // debug("picture = " + picture);

    // if no BufferedImage is passed in, do the conversion to create one

    if (null == image)
    {
      IConverter converter = mConverters.get(streamIndex);
      image = converter.toImage(picture);
    }

    // if should show stats, add them to the image

    if (mShowStats)
      drawStats(picture, image);

    // get the frame

    MediaFrame frame = mFrames.get(streamIndex);

    // if in real time, queue the video frame for viewing

    if (mRealtime)
    {
      VideoQueue queue = getVideoQueue(streamIndex, frame);

      // enqueue the image
      if (queue != null)
        queue.offerMedia(image, picture.getTimeStamp() + mAudioLantency.get(),
            MICROSECONDS);
    }

    // otherwise just set the image on the frame

    else
      frame.setVideoImage(image);
  }

  /**
   * @param streamIndex
   * @param frame
   * @return
   */

  private VideoQueue getVideoQueue(int streamIndex, MediaFrame frame)
  {
    VideoQueue queue = mVideoQueues.get(streamIndex);
    if (null == queue)
    {
      // create the queue

      queue = new VideoQueue(mBuffersCapacity, TIME_UNIT, frame);
      mVideoQueues.put(streamIndex, queue);
    }
    return queue;
  }

  /** {@inheritDoc} */

  @Override
  public void onAudioSamples(IMediaTool tool, IAudioSamples samples,
      int streamIndex)
  {
    // if not supposed to play audio, don't

    if (!mPlayAudio)
      return;

    // debug("samples = " + samples);

    if (mRealtime)
    {

      // queue the audio frame for playing

      AudioQueue queue = getAudioQueue(tool, streamIndex);
      // enqueue the audio samples

      if (queue != null)
        queue.offerMedia(samples, samples.getTimeStamp(), MICROSECONDS);
    }
    else
    {
      SourceDataLine line = getJavaSoundLine(tool.getContainer().getStream(
          streamIndex));

      if (line != null)
      {
        int size = samples.getSize();
        line.write(samples.getData().getByteArray(0, size), 0, size);
      }
    }
  }

  /**
   * @param tool
   * @param streamIndex
   * @return
   */

  private AudioQueue getAudioQueue(IMediaTool tool, int streamIndex)
  {
    AudioQueue queue = mAudioQueues.get(streamIndex);
    SourceDataLine line = getJavaSoundLine(tool.getContainer().getStream(
        streamIndex));
    if (null == queue && line != null)
    {
      // if the audio line is closed, open it

      // create the queue and add it to the list

      queue = new AudioQueue(mBuffersCapacity, TIME_UNIT, line);
      mAudioQueues.put(streamIndex, queue);
    }
    return queue;
  }

  /**
   * Flush all media buffers.
   */

  public void flush()
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
   * {@inheritDoc} Closes any open windows on screen.
   */

  @Override
  public void onClose(IMediaTool tool)
  {
    flush();
    for (MediaFrame frame : mFrames.values())
      frame.dispose();
    for (SourceDataLine line : mAudioLines.values())
    {
      line.close();
    }
  };

  /**
   * Show the video time on the video.
   * 
   * @param picture
   *          the video picture from which to extract the time stamp
   * @param image
   *          the image on which to draw the time stamp
   */

  private static void drawStats(IVideoPicture picture, BufferedImage image)
  {
    if (image == null)
      throw new RuntimeException("must be used with a IMediaTool"
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
   * @param stream
   *          the stream we'll be decoding in to this line.
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

        int bytesPerSample = (int) IAudioSamples.findSampleBitDepth(audioCoder
            .getSampleFormat())
            * audioCoder.getChannels() / 8;
        line.open(audioFormat);
        line.start();
        mAudioLines.put(streamIndex, line);
        mAudioLantency.compareAndSet(0, IAudioSamples.samplesToDefaultPts(line
            .getBufferSize()
            / bytesPerSample, audioCoder.getSampleRate()));
        log.debug("Opened line with {} bytes ({} microseconds latency)", line
            .getBufferSize(), mAudioLantency.get());
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

  public class AudioQueue extends SelfServicingMediaQueue<IAudioSamples>
  {
    // removes the warning

    public static final long serialVersionUID = 0;

    // the audio line

    private final SourceDataLine mLine;

    /**
     * Construct queue and activate it's internal thread.
     * 
     * @param capacity
     *          the total duraiton of media stored in the queue
     * @param unit
     *          the time unit of the capacity (MILLISECONDS, MICROSECONDS, etc).
     * @param sourceDataLine
     *          the swing frame on which samples are displayed
     */

    public AudioQueue(long capacity, TimeUnit unit,
        SourceDataLine sourceDataLine)
    {
      super(TIME_UNIT.convert(capacity, unit), DEFALUT_AUDIO_EARLY_WINDOW,
          DEFALUT_AUDIO_LATE_WINDOW, TIME_UNIT, Thread.MIN_PRIORITY, "audio");
      mLine = sourceDataLine;
    }

    /** {@inheritDoc} */

    public void dispatch(IAudioSamples samples, long timeStamp)
    {
      int size = samples.getSize();
      mLine.write(samples.getData().getByteArray(0, size), 0, size);
    }
  }

  /**
   * A queue of video images which automatically displays video frames at the
   * correct time.
   */

  public class VideoQueue extends SelfServicingMediaQueue<BufferedImage>
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
      super(TIME_UNIT.convert(capacity, unit), DEFALUT_VIDEO_EARLY_WINDOW,
          DEFALUT_VIDEO_LATE_WINDOW, TIME_UNIT, Thread.MIN_PRIORITY, "video");
      mMediaFrame = mediaFrame;
    }

    /** {@inheritDoc} */

    public void dispatch(BufferedImage image, long timeStamp)
    {
      mMediaFrame.setVideoImage(image);
    }
  }

  /**
   * When created, this queue start a thread which extracts media frames in a
   * timely way and presents them to the analog hole (viewer).
   */

  abstract class SelfServicingMediaQueue<Item> extends
      LinkedList<DelayedItem<Item>>
  {
    /**
     * to make warning go away
     */

    private static final long serialVersionUID = 1L;

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
          DelayedItem<Item> delayedItem = null;

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

              while (!mDone && (delayedItem = poll()) == null)
              {
                try
                {
                  mCondition.await();
                }
                catch (InterruptedException e)
                {
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
              do
              {
                // this is the story of goldilocks testing the the media

                long now = MICROSECONDS.convert(System.nanoTime(), NANOSECONDS);
                mStartClockTime.compareAndSet(Global.NO_PTS, now);
                mStartContainerTime.compareAndSet(Global.NO_PTS, delayedItem
                    .getTimeStamp());
                long clockTimeFromStart = now - mStartClockTime.get();
                long streamTimeFromStart = delayedItem.getTimeStamp()
                    - mStartContainerTime.get();

                long delta = streamTimeFromStart - clockTimeFromStart;

                // if the media is too new and unripe, goldilocks sleeps for a
                // bit

                if (delta >= mEarlyWindow)
                {
                  // debug("delta: " + delta);
                  try
                  {
                    sleep(MILLISECONDS.convert(delta - mEarlyWindow, TIME_UNIT));
                  }
                  catch (InterruptedException e)
                  {
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
                        "%5d DROP [%2d]: %s[%5d] delta: %d",
                        MILLISECONDS.convert(now, TIME_UNIT),
                        size(),
                        (delayedItem.getItem() instanceof BufferedImage ? "IMAGE"
                            : "sound"), MILLISECONDS.convert(delayedItem
                            .getTimeStamp(), TIME_UNIT), MILLISECONDS.convert(
                            delta, TIME_UNIT));
                  }

                  // if the media is just right, goldilocks dispaches it
                  // for presentiation becuse she's a badass bitch

                  else
                  {
                    dispatch(delayedItem.getItem(), delayedItem.getTimeStamp());
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
              while (true);
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
        while (!mDone && !isEmpty())
        {
          try
          {
            mCondition.await();
          }
          catch (InterruptedException e)
          {
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

    public abstract void dispatch(Item item, long timeStamp);

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

    public void offerMedia(Item item, long timeStamp, TimeUnit unit)
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

        while (!mDone && !isEmpty()
            && (convertedTime - peek().getTimeStamp()) > mCapacity)
        {
          try
          {
            // log.debug("Reader blocking; ts:{}; media:{}",
            // timeStamp, item);
            mCondition.await();
          }
          catch (InterruptedException e)
          {
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

          super.offer(new DelayedItem<Item>(item, convertedTime));

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

  class DelayedItem<Item>
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

  /** A media viewer frame. */

  protected class MediaFrame extends JFrame
  {
    // removes the warning

    public static final long serialVersionUID = 0;

    // the video image

    private BufferedImage mImage;

    // the video panel

    private final JPanel mVideoPanel;

    /**
     * Construct a media frame.
     * 
     * @param showStats
     *          display media statistics on the screen
     */

    public MediaFrame(int streamIndex, BufferedImage image)
    {
      // set title based on index

      setTitle("stream " + streamIndex);

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

      // set the initial video image

      setVideoImage(image);

      // show the frame

      setVisible(true);
    }

    // configure the size of a the video panel

    protected void setVideoSize(Dimension videoSize)
    {
      mVideoPanel.setPreferredSize(videoSize);
      invalidate();
      pack();
    }

    // set the video image

    protected void setVideoImage(BufferedImage image)
    {
      mImage = image;
      if (image != null)
      {
        if (mVideoPanel.getWidth() != mImage.getWidth()
            || mVideoPanel.getHeight() != mImage.getHeight())
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

}

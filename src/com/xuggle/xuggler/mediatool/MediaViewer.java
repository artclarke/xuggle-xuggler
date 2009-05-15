/*
 * Copyright (c) 2008-2009 by Xuggle Inc. All rights reserved.
 *
 * It is REQUESTED BUT NOT REQUIRED if you use this library, that you let 
 * us know by sending e-mail to info@xuggle.com telling us briefly how you're
 * using the library and what you like or don't like about it.
 *
 * This library is free software; you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any later
 * version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along
 * with this library; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

package com.xuggle.xuggler.mediatool;

import java.util.Map;
import java.util.HashMap;

import java.awt.Color;
import java.awt.Graphics;
import java.awt.Graphics2D;
import java.awt.Dimension;
import java.awt.RenderingHints;
import java.awt.geom.Rectangle2D;
import java.awt.image.BufferedImage;

import javax.swing.JPanel;
import javax.swing.JFrame;

import com.xuggle.xuggler.IVideoPicture;
import com.xuggle.xuggler.video.IConverter;
import com.xuggle.xuggler.video.ConverterFactory;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Add this to a media reader to display video. Optionally display some basic
 * statistics overlaid on the screen (currently just a running clock).
 * 
 * <p>
 * 
 * TODO:
 * 
 * </p>
 * <ul>
 * 
 * <li>sound support</li>
 * 
 * <li>add option to show media in real time</li>
 * 
 * </ul>
 * 
 */

public class MediaViewer extends MediaAdapter
{
  private final Logger log = LoggerFactory.getLogger(this.getClass());
  
  // video converters

  Map<Integer, IConverter> mConverters = new HashMap<Integer, IConverter>();

  // video frames

  Map<Integer, MediaFrame> mFrames = new HashMap<Integer, MediaFrame>();

  // video position index

  Map<MediaFrame, Integer> mFrameIndex = new HashMap<MediaFrame, Integer>();

  // next frame index

  private int mNextFrameIndex = 0;

  // show or hide media statistics

  private boolean mShowStats;
  
  /**
   * Construct a media viewer.
   */

  public MediaViewer()
  {
    this(false);
  }
    
  /**
   * Construct a media viewer, optionally with some basic movie
   * statistics overlayed on teh video
   *
   * @param showStats display media statistics
   */

  public MediaViewer(boolean showStats)
  {
    mShowStats = showStats;
  }

  /** {@inheritDoc} */
  
  public void onVideoPicture(IMediaTool tool, IVideoPicture picture, 
    BufferedImage image, int streamIndex)
  {
    log.trace("picture: {}", picture);

    // if no BufferedImage is passed in, do the conversion to create one

    if (null == image)
    {
      IConverter converter = mConverters.get(streamIndex);
      if (null == converter)
      {
        converter = ConverterFactory.createConverter(
          ConverterFactory.XUGGLER_BGR_24, picture);
        mConverters.put(streamIndex, converter);
      }
      image = converter.toImage(picture);
    }

    // if should show stats, add them to the image

    if (mShowStats)
      drawStats(picture, image);

    // get the frame

    MediaFrame frame = mFrames.get(streamIndex);
    if (null == frame)
    {
      frame = new MediaFrame(streamIndex, image);
      mFrames.put(streamIndex, frame);
      mFrameIndex.put(frame, mNextFrameIndex++);
      frame.setLocation(
        image.getWidth() * mFrameIndex.get(frame),
        (int)frame.getLocation().getY());
    }
    
    // set the image on the frame

    frame.setVideoImage(image);
  }

  /**
   * Show the video time on the video.
   *
   * @param picture the video picture from which to extract the time stamp
   * @param image the image on which to draw the time stamp
   */

  static void drawStats(IVideoPicture picture, 
    BufferedImage image)
  {
    if (image == null)
      throw new RuntimeException("must be used with a IMediaTool"+
          " that created BuffedImages");
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
     * @param showStats display media statistics on the screen
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
            paintPanel((Graphics2D)graphics);
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

      if (mVideoPanel.getWidth() != mImage.getWidth() || 
        mVideoPanel.getHeight() != mImage.getHeight())
      {
        setVideoSize(new Dimension(mImage.getWidth(), mImage.getHeight()));
      }
      repaint();
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

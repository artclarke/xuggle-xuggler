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

package com.xuggle.xuggler;

import java.awt.Graphics;
import java.awt.Dimension;
import java.awt.Graphics2D;
import java.awt.image.BufferedImage;

import javax.swing.JFrame;
import javax.swing.JPanel;

/** 
 * Addd this to a media reader to display video.
 *
 * TODO:
 *   - sound suport
 *   - add option to show media in real time
 */

public class MediaViewer extends JFrame implements MediaReader.IListener
{
  public static final long serialVersionUID = 0;
  
  // the video image
  
  private BufferedImage mImage;
  
  /**
   * Construct a media viewer.
   */

  public MediaViewer()
  {
    constructFrame();
  }
    
  /** Construct the frame. */

  protected void constructFrame()
  {
    // the panel which shows the video image
  
    JPanel videoPanel = new JPanel()
    {
      public static final long serialVersionUID = 0;
      
      /** {@inheritDoc} */
      
      public void paint(Graphics graphics)
      {
        // call parent paint

        super.paint(graphics);
        
        // if there exists an image

        if (mImage != null)
        {
          // resize the frame to fit the media

          if (getWidth() != mImage.getWidth() ||
            getHeight() != mImage.getHeight())
          {
            setPreferredSize(new Dimension(
                mImage.getWidth(), mImage.getHeight()));
            MediaViewer.this.invalidate();
            MediaViewer.this.pack();
          }

          // drae the image into the rame

          graphics.drawImage(mImage, 0, 0, null);
        }
      }
    };

    // add the videoPanel 

    getContentPane().add(videoPanel);

    // pack and show the frame

    pack();
    setVisible(true);
  }

  /** {@inheritDoc} */
  
  public void onVideoPicture(IVideoPicture picture, BufferedImage image, 
    int streamIndex)
  {
    mImage = image;
    repaint();
  }

  /** {@inheritDoc} */
  
  public void onAudioSamples(IAudioSamples samples, int streamIndex)
  {
  }
  
  /** {@inheritDoc} */
  
  public void onOpen(MediaReader source)
  {
  }
  
  /** {@inheritDoc} */
  
  public void onClose(MediaReader source)
  {
  }
}

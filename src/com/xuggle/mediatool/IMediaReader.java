/*******************************************************************************
 * Copyright (c) 2008, 2010 Xuggle Inc.  All rights reserved.
 *  
 * This file is part of Xuggle-Xuggler-Main.
 *
 * Xuggle-Xuggler-Main is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Xuggle-Xuggler-Main is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Xuggle-Xuggler-Main.  If not, see <http://www.gnu.org/licenses/>.
 *******************************************************************************/

package com.xuggle.mediatool;

import java.awt.image.BufferedImage;

import com.xuggle.mediatool.event.IVideoPictureEvent;
import com.xuggle.xuggler.IAudioSamples;
import com.xuggle.xuggler.IContainer;
import com.xuggle.xuggler.IContainerFormat;
import com.xuggle.xuggler.IError;
import com.xuggle.xuggler.IVideoPicture;

/**
 * An {@link IMediaCoder} that reads and decodes media from an
 * {@link IContainer}.
 * 
 * <p>
 * An {@link IMediaReader} opens up a media container,
 * reads packets from it, decodes the data, and then dispatches
 * information about the data to any registered
 * {@link IMediaListener} objects.  The main method of
 * interest is {@link #readPacket()}.
 * 
 * </p>
 * <p>
 * 
 * Here's an example of a very simple program that prints out
 * a line when the {@link IMediaReader} decides to open a container.
 * 
 *  </p>
 *  <pre>
 *  IMediaDebugListener myListener = new MediaListenerAdapter(){
 *    public void onOpen(IMediaGenerator pipe) {
 *      System.out.println("opened: " + ((IMediaReader)pipe).getUrl());
 *    }
 *  };
 *  IMediaReader reader = ToolFactory.makeReader("myinputfile.flv");
 *  reader.addListener(myListener);
 *  while(reader.readPacket() == null)
 *    ;
 *  </pre>
 *  <p>
 *  
 *  And here's a slightly more involved example where we read
 *  a file and display it on screen in real-time:
 *  
 *  </p>
 *  <pre>
 *  IMediaReader reader = ToolFactory.makeReader("myinputfile.flv");
 *  reader.addListener(ToolFactory.makeViewer());
 *  while(reader.readPacket() == null)
 *    ;
 *  </pre>
 *  <p>
 *  For examples of this class in action, see the
 *  com.xuggle.mediatool.demos package.
 *  </p>
 * @author trebor
 * @author aclarke
 *
 */

public interface IMediaReader extends IMediaCoder
{

  /**
   * Set if the underlying media container supports adding dynamic streams. See
   * {@link IContainer#open(String, IContainer.Type, IContainerFormat, boolean, boolean)}
   * . The default value for this is false.
   * 
   * <p>
   * 
   * If set to false, the {@link IMediaReader} can assume no new streams will be
   * added after {@link #open()} has been called, and may decide to query the
   * entire media file to find all meta data. If true then {@link IMediaReader}
   * will not read ahead; instead it will only query meta data for a stream when
   * a {@link #readPacket()} returns the first packet in a new stream. Note that
   * a {@link IMediaWriter} can only initialize itself from a
   * {@link IMediaReader} that has this parameter set to false.
   * 
   * </p>
   * <p>
   * 
   * To have an effect, the MediaReader must not have been created with an
   * already open {@link IContainer}, and this method must be called before the
   * first call to {@link #readPacket}.
   * 
   * </p>
   * 
   * @param streamsCanBeAddedDynamically true if new streams may appear at any
   *        time during a {@link #readPacket} call
   * 
   * @throws RuntimeException if the media container is already open
   */

  public abstract void setAddDynamicStreams(boolean streamsCanBeAddedDynamically);

  /** 
   * Report if the underlying media container supports adding dynamic
   * streams.  See {@link IContainer#open(String, IContainer.Type,
   * IContainerFormat, boolean, boolean)}.
   * 
   * @return true if new streams can may appear at any time during a
   *         {@link #readPacket} call
   * @see #setAddDynamicStreams(boolean)
   */

  public abstract boolean canAddDynamicStreams();

  /** 
   * Set if the underlying media container will attempt to establish all
   * meta data when the container is opened, which will potentially
   * block until it has ready enough data to find all streams in a
   * container.  If false, it will only block to read a minimal header
   * for this container format.  See {@link IContainer#open(String,
   * IContainer.Type, IContainerFormat, boolean, boolean)}.  The default
   * value for this is true.
   *
   * <p>
   * 
   * To have an effect, the MediaReader must not have been created with
   * an already open {@link IContainer}, and this method must be called
   * before the first call to {@link #readPacket}.
   *
   * </p>
   * 
   * @param queryStreamMetaData true if meta data is to be queried
   * 
   * @throws RuntimeException if the media container is already open
   */

  public abstract void setQueryMetaData(boolean queryStreamMetaData);

  /** 
   * Report if the underlying media container will attempt to establish
   * all meta data when the container is opened, which will potentially
   * block until it has ready enough data to find all streams in a
   * container.  If false, it will only block to read a minimal header
   * for this container format.  See {@link IContainer#open(String,
   * IContainer.Type, IContainerFormat, boolean, boolean)}.
   * 
   * @return true meta data will be queried
   * @see #setQueryMetaData(boolean)
   */

  public abstract boolean willQueryMetaData();

  /**
   * Should {@link IMediaReader} automatically call {@link #close()}, only if
   * ERROR_EOF is returned from {@link #readPacket}. Otherwise {@link #close()}
   * is automatically called when any error value is returned. The default value
   * for this is false.
   * 
   * @param closeOnEofOnly true if meta data is to be queried
   * 
   * @throws RuntimeException if the media container is already open
   */

  public abstract void setCloseOnEofOnly(boolean closeOnEofOnly);

  /** 
   * Report if close will called only if ERROR_EOF is returned from
   * {@link #readPacket}.  Otherwise close is called when any error
   * value is returned.  The default value for this is false.
   * 
   * @return true if will close on ERROR_EOF only
   * @see #setCloseOnEofOnly(boolean)
   */

  public abstract boolean willCloseOnEofOnly();

  /**
   * Decodes the next packet and calls all registered {@link IMediaListener}
   * objects.
   * 
   * <p>
   * 
   * If a complete {@link IVideoPicture} or {@link IAudioSamples} set are
   * decoded, it will be dispatched to the listeners added to the media reader.
   * 
   * </p>
   * 
   * <p>
   * 
   * This method will automatically call {@link #open()} if it has not
   * already been called, and will automatically call {@link #close()} when
   * it reads an error or end of file from the file.  The default
   * close behavior can be changed with {@link #setCloseOnEofOnly(boolean)}.
   * 
   * </p>
   * 
   * @return null if there are more packets to read, otherwise return an IError
   *         instance. If {@link IError#getType()} ==
   *         {@link com.xuggle.xuggler.IError.Type#ERROR_EOF} then end of file
   *         has been reached.
   */

  public abstract IError readPacket();

  /**
   * Asks the {@link IMediaReader} to generate {@link BufferedImage} images when
   * calling
   * {@link IMediaListener#onVideoPicture(IVideoPictureEvent)}
   * .
   * 
   * <p>
   * NOTE: Only {@link BufferedImage#TYPE_3BYTE_BGR} is supported today.
   * </p>
   * 
   * <p>
   * If set to a non-negative value, {@link IMediaReader} will resample any
   * video data it has decoded into the right colorspace for the
   * {@link BufferedImage}, and generate a new {@link BufferedImage} to pass in
   * on each
   * {@link IMediaListener#onVideoPicture(IVideoPictureEvent)
   * }
   * call.
   * </p>
   * 
   * @param bufferedImageType The buffered image type (e.g.
   *        {@link BufferedImage#TYPE_3BYTE_BGR}) you want {@link IMediaReader}
   *        to generate. Set to -1 to disable this feature.
   * 
   * @see BufferedImage
   */

  public abstract void setBufferedImageTypeToGenerate(int bufferedImageType);

  /**
   * Get the {@link BufferedImage} type this {@link IMediaReader} will
   * generate.
   * @return the type, or -1 if disabled.
   * 
   * @see #getBufferedImageTypeToGenerate()
   */

  public abstract int getBufferedImageTypeToGenerate();
  
  /**
   * {@inheritDoc}
   */

  public abstract IContainer getContainer();

  /**
   * {@inheritDoc}
   */

  public abstract String getUrl();

  /**
   * {@inheritDoc}
   */

  public abstract void open();

  /**
   * {@inheritDoc}
   */

  public abstract void close();

}

package com.xuggle.mediatool;

import java.awt.image.BufferedImage;

import com.xuggle.xuggler.IAudioSamples;
import com.xuggle.xuggler.IContainer;
import com.xuggle.xuggler.IContainerFormat;
import com.xuggle.xuggler.IError;
import com.xuggle.xuggler.IVideoPicture;

public interface IMediaReader extends IMediaTool
{

  /**
   * Set if the underlying media container supports adding dynamic
   * streams. See {@link IContainer#open(String, IContainer.Type,
   * IContainerFormat, boolean, boolean)} . The default value for this
   * is false.
   * 
   * <p>
   * 
   * If set to false, Xuggler can assume no new streams will be added
   * after {@link #open()} has been called, and may decide to query the
   * entire media file to find all meta data. If true then Xuggler will
   * not read ahead; instead it will only query meta data for a stream
   * when a {@link #readPacket()} returns the first packet in a new
   * stream. Note that a {@link IMediaWriter} can only initialize itself
   * from a {@link IMediaReader} that has this parameter set to false.
   * 
   * </p>
   * 
   * <p>
   * 
   * To have an effect, the MediaReader must not have been created with
   * an already open {@link IContainer}, and this method must be called
   * before the first call to {@link #readPacket}.
   * 
   * </p>
   * 
   * @param streamsCanBeAddedDynamically
   *          true if new streams may appear at any time during a
   *          {@link #readPacket} call
   * 
   * @throws RuntimeException
   *           if the media container is already open
   */

  public abstract void setAddDynamicStreams(boolean streamsCanBeAddedDynamically);

  /** 
   * Report if the underlying media container supports adding dynamic
   * streams.  See {@link IContainer#open(String, IContainer.Type,
   * IContainerFormat, boolean, boolean)}.
   * 
   * @return true if new streams can may appear at any time during a
   *         {@link #readPacket} call
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
   */

  public abstract boolean willQueryMetaData();

  /** 
   * Set to close, if and only if ERROR_EOF is returned from {@link
   * #readPacket}.  Otherwise close is called when any error value
   * is returned.  The default value for this is false.
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
   */

  public abstract boolean willCloseOnEofOnly();

  /**
   * This decodes the next packet and calls registered {@link IMediaPipeListener}s.
   * 
   * <p>
   * 
   * If a complete {@link IVideoPicture} or {@link IAudioSamples} set are
   * decoded, it will be dispatched to the listeners added to the media reader.
   * 
   * </p>
   * 
   * @return null if there are more packets to read, otherwise return an IError
   *         instance. If {@link com.xuggle.xuggler.IError#getType()} ==
   *         {@link com.xuggle.xuggler.IError.Type#ERROR_EOF} then end of file
   *         has been reached.
   */

  public abstract IError readPacket();

  /**
   * Asks the {@link IMediaReader} to generate {@link BufferedImage}
   * images when calling {@link IMediaPipeListener#onVideoPicture(IMediaPipe, IVideoPicture, BufferedImage, int)}.
   * 
   * <p>
   * NOTE: Only {@link BufferedImage#TYPE_3BYTE_BGR} is supported today.
   * </p>
   * 
   * <p>
   * If set to a non-negative value, {@link IMediaReader} will
   * resample any video data it has decoded into the right colorspace
   * for the {@link BufferedImage}, and generate a new {@link BufferedImage}
   * to pass in on each {@link IMediaPipeListener#onVideoPicture(IMediaPipe, IVideoPicture, BufferedImage, int)
   * } call.
   * </p>
   * 
   * @param bufferedImageType The buffered image type (e.g.
   *   {@link BufferedImage#TYPE_3BYTE_BGR}) you want 
   *   {@link IMediaReader} to generate.  Set to -1 to disable
   *   this feature.
   *   
   * @see BufferedImage
   * @throws RuntimeException if the media container has been opened you
   *   cannot change this setting.
   */

  public abstract int getBufferedImageTypeToGenerate();

  /**
   * Get the {@link BufferedImage} type this {@link IMediaReader} will
   * generate.
   * @return the type, or -1 if disabled.
   * 
   * @see #getBufferedImageTypeToGenerate()
   */

  public abstract void setBufferedImageTypeToGenerate(int bufferedImageType);

}

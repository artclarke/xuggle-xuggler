package com.xuggle.mediatool;

import static java.lang.System.exit;
import static java.lang.System.out;

import java.io.File;

import com.xuggle.ferry.JNIMemoryManager;
import com.xuggle.ferry.RefCounted;
import com.xuggle.xuggler.IContainer;
import com.xuggle.xuggler.ICodec;

/**
 * Creates objects that implement the MediaTool interfaces, and provides global
 * settings for the API.
 * <p>
 * Here's an example of a {@link MediaTool} program that opens up an input media
 * file, transcodes all the media to a new format, while playing the media on
 * your machine:
 * </p>
 * 
 * <pre>
 * IMediaReader reader = MediaTool.makeReader(&quot;input.mpg&quot;);
 * reader.addListener(MediaTool.makeViewer(true));
 * reader.addListener(MediaTool.makeWriter(&quot;output.flv&quot;, reader));
 * while (reader.readPacket() == null)
 *   ;
 * </pre>
 * 
 * @author trebor
 * @author aclarke
 * 
 */
public class MediaTool
{

  /**
   * A private constructor to force only static members in this class.
   */
  private MediaTool()
  {

  }

  /* IMediaReader Factories */

  /**
   * Create an {@link IMediaReader} to reads and dispatches decoded media from a
   * media container for a given source URL.
   * 
   * @param url the location of the media content, a file name will also work
   *        here
   * @return An {@link IMediaReader}
   */

  public static IMediaReader makeReader(String url)
  {
    return new MediaReader(url);
  }

  /**
   * Create an {@link IMediaReader} to reads and dispatches decoded media from a
   * media container for a given source URL.
   * 
   * <p>
   * 
   * Any Xuggler resourced opened by the {@link IMediaReader} will be closed by
   * the {@link IMediaReader}, however resources opened outside the
   * {@link IMediaReader} will not be closed. In short {@link IMediaReader}
   * closes what it opens.
   * 
   * </p>
   * 
   * @param container an already created media container to read data from.
   */

  public static IMediaReader makeReader(IContainer container)
  {
    return new MediaReader(container);
  }

  /* MediaWriter constructors */
  /**
   * Use a specified {@link IMediaReader} as a source for media data and meta
   * data about the container and it's streams. The {@link IMediaReader} must be
   * configured such that streams will not be dynamically added to the
   * container, which is the default for {@link IMediaReader}.
   * 
   * @param url the url or filename of the media destination
   * @param reader the media source
   * 
   * @throws IllegalArgumentException if the specified {@link IMediaReader} is
   *         configure to allow dynamic adding of streams.
   */

  public static IMediaWriter makeWriter(String url, IMediaReader reader)
  {
    return new MediaWriter(url, reader);
  }

  /**
   * Use a specified {@link IContainer} as a source for and meta data about the
   * container and it's streams. The {@link IContainer} must be configured such
   * that streams will not be dynamically added to the container.
   * 
   * @param url the url or filename of the media destination
   * @param inputContainer the source media container which will be queries to
   *        determine what streams and media should be added when writing data.
   * 
   * @throws IllegalArgumentException if the specifed {@link IContainer} is not
   *         a of type READ or is configure to allow dynamic adding of streams.
   */

  public static IMediaWriter makeWriter(String url, IContainer inputContainer)
  {
    return new MediaWriter(url, inputContainer);
  }

  /**
   * Create a MediaWriter which will require subsequent calls to
   * {@link IMediaWriter#addVideoStream(int, int, ICodec, int, int)} and/or
   * {@link IMediaWriter#addAudioStream(int, int, ICodec, int, int)} to
   * configure the writer. Streams may be added or further configured as needed
   * until the first attempt to write data.
   * 
   * @param url the url or filename of the media destination
   */
  public static IMediaWriter makeWriter(String url)
  {
    return new MediaWriter(url);
  }

  /** Media Viewer Factories */
  /**
   * Construct a default media viewer.
   */

  public static IMediaViewer makeViewer()
  {
    return new MediaViewer();
  }

  /**
   * Construct a media viewer which plays in the specified mode.
   * 
   * @param mode the play mode of this viewer
   */

  public static IMediaViewer makeViewer(IMediaViewer.Mode mode)
  {
    return new MediaViewer(mode);
  }

  /**
   * Construct a media viewer and optionally show media statistics.
   * 
   * @param showStats display media statistics
   */

  public static IMediaViewer makeViewer(boolean showStats)
  {
    return new MediaViewer(showStats);
  }

  /**
   * Construct a media viewer which plays in the specified mode and optionally
   * shows media statistics.
   * 
   * @param mode the play mode of this viewer
   * @param showStats display media statistics
   */

  public static IMediaViewer makeViewer(IMediaViewer.Mode mode,
      boolean showStats)
  {
    return new MediaViewer(mode, showStats);
  }

  /**
   * Construct a media viewer, optionally show media statistics and specify the
   * default frame close behavior.
   * 
   * @param showStats display media statistics
   * @param defaultCloseOperation what should Swing do if the window is closed.
   *        See the {@link javax.swing.WindowConstants} documentation for valid
   *        values.
   */

  public static IMediaViewer makeViewer(boolean showStats,
      int defaultCloseOperation)
  {
    return new MediaViewer(showStats, defaultCloseOperation);
  }

  /**
   * Construct a media viewer which plays in the specified mode, optionally
   * shows media statistics and specifies the default frame close behavior.
   * 
   * @param mode the play mode of this viewer
   * @param showStats display media statistics
   * @param defaultCloseOperation what should Swing do if the window is closed.
   *        See the {@link javax.swing.WindowConstants} documentation for valid
   *        values.
   */

  public static IMediaViewer makeViewer(IMediaViewer.Mode mode,
      boolean showStats, int defaultCloseOperation)
  {
    return new MediaViewer(mode, showStats, defaultCloseOperation);
  }

  /** {@link IMediaDebugListener} Factories */

  /**
   * Construct a debug listener which logs all event types.
   */

  public static IMediaDebugListener makeDebugListener()
  {
    return new MediaDebugListener();
  }

  /**
   * Construct a debug listener with custom set of event types to log.
   * 
   * @param events the event types which will be logged
   */

  public static IMediaDebugListener makeDebugListener(
      IMediaDebugListener.Event... events)
  {
    return new MediaDebugListener(events);
  }

  /**
   * Construct a debug listener with custom set of event types to log.
   * 
   * @param mode log mode, see {@link IMediaDebugListener.Mode}
   * @param events the event types which will be logged
   */

  public static IMediaDebugListener makeDebugListener(
      IMediaDebugListener.Mode mode, IMediaDebugListener.Event... events)
  {
    return new MediaDebugListener(mode, events);
  }

  /**
   * A sample program for the {@link MediaTool} that takes an input media file
   * and an output file name, then transcodes the input to the output format
   * while displaying the media in real time.
   * 
   * @param args input filename and output filename
   */

  public static void main(String[] args)
  {
    if (args.length < 2)
    {
      out.println("To perform a simple media transcode.  The destination "
          + "format will be guessed from the file extention.");
      out.println("");
      out.println("   TranscodeAudioAndVideo <source-file> <destination-file>");
      out.println("");
      out
          .println("The destination type will be guess from the supplied file extsion.");
      exit(0);
    }

    File source = new File(args[0]);
    if (!source.exists())
    {
      out.println("Source file does not exist: " + source);
      exit(0);
    }

    IMediaReader reader = MediaTool.makeReader(args[0]);
    reader.addListener(MediaTool.makeViewer(true));
    reader.addListener(MediaTool.makeWriter(args[1], reader));
    while (reader.readPacket() == null)
      ;

  }

  /**
   * Turns on and off Turbo-Charging for the {@link MediaTool} package.
   * <p>
   * Turbo-Charging is off by default.
   * </p>
   * <p>
   * When running in Turbo-Mode {@link MediaTool} will make a variety of tuning
   * assumptions that can speed up execution of your program, sometimes by
   * significant amounts.  {@link MediaTool} was designed from
   * the ground up to run in Turbo-Mode, but it can cause issues
   * for other {@link com.xuggle.xuggler}-based programs running
   * in the same Java process.
   * </p>
   * <p> It is safe to turn on if your program only uses
   * interfaces in the {@link com.xuggle.mediatool} API, and you are not running
   * in the same Java process as other programs using the
   * {@link com.xuggle.xuggler} API.
   * </p>
   * <p>
   * If you turn on Turbo-Charging and then access any of the
   * underlying {@link com.xuggle.xuggler} interfaces (e.g.
   * {@link IMediaTool#getContainer()}) behind MediaTool,
   * you must:
   * <ul>
   * <li>Call
   * {@link RefCounted#delete()} on any references returned on the stack
   * to your methods.</li>
   * <li>Call {@link RefCounted#copyReference()} on any
   * {@link com.xuggle.xuggler} parameters passed to your methods
   * that you need to keep a reference to after your method returns.</li>
   * <li>Ensure you do not call {@link RefCounted#delete()} on
   * parameters passed <strong>into</strong> your methods.  The calling
   * method is managing that reference.</li>
   * </ul>
   * <p>
   * Failure to follow these rules could lead to {@link OutOfMemoryError}
   * errors, or to premature releasing of resources.
   * </p>
   * <p>
   * Turbo mode works by changing the global
   * {@link com.xuggle.ferry.JNIMemoryManager.MemoryModel} that the underlying
   * {@link com.xuggle.xuggler} API in your program is using. If you are using
   * {@link MediaTool} in a java program that contains other code using the
   * {@link com.xuggle.xuggler} API, you will force that code to use the new
   * memory model.
   * </p>
   * 
   * @param turbo should we turn on turbo mode
   * 
   * @see JNIMemoryManager#setMemoryModel(com.xuggle.ferry.JNIMemoryManager.MemoryModel)
   */
  public static void setTurboCharged(boolean turbo)
  {
    if (turbo)
      JNIMemoryManager
          .setMemoryModel(JNIMemoryManager.MemoryModel.NATIVE_BUFFERS);
    else
      JNIMemoryManager
          .setMemoryModel(JNIMemoryManager.MemoryModel.JAVA_STANDARD_HEAP);
  }

  /**
   * Is {@link MediaTool} running in turbo charged mode?
   * @return true if trubo-charged
   */
  public static boolean isTurboCharged()
  {
    return !(JNIMemoryManager.getMemoryModel() == JNIMemoryManager.MemoryModel.JAVA_STANDARD_HEAP);
  }
}

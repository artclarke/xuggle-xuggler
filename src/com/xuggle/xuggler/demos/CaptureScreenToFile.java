package com.xuggle.xuggler.demos;
import java.awt.AWTException;
import java.awt.Rectangle;
import java.awt.Robot;
import java.awt.Toolkit;
import java.awt.image.BufferedImage;

import com.xuggle.xuggler.Global;
import com.xuggle.xuggler.ICodec;
import com.xuggle.xuggler.IContainer;
import com.xuggle.xuggler.IPacket;
import com.xuggle.xuggler.IPixelFormat;
import com.xuggle.xuggler.IRational;
import com.xuggle.xuggler.IStream;
import com.xuggle.xuggler.IStreamCoder;
import com.xuggle.xuggler.IVideoPicture;
import com.xuggle.xuggler.video.ConverterFactory;
import com.xuggle.xuggler.video.IConverter;

/**
 * This demonstration application shows how to use Xuggler and Java to take
 * snapshots of your screen and encode them in a video.
 * 
 * <p>
 * You can find the original code <a href="http://flexingineer.blogspot.com/2009/05/encoding-video-from-series-of-images.html"
 * > here </a>.
 * </p>
 * 
 * @author Denis Zgonjanin
 * @author aclarke
 * 
 */

public class CaptureScreenToFile
{
  private final IContainer outContainer;
  private final IStream outStream;
  private final IStreamCoder outStreamCoder;

  private final IRational frameRate;

  private final Robot robot;
  private final Toolkit toolkit;
  private final Rectangle screenBounds;

  private long timeStamp;

  /**
   * Takes up to one argument, which just gives a file name to encode as. We
   * guess which video codec to use based on the filename.
   * 
   * @param args
   *          The filename to write to.
   */

  public static void main(String[] args)
  {
    String outFile = null;
    if (args.length < 1)
    {
      outFile = "output.mp4";
    }
    else if (args.length == 1)
    {
      outFile = args[0];
    }
    else if (args.length > 1)
    {
      System.err.println("Must pass in an output file name");
      return;
    }
    try
    {
      CaptureScreenToFile videoEncoder = new CaptureScreenToFile(outFile);
      int index = 0;
      while (index < 30)
      {
        System.out.println("encoded image");
        videoEncoder.encodeImage(videoEncoder.takeSingleSnapshot());

        try
        {
          // sleep for framerate milliseconds
          Thread.sleep((long) (1000 / videoEncoder.frameRate.getDouble()));
        }
        catch (InterruptedException e)
        {
          e.printStackTrace(System.out);
        }
        index++;
      }
      videoEncoder.closeStreams();
    }
    catch (RuntimeException e)
    {
      System.err.println("we can't get permission to capture the screen");
    }
  }

  /**
   * Create the demonstration object with lots of defaults. Throws an exception
   * if we can't get the screen or open the file.
   * 
   * @param outFile
   *          File to write to.
   */
  public CaptureScreenToFile(String outFile)
  {
    try
    {
      robot = new Robot();
    }
    catch (AWTException e)
    {
      System.out.println(e.getMessage());
      throw new RuntimeException(e);
    }
    toolkit = Toolkit.getDefaultToolkit();
    screenBounds = new Rectangle(toolkit.getScreenSize());

    // Change this to change the frame rate you record at
    frameRate = IRational.make(3, 1);

    outContainer = IContainer.make();

    int retval = outContainer.open(outFile, IContainer.Type.WRITE, null);
    if (retval < 0)
      throw new RuntimeException("could not open output file");

    outStream = outContainer.addNewStream(0);
    outStreamCoder = outStream.getStreamCoder();

    ICodec codec = ICodec.guessEncodingCodec(null, null, outFile, null,
        ICodec.Type.CODEC_TYPE_VIDEO);
    if (codec == null)
      throw new RuntimeException("could not guess a codec");

    outStreamCoder.setNumPicturesInGroupOfPictures(30);
    outStreamCoder.setCodec(codec);

    outStreamCoder.setBitRate(25000);
    outStreamCoder.setBitRateTolerance(9000);

    int width = toolkit.getScreenSize().width;
    int height = toolkit.getScreenSize().height;

    outStreamCoder.setPixelType(IPixelFormat.Type.YUV420P);
    outStreamCoder.setHeight(height);
    outStreamCoder.setWidth(width);
    outStreamCoder.setFlag(IStreamCoder.Flags.FLAG_QSCALE, true);
    outStreamCoder.setGlobalQuality(0);

    outStreamCoder.setFrameRate(frameRate);
    outStreamCoder.setTimeBase(IRational.make(frameRate.getDenominator(),
        frameRate.getNumerator()));

    retval = outStreamCoder.open();
    if (retval < 0)
      throw new RuntimeException("could not open input decoder");
    retval = outContainer.writeHeader();
    if (retval < 0)
      throw new RuntimeException("could not write file header");
    timeStamp = 0;
  }

  /**
   * Encode the given image to the file and increment our time stamp.
   * 
   * @param originalImage
   *          an image of the screen.
   */

  public void encodeImage(BufferedImage originalImage)
  {
    BufferedImage worksWithXugglerBufferedImage = convertToType(originalImage,
        BufferedImage.TYPE_3BYTE_BGR);
    IPacket packet = IPacket.make();

    IConverter converter = null;
    try
    {
      converter = ConverterFactory.createConverter(
          worksWithXugglerBufferedImage, IPixelFormat.Type.YUV420P);
    }
    catch (UnsupportedOperationException e)
    {
      System.out.println(e.getMessage());
      e.printStackTrace(System.out);
    }

    IVideoPicture outFrame = converter.toPicture(worksWithXugglerBufferedImage,
        timeStamp);
    // increment timestamp by framerate in microseconds
    timeStamp += Global.DEFAULT_PTS_PER_SECOND / frameRate.getDouble();

    outFrame.setQuality(0);
    int retval = outStreamCoder.encodeVideo(packet, outFrame, 0);
    if (retval < 0)
      throw new RuntimeException("could not encode video");
    if (packet.isComplete())
    {
      retval = outContainer.writePacket(packet);
      if (retval < 0)
        throw new RuntimeException("could not save packet to container");
    }

  }

  /**
   * Close out the file we're currently working on.
   */

  public void closeStreams()
  {
    int retval = outContainer.writeTrailer();
    if (retval < 0)
      throw new RuntimeException("Could not write trailer to output file");
  }

  /**
   * Use the AWT robot to take a snapshot of the current screen.
   * 
   * @return a picture of the desktop
   */

  public BufferedImage takeSingleSnapshot()
  {
    return robot.createScreenCapture(this.screenBounds);
  }

  /**
   * Convert a {@link BufferedImage} of any type, to {@link BufferedImage} of a
   * specified type. If the source image is the same type as the target type,
   * then original image is returned, otherwise new image of the correct type is
   * created and the content of the source image is copied into the new image.
   * 
   * @param sourceImage
   *          the image to be converted
   * @param targetType
   *          the desired BufferedImage type
   * 
   * @return a BufferedImage of the specifed target type.
   * 
   * @see BufferedImage
   */

  public static BufferedImage convertToType(BufferedImage sourceImage,
      int targetType)
  {
    BufferedImage image;

    // if the source image is already the target type, return the source image

    if (sourceImage.getType() == targetType)
      image = sourceImage;

    // otherwise create a new image of the target type and draw the new
    // image

    else
    {
      image = new BufferedImage(sourceImage.getWidth(),
          sourceImage.getHeight(), targetType);
      image.getGraphics().drawImage(sourceImage, 0, 0, null);
    }

    return image;
  }

}

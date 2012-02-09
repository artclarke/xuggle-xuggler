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

package com.xuggle.xuggler;

import java.util.List;

import org.apache.commons.cli.*;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.xuggle.xuggler.Global;
import com.xuggle.xuggler.IAudioResampler;
import com.xuggle.xuggler.IAudioSamples;
import com.xuggle.xuggler.IAudioSamples.Format;
import com.xuggle.xuggler.ICodec;
import com.xuggle.xuggler.IContainer;
import com.xuggle.xuggler.IContainerFormat;
import com.xuggle.xuggler.IVideoPicture;
import com.xuggle.xuggler.IPacket;
import com.xuggle.xuggler.IRational;
import com.xuggle.xuggler.IStream;
import com.xuggle.xuggler.IStreamCoder;
import com.xuggle.xuggler.IVideoResampler;

import com.xuggle.xuggler.io.URLProtocolManager;

/**
 * An example class that shows how to use the Xuggler library to open, decode,
 * re-sample, encode and write media files.
 * 
 * <p>
 * 
 * This class is called by the {@link Xuggler} class to do all the heavy
 * lifting. It is meant as a Demonstration class and implements a small subset
 * of the functionality that the (much more full-featured) <code>ffmpeg</code>
 * command line tool implements. It is really meant to show people how the
 * Xuggler library is used from java.
 * 
 * </p>
 * <p>
 * 
 * Read <a href="{@docRoot}/src-html/com/xuggle/xuggler/Converter.html">the
 * Converter.java source</a> for a good example of a class exercising this
 * library.
 * 
 * </p>
 * <p>
 * 
 * <strong>It is not our intent to replicate all features in the
 * <code>ffmpeg</code> command line tool in this class.</strong>
 * 
 * </p>
 * <p>
 * 
 * If you are just trying to convert pre-existing media files from one format to
 * another with a batch-processing program we strongly recommend you use the
 * <code>ffmpeg</code> command-line tool to do it. Look, here's even the <a
 * href="http://ffmpeg.org/ffmpeg-doc.html">documentation</a> for that program.
 * 
 * </p>
 * <p>
 * 
 * If on the other hand you need to programatically decide when and how you do
 * video processing, or process only parts of files, or do transcoding live
 * within a Java server without calling out to another process, then by all
 * means use Xuggler and use this class as an example of how to do conversion.
 * But please recognize you will likely need to implement code specific to your
 * application. <strong>This class is no substitute for actually understanding
 * the how to use the Xuggler API within your specific use-case</strong>
 * 
 * </p>
 * <p>
 * 
 * And if you haven't gotten the impression, please stop asking us to support
 * <code>ffmpeg</code> options like "-re" in this class. This class is only
 * meant as a teaching-aide for the underlying Xuggler API.
 * 
 * </p>
 * <p>
 * 
 * Instead implement your own Java class based off of this that does real-time
 * playback yourself. Really. Please. We'd appreciate it very much.
 * 
 * </p>
 * <p>
 * Now, all that said, here's how to create a main class that uses this
 * Converter class:
 * </p>
 * 
 * <pre>
 * public static void main(String[] args)
 * {
 *   Converter converter = new Converter();
 *   try
 *   {
 *     // first define options
 *     Options options = converter.defineOptions();
 *     // And then parse them.
 *     CommandLine cmdLine = converter.parseOptions(options, args);
 *     // Finally, run the converter.
 *     converter.run(cmdLine);
 *   }
 *   catch (Exception exception)
 *   {
 *     System.err.printf(&quot;Error: %s\n&quot;, exception.getMessage());
 *   }
 * }
 * </pre>
 * 
 * <p>
 * 
 * Pass &quot;--help&quot to your main class as the argument to see the list of
 * accepted options.
 * 
 * </p>
 * 
 * @author aclarke
 * 
 */
public class Converter
{
  static
  {
    // this forces the FFMPEG io library to be loaded which means we can bypass
    // FFMPEG's file io if needed
    URLProtocolManager.getManager();
  }

  /**
   * Create a new Converter object.
   */
  public Converter()
  {

  }

  private final Logger log = LoggerFactory.getLogger(this.getClass());

  /**
   * A container we'll use to read data from.
   */
  private IContainer mIContainer = null;
  /**
   * A container we'll use to write data from.
   */
  private IContainer mOContainer = null;

  /**
   * A set of {@link IStream} values for each stream in the input
   * {@link IContainer}.
   */
  private IStream[] mIStreams = null;
  /**
   * A set of {@link IStreamCoder} objects we'll use to decode audio and video.
   */
  private IStreamCoder[] mICoders = null;

  /**
   * A set of {@link IStream} objects for each stream we'll output to the output
   * {@link IContainer}.
   */
  private IStream[] mOStreams = null;
  /**
   * A set of {@link IStreamCoder} objects we'll use to encode audio and video.
   */
  private IStreamCoder[] mOCoders = null;

  /**
   * A set of {@link IVideoPicture} objects that we'll use to hold decoded video
   * data.
   */
  private IVideoPicture[] mIVideoPictures = null;
  /**
   * A set of {@link IVideoPicture} objects we'll use to hold
   * potentially-resampled video data before we encode it.
   */
  private IVideoPicture[] mOVideoPictures = null;

  /**
   * A set of {@link IAudioSamples} objects we'll use to hold decoded audio
   * data.
   */
  private IAudioSamples[] mISamples = null;
  /**
   * A set of {@link IAudioSamples} objects we'll use to hold
   * potentially-resampled audio data before we encode it.
   */
  private IAudioSamples[] mOSamples = null;

  /**
   * A set of {@link IAudioResampler} objects (one for each stream) we'll use to
   * resample audio if needed.
   */
  private IAudioResampler[] mASamplers = null;
  /**
   * A set of {@link IVideoResampler} objects (one for each stream) we'll use to
   * resample video if needed.
   */
  private IVideoResampler[] mVSamplers = null;

  /**
   * Should we convert audio
   */
  private boolean mHasAudio = true;
  /**
   * Should we convert video
   */
  private boolean mHasVideo = true;

  /**
   * Should we force an interleaving of the output
   */
  private final boolean mForceInterleave = true;

  /**
   * Should we attempt to encode 'in real time'
   */
  private boolean mRealTimeEncoder;

  private Long mStartClockTime;
  private Long mStartStreamTime;

  /**
   * Define all the command line options this program can take.
   * 
   * @return The set of accepted options.
   */
  public Options defineOptions()
  {
    Options options = new Options();

    Option help = new Option("help", "print this message");

    /*
     * Output container options.
     */
    OptionBuilder.withArgName("container-format");
    OptionBuilder.hasArg(true);
    OptionBuilder
        .withDescription("output container format to use (e.g. \"mov\")");
    Option containerFormat = OptionBuilder.create("containerformat");

    OptionBuilder.withArgName("icontainer-format");
    OptionBuilder.hasArg(true);
    OptionBuilder
        .withDescription("input container format to use (e.g. \"mov\")");
    Option icontainerFormat = OptionBuilder.create("icontainerformat");    
    
    OptionBuilder.withArgName("file");
    OptionBuilder.hasArg(true);
    OptionBuilder
        .withDescription("container option presets file");
    Option cpreset = OptionBuilder.create("cpreset");

    /*
     * Audio options
     */
    OptionBuilder.hasArg(false);
    OptionBuilder.withDescription("no audio");
    Option ano = OptionBuilder.create("ano");

    OptionBuilder.withArgName("file");
    OptionBuilder.hasArg(true);
    OptionBuilder
        .withDescription("audio option presets file");
    Option apreset = OptionBuilder.create("apreset");

    OptionBuilder.withArgName("codec");
    OptionBuilder.hasArg(true);
    OptionBuilder
        .withDescription("audio codec to encode with (e.g. \"libmp3lame\")");
    Option acodec = OptionBuilder.create("acodec");

    OptionBuilder.withArgName("icodec");
    OptionBuilder.hasArg(true);
    OptionBuilder
        .withDescription("input audio codec  (e.g. \"libmp3lame\")");
    Option iacodec = OptionBuilder.create("iacodec");    
    
    OptionBuilder.withArgName("sample-rate");
    OptionBuilder.hasArg(true);
    OptionBuilder
        .withDescription("audio sample rate to (up/down) encode with (in hz) (e.g. \"22050\")");
    Option asamplerate = OptionBuilder.create("asamplerate");

    OptionBuilder.withArgName("isample-rate");
    OptionBuilder.hasArg(true);
    OptionBuilder
        .withDescription("input audio sample rate to (up/down) encode with (in hz) (e.g. \"22050\")");
    Option iasamplerate = OptionBuilder.create("iasamplerate");    
    
    OptionBuilder.withArgName("channels");
    OptionBuilder.hasArg(true);
    OptionBuilder
        .withDescription("number of audio channels (1 or 2) to encode with (e.g. \"2\")");
    Option achannels = OptionBuilder.create("achannels");

    OptionBuilder.withArgName("ichannels");
    OptionBuilder.hasArg(true);
    OptionBuilder
        .withDescription("input number of audio channels (1 or 2)");
    Option iachannels = OptionBuilder.create("iachannels");    
    
    OptionBuilder.withArgName("abit-rate");
    OptionBuilder.hasArg(true);
    OptionBuilder
        .withDescription("bit rate to encode audio with (in bps) (e.g. \"60000\")");
    Option abitrate = OptionBuilder.create("abitrate");

    OptionBuilder.withArgName("stream");
    OptionBuilder.hasArg(true);
    OptionBuilder
        .withDescription("if multiple audio streams of a given type, this is the stream you want to output");
    Option astream = OptionBuilder.create("astream");

    OptionBuilder.withArgName("quality");
    OptionBuilder.hasArg(true);
    OptionBuilder
        .withDescription("quality setting to use for audio.  0 means same as source; higher numbers are (perversely) lower quality.  Defaults to 0.");
    Option aquality = OptionBuilder.create("aquality");

    /*
     * Video options
     */
    OptionBuilder.hasArg(false);
    OptionBuilder.withDescription("no video");
    Option vno = OptionBuilder.create("vno");

    OptionBuilder.withArgName("file");
    OptionBuilder.hasArg(true);
    OptionBuilder
        .withDescription("video option presets file");
    Option vpreset = OptionBuilder.create("vpreset");

    OptionBuilder.withArgName("codec");
    OptionBuilder.hasArg(true);
    OptionBuilder
        .withDescription("video codec to encode with (e.g. \"mpeg4\")");
    Option vcodec = OptionBuilder.create("vcodec");

    OptionBuilder.withArgName("factor");
    OptionBuilder.hasArg(true);
    OptionBuilder
        .withDescription("scaling factor to scale output video by (e.g. \"0.75\")");
    Option vscaleFactor = OptionBuilder.create("vscalefactor");

    OptionBuilder.withArgName("vbitrate");
    OptionBuilder.hasArg(true);
    OptionBuilder
        .withDescription("bit rate to encode video with (in bps) (e.g. \"60000\")");
    Option vbitrate = OptionBuilder.create("vbitrate");

    OptionBuilder.withArgName("vbitratetolerance");
    OptionBuilder.hasArg(true);
    OptionBuilder
        .withDescription("bit rate tolerance the bitstream is allowed to diverge from the reference (in bits) (e.g. \"1200000\")");
    Option vbitratetolerance = OptionBuilder.create("vbitratetolerance");

    OptionBuilder.withArgName("stream");
    OptionBuilder.hasArg(true);
    OptionBuilder
        .withDescription("if multiple video streams of a given type, this is the stream you want to output");
    Option vstream = OptionBuilder.create("vstream");

    OptionBuilder.withArgName("quality");
    OptionBuilder.hasArg(true);
    OptionBuilder
        .withDescription("quality setting to use for video.  0 means same as source; higher numbers are (perversely) lower quality.  Defaults to 0.  If set, then bitrate flags are ignored.");
    Option vquality = OptionBuilder.create("vquality");
    
    OptionBuilder.withArgName("realtime");
    OptionBuilder.hasArg(false);
    OptionBuilder.withDescription("attempt to encode frames at the realtime rate -- i.e. it encodes when the picture should play");
    Option realtime = OptionBuilder.create("realtime");

    options.addOption(help);
    options.addOption(containerFormat);
    options.addOption(cpreset);
    options.addOption(ano);
    options.addOption(apreset);
    options.addOption(acodec);
    options.addOption(asamplerate);
    options.addOption(achannels);
    options.addOption(abitrate);
    options.addOption(astream);
    options.addOption(aquality);
    options.addOption(vno);
    options.addOption(vpreset);
    options.addOption(vcodec);
    options.addOption(vscaleFactor);
    options.addOption(vbitrate);
    options.addOption(vbitratetolerance);
    options.addOption(vstream);
    options.addOption(vquality);

    options.addOption(icontainerFormat);
    options.addOption(iacodec);
    options.addOption(iachannels);
    options.addOption(iasamplerate);

    options.addOption(realtime);
    
    return options;
  }

  /**
   * Given a set of arguments passed into this object, return back a parsed
   * command line.
   * 
   * @param opt
   *          A set of options as defined by {@link #defineOptions()}.
   * @param args
   *          A set of command line arguments passed into this class.
   * @return A parsed command line.
   * @throws ParseException
   *           If there is an error in the command line.
   */
  public CommandLine parseOptions(Options opt, String[] args)
      throws ParseException
  {
    CommandLine cmdLine = null;

    CommandLineParser parser = new GnuParser();

    cmdLine = parser.parse(opt, args);

    if (cmdLine.hasOption("help"))
    {
      HelpFormatter help = new HelpFormatter();
      help.printHelp("Xuggler [options] input_url output_url", opt);
      System.exit(1);
    }
    // Make sure we have only two left over args
    if (cmdLine.getArgs().length != 2)
      throw new ParseException("missing input or output url");

    return cmdLine;
  }

  /**
   * Get an integer value from a command line argument.
   * 
   * @param cmdLine
   *          A parsed command line (as returned from
   *          {@link #parseOptions(Options, String[])}
   * @param key
   *          The key for an option you want.
   * @param defaultVal
   *          The default value you want set if the key is not present in
   *          cmdLine.
   * @return The value for the key in the cmdLine, or defaultVal if it's not
   *         there.
   */
  private int getIntOptionValue(CommandLine cmdLine, String key, int defaultVal)
  {
    int retval = defaultVal;
    String optValue = cmdLine.getOptionValue(key);

    if (optValue != null)
    {
      try
      {
        retval = Integer.parseInt(optValue);
      }
      catch (Exception ex)
      {
        log
            .warn(
                "Option \"{}\" value \"{}\" cannot be converted to integer; using {} instead",
                new Object[]
                {
                    key, optValue, defaultVal
                });
      }
    }
    return retval;
  }

  /**
   * Get a double value from a command line argument.
   * 
   * @param cmdLine
   *          A parsed command line (as returned from
   *          {@link #parseOptions(Options, String[])}
   * @param key
   *          The key for an option you want.
   * @param defaultVal
   *          The default value you want set if the key is not present in
   *          cmdLine.
   * @return The value for the key in the cmdLine, or defaultVal if it's not
   *         there.
   */
  private double getDoubleOptionValue(CommandLine cmdLine, String key,
      double defaultVal)
  {
    double retval = defaultVal;
    String optValue = cmdLine.getOptionValue(key);

    if (optValue != null)
    {
      try
      {
        retval = Double.parseDouble(optValue);
      }
      catch (Exception ex)
      {
        log
            .warn(
                "Option \"{}\" value \"{}\" cannot be converted to double; using {} instead",
                new Object[]
                {
                    key, optValue, defaultVal
                });
      }
    }
    return retval;
  }

  /**
   * Open an initialize all Xuggler objects needed to encode and decode a video
   * file.
   * 
   * @param cmdLine
   *          A command line (as returned from
   *          {@link #parseOptions(Options, String[])}) that specifies what
   *          files we want to process and how to process them.
   * @return Number of streams in the input file, or <= 0 on error.
   */

  int setupStreams(CommandLine cmdLine)
  {
    String inputURL = cmdLine.getArgs()[0];
    String outputURL = cmdLine.getArgs()[1];

    mHasAudio = !cmdLine.hasOption("ano");
    mHasVideo = !cmdLine.hasOption("vno");

    mRealTimeEncoder = cmdLine.hasOption("realtime");
    
    String acodec = cmdLine.getOptionValue("acodec");
    String vcodec = cmdLine.getOptionValue("vcodec");
    String containerFormat = cmdLine.getOptionValue("containerformat");
    int astream = getIntOptionValue(cmdLine, "astream", -1);
    int aquality = getIntOptionValue(cmdLine, "aquality", 0);

    int sampleRate = getIntOptionValue(cmdLine, "asamplerate", 0);
    int channels = getIntOptionValue(cmdLine, "achannels", 0);
    int abitrate = getIntOptionValue(cmdLine, "abitrate", 0);
    int vbitrate = getIntOptionValue(cmdLine, "vbitrate", 0);
    int vbitratetolerance = getIntOptionValue(cmdLine, "vbitratetolerance", 0);
    int vquality = getIntOptionValue(cmdLine, "vquality", -1);
    int vstream = getIntOptionValue(cmdLine, "vstream", -1);
    double vscaleFactor = getDoubleOptionValue(cmdLine, "vscalefactor", 1.0);

    String icontainerFormat = cmdLine.getOptionValue("icontainerformat");    
    String iacodec = cmdLine.getOptionValue("iacodec");
    int isampleRate = getIntOptionValue(cmdLine, "iasamplerate", 0);
    int ichannels = getIntOptionValue(cmdLine, "iachannels", 0);
    
    // Should have everything now!
    int retval = 0;

    /**
     * Create one container for input, and one for output.
     */
    mIContainer = IContainer.make();
    mOContainer = IContainer.make();
    
    String cpreset = cmdLine.getOptionValue("cpreset");
    if (cpreset != null)
      Configuration.configure(cpreset, mOContainer);
    
    IContainerFormat iFmt = null;
    IContainerFormat oFmt = null;

    
    // override input format
    if (icontainerFormat != null)
    {
      iFmt = IContainerFormat.make();
     
      /**
       * Try to find an output format based on what the user specified, or
       * failing that, based on the outputURL (e.g. if it ends in .flv, we'll
       * guess FLV).
       */
      retval = iFmt.setInputFormat(icontainerFormat);
      if (retval < 0)
        throw new RuntimeException("could not find input container format: "
            + icontainerFormat);
    }    
    
    // override the input codec
    if (iacodec != null)
    {
      ICodec codec = null;
      /**
       * Looks like they did specify one; let's look it up by name.
       */
      codec = ICodec.findDecodingCodecByName(iacodec);
      if (codec == null || codec.getType() != ICodec.Type.CODEC_TYPE_AUDIO)
        throw new RuntimeException("could not find decoder: " + iacodec);
      /**
       * Now, tell the output stream coder that it's to use that codec.
       */
      mIContainer.setForcedAudioCodec(codec.getID());
    }
    

    /**
     * Open the input container for Reading.
     */
    IMetaData parameters = IMetaData.make();
    
    if (isampleRate > 0)
      parameters.setValue("sample_rate", ""+isampleRate);

    if (ichannels > 0)
      parameters.setValue("channels", ""+ichannels);
    
    IMetaData rejectParameters = IMetaData.make();

    retval = mIContainer.open(inputURL, IContainer.Type.READ, iFmt, false, true, 
        parameters, rejectParameters);
    if (retval < 0)
      throw new RuntimeException("could not open url: " + inputURL);
    if (rejectParameters.getNumKeys() > 0)
      throw new RuntimeException("some parameters were rejected: " + rejectParameters);
    /**
     * If the user EXPLICITLY asked for a output container format, we'll try to
     * honor their request here.
     */
    if (containerFormat != null)
    {
      oFmt = IContainerFormat.make();
      /**
       * Try to find an output format based on what the user specified, or
       * failing that, based on the outputURL (e.g. if it ends in .flv, we'll
       * guess FLV).
       */
      retval = oFmt.setOutputFormat(containerFormat, outputURL, null);
      if (retval < 0)
        throw new RuntimeException("could not find output container format: "
            + containerFormat);
    }
       
    /**
     * Open the output container for writing. If oFmt is null, we are telling
     * Xuggler to guess the output container format based on the outputURL.
     */
    retval = mOContainer.open(outputURL, IContainer.Type.WRITE, oFmt);
    if (retval < 0)
      throw new RuntimeException("could not open output url: " + outputURL);

    /**
     * Find out how many streams are there in the input container? For example,
     * most FLV files will have 2 -- 1 audio stream and 1 video stream.
     */
    int numStreams = mIContainer.getNumStreams();
    if (numStreams <= 0)
      throw new RuntimeException("not streams in input url: " + inputURL);

    /**
     * Here we create IStream, IStreamCoders and other objects for each input
     * stream.
     * 
     * We make parallel objects for each output stream as well.
     */
    mIStreams = new IStream[numStreams];
    mICoders = new IStreamCoder[numStreams];
    mOStreams = new IStream[numStreams];
    mOCoders = new IStreamCoder[numStreams];
    mASamplers = new IAudioResampler[numStreams];
    mVSamplers = new IVideoResampler[numStreams];
    mIVideoPictures = new IVideoPicture[numStreams];
    mOVideoPictures = new IVideoPicture[numStreams];
    mISamples = new IAudioSamples[numStreams];
    mOSamples = new IAudioSamples[numStreams];

    /**
     * Now let's go through the input streams one by one and explicitly set up
     * our contexts.
     */
    for (int i = 0; i < numStreams; i++)
    {
      /**
       * Get the IStream for this input stream.
       */
      IStream is = mIContainer.getStream(i);
      /**
       * And get the input stream coder. Xuggler will set up all sorts of
       * defaults on this StreamCoder for you (such as the audio sample rate)
       * when you open it.
       * 
       * You can create IStreamCoders yourself using
       * IStreamCoder#make(IStreamCoder.Direction), but then you have to set all
       * parameters yourself.
       */
      IStreamCoder ic = is.getStreamCoder();

      /**
       * Find out what Codec Xuggler guessed the input stream was encoded with.
       */
      ICodec.Type cType = ic.getCodecType();

      mIStreams[i] = is;
      mICoders[i] = ic;
      mOStreams[i] = null;
      mOCoders[i] = null;
      mASamplers[i] = null;
      mVSamplers[i] = null;
      mIVideoPictures[i] = null;
      mOVideoPictures[i] = null;
      mISamples[i] = null;
      mOSamples[i] = null;

      if (cType == ICodec.Type.CODEC_TYPE_AUDIO && mHasAudio
          && (astream == -1 || astream == i))
      {
         /**
         * First, did the user specify an audio codec?
         */
        ICodec codec = null;
        if (acodec != null)
        {
          /**
           * Looks like they did specify one; let's look it up by name.
           */
          codec = ICodec.findEncodingCodecByName(acodec);
          if (codec == null || codec.getType() != cType)
            throw new RuntimeException("could not find encoder: " + acodec);
 
        }
        else
        {
          /**
           * Looks like the user didn't specify an output coder for audio.
           * 
           * So we ask Xuggler to guess an appropriate output coded based on the
           * URL, container format, and that it's audio.
           */
          codec = ICodec.guessEncodingCodec(oFmt, null, outputURL, null,
              cType);
          if (codec == null)
            throw new RuntimeException("could not guess " + cType
                + " encoder for: " + outputURL);
        }
        /**
         * So it looks like this stream as an audio stream. Now we add an audio
         * stream to the output container that we will use to encode our
         * resampled audio.
         */
        IStream os = mOContainer.addNewStream(codec);

        /**
         * And we ask the IStream for an appropriately configured IStreamCoder
         * for output.
         * 
         * Unfortunately you still need to specify a lot of things for
         * outputting (because we can't really guess what you want to encode
         * as).
         */
        IStreamCoder oc = os.getStreamCoder();

        mOStreams[i] = os;
        mOCoders[i] = oc;

       /**
         * Now let's see if the codec can support the input sample format; if not
         * we pick the last sample format the codec supports.
         */
        Format preferredFormat = ic.getSampleFormat();
        
        List<Format> formats = codec.getSupportedAudioSampleFormats();
        for(Format format : formats) {
          oc.setSampleFormat(format);
          if (format == preferredFormat)
            break;
        }

        final String apreset = cmdLine.getOptionValue("apreset");
        if (apreset != null)
          Configuration.configure(apreset, oc);
        
        /**
         * In general a IStreamCoder encoding audio needs to know: 1) A ICodec
         * to use. 2) The sample rate and number of channels of the audio. Most
         * everything else can be defaulted.
         */

        /**
         * If the user didn't specify a sample rate to encode as, then just use
         * the same sample rate as the input.
         */
        if (sampleRate == 0)
          sampleRate = ic.getSampleRate();
        oc.setSampleRate(sampleRate);
        /**
         * If the user didn't specify a bit rate to encode as, then just use the
         * same bit as the input.
         */
        if (abitrate == 0)
          abitrate = ic.getBitRate();
        if (abitrate == 0)
          // some containers don't give a bit-rate
          abitrate = 64000;
        oc.setBitRate(abitrate);
        
        /**
         * If the user didn't specify the number of channels to encode audio as,
         * just assume we're keeping the same number of channels.
         */
        if (channels == 0)
          channels = ic.getChannels();
        oc.setChannels(channels);

        /**
         * And set the quality (which defaults to 0, or highest, if the user
         * doesn't tell us one).
         */
        oc.setGlobalQuality(aquality);

        /**
         * Now check if our output channels or sample rate differ from our input
         * channels or sample rate.
         * 
         * If they do, we're going to need to resample the input audio to be in
         * the right format to output.
         */
        if (oc.getChannels() != ic.getChannels()
            || oc.getSampleRate() != ic.getSampleRate()
            || oc.getSampleFormat() != ic.getSampleFormat())
        {
          /**
           * Create an audio resampler to do that job.
           */
          mASamplers[i] = IAudioResampler.make(oc.getChannels(), ic
              .getChannels(), oc.getSampleRate(), ic.getSampleRate(),
              oc.getSampleFormat(), ic.getSampleFormat());
          if (mASamplers[i] == null)
          {
            throw new RuntimeException(
                "could not open audio resampler for stream: " + i);
          }
        }
        else
        {
          mASamplers[i] = null;
        }
        /**
         * Finally, create some buffers for the input and output audio
         * themselves.
         * 
         * We'll use these repeated during the #run(CommandLine) method.
         */
        mISamples[i] = IAudioSamples.make(1024, ic.getChannels(), ic.getSampleFormat());
        mOSamples[i] = IAudioSamples.make(1024, oc.getChannels(), oc.getSampleFormat());
      }
      else if (cType == ICodec.Type.CODEC_TYPE_VIDEO && mHasVideo
          && (vstream == -1 || vstream == i))
      {
        /**
         * If you're reading these commends, this does the same thing as the
         * above branch, only for video. I'm going to assume you read those
         * comments and will only document something substantially different
         * here.
         */
        ICodec codec = null;
        if (vcodec != null)
        {
          codec = ICodec.findEncodingCodecByName(vcodec);
          if (codec == null || codec.getType() != cType)
            throw new RuntimeException("could not find encoder: " + vcodec);
        }
        else
        {
          codec = ICodec.guessEncodingCodec(oFmt, null, outputURL, null,
              cType);
          if (codec == null)
            throw new RuntimeException("could not guess " + cType
                + " encoder for: " + outputURL);

        }
        final IStream os = mOContainer.addNewStream(codec);
        final IStreamCoder oc = os.getStreamCoder();

        mOStreams[i] = os;
        mOCoders[i] = oc;


        // Set options AFTER selecting codec
        final String vpreset = cmdLine.getOptionValue("vpreset");
        if (vpreset != null)
          Configuration.configure(vpreset, oc);
        
        /**
         * In general a IStreamCoder encoding video needs to know: 1) A ICodec
         * to use. 2) The Width and Height of the Video 3) The pixel format
         * (e.g. IPixelFormat.Type#YUV420P) of the video data. Most everything
         * else can be defaulted.
         */
        if (vbitrate == 0)
          vbitrate = ic.getBitRate();
        if (vbitrate == 0)
          vbitrate = 250000;
        oc.setBitRate(vbitrate);
        if (vbitratetolerance > 0)
          oc.setBitRateTolerance(vbitratetolerance);

        int oWidth = ic.getWidth();
        int oHeight = ic.getHeight();

        if (oHeight <= 0 || oWidth <= 0)
          throw new RuntimeException("could not find width or height in url: "
              + inputURL);

        /**
         * For this program we don't allow the user to specify the pixel format
         * type; we force the output to be the same as the input.
         */
        oc.setPixelType(ic.getPixelType());

        if (vscaleFactor != 1.0)
        {
          /**
           * In this case, it looks like the output video requires rescaling, so
           * we create a IVideoResampler to do that dirty work.
           */
          oWidth = (int) (oWidth * vscaleFactor);
          oHeight = (int) (oHeight * vscaleFactor);

          mVSamplers[i] = IVideoResampler
              .make(oWidth, oHeight, oc.getPixelType(), ic.getWidth(), ic
                  .getHeight(), ic.getPixelType());
          if (mVSamplers[i] == null)
          {
            throw new RuntimeException(
                "This version of Xuggler does not support video resampling "
                    + i);
          }
        }
        else
        {
          mVSamplers[i] = null;
        }
        oc.setHeight(oHeight);
        oc.setWidth(oWidth);

        if (vquality >= 0)
        {
          oc.setFlag(IStreamCoder.Flags.FLAG_QSCALE, true);
          oc.setGlobalQuality(vquality);
        }

        /**
         * TimeBases are important, especially for Video. In general Audio
         * encoders will assume that any new audio happens IMMEDIATELY after any
         * prior audio finishes playing. But for video, we need to make sure
         * it's being output at the right rate.
         * 
         * In this case we make sure we set the same time base as the input, and
         * then we don't change the time stamps of any IVideoPictures.
         * 
         * But take my word that time stamps are tricky, and this only touches
         * the envelope. The good news is, it's easier in Xuggler than some
         * other systems.
         */
        IRational num = null;
        num = ic.getFrameRate();
        oc.setFrameRate(num);
        oc.setTimeBase(IRational.make(num.getDenominator(), num
                .getNumerator()));
        num = null;

        /**
         * And allocate buffers for us to store decoded and resample video
         * pictures.
         */
        mIVideoPictures[i] = IVideoPicture.make(ic.getPixelType(), ic
            .getWidth(), ic.getHeight());
        mOVideoPictures[i] = IVideoPicture.make(oc.getPixelType(), oc
            .getWidth(), oc.getHeight());
      }
      else
      {
        log.warn("Ignoring input stream {} of type {}", i, cType);
      }

      /**
       * Now, once you've set up all the parameters on the StreamCoder, you must
       * open() them so they can do work.
       * 
       * They will return an error if not configured correctly, so we check for
       * that here.
       */
      if (mOCoders[i] != null)
      {
        // some codecs require experimental mode to be set, and so we set it here.
        retval = mOCoders[i].setStandardsCompliance(IStreamCoder.CodecStandardsCompliance.COMPLIANCE_EXPERIMENTAL);
        if (retval < 0)
          throw new RuntimeException ("could not set compliance mode to experimental");
        
        retval = mOCoders[i].open(null, null);
        if (retval < 0)
          throw new RuntimeException(
              "could not open output encoder for stream: " + i);
        retval = mICoders[i].open(null, null);
        if (retval < 0)
          throw new RuntimeException(
              "could not open input decoder for stream: " + i);
      }
    }

    /**
     * Pretty much every output container format has a header they need written,
     * so we do that here.
     * 
     * You must configure your output IStreams correctly before writing a
     * header, and few formats deal nicely with key parameters changing (e.g.
     * video width) after a header is written.
     */
    retval = mOContainer.writeHeader();
    if (retval < 0)
      throw new RuntimeException("Could not write header for: " + outputURL);

    /**
     * That's it with setup; we're good to begin!
     */
    return numStreams;
  }

  /**
   * Close and release all resources we used to run this program.
   */
  void closeStreams()
  {
    int numStreams = 0;
    int i = 0;

    numStreams = mIContainer.getNumStreams();
    /**
     * Some video coders (e.g. MP3) will often "read-ahead" in a stream and keep
     * extra data around to get efficient compression. But they need some way to
     * know they're never going to get more data. The convention for that case
     * is to pass null for the IMediaData (e.g. IAudioSamples or IVideoPicture)
     * in encodeAudio(...) or encodeVideo(...) once before closing the coder.
     * 
     * In that case, the IStreamCoder will flush all data.
     */
    for (i = 0; i < numStreams; i++)
    {
      if (mOCoders[i] != null)
      {
        IPacket oPacket = IPacket.make();
        do {
          if (mOCoders[i].getCodecType() == ICodec.Type.CODEC_TYPE_AUDIO)
            mOCoders[i].encodeAudio(oPacket, null, 0);
          else
            mOCoders[i].encodeVideo(oPacket, null, 0);
          if (oPacket.isComplete())
            mOContainer.writePacket(oPacket, mForceInterleave);
        } while (oPacket.isComplete());
      }
    }
    /**
     * Some container formats require a trailer to be written to avoid a corrupt
     * files.
     * 
     * Others, such as the FLV container muxer, will take a writeTrailer() call
     * to tell it to seek() back to the start of the output file and write the
     * (now known) duration into the Meta Data.
     * 
     * So trailers are required. In general if a format is a streaming format,
     * then the writeTrailer() will never seek backwards.
     * 
     * Make sure you don't close your codecs before you write your trailer, or
     * we'll complain loudly and not actually write a trailer.
     */
    int retval = mOContainer.writeTrailer();
    if (retval < 0)
      throw new RuntimeException("Could not write trailer to output file");

    /**
     * We do a nice clean-up here to show you how you should do it.
     * 
     * That said, Xuggler goes to great pains to clean up after you if you
     * forget to release things. But still, you should be a good boy or giral
     * and clean up yourself.
     */
    for (i = 0; i < numStreams; i++)
    {
      if (mOCoders[i] != null)
      {
        /**
         * And close the input coder to tell Xuggler it can release all native
         * memory.
         */
        mOCoders[i].close();
      }
      mOCoders[i] = null;
      if (mICoders[i] != null)
        /**
         * Close the input coder to tell Xuggler it can release all native
         * memory.
         */
        mICoders[i].close();
      mICoders[i] = null;
    }

    /**
     * Tell Xuggler it can close the output file, write all data, and free all
     * relevant memory.
     */
    mOContainer.close();
    /**
     * And do the same with the input file.
     */
    mIContainer.close();

    /**
     * Technically setting everything to null here doesn't do anything but tell
     * Java it can collect the memory it used.
     * 
     * The interesting thing to note here is that if you forget to close() a
     * Xuggler object, but also loose all references to it from Java, you won't
     * leak the native memory. Instead, we'll clean up after you, but we'll
     * complain LOUDLY in your logs, so you really don't want to do that.
     */
    mOContainer = null;
    mIContainer = null;
    mISamples = null;
    mOSamples = null;
    mIVideoPictures = null;
    mOVideoPictures = null;
    mOCoders = null;
    mICoders = null;
    mASamplers = null;
    mVSamplers = null;
  }

  /**
   * Allow child class to override this method to alter the audio frame before
   * it is rencoded and written. In this implementation the audio frame is
   * passed through unmodified.
   * 
   * @param audioFrame
   *          the source audio frame to be modified
   * 
   * @return the modified audio frame
   */

  protected IAudioSamples alterAudioFrame(IAudioSamples audioFrame)
  {
    return audioFrame;
  }

  /**
   * Allow child class to override this method to alter the video frame before
   * it is rencoded and written. In this implementation the video frame is
   * passed through unmodified.
   * 
   * @param videoFrame
   *          the source video frame to be modified
   * 
   * @return the modified video frame
   */

  protected IVideoPicture alterVideoFrame(IVideoPicture videoFrame)
  {
    return videoFrame;
  }

  /**
   * Takes a given command line and decodes the input file, and encodes with new
   * parameters to the output file.
   * 
   * @param cmdLine
   *          A command line returned from
   *          {@link #parseOptions(Options, String[])}.
   */
  public void run(CommandLine cmdLine)
  {
    /**
     * Setup all our input and outputs
     */
    setupStreams(cmdLine);

    /**
     * Create packet buffers for reading data from and writing data to the
     * conatiners.
     */
    IPacket iPacket = IPacket.make();
    IPacket oPacket = IPacket.make();

    /**
     * Keep some "pointers' we'll use for the audio we're working with.
     */
    IAudioSamples inSamples = null;
    IAudioSamples outSamples = null;
    IAudioSamples reSamples = null;

    int retval = 0;

    /**
     * And keep some convenience pointers for the specific stream we're working
     * on for a packet.
     */
    IStreamCoder ic = null;
    IStreamCoder oc = null;
    IAudioResampler as = null;
    IVideoResampler vs = null;
    IVideoPicture inFrame = null;
    IVideoPicture reFrame = null;

    /**
     * Now, we've already opened the files in #setupStreams(CommandLine). We
     * just keep reading packets from it until the IContainer returns <0
     */
    while (mIContainer.readNextPacket(iPacket) == 0)
    {
      /**
       * Find out which stream this packet belongs to.
       */
      int i = iPacket.getStreamIndex();
      int offset = 0;

      /**
       * Find out if this stream has a starting timestamp
       */
      IStream stream = mIContainer.getStream(i);
      long tsOffset = 0;
      if (stream.getStartTime() != Global.NO_PTS && stream.getStartTime() > 0
          && stream.getTimeBase() != null)
      {
        IRational defTimeBase = IRational.make(1,
            (int) Global.DEFAULT_PTS_PER_SECOND);
        tsOffset = defTimeBase.rescale(stream.getStartTime(), stream
            .getTimeBase());
      }
      /**
       * And look up the appropriate objects that are working on that stream.
       */
      ic = mICoders[i];
      oc = mOCoders[i];
      as = mASamplers[i];
      vs = mVSamplers[i];
      inFrame = mIVideoPictures[i];
      reFrame = mOVideoPictures[i];
      inSamples = mISamples[i];
      reSamples = mOSamples[i];

      if (oc == null)
        // we didn't set up this coder; ignore the packet
        continue;

      /**
       * Find out if the stream is audio or video.
       */
      ICodec.Type cType = ic.getCodecType();

      if (cType == ICodec.Type.CODEC_TYPE_AUDIO && mHasAudio)
      {
        /**
         * Decoding audio works by taking the data in the packet, and eating
         * chunks from it to create decoded raw data.
         * 
         * However, there may be more data in a packet than is needed to get one
         * set of samples (or less), so you need to iterate through the byts to
         * get that data.
         * 
         * The following loop is the standard way of doing that.
         */
        while (offset < iPacket.getSize())
        {
          retval = ic.decodeAudio(inSamples, iPacket, offset);
          if (retval <= 0)
            throw new RuntimeException("could not decode audio.  stream: " + i);

          if (inSamples.getTimeStamp() != Global.NO_PTS)
            inSamples.setTimeStamp(inSamples.getTimeStamp() - tsOffset);

          log.trace("packet:{}; samples:{}; offset:{}", new Object[]
          {
              iPacket, inSamples, tsOffset
          });

          /**
           * If not an error, the decodeAudio returns the number of bytes it
           * consumed. We use that so the next time around the loop we get new
           * data.
           */
          offset += retval;
          int numSamplesConsumed = 0;
          /**
           * If as is not null then we know a resample was needed, so we do that
           * resample now.
           */
          if (as != null && inSamples.getNumSamples() > 0)
          {
            retval = as.resample(reSamples, inSamples, inSamples
                .getNumSamples());

            outSamples = reSamples;
          }
          else
          {
            outSamples = inSamples;
          }

          /**
           * Include call a hook to derivied classes to allow them to alter the
           * audio frame.
           */

          outSamples = alterAudioFrame(outSamples);

          /**
           * Now that we've resampled, it's time to encode the audio.
           * 
           * This workflow is similar to decoding; you may have more, less or
           * just enough audio samples available to encode a packet. But you
           * must iterate through.
           * 
           * Unfortunately (don't ask why) there is a slight difference between
           * encodeAudio and decodeAudio; encodeAudio returns the number of
           * samples consumed, NOT the number of bytes. This can be confusing,
           * and we encourage you to read the IAudioSamples documentation to
           * find out what the difference is.
           * 
           * But in any case, the following loop encodes the samples we have
           * into packets.
           */
          while (numSamplesConsumed < outSamples.getNumSamples())
          {
            retval = oc.encodeAudio(oPacket, outSamples, numSamplesConsumed);
            if (retval <= 0)
              throw new RuntimeException("Could not encode any audio: "
                  + retval);
            /**
             * Increment the number of samples consumed, so that the next time
             * through this loop we encode new audio
             */
            numSamplesConsumed += retval;
            log.trace("out packet:{}; samples:{}; offset:{}", new Object[]{
                oPacket, outSamples, tsOffset
            });

            writePacket(oPacket);
          }
        }

      }
      else if (cType == ICodec.Type.CODEC_TYPE_VIDEO && mHasVideo)
      {
        /**
         * This encoding workflow is pretty much the same as the for the audio
         * above.
         * 
         * The only major delta is that encodeVideo() will always consume one
         * frame (whereas encodeAudio() might only consume some samples in an
         * IAudioSamples buffer); it might not be able to output a packet yet,
         * but you can assume that you it consumes the entire frame.
         */
        IVideoPicture outFrame = null;
        while (offset < iPacket.getSize())
        {
          retval = ic.decodeVideo(inFrame, iPacket, offset);
          if (retval <= 0)
            throw new RuntimeException("could not decode any video.  stream: "
                + i);

          log.trace("decoded vid ts: {}; pkts ts: {}", inFrame.getTimeStamp(),
              iPacket.getTimeStamp());
          if (inFrame.getTimeStamp() != Global.NO_PTS)
            inFrame.setTimeStamp(inFrame.getTimeStamp() - tsOffset);

          offset += retval;
          if (inFrame.isComplete())
          {
            if (vs != null)
            {
              retval = vs.resample(reFrame, inFrame);
              if (retval < 0)
                throw new RuntimeException("could not resample video");
              outFrame = reFrame;
            }
            else
            {
              outFrame = inFrame;
            }

            /**
             * Include call a hook to derivied classes to allow them to alter
             * the audio frame.
             */

            outFrame = alterVideoFrame(outFrame);

            outFrame.setQuality(0);
            retval = oc.encodeVideo(oPacket, outFrame, 0);
            if (retval < 0)
              throw new RuntimeException("could not encode video");
            writePacket(oPacket);
          }
        }
      }
      else
      {
        /**
         * Just to be complete; there are other types of data that can show up
         * in streams (e.g. SUB TITLE).
         * 
         * Right now we don't support decoding and encoding that data, but youc
         * could still decide to write out the packets if you wanted.
         */
        log.trace("ignoring packet of type: {}", cType);
      }

    }

    // and cleanup.
    closeStreams();
  }

  private void writePacket(IPacket oPacket)
  {
    int retval;
    if (oPacket.isComplete())
    {
      if (mRealTimeEncoder)
      {
        delayForRealTime(oPacket);
      }
      /**
       * If we got a complete packet out of the encoder, then go ahead
       * and write it to the container.
       */
      retval = mOContainer.writePacket(oPacket, mForceInterleave);
      if (retval < 0)
        throw new RuntimeException("could not write output packet");
    }
  }

  /**
   * WARNING for those who want to copy this method and think it'll stream
   * for them -- it won't.  It doesn't interleave packets from non-interleaved
   * containers, so instead it'll write chunky data.  But it's useful if you
   * have previously interleaved data that you want to write out slowly to
   * a file, or, a socket.
   * @param oPacket the packet about to be written.
   */
  private void delayForRealTime(IPacket oPacket)
  {
    // convert packet timestamp to microseconds
    final IRational timeBase = oPacket.getTimeBase();
    if (timeBase == null || timeBase.getNumerator() == 0 ||
        timeBase.getDenominator() == 0)
      return;
    long dts = oPacket.getDts();
    if (dts == Global.NO_PTS)
      return;
    
    final long currStreamTime = IRational.rescale(dts,
        1,
        1000000,
        timeBase.getNumerator(),
        timeBase.getDenominator(),
        IRational.Rounding.ROUND_NEAR_INF);
    if (mStartStreamTime == null)
      mStartStreamTime = currStreamTime;

    // convert now to microseconds
    final long currClockTime = System.nanoTime()/1000;
    if (mStartClockTime == null)
      mStartClockTime = currClockTime;
    
    final long currClockDelta  = currClockTime - mStartClockTime;
    if (currClockDelta < 0)
      return;
    final long currStreamDelta = currStreamTime - mStartStreamTime;
    if (currStreamDelta < 0)
      return;
    final long streamToClockDeltaMilliseconds = (currStreamDelta - currClockDelta)/1000;
    if (streamToClockDeltaMilliseconds <= 0)
      return;
    try
    {
      Thread.sleep(streamToClockDeltaMilliseconds);
    }
    catch (InterruptedException e)
    {
    }
  }

  /**
   * 
   * A simple test of xuggler, this program takes an input file, and outputs it
   * as an output file.
   * 
   * @param args
   *          The command line args passed to this program.
   */
  
  public static void main(String[] args)
  {
    Converter converter = new Converter();

    try
    {
      // first define options
      Options options = converter.defineOptions();
      // And then parse them.
      CommandLine cmdLine = converter.parseOptions(options, args);
      // Finally, run the converter.
      converter.run(cmdLine);
    }
    catch (Exception exception)
    {
      System.err.printf("Error: %s\n", exception.getMessage());
    }
  }

}

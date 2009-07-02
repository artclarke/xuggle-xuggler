package com.xuggle.xuggler;

import java.util.Collection;

/**
 * A global configuration class for Xuggler.  
 * 
 * Today this just includes methods to print out all the
 * available options on this system.
 * 
 * @author aclarke
 *
 */
public class Configuration
{
  /** don't allow construction */
  private Configuration() {}
  
  /**
   * A main program that just prints information on all avaiable
   * Xuggler options.
   * @param args ignored
   */
  public static void main(String[] args)
  {
    printHelp(System.out);
  }

  
  /**
   * Print out help about this Xuggler installation.
   * 
   * Specifically all installed and supported container formats, installed
   * and supported codecs, all {@link IContainer} options,
   * all {@link IStreamCoder} options and all {@link IVideoResampler}
   * options.
   * @param stream stream to print to
   */
  public static void printHelp(java.io.PrintStream stream)
  {
    printSupportedFormats(stream);
    printSupportedCodecs(stream);
    printSupportedContainerProperties(stream);
    printSupportedStreamCoderProperties(stream);
    printSupportedVideoResamplerProperties(stream);
  }

  /**
   * Print out all installed and supported container formats on this system.
   * @param stream stream to print to
   */
  private static void printSupportedFormats(java.io.PrintStream stream)
  {
    stream.println("=======================================");
    stream.println("  Demuxable Formats");
    stream.println("=======================================");
    
    Collection<IContainerFormat> inputFormats = IContainerFormat.getInstalledInputFormats();
    for(IContainerFormat inputFormat : inputFormats)
    {
      stream.printf("  \"%s\": %s\n", inputFormat.getInputFormatShortName(),
          inputFormat.getInputFormatLongName());
    }
    stream.println("=======================================");
    stream.println("  Muxable Formats");
    stream.println("=======================================");
    
    Collection<IContainerFormat> outputFormats = IContainerFormat.getInstalledOutputFormats();
    for(IContainerFormat outputFormat : outputFormats)
    {
      stream.printf("  \"%s\": %s\n", outputFormat.getOutputFormatShortName(),
          outputFormat.getOutputFormatLongName());
    }
  }
  
  /**
   * Print out all installed and supported codecs on this system.
   * @param stream stream to print to
   */
  private static void printSupportedCodecs(java.io.PrintStream stream)
  {
    Collection<ICodec> codecs = ICodec.getInstalledCodecs();
    
    stream.println("=======================================");
    stream.println("  Decodeable Codecs");
    stream.println("=======================================");

    for(ICodec codec : codecs)
    {
      if (codec.canDecode())
      {
        stream.printf("%s %s (%s): %s\n",
            codec.getType(),
            codec.getID(),
            codec.getName(),
            codec.getLongName());
      }
    }
    stream.println("=======================================");
    stream.println("  Encodeable Codecs");
    stream.println("=======================================");
    for(ICodec codec : codecs)
    {
      if (codec.canEncode())
      {
        stream.printf("%s %s (%s): %s\n",
            codec.getType(),
            codec.getID(),
            codec.getName(),
            codec.getLongName());
      }
    }
  }

  /**
   * Print out all {@link IVideoResampler} supported properties, if
   * that option is supported in this version of Xuggler.
   * 
   * @param stream stream to print to.
   */
  private static void printSupportedVideoResamplerProperties(java.io.PrintStream stream)
  {
    if (IVideoResampler.isSupported(IVideoResampler.Feature.FEATURE_COLORSPACECONVERSION))
      printConfigurable(stream, IVideoResampler.make(100,
          100,
          IPixelFormat.Type.YUV420P,
          200,
          200,
          IPixelFormat.Type.RGB24));
  }

  /**
   * Print out all {@link IStreamCoder} supported properties.
   * 
   * @param stream stream to print to.
   */
  private static void printSupportedStreamCoderProperties(java.io.PrintStream stream)
  {
    printConfigurable(stream, IStreamCoder.make(IStreamCoder.Direction.ENCODING));
  }

  /**
   * Print out all {@link IContainer} supported properties.
   * 
   * @param stream stream to print to.
   */
  private static void printSupportedContainerProperties(java.io.PrintStream stream)
  {
    try
    {
      IContainer container = IContainer.make();
      java.io.File tmpFile = java.io.File.createTempFile("xuggler", ".flv");
      try
      {
        String filename = tmpFile.getAbsolutePath();
        container.open(filename,
            IContainer.Type.WRITE, null);
        printConfigurable(stream, container);
        container.close();
        container.delete();
      } finally {
        tmpFile.delete();
      }
    } catch (Throwable t) {
      return;
    }
  }

  /**
   * Print out all configurable options on the {@link IConfigurable} object.
   * 
   * @param stream stream to print to
   * @param configObj {@link IConfigurable} object.
   */
  private static void printConfigurable(java.io.PrintStream stream,
      IConfigurable configObj)
  {
    stream.println("=======================================");
    stream.println("  " + configObj.getClass().getName() + " Properties");
    stream.println("=======================================");
    int numOptions = configObj.getNumProperties();
    
    for(int i = 0; i < numOptions; i++)
    {
      IProperty prop = configObj.getPropertyMetaData(i);
      printOption(stream, configObj, prop);
    }
   
  }
  
  /**
   * Print information about the property on the configurable object.
   * 
   * @param stream stream to print to
   * @param configObj configurable object
   * @param prop property on object
   */
  public static void printOption(java.io.PrintStream stream,
      IConfigurable configObj, IProperty prop)
  {
    if (prop.getType() != IProperty.Type.PROPERTY_FLAGS)
    {
      stream.printf("  %s; default= %s; type=%s;\n",
          prop.getName(),
          configObj.getPropertyAsString(prop.getName()),
          prop.getType());
    } else {
      // it's a flag
      stream.printf("  %s; default= %d; valid values=(",
          prop.getName(),
          configObj.getPropertyAsLong(prop.getName()));
      int numSettings = prop.getNumFlagSettings();
      long value = configObj.getPropertyAsLong(prop.getName());
      for(int i = 0; i < numSettings; i++)
      {
        IProperty fprop = prop.getFlagConstant(i);
        long flagMask = fprop.getDefault();
        boolean isSet = (value & flagMask)>0;
        stream.printf("%s%s; ",
            isSet ? "+" : "-",
                fprop.getName());
      }
      stream.printf("); type=%s;\n", prop.getType());
    }
    stream.printf("    help for %s: %s\n",
        prop.getName(),
        prop.getHelp() == null ? "no help available" : prop.getHelp());
  }
}

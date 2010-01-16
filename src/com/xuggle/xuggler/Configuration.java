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

import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.util.Collection;
import java.util.Enumeration;
import java.util.Properties;

/**
 * A global configuration class for Xuggler.  
 * <p>
 * This object can help print setting options
 * on {@link IConfigurable} classes, and also
 * provides a convenient method {@link #configure(Properties, IConfigurable)}
 * that lets you configure {@link IConfigurable} objects from
 * a Java properties or FFmpeg preset file.
 * </p> 
 * @see #configure(Properties, IConfigurable)
 * @see IConfigurable
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
    printSupportedContainerFormats(stream);
    printSupportedCodecs(stream);
    printSupportedContainerProperties(stream);
    printSupportedStreamCoderProperties(stream);
    printSupportedVideoResamplerProperties(stream);
  }

  /**
   * Print out all installed and supported container formats on this system.
   * @param stream stream to print to
   */
  public static void printSupportedContainerFormats(java.io.PrintStream stream)
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
  public static void printSupportedCodecs(java.io.PrintStream stream)
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
  public static void printSupportedVideoResamplerProperties(java.io.PrintStream stream)
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
  public static void printSupportedStreamCoderProperties(java.io.PrintStream stream)
  {
    printConfigurable(stream, IStreamCoder.make(IStreamCoder.Direction.ENCODING));
  }

  /**
   * Print out all {@link IContainer} supported properties.
   * 
   * @param stream stream to print to.
   */
  public static void printSupportedContainerProperties(java.io.PrintStream stream)
  {
    printConfigurable(stream, IContainer.make());
  }

  /**
   * Print out all configurable options on the {@link IConfigurable} object.
   * 
   * @param stream stream to print to
   * @param configObj {@link IConfigurable} object.
   */
  public static void printConfigurable(java.io.PrintStream stream,
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
  
  /**
   * Configures an {@link IConfigurable} from a set of Java {@link Properties}.
   * <p>
   * Here's some sample code that shows configuring a IStreamCoder
   * from a FFmpeg preset file:
   * </p>
   * <pre>
   *   Properties props = new Properties();
   *   props.load(new FileInputStream(file));
   *   IStreamCoder coder = IStreamCoder.make(Direction.ENCODING);
   *   int retval = Configuration.configure(props, coder);
   *   if (retval < 0)
   *      throw new RuntimeException("Could not configure object");
   * </pre>
   * @param properties The properties to use.
   * @param config The item to configure.
   * @return <0 on error; >= 0 on success.
   */
  @SuppressWarnings("unchecked")
  public static int configure(final Properties properties, final IConfigurable config)
  {
    for (
        final Enumeration<String> names = (Enumeration<String>) properties.propertyNames();
        names.hasMoreElements();
    )
    {
      final String name = names.nextElement();
      final String value = properties.getProperty(name);
      if (value != null) {
        final int retval = config.setProperty(name, value);
        if (retval < 0)
          return retval;
      }
    }
    return 0;
  }
  /**
   * Configures an {@link IConfigurable} from a file.
   * <p>
   * This is a handy way to configure a Xuggler {@link IConfigurable}
   * object, and will also work with FFmpeg preset files.
   * </p>
   * <p>
   * Here's some sample code that shows configuring a IStreamCoder
   * from a FFmpeg preset file:
   * </p>
   * <pre>
   *   Property props = new Properties();
   *   props.load(new FileInputStream("location/to/preset/file.preset"));
   *   IStreamCoder coder = IStreamCoder.make(Direction.ENCODING);
   *   int retval = Configuration.configure(props, coder);
   *   if (retval < 0)
   *      throw new RuntimeException("Could not configure object");
   * </pre>
   * @param file A filename for the properties file.
   * @param config The item to configure.
   * @return <0 on error; >= 0 on success.
   */
  @SuppressWarnings("unchecked")
  public static int configure(final String file, final IConfigurable config)
  {
    Properties props = new Properties();
    try
    {
      props.load(new FileInputStream(file));
    }
    catch (FileNotFoundException e)
    {
      e.printStackTrace();
      return -1;
    }
    catch (IOException e)
    {
      e.printStackTrace();
      return -1;
    }
    return Configuration.configure(props, config);
  }

}

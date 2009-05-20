/*
 * This file is part of Xuggler.
 * 
 * Xuggler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 * 
 * Xuggler is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public
 * License along with Xuggler.  If not, see <http://www.gnu.org/licenses/>.
 */

%module Xuggler
%{

// Xuggler.i: Start generated code
// >>>>>>>>>>>>>>>>>>>>>>>>>>>
#include <com/xuggle/ferry/JNIHelper.h>
#include <com/xuggle/xuggler/IProperty.h>
#include <com/xuggle/xuggler/IPixelFormat.h>
#include <com/xuggle/xuggler/ITimeValue.h>
#include <com/xuggle/xuggler/IRational.h>
#include <com/xuggle/xuggler/IMediaData.h>
#include <com/xuggle/xuggler/ICodec.h>
#include <com/xuggle/xuggler/IPacket.h>
#include <com/xuggle/xuggler/IAudioSamples.h>
#include <com/xuggle/xuggler/IAudioResampler.h>
#include <com/xuggle/xuggler/IVideoPicture.h>
#include <com/xuggle/xuggler/IVideoResampler.h>
#include <com/xuggle/xuggler/IStreamCoder.h>
#include <com/xuggle/xuggler/IStream.h>
#include <com/xuggle/xuggler/IContainerParameters.h>
#include <com/xuggle/xuggler/IContainerFormat.h>
#include <com/xuggle/xuggler/IContainer.h>
#include <com/xuggle/xuggler/IMediaDataWrapper.h>
#include <com/xuggle/xuggler/Global.h>
#include <com/xuggle/xuggler/IError.h>

using namespace VS_CPP_NAMESPACE;

/*
 * This will be called if an when we're loaded
 * directly by Java.  If we're linked to via
 * C/C++ linkage on another library, they
 * must call sSetVM().
 */
JNIEXPORT jint JNICALL
JNI_OnLoad(JavaVM *jvm, void *)
{
  if (!com::xuggle::ferry::JNIHelper::sGetVM())
    com::xuggle::ferry::JNIHelper::sSetVM(jvm);
  Global::init();
  return com::xuggle::ferry::JNIHelper::sGetJNIVersion();
}


// <<<<<<<<<<<<<<<<<<<<<<<<<<<
// Xuggler.i: End generated code

%}

%pragma(java) jniclassimports=%{
import com.xuggle.ferry.*;
%}

%pragma(java) moduleimports=%{
import org.apache.commons.cli.CommandLine;
import org.apache.commons.cli.Options;

import com.xuggle.xuggler.Converter;

/**
 * A very simple command line program that decodes a video file, and re-encodes it to a new file.
 * <p>
 * Very loosely based on the FFMPEG command line program, just with far fewer
 * bells and whistles.  It is meant many as a demonstration program for those trying
 * to learn this library.
 * </p>
 * <p>
 * To use, make sure that the Xuggler shared libraries have been installed in your system, and
 * then run (from the root of your build tree):
 * <code>
 * <pre>
 * java -jar dist/lib/xuggle-xuggler.jar --help
 * </pre>
 * </code>
 * And yes, I hate Java classpaths as much, if not more, than you do.
 * </p>
 * <p>
 * In reality, the {@link Converter} program does all the work here.
 * @see com.xuggle.xuggler.Converter
 */
%}

%pragma(java) modulecode=%{
  /**
   * 
   * A simple test of xuggler, this program takes an input
   * file, and outputs it as an output file.
   * 
   * @param args The command line args passed to this program.
   */
  public static void main(String[] args)
  {
    Converter converter = new Converter();
    
    try {
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

%}

%pragma(java) jniclasscode=%{
// Xuggler.i: Start generated code
// >>>>>>>>>>>>>>>>>>>>>>>>>>>

  static {
    com.xuggle.ferry.JNILibraryLoader.loadLibrary("xuggle-xuggler",
      new Long(Version.MAJOR_VERSION));
  }
  
// <<<<<<<<<<<<<<<<<<<<<<<<<<<
// Xuggler.i: End generated code
  
%}
// As per 1.17, we now make sure we generate proper Java enums on
// classes
%include "enums.swg"

%import <com/xuggle/ferry/Ferry.i>

%include <com/xuggle/xuggler/Xuggler.h>
%include <com/xuggle/xuggler/IProperty.h>
%include <com/xuggle/xuggler/IPixelFormat.h>
%include <com/xuggle/xuggler/IRational.swg>
%include <com/xuggle/xuggler/ITimeValue.h>
%include <com/xuggle/xuggler/ICodec.swg>
%include <com/xuggle/xuggler/IMediaData.swg>
%include <com/xuggle/xuggler/IPacket.swg>
%include <com/xuggle/xuggler/IAudioSamples.swg>
%include <com/xuggle/xuggler/IAudioResampler.h>
%include <com/xuggle/xuggler/IVideoPicture.swg>
%include <com/xuggle/xuggler/IVideoResampler.swg>
%include <com/xuggle/xuggler/IStreamCoder.swg>
%include <com/xuggle/xuggler/IStream.swg>
%include <com/xuggle/xuggler/IContainerParameters.h>
%include <com/xuggle/xuggler/IContainerFormat.swg>
%include <com/xuggle/xuggler/IContainer.swg>
%include <com/xuggle/xuggler/IMediaDataWrapper.swg>
%include <com/xuggle/xuggler/Global.h>
%include <com/xuggle/xuggler/IError.swg>

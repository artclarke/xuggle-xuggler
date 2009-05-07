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
%module Ferry
%{
#include <stdexcept>
#include <com/xuggle/ferry/JNIHelper.h>

extern "C" {
  JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *jvm, void *);
};

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
  return com::xuggle::ferry::JNIHelper::sGetJNIVersion();
}
#include <com/xuggle/ferry/RefCounted.h>
#include <com/xuggle/ferry/AtomicInteger.h>
#include <com/xuggle/ferry/Logger.h>
#include <com/xuggle/ferry/Mutex.h>
#include <com/xuggle/ferry/IBuffer.h>
#include <com/xuggle/ferry/RefCountedTester.h>

using namespace com::xuggle::ferry;

%}
%pragma(java) jniclasscode=%{
  static {
    JNILibraryLoader.loadLibrary(
      "xuggle-ferry",
      new Long(Version.MAJOR_VERSION));
    // This seems nuts, but it works around a Java 1.6 bug where
    // a race condition exists when JNI_NewDirectByteBuffer is called
    // from multiple threads.  See:
    // http://mail.openjdk.java.net/pipermail/hotspot-runtime-dev/2009-January/000382.html
    IBuffer buffer = IBuffer.make(null, 2);
    buffer.getByteBuffer(0,2);
  }
  
  public native static boolean isMirroringNativeMemoryInJVM();
  public native static void setMirroringNativeMemoryInJVM(boolean value);
  
%}
%pragma(java) moduleimports=%{
/**
 * Internal Only.
 * <p>
 * I meant that.
 * </p>
 * <p>
 * Really.  There's nothing here.
 * </p>
 */
%}
%import <com/xuggle/ferry/JNIHelper.swg>

%include <com/xuggle/Xuggle.h>
%include <com/xuggle/ferry/Ferry.h>
%include <com/xuggle/ferry/AtomicInteger.h>
%include <com/xuggle/ferry/RefCounted.swg>
%include <com/xuggle/ferry/Logger.h>
%include <com/xuggle/ferry/Mutex.h>
%include <com/xuggle/ferry/IBuffer.swg>
%include <com/xuggle/ferry/RefCountedTester.h>

%typemap(javaimports) SWIGTYPE, SWIGTYPE*, SWIGTYPE& "import com.xuggle.ferry.*;"


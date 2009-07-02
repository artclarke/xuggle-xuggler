/*
 * Copyright (c) 2008, 2009 by Xuggle Incorporated.  All rights reserved.
 * 
 * This file is part of Xuggler.
 * 
 * You can redistribute Xuggler and/or modify it under the terms of the GNU
 * Affero General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any
 * later version.
 * 
 * Xuggler is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public
 * License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with Xuggler.  If not, see <http://www.gnu.org/licenses/>.
 */

%typemap(javainterfaces) com::xuggle::xuggler::IContainer, com::xuggle::xuggler::IContainer*, com::xuggle::xuggler::IContainer& "com.xuggle.xuggler.IConfigurable";
%typemap (javacode) com::xuggle::xuggler::IContainer,com::xuggle::xuggler::IContainer*,com::xuggle::xuggler::IContainer& %{

  /**
   * info about this container.  We only print information that can be
   * determined without reading data from the container.
   * @return a string representation of this object
   */
   
  @Override
  public String toString()
  {
    StringBuilder result = new StringBuilder();
    
    result.append(this.getClass().getName()+"@"+hashCode()+"[");
    result.append("url:"+getURL()+";");
    result.append("type:"+getType()+";");
    result.append("format:"+getContainerFormat()+";");
    result.append("]");
    return result.toString();
  }

  /**
   * Open this container and make it ready for reading or writing.
   * Reading or writing ability is determined by
   * what the {@link com.xuggle.xuggler.io.IURLProtocolHandler}
   * passed in supports.
   * @see #open(String, IContainer.Type, IContainerFormat)
   */  
  public int open(
    com.xuggle.xuggler.io.IURLProtocolHandler handler,
    IContainer.Type type, IContainerFormat format)
  {
    return open(com.xuggle.xuggler.io.XugglerIO.map(handler), type, format); 
  }

  /**
   * Open this container and make it ready for writing.
   * @see #open(String, IContainer.Type, IContainerFormat)
   */  
  public int open(
    java.io.OutputStream output,
    IContainerFormat format)
  {
    return open(com.xuggle.xuggler.io.XugglerIO.map(output),
     IContainer.Type.WRITE, format); 
  }

  /**
   * Open this container and make it ready for reading.
   * @see #open(String, IContainer.Type, IContainerFormat)
   */  
  public int open(
    java.io.InputStream input,
    IContainerFormat format)
  {
    return open(com.xuggle.xuggler.io.XugglerIO.map(input),
     IContainer.Type.READ, format); 
  }

  /**
   * Open this container and make it ready for writing.
   * @see #open(String, IContainer.Type, IContainerFormat)
   */  
  public int open(
    java.io.DataOutput output,
    IContainerFormat format)
  {
    return open(com.xuggle.xuggler.io.XugglerIO.map(output),
     IContainer.Type.WRITE, format); 
  }

  /**
   * Open this container and make it ready for writing.
   * @see #open(String, IContainer.Type, IContainerFormat)
   */  
  public int open(
    java.io.DataOutputStream output,
    IContainerFormat format)
  {
    return open(com.xuggle.xuggler.io.XugglerIO.map(
        (java.io.OutputStream)output),
     IContainer.Type.WRITE, format); 
  }

  /**
   * Open this container and make it ready for reading.
   * @see #open(String, IContainer.Type, IContainerFormat)
   */  
  public int open(
    java.io.DataInput input,
    IContainerFormat format)
  {
    return open(com.xuggle.xuggler.io.XugglerIO.map(input),
     IContainer.Type.READ, format); 
  }

  /**
   * Open this container and make it ready for reading.
   * @see #open(String, IContainer.Type, IContainerFormat)
   */  
  public int open(
    java.io.DataInputStream input,
    IContainerFormat format)
  {
    return open(com.xuggle.xuggler.io.XugglerIO.map(
        (java.io.InputStream)input),
     IContainer.Type.READ, format); 
  }

  /**
   * Open this container and make it ready for reading or writing.
   * @see #open(String, IContainer.Type, IContainerFormat)
   */  
  public int open(
    java.io.RandomAccessFile file,
    IContainer.Type type,
    IContainerFormat format)
  {
    return open(com.xuggle.xuggler.io.XugglerIO.map(file),
     type, format); 
  }

  /**
   * Open this container and make it ready for writing.
   * @see #open(String, IContainer.Type, IContainerFormat)
   */  
  public int open(
    java.nio.channels.WritableByteChannel output,
    IContainerFormat format)
  {
    return open(com.xuggle.xuggler.io.XugglerIO.map(output),
     IContainer.Type.WRITE, format); 
  }

  /**
   * Open this container and make it ready for reading.
   * @see #open(String, IContainer.Type, IContainerFormat)
   */  
  public int open(
    java.nio.channels.ReadableByteChannel input,
    IContainerFormat format)
  {
    return open(com.xuggle.xuggler.io.XugglerIO.map(input),
     IContainer.Type.READ, format); 
  }

  /**
   * Open this container and make it ready for reading or writing.
   * @see #open(String, IContainer.Type, IContainerFormat)
   */  
  public int open(
    java.nio.channels.ByteChannel channel,
    IContainer.Type type,
    IContainerFormat format)
  {
    return open(com.xuggle.xuggler.io.XugglerIO.map(channel),
     type, format); 
  }


/**
 * Open this container and make it ready for reading or writing, optionally 
 * reading as far into the container as necessary to find all streams.
 * @see #open(String, IContainer.Type, IContainerFormat, boolean, boolean) 
 */
  public int open(com.xuggle.xuggler.io.IURLProtocolHandler handler,
    IContainer.Type type, IContainerFormat format,
    boolean streamsCanBeAddedDynamically,
    boolean queryStreamMetaData)
  {
    return open(com.xuggle.xuggler.io.XugglerIO.map(handler), type, format,
      streamsCanBeAddedDynamically,
      queryStreamMetaData); 
  }

  /**
   * Open this container and make it ready for writing.
   * @see #open(String, IContainer.Type, IContainerFormat, boolean, boolean) 
   */  
  public int open(
    java.io.OutputStream output,
    IContainerFormat format,
    boolean streamsCanBeAddedDynamically,
    boolean queryStreamMetaData)
  {
    return open(com.xuggle.xuggler.io.XugglerIO.map(output),
     IContainer.Type.WRITE, format, streamsCanBeAddedDynamically, queryStreamMetaData); 
  }

  /**
   * Open this container and make it ready for reading.
   * @see #open(String, IContainer.Type, IContainerFormat, boolean, boolean) 
   */  
  public int open(
    java.io.InputStream input,
    IContainerFormat format,
    boolean streamsCanBeAddedDynamically,
    boolean queryStreamMetaData)
  {
    return open(com.xuggle.xuggler.io.XugglerIO.map(input),
     IContainer.Type.READ, format, streamsCanBeAddedDynamically, queryStreamMetaData); 
  }

  /**
   * Open this container and make it ready for writing.
   * @see #open(String, IContainer.Type, IContainerFormat, boolean, boolean) 
   */  
  public int open(
    java.io.DataOutput output,
    IContainerFormat format,
    boolean streamsCanBeAddedDynamically,
    boolean queryStreamMetaData)
  {
    return open(com.xuggle.xuggler.io.XugglerIO.map(output),
     IContainer.Type.WRITE, format, streamsCanBeAddedDynamically, queryStreamMetaData); 
  }

  /**
   * Open this container and make it ready for writing.
   * @see #open(String, IContainer.Type, IContainerFormat, boolean, boolean) 
   */  
  public int open(
    java.io.DataOutputStream output,
    IContainerFormat format,
    boolean streamsCanBeAddedDynamically,
    boolean queryStreamMetaData)
  {
    return open(com.xuggle.xuggler.io.XugglerIO.map(
        (java.io.OutputStream)output),
     IContainer.Type.WRITE, format, streamsCanBeAddedDynamically, queryStreamMetaData); 
  }

  /**
   * Open this container and make it ready for reading.
   * @see #open(String, IContainer.Type, IContainerFormat, boolean, boolean) 
   */  
  public int open(
    java.io.DataInput input,
    IContainerFormat format,
    boolean streamsCanBeAddedDynamically,
    boolean queryStreamMetaData)
  {
    return open(com.xuggle.xuggler.io.XugglerIO.map(input),
     IContainer.Type.READ, format, streamsCanBeAddedDynamically, queryStreamMetaData); 
  }

  /**
   * Open this container and make it ready for reading.
   * @see #open(String, IContainer.Type, IContainerFormat, boolean, boolean) 
   */  
  public int open(
    java.io.DataInputStream input,
    IContainerFormat format,
    boolean streamsCanBeAddedDynamically,
    boolean queryStreamMetaData)
  {
    return open(com.xuggle.xuggler.io.XugglerIO.map(
        (java.io.InputStream)input),
     IContainer.Type.READ, format, streamsCanBeAddedDynamically, queryStreamMetaData); 
  }
  
  /**
   * Open this container and make it ready for reading or writing.
   * @see #open(String, IContainer.Type, IContainerFormat, boolean, boolean) 
   */  
  public int open(
    java.io.RandomAccessFile file,
    IContainer.Type type,
    IContainerFormat format,
    boolean streamsCanBeAddedDynamically,
    boolean queryStreamMetaData)
  {
    return open(com.xuggle.xuggler.io.XugglerIO.map(file),
     type, format, streamsCanBeAddedDynamically, queryStreamMetaData); 
  }

  /**
   * Open this container and make it ready for writing.
   * @see #open(String, IContainer.Type, IContainerFormat, boolean, boolean) 
   */  
  public int open(
    java.nio.channels.WritableByteChannel output,
    IContainerFormat format,
    boolean streamsCanBeAddedDynamically,
    boolean queryStreamMetaData)
  {
    return open(com.xuggle.xuggler.io.XugglerIO.map(output),
     IContainer.Type.WRITE, format, streamsCanBeAddedDynamically, queryStreamMetaData); 
  }

  /**
   * Open this container and make it ready for reading.
   * @see #open(String, IContainer.Type, IContainerFormat, boolean, boolean) 
   */  
  public int open(
    java.nio.channels.ReadableByteChannel input,
    IContainerFormat format,
    boolean streamsCanBeAddedDynamically,
    boolean queryStreamMetaData)
  {
    return open(com.xuggle.xuggler.io.XugglerIO.map(input),
     IContainer.Type.READ, format, streamsCanBeAddedDynamically, queryStreamMetaData); 
  }

  /**
   * Open this container and make it ready for reading or writing.
   * @see #open(String, IContainer.Type, IContainerFormat, boolean, boolean) 
   */  
  public int open(
    java.nio.channels.ByteChannel channel,
    IContainer.Type type,
    IContainerFormat format,
    boolean streamsCanBeAddedDynamically,
    boolean queryStreamMetaData)
  {
    return open(com.xuggle.xuggler.io.XugglerIO.map(channel),
     type, format, streamsCanBeAddedDynamically, queryStreamMetaData); 
  }

  /**
   * Gets the SDP data as a Java string.
   * <p>
   * This method only supports SDP files up to 4K in size.
   * If you have a larger SDP file, use the
   * {@link #createSDPData(com.xuggle.ferry.IBuffer)} method and pass in your
   * own {@link com.xuggle.ferry.IBuffer} object.
   * </p> 
   * @return A {@link String} representing the data.
   * @see #createSDPData(IBuffer)
   */
  public String createSDPData()
  {
    IBuffer buffer = IBuffer.make(null, 4096);
    int len = this.createSDPData(buffer);
    if (len > 1)
    {
      byte [] stringBuf = new byte[len-1];
      buffer.get(0, stringBuf, 0, stringBuf.length);
      return new String(stringBuf);
    }
    return null;
  }
  /**
   * {@inheritDoc}
   */
  public java.util.Collection<String> getPropertyNames()
  {
    java.util.Collection<String> retval = new java.util.LinkedList<String>();
    int numProperties = this.getNumProperties();
    for(int i = 0; i < numProperties; i++)
    {
      IProperty property = this.getPropertyMetaData(i);
      String name = property.getName();
      retval.add(name);
    }
    return retval;
  }

  
%}

%include <com/xuggle/xuggler/IContainer.h>

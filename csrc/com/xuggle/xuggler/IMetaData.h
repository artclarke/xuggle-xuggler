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

#ifndef IMETADATA_H_
#define IMETADATA_H_

#include <com/xuggle/xuggler/Xuggler.h>
#include <com/xuggle/ferry/RefCounted.h>

namespace com { namespace xuggle { namespace xuggler
{

/**
 * Get MetaData about a {@link IContainer} or {@link IStream}.
 * <p>
 * MetaData is a bag of key/value pairs that can be embedded
 * in some {@link IContainer} or some {@link IStream}
 * in an {@link IContainer}, and are then written to
 * or read from a media file.  Keys must be unique, and
 * any attempt to set a key value replaces any previous values.
 * </p>
 * <p>
 * An example is the "title" meta-data item in an MP3 file.
 * </p>
 * <p>
 * Support for IMetaData differs depending upon the {@link
 * IContainer} format you're using and the implementation
 * in <a href="http://www.ffmpeg.org/">FFmpeg</a>.  For example,
 * MP3 meta-data reading and writing is supported, but
 * (as of the writing of this comment) FLV meta-data writing
 * is not supported.
 * </p>
 */
class VS_API_XUGGLER IMetaData : public com::xuggle::ferry::RefCounted
{
public:
  typedef enum {
    METADATA_NONE=0,
    METADATA_MATCH_CASE=1,
//    METADATA_IGNORE_SUFFIX=2,
  } Flags;

  /**
   * Get the total number of keys currently in this
   * {@link IMetaData} object.
   * 
   * @return the number of keys.
   */
  virtual int32_t getNumKeys()=0;
  
  /**
   * Get the key at the given position, or null if no such
   * key at that position.
   * 
   * <p>
   * Note: positions of keys may change between
   * calls to {@link #setValue(String, String)} and 
   * should be requiried.
   * </p>
   * 
   * @param position The position.  Must be >=0 and < 
   * {@link #getNumKeys()}.
   * 
   * @return the key, or null if not found.
   */
  virtual const char* getKey(int32_t position)=0;

  /**
   * Get the value for the given key.
   * 
   * @param key The key
   * @param flag A flag for how to search
   * 
   * @return The value, or null if none.
   */
  virtual const char *getValue(const char* key, Flags flag)=0;
  
  /**
   * Sets the value for the given key to value.  This overrides
   * any prior setting for key, or adds key to the meta-data
   * if appropriate.
   * 
   * @param key The key to set.
   * @param value The value to set.
   */
  virtual int32_t setValue(const char* key, const char* value)=0;
  
  /**
   * Create a new {@link IMetaData} bag of properties with
   * no values set.
   */
  static IMetaData* make();
 
protected:
  IMetaData();
  virtual
  ~IMetaData();
};

}}}

#endif /* IMETADATA_H_ */

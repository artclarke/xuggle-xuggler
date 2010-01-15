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

/*
 * IIndexEntry.h
 *
 *  Created on: Jan 14, 2010
 *      Author: aclarke
 */

#ifndef IINDEXENTRY_H_
#define IINDEXENTRY_H_

#include <com/xuggle/ferry/RefCounted.h>
#include <com/xuggle/xuggler/Xuggler.h>

namespace com { namespace xuggle { namespace xuggler
{

/**
 * An index entry for a {@link IStream}.
 * @see IStream#getIndexEntry(int)
 * @see IStream#getNumIndexEntries()
 * @see IStream#getIndexEntries()
 * @since 3.4
 *
 */
class VS_API_XUGGLER IIndexEntry: public com::xuggle::ferry::RefCounted
{
public:
  /**
   * A bit mask value that may be set in {@link #getFlags}.
   */
  static const int32_t IINDEX_FLAG_KEYFRAME = 0x0001;

  /**
   * Create a new {@link IIndexEntry} with the specified
   * values.
   *
   * @param position The value to be returned from {@link #getPosition()}.
   * @param timeStamp The value to be returned from {@link #getTimeStamp()}.
   * @param flags The value to be returned from {@link #getFlags()}.
   * @param size The value to be returned from {@link #getSize()}.
   * @param minDistance The value to be returned from {@link #getMinDistance()}.
   */
  static IIndexEntry* make(int64_t position, int64_t timeStamp,
      int32_t flags, int32_t size, int32_t minDistance);

  /**
   * The position in bytes of the frame corresponding to this index entry
   * in the {@link IContainer}.
   * @return The byte-offset from start of the IContainer where the
   *   frame for this {@link IIndexEntry} can be found.
   */
  virtual int64_t getPosition()=0;
  /**
   * The actual time stamp, in units of {@link IStream#getTimeBase()}, of the frame this entry points to.
   * @return The time stamp for this entry.
   */
  virtual int64_t getTimeStamp()=0;
  /**
   * Flags set for this entry.  See the IINDEX_FLAG* constants
   * above.
   * @return the flags.
   */
  virtual int32_t getFlags()=0;

  /**
   * The size of bytes of the frame this index entry points to.
   * @return The size in bytes.
   */
  virtual int32_t getSize()=0;
  /**
   * Minimum number of index entries between this index entry
   * and the last keyframe in the index, used to avoid unneeded searching.
   * @return the minimum distance, in bytes.
   */
  virtual int32_t getMinDistance()=0;
  /**
   * Is this index entry pointing to a key frame.
   * Really shorthand for <code>{@link #getFlags()} &amp; {@link #IINDEX_FLAG_KEYFRAME}</code>.
   * @return True if this index entry is for a key frame.
   */
  virtual bool    isKeyFrame()=0;

protected:
  IIndexEntry();
  virtual ~IIndexEntry();
};

}}}
#endif /* IINDEXENTRY_H_ */

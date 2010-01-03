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
 * MetaData.h
 *
 *  Created on: Jun 29, 2009
 *      Author: aclarke
 */

#ifndef METADATA_H_
#define METADATA_H_

#include <com/xuggle/xuggler/IMetaData.h>
#include <com/xuggle/xuggler/FfmpegIncludes.h>
namespace com { namespace xuggle { namespace xuggler
{

class MetaData : public IMetaData
{
  VS_JNIUTILS_REFCOUNTED_OBJECT(MetaData);
public:
  virtual int32_t getNumKeys();
  virtual const char* getKey(int32_t position);

  virtual const char *getValue(const char* key, Flags flag);
  virtual int32_t setValue(const char* key, const char* value);
  
  /**
   * Create a MetaData object using metaDataToReference.
   * Once this is done, this MetaData object is responsible
   * for calling av_metadata_free(*metaDataToReference),
   * so take care.
   * 
   */
  static MetaData* make(AVMetadata ** metaDataToReference);
  
  /**
   * Copies all meta data currently in metaDataToCopy
   * and returns a new object.
   */
  static MetaData* make(AVMetadata* metaDataToCopy);
  
  /**
   * Destroys the current data, and copies all data
   * from copy.
   */
  int32_t copy(IMetaData* copy);

protected:
  MetaData();
  virtual
  ~MetaData();
private:
  AVMetadata** mMetaData;
  AVMetadata* mLocalMeta;
};

}

}

}

#endif /* METADATA_H_ */

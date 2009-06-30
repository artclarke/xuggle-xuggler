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
   * Copies all meta data currently in metaDataToCopy
   * and returns a new object.
   */
  static MetaData* make(AVMetadata* metaDataToCopy);
  
  /**
   * Takes the current contents of this object, and
   * copies them into dest, destroying any prior contents
   * in dest.
   */
  int32_t override(AVMetadata** dest);

protected:
  MetaData();
  virtual
  ~MetaData();
private:
  AVMetadata* mMetaData;
};

}

}

}

#endif /* METADATA_H_ */

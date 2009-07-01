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

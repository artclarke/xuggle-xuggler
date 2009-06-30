/*
 * MetaData.h
 *
 *  Created on: Jun 29, 2009
 *      Author: aclarke
 */

#ifndef METADATA_H_
#define METADATA_H_

#include <com/xuggle/xuggler/IMetaData.h>
namespace com
{

namespace xuggle
{

namespace xuggler
{

class MetaData : public IMetaData
{
  VS_JNIUTILS_REFCOUNTED_OBJECT_PRIVATE_MAKE(MetaData);
public:
  MetaData();
  virtual
  ~MetaData();
};

}

}

}

#endif /* METADATA_H_ */

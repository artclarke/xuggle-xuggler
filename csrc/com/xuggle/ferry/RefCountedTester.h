/*
 * RefCountedTester.h
 *
 *  Created on: Oct 17, 2008
 *      Author: aclarke
 */

#ifndef REFCOUNTEDTESTER_H_
#define REFCOUNTEDTESTER_H_

#include <com/xuggle/ferry/RefCounted.h>

namespace com {

namespace xuggle {

namespace ferry {


/**
 * This object exists in order for the ferry test
 * libraries to test the memory management functionality
 * of the RefCounted class from Java.
 *
 * It is NOT part of the public API.
 */
class RefCountedTester : public RefCounted
{
  VS_JNIUTILS_REFCOUNTED_OBJECT(RefCountedTester);
public:

  /**
   * Acquires a reference to a passed in object,
   * and returns the new object.  This, when wrapped
   * by Swig, will be wrapped in a new Java object.
   */
  static RefCountedTester *make(RefCountedTester *objToAcquire);

protected:
  RefCountedTester();
  virtual ~RefCountedTester();
};

}

}

}

#endif /* REFCOUNTEDTESTER_H_ */

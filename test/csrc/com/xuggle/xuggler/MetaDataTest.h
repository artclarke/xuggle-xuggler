/*
 * MetaDataTest.h
 *
 *  Created on: Jun 30, 2009
 *      Author: aclarke
 */

#ifndef METADATATEST_H_
#define METADATATEST_H_

#include <com/xuggle/testutils/TestUtils.h>
#include "Helper.h"
using namespace VS_CPP_NAMESPACE;


class MetaDataTest: public CxxTest::TestSuite
{
public:
  MetaDataTest();
  virtual
  ~MetaDataTest();
  void setUp();
  void tearDown();
  void testCreation();
  void testContainerGetMetaData();
  void testContainerSetMetaData();
  void testContainerGetMetaDataIsWriteThrough();
  void testStreamGetMetaData();
  void testStreamSetMetaData();
  void testStreamGetMetaDataIsWriteThrough();

};

#endif /* METADATATEST_H_ */

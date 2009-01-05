/*
 * RefCountedTester.cpp
 *
 *  Created on: Oct 17, 2008
 *      Author: aclarke
 */

#include "RefCountedTester.h"

namespace com {

namespace xuggle {

namespace ferry {

RefCountedTester :: RefCountedTester()
{
}

RefCountedTester :: ~RefCountedTester()
{
}

RefCountedTester*
RefCountedTester :: make(RefCountedTester * obj)
{
  if (obj) obj->acquire();
  return obj;
}


}

}

}

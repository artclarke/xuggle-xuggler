/*
 * This file is part of Xuggler.
 * 
 * Xuggler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 * 
 * Xuggler is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public
 * License along with Xuggler.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Error.h
 *
 *  Created on: Mar 20, 2009
 *      Author: aclarke
 */

#ifndef ERROR_H_
#define ERROR_H_

#include <com/xuggle/xuggler/IError.h>

namespace com { namespace xuggle { namespace xuggler
{

class Error : public IError
{
  VS_JNIUTILS_REFCOUNTED_OBJECT_PRIVATE_MAKE(Error);
public:
  virtual const char* getDescription();
  virtual int32_t getErrorNumber();
  virtual Type getType();
  
  static Error* make(int32_t errNo);
  static Error* make(Type type);
protected:
  Error();
  virtual ~Error();
  
private:
  static Error* make(int32_t errNo, Type type);
  
  static Type errorNumberToType(int32_t errorNo);
  static int32_t typeToErrorNumber(Type type);
  Type mType;
  int32_t mErrorNo;
  char mErrorStr[256];
  
};

}}}

#endif /* ERROR_H_ */

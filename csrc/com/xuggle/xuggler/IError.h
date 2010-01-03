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
 * IError.h
 *
 *  A way of translating between FFMPEG error codes and some xuggler defined
 *  values.
 *  
 *  Created on: Mar 20, 2009
 *      Author: aclarke
 */

#ifndef IERROR_H_
#define IERROR_H_

#include <com/xuggle/ferry/RefCounted.h>
#include <com/xuggle/xuggler/Xuggler.h>

namespace com { namespace xuggle { namespace xuggler
{

/**
 * Maps from int return codes to defined Error values.
 * <p>
 * This class is used to map from Xuggler return codes
 * (for example on {@link IContainer#readNextPacket(IPacket)}
 * into an enum type if the error is known and a text description.
 * </p><p>
 * WARNING: Do not write code that depends on the integer values
 * returned from Xuggler; instead use the integer value to create
 * one of these objects.  That's because integer values returned
 * from methods can have different meanings on different OS systems
 * (for example, ERROR_AGAIN is -11 on Linux, but a different
 * value on MacOS).  This class maps the error to a os-type-safe
 * value.
 * </p><p>
 * Also, do not depend on the string messages staying constant.  They
 * are for debugging purposes only.  And we can't control whether or
 * not they are localized -- that's up to your OS.  Sorry.
 * </p>
 */
class VS_API_XUGGLER IError : public com::xuggle::ferry::RefCounted
{
public:
  /**
   * A set of errors that Xuggler knows about.
   */
  typedef enum {
    /* Unknown error */
    ERROR_UNKNOWN,
    /* IO error */
    ERROR_IO,
    /* Number expected */
    ERROR_NUMEXPECTED,
    /* Invalid data */
    ERROR_INVALIDDATA,
    /* Out of memory */
    ERROR_NOMEM,
    /* Format not supported */
    ERROR_NOFMT,
    /* Operation not supported */
    ERROR_NOTSUPPORTED,
    /* File not found */
    ERROR_NOENT,
    /* End of file or pipe */
    ERROR_EOF,
    /* Value out of range */
    ERROR_RANGE,
    /* Try again; Not enough data available on this non-blocking call */
    ERROR_AGAIN,
    /* Not supported, and the FFMPEG team wouldn't mind a patch */
    ERROR_PATCHWELCOME,
    /* Thread was interrupted while trying an IO operation */
    ERROR_INTERRUPTED,
  } Type;
  

  /**
   * Get the OS-independent Xuggler type for this error.
   * 
   * @return the type.
   */
  virtual Type getType()=0;

  /**
   * Get a text description for this error.
   * 
   * The description returned will be in whatever language
   * the underlying OS decides to use, and no, we can't
   * support localization here if the OS hasn't already done it.
   * 
   * Sorry.
   * 
   * @return the description.
   */
  virtual const char* getDescription()=0;
  
  /**
   * Return the raw integer value that Xuggler returned and
   * was used to create this IError.
   * 
   * Note that this value can have different meanings on
   * different operating systems.  Use {@link #getType()}
   * instead for programmatic decisions.
   * 
   * @return the native error number.
   */
  virtual int32_t getErrorNumber()=0;
  
  /**
   * Create a new IError object from a return value passed in from Xuggler.
   * 
   * @param errorNumber The error number as returned from another
   *   Xuggler call.  ErrorNumber must be < 0.
   * @return a new IError, or null on error.
   */
  static IError* make(int32_t errorNumber);
  
  /**
   * Create a new IError object from an IError.Type enum value.
   * 
   * @param type The type to use for creation.
   * @return a new IError, or null on error.
   */
  static IError* make(Type type);

protected:
  IError();
  virtual ~IError();
};

}}}
#endif /* IERROR_H_ */

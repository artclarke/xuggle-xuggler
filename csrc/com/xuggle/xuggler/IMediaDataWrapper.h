/*
 * Copyright (c) 2008-2009 by Xuggle Inc. All rights reserved.
 *
 * It is REQUESTED BUT NOT REQUIRED if you use this library, that you let 
 * us know by sending e-mail to info@xuggle.com telling us briefly how you're
 * using the library and what you like or don't like about it.
 *
 * This library is free software; you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any later
 * version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along
 * with this library; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */
#ifndef IMEDIADATAWRAPPER_H_
#define IMEDIADATAWRAPPER_H_

#include <com/xuggle/xuggler/Xuggler.h>
#include <com/xuggle/xuggler/IMediaData.h>

namespace com { namespace xuggle { namespace xuggler {

/**
 * This class wraps an IMediaData object, but then allows you to set
 * new TimeStamps and TimeBases.
 * 
 * The underlying wrapped object's time stamps and time bases do not change.  This
 * can be useful when you need to use a IMediaObject in a time space that has
 * different time bases than the frame originally expected, and you don't
 * want to change the actual object.
 */
class VS_API_XUGGLER IMediaDataWrapper: public IMediaData
{
public:
#ifndef SWIG
  /**
   * Return the object being wrapped
   * 
   * @return the wrapped object
   */
  virtual IMediaData* get()=0;
#endif
  /**
   * Set an object to wrap, or null to release the old object.
   * 
   * @param aObj The object to wrap; null just releases the last object
   */
  virtual void wrap(IMediaData* aObj)=0;

  /**
   * Allows you to reset whether the wrapper things this is key or not.
   * 
   * Note the underlying wrapped object will continue to keep it's prior setting.
   * 
   * @param aIsKey The new key value.
   */
  virtual void setKey(bool aIsKey)=0;
  
  /**
   * Create a new IMediaDataWrapper object that wraps the given obj.
   * 
   * @param obj The object to wrap.
   * @return a new object or null on error.
   */
  static IMediaDataWrapper* make(IMediaData* obj);

  /*
   * Added for 1.23
   */

#ifndef SWIG
  /**
   * Gets the non IMediaDataWrapper object ultimately wrapped in this
   * wrapper, or null if there isn't one.
   * 
   * @return The non IMediaDataWrapper object ultimately wrapped
   */
  virtual IMediaData* unwrap()=0;
#endif
  
protected:
  IMediaDataWrapper();
  virtual ~IMediaDataWrapper();
};

}}}

#endif /* IMEDIADATAWRAPPER_H_ */

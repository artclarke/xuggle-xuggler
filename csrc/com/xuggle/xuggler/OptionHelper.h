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
#ifndef OPTIONHELPER_H_
#define OPTIONHELPER_H_

namespace com { namespace xuggle { namespace xuggler {

  class OptionHelper
  {
  public:
    OptionHelper();
    virtual ~OptionHelper();

    /**
     * Looks up the property 'name' in 'context' and sets the
     * value of the property to 'value'.
     * 
     * @param context AVClass to search for option in.
     * @param name name of option
     * @param value Value of option
     * 
     * @return >= 0 on success; <0 on error.
     */
    static int32_t setProperty(void * context, const char* name, const char* value);
    
    /**
     * Gets the value of this property, and returns as a new[]ed string.
     * 
     * Caller must call delete[] on string.
     * 
     * @param context AVClass context to search for option in.
     * @param name name of option
     * 
     * @return string version of option value (caller must call delete[]) or
     *   null on error.
     */
    static char * getPropertyAsString(void * context, const char* name);


  };

}}}
#endif /* OPTIONHELPER_H_ */

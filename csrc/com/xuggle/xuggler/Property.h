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

#ifndef PROPERTY_H_
#define PROPERTY_H_

#include <com/xuggle/xuggler/IProperty.h>
#include <com/xuggle/xuggler/IRational.h>

extern "C" {
#include <libavcodec/opt.h>
}

namespace com { namespace xuggle { namespace xuggler {

  /**
   * A wrapper for an FFMPEG AVOption value.
   */
  class Property : public com::xuggle::xuggler::IProperty
  {
    VS_JNIUTILS_REFCOUNTED_OBJECT_PRIVATE_MAKE(Property);
  public:
    static Property *make(const AVOption *start, const AVOption *option);

  public:
    /**
     * IProperty implementation
     */
    virtual const char *getName();
    virtual const char *getHelp();
    virtual const char *getUnit();
    virtual Type getType();
    virtual int64_t getDefault();
    virtual double getDefaultAsDouble();
    virtual int32_t getFlags();
    virtual int32_t getNumFlagSettings();
    virtual IProperty *getFlagConstant(int32_t position);
    virtual IProperty *getFlagConstant(const char* name);
    

    /** For internal use */
    /**
     * Returns the total number of settable properties on this object
     * 
     * @param context AVClass to search for options in
     * 
     * @return total number of options (not including constant definitions)
     */
    static int32_t getNumProperties(void *context);
    
    /**
     * Returns the metadata for the numbered property.
     * 
     * Note that a property number is guaranteed to stay constant during this run
     * of Xuggler, but the property number may change if the process restarts.
     * 
     * @param context AVClass to search for options in
     * @param propertyNo The property number in the options list.
     *   
     * @return the meta data for this property, or null if not found
     */
    static IProperty *getPropertyMetaData(void *context, int32_t propertyNo);
    
    /**
     * Returns the metadata for the named property.

     * @param context AVClass to search for options in
     * @param name The property number in the options list.
     *   
     * @return the meta data for this property, or null if not found
     */
    static IProperty *getPropertyMetaData(void *context, const char* name);
        
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
     * Looks up the property 'name' in 'context' and sets the
     * value of the property to 'value'.
     * 
     * @param context AVClass to search for option in.
     * @param name name of option
     * @param value Value of option
     * 
     * @return >= 0 on success; <0 on error.
     */
    static int32_t setProperty(void * context, const char* name, double value);
    
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
    static int32_t setProperty(void * context, const char* name, int64_t value);
    
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
    static int32_t setProperty(void * context, const char* name, bool value);
    
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
    static int32_t setProperty(void * context, const char* name, IRational *value);

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

    /**
     * Gets the value of this property, and returns as a double;
     * 
     * @param context AVClass context to search for option in.
     * @param name name of option
     * 
     * @return double value of property, or 0 on error.
     */
    static double getPropertyAsDouble(void * context, const char* name);

    /**
     * Gets the value of this property, and returns as an long;
     * 
     * @param context AVClass context to search for option in.
     * @param name name of option
     * 
     * @return long value of property, or 0 on error.
     */
    static int64_t getPropertyAsLong(void * context, const char* name);

    /**
     * Gets the value of this property, and returns as an IRational;
     * 
     * @param context AVClass context to search for option in.
     * @param name name of option
     * 
     * @return long value of property, or 0 on error.
     */
    static IRational *getPropertyAsRational(void * context, const char* name);

    /**
     * Gets the value of this property, and returns as a boolean
     * 
     * @param context AVClass context to search for option in.
     * @param name name of option
     * 
     * @return boolean value of property, or false on error.
     */
    static bool getPropertyAsBoolean(void * context, const char* name);
  protected:
    Property();
    virtual
    ~Property();
  private:
    const AVOption *mOption;
    const AVOption *mOptionStart;
  };

}}}
#endif /* PROPERTY_H_ */

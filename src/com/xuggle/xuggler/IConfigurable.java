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
package com.xuggle.xuggler;

/**
 * Methods that implement this interface can be configuring
 * using setProperty and getProperty methods, and expose
 * {@link IProperty} meta data about their properties.
 * 
 * @author aclarke
 *
 */
public interface IConfigurable
{
  /**
   * Returns the total number of settable properties on this object  
   * @return      total number of options (not including constant definitions) 
   *                
   */
  public int getNumProperties();

  /**
   * Returns the name of the numbered property.  
   * @param       propertyNo The property number in the options list.  
   * @return      an IProperty value for this properties meta-data  
   */
  public IProperty getPropertyMetaData(int propertyNo);

  /**
   * Returns the name of the numbered property.  
   * @param       name The property name.  
   * @return      an IProperty value for this properties meta-data  
   */
  public IProperty getPropertyMetaData(String name);

  /**
   * Sets a property on this Object.  
   * All AVOptions supported by the underlying AVClass are supported. 
   *  
   * @param       name The property name. For example "b" for bit-rate.  
   * @param       value The value of the property.  
   * @return      >= 0 if the property was successfully set; <0 on error  
   */
  public int setProperty(String name, String value);

  /**
   * Looks up the property 'name' and sets the  
   * value of the property to 'value'.  
   * @param       name name of option  
   * @param       value Value of option  
   * @return      >= 0 on success; <0 on error.  
   */
  public int setProperty(String name, double value);

  /**
   * Looks up the property 'name' and sets the  
   * value of the property to 'value'.  
   * @param       name name of option  
   * @param       value Value of option  
   * @return      >= 0 on success; <0 on error.  
   */
  public int setProperty(String name, long value);

  /**
   * Looks up the property 'name' and sets the  
   * value of the property to 'value'.  
   * @param       name name of option  
   * @param       value Value of option  
   * @return      >= 0 on success; <0 on error.  
   */
  public int setProperty(String name, boolean value);

  /**
   * Looks up the property 'name' and sets the  
   * value of the property to 'value'.  
   * @param       name name of option  
   * @param       value Value of option  
   * @return      >= 0 on success; <0 on error.  
   */
  public int setProperty(String name, IRational value);

  /**
   * Gets a property on this Object.  
   * Note for C++ callers; you must free the returned array with  
   * delete[] in order to avoid a memory leak. Other language  
   * folks need not worry.  
   * @param       name property name  
   * @return      an string copy of the option value, or null if the option 
   *               doesn't exist.  
   */
  public String getPropertyAsString(String name);

  /**
   * Gets the value of this property, and returns as a double;  
   * @param       name name of option  
   * @return      double value of property, or 0 on error.  
   */
  public double getPropertyAsDouble(String name);

  /**
   * Gets the value of this property, and returns as an long;  
   * @param       name name of option  
   * @return      long value of property, or 0 on error.  
   */
  public long getPropertyAsLong(String name);

  /**
   * Gets the value of this property, and returns as an IRational;  
   * @param       name name of option  
   * @return      long value of property, or 0 on error.  
   */
  public IRational getPropertyAsRational(String name);

  /**
   * Gets the value of this property, and returns as a boolean  
   * @param       name name of option  
   * @return      boolean value of property, or false on error.  
   */
  public boolean getPropertyAsBoolean(String name);


}

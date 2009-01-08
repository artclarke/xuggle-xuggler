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
#ifndef LOGGERSTACK_H_
#define LOGGERSTACK_H_

#include <com/xuggle/ferry/Logger.h>

namespace com { namespace xuggle { namespace ferry
  {

  /**
   * This class records the Global logging levels when it is created,
   * and, if it changes the Global logging level, restores it to the
   * original values when it is destroyed.
   *
   * It is handy to use in tests where you know a log (and hence error
   * message) will be generated, but you want to temporarily turn off
   * logging when the test runs.
   */
  class VS_API_FERRY LoggerStack
  {
  public:
    LoggerStack();
    virtual ~LoggerStack();
    /**
     * If false, sets level and all lower levels to false.
     *
     * If true, sets current level to true, and all lower levels to original
     * value.
     *
     * @param level The level to set.
     * @param value Whether to log (true) or not (false).
     *
     */
    void setGlobalLevel(Logger::Level level, bool value);
  private:
    bool mHasChangedLevel[5];
    bool mOrigLevel[5];
  };

  }}}

#endif /*LOGGERSTACK_H_*/

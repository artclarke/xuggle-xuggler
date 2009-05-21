/*
 * Copyright (c) 2008, 2009 by Xuggle Incorporated. All rights reserved.
 * 
 * This file is part of Xuggler.
 * 
 * You can redistribute Xuggler and/or modify it under the terms of the GNU
 * Affero General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 * 
 * Xuggler is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Affero General Public License for more
 * details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with Xuggler. If not, see <http://www.gnu.org/licenses/>.
 */

package com.xuggle.ferry;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.xuggle.ferry.NativeLogger;

/**
 * Internal Only.
 * <p>
 * This object is called by Native code when it logs inside a JVM. It forwards
 * to our SLF4J logger.
 * </p>
 * <p>
 * <strong>Do not change the signatures of methods in this class without
 * changing the corresponding native code that depends on it.</strong>
 * </p>
 * 
 */
public class NativeLogger
{
  // This is a logger this class can use for its own logging
  private static final Logger log = LoggerFactory.getLogger(NativeLogger.class);

  public static NativeLogger getLogger(String loggerName)
  {
    log.trace("Native code asked for logger: {}", loggerName);
    NativeLogger retval = null;
    Logger logger = LoggerFactory.getLogger(loggerName);
    if (logger != null)
      retval = new NativeLogger(logger);
    return retval;
  }

  private final Logger mLogger;

  private NativeLogger(Logger logger)
  {
    mLogger = logger;
  }

  public String getName()
  {
    return mLogger.getName();
  }

  public boolean log(int level, String msg)
  {
    boolean didLog = false;
    log.trace("log level: {}, msg: {}", level, msg);

    switch (level)
    {
      case 1:
        if (mLogger.isWarnEnabled())
        {
          mLogger.warn(msg);
          didLog = true;
        }
        break;
      case 2:
        if (mLogger.isInfoEnabled())
        {
          mLogger.info(msg);
          didLog = true;
        }
        break;
      case 3:
        if (mLogger.isDebugEnabled())
        {
          mLogger.debug(msg);
          didLog = true;
        }
        break;
      case 4:
        if (mLogger.isTraceEnabled())
        {
          mLogger.trace(msg);
          didLog = true;
        }
        break;
      case 0:
      default: // treat default as Error
        if (mLogger.isErrorEnabled())
        {
          mLogger.error(msg);
          didLog = true;
        }
        break;

    }

    return didLog;
  }

}

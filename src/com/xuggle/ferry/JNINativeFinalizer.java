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

package com.xuggle.ferry;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Internal Only.
 * 
 * This class is held on to by the RefCounted classes and nulled when that object
 * is collected.  It has no references to any (non static) members and so its
 * finalizer will not hold up the collection of any other object.
 * <p>
 * It exists so that we still have a mechanism that always frees native memory; in
 * most cases the JNIWeakReference will enqueue correctly, and then the next
 * call to a jni_utils based method will drain that queue, but sometimes there is
 * no extra call to one of those methods; in this case we'll drain the queue
 * when this gets finalized. 
 * </p><p>
 * It does a GC which might race with the GC that a JNIWeakReference does on
 * allocation of a new object but that's ok.
 * </p>
 * @author aclarke
 *
 */
public class JNINativeFinalizer
{
  private static final Logger log = LoggerFactory.getLogger(JNINativeFinalizer.class);
  static { log.trace("static <init>"); }

  protected void finalize()
  {
    //log.debug("finalize");
    JNIWeakReference.getMgr().gc();
  }
}

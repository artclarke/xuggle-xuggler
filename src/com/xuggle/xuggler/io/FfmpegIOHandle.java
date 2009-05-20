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

package com.xuggle.xuggler.io;


import com.xuggle.xuggler.io.FfmpegIO;
import com.xuggle.ferry.JNIPointerReference;

/**
 * For Internal Use Only.
 * A Handle that the JavaFFMPEGIO native code can use.  It
 * is completely opaque to callers, but is used by the {@link FfmpegIO}
 * class to pass around native FFMPEG pointers.
 */
public class FfmpegIOHandle extends JNIPointerReference
{
  // we create this class just to get get type-warnings
  // from Java code.  no additional functionality over the
  // super class is expected.
}

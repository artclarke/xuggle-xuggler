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
package com.xuggle.xuggler;

import java.util.List;

import com.xuggle.xuggler.ICodec;
import com.xuggle.xuggler.IContainerFormat;
import com.xuggle.xuggler.ICodec.ID;

/**
 * Prints information about which codecs can be inserted into a container format.
 * @author aclarke
 *
 * @since 3.3
 */
public class GetSupportedCodecs
{

  /**
   * Given the short name of a container, prints out information about
   * it, including which codecs Xuggler can write (mux) into that container.
   * @param args One string argument representing the short name of the format
   */
  public static void main(String[] args)
  {
    if (args.length != 1) {
      System.err.println("Usage: program_name container_short_name");
      return;
    }
    IContainerFormat format = IContainerFormat.make();
    format.setOutputFormat(args[0], null, null);

    List<ID> codecs = format.getOutputCodecsSupported();

    System.out.println("Container Format: "+format);
    System.out.println();
    System.out.println("Total codecs supported: " + format.getOutputNumCodecsSupported());
    System.out.println("Supported Codecs:");
    for(ID id : codecs) {
      if (id != null) {
        ICodec codec = ICodec.findEncodingCodec(id);
        if (codec != null && codec.canEncode()) {
          System.out.println(codec);
        }
      }
    }    
  }

}

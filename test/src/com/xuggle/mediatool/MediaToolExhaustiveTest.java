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
package com.xuggle.mediatool;

import static org.junit.Assert.*;

import java.util.Collection;
import java.util.LinkedList;

import javax.swing.WindowConstants;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;
import org.junit.runners.Parameterized.Parameters;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.xuggle.ferry.JNIMemoryManager;
import com.xuggle.ferry.JNIMemoryManager.MemoryModel;
import com.xuggle.mediatool.MediaReader;
import com.xuggle.mediatool.MediaViewer;
import com.xuggle.mediatool.MediaWriter;

/**
 * Tests the MediaWriter and MediaReader (and optionally the
 * MediaViewer class) with all supported memory models.
 * 
 * Uses a long file because some issues should only show up
 * then.
 * 
 * @author aclarke
 *
 */
@RunWith(Parameterized.class)
public class MediaToolExhaustiveTest
{
  private final Logger log = LoggerFactory.getLogger(this.getClass());

  @Parameters
  public static Collection<Object[]> getModels()
  {
    Collection<Object[]> retval = new LinkedList<Object[]>();
    // add all the models.
    final boolean TEST_ALL=true;
    if (TEST_ALL) {
      for (JNIMemoryManager.MemoryModel model :
        JNIMemoryManager.MemoryModel.values())
        for(boolean doWrite : new boolean[]{true,false})
          for(boolean doViewer: new boolean[]{true, false})
          retval.add(new Object[]{
              model, "fixtures/testfile.flv", 1, doWrite, doViewer
          }
          );
    }
    else {
      retval.add(new Object[]{
          JNIMemoryManager.MemoryModel.JAVA_DIRECT_BUFFERS,
//          "fixtures/testfile_videoonly_20sec.flv", // a short file
          "fixtures/testfile.flv",
          1,
          true,
          true
      });
    }
    return retval;
  }
  final MediaViewer.Mode mViewerMode = IMediaViewer.Mode.valueOf(
      System.getProperty(this.getClass().getName() + ".ViewerMode", 
        //IMediaViewer.Mode.AUDIO_VIDEO.name()
        MediaViewer.Mode.DISABLED.name()
        ));

  private final MemoryModel mModel;
  private final int mIterations;
  private final String mURL;
  private final boolean mDoWrite;
  private final boolean mDoViewer;
  public MediaToolExhaustiveTest
  (
      JNIMemoryManager.MemoryModel model,
      String url,
      int numIterations,
      boolean doWrite,
      boolean runViewer
  )
  {
    mURL = url;
    mIterations = numIterations;
    mModel = model;
    mDoWrite = doWrite;
    mDoViewer = runViewer;
    log.debug("url: {}; iterations: {}; model: {}; write: {}; view: {}",
        new Object[]{
        mURL,
        mIterations,
        mModel,
        mDoWrite,
        mDoViewer
    });
  }
  
  @Test
  public void processFile()
  {
    read(mModel, mURL, mIterations, mDoWrite, mDoViewer, mViewerMode);
    JNIMemoryManager.getMgr().flush();
  }
  
  public static void read(IMediaReader reader, int numIterations)
  {
  }
  public static void read(JNIMemoryManager.MemoryModel model,
      String url, int numIterations, boolean doWrite, boolean doViewer,
      MediaViewer.Mode viewerMode)
  {
    JNIMemoryManager.setMemoryModel(model);
    for(int i = 0; i < numIterations; i++)
    {
      MediaReader reader = new MediaReader(url);
      MediaWriter writer = null;
      assertNotNull(reader);
      if (doWrite) {
        String outURL =
          MediaToolExhaustiveTest.class.getName()+"_"+
          model + "_" +
          i + ".mov";
        // adds itself to the reader
        writer = new MediaWriter(outURL, reader);
        reader.addListener(writer);
      }
      if (doViewer)
      {
        MediaViewer viewer = new MediaViewer(viewerMode,
            true, 
            WindowConstants.EXIT_ON_CLOSE);
        reader.addListener(viewer);
          
      }
      while(reader.readPacket() == null)
        /* continue */ ;
    }
  }
}

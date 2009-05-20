/*
 * Copyright (c) 2008, 2009 by Xuggle Incorporated.  All rights reserved.
 * 
 * This file is part of Xuggler.
 * 
 * You can redistribute Xuggler and/or modify it under the terms of the GNU
 * Affero General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any
 * later version.
 * 
 * Xuggler is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public
 * License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with Xuggler.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * ContainerParameters.h
 *
 *  Created on: Mar 20, 2009
 *      Author: aclarke
 */

#ifndef ContainerParameters_H_
#define ContainerParameters_H_

#include <com/xuggle/xuggler/IContainerParameters.h>
#include <com/xuggle/xuggler/FfmpegIncludes.h>

namespace com { namespace xuggle { namespace xuggler
{

class ContainerParameters : public IContainerParameters
{
  VS_JNIUTILS_REFCOUNTED_OBJECT(ContainerParameters);
public:
  virtual IRational* getTimeBase();
  virtual void setTimeBase(IRational* base);

  virtual int32_t getAudioSampleRate();
  virtual void setAudioSampleRate(int32_t sampleRate);
  
  virtual int32_t getAudioChannels();
  virtual void setAudioChannels(int32_t channels);

  virtual int32_t getVideoWidth();
  virtual void setVideoWidth(int32_t width);
  
  virtual int32_t getVideoHeight();
  virtual void setVideoHeight(int32_t height);

  virtual IPixelFormat::Type getPixelFormat();
  virtual void setPixelFormat(IPixelFormat::Type type);

  virtual int32_t getTVChannel();
  virtual void setTVChannel(int32_t channel);
  
  virtual const char* getTVStandard();
  virtual void setTVStandard(const char* standard);

  virtual bool isMPEG2TSRaw();
  virtual void setMPEG2TSRaw(bool setting);
  
  virtual bool isMPEG2TSComputePCR();
  virtual void setMPEG2TSComputePCR(bool setting);
  
  virtual bool isInitialPause();
  virtual void setInitialPause(bool setting);
  
  // Not for calling from Java
  virtual AVFormatParameters* getAVParameters();
protected:
  ContainerParameters();
  virtual ~ContainerParameters();
  
private:
  AVFormatParameters mParameters;
  char mStandard[256];
};

}}}

#endif /* ContainerParameters_H_ */

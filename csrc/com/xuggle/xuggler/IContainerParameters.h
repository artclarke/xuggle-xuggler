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
 * IContainerParameters.h
 *
 *  Created on: Mar 20, 2009
 *      Author: aclarke
 */

#ifndef IContainerParameters_H_
#define IContainerParameters_H_

#include <com/xuggle/ferry/RefCounted.h>
#include <com/xuggle/xuggler/Xuggler.h>
#include <com/xuggle/xuggler/IRational.h>
#include <com/xuggle/xuggler/IPixelFormat.h>

namespace com { namespace xuggle { namespace xuggler
{

/**
 * Parameters that can be set on a {@link IContainer} before opening.
 * <p>
 * Normally this class can be ignored, but for some container formats, such
 * as webcams, you need to specify additional meta-data information for
 * the container.  Examples include the TimeBase (e.g. frame rate),
 * and video width and height.
 * </p>
 * @see IContainer#setParameters(IContainerParameters)
 * 
 */
class VS_API_XUGGLER IContainerParameters : public com::xuggle::ferry::RefCounted
{
public:
  /**
   * Get the time base.  For container formats such as
   * webcameras, this is used as the requested frame rate.
   * 
   * @return the time base.
   */
  virtual IRational* getTimeBase()=0;
  
  /**
   * Set the time base.
   * 
   * @param base The time base.  Ignored if null.
   * 
   * @see #getTimeBase()
   */
  virtual void setTimeBase(IRational* base)=0;

  /**
   * Get the requested audio sample rate.
   * 
   * For some container formats, such as Alsa Microphones,
   * this is the sample-rate you want the mic to use.
   * 
   * @return the audio sample rate.
   */
  virtual int32_t getAudioSampleRate()=0;
  
  /**
   * Set the requested audio sample rate.
   * 
   * @param sampleRate the sample rate.
   * 
   * @see #getAudioSampleRate() 
   */
  virtual void setAudioSampleRate(int32_t sampleRate)=0;
  
  /**
   * Get the requested audio channels.
   * 
   * For some container formats, such as DV cameras,
   * this is the number of audio channels you'd like to get
   * samples for.
   * 
   * @return the audio channels.
   */
  virtual int32_t getAudioChannels()=0;
  
  /**
   * Set the requested audio channels.
   * 
   * @param channels The number of channels.
   * 
   * @see #getAudioChannels()
   */
  virtual void setAudioChannels(int32_t channels)=0;

  /**
   * Get the requested video width.
   * 
   * For some container formats, such as VFW (Video For Windows)
   * this is the requested webcam video capture width.
   * 
   * @return the video width.
   */
  virtual int32_t getVideoWidth()=0;
  
  /**
   * Set the requested video width.
   * 
   * @param width The width in pixels.
   * 
   * @see #getVideoWidth()
   */
  virtual void setVideoWidth(int32_t width)=0;
  
  /**
   * Get the requested video height.
   * 
   * For some container formats, such as VFW (Video For Windows)
   * this is the requested webcam video capture height.
   * 
   * @return the video height.
   */
  virtual int32_t getVideoHeight()=0;

  /**
   * Set the requested video height.
   * 
   * @param height The height in pixels.
   * 
   * @see #getVideoHeight()
   */
  virtual void setVideoHeight(int32_t height)=0;

  /**
   * Get the requested video pixel format.
   * 
   * For some container formats, such as VFW (Video For Windows)
   * this is the requested webcam video capture pixel format.
   * 
   * @return the video pixel format.
   */
  virtual IPixelFormat::Type getPixelFormat()=0;

  /**
   * Set the requested video pixel format.
   * 
   * @param type The pixel format.
   * 
   * @see #getPixelFormat()
   */
  virtual void setPixelFormat(IPixelFormat::Type type)=0;

  /**
   * The TV channel requested.
   * 
   * Some container formats (well devices really) can have
   * a channel you're requesting.  This is it.
   * 
   * @return the TV channel.
   */
  virtual int32_t getTVChannel()=0;
  
  /**
   * Set the tv channel.
   * 
   * @param channel the tv channel
   * 
   * @see #getTVChannel()
   */
  virtual void setTVChannel(int32_t channel)=0;
  
  /**
   * Get the TV Standard.
   * 
   * For some container formats representing television tuners,
   * this is the tv standard they will get data as (e.g.
   * NTSC, PAL, SECAM).
   * 
   * @return the TV standard
   */
  virtual const char* getTVStandard()=0;
  
  /**
   * Set the TV Standard.
   * 
   * @param standard The TV standard.  Empty strings are treated
   *   as null strings.
   *   
   * @see #getTVStandard()
   */
  virtual void setTVStandard(const char* standard)=0;

  /**
   * Are we forcing raw MPEG-2 transport stream output.
   * @return is MPEG-2 transport stream raw 
   */
  virtual bool isMPEG2TSRaw()=0;
  
  /**
   * Force raw MPEG-2 transport stream output, if possible. 
   * @param setting true to try forcing; false to not.
   */
  virtual void setMPEG2TSRaw(bool setting)=0;
  
  /**
   * Are we computing PCR for each transport stream packet?
   * 
   * Only really meaningful if {@link #isMPEG2TSRaw()} is true.
   * 
   * @return computing PCR for each transport stream packet.
   */
  virtual bool isMPEG2TSComputePCR()=0;
  
  /**
   * Compute exact PCR for each transport stream packet
   *  (only meaningful if {@link #isMPEG2TSRaw()} is true).
   * @param setting true for true; false for false.
   */
  virtual void setMPEG2TSComputePCR(bool setting)=0;

  /**
   * If RTSP, will we pause the stream immediately?
   * @return true for yes, false for no.
   */
  virtual bool isInitialPause()=0;
  
  /**
   * Should we pause the RTSP stream on open or immediately start?
   * @param setting true to pause; false to not.
   */
  virtual void setInitialPause(bool setting)=0;
  
  /**
   * Create a new parameters object with default settings.
   * 
   * @return A new object, or null on error.
   */
  static IContainerParameters* make();
protected:
  IContainerParameters();
  virtual ~IContainerParameters();
};

}}}

#endif /* IContainerParameters_H_ */

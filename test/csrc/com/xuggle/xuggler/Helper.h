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

#ifndef HELPER_H_
#define HELPER_H_

#include <com/xuggle/ferry/RefPointer.h>
#include <com/xuggle/ferry/Logger.h>
#include <com/xuggle/ferry/LoggerStack.h>

#include <com/xuggle/xuggler/IAudioResampler.h>
#include <com/xuggle/xuggler/IVideoResampler.h>
#include <com/xuggle/xuggler/IContainer.h>
#include <com/xuggle/xuggler/IContainerFormat.h>
#include <com/xuggle/xuggler/IPacket.h>
#include <com/xuggle/xuggler/ICodec.h>
#include <com/xuggle/xuggler/IPixelFormat.h>
#include <com/xuggle/xuggler/IStream.h>
#include <com/xuggle/xuggler/IStreamCoder.h>
#include <com/xuggle/xuggler/ICodec.h>

// Let everyone in this packet use the ferry
using namespace com::xuggle::ferry;

namespace com { namespace xuggle { namespace xuggler
{

  class Helper
  {
  public:
    const char * FIXTURE_DIRECTORY;
    const char * SAMPLE_FILE;
    int expected_num_streams;
    int expected_packets;
    int expected_video_packets;
    int expected_video_frames;
    int expected_video_key_frames;
    int expected_audio_packets;
    int expected_audio_samples;
    double expected_frame_rate;
    double expected_time_base;
    int expected_width;
    int expected_height;
    int expected_gops;
    IPixelFormat::Type expected_pixel_format;
    int expected_bit_rate;
    int expected_sample_rate;
    int expected_channels;
    ICodec::ID *expected_codec_ids;
    ICodec::Type *expected_codec_types;
        
    RefPointer<IContainer> container;
    RefPointer<IContainerFormat> format;
    RefPointer<IPacket> packet;
    int packetOffset;
    RefPointer<IStream> * streams;
    int num_streams;
    RefPointer<IStreamCoder> * coders;
    int num_coders;
    RefPointer<ICodec> *codecs;
    int num_codecs;
    RefPointer<IVideoResampler>* vresamplers;
    int num_vresamplers;
    RefPointer<IAudioResampler>* aresamplers;
    int num_aresamplers;
    
    int first_output_video_stream;
    int first_output_audio_stream;
    int first_input_video_stream;
    int first_input_audio_stream;

    bool first_output_video_coder_open;
    bool first_output_audio_coder_open;
    bool first_input_video_coder_open;
    bool first_input_audio_coder_open;

    bool container_isopen;
    bool need_trailer_write;
    
    void reset();

    /**
     * Resets the helper object to help with reading files.
     * 
     * Allocates a container, format, codec and packet.
     * Opens the input URL in READ mode with a null ContainerFormat context.
     * Finds the number of input streams and fills out the streams,
     * coders, and codes arrays.
     * 
     * @param url       The URL of the resource to open.
     * 
     * @exception ::tut::exception Throws a Tut exception if anything untowards happens.
     */
    void setupReading(const char* url);

    /**
     * Resets the helper object to help with writing files.
     * 
     * Allocates a container, codec and packet.
     * Looks for a containerformat of the specified short-name.
     * Opens the output URL in WRITE mode with the specified container format.
     * Adds 1 audio stream if a codec's shortname is specified.
     * Adds 1 video stream if a codec's shortname is specified.
     * 
     * @param url       The URL of the resource to write.
     * @param videoCodec The shortname of the video codec, or 0 or "" for none.
     * @param audioCodec The shortname of the audio codec, or 0 or "" for none.
     * @param format    The shortname of the format, or 0 or "" for guess format.
     * 
     * @exception ::tut::exception Throws a Tut exception if anything untowards happens.
     */
    void setupWriting(const char* url,
        const char* videoCodec,
        const char* audioCodec,
        const char* format);
    
    /**
     * Resets the helper object to help with writing files.
     * 
     * Allocates a container, codec and packet.
     * Looks for a containerformat of the specified short-name.
     * Opens the output URL in WRITE mode with the specified container format.
     * Adds 1 audio stream if a codec's shortname is specified.
     * Adds 1 video stream if a codec's shortname is specified.
     * 
     * @param url       The URL of the resource to write.
     * @param videoCodec The shortname of the video codec, or 0 or "" for none.
     * @param audioCodec The shortname of the audio codec, or 0 or "" for none.
     * @param format    The shortname of the format, or 0 or "" for guess format.
     * @param pixelformat The Pixel format to use for video coders
     * @param width The picture width to use for video coders
     * @param height The picture height to use for video coders
     * @param allocateVideoResamplers Let the Helper allocate video resamplers
     * @param sampleRate The audio sample rate to use for audio coders
     * @param channels The number of audio channels to use for audio coders
     * @param allocateAudioResamplers Let the Helper allocate audio resamplers
     *  
     * @exception ::tut::exception Throws a Tut exception if anything untowards happens.
     */
    void setupWriting(
        const char* url,
        const char* videoCodec,
        const char* audioCodec,
        const char* format,
        IPixelFormat::Type pixelformat,
        int width,
        int height,
        bool allocateVideoResamplers,
        int sampleRate,
        int channels,
        bool allocateAudioResamplers);

    /**
     * Decodes the stream in an input helper, and then encodes
     * the frames and outputs as specified by the output helper.
     * 
     * We assume that setupReading and setupWriting have been
     * called on inputHelper and outputHelper respectively before
     * doing work.
     * 
     * @param inputHelper The helper we assume we're reading/decoding from.
     * @param outputHelper The helper we assume we're writing/encoding to.
     * @param maxSecondsToEncode Max seconds worth of content to output.
     * @param frameListener If non null, this function will call this back
     *   every time it is about to encode a frame.  The function can write 
     *   data (but not change the size) in the pOutFrame.
     * @param frameListenerCtx A pointer we'll pass back into the frameListener
     *   function.
     * @param sampleListener If non null, this function will call this back
     *   every time it is about to encode some audio samples.  The function can
     *   write (e.g. resample) the data in the output frame if desired.
     * @param sampleListenerCtx A pointer we'll pass back into the sampleListener
     *   function.
     * 
     * @warning If you specify a call back, this function ASSUMES you want
     *   to change the data, and doesn't copy the contents of pInFrame into
     *   pOutFrame.  You must do that.
     * 
     * @warning If the output width, size, pixel format, etc. is different
     *   than the input, you need to define call backs that do the resizing
     *   or resamplings on your behalf.
     */
    static void decodeAndEncode(
        Helper& inputHelper,
        Helper& outputHelper,
        int maxSecondsToEncode,
        void (*frameListener)(void* context, IVideoPicture* pOutFrame, IVideoPicture* pInFrame),
        void *frameListenerCtx,
        void (*sampleListener)(void* context, IAudioSamples* pOutSamples, IAudioSamples *pInSamples),
        void *sampleListenerCtx
        );
        
    /**
     * Get next decoded object in stream.
     * 
     * Assuming the stream is setup for reading, this function gets the
     * next VideoPicture or Samples and returns it.
     * 
     * @param pOutSamples The samples buffer to return items into;  If null,
     *   samples are dropped.
     * @param pOutFrame The audio frame to write items into.  If null, the
     *   frame is dropped.
     * 
     * @return 0 means end of file was reached.  Otherwise 1 means audio was
     *   returned.  2 means video was returned.
     * 
     * @exception ::tut::exception Throws a Tut exception if anything untowards happens.
     * 
     */
    int readNextDecodedObject(IAudioSamples* pOutSamples, IVideoPicture* pOutFrame);
    
    /**
     * Write this frame to the first video stream in the output file.
     * 
     * @param pInFrame The frame to write to the video stream.
     * 
     * @return >= 0 on success; < 0 on error.
     */
    int writeFrame(IVideoPicture* pInFrame);

    /**
     * Write this set of samples to the first audio stream in the output file.
     * 
     * @param pInSamples The samples to write to the video stream.
     * 
     * @return >= 0 on success; < 0 on error.
     */
    int writeSamples(IAudioSamples* pInSamples);
    
    const char* getSamplePath() { return mSampleFile; }
    Helper();
    virtual ~Helper();
    
  private:
    void allocateStreams(int num_streams);
    void resetStreams();

    char mFixtureDir[4098];
    char mSampleFile[4098];
  };

}}}

#endif /*HELPER_H_*/

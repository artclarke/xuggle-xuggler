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

#ifndef ICODEC_H_
#define ICODEC_H_

#include <com/xuggle/ferry/RefCounted.h>
#include <com/xuggle/xuggler/Xuggler.h>
#include <com/xuggle/xuggler/IRational.h>
#include <com/xuggle/xuggler/IPixelFormat.h>
#include <com/xuggle/xuggler/IAudioSamples.h>
namespace com
{
namespace xuggle
{
namespace xuggler
{

class IContainerFormat;
/**
 * A "key" to an {@link IStreamCoder} that tells it how to encode or decode data.
 *
 * Use these objects to tell a IStreamCoder you want to use MP3 or NellyMoser
 * for example.
 */
class VS_API_XUGGLER ICodec : public com::xuggle::ferry::RefCounted
{
public:
  /**
   * These are the codecs this library currently supports.
   * These are based on FFMPEG r23197 or later
   */
  typedef enum
  {

      CODEC_ID_NONE,

      /* video codecs */
      CODEC_ID_MPEG1VIDEO,
      CODEC_ID_MPEG2VIDEO,
      CODEC_ID_MPEG2VIDEO_XVMC,
      CODEC_ID_H261,
      CODEC_ID_H263,
      CODEC_ID_RV10,
      CODEC_ID_RV20,
      CODEC_ID_MJPEG,
      CODEC_ID_MJPEGB,
      CODEC_ID_LJPEG,
      CODEC_ID_SP5X,
      CODEC_ID_JPEGLS,
      CODEC_ID_MPEG4,
      CODEC_ID_RAWVIDEO,
      CODEC_ID_MSMPEG4V1,
      CODEC_ID_MSMPEG4V2,
      CODEC_ID_MSMPEG4V3,
      CODEC_ID_WMV1,
      CODEC_ID_WMV2,
      CODEC_ID_H263P,
      CODEC_ID_H263I,
      CODEC_ID_FLV1,
      CODEC_ID_SVQ1,
      CODEC_ID_SVQ3,
      CODEC_ID_DVVIDEO,
      CODEC_ID_HUFFYUV,
      CODEC_ID_CYUV,
      CODEC_ID_H264,
      CODEC_ID_INDEO3,
      CODEC_ID_VP3,
      CODEC_ID_THEORA,
      CODEC_ID_ASV1,
      CODEC_ID_ASV2,
      CODEC_ID_FFV1,
      CODEC_ID_4XM,
      CODEC_ID_VCR1,
      CODEC_ID_CLJR,
      CODEC_ID_MDEC,
      CODEC_ID_ROQ,
      CODEC_ID_INTERPLAY_VIDEO,
      CODEC_ID_XAN_WC3,
      CODEC_ID_XAN_WC4,
      CODEC_ID_RPZA,
      CODEC_ID_CINEPAK,
      CODEC_ID_WS_VQA,
      CODEC_ID_MSRLE,
      CODEC_ID_MSVIDEO1,
      CODEC_ID_IDCIN,
      CODEC_ID_8BPS,
      CODEC_ID_SMC,
      CODEC_ID_FLIC,
      CODEC_ID_TRUEMOTION1,
      CODEC_ID_VMDVIDEO,
      CODEC_ID_MSZH,
      CODEC_ID_ZLIB,
      CODEC_ID_QTRLE,
      CODEC_ID_SNOW,
      CODEC_ID_TSCC,
      CODEC_ID_ULTI,
      CODEC_ID_QDRAW,
      CODEC_ID_VIXL,
      CODEC_ID_QPEG,
  #if LIBAVCODEC_VERSION_MAJOR < 53
      CODEC_ID_XVID,
  #endif
      CODEC_ID_PNG,
      CODEC_ID_PPM,
      CODEC_ID_PBM,
      CODEC_ID_PGM,
      CODEC_ID_PGMYUV,
      CODEC_ID_PAM,
      CODEC_ID_FFVHUFF,
      CODEC_ID_RV30,
      CODEC_ID_RV40,
      CODEC_ID_VC1,
      CODEC_ID_WMV3,
      CODEC_ID_LOCO,
      CODEC_ID_WNV1,
      CODEC_ID_AASC,
      CODEC_ID_INDEO2,
      CODEC_ID_FRAPS,
      CODEC_ID_TRUEMOTION2,
      CODEC_ID_BMP,
      CODEC_ID_CSCD,
      CODEC_ID_MMVIDEO,
      CODEC_ID_ZMBV,
      CODEC_ID_AVS,
      CODEC_ID_SMACKVIDEO,
      CODEC_ID_NUV,
      CODEC_ID_KMVC,
      CODEC_ID_FLASHSV,
      CODEC_ID_CAVS,
      CODEC_ID_JPEG2000,
      CODEC_ID_VMNC,
      CODEC_ID_VP5,
      CODEC_ID_VP6,
      CODEC_ID_VP6F,
      CODEC_ID_TARGA,
      CODEC_ID_DSICINVIDEO,
      CODEC_ID_TIERTEXSEQVIDEO,
      CODEC_ID_TIFF,
      CODEC_ID_GIF,
      CODEC_ID_FFH264,
      CODEC_ID_DXA,
      CODEC_ID_DNXHD,
      CODEC_ID_THP,
      CODEC_ID_SGI,
      CODEC_ID_C93,
      CODEC_ID_BETHSOFTVID,
      CODEC_ID_PTX,
      CODEC_ID_TXD,
      CODEC_ID_VP6A,
      CODEC_ID_AMV,
      CODEC_ID_VB,
      CODEC_ID_PCX,
      CODEC_ID_SUNRAST,
      CODEC_ID_INDEO4,
      CODEC_ID_INDEO5,
      CODEC_ID_MIMIC,
      CODEC_ID_RL2,
      CODEC_ID_8SVX_EXP,
      CODEC_ID_8SVX_FIB,
      CODEC_ID_ESCAPE124,
      CODEC_ID_DIRAC,
      CODEC_ID_BFI,
      CODEC_ID_CMV,
      CODEC_ID_MOTIONPIXELS,
      CODEC_ID_TGV,
      CODEC_ID_TGQ,
      CODEC_ID_TQI,
      CODEC_ID_AURA,
      CODEC_ID_AURA2,
      CODEC_ID_V210X,
      CODEC_ID_TMV,
      CODEC_ID_V210,
      CODEC_ID_DPX,
      CODEC_ID_MAD,
      CODEC_ID_FRWU,
      CODEC_ID_FLASHSV2,
      CODEC_ID_CDGRAPHICS,
      CODEC_ID_R210,
      CODEC_ID_ANM,
      CODEC_ID_BINKVIDEO,
      CODEC_ID_IFF_ILBM,
      CODEC_ID_IFF_BYTERUN1,
      CODEC_ID_KGV1,
      CODEC_ID_YOP,
      CODEC_ID_VP8,

      CODEC_ID_PCM_S16LE= 0x10000,
      CODEC_ID_PCM_S16BE,
      CODEC_ID_PCM_U16LE,
      CODEC_ID_PCM_U16BE,
      CODEC_ID_PCM_S8,
      CODEC_ID_PCM_U8,
      CODEC_ID_PCM_MULAW,
      CODEC_ID_PCM_ALAW,
      CODEC_ID_PCM_S32LE,
      CODEC_ID_PCM_S32BE,
      CODEC_ID_PCM_U32LE,
      CODEC_ID_PCM_U32BE,
      CODEC_ID_PCM_S24LE,
      CODEC_ID_PCM_S24BE,
      CODEC_ID_PCM_U24LE,
      CODEC_ID_PCM_U24BE,
      CODEC_ID_PCM_S24DAUD,
      CODEC_ID_PCM_ZORK,
      CODEC_ID_PCM_S16LE_PLANAR,
      CODEC_ID_PCM_DVD,
      CODEC_ID_PCM_F32BE,
      CODEC_ID_PCM_F32LE,
      CODEC_ID_PCM_F64BE,
      CODEC_ID_PCM_F64LE,
      CODEC_ID_PCM_BLURAY,

      CODEC_ID_ADPCM_IMA_QT= 0x11000,
      CODEC_ID_ADPCM_IMA_WAV,
      CODEC_ID_ADPCM_IMA_DK3,
      CODEC_ID_ADPCM_IMA_DK4,
      CODEC_ID_ADPCM_IMA_WS,
      CODEC_ID_ADPCM_IMA_SMJPEG,
      CODEC_ID_ADPCM_MS,
      CODEC_ID_ADPCM_4XM,
      CODEC_ID_ADPCM_XA,
      CODEC_ID_ADPCM_ADX,
      CODEC_ID_ADPCM_EA,
      CODEC_ID_ADPCM_G726,
      CODEC_ID_ADPCM_CT,
      CODEC_ID_ADPCM_SWF,
      CODEC_ID_ADPCM_YAMAHA,
      CODEC_ID_ADPCM_SBPRO_4,
      CODEC_ID_ADPCM_SBPRO_3,
      CODEC_ID_ADPCM_SBPRO_2,
      CODEC_ID_ADPCM_THP,
      CODEC_ID_ADPCM_IMA_AMV,
      CODEC_ID_ADPCM_EA_R1,
      CODEC_ID_ADPCM_EA_R3,
      CODEC_ID_ADPCM_EA_R2,
      CODEC_ID_ADPCM_IMA_EA_SEAD,
      CODEC_ID_ADPCM_IMA_EA_EACS,
      CODEC_ID_ADPCM_EA_XAS,
      CODEC_ID_ADPCM_EA_MAXIS_XA,
      CODEC_ID_ADPCM_IMA_ISS,

      CODEC_ID_AMR_NB= 0x12000,
      CODEC_ID_AMR_WB,

      CODEC_ID_RA_144= 0x13000,
      CODEC_ID_RA_288,

      CODEC_ID_ROQ_DPCM= 0x14000,
      CODEC_ID_INTERPLAY_DPCM,
      CODEC_ID_XAN_DPCM,
      CODEC_ID_SOL_DPCM,

      CODEC_ID_MP2= 0x15000,
      CODEC_ID_MP3,
      CODEC_ID_AAC,
      CODEC_ID_AC3,
      CODEC_ID_DTS,
      CODEC_ID_VORBIS,
      CODEC_ID_DVAUDIO,
      CODEC_ID_WMAV1,
      CODEC_ID_WMAV2,
      CODEC_ID_MACE3,
      CODEC_ID_MACE6,
      CODEC_ID_VMDAUDIO,
      CODEC_ID_SONIC,
      CODEC_ID_SONIC_LS,
      CODEC_ID_FLAC,
      CODEC_ID_MP3ADU,
      CODEC_ID_MP3ON4,
      CODEC_ID_SHORTEN,
      CODEC_ID_ALAC,
      CODEC_ID_WESTWOOD_SND1,
      CODEC_ID_GSM,
      CODEC_ID_QDM2,
      CODEC_ID_COOK,
      CODEC_ID_TRUESPEECH,
      CODEC_ID_TTA,
      CODEC_ID_SMACKAUDIO,
      CODEC_ID_QCELP,
      CODEC_ID_WAVPACK,
      CODEC_ID_DSICINAUDIO,
      CODEC_ID_IMC,
      CODEC_ID_MUSEPACK7,
      CODEC_ID_MLP,
      CODEC_ID_GSM_MS,
      CODEC_ID_ATRAC3,
      CODEC_ID_VOXWARE,
      CODEC_ID_APE,
      CODEC_ID_NELLYMOSER,
      CODEC_ID_MUSEPACK8,
      CODEC_ID_SPEEX,
      CODEC_ID_WMAVOICE,
      CODEC_ID_WMAPRO,
      CODEC_ID_WMALOSSLESS,
      CODEC_ID_ATRAC3P,
      CODEC_ID_EAC3,
      CODEC_ID_SIPR,
      CODEC_ID_MP1,
      CODEC_ID_TWINVQ,
      CODEC_ID_TRUEHD,
      CODEC_ID_MP4ALS,
      CODEC_ID_ATRAC1,
      CODEC_ID_BINKAUDIO_RDFT,
      CODEC_ID_BINKAUDIO_DCT,

      /* subtitle codecs */
      CODEC_ID_DVD_SUBTITLE= 0x17000,
      CODEC_ID_DVB_SUBTITLE,
      CODEC_ID_TEXT,
      CODEC_ID_XSUB,
      CODEC_ID_SSA,
      CODEC_ID_MOV_TEXT,
      CODEC_ID_HDMV_PGS_SUBTITLE,
      CODEC_ID_DVB_TELETEXT,

      CODEC_ID_TTF= 0x18000,

      CODEC_ID_PROBE= 0x19000,

      CODEC_ID_MPEG2TS= 0x20000,
  } ID;

  /**
   * The different types of Codecs that can exist in the system.
   */
  typedef enum
  {
    CODEC_TYPE_UNKNOWN = -1,
    CODEC_TYPE_VIDEO,
    CODEC_TYPE_AUDIO,
    CODEC_TYPE_DATA,
    CODEC_TYPE_SUBTITLE,
    CODEC_TYPE_ATTACHMENT
  } Type;

  /**
   * Get the name of the codec.
   * @return The name of this Codec.
   */
  virtual const char *
  getName()=0;

  /**
   * Get the ID of this codec, as an integer.
   * @return the ID of this codec, as an integer.
   */
  virtual int
  getIDAsInt()=0;

  /**
   * Get the ID of this codec as an enumeration.
   * @return the ID of this codec, an enum ID
   */
  virtual ID
  getID()=0;

  /**
   * Get the type of this codec.
   * @return The type of this Codec, as a enum Type
   */
  virtual Type
  getType()=0;

  /**
   * Can this codec be used for decoding?
   * @return Can this Codec decode?
   */
  virtual bool
  canDecode()=0;

  /**
   * Can this codec be used for encoding?
   * @return Can this Codec encode?
   */
  virtual bool
  canEncode()=0;

  /**
   * Find a codec that can be used for encoding.
   * @param id The id of the codec
   * @return the codec, or null if we can't find it.
   *
   */
  static ICodec *
  findEncodingCodec(ICodec::ID id);
  /**
   * Find a codec that can be used for encoding.
   * @param id The id of the codec, as an integer.
   * @return the codec, or null if we can't find it.
   */
  static ICodec *
  findEncodingCodecByIntID(int id);
  /**
   * Find a codec that can be used for encoding.
   * @param id The id of the codec, as a FFMPEG short-name string
   *   (for example, "mpeg4").
   * @return the codec, or null if we can't find it.
   */
  static ICodec *
  findEncodingCodecByName(const char*id);
  /**
   * Find a codec that can be used for decoding.
   * @param id The id of the codec
   * @return the codec, or null if we can't find it.
   */
  static ICodec *
  findDecodingCodec(ICodec::ID id);
  /**
   * Find a codec that can be used for decoding.
   * @param id The id of the codec, as an integer
   * @return the codec, or null if we can't find it.
   */
  static ICodec *
  findDecodingCodecByIntID(int id);
  /**
   * Find a codec that can be used for decoding.
   * @param id The id of the codec, as a FFMPEG short-name string
   *   (for example, "mpeg4")
   * @return the codec, or null if we can't find it.
   */
  static ICodec *
  findDecodingCodecByName(const char*id);

  /**
   * Ask us to guess an encoding codec based on the inputs
   * passed in.
   * <p>
   * You must pass in at least one non null fmt, shortName,
   * url or mime_type.
   * </p>
   * @param fmt An IContainerFormat for the container you'll want to encode into.
   * @param shortName The FFMPEG short name of the codec (e.g. "mpeg4").
   * @param url The URL you'll be writing packets to.
   * @param mimeType The mime type of the container.
   * @param type The codec type.
   * @return the codec, or null if we can't find it.
   */
  static ICodec *
  guessEncodingCodec(IContainerFormat* fmt, const char *shortName,
      const char*url, const char*mimeType, ICodec::Type type);

protected:
  ICodec();
  virtual
  ~ICodec();

  /**
   * Added for 1.17
   */
public:
  /**
   * Get the long name for this codec.
   *
   * @return the long name.
   */
  virtual const char *
  getLongName()=0;

  /*
   * Added for 2.1
   */

  /**
   * Capability flags
   */
  typedef enum
  {
    CAP_DRAW_HORIZ_BAND = 0x0001,
    CAP_DR1 = 0x0002,
    CAP_PARSE_ONLY = 0x0004,
    CAP_TRUNCATED = 0x0008,
    CAP_HWACCEL = 0x0010,
    CAP_DELAY = 0x0020,
    CAP_SMALL_LAST_FRAME = 0x0040,
    CAP_HWACCEL_VDPAU = 0x0080,
    CAP_SUBFRAMES =0x0100,

  } Capabilities;

  /**
   * Get the capabilites flag from the codec
   * 
   * @return the capabilities flag
   */
  virtual int32_t getCapabilities()=0;
  
  /**
   * Convenience method to check individual capability flags.
   * 
   * @param capability the capability
   * 
   * @return true if flag is set; false otherwise.
   */
  virtual bool hasCapability(Capabilities capability)=0;
  
  /**
   * Get the number of installed codecs on this system.
   * @return the number of installed codecs.
   */
  static int32_t getNumInstalledCodecs();
  
  /**
   * Get the {@link ICodec} at the given index.
   * 
   * @param index the index in our list
   * 
   * @return the codec, or null if index < 0 or index >= 
   *   {@link #getNumInstalledCodecs()}
   */
  static ICodec* getInstalledCodec(int32_t index);

  /**
   * Get the number of frame rates this codec supports for encoding.
   * Not all codecs will report this number.
   * @return the number or 0 if we don't know.
   */
  virtual int32_t getNumSupportedVideoFrameRates()=0;
  
  /**
   * Return the supported frame rate at the given index.
   * 
   * @param index the index in our list.
   * 
   * @return the frame rate, or null if unknown, if index <0 or
   *   if index >= {@link #getNumSupportedVideoFrameRates()}
   */
  virtual IRational* getSupportedVideoFrameRate(int32_t index)=0;
  
  /**
   * Get the number of supported video pixel formats this codec supports
   * for encoding.  Not all codecs will report this.
   * 
   * @return the number or 0 if we don't know.
   */
  virtual int32_t getNumSupportedVideoPixelFormats()=0;

  /**
   * Return the supported video pixel format at the given index.
   * 
   * @param index the index in our list.
   * 
   * @return the pixel format, or {@link IPixelFormat.Type#NONE} if unknown,
   *   if index <0 or
   *   if index >= {@link #getNumSupportedVideoPixelFormats()}
   */
  virtual IPixelFormat::Type getSupportedVideoPixelFormat(int32_t index)=0;
  
  /**
   * Get the number of different audio sample rates this codec supports
   * for encoding.  Not all codecs will report this.
   * 
   * @return the number or 0 if we don't know.
   */
  virtual int32_t getNumSupportedAudioSampleRates()=0;
  
  /**
   * Return the support audio sample rate at the given index.
   * 
   * @param index the index in our list.
   * 
   * @return the sample rate, or 0 if unknown, index < 0 or
   *   index >= {@link #getNumSupportedAudioSampleRates()}
   */
  virtual int32_t getSupportedAudioSampleRate(int32_t index)=0;
  
  /**
   * Get the number of different audio sample formats this codec supports
   * for encoding.  Not all codecs will report this.
   * 
   * @return the number or 0 if we don't know.
   */

  virtual int32_t getNumSupportedAudioSampleFormats()=0;
  
  /**
   * Get the supported sample format at this index.
   * 
   * @param index the index in our list.
   * 
   * @return the format, or {@link IAudioSamples.Format.FMT_NONE} if
   *   unknown, index < 0 or index >=
   *   {@link #getNumSupportedAudioSampleFormats()}.
   */
  virtual IAudioSamples::Format getSupportedAudioSampleFormat(int32_t index)=0;
  
  /**
   * Get the number of different audio channel layouts this codec supports
   * for encoding.  Not all codecs will report this.
   * 
   * 
   * @return the number or 0 if we don't know.
   */

  virtual int32_t getNumSupportedAudioChannelLayouts()=0;
  
  /**
   * Get the supported audio channel layout at this index.
   * 
   * The value returned is a bit flag representing the different
   * types of audio layout this codec can support.  Test the values
   * by bit-comparing them to the {@link IAudioSamples.ChannelLayout}
   * enum types.
   * 
   * @param index the index
   * 
   * @return the channel layout, or 0 if unknown, index < 0 or
   *   index >= {@link #getNumSupportedAudioChannelLayouts}.
   */
  virtual int64_t getSupportedAudioChannelLayout(int32_t index)=0;
  
};

}
}
}

#endif /*IH_*/

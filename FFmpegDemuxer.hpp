#include "lotus_demuxer.h"
//#pragma once
#define __D(...) printf(__VA_ARGS__)
#define __I(...) printf(__VA_ARGS__)
#define __E(...) printf(__VA_ARGS__)
#define __W(...) printf(__VA_ARGS__)

extern "C" {
//#include "libavcodec/packet.h"
#ifndef AVCODEC_PACKET_H
#define AVCODEC_PACKET_H

#include <stddef.h>
#include <stdint.h>

#include "libavutil/attributes.h"
#include "libavutil/buffer.h"
#include "libavutil/dict.h"
#include "libavutil/rational.h"
#include "libavutil/version.h"

#include "libavcodec/version.h"

/**
 * @defgroup lavc_packet AVPacket
 *
 * Types and functions for working with AVPacket.
 * @{
 */
enum AVPacketSideDataType {
  /**
   * An AV_PKT_DATA_PALETTE side data packet contains exactly AVPALETTE_SIZE
   * bytes worth of palette. This side data signals that a new palette is
   * present.
   */
  AV_PKT_DATA_PALETTE,

  /**
   * The AV_PKT_DATA_NEW_EXTRADATA is used to notify the codec or the format
   * that the extradata buffer was changed and the receiving side should
   * act upon it appropriately. The new extradata is embedded in the side
   * data buffer and should be immediately used for processing the current
   * frame or packet.
   */
  AV_PKT_DATA_NEW_EXTRADATA,

  /**
   * An AV_PKT_DATA_PARAM_CHANGE side data packet is laid out as follows:
   * @code
   * u32le param_flags
   * if (param_flags & AV_SIDE_DATA_PARAM_CHANGE_CHANNEL_COUNT)
   *     s32le channel_count
   * if (param_flags & AV_SIDE_DATA_PARAM_CHANGE_CHANNEL_LAYOUT)
   *     u64le channel_layout
   * if (param_flags & AV_SIDE_DATA_PARAM_CHANGE_SAMPLE_RATE)
   *     s32le sample_rate
   * if (param_flags & AV_SIDE_DATA_PARAM_CHANGE_DIMENSIONS)
   *     s32le width
   *     s32le height
   * @endcode
   */
  AV_PKT_DATA_PARAM_CHANGE,

  /**
   * An AV_PKT_DATA_H263_MB_INFO side data packet contains a number of
   * structures with info about macroblocks relevant to splitting the
   * packet into smaller packets on macroblock edges (e.g. as for RFC 2190).
   * That is, it does not necessarily contain info about all macroblocks,
   * as long as the distance between macroblocks in the info is smaller
   * than the target payload size.
   * Each MB info structure is 12 bytes, and is laid out as follows:
   * @code
   * u32le bit offset from the start of the packet
   * u8    current quantizer at the start of the macroblock
   * u8    GOB number
   * u16le macroblock address within the GOB
   * u8    horizontal MV predictor
   * u8    vertical MV predictor
   * u8    horizontal MV predictor for block number 3
   * u8    vertical MV predictor for block number 3
   * @endcode
   */
  AV_PKT_DATA_H263_MB_INFO,

  /**
   * This side data should be associated with an audio stream and contains
   * ReplayGain information in form of the AVReplayGain struct.
   */
  AV_PKT_DATA_REPLAYGAIN,

  /**
   * This side data contains a 3x3 transformation matrix describing an affine
   * transformation that needs to be applied to the decoded video frames for
   * correct presentation.
   *
   * See libavutil/display.h for a detailed description of the data.
   */
  AV_PKT_DATA_DISPLAYMATRIX,

  /**
   * This side data should be associated with a video stream and contains
   * Stereoscopic 3D information in form of the AVStereo3D struct.
   */
  AV_PKT_DATA_STEREO3D,

  /**
   * This side data should be associated with an audio stream and corresponds
   * to enum AVAudioServiceType.
   */
  AV_PKT_DATA_AUDIO_SERVICE_TYPE,

  /**
   * This side data contains quality related information from the encoder.
   * @code
   * u32le quality factor of the compressed frame. Allowed range is between 1 (good) and FF_LAMBDA_MAX (bad).
   * u8    picture type
   * u8    error count
   * u16   reserved
   * u64le[error count] sum of squared differences between encoder in and output
   * @endcode
   */
  AV_PKT_DATA_QUALITY_STATS,

  /**
   * This side data contains an integer value representing the stream index
   * of a "fallback" track.  A fallback track indicates an alternate
   * track to use when the current track can not be decoded for some reason.
   * e.g. no decoder available for codec.
   */
  AV_PKT_DATA_FALLBACK_TRACK,

  /**
   * This side data corresponds to the AVCPBProperties struct.
   */
  AV_PKT_DATA_CPB_PROPERTIES,

  /**
   * Recommmends skipping the specified number of samples
   * @code
   * u32le number of samples to skip from start of this packet
   * u32le number of samples to skip from end of this packet
   * u8    reason for start skip
   * u8    reason for end   skip (0=padding silence, 1=convergence)
   * @endcode
   */
  AV_PKT_DATA_SKIP_SAMPLES,

  /**
   * An AV_PKT_DATA_JP_DUALMONO side data packet indicates that
   * the packet may contain "dual mono" audio specific to Japanese DTV
   * and if it is true, recommends only the selected channel to be used.
   * @code
   * u8    selected channels (0=mail/left, 1=sub/right, 2=both)
   * @endcode
   */
  AV_PKT_DATA_JP_DUALMONO,

  /**
   * A list of zero terminated key/value strings. There is no end marker for
   * the list, so it is required to rely on the side data size to stop.
   */
  AV_PKT_DATA_STRINGS_METADATA,

  /**
   * Subtitle event position
   * @code
   * u32le x1
   * u32le y1
   * u32le x2
   * u32le y2
   * @endcode
   */
  AV_PKT_DATA_SUBTITLE_POSITION,

  /**
   * Data found in BlockAdditional element of matroska container. There is
   * no end marker for the data, so it is required to rely on the side data
   * size to recognize the end. 8 byte id (as found in BlockAddId) followed
   * by data.
   */
  AV_PKT_DATA_MATROSKA_BLOCKADDITIONAL,

  /**
   * The optional first identifier line of a WebVTT cue.
   */
  AV_PKT_DATA_WEBVTT_IDENTIFIER,

  /**
   * The optional settings (rendering instructions) that immediately
   * follow the timestamp specifier of a WebVTT cue.
   */
  AV_PKT_DATA_WEBVTT_SETTINGS,

  /**
   * A list of zero terminated key/value strings. There is no end marker for
   * the list, so it is required to rely on the side data size to stop. This
   * side data includes updated metadata which appeared in the stream.
   */
  AV_PKT_DATA_METADATA_UPDATE,

  /**
   * MPEGTS stream ID as uint8_t, this is required to pass the stream ID
   * information from the demuxer to the corresponding muxer.
   */
  AV_PKT_DATA_MPEGTS_STREAM_ID,

  /**
   * Mastering display metadata (based on SMPTE-2086:2014). This metadata
   * should be associated with a video stream and contains data in the form
   * of the AVMasteringDisplayMetadata struct.
   */
  AV_PKT_DATA_MASTERING_DISPLAY_METADATA,

  /**
   * This side data should be associated with a video stream and corresponds
   * to the AVSphericalMapping structure.
   */
  AV_PKT_DATA_SPHERICAL,

  /**
   * Content light level (based on CTA-861.3). This metadata should be
   * associated with a video stream and contains data in the form of the
   * AVContentLightMetadata struct.
   */
  AV_PKT_DATA_CONTENT_LIGHT_LEVEL,

  /**
   * ATSC A53 Part 4 Closed Captions. This metadata should be associated with
   * a video stream. A53 CC bitstream is stored as uint8_t in AVPacketSideData.data.
   * The number of bytes of CC data is AVPacketSideData.size.
   */
  AV_PKT_DATA_A53_CC,

  /**
   * This side data is encryption initialization data.
   * The format is not part of ABI, use av_encryption_init_info_* methods to
   * access.
   */
  AV_PKT_DATA_ENCRYPTION_INIT_INFO,

  /**
   * This side data contains encryption info for how to decrypt the packet.
   * The format is not part of ABI, use av_encryption_info_* methods to access.
   */
  AV_PKT_DATA_ENCRYPTION_INFO,

  /**
   * Active Format Description data consisting of a single byte as specified
   * in ETSI TS 101 154 using AVActiveFormatDescription enum.
   */
  AV_PKT_DATA_AFD,

  /**
   * Producer Reference Time data corresponding to the AVProducerReferenceTime struct,
   * usually exported by some encoders (on demand through the prft flag set in the
   * AVCodecContext export_side_data field).
   */
  AV_PKT_DATA_PRFT,

  /**
   * ICC profile data consisting of an opaque octet buffer following the
   * format described by ISO 15076-1.
   */
  AV_PKT_DATA_ICC_PROFILE,

  /**
   * DOVI configuration
   * ref:
   * dolby-vision-bitstreams-within-the-iso-base-media-file-format-v2.1.2, section 2.2
   * dolby-vision-bitstreams-in-mpeg-2-transport-stream-multiplex-v1.2, section 3.3
   * Tags are stored in struct AVDOVIDecoderConfigurationRecord.
   */
  AV_PKT_DATA_DOVI_CONF,

  /**
   * Timecode which conforms to SMPTE ST 12-1:2014. The data is an array of 4 uint32_t
   * where the first uint32_t describes how many (1-3) of the other timecodes are used.
   * The timecode format is described in the documentation of av_timecode_get_smpte_from_framenum()
   * function in libavutil/timecode.h.
   */
  AV_PKT_DATA_S12M_TIMECODE,

  /**
   * HDR10+ dynamic metadata associated with a video frame. The metadata is in
   * the form of the AVDynamicHDRPlus struct and contains
   * information for color volume transform - application 4 of
   * SMPTE 2094-40:2016 standard.
   */
  AV_PKT_DATA_DYNAMIC_HDR10_PLUS,

  /**
   * The number of side data types.
   * This is not part of the public API/ABI in the sense that it may
   * change when new side data types are added.
   * This must stay the last enum value.
   * If its value becomes huge, some code using it
   * needs to be updated as it assumes it to be smaller than other limits.
   */
  AV_PKT_DATA_NB
};

#define AV_PKT_DATA_QUALITY_FACTOR AV_PKT_DATA_QUALITY_STATS  // DEPRECATED

typedef struct AVPacketSideData {
  uint8_t *data;
  size_t size;
  enum AVPacketSideDataType type;
} AVPacketSideData;

/**
 * This structure stores compressed data. It is typically exported by demuxers
 * and then passed as input to decoders, or received as output from encoders and
 * then passed to muxers.
 *
 * For video, it should typically contain one compressed frame. For audio it may
 * contain several compressed frames. Encoders are allowed to output empty
 * packets, with no compressed data, containing only side data
 * (e.g. to update some stream parameters at the end of encoding).
 *
 * The semantics of data ownership depends on the buf field.
 * If it is set, the packet data is dynamically allocated and is
 * valid indefinitely until a call to av_packet_unref() reduces the
 * reference count to 0.
 *
 * If the buf field is not set av_packet_ref() would make a copy instead
 * of increasing the reference count.
 *
 * The side data is always allocated with av_malloc(), copied by
 * av_packet_ref() and freed by av_packet_unref().
 *
 * sizeof(AVPacket) being a part of the public ABI is deprecated. once
 * av_init_packet() is removed, new packets will only be able to be allocated
 * with av_packet_alloc(), and new fields may be added to the end of the struct
 * with a minor bump.
 *
 * @see av_packet_alloc
 * @see av_packet_ref
 * @see av_packet_unref
 */
typedef struct AVPacket {
  /**
   * A reference to the reference-counted buffer where the packet data is
   * stored.
   * May be NULL, then the packet data is not reference-counted.
   */
  AVBufferRef *buf;
  /**
   * Presentation timestamp in AVStream->time_base units; the time at which
   * the decompressed packet will be presented to the user.
   * Can be AV_NOPTS_VALUE if it is not stored in the file.
   * pts MUST be larger or equal to dts as presentation cannot happen before
   * decompression, unless one wants to view hex dumps. Some formats misuse
   * the terms dts and pts/cts to mean something different. Such timestamps
   * must be converted to true pts/dts before they are stored in AVPacket.
   */
  int64_t pts;
  /**
   * Decompression timestamp in AVStream->time_base units; the time at which
   * the packet is decompressed.
   * Can be AV_NOPTS_VALUE if it is not stored in the file.
   */
  int64_t dts;
  uint8_t *data;
  int size;
  int stream_index;
  /**
   * A combination of AV_PKT_FLAG values
   */
  int flags;
  /**
   * Additional packet data that can be provided by the container.
   * Packet can contain several types of side information.
   */
  AVPacketSideData *side_data;
  int side_data_elems;

  /**
   * Duration of this packet in AVStream->time_base units, 0 if unknown.
   * Equals next_pts - this_pts in presentation order.
   */
  int64_t duration;

  int64_t pos;  ///< byte position in stream, -1 if unknown

  /**
   * for some private data of the user
   */
  void *opaque;

  /**
   * AVBufferRef for free use by the API user. FFmpeg will never check the
   * contents of the buffer ref. FFmpeg calls av_buffer_unref() on it when
   * the packet is unreferenced. av_packet_copy_props() calls create a new
   * reference with av_buffer_ref() for the target packet's opaque_ref field.
   *
   * This is unrelated to the opaque field, although it serves a similar
   * purpose.
   */
  AVBufferRef *opaque_ref;

  /**
   * Time base of the packet's timestamps.
   * In the future, this field may be set on packets output by encoders or
   * demuxers, but its value will be by default ignored on input to decoders
   * or muxers.
   */
  AVRational time_base;
} AVPacket;

#if FF_API_INIT_PACKET
attribute_deprecated typedef struct AVPacketList {
  AVPacket pkt;
  struct AVPacketList *next;
} AVPacketList;
#endif

#define AV_PKT_FLAG_KEY 0x0001      ///< The packet contains a keyframe
#define AV_PKT_FLAG_CORRUPT 0x0002  ///< The packet content is corrupted
/**
 * Flag is used to discard packets which are required to maintain valid
 * decoder state but are not required for output and should be dropped
 * after decoding.
 **/
#define AV_PKT_FLAG_DISCARD 0x0004
/**
 * The packet comes from a trusted source.
 *
 * Otherwise-unsafe constructs such as arbitrary pointers to data
 * outside the packet may be followed.
 */
#define AV_PKT_FLAG_TRUSTED 0x0008
/**
 * Flag is used to indicate packets that contain frames that can
 * be discarded by the decoder.  I.e. Non-reference frames.
 */
#define AV_PKT_FLAG_DISPOSABLE 0x0010

enum AVSideDataParamChangeFlags {
#if FF_API_OLD_CHANNEL_LAYOUT
  /**
   * @deprecated those are not used by any decoder
   */
  AV_SIDE_DATA_PARAM_CHANGE_CHANNEL_COUNT = 0x0001,
  AV_SIDE_DATA_PARAM_CHANGE_CHANNEL_LAYOUT = 0x0002,
#endif
  AV_SIDE_DATA_PARAM_CHANGE_SAMPLE_RATE = 0x0004,
  AV_SIDE_DATA_PARAM_CHANGE_DIMENSIONS = 0x0008,
};

/**
 * Allocate an AVPacket and set its fields to default values.  The resulting
 * struct must be freed using av_packet_free().
 *
 * @return An AVPacket filled with default values or NULL on failure.
 *
 * @note this only allocates the AVPacket itself, not the data buffers. Those
 * must be allocated through other means such as av_new_packet.
 *
 * @see av_new_packet
 */
AVPacket *av_packet_alloc(void);

/**
 * Create a new packet that references the same data as src.
 *
 * This is a shortcut for av_packet_alloc()+av_packet_ref().
 *
 * @return newly created AVPacket on success, NULL on error.
 *
 * @see av_packet_alloc
 * @see av_packet_ref
 */
AVPacket *av_packet_clone(const AVPacket *src);

/**
 * Free the packet, if the packet is reference counted, it will be
 * unreferenced first.
 *
 * @param pkt packet to be freed. The pointer will be set to NULL.
 * @note passing NULL is a no-op.
 */
void av_packet_free(AVPacket **pkt);

#if FF_API_INIT_PACKET
/**
 * Initialize optional fields of a packet with default values.
 *
 * Note, this does not touch the data and size members, which have to be
 * initialized separately.
 *
 * @param pkt packet
 *
 * @see av_packet_alloc
 * @see av_packet_unref
 *
 * @deprecated This function is deprecated. Once it's removed,
               sizeof(AVPacket) will not be a part of the ABI anymore.
 */
// attribute_deprecated
void av_init_packet(AVPacket *pkt);
#endif

/**
 * Allocate the payload of a packet and initialize its fields with
 * default values.
 *
 * @param pkt packet
 * @param size wanted payload size
 * @return 0 if OK, AVERROR_xxx otherwise
 */
int av_new_packet(AVPacket *pkt, int size);

/**
 * Reduce packet size, correctly zeroing padding
 *
 * @param pkt packet
 * @param size new size
 */
void av_shrink_packet(AVPacket *pkt, int size);

/**
 * Increase packet size, correctly zeroing padding
 *
 * @param pkt packet
 * @param grow_by number of bytes by which to increase the size of the packet
 */
int av_grow_packet(AVPacket *pkt, int grow_by);

/**
 * Initialize a reference-counted packet from av_malloc()ed data.
 *
 * @param pkt packet to be initialized. This function will set the data, size,
 *        and buf fields, all others are left untouched.
 * @param data Data allocated by av_malloc() to be used as packet data. If this
 *        function returns successfully, the data is owned by the underlying AVBuffer.
 *        The caller may not access the data through other means.
 * @param size size of data in bytes, without the padding. I.e. the full buffer
 *        size is assumed to be size + AV_INPUT_BUFFER_PADDING_SIZE.
 *
 * @return 0 on success, a negative AVERROR on error
 */
int av_packet_from_data(AVPacket *pkt, uint8_t *data, int size);

/**
 * Allocate new information of a packet.
 *
 * @param pkt packet
 * @param type side information type
 * @param size side information size
 * @return pointer to fresh allocated data or NULL otherwise
 */
uint8_t *av_packet_new_side_data(AVPacket *pkt, enum AVPacketSideDataType type, size_t size);

/**
 * Wrap an existing array as a packet side data.
 *
 * @param pkt packet
 * @param type side information type
 * @param data the side data array. It must be allocated with the av_malloc()
 *             family of functions. The ownership of the data is transferred to
 *             pkt.
 * @param size side information size
 * @return a non-negative number on success, a negative AVERROR code on
 *         failure. On failure, the packet is unchanged and the data remains
 *         owned by the caller.
 */
int av_packet_add_side_data(AVPacket *pkt, enum AVPacketSideDataType type, uint8_t *data, size_t size);

/**
 * Shrink the already allocated side data buffer
 *
 * @param pkt packet
 * @param type side information type
 * @param size new side information size
 * @return 0 on success, < 0 on failure
 */
int av_packet_shrink_side_data(AVPacket *pkt, enum AVPacketSideDataType type, size_t size);

/**
 * Get side information from packet.
 *
 * @param pkt packet
 * @param type desired side information type
 * @param size If supplied, *size will be set to the size of the side data
 *             or to zero if the desired side data is not present.
 * @return pointer to data if present or NULL otherwise
 */
uint8_t *av_packet_get_side_data(const AVPacket *pkt, enum AVPacketSideDataType type, size_t *size);

const char *av_packet_side_data_name(enum AVPacketSideDataType type);

/**
 * Pack a dictionary for use in side_data.
 *
 * @param dict The dictionary to pack.
 * @param size pointer to store the size of the returned data
 * @return pointer to data if successful, NULL otherwise
 */
uint8_t *av_packet_pack_dictionary(AVDictionary *dict, size_t *size);
/**
 * Unpack a dictionary from side_data.
 *
 * @param data data from side_data
 * @param size size of the data
 * @param dict the metadata storage dictionary
 * @return 0 on success, < 0 on failure
 */
int av_packet_unpack_dictionary(const uint8_t *data, size_t size, AVDictionary **dict);

/**
 * Convenience function to free all the side data stored.
 * All the other fields stay untouched.
 *
 * @param pkt packet
 */
void av_packet_free_side_data(AVPacket *pkt);

/**
 * Setup a new reference to the data described by a given packet
 *
 * If src is reference-counted, setup dst as a new reference to the
 * buffer in src. Otherwise allocate a new buffer in dst and copy the
 * data from src into it.
 *
 * All the other fields are copied from src.
 *
 * @see av_packet_unref
 *
 * @param dst Destination packet. Will be completely overwritten.
 * @param src Source packet
 *
 * @return 0 on success, a negative AVERROR on error. On error, dst
 *         will be blank (as if returned by av_packet_alloc()).
 */
int av_packet_ref(AVPacket *dst, const AVPacket *src);

/**
 * Wipe the packet.
 *
 * Unreference the buffer referenced by the packet and reset the
 * remaining packet fields to their default values.
 *
 * @param pkt The packet to be unreferenced.
 */
void av_packet_unref(AVPacket *pkt);

/**
 * Move every field in src to dst and reset src.
 *
 * @see av_packet_unref
 *
 * @param src Source packet, will be reset
 * @param dst Destination packet
 */
void av_packet_move_ref(AVPacket *dst, AVPacket *src);

/**
 * Copy only "properties" fields from src to dst.
 *
 * Properties for the purpose of this function are all the fields
 * beside those related to the packet data (buf, data, size)
 *
 * @param dst Destination packet
 * @param src Source packet
 *
 * @return 0 on success AVERROR on failure.
 */
int av_packet_copy_props(AVPacket *dst, const AVPacket *src);

/**
 * Ensure the data described by a given packet is reference counted.
 *
 * @note This function does not ensure that the reference will be writable.
 *       Use av_packet_make_writable instead for that purpose.
 *
 * @see av_packet_ref
 * @see av_packet_make_writable
 *
 * @param pkt packet whose data should be made reference counted.
 *
 * @return 0 on success, a negative AVERROR on error. On failure, the
 *         packet is unchanged.
 */
int av_packet_make_refcounted(AVPacket *pkt);

/**
 * Create a writable reference for the data described by a given packet,
 * avoiding data copy if possible.
 *
 * @param pkt Packet whose data should be made writable.
 *
 * @return 0 on success, a negative AVERROR on failure. On failure, the
 *         packet is unchanged.
 */
int av_packet_make_writable(AVPacket *pkt);

/**
 * Convert valid timing fields (timestamps / durations) in a packet from one
 * timebase to another. Timestamps with unknown values (AV_NOPTS_VALUE) will be
 * ignored.
 *
 * @param pkt packet on which the conversion will be performed
 * @param tb_src source timebase, in which the timing fields in pkt are
 *               expressed
 * @param tb_dst destination timebase, to which the timing fields will be
 *               converted
 */
void av_packet_rescale_ts(AVPacket *pkt, AVRational tb_src, AVRational tb_dst);

/**
 * @}
 */

#endif  // AVCODEC_PACKET_H

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>

#if LIBAVCODEC_VERSION_MAJOR >= 59
#include <libavcodec/bsf.h>
#endif
}
//#include "NvCodecUtils.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <chrono>
#include <iomanip>
//#include "Logger.h"
#include <thread>

#include <time.h>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>

#ifdef _WIN32
#include <windows.h>
#include <winsock.h>

#pragma comment(lib, "ws2_32.lib")
#undef ERROR
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#define SOCKET int
#define INVALID_SOCKET -1
#endif

enum LogLevel { TRACE, INFO, WARNING, ERROR, FATAL };

namespace simplelogger {
class Logger {
 public:
  Logger(LogLevel loglevel, bool printTimeStamp) : level(loglevel), bPrintTimeStamp(printTimeStamp) {}
  virtual ~Logger() {}
  virtual std::ostream &GetStream() = 0;
  virtual void FlushStream() {}
  bool ShouldLogFor(LogLevel l) { return l >= level; }
  char *GetLead(LogLevel l, const char *szFile, int nLine, const char *szFunc) {
    if (l < TRACE || l > FATAL) {
        sprintf(szLead, "[?????] %s, line:%d, func:%s", szFile, nLine, szFunc);
      return szLead;
    }
    const char *szLevels[] = {"TRACE", "INFO", "WARN", "ERROR", "FATAL"};
    if (bPrintTimeStamp) {
      time_t t = time(NULL);
      struct tm *ptm = localtime(&t);
      sprintf(szLead, "[%-5s][%02d:%02d:%02d] ", szLevels[l], ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
    } else {
      sprintf(szLead, "[%-5s] ", szLevels[l]);
    }
    return szLead;
  }
  void EnterCriticalSection() { mtx.lock(); }
  void LeaveCriticalSection() { mtx.unlock(); }

 private:
  LogLevel level;
  char szLead[80];
  bool bPrintTimeStamp;
  std::mutex mtx;
};

class LoggerFactory {
 public:
  static Logger *CreateFileLogger(std::string strFilePath, LogLevel level = INFO, bool bPrintTimeStamp = true) {
    return new FileLogger(strFilePath, level, bPrintTimeStamp);
  }
  static Logger *CreateConsoleLogger(LogLevel level = INFO, bool bPrintTimeStamp = true) {
    return new ConsoleLogger(level, bPrintTimeStamp);
  }
  static Logger *CreateUdpLogger(char *szHost, unsigned uPort, LogLevel level = INFO, bool bPrintTimeStamp = true) {
    return new UdpLogger(szHost, uPort, level, bPrintTimeStamp);
  }

 private:
  LoggerFactory() {}

  class FileLogger : public Logger {
   public:
    FileLogger(std::string strFilePath, LogLevel loglevel, bool printTimeStamp) : Logger(loglevel, printTimeStamp) {
      pFileOut = new std::ofstream();
      pFileOut->open(strFilePath.c_str());
    }
    ~FileLogger() { pFileOut->close(); }
    std::ostream &GetStream() { return *pFileOut; }

   private:
    std::ofstream *pFileOut;
  };

  class ConsoleLogger : public Logger {
   public:
    ConsoleLogger(LogLevel loglevel, bool printTimeStamp) : Logger(loglevel, printTimeStamp) {}
    std::ostream &GetStream() { return std::cout; }
  };

  class UdpLogger : public Logger {
   private:
    class UdpOstream : public std::ostream {
     public:
      UdpOstream(char *szHost, unsigned short uPort) : std::ostream(&sb), socket(INVALID_SOCKET) {
#ifdef _WIN32
        WSADATA w;
        if (WSAStartup(0x0101, &w) != 0) {
          fprintf(stderr, "WSAStartup() failed.\n");
          return;
        }
#endif
        socket = ::socket(AF_INET, SOCK_DGRAM, 0);
        if (socket == INVALID_SOCKET) {
#ifdef _WIN32
          WSACleanup();
#endif
          fprintf(stderr, "socket() failed.\n");
          return;
        }
#ifdef _WIN32
        unsigned int b1, b2, b3, b4;
        sscanf(szHost, "%u.%u.%u.%u", &b1, &b2, &b3, &b4);
        struct in_addr addr = {(unsigned char)b1, (unsigned char)b2, (unsigned char)b3, (unsigned char)b4};
#else
        struct in_addr addr = {inet_addr(szHost)};
#endif
        struct sockaddr_in s = {};
        s.sin_family = AF_INET;
        s.sin_port = htons(uPort);
        s.sin_addr = addr;
        server = s;
      }
      ~UdpOstream() throw() {
        if (socket == INVALID_SOCKET) {
          return;
        }
#ifdef _WIN32
        closesocket(socket);
        WSACleanup();
#else
        close(socket);
#endif
      }
      void Flush() {
          auto   buf = sb.str().c_str();
          size_t len = sb.str().length() + 1;
          auto ret = sendto(socket,
                            buf, len,
                            0,
                            (struct sockaddr *)&server,
                            sizeof(sockaddr_in));
        if (ret == -1) {
          fprintf(stderr, "sendto() failed.\n");
        }
        sb.str("");
      }

     private:
      std::stringbuf sb;
      SOCKET socket;
      struct sockaddr_in server;
    };

   public:
    UdpLogger(char *szHost, unsigned uPort, LogLevel loglevel, bool printTimeStamp)
        : Logger(loglevel, printTimeStamp), udpOut(szHost, (unsigned short)uPort) {}
    UdpOstream &GetStream() { return udpOut; }
    virtual void FlushStream() { udpOut.Flush(); }

   private:
    UdpOstream udpOut;
  };
};

class LogTransaction {
 public:
  LogTransaction(Logger *logger, LogLevel loglevel, const char *szFile, const int nLine, const char *szFunc)
      : pLogger(logger), level(loglevel) {
    if (!pLogger) {
      std::cout << "[-----] ";
      return;
    }
    if (!pLogger->ShouldLogFor(level)) {
      return;
    }
    pLogger->EnterCriticalSection();
    pLogger->GetStream() << pLogger->GetLead(level, szFile, nLine, szFunc);
  }
  ~LogTransaction() {
    if (!pLogger) {
      std::cout << std::endl;
      return;
    }
    if (!pLogger->ShouldLogFor(level)) {
      return;
    }
    pLogger->GetStream() << std::endl;
    pLogger->FlushStream();
    pLogger->LeaveCriticalSection();
    if (level == FATAL) {
      exit(1);
    }
  }
  std::ostream &GetStream() {
    if (!pLogger) {
      return std::cout;
    }
    if (!pLogger->ShouldLogFor(level)) {
      return ossNull;
    }
    return pLogger->GetStream();
  }

 private:
  Logger *pLogger;
  LogLevel level;
  std::ostringstream ossNull;
};

}  // namespace simplelogger

#ifdef _NV_ENCODEAPI_H_
inline bool check(NVENCSTATUS e, int line, const char *file) {
  const char *aszErrName[] = {
      "NV_ENC_SUCCESS",
      "NV_ENC_ERR_NO_ENCODE_DEVICE",
      "NV_ENC_ERR_UNSUPPORTED_DEVICE",
      "NV_ENC_ERR_INVALID_ENCODERDEVICE",
      "NV_ENC_ERR_INVALID_DEVICE",
      "NV_ENC_ERR_DEVICE_NOT_EXIST",
      "NV_ENC_ERR_INVALID_PTR",
      "NV_ENC_ERR_INVALID_EVENT",
      "NV_ENC_ERR_INVALID_PARAM",
      "NV_ENC_ERR_INVALID_CALL",
      "NV_ENC_ERR_OUT_OF_MEMORY",
      "NV_ENC_ERR_ENCODER_NOT_INITIALIZED",
      "NV_ENC_ERR_UNSUPPORTED_PARAM",
      "NV_ENC_ERR_LOCK_BUSY",
      "NV_ENC_ERR_NOT_ENOUGH_BUFFER",
      "NV_ENC_ERR_INVALID_VERSION",
      "NV_ENC_ERR_MAP_FAILED",
      "NV_ENC_ERR_NEED_MORE_INPUT",
      "NV_ENC_ERR_ENCODER_BUSY",
      "NV_ENC_ERR_EVENT_NOT_REGISTERD",
      "NV_ENC_ERR_GENERIC",
      "NV_ENC_ERR_INCOMPATIBLE_CLIENT_KEY",
      "NV_ENC_ERR_UNIMPLEMENTED",
      "NV_ENC_ERR_RESOURCE_REGISTER_FAILED",
      "NV_ENC_ERR_RESOURCE_NOT_REGISTERED",
      "NV_ENC_ERR_RESOURCE_NOT_MAPPED",
  };
  if (e != NV_ENC_SUCCESS) {
    __E("NVENC error at %s:%d code=%d \"%s\" \n", file, line, static_cast<unsigned int>(e), aszErrName[e])
    return false;
  }
  return true;
}
#endif

template <typename T>
void check(T result, char const *const func, const char *const file, int const line) {
  if (result) {
    __E("Demuxer error at %s:%d code=%d \"%s\" \n", file, line, static_cast<unsigned int>(result), func);
    exit(EXIT_FAILURE);
  }
}

#define ck(call) check(call, #call, __FILE__, __LINE__)

/**
 * @brief Wrapper class around std::thread
 */
class NvThread {
 public:
  NvThread() = default;
  NvThread(const NvThread &) = delete;
  NvThread &operator=(const NvThread &other) = delete;

  NvThread(std::thread &&thread) : t(std::move(thread)) {}

  NvThread(NvThread &&thread) : t(std::move(thread.t)) {}

  NvThread &operator=(NvThread &&other) {
    t = std::move(other.t);
    return *this;
  }

  ~NvThread() { join(); }

  void join() {
    if (t.joinable()) {
      t.join();
    }
  }

 private:
  std::thread t;
};

#ifndef _WIN32
#define _stricmp strcasecmp
#define _stat64 stat64
#endif

/**
 * @brief Template class to facilitate color space conversion
 */
template <typename T>
class YuvConverter {
 public:
  YuvConverter(int width, int height) : nWidth(width), nHeight(height) { pQuad = new T[nWidth * nHeight / 4]; }
  ~YuvConverter() { delete pQuad; }
  void PlanarToUVInterleaved(T *pFrame, int nPitch = 0) {
    if (nPitch == 0) {
      nPitch = nWidth;
    }
    T *puv = pFrame + nPitch * nHeight;
    if (nPitch == nWidth) {
      memcpy(pQuad, puv, nWidth * nHeight / 4 * sizeof(T));
    } else {
      for (int i = 0; i < nHeight / 2; i++) {
        memcpy(pQuad + nWidth / 2 * i, puv + nPitch / 2 * i, nWidth / 2 * sizeof(T));
      }
    }
    T *pv = puv + (nPitch / 2) * (nHeight / 2);
    for (int y = 0; y < nHeight / 2; y++) {
      for (int x = 0; x < nWidth / 2; x++) {
        puv[y * nPitch + x * 2] = pQuad[y * nWidth / 2 + x];
        puv[y * nPitch + x * 2 + 1] = pv[y * nPitch / 2 + x];
      }
    }
  }
  void UVInterleavedToPlanar(T *pFrame, int nPitch = 0) {
    if (nPitch == 0) {
      nPitch = nWidth;
    }
    T *puv = pFrame + nPitch * nHeight, *pu = puv, *pv = puv + nPitch * nHeight / 4;
    for (int y = 0; y < nHeight / 2; y++) {
      for (int x = 0; x < nWidth / 2; x++) {
        pu[y * nPitch / 2 + x] = puv[y * nPitch + x * 2];
        pQuad[y * nWidth / 2 + x] = puv[y * nPitch + x * 2 + 1];
      }
    }
    if (nPitch == nWidth) {
      memcpy(pv, pQuad, nWidth * nHeight / 4 * sizeof(T));
    } else {
      for (int i = 0; i < nHeight / 2; i++) {
        memcpy(pv + nPitch / 2 * i, pQuad + nWidth / 2 * i, nWidth / 2 * sizeof(T));
      }
    }
  }

 private:
  T *pQuad;
  int nWidth, nHeight;
};

/**
 * @brief Utility class to measure elapsed time in seconds between the block of executed code
 */
class StopWatch {
 public:
  void Start() { t0 = std::chrono::high_resolution_clock::now(); }
  double Stop() {
      return
          (double)std::chrono::duration_cast<std::chrono::nanoseconds>(
              std::chrono::high_resolution_clock::now().time_since_epoch() - t0.time_since_epoch()
              ).count() / 1.0e9;
  }

 private:
  std::chrono::high_resolution_clock::time_point t0;
};

inline void CheckInputFile(const char *szInFilePath) {
  std::ifstream fpIn(szInFilePath, std::ios::in | std::ios::binary);
  if (fpIn.fail()) {
    std::ostringstream err;
    err << "Unable to open input file: " << szInFilePath << std::endl;
    throw std::invalid_argument(err.str());
  }
}

inline void ValidateResolution(int nWidth, int nHeight) {
  if (nWidth <= 0 || nHeight <= 0) {
    std::ostringstream err;
    err << "Please specify positive non zero resolution as -s WxH. Current resolution is " << nWidth << "x" << nHeight
        << std::endl;
    throw std::invalid_argument(err.str());
  }
}

template <class COLOR32>
void Nv12ToColor32(uint8_t *dpNv12, int nNv12Pitch, uint8_t *dpBgra, int nBgraPitch, int nWidth, int nHeight,
                   int iMatrix = 0);
template <class COLOR64>
void Nv12ToColor64(uint8_t *dpNv12, int nNv12Pitch, uint8_t *dpBgra, int nBgraPitch, int nWidth, int nHeight,
                   int iMatrix = 0);

template <class COLOR32>
void P016ToColor32(uint8_t *dpP016, int nP016Pitch, uint8_t *dpBgra, int nBgraPitch, int nWidth, int nHeight,
                   int iMatrix = 4);
template <class COLOR64>
void P016ToColor64(uint8_t *dpP016, int nP016Pitch, uint8_t *dpBgra, int nBgraPitch, int nWidth, int nHeight,
                   int iMatrix = 4);

template <class COLOR32>
void YUV444ToColor32(uint8_t *dpYUV444, int nPitch, uint8_t *dpBgra, int nBgraPitch, int nWidth, int nHeight,
                     int iMatrix = 0);
template <class COLOR64>
void YUV444ToColor64(uint8_t *dpYUV444, int nPitch, uint8_t *dpBgra, int nBgraPitch, int nWidth, int nHeight,
                     int iMatrix = 0);

template <class COLOR32>
void YUV444P16ToColor32(uint8_t *dpYUV444, int nPitch, uint8_t *dpBgra, int nBgraPitch, int nWidth, int nHeight,
                        int iMatrix = 4);
template <class COLOR64>
void YUV444P16ToColor64(uint8_t *dpYUV444, int nPitch, uint8_t *dpBgra, int nBgraPitch, int nWidth, int nHeight,
                        int iMatrix = 4);

template <class COLOR32>
void Nv12ToColorPlanar(uint8_t *dpNv12, int nNv12Pitch, uint8_t *dpBgrp, int nBgrpPitch, int nWidth, int nHeight,
                       int iMatrix = 0);
template <class COLOR32>
void P016ToColorPlanar(uint8_t *dpP016, int nP016Pitch, uint8_t *dpBgrp, int nBgrpPitch, int nWidth, int nHeight,
                       int iMatrix = 4);

template <class COLOR32>
void YUV444ToColorPlanar(uint8_t *dpYUV444, int nPitch, uint8_t *dpBgrp, int nBgrpPitch, int nWidth, int nHeight,
                         int iMatrix = 0);
template <class COLOR32>
void YUV444P16ToColorPlanar(uint8_t *dpYUV444, int nPitch, uint8_t *dpBgrp, int nBgrpPitch, int nWidth, int nHeight,
                            int iMatrix = 4);

void Bgra64ToP016(uint8_t *dpBgra, int nBgraPitch, uint8_t *dpP016, int nP016Pitch, int nWidth, int nHeight,
                  int iMatrix = 4);

void ConvertUInt8ToUInt16(uint8_t *dpUInt8, uint16_t *dpUInt16, int nSrcPitch, int nDestPitch, int nWidth, int nHeight);
void ConvertUInt16ToUInt8(uint16_t *dpUInt16, uint8_t *dpUInt8, int nSrcPitch, int nDestPitch, int nWidth, int nHeight);

void ResizeNv12(unsigned char *dpDstNv12, int nDstPitch, int nDstWidth, int nDstHeight, unsigned char *dpSrcNv12,
                int nSrcPitch, int nSrcWidth, int nSrcHeight, unsigned char *dpDstNv12UV = nullptr);
void ResizeP016(unsigned char *dpDstP016, int nDstPitch, int nDstWidth, int nDstHeight, unsigned char *dpSrcP016,
                int nSrcPitch, int nSrcWidth, int nSrcHeight, unsigned char *dpDstP016UV = nullptr);

void ScaleYUV420(unsigned char *dpDstY, unsigned char *dpDstU, unsigned char *dpDstV, int nDstPitch,
                 int nDstChromaPitch, int nDstWidth, int nDstHeight, unsigned char *dpSrcY, unsigned char *dpSrcU,
                 unsigned char *dpSrcV, int nSrcPitch, int nSrcChromaPitch, int nSrcWidth, int nSrcHeight,
                 bool bSemiplanar);

#ifdef __cuda_cuda_h__
void ComputeCRC(uint8_t *pBuffer, uint32_t *crcValue, CUstream_st *outputCUStream);
#endif

//---------------------------------------------------------------------------
//! \file FFmpegDemuxer.h
//! \brief Provides functionality for stream demuxing
//!
//! This header file is used by Decode/Transcode apps to demux input video clips before decoding frames from it.
//---------------------------------------------------------------------------

/**
 * @brief libavformat wrapper class. Retrieves the elementary encoded stream from the container format.
 */
class FFmpegDemuxer {
 private:
  AVFormatContext *av = NULL;
  AVIOContext *avioc = NULL;
  AVPacket pkt, pktFiltered; /*!< AVPacket stores compressed data typically exported by demuxers and then passed as
                                input to decoders */
  AVBSFContext *bsfc = NULL;

  int v_idx;
  bool bMp4H264, bMp4HEVC, bMp4MPEG4;
  AVCodecID codec_id;
  AVPixelFormat pixel_format;
  int nWidth, nHeight, nBitDepth, nBPP, nChromaHeight;
  double timeBase = 0.0;

  uint8_t *pDataWithHeader = NULL;

  unsigned int frameCount = 0;

 public:
  class DataProvider {
   public:
    virtual ~DataProvider() {}
    virtual int GetData(uint8_t *pBuf, int nBuf) = 0;
  };

 private:
  /**
   *   @brief  Private constructor to initialize libavformat resources.
   *   @param  av - Pointer to AVFormatContext allocated inside avformat_open_input()
   */
  FFmpegDemuxer(AVFormatContext *avCtx) : av(avCtx) {
    if (!av) {
      __E("No AVFormatContext provided. \n");
      return;
    }

    __I("Media format: %s (%s) \n", av->iformat->long_name, av->iformat->name);

    ck(avformat_find_stream_info(av, NULL));
    v_idx = av_find_best_stream(av, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (v_idx < 0) {
      char err[64] = {0};
      av_strerror(v_idx, err, sizeof(err));

      __E("Demuxer error at %s:%d av_find_best_stream: Could not find stream in input, %s \n", __FILE__, __LINE__, err);

      if (v_idx == AVERROR_DECODER_NOT_FOUND) {
        __E("FFmpeg: av_find_best_stream.ff_find_decoder() CANNOT found ANY Decoder match this input, "
            "avformat.c::av_find_best_stream function should fix it\n");
        sleep(1);
      }

      return;
    }

    // av->streams[v_idx]->need_parsing = AVSTREAM_PARSE_NONE;
    codec_id = av->streams[v_idx]->codecpar->codec_id;
    nWidth = av->streams[v_idx]->codecpar->width;
    nHeight = av->streams[v_idx]->codecpar->height;
    pixel_format = (AVPixelFormat)av->streams[v_idx]->codecpar->format;
    AVRational rTimeBase = av->streams[v_idx]->time_base;
    timeBase = av_q2d(rTimeBase);

    // Set bit depth, chroma height, bits per pixel based on pixel_format of input
    switch (pixel_format) {
      case AV_PIX_FMT_YUV420P10LE:
        nBitDepth = 10;
        nChromaHeight = (nHeight + 1) >> 1;
        nBPP = 2;  // 1.5
        break;
      case AV_PIX_FMT_YUV420P12LE:
        nBitDepth = 12;
        nChromaHeight = (nHeight + 1) >> 1;
        nBPP = 2;
        break;
      case AV_PIX_FMT_YUV444P10LE:
        nBitDepth = 10;
        nChromaHeight = nHeight << 1;
        nBPP = 2;  // 3
        break;
      case AV_PIX_FMT_YUV444P12LE:
        nBitDepth = 12;
        nChromaHeight = nHeight << 1;
        nBPP = 2;  // 3
        break;
      case AV_PIX_FMT_YUV444P:
        nBitDepth = 8;
        nChromaHeight = nHeight << 1;
        nBPP = 1;  // 3
        break;
      case AV_PIX_FMT_YUV420P:
      case AV_PIX_FMT_YUVJ420P:
      case AV_PIX_FMT_YUVJ422P:  // jpeg decoder output is subsampled to NV12 for 422/444 so treat it as 420
      case AV_PIX_FMT_YUVJ444P:  // jpeg decoder output is subsampled to NV12 for 422/444 so treat it as 420
        nBitDepth = 8;
        nChromaHeight = (nHeight + 1) >> 1;
        nBPP = 1;  // 3
        break;
      default:
        __W("ChromaFormat not recognized. Assuming 420 \n");
        nBitDepth = 8;
        nChromaHeight = (nHeight + 1) >> 1;
        nBPP = 1;
    }

    bMp4H264 = codec_id == AV_CODEC_ID_H264 && (!strcmp(av->iformat->long_name, "QuickTime / MOV") ||
                                                !strcmp(av->iformat->long_name, "FLV (Flash Video)") ||
                                                !strcmp(av->iformat->long_name, "Matroska / WebM"));
    bMp4HEVC = codec_id == AV_CODEC_ID_HEVC && (!strcmp(av->iformat->long_name, "QuickTime / MOV") ||
                                                !strcmp(av->iformat->long_name, "FLV (Flash Video)") ||
                                                !strcmp(av->iformat->long_name, "Matroska / WebM"));

    bMp4MPEG4 = codec_id == AV_CODEC_ID_MPEG4 && (!strcmp(av->iformat->long_name, "QuickTime / MOV") ||
                                                  !strcmp(av->iformat->long_name, "FLV (Flash Video)") ||
                                                  !strcmp(av->iformat->long_name, "Matroska / WebM"));

    // Initialize packet fields with default values
    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;
    av_init_packet(&pktFiltered);
    pktFiltered.data = NULL;
    pktFiltered.size = 0;

    // Initialize bitstream filter and its required resources
    if (bMp4H264) {
      const AVBitStreamFilter *bsf = av_bsf_get_by_name("h264_mp4toannexb");
      if (!bsf) {
        __E("FFmpeg error: av_bsf_get_by_name failed \n");
        return;
      }
      ck(av_bsf_alloc(bsf, &bsfc));
      avcodec_parameters_copy(bsfc->par_in, av->streams[v_idx]->codecpar);
      ck(av_bsf_init(bsfc));
    }
    if (bMp4HEVC) {
      const AVBitStreamFilter *bsf = av_bsf_get_by_name("hevc_mp4toannexb");
      if (!bsf) {
        __E("FFmpeg error: av_bsf_get_by_name failed \n");
        return;
      }
      ck(av_bsf_alloc(bsf, &bsfc));
      avcodec_parameters_copy(bsfc->par_in, av->streams[v_idx]->codecpar);
      ck(av_bsf_init(bsfc));
    }
  }

  AVFormatContext *createAv(const char *szFilePath) {
    avformat_network_init();
    AVFormatContext *ctx = NULL;
    ck(avformat_open_input(&ctx, szFilePath, NULL, NULL));
    return ctx;
  }

  AVFormatContext *createAv(DataProvider *pDataProvider) {
    AVFormatContext *ctx = NULL;
    if (!(ctx = avformat_alloc_context())) {
      __E("FFmpeg error: avformat_alloc_context failed \n");
      return NULL;
    }

    uint8_t *avioc_buffer = NULL;
    int avioc_buffer_size = 8 * 1024 * 1024;
    avioc_buffer = (uint8_t *)av_malloc((size_t)avioc_buffer_size);
    if (!avioc_buffer) {
      __E("FFmpeg error: av_malloc failed \n");
      return NULL;
    }
    avioc = avio_alloc_context(avioc_buffer, avioc_buffer_size,
                               0,  // write_flag
                               pDataProvider, &dataProviderRead,
                               NULL,   // write_packet
                               NULL);  // seek
    if (!avioc) {
      __E("FFmpeg error: avio_alloc_context failed\n");
      return NULL;
    }

    ctx->pb = avioc;
    ck(avformat_open_input(&ctx, NULL, NULL, NULL));
    return ctx;
  }

 public:
  FFmpegDemuxer(const char *szFilePath) : FFmpegDemuxer(createAv(szFilePath)) {}
  FFmpegDemuxer(DataProvider *pDataProvider) : FFmpegDemuxer(createAv(pDataProvider)) { avioc = av->pb; }
  ~FFmpegDemuxer() {
    if (!av) {
      return;
    }

    if (pkt.data) {
      av_packet_unref(&pkt);
    }
    if (pktFiltered.data) {
      av_packet_unref(&pktFiltered);
    }

    if (bsfc) {
      av_bsf_free(&bsfc);
    }

    avformat_close_input(&av);

    if (avioc) {
      av_freep(&avioc->buffer);
      av_freep(&avioc);
      avioc = NULL;
    }

    if (pDataWithHeader) {
      av_free(pDataWithHeader);
    }
  }

  AVCodecID getVideoCodec() { return codec_id; }
  AVPixelFormat getPixelFormat() { return pixel_format; }
  int getWidth() { return nWidth; }
  int getHeight() { return nHeight; }
  int getBitDepth() { return nBitDepth; }
  int getFrameSize() { return nWidth * (nHeight + nChromaHeight) * nBPP; }
  bool demux(uint8_t **ppVideo, uint32_t *pnVideoBytes, int64_t *pts = NULL) {
    if (!av) {
      return false;
    }

    *pnVideoBytes = 0;

    if (pkt.data) {
      av_packet_unref(&pkt);
    }

    int e = 0;
    while ((e = av_read_frame(av, &pkt)) >= 0 && pkt.stream_index != v_idx) {
      av_packet_unref(&pkt);
    }
    if (e < 0) {
      return false;
    }

    if (bMp4H264 || bMp4HEVC) {
      if (pktFiltered.data) {
        av_packet_unref(&pktFiltered);
      }
      ck(av_bsf_send_packet(bsfc, &pkt));
      ck(av_bsf_receive_packet(bsfc, &pktFiltered));
      *ppVideo = pktFiltered.data;
      *pnVideoBytes = (uint32_t)pktFiltered.size;
      if (pts) {
          // 1sec * 1000: millisec
          auto cur_pts = (timeBase * (double)pktFiltered.pts * 1000);
          *pts = static_cast<int64_t>(cur_pts);
      }
    }
    else
    {
        if (bMp4MPEG4 && (frameCount == 0))
        {
            size_t extraDataSize = (size_t)av->streams[v_idx]->codecpar->extradata_size;

            if (extraDataSize > 0) {
                auto dataWithHeadersize = extraDataSize + (size_t)pkt.size - 3 * sizeof(uint8_t);
                // extradata contains start codes 00 00 01. Subtract its size
                pDataWithHeader = (uint8_t *)av_malloc(dataWithHeadersize);

                if (!pDataWithHeader) {
                    __E("FFmpeg error: av_malloc failed\n");
                    return false;
                }

                memcpy(pDataWithHeader, av->streams[v_idx]->codecpar->extradata,
                       extraDataSize);
                memcpy(pDataWithHeader + extraDataSize,
                       pkt.data + 3,
                       (size_t)pkt.size - 3 * sizeof(uint8_t));

                *ppVideo = pDataWithHeader;
                *pnVideoBytes = static_cast<uint32_t>(dataWithHeadersize);
            }

        }
        else
        {
            *ppVideo      = pkt.data;
            *pnVideoBytes = static_cast<uint32_t>(pkt.size);
        }

        if (pts) {
            auto cur_pts = (timeBase * (double)pkt.pts * 1000);
            *pts = static_cast<int64_t>(cur_pts);
        }
    }

    frameCount++;

    return true;
  }

  static int dataProviderRead(void *opaque, uint8_t *pBuf, int nBuf) {
    return ((DataProvider *)opaque)->GetData(pBuf, nBuf);
  }
};

inline uint32_t FFmpeg2NvCodecId(AVCodecID id) {
  switch (id) {
    case AV_CODEC_ID_MPEG1VIDEO:
      return LOTUS_CODEC_ID_MPEG1;
    case AV_CODEC_ID_MPEG2VIDEO:
      return LOTUS_CODEC_ID_MPEG2;
    case AV_CODEC_ID_MPEG4:
      return LOTUS_CODEC_ID_MPEG4;
    case AV_CODEC_ID_VC1:
      return LOTUS_CODEC_ID_VC1;
    case AV_CODEC_ID_H264:
      return LOTUS_CODEC_ID_H264;
    case AV_CODEC_ID_HEVC:
      return LOTUS_CODEC_ID_HEVC;
    case AV_CODEC_ID_VP8:
      return LOTUS_CODEC_ID_VP8;
    case AV_CODEC_ID_VP9:
      return LOTUS_CODEC_ID_VP9;
    case AV_CODEC_ID_MJPEG:
      return LOTUS_CODEC_ID_JPEG;
    default:
      return LOTUS_CODEC_ID_NumCodecs;
  }
}

#if 0
extern "C" void *TestDemuxerCreate(const char *szInFilePath) {
  auto demuxer = new FFmpegDemuxer(szInFilePath);

  return static_cast<void *>(demuxer);
}

extern "C" int TestDemux(void *demuxer_handle, uint8_t **ppBitstream, uint32_t *pBitstreamBytes) {
  auto demuxer = static_cast<FFmpegDemuxer *>(demuxer_handle);

  return demuxer->demux(ppBitstream, pBitstreamBytes) ? 0 : -1;
}

extern "C" void TestDemuxerDestroy(void *demuxer_handle) {
  auto demuxer = static_cast<FFmpegDemuxer *>(demuxer_handle);
  delete (demuxer);
}

extern "C" uint32_t TestDemuxCodecId(void *demuxer_handle) {
  auto demuxer = static_cast<FFmpegDemuxer *>(demuxer_handle);

  return FFmpeg2NvCodecId(demuxer->getVideoCodec());
}

extern "C" int TestDemuxWidth(void *demuxer_handle) {
  auto demuxer = static_cast<FFmpegDemuxer *>(demuxer_handle);

  return demuxer->getWidth();
}

extern "C" int TestDemuxHeight(void *demuxer_handle) {
  auto demuxer = static_cast<FFmpegDemuxer *>(demuxer_handle);

  return demuxer->getHeight();
}
#endif

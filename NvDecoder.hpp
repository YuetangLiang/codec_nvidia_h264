#pragma once

#include <assert.h>
#include <stdint.h>
#include <mutex>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <string.h>
//#include "nvcuvid.h"

/********************************************************************************************************************/
//! \file nvcuvid.h
//!   NVDECODE API provides video decoding interface to NVIDIA GPU devices.
//! \date 2015-2019
//!  This file contains the interface constants, structure definitions and function prototypes.
/********************************************************************************************************************/

#if !defined(__NVCUVID_H__)
#define __NVCUVID_H__

#include "cuviddec.h"

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */


/***********************************************/
//!
//! High-level helper APIs for video sources
//!
/***********************************************/

typedef void *CUvideosource;
typedef void *CUvideoparser;
typedef long long CUvideotimestamp;


/************************************************************************/
//! \enum cudaVideoState
//! Video source state enums
//! Used in cuvidSetVideoSourceState and cuvidGetVideoSourceState APIs
/************************************************************************/
typedef enum {
    cudaVideoState_Error   = -1,    /**< Error state (invalid source)                  */
    cudaVideoState_Stopped = 0,     /**< Source is stopped (or reached end-of-stream)  */
    cudaVideoState_Started = 1      /**< Source is running and delivering data         */
} cudaVideoState;

/************************************************************************/
//! \enum cudaAudioCodec
//! Audio compression enums
//! Used in CUAUDIOFORMAT structure
/************************************************************************/
typedef enum {
    cudaAudioCodec_MPEG1=0,         /**< MPEG-1 Audio               */
    cudaAudioCodec_MPEG2,           /**< MPEG-2 Audio               */
    cudaAudioCodec_MP3,             /**< MPEG-1 Layer III Audio     */
    cudaAudioCodec_AC3,             /**< Dolby Digital (AC3) Audio  */
    cudaAudioCodec_LPCM,            /**< PCM Audio                  */
    cudaAudioCodec_AAC,             /**< AAC Audio                  */
} cudaAudioCodec;

/************************************************************************************************/
//! \ingroup STRUCTS
//! \struct CUVIDEOFORMAT
//! Video format
//! Used in cuvidGetSourceVideoFormat API
/************************************************************************************************/
typedef struct
{
    cudaVideoCodec codec;                   /**< OUT: Compression format          */
   /**
    * OUT: frame rate = numerator / denominator (for example: 30000/1001)
    */
    struct {
        /**< OUT: frame rate numerator   (0 = unspecified or variable frame rate) */
        unsigned int numerator;
        /**< OUT: frame rate denominator (0 = unspecified or variable frame rate) */
        unsigned int denominator;
    } frame_rate;
    unsigned char progressive_sequence;     /**< OUT: 0=interlaced, 1=progressive                                      */
    unsigned char bit_depth_luma_minus8;    /**< OUT: high bit depth luma. E.g, 2 for 10-bitdepth, 4 for 12-bitdepth   */
    unsigned char bit_depth_chroma_minus8;  /**< OUT: high bit depth chroma. E.g, 2 for 10-bitdepth, 4 for 12-bitdepth */
    unsigned char min_num_decode_surfaces;  /**< OUT: Minimum number of decode surfaces to be allocated for correct
                                                      decoding. The client can send this value in ulNumDecodeSurfaces
                                                      (in CUVIDDECODECREATEINFO structure).
                                                      This guarantees correct functionality and optimal video memory
                                                      usage but not necessarily the best performance, which depends on
                                                      the design of the overall application. The optimal number of
                                                      decode surfaces (in terms of performance and memory utilization)
                                                      should be decided by experimentation for each application, but it
                                                      cannot go below min_num_decode_surfaces.
                                                      If this value is used for ulNumDecodeSurfaces then it must be
                                                      returned to parser during sequence callback.                     */
    unsigned int coded_width;               /**< OUT: coded frame width in pixels                                      */
    unsigned int coded_height;              /**< OUT: coded frame height in pixels                                     */
   /**
    * area of the frame that should be displayed
    * typical example:
    * coded_width = 1920, coded_height = 1088
    * display_area = { 0,0,1920,1080 }
    */
    struct {
        int left;                           /**< OUT: left position of display rect    */
        int top;                            /**< OUT: top position of display rect     */
        int right;                          /**< OUT: right position of display rect   */
        int bottom;                         /**< OUT: bottom position of display rect  */
    } display_area;
    cudaVideoChromaFormat chroma_format;    /**< OUT:  Chroma format                   */
    unsigned int bitrate;                   /**< OUT: video bitrate (bps, 0=unknown)   */
   /**
    * OUT: Display Aspect Ratio = x:y (4:3, 16:9, etc)
    */
    struct {
        int x;
        int y;
    } display_aspect_ratio;
    /**
    * Video Signal Description
    * Refer section E.2.1 (VUI parameters semantics) of H264 spec file
    */
    struct {
        unsigned char video_format          : 3; /**< OUT: 0-Component, 1-PAL, 2-NTSC, 3-SECAM, 4-MAC, 5-Unspecified     */
        unsigned char video_full_range_flag : 1; /**< OUT: indicates the black level and luma and chroma range           */
        unsigned char reserved_zero_bits    : 4; /**< Reserved bits                                                      */
        unsigned char color_primaries;           /**< OUT: chromaticity coordinates of source primaries                  */
        unsigned char transfer_characteristics;  /**< OUT: opto-electronic transfer characteristic of the source picture */
        unsigned char matrix_coefficients;       /**< OUT: used in deriving luma and chroma signals from RGB primaries   */
    } video_signal_description;
    unsigned int seqhdr_data_length;             /**< OUT: Additional bytes following (CUVIDEOFORMATEX)                  */
} CUVIDEOFORMAT;

/****************************************************************/
//! \ingroup STRUCTS
//! \struct CUVIDEOFORMATEX
//! Video format including raw sequence header information
//! Used in cuvidGetSourceVideoFormat API
/****************************************************************/
typedef struct
{
    CUVIDEOFORMAT format;                 /**< OUT: CUVIDEOFORMAT structure */
    unsigned char raw_seqhdr_data[1024];  /**< OUT: Sequence header data    */
} CUVIDEOFORMATEX;

/****************************************************************/
//! \ingroup STRUCTS
//! \struct CUAUDIOFORMAT
//! Audio formats
//! Used in cuvidGetSourceAudioFormat API
/****************************************************************/
typedef struct
{
    cudaAudioCodec codec;       /**< OUT: Compression format                                              */
    unsigned int channels;      /**< OUT: number of audio channels                                        */
    unsigned int samplespersec; /**< OUT: sampling frequency                                              */
    unsigned int bitrate;       /**< OUT: For uncompressed, can also be used to determine bits per sample */
    unsigned int reserved1;     /**< Reserved for future use                                              */
    unsigned int reserved2;     /**< Reserved for future use                                              */
} CUAUDIOFORMAT;


/***************************************************************/
//! \enum CUvideopacketflags
//! Data packet flags
//! Used in CUVIDSOURCEDATAPACKET structure
/***************************************************************/
typedef enum {
    CUVID_PKT_ENDOFSTREAM   = 0x01,   /**< Set when this is the last packet for this stream                              */
    CUVID_PKT_TIMESTAMP     = 0x02,   /**< Timestamp is valid                                                            */
    CUVID_PKT_DISCONTINUITY = 0x04,   /**< Set when a discontinuity has to be signalled                                  */
    CUVID_PKT_ENDOFPICTURE  = 0x08,   /**< Set when the packet contains exactly one frame or one field                   */
    CUVID_PKT_NOTIFY_EOS    = 0x10,   /**< If this flag is set along with CUVID_PKT_ENDOFSTREAM, an additional (dummy)
                                           display callback will be invoked with null value of CUVIDPARSERDISPINFO which
                                           should be interpreted as end of the stream.                                   */
} CUvideopacketflags;

/*****************************************************************************/
//! \ingroup STRUCTS
//! \struct CUVIDSOURCEDATAPACKET
//! Data Packet
//! Used in cuvidParseVideoData API
//! IN for cuvidParseVideoData
/*****************************************************************************/
typedef struct _CUVIDSOURCEDATAPACKET
{
    unsigned long flags;            /**< IN: Combination of CUVID_PKT_XXX flags                              */
    unsigned long payload_size;     /**< IN: number of bytes in the payload (may be zero if EOS flag is set) */
    const unsigned char *payload;   /**< IN: Pointer to packet payload data (may be NULL if EOS flag is set) */
    CUvideotimestamp timestamp;     /**< IN: Presentation time stamp (10MHz clock), only valid if
                                             CUVID_PKT_TIMESTAMP flag is set                                 */
} CUVIDSOURCEDATAPACKET;

// Callback for packet delivery
typedef int (CUDAAPI *PFNVIDSOURCECALLBACK)(void *, CUVIDSOURCEDATAPACKET *);

/**************************************************************************************************************************/
//! \ingroup STRUCTS
//! \struct CUVIDSOURCEPARAMS
//! Describes parameters needed in cuvidCreateVideoSource API
//! NVDECODE API is intended for HW accelerated video decoding so CUvideosource doesn't have audio demuxer for all supported
//! containers. It's recommended to clients to use their own or third party demuxer if audio support is needed.
/**************************************************************************************************************************/
typedef struct _CUVIDSOURCEPARAMS
{
    unsigned int ulClockRate;                   /**< IN: Time stamp units in Hz (0=default=10000000Hz)      */
    unsigned int uReserved1[7];                 /**< Reserved for future use - set to zero                  */
    void *pUserData;                            /**< IN: User private data passed in to the data handlers   */
    PFNVIDSOURCECALLBACK pfnVideoDataHandler;   /**< IN: Called to deliver video packets                    */
    PFNVIDSOURCECALLBACK pfnAudioDataHandler;   /**< IN: Called to deliver audio packets.                   */
    void *pvReserved2[8];                       /**< Reserved for future use - set to NULL                  */
} CUVIDSOURCEPARAMS;


/**********************************************/
//! \ingroup ENUMS
//! \enum CUvideosourceformat_flags
//! CUvideosourceformat_flags
//! Used in cuvidGetSourceVideoFormat API
/**********************************************/
typedef enum {
    CUVID_FMT_EXTFORMATINFO = 0x100             /**< Return extended format structure (CUVIDEOFORMATEX) */
} CUvideosourceformat_flags;

#if !defined(__APPLE__)
/***************************************************************************************************************************/
//! \ingroup FUNCTS
//! \fn CUresult CUDAAPI cuvidCreateVideoSource(CUvideosource *pObj, const char *pszFileName, CUVIDSOURCEPARAMS *pParams)
//! Create CUvideosource object. CUvideosource spawns demultiplexer thread that provides two callbacks: 
//! pfnVideoDataHandler() and pfnAudioDataHandler()
//! NVDECODE API is intended for HW accelerated video decoding so CUvideosource doesn't have audio demuxer for all supported 
//! containers. It's recommended to clients to use their own or third party demuxer if audio support is needed.
/***************************************************************************************************************************/
CUresult CUDAAPI cuvidCreateVideoSource(CUvideosource *pObj, const char *pszFileName, CUVIDSOURCEPARAMS *pParams);

/***************************************************************************************************************************/
//! \ingroup FUNCTS
//! \fn CUresult CUDAAPI cuvidCreateVideoSourceW(CUvideosource *pObj, const wchar_t *pwszFileName, CUVIDSOURCEPARAMS *pParams)
//! Create video source
/***************************************************************************************************************************/
CUresult CUDAAPI cuvidCreateVideoSourceW(CUvideosource *pObj, const wchar_t *pwszFileName, CUVIDSOURCEPARAMS *pParams);

/********************************************************************/
//! \ingroup FUNCTS
//! \fn CUresult CUDAAPI cuvidDestroyVideoSource(CUvideosource obj)
//! Destroy video source
/********************************************************************/
CUresult CUDAAPI cuvidDestroyVideoSource(CUvideosource obj);

/******************************************************************************************/
//! \ingroup FUNCTS
//! \fn CUresult CUDAAPI cuvidSetVideoSourceState(CUvideosource obj, cudaVideoState state)
//! Set video source state to:
//! cudaVideoState_Started - to signal the source to run and deliver data
//! cudaVideoState_Stopped - to stop the source from delivering the data
//! cudaVideoState_Error   - invalid source
/******************************************************************************************/
CUresult CUDAAPI cuvidSetVideoSourceState(CUvideosource obj, cudaVideoState state);

/******************************************************************************************/
//! \ingroup FUNCTS
//! \fn cudaVideoState CUDAAPI cuvidGetVideoSourceState(CUvideosource obj)
//! Get video source state
//! Returns:
//! cudaVideoState_Started - if Source is running and delivering data
//! cudaVideoState_Stopped - if Source is stopped or reached end-of-stream
//! cudaVideoState_Error   - if Source is in error state
/******************************************************************************************/
cudaVideoState CUDAAPI cuvidGetVideoSourceState(CUvideosource obj);

/******************************************************************************************************************/
//! \ingroup FUNCTS
//! \fn CUresult CUDAAPI cuvidGetSourceVideoFormat(CUvideosource obj, CUVIDEOFORMAT *pvidfmt, unsigned int flags)
//! Gets video source format in pvidfmt, flags is set to combination of CUvideosourceformat_flags as per requirement
/******************************************************************************************************************/
CUresult CUDAAPI cuvidGetSourceVideoFormat(CUvideosource obj, CUVIDEOFORMAT *pvidfmt, unsigned int flags);

/**************************************************************************************************************************/
//! \ingroup FUNCTS
//! \fn CUresult CUDAAPI cuvidGetSourceAudioFormat(CUvideosource obj, CUAUDIOFORMAT *paudfmt, unsigned int flags)
//! Get audio source format
//! NVDECODE API is intended for HW accelerated video decoding so CUvideosource doesn't have audio demuxer for all supported 
//! containers. It's recommended to clients to use their own or third party demuxer if audio support is needed.
/**************************************************************************************************************************/
CUresult CUDAAPI cuvidGetSourceAudioFormat(CUvideosource obj, CUAUDIOFORMAT *paudfmt, unsigned int flags);

#endif
/**********************************************************************************/
//! \ingroup STRUCTS
//! \struct CUVIDPARSERDISPINFO
//! Used in cuvidParseVideoData API with PFNVIDDISPLAYCALLBACK pfnDisplayPicture
/**********************************************************************************/
typedef struct _CUVIDPARSERDISPINFO
{
    int picture_index;          /**< OUT: Index of the current picture                                                         */
    int progressive_frame;      /**< OUT: 1 if progressive frame; 0 otherwise                                                  */
    int top_field_first;        /**< OUT: 1 if top field is displayed first; 0 otherwise                                       */
    int repeat_first_field;     /**< OUT: Number of additional fields (1=ivtc, 2=frame doubling, 4=frame tripling, 
                                     -1=unpaired field)                                                                        */
    CUvideotimestamp timestamp; /**< OUT: Presentation time stamp                                                              */
} CUVIDPARSERDISPINFO;

/***********************************************************************************************************************/
//! Parser callbacks
//! The parser will call these synchronously from within cuvidParseVideoData(), whenever there is sequence change or a picture
//! is ready to be decoded and/or displayed. First argument in functions is "void *pUserData" member of structure CUVIDSOURCEPARAMS
//! Return values from these callbacks are interpreted as below. If the callbacks return failure, it will be propagated by
//! cuvidParseVideoData() to the application.
//! PFNVIDSEQUENCECALLBACK : 0: fail, 1: succeeded, > 1: override dpb size of parser (set by CUVIDPARSERPARAMS::ulMaxNumDecodeSurfaces
//! while creating parser)
//! PFNVIDDECODECALLBACK   : 0: fail, >=1: succeeded
//! PFNVIDDISPLAYCALLBACK  : 0: fail, >=1: succeeded
/***********************************************************************************************************************/
typedef int (CUDAAPI *PFNVIDSEQUENCECALLBACK)(void *, CUVIDEOFORMAT *);
typedef int (CUDAAPI *PFNVIDDECODECALLBACK)(void *, CUVIDPICPARAMS *);
typedef int (CUDAAPI *PFNVIDDISPLAYCALLBACK)(void *, CUVIDPARSERDISPINFO *);

/**************************************/
//! \ingroup STRUCTS
//! \struct CUVIDPARSERPARAMS
//! Used in cuvidCreateVideoParser API
/**************************************/
typedef struct _CUVIDPARSERPARAMS
{
    cudaVideoCodec CodecType;                   /**< IN: cudaVideoCodec_XXX                                                  */
    unsigned int ulMaxNumDecodeSurfaces;        /**< IN: Max # of decode surfaces (parser will cycle through these)          */
    unsigned int ulClockRate;                   /**< IN: Timestamp units in Hz (0=default=10000000Hz)                        */
    unsigned int ulErrorThreshold;              /**< IN: % Error threshold (0-100) for calling pfnDecodePicture (100=always 
                                                     IN: call pfnDecodePicture even if picture bitstream is fully corrupted) */
    unsigned int ulMaxDisplayDelay;             /**< IN: Max display queue delay (improves pipelining of decode with display)
                                                         0=no delay (recommended values: 2..4)                               */
    unsigned int uReserved1[5];                 /**< IN: Reserved for future use - set to 0                                  */
    void *pUserData;                            /**< IN: User data for callbacks                                             */
    PFNVIDSEQUENCECALLBACK pfnSequenceCallback; /**< IN: Called before decoding frames and/or whenever there is a fmt change */
    PFNVIDDECODECALLBACK pfnDecodePicture;      /**< IN: Called when a picture is ready to be decoded (decode order)         */
    PFNVIDDISPLAYCALLBACK pfnDisplayPicture;    /**< IN: Called whenever a picture is ready to be displayed (display order)  */
    void *pvReserved2[7];                       /**< Reserved for future use - set to NULL                                   */
    CUVIDEOFORMATEX *pExtVideoInfo;             /**< IN: [Optional] sequence header data from system layer                   */
} CUVIDPARSERPARAMS;

/************************************************************************************************/
//! \ingroup FUNCTS
//! \fn CUresult CUDAAPI cuvidCreateVideoParser(CUvideoparser *pObj, CUVIDPARSERPARAMS *pParams)
//! Create video parser object and initialize
/************************************************************************************************/
CUresult CUDAAPI cuvidCreateVideoParser(CUvideoparser *pObj, CUVIDPARSERPARAMS *pParams);

/************************************************************************************************/
//! \ingroup FUNCTS
//! \fn CUresult CUDAAPI cuvidParseVideoData(CUvideoparser obj, CUVIDSOURCEDATAPACKET *pPacket)
//! Parse the video data from source data packet in pPacket 
//! Extracts parameter sets like SPS, PPS, bitstream etc. from pPacket and 
//! calls back pfnDecodePicture with CUVIDPICPARAMS data for kicking of HW decoding
//! calls back pfnSequenceCallback with CUVIDEOFORMAT data for initial sequence header or when
//! the decoder encounters a video format change
//! calls back pfnDisplayPicture with CUVIDPARSERDISPINFO data to display a video frame
/************************************************************************************************/
CUresult CUDAAPI cuvidParseVideoData(CUvideoparser obj, CUVIDSOURCEDATAPACKET *pPacket);

/************************************************************************************************/
//! \ingroup FUNCTS
//! \fn CUresult CUDAAPI cuvidDestroyVideoParser(CUvideoparser obj)
//! Destroy the video parser
/************************************************************************************************/
CUresult CUDAAPI cuvidDestroyVideoParser(CUvideoparser obj);

/**********************************************************************************************/

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif // __NVCUVID_H__




/**
* @brief Exception class for error reporting from the decode API.
*/
class NVDECException : public std::exception
{
public:
    NVDECException(const std::string& errorStr, const CUresult errorCode)
        : m_errorString(errorStr), m_errorCode(errorCode) {}

    virtual ~NVDECException() throw() {}
    virtual const char* what() const throw() { return m_errorString.c_str(); }
    CUresult  getErrorCode() const { return m_errorCode; }
    const std::string& getErrorString() const { return m_errorString; }
    static NVDECException makeNVDECException(const std::string& errorStr, const CUresult errorCode,
        const std::string& functionName, const std::string& fileName, int lineNo);
private:
    std::string m_errorString;
    CUresult m_errorCode;
};

inline NVDECException NVDECException::makeNVDECException(const std::string& errorStr, const CUresult errorCode, const std::string& functionName,
    const std::string& fileName, int lineNo)
{
    std::ostringstream errorLog;
    errorLog << functionName << " : " << errorStr << " at " << fileName << ":" << lineNo << std::endl;
    NVDECException exception(errorLog.str(), errorCode);
    return exception;
}

#define NVDEC_THROW_ERROR( errorStr, errorCode )                                                         \
    do                                                                                                   \
    {                                                                                                    \
        throw NVDECException::makeNVDECException(errorStr, errorCode, __FUNCTION__, __FILE__, __LINE__); \
    } while (0)


#define NVDEC_API_CALL( cuvidAPI )                                                                                 \
    do                                                                                                             \
    {                                                                                                              \
        CUresult errorCode = cuvidAPI;                                                                             \
        if( errorCode != CUDA_SUCCESS)                                                                             \
        {                                                                                                          \
            std::ostringstream errorLog;                                                                           \
            errorLog << #cuvidAPI << " returned error " << errorCode;                                              \
            throw NVDECException::makeNVDECException(errorLog.str(), errorCode, __FUNCTION__, __FILE__, __LINE__); \
        }                                                                                                          \
    } while (0)

struct Rect {
    int l, t, r, b;
};

struct Dim {
    int w, h;
};

/**
* @brief Base class for decoder interface.
*/
class NvDecoder {

public:
    typedef enum
    {
        // return decompressed image as it is - write planar output
        IMAGE_UNCHANGED   = 0,
        // return planar luma and chroma, assuming YCbCr colorspace
        IMAGE_YUV         = 1,
        // return luma component only, if YCbCr colorspace,
        // or try to convert to grayscale,
        // writes to 1-st channel of nvjpegImage_t
        IMAGE_Y           = 2,
        // convert to planar RGB
        IMAGE_RGB         = 3,
        // convert to planar BGR
        IMAGE_BGR         = 4,
        // convert to interleaved RGB and write to 1-st channel of nvjpegImage_t
        IMAGE_RGBI        = 5,
        // convert to interleaved BGR and write to 1-st channel of nvjpegImage_t
        IMAGE_BGRI        = 6,
        IMAGE_NV12         = 7,
        // maximum allowed value
        IMAGE_FORMAT_MAX  = 7
    } ImageFormat_t;
    ImageFormat_t oformat = IMAGE_NV12;

    /**
    *  @brief This function is used to initialize the decoder session.
    *  Application must call this function to initialize the decoder, before
    *  starting to decode any frames.
    */
    NvDecoder(uint16_t instanceId, Rect *pCropRect = NULL, Dim  *pResizeDim = NULL, CUcontext cuContext = NULL);
    ~NvDecoder();

    void nvCreateParser(CUcontext cuContext, bool bUseDeviceFrame,
                        cudaVideoCodec eCodec, std::mutex *pMutex = NULL,
                        bool bLowLatency = false,
                        bool bDeviceFramePitched = false,
                        const Rect *pCropRect = NULL,
                        const Dim *pResizeDim = NULL,
                        int maxWidth = 0, int maxHeight = 0);

    CUcontext getContext() { return m_cuContext; }

    /**
    *  @brief  This function is used to get the current decode width.
    */
    int getWidth() { assert(m_nWidth); return m_nWidth; }

    /**
    *  @brief  This function is used to get the current decode height (Luma height).
    */
    int getHeight() { assert(m_nLumaHeight); return m_nLumaHeight; }
    int getChromaHeight() { assert(m_nChromaHeight); return m_nChromaHeight; }
    int getNumChromaPlanes() {
        assert(m_nNumChromaPlanes); return m_nNumChromaPlanes;
    }

    /**
    *   @brief  This function is used to get the current frame size based on pixel format.
    */
    int getFrameSize() {
        assert(m_nWidth);
        return (m_nWidth * m_nBPP) * (m_nLumaHeight + m_nChromaHeight * m_nNumChromaPlanes);
    }

    /**
    *  @brief  This function is used to get the pitch(stride) of the device buffer holding the decoded frame.
    */
    int getDeviceFramePitch() {
        assert(m_nWidth);
        return m_nDeviceFramePitch ? (int)m_nDeviceFramePitch : m_nWidth * m_nBPP; }

    /**
    *   @brief  This function is used to get the bit depth associated with the pixel format.
    */
    int getBitDepth() { assert(m_nWidth); return m_nBitDepthMinus8 + 8; }

    /**
    *   @brief  This function is used to get the bytes used per pixel.
    */
    int getBPP() { assert(m_nWidth); return m_nBPP; }

    /**
    *   @brief  This function is used to get the YUV chroma format
    */
    cudaVideoSurfaceFormat getOutputFormat() { return m_eOutputFormat; }
    CUVIDEOFORMAT getVideoFormatInfo() { assert(m_nWidth); return m_videoFormat; }

    /**
    *   @brief  This function is used to print information about the video stream
    */
    std::string getVideoInfo() const { return m_videoInfo.str(); }

    /**
    *   @brief  This function decodes a frame and returns frames that are available for display.
        The frames should be used or buffered before making subsequent calls to the Decode function again
    *   @param  bitstream - pointer to the data buffer that is to be decoded
    *   @param  nSize - size of the data buffer in bytes
    *   @param  pppFrame - CUvideopacketflags for setting decode options
    *   @param  pnFrameReturned	 - pointer to array of decoded frames that are returned
    *   @param  flags - CUvideopacketflags for setting decode options	
    *   @param  ppTimestamp - pointer to array of timestamps for decoded frames that are returned
    *   @param  timestamp - presentation timestamp
    *   @param  stream - CUstream to be used for post-processing operations
    */
    int decode(const uint8_t *bitstream, int bitstreamBytes,
               uint8_t ***pppFrame, int *pnFrameReturned, uint32_t flags = 0, int64_t **ppTimestamp = NULL, int64_t timestamp = 0, CUstream stream = 0);

    int decode_lockFrame(const uint8_t *pData, int nSize, uint8_t ***pppFrame, int *pnFrameReturned, uint32_t flags = 0, int64_t **ppTimestamp = NULL, int64_t timestamp = 0, CUstream stream = 0);

    void unlockFrame(uint8_t **ppFrame, int nFrame);

    int setReconfigParams(const Rect * pCropRect, const Dim * pResizeDim);


    /**
    *   @brief  ISR when decoding of sequence starts
    */
    static int CUDAAPI handleNvSequenceIsr(void *self, CUVIDEOFORMAT *pVideoFormat) {
        return ((NvDecoder *)self)->handleNvSequence(pVideoFormat);
    }
    int handleNvSequence(CUVIDEOFORMAT *pVideoFormat);

    /**
    *   @brief  Callback function to be registered for getting a callback when a decoded frame is ready to be decoded
    */
    static int CUDAAPI handleNvDecodeIsr(void *pUserData, CUVIDPICPARAMS *pPicParams) {
        return ((NvDecoder *)pUserData)->handleNvDecode(pPicParams);
    }
    int handleNvDecode(CUVIDPICPARAMS *pPicParams);

    /**
    *   @brief  Callback function to be registered for getting a callback when a decoded frame is available for display
    *   Frames are fetched and stored in internal buffer
    */
    static int CUDAAPI handleNvPostProcIsr(void *pUserData, CUVIDPARSERDISPINFO *pDispInfo) {
        return ((NvDecoder *)pUserData)->handleNvPostProc(pDispInfo);
    }
    int handleNvPostProc(CUVIDPARSERDISPINFO *pDispInfo);

    /**
    *   @brief  This function reconfigure decoder if there is a change in sequence params.
    */
    int nvCreateDecoder(CUVIDEOFORMAT *pVideoFormat);
    int nvReconfigureDecoder(CUVIDEOFORMAT *pVideoFormat);
    std::string getCodecString(cudaVideoCodec eCodec) {
        static struct {
            cudaVideoCodec eCodec;
            std::string name;
        } aCodecName [] = {
            { cudaVideoCodec_MPEG1,     "MPEG-1"       },
            { cudaVideoCodec_MPEG2,     "MPEG-2"       },
            { cudaVideoCodec_MPEG4,     "MPEG-4 (ASP)" },
            { cudaVideoCodec_VC1,       "VC-1/WMV"     },
            { cudaVideoCodec_H264,      "AVC/H.264"    },
            { cudaVideoCodec_JPEG,      "M-JPEG"       },
            { cudaVideoCodec_H264_SVC,  "H.264/SVC"    },
            { cudaVideoCodec_H264_MVC,  "H.264/MVC"    },
            { cudaVideoCodec_HEVC,      "H.265/HEVC"   },
            { cudaVideoCodec_VP8,       "VP8"          },
            { cudaVideoCodec_VP9,       "VP9"          },
            { cudaVideoCodec_NumCodecs, "Invalid"      },
            { cudaVideoCodec_YUV420,    "YUV  4:2:0"   },
            { cudaVideoCodec_YV12,      "YV12 4:2:0"   },
            { cudaVideoCodec_NV12,      "NV12 4:2:0"   },
            { cudaVideoCodec_YUYV,      "YUYV 4:2:2"   },
            { cudaVideoCodec_UYVY,      "UYVY 4:2:2"   },
        };

        if (eCodec >= 0 && eCodec <= cudaVideoCodec_NumCodecs) {
            return aCodecName[eCodec].name;
        }
        for (int i = cudaVideoCodec_NumCodecs + 1; i < sizeof(aCodecName) / sizeof(aCodecName[0]); i++) {
            if (eCodec == aCodecName[i].eCodec) {
                return aCodecName[eCodec].name;
            }
        }
        return "Unknown";
    }

    std::string getChromaString(cudaVideoChromaFormat eChromaFormat) {
        static struct {
            cudaVideoChromaFormat eChromaFormat;
            std::string name;
        } aChromaFormatName[] = {
            { cudaVideoChromaFormat_Monochrome, "YUV 400 (Monochrome)" },
            { cudaVideoChromaFormat_420,        "YUV 420"              },
            { cudaVideoChromaFormat_422,        "YUV 422"              },
            { cudaVideoChromaFormat_444,        "YUV 444"              },
        };

        if (eChromaFormat >= 0 && eChromaFormat < sizeof(aChromaFormatName) / sizeof(aChromaFormatName[0])) {
            return aChromaFormatName[eChromaFormat].name;
        }
        return "Unknown";
    }

    float getChromaHeightFactor(cudaVideoChromaFormat eChromaFormat) {
        float factor = 0.5;
        switch (eChromaFormat)
        {
        case cudaVideoChromaFormat_Monochrome:
            factor = 0.0;
            break;
        case cudaVideoChromaFormat_420:
            factor = 0.5;
            break;
        case cudaVideoChromaFormat_422:
            factor = 1.0;
            break;
        case cudaVideoChromaFormat_444:
            factor = 1.0;
            break;
        }

        return factor;
    }

    int getChromaPlaneCount(cudaVideoChromaFormat eChromaFormat) {
        int numPlane = 1;
        switch (eChromaFormat)
        {
        case cudaVideoChromaFormat_Monochrome:
            numPlane = 0;
            break;
        case cudaVideoChromaFormat_420:
            numPlane = 1;
            break;
        case cudaVideoChromaFormat_444:
            numPlane = 2;
            break;
        }

        return numPlane;
    }

    int nDecodeSurface = 0;
    int iGpu = 0;
    CUdevice cuDevice = 0;
    int nGpu = 0;
    char m_deviceName[80];
    CUcontext m_selfCtx = NULL;
    CUcontext m_cuContext = NULL;
    CUvideoctxlock m_ctxLock;
    std::mutex *m_pMutex;
    CUvideoparser m_hParser = NULL;
    CUvideodecoder m_hDecoder = NULL;
    bool m_bUseDeviceFrame;
    // dimension of the output
    unsigned int m_nWidth = 0, m_nLumaHeight = 0, m_nChromaHeight = 0;
    unsigned int m_nNumChromaPlanes = 0;
    // height of the mapped surface 
    //int m_nSurfaceChromaHeight = 0;
    float m_chromaHeight_factor = 0.0;
    int m_nSurfaceHeight = 0;
    int m_nSurfaceWidth = 0;
    cudaVideoCodec m_eCodec = cudaVideoCodec_NumCodecs;
    cudaVideoChromaFormat m_eChromaFormat;
    cudaVideoSurfaceFormat m_eOutputFormat;
    int m_nBitDepthMinus8 = 0;
    int m_nBPP = 1;
    CUVIDEOFORMAT m_videoFormat = {};
    Rect m_displayRect = {};


    void* m_d_RGBi_frame = NULL;
    void* m_d_RGBp_frame = NULL;

    std::mutex               m_mtxVPFrame;
    std::vector<uint8_t *>   m_vpFrame; // post processed frame ptrs
    std::vector<int64_t>     m_vTimestamp;
    std::vector<uint8_t *>   m_vpFrameRet; // returned frame ptrs

    int m_nDecodedFrame = 0, m_nDecodedFrameReturned = 0;
    int m_nDecodePicCnt = 0, m_nPicNumInDecodeOrder[32];
    bool m_bEndDecodeDone = false;
    int m_nFrameAlloc = 0;
    CUstream m_cuvidStream = 0;
    bool m_bDeviceFramePitched = false;
    size_t m_nDeviceFramePitch = 0;
    Rect m_cropRect = {};
    Dim m_resizeDim = {};

    std::ostringstream m_videoInfo;
    unsigned int m_nMaxWidth = 0, m_nMaxHeight = 0;
    bool m_bReconfigExternal = false;
    bool m_bReconfigExtPPChange = false;
};

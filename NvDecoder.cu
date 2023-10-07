#include <iostream>
#include <algorithm>
#include <chrono>
#include <npp.h>
#include "NvDecoder.hpp"



#define __D(fmt, args...) printf("" fmt, ## args)
#define __I(fmt, args...) printf("" fmt, ## args)
#define __E(fmt, args...) fprintf(stderr, "" fmt, ## args)

#define _stricmp strcasecmp
#define _stat64 stat64

#ifndef ck
#define ck(call) check((call), #call, __FILE__, __LINE__)
template <typename T>
void check(T result, char const *const func, const char *const file,
           int const line) {
    if (result) {
        fprintf(stderr, "Decoder error at %s:%d code=%d \"%s\" \n", file, line,
                static_cast<unsigned int>(result), func);
        exit(EXIT_FAILURE);
    }
}
#endif


#define START_TIMER auto start = std::chrono::high_resolution_clock::now();
#define STOP_TIMER(print_message) std::cout << print_message << \
    std::chrono::duration_cast<std::chrono::milliseconds>( \
    std::chrono::high_resolution_clock::now() - start).count() \
    << " ms " << std::endl;

#define CUDA_DRVAPI_CALL( call )                                                                                                 \
    do                                                                                                                           \
    {                                                                                                                            \
        CUresult err__ = call;                                                                                                   \
        if (err__ != CUDA_SUCCESS)                                                                                               \
        {                                                                                                                        \
            const char *szErrName = NULL;                                                                                        \
            cuGetErrorName(err__, &szErrName);                                                                                   \
            std::ostringstream errorLog;                                                                                         \
            errorLog << "CUDA driver API error " << szErrName ;                                                                  \
            throw NVDECException::makeNVDECException(errorLog.str(), err__, __FUNCTION__, __FILE__, __LINE__);                   \
        }                                                                                                                        \
    }                                                                                                                            \
    while (0)





/* Return value from HandleVideoSequence() are interpreted as   :
*  0: fail, 1: succeeded, > 1: override dpb size of parser (set by CUVIDPARSERPARAMS::ulMaxNumDecodeSurfaces while creating parser)
*/
int NvDecoder::handleNvSequence(CUVIDEOFORMAT *pVideoFormat)
{
    START_TIMER
    m_videoInfo.str("");
    m_videoInfo.clear();
    m_videoInfo << "Video Input Information" << std::endl
        << "\tCodec        : " << getCodecString(pVideoFormat->codec) << std::endl
        << "\tFrame rate   : " << pVideoFormat->frame_rate.numerator << "/" << pVideoFormat->frame_rate.denominator
            << " = " << 1.0 * pVideoFormat->frame_rate.numerator / pVideoFormat->frame_rate.denominator << " fps" << std::endl
        << "\tSequence     : " << (pVideoFormat->progressive_sequence ? "Progressive" : "Interlaced") << std::endl
        << "\tCoded size   : [" << pVideoFormat->coded_width << ", " << pVideoFormat->coded_height << "]" << std::endl
        << "\tDisplay area : [" << pVideoFormat->display_area.left << ", " << pVideoFormat->display_area.top << ", "
            << pVideoFormat->display_area.right << ", " << pVideoFormat->display_area.bottom << "]" << std::endl
        << "\tChroma       : " << getChromaString(pVideoFormat->chroma_format) << std::endl
        << "\tBit depth    : " << pVideoFormat->bit_depth_luma_minus8 + 8
    ;
    m_videoInfo << std::endl;

    nDecodeSurface = pVideoFormat->min_num_decode_surfaces;

    CUVIDDECODECAPS decodecaps = {
        .eCodecType      = pVideoFormat->codec,
        .eChromaFormat   = pVideoFormat->chroma_format,
        .nBitDepthMinus8 = pVideoFormat->bit_depth_luma_minus8
    };

    CUDA_DRVAPI_CALL(cuCtxPushCurrent(m_cuContext));
    NVDEC_API_CALL(cuvidGetDecoderCaps(&decodecaps));
    CUDA_DRVAPI_CALL(cuCtxPopCurrent(NULL));

    if(!decodecaps.bIsSupported) {
        NVDEC_THROW_ERROR("Codec not supported on this GPU", CUDA_ERROR_NOT_SUPPORTED);
        return nDecodeSurface;
    }

    if ((pVideoFormat->coded_width > decodecaps.nMaxWidth) ||
        (pVideoFormat->coded_height > decodecaps.nMaxHeight)){

        std::ostringstream errorString;
        errorString << std::endl
                    << "Resolution          : " << pVideoFormat->coded_width << "x" << pVideoFormat->coded_height << std::endl
                    << "Max Supported (wxh) : " << decodecaps.nMaxWidth << "x" << decodecaps.nMaxHeight << std::endl
                    << "Resolution not supported on this GPU";

        const std::string cErr = errorString.str();
        NVDEC_THROW_ERROR(cErr, CUDA_ERROR_NOT_SUPPORTED);
        return nDecodeSurface;
    }

    if ((pVideoFormat->coded_width>>4)*(pVideoFormat->coded_height>>4) > decodecaps.nMaxMBCount){

        std::ostringstream errorString;
        errorString << std::endl
                    << "MBCount             : " << (pVideoFormat->coded_width >> 4)*(pVideoFormat->coded_height >> 4) << std::endl
                    << "Max Supported mbcnt : " << decodecaps.nMaxMBCount << std::endl
                    << "MBCount not supported on this GPU";

        const std::string cErr = errorString.str();
        NVDEC_THROW_ERROR(cErr, CUDA_ERROR_NOT_SUPPORTED);
        return nDecodeSurface;
    }

    if (m_nWidth && m_nLumaHeight && m_nChromaHeight) {

        // cuvidCreateDecoder() has been called before, and now there's possible config change
        return nvReconfigureDecoder(pVideoFormat);
    }

    nvCreateDecoder(pVideoFormat);

    STOP_TIMER("Session Initialization Time: ");
    return nDecodeSurface;
}

int NvDecoder::nvCreateDecoder(CUVIDEOFORMAT *pVideoFormat)
{
    // eCodec has been set in the constructor (for parser). Here it's set again for potential correction
    m_eCodec          = pVideoFormat->codec;
    m_eChromaFormat   = pVideoFormat->chroma_format;
    m_nBitDepthMinus8 = pVideoFormat->bit_depth_luma_minus8;
    m_nBPP            = m_nBitDepthMinus8 > 0 ? 2 : 1;

    if (m_eChromaFormat == cudaVideoChromaFormat_420) {
        m_eOutputFormat = pVideoFormat->bit_depth_luma_minus8 ? cudaVideoSurfaceFormat_P016 : cudaVideoSurfaceFormat_NV12;
    } else if (m_eChromaFormat == cudaVideoChromaFormat_444) {
        m_eOutputFormat = pVideoFormat->bit_depth_luma_minus8 ? cudaVideoSurfaceFormat_YUV444_16Bit : cudaVideoSurfaceFormat_YUV444;
    }
    m_videoFormat = *pVideoFormat;

    CUVIDDECODECREATEINFO nvI = { 0 };
    nvI.CodecType      = pVideoFormat->codec;
    nvI.ChromaFormat   = pVideoFormat->chroma_format;
    nvI.OutputFormat   = m_eOutputFormat;
    nvI.bitDepthMinus8 = pVideoFormat->bit_depth_luma_minus8;
    if (pVideoFormat->progressive_sequence)
        nvI.DeinterlaceMode = cudaVideoDeinterlaceMode_Weave;
    else
        nvI.DeinterlaceMode = cudaVideoDeinterlaceMode_Adaptive;
    nvI.ulNumOutputSurfaces = 2;
    // With PreferCUVID, JPEG is still decoded by CUDA while video is decoded by NVDEC hardware
    nvI.ulCreationFlags     = cudaVideoCreate_PreferCUVID;
    nvI.ulNumDecodeSurfaces = nDecodeSurface;
    nvI.vidLock  = m_ctxLock;
    nvI.ulWidth  = pVideoFormat->coded_width;
    nvI.ulHeight = pVideoFormat->coded_height;
    if (m_nMaxWidth < (int)pVideoFormat->coded_width)
        m_nMaxWidth = pVideoFormat->coded_width;
    if (m_nMaxHeight < (int)pVideoFormat->coded_height)
        m_nMaxHeight = pVideoFormat->coded_height;
    nvI.ulMaxWidth  = m_nMaxWidth;
    nvI.ulMaxHeight = m_nMaxHeight;

    if (!(m_cropRect.r && m_cropRect.b) &&
        !(m_resizeDim.w && m_resizeDim.h))
    {
        m_nWidth      = pVideoFormat->display_area.right - pVideoFormat->display_area.left;
        m_nLumaHeight = pVideoFormat->display_area.bottom - pVideoFormat->display_area.top;
        nvI.ulTargetWidth  = pVideoFormat->coded_width;
        nvI.ulTargetHeight = pVideoFormat->coded_height;
    }
    else
    {
        if (m_resizeDim.w && m_resizeDim.h) {
            nvI.display_area.left   = pVideoFormat->display_area.left;
            nvI.display_area.top    = pVideoFormat->display_area.top;
            nvI.display_area.right  = pVideoFormat->display_area.right;
            nvI.display_area.bottom = pVideoFormat->display_area.bottom;
            m_nWidth      = m_resizeDim.w;
            m_nLumaHeight = m_resizeDim.h;
        }

        __I("%d,%d,%d,%d. %dx%d\n",
            m_cropRect.l,
            m_cropRect.t,
            m_cropRect.r,
            m_cropRect.b,
            m_nWidth, m_nLumaHeight);

        if (m_cropRect.r && m_cropRect.b) {
            nvI.display_area.left    = m_cropRect.l;
            nvI.display_area.top     = m_cropRect.t;
            nvI.display_area.right   = m_cropRect.r;
            nvI.display_area.bottom  = m_cropRect.b;
            m_nWidth      = m_cropRect.r - m_cropRect.l;
            m_nLumaHeight = m_cropRect.b - m_cropRect.t;
        }
        nvI.ulTargetWidth  = m_nWidth;
        nvI.ulTargetHeight = m_nLumaHeight;
    }

    m_nSurfaceHeight = nvI.ulTargetHeight;
    m_nSurfaceWidth = nvI.ulTargetWidth;

    m_chromaHeight_factor = getChromaHeightFactor(nvI.ChromaFormat);
    m_nChromaHeight = (int)(m_nLumaHeight * m_chromaHeight_factor);
    m_nNumChromaPlanes = getChromaPlaneCount(nvI.ChromaFormat);
    //m_nSurfaceChromaHeight = (int)(m_nSurfaceHeight * m_chromaHeight_factor);

    m_displayRect.b = nvI.display_area.bottom;
    m_displayRect.t = nvI.display_area.top;
    m_displayRect.l = nvI.display_area.left;
    m_displayRect.r = nvI.display_area.right;


    m_videoInfo << "Video Decoding Params:" << std::endl
        << "\tNum Surfaces : " << nvI.ulNumDecodeSurfaces << std::endl
        << "\tCrop         : [" << nvI.display_area.left << ", " << nvI.display_area.top << ", "
        << nvI.display_area.right << ", " << nvI.display_area.bottom << "]" << std::endl
        << "\tResize       : " << nvI.ulTargetWidth << "x" << nvI.ulTargetHeight << std::endl
        << "\tDeinterlace  : " << std::vector<const char *>{"Weave", "Bob", "Adaptive"}[nvI.DeinterlaceMode]
    ;
    m_videoInfo << std::endl;

    CUDA_DRVAPI_CALL(cuCtxPushCurrent(m_cuContext));
    NVDEC_API_CALL(cuvidCreateDecoder(&m_hDecoder, &nvI));
    CUDA_DRVAPI_CALL(cuCtxPopCurrent(NULL));
    return 0;
}

int NvDecoder::nvReconfigureDecoder(CUVIDEOFORMAT *pVideoFormat)
{
    if (pVideoFormat->bit_depth_luma_minus8   != m_videoFormat.bit_depth_luma_minus8 ||
        pVideoFormat->bit_depth_chroma_minus8 != m_videoFormat.bit_depth_chroma_minus8)
    {
        NVDEC_THROW_ERROR("Reconfigure Not supported for bit depth change", CUDA_ERROR_NOT_SUPPORTED);
    }

    if (pVideoFormat->chroma_format != m_videoFormat.chroma_format) {
        NVDEC_THROW_ERROR("Reconfigure Not supported for chroma format change", CUDA_ERROR_NOT_SUPPORTED);
    }

    bool bDecodeResChange   = !(pVideoFormat->coded_width == m_videoFormat.coded_width &&
                                pVideoFormat->coded_height == m_videoFormat.coded_height);

    bool bDisplayRectChange = !(pVideoFormat->display_area.bottom == m_videoFormat.display_area.bottom &&
                                pVideoFormat->display_area.top    == m_videoFormat.display_area.top &&
                                pVideoFormat->display_area.left   == m_videoFormat.display_area.left &&
                                pVideoFormat->display_area.right  == m_videoFormat.display_area.right);

    nDecodeSurface = pVideoFormat->min_num_decode_surfaces;

    if ((pVideoFormat->coded_width > m_nMaxWidth) ||
        (pVideoFormat->coded_height > m_nMaxHeight))
    {
        // For VP9, let driver  handle the change if new width/height > maxwidth/maxheight
        if ((m_eCodec != cudaVideoCodec_VP9) || m_bReconfigExternal) {
            NVDEC_THROW_ERROR("Reconfigure Not supported when width/height > maxwidth/maxheight", CUDA_ERROR_NOT_SUPPORTED);
        }
        return 1;
    }

    if (!bDecodeResChange && !m_bReconfigExtPPChange) {
        // if the coded_width/coded_height hasn't changed but display resolution has changed, then need to update width/height for
        // correct output without cropping. Example : 1920x1080 vs 1920x1088
        if (bDisplayRectChange)
        {
            m_nWidth           = pVideoFormat->display_area.right - pVideoFormat->display_area.left;
            m_nLumaHeight      = pVideoFormat->display_area.bottom - pVideoFormat->display_area.top;
            m_nChromaHeight    = int(m_nLumaHeight * getChromaHeightFactor(pVideoFormat->chroma_format));
            m_nNumChromaPlanes = getChromaPlaneCount(pVideoFormat->chroma_format);
        }

        // no need for reconfigureDecoder(). Just return
        return 1;
    }

    CUVIDRECONFIGUREDECODERINFO nvC = { 0 };

    nvC.ulWidth = m_videoFormat.coded_width = pVideoFormat->coded_width;
    nvC.ulHeight = m_videoFormat.coded_height = pVideoFormat->coded_height;

    // Dont change display rect and get scaled output from decoder. This will help display app to present apps smoothly
    nvC.display_area.bottom = m_displayRect.b;
    nvC.display_area.top    = m_displayRect.t;
    nvC.display_area.left   = m_displayRect.l;
    nvC.display_area.right  = m_displayRect.r;
    nvC.ulTargetWidth  = m_nSurfaceWidth;
    nvC.ulTargetHeight = m_nSurfaceHeight;

    // If external reconfigure is called along with resolution change even if post processing params is not changed,
    // do full reconfigure params update
    if ((m_bReconfigExternal && bDecodeResChange) ||
        m_bReconfigExtPPChange)
    {
        // update display rect and target resolution if requested explicitely
        m_bReconfigExternal    = false;
        m_bReconfigExtPPChange = false;
        m_videoFormat = *pVideoFormat;

        if (!(m_cropRect.r && m_cropRect.b) &&
            !(m_resizeDim.w && m_resizeDim.h))
        {
            m_nWidth      = pVideoFormat->display_area.right - pVideoFormat->display_area.left;
            m_nLumaHeight = pVideoFormat->display_area.bottom - pVideoFormat->display_area.top;
            nvC.ulTargetWidth  = pVideoFormat->coded_width;
            nvC.ulTargetHeight = pVideoFormat->coded_height;
        }
        else
        {
            if (m_resizeDim.w && m_resizeDim.h) {
                nvC.display_area.left   = pVideoFormat->display_area.left;
                nvC.display_area.top    = pVideoFormat->display_area.top;
                nvC.display_area.right  = pVideoFormat->display_area.right;
                nvC.display_area.bottom = pVideoFormat->display_area.bottom;
                m_nWidth      = m_resizeDim.w;
                m_nLumaHeight = m_resizeDim.h;
            }

            if (m_cropRect.r && m_cropRect.b) {
                nvC.display_area.left    = m_cropRect.l;
                nvC.display_area.top     = m_cropRect.t;
                nvC.display_area.right   = m_cropRect.r;
                nvC.display_area.bottom  = m_cropRect.b;
                m_nWidth      = m_cropRect.r - m_cropRect.l;
                m_nLumaHeight = m_cropRect.b - m_cropRect.t;
            }
            nvC.ulTargetWidth  = m_nWidth;
            nvC.ulTargetHeight = m_nLumaHeight;
        }

        m_nChromaHeight = int(m_nLumaHeight * getChromaHeightFactor(pVideoFormat->chroma_format));
        m_nNumChromaPlanes = getChromaPlaneCount(pVideoFormat->chroma_format);
        m_nSurfaceHeight = nvC.ulTargetHeight;
        m_nSurfaceWidth = nvC.ulTargetWidth;
        m_displayRect.b = nvC.display_area.bottom;
        m_displayRect.t = nvC.display_area.top;
        m_displayRect.l = nvC.display_area.left;
        m_displayRect.r = nvC.display_area.right;
    }

    nvC.ulNumDecodeSurfaces = nDecodeSurface;

    START_TIMER
    CUDA_DRVAPI_CALL(cuCtxPushCurrent(m_cuContext));
    NVDEC_API_CALL(cuvidReconfigureDecoder(m_hDecoder, &nvC));
    CUDA_DRVAPI_CALL(cuCtxPopCurrent(NULL));
    STOP_TIMER("Session Reconfigure Time: ");

    return nDecodeSurface;
}

int NvDecoder::setReconfigParams(const Rect *pCropRect, const Dim *pResizeDim)
{
    m_bReconfigExternal    = true;
    m_bReconfigExtPPChange = false;

    if (pCropRect)
    {
        if (!((pCropRect->t == m_cropRect.t)  &&
              (pCropRect->l == m_cropRect.l)  &&
              (pCropRect->b == m_cropRect.b) &&
              (pCropRect->r == m_cropRect.r))) {
            m_bReconfigExtPPChange = true;
            m_cropRect = *pCropRect;
        }
    }

    if (pResizeDim)
    {
        if (!((pResizeDim->w == m_resizeDim.w) &&
              (pResizeDim->h == m_resizeDim.h))) {
            m_bReconfigExtPPChange = true;
            m_resizeDim = *pResizeDim;
        }
    }

    // Clear existing output buffers of different size
    uint8_t *pFrame = NULL;
    while (!m_vpFrame.empty())
    {
        pFrame = m_vpFrame.back();
        m_vpFrame.pop_back();
        if (m_bUseDeviceFrame)
        {
            // d_frame
            CUDA_DRVAPI_CALL(cuCtxPushCurrent(m_cuContext));
            CUDA_DRVAPI_CALL(cuMemFree((CUdeviceptr)pFrame));
            CUDA_DRVAPI_CALL(cuCtxPopCurrent(NULL));
        }
        else
        {
            // h_frame
            delete pFrame;
        }
    }
    m_vpFrameRet.clear();

    return 0;
}

/* Return value from HandlePictureDecode() are interpreted as:
*  0: fail, >=1: succeeded
*/
int NvDecoder::handleNvDecode(CUVIDPICPARAMS *pPicParams) {
    if (!m_hDecoder)
    {
        NVDEC_THROW_ERROR("Decoder not initialized.", CUDA_ERROR_NOT_INITIALIZED);
        return 0;
    }
    m_nPicNumInDecodeOrder[pPicParams->CurrPicIdx] = m_nDecodePicCnt++;
    NVDEC_API_CALL(cuvidDecodePicture(m_hDecoder, pPicParams));
    return 1;
}

/* Return value from HandlePictureDisplay() are interpreted as:
*  0: fail, >=1: succeeded
*/
int NvDecoder::handleNvPostProc(CUVIDPARSERDISPINFO *pDispInfo) {
    CUVIDPROCPARAMS nvPr = {};
    nvPr.progressive_frame = pDispInfo->progressive_frame;
    nvPr.second_field      = pDispInfo->repeat_first_field + 1;
    nvPr.top_field_first   = pDispInfo->top_field_first;
    nvPr.unpaired_field    = pDispInfo->repeat_first_field < 0;
    nvPr.output_stream     = m_cuvidStream;

    CUdeviceptr  d_srcFrame = 0;
    unsigned int d_srcPitch = 0;

    NVDEC_API_CALL(cuvidMapVideoFrame(m_hDecoder,
                                      pDispInfo->picture_index,
                                      &d_srcFrame,
                                      &d_srcPitch,
                                      &nvPr));

    CUVIDGETDECODESTATUS nvS = {};
    CUresult result = cuvidGetDecodeStatus(m_hDecoder,
                                           pDispInfo->picture_index,
                                           &nvS);
    if (result == CUDA_SUCCESS &&
        (nvS.decodeStatus == cuvidDecodeStatus_Error ||
         nvS.decodeStatus == cuvidDecodeStatus_Error_Concealed))
    {
        printf("Decode Error occurred for picture %d\n", m_nPicNumInDecodeOrder[pDispInfo->picture_index]);
    }

    uint8_t *pDecodedFrame = nullptr;
    int frameSize; // = getFrameSize(); // NV12
    frameSize = d_srcPitch*m_nLumaHeight*3; // RGBI
    /* if (oformat == IMAGE_RGBI) { */
    /*     frameSize = d_srcPitch*m_nLumaHeight*3; */
    /* } */

    {

        std::lock_guard<std::mutex> lock(m_mtxVPFrame);
        if ((unsigned)++m_nDecodedFrame > m_vpFrame.size())
        {
            // Not enough frames in stock
            m_nFrameAlloc++;
            uint8_t *pFrame = NULL;
            if (m_bUseDeviceFrame)
            {
                // GPU DEVICE memory if m_bUseDeviceFrame:1
                CUDA_DRVAPI_CALL(cuCtxPushCurrent(m_cuContext));
                if (m_bDeviceFramePitched)
                {
                    CUDA_DRVAPI_CALL(cuMemAllocPitch((CUdeviceptr *)&pFrame,
                                                     &m_nDeviceFramePitch,
                                                     d_srcPitch*3, //m_nWidth * m_nBPP,
                                                     m_nLumaHeight*3, //m_nLumaHeight + (m_nChromaHeight * m_nNumChromaPlanes),
                                                     16));
                }
                else
                {
                    CUDA_DRVAPI_CALL(cuMemAlloc((CUdeviceptr *)&pFrame, frameSize));
                }
                CUDA_DRVAPI_CALL(cuCtxPopCurrent(NULL));
            }
            else
            {
                // CPU HOST memory if m_bUseDeviceFrame:0
                pFrame = new uint8_t[frameSize];
            }
            m_vpFrame.push_back(pFrame);
        }
        pDecodedFrame = m_vpFrame[m_nDecodedFrame - 1];
    }

    CUDA_DRVAPI_CALL(cuCtxPushCurrent(m_cuContext));
    // TODO start

    if (oformat == IMAGE_NV12) {
        CUDA_MEMCPY2D m = { 0 };
        m.srcMemoryType = CU_MEMORYTYPE_DEVICE;
        m.srcDevice     = d_srcFrame;
        m.srcPitch      = d_srcPitch;
        m.Height        = m_nLumaHeight;

        m.dstDevice     = (CUdeviceptr)(m.dstHost = pDecodedFrame);
        m.dstMemoryType = m_bUseDeviceFrame ? CU_MEMORYTYPE_DEVICE : CU_MEMORYTYPE_HOST;
        m.dstPitch      = m_nDeviceFramePitch ? m_nDeviceFramePitch : m_nWidth * m_nBPP;
        m.WidthInBytes  = m_nWidth * m_nBPP;
        CUDA_DRVAPI_CALL(cuMemcpy2DAsync(&m, m_cuvidStream));

        m.srcDevice = (CUdeviceptr)((uint8_t *)d_srcFrame + m.srcPitch * m_nSurfaceHeight);
        m.dstDevice = (CUdeviceptr)(m.dstHost = pDecodedFrame + m.dstPitch * m_nLumaHeight);
        m.Height = m_nChromaHeight; // TODO: (size_t)(m_nSurfaceHeight * m_chromaHeight_factor)
        CUDA_DRVAPI_CALL(cuMemcpy2DAsync(&m, m_cuvidStream));

        if (m_nNumChromaPlanes == 2)
        {
            m.srcDevice = (CUdeviceptr)((uint8_t *)d_srcFrame + m.srcPitch * m_nSurfaceHeight * 2); // TODO
            m.dstDevice = (CUdeviceptr)(m.dstHost = pDecodedFrame + m.dstPitch * m_nLumaHeight * 2); // TODO
            m.Height = m_nChromaHeight; // TODO
            CUDA_DRVAPI_CALL(cuMemcpy2DAsync(&m, m_cuvidStream));
        }
        CUDA_DRVAPI_CALL(cuStreamSynchronize(m_cuvidStream));
    } else {
        // 2RGBi
        if (m_d_RGBi_frame == NULL) {
            cudaMalloc(&m_d_RGBi_frame, d_srcPitch*m_nLumaHeight*3*sizeof(float));
        }


        NppiSize d_size = {(int)d_srcPitch, (int)m_nLumaHeight};
        Npp8u* y_plane = (Npp8u*)d_srcFrame;
        Npp8u* uv_plane = y_plane + d_srcPitch * m_nLumaHeight;
        Npp8u* src_planes[2] = {y_plane,uv_plane};
        nppiNV12ToRGB_8u_P2C3R(src_planes,
                               d_srcPitch,
                               (Npp8u*)m_d_RGBi_frame,
                               d_srcPitch*3,
                               d_size);

        if (oformat == IMAGE_RGBI)
        {
            CUDA_MEMCPY2D m = { 0 };
            m.srcMemoryType = CU_MEMORYTYPE_DEVICE;
            m.srcDevice     = (CUdeviceptr)m_d_RGBi_frame;
            m.srcPitch      = d_srcPitch*3;
            m.Height        = m_nLumaHeight;

            m.dstDevice     = (CUdeviceptr)(m.dstHost = pDecodedFrame);
            m.dstMemoryType = m_bUseDeviceFrame ? CU_MEMORYTYPE_DEVICE : CU_MEMORYTYPE_HOST;
            //m.dstPitch      = m_nDeviceFramePitch ? m_nDeviceFramePitch : m_nWidth * m_nBPP; //TODO: m_nDeviceFramePitch should be rgbi
            m.dstPitch      = m_nDeviceFramePitch ? m_nDeviceFramePitch : m_nWidth * 3;
            m.WidthInBytes  = m_nWidth * 3;
            CUDA_DRVAPI_CALL(cuMemcpy2DAsync(&m, m_cuvidStream));
            CUDA_DRVAPI_CALL(cuStreamSynchronize(m_cuvidStream));
        }
        else if (oformat == IMAGE_RGB)
        {
            // 2RGBp
            if (m_d_RGBp_frame == NULL) {
                cudaMalloc(&m_d_RGBp_frame, d_srcPitch*m_nLumaHeight*3*sizeof(float));
            }

            Npp8u* r_plane = (Npp8u*)m_d_RGBp_frame;
            Npp8u* g_plane = r_plane + d_srcPitch*m_nLumaHeight;
            Npp8u* b_plane = r_plane + d_srcPitch*m_nLumaHeight*2;
            Npp8u* rgb_planes[3] = {r_plane,g_plane,b_plane};
            nppiCopy_8u_C3P3R((Npp8u*)m_d_RGBi_frame,
                              d_srcPitch*3,
                              rgb_planes,
                              d_srcPitch,
                              d_size);

            CUDA_MEMCPY2D m = { 0 };
            m.srcMemoryType = CU_MEMORYTYPE_DEVICE;
            m.srcDevice     = (CUdeviceptr)m_d_RGBp_frame;
            m.srcPitch      = d_srcPitch;
            m.Height        = m_nLumaHeight*3;

            m.dstDevice     = (CUdeviceptr)(m.dstHost = pDecodedFrame);
            m.dstMemoryType = m_bUseDeviceFrame ? CU_MEMORYTYPE_DEVICE : CU_MEMORYTYPE_HOST;
            m.dstPitch      = m_nDeviceFramePitch ? m_nDeviceFramePitch : m_nWidth;
            m.WidthInBytes  = m_nWidth;
            CUDA_DRVAPI_CALL(cuMemcpy2DAsync(&m, m_cuvidStream));
            CUDA_DRVAPI_CALL(cuStreamSynchronize(m_cuvidStream));
        }



    }


    // TODO end
    CUDA_DRVAPI_CALL(cuCtxPopCurrent(NULL));

    if ((int)m_vTimestamp.size() < m_nDecodedFrame) {
        m_vTimestamp.resize(m_vpFrame.size());
    }
    m_vTimestamp[m_nDecodedFrame - 1] = pDispInfo->timestamp;

    NVDEC_API_CALL(cuvidUnmapVideoFrame(m_hDecoder, d_srcFrame));
    return 1;
}

NvDecoder::NvDecoder(uint16_t instanceId, Rect *pCropRect, Dim  *pResizeDim, CUcontext cuContext)
{

    iGpu = (int)instanceId;

    ck(cuInit(0));

    ck(cuDeviceGetCount(&nGpu));
    if (iGpu < 0 || iGpu >= nGpu) {
        __E("GPU Decoder instance out of range. Should be within[0, %d] \n",
            nGpu-1);
        return;
    }

    ck(cuDeviceGet(&cuDevice, iGpu));
    ck(cuDeviceGetName(m_deviceName, sizeof(m_deviceName), cuDevice));
    __I("GPU Decoder instance:%d in use: %s \n",
        iGpu, m_deviceName);

    if (cuContext == NULL) {
        ck(cuCtxCreate(&m_selfCtx, 0, cuDevice));
        cuContext = m_selfCtx;
    }
    nvCreateParser(cuContext,
                   false, // m_bUseDeviceFrame
                   cudaVideoCodec_H264,
                   NULL, false, false,
                   pCropRect, pResizeDim);
}

void NvDecoder::nvCreateParser(CUcontext cuContext,
                               bool bUseDeviceFrame,
                               cudaVideoCodec eCodec,
                               std::mutex *pMutex,
                               bool bLowLatency,
                               bool bDeviceFramePitched,
                               const Rect *pCropRect,
                               const Dim *pResizeDim,
                               int maxWidth,
                               int maxHeight)
{
    m_cuContext = cuContext;
    m_bUseDeviceFrame = bUseDeviceFrame;
    m_eCodec = eCodec;
    m_pMutex = pMutex;
    m_bDeviceFramePitched = bDeviceFramePitched;
    m_nMaxWidth = maxWidth;
    m_nMaxHeight = maxHeight;

    if (pCropRect) m_cropRect = *pCropRect;
    if (pResizeDim) m_resizeDim = *pResizeDim;

    NVDEC_API_CALL(cuvidCtxLockCreate(&m_ctxLock, cuContext));

    CUVIDPARSERPARAMS nvPa = {};
    nvPa.CodecType              = eCodec;
    nvPa.ulMaxNumDecodeSurfaces = 1;
    nvPa.ulMaxDisplayDelay      = bLowLatency ? 0 : 1;
    nvPa.pUserData           = this;
    nvPa.pfnSequenceCallback = handleNvSequenceIsr;
    nvPa.pfnDecodePicture    = handleNvDecodeIsr;
    nvPa.pfnDisplayPicture   = handleNvPostProcIsr;

    if (m_pMutex) m_pMutex->lock();
    NVDEC_API_CALL(cuvidCreateVideoParser(&m_hParser, &nvPa));
    if (m_pMutex) m_pMutex->unlock();
}

NvDecoder::~NvDecoder() {

    START_TIMER
    cuCtxPushCurrent(m_cuContext);
    cuCtxPopCurrent(NULL);

    if (m_hParser) {
        cuvidDestroyVideoParser(m_hParser);
    }

    if (m_hDecoder) {
        if (m_pMutex) m_pMutex->lock();
        cuvidDestroyDecoder(m_hDecoder);
        if (m_pMutex) m_pMutex->unlock();
    }

    std::lock_guard<std::mutex> lock(m_mtxVPFrame);
    if (m_vpFrame.size() != m_nFrameAlloc)
    {
        //LOG(WARNING) << "nFrameAlloc(" << m_nFrameAlloc << ") != m_vpFrame.size()(" << m_vpFrame.size() << ")";
    }
    for (uint8_t *pFrame : m_vpFrame)
    {
        if (m_bUseDeviceFrame)
        {
            if (m_pMutex) m_pMutex->lock();
            cuCtxPushCurrent(m_cuContext);
            cuMemFree((CUdeviceptr)pFrame);
            cuCtxPopCurrent(NULL);
            if (m_pMutex) m_pMutex->unlock();
        }
        else
        {
            delete[] pFrame;
        }
    }
    cuvidCtxLockDestroy(m_ctxLock);
    STOP_TIMER("Session Deinitialization Time: ");

    if (m_selfCtx) {
        cuCtxDestroy(m_selfCtx);
        m_selfCtx = NULL;
    }

}

int NvDecoder::decode(const uint8_t *bitstream, int bitstreamBytes,
                       uint8_t ***pppFrame, int *pnFrameReturned,
                       uint32_t flags, int64_t **ppTimestamp, int64_t timestamp, CUstream stream)
{
    if (!m_hParser)
    {
        NVDEC_THROW_ERROR("Parser not initialized.", CUDA_ERROR_NOT_INITIALIZED);
        return -1;
    }

    CUVIDSOURCEDATAPACKET packet = {0};
    packet.payload      = bitstream;
    packet.payload_size = bitstreamBytes;
    packet.flags        = flags | CUVID_PKT_TIMESTAMP;
    packet.timestamp    = timestamp;
    if (!bitstream || bitstreamBytes == 0) {
        packet.flags |= CUVID_PKT_ENDOFSTREAM;
    }

    m_nDecodedFrame = 0;
    m_cuvidStream = stream;
    if (m_pMutex) m_pMutex->lock();
    NVDEC_API_CALL(cuvidParseVideoData(m_hParser, &packet));
    if (m_pMutex) m_pMutex->unlock();
    m_cuvidStream = 0;

    if (m_nDecodedFrame > 0)
    {
        if (pppFrame)
        {
            m_vpFrameRet.clear();
            std::lock_guard<std::mutex> lock(m_mtxVPFrame);
            m_vpFrameRet.insert(m_vpFrameRet.begin(),
                                m_vpFrame.begin(),
                                m_vpFrame.begin() + m_nDecodedFrame);
            *pppFrame = &m_vpFrameRet[0];
        }
        if (ppTimestamp)
        {
            *ppTimestamp = &m_vTimestamp[0];
        }
    }
    if (pnFrameReturned)
    {
        *pnFrameReturned = m_nDecodedFrame;
    }
    return 0;
}

int NvDecoder::decode_lockFrame(const uint8_t *bitstream, int bitstreamBytes, uint8_t ***pppFrame, int *pnFrameReturned, uint32_t flags, int64_t **ppTimestamp, int64_t timestamp, CUstream stream)
{
    auto ret = decode(bitstream, bitstreamBytes, pppFrame, pnFrameReturned, flags, ppTimestamp, timestamp, stream);
    if (ret) {
        return ret;
    }

    std::lock_guard<std::mutex> lock(m_mtxVPFrame);
    m_vpFrame.erase(m_vpFrame.begin(), m_vpFrame.begin() + m_nDecodedFrame);
    return ret;
}

void NvDecoder::unlockFrame(uint8_t **ppFrame, int nFrame)
{
    std::lock_guard<std::mutex> lock(m_mtxVPFrame);
    m_vpFrame.insert(m_vpFrame.end(), &ppFrame[0], &ppFrame[nFrame]);
}

#if 0
extern "C"
void *
TestH264DecoderCreate(
    uint16_t instanceId,
    TestRect *lCropRect,
    TestDim  *lResizeDim)
{
    Rect _rect, *pCropRect = NULL;
    Dim  _dim,  *pResizeDim = NULL;

    if (lCropRect) {
        pCropRect = &_rect;
        pCropRect->l = lCropRect->left;
        pCropRect->t = lCropRect->top;
        pCropRect->r = lCropRect->right;
        pCropRect->b = lCropRect->bottom;
    }

    if (lResizeDim) {
        pResizeDim = &_dim;
        pResizeDim->w = lResizeDim->w;
        pResizeDim->h = lResizeDim->h;
    }

    auto decoder = new NvDecoder(instanceId, pCropRect, pResizeDim);
    return static_cast<void*>(decoder);
}

extern "C"
void
TestH264DecoderDestroy(void *decoder_handle)
{
    auto decoder = static_cast<NvDecoder*>(decoder_handle);
    delete(decoder);
}

extern "C"
TestMediaStatus
TestH264Decode(
    void *decoder_handle,
    const TestBitstreams *bitstreams,
    TestImage *output_image)
{
    auto decoder = static_cast<NvDecoder*>(decoder_handle);

    if (output_image->colorStd) {
        auto oformat = static_cast<NvDecoder::ImageFormat_t>(output_image->colorStd);
        if (decoder->oformat != oformat) {
            __D("convert to format: %d:%s \n", oformat,
                (oformat == NvDecoder::IMAGE_NV12)  ? "nv12" :
                (oformat == NvDecoder::IMAGE_YUV)  ? "yuv" :
                (oformat == NvDecoder::IMAGE_Y)    ? "y" :
                (oformat == NvDecoder::IMAGE_RGB)  ? "rgb" :
                (oformat == NvDecoder::IMAGE_BGR)  ? "bgr" :
                (oformat == NvDecoder::IMAGE_RGBI) ? "rgbi" :
                (oformat == NvDecoder::IMAGE_BGRI) ? "bgri" :
                "unknown"
                );
            decoder->oformat = oformat;
        }
    }

    uint8_t **ppFrame;
    int   nFrameReturned = 0;
    auto ret = decoder->decode(bitstreams->bitstream, bitstreams->bitstreamBytes,
                               &ppFrame, &nFrameReturned);
    if (nFrameReturned == 0) {
        return -1;
    }

    for (int i = 0; i < nFrameReturned; i++) {
        output_image->img          = ppFrame[i];
        output_image->bytePerPixel = decoder->getBPP();
        output_image->width        = decoder->getWidth();
        output_image->height       = decoder->getHeight();
        output_image->colorStd = static_cast<TestImageFormat_t>(decoder->oformat);
    }

    return 0;
}

extern "C"
void
printH264Info(void *decoder_handle)
{
    auto decoder = static_cast<NvDecoder*>(decoder_handle);

    std::vector <std::string> aszDecodeOutFormat = { "NV12", "P016", "YUV444", "YUV444P16" };
    /* if (bOutPlanar) { */
    /*     aszDecodeOutFormat[0] = "iyuv"; */
    /*     aszDecodeOutFormat[1] = "yuv420p16"; */
    /* } */

    __I("%s \nDecodeOutFormat: %s \n",
        decoder->getVideoInfo().c_str(),
        aszDecodeOutFormat[decoder->getOutputFormat()].c_str()
        );
}




extern "C"
void
printDecoderCapability()
{
    ck(cuInit(0));
    int nGpu = 0;
    ck(cuDeviceGetCount(&nGpu));
    std::cout << "Decoder Capability" << std::endl << std::endl;
    const char *aszCodecName[] = {"JPEG", "MPEG1", "MPEG2", "MPEG4", "H264", "HEVC", "HEVC", "HEVC", "HEVC", "HEVC", "HEVC", "VC1", "VP8", "VP9", "VP9", "VP9"};
    const char *aszChromaFormat[] = { "4:0:0", "4:2:0", "4:2:2", "4:4:4" };
    //char strOutputFormats[64];
    cudaVideoCodec aeCodec[] = { cudaVideoCodec_JPEG, cudaVideoCodec_MPEG1, cudaVideoCodec_MPEG2, cudaVideoCodec_MPEG4, cudaVideoCodec_H264, cudaVideoCodec_HEVC,
        cudaVideoCodec_HEVC, cudaVideoCodec_HEVC, cudaVideoCodec_HEVC, cudaVideoCodec_HEVC, cudaVideoCodec_HEVC, cudaVideoCodec_VC1, cudaVideoCodec_VP8,
        cudaVideoCodec_VP9, cudaVideoCodec_VP9, cudaVideoCodec_VP9 };
    int anBitDepthMinus8[] = {0, 0, 0, 0, 0, 0, 2, 4, 0, 2, 4, 0, 0, 0, 2, 4};

    cudaVideoChromaFormat aeChromaFormat[] = { cudaVideoChromaFormat_420, cudaVideoChromaFormat_420, cudaVideoChromaFormat_420, cudaVideoChromaFormat_420,
        cudaVideoChromaFormat_420, cudaVideoChromaFormat_420, cudaVideoChromaFormat_420, cudaVideoChromaFormat_420, cudaVideoChromaFormat_444, cudaVideoChromaFormat_444,
        cudaVideoChromaFormat_444, cudaVideoChromaFormat_420, cudaVideoChromaFormat_420, cudaVideoChromaFormat_420, cudaVideoChromaFormat_420, cudaVideoChromaFormat_420 };

    for (int iGpu = 0; iGpu < nGpu; iGpu++) {

        CUcontext cuContext = NULL;
        //createCudaContext(&cuContext, iGpu, 0);
        CUdevice cuDevice = 0;
        ck(cuDeviceGet(&cuDevice, iGpu));
        char szDeviceName[80];
        ck(cuDeviceGetName(szDeviceName, sizeof(szDeviceName), cuDevice));
        std::cout << "GPU in use: " << szDeviceName << std::endl;
        ck(cuCtxCreate(&cuContext, 9, cuDevice));


        for (int i = 0; i < sizeof(aeCodec) / sizeof(aeCodec[0]); i++) {

            CUVIDDECODECAPS decodeCaps = {};
            decodeCaps.eCodecType = aeCodec[i];
            decodeCaps.eChromaFormat = aeChromaFormat[i];
            decodeCaps.nBitDepthMinus8 = anBitDepthMinus8[i];

            cuvidGetDecoderCaps(&decodeCaps);

            std::string outputFormat = "";
            if (decodeCaps.nOutputFormatMask & (1U << cudaVideoSurfaceFormat_NV12)) {
                outputFormat = "NV12";
            }

            if (decodeCaps.nOutputFormatMask & (1U << cudaVideoSurfaceFormat_P016)) {
                outputFormat = "P016";
            }

            if (decodeCaps.nOutputFormatMask & (1U << cudaVideoSurfaceFormat_YUV444)) {
                outputFormat = "YUV444";
            }

            if (decodeCaps.nOutputFormatMask & (1U << cudaVideoSurfaceFormat_YUV444_16Bit)) {
                outputFormat = "YUV444P16";
            }


            if (decodeCaps.bIsSupported) {
                __I("Codec: %-5s  BitDepth: %-3u MaxWidth: %-6u MaxHeight: %-6u MaxMBCount: %-9u Format: %-s %s\n",
                    aszCodecName[i],
                    decodeCaps.nBitDepthMinus8 + 8,
                    decodeCaps.nMaxWidth, decodeCaps.nMaxHeight,
                    decodeCaps.nMaxMBCount,
                    aszChromaFormat[decodeCaps.eChromaFormat], &outputFormat[0]);
            }
        }

        ck(cuCtxDestroy(cuContext));
    }
}
#endif

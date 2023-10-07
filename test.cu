#include <iostream>
#include <algorithm>
#include <thread>
#include <cuda.h>
#include <sstream>
#include <iostream>
#include <fstream>
#include <iomanip>

#include "test_demuxer.h"
#include "test_viddec.h"
//#include "test_nvjpg.h"

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




int main(int argc, char **argv)
{
    int nFrame = 0;
    bool bDecodeOutSemiPlanar = false;
    void *encoder = NULL;
    bool bOutPlanar = false;
    TestRect lCropRect = {
        .left   = 30,
        .top    = 10,
        .right  = 640,
        .bottom = 480
    };
    TestDim  lResizeDim = {
        .w = 640,
        .h = 480
    };

    //printDecoderCapability();

    auto szInFilePath = argc > 1 ? std::string(argv[1]) : "input.mp4";
    auto demuxer = TestDemuxerCreate(&szInFilePath[0]);
    auto decoder = TestH264DecoderCreate(0, &lCropRect, &lResizeDim);
    TestBitstreams bitstreams;

    do {
        TestImage om;
        //om.colorStd = TEST_IMAGE_NV12;
        //om.colorStd = TEST_IMAGE_RGBI;
        om.colorStd = TEST_IMAGE_RGB;

        TestDemux(demuxer,
                   &bitstreams.bitstream,
                   &bitstreams.bitstreamBytes);

        auto ret = TestH264Decode(decoder, &bitstreams, &om);
        if (ret != TEST_MEDIA_STATUS_OK) {
            // bitstream insufficient
            continue;
        }

        if (!nFrame) {
            printH264Info(decoder);
        }

        //bDecodeOutSemiPlanar = (om.colorStd == TEST_IMAGE_YUV);
        if (om.img) {
            if (om.colorStd == TEST_IMAGE_NV12) {
                // dump NV12
                char nv12_path[100];
                sprintf(nv12_path, "%d_NV12_%dx%d.yuv",
                        nFrame,
                        om.width, om.height);
                auto nv12File = fopen(nv12_path, "w+");

                if (fwrite(om.img, om.width*om.height*1.5, 1, nv12File) != 1) {
                    return -1;
                }
                fclose(nv12File);

            } else if (om.colorStd == TEST_IMAGE_RGBI) {
                // dump RGBI
                char rgbi_path[100];
                sprintf(rgbi_path, "%d_RGBi_%dx%d.rgb",
                        nFrame,
                        om.width, om.height);
                auto rgbiFile = fopen(rgbi_path, "w+");

                if (fwrite(om.img, om.width*om.height*3, 1, rgbiFile) != 1) {
                    return -1;
                }
                fclose(rgbiFile);
            } else if (om.colorStd == TEST_IMAGE_RGB) {
                // dump RGBp
                char rgbp_path[100];
                sprintf(rgbp_path, "%d_RGBp_%dx%d.rgb",
                        nFrame,
                        om.width, om.height);
                auto rgbpFile = fopen(rgbp_path, "w+");

                if (fwrite(om.img, om.width*om.height*3, 1, rgbpFile) != 1) {
                    return -1;
                }
                fclose(rgbpFile);
            }

            // convert2jpg
            if (!encoder) {
                encoder = TestJpgEncoderCreate(om.colorStd,
                                                om.width, om.height,
                                                0,0);
            }

            TestBitstreams output_bs = {0};
            TestJpgEncode(encoder, &om, &output_bs);

            auto jpg_path = std::to_string(nFrame) + "out.jpg";
            auto outputFile = fopen(&jpg_path[0], "w+");

            if (fwrite(output_bs.bitstream, output_bs.bitstreamBytes, 1, outputFile) != 1) {
                return 0;
            }
            fclose(outputFile);
        }

        if (bOutPlanar && bDecodeOutSemiPlanar) {
            //ConvertSemiplanarToPlanar(ppFrame[i], dec.GetWidth(), dec.GetHeight(), dec.GetBitDepth());
            __D("\n");
        }


        nFrame += 1;

    } while (bitstreams.bitstreamBytes);

    TestDemuxerDestroy(demuxer);

    std::cout << "Total frame decoded: " << nFrame << std::endl;

    return 0;
}

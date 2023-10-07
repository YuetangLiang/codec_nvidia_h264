# codec_nvidia_h264
## sample
```cpp
    int nFrame = 0;
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

    auto szInFilePath = "input.mp4";
    auto demuxer = TestDemuxerCreate(&szInFilePath[0]);
    auto decoder = TestH264DecoderCreate(0, &lCropRect, &lResizeDim);
    TestBitstreams bitstreams;

    do {
        TestDemux(demuxer,
                   &bitstreams.bitstream,
                   &bitstreams.bitstreamBytes);

        auto ret = TestH264Decode(decoder, &bitstreams, &om);
        if (ret != TEST_MEDIA_STATUS_OK) {
            continue;
        }

        if (!nFrame) {
            printH264Info(decoder);
        }

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
        nFrame += 1;

    } while (bitstreams.bitstreamBytes);
    TestDemuxerDestroy(demuxer);

```

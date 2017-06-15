#include <Windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <Mfreadwrite.h>
#include <mferror.h>

#include <utility>
#include <random>

#include"IMFObjectWrapper.h"

#pragma comment(lib, "mfreadwrite")
#pragma comment(lib, "mfplat")
#pragma comment(lib, "mfuuid")

// Format constants
const UINT32 VIDEO_WIDTH = 640;
const UINT32 VIDEO_HEIGHT = 480;
const UINT32 VIDEO_FPS = 30;
const UINT64 VIDEO_FRAME_DURATION = 10 * 1000 * 1000 / VIDEO_FPS;
const UINT32 VIDEO_BIT_RATE = 800000;
const GUID   VIDEO_ENCODING_FORMAT = MFVideoFormat_WMV3;
const GUID   VIDEO_INPUT_FORMAT = MFVideoFormat_RGB32;
const UINT32 VIDEO_PELS = VIDEO_WIDTH * VIDEO_HEIGHT;
const UINT32 VIDEO_FRAME_COUNT = 20 * VIDEO_FPS;

// Buffer to hold the video frame data.
DWORD videoFrameBuffer[VIDEO_PELS];

struct InitializeSinkWriterResult
{
    IMFWrappers::IMFSinkWriterWrapper sinkWritter;
    DWORD streamIndex;

    explicit InitializeSinkWriterResult(IMFWrappers::IMFSinkWriterWrapper&& sinkWritter, DWORD streamIndex) : sinkWritter(std::move(sinkWritter)), streamIndex(streamIndex) {};
};

InitializeSinkWriterResult InitializeSinkWriter()
{
    IMFWrappers::IMFSinkWriterWrapper pSinkWriter("output.wmv", NULL, NULL);

    // Set the output media type.
    IMFWrappers::IMFMediaTypeWrapper pMediaTypeOut;

    pMediaTypeOut.setGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    pMediaTypeOut.setGUID(MF_MT_SUBTYPE, VIDEO_ENCODING_FORMAT);
    pMediaTypeOut.setUINT32(MF_MT_AVG_BITRATE, VIDEO_BIT_RATE);
    pMediaTypeOut.setUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
    pMediaTypeOut.setAttributeSize(MF_MT_FRAME_SIZE, VIDEO_WIDTH, VIDEO_HEIGHT);
    pMediaTypeOut.setAttributeRatio(MF_MT_FRAME_RATE, VIDEO_FPS, 1);
    pMediaTypeOut.setAttributeRatio(MF_MT_PIXEL_ASPECT_RATIO, 1, 1);

    // Set the input media type.
    IMFWrappers::IMFMediaTypeWrapper pMediaTypeIn;

    pMediaTypeIn.setGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    pMediaTypeIn.setGUID(MF_MT_SUBTYPE, VIDEO_INPUT_FORMAT);
    pMediaTypeIn.setUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
    pMediaTypeIn.setAttributeSize(MF_MT_FRAME_SIZE, VIDEO_WIDTH, VIDEO_HEIGHT);
    pMediaTypeIn.setAttributeRatio(MF_MT_FRAME_RATE, VIDEO_FPS, 1);
    pMediaTypeIn.setAttributeRatio(MF_MT_PIXEL_ASPECT_RATIO, 1, 1);

    DWORD streamIndex = pSinkWriter.AddStream(pMediaTypeOut);
    pSinkWriter.setInputMediaType(streamIndex, pMediaTypeIn, NULL);
    pSinkWriter.beginWritting();

    pSinkWriter.addRef();

    return InitializeSinkWriterResult(std::move(pSinkWriter), streamIndex);
}

void WriteFrame(const IMFWrappers::IMFSinkWriterWrapper& pWriter, DWORD streamIndex, const LONGLONG rtStart)
{
    const LONG cbWidth = 4 * VIDEO_WIDTH;
    const DWORD cbBuffer = cbWidth * VIDEO_HEIGHT;
    BYTE *pData = NULL;

    IMFWrappers::IMFMediaBufferWrapper pBuffer(cbBuffer);
    pBuffer.lock(&pData);
    pBuffer.copyImage(pData, cbWidth, (BYTE*)videoFrameBuffer, cbWidth, cbWidth, VIDEO_HEIGHT);
    pBuffer.unlock();
    pBuffer.setCurrentLength(cbBuffer);

    IMFWrappers::IMFSampleWrapper pSample;
    pSample.addBuffer(pBuffer);
    pSample.setSampleTime(rtStart);
    pSample.setSampleDuration(VIDEO_FRAME_DURATION);

    pWriter.writeSample(streamIndex, pSample);

    pSample.release();
    pBuffer.release();
}

void main()
{
    std::default_random_engine generator;
    std::uniform_int_distribution<UINT32> distribution(0, VIDEO_WIDTH);

    // Set all pixels to green
    for (DWORD i = 0; i < VIDEO_PELS; ++i)
    {
        videoFrameBuffer[i] = 0x0000FF00;
    }

    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (SUCCEEDED(hr))
    {
        hr = MFStartup(MF_VERSION);
        if (SUCCEEDED(hr))
        {
            try
            {
                auto sinkWriterAndStream = InitializeSinkWriter();

                // Send frames to the sink writer.
                LONGLONG rtStart = 0;

                for (DWORD i = 0; i < VIDEO_FRAME_COUNT; ++i)
                {
                    // Add some random pixels
                    for (size_t j = 0; j < 200; ++j)
                    {
                        videoFrameBuffer[distribution(generator) + (i % VIDEO_HEIGHT) * VIDEO_WIDTH] = (rand() % 0xFFFFFF);
                    }

                    WriteFrame(sinkWriterAndStream.sinkWritter, sinkWriterAndStream.streamIndex, rtStart);
                    rtStart += VIDEO_FRAME_DURATION;
                }

            }
            catch (const WindowsError& err)
            {
                std::cout << "Catched exception - " << err.toString() << std::endl;
            }

            MFShutdown();
        }
        CoUninitialize();
    }
}

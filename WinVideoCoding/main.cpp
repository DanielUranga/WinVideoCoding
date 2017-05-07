#include <new>
#include <iostream>
#include <windows.h>
#include <mfapi.h>
#include <Mfidl.h>
#include <shlwapi.h>
#include <codecapi.h>
#include <iostream>

#include "CSession.h"
#include "SafeRelease.h"
#include "WindowsError.h"
#include "IMFObjectWrapper.h"

#pragma comment(lib, "mfplat")
#pragma comment(lib, "mf")
#pragma comment(lib, "mfuuid")
#pragma comment(lib, "shlwapi")

#pragma warning(disable : 4996)

struct H264ProfileInfo
{
    UINT32  profile;
    MFRatio fps;
    MFRatio frame_size;
    UINT32  bitrate;
};

H264ProfileInfo h264_profiles[] =
{
    { eAVEncH264VProfile_Base,{ 15, 1 },{ 176, 144 },   128000 },
    { eAVEncH264VProfile_Base,{ 15, 1 },{ 352, 288 },   384000 },
    { eAVEncH264VProfile_Base,{ 30, 1 },{ 352, 288 },   384000 },
    { eAVEncH264VProfile_Base,{ 29970, 1000 },{ 320, 240 },   528560 },
    { eAVEncH264VProfile_Base,{ 15, 1 },{ 720, 576 },  4000000 },
    { eAVEncH264VProfile_Main,{ 25, 1 },{ 720, 576 }, 10000000 },
    { eAVEncH264VProfile_Main,{ 30, 1 },{ 352, 288 }, 10000000 },
};

struct AACProfileInfo
{
    UINT32  samplesPerSec;
    UINT32  numChannels;
    UINT32  bitsPerSample;
    UINT32  bytesPerSec;
    UINT32  aacProfile;
};

AACProfileInfo aac_profiles[] =
{
    { 96000, 2, 16, 24000, 0x29 },
    { 48000, 2, 16, 24000, 0x29 },
    { 44100, 2, 16, 16000, 0x29 },
    { 44100, 2, 16, 12000, 0x29 },
};


int video_profile = 0;
int audio_profile = 0;

IMFWrappers::IMFMediaSourceWrapper CreateMediaSource(PCWSTR pszURL)
{
    // Create the source resolver.
    IMFWrappers::IMFSourceResolverWrapper pResolver;
    IMFWrappers::IMFMediaSourceWrapper pSource;

    // Use the source resolver to create the media source
    MF_OBJECT_TYPE objecType = MF_OBJECT_INVALID;
    pResolver.CreateObjectFromURL(pszURL, MF_RESOLUTION_MEDIASOURCE, NULL, &objecType, pSource);

    pSource.QueryInterface();

    return std::move(pSource);
}

IMFWrappers::IMFAttributesWrapper CreateAACProfile(DWORD index)
{
    if (index >= ARRAYSIZE(h264_profiles))
    {
        THROW_WINDOWS_ERROR(E_INVALIDARG);
    }

    const AACProfileInfo& profile = aac_profiles[index];

    IMFWrappers::IMFAttributesWrapper pAttributes(7);

    pAttributes.setGUID(MF_MT_SUBTYPE, MFAudioFormat_AAC);
    pAttributes.setUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, profile.bitsPerSample);
    pAttributes.setUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, profile.samplesPerSec);
    pAttributes.setUINT32(MF_MT_AUDIO_NUM_CHANNELS, profile.numChannels);
    pAttributes.setUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, profile.bytesPerSec);
    pAttributes.setUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 1);
    pAttributes.setUINT32(MF_MT_AAC_AUDIO_PROFILE_LEVEL_INDICATION, profile.aacProfile);

    pAttributes.addRef(); // Is this needed?

    return std::move(pAttributes);
}

IMFWrappers::IMFAttributesWrapper CreateH264Profile(DWORD index)
{
    if (index >= ARRAYSIZE(h264_profiles))
    {
        THROW_WINDOWS_ERROR(E_INVALIDARG);
    }

    const H264ProfileInfo& profile = h264_profiles[index];

    IMFWrappers::IMFAttributesWrapper pAttributes(5);

    pAttributes.setGUID(MF_MT_SUBTYPE, MFVideoFormat_H264);
    pAttributes.setUINT32(MF_MT_MPEG2_PROFILE, profile.profile);
    pAttributes.setAttributeSize(MF_MT_FRAME_SIZE, profile.frame_size.Numerator, profile.frame_size.Numerator);
    pAttributes.setAttributeRatio(MF_MT_FRAME_RATE, profile.fps.Numerator, profile.fps.Denominator);
    pAttributes.setUINT32(MF_MT_AVG_BITRATE, profile.bitrate);

    pAttributes.addRef(); // Is this needed?

    return std::move(pAttributes);
}

IMFWrappers::IMFTranscodeProfileWrapper CreateTranscodeProfile()
{
    IMFWrappers::IMFTranscodeProfileWrapper pProfile;

    // Audio attributes.
    IMFWrappers::IMFAttributesWrapper pAudio = CreateAACProfile(audio_profile);
    pProfile.SetAudioAttributes(pAudio);

    // Video attributes.
    IMFWrappers::IMFAttributesWrapper pVideo = CreateH264Profile(video_profile);
    pProfile.SetVideoAttributes(pVideo);

    // Container attributes.
    IMFWrappers::IMFAttributesWrapper pContainer(1);
    pContainer.setGUID(MF_TRANSCODE_CONTAINERTYPE, MFTranscodeContainerType_MPEG4);
    
    pProfile.SetContainerAttributes(pContainer);
    pProfile.addRef();

    return std::move(pProfile);
}

void RunEncodingSession(CSession *pSession, MFTIME duration)
{
    const DWORD WAIT_PERIOD = 500;
    const int   UPDATE_INCR = 5;

    HRESULT hr = S_OK;
    MFTIME pos;
    LONGLONG prev = 0;
    while (1)
    {
        hr = pSession->Wait(WAIT_PERIOD);
        if (hr == E_PENDING)
        {
            hr = pSession->GetEncodingPosition(&pos);

            LONGLONG percent = (100 * pos) / duration;
            if (percent >= prev + UPDATE_INCR)
            {
                std::cout << percent << "% .. ";
                prev = percent;
            }
        }
        else
        {
            std::cout << std::endl;
            break;
        }
    }
    if (FAILED(hr))
    {
        THROW_WINDOWS_ERROR(hr);
    }
}

void EncodeFile(PCWSTR pszInput, PCWSTR pszOutput)
{
    IMFWrappers::IMFMediaSourceWrapper pSource(CreateMediaSource(pszInput));

    MFTIME duration = pSource.getDuration();
    std::cout << "Duration: " << duration << std::endl;

    IMFWrappers::IMFTranscodeProfileWrapper pProfile = CreateTranscodeProfile();

    IMFWrappers::IMFTopologyWrapper pTopology(pSource, pszOutput, pProfile);

    CSession *pSession;
    DO_CHECKED_OPERATION(CSession::Create(&pSession));
    DO_CHECKED_OPERATION(pSession->StartEncodingSession(pTopology.get()));

    RunEncodingSession(pSession, duration);

    pSession->Release();

}

int main(int argc, char* argv[]) 
{

    HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);

    if (argc < 3 || argc > 5)
    {
        std::cout << "Usage:" << std::endl;
        std::cout << "input output [ audio_profile video_profile ]" << std::endl;
        return 1;
    }

    if (argc > 3)
    {
        audio_profile = atoi(argv[3]);
    }
    if (argc > 4)
    {
        video_profile = atoi(argv[4]);
    }

    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (SUCCEEDED(hr))
    {
        hr = MFStartup(MF_VERSION);
        if (SUCCEEDED(hr))
        {
            // arg1
            size_t arg1Length = mbsrtowcs(NULL, (const char**)&argv[1], 0, NULL);
            wchar_t* arg1 = new wchar_t[arg1Length + 1]();
            arg1Length = mbsrtowcs(arg1, (const char**)&argv[1], arg1Length + 1, NULL);

            // arg2
            size_t arg2Length = mbsrtowcs(NULL, (const char**)&argv[2], 0, NULL);
            wchar_t* arg2 = new wchar_t[arg2Length + 1]();
            arg1Length = mbsrtowcs(arg2, (const char**)&argv[2], arg2Length + 1, NULL);

            try
            {
                EncodeFile(arg1, arg2);
            }
            catch (const WindowsError& err)
            {
                std::cout << "Catched exception - " << err.toString() << std::endl;
            }
            catch (const char* err)
            {
                std::cout << "Catched exception - " << err << std::endl;
            }

            delete[] arg1;
            delete[] arg2;

            MFShutdown();
        }
        else
        {
            std::cout << "Can't MFStartup: " << std::hex << hr << std::endl;
        }
        CoUninitialize();
    }
    else
    {
        std::cout << "Can't CoInitializeEx: " << std::hex << hr << std::endl;
    }

    if (SUCCEEDED(hr))
    {
        std::cout << "Done." << std::endl;
    }
    else
    {
        std::cout << "Error: " << std::hex << hr << std::endl;
    }

    return 0;
}

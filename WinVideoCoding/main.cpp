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

void CreateMediaSource(PCWSTR pszURL, IMFMediaSource **ppSource)
{
	MF_OBJECT_TYPE ObjectType = MF_OBJECT_INVALID;
	SafeReleasePointerWrapper<IMFSourceResolver> pResolver(nullptr);
	SafeReleasePointerWrapper<IUnknown> pSource(nullptr);

	// Create the source resolver.
	HRESULT hr = MFCreateSourceResolver(pResolver.getPointer());
	if (FAILED(hr))
	{
		throw WindowsError(hr);
	}

	// Use the source resolver to create the media source
	hr = pResolver.get()->CreateObjectFromURL(pszURL, MF_RESOLUTION_MEDIASOURCE, NULL, &ObjectType, pSource.getPointer());
	if (FAILED(hr))
	{
		throw WindowsError(hr);
	}

	// Get the IMFMediaSource interface from the media source.
	hr = pSource.get()->QueryInterface(IID_PPV_ARGS(ppSource));
}

void GetSourceDuration(IMFMediaSource *pSource, MFTIME *pDuration)
{
	*pDuration = 0;

	IMFPresentationDescriptor *pPD = NULL;

	HRESULT hr = pSource->CreatePresentationDescriptor(&pPD);
	if (SUCCEEDED(hr))
	{
		hr = pPD->GetUINT64(MF_PD_DURATION, (UINT64*)pDuration);
		pPD->Release();
	}
	else
	{
		throw WindowsError(hr);
	}
}

void CreateAACProfile(DWORD index, IMFAttributes **ppAttributes)
{
	if (index >= ARRAYSIZE(h264_profiles))
	{
		throw WindowsError(E_INVALIDARG);
	}

	const AACProfileInfo& profile = aac_profiles[index];

	SafeReleasePointerWrapper<IMFAttributes> pAttributes(nullptr);

	HRESULT hr = MFCreateAttributes(pAttributes.getPointer(), 7);
	if (SUCCEEDED(hr))
	{
		hr = pAttributes.get()->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_AAC);
	}
	if (SUCCEEDED(hr))
	{
		hr = pAttributes.get()->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, profile.bitsPerSample);
	}
	if (SUCCEEDED(hr))
	{
		hr = pAttributes.get()->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, profile.samplesPerSec);
	}
	if (SUCCEEDED(hr))
	{
		hr = pAttributes.get()->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, profile.numChannels);
	}
	if (SUCCEEDED(hr))
	{
		hr = pAttributes.get()->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, profile.bytesPerSec);
	}
	if (SUCCEEDED(hr))
	{
		hr = pAttributes.get()->SetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 1);
	}
	if (SUCCEEDED(hr))
	{
		hr = pAttributes.get()->SetUINT32(MF_MT_AAC_AUDIO_PROFILE_LEVEL_INDICATION, profile.aacProfile);
	}
	if (SUCCEEDED(hr))
	{
		*ppAttributes = pAttributes.get();
		(*ppAttributes)->AddRef();
	}
	if (FAILED(hr))
	{
		throw WindowsError(hr);
	}
}

void CreateH264Profile(DWORD index, IMFAttributes **ppAttributes)
{
	if (index >= ARRAYSIZE(h264_profiles))
	{
		throw WindowsError(E_INVALIDARG);
	}

	SafeReleasePointerWrapper<IMFAttributes> pAttributes(nullptr);

	const H264ProfileInfo& profile = h264_profiles[index];

	HRESULT hr = MFCreateAttributes(pAttributes.getPointer(), 5);
	if (SUCCEEDED(hr))
	{
		hr = pAttributes.get()->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_H264);
	}
	if (SUCCEEDED(hr))
	{
		hr = pAttributes.get()->SetUINT32(MF_MT_MPEG2_PROFILE, profile.profile);
	}
	if (SUCCEEDED(hr))
	{
		hr = MFSetAttributeSize(pAttributes.get(), MF_MT_FRAME_SIZE, profile.frame_size.Numerator, profile.frame_size.Numerator);
	}
	if (SUCCEEDED(hr))
	{
		hr = MFSetAttributeRatio(pAttributes.get(), MF_MT_FRAME_RATE, profile.fps.Numerator, profile.fps.Denominator);
	}
	if (SUCCEEDED(hr))
	{
		hr = pAttributes.get()->SetUINT32(MF_MT_AVG_BITRATE, profile.bitrate);
	}
	if (SUCCEEDED(hr))
	{
		*ppAttributes = pAttributes.get();
		(*ppAttributes)->AddRef();
	}
	if (FAILED(hr))
	{
		throw WindowsError(hr);
	}
}

void CreateTranscodeProfile(IMFTranscodeProfile **ppProfile)
{
	SafeReleasePointerWrapper<IMFTranscodeProfile> pProfile(nullptr);
	SafeReleasePointerWrapper<IMFAttributes> pAudio(nullptr);
	SafeReleasePointerWrapper<IMFAttributes> pVideo(nullptr);
	SafeReleasePointerWrapper<IMFAttributes> pContainer(nullptr);

	HRESULT hr = MFCreateTranscodeProfile(pProfile.getPointer());
	if (FAILED(hr))
	{
		throw WindowsError(hr);
	}

	// Audio attributes.
	CreateAACProfile(audio_profile, pAudio.getPointer());
	pProfile.get()->SetAudioAttributes(pAudio.get());
	if (FAILED(hr))
	{
		throw WindowsError(hr);
	}

	// Video attributes.
	CreateH264Profile(video_profile, pVideo.getPointer());
	pProfile.get()->SetVideoAttributes(pVideo.get());
	if (FAILED(hr))
	{
		throw WindowsError(hr);
	}

	// Container attributes.
	hr = MFCreateAttributes(pContainer.getPointer(), 1);
	if (FAILED(hr))
	{
		throw WindowsError(hr);
	}

	hr = pContainer.get()->SetGUID(MF_TRANSCODE_CONTAINERTYPE, MFTranscodeContainerType_MPEG4);
	if (FAILED(hr))
	{
		throw WindowsError(hr);
	}

	hr = pProfile.get()->SetContainerAttributes(pContainer.get());
	if (FAILED(hr))
	{
		throw WindowsError(hr);
	}

	*ppProfile = pProfile.get();
	(*ppProfile)->AddRef();
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
		throw WindowsError(hr);
	}
}

void EncodeFile(PCWSTR pszInput, PCWSTR pszOutput)
{

	SafeReleasePointerWrapper<IMFTranscodeProfile> pProfile(nullptr);
	SafeReleasePointerWrapper<IMFMediaSource> pSource(nullptr);
	SafeReleasePointerWrapper<IMFTopology> pTopology(nullptr);
	SafeReleasePointerWrapper<CSession> pSession(nullptr);

	CreateMediaSource(pszInput, pSource.getPointer());
	
	MFTIME duration = 0;
	GetSourceDuration(pSource.get(), &duration);
	std::cout << "Duration: " << duration << std::endl;

	CreateTranscodeProfile(pProfile.getPointer());
	
	IMFAttributes* imfAttrs = nullptr;
	UINT32 count = 0;
	HRESULT hr = pProfile.get()->GetContainerAttributes(&imfAttrs);
	if (FAILED(hr))
	{
		if (pSource.get())
		{
			pSource.get()->Shutdown();
		}
		throw WindowsError(hr);
	}

	hr = MFCreateTranscodeTopology(pSource.get(), pszOutput, pProfile.get(), pTopology.getPointer());
	if (FAILED(hr))
	{
		if (pSource.get())
		{
			pSource.get()->Shutdown();
		}
		throw WindowsError(hr);
	}

	hr = CSession::Create(pSession.getPointer());
	if (FAILED(hr))
	{
		if (pSource.get())
		{
			pSource.get()->Shutdown();
		}
		throw WindowsError(hr);
	}

	hr = pSession.get()->StartEncodingSession(pTopology.get());
	if (FAILED(hr))
	{
		if (pSource.get())
		{
			pSource.get()->Shutdown();
		}
		throw WindowsError(hr);
	}

	RunEncodingSession(pSession.get(), duration);

	if (pSource.get())
	{
		pSource.get()->Shutdown();
	}

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

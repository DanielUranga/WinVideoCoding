#pragma once

#include <iostream>
#include <utility>

#include "WindowsError.h"
#include "mfapi.h"
#include <mfidl.h>

template <class T> void SafeRelease(T **ppT)
{
	if (*ppT)
	{
		(*ppT)->Release();
		*ppT = NULL;
	}
}

template<typename T> class SafeReleasePointerWrapper
{
public:
	explicit SafeReleasePointerWrapper(T* ptr) : ptr(ptr) {};
	~SafeReleasePointerWrapper() { SafeRelease(&ptr); }
	T** getPointer() { return &ptr; }
	T* get() { return ptr; }

	void addRef() { throw "Should never be called - addRef"; }
	MFTIME getDuration() { throw "Should never be called"; return 0; }

	void setAttributeSize(REFGUID guidKey, UINT32 unWidth, UINT32 unHeight) { throw "Should never be called"; };
	void setAttributeRatio(REFGUID guidKey, UINT32 unWidth, UINT32 unHeight) { throw "Should never be called"; };
	void setGUID(REFGUID guidKey, REFGUID guidValue) { throw "Should never be called"; };
	void setUINT32(REFGUID guidKey, UINT32 unValue) { throw "Should never be called"; };

	void SetAudioAttributes(SafeReleasePointerWrapper<IMFAttributes>& attr) { throw "Should never be called"; };
	void SetVideoAttributes(SafeReleasePointerWrapper<IMFAttributes>& attr) { throw "Should never be called"; };

private:
	T *ptr;
};

// addRefs

template<>
void SafeReleasePointerWrapper<IMFAttributes>::addRef()
{
	HRESULT hr = ptr->AddRef();
	if (FAILED(hr))
	{
		throw WindowsError(hr);
	}
}

template<>
void SafeReleasePointerWrapper<IMFTranscodeProfile>::addRef()
{
	HRESULT hr = ptr->AddRef();
	if (FAILED(hr))
	{
		throw WindowsError(hr);
	}
}

// IMFAttributes specializations

template<>
void SafeReleasePointerWrapper<IMFAttributes>::setGUID(REFGUID guidKey, REFGUID guidValue)
{
	HRESULT hr = ptr->SetGUID(guidKey, guidValue);
	if (FAILED(hr))
	{
		throw WindowsError(hr);
	}
}

template<>
void SafeReleasePointerWrapper<IMFAttributes>::setAttributeSize(REFGUID guidKey, UINT32 unWidth, UINT32 unHeight)
{
	HRESULT hr = MFSetAttributeSize(ptr, guidKey, unWidth, unHeight);
	if (FAILED(hr))
	{
		throw WindowsError(hr);
	}
}

template<>
void SafeReleasePointerWrapper<IMFAttributes>::setAttributeRatio(REFGUID guidKey, UINT32 unWidth, UINT32 unHeight)
{
	HRESULT hr = MFSetAttributeRatio(ptr, guidKey, unWidth, unHeight);
	if (FAILED(hr))
	{
		throw WindowsError(hr);
	}
}

template<>
void SafeReleasePointerWrapper<IMFAttributes>::setUINT32(REFGUID guidKey, UINT32 unValue)
{
	HRESULT hr = ptr->SetUINT32(guidKey, unValue);
	if (FAILED(hr))
	{
		throw WindowsError(hr);
	}
}

// IMFMediaSource specializations

template<>
SafeReleasePointerWrapper<IMFMediaSource>::~SafeReleasePointerWrapper()
{
	ptr->Shutdown();
	SafeRelease(&ptr);
}

template<>
MFTIME SafeReleasePointerWrapper<IMFMediaSource>::getDuration()
{
	MFTIME pDuration = 0;
	IMFPresentationDescriptor *pPD = NULL;
	HRESULT hr = ptr->CreatePresentationDescriptor(&pPD);
	if (SUCCEEDED(hr))
	{
		UINT64 tmp;
		hr = pPD->GetUINT64(MF_PD_DURATION, &tmp);
		pDuration = tmp;
		pPD->Release();
	}
	else
	{
		throw WindowsError(hr);
	}
	return pDuration;
}

// IMFTranscodeProfile specializations

void SafeReleasePointerWrapper<IMFTranscodeProfile>::SetAudioAttributes(SafeReleasePointerWrapper<IMFAttributes>& attr)
{
	HRESULT hr = ptr->SetAudioAttributes(attr.get());
	if (FAILED(hr))
	{
		throw WindowsError(hr);
	}
}

void SafeReleasePointerWrapper<IMFTranscodeProfile>::SetVideoAttributes(SafeReleasePointerWrapper<IMFAttributes>& attr)
{
	HRESULT hr = ptr->SetVideoAttributes(attr.get());
	if (FAILED(hr))
	{
		throw WindowsError(hr);
	}
}


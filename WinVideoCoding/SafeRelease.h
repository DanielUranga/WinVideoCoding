#pragma once

#include <iostream>

#include "WindowsError.h"
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

	MFTIME getDuration() { throw "Should never be called"; return 0; }
	void setGUID(REFGUID guidKey, REFGUID guidValue) { throw "Should never be called"; };
	void setUINT32(REFGUID guidKey, UINT32 unValue) { throw "Should never be called"; };

private:
	T *ptr;
};

// IMFAttributes specializations:

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
void SafeReleasePointerWrapper<IMFAttributes>::setUINT32(REFGUID guidKey, UINT32 unValue)
{
	HRESULT hr = ptr->SetUINT32(guidKey, unValue);
	if (FAILED(hr))
	{
		throw WindowsError(hr);
	}
}

// IMFMediaSource specializations:

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


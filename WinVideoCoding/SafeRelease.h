#pragma once

#include <iostream>

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
private:
	T *ptr;
};

// IMFMediaSource specialization calls IMFMediaSource::Shutdown on destruction
template<> class SafeReleasePointerWrapper<IMFMediaSource>
{
public:
	explicit SafeReleasePointerWrapper(IMFMediaSource* ptr) : ptr(ptr) {};
	~SafeReleasePointerWrapper() { ptr->Shutdown(); SafeRelease(&ptr); }
	IMFMediaSource** getPointer() { return &ptr; }
	IMFMediaSource* get() { return ptr; }
private:
	IMFMediaSource *ptr;
};

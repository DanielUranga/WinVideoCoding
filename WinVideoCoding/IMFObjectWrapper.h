#pragma once

#include <iostream>
#include <utility>

#include "SafeRelease.h"
#include "WindowsError.h"
#include "mfapi.h"
#include <mfidl.h>

namespace IMFWrappers
{

    template<typename T>
    class IMFObjectWrapper
    {
    public:
        IMFObjectWrapper() : ptr(nullptr) {};
        IMFObjectWrapper(const IMFObjectWrapper&) = delete;
        IMFObjectWrapper(IMFObjectWrapper&& other) : ptr(other.ptr) { other.ptr = nullptr; }
        virtual ~IMFObjectWrapper() {};

        T** getPointer() { return &ptr; }
        T* get() { return ptr; }

    protected:
        T* ptr;
    };

    // ------------------------------------------------------------------------

    template<typename T>
    class AddRefWrapper : public IMFObjectWrapper<T>
    {
    public:
        AddRefWrapper() {};
        AddRefWrapper(const AddRefWrapper&) = delete;

        virtual void addRef()
        {
            HRESULT hr = ptr->AddRef();
            if (FAILED(hr))
            {
                throw_windows_error(hr);
            }
        }
    };

    // ------------------------------------------------------------------------

    class IMFAttributesWrapper : public AddRefWrapper<IMFAttributes>
    {

    public:

        IMFAttributesWrapper(const size_t size)
        {
            HRESULT hr = MFCreateAttributes(&ptr, 7);
            if (FAILED(hr))
            {
                throw_windows_error(hr);
            }
        }

        IMFAttributesWrapper(IMFAttributesWrapper&& other)
        {
            this->ptr = other.ptr;
            other.ptr = nullptr;
        }

        void setGUID(REFGUID guidKey, REFGUID guidValue)
        {
            HRESULT hr = ptr->SetGUID(guidKey, guidValue);
            if (FAILED(hr))
            {
                throw_windows_error(hr);
            }
        }

        void setAttributeSize(REFGUID guidKey, UINT32 unWidth, UINT32 unHeight)
        {
            HRESULT hr = MFSetAttributeSize(ptr, guidKey, unWidth, unHeight);
            if (FAILED(hr))
            {
                throw_windows_error(hr);
            }
        }

        void setAttributeRatio(REFGUID guidKey, UINT32 unWidth, UINT32 unHeight)
        {
            HRESULT hr = MFSetAttributeRatio(ptr, guidKey, unWidth, unHeight);
            if (FAILED(hr))
            {
                throw_windows_error(hr);
            }
        }

        void setUINT32(REFGUID guidKey, UINT32 unValue)
        {
            HRESULT hr = ptr->SetUINT32(guidKey, unValue);
            if (FAILED(hr))
            {
                throw_windows_error(hr);
            }
        }

    };

    // ------------------------------------------------------------------------

    class IMFMediaSourceWrapper : public AddRefWrapper<IMFMediaSource>
    {
    public:

        IMFMediaSourceWrapper() {}
        IMFMediaSourceWrapper(const IMFMediaSourceWrapper& other) = delete;

        IMFMediaSourceWrapper(IMFMediaSourceWrapper&& other)
        {
            this->ptr = other.ptr;
            other.ptr = nullptr;
        }

        ~IMFMediaSourceWrapper()
        {
            if (ptr != nullptr)
            {
                ptr->Shutdown();
            }
            SafeRelease(&ptr);
        }

        MFTIME getDuration()
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
                throw_windows_error(hr);
            }
            return pDuration;
        }

        void QueryInterface()
        {
            HRESULT hr = ptr->QueryInterface(IID_PPV_ARGS(&ptr));
            if (FAILED(hr))
            {
                throw_windows_error(hr);
            }
        }

    };

    // ------------------------------------------------------------------------
    class IMFSourceResolverWrapper : public IMFObjectWrapper<IMFSourceResolver>
    {
    public:
        IMFSourceResolverWrapper()
        {
            HRESULT hr = MFCreateSourceResolver(&ptr);
            if (FAILED(hr))
            {
                throw_windows_error(hr);
            }
        }

        template <typename T>
        void CreateObjectFromURL(LPCWSTR pwszURL, DWORD dwFlags, IPropertyStore *pProps, MF_OBJECT_TYPE *pObjectType, IMFObjectWrapper<T>& ppObject)
        {
            HRESULT hr = ptr->CreateObjectFromURL(pwszURL, MF_RESOLUTION_MEDIASOURCE, NULL, pObjectType, reinterpret_cast<IUnknown**>(ppObject.getPointer()));
            if (FAILED(hr))
            {
                throw_windows_error(hr);
            }
        }

    };

    class IMFTranscodeProfileWrapper : public AddRefWrapper<IMFTranscodeProfile>
    {
    public:
        IMFTranscodeProfileWrapper()
        {
            HRESULT hr = MFCreateTranscodeProfile(&ptr);
            if (FAILED(hr))
            {
                throw_windows_error(hr);
            }
        }

        IMFTranscodeProfileWrapper(IMFTranscodeProfileWrapper&& other)
        {
            this->ptr = other.ptr;
            other.ptr = nullptr;
        }

        void SetAudioAttributes(IMFAttributesWrapper& attr)
        {
            HRESULT hr = ptr->SetAudioAttributes(attr.get());
            if (FAILED(hr))
            {
                throw_windows_error(hr);
            }
        }

        void SetContainerAttributes(IMFAttributesWrapper& attr)
        {
            HRESULT hr = ptr->SetContainerAttributes(attr.get());
            if (FAILED(hr))
            {
                throw_windows_error(hr);
            }
        }

        void SetVideoAttributes(IMFAttributesWrapper& attr)
        {
            HRESULT hr = ptr->SetVideoAttributes(attr.get());
            if (FAILED(hr))
            {
                throw_windows_error(hr);
            }
        }
    };

    class IMFTopologyWrapper : public IMFObjectWrapper<IMFTopology>
    {
    public:
        IMFTopologyWrapper(IMFMediaSourceWrapper& pSrc, LPCWSTR pwszOutputFilePath, IMFTranscodeProfileWrapper& pProfile)
        {
            HRESULT hr = MFCreateTranscodeTopology(pSrc.get(), pwszOutputFilePath, pProfile.get(), &ptr);
            if (FAILED(hr))
            {
                throw_windows_error(hr);
            }
        }
    };

}

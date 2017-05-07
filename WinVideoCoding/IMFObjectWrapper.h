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
            DO_CHECKED_OPERATION(ptr->AddRef());
        }
    };

    // ------------------------------------------------------------------------

    class IMFAttributesWrapper : public AddRefWrapper<IMFAttributes>
    {

    public:

        IMFAttributesWrapper(const size_t size)
        {
            DO_CHECKED_OPERATION(MFCreateAttributes(&ptr, 7));
        }

        IMFAttributesWrapper(IMFAttributesWrapper&& other)
        {
            this->ptr = other.ptr;
            other.ptr = nullptr;
        }

        void setGUID(REFGUID guidKey, REFGUID guidValue)
        {
            DO_CHECKED_OPERATION(ptr->SetGUID(guidKey, guidValue));
        }

        void setAttributeSize(REFGUID guidKey, UINT32 unWidth, UINT32 unHeight)
        {
            DO_CHECKED_OPERATION(MFSetAttributeSize(ptr, guidKey, unWidth, unHeight));
        }

        void setAttributeRatio(REFGUID guidKey, UINT32 unWidth, UINT32 unHeight)
        {
            DO_CHECKED_OPERATION(MFSetAttributeRatio(ptr, guidKey, unWidth, unHeight));
        }

        void setUINT32(REFGUID guidKey, UINT32 unValue)
        {
            DO_CHECKED_OPERATION(ptr->SetUINT32(guidKey, unValue));
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
            DO_CHECKED_OPERATION(ptr->CreatePresentationDescriptor(&pPD));
            UINT64 tmp;
            DO_CHECKED_OPERATION(pPD->GetUINT64(MF_PD_DURATION, &tmp));
            pDuration = tmp;
            pPD->Release();
            return pDuration;
        }

        void QueryInterface()
        {
            DO_CHECKED_OPERATION(ptr->QueryInterface(IID_PPV_ARGS(&ptr)));
        }

    };

    // ------------------------------------------------------------------------
    class IMFSourceResolverWrapper : public IMFObjectWrapper<IMFSourceResolver>
    {
    public:
        IMFSourceResolverWrapper()
        {
            DO_CHECKED_OPERATION(MFCreateSourceResolver(&ptr));
        }

        template <typename T>
        void CreateObjectFromURL(LPCWSTR pwszURL, DWORD dwFlags, IPropertyStore *pProps, MF_OBJECT_TYPE *pObjectType, IMFObjectWrapper<T>& ppObject)
        {
            DO_CHECKED_OPERATION(ptr->CreateObjectFromURL(pwszURL, MF_RESOLUTION_MEDIASOURCE, NULL, pObjectType, reinterpret_cast<IUnknown**>(ppObject.getPointer())));
        }

    };

    class IMFTranscodeProfileWrapper : public AddRefWrapper<IMFTranscodeProfile>
    {
    public:
        IMFTranscodeProfileWrapper()
        {
            DO_CHECKED_OPERATION(MFCreateTranscodeProfile(&ptr));
        }

        IMFTranscodeProfileWrapper(IMFTranscodeProfileWrapper&& other)
        {
            this->ptr = other.ptr;
            other.ptr = nullptr;
        }

        void SetAudioAttributes(IMFAttributesWrapper& attr)
        {
            DO_CHECKED_OPERATION(ptr->SetAudioAttributes(attr.get()));
        }

        void SetContainerAttributes(IMFAttributesWrapper& attr)
        {
            DO_CHECKED_OPERATION(ptr->SetContainerAttributes(attr.get()));
        }

        void SetVideoAttributes(IMFAttributesWrapper& attr)
        {
            DO_CHECKED_OPERATION(ptr->SetVideoAttributes(attr.get()));
        }
    };

    class IMFTopologyWrapper : public IMFObjectWrapper<IMFTopology>
    {
    public:
        IMFTopologyWrapper(IMFMediaSourceWrapper& pSrc, LPCWSTR pwszOutputFilePath, IMFTranscodeProfileWrapper& pProfile)
        {
            DO_CHECKED_OPERATION(MFCreateTranscodeTopology(pSrc.get(), pwszOutputFilePath, pProfile.get(), &ptr));
        }
    };

}

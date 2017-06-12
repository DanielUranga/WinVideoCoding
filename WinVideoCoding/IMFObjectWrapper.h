#pragma once

#include <iostream>
#include <utility>

#include "SafeRelease.h"
#include "WindowsError.h"

#include <Windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <Mfreadwrite.h>
#include <mferror.h>

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
        T* get() const { return ptr; }

    protected:
        T* ptr;
    };

    // ------------------------------------------------------------------------

    template<typename T>
    class AddRefWrapper : public virtual IMFObjectWrapper<T>
    {
    public:
        AddRefWrapper() {};

        virtual void addRef()
        {
            DO_CHECKED_OPERATION(ptr->AddRef());
        }
    };

    // ------------------------------------------------------------------------

    template<typename T>
    struct IMFHasAttributes : public virtual IMFObjectWrapper<T>
    {
        void setAttributeSize(REFGUID guidKey, UINT32 unWidth, UINT32 unHeight)
        {
            DO_CHECKED_OPERATION(MFSetAttributeSize(ptr, guidKey, unWidth, unHeight));
        }

        void setAttributeRatio(REFGUID guidKey, UINT32 unWidth, UINT32 unHeight)
        {
            DO_CHECKED_OPERATION(MFSetAttributeRatio(ptr, guidKey, unWidth, unHeight));
        }

        void setGUID(REFGUID guidKey, REFGUID guidValue)
        {
            DO_CHECKED_OPERATION(ptr->SetGUID(guidKey, guidValue));
        }

        void setUINT32(REFGUID guidKey, UINT32 unValue)
        {
            DO_CHECKED_OPERATION(ptr->SetUINT32(guidKey, unValue));
        }

    };

    // ------------------------------------------------------------------------

    struct IMFAttributesWrapper : public AddRefWrapper<IMFAttributes>, public IMFHasAttributes<IMFAttributes>
    {
        IMFAttributesWrapper(const size_t size)
        {
            DO_CHECKED_OPERATION(MFCreateAttributes(&ptr, size));
        }
    };

    // ------------------------------------------------------------------------

    struct IMFMediaTypeWrapper : public IMFHasAttributes<IMFMediaType>
    {
        IMFMediaTypeWrapper()
        {
            DO_CHECKED_OPERATION(MFCreateMediaType(&ptr));
        }
    };

    // ------------------------------------------------------------------------

    struct IMFMediaSourceWrapper : public AddRefWrapper<IMFMediaSource>
    {

        IMFMediaSourceWrapper() {}

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
                ptr->Release();
                ptr = nullptr;
            }
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

    struct IMFTopologyWrapper : public IMFObjectWrapper<IMFTopology>
    {
        IMFTopologyWrapper(IMFMediaSourceWrapper& pSrc, LPCWSTR pwszOutputFilePath, IMFTranscodeProfileWrapper& pProfile)
        {
            DO_CHECKED_OPERATION(MFCreateTranscodeTopology(pSrc.get(), pwszOutputFilePath, pProfile.get(), &ptr));
        }
    };

    // ------------------------------------------------------------------------

    struct IMFSinkWriterWrapper : public IMFObjectWrapper<IMFSinkWriter>
    {

        IMFSinkWriterWrapper(IMFSinkWriterWrapper&& other) : IMFObjectWrapper<IMFSinkWriter>(std::move(other)) {}

        ~IMFSinkWriterWrapper()
        {
            if (ptr != nullptr)
            {
                ptr->Finalize();
            }
        }

        // TODO: pAttributes should be of type std::optional<IMFAttributesWrapper>
        IMFSinkWriterWrapper(std::string pwszOutputURL, IMFByteStream *pByteStream, IMFAttributes *pAttributes)
        {
            DO_CHECKED_OPERATION(MFCreateSinkWriterFromURL(L"output.wmv", nullptr, nullptr, &ptr));
        }

        DWORD AddStream(const IMFMediaTypeWrapper& pTargetMediaType)
        {
            DWORD streamIndex;
            DO_CHECKED_OPERATION(ptr->AddStream(pTargetMediaType.get(), &streamIndex));
            return streamIndex;
        }

        void beginWritting()
        {
            DO_CHECKED_OPERATION(ptr->BeginWriting());
        }

        // TODO: pEncodingParameters should be of type std::optional<IMFAttributesWrapper>
        void setInputMediaType(DWORD dwStreamIndex, const IMFMediaTypeWrapper& pInputMediaType, IMFAttributes *pEncodingParameters)
        {
            DO_CHECKED_OPERATION(ptr->SetInputMediaType(dwStreamIndex, pInputMediaType.get(), nullptr));
        }
    };

}

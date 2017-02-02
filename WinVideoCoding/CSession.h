#pragma once

#include <Mfobjects.h>
#include <Mfidl.h>

#include "SafeRelease.h"

class CSession : public IMFAsyncCallback
{
public:
	static HRESULT Create(CSession **ppSession);

	// IUnknown methods
	STDMETHODIMP QueryInterface(REFIID riid, void** ppv);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();

	// IMFAsyncCallback methods
	STDMETHODIMP GetParameters(DWORD* pdwFlags, DWORD* pdwQueue)
	{
		// Implementation of this method is optional.
		return E_NOTIMPL;
	}
	STDMETHODIMP Invoke(IMFAsyncResult *pResult);

	// Other methods
	HRESULT StartEncodingSession(IMFTopology *pTopology);
	HRESULT GetEncodingPosition(MFTIME *pTime);
	HRESULT Wait(DWORD dwMsec);

private:
	CSession() : m_cRef(1), m_pSession(NULL), m_pClock(NULL), m_hrStatus(S_OK), m_hWaitEvent(NULL)
	{
	}
	virtual ~CSession()
	{
		if (m_pSession)
		{
			m_pSession->Shutdown();
		}

		SafeRelease(&m_pClock);
		SafeRelease(&m_pSession);
		CloseHandle(m_hWaitEvent);
	}

	HRESULT Initialize();

private:
	IMFMediaSession      *m_pSession;
	IMFPresentationClock *m_pClock;
	HRESULT m_hrStatus;
	HANDLE  m_hWaitEvent;
	long    m_cRef;
};

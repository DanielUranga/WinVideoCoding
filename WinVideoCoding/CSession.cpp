#include "CSession.h"

#include <Shlwapi.h>
#include <new>

HRESULT CSession::Create(CSession **ppSession)
{
	*ppSession = NULL;

	CSession *pSession = new (std::nothrow) CSession();
	if (pSession == NULL)
	{
		return E_OUTOFMEMORY;
	}

	HRESULT hr = pSession->Initialize();
	if (FAILED(hr))
	{
		pSession->Release();
		return hr;
	}
	*ppSession = pSession;
	return S_OK;
}

STDMETHODIMP CSession::QueryInterface(REFIID riid, void** ppv)
{
	static const QITAB qit[] =
	{
		QITABENT(CSession, IMFAsyncCallback),
		{ 0 }
	};
	return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CSession::AddRef()
{
	return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CSession::Release()
{
	long cRef = InterlockedDecrement(&m_cRef);
	if (cRef == 0)
	{
		delete this;
	}
	return cRef;
}

HRESULT CSession::Initialize()
{
	IMFClock *pClock = NULL;

	HRESULT hr = MFCreateMediaSession(NULL, &m_pSession);
	if (FAILED(hr))
	{
		goto done;
	}

	hr = m_pSession->GetClock(&pClock);
	if (FAILED(hr))
	{
		goto done;
	}

	hr = pClock->QueryInterface(IID_PPV_ARGS(&m_pClock));
	if (FAILED(hr))
	{
		goto done;
	}

	hr = m_pSession->BeginGetEvent(this, NULL);
	if (FAILED(hr))
	{
		goto done;
	}

	m_hWaitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (m_hWaitEvent == NULL)
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
	}
done:
	SafeRelease(&pClock);
	return hr;
}

// Implements IMFAsyncCallback::Invoke
STDMETHODIMP CSession::Invoke(IMFAsyncResult *pResult)
{
	IMFMediaEvent* pEvent = NULL;
	MediaEventType meType = MEUnknown;
	HRESULT hrStatus = S_OK;

	HRESULT hr = m_pSession->EndGetEvent(pResult, &pEvent);
	if (FAILED(hr))
	{
		goto done;
	}

	hr = pEvent->GetType(&meType);
	if (FAILED(hr))
	{
		goto done;
	}

	hr = pEvent->GetStatus(&hrStatus);
	if (FAILED(hr))
	{
		goto done;
	}

	if (FAILED(hrStatus))
	{
		hr = hrStatus;
		goto done;
	}

	switch (meType)
	{
	case MESessionEnded:
		hr = m_pSession->Close();
		if (FAILED(hr))
		{
			goto done;
		}
		break;

	case MESessionClosed:
		SetEvent(m_hWaitEvent);
		break;
	}

	if (meType != MESessionClosed)
	{
		hr = m_pSession->BeginGetEvent(this, NULL);
	}

done:
	if (FAILED(hr))
	{
		m_hrStatus = hr;
		m_pSession->Close();
	}

	SafeRelease(&pEvent);
	return hr;
}

HRESULT CSession::StartEncodingSession(IMFTopology *pTopology)
{
	HRESULT hr = m_pSession->SetTopology(0, pTopology);
	if (SUCCEEDED(hr))
	{
		PROPVARIANT varStart;
		PropVariantClear(&varStart);
		hr = m_pSession->Start(&GUID_NULL, &varStart);
	}
	return hr;
}

HRESULT CSession::GetEncodingPosition(MFTIME *pTime)
{
	return m_pClock->GetTime(pTime);
}

HRESULT CSession::Wait(DWORD dwMsec)
{
	HRESULT hr = S_OK;

	DWORD dwTimeoutStatus = WaitForSingleObject(m_hWaitEvent, dwMsec);
	if (dwTimeoutStatus != WAIT_OBJECT_0)
	{
		hr = E_PENDING;
	}
	else
	{
		hr = m_hrStatus;
	}
	return hr;
}

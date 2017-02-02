#pragma once

#include <winerror.h>

#include <string>
#include <locale>
#include <codecvt>

class WindowsError {
public:

	explicit WindowsError(int errorCode) : errorCode(errorCode) {}

	std::string toString() const {
		wchar_t wBuf[256];
		FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, NULL, errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), wBuf, 256, NULL);
		return wideCharToMultiByte(wBuf);
	}

private:

	std::string wideCharToMultiByte(const wchar_t *lpwstr) const {
		int size = WideCharToMultiByte(CP_UTF8, 0, lpwstr, -1, NULL, 0, NULL, NULL);
		char *buffer = new char[size + 1];
		WideCharToMultiByte(CP_UTF8, 0, lpwstr, -1, buffer, size, NULL, NULL);
		std::string str(buffer);
		delete[]buffer;
		return str;
	}

	int errorCode;
};

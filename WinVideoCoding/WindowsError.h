#pragma once

#include <windows.h>
#include <winerror.h>

#include <sstream>
#include <string>
#include <locale>
#include <codecvt>

#define throw_windows_error(arg) throw WindowsError(arg, __FILE__, __LINE__);

class WindowsError {
public:

    explicit WindowsError(int errorCode, const char *file, int line) : errorCode(errorCode), file(file), line(line) {}

    std::string toString() const {
        wchar_t wBuf[256];
        FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, NULL, errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), wBuf, 256, NULL);
        std::string excp = wideCharToMultiByte(wBuf);
        std::ostringstream o;
        o << file << ":" << line << ": " << excp;
        return o.str();
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
    std::string file;
    int line;
};

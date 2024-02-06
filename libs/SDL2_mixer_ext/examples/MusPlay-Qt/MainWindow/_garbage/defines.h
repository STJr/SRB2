#ifndef DEFINES_H
#define DEFINES_H

#ifdef MUSPLAY_USE_WINAPI
#include <windows.h>
#include <string>
typedef std::string QString;

inline std::string Wstr2Str(const std::wstring &in)
{
    std::string out;
    out.resize(in.size()*2);
    int res = WideCharToMultiByte(CP_UTF8, 0, (wchar_t*)in.c_str(), in.size(), (char*)out.c_str(), out.size(), 0, 0);
    out.resize(res);
    return out;
}

inline std::wstring Str2Wstr(const std::string &in)
{
    std::wstring out;
    out.resize(in.size());
    int res = MultiByteToWideChar(CP_UTF8, 0, (char*)in.c_str(), in.size(), (wchar_t*)out.c_str(), out.size());
    out.resize(res);
    return out;
}
#endif

#endif // DEFINES_H

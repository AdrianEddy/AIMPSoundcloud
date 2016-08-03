#pragma once

#include <string>
#include <cinttypes>
#include "Config.h"
#include "rapidjson/document.h"

#define DebugA(...) { char msg[2048]; sprintf_s(msg, __VA_ARGS__); OutputDebugStringA(msg); }
#define DebugW(...) { wchar_t msg[2048]; wsprintf(msg, __VA_ARGS__); OutputDebugStringW(msg); }

struct Tools {
    static std::wstring ToWString(const std::string &);
    static std::string ToString(const std::wstring &);
    static std::wstring ToWString(const char *);
    static std::wstring ToWString(const rapidjson::Value &);

    template<typename T>
    static void ReplaceString(const T &search, const T &replace, T &subject) {
        size_t pos = 0;
        while ((pos = subject.find(search, pos)) != T::npos) {
            subject.replace(pos, search.length(), replace);
            pos += replace.length();
        }
    }

    static int64_t TrackIdFromUrl(const std::wstring &);
    static Config::TrackInfo *TrackInfo(int64_t id);
    static Config::TrackInfo *TrackInfo(IAIMPString *FileName);

    static std::wstring UrlEncode(const std::wstring &);
    static void OutputLastError();
};


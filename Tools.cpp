#include "Tools.h"

#include <windows.h>
#include <locale>
#include <sstream>
#include <iomanip>
#include <codecvt>

static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> wStrConverter;

std::wstring Tools::ToWString(const std::string &string) {
    return wStrConverter.from_bytes(string);
}

std::wstring Tools::ToWString(const char *string) {
    return wStrConverter.from_bytes(string);
}

std::wstring Tools::ToWString(const rapidjson::Value &val) {
    if (val.IsString() && val.GetStringLength() > 0) {
        const char *ptr = val.GetString();
        return wStrConverter.from_bytes(ptr, ptr + val.GetStringLength());
    }
    return std::wstring();
}

std::string Tools::ToString(const std::wstring &string) {
    return wStrConverter.to_bytes(string);
}

void Tools::OutputLastError() {
    DWORD errorMessageID = ::GetLastError();
    if (errorMessageID == 0)
        return;

    LPWSTR messageBuffer = nullptr;
    size_t size = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&messageBuffer, 0, NULL);

    DebugW(L"0x%x: %s", errorMessageID, messageBuffer);

    LocalFree(messageBuffer);
}

std::wstring Tools::UrlEncode(const std::wstring &url) {
    std::wostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (std::wstring::const_iterator i = url.begin(), n = url.end(); i != n; ++i) {
        const std::wstring::value_type c = (*i);
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
            continue;
        }
        escaped << '%' << std::setw(2) << int((unsigned char)c);
    }

    return escaped.str();
}

int64_t Tools::TrackIdFromUrl(const std::wstring &url) {
    std::wstring::size_type ptr, ptr_end;
    if ((ptr = url.find(L"tracks/")) != std::wstring::npos) {
        ptr += 7;
        if ((ptr_end = url.find(L"/", ptr)) != std::wstring::npos) {
            return std::stoll(url.substr(ptr, ptr_end - ptr));
        }
    }
    return 0;
}

void Tools::ReplaceString(const std::string &search, const std::string &replace, std::string &subject) {
    size_t pos = 0;
    while ((pos = subject.find(search, pos)) != std::string::npos) {
        subject.replace(pos, search.length(), replace);
        pos += replace.length();
    }
}

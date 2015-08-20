#include "Tools.h"

#include <windows.h>
#include <locale>
#include <sstream>
#include <iomanip>
#include <codecvt>

std::wstring Tools::ToWString(const std::string &string) {
    static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.from_bytes(string);
}

std::wstring Tools::ToWString(const char *string) {
    static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.from_bytes(string);
}

std::wstring Tools::ToWString(const rapidjson::Value &val) {
    if (val.IsString() && val.GetStringLength() > 0) {
        return Tools::ToWString(val.GetString());
    }
    return std::wstring();
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


/*
std::string Tools::ToString(const std::string &arg) {
    static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.to_bytes(string);
}
*/
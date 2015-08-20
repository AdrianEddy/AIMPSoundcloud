#pragma once

#include "SDK/apiInternet.h"
#include "IUnknownInterfaceImpl.h"
#include <functional>

class AimpHTTP {
    typedef std::function<void(unsigned char *, int)> CallbackFunc;

    class EventListener : public IUnknownInterfaceImpl<IAIMPHTTPClientEvents> {
    public:
        EventListener(CallbackFunc callback, bool isFile = false);

        void WINAPI OnAccept(IAIMPString *ContentType, const INT64 ContentSize, BOOL *Allow);
        void WINAPI OnComplete(IAIMPErrorInfo *ErrorInfo, BOOL Canceled);
        void WINAPI OnProgress(const INT64 Downloaded, const INT64 Total);

    private:
        bool m_isFileStream;
        CallbackFunc m_callback;
        IAIMPStream *m_stream;
        friend class AimpHTTP;
    };

public:
    static bool Get(const std::wstring &url, CallbackFunc callback);
    static bool Download(const std::wstring &url, const std::wstring &destination, CallbackFunc callback);
    static bool Post(const std::wstring &url, const std::string &body, CallbackFunc callback);
    static bool Init(IAIMPCore *Core);

private:
    AimpHTTP();
    AimpHTTP(const AimpHTTP&);
    AimpHTTP& operator=(const AimpHTTP&);

    static IAIMPCore *m_core;
    static IAIMPServiceHTTPClient *m_httpClient;
};

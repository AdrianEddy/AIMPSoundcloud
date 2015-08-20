#include "AimpHTTP.h"

#include "AIMPString.h"
#include "SDK/apiFileManager.h"

IAIMPCore *AimpHTTP::m_core = nullptr;
IAIMPServiceHTTPClient *AimpHTTP::m_httpClient = nullptr;

AimpHTTP::EventListener::EventListener(CallbackFunc callback, bool isFile) : m_isFileStream(isFile), m_callback(callback) {

}

void WINAPI AimpHTTP::EventListener::OnAccept(IAIMPString *ContentType, const INT64 ContentSize, BOOL *Allow) {
    *Allow = true;
}

void WINAPI AimpHTTP::EventListener::OnComplete(IAIMPErrorInfo *ErrorInfo, BOOL Canceled) {
    // TODO: Error checking
    if (m_stream) {
        if (m_isFileStream) {
            m_stream->Release();
            m_callback(nullptr, 0);
            return;
        }

        int s = (int)m_stream->GetSize();
        unsigned char *buf = new unsigned char[s + 1];
        buf[s] = 0;
        m_stream->Seek(0, AIMP_STREAM_SEEKMODE_FROM_BEGINNING);
        m_stream->Read(buf, s);
        m_stream->Release();
        m_callback(buf, s);
        delete[] buf;
    }
}

void WINAPI AimpHTTP::EventListener::OnProgress(const INT64 Downloaded, const INT64 Total) {

}

bool AimpHTTP::Get(const std::wstring &url, CallbackFunc callback) {
    EventListener *listener = new EventListener(callback);
    m_core->CreateObject(IID_IAIMPMemoryStream, reinterpret_cast<void **>(&(listener->m_stream)));
    return SUCCEEDED(m_httpClient->Get(new AIMPString(url), 0, listener->m_stream, listener, 0, nullptr));
}

bool AimpHTTP::Download(const std::wstring &url, const std::wstring &destination, CallbackFunc callback) {
    EventListener *listener = new EventListener(callback, true);
    IAIMPServiceFileStreaming *fileStreaming = nullptr;
    if (SUCCEEDED(m_core->QueryInterface(IID_IAIMPServiceFileStreaming, reinterpret_cast<void **>(&fileStreaming)))) {
        fileStreaming->CreateStreamForFile(new AIMPString(destination), AIMP_SERVICE_FILESTREAMING_FLAG_CREATENEW, -1, -1, &(listener->m_stream));
        fileStreaming->Release();
    }
    
    return SUCCEEDED(m_httpClient->Get(new AIMPString(url), 0, listener->m_stream, listener, 0, nullptr));
}

bool AimpHTTP::Post(const std::wstring &url, const std::string &body, CallbackFunc callback) {
    IAIMPStream *postData = nullptr;
    if (SUCCEEDED(m_core->CreateObject(IID_IAIMPMemoryStream, reinterpret_cast<void**>(&postData)))) {
        postData->Write((unsigned char *)(body.c_str()), body.size() + 1, nullptr);

        EventListener *listener = new EventListener(callback);
        m_core->CreateObject(IID_IAIMPMemoryStream, reinterpret_cast<void **>(&(listener->m_stream)));
        bool ok = SUCCEEDED(m_httpClient->Post(new AIMPString(url), 0, listener->m_stream, postData, listener, 0, nullptr));

        postData->Release();
        return ok;
    }
    return false;
}

bool AimpHTTP::Init(IAIMPCore *Core) {
    m_core = Core;

    return SUCCEEDED(m_core->QueryInterface(IID_IAIMPServiceHTTPClient, reinterpret_cast<void **>(&m_httpClient)));
}
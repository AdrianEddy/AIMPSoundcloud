#include "OptionsDialog.h"
#include "AIMPString.h"
#include "resource.h"
#include "TcpServer.h"
#include "AimpHTTP.h"
#include "Tools.h"
#include "rapidjson/document.h"
#include "AIMPSoundcloud.h"
#include <Shellapi.h>

#include <Commctrl.h>
#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "Msimg32.lib")

#include <gdiplus.h>
#include "GdiPlusImageLoader.h"

extern HINSTANCE g_hInst;

// CONTROL         "", IDC_AVATAR, WC_STATIC, SS_BITMAP | SS_REALSIZEIMAGE, 15, 20, 100, 100, WS_EX_LEFT

// TODO: limit stream entries 
// TODO: add duration to title

OptionsDialog::OptionsDialog(AIMPSoundcloudPlugin *plugin) : m_plugin(plugin), m_handle(NULL) {

}

OptionsDialog::~OptionsDialog() {

}

HRESULT WINAPI OptionsDialog::GetName(IAIMPString **S) {
    *S = new AIMPString(L"SoundCloud");
    return S_OK;
}

HWND WINAPI OptionsDialog::CreateFrame(HWND ParentWnd) {
    m_handle = CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_SETTINGS), ParentWnd, DlgProc, (LPARAM)this);
    if (m_handle != NULL) {
        // TODO OnModified = HandlerModified;
        RECT R;
        GetWindowRect(ParentWnd, &R);
        OffsetRect(&R, -R.left, -R.top);
        SetWindowPos(m_handle, NULL, R.left, R.top, R.right - R.left, R.bottom - R.top, SWP_SHOWWINDOW);
    }
    return m_handle;
}

void WINAPI OptionsDialog::DestroyFrame() {
    DestroyWindow(m_handle);
    m_handle = NULL;
}

void WINAPI OptionsDialog::Notification(int ID) {
    switch (ID) {
        case AIMP_SERVICE_OPTIONSDIALOG_NOTIFICATION_LOCALIZATION: break;
        case AIMP_SERVICE_OPTIONSDIALOG_NOTIFICATION_LOAD: break;
        case AIMP_SERVICE_OPTIONSDIALOG_NOTIFICATION_SAVE: break;
    }
}

inline Gdiplus::Bitmap *getConnectBtnImage(bool connected) {
    static GdiPlusImageLoader connect(IDB_CONNECT, L"PNG");
    static GdiPlusImageLoader disconnect(IDB_DISCONNECT, L"PNG");
    return connected ? disconnect : connect;
}

void OptionsDialog::LoadProfileInfo() {
    if (Config::GetInt64(L"UserId", 0) != 0) {
        UpdateProfileInfo();
        return;
    }

    if (m_plugin->getAccessToken().empty())
        return;

    AimpHTTP::Get(L"https://api.soundcloud.com/me/?oauth_token=" + m_plugin->getAccessToken(), [this](unsigned char *data, int size) {
        rapidjson::Document d;
        d.Parse(reinterpret_cast<const char *>(data));

        if (d.HasMember("id")) {
            Config::SetInt64(L"UserId", d["id"].GetInt64());

            if (d.HasMember("full_name") && d["full_name"].GetStringLength() > 2) {
                Config::SetString(L"UserName", Tools::ToWString(d["full_name"].GetString()));
            } else {
                Config::SetString(L"UserName", Tools::ToWString(d["username"].GetString()));
            }

            if (d.HasMember("avatar_url")) { // Get the avatar
                AimpHTTP::Download(Tools::ToWString(d["avatar_url"].GetString()), Config::PluginConfigFolder() + L"user_avatar.jpg", [this](unsigned char *data, int size) {
                    UpdateProfileInfo();
                });
            }
        }
    });
}

void OptionsDialog::UpdateProfileInfo() {
    std::wstring unameW = Config::GetString(L"UserName");
    SendDlgItemMessage(m_handle, IDC_USERNAME, WM_SETTEXT, 0, (LPARAM)unameW.c_str());

    GdiPlusImageLoader avatar(Config::PluginConfigFolder() + L"user_avatar.jpg");
    HBITMAP hbm = nullptr;
    if (avatar)
        avatar->GetHBITMAP(NULL, &hbm);

    SendDlgItemMessage(m_handle, IDC_AVATAR, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hbm);

    Gdiplus::Bitmap *btnImage = getConnectBtnImage(m_plugin->isConnected());
    SetWindowPos(GetDlgItem(m_handle, IDC_CONNECTBTN), NULL, 0, 0, btnImage->GetWidth(), btnImage->GetHeight(), SWP_NOMOVE | SWP_NOZORDER);

}

LRESULT CALLBACK OptionsDialog::ButtonProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    static OptionsDialog *dialog = nullptr;
    if (!dialog) {
        dialog = (OptionsDialog *)dwRefData;
    }
            
    switch (uMsg) {
        case WM_ERASEBKGND:
            return TRUE;

        case WM_PAINT: {
            Gdiplus::Bitmap *btnImage = getConnectBtnImage(dialog->m_plugin->isConnected());

            HBITMAP hBmp;
            btnImage->GetHBITMAP(NULL, &hBmp);

            RECT rect;
            GetClientRect(hWnd, &rect);

            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);

            HDC hdcMem = CreateCompatibleDC(hdc);
            HGDIOBJ oldBitmap = SelectObject(hdcMem, hBmp);

            BITMAP bitmap;
            GetObject(hBmp, sizeof(bitmap), &bitmap);
            BLENDFUNCTION bf;
            bf.BlendOp = AC_SRC_OVER;
            bf.BlendFlags = 0;
            bf.SourceConstantAlpha = 0xff;
            bf.AlphaFormat = AC_SRC_ALPHA;
            AlphaBlend(hdc, 0, 0, rect.right, rect.bottom, hdcMem, 0, 0, bitmap.bmWidth, bitmap.bmHeight, bf);

            SelectObject(hdcMem, oldBitmap);
            DeleteDC(hdcMem);

            EndPaint(hWnd, &ps);
            return TRUE;
        }
        case WM_SETCURSOR:
            SetCursor(LoadCursor(NULL, IDC_HAND));
            return TRUE;

        case WM_NCDESTROY:
            RemoveWindowSubclass(hWnd, ButtonProc, uIdSubclass);
            break;
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

BOOL CALLBACK OptionsDialog::DlgProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
    static OptionsDialog *dialog = nullptr;
    static AIMPSoundcloudPlugin *plugin = nullptr;

    switch (Msg) {
        case WM_INITDIALOG: {
            dialog = (OptionsDialog *)lParam;
            dialog->m_handle = hwnd;
            plugin = dialog->m_plugin;

            HWND btn = GetDlgItem(hwnd, IDC_CONNECTBTN);
            SetWindowSubclass(btn, ButtonProc, 0, /*OptionsDialog*/lParam);

            dialog->LoadProfileInfo();

            HWND username = GetDlgItem(hwnd, IDC_USERNAME);
            HFONT font = CreateFont(24, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_MODERN, L"Arial");
            SendMessage(username, WM_SETFONT, WPARAM(font), TRUE);
        } break;
        case WM_COMMAND: {
            switch (LOWORD(wParam)) {
                case IDC_CONNECTBTN: 
                    if (!plugin->isConnected()) {
                        #define RESP(Title, Text) \
                            *response = "HTTP/1.1 200 OK\r\n" \
                                        "Content-Type: text/html\r\n" \
                                        "Connection: close\r\n" \
                                        "Cache-Control: no-store, no-cache, must-revalidate, post-check=0, pre-check=0\r\n" \
                                        "Pragma: no-cache\r\n" \
                                        "Server: AIMPSoundcloud plugin\r\n" \
                                        "\r\n" \
                                        "<html><head><title>" Title "</title>" \
                                        "<style>" \
                                        "body { padding-top: 50pt; }" \
                                        "p,h1 { font-family: Verdana,Arial,sans-serif; text-align: center; font-size: 14pt; }" \
                                        "h1   { font-size: 22pt; }" \
                                        "</style></head><body>" \
                                        "<h1>" Title "</h1>" \
                                        "<p>" Text "</p>" \
                                        "<script type=\"text/javascript\">window.open('', '_self', ''), window.close();</script></body></html>"

                        (new TcpServer(38916, [] (TcpServer *s, char *request, char **response) -> bool {
                            char *token = strstr(request, "code=");
                            if (token) {
                                token += 5;
                                if (char *amp = strstr(token, "&")) *amp = 0;
                                if (char *space = strstr(token, " ")) *space = 0;

                                DebugA("Access code: %s\n", token);

                                std::string postData = "client_id=" CLIENT_ID "&client_secret=" CLIENT_SECRET "&grant_type=authorization_code&redirect_uri=http%3A%2F%2Flocalhost%3A38916%2F&code=";
                                postData += token;

                                AimpHTTP::Post(L"https://api.soundcloud.com/oauth2/token", postData, [](unsigned char *data, int size) {
                                    rapidjson::Document d;
                                    d.Parse(reinterpret_cast<const char *>(data));

                                    if (d.HasMember("access_token")) {
                                        plugin->setAccessToken(Tools::ToWString(d["access_token"].GetString()));
                                        Config::SetString(L"AccessToken", plugin->getAccessToken());
                                        dialog->LoadProfileInfo();
                                    }
                                });

                                RESP("Authorization Granted", "You may now close this browser window and return to AIMP.");
                                return true;
                            }

                            RESP("Error occured", "Couldn't connect to SoundCloud account.");
                            return true;
                        }))->Start();

                        #undef RESP

                        ShellExecuteA(dialog->m_handle, "open", "https://soundcloud.com/connect?client_id=" CLIENT_ID "&redirect_uri=http%3A%2F%2Flocalhost%3A38916%2F&response_type=code&scope=non-expiring", NULL, NULL, SW_SHOWNORMAL);
                    } else {
                        Config::Delete(L"AccessToken");
                        Config::Delete(L"UserName");
                        Config::Delete(L"UserId");
                        plugin->setAccessToken(L"");
                        std::wstring path = Config::PluginConfigFolder() + L"user_avatar.jpg";
                        DeleteFile(path.c_str());

                        dialog->UpdateProfileInfo();
                    }
                break;
            }
        } break;
        default: return FALSE;
    }
    return TRUE;
}

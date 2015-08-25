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

OptionsDialog::OptionsDialog(Plugin *plugin) : m_userId(0), m_plugin(plugin), m_handle(NULL) {
    if (FAILED(m_plugin->core()->QueryInterface(IID_IAIMPServiceOptionsDialog, reinterpret_cast<void **>(&m_dialogService)))) {
        m_dialogService = nullptr;
    }
}

OptionsDialog::~OptionsDialog() {
    if (m_dialogService)
        m_dialogService->Release();
}

void OptionsDialog::OptionsModified() {
    if (m_dialogService)
        m_dialogService->FrameModified(this);
}

HRESULT WINAPI OptionsDialog::GetName(IAIMPString **S) {
    *S = new AIMPString(L"SoundCloud");
    return S_OK;
}

HWND WINAPI OptionsDialog::CreateFrame(HWND ParentWnd) {
    m_handle = CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_SETTINGS), ParentWnd, DlgProc, (LPARAM)this);
    if (m_handle != NULL) {
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
        case AIMP_SERVICE_OPTIONSDIALOG_NOTIFICATION_LOAD: {
            m_userId    = Config::GetInt64(L"UserId");
            m_userName  = Config::GetString(L"UserName");
            m_userLogin = Config::GetString(L"UserLogin");
            m_userInfo  = Config::GetString(L"UserInfo");
            m_plugin->setAccessToken(Config::GetString(L"AccessToken"));

            SendDlgItemMessage(m_handle, IDC_ADDDURATION, BM_SETCHECK, Config::GetInt32(L"AddDurationToTitle", 0), 0);
            SendDlgItemMessage(m_handle, IDC_LIMITSTREAM, BM_SETCHECK, Config::GetInt32(L"LimitUserStream", 1), 0);
            SendDlgItemMessage(m_handle, IDC_LIMITSTREAMVALUESPIN, UDM_SETPOS32, 0, Config::GetInt32(L"LimitUserStreamValue", 5000));

            SendDlgItemMessage(m_handle, IDC_MONITORLIKES, BM_SETCHECK, Config::GetInt32(L"MonitorLikes", 1), 0);
            SendDlgItemMessage(m_handle, IDC_MONITORSTREAM, BM_SETCHECK, Config::GetInt32(L"MonitorStream", 1), 0);
            SendDlgItemMessage(m_handle, IDC_CHECKONSTARTUP, BM_SETCHECK, Config::GetInt32(L"CheckOnStartup", 1), 0);
            SendDlgItemMessage(m_handle, IDC_CHECKEVERY, BM_SETCHECK, Config::GetInt32(L"CheckEveryEnabled", 1), 0);
            SendDlgItemMessage(m_handle, IDC_CHECKEVERYVALUESPIN, UDM_SETPOS32, 0, Config::GetInt32(L"CheckEveryHours", 1));

            BOOL enable = SendDlgItemMessage(m_handle, IDC_LIMITSTREAM, BM_GETCHECK, 0, 0) == BST_CHECKED;
            EnableWindow(GetDlgItem(m_handle, IDC_LIMITSTREAMVALUE), enable);
            EnableWindow(GetDlgItem(m_handle, IDC_LIMITSTREAMVALUESPIN), enable);
            EnableWindow(GetDlgItem(m_handle, IDC_TRACKS), enable);

            enable = SendDlgItemMessage(m_handle, IDC_CHECKEVERY, BM_GETCHECK, 0, 0) == BST_CHECKED;
            EnableWindow(GetDlgItem(m_handle, IDC_CHECKEVERYVALUE), enable);
            EnableWindow(GetDlgItem(m_handle, IDC_CHECKEVERYVALUESPIN), enable);
            EnableWindow(GetDlgItem(m_handle, IDC_HOURS), enable);

            LoadProfileInfo();
        } break;
        case AIMP_SERVICE_OPTIONSDIALOG_NOTIFICATION_SAVE: {
            Config::SetInt64(L"UserId", m_userId);
            Config::SetString(L"UserName", m_userName);
            Config::SetString(L"UserLogin", m_userLogin);
            Config::SetString(L"UserInfo", m_userInfo);

            Config::SetString(L"AccessToken", m_plugin->getAccessToken());

            Config::SetInt32(L"LimitUserStreamValue", SendDlgItemMessage(m_handle, IDC_LIMITSTREAMVALUESPIN, UDM_GETPOS32, 0, 0));
            Config::SetInt32(L"CheckEveryHours", SendDlgItemMessage(m_handle, IDC_CHECKEVERYVALUESPIN, UDM_GETPOS32, 0, 0));

            Config::SetInt32(L"AddDurationToTitle", SendDlgItemMessage(m_handle, IDC_ADDDURATION, BM_GETCHECK, 0, 0) == BST_CHECKED);
            Config::SetInt32(L"LimitUserStream", SendDlgItemMessage(m_handle, IDC_LIMITSTREAM, BM_GETCHECK, 0, 0) == BST_CHECKED);
            Config::SetInt32(L"CheckEveryEnabled", SendDlgItemMessage(m_handle, IDC_CHECKEVERY, BM_GETCHECK, 0, 0) == BST_CHECKED);
            Config::SetInt32(L"CheckOnStartup", SendDlgItemMessage(m_handle, IDC_CHECKONSTARTUP, BM_GETCHECK, 0, 0) == BST_CHECKED);
            Config::SetInt32(L"MonitorLikes", SendDlgItemMessage(m_handle, IDC_MONITORLIKES, BM_GETCHECK, 0, 0) == BST_CHECKED);
            Config::SetInt32(L"MonitorStream", SendDlgItemMessage(m_handle, IDC_MONITORSTREAM, BM_GETCHECK, 0, 0) == BST_CHECKED);

            m_plugin->StartMonitorTimer();
        } break;
    }
}

inline Gdiplus::Bitmap *getConnectBtnImage(bool connected) {
    static GdiPlusImageLoader connect(IDB_CONNECT, L"PNG");
    static GdiPlusImageLoader disconnect(IDB_DISCONNECT, L"PNG");
    return connected ? disconnect : connect;
}

void OptionsDialog::LoadProfileInfo() {
    if (m_userId > 0 || m_plugin->getAccessToken().empty()) {
        UpdateProfileInfo();
        return;
    }

    AimpHTTP::Get(L"https://api.soundcloud.com/me/?oauth_token=" + m_plugin->getAccessToken(), [this](unsigned char *data, int size) {
        rapidjson::Document d;
        d.Parse(reinterpret_cast<const char *>(data));

        if (d.HasMember("id")) {
            m_userId = d["id"].GetInt64();

            if (d.HasMember("full_name") && d["full_name"].GetStringLength() > 2) {
                m_userName = Tools::ToWString(d["full_name"]);
                m_userLogin = Tools::ToWString(d["username"]);
            } else {
                m_userName = Tools::ToWString(d["username"]);
            }

            m_userInfo.clear();
            std::wstring city = Tools::ToWString(d["city"]);
            std::wstring country = Tools::ToWString(d["country"]);
                 if (!city.empty() && !country.empty()) m_userInfo += city + L", " + country;
            else if (!country.empty())                  m_userInfo += country;
            else if (!city.empty())                     m_userInfo += city;

            OptionsModified();

            if (d.HasMember("avatar_url")) { // Get the avatar
                AimpHTTP::Download(Tools::ToWString(d["avatar_url"]), Config::PluginConfigFolder() + L"user_avatar.jpg", [this](unsigned char *data, int size) {
                    UpdateProfileInfo();
                    OptionsModified();
                });
            }
        }
    });
}

void OptionsDialog::UpdateProfileInfo() {
    RECT rc;
    HWND authGroupBox = GetDlgItem(m_handle, IDC_AUTHGROUPBOX);
    GetClientRect(authGroupBox, &rc);
    MapWindowPoints(authGroupBox, m_handle, (LPPOINT)&rc, 2);

    if (m_userId > 0 && m_plugin->isConnected()) {
        SendDlgItemMessage(m_handle, IDC_USERNAME, WM_SETTEXT, 0, (LPARAM)m_userName.c_str());

        std::wstring uinfo = m_userInfo;
        if (!m_userLogin.empty())
            uinfo = m_userLogin + L"\r\n" + m_userInfo;
        SendDlgItemMessage(m_handle, IDC_USERINFO, WM_SETTEXT, 0, (LPARAM)uinfo.c_str());

        Gdiplus::Bitmap *btnImage = getConnectBtnImage(true);
        SetWindowPos(GetDlgItem(m_handle, IDC_CONNECTBTN), NULL, rc.right - btnImage->GetWidth() - 10, rc.bottom - btnImage->GetHeight() - 10, btnImage->GetWidth(), btnImage->GetHeight(), SWP_NOZORDER);

        GdiPlusImageLoader avatar(Config::PluginConfigFolder() + L"user_avatar.jpg");
        HBITMAP hbm = nullptr;
        if (avatar) {
            avatar->GetHBITMAP(NULL, &hbm); // Released in AvatarProc
            SendDlgItemMessage(m_handle, IDC_AVATAR, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hbm);
        }
    } else {
        SendDlgItemMessage(m_handle, IDC_USERINFO, WM_SETTEXT, 0, NULL);
        SendDlgItemMessage(m_handle, IDC_USERNAME, WM_SETTEXT, 0, NULL);
        SendDlgItemMessage(m_handle, IDC_AVATAR, STM_SETIMAGE, IMAGE_BITMAP, NULL);

        Gdiplus::Bitmap *btnImage = getConnectBtnImage(false);
        SetWindowPos(GetDlgItem(m_handle, IDC_CONNECTBTN), NULL, rc.left + ((rc.right - rc.left) - btnImage->GetWidth()) / 2, rc.top + ((rc.bottom - rc.top) - btnImage->GetHeight()) / 2, btnImage->GetWidth(), btnImage->GetHeight(), SWP_NOZORDER);
    }
    GetClientRect(m_handle, &rc);
    RedrawWindow(m_handle, &rc, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_ALLCHILDREN);
}

LRESULT CALLBACK OptionsDialog::ButtonProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    static OptionsDialog *dialog = nullptr;
    static HBITMAP hBmp = nullptr;
    static bool previousConnected = false;
    if (!dialog) {
        dialog = (OptionsDialog *)dwRefData;
        Gdiplus::Bitmap *btnImage = getConnectBtnImage(dialog->m_plugin->isConnected());
        btnImage->GetHBITMAP(NULL, &hBmp);
        previousConnected = dialog->m_plugin->isConnected();
    } else if (previousConnected != dialog->m_plugin->isConnected()) {
        if (hBmp)
            DeleteObject(hBmp);
        Gdiplus::Bitmap *btnImage = getConnectBtnImage(dialog->m_plugin->isConnected());
        btnImage->GetHBITMAP(NULL, &hBmp);
        previousConnected = dialog->m_plugin->isConnected();
    }
    
    switch (uMsg) {
        case WM_PAINT: {
            RECT rect;
            GetClientRect(hWnd, &rect);

            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);

            HDC hdcMem = CreateCompatibleDC(hdc);
            SelectObject(hdcMem, hBmp);

            BITMAP bitmap;
            GetObject(hBmp, sizeof(bitmap), &bitmap);
            BLENDFUNCTION bf;
            bf.BlendOp = AC_SRC_OVER;
            bf.BlendFlags = 0;
            bf.SourceConstantAlpha = 0xff;
            bf.AlphaFormat = AC_SRC_ALPHA;
            AlphaBlend(hdc, 0, 0, rect.right, rect.bottom, hdcMem, 0, 0, bitmap.bmWidth, bitmap.bmHeight, bf);

            DeleteDC(hdcMem);

            EndPaint(hWnd, &ps);
            return TRUE;
        }
        case WM_SETCURSOR:
            SetCursor(LoadCursor(NULL, IDC_HAND));
            return TRUE;
        case WM_DESTROY:
            DeleteObject(hBmp);
            dialog = nullptr;
        break;
        case WM_NCDESTROY:
            RemoveWindowSubclass(hWnd, ButtonProc, uIdSubclass);
            break;
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK OptionsDialog::FrameProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    static OptionsDialog *dialog = nullptr;
    static HBRUSH bgBrush;
    static HPEN penOuter;
    static HPEN penInner;
    static RECT rect;
    static RECT captionRect;
    static HFONT captionFont;
    static TRIVERTEX gradient[2];
    static wchar_t text[64];

    if (!dialog) {
        dialog = (OptionsDialog *)dwRefData;

        GetWindowText(hWnd, text, 64);

        bgBrush = CreateSolidBrush(RGB(240, 240, 240));
        penOuter = CreatePen(PS_SOLID, 1, RGB(188, 188, 188));
        penInner = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
        captionFont = CreateFont(13, 0, 0, 0, FW_BLACK, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_MODERN, L"Tahoma");

        GetClientRect(hWnd, &rect);
        rect.right += 7;
        rect.bottom += 11;

        captionRect = rect;
        captionRect.bottom = 21;

        gradient[0] = { captionRect.left  + 1, captionRect.top + 1, 0xff00, 0x8800, 0x0000, 0xffff }; // #ff8800
        gradient[1] = { captionRect.right - 1, captionRect.bottom,  0xff00, 0x3300, 0x0000, 0xffff }; // #ff3300
    }

    switch (uMsg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            SelectObject(hdc, bgBrush);

            // Outer frame
            SelectObject(hdc, penOuter);
            Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);

            SelectObject(hdc, penInner);
            Rectangle(hdc, rect.left + 1, rect.top + 1, rect.right - 1, rect.bottom - 1);

            // Caption
            SelectObject(hdc, penOuter);
            Rectangle(hdc, rect.left, rect.top, rect.right, captionRect.bottom + 1);

            GRADIENT_RECT r = { 0, 1 };
            GradientFill(hdc, gradient, 2, &r, 1, GRADIENT_FILL_RECT_V);

            SetTextColor(hdc, 0x00ffffff);
            SetBkMode(hdc, TRANSPARENT);

            SelectObject(hdc, captionFont);
            DrawText(hdc, text, wcslen(text), &captionRect, DT_NOCLIP | DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            EndPaint(hWnd, &ps);
            return TRUE;
        }
        case WM_DESTROY:
            DeleteObject(bgBrush);
            DeleteObject(penOuter);
            DeleteObject(penInner);
            DeleteObject(captionFont);
            dialog = nullptr;
            break;
        case WM_NCDESTROY:
            RemoveWindowSubclass(hWnd, FrameProc, uIdSubclass);
            break;
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK OptionsDialog::GroupBoxProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    static OptionsDialog *dialog = nullptr;
    static HBRUSH bgBrush;
    static HPEN penOuter;
    static HPEN penInner;
    static HFONT captionFont;

    if (!dialog) {
        dialog = (OptionsDialog *)dwRefData;

        bgBrush = CreateSolidBrush(RGB(240, 240, 240));
        penOuter = CreatePen(PS_SOLID, 1, RGB(188, 188, 188));
        penInner = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
        captionFont = CreateFont(13, 0, 0, 0, FW_REGULAR, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_MODERN, L"Tahoma");
    }

    switch (uMsg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            SelectObject(hdc, GetStockObject(NULL_BRUSH));

            wchar_t text[64];
            GetWindowText(hWnd, text, 64);

            RECT rect;
            GetClientRect(hWnd, &rect);

            // Outer frame
            SelectObject(hdc, penOuter);
            Rectangle(hdc, rect.left, rect.top + 7, rect.right, rect.bottom);

            SelectObject(hdc, penInner);
            Rectangle(hdc, rect.left + 1, rect.top + 7 + 1, rect.right - 1, rect.bottom - 1);

            // Caption
            SetTextColor(hdc, RGB(0x80, 0, 0));
            SetBkMode(hdc, TRANSPARENT);
            SelectObject(hdc, captionFont);

            RECT captionRect(rect);
            captionRect.left += 5;

            SIZE textSize;
            GetTextExtentPoint32(hdc, text, wcslen(text), &textSize);
            LPtoDP(hdc, (POINT *)&textSize, 2);
            captionRect.right = captionRect.left + textSize.cx + 10;
            captionRect.bottom = captionRect.top + textSize.cy;

            FillRect(hdc, &captionRect, bgBrush);

            captionRect.left += 5;
            DrawText(hdc, text, wcslen(text), &captionRect, DT_NOCLIP | DT_LEFT | DT_TOP | DT_SINGLELINE);

            EndPaint(hWnd, &ps);
            return TRUE;
        }
        case WM_DESTROY:
            DeleteObject(bgBrush);
            DeleteObject(penOuter);
            DeleteObject(penInner);
            DeleteObject(captionFont);
            dialog = nullptr;
            break;
        case WM_NCDESTROY:
            RemoveWindowSubclass(hWnd, GroupBoxProc, uIdSubclass);
            break;
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK OptionsDialog::AvatarProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    static OptionsDialog *dialog = nullptr;
    static HPEN penOuter;
    static HPEN penInner;

    if (!dialog) {
        dialog = (OptionsDialog *)dwRefData;
        penOuter = CreatePen(PS_SOLID, 1, RGB(188, 188, 188));
        penInner = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
    }

    switch (uMsg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);

            HBITMAP bmp = (HBITMAP)SendMessage(hWnd, STM_GETIMAGE, IMAGE_BITMAP, NULL);

            if (bmp) {
                HDC hdcMem = CreateCompatibleDC(hdc);
                SelectObject(hdcMem, bmp);
                BITMAP bitmap;
                GetObject(bmp, sizeof(bitmap), &bitmap);
                BitBlt(hdc, 0, 0, bitmap.bmWidth, bitmap.bmHeight, hdcMem, 0, 0, SRCCOPY);
                DeleteDC(hdcMem);

                SelectObject(hdc, GetStockObject(NULL_BRUSH));

                SelectObject(hdc, penOuter);
                Rectangle(hdc, 0, 0, bitmap.bmWidth, bitmap.bmHeight);

                SelectObject(hdc, penInner);
                Rectangle(hdc, 1, 1, bitmap.bmWidth - 1, bitmap.bmHeight - 1);
            }
            
            EndPaint(hWnd, &ps);
            return TRUE;
        }
        case WM_DESTROY:
            if (HBITMAP bmp = (HBITMAP)SendMessage(hWnd, STM_GETIMAGE, IMAGE_BITMAP, NULL)) {
                DeleteObject(bmp);
            }
            DeleteObject(penOuter);
            DeleteObject(penInner);
            dialog = nullptr;
            break;
        case WM_NCDESTROY:
            RemoveWindowSubclass(hWnd, AvatarProc, uIdSubclass);
            break;
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

BOOL CALLBACK OptionsDialog::DlgProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
    static OptionsDialog *dialog = nullptr;
    static Plugin *plugin = nullptr;
    static HFONT userNameFont;

    switch (Msg) {
        case WM_INITDIALOG: {
            dialog = (OptionsDialog *)lParam;
            dialog->m_handle = hwnd;
            plugin = dialog->m_plugin;

            SetWindowSubclass(GetDlgItem(hwnd, IDC_CONNECTBTN), ButtonProc, 0, /*OptionsDialog*/lParam);
            SetWindowSubclass(GetDlgItem(hwnd, IDC_MAINFRAME), FrameProc, 0, /*OptionsDialog*/lParam);
            SetWindowSubclass(GetDlgItem(hwnd, IDC_AUTHGROUPBOX), GroupBoxProc, 0, /*OptionsDialog*/lParam);
            SetWindowSubclass(GetDlgItem(hwnd, IDC_GENERALGROUPBOX), GroupBoxProc, 0, /*OptionsDialog*/lParam);
            SetWindowSubclass(GetDlgItem(hwnd, IDC_MONITORGROUPBOX), GroupBoxProc, 0, /*OptionsDialog*/lParam);
            SetWindowSubclass(GetDlgItem(hwnd, IDC_AVATAR), AvatarProc, 0, /*OptionsDialog*/lParam);

            SendDlgItemMessage(hwnd, IDC_LIMITSTREAMVALUESPIN, UDM_SETRANGE32, 1, 50000);
            SendDlgItemMessage(hwnd, IDC_CHECKEVERYVALUESPIN, UDM_SETRANGE32, 1, 720);

            userNameFont = CreateFont(23, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_MODERN, L"Tahoma");
            SendDlgItemMessage(hwnd, IDC_USERNAME, WM_SETFONT, WPARAM(userNameFont), TRUE);
        } break;
        case WM_CTLCOLORSTATIC: {
            DWORD CtrlID = GetDlgCtrlID((HWND)lParam);
            if (CtrlID == IDC_USERINFO)  {
                HDC hdcStatic = (HDC)wParam;
                SetTextColor(hdcStatic, RGB(0x80, 0x80, 0x80));
                SetBkMode(hdcStatic, TRANSPARENT);
                return (INT_PTR)GetStockObject(NULL_BRUSH);
            }
            return FALSE;
        } break;
        case WM_NOTIFY:
            switch (((LPNMHDR)lParam)->code) {
                case UDN_DELTAPOS:
                    dialog->OptionsModified();
                break;
            }
        break;
        case WM_COMMAND: {
            switch (LOWORD(wParam)) {
                case IDC_CONNECTBTN: 
                    if (!plugin->isConnected()) {
                        Connect([] {
                            dialog->OptionsModified();
                            dialog->LoadProfileInfo();
                        });
                    } else {
                        plugin->setAccessToken(L"");
                        dialog->m_userName.clear();
                        dialog->m_userInfo.clear();
                        dialog->m_userId = 0;
                        std::wstring path = Config::PluginConfigFolder() + L"user_avatar.jpg";
                        DeleteFile(path.c_str());

                        dialog->OptionsModified();
                        dialog->UpdateProfileInfo();
                    }
                break;
                case IDC_CHECKEVERYVALUE:
                case IDC_LIMITSTREAMVALUE:
                    if (HIWORD(wParam) == EN_CHANGE) {
                        if (dialog)
                            dialog->OptionsModified();
                    }
                break;
                case IDC_MONITORLIKES:
                case IDC_MONITORSTREAM:
                case IDC_ADDDURATION:
                case IDC_CHECKONSTARTUP:
                case IDC_CHECKEVERY:
                case IDC_LIMITSTREAM:
                    if (HIWORD(wParam) == BN_CLICKED) {
                        dialog->OptionsModified();

                        BOOL enable = SendMessage((HWND)lParam, BM_GETCHECK, 0, 0) == BST_CHECKED;
                        if (LOWORD(wParam) == IDC_LIMITSTREAM) {
                            EnableWindow(GetDlgItem(hwnd, IDC_LIMITSTREAMVALUE), enable);
                            EnableWindow(GetDlgItem(hwnd, IDC_LIMITSTREAMVALUESPIN), enable);
                            EnableWindow(GetDlgItem(hwnd, IDC_TRACKS), enable);
                        } else if (LOWORD(wParam) == IDC_CHECKEVERY) {
                            EnableWindow(GetDlgItem(hwnd, IDC_CHECKEVERYVALUE), enable);
                            EnableWindow(GetDlgItem(hwnd, IDC_CHECKEVERYVALUESPIN), enable);
                            EnableWindow(GetDlgItem(hwnd, IDC_HOURS), enable);
                        }
                    }
                break;
            }
        } break;
        case WM_DESTROY:
            DeleteObject(userNameFont);
        break;
        default: return FALSE;
    }
    return TRUE;
}

void OptionsDialog::Connect(std::function<void()> onFinished) {
    (new TcpServer(38916, [onFinished](TcpServer *s, char *request, std::string &response) -> bool {
        response = "HTTP/1.1 200 OK\r\n"
                   "Content-Type: text/html\r\n"
                   "Connection: close\r\n"
                   "Cache-Control: no-store, no-cache, must-revalidate, post-check=0, pre-check=0\r\n"
                   "Pragma: no-cache\r\n"
                   "Server: AIMPSoundcloud plugin\r\n"
                   "\r\n";

        if (HRSRC hResource = ::FindResource(g_hInst, MAKEINTRESOURCE(IDR_CONNECTRES1), L"CONNECTRESP")) {
            if (DWORD dataSize = ::SizeofResource(g_hInst, hResource)) {
                if (void *pResourceData = ::LockResource(::LoadResource(g_hInst, hResource))) {
                    response += std::string(reinterpret_cast<char *>(pResourceData), dataSize);
                }
            }
        }

        char *token = strstr(request, "code=");
        if (token) {
            token += 5;
            if (char *amp = strstr(token, "&")) *amp = 0;
            if (char *space = strstr(token, " ")) *space = 0;

            DebugA("Access code: %s\n", token);

            std::string postData = "client_id=" CLIENT_ID "&client_secret=" CLIENT_SECRET "&grant_type=authorization_code&redirect_uri=http%3A%2F%2Flocalhost%3A38916%2F&code=";
            postData += token;

            AimpHTTP::Post(L"https://api.soundcloud.com/oauth2/token", postData, [onFinished](unsigned char *data, int size) {
                rapidjson::Document d;
                d.Parse(reinterpret_cast<const char *>(data));

                if (d.HasMember("access_token")) {
                    Plugin::instance()->setAccessToken(Tools::ToWString(d["access_token"].GetString()));
                    Config::SetString(L"AccessToken", Plugin::instance()->getAccessToken());
                    if (onFinished)
                        onFinished();
                }
            });

            Tools::ReplaceString("%TITLE%", "Authorization granted", response);
            Tools::ReplaceString("%TEXT%", "You may now close this browser window and return to AIMP.", response);
            return true;
        }

        Tools::ReplaceString("%TITLE%", "Sorry, an error occurred", response);
        Tools::ReplaceString("%TEXT%", "Couldn't connect to SoundCloud account. Please return to AIMP and try again.", response);
        return true;
    }))->Start();

    ShellExecuteA(Plugin::instance()->GetMainWindowHandle(),
                  "open",
                  "https://soundcloud.com/connect?client_id=" CLIENT_ID "&redirect_uri=http%3A%2F%2Flocalhost%3A38916%2F&response_type=code&scope=non-expiring",
                  NULL, NULL, SW_SHOWNORMAL);
}

BOOL WINAPI OptionsDialog::SelectFirstControl() {
    // TODO
    //PostMessage(m_handle, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(m_handle, IDC_CONNECTBTN), TRUE);
    return false;
}

BOOL WINAPI OptionsDialog::SelectNextControl(BOOL FindForward, BOOL CheckTabStop) {
    // TODO
    //PostMessage(m_handle, WM_NEXTDLGCTL, 0, FALSE); // Next control
    return false;
}
                                            
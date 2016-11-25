#include "OptionsDialog.h"
#include "AIMPString.h"
#include "resource.h"
#include "TcpServer.h"
#include "AimpHTTP.h"
#include "Tools.h"
#include "rapidjson/document.h"
#include "AIMPSoundcloud.h"
#include "ExclusionsDialog.h"
#include <Shellapi.h>

#include <Commctrl.h>
#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "Msimg32.lib")

#include <gdiplus.h>
#include "GdiPlusImageLoader.h"

#define WM_SET_MY_FOCUS WM_USER + 1
#define WM_UPDATESIZE   WM_USER + 2
#define WM_UPDATELOCALE WM_USER + 3
#define WM_SUBCLASSINIT WM_USER + 4

extern HINSTANCE g_hInst;

OptionsDialog::OptionsDialog(Plugin *plugin) : m_userId(0), m_plugin(plugin), m_handle(NULL), m_currentFocusControl(0) {
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
    static AIMPString name(L"SoundCloud");
    name->AddRef();
    *S = name;
    return S_OK;
}

HWND WINAPI OptionsDialog::CreateFrame(HWND ParentWnd) {
    m_handle = CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_SETTINGS), ParentWnd, DlgProc, (LPARAM)this);
    if (m_handle != NULL) {
        RECT R;
        GetWindowRect(ParentWnd, &R);
        OffsetRect(&R, -R.left, -R.top);
        SetWindowPos(m_handle, NULL, R.left, R.top, R.right - R.left, R.bottom - R.top, SWP_SHOWWINDOW);
        Notification(AIMP_SERVICE_OPTIONSDIALOG_NOTIFICATION_LOCALIZATION);
        Notification(AIMP_SERVICE_OPTIONSDIALOG_NOTIFICATION_LOAD);
    }
    return m_handle;
}

void WINAPI OptionsDialog::DestroyFrame() {
    DestroyWindow(m_handle);
    m_handle = NULL;
}

void WINAPI OptionsDialog::Notification(int ID) {
    switch (ID) {
        case AIMP_SERVICE_OPTIONSDIALOG_NOTIFICATION_LOCALIZATION: {
            std::wstring limitUserStreamText0 = m_plugin->Lang(L"SoundCloud.Options\\LimitUserStream", 0);
            std::wstring limitUserStreamText1 = m_plugin->Lang(L"SoundCloud.Options\\LimitUserStream", 1);
            std::wstring checkEveryText0 = m_plugin->Lang(L"SoundCloud.Options\\CheckEvery", 0);
            std::wstring checkEveryText1 = m_plugin->Lang(L"SoundCloud.Options\\CheckEvery", 1);
            SetDlgItemText(m_handle, IDC_MAINFRAME,       m_plugin->Lang(L"SoundCloud.Options\\Title").c_str());
            SetDlgItemText(m_handle, IDC_AUTHGROUPBOX,    m_plugin->Lang(L"SoundCloud.Options\\Account").c_str());
            SetDlgItemText(m_handle, IDC_GENERALGROUPBOX, m_plugin->Lang(L"SoundCloud.Options\\General").c_str());
            SetDlgItemText(m_handle, IDC_ADDDURATION,     m_plugin->Lang(L"SoundCloud.Options\\AddDurationToTitle").c_str());
            SetDlgItemText(m_handle, IDC_ADDARTIST,       m_plugin->Lang(L"SoundCloud.Options\\AddUsernameToTitle").c_str());
            SetDlgItemText(m_handle, IDC_LIMITSTREAM,     limitUserStreamText0.c_str());
            SetDlgItemText(m_handle, IDC_TRACKS,          limitUserStreamText1.c_str());
            SetDlgItemText(m_handle, IDC_MONITORGROUPBOX, m_plugin->Lang(L"SoundCloud.Options\\MonitorURLs").c_str());
            SetDlgItemText(m_handle, IDC_MONITORLIKES,    m_plugin->Lang(L"SoundCloud.Options\\MonitorLikes").c_str());
            SetDlgItemText(m_handle, IDC_MONITORSTREAM,   m_plugin->Lang(L"SoundCloud.Options\\MonitorStream").c_str());
            SetDlgItemText(m_handle, IDC_CHECKONSTARTUP,  m_plugin->Lang(L"SoundCloud.Options\\CheckAtStartup").c_str());
            SetDlgItemText(m_handle, IDC_MANAGEEXCLUSIONS,m_plugin->Lang(L"SoundCloud.Exclusions\\Header").c_str());
            SetDlgItemText(m_handle, IDC_CHECKEVERY,      checkEveryText0.c_str());
            SetDlgItemText(m_handle, IDC_HOURS,           checkEveryText1.c_str());
            SendDlgItemMessage(m_handle, IDC_CONNECTBTN, WM_UPDATELOCALE, 0, 0);

            HDC hdc = GetDC(m_handle);

            auto adjustRect = [&](const std::wstring &text, const std::wstring &text2, UINT cbId, UINT editId, UINT spinId, UINT suffixId) {
                HWND control = GetDlgItem(m_handle, cbId);
                SelectObject(hdc, (HFONT)SendMessage(control, WM_GETFONT, 0, 0));

                SIZE textSize;
                GetTextExtentPoint32(hdc, text.c_str(), text.size(), &textSize);
                LPtoDP(hdc, (POINT *)&textSize, 2);
                int offset = textSize.cx;

                SetWindowPos(control, NULL, 0, 0, offset + 20, 13, SWP_NOZORDER | SWP_NOMOVE);

                RECT rc;
                control = GetDlgItem(m_handle, editId); GetWindowRect(control, &rc); MapWindowPoints(HWND_DESKTOP, m_handle, (LPPOINT)&rc, 2);
                SetWindowPos(control, NULL, offset + 50, rc.top, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

                control = GetDlgItem(m_handle, spinId); GetWindowRect(control, &rc); MapWindowPoints(HWND_DESKTOP, m_handle, (LPPOINT)&rc, 2);
                SetWindowPos(control, NULL, offset + 94, rc.top, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

                GetTextExtentPoint32(hdc, text2.c_str(), text2.size(), &textSize);
                LPtoDP(hdc, (POINT *)&textSize, 2);

                control = GetDlgItem(m_handle, suffixId); GetWindowRect(control, &rc); MapWindowPoints(HWND_DESKTOP, m_handle, (LPPOINT)&rc, 2);
                SetWindowPos(control, NULL, offset + 115, rc.top, textSize.cx, 13, SWP_NOZORDER);
            };

            adjustRect(limitUserStreamText0, limitUserStreamText1, IDC_LIMITSTREAM, IDC_LIMITSTREAMVALUE, IDC_LIMITSTREAMVALUESPIN, IDC_TRACKS);
            adjustRect(checkEveryText0, checkEveryText1, IDC_CHECKEVERY, IDC_CHECKEVERYVALUE, IDC_CHECKEVERYVALUESPIN, IDC_HOURS);

            ReleaseDC(m_handle, hdc);
        } break;
        case AIMP_SERVICE_OPTIONSDIALOG_NOTIFICATION_LOAD: {
            m_userId    = Config::GetInt64(L"UserId");
            m_userName  = Config::GetString(L"UserName");
            m_userLogin = Config::GetString(L"UserLogin");
            m_userInfo  = Config::GetString(L"UserInfo");
            m_plugin->setAccessToken(Config::GetString(L"AccessToken"));

            SendDlgItemMessage(m_handle, IDC_ADDDURATION, BM_SETCHECK, Config::GetInt32(L"AddDurationToTitle", 0), 0);
            SendDlgItemMessage(m_handle, IDC_ADDARTIST, BM_SETCHECK, Config::GetInt32(L"AddUsernameToTitle", 0), 0);
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
            Config::SetInt32(L"AddUsernameToTitle", SendDlgItemMessage(m_handle, IDC_ADDARTIST, BM_GETCHECK, 0, 0) == BST_CHECKED);
            Config::SetInt32(L"LimitUserStream", SendDlgItemMessage(m_handle, IDC_LIMITSTREAM, BM_GETCHECK, 0, 0) == BST_CHECKED);
            Config::SetInt32(L"CheckEveryEnabled", SendDlgItemMessage(m_handle, IDC_CHECKEVERY, BM_GETCHECK, 0, 0) == BST_CHECKED);
            Config::SetInt32(L"CheckOnStartup", SendDlgItemMessage(m_handle, IDC_CHECKONSTARTUP, BM_GETCHECK, 0, 0) == BST_CHECKED);
            Config::SetInt32(L"MonitorLikes", SendDlgItemMessage(m_handle, IDC_MONITORLIKES, BM_GETCHECK, 0, 0) == BST_CHECKED);
            Config::SetInt32(L"MonitorStream", SendDlgItemMessage(m_handle, IDC_MONITORSTREAM, BM_GETCHECK, 0, 0) == BST_CHECKED);

            m_plugin->StartMonitorTimer();
        } break;
    }
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
                // No idea why, but on old Windows XP AIMP had trouble getting https avatar. Replace with http
                std::wstring url(Tools::ToWString(d["avatar_url"]));
                if (url.find(L"https") == 0) 
                    url.replace(0, 5, L"http");

                AimpHTTP::Download(url, Config::PluginConfigFolder() + L"user_avatar.jpg", [this](unsigned char *data, int size) {
                    UpdateProfileInfo();
                    OptionsModified();
                });
            }
        }
    });
}

void OptionsDialog::UpdateProfileInfo() {
    if (m_userId > 0 && m_plugin->isConnected()) {
        SendDlgItemMessage(m_handle, IDC_USERNAME, WM_SETTEXT, 0, (LPARAM)m_userName.c_str());

        std::wstring uinfo = m_userInfo;
        if (!m_userLogin.empty())
            uinfo = m_userLogin + L"\r\n" + m_userInfo;
        SendDlgItemMessage(m_handle, IDC_USERINFO, WM_SETTEXT, 0, (LPARAM)uinfo.c_str());

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
    }
    SendDlgItemMessage(m_handle, IDC_CONNECTBTN, WM_UPDATESIZE, 0, 0);

    RECT rc;
    GetClientRect(m_handle, &rc);
    RedrawWindow(m_handle, &rc, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_ALLCHILDREN);
}

void GetRoundRectPath(Gdiplus::GraphicsPath *pPath, Gdiplus::Rect r, int dia) {
    if (dia > r.Width)  dia = r.Width;
    if (dia > r.Height) dia = r.Height;

    Gdiplus::Rect Corner(r.X, r.Y, dia, dia);
    pPath->Reset();
    pPath->AddArc(Corner, 180, 90);
    if (dia == 20) {
        Corner.Width += 1;
        Corner.Height += 1;
        r.Width -= 1;
        r.Height -= 1;
    }
    Corner.X += (r.Width - dia - 1);
    pPath->AddArc(Corner, 270, 90);
    Corner.Y += (r.Height - dia - 1);
    pPath->AddArc(Corner, 0, 90);
    Corner.X -= (r.Width - dia - 1);
    pPath->AddArc(Corner, 90, 90);
    pPath->CloseFigure();
}

LRESULT CALLBACK OptionsDialog::ButtonProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    using namespace Gdiplus;
    struct PaintData {
        GdiPlusImageLoader Logo;
        std::wstring Text;
        Font Font;
        Pen BorderPen;
        ARGB Gradient[2];
        SolidBrush TextColor;
        Pen LinePen;
        int LogoOffset;
        int Margin;
        int Height;
    };

    static PaintData connect    { { IDB_WHITELOGO,  L"PNG" }, {}, { L"Tahoma", 10, FontStyleBold }, 0xffd05300, { 0xfffb8112, 0xffff3701 }, 0xffffffff, 0xfffe9455, 47, 66, 29 };
    static PaintData disconnect { { IDB_ORANGELOGO, L"PNG" }, {}, { L"Tahoma", 10                }, 0xffbfbfbf, { 0xffffffff, 0xfff1f1f1 }, 0xff333333, 0xffe4e4e4, 37, 56, 25 };

    static PaintData *currentBtn = nullptr;
    static OptionsDialog *dialog = nullptr;
    static bool mouseOver = false;
    static bool customFocus = false;
    static bool connected = false;

    static Rect r;
    static RectF layoutRect;
    static SolidBrush focusBrush(0x11000000);
    static Gdiplus::GraphicsPath borderPath;
    static Gdiplus::StringFormat format;

    switch (uMsg) {
        case WM_SUBCLASSINIT: {
            dialog = (OptionsDialog *)dwRefData;
            format.SetAlignment(StringAlignmentCenter);
            format.SetLineAlignment(StringAlignmentCenter);
        } break;
        case WM_MOUSELEAVE:   mouseOver = false; break;
        case WM_MOUSEMOVE:    mouseOver = true; break;
        case WM_SET_MY_FOCUS: customFocus = (wParam == 1); break;
        case WM_KILLFOCUS:    customFocus = false; break;
        case WM_UPDATELOCALE:
            connect.Text    = Plugin::instance()->Lang(L"SoundCloud.Options\\ConnectButton", 0);
            disconnect.Text = Plugin::instance()->Lang(L"SoundCloud.Options\\ConnectButton", 1);
        break;
        case WM_UPDATESIZE: {
            if (!dialog)
                break;
            HWND parent = GetDlgItem(dialog->m_handle, IDC_AUTHGROUPBOX);
            RECT rc;
            GetClientRect(parent, &rc);
            MapWindowPoints(parent, dialog->m_handle, (LPPOINT)&rc, 2);
            RectF textRect;
            if (connected = dialog->m_plugin->isConnected()) { // Intentional assignment(=) here
                currentBtn = &disconnect;
            } else {
                currentBtn = &connect;
            }
            Gdiplus::Graphics(hWnd).MeasureString(currentBtn->Text.c_str(), currentBtn->Text.size(), &currentBtn->Font, PointF(0, 0), &textRect);
            r.Width = textRect.Width + currentBtn->Margin;
            r.Height = currentBtn->Height;
            if (connected) {
                SetWindowPos(hWnd, NULL, rc.right - r.Width - 10, rc.bottom - r.Height - 10, r.Width, r.Height, SWP_NOZORDER);
            } else {
                SetWindowPos(hWnd, NULL, rc.left + ((rc.right - rc.left) - r.Width) / 2, rc.top + ((rc.bottom - rc.top) - r.Height) / 2, r.Width, r.Height, SWP_NOZORDER);
            }
            
            GetRoundRectPath(&borderPath, r, 8);
            layoutRect = RectF(currentBtn->LogoOffset, 0, r.Width - currentBtn->LogoOffset, r.Height);
        } break;
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            Gdiplus::Graphics g(hdc);

            LinearGradientBrush linGrBrush(Point(0, 0), Point(0, r.Height), currentBtn->Gradient[mouseOver ? 1 : 0], currentBtn->Gradient[mouseOver ? 0 : 1]);
            g.FillPath(&linGrBrush, &borderPath);
            if (customFocus)
                g.FillPath(&focusBrush, &borderPath);

            g.DrawImage(currentBtn->Logo, 7, 7, currentBtn->Logo->GetWidth(), currentBtn->Logo->GetHeight());
            g.DrawLine(&currentBtn->LinePen, currentBtn->LogoOffset, 0, currentBtn->LogoOffset, r.Height);
            g.DrawString(currentBtn->Text.c_str(), currentBtn->Text.size(), &currentBtn->Font, layoutRect, &format, &currentBtn->TextColor);
            g.DrawPath(&currentBtn->BorderPen, &borderPath);

            EndPaint(hWnd, &ps);
            return TRUE;
        }
        case WM_SETCURSOR:
            SetCursor(LoadCursor(NULL, IDC_HAND));
            return TRUE;
        case WM_DESTROY:
            dialog = nullptr;
        break;
        case WM_NCDESTROY:
            RemoveWindowSubclass(hWnd, ButtonProc, uIdSubclass);
        break;
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK OptionsDialog::FrameProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    static HBRUSH bgBrush;
    static HPEN penOuter;
    static HPEN penInner;
    static RECT rect;
    static RECT captionRect;
    static HFONT captionFont;
    static TRIVERTEX gradient[2];
    static wchar_t text[64];

    switch (uMsg) {
        case WM_SUBCLASSINIT: {
            GetWindowText(hWnd, text, 64);

            bgBrush = CreateSolidBrush(RGB(240, 240, 240));
            penOuter = CreatePen(PS_SOLID, 1, RGB(188, 188, 188));
            penInner = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
            captionFont = CreateFont(13, 0, 0, 0, FW_BLACK, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_MODERN, L"Tahoma");
        } break;
        case WM_SIZE:
            GetClientRect(hWnd, &rect);

            captionRect = rect;
            captionRect.bottom = 21;

            gradient[0] = { captionRect.left + 1, captionRect.top + 1, 0xff00, 0x8800, 0x0000, 0xffff }; // #ff8800
            gradient[1] = { captionRect.right - 1, captionRect.bottom, 0xff00, 0x3300, 0x0000, 0xffff }; // #ff3300
            return TRUE;
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
        break;
        case WM_NCDESTROY:
            RemoveWindowSubclass(hWnd, FrameProc, uIdSubclass);
        break;
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK OptionsDialog::GroupBoxProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    static HBRUSH bgBrush;
    static HPEN penOuter;
    static HPEN penInner;
    static HFONT captionFont;

    switch (uMsg) {
        case WM_SUBCLASSINIT:
            bgBrush = CreateSolidBrush(RGB(240, 240, 240));
            penOuter = CreatePen(PS_SOLID, 1, RGB(188, 188, 188));
            penInner = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
            captionFont = CreateFont(13, 0, 0, 0, FW_REGULAR, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_MODERN, L"Tahoma");
        break;
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
            if (GetWindowLong(hWnd, GWL_ID) == IDC_AUTHGROUPBOX) {
                DeleteObject(bgBrush);
                DeleteObject(penOuter);
                DeleteObject(penInner);
                DeleteObject(captionFont);
            }
        break;
        case WM_NCDESTROY:
            RemoveWindowSubclass(hWnd, GroupBoxProc, uIdSubclass);
        break;
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK OptionsDialog::AvatarProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    static HPEN penOuter;
    static HPEN penInner;

    switch (uMsg) {
        case WM_SUBCLASSINIT:
            penOuter = CreatePen(PS_SOLID, 1, RGB(188, 188, 188));
            penInner = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
        break;
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);

            if (HBITMAP bmp = (HBITMAP)SendMessage(hWnd, STM_GETIMAGE, IMAGE_BITMAP, NULL)) {
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
        break;
        case WM_NCDESTROY:
            RemoveWindowSubclass(hWnd, AvatarProc, uIdSubclass);
        break;
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK OptionsDialog::LinkProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    static bool mouseOver = false;
    static bool mouseTracking = false;
    static HFONT versionFont = NULL;

    switch (uMsg) {
        case WM_SUBCLASSINIT: {
            HFONT hOrigFont = (HFONT)SendMessage(hWnd, WM_GETFONT, 0, 0);
            LOGFONT lf;
            GetObject(hOrigFont, sizeof(lf), &lf);
            lf.lfUnderline = TRUE;
            versionFont = CreateFontIndirect(&lf);
            SendMessage(hWnd, WM_SETFONT, WPARAM(versionFont), TRUE);
        } break;
        case WM_USER: return mouseOver;
        case WM_LBUTTONUP: 
            switch (GetWindowLong(hWnd, GWL_ID)) {
                case IDC_VERSION: ShellExecute(hWnd, L"open", L"http://www.aimp.ru/forum/index.php?topic=49938", NULL, NULL, SW_SHOWNORMAL); break;
                case IDC_MANAGEEXCLUSIONS: ExclusionsDialog::Show(GetParent(GetParent(hWnd))); break;
            }
        break;
        case WM_MOUSELEAVE:
            mouseOver = false; 
            InvalidateRect(hWnd, NULL, FALSE);
            mouseTracking = false;
        break;
        case WM_MOUSEMOVE:
            mouseOver = true;
            InvalidateRect(hWnd, NULL, FALSE);
            if (!mouseTracking) {
                TRACKMOUSEEVENT tme;
                tme.cbSize = sizeof(TRACKMOUSEEVENT);
                tme.dwFlags = TME_LEAVE;
                tme.hwndTrack = hWnd;

                if (_TrackMouseEvent(&tme)) {
                    mouseTracking = true;
                }
            }
        break;
        case WM_SETCURSOR:
            SetCursor(LoadCursor(NULL, IDC_HAND));
            return TRUE;
        case WM_DESTROY:
            if (GetWindowLong(hWnd, GWL_ID) == IDC_VERSION) {
                DeleteObject(versionFont);
            }
        break;
        case WM_NCDESTROY:
            RemoveWindowSubclass(hWnd, LinkProc, uIdSubclass);
        break;
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

BOOL CALLBACK OptionsDialog::DlgProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
    static OptionsDialog *dialog = nullptr;
    static Plugin *plugin = nullptr;
    static HFONT userNameFont;
    static HBRUSH bgBrush;

    switch (Msg) {
        case WM_INITDIALOG: {
            dialog = (OptionsDialog *)lParam;
            dialog->m_handle = hwnd;
            plugin = dialog->m_plugin;

            SetWindowSubclass(GetDlgItem(hwnd, IDC_CONNECTBTN),      ButtonProc,   0, /*OptionsDialog*/lParam); SendDlgItemMessage(hwnd, IDC_CONNECTBTN, WM_SUBCLASSINIT, 0, 0);
            SetWindowSubclass(GetDlgItem(hwnd, IDC_MAINFRAME),       FrameProc,    0, /*OptionsDialog*/lParam); SendDlgItemMessage(hwnd, IDC_MAINFRAME, WM_SUBCLASSINIT, 0, 0);
            SetWindowSubclass(GetDlgItem(hwnd, IDC_AUTHGROUPBOX),    GroupBoxProc, 0, /*OptionsDialog*/lParam); SendDlgItemMessage(hwnd, IDC_AUTHGROUPBOX, WM_SUBCLASSINIT, 0, 0);
            SetWindowSubclass(GetDlgItem(hwnd, IDC_GENERALGROUPBOX), GroupBoxProc, 0, /*OptionsDialog*/lParam); SendDlgItemMessage(hwnd, IDC_GENERALGROUPBOX, WM_SUBCLASSINIT, 0, 0);
            SetWindowSubclass(GetDlgItem(hwnd, IDC_MONITORGROUPBOX), GroupBoxProc, 0, /*OptionsDialog*/lParam); SendDlgItemMessage(hwnd, IDC_MONITORGROUPBOX, WM_SUBCLASSINIT, 0, 0);
            SetWindowSubclass(GetDlgItem(hwnd, IDC_AVATAR),          AvatarProc,   0, /*OptionsDialog*/lParam); SendDlgItemMessage(hwnd, IDC_AVATAR, WM_SUBCLASSINIT, 0, 0);
            SetWindowSubclass(GetDlgItem(hwnd, IDC_VERSION),         LinkProc,     0, /*OptionsDialog*/lParam); SendDlgItemMessage(hwnd, IDC_VERSION, WM_SUBCLASSINIT, 0, 0);
            SetWindowSubclass(GetDlgItem(hwnd, IDC_MANAGEEXCLUSIONS),LinkProc,     0, /*OptionsDialog*/lParam); SendDlgItemMessage(hwnd, IDC_MANAGEEXCLUSIONS, WM_SUBCLASSINIT, 0, 0);

            SendDlgItemMessage(hwnd, IDC_LIMITSTREAMVALUESPIN, UDM_SETRANGE32, 1, 50000);
            SendDlgItemMessage(hwnd, IDC_CHECKEVERYVALUESPIN, UDM_SETRANGE32, 1, 720);

            userNameFont = CreateFont(23, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_MODERN, L"Tahoma");
            SendDlgItemMessage(hwnd, IDC_USERNAME, WM_SETFONT, WPARAM(userNameFont), TRUE);
            bgBrush = CreateSolidBrush(RGB(240, 240, 240));
        } break;
        case WM_SIZE: {
            RECT rc, rc2;
            GetClientRect(hwnd, &rc);

            SetWindowPos(GetDlgItem(hwnd, IDC_MAINFRAME), NULL, rc.left, rc.top, rc.right, rc.bottom, SWP_NOZORDER);

            HWND group = GetDlgItem(hwnd, IDC_AUTHGROUPBOX);    GetClientRect(group, &rc2); SetWindowPos(group, NULL, 0, 0, rc.right - 21, rc2.bottom, SWP_NOMOVE | SWP_NOZORDER);
                 group = GetDlgItem(hwnd, IDC_GENERALGROUPBOX); GetClientRect(group, &rc2); SetWindowPos(group, NULL, 0, 0, rc.right - 21, rc2.bottom, SWP_NOMOVE | SWP_NOZORDER);
                 group = GetDlgItem(hwnd, IDC_MONITORGROUPBOX); GetClientRect(group, &rc2); SetWindowPos(group, NULL, 0, 0, rc.right - 21, rc2.bottom, SWP_NOMOVE | SWP_NOZORDER);
            HWND version = GetDlgItem(hwnd, IDC_VERSION);
            GetClientRect(version, &rc2);
            SetWindowPos(version, NULL, rc.right - /*124*/138, rc.bottom - rc2.bottom - 7, /*120*/137, rc2.bottom, SWP_NOZORDER);

            HWND manageLink = GetDlgItem(hwnd, IDC_MANAGEEXCLUSIONS);
            GetClientRect(manageLink, &rc2);
            SetWindowPos(manageLink, NULL, rc.left + 10, rc.bottom - rc2.bottom - 7, 120, rc2.bottom, SWP_NOZORDER);
            return TRUE;
        } break;
        case WM_CTLCOLORSTATIC: {
            DWORD CtrlID = GetDlgCtrlID((HWND)lParam);
            HDC hdcStatic = (HDC)wParam;
            if (CtrlID == IDC_VERSION || CtrlID == IDC_MANAGEEXCLUSIONS) {
                if (SendMessage((HWND)lParam, WM_USER, 0, 0) == 1) {
                    SetTextColor(hdcStatic, RGB(0x34, 0x9b, 0xce));
                } else {
                    SetTextColor(hdcStatic, RGB(0xef, 0x68, 0x00));
                }
            } else if (CtrlID == IDC_USERINFO) {
                SetTextColor(hdcStatic, RGB(0x80, 0x80, 0x80));
            } else {
                SetTextColor(hdcStatic, RGB(0x00, 0x00, 0x00));
            }
            SetBkMode(hdcStatic, TRANSPARENT);
            return (INT_PTR)bgBrush;
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
                case IDC_ADDARTIST:
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
            DeleteObject(bgBrush);
            DeleteObject(userNameFont);
        break;
        default: return FALSE;
    }
    return TRUE;
}

void OptionsDialog::Connect(std::function<void()> onFinished) {
    // For some reason AIMP can't get lang strings from different thread, so preload them here
    std::string s1 = Tools::ToString(Plugin::instance()->Lang(L"SoundCloud\\ConnectStatusOK", 0)),
                s2 = Tools::ToString(Plugin::instance()->Lang(L"SoundCloud\\ConnectStatusOK", 1)),
                s3 = Tools::ToString(Plugin::instance()->Lang(L"SoundCloud\\ConnectStatusError", 0)),
                s4 = Tools::ToString(Plugin::instance()->Lang(L"SoundCloud\\ConnectStatusError", 1));
    
    (new TcpServer(38916, [onFinished, s1, s2, s3, s4](TcpServer *s, char *request, std::string &response) -> bool {
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

            Tools::ReplaceString(std::string("%TITLE%"), s1, response);
            Tools::ReplaceString(std::string("%TEXT%"), s2, response);
            return true;
        }

        Tools::ReplaceString(std::string("%TITLE%"), s3, response);
        Tools::ReplaceString(std::string("%TEXT%"), s4, response);
        return true;
    }))->Start();

    ShellExecuteA(Plugin::instance()->GetMainWindowHandle(),
                  "open",
                  "https://soundcloud.com/connect?client_id=" CLIENT_ID "&redirect_uri=http%3A%2F%2Flocalhost%3A38916%2F&response_type=code&scope=non-expiring",
                  NULL, NULL, SW_SHOWNORMAL);
}

static std::vector<int> s_tabOrder({ IDC_CONNECTBTN,
                                     IDC_ADDDURATION,
                                     IDC_ADDARTIST,
                                     IDC_LIMITSTREAM,
                                     IDC_LIMITSTREAMVALUE,
                                     IDC_MONITORLIKES,
                                     IDC_MONITORSTREAM,
                                     IDC_CHECKONSTARTUP,
                                     IDC_CHECKEVERY,
                                     IDC_CHECKEVERYVALUE
});

BOOL WINAPI OptionsDialog::SelectFirstControl() {
    m_currentFocusControl = 0;
    HWND hWnd = GetDlgItem(m_handle, s_tabOrder[m_currentFocusControl]);
    SetFocus(hWnd);
    SendMessage(hWnd, WM_SET_MY_FOCUS, 1, 0);
    return true;
}

BOOL WINAPI OptionsDialog::SelectNextControl(BOOL FindForward, BOOL CheckTabStop) {
    if (s_tabOrder[m_currentFocusControl] == IDC_CONNECTBTN) {
        SendMessage(GetDlgItem(m_handle, IDC_CONNECTBTN), WM_SET_MY_FOCUS, 0, 0);
    }
    if (FindForward) {
        if (m_currentFocusControl + 1 > s_tabOrder.size() - 1)
            return false;

        ++m_currentFocusControl;
    } else {
        if (m_currentFocusControl <= 0)
            return false;

        --m_currentFocusControl;
    }
    HWND hWnd = GetDlgItem(m_handle, s_tabOrder[m_currentFocusControl]);
    SetFocus(hWnd);
    if (s_tabOrder[m_currentFocusControl] == IDC_CONNECTBTN) {
        SendMessage(hWnd, WM_SET_MY_FOCUS, 1, 0);
    }
    return true;
}

#include "AddURLDialog.h"
#include "resource.h"
#include "Tools.h"
#include "SoundCloudAPI.h"
#include "GdiPlusImageLoader.h"
#include "AIMPSoundcloud.h"

extern HINSTANCE g_hInst;

void AddURLDialog::Show() {
    HWND parent = Plugin::instance()->GetMainWindowHandle();

    HWND handle = CreateDialog(g_hInst, MAKEINTRESOURCE(IDD_ADDURL), parent, DlgProc);
    if (handle != NULL) {
        ShowWindow(handle, SW_SHOW);
    }
}

BOOL CALLBACK AddURLDialog::DlgProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
    switch (Msg) {
        case WM_CLOSE:
            EndDialog(hwnd, 0);
            break;
        case WM_INITDIALOG: {
            SendDlgItemMessage(hwnd, IDC_CREATENEW, BM_SETCHECK, BST_CHECKED, NULL);
            GdiPlusImageLoader icon(IDB_ICON, L"PNG");
            HICON bitmap;
            icon->GetHICON(&bitmap);
            SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)bitmap);
        } break;
        case WM_COMMAND: {
            switch (LOWORD(wParam)) {
                case IDOK: {
                    wchar_t buf[1024];
                    GetDlgItemText(hwnd, IDC_SOUNDCLOUDURL, buf, 1024);
                    std::wstring url(buf);

                    GetDlgItemText(hwnd, IDC_PLAYLISTTITLE, buf, 1024);
                    std::wstring playlistTitle(buf);

                    bool createnew = SendDlgItemMessage(hwnd, IDC_CREATENEW, BM_GETCHECK, NULL, NULL) == BST_CHECKED;

                    SoundCloudAPI::ResolveUrl(url, playlistTitle, createnew);
                    EndDialog(hwnd, 0);
                } break;
                case IDC_CREATENEW:
                    if (HIWORD(wParam) == BN_CLICKED) {
                        LRESULT chkState = SendMessage((HWND)lParam, BM_GETCHECK, 0, 0);
                        BOOL enable = chkState == BST_CHECKED;

                        EnableWindow(GetDlgItem(hwnd, IDC_PLAYLISTTITLE), enable);
                        EnableWindow(GetDlgItem(hwnd, IDC_PLAYLISTTITLECAPTION), enable);
                    }
                break;
            }
        } break;
        default: return FALSE;
    }
    return TRUE;
}

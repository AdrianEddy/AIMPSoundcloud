#pragma once

#include <windows.h>
#include <functional>
#include <cinttypes>
#include "IUnknownInterfaceImpl.h"
#include "SDK/apiOptions.h"

class Plugin;

class OptionsDialog : public IUnknownInterfaceImpl<IAIMPOptionsDialogFrame> {
public:
    OptionsDialog(Plugin *plugin);
    ~OptionsDialog();

    virtual HRESULT WINAPI QueryInterface(REFIID riid, LPVOID* ppvObj) {
        if (!ppvObj) return E_POINTER;

        if (riid == IID_IAIMPOptionsDialogFrame) {
            *ppvObj = this;
            AddRef();
            return S_OK;
        }
        /*if (riid == IID_IAIMPOptionsDialogFrameKeyboardHelper) {
            *ppvObj = this;
            AddRef();
            return S_OK;
        }*/

        return E_NOINTERFACE;
    }

    HRESULT WINAPI GetName(IAIMPString **S);

    HWND WINAPI CreateFrame(HWND ParentWnd);
    void WINAPI DestroyFrame();

    void WINAPI Notification(int ID);

    void LoadProfileInfo();
    void UpdateProfileInfo();

    void OptionsModified();

public:
    virtual BOOL WINAPI DialogChar(WCHAR CharCode, int Unused) { return false; }
    virtual BOOL WINAPI DialogKey(WORD CharCode, int Unused) { return false; }
    virtual BOOL WINAPI SelectFirstControl();
    virtual BOOL WINAPI SelectNextControl(BOOL FindForward, BOOL CheckTabStop);

private:
    int64_t m_userId;
    std::wstring m_userName;
    std::wstring m_userLogin;
    std::wstring m_userInfo;

    HWND m_handle;
    Plugin *m_plugin;

    IAIMPServiceOptionsDialog *m_dialogService;

    static BOOL CALLBACK DlgProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam);

    static LRESULT CALLBACK ButtonProc  (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
    static LRESULT CALLBACK FrameProc   (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
    static LRESULT CALLBACK GroupBoxProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
    static LRESULT CALLBACK AvatarProc  (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
};
#pragma once

#include <windows.h>
#include <functional>
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

        return E_NOINTERFACE;
    }

    HRESULT WINAPI GetName(IAIMPString **S);

    HWND WINAPI CreateFrame(HWND ParentWnd);
    void WINAPI DestroyFrame();

    void WINAPI Notification(int ID);

    void LoadProfileInfo();
    void UpdateProfileInfo();

private:
    HWND m_handle;
    Plugin *m_plugin;

    static BOOL CALLBACK DlgProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK ButtonProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
};
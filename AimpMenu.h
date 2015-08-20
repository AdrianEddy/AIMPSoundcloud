#pragma once

#include "SDK/apiCore.h"
#include "SDK/apiMenu.h"
#include "IUnknownInterfaceImpl.h"
#include <functional>

class AimpMenu {
    typedef std::function<void()> CallbackFunc;

    class ClickHandler : public IUnknownInterfaceImpl<IAIMPActionEvent> {
    public:
        ClickHandler(CallbackFunc callback) : m_callback(callback) { }

        virtual HRESULT WINAPI QueryInterface(REFIID riid, LPVOID* ppvObj) {
            if (!ppvObj) return E_POINTER;

            if (riid == IID_IAIMPActionEvent) {
                *ppvObj = this;
                AddRef();
                return S_OK;
            }

            return E_NOINTERFACE;
        }

        virtual void WINAPI OnExecute(IUnknown *Data) { m_callback(); }

    private:
        CallbackFunc m_callback;
    };
public:
    AimpMenu(IAIMPMenuItem *item);

    IAIMPMenuItem *Add(const std::wstring &name, CallbackFunc action, UINT icon = 0);

    static AimpMenu *Get(int id);

    static bool Init(IAIMPCore *Core);

private:
    AimpMenu(const AimpMenu&);
    AimpMenu& operator=(const AimpMenu&);

    IAIMPMenuItem *m_menuItem;

    static IAIMPCore *m_core;
    static IAIMPServiceMenuManager *m_menuManager;
};

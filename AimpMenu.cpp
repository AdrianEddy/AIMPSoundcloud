#include "AimpMenu.h"

#include "AIMPString.h"
#include "SDK/apiMenu.h"
#include <functional>

IAIMPCore *AimpMenu::m_core = nullptr;
IAIMPServiceMenuManager *AimpMenu::m_menuManager = nullptr;

extern HINSTANCE g_hInst;

AimpMenu::AimpMenu(IAIMPMenuItem *item) : m_menuItem(item) {

}

AimpMenu::~AimpMenu() {
    if (m_menuItem)
        m_menuItem->Release();
}

IAIMPMenuItem *AimpMenu::Add(const std::wstring &name, CallbackFunc action, UINT icon, CallbackFunc showAction) {
    IAIMPMenuItem *newItem = nullptr;
    if (SUCCEEDED(m_core->CreateObject(IID_IAIMPMenuItem, reinterpret_cast<void **>(&newItem)))) {
        newItem->SetValueAsObject(AIMP_MENUITEM_PROPID_PARENT, m_menuItem);
        newItem->SetValueAsObject(AIMP_MENUITEM_PROPID_ID, new AIMPString(L"AIMPSoundcloud" + name));

        if (action) {
            IAIMPAction *newAction = nullptr;
            if (SUCCEEDED(m_core->CreateObject(IID_IAIMPAction, reinterpret_cast<void **>(&newAction)))) {
                newAction->SetValueAsObject(AIMP_ACTION_PROPID_ID, new AIMPString(L"AIMPSoundcloudAction" + name));
                newAction->SetValueAsObject(AIMP_ACTION_PROPID_GROUPNAME, new AIMPString(L"SoundCloud"));
                newAction->SetValueAsObject(AIMP_ACTION_PROPID_NAME, new AIMPString(name));
                newAction->SetValueAsObject(AIMP_ACTION_PROPID_EVENT, new ClickHandler(action, newItem));
                newAction->SetValueAsInt32(AIMP_ACTION_PROPID_ENABLED, true);

                m_core->RegisterExtension(IID_IAIMPServiceActionManager, newAction);
                newItem->SetValueAsObject(AIMP_MENUITEM_PROPID_ACTION, newAction);

                newAction->Release();
            }
        } else {
            newItem->SetValueAsObject(AIMP_MENUITEM_PROPID_ID, new AIMPString(L"AIMPSoundcloud" + name));
            newItem->SetValueAsObject(AIMP_MENUITEM_PROPID_NAME, new AIMPString(name));
            newItem->SetValueAsInt32(AIMP_MENUITEM_PROPID_ENABLED, true);
        }

        if (showAction) {
            newItem->SetValueAsObject(AIMP_MENUITEM_PROPID_EVENT_ONSHOW, new ClickHandler(showAction, newItem));
        }

        if (icon > 0) {
            SetIcon(newItem, icon);
        }

        newItem->SetValueAsInt32(AIMP_MENUITEM_PROPID_STYLE, AIMP_MENUITEM_STYLE_NORMAL);
        newItem->SetValueAsInt32(AIMP_MENUITEM_PROPID_VISIBLE, true);

        m_core->RegisterExtension(IID_IAIMPServiceMenuManager, newItem);
        return newItem;
    }
    return nullptr;
}


bool AimpMenu::SetIcon(IAIMPMenuItem *item, UINT icon) {
    bool ok = false;
    if (icon <= 0) {
        item->SetValueAsObject(AIMP_MENUITEM_PROPID_GLYPH, nullptr);
        return true;
    }

    IAIMPImage *img = nullptr;
    if (SUCCEEDED(m_core->CreateObject(IID_IAIMPImage, reinterpret_cast<void **>(&img)))) {
        IAIMPStream *imgStream = nullptr;
        if (SUCCEEDED(m_core->CreateObject(IID_IAIMPMemoryStream, reinterpret_cast<void **>(&imgStream)))) {
            if (HRSRC hResource = ::FindResource(g_hInst, MAKEINTRESOURCE(icon), L"PNG")) {
                if (DWORD imageSize = ::SizeofResource(g_hInst, hResource)) {
                    if (void *pResourceData = ::LockResource(::LoadResource(g_hInst, hResource))) {
                        unsigned int written = 0;
                        if (SUCCEEDED(imgStream->Write(reinterpret_cast<unsigned char *>(pResourceData), imageSize, &written)) && written > 0) {
                            if (SUCCEEDED(img->LoadFromStream(imgStream))) {
                                item->SetValueAsObject(AIMP_MENUITEM_PROPID_GLYPH, img);
                                ok = true;
                            }
                            imgStream->Release();
                        }
                    }
                }
            }
        }
        img->Release();
    }
    return ok;
}

AimpMenu *AimpMenu::Get(int id) {
    IAIMPMenuItem *menuItem = nullptr;
    if (SUCCEEDED(m_menuManager->GetBuiltIn(id, &menuItem))) {
        return new AimpMenu(menuItem);
    }
    return nullptr;
}

bool AimpMenu::Init(IAIMPCore *Core) {
    m_core = Core;

    return SUCCEEDED(m_core->QueryInterface(IID_IAIMPServiceMenuManager, reinterpret_cast<void **>(&m_menuManager)));
}

void AimpMenu::Deinit() {
    if (m_menuManager)
        m_menuManager->Release();
}
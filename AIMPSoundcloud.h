#pragma once

#include "SDK/apiPlugin.h"
#include "SDK/apiPlaylists.h"
#include "OptionsDialog.h"
#include "AimpHTTP.h"
#include "Config.h"
#include "SoundCloudAPI.h"
#include "AimpMenu.h"
#include "resource.h"
#include "Tools.h"
#include "AddURLDialog.h"

#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")

extern HINSTANCE g_hInst;

static const wchar_t Name[] = L"Soundcloud player";
static const wchar_t Author[] = L"Eddy";
static const wchar_t ShortDesc[] = L"Provides ability to play Soundcloud tracks within AIMP";

class AIMPSoundcloudPlugin : public IUnknownInterfaceImpl<IAIMPPlugin> {
    friend class OptionsDialog;

public:
    AIMPSoundcloudPlugin() : m_core(nullptr) {

    }

    PWCHAR WINAPI InfoGet(int Index) {
        switch (Index) {
            case AIMP_PLUGIN_INFO_NAME:              return const_cast<PWCHAR>(Name);
            case AIMP_PLUGIN_INFO_AUTHOR:            return const_cast<PWCHAR>(Author);
            case AIMP_PLUGIN_INFO_SHORT_DESCRIPTION: return const_cast<PWCHAR>(ShortDesc);
        }
        return nullptr;
    }

    DWORD WINAPI InfoGetCategories() {
        return AIMP_PLUGIN_CATEGORY_ADDONS;
    }
    
    HRESULT WINAPI Initialize(IAIMPCore *Core) {
        Gdiplus::GdiplusStartupInput gdiplusStartupInput;
        Gdiplus::GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL);

        m_core = Core;
        Core->RegisterExtension(IID_IAIMPServiceOptionsDialog, new OptionsDialog(this));

               Config::Init(Core);
             AimpHTTP::Init(Core);
             AimpMenu::Init(Core);
        SoundCloudAPI::Init(this);

        AimpMenu *addMenu = new AimpMenu(AimpMenu::Get(AIMP_MENUID_PLAYER_PLAYLIST_ADDING)
                                                 ->Add(L"SoundCloud", nullptr, IDB_ICON));

        addMenu->Add(L"Add URL", AddURLDialog::Show);
        addMenu->Add(L"Add recommendations for current song", [] { DebugA("Clicked\n"); });

        AimpMenu *playlistMenu = AimpMenu::Get(AIMP_MENUID_PLAYER_PLAYLIST_MANAGE);
        playlistMenu->Add(L"SoundCloud likes", SoundCloudAPI::LoadLikes, IDB_ICON);
        playlistMenu->Add(L"SoundCloud stream", SoundCloudAPI::LoadStream, IDB_ICON);
        
        m_accessToken = Config::GetString(L"AccessToken");

        if (FAILED(m_core->QueryInterface(IID_IAIMPServicePlaylistManager, reinterpret_cast<void**>(&m_playlistManager))))
            return E_FAIL;

        return S_OK;
    }

    HRESULT WINAPI Finalize() {
        Gdiplus::GdiplusShutdown(m_gdiplusToken);
        return S_OK;
    }

    void WINAPI SystemNotification(int NotifyID, IUnknown* Data) {

    }

    IAIMPPlaylist *GetPlaylist(const std::wstring &playlistName, bool activate = true);
    IAIMPPlaylist *GetCurrentPlaylist();

    inline std::wstring getAccessToken() const { return m_accessToken; }
    inline void setAccessToken(const std::wstring &accessToken) { m_accessToken = accessToken; }

    inline bool isConnected() const { return !m_accessToken.empty(); }

    inline IAIMPCore *core() const { return m_core; }

private:
    IAIMPServicePlaylistManager *m_playlistManager;

    ULONG_PTR m_gdiplusToken;
    std::wstring m_accessToken;
    IAIMPCore *m_core;
};
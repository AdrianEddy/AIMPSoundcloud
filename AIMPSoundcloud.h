#pragma once

#include "SDK/apiPlugin.h"
#include "SDK/apiPlaylists.h"
#include "SDK/apiMessages.h"
#include "OptionsDialog.h"
#include "AimpHTTP.h"
#include "Config.h"
#include "SoundCloudAPI.h"
#include "AimpMenu.h"
#include "resource.h"
#include "Tools.h"
#include "AddURLDialog.h"
#include "MessageHook.h"
#include <vector>

#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")

extern HINSTANCE g_hInst;

class IAIMPFileInfo;

static const wchar_t Name[] = L"Soundcloud player";
static const wchar_t Author[] = L"Eddy";
static const wchar_t ShortDesc[] = L"Provides ability to play Soundcloud tracks within AIMP";

class AIMPSoundcloudPlugin : public IUnknownInterfaceImpl<IAIMPPlugin> {
    friend class OptionsDialog;
    friend class MessageHook;

public:
    AIMPSoundcloudPlugin() : m_playlistManager(nullptr), m_messageDispatcher(nullptr), m_gdiplusToken(0), m_core(nullptr) {

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

        if (!  Config::Init(Core)) return E_FAIL;
        if (!AimpHTTP::Init(Core)) return E_FAIL;
        if (!AimpMenu::Init(Core)) return E_FAIL;
        SoundCloudAPI::Init(this);

        Config::LoadExtendedConfig();

        m_accessToken = Config::GetString(L"AccessToken");

        AimpMenu *addMenu = AimpMenu::Get(AIMP_MENUID_PLAYER_PLAYLIST_ADDING);
        addMenu->Add(L"SoundCloud URL", AddURLDialog::Show, IDB_ICON)->Release();
        delete addMenu;

        AimpMenu *playlistMenu = AimpMenu::Get(AIMP_MENUID_PLAYER_PLAYLIST_MANAGE);
        playlistMenu->Add(L"SoundCloud stream", SoundCloudAPI::LoadStream, IDB_ICON)->Release();
        playlistMenu->Add(L"SoundCloud likes", SoundCloudAPI::LoadLikes, IDB_ICON)->Release();
        delete playlistMenu;

        AimpMenu *contextMenu = AimpMenu::Get(AIMP_MENUID_PLAYER_PLAYLIST_CONTEXT_FUNCTIONS);
        AimpMenu *recommendations = new AimpMenu(contextMenu->Add(L"Load recommendations", nullptr, IDB_ICON));
        recommendations->Add(L"Here", [this] {
            ForSelectedTracks([](IAIMPPlaylist *pl, IAIMPPlaylistItem *item, int64_t id) -> int {
                if (id > 0) {
                    SoundCloudAPI::LoadRecommendations(id, false);
                }
                return 0;
            });
        })->Release();
        recommendations->Add(L"Create new playlist", [this] {
            ForSelectedTracks([](IAIMPPlaylist *pl, IAIMPPlaylistItem *item, int64_t id) -> int {
                if (id > 0) {
                    SoundCloudAPI::LoadRecommendations(id, true);
                }
                return 0;
            });
        })->Release();
        delete recommendations;

        contextMenu->Add(L"Add to exclusions", [this] {
            ForSelectedTracks([] (IAIMPPlaylist *pl, IAIMPPlaylistItem *item, int64_t id) -> int {
                if (id > 0) {
                    Config::TrackExclusions.insert(id);
                    return FLAG_DELETE_ITEM;
                }
                return 0;
            });
            Config::SaveExtendedConfig();
        }, IDB_ICON)->Release();

        contextMenu->Add(L"Unlike", [this] {
            ForSelectedTracks([](IAIMPPlaylist *pl, IAIMPPlaylistItem *item, int64_t id) -> int {
                if (id > 0) {
                    SoundCloudAPI::UnlikeSong(id);
                }
                return 0;
            });
        }, IDB_ICON)->Release();

        contextMenu->Add(L"Like", [this] {
            ForSelectedTracks([](IAIMPPlaylist *pl, IAIMPPlaylistItem *item, int64_t id) -> int {
                if (id > 0) {
                    SoundCloudAPI::LikeSong(id);
                }
                return 0;
            });
            // TODO: should wait for result
            SoundCloudAPI::LoadLikes();
        }, IDB_ICON)->Release();
        delete contextMenu;

        if (FAILED(m_core->QueryInterface(IID_IAIMPServicePlaylistManager, reinterpret_cast<void**>(&m_playlistManager))))
            return E_FAIL;

        if (FAILED(m_core->QueryInterface(IID_IAIMPServiceMessageDispatcher, reinterpret_cast<void**>(&m_messageDispatcher))))
            return E_FAIL;

        m_messageHook = new MessageHook(this);
        m_messageDispatcher->Hook(m_messageHook);

        return S_OK;
    }

    HRESULT WINAPI Finalize() {
        m_messageDispatcher->Unhook(m_messageHook);
        m_messageDispatcher->Release();
        m_playlistManager->Release();

        AimpMenu::Deinit();
        AimpHTTP::Deinit();
        Config::Deinit();

        Gdiplus::GdiplusShutdown(m_gdiplusToken);
        return S_OK;
    }

    void WINAPI SystemNotification(int NotifyID, IUnknown *Data) {

    }

    IAIMPPlaylist *GetPlaylist(const std::wstring &playlistName, bool activate = true);
    IAIMPPlaylist *GetCurrentPlaylist();
    IAIMPPlaylist *UpdatePlaylistGrouping(IAIMPPlaylist *pl);
    IAIMPPlaylistItem *GetCurrentTrack();

    enum CallbackFlags {
        FLAG_DELETE_ITEM = 0x01,
        FLAG_STOP_LOOP   = 0x02,
    };

    void ForSelectedTracks(std::function<int(IAIMPPlaylist *, IAIMPPlaylistItem *, int64_t)>);
    void ForEveryItem(IAIMPPlaylist *pl, std::function<int(IAIMPPlaylistItem *, IAIMPFileInfo *, int64_t)> callback);

    inline std::wstring getAccessToken() const { return m_accessToken; }
    inline void setAccessToken(const std::wstring &accessToken) { m_accessToken = accessToken; }

    inline bool isConnected() const { return !m_accessToken.empty(); }

    inline IAIMPCore *core() const { return m_core; }

private:
    MessageHook *m_messageHook;
    IAIMPServicePlaylistManager *m_playlistManager;
    IAIMPServiceMessageDispatcher *m_messageDispatcher;

    ULONG_PTR m_gdiplusToken;
    std::wstring m_accessToken;
    IAIMPCore *m_core;
};
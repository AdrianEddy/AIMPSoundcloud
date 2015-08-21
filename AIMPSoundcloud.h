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

class Plugin : public IUnknownInterfaceImpl<IAIMPPlugin> {
    friend class OptionsDialog;
    friend class MessageHook;

public:
    static Plugin *instance() {
        if (!m_instance) 
            m_instance = new Plugin();

        return m_instance;
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
    
    HRESULT WINAPI Initialize(IAIMPCore *Core);
    HRESULT WINAPI Finalize();

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

    HWND GetMainWindowHandle();

    inline std::wstring getAccessToken() const { return m_accessToken; }
    inline void setAccessToken(const std::wstring &accessToken) { m_accessToken = accessToken; }

    inline bool isConnected() const { return !m_accessToken.empty(); }

    inline IAIMPCore *core() const { return m_core; }

private:
    Plugin() : m_playlistManager(nullptr), m_messageDispatcher(nullptr), m_gdiplusToken(0), m_core(nullptr) {
        AddRef();
    }
    Plugin(const Plugin &);
    Plugin &operator=(const Plugin &);

    static Plugin *m_instance;

    MessageHook *m_messageHook;
    IAIMPServicePlaylistManager *m_playlistManager;
    IAIMPServiceMessageDispatcher *m_messageDispatcher;

    ULONG_PTR m_gdiplusToken;
    std::wstring m_accessToken;
    IAIMPCore *m_core;
};
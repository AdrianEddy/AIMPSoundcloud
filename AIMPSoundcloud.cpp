#include "AIMPSoundcloud.h"

#include "AIMPString.h"
#include "SDK/apiPlayer.h"
#include "Timer.h"
#include "PlaylistListener.h"
#include "AddURLDialog.h"
#include "Tools.h"
#include "AimpHTTP.h"
#include "SoundCloudAPI.h"
#include "AimpMenu.h"
#include "resource.h"
#include <set>

HRESULT __declspec(dllexport) WINAPI AIMPPluginGetHeader(IAIMPPlugin **Header) {
    *Header = Plugin::instance();
    return S_OK;
}

Plugin *Plugin::m_instance = nullptr;

HRESULT WINAPI Plugin::Initialize(IAIMPCore *Core) {
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    if (Gdiplus::GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL) != Gdiplus::Status::Ok)
        return E_FAIL;

    m_core = Core;
    AIMPString::Init(Core);

    if (FAILED(m_core->QueryInterface(IID_IAIMPServiceMUI, reinterpret_cast<void **>(&m_muiService))))
        return E_FAIL;

      if (!Config::Init(Core)) return E_FAIL;
    if (!AimpHTTP::Init(Core)) return E_FAIL;
    if (!AimpMenu::Init(Core)) return E_FAIL;

    Config::LoadExtendedConfig();

    m_accessToken = Config::GetString(L"AccessToken");

    if (AimpMenu *addMenu = AimpMenu::Get(AIMP_MENUID_PLAYER_PLAYLIST_ADDING)) {
        addMenu->Add(Lang(L"SoundCloud.Menu\\AddURL"), [this](IAIMPMenuItem *) { AddURLDialog::Show(); }, IDB_ICON)->Release();
        delete addMenu;
    }

    if (AimpMenu *playlistMenu = AimpMenu::Get(AIMP_MENUID_PLAYER_PLAYLIST_MANAGE)) {
        playlistMenu->Add(Lang(L"SoundCloud.Menu\\MyTracksAndPlaylists"), [this](IAIMPMenuItem *) {
            SoundCloudAPI::LoadMyTracksAndPlaylists();
        }, IDB_ICON, [this](IAIMPMenuItem *item) {
            item->SetValueAsInt32(AIMP_MENUITEM_PROPID_VISIBLE, isConnected());
        })->Release();
        playlistMenu->Add(Lang(L"SoundCloud.Menu\\MyStream"), [this](IAIMPMenuItem *) {
            if (!isConnected()) {
                OptionsDialog::Connect(SoundCloudAPI::LoadStream);
                return;
            }
            SoundCloudAPI::LoadStream();
        }, IDB_ICON)->Release();
        playlistMenu->Add(Lang(L"SoundCloud.Menu\\MyLikes"), [this](IAIMPMenuItem *) {
            if (!isConnected()) {
                OptionsDialog::Connect(SoundCloudAPI::LoadLikes);
                return;
            }
            SoundCloudAPI::LoadLikes();
        }, IDB_ICON)->Release();
        delete playlistMenu;
    }

    auto enableIfValid = [this](IAIMPMenuItem *item) {
        int valid = 0;
        ForSelectedTracks([&valid](IAIMPPlaylist *, IAIMPPlaylistItem *, int64_t id) -> int {
            if (id > 0) valid++;
            return 0;
        });

        item->SetValueAsInt32(AIMP_MENUITEM_PROPID_VISIBLE, valid > 0);
    };

    if (AimpMenu *contextMenu = AimpMenu::Get(AIMP_MENUID_PLAYER_PLAYLIST_CONTEXT_FUNCTIONS)) {
        AimpMenu *recommendations = new AimpMenu(contextMenu->Add(Lang(L"SoundCloud.Menu\\LoadRecommendations", 0), nullptr, IDB_ICON, enableIfValid));
        recommendations->Add(Lang(L"SoundCloud.Menu\\LoadRecommendations", 1), [this](IAIMPMenuItem *) {
            ForSelectedTracks([](IAIMPPlaylist *pl, IAIMPPlaylistItem *item, int64_t id) -> int {
                if (id > 0) {
                    SoundCloudAPI::LoadRecommendations(id, false, item);
                }
                return 0;
            });
        })->Release();
        recommendations->Add(Lang(L"SoundCloud.Menu\\LoadRecommendations", 2), [this](IAIMPMenuItem *) {
            ForSelectedTracks([](IAIMPPlaylist *pl, IAIMPPlaylistItem *item, int64_t id) -> int {
                if (id > 0) {
                    SoundCloudAPI::LoadRecommendations(id, true, item);
                }
                return 0;
            });
        })->Release();
        delete recommendations;

        contextMenu->Add(Lang(L"SoundCloud.Menu\\OpenInBrowser"), [this](IAIMPMenuItem *) {
            ForSelectedTracks([this](IAIMPPlaylist *, IAIMPPlaylistItem *, int64_t id) -> int {
                if (id > 0) {
                    wchar_t url[256];
                    wsprintf(url, L"https://api.soundcloud.com/tracks/%ld?client_id=" TEXT(CLIENT_ID) L"&oauth_token=", id);
                    AimpHTTP::Get(url + m_accessToken, [this](unsigned char *data, int size) {
                        rapidjson::Document d;
                        d.Parse(reinterpret_cast<const char *>(data));

                        if (d.IsObject() && d.HasMember("permalink_url")) {
                            ShellExecuteA(GetMainWindowHandle(), "open", d["permalink_url"].GetString(), NULL, NULL, SW_SHOWNORMAL);
                        }
                    });
                }
                return 0;
            });
        }, IDB_ICON, enableIfValid)->Release();

        contextMenu->Add(Lang(L"SoundCloud.Menu\\AddToExclusions"), [this](IAIMPMenuItem *) {
            ForSelectedTracks([](IAIMPPlaylist *, IAIMPPlaylistItem *, int64_t id) -> int {
                if (id > 0) {
                    Config::TrackExclusions.insert(id);
                    return FLAG_DELETE_ITEM;
                }
                return 0;
            });
            Config::SaveExtendedConfig();
        }, IDB_ICON, enableIfValid)->Release();

        contextMenu->Add(Lang(L"SoundCloud.Menu\\LikeUnlike", 0), [this](IAIMPMenuItem *) {
            ForSelectedTracks([](IAIMPPlaylist *, IAIMPPlaylistItem *, int64_t id) -> int {
                if (id > 0) {
                    if (Config::Likes.find(id) != Config::Likes.end()) {
                        SoundCloudAPI::UnlikeSong(id);
                        Config::Likes.erase(id);
                    } else {
                        SoundCloudAPI::LikeSong(id);
                        Config::Likes.insert(id);
                    }
                }
                return 0;
            });
            Timer::SingleShot(1000, SoundCloudAPI::LoadLikes);
            Config::SaveExtendedConfig();
        }, IDB_ICON, [this](IAIMPMenuItem *item) {
            int likes = 0;
            int unlikes = 0;
            int valid = 0;
            ForSelectedTracks([&](IAIMPPlaylist *, IAIMPPlaylistItem *, int64_t id) -> int {
                if (id > 0) {
                    valid++;
                    if (Config::Likes.find(id) != Config::Likes.end()) {
                        unlikes++;
                    } else {
                        likes++;
                    }
                }
                return 0;
            });

            if (valid == 0) {
                item->SetValueAsInt32(AIMP_MENUITEM_PROPID_VISIBLE, false);
                return;
            }
            item->SetValueAsInt32(AIMP_MENUITEM_PROPID_VISIBLE, true);

            if (unlikes == 0) {
                item->SetValueAsObject(AIMP_MENUITEM_PROPID_NAME, AIMPString(Lang(L"SoundCloud.Menu\\LikeUnlike", 1))); // Like
            } else if (likes == 0) {
                item->SetValueAsObject(AIMP_MENUITEM_PROPID_NAME, AIMPString(Lang(L"SoundCloud.Menu\\LikeUnlike", 2))); // Unlike
            } else {
                item->SetValueAsObject(AIMP_MENUITEM_PROPID_NAME, AIMPString(Lang(L"SoundCloud.Menu\\LikeUnlike", 0))); // Like / Unlike
            }
        })->Release();
        delete contextMenu;
    }

    if (FAILED(m_core->QueryInterface(IID_IAIMPServicePlaylistManager, reinterpret_cast<void **>(&m_playlistManager))))
        return E_FAIL;

    if (FAILED(m_core->QueryInterface(IID_IAIMPServiceMessageDispatcher, reinterpret_cast<void **>(&m_messageDispatcher))))
        return E_FAIL;

    m_messageHook = new MessageHook(this);
    if (FAILED(m_messageDispatcher->Hook(m_messageHook))) {
        delete m_messageHook;
        return E_FAIL;
    }

    if (FAILED(m_core->RegisterExtension(IID_IAIMPServiceOptionsDialog, static_cast<OptionsDialog::Base *>(new OptionsDialog(this)))))
        return E_FAIL;
    
    if (FAILED(m_core->RegisterExtension(IID_IAIMPServicePlaylistManager, new PlaylistListener())))
        return E_FAIL;
     
    if (Config::GetInt32(L"CheckOnStartup", 1)) {
        Timer::SingleShot(2000, MonitorCallback);
    }

    StartMonitorTimer();

    return S_OK;
}

void Plugin::StartMonitorTimer() {
    bool enabled = Config::GetInt32(L"CheckEveryEnabled", 1) == 1;
    if (m_monitorTimer > 0 && enabled)
        return;

    KillMonitorTimer();
    if (enabled) {
        m_monitorTimer = Timer::Schedule(Config::GetInt32(L"CheckEveryHours", 1) * 60 * 60 * 1000, MonitorCallback);
    }
}

void Plugin::KillMonitorTimer() {
    if (m_monitorTimer > 0) {
        Timer::Cancel(m_monitorTimer);
        m_monitorTimer = 0;
    }
}

void Plugin::MonitorCallback() {
    if (m_instance->m_monitorPendingUrls.empty()) {
        for (const auto &x : Config::MonitorUrls) {
            m_instance->m_monitorPendingUrls.push(x);
        }
        if (m_instance->isConnected() && Config::GetInt32(L"MonitorLikes", 1)) {
            if (IAIMPPlaylist *pl = m_instance->GetPlaylist(m_instance->Lang(L"SoundCloud\\Likes", 0), false)) {
                std::wstring playlistId = m_instance->PlaylistId(pl);
                pl->Release();
                if (!playlistId.empty()) {
                    std::wstring refName = Config::GetString(L"UserName") + m_instance->Lang(L"SoundCloud\\Likes", 1);

                    m_instance->m_monitorPendingUrls.push({ L"https://api.soundcloud.com/me/favorites?limit=200", playlistId, SoundCloudAPI::LoadingState::LoadingLikes, refName });
                }
            }
        }
        if (m_instance->isConnected() && Config::GetInt32(L"MonitorStream", 1)) {
            if (IAIMPPlaylist *pl = m_instance->GetPlaylist(m_instance->Lang(L"SoundCloud\\Stream", 0), false)) {
                std::wstring playlistId = m_instance->PlaylistId(pl);
                pl->Release();
                if (!playlistId.empty()) {
                    std::wstring refName = Config::GetString(L"UserName") + m_instance->Lang(L"SoundCloud\\Stream", 1);
                    const int flags = SoundCloudAPI::LoadingState::IgnoreExistingPosition | SoundCloudAPI::LoadingState::IgnoreNextPage;

                    m_instance->m_monitorPendingUrls.push({ L"https://api.soundcloud.com/me/activities?limit=300", playlistId, flags, refName });
                }
            }
        }
    } 
    if (!m_instance->m_monitorPendingUrls.empty()) {
        const Config::MonitorUrl &url = m_instance->m_monitorPendingUrls.front();
        IAIMPPlaylist *pl = m_instance->GetPlaylistById(url.PlaylistID, false);
        if (!pl) {
            m_instance->m_monitorPendingUrls.pop();
            if (!m_instance->m_monitorPendingUrls.empty())
                MonitorCallback();
            return;
        }

        SoundCloudAPI::LoadingState *state = new SoundCloudAPI::LoadingState();
        state->ReferenceName = url.GroupName;
        state->Flags = url.Flags;

        SoundCloudAPI::GetExistingTrackIds(pl, state);
        if (m_instance->m_monitorPendingUrls.size() > 1) {
            SoundCloudAPI::LoadFromUrl(url.URL, pl, state, MonitorCallback);
        } else {
            SoundCloudAPI::LoadFromUrl(url.URL, pl, state); // last one without callback
        }
        m_instance->m_monitorPendingUrls.pop();
    }
}

HRESULT WINAPI Plugin::Finalize() {
    Timer::Cancel(m_monitorTimer);
    m_messageDispatcher->Unhook(m_messageHook);
    m_messageDispatcher->Release();
    m_muiService->Release();
    m_playlistManager->Release();

    AimpMenu::Deinit();
    AimpHTTP::Deinit();
      Config::Deinit();

    Gdiplus::GdiplusShutdown(m_gdiplusToken);
    return S_OK;
}

IAIMPPlaylist *Plugin::GetPlaylistById(const std::wstring &playlistId, bool activate) {
    IAIMPPlaylist *playlistPointer = nullptr;
    if (SUCCEEDED(m_playlistManager->GetLoadedPlaylistByID(AIMPString(playlistId), &playlistPointer)) && playlistPointer) {
        if (activate)
            m_playlistManager->SetActivePlaylist(playlistPointer);

        return UpdatePlaylistGrouping(playlistPointer);
    }

    return nullptr;
}


IAIMPPlaylist *Plugin::GetPlaylist(const std::wstring &playlistName, bool activate, bool create) {
    IAIMPPlaylist *playlistPointer = nullptr;
    if (SUCCEEDED(m_playlistManager->GetLoadedPlaylistByName(AIMPString(playlistName), &playlistPointer)) && playlistPointer) {
        if (activate)
            m_playlistManager->SetActivePlaylist(playlistPointer);

        return UpdatePlaylistGrouping(playlistPointer);
    } else if (create) {
        if (SUCCEEDED(m_playlistManager->CreatePlaylist(AIMPString(playlistName), activate, &playlistPointer)))
            return UpdatePlaylistGrouping(playlistPointer);
    }

    return nullptr;
}

IAIMPPlaylist *Plugin::GetCurrentPlaylist() {
    IAIMPPlaylist *pl = nullptr;
    if (SUCCEEDED(m_playlistManager->GetActivePlaylist(&pl))) {
        return pl;
    }
    return nullptr;
}

IAIMPPlaylist *Plugin::UpdatePlaylistGrouping(IAIMPPlaylist *pl) {
    if (!pl)
        return nullptr;

    // Change playlist grouping template to %A (album)
    IAIMPPropertyList *plProp = nullptr;
    if (SUCCEEDED(pl->QueryInterface(IID_IAIMPPropertyList, reinterpret_cast<void **>(&plProp)))) {
        plProp->SetValueAsInt32(AIMP_PLAYLIST_PROPID_GROUPPING_OVERRIDEN, 1);
        plProp->SetValueAsObject(AIMP_PLAYLIST_PROPID_GROUPPING_TEMPLATE, AIMPString(L"%A"));
        plProp->Release();
    }
    return pl;
}

std::wstring Plugin::PlaylistId(IAIMPPlaylist *pl) {
    std::wstring playlistId;
    IAIMPPropertyList *plProp = nullptr;
    if (SUCCEEDED(pl->QueryInterface(IID_IAIMPPropertyList, reinterpret_cast<void **>(&plProp)))) {
        IAIMPString *id = nullptr;
        if (SUCCEEDED(plProp->GetValueAsObject(AIMP_PLAYLIST_PROPID_ID, IID_IAIMPString, reinterpret_cast<void **>(&id)))) {
            playlistId = id->GetData();
            id->Release();
        }
        plProp->Release();
    }
    return playlistId;
}

IAIMPPlaylistItem *Plugin::GetCurrentTrack() {
    IAIMPServicePlayer *player = nullptr;
    if (SUCCEEDED(m_core->QueryInterface(IID_IAIMPServicePlayer, reinterpret_cast<void **>(&player)))) {
        IAIMPPlaylistItem *item = nullptr;
        if (SUCCEEDED(player->GetPlaylistItem(&item))) {
            player->Release();
            return item;
        }
        player->Release();
    }
    return nullptr;
}

void Plugin::ForSelectedTracks(std::function<int(IAIMPPlaylist *, IAIMPPlaylistItem *, int64_t)> callback) {
    if (!callback)
        return;

    if (IAIMPPlaylist *pl = GetCurrentPlaylist()) {
        pl->BeginUpdate();
        std::set<IAIMPPlaylistItem *> to_del;
        auto delPending = [&] {
            for (auto x : to_del) {
                pl->Delete(x);
                x->Release();
            }
        };
        for (int i = 0, n = pl->GetItemCount(); i < n; ++i) {
            IAIMPPlaylistItem *item = nullptr;
            if (SUCCEEDED(pl->GetItem(i, IID_IAIMPPlaylistItem, reinterpret_cast<void **>(&item)))) {
                int isSelected = 0;
                if (SUCCEEDED(item->GetValueAsInt32(AIMP_PLAYLISTITEM_PROPID_SELECTED, &isSelected))) {
                    if (isSelected) {
                        IAIMPString *url = nullptr;
                        if (SUCCEEDED(item->GetValueAsObject(AIMP_PLAYLISTITEM_PROPID_FILENAME, IID_IAIMPString, reinterpret_cast<void **>(&url)))) {
                            int64_t id = Tools::TrackIdFromUrl(url->GetData());
                            url->Release();
                            
                            int result = callback(pl, item, id);
                            if (result & FLAG_DELETE_ITEM) {
                                to_del.insert(item);
                                if (result & FLAG_STOP_LOOP) {
                                    delPending();
                                    pl->EndUpdate();
                                    pl->Release();
                                    return;
                                }
                                continue;
                            }
                            if (result & FLAG_STOP_LOOP) {
                                delPending();
                                item->Release();
                                pl->EndUpdate();
                                pl->Release();
                                return;
                            }
                        }
                    }
                }
                item->Release();
            }
        }
        delPending();
        pl->EndUpdate();
        pl->Release();
    }
}

void Plugin::ForEveryItem(IAIMPPlaylist *pl, std::function<int(IAIMPPlaylistItem *, IAIMPFileInfo *, int64_t)> callback) {
    if (!pl || !callback)
        return;

    pl->BeginUpdate();
    for (int i = 0, n = pl->GetItemCount(); i < n; ++i) {
        IAIMPPlaylistItem *item = nullptr;
        if (SUCCEEDED(pl->GetItem(i, IID_IAIMPPlaylistItem, reinterpret_cast<void **>(&item)))) {
            IAIMPFileInfo *finfo = nullptr;
            if (SUCCEEDED(item->GetValueAsObject(AIMP_PLAYLISTITEM_PROPID_FILEINFO, IID_IAIMPFileInfo, reinterpret_cast<void **>(&finfo)))) {
                IAIMPString *custom = nullptr;
                if (SUCCEEDED(finfo->GetValueAsObject(AIMP_FILEINFO_PROPID_FILENAME, IID_IAIMPString, reinterpret_cast<void **>(&custom)))) {
                    std::wstring url(custom->GetData());
                    custom->Release();

                    int64_t id = Tools::TrackIdFromUrl(url);
                    int result = callback(item, finfo, id);
                    if (result & FLAG_DELETE_ITEM) {
                        pl->Delete(item);
                        finfo->Release();
                        item->Release();
                        i--;
                        n--;
                        if (result & FLAG_STOP_LOOP) {
                            pl->EndUpdate();
                            return;
                        }
                        continue;
                    }
                    if (result & FLAG_STOP_LOOP) {
                        finfo->Release();
                        item->Release();
                        pl->EndUpdate();
                        return;
                    }
                }
                finfo->Release();
            }
            item->Release();
        }
    }
    pl->EndUpdate();
}

HWND Plugin::GetMainWindowHandle() {
    HWND handle = NULL;
    if (SUCCEEDED(m_messageDispatcher->Send(AIMP_MSG_PROPERTY_HWND, AIMP_MSG_PROPVALUE_GET, &handle))) {
        return handle;
    }
    return NULL;
}

std::wstring Plugin::Lang(const std::wstring &key, int part) {
    std::wstring ret;
    if (!m_muiService)
        return ret;

    IAIMPString *value = nullptr;
    if (part > -1) {
        if (SUCCEEDED(m_muiService->GetValuePart(AIMPString(key), part, &value))) {
            ret = value->GetData();
            value->Release();
        }
    } else {
        if (SUCCEEDED(m_muiService->GetValue(AIMPString(key), &value))) {
            ret = value->GetData();
            value->Release();
        }
    }
    return ret;
}

#include "AIMPSoundcloud.h"

#include "AIMPString.h"
#include "SDK/apiPlayer.h"

HRESULT __declspec(dllexport) WINAPI AIMPPluginGetHeader(IAIMPPlugin **Header) {
    *Header = new AIMPSoundcloudPlugin();
    (*Header)->AddRef();
    return S_OK;
}

IAIMPPlaylist *AIMPSoundcloudPlugin::GetPlaylist(const std::wstring &playlistName, bool activate) {
    AIMPString plName(playlistName);
    plName.AddRef();

    IAIMPPlaylist *playlistPointer = nullptr;
    if (SUCCEEDED(m_playlistManager->GetLoadedPlaylistByName(&plName, &playlistPointer)) && playlistPointer) {
        if (activate)
            m_playlistManager->SetActivePlaylist(playlistPointer);

        return UpdatePlaylistGrouping(playlistPointer);
    } else {
        if (SUCCEEDED(m_playlistManager->CreatePlaylist(&plName, activate, &playlistPointer)))
            return UpdatePlaylistGrouping(playlistPointer);
    }

    return nullptr;
}

IAIMPPlaylist *AIMPSoundcloudPlugin::GetCurrentPlaylist() {
    IAIMPPlaylist *pl = nullptr;
    if (SUCCEEDED(m_playlistManager->GetActivePlaylist(&pl))) {
        return pl;
    }
    return nullptr;
}

IAIMPPlaylist *AIMPSoundcloudPlugin::UpdatePlaylistGrouping(IAIMPPlaylist *pl) {
    if (!pl)
        return nullptr;

    // Change playlist grouping template
    IAIMPPropertyList *plProp = nullptr;
    if (SUCCEEDED(pl->QueryInterface(IID_IAIMPPropertyList, reinterpret_cast<void **>(&plProp)))) {
        plProp->SetValueAsInt32(AIMP_PLAYLIST_PROPID_GROUPPING_OVERRIDEN, 1);
        plProp->SetValueAsObject(AIMP_PLAYLIST_PROPID_GROUPPING_TEMPLATE, new AIMPString(L"%A"));
        plProp->Release();
    }
    return pl;
}

IAIMPPlaylistItem *AIMPSoundcloudPlugin::GetCurrentTrack() {
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

void AIMPSoundcloudPlugin::ForSelectedTracks(std::function<int(IAIMPPlaylist *, IAIMPPlaylistItem *, int64_t)> callback) {
    if (!callback)
        return;

    if (IAIMPPlaylist *pl = GetCurrentPlaylist()) {
        pl->BeginUpdate();
        std::set<IAIMPPlaylistItem *> to_del;
        auto delPending = [&] {
            for (auto x : to_del) {
                pl->Delete(x);
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
                                    return;
                                }
                                continue;
                            }
                            if (result & FLAG_STOP_LOOP) {
                                item->Release();
                                pl->EndUpdate();
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

void AIMPSoundcloudPlugin::ForEveryItem(IAIMPPlaylist *pl, std::function<int(IAIMPPlaylistItem *, IAIMPFileInfo *, int64_t)> callback) {
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

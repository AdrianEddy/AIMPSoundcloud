#include "SoundCloudAPI.h"

#include "AIMPSoundcloud.h"
#include "AimpHTTP.h"
#include "SDK/apiFileManager.h"
#include "SDK/apiPlaylists.h"
#include "AIMPString.h"
#include "Tools.h"
#include <Strsafe.h>
#include <string>

void SoundCloudAPI::AddFromJson(IAIMPPlaylist *playlist, const rapidjson::Value &d, LoadingState *state) {
    if (!playlist)
        return;

    int insertAt = 0; 
    if (state) {
        insertAt = state->InsertPos;
        if (insertAt >= 0)
            insertAt += state->AdditionalPos;
    }

    IAIMPFileInfo *file_info = nullptr;
    if (Plugin::instance()->core()->CreateObject(IID_IAIMPFileInfo, reinterpret_cast<void **>(&file_info)) == S_OK) {
        auto processItem = [&](const rapidjson::Value &item) {
            if (!item.HasMember("id")) {
                DebugA("NOT adding %s\n ", item["title"].GetString());
                return;
            }

            int64_t trackId = item["id"].GetInt64();
            if (state->TrackIds.find(trackId) != state->TrackIds.end()) {
                // Already added earlier
                if (insertAt >= 0) {
                    insertAt++;
                    if (state) {
                        state->InsertPos++;
                        if (state->UpdateAdditionalPos)
                            state->AdditionalPos++;
                    }
                }
                return;
            }

            if (Config::TrackExclusions.find(trackId) != Config::TrackExclusions.end())
                return; // Track excluded

            state->TrackIds.insert(trackId);

            if (state && strcmp(item["kind"].GetString(), "playlist") == 0) {
                DebugA("Adding pending playlist %s\n", item["title"].GetString());
                std::wstring url = Tools::ToWString(item["tracks_uri"]) + L"?limit=200";
                std::wstring title = Tools::ToWString(item["user"]["username"]) + L" - " + Tools::ToWString(item["title"]);
                state->PendingPlaylists.push({ title, url, playlist->GetItemCount() });
                return;
            }

            if (!item.HasMember("stream_url")) {
                wchar_t stream_url[256];
                wsprintf(stream_url, L"https://api.soundcloud.com/tracks/%ld/stream?client_id=" TEXT(STREAM_CLIENT_ID), trackId);

                file_info->SetValueAsObject(AIMP_FILEINFO_PROPID_FILENAME, new AIMPString(stream_url));
            } else {
                file_info->SetValueAsObject(AIMP_FILEINFO_PROPID_FILENAME, new AIMPString(Tools::ToWString(item["stream_url"]) + L"?client_id=" TEXT(STREAM_CLIENT_ID)));
            }

            if (!state->ReferenceName.empty()) {
                file_info->SetValueAsObject(AIMP_FILEINFO_PROPID_ALBUM, new AIMPString(state->ReferenceName));
            }

            file_info->SetValueAsObject(AIMP_FILEINFO_PROPID_ARTIST, new AIMPString(Tools::ToWString(item["user"]["username"])));
            file_info->SetValueAsObject(AIMP_FILEINFO_PROPID_TITLE, new AIMPString(Tools::ToWString(item["title"])));
            file_info->SetValueAsObject(AIMP_FILEINFO_PROPID_URL, new AIMPString(Tools::ToWString(item["permalink_url"])));
            if (item.HasMember("genre"))
                file_info->SetValueAsObject(AIMP_FILEINFO_PROPID_GENRE, new AIMPString(Tools::ToWString(item["genre"])));

            file_info->SetValueAsFloat(AIMP_FILEINFO_PROPID_DURATION, item["duration"].GetInt64() / 1000.0);

            const DWORD flags = AIMP_PLAYLIST_ADD_FLAGS_FILEINFO | AIMP_PLAYLIST_ADD_FLAGS_NOCHECKFORMAT | AIMP_PLAYLIST_ADD_FLAGS_NOEXPAND | AIMP_PLAYLIST_ADD_FLAGS_NOASYNC;
            if (SUCCEEDED(playlist->Add(file_info, flags, insertAt))) {
                if (insertAt >= 0) {
                    insertAt++;
                    if (state) {
                        state->InsertPos++;
                        if (state->UpdateAdditionalPos)
                            state->AdditionalPos++;
                    }
                }
            }

            DebugA("Adding %s\n ", item["title"].GetString());
        };

        if (d.IsArray()) {
            for (auto x = d.Begin(), e = d.End(); x != e; x++) {
                const rapidjson::Value *px = &(*x);
                if (!px->IsObject())
                    continue;

                if (px->HasMember("origin"))
                    px = &((*px)["origin"]);

                processItem(*px);
            }
        } else if (d.IsObject()) {
            processItem(d);
        }
        file_info->Release();
    }
}

void SoundCloudAPI::LoadFromUrl(const std::wstring &url, IAIMPPlaylist *playlist, LoadingState *state) {
    if (!playlist || !state)
        return;

    AimpHTTP::Get(url + L"&client_id=" TEXT(CLIENT_ID) L"&oauth_token=" + Plugin::instance()->getAccessToken(), [playlist, state](unsigned char *data, int size) {
        playlist->BeginUpdate();
        rapidjson::Document d;
        d.Parse(reinterpret_cast<const char *>(data));

        if (d.IsObject() && d.HasMember("collection")) {
            AddFromJson(playlist, d["collection"], state);
        } else {
            AddFromJson(playlist, d, state);
        }

        bool hasAnyPending = false;

        if (d.IsObject() && d.HasMember("next_href")) {
            LoadFromUrl(Tools::ToWString(d["next_href"]), playlist, state);
            hasAnyPending = true;
        } else {
            if (!state->PendingPlaylists.empty()) {
                const LoadingState::PendingPlaylist &pl = state->PendingPlaylists.front();
                state->InsertPos = pl.PlaylistPosition;
                state->UpdateAdditionalPos = true;
                state->ReferenceName = pl.Title;
                LoadFromUrl(pl.Url, playlist, state);
                state->PendingPlaylists.pop();

                hasAnyPending = true;
            }
        }

        playlist->EndUpdate();

        if (!hasAnyPending) {
            // Finished
            playlist->Release();
            delete state;
        }
    });
}

void SoundCloudAPI::LoadLikes() {
    if (!Plugin::instance()->isConnected())
        return;

    IAIMPPlaylist *pl = Plugin::instance()->GetPlaylist(L"Soundcloud - Likes");

    LoadingState *state = new LoadingState();
    state->ReferenceName = Config::GetString(L"UserName") + L"'s likes";
    GetExistingTrackIds(pl, state);

    // TODO: load all likes, not only 200
    LoadFromUrl(L"https://api.soundcloud.com/me/favorites?limit=200", pl, state);
}

void SoundCloudAPI::LoadStream() {
    if (!Plugin::instance()->isConnected())
        return;

    IAIMPPlaylist *pl = Plugin::instance()->GetPlaylist(L"Soundcloud - Stream");

    LoadingState *state = new LoadingState();
    state->ReferenceName = Config::GetString(L"UserName") + L"'s stream";
    GetExistingTrackIds(pl, state);

    LoadFromUrl(L"https://api.soundcloud.com/me/activities?limit=300", pl, state);
}

void SoundCloudAPI::GetExistingTrackIds(IAIMPPlaylist *pl, LoadingState *state) {
    if (!pl || !state)
        return;

    // Fetch current track ids from playlist
    Plugin::instance()->ForEveryItem(pl, [&](IAIMPPlaylistItem *item, IAIMPFileInfo *info, int64_t id) -> int {
        if (id > 0) {
            state->TrackIds.insert(id);
        }
        return 0;
    });
}

void SoundCloudAPI::ResolveUrl(const std::wstring &url, const std::wstring &playlistTitle, bool createPlaylist) {
    if (url.find(L"/stream") != std::wstring::npos) {
        LoadStream();
        return;
    }

    AimpHTTP::Get(L"https://api.soundcloud.com/resolve?url=" + Tools::UrlEncode(url) + L"&client_id=" TEXT(CLIENT_ID) L"&oauth_token=" + Plugin::instance()->getAccessToken(), [createPlaylist, url, playlistTitle](unsigned char *data, int size) {
        rapidjson::Document d;
        d.Parse(reinterpret_cast<const char *>(data));

        if ((d.IsObject() && d.HasMember("kind")) || d.IsArray()) {
            wchar_t finalUrl[256];
            rapidjson::Value *addDirectly = nullptr;
            std::wstring plName;
            bool monitor = true;

            if (d.IsArray()) {
                plName = url;
                addDirectly = &d;
            } else {
                if (strcmp(d["kind"].GetString(), "user") == 0) {
                    plName = L"Soundcloud - " + Tools::ToWString(d["username"]);
                    wsprintf(finalUrl, L"https://api.soundcloud.com/users/%ld/tracks?client_id=" TEXT(CLIENT_ID), d["id"].GetInt64());

                    // TODO: load playlists
                    // wsprintf(finalUrl, L"https://api.soundcloud.com/users/%ld/playlists?client_id=" TEXT(CLIENT_ID), d["id"].GetInt64());

                } else if (strcmp(d["kind"].GetString(), "track") == 0) {
                    if (url.find(L"/recommended") != std::wstring::npos) {
                        plName = L"Recommended for " + Tools::ToWString(d["title"]);
                        wsprintf(finalUrl, L"https://api.soundcloud.com/tracks/%ld/related?client_id=" TEXT(CLIENT_ID), d["id"].GetInt64());
                    } else {
                        plName = Tools::ToWString(d["title"]);
                        addDirectly = &d;
                        monitor = false;
                    }
                } else if (strcmp(d["kind"].GetString(), "playlist") == 0 && d.HasMember("tracks")) {
                    plName = Tools::ToWString(d["user"]["username"]) + L" - " + Tools::ToWString(d["title"]);
                    addDirectly = &(d["tracks"]);

                    wsprintf(finalUrl, L"https://api.soundcloud.com/playlists/%ld?client_id=" TEXT(CLIENT_ID), d["id"].GetInt64());
                }
            }

            IAIMPPlaylist *pl = nullptr;
            std::wstring finalPlaylistName;
            if (createPlaylist) {
                finalPlaylistName = playlistTitle.empty() ? plName : playlistTitle;
                pl = Plugin::instance()->GetPlaylist(finalPlaylistName);
                if (!pl)
                    return;
            } else {
                pl = Plugin::instance()->GetCurrentPlaylist();
                if (!pl)
                    return;

                IAIMPPropertyList *plProp = nullptr;
                if (SUCCEEDED(pl->QueryInterface(IID_IAIMPPropertyList, reinterpret_cast<void **>(&plProp)))) {
                    IAIMPString *name = nullptr;
                    if (SUCCEEDED(plProp->GetValueAsObject(AIMP_PLAYLIST_PROPID_NAME, IID_IAIMPString, reinterpret_cast<void **>(&name)))) {
                        finalPlaylistName = name->GetData();
                        name->Release();
                    }
                    plProp->Release();
                }
            }

            LoadingState *state = new LoadingState();
            state->ReferenceName = plName;

            GetExistingTrackIds(pl, state);
            
            if (addDirectly) {
                AddFromJson(pl, *addDirectly, state);
                delete state;
            } else {
                LoadFromUrl(finalUrl, pl, state);
            }

            if (monitor) {
                Config::MonitorUrls.insert({ finalUrl, finalPlaylistName, plName });
                Config::SaveExtendedConfig();
            }
        }
    });
}

void SoundCloudAPI::LikeSong(int64_t trackId) {
    std::wstring url(L"https://api.soundcloud.com/me/favorites/");
    url += std::to_wstring(trackId);
    url += L"?client_id=" TEXT(CLIENT_ID)
           L"&oauth_token=" + Plugin::instance()->getAccessToken();

    AimpHTTP::Put(url, [](unsigned char *data, int size) {
        rapidjson::Document d;
        d.Parse(reinterpret_cast<const char *>(data));

        if (d.IsObject() && d.HasMember("status")) {
            DebugA("Like result %s\n", d["status"].GetString());
        }
    });
}

void SoundCloudAPI::UnlikeSong(int64_t trackId) {
    std::wstring url(L"https://api.soundcloud.com/me/favorites/");
    url += std::to_wstring(trackId);
    url += L"?client_id=" TEXT(CLIENT_ID)
           L"&oauth_token=" + Plugin::instance()->getAccessToken();

    AimpHTTP::Delete(url, [trackId](unsigned char *data, int size) {
        rapidjson::Document d;
        d.Parse(reinterpret_cast<const char *>(data));

        if (d.IsObject() && d.HasMember("status")) {
            DebugA("Unlike result %s\n", d["status"].GetString());

            if (strcmp(d["status"].GetString(), "200 - OK") == 0) {
                
            }
        }
    });
    IAIMPPlaylist *likes = Plugin::instance()->GetPlaylist(L"Soundcloud - Likes");
    Plugin::instance()->ForEveryItem(likes, [&](IAIMPPlaylistItem *item, IAIMPFileInfo *info, int64_t id) -> int {
        if (id == trackId) {
            return Plugin::FLAG_DELETE_ITEM | Plugin::FLAG_STOP_LOOP;
        }
        return 0;
    });
    likes->Release();
}

void SoundCloudAPI::LoadRecommendations(int64_t trackId, bool createPlaylist) {
    // TODO
}

#include "SoundCloudAPI.h"

#include "AIMPSoundcloud.h"
#include "AimpHTTP.h"
#include "SDK/apiFileManager.h"
#include "SDK/apiPlaylists.h"
#include "AIMPString.h"
#include "Tools.h"
#include <Strsafe.h>
#include <string>
#include <set>

void SoundCloudAPI::AddFromJson(IAIMPPlaylist *playlist, const rapidjson::Value &d, LoadingState *state) {
    if (!playlist || !state)
        return;

    int insertAt = state->InsertPos;
    if (insertAt >= 0)
        insertAt += state->AdditionalPos;

    IAIMPFileInfo *file_info = nullptr;
    if (Plugin::instance()->core()->CreateObject(IID_IAIMPFileInfo, reinterpret_cast<void **>(&file_info)) == S_OK) {
        auto processItem = [&](const rapidjson::Value &item) {
            if (!item.HasMember("id")) {
                return;
            }

            int64_t trackId = item["id"].GetInt64();
            if (state->TrackIds.find(trackId) != state->TrackIds.end()) {
                // Already added earlier
                if (insertAt >= 0 && !(state->Flags & LoadingState::IgnoreExistingPosition)) {
                    insertAt++;
                    state->InsertPos++;
                    if (state->Flags & LoadingState::UpdateAdditionalPos)
                        state->AdditionalPos++;
                }
                return;
            }

            if (Config::TrackExclusions.find(trackId) != Config::TrackExclusions.end())
                return; // Track excluded

            state->TrackIds.insert(trackId);
            if (state->Flags & LoadingState::LoadingLikes) {
                Config::Likes.insert(trackId);
            }

            if (strcmp(item["kind"].GetString(), "playlist") == 0) {
                DebugA("Adding pending playlist %s\n", item["title"].GetString());
                std::wstring url = Tools::ToWString(item["tracks_uri"]);
                std::wstring title = Tools::ToWString(item["user"]["username"]) + L" - " + Tools::ToWString(item["title"]);
                state->PendingUrls.push({ title, url, playlist->GetItemCount() });
                return;
            }

            std::wstring stream_url;
            if (item.HasMember("stream_url")) 
                stream_url = Tools::ToWString(item["stream_url"]);

            if (stream_url.empty())
                stream_url = L"https://api.soundcloud.com/tracks/" + std::to_wstring(trackId) + L"/stream";

            stream_url += L"?client_id=" TEXT(STREAM_CLIENT_ID);
            file_info->SetValueAsObject(AIMP_FILEINFO_PROPID_FILENAME, new AIMPString(stream_url));

            if (!state->ReferenceName.empty()) {
                file_info->SetValueAsObject(AIMP_FILEINFO_PROPID_ALBUM, new AIMPString(state->ReferenceName));
            }

            file_info->SetValueAsObject(AIMP_FILEINFO_PROPID_ARTIST, new AIMPString(Tools::ToWString(item["user"]["username"])));
            std::wstring title(Tools::ToWString(item["title"]));
            if (Config::GetInt32(L"AddDurationToTitle", 0)) {
                double duration = item["duration"].GetInt64() / 1000.0;
                wchar_t buf[128];
                unsigned int hours = floor(duration / 3600.0);
                if (hours > 0) {
                    swprintf_s(buf, L" (%d:%02d:%02d)", hours, (uint32_t)floor(fmod(duration, 3600.0) / 60.0), (uint32_t)floor(fmod(duration, 60.0)));
                } else {
                    swprintf_s(buf, L" (%d:%02d)", (uint32_t)floor(fmod(duration, 3600.0) / 60.0), (uint32_t)floor(fmod(duration, 60.0)));
                }
                title += buf;
            }
            file_info->SetValueAsObject(AIMP_FILEINFO_PROPID_TITLE, new AIMPString(title));
            file_info->SetValueAsObject(AIMP_FILEINFO_PROPID_URL, new AIMPString(Tools::ToWString(item["permalink_url"])));
            if (item.HasMember("genre"))
                file_info->SetValueAsObject(AIMP_FILEINFO_PROPID_GENRE, new AIMPString(Tools::ToWString(item["genre"])));

            file_info->SetValueAsFloat(AIMP_FILEINFO_PROPID_DURATION, item["duration"].GetInt64() / 1000.0);

            const DWORD flags = AIMP_PLAYLIST_ADD_FLAGS_FILEINFO | AIMP_PLAYLIST_ADD_FLAGS_NOCHECKFORMAT | AIMP_PLAYLIST_ADD_FLAGS_NOEXPAND | AIMP_PLAYLIST_ADD_FLAGS_NOASYNC;
            if (SUCCEEDED(playlist->Add(file_info, flags, insertAt))) {
                state->AddedItems++;
                if (insertAt >= 0) {
                    insertAt++;
                    state->InsertPos++;
                    if (state->Flags & LoadingState::UpdateAdditionalPos)
                        state->AdditionalPos++;
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

                // Playlists
                if (px->HasMember("kind") && strcmp((*px)["kind"].GetString(), "playlist") == 0 && px->HasMember("tracks") && (*px)["tracks"].IsArray() && (*px)["tracks"].Size() > 0) {
                    std::wstring oldName = state->ReferenceName;
                    state->ReferenceName = Tools::ToWString((*px)["user"]["username"]) + L" - " + Tools::ToWString((*px)["title"]);
                    for (auto xx = (*px)["tracks"].Begin(), ee = (*px)["tracks"].End(); xx != ee; xx++) {
                        processItem(*xx);
                    }
                    state->ReferenceName = oldName;
                    continue;
                }

                processItem(*px);
            }
        } else if (d.IsObject()) {
            processItem(d);
        }
        file_info->Release();
    }
}

void SoundCloudAPI::LoadFromUrl(std::wstring url, IAIMPPlaylist *playlist, LoadingState *state, std::function<void()> finishCallback) {
    if (!playlist || !state)
        return;

    if (url.find(L'?') == std::wstring::npos) {
        url += L'?';
    } else {
        url += L'&';
    }
    url += L"client_id=" TEXT(CLIENT_ID) L"&oauth_token=" + Plugin::instance()->getAccessToken();

    AimpHTTP::Get(url, [playlist, state, finishCallback](unsigned char *data, int size) {
        rapidjson::Document d;
        d.Parse(reinterpret_cast<const char *>(data));

        playlist->BeginUpdate();
        if (d.IsObject() && d.HasMember("collection")) {
            AddFromJson(playlist, d["collection"], state);
        } else {
            AddFromJson(playlist, d, state);
        }
        playlist->EndUpdate();

        bool processNextPage = d.IsObject() && d.HasMember("next_href");

        if ((state->Flags & LoadingState::IgnoreNextPage) ||
            (Config::GetInt32(L"LimitUserStream", 0) && state->AddedItems >= Config::GetInt32(L"LimitUserStreamValue", 5000))) {
            processNextPage = false;
        }

        if (processNextPage) {
            LoadFromUrl(Tools::ToWString(d["next_href"]), playlist, state, finishCallback);
        } else if (state->Flags & LoadingState::LoadingLikes && d.IsArray() && d.Size() > 0) {
            state->Offset += 200;
            LoadFromUrl(L"https://api.soundcloud.com/me/favorites?limit=200&offset=" + std::to_wstring(state->Offset), playlist, state, finishCallback);
        } else if (!state->PendingUrls.empty()) {
            const LoadingState::PendingUrl &pl = state->PendingUrls.front();
            if (!pl.Title.empty()) {
                state->ReferenceName = pl.Title;
            }
            if (pl.PlaylistPosition > -3) { // -3 = don't change
                state->InsertPos = pl.PlaylistPosition;
                state->Flags = LoadingState::UpdateAdditionalPos | LoadingState::IgnoreExistingPosition;
            }

            LoadFromUrl(pl.Url, playlist, state, finishCallback);
            state->PendingUrls.pop();
        } else {
            // Finished
            if (state->Flags & LoadingState::LoadingLikes) {
                Config::SaveExtendedConfig();
            }
            playlist->Release();
            delete state;
            if (finishCallback)
                finishCallback();
        }
    });
}

void SoundCloudAPI::LoadLikes() {
    if (!Plugin::instance()->isConnected())
        return;

    IAIMPPlaylist *pl = Plugin::instance()->GetPlaylist(L"Soundcloud - Likes");

    LoadingState *state = new LoadingState();
    state->Flags = LoadingState::LoadingLikes;
    state->ReferenceName = Config::GetString(L"UserName") + L"'s likes";
    GetExistingTrackIds(pl, state);

    LoadFromUrl(L"https://api.soundcloud.com/me/favorites?limit=200", pl, state);
}

void SoundCloudAPI::LoadStream() {
    if (!Plugin::instance()->isConnected())
        return;

    IAIMPPlaylist *pl = Plugin::instance()->GetPlaylist(L"Soundcloud - Stream");

    LoadingState *state = new LoadingState();
    state->ReferenceName = Config::GetString(L"UserName") + L"'s stream";
    state->Flags = LoadingState::IgnoreExistingPosition;
    GetExistingTrackIds(pl, state);

    LoadFromUrl(L"https://api.soundcloud.com/me/activities?limit=300", pl, state);
}

void SoundCloudAPI::GetExistingTrackIds(IAIMPPlaylist *pl, LoadingState *state) {
    if (!pl || !state)
        return;

    // Fetch current track ids from playlist
    Plugin::instance()->ForEveryItem(pl, [&](IAIMPPlaylistItem *, IAIMPFileInfo *, int64_t id) -> int {
        if (id > 0) {
            state->TrackIds.insert(id);
        }
        return 0;
    });
}

void SoundCloudAPI::ResolveUrl(const std::wstring &url, const std::wstring &playlistTitle, bool createPlaylist) {
    if (url.find(L"soundcloud.com/stream") != std::wstring::npos) {
        LoadStream();
        return;
    }

    AimpHTTP::Get(L"https://api.soundcloud.com/resolve?url=" + Tools::UrlEncode(url) + L"&client_id=" TEXT(CLIENT_ID) L"&oauth_token=" + Plugin::instance()->getAccessToken(), [createPlaylist, url, playlistTitle](unsigned char *data, int size) {
        rapidjson::Document d;
        d.Parse(reinterpret_cast<const char *>(data));

        if ((d.IsObject() && d.HasMember("kind")) || d.IsArray()) {
            std::wstring finalUrl;
            rapidjson::Value *addDirectly = nullptr;
            std::wstring plName;
            bool monitor = true;
            LoadingState *state = new LoadingState();
            std::set<std::wstring> toMonitor;

            if (d.IsArray()) {
                plName = url;
                addDirectly = &d;
            } else {
                if (strcmp(d["kind"].GetString(), "user") == 0) {
                    plName = L"Soundcloud - " + Tools::ToWString(d["username"]);

                    std::wstring base = L"https://api.soundcloud.com/users/" + std::to_wstring(d["id"].GetInt64());
                    finalUrl = base + L"/playlists";
                    std::wstring secondUrl = base + L"/tracks";
                    toMonitor.insert(secondUrl);

                    state->PendingUrls.push({ std::wstring(), secondUrl, 0 });

                    state->Flags = LoadingState::IgnoreExistingPosition;
                } else if (strcmp(d["kind"].GetString(), "track") == 0) {
                    if (url.find(L"/recommended") != std::wstring::npos) {
                        plName = L"Recommended for " + Tools::ToWString(d["title"]);
                        finalUrl = L"https://api.soundcloud.com/tracks/" + std::to_wstring(d["id"].GetInt64()) + L"/related";
                    } else {
                        plName = Tools::ToWString(d["title"]);
                        addDirectly = &d;

                        monitor = false;
                    }
                } else if (strcmp(d["kind"].GetString(), "playlist") == 0 && d.HasMember("tracks")) {
                    plName = Tools::ToWString(d["user"]["username"]) + L" - " + Tools::ToWString(d["title"]);
                    addDirectly = &d["tracks"];

                    finalUrl = L"https://api.soundcloud.com/playlists/" + std::to_wstring(d["id"].GetInt64());
                } else {
                    MessageBox(Plugin::instance()->GetMainWindowHandle(), L"Could not resolve this url.", L"Error", MB_OK | MB_ICONERROR);
                    delete state;
                    return;
                }
            }

            IAIMPPlaylist *pl = nullptr;
            std::wstring finalPlaylistName;
            if (createPlaylist) {
                finalPlaylistName = playlistTitle.empty() ? plName : playlistTitle;
                pl = Plugin::instance()->GetPlaylist(finalPlaylistName);
                if (!pl) {
                    delete state;
                    return;
                }
            } else {
                pl = Plugin::instance()->GetCurrentPlaylist();
                if (!pl) {
                    delete state;
                    return;
                }
            }

            std::wstring playlistId;
            IAIMPPropertyList *plProp = nullptr;
            if (SUCCEEDED(pl->QueryInterface(IID_IAIMPPropertyList, reinterpret_cast<void **>(&plProp)))) {
                IAIMPString *str = nullptr;
                if (SUCCEEDED(plProp->GetValueAsObject(AIMP_PLAYLIST_PROPID_ID, IID_IAIMPString, reinterpret_cast<void **>(&str)))) {
                    playlistId = str->GetData();
                    str->Release();
                }
                if (finalPlaylistName.empty()) {
                    if (SUCCEEDED(plProp->GetValueAsObject(AIMP_PLAYLIST_PROPID_NAME, IID_IAIMPString, reinterpret_cast<void **>(&str)))) {
                        finalPlaylistName = str->GetData();
                        str->Release();
                    }
                }
                plProp->Release();
            }

            state->ReferenceName = plName;

            GetExistingTrackIds(pl, state);
            
            if (addDirectly) {
                AddFromJson(pl, *addDirectly, state);
                delete state;
            } else {
                LoadFromUrl(finalUrl, pl, state);
            }

            if (monitor) {
                toMonitor.insert(finalUrl);
                for (const auto &x : toMonitor) {
                    Config::MonitorUrls.push_back({ x, playlistId, state->Flags, plName });
                }
                Config::SaveExtendedConfig();
            }
        } else {
            MessageBox(Plugin::instance()->GetMainWindowHandle(), L"Could not resolve this url.", L"Error", MB_OK | MB_ICONERROR);
            return;
        }
    });
}



void SoundCloudAPI::LoadMyTracksAndPlaylists() {
    if (!Plugin::instance()->isConnected())
        return;

    auto processResponse = [](unsigned char *data, int size) {
        rapidjson::Document d;
        d.Parse(reinterpret_cast<const char *>(data));
        LoadingState *state = new LoadingState();

        if (d.IsArray()) {
            if (d.Size() > 0 && (*d.Begin()).HasMember("kind") && strcmp((*d.Begin())["kind"].GetString(), "track") == 0) {
                state->ReferenceName = L"Soundcloud - " + Config::GetString(L"UserName");
                IAIMPPlaylist *pl = Plugin::instance()->GetPlaylist(state->ReferenceName);
                if (pl) {
                    AddFromJson(pl, d, state);
                }
            } else {
                for (auto x = d.Begin(), e = d.End(); x != e; x++) {
                    const rapidjson::Value &px = *x;
                    if (!px.IsObject())
                        continue;

                    if (px.HasMember("kind") && strcmp(px["kind"].GetString(), "playlist") == 0 && px.HasMember("tracks") && px["tracks"].IsArray() && px["tracks"].Size() > 0) {
                        // Create playlist and add tracks in a loop
                        state->ReferenceName = Tools::ToWString(px["title"]);
                        IAIMPPlaylist *pl = Plugin::instance()->GetPlaylist(state->ReferenceName);
                        if (pl) {
                            if (px.HasMember("secret_uri")) {
                                Config::MonitorUrls.push_back({ Tools::ToWString(px["secret_uri"]), Plugin::instance()->PlaylistId(pl), state->Flags, state->ReferenceName });
                            }

                            AddFromJson(pl, px["tracks"], state);
                        }
                    }
                }
            }
        }
        delete state;
        Config::SaveExtendedConfig();
    };

    AimpHTTP::Get(L"https://api.soundcloud.com/me/tracks?oauth_token=" + Plugin::instance()->getAccessToken(), processResponse);
    AimpHTTP::Get(L"https://api.soundcloud.com/me/playlists?oauth_token=" + Plugin::instance()->getAccessToken(), processResponse);
}


void SoundCloudAPI::LikeSong(int64_t trackId) {
    std::wstring url(L"https://api.soundcloud.com/me/favorites/");
    url += std::to_wstring(trackId);
    url += L"?client_id=" TEXT(CLIENT_ID)
           L"&oauth_token=" + Plugin::instance()->getAccessToken();

    AimpHTTP::Put(url);
}

void SoundCloudAPI::UnlikeSong(int64_t trackId) {
    std::wstring url(L"https://api.soundcloud.com/me/favorites/");
    url += std::to_wstring(trackId);
    url += L"?client_id=" TEXT(CLIENT_ID)
           L"&oauth_token=" + Plugin::instance()->getAccessToken();

    AimpHTTP::Delete(url);

    if (IAIMPPlaylist *likes = Plugin::instance()->GetPlaylist(L"Soundcloud - Likes", false, false)) {
        Plugin::instance()->ForEveryItem(likes, [&](IAIMPPlaylistItem *item, IAIMPFileInfo *info, int64_t id) -> int {
            if (id == trackId) {
                return Plugin::FLAG_DELETE_ITEM | Plugin::FLAG_STOP_LOOP;
            }
            return 0;
        });
        likes->Release();
    }
}

void SoundCloudAPI::LoadRecommendations(int64_t trackId, bool createPlaylist, IAIMPPlaylistItem *item) {
    IAIMPString *name = nullptr;
    std::wstring plName(L"Recommended for ");
    if (item && SUCCEEDED(item->GetValueAsObject(AIMP_PLAYLISTITEM_PROPID_DISPLAYTEXT, IID_IAIMPString, reinterpret_cast<void  **>(&name)))) {
        plName += name->GetData();
        name->Release();
    }

    IAIMPPlaylist *pl = nullptr;
    if (createPlaylist) {
        pl = Plugin::instance()->GetPlaylist(plName);
        if (!pl)
            return;
    } else {
        pl = Plugin::instance()->GetCurrentPlaylist();
        if (!pl)
            return;
    }

    LoadingState *state = new LoadingState();
    state->ReferenceName = plName;
    state->Flags = LoadingState::IgnoreExistingPosition;
    int plIndex = 0;
    if (item && SUCCEEDED(item->GetValueAsInt32(AIMP_PLAYLISTITEM_PROPID_INDEX, &plIndex))) {
        state->InsertPos = plIndex + 1;
    }
    GetExistingTrackIds(pl, state);

    LoadFromUrl(L"https://api.soundcloud.com/tracks/" + std::to_wstring(trackId) + L"/related", pl, state);
}

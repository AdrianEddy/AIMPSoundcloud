#include "SoundCloudAPI.h"

#include "AIMPSoundcloud.h"
#include "AimpHTTP.h"
#include "SDK/apiFileManager.h"
#include "SDK/apiPlaylists.h"
#include "AIMPString.h"
#include "Tools.h"
#include <Strsafe.h>

AIMPSoundcloudPlugin *SoundCloudAPI::m_plugin = nullptr;

void SoundCloudAPI::Init(AIMPSoundcloudPlugin *plugin) {
    m_plugin = plugin;
}

void SoundCloudAPI::AddFromJson(IAIMPPlaylist *playlist, const rapidjson::Value &d, LoadingState *state) {
    if (!playlist)
        return;

    int insertAt = -1;
    if (state) {
        insertAt = state->InsertPos; // -1 = insert at the end
        if (insertAt >= 0)
            insertAt += state->AdditionalPos;
    }

    auto processItem = [&](const rapidjson::Value &item, IAIMPFileInfo *file_info) {
        // TODO: don't add duplicates

        if (state && strcmp(item["kind"].GetString(), "playlist") == 0) {
            DebugA("Adding pending playlist %s\n", item["title"].GetString());
            std::wstring url = Tools::ToWString(item["tracks_uri"]);
            url += L"?limit=200";
            state->PendingPlaylists.push({ url, playlist->GetItemCount() });
            return;
        }

        if (!item.HasMember("stream_url")) {
            if (item.HasMember("id")) {
                wchar_t stream_url[256];
                wsprintf(stream_url, L"https://api.soundcloud.com/tracks/%ld/stream?client_id=" TEXT(STREAM_CLIENT_ID), item["id"].GetInt64());

                file_info->SetValueAsObject(AIMP_FILEINFO_PROPID_FILENAME, new AIMPString(stream_url));
            } else {
                DebugA("NOT adding %s\n ", item["title"].GetString());
                return;
            }
        } else {
            file_info->SetValueAsObject(AIMP_FILEINFO_PROPID_FILENAME, new AIMPString(Tools::ToWString(item["stream_url"]) + L"?client_id=" TEXT(STREAM_CLIENT_ID)));
        }

        file_info->SetValueAsObject(AIMP_FILEINFO_PROPID_ARTIST, new AIMPString(Tools::ToWString(item["user"]["username"])));
        file_info->SetValueAsObject(AIMP_FILEINFO_PROPID_TITLE, new AIMPString(Tools::ToWString(item["title"])));
        file_info->SetValueAsObject(AIMP_FILEINFO_PROPID_URL, new AIMPString(Tools::ToWString(item["permalink_url"])));
        if (item.HasMember("genre"))
            file_info->SetValueAsObject(AIMP_FILEINFO_PROPID_GENRE, new AIMPString(Tools::ToWString(item["genre"])));
        // TODO: options: Add length to title
        file_info->SetValueAsFloat(AIMP_FILEINFO_PROPID_DURATION, item["duration"].GetInt64() / 1000.0);

        const DWORD flags = AIMP_PLAYLIST_ADD_FLAGS_FILEINFO | AIMP_PLAYLIST_ADD_FLAGS_NOCHECKFORMAT | AIMP_PLAYLIST_ADD_FLAGS_NOEXPAND | AIMP_PLAYLIST_ADD_FLAGS_NOASYNC;
        if (SUCCEEDED(playlist->Add(file_info, flags, insertAt))) {
            if (insertAt >= 0) {
                insertAt++;
                if (state)
                    state->AdditionalPos++;
            }
        }

        DebugA("Adding %s\n ", item["title"].GetString());
    };

    IAIMPFileInfo *file_info = nullptr;
    if (m_plugin->core()->CreateObject(IID_IAIMPFileInfo, reinterpret_cast<void **>(&file_info)) == S_OK) {
        if (d.IsArray()) {
            for (auto x = d.Begin(), e = d.End(); x != e; x++) {
                const rapidjson::Value *px = &(*x);
                if (!px->IsObject())
                    continue;

                if (px->HasMember("origin"))
                    px = &((*px)["origin"]);

                processItem(*px, file_info);
            }
        } else if (d.IsObject()) {
            processItem(d, file_info);
        }
        file_info->Release();
    }
}

void SoundCloudAPI::LoadFromUrl(const std::wstring &url, IAIMPPlaylist *playlist, LoadingState *state) {
    if (!playlist || !state)
        return;

    AimpHTTP::Get(url + L"&client_id=" TEXT(CLIENT_ID) L"&oauth_token=" + m_plugin->getAccessToken(), [playlist, state](unsigned char *data, int size) {
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
                LoadFromUrl(pl.Url, playlist, state);
                state->PendingPlaylists.pop();

                hasAnyPending = true;
            }
        }

        playlist->EndUpdate();

        if (!hasAnyPending) {
            // Finished
            delete state;
        }
    });
}

void SoundCloudAPI::LoadLikes() {
    if (!m_plugin->isConnected())
        return;

    IAIMPPlaylist *pl = m_plugin->GetPlaylist(L"Soundcloud - Likes");
    pl->DeleteAll();

    LoadFromUrl(L"https://api.soundcloud.com/me/favorites?limit=200", pl, new LoadingState());
}

void SoundCloudAPI::LoadStream() {
    if (!m_plugin->isConnected())
        return;

    IAIMPPlaylist *pl = m_plugin->GetPlaylist(L"Soundcloud - Stream");
    pl->DeleteAll();

    LoadFromUrl(L"https://api.soundcloud.com/me/activities?limit=300", pl, new LoadingState());
}

void SoundCloudAPI::ResolveUrl(const std::wstring &url, const std::wstring &playlistTitle, bool createPlaylist) {
    if (url.find(L"/stream") != std::wstring::npos) {
        LoadStream();
        return;
    }

    AimpHTTP::Get(L"https://api.soundcloud.com/resolve?url=" + Tools::UrlEncode(url) + L"&client_id=" TEXT(CLIENT_ID) L"&oauth_token=" + m_plugin->getAccessToken(), [createPlaylist, url, playlistTitle](unsigned char *data, int size) {
        rapidjson::Document d;
        d.Parse(reinterpret_cast<const char *>(data));

        if ((d.IsObject() && d.HasMember("kind")) || d.IsArray()) {
            wchar_t finalUrl[256];
            rapidjson::Value *addDirectly = nullptr;
            std::wstring plName;

            if (d.IsArray()) {
                plName = url;
                addDirectly = &d;
            } else {
                if (strcmp(d["kind"].GetString(), "user") == 0) {
                    plName = L"Soundcloud - " + Tools::ToWString(d["username"]);
                    wsprintf(finalUrl, L"https://api.soundcloud.com/users/%ld/tracks?client_id=" TEXT(CLIENT_ID), d["id"].GetInt64());
                    // TODO: load playlists

                } else if (strcmp(d["kind"].GetString(), "track") == 0) {
                    if (url.find(L"/recommended") != std::wstring::npos) {
                        plName = L"Recommended for " + Tools::ToWString(d["title"]);
                        wsprintf(finalUrl, L"https://api.soundcloud.com/tracks/%ld/related?client_id=" TEXT(CLIENT_ID), d["id"].GetInt64());
                    } else {
                        plName = Tools::ToWString(d["title"]);
                        addDirectly = &d;
                    }
                } else if (strcmp(d["kind"].GetString(), "playlist") == 0 && d.HasMember("tracks")) {
                    plName = Tools::ToWString(d["user"]["username"]) + L" - " + Tools::ToWString(d["title"]);
                    addDirectly = &(d["tracks"]);
                }
            }

            IAIMPPlaylist *pl = nullptr;
            if (createPlaylist) {
                pl = m_plugin->GetPlaylist(playlistTitle.empty()? plName : playlistTitle);
                pl->DeleteAll();
            } else {
                pl = m_plugin->GetCurrentPlaylist();
            }

            if (addDirectly) {
                AddFromJson(pl, *addDirectly, nullptr);
            } else {
                LoadFromUrl(finalUrl, pl, new LoadingState());
            }
        }
    });
}

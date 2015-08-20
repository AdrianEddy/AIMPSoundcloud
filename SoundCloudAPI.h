#pragma once

#include "rapidjson/document.h"
#include <queue>

class AIMPSoundcloudPlugin;
class IAIMPPlaylist;

class SoundCloudAPI {
public:
    struct LoadingState {
        struct PendingPlaylist {
            std::wstring Url;
            int PlaylistPosition;
        };
        std::queue<PendingPlaylist> PendingPlaylists;
        int AdditionalPos;
        int InsertPos;
        LoadingState() : AdditionalPos(0), InsertPos(-1) {}
    };

    static void Init(AIMPSoundcloudPlugin *plugin);

    static void LoadLikes();
    static void LoadStream();

    static void LoadFromUrl(const std::wstring &url, IAIMPPlaylist *playlist, LoadingState *state);
    static void ResolveUrl(const std::wstring &url, const std::wstring &playlistTitle = std::wstring(), bool createPlaylist = true);

private:
    static void AddFromJson(IAIMPPlaylist *, const rapidjson::Value &, LoadingState *state);

    SoundCloudAPI();
    SoundCloudAPI(const SoundCloudAPI &);
    SoundCloudAPI &operator=(const SoundCloudAPI &);

    static AIMPSoundcloudPlugin *m_plugin;
};

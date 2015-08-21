#pragma once

#include "rapidjson/document.h"
#include <queue>
#include <unordered_set>
#include <cstdint>

class IAIMPPlaylist;

class SoundCloudAPI {
public:
    struct LoadingState {
        struct PendingPlaylist {
            std::wstring Title;
            std::wstring Url;
            int PlaylistPosition;
        };
        std::unordered_set<int64_t> TrackIds;
        std::queue<PendingPlaylist> PendingPlaylists;
        std::wstring ReferenceName;
        int AdditionalPos;
        int InsertPos;
        bool UpdateAdditionalPos;
        LoadingState() : AdditionalPos(0), InsertPos(0), UpdateAdditionalPos(false) {}
    };

    static void LoadLikes();
    static void LoadStream();

    static void LoadFromUrl(const std::wstring &url, IAIMPPlaylist *playlist, LoadingState *state);
    static void ResolveUrl(const std::wstring &url, const std::wstring &playlistTitle = std::wstring(), bool createPlaylist = true);

    static void LikeSong(int64_t trackId);
    static void UnlikeSong(int64_t trackId);
    static void LoadRecommendations(int64_t trackId, bool createPlaylist);

private:
    static void GetExistingTrackIds(IAIMPPlaylist *pl, LoadingState *state);
    static void AddFromJson(IAIMPPlaylist *, const rapidjson::Value &, LoadingState *state);

    SoundCloudAPI();
    SoundCloudAPI(const SoundCloudAPI &);
    SoundCloudAPI &operator=(const SoundCloudAPI &);
};

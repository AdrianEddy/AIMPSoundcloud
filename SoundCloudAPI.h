#pragma once

#include "rapidjson/document.h"
#include <queue>
#include <unordered_set>
#include <cstdint>

class IAIMPPlaylist;
class IAIMPPlaylistItem;

class SoundCloudAPI {
public:
    struct LoadingState {
        struct PendingPlaylist {
            std::wstring Title;
            std::wstring Url;
            int PlaylistPosition;
        };
        enum LoadingFlags {
            None                   = 0x00,
            UpdateAdditionalPos    = 0x01,
            LoadingLikes           = 0x02,
            IgnoreExistingPosition = 0x04
        };
        std::unordered_set<int64_t> TrackIds;
        std::queue<PendingPlaylist> PendingPlaylists;
        std::wstring ReferenceName;
        std::wstring PendingUrl;
        int AdditionalPos;
        int InsertPos;
        int PendingPos;
        int Offset;
        LoadingFlags Flags;
        LoadingState() : AdditionalPos(0), InsertPos(0), Offset(0), PendingPos(-3), Flags(None) {}
    };

    static void LoadLikes();
    static void LoadStream();

    static void LoadFromUrl(std::wstring url, IAIMPPlaylist *playlist, LoadingState *state);
    static void ResolveUrl(const std::wstring &url, const std::wstring &playlistTitle = std::wstring(), bool createPlaylist = true);

    static void LikeSong(int64_t trackId);
    static void UnlikeSong(int64_t trackId);
    static void LoadRecommendations(int64_t trackId, bool createPlaylist, IAIMPPlaylistItem *item = nullptr);

private:
    static void GetExistingTrackIds(IAIMPPlaylist *pl, LoadingState *state);
    static void AddFromJson(IAIMPPlaylist *, const rapidjson::Value &, LoadingState *state);

    SoundCloudAPI();
    SoundCloudAPI(const SoundCloudAPI &);
    SoundCloudAPI &operator=(const SoundCloudAPI &);
};

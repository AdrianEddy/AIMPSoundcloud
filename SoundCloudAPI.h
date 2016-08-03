#pragma once

#include "rapidjson/document.h"
#include <queue>
#include <unordered_set>
#include <cstdint>
#include <functional>
#include <memory>

class IAIMPPlaylist;
class IAIMPPlaylistItem;

class SoundCloudAPI {
public:
    struct LoadingState {
        struct PendingUrl {
            std::wstring Title;
            std::wstring Url;
            int PlaylistPosition;
        };
        enum LoadingFlags {
            None                   = 0x00,
            UpdateAdditionalPos    = 0x01,
            LoadingLikes           = 0x02,
            IgnoreExistingPosition = 0x04,
            IgnoreNextPage         = 0x08,
            LoadAllPages           = 0x10
        };
        std::unordered_set<int64_t> TrackIds;
        std::queue<PendingUrl> PendingUrls;
        std::wstring ReferenceName;
        std::wstring CleanURL;
        int AdditionalPos;
        int InsertPos;
        int Offset;
        int AddedItems;
        int Flags;
        LoadingState() : AdditionalPos(0), InsertPos(0), Offset(0), AddedItems(0), Flags(None) {}
    };

    static void LoadLikes();
    static void LoadStream();
    static void LoadMyTracksAndPlaylists();

    static void LoadFromUrl(std::wstring url, IAIMPPlaylist *playlist, std::shared_ptr<LoadingState> state, std::function<void()> finishCallback = std::function<void()>());
    static void ResolveUrl(const std::wstring &url, const std::wstring &playlistTitle = std::wstring(), bool createPlaylist = true);

    static void LikeSong(int64_t trackId);
    static void UnlikeSong(int64_t trackId);
    static void RepostSong(int64_t trackId);
    static void LoadRecommendations(int64_t trackId, bool createPlaylist, IAIMPPlaylistItem *item = nullptr);
    static void GetExistingTrackIds(IAIMPPlaylist *pl, std::shared_ptr<LoadingState> state);

private:
    static void AddFromJson(IAIMPPlaylist *, const rapidjson::Value &, std::shared_ptr<LoadingState> state);

    SoundCloudAPI();
    SoundCloudAPI(const SoundCloudAPI &);
    SoundCloudAPI &operator=(const SoundCloudAPI &);
};

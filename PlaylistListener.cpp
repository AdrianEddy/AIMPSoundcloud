#include "PlaylistListener.h"
#include "Config.h"
#include <algorithm>

void WINAPI PlaylistListener::PlaylistActivated(IAIMPPlaylist *Playlist) {

}

void WINAPI PlaylistListener::PlaylistAdded(IAIMPPlaylist *Playlist) {

}

void WINAPI PlaylistListener::PlaylistRemoved(IAIMPPlaylist *Playlist) {
    std::wstring playlistId;
    IAIMPPropertyList *plProp = nullptr;
    if (SUCCEEDED(Playlist->QueryInterface(IID_IAIMPPropertyList, reinterpret_cast<void **>(&plProp)))) {
        IAIMPString *id = nullptr;
        if (SUCCEEDED(plProp->GetValueAsObject(AIMP_PLAYLIST_PROPID_ID, IID_IAIMPString, reinterpret_cast<void **>(&id)))) {
            playlistId = id->GetData();
            id->Release();
        }
        plProp->Release();
    }

    if (!playlistId.empty()) {
        Config::MonitorUrls.erase(
            std::remove_if(Config::MonitorUrls.begin(), Config::MonitorUrls.end(), [&](const Config::MonitorUrl &element) -> bool {
                return element.PlaylistID == playlistId;
            }
        ), Config::MonitorUrls.end());
    }
    Config::SaveExtendedConfig();
}

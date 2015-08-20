#include "AIMPSoundcloud.h"

#include "AIMPString.h"

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

        return playlistPointer;
    } else {
        if (SUCCEEDED(m_playlistManager->CreatePlaylist(&plName, activate, &playlistPointer)))
            return playlistPointer;
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


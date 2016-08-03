#include "PlayerHook.h"
#include <string>
#include "Tools.h"
#include "Config.h"

HRESULT WINAPI PlayerHook::OnCheckURL(IAIMPString *URL, BOOL *Handled) {
    if (wcsstr(URL->GetData(), L"soundcloud://") == 0 && wcsstr(URL->GetData(), L"soundcloud.com") == 0)
        return E_FAIL;

    int64_t id = Tools::TrackIdFromUrl(URL->GetData());
    std::wstring stream_url = L"https://api.soundcloud.com/tracks/" + std::to_wstring(id) + L"/stream";
    
    if (auto ti = Tools::TrackInfo(id)) {
        stream_url = ti->Stream;
    }

    stream_url += L"?client_id=" TEXT(STREAM_CLIENT_ID);
    URL->SetData(const_cast<wchar_t *>(stream_url.c_str()), stream_url.size());

    *Handled = 1;
    return S_OK;
}

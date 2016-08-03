#include "Config.h"

#include "AIMPString.h"
#include "AIMPSoundcloud.h"
#include <ShlObj.h>
#include "SDK/apiCore.h"
#include "AimpHTTP.h"
#include "Tools.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/filewritestream.h"

IAIMPConfig *Config::m_config = nullptr;
std::wstring Config::m_configFolder;

std::unordered_set<int64_t> Config::TrackExclusions;
std::unordered_set<int64_t> Config::Likes;
std::vector<Config::MonitorUrl> Config::MonitorUrls;
std::unordered_map<int64_t, Config::TrackInfo> Config::TrackInfos;

bool Config::Init(IAIMPCore *core) {
    IAIMPString *str = nullptr;
    if (SUCCEEDED(core->GetPath(AIMP_CORE_PATH_PROFILE, &str))) {
        m_configFolder = std::wstring(str->GetData()) + L"AIMPSoundcloud\\";
        CreateDirectory(m_configFolder.c_str(), NULL);
        str->Release();
    }
    return SUCCEEDED(core->QueryInterface(IID_IAIMPConfig, reinterpret_cast<void **>(&m_config))) && m_config;
}

void Config::Deinit() {
    if (m_config)
        m_config->Release();
}

void Config::Delete(const std::wstring &name) {
    m_config->Delete(AIMPString(L"Soundcloud\\" + name));
}

void Config::SetString(const std::wstring &name, const std::wstring &value) {
    m_config->SetValueAsString(AIMPString(L"Soundcloud\\" + name), AIMPString(value));
}

void Config::SetInt64(const std::wstring &name, const int64_t &value) {
    m_config->SetValueAsInt64(AIMPString(L"Soundcloud\\" + name), value);
}

void Config::SetInt32(const std::wstring &name, const int32_t &value) {
    m_config->SetValueAsInt32(AIMPString(L"Soundcloud\\" + name), value);
}

std::wstring Config::GetString(const std::wstring &name, const std::wstring &def) {
    IAIMPString *str = nullptr;
    if (SUCCEEDED(m_config->GetValueAsString(AIMPString(L"Soundcloud\\" + name), &str)) && str) {
        std::wstring result = str->GetData();
        str->Release();
        return result;
    }
    return def;
}

int64_t Config::GetInt64(const std::wstring &name, const int64_t &def) {
    int64_t val = 0;
    if (SUCCEEDED(m_config->GetValueAsInt64(AIMPString(L"Soundcloud\\" + name), &val))) {
        return val;
    }
    return def;
}

int32_t Config::GetInt32(const std::wstring &name, const int32_t &def) {
    int32_t val = 0;
    if (SUCCEEDED(m_config->GetValueAsInt32(AIMPString(L"Soundcloud\\" + name), &val))) {
        return val;
    }
    return def;
}

void Config::SaveExtendedConfig() {
    std::wstring configFile = m_configFolder + L"Config.json";
    FILE *file = nullptr;
    if (_wfopen_s(&file, configFile.c_str(), L"wb") == 0) {
        using namespace rapidjson;
        char writeBuffer[65536];

        FileWriteStream stream(file, writeBuffer, sizeof(writeBuffer));
        PrettyWriter<decltype(stream), UTF16<>> writer(stream);

        writer.StartObject();
        writer.String(L"Exclusions");
        writer.StartArray();
        for (const auto &trackId : TrackExclusions) {
            writer.Int64(trackId);
        }
        writer.EndArray();

        writer.String(L"Likes");
        writer.StartArray();
        for (const auto &trackId : Likes) {
            writer.Int64(trackId);
        }
        writer.EndArray();

        writer.String(L"MonitorURLs");
        writer.StartArray();
        for (const auto &monitorUrl : MonitorUrls) {
            writer << monitorUrl;
        }
        writer.EndArray();

        writer.EndObject();

        fclose(file);
    }
    SaveCache();
}

void Config::LoadExtendedConfig() {
    TrackExclusions.clear();
    MonitorUrls.clear();

    std::wstring configFile = m_configFolder + L"Config.json";
    FILE *file = nullptr;
    if (_wfopen_s(&file, configFile.c_str(), L"rb") == 0) {
        using namespace rapidjson;
        char buffer[65536];

        FileReadStream stream(file, buffer, sizeof(buffer));
        GenericDocument<UTF16<>> d;
        d.ParseStream<0, UTF8<>, decltype(stream)>(stream);

        if (d.IsObject()) {
            if (d.HasMember(L"Exclusions")) {
                const GenericValue<UTF16<>> &v = d[L"Exclusions"];
                if (v.IsArray()) {
                    for (auto x = v.Begin(), e = v.End(); x != e; x++) {
                        TrackExclusions.insert((*x).GetInt64());
                    }
                }
            }
            if (d.HasMember(L"Likes")) {
                const GenericValue<UTF16<>> &v = d[L"Likes"];
                if (v.IsArray()) {
                    for (auto x = v.Begin(), e = v.End(); x != e; x++) {
                        Likes.insert((*x).GetInt64());
                    }
                }
            }
            if (d.HasMember(L"MonitorURLs")) {
                const GenericValue<UTF16<>> &v = d[L"MonitorURLs"];
                if (v.IsArray()) {
                    for (auto x = v.Begin(), e = v.End(); x != e; x++) {
                        if ((*x).IsObject()) {
                            MonitorUrls.push_back(*x);
                        }
                    }
                }
            }
        }
        fclose(file);
    }
    LoadCache();
}

void Config::SaveCache() {
    std::wstring configFile = m_configFolder + L"Cache.json";
    FILE *file = nullptr;
    if (_wfopen_s(&file, configFile.c_str(), L"wb") == 0) {
        using namespace rapidjson;
        char writeBuffer[65536];

        FileWriteStream stream(file, writeBuffer, sizeof(writeBuffer));
        Writer<decltype(stream), UTF16<>> writer(stream);

        writer.StartObject();
        for (const auto &ti : TrackInfos) {
            writer.String(std::to_wstring(ti.first).c_str());
            writer << ti.second;
        }
        writer.EndObject();

        fclose(file);
    }
}

void Config::LoadCache() {
    TrackInfos.clear();

    std::wstring configFile = m_configFolder + L"Cache.json";
    FILE *file = nullptr;
    if (_wfopen_s(&file, configFile.c_str(), L"rb") == 0) {
        using namespace rapidjson;
        char buffer[65536];

        FileReadStream stream(file, buffer, sizeof(buffer));
        GenericDocument<UTF16<>> d;
        d.ParseStream<0, UTF8<>, decltype(stream)>(stream);

        if (d.IsObject()) {
            for (auto x = d.MemberBegin(), e = d.MemberEnd(); x != e; x++) {
                int64_t id = std::stoll((*x).name.GetString());
                TrackInfos[id] = (*x).value;
                TrackInfos[id].Id = id;
            }
        }
        fclose(file);
    }
}

bool Config::ResolveTrackInfo(int64_t id) {
    std::wstring url(L"https://api.soundcloud.com/tracks/");
    url += std::to_wstring(id);
    url += L"/?client_id=" TEXT(STREAM_CLIENT_ID);

    if (Plugin::instance()->isConnected())
        url += L"&oauth_token=" + Plugin::instance()->getAccessToken();

    bool result = false;
    std::wstring title(L"Unknown"), permalink, waveform_id, artwork;
    double duration = -1;
    std::wstring stream_url(L"https://api.soundcloud.com/tracks/" + std::to_wstring(id) + L"/stream");

    AimpHTTP::Get(url, [&](unsigned char *data, int size) {
        rapidjson::Document d;
        d.Parse(reinterpret_cast<const char *>(data));

        if (d.IsObject() && d.HasMember("id")) {
            int64_t trackId = d["id"].GetInt64();
            
            if (d.HasMember("stream_url"))
                stream_url = Tools::ToWString(d["stream_url"]);

            waveform_id = Tools::ToWString(d["waveform_url"]);
            if (!waveform_id.empty()) {
                std::wstring::size_type ptr, ptr_end;
                if ((ptr = waveform_id.find(L".com/")) != std::wstring::npos) {
                    ptr += 5;
                    if ((ptr_end = waveform_id.find(L".", ptr)) != std::wstring::npos) {
                        waveform_id = waveform_id.substr(ptr, ptr_end - ptr);
                    }
                }
            }
            title = Tools::ToWString(d["title"]);
            permalink = Tools::ToWString(d["permalink_url"]);
            artwork = Tools::ToWString(d["artwork_url"]);
            duration = d["duration"].GetInt64() / 1000.0;
            result = true;
        }

        Config::TrackInfos[id] = Config::TrackInfo(id, title, stream_url, permalink, waveform_id, artwork, duration);

        Config::SaveCache();
    }, true);

    return result;
}

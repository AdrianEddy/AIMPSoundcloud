#include "Config.h"

#include "AIMPString.h"
#include <ShlObj.h>
#include "SDK/apiCore.h"
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
}

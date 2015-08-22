#pragma once

#include <string>
#include <unordered_set>
#include <set>
#include "SDK/apiCore.h"
#include <cstdint>
#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/filewritestream.h"

#define CLIENT_ID        "7ab49ecff90c309cd51cab81c1f5baed"
#define STREAM_CLIENT_ID "b45b1aa10f1ac2941910a7f0d10f8e28"
#define CLIENT_SECRET    "7a4d4f50405fdf80e5446d319c14c6fa"

class Config {
public:
    struct MonitorUrl {
        std::wstring URL;
        std::wstring PlaylistName;
        std::wstring GroupName;

        typedef rapidjson::PrettyWriter<rapidjson::FileWriteStream, rapidjson::UTF16<>> Writer;
        typedef rapidjson::GenericValue<rapidjson::UTF16<>> Value;

        MonitorUrl(const std::wstring &url, const std::wstring &playlistName, const std::wstring &groupName = std::wstring())
            : URL(url), PlaylistName(playlistName), GroupName(groupName) {
        }
        MonitorUrl(const Value &v) {
            if (v.IsObject()) {
                URL = v[L"URL"].GetString();
                PlaylistName = v[L"PlaylistName"].GetString();
                GroupName = v[L"GroupName"].GetString();
            }
        }

        friend bool operator <(const MonitorUrl &a, const MonitorUrl &b) {
            return a.URL < b.URL && a.PlaylistName < b.PlaylistName && a.GroupName < b.GroupName;
        }
        friend Writer &operator <<(Writer &writer, const MonitorUrl &that) {
            writer.StartObject();

            writer.String(L"URL");
            writer.String(that.URL.c_str(), that.URL.size());

            writer.String(L"PlaylistName");
            writer.String(that.PlaylistName.c_str(), that.PlaylistName.size());

            writer.String(L"GroupName");
            writer.String(that.GroupName.c_str(), that.GroupName.size());

            writer.EndObject();
            return writer;
        }
    };

    static bool Init(IAIMPCore *core);
    static void Deinit();

    static void Delete(const std::wstring &name);

    static void SetString(const std::wstring &name, const std::wstring &value);
    static void SetInt64(const std::wstring &name, const int64_t &value);

    static std::wstring GetString(const std::wstring &name, const std::wstring &def = std::wstring());
    static int64_t GetInt64(const std::wstring &name, const int64_t &def = 0);

    static inline std::wstring PluginConfigFolder() { return m_configFolder; }

    static void SaveExtendedConfig();
    static void LoadExtendedConfig();

    static std::unordered_set<int64_t> Likes;
    static std::unordered_set<int64_t> TrackExclusions;
    static std::set<MonitorUrl> MonitorUrls;

private:
    Config();
    Config(const Config&);
    Config& operator=(const Config&);

    static std::wstring m_configFolder;
    static IAIMPConfig *m_config;
};

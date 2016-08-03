#pragma once

#include <string>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include "SDK/apiCore.h"
#include <cstdint>
#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/filewritestream.h"

#define CLIENT_ID        "7ab49ecff90c309cd51cab81c1f5baed"
#define STREAM_CLIENT_ID "02gUJC0hH2ct1EGOcYXQIzRFU91c72Ea"
#define CLIENT_SECRET    "7a4d4f50405fdf80e5446d319c14c6fa"

class Config {
public:
    struct MonitorUrl {
        std::wstring URL;
        std::wstring PlaylistID;
        int Flags;
        std::wstring GroupName;

        typedef rapidjson::PrettyWriter<rapidjson::FileWriteStream, rapidjson::UTF16<>> Writer;
        typedef rapidjson::GenericValue<rapidjson::UTF16<>> Value;

        MonitorUrl(const std::wstring &url, const std::wstring &playlistID, int flags, const std::wstring &groupName = std::wstring())
            : URL(url), PlaylistID(playlistID), Flags(flags), GroupName(groupName) {

        }

        MonitorUrl(const Value &v) {
            if (v.IsObject()) {
                URL = v[L"URL"].GetString();
                Flags = v[L"Flags"].GetInt();
                PlaylistID = v[L"PlaylistID"].GetString();
                GroupName = v[L"GroupName"].GetString();
            }
        }

        friend Writer &operator <<(Writer &writer, const MonitorUrl &that) {
            writer.StartObject();

            writer.String(L"URL");
            writer.String(that.URL.c_str(), that.URL.size());

            writer.String(L"Flags");
            writer.Int(that.Flags);

            writer.String(L"PlaylistID");
            writer.String(that.PlaylistID.c_str(), that.PlaylistID.size());

            writer.String(L"GroupName");
            writer.String(that.GroupName.c_str(), that.GroupName.size());

            writer.EndObject();
            return writer;
        }
    };
    struct TrackInfo {
        int64_t Id;
        std::wstring Name;
        std::wstring Stream;
        std::wstring Permalink;
        std::wstring Waveform_ID;
        std::wstring Artwork;
        double Duration;

        typedef rapidjson::Writer<rapidjson::FileWriteStream, rapidjson::UTF16<>> Writer;
        typedef rapidjson::GenericValue<rapidjson::UTF16<>> Value;

        TrackInfo(): Duration(0) { }
        TrackInfo(int64_t id, const std::wstring &name, const std::wstring &stream, const std::wstring &permalink, const std::wstring &waveform, const std::wstring &artwork, double duration)
            : Id(id), Name(name), Stream(stream), Permalink(permalink), Waveform_ID(waveform), Artwork(artwork), Duration(duration) {

        }

        TrackInfo(const Value &v) {
            if (v.IsObject()) {
                Name        = v[L"N"].GetString();
                Stream      = v[L"S"].GetString();
                Permalink   = v[L"P"].GetString();
                Waveform_ID = v[L"W"].GetString();
                Artwork     = v[L"A"].GetString();
                Duration    = v[L"D"].GetDouble();
            }
        }

        friend Writer &operator <<(Writer &writer, const TrackInfo &that) {
            writer.StartObject();

            writer.String(L"N");
            writer.String(that.Name.c_str(), that.Name.size());
            
            writer.String(L"S");
            writer.String(that.Stream.c_str(), that.Stream.size());
            
            writer.String(L"P");
            writer.String(that.Permalink.c_str(), that.Permalink.size());

            writer.String(L"W");
            writer.String(that.Waveform_ID.c_str(), that.Waveform_ID.size());

            writer.String(L"A");
            writer.String(that.Artwork.c_str(), that.Artwork.size());

            writer.String(L"D");
            writer.Double(that.Duration);

            writer.EndObject();
            return writer;
        }
    };

    static bool Init(IAIMPCore *core);
    static void Deinit();

    static void Delete(const std::wstring &name);

    static void SetString(const std::wstring &name, const std::wstring &value);
    static void SetInt64(const std::wstring &name, const int64_t &value);
    static void SetInt32(const std::wstring &name, const int32_t &value);

    static std::wstring GetString(const std::wstring &name, const std::wstring &def = std::wstring());
    static int64_t GetInt64(const std::wstring &name, const int64_t &def = 0);
    static int32_t GetInt32(const std::wstring &name, const int32_t &def = 0);

    static inline std::wstring PluginConfigFolder() { return m_configFolder; }

    static void SaveExtendedConfig();
    static void LoadExtendedConfig();

    static void SaveCache();
    static void LoadCache();
    static bool ResolveTrackInfo(int64_t id);

    static std::unordered_set<int64_t> Likes;
    static std::unordered_set<int64_t> TrackExclusions;
    static std::vector<MonitorUrl> MonitorUrls;
    static std::unordered_map<int64_t, TrackInfo> TrackInfos;

private:
    Config();
    Config(const Config&);
    Config& operator=(const Config&);

    static std::wstring m_configFolder;
    static IAIMPConfig *m_config;
};

#pragma once

#include <string>
#include "SDK/apiCore.h"

#define CLIENT_ID        "7ab49ecff90c309cd51cab81c1f5baed"
#define STREAM_CLIENT_ID "b45b1aa10f1ac2941910a7f0d10f8e28"
#define CLIENT_SECRET    "7a4d4f50405fdf80e5446d319c14c6fa"

class Config {
public:
    static bool Init(IAIMPCore *core);

    static void Delete(const std::wstring &name);

    static void SetString(const std::wstring &name, const std::wstring &value);
    static void SetInt64(const std::wstring &name, const INT64 &value);

    static std::wstring GetString(const std::wstring &name, const std::wstring &def = std::wstring());
    static INT64 GetInt64(const std::wstring &name, const INT64 &def = 0);

    static inline std::wstring PluginConfigFolder() { return m_configFolder; }

private:
    Config();
    Config(const Config&);
    Config& operator=(const Config&);

    static std::wstring m_configFolder;
    static IAIMPConfig *m_config;
};

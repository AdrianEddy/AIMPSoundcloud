#include "Config.h"

#include "AIMPString.h"
#include <ShlObj.h>
#include "SDK/apiCore.h"

IAIMPConfig *Config::m_config = nullptr;
std::wstring Config::m_configFolder;

bool Config::Init(IAIMPCore *core) {
    IAIMPString *str = nullptr;
    if (SUCCEEDED(core->GetPath(AIMP_CORE_PATH_PROFILE, &str))) {
        m_configFolder = std::wstring(str->GetData()) + L"AIMPSoundcloud\\";
        CreateDirectory(m_configFolder.c_str(), NULL);
        str->Release();
    }
    return SUCCEEDED(core->QueryInterface(IID_IAIMPConfig, reinterpret_cast<void **>(&m_config))) && m_config;
}

void Config::Delete(const std::wstring &name) {
    m_config->Delete(new AIMPString(L"Soundcloud\\" + name));
}

void Config::SetString(const std::wstring &name, const std::wstring &value) {
    m_config->SetValueAsString(new AIMPString(L"Soundcloud\\" + name), new AIMPString(value));
}

void Config::SetInt64(const std::wstring &name, const INT64 &value) {
    m_config->SetValueAsInt64(new AIMPString(L"Soundcloud\\" + name), value);
}

std::wstring Config::GetString(const std::wstring &name, const std::wstring &def) {
    IAIMPString *str = nullptr;
    if (SUCCEEDED(m_config->GetValueAsString(new AIMPString(L"Soundcloud\\" + name), &str)) && str) {
        std::wstring result = str->GetData();
        str->Release();
        return result;
    }
    return def;
}

INT64 Config::GetInt64(const std::wstring &name, const INT64 &def) {
    INT64 val = 0;
    if (SUCCEEDED(m_config->GetValueAsInt64(new AIMPString(L"Soundcloud\\" + name), &val))) {
        return val;
    }
    return def;
}

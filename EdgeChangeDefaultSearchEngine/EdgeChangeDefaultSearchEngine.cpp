#include <windows.h>
#include <ShlObj.h>
#include <fstream>
#include <chrono>
#include <ctime>
#include <string>
#include "../EdgeAutomation/dllmain.cpp"
#include <iostream>

#pragma comment(lib, "Shell32.lib")

using namespace std;

typedef BOOL(*ExecuteTaskFn)();

std::string static GetCurrentTimeMillisecond()
{
    time_t now = time(NULL);

    tm localTime;

    localtime_s(&localTime, &now);

    char buffer[64];

    strftime(
        buffer,
        sizeof(buffer),
        "%Y-%m-%d %H:%M:%S",
        &localTime);

    return std::string(buffer);
}

std::string static GetLogPath()
{
    char appData[MAX_PATH];

    SHGetFolderPathA(
        NULL,
        CSIDL_LOCAL_APPDATA,
        NULL,
        0,
        appData);

    std::string folder =
        std::string(appData) +
        "\\ReverseEnginneringTesting";

    // Create folder if it doesn't exist
    CreateDirectoryA(
        folder.c_str(),
        NULL);

    return folder + "\\execution.log";
}

void static WriteLog(
    const std::string& startTime,
    const std::string& endTime,
    const std::string& status,
    long durationSeconds)
{
    std::ofstream logFile(
        GetLogPath().c_str(),
        std::ios::app);

    logFile << "----------------------------------\n";
    logFile << "Start Time : " << startTime << "\n";
    logFile << "End Time   : " << endTime << "\n";
    logFile << "Status     : " << status << "\n";
    logFile << "Total Time : "
        << durationSeconds
        << " seconds\n";
    logFile << "----------------------------------\n\n";

    logFile.close();
}

int main()
{
    system("taskkill /F /IM msedge.exe");
    auto start =
        std::chrono::steady_clock::now();

    std::string startTime =
        GetCurrentTimeMillisecond();

    bool success = false;

    // the automation from the dll will go here

    success = ChangeSearchEngine();

    if (success) {
		cout << "Successfully changed the default search engine." << endl;
    }
    else {
		cout << "Failed to change the default search engine." << endl;
    }

    // ends here

    auto end =
        std::chrono::steady_clock::now();

    std::string endTime =
        GetCurrentTimeMillisecond();

    long duration =
        (long)std::chrono::duration_cast
        <
        std::chrono::seconds
        >
        (
            end - start
        ).count();

    WriteLog(
        startTime,
        endTime,
        success ? "Success" : "Failure",
        duration);

    return success ? 0 : 1;
}
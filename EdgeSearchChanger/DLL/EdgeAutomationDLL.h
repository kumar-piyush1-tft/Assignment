#pragma once

#ifdef EDGEAUTOMATIONDLL_EXPORTS
#define EDGE_API __declspec(dllexport)
#else
#define EDGE_API __declspec(dllimport)
#endif

// -------------------------------------------------------
// Returned by RunAutomation() to the EXE caller.
// -------------------------------------------------------
enum class AutomationStatus
{
    Success              = 0,
    ErrNoLocalAppData    = 1,
    ErrPrefsNotFound     = 2,
    ErrPrefsReadFailed   = 3,
    ErrJsonParseFailed   = 4,
    ErrPrefsWriteFailed  = 5,
    ErrEdgeStillRunning  = 6,
};

extern "C"
{
    // Main entry point called by the EXE.
    // Returns AutomationStatus cast to int.
    EDGE_API int RunAutomation();

    // Helper: human-readable string for a status code.
    EDGE_API const char* StatusMessage(int statusCode);
}

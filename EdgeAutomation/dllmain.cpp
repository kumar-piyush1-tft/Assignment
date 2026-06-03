// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include <windows.h>
#include <string>

#pragma comment(lib, "Advapi32.lib")

extern "C" __declspec(dllexport)
BOOL ChangeSearchEngine()
{
    HKEY hKey;
    const char* providerName = "Yahoo";
    const char* searchUrl = "https://www.yahoo.com/search?q={searchTerms}";
    LONG result = RegCreateKeyExA(
        HKEY_CURRENT_USER,
        "SOFTWARE\\Policies\\Microsoft\\Edge",
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        KEY_WRITE,
        NULL,
        &hKey,
        NULL);

    if (result != ERROR_SUCCESS)
        return FALSE;

    DWORD enabled = 1;

    RegSetValueExA(
        hKey,
        "DefaultSearchProviderEnabled",
        0,
        REG_DWORD,
        reinterpret_cast<const BYTE*>(&enabled),
        sizeof(enabled));

    RegSetValueExA(
        hKey,
        "DefaultSearchProviderName",
        0,
        REG_SZ,
        reinterpret_cast<const BYTE*>(providerName),
        static_cast<DWORD>(strlen(providerName) + 1));
     RegSetValueExA(
        hKey,
        "DefaultSearchProviderSearchURL",
        0,
        REG_SZ,
        reinterpret_cast<const BYTE*>(searchUrl),
		 static_cast<DWORD>(strlen(searchUrl) + 1));

    RegCloseKey(hKey);

    return TRUE;
}


BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}



#include "pch.h"
#include "EdgeChanger.h"
#include <string>
#include <winuser.h>
#pragma comment(lib, "user32.lib")

void PressKey(WORD vk)
{
    keybd_event(vk, 0, 0, 0);
    Sleep(50);
    keybd_event(vk, 0, KEYEVENTF_KEYUP, 0);
    Sleep(150);
}

bool LaunchEdgeToSettings()
{
    // Try common Edge paths
    const wchar_t* edgePaths[] = {
        L"C:\\Program Files (x86)\\Microsoft\\Edge\\Application\\msedge.exe",
        L"C:\\Program Files\\Microsoft\\Edge\\Application\\msedge.exe"
    };

    const wchar_t* url =
        L"edge://settings/privacy/services/search";

    for (auto edgePath : edgePaths)
    {
        // Check if file exists
        if (GetFileAttributesW(edgePath) == INVALID_FILE_ATTRIBUTES)
            continue;

        // Build command line
        std::wstring cmd = L"\"";
        cmd += edgePath;
        cmd += L"\" --new-window ";
        cmd += url;

        STARTUPINFOW si = {};
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_SHOW;
        PROCESS_INFORMATION pi = {};

        BOOL ok = CreateProcessW(
            NULL,
            (LPWSTR)cmd.c_str(),
            NULL, NULL, FALSE,
            0, NULL, NULL, &si, &pi);

        if (ok)
        {
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            return true;
        }
    }
    return false;
}

bool ChangeSearchEngine()
{
    try
    {
        // 1. Kill Edge
        system("taskkill /IM msedge.exe /F >nul 2>&1");
        Sleep(2000);

        // 2. Launch Edge with settings URL
        if (!LaunchEdgeToSettings())
            return false;

        // 3. Wait for page to fully load
        Sleep(5000);

        // 4. Find Edge window and bring to foreground
        HWND hEdge = NULL;
        for (int i = 0; i < 20; i++)
        {
            hEdge = FindWindowW(L"Chrome_WidgetWin_1", NULL);
            if (hEdge) break;
            Sleep(500);
        }
        if (!hEdge) return false;

        SetForegroundWindow(hEdge);
        Sleep(1000);

        // 5. Click address bar and type URL to make sure
        //    we're on the right page
        // Ctrl+L to focus address bar
        keybd_event(VK_CONTROL, 0, 0, 0);
        keybd_event('L', 0, 0, 0);
        keybd_event('L', 0, KEYEVENTF_KEYUP, 0);
        keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
        Sleep(500);

        // Type the URL character by character
        const char* url = "edge://settings/privacy/services/search";
        for (int i = 0; url[i]; i++)
        {
            keybd_event((BYTE)VkKeyScanA(url[i]), 0, 0, 0);
            Sleep(20);
            keybd_event((BYTE)VkKeyScanA(url[i]), 0, KEYEVENTF_KEYUP, 0);
            Sleep(20);
        }
        Sleep(200);
        PressKey(VK_RETURN);
        Sleep(4000); // wait for settings page to load

        // 6. Tab to the search engine dropdown
        //    Press Tab 3 times to reach it
        PressKey(VK_TAB);
        Sleep(300);
        PressKey(VK_TAB);
        Sleep(300);
        PressKey(VK_TAB);
        Sleep(300);
        PressKey(VK_TAB);  // ← add this 4th tab
        Sleep(300);

        // 7. Open dropdown with Enter
        PressKey(VK_RETURN);
        Sleep(800);


        for (int i = 0; i < 1; i++)  // ← change 1 to 3 in Ghidra
        {
            PressKey(VK_DOWN);
            Sleep(1000);
        }

        
        // 9. Confirm with Enter
        PressKey(VK_RETURN);
        Sleep(3000);

        // 10. Close Edge gracefully
        SetForegroundWindow(hEdge);
        Sleep(500);
        keybd_event(VK_MENU, 0, 0, 0);
        keybd_event(VK_F4, 0, 0, 0);
        keybd_event(VK_F4, 0, KEYEVENTF_KEYUP, 0);
        keybd_event(VK_MENU, 0, KEYEVENTF_KEYUP, 0);
        Sleep(3000);

        // 11. Force kill if still running
        system("taskkill /IM msedge.exe /F >nul 2>&1");
        Sleep(1000);

        return true;
    }
    catch (...)
    {
        return false;
    }
}



























//#include "pch.h"
//#include "EdgeChanger.h"
//#include <string>
//#include <shellapi.h>
//#include <UIAutomation.h>
//#include <winuser.h>      // ← SetCursorPos, mouse_event, keybd_event
//#include <objbase.h>      // ← CoInitializeEx, CoUninitialize
//#pragma comment(lib, "UIAutomationCore.lib")
//#pragma comment(lib, "ole32.lib")
//#pragma comment(lib, "oleaut32.lib")
//#pragma comment(lib, "user32.lib")  // ← mouse_event, SetCursorPos
//
//bool ChangeSearchEngine()
//{
//    try
//    {
//        // 1. Kill Edge
//        system("taskkill /IM msedge.exe /F >nul 2>&1");
//        Sleep(2000);
//
//        // 2. Open Edge to search settings
//        system("start msedge \"edge://settings/privacy/services/search\"");
//        Sleep(4000);
//
//        // 3. Init COM
//        HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
//        if (FAILED(hr) && hr != RPC_E_CHANGED_MODE)
//            return false;
//
//        // 4. Create UI Automation
//        IUIAutomation* pAuto = nullptr;
//        hr = CoCreateInstance(
//            CLSID_CUIAutomation, NULL,
//            CLSCTX_INPROC_SERVER,
//            IID_IUIAutomation,
//            (void**)&pAuto);
//
//        if (FAILED(hr) || !pAuto)
//        {
//            CoUninitialize();
//            return false;
//        }
//
//        // 5. Find Edge window (retry up to 10s)
//        HWND hEdge = NULL;
//        for (int i = 0; i < 20; i++)
//        {
//            hEdge = FindWindowW(L"Chrome_WidgetWin_1", NULL);
//            if (hEdge) break;
//            Sleep(500);
//        }
//        if (!hEdge)
//        {
//            pAuto->Release();
//            CoUninitialize();
//            return false;
//        }
//
//        Sleep(2000); // Let page fully render
//
//        // 6. Get root element for Edge window
//        IUIAutomationElement* pRoot = nullptr;
//        pAuto->ElementFromHandle(hEdge, &pRoot);
//        if (!pRoot)
//        {
//            pAuto->Release();
//            CoUninitialize();
//            return false;
//        }
//
//        // 7. Find the combobox on the page
//        IUIAutomationCondition* pCond = nullptr;
//        VARIANT varType;
//        varType.vt = VT_I4;
//        varType.lVal = UIA_ComboBoxControlTypeId;
//        pAuto->CreatePropertyCondition(
//            UIA_ControlTypePropertyId, varType, &pCond);
//
//        IUIAutomationElementArray* pCombos = nullptr;
//        pRoot->FindAll(TreeScope_Descendants, pCond, &pCombos);
//        pCond->Release();
//
//        IUIAutomationElement* pCombo = nullptr;
//        if (pCombos)
//        {
//            int count = 0;
//            pCombos->get_Length(&count);
//            for (int i = 0; i < count; i++)
//            {
//                IUIAutomationElement* pElem = nullptr;
//                pCombos->GetElement(i, &pElem);
//                if (pElem)
//                {
//                    pCombo = pElem;
//                    break;
//                }
//            }
//            pCombos->Release();
//        }
//
//        if (!pCombo)
//        {
//            pRoot->Release();
//            pAuto->Release();
//            CoUninitialize();
//            return false;
//        }
//
//        // 8. Expand the combobox dropdown
//        IUIAutomationExpandCollapsePattern* pExpand = nullptr;
//        pCombo->GetCurrentPatternAs(
//            UIA_ExpandCollapsePatternId,
//            IID_PPV_ARGS(&pExpand));
//        if (pExpand)
//        {
//            pExpand->Expand();
//            pExpand->Release();
//        }
//        else
//        {
//            // Fallback: mouse click to open dropdown
//            RECT rect;
//            pCombo->get_CurrentBoundingRectangle(&rect);
//            int x = (rect.left + rect.right) / 2;
//            int y = (rect.top + rect.bottom) / 2;
//            SetCursorPos(x, y);
//            Sleep(100);
//            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
//            Sleep(80);
//            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
//        }
//        Sleep(1000);
//
//        // 9. Find Yahoo! in dropdown — search entire desktop
//        //    since dropdown may be a popup outside Edge window
//        IUIAutomationElement* pDesktop = nullptr;
//        pAuto->GetRootElement(&pDesktop);
//
//        IUIAutomationElement* pYahoo = nullptr;
//
//        if (pDesktop)
//        {
//            IUIAutomationCondition* pNameCond = nullptr;
//            VARIANT varName;
//            varName.vt = VT_BSTR;
//            varName.bstrVal = SysAllocString(L"Yahoo!");
//            pAuto->CreatePropertyCondition(
//                UIA_NamePropertyId, varName, &pNameCond);
//            SysFreeString(varName.bstrVal);
//
//            pDesktop->FindFirst(
//                TreeScope_Descendants, pNameCond, &pYahoo);
//            pNameCond->Release();
//            pDesktop->Release();
//        }
//
//        if (!pYahoo)
//        {
//            pCombo->Release();
//            pRoot->Release();
//            pAuto->Release();
//            CoUninitialize();
//            return false;
//        }
//
//        // 10. Click Yahoo!
//        IUIAutomationInvokePattern* pInvoke = nullptr;
//        hr = pYahoo->GetCurrentPatternAs(
//            UIA_InvokePatternId,
//            IID_PPV_ARGS(&pInvoke));
//
//        if (SUCCEEDED(hr) && pInvoke)
//        {
//            pInvoke->Invoke();
//            pInvoke->Release();
//        }
//        else
//        {
//            // Fallback: mouse click
//            RECT rect;
//            pYahoo->get_CurrentBoundingRectangle(&rect);
//            int x = (rect.left + rect.right) / 2;
//            int y = (rect.top + rect.bottom) / 2;
//            SetCursorPos(x, y);
//            Sleep(100);
//            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
//            Sleep(80);
//            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
//        }
//
//        Sleep(1000);
//
//        // 11. Cleanup
//        pYahoo->Release();
//        pCombo->Release();
//        pRoot->Release();
//        pAuto->Release();
//        CoUninitialize();
//
//        // 12. Close Edge
//        system("taskkill /IM msedge.exe /F >nul 2>&1");
//
//        return true;
//    }
//    catch (...)
//    {
//        return false;
//    }
//}
























//#include "pch.h"
//#include "EdgeChanger.h"
//#include <Windows.h>
//#include <ShlObj.h>
//#include <fstream>
//#include <nlohmann/json.hpp>
//
//using json = nlohmann::json;
//
//json BuildYahooData()
//{
//    json y;
//    y["alternate_urls"] = json::array();
//    y["contextual_search_url"] = "";
//    y["created_from_play_api"] = false;
//    y["date_created"] = "0";
//    y["doodle_url"] = "";
//    y["enforced_by_policy"] = false;
//    y["favicon_url"] = "https://search.yahoo.com/favicon.ico";
//    y["featured_by_policy"] = false;
//    y["id"] = "3";
//    y["image_search_branding_label"] = "";
//    y["image_translate_source_language_param_key"] = "";
//    y["image_translate_target_language_param_key"] = "";
//    y["image_translate_url"] = "";
//    y["image_url"] = "";
//    y["image_url_post_params"] = "";
//    y["input_encodings"] = json::array({ "UTF-8" });
//    y["is_active"] = 1;
//    y["is_for_msb_tab_to_search"] = false;
//    y["keyword"] = "yahoo.com";
//    y["last_modified"] = "13420737747299081";
//    y["last_visited"] = "13424898392512023";
//    y["logo_url"] = "";
//    y["managed_default_search_engine"] = false;
//    y["managed_search_engine"] = false;
//    y["new_tab_url"] = "https://search.yahoo.com?fr=crmas_sfp";
//    y["originating_url"] = "";
//    y["policy_origin"] = 0;
//    y["preconnect_to_search_url"] = false;
//    y["prefetch_likely_navigations"] = false;
//    y["prepopulate_id"] = 2;
//    y["safe_for_autoreplace"] = true;
//    y["search_intent_params"] = json::array();
//    y["search_url_post_params"] = "";
//    y["short_name"] = "Yahoo!";
//    y["short_name_lang"] = "";
//    y["starter_pack_id"] = 0;
//    y["suggestions_url"] = "https://search.yahoo.com/sugg/chrome?output=fxjson&appid=crmas_sfp&command={searchTerms}";
//    y["suggestions_url_post_params"] = "";
//    y["synced_guid"] = "485bf7d3-0215-45af-87dc-538868000002";
//    y["url"] = "https://search.yahoo.com/search{google:pathWildcard}?ei={inputEncoding}&fr=crmas_sfp&p={searchTerms}";
//    y["usage_count"] = 0;
//    return y;
//}
//
//bool UpdatePreferences(const std::string& path, const json& yahooData)
//{
//    std::ifstream in(path);
//    if (!in.good()) return false;
//
//    json prefs;
//    try { in >> prefs; }
//    catch (...) { return false; }
//    in.close();
//
//    prefs["default_search_provider"]["guid"] = "485bf7d3-0215-45af-87dc-538868000002";
//    prefs["default_search_provider"]["enabled"] = true;
//    prefs["default_search_provider"]["keyword"] = "yahoo.com";
//    prefs["default_search_provider"]["name"] = "Yahoo!";
//    prefs["default_search_provider"]["search_url"] = "https://search.yahoo.com/search?ei={inputEncoding}&fr=crmas_sfp&p={searchTerms}";
//    prefs["default_search_provider_data"]["mirrored_template_url_data"] = yahooData;
//
//    std::ofstream out(path);
//    if (!out.good()) return false;
//    out << prefs.dump();
//    return true;
//}
//
//bool UpdateSecurePreferences(const std::string& path, const json& yahooData)
//{
//    std::ifstream in(path);
//    if (!in.good()) return false;
//
//    json prefs;
//    try { in >> prefs; }
//    catch (...) { return false; }
//    in.close();
//
//    // Key difference: Secure Preferences uses "template_url_data" not "mirrored_template_url_data"
//    prefs["default_search_provider_data"]["template_url_data"] = yahooData;
//
//    std::ofstream out(path);
//    if (!out.good()) return false;
//    out << prefs.dump();
//    return true;
//}
//
//bool ChangeSearchEngine()
//{
//    // 1. Kill Edge and wait for it to fully release file locks
//    system("taskkill /IM msedge.exe /F >nul 2>&1");
//    Sleep(1500);
//
//    // 2. Build paths
//    char path[MAX_PATH];
//    SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path);
//    std::string base = std::string(path) + "\\Microsoft\\Edge\\User Data\\Default\\";
//
//    json yahooData = BuildYahooData();
//
//    // 3. Update only the two files that actually change
//    bool p = UpdatePreferences(base + "Preferences", yahooData);
//    bool sp = UpdateSecurePreferences(base + "Secure Preferences", yahooData);
//
//    return p && sp;
//}
























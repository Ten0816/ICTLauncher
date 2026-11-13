#pragma once
#include <Siv3D.hpp>
#include <string>
#include <vector>

#if SIV3D_PLATFORM(WINDOWS)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

// Windows/ブラウザ固有の処理。UI から Win32 API を直接操作しない。
namespace Config
{
    enum class BrowserType { Edge, Chrome, None };

    struct BrowserInfo
    {
        FilePath path;
        BrowserType type = BrowserType::None;
    };

    struct BrowserProcess
    {
        HANDLE processHandle = nullptr;
        DWORD processId = 0;
        HWND windowHandle = nullptr;
        DWORD windowProcessId = 0;
        std::vector<HWND> existingWindows;
        bool waitingForWindow = false;
        ULONGLONG launchTick = 0;
        FilePath executablePath;

        void Close();
    };

    BrowserInfo FindBrowser();
    bool IsURL(StringView path);
    bool IsHTML(const FilePath& path);
    bool LaunchBrowser(const BrowserInfo& browser, const FilePath& target, BrowserProcess& result);
    HWND FindNewBrowserWindow(const BrowserProcess& browser);
    bool IsTrackedWindowOpen(const BrowserProcess& browser);
}
#endif

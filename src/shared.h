#pragma once

#include <windows.h>
#include <shellapi.h>
#include <shobjidl.h>

#include <iostream>
#include <chrono>

void SaveWinPosition(HWND hwnd, const char* subKeyName) {
    WINDOWPLACEMENT wp;
    wp.length = sizeof(WINDOWPLACEMENT);
    GetWindowPlacement(hwnd, &wp);

    // wp.rcNormalPosition is the coordinates when NOT minimized/maximized
    DWORD x = (DWORD)wp.rcNormalPosition.left;
    DWORD y = (DWORD)wp.rcNormalPosition.top;
    DWORD w = (DWORD)(wp.rcNormalPosition.right - wp.rcNormalPosition.left);
    DWORD h = (DWORD)(wp.rcNormalPosition.bottom - wp.rcNormalPosition.top);

    // If the window is currently minimized, we want to save the 'normal' coords
    // instead of the -32000 values.
    
    HKEY hKey;
    char fullPath[256];
    wsprintf(fullPath, "Software\\IBKR_Tunnel\\WindowClass\\%s", subKeyName);

    if (RegCreateKeyEx(HKEY_CURRENT_USER, fullPath, 0, NULL, 
        REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) 
    {
        RegSetValueEx(hKey, "X", 0, REG_DWORD, (const BYTE*)&x, sizeof(DWORD));
        RegSetValueEx(hKey, "Y", 0, REG_DWORD, (const BYTE*)&y, sizeof(DWORD));
        RegSetValueEx(hKey, "W", 0, REG_DWORD, (const BYTE*)&w, sizeof(DWORD));
        RegSetValueEx(hKey, "H", 0, REG_DWORD, (const BYTE*)&h, sizeof(DWORD));
        RegCloseKey(hKey);
    }
}

bool LoadWinPosition(const char* subKeyName, int &x, int &y, int &w, int &h) {
    HKEY hKey;
    char fullPath[256];
    wsprintf(fullPath, "Software\\IBKR_Tunnel\\WindowClass\\%s", subKeyName);

    if (RegOpenKeyEx(HKEY_CURRENT_USER, fullPath, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD dwSize = sizeof(DWORD);
        RegQueryValueEx(hKey, "X", NULL, NULL, (LPBYTE)&x, &dwSize);
        RegQueryValueEx(hKey, "Y", NULL, NULL, (LPBYTE)&y, &dwSize);
        RegQueryValueEx(hKey, "W", NULL, NULL, (LPBYTE)&w, &dwSize);
        RegQueryValueEx(hKey, "H", NULL, NULL, (LPBYTE)&h, &dwSize);
        RegCloseKey(hKey);
        return true;
    }
    return false;
}

HICON CreateGrayIcon(HICON hOriginal) {
    ICONINFO ii;
    GetIconInfo(hOriginal, &ii);

    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem    = CreateCompatibleDC(hdcScreen);

    BITMAP bm;
    GetObject(ii.hbmColor, sizeof(bm), &bm);

    HBITMAP hbmGray = CreateCompatibleBitmap(hdcScreen, bm.bmWidth, bm.bmHeight);
    SelectObject(hdcMem, hbmGray);

    DrawIconEx(hdcMem, 0, 0, hOriginal, bm.bmWidth, bm.bmHeight, 0, NULL, DI_NORMAL);

    // Blend to gray
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth       = bm.bmWidth;
    bmi.bmiHeader.biHeight      = -bm.bmHeight;
    bmi.bmiHeader.biPlanes      = 1;
    bmi.bmiHeader.biBitCount    = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    std::vector<DWORD> pixels(bm.bmWidth * bm.bmHeight);
    GetDIBits(hdcMem, hbmGray, 0, bm.bmHeight, pixels.data(), &bmi, DIB_RGB_COLORS);

    for (auto& px : pixels) {
        BYTE r = (px >> 16) & 0xFF;
        BYTE g = (px >>  8) & 0xFF;
        BYTE b =  px        & 0xFF;
        BYTE a = (px >> 24) & 0xFF;
        BYTE gray = (BYTE)(0.299f * r + 0.587f * g + 0.114f * b);
        // Reduce opacity too so it looks "disabled"
        px = ((a / 2) << 24) | (gray << 16) | (gray << 8) | gray;
    }

    SetDIBits(hdcMem, hbmGray, 0, bm.bmHeight, pixels.data(), &bmi, DIB_RGB_COLORS);

    ICONINFO iiGray = { TRUE, ii.xHotspot, ii.yHotspot, ii.hbmMask, hbmGray };
    HICON hGray = CreateIconIndirect(&iiGray);

    DeleteObject(ii.hbmColor);
    DeleteObject(ii.hbmMask);
    DeleteObject(hbmGray);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);

    return hGray;
}

#include <tlhelp32.h>

bool IsProcessRunning(const char* processName) {
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) return false;

    PROCESSENTRY32 pe = { sizeof(PROCESSENTRY32) };
    bool found = false;

    if (Process32First(hSnap, &pe)) {
        do {
            if (_stricmp(pe.szExeFile, processName) == 0) {
                found = true;
                break;
            }
        } while (Process32Next(hSnap, &pe));
    }

    CloseHandle(hSnap);
    return found;
}

void EnsureGatewayRunning() {
    if (!IsProcessRunning("ibgateway.exe")) {
        ShellExecuteA(NULL, "open",
            "C:\\Program Files\\IBKR\\ibgateway\\1046\\ibgateway.exe",
            NULL, NULL, SW_SHOW);
    }
}

#include <initguid.h>
#include <propkey.h>
#include <propvarutil.h>

void SetWindowTaskbarId(HWND hWnd, const wchar_t* id) {
    IPropertyStore* pps;
    if (SUCCEEDED(SHGetPropertyStoreForWindow(hWnd, IID_PPV_ARGS(&pps)))) {
        PROPVARIANT pv;
        InitPropVariantFromString(id, &pv);
        pps->SetValue(PKEY_AppUserModel_ID, pv);
        pps->Commit();
        PropVariantClear(&pv);
        pps->Release();
    }
}
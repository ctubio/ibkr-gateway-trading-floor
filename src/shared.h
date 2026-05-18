#pragma once

#include <thread>
#include <mutex>
#include <iostream>
#include <chrono>

#include "EWrapper.h"
#include "EClientSocket.h"
#include "EReaderOSSignal.h"
#include "EReader.h"

#include <windows.h>
#include <shellapi.h>
#include <shobjidl.h>

#include <dwmapi.h>
#include <initguid.h>
#include <propkey.h>
#include <propvarutil.h>
#include <mmsystem.h>

constexpr const char* APP_REG_ROOT = "Software\\ibkr-gateway-trading-floor";

std::unordered_map<std::string, HWND> g_AppWindows;

// Dark mode colors
#define DM_BG        RGB(32,  32,  32)
#define DM_BG2       RGB(45,  45,  45)
#define DM_TEXT      RGB(220, 220, 220)
#define DM_BORDER    RGB(70,  70,  70)

// Light mode colors  
#define LM_BG        GetSysColor(COLOR_BTNFACE)
#define LM_TEXT      GetSysColor(COLOR_WINDOWTEXT)

HBRUSH hDarkBrush = NULL;
HBRUSH hDarkBrush2 = NULL;

static HWND hDebugEdit = NULL;
static std::vector<std::string> debugBuffer; // stores messages when window is closed

static const char* SETTINGS_CLASS_NAME      = "TNTSettingsWindowClass";
static const char* DEBUGLOG_CLASS_NAME      = "TNTSettingsDebugLogWindowClass";
static const char* BOOK_CLASS_NAME          = "TNTBookWindowClass";
static const char* BOOK_NEW_LIST_CLASS_NAME = "TNTBookNewListWindowClass";
static const char* COINS_CLASS_NAME         = "TNTCoinsWindowClass";
static const char* DASHBOARD_CLASS_NAME     = "TNTDashboardClass";
static const char* ORDERS_CLASS_NAME        = "TNTOrdersWindowClass";
static const char* LEVELS_CLASS_NAME        = "TNTLevelsWindowClass";
static const char* NEWS_CLASS_NAME          = "TNTNewsWindowClass";
static const char* DIAMONDS_CLASS_NAME      = "TNTDiamondsWindowClass";
static const char* TICKER_CLASS_NAME        = "TNTTickerWindowClass";

NOTIFYICONDATA nid = { 0 };

void LogDebug(const std::string& msg) {
    time_t now = time(0);
    char tstr[26] = {};
    ctime_s(tstr, sizeof(tstr), &now);
    std::string timestamp = tstr;
    if (!timestamp.empty()) timestamp.pop_back(); // remove newline

    std::string fullMsg = "[" + timestamp + "] " + msg + "\r\n";
    debugBuffer.push_back(fullMsg);

    if (hDebugEdit && IsWindow(hDebugEdit)) {
        // Append to edit control
        int len = GetWindowTextLength(hDebugEdit);
        SendMessage(hDebugEdit, EM_SETSEL, len, len);
        SendMessageA(hDebugEdit, EM_REPLACESEL, FALSE, (LPARAM)fullMsg.c_str());
        // Auto-scroll to bottom
        SendMessage(hDebugEdit, EM_SCROLLCARET, 0, 0);
    }
}

void InitDarkBrushes() {
    if (hDarkBrush)  DeleteObject(hDarkBrush);
    if (hDarkBrush2) DeleteObject(hDarkBrush2);
    hDarkBrush  = CreateSolidBrush(DM_BG);
    hDarkBrush2 = CreateSolidBrush(DM_BG2);
}

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

void SaveWinPosition(HWND hWnd) {
    WINDOWPLACEMENT wp;
    wp.length = sizeof(WINDOWPLACEMENT);
    GetWindowPlacement(hWnd, &wp);

    // wp.rcNormalPosition is the coordinates when NOT minimized/maximized
    DWORD x = (DWORD)wp.rcNormalPosition.left;
    DWORD y = (DWORD)wp.rcNormalPosition.top;
    DWORD w = (DWORD)(wp.rcNormalPosition.right - wp.rcNormalPosition.left);
    DWORD h = (DWORD)(wp.rcNormalPosition.bottom - wp.rcNormalPosition.top);

    // If the window is currently minimized, we want to save the 'normal' coords
    // instead of the -32000 values.
    
    HKEY hKey;
    char className[256] = {};
    GetClassNameA(hWnd, className, sizeof(className));
    char fullPath[256];
    wsprintf(fullPath, "%s\\WindowSettings\\%s", APP_REG_ROOT, className);

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
    wsprintf(fullPath, "%s\\WindowSettings\\%s", APP_REG_ROOT, subKeyName);

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

void startGenericWindow(const char* className, const char* title, const wchar_t* taskbarId, int defaultW, int defaultH, HINSTANCE hInst = NULL) {
    HWND& hWnd = g_AppWindows[className];
    
    if (hWnd && IsWindow(hWnd)) {
        ShowWindow(hWnd, SW_SHOW);
        SetForegroundWindow(hWnd);
        return;
    }

    int w = defaultW, h = defaultH;
    int x = (GetSystemMetrics(SM_CXSCREEN) - w) / 2;
    int y = (GetSystemMetrics(SM_CYSCREEN) - h) / 2;
    LoadWinPosition(className, x, y, w, h);
    
    if (hInst) {
        hWnd = CreateWindow(className, title, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, x, y, w, h, NULL, NULL, hInst, NULL);
        ShowWindow(hWnd, SW_SHOW);
        UpdateWindow(hWnd);
    } else {
        DWORD dwExStyle = WS_EX_APPWINDOW;
        if (className == DEBUGLOG_CLASS_NAME) {
             dwExStyle = WS_EX_TOPMOST;
        }
        if (className == BOOK_NEW_LIST_CLASS_NAME) {
             dwExStyle = WS_EX_DLGMODALFRAME | WS_EX_TOPMOST;
        }
        DWORD dwStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE;
        if (className == DEBUGLOG_CLASS_NAME) {
            dwStyle = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
        }
        HWND hWndParent = NULL;
        if (className == BOOK_NEW_LIST_CLASS_NAME) {
            dwStyle = WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE;
            hWndParent = g_AppWindows[BOOK_CLASS_NAME];;
        }
        hWnd = CreateWindowExA(dwExStyle, className, title, dwStyle, x, y, w, h, hWndParent, NULL, GetModuleHandle(NULL), NULL);   
    }
    
    if (className != BOOK_NEW_LIST_CLASS_NAME)
        SetWindowTaskbarId(hWnd, taskbarId);
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

std::unordered_map<std::string, HICON> offlineIcons;
std::unordered_map<std::string, HICON> onlineIcons;

void registerWindowClass(HINSTANCE hInst, WNDPROC WndProc, const char* className, int iconId) {
    HICON& offlineIcon = offlineIcons[className];
    HICON& onlineIcon  = onlineIcons[className];
    onlineIcon  = (HICON)LoadImage(hInst, MAKEINTRESOURCE(iconId), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
    offlineIcon = CreateGrayIcon(onlineIcon);
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.lpszClassName = className;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    if (className == DEBUGLOG_CLASS_NAME || className == BOOK_NEW_LIST_CLASS_NAME) {
        wc.hInstance = GetModuleHandle(NULL);
    } else {
        wc.hInstance = hInst;
    }
    wc.hIcon = offlineIcon;
    RegisterClass(&wc);
}

// ─── Window Session ───────────────────────────────────────────────────────────

void Settings_Save(const char* key, DWORD value) {
    HKEY hKey;
    char fullPath[256];
    wsprintf(fullPath, "%s\\Settings", APP_REG_ROOT);
    if (RegCreateKeyExA(HKEY_CURRENT_USER, fullPath, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        RegSetValueExA(hKey, key, 0, REG_DWORD, (const BYTE*)&value, sizeof(DWORD));
        RegCloseKey(hKey);
    }
}

DWORD Settings_Load(const char* key, DWORD defaultValue) {
    HKEY hKey;
    char fullPath[256];
    wsprintf(fullPath, "%s\\Settings", APP_REG_ROOT);
    DWORD value = defaultValue;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, fullPath, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD size = sizeof(DWORD);
        RegQueryValueExA(hKey, key, NULL, NULL, (LPBYTE)&value, &size);
        RegCloseKey(hKey);
    }
    return value;
}

bool Settings_DarkMode() {
    return Settings_Load("DarkMode", 0) != 0;
}

void ApplyDarkMode(HWND hWnd) {
    BOOL dark = Settings_DarkMode() ? TRUE : FALSE;
    DwmSetWindowAttribute(hWnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));
}

#include <atomic>
#include <string>
#include <queue>
#include <condition_variable>

void PlaySound_Async(int resourceId) {
    if (!Settings_Load("PlaySounds", 0)) return;

    // We use a static worker thread and a queue to ensure sounds play sequentially
    struct SoundQueue {
        std::queue<int> queue;
        std::mutex mutex;
        std::condition_variable cv;
        std::atomic<bool> running{true};
        std::thread worker;

        SoundQueue() {
            worker = std::thread([this]() {
                while (running) {
                    int resId = 0;
                    {
                        std::unique_lock<std::mutex> lock(mutex);
                        cv.wait(lock, [this] { return !queue.empty() || !running; });
                        if (!running) break;
                        resId = queue.front();
                        queue.pop();
                    }

                    // Actual playback logic
                    HRSRC hRes = FindResource(NULL, MAKEINTRESOURCE(resId), RT_RCDATA);
                    if (hRes) {
                        HGLOBAL hMem = LoadResource(NULL, hRes);
                        if (hMem) {
                            void* pData = LockResource(hMem);
                            DWORD size = SizeofResource(NULL, hRes);
                            if (pData && size) {
                                static std::atomic<int> soundSequence(0);
                                int currentSeq = ++soundSequence;

                                char tempPath[MAX_PATH];
                                GetTempPathA(MAX_PATH, tempPath);
                                std::string mp3File = std::string(tempPath) + "ib_snd_" + std::to_string(currentSeq) + ".mp3";
                                std::string alias = "mci_snd_" + std::to_string(currentSeq);

                                FILE* f = fopen(mp3File.c_str(), "wb");
                                if (f) {
                                    fwrite(pData, 1, size, f);
                                    fclose(f);

                                    std::string openCmd = "open \"" + mp3File + "\" type mpegvideo alias " + alias;
                                    std::string playCmd = "play " + alias + " from 0";
                                    mciSendStringA(openCmd.c_str(), NULL, 0, NULL);
                                    mciSendStringA(playCmd.c_str(), NULL, 0, NULL);

                                    // Wait for sound to finish (approximate) or use a fixed delay
                                    // Since we are in a dedicated worker thread, we can afford to sleep
                                    std::this_thread::sleep_for(std::chrono::seconds(3));
                                    
                                    std::string closeCmd = "close " + alias;
                                    mciSendStringA(closeCmd.c_str(), NULL, 0, NULL);
                                    DeleteFileA(mp3File.c_str());
                                }
                            }
                        }
                    }
                }
            });
        }
        ~SoundQueue() {
            running = false;
            cv.notify_all();
            if (worker.joinable()) worker.join();
        }
    };

    static SoundQueue sq;
    {
        std::lock_guard<std::mutex> lock(sq.mutex);
        sq.queue.push(resourceId);
    }
    sq.cv.notify_one();
}

void Session_AddWindow(HWND hWnd) {
    ApplyDarkMode(hWnd);

    char className[256] = {};
    GetClassNameA(hWnd, className, sizeof(className));
    // Load existing list
    HKEY hKey;
    char fullPath[256];
    wsprintf(fullPath, "%s\\Settings", APP_REG_ROOT);
    std::vector<std::string> windows;

    if (RegOpenKeyExA(HKEY_CURRENT_USER, fullPath, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD size = 0;
        RegQueryValueExA(hKey, "OpenWindows", NULL, NULL, NULL, &size);
        if (size > 0) {
            std::vector<char> buf(size);
            RegQueryValueExA(hKey, "OpenWindows", NULL, NULL, (LPBYTE)buf.data(), &size);
            const char* p = buf.data();
            while (*p) {
                if (strcmp(p, className) != 0) // avoid duplicates
                    windows.push_back(p);
                p += strlen(p) + 1;
            }
        }
        RegCloseKey(hKey);
    }

    windows.push_back(className);

    // Save back
    std::string multiStr;
    for (const auto& w : windows) { multiStr += w; multiStr += '\0'; }
    multiStr += '\0';

    if (RegCreateKeyExA(HKEY_CURRENT_USER, fullPath, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        RegSetValueExA(hKey, "OpenWindows", 0, REG_MULTI_SZ,
            (const BYTE*)multiStr.data(), (DWORD)multiStr.size());
        RegCloseKey(hKey);
    }
}

void Session_RemoveWindow(HWND hWnd) {
    char className[256] = {};
    GetClassNameA(hWnd, className, sizeof(className));
    
    HKEY hKey;
    char fullPath[256];
    wsprintf(fullPath, "%s\\Settings", APP_REG_ROOT);
    std::vector<std::string> windows;

    if (RegOpenKeyExA(HKEY_CURRENT_USER, fullPath, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD size = 0;
        RegQueryValueExA(hKey, "OpenWindows", NULL, NULL, NULL, &size);
        if (size > 0) {
            std::vector<char> buf(size);
            RegQueryValueExA(hKey, "OpenWindows", NULL, NULL, (LPBYTE)buf.data(), &size);
            const char* p = buf.data();
            while (*p) {
                if (strcmp(p, className) != 0)
                    windows.push_back(p);
                p += strlen(p) + 1;
            }
        }
        RegCloseKey(hKey);
    }

    std::string multiStr;
    for (const auto& w : windows) { multiStr += w; multiStr += '\0'; }
    multiStr += '\0';

    if (RegCreateKeyExA(HKEY_CURRENT_USER, fullPath, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        RegSetValueExA(hKey, "OpenWindows", 0, REG_MULTI_SZ,
            (const BYTE*)multiStr.data(), (DWORD)multiStr.size());
        RegCloseKey(hKey);
    }

    g_AppWindows[className] = NULL;
}

void Session_RestoreWindows(
    const std::function<void()>& startBook,
    const std::function<void()>& startCoins,
    const std::function<void()>& startDiamonds,
    const std::function<void()>& startNews,
    const std::function<void()>& startSettings,
    const std::function<void()>& startTimesales,
    const std::function<void()>& startLevels,
    const std::function<void()>& startTicker,
    const std::function<void()>& startOrders
) {
    HKEY hKey;
    char fullPath[256];
    wsprintf(fullPath, "%s\\Settings", APP_REG_ROOT);
    if (RegOpenKeyExA(HKEY_CURRENT_USER, fullPath, 0, KEY_READ, &hKey) != ERROR_SUCCESS) return;

    DWORD size = 0;
    RegQueryValueExA(hKey, "OpenWindows", NULL, NULL, NULL, &size);
    if (size == 0) { RegCloseKey(hKey); return; }

    std::vector<char> buf(size);
    RegQueryValueExA(hKey, "OpenWindows", NULL, NULL, (LPBYTE)buf.data(), &size);
    RegCloseKey(hKey);

    const char* p = buf.data();
    while (*p) {
        std::string cls = p;
        if      (cls == "TNTBookWindowClass")      startBook();
        else if (cls == "TNTCoinsWindowClass")     startCoins();
        else if (cls == "TNTDiamondsWindowClass")  startDiamonds();
        else if (cls == "TNTNewsWindowClass")      startNews();
        else if (cls == "TNTSettingsWindowClass")  startSettings();
        else if (cls == "TNTTimesalesWindowClass") startTimesales();
        else if (cls == "TNTLevelsWindowClass")    startLevels();
        else if (cls == "TNTTickerWindowClass")    startTicker();
        else if (cls == "TNTOrdersWindowClass")    startOrders();
        p += strlen(p) + 1;
    }
}

LRESULT HandleDarkModeMessages(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_ERASEBKGND: {
            HDC hdc = (HDC)wParam;
            RECT rc;
            GetClientRect(hWnd, &rc);
            FillRect(hdc, &rc, Settings_DarkMode() ? hDarkBrush : (HBRUSH)(COLOR_BTNFACE + 1));
            return 1;
        }
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORLISTBOX: {
            if (!Settings_DarkMode()) return 0;
            SetTextColor((HDC)wParam, DM_TEXT);
            SetBkColor((HDC)wParam, DM_BG2);
            return (LRESULT)hDarkBrush2;
        }
        case WM_CTLCOLORSTATIC: {
            if (!Settings_DarkMode()) return 0;
            SetTextColor((HDC)wParam, DM_TEXT);
            SetBkColor((HDC)wParam, DM_BG);
            return (LRESULT)hDarkBrush;
        }
        case WM_CTLCOLORBTN: {
            if (!Settings_DarkMode()) return 0;
            SetTextColor((HDC)wParam, DM_TEXT);
            SetBkColor((HDC)wParam, DM_BG);
            return (LRESULT)hDarkBrush;
        }
        case WM_DRAWITEM: {
            DRAWITEMSTRUCT* dis = (DRAWITEMSTRUCT*)lParam;
            if (dis->CtlType != ODT_BUTTON) return 0;
            bool dark = Settings_DarkMode();
            bool pressed  = (dis->itemState & ODS_SELECTED);
            bool disabled = (dis->itemState & ODS_DISABLED);
            COLORREF bgColor     = dark ? (pressed ? RGB(60,60,60) : RGB(50,50,50))
                                        : (pressed ? RGB(180,180,180) : RGB(225,225,225));
            COLORREF textColor   = disabled ? RGB(120,120,120) : (dark ? DM_TEXT : LM_TEXT);
            COLORREF borderColor = dark ? DM_BORDER : RGB(170,170,170);
            HBRUSH hBg = CreateSolidBrush(bgColor);
            FillRect(dis->hDC, &dis->rcItem, hBg);
            DeleteObject(hBg);
            HPEN hPen = CreatePen(PS_SOLID, 1, borderColor);
            HPEN hOld = (HPEN)SelectObject(dis->hDC, hPen);
            SelectObject(dis->hDC, GetStockObject(NULL_BRUSH));
            Rectangle(dis->hDC, dis->rcItem.left, dis->rcItem.top,
                    dis->rcItem.right, dis->rcItem.bottom);
            SelectObject(dis->hDC, hOld);
            DeleteObject(hPen);
            wchar_t text[128] = {};
            GetWindowTextW(dis->hwndItem, text, sizeof(text)/sizeof(wchar_t));
            SetTextColor(dis->hDC, textColor);
            SetBkMode(dis->hDC, TRANSPARENT);
            DrawTextW(dis->hDC, text, -1, &dis->rcItem, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            if (dis->itemState & ODS_FOCUS)
                DrawFocusRect(dis->hDC, &dis->rcItem);
            return TRUE;
        }
    }
    return 0;
}


LRESULT HandleCommonMessages(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    char className[256] = {};
    GetClassNameA(hWnd, className, sizeof(className));

    switch (message) {
        case WM_CREATE:
            if (className != DASHBOARD_CLASS_NAME) 
                Session_AddWindow(hWnd);
            return 0;
        case WM_CLOSE:
            if (className == DASHBOARD_CLASS_NAME) 
                ShowWindow(hWnd, SW_HIDE);
            else
                DestroyWindow(hWnd);
            return 0;
        case WM_MOVE:
            SaveWinPosition(hWnd);
            return 0;
        case WM_DESTROY:
            SaveWinPosition(hWnd);
            if (className == DASHBOARD_CLASS_NAME) {
                Shell_NotifyIcon(NIM_DELETE, &nid);
                PostQuitMessage(0);
            } else {
                Session_RemoveWindow(hWnd);
            }
            return 0;
        default: {
            LRESULT res = HandleDarkModeMessages(hWnd, message, wParam, lParam);
            if (res) return res;
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }
    return 0;
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

std::string GetGatewayPath() {
    // 1. Try registry first
    HKEY hKey;
    char fullPath[256];
    wsprintf(fullPath, "%s\\Settings", APP_REG_ROOT);
    if (RegOpenKeyExA(HKEY_CURRENT_USER, fullPath, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        char path[MAX_PATH] = {};
        DWORD size = sizeof(path);
        if (RegQueryValueExA(hKey, "GatewayPath", NULL, NULL,
            (LPBYTE)path, &size) == ERROR_SUCCESS && strlen(path) > 0) {
            RegCloseKey(hKey);
            return std::string(path);
        }
        RegCloseKey(hKey);
    }
    return "";
}

void SaveGatewayPath(const std::string& path) {
    HKEY hKey;
    char fullPath[256];
    wsprintf(fullPath, "%s\\Settings", APP_REG_ROOT);
    if (RegCreateKeyExA(HKEY_CURRENT_USER, fullPath, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        RegSetValueExA(hKey, "GatewayPath", 0, REG_SZ,
            (const BYTE*)path.c_str(), (DWORD)path.size() + 1);
        RegCloseKey(hKey);
    }
}

std::string AskGatewayPath(HWND hParent) {
    OPENFILENAMEA ofn = {};
    char path[MAX_PATH] = "ibgateway.exe";

    ofn.lStructSize     = sizeof(ofn);
    ofn.hwndOwner       = hParent;
    ofn.lpstrFilter     = "Executable\0*.exe\0All Files\0*.*\0";
    ofn.lpstrFile       = path;
    ofn.nMaxFile        = sizeof(path);
    ofn.lpstrTitle      = "Locate ibgateway.exe";
    ofn.lpstrInitialDir = "C:\\Program Files\\IBKR";
    ofn.Flags           = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

    if (GetOpenFileNameA(&ofn))
        return std::string(path);

    return "";
}
bool alreadyEnsureGatewayRunning = false;
void EnsureGatewayRunning(HWND hParent) {
    if (alreadyEnsureGatewayRunning ||IsProcessRunning("ibgateway.exe")) return;
    alreadyEnsureGatewayRunning = true;

    // Get path from registry or default
    std::string path = GetGatewayPath();

    // Check default location if registry is empty
    if (path.empty()) {
        const char* defaultPath = "C:\\whatever\\ibgateway.exe";
        if (GetFileAttributesA(defaultPath) != INVALID_FILE_ATTRIBUTES) {
            path = defaultPath;
            SaveGatewayPath(path);
        }
    }

    // If path from registry doesn't exist anymore, ask user
    if (path.empty() || GetFileAttributesA(path.c_str()) == INVALID_FILE_ATTRIBUTES) {
        MessageBoxA(hParent,
            "IB Gateway not found. Please locate ibgateway.exe.",
            "Gateway Not Found", MB_OK | MB_ICONINFORMATION);
        path = AskGatewayPath(hParent);
        if (path.empty()) return; // user cancelled
        SaveGatewayPath(path);
    }

    alreadyEnsureGatewayRunning = false;

    LogDebug("Running IBKR Gateway, please login..");
    ShellExecuteA(NULL, "open", path.c_str(), NULL, NULL, SW_SHOW);
}

void KillGateway() {
    std::string path = GetGatewayPath();
    if (path.empty()) {
        path = "C:\\whatever\\ibgateway.exe";
    }

    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) return;

    PROCESSENTRY32 pe = { sizeof(PROCESSENTRY32) };
    if (Process32First(hSnap, &pe)) {
        do {
            if (_stricmp(pe.szExeFile, std::filesystem::path(path).filename().string().c_str()) == 0) {
                HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
                if (hProcess) {
                    TerminateProcess(hProcess, 0);
                    CloseHandle(hProcess);
                }
            }
        } while (Process32Next(hSnap, &pe));
    }
    CloseHandle(hSnap);
}

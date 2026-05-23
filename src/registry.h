#pragma once

#include <dwmapi.h>
#include <functional>

constexpr const char* APP_REG_ROOT = "Software\\ibkr-gateway-trading-floor";

static const char* SETTINGS_CLASS_NAME      = "TNTSettingsWindowClass";
static const char* DEBUGLOG_CLASS_NAME      = "TNTSettingsDebugLogWindowClass";
static const char* BOOK_CLASS_NAME          = "TNTBookWindowClass";
static const char* BOOK_NEW_LIST_CLASS_NAME = "TNTBookNewListWindowClass";
static const char* COINS_CLASS_NAME         = "TNTCoinsWindowClass";
static const char* DASHBOARD_CLASS_NAME     = "TNTDashboardClass";
static const char* ORDERS_CLASS_NAME        = "TNTOrdersWindowClass";
static const char* LEVELS_CLASS_NAME        = "TNTLevelsWindowClass";
static const char* NEWS_CLASS_NAME          = "TNTNewsWindowClass";
static const char* NEWS_ARTICLE_CLASS_NAME  = "TNTNewsArticleWindowClass";
static const char* DIAMONDS_CLASS_NAME      = "TNTDiamondsWindowClass";
static const char* TICKER_CLASS_NAME        = "TNTTickerWindowClass";
static const char* TIMESALES_CLASS_NAME     = "TNTTimesalesWindowClass";

void Settings_SaveString(const char* key, const std::string& value) {
    HKEY hKey;
    char fullPath[256];
    wsprintf(fullPath, "%s\\Settings", APP_REG_ROOT);
    if (RegCreateKeyExA(HKEY_CURRENT_USER, fullPath, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        RegSetValueExA(hKey, key, 0, REG_SZ,
            (const BYTE*)value.c_str(), (DWORD)value.size() + 1);
        RegCloseKey(hKey);
    }
}

std::string Settings_LoadString(const char* key, const std::string& defaultValue = "") {
    HKEY hKey;
    char fullPath[256];
    wsprintf(fullPath, "%s\\Settings", APP_REG_ROOT);
    if (RegOpenKeyExA(HKEY_CURRENT_USER, fullPath, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        char buf[512] = {};
        DWORD size = sizeof(buf);
        if (RegQueryValueExA(hKey, key, NULL, NULL, (LPBYTE)buf, &size) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return std::string(buf);
        }
        RegCloseKey(hKey);
    }
    return defaultValue;
}

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

bool Settings_KillGatewayOnExit() {
    return Settings_Load("KillGatewayOnExit", 0) != 0;
}

bool Settings_AutoGateway() {
    return Settings_Load("AutoGateway", 0) != 0;
}

bool Settings_DarkMode() {
    return Settings_Load("DarkMode", 0) != 0;
}

void ApplyDarkMode(HWND hWnd) {
    BOOL dark = Settings_DarkMode() ? TRUE : FALSE;
    DwmSetWindowAttribute(hWnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));
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
    const std::function<void()>& startOrders,
    const std::function<void()>& startDebugLog
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
        if      (cls == BOOK_CLASS_NAME)      startBook();
        else if (cls == COINS_CLASS_NAME)     startCoins();
        else if (cls == DIAMONDS_CLASS_NAME)  startDiamonds();
        else if (cls == NEWS_CLASS_NAME)      startNews();
        else if (cls == SETTINGS_CLASS_NAME)  startSettings();
        else if (cls == TIMESALES_CLASS_NAME) startTimesales();
        else if (cls == LEVELS_CLASS_NAME)    startLevels();
        else if (cls == TICKER_CLASS_NAME)    startTicker();
        else if (cls == ORDERS_CLASS_NAME)    startOrders();
        else if (cls == DEBUGLOG_CLASS_NAME)  startDebugLog();
        p += strlen(p) + 1;
    }
}
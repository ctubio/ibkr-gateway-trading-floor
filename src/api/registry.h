#pragma once

#include <dwmapi.h>
#include <functional>
#include <vector>
#include <string>

constexpr const char* APP_REG_ROOT = "Software\\ibkr-gateway-trading-floor";

static const char* DASHBOARD_CLASS_NAME          = "TNTDashboardClass";
static const char* SETTINGS_CLASS_NAME           = "TNTSettingsWindowClass";
static const char* DEBUGLOG_CLASS_NAME           = "TNTDebugLogWindowClass";
static const char* WATCHLIST_NEW_LIST_CLASS_NAME = "TNTWatchlistNewListWindowClass";
static const char* COINS_CLASS_NAME              = "TNTCoinsWindowClass";
static const char* ORDERS_CLASS_NAME             = "TNTOrdersWindowClass";
static const char* NEWS_CLASS_NAME               = "TNTNewsWindowClass";
static const char* NEWS_ARTICLE_CLASS_NAME       = "TNTNewsArticleWindowClass";
static const char* DIAMONDS_CLASS_NAME           = "TNTDiamondsWindowClass";
static const char* WATCHLIST_CLASS_NAME          = "TNTWatchlistWindowClass";
static const char* MARKET_CLASS_NAME             = "TNTMarketWindowClass";
static const char* MARKET_SEARCH_CLASS_NAME      = "TNTTsSearchWindowClass";

// Dark mode colors
#define DM_BG        RGB(30,  30,  30)   // Slightly darker, flatter background
#define DM_BG2       RGB(42,  42,  42)   // Header/Alternate row background
#define DM_TEXT      RGB(230, 230, 230)  // Crisp, slightly off-white text
#define DM_BORDER    RGB(65,  65,  65)   // Outer borders
#define DM_SEPARATOR RGB(85,  85,  85)   // Subtle inner column dividers

// Light mode colors  
#define LM_BG        GetSysColor(COLOR_BTNFACE)
#define LM_TEXT      GetSysColor(COLOR_WINDOWTEXT)

HBRUSH hDarkBrush = NULL;
HBRUSH hDarkBrush2 = NULL;

static std::vector<std::string> debugBuffer; // stores messages when window is closed

void LogDebug(const std::string& msg) {
    time_t now = time(0);
    char tstr[26] = {};
    ctime_s(tstr, sizeof(tstr), &now);
    std::string timestamp = tstr;
    if (!timestamp.empty()) timestamp.pop_back();

    std::string fullMsg = "[" + timestamp + "] " + msg + "\r\n";
    debugBuffer.push_back(fullMsg);

    HWND hLogWnd = FindWindowA(DEBUGLOG_CLASS_NAME, NULL);
    if (hLogWnd && IsWindow(hLogWnd)) {
        HWND hDebugEdit = (HWND)GetPropA(hLogWnd, "hDebugEdit");
        if (hDebugEdit && IsWindow(hDebugEdit)) {
            // Append to edit control
            int len = GetWindowTextLength(hDebugEdit);
            SendMessage(hDebugEdit, EM_SETSEL, len, len);
            SendMessageA(hDebugEdit, EM_REPLACESEL, FALSE, (LPARAM)fullMsg.c_str());
            // Auto-scroll to bottom
            SendMessage(hDebugEdit, EM_SCROLLCARET, 0, 0);
        }
    }
}

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
        char buf[2048] = {}; // Increased buffer to handle many saved windows
        DWORD size = sizeof(buf);
        if (RegQueryValueExA(hKey, key, NULL, NULL, (LPBYTE)buf, &size) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return std::string(buf);
        }
        RegCloseKey(hKey);
    }
    return defaultValue;
}

void Settings_SaveMarket(const std::vector<std::string>& sessions) {
    std::string combined;
    for (size_t i = 0; i < sessions.size(); ++i) {
        if (i > 0) combined += " ";
        combined += sessions[i];
    }
    Settings_SaveString("OpenMarket", combined);
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

void Settings_Delete(const char* key) {
    HKEY hKey;
    char fullPath[256];
    wsprintf(fullPath, "%s\\Settings", APP_REG_ROOT);
    if (RegOpenKeyExA(HKEY_CURRENT_USER, fullPath, 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
        RegDeleteValueA(hKey, key);
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

void SaveWinPosition(HWND hWnd) {
    WINDOWPLACEMENT wp;
    wp.length = sizeof(WINDOWPLACEMENT);
    GetWindowPlacement(hWnd, &wp);

    DWORD x = (DWORD)wp.rcNormalPosition.left;
    DWORD y = (DWORD)wp.rcNormalPosition.top;
    DWORD w = (DWORD)(wp.rcNormalPosition.right - wp.rcNormalPosition.left);
    DWORD h = (DWORD)(wp.rcNormalPosition.bottom - wp.rcNormalPosition.top);
    
    HKEY hKey;
    HANDLE hProp = GetPropA(hWnd, "WinPositionKey");
    std::string winKey;
    if (hProp) {
        winKey = *(std::string*)hProp; 
    } else {
        char className[256] = {};
        GetClassNameA(hWnd, className, sizeof(className));
        winKey = className;
    }
    char fullPath[256];
    wsprintf(fullPath, "%s\\WindowSettings\\%s", APP_REG_ROOT, winKey.c_str());

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

struct EnumContext {
    HWND targetHwnd;
    const char* targetClassName;
    bool foundOther;
};

BOOL CALLBACK FindEnumWindowsProc(HWND hwnd, LPARAM lParam) {
    EnumContext* ctx = (EnumContext*)lParam;

    if (hwnd == ctx->targetHwnd) return TRUE;

    char className[256];
    GetClassNameA(hwnd, className, sizeof(className));

    if (strcmp(className, ctx->targetClassName) == 0 && IsWindowVisible(hwnd)) {
        ctx->foundOther = true;
        return FALSE; // Stop enumerating, we found what we needed
    }

    return TRUE; // Continue enumerating
}

LRESULT CALLBACK ListViewSubclassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    if (msg == WM_NOTIFY) {
        NMHDR* hdr = (NMHDR*)lParam;
        
        // The Header control sends paint messages (NM_CUSTOMDRAW) to its parent (the ListView)
        if (hdr->code == NM_CUSTOMDRAW && hdr->hwndFrom == ListView_GetHeader(hWnd)) {
            if (!Settings_DarkMode()) return DefSubclassProc(hWnd, msg, wParam, lParam);

            NMCUSTOMDRAW* cd = (NMCUSTOMDRAW*)lParam;
            switch (cd->dwDrawStage) {
                case CDDS_PREPAINT: {
                    // Paint the entire background first to cover the empty space on the far right
                    RECT rcClient;
                    GetClientRect(hdr->hwndFrom, &rcClient);
                    FillRect(cd->hdc, &rcClient, hDarkBrush2);
                    
                    // Tell Windows we want to draw the individual columns next
                    return CDRF_NOTIFYITEMDRAW; 
                }
                case CDDS_ITEMPREPAINT: {
                    HDC hdc = cd->hdc;
                    RECT rc = cd->rc;

                    // 1. Draw modern, subtle separators
                    HPEN hPen = CreatePen(PS_SOLID, 1, DM_SEPARATOR);
                    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
                    
                    // Draw a vertical divider, inset by 4 pixels (doesn't touch top/bottom edges)
                    MoveToEx(hdc, rc.right - 1, rc.top + 4, NULL);
                    LineTo(hdc, rc.right - 1, rc.bottom - 4);

                    // Draw a single subtle bottom border under the header
                    HPEN hBotPen = CreatePen(PS_SOLID, 1, DM_BORDER);
                    SelectObject(hdc, hBotPen);
                    MoveToEx(hdc, rc.left, rc.bottom - 1, NULL);
                    LineTo(hdc, rc.right, rc.bottom - 1);

                    SelectObject(hdc, hOldPen);
                    DeleteObject(hPen);
                    DeleteObject(hBotPen);

                    // 2. Get the Column Text and its Alignment
                    char text[128] = {0};
                    HDITEMA hdi = {0};
                    hdi.mask = HDI_TEXT | HDI_FORMAT;
                    hdi.pszText = text;
                    hdi.cchTextMax = sizeof(text);
                    SendMessageA(hdr->hwndFrom, HDM_GETITEMA, cd->dwItemSpec, (LPARAM)&hdi);

                    // 3. Draw the Text (Centered nicely)
                    SetTextColor(hdc, DM_TEXT);
                    SetBkMode(hdc, TRANSPARENT);
                    
                    UINT format = DT_VCENTER | DT_SINGLELINE;
                    if (hdi.fmt & HDF_CENTER) { format |= DT_CENTER; }
                    else if (hdi.fmt & HDF_RIGHT) { format |= DT_RIGHT; rc.right -= 8; }
                    else { format |= DT_LEFT; rc.left += 8; } // Pad left/right so text doesn't touch separators

                    DrawTextA(hdc, text, -1, &rc, format);

                    // Tell Windows to skip its default light-mode rendering
                    return CDRF_SKIPDEFAULT; 
                }
            }
        }
    }

    if (msg == WM_NCPAINT) {
        // Let Windows paint the NC area first (scrollbars etc.)
        LRESULT res = DefSubclassProc(hWnd, msg, wParam, lParam);

        if (!Settings_DarkMode()) return res;

        // Now overdraw just the border with our dark colour.
        // GetWindowDC covers the full window rect including NC area.
        HDC hdc = GetWindowDC(hWnd);
        if (hdc) {
            RECT rcWin;
            GetWindowRect(hWnd, &rcWin);
            // Convert to window-relative coordinates (top-left = 0,0)
            OffsetRect(&rcWin, -rcWin.left, -rcWin.top);

            // WS_EX_CLIENTEDGE draws a 2-pixel sunken border; overdraw it.
            HPEN hPen  = CreatePen(PS_SOLID, 1, DM_BORDER);
            HPEN hOld  = (HPEN)SelectObject(hdc, hPen);
            HBRUSH hBr = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));

            // Outer edge
            Rectangle(hdc, rcWin.left, rcWin.top, rcWin.right, rcWin.bottom);
            // Inner edge (WS_EX_CLIENTEDGE is 2px wide)
            Rectangle(hdc, rcWin.left + 1, rcWin.top + 1, rcWin.right - 1, rcWin.bottom - 1);

            SelectObject(hdc, hOld);
            SelectObject(hdc, hBr);
            DeleteObject(hPen);
            ReleaseDC(hWnd, hdc);
        }
        return res;
    }

    // Clean up subclass on destroy
    if (msg == WM_NCDESTROY) {
        RemoveWindowSubclass(hWnd, ListViewSubclassProc, uIdSubclass);
    }
    
    return DefSubclassProc(hWnd, msg, wParam, lParam);
}

// Applies dark theme to a specific SysListView32
void ApplyDarkModeToLists(HWND hList, bool dark) {
    // Safely attach our Custom Draw subclass (SetWindowSubclass ignores duplicate calls automatically)
    SetWindowSubclass(hList, ListViewSubclassProc, 999, 0);

    if (dark) {
        // Theme scrollbars
        SetWindowTheme(hList, L"DarkMode_Explorer", NULL);
        
        // Set listview background and text colors
        ListView_SetBkColor(hList, DM_BG);
        ListView_SetTextBkColor(hList, DM_BG);
        ListView_SetTextColor(hList, DM_TEXT);
    } else {
        // Revert to default light mode themes
        SetWindowTheme(hList, L"Explorer", NULL);
        
        ListView_SetBkColor(hList, GetSysColor(COLOR_WINDOW));
        ListView_SetTextBkColor(hList, GetSysColor(COLOR_WINDOW));
        ListView_SetTextColor(hList, GetSysColor(COLOR_WINDOWTEXT));
    }
}

// Callback to find listviews and apply the theme
BOOL CALLBACK EnumChildProcForLists(HWND hwnd, LPARAM lParam) {
    char className[256];
    if (GetClassNameA(hwnd, className, sizeof(className))) {
        if (strcmp(className, "SysListView32") == 0) {
            bool dark = (bool)lParam;
            ApplyDarkModeToLists(hwnd, dark);
        }
    }
    return TRUE;
}

void ApplyDarkMode(HWND hWnd) {
    BOOL dark = Settings_DarkMode() ? TRUE : FALSE;
    DwmSetWindowAttribute(hWnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));

    // Automatically find and theme any ListViews inside this window
    EnumChildWindows(hWnd, EnumChildProcForLists, (LPARAM)dark);
    
    DWM_WINDOW_CORNER_PREFERENCE preference = DWMWCP_ROUND;
    DwmSetWindowAttribute(hWnd, DWMWA_WINDOW_CORNER_PREFERENCE, &preference, sizeof(preference));
    
    char className[256] = {};
    GetClassNameA(hWnd, className, sizeof(className));
    if (strcmp(className, DASHBOARD_CLASS_NAME) == 0) {
        // Set backdrop type
        // DWMSBT_MAINWINDOW    = Mica
        // DWMSBT_TABBEDWINDOW  = Mica Alt (a darker/more intense version)
        // DWMSBT_TRANSIENTWINDOW = Acrylic
        DWM_SYSTEMBACKDROP_TYPE backdropType = DWMSBT_TRANSIENTWINDOW;
        DwmSetWindowAttribute(hWnd, DWMWA_SYSTEMBACKDROP_TYPE, &backdropType, sizeof(backdropType));
    }
}

void Session_AddWindow(HWND hWnd) {
    ApplyDarkMode(hWnd);

    char className[256] = {};
    GetClassNameA(hWnd, className, sizeof(className));

    if (strcmp(className, DASHBOARD_CLASS_NAME) == 0) return; // Dashboard is always open on boot, no need to track in registry
    
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
    
    EnumContext ctx = { hWnd, className, false };
    EnumWindows(FindEnumWindowsProc, (LPARAM)&ctx);
    if (ctx.foundOther) return;
    
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

void Watchlist_DeleteList(const char* listName) {
    HKEY hKey;
    char fullPath[256];
    wsprintf(fullPath, "%s\\Watchlist", APP_REG_ROOT);
    if (RegOpenKeyExA(HKEY_CURRENT_USER, fullPath, 0, KEY_WRITE, &hKey) == ERROR_SUCCESS) {
        RegDeleteValueA(hKey, listName);
        RegCloseKey(hKey);
    }
}

void Watchlist_SaveFullList(const char* listName, const std::vector<std::string>& items) {
    std::string multiStr;
    for (const auto& item : items) {
        multiStr += item;
        multiStr += '\0';
    }
    multiStr += '\0';

    HKEY hKey;
    char fullPath[256];
    wsprintf(fullPath, "%s\\Watchlist", APP_REG_ROOT);
    if (RegCreateKeyExA(HKEY_CURRENT_USER, fullPath, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        RegSetValueExA(hKey, listName, 0, REG_MULTI_SZ,
            (const BYTE*)multiStr.data(), (DWORD)multiStr.size());
        RegCloseKey(hKey);
    }
}

// Returns every list name stored under the Watchlist registry key.
// Shared by Watchlist_LoadAllLists (combo box) and any caller that only needs the names.
std::vector<std::string> Watchlist_LoadAllListNames() {
    std::vector<std::string> names;
    HKEY hKey;
    char fullPath[256];
    wsprintf(fullPath, "%s\\Watchlist", APP_REG_ROOT);
    if (RegOpenKeyExA(HKEY_CURRENT_USER, fullPath, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
        return names;
    char valueName[256];
    DWORD index = 0, nameSize = sizeof(valueName);
    while (RegEnumValueA(hKey, index++, valueName, &nameSize,
                         NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
        names.push_back(valueName);
        nameSize = sizeof(valueName);
    }
    RegCloseKey(hKey);
    return names;
}

std::vector<std::string> Watchlist_ReadListEntries(const char* listName) {
    std::vector<std::string> entries;
    HKEY hKey;
    char fullPath[256];
    wsprintf(fullPath, "%s\\Watchlist", APP_REG_ROOT);
    if (RegOpenKeyExA(HKEY_CURRENT_USER, fullPath, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
        return entries;
    DWORD type, size = 0;
    RegQueryValueExA(hKey, listName, NULL, &type, NULL, &size);
    if (size > 0) {
        std::vector<char> buf(size);
        RegQueryValueExA(hKey, listName, NULL, &type, (LPBYTE)buf.data(), &size);
        const char* p = buf.data();
        while (*p) { entries.push_back(p); p += strlen(p) + 1; }
    }
    RegCloseKey(hKey);
    return entries;
}

// Check if a window is currently set to Always On Top
// For single-instance windows (classNameOnly = true), checks the first window of that class
// For multi-instance windows (classNameOnly = false), checks the window with the specific title/identifier
HWND IsWindowAlwaysOnTop(const char* windowClassName, const char* windowIdentifier = nullptr) {
    HWND hWnd = windowIdentifier ? 
        FindWindowA(windowClassName, windowIdentifier) : 
        FindWindowA(windowClassName, NULL);
    
    if (!hWnd) {
        return NULL; // Window not found
    }

    LONG_PTR exStyle = GetWindowLongPtr(hWnd, GWL_EXSTYLE);
    
    if (exStyle & WS_EX_TOPMOST) {
        return hWnd;
    }
    return NULL;
}

// Toggle Always On Top state for a window and save the preference
// For single-instance windows: className only
// For Market windows: className and symbol (e.g., "MSFT")
void ToggleWindowAlwaysOnTop(const char* windowClassName, const char* windowIdentifier = nullptr) {
    HWND hWnd = windowIdentifier ? 
        FindWindowA(windowClassName, windowIdentifier) : 
        FindWindowA(windowClassName, NULL);
    
    if (!hWnd) {
        return; // Window not found
    }
    
    LONG_PTR exStyle = GetWindowLongPtr(hWnd, GWL_EXSTYLE);
    bool isCurrentlyOnTop = (exStyle & WS_EX_TOPMOST) != 0;
    
    if (isCurrentlyOnTop) {
        // Currently always on top, remove it
        SetWindowPos(hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        // Save state: not on top
        char key[256];
        if (windowIdentifier && strlen(windowIdentifier) > 0) {
            sprintf(key, "AlwaysOnTop_%s_%s", windowClassName, windowIdentifier);
        } else {
            sprintf(key, "AlwaysOnTop_%s", windowClassName);
        }
        Settings_Save(key, 0);
    } else {
        // Not always on top, make it so
        SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        // Save state: on top
        char key[256];
        if (windowIdentifier && strlen(windowIdentifier) > 0) {
            sprintf(key, "AlwaysOnTop_%s_%s", windowClassName, windowIdentifier);
        } else {
            sprintf(key, "AlwaysOnTop_%s", windowClassName);
        }
        if (IsIconic(hWnd)) {
            ShowWindow(hWnd, SW_RESTORE);
        } else {
            ShowWindow(hWnd, SW_SHOW);
        }
        Settings_Save(key, 1);
    }
}

// Enumerate all open Market windows and extract their symbols
struct MarketWindowInfo {
    HWND hWnd;
    std::string symbol;
};

static std::vector<MarketWindowInfo> EnumerateMarketWindows() {
    std::vector<MarketWindowInfo> result;
    
    HWND hWnd = FindWindowA(MARKET_CLASS_NAME, NULL);
    while (hWnd) {
        // Get the window title
        char title[256] = {};
        GetWindowTextA(hWnd, title, sizeof(title));
        
        // Extract symbol from title (format: "Market: SYMBOL")
        std::string titleStr = title;
        std::string winTitle = "Market: ";
        size_t pos = titleStr.find(winTitle);
        if (pos != std::string::npos) {
            std::string symbol = titleStr.substr(pos + winTitle.length());
            result.push_back({hWnd, symbol});
        }
        
        // Find next window of the same class with a different title
        hWnd = FindWindowExA(NULL, hWnd, MARKET_CLASS_NAME, NULL);
    }
    
    return result;
}

// Check if a specific Market window is set to Always On Top
bool IsMarketAlwaysOnTop(const std::string& symbol) {
    char key[256];
    sprintf(key, "AlwaysOnTop_%s_%s", MARKET_CLASS_NAME, symbol.c_str());
    return Settings_Load(key, 0) != 0;
}

// Toggle Always On Top for a specific Market window by symbol
// Uses consistent registry key format based on symbol only
void ToggleMarketAlwaysOnTop(const std::string& symbol) {
    std::string fullTitle = "Market: " + symbol;
    HWND hWnd = FindWindowA(MARKET_CLASS_NAME, fullTitle.c_str());
    
    if (!hWnd) {
        return; // Window not found
    }
    
    LONG_PTR exStyle = GetWindowLongPtr(hWnd, GWL_EXSTYLE);
    bool isCurrentlyOnTop = (exStyle & WS_EX_TOPMOST) != 0;
    
    char key[256];
    sprintf(key, "AlwaysOnTop_%s_%s", MARKET_CLASS_NAME, symbol.c_str());
    
    if (isCurrentlyOnTop) {
        // Currently always on top, remove it
        SetWindowPos(hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        Settings_Save(key, 0);
    } else {
        // Not always on top, make it so
        SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        Settings_Save(key, 1);
    }
}

// Set Always On Top state for a Market window using its HWND directly.
// Called at restore time — avoids a FindWindowA-by-title race where the window
// title may not yet be set when StartMarket() returns.
void SetMarketAlwaysOnTop(HWND hWnd, bool onTop) {
    if (!hWnd) return;
    SetWindowPos(hWnd, onTop ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
}

// ── Market splitter persistence ───────────────────────────────────────────
// splitX / splitY are stored as DWORD scaled ×10000 so floats survive REG_DWORD.
// Keys are per-symbol so multiple open windows never collide.

void Settings_SaveMarketSplitter(const std::string& symbol, float splitX, float splitY) {
    char keyX[256], keyY[256];
    sprintf(keyX, "TsSplitX_%s", symbol.c_str());
    sprintf(keyY, "TsSplitY_%s", symbol.c_str());
    Settings_Save(keyX, (DWORD)(splitX * 10000.0f));
    Settings_Save(keyY, (DWORD)(splitY * 10000.0f));
}

bool Settings_LoadMarketSplitter(const std::string& symbol, float& splitX, float& splitY) {
    char keyX[256], keyY[256];
    sprintf(keyX, "TsSplitX_%s", symbol.c_str());
    sprintf(keyY, "TsSplitY_%s", symbol.c_str());

    // Use sentinel 0xFFFFFFFF to detect "not saved yet"
    DWORD dx = Settings_Load(keyX, 0xFFFFFFFF);
    DWORD dy = Settings_Load(keyY, 0xFFFFFFFF);
    if (dx == 0xFFFFFFFF || dy == 0xFFFFFFFF) return false;

    float fx = (float)dx / 10000.0f;
    float fy = (float)dy / 10000.0f;
    // Clamp defensively in case of corrupted registry data
    if (fx < 0.1f || fx > 0.9f || fy < 0.1f || fy > 0.9f) return false;

    splitX = fx;
    splitY = fy;
    return true;
}

void Session_RestoreWindows(
    const std::function<void()>& StartCoins,
    const std::function<void()>& StartDiamonds,
    const std::function<void()>& StartNews,
    const std::function<void()>& StartSettings,
    const std::function<HWND(const std::string&, int)>& StartMarket,
    const std::function<void()>& StartWatchlist,
    const std::function<void()>& StartOrders,
    const std::function<void()>& StartDebugLog
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
        if (cls == COINS_CLASS_NAME)     {
            StartCoins();
            char key[256];
            sprintf(key, "AlwaysOnTop_%s", COINS_CLASS_NAME);
            if(Settings_Load(key, 0))
                ToggleWindowAlwaysOnTop(COINS_CLASS_NAME);
        }
        else if (cls == DIAMONDS_CLASS_NAME)  { 
            StartDiamonds(); 
            char key[256];
            sprintf(key, "AlwaysOnTop_%s", DIAMONDS_CLASS_NAME);
            if(Settings_Load(key, 0))
                ToggleWindowAlwaysOnTop(DIAMONDS_CLASS_NAME); 
        }
        else if (cls == NEWS_CLASS_NAME)      { 
            StartNews(); 
            char key[256];
            sprintf(key, "AlwaysOnTop_%s", NEWS_CLASS_NAME);
            if(Settings_Load(key, 0)) 
                ToggleWindowAlwaysOnTop(NEWS_CLASS_NAME); 
        }
        else if (cls == SETTINGS_CLASS_NAME)  { 
            StartSettings(); 
            char key[256];
            sprintf(key, "AlwaysOnTop_%s", SETTINGS_CLASS_NAME);
            if(Settings_Load(key, 0)) 
                ToggleWindowAlwaysOnTop(SETTINGS_CLASS_NAME); 
        }
        else if (cls == WATCHLIST_CLASS_NAME)    { 
            StartWatchlist(); 
            char key[256];
            sprintf(key, "AlwaysOnTop_%s", WATCHLIST_CLASS_NAME);
            if(Settings_Load(key, 0)) 
                ToggleWindowAlwaysOnTop(WATCHLIST_CLASS_NAME); 
        }
        else if (cls == ORDERS_CLASS_NAME)    { 
            StartOrders(); 
            char key[256];
            sprintf(key, "AlwaysOnTop_%s", ORDERS_CLASS_NAME);
            if(Settings_Load(key, 0)) 
                ToggleWindowAlwaysOnTop(ORDERS_CLASS_NAME); 
        }
        else if (cls == DASHBOARD_CLASS_NAME)    { 
            char key[256];
            sprintf(key, "AlwaysOnTop_%s", DASHBOARD_CLASS_NAME);
            if(Settings_Load(key, 0)) 
                ToggleWindowAlwaysOnTop(DASHBOARD_CLASS_NAME); 
        }
        else if (cls == DEBUGLOG_CLASS_NAME)  { 
            StartDebugLog(); 
            char key[256];
            sprintf(key, "AlwaysOnTop_%s", DEBUGLOG_CLASS_NAME);
            if(Settings_Load(key, 0)) 
                ToggleWindowAlwaysOnTop(DEBUGLOG_CLASS_NAME); 
        }
        else if (cls == MARKET_CLASS_NAME) {
            std::string tsSaved = Settings_LoadString("OpenMarket");
            if (tsSaved.empty()) {
                StartMarket("", 0);
            } else {
                size_t start = 0;
                while (start < tsSaved.length()) {
                    size_t end = tsSaved.find(' ', start);
                    if (end == std::string::npos) end = tsSaved.length();
                    std::string token = tsSaved.substr(start, end - start);
                    auto dot = token.find('.');
                    if (dot != std::string::npos) {
                        int cid = std::stoi(token.substr(0, dot));
                        std::string sym = token.substr(dot + 1);
                        HWND tsHwnd = StartMarket(sym, cid);

                        // Restore AlwaysOnTop using the HWND we just got — avoids a
                        // FindWindowA-by-title race where the title may not be set yet.
                        char key[256];
                        sprintf(key, "AlwaysOnTop_%s_%s", MARKET_CLASS_NAME, sym.c_str());
                        if (Settings_Load(key, 0)) {
                            SetMarketAlwaysOnTop(tsHwnd, true);
                        }
                    }
                    start = end + 1;
                }
            }
        }
        p += strlen(p) + 1;
    }
}
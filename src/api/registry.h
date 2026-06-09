#pragma once

constexpr const char* APP_REG_ROOT = "Software\\ibkr-gateway-trading-floor";

static const char* DASHBOARD_CLASS_NAME          = "Dashboard";
static const char* DIAMONDS_CLASS_NAME           = "Diamonds";
static const char* ORDERS_CLASS_NAME             = "Orders";
static const char* ORDERS_EDIT_CLASS_NAME        = "Orders_Edit";
static const char* WATCHLIST_CLASS_NAME          = "Watchlist";
static const char* WATCHLIST_NEW_LIST_CLASS_NAME = "Watchlist_NewList";
static const char* MARKET_CLASS_NAME             = "Market";
static const char* MARKET_SEARCH_CLASS_NAME      = "Market_SearchSymbol";
static const char* NEWS_CLASS_NAME               = "News";
static const char* NEWS_ARTICLE_CLASS_NAME       = "NewsArticle";
static const char* SETTINGS_CLASS_NAME           = "Settings";
static const char* DEBUGLOG_CLASS_NAME           = "DebugLog";

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

static std::deque<std::string> debugBuffer; // stores messages when window is closed

// Payload passed during HWND creation
struct MarketInitData { std::string symbol; int conId; std::string winKey; };

void LogDebug(const std::string& msg) {
    time_t now = time(0);
    char tstr[26] = {};
    ctime_s(tstr, sizeof(tstr), &now);
    std::string timestamp = tstr;
    if (!timestamp.empty()) timestamp.pop_back();

    std::string fullMsg = "[" + timestamp + "] " + msg + "\r\n";
    debugBuffer.push_back(fullMsg);
    if (debugBuffer.size() > 50) {
        debugBuffer.pop_front();
    }

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

// Generic registry helpers to reduce duplication
void RegSetString(const char* subPath, const char* valueName, const std::string& value) {
    HKEY hKey;
    char fullPath[256];
    wsprintf(fullPath, "%s\\%s", APP_REG_ROOT, subPath);
    if (RegCreateKeyExA(HKEY_CURRENT_USER, fullPath, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        RegSetValueExA(hKey, valueName, 0, REG_SZ, (const BYTE*)value.c_str(), (DWORD)value.size() + 1);
        RegCloseKey(hKey);
    }
}

std::string RegGetString(const char* subPath, const char* valueName, const std::string& defaultValue = "") {
    HKEY hKey;
    char fullPath[256];
    wsprintf(fullPath, "%s\\%s", APP_REG_ROOT, subPath);
    if (RegOpenKeyExA(HKEY_CURRENT_USER, fullPath, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        char buf[2048] = {};
        DWORD size = sizeof(buf);
        if (RegQueryValueExA(hKey, valueName, NULL, NULL, (LPBYTE)buf, &size) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return std::string(buf);
        }
        RegCloseKey(hKey);
    }
    return defaultValue;
}

void RegSetDword(const char* subPath, const char* valueName, DWORD value) {
    HKEY hKey;
    char fullPath[256];
    wsprintf(fullPath, "%s\\%s", APP_REG_ROOT, subPath);
    if (RegCreateKeyExA(HKEY_CURRENT_USER, fullPath, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        RegSetValueExA(hKey, valueName, 0, REG_DWORD, (const BYTE*)&value, sizeof(DWORD));
        RegCloseKey(hKey);
    }
}

DWORD RegGetDword(const char* subPath, const char* valueName, DWORD defaultValue = 0) {
    HKEY hKey;
    char fullPath[256];
    wsprintf(fullPath, "%s\\%s", APP_REG_ROOT, subPath);
    DWORD value = defaultValue;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, fullPath, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD size = sizeof(DWORD);
        RegQueryValueExA(hKey, valueName, NULL, NULL, (LPBYTE)&value, &size);
        RegCloseKey(hKey);
    }
    return value;
}

void RegDelete(const char* subPath, const char* valueName) {
    HKEY hKey;
    char fullPath[256];
    wsprintf(fullPath, "%s\\%s", APP_REG_ROOT, subPath);
    if (RegOpenKeyExA(HKEY_CURRENT_USER, fullPath, 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
        RegDeleteValueA(hKey, valueName);
        RegCloseKey(hKey);
    }
}

// Convenience wrappers
void Settings_SaveString(const char* key, const std::string& value) {
    RegSetString("Settings", key, value);
}

std::string Settings_LoadString(const char* key, const std::string& defaultValue = "") {
    return RegGetString("Settings", key, defaultValue);
}

void Settings_News_Save(const char* key, const std::string& value) {
    RegSetString(NEWS_CLASS_NAME, key, value);
}

std::string Settings_News_Load(const char* key, const std::string& defaultValue = "") {
    return RegGetString(NEWS_CLASS_NAME, key, defaultValue);
}

void Settings_LastList_Save(const std::string& value) {
    RegSetString(WATCHLIST_CLASS_NAME, "LastList", value);
}

std::string Settings_LastList_Load(const std::string& defaultValue = "") {
    return RegGetString(WATCHLIST_CLASS_NAME, "LastList", defaultValue);
}

std::string Settings_Tab_Load(const char* key, const std::string& defaultValue = "") {
    return RegGetString(DIAMONDS_CLASS_NAME, key, defaultValue);
}

void Settings_Tab_Save(const char* key, const std::string& value) {
    RegSetString(DIAMONDS_CLASS_NAME, key, value);
}

std::string Settings_SymbolColors_Load(const std::string& defaultValue = "") {
    return RegGetString(DIAMONDS_CLASS_NAME, "SymbolColors", defaultValue);
}

void Settings_SymbolColors_Save(const std::string& value) {
    RegSetString(DIAMONDS_CLASS_NAME, "SymbolColors", value);
}

void Settings_SaveMarket(const std::vector<std::string>& sessions) {
    std::string combined;
    for (size_t i = 0; i < sessions.size(); ++i) {
        if (i > 0) combined += " ";
        combined += sessions[i];
    }
    Settings_SaveString("OpenWindows_Market", combined);
}

DWORD Settings_Sort_Load(const char* windowClassKey, const char* sortKey, DWORD defaultValue) {
    return RegGetDword(windowClassKey, sortKey, defaultValue);
}

void Settings_Sort_Save(const char* windowClassKey, const char* sortKey, DWORD value) {
    RegSetDword(windowClassKey, sortKey, value);
}

DWORD Settings_CheckedTabs_Load(DWORD defaultValue) {
    return RegGetDword(DIAMONDS_CLASS_NAME, "CheckedTabs", defaultValue);
}

void Settings_CheckedTabs_Save(DWORD value) {
    RegSetDword(DIAMONDS_CLASS_NAME, "CheckedTabs", value);
}

void Settings_Save(const char* key, DWORD value) {
    RegSetDword("Settings", key, value);
}

void Settings_Delete(const char* key) {
    RegDelete("Settings", key);
}

DWORD Settings_Load(const char* key, DWORD defaultValue) {
    return RegGetDword("Settings", key, defaultValue);
}

DWORD Settings_Overnight_Load(const char* windowClassKey, DWORD defaultValue) {
    return RegGetDword(windowClassKey, "OVERNIGHT", defaultValue);
}

void Settings_Overnight_Save(const char* windowClassKey, DWORD value) {
    RegSetDword(windowClassKey, "OVERNIGHT", value);
}

void Settings_AlwaysOnTop_Save(const char* windowClassKey, DWORD value) {
    RegSetDword(windowClassKey, "AlwaysOnTop", value);
}

void Settings_AlwaysOnTop_Delete(const char* windowClassKey) {
    RegDelete(windowClassKey, "AlwaysOnTop");
}

DWORD Settings_AlwaysOnTop_Load(const char* windowClassKey, DWORD defaultValue) {
    return RegGetDword(windowClassKey, "AlwaysOnTop", defaultValue);
}

bool Settings_KillGatewayOnExit() {
    return Settings_Load("Gateway_KillOnExit", 0) != 0;
}

bool Settings_AutoGateway() {
    return Settings_Load("Gateway_AutoStart", 0) != 0;
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
    
    std::string winKey;
    char className[256] = {};
    GetClassNameA(hWnd, className, sizeof(className));
    if (strcmp(className, MARKET_CLASS_NAME) == 0) {
        MarketInitData* data = (MarketInitData*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
        winKey = data->winKey;
    } else {
        winKey = className;
    }

    char fullPath[256];
    wsprintf(fullPath, "%s\\%s", APP_REG_ROOT, winKey.c_str());

    HKEY hKey;
    if (RegCreateKeyEx(HKEY_CURRENT_USER, fullPath, 0, NULL, 
        REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) 
    {
        RegSetValueEx(hKey, "Window_X", 0, REG_DWORD, (const BYTE*)&x, sizeof(DWORD));
        RegSetValueEx(hKey, "Window_Y", 0, REG_DWORD, (const BYTE*)&y, sizeof(DWORD));
        RegSetValueEx(hKey, "Window_W", 0, REG_DWORD, (const BYTE*)&w, sizeof(DWORD));
        RegSetValueEx(hKey, "Window_H", 0, REG_DWORD, (const BYTE*)&h, sizeof(DWORD));
        RegCloseKey(hKey);
    }
}

bool LoadWinPosition(const char* subKeyName, int &x, int &y, int &w, int &h) {
    HKEY hKey;
    char fullPath[256];
    wsprintf(fullPath, "%s\\%s", APP_REG_ROOT, subKeyName);

    if (RegOpenKeyEx(HKEY_CURRENT_USER, fullPath, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD dwSize = sizeof(DWORD);
        RegQueryValueEx(hKey, "Window_X", NULL, NULL, (LPBYTE)&x, &dwSize);
        RegQueryValueEx(hKey, "Window_Y", NULL, NULL, (LPBYTE)&y, &dwSize);
        RegQueryValueEx(hKey, "Window_W", NULL, NULL, (LPBYTE)&w, &dwSize);
        RegQueryValueEx(hKey, "Window_H", NULL, NULL, (LPBYTE)&h, &dwSize);
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
LRESULT CALLBACK RichEditColorScrollSubclass(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, 
                                             UINT_PTR uIdSubclass, DWORD_PTR dwRefData) 
{
    switch (uMsg) {
        case WM_NCPAINT: {
            if (!Settings_DarkMode()) return DefSubclassProc(hWnd, uMsg, wParam, lParam);
            
            HDC hdc = GetWindowDC(hWnd);
            if (hdc) {
                RECT rcWindow;
                GetWindowRect(hWnd, &rcWindow);
                
                // Translate coordinates to relative 0,0 window space
                int winWidth  = rcWindow.right - rcWindow.left;
                int winHeight = rcWindow.bottom - rcWindow.top;

                // Target the Vertical Scrollbar territory 
                // (Usually sits on the right side, wide as GetSystemMetrics(SM_CXVSCROLL))
                int scrollWidth = GetSystemMetrics(SM_CXVSCROLL);
                RECT rcScroll = { winWidth - scrollWidth, 0, winWidth, winHeight };

                // 4. Paint over it with your custom theme color brush
                HBRUSH hCustomBrush = CreateSolidBrush(RGB(45, 45, 45)); // Dark palette
                FillRect(hdc, &rcScroll, hCustomBrush);

                // Clean up GDI assets
                DeleteObject(hCustomBrush);
                ReleaseDC(hWnd, hdc);
            }
            return 0; // Signify message handled
        }

        // To prevent flashes, you must also intercept mouse interactions on the bar
        case WM_NCMOUSEMOVE:
        case WM_NCLBUTTONDOWN:
        case WM_NCLBUTTONUP:
            // Force redraw immediately when clicked or hovered so blue highlights don't overwrite
            InvalidateRect(hWnd, NULL, FALSE);
            break;
        case WM_NCDESTROY:
            RemoveWindowSubclass(hWnd, RichEditColorScrollSubclass, uIdSubclass);
            break;
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
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

void ApplyDarkModeToRichEdits(HWND hEdit, bool dark) {
    SetWindowSubclass(hEdit, RichEditColorScrollSubclass, 888, 0);
    if (dark) {
        // Theme scrollbars
        SetWindowTheme(hEdit, L"DarkMode_Explorer", L"ScrollBar");
    } else {
        // Revert to default light mode themes
        SetWindowTheme(hEdit, L"Explorer", L"ScrollBar");
    }
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

BOOL CALLBACK EnumChildProcForEdits(HWND hwnd, LPARAM lParam) {
    char className[256];
    if (GetClassNameA(hwnd, className, sizeof(className))) {
        if (StrStrIA(className, "EDIT") != NULL) {
            LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_STYLE);
            if ((style & ES_MULTILINE) == ES_MULTILINE) {
                bool dark = (bool)lParam;
                ApplyDarkModeToRichEdits(hwnd, dark);
            }
        }
    }
    return TRUE;
}

void ApplyDarkMode(HWND hWnd) {
    BOOL dark = Settings_DarkMode() ? TRUE : FALSE;
    DwmSetWindowAttribute(hWnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));

    // Automatically find and theme any ListViews and RichEdit inside this window
    EnumChildWindows(hWnd, EnumChildProcForLists, (LPARAM)dark);
    EnumChildWindows(hWnd, EnumChildProcForEdits, (LPARAM)dark);

    DWM_WINDOW_CORNER_PREFERENCE preference = DWMWCP_ROUND;
    DwmSetWindowAttribute(hWnd, DWMWA_WINDOW_CORNER_PREFERENCE, &preference, sizeof(preference));
    
    //char className[256] = {};
    //GetClassNameA(hWnd, className, sizeof(className));
    //if (strcmp(className, DASHBOARD_CLASS_NAME) == 0) {
    // Set backdrop type
    // DWMSBT_MAINWINDOW    = Mica
    // DWMSBT_TABBEDWINDOW  = Mica Alt (a darker/more intense version)
    // DWMSBT_TRANSIENTWINDOW = Acrylic
    DWM_SYSTEMBACKDROP_TYPE backdropType = DWMSBT_TRANSIENTWINDOW;
    DwmSetWindowAttribute(hWnd, DWMWA_SYSTEMBACKDROP_TYPE, &backdropType, sizeof(backdropType));
    //}
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
            sprintf(key, "%s_%s", windowClassName, windowIdentifier);
        } else {
            sprintf(key, "%s", windowClassName);
        }
        Settings_AlwaysOnTop_Save(key, 0);
    } else {
        // Not always on top, make it so
        SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        // Save state: on top
        char key[256];
        if (windowIdentifier && strlen(windowIdentifier) > 0) {
            sprintf(key, "%s_%s", windowClassName, windowIdentifier);
        } else {
            sprintf(key, "%s", windowClassName);
        }
        if (IsIconic(hWnd)) {
            ShowWindow(hWnd, SW_RESTORE);
        } else {
            ShowWindow(hWnd, SW_SHOW);
        }
        Settings_AlwaysOnTop_Save(key, 1);
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
        std::string symbol;
        MarketInitData* data = (MarketInitData*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
        if (data && !data->symbol.empty()) {
            result.push_back({hWnd, data->symbol});
        }
        
        // Find next window of the same class with a different title
        hWnd = FindWindowExA(NULL, hWnd, MARKET_CLASS_NAME, NULL);
    }
    
    return result;
}

// Check if a specific Market window is set to Always On Top
bool IsMarketAlwaysOnTop(const std::string& symbol) {
    char key[256];
    sprintf(key, "%s_%s", MARKET_CLASS_NAME, symbol.c_str());
    return Settings_AlwaysOnTop_Load(key, 0) != 0;
}

// Toggle Always On Top for a specific Market window by symbol
// Uses consistent registry key format based on symbol only
void ToggleMarketAlwaysOnTop(HWND hWnd, const std::string& symbol) {
    if (!hWnd) {
        return; // Window not found
    }
    
    LONG_PTR exStyle = GetWindowLongPtr(hWnd, GWL_EXSTYLE);
    bool isCurrentlyOnTop = (exStyle & WS_EX_TOPMOST) != 0;
    
    char key[256];
    sprintf(key, "%s_%s", MARKET_CLASS_NAME, symbol.c_str());
    
    if (isCurrentlyOnTop) {
        // Currently always on top, remove it
        SetWindowPos(hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        Settings_AlwaysOnTop_Save(key, 0);
    } else {
        // Not always on top, make it so
        SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        Settings_AlwaysOnTop_Save(key, 1);
    }
}

// Set Always On Top state for a Market window using its HWND directly.
// Called at restore time — avoids a FindWindowA-by-title race where the window
// title may not yet be set when StartMarket() returns.
void SetMarketAlwaysOnTop(HWND hWnd, bool onTop) {
    if (!hWnd) return;
    SetWindowPos(hWnd, onTop ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
}

void Save_OpenWindows(const char* className) {
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

void Session_AddWindow(HWND hWnd, LPARAM lParam) {
    ApplyDarkMode(hWnd);

    char className[256] = {};
    GetClassNameA(hWnd, className, sizeof(className));

    std::string winKey;
    if (strcmp(className, MARKET_CLASS_NAME) == 0) {
        MarketInitData* data = (MarketInitData*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
        if (data) {
            winKey = data->winKey;
        }
    } else {
        winKey = className;
    }

    if (!winKey.empty()) {
        if (Settings_AlwaysOnTop_Load(winKey.c_str(), 0)) {
            if (strcmp(className, MARKET_CLASS_NAME) == 0)
                SetMarketAlwaysOnTop(hWnd, true);
            else ToggleWindowAlwaysOnTop(winKey.c_str());                        
        }

        if (strcmp(className, DASHBOARD_CLASS_NAME) != 0) { // Dashboard is always open on boot, no need to track in registry
            Save_OpenWindows(className);
        }
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
    wsprintf(fullPath, "%s\\%s\\SymbolLists", APP_REG_ROOT, WATCHLIST_CLASS_NAME);
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
    wsprintf(fullPath, "%s\\%s\\SymbolLists", APP_REG_ROOT, WATCHLIST_CLASS_NAME);
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
    wsprintf(fullPath, "%s\\%s\\SymbolLists", APP_REG_ROOT, WATCHLIST_CLASS_NAME);
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
    wsprintf(fullPath, "%s\\%s\\SymbolLists", APP_REG_ROOT, WATCHLIST_CLASS_NAME);
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
void Settings_NewList_Save(const char* newName) {
    HKEY hKey;
    char fullPath[256];
    wsprintf(fullPath, "%s\\%s\\SymbolLists", APP_REG_ROOT, WATCHLIST_CLASS_NAME);
    if (RegCreateKeyExA(HKEY_CURRENT_USER, fullPath, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        const char empty[2] = { '\0', '\0' };
        RegSetValueExA(hKey, newName, 0, REG_MULTI_SZ, (const BYTE*)empty, 2);
        RegCloseKey(hKey);
    }
}

// ── Market splitter persistence ───────────────────────────────────────────
// splitY are stored as DWORD scaled ×10000 so floats survive REG_DWORD.
// Keys are per-symbol so multiple open windows never collide.

void Settings_SaveMarketSplitter(const std::string& symbol, float splitY) {
    char windowKey[256];
    sprintf(windowKey, "%s_%s", MARKET_CLASS_NAME, symbol.c_str());
    RegSetDword(windowKey, "SplitY", (DWORD)(splitY * 10000.0f));
}

bool Settings_LoadMarketSplitter(const std::string& symbol, float& splitY) {
    char windowKey[256];
    sprintf(windowKey, "%s_%s", MARKET_CLASS_NAME, symbol.c_str());

    // Use sentinel 0xFFFFFFFF to detect "not saved yet"
    DWORD dx = RegGetDword(windowKey, "SplitY", 0xFFFFFFFF);
    if (dx == 0xFFFFFFFF) return false;

    float fx = (float)dx / 10000.0f;
    // Clamp defensively in case of corrupted registry data
    if (fx < 0.1f || fx > 0.9f) return false;

    splitY = fx;
    return true;
}

// ── TTS voice persistence ─────────────────────────────────────────────────────
// Saves/loads the SAPI token ID (registry path string) for the selected TTS voice.
// Default is empty string → caller should fall back to Herena-Catalan search.

void Settings_SaveTtsVoice(const std::string& tokenId) {
    Settings_SaveString("VoiceTokenId", tokenId);
}

std::string Settings_LoadTtsVoice() {
    return Settings_LoadString("VoiceTokenId", "");
}

// ── Shared TTS voice-selection helper ────────────────────────────────────────
// Case-insensitive wstring contains check (shared utility, safe to define here).
static bool TTS_ContainsCI(const WCHAR* s, const WCHAR* needle) {
    if (!s || !needle) return false;
    std::wstring ws(s), wn(needle);
    auto toLo = [](wchar_t c) { return (wchar_t)towlower(c); };
    std::transform(ws.begin(), ws.end(), ws.begin(), toLo);
    std::transform(wn.begin(), wn.end(), wn.begin(), toLo);
    return ws.find(wn) != std::wstring::npos;
}

// Search one voice category for a token matching the given ID string.
// If tokenId is empty, falls back to searching for Herena/Catalan.
// Returns a token the caller must Release(), or nullptr.
static ISpObjectToken* TTS_FindVoiceInCategory(const WCHAR* categoryId, const std::wstring& tokenId) {
    IEnumSpObjectTokens* pEnum = nullptr;
    if (FAILED(SpEnumTokens(categoryId, NULL, NULL, &pEnum))) return nullptr;

    ISpObjectToken* pToken  = nullptr;
    ISpObjectToken* pFound  = nullptr;

    while (!pFound && SUCCEEDED(pEnum->Next(1, &pToken, NULL)) && pToken) {
        WCHAR* pId       = nullptr;
        WCHAR* pDesc     = nullptr;
        WCHAR* pAttrName = nullptr;

        pToken->GetId(&pId);
        SpGetDescription(pToken, &pDesc);
        ISpDataKey* pAttribs = nullptr;
        if (SUCCEEDED(pToken->OpenKey(L"Attributes", &pAttribs))) {
            pAttribs->GetStringValue(L"Name", &pAttrName);
            pAttribs->Release();
        }

        bool match = false;
        if (!tokenId.empty()) {
            // Exact match on token ID (registry path)
            match = (pId && tokenId == std::wstring(pId));
        } else {
            // Default fallback: search for Herena/Catalan
            match = TTS_ContainsCI(pId,       L"herena")  ||
                    TTS_ContainsCI(pDesc,     L"herena")  ||
                    TTS_ContainsCI(pAttrName, L"herena")  ||
                    TTS_ContainsCI(pId,       L"helena")  ||
                    TTS_ContainsCI(pDesc,     L"helena")  ||
                    TTS_ContainsCI(pAttrName, L"helena")  ||
                    TTS_ContainsCI(pId,       L"ca-es")   ||
                    TTS_ContainsCI(pDesc,     L"ca-es")   ||
                    TTS_ContainsCI(pAttrName, L"ca-es")   ||
                    TTS_ContainsCI(pId,       L"catalan") ||
                    TTS_ContainsCI(pDesc,     L"catalan") ||
                    TTS_ContainsCI(pAttrName, L"catalan");
        }

        if (pId)       CoTaskMemFree(pId);
        if (pDesc)     CoTaskMemFree(pDesc);
        if (pAttrName) CoTaskMemFree(pAttrName);

        if (match) pFound = pToken;
        else       { pToken->Release(); pToken = nullptr; }
    }
    pEnum->Release();
    return pFound;
}

// Apply the saved (or default Herena-Catalan) voice to an ISpVoice instance.
// Searches both classic SAPI and OneCore registries.
static void TTS_ApplySavedVoice(ISpVoice* pVoice) {
    if (!pVoice) return;

    std::string savedA = Settings_LoadTtsVoice();
    std::wstring tokenId(savedA.begin(), savedA.end());

    // 1. Classic SAPI voices
    ISpObjectToken* pFound = TTS_FindVoiceInCategory(SPCAT_VOICES, tokenId);

    // 2. OneCore voices (Win10+ neural / browser voices)
    if (!pFound)
        pFound = TTS_FindVoiceInCategory(
            L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Speech_OneCore\\Voices", tokenId);

    if (pFound) {
        pVoice->SetVoice(pFound);
        pFound->Release();
    }
    // else: keep system default
}

// ── Voice enumeration for the Settings UI ────────────────────────────────────
struct TtsVoiceEntry {
    std::wstring tokenId;   // SAPI token registry path (used as key)
    std::wstring display;   // Friendly name shown in the combo
};

// Custom message broadcast to all top-level windows when the user picks a new
// TTS voice in Settings, so open Market and Dashboard windows hot-swap immediately.
// wParam = 0, lParam = 0.
#define WM_TTS_VOICE_CHANGED (WM_APP + 201)

// Enumerate ALL voices from both classic SAPI and OneCore registries.
// Duplicates (same tokenId) are suppressed so voices that appear in both
// registries are not listed twice.
static std::vector<TtsVoiceEntry> TTS_EnumerateVoices() {
    std::vector<TtsVoiceEntry> voices;
    auto addFromCategory = [&](const WCHAR* categoryId) {
        IEnumSpObjectTokens* pEnum = nullptr;
        if (FAILED(SpEnumTokens(categoryId, NULL, NULL, &pEnum))) return;

        ISpObjectToken* pToken = nullptr;
        while (SUCCEEDED(pEnum->Next(1, &pToken, NULL)) && pToken) {
            WCHAR* pId       = nullptr;
            WCHAR* pDesc     = nullptr;
            WCHAR* pAttrName = nullptr;

            pToken->GetId(&pId);
            SpGetDescription(pToken, &pDesc);
            ISpDataKey* pAttribs = nullptr;
            if (SUCCEEDED(pToken->OpenKey(L"Attributes", &pAttribs))) {
                pAttribs->GetStringValue(L"Name", &pAttrName);
                pAttribs->Release();
            }

            if (pId) {
                std::wstring tid(pId);
                // Deduplicate by tokenId
                bool dup = false;
                for (const auto& v : voices) if (v.tokenId == tid) { dup = true; break; }
                if (!dup) {
                    std::wstring disp = pAttrName ? std::wstring(pAttrName)
                                      : pDesc    ? std::wstring(pDesc)
                                      :            tid;
                    voices.push_back({ tid, disp });
                }
            }

            if (pId)       CoTaskMemFree(pId);
            if (pDesc)     CoTaskMemFree(pDesc);
            if (pAttrName) CoTaskMemFree(pAttrName);
            pToken->Release();
        }
        pEnum->Release();
    };

    addFromCategory(SPCAT_VOICES);
    addFromCategory(L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Speech_OneCore\\Voices");
    return voices;
}

void Session_RestoreWindows(
    const std::function<void()>& StartDiamonds,
    const std::function<void()>& StartNews,
    const std::function<void()>& StartSettings,
    const std::function<void(const std::string&, int)>& StartMarket,
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
        if (cls == DIAMONDS_CLASS_NAME)  { 
            StartDiamonds(); 
        }
        else if (cls == NEWS_CLASS_NAME)      { 
            StartNews(); 
        }
        else if (cls == SETTINGS_CLASS_NAME)  { 
            StartSettings(); 
        }
        else if (cls == WATCHLIST_CLASS_NAME)    { 
            StartWatchlist(); 
        }
        else if (cls == ORDERS_CLASS_NAME)    { 
            StartOrders(); 
        }
        else if (cls == DEBUGLOG_CLASS_NAME)  { 
            StartDebugLog(); 
        }
        else if (cls == MARKET_CLASS_NAME) {
            std::string tsSaved = Settings_LoadString("OpenWindows_Market");
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
                        StartMarket(sym, cid);
                    }
                    start = end + 1;
                }
            }
        }
        p += strlen(p) + 1;
    }
}
#pragma once

#include <cstring>
#include <algorithm>
#include <string>
#include <vector>
#include <unordered_map>
#include <dwmapi.h>
#include <uxtheme.h>
#include <commctrl.h>

NOTIFYICONDATAW nid = { 0 };

// Sentinel: no custom color – let HandleDarkModeMessages / system theme paint this control
#define COLOR_THEME   ((COLORREF)0xFFFFFFFF)

// ─── Per-control color table ──────────────────────────────────────────────────
static HWND     gClrHwnd[160]  = {};
static COLORREF gClrColor[160] = {};
static int      gClrCount      = 0;

static void SetCtrlColor(HWND hw, COLORREF c) {
    for (int i = 0; i < gClrCount; i++)
        if (gClrHwnd[i] == hw) { gClrColor[i] = c; return; }
    if (gClrCount < 160) { gClrHwnd[gClrCount] = hw; gClrColor[gClrCount++] = c; }
}
static COLORREF GetCtrlColor(HWND hw) {
    for (int i = 0; i < gClrCount; i++)
        if (gClrHwnd[i] == hw) return gClrColor[i];
    return COLOR_THEME;  // not registered = let theme handle it
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

HWND StartGenericWindow(const char* className, const char* title, const wchar_t* taskbarId, int defaultW, int defaultH, HINSTANCE hInst = NULL, const std::string& windowKey = "", LPVOID lpParam = NULL) {
    if (hDarkBrush == NULL || hDarkBrush2 == NULL) InitDarkBrushes();
    // Multi-instance windows (windowKey differs from className, e.g. market per-symbol)
    // are distinguished by title - each has a unique one. Single-instance windows match
    // on class alone. Either way no map needed: FindWindowA does the work.
    bool multiInstance = !windowKey.empty() && windowKey != className;
    HWND hWnd = FindWindowA(className, multiInstance ? title : NULL);

    if (hWnd && IsWindow(hWnd)) {
        if (IsIconic(hWnd)) {
            ShowWindow(hWnd, SW_RESTORE);
        } else {
            ShowWindow(hWnd, SW_SHOW);
        }
        SetForegroundWindow(hWnd);
        SetActiveWindow(hWnd);
        SetFocus(hWnd);
        return hWnd;
    }

    int w = defaultW, h = defaultH;
    int x = (GetSystemMetrics(SM_CXSCREEN) - w) / 2;
    int y = (GetSystemMetrics(SM_CYSCREEN) - h) / 2;
    
    std::string* winPostionKey = new std::string(multiInstance ? windowKey.c_str() : className);
    LoadWinPosition(winPostionKey->c_str(), x, y, w, h);

    if (hInst) {
        hWnd = CreateWindow(className, title, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, x, y, w, h, NULL, NULL, hInst, lpParam);
        ShowWindow(hWnd, SW_SHOW);
        UpdateWindow(hWnd);
    } else {
        HWND hWndParent = NULL;
        DWORD dwExStyle = WS_EX_APPWINDOW;
        DWORD dwStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE;
        if (strcmp(className, NEWS_CLASS_NAME)         == 0
         || strcmp(className, ORDERS_CLASS_NAME)       == 0
         || strcmp(className, DIAMONDS_CLASS_NAME)     == 0
         || strcmp(className, MARKET_CLASS_NAME)    == 0
         || strcmp(className, WATCHLIST_CLASS_NAME)       == 0
         || strcmp(className, NEWS_ARTICLE_CLASS_NAME) == 0
        ) {
            dwStyle = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
        }
        if (strcmp(className, DEBUGLOG_CLASS_NAME) == 0) {
            dwExStyle = WS_EX_TOPMOST;
            dwStyle   = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
        }
        if (strcmp(className, WATCHLIST_NEW_LIST_CLASS_NAME) == 0) {
            dwExStyle = WS_EX_DLGMODALFRAME | WS_EX_TOPMOST;
            dwStyle   = WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE;
            hWndParent = FindWindowA(WATCHLIST_CLASS_NAME, NULL);
        }
        if (strcmp(className, NEWS_ARTICLE_CLASS_NAME) == 0) {
            dwExStyle = WS_EX_DLGMODALFRAME;
            dwStyle   = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
            hWndParent = FindWindowA(NEWS_CLASS_NAME, NULL);
        }
        hWnd = CreateWindowExA(dwExStyle, className, title, dwStyle, x, y, w, h, hWndParent, NULL, GetModuleHandle(NULL), lpParam);
    }

    SetPropA(hWnd, "WinPositionKey", (HANDLE)winPostionKey);

    if (strcmp(className, WATCHLIST_NEW_LIST_CLASS_NAME) != 0)
        SetWindowTaskbarId(hWnd, taskbarId);

    return hWnd;
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

struct ListViewZoomData {
    HFONT hFont;
    HFONT hBoldFont;
    int fontSize;
    const char* settingKey;
};

HFONT CreateBoldFont(HFONT hNormalFont) {
    LOGFONTA lf;
    if (!GetObjectA(hNormalFont, sizeof(LOGFONTA), &lf)) {
        return NULL; 
    }
    
    lf.lfWeight = FW_BOLD; 
    
    return CreateFontIndirectA(&lf);
}

static void ApplyListViewFont(HWND hList, HFONT& hFont, HFONT& hBoldFont, int fontSize) {
    if (hFont) {
        DeleteObject(hFont);
    }

    // Calculate the correct height based on the device context
    HDC hdc = GetDC(hList);
    int fontHeight = -MulDiv(fontSize, GetDeviceCaps(hdc, LOGPIXELSY), 72);
    ReleaseDC(hList, hdc);

    hFont = CreateFontA(fontHeight, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, 
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
    
    if (hBoldFont) {
        DeleteObject(hBoldFont);
    }
    hBoldFont = CreateBoldFont(hFont);
    
    SendMessage(hList, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

    HWND hHeader = ListView_GetHeader(hList);
    int colCount = Header_GetItemCount(hHeader);
    for (int i = 0; i < colCount; i++) {
        ListView_SetColumnWidth(hList, i, LVSCW_AUTOSIZE_USEHEADER);
    }
    
    // Redraw the control completely
    InvalidateRect(hList, NULL, TRUE);
}

LRESULT CALLBACK ListViewZoomProc(HWND hList, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    auto* zoomData = reinterpret_cast<ListViewZoomData*>(dwRefData);
    if (!zoomData) return DefSubclassProc(hList, uMsg, wParam, lParam);

    if (uMsg == WM_MOUSEWHEEL) {
        if (GetKeyState(VK_CONTROL) & 0x8000) {
            int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);

            if (zDelta > 0) zoomData->fontSize++;
            else zoomData->fontSize--;

            if (zoomData->fontSize < 6)  zoomData->fontSize = 6;
            if (zoomData->fontSize > 30) zoomData->fontSize = 30;

            Settings_Save(zoomData->settingKey, zoomData->fontSize);
            ApplyListViewFont(hList, zoomData->hFont, zoomData->hBoldFont, zoomData->fontSize);

            return 0; // Prevent default scrolling
        }
    }
    return DefSubclassProc(hList, uMsg, wParam, lParam);
}

std::unordered_map<std::string, HICON> offlineIcons;
std::unordered_map<std::string, HICON> onlineIcons;

void RegisterWindowClass(HINSTANCE hInst, WNDPROC WndProc, const char* className, int iconId) {
    HICON& offlineIcon = offlineIcons[std::string(className)];
    HICON& onlineIcon  = onlineIcons[std::string(className)];
    onlineIcon  = (HICON)LoadImage(hInst, MAKEINTRESOURCE(iconId), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
    offlineIcon = CreateGrayIcon(onlineIcon);
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.lpszClassName = className;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    if (strcmp(className, DEBUGLOG_CLASS_NAME) == 0 || strcmp(className, WATCHLIST_NEW_LIST_CLASS_NAME) == 0) {
        wc.hInstance = GetModuleHandle(NULL);
    } else {
        wc.hInstance = hInst;
    }
    wc.hIcon = onlineIcon;
    RegisterClass(&wc);
}

void ClearAlwaysOnTopSetting(HWND hWnd) {
    char className[256] = {};
    GetClassNameA(hWnd, className, sizeof(className));

    char key[256] = {};
    if (strcmp(className, MARKET_CLASS_NAME) == 0) {
        char title[256] = {};
        GetWindowTextA(hWnd, title, sizeof(title));
        const std::string titleStr = title;
        const std::string prefix = "Market: ";
        size_t pos = titleStr.find(prefix);
        if (pos == std::string::npos) {
            return;
        }
        std::string symbol = titleStr.substr(pos + prefix.length());
        if (symbol.empty()) {
            return;
        }
        sprintf(key, "AlwaysOnTop_%s_%s", className, symbol.c_str());
    } else if (className == "ImpossibleToFindClass") {
        sprintf(key, "AlwaysOnTop_%s", className);
    }

    Settings_Delete(key);
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
            char className[256] = {};
            GetClassNameA(hWnd, className, sizeof(className));
            if (strcmp(className, COINS_CLASS_NAME) == 0) {
                COLORREF clr = GetCtrlColor((HWND)lParam);
                if (clr != COLOR_THEME) {
                    SetTextColor((HDC)wParam, clr);
                    if (Settings_DarkMode()) {
                        SetBkColor((HDC)wParam, DM_BG);
                        return (LRESULT)hDarkBrush;
                    } else {
                        SetBkColor((HDC)wParam, GetSysColor(COLOR_BTNFACE));
                        return (LRESULT)GetSysColorBrush(COLOR_BTNFACE);
                    }
                }
            }
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
            bool focused  = (dis->itemState & ODS_FOCUS);

            // 1. Modern, softer background colors
            COLORREF bgColor = dark 
                ? (pressed ? RGB(75, 75, 75) : RGB(45, 45, 45))
                : (pressed ? RGB(210, 210, 210) : RGB(240, 240, 240));

            // 2. Subtle border colors that blend smoothly
            COLORREF borderColor = dark 
                ? (pressed ? RGB(100, 100, 100) : RGB(65, 65, 65))
                : (pressed ? RGB(180, 180, 180) : RGB(215, 215, 215));

            COLORREF textColor = disabled 
                ? (dark ? RGB(120, 120, 120) : RGB(160, 160, 160)) 
                : (dark ? DM_TEXT : LM_TEXT);

            // --- Fill Background ---
            HBRUSH hBg = CreateSolidBrush(bgColor);
            FillRect(dis->hDC, &dis->rcItem, hBg);
            DeleteObject(hBg);

            // --- Draw Subtle Border ---
            HPEN hPen = CreatePen(PS_SOLID, 1, borderColor);
            HPEN hOld = (HPEN)SelectObject(dis->hDC, hPen);
            SelectObject(dis->hDC, GetStockObject(NULL_BRUSH)); // Transparent inside
            Rectangle(dis->hDC, dis->rcItem.left, dis->rcItem.top, dis->rcItem.right, dis->rcItem.bottom);
            SelectObject(dis->hDC, hOld);
            DeleteObject(hPen);

            // --- Draw Icon or Text ---
            // Icon buttons store their handle via SetProp; text buttons return NULL.
            HICON hIcon = (HICON)GetProp(dis->hwndItem, "hIcon");
            if (hIcon) {
                // Centre the 24×24 icon inside the button rect, nudge 1px on press
                int iconW = 24, iconH = 24;
                int ix = dis->rcItem.left + ((dis->rcItem.right  - dis->rcItem.left) - iconW) / 2 + (pressed ? 1 : 0);
                int iy = dis->rcItem.top  + ((dis->rcItem.bottom - dis->rcItem.top)  - iconH) / 2 + (pressed ? 1 : 0);
                DrawIconEx(dis->hDC, ix, iy, hIcon, iconW, iconH, 0, NULL, DI_NORMAL);
            } else {
                wchar_t text[128] = {};
                GetWindowTextW(dis->hwndItem, text, sizeof(text)/sizeof(wchar_t));
                SetTextColor(dis->hDC, textColor);
                SetBkMode(dis->hDC, TRANSPARENT);
                DrawTextW(dis->hDC, text, -1, &dis->rcItem, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            }

            // --- Modern Focus Indicator ---
            // DrawFocusRect draws an ugly black/white dotted line. 
            // This draws a subtle inner border instead when focused (optional).
            if (focused && !pressed) {
                RECT rcFocus = dis->rcItem;
                InflateRect(&rcFocus, -2, -2); // shrink by 2 pixels
                HPEN hFocusPen = CreatePen(PS_DOT, 1, dark ? RGB(100, 100, 100) : RGB(170, 170, 170));
                HPEN hOldFocus = (HPEN)SelectObject(dis->hDC, hFocusPen);
                Rectangle(dis->hDC, rcFocus.left, rcFocus.top, rcFocus.right, rcFocus.bottom);
                SelectObject(dis->hDC, hOldFocus);
                DeleteObject(hFocusPen);
            }

            return TRUE;
        }
    }
    return 0;
}

LRESULT HandleCommonMessages(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    char className[256] = {};
    GetClassNameA(hWnd, className, sizeof(className));

    switch (message) {
        case WM_CREATE: {
            HICON hIcon = api.isConnected() ? onlineIcons[std::string(className)] : offlineIcons[std::string(className)];
            SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
            SendMessage(hWnd, WM_SETICON, ICON_BIG,   (LPARAM)hIcon);
            Session_AddWindow(hWnd);
            return 0;
        }
        case WM_CLOSE:
            if (strcmp(className, DASHBOARD_CLASS_NAME) == 0) 
                ShowWindow(hWnd, SW_HIDE);
            else
                DestroyWindow(hWnd);
            return 0;
        case WM_SIZE:
        case WM_MOVE:
            SaveWinPosition(hWnd);
            return 0;
        case WM_DESTROY:
            SaveWinPosition(hWnd);
            ClearAlwaysOnTopSetting(hWnd);
            if (strcmp(className, DASHBOARD_CLASS_NAME) == 0) {
                api.removeApiUpdateWindow(hWnd);
                api.clearApiErrorWindow(hWnd);
                Shell_NotifyIconW(NIM_DELETE, &nid);
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
                found = true; break;
            }
        } while (Process32Next(hSnap, &pe));
    }
    CloseHandle(hSnap);
    return found;
}

std::string GetGatewayPath() {
    HKEY hKey;
    char fullPath[256];
    wsprintf(fullPath, "%s\\Settings", APP_REG_ROOT);
    if (RegOpenKeyExA(HKEY_CURRENT_USER, fullPath, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        char path[MAX_PATH] = {};
        DWORD size = sizeof(path);
        if (RegQueryValueExA(hKey, "GatewayPath", NULL, NULL, (LPBYTE)path, &size) == ERROR_SUCCESS && strlen(path) > 0) {
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
        RegSetValueExA(hKey, "GatewayPath", 0, REG_SZ, (const BYTE*)path.c_str(), (DWORD)path.size() + 1);
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
    if (GetOpenFileNameA(&ofn)) return std::string(path);
    return "";
}

bool alreadyEnsureGatewayRunning = false;
void EnsureGatewayRunning(HWND hParent) {
    if (alreadyEnsureGatewayRunning || !Settings_AutoGateway() || IsProcessRunning("ibgateway.exe")) return;
    alreadyEnsureGatewayRunning = true;
    std::string path = GetGatewayPath();
    if (path.empty()) {
        const char* defaultPath = "C:\\whatever\\ibgateway.exe";
        if (GetFileAttributesA(defaultPath) != INVALID_FILE_ATTRIBUTES) {
            path = defaultPath;
            SaveGatewayPath(path);
        }
    }
    if (path.empty() || GetFileAttributesA(path.c_str()) == INVALID_FILE_ATTRIBUTES) {
        MessageBoxA(hParent, "IB Gateway not found. Please locate ibgateway.exe.", "Gateway Not Found", MB_OK | MB_ICONINFORMATION);
        path = AskGatewayPath(hParent);
        if (path.empty()) return; 
        SaveGatewayPath(path);
    }
    alreadyEnsureGatewayRunning = false;
    LogDebug("Running IBKR Gateway, please login..");
    ShellExecuteA(NULL, "open", path.c_str(), NULL, NULL, SW_SHOW);
}

#include <filesystem>
void KillGateway() {
    std::string path = GetGatewayPath();
    if (path.empty()) path = "C:\\whatever\\ibgateway.exe";
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

std::string FormatWithCommas(double value) {
    char temp[64];
    // Format to 2 decimal places first
    snprintf(temp, sizeof(temp), "%.2f", value);
    std::string s(temp);

    // Find the decimal point
    int dotPos = s.find('.');
    if (dotPos == std::string::npos) dotPos = s.length();

    // If it's a negative number, don't put a comma after the minus sign!
    int start = (value < 0.0) ? 1 : 0;

    // Insert commas every 3 characters moving left from the decimal point
    for (int i = dotPos - 3; i > start; i -= 3) {
        s.insert(i, ",");
    }

    return s;
}

std::wstring StringToWide(const std::string& str) {
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}
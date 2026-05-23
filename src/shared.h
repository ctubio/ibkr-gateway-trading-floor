#pragma once

#include <cstring>

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

NOTIFYICONDATA nid = { 0 };

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
        HWND hWndParent = NULL;
        DWORD dwExStyle = WS_EX_APPWINDOW;
        DWORD dwStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE;
        if (strcmp(className, NEWS_CLASS_NAME)      == 0
         || strcmp(className, ORDERS_CLASS_NAME)    == 0
         || strcmp(className, DIAMONDS_CLASS_NAME)  == 0
         || strcmp(className, TIMESALES_CLASS_NAME) == 0
         || strcmp(className, NEWS_ARTICLE_CLASS_NAME) == 0
        ) {
            dwStyle = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
        }
        if (strcmp(className, DEBUGLOG_CLASS_NAME) == 0) {
            dwExStyle = WS_EX_TOPMOST;
            dwStyle   = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
        }
        if (strcmp(className, BOOK_NEW_LIST_CLASS_NAME) == 0) {
            dwExStyle = WS_EX_DLGMODALFRAME | WS_EX_TOPMOST;
            dwStyle = WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE;
            hWndParent = g_AppWindows[BOOK_CLASS_NAME];
        }
        if (strcmp(className, NEWS_ARTICLE_CLASS_NAME) == 0) {
            dwExStyle = WS_EX_DLGMODALFRAME;
            dwStyle = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
            hWndParent = g_AppWindows[NEWS_CLASS_NAME];
        }
        hWnd = CreateWindowExA(dwExStyle, className, title, dwStyle, x, y, w, h, hWndParent, NULL, GetModuleHandle(NULL), NULL);   
    }
    
    if (strcmp(className, BOOK_NEW_LIST_CLASS_NAME) != 0)
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
    if (strcmp(className, DEBUGLOG_CLASS_NAME) == 0 || strcmp(className, BOOK_NEW_LIST_CLASS_NAME) == 0) {
        wc.hInstance = GetModuleHandle(NULL);
    } else {
        wc.hInstance = hInst;
    }
    wc.hIcon = onlineIcon;
    RegisterClass(&wc);
}

// ─── Window Session ───────────────────────────────────────────────────────────

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
        case WM_CREATE: {
            HICON hIcon = api.isConnected() ? onlineIcons[className] : offlineIcons[className];
            SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
            SendMessage(hWnd, WM_SETICON, ICON_BIG,   (LPARAM)hIcon);
            if (strcmp(className, DASHBOARD_CLASS_NAME) != 0) 
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
            if (strcmp(className, DASHBOARD_CLASS_NAME) == 0) {
                api.removeApiUpdateWindow(hWnd);
                api.clearApiErrorWindow(hWnd);
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
    if (alreadyEnsureGatewayRunning || !Settings_AutoGateway() || IsProcessRunning("ibgateway.exe")) return;
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

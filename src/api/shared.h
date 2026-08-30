#pragma once

NOTIFYICONDATAW nid = { 0 };

// ─── Colors ───────────────────────────────────────────────────────────────────
#define COINS_CLR_GREEN       RGB( 18, 220,  18)
#define COINS_CLR_GREEN_DARK  RGB(92, 214, 92)
#define COINS_CLR_GREEN_DARK2 RGB(32, 150, 32)
#define COINS_CLR_RED         RGB(220,  55,  55)
#define COINS_CLR_RED_DARK    RGB(217, 95, 95)
#define COINS_CLR_RED_DARK2   RGB(148, 33, 33)
#define COINS_CLR_WHITE       RGB(220, 220, 220)
#define COINS_CLR_BLACK       RGB(30,  30,  30)
#define COINS_CLR_GRAY        RGB(150, 150, 150)
#define COINS_CLR_BLUE        RGB(80, 160, 255)
#define COINS_CLR_PURPLE      RGB(185, 105, 225)
#define COINS_CLR_CYAN        RGB(0, 255, 255)
#define COINS_CLR_PINK        RGB(225, 105, 211)
#define COINS_CLR_ORANGE      RGB(255, 165, 0)
#define COINS_CLR_YELLOW      RGB(201, 183, 41)
// Dark background fills for order-side colored input boxes
#define COINS_BG_DARK_GREEN RGB( 34, 82, 50)
#define COINS_BG_DARK_RED   RGB(102, 43, 43)
// Sentinel: no custom col or – let HandleDarkModeMessages / system theme paint this control
#define COLOR_THEME   ((COLORREF)0xFFFFFFFF)

// glyph: E767 = Volume on (Segoe MDL2 Assets)
static const wchar_t SPEAKER_GLYPH[] = L"\uE767";
// glyph: E708 = QuietHours on (Segoe MDL2 Assets)
static const wchar_t MOON_GLYPH[] = L"\uE708";
// glyph: E72E = Lock on (Segoe MDL2 Assets)
static const wchar_t LOCK_GLYPH[] = L"\uE72E";
// glyph: E7C1 = Warning on (Segoe MDL2 Assets)
static const wchar_t RINGER_GLYPH[] = L"\uE7C1";

// glyph: E936 = FlickUp on (Segoe MDL2 Assets)
static const wchar_t BOTTOM_GLYPH[] = L"\uE936";
// glyph: E935 = FlickDown on (Segoe MDL2 Assets)
static const wchar_t TOP_GLYPH[] = L"\uE935";
// glyph: E937 = FlickLeft on (Segoe MDL2 Assets)
static const wchar_t RIGHT_GLYPH[] = L"\uE937";
// glyph: E938 = FlickRight on (Segoe MDL2 Assets)
static const wchar_t LEFT_GLYPH[] = L"\uE938";
// glyph: E945 = LightningBolt on (Segoe MDL2 Assets)
static const wchar_t LIGHT_GLYPH[] = L"\uE945";
// glyph: EC43 = MobLocation on (Segoe MDL2 Assets)
static const wchar_t LOCATE_GLYPH[] = L"\uEC43";

// ─── Lock hotkeys ──────────────────────────────────────────────────
static bool lockHotkeys = false;

// Force the MinGW linker to keep riched20.dll when compiling with -static
extern "C" __declspec(dllimport) long __stdcall CreateTextServices(void*, void*, void*);
static const void* force_riched20_link = (void*)CreateTextServices;

// ── Global TWS API instance ───────────────────────────────────────────────────────────
TradingAPI& api() {
    static TradingAPI* TWS_API = new TradingAPI();
    return *TWS_API;
}

// ── Label Colors ───────────────────────────────────────────────────────────
static void SetCtrlColor(HWND hw, COLORREF c) {
    if (!hw) return;
    if (c == COLOR_THEME) {
        RemovePropA(hw, "CtrlColor");
    } else {
        SetPropA(hw, "CtrlColor", (HANDLE)(uintptr_t)(c + 1));
    }
}
static COLORREF GetCtrlColor(HWND hw) {
    if (!hw) return COLOR_THEME;
    HANDLE h = GetPropA(hw, "CtrlColor");
    if (!h) return COLOR_THEME;
    return (COLORREF)((uintptr_t)h - 1);
}

void InitDarkBrushes() {
    hDarkBrush  = CreateSolidBrush(DM_BG);
    hDarkBrush2 = CreateSolidBrush(DM_BG2);
    hBrushDarkGreen = CreateSolidBrush(COINS_BG_DARK_GREEN);
    hBrushDarkRed   = CreateSolidBrush(COINS_BG_DARK_RED);

    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
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
    // Multi-instance windows (windowKey differs from className, e.g. market per-symbol)
    // are distinguished by title - each has a unique one. Single-instance windows match
    // on class alone. Either way no map needed: FindWindowA does the work.
    bool multiInstance = !windowKey.empty() && windowKey != className;
    HWND hWnd = NULL;
    if (multiInstance) {
        auto tsWindows = EnumerateMarketWindows();
        for (size_t i = 0; i < tsWindows.size() && i < 100; ++i) {
            TradingAPI::MarketInitData* data = (TradingAPI::MarketInitData*)GetWindowLongPtr(tsWindows[i].hWnd, GWLP_USERDATA);
            if (data && data->winKey == windowKey) {
                hWnd = tsWindows[i].hWnd;
                break;
            }
        }
    } else {
        hWnd = FindWindowA(className, NULL);
    }

    if (hWnd && IsWindow(hWnd)) {
        if (IsIconic(hWnd)) {
            ShowWindow(hWnd, SW_RESTORE);
            SetForegroundWindow(hWnd);
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
    
    LoadWinPosition(multiInstance ? windowKey.c_str() : className, x, y, w, h);

    if (hInst) { // dashboard window
        hWnd = CreateWindow(className, title, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, x, y, w, h, NULL, NULL, hInst, lpParam);
        ShowWindow(hWnd, SW_SHOW);
        UpdateWindow(hWnd);
    } else {
        HWND hWndParent = NULL;
        DWORD dwExStyle = WS_EX_APPWINDOW;
        DWORD dwStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE;
        if (strcmp(className, ORDERS_CLASS_NAME)    == 0
         || strcmp(className, DIAMONDS_CLASS_NAME)  == 0
         || strcmp(className, MARKET_CLASS_NAME)    == 0
        ) {
            dwStyle = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
        }
        if (strcmp(className, DEBUGLOG_CLASS_NAME) == 0) {
            dwExStyle = WS_EX_TOPMOST;
            dwStyle   = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
        }
        if (strcmp(className, DASHBOARD_EXCHANGE_CLASS_NAME) == 0) {
            dwExStyle = WS_EX_DLGMODALFRAME | WS_EX_TOPMOST;
            dwStyle   = WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE;
            hWndParent = FindWindowA(DASHBOARD_CLASS_NAME, NULL);
        }
        hWnd = CreateWindowExA(dwExStyle, className, title, dwStyle, x, y, w, h, hWndParent, NULL, GetModuleHandle(NULL), lpParam);
    }

    if (strcmp(className, DASHBOARD_EXCHANGE_CLASS_NAME) != 0)
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
static HFONT Coins_MakeMDL2Font(int ptSize) {
    HDC hdc = GetDC(NULL);
    int h   = -MulDiv(ptSize, GetDeviceCaps(hdc, LOGPIXELSY), 72);
    ReleaseDC(NULL, hdc);
    return CreateFontW(h, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe MDL2 Assets");
}

static HFONT hFont_Icons = Coins_MakeMDL2Font(11);   // Segoe MDL2 Assets for speaker/moon/lock glyphs

class ScopedFont {
    HFONT hFont = NULL;

public:
    // Constructor handles the DPI-aware height calculation and font creation directly
    ScopedFont(int ptSize, bool bold, const char* family = "Proxima Nova") {
        HDC hdc = GetDC(NULL);
        int h = -MulDiv(ptSize, GetDeviceCaps(hdc, LOGPIXELSY), 72);
        ReleaseDC(NULL, hdc);
        
        hFont = CreateFontA(
            h, 0, 0, 0,
            bold ? FW_BOLD : FW_NORMAL,
            FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, family
        );
    }

    // Destructor automatically cleans up the GDI resource
    ~ScopedFont() {
        if (hFont) {
            DeleteObject(hFont);
        }
    }

    // Prevent copying to avoid double-deletion crashes
    ScopedFont(const ScopedFont&) = delete;
    ScopedFont& operator=(const ScopedFont&) = delete;

    // Allow moving
    ScopedFont(ScopedFont&& other) noexcept : hFont(other.hFont) {
        other.hFont = NULL;
    }
    
    ScopedFont& operator=(ScopedFont&& other) noexcept {
        if (this != &other) {
            if (hFont) DeleteObject(hFont);
            hFont = other.hFont;
            other.hFont = NULL;
        }
        return *this;
    }

    // Implicit conversion to HFONT
    operator HFONT() const { return hFont; }
    
    // Explicit getter 
    HFONT get() const { return hFont; }
};

static ScopedFont hFont11pt(11, false);
static ScopedFont hFont11ptbold(11, true);
static ScopedFont hFont12pt(12, false);
static ScopedFont hFont12ptbold(12, true);
static ScopedFont hFont14pt(14, false);
static ScopedFont hFont14ptbold(14, true);
static ScopedFont hFont16pt(16, false);
static ScopedFont hFont16ptbold(16, true);
static ScopedFont hFont21ptbold(21, true);

// Suppresses WM_ERASEBKGND on list views so custom-draw repaints stay flicker-free.
// (Previously also handled Ctrl+MouseWheel zoom — that feature has been removed;
// this proc now only keeps the zero-flicker background-erase suppression.)
LRESULT CALLBACK ListViewNoFlickerProc(HWND hList, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    if (uMsg == WM_ERASEBKGND) {
        return 1; // Zero-flicker: suppress background erase entirely
    }
    return DefSubclassProc(hList, uMsg, wParam, lParam);
}

std::unordered_map<std::string, HICON> offlineIcons;
std::unordered_map<std::string, HICON> onlineIcons;

void RegisterWindowClass(HINSTANCE hInst, WNDPROC WndProc, const char* className, int iconId, bool isPopup = false) {
    if (hDarkBrush == NULL || hDarkBrush2 == NULL) InitDarkBrushes();
    HICON& offlineIcon = offlineIcons[std::string(className)];
    HICON& onlineIcon  = onlineIcons[std::string(className)];
    onlineIcon  = (HICON)LoadImage(hInst, MAKEINTRESOURCE(iconId), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
    offlineIcon = CreateGrayIcon(onlineIcon);
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc   = WndProc;
    wc.lpszClassName = className;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.hInstance     = isPopup ? GetModuleHandle(NULL) : hInst;
    wc.hIcon         = onlineIcon;
    RegisterClass(&wc);
}

static LRESULT CALLBACK DarkGroupBoxSubclassProc(HWND hCtrl, UINT msg, WPARAM wParam, LPARAM lParam,
                                                 UINT_PTR uIdSubclass, DWORD_PTR /*dwRefData*/) {
    if (msg == WM_PAINT && Settings_DarkMode()) {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hCtrl, &ps);
        RECT rc;
        GetClientRect(hCtrl, &rc);

        // 1. Fill background with dark mode background
        FillRect(hdc, &rc, hDarkBrush);

        // 2. Get the GroupBox text
        TCHAR text[256];
        int len = GetWindowText(hCtrl, text, 256);

        // 3. Get the control's font and select it into the DC
        HFONT hFont = (HFONT)SendMessage(hCtrl, WM_GETFONT, 0, 0);
        HGDIOBJ hOldFont = SelectObject(hdc, hFont);

        // 4. Calculate text dimensions to correctly place the border and text
        SIZE textSize = {0};
        if (len > 0) {
            GetTextExtentPoint32(hdc, text, len, &textSize);
        } else {
            textSize.cy = 14; // Fallback height if there is no text
        }

        // Define where the text will be placed (standard Win32 offset is usually left + 9)
        RECT textRect = { rc.left + 9, rc.top, rc.left + 9 + textSize.cx + 8, rc.top + textSize.cy };

        // 5. Exclude the text area so the border doesn't draw a line through our label
        if (len > 0) {
            ExcludeClipRect(hdc, textRect.left, textRect.top, textRect.right, textRect.bottom);
        }

        // 6. Draw gray border (RGB(100, 100, 100))
        HPEN hPen = CreatePen(PS_SOLID, 1, RGB(100, 100, 100));
        HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
        HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, (HBRUSH)GetStockObject(NULL_BRUSH));

        // Shift the top down by half the text height so the line intersects the vertical center of the text
        Rectangle(hdc, rc.left, rc.top + (textSize.cy / 2), rc.right, rc.bottom);

        SelectObject(hdc, hOldBrush);
        SelectObject(hdc, hOldPen);
        DeleteObject(hPen);

        // 7. Restore the clipping region so we can draw the text
        SelectClipRgn(hdc, NULL);

        // 8. Draw the text
        if (len > 0) {
            SetTextColor(hdc, RGB(255, 255, 255)); // Set to your preferred dark mode text color
            SetBkMode(hdc, TRANSPARENT);
            
            // Shift slightly to the right for padding inside the clipped area
            RECT drawRect = textRect;
            drawRect.left += 2;
            DrawText(hdc, text, len, &drawRect, DT_LEFT | DT_TOP | DT_SINGLELINE);
        }

        // Cleanup font
        SelectObject(hdc, hOldFont);

        EndPaint(hCtrl, &ps);
        return 0;
    }
    if (msg == WM_NCDESTROY) {
        RemoveWindowSubclass(hCtrl, DarkGroupBoxSubclassProc, uIdSubclass);
    }
    return DefSubclassProc(hCtrl, msg, wParam, lParam);
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
            if (strcmp(className, DASHBOARD_CLASS_NAME) == 0 || strcmp(className, MARKET_CLASS_NAME) == 0 || strcmp(className, DIAMONDS_CLASS_NAME) == 0 || strcmp(className, ORDERS_CLASS_NAME) == 0) {
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
            HICON hIcon = api().isConnected() ? onlineIcons[std::string(className)] : offlineIcons[std::string(className)];
            SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
            SendMessage(hWnd, WM_SETICON, ICON_BIG,   (LPARAM)hIcon);
            Session_AddWindow(hWnd, lParam);
            return 0;
        }
        case WM_CLOSE:
#ifndef GATEWAY_SIM
            if (strcmp(className, DASHBOARD_CLASS_NAME) == 0) {
                ShowWindow(hWnd, SW_HIDE);
                ToggleTWS(SW_HIDE);
            }
            else
#endif
                DestroyWindow(hWnd);
            return 0;
        case WM_SIZE:
        case WM_MOVE:
            SaveWinPosition(hWnd);
            return DefWindowProc(hWnd, message, wParam, lParam);
        case WM_DESTROY:
            SaveWinPosition(hWnd);
            if (strcmp(className, DASHBOARD_CLASS_NAME) != 0) {
                Session_RemoveWindow(hWnd);
            }
            if (strcmp(className, MARKET_CLASS_NAME) == 0) {
                TradingAPI::MarketInitData* data = (TradingAPI::MarketInitData*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
                if (data) delete data;
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

std::string FormatWithCommas(double value, bool showDecimals = true) {
    // Format to 2 decimal places first
    std::string s = showDecimals ? std::format("{:.2f}", value) : std::format("{:.0f}", value);

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

std::string formatVolume(long long volume) {
    if (volume < 0) return "0"; // guard against bad/sentinel data

    const long long THOUSAND = 1'000LL;
    const long long MILLION  = 1'000'000LL;
    const long long BILLION  = 1'000'000'000LL;
    const long long TRILLION = 1'000'000'000'000LL;

    auto formatScaled = [](double value, char suffix) -> std::string {
        char buf[32];
        // 1 decimal place, but drop it if it's a whole number (e.g. "5M" not "5.0M")
        if ((suffix == 'K' && value >= 1) || std::fabs(value - std::round(value)) < 0.05) {
            std::snprintf(buf, sizeof(buf), "%.0f%c", value, suffix);
        } else {
            std::snprintf(buf, sizeof(buf), "%.1f%c", value, suffix);
        }
        return std::string(buf);
    };

    if (volume >= TRILLION) {
        return formatScaled(static_cast<double>(volume) / TRILLION, 'T');
    } else if (volume >= BILLION) {
        return formatScaled(static_cast<double>(volume) / BILLION, 'B');
    } else if (volume >= MILLION) {
        return formatScaled(static_cast<double>(volume) / MILLION, 'M');
    } else if (volume >= THOUSAND) {
        return formatScaled(static_cast<double>(volume) / THOUSAND, 'K');
    } else {
        return std::to_string(volume);
    }
}

void CenterEditText(HWND hEdit) {
    if (!hEdit) return;
    RECT erc = {};
    GetClientRect(hEdit, &erc);
    HDC hdc = GetDC(hEdit);
    HFONT hFont = (HFONT)SendMessage(hEdit, WM_GETFONT, 0, 0);
    HGDIOBJ oldFont = SelectObject(hdc, hFont ? hFont : GetStockObject(SYSTEM_FONT));
    TEXTMETRICA tm = {};
    GetTextMetricsA(hdc, &tm);
    SelectObject(hdc, oldFont);
    ReleaseDC(hEdit, hdc);
    int pad = std::max(0, (int)((erc.bottom - erc.top) - tm.tmHeight) / 2 - 2);
    RECT rect = { 0, pad, erc.right, erc.bottom - pad };
    SendMessageA(hEdit, EM_SETRECTNP, 0, (LPARAM)&rect);
}
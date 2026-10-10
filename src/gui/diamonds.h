#pragma once
// "Proxima Nova", Verdana, Arial, sans-serif
static const int windowDiamondsWidth = 1030;
void StartDiamonds() { StartGenericWindow(DIAMONDS_CLASS_NAME, "Diamonds", L"TWSAPIClientTradingFloor.Diamonds", windowDiamondsWidth, 420); }

#define ID_DIAMONDS_RESULTS_LIST    7001
#define ID_VIEW_SELECTIONS_ANY_BTN  7002
#define ID_VIEW_SELECTIONS_TOP_BTN  7003

#define ID_DIAMONDS_CHK_0           7010   // "Growth"
#define ID_DIAMONDS_CHK_1           7011   // "Dividends"
#define ID_DIAMONDS_CHK_2           7012   // "Quarantine"
#define ID_DIAMONDS_ROW_TIMECLOCK   7013

#define DIAMONDS_CHK_STRIP_H        32     // height of the checkbox bar at the bottom


// ── Deferred sort (prevents flicker on every tick) ────────────────────────────
#define TIMER_DIAMONDS_VIEW      7110
#define DIAMONDS_VIEW_TIMER_MS   5000

#define TIMER_DIAMONDS_SORT      7111
#define DIAMONDS_SORT_TIMER_MS   7000   // re-sort at most every 7 seconds (or sooner if user clicks a column header)

#define TIMER_DIAMONDS_PAINT     7112
#define DIAMONDS_PAINT_TIMER_MS  60     // ~16 FPS (Butter smooth, zero flicker)

#define TIMER_DIAMONDS_CAPTION   7113
#define DIAMONDS_CAPTION_TIMER_MS 60   // lets DWM publish updated caption-button bounds

// ── Filter / tab constants ────────────────────────────────────────────────────
#define DTAB_ALL              0
#define DTAB_GROWTH           1
#define DTAB_QUARENTINE       2
#define DIAMONDS_TAB_COUNT    3

static const char* diamondTabNames[DIAMONDS_TAB_COUNT] = { "Growth", "Dividends", "Quarantine" };

static const size_t DIAMONDS_TITLE_EVENTS_MAX = 21;

// ── Symbol color palette ──────────────────────────────────────────────────────
// Index 0-5 = named colors.  No entry in the map (or index -1) = inherit theme.
#define DIAMONDS_COLOR_COUNT  6
#define DIAMONDS_COLOR_NONE  -1   // sentinel: remove override, inherit by theme

struct DiamondsColorDef { COLORREF rgb; const char* label; };
static const DiamondsColorDef diamondColorPalette[DIAMONDS_COLOR_COUNT] = {
    { RGB(159,  27,  27), "Set Color: Red"    },
    { RGB( 18, 220,  18), "Set Color: Green"  },
    { RGB(  0, 167, 255), "Set Color: Blue"   },
    { RGB(167,  84, 212), "Set Color: Purple" },
    { RGB(255, 215,   0), "Set Color: Gold"   },
    { RGB(163, 104,  14), "Set Color: Brown"  },
};



// ── Column indices (keep in sync with diamondCols[]) ─────────────────────────
enum DiamondColIdx {
    DCOL_FAKE = 0,
    DCOL_MKTVAL,
    DCOL_SYMBOL,
    DCOL_POSITION,
    DCOL_AVGPRICE,
    DCOL_ALERT,
    DCOL_LAST,
    DCOL_BIDASK,
    DCOL_SIZE,
    DCOL_VWAP,
    DCOL_CHG5MIN,
    DCOL_VOLRATE,
    DCOL_DAILYPNL,
    DCOL_CHGPCT,
    DCOL_CHG13WEEK,
    DCOL_CHG26WEEK,
    DCOL_CHG52WEEK,
    DCOL_UNREALIZED_PL,
    DCOL_DIV_YIELD,
    DCOL_DIV_DATE,
    DCOL_DIV_AMT,
    DCOL_ANNUAL_DIV,
    DCOL_EXCHANGE,
    DCOL_COUNT
};

// ── Unified Virtual List Cache (Replaces diamondsPnlCache) ─────────────────
struct DiamondRowCache {
    int conId = 0;
    std::string symbol;
    double sortValues[DCOL_COUNT] = {0.0};  // Raw doubles for fast sorting
    std::string textCols[DCOL_COUNT];       // Pre-formatted strings for instant UI painting

    // Day high/low, used only to color DCOL_LAST based on where `last` sits
    // within today's range (see WM_NOTIFY / NM_CUSTOMDRAW).
    double dayHigh = 0.0;
    double dayLow = 0.0;
    double prevClose = 0.0;
    bool halted = false;
    double bidSize = 0.0;  // for DCOL_SIZE multi-line display
    double askSize = 0.0;  // for DCOL_SIZE multi-line display
    std::string bidSizeStr = "";
    std::string askSizeStr = "";
    double bid = 0.0;
    double ask = 0.0;
    std::string bidStr = "";
    std::string askStr = "";
    std::string pctNetLiq = "";  // for DCOL_MKTVAL multi-line display
    std::string upStr = "";  // for DCOL_ALERT multi-line display
    std::string downStr = "";  // for DCOL_ALERT multi-line display
    double upAlert = 0.0;  // for DCOL_ALERT multi-line display
    double downAlert = 0.0;  // for DCOL_ALERT multi-line display
    std::string unrealizedPnLPctStr = "";
    double volRatio = 0.0; // used to color DCOL_VOLRATE
};

// Weekly reference closes are immutable once received for a conId. Keep them
// outside the live portfolio map so market-data updates do not need to lock it
// after the values have been populated.
struct DiamondsWeeklyCloseCache {
    double closeAgo13Week = 0.0;
    double closeAgo26Week = 0.0;
    double closeAgo52Week = 0.0;
    ULONGLONG lastAttemptMs = 0;   // PERF: throttle re-locking portfolioMutex
};

// Registry dividend cache, read once per symbol, with negative-result throttling.
struct DiamondsDividendCache {
    bool loaded = false;            // true once a registry value was successfully read
    double annual = 0.0, amount = 0.0, dateSortable = 0.0;
    std::string date;
    ULONGLONG lastAttemptMs = 0;    // throttles retries for symbols with no cached entry
};

struct DiamondsTitleEvent {
    std::string text;
    COLORREF color; // 0 = theme text
    int conId;
    std::string symbol;
};

struct DiamondsState {
    // Ephemeral storage for triggered alerts to prevent spamming
    std::unordered_set<int> firedAlertsUp;
    std::unordered_set<int> firedAlertsDown;
    // Bitmask: bit N set means group N is currently visible.  Default = all visible.
    UINT checkedTabs = 0x7;
    // Maps conId → assigned group (DTAB_ALL = untagged = shown when bit 0 is set).
    std::unordered_map<int, int> tabMap;
    // Alert values are refreshed once at startup and whenever the alert editor
    // notifies this window. Repopulation and row updates read this snapshot only.
    std::unordered_map<int, AlertEntry> alertCache;
    // Maps conId → color index (0..DIAMONDS_COLOR_COUNT-1), or not present = inherit.
    std::unordered_map<int, int> symbolColors;
    bool checkboxesVisible = false;
    // ── Sort state ────────────────────────────────────────────────────────────────
    int sortCol = DCOL_SYMBOL;
    bool sortAsc = true;
    // Keyed by conId. Populated / updated in Diamonds_UpdateMarketCols.
    std::unordered_map<int, MiniSparkline> sparklines;
    std::unordered_map<int, DiamondsWeeklyCloseCache> weeklyCloseCache;
    std::unordered_map<int, DiamondsDividendCache>    dividendCache;   // NEW
    // Data storage: Fast O(1) lookup by conId for live data streams
    std::unordered_map<int, DiamondRowCache> dataCache;
    // The list view sends one row-level custom-draw notification before that
    // row's subitems, so reuse its cache entry throughout the subitem callbacks.
    const DiamondRowCache* paintRowCache = nullptr;
    // UI Viewport: Holds conIds in sorted order. The ListView only knows about this vector's size.
    std::vector<int> displayOrder;
    // Full visible-range redraw (sort order changed).
    bool dirty = false;
    // Per-row dirty tracking (UI thread only).
    // dirtyMarket: L1 tick arrived, row cache must be rebuilt from getMarketData().
    // dirtyRedraw: row's cache is current (or only PnL changed), row just needs repainting.
    std::unordered_set<int> dirtyMarket;
    std::unordered_set<int> dirtyRedraw;
    HIMAGELIST rowHeightImageList = NULL;
    int viewSelectionEnabled = 0;
    std::deque<DiamondsTitleEvent> diamondsTitleEvents;
    // Client-space bounds of drawn title events and their associated conIds.
    std::vector<std::pair<RECT, int>> titleHitRects;
};

static DiamondsState diamondsState;

static int Diamonds_FrameY(HWND hWnd) {
    UINT dpi = GetDpiForWindow(hWnd);
    return GetSystemMetricsForDpi(SM_CYFRAME, dpi) + GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);
}

// Height of the area we own at the top of the client rect.
static int Diamonds_CaptionH(HWND hWnd) {
    UINT dpi = GetDpiForWindow(hWnd);
    int height = GetSystemMetricsForDpi(SM_CYCAPTION, dpi) + Diamonds_FrameY(hWnd);
    return std::max(1, height - MulDiv(4, dpi, 96));
}

static void Diamonds_UpdateFrameMargins(HWND hWnd) {
    MARGINS m = { 0, 0, Diamonds_CaptionH(hWnd), 0 };
    DwmExtendFrameIntoClientArea(hWnd, &m);
}

static void Diamonds_PaintCaption(HWND hWnd, HDC hdcWin) {
    diamondsState.titleHitRects.clear();

    RECT rc;
    GetClientRect(hWnd, &rc);
    const int w = rc.right, capH = Diamonds_CaptionH(hWnd);
    if (w <= 0 || capH <= 0) return;

    RECT btn = {};
    DwmGetWindowAttribute(hWnd, DWMWA_CAPTION_BUTTON_BOUNDS, &btn, sizeof(btn));
    const int maxX = (btn.left > 0 ? btn.left : w - 140) - 8;
    const int iconW = GetSystemMetrics(SM_CXSMICON);
    const int iconH = GetSystemMetrics(SM_CYSMICON);
    const int iconX = 8;
    int x = iconX + iconW + 8;

    BITMAPINFO bi = {};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = w;
    bi.bmiHeader.biHeight = -capH;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;

    HDC mem = CreateCompatibleDC(hdcWin);
    if (!mem) return;
    void* bits = nullptr;
    HBITMAP dib = CreateDIBSection(hdcWin, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (!dib || !bits) {
        if (dib) DeleteObject(dib);
        DeleteDC(mem);
        return;
    }
    std::memset(bits, 0, static_cast<size_t>(w) * capH * 4);

    HGDIOBJ oldBmp = SelectObject(mem, dib);
    HGDIOBJ oldFont = SelectObject(mem, hFont11ptbold.get());

    HICON hIcon = (HICON)SendMessage(hWnd, WM_GETICON, ICON_SMALL, 0);
    if (!hIcon) hIcon = (HICON)GetClassLongPtr(hWnd, GCLP_HICONSM);
    if (hIcon) {
        const int iconY = (capH - iconH) / 2;
        DrawIconEx(mem, iconX, iconY, hIcon, iconW, iconH, 0, NULL, DI_NORMAL);

        DWORD* px = (DWORD*)bits;
        for (int yy = std::max(0, iconY); yy < std::min(capH, iconY + iconH); ++yy) {
            for (int xx = iconX; xx < std::min(w, iconX + iconW); ++xx) {
                DWORD& p = px[(size_t)yy * w + xx];
                if ((p >> 24) == 0 && (p & 0x00FFFFFF) != 0) p |= 0xFF000000;
            }
        }
    }

    HTHEME theme = OpenThemeData(hWnd, L"CompositedWindow::Window");
    if (theme) {
        const COLORREF themeText = darkMode ? DM_TEXT : LM_TEXT;
        auto drawSeg = [&](const std::string& s, COLORREF clr, int conId) -> bool {
            std::wstring ws = StringToWide(s);
            SIZE sz;
            GetTextExtentPoint32W(mem, ws.c_str(), static_cast<int>(ws.size()), &sz);
            if (x + sz.cx > maxX) return false;
            RECT r = { x, 6, x + sz.cx, capH - 6 };
            DTTOPTS opts = { sizeof(opts) };
            opts.dwFlags = DTT_COMPOSITED | DTT_TEXTCOLOR;
            opts.crText = clr ? clr : themeText;
            DrawThemeTextEx(theme, mem, 0, 0, ws.c_str(), -1, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX, &r, &opts);
            if (conId > 0) diamondsState.titleHitRects.push_back({ r, conId });
            x += sz.cx + 14;
            return true;
        };
        if (diamondsState.diamondsTitleEvents.empty()) {
            drawSeg("Today, is a beautiful day.", 0, 0);
        } else {
            for (const auto& ev : diamondsState.diamondsTitleEvents)
                if (!drawSeg(ev.text, ev.color, ev.conId)) break;
        }
        CloseThemeData(theme);
    }

    BitBlt(hdcWin, 0, 0, w, capH, mem, 0, 0, SRCCOPY);
    SelectObject(mem, oldFont);
    SelectObject(mem, oldBmp);
    DeleteObject(dib);
    DeleteDC(mem);
}

// Returns the conId of the title event under a client-space point, or 0.
static int Diamonds_TitleHitTest(POINT clientPt) {
    for (const auto& [r, conId] : diamondsState.titleHitRects)
        if (PtInRect(&r, clientPt)) return conId;
    return 0;
}

// Repaint the custom caption so it is clipped against the current caption-button bounds.
static void Diamonds_InvalidateCaption(HWND hWnd) {
    RECT rc;
    GetClientRect(hWnd, &rc);
    rc.bottom = Diamonds_CaptionH(hWnd);
    InvalidateRect(hWnd, &rc, TRUE);
}

static void Diamonds_UpdateEventTitle(HWND hWnd) {
    // Keep a plain-text title for the taskbar / Alt-Tab; the colored one is painted by us.
    std::string title;
    for (const auto& ev : diamondsState.diamondsTitleEvents) {
        if (!title.empty()) title += "  ";
        title += ev.text;
    }
    if (title.empty()) title = "Today, is a beautiful day.";
    SetWindowTextA(hWnd, title.c_str());
    Diamonds_InvalidateCaption(hWnd);
}

static void Diamonds_AddTitleEvent(const std::string& symbol, int conId, COLORREF color, const std::string& price) {
    std::string displayPrice = price;
    if (displayPrice.size() >= 3 && displayPrice.compare(displayPrice.size() - 3, 3, ".00") == 0)
        displayPrice.erase(displayPrice.size() - 3);

    diamondsState.diamondsTitleEvents.push_front({ symbol + " " + displayPrice, color, conId, symbol });
    while (diamondsState.diamondsTitleEvents.size() > DIAMONDS_TITLE_EVENTS_MAX) diamondsState.diamondsTitleEvents.pop_back();

    HWND hWnd = FindWindowA(DIAMONDS_CLASS_NAME, NULL);
    if (hWnd && IsWindow(hWnd))
        Diamonds_UpdateEventTitle(hWnd);
}

static void Diamonds_RefreshAlertCache() {
    diamondsState.alertCache.clear();
    for (auto const& alert : Settings_Alerts_LoadAll())
        diamondsState.alertCache[alert.conId] = alert;
}

static DiamondsWeeklyCloseCache Diamonds_GetWeeklyCloseCache(int conId) {
    auto& cached = diamondsState.weeklyCloseCache[conId];
    if (cached.closeAgo13Week > 0.0 && cached.closeAgo26Week > 0.0 && cached.closeAgo52Week > 0.0)
        return cached;

    ULONGLONG now = GetTickCount64();
    if (now - cached.lastAttemptMs < 7000) return cached;
    cached.lastAttemptMs = now;

    std::lock_guard<std::mutex> lock(api().getPortfolioMutex());
    auto& portfolio = api().getPortfolioMap();
    auto it = portfolio.find(conId);
    if (it != portfolio.end()) {
        if (it->second.closeAgo13Week > 0.0) cached.closeAgo13Week = it->second.closeAgo13Week;
        if (it->second.closeAgo26Week > 0.0) cached.closeAgo26Week = it->second.closeAgo26Week;
        if (it->second.closeAgo52Week > 0.0) cached.closeAgo52Week = it->second.closeAgo52Week;
    }
    return cached;
}

// ── Column definitions ────────────────────────────────────────────────────────

struct DiamondCol { const char* header; int width; int fmt; };
static const DiamondCol diamondCols[] = {
    { "Fake",               0, LVCFMT_RIGHT },
    { "Value",             70, LVCFMT_RIGHT },
    { "Symbol",            90, LVCFMT_LEFT  },
    { "Position",         110, LVCFMT_RIGHT },
    { "AvgPx",             85, LVCFMT_RIGHT },
    { "Alert",             45, LVCFMT_RIGHT },
    { "Last",              90, LVCFMT_RIGHT },
    { "Price",             65, LVCFMT_RIGHT },
    { "Size",              50, LVCFMT_RIGHT },
    { "VWAP",              70, LVCFMT_RIGHT },
    { "5m",                70, LVCFMT_RIGHT },
    { "Vol",               60, LVCFMT_RIGHT },
    { "Daily",             90, LVCFMT_RIGHT },  // {"fix_tag":7681,"name":"Price/EMA(20)","description":"Price to Exponential moving average (N = 20) ratio - 1, displayed in percents","groups":["G40"],"id":"PRICE_VS_EMA20"}
    { "Change %",          95, LVCFMT_RIGHT },  // {"fix_tag":7679,"name":"Price/EMA(100)","description":"Price to Exponential moving average (N = 100) ratio - 1, displayed in percents","groups":["G40"],"id":"PRICE_VS_EMA100"}
    { "13w",              115, LVCFMT_RIGHT },
    { "26w",              115, LVCFMT_RIGHT },
    { "52w",              115, LVCFMT_RIGHT },
    { "Unrealized",        85, LVCFMT_RIGHT },
    { "Yield %",           90, LVCFMT_RIGHT },
    { "Date",             125, LVCFMT_RIGHT },
    { "Amount",            85, LVCFMT_RIGHT },
    { "Annual",            85, LVCFMT_RIGHT },
    { "Exchange",          90, LVCFMT_CENTER },
    // {"fix_tag":7290,"name":"P/E excluding extraordinary items","description":"This ratio is calculated by dividing the current Price by the sum of the Diluted Earnings Per Share from continuing operations BEFORE Extraordinary Items and Accounting Changes over the last four interim periods.","groups":["G15"],"id":"PE"}
    // {"fix_tag":7281,"name":"Category","description":"Displays a more detailed level of description within the industry under which the underlying company can be categorized.","groups":["G-3"],"id":"CATEGORY"}
    // {"fix_tag":7289,"name":"Market capitalization","description":"This value is calculated by multiplying the current Price by the current number of Shares Outstanding.","groups":["G15"],"id":"MKT_CAP"}
};
static_assert((int)(sizeof(diamondCols) / sizeof(diamondCols[0])) == DCOL_COUNT,
              "diamondCols count must match DiamondColIdx::DCOL_COUNT");

// Dividend columns (Yield/Date/Amount/Annual) show when "Dividends"
// (bit 1) is checked. 13w/26w/52w change columns show when "Quarantine"
// (bit 2) is checked. Each group is hidden by collapsing its column widths to
// 0 and restored to diamondCols[]'s defined width when its tab is checked.
// The window is resized to fit however many groups are currently visible.
static void Diamonds_UpdateDivColumnsVisibility(HWND hWnd) {
    HWND hList = GetDlgItem(hWnd, ID_DIAMONDS_RESULTS_LIST);
    if (!hList) return;

    bool isMaximized = false;

    WINDOWPLACEMENT wp;
    wp.length = sizeof(WINDOWPLACEMENT);
    if (GetWindowPlacement(hWnd, &wp)) {
        isMaximized = wp.showCmd == SW_SHOWMAXIMIZED;
    }

    bool showDiv   = false;
    bool showWeeks = false;
    if (isMaximized) {
        showDiv   = (diamondsState.checkedTabs & (1u << 1)) != 0;   // Dividends
        showWeeks = (diamondsState.checkedTabs & (1u << 2)) != 0;   // Quarantine
    }

    for (int i = DCOL_DIV_YIELD; i <= DCOL_ANNUAL_DIV; ++i) {
        ListView_SetColumnWidth(hList, i, showDiv ? diamondCols[i].width : 0);
    }
    for (int i = DCOL_CHG13WEEK; i <= DCOL_CHG52WEEK; ++i) {
        ListView_SetColumnWidth(hList, i, showWeeks ? diamondCols[i].width : 0);
    }
    ListView_SetColumnWidth(hList, DCOL_AVGPRICE, showWeeks ? diamondCols[DCOL_AVGPRICE].width : 0);
    ListView_SetColumnWidth(hList, DCOL_EXCHANGE, showWeeks ? diamondCols[DCOL_EXCHANGE].width : 0);

    // Sum the extra width needed for each currently-visible group.
    int extraWidth = 0;
    if (showDiv) {
        extraWidth += diamondCols[DCOL_DIV_YIELD].width   + diamondCols[DCOL_DIV_DATE].width +
                      diamondCols[DCOL_DIV_AMT].width      + diamondCols[DCOL_ANNUAL_DIV].width;
    }
    if (showWeeks) {
        extraWidth += diamondCols[DCOL_CHG13WEEK].width + diamondCols[DCOL_CHG26WEEK].width +
                      diamondCols[DCOL_CHG52WEEK].width + 
                      diamondCols[DCOL_AVGPRICE].width + 
                      diamondCols[DCOL_EXCHANGE].width;
    }
    if (extraWidth > 0) extraWidth += 10; // margin, same buffer the original single-group case used

    RECT windowRect;
    GetWindowRect(hWnd, &windowRect);
    int left = windowRect.left;

    if (isMaximized) {    
        RECT workArea;
        SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);
        left = workArea.left - workArea.left + ((workArea.right - workArea.left - windowDiamondsWidth - extraWidth)/2); // relative to monitor, or workArea.left
    }

    MoveWindow(hWnd, left, windowRect.top, windowDiamondsWidth + extraWidth, windowRect.bottom - windowRect.top, TRUE);
}


static void Diamonds_SetRowHeight(HWND hList, int rowHeight) {
    if (diamondsState.rowHeightImageList) {
        ImageList_Destroy(diamondsState.rowHeightImageList);
        diamondsState.rowHeightImageList = NULL;
    }
    // Width can stay tiny (1px) since LVS_REPORT never shows the icon glyph
    // area when there's no LVCFMT_IMAGE column, but height controls row height.
    diamondsState.rowHeightImageList = ImageList_Create(1, rowHeight, ILC_COLOR32 | ILC_MASK, 1, 1);
    if (!diamondsState.rowHeightImageList) return;

    // Add one fully-transparent 1x1 bitmap so the image list is non-empty.
    HBITMAP hbmImage = CreateBitmap(1, rowHeight, 1, 1, NULL);
    HBITMAP hbmMask  = CreateBitmap(1, rowHeight, 1, 1, NULL);
    ImageList_Add(diamondsState.rowHeightImageList, hbmImage, hbmMask);
    DeleteObject(hbmImage);
    DeleteObject(hbmMask);

    ListView_SetImageList(hList, diamondsState.rowHeightImageList, LVSIL_SMALL);
}

// ── Registry persistence for tab assignments ──────────────────────────────────

// Saves diamondsState.tabMap to the registry as two space-separated conId lists.
static void Diamonds_SaveTabMap() {
    std::string growthList, quarentineList;
    for (auto& [conId, tab] : diamondsState.tabMap) {
        if (tab == DTAB_GROWTH) {
            if (!growthList.empty()) growthList += ' ';
            growthList += std::to_string(conId);
        } else if (tab == DTAB_QUARENTINE) {
            if (!quarentineList.empty()) quarentineList += ' ';
            quarentineList += std::to_string(conId);
        }
    }
    Settings_Tab_Save("Tab_Dividends",  growthList);
    Settings_Tab_Save("Tab_Quarantine", quarentineList);
}

// Loads diamondsState.tabMap from the registry.
static void Diamonds_LoadTabMap() {
    diamondsState.tabMap.clear();
    auto parseIds = [](const std::string& s, int tab) {
        size_t start = 0;
        while (start < s.size()) {
            size_t end = s.find(' ', start);
            if (end == std::string::npos) end = s.size();
            if (end > start) {
                try { diamondsState.tabMap[std::stoi(s.substr(start, end - start))] = tab; }
                catch (...) {}
            }
            start = end + 1;
        }
    };
    parseIds(Settings_Tab_Load("Tab_Dividends"),  DTAB_GROWTH);
    parseIds(Settings_Tab_Load("Tab_Quarantine"), DTAB_QUARENTINE);
}

// ── Symbol color persistence ──────────────────────────────────────────────────

static void Diamonds_SaveSymbolColors() {
    std::string s;
    for (auto& [conId, idx] : diamondsState.symbolColors) {
        if (!s.empty()) s += ' ';
        s += std::to_string(conId) + ':' + std::to_string(idx);
    }
    Settings_SymbolColors_Save(s);
}

static void Diamonds_LoadSymbolColors() {
    diamondsState.symbolColors.clear();
    std::string s = Settings_SymbolColors_Load();
    size_t pos = 0;
    while (pos < s.size()) {
        size_t end = s.find(' ', pos);
        if (end == std::string::npos) end = s.size();
        std::string tok = s.substr(pos, end - pos);
        auto colon = tok.find(':');
        if (colon != std::string::npos) {
            try {
                int conId = std::stoi(tok.substr(0, colon));
                int idx   = std::stoi(tok.substr(colon + 1));
                if (idx >= 0 && idx < DIAMONDS_COLOR_COUNT)
                    diamondsState.symbolColors[conId] = idx;
            } catch (...) {}
        }
        pos = end + 1;
    }
}

// Drops diamondsState.tabMap entries for conIds that are no longer a held
// position, then persists the pruned map. Also drops diamondsState.symbolColors
// entries, but only when the symbol is neither a current position NOR has an
// alert set, a symbol with an alert is allowed to keep a color override
// even while not held.
static void Diamonds_CleanupStaleTabAssignments() {
    std::unordered_set<int> liveConIds;
    {
        std::lock_guard<std::mutex> lock(api().getPortfolioMutex());
        for (auto const& [conId, info] : api().getPortfolioMap())
            liveConIds.insert(conId);
    }

    if (!diamondsState.tabMap.empty()) {
        bool changedTabs = false;
        for (auto it = diamondsState.tabMap.begin(); it != diamondsState.tabMap.end(); ) {
            if (!liveConIds.count(it->first)) { it = diamondsState.tabMap.erase(it); changedTabs = true; }
            else ++it;
        }
        if (changedTabs) Diamonds_SaveTabMap();
    }

    if (!diamondsState.symbolColors.empty()) {
        bool changedColors = false;
        for (auto it = diamondsState.symbolColors.begin(); it != diamondsState.symbolColors.end(); ) {
            bool isLive = liveConIds.count(it->first) != 0;
            bool hasAlert = false;
            if (!isLive) {
                hasAlert = diamondsState.alertCache.find(it->first) != diamondsState.alertCache.end();
            }
            if (!isLive && !hasAlert) { it = diamondsState.symbolColors.erase(it); changedColors = true; }
            else ++it;
        }
        if (changedColors) Diamonds_SaveSymbolColors();
    }
}

// Drops "Dividends" registry values (written by Settings_Dividends_Save, see
// registry.h) for conIds that are no longer a held position. That subkey is a
// pure fetch-once-per-session cache keyed by SYMBOL_CONID with nothing else
// pruning it, so closed-out positions would otherwise accumulate there forever
//, same rationale as Diamonds_CleanupStaleTabAssignments() above, just aimed
// at a different registry subkey (Dividends instead of Tab_*/SymbolColors).
static void Diamonds_CleanupStaleDividends() {
    std::unordered_set<int> liveConIds;
    {
        std::lock_guard<std::mutex> lock(api().getPortfolioMutex());
        for (auto const& [conId, info] : api().getPortfolioMap())
            liveConIds.insert(conId);
    }

    HKEY hKey;
    std::string fullPath = std::format("{}\\Dividends", APP_REG_ROOT);
    if (RegOpenKeyExA(HKEY_CURRENT_USER, fullPath.c_str(), 0, KEY_QUERY_VALUE | KEY_SET_VALUE, &hKey) != ERROR_SUCCESS)
        return; // no Dividends subkey yet, nothing to clean

    std::vector<std::string> toDelete;
    char valueName[128];
    DWORD index = 0;
    while (true) {
        DWORD nameSize = sizeof(valueName);
        if (RegEnumValueA(hKey, index++, valueName, &nameSize, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
            break;

        std::string name(valueName);
        size_t underscore = name.rfind('_');
        if (underscore == std::string::npos || underscore == name.size() - 1) continue; // not SYMBOL_CONID shaped, leave alone

        int conId = 0;
        try { conId = std::stoi(name.substr(underscore + 1)); }
        catch (...) { continue; } // trailing part isn't a conId, leave it alone

        if (!liveConIds.count(conId))
            toDelete.push_back(name);
    }
    RegCloseKey(hKey);

    for (const auto& name : toDelete)
        RegDelete("Dividends", name.c_str());
}

static void Disamonds_SetTime(const std::string& time_str) {
    HWND hDiamonds = FindWindowA(DIAMONDS_CLASS_NAME, NULL);
    if (hDiamonds && IsWindow(hDiamonds)) {
        SetWindowTextA(GetDlgItem(hDiamonds, ID_DIAMONDS_ROW_TIMECLOCK), time_str.c_str());
    }
}

// ── Layout ────────────────────────────────────────────────────────────────────

static void Diamonds_Layout(HWND hWnd) {
    HWND hList = GetDlgItem(hWnd, ID_DIAMONDS_RESULTS_LIST);
    if (!hList) return;
    RECT rc; GetClientRect(hWnd, &rc);
    const int capH = Diamonds_CaptionH(hWnd);
    int listH = diamondsState.checkboxesVisible ? rc.bottom - DIAMONDS_CHK_STRIP_H : rc.bottom;
    MoveWindow(hList, 0, capH, rc.right, listH - capH, TRUE);

    if (!diamondsState.checkboxesVisible) return;

    // Space the three checkboxes evenly across the bottom strip.
    static const int chkW[DIAMONDS_TAB_COUNT] = { 70, 90, 90 };
    int totalW = 0;
    for (int w : chkW) totalW += w;
    int startX = (rc.right - totalW) / 2;
    int y = listH + (DIAMONDS_CHK_STRIP_H - 20) / 2;
    int x = startX;
    for (int i = 0; i < DIAMONDS_TAB_COUNT; ++i) {
        SetWindowPos(GetDlgItem(hWnd, ID_DIAMONDS_CHK_0 + i), NULL, x + (i * 20), y, chkW[i], 20, SWP_NOZORDER | SWP_NOACTIVATE);
        x += chkW[i];
    }
    SetWindowPos(GetDlgItem(hWnd, ID_VIEW_SELECTIONS_TOP_BTN), NULL, 5, y, 40, 22, SWP_NOZORDER | SWP_NOACTIVATE);
    SetWindowPos(GetDlgItem(hWnd, ID_VIEW_SELECTIONS_ANY_BTN), NULL, 5 + 40 + 5, y, 40, 22, SWP_NOZORDER | SWP_NOACTIVATE);
    SetWindowPos(GetDlgItem(hWnd, ID_DIAMONDS_ROW_TIMECLOCK), NULL, rc.right - 155, y, 150, 20, SWP_NOZORDER | SWP_NOACTIVATE);
}

static void Diamonds_UpdateAnyButton(HWND hWnd) {
    HWND hAny = GetDlgItem(hWnd, ID_VIEW_SELECTIONS_ANY_BTN);
    if (diamondsState.viewSelectionEnabled == ID_VIEW_SELECTIONS_ANY_BTN) {
        SetWindowTextA(hAny, "Stop");
    } else {
        std::string rowCount = std::to_string(diamondsState.displayOrder.size());
        SetWindowTextA(hAny, rowCount.c_str());
    }
}

static void Diamonds_ShowCheckboxes(HWND hWnd, bool show) {
    if (diamondsState.checkboxesVisible == show) return;
    diamondsState.checkboxesVisible = show;
    int sw = show ? SW_SHOW : SW_HIDE;
    for (int i = 0; i < DIAMONDS_TAB_COUNT; ++i)
        ShowWindow(GetDlgItem(hWnd, ID_DIAMONDS_CHK_0 + i), sw);
    ShowWindow(GetDlgItem(hWnd, ID_VIEW_SELECTIONS_TOP_BTN), sw);
    ShowWindow(GetDlgItem(hWnd, ID_VIEW_SELECTIONS_ANY_BTN), sw);
    ShowWindow(GetDlgItem(hWnd, ID_DIAMONDS_ROW_TIMECLOCK), sw);
    Diamonds_Layout(hWnd);
}

static void DrawTwoLineCell(HDC hdc, const RECT& cellRect,
                            const char* topText, COLORREF topColor,
                            const char* bottomText, COLORREF bottomColor,
                            HBRUSH backgroundBrush) {
    FillRect(hdc, &cellRect, backgroundBrush);

    RECT topRect = cellRect;
    RECT bottomRect = cellRect;
    InflateRect(&topRect, -6, 0);
    InflateRect(&bottomRect, -6, 0);

    int midY = cellRect.top + (cellRect.bottom - cellRect.top) / 2;
    topRect.bottom = midY + 1;
    bottomRect.top = midY - 1;

    SetBkMode(hdc, TRANSPARENT);
    SelectObject(hdc, hFont11ptbold.get());

    SetTextColor(hdc, topColor);
    DrawTextA(hdc, topText, -1, &topRect, DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    SetTextColor(hdc, bottomColor);
    DrawTextA(hdc, bottomText, -1, &bottomRect, DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
}

// ── Virtual Sort ──────────────────────────────────────────────────────────────

static void Diamonds_ApplySort(HWND hList) {
    if (diamondsState.displayOrder.empty()) return;

    // Resolve pointers once to avoid O(n log n) hash lookups in the comparator.
    // Pointers stay valid because nothing inserts into dataCache during the sort.
    std::vector<std::pair<const DiamondRowCache*, int>> tmp;
    tmp.reserve(diamondsState.displayOrder.size());
    for (int id : diamondsState.displayOrder) {
        tmp.push_back({ &diamondsState.dataCache.at(id), id });
    }

    std::sort(tmp.begin(), tmp.end(), [](const auto& x, const auto& y) {
        const auto& a = *x.first;
        const auto& b = *y.first;
        if (diamondsState.sortCol == DCOL_SYMBOL || diamondsState.sortCol == DCOL_EXCHANGE) {
            int cmp = _stricmp(a.textCols[diamondsState.sortCol].c_str(), b.textCols[diamondsState.sortCol].c_str());
            return diamondsState.sortAsc ? (cmp > 0) : (cmp < 0);
        } else {
            double v1 = a.sortValues[diamondsState.sortCol];
            double v2 = b.sortValues[diamondsState.sortCol];
            if (v1 == v2) return false;
            if (diamondsState.sortCol == DCOL_DIV_DATE) {
                return diamondsState.sortAsc ? (v1 > v2) : (v1 < v2);
            } else {
                return diamondsState.sortAsc ? (v1 < v2) : (v1 > v2);
            }
        }
    });

    for (size_t i = 0; i < tmp.size(); ++i) {
        diamondsState.displayOrder[i] = tmp[i].second;
    }

    // ZERO-FLICKER FIX: Delegate to the paint timer instead of invalidating instantly
    diamondsState.dirty = true;
}
// ── Helpers ───────────────────────────────────────────────────────────────────

// Sentinel string displayed whenever a value cannot be computed (e.g. market closed, last == 0).
static const char* DIAMONDS_NO_DATA = "--";

static const double BOTTOM_SORT_VALUE = -999999.0;

static void Diamonds_UpdatePnLCols(HWND hWnd, int conId) {
    // Grab our new unified cache row
    auto& row = diamondsState.dataCache[conId];
    row.conId = conId; 
    
    TradingAPI::PnlSinglePayload pnlSingle;
    double avgCost = 0.0;
    std::string exchange;
    {
        std::lock_guard<std::mutex> lk(api().getPortfolioMutex());
        auto& pm = api().getPortfolioMap();
        auto it = pm.find(conId);
        if (it != pm.end()) {
            pnlSingle = it->second.pnlSingle;
            avgCost = it->second.avgCost;
            exchange = it->second.exchange;
        }
    }

    if (pnlSingle.conId > 0) {
        row.sortValues[DCOL_DAILYPNL] = pnlSingle.dailyPnL;
        row.textCols[DCOL_DAILYPNL] = std::format("{:+.2f}", pnlSingle.dailyPnL);

        row.sortValues[DCOL_UNREALIZED_PL] = pnlSingle.unrealizedPnL;
        row.textCols[DCOL_UNREALIZED_PL] = std::format("{:+.2f}", pnlSingle.unrealizedPnL);

        // Recompute the % column, derived from unrealizedPnL / cost basis,
        // NOT from last price, so it stays valid even when last == 0
        // (market closed / no quote yet) and matches IBKR's own PnL figure
        // rather than a reconstruction from price.
        double shares = row.sortValues[DCOL_POSITION];
        double costBasis = avgCost * std::fabs(shares);
        double pct = (costBasis > 0.0) ? pnlSingle.unrealizedPnL / costBasis * 100.0 : 0;
        row.unrealizedPnLPctStr = std::format("{:.2f}%", pct);

        
        row.textCols[DCOL_EXCHANGE] = exchange;

        diamondsState.dirtyRedraw.insert(conId);
    }
}

// Dividend data changes rarely and is now fetched once per position via a
// low-frequency one-shot request instead of the always-open L1 subscription
// (see queueDividendFetch/HandleDividendTick in ibkr.cpp). That means it can
// be genuinely unavailable for a while, most visibly over weekends, when
// there's no live market data connection for the one-shot fetch to ever
// complete. Falls back to the last value cached in the registry whenever the
// live fields are still at their empty/zero defaults. Any later live update
// simply overwrites these through the normal Diamonds_UpdateMarketCols() path.
static void Diamonds_ApplyCachedDividends(DiamondRowCache& cacheRow, int conId, const TradingAPI::L1Book& tickInfo) {
    bool haveLiveDividendData = (tickInfo.annualDividends != 0.0) || (tickInfo.dividendAmount != 0.0) || !tickInfo.dividendDate.empty();
    if (haveLiveDividendData) return;

    auto& dc = diamondsState.dividendCache[conId];
    if (!dc.loaded) {
        // Only touch the registry on a miss, and at most once per 30s per symbol.
        ULONGLONG now = GetTickCount64();
        if (dc.lastAttemptMs != 0 && now - dc.lastAttemptMs < 30000) return;
        dc.lastAttemptMs = now;
        if (!Settings_Dividends_Load(cacheRow.symbol, conId, dc.annual, dc.amount, dc.date, dc.dateSortable))
            return;
        dc.loaded = true;
    }

    cacheRow.sortValues[DCOL_DIV_AMT] = dc.amount;
    cacheRow.textCols[DCOL_DIV_AMT]   = std::format("{:.3f}", dc.amount);

    cacheRow.sortValues[DCOL_ANNUAL_DIV] = dc.annual;
    cacheRow.textCols[DCOL_ANNUAL_DIV]   = std::format("{:.3f}", dc.annual);

    cacheRow.sortValues[DCOL_DIV_DATE] = dc.dateSortable;
    cacheRow.textCols[DCOL_DIV_DATE]   = dc.date;

    double priceForYield = tickInfo.last > 0.0 ? tickInfo.last : tickInfo.prevClose;
    if (priceForYield > 0.0 && dc.annual > 0.0) {
        double pct = dc.annual / priceForYield * 100.0;
        cacheRow.sortValues[DCOL_DIV_YIELD] = pct;
        cacheRow.textCols[DCOL_DIV_YIELD]   = std::format("{:.2f}%", pct);
    } else if (dc.annual == 0.0) {
        cacheRow.sortValues[DCOL_DIV_YIELD] = 0.0;
        cacheRow.textCols[DCOL_DIV_YIELD]   = "0.00%";
    } else {
        cacheRow.sortValues[DCOL_DIV_YIELD] = BOTTOM_SORT_VALUE - conId;
        cacheRow.textCols[DCOL_DIV_YIELD]   = DIAMONDS_NO_DATA;
    }
}

static void Diamonds_UpdateMarketCols(int conId, const TradingAPI::L1Book& t) {
    // Ensure a cache row exists even if this tick arrived before
    // Diamonds_Repopulate had a chance to create one for it, e.g. right after
    // the window is closed and reopened, a WM_MARKET_L1 posted just before the
    // repopulate finishes used to hit this function while the cache was still
    // empty/rebuilding and the early-return below silently dropped the tick
    // for good (nothing ever re-requested it). That could look exactly like
    // "some columns stop updating" after a close/reopen cycle. Auto-creating
    // the row (mirroring what Diamonds_UpdatePnLCols already does) means the
    // tick is never lost; if the row isn't in diamondsState.displayOrder yet it
    // simply becomes visible on the next repopulate/sort instead of vanishing.
    auto& row = diamondsState.dataCache[conId];
    row.conId = conId;
    // Helper to write both sortable raw data and display string
    auto setCol = [&](int col, double val, int decimals, bool alwaysShow = false, bool alwaysSign = false) {
        row.sortValues[col] = val;
        if (val != 0.0 || alwaysShow) {
            row.textCols[col] = FormatFixed(val, decimals, alwaysSign);
        } else {
            row.textCols[col] = "";
        }
    };

    auto setNA = [&](int col, std::string placeHolder = DIAMONDS_NO_DATA) {
        row.sortValues[col] = BOTTOM_SORT_VALUE - row.conId; // Pushes NA to bottom on sorts
        row.textCols[col] = placeHolder;
    };

    row.bidSize = t.bidSize;
    row.askSize = t.askSize;
    row.bidSizeStr = FormatFixed(t.bidSize, 0);
    row.askSizeStr = FormatFixed(t.askSize, 0);
    row.sortValues[DCOL_SIZE] = t.askSize + t.bidSize;
    row.textCols[DCOL_SIZE]   = row.askSizeStr + "\n" + row.bidSizeStr;

    row.bid = t.bid;
    row.ask = t.ask;
    row.bidStr = FormatFixed(t.bid, 2);
    row.askStr = FormatFixed(t.ask, 2);
    row.sortValues[DCOL_BIDASK] = (t.ask + t.bid) / 2;
    row.textCols[DCOL_BIDASK]   = row.askStr + "\n" + row.bidStr;


    setCol(DCOL_DIV_AMT, t.dividendAmount,  3, true);
    setCol(DCOL_ANNUAL_DIV, t.annualDividends, 3, true);
    
    row.textCols[DCOL_DIV_DATE] = t.dividendDate;
    row.sortValues[DCOL_DIV_DATE] = t.dividendDateSortable;

    if (t.last > 0.0 && t.annualDividends > 0.0) {
        double pct = t.dividendYield();
        row.sortValues[DCOL_DIV_YIELD] = pct;
        row.textCols[DCOL_DIV_YIELD]   = std::format("{:.2f}%", pct);
    }
    else if (t.annualDividends == 0.0) setCol(DCOL_DIV_YIELD, 0.0, 2, true);
    else setNA(DCOL_DIV_YIELD);
    Diamonds_ApplyCachedDividends(row, conId, t);

    auto setWeekChangePct = [&](int col, double closeAgo) {
        if (closeAgo > 0.0 && t.last > 0.0) {
            double pct = (t.last - closeAgo) / closeAgo * 100.0;
            row.sortValues[col] = pct;
            row.textCols[col]   = FormatFixed(pct, 2, true) + "%";
        } else {
            setNA(col);
        }
    };

    DiamondsWeeklyCloseCache weeklyCloses = Diamonds_GetWeeklyCloseCache(conId);
    setWeekChangePct(DCOL_CHG13WEEK, weeklyCloses.closeAgo13Week);
    setWeekChangePct(DCOL_CHG26WEEK, weeklyCloses.closeAgo26Week);
    setWeekChangePct(DCOL_CHG52WEEK, weeklyCloses.closeAgo52Week);

    // Day high/low, used by the Last column's custom-draw color (see WM_NOTIFY/NM_CUSTOMDRAW).
    row.dayHigh = t.high;
    row.dayLow = t.low;
    row.prevClose = t.prevClose;
    row.halted = t.halted;

    double shares = row.sortValues[DCOL_POSITION];

    double mktVal = shares * (t.last > 0 ? t.last : t.prevClose);
    row.pctNetLiq = FormatFixed((NetLiquidation > 0.0 && mktVal != 0.0) ? (mktVal / NetLiquidation * 100.0) : 0.0, 2) + "%";
    setCol(DCOL_MKTVAL, mktVal, 2, true);

    if (t.rtVolRatio5min > 0.0 || t.rtVolSum5min > 0.0) {
        row.volRatio = t.rtVolRatio5min;
        row.sortValues[DCOL_VOLRATE] = t.rtVolSum5min;
        row.textCols[DCOL_VOLRATE]   = formatVolume((long long)t.rtVolSum5min);
    } else {
        row.volRatio = 0.0;
        setNA(DCOL_VOLRATE, "0");
    }

    if (t.last <= 0.0) {
        setNA(DCOL_LAST); setNA(DCOL_CHGPCT);
        setNA(DCOL_CHG5MIN);
        setNA(DCOL_VWAP);
        return;
    }

    setCol(DCOL_LAST, t.last, 2, true);

    // Alert Up Trigger (Alert High is equal to or lower than Last)
    if (row.upAlert > 0.0 && t.last >= row.upAlert) {
        if (diamondsState.firedAlertsUp.find(conId) == diamondsState.firedAlertsUp.end()) {
            diamondsState.firedAlertsUp.insert(conId); // Mark as fired
            //std::string msg = std::format("Last: {:.2f}\n\nAlert: {:.2f}", t.last, alertHigh);
            std::string msg = FormatFixed(t.last, 2);
            std::string title = row.symbol + ": Alert UP!";
            HWND hMain = FindWindowA(DASHBOARD_CLASS_NAME, NULL);
            if (hMain) {
                AlertPopupData* data = new AlertPopupData{title, msg, row.symbol, shares, t.last, conId, true};
                PostMessage(hMain, WM_SHOW_ALERT, 0, (LPARAM)data);
            }
        }
    }

    // Alert Down Trigger (Alert Low is equal to or higher than Last)
    if (row.downAlert > 0.0 && t.last <= row.downAlert) {
        if (diamondsState.firedAlertsDown.find(conId) == diamondsState.firedAlertsDown.end()) {
            diamondsState.firedAlertsDown.insert(conId); // Mark as fired
            //std::string msg = std::format("Alert: {:.2f}\n\nLast: {:.2f}", alertLow, t.last);
            std::string msg = FormatFixed(t.last, 2);
            std::string title = row.symbol + ": Alert DOWN!";
            HWND hMain = FindWindowA(DASHBOARD_CLASS_NAME, NULL);
            if (hMain) {
                AlertPopupData* data = new AlertPopupData{title, msg, row.symbol, shares, t.last, conId, false};
                PostMessage(hMain, WM_SHOW_ALERT, 0, (LPARAM)data);
            }
        }
    }

    // ── VWAP: display the VWAP price, but sort by (Last - VWAP) so the
    // column ranks by how far price has drifted from VWAP, not by VWAP itself. ──
    double vwapDiff = (t.vwap > 0.0 && t.last > 0.0) ? t.last - t.vwap : 0.0;
    setCol(DCOL_VWAP, vwapDiff, 2, true);

    // 5-minute price change, in dollars, Last vs. the price ~5 minutes ago,
    // sampled from the same long-lived history the sparkline's reference dots
    // use. Shows "--" until at least 5 minutes of history has accumulated
    // for this symbol (same "appears once ready" behavior as those dots).
    {
        double price5MinAgo = 0.0;
        auto& spark = diamondsState.sparklines[conId];
        if (spark.GetPriceMinutesAgo(5, price5MinAgo) && price5MinAgo > 0.0) {
            double priceDiff5min = t.last - price5MinAgo;
            //setCol(DCOL_CHG5MIN, priceDiff5min, 2, true);
            row.textCols[DCOL_CHG5MIN] = FormatFixed(priceDiff5min, 2, false);
            row.sortValues[DCOL_CHG5MIN] = row.textCols[DCOL_CHG5MIN] == "0.00" ? BOTTOM_SORT_VALUE - row.conId : priceDiff5min;
        } else {
            setNA(DCOL_CHG5MIN);
        }
    }

    if (t.prevClose > 0.0) {
        setCol(DCOL_CHGPCT, t.changePct(), 2, true, true);
        row.textCols[DCOL_CHGPCT] += "%";
    } else setNA(DCOL_CHGPCT);
}

static void Diamonds_UpdateAlertCols(int conId) {
    auto& row = diamondsState.dataCache[conId];
    row.conId = conId;

    row.upStr.clear();
    row.downStr.clear();
    auto alertIt = diamondsState.alertCache.find(conId);
    if (alertIt != diamondsState.alertCache.end()) {
        row.upStr = alertIt->second.upStr;
        row.downStr = alertIt->second.downStr;
    }
    row.upAlert = std::atof(row.upStr.c_str());
    row.downAlert = std::atof(row.downStr.c_str());

    row.textCols[DCOL_ALERT] = row.upStr + "\n" + row.downStr;
    row.sortValues[DCOL_ALERT]   = (row.upAlert + row.downAlert) / 2.0;
}

// ── Repopulate ────────────────────────────────────────────────────────────────
static void Diamonds_Repopulate(HWND hWnd) {
    HWND hList = GetDlgItem(hWnd, ID_DIAMONDS_RESULTS_LIST);
    if (!hList) return;

    diamondsState.displayOrder.clear(); // Clear the virtual list viewport

    std::vector<TradingAPI::PositionInfo> rows;
    std::unordered_set<int> portfolioConIds;
    {
        std::lock_guard<std::mutex> lock(api().getPortfolioMutex());
        for (auto const& [conId, info] : api().getPortfolioMap()) {
            if (info.isWatchOnly) continue; // watch-only rows are added explicitly below, Quarantine-only
            auto it = diamondsState.tabMap.find(info.conId);
            int  assignedTab = (it != diamondsState.tabMap.end()) ? it->second : DTAB_ALL;
            if ((diamondsState.checkedTabs >> assignedTab) & 1) rows.push_back(info);

            portfolioConIds.insert(info.conId);
        }
    }

    // ── Alert-only symbols: have an Alert Up/Down set but aren't a current
    // held position. Shown only under the Quarantine tab (forced there regardless
    // of diamondsState.tabMap, since there's no held position to assign a group
    // to), see the NM_RCLICK handler below for the disabled "Move to *".
    if ((diamondsState.checkedTabs >> DTAB_QUARENTINE) & 1) {
        for (auto const& cacheEntry : diamondsState.alertCache) {
            const auto& alert = cacheEntry.second;
            if (portfolioConIds.count(alert.conId)) continue; // already a real held position

            api().watchSymbol(alert.conId, alert.symbol);

            TradingAPI::PositionInfo pseudo;
            pseudo.conId  = alert.conId;
            pseudo.symbol = alert.symbol;
            pseudo.shares = 0.0;
            pseudo.avgCost = 0.0;
            rows.push_back(pseudo);
        }
    }

    // Build/refresh the cache rows.
    // Rule: only write the fields we own here (identity, position, avgCost, market
    // data). Never touch DCOL_DAILYPNL / DCOL_UNREALIZED_PL
    //, those are owned by WM_PNL_SINGLE and must survive a repopulate so they
    // remain visible when the window is closed and reopened.
    for (const auto& pos : rows) {
        bool isHeldPosition = (portfolioConIds.count(pos.conId) > 0);

        // operator[] creates a default row only when the conId is new.
        // For existing rows it returns the current entry, PnL fields are preserved.
        auto& cacheRow = diamondsState.dataCache[pos.conId];
        cacheRow.conId  = pos.conId;
        cacheRow.symbol = pos.symbol;

        cacheRow.textCols[DCOL_SYMBOL] = pos.symbol;

        cacheRow.sortValues[DCOL_POSITION] = pos.shares;
        cacheRow.textCols[DCOL_POSITION] = isHeldPosition ? std::format("{:.4g}", pos.shares) : "--";

        cacheRow.sortValues[DCOL_AVGPRICE] = pos.avgCost;
        cacheRow.textCols[DCOL_AVGPRICE] = isHeldPosition ? std::format("{:.2f}", pos.avgCost) : "--";

        Diamonds_UpdateAlertCols(pos.conId);

        // Pre-fill market data if already cached, this also seeds the estimated
        // PnL columns for the first open (before WM_PNL_SINGLE arrives).
        // This runs for both held positions AND watch-only (quarantine) symbols.
        TradingAPI::L1Book tickInfo;
        if (api().getMarketData(pos.conId, tickInfo)) {
            Diamonds_UpdateMarketCols(pos.conId, tickInfo);
        }

        if (isHeldPosition) {
            Diamonds_UpdatePnLCols(hWnd, pos.conId);
        }

        diamondsState.displayOrder.push_back(pos.conId);
    }

    // VIRTUAL LIST MAGIC: Tell the UI exactly how many items exist. It will ask for text later.
    ListView_SetItemCountEx(hList, diamondsState.displayOrder.size(), LVSICF_NOINVALIDATEALL | LVSICF_NOSCROLL);
    InvalidateRect(hList, NULL, FALSE);
    Diamonds_ApplySort(hList);

    Diamonds_UpdateAnyButton(hWnd);
}

// Drains per-row dirty sets. Called from TIMER_DIAMONDS_PAINT only.
static void Diamonds_FlushDirty(HWND hWnd) {
    if (diamondsState.dirtyMarket.empty() && diamondsState.dirtyRedraw.empty() && !diamondsState.dirty)
        return;

    HWND hList = GetDlgItem(hWnd, ID_DIAMONDS_RESULTS_LIST);
    if (!hList) return;

    // 1) Rebuild cache rows for every symbol that ticked (visible or not):
    //    alert triggers and the sparkline history depend on this running for all of them.
    if (!diamondsState.dirtyMarket.empty()) {
        std::unordered_set<int> pending;
        pending.swap(diamondsState.dirtyMarket);
        for (int conId : pending) {
            TradingAPI::L1Book fresh;
            if (!api().getMarketData(conId, fresh)) continue;
            Diamonds_UpdateMarketCols(conId, fresh);
            if (fresh.last > 0.0)
                diamondsState.sparklines[conId].AddPrice(fresh.last);
            diamondsState.dirtyRedraw.insert(conId);
        }
    }

    const int total = (int)diamondsState.displayOrder.size();
    if (total == 0) {
        diamondsState.dirty = false;
        diamondsState.dirtyRedraw.clear();
        return;
    }

    int top    = ListView_GetTopIndex(hList);
    int bottom = std::min(top + ListView_GetCountPerPage(hList), total - 1);
    if (top < 0) top = 0;

    // 2) Full visible redraw (sort order changed).
    if (diamondsState.dirty) {
        ListView_RedrawItems(hList, top, bottom);
        diamondsState.dirty = false;
        diamondsState.dirtyRedraw.clear();
        return;
    }

    // 3) Per-row redraw: only visible rows whose conId is dirty, coalescing adjacent rows.
    if (diamondsState.dirtyRedraw.empty()) return;
    int runStart = -1;
    for (int i = top; i <= bottom + 1; ++i) {
        bool hit = (i <= bottom) && diamondsState.dirtyRedraw.count(diamondsState.displayOrder[i]) != 0;
        if (hit) {
            if (runStart < 0) runStart = i;
        } else if (runStart >= 0) {
            ListView_RedrawItems(hList, runStart, i - 1);
            runStart = -1;
        }
    }
    diamondsState.dirtyRedraw.clear();
    // No UpdateWindow(): let the repaint go through the normal message loop.
}

// ── Window procedure ──────────────────────────────────────────────────────────

LRESULT CALLBACK WndProcDiamonds(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE: {
        HINSTANCE hInst = ((LPCREATESTRUCT)lParam)->hInstance;

        HWND hList = CreateWindowExA(
            WS_EX_CLIENTEDGE, "SysListView32", "",
            WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_OWNERDATA,
            0, 0, 1100, 420, hWnd, (HMENU)ID_DIAMONDS_RESULTS_LIST, hInst, NULL);

        // No paint timer needed, rows are selectively invalidated on data arrival.
        // Start the paint-limiter timer so Diamonds_ApplySort's dirty flag is flushed.
        SetTimer(hWnd, TIMER_DIAMONDS_PAINT, DIAMONDS_PAINT_TIMER_MS, NULL);

        Diamonds_SetRowHeight(hList, 28);
        SendMessage(hList, WM_SETFONT, (WPARAM)hFont16pt.get(), TRUE);
        SendMessage(ListView_GetHeader(hList), WM_SETFONT, (WPARAM)hFont11pt.get(), TRUE);
        SetWindowSubclass(hList, ListViewNoFlickerProc, 0, 0);
        SetWindowSubclass(hList, ListViewForwardKey_SubclassProc, 1, 0);

        ListView_SetExtendedListViewStyle(hList, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

        LVCOLUMNA lvc = {};
        lvc.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_FMT;
        for (int i = 0; i < DCOL_COUNT; ++i) {
            lvc.cx      = diamondCols[i].width;
            lvc.pszText = (LPSTR)diamondCols[i].header;
            lvc.fmt     = diamondCols[i].fmt;
            ListView_InsertColumn(hList, i, &lvc);
        }

        HWND hViewTop = CreateWindowA("BUTTON", "Top",
            WS_CHILD | BS_PUSHBUTTON | BS_OWNERDRAW,
            0, 0, 40, 22, hWnd, (HMENU)ID_VIEW_SELECTIONS_TOP_BTN, hInst, NULL);
        SendMessage(hViewTop, WM_SETFONT, (WPARAM)hFont11pt.get(), TRUE);

        HWND hViewAny = CreateWindowA("BUTTON", "Any",
            WS_CHILD | BS_PUSHBUTTON | BS_OWNERDRAW,
            0, 0, 40, 22, hWnd, (HMENU)ID_VIEW_SELECTIONS_ANY_BTN, hInst, NULL);
        SendMessage(hViewAny, WM_SETFONT, (WPARAM)hFont11pt.get(), TRUE);

        HWND hTimeClock = CreateWindowExA(WS_EX_TRANSPARENT, "STATIC", "",
            WS_CHILD | SS_RIGHT | SS_NOPREFIX,
            0, 0, 150, 20, hWnd, (HMENU)ID_DIAMONDS_ROW_TIMECLOCK, hInst, NULL);
        SendMessage(hTimeClock, WM_SETFONT, (WPARAM)hFont11pt.get(), TRUE);

        // Create the three filter checkboxes (hidden until window is focused).
        for (int i = 0; i < DIAMONDS_TAB_COUNT; ++i) {
            HWND hChk = CreateWindowA("BUTTON", diamondTabNames[i],
                WS_CHILD | BS_AUTOCHECKBOX | BS_NOTIFY,
                0, 0, 10, 10,
                hWnd, (HMENU)(UINT_PTR)(ID_DIAMONDS_CHK_0 + i), hInst, NULL);
            // Default: all checked.
            SendMessage(hChk, BM_SETCHECK, BST_CHECKED, 0);
        }


        // Load saved tab assignments, checkbox state, sort settings, and symbol colors.
        Diamonds_LoadTabMap();
        Diamonds_LoadSymbolColors();
        Diamonds_RefreshAlertCache();
        diamondsState.sortCol = (int)Settings_Sort_Load(DIAMONDS_CLASS_NAME, "SortCol", DCOL_SYMBOL);
        diamondsState.sortAsc = Settings_Sort_Load(DIAMONDS_CLASS_NAME, "SortAsc", 1) != 0;
        if (diamondsState.sortCol < 0 || diamondsState.sortCol >= DCOL_COUNT) diamondsState.sortCol = DCOL_SYMBOL;

        // Restore checkbox bitmask (default 0x7 = all checked).
        diamondsState.checkedTabs = (UINT)Settings_CheckedTabs_Load(0x7);
        diamondsState.checkedTabs &= 0x7;  // clamp to valid 3-bit range
        for (int i = 0; i < DIAMONDS_TAB_COUNT; ++i) {
            bool checked = (diamondsState.checkedTabs >> i) & 1;
            HWND tab = GetDlgItem(hWnd, ID_DIAMONDS_CHK_0 + i);
            SendMessage(tab, BM_SETCHECK, checked ? BST_CHECKED : BST_UNCHECKED, 0);
        }
        
        api().addApiUpdateWindow(hWnd);

        Diamonds_UpdateDivColumnsVisibility(hWnd);
        Diamonds_Repopulate(hWnd);
        Diamonds_UpdateFrameMargins(hWnd);
        SetWindowPos(hWnd, NULL, 0, 0, 0, 0,
            SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        Diamonds_UpdateEventTitle(hWnd);

        SetTimer(hWnd, TIMER_DIAMONDS_SORT, DIAMONDS_SORT_TIMER_MS, NULL);
        break;
    }


    // Make the client area cover the caption (keep the side/bottom borders as-is).
    case WM_NCCALCSIZE:
        if (wParam) {
            auto* p = (NCCALCSIZE_PARAMS*)lParam;
            LONG origTop = p->rgrc[0].top;
            DefWindowProc(hWnd, message, wParam, lParam);
            p->rgrc[0].top = origTop + (IsZoomed(hWnd) ? Diamonds_FrameY(hWnd) : 0);
            return 0;
        }
        break;

    case WM_NCHITTEST: {
        LRESULT dwm = 0;
        if (DwmDefWindowProc(hWnd, message, wParam, lParam, &dwm)) return dwm;
        POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        RECT wr;
        GetWindowRect(hWnd, &wr);
        int dy = pt.y - wr.top;
        if (!IsZoomed(hWnd) && dy >= 0 && dy < Diamonds_FrameY(hWnd) / 2) return HTTOP;
        POINT cpt = pt;
        ScreenToClient(hWnd, &cpt);
        if (Diamonds_TitleHitTest(cpt)) return HTCLIENT;
        if (cpt.y >= 0 && cpt.y < Diamonds_CaptionH(hWnd)) return HTCAPTION;
        break;
    }

    case WM_NCMOUSEMOVE:
    case WM_NCLBUTTONDOWN:
    case WM_NCLBUTTONUP:
    case WM_NCMOUSELEAVE: {
        LRESULT dwm = 0;
        if (DwmDefWindowProc(hWnd, message, wParam, lParam, &dwm)) return dwm;
        break;
    }

    case WM_ERASEBKGND: {
        HDC hdc = (HDC)wParam;
        RECT rc;
        GetClientRect(hWnd, &rc);
        RECT cap = rc;
        cap.bottom = Diamonds_CaptionH(hWnd);
        FillRect(hdc, &cap, (HBRUSH)GetStockObject(BLACK_BRUSH));
        RECT rest = rc;
        rest.top = cap.bottom;
        FillRect(hdc, &rest, darkMode ? hDarkBrush : (HBRUSH)(COLOR_BTNFACE + 1));
        return 1;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        Diamonds_PaintCaption(hWnd, hdc);
        EndPaint(hWnd, &ps);
        return 0;
    }

    case WM_GETMINMAXINFO: {
        bool isMaximized = false;

        WINDOWPLACEMENT wp;
        wp.length = sizeof(WINDOWPLACEMENT);
        if (GetWindowPlacement(hWnd, &wp)) {
            isMaximized = wp.showCmd == SW_SHOWMAXIMIZED;
        }

        bool showDiv   = false;
        bool showWeeks = false;
        if (isMaximized) {
            showDiv   = (diamondsState.checkedTabs & (1u << 1)) != 0;   // Dividends
            showWeeks = (diamondsState.checkedTabs & (1u << 2)) != 0;   // Quarantine
        }

        int extraWidth = 0;
        if (showDiv) {
            extraWidth += diamondCols[DCOL_DIV_YIELD].width   + diamondCols[DCOL_DIV_DATE].width +
                        diamondCols[DCOL_DIV_AMT].width      + diamondCols[DCOL_ANNUAL_DIV].width;
        }
        if (showWeeks) {
            extraWidth += diamondCols[DCOL_CHG13WEEK].width + diamondCols[DCOL_CHG26WEEK].width +
                        diamondCols[DCOL_CHG52WEEK].width + 
                        diamondCols[DCOL_AVGPRICE].width + diamondCols[DCOL_EXCHANGE].width;
        }
        if (extraWidth > 0) extraWidth += 10; // margin, same buffer the original single-group case used
        MINMAXINFO* mmi = (MINMAXINFO*)lParam;
        mmi->ptMinTrackSize.x = windowDiamondsWidth + extraWidth;
        mmi->ptMaxTrackSize.x = windowDiamondsWidth + extraWidth;// 2. Retrieve the desktop work area (excludes the taskbar)
        RECT workArea;
        SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);

        // 3. Force the maximized size and position to respect the work area
        mmi->ptMaxSize.x = workArea.right - workArea.left;
        mmi->ptMaxSize.y = workArea.bottom - workArea.top;
        mmi->ptMaxPosition.x = workArea.left - workArea.left + ((mmi->ptMaxSize.x - mmi->ptMaxTrackSize.x)/2); // relative to monitor, or workArea.left
        mmi->ptMaxPosition.y = workArea.top - workArea.top;
        return 0;
    }

    case WM_SIZE: {
        Diamonds_UpdateFrameMargins(hWnd);
        Diamonds_UpdateDivColumnsVisibility(hWnd);
        Diamonds_Layout(hWnd);
        Diamonds_InvalidateCaption(hWnd);
        // DWM may still report old caption-button bounds during maximize/restore.
        SetTimer(hWnd, TIMER_DIAMONDS_CAPTION, DIAMONDS_CAPTION_TIMER_MS, NULL);
        break;
    }

    case WM_LBUTTONDOWN: {
        POINT pt = { (short)LOWORD(lParam), (short)HIWORD(lParam) };
        int conId = Diamonds_TitleHitTest(pt);
        if (conId > 0) {
            api().updateDisplayGroup(conId);
            return 0;
        }
        break;
    }

    case WM_LBUTTONDBLCLK: {
        POINT pt = { (short)LOWORD(lParam), (short)HIWORD(lParam) };
        int conId = Diamonds_TitleHitTest(pt);
        if (conId > 0 && !lockHotkeys) {
            for (const auto& ev : diamondsState.diamondsTitleEvents) {
                if (ev.conId == conId && !ev.symbol.empty()) {
                    StartMarket(ev.symbol, conId);
                    break;
                }
            }
            return 0;
        }
        break;
    }

    case WM_SETCURSOR: {
        if (LOWORD(lParam) == HTCLIENT) {
            POINT pt;
            GetCursorPos(&pt);
            ScreenToClient(hWnd, &pt);
            if (Diamonds_TitleHitTest(pt)) {
                SetCursor(LoadCursor(NULL, IDC_HAND));
                return TRUE;
            }
        }
        break;
    }

    // ── Checkboxes show when active, hide when inactive ───────────────────────
    case WM_ACTIVATE:
        Diamonds_ShowCheckboxes(hWnd, LOWORD(wParam) != WA_INACTIVE);
        break;

    case WM_CTLCOLORSTATIC: {
        HWND hCtrl = (HWND)lParam;
        int id = GetDlgCtrlID(hCtrl);
        if (id >= ID_DIAMONDS_CHK_0 && id <= ID_DIAMONDS_CHK_2) {
            HDC hdc = (HDC)wParam;
            bool checked = SendMessage(hCtrl, BM_GETCHECK, 0, 0) == BST_CHECKED;
            SetTextColor(hdc, checked ? (darkMode ? DM_TEXT : LM_TEXT) : COINS_CLR_GRAY);
            SetBkColor(hdc, darkMode ? DM_BG : GetSysColor(COLOR_BTNFACE));
            return (LRESULT)(darkMode ? hDarkBrush : hLightBrush);
        }
        if (id == ID_DIAMONDS_ROW_TIMECLOCK) {
            HDC hdc = (HDC)wParam;
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, darkMode ? DM_TEXT : LM_TEXT);
            SetBkColor(hdc, darkMode ? DM_BG : GetSysColor(COLOR_BTNFACE));
            return (LRESULT)(darkMode ? hDarkBrush : hLightBrush);
        }
        break;
    }

    case WM_KEYDOWN: {
        if (wParam == 'A') {
            SendMessage(hWnd, WM_COMMAND, ID_VIEW_SELECTIONS_ANY_BTN, 0);
            return 0;
        }
        if (wParam == 'T') {
            SendMessage(hWnd, WM_COMMAND, ID_VIEW_SELECTIONS_TOP_BTN, 0);
            return 0;
        }
        break;
    }

    case WM_COMMAND: {
        WORD id = LOWORD(wParam);
        if (id >= ID_DIAMONDS_CHK_0 && id <= ID_DIAMONDS_CHK_2 && HIWORD(wParam) == BN_CLICKED) {
            // Rebuild bitmask from checkbox states.
            diamondsState.checkedTabs = 0;
            for (int i = 0; i < DIAMONDS_TAB_COUNT; ++i) {
                HWND tab = GetDlgItem(hWnd, ID_DIAMONDS_CHK_0 + i);
                if (SendMessage(tab, BM_GETCHECK, 0, 0) == BST_CHECKED) {
                    diamondsState.checkedTabs |= (1u << i);
                }
                InvalidateRect(tab, NULL, TRUE);
            }
            Settings_CheckedTabs_Save((int)diamondsState.checkedTabs);
            Diamonds_UpdateDivColumnsVisibility(hWnd);
            Diamonds_Repopulate(hWnd);
            InvalidateRect(hWnd, NULL, TRUE);
        }
        if (id == ID_VIEW_SELECTIONS_ANY_BTN || id == ID_VIEW_SELECTIONS_TOP_BTN) {
            if (diamondsState.viewSelectionEnabled != id) {
                diamondsState.viewSelectionEnabled = id;
                if (id == ID_VIEW_SELECTIONS_TOP_BTN) {
                    SetWindowText(GetDlgItem(hWnd, ID_VIEW_SELECTIONS_TOP_BTN), "Stop");
                } else if (id == ID_VIEW_SELECTIONS_ANY_BTN) {
                    SetWindowText(GetDlgItem(hWnd, ID_VIEW_SELECTIONS_TOP_BTN), "Top");
                }
                Diamonds_UpdateAnyButton(hWnd);
                SetTimer(hWnd, TIMER_DIAMONDS_VIEW, DIAMONDS_VIEW_TIMER_MS, NULL);
                SendMessage(hWnd, WM_TIMER, TIMER_DIAMONDS_VIEW, 0);
            } else {
                diamondsState.viewSelectionEnabled = 0;
                SetWindowText(GetDlgItem(hWnd, ID_VIEW_SELECTIONS_TOP_BTN), "Top");
                Diamonds_UpdateAnyButton(hWnd);
                KillTimer(hWnd, TIMER_DIAMONDS_VIEW);
            }
        }
        break;
    }

    case WM_DIAMONDS_UPDATE: {
        Diamonds_Repopulate(hWnd);
        break;
    }

    case WM_ALERTS_CHANGED: {
        int conId = (int)lParam;
        if (!conId) break;
        
        diamondsState.firedAlertsUp.erase(conId);
        diamondsState.firedAlertsDown.erase(conId);

        Diamonds_RefreshAlertCache();
        Diamonds_Repopulate(hWnd);
        InvalidateRect(hWnd, NULL, TRUE);
        break;
    }

    // ── Live market data update for one symbol ────────────────────────────────
    // Posted by Impl::tickPrice / tickSize / tickString / tickGeneric
    case WM_MARKET_L1: {
        int conId = (int)lParam;
        if (!conId) break;
        diamondsState.dirtyMarket.insert(conId);
        break;
    }
    
    // ── Live per-position PnL update (reqPnLSingle stream) ───────────────────
    // Posted by Impl::pnlSingle() on the API thread via PostMessage.
    //   wParam = conId (fast row-lookup key, no pointer deref needed)
    //   lParam = heap-allocated TradingAPI::PnlSinglePayload*, we own it, must delete.
    case WM_PNL_SINGLE: {
        int conId = (int)lParam;
        if (!conId) break;
        Diamonds_UpdatePnLCols(hWnd, conId);
        // ZERO-FLICKER FIX: Stop auto-sorting the entire grid on every single PnL tick!
        break;
    }

    // ── Connection state changed ──────────────────────────────────────────────
    case WM_API_UPDATE: {
        HWND hList = GetDlgItem(hWnd, ID_DIAMONDS_RESULTS_LIST);
        if (!hList) break;
        if (api().isMarketDataConnected() && api().isTradingConnected()) {
            // Re-request positions (market data re-subscribed in positionEnd()).
            Diamonds_Repopulate(hWnd);
        } else {
            diamondsState.displayOrder.clear();
            diamondsState.dataCache.clear();
            diamondsState.dirtyMarket.clear();
            diamondsState.dirtyRedraw.clear();
            diamondsState.dirty = false;
            ListView_SetItemCountEx(hList, 0, LVSICF_NOINVALIDATEALL);
            InvalidateRect(hList, NULL, FALSE);
            Diamonds_UpdateAnyButton(hWnd);
        }
        RECT rc;
        GetClientRect(hWnd, &rc);
        rc.bottom = Diamonds_CaptionH(hWnd);
        InvalidateRect(hWnd, &rc, FALSE);
        break;
    }

    // ── Notification handling ─────────────────────────────────────────────────
    case WM_NOTIFY: {
        NMHDR* hdr = (NMHDR*)lParam;
        if (hdr->idFrom != ID_DIAMONDS_RESULTS_LIST) break;

        // Scroll finished (comctl32 v6): repaint so the exposed strip below the
        // last row is redrawn instead of keeping blitted stale pixels.
        if (hdr->code == LVN_ENDSCROLL) {
            InvalidateRect(hdr->hwndFrom, NULL, TRUE);
            return 0;
        }

        // ── Row selected: push symbol to TWS-linked windows/apps ─────────────
        if (hdr->code == LVN_ITEMCHANGED) {
            NMLISTVIEW* nmlv = (NMLISTVIEW*)lParam;
            if ((nmlv->uChanged & LVIF_STATE) && (nmlv->uNewState & LVIS_SELECTED) &&
                nmlv->iItem >= 0 && nmlv->iItem < (int)diamondsState.displayOrder.size()) {
                int conId = diamondsState.displayOrder[nmlv->iItem];
                api().updateDisplayGroup(conId);
                ListView_SetItemState(hdr->hwndFrom, nmlv->iItem, 0, LVIS_SELECTED);
            }
        }

        // --- VIRTUAL LIST TEXT REQUEST ---
        if (hdr->code == LVN_GETDISPINFO) {
            NMLVDISPINFO* pdi = (NMLVDISPINFO*)lParam;
            if (pdi->item.iItem < 0 || pdi->item.iItem >= (int)diamondsState.displayOrder.size()) return 0;
            
            int conId = diamondsState.displayOrder[pdi->item.iItem];
            const auto& row = diamondsState.dataCache[conId];

            if (pdi->item.mask & LVIF_TEXT) {
                // VIRTUAL LIST FIX: Direct pointer assignment is zero-copy and avoids buffer truncation
                pdi->item.pszText = (LPSTR)row.textCols[pdi->item.iSubItem].c_str();
            }
            return 0;
        }
        if (hdr->code == LVN_COLUMNCLICK) {
            NMLISTVIEW* nmlv = (NMLISTVIEW*)lParam;
            int col = nmlv->iSubItem;
            if (col == diamondsState.sortCol) diamondsState.sortAsc = !diamondsState.sortAsc;
            else { diamondsState.sortCol = col; diamondsState.sortAsc = false; }
            Settings_Sort_Save(DIAMONDS_CLASS_NAME, "SortCol", diamondsState.sortCol);
            Settings_Sort_Save(DIAMONDS_CLASS_NAME, "SortAsc", diamondsState.sortAsc ? 1 : 0);
            HWND hList = GetDlgItem(hWnd, ID_DIAMONDS_RESULTS_LIST);
            Diamonds_ApplySort(hList);
            InvalidateRect(hList, NULL, FALSE);
            return 0;
        }

        if (hdr->code == NM_DBLCLK) {
            if (!lockHotkeys) {
                LPNMITEMACTIVATE act = (LPNMITEMACTIVATE)lParam;
                int row = act->iItem;
                if (row >= 0) {
                    int conId = diamondsState.displayOrder[row];
                    const std::string& sym = diamondsState.dataCache[conId].textCols[DCOL_SYMBOL];
                    StartMarket(sym, conId);
                }
            }
        }

        if (hdr->code == NM_RCLICK) {
            if (!lockHotkeys) {
                LPNMITEMACTIVATE act = (LPNMITEMACTIVATE)lParam;
                int row = act->iItem;
                if (row >= 0) {
                    int conId = diamondsState.displayOrder[row];
                    const std::string& sym = diamondsState.dataCache[conId].textCols[DCOL_SYMBOL];

                    // Determine current group assignment for this item.
                    auto mapIt = diamondsState.tabMap.find(conId);
                    int currentGroup = (mapIt != diamondsState.tabMap.end()) ? mapIt->second : DTAB_ALL;

                    // Determine current color assignment for this item.
                    auto colorIt = diamondsState.symbolColors.find(conId);
                    int currentColor = (colorIt != diamondsState.symbolColors.end()) ? colorIt->second : DIAMONDS_COLOR_NONE;

                    // Determine if this is a currently held portfolio position
                    bool isHeldPosition = false;
                    {
                        std::lock_guard<std::mutex> lock(api().getPortfolioMutex());
                        auto& pm = api().getPortfolioMap();
                        auto pit = pm.find(conId);
                        isHeldPosition = (pit != pm.end()) && !pit->second.isWatchOnly;
                    }

                    // ── Build context menu ────────────────────────────────────────
                    // IDs 1-3:   group assignment
                    // IDs 200-206: color options (200+idx for colors, 206 = None)
                    HMENU hMenu = CreatePopupMenu();

                    // ── Quick placeholder orders ───────────────────────────────────
                    double quickLastPrice = 0.0;
                    {
                        TradingAPI::L1Book quickInfo;
                        if (conId > 0 && api().getMarketData(conId, quickInfo)) quickLastPrice = quickInfo.last;
                    }
                    std::string sellLabel = sym + (
                        (quickLastPrice > 0.0)
                            ? std::format(" SELL 1 @ {:.2f}", quickLastPrice)
                            : " SELL 1"
                    );
                    std::string buyLabel = sym + (
                        (quickLastPrice > 0.0)
                            ? std::format(" BUY 1 @ {:.2f}", quickLastPrice)
                            : " BUY 1"
                    );
                    AppendMenuA(hMenu, MF_STRING | ((conId == 0 || quickLastPrice <= 0.0) ? MF_GRAYED : 0), 301, sellLabel.c_str());
                    AppendMenuA(hMenu, MF_STRING | ((conId == 0 || quickLastPrice <= 0.0) ? MF_GRAYED : 0), 300, buyLabel.c_str());

                    AppendMenuA(hMenu, MF_SEPARATOR, 0, NULL);
                    AppendMenuA(hMenu, MF_STRING, 302, "Edit Alerts");
                    AppendMenuA(hMenu, MF_SEPARATOR, 0, NULL);

                    AppendMenuA(hMenu, MF_STRING | ((currentGroup == DTAB_ALL        || !isHeldPosition) ? MF_GRAYED : 0), 1, "Move to Growth");
                    AppendMenuA(hMenu, MF_STRING | ((currentGroup == DTAB_GROWTH     || !isHeldPosition) ? MF_GRAYED : 0), 2, "Move to Dividends");
                    AppendMenuA(hMenu, MF_STRING | ((currentGroup == DTAB_QUARENTINE || !isHeldPosition) ? MF_GRAYED : 0), 3, "Move to Quarantine");

                    // ── Color submenu ─────────────────────────────────────────────
                    AppendMenuA(hMenu, MF_SEPARATOR, 0, NULL);
                    
                    // "None" option, grayed when no color is currently assigned.
                    AppendMenuA(hMenu, MF_STRING | (currentColor == DIAMONDS_COLOR_NONE ? MF_GRAYED : 0),
                                200 + DIAMONDS_COLOR_COUNT, "Set Color: None");

                    for (int i = 0; i < DIAMONDS_COLOR_COUNT; ++i) {
                        bool isCurrent = (currentColor == i);
                        AppendMenuA(hMenu, MF_STRING | (isCurrent ? MF_GRAYED : 0),
                                    200 + i, diamondColorPalette[i].label);
                    }

                    POINT pt;
                    GetCursorPos(&pt);
                    int cmd = (int)TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_RIGHTBUTTON,
                                                pt.x, pt.y, 0, hWnd, NULL);
                    DestroyMenu(hMenu);

                    if (cmd >= 1 && cmd <= 3 && isHeldPosition) {
                        // Group assignment.
                        int targetTab = cmd - 1;
                        if (targetTab == DTAB_ALL)
                            diamondsState.tabMap.erase(conId);
                        else
                            diamondsState.tabMap[conId] = targetTab;
                        Diamonds_SaveTabMap();
                        Diamonds_Repopulate(hWnd);
                        InvalidateRect(hWnd, NULL, TRUE);
                        Diamonds_CleanupStaleTabAssignments();
                        Diamonds_CleanupStaleDividends();
                    } else if (cmd >= 200 && cmd <= 200 + DIAMONDS_COLOR_COUNT) {
                        // Color assignment.
                        int pickedIdx = cmd - 200;
                        if (pickedIdx == DIAMONDS_COLOR_COUNT) {
                            // "None", remove override.
                            diamondsState.symbolColors.erase(conId);
                        } else {
                            diamondsState.symbolColors[conId] = pickedIdx;
                        }
                        Diamonds_SaveSymbolColors();
                        // Invalidate just this row so the color appears immediately.
                        HWND hList = GetDlgItem(hWnd, ID_DIAMONDS_RESULTS_LIST);
                        ListView_RedrawItems(hList, row, row);
                        UpdateWindow(hList);
                        Diamonds_CleanupStaleTabAssignments();
                        Diamonds_CleanupStaleDividends();
                    } else if (cmd == 300 && conId > 0) {
                        // Quick BUY placeholder: 1 share @ $1.00.
                        TradingAPI::L1Book quickInfo;
                        if (api().getMarketData(conId, quickInfo) && quickInfo.last > 0.0) {
                            double buyPrice = quickInfo.last;
                            std::thread([conId, sym, buyPrice]() {
                                HWND hDashboard = FindWindowA(DASHBOARD_CLASS_NAME, NULL);
                                if (hDashboard && IsWindow(hDashboard)) {
                                    PostMessageA(hDashboard, WM_OPEN_ORDERS_WINDOW, 0, 0);
                                }
                                HWND hOrders = FindWindowA(ORDERS_CLASS_NAME, NULL);
                                int max = 10;
                                while (!hOrders || !IsWindow(hOrders)) {
                                    if (!max--) break;
                                    std::this_thread::sleep_for(std::chrono::milliseconds(121));
                                    hOrders = FindWindowA(ORDERS_CLASS_NAME, NULL);
                                }
                                if (hOrders && IsWindow(hOrders)) {
                                    api().submitOrder(conId, sym, "BUY", false, 1.0, buyPrice, 0.0, 0.0, 0.0, false);
                                }
                            }).detach();
                        }
                    } else if (cmd == 301 && conId > 0) {
                        // Quick SELL placeholder: 1 share @ 2x last price.
                        TradingAPI::L1Book quickInfo;
                        if (api().getMarketData(conId, quickInfo) && quickInfo.last > 0.0) {
                            double sellPrice = quickInfo.last;
                            std::thread([conId, sym, sellPrice]() {
                                HWND hDashboard = FindWindowA(DASHBOARD_CLASS_NAME, NULL);
                                if (hDashboard && IsWindow(hDashboard)) {
                                    PostMessageA(hDashboard, WM_OPEN_ORDERS_WINDOW, 0, 0);
                                }
                                HWND hOrders = FindWindowA(ORDERS_CLASS_NAME, NULL);
                                int max = 10;
                                while (!hOrders || !IsWindow(hOrders)) {
                                    if (!max--) break;
                                    std::this_thread::sleep_for(std::chrono::milliseconds(121));
                                    hOrders = FindWindowA(ORDERS_CLASS_NAME, NULL);
                                }
                                if (hOrders && IsWindow(hOrders)) {
                                    api().submitOrder(conId, sym, "SELL", false, 1.0, sellPrice, 0.0, 0.0, 0.0, false);
                                }
                            }).detach();
                        }
                    } else if (cmd == 302) {
                        TradingAPI::L1Book quickInfo;
                        if (api().getMarketData(conId, quickInfo) && quickInfo.last > 0.0) {
                            StartAlertEditor(sym, conId, quickInfo.last);
                        }
                    }
                }
            }
        }

        if (hdr->code == NM_CUSTOMDRAW) {
            NMLVCUSTOMDRAW* cd = (NMLVCUSTOMDRAW*)lParam;
            switch (cd->nmcd.dwDrawStage) {
                case CDDS_PREPAINT:
                    return CDRF_NOTIFYITEMDRAW;

                case CDDS_ITEMPREPAINT:
                    cd->nmcd.uItemState &= ~CDIS_SELECTED;
                    diamondsState.paintRowCache = nullptr;
                    if (cd->nmcd.dwItemSpec < diamondsState.displayOrder.size()) {
                        int conId = diamondsState.displayOrder[(size_t)cd->nmcd.dwItemSpec];
                        auto cacheIt = diamondsState.dataCache.find(conId);
                        if (cacheIt != diamondsState.dataCache.end())
                            diamondsState.paintRowCache = &cacheIt->second;
                    }
                    if (darkMode) {
                        cd->clrTextBk = (cd->nmcd.dwItemSpec % 2 == 0) ? DM_BG : DM_BG2;
                        cd->clrText   = DM_TEXT;
                    } else {
                        cd->clrTextBk = (cd->nmcd.dwItemSpec % 2 == 0) ? GetSysColor(COLOR_WINDOW) : RGB(245, 245, 245);
                        cd->clrText   = LM_TEXT;
                    }
                    return CDRF_NOTIFYSUBITEMDRAW;

                case CDDS_ITEMPREPAINT | CDDS_SUBITEM: {
                    if (!diamondsState.paintRowCache) return CDRF_DODEFAULT;
                    const DiamondRowCache& cacheRow = *diamondsState.paintRowCache;

                    // ── Symbol column: apply per-symbol color override ────────
                    if (cd->iSubItem == DCOL_SYMBOL) {
                        SelectObject(cd->nmcd.hdc, hFont16ptbold.get());
                        auto cit = diamondsState.symbolColors.find(cacheRow.conId);
                        if (cit != diamondsState.symbolColors.end() &&
                            cit->second >= 0 && cit->second < DIAMONDS_COLOR_COUNT) {
                            cd->clrText = diamondColorPalette[cit->second].rgb;
                            if (darkMode) cd->clrTextBk = (cd->nmcd.dwItemSpec % 2 == 0) ? DM_BG : DM_BG2;
                        }
                        return CDRF_NEWFONT;
                    }
                    if (cd->iSubItem == DCOL_EXCHANGE) {
                        SelectObject(cd->nmcd.hdc, hFont12ptbold.get());
                        cd->clrText = darkMode ? DM_TEXT : LM_TEXT;
                        if (darkMode) cd->clrTextBk = (cd->nmcd.dwItemSpec % 2 == 0) ? DM_BG : DM_BG2;
                        return CDRF_NEWFONT;
                    }
                    if (cd->iSubItem == DCOL_VWAP) {
                        if (cacheRow.textCols[DCOL_VWAP] != DIAMONDS_NO_DATA && !cacheRow.textCols[DCOL_VWAP].empty()) {
                            double diff = cacheRow.sortValues[DCOL_VWAP];
                            if      (diff > 0.0) cd->clrText = COINS_CLR_GREEN;
                            else if (diff < 0.0) cd->clrText = COINS_CLR_RED;
                            else cd->clrText = darkMode ? DM_TEXT : LM_TEXT;
                        }
                        if (darkMode) cd->clrTextBk = (cd->nmcd.dwItemSpec % 2 == 0) ? DM_BG : DM_BG2;
                        SelectObject(cd->nmcd.hdc, hFont14pt.get());
                        return CDRF_NEWFONT;
                    }
                    if (cd->iSubItem == DCOL_VOLRATE) {
                        if (cacheRow.textCols[DCOL_VOLRATE] != DIAMONDS_NO_DATA && !cacheRow.textCols[DCOL_VOLRATE].empty()) {
                            double ratio = cacheRow.volRatio;
                            if      (ratio >= 3.0) cd->clrText = COINS_CLR_PINK;
                            else if (ratio >= 1.5) cd->clrText = COINS_CLR_PURPLE;
                            else cd->clrText = COINS_CLR_BLUE;
                        } else {
                            cd->clrText = darkMode ? DM_TEXT : LM_TEXT;
                        }
                        if (darkMode) cd->clrTextBk = (cd->nmcd.dwItemSpec % 2 == 0) ? DM_BG : DM_BG2;
                        SelectObject(cd->nmcd.hdc, hFont14pt.get());
                        return CDRF_NEWFONT;
                    }
                    // Only colour P&L / change columns, and only when the
                    // cell holds a real numeric value (not the "--" sentinel).
                    if (cd->iSubItem == DCOL_CHGPCT || cd->iSubItem == DCOL_DAILYPNL || cd->iSubItem == DCOL_POSITION || cd->iSubItem == DCOL_CHG5MIN || cd->iSubItem == DCOL_CHG13WEEK || cd->iSubItem == DCOL_CHG26WEEK || cd->iSubItem == DCOL_CHG52WEEK) {
                        double val = cacheRow.sortValues[cd->iSubItem];
                        if (val < BOTTOM_SORT_VALUE) val = 0.0;
                        // Guard: skip colouring the "--" sentinel, atof("--") == 0
                        // which would leave the cell uncoloured anyway, but being
                        // explicit avoids any locale-specific atof surprises.
                        if      (val > 0.0) cd->clrText = COINS_CLR_GREEN;
                        else if (val < 0.0) cd->clrText = COINS_CLR_RED;
                        else cd->clrText = darkMode ? DM_TEXT : LM_TEXT;
                        if (darkMode) cd->clrTextBk = (cd->nmcd.dwItemSpec % 2 == 0) ? DM_BG : DM_BG2;
                        if (cd->iSubItem == DCOL_CHG5MIN) {
                            SelectObject(cd->nmcd.hdc, hFont14pt.get());
                        } else {
                            SelectObject(cd->nmcd.hdc, hFont16pt.get());
                        }
                        // For the Position cell also request post-paint so we can
                        // overlay the mini sparkline after the text is drawn.
                        if (cd->iSubItem == DCOL_POSITION)
                            return CDRF_NEWFONT | CDRF_NOTIFYPOSTPAINT;
                        return CDRF_NEWFONT;
                    }
                    if (cd->iSubItem == DCOL_UNREALIZED_PL) {
                        int rowIndex = (int)cd->nmcd.dwItemSpec;
                        
                        HBRUSH hBrush = darkMode ? ((cd->nmcd.dwItemSpec % 2 == 0) ? hDarkBrush : hDarkBrush2) 
                                                : ((cd->nmcd.dwItemSpec % 2 == 0) ? hLightBrushBg : hLightBrushBg2);
                        
                        RECT rcCell;
                        ListView_GetSubItemRect(cd->nmcd.hdr.hwndFrom, rowIndex, cd->iSubItem, LVIR_BOUNDS, &rcCell);
                        double val = cacheRow.sortValues[DCOL_UNREALIZED_PL];
                        COLORREF topColor = val > 0.0 ? COINS_CLR_GREEN : val < 0.0 ? COINS_CLR_RED : COINS_CLR_GRAY;
                        COLORREF bottomColor = val > 0.0 ? COINS_CLR_GREEN_DARK2 : val < 0.0 ? COINS_CLR_RED_DARK2 : COINS_CLR_GRAY;
                        DrawTwoLineCell(cd->nmcd.hdc, rcCell,
                                        cacheRow.textCols[DCOL_UNREALIZED_PL].c_str(), topColor,
                                        cacheRow.unrealizedPnLPctStr.c_str(), bottomColor, hBrush);
                        return CDRF_SKIPDEFAULT;
                    }
                    if (cd->iSubItem == DCOL_SIZE) {
                        int rowIndex = (int)cd->nmcd.dwItemSpec;
                        
                        HBRUSH hBrush = darkMode ? ((cd->nmcd.dwItemSpec % 2 == 0) ? hDarkBrush : hDarkBrush2) 
                                                : ((cd->nmcd.dwItemSpec % 2 == 0) ? hLightBrushBg : hLightBrushBg2);
                        
                        RECT rcCell;
                        ListView_GetSubItemRect(cd->nmcd.hdr.hwndFrom, rowIndex, cd->iSubItem, LVIR_BOUNDS, &rcCell);
                        COLORREF sizeColor = cacheRow.halted ? COINS_CLR_GRAY
                            : cacheRow.askSize > cacheRow.bidSize ? COINS_CLR_RED
                            : cacheRow.askSize < cacheRow.bidSize ? COINS_CLR_GREEN
                            : COINS_CLR_BLUE;
                        DrawTwoLineCell(cd->nmcd.hdc, rcCell,
                                        cacheRow.askSizeStr.c_str(), sizeColor,
                                        cacheRow.bidSizeStr.c_str(), sizeColor, hBrush);
                        return CDRF_SKIPDEFAULT;
                    }
                    if (cd->iSubItem == DCOL_LAST) {
                        double last = cacheRow.sortValues[DCOL_LAST];
                        double high = cacheRow.dayHigh;
                        double low = cacheRow.dayLow;
                        double prevClose = cacheRow.prevClose;

                        if (last > 0.0 && high > 0.0 && low > 0.0 && high > low) {
                            double pct = (last - low) / (high - low) * 100.0; // 0% at low, 100% at high
                            if (pct <= 25.0) cd->clrText = COINS_CLR_RED;
                            else if (pct >= 75.0) cd->clrText = COINS_CLR_GREEN;
                            else cd->clrText = darkMode ? DM_TEXT : LM_TEXT;
                        } else if (last > 0.0 && prevClose > 0.0) {
                            if (last > prevClose) cd->clrText = COINS_CLR_GREEN;
                            else if (last < prevClose) cd->clrText = COINS_CLR_RED;
                            else cd->clrText = darkMode ? DM_TEXT : LM_TEXT;
                        } else {
                            cd->clrText = darkMode ? DM_TEXT : LM_TEXT;
                        }
                        if (darkMode) cd->clrTextBk = (cd->nmcd.dwItemSpec % 2 == 0) ? DM_BG : DM_BG2;
                        SelectObject(cd->nmcd.hdc, hFont16pt.get());
                        return CDRF_NEWFONT;
                    }
                    if (cd->iSubItem == DCOL_BIDASK) {
                        int rowIndex = (int)cd->nmcd.dwItemSpec;
                        
                        HBRUSH hBrush = darkMode ? ((cd->nmcd.dwItemSpec % 2 == 0) ? hDarkBrush : hDarkBrush2) 
                                                : ((cd->nmcd.dwItemSpec % 2 == 0) ? hLightBrushBg : hLightBrushBg2);
                        
                        RECT rcCell;
                        ListView_GetSubItemRect(cd->nmcd.hdr.hwndFrom, rowIndex, cd->iSubItem, LVIR_BOUNDS, &rcCell);
                        double last = cacheRow.sortValues[DCOL_LAST];
                        COLORREF quoteColor = COINS_CLR_WHITE;
                        if (cacheRow.halted || last <= 0.0 || cacheRow.ask <= 0.0 || cacheRow.bid <= 0.0) {
                            quoteColor = COINS_CLR_GRAY;
                        } else {
                            double distAsk = std::fabs(cacheRow.ask - last);
                            double distBid = std::fabs(last - cacheRow.bid);
                            if (distAsk < distBid) quoteColor = COINS_CLR_GREEN;
                            else if (distBid < distAsk) quoteColor = COINS_CLR_RED;
                        }
                        DrawTwoLineCell(cd->nmcd.hdc, rcCell,
                                        cacheRow.askStr.c_str(), quoteColor,
                                        cacheRow.bidStr.c_str(), quoteColor, hBrush);
                        return CDRF_SKIPDEFAULT;
                    }
                    if (cd->iSubItem == DCOL_MKTVAL) {
                        int rowIndex = (int)cd->nmcd.dwItemSpec;
                        
                        HBRUSH hBrush = darkMode ? ((cd->nmcd.dwItemSpec % 2 == 0) ? hDarkBrush : hDarkBrush2) 
                                                : ((cd->nmcd.dwItemSpec % 2 == 0) ? hLightBrushBg : hLightBrushBg2);
                        
                        RECT rcCell;
                        ListView_GetSubItemRect(cd->nmcd.hdr.hwndFrom, rowIndex, cd->iSubItem, LVIR_BOUNDS, &rcCell);
                        DrawTwoLineCell(cd->nmcd.hdc, rcCell,
                                        cacheRow.textCols[DCOL_MKTVAL].c_str(), darkMode ? DM_TEXT : LM_TEXT,
                                        cacheRow.pctNetLiq.c_str(), COINS_CLR_GRAY, hBrush);
                        return CDRF_SKIPDEFAULT;
                    }
                    if (cd->iSubItem == DCOL_AVGPRICE) {
                        if (darkMode) {
                            cd->clrTextBk = (cd->nmcd.dwItemSpec % 2 == 0) ? DM_BG : DM_BG2;
                            cd->clrText   = DM_TEXT;
                        } else {
                            cd->clrTextBk = (cd->nmcd.dwItemSpec % 2 == 0) ? GetSysColor(COLOR_WINDOW) : RGB(245, 245, 245);
                            cd->clrText   = LM_TEXT;
                        }
                        SelectObject(cd->nmcd.hdc, hFont14pt.get());
                        return CDRF_NEWFONT;
                    }
                    if (cd->iSubItem == DCOL_ALERT) {
                        int rowIndex = (int)cd->nmcd.dwItemSpec;
                        double last = cacheRow.sortValues[DCOL_LAST];
                        
                        HBRUSH hBrush = darkMode ? ((cd->nmcd.dwItemSpec % 2 == 0) ? hDarkBrush : hDarkBrush2) 
                                                : ((cd->nmcd.dwItemSpec % 2 == 0) ? hLightBrushBg : hLightBrushBg2);
                        
                        RECT rcCell;
                        ListView_GetSubItemRect(cd->nmcd.hdr.hwndFrom, rowIndex, cd->iSubItem, LVIR_BOUNDS, &rcCell);
                        COLORREF upColor = (last > 0 && cacheRow.upAlert > 0 && cacheRow.upAlert <= last) ? COINS_CLR_GREEN : COINS_CLR_GRAY;
                        COLORREF downColor = (last > 0 && cacheRow.downAlert > 0 && cacheRow.downAlert >= last) ? COINS_CLR_RED : COINS_CLR_GRAY;
                        DrawTwoLineCell(cd->nmcd.hdc, rcCell,
                                        cacheRow.upStr.c_str(), upColor,
                                        cacheRow.downStr.c_str(), downColor, hBrush);
                        return CDRF_SKIPDEFAULT;
                    }
                    if (cd->iSubItem == DCOL_DIV_YIELD || cd->iSubItem == DCOL_DIV_DATE  ||  cd->iSubItem == DCOL_DIV_AMT || cd->iSubItem == DCOL_ANNUAL_DIV) {
                        cd->clrText = COINS_CLR_PURPLE;
                        if (darkMode) cd->clrTextBk = (cd->nmcd.dwItemSpec % 2 == 0) ? DM_BG : DM_BG2;
                        SelectObject(cd->nmcd.hdc, hFont14pt.get());
                        return CDRF_NEWFONT;
                    }
                    return CDRF_DODEFAULT;
                }

                case CDDS_ITEMPOSTPAINT | CDDS_SUBITEM: {
                    if (cd->iSubItem != DCOL_POSITION) return CDRF_DODEFAULT;

                    int rowIndex = (int)cd->nmcd.dwItemSpec;
                    if (rowIndex < 0 || rowIndex >= diamondsState.displayOrder.size()) return CDRF_DODEFAULT;

                    int conId = diamondsState.displayOrder[rowIndex];
                    auto sit  = diamondsState.sparklines.find(conId);
                    if (sit == diamondsState.sparklines.end() || !sit->second.HasData())
                        return CDRF_DODEFAULT;

                    RECT cellRect;
                    ListView_GetSubItemRect(GetDlgItem(hWnd, ID_DIAMONDS_RESULTS_LIST), rowIndex, DCOL_POSITION, LVIR_BOUNDS, &cellRect);
                    // The list view already paints through its own double buffer.
                    // Drawing through a second buffer here can copy an intermediate
                    // frame back over the row while the list is being invalidated.
                    sit->second.Draw(cd->nmcd.hdc, cellRect);
                    return CDRF_DODEFAULT;
                }
            }
        }
        break;
    }

    case WM_TIMER: {
        if (wParam == TIMER_DIAMONDS_VIEW) {
            // Persist the last viewed ID across timer ticks
            static int lastViewedConId = 0;
            
            if (!diamondsState.displayOrder.empty()) {
                size_t nextIdx = 0;
                if (diamondsState.viewSelectionEnabled == ID_VIEW_SELECTIONS_ANY_BTN) {
                    // Locate the last viewed item's current position to handle sorting/sizing changes gracefully
                    auto it = std::find(diamondsState.displayOrder.begin(), diamondsState.displayOrder.end(), lastViewedConId);
                    if (it != diamondsState.displayOrder.end()) {
                        nextIdx = (std::distance(diamondsState.displayOrder.begin(), it) + 1) % diamondsState.displayOrder.size();
                    }
                }

                int conId = diamondsState.displayOrder[nextIdx];

                if (lastViewedConId != conId) {
                    lastViewedConId = conId;
                    api().updateDisplayGroup(conId);
                }
            }
        }
        if (wParam == TIMER_DIAMONDS_SORT) {
            HWND hList = GetDlgItem(hWnd, ID_DIAMONDS_RESULTS_LIST);
            Diamonds_ApplySort(hList);
            InvalidateRect(hList, NULL, FALSE);
        }
        if (wParam == TIMER_DIAMONDS_PAINT) {
            Diamonds_FlushDirty(hWnd);
        }
        if (wParam == TIMER_DIAMONDS_CAPTION) {
            KillTimer(hWnd, TIMER_DIAMONDS_CAPTION);
            Diamonds_InvalidateCaption(hWnd);
        }
        break;
    }

    case WM_DESTROY:
        KillTimer(hWnd, TIMER_DIAMONDS_SORT);
        KillTimer(hWnd, TIMER_DIAMONDS_VIEW);
        KillTimer(hWnd, TIMER_DIAMONDS_PAINT);
        KillTimer(hWnd, TIMER_DIAMONDS_CAPTION);
        api().removeApiUpdateWindow(hWnd);
        diamondsState.dataCache.clear();
        diamondsState.dirtyMarket.clear();
        diamondsState.dirtyRedraw.clear();
        diamondsState.dirty = false;
        diamondsState.sparklines.clear();
        diamondsState.dividendCache.clear();   // NEW
        if (diamondsState.rowHeightImageList) {
            ImageList_Destroy(diamondsState.rowHeightImageList);
            diamondsState.rowHeightImageList = NULL;
        }
        break;
    }

    return HandleCommonMessages(hWnd, message, wParam, lParam);
}
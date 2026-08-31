#pragma once
// "Proxima Nova", Verdana, Arial, sans-serif
int windowDiamondsWidth = 1380;
void StartDiamonds() { StartGenericWindow(DIAMONDS_CLASS_NAME, "Diamonds", L"TWSAPIClientTradingFloor.Diamonds", windowDiamondsWidth, 420); }

#define ID_DIAMONDS_RESULTS_LIST 7001
#define ID_DIAMONDS_CHK_0        7002   // "Growth"
#define ID_DIAMONDS_CHK_1        7003   // "Dividends"
#define ID_DIAMONDS_CHK_2        7004   // "Quarantine"
#define DIAMONDS_CHK_STRIP_H     32     // height of the checkbox bar at the bottom

// ── Filter / tab constants ────────────────────────────────────────────────────
#define DTAB_ALL              0
#define DTAB_GROWTH           1
#define DTAB_QUARENTINE       2
#define DIAMONDS_TAB_COUNT    3

static const char* diamondTabNames[DIAMONDS_TAB_COUNT] = { "Growth", "Dividends", "Quarantine" };

// Bitmask: bit N set means group N is currently visible.  Default = all visible.
static UINT diamondsCheckedTabs = 0x7;

// Maps conId → assigned group (DTAB_ALL = untagged = shown when bit 0 is set).
static std::map<int,int> diamondsTabMap;

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

// Maps conId → color index (0..DIAMONDS_COLOR_COUNT-1), or not present = inherit.
static std::map<int,int> diamondsSymbolColors;

// ── Alert-only pseudo rows ────────────────────────────────────────────────
// A symbol with an Alert Up/Down set but that isn't a current position has
// no real conId. The whole Diamonds ListView (display order, sort, custom
// draw, context menu) is keyed by conId, so each such symbol gets a stable,
// unique synthetic *negative* id for the duration of this session — real
// IBKR conIds are always positive, so there's no collision risk.
static std::unordered_map<std::string, int> diamondsAlertOnlyIds;
static int diamondsNextAlertOnlyId = -1;

static int Diamonds_GetOrCreateAlertOnlyId(const std::string& symbol) {
    auto it = diamondsAlertOnlyIds.find(symbol);
    if (it != diamondsAlertOnlyIds.end()) return it->second;
    int id = diamondsNextAlertOnlyId--;
    diamondsAlertOnlyIds[symbol] = id;
    return id;
}

static bool diamondsChkVisible = false;

// ── Deferred sort (prevents flicker on every tick) ────────────────────────────
#define TIMER_DIAMONDS_SORT      7010
#define DIAMONDS_SORT_TIMER_MS   7000   // re-sort at most every 5 seconds (or sooner if user clicks a column header)


// ── Column indices (keep in sync with diamondCols[]) ─────────────────────────
enum DiamondColIdx {
    DCOL_SYMBOL = 0,
    DCOL_POSITION,
    DCOL_AVGPRICE,
    DCOL_ALERTUP,
    DCOL_ALERTDOWN,
    DCOL_ASKSIZE,
    DCOL_ASK,
    DCOL_LAST,
    DCOL_BID,
    DCOL_BIDSIZE,
    DCOL_VWAP,
    DCOL_CHG5MIN,
    DCOL_DAILYPNL,
    DCOL_CHGPCT,
    DCOL_CHG13WEEK,
    DCOL_CHG26WEEK,
    DCOL_CHG52WEEK,
    DCOL_UNREALIZED_PL,
    DCOL_UNREALIZED_PL_PCT,
    DCOL_MKTVAL,
    DCOL_PCT_NETLIQ,
    DCOL_DIV_YIELD,
    DCOL_DIV_DATE,
    DCOL_DIV_AMT,
    DCOL_ANNUAL_DIV,
    DCOL_COUNT
};

// ── Sort state ────────────────────────────────────────────────────────────────
static int  diamondsSortCol = DCOL_SYMBOL;
static bool diamondsSortAsc = true;

// Keyed by conId. Populated / updated in Diamonds_UpdateMarketCols.
static std::map<int, MiniSparkline> diamondsSparklines;

// ── Unified Virtual List Cache (Replaces diamondsPnlCache) ─────────────────
struct DiamondRowCache {
    int conId = 0;
    std::string symbol;
    double sortValues[DCOL_COUNT] = {0.0};  // Raw doubles for fast sorting
    std::string textCols[DCOL_COUNT];       // Pre-formatted strings for instant UI painting

    // Day high/low — used only to color DCOL_LAST based on where `last` sits
    // within today's range (see WM_NOTIFY / NM_CUSTOMDRAW).
    double dayHigh = 0.0;
    double dayLow = 0.0;
    double prevClose = 0.0;
    bool halted = false;
};

// Data storage: Fast O(1) lookup by conId for live data streams
static std::map<int, DiamondRowCache> diamondDataCache;

// UI Viewport: Holds conIds in sorted order. The ListView only knows about this vector's size.
static std::vector<int> diamondDisplayOrder;

// Paint Limiter
static bool diamondsDirty = false;
#define TIMER_DIAMONDS_PAINT 7011
#define DIAMONDS_PAINT_TIMER_MS  60     // ~16 FPS (Butter smooth, zero flicker)

// ── Column definitions ────────────────────────────────────────────────────────

struct DiamondCol { const char* header; int width; int fmt; };
static const DiamondCol diamondCols[] = {
    { "Symbol",            90, LVCFMT_LEFT  },
    { "Position",         110, LVCFMT_RIGHT },
    { "AvgPx",             85, LVCFMT_RIGHT },
    { "AlertUp",           85, LVCFMT_RIGHT },
    { "AlertDown",         85, LVCFMT_RIGHT },
    { "Asks",              70, LVCFMT_RIGHT },
    { "Ask",               90, LVCFMT_RIGHT },
    { "Last",              90, LVCFMT_RIGHT },
    { "Bid",               90, LVCFMT_RIGHT },
    { "Bids",              70, LVCFMT_RIGHT },
    { "VWAP",              80, LVCFMT_RIGHT },
    { "5m",                80, LVCFMT_RIGHT },
    { "Daily",             90, LVCFMT_RIGHT },  // {"fix_tag":7681,"name":"Price/EMA(20)","description":"Price to Exponential moving average (N = 20) ratio - 1, displayed in percents","groups":["G40"],"id":"PRICE_VS_EMA20"}
    { "Change %",          95, LVCFMT_RIGHT },  // {"fix_tag":7679,"name":"Price/EMA(100)","description":"Price to Exponential moving average (N = 100) ratio - 1, displayed in percents","groups":["G40"],"id":"PRICE_VS_EMA100"}
    { "13w",              115, LVCFMT_RIGHT },
    { "26w",              115, LVCFMT_RIGHT },
    { "52w",              115, LVCFMT_RIGHT },
    { "Unrealized",       100, LVCFMT_RIGHT },
    { "Unrealized %",     105, LVCFMT_RIGHT },
    { "Value",             95, LVCFMT_RIGHT },
    { "Net %",             85, LVCFMT_RIGHT },
    { "Yield %",           90, LVCFMT_RIGHT },
    { "Date",             125, LVCFMT_RIGHT },
    { "Amount",            85, LVCFMT_RIGHT },
    { "Annual",            85, LVCFMT_RIGHT },
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

    bool showDiv   = (diamondsCheckedTabs & (1u << 1)) != 0;   // Dividends
    bool showWeeks = (diamondsCheckedTabs & (1u << 2)) != 0;   // Quarantine

    for (int i = DCOL_DIV_YIELD; i <= DCOL_ANNUAL_DIV; ++i) {
        ListView_SetColumnWidth(hList, i, showDiv ? diamondCols[i].width : 0);
    }
    for (int i = DCOL_CHG13WEEK; i <= DCOL_CHG52WEEK; ++i) {
        ListView_SetColumnWidth(hList, i, showWeeks ? diamondCols[i].width : 0);
    }
    for (int i = DCOL_AVGPRICE; i <= DCOL_ALERTDOWN; ++i) {
        ListView_SetColumnWidth(hList, i, showWeeks ? diamondCols[i].width : 0);
    }

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
                      diamondCols[DCOL_ALERTUP].width + diamondCols[DCOL_ALERTDOWN].width;
    }
    if (extraWidth > 0) extraWidth += 10; // margin, same buffer the original single-group case used

    RECT windowRect;
    GetWindowRect(hWnd, &windowRect);
    MoveWindow(hWnd, windowRect.left, windowRect.top, windowDiamondsWidth + extraWidth, windowRect.bottom - windowRect.top, TRUE);
}

static HIMAGELIST diamondsRowHeightImageList = NULL;

static void Diamonds_SetRowHeight(HWND hList, int rowHeight) {
    if (diamondsRowHeightImageList) {
        ImageList_Destroy(diamondsRowHeightImageList);
        diamondsRowHeightImageList = NULL;
    }
    // Width can stay tiny (1px) since LVS_REPORT never shows the icon glyph
    // area when there's no LVCFMT_IMAGE column, but height controls row height.
    diamondsRowHeightImageList = ImageList_Create(1, rowHeight, ILC_COLOR32 | ILC_MASK, 1, 1);
    if (!diamondsRowHeightImageList) return;

    // Add one fully-transparent 1x1 bitmap so the image list is non-empty.
    HBITMAP hbmImage = CreateBitmap(1, rowHeight, 1, 1, NULL);
    HBITMAP hbmMask  = CreateBitmap(1, rowHeight, 1, 1, NULL);
    ImageList_Add(diamondsRowHeightImageList, hbmImage, hbmMask);
    DeleteObject(hbmImage);
    DeleteObject(hbmMask);

    ListView_SetImageList(hList, diamondsRowHeightImageList, LVSIL_SMALL);
}

// ── Registry persistence for tab assignments ──────────────────────────────────

// Saves diamondsTabMap to the registry as two space-separated conId lists.
static void Diamonds_SaveTabMap() {
    std::string growthList, quarentineList;
    for (auto& [conId, tab] : diamondsTabMap) {
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

// Loads diamondsTabMap from the registry.
static void Diamonds_LoadTabMap() {
    diamondsTabMap.clear();
    auto parseIds = [](const std::string& s, int tab) {
        size_t start = 0;
        while (start < s.size()) {
            size_t end = s.find(' ', start);
            if (end == std::string::npos) end = s.size();
            if (end > start) {
                try { diamondsTabMap[std::stoi(s.substr(start, end - start))] = tab; }
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
    for (auto& [conId, idx] : diamondsSymbolColors) {
        if (!s.empty()) s += ' ';
        s += std::to_string(conId) + ':' + std::to_string(idx);
    }
    Settings_SymbolColors_Save(s);
}

static void Diamonds_LoadSymbolColors() {
    diamondsSymbolColors.clear();
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
                    diamondsSymbolColors[conId] = idx;
            } catch (...) {}
        }
        pos = end + 1;
    }
}

// Drops diamondsTabMap entries for conIds that are no longer a held
// position, then persists the pruned map. Also drops diamondsSymbolColors
// entries, but only when the symbol is neither a current position NOR has an
// alert set — a symbol with an alert is allowed to keep a color override
// even while not held.
static void Diamonds_CleanupStaleTabAssignments() {
    std::unordered_set<int> liveConIds;
    {
        std::lock_guard<std::mutex> lock(api().getPortfolioMutex());
        for (auto const& [conId, info] : api().getPortfolioMap())
            liveConIds.insert(conId);
    }

    if (!diamondsTabMap.empty()) {
        bool changedTabs = false;
        for (auto it = diamondsTabMap.begin(); it != diamondsTabMap.end(); ) {
            if (!liveConIds.count(it->first)) { it = diamondsTabMap.erase(it); changedTabs = true; }
            else ++it;
        }
        if (changedTabs) Diamonds_SaveTabMap();
    }

    if (!diamondsSymbolColors.empty()) {
        bool changedColors = false;
        for (auto it = diamondsSymbolColors.begin(); it != diamondsSymbolColors.end(); ) {
            bool isLive = liveConIds.count(it->first) != 0;
            bool hasAlert = false;
            if (!isLive) {
                auto cacheIt = diamondDataCache.find(it->first);
                if (cacheIt != diamondDataCache.end() && !cacheIt->second.symbol.empty()) {
                    std::string up, down;
                    hasAlert = Settings_Alerts_Load(cacheIt->second.symbol, up, down);
                }
            }
            if (!isLive && !hasAlert) { it = diamondsSymbolColors.erase(it); changedColors = true; }
            else ++it;
        }
        if (changedColors) Diamonds_SaveSymbolColors();
    }
}

// Drops "Dividends" registry values (written by Settings_Dividends_Save, see
// registry.h) for conIds that are no longer a held position. That subkey is a
// pure fetch-once-per-session cache keyed by conId with nothing else pruning
// it, so closed-out positions would otherwise accumulate there forever — same
// rationale as Diamonds_CleanupStaleTabAssignments() above, just aimed at a
// different registry subkey (Dividends instead of Tab_*/SymbolColors).
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
        return; // no Dividends subkey yet — nothing to clean

    std::vector<std::string> toDelete;
    char valueName[64];
    DWORD index = 0;
    while (true) {
        DWORD nameSize = sizeof(valueName);
        if (RegEnumValueA(hKey, index++, valueName, &nameSize, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
            break;

        int conId = 0;
        try { conId = std::stoi(std::string(valueName)); }
        catch (...) { continue; } // value name isn't conId-shaped — leave it alone

        if (!liveConIds.count(conId))
            toDelete.push_back(valueName);
    }
    RegCloseKey(hKey);

    for (const auto& name : toDelete)
        RegDelete("Dividends", name.c_str());
}

// ── Layout ────────────────────────────────────────────────────────────────────

static void Diamonds_Layout(HWND hWnd) {
    HWND hList = GetDlgItem(hWnd, ID_DIAMONDS_RESULTS_LIST);
    if (!hList) return;
    RECT rc; GetClientRect(hWnd, &rc);
    int listH = diamondsChkVisible ? rc.bottom - DIAMONDS_CHK_STRIP_H : rc.bottom;
    MoveWindow(hList, 0, 0, rc.right, listH, TRUE);

    if (!diamondsChkVisible) return;

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
}

static void Diamonds_ShowCheckboxes(HWND hWnd, bool show) {
    if (diamondsChkVisible == show) return;
    diamondsChkVisible = show;
    int sw = show ? SW_SHOW : SW_HIDE;
    for (int i = 0; i < DIAMONDS_TAB_COUNT; ++i)
        ShowWindow(GetDlgItem(hWnd, ID_DIAMONDS_CHK_0 + i), sw);
    Diamonds_Layout(hWnd);
}

// ── Virtual Sort ──────────────────────────────────────────────────────────────

static void Diamonds_ApplySort(HWND hList) {
    if (diamondDisplayOrder.empty()) return;

    std::sort(diamondDisplayOrder.begin(), diamondDisplayOrder.end(), [](int idA, int idB) {
        const auto& a = diamondDataCache[idA];
        const auto& b = diamondDataCache[idB];

        if (diamondsSortCol == DCOL_SYMBOL) {
            int cmp = _stricmp(a.textCols[DCOL_SYMBOL].c_str(), b.textCols[DCOL_SYMBOL].c_str());
            return diamondsSortAsc ? (cmp < 0) : (cmp > 0);
        } else {
            double v1 = a.sortValues[diamondsSortCol];
            double v2 = b.sortValues[diamondsSortCol];
            if (v1 == v2) return false;
            if (diamondsSortCol == DCOL_DIV_DATE) {
                return diamondsSortAsc ? (v1 > v2) : (v1 < v2);
            } else {
                return diamondsSortAsc ? (v1 < v2) : (v1 > v2);
            }
        }
    });

    // ZERO-FLICKER FIX: Delegate to the paint timer instead of invalidating instantly
    diamondsDirty = true;
}
// ── Helpers ───────────────────────────────────────────────────────────────────

// Sentinel string displayed whenever a value cannot be computed (e.g. market closed, last == 0).
static const char* DIAMONDS_NO_DATA = "--";


static void Diamonds_UpdatePnLCols(HWND hWnd, int conId) {
    // Grab our new unified cache row
    auto& row = diamondDataCache[conId];
    row.conId = conId; 
    
    TradingAPI::PnlSinglePayload pnlSingle;
    double avgCost = 0.0;
    {
        std::lock_guard<std::mutex> lk(api().getPortfolioMutex());
        auto& pm = api().getPortfolioMap();
        auto it = pm.find(conId);
        if (it != pm.end()) {
            pnlSingle = it->second.pnlSingle;
            avgCost = it->second.avgCost;
        }
    }

    if (pnlSingle.conId > 0) {
        //if (pnlSingle.has_daily) {
            row.sortValues[DCOL_DAILYPNL] = pnlSingle.dailyPnL;
            row.textCols[DCOL_DAILYPNL] = std::format("{:+.2f}", pnlSingle.dailyPnL);
        //}
        if (pnlSingle.has_unrealized) {
            row.sortValues[DCOL_UNREALIZED_PL] = pnlSingle.unrealizedPnL;
            row.textCols[DCOL_UNREALIZED_PL] = std::format("{:+.2f}", pnlSingle.unrealizedPnL);

            // Recompute the % column — derived from unrealizedPnL / cost basis,
            // NOT from last price, so it stays valid even when last == 0
            // (market closed / no quote yet) and matches IBKR's own PnL figure
            // rather than a reconstruction from price.
            double shares = row.sortValues[DCOL_POSITION];
            double costBasis = avgCost * std::fabs(shares);
            if (costBasis > 0.0) {
                double pct = pnlSingle.unrealizedPnL / costBasis * 100.0;
                row.sortValues[DCOL_UNREALIZED_PL_PCT] = pct;
                row.textCols[DCOL_UNREALIZED_PL_PCT] = std::format("{:+.2f}%", pct);
            } else {
                row.sortValues[DCOL_UNREALIZED_PL_PCT] = -999999.0;
                row.textCols[DCOL_UNREALIZED_PL_PCT] = "--";
            }
        }

        diamondsDirty = true;
    }
}

static void Diamonds_UpdateMarketCols(int conId, const TradingAPI::L1Book& t) {
    // Ensure a cache row exists even if this tick arrived before
    // Diamonds_Repopulate had a chance to create one for it — e.g. right after
    // the window is closed and reopened, a WM_MARKET_L1 posted just before the
    // repopulate finishes used to hit this function while the cache was still
    // empty/rebuilding and the early-return below silently dropped the tick
    // for good (nothing ever re-requested it). That could look exactly like
    // "some columns stop updating" after a close/reopen cycle. Auto-creating
    // the row (mirroring what Diamonds_UpdatePnLCols already does) means the
    // tick is never lost; if the row isn't in diamondDisplayOrder yet it
    // simply becomes visible on the next repopulate/sort instead of vanishing.
    auto& row = diamondDataCache[conId];
    row.conId = conId;
    // Helper to write both sortable raw data and display string
    auto setCol = [&](int col, double val, std::string_view fmt, bool alwaysShow = false) {
        row.sortValues[col] = val;
        if (val != 0.0 || alwaysShow) {
            row.textCols[col] = std::vformat(fmt, std::make_format_args(val));
        } else {
            row.textCols[col] = "";
        }
    };

    auto setNA = [&](int col, std::string placeHolder = DIAMONDS_NO_DATA) {
        row.sortValues[col] = -999999.0; // Pushes NA to bottom on sorts
        row.textCols[col] = placeHolder;
    };

    setCol(DCOL_ASKSIZE, t.askSize, "{:.0f}");
    setCol(DCOL_ASK,     t.ask,     "{:.2f}");
    setCol(DCOL_BID,     t.bid,     "{:.2f}");
    setCol(DCOL_BIDSIZE, t.bidSize, "{:.0f}");

    setCol(DCOL_DIV_AMT, t.dividendAmount,  "{:.3f}");
    setCol(DCOL_ANNUAL_DIV, t.annualDividends, "{:.3f}");
    
    row.textCols[DCOL_DIV_DATE] = t.dividendDate;
    row.sortValues[DCOL_DIV_DATE] = t.dividendDateSortable;

    if (t.last > 0.0 && t.annualDividends > 0.0) setCol(DCOL_DIV_YIELD, t.dividendYield(), "{:.2f}%");
    else if (t.annualDividends == 0.0) setCol(DCOL_DIV_YIELD, 0.0, "");
    else setNA(DCOL_DIV_YIELD);

    double closeAgo13Week = 0.0, closeAgo26Week = 0.0, closeAgo52Week = 0.0;
    {
        std::lock_guard<std::mutex> lock(api().getPortfolioMutex());
        auto& pm = api().getPortfolioMap();
        auto pit = pm.find(conId);
        if (pit != pm.end()) {
            closeAgo13Week = pit->second.closeAgo13Week;
            closeAgo26Week = pit->second.closeAgo26Week;
            closeAgo52Week = pit->second.closeAgo52Week;
        }
    }
    auto setWeekChangePct = [&](int col, double closeAgo) {
        if (closeAgo > 0.0 && t.last > 0.0) {
            double pct = (t.last - closeAgo) / closeAgo * 100.0;
            row.sortValues[col] = pct;
            row.textCols[col]   = std::format("{:+.2f}%", pct);
        } else {
            setNA(col);
        }
    };

    setWeekChangePct(DCOL_CHG13WEEK, closeAgo13Week);
    setWeekChangePct(DCOL_CHG26WEEK, closeAgo26Week);
    setWeekChangePct(DCOL_CHG52WEEK, closeAgo52Week);

    // Day high/low, used by the Last column's custom-draw color (see WM_NOTIFY/NM_CUSTOMDRAW).
    row.dayHigh = t.high;
    row.dayLow = t.low;
    row.prevClose = t.prevClose;
    row.halted = t.halted;

    double shares = row.sortValues[DCOL_POSITION];
    double netLiq = 0.0;
    auto summary = api().getAccountSummary();
    if (summary.count("NetLiquidation")) {
        try { netLiq = std::stod(summary["NetLiquidation"]); } catch (...) {}
    }

    double mktVal = shares * (t.last > 0 ? t.last : t.prevClose);
    double pctNetLiq = (netLiq > 0.0 && mktVal != 0.0) ? (mktVal / netLiq * 100.0) : 0.0;

    setCol(DCOL_MKTVAL, mktVal, "{:.2f}", true);
    setCol(DCOL_PCT_NETLIQ, pctNetLiq, "{:.2f}%", true);
    
    if (t.last <= 0.0) {
        setNA(DCOL_LAST); setNA(DCOL_CHGPCT);
        setNA(DCOL_CHG5MIN);
        setNA(DCOL_VWAP);
        return;
    }

    setCol(DCOL_LAST, t.last, "{:.2f}", true);
    diamondsSparklines[conId].AddPrice(t.last);

    // ── VWAP: display the VWAP price, but sort by (Last - VWAP) so the
    // column ranks by how far price has drifted from VWAP, not by VWAP itself. ──
    double vwapDiff = (t.vwap > 0.0 && t.last > 0.0) ? t.last - t.vwap : 0.0;
    setCol(DCOL_VWAP, vwapDiff, "{:+.2f}", true);

    // 5-minute price change, in dollars — Last vs. the price ~5 minutes ago,
    // sampled from the same long-lived history the sparkline's reference dots
    // use. Shows "--" until at least 5 minutes of history has accumulated
    // for this symbol (same "appears once ready" behavior as those dots).
    {
        double price5MinAgo = 0.0;
        if (diamondsSparklines[conId].GetPriceMinutesAgo(5, price5MinAgo) && price5MinAgo > 0.0) {
            setCol(DCOL_CHG5MIN, t.last - price5MinAgo, "{:+.2f}", true);
        } else {
            setNA(DCOL_CHG5MIN);
        }
    }

    if (t.prevClose > 0.0) setCol(DCOL_CHGPCT, t.changePct(), "{:+.2f}%", true);
    else setNA(DCOL_CHGPCT);
}

// Dividend data changes rarely and is now fetched once per position via a
// low-frequency one-shot request instead of the always-open L1 subscription
// (see queueDividendFetch/HandleDividendTick in ibkr.cpp). That means it can
// be genuinely unavailable for a while — most visibly over weekends, when
// there's no live market data connection for the one-shot fetch to ever
// complete. Falls back to the last value cached in the registry whenever the
// live fields are still at their empty/zero defaults. Any later live update
// simply overwrites these through the normal Diamonds_UpdateMarketCols() path.
static void Diamonds_ApplyCachedDividends(DiamondRowCache& cacheRow, int conId, const TradingAPI::L1Book& tickInfo) {
    bool haveLiveDividendData = (tickInfo.annualDividends != 0.0) || (tickInfo.dividendAmount != 0.0) || !tickInfo.dividendDate.empty();
    if (haveLiveDividendData) return;

    double cachedAnnual = 0.0, cachedAmount = 0.0, cachedDateSortable = 0.0;
    std::string cachedDate;
    if (!Settings_Dividends_Load(conId, cachedAnnual, cachedAmount, cachedDate, cachedDateSortable)) return;

    cacheRow.sortValues[DCOL_DIV_AMT] = cachedAmount;
    cacheRow.textCols[DCOL_DIV_AMT]   = std::format("{:.3f}", cachedAmount);

    cacheRow.sortValues[DCOL_ANNUAL_DIV] = cachedAnnual;
    cacheRow.textCols[DCOL_ANNUAL_DIV]   = std::format("{:.3f}", cachedAnnual);

    cacheRow.sortValues[DCOL_DIV_DATE] = cachedDateSortable;
    cacheRow.textCols[DCOL_DIV_DATE]   = cachedDate;

    // Yield needs a current price — fall back to the last known price (last,
    // else prevClose) so it still shows something with only stale price data.
    double priceForYield = tickInfo.last > 0.0 ? tickInfo.last : tickInfo.prevClose;
    if (priceForYield > 0.0 && cachedAnnual > 0.0) {
        double pct = cachedAnnual / priceForYield * 100.0;
        cacheRow.sortValues[DCOL_DIV_YIELD] = pct;
        cacheRow.textCols[DCOL_DIV_YIELD]   = std::format("{:.2f}%", pct);
    } else if (cachedAnnual == 0.0) {
        cacheRow.sortValues[DCOL_DIV_YIELD] = 0.0;
        cacheRow.textCols[DCOL_DIV_YIELD]   = "";
    } else {
        cacheRow.sortValues[DCOL_DIV_YIELD] = -999999.0;
        cacheRow.textCols[DCOL_DIV_YIELD]   = DIAMONDS_NO_DATA;
    }
}

// Loads the Alert Up / Alert Down strings for one symbol from the registry
// and writes them into DCOL_ALERTUP / DCOL_ALERTDOWN. Called for every row
// (portfolio position or alert-only pseudo row) on every repopulate, and
// whenever WM_ALERTS_CHANGED fires so an edit made in the Alerts popup shows
// up immediately without waiting for the next natural repopulate.
static void Diamonds_UpdateAlertCols(int conId, const std::string& symbol) {
    auto& row = diamondDataCache[conId];
    row.conId = conId;

    std::string upStr, downStr;
    Settings_Alerts_Load(symbol, upStr, downStr);

    row.textCols[DCOL_ALERTUP]   = upStr;
    row.textCols[DCOL_ALERTDOWN] = downStr;

    try { row.sortValues[DCOL_ALERTUP]   = upStr.empty()   ? -999999.0 : std::stod(upStr); }
    catch (...) { row.sortValues[DCOL_ALERTUP] = -999999.0; }
    try { row.sortValues[DCOL_ALERTDOWN] = downStr.empty() ? -999999.0 : std::stod(downStr); }
    catch (...) { row.sortValues[DCOL_ALERTDOWN] = -999999.0; }
}

// ── Repopulate ────────────────────────────────────────────────────────────────
static void Diamonds_Repopulate(HWND hWnd) {
    HWND hList = GetDlgItem(hWnd, ID_DIAMONDS_RESULTS_LIST);
    if (!hList) return;

    diamondDisplayOrder.clear(); // Clear the virtual list viewport

    std::vector<TradingAPI::PositionInfo> rows;
    std::unordered_set<std::string> portfolioSymbolsUpper;
    {
        std::lock_guard<std::mutex> lock(api().getPortfolioMutex());
        for (auto const& [conId, info] : api().getPortfolioMap()) {
            auto it = diamondsTabMap.find(info.conId);
            int  assignedTab = (it != diamondsTabMap.end()) ? it->second : DTAB_ALL;
            if ((diamondsCheckedTabs >> assignedTab) & 1) rows.push_back(info);

            std::string upperSym = info.symbol;
            std::transform(upperSym.begin(), upperSym.end(), upperSym.begin(), ::toupper);
            portfolioSymbolsUpper.insert(upperSym);
        }
    }

    // ── Alert-only symbols: have an Alert Up/Down set but aren't a current
    // position. Shown only under the Quarantine tab (forced there regardless
    // of diamondsTabMap, since there's no real position to assign a group
    // to) — see the NM_RCLICK handler below for the disabled "Move to *".
    if ((diamondsCheckedTabs >> DTAB_QUARENTINE) & 1) {
        for (auto const& [symbol, upDown] : Settings_Alerts_LoadAll()) {
            std::string upperSym = symbol;
            std::transform(upperSym.begin(), upperSym.end(), upperSym.begin(), ::toupper);
            if (portfolioSymbolsUpper.count(upperSym)) continue; // already a real position

            TradingAPI::PositionInfo pseudo;
            pseudo.conId  = Diamonds_GetOrCreateAlertOnlyId(symbol);
            pseudo.symbol = symbol;
            rows.push_back(pseudo);
        }
    }

    // Build/refresh the cache rows.
    // Rule: only write the fields we own here (identity, position, avgCost, market
    // data). Never touch DCOL_DAILYPNL / DCOL_UNREALIZED_PL / DCOL_UNREALIZED_PL_PCT
    // — those are owned by WM_PNL_SINGLE and must survive a repopulate so they
    // remain visible when the window is closed and reopened.
    for (const auto& pos : rows) {
        bool isRealPosition = (pos.conId > 0);

        // operator[] creates a default row only when the conId is new.
        // For existing rows it returns the current entry — PnL fields are preserved.
        auto& cacheRow = diamondDataCache[pos.conId];
        cacheRow.conId  = pos.conId;
        cacheRow.symbol = pos.symbol;

        cacheRow.textCols[DCOL_SYMBOL] = pos.symbol;

        cacheRow.sortValues[DCOL_POSITION] = pos.shares;
        cacheRow.textCols[DCOL_POSITION] = isRealPosition ? std::format("{:.4g}", pos.shares) : "--";

        cacheRow.sortValues[DCOL_AVGPRICE] = pos.avgCost;
        cacheRow.textCols[DCOL_AVGPRICE] = isRealPosition ? std::format("{:.2f}", pos.avgCost) : "--";

        Diamonds_UpdateAlertCols(pos.conId, pos.symbol);

        if (isRealPosition) {
            // Pre-fill market data if already cached — this also seeds the estimated
            // PnL columns for the first open (before WM_PNL_SINGLE arrives).
            TradingAPI::L1Book tickInfo;
            if (api().getMarketData(pos.conId, tickInfo)) {
                Diamonds_UpdateMarketCols(pos.conId, tickInfo);
            }

            Diamonds_ApplyCachedDividends(cacheRow, pos.conId, tickInfo);

            Diamonds_UpdatePnLCols(hWnd, pos.conId);
        }

        diamondDisplayOrder.push_back(pos.conId);
    }

    // VIRTUAL LIST MAGIC: Tell the UI exactly how many items exist. It will ask for text later.
    ListView_SetItemCountEx(hList, diamondDisplayOrder.size(), LVSICF_NOINVALIDATEALL | LVSICF_NOSCROLL);
    InvalidateRect(hList, NULL, FALSE);
    Diamonds_ApplySort(hList);

    std::string title = "Diamonds: " + std::to_string(rows.size());
    int activeTabs = 0;
    for (int i = 0; i < DIAMONDS_TAB_COUNT; ++i) {
        if (SendMessage(GetDlgItem(hWnd, ID_DIAMONDS_CHK_0 + i), BM_GETCHECK, 0, 0) == BST_CHECKED)
            title += std::string(activeTabs++ == 0 ? " " : " + ") + diamondTabNames[i];
    }
    title += " Positions";
    SetWindowTextA(hWnd, title.c_str());
}

// ── Window procedure ──────────────────────────────────────────────────────────

LRESULT CALLBACK WndProcDiamonds(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {

    case WM_CREATE: {
        // ZERO-FLICKER FIX: Prevent the parent from drawing over the list view
        //SetWindowLong(hWnd, GWL_STYLE, GetWindowLong(hWnd, GWL_STYLE) | WS_CLIPCHILDREN);

        // ZERO-FLICKER FIX: Remove the class background brush so Windows never
        // auto-erases behind our back during DefWindowProc(WM_SIZE) etc.
        //SetClassLongPtr(hWnd, GCLP_HBRBACKGROUND, (LONG_PTR)NULL);

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

        ListView_SetExtendedListViewStyle(hList, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

        LVCOLUMNA lvc = {};
        lvc.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_FMT;
        for (int i = 0; i < DCOL_COUNT; ++i) {
            lvc.cx      = diamondCols[i].width;
            lvc.pszText = (LPSTR)diamondCols[i].header;
            lvc.fmt     = diamondCols[i].fmt;
            ListView_InsertColumn(hList, i, &lvc);
        }

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
        diamondsSortCol = (int)Settings_Sort_Load(DIAMONDS_CLASS_NAME, "SortCol", DCOL_SYMBOL);
        diamondsSortAsc = Settings_Sort_Load(DIAMONDS_CLASS_NAME, "SortAsc", 1) != 0;
        if (diamondsSortCol < 0 || diamondsSortCol >= DCOL_COUNT) diamondsSortCol = DCOL_SYMBOL;

        // Restore checkbox bitmask (default 0x7 = all checked).
        diamondsCheckedTabs = (UINT)Settings_CheckedTabs_Load(0x7);
        diamondsCheckedTabs &= 0x7;  // clamp to valid 3-bit range
        for (int i = 0; i < DIAMONDS_TAB_COUNT; ++i) {
            bool checked = (diamondsCheckedTabs >> i) & 1;
            HWND tab = GetDlgItem(hWnd, ID_DIAMONDS_CHK_0 + i);
            SendMessage(tab, BM_SETCHECK, checked ? BST_CHECKED : BST_UNCHECKED, 0);
            SetCtrlColor(tab, checked ? (Settings_DarkMode() ? DM_TEXT : LM_TEXT) : COINS_CLR_GRAY);
        }
        Diamonds_UpdateDivColumnsVisibility(hWnd);

        api().addApiUpdateWindow(hWnd);
        Diamonds_Repopulate(hWnd);

        SetTimer(hWnd, TIMER_DIAMONDS_SORT, DIAMONDS_SORT_TIMER_MS, NULL);
        break;
    }

    case WM_GETMINMAXINFO: {
        bool showDiv   = (diamondsCheckedTabs & (1u << 1)) != 0;   // Dividends
        bool showWeeks = (diamondsCheckedTabs & (1u << 2)) != 0;   // Quarantine
        int extraWidth = 0;
        if (showDiv) {
            extraWidth += diamondCols[DCOL_DIV_YIELD].width   + diamondCols[DCOL_DIV_DATE].width +
                        diamondCols[DCOL_DIV_AMT].width      + diamondCols[DCOL_ANNUAL_DIV].width;
        }
        if (showWeeks) {
            extraWidth += diamondCols[DCOL_CHG13WEEK].width + diamondCols[DCOL_CHG26WEEK].width +
                        diamondCols[DCOL_CHG52WEEK].width + 
                        diamondCols[DCOL_AVGPRICE].width +
                        diamondCols[DCOL_ALERTUP].width + diamondCols[DCOL_ALERTDOWN].width;
        }
        if (extraWidth > 0) extraWidth += 10; // margin, same buffer the original single-group case used
        MINMAXINFO* mmi = (MINMAXINFO*)lParam;
        mmi->ptMinTrackSize.x = windowDiamondsWidth + extraWidth;
        mmi->ptMaxTrackSize.x = windowDiamondsWidth + extraWidth;
        return 0;
    }

    case WM_SIZE: {
        Diamonds_Layout(hWnd);
        break;
    }

    // ── Checkboxes show when active, hide when inactive ───────────────────────
    case WM_ACTIVATE:
        Diamonds_ShowCheckboxes(hWnd, LOWORD(wParam) != WA_INACTIVE);
        return 0;

    case WM_COMMAND: {
        WORD id = LOWORD(wParam);
        if (id >= ID_DIAMONDS_CHK_0 && id <= ID_DIAMONDS_CHK_2 && HIWORD(wParam) == BN_CLICKED) {
            // Rebuild bitmask from checkbox states.
            diamondsCheckedTabs = 0;
            for (int i = 0; i < DIAMONDS_TAB_COUNT; ++i) {
                HWND tab = GetDlgItem(hWnd, ID_DIAMONDS_CHK_0 + i);
                if (SendMessage(tab, BM_GETCHECK, 0, 0) == BST_CHECKED) {
                    diamondsCheckedTabs |= (1u << i);
                    SetCtrlColor(tab, Settings_DarkMode() ? DM_TEXT : LM_TEXT);
                } else {
                    SetCtrlColor(tab, COINS_CLR_GRAY);
                }
            }
            Settings_CheckedTabs_Save((int)diamondsCheckedTabs);
            Diamonds_UpdateDivColumnsVisibility(hWnd);
            Diamonds_Repopulate(hWnd);
            InvalidateRect(hWnd, NULL, TRUE);
            //UpdateWindow(hWnd);
        }
        break;
    }

    case WM_DIAMONDS_UPDATE: {
        Diamonds_Repopulate(hWnd);
        break;
    }

    case WM_ALERTS_CHANGED: {
        Diamonds_Repopulate(hWnd);
        break;
    }

    // ── Live market data update for one symbol ────────────────────────────────
    // Posted by Impl::tickPrice / tickSize / tickString / tickGeneric
    case WM_MARKET_L1: {
        int conId = (int)lParam;
        if (!conId) break;
        TradingAPI::L1Book info;
        if (api().getMarketData(conId, info)) {
            Diamonds_UpdateMarketCols(conId, info);
            // Defer to the throttled paint timer -- see note in Diamonds_UpdatePnLCols.
            diamondsDirty = true;
        }
        Diamonds_UpdatePnLCols(hWnd, conId);
        
        // ZERO-FLICKER FIX: Stop auto-sorting the entire grid on every single market tick!
        // This stops the rows from continuously jumping up and down (which the user perceived as flickering).
        // Sorting will now only happen when the user clicks a column header, or when repopulated.
        
        break;
    }
    // ── Live per-position PnL update (reqPnLSingle stream) ───────────────────
    // Posted by Impl::pnlSingle() on the API thread via PostMessage.
    //   wParam = conId (fast row-lookup key, no pointer deref needed)
    //   lParam = heap-allocated TradingAPI::PnlSinglePayload* — we own it, must delete.
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
            diamondDisplayOrder.clear();
            diamondDataCache.clear();
            ListView_SetItemCountEx(hList, 0, LVSICF_NOINVALIDATEALL);
            InvalidateRect(hList, NULL, FALSE);
        }
        break;
    }

    // ── Notification handling ─────────────────────────────────────────────────
    case WM_NOTIFY: {
        NMHDR* hdr = (NMHDR*)lParam;
        if (hdr->idFrom != ID_DIAMONDS_RESULTS_LIST) break;

        // ── Row selected: push symbol to TWS-linked windows/apps ─────────────
        if (hdr->code == LVN_ITEMCHANGED) {
            NMLISTVIEW* nmlv = (NMLISTVIEW*)lParam;
            if ((nmlv->uChanged & LVIF_STATE) && (nmlv->uNewState & LVIS_SELECTED) &&
                nmlv->iItem >= 0 && nmlv->iItem < (int)diamondDisplayOrder.size()) {
                int conId = diamondDisplayOrder[nmlv->iItem];
                api().updateDisplayGroup(conId);
            }
        }

        // --- VIRTUAL LIST TEXT REQUEST ---
        if (hdr->code == LVN_GETDISPINFO) {
            NMLVDISPINFO* pdi = (NMLVDISPINFO*)lParam;
            if (pdi->item.iItem < 0 || pdi->item.iItem >= (int)diamondDisplayOrder.size()) return 0;
            
            int conId = diamondDisplayOrder[pdi->item.iItem];
            const auto& row = diamondDataCache[conId];

            if (pdi->item.mask & LVIF_TEXT) {
                // VIRTUAL LIST FIX: Direct pointer assignment is zero-copy and avoids buffer truncation
                pdi->item.pszText = (LPSTR)row.textCols[pdi->item.iSubItem].c_str();
            }
            return 0;
        }
        if (hdr->code == LVN_COLUMNCLICK) {
            NMLISTVIEW* nmlv = (NMLISTVIEW*)lParam;
            int col = nmlv->iSubItem;
            if (col == diamondsSortCol) diamondsSortAsc = !diamondsSortAsc;
            else { diamondsSortCol = col; diamondsSortAsc = false; }
            Settings_Sort_Save(DIAMONDS_CLASS_NAME, "SortCol", diamondsSortCol);
            Settings_Sort_Save(DIAMONDS_CLASS_NAME, "SortAsc", diamondsSortAsc ? 1 : 0);
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
                    int conId = diamondDisplayOrder[row];
                    const std::string& sym = diamondDataCache[conId].textCols[DCOL_SYMBOL];
                    StartMarket(sym, conId);
                }
            }
        }

        if (hdr->code == NM_RCLICK) {
            if (!lockHotkeys) {
                LPNMITEMACTIVATE act = (LPNMITEMACTIVATE)lParam;
                int row = act->iItem;
                if (row >= 0) {
                    int conId = diamondDisplayOrder[row];
                    const std::string& sym = diamondDataCache[conId].textCols[DCOL_SYMBOL];

                    // Determine current group assignment for this item.
                    auto mapIt = diamondsTabMap.find(conId);
                    int currentGroup = (mapIt != diamondsTabMap.end()) ? mapIt->second : DTAB_ALL;

                    // Determine current color assignment for this item.
                    auto colorIt = diamondsSymbolColors.find(conId);
                    int currentColor = (colorIt != diamondsSymbolColors.end()) ? colorIt->second : DIAMONDS_COLOR_NONE;

                    // ── Build context menu ────────────────────────────────────────
                    // IDs 1-3:   group assignment
                    // IDs 200-206: color options (200+idx for colors, 206 = None)
                                        HMENU hMenu = CreatePopupMenu();
                    bool isRealPosition = (conId > 0);

                    // ── Quick placeholder orders ───────────────────────────────────
                    double quickLastPrice = 0.0;
                    {
                        TradingAPI::L1Book quickInfo;
                        if (isRealPosition && api().getMarketData(conId, quickInfo)) quickLastPrice = quickInfo.last;
                    }
                    std::string sellLabel = sym + (
                        (quickLastPrice > 0.0)
                            ? std::format(" SELL 1 @ {:.2f}", quickLastPrice * 2.0)
                            : " SELL 1 @ 2x Price"
                    );
                    AppendMenuA(hMenu, MF_STRING | (quickLastPrice <= 0.0 ? MF_GRAYED : 0), 301, sellLabel.c_str());
                    AppendMenuA(hMenu, MF_STRING | (isRealPosition ? 0 : MF_GRAYED), 300, (sym + " BUY 1 @ 1").c_str());

                    AppendMenuA(hMenu, MF_SEPARATOR, 0, NULL);
                    AppendMenuA(hMenu, MF_STRING, 302, "Edit Alerts");
                    AppendMenuA(hMenu, MF_SEPARATOR, 0, NULL);

                    AppendMenuA(hMenu, MF_STRING | ((currentGroup == DTAB_ALL        || !isRealPosition) ? MF_GRAYED : 0), 1, "Move to Growth");
                    AppendMenuA(hMenu, MF_STRING | ((currentGroup == DTAB_GROWTH     || !isRealPosition) ? MF_GRAYED : 0), 2, "Move to Dividends");
                    AppendMenuA(hMenu, MF_STRING | ((currentGroup == DTAB_QUARENTINE || !isRealPosition) ? MF_GRAYED : 0), 3, "Move to Quarantine");

                    // ── Color submenu ─────────────────────────────────────────────
                    AppendMenuA(hMenu, MF_SEPARATOR, 0, NULL);
                    
                    // "None" option — grayed when no color is currently assigned.
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

                    if (cmd >= 1 && cmd <= 3 && isRealPosition) {
                        // Group assignment.
                        int targetTab = cmd - 1;
                        if (targetTab == DTAB_ALL)
                            diamondsTabMap.erase(conId);
                        else
                            diamondsTabMap[conId] = targetTab;
                        Diamonds_SaveTabMap();
                        Diamonds_Repopulate(hWnd);
                        InvalidateRect(hWnd, NULL, TRUE);
                        Diamonds_CleanupStaleTabAssignments();
                        Diamonds_CleanupStaleDividends();
                    } else if (cmd >= 200 && cmd <= 200 + DIAMONDS_COLOR_COUNT) {
                        // Color assignment.
                        int pickedIdx = cmd - 200;
                        if (pickedIdx == DIAMONDS_COLOR_COUNT) {
                            // "None" — remove override.
                            diamondsSymbolColors.erase(conId);
                        } else {
                            diamondsSymbolColors[conId] = pickedIdx;
                        }
                        Diamonds_SaveSymbolColors();
                        // Invalidate just this row so the color appears immediately.
                        HWND hList = GetDlgItem(hWnd, ID_DIAMONDS_RESULTS_LIST);
                        ListView_RedrawItems(hList, row, row);
                        UpdateWindow(hList);
                        Diamonds_CleanupStaleTabAssignments();
                        Diamonds_CleanupStaleDividends();
                    } else if (cmd == 300 && isRealPosition) {
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
                    } else if (cmd == 301 && isRealPosition) {
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
                        StartAlertsEditor(sym);
                    }
                }
            }
        }

        if (hdr->code == NM_CUSTOMDRAW) {
            NMLVCUSTOMDRAW* cd = (NMLVCUSTOMDRAW*)lParam;
            bool dark = Settings_DarkMode();

            switch (cd->nmcd.dwDrawStage) {
                case CDDS_PREPAINT:
                    return CDRF_NOTIFYITEMDRAW;

                case CDDS_ITEMPREPAINT:
                    cd->nmcd.uItemState &= ~CDIS_SELECTED;
                    if (dark) {
                        cd->clrTextBk = (cd->nmcd.dwItemSpec % 2 == 0) ? DM_BG : DM_BG2;
                        cd->clrText   = DM_TEXT;
                    } else {
                        cd->clrTextBk = (cd->nmcd.dwItemSpec % 2 == 0) ? GetSysColor(COLOR_WINDOW) : RGB(245, 245, 245);
                        cd->clrText   = LM_TEXT;
                    }
                    return CDRF_NOTIFYSUBITEMDRAW;

                case CDDS_ITEMPREPAINT | CDDS_SUBITEM: {
                    // ── Symbol column: apply per-symbol color override ────────
                    if (cd->iSubItem == DCOL_SYMBOL) {
                        SelectObject(cd->nmcd.hdc, hFont16ptbold.get());
                        int rowIndex = (int)cd->nmcd.dwItemSpec;
                        int conId = diamondDisplayOrder[rowIndex];
                        
                        auto cit = diamondsSymbolColors.find(conId);
                        if (cit != diamondsSymbolColors.end() &&
                            cit->second >= 0 && cit->second < DIAMONDS_COLOR_COUNT) {
                            cd->clrText = diamondColorPalette[cit->second].rgb;
                            if (dark) cd->clrTextBk = (cd->nmcd.dwItemSpec % 2 == 0) ? DM_BG : DM_BG2;
                        }
                        return CDRF_NEWFONT;
                    }
                    // Only colour P&L / change columns — and only when the
                    // cell holds a real numeric value (not the "--" sentinel).
                    if (cd->iSubItem == DCOL_CHGPCT || cd->iSubItem == DCOL_DAILYPNL || cd->iSubItem == DCOL_UNREALIZED_PL || cd->iSubItem == DCOL_UNREALIZED_PL_PCT || cd->iSubItem == DCOL_POSITION || cd->iSubItem == DCOL_CHG5MIN || cd->iSubItem == DCOL_CHG13WEEK || cd->iSubItem == DCOL_CHG26WEEK || cd->iSubItem == DCOL_CHG52WEEK) {
                        int rowIndex = (int)cd->nmcd.dwItemSpec;
                        int conId = diamondDisplayOrder[rowIndex];
                        const std::string& textVal = diamondDataCache[conId].textCols[cd->iSubItem];
                        // Guard: skip colouring the "--" sentinel — atof("--") == 0
                        // which would leave the cell uncoloured anyway, but being
                        // explicit avoids any locale-specific atof surprises.
                        if (!textVal.empty()) {
                            double val = atof(textVal.c_str());
                            if      (val > 0.0) cd->clrText = COINS_CLR_GREEN;
                            else if (val < 0.0) cd->clrText = COINS_CLR_RED;
                            else cd->clrText = dark ? DM_TEXT : LM_TEXT;
                        }
                        if (dark) cd->clrTextBk = (cd->nmcd.dwItemSpec % 2 == 0) ? DM_BG : DM_BG2;
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
                    if (cd->iSubItem == DCOL_ASKSIZE || cd->iSubItem == DCOL_BIDSIZE) {
                        int rowIndex = (int)cd->nmcd.dwItemSpec;
                        int conId = diamondDisplayOrder[rowIndex];
                        bool halted = diamondDataCache[conId].halted;
                        cd->clrText = halted ? COINS_CLR_GRAY : COINS_CLR_BLUE;
                        if (dark) cd->clrTextBk = (cd->nmcd.dwItemSpec % 2 == 0) ? DM_BG : DM_BG2;
                        SelectObject(cd->nmcd.hdc, hFont14pt.get());
                        return CDRF_NEWFONT;
                    }
                    if (cd->iSubItem == DCOL_VWAP) {
                        int rowIndex = (int)cd->nmcd.dwItemSpec;
                        int conId = diamondDisplayOrder[rowIndex];
                        const auto& cacheRow = diamondDataCache[conId];
                        if (cacheRow.textCols[DCOL_VWAP] != DIAMONDS_NO_DATA && !cacheRow.textCols[DCOL_VWAP].empty()) {
                            double diff = cacheRow.sortValues[DCOL_VWAP];
                            if      (diff > 0.0) cd->clrText = COINS_CLR_GREEN;
                            else if (diff < 0.0) cd->clrText = COINS_CLR_RED;
                            else cd->clrText = dark ? DM_TEXT : LM_TEXT;
                        }
                        if (dark) cd->clrTextBk = (cd->nmcd.dwItemSpec % 2 == 0) ? DM_BG : DM_BG2;
                        SelectObject(cd->nmcd.hdc, hFont14pt.get());
                        return CDRF_NEWFONT;
                    }
                    if (cd->iSubItem == DCOL_LAST) {
                        int rowIndex = (int)cd->nmcd.dwItemSpec;
                        int conId = diamondDisplayOrder[rowIndex];
                        const auto& cacheRow = diamondDataCache[conId];
                        double last = cacheRow.sortValues[DCOL_LAST];
                        double high = cacheRow.dayHigh;
                        double low = cacheRow.dayLow;
                        double prevClose = cacheRow.prevClose;

                        if (last > 0.0 && high > 0.0 && low > 0.0 && high > low) {
                            double pct = (last - low) / (high - low) * 100.0; // 0% at low, 100% at high
                            if (pct <= 25.0) cd->clrText = COINS_CLR_RED;
                            else if (pct >= 75.0) cd->clrText = COINS_CLR_GREEN;
                            else cd->clrText = dark ? DM_TEXT : LM_TEXT;
                        } else if (last > 0.0 && prevClose > 0.0) {
                            if (last > prevClose) cd->clrText = COINS_CLR_GREEN;
                            else if (last < prevClose) cd->clrText = COINS_CLR_RED;
                            else cd->clrText = dark ? DM_TEXT : LM_TEXT;
                        } else {
                            cd->clrText = dark ? DM_TEXT : LM_TEXT;
                        }
                        if (dark) cd->clrTextBk = (cd->nmcd.dwItemSpec % 2 == 0) ? DM_BG : DM_BG2;
                        SelectObject(cd->nmcd.hdc, hFont16pt.get());
                        return CDRF_NEWFONT;
                    }
                    if (cd->iSubItem == DCOL_ASK || cd->iSubItem == DCOL_BID) {
                        int rowIndex = (int)cd->nmcd.dwItemSpec;
                        int conId = diamondDisplayOrder[rowIndex];
                        const std::string& textValB = diamondDataCache[conId].textCols[DCOL_BIDSIZE];
                        const std::string& textValA = diamondDataCache[conId].textCols[DCOL_ASKSIZE];
                        if (!textValB.empty() && !textValA.empty()) {
                            double valB = atof(textValB.c_str());
                            double valA = atof(textValA.c_str());
                            if (valA > valB) cd->clrText = COINS_CLR_RED;
                            else if (valA < valB) cd->clrText = COINS_CLR_GREEN;
                            else cd->clrText = dark ? DM_TEXT : LM_TEXT;
                        }
                        if (dark) cd->clrTextBk = (cd->nmcd.dwItemSpec % 2 == 0) ? DM_BG : DM_BG2;
                        SelectObject(cd->nmcd.hdc, hFont16pt.get());
                        return CDRF_NEWFONT;
                    }
                    if (cd->iSubItem == DCOL_PCT_NETLIQ) {
                        cd->clrText = COINS_CLR_GRAY;
                        if (dark) cd->clrTextBk = (cd->nmcd.dwItemSpec % 2 == 0) ? DM_BG : DM_BG2;
                        SelectObject(cd->nmcd.hdc, hFont14pt.get());
                        return CDRF_NEWFONT;
                    }
                    if (cd->iSubItem == DCOL_AVGPRICE || cd->iSubItem == DCOL_MKTVAL || cd->iSubItem == DCOL_ALERTUP || cd->iSubItem == DCOL_ALERTDOWN) {
                        if (dark) {
                            cd->clrTextBk = (cd->nmcd.dwItemSpec % 2 == 0) ? DM_BG : DM_BG2;
                            cd->clrText   = DM_TEXT;
                        } else {
                            cd->clrTextBk = (cd->nmcd.dwItemSpec % 2 == 0) ? GetSysColor(COLOR_WINDOW) : RGB(245, 245, 245);
                            cd->clrText   = LM_TEXT;
                        }
                        SelectObject(cd->nmcd.hdc, hFont14pt.get());
                        return CDRF_NEWFONT;
                    }
                    if (cd->iSubItem == DCOL_DIV_YIELD || cd->iSubItem == DCOL_DIV_DATE  ||  cd->iSubItem == DCOL_DIV_AMT || cd->iSubItem == DCOL_ANNUAL_DIV) {
                        cd->clrText = COINS_CLR_PURPLE;
                        if (dark) cd->clrTextBk = (cd->nmcd.dwItemSpec % 2 == 0) ? DM_BG : DM_BG2;
                        SelectObject(cd->nmcd.hdc, hFont14pt.get());
                        return CDRF_NEWFONT;
                    }
                    return CDRF_DODEFAULT;
                }

                case CDDS_ITEMPOSTPAINT | CDDS_SUBITEM: {
                    if (cd->iSubItem != DCOL_POSITION) return CDRF_DODEFAULT;

                    int rowIndex = (int)cd->nmcd.dwItemSpec;
                    if (rowIndex < 0 || rowIndex >= diamondDisplayOrder.size()) return CDRF_DODEFAULT;

                    int conId = diamondDisplayOrder[rowIndex];
                    auto sit  = diamondsSparklines.find(conId);
                    if (sit == diamondsSparklines.end() || !sit->second.HasData())
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
        if (wParam == TIMER_DIAMONDS_SORT) {
            HWND hList = GetDlgItem(hWnd, ID_DIAMONDS_RESULTS_LIST);
            Diamonds_ApplySort(hList);
            InvalidateRect(hList, NULL, FALSE);
        }
        if (wParam == TIMER_DIAMONDS_PAINT) {
            if (diamondsDirty) {
                HWND hList = GetDlgItem(hWnd, ID_DIAMONDS_RESULTS_LIST);
                if (hList && !diamondDisplayOrder.empty()) {
                    // Get the range of items currently visible to the user
                    int top = ListView_GetTopIndex(hList);
                    int count = ListView_GetCountPerPage(hList);
                    int bottom = top + count;
                    
                    // Clamp to actual size
                    if (bottom >= (int)diamondDisplayOrder.size()) 
                        bottom = (int)diamondDisplayOrder.size() - 1;

                    // Only redraw the specific rows that have changed on screen
                    ListView_RedrawItems(hList, top, bottom);
                    UpdateWindow(hList); // Force immediate flush
                }
                diamondsDirty = false;
            }
        }
        break;  // was missing — without this, every timer tick fell through into WM_DESTROY,
                // killing timers, clearing the cache, and calling removeApiUpdateWindow.
    }

    case WM_DESTROY:
        KillTimer(hWnd, TIMER_DIAMONDS_SORT);
        KillTimer(hWnd, TIMER_DIAMONDS_PAINT);
        api().removeApiUpdateWindow(hWnd);
        diamondDataCache.clear();
        diamondsSparklines.clear();
        if (diamondsRowHeightImageList) {
            ImageList_Destroy(diamondsRowHeightImageList);
            diamondsRowHeightImageList = NULL;
        }
        break;
    }

    return HandleCommonMessages(hWnd, message, wParam, lParam);
}
#pragma once
// "Proxima Nova", Verdana, Arial, sans-serif
int windowDiamondsWidth = 1446;
void StartDiamonds() { StartGenericWindow(DIAMONDS_CLASS_NAME, "Diamonds", L"TWSAPIClientTradingFloor.Diamonds", windowDiamondsWidth, 420); }

#define ID_DIAMONDS_RESULTS_LIST 7001
#define ID_DIAMONDS_CHK_0        7002   // "Growth"
#define ID_DIAMONDS_CHK_1        7003   // "High-Yield Dividends"
#define ID_DIAMONDS_CHK_2        7004   // "Quarantine"
#define DIAMONDS_CHK_STRIP_H     32     // height of the checkbox bar at the bottom

// ── Filter / tab constants ────────────────────────────────────────────────────
#define DTAB_ALL              0
#define DTAB_GROWTH           1
#define DTAB_QUARENTINE       2
#define DIAMONDS_TAB_COUNT    3

static const char* g_DiamondTabNames[DIAMONDS_TAB_COUNT] = { "Growth", "High-Yield Dividends", "Quarantine" };

// Bitmask: bit N set means group N is currently visible.  Default = all visible.
static UINT g_DiamondsCheckedTabs = 0x7;

// Maps conId → assigned group (DTAB_ALL = untagged = shown when bit 0 is set).
static std::map<int,int> g_DiamondsTabMap;

// ── Symbol color palette ──────────────────────────────────────────────────────
// Index 0-5 = named colors.  No entry in the map (or index -1) = inherit theme.
#define DIAMONDS_COLOR_COUNT  6
#define DIAMONDS_COLOR_NONE  -1   // sentinel: remove override, inherit by theme

struct DiamondsColorDef { COLORREF rgb; const char* label; };
static const DiamondsColorDef g_DiamondColorPalette[DIAMONDS_COLOR_COUNT] = {
    { RGB(159,  27,  27), "Set Color: Red"    },
    { RGB( 18, 220,  18), "Set Color: Green"  },
    { RGB(  0, 167, 255), "Set Color: Blue"   },
    { RGB(167,  84, 212), "Set Color: Purple" },
    { RGB(255, 215,   0), "Set Color: Gold"   },
    { RGB(163, 104,  14), "Set Color: Brown"  },
};

// Maps conId → color index (0..DIAMONDS_COLOR_COUNT-1), or not present = inherit.
static std::map<int,int> g_DiamondsSymbolColors;

// Stash the hList pointer so the static sort callback can reach it.
static HWND g_DiamondsListForSort = NULL;

static bool g_DiamondsChkVisible = false;

// ── Deferred sort (prevents flicker on every tick) ────────────────────────────
#define DIAMONDS_SORT_TIMER_ID  7010
#define DIAMONDS_SORT_TIMER_MS   500   // re-sort at most twice per second
static bool g_DiamondsSortPending = false;

static ListViewZoomData DiamondsZoomData = { NULL, NULL, 17, "Zoom_Diamonds" };

// ── Column indices (keep in sync with diamondCols[]) ─────────────────────────
enum DiamondColIdx {
    DCOL_SYMBOL = 0,
    DCOL_POSITION,
    DCOL_AVGPRICE,
    DCOL_ASKSIZE,
    DCOL_ASK,
    DCOL_LAST,
    DCOL_BID,
    DCOL_BIDSIZE,
    DCOL_DAILYPNL,
    DCOL_CHGPCT,
    //DCOL_CLOSE,
    //DCOL_OPEN,
    DCOL_UNREALIZED_PL_PCT,
    DCOL_UNREALIZED_PL,
    DCOL_MKTVAL,
    DCOL_PCT_NETLIQ,
    DCOL_DIV_YIELD,
    DCOL_DIV_DATE,
    DCOL_DIV_AMT,
    DCOL_ANNUAL_DIV,
    DCOL_COUNT
};

// ── Sort state ────────────────────────────────────────────────────────────────
static int  g_DiamondsSortCol = DCOL_SYMBOL;
static bool g_DiamondsSortAsc = true;

// Keyed by conId. Populated / updated in Diamonds_UpdateMarketCols.
static std::map<int, MiniSparkline> g_DiamondsSparklines;

// ── Unified Virtual List Cache (Replaces g_DiamondsPnlCache) ─────────────────
struct DiamondRowCache {
    int conId = 0;
    std::string symbol;
    double sortValues[DCOL_COUNT] = {0.0};  // Raw doubles for fast sorting
    std::string textCols[DCOL_COUNT];       // Pre-formatted strings for instant UI painting
};

// Data storage: Fast O(1) lookup by conId for live data streams
static std::map<int, DiamondRowCache> g_DiamondDataCache;

// UI Viewport: Holds conIds in sorted order. The ListView only knows about this vector's size.
static std::vector<int> g_DiamondDisplayOrder;

// Paint Limiter
static bool g_DiamondsDirty = false;
#define DIAMONDS_PAINT_TIMER_ID 7011
#define DIAMONDS_PAINT_TIMER_MS  60     // ~16 FPS (Butter smooth, zero flicker)

// ── Column definitions ────────────────────────────────────────────────────────

struct DiamondCol { const char* header; int width; int fmt; };
static const DiamondCol diamondCols[] = {
    { "Symbol",            90, LVCFMT_LEFT  },
    { "Position",         110, LVCFMT_RIGHT },
    { "AvgPx",            100, LVCFMT_RIGHT },
    { "AskSz",             80, LVCFMT_RIGHT },
    { "Ask",              100, LVCFMT_RIGHT },
    { "Last",             100, LVCFMT_RIGHT },
    { "Bid",              100, LVCFMT_RIGHT },
    { "BidSz",             80, LVCFMT_RIGHT },
    { "Daily",            100, LVCFMT_RIGHT },  // {"fix_tag":7681,"name":"Price/EMA(20)","description":"Price to Exponential moving average (N = 20) ratio - 1, displayed in percents","groups":["G40"],"id":"PRICE_VS_EMA20"}
    { "Change %",         115, LVCFMT_RIGHT },  // {"fix_tag":7679,"name":"Price/EMA(100)","description":"Price to Exponential moving average (N = 100) ratio - 1, displayed in percents","groups":["G40"],"id":"PRICE_VS_EMA100"}
    //{ "Close",             85, LVCFMT_RIGHT },  // {"fix_tag":7678,"name":"Price/EMA(200)","description":"Price to Exponential moving average (N = 200) ratio - 1, displayed in percents","groups":["G40"],"id":"PRICE_VS_EMA200"}
    //{ "Open",              80, LVCFMT_RIGHT },  // {"fix_tag":7743,"name":"52 Week Change %","description":"This is the percentage change in the company's stock price over the last fifty two weeks.","groups":["G5"],"id":"52WK_PRICE_PCT_CHANGE"}
    { "Unrealized %",     140, LVCFMT_RIGHT },
    { "Unrealized",       115, LVCFMT_RIGHT },
    { "Mkt Value",        105, LVCFMT_RIGHT },
    { "Net %",             90, LVCFMT_RIGHT },
    { "Yield %",           90, LVCFMT_RIGHT },
    { "Date",             135, LVCFMT_RIGHT },
    { "Amount",            85, LVCFMT_RIGHT },
    { "Annual",            85, LVCFMT_RIGHT },
    // {"fix_tag":7290,"name":"P/E excluding extraordinary items","description":"This ratio is calculated by dividing the current Price by the sum of the Diluted Earnings Per Share from continuing operations BEFORE Extraordinary Items and Accounting Changes over the last four interim periods.","groups":["G15"],"id":"PE"}
    // {"fix_tag":7281,"name":"Category","description":"Displays a more detailed level of description within the industry under which the underlying company can be categorized.","groups":["G-3"],"id":"CATEGORY"}
    // {"fix_tag":7289,"name":"Market capitalization","description":"This value is calculated by multiplying the current Price by the current number of Shares Outstanding.","groups":["G15"],"id":"MKT_CAP"}
};
static_assert((int)(sizeof(diamondCols) / sizeof(diamondCols[0])) == DCOL_COUNT,
              "diamondCols count must match DiamondColIdx::DCOL_COUNT");

// ── Dividend column visibility ────────────────────────────────────────────────
// The last 4 columns (Yield/Date/Amount/Annual) are only shown when the
// 2nd CheckedTab checkbox ("High-Yield Dividends", ID_DIAMONDS_CHK_1, bit 1
// of g_DiamondsCheckedTabs) is checked. Hidden by collapsing the column width
// to 0; restored to its defined width from diamondCols[] when shown again.
static void Diamonds_UpdateDivColumnsVisibility(HWND hWnd) {
    HWND hList = GetDlgItem(hWnd, ID_DIAMONDS_RESULTS_LIST);
    if (!hList) return;
    bool show = (g_DiamondsCheckedTabs & (1u << 1)) != 0;
    for (int i = DCOL_DIV_YIELD; i <= DCOL_ANNUAL_DIV; ++i) {
        ListView_SetColumnWidth(hList, i, show ? diamondCols[i].width : 0);
    }
    RECT windowRect; 
    GetWindowRect(hWnd, &windowRect);
    if (show) {
        MoveWindow(hWnd, windowRect.left, windowRect.top, windowDiamondsWidth + 16 + diamondCols[DCOL_DIV_YIELD].width + diamondCols[DCOL_DIV_DATE].width + diamondCols[DCOL_DIV_AMT].width + diamondCols[DCOL_ANNUAL_DIV].width,  windowRect.bottom - windowRect.top, TRUE);
    } else {
        MoveWindow(hWnd, windowRect.left, windowRect.top, windowDiamondsWidth,  windowRect.bottom - windowRect.top, TRUE);
    }
}

static HIMAGELIST g_DiamondsRowHeightImageList = NULL;

static void Diamonds_SetRowHeight(HWND hList, int rowHeight) {
    if (g_DiamondsRowHeightImageList) {
        ImageList_Destroy(g_DiamondsRowHeightImageList);
        g_DiamondsRowHeightImageList = NULL;
    }
    // Width can stay tiny (1px) since LVS_REPORT never shows the icon glyph
    // area when there's no LVCFMT_IMAGE column, but height controls row height.
    g_DiamondsRowHeightImageList = ImageList_Create(1, rowHeight, ILC_COLOR32 | ILC_MASK, 1, 1);
    if (!g_DiamondsRowHeightImageList) return;

    // Add one fully-transparent 1x1 bitmap so the image list is non-empty.
    HBITMAP hbmImage = CreateBitmap(1, rowHeight, 1, 1, NULL);
    HBITMAP hbmMask  = CreateBitmap(1, rowHeight, 1, 1, NULL);
    ImageList_Add(g_DiamondsRowHeightImageList, hbmImage, hbmMask);
    DeleteObject(hbmImage);
    DeleteObject(hbmMask);

    ListView_SetImageList(hList, g_DiamondsRowHeightImageList, LVSIL_SMALL);
}

// ── Registry persistence for tab assignments ──────────────────────────────────

// Saves g_DiamondsTabMap to the registry as two space-separated conId lists.
static void Diamonds_SaveTabMap() {
    std::string growthList, quarentineList;
    for (auto& [conId, tab] : g_DiamondsTabMap) {
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

// Loads g_DiamondsTabMap from the registry.
static void Diamonds_LoadTabMap() {
    g_DiamondsTabMap.clear();
    auto parseIds = [](const std::string& s, int tab) {
        size_t start = 0;
        while (start < s.size()) {
            size_t end = s.find(' ', start);
            if (end == std::string::npos) end = s.size();
            if (end > start) {
                try { g_DiamondsTabMap[std::stoi(s.substr(start, end - start))] = tab; }
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
    for (auto& [conId, idx] : g_DiamondsSymbolColors) {
        if (!s.empty()) s += ' ';
        s += std::to_string(conId) + ':' + std::to_string(idx);
    }
    Settings_SymbolColors_Save(s);
}

static void Diamonds_LoadSymbolColors() {
    g_DiamondsSymbolColors.clear();
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
                    g_DiamondsSymbolColors[conId] = idx;
            } catch (...) {}
        }
        pos = end + 1;
    }
}

// ── Layout ────────────────────────────────────────────────────────────────────

static void Diamonds_Layout(HWND hWnd) {
    HWND hList = GetDlgItem(hWnd, ID_DIAMONDS_RESULTS_LIST);
    if (!hList) return;
    RECT rc; GetClientRect(hWnd, &rc);
    int listH = g_DiamondsChkVisible ? rc.bottom - DIAMONDS_CHK_STRIP_H : rc.bottom;
    MoveWindow(hList, 0, 0, rc.right, listH, TRUE);

    if (!g_DiamondsChkVisible) return;

    // Space the three checkboxes evenly across the bottom strip.
    static const int chkW[DIAMONDS_TAB_COUNT] = { 90, 175, 115 };
    int totalW = 0;
    for (int w : chkW) totalW += w;
    int startX = (rc.right - totalW) / 2;
    int y = listH + (DIAMONDS_CHK_STRIP_H - 20) / 2;
    int x = startX;
    for (int i = 0; i < DIAMONDS_TAB_COUNT; ++i) {
        SetWindowPos(GetDlgItem(hWnd, ID_DIAMONDS_CHK_0 + i), NULL,
                     x, y, chkW[i], 20, SWP_NOZORDER | SWP_NOACTIVATE);
        x += chkW[i];
    }
}

static void Diamonds_ShowCheckboxes(HWND hWnd, bool show) {
    if (g_DiamondsChkVisible == show) return;
    g_DiamondsChkVisible = show;
    int sw = show ? SW_SHOW : SW_HIDE;
    for (int i = 0; i < DIAMONDS_TAB_COUNT; ++i)
        ShowWindow(GetDlgItem(hWnd, ID_DIAMONDS_CHK_0 + i), sw);
    Diamonds_Layout(hWnd);
}

// ── Sort helpers ──────────────────────────────────────────────────────────────

static void Diamonds_SetSortArrow(HWND hList, int col, bool asc) {
    HWND hHdr = ListView_GetHeader(hList);
    int n = Header_GetItemCount(hHdr);
    for (int i = 0; i < n; ++i) {
        HDITEM hdi = {}; hdi.mask = HDI_FORMAT;
        Header_GetItem(hHdr, i, &hdi);
        hdi.fmt &= ~(HDF_SORTUP | HDF_SORTDOWN);
        if (i == col) hdi.fmt |= asc ? HDF_SORTUP : HDF_SORTDOWN;
        Header_SetItem(hHdr, i, &hdi);
    }
}

// Numeric columns — strip leading +/% suffix chars before atof.
static bool Diamonds_ColIsNumeric(int col) {
    return col != DCOL_SYMBOL && col != DCOL_DIV_DATE;
}

// ── Virtual Sort ──────────────────────────────────────────────────────────────

static void Diamonds_ApplySort(HWND hList) {
    if (g_DiamondDisplayOrder.empty()) return;

    std::sort(g_DiamondDisplayOrder.begin(), g_DiamondDisplayOrder.end(), [](int idA, int idB) {
        const auto& a = g_DiamondDataCache[idA];
        const auto& b = g_DiamondDataCache[idB];

        if (g_DiamondsSortCol == DCOL_SYMBOL || g_DiamondsSortCol == DCOL_DIV_DATE) {
            int cmp = _stricmp(a.textCols[g_DiamondsSortCol].c_str(), b.textCols[g_DiamondsSortCol].c_str());
            return g_DiamondsSortAsc ? (cmp < 0) : (cmp > 0);
        } else {
            double v1 = a.sortValues[g_DiamondsSortCol];
            double v2 = b.sortValues[g_DiamondsSortCol];
            if (v1 == v2) return false;
            return g_DiamondsSortAsc ? (v1 < v2) : (v1 > v2);
        }
    });

    Diamonds_SetSortArrow(hList, g_DiamondsSortCol, g_DiamondsSortAsc);
    // ZERO-FLICKER FIX: Delegate to the paint timer instead of invalidating instantly
    g_DiamondsDirty = true;
}
// ── Helpers ───────────────────────────────────────────────────────────────────

// Sentinel string displayed whenever a value cannot be computed (e.g. market closed, last == 0).
static const char* DIAMONDS_NO_DATA = "--";


static void Diamonds_UpdatePnLCols(HWND hWnd, int conId) {
    // Grab our new unified cache row
    auto& row = g_DiamondDataCache[conId];
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
        if (pnlSingle.has_daily) {
            row.sortValues[DCOL_DAILYPNL] = pnlSingle.dailyPnL;
            row.textCols[DCOL_DAILYPNL] = std::format("{:+.2f}", pnlSingle.dailyPnL);
        }
        if (pnlSingle.has_unrealized) {
            row.sortValues[DCOL_UNREALIZED_PL] = pnlSingle.unrealizedPnL;
            row.textCols[DCOL_UNREALIZED_PL] = std::format("{:+.2f}", pnlSingle.unrealizedPnL);

            // Recompute the % column
            double last = row.sortValues[DCOL_LAST];
            if (avgCost > 0.0 && last > 0.0) {
                double pct = (last - avgCost) / avgCost * 100.0;
                row.sortValues[DCOL_UNREALIZED_PL_PCT] = pct;
                row.textCols[DCOL_UNREALIZED_PL_PCT] = std::format("{:+.2f}%", pct);
            } else {
                row.sortValues[DCOL_UNREALIZED_PL_PCT] = -999999.0;
                row.textCols[DCOL_UNREALIZED_PL_PCT] = "--";
            }
        }

        // ZERO-FLICKER FIX: Invalidate ONLY the row that changed, perfectly smoothly
        auto it = std::find(g_DiamondDisplayOrder.begin(), g_DiamondDisplayOrder.end(), conId);
        if (it != g_DiamondDisplayOrder.end()) {
            int row = (int)std::distance(g_DiamondDisplayOrder.begin(), it);
            HWND hList = GetDlgItem(hWnd, ID_DIAMONDS_RESULTS_LIST);
            if (hList) ListView_RedrawItems(hList, row, row);
        }
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
    // tick is never lost; if the row isn't in g_DiamondDisplayOrder yet it
    // simply becomes visible on the next repopulate/sort instead of vanishing.
    auto& row = g_DiamondDataCache[conId];
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

    auto setNA = [&](int col) {
        row.sortValues[col] = -999999.0; // Pushes NA to bottom on sorts
        row.textCols[col] = DIAMONDS_NO_DATA;
    };

    setCol(DCOL_ASKSIZE, t.askSize, "{:.0f}");
    setCol(DCOL_ASK,     t.ask,     "{:.2f}");
    setCol(DCOL_BID,     t.bid,     "{:.2f}");
    setCol(DCOL_BIDSIZE, t.bidSize, "{:.0f}");
    setCol(DCOL_DIV_AMT, t.dividendAmount,  "{:.3f}");
    setCol(DCOL_ANNUAL_DIV, t.annualDividends, "{:.3f}");
    
    row.textCols[DCOL_DIV_DATE] = t.dividendDate;
    row.sortValues[DCOL_DIV_DATE] = 0; // Handled by string compare in sort

    if (t.last > 0.0 && t.annualDividends > 0.0) setCol(DCOL_DIV_YIELD, t.dividendYield(), "{:.2f}%");
    else if (t.annualDividends == 0.0) setCol(DCOL_DIV_YIELD, 0.0, "");
    else setNA(DCOL_DIV_YIELD);

    double displayLast = (t.last > 0.0) ? t.last : t.prevClose;
    if (displayLast <= 0.0) {
        setNA(DCOL_LAST); setNA(DCOL_DAILYPNL); setNA(DCOL_CHGPCT); 
        setNA(DCOL_UNREALIZED_PL); setNA(DCOL_UNREALIZED_PL_PCT); 
        setNA(DCOL_MKTVAL); setNA(DCOL_PCT_NETLIQ);
        return;
    }

    setCol(DCOL_LAST, displayLast, "{:.2f}", true);
    g_DiamondsSparklines[conId].AddPrice(displayLast);

    double shares = row.sortValues[DCOL_POSITION];
    double avgCost = row.sortValues[DCOL_AVGPRICE];
    
    double netLiq = 0.0;
    auto summary = api().getAccountSummary();
    if (summary.count("NetLiquidation")) {
        try { netLiq = std::stod(summary["NetLiquidation"]); } catch (...) {}
    }

    double mktVal = shares * t.last;
    double pctNetLiq = (netLiq > 0.0 && mktVal != 0.0) ? (mktVal / netLiq * 100.0) : 0.0;

    if (t.prevClose > 0.0) setCol(DCOL_CHGPCT, t.changePct(), "{:+.2f}%", true);
    else setNA(DCOL_CHGPCT);

    setCol(DCOL_MKTVAL, mktVal, "{:.2f}", true);
    setCol(DCOL_PCT_NETLIQ, pctNetLiq, "{:.2f}%", true);
}

// ── Repopulate ────────────────────────────────────────────────────────────────
static void Diamonds_Repopulate(HWND hWnd) {
    HWND hList = GetDlgItem(hWnd, ID_DIAMONDS_RESULTS_LIST);
    if (!hList) return;

    g_DiamondDisplayOrder.clear(); // Clear the virtual list viewport

    std::vector<TradingAPI::PositionInfo> rows;
    {
        std::lock_guard<std::mutex> lock(api().getPortfolioMutex());
        for (auto const& [conId, info] : api().getPortfolioMap()) {
            auto it = g_DiamondsTabMap.find(info.conId);
            int  assignedTab = (it != g_DiamondsTabMap.end()) ? it->second : DTAB_ALL;
            if (!((g_DiamondsCheckedTabs >> assignedTab) & 1)) continue;
            rows.push_back(info);
        }
    }

    // Build/refresh the cache rows.
    // Rule: only write the fields we own here (identity, position, avgCost, market
    // data). Never touch DCOL_DAILYPNL / DCOL_UNREALIZED_PL / DCOL_UNREALIZED_PL_PCT
    // — those are owned by WM_PNL_SINGLE and must survive a repopulate so they
    // remain visible when the window is closed and reopened.
    for (const auto& pos : rows) {
        // operator[] creates a default row only when the conId is new.
        // For existing rows it returns the current entry — PnL fields are preserved.
        auto& cacheRow = g_DiamondDataCache[pos.conId];
        cacheRow.conId  = pos.conId;
        cacheRow.symbol = pos.symbol;

        cacheRow.textCols[DCOL_SYMBOL] = pos.symbol;

        cacheRow.sortValues[DCOL_POSITION] = pos.shares;
        cacheRow.textCols[DCOL_POSITION] = std::format("{:.4g}", pos.shares);

        cacheRow.sortValues[DCOL_AVGPRICE] = pos.avgCost;
        cacheRow.textCols[DCOL_AVGPRICE] = std::format("{:.2f}", pos.avgCost);

        // Pre-fill market data if already cached — this also seeds the estimated
        // PnL columns for the first open (before WM_PNL_SINGLE arrives).
        TradingAPI::L1Book tickInfo;
        if (api().getWatchlistData(pos.conId, tickInfo)) {
            Diamonds_UpdateMarketCols(pos.conId, tickInfo);
        }

        Diamonds_UpdatePnLCols(hWnd, pos.conId);

        g_DiamondDisplayOrder.push_back(pos.conId);
    }

    // VIRTUAL LIST MAGIC: Tell the UI exactly how many items exist. It will ask for text later.
    ListView_SetItemCountEx(hList, g_DiamondDisplayOrder.size(), LVSICF_NOINVALIDATEALL | LVSICF_NOSCROLL);
    InvalidateRect(hList, NULL, FALSE);
    Diamonds_ApplySort(hList);

    std::string title = "Diamonds: " + std::to_string(rows.size());
    int activeTabs = 0;
    for (int i = 0; i < DIAMONDS_TAB_COUNT; ++i) {
        if (SendMessage(GetDlgItem(hWnd, ID_DIAMONDS_CHK_0 + i), BM_GETCHECK, 0, 0) == BST_CHECKED)
            title += std::string(activeTabs++ == 0 ? " " : " + ") + g_DiamondTabNames[i];
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
        SetTimer(hWnd, DIAMONDS_PAINT_TIMER_ID, DIAMONDS_PAINT_TIMER_MS, NULL);

        Diamonds_SetRowHeight(hList, 28);
        DiamondsZoomData.fontSize = (int)Settings_Load(DiamondsZoomData.settingKey, DiamondsZoomData.fontSize);
        ApplyListViewFont(hList, DiamondsZoomData.hFont, DiamondsZoomData.hBoldFont, DiamondsZoomData.fontSize);
        SetWindowSubclass(hList, ListViewZoomProc, 0, (DWORD_PTR)&DiamondsZoomData);

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
            HWND hChk = CreateWindowA("BUTTON", g_DiamondTabNames[i],
                WS_CHILD | BS_AUTOCHECKBOX | BS_NOTIFY,
                0, 0, 10, 10,
                hWnd, (HMENU)(UINT_PTR)(ID_DIAMONDS_CHK_0 + i), hInst, NULL);
            // Default: all checked.
            SendMessage(hChk, BM_SETCHECK, BST_CHECKED, 0);
        }


        // Load saved tab assignments, checkbox state, sort settings, and symbol colors.
        Diamonds_LoadTabMap();
        Diamonds_LoadSymbolColors();
        g_DiamondsSortCol = (int)Settings_Sort_Load(DIAMONDS_CLASS_NAME, "SortCol", DCOL_SYMBOL);
        g_DiamondsSortAsc = Settings_Sort_Load(DIAMONDS_CLASS_NAME, "SortAsc", 1) != 0;
        if (g_DiamondsSortCol < 0 || g_DiamondsSortCol >= DCOL_COUNT) g_DiamondsSortCol = DCOL_SYMBOL;

        // Restore checkbox bitmask (default 0x7 = all checked).
        g_DiamondsCheckedTabs = (UINT)Settings_CheckedTabs_Load(0x7);
        g_DiamondsCheckedTabs &= 0x7;  // clamp to valid 3-bit range
        for (int i = 0; i < DIAMONDS_TAB_COUNT; ++i) {
            bool checked = (g_DiamondsCheckedTabs >> i) & 1;
            SendMessage(GetDlgItem(hWnd, ID_DIAMONDS_CHK_0 + i),
                        BM_SETCHECK, checked ? BST_CHECKED : BST_UNCHECKED, 0);
        }
        Diamonds_UpdateDivColumnsVisibility(hWnd);

        api().addApiUpdateWindow(hWnd);
        Diamonds_Repopulate(hWnd);
        break;
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
            g_DiamondsCheckedTabs = 0;
            for (int i = 0; i < DIAMONDS_TAB_COUNT; ++i) {
                if (SendMessage(GetDlgItem(hWnd, ID_DIAMONDS_CHK_0 + i), BM_GETCHECK, 0, 0) == BST_CHECKED)
                    g_DiamondsCheckedTabs |= (1u << i);
            }
            Settings_CheckedTabs_Save((int)g_DiamondsCheckedTabs);
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

    // ── Live market data update for one symbol ────────────────────────────────
    // Posted by Impl::tickPrice / tickSize / tickString / tickGeneric
    case WM_MARKET_L1: {
        int conId = (int)lParam;
        if (!conId) break;
        TradingAPI::L1Book info;
        if (api().getWatchlistData(conId, info)) {
            Diamonds_UpdateMarketCols(conId, info);
            // ZERO-FLICKER FIX: Invalidate ONLY the row that changed, perfectly smoothly
            auto it = std::find(g_DiamondDisplayOrder.begin(), g_DiamondDisplayOrder.end(), conId);
            if (it != g_DiamondDisplayOrder.end()) {
                int row = (int)std::distance(g_DiamondDisplayOrder.begin(), it);
                HWND hList = GetDlgItem(hWnd, ID_DIAMONDS_RESULTS_LIST);
                if (hList) ListView_RedrawItems(hList, row, row);
            }
        }
        
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
            // Clear everything — positions and prices are stale.
            // ZERO-FLICKER FIX: Never use ListView_DeleteAllItems on an LVS_OWNERDATA list.
            // It's invalid for virtual lists and triggers a full erase flash.
            g_DiamondDisplayOrder.clear();
            g_DiamondDataCache.clear();
            ListView_SetItemCountEx(hList, 0, LVSICF_NOINVALIDATEALL);
            InvalidateRect(hList, NULL, FALSE);
        }
        break;
    }

    // ── Notification handling ─────────────────────────────────────────────────
    case WM_NOTIFY: {
        NMHDR* hdr = (NMHDR*)lParam;
        if (hdr->idFrom != ID_DIAMONDS_RESULTS_LIST) break;

        // --- VIRTUAL LIST TEXT REQUEST ---
        if (hdr->code == LVN_GETDISPINFO) {
            NMLVDISPINFO* pdi = (NMLVDISPINFO*)lParam;
            if (pdi->item.iItem < 0 || pdi->item.iItem >= (int)g_DiamondDisplayOrder.size()) return 0;
            
            int conId = g_DiamondDisplayOrder[pdi->item.iItem];
            const auto& row = g_DiamondDataCache[conId];

            if (pdi->item.mask & LVIF_TEXT) {
                // VIRTUAL LIST FIX: Direct pointer assignment is zero-copy and avoids buffer truncation
                pdi->item.pszText = (LPSTR)row.textCols[pdi->item.iSubItem].c_str();
            }
            return 0;
        }
        if (hdr->code == LVN_COLUMNCLICK) {
            NMLISTVIEW* nmlv = (NMLISTVIEW*)lParam;
            int col = nmlv->iSubItem;
            if (col == g_DiamondsSortCol) g_DiamondsSortAsc = !g_DiamondsSortAsc;
            else { g_DiamondsSortCol = col; g_DiamondsSortAsc = true; }
            Settings_Sort_Save(DIAMONDS_CLASS_NAME, "SortCol", g_DiamondsSortCol);
            Settings_Sort_Save(DIAMONDS_CLASS_NAME, "SortAsc", g_DiamondsSortAsc ? 1 : 0);
            HWND hList = GetDlgItem(hWnd, ID_DIAMONDS_RESULTS_LIST);
            Diamonds_ApplySort(hList);
            InvalidateRect(hList, NULL, FALSE);
            return 0;
        }

        if (hdr->code == NM_DBLCLK) {
            LPNMITEMACTIVATE act = (LPNMITEMACTIVATE)lParam;
            int row = act->iItem;
            if (row >= 0) {
                int conId = g_DiamondDisplayOrder[row];
                const std::string& sym = g_DiamondDataCache[conId].textCols[DCOL_SYMBOL];
                StartMarket(sym, conId);
            }
        }

        if (hdr->code == NM_RCLICK) {
            LPNMITEMACTIVATE act = (LPNMITEMACTIVATE)lParam;
            int row = act->iItem;
            if (row >= 0) {
                int conId = g_DiamondDisplayOrder[row];
                const std::string& sym = g_DiamondDataCache[conId].textCols[DCOL_SYMBOL];
                // Build the full registry entry string for this symbol.
                std::string exchange;
                {
                    std::lock_guard<std::mutex> lock(api().getPortfolioMutex());
                    auto& pmap = api().getPortfolioMap();
                    auto pit = pmap.find(conId);
                    if (pit != pmap.end()) exchange = pit->second.exchange;
                }
                std::string entry = std::to_string(conId) + "." + sym;
                if (!exchange.empty()) entry += "." + exchange;

                // Read watchlist lists fresh every time.
                std::vector<std::string> watchlistLists = Watchlist_LoadAllListNames();

                // Determine current group assignment for this item.
                auto mapIt = g_DiamondsTabMap.find(conId);
                int currentGroup = (mapIt != g_DiamondsTabMap.end()) ? mapIt->second : DTAB_ALL;

                // Determine current color assignment for this item.
                auto colorIt = g_DiamondsSymbolColors.find(conId);
                int currentColor = (colorIt != g_DiamondsSymbolColors.end()) ? colorIt->second : DIAMONDS_COLOR_NONE;

                // ── Build context menu ────────────────────────────────────────
                // IDs 1-3:   group assignment
                // IDs 100+:  "Add to Watchlist: <name>"
                // IDs 200-206: color options (200+idx for colors, 206 = None)
                HMENU hMenu = CreatePopupMenu();

                AppendMenuA(hMenu, MF_STRING | (currentGroup == DTAB_ALL        ? MF_GRAYED : 0), 1, "Move to Growth");
                AppendMenuA(hMenu, MF_STRING | (currentGroup == DTAB_GROWTH     ? MF_GRAYED : 0), 2, "Move to High-Yield Dividends");
                AppendMenuA(hMenu, MF_STRING | (currentGroup == DTAB_QUARENTINE ? MF_GRAYED : 0), 3, "Move to Quarantine");

                if (!watchlistLists.empty()) {
                    AppendMenuA(hMenu, MF_SEPARATOR, 0, NULL);
                    for (int i = 0; i < (int)watchlistLists.size(); ++i) {
                        // Gray this entry if the symbol is already in that list.
                        auto listEntries = Watchlist_ReadListEntries(watchlistLists[i].c_str());
                        bool alreadyIn = false;
                        for (const auto& e : listEntries)
                            if (e == entry) { alreadyIn = true; break; }
                        std::string label = "Add to Watchlist: " + watchlistLists[i];
                        AppendMenuA(hMenu, MF_STRING | (alreadyIn ? MF_GRAYED : 0), 100 + i, label.c_str());
                    }
                }

                // ── Color submenu ─────────────────────────────────────────────
                AppendMenuA(hMenu, MF_SEPARATOR, 0, NULL);
                
                // "None" option — grayed when no color is currently assigned.
                AppendMenuA(hMenu, MF_STRING | (currentColor == DIAMONDS_COLOR_NONE ? MF_GRAYED : 0),
                            200 + DIAMONDS_COLOR_COUNT, "Set Color: None");

                for (int i = 0; i < DIAMONDS_COLOR_COUNT; ++i) {
                    bool isCurrent = (currentColor == i);
                    AppendMenuA(hMenu, MF_STRING | (isCurrent ? MF_GRAYED : 0),
                                200 + i, g_DiamondColorPalette[i].label);
                }
                POINT pt;
                GetCursorPos(&pt);
                int cmd = (int)TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_RIGHTBUTTON,
                                              pt.x, pt.y, 0, hWnd, NULL);
                DestroyMenu(hMenu);

                if (cmd >= 1 && cmd <= 3) {
                    // Group assignment.
                    int targetTab = cmd - 1;
                    if (targetTab == DTAB_ALL)
                        g_DiamondsTabMap.erase(conId);
                    else
                        g_DiamondsTabMap[conId] = targetTab;
                    Diamonds_SaveTabMap();
                    Diamonds_Repopulate(hWnd);
                    InvalidateRect(hWnd, NULL, TRUE);
                } else if (cmd >= 100 && cmd < 100 + (int)watchlistLists.size()) {
                    // Add to watchlist list (only if not already present — menu grayed it, but guard anyway).
                    const std::string& listName = watchlistLists[cmd - 100];
                    auto entries = Watchlist_ReadListEntries(listName.c_str());
                    bool exists = false;
                    for (const auto& e : entries) if (e == entry) { exists = true; break; }
                    if (!exists) {
                        entries.push_back(entry);
                        Watchlist_SaveFullList(listName.c_str(), entries);
                        Watchlist_NotifyListChanged(listName);
                    }
                } else if (cmd >= 200 && cmd <= 200 + DIAMONDS_COLOR_COUNT) {
                    // Color assignment.
                    int pickedIdx = cmd - 200;
                    if (pickedIdx == DIAMONDS_COLOR_COUNT) {
                        // "None" — remove override.
                        g_DiamondsSymbolColors.erase(conId);
                    } else {
                        g_DiamondsSymbolColors[conId] = pickedIdx;
                    }
                    Diamonds_SaveSymbolColors();
                    // Invalidate just this row so the color appears immediately.
                    HWND hList = GetDlgItem(hWnd, ID_DIAMONDS_RESULTS_LIST);
                    ListView_RedrawItems(hList, row, row);
                    UpdateWindow(hList);
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
                        SelectObject(cd->nmcd.hdc, DiamondsZoomData.hBoldFont);
                        int rowIndex = (int)cd->nmcd.dwItemSpec;
                        int conId = g_DiamondDisplayOrder[rowIndex];
                        
                        auto cit = g_DiamondsSymbolColors.find(conId);
                        if (cit != g_DiamondsSymbolColors.end() &&
                            cit->second >= 0 && cit->second < DIAMONDS_COLOR_COUNT) {
                            cd->clrText = g_DiamondColorPalette[cit->second].rgb;
                            if (dark) cd->clrTextBk = (cd->nmcd.dwItemSpec % 2 == 0) ? DM_BG : DM_BG2;
                        }
                        return CDRF_NEWFONT;
                    }
                    // Only colour P&L / change columns — and only when the
                    // cell holds a real numeric value (not the "--" sentinel).
                    if (cd->iSubItem == DCOL_CHGPCT || cd->iSubItem == DCOL_DAILYPNL || cd->iSubItem == DCOL_UNREALIZED_PL || cd->iSubItem == DCOL_UNREALIZED_PL_PCT || cd->iSubItem == DCOL_POSITION) {
                        int rowIndex = (int)cd->nmcd.dwItemSpec;
                        int conId = g_DiamondDisplayOrder[rowIndex];
                        const std::string& textVal = g_DiamondDataCache[conId].textCols[cd->iSubItem];
                        // Guard: skip colouring the "--" sentinel — atof("--") == 0
                        // which would leave the cell uncoloured anyway, but being
                        // explicit avoids any locale-specific atof surprises.
                        if (!textVal.empty()) {
                            double val = atof(textVal.c_str());
                            if      (val >= 0.0) cd->clrText = COINS_CLR_GREEN;
                            else if (val < 0.0) cd->clrText = COINS_CLR_RED;
                        }
                        if (dark) cd->clrTextBk = (cd->nmcd.dwItemSpec % 2 == 0) ? DM_BG : DM_BG2;
                        SelectObject(cd->nmcd.hdc, DiamondsZoomData.hFont);
                        // For the Position cell also request post-paint so we can
                        // overlay the mini sparkline after the text is drawn.
                        if (cd->iSubItem == DCOL_POSITION)
                            return CDRF_NEWFONT | CDRF_NOTIFYPOSTPAINT;
                        return CDRF_NEWFONT;
                    }
                    if (cd->iSubItem == DCOL_ASKSIZE || cd->iSubItem == DCOL_BIDSIZE) {
                        cd->clrText = COINS_CLR_BLUE;
                        if (dark) cd->clrTextBk = (cd->nmcd.dwItemSpec % 2 == 0) ? DM_BG : DM_BG2;
                        return CDRF_NEWFONT;
                    }
                    if (cd->iSubItem == DCOL_ASK || cd->iSubItem == DCOL_LAST || cd->iSubItem == DCOL_BID) {
                        int rowIndex = (int)cd->nmcd.dwItemSpec;
                        int conId = g_DiamondDisplayOrder[rowIndex];
                        const std::string& textValB = g_DiamondDataCache[conId].textCols[DCOL_BIDSIZE];
                        const std::string& textValA = g_DiamondDataCache[conId].textCols[DCOL_ASKSIZE];
                        if (!textValB.empty() && !textValA.empty()) {
                            double valB = atof(textValB.c_str());
                            double valA = atof(textValA.c_str());
                            if      (valA > valB) cd->clrText = COINS_CLR_RED;
                            else if (valA < valB) cd->clrText = COINS_CLR_GREEN;
                            else cd->clrText = dark ? DM_TEXT : LM_TEXT;
                        }
                        if (dark) cd->clrTextBk = (cd->nmcd.dwItemSpec % 2 == 0) ? DM_BG : DM_BG2;
                        return CDRF_NEWFONT;
                    }
                    if (cd->iSubItem == DCOL_PCT_NETLIQ) {
                        cd->clrText = COINS_CLR_GRAY;
                        if (dark) cd->clrTextBk = (cd->nmcd.dwItemSpec % 2 == 0) ? DM_BG : DM_BG2;
                        return CDRF_NEWFONT;
                    }
                    if (cd->iSubItem == DCOL_AVGPRICE || cd->iSubItem == DCOL_MKTVAL) {
                        if (dark) {
                            cd->clrTextBk = (cd->nmcd.dwItemSpec % 2 == 0) ? DM_BG : DM_BG2;
                            cd->clrText   = DM_TEXT;
                        } else {
                            cd->clrTextBk = (cd->nmcd.dwItemSpec % 2 == 0) ? GetSysColor(COLOR_WINDOW) : RGB(245, 245, 245);
                            cd->clrText   = LM_TEXT;
                        }
                        return CDRF_NEWFONT;
                    }
                    if (cd->iSubItem == DCOL_DIV_YIELD || cd->iSubItem == DCOL_DIV_DATE  ||  cd->iSubItem == DCOL_DIV_AMT || cd->iSubItem == DCOL_ANNUAL_DIV) {
                        cd->clrText = COINS_CLR_PURPLE;
                        if (dark) cd->clrTextBk = (cd->nmcd.dwItemSpec % 2 == 0) ? DM_BG : DM_BG2;
                        return CDRF_NEWFONT;
                    }
                    return CDRF_DODEFAULT;
                }

                case CDDS_ITEMPOSTPAINT | CDDS_SUBITEM: {
                    if (cd->iSubItem != DCOL_POSITION) return CDRF_DODEFAULT;

                    int rowIndex = (int)cd->nmcd.dwItemSpec;
                    if (rowIndex < 0 || rowIndex >= g_DiamondDisplayOrder.size()) return CDRF_DODEFAULT;

                    int conId = g_DiamondDisplayOrder[rowIndex];
                    auto sit  = g_DiamondsSparklines.find(conId);
                    if (sit == g_DiamondsSparklines.end() || !sit->second.HasData())
                        return CDRF_DODEFAULT;

                    RECT cellRect;
                    ListView_GetSubItemRect(GetDlgItem(hWnd, ID_DIAMONDS_RESULTS_LIST), rowIndex, DCOL_POSITION, LVIR_BOUNDS, &cellRect);
                    sit->second.Draw(cd->nmcd.hdc, cellRect);
                    return CDRF_DODEFAULT;
                }
            }
        }
        break;
    }

    case WM_TIMER: {
        if (wParam == DIAMONDS_PAINT_TIMER_ID) {
            if (g_DiamondsDirty) {
                HWND hList = GetDlgItem(hWnd, ID_DIAMONDS_RESULTS_LIST);
                if (hList && !g_DiamondDisplayOrder.empty()) {
                    // Get the range of items currently visible to the user
                    int top = ListView_GetTopIndex(hList);
                    int count = ListView_GetCountPerPage(hList);
                    int bottom = top + count;
                    
                    // Clamp to actual size
                    if (bottom >= (int)g_DiamondDisplayOrder.size()) 
                        bottom = (int)g_DiamondDisplayOrder.size() - 1;

                    // Only redraw the specific rows that have changed on screen
                    ListView_RedrawItems(hList, top, bottom);
                    UpdateWindow(hList); // Force immediate flush
                }
                g_DiamondsDirty = false;
            }
        }
        break;  // was missing — without this, every timer tick fell through into WM_DESTROY,
                // killing timers, clearing the cache, and calling removeApiUpdateWindow.
    }

    case WM_DESTROY:
        KillTimer(hWnd, DIAMONDS_SORT_TIMER_ID);
        KillTimer(hWnd, DIAMONDS_PAINT_TIMER_ID);
        g_DiamondsSortPending = false;
        api().removeApiUpdateWindow(hWnd);
        g_DiamondDataCache.clear();
        g_DiamondsSparklines.clear();
        if (g_DiamondsRowHeightImageList) {
            ImageList_Destroy(g_DiamondsRowHeightImageList);
            g_DiamondsRowHeightImageList = NULL;
        }
        if (DiamondsZoomData.hFont) {
            DeleteObject(DiamondsZoomData.hFont);
        }
        if (DiamondsZoomData.hBoldFont) {
            DeleteObject(DiamondsZoomData.hBoldFont);
        }
        break;
    }

    return HandleCommonMessages(hWnd, message, wParam, lParam);
}
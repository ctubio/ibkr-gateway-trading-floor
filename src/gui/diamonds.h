#pragma once
// Requires <windowsx.h> for GET_X_LPARAM / GET_Y_LPARAM (include before this header).

void StartDiamonds() { StartGenericWindow(DIAMONDS_CLASS_NAME, "Diamonds", L"IBKRGatewayClient.Diamonds", 1476, 420); }

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

static ListViewZoomData DiamondsZoomData = { NULL, NULL, 14, "Zoom_Diamonds" };

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
static int  g_DiamondsSortCol = DCOL_SYMBOL;
static bool g_DiamondsSortAsc = true;

// ── Column definitions ────────────────────────────────────────────────────────

struct DiamondCol { const char* header; int width; int fmt; };
static const DiamondCol diamondCols[] = {
    { "Instrument",        90, LVCFMT_LEFT  },
    { "Position",          70, LVCFMT_RIGHT },
    { "Avg Price",         80, LVCFMT_RIGHT },
    { "Ask Size",          70, LVCFMT_RIGHT },
    { "Ask",               70, LVCFMT_RIGHT },
    { "Last",              70, LVCFMT_RIGHT },
    { "Bid",               70, LVCFMT_RIGHT },
    { "Bid Size",          70, LVCFMT_RIGHT },
    { "Daily P&L",         85, LVCFMT_RIGHT },
    { "Change %",          70, LVCFMT_RIGHT },
    { "Unrealized P&L",    95, LVCFMT_RIGHT },
    { "Unrealized P&L %",  95, LVCFMT_RIGHT },
    { "Market Value",      85, LVCFMT_RIGHT },
    { "% of Net Liq",      85, LVCFMT_RIGHT },
    { "Div Yield %",       80, LVCFMT_RIGHT },
    { "Div Date",          85, LVCFMT_RIGHT },
    { "Div Amount",        80, LVCFMT_RIGHT },
    { "Annual Div",        80, LVCFMT_RIGHT },
};
static_assert((int)(sizeof(diamondCols) / sizeof(diamondCols[0])) == DCOL_COUNT,
              "diamondCols count must match DiamondColIdx::DCOL_COUNT");

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
    Settings_SaveString("DiamondsTabGrowth", growthList);
    Settings_SaveString("DiamondsTabQuarantine",    quarentineList);
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
    parseIds(Settings_LoadString("DiamondsTabGrowth"), DTAB_GROWTH);
    parseIds(Settings_LoadString("DiamondsTabQuarantine"),    DTAB_QUARENTINE);
}

// ── Symbol color persistence ──────────────────────────────────────────────────

static void Diamonds_SaveSymbolColors() {
    std::string s;
    for (auto& [conId, idx] : g_DiamondsSymbolColors) {
        if (!s.empty()) s += ' ';
        s += std::to_string(conId) + ':' + std::to_string(idx);
    }
    Settings_SaveString("DiamondsSymbolColors", s);
}

static void Diamonds_LoadSymbolColors() {
    g_DiamondsSymbolColors.clear();
    std::string s = Settings_LoadString("DiamondsSymbolColors");
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

static int CALLBACK Diamonds_SortCompare(LPARAM idx1, LPARAM idx2, LPARAM /*extra*/) {
    char b1[64] = {}, b2[64] = {};
    ListView_GetItemText(g_DiamondsListForSort, (int)idx1, g_DiamondsSortCol, b1, sizeof(b1));
    ListView_GetItemText(g_DiamondsListForSort, (int)idx2, g_DiamondsSortCol, b2, sizeof(b2));
    int cmp;
    if (Diamonds_ColIsNumeric(g_DiamondsSortCol)) {
        double v1 = atof(b1), v2 = atof(b2);
        cmp = (v1 < v2) ? -1 : (v1 > v2) ? 1 : 0;
    } else {
        cmp = _stricmp(b1, b2);
    }
    return g_DiamondsSortAsc ? cmp : -cmp;
}

static void Diamonds_ApplySort(HWND hList) {
    g_DiamondsListForSort = hList;
    ListView_SortItemsEx(hList, Diamonds_SortCompare, 0);
    Diamonds_SetSortArrow(hList, g_DiamondsSortCol, g_DiamondsSortAsc);
}

// ── Helpers ───────────────────────────────────────────────────────────────────

// Sentinel string displayed whenever a value cannot be computed (e.g. market closed, last == 0).
static const char* DIAMONDS_NO_DATA = "--";

// Find a row in the Diamonds list by conId (stored in lParam).
static int Diamonds_FindRow(HWND hList, int conId) {
    int count = ListView_GetItemCount(hList);
    for (int i = 0; i < count; ++i) {
        LVITEMA lvi = {};
        lvi.mask  = LVIF_PARAM;
        lvi.iItem = i;
        ListView_GetItem(hList, &lvi);
        if ((int)lvi.lParam == conId) return i;
    }
    return -1;
}

// Update the market-data columns of one row from a WatchlistInfo snapshot.
// All columns that depend on a valid Last price display "--" when Last == 0,
// preventing bogus P&L calculations (e.g. -100%) during market-closed hours.
static void Diamonds_UpdateMarketCols(HWND hList, int row, const TradingAPI::WatchlistInfo& t) {
    char buf[64];

    // Helper: format a non-zero double, or blank the cell.
    auto setNum = [&](int col, double val, const char* fmt) {
        if (val != 0.0) snprintf(buf, sizeof(buf), fmt, val);
        else            buf[0] = '\0';
        ListView_SetItemText(hList, row, col, buf);
    };

    // Helper: set a cell to the "--" sentinel.
    auto setNA = [&](int col) {
        ListView_SetItemText(hList, row, col, (LPSTR)DIAMONDS_NO_DATA);
    };

    auto setStr = [&](int col, const std::string& val) {
        ListView_SetItemText(hList, row, col, (LPSTR)val.c_str());
    };

    // ── Columns that do NOT require a valid Last ──────────────────────────────
    setNum(DCOL_ASKSIZE, t.askSize, "%.0f");
    setNum(DCOL_ASK,     t.ask,     "%.2f");
    setNum(DCOL_BID,     t.bid,     "%.2f");
    setNum(DCOL_BIDSIZE, t.bidSize, "%.0f");

    // ── Dividend columns — independent of Last being live ────────────────────
    setNum(DCOL_DIV_YIELD,  t.dividendYield(),   "%.2f%%");
    setStr(DCOL_DIV_DATE,   t.dividendDate);
    setNum(DCOL_DIV_AMT,    t.dividendAmount,    "%.3f");
    setNum(DCOL_ANNUAL_DIV, t.annualDividends,   "%.3f");

    // ── Columns that require Last > 0 ─────────────────────────────────────────
    // When the market is closed, reqMktData may return Last == 0.
    // In that case the gateway falls back to reqHistoricalData to populate
    // prevClose (and Last itself if still 0 after the live subscription).
    // Until a valid Last is available we show "--" to avoid misleading values.
    if (t.last <= 0.0) {
        setNA(DCOL_LAST);
        setNA(DCOL_DAILYPNL);
        setNA(DCOL_CHGPCT);
        setNA(DCOL_UNREALIZED_PL);
        setNA(DCOL_UNREALIZED_PL_PCT);
        setNA(DCOL_MKTVAL);
        setNA(DCOL_PCT_NETLIQ);
        return;
    }

    // Last is valid — proceed with normal calculations.
    snprintf(buf, sizeof(buf), "%.2f", t.last);
    ListView_SetItemText(hList, row, DCOL_LAST, buf);

    // Retrieve portfolio data under the portfolio mutex.
    char symBuf[64];
    ListView_GetItemText(hList, row, DCOL_SYMBOL, symBuf, sizeof(symBuf));
    std::string symbol = symBuf;

    double shares = 0.0, avgCost = 0.0;
    {
        std::lock_guard<std::mutex> lock(api.getPortfolioMutex());
        auto& pmap = api.getPortfolioMap();
        if (pmap.count(symbol)) {
            shares  = pmap[symbol].shares;
            avgCost = pmap[symbol].avgCost;
        }
    }

    double netLiq = 0.0;
    auto summary = api.getAccountSummary();
    if (summary.count("NetLiquidation")) {
        try { netLiq = std::stod(summary["NetLiquidation"]); } catch (...) {}
    }

    // Only compute daily P&L when we have a valid prevClose to diff against.
    double mktVal     = shares * t.last;
    double unrlPnL    = (avgCost > 0.0) ? shares * (t.last - avgCost) : 0.0;
    double unrlPnLPct = (avgCost > 0.0 && t.last > 0.0)
                        ? ((t.last - avgCost) / avgCost * 100.0) : 0.0;
    double pctNetLiq  = (netLiq > 0.0 && mktVal != 0.0) ? (mktVal / netLiq * 100.0) : 0.0;

    // Daily P&L: only meaningful when prevClose is available.
    if (t.prevClose > 0.0) {
        double dailyPnL = shares * t.change();
        setNum(DCOL_DAILYPNL, dailyPnL, "%+.2f");
        setNum(DCOL_CHGPCT,   t.changePct(), "%+.2f%%");
    } else {
        setNA(DCOL_DAILYPNL);
        setNA(DCOL_CHGPCT);
    }

    setNum(DCOL_UNREALIZED_PL,     unrlPnL,    "%+.2f");
    setNum(DCOL_UNREALIZED_PL_PCT, unrlPnLPct, "%+.2f%%");
    setNum(DCOL_MKTVAL,            mktVal,     "%.2f");
    setNum(DCOL_PCT_NETLIQ,        pctNetLiq,  "%.2f%%");
}

// ── Repopulate ────────────────────────────────────────────────────────────────

static void Diamonds_Repopulate(HWND hWnd) {
    HWND hList = GetDlgItem(hWnd, ID_DIAMONDS_RESULTS_LIST);
    if (!hList) return;

    SendMessage(hList, WM_SETREDRAW, FALSE, 0);
    ListView_DeleteAllItems(hList);

    std::vector<TradingAPI::PositionInfo> rows;
    {
        std::lock_guard<std::mutex> lock(api.getPortfolioMutex());
        for (auto const& [sym, info] : api.getPortfolioMap()) {
            // Show item if its assigned group's bit is set in g_DiamondsCheckedTabs.
            auto it = g_DiamondsTabMap.find(info.conId);
            int  assignedTab = (it != g_DiamondsTabMap.end()) ? it->second : DTAB_ALL;
            if (!((g_DiamondsCheckedTabs >> assignedTab) & 1)) continue;
            rows.push_back(info);
        }
    }

    // Initial insertion order: alphabetical by symbol.
    // After insert we re-sort by the active sort column/direction.
    std::sort(rows.begin(), rows.end(),
              [](const TradingAPI::PositionInfo& a, const TradingAPI::PositionInfo& b) {
                  return a.symbol < b.symbol;
              });

    char buf[64];
    for (int i = 0; i < (int)rows.size(); ++i) {
        const auto& pos = rows[i];

        // ── Symbol (col 0) with conId in lParam ──────────────────────────────
        LVITEMA lvi = {};
        lvi.mask     = LVIF_TEXT | LVIF_PARAM;
        lvi.iItem    = i;
        lvi.iSubItem = 0;
        lvi.lParam   = pos.conId;
        lvi.pszText  = (LPSTR)pos.symbol.c_str();
        ListView_InsertItem(hList, &lvi);

        // ── Position columns ─────────────────────────────────────────────────
        snprintf(buf, sizeof(buf), "%.4g", pos.shares);
        ListView_SetItemText(hList, i, DCOL_POSITION, buf);

        snprintf(buf, sizeof(buf), "%.2f", pos.avgCost);
        ListView_SetItemText(hList, i, DCOL_AVGPRICE, buf);

        // ── Market data columns — pre-fill from cache if already available ───
        TradingAPI::WatchlistInfo tickInfo;
        if (api.getWatchlistData(pos.conId, pos.symbol, tickInfo))
            Diamonds_UpdateMarketCols(hList, i, tickInfo);
    }

    SendMessage(hList, WM_SETREDRAW, TRUE, 0);
    RedrawWindow(hList, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
    Diamonds_ApplySort(hList);

    std::string title = "Diamonds: " + std::to_string(rows.size()) + " ";
    int activeTabs = 0;
    for (int i = 0; i < DIAMONDS_TAB_COUNT; ++i) {
        if (SendMessage(GetDlgItem(hWnd, ID_DIAMONDS_CHK_0 + i), BM_GETCHECK, 0, 0) == BST_CHECKED)
            title += std::string(activeTabs++ == 0 ? "" : " + ") + g_DiamondTabNames[i];
    }
    title += " Positions";
    SetWindowTextA(hWnd, title.c_str());
}

// ── Window procedure ──────────────────────────────────────────────────────────

LRESULT CALLBACK WndProcDiamonds(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {

    // ── Checkboxes show when active, hide when inactive ───────────────────────
    case WM_ACTIVATE:
        Diamonds_ShowCheckboxes(hWnd, LOWORD(wParam) != WA_INACTIVE);
        return 0;

    case WM_CREATE: {
        HINSTANCE hInst = ((LPCREATESTRUCT)lParam)->hInstance;

        HWND hList = CreateWindowExA(
            WS_EX_CLIENTEDGE, "SysListView32", "",
            WS_CHILD | WS_VISIBLE | WS_BORDER |
            LVS_REPORT | LVS_SHOWSELALWAYS,
            0, 0, 1100, 420,
            hWnd, (HMENU)ID_DIAMONDS_RESULTS_LIST, hInst, NULL);

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

        api.setDiamondsWindow(hWnd);
        api.addApiUpdateWindow(hWnd);

        // Load saved tab assignments, checkbox state, sort settings, and symbol colors.
        Diamonds_LoadTabMap();
        Diamonds_LoadSymbolColors();
        g_DiamondsSortCol = (int)Settings_Load("DiamondsSortCol", DCOL_SYMBOL);
        g_DiamondsSortAsc = Settings_Load("DiamondsSortAsc", 1) != 0;
        if (g_DiamondsSortCol < 0 || g_DiamondsSortCol >= DCOL_COUNT) g_DiamondsSortCol = DCOL_SYMBOL;

        // Restore checkbox bitmask (default 0x7 = all checked).
        g_DiamondsCheckedTabs = (UINT)Settings_Load("DiamondsCheckedTabs", 0x7);
        g_DiamondsCheckedTabs &= 0x7;  // clamp to valid 3-bit range
        for (int i = 0; i < DIAMONDS_TAB_COUNT; ++i) {
            bool checked = (g_DiamondsCheckedTabs >> i) & 1;
            SendMessage(GetDlgItem(hWnd, ID_DIAMONDS_CHK_0 + i),
                        BM_SETCHECK, checked ? BST_CHECKED : BST_UNCHECKED, 0);
        }
        break;
    }

    case WM_SIZE: {
        Diamonds_Layout(hWnd);
        break;
    }

    case WM_COMMAND: {
        WORD id = LOWORD(wParam);
        if (id >= ID_DIAMONDS_CHK_0 && id <= ID_DIAMONDS_CHK_2 && HIWORD(wParam) == BN_CLICKED) {
            // Rebuild bitmask from checkbox states.
            g_DiamondsCheckedTabs = 0;
            for (int i = 0; i < DIAMONDS_TAB_COUNT; ++i) {
                if (SendMessage(GetDlgItem(hWnd, ID_DIAMONDS_CHK_0 + i), BM_GETCHECK, 0, 0) == BST_CHECKED)
                    g_DiamondsCheckedTabs |= (1u << i);
            }
            Settings_Save("DiamondsCheckedTabs", (int)g_DiamondsCheckedTabs);
            Diamonds_Repopulate(hWnd);
        }
        break;
    }

    case WM_DIAMONDS_UPDATE: {
        Diamonds_Repopulate(hWnd);
        break;
    }

    // ── Live market data update for one symbol ────────────────────────────────
    // Posted by Impl::tickPrice / tickSize / tickString / tickGeneric
    // lParam = new std::string("conId.symbol") — we own it and must delete.
    case WM_WATCHLIST_UPDATE: {
        auto* key = reinterpret_cast<std::string*>(lParam);
        if (!key) break;

        auto dot = key->find('.');
        if (dot != std::string::npos) {
            int conId          = std::stoi(key->substr(0, dot));
            std::string symbol = key->substr(dot + 1);

            TradingAPI::WatchlistInfo info;
            if (api.getWatchlistData(conId, symbol, info)) {
                HWND hList = GetDlgItem(hWnd, ID_DIAMONDS_RESULTS_LIST);
                int row = Diamonds_FindRow(hList, conId);
                if (row >= 0)
                    Diamonds_UpdateMarketCols(hList, row, info);
            }
        }
        delete key;
        break;
    }

    // ── Connection state changed ──────────────────────────────────────────────
    case WM_API_UPDATE: {
        HWND hList = GetDlgItem(hWnd, ID_DIAMONDS_RESULTS_LIST);
        if (!hList) break;
        if (api.isMarketDataConnected() && api.isTradingConnected()) {
            // Re-request positions (market data re-subscribed in positionEnd()).
            api.setDiamondsWindow(hWnd);
        } else {
            // Clear everything — positions and prices are stale.
            ListView_DeleteAllItems(hList);
        }
        break;
    }

    // ── Notification handling ─────────────────────────────────────────────────
    case WM_NOTIFY: {
        NMHDR* hdr = (NMHDR*)lParam;
        if (hdr->idFrom != ID_DIAMONDS_RESULTS_LIST) break;

        if (hdr->code == LVN_COLUMNCLICK) {
            NMLISTVIEW* nmlv = (NMLISTVIEW*)lParam;
            int col = nmlv->iSubItem;
            if (col == g_DiamondsSortCol) g_DiamondsSortAsc = !g_DiamondsSortAsc;
            else { g_DiamondsSortCol = col; g_DiamondsSortAsc = true; }
            Settings_Save("DiamondsSortCol", g_DiamondsSortCol);
            Settings_Save("DiamondsSortAsc", g_DiamondsSortAsc ? 1 : 0);
            HWND hList = GetDlgItem(hWnd, ID_DIAMONDS_RESULTS_LIST);
            Diamonds_ApplySort(hList);
            return 0;
        }

        if (hdr->code == NM_DBLCLK) {
            LPNMITEMACTIVATE act = (LPNMITEMACTIVATE)lParam;
            int row = act->iItem;
            if (row >= 0) {
                HWND hList = GetDlgItem(hWnd, ID_DIAMONDS_RESULTS_LIST);
                char sym[256] = {};
                ListView_GetItemText(hList, row, 0, sym, sizeof(sym));
                LVITEMA item = {};
                item.mask  = LVIF_PARAM;
                item.iItem = row;
                ListView_GetItem(hList, &item);
                StartMarket(sym, (int)item.lParam);
            }
        }

        if (hdr->code == NM_RCLICK) {
            LPNMITEMACTIVATE act = (LPNMITEMACTIVATE)lParam;
            int row = act->iItem;
            if (row >= 0) {
                HWND hList = GetDlgItem(hWnd, ID_DIAMONDS_RESULTS_LIST);

                // Get conId and symbol from the clicked row.
                char sym[64] = {};
                ListView_GetItemText(hList, row, 0, sym, sizeof(sym));
                LVITEMA item = {};
                item.mask  = LVIF_PARAM;
                item.iItem = row;
                ListView_GetItem(hList, &item);
                int conId = (int)item.lParam;

                // Build the full registry entry string for this symbol.
                std::string exchange;
                {
                    std::lock_guard<std::mutex> lock(api.getPortfolioMutex());
                    auto& pmap = api.getPortfolioMap();
                    auto pit = pmap.find(sym);
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

                AppendMenuA(hMenu, MF_STRING | (currentGroup == DTAB_ALL        ? MF_GRAYED : 0), 1, "Assign to Growth");
                AppendMenuA(hMenu, MF_STRING | (currentGroup == DTAB_GROWTH     ? MF_GRAYED : 0), 2, "Assign to High-Yield Dividends");
                AppendMenuA(hMenu, MF_STRING | (currentGroup == DTAB_QUARENTINE ? MF_GRAYED : 0), 3, "Assign to Quarantine");

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
                        auto cit = g_DiamondsSymbolColors.find((int)cd->nmcd.lItemlParam);
                        if (cit != g_DiamondsSymbolColors.end() &&
                            cit->second >= 0 && cit->second < DIAMONDS_COLOR_COUNT) {
                            cd->clrText = g_DiamondColorPalette[cit->second].rgb;
                            if (dark) cd->clrTextBk = (cd->nmcd.dwItemSpec % 2 == 0) ? DM_BG : DM_BG2;
                        }
                        return CDRF_NEWFONT;
                    }
                    // Only colour P&L / change columns — and only when the
                    // cell holds a real numeric value (not the "--" sentinel).
                    if (cd->iSubItem == DCOL_CHGPCT    ||
                        cd->iSubItem == DCOL_DAILYPNL  ||
                        cd->iSubItem == DCOL_UNREALIZED_PL ||
                        cd->iSubItem == DCOL_UNREALIZED_PL_PCT||
                        cd->iSubItem == DCOL_POSITION)
                    {
                        HWND hList = GetDlgItem(hWnd, ID_DIAMONDS_RESULTS_LIST);
                        char buf[32] = {};
                        ListView_GetItemText(hList, (int)cd->nmcd.dwItemSpec, cd->iSubItem, buf, sizeof(buf));

                        // Guard: skip colouring the "--" sentinel — atof("--") == 0
                        // which would leave the cell uncoloured anyway, but being
                        // explicit avoids any locale-specific atof surprises.
                        if (strcmp(buf, DIAMONDS_NO_DATA) != 0 && buf[0] != '\0') {
                            double val = atof(buf);
                            if      (val >= 0.0) cd->clrText = RGB(80, 200, 120);
                            else if (val < 0.0) cd->clrText = RGB(220, 80, 80);
                        }
                        if (dark) cd->clrTextBk = (cd->nmcd.dwItemSpec % 2 == 0) ? DM_BG : DM_BG2;
                        SelectObject(cd->nmcd.hdc, DiamondsZoomData.hFont);
                        return CDRF_NEWFONT;
                    }
                    if (cd->iSubItem == DCOL_AVGPRICE    ||
                        cd->iSubItem == DCOL_MKTVAL)
                    {
                        if (dark) {
                            cd->clrTextBk = (cd->nmcd.dwItemSpec % 2 == 0) ? DM_BG : DM_BG2;
                            cd->clrText   = DM_TEXT;
                        } else {
                            cd->clrTextBk = (cd->nmcd.dwItemSpec % 2 == 0) ? GetSysColor(COLOR_WINDOW) : RGB(245, 245, 245);
                            cd->clrText   = LM_TEXT;
                        }
                        return CDRF_NEWFONT;
                    }
                    if (cd->iSubItem == DCOL_ASKSIZE    ||
                        cd->iSubItem == DCOL_BIDSIZE)
                    {
                        cd->clrText = RGB(51, 146, 255);
                        if (dark) cd->clrTextBk = (cd->nmcd.dwItemSpec % 2 == 0) ? DM_BG : DM_BG2;
                        return CDRF_NEWFONT;
                    }
                    return CDRF_DODEFAULT;
                }
            }
        }
        break;
    }

    case WM_DESTROY:
        api.unsetDiamondsWindow();   // cancels market data subscriptions + positions
        api.removeApiUpdateWindow(hWnd);
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
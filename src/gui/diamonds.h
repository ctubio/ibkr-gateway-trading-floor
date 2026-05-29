#pragma once
// Requires <windowsx.h> for GET_X_LPARAM / GET_Y_LPARAM (include before this header).

void StartDiamonds() { StartGenericWindow(DIAMONDS_CLASS_NAME, "Diamonds", L"IBKRGatewayClient.Diamonds", 1476, 420); }

#define ID_DIAMONDS_RESULTS_LIST 7001

// ── Titlebar tab constants ────────────────────────────────────────────────────
#define DTAB_ALL              0
#define DTAB_WATCH            1
#define DTAB_DIV              2
#define DIAMONDS_TAB_COUNT    3
#define DIAMONDS_TAB_HEIGHT   26        // height of the NC band carved below the caption
#define DIAMONDS_TAB_WIDTH    100       // fixed width per tab, pixels

static const char* g_DiamondTabNames[DIAMONDS_TAB_COUNT] = { "All", "Watchlist", "Dividends" };
static int         g_DiamondsActiveTab = DTAB_ALL;
// Maps conId → assigned tab (DTAB_ALL = untagged, visible in all tabs).
static std::map<int,int> g_DiamondsTabMap;

static ListViewZoomData DiamondsZoomData = { NULL, 14, "DiamondsListZoom" };

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
    std::string watchList, divList;
    for (auto& [conId, tab] : g_DiamondsTabMap) {
        if (tab == DTAB_WATCH) {
            if (!watchList.empty()) watchList += ' ';
            watchList += std::to_string(conId);
        } else if (tab == DTAB_DIV) {
            if (!divList.empty()) divList += ' ';
            divList += std::to_string(conId);
        }
    }
    Settings_SaveString("DiamondsTabWatch", watchList);
    Settings_SaveString("DiamondsTabDiv",   divList);
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
    parseIds(Settings_LoadString("DiamondsTabWatch"), DTAB_WATCH);
    parseIds(Settings_LoadString("DiamondsTabDiv"),   DTAB_DIV);
}

// ── Tab geometry & painting ───────────────────────────────────────────────────
//
// A DIAMONDS_TAB_HEIGHT px band is carved out of the NC area immediately below
// the caption (via WM_NCCALCSIZE).  Tabs are painted there with GetWindowDC.
// The ListView client area is completely untouched.

// Returns the tab band rect in WINDOW coordinates.
// Uses ClientToScreen so it works correctly when maximized or DPI-scaled.
static RECT Diamonds_TabBandRect(HWND hWnd) {
    RECT  wr;  GetWindowRect(hWnd, &wr);
    POINT ptCl = { 0, 0 };
    ClientToScreen(hWnd, &ptCl);
    int clientTopW = ptCl.y - wr.top;       // client top in window coords
    if (clientTopW < DIAMONDS_TAB_HEIGHT) return { 0, 0, 0, 0 };
    return { 0, clientTopW - DIAMONDS_TAB_HEIGHT, wr.right - wr.left, clientTopW };
}

// Returns 0-based tab index under ptW (window coords), or -1.
static int Diamonds_HitTestTab(HWND hWnd, POINT ptW) {
    RECT band = Diamonds_TabBandRect(hWnd);
    if (ptW.y < band.top || ptW.y >= band.bottom) return -1;
    for (int i = 0; i < DIAMONDS_TAB_COUNT; ++i) {
        int x0 = band.left + i * DIAMONDS_TAB_WIDTH;
        if (ptW.x >= x0 && ptW.x < x0 + DIAMONDS_TAB_WIDTH) return i;
    }
    return -1;
}

// Paints the tab band into the NC area.  Call AFTER DefWindowProc so we
// overdraw whatever the system put there.
static void Diamonds_PaintTabs(HWND hWnd) {
    RECT band = Diamonds_TabBandRect(hWnd);
    if (band.right == 0) return;

    HDC hdc = GetWindowDC(hWnd);
    if (!hdc) return;

    bool dark = Settings_DarkMode();

    // Band background — a solid stripe clearly distinct from the caption above.
    COLORREF bgCol = dark ? RGB(25, 25, 28) : RGB(235, 235, 238);
    HBRUSH   bgBr  = CreateSolidBrush(bgCol);
    FillRect(hdc, &band, bgBr);
    DeleteObject(bgBr);

    // Bottom separator line (between band and client area).
    COLORREF sepCol = dark ? RGB(75, 75, 80) : RGB(180, 180, 185);
    HPEN     sepPen = CreatePen(PS_SOLID, 1, sepCol);
    HPEN     oldPen = (HPEN)SelectObject(hdc, sepPen);
    MoveToEx(hdc, band.left,  band.bottom - 1, NULL);
    LineTo  (hdc, band.right, band.bottom - 1);
    SelectObject(hdc, oldPen);
    DeleteObject(sepPen);

    HFONT hFont   = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    HFONT oldFont = (HFONT)SelectObject(hdc, hFont);
    SetBkMode(hdc, TRANSPARENT);

    for (int i = 0; i < DIAMONDS_TAB_COUNT; ++i) {
        bool isActive = (i == g_DiamondsActiveTab);

        RECT tr = {
            band.left + i * DIAMONDS_TAB_WIDTH, band.top,
            band.left + i * DIAMONDS_TAB_WIDTH + DIAMONDS_TAB_WIDTH, band.bottom
        };

        // Active tab: same color as client area so it looks "open".
        // Inactive: a step darker than the band background.
        COLORREF tabBg = isActive
            ? (dark ? RGB(30, 30, 30) : RGB(255, 255, 255))
            : (dark ? RGB(40, 40, 44) : RGB(215, 215, 218));
        HBRUSH tabBr = CreateSolidBrush(tabBg);
        FillRect(hdc, &tr, tabBr);
        DeleteObject(tabBr);

        // Tab border: left, top, right.
        // Active tab has no bottom border — it merges with the client area.
        HPEN pen   = CreatePen(PS_SOLID, 1, sepCol);
        HPEN oldP2 = (HPEN)SelectObject(hdc, pen);
        int  bBot  = isActive ? tr.bottom : tr.bottom - 1;
        MoveToEx(hdc, tr.left,      bBot,    NULL);
        LineTo  (hdc, tr.left,      tr.top);
        LineTo  (hdc, tr.right - 1, tr.top);
        LineTo  (hdc, tr.right - 1, bBot);
        SelectObject(hdc, oldP2);
        DeleteObject(pen);

        // Label — high-contrast text regardless of dark/light mode.
        COLORREF txtCol = isActive
            ? (dark ? RGB(230, 230, 235) : RGB(10, 10, 10))
            : (dark ? RGB(140, 140, 145) : RGB(90, 90, 95));
        SetTextColor(hdc, txtCol);
        DrawTextA(hdc, g_DiamondTabNames[i], -1, &tr,
                  DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    SelectObject(hdc, oldFont);
    ReleaseDC(hWnd, hdc);
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
            // Strict single-tab membership:
            //   DTAB_ALL   → items NOT in g_DiamondsTabMap (unassigned)
            //   DTAB_WATCH → items mapped to DTAB_WATCH
            //   DTAB_DIV   → items mapped to DTAB_DIV
            auto it = g_DiamondsTabMap.find(info.conId);
            int  assignedTab = (it != g_DiamondsTabMap.end()) ? it->second : DTAB_ALL;
            if (assignedTab != g_DiamondsActiveTab) continue;
            rows.push_back(info);
        }
    }

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

    SetWindowTextA(hWnd, ("Diamonds: " + std::to_string(rows.size()) + " Positions").c_str());
}

// ── Window procedure ──────────────────────────────────────────────────────────

LRESULT CALLBACK WndProcDiamonds(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {

    // ── Non-client area: carve DIAMONDS_TAB_HEIGHT px below the caption ──────
    case WM_NCCALCSIZE: {
        LRESULT lr = DefWindowProc(hWnd, message, wParam, lParam);
        if (wParam) {
            // Push the client rect's top edge down to create the tab band.
            NCCALCSIZE_PARAMS* p = (NCCALCSIZE_PARAMS*)lParam;
            p->rgrc[0].top += DIAMONDS_TAB_HEIGHT;
        }
        return lr;
    }

    // Repaint tabs after any NC repaint (focus change, theme change, resize).
    // For WM_NCACTIVATE we always return TRUE so the caption redraws correctly
    // when the window gains/loses focus, then we overdraw with our tabs.
    case WM_NCPAINT: {
        LRESULT lr = DefWindowProc(hWnd, message, wParam, lParam);
        Diamonds_PaintTabs(hWnd);
        return lr;
    }
    case WM_NCACTIVATE: {
        LRESULT lr = DefWindowProc(hWnd, message, wParam, lParam);
        Diamonds_PaintTabs(hWnd);
        return TRUE;   // always repaint the NC area on focus change
    }

    // ── Non-client left-button: intercept tab clicks ──────────────────────
    case WM_NCLBUTTONDOWN: {
        // Convert screen coords to window coords and hit-test the tab band.
        POINT ptScreen = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        RECT  wr;  GetWindowRect(hWnd, &wr);
        POINT ptW  = { ptScreen.x - wr.left, ptScreen.y - wr.top };
        int   tab  = Diamonds_HitTestTab(hWnd, ptW);
        if (tab >= 0 && tab != g_DiamondsActiveTab) {
            g_DiamondsActiveTab = tab;
            Diamonds_PaintTabs(hWnd);
            Diamonds_Repopulate(hWnd);
            return 0;   // consumed — do NOT pass to DefWindowProc (avoids drag)
        }
        break;
    }

    case WM_CREATE: {
        HINSTANCE hInst = ((LPCREATESTRUCT)lParam)->hInstance;

        HWND hList = CreateWindowExA(
            WS_EX_CLIENTEDGE, "SysListView32", "",
            WS_CHILD | WS_VISIBLE | WS_BORDER |
            LVS_REPORT | LVS_SHOWSELALWAYS | LVS_NOSORTHEADER,
            0, 0, 1100, 420,
            hWnd, (HMENU)ID_DIAMONDS_RESULTS_LIST, hInst, NULL);

        DiamondsZoomData.fontSize = (int)Settings_Load(DiamondsZoomData.settingKey, DiamondsZoomData.fontSize);
        ApplyListViewFont(hList, DiamondsZoomData.hFont, DiamondsZoomData.fontSize);
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

        api.setDiamondsWindow(hWnd);
        api.addApiUpdateWindow(hWnd);

        // Load saved tab assignments from the registry.
        Diamonds_LoadTabMap();

        // Force WM_NCCALCSIZE to run now so the tab band is carved out before
        // the first paint.  Without this the ListView briefly fills the full
        // window height on startup.
        SetWindowPos(hWnd, NULL, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                     SWP_NOACTIVATE | SWP_FRAMECHANGED);
        break;
    }

    case WM_SIZE: {
        HWND hList = GetDlgItem(hWnd, ID_DIAMONDS_RESULTS_LIST);
        if (!hList) return 0;
        RECT rc; GetClientRect(hWnd, &rc);
        MoveWindow(hList, 0, 0, rc.right, rc.bottom, TRUE);
        // Repaint the NC tab band after maximize / restore.
        RedrawWindow(hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE | RDW_NOCHILDREN);
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

                // Read book lists fresh every time (user may have edited them).
                std::vector<std::string> bookLists = Book_LoadAllListNames();

                // Build context menu.
                // IDs 1-3: tab assignments.  IDs 100+: "Add to Book: <name>".
                HMENU hMenu = CreatePopupMenu();
                AppendMenuA(hMenu, MF_STRING | (g_DiamondsActiveTab == DTAB_ALL   ? MF_GRAYED : 0), 1, "Move to All");
                AppendMenuA(hMenu, MF_STRING | (g_DiamondsActiveTab == DTAB_WATCH ? MF_GRAYED : 0), 2, "Move to Watchlist");
                AppendMenuA(hMenu, MF_STRING | (g_DiamondsActiveTab == DTAB_DIV   ? MF_GRAYED : 0), 3, "Move to Dividends");

                if (!bookLists.empty()) {
                    AppendMenuA(hMenu, MF_SEPARATOR, 0, NULL);
                    for (int i = 0; i < (int)bookLists.size(); ++i) {
                        std::string label = "Add to Book: " + bookLists[i];
                        AppendMenuA(hMenu, MF_STRING, 100 + i, label.c_str());
                    }
                }

                POINT pt;
                GetCursorPos(&pt);
                int cmd = (int)TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_RIGHTBUTTON,
                                              pt.x, pt.y, 0, hWnd, NULL);
                DestroyMenu(hMenu);

                if (cmd >= 1 && cmd <= 3) {
                    // Tab assignment.
                    int targetTab = cmd - 1;
                    if (targetTab == DTAB_ALL)
                        g_DiamondsTabMap.erase(conId);
                    else
                        g_DiamondsTabMap[conId] = targetTab;
                    Diamonds_SaveTabMap();
                    Diamonds_Repopulate(hWnd);
                } else if (cmd >= 100 && cmd < 100 + (int)bookLists.size()) {
                    // Add to book list.
                    const std::string& listName = bookLists[cmd - 100];
                    // Look up the full PositionInfo so we can include the exchange,
                    // producing the same "conId.symbol.exchange" format that book.h uses.
                    std::string exchange;
                    {
                        std::lock_guard<std::mutex> lock(api.getPortfolioMutex());
                        auto& pmap = api.getPortfolioMap();
                        auto it = pmap.find(sym);
                        if (it != pmap.end())
                            exchange = it->second.exchange;
                    }
                    std::string entry = std::to_string(conId) + "." + sym;
                    if (!exchange.empty())
                        entry += "." + exchange;

                    auto entries = Book_ReadListEntries(listName.c_str());
                    // Only add if not already present.
                    bool exists = false;
                    for (const auto& e : entries)
                        if (e == entry) { exists = true; break; }
                    if (!exists) {
                        entries.push_back(entry);
                        Book_SaveFullList(listName.c_str(), entries);
                        Book_NotifyListChanged(listName);
                        Book_NotifyBookWindow(listName);
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
                    if (dark) {
                        cd->clrTextBk = (cd->nmcd.dwItemSpec % 2 == 0) ? DM_BG : DM_BG2;
                        cd->clrText   = DM_TEXT;
                    } else {
                        cd->clrTextBk = (cd->nmcd.dwItemSpec % 2 == 0) ? GetSysColor(COLOR_WINDOW) : RGB(245, 245, 245);
                        cd->clrText   = LM_TEXT;
                    }
                    return CDRF_NOTIFYSUBITEMDRAW;

                case CDDS_ITEMPREPAINT | CDDS_SUBITEM: {
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
        break;
    }

    return HandleCommonMessages(hWnd, message, wParam, lParam);
}
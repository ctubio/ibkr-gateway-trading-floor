#pragma once

void startDiamonds() { startGenericWindow(DIAMONDS_CLASS_NAME, "Diamonds", L"IBKRGatewayClient.Diamonds", 800, 500); }

// Table Columns
struct DiamondCol { const char* header; int width; };
static DiamondCol diamondCols[] = {
    { "Symbol", 100 },
    { "Shares", 80 },
    { "Avg Cost", 100 },
    { "Daily PnL", 100 },
    { "52W %", 80 },
    { "Mkt Value", 100 },
    { "Mkt Cap", 120 },
};
static const int DIAMOND_COL_COUNT = sizeof(diamondCols) / sizeof(diamondCols[0]);

static HWND hTabCtrl = NULL;
static HWND hPortfolioList = NULL;
static HWND hWatchlistList = NULL;
static HWND hBookCombo = NULL;

void UpdateDiamondTable(HWND hList, bool isPortfolio) {
    ListView_DeleteAllItems(hList);

    if (isPortfolio) {
        std::lock_guard<std::mutex> lock(api.getPortfolioMutex());
        int i = 0;
        for (auto const& [sym, info] : api.getPortfolioMap()) {
            LVITEM lvi = { 0 };
            lvi.mask = LVIF_TEXT;
            lvi.iItem = i;
            lvi.iSubItem = 0;
            lvi.pszText = (LPSTR)info.symbol.c_str();
            ListView_InsertItem(hList, &lvi);

            char buf[64];
            snprintf(buf, sizeof(buf), "%.2f", (double)info.shares);
            ListView_SetItemText(hList, i, 1, buf);
            snprintf(buf, sizeof(buf), "%.2f", info.avgCost);
            ListView_SetItemText(hList, i, 2, buf);
            snprintf(buf, sizeof(buf), "%.2f", info.dailyPnL);
            ListView_SetItemText(hList, i, 3, buf);
            snprintf(buf, sizeof(buf), "%.2f%%", info.fiftyTwoWeekChange);
            ListView_SetItemText(hList, i, 4, buf);
            snprintf(buf, sizeof(buf), "%.2f", info.marketValue);
            ListView_SetItemText(hList, i, 5, buf);
            snprintf(buf, sizeof(buf), "%.2f", info.marketCap);
            ListView_SetItemText(hList, i, 6, buf);
            i++;
        }
    } else {
        std::lock_guard<std::mutex> lock(api.getWatchlistMutex());
        int i = 0;
        for (auto const& [sym, info] : api.getWatchlistMap()) {
            LVITEM lvi = { 0 };
            lvi.mask = LVIF_TEXT;
            lvi.iItem = i;
            lvi.iSubItem = 0;
            lvi.pszText = (LPSTR)info.symbol.c_str();
            ListView_InsertItem(hList, &lvi);

            char buf[64];
            snprintf(buf, sizeof(buf), "%.2f", info.price);
            ListView_SetItemText(hList, i, 1, buf);
            snprintf(buf, sizeof(buf), "%.2f", info.change);
            ListView_SetItemText(hList, i, 2, buf);
            snprintf(buf, sizeof(buf), "%.2f%%", info.percentChange);
            ListView_SetItemText(hList, i, 3, buf);
            snprintf(buf, sizeof(buf), "%.2f", info.marketCap);
            ListView_SetItemText(hList, i, 4, buf);
            i++;
        }
    }
}

LRESULT CALLBACK WndProcDiamonds(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE: {
        HINSTANCE hInst = ((LPCREATESTRUCT)lParam)->hInstance;
        // Create a simple layout: two listviews (Portfolio, Watchlist)
        hTabCtrl = CreateWindowA("SysTabControl32", "", 
            WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS, 
            0, 0, 800, 28, 
            hWnd, (HMENU)1, hInst, NULL);

        // Portfolio list (left)
        hPortfolioList = CreateWindowExA(WS_EX_CLIENTEDGE, "SysListView32", NULL,
            WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS | WS_BORDER,
            8, 36, 380, 420,
            hWnd, (HMENU)1001, hInst, NULL);

        // Watchlist list (right)
        hWatchlistList = CreateWindowExA(WS_EX_CLIENTEDGE, "SysListView32", NULL,
            WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS | WS_BORDER,
            400, 36, 380, 420,
            hWnd, (HMENU)1002, hInst, NULL);

        // Ensure common control styles are set
        ListView_SetExtendedListViewStyle(hPortfolioList, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
        ListView_SetExtendedListViewStyle(hWatchlistList, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

        // Add columns to both lists
        for (int col = 0; col < DIAMOND_COL_COUNT; ++col) {
            LVCOLUMN lvc = { 0 };
            lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
            lvc.pszText = const_cast<char*>(diamondCols[col].header);
            lvc.cx = diamondCols[col].width;
            lvc.iSubItem = col;
            ListView_InsertColumn(hPortfolioList, col, &lvc);
            // Watchlist has fewer columns; insert up to DIAMOND_COL_COUNT as well
            ListView_InsertColumn(hWatchlistList, col, &lvc);
        }

        // Initial population
        UpdateDiamondTable(hPortfolioList, true);
        UpdateDiamondTable(hWatchlistList, false);
        break;
    }

    case WM_SIZE: {
        RECT rc;
        GetClientRect(hWnd, &rc);
        int w = rc.right - rc.left;
        int h = rc.bottom - rc.top;
        if (hTabCtrl) SetWindowPos(hTabCtrl, NULL, 0, 0, w, 28, SWP_NOZORDER);
        if (hPortfolioList) SetWindowPos(hPortfolioList, NULL, 8, 36, (w/2) - 16, h - 44, SWP_NOZORDER);
        if (hWatchlistList) SetWindowPos(hWatchlistList, NULL, (w/2) + 8, 36, (w/2) - 16, h - 44, SWP_NOZORDER);
        break;
    }

    case WM_DIAMONDS_UPDATE:
        UpdateDiamondTable(hPortfolioList, true);
        UpdateDiamondTable(hWatchlistList, false);
        break;

    case WM_DESTROY:
        break;
}
    return HandleCommonMessages(hWnd, message, wParam, lParam);
}

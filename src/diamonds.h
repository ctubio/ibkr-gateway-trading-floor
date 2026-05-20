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
        
        hTabCtrl = CreateWindowA("SysTabControl32", "", 
            WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS, 
            0, 0, 800, 500, 
            hWnd, (HMENU)1, hInst, NULL);
        
        // Simplified Tab implementation: we'll use a simple toggle or just the list for now
        // to avoid the complex TCFIELD/TCM_SETITEM setup which is causing errors.
        // We will implement the actual tab switching logic in a subsequent step.
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

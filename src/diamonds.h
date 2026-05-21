#pragma once

void startDiamonds() { startGenericWindow(DIAMONDS_CLASS_NAME, "Diamonds", L"IBKRGatewayClient.Diamonds", 700, 400); }

// ── Column definitions ────────────────────────────────────────────────────────

struct DiamondCol { const char* header; int width; int fmt; };
static const DiamondCol diamondCols[] = {
    { "Symbol",   100, LVCFMT_LEFT  },
    { "Shares",    80, LVCFMT_RIGHT },
    { "Avg Cost",  90, LVCFMT_RIGHT },
    { "Daily PnL", 90, LVCFMT_RIGHT },
    { "52W %",     75, LVCFMT_RIGHT },
    { "Mkt Value", 90, LVCFMT_RIGHT },
    { "Mkt Cap",  110, LVCFMT_RIGHT },
};
static const int DIAMOND_COL_COUNT = (int)(sizeof(diamondCols) / sizeof(diamondCols[0]));

// ── Repopulate ────────────────────────────────────────────────────────────────

static void Diamonds_Repopulate(HWND hList) {
    SendMessage(hList, WM_SETREDRAW, FALSE, 0);
    ListView_DeleteAllItems(hList);

    std::vector<TradingAPI::PositionInfo> rows;
    {
        std::lock_guard<std::mutex> lock(api.getPortfolioMutex());
        for (auto const& [sym, info] : api.getPortfolioMap())
            rows.push_back(info);
    }

    // Sort alphabetically by symbol
    std::sort(rows.begin(), rows.end(),
              [](const TradingAPI::PositionInfo& a, const TradingAPI::PositionInfo& b) {
                  return a.symbol < b.symbol;
              });

    char buf[64];
    for (int i = 0; i < (int)rows.size(); ++i) {
        const auto& info = rows[i];

        LVITEMA lvi = {};
        lvi.mask     = LVIF_TEXT;
        lvi.iItem    = i;
        lvi.iSubItem = 0;
        lvi.pszText  = (LPSTR)info.symbol.c_str();
        ListView_InsertItem(hList, &lvi);

        snprintf(buf, sizeof(buf), "%.2f", DecimalFunctions::decimalToDouble(info.shares));
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
    }

    SendMessage(hList, WM_SETREDRAW, TRUE, 0);
    RedrawWindow(hList, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
}

// ── Window procedure ──────────────────────────────────────────────────────────

LRESULT CALLBACK WndProcDiamonds(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {

    case WM_CREATE: {
        HINSTANCE hInst = ((LPCREATESTRUCT)lParam)->hInstance;

        HWND hList = CreateWindowExA(
            WS_EX_CLIENTEDGE, "SysListView32", "",
            WS_CHILD | WS_VISIBLE | WS_BORDER |
            LVS_REPORT | LVS_SHOWSELALWAYS | LVS_NOSORTHEADER,
            0, 0, 700, 400,
            hWnd, (HMENU)1, hInst, NULL);

        DWORD exStyle = LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER;
        if (Settings_DarkMode()) exStyle |= LVS_EX_GRIDLINES;
        ListView_SetExtendedListViewStyle(hList, exStyle);

        LVCOLUMNA lvc = {};
        lvc.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_FMT;
        for (int i = 0; i < DIAMOND_COL_COUNT; ++i) {
            lvc.cx      = diamondCols[i].width;
            lvc.pszText = (LPSTR)diamondCols[i].header;
            lvc.fmt     = diamondCols[i].fmt;
            ListView_InsertColumn(hList, i, &lvc);
        }

        api.setDiamondsWindow(hWnd);
        break;
    }

    case WM_SIZE: {
        HWND hList = GetDlgItem(hWnd, 1);
        if (!hList) return 0;
        RECT rc;
        GetClientRect(hWnd, &rc);
        MoveWindow(hList, 0, 0, rc.right, rc.bottom, TRUE);
        break;
    }

    case WM_DIAMONDS_UPDATE: {
        HWND hList = GetDlgItem(hWnd, 1);
        if (hList) Diamonds_Repopulate(hList);
        break;
    }

    // ── Dark mode ─────────────────────────────────────────────────────────────
    case WM_NOTIFY: {
        NMHDR* hdr = (NMHDR*)lParam;
        if (hdr->idFrom != 1) break;

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
                    }
                    return CDRF_NOTIFYSUBITEMDRAW;

                case CDDS_ITEMPREPAINT | CDDS_SUBITEM: {
                    if (cd->iSubItem == 3) { // Daily PnL — green/red
                        char buf[32] = {};
                        HWND hList = GetDlgItem(hWnd, 1);
                        ListView_GetItemText(hList, (int)cd->nmcd.dwItemSpec, 3, buf, sizeof(buf));
                        double val = atof(buf);
                        if (val > 0) cd->clrText = RGB(80, 200, 120);
                        else if (val < 0) cd->clrText = RGB(220, 80, 80);
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
        api.setDiamondsWindow(NULL);
        break;
    }

    return HandleCommonMessages(hWnd, message, wParam, lParam);
}
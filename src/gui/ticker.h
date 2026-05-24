#pragma once

void startTicker() { startGenericWindow(TICKER_CLASS_NAME, "Ticker", L"IBKRGatewayClient.Ticker", 560, 400); }

#define ID_TICKER_LIST_COMBO  8001
#define ID_TICKER_LIST        8002

static HWND hTickerListCombo = NULL;
static HWND hTickerList      = NULL;
static bool tickerSelectorsVisible = false;
static std::vector<std::string> tickerCurrentEntries; // entries currently subscribed

static const int TICKER_COMBO_H    = 24;
static const int TICKER_SELECTOR_H = 8 + TICKER_COMBO_H + 8;

// ── Column definitions ────────────────────────────────────────────────────────

struct TickerCol { const char* header; int width; int fmt; };
static const TickerCol tickerCols[] = {
    { "Symbol",   80, LVCFMT_LEFT  },
    { "Last",     75, LVCFMT_RIGHT },
    { "Change",   75, LVCFMT_RIGHT },
    { "Chg %",    70, LVCFMT_RIGHT },
    { "Volume",   90, LVCFMT_RIGHT },
    { "Bid",      75, LVCFMT_RIGHT },
    { "Ask",      75, LVCFMT_RIGHT },
    { "High",     75, LVCFMT_RIGHT },
    { "Low",      75, LVCFMT_RIGHT },
};
static const int TICKER_COL_COUNT = (int)(sizeof(tickerCols) / sizeof(tickerCols[0]));

// ── Helpers ───────────────────────────────────────────────────────────────────

static std::string Ticker_GetSelectedList() {
    int sel = (int)SendMessage(hTickerListCombo, CB_GETCURSEL, 0, 0);
    if (sel == CB_ERR) return "";
    int len = (int)SendMessage(hTickerListCombo, CB_GETLBTEXTLEN, sel, 0);
    if (len <= 0) return "";
    std::string name(len + 1, '\0');
    SendMessageA(hTickerListCombo, CB_GETLBTEXT, sel, (LPARAM)name.data());
    name.resize(len);
    return name;
}

struct TickerRowData { std::string symbol; int conId = 0; };
// Find a ListView row by its lParam (which stores symbol as std::string*).
// Returns the row index, or -1 if not found.
static int Ticker_FindRow(const std::string& symbol, int conId) {
    int count = ListView_GetItemCount(hTickerList);
    for (int i = 0; i < count; ++i) {
        LVITEMA lvi = {};
        lvi.mask  = LVIF_PARAM;
        lvi.iItem = i;
        SendMessageA(hTickerList, LVM_GETITEMA, 0, (LPARAM)&lvi);
        auto* rd = reinterpret_cast<TickerRowData*>(lvi.lParam);
        if (rd && rd->symbol == symbol && rd->conId == conId) return i;
    }
    return -1;
}


// Free all lParam TickerRowData structs and delete all rows.
static void Ticker_ClearList() {
    if (!hTickerList) return;
    int count = ListView_GetItemCount(hTickerList);
    for (int i = 0; i < count; ++i) {
        LVITEMA lvi = {};
        lvi.mask  = LVIF_PARAM;
        lvi.iItem = i;
        SendMessageA(hTickerList, LVM_GETITEMA, 0, (LPARAM)&lvi);
        delete reinterpret_cast<TickerRowData*>(lvi.lParam);
    }
    ListView_DeleteAllItems(hTickerList);
}

// Insert a placeholder row for symbol+conId.
static void Ticker_InsertRow(const std::string& symbol, int conId) {
    auto* rd    = new TickerRowData{symbol, conId};
    LVITEMA lvi = {};
    lvi.mask    = LVIF_TEXT | LVIF_PARAM;
    lvi.iItem   = ListView_GetItemCount(hTickerList);
    lvi.lParam  = (LPARAM)rd;
    lvi.pszText = (LPSTR)symbol.c_str();
    SendMessageA(hTickerList, LVM_INSERTITEMA, 0, (LPARAM)&lvi);
}

// Update a row with the latest TickerInfo data.
static void Ticker_UpdateRow(int row, const TradingAPI::TickerInfo& info) {
    char buf[32];

    auto fmt = [&](double v, const char* fmt_str) -> const char* {
        if (v == 0.0) { buf[0] = '\0'; } else { snprintf(buf, sizeof(buf), fmt_str, v); }
        return buf;
    };

    ListView_SetItemText(hTickerList, row, 1, (LPSTR)fmt(info.last,      "%.2f"));
    ListView_SetItemText(hTickerList, row, 2, (LPSTR)fmt(info.change(),  "%.2f"));
    ListView_SetItemText(hTickerList, row, 3, (LPSTR)fmt(info.changePct(),"%.2f%%"));

    if (info.volume > 0) snprintf(buf, sizeof(buf), "%lld", info.volume);
    else buf[0] = '\0';
    ListView_SetItemText(hTickerList, row, 4, buf);

    ListView_SetItemText(hTickerList, row, 5, (LPSTR)fmt(info.bid,  "%.2f"));
    ListView_SetItemText(hTickerList, row, 6, (LPSTR)fmt(info.ask,  "%.2f"));
    ListView_SetItemText(hTickerList, row, 7, (LPSTR)fmt(info.high, "%.2f"));
    ListView_SetItemText(hTickerList, row, 8, (LPSTR)fmt(info.low,  "%.2f"));
}

// ── Layout ────────────────────────────────────────────────────────────────────

static void Ticker_Layout(HWND hWnd) {
    if (!hTickerList) return;
    RECT rc; GetClientRect(hWnd, &rc);
    const int m = 8;

    int availH    = tickerSelectorsVisible ? rc.bottom - TICKER_SELECTOR_H : rc.bottom;
    int selectorY = rc.bottom - TICKER_SELECTOR_H;

    MoveWindow(hTickerList, 0, 0, rc.right, availH, TRUE);

    if (tickerSelectorsVisible) {
        int comboW = rc.right - m * 2;
        int comboY = selectorY + (TICKER_SELECTOR_H - TICKER_COMBO_H) / 2;
        SetWindowPos(hTickerListCombo, NULL, m, comboY, comboW, 200,
                     SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

static void Ticker_ShowSelectors(HWND hWnd, bool show) {
    if (tickerSelectorsVisible == show) return;
    tickerSelectorsVisible = show;
    ShowWindow(hTickerListCombo, show ? SW_SHOW : SW_HIDE);
    Ticker_Layout(hWnd);
}

// ── Subscribe ─────────────────────────────────────────────────────────────────

static void Ticker_Subscribe(HWND hWnd, const std::string& listName) {
    if (listName.empty()) return;
    Settings_SaveString("LastTickerList", listName);

    auto entries = Book_ReadListEntries(listName.c_str());
    tickerCurrentEntries = entries;

    Ticker_ClearList();

    // Insert placeholder rows immediately so the user sees the symbols.
    for (const auto& entry : entries) {
        auto d1 = entry.find('.');
        if (d1 == std::string::npos) continue;
        std::string rest = entry.substr(d1 + 1);
        auto d2 = rest.find('.');
        std::string symbol = (d2 != std::string::npos) ? rest.substr(0, d2) : rest;
        int conId = std::stoi(entry.substr(0, d1));
        LogDebug("Symbol is: " + symbol + ", conId: " + std::to_string(conId));
        Ticker_InsertRow(symbol, conId);
    }

    SetWindowTextA(hWnd, ("Ticker: " + listName).c_str());
    api.setTickerWindow(hWnd, entries);
}

// ── Window procedure ──────────────────────────────────────────────────────────

LRESULT CALLBACK WndProcTicker(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {

    case WM_CREATE: {
        HINSTANCE hInst = ((LPCREATESTRUCT)lParam)->hInstance;
        const int m = 8;

        // List first — lower Z-order, behind the combo.
        hTickerList = CreateWindowExA(
            WS_EX_CLIENTEDGE, "SysListView32", "",
            WS_CHILD | WS_VISIBLE | WS_BORDER |
            LVS_REPORT | LVS_SHOWSELALWAYS | LVS_NOSORTHEADER,
            0, 0, 560, 400,
            hWnd, (HMENU)ID_TICKER_LIST, hInst, NULL);

        DWORD exStyle = LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER;
        if (Settings_DarkMode()) exStyle |= LVS_EX_GRIDLINES;
        ListView_SetExtendedListViewStyle(hTickerList, exStyle);

        LVCOLUMNA lvc = {};
        lvc.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_FMT;
        for (int i = 0; i < TICKER_COL_COUNT; ++i) {
            lvc.cx      = tickerCols[i].width;
            lvc.pszText = (LPSTR)tickerCols[i].header;
            lvc.fmt     = tickerCols[i].fmt;
            ListView_InsertColumn(hTickerList, i, &lvc);
        }

        // Combo on top (higher Z-order), hidden until focused.
        hTickerListCombo = CreateWindowA("COMBOBOX", NULL,
            WS_CHILD | WS_VSCROLL | CBS_DROPDOWNLIST,
            m, m, 200, 200,
            hWnd, (HMENU)ID_TICKER_LIST_COMBO, hInst, NULL);

        Book_LoadAllLists(hTickerListCombo);

        // Restore last session.
        std::string lastList = Settings_LoadString("LastTickerList");
        if (!lastList.empty()) {
            int idx = (int)SendMessageA(hTickerListCombo, CB_FINDSTRINGEXACT, -1, (LPARAM)lastList.c_str());
            if (idx != CB_ERR) {
                SendMessage(hTickerListCombo, CB_SETCURSEL, idx, 0);
                Ticker_Subscribe(hWnd, lastList);
            }
        }

        api.addApiUpdateWindow(hWnd);
        break;
    }

    case WM_SIZE:
        Ticker_Layout(hWnd);
        return 0;

    case WM_ACTIVATE:
        Ticker_ShowSelectors(hWnd, LOWORD(wParam) != WA_INACTIVE);
        return 0;

    case WM_COMMAND:
        if (LOWORD(wParam) == ID_TICKER_LIST_COMBO && HIWORD(wParam) == CBN_SELCHANGE)
            Ticker_Subscribe(hWnd, Ticker_GetSelectedList());
        break;

    case WM_TICKER_UPDATE: {
        auto* key = reinterpret_cast<std::string*>(lParam);
        if (!key) break;
        // key format: "conId.symbol"
        auto dot = key->find('.');
        if (dot != std::string::npos) {
            int conId          = std::stoi(key->substr(0, dot));
            std::string symbol = key->substr(dot + 1);
            TradingAPI::TickerInfo info;
            if (api.getTickerData(conId, symbol, info)) {
                int row = Ticker_FindRow(symbol, conId);
                if (row >= 0) Ticker_UpdateRow(row, info);
            }
        }
        delete key;
        break;
    }

    case WM_API_UPDATE:
        if (api.isMarketDataConnected() && api.isTradingConnected()) {
            // Re-subscribe after reconnect with the same entries.
            api.reqWatchlist();
        } else {
            // Clear data but keep the symbol rows visible (just blank the values).
            int count = ListView_GetItemCount(hTickerList);
            for (int i = 0; i < count; ++i)
                for (int col = 1; col < TICKER_COL_COUNT; ++col)
                    ListView_SetItemText(hTickerList, i, col, (LPSTR)"");
        }
        break;

    case WM_NOTIFY: {
        NMHDR* hdr = (NMHDR*)lParam;
        if (hdr->idFrom != ID_TICKER_LIST) break;
        if (hdr->code == NM_DBLCLK) {
            LPNMITEMACTIVATE act = (LPNMITEMACTIVATE)lParam;
            int row = act->iItem;
            if (row >= 0) {
                LVITEMA lvi = {};
                lvi.mask  = LVIF_PARAM;
                lvi.iItem = row;
                SendMessageA(hTickerList, LVM_GETITEMA, 0, (LPARAM)&lvi);
                auto* rd = reinterpret_cast<TickerRowData*>(lvi.lParam);
                if (rd) {
                    char msg[256];
                    snprintf(msg, sizeof(msg), "conId: %d\nSymbol: %s", rd->conId, rd->symbol.c_str());
                    MessageBoxA(hWnd, msg, "NM_DBLCLK", MB_OK);
                }
            }
        }
        if (hdr->code == NM_CUSTOMDRAW) {
            NMLVCUSTOMDRAW* cd = (NMLVCUSTOMDRAW*)lParam;
            bool dark = Settings_DarkMode();
            switch (cd->nmcd.dwDrawStage) {
                case CDDS_PREPAINT: return CDRF_NOTIFYITEMDRAW;
                case CDDS_ITEMPREPAINT:
                    if (dark) {
                        cd->clrTextBk = (cd->nmcd.dwItemSpec % 2 == 0) ? DM_BG : DM_BG2;
                        cd->clrText   = DM_TEXT;
                    }
                    return CDRF_NOTIFYSUBITEMDRAW;
                case CDDS_ITEMPREPAINT | CDDS_SUBITEM: {
                    if (cd->iSubItem == 2 || cd->iSubItem == 3) {
                        // Change / %Change — colour green or red
                        char buf[32] = {};
                        ListView_GetItemText(hTickerList, (int)cd->nmcd.dwItemSpec, 2, buf, sizeof(buf));
                        double v = atof(buf);
                        if (v > 0)      cd->clrText = RGB(80, 200, 120);
                        else if (v < 0) cd->clrText = RGB(220, 80, 80);
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
        api.unsetTickerWindow();
        api.removeApiUpdateWindow(hWnd);
        Ticker_ClearList(); // frees lParam TickerRowData structs
        hTickerListCombo = NULL;
        hTickerList      = NULL;
        tickerCurrentEntries.clear();
        break;
    }

    return HandleCommonMessages(hWnd, message, wParam, lParam);
}
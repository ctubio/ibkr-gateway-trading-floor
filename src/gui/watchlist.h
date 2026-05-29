#pragma once

void StartWatchlist() { StartGenericWindow(WATCHLIST_CLASS_NAME, "Watchlist", L"IBKRGatewayClient.Watchlist", 1000, 400); }

#define ID_WATCHLIST_LIST_COMBO  8001
#define ID_WATCHLIST_LIST        8002

static bool watchlistSelectorsVisible = false;
static std::vector<std::string> watchlistCurrentEntries; // entries currently subscribed
static std::string watchlistCurrentListName;             // name of the currently subscribed list

static const int WATCHLIST_COMBO_H    = 24;
static const int WATCHLIST_SELECTOR_H = 8 + WATCHLIST_COMBO_H + 8;

static ListViewZoomData WatchlistZoomData = { NULL, 14, "WatchlistListZoom" };

// ── Column definitions ────────────────────────────────────────────────────────

enum WatchlistColIdx {
    TCOL_SYMBOL = 0,
    TCOL_LAST,
    TCOL_CHGPCT,
    TCOL_BID,
    TCOL_BIDSIZE,
    TCOL_ASK,
    TCOL_ASKSIZE,
    TCOL_DIV_YIELD,
    TCOL_DIV_DATE,
    TCOL_DIV_AMT,
    TCOL_ANNUAL_DIV,
    TCOL_COUNT
};

struct WatchlistCol { const char* header; int width; int fmt; };
static const WatchlistCol watchlistCols[] = {
    { "Instrument",   90, LVCFMT_LEFT  },
    { "Last",         75, LVCFMT_RIGHT },
    { "Change %",     75, LVCFMT_RIGHT },
    { "Bid",          75, LVCFMT_RIGHT },
    { "Bid Size",     75, LVCFMT_RIGHT },
    { "Ask",          75, LVCFMT_RIGHT },
    { "Ask Size",     75, LVCFMT_RIGHT },
    { "Div Yield %",  80, LVCFMT_RIGHT },
    { "Div Date",     85, LVCFMT_RIGHT },
    { "Div Amount",   80, LVCFMT_RIGHT },
    { "Annual Div",   80, LVCFMT_RIGHT },
};

// Sentinel string displayed whenever a value cannot be computed (e.g. market closed, last == 0).
static const char* WATCHLIST_NO_DATA = "--";

// ── Helpers ───────────────────────────────────────────────────────────────────

static std::string Watchlist_GetSelectedList(HWND hWnd) {
    HWND hWatchlistListCombo = GetDlgItem(hWnd, ID_WATCHLIST_LIST_COMBO);
    int sel = (int)SendMessage(hWatchlistListCombo, CB_GETCURSEL, 0, 0);
    if (sel == CB_ERR) return "";
    int len = (int)SendMessage(hWatchlistListCombo, CB_GETLBTEXTLEN, sel, 0);
    if (len <= 0) return "";
    std::string name(len + 1, '\0');
    SendMessageA(hWatchlistListCombo, CB_GETLBTEXT, sel, (LPARAM)name.data());
    name.resize(len);
    return name;
}

struct WatchlistRowData { std::string symbol; int conId = 0; };

// Find a ListView row by its lParam (which stores symbol+conId as WatchlistRowData*).
// Returns the row index, or -1 if not found.
static int Watchlist_FindRow(HWND hWnd, const std::string& symbol, int conId) {
    HWND hWatchlistList = GetDlgItem(hWnd, ID_WATCHLIST_LIST);
    int count = ListView_GetItemCount(hWatchlistList);
    for (int i = 0; i < count; ++i) {
        LVITEMA lvi = {};
        lvi.mask  = LVIF_PARAM;
        lvi.iItem = i;
        SendMessageA(hWatchlistList, LVM_GETITEMA, 0, (LPARAM)&lvi);
        auto* rd = reinterpret_cast<WatchlistRowData*>(lvi.lParam);
        if (rd && rd->symbol == symbol && rd->conId == conId) return i;
    }
    return -1;
}

// Free all lParam WatchlistRowData structs and delete all rows.
static void Watchlist_ClearList(HWND hWnd) {
    HWND hWatchlistList = GetDlgItem(hWnd, ID_WATCHLIST_LIST);
    if (!hWatchlistList) return;
    int count = ListView_GetItemCount(hWatchlistList);
    for (int i = 0; i < count; ++i) {
        LVITEMA lvi = {};
        lvi.mask  = LVIF_PARAM;
        lvi.iItem = i;
        SendMessageA(hWatchlistList, LVM_GETITEMA, 0, (LPARAM)&lvi);
        delete reinterpret_cast<WatchlistRowData*>(lvi.lParam);
    }
    ListView_DeleteAllItems(hWatchlistList);
}

// Insert a placeholder row for symbol+conId.
static void Watchlist_InsertRow(HWND hWnd, const std::string& symbol, int conId) {
    HWND hWatchlistList = GetDlgItem(hWnd, ID_WATCHLIST_LIST);
    auto* rd    = new WatchlistRowData{symbol, conId};
    LVITEMA lvi = {};
    lvi.mask    = LVIF_TEXT | LVIF_PARAM;
    lvi.iItem   = ListView_GetItemCount(hWatchlistList);
    lvi.lParam  = (LPARAM)rd;
    lvi.pszText = (LPSTR)symbol.c_str();
    SendMessageA(hWatchlistList, LVM_INSERTITEMA, 0, (LPARAM)&lvi);
}

// Update a row with the latest WatchlistInfo data.
// All cells derived from Last price show "--" when Last == 0 (market closed),
// preventing bogus -100% change readings and misleading empty-string cells.
static void Watchlist_UpdateRow(HWND hWnd, int row, const TradingAPI::WatchlistInfo& info) {
    HWND hWatchlistList = GetDlgItem(hWnd, ID_WATCHLIST_LIST);
    char buf[32];

    // Helper: format a non-zero double into buf and return buf; otherwise blank.
    auto fmt = [&](double v, const char* fmt_str) -> const char* {
        if (v == 0.0) { buf[0] = '\0'; }
        else          { snprintf(buf, sizeof(buf), fmt_str, v); }
        return buf;
    };

    // ── Columns independent of Last ───────────────────────────────────────────
    ListView_SetItemText(hWatchlistList, row, TCOL_BID,        (LPSTR)fmt(info.bid,     "%.2f"));
    ListView_SetItemText(hWatchlistList, row, TCOL_BIDSIZE,    (LPSTR)fmt(info.bidSize, "%.0f"));
    ListView_SetItemText(hWatchlistList, row, TCOL_ASK,        (LPSTR)fmt(info.ask,     "%.2f"));
    ListView_SetItemText(hWatchlistList, row, TCOL_ASKSIZE,    (LPSTR)fmt(info.askSize, "%.0f"));

    // Dividend yield requires Last to be > 0, but the function itself guards
    // for that internally (returns 0 when last <= 0); show blank rather than "--".
    ListView_SetItemText(hWatchlistList, row, TCOL_DIV_YIELD,  (LPSTR)fmt(info.dividendYield(), "%.2f%%"));
    ListView_SetItemText(hWatchlistList, row, TCOL_DIV_DATE,   (LPSTR)info.dividendDate.c_str());
    ListView_SetItemText(hWatchlistList, row, TCOL_DIV_AMT,    (LPSTR)fmt(info.dividendAmount,  "%.3f"));
    ListView_SetItemText(hWatchlistList, row, TCOL_ANNUAL_DIV, (LPSTR)fmt(info.annualDividends, "%.3f"));

    // ── Columns that require Last > 0 ────────────────────────────────────────
    // When the market is closed, reqMktData returns Last == 0.
    // Display "--" for Last and all derived columns so the user sees clearly
    // that no live price is available, and so the custom-draw colour logic
    // does not attempt to parse a numeric value from an empty or zero string.
    if (info.last <= 0.0) {
        ListView_SetItemText(hWatchlistList, row, TCOL_LAST,   (LPSTR)WATCHLIST_NO_DATA);
        ListView_SetItemText(hWatchlistList, row, TCOL_CHGPCT, (LPSTR)WATCHLIST_NO_DATA);
        return;
    }

    // Last is valid — compute derived values normally.
    ListView_SetItemText(hWatchlistList, row, TCOL_LAST, (LPSTR)fmt(info.last, "%.2f"));

    // Change % is only meaningful when prevClose is available.
    if (info.prevClose > 0.0) {
        ListView_SetItemText(hWatchlistList, row, TCOL_CHGPCT,
                             (LPSTR)fmt(info.changePct(), "%+.2f%%"));
    } else {
        ListView_SetItemText(hWatchlistList, row, TCOL_CHGPCT, (LPSTR)WATCHLIST_NO_DATA);
    }
}

// ── Layout ────────────────────────────────────────────────────────────────────

static void Watchlist_Layout(HWND hWnd) {
    HWND hWatchlistList = GetDlgItem(hWnd, ID_WATCHLIST_LIST);
    if (!hWatchlistList) return;
    RECT rc; GetClientRect(hWnd, &rc);
    const int m = 8;

    int availH    = watchlistSelectorsVisible ? rc.bottom - WATCHLIST_SELECTOR_H : rc.bottom;
    int selectorY = rc.bottom - WATCHLIST_SELECTOR_H;

    MoveWindow(hWatchlistList, 0, 0, rc.right, availH, TRUE);

    if (watchlistSelectorsVisible) {
        int comboW = rc.right - m * 2;
        int comboY = selectorY + (WATCHLIST_SELECTOR_H - WATCHLIST_COMBO_H) / 2;
        SetWindowPos(GetDlgItem(hWnd, ID_WATCHLIST_LIST_COMBO), NULL, m, comboY, comboW, 200,
                     SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

static void Watchlist_ShowSelectors(HWND hWnd, bool show) {
    if (watchlistSelectorsVisible == show) return;
    watchlistSelectorsVisible = show;
    ShowWindow(GetDlgItem(hWnd, ID_WATCHLIST_LIST_COMBO), show ? SW_SHOW : SW_HIDE);
    Watchlist_Layout(hWnd);
}

// ── Subscribe ─────────────────────────────────────────────────────────────────

static void Watchlist_Subscribe(HWND hWnd, const std::string& listName) {
    if (listName.empty()) return;
    Settings_SaveString("LastWatchlistList", listName);
    watchlistCurrentListName = listName;

    auto entries = Book_ReadListEntries(listName.c_str());
    watchlistCurrentEntries = entries;

    Watchlist_ClearList(hWnd);

    // Insert placeholder rows immediately so the user sees the symbols.
    for (const auto& entry : entries) {
        auto d1 = entry.find('.');
        if (d1 == std::string::npos) continue;
        std::string rest = entry.substr(d1 + 1);
        auto d2 = rest.find('.');
        std::string symbol = (d2 != std::string::npos) ? rest.substr(0, d2) : rest;
        int conId = std::stoi(entry.substr(0, d1));
        Watchlist_InsertRow(hWnd, symbol, conId);
    }

    SetWindowTextA(hWnd, ("Watchlist: " + listName).c_str());
    api.setWatchlistWindow(hWnd, entries);
}

// ── Window procedure ──────────────────────────────────────────────────────────

LRESULT CALLBACK WndProcWatchlist(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {

    case WM_CREATE: {
        HINSTANCE hInst = ((LPCREATESTRUCT)lParam)->hInstance;
        const int m = 8;

        // List first — lower Z-order, behind the combo.
        HWND hWatchlistList = CreateWindowExA(
            WS_EX_CLIENTEDGE, "SysListView32", "",
            WS_CHILD | WS_VISIBLE | WS_BORDER |
            LVS_REPORT | LVS_SHOWSELALWAYS | LVS_NOSORTHEADER,
            0, 0, 1000, 400,
            hWnd, (HMENU)ID_WATCHLIST_LIST, hInst, NULL);

        WatchlistZoomData.fontSize = (int)Settings_Load(WatchlistZoomData.settingKey, WatchlistZoomData.fontSize);
        ApplyListViewFont(hWatchlistList, WatchlistZoomData.hFont, WatchlistZoomData.fontSize);
        SetWindowSubclass(hWatchlistList, ListViewZoomProc, 0, (DWORD_PTR)&WatchlistZoomData);

        ListView_SetExtendedListViewStyle(hWatchlistList, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

        LVCOLUMNA lvc = {};
        lvc.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_FMT;
        for (int i = 0; i < TCOL_COUNT; ++i) {
            lvc.cx      = watchlistCols[i].width;
            lvc.pszText = (LPSTR)watchlistCols[i].header;
            lvc.fmt     = watchlistCols[i].fmt;
            ListView_InsertColumn(hWatchlistList, i, &lvc);
        }

        // Combo on top (higher Z-order), hidden until focused.
        HWND hWatchlistListCombo = CreateWindowA("COMBOBOX", NULL,
            WS_CHILD | WS_VSCROLL | CBS_DROPDOWNLIST,
            m, m, 200, 200,
            hWnd, (HMENU)ID_WATCHLIST_LIST_COMBO, hInst, NULL);

        Book_LoadAllLists(hWatchlistListCombo);

        // Restore last session.
        std::string lastList = Settings_LoadString("LastWatchlistList");
        if (!lastList.empty()) {
            int idx = (int)SendMessageA(hWatchlistListCombo, CB_FINDSTRINGEXACT, -1, (LPARAM)lastList.c_str());
            if (idx != CB_ERR) {
                SendMessage(hWatchlistListCombo, CB_SETCURSEL, idx, 0);
                Watchlist_Subscribe(hWnd, lastList);
            }
        }

        api.addApiUpdateWindow(hWnd);
        break;
    }

    case WM_SIZE:
        Watchlist_Layout(hWnd);
        return 0;

    case WM_ACTIVATE:
        Watchlist_ShowSelectors(hWnd, LOWORD(wParam) != WA_INACTIVE);
        return 0;

    case WM_COMMAND:
        if (LOWORD(wParam) == ID_WATCHLIST_LIST_COMBO && HIWORD(wParam) == CBN_SELCHANGE)
            Watchlist_Subscribe(hWnd, Watchlist_GetSelectedList(hWnd));
        break;

    // ── Live or historical market data update for one symbol ──────────────────
    // lParam = new std::string("conId.symbol") — we own it and must delete.
    case WM_WATCHLIST_UPDATE: {
        auto* key = reinterpret_cast<std::string*>(lParam);
        if (!key) break;
        // key format: "conId.symbol"
        auto dot = key->find('.');
        if (dot != std::string::npos) {
            int conId          = std::stoi(key->substr(0, dot));
            std::string symbol = key->substr(dot + 1);
            TradingAPI::WatchlistInfo info;
            if (api.getWatchlistData(conId, symbol, info)) {
                int row = Watchlist_FindRow(hWnd, symbol, conId);
                if (row >= 0) Watchlist_UpdateRow(hWnd, row, info);
            }
        }
        delete key;
        break;
    }

    // ── Book list changed notification ────────────────────────────────────────
    // Sent by Book_NotifyListChanged whenever a list is mutated (add/delete/sort).
    // lParam = new std::string*(listName) — we own and must delete.
    case WM_BOOK_LIST_CHANGED: {
        auto* name = reinterpret_cast<std::string*>(lParam);
        if (name) {
            if (*name == watchlistCurrentListName)
                Watchlist_Subscribe(hWnd, *name);
            delete name;
        }
        return 0;
    }

    case WM_API_UPDATE:
        if (api.isMarketDataConnected() && api.isTradingConnected()) {
            // Re-subscribe after reconnect with the same entries.
            api.reqWatchlist();
        } else {
            // Clear data but keep the symbol rows visible (just blank the values).
            HWND hWatchlistList = GetDlgItem(hWnd, ID_WATCHLIST_LIST);
            int count = ListView_GetItemCount(hWatchlistList);
            for (int i = 0; i < count; ++i)
                for (int col = 1; col < TCOL_COUNT; ++col)
                    ListView_SetItemText(hWatchlistList, i, col, (LPSTR)"");
        }
        break;

    case WM_NOTIFY: {
        NMHDR* hdr = (NMHDR*)lParam;
        if (hdr->idFrom != ID_WATCHLIST_LIST) break;
        if (hdr->code == NM_DBLCLK) {
            LPNMITEMACTIVATE act = (LPNMITEMACTIVATE)lParam;
            int row = act->iItem;
            if (row >= 0) {
                LVITEMA lvi = {};
                lvi.mask  = LVIF_PARAM;
                lvi.iItem = row;
                SendMessageA(GetDlgItem(hWnd, ID_WATCHLIST_LIST), LVM_GETITEMA, 0, (LPARAM)&lvi);
                auto* rd = reinterpret_cast<WatchlistRowData*>(lvi.lParam);
                if (rd) {
                    StartMarket(rd->symbol, rd->conId);
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
                    if (cd->iSubItem == TCOL_CHGPCT) {
                        char buf[32] = {};
                        ListView_GetItemText(GetDlgItem(hWnd, ID_WATCHLIST_LIST),
                                            (int)cd->nmcd.dwItemSpec, TCOL_CHGPCT,
                                            buf, sizeof(buf));

                        // Guard: only colour numeric cells — skip "--" sentinel
                        // and empty strings to avoid atof("--") == 0.0 masking
                        // a missing-data cell as a neutral (uncoloured) number.
                        if (strcmp(buf, WATCHLIST_NO_DATA) != 0 && buf[0] != '\0') {
                            double v = atof(buf);
                            if      (v > 0.0) cd->clrText = RGB(80, 200, 120);
                            else if (v < 0.0) cd->clrText = RGB(220, 80, 80);
                        }
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
        api.unsetWatchlistWindow();
        api.removeApiUpdateWindow(hWnd);
        Watchlist_ClearList(hWnd); // frees lParam WatchlistRowData structs
        watchlistCurrentEntries.clear();
            if (WatchlistZoomData.hFont) {
                DeleteObject(WatchlistZoomData.hFont);
            }
        break;
    }

    return HandleCommonMessages(hWnd, message, wParam, lParam);
}
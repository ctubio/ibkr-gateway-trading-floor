#pragma once

void StartWatchlist() { StartGenericWindow(WATCHLIST_CLASS_NAME, "Watchlist", L"TWSAPIClientTradingFloor.Watchlist", 1000, 400); }


#define ID_WATCHLIST_LIST_COMBO       8001
#define ID_WATCHLIST_LIST             8002
#define ID_WATCHLIST_EDIT             8003
#define ID_WATCHLIST_NEW_SYMBOL_INPUT 8004

#define TIMER_WL_DROPDOWN        98


#define WM_WATCHLIST_NEW_LIST_START  (WM_APP + 200)
#define WM_WATCHLIST_LIST_CHANGED    (WM_APP + 201)
// Posted Watchlist when the set of lists changes (create/delete).
// lParam = new std::string*(newListName, or "" if a list was deleted) — receiver must delete.
#define WM_WATCHLIST_LISTS_CHANGED   (WM_APP + 203)

static bool watchlistSelectorsVisible = false;
static std::vector<std::string> watchlistCurrentEntries;
static std::string watchlistCurrentListName;

// Autocomplete state
static bool          wl_suppressSearch = false;
static bool          wl_showingOffline = false;
static std::string   wl_pendingFullEntry;
static std::vector<std::string> wl_currentResults;

static const int WATCHLIST_COMBO_H    = 24;
static const int WATCHLIST_SELECTOR_H = 8 + WATCHLIST_COMBO_H + 8;

static ListViewZoomData WatchlistZoomData = { NULL, NULL, 14, "Zoom_Watchlist" };
static ListViewZoomData WatchlistAutocompleteZoomData = { NULL, NULL, 14, "Zoom_Watchlist" };

// ── Column definitions ────────────────────────────────────────────────────────

enum WatchlistColIdx {
    TCOL_SYMBOL = 0,
    TCOL_ASKSIZE,
    TCOL_ASK,
    TCOL_LAST,
    TCOL_BID,
    TCOL_BIDSIZE,
    TCOL_CHGPCT,
    TCOL_DIV_YIELD,
    TCOL_DIV_DATE,
    TCOL_DIV_AMT,
    TCOL_ANNUAL_DIV,
    TCOL_COUNT
};

// ── Sort state ────────────────────────────────────────────────────────────────
static int  g_WatchlistSortCol = TCOL_SYMBOL;
static bool g_WatchlistSortAsc = true;
static HWND g_WatchlistListForSort = NULL;

struct WatchlistCol { const char* header; int width; int fmt; };
static const WatchlistCol watchlistCols[] = {
    { "Instrument",   90, LVCFMT_LEFT  },
    { "Ask Size",     75, LVCFMT_RIGHT },
    { "Ask",          75, LVCFMT_RIGHT },
    { "Last",         75, LVCFMT_RIGHT },
    { "Bid",          75, LVCFMT_RIGHT },
    { "Bid Size",     75, LVCFMT_RIGHT },
    { "Change %",     75, LVCFMT_RIGHT },
    { "Div Yield %",  80, LVCFMT_RIGHT },
    { "Div Date",     85, LVCFMT_RIGHT },
    { "Div Amount",   80, LVCFMT_RIGHT },
    { "Annual Div",   80, LVCFMT_RIGHT },
};

// Sentinel string displayed whenever a value cannot be computed (e.g. market closed, last == 0).
static const char* WATCHLIST_NO_DATA = "--";

// ── Helpers ───────────────────────────────────────────────────────────────────

// Notifies the Watchlist window (if open) that a watchlist list's contents changed.
// The watchlist will re-subscribe if it is currently showing that list.
void Watchlist_NotifyListChanged(const std::string& listName) {
    HWND hWL = FindWindowA(WATCHLIST_CLASS_NAME, NULL);
    if (hWL)
        PostMessage(hWL, WM_WATCHLIST_LIST_CHANGED, 0, (LPARAM)new std::string(listName));
}

std::string Watchlist_DisplayLabel(const std::string& entry) {
    // Format: conId.symbol.exchange — skip the conId part
    auto first = entry.find('.');
    if (first == std::string::npos) return entry;
    return entry.substr(first + 1); // returns "symbol.exchange"
}

void Watchlist_LoadAllLists(HWND hCB) {
    SendMessage(hCB, CB_RESETCONTENT, 0, 0);
    for (const auto& name : Watchlist_LoadAllListNames())
        SendMessageA(hCB, CB_ADDSTRING, 0, (LPARAM)name.c_str());
    if (SendMessage(hCB, CB_GETCOUNT, 0, 0) > 0)
        SendMessage(hCB, CB_SETCURSEL, 0, 0);
}

// Returns the real watchlist list name currently selected in the combo,
// or "" if nothing / the sentinel ("── Create new Watchlist ──") is selected.
// Index 0 is always the sentinel, indices 1..N are real lists.
static std::string Watchlist_GetSelectedList(HWND hWnd) {
    HWND hCB = GetDlgItem(hWnd, ID_WATCHLIST_LIST_COMBO);
    int sel = (int)SendMessage(hCB, CB_GETCURSEL, 0, 0);
    if (sel <= 0) return "";   // CB_ERR or sentinel
    int len = (int)SendMessage(hCB, CB_GETLBTEXTLEN, sel, 0);
    if (len <= 0) return "";
    std::string name(len + 1, '\0');
    SendMessageA(hCB, CB_GETLBTEXT, sel, (LPARAM)name.data());
    name.resize(len);
    return name;
}

struct WatchlistRowData { std::string symbol; int conId = 0; };

// Find a ListView row by its lParam (which stores symbol+conId as WatchlistRowData*).
// Returns the row index, or -1 if not found.
static int Watchlist_FindRow(HWND hWnd, int conId) {
    HWND hWatchlistList = GetDlgItem(hWnd, ID_WATCHLIST_LIST);
    int count = ListView_GetItemCount(hWatchlistList);
    for (int i = 0; i < count; ++i) {
        LVITEMA lvi = {};
        lvi.mask  = LVIF_PARAM;
        lvi.iItem = i;
        SendMessageA(hWatchlistList, LVM_GETITEMA, 0, (LPARAM)&lvi);
        auto* rd = reinterpret_cast<WatchlistRowData*>(lvi.lParam);
        if (rd && rd->conId == conId) return i;
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

// ── Sort helpers ──────────────────────────────────────────────────────────────

static void Watchlist_SetSortArrow(HWND hList, int col, bool asc) {
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

static bool Watchlist_ColIsNumeric(int col) {
    return col != TCOL_SYMBOL && col != TCOL_DIV_DATE;
}

static int CALLBACK Watchlist_SortCompare(LPARAM idx1, LPARAM idx2, LPARAM /*extra*/) {
    char b1[64] = {}, b2[64] = {};
    ListView_GetItemText(g_WatchlistListForSort, (int)idx1, g_WatchlistSortCol, b1, sizeof(b1));
    ListView_GetItemText(g_WatchlistListForSort, (int)idx2, g_WatchlistSortCol, b2, sizeof(b2));
    int cmp;
    if (Watchlist_ColIsNumeric(g_WatchlistSortCol)) {
        double v1 = atof(b1), v2 = atof(b2);
        cmp = (v1 < v2) ? -1 : (v1 > v2) ? 1 : 0;
    } else {
        cmp = _stricmp(b1, b2);
    }
    return g_WatchlistSortAsc ? cmp : -cmp;
}

static void Watchlist_ApplySort(HWND hWnd) {
    HWND hList = GetDlgItem(hWnd, ID_WATCHLIST_LIST);
    g_WatchlistListForSort = hList;
    ListView_SortItemsEx(hList, Watchlist_SortCompare, 0);
    Watchlist_SetSortArrow(hList, g_WatchlistSortCol, g_WatchlistSortAsc);
}


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

// ── Subscribe ─────────────────────────────────────────────────────────────────

static void Watchlist_Subscribe(HWND hWnd, const std::string& listName) {
    if (listName.empty()) return;
    Settings_LastList_Save(listName);
    watchlistCurrentListName = listName;

    auto entries = Watchlist_ReadListEntries(listName.c_str());
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
    api().setWatchlistWindow(hWnd, entries);
    Watchlist_ApplySort(hWnd);
}

// Reload the combo from the registry, keeping the sentinel at index 0.
// Tries to restore whichever list was selected before.
static void Watchlist_LoadListCombo(HWND hWnd) {
    HWND hCB = GetDlgItem(hWnd, ID_WATCHLIST_LIST_COMBO);
    std::string current = Watchlist_GetSelectedList(hWnd);  // save before reset
    SendMessage(hCB, CB_RESETCONTENT, 0, 0);
    SendMessageA(hCB, CB_ADDSTRING, 0, (LPARAM)"-- Create new Watchlist --");
    for (const auto& name : Watchlist_LoadAllListNames())
        SendMessageA(hCB, CB_ADDSTRING, 0, (LPARAM)name.c_str());
    // Re-select the previous list if it still exists.
    if (!current.empty()) {
        int idx = (int)SendMessageA(hCB, CB_FINDSTRINGEXACT, -1, (LPARAM)current.c_str());
        if (idx != CB_ERR) { SendMessage(hCB, CB_SETCURSEL, idx, 0); return; }
    }
    // Fall back to sentinel (no auto-subscribe).
    
    SendMessage(hCB, CB_SETCURSEL, 0, 0);
}

// ── Layout ────────────────────────────────────────────────────────────────────

static void Watchlist_Layout(HWND hWnd) {
    HWND hList = GetDlgItem(hWnd, ID_WATCHLIST_LIST);
    if (!hList) return;
    RECT rc; GetClientRect(hWnd, &rc);
    const int m = 8, gap = 4;
    const int editW = 200;

    int availH    = watchlistSelectorsVisible ? rc.bottom - WATCHLIST_SELECTOR_H : rc.bottom;
    int selectorY = rc.bottom - WATCHLIST_SELECTOR_H;
    int comboY    = selectorY + (WATCHLIST_SELECTOR_H - WATCHLIST_COMBO_H) / 2;

    MoveWindow(hList, 0, 0, rc.right, availH, TRUE);

    if (watchlistSelectorsVisible) {
        int totalW = rc.right - m * 2;
        int comboW = totalW - editW - gap;

        // Search edit on the right
        SetWindowPos(GetDlgItem(hWnd, ID_WATCHLIST_EDIT), NULL,
                     m + comboW + gap, comboY, editW, WATCHLIST_COMBO_H,
                     SWP_NOZORDER | SWP_NOACTIVATE);
        // List selector combo on the left
        SetWindowPos(GetDlgItem(hWnd, ID_WATCHLIST_LIST_COMBO), NULL,
                     m, comboY, comboW, 200,
                     SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

static void Watchlist_ShowSelectors(HWND hWnd, bool show) {
    if (watchlistSelectorsVisible == show) return;
    watchlistSelectorsVisible = show;
    int sw = show ? SW_SHOW : SW_HIDE;
    ShowWindow(GetDlgItem(hWnd, ID_WATCHLIST_LIST_COMBO), sw);
    ShowWindow(GetDlgItem(hWnd, ID_WATCHLIST_EDIT), sw);
    if (!show) {
        // Hide autocomplete too when window loses focus.
        HWND hAC = (HWND)GetPropA(hWnd, "hWLAutoComplete");
        if (hAC) ShowWindow(hAC, SW_HIDE);
    }
    Watchlist_Layout(hWnd);
    if (show) SetFocus(GetDlgItem(hWnd, ID_WATCHLIST_EDIT));
}

// ── Autocomplete helpers ──────────────────────────────────────────────────────

static void Watchlist_UpdateAutoComplete(HWND hWnd, const std::vector<std::string>& results) {
    HWND hEdit = GetDlgItem(hWnd, ID_WATCHLIST_EDIT);
    HWND hAC   = (HWND)GetPropA(hWnd, "hWLAutoComplete");
    if (!hAC) return;

    EnableWindow(hAC, TRUE);
    wl_currentResults = results;

    if (results.empty()) { ShowWindow(hAC, SW_HIDE); return; }

    SendMessage(hAC, LB_RESETCONTENT, 0, 0);
    for (const auto& s : results)
        SendMessageA(hAC, LB_ADDSTRING, 0, (LPARAM)Watchlist_DisplayLabel(s).c_str());

    RECT r;
    GetWindowRect(hEdit, &r);
    int itemH    = (int)SendMessage(hAC, LB_GETITEMHEIGHT, 0, 0);
    int visItems = std::min((int)results.size(), 6);
    SetWindowPos(hAC, HWND_TOPMOST, r.left, r.bottom, r.right - r.left, itemH * visItems + 4,
                 SWP_SHOWWINDOW | SWP_NOACTIVATE);
}

// Adds fullEntry ("conId.symbol.exchange") to the currently selected list.
// Re-subscribes watchlist.
static void Watchlist_AddItem(HWND hWnd, const std::string& fullEntry) {
    std::string listName = Watchlist_GetSelectedList(hWnd);
    if (listName.empty()) return;
    auto entries = Watchlist_ReadListEntries(listName.c_str());
    for (const auto& e : entries) if (e == fullEntry) return;   // duplicate
    entries.push_back(fullEntry);
    
    std::sort(entries.begin(), entries.end(), [](const std::string& a, const std::string& b) {
        size_t dot_a = a.find('.');
        size_t dot_b = b.find('.');

        // Create a string_view that ignores everything up to and including the first dot
        std::string_view view_a = (dot_a != std::string::npos) ? std::string_view(a).substr(dot_a + 1) : a;
        std::string_view view_b = (dot_b != std::string::npos) ? std::string_view(b).substr(dot_b + 1) : b;

        return view_a < view_b;
     });
    Watchlist_SaveFullList(listName.c_str(), entries);
    Watchlist_Subscribe(hWnd, listName);  // refresh this window immediately
}

// ── Subclass procs ────────────────────────────────────────────────────────────

// Combo subclass: DEL key deletes the selected list.
LRESULT CALLBACK WatchlistComboSubclassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam,
                                             UINT_PTR uIdSubclass, DWORD_PTR /*dwRefData*/) {
    if (msg == WM_KEYDOWN && wParam == VK_DELETE) {
        HWND hWL   = GetParent(hWnd);
        std::string name = Watchlist_GetSelectedList(hWL);
        if (!name.empty()) {
            std::string prompt = "Delete watchlist \"" + name + "\"?";
            if (MessageBoxA(hWL, prompt.c_str(), "Confirm", MB_YESNO | MB_ICONQUESTION) == IDYES) {
                Watchlist_DeleteList(name.c_str());
                // If we were showing this list, clear it.
                if (watchlistCurrentListName == name) {
                    watchlistCurrentListName.clear();
                    Watchlist_ClearList(hWL);
                    watchlistCurrentEntries.clear();
                    SetWindowTextA(hWL, "Watchlist");
                }
                Watchlist_LoadListCombo(hWL);
                PostMessage(hWL, WM_WATCHLIST_LISTS_CHANGED, 0, (LPARAM)new std::string(""));
            }
        }
        return 0;
    }
    if (msg == WM_NCDESTROY) RemoveWindowSubclass(hWnd, WatchlistComboSubclassProc, uIdSubclass);
    return DefSubclassProc(hWnd, msg, wParam, lParam);
}

// Edit subclass: arrow keys / Enter / Escape navigate the autocomplete.
LRESULT CALLBACK WatchlistEditSubclassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam,
                                            UINT_PTR uIdSubclass, DWORD_PTR /*dwRefData*/) {
    if (msg == WM_KEYDOWN) {
        HWND hWL = GetParent(hWnd);
        HWND hAC = (HWND)GetPropA(hWL, "hWLAutoComplete");
        bool acVis = hAC && IsWindowVisible(hAC);

        if (wParam == VK_DOWN && acVis) {
            int cnt = (int)SendMessage(hAC, LB_GETCOUNT, 0, 0);
            int sel = (int)SendMessage(hAC, LB_GETCURSEL, 0, 0);
            if (sel == LB_ERR) sel = -1;
            if (sel + 1 < cnt) SendMessage(hAC, LB_SETCURSEL, sel + 1, 0);
            return 0;
        }
        if (wParam == VK_UP && acVis) {
            int sel = (int)SendMessage(hAC, LB_GETCURSEL, 0, 0);
            if (sel > 0) SendMessage(hAC, LB_SETCURSEL, sel - 1, 0);
            return 0;
        }
        if (wParam == VK_RETURN) {
            if (acVis) {
                int sel = (int)SendMessage(hAC, LB_GETCURSEL, 0, 0);
                if (sel == LB_ERR && SendMessage(hAC, LB_GETCOUNT, 0, 0) > 0) {
                    sel = 0;
                }
                if (sel != LB_ERR && sel < (int)wl_currentResults.size()) {
                    wl_pendingFullEntry = wl_currentResults[sel];
                    wl_suppressSearch = true;
                    SetWindowTextA(hWnd, Watchlist_DisplayLabel(wl_pendingFullEntry).c_str());
                    wl_suppressSearch = false;
                    ShowWindow(hAC, SW_HIDE);
                    Watchlist_AddItem(hWL, wl_pendingFullEntry);
                    SetWindowTextA(hWnd, "");
                    wl_pendingFullEntry.clear();
                }
            }
            return 0;
        }
        if (wParam == VK_ESCAPE && acVis) {
            ShowWindow(hAC, SW_HIDE);
            return 0;
        }
    }
    if (msg == WM_NCDESTROY) RemoveWindowSubclass(hWnd, WatchlistEditSubclassProc, uIdSubclass);
    return DefSubclassProc(hWnd, msg, wParam, lParam);
}

// Autocomplete listbox subclass: mouse click selects and adds the item.
LRESULT CALLBACK WatchlistAutoCompleteSubclassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam,
                                                    UINT_PTR uIdSubclass, DWORD_PTR /*dwRefData*/) {
    if (msg == WM_LBUTTONUP) {
        int sel = (int)SendMessage(hWnd, LB_GETCURSEL, 0, 0);
        if (sel != LB_ERR && sel < (int)wl_currentResults.size()) {
            HWND hWL   = (HWND)GetPropA(hWnd, "hWLParent");
            HWND hEdit = GetDlgItem(hWL, ID_WATCHLIST_EDIT);
            wl_pendingFullEntry = wl_currentResults[sel];
            KillTimer(hWL, TIMER_WL_DROPDOWN);
            ShowWindow(hWnd, SW_HIDE);
            SetFocus(hEdit);
            Watchlist_AddItem(hWL, wl_pendingFullEntry);
            SetWindowTextA(hEdit, "");
            wl_pendingFullEntry.clear();
        }
    }
    if (msg == WM_NCDESTROY) RemoveWindowSubclass(hWnd, WatchlistAutoCompleteSubclassProc, uIdSubclass);
    return DefSubclassProc(hWnd, msg, wParam, lParam);
}

// ── Window procedure ──────────────────────────────────────────────────────────

LRESULT CALLBACK WndProcNewList(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE: {
            // Create the edit control and return immediately so WM_CREATE finishes.
            // The window will fully paint its NC area (title bar, border, icons)
            // before WM_WATCHLIST_NEW_LIST_START arrives, fixing the blank-title glitch.
            CreateWindowA("EDIT", "",
                WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                10, 10, 260 - 28, 24,
                hWnd, (HMENU)ID_WATCHLIST_NEW_SYMBOL_INPUT, GetModuleHandle(NULL), NULL);

            PostMessage(hWnd, WM_WATCHLIST_NEW_LIST_START, 0, 0);
            break;
        }

        case WM_WATCHLIST_NEW_LIST_START: {
            // Window is fully created and painted by now — safe to run the modal loop.
            SetFocus(GetDlgItem(hWnd, ID_WATCHLIST_NEW_SYMBOL_INPUT));

            MSG dlgMsg;
            char newName[128] = {};
            bool dlgDone = false;

            while (!dlgDone && GetMessage(&dlgMsg, NULL, 0, 0)) {
                if (dlgMsg.hwnd == GetDlgItem(hWnd, ID_WATCHLIST_NEW_SYMBOL_INPUT)
                    && dlgMsg.message == WM_KEYDOWN
                    && dlgMsg.wParam == VK_RETURN)
                {
                    GetWindowTextA(GetDlgItem(hWnd, ID_WATCHLIST_NEW_SYMBOL_INPUT), newName, sizeof(newName));
                    dlgDone = true;
                } else if (dlgMsg.message == WM_KEYDOWN && dlgMsg.wParam == VK_ESCAPE) {
                    dlgDone = true;
                } else {
                    TranslateMessage(&dlgMsg);
                    DispatchMessage(&dlgMsg);
                }
                if (!IsWindow(hWnd)) dlgDone = true;
            }

            DestroyWindow(hWnd);

            if (strlen(newName) > 0) {
                Settings_NewList_Save(newName);

                // Notify Watchlist to reload their combos.
                HWND hWL = FindWindowA(WATCHLIST_CLASS_NAME, NULL);
                PostMessage(hWL, WM_WATCHLIST_LISTS_CHANGED, 0, (LPARAM)new std::string(newName));
            }
            break;
        }
    }

    return HandleCommonMessages(hWnd, message, wParam, lParam);
}

LRESULT CALLBACK WndProcWatchlist(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {

    case WM_CREATE: {
        HINSTANCE hInst = ((LPCREATESTRUCT)lParam)->hInstance;
        const int m = 8;

        // List first — lower Z-order, behind the selector row.
        HWND hWatchlistList = CreateWindowExA(
            WS_EX_CLIENTEDGE, "SysListView32", "",
            WS_CHILD | WS_VISIBLE | WS_BORDER |
            LVS_REPORT | LVS_SHOWSELALWAYS,
            0, 0, 1000, 400,
            hWnd, (HMENU)ID_WATCHLIST_LIST, hInst, NULL);

        WatchlistZoomData.fontSize = (int)Settings_Load(WatchlistZoomData.settingKey, WatchlistZoomData.fontSize);
        ApplyListViewFont(hWatchlistList, WatchlistZoomData.hFont, WatchlistZoomData.hBoldFont, WatchlistZoomData.fontSize);
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

        // Search edit (right side of selector row), hidden until focused.
        HWND hEdit = CreateWindowA("EDIT", NULL,
            WS_CHILD | WS_BORDER | ES_AUTOHSCROLL | ES_UPPERCASE,
            m, m, 200, WATCHLIST_COMBO_H,
            hWnd, (HMENU)ID_WATCHLIST_EDIT, hInst, NULL);
        SetWindowSubclass(hEdit, WatchlistEditSubclassProc, 1, 0);

        // List selector combo (right side of selector row), hidden until focused.
        HWND hCB = CreateWindowA("COMBOBOX", NULL,
            WS_CHILD | WS_VSCROLL | CBS_DROPDOWNLIST,
            m, m, 200, 200,
            hWnd, (HMENU)ID_WATCHLIST_LIST_COMBO, hInst, NULL);
        SetWindowSubclass(hCB, WatchlistComboSubclassProc, 2, 0);

        // Autocomplete popup — top-level, topmost, hidden by default.
        HWND hAC = CreateWindowExA(
            WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
            "LISTBOX", NULL,
            WS_POPUP | WS_BORDER | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT,
            0, 0, 200, 100,
            NULL, NULL, hInst, NULL);
        SetWindowSubclass(hAC, WatchlistAutoCompleteSubclassProc, 3, 0);
        // Store the watchlist HWND on the autocomplete so its subclass can reach it.
        SetPropA(hAC, "hWLParent", (HANDLE)hWnd);
        SetPropA(hWnd, "hWLAutoComplete", (HANDLE)hAC);
        WatchlistAutocompleteZoomData.fontSize = WatchlistZoomData.fontSize;
        ApplyListViewFont(hAC, WatchlistAutocompleteZoomData.hFont, WatchlistAutocompleteZoomData.hBoldFont, WatchlistAutocompleteZoomData.fontSize);

        // Populate combo (sentinel + all lists).
        Watchlist_LoadListCombo(hWnd);

        // Restore last list.
        std::string openList = Settings_LastList_Load();
        if (!openList.empty()) {
            int idx = (int)SendMessageA(hCB, CB_FINDSTRINGEXACT, -1, (LPARAM)openList.c_str());
            if (idx != CB_ERR) {
                SendMessage(hCB, CB_SETCURSEL, idx, 0);
                Watchlist_Subscribe(hWnd, openList);
            }
        }

        api().addApiUpdateWindow(hWnd);

        // Load sort settings.
        g_WatchlistSortCol = (int)Settings_Sort_Load(WATCHLIST_CLASS_NAME, "SortCol", TCOL_SYMBOL);
        g_WatchlistSortAsc = Settings_Sort_Load(WATCHLIST_CLASS_NAME, "SortAsc", 1) != 0;
        if (g_WatchlistSortCol < 0 || g_WatchlistSortCol >= TCOL_COUNT) g_WatchlistSortCol = TCOL_SYMBOL;
        break;
    }

    case WM_SIZE:
        Watchlist_Layout(hWnd);
        return 0;

    case WM_ACTIVATE:
        Watchlist_ShowSelectors(hWnd, LOWORD(wParam) != WA_INACTIVE);
        return 0;

    case WM_COMMAND:
        if (LOWORD(wParam) == ID_WATCHLIST_LIST_COMBO && HIWORD(wParam) == CBN_SELCHANGE) {
            HWND hCB = GetDlgItem(hWnd, ID_WATCHLIST_LIST_COMBO);
            int sel = (int)SendMessage(hCB, CB_GETCURSEL, 0, 0);
            if (sel == 0) {
                // Sentinel selected — open "New Watchlist" dialog, then re-select previous.
                std::string prev = watchlistCurrentListName;
                StartGenericWindow(WATCHLIST_NEW_LIST_CLASS_NAME, "New Watchlist Name", L"TWSAPIClientTradingFloor.WatchlistNewList", 260, 75);
                // Restore selection (new list will arrive via WM_WATCHLIST_LISTS_CHANGED).
                if (!prev.empty()) {
                    int idx = (int)SendMessageA(hCB, CB_FINDSTRINGEXACT, -1, (LPARAM)prev.c_str());
                    if (idx != CB_ERR) SendMessage(hCB, CB_SETCURSEL, idx, 0);
                    else               SendMessage(hCB, CB_SETCURSEL, 0, 0);
                }
            } else {
                std::string name = Watchlist_GetSelectedList(hWnd);
                if (!name.empty()) Watchlist_Subscribe(hWnd, name);
            }
        }
        if (LOWORD(wParam) == ID_WATCHLIST_EDIT && HIWORD(wParam) == EN_CHANGE) {
            if (wl_suppressSearch) break;
            wl_pendingFullEntry.clear();
            char text[256] = {};
            GetWindowTextA(GetDlgItem(hWnd, ID_WATCHLIST_EDIT), text, sizeof(text));
            std::string query = text;
            auto dot = query.find('.');
            if (dot != std::string::npos) query = query.substr(0, dot);
            HWND hAC = (HWND)GetPropA(hWnd, "hWLAutoComplete");
            if (query.size() >= 1) {
                if (!api().isConnected()) {
                    wl_showingOffline = true;
                    SendMessage(hAC, LB_RESETCONTENT, 0, 0);
                    SendMessageA(hAC, LB_ADDSTRING, 0, (LPARAM)"Gateway is disconnected!");
                    RECT r; GetWindowRect(GetDlgItem(hWnd, ID_WATCHLIST_EDIT), &r);
                    int ih = (int)SendMessage(hAC, LB_GETITEMHEIGHT, 0, 0);
                    SetWindowPos(hAC, HWND_TOPMOST, r.left, r.bottom, r.right - r.left, ih + 4,
                                 SWP_SHOWWINDOW | SWP_NOACTIVATE);
                    EnableWindow(hAC, FALSE);
                } else {
                    wl_showingOffline = false;
                    api().searchSymbols(hWnd, query);
                }
            } else {
                ShowWindow(hAC, SW_HIDE);
            }
        }
        if (LOWORD(wParam) == ID_WATCHLIST_EDIT && HIWORD(wParam) == EN_KILLFOCUS) {
            SetTimer(hWnd, TIMER_WL_DROPDOWN, 150, NULL);
        }
        break;

    case WM_TIMER:
        if (wParam == TIMER_WL_DROPDOWN) {
            KillTimer(hWnd, TIMER_WL_DROPDOWN);
            HWND hAC = (HWND)GetPropA(hWnd, "hWLAutoComplete");
            if (hAC) ShowWindow(hAC, SW_HIDE);
        }
        break;

    case WM_SYMBOL_RESULTS: {
        if (wl_showingOffline) break;
        char text[256] = {};
        GetWindowTextA(GetDlgItem(hWnd, ID_WATCHLIST_EDIT), text, sizeof(text));
        std::string query = text;
        auto results = api().getSymbolResults();
        if (query.find('.') != std::string::npos) {
            std::string upper = query;
            for (auto& c : upper) c = (char)toupper((unsigned char)c);
            std::vector<std::string> filtered;
            for (const auto& r : results) {
                std::string label = Watchlist_DisplayLabel(r);
                std::string lu = label;
                for (auto& c : lu) c = (char)toupper((unsigned char)c);
                if (lu.find(upper) == 0) filtered.push_back(r);
            }
            Watchlist_UpdateAutoComplete(hWnd, filtered);
        } else {
            Watchlist_UpdateAutoComplete(hWnd, results);
        }
        break;
    }

    // ── Live or historical market data update for one symbol ──────────────────
    // lParam = new std::string("conId.symbol") — we own it and must delete.
    case WM_API_WATCHLIST_UPDATE: {
        int conId = (int)lParam;
        if (!conId) break;
        for (const auto& entry : watchlistCurrentEntries) {
            auto d1 = entry.find('.');
            if (d1 == std::string::npos) continue;
            if (conId == std::stoi(entry.substr(0, d1))) {
                TradingAPI::WatchlistInfo info;
                if (api().getWatchlistData(conId, info)) {
                    int row = Watchlist_FindRow(hWnd, conId);
                    if (row >= 0) Watchlist_UpdateRow(hWnd, row, info);
                }
                break;
            }
        }
        Watchlist_ApplySort(hWnd);
        break;
    }

    // ── Watchlist list changed notification ────────────────────────────────────────
    // Sent by Watchlist_NotifyListChanged whenever a list is mutated (add/delete/sort).
    // lParam = new std::string*(listName) — we own and must delete.
    case WM_WATCHLIST_LIST_CHANGED: {
        auto* name = reinterpret_cast<std::string*>(lParam);
        if (name) {
            if (*name == watchlistCurrentListName)
                Watchlist_Subscribe(hWnd, *name);
            delete name;
        }
        return 0;
    }

    case WM_API_UPDATE:
        if (api().isMarketDataConnected() && api().isTradingConnected()) {
            // Re-subscribe after reconnect with the same entries.
            api().setWatchlistWindow(hWnd, watchlistCurrentEntries);
        } else {
            // Clear data but keep the symbol rows visible (just blank the values).
            HWND hWatchlistList = GetDlgItem(hWnd, ID_WATCHLIST_LIST);
            int count = ListView_GetItemCount(hWatchlistList);
            for (int i = 0; i < count; ++i)
                for (int col = 1; col < TCOL_COUNT; ++col)
                    ListView_SetItemText(hWatchlistList, i, col, (LPSTR)"");
        }
        break;

    // ── Watchlist list-set changed (list created or deleted externally) ────────────
    case WM_WATCHLIST_LISTS_CHANGED: {
        auto* name = reinterpret_cast<std::string*>(lParam);
        std::string newName = name ? *name : "";
        delete name;
        Watchlist_LoadListCombo(hWnd);
        // If a new list was created, auto-select and subscribe to it.
        if (newName.empty()) {
            HWND hCB = GetDlgItem(hWnd, ID_WATCHLIST_LIST_COMBO);
            int count = (int)SendMessage(hCB, CB_GETCOUNT, 0, 0);
            if (count > 1) {
                int targetIndex = 1;
                SendMessage(hCB, CB_SETCURSEL, targetIndex, 0);
                int textLen = (int)SendMessage(hCB, CB_GETLBTEXTLEN, targetIndex, 0);
                if (textLen != CB_ERR) {
                    std::string selectedName;
                    selectedName.resize(textLen);
                    SendMessageA(hCB, CB_GETLBTEXT, targetIndex, (LPARAM)selectedName.data());
                    Watchlist_Subscribe(hWnd, selectedName);
                }
            }
        } else {
            HWND hCB = GetDlgItem(hWnd, ID_WATCHLIST_LIST_COMBO);
            int idx = (int)SendMessageA(hCB, CB_FINDSTRINGEXACT, -1, (LPARAM)newName.c_str());
            if (idx != CB_ERR) {
                SendMessage(hCB, CB_SETCURSEL, idx, 0);
                Watchlist_Subscribe(hWnd, newName);
            }
        }
        return 0;
    }

    case WM_NOTIFY: {
        NMHDR* hdr = (NMHDR*)lParam;
        if (hdr->idFrom != ID_WATCHLIST_LIST) break;

        if (hdr->code == LVN_COLUMNCLICK) {
            NMLISTVIEW* nmlv = (NMLISTVIEW*)lParam;
            int col = nmlv->iSubItem;
            if (col == g_WatchlistSortCol) g_WatchlistSortAsc = !g_WatchlistSortAsc;
            else { g_WatchlistSortCol = col; g_WatchlistSortAsc = true; }
            Settings_Sort_Save(WATCHLIST_CLASS_NAME, "SortCol", g_WatchlistSortCol);
            Settings_Sort_Save(WATCHLIST_CLASS_NAME, "SortAsc", g_WatchlistSortAsc ? 1 : 0);
            Watchlist_ApplySort(hWnd);
            return 0;
        }

        if (hdr->code == LVN_KEYDOWN) {
            NMLVKEYDOWN* kd = (NMLVKEYDOWN*)lParam;
            if (kd->wVKey == VK_DELETE) {
                HWND hList = GetDlgItem(hWnd, ID_WATCHLIST_LIST);
                int row = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
                if (row >= 0) {
                    // Retrieve the stored entry string from watchlistCurrentEntries
                    // by matching via the row's lParam (WatchlistRowData*).
                    LVITEMA lvi = {};
                    lvi.mask  = LVIF_PARAM;
                    lvi.iItem = row;
                    SendMessageA(hList, LVM_GETITEMA, 0, (LPARAM)&lvi);
                    auto* rd = reinterpret_cast<WatchlistRowData*>(lvi.lParam);
                    if (rd) {
                        // Find and remove the matching entry from the registry list.
                        std::string listName = watchlistCurrentListName;
                        auto entries = Watchlist_ReadListEntries(listName.c_str());
                        auto it = std::remove_if(entries.begin(), entries.end(),
                            [&](const std::string& e) {
                                auto d1 = e.find('.');
                                if (d1 == std::string::npos) return false;
                                int cid = std::stoi(e.substr(0, d1));
                                auto d2 = e.find('.', d1 + 1);
                                std::string sym = (d2 != std::string::npos)
                                    ? e.substr(d1 + 1, d2 - d1 - 1)
                                    : e.substr(d1 + 1);
                                return cid == rd->conId && sym == rd->symbol;
                            });
                        if (it != entries.end()) {
                            entries.erase(it, entries.end());
                            Watchlist_SaveFullList(listName.c_str(), entries);
                            // Refresh: delete row from view and update our entry cache.
                            delete rd;
                            ListView_DeleteItem(hList, row);
                            watchlistCurrentEntries = entries;
                            api().setWatchlistWindow(hWnd, watchlistCurrentEntries);
                        }
                    }
                }
            }
            break;
        }
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
                    if (cd->iSubItem == TCOL_SYMBOL) {
                        SelectObject(cd->nmcd.hdc, WatchlistZoomData.hBoldFont);
                        return CDRF_NEWFONT;
                    }
                    if (cd->iSubItem == TCOL_LAST) {
                        SelectObject(cd->nmcd.hdc, WatchlistZoomData.hFont);
                        return CDRF_NEWFONT;
                    }
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
        api().unsetWatchlistWindow();
        api().removeApiUpdateWindow(hWnd);
        Watchlist_ClearList(hWnd);
        watchlistCurrentEntries.clear();
        watchlistCurrentListName.clear();
        {
            HWND hAC = (HWND)GetPropA(hWnd, "hWLAutoComplete");
            if (hAC) { RemovePropA(hAC, "hWLParent"); DestroyWindow(hAC); }
            RemovePropA(hWnd, "hWLAutoComplete");
        }
        if (WatchlistZoomData.hFont) {
            DeleteObject(WatchlistZoomData.hFont);
        }
        if (WatchlistZoomData.hBoldFont) {
            DeleteObject(WatchlistZoomData.hBoldFont);
        }
        if (WatchlistAutocompleteZoomData.hFont) {
            DeleteObject(WatchlistAutocompleteZoomData.hFont);
        }
        if (WatchlistAutocompleteZoomData.hBoldFont) {
            DeleteObject(WatchlistAutocompleteZoomData.hBoldFont);
        }
        break;
    }

    return HandleCommonMessages(hWnd, message, wParam, lParam);
}
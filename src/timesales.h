#pragma once

void startTimesales() { startGenericWindow(TIMESALES_CLASS_NAME, "Time & Sales", L"IBKRGatewayClient.Timesales", 380, 500); }

#define ID_TS_LIST_COMBO    7001
#define ID_TS_SYM_COMBO     7002
#define ID_TS_LIST          7003
#define ID_TS_FILTER_CHECK  7004
#define ID_TS_LIST_F100     7005
#define ID_TS_LIST_F1000    7006

static HWND hTsListCombo    = NULL;
static HWND hTsSymCombo     = NULL;
static HWND hTsList         = NULL;   // all ticks
static HWND hTsListF100     = NULL;   // size >= 100
static HWND hTsListF1000    = NULL;   // size >= 1000
static HWND hTsFilterCheck  = NULL;
static std::vector<std::string> tsSymEntries;
static bool tsSelectorsVisible = false;
static bool tsFilteredView     = false;

static const int TS_COMBO_H    = 24;
static const int TS_COMBO_GAP  = 8;
static const int TS_CHECK_W    = 20;
static const int TS_CHECK_GAP  = 6;
static const int TS_SELECTOR_H = 8 + TS_COMBO_H + 8;

// ── Column definitions ────────────────────────────────────────────────────────

struct TsCol { const char* header; int width; int fmt; };
static const TsCol tsCols[] = {
    { "Price",    60, LVCFMT_RIGHT },
    { "Size",     40, LVCFMT_RIGHT },
    { "Time",     90, LVCFMT_LEFT  },
    { "Exchange", 90, LVCFMT_LEFT  },
};
static const int TS_COL_COUNT = (int)(sizeof(tsCols) / sizeof(tsCols[0]));

// ── Selector helpers ──────────────────────────────────────────────────────────

static std::string Ts_GetSelectedList() {
    int sel = (int)SendMessage(hTsListCombo, CB_GETCURSEL, 0, 0);
    if (sel == CB_ERR) return "";
    int len = (int)SendMessage(hTsListCombo, CB_GETLBTEXTLEN, sel, 0);
    if (len <= 0) return "";
    std::string name(len + 1, '\0');
    SendMessageA(hTsListCombo, CB_GETLBTEXT, sel, (LPARAM)name.data());
    name.resize(len);
    return name;
}

static bool Ts_LoadListCombo(const std::string& savedList = "") {
    SendMessage(hTsListCombo, CB_RESETCONTENT, 0, 0);
    Book_LoadAllLists(hTsListCombo);
    if ((int)SendMessage(hTsListCombo, CB_GETCOUNT, 0, 0) == 0) return false;
    int idx = CB_ERR;
    if (!savedList.empty())
        idx = (int)SendMessageA(hTsListCombo, CB_FINDSTRINGEXACT, -1, (LPARAM)savedList.c_str());
    SendMessage(hTsListCombo, CB_SETCURSEL, idx == CB_ERR ? 0 : idx, 0);
    return true;
}

static bool Ts_LoadSymbolCombo(const std::string& savedEntry = "") {
    SendMessage(hTsSymCombo, CB_RESETCONTENT, 0, 0);
    tsSymEntries.clear();
    std::string listName = Ts_GetSelectedList();
    if (listName.empty()) return false;
    for (const auto& full : Book_ReadListEntries(listName.c_str())) {
        tsSymEntries.push_back(full);
        SendMessageA(hTsSymCombo, CB_ADDSTRING, 0, (LPARAM)Book_DisplayLabel(full).c_str());
    }
    if (tsSymEntries.empty()) return false;
    int idx = CB_ERR;
    for (int i = 0; i < (int)tsSymEntries.size(); ++i)
        if (tsSymEntries[i] == savedEntry) { idx = i; break; }
    SendMessage(hTsSymCombo, CB_SETCURSEL, idx == CB_ERR ? 0 : idx, 0);
    return true;
}

// ── ListView helpers ──────────────────────────────────────────────────────────

static HWND Ts_CreateListView(HWND hParent, int id, HINSTANCE hInst) {
    HWND hList = CreateWindowExA(
        WS_EX_CLIENTEDGE, "SysListView32", "",
        WS_CHILD | WS_BORDER |
        LVS_REPORT | LVS_SHOWSELALWAYS | LVS_NOSORTHEADER,
        0, 0, 10, 10,
        hParent, (HMENU)(intptr_t)id, hInst, NULL);

    DWORD exStyle = LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER;
    if (Settings_DarkMode()) exStyle |= LVS_EX_GRIDLINES;
    ListView_SetExtendedListViewStyle(hList, exStyle);

    LVCOLUMNA lvc = {};
    lvc.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_FMT;
    for (int i = 0; i < TS_COL_COUNT; ++i) {
        lvc.cx      = tsCols[i].width;
        lvc.pszText = (LPSTR)tsCols[i].header;
        lvc.fmt     = tsCols[i].fmt;
        ListView_InsertColumn(hList, i, &lvc);
    }
    return hList;
}

// Insert one tick row at position 0 (newest first), cap at 500.
static void Ts_InsertTick(HWND hList, double price, double size,
                          const std::string& time, const std::string& exchange) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%.2f", price);
    LVITEMA lvi = {};
    lvi.mask    = LVIF_TEXT;
    lvi.iItem   = 0;
    lvi.pszText = buf;
    ListView_InsertItem(hList, &lvi);

    snprintf(buf, sizeof(buf), "%.0f", size);
    ListView_SetItemText(hList, 0, 1, buf);
    ListView_SetItemText(hList, 0, 2, (LPSTR)time.c_str());
    ListView_SetItemText(hList, 0, 3, (LPSTR)exchange.c_str());

    int count = ListView_GetItemCount(hList);
    if (count > 500) ListView_DeleteItem(hList, count - 1);
}

// ── Layout ────────────────────────────────────────────────────────────────────

static void Ts_Layout(HWND hWnd) {
    if (!hTsList) return;
    RECT rc; GetClientRect(hWnd, &rc);
    const int m = 8;

    int availH = tsSelectorsVisible ? rc.bottom - TS_SELECTOR_H : rc.bottom;
    int availW = rc.right;
    int selectorY = rc.bottom - TS_SELECTOR_H;

    // ── Lists fill from top down ──
    if (tsFilteredView) {
        int leftW  = availW / 2;
        int rightW = availW - leftW;
        int halfH  = availH / 2;

        ShowWindow(hTsList,      SW_SHOW);
        ShowWindow(hTsListF100,  SW_SHOW);
        ShowWindow(hTsListF1000, SW_SHOW);

        MoveWindow(hTsList,      0,     0,         leftW,  availH,         TRUE);
        MoveWindow(hTsListF100,  leftW, 0,         rightW, halfH,          TRUE);
        MoveWindow(hTsListF1000, leftW, halfH,     rightW, availH - halfH, TRUE);
    } else {
        ShowWindow(hTsListF100,  SW_HIDE);
        ShowWindow(hTsListF1000, SW_HIDE);
        ShowWindow(hTsList,      SW_SHOW);
        MoveWindow(hTsList, 0, 0, availW, availH, TRUE);
    }

    // ── Selector row sits at the bottom ──
    if (tsSelectorsVisible) {
        int totalComboW = rc.right - m * 2 - TS_COMBO_GAP - TS_CHECK_GAP - TS_CHECK_W;
        int eachW = totalComboW / 2;
        int comboY = selectorY + (TS_SELECTOR_H - TS_COMBO_H) / 2;

        SetWindowPos(hTsListCombo,   NULL, m,                         comboY,
                     eachW, 200, SWP_NOZORDER | SWP_NOACTIVATE);
        SetWindowPos(hTsSymCombo,    NULL, m + eachW + TS_COMBO_GAP, comboY,
                     eachW, 200, SWP_NOZORDER | SWP_NOACTIVATE);
        SetWindowPos(hTsFilterCheck, NULL,
                     rc.right - m - TS_CHECK_W,
                     selectorY + (TS_SELECTOR_H - TS_CHECK_W) / 2,
                     TS_CHECK_W, TS_CHECK_W,
                     SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

static void Ts_ShowSelectors(HWND hWnd, bool show) {
    if (tsSelectorsVisible == show) return;
    tsSelectorsVisible = show;
    int sw = show ? SW_SHOW : SW_HIDE;
    ShowWindow(hTsListCombo,   sw);
    ShowWindow(hTsSymCombo,    sw);
    ShowWindow(hTsFilterCheck, sw);
    Ts_Layout(hWnd);
}

// ── Subscribe ─────────────────────────────────────────────────────────────────

static void Ts_Subscribe(HWND hWnd, const std::string& fullEntry) {
    auto firstDot = fullEntry.find('.');
    if (firstDot == std::string::npos) return;
    std::string conIdStr = fullEntry.substr(0, firstDot);
    std::string rest     = fullEntry.substr(firstDot + 1);
    auto secondDot       = rest.find('.');
    std::string symbol   = (secondDot != std::string::npos) ? rest.substr(0, secondDot) : rest;

    Settings_SaveString("LastTsList",  Ts_GetSelectedList());
    Settings_SaveString("LastTsEntry", fullEntry);

    ListView_DeleteAllItems(hTsList);
    if (hTsListF100)  ListView_DeleteAllItems(hTsListF100);
    if (hTsListF1000) ListView_DeleteAllItems(hTsListF1000);
    SetWindowTextA(hWnd, ("Time & Sales: " + symbol).c_str());

    api.setTimesalesWindow(hWnd, std::stoi(conIdStr), symbol);
}

// ── Window procedure ──────────────────────────────────────────────────────────

LRESULT CALLBACK WndProcTimesales(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {

    case WM_CREATE: {
        HINSTANCE hInst = ((LPCREATESTRUCT)lParam)->hInstance;
        const int m = 8;

        // Lists created FIRST so they sit behind the selectors in Z-order.
        hTsList      = Ts_CreateListView(hWnd, ID_TS_LIST,      hInst);
        hTsListF100  = Ts_CreateListView(hWnd, ID_TS_LIST_F100,  hInst);
        hTsListF1000 = Ts_CreateListView(hWnd, ID_TS_LIST_F1000, hInst);
        ShowWindow(hTsList, SW_SHOW);

        // Selectors created AFTER lists — higher Z-order, rendered on top.
        hTsListCombo = CreateWindowA("COMBOBOX", NULL,
            WS_CHILD | WS_VSCROLL | CBS_DROPDOWNLIST,
            m, m, 180, 200,
            hWnd, (HMENU)ID_TS_LIST_COMBO, hInst, NULL);

        hTsSymCombo = CreateWindowA("COMBOBOX", NULL,
            WS_CHILD | WS_VSCROLL | CBS_DROPDOWNLIST,
            m + 180 + TS_COMBO_GAP, m, 180, 200,
            hWnd, (HMENU)ID_TS_SYM_COMBO, hInst, NULL);

        hTsFilterCheck = CreateWindowA("BUTTON", "",
            WS_CHILD | BS_AUTOCHECKBOX,
            0, 0, TS_CHECK_W, TS_CHECK_W,
            hWnd, (HMENU)ID_TS_FILTER_CHECK, hInst, NULL);

        // Restore last session
        std::string lastList  = Settings_LoadString("LastTsList");
        std::string lastEntry = Settings_LoadString("LastTsEntry");
        Ts_LoadListCombo(lastList);
        if (Ts_LoadSymbolCombo(lastEntry)) {
            int sel = (int)SendMessage(hTsSymCombo, CB_GETCURSEL, 0, 0);
            if (sel != CB_ERR && sel < (int)tsSymEntries.size())
                Ts_Subscribe(hWnd, tsSymEntries[sel]);
        }
        break;
    }

    case WM_SIZE:
        Ts_Layout(hWnd);
        return 0;

    case WM_ACTIVATE:
        Ts_ShowSelectors(hWnd, LOWORD(wParam) != WA_INACTIVE);
        return 0;

    case WM_COMMAND:
        if (LOWORD(wParam) == ID_TS_LIST_COMBO && HIWORD(wParam) == CBN_SELCHANGE) {
            Settings_SaveString("LastTsList", Ts_GetSelectedList());
            Ts_LoadSymbolCombo();
            if (!tsSymEntries.empty()) Ts_Subscribe(hWnd, tsSymEntries[0]);
        }
        if (LOWORD(wParam) == ID_TS_SYM_COMBO && HIWORD(wParam) == CBN_SELCHANGE) {
            int sel = (int)SendMessage(hTsSymCombo, CB_GETCURSEL, 0, 0);
            if (sel != CB_ERR && sel < (int)tsSymEntries.size())
                Ts_Subscribe(hWnd, tsSymEntries[sel]);
        }
        if (LOWORD(wParam) == ID_TS_FILTER_CHECK && HIWORD(wParam) == BN_CLICKED) {
            HWND hChk = GetDlgItem(hWnd, ID_TS_FILTER_CHECK);
            tsFilteredView = (SendMessage(hChk, BM_GETCHECK, 0, 0) == BST_CHECKED);
            Settings_Save("TsFilteredView", tsFilteredView ? 1 : 0);
            // Clear filtered lists when toggling on — they'll fill from live ticks
            if (tsFilteredView) {
                ListView_DeleteAllItems(hTsListF100);
                ListView_DeleteAllItems(hTsListF1000);
            }
            Ts_Layout(hWnd);
        }
        break;

    case WM_TIMESALES_TICK: {
        auto* tick = reinterpret_cast<TradingAPI::TsTickEntry*>(lParam);

        Ts_InsertTick(hTsList, tick->price, tick->size, tick->time, tick->exchange);

        if (tsFilteredView) {
            if (tick->size >= 100.0)
                Ts_InsertTick(hTsListF100, tick->price, tick->size, tick->time, tick->exchange);
            if (tick->size >= 1000.0)
                Ts_InsertTick(hTsListF1000, tick->price, tick->size, tick->time, tick->exchange);
        }

        delete tick;
        break;
    }

    case WM_NOTIFY: {
        NMHDR* hdr = (NMHDR*)lParam;
        if (hdr->code != NM_CUSTOMDRAW) break;
        if (hdr->idFrom != ID_TS_LIST &&
            hdr->idFrom != ID_TS_LIST_F100 &&
            hdr->idFrom != ID_TS_LIST_F1000) break;

        NMLVCUSTOMDRAW* cd = (NMLVCUSTOMDRAW*)lParam;
        if (!Settings_DarkMode()) break;
        switch (cd->nmcd.dwDrawStage) {
            case CDDS_PREPAINT:     return CDRF_NOTIFYITEMDRAW;
            case CDDS_ITEMPREPAINT:
                cd->clrTextBk = (cd->nmcd.dwItemSpec % 2 == 0) ? DM_BG : DM_BG2;
                cd->clrText   = DM_TEXT;
                return CDRF_DODEFAULT;
        }
        break;
    }

    case WM_DESTROY:
        api.unsetTimesalesWindow();
        hTsListCombo  = NULL;
        hTsSymCombo   = NULL;
        hTsList       = NULL;
        hTsListF100   = NULL;
        hTsListF1000  = NULL;
        hTsFilterCheck = NULL;
        tsSymEntries.clear();
        break;
    }

    return HandleCommonMessages(hWnd, message, wParam, lParam);
}
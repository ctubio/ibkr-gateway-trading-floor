#pragma once

#include <map>
#include <string>
#include <vector>

// Payload passed during HWND creation
struct TsInitData { std::string symbol; int conId; };

void StartTimesalesSearch(); // Forward declaration
HWND StartTimesales(const std::string& symbol = "", int conId = 0);

#define ID_TS_LIST          6003
#define ID_TS_FILTER_CHECK  6004
#define ID_TS_LIST_F100     6005
#define ID_TS_LIST_F1000    6006
#define ID_TS_SEARCH_INPUT  6007
#define ID_TS_SEARCH_LIST   6008

// State mapped per-window to support infinite instances safely
struct TsState {
    HWND hTsList = NULL;
    HWND hTsListF100 = NULL;
    HWND hTsListF1000 = NULL;
    HWND hTsFilterCheck = NULL;
    bool tsFilteredView = false;
    std::string symbol;
    int conId = 0;
};
static std::map<HWND, TsState*> tsStates;

static void UpdateTimesalesRegistry() {
    std::vector<std::string> sessions;
    for (const auto& pair : tsStates) {
        if (pair.second && pair.second->conId != 0 && !pair.second->symbol.empty()) {
            sessions.push_back(std::to_string(pair.second->conId) + "." + pair.second->symbol);
        }
    }
    Settings_SaveTimesales(sessions);
}

// ── Column definitions ────────────────────────────────────────────────────────
struct TsCol { const char* header; int width; int fmt; };
static const TsCol tsCols[] = {
    { "Price",    60, LVCFMT_RIGHT },
    { "Size",     40, LVCFMT_RIGHT },
    { "Time",     90, LVCFMT_LEFT  },
    { "Exchange", 90, LVCFMT_LEFT  },
};
static const int TS_COL_COUNT = (int)(sizeof(tsCols) / sizeof(tsCols[0]));

// ── ListView helpers ──────────────────────────────────────────────────────────
static HWND Ts_CreateListView(HWND hParent, int id, HINSTANCE hInst) {
    HWND hList = CreateWindowExA(
        WS_EX_CLIENTEDGE, "SysListView32", "",
        WS_CHILD | WS_BORDER | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_NOSORTHEADER,
        0, 0, 10, 10, hParent, (HMENU)(intptr_t)id, hInst, NULL);

    DWORD exStyle = LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER;
    if (Settings_DarkMode()) exStyle |= LVS_EX_GRIDLINES;
    ListView_SetExtendedListViewStyle(hList, exStyle);

    LVCOLUMNA lvc = {};
    lvc.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_FMT;
    for (int i = 0; i < TS_COL_COUNT; ++i) {
        lvc.cx = tsCols[i].width; lvc.pszText = (LPSTR)tsCols[i].header; lvc.fmt = tsCols[i].fmt;
        ListView_InsertColumn(hList, i, &lvc);
    }
    return hList;
}

static void Ts_InsertTick(HWND hList, double price, double size, const std::string& time, const std::string& exchange) {
    char buf[32]; snprintf(buf, sizeof(buf), "%.2f", price);
    LVITEMA lvi = {}; lvi.mask = LVIF_TEXT; lvi.iItem = 0; lvi.pszText = buf;
    ListView_InsertItem(hList, &lvi);
    snprintf(buf, sizeof(buf), "%.0f", size);
    ListView_SetItemText(hList, 0, 1, buf);
    ListView_SetItemText(hList, 0, 2, (LPSTR)time.c_str());
    ListView_SetItemText(hList, 0, 3, (LPSTR)exchange.c_str());

    int count = ListView_GetItemCount(hList);
    if (count > 500) ListView_DeleteItem(hList, count - 1);
}

// ── Layout ────────────────────────────────────────────────────────────────────
static void Ts_Layout(HWND hWnd, TsState* state) {
    if (!state || !state->hTsList) return;
    RECT rc; GetClientRect(hWnd, &rc);
    int availH = rc.bottom - 24; 
    int availW = rc.right;

    if (state->tsFilteredView) {
        int leftW = availW / 2; int rightW = availW - leftW; int halfH = availH / 2;
        ShowWindow(state->hTsList, SW_SHOW); ShowWindow(state->hTsListF100, SW_SHOW); ShowWindow(state->hTsListF1000, SW_SHOW);
        MoveWindow(state->hTsList, 0, 0, leftW, availH, TRUE);
        MoveWindow(state->hTsListF100, leftW, 0, rightW, halfH, TRUE);
        MoveWindow(state->hTsListF1000, leftW, halfH, rightW, availH - halfH, TRUE);
    } else {
        ShowWindow(state->hTsListF100, SW_HIDE); ShowWindow(state->hTsListF1000, SW_HIDE); ShowWindow(state->hTsList, SW_SHOW);
        MoveWindow(state->hTsList, 0, 0, availW, availH, TRUE);
    }
    SetWindowPos(state->hTsFilterCheck, NULL, 8, rc.bottom - 20, 100, 16, SWP_NOZORDER | SWP_NOACTIVATE);
}

// ── Search Popup Elements ─────────────────────────────────────────────────────
static std::vector<std::string> tsSearchResults;

LRESULT CALLBACK TsSearchEditSubclass(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    if (msg == WM_KEYDOWN) {
        HWND hTsSearchList = GetDlgItem(GetParent(hWnd), ID_TS_SEARCH_LIST);
        bool vis = IsWindowVisible(hTsSearchList);
        if (wParam == VK_DOWN && vis) {
            int count = SendMessage(hTsSearchList, LB_GETCOUNT, 0, 0);
            int sel = SendMessage(hTsSearchList, LB_GETCURSEL, 0, 0);
            if (sel + 1 < count) SendMessage(hTsSearchList, LB_SETCURSEL, sel + 1, 0);
            return 0;
        }
        if (wParam == VK_UP && vis) {
            int sel = SendMessage(hTsSearchList, LB_GETCURSEL, 0, 0);
            if (sel > 0) SendMessage(hTsSearchList, LB_SETCURSEL, sel - 1, 0);
            return 0;
        }
        if (wParam == VK_RETURN && vis) {
            int sel = SendMessage(hTsSearchList, LB_GETCURSEL, 0, 0);
            if (sel != LB_ERR && sel < (int)tsSearchResults.size()) {
                std::string r = tsSearchResults[sel];
                auto dot = r.find('.');
                if (dot != std::string::npos) {
                    int cid = std::stoi(r.substr(0, dot));
                    std::string rest = r.substr(dot + 1);
                    auto d2 = rest.find('.');
                    StartTimesales((d2 != std::string::npos) ? rest.substr(0, d2) : rest, cid);
                }
                DestroyWindow(GetParent(hWnd));
            }
            return 0;
        }
        if (wParam == VK_ESCAPE) { DestroyWindow(GetParent(hWnd)); return 0; }
    }
    if (msg == WM_NCDESTROY) RemoveWindowSubclass(hWnd, TsSearchEditSubclass, uIdSubclass);
    return DefSubclassProc(hWnd, msg, wParam, lParam);
}

LRESULT CALLBACK TsSearchListSubclass(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    if (msg == WM_LBUTTONUP) {
        int sel = SendMessage(hWnd, LB_GETCURSEL, 0, 0);
        if (sel != LB_ERR && sel < (int)tsSearchResults.size()) {
            std::string r = tsSearchResults[sel];
            auto dot = r.find('.');
            if (dot != std::string::npos) {
                int cid = std::stoi(r.substr(0, dot));
                std::string rest = r.substr(dot + 1);
                auto d2 = rest.find('.');
                StartTimesales((d2 != std::string::npos) ? rest.substr(0, d2) : rest, cid);
            }
            DestroyWindow(GetParent(hWnd));
        }
    }
    if (msg == WM_NCDESTROY) RemoveWindowSubclass(hWnd, TsSearchListSubclass, uIdSubclass);
    return DefSubclassProc(hWnd, msg, wParam, lParam);
}

LRESULT CALLBACK WndProcTsSearch(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE: {
            HINSTANCE hInst = ((LPCREATESTRUCT)lParam)->hInstance;
            CreateWindowA("STATIC", "Search Symbol:", WS_CHILD | WS_VISIBLE, 10, 10, 150, 20, hWnd, NULL, hInst, NULL);
            HWND hTsSearchEdit = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_UPPERCASE | ES_AUTOHSCROLL, 10, 30, 240, 24, hWnd, (HMENU)ID_TS_SEARCH_INPUT, hInst, NULL);
            SetWindowSubclass(hTsSearchEdit, TsSearchEditSubclass, 1, 0);
            HWND hTsSearchList = CreateWindowA("LISTBOX", "", WS_CHILD | WS_VISIBLE | WS_BORDER | LBS_NOTIFY, 10, 60, 240, 150, hWnd, (HMENU)ID_TS_SEARCH_LIST, hInst, NULL);
            SetWindowSubclass(hTsSearchList, TsSearchListSubclass, 2, 0);
            SetFocus(hTsSearchEdit);
            break;
        }
        case WM_COMMAND:
            if (LOWORD(wParam) == ID_TS_SEARCH_INPUT && HIWORD(wParam) == EN_CHANGE) {
                char t[256] = {}; GetWindowTextA(GetDlgItem(hWnd, ID_TS_SEARCH_INPUT), t, sizeof(t));
                if (strlen(t) > 0) { api.setSymbolSearchWindow(hWnd); api.searchSymbols(t); }
                else { SendMessage(GetDlgItem(hWnd, ID_TS_SEARCH_LIST), LB_RESETCONTENT, 0, 0); tsSearchResults.clear(); }
            }
            break;
        case WM_SYMBOL_RESULTS: {
            tsSearchResults = api.getSymbolResults();
            HWND hTsSearchList = GetDlgItem(hWnd, ID_TS_SEARCH_LIST);
            SendMessage(hTsSearchList, LB_RESETCONTENT, 0, 0);
            for (const auto& r : tsSearchResults) {
                auto d = r.find('.');
                std::string disp = (d != std::string::npos) ? r.substr(d + 1) : r;
                SendMessageA(hTsSearchList, LB_ADDSTRING, 0, (LPARAM)disp.c_str());
            }
            break;
        }
        case WM_DESTROY:
            api.setSymbolSearchWindow(NULL);
            break;
    }
    return HandleCommonMessages(hWnd, message, wParam, lParam);
}

void StartTimesalesSearch() {
    static bool registered = false;
    if (!registered) {
        WNDCLASS wc = { 0 };
        wc.lpfnWndProc = WndProcTsSearch;
        wc.hInstance = GetModuleHandle(NULL);
        wc.lpszClassName = TIMESALES_SEARCH_CLASS_NAME;
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        RegisterClass(&wc);
        registered = true;
    }
    HWND hWnd = CreateWindowExA(WS_EX_DLGMODALFRAME | WS_EX_TOPMOST, TIMESALES_SEARCH_CLASS_NAME, "Time & Sales: Search Symbol", 
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE, 
        (GetSystemMetrics(SM_CXSCREEN) - 260) / 2, (GetSystemMetrics(SM_CYSCREEN) - 240) / 2, 275, 260, 
        NULL, NULL, GetModuleHandle(NULL), NULL);
    ApplyDarkMode(hWnd);
}

HWND StartTimesales(const std::string& symbol, int conId) {
    if (symbol.empty() || conId == 0) {
        StartTimesalesSearch();
        return NULL;
    }
    std::string key = TIMESALES_CLASS_NAME + std::string("_") + std::to_string(conId);
    TsInitData* data = new TsInitData{symbol, conId};
    return StartGenericWindow(TIMESALES_CLASS_NAME, ("Time & Sales: " + symbol).c_str(), L"IBKRGatewayClient.Timesales", 380, 500, NULL, key, data);
}

// ── Window procedure ──────────────────────────────────────────────────────────
LRESULT CALLBACK WndProcTimesales(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    TsState* state = nullptr;
    if (message != WM_CREATE) {
        auto it = tsStates.find(hWnd);
        if (it != tsStates.end()) state = it->second;
    }

    switch (message) {
    case WM_CREATE: {
        HINSTANCE hInst = ((LPCREATESTRUCT)lParam)->hInstance;
        TsInitData* data = (TsInitData*)(((LPCREATESTRUCT)lParam)->lpCreateParams);
        state = new TsState();
        if (data) {
            state->symbol = data->symbol;
            state->conId = data->conId;
            delete data;
        }
        tsStates[hWnd] = state;

        state->hTsList      = Ts_CreateListView(hWnd, ID_TS_LIST,       hInst);
        state->hTsListF100  = Ts_CreateListView(hWnd, ID_TS_LIST_F100,  hInst);
        state->hTsListF1000 = Ts_CreateListView(hWnd, ID_TS_LIST_F1000, hInst);
        ShowWindow(state->hTsList, SW_SHOW);

        state->hTsFilterCheck = CreateWindowA("BUTTON", "Filter Size", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 0, 0, 100, 16, hWnd, (HMENU)ID_TS_FILTER_CHECK, hInst, NULL);
        
        api.setTimesalesWindow(hWnd, state->conId, state->symbol);
        api.addApiUpdateWindow(hWnd);
        UpdateTimesalesRegistry(); // Ensure registry is immediately aware of this new instance
        break;
    }

    case WM_SIZE:
        Ts_Layout(hWnd, state);
        return 0;

    case WM_COMMAND:
        if (LOWORD(wParam) == ID_TS_FILTER_CHECK && HIWORD(wParam) == BN_CLICKED && state) {
            state->tsFilteredView = (SendMessage(state->hTsFilterCheck, BM_GETCHECK, 0, 0) == BST_CHECKED);
            if (state->tsFilteredView) {
                ListView_DeleteAllItems(state->hTsListF100); ListView_DeleteAllItems(state->hTsListF1000);
            }
            Ts_Layout(hWnd, state);
        }
        break;

    case WM_TIMESALES_TICK: {
        auto* tick = reinterpret_cast<TradingAPI::TsTickEntry*>(lParam);
        if (state) {
            Ts_InsertTick(state->hTsList, tick->price, tick->size, tick->time, tick->exchange);
            if (state->tsFilteredView) {
                if (tick->size >= 100.0) Ts_InsertTick(state->hTsListF100, tick->price, tick->size, tick->time, tick->exchange);
                if (tick->size >= 1000.0) Ts_InsertTick(state->hTsListF1000, tick->price, tick->size, tick->time, tick->exchange);
            }
        }
        delete tick;
        break;
    }

    case WM_NOTIFY: {
        NMHDR* hdr = (NMHDR*)lParam;
        if (hdr->code != NM_CUSTOMDRAW) break;
        if (hdr->idFrom != ID_TS_LIST && hdr->idFrom != ID_TS_LIST_F100 && hdr->idFrom != ID_TS_LIST_F1000) break;
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

    case WM_API_UPDATE: {
        if (state) {
            if (api.isMarketDataConnected() && api.isTradingConnected()) {
                api.setTimesalesWindow(hWnd, state->conId, state->symbol);
            } else {
                ListView_DeleteAllItems(state->hTsList);
                if (state->hTsListF100)  ListView_DeleteAllItems(state->hTsListF100);
                if (state->hTsListF1000) ListView_DeleteAllItems(state->hTsListF1000);
            }
        }
        break;
    }

    case WM_DESTROY:
        api.unsetTimesalesWindow(hWnd);
        api.removeApiUpdateWindow(hWnd);
        if (state) { delete state; tsStates.erase(hWnd); }
        UpdateTimesalesRegistry(); // Ensure registry forgets this instance immediately
        break;
    }
    return HandleCommonMessages(hWnd, message, wParam, lParam);
}
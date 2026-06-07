#pragma once

int windowMarketWidth = 414;
int windowMarketHeight = 500;

void StartMarketSearch(); // Forward declaration
void StartMarket(const std::string& symbol = "", int conId = 0);

#define ID_TS_LIST          6003
#define ID_TS_FILTER_CHECK  6004
#define ID_TS_LIST_F100     6005
#define ID_TS_LIST_F1000    6006
#define ID_TS_SEARCH_INPUT  6007
#define ID_TS_SEARCH_LIST   6008
#define ID_TS_L2_LIST       6009   // Level 2 depth SysListView32 (left panel)
#define ID_TS_SPEAKER       6010   // Speaker icon for TTS

#define TIMER_TS_SPEAKER    0xC020  // WM_TIMER id for per-market TTS (21s)

// ── Layout constants ─────────────────────────────────────────────────────────
// Header layout (left → right):
//   [speaker icon]   (far left, vertically centred)
//   [filter checkbox](far left, below speaker)
//   [O: x  C: x  H: x  L: x]  (row 1 of stats, after left controls)
//   [Pos: x  Avg: x]           (row 2 of stats)
//   <---- flexible gap ---->
//   [178.00  +1.00  0.56%]     (large last + change, right-aligned just left of Ask/Bid)
//   [Ask  182.87  x 154]       (row 1, right block)
//   [Bid  177.00  x 196]       (row 2, right block)
static const int HEADER_H = 52;   // two-row header height
static const int L2_W     = 224;  // Fixed width of the Level 2 depth panel
static const int ORDER_BAR_H = 38;

static ListViewZoomData MarketZoomData = { NULL, NULL, 14, "Zoom_Market" };

// State mapped per-window to support infinite instances safely
struct TsState {
    HWND hTsList = NULL;
    HWND hTsListF100 = NULL;
    HWND hTsListF1000 = NULL;
    HWND hTsFilterCheck = NULL;
    HWND hL2List = NULL;
    bool tsFilteredView = false;
    std::string symbol;
    int conId = 0;

    // ── Level 1 quote (updated via WM_MARKET_L1) ─────────────────────────────
    TradingAPI::Level1Info l1Info;

    // ── Portfolio snapshot ────────────────────────────────────────────────────
    double position = 0.0;
    double avgPrice = 0.0;

    // ── Cached fonts ──────────────────────────────────────────────────────────
    HFONT hBigFont     = NULL;   // ~22pt bold — large last price
    HFONT hStatusFont      = NULL;   // ~14pt regular — change / bid-ask labels / stats
    HFONT hOrderFont       = NULL;   // ~20pt bold — order entry controls
    HFONT hSmallFont = NULL;   // ~13pt regular — smaller stats below change
    HFONT hSpeakerFont = NULL;   // Segoe MDL2 Assets — speaker glyph

    // ── TTS state ─────────────────────────────────────────────────────────────
    ISpVoice* hTtsVoice    = nullptr;
    bool      ttsOn        = false;
    bool      ttsComInit   = false;
    HWND      hSpeakerBtn  = NULL;

    // ── Splitter state ────────────────────────────────────────────────────────
    float splitX = 0.5f;
    float splitY = 0.5f;
    int dragMode = 0;

    // ── Order entry bar ───────────────────────────────────────────────────────
    HWND  hOrderLabel    = NULL;
    HWND  hOrderPrice    = NULL;
    HWND  hOrderQty      = NULL;
    bool  orderBarVisible = false;
    std::string orderSide;   // "BUY" or "SELL"
};
static std::map<HWND, TsState*> tsStates;

static LRESULT CALLBACK Market_ListForwardCtrlProc(
    HWND hList, UINT msg, WPARAM wParam, LPARAM lParam,
    UINT_PTR uIdSubclass, DWORD_PTR /*dwRefData*/)
{
    if (msg == WM_KEYDOWN && (wParam == VK_CONTROL || wParam == VK_ESCAPE))
        SendMessage(GetParent(hList), WM_KEYDOWN, wParam, lParam);
    if (msg == WM_NCDESTROY)
        RemoveWindowSubclass(hList, Market_ListForwardCtrlProc, uIdSubclass);
    return DefSubclassProc(hList, msg, wParam, lParam);
}

static void UpdateMarketRegistry() {
    std::vector<std::string> sessions;
    for (const auto& pair : tsStates) {
        if (pair.second && pair.second->conId != 0 && !pair.second->symbol.empty()) {
            sessions.push_back(std::to_string(pair.second->conId) + "." + pair.second->symbol);
        }
    }
    Settings_SaveMarket(sessions);
}

// ── Column definitions ────────────────────────────────────────────────────────
struct TsCol { const char* header; int width; int fmt; };
static const TsCol tsCols[] = {
    { "Price",    60, LVCFMT_RIGHT },
    { "Size",     40, LVCFMT_RIGHT },
    { "Time",     70, LVCFMT_LEFT  },
};
static const int TS_COL_COUNT = (int)(sizeof(tsCols) / sizeof(tsCols[0]));

// ── L2 column definitions ─────────────────────────────────────────────────────
struct L2Col { const char* header; int width; int fmt; };
static const L2Col l2Cols[] = {
    { "Size",  46, LVCFMT_RIGHT },
    { "Bid",   64, LVCFMT_RIGHT },
    { "Ask",   64, LVCFMT_RIGHT },
    { "Size",  46, LVCFMT_RIGHT },
};
static const int L2_COL_COUNT = (int)(sizeof(l2Cols) / sizeof(l2Cols[0]));

static HWND Market_CreateL2List(HWND hParent, HINSTANCE hInst) {
    HWND hList = CreateWindowExA(
        WS_EX_CLIENTEDGE, "SysListView32", "",
        WS_CHILD | WS_BORDER | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_NOSORTHEADER,
        0, 0, L2_W, 100, hParent, (HMENU)(intptr_t)ID_TS_L2_LIST, hInst, NULL);
    ListView_SetExtendedListViewStyle(hList, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
    LVCOLUMNA lvc = {};
    lvc.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_FMT;
    for (int i = 0; i < L2_COL_COUNT; ++i) {
        lvc.cx = l2Cols[i].width; lvc.pszText = (LPSTR)l2Cols[i].header; lvc.fmt = l2Cols[i].fmt;
        ListView_InsertColumn(hList, i, &lvc);
        if (i == 0) {
            LVCOLUMN lvcUpdate = { 0 };
            lvcUpdate.mask = LVCF_FMT;
            lvcUpdate.fmt = l2Cols[i].fmt;
            ListView_SetColumn(hList, i, &lvcUpdate);
        }
    }
    return hList;
}

static HWND TimeSales_CreateListView(HWND hParent, int id, HINSTANCE hInst) {
    HWND hList = CreateWindowExA(
        WS_EX_CLIENTEDGE, "SysListView32", "",
        WS_CHILD | WS_BORDER | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_NOSORTHEADER,
        0, 0, 10, 10, hParent, (HMENU)(intptr_t)id, hInst, NULL);
    ListView_SetExtendedListViewStyle(hList, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
    LVCOLUMNA lvc = {};
    lvc.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_FMT;
    for (int i = 0; i < TS_COL_COUNT; ++i) {
        lvc.cx = tsCols[i].width; lvc.pszText = (LPSTR)tsCols[i].header; lvc.fmt = tsCols[i].fmt;
        ListView_InsertColumn(hList, i, &lvc);
    }
    return hList;
}

static void TimeSales_InsertTick(HWND hList, double price, double size, const std::string& time) {
    char buf[32]; snprintf(buf, sizeof(buf), "%.2f", price);
    LVITEMA lvi = {}; lvi.mask = LVIF_TEXT; lvi.iItem = 0; lvi.pszText = buf;
    ListView_InsertItem(hList, &lvi);
    snprintf(buf, sizeof(buf), "%.0f", size);
    ListView_SetItemText(hList, 0, 1, buf);
    ListView_SetItemText(hList, 0, 2, (LPSTR)time.c_str());
    int count = ListView_GetItemCount(hList);
    if (count > 500) ListView_DeleteItem(hList, count - 1);
}

// ── Right block geometry (shared by paint and layout) ────────────────────────
// Ask/Bid block is fixed-width, anchored to the right edge.
// label(28) + price(74) + " x "(16) + size(44) = 162 px + 6 margin = 168 from right
static const int RB_LABEL_W = 28;
static const int RB_PRICE_W = 74;
static const int RB_SEP_W   = 16;
static const int RB_SIZE_W  = 44;
static const int RB_TOTAL   = RB_LABEL_W + RB_PRICE_W + RB_SEP_W + RB_SIZE_W;
static const int RB_MARGIN  = 4;

// Left controls block (speaker + checkbox): fixed width anchored to left edge.
static const int LC_ICON_W  = 22;   // speaker icon width
static const int LC_MARGIN  = 4;    // left margin and gap between icon and stats

// ── Layout ────────────────────────────────────────────────────────────────────
static void Market_Layout(HWND hWnd, TsState* state) {
    if (!state || !state->hTsList) return;
    RECT rc; GetClientRect(hWnd, &rc);

    const int hdrH  = HEADER_H;
    const int bodyY = hdrH;
    const int barH  = (state->orderBarVisible) ? ORDER_BAR_H : 0;
    const int bodyH = rc.bottom - hdrH - barH;
    const int bodyW = rc.right;

    // ── Speaker button: far left, vertically centred in top half of header ───
    if (state->hSpeakerBtn) {
        int btnY = (hdrH / 2 - 14) / 2;   // centred in top row
        SetWindowPos(state->hSpeakerBtn, NULL,
                     LC_MARGIN, btnY, LC_ICON_W, 22,
                     SWP_NOZORDER | SWP_NOACTIVATE);
    }

    // ── Filter checkbox: far left, centred in bottom half of header ──────────
    if (state->hTsFilterCheck) {
        int chkY = hdrH / 2 + (hdrH / 2 - 16) / 2;
        SetWindowPos(state->hTsFilterCheck, NULL,
                     LC_MARGIN + (LC_ICON_W - 16) / 2, chkY, 16, 16,
                     SWP_NOZORDER | SWP_NOACTIVATE);
    }

    // ── Level 2 panel ─────────────────────────────────────────────────────────
    if (state->hL2List)
        MoveWindow(state->hL2List, 0, bodyY, L2_W, bodyH, TRUE);

    // ── T&S area ──────────────────────────────────────────────────────────────
    const int tsX = L2_W;
    const int tsW = bodyW - L2_W;

    if (state->tsFilteredView) {
        const int splitThick = 6;
        int leftW  = (int)(tsW * state->splitX) - splitThick / 2;
        int rightW = tsW - leftW - splitThick;
        int topH   = (int)(bodyH * state->splitY) - splitThick / 2;
        int botH   = bodyH - topH - splitThick;

        ShowWindow(state->hTsList,      SW_SHOW);
        ShowWindow(state->hTsListF100,  SW_SHOW);
        ShowWindow(state->hTsListF1000, SW_SHOW);
        MoveWindow(state->hTsList,      tsX,                        bodyY,                     leftW,  bodyH, TRUE);
        MoveWindow(state->hTsListF100,  tsX + leftW + splitThick,   bodyY,                     rightW, topH,  TRUE);
        MoveWindow(state->hTsListF1000, tsX + leftW + splitThick,   bodyY + topH + splitThick, rightW, botH,  TRUE);
    } else {
        ShowWindow(state->hTsListF100,  SW_HIDE);
        ShowWindow(state->hTsListF1000, SW_HIDE);
        ShowWindow(state->hTsList,      SW_SHOW);
        MoveWindow(state->hTsList, tsX, bodyY, tsW, bodyH, TRUE);
    }

    // ── Order entry bar ───────────────────────────────────────────────────────
    if (state->hOrderLabel && state->hOrderPrice && state->hOrderQty) {
        const int m    = 8;
        const int lblW = 42;
        const int editH = ORDER_BAR_H - 6;
        const int editY = rc.bottom - ORDER_BAR_H + (ORDER_BAR_H - editH) / 2;
        const int lblY  = rc.bottom - ORDER_BAR_H + (ORDER_BAR_H - 18) / 2;
        int availW = rc.right - m * 3 - lblW;
        int priceW = availW / 2;
        int qtyW   = availW - priceW - 1;

        SetWindowPos(state->hOrderLabel, NULL, m, lblY, lblW, 18,
                     SWP_NOZORDER | SWP_NOACTIVATE);
        SetWindowPos(state->hOrderPrice, NULL, m + lblW + m, editY, priceW, editH,
                     SWP_NOZORDER | SWP_NOACTIVATE);
        SetWindowPos(state->hOrderQty, NULL, m + lblW + m + priceW + m, editY, qtyW, editH,
                     SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

static void OrderBar_Show(HWND hWnd, TsState* state, const std::string& side) {
    if (!state || !state->hOrderLabel) return;
    state->orderSide = side;
    state->orderBarVisible = true;
    SetWindowTextA(state->hOrderLabel, side.c_str());
    SetCtrlColor(state->hOrderLabel, (state->orderSide == "BUY") ? COINS_CLR_GREEN : COINS_CLR_RED);

    // Pre-fill price from current last / bid / ask
    double suggestedPrice = 0.0;
    if (side == "BUY"  && state->l1Info.ask > 0.0) suggestedPrice = state->l1Info.ask;
    else if (side == "SELL" && state->l1Info.bid > 0.0) suggestedPrice = state->l1Info.bid;
    else if (state->l1Info.last > 0.0) suggestedPrice = state->l1Info.last;
    else if (state->l1Info.prevClose > 0.0) suggestedPrice = state->l1Info.prevClose;
    
    char buf[32];
    snprintf(buf, sizeof(buf), "%.2f", suggestedPrice);
    SetWindowTextA(state->hOrderPrice, buf);
    char buqty[32];
    int qty = (int)Settings_Load("OrderQty", 100);
    snprintf(buqty, sizeof(buqty), "%d", qty);
    SetWindowTextA(state->hOrderQty, buqty);

    ShowWindow(state->hOrderLabel, SW_SHOW);
    ShowWindow(state->hOrderPrice, SW_SHOW);
    ShowWindow(state->hOrderQty,   SW_SHOW);

    Market_Layout(hWnd, state);
    SetFocus(state->hOrderPrice);
    int len = GetWindowTextLengthA(state->hOrderPrice);
    SendMessageA(state->hOrderPrice, EM_SETSEL, len, len);
}

void Market_Layout_HideBar(HWND hWnd, TsState* state) {
    ShowWindow(state->hOrderLabel, SW_HIDE);
    ShowWindow(state->hOrderPrice, SW_HIDE);
    ShowWindow(state->hOrderQty,   SW_HIDE);
    state->orderBarVisible = false;
    Market_Layout(hWnd, state);
}

// Subclass for the order-bar price and qty edit controls.
// uIdSubclass == 1 → price (step 0.01, 2 dec)
// uIdSubclass == 2 → qty   (step 1,    0 dec)
static LRESULT CALLBACK OrderBar_EditSubclassProc(
    HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam,
    UINT_PTR uIdSubclass, DWORD_PTR /*dwRefData*/)
{
    if (msg == WM_GETDLGCODE)
        return DefSubclassProc(hWnd, msg, wParam, lParam)
               | DLGC_WANTTAB | DLGC_WANTARROWS | DLGC_WANTALLKEYS;

    if (msg == WM_CHAR) {
        if (wParam == VK_ESCAPE || wParam == VK_TAB || wParam == VK_RETURN)
            return 0;
    }

    if (msg == WM_KEYDOWN) {
        HWND hMarket = GetParent(hWnd);
        auto it = tsStates.find(hMarket);
        TsState* st = (it != tsStates.end()) ? it->second : nullptr;

        if (wParam == VK_ESCAPE) {
            if (st) {
                api.cancelOrders(st->conId);
            }
            return 0;
        }
        if (wParam == VK_TAB) {
            if (st) {
                HWND hNext = (hWnd == st->hOrderPrice) ? st->hOrderQty : st->hOrderPrice;
                SetFocus(hNext);
                // Place caret at end, no selection
                int len = GetWindowTextLengthA(hNext);
                SendMessageA(hNext, EM_SETSEL, len, len);
            }
            return 0;
        }
        if (wParam == VK_RETURN) {
            if (st) {
                char pBuf[32] = {}, qBuf[32] = {};
                GetWindowTextA(st->hOrderPrice, pBuf, sizeof(pBuf));
                GetWindowTextA(st->hOrderQty,   qBuf, sizeof(qBuf));
                double price = std::atof(pBuf);
                double qty = std::atof(qBuf);
                if (price > 0 && qty > 0) {
                    api.submitOrder(st->conId, st->symbol, st->orderSide, qty, price);
                }
                Market_Layout_HideBar(hMarket, st);
            }
            return 0;
        }
        if (wParam == VK_UP || wParam == VK_DOWN) {
            char buf[32] = {};
            GetWindowTextA(hWnd, buf, sizeof(buf));
            double val  = atof(buf);
            double step = (uIdSubclass == 1) ? 0.01 : 1.0;
            val += (wParam == VK_UP) ? step : -step;
            if (val < 0.0) val = 0.0;
            snprintf(buf, sizeof(buf), (uIdSubclass == 1) ? "%.2f" : "%.0f", val);
            SetWindowTextA(hWnd, buf);
            int len = GetWindowTextLengthA(hWnd);
            SendMessageA(hWnd, EM_SETSEL, len, len);
            return 0;
        }
        
        if (wParam == VK_CONTROL) {
            bool isRight = (lParam & (1 << 24)) != 0;
            if (isRight) {
                if (st->orderBarVisible && st->orderSide == "SELL") {
                    Market_Layout_HideBar(hMarket, st);
                } else {
                    OrderBar_Show(hMarket, st, "SELL");
                }
            } else {
                if (st->orderBarVisible && st->orderSide == "BUY") {
                    Market_Layout_HideBar(hMarket, st);
                } else {
                    OrderBar_Show(hMarket, st, "BUY");
                }
            }
        }
    }
    if (msg == WM_NCDESTROY)
        RemoveWindowSubclass(hWnd, OrderBar_EditSubclassProc, uIdSubclass);
    return DefSubclassProc(hWnd, msg, wParam, lParam);
}

// ── Search Popup ──────────────────────────────────────────────────────────────
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
            if (sel == LB_ERR && SendMessage(hTsSearchList, LB_GETCOUNT, 0, 0) > 0) sel = 0;
            if (sel != LB_ERR && sel < (int)tsSearchResults.size()) {
                std::string r = tsSearchResults[sel];
                auto dot = r.find('.');
                if (dot != std::string::npos) {
                    int cid = std::stoi(r.substr(0, dot));
                    std::string rest = r.substr(dot + 1);
                    auto d2 = rest.find('.');
                    StartMarket((d2 != std::string::npos) ? rest.substr(0, d2) : rest, cid);
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
                StartMarket((d2 != std::string::npos) ? rest.substr(0, d2) : rest, cid);
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
            HWND hTsSearchEdit = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_UPPERCASE | ES_AUTOHSCROLL, 10, 10, 240, 24, hWnd, (HMENU)ID_TS_SEARCH_INPUT, hInst, NULL);
            SetWindowSubclass(hTsSearchEdit, TsSearchEditSubclass, 1, 0);
            HWND hTsSearchList = CreateWindowA("LISTBOX", "", WS_CHILD | WS_VISIBLE | WS_BORDER | LBS_NOTIFY, 10, 40, 240, 180, hWnd, (HMENU)ID_TS_SEARCH_LIST, hInst, NULL);
            SetWindowSubclass(hTsSearchList, TsSearchListSubclass, 2, 0);
            ApplyListViewFont(hTsSearchList, MarketZoomData.hFont, MarketZoomData.hBoldFont, MarketZoomData.fontSize);
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

void StartMarketSearch() {
    HWND hWnd = CreateWindowExA(WS_EX_DLGMODALFRAME | WS_EX_TOPMOST, MARKET_SEARCH_CLASS_NAME, "Market: Search Symbol",
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        (GetSystemMetrics(SM_CXSCREEN) - 268) / 2, (GetSystemMetrics(SM_CYSCREEN) - 255) / 2, 268, 255,
        NULL, NULL, GetModuleHandle(NULL), NULL);
}

void StartMarket(const std::string& symbol, int conId) {
    if (symbol.empty() || conId == 0) {
        StartMarketSearch();
        return;
    }
    std::string key = MARKET_CLASS_NAME + std::string("_") + symbol;
    MarketInitData* data = new MarketInitData{symbol, conId, key};
    StartGenericWindow(MARKET_CLASS_NAME, symbol.c_str(), L"IBKRGatewayClient.Market", windowMarketWidth, windowMarketHeight, NULL, key, data);
}

static int HitTestSplitter(HWND hWnd, TsState* state, int x, int y) {
    if (!state->tsFilteredView) return 0;

    RECT rc; GetClientRect(hWnd, &rc);
    const int bodyH      = rc.bottom - HEADER_H;
    const int tsX        = L2_W;
    const int tsW        = rc.right - L2_W;
    const int splitThick = 6;

    int relX = x - tsX;
    int relY = y - HEADER_H;
    if (relX < 0 || relY < 0 || relY > bodyH) return 0;

    int splitXPos = (int)(tsW * state->splitX);
    int splitYPos = (int)(bodyH * state->splitY);

    if (relX >= splitXPos - splitThick && relX <= splitXPos + splitThick)
        return 1;
    if (relX > splitXPos + splitThick && relY >= splitYPos - splitThick && relY <= splitYPos + splitThick)
        return 2;

    return 0;
}

// ── Formatting helpers ────────────────────────────────────────────────────────
static std::string Market_Fmt(double v, int dec = 2) {
    if (v == 0.0) return "--";
    char buf[32]; snprintf(buf, sizeof(buf), "%.*f", dec, v); return buf;
}
static std::string Market_FmtQty(double v) {
    if (v == 0.0) return "--";
    if (v == (long long)v) { char b[32]; snprintf(b, sizeof(b), "%lld", (long long)v); return b; }
    char b[32]; snprintf(b, sizeof(b), "%.2f", v); return b;
}

// ── L2 list refresh ───────────────────────────────────────────────────────────
static void Market_RefreshL2(HWND hWnd, TsState* state) {
    if (!state || !state->hL2List) return;

    std::vector<TradingAPI::Level2Entry> bids, asks;
    api.getLevel2Snapshot(hWnd, bids, asks);

    HWND hList = state->hL2List;
    SendMessage(hList, WM_SETREDRAW, FALSE, 0);
    ListView_DeleteAllItems(hList);

    int rows = (int)std::max(bids.size(), asks.size());
    for (int i = 0; i < rows; ++i) {
        LVITEMA lvi = {}; lvi.mask = LVIF_TEXT | LVIF_PARAM;
        int hasBid = (i < (int)bids.size()) ? 1 : 0;
        int hasAsk = (i < (int)asks.size()) ? 2 : 0;
        lvi.lParam  = (LPARAM)(hasBid | hasAsk);
        lvi.iItem   = i;
        std::string bidSzStr = hasBid ? Market_FmtQty(bids[i].size) : "";
        lvi.pszText = (LPSTR)bidSzStr.c_str();
        ListView_InsertItem(hList, &lvi);
        std::string bidStr   = hasBid ? Market_Fmt(bids[i].price) : "";
        std::string askStr   = hasAsk ? Market_Fmt(asks[i].price) : "";
        std::string askSzStr = hasAsk ? Market_FmtQty(asks[i].size) : "";
        ListView_SetItemText(hList, i, 1, (LPSTR)bidStr.c_str());
        ListView_SetItemText(hList, i, 2, (LPSTR)askStr.c_str());
        ListView_SetItemText(hList, i, 3, (LPSTR)askSzStr.c_str());
    }
    SendMessage(hList, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(hList, NULL, FALSE);
}

// ── Header paint ──────────────────────────────────────────────────────────────
//
// Fixed-position zones (never move with resize):
//   [LC_MARGIN .. LC_MARGIN+LC_ICON_W]  — speaker icon (top half) + checkbox (bottom half)
//   [rc.right - RB_TOTAL - RB_MARGIN .. rc.right - RB_MARGIN]  — Ask/Bid block
//
// Stats block starts at STATS_X, two rows:
//   Row 1 (top half):    O:  C:  H:  L:
//   Row 2 (bottom half): Pos:  Avg:
//
// Last+Change block: right-aligned to just left of the Ask/Bid block.
//   Drawn as: large last price then smaller change/pct%, right edge = RB_X - 8.
//   Left edge is flexible — the block is right-justified into whatever space remains.
//
static void Market_PaintHeader(HWND hWnd, TsState* state) {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hWnd, &ps);

    RECT rc; GetClientRect(hWnd, &rc);
    const bool     dark       = Settings_DarkMode();
    const COLORREF bgColor    = dark ? DM_BG   : GetSysColor(COLOR_BTNFACE);
    const COLORREF textColor  = dark ? DM_TEXT : GetSysColor(COLOR_WINDOWTEXT);
    const COLORREF labelColor = dark ? RGB(160,160,160) : RGB(110,110,110);
    const COLORREF sepColor   = dark ? RGB(60,60,60)    : RGB(200,200,200);

    // Fill background
    RECT hdrRc = { 0, 0, rc.right, HEADER_H };
    HBRUSH hBgBrush = CreateSolidBrush(bgColor);
    FillRect(hdc, &hdrRc, hBgBrush);
    DeleteObject(hBgBrush);
    SetBkMode(hdc, TRANSPARENT);

    const TradingAPI::Level1Info& L1 = state->l1Info;
    double displayLast = (L1.last > 0.0) ? L1.last : L1.prevClose;

    const int rowH = HEADER_H / 2;   // height of each of the two stat rows

    // ── Color helpers ─────────────────────────────────────────────────────────
    COLORREF openColor  = textColor;
    if (displayLast > 0.0 && L1.open > 0.0)
        openColor = (displayLast >= L1.open) ? COINS_CLR_GREEN : COINS_CLR_RED;
    COLORREF vwapColor  = COINS_CLR_ORANGE;
    COLORREF highColor = (L1.high > 0.0) ? COINS_CLR_GREEN : textColor;
    COLORREF lowColor  = (L1.low  > 0.0) ? COINS_CLR_RED   : textColor;
    // COLORREF posColor  = (state->position > 0.0) ? COINS_CLR_GREEN : (state->position < 0.0) ? COINS_CLR_RED : textColor;
    // COLORREF avgPrColor = textColor;
    // if (displayLast > 0.0 && state->avgPrice > 0.0) avgPrColor = (displayLast >= state->avgPrice) ? COINS_CLR_GREEN : COINS_CLR_RED;

    // ── RIGHT BLOCK: Ask / Bid ────────────────────────────────────────────────
    // Anchored to right edge, two rows.
    const int RB_X = rc.right - RB_TOTAL - RB_MARGIN;

    HFONT hOldFont = (HFONT)SelectObject(hdc, state->hStatusFont);

    // Ask (top row, red)
    {
        SetTextColor(hdc, COINS_CLR_RED);
        RECT lr = { RB_X, 1, RB_X + RB_LABEL_W, rowH };
        DrawTextA(hdc, "Ask", -1, &lr, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

        SelectObject(hdc, state->hBigFont);
        std::string askStr = Market_Fmt(L1.ask);
        RECT pr = { RB_X + RB_LABEL_W, 1, RB_X + RB_LABEL_W + RB_PRICE_W, rowH };
        DrawTextA(hdc, askStr.c_str(), -1, &pr, DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

        SelectObject(hdc, state->hStatusFont);
        char szBuf[32]; snprintf(szBuf, sizeof(szBuf), " x %s", Market_FmtQty(L1.askSize).c_str());
        RECT sr = { RB_X + RB_LABEL_W + RB_PRICE_W, 1, RB_X + RB_TOTAL, rowH };
        DrawTextA(hdc, szBuf, -1, &sr, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    }

    // Bid (bottom row, blue)
    {
        SetTextColor(hdc, COINS_CLR_BLUE);
        RECT lr = { RB_X, rowH, RB_X + RB_LABEL_W, HEADER_H - 1 };
        DrawTextA(hdc, "Bid", -1, &lr, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

        SelectObject(hdc, state->hBigFont);
        std::string bidStr = Market_Fmt(L1.bid);
        RECT pr = { RB_X + RB_LABEL_W, rowH, RB_X + RB_LABEL_W + RB_PRICE_W, HEADER_H - 1 };
        DrawTextA(hdc, bidStr.c_str(), -1, &pr, DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

        SelectObject(hdc, state->hStatusFont);
        char szBuf[32]; snprintf(szBuf, sizeof(szBuf), " x %s", Market_FmtQty(L1.bidSize).c_str());
        RECT sr = { RB_X + RB_LABEL_W + RB_PRICE_W, rowH, RB_X + RB_TOTAL, HEADER_H - 1 };
        DrawTextA(hdc, szBuf, -1, &sr, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    }

    // ── STATS BLOCK: O/C/H/L (row 1) + Pos/Avg (row 2) ──────────────────────
    // Starts right after the left controls (speaker + checkbox).
    const int STATS_X = LC_MARGIN + LC_ICON_W + LC_MARGIN;

    SelectObject(hdc, state->hStatusFont);

    // Row 1: O  C  H  L
    struct StatItem { const char* label; std::string value; COLORREF color; };
    StatItem row1[] = {
        { "C:", Market_Fmt(L1.prevClose), textColor  },
        { "H:", Market_Fmt(L1.high),      highColor  },
        { "W:", Market_Fmt(L1.vwap),      vwapColor  },
    };
    // Row 2: Pos  Avg
    StatItem row2[] = {
        { "O:", Market_Fmt(L1.open),      openColor  },
        { "L:", Market_Fmt(L1.low),       lowColor   },
        { "V:", Market_FmtQty(L1.volume),    textColor  },
        //{ "Pos:", Market_FmtQty(state->position), posColor   },
        //{ "Avg:", Market_Fmt(state->avgPrice),     avgPrColor },
    };

    // Helper: draw a row of stat pairs starting at (startX, y0).
    // Returns the x position after the last pair.
    auto drawStatRow = [&](StatItem* items, int count, int startX, int y0, int y1) -> int {
        int cx = startX;
        const int GAP = 8;
        for (int i = 0; i < count; i++) {
            // label
            char labBuf[16]; snprintf(labBuf, sizeof(labBuf), "%s ", items[i].label);
            SIZE lblSz;
            GetTextExtentPoint32A(hdc, labBuf, (int)strlen(labBuf), &lblSz);
            SetTextColor(hdc, labelColor);
            RECT lr = { cx, y0, cx + lblSz.cx, y1 };
            DrawTextA(hdc, labBuf, -1, &lr, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
            cx += lblSz.cx;

            // value
            SIZE valSz;
            GetTextExtentPoint32A(hdc, items[i].value.c_str(), (int)items[i].value.size(), &valSz);
            SetTextColor(hdc, items[i].color);
            RECT vr = { cx, y0, cx + valSz.cx + 2, y1 };
            DrawTextA(hdc, items[i].value.c_str(), -1, &vr, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
            cx += valSz.cx + GAP;
        }
        return cx;
    };

    drawStatRow(row1, 3, STATS_X, 0,    rowH);
    drawStatRow(row2, 3, STATS_X, rowH, HEADER_H - 1);

    // ── LAST + CHANGE: right-aligned just left of Ask/Bid block ──────────────
    // We measure both pieces then right-justify them to RB_X - 10.
    const int LC_RIGHT = RB_X - 10;   // right edge for the last+change block

    double chg    = L1.change();
    double chgPct = L1.changePct();

    // Measure large last price
    SelectObject(hdc, state->hBigFont);
    std::string lastStr = Market_Fmt(displayLast);
    SIZE lastSz = {};
    GetTextExtentPoint32A(hdc, lastStr.c_str(), (int)lastStr.size(), &lastSz);

    // Measure change text (smaller font)
    char chgBuf[48] = {};
    SIZE chgSz = {};
    bool showChg = (L1.last > 0.0 && (chg != 0.0 || L1.prevClose > 0.0));
    if (showChg) {
        snprintf(chgBuf, sizeof(chgBuf), " %.2f  %.2f%%", chg, chgPct);
        SelectObject(hdc, state->hSmallFont);
        GetTextExtentPoint32A(hdc, chgBuf, (int)strlen(chgBuf), &chgSz);
    }

    // Total width of the last+change block
    int lcBlockW = std::max(lastSz.cx, (showChg ? chgSz.cx : 0)) + 7;

    // Left edge of the block: right-justified to LC_RIGHT, but no further left
    // than STATS_X (so it never overlaps the stats when window is very narrow).
    int lcX = LC_RIGHT - lcBlockW;
    if (lcX < STATS_X) lcX = STATS_X;

    if (lcX < LC_RIGHT) {
        // Draw large last price (vertically centred, full header height)
        SelectObject(hdc, state->hBigFont);
        SetTextColor(hdc, textColor);
        RECT lastRc = { lcX, 0, LC_RIGHT - 7, HEADER_H - 7 };
        DrawTextA(hdc, lastStr.c_str(), -1, &lastRc, DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

        // Draw change/pct% in smaller font immediately to below
        if (showChg) {
            COLORREF chgColor = (chg >= 0.0) ? COINS_CLR_GREEN : COINS_CLR_RED;
            SelectObject(hdc, state->hSmallFont);
            SetTextColor(hdc, chgColor);
            int chgX = lcX;
            if (chgX < LC_RIGHT) {
                RECT chgRc = { chgX, 0, LC_RIGHT - 7, HEADER_H - 3 };
                DrawTextA(hdc, chgBuf, -1, &chgRc, DT_RIGHT | DT_BOTTOM | DT_SINGLELINE | DT_NOPREFIX);
            }
        }
    }

    // ── Bottom separator ──────────────────────────────────────────────────────
    SelectObject(hdc, hOldFont);
    HPEN hSepPen = CreatePen(PS_SOLID, 1, sepColor);
    HPEN hOldPen = (HPEN)SelectObject(hdc, hSepPen);
    MoveToEx(hdc, 0, HEADER_H - 1, NULL);
    LineTo(hdc, rc.right, HEADER_H - 1);
    SelectObject(hdc, hOldPen);
    DeleteObject(hSepPen);

    EndPaint(hWnd, &ps);
}

// ── Market TTS helpers ────────────────────────────────────────────────────────
static bool Market_InitVoice(TsState* state) {
    if (state->hTtsVoice) return true;
    if (!state->ttsComInit) {
        HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
        state->ttsComInit = SUCCEEDED(hr) || (hr == RPC_E_CHANGED_MODE);
    }
    HRESULT hr = CoCreateInstance(CLSID_SpVoice, NULL, CLSCTX_ALL, IID_ISpVoice, (void**)&state->hTtsVoice);
    if (FAILED(hr)) { state->hTtsVoice = nullptr; return false; }
    TTS_ApplySavedVoice(state->hTtsVoice);
    return true;
}

static void Market_SpeakLast(TsState* state) {
    if (!state->hTtsVoice) return;
    double displayLast = (state->l1Info.last > 0.0) ? state->l1Info.last : state->l1Info.prevClose;
    if (displayLast <= 0.0) return;
    char buf[64]; snprintf(buf, sizeof(buf), "%.2f", displayLast);
    std::string s(buf);
    std::wstring ws(s.begin(), s.end());
    ws.erase(std::remove(ws.begin(), ws.end(), L','), ws.end());
    std::replace(ws.begin(), ws.end(), L'.', L',');
    state->hTtsVoice->Speak(ws.c_str(), SVSFlagsAsync | SVSFPurgeBeforeSpeak, NULL);
}

static void Market_ToggleTTS(HWND hWnd, TsState* state) {
    state->ttsOn = !state->ttsOn;
    if (state->ttsOn) {
        if (!Market_InitVoice(state)) { state->ttsOn = false; return; }
        if (state->hSpeakerBtn)
            SetCtrlColor(state->hSpeakerBtn, Settings_DarkMode() ? COINS_CLR_WHITE : COINS_CLR_BLACK);
        SetTimer(hWnd, TIMER_TS_SPEAKER, 21000, NULL);
        Market_SpeakLast(state);
    } else {
        KillTimer(hWnd, TIMER_TS_SPEAKER);
        if (state->hTtsVoice)
            state->hTtsVoice->Speak(NULL, SVSFPurgeBeforeSpeak, NULL);
        if (state->hSpeakerBtn)
            SetCtrlColor(state->hSpeakerBtn, COINS_CLR_GRAY);
    }
    if (state->hSpeakerBtn) InvalidateRect(state->hSpeakerBtn, NULL, TRUE);
}
void Market_RefreshPositionAndAvg(HWND hWnd, TsState* state) {
    if (!state) return;
    std::lock_guard<std::mutex> lk(api.getPortfolioMutex());
    auto& pm = api.getPortfolioMap();
    auto it = pm.find(state->symbol);
    if (it != pm.end()) {
        state->position = it->second.shares;
        state->avgPrice = it->second.avgCost;
        
        SetWindowTextA(hWnd, (state->symbol + ": " + Market_FmtQty(state->position) + " @ " + Market_Fmt(state->avgPrice)).c_str());
    }
}

// ── Window procedure ──────────────────────────────────────────────────────────
LRESULT CALLBACK WndProcMarket(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    TsState* state = nullptr;
    if (message != WM_CREATE) {
        auto it = tsStates.find(hWnd);
        if (it != tsStates.end()) state = it->second;
    }

    switch (message) {
    case WM_CREATE: {
        HINSTANCE hInst = ((LPCREATESTRUCT)lParam)->hInstance;
        MarketInitData* data = (MarketInitData*)(((LPCREATESTRUCT)lParam)->lpCreateParams);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)data);
        state = new TsState();
        if (data) {
            state->symbol = data->symbol;
            state->conId  = data->conId;
        }
        tsStates[hWnd] = state;

        // ── Header fonts ──────────────────────────────────────────────────────
        state->hBigFont = CreateFontA(-22, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
        state->hOrderFont = CreateFontA(-20, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
        state->hStatusFont = CreateFontA(-14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
        state->hSmallFont = CreateFontA(-13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
        state->hSpeakerFont = CreateFontW(-11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe MDL2 Assets");

        // ── Snapshot position + avg price ─────────────────────────────────────
        if (!state->symbol.empty()) {
            Market_RefreshPositionAndAvg(hWnd, state);
        }

        // ── Lists ─────────────────────────────────────────────────────────────
        state->hL2List      = Market_CreateL2List(hWnd, hInst);
        state->hTsList      = TimeSales_CreateListView(hWnd, ID_TS_LIST,       hInst);
        state->hTsListF100  = TimeSales_CreateListView(hWnd, ID_TS_LIST_F100,  hInst);
        state->hTsListF1000 = TimeSales_CreateListView(hWnd, ID_TS_LIST_F1000, hInst);
        SetWindowSubclass(state->hTsList,      Market_ListForwardCtrlProc, 10, 0);
        SetWindowSubclass(state->hTsListF100,  Market_ListForwardCtrlProc, 11, 0);
        SetWindowSubclass(state->hTsListF1000, Market_ListForwardCtrlProc, 12, 0);
        SetWindowSubclass(state->hL2List,      Market_ListForwardCtrlProc, 13, 0);

        {
            HFONT hListFont = CreateFontA(-14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
            SendMessage(state->hTsList,      WM_SETFONT, (WPARAM)hListFont, TRUE);
            SendMessage(state->hTsListF100,  WM_SETFONT, (WPARAM)hListFont, TRUE);
            SendMessage(state->hTsListF1000, WM_SETFONT, (WPARAM)hListFont, TRUE);
            SendMessage(state->hL2List,      WM_SETFONT, (WPARAM)hListFont, TRUE);
        }

        ShowWindow(state->hTsList, SW_SHOW);
        ShowWindow(state->hL2List, SW_SHOW);

        // ── Filter checkbox (far left, below speaker) ─────────────────────────
        state->hTsFilterCheck = CreateWindowA("BUTTON", "",
            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
            0, 0, 16, 16, hWnd, (HMENU)ID_TS_FILTER_CHECK, hInst, NULL);

        {
            HWND hTip = CreateWindowA(TOOLTIPS_CLASS, NULL,
                WS_POPUP | TTS_ALWAYSTIP,
                CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                hWnd, NULL, hInst, NULL);
            TOOLINFOA ti = {};
            ti.cbSize   = sizeof(ti);
            ti.uFlags   = TTF_IDISHWND | TTF_SUBCLASS;
            ti.hwnd     = hWnd;
            ti.uId      = (UINT_PTR)state->hTsFilterCheck;
            ti.lpszText = (LPSTR)"Filter Size x 100 and 1000";
            SendMessage(hTip, TTM_ADDTOOLA, 0, (LPARAM)&ti);
        }

        // ── Speaker button (far left, top half) ───────────────────────────────
        state->hSpeakerBtn = CreateWindowW(L"STATIC", SPEAKER_GLYPH,
            WS_CHILD | WS_VISIBLE | SS_CENTER | SS_NOTIFY,
            0, 0, 22, 22, hWnd, (HMENU)ID_TS_SPEAKER, hInst, NULL);
        SendMessage(state->hSpeakerBtn, WM_SETFONT, (WPARAM)state->hSpeakerFont, TRUE);
        SetCtrlColor(state->hSpeakerBtn, COINS_CLR_GRAY);

        // ── Order entry bar (hidden until Ctrl key pressed) ───────────────────
        state->hOrderLabel = CreateWindowA("STATIC", "BUY",
            WS_CHILD | SS_CENTER | SS_CENTERIMAGE,
            0, 0, 42, 26, hWnd, NULL, hInst, NULL);

        state->hOrderPrice = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "0.00",
            WS_CHILD | ES_AUTOHSCROLL | ES_CENTER,
            0, 0, 10, 10, hWnd, NULL, hInst, NULL);
        SetWindowSubclass(state->hOrderPrice, OrderBar_EditSubclassProc, 1, 0);

        state->hOrderQty = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "1",
            WS_CHILD | ES_AUTOHSCROLL | ES_CENTER,
            0, 0, 10, 10, hWnd, NULL, hInst, NULL);
        SetWindowSubclass(state->hOrderQty, OrderBar_EditSubclassProc, 2, 0);

        // Apply font to order bar controls
        if (state->hOrderFont) {
            SendMessage(state->hOrderLabel, WM_SETFONT, (WPARAM)state->hOrderFont, TRUE);
            SendMessage(state->hOrderPrice, WM_SETFONT, (WPARAM)state->hOrderFont, TRUE);
            SendMessage(state->hOrderQty,   WM_SETFONT, (WPARAM)state->hOrderFont, TRUE);
        }

        // Restore splitter + filter
        if (!state->symbol.empty()) {
            Settings_LoadMarketSplitter(state->symbol, state->splitX, state->splitY);
            char filterKey[256];
            sprintf(filterKey, "TsFilterSize_%s", state->symbol.c_str());
            RECT rc; GetWindowRect(hWnd, &rc);
            if (Settings_Load(filterKey, 0)) {
                state->tsFilteredView = true;
                SendMessage(state->hTsFilterCheck, BM_SETCHECK, BST_CHECKED, 0);
                MoveWindow(hWnd, rc.left, rc.top, windowMarketWidth + 181, windowMarketHeight, TRUE);
            } else {
                MoveWindow(hWnd, rc.left, rc.top, windowMarketWidth, windowMarketHeight, TRUE);
            }
        }

        api.setMarketWindow(hWnd, state->conId, state->symbol);
        api.addApiUpdateWindow(hWnd);
        UpdateMarketRegistry();
        break;
    }

    case WM_SIZE:
        Market_Layout(hWnd, state);
        InvalidateRect(hWnd, NULL, FALSE);
        return 0;

    case WM_PAINT:
        if (state) Market_PaintHeader(hWnd, state);
        else { PAINTSTRUCT ps; BeginPaint(hWnd, &ps); EndPaint(hWnd, &ps); }
        return 0;

    case WM_KEYDOWN: {
        if (!state) break;
        if (wParam == VK_ESCAPE) {
            if (state) {
                api.cancelOrders(state->conId);
            }
            return 0;
        }
        if (wParam == VK_CONTROL) {
            bool isRight = (lParam & (1 << 24)) != 0;
            if (isRight) {
                if (state->orderBarVisible && state->orderSide == "SELL") {
                    Market_Layout_HideBar(hWnd, state);
                } else {
                    OrderBar_Show(hWnd, state, "SELL");
                }
            } else {
                if (state->orderBarVisible && state->orderSide == "BUY") {
                    Market_Layout_HideBar(hWnd, state);
                } else {
                    OrderBar_Show(hWnd, state, "BUY");
                }
            }
        }
        break;
    }

    case WM_MARKET_L1: {
        if (!state) break;
        TradingAPI::Level1Info fresh;
        if (api.getLevel1Data(hWnd, fresh)) {
            if (fresh.last      > 0.0) state->l1Info.last      = fresh.last;
            if (fresh.open      > 0.0) state->l1Info.open      = fresh.open;
            if (fresh.prevClose > 0.0) state->l1Info.prevClose = fresh.prevClose;
            if (fresh.high      > 0.0) state->l1Info.high      = fresh.high;
            if (fresh.low       > 0.0) state->l1Info.low       = fresh.low;
            if (fresh.bid       > 0.0) state->l1Info.bid       = fresh.bid;
            if (fresh.ask       > 0.0) state->l1Info.ask       = fresh.ask;
            if (fresh.bidSize   > 0.0) state->l1Info.bidSize   = fresh.bidSize;
            if (fresh.askSize   > 0.0) state->l1Info.askSize   = fresh.askSize;
        }
        TradingAPI::WatchlistInfo wi;
        if (api.getWatchlistData(state->conId, state->symbol, wi)) {
            if (wi.open      > 0.0 && state->l1Info.open      == 0.0) state->l1Info.open      = wi.open;
            if (wi.prevClose > 0.0 && state->l1Info.prevClose == 0.0) state->l1Info.prevClose = wi.prevClose;
            if (wi.high      > 0.0 && state->l1Info.high      == 0.0) state->l1Info.high      = wi.high;
            if (wi.low       > 0.0 && state->l1Info.low       == 0.0) state->l1Info.low       = wi.low;
            if (wi.vwap      > 0.0 && state->l1Info.vwap      == 0.0) state->l1Info.vwap      = wi.vwap;
        }
        Market_RefreshPositionAndAvg(hWnd, state);
        RECT hdrRc; GetClientRect(hWnd, &hdrRc); hdrRc.bottom = HEADER_H;
        InvalidateRect(hWnd, &hdrRc, FALSE);
        break;
    }

    case WM_MARKET_L2:
        if (state) Market_RefreshL2(hWnd, state);
        break;

    case WM_WATCHLIST_UPDATE: {
        auto* key = reinterpret_cast<std::string*>(lParam);
        if (!key) break;
        if (state) {
            auto dot = key->find('.');
            if (dot != std::string::npos) {
                int         updConId = std::stoi(key->substr(0, dot));
                std::string updSym   = key->substr(dot + 1);
                if (updConId == state->conId && updSym == state->symbol) {
                    TradingAPI::WatchlistInfo wi;
                    if (api.getWatchlistData(state->conId, state->symbol, wi)) {
                        if (wi.last      > 0.0) state->l1Info.last      = wi.last;
                        if (wi.open      > 0.0) state->l1Info.open      = wi.open;
                        if (wi.prevClose > 0.0) state->l1Info.prevClose = wi.prevClose;
                        if (wi.high      > 0.0) state->l1Info.high      = wi.high;
                        if (wi.low       > 0.0) state->l1Info.low       = wi.low;
                        if (wi.bid       > 0.0) state->l1Info.bid       = wi.bid;
                        if (wi.ask       > 0.0) state->l1Info.ask       = wi.ask;
                        if (wi.bidSize   > 0.0) state->l1Info.bidSize   = wi.bidSize;
                        if (wi.askSize   > 0.0) state->l1Info.askSize   = wi.askSize;
                        if (wi.volume    > 0)   state->l1Info.volume    = wi.volume;
                        if (wi.vwap      > 0.0) state->l1Info.vwap      = wi.vwap;

                        Market_RefreshPositionAndAvg(hWnd, state);
                    }
                    RECT hdrRc; GetClientRect(hWnd, &hdrRc); hdrRc.bottom = HEADER_H;
                    InvalidateRect(hWnd, &hdrRc, FALSE);
                }
            }
        }
        delete key;
        break;
    }

    case WM_COMMAND:
        if (LOWORD(wParam) == ID_TS_FILTER_CHECK && HIWORD(wParam) == BN_CLICKED && state) {
            state->tsFilteredView = (SendMessage(state->hTsFilterCheck, BM_GETCHECK, 0, 0) == BST_CHECKED);
            RECT rc; GetWindowRect(hWnd, &rc);
            if (state->tsFilteredView) {
                ListView_DeleteAllItems(state->hTsListF100);
                ListView_DeleteAllItems(state->hTsListF1000);
                MoveWindow(hWnd, rc.left, rc.top, windowMarketWidth + 181, windowMarketHeight, TRUE);
            } else {
                MoveWindow(hWnd, rc.left, rc.top, windowMarketWidth, windowMarketHeight, TRUE);
            }
            if (!state->symbol.empty()) {
                char filterKey[256];
                sprintf(filterKey, "TsFilterSize_%s", state->symbol.c_str());
                Settings_Save(filterKey, state->tsFilteredView ? 1 : 0);
            }
            Market_Layout(hWnd, state);
        }
        if (LOWORD(wParam) == ID_TS_SPEAKER && HIWORD(wParam) == STN_CLICKED && state)
            Market_ToggleTTS(hWnd, state);
        break;

    case WM_TIMER:
        if (wParam == TIMER_TS_SPEAKER && state && state->ttsOn)
            Market_SpeakLast(state);
        break;

    case WM_TTS_VOICE_CHANGED: {
        if (!state) break;
        if (state->hTtsVoice) {
            state->hTtsVoice->Speak(NULL, SVSFPurgeBeforeSpeak, NULL);
            state->hTtsVoice->Release();
            state->hTtsVoice = nullptr;
        }
        if (state->ttsOn) {
            KillTimer(hWnd, TIMER_TS_SPEAKER);
            if (Market_InitVoice(state)) {
                SetTimer(hWnd, TIMER_TS_SPEAKER, 21000, NULL);
                Market_SpeakLast(state);
            } else {
                state->ttsOn = false;
                if (state->hSpeakerBtn) {
                    SetCtrlColor(state->hSpeakerBtn, COINS_CLR_GRAY);
                    InvalidateRect(state->hSpeakerBtn, NULL, TRUE);
                }
            }
        }
        break;
    }

    case WM_MARKET_TICK: {
        auto* tick = reinterpret_cast<TradingAPI::TsTickEntry*>(lParam);
        if (state) {
            TimeSales_InsertTick(state->hTsList, tick->price, tick->size, tick->time);
            if (state->tsFilteredView) {
                if (tick->size >= 100.0)  TimeSales_InsertTick(state->hTsListF100,  tick->price, tick->size, tick->time);
                if (tick->size >= 1000.0) TimeSales_InsertTick(state->hTsListF1000, tick->price, tick->size, tick->time);
            }
            TradingAPI::WatchlistInfo wi;
            if (api.getWatchlistData(state->conId, state->symbol, wi)) {
                if (wi.last      > 0.0) state->l1Info.last      = wi.last;
                if (wi.open      > 0.0) state->l1Info.open      = wi.open;
                if (wi.prevClose > 0.0) state->l1Info.prevClose = wi.prevClose;
                if (wi.high      > 0.0) state->l1Info.high      = wi.high;
                if (wi.low       > 0.0) state->l1Info.low       = wi.low;
                if (wi.bid       > 0.0) state->l1Info.bid       = wi.bid;
                if (wi.ask       > 0.0) state->l1Info.ask       = wi.ask;
                if (wi.bidSize   > 0.0) state->l1Info.bidSize   = wi.bidSize;
                if (wi.askSize   > 0.0) state->l1Info.askSize   = wi.askSize;
                if (wi.volume    > 0)   state->l1Info.volume    = wi.volume;
                if (wi.vwap      > 0.0) state->l1Info.vwap      = wi.vwap;
            }
            {
                Market_RefreshPositionAndAvg(hWnd, state);
            }
            RECT hdrRc; GetClientRect(hWnd, &hdrRc); hdrRc.bottom = HEADER_H;
            InvalidateRect(hWnd, &hdrRc, FALSE);
        }
        delete tick;
        break;
    }

    case WM_NOTIFY: {
        NMHDR* hdr = (NMHDR*)lParam;
        if (hdr->code != NM_CUSTOMDRAW) break;

        if (hdr->idFrom == ID_TS_LIST || hdr->idFrom == ID_TS_LIST_F100 || hdr->idFrom == ID_TS_LIST_F1000) {
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

        if (hdr->idFrom == ID_TS_L2_LIST) {
            NMLVCUSTOMDRAW* cd = (NMLVCUSTOMDRAW*)lParam;
            switch (cd->nmcd.dwDrawStage) {
                case CDDS_PREPAINT:     return CDRF_NOTIFYITEMDRAW;
                case CDDS_ITEMPREPAINT: return CDRF_NOTIFYSUBITEMDRAW;
                case CDDS_ITEMPREPAINT | CDDS_SUBITEM: {
                    bool dark = Settings_DarkMode();
                    COLORREF rowBg = dark
                        ? (cd->nmcd.dwItemSpec % 2 == 0 ? DM_BG : DM_BG2)
                        : (cd->nmcd.dwItemSpec % 2 == 0 ? COINS_CLR_GRAY : COINS_CLR_WHITE);
                    cd->clrTextBk = rowBg;
                    cd->clrText   = (cd->iSubItem <= 1) ? COINS_CLR_BLUE : COINS_CLR_RED;
                    return CDRF_DODEFAULT;
                }
            }
            break;
        }
        break;
    }

    case WM_API_UPDATE: {
        if (state) {
            if (api.isMarketDataConnected() && api.isTradingConnected()) {
                api.setMarketWindow(hWnd, state->conId, state->symbol);
            } else {
                ListView_DeleteAllItems(state->hTsList);
                if (state->hTsListF100)  ListView_DeleteAllItems(state->hTsListF100);
                if (state->hTsListF1000) ListView_DeleteAllItems(state->hTsListF1000);
                if (state->hL2List)      ListView_DeleteAllItems(state->hL2List);
                state->l1Info = TradingAPI::Level1Info{};
                RECT hdrRc; GetClientRect(hWnd, &hdrRc); hdrRc.bottom = HEADER_H;
                InvalidateRect(hWnd, &hdrRc, FALSE);
            }
        }
        break;
    }

    case WM_SETCURSOR: {
        int id = GetDlgCtrlID((HWND)wParam);
        if (id == ID_TS_SPEAKER) {
            SetCursor(LoadCursor(NULL, IDC_HAND));
            return TRUE;
        }
        if (state && state->tsFilteredView && LOWORD(lParam) == HTCLIENT) {
            POINT pt; GetCursorPos(&pt); ScreenToClient(hWnd, &pt);
            int hit = HitTestSplitter(hWnd, state, pt.x, pt.y);
            if (hit == 1) { SetCursor(LoadCursor(NULL, IDC_SIZEWE)); return TRUE; }
            if (hit == 2) { SetCursor(LoadCursor(NULL, IDC_SIZENS)); return TRUE; }
        }
        break;
    }

    case WM_LBUTTONDOWN: {
        if (state) {
            int x = (short)LOWORD(lParam);
            int y = (short)HIWORD(lParam);
            if (state->tsFilteredView) {
                state->dragMode = HitTestSplitter(hWnd, state, x, y);
                if (state->dragMode != 0) SetCapture(hWnd);
            }
        }
        break;
    }

    case WM_MOUSEMOVE: {
        if (state && state->dragMode != 0) {
            RECT rc; GetClientRect(hWnd, &rc);
            const int bodyH = rc.bottom - HEADER_H;
            const int tsW   = rc.right - L2_W;
            int x = (short)LOWORD(lParam) - L2_W;
            int y = (short)HIWORD(lParam) - HEADER_H;

            if (state->dragMode == 1) {
                float ns = (tsW > 0) ? (float)x / (float)tsW : 0.5f;
                state->splitX = std::max(0.1f, std::min(0.9f, ns));
            } else if (state->dragMode == 2) {
                float ns = (bodyH > 0) ? (float)y / (float)bodyH : 0.5f;
                state->splitY = std::max(0.1f, std::min(0.9f, ns));
            }
            Market_Layout(hWnd, state);
            InvalidateRect(hWnd, NULL, TRUE);
        }
        break;
    }

    case WM_LBUTTONUP: {
        if (state && state->dragMode != 0) {
            state->dragMode = 0;
            ReleaseCapture();
            if (!state->symbol.empty())
                Settings_SaveMarketSplitter(state->symbol, state->splitX, state->splitY);
        }
        break;
    }
    
    case WM_DESTROY:
        api.unsetMarketWindow(hWnd);
        api.removeApiUpdateWindow(hWnd);
        MarketInitData* data = (MarketInitData*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
        if (data) delete data;
        if (state) {
            if (state->ttsOn) KillTimer(hWnd, TIMER_TS_SPEAKER);
            if (state->hTtsVoice) {
                state->hTtsVoice->Speak(NULL, SVSFPurgeBeforeSpeak, NULL);
                state->hTtsVoice->Release();
                state->hTtsVoice = nullptr;
            }
            if (state->hBigFont)     DeleteObject(state->hBigFont);
            if (state->hStatusFont)      DeleteObject(state->hStatusFont);
            if (state->hOrderFont)       DeleteObject(state->hOrderFont);
            if (state->hSmallFont)      DeleteObject(state->hSmallFont);
            if (state->hSpeakerFont) DeleteObject(state->hSpeakerFont);
            // Order bar controls are children and destroyed with the window,
            // but null the pointers so nothing uses them after destruction.
            state->hOrderLabel = state->hOrderPrice = state->hOrderQty = NULL;
            delete state;
            tsStates.erase(hWnd);
        }
        UpdateMarketRegistry();
        break;
    }
    return HandleCommonMessages(hWnd, message, wParam, lParam);
}
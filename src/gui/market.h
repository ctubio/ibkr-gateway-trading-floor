#pragma once

#include <map>
#include <string>
#include <vector>

// Payload passed during HWND creation
struct TsInitData { std::string symbol; int conId; };

void StartMarketSearch(); // Forward declaration
HWND StartMarket(const std::string& symbol = "", int conId = 0);

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
static const int HEADER_H = 90;   // Stats bar (28) + separator (1) + L1 quote (60) + separator (1)
static const int L2_W     = 200;  // Fixed width of the Level 2 depth panel

static ListViewZoomData MarketZoomData = { NULL, NULL, 14, "Zoom_Market" };

// State mapped per-window to support infinite instances safely
struct TsState {
    HWND hTsList = NULL;
    HWND hTsListF100 = NULL;
    HWND hTsListF1000 = NULL;
    HWND hTsFilterCheck = NULL;
    HWND hL2List = NULL;           // Level 2 depth list (left panel)
    bool tsFilteredView = false;
    std::string symbol;
    int conId = 0;

    // ── Level 1 quote (updated via WM_MARKET_L1) ─────────────────────────────
    TradingAPI::Level1Info l1Info;

    // ── Portfolio snapshot (position + avg price for the stats header) ────────
    double position = 0.0;
    double avgPrice = 0.0;

    // ── Cached fonts for the header panel (created in WM_CREATE) ─────────────
    HFONT hBigFont     = NULL;   // ~22pt bold — large last price
    HFONT hSmFont      = NULL;   // ~11pt regular — labels / change / bid-ask
    HFONT hStatFont    = NULL;   // ~13pt regular — stats bar (O/C/H/L/Pos/AvgPr)
    HFONT hSpeakerFont = NULL;   // Segoe MDL2 Assets — speaker glyph

    // ── TTS state ─────────────────────────────────────────────────────────────
    ISpVoice* hTtsVoice    = nullptr;
    bool      ttsOn        = false;
    bool      ttsComInit   = false;
    HWND      hSpeakerBtn  = NULL;  // speaker icon (SS_NOTIFY static)

    // --- Splitter State Variables ---
    float splitX = 0.5f; // Vertical split ratio (50% default) within the T&S area
    float splitY = 0.5f; // Horizontal split ratio (50% default)
    int dragMode = 0;    // 0 = none, 1 = dragging vertical, 2 = dragging horizontal
};
static std::map<HWND, TsState*> tsStates;

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
    { "Time",     90, LVCFMT_LEFT  },
    { "Exchange", 90, LVCFMT_LEFT  },
};
static const int TS_COL_COUNT = (int)(sizeof(tsCols) / sizeof(tsCols[0]));

// ── L2 column definitions ─────────────────────────────────────────────────────
// Layout: BidSz | Bid | Ask | AskSz
// Bid rows fill cols 0-1 (blue); ask rows fill cols 2-3 (red).
struct L2Col { const char* header; int width; int fmt; };
static const L2Col l2Cols[] = {
    { "B.Sz",  46, LVCFMT_RIGHT },
    { "Bid",   54, LVCFMT_RIGHT },
    { "Ask",   54, LVCFMT_RIGHT },
    { "A.Sz",  46, LVCFMT_RIGHT },
};
static const int L2_COL_COUNT = (int)(sizeof(l2Cols) / sizeof(l2Cols[0]));

static HWND Ts_CreateL2List(HWND hParent, HINSTANCE hInst) {
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
    }
    return hList;
}
static HWND Ts_CreateListView(HWND hParent, int id, HINSTANCE hInst) {
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
// Window structure (top → bottom):
//   HEADER_H px  : painted header (stats bar + L1 quote block)
//                  Filter checkbox is positioned top-right in the stats bar row
//   remaining    : L2 panel (left, L2_W wide) | T&S panel(s) (right)
static void Ts_Layout(HWND hWnd, TsState* state) {
    if (!state || !state->hTsList) return;
    RECT rc; GetClientRect(hWnd, &rc);

    const int hdrH     = HEADER_H;
    const int bodyY    = hdrH;
    const int bodyH    = rc.bottom - hdrH;
    const int bodyW    = rc.right;

    // ── Filter checkbox: top-right corner of header (stats row) ──────────────
    const int chkW = 16, chkH = 16;
    if (state->hTsFilterCheck)
        SetWindowPos(state->hTsFilterCheck, NULL,
            rc.right - chkW - 4, 6, chkW, chkH,
            SWP_NOZORDER | SWP_NOACTIVATE);

    // ── Speaker button: in the L1 quote area ─────────────────────────────────
    if (state->hSpeakerBtn) {
        // Positioned just to the right of the large last-price text area
        int spX = rc.right / 2 - 26;
        int spY = 23 + 4;
        SetWindowPos(state->hSpeakerBtn, NULL, spX, spY, 22, 22,
            SWP_NOZORDER | SWP_NOACTIVATE);
    }

    // ── Level 2 panel (always shown on left) ──────────────────────────────────
    if (state->hL2List)
        MoveWindow(state->hL2List, 0, bodyY, L2_W, bodyH, TRUE);

    // ── T&S area (right of L2 panel) ─────────────────────────────────────────
    const int tsX  = L2_W;
    const int tsW  = bodyW - L2_W;

    if (state->tsFilteredView) {
        const int splitThick = 6;
        int leftW = (int)(tsW * state->splitX) - splitThick / 2;
        int rightW = tsW - leftW - splitThick;
        int topH  = (int)(bodyH * state->splitY) - splitThick / 2;
        int botH  = bodyH - topH - splitThick;

        ShowWindow(state->hTsList,     SW_SHOW);
        ShowWindow(state->hTsListF100, SW_SHOW);
        ShowWindow(state->hTsListF1000,SW_SHOW);
        MoveWindow(state->hTsList,      tsX,               bodyY,           leftW,  bodyH, TRUE);
        MoveWindow(state->hTsListF100,  tsX + leftW + splitThick, bodyY,           rightW, topH,  TRUE);
        MoveWindow(state->hTsListF1000, tsX + leftW + splitThick, bodyY + topH + splitThick, rightW, botH, TRUE);
    } else {
        ShowWindow(state->hTsListF100,  SW_HIDE);
        ShowWindow(state->hTsListF1000, SW_HIDE);
        ShowWindow(state->hTsList,      SW_SHOW);
        MoveWindow(state->hTsList, tsX, bodyY, tsW, bodyH, TRUE);
    }
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
            if (sel == LB_ERR && SendMessage(hTsSearchList, LB_GETCOUNT, 0, 0) > 0) {
                sel = 0;
            }
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
    static bool registered = false;
    if (!registered) {
        WNDCLASS wc = { 0 };
        wc.lpfnWndProc = WndProcTsSearch;
        wc.hInstance = GetModuleHandle(NULL);
        wc.lpszClassName = MARKET_SEARCH_CLASS_NAME;
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        RegisterClass(&wc);
        registered = true;
    }
    HWND hWnd = CreateWindowExA(WS_EX_DLGMODALFRAME | WS_EX_TOPMOST, MARKET_SEARCH_CLASS_NAME, "Market: Search Symbol", 
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE, 
        (GetSystemMetrics(SM_CXSCREEN) - 268) / 2, (GetSystemMetrics(SM_CYSCREEN) - 255) / 2, 268, 255, 
        NULL, NULL, GetModuleHandle(NULL), NULL);
    ApplyDarkMode(hWnd);
}

HWND StartMarket(const std::string& symbol, int conId) {
    if (symbol.empty() || conId == 0) {
        StartMarketSearch();
        return NULL;
    }
    std::string key = MARKET_CLASS_NAME + std::string("_") + std::to_string(conId);
    TsInitData* data = new TsInitData{symbol, conId};
    return StartGenericWindow(MARKET_CLASS_NAME, ("Market: " + symbol).c_str(), L"IBKRGatewayClient.Market", 380, 500, NULL, key, data);
}

static int HitTestSplitter(HWND hWnd, TsState* state, int x, int y) {
    if (!state->tsFilteredView) return 0;

    RECT rc; GetClientRect(hWnd, &rc);
    const int bodyY  = HEADER_H;
    const int bodyH  = rc.bottom - HEADER_H;
    const int tsX    = L2_W;
    const int tsW    = rc.right - L2_W;
    const int splitThick = 6;

    // x is relative to the T&S sub-area
    int relX = x - tsX;
    int relY = y - bodyY;
    if (relX < 0 || relY < 0 || relY > bodyH) return 0;

    int splitXPos = (int)(tsW * state->splitX);
    int splitYPos = (int)(bodyH * state->splitY);

    if (relX >= splitXPos - splitThick && relX <= splitXPos + splitThick && relY >= 0 && relY <= bodyH)
        return 1; // vertical splitter
    if (relX > splitXPos + splitThick && relY >= splitYPos - splitThick && relY <= splitYPos + splitThick)
        return 2; // horizontal splitter

    return 0;
}

// ── Formatting helpers ────────────────────────────────────────────────────────
static std::string Ts_Fmt(double v, int dec = 2) {
    if (v == 0.0) return "--";
    char buf[32]; snprintf(buf, sizeof(buf), "%.*f", dec, v); return buf;
}
static std::string Ts_FmtSigned(double v, int dec = 2) {
    if (v == 0.0) return "--";
    char buf[32]; snprintf(buf, sizeof(buf), "%+.*f", dec, v); return buf;
}
static std::string Ts_FmtQty(double v) {
    if (v == 0.0) return "--";
    if (v == (long long)v) { char b[32]; snprintf(b,sizeof(b),"%lld",(long long)v); return b; }
    char b[32]; snprintf(b,sizeof(b),"%.2f",v); return b;
}

// ── L2 list refresh ───────────────────────────────────────────────────────────
// Rebuilds all rows from the latest book snapshot.
// Each row shows a paired bid+ask level side by side (merged table):
//   col0=B.Sz  col1=Bid  col2=Ask  col3=A.Sz
static void Ts_RefreshL2(HWND hWnd, TsState* state) {
    if (!state || !state->hL2List) return;

    std::vector<TradingAPI::Level2Entry> bids, asks;
    api.getLevel2Snapshot(hWnd, bids, asks);

    HWND hList = state->hL2List;
    SendMessage(hList, WM_SETREDRAW, FALSE, 0);
    ListView_DeleteAllItems(hList);

    int rows = (int)std::max(bids.size(), asks.size());
    for (int i = 0; i < rows; ++i) {
        // Insert empty row
        LVITEMA lvi = {}; lvi.mask = LVIF_TEXT | LVIF_PARAM;
        // lParam encodes row parity: 0=bid side exists, 1=ask side exists, 2=both
        int hasBid = (i < (int)bids.size()) ? 1 : 0;
        int hasAsk = (i < (int)asks.size()) ? 2 : 0;
        lvi.lParam  = (LPARAM)(hasBid | hasAsk);
        lvi.iItem   = i;
        std::string bidSzStr = hasBid ? Ts_FmtQty(bids[i].size) : "";
        lvi.pszText = (LPSTR)bidSzStr.c_str();
        ListView_InsertItem(hList, &lvi);

        std::string bidStr = hasBid ? Ts_Fmt(bids[i].price) : "";
        std::string askStr = hasAsk ? Ts_Fmt(asks[i].price) : "";
        std::string askSzStr = hasAsk ? Ts_FmtQty(asks[i].size) : "";
        ListView_SetItemText(hList, i, 1, (LPSTR)bidStr.c_str());
        ListView_SetItemText(hList, i, 2, (LPSTR)askStr.c_str());
        ListView_SetItemText(hList, i, 3, (LPSTR)askSzStr.c_str());
    }
    SendMessage(hList, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(hList, NULL, FALSE);
}

// ── Header paint ──────────────────────────────────────────────────────────────
// Paints the HEADER_H px band at the top of the market window.
//
// Row 1 (28px) — stats bar (larger font, colored values):
//   O: xxx  C: xxx  H: xxx  L: xxx  Pos: xxx  AvgPr: xxx
//   [checkbox top-right]
//
// Separator (1px)
//
// Row 2 (~60px) — L1 quote:
//   left:   large last price (bold ~22pt) + speaker icon to its right
//           change / changePct% below (red/green)
//   right:  Ask  price x size  (red)
//           Bid  price x size  (blue)
//
// Separator (1px)
static void Ts_PaintHeader(HWND hWnd, TsState* state) {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hWnd, &ps);

    RECT rc; GetClientRect(hWnd, &rc);
    const bool dark = Settings_DarkMode();
    const COLORREF bgColor    = dark ? DM_BG   : GetSysColor(COLOR_BTNFACE);
    const COLORREF textColor  = dark ? DM_TEXT : GetSysColor(COLOR_WINDOWTEXT);
    const COLORREF labelColor = dark ? RGB(160,160,160) : RGB(100,100,100);
    const COLORREF sepColor   = dark ? RGB(60,60,60) : RGB(200,200,200);
    const COLORREF redColor   = RGB(220, 70, 70);
    const COLORREF greenColor = RGB(80, 200, 120);
    const COLORREF blueColor  = RGB(80, 160, 255);

    // ── Fill background ───────────────────────────────────────────────────────
    RECT hdrRc = { 0, 0, rc.right, HEADER_H };
    HBRUSH hBgBrush = CreateSolidBrush(bgColor);
    FillRect(hdc, &hdrRc, hBgBrush);
    DeleteObject(hBgBrush);

    SetBkMode(hdc, TRANSPARENT);

    const TradingAPI::Level1Info& L1 = state->l1Info;

    // ── Row 1: stats bar (0..27) — draw label+value pairs with individual colors ──
    // Reserve right edge for checkbox (20px)
    const int statsRowH = 28;
    const int statsRight = rc.right - 22;  // leave room for checkbox

    HFONT hOldFont = (HFONT)SelectObject(hdc, state->hStatFont);
    SetTextColor(hdc, textColor);

    // We draw each label+value pair individually so values can be colored.
    // Layout: O C H L Pos AvgPr — evenly spaced across statsRight
    struct StatItem { const char* label; std::string value; COLORREF color; };

    double displayLast = (L1.last > 0.0) ? L1.last : L1.prevClose;

    // Color rules:
    // Open:    green if last > open, red if last < open, default otherwise
    // Close:   neutral (reference value)
    // High:    green (it's the high)
    // Low:     red (it's the low)
    // Pos:     green if positive, red if negative
    // AvgPr:   green if last > avgPrice, red if last < avgPrice
    COLORREF openColor   = textColor;
    if (displayLast > 0.0 && L1.open > 0.0) {
        openColor = (displayLast >= L1.open) ? greenColor : redColor;
    }
    COLORREF highColor   = (L1.high > 0.0) ? greenColor : textColor;
    COLORREF lowColor    = (L1.low  > 0.0) ? redColor   : textColor;
    COLORREF posColor    = (state->position > 0.0) ? greenColor
                         : (state->position < 0.0) ? redColor
                         : textColor;
    COLORREF avgPrColor  = textColor;
    if (displayLast > 0.0 && state->avgPrice > 0.0) {
        avgPrColor = (displayLast >= state->avgPrice) ? greenColor : redColor;
    }

    StatItem stats[] = {
        { "O:",    Ts_Fmt(L1.open),           openColor   },
        { "C:",    Ts_Fmt(L1.prevClose),       textColor   },
        { "H:",    Ts_Fmt(L1.high),            highColor   },
        { "L:",    Ts_Fmt(L1.low),             lowColor    },
        { "Pos:",  Ts_FmtQty(state->position), posColor    },
        { "Ap:",   Ts_Fmt(state->avgPrice),    avgPrColor  },
    };
    const int nStats = 6;

    // Measure each pair, then lay them out left-to-right with even spacing
    SIZE sz;
    int totalW = 0;
    int pairWidths[nStats] = {};
    char comboBuf[nStats][48];
    for (int i = 0; i < nStats; i++) {
        snprintf(comboBuf[i], sizeof(comboBuf[i]), "%s %s", stats[i].label, stats[i].value.c_str());
        GetTextExtentPoint32A(hdc, comboBuf[i], (int)strlen(comboBuf[i]), &sz);
        pairWidths[i] = sz.cx;
        totalW += sz.cx;
    }
    // Distribute gap evenly
    int gap = (nStats > 1) ? std::max(4, (statsRight - 8 - totalW) / (nStats - 1)) : 0;

    int cx = 8;
    for (int i = 0; i < nStats; i++) {
        // Draw label in labelColor
        char labBuf[16]; snprintf(labBuf, sizeof(labBuf), "%s ", stats[i].label);
        SIZE lblSz;
        GetTextExtentPoint32A(hdc, labBuf, (int)strlen(labBuf), &lblSz);
        SetTextColor(hdc, labelColor);
        RECT labRc = { cx, 4, cx + lblSz.cx, statsRowH - 2 };
        DrawTextA(hdc, labBuf, -1, &labRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

        // Draw value in its color
        SetTextColor(hdc, stats[i].color);
        RECT valRc = { cx + lblSz.cx, 4, cx + pairWidths[i] + 4, statsRowH - 2 };
        DrawTextA(hdc, stats[i].value.c_str(), -1, &valRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

        cx += pairWidths[i] + gap;
    }

    // ── Separator ─────────────────────────────────────────────────────────────
    HPEN hSepPen = CreatePen(PS_SOLID, 1, sepColor);
    HPEN hOldPen = (HPEN)SelectObject(hdc, hSepPen);
    MoveToEx(hdc, 0, statsRowH, NULL); LineTo(hdc, rc.right, statsRowH);
    SelectObject(hdc, hOldPen); DeleteObject(hSepPen);

    // ── Row 2: L1 quote block (statsRowH+1 .. HEADER_H-2) ────────────────────
    const int quoteY = statsRowH + 1;
    const int quoteH = HEADER_H - quoteY - 1;

    // Left half: large last price + speaker icon to its right + change below
    SelectObject(hdc, state->hBigFont);
    std::string lastStr = Ts_Fmt(displayLast);
    SetTextColor(hdc, textColor);
    // last price rect: from left up to midpoint minus a margin for speaker icon
    RECT lastRc = { 10, quoteY + 4, rc.right / 2 - 30, quoteY + quoteH / 2 + 4 };
    DrawTextA(hdc, lastStr.c_str(), -1, &lastRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

    // Change row
    SelectObject(hdc, state->hSmFont);
    double chg    = L1.change();
    double chgPct = L1.changePct();
    if (L1.last > 0.0 && (chg != 0.0 || L1.prevClose > 0.0)) {
        char buf[64]; snprintf(buf, sizeof(buf), "%.2f  %.2f%%", chg, chgPct);
        COLORREF chgColor = (chg >= 0.0) ? greenColor : redColor;
        SetTextColor(hdc, chgColor);
        RECT chgRc = { 10, quoteY + quoteH / 2 + 4, rc.right / 2 - 30, quoteY + quoteH };
        DrawTextA(hdc, buf, -1, &chgRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    }

    // Right half: Ask / Bid rows
    int rX = rc.right / 2 + 8;
    int rW = rc.right - rX - 8;
    int rowH = quoteH / 2;
    char buf[64];

    // Ask row (red)
    {
        SetTextColor(hdc, redColor);
        RECT lblRc = { rX, quoteY + 2, rX + 28, quoteY + rowH };
        DrawTextA(hdc, "Ask", -1, &lblRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

        SelectObject(hdc, state->hBigFont);
        std::string priceStr = Ts_Fmt(L1.ask);
        RECT pRc = { rX + 30, quoteY + 2, rX + 30 + 80, quoteY + rowH };
        DrawTextA(hdc, priceStr.c_str(), -1, &pRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

        SelectObject(hdc, state->hSmFont);
        snprintf(buf, sizeof(buf), "x %s", Ts_FmtQty(L1.askSize).c_str());
        RECT szRc = { rX + 115, quoteY + 2, rX + rW, quoteY + rowH };
        DrawTextA(hdc, buf, -1, &szRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    }

    // Bid row (blue)
    {
        SetTextColor(hdc, blueColor);
        RECT lblRc = { rX, quoteY + rowH + 2, rX + 28, quoteY + quoteH };
        DrawTextA(hdc, "Bid", -1, &lblRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

        SelectObject(hdc, state->hBigFont);
        std::string priceStr = Ts_Fmt(L1.bid);
        RECT pRc = { rX + 30, quoteY + rowH + 2, rX + 30 + 80, quoteY + quoteH };
        DrawTextA(hdc, priceStr.c_str(), -1, &pRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

        SelectObject(hdc, state->hSmFont);
        snprintf(buf, sizeof(buf), "x %s", Ts_FmtQty(L1.bidSize).c_str());
        RECT szRc = { rX + 115, quoteY + rowH + 2, rX + rW, quoteY + quoteH };
        DrawTextA(hdc, buf, -1, &szRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    }

    // ── Bottom separator ──────────────────────────────────────────────────────
    SelectObject(hdc, hOldFont);
    hSepPen = CreatePen(PS_SOLID, 1, sepColor);
    hOldPen = (HPEN)SelectObject(hdc, hSepPen);
    MoveToEx(hdc, 0, HEADER_H - 1, NULL); LineTo(hdc, rc.right, HEADER_H - 1);
    SelectObject(hdc, hOldPen); DeleteObject(hSepPen);

    EndPaint(hWnd, &ps);
}

// ── Market TTS helpers ────────────────────────────────────────────────────────
static const wchar_t TS_SPEAKER_GLYPH[] = L"\uE767";   // Segoe MDL2 Assets volume-on

static bool Ts_InitVoice(TsState* state) {
    if (state->hTtsVoice) return true;
    if (!state->ttsComInit) {
        HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
        state->ttsComInit = SUCCEEDED(hr) || (hr == RPC_E_CHANGED_MODE);
    }
    HRESULT hr = CoCreateInstance(CLSID_SpVoice, NULL, CLSCTX_ALL, IID_ISpVoice, (void**)&state->hTtsVoice);
    if (FAILED(hr)) { state->hTtsVoice = nullptr; return false; }
    return true;
}

static void Ts_SpeakLast(TsState* state) {
    if (!state->hTtsVoice) return;
    double displayLast = (state->l1Info.last > 0.0) ? state->l1Info.last : state->l1Info.prevClose;
    if (displayLast <= 0.0) return;
    char buf[64]; snprintf(buf, sizeof(buf), "%.2f", displayLast);
    // Strip trailing zeros: "123.50" → "123.5", "123.00" → "123"
    std::string s(buf);
    if (s.find('.') != std::string::npos) {
        while (s.size() > 1 && s.back() == '0') s.pop_back();
        if (s.back() == '.') s.pop_back();
    }
    std::wstring ws(s.begin(), s.end());
    state->hTtsVoice->Speak(ws.c_str(), SVSFlagsAsync | SVSFPurgeBeforeSpeak, NULL);
}

static void Ts_ToggleTTS(HWND hWnd, TsState* state) {
    state->ttsOn = !state->ttsOn;
    if (state->ttsOn) {
        if (!Ts_InitVoice(state)) { state->ttsOn = false; return; }
        // Color speaker bright
        if (state->hSpeakerBtn)
            SetCtrlColor(state->hSpeakerBtn,
                Settings_DarkMode() ? RGB(220,220,220) : RGB(30,30,30));
        SetTimer(hWnd, TIMER_TS_SPEAKER, 21000, NULL);
        Ts_SpeakLast(state);   // speak immediately
    } else {
        KillTimer(hWnd, TIMER_TS_SPEAKER);
        if (state->hTtsVoice)
            state->hTtsVoice->Speak(NULL, SVSFPurgeBeforeSpeak, NULL);
        if (state->hSpeakerBtn)
            SetCtrlColor(state->hSpeakerBtn, RGB(120,120,120));
    }
    if (state->hSpeakerBtn) InvalidateRect(state->hSpeakerBtn, NULL, TRUE);
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
        TsInitData* data = (TsInitData*)(((LPCREATESTRUCT)lParam)->lpCreateParams);
        state = new TsState();
        if (data) {
            state->symbol = data->symbol;
            state->conId  = data->conId;
            delete data;
        }
        tsStates[hWnd] = state;

        // ── Header fonts ──────────────────────────────────────────────────────
        state->hBigFont = CreateFontA(-22, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
        state->hSmFont  = CreateFontA(-11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
        // Stats bar font — slightly larger (fits single row within HEADER_H)
        state->hStatFont = CreateFontA(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
        // Speaker icon font (Segoe MDL2 Assets)
        state->hSpeakerFont = CreateFontW(-13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe MDL2 Assets");

        // ── Snapshot position + avg price from portfolio ───────────────────────
        if (!state->symbol.empty()) {
            std::lock_guard<std::mutex> lk(api.getPortfolioMutex());
            auto& pm = api.getPortfolioMap();
            auto it = pm.find(state->symbol);
            if (it != pm.end()) {
                state->position = it->second.shares;
                state->avgPrice = it->second.avgCost;
            }
        }

        // ── Lists (font size 14) ──────────────────────────────────────────────
        state->hL2List      = Ts_CreateL2List(hWnd, hInst);
        state->hTsList      = Ts_CreateListView(hWnd, ID_TS_LIST,       hInst);
        state->hTsListF100  = Ts_CreateListView(hWnd, ID_TS_LIST_F100,  hInst);
        state->hTsListF1000 = Ts_CreateListView(hWnd, ID_TS_LIST_F1000, hInst);

        // Apply font-14 to all three T&S lists and the L2 list
        {
            HFONT hListFont = CreateFontA(-14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
            SendMessage(state->hTsList,      WM_SETFONT, (WPARAM)hListFont, TRUE);
            SendMessage(state->hTsListF100,  WM_SETFONT, (WPARAM)hListFont, TRUE);
            SendMessage(state->hTsListF1000, WM_SETFONT, (WPARAM)hListFont, TRUE);
            SendMessage(state->hL2List,      WM_SETFONT, (WPARAM)hListFont, TRUE);
            // Note: we intentionally do not store hListFont in TsState for now
            // (it's managed by the OS when the list is destroyed).  If you need
            // to delete it manually, add an hListFont member to TsState.
        }

        ShowWindow(state->hTsList,  SW_SHOW);
        ShowWindow(state->hL2List,  SW_SHOW);

        // ── Filter checkbox (no label — tooltip explains) ─────────────────────
        state->hTsFilterCheck = CreateWindowA("BUTTON", "",
            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
            0, 0, 16, 16, hWnd, (HMENU)ID_TS_FILTER_CHECK, hInst, NULL);

        // Tooltip for filter checkbox
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

        // ── Speaker button (MDL2 Assets glyph, SS_NOTIFY so clicks fire WM_COMMAND) ──
        state->hSpeakerBtn = CreateWindowW(L"STATIC", TS_SPEAKER_GLYPH,
            WS_CHILD | WS_VISIBLE | SS_CENTER | SS_NOTIFY,
            0, 0, 22, 22, hWnd, (HMENU)ID_TS_SPEAKER, hInst, NULL);
        SendMessage(state->hSpeakerBtn, WM_SETFONT, (WPARAM)state->hSpeakerFont, TRUE);
        SetCtrlColor(state->hSpeakerBtn, RGB(120, 120, 120));  // dim = inactive

        // Restore splitter positions and filter state
        if (!state->symbol.empty()) {
            Settings_LoadMarketSplitter(state->symbol, state->splitX, state->splitY);
            char filterKey[256];
            sprintf(filterKey, "TsFilterSize_%s", state->symbol.c_str());
            if (Settings_Load(filterKey, 0)) {
                state->tsFilteredView = true;
                SendMessage(state->hTsFilterCheck, BM_SETCHECK, BST_CHECKED, 0);
            }
        }

        api.setMarketWindow(hWnd, state->conId, state->symbol);
        api.addApiUpdateWindow(hWnd);
        UpdateMarketRegistry();
        break;
    }

    case WM_SIZE:
        Ts_Layout(hWnd, state);
        InvalidateRect(hWnd, NULL, FALSE);   // repaint header on resize
        return 0;

    case WM_PAINT:
        if (state) Ts_PaintHeader(hWnd, state);
        else { PAINTSTRUCT ps; BeginPaint(hWnd, &ps); EndPaint(hWnd, &ps); }
        return 0;

    case WM_MARKET_L1: {
        if (!state) break;
        // Primary: pull from the dedicated reqMktData subscription
        TradingAPI::Level1Info fresh;
        if (api.getLevel1Data(hWnd, fresh)) {
            // Merge: keep last from the dedicated feed only if watchlist hasn't
            // already supplied it via WM_MARKET_TICK (non-zero wins)
            if (fresh.last     > 0.0) state->l1Info.last     = fresh.last;
            if (fresh.open     > 0.0) state->l1Info.open     = fresh.open;
            if (fresh.prevClose> 0.0) state->l1Info.prevClose= fresh.prevClose;
            if (fresh.high     > 0.0) state->l1Info.high     = fresh.high;
            if (fresh.low      > 0.0) state->l1Info.low      = fresh.low;
            if (fresh.bid      > 0.0) state->l1Info.bid      = fresh.bid;
            if (fresh.ask      > 0.0) state->l1Info.ask      = fresh.ask;
            if (fresh.bidSize  > 0.0) state->l1Info.bidSize  = fresh.bidSize;
            if (fresh.askSize  > 0.0) state->l1Info.askSize  = fresh.askSize;
        }
        // Also try watchlist as a secondary source (covers open/close for
        // symbols that trade infrequently so WM_MARKET_TICK fires rarely)
        TradingAPI::WatchlistInfo wi;
        if (api.getWatchlistData(state->conId, state->symbol, wi)) {
            if (wi.open      > 0.0 && state->l1Info.open      == 0.0) state->l1Info.open      = wi.open;
            if (wi.prevClose > 0.0 && state->l1Info.prevClose  == 0.0) state->l1Info.prevClose = wi.prevClose;
            if (wi.high      > 0.0 && state->l1Info.high       == 0.0) state->l1Info.high      = wi.high;
            if (wi.low       > 0.0 && state->l1Info.low        == 0.0) state->l1Info.low       = wi.low;
        }
        // Refresh position/avgPrice
        {
            std::lock_guard<std::mutex> lk(api.getPortfolioMutex());
            auto& pm = api.getPortfolioMap();
            auto it = pm.find(state->symbol);
            if (it != pm.end()) {
                state->position = it->second.shares;
                state->avgPrice = it->second.avgCost;
            }
        }
        RECT hdrRc; GetClientRect(hWnd, &hdrRc); hdrRc.bottom = HEADER_H;
        InvalidateRect(hWnd, &hdrRc, FALSE);
        break;
    }

    case WM_MARKET_L2:
        if (state) Ts_RefreshL2(hWnd, state);
        break;

    // ── Watchlist / historical-data tick ─────────────────────────────────────
    // Fires for every streaming tick AND for the historical-data fallback that
    // the gateway uses to populate prevClose/last when the market is closed.
    // This is the only path that brings L1 data when there are no T&S prints.
    case WM_WATCHLIST_UPDATE: {
        auto* key = reinterpret_cast<std::string*>(lParam);
        if (!key) break;
        if (state) {
            auto dot = key->find('.');
            if (dot != std::string::npos) {
                int    updConId = std::stoi(key->substr(0, dot));
                std::string updSym = key->substr(dot + 1);
                if (updConId == state->conId && updSym == state->symbol) {
                    TradingAPI::WatchlistInfo wi;
                    if (api.getWatchlistData(state->conId, state->symbol, wi)) {
                        // Populate all L1 fields; last from watchlist includes
                        // the gateway's historical-data fallback when last == 0.
                        if (wi.last      > 0.0) state->l1Info.last      = wi.last;
                        if (wi.open      > 0.0) state->l1Info.open      = wi.open;
                        if (wi.prevClose > 0.0) state->l1Info.prevClose = wi.prevClose;
                        if (wi.high      > 0.0) state->l1Info.high      = wi.high;
                        if (wi.low       > 0.0) state->l1Info.low       = wi.low;
                        if (wi.bid       > 0.0) state->l1Info.bid       = wi.bid;
                        if (wi.ask       > 0.0) state->l1Info.ask       = wi.ask;
                        if (wi.bidSize   > 0.0) state->l1Info.bidSize   = wi.bidSize;
                        if (wi.askSize   > 0.0) state->l1Info.askSize   = wi.askSize;

                        std::lock_guard<std::mutex> lk(api.getPortfolioMutex());
                        auto& pm = api.getPortfolioMap();
                        auto pit = pm.find(state->symbol);
                        if (pit != pm.end()) {
                            state->position = pit->second.shares;
                            state->avgPrice = pit->second.avgCost;
                        }
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
            if (state->tsFilteredView) {
                ListView_DeleteAllItems(state->hTsListF100); ListView_DeleteAllItems(state->hTsListF1000);
            }
            if (!state->symbol.empty()) {
                char filterKey[256];
                sprintf(filterKey, "TsFilterSize_%s", state->symbol.c_str());
                Settings_Save(filterKey, state->tsFilteredView ? 1 : 0);
            }
            Ts_Layout(hWnd, state);
        }
        // Speaker icon or last-price area toggles TTS
        if ((LOWORD(wParam) == ID_TS_SPEAKER) && HIWORD(wParam) == STN_CLICKED && state)
            Ts_ToggleTTS(hWnd, state);
        break;

    case WM_TIMER:
        if (wParam == TIMER_TS_SPEAKER && state && state->ttsOn)
            Ts_SpeakLast(state);
        break;

    case WM_MARKET_TICK: {
        auto* tick = reinterpret_cast<TradingAPI::TsTickEntry*>(lParam);
        if (state) {
            Ts_InsertTick(state->hTsList, tick->price, tick->size, tick->time, tick->exchange);
            if (state->tsFilteredView) {
                if (tick->size >= 100.0)  Ts_InsertTick(state->hTsListF100,  tick->price, tick->size, tick->time, tick->exchange);
                if (tick->size >= 1000.0) Ts_InsertTick(state->hTsListF1000, tick->price, tick->size, tick->time, tick->exchange);
            }

            // ── Update L1 header on every T&S tick ───────────────────────────
            // last: use the actual print price (always accurate, no subscription needed)
            state->l1Info.last = tick->price;

            // open/close/high/low/bid/ask: pull from the watchlist subscription
            // which is already streaming for this symbol — no separate reqMktData needed.
            TradingAPI::WatchlistInfo wi;
            if (api.getWatchlistData(state->conId, state->symbol, wi)) {
                state->l1Info.open      = wi.open;
                state->l1Info.prevClose = wi.prevClose;
                state->l1Info.high      = wi.high;
                state->l1Info.low       = wi.low;
                state->l1Info.bid       = wi.bid;
                state->l1Info.ask       = wi.ask;
                state->l1Info.bidSize   = wi.bidSize;
                state->l1Info.askSize   = wi.askSize;
            }

            // Refresh position / avg price
            {
                std::lock_guard<std::mutex> lk(api.getPortfolioMutex());
                auto& pm = api.getPortfolioMap();
                auto pit = pm.find(state->symbol);
                if (pit != pm.end()) {
                    state->position = pit->second.shares;
                    state->avgPrice = pit->second.avgCost;
                }
            }

            // Invalidate only the header band (leave lists undisturbed)
            RECT hdrRc; GetClientRect(hWnd, &hdrRc); hdrRc.bottom = HEADER_H;
            InvalidateRect(hWnd, &hdrRc, FALSE);
        }
        delete tick;
        break;
    }

    case WM_NOTIFY: {
        NMHDR* hdr = (NMHDR*)lParam;
        if (hdr->code != NM_CUSTOMDRAW) break;

        // ── T&S lists ─────────────────────────────────────────────────────────
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

        // ── L2 depth list ─────────────────────────────────────────────────────
        if (hdr->idFrom == ID_TS_L2_LIST) {
            NMLVCUSTOMDRAW* cd = (NMLVCUSTOMDRAW*)lParam;
            switch (cd->nmcd.dwDrawStage) {
                case CDDS_PREPAINT:     return CDRF_NOTIFYITEMDRAW;
                case CDDS_ITEMPREPAINT: return CDRF_NOTIFYSUBITEMDRAW;
                case CDDS_ITEMPREPAINT | CDDS_SUBITEM: {
                    bool dark = Settings_DarkMode();
                    COLORREF rowBg = dark
                        ? (cd->nmcd.dwItemSpec % 2 == 0 ? DM_BG : DM_BG2)
                        : (cd->nmcd.dwItemSpec % 2 == 0 ? RGB(245,245,245) : RGB(255,255,255));
                    int col = cd->iSubItem;
                    // Bid side: cols 0-1 → blue tint text
                    // Ask side: cols 2-3 → red tint text
                    if (col <= 1) {
                        cd->clrText   = RGB(80, 160, 255);
                        cd->clrTextBk = rowBg;
                    } else {
                        cd->clrText   = RGB(220, 70, 70);
                        cd->clrTextBk = rowBg;
                    }
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
                state->l1Info = TradingAPI::Level1Info{};   // clear stale quote
                RECT hdrRc = { 0, 0, 0, 0 };
                GetClientRect(hWnd, &hdrRc); hdrRc.bottom = HEADER_H;
                InvalidateRect(hWnd, &hdrRc, FALSE);
            }
        }
        break;
    }
    
    case WM_SETCURSOR: {
        if (state && state->tsFilteredView && LOWORD(lParam) == HTCLIENT) {
            POINT pt; 
            GetCursorPos(&pt); 
            ScreenToClient(hWnd, &pt);
            
            int hit = HitTestSplitter(hWnd, state, pt.x, pt.y);
            if (hit == 1) {
                SetCursor(LoadCursor(NULL, IDC_SIZEWE)); // Left/Right resize cursor
                return TRUE;
            } else if (hit == 2) {
                SetCursor(LoadCursor(NULL, IDC_SIZENS)); // Up/Down resize cursor
                return TRUE;
            }
        }
        break; 
    }

    case WM_LBUTTONDOWN: {
        if (state) {
            int x = (short)LOWORD(lParam);
            int y = (short)HIWORD(lParam);

            // Click on last-price area in header toggles TTS
            // (speaker icon also handled via WM_COMMAND/STN_CLICKED)
            const int quoteY = 29;  // statsRowH + 1
            const int quoteH = HEADER_H - quoteY - 1;
            RECT rc2; GetClientRect(hWnd, &rc2);
            RECT lastPriceRc = { 10, quoteY, rc2.right / 2 - 30, quoteY + quoteH };
            if (PtInRect(&lastPriceRc, { x, y }))
                Ts_ToggleTTS(hWnd, state);

            if (state->tsFilteredView) {
                state->dragMode = HitTestSplitter(hWnd, state, x, y);
                if (state->dragMode != 0) {
                    SetCapture(hWnd);
                }
            }
        }
        break;
    }

    case WM_MOUSEMOVE: {
        if (state && state->dragMode != 0) {
            RECT rc; GetClientRect(hWnd, &rc);
            const int bodyH = rc.bottom - HEADER_H;
            const int tsW   = rc.right - L2_W;
            int x = (short)LOWORD(lParam) - L2_W;  // relative to T&S area
            int y = (short)HIWORD(lParam) - HEADER_H;

            if (state->dragMode == 1) {
                float newSplit = (tsW > 0) ? (float)x / (float)tsW : 0.5f;
                if (newSplit < 0.1f) newSplit = 0.1f;
                if (newSplit > 0.9f) newSplit = 0.9f;
                state->splitX = newSplit;
            } else if (state->dragMode == 2) {
                float newSplit = (bodyH > 0) ? (float)y / (float)bodyH : 0.5f;
                if (newSplit < 0.1f) newSplit = 0.1f;
                if (newSplit > 0.9f) newSplit = 0.9f;
                state->splitY = newSplit;
            }
            Ts_Layout(hWnd, state);
            InvalidateRect(hWnd, NULL, TRUE);
        }
        break;
    }

    case WM_LBUTTONUP: {
        if (state && state->dragMode != 0) {
            state->dragMode = 0; // Stop dragging
            ReleaseCapture();    // Release the mouse lock

            // Persist the new splitter positions for this symbol
            if (!state->symbol.empty()) {
                Settings_SaveMarketSplitter(state->symbol, state->splitX, state->splitY);
            }
        }
        break;
    }
    case WM_DESTROY:
        api.unsetMarketWindow(hWnd);
        api.removeApiUpdateWindow(hWnd);
        if (state) {
            // Stop TTS
            if (state->ttsOn) KillTimer(hWnd, TIMER_TS_SPEAKER);
            if (state->hTtsVoice) {
                state->hTtsVoice->Speak(NULL, SVSFPurgeBeforeSpeak, NULL);
                state->hTtsVoice->Release();
                state->hTtsVoice = nullptr;
            }
            if (state->hBigFont)     DeleteObject(state->hBigFont);
            if (state->hSmFont)      DeleteObject(state->hSmFont);
            if (state->hStatFont)    DeleteObject(state->hStatFont);
            if (state->hSpeakerFont) DeleteObject(state->hSpeakerFont);
            delete state;
            tsStates.erase(hWnd);
        }
        UpdateMarketRegistry();
        break;
    }
    return HandleCommonMessages(hWnd, message, wParam, lParam);
}
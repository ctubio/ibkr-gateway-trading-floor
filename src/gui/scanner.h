#pragma once

void StartScanner() { StartGenericWindow(SCANNER_CLASS_NAME, "Scanner", L"TWSAPIClientTradingFloor.Scanner", 400, 600); }

#define ID_SCANNER_RESULTS_LIST         3001
#define ID_SCANNER_BTN_NYSE             3002
#define ID_SCANNER_BTN_NASDAQ_NATIONAL  3003
#define ID_SCANNER_BTN_NASDAQ_SCM       3004

enum ScannerColIdx { SCOL_INSTRUMENT = 0, SCOL_CHGPCT, SCOL_LAST, SCOL_COUNT };

static HWND hScannerResults   = NULL;
static HWND hScannerBtnNYSE    = NULL;
static HWND hScannerBtnNASDAQNational  = NULL;
static HWND hScannerBtnNASDAQSCM  = NULL;
static int  g_ScannerActiveIndex = 0; // 0 = NYSE, 1 = NASDAQ National, 2 = NASDAQ SCM

// conId -> symbol for the rows currently shown; also the map handed to
// api().setWatchlistWindow() so Change%/Last Price arrive via WM_MARKET_L1,
// reusing the existing watchlist L1 subscription plumbing unchanged.
static std::unordered_map<int, std::string> g_ScannerEntries;

static const int SCANNER_BTN_H       = 24;
static const int SCANNER_SELECTOR_H  = 8 + SCANNER_BTN_H + 8;
static const char* SCANNER_NO_DATA   = "--";

static ListViewZoomData ScannerZoomData = { NULL, NULL, 14, "Zoom_Scanner" };

// ── Helpers ───────────────────────────────────────────────────────────────────

static int Scanner_FindRow(int conId) {
    if (!hScannerResults) return -1;
    int count = ListView_GetItemCount(hScannerResults);
    for (int i = 0; i < count; ++i) {
        LVITEMA lvi = {};
        lvi.mask  = LVIF_PARAM;
        lvi.iItem = i;
        SendMessageA(hScannerResults, LVM_GETITEMA, 0, (LPARAM)&lvi);
        if ((int)lvi.lParam == conId) return i;
    }
    return -1;
}

static void Scanner_ClearList(HWND hWnd) {
    if (!hScannerResults) return;
    
    SendMessage(hScannerResults, WM_SETREDRAW, FALSE, 0);

    ListView_DeleteAllItems(hScannerResults);
    
    SendMessage(hScannerResults, WM_SETREDRAW, TRUE, 0);
    RedrawWindow(hWnd, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
}

// ── Layout ────────────────────────────────────────────────────────────────────

static void Scanner_Layout(HWND hWnd) {
    if (!hScannerResults) return;
    RECT rc; GetClientRect(hWnd, &rc);
    const int m = 8;

    bool btnsVisible = hScannerBtnNASDAQSCM && IsWindowVisible(hScannerBtnNASDAQSCM);

    if (btnsVisible) {
        
        int btnW = 130;
        int btnY = rc.bottom - SCANNER_SELECTOR_H + (SCANNER_SELECTOR_H - SCANNER_BTN_H) / 2;
        int totalW = (btnW * 3) + m - 60;
        int startX = (rc.right - totalW) / 2;
        MoveWindow(hScannerResults, 0, 0, rc.right, rc.bottom - SCANNER_SELECTOR_H, TRUE);
        SetWindowPos(hScannerBtnNYSE,           NULL, startX,                          btnY, btnW, SCANNER_BTN_H, SWP_NOZORDER | SWP_NOACTIVATE);
        SetWindowPos(hScannerBtnNASDAQNational, NULL, startX + btnW + m,               btnY, btnW, SCANNER_BTN_H, SWP_NOZORDER | SWP_NOACTIVATE);
        SetWindowPos(hScannerBtnNASDAQSCM,      NULL, startX + btnW + m + btnW + m,    btnY, btnW, SCANNER_BTN_H, SWP_NOZORDER | SWP_NOACTIVATE);
    } else {
        MoveWindow(hScannerResults, 0, 0, rc.right, rc.bottom, TRUE);
    }
}

// ── Subscribe / switch scanner ────────────────────────────────────────────────

static void Scanner_Subscribe(HWND hWnd, int index) {
    g_ScannerActiveIndex = index;
    Settings_Scanner_Save("LastIndex", (DWORD)index);

    if (hScannerBtnNYSE)           SendMessage(hScannerBtnNYSE,           BM_SETCHECK, index == 0 ? BST_CHECKED : BST_UNCHECKED, 0);
    if (hScannerBtnNASDAQNational) SendMessage(hScannerBtnNASDAQNational, BM_SETCHECK, index == 1 ? BST_CHECKED : BST_UNCHECKED, 0);
    if (hScannerBtnNASDAQSCM)      SendMessage(hScannerBtnNASDAQSCM,      BM_SETCHECK, index == 2 ? BST_CHECKED : BST_UNCHECKED, 0);

    Scanner_ClearList(hWnd);
    g_ScannerEntries.clear();
    api().unsetWatchlistWindow(hWnd); // drop L1 subscriptions for the previous rows

    SetWindowTextA(hWnd, index == 0 ? "Scanner: New York Stock Exchange" : (index == 1 ? "Scanner: NASDAQ National Market" : "Scanner: NASDAQ Small/Mid Caps"));
    api().reqScanner(index);
}

// ── Window procedure ──────────────────────────────────────────────────────────

LRESULT CALLBACK WndProcScanner(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {

    case WM_CREATE: {
        HINSTANCE hInst = ((LPCREATESTRUCT)lParam)->hInstance;

        hScannerResults = CreateWindowExA(
            WS_EX_CLIENTEDGE, "SysListView32", "",
            WS_CHILD | WS_VISIBLE | WS_BORDER |
            LVS_REPORT | LVS_SHOWSELALWAYS | LVS_NOSORTHEADER,
            0, 0, 400, 400,
            hWnd, (HMENU)ID_SCANNER_RESULTS_LIST, hInst, NULL);

        ScannerZoomData.fontSize = (int)Settings_Load(ScannerZoomData.settingKey, ScannerZoomData.fontSize);
        ApplyListViewFont(hScannerResults, ScannerZoomData.hFont, ScannerZoomData.hBoldFont, ScannerZoomData.fontSize);
        SetWindowSubclass(hScannerResults, ListViewZoomProc, 0, (DWORD_PTR)&ScannerZoomData);
        ListView_SetExtendedListViewStyle(hScannerResults, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

        LVCOLUMNA lvc = {};
        lvc.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_FMT;

        lvc.fmt = LVCFMT_LEFT;  lvc.cx = 180; lvc.pszText = (LPSTR)"Instrument";  ListView_InsertColumn(hScannerResults, SCOL_INSTRUMENT, &lvc);
        lvc.fmt = LVCFMT_RIGHT; lvc.cx = 100; lvc.pszText = (LPSTR)"Change %";    ListView_InsertColumn(hScannerResults, SCOL_CHGPCT,     &lvc);
        lvc.fmt = LVCFMT_RIGHT; lvc.cx = 100; lvc.pszText = (LPSTR)"Last Price";  ListView_InsertColumn(hScannerResults, SCOL_LAST,       &lvc);

        // Bottom selector buttons — hidden until the window is activated.
        hScannerBtnNYSE = CreateWindowA("BUTTON", "NYSE",
            WS_CHILD | BS_AUTORADIOBUTTON,
            0, 0, 10, 10, hWnd, (HMENU)ID_SCANNER_BTN_NYSE, hInst, NULL);
        hScannerBtnNASDAQNational = CreateWindowA("BUTTON", "NASDAQ",
            WS_CHILD | BS_AUTORADIOBUTTON,
            0, 0, 10, 10, hWnd, (HMENU)ID_SCANNER_BTN_NASDAQ_NATIONAL, hInst, NULL);
        hScannerBtnNASDAQSCM = CreateWindowA("BUTTON", "Small Caps",
            WS_CHILD | BS_AUTORADIOBUTTON,
            0, 0, 10, 10, hWnd, (HMENU)ID_SCANNER_BTN_NASDAQ_SCM, hInst, NULL);

        int savedIndex = (int)Settings_Scanner_Load("LastIndex", 0);
        g_ScannerActiveIndex = (savedIndex == 1) ? 1 : 0;

        api().addApiUpdateWindow(hWnd);
        Scanner_Subscribe(hWnd, g_ScannerActiveIndex);
        break;
    }

    case WM_SIZE:
        Scanner_Layout(hWnd);
        break;

    case WM_ACTIVATE:
        if (LOWORD(wParam) != WA_INACTIVE) {
            ShowWindow(hScannerBtnNYSE,           SW_SHOW);
            ShowWindow(hScannerBtnNASDAQNational, SW_SHOW);
            ShowWindow(hScannerBtnNASDAQSCM,      SW_SHOW);
        } else {
            ShowWindow(hScannerBtnNYSE,           SW_HIDE);
            ShowWindow(hScannerBtnNASDAQNational, SW_HIDE);
            ShowWindow(hScannerBtnNASDAQSCM,      SW_HIDE);
        }
        Scanner_Layout(hWnd);
        return 0;

    case WM_COMMAND:
        if ((LOWORD(wParam) == ID_SCANNER_BTN_NYSE || LOWORD(wParam) == ID_SCANNER_BTN_NASDAQ_NATIONAL || LOWORD(wParam) == ID_SCANNER_BTN_NASDAQ_SCM) && HIWORD(wParam) == BN_CLICKED) {
            int newIndex = 0;
            if (LOWORD(wParam) == ID_SCANNER_BTN_NYSE) {
                newIndex = 0;
            } else if (LOWORD(wParam) == ID_SCANNER_BTN_NASDAQ_NATIONAL) {
                newIndex = 1;
            } else if (LOWORD(wParam) == ID_SCANNER_BTN_NASDAQ_SCM){
                newIndex = 2;
            }
            if (newIndex != g_ScannerActiveIndex) Scanner_Subscribe(hWnd, newIndex);
        }
        break;

    // ── Scanner subscription refreshed (rank/contract only, no price yet) ────
    case WM_SCANNER_DATA: {
        auto rows = api().getScannerResults();

        Scanner_ClearList(hWnd);
        g_ScannerEntries.clear();

        for (const auto& row : rows) {
            if (row.conId <= 0) continue;
            std::string label = row.symbol;
            if (!row.exchange.empty()) label += "." + row.exchange;

            LVITEMA lvi = {};
            lvi.mask     = LVIF_TEXT | LVIF_PARAM;
            lvi.iItem    = ListView_GetItemCount(hScannerResults);
            lvi.lParam   = (LPARAM)row.conId;
            lvi.pszText  = (LPSTR)label.c_str();
            int idx = (int)SendMessageA(hScannerResults, LVM_INSERTITEMA, 0, (LPARAM)&lvi);
            ListView_SetItemText(hScannerResults, idx, SCOL_CHGPCT, (LPSTR)SCANNER_NO_DATA);
            ListView_SetItemText(hScannerResults, idx, SCOL_LAST,   (LPSTR)SCANNER_NO_DATA);

            g_ScannerEntries[row.conId] = row.symbol;
        }

        // Subscribe live L1 data for the returned symbols — reuses the same
        // mechanism the Watchlist window uses; Change%/Last arrive via
        // WM_MARKET_L1 below.
        api().setWatchlistWindow(hWnd, g_ScannerEntries);
        break;
    }

    // ── Live L1 update for one scanner row ────────────────────────────────────
    case WM_MARKET_L1: {
        int conId = (int)lParam;
        if (!conId || !g_ScannerEntries.count(conId)) break;

        TradingAPI::L1Book info;
        if (api().getWatchlistData(conId, info)) {
            int row = Scanner_FindRow(conId);
            if (row >= 0 && info.last > 0.0) {
                ListView_SetItemText(hScannerResults, row, SCOL_LAST,
                    (LPSTR)std::format("{:.2f}", info.last).c_str());
                if (info.prevClose > 0.0) {
                    ListView_SetItemText(hScannerResults, row, SCOL_CHGPCT,
                        (LPSTR)std::format("{:+.2f}%", info.changePct()).c_str());
                }
            }
        }
        break;
    }

    case WM_API_UPDATE:
        if (api().isMarketDataConnected() && api().isTradingConnected()) {
            Scanner_Subscribe(hWnd, g_ScannerActiveIndex); // re-request after reconnect
        } else {
            Scanner_ClearList(hWnd);
            g_ScannerEntries.clear();
        }
        break;

    case WM_NOTIFY: {
        NMHDR* hdr = (NMHDR*)lParam;
        if (hdr->idFrom != ID_SCANNER_RESULTS_LIST) break;

        if (hdr->code == NM_DBLCLK) {
            NMITEMACTIVATE* nm = (NMITEMACTIVATE*)lParam;
            if (nm->iItem >= 0) {
                LVITEMA lvi = {};
                lvi.mask  = LVIF_PARAM;
                lvi.iItem = nm->iItem;
                SendMessageA(hScannerResults, LVM_GETITEMA, 0, (LPARAM)&lvi);
                int conId = (int)lvi.lParam;
                auto it = g_ScannerEntries.find(conId);
                if (it != g_ScannerEntries.end()) StartMarket(it->second, conId);
            }
            return 0;
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
                    if (cd->iSubItem == SCOL_CHGPCT) {
                        char buf[32] = {};
                        ListView_GetItemText(hScannerResults, (int)cd->nmcd.dwItemSpec, SCOL_CHGPCT, buf, sizeof(buf));
                        if (strcmp(buf, SCANNER_NO_DATA) != 0 && buf[0] != '\0') {
                            double v = atof(buf);
                            if      (v > 0.0) cd->clrText = COINS_CLR_GREEN;
                            else if (v < 0.0) cd->clrText = COINS_CLR_RED;
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
        api().cancelScanner();
        api().unsetWatchlistWindow(hWnd);
        api().removeApiUpdateWindow(hWnd);
        g_ScannerEntries.clear();
        hScannerResults = hScannerBtnNASDAQSCM = hScannerBtnNYSE = hScannerBtnNASDAQNational = NULL;
        if (ScannerZoomData.hFont)     DeleteObject(ScannerZoomData.hFont);
        if (ScannerZoomData.hBoldFont) DeleteObject(ScannerZoomData.hBoldFont);
        break;
    }

    return HandleCommonMessages(hWnd, message, wParam, lParam);
}
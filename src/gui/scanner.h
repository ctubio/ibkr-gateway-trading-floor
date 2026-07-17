#pragma once

void StartScanner() { StartGenericWindow(SCANNER_CLASS_NAME, "Scanner", L"TWSAPIClientTradingFloor.Scanner", 432, 600); }

#define ID_SCANNER_RESULTS_LIST         3001
#define ID_SCANNER_BTN_NYSE             3002
#define ID_SCANNER_BTN_NASDAQ_NATIONAL  3003
#define ID_SCANNER_BTN_NASDAQ_SCM       3004
#define ID_SCANNER_COMBO_SCANCODE       3005

enum ScannerColIdx { SCOL_INSTRUMENT = 0, SCOL_CHGPCT, SCOL_LAST, SCOL_COUNT };

// Scan-code selector options — independent of the NYSE/NASDAQ location radio
// buttons above. Index matches TradingAPI::runScanner()'s scanCodeIndex param.
enum ScannerScanCodeIdx { SCANCODE_TOP_PERC_GAIN = 0, SCANCODE_TOP_PERC_LOSE, SCANCODE_MOST_ACTIVE, SCANCODE_COUNT };
static const char* g_ScannerScanCodeLabels[SCANCODE_COUNT] = { "GAIN", "LOSE", "ACTIVE" };

static HWND hScannerResults   = NULL;
static HWND hScannerBtnNYSE    = NULL;
static HWND hScannerBtnNASDAQNational  = NULL;
static HWND hScannerBtnNASDAQSCM  = NULL;
static HWND hScannerComboScanCode = NULL;
static int  g_ScannerActiveId = 0;   // 0 = NYSE, 1 = NASDAQ National, 2 = NASDAQ SCM
static int  g_ScannerScanCodeId = 0; // 0 = TOP_PERC_GAIN, 1 = TOP_PERC_LOSE, 2 = MOST_ACTIVE

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
        int btnW = 90;
        int btnY = rc.bottom - SCANNER_SELECTOR_H + (SCANNER_SELECTOR_H - SCANNER_BTN_H) / 2;
        int totalW = (btnW * 3) + (m * 2);
        int startX = 10;
        MoveWindow(hScannerResults, 0, 0, rc.right, rc.bottom - SCANNER_SELECTOR_H, TRUE);
        SetWindowPos(hScannerBtnNYSE,           NULL, startX,                          btnY, btnW-20, SCANNER_BTN_H, SWP_NOZORDER | SWP_NOACTIVATE);
        SetWindowPos(hScannerBtnNASDAQNational, NULL, startX + btnW-20 + m,            btnY, btnW+10, SCANNER_BTN_H, SWP_NOZORDER | SWP_NOACTIVATE);
        SetWindowPos(hScannerBtnNASDAQSCM,      NULL, startX + btnW-20 + m + btnW + m, btnY, btnW+10, SCANNER_BTN_H, SWP_NOZORDER | SWP_NOACTIVATE);

        // Scan-code selector (TOP_PERC_GAIN / TOP_PERC_LOSE / MOST_ACTIVE) —
        // anchored to the far right of the bottom bar, independent of the
        // location radio-button group. Height passed here is the combo's
        // drop-down list height, not the closed-box height (fixed by the
        // system to roughly the font's line height), so it's kept tall
        // enough that the list isn't clipped when opened.
        int comboW = 110;
        int comboDropH = 200;
        SetWindowPos(hScannerComboScanCode, NULL, rc.right - comboW - m, btnY-3, comboW, comboDropH, SWP_NOZORDER | SWP_NOACTIVATE);
    } else {
        MoveWindow(hScannerResults, 0, 0, rc.right, rc.bottom, TRUE);
    }
}

// ── Subscribe / switch scanner ────────────────────────────────────────────────

// Re-requests the scan using the current g_ScannerActiveId (location) and
// g_ScannerScanCodeId (TOP_PERC_GAIN / TOP_PERC_LOSE / MOST_ACTIVE), and syncs
// the UI controls to match. Called whenever either selection changes, and on
// initial creation / reconnect.
static void Scanner_ApplySelection(HWND hWnd) {
    Settings_Scanner_Save("LastIndex", (DWORD)g_ScannerActiveId);
    Settings_Scanner_Save("LastScanCode", (DWORD)g_ScannerScanCodeId);

    if (hScannerBtnNYSE)           SendMessage(hScannerBtnNYSE,           BM_SETCHECK, g_ScannerActiveId == 0 ? BST_CHECKED : BST_UNCHECKED, 0);
    if (hScannerBtnNASDAQNational) SendMessage(hScannerBtnNASDAQNational, BM_SETCHECK, g_ScannerActiveId == 1 ? BST_CHECKED : BST_UNCHECKED, 0);
    if (hScannerBtnNASDAQSCM)      SendMessage(hScannerBtnNASDAQSCM,      BM_SETCHECK, g_ScannerActiveId == 2 ? BST_CHECKED : BST_UNCHECKED, 0);
    if (hScannerComboScanCode)     SendMessage(hScannerComboScanCode, CB_SETCURSEL, g_ScannerScanCodeId, 0);

    Scanner_ClearList(hWnd);
    g_ScannerEntries.clear();
    api().unsetWatchlistWindow(hWnd); // drop L1 subscriptions for the previous rows

    const char* locationName = g_ScannerActiveId == 0 ? "New York Stock Exchange" : (g_ScannerActiveId == 1 ? "NASDAQ National Market" : "NASDAQ Small/Mid Caps");
    std::string title = std::string("Scanner: ") + locationName + " - " + g_ScannerScanCodeLabels[g_ScannerScanCodeId];
    SetWindowTextA(hWnd, title.c_str());
    api().runScanner(g_ScannerActiveId, g_ScannerScanCodeId);
}

// Switch the NYSE/NASDAQ-National/NASDAQ-SCM location.
static void Scanner_Subscribe(HWND hWnd, int scannerId) {
    g_ScannerActiveId = scannerId;
    Scanner_ApplySelection(hWnd);
}

// Switch the scan code (TOP_PERC_GAIN / TOP_PERC_LOSE / MOST_ACTIVE).
static void Scanner_SetScanCode(HWND hWnd, int scanCodeId) {
    g_ScannerScanCodeId = scanCodeId;
    Scanner_ApplySelection(hWnd);
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

        lvc.fmt = LVCFMT_LEFT;  lvc.cx = 100; lvc.pszText = (LPSTR)"Symbol";    ListView_InsertColumn(hScannerResults, SCOL_INSTRUMENT, &lvc);
        lvc.fmt = LVCFMT_RIGHT; lvc.cx = 100; lvc.pszText = (LPSTR)"Change %";  ListView_InsertColumn(hScannerResults, SCOL_CHGPCT,     &lvc);
        lvc.fmt = LVCFMT_RIGHT; lvc.cx = 100; lvc.pszText = (LPSTR)"Last";      ListView_InsertColumn(hScannerResults, SCOL_LAST,       &lvc);

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

        // Scan-code selector (TOP_PERC_GAIN / TOP_PERC_LOSE / MOST_ACTIVE) —
        // hidden until activated, same as the location radio buttons above.
        hScannerComboScanCode = CreateWindowA("COMBOBOX", "",
            WS_CHILD | CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_VSCROLL,
            0, 0, 130, 200, hWnd, (HMENU)ID_SCANNER_COMBO_SCANCODE, hInst, NULL);
        SendMessage(hScannerComboScanCode, WM_SETFONT, (WPARAM)ScannerZoomData.hFont, TRUE);
        for (int i = 0; i < SCANCODE_COUNT; ++i)
            SendMessageA(hScannerComboScanCode, CB_ADDSTRING, 0, (LPARAM)g_ScannerScanCodeLabels[i]);

        int savedIndex = (int)Settings_Scanner_Load("LastIndex", 0);
        g_ScannerActiveId = (savedIndex == 1) ? 1 : 0;

        int savedScanCode = (int)Settings_Scanner_Load("LastScanCode", 0);
        g_ScannerScanCodeId = (savedScanCode >= 0 && savedScanCode < SCANCODE_COUNT) ? savedScanCode : 0;
        SendMessage(hScannerComboScanCode, CB_SETCURSEL, g_ScannerScanCodeId, 0);

        api().addApiUpdateWindow(hWnd);
        Scanner_ApplySelection(hWnd);
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
            ShowWindow(hScannerComboScanCode,     SW_SHOW);
        } else {
            ShowWindow(hScannerBtnNYSE,           SW_HIDE);
            ShowWindow(hScannerBtnNASDAQNational, SW_HIDE);
            ShowWindow(hScannerBtnNASDAQSCM,      SW_HIDE);
            ShowWindow(hScannerComboScanCode,     SW_HIDE);
        }
        Scanner_Layout(hWnd);
        return 0;

    case WM_COMMAND:
        if ((LOWORD(wParam) == ID_SCANNER_BTN_NYSE || LOWORD(wParam) == ID_SCANNER_BTN_NASDAQ_NATIONAL || LOWORD(wParam) == ID_SCANNER_BTN_NASDAQ_SCM) && HIWORD(wParam) == BN_CLICKED) {
            int scannerId = g_ScannerActiveId;
            if (LOWORD(wParam) == ID_SCANNER_BTN_NYSE) {
                scannerId = 0;
            } else if (LOWORD(wParam) == ID_SCANNER_BTN_NASDAQ_NATIONAL) {
                scannerId = 1;
            } else if (LOWORD(wParam) == ID_SCANNER_BTN_NASDAQ_SCM){
                scannerId = 2;
            }
            if (scannerId != g_ScannerActiveId) Scanner_Subscribe(hWnd, scannerId);
        }
        if (LOWORD(wParam) == ID_SCANNER_COMBO_SCANCODE && HIWORD(wParam) == CBN_SELCHANGE) {
            int scanCodeId = (int)SendMessageA(hScannerComboScanCode, CB_GETCURSEL, 0, 0);
            if (scanCodeId >= 0 && scanCodeId != g_ScannerScanCodeId) Scanner_SetScanCode(hWnd, scanCodeId);
        }
        break;

    // ── Scanner subscription refreshed (rank/contract only, no price yet) ────
    case WM_SCANNER_DATA: {
        auto rows = api().getScannerResults();

        // Snapshot the currently displayed Change%/Last for each conId before
        // clearing, so a routine scan refresh doesn't blank rows for the few
        // seconds it takes fresh snapshot data to arrive. Symbols that are
        // genuinely new (no prior row) still fall back to SCANNER_NO_DATA.
        std::unordered_map<int, std::pair<std::string, std::string>> prevValues;
        {
            int count = ListView_GetItemCount(hScannerResults);
            for (int i = 0; i < count; ++i) {
                LVITEMA lvi = {};
                lvi.mask  = LVIF_PARAM;
                lvi.iItem = i;
                SendMessageA(hScannerResults, LVM_GETITEMA, 0, (LPARAM)&lvi);
                int conId = (int)lvi.lParam;
                if (conId <= 0) continue;

                char chgBuf[32]  = {};
                char lastBuf[32] = {};
                ListView_GetItemText(hScannerResults, i, SCOL_CHGPCT, chgBuf,  sizeof(chgBuf));
                ListView_GetItemText(hScannerResults, i, SCOL_LAST,   lastBuf, sizeof(lastBuf));
                prevValues[conId] = { chgBuf, lastBuf };
            }
        }

        Scanner_ClearList(hWnd);
        g_ScannerEntries.clear();

        for (const auto& row : rows) {
            if (row.conId <= 0) continue;
            std::string label = row.symbol;
            // if (!row.exchange.empty()) label += "." + row.exchange;

            LVITEMA lvi = {};
            lvi.mask     = LVIF_TEXT | LVIF_PARAM;
            lvi.iItem    = ListView_GetItemCount(hScannerResults);
            lvi.lParam   = (LPARAM)row.conId;
            lvi.pszText  = (LPSTR)label.c_str();
            int idx = (int)SendMessageA(hScannerResults, LVM_INSERTITEMA, 0, (LPARAM)&lvi);

            auto pit = prevValues.find(row.conId);
            if (pit != prevValues.end()) {
                ListView_SetItemText(hScannerResults, idx, SCOL_CHGPCT, (LPSTR)pit->second.first.c_str());
                ListView_SetItemText(hScannerResults, idx, SCOL_LAST,   (LPSTR)pit->second.second.c_str());
            } else {
                ListView_SetItemText(hScannerResults, idx, SCOL_CHGPCT, (LPSTR)SCANNER_NO_DATA);
                ListView_SetItemText(hScannerResults, idx, SCOL_LAST,   (LPSTR)SCANNER_NO_DATA);
            }

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
            double displayLast = (info.last > 0.0) ? info.last : info.prevClose;
            if (row >= 0 && displayLast > 0.0) {
                ListView_SetItemText(hScannerResults, row, SCOL_LAST,
                    (LPSTR)std::format("{:.2f}", displayLast).c_str());
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
            Scanner_ApplySelection(hWnd); // re-request (same location + scan code) after reconnect
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
        hScannerResults = hScannerBtnNASDAQSCM = hScannerBtnNYSE = hScannerBtnNASDAQNational = hScannerComboScanCode = NULL;
        if (ScannerZoomData.hFont)     DeleteObject(ScannerZoomData.hFont);
        if (ScannerZoomData.hBoldFont) DeleteObject(ScannerZoomData.hBoldFont);
        break;
    }

    return HandleCommonMessages(hWnd, message, wParam, lParam);
}
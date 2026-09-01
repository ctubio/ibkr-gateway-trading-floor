#pragma once

// ─── Alerts editor popup ──────────────────────────────────────────────────
//
// A tiny single-instance dialog-style window (same pattern as
// DASHBOARD_EXCHANGE_CLASS_NAME) for setting/editing a per-symbol "Alert Up"
// / "Alert Down" price pair. Opened from the Market window's flag icon and
// from the Diamonds window's "Edit Alerts" context menu item.
//
// Values are stored in the registry under a dedicated "Alerts" subkey (see
// Settings_Alerts_* in registry.h), keyed by SYMBOL_CONID_UP / SYMBOL_CONID_DOWN.
// Nothing here checks live prices or fires notifications yet — that's a later step.

#define ID_ALERTS_UP_EDIT    5301
#define ID_ALERTS_DOWN_EDIT  5302

struct AlertsEditState {
    std::string symbol;   // symbol currently loaded into the popup
    int conId = 0;        // conId currently loaded into the popup
};
static AlertsEditState alertsEditState;

// Posts WM_ALERTS_CHANGED directly to the two window classes that actually
// handle it (WndProcMarket / WndProcDiamonds) instead of enumerating every
// top-level window in the process and filtering by PID. Diamonds is
// single-instance; Market can have many, so we walk all of them the same way
// EnumerateMarketWindows() in registry.h does.
static void Alerts_NotifyChanged(int conId) {
    HWND hDiamonds = FindWindowA(DIAMONDS_CLASS_NAME, NULL);
    if (hDiamonds && IsWindow(hDiamonds))
        PostMessage(hDiamonds, WM_ALERTS_CHANGED, 0,  (LPARAM)conId);
    
    auto tsWindows = EnumerateMarketWindows();
    for (size_t i = 0; i < tsWindows.size() && i < 100; ++i) {
        TradingAPI::MarketInitData* data = (TradingAPI::MarketInitData*)GetWindowLongPtr(tsWindows[i].hWnd, GWLP_USERDATA);
        if (data && data->conId == conId) {
            PostMessage(tsWindows[i].hWnd, WM_ALERTS_CHANGED, 0,  (LPARAM)conId);
            break;
        }
    }
}

// (Re)loads the popup for `symbol` and `conId`: updates the title and both edit fields
// from whatever is currently saved in the registry (empty if none), and
// focuses/selects the Alert Up field. Safe to call on an already-open popup
// (single-instance window) to repoint it at a different symbol.
static void AlertsEditor_Populate(HWND hWnd, const std::string& symbol, int conId) {
    alertsEditState.symbol = symbol;
    alertsEditState.conId  = conId;
    SetWindowTextA(hWnd, ("Edit Alerts: " + symbol).c_str());

    std::string upStr, downStr;
    Settings_Alerts_Load(symbol, conId, upStr, downStr);

    HWND hUp   = GetDlgItem(hWnd, ID_ALERTS_UP_EDIT);
    HWND hDown = GetDlgItem(hWnd, ID_ALERTS_DOWN_EDIT);
    if (hUp)   SetWindowTextA(hUp,   upStr.c_str());
    if (hDown) SetWindowTextA(hDown, downStr.c_str());

    if (hUp) {
        SetFocus(hUp);
        int len = GetWindowTextLengthA(hUp);
        SendMessageA(hUp, EM_SETSEL, 0, len);
    }
}

// ENTER: save both fields (empty = remove that direction's key) and close.
static void AlertsEditor_SaveAndClose(HWND hWnd) {
    HWND hUp   = GetDlgItem(hWnd, ID_ALERTS_UP_EDIT);
    HWND hDown = GetDlgItem(hWnd, ID_ALERTS_DOWN_EDIT);

    char upBuf[32] = {}, downBuf[32] = {};
    if (hUp)   GetWindowTextA(hUp,   upBuf,   sizeof(upBuf));
    if (hDown) GetWindowTextA(hDown, downBuf, sizeof(downBuf));

    auto trim = [](std::string s) -> std::string {
        size_t b = s.find_first_not_of(" \t\r\n");
        size_t e = s.find_last_not_of(" \t\r\n");
        return (b == std::string::npos) ? std::string() : s.substr(b, e - b + 1);
    };
    std::string upStr   = trim(upBuf);
    std::string downStr = trim(downBuf);

    if (!alertsEditState.symbol.empty() && alertsEditState.conId > 0) {
        Settings_Alerts_Save(alertsEditState.symbol, alertsEditState.conId, upStr, downStr);
        Alerts_NotifyChanged(alertsEditState.conId);

    }

    DestroyWindow(hWnd);
}

// Subclass shared by both edit fields:
//   ESC    -> close without saving
//   ENTER  -> save + close
//   TAB    -> toggle focus between Alert Up / Alert Down
static LRESULT CALLBACK AlertsEditor_KeySubclassProc(HWND hCtrl, UINT msg, WPARAM wParam, LPARAM lParam,
                                                      UINT_PTR uIdSubclass, DWORD_PTR /*dwRefData*/) {
    if (msg == WM_CHAR) {
        if (wParam == VK_RETURN || wParam == VK_ESCAPE || wParam == VK_TAB)
            return 0;
    }
    if (msg == WM_KEYDOWN) {
        HWND hParent = GetParent(hCtrl);
        if (wParam == VK_ESCAPE) {
            DestroyWindow(hParent);  // close without saving
            return 0;
        }
        if (wParam == VK_RETURN) {
            AlertsEditor_SaveAndClose(hParent);
            return 0;
        }
        if (wParam == VK_TAB) {
            HWND hUp   = GetDlgItem(hParent, ID_ALERTS_UP_EDIT);
            HWND hDown = GetDlgItem(hParent, ID_ALERTS_DOWN_EDIT);
            HWND hNext = (hCtrl == hUp) ? hDown : hUp;
            if (hNext) {
                SetFocus(hNext);
                int len = GetWindowTextLengthA(hNext);
                SendMessageA(hNext, EM_SETSEL, 0, len);
            }
            return 0;
        }
        if (wParam == VK_UP || wParam == VK_DOWN) {
            char buf[32] = {};
            GetWindowTextA(hCtrl, buf, sizeof(buf));
            double val  = atof(buf);
            double step = ((GetKeyState(VK_SHIFT) & 0x8000) != 0) ? 1.0 : 0.01;
            val += (wParam == VK_UP) ? step : -step;
            if (val < 0.0) val = 0.0;
            std::string s = std::format("{:.2f}", val);
            SetWindowTextA(hCtrl, s.c_str());
            int len = GetWindowTextLengthA(hCtrl);
            SendMessageA(hCtrl, EM_SETSEL, len, len);
            return 0;
        }
    }
    if (msg == WM_NCDESTROY)
        RemoveWindowSubclass(hCtrl, AlertsEditor_KeySubclassProc, uIdSubclass);
    return DefSubclassProc(hCtrl, msg, wParam, lParam);
}

LRESULT CALLBACK WndProcAlerts(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE: {
            HINSTANCE hInst = ((LPCREATESTRUCT)lParam)->hInstance;

            CreateWindowA("STATIC", "Alert Up:",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                12, 16, 80, 20, hWnd, NULL, hInst, NULL);
            HWND hUp = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
                WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_CENTER,
                96, 12, 130, 24, hWnd, (HMENU)ID_ALERTS_UP_EDIT, hInst, NULL);

            CreateWindowA("STATIC", "Alert Down:",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                12, 50, 80, 20, hWnd, NULL, hInst, NULL);
            HWND hDown = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
                WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_CENTER,
                96, 46, 130, 24, hWnd, (HMENU)ID_ALERTS_DOWN_EDIT, hInst, NULL);

            SendMessage(hUp,   WM_SETFONT, (WPARAM)hFont12pt.get(), TRUE);
            SendMessage(hDown, WM_SETFONT, (WPARAM)hFont12pt.get(), TRUE);

            SetWindowSubclass(hUp,   AlertsEditor_KeySubclassProc, 1, 0);
            SetWindowSubclass(hDown, AlertsEditor_KeySubclassProc, 2, 0);
            break;
        }

        case WM_KEYDOWN: {
            if (wParam == VK_ESCAPE) {
                DestroyWindow(hWnd);
                return 0;
            }
            break;
        }

        case WM_DESTROY:
            alertsEditState.symbol.clear();
            alertsEditState.conId = 0;
            break;
    }
    return HandleCommonMessages(hWnd, message, wParam, lParam);
}

// Opens (or refocuses) the Alerts editor popup for `symbol` and `conId`. Single-instance:
// if already open (e.g. for a different symbol), it's repointed at `symbol`
// instead of a second window being created.
void StartAlertsEditor(const std::string& symbol, int conId) {
    HWND hWnd = StartGenericWindow(ALERTS_CLASS_NAME, "Edit Alerts", L"TWSAPIClientTradingFloor.Alerts", 260, 130);
    if (hWnd) AlertsEditor_Populate(hWnd, symbol, conId);
}

// Helper window procedure for the flash overlay
static LRESULT CALLBACK FlashWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_PAINT) {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        HBRUSH hBrush = (HBRUSH)GetWindowLongPtr(hwnd, GWLP_USERDATA);
        FillRect(hdc, &ps.rcPaint, hBrush);
        EndPaint(hwnd, &ps);
        return 0;
    }
    if (msg == WM_LBUTTONDOWN || msg == WM_KEYDOWN) {
        DestroyWindow(hwnd); // Dismiss early if clicked or keyed
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// Triggers a full screen flash (isGreen = true for green, false for red)
void FlashScreen(bool isGreen, int durationMs = 800) {
    std::thread([isGreen, durationMs]() {
        HINSTANCE hInstance = GetModuleHandle(NULL);
        WNDCLASSA wc = {};
        wc.lpfnWndProc   = FlashWndProc;
        wc.hInstance     = hInstance;
        wc.lpszClassName = "ScreenFlashOverlay";
        wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
        RegisterClassA(&wc);

        // Pick color: Red or Green with alpha transparency (~50% opacity so you can still see charts)
        COLORREF rgb = isGreen ? COINS_CLR_GREEN : COINS_CLR_RED;
        HBRUSH hBrush = CreateSolidBrush(rgb);

        // Cover all virtual screens (supports multi-monitor setups)
        int x = GetSystemMetrics(SM_XVIRTUALSCREEN);
        int y = GetSystemMetrics(SM_YVIRTUALSCREEN);
        int w = GetSystemMetrics(SM_CXVIRTUALSCREEN);
        int h = GetSystemMetrics(SM_CYVIRTUALSCREEN);

        // WS_EX_LAYERED + WS_EX_TRANSPARENT allows clicks to pass *right through* the flash overlay
        HWND hwnd = CreateWindowExA(
            WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW,
            "ScreenFlashOverlay", "",
            WS_POPUP | WS_VISIBLE,
            x, y, w, h,
            NULL, NULL, hInstance, NULL
        );

        if (!hwnd) {
            DeleteObject(hBrush);
            return;
        }

        // Set 45% transparency (69 out of 255) so it flashes nicely without blinding you
        SetLayeredWindowAttributes(hwnd, 0, 69, LWA_ALPHA);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)hBrush);
        ShowWindow(hwnd, SW_SHOW);
        UpdateWindow(hwnd);

        // Wait for the duration of the flash
        std::this_thread::sleep_for(std::chrono::milliseconds(durationMs));

        DestroyWindow(hwnd);
        UnregisterClassA("ScreenFlashOverlay", hInstance);
        DeleteObject(hBrush);
    }).detach();
}

#define ID_ALERT_KEEP_BTN   5401
#define ID_ALERT_DELETE_BTN 5402

struct AlertPopupData {
    std::string title;
    std::string msg;
    std::string symbol;
    int conId;
    bool isUp;
};

LRESULT CALLBACK WndProcAlertNotification(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE: {
            CREATESTRUCT* cs = (CREATESTRUCT*)lParam;
            AlertPopupData* data = (AlertPopupData*)cs->lpCreateParams;
            SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)data);

            // Message text
            HWND hMsg = CreateWindowA("STATIC", data->msg.c_str(),
                WS_CHILD | WS_VISIBLE | SS_CENTER,
                8, 20, 260, 60, hWnd, NULL, cs->hInstance, NULL);
            SendMessage(hMsg, WM_SETFONT, (WPARAM)hFont12ptbold.get(), TRUE);
            SetCtrlColor(hMsg, data->isUp ? COINS_CLR_GREEN : COINS_CLR_RED);
            
            // Buttons
            HWND hKeep = CreateWindowA("BUTTON", "Keep",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_OWNERDRAW,
                0, 100, (300 / 2) - 4, 22, hWnd, (HMENU)ID_ALERT_KEEP_BTN, cs->hInstance, NULL);
            SendMessage(hKeep, WM_SETFONT, (WPARAM)hFont11pt.get(), TRUE);
            
            HWND hDelete = CreateWindowA("BUTTON", "Delete",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_OWNERDRAW,
                (300 / 2) + 4, 100, (300 / 2) - 4, 22, hWnd, (HMENU)ID_ALERT_DELETE_BTN, cs->hInstance, NULL);
            SendMessage(hDelete, WM_SETFONT, (WPARAM)hFont11pt.get(), TRUE);

            break;
        }
        case WM_COMMAND: {
            int wmId = LOWORD(wParam);
            if (wmId == ID_ALERT_KEEP_BTN || wmId == ID_ALERT_DELETE_BTN) {
                if (wmId == ID_ALERT_DELETE_BTN) {
                    AlertPopupData* data = (AlertPopupData*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
                    if (data) {
                        std::string upStr, downStr;
                        Settings_Alerts_Load(data->symbol, data->conId, upStr, downStr);
                        if (data->isUp) upStr = "";
                        else downStr = "";
                        Settings_Alerts_Save(data->symbol, data->conId, upStr, downStr);
                        Alerts_NotifyChanged(data->conId);
                    }
                }
                DestroyWindow(hWnd);
            }
            break;
        }
        case WM_DESTROY: {
            AlertPopupData* data = (AlertPopupData*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
            if (data) delete data;
            break;
        }
    }
    return HandleCommonMessages(hWnd, message, wParam, lParam);
}


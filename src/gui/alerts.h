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
#define ID_ALERT_EDIT_BTN    5303

struct AlertsEditState {
    std::string symbol;   // symbol currently loaded into the popup
    int conId = 0;        // conId currently loaded into the popup
};
static AlertsEditState alertsEditState;

void Market_NotifyAlertsChanged(int conId) {
    for (const auto& [hWnd, st] : marketStates) {
        if (st && st->conId == conId) {
            PostMessage(hWnd, WM_ALERTS_CHANGED, 0, (LPARAM)conId);
            break;
        }
    }
}

// Posts WM_ALERTS_CHANGED directly to the two window classes that actually
// handle it (WndProcMarket / WndProcDiamonds) instead of enumerating every
// top-level window in the process.
static void Alerts_NotifyChanged(int conId) {
    HWND hDiamonds = FindWindowA(DIAMONDS_CLASS_NAME, NULL);
    if (hDiamonds && IsWindow(hDiamonds))
        PostMessage(hDiamonds, WM_ALERTS_CHANGED, 0,  (LPARAM)conId);
    
    Market_NotifyAlertsChanged(conId);
}

// (Re)loads the popup for `symbol` and `conId`: updates the title and both edit fields
// from whatever is currently saved in the registry (empty if none), and
// focuses/selects the Alert Up field. Safe to call on an already-open popup
// (single-instance window) to repoint it at a different symbol.
static void AlertEditor_Populate(HWND hWnd, const std::string& symbol, int conId, double price) {
    alertsEditState.symbol = symbol;
    alertsEditState.conId  = conId;
    SetWindowTextA(hWnd, std::format("{} at {:.2f}", symbol, price).c_str());

    std::string upStr, downStr;
    Settings_Alerts_Load(symbol, conId, upStr, downStr);

    HWND hUp   = GetDlgItem(hWnd, ID_ALERTS_UP_EDIT);
    HWND hDown = GetDlgItem(hWnd, ID_ALERTS_DOWN_EDIT);
    if (hUp)   SetWindowTextA(hUp,   upStr.c_str());
    if (hDown) SetWindowTextA(hDown, downStr.c_str());
    CenterEditText(hUp);
    CenterEditText(hDown);

    if (hUp) {
        SetFocus(hUp);
        int len = GetWindowTextLengthA(hUp);
        SendMessageA(hUp, EM_SETSEL, 0, len);
    }
}

// ENTER: save both fields (empty = remove that direction's key) and close.
static void AlertEditor_SaveAndClose(HWND hWnd) {
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
static LRESULT CALLBACK AlertEditor_KeySubclassProc(HWND hCtrl, UINT msg, WPARAM wParam, LPARAM lParam,
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
            AlertEditor_SaveAndClose(hParent);
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
            std::string s = std::format("{:.0f}", val);
            SetWindowTextA(hCtrl, s.c_str());
            int len = GetWindowTextLengthA(hCtrl);
            SendMessageA(hCtrl, EM_SETSEL, len, len);
            return 0;
        }
    }
    if (msg == WM_NCDESTROY)
        RemoveWindowSubclass(hCtrl, AlertEditor_KeySubclassProc, uIdSubclass);
    return DefSubclassProc(hCtrl, msg, wParam, lParam);
}

LRESULT CALLBACK WndProcAlertsEditor(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE: {
            HINSTANCE hInst = ((LPCREATESTRUCT)lParam)->hInstance;

            int editH = 37;
            int editW = 130;

            CreateWindowA("STATIC", "Alert Up:",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                12, 20, 80, 20, hWnd, NULL, hInst, NULL);
            HWND hUp = CreateWindowA("EDIT", "",
                WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_BORDER | ES_AUTOHSCROLL | ES_CENTER | ES_MULTILINE,
                96, 12, editW, editH, hWnd, (HMENU)ID_ALERTS_UP_EDIT, hInst, NULL);

            CreateWindowA("STATIC", "Alert Down:",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                12, 20 + editH + 8, 80, 20, hWnd, NULL, hInst, NULL);
            HWND hDown = CreateWindowA("EDIT", "",
                WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_BORDER | ES_AUTOHSCROLL | ES_CENTER | ES_MULTILINE,
                96, 12 + editH + 8, editW, editH, hWnd, (HMENU)ID_ALERTS_DOWN_EDIT, hInst, NULL);

            SetWindowSubclass(hUp,   AlertEditor_KeySubclassProc, 1, 0);
            SetWindowSubclass(hDown, AlertEditor_KeySubclassProc, 2, 0);

            SendMessage(hUp,   WM_SETFONT, (WPARAM)hFont16ptbold.get(), TRUE);
            SendMessage(hDown, WM_SETFONT, (WPARAM)hFont16ptbold.get(), TRUE);
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
void StartAlertEditor(const std::string& symbol, int conId, double price) {
    HWND hWnd = StartGenericWindow(ALERTS_EDITOR_CLASS_NAME, "Edit Alerts", L"TWSAPIClientTradingFloor.Alerts", 245, 135);
    if (hWnd) AlertEditor_Populate(hWnd, symbol, conId, price);
}

#define IDT_SCREEN_FLASH_TIMER 5450
static const BYTE FLASH_PEAK_ALPHA = 69;

static HWND  hScreenFlashOverlay = NULL;
static DWORD s_flashStartTime      = 0;
static int   s_flashDurationMs     = 800;

// SetTimer callback to adjust opacity and hide the pre-created fullscreen overlay window
static VOID CALLBACK FlashTimerProc(HWND hwnd, UINT /*uMsg*/, UINT_PTR idEvent, DWORD dwTime) {
    if (idEvent != IDT_SCREEN_FLASH_TIMER) return;

    DWORD now = dwTime ? dwTime : GetTickCount();
    DWORD elapsed = now - s_flashStartTime;

    if (elapsed >= (DWORD)s_flashDurationMs || s_flashDurationMs <= 0) {
        KillTimer(hwnd, idEvent);
        SetLayeredWindowAttributes(hwnd, 0, 0, LWA_ALPHA);
        ShowWindow(hwnd, SW_HIDE);
        return;
    }

    // Smoothly fade out opacity from peak alpha to 0
    float progress = (float)elapsed / (float)s_flashDurationMs;
    int currentAlpha = (int)(FLASH_PEAK_ALPHA * (1.0f - progress));
    if (currentAlpha < 0)   currentAlpha = 0;
    if (currentAlpha > 255) currentAlpha = 255;

    SetLayeredWindowAttributes(hwnd, 0, (BYTE)currentAlpha, LWA_ALPHA);
}

// Window procedure for the pre-created fullscreen flash overlay
LRESULT CALLBACK WndProcScreenFlashOverlay(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            HBRUSH hBrush = (HBRUSH)GetWindowLongPtr(hwnd, GWLP_USERDATA);
            if (!hBrush) hBrush = hBrushGreen;
            FillRect(hdc, &ps.rcPaint, hBrush);
            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_ERASEBKGND:
            return 1;

        case WM_LBUTTONDOWN:
        case WM_KEYDOWN:
            KillTimer(hwnd, IDT_SCREEN_FLASH_TIMER);
            SetLayeredWindowAttributes(hwnd, 0, 0, LWA_ALPHA);
            ShowWindow(hwnd, SW_HIDE);
            return 0;

        case WM_DESTROY:
            KillTimer(hwnd, IDT_SCREEN_FLASH_TIMER);
            return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// Pre-creates the layered fullscreen overlay window on the main UI thread
void CreateScreenFlashOverlay(HINSTANCE hInst = NULL) {
    if (hScreenFlashOverlay && IsWindow(hScreenFlashOverlay)) return;
    if (!hInst) hInst = GetModuleHandle(NULL);

    int x = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int y = GetSystemMetrics(SM_YVIRTUALSCREEN);
    int w = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int h = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    if (w == 0 || h == 0) {
        x = 0; y = 0;
        w = GetSystemMetrics(SM_CXSCREEN);
        h = GetSystemMetrics(SM_CYSCREEN);
    }

    hScreenFlashOverlay = CreateWindowExA(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
        SCREEN_FLASH_OVERLAY_CLASS_NAME, "",
        WS_POPUP,
        x, y, w, h,
        NULL, NULL, hInst, NULL
    );

    if (hScreenFlashOverlay) {
        SetLayeredWindowAttributes(hScreenFlashOverlay, 0, 0, LWA_ALPHA);
        ShowWindow(hScreenFlashOverlay, SW_HIDE);
    }
}

// Destroys the pre-created overlay window before unregistering the class
void DestroyScreenFlashOverlay() {
    if (hScreenFlashOverlay && IsWindow(hScreenFlashOverlay)) {
        KillTimer(hScreenFlashOverlay, IDT_SCREEN_FLASH_TIMER);
        DestroyWindow(hScreenFlashOverlay);
        hScreenFlashOverlay = NULL;
    }
}

// Triggers a full screen flash (isGreen = true for green, false for red)
void FlashScreen(bool isGreen, int durationMs = 800) {
    if (!fullScreenAlerts) return;

    if (!hScreenFlashOverlay || !IsWindow(hScreenFlashOverlay)) return;

    // Cover all virtual screens (supports multi-monitor setups)
    int x = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int y = GetSystemMetrics(SM_YVIRTUALSCREEN);
    int w = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int h = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    if (w == 0 || h == 0) {
        x = 0; y = 0;
        w = GetSystemMetrics(SM_CXSCREEN);
        h = GetSystemMetrics(SM_CYSCREEN);
    }

    SetWindowPos(hScreenFlashOverlay, HWND_TOPMOST, x, y, w, h, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_SHOWWINDOW);

    SetWindowLongPtr(hScreenFlashOverlay, GWLP_USERDATA, (LONG_PTR)(isGreen ? hBrushGreen : hBrushRed));
    InvalidateRect(hScreenFlashOverlay, NULL, TRUE);

    s_flashStartTime  = GetTickCount();
    s_flashDurationMs = (durationMs > 0) ? durationMs : 800;

    SetLayeredWindowAttributes(hScreenFlashOverlay, 0, FLASH_PEAK_ALPHA, LWA_ALPHA);
    ShowWindow(hScreenFlashOverlay, SW_SHOWNOACTIVATE);
    UpdateWindow(hScreenFlashOverlay);

    SetTimer(hScreenFlashOverlay, IDT_SCREEN_FLASH_TIMER, 20, FlashTimerProc);
}

#define ID_ALERT_KEEP_BTN   5401
#define ID_ALERT_DELETE_BTN 5402
#define ID_ALERT_SYMBOL     5403

struct AlertPopupData {
    std::string title;
    std::string msg;
    std::string symbol;
    double price;
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
            HWND hSymbol = CreateWindowA("STATIC", data->symbol.c_str(),
                WS_CHILD | WS_VISIBLE | SS_CENTER,
                10, 20, 140, 40, hWnd, (HMENU)ID_ALERT_SYMBOL, cs->hInstance, NULL);
            SendMessage(hSymbol, WM_SETFONT, (WPARAM)hFont21ptbold.get(), TRUE);
            
            HWND hMsg = CreateWindowA("STATIC", data->msg.c_str(),
                WS_CHILD | WS_VISIBLE | SS_CENTER,
                150, 20, 140, 40, hWnd, NULL, cs->hInstance, NULL);
            SendMessage(hMsg, WM_SETFONT, (WPARAM)hFont21ptbold.get(), TRUE);
            
            // Buttons
            HWND hKeep = CreateWindowA("BUTTON", "Keep",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_OWNERDRAW,
                0, 75, (300 / 3) - 8, 22, hWnd, (HMENU)ID_ALERT_KEEP_BTN, cs->hInstance, NULL);
            SendMessage(hKeep, WM_SETFONT, (WPARAM)hFont11pt.get(), TRUE);

            HWND hEdit = CreateWindowA("BUTTON", "Edit",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_OWNERDRAW,
                (300 / 3), 75, (300 / 3) - 8, 22, hWnd, (HMENU)ID_ALERT_EDIT_BTN, cs->hInstance, NULL);
            SendMessage(hEdit, WM_SETFONT, (WPARAM)hFont11pt.get(), TRUE);
            
            HWND hDelete = CreateWindowA("BUTTON", "Delete",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_OWNERDRAW,
                (300 / 3) * 2, 75, (300 / 3) - 8, 22, hWnd, (HMENU)ID_ALERT_DELETE_BTN, cs->hInstance, NULL);
            SendMessage(hDelete, WM_SETFONT, (WPARAM)hFont11pt.get(), TRUE);

            ShowWindow(hWnd, SW_SHOWNOACTIVATE);
            break;
        }

        case WM_CTLCOLORSTATIC: {
            int id = GetDlgCtrlID((HWND)lParam);
            if (id == ID_ALERT_SYMBOL) {
                AlertPopupData* data = (AlertPopupData*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
                if (data) {
                    HDC hdc = (HDC)wParam;
                    SetTextColor(hdc, data->isUp ? COINS_CLR_GREEN : COINS_CLR_RED);
                    SetBkMode(hdc, TRANSPARENT);
                    return (LRESULT)(darkMode ? hDarkBrush : hLightBrush);
                }
            }
            break;
        }

        case WM_COMMAND: {
            int wmId = LOWORD(wParam);
            if (wmId == ID_ALERT_KEEP_BTN || wmId == ID_ALERT_DELETE_BTN || wmId == ID_ALERT_EDIT_BTN) {
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
                if (wmId == ID_ALERT_EDIT_BTN) {
                    AlertPopupData* data = (AlertPopupData*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
                    if (data) {
                        StartAlertEditor(data->symbol, data->conId, data->price);
                    }
                }
                DestroyWindow(hWnd);
            }
            break;
        }

        case WM_KEYDOWN: {
            if (wParam == 'K' || wParam == 'k') {
                SendMessage(hWnd, WM_COMMAND, ID_ALERT_KEEP_BTN, 0);
                return 0;
            }
            if (wParam == 'E' || wParam == 'e') {
                SendMessage(hWnd, WM_COMMAND, ID_ALERT_EDIT_BTN, 0);
                return 0;
            }
            if (wParam == 'D' || wParam == 'd') {
                SendMessage(hWnd, WM_COMMAND, ID_ALERT_DELETE_BTN, 0);
                return 0;
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


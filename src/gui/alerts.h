#pragma once

// ─── Alerts editor popup ──────────────────────────────────────────────────
//
// A tiny single-instance dialog-style window (same pattern as
// DASHBOARD_EXCHANGE_CLASS_NAME) for setting/editing a per-symbol "Alert Up"
// / "Alert Down" price pair. Opened from the Market window's flag icon and
// from the Diamonds window's "Edit Alerts" context menu item.
//
// Values are stored in the registry under a dedicated "Alerts" subkey (see
// Settings_Alerts_* in registry.h), keyed by SYMBOL_UP / SYMBOL_DOWN. Nothing
// here checks live prices or fires notifications yet — that's a later step.

#define ID_ALERTS_UP_EDIT    5301
#define ID_ALERTS_DOWN_EDIT  5302

struct AlertsEditState {
    std::string symbol;   // symbol currently loaded into the popup
};
static AlertsEditState alertsEditState;

// Broadcasts WM_ALERTS_CHANGED to every top-level window of this process so
// any open Market window (flag icon color) and the Diamonds window (Alert
// Up/Down columns + Quarantine membership for alert-only symbols) refresh
// themselves from the registry. Mirrors WM_TTS_VOICE_CHANGED's broadcast.
static void Alerts_NotifyChanged() {
    EnumWindows([](HWND hw, LPARAM) -> BOOL {
        DWORD pid;
        GetWindowThreadProcessId(hw, &pid);
        if (pid == GetCurrentProcessId())
            PostMessage(hw, WM_ALERTS_CHANGED, 0, 0);
        return TRUE;
    }, 0);
}

// (Re)loads the popup for `symbol`: updates the title and both edit fields
// from whatever is currently saved in the registry (empty if none), and
// focuses/selects the Alert Up field. Safe to call on an already-open popup
// (single-instance window) to repoint it at a different symbol.
static void AlertsEditor_Populate(HWND hWnd, const std::string& symbol) {
    alertsEditState.symbol = symbol;
    SetWindowTextA(hWnd, ("Edit Alerts: " + symbol).c_str());

    std::string upStr, downStr;
    Settings_Alerts_Load(symbol, upStr, downStr);

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

    if (!alertsEditState.symbol.empty())
        Settings_Alerts_Save(alertsEditState.symbol, upStr, downStr);

    DestroyWindow(hWnd);
    Alerts_NotifyChanged();
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
            break;
    }
    return HandleCommonMessages(hWnd, message, wParam, lParam);
}

// Opens (or refocuses) the Alerts editor popup for `symbol`. Single-instance:
// if already open (e.g. for a different symbol), it's repointed at `symbol`
// instead of a second window being created.
void StartAlertsEditor(const std::string& symbol) {
    HWND hWnd = StartGenericWindow(ALERTS_CLASS_NAME, "Edit Alerts", L"TWSAPIClientTradingFloor.Alerts", 260, 130);
    if (hWnd) AlertsEditor_Populate(hWnd, symbol);
}
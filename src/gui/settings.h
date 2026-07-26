#pragma once

void StartSettings() { StartGenericWindow(SETTINGS_CLASS_NAME, "Settings", L"TWSAPIClientTradingFloor.Settings", 276, 530); }

void StartDebugLog() { StartGenericWindow(DEBUGLOG_CLASS_NAME, "Debug Log", L"TWSAPIClientTradingFloor.DebugLog", 790, 243); }

#define ID_SETTINGS_KILL_GATEWAY      4001
#define ID_SETTINGS_DARK_MODE         4002
#define ID_SETTINGS_PLAY_SOUNDS       4003
#define ID_SETTINGS_AUTO_GATEWAY      4004
#define ID_SETTINGS_DEBUG_LOG         4005
#define ID_SETTINGS_VOICE_COMBO       4006
#define ID_SETTINGS_QTY_VALUE         4007
#define ID_SETTINGS_STOP_VALUE        4008
#define ID_SETTINGS_PROFIT_VALUE      4009
#define ID_SETTINGS_GATEWAY_PATH      4010
#define ID_SETTINGS_GATEWAY_PATH_EDIT 4011
#define ID_SETTINGS_RISK_VALUE        4012

static HWND hDebugEdit = NULL;
static HWND hGatewayEdit = NULL;
static std::vector<TtsVoiceEntry> g_settingsVoices; // populated once on WM_CREATE

// ─── Debug Log ────────────────────────────────────────────────────────────────
void FlushDebugBuffer() {
    if (!hDebugEdit || !IsWindow(hDebugEdit)) return;
    SendMessageA(hDebugEdit, WM_SETTEXT, 0, (LPARAM)""); // clear first
    for (const auto& msg : debugBuffer) {
        int len = GetWindowTextLength(hDebugEdit);
        SendMessage(hDebugEdit, EM_SETSEL, len, len);
        SendMessageA(hDebugEdit, EM_REPLACESEL, FALSE, (LPARAM)msg.c_str());
    }
    // Scroll to bottom
    SendMessage(hDebugEdit, EM_SETSEL, -1, -1);
    SendMessage(hDebugEdit, EM_SCROLLCARET, 0, 0);
}

LRESULT CALLBACK WndProcDebugLog(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE: {
            
            hDebugEdit = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
                WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL |
                ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_READONLY,
                0, 0, 0, 0, hWnd, NULL, GetModuleHandle(NULL), NULL);
            SetPropA(hWnd, "hDebugEdit", hDebugEdit);
            RECT rc;
            GetClientRect(hWnd, &rc);
            SetWindowPos(hDebugEdit, NULL, 0, 0, rc.right, rc.bottom, SWP_NOZORDER);
            FlushDebugBuffer(); // ← show all buffered messages
            break;
        }
        case WM_DESTROY:
            RemovePropA(hWnd, "hDebugEdit");
            break;
        case WM_SIZE: {
            if (hDebugEdit) {
                RECT rc;
                GetClientRect(hWnd, &rc);
                SetWindowPos(hDebugEdit, NULL, 0, 0, rc.right, rc.bottom, SWP_NOZORDER);
            }
            break;
        }
    }
    return HandleCommonMessages(hWnd, message, wParam, lParam);
}

// Call this on every window after creating it
void ApplyDarkModeToAllWindows() {
    // Enumerate all top-level windows owned by this process
    EnumWindows([](HWND hWnd, LPARAM) -> BOOL {
        DWORD pid;
        GetWindowThreadProcessId(hWnd, &pid);
        if (pid == GetCurrentProcessId()) {
            ApplyDarkMode(hWnd);
            InvalidateRect(hWnd, NULL, TRUE); // ← force repaint
            RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_ALLCHILDREN);
        }
        return TRUE;
    }, 0);
}

LRESULT CALLBACK WndProcSettings(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE: {
            HINSTANCE hInst = ((LPCREATESTRUCT)lParam)->hInstance;
            int m  = 8;   // outer margin
            int gm = 14;  // inner margin inside a group box
            int w  = 252; // usable width inside client area
            int gw = w - gm * 2; // control width inside group box
            int y  = m;

            // ── Gateway ──────────────────────────────────────────────────────
            CreateWindowA("BUTTON", "Gateway",
                WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                m, y, w, 130,
                hWnd, NULL, hInst, NULL);

            HWND hChkAutoGtw = CreateWindowA("BUTTON", "Auto-start IBKR Gateway",
                WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                m + gm, y + 18, gw, 22,
                hWnd, (HMENU)ID_SETTINGS_AUTO_GATEWAY, hInst, NULL);
            if (Settings_AutoGateway())
                SendMessage(hChkAutoGtw, BM_SETCHECK, BST_CHECKED, 0);

            HWND hChkKill = CreateWindowA("BUTTON", "Kill IBKR Gateway on exit",
                WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                m + gm, y + 44, gw, 22,
                hWnd, (HMENU)ID_SETTINGS_KILL_GATEWAY, hInst, NULL);
            if (Settings_KillGatewayOnExit())
                SendMessage(hChkKill, BM_SETCHECK, BST_CHECKED, 0);

            CreateWindowA("BUTTON", "Change executable path",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_OWNERDRAW,
                m + gm, y + 70, gw, 22,
                hWnd, (HMENU)ID_SETTINGS_GATEWAY_PATH, hInst, NULL);

            hGatewayEdit = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
                WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_LEFT | ES_READONLY,
                m + gm, y + 98, gw, 20,
                hWnd, (HMENU)ID_SETTINGS_GATEWAY_PATH_EDIT, hInst, NULL);
            SetWindowTextA(hGatewayEdit, GetGatewayPath().c_str());
            y += 138;

            // ── Display ──────────────────────────────────────────────────────
            CreateWindowA("BUTTON", "Display",
                WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                m, y, w, 46,
                hWnd, NULL, hInst, NULL);

            HWND hChkDark = CreateWindowA("BUTTON", "Dark mode",
                WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                m + gm, y + 18, gw, 22,
                hWnd, (HMENU)ID_SETTINGS_DARK_MODE, hInst, NULL);
            if (Settings_DarkMode())
                SendMessage(hChkDark, BM_SETCHECK, BST_CHECKED, 0);
            y += 54;

            // ── Audio ────────────────────────────────────────────────────────
            CreateWindowA("BUTTON", "Audio",
                WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                m, y, w, 78,
                hWnd, NULL, hInst, NULL);

            HWND hChkSounds = CreateWindowA("BUTTON", "Play notification sounds",
                WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                m + gm, y + 18, gw, 22,
                hWnd, (HMENU)ID_SETTINGS_PLAY_SOUNDS, hInst, NULL);
            if (Settings_Load("PlaySounds", 0))
                SendMessage(hChkSounds, BM_SETCHECK, BST_CHECKED, 0);

            // ── TTS Voice selector ───────────────────────────────────────────
            CreateWindowA("STATIC", "Voice:",
                WS_CHILD | WS_VISIBLE,
                m + gm, y + 44, 40, 20,
                hWnd, NULL, hInst, NULL);

            HWND hVoiceCombo = CreateWindowA("COMBOBOX", "",
                WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
                m + gm + 44, y + 42, gw - 44, 200,
                hWnd, (HMENU)ID_SETTINGS_VOICE_COMBO, hInst, NULL);

            // Enumerate all system voices and fill the combo
            g_settingsVoices = TTS_EnumerateVoices();
            std::string savedTokenA = Settings_LoadTtsVoice();
            std::wstring savedToken(savedTokenA.begin(), savedTokenA.end());

            int selectIdx = -1;      // index to pre-select
            int herenaIdx = -1;      // fallback: first Herena/Catalan entry

            for (int i = 0; i < (int)g_settingsVoices.size(); ++i) {
                const auto& v = g_settingsVoices[i];
                SendMessageW(hVoiceCombo, CB_ADDSTRING, 0, (LPARAM)v.display.c_str());

                if (!savedToken.empty() && v.tokenId == savedToken)
                    selectIdx = i;

                if (herenaIdx < 0 &&
                    (TTS_ContainsCI(v.tokenId.c_str(),  L"herena") ||
                    TTS_ContainsCI(v.display.c_str(),  L"herena")  ||
                    TTS_ContainsCI(v.tokenId.c_str(),  L"helena")  ||
                    TTS_ContainsCI(v.display.c_str(),  L"helena")  ||
                    TTS_ContainsCI(v.tokenId.c_str(),  L"ca-es")   ||
                    TTS_ContainsCI(v.display.c_str(),  L"ca-es")   ||
                    TTS_ContainsCI(v.tokenId.c_str(),  L"catalan") ||
                    TTS_ContainsCI(v.display.c_str(),  L"catalan")))
                    herenaIdx = i;
            }

            // If nothing saved yet, default to first Herena-Catalan found
            if (selectIdx < 0) selectIdx = (herenaIdx >= 0) ? herenaIdx : 0;
            if (!g_settingsVoices.empty())
                SendMessage(hVoiceCombo, CB_SETCURSEL, selectIdx, 0);
            y += 86;

            // ── Trading ──────────────────────────────────────────────────────
            CreateWindowA("BUTTON", "Trading",
                WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                m, y, w, 148,
                hWnd, NULL, hInst, NULL);

            // Row helper: label + edit
            auto MakeRow = [&](const char* label, UINT id, int rowY, bool isInt) -> HWND {
                CreateWindowA("STATIC", label,
                    WS_CHILD | WS_VISIBLE,
                    m + gm, y + rowY, 72, 20,
                    hWnd, NULL, hInst, NULL);
                DWORD numStyle = isInt ? ES_NUMBER : 0;
                return CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
                    WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_CENTER | numStyle,
                    m + gm + 76, y + rowY - 3, 80, 26,
                    hWnd, (HMENU)(UINT_PTR)id, hInst, NULL);
            };

            HWND hQtyEdit    = MakeRow("Order Qty:",  ID_SETTINGS_QTY_VALUE,    18, true);
            HWND hStopEdit   = MakeRow("Stop:",       ID_SETTINGS_STOP_VALUE,   51, false);
            HWND hProfitEdit = MakeRow("Profit:",     ID_SETTINGS_PROFIT_VALUE, 84, false);
            HWND hRiskEdit   = MakeRow("Risk %:",     ID_SETTINGS_RISK_VALUE,  117, false);

            SetWindowTextA(hQtyEdit,    std::format("{}",    (int)Settings_Load("OrderQty", 100)).c_str());
            SetWindowTextA(hStopEdit,   std::format("{:.2f}", Settings_LoadFloat("StopPrice",  1.0f)).c_str());
            SetWindowTextA(hProfitEdit, std::format("{:.2f}", Settings_LoadFloat("ProfitPrice", 2.0f)).c_str());
            SetWindowTextA(hRiskEdit,   std::format("{:.2f}", Settings_LoadFloat("RiskPct",    1.0f)).c_str());
            y += 156;

            // ── Debug ────────────────────────────────────────────────────────
            CreateWindowA("BUTTON", "Debug",
                WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                m, y, w, 46,
                hWnd, NULL, hInst, NULL);

            CreateWindowA("BUTTON", "View Log",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_OWNERDRAW,
                m + gm, y + 16, gw, 22,
                hWnd, (HMENU)ID_SETTINGS_DEBUG_LOG, hInst, NULL);
            break;
        }

        case WM_DESTROY:
            hDebugEdit = NULL;
            break;

        case WM_COMMAND:
            if (LOWORD(wParam) == ID_SETTINGS_PLAY_SOUNDS) {
                HWND hChk = GetDlgItem(hWnd, ID_SETTINGS_PLAY_SOUNDS);
                DWORD checked = (SendMessage(hChk, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
                Settings_Save("PlaySounds", checked);
            }
            if (LOWORD(wParam) == ID_SETTINGS_AUTO_GATEWAY) {
                HWND hChk = GetDlgItem(hWnd, ID_SETTINGS_AUTO_GATEWAY);
                DWORD checked = (SendMessage(hChk, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
                Settings_Save("Gateway_AutoStart", checked);
            }
            if (LOWORD(wParam) == ID_SETTINGS_DEBUG_LOG) {
                StartDebugLog();
                FlushDebugBuffer();
            }
            if (LOWORD(wParam) == ID_SETTINGS_KILL_GATEWAY) {
                HWND hChk = GetDlgItem(hWnd, ID_SETTINGS_KILL_GATEWAY);
                DWORD checked = (SendMessage(hChk, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
                Settings_Save("Gateway_KillOnExit", checked);
            }
            if (LOWORD(wParam) == ID_SETTINGS_DARK_MODE) {
                HWND hChk = GetDlgItem(hWnd, ID_SETTINGS_DARK_MODE);
                DWORD checked = (SendMessage(hChk, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
                Settings_Save("DarkMode", checked);
                ApplyDarkModeToAllWindows();
                // Force full repaint of this window and all children
                InvalidateRect(hWnd, NULL, TRUE);
                RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_UPDATENOW);
            }
            if (LOWORD(wParam) == ID_SETTINGS_QTY_VALUE) {
                HWND hEdit = GetDlgItem(hWnd, ID_SETTINGS_QTY_VALUE);
                int len = GetWindowTextLength(hEdit);
                int qty = 0;
                if (len > 0) {
                    char buf[len + 1];
                    GetWindowTextA(hEdit, buf, len + 1);
                    qty = atoi(buf);
                }
                Settings_Save("OrderQty", qty);
            }
            if (LOWORD(wParam) == ID_SETTINGS_STOP_VALUE) {
                HWND hEdit = GetDlgItem(hWnd, ID_SETTINGS_STOP_VALUE);
                int len = GetWindowTextLength(hEdit);
                float price = 1.0f;
                if (len > 0) {
                    char buf[len + 1];
                    GetWindowTextA(hEdit, buf, len + 1);
                    price = (float)atof(buf); // atof handles decimals
                }
                Settings_SaveFloat("StopPrice", price);
            }
            if (LOWORD(wParam) == ID_SETTINGS_PROFIT_VALUE) {
                HWND hEdit = GetDlgItem(hWnd, ID_SETTINGS_PROFIT_VALUE);
                int len = GetWindowTextLength(hEdit);
                float price = 2.0f;
                if (len > 0) {
                    char buf[len + 1];
                    GetWindowTextA(hEdit, buf, len + 1);
                    price = (float)atof(buf); // atof handles decimals
                }
                Settings_SaveFloat("ProfitPrice", price);
            }
            if (LOWORD(wParam) == ID_SETTINGS_RISK_VALUE) {
                HWND hEdit = GetDlgItem(hWnd, ID_SETTINGS_RISK_VALUE);
                int len = GetWindowTextLength(hEdit);
                float pct = 1.0f;
                if (len > 0) {
                    char buf[len + 1];
                    GetWindowTextA(hEdit, buf, len + 1);
                    pct = (float)atof(buf); // atof handles decimals
                }
                Settings_SaveFloat("RiskPct", pct);
            }
            if (LOWORD(wParam) == ID_SETTINGS_VOICE_COMBO && HIWORD(wParam) == CBN_SELCHANGE) {
                HWND hCombo = GetDlgItem(hWnd, ID_SETTINGS_VOICE_COMBO);
                int idx = (int)SendMessage(hCombo, CB_GETCURSEL, 0, 0);
                if (idx >= 0 && idx < (int)g_settingsVoices.size()) {
                    const std::wstring& tid = g_settingsVoices[idx].tokenId;
                    std::string tidA(tid.begin(), tid.end());
                    Settings_SaveTtsVoice(tidA);
                    // Notify all open windows to hot-swap to the new voice immediately
                    DWORD pid = GetCurrentProcessId();
                    EnumWindows([](HWND hw, LPARAM) -> BOOL {
                        DWORD wpid;
                        GetWindowThreadProcessId(hw, &wpid);
                        if (wpid == GetCurrentProcessId())
                            PostMessage(hw, WM_TTS_VOICE_CHANGED, 0, 0);
                        return TRUE;
                    }, 0);
                }
            }
            if (LOWORD(wParam) == ID_SETTINGS_GATEWAY_PATH) {
                HWND hPathEdit = GetDlgItem(hWnd, ID_SETTINGS_GATEWAY_PATH_EDIT);   
                if (hPathEdit != NULL) {
                    std::string path = AskGatewayPath(hWnd);
                    if (!path.empty()) {
                        SaveGatewayPath(path);
                        SetWindowTextA(hPathEdit, path.c_str());
                    }
                }
            }
            break;
    }
    return HandleCommonMessages(hWnd, message, wParam, lParam);
}
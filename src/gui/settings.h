#pragma once

void StartSettings() { StartGenericWindow(SETTINGS_CLASS_NAME, "Settings", L"IBKRGatewayClient.Settings", 276, 275); }

void StartDebugLog() { StartGenericWindow(DEBUGLOG_CLASS_NAME, "Debug Log", L"IBKRGatewayClient.DebugLog", 790, 240); }

#define ID_SETTINGS_KILL_GATEWAY 4001
#define ID_SETTINGS_DARK_MODE    4002
#define ID_SETTINGS_PLAY_SOUNDS  4003
#define ID_SETTINGS_AUTO_GATEWAY 4004
#define ID_SETTINGS_DEBUG_LOG    4005
#define ID_SETTINGS_VOICE_COMBO  4006

static HWND hDebugEdit = NULL;
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
            int margin = 8;

            HWND hChkAutoGtw = CreateWindowA("BUTTON", "Auto-start IBKR Gateway",
                WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                margin, margin, 250, 24,
                hWnd, (HMENU)ID_SETTINGS_AUTO_GATEWAY, hInst, NULL);
            if (Settings_AutoGateway())
                SendMessage(hChkAutoGtw, BM_SETCHECK, BST_CHECKED, 0);

            HWND hChkKill = CreateWindowA("BUTTON", "Kill IBKR Gateway on exit",
                WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                margin, margin + 32, 250, 24,
                hWnd, (HMENU)ID_SETTINGS_KILL_GATEWAY, hInst, NULL);
            if (Settings_KillGatewayOnExit())
                SendMessage(hChkKill, BM_SETCHECK, BST_CHECKED, 0);

            HWND hChkDark = CreateWindowA("BUTTON", "Dark mode",
                WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                margin, margin + 64, 250, 24,
                hWnd, (HMENU)ID_SETTINGS_DARK_MODE, hInst, NULL);
            if (Settings_DarkMode())
                SendMessage(hChkDark, BM_SETCHECK, BST_CHECKED, 0);

            HWND hChkSounds = CreateWindowA("BUTTON", "Play notification sounds",
                WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                margin, margin + 96, 250, 24,
                hWnd, (HMENU)ID_SETTINGS_PLAY_SOUNDS, hInst, NULL);
            if (Settings_Load("PlaySounds", 0))
                SendMessage(hChkSounds, BM_SETCHECK, BST_CHECKED, 0);

            // ── TTS Voice selector ────────────────────────────────────────────
            CreateWindowA("STATIC", "Voice:",
                WS_CHILD | WS_VISIBLE,
                margin, margin + 128, 44, 20,
                hWnd, NULL, hInst, NULL);

            HWND hVoiceCombo = CreateWindowA("COMBOBOX", "",
                WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
                margin + 48, margin + 126, 202, 200,
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
                     TTS_ContainsCI(v.display.c_str(),  L"herena") ||
                     TTS_ContainsCI(v.tokenId.c_str(),  L"ca-es")  ||
                     TTS_ContainsCI(v.display.c_str(),  L"catalan")))
                    herenaIdx = i;
            }

            // If nothing saved yet, default to first Herena-Catalan found
            if (selectIdx < 0) selectIdx = (herenaIdx >= 0) ? herenaIdx : 0;
            if (!g_settingsVoices.empty())
                SendMessage(hVoiceCombo, CB_SETCURSEL, selectIdx, 0);

            HWND hBtnDebug = CreateWindowA("BUTTON", "Debug Log",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_OWNERDRAW,
                margin, margin + 164, 250, 24,
                hWnd, (HMENU)ID_SETTINGS_DEBUG_LOG, hInst, NULL);

            ApplyDarkMode(hWnd);
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
            break;
    }
    return HandleCommonMessages(hWnd, message, wParam, lParam);
}
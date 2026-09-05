#pragma once

// Layout: two columns. Column 1 holds Gateway/Display/Audio; column 2 holds Trading/Configuration.
// Client area is 560px wide: two 252px fieldsets with a 16px gutter between them (252 + 16 + 252 = 560).
// The outer window (title bar + borders) adds ~36px horizontally and ~30px vertically, giving a
// total size of 596x713.
int WindowSettingsWidth = 543;
int WindowSettingsHeight = 405;

void StartSettings() { StartGenericWindow(SETTINGS_CLASS_NAME, "Settings", L"TWSAPIClientTradingFloor.Settings", WindowSettingsWidth, WindowSettingsHeight); }

void StartDebugLog() { StartGenericWindow(DEBUGLOG_CLASS_NAME, "Debug Log", L"TWSAPIClientTradingFloor.DebugLog", 790, 243); }

#define ID_SETTINGS_KILL_GATEWAY       4001
#define ID_SETTINGS_DARK_MODE          4002
#define ID_SETTINGS_PLAY_SOUNDS        4003
#define ID_SETTINGS_AUTO_GATEWAY       4004
#define ID_SETTINGS_DEBUG_LOG          4005
#define ID_SETTINGS_VOICE_COMBO        4006
#define ID_SETTINGS_QTY_VALUE          4007
#define ID_SETTINGS_STOP_VALUE         4008
#define ID_SETTINGS_PROFIT_VALUE       4009
#define ID_SETTINGS_GATEWAY_PATH       4010
#define ID_SETTINGS_GATEWAY_PATH_EDIT  4011
#define ID_SETTINGS_RISK_VALUE         4012
#define ID_SETTINGS_SAFETY_VALUE       4013
#define ID_SETTINGS_GROUP_ID           4014
#define ID_SETTINGS_CLIENT_ID          4015
#define ID_SETTINGS_BACKUP_RESTORE     4016
#define ID_SETTINGS_BACKUP_DOWNLOAD    4017
#define ID_SETTINGS_FULL_SCREEN_ALERTS 4018
#define ID_SETTINGS_USERNAME           4019
#define ID_SETTINGS_PASSWORD           4020
#define ID_SETTINGS_LOCK               4021

static HWND hSettingBox1 = NULL;
static HWND hSettingBox2 = NULL;
static HWND hSettingBox3 = NULL;
static HWND hSettingBox4 = NULL;
static HWND hSettingBox5 = NULL;
static HWND hDebugEdit = NULL;
static HWND hGatewayEdit = NULL;
static std::vector<TtsVoiceEntry> settingsVoices; // populated once on WM_CREATE

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
            hDebugEdit = NULL;   // ← avoid a stale handle lingering after the window is destroyed
            break;
        case WM_SIZE: {
            if (hDebugEdit) {
                int w = LOWORD(lParam);
                int h = HIWORD(lParam);
                SetWindowPos(hDebugEdit, NULL, 0, 0, w, h, SWP_NOZORDER | SWP_NOACTIVATE);
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

// Add near the top of settings.h, e.g. just above WndProcSettings:
static void Settings_SaveCredentialsFromUI(HWND hWnd) {
    char userBuf[256] = {}, passBuf[256] = {};
    GetWindowTextA(GetDlgItem(hWnd, ID_SETTINGS_USERNAME), userBuf, sizeof(userBuf));
    GetWindowTextA(GetDlgItem(hWnd, ID_SETTINGS_PASSWORD), passBuf, sizeof(passBuf));
    Credentials_Save(userBuf, passBuf);
}

LRESULT CALLBACK WndProcSettings(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE: {
            HINSTANCE hInst = ((LPCREATESTRUCT)lParam)->hInstance;

            int m  = 8;   // outer margin
            int gm = 14;  // inner margin inside a group box
            int w  = 252; // usable width of one column (fieldset)
            int gw = w - gm * 2; // control width inside group box
            int col2_x = m + w + 16; // x origin of the second column (fieldset + gutter)
            int y  = m;

            // ── Gateway ──────────────────────────────────────────────────────
            hSettingBox1 = CreateWindowA("BUTTON", "Gateway:",
                WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                m, y, w, 250,
                hWnd, NULL, hInst, NULL);
            SetWindowSubclass(hSettingBox1, DarkGroupBoxSubclassProc, 1, 0);
            SendMessage(hSettingBox1, WM_SETFONT, (WPARAM)hFont11pt.get(), TRUE);

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

            CreateWindowA("STATIC", "Username:",
                WS_CHILD | WS_VISIBLE,
                m + gm, y + 128, 72, 20,
                hWnd, NULL, hInst, NULL);
            HWND hUsernameEdit = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
                WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_CENTER,
                m + gm + 76, y + 125, 147, 26,
                hWnd, (HMENU)ID_SETTINGS_USERNAME, hInst, NULL);
            
            CreateWindowA("STATIC", "Password:",
                WS_CHILD | WS_VISIBLE,
                m + gm, y + 158, 72, 20,
                hWnd, NULL, hInst, NULL);
            HWND hPasswordEdit = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
                WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_CENTER | ES_PASSWORD,
                m + gm + 76, y + 155, 147, 26,
                hWnd, (HMENU)ID_SETTINGS_PASSWORD, hInst, NULL);

            // ── Populate from Windows Credential Manager (if previously saved) ────────
            {
                std::string savedUsername, savedPassword;
                if (Credentials_Load(savedUsername, savedPassword)) {
                    SetWindowTextA(hUsernameEdit, savedUsername.c_str());
                    SetWindowTextA(hPasswordEdit, savedPassword.c_str());
                }
            }

            // Client ID — passed as the second parameter to api().connect().
            CreateWindowA("STATIC", "Client ID:",
                WS_CHILD | WS_VISIBLE,
                m + gm, y + 188, 72, 20,
                hWnd, NULL, hInst, NULL);
            HWND hClientIdEdit = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
                WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER,
                m + gm + 76, y + 185, 80, 26,
                hWnd, (HMENU)ID_SETTINGS_CLIENT_ID, hInst, NULL);
            SetWindowTextA(hClientIdEdit, std::format("{}", (int)Settings_Load("ClientId", 0)).c_str());

            // Group ID — TWS "linked window" group color id, used for
            // subscribeToGroupEvents()/updateDisplayGroup().
            CreateWindowA("STATIC", "Group ID:",
                WS_CHILD | WS_VISIBLE,
                m + gm, y + 218, 72, 20,
                hWnd, NULL, hInst, NULL);
            HWND hGroupIdEdit = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
                WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER,
                m + gm + 76, y + 215, 80, 26,
                hWnd, (HMENU)ID_SETTINGS_GROUP_ID, hInst, NULL);
            SetWindowTextA(hGroupIdEdit, std::format("{}", (int)Settings_Load("GroupId", 4)).c_str());

            y += 258;

            // ── Display ──────────────────────────────────────────────────────
            hSettingBox2 = CreateWindowA("BUTTON", "Display:",
                WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                m, y, w, 14 + (30 * 3),
                hWnd, NULL, hInst, NULL);
            SetWindowSubclass(hSettingBox2, DarkGroupBoxSubclassProc, 1, 0);
            SendMessage(hSettingBox2, WM_SETFONT, (WPARAM)hFont11pt.get(), TRUE);

            HWND hChkDark = CreateWindowA("BUTTON", "Dark mode",
                WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                m + gm, y + 18, gw, 22,
                hWnd, (HMENU)ID_SETTINGS_DARK_MODE, hInst, NULL);
            if (darkMode)
                SendMessage(hChkDark, BM_SETCHECK, BST_CHECKED, 0);

            HWND hChkAlerts = CreateWindowA("BUTTON", "Full screen alerts",
                WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                m + gm, y + 44, gw, 22,
                hWnd, (HMENU)ID_SETTINGS_FULL_SCREEN_ALERTS, hInst, NULL);
            if (Settings_Load("FullScreenAlerts", 0))
                SendMessage(hChkAlerts, BM_SETCHECK, BST_CHECKED, 0);
                
            CreateWindowA("STATIC", "Lock:",
                WS_CHILD | WS_VISIBLE,
                m + gm, y + 70, 72, 20,
                hWnd, NULL, hInst, NULL);
            HWND hLockEdit = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
                WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_CENTER | ES_PASSWORD,
                m + gm + 76, y + 67, 147, 26,
                hWnd, (HMENU)ID_SETTINGS_LOCK, hInst, NULL);
            SetWindowTextA(hLockEdit, Settings_LoadString("Lock", "").c_str());
            //y += 114;
            y = m;

            // ── Trading ────────────────────────────────────────────────────── (column 2)
            hSettingBox4 = CreateWindowA("BUTTON", "Trading:",
                WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                col2_x, y, w, 37 * 5,
                hWnd, NULL, hInst, NULL);
            SetWindowSubclass(hSettingBox4, DarkGroupBoxSubclassProc, 1, 0);
            SendMessage(hSettingBox4, WM_SETFONT, (WPARAM)hFont11pt.get(), TRUE);

            // Row helper: label + edit
            auto MakeRow = [&](const char* label, UINT id, int rowY, bool isInt) -> HWND {
                CreateWindowA("STATIC", label,
                    WS_CHILD | WS_VISIBLE,
                    col2_x + gm, y + rowY, 72, 20,
                    hWnd, NULL, hInst, NULL);
                DWORD numStyle = isInt ? ES_NUMBER : 0;
                return CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
                    WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_CENTER | numStyle,
                    col2_x + gm + 76, y + rowY - 3, 80, 26,
                    hWnd, (HMENU)(UINT_PTR)id, hInst, NULL);
            };

            HWND hQtyEdit    = MakeRow("Quantity:",   ID_SETTINGS_QTY_VALUE,     20, true);
            HWND hStopEdit   = MakeRow("Stop:",       ID_SETTINGS_STOP_VALUE,    53, false);
            HWND hProfitEdit = MakeRow("Profit:",     ID_SETTINGS_PROFIT_VALUE,  86, false);
            HWND hRiskEdit   = MakeRow("Risk %:",     ID_SETTINGS_RISK_VALUE,   119, false);
            HWND hSafetyEdit = MakeRow("Safety:",     ID_SETTINGS_SAFETY_VALUE, 152, false);
            y += 152 + 40;

            SetWindowTextA(hQtyEdit,    std::format("{}",    (int)Settings_Load("OrderQty", 20)).c_str());
            SetWindowTextA(hStopEdit,   std::format("{:.2f}", Settings_LoadFloat("StopPrice",  1.0f)).c_str());
            SetWindowTextA(hProfitEdit, std::format("{:.2f}", Settings_LoadFloat("ProfitPrice", 2.0f)).c_str());
            SetWindowTextA(hRiskEdit,   std::format("{:.2f}", Settings_LoadFloat("RiskPct",    1.0f)).c_str());
            SetWindowTextA(hSafetyEdit, std::format("{:.2f}", Settings_LoadFloat("Safety",      2.0f)).c_str());

            
            // ── Audio ────────────────────────────────────────────────────────
            hSettingBox3 = CreateWindowA("BUTTON", "Audio:",
                WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                col2_x, y, w, 14 + (30 * 2),
                hWnd, NULL, hInst, NULL);
            SetWindowSubclass(hSettingBox3, DarkGroupBoxSubclassProc, 1, 0);
            SendMessage(hSettingBox3, WM_SETFONT, (WPARAM)hFont11pt.get(), TRUE);

            HWND hChkSounds = CreateWindowA("BUTTON", "Play notification sounds",
                WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                col2_x + gm, y + 18, gw, 22,
                hWnd, (HMENU)ID_SETTINGS_PLAY_SOUNDS, hInst, NULL);
            if (Settings_Load("PlaySounds", 0))
                SendMessage(hChkSounds, BM_SETCHECK, BST_CHECKED, 0);

            // ── TTS Voice selector ───────────────────────────────────────────
            CreateWindowA("STATIC", "Voice:",
                WS_CHILD | WS_VISIBLE,
                col2_x + gm, y + 44, 40, 20,
                hWnd, NULL, hInst, NULL);

            HWND hVoiceCombo = CreateWindowA("COMBOBOX", "",
                WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
                col2_x + gm + 44, y + 42, gw - 44, 200,
                hWnd, (HMENU)ID_SETTINGS_VOICE_COMBO, hInst, NULL);

            // Enumerate all system voices and fill the combo
            settingsVoices = TTS_EnumerateVoices();
            std::string savedTokenA = Settings_LoadTtsVoice();
            std::wstring savedToken(savedTokenA.begin(), savedTokenA.end());

            int selectIdx = -1;      // index to pre-select
            int herenaIdx = -1;      // fallback: first Herena/Catalan entry

            for (int i = 0; i < (int)settingsVoices.size(); ++i) {
                const auto& v = settingsVoices[i];
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
            if (!settingsVoices.empty())
                SendMessage(hVoiceCombo, CB_SETCURSEL, selectIdx, 0);

            y += 82;

            // ── System Tools ─────────────────────────────────────────── (column 2)
            hSettingBox5 = CreateWindowA("BUTTON", "Configuration:",
                WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                col2_x, y, w, 78,
                hWnd, NULL, hInst, NULL);
            SetWindowSubclass(hSettingBox5, DarkGroupBoxSubclassProc, 1, 0);
            SendMessage(hSettingBox5, WM_SETFONT, (WPARAM)hFont11pt.get(), TRUE);

            // Two buttons share the row: Backup (left) and Restore (right), with a 4px gutter.
            int btn = (gw - 4) / 2; // width of each button
            CreateWindowA("BUTTON", "Backup",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_OWNERDRAW,
                col2_x + gm, y + 20, btn, 22,
                hWnd, (HMENU)ID_SETTINGS_BACKUP_DOWNLOAD, hInst, NULL);

            CreateWindowA("BUTTON", "Restore",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_OWNERDRAW,
                col2_x + gm + btn + 4, y + 20, btn, 22,
                hWnd, (HMENU)ID_SETTINGS_BACKUP_RESTORE, hInst, NULL);

            CreateWindowA("BUTTON", "Debug Log",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_OWNERDRAW,
                col2_x + gm, y + 48, gw, 22,
                hWnd, (HMENU)ID_SETTINGS_DEBUG_LOG, hInst, NULL);

            break;
        }

        case WM_DESTROY:
            hSettingBox1 = hSettingBox2 = hSettingBox3 = hSettingBox4 = hSettingBox5 = hDebugEdit = NULL;
            break;

        case WM_COMMAND:
            if (LOWORD(wParam) == ID_SETTINGS_AUTO_GATEWAY) {
                HWND hChk = GetDlgItem(hWnd, ID_SETTINGS_AUTO_GATEWAY);
                DWORD checked = (SendMessage(hChk, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
                Settings_Save("Gateway_AutoStart", checked);
            }
            if (LOWORD(wParam) == ID_SETTINGS_KILL_GATEWAY) {
                HWND hChk = GetDlgItem(hWnd, ID_SETTINGS_KILL_GATEWAY);
                DWORD checked = (SendMessage(hChk, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
                Settings_Save("Gateway_KillOnExit", checked);
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
            if (LOWORD(wParam) == ID_SETTINGS_GROUP_ID) {
                HWND hEdit = GetDlgItem(hWnd, ID_SETTINGS_GROUP_ID);
                int len = GetWindowTextLength(hEdit);
                int groupId = 4;
                if (len > 0) {
                    char buf[len + 1];
                    GetWindowTextA(hEdit, buf, len + 1);
                    groupId = atoi(buf);
                }
                Settings_Save("GroupId", groupId);
            }
            if (LOWORD(wParam) == ID_SETTINGS_CLIENT_ID) {
                HWND hEdit = GetDlgItem(hWnd, ID_SETTINGS_CLIENT_ID);
                int len = GetWindowTextLength(hEdit);
                int clientId = 0;
                if (len > 0) {
                    char buf[len + 1];
                    GetWindowTextA(hEdit, buf, len + 1);
                    clientId = atoi(buf);
                }
                Settings_Save("ClientId", clientId);
            }
            if (LOWORD(wParam) == ID_SETTINGS_PLAY_SOUNDS) {
                HWND hChk = GetDlgItem(hWnd, ID_SETTINGS_PLAY_SOUNDS);
                DWORD checked = (SendMessage(hChk, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
                Settings_Save("PlaySounds", checked);
            }
            if (LOWORD(wParam) == ID_SETTINGS_DEBUG_LOG) {
                StartDebugLog();
                FlushDebugBuffer();
            }
            if ((LOWORD(wParam) == ID_SETTINGS_PASSWORD || LOWORD(wParam) == ID_SETTINGS_USERNAME) && HIWORD(wParam) == EN_CHANGE) {
                Settings_SaveCredentialsFromUI(hWnd);
            }
            if ((LOWORD(wParam) == ID_SETTINGS_LOCK)) {
                HWND hEdit = GetDlgItem(hWnd, ID_SETTINGS_LOCK);
                int len = GetWindowTextLength(hEdit);
                std::string lock = "";
                if (len > 0) {
                    char buf[len + 1];
                    GetWindowTextA(hEdit, buf, len + 1);
                    lock = std::string(buf);
                }
                Settings_SaveString("Lock", lock);
            }
            if (LOWORD(wParam) == ID_SETTINGS_FULL_SCREEN_ALERTS) {
                HWND hChk = GetDlgItem(hWnd, ID_SETTINGS_FULL_SCREEN_ALERTS);
                DWORD checked = (SendMessage(hChk, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
                Settings_Save("FullScreenAlerts", checked);
            }
            if (LOWORD(wParam) == ID_SETTINGS_DARK_MODE) {
                HWND hChk = GetDlgItem(hWnd, ID_SETTINGS_DARK_MODE);
                DWORD checked = (SendMessage(hChk, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
                Settings_Save("DarkMode", checked);
                darkMode = (checked == 1);
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
            if (LOWORD(wParam) == ID_SETTINGS_SAFETY_VALUE) {
                HWND hEdit = GetDlgItem(hWnd, ID_SETTINGS_SAFETY_VALUE);
                int len = GetWindowTextLength(hEdit);
                float safety = 2.0f;
                if (len > 0) {
                    char buf[len + 1];
                    GetWindowTextA(hEdit, buf, len + 1);
                    safety = (float)atof(buf); // atof handles decimals
                }
                Settings_SaveFloat("Safety", safety);
            }
            if (LOWORD(wParam) == ID_SETTINGS_VOICE_COMBO && HIWORD(wParam) == CBN_SELCHANGE) {
                HWND hCombo = GetDlgItem(hWnd, ID_SETTINGS_VOICE_COMBO);
                int idx = (int)SendMessage(hCombo, CB_GETCURSEL, 0, 0);
                if (idx >= 0 && idx < (int)settingsVoices.size()) {
                    const std::wstring& tid = settingsVoices[idx].tokenId;
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
            if (LOWORD(wParam) == ID_SETTINGS_BACKUP_DOWNLOAD) {
                OPENFILENAMEA ofn;
                char szFile[MAX_PATH] = "Trading-Floor_Settings_Backup.reg";
                ZeroMemory(&ofn, sizeof(ofn));
                ofn.lStructSize = sizeof(ofn);
                ofn.hwndOwner = hWnd;
                ofn.lpstrFile = szFile;
                ofn.nMaxFile = sizeof(szFile);
                ofn.lpstrFilter = "Registry Files (*.reg)\0*.reg\0All Files (*.*)\0*.*\0";
                ofn.lpstrDefExt = "reg";
                ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

                if (GetSaveFileNameA(&ofn)) {
                    // Tell reg.exe to export the root registry path for this app.
                    // /y suppresses the overwrite confirmation since GetSaveFileName already handles it.
                    std::string cmd = std::format("reg.exe export \"HKCU\\{}\" \"{}\" /y", APP_REG_ROOT, szFile);
                    
                    STARTUPINFOA si = { sizeof(si) };
                    si.dwFlags = STARTF_USESHOWWINDOW;
                    si.wShowWindow = SW_HIDE; // Hide the cmd window popup
                    PROCESS_INFORMATION pi = { 0 };
                    
                    char cmdBuf[1024];
                    strcpy_s(cmdBuf, sizeof(cmdBuf), cmd.c_str());
                    
                    if (CreateProcessA(NULL, cmdBuf, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
                        WaitForSingleObject(pi.hProcess, INFINITE);
                        CloseHandle(pi.hProcess);
                        CloseHandle(pi.hThread);
                        MessageBoxA(hWnd, "Settings backed up successfully.", "Backup", MB_OK | MB_ICONINFORMATION);
                    } else {
                        MessageBoxA(hWnd, "Failed to create backup.", "Error", MB_OK | MB_ICONERROR);
                    }
                }
            }

            if (LOWORD(wParam) == ID_SETTINGS_BACKUP_RESTORE) {
                OPENFILENAMEA ofn;
                char szFile[MAX_PATH] = { 0 };
                ZeroMemory(&ofn, sizeof(ofn));
                ofn.lStructSize = sizeof(ofn);
                ofn.hwndOwner = hWnd;
                ofn.lpstrFile = szFile;
                ofn.nMaxFile = sizeof(szFile);
                ofn.lpstrFilter = "Registry Files (*.reg)\0*.reg\0All Files (*.*)\0*.*\0";
                ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

                if (GetOpenFileNameA(&ofn)) {
                    // Tell reg.exe to import the selected .reg file back into the registry.
                    std::string cmd = std::format("reg.exe import \"{}\"", szFile);
                    
                    STARTUPINFOA si = { sizeof(si) };
                    si.dwFlags = STARTF_USESHOWWINDOW;
                    si.wShowWindow = SW_HIDE;
                    PROCESS_INFORMATION pi = { 0 };
                    
                    char cmdBuf[1024];
                    strcpy_s(cmdBuf, sizeof(cmdBuf), cmd.c_str());
                    
                    if (CreateProcessA(NULL, cmdBuf, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
                        WaitForSingleObject(pi.hProcess, INFINITE);
                        CloseHandle(pi.hProcess);
                        CloseHandle(pi.hThread);
                        MessageBoxA(hWnd, "Settings restored successfully. Please restart the application to apply all changes.", "Restore", MB_OK | MB_ICONINFORMATION);
                    } else {
                        MessageBoxA(hWnd, "Failed to restore backup.", "Error", MB_OK | MB_ICONERROR);
                    }
                }
            }
            break;
    }
    return HandleCommonMessages(hWnd, message, wParam, lParam);
}
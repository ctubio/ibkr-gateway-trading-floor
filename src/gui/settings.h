#pragma once

static const int WindowSettingsWidth = 543;
static const int WindowSettingsHeight = 430;

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
#define ID_SETTINGS_ADDRESS            4022

static std::vector<TtsVoiceEntry> settingsVoices; // populated once on WM_CREATE

// ─── Debug Log ────────────────────────────────────────────────────────────────
void FlushDebugBuffer() {
    if (!hDebugEdit || !IsWindow(hDebugEdit)) return;

    // Copy out under lock, debugBuffer can be mutated concurrently by
    // LogDebug() from other threads (see registry.h).
    std::vector<std::string> snapshot;
    {
        std::lock_guard<std::mutex> lock(debugBufferMutex);
        snapshot.assign(debugBuffer.begin(), debugBuffer.end());
    }

    SendMessageA(hDebugEdit, WM_SETTEXT, 0, (LPARAM)""); // clear first
    for (const auto& msg : snapshot) {
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
            RECT rc;
            GetClientRect(hWnd, &rc);
            SetWindowPos(hDebugEdit, NULL, 0, 0, rc.right, rc.bottom, SWP_NOZORDER);
            FlushDebugBuffer(); // ← show all buffered messages
            break;
        }
        case WM_DESTROY:
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
        // ── Cross-thread append (see LogDebug in registry.h) ───────────────────
        // Runs on this window's own (UI) thread, so it's the only code path
        // that's allowed to touch hDebugEdit directly.
        case WM_DEBUG_LOG_APPEND: {
            FlushDebugBuffer();
            return 0;
        }
    }
    return HandleCommonMessages(hWnd, message, wParam, lParam);
}

// Add near the top of settings.h, e.g. just above WndProcSettings:
static void Settings_SaveCredentialsFromUI(HWND hWnd) {
    std::string username = EditText(GetDlgItem(hWnd, ID_SETTINGS_USERNAME));
    std::string password = EditText(GetDlgItem(hWnd, ID_SETTINGS_PASSWORD));
    Credentials_Save(username, password);
}

static void SaveCheck(HWND hWnd, int id, const char* key, bool& value) {
    value = SendDlgItemMessage(hWnd, id, BM_GETCHECK, 0, 0) == BST_CHECKED;
    RegSetDword(SETTINGS_CLASS_NAME, key, value);
}

static const struct {
    int id;
    const char* key;
    bool* value;
} settingsChecks[] = {
    {ID_SETTINGS_AUTO_GATEWAY, "Gateway_AutoStart", &autoGateway},
    {ID_SETTINGS_KILL_GATEWAY, "Gateway_KillOnExit", &killGatewayOnExit},
    {ID_SETTINGS_PLAY_SOUNDS, "PlaySounds", &playSounds},
    {ID_SETTINGS_FULL_SCREEN_ALERTS, "FullScreenAlerts", &fullScreenAlerts},
};

enum class NumericSettingKind {
    Integer,
    Float,
};

struct SettingsNumericEdit {
    int id;
    const char* key;
    NumericSettingKind kind;
    void* value;
};

static const SettingsNumericEdit settingsNumericEdits[] = {
    {ID_SETTINGS_GROUP_ID, "GroupId", NumericSettingKind::Integer, &groupIdGateway},
    {ID_SETTINGS_CLIENT_ID, "ClientId", NumericSettingKind::Integer, &clientIdGateway},
    {ID_SETTINGS_QTY_VALUE, "OrderQty", NumericSettingKind::Integer, &qtyGateway},
    {ID_SETTINGS_STOP_VALUE, "StopPrice", NumericSettingKind::Float, &stopGateway},
    {ID_SETTINGS_PROFIT_VALUE, "ProfitPrice", NumericSettingKind::Float, &profitGateway},
    {ID_SETTINGS_RISK_VALUE, "RiskPct", NumericSettingKind::Float, &riskGateway},
    {ID_SETTINGS_SAFETY_VALUE, "Safety", NumericSettingKind::Float, &safetyGateway},
};

static void SaveNumericEdit(HWND hWnd, const SettingsNumericEdit& setting) {
    HWND hEdit = GetDlgItem(hWnd, setting.id);
    if (setting.kind == NumericSettingKind::Integer) {
        int& value = *static_cast<int*>(setting.value);
        std::string text = EditText(hEdit);
        if (!text.empty()) value = std::atoi(text.c_str());
        RegSetDword(SETTINGS_CLASS_NAME, setting.key, value);
    } else {
        double& value = *static_cast<double*>(setting.value);
        if (GetWindowTextLengthA(hEdit) > 0)
            value = EditNum(hEdit);
        Settings_SaveFloat(setting.key, value);
    }
}

static bool RunRegExe(const std::string& args) {
    std::string command = "reg.exe " + args;
    command.push_back('\0');

    STARTUPINFOA si = { sizeof(si) };
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi = { 0 };

    if (!CreateProcessA(NULL, command.data(), NULL, NULL, FALSE, CREATE_NO_WINDOW,
                        NULL, NULL, &si, &pi))
        return false;

    DWORD waitResult = WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exitCode = 1;
    bool succeeded = waitResult == WAIT_OBJECT_0 &&
                     GetExitCodeProcess(pi.hProcess, &exitCode) &&
                     exitCode == 0;
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return succeeded;
}

LRESULT CALLBACK WndProcSettings(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE: {
            int m  = 8;   // outer margin
            int gm = 14;  // inner margin inside a group box
            int w  = 252; // usable width of one column (fieldset)
            int gw = w - gm * 2; // control width inside group box
            int col2_x = m + w + 16; // x origin of the second column (fieldset + gutter)
            int y  = m;

            auto MakeRow = [&](const char* label, UINT id, int xOrigin, int rowBaseY,
                               int rowY, int editWidth, bool isInt, DWORD extraStyle = 0) -> HWND {
                Ctl(hWnd, "STATIC", label, WS_CHILD | WS_VISIBLE,
                    xOrigin + gm, rowBaseY + rowY, 72, 20);
                DWORD numStyle = isInt ? ES_NUMBER : 0;
                return Ctl(hWnd, "EDIT", "",
                    WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_CENTER | numStyle | extraStyle,
                    xOrigin + gm + 76, rowBaseY + rowY - 3, editWidth, 26,
                    id, nullptr, WS_EX_CLIENTEDGE);
            };

            // ── Gateway ──────────────────────────────────────────────────────
            GroupBox(hWnd, "Gateway:", m, y, w, 280);

            HWND hChkAutoGtw = Ctl(hWnd, "BUTTON", "Auto-start IBKR Gateway",
                WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, m + gm, y + 18, gw, 22,
                ID_SETTINGS_AUTO_GATEWAY);
            SendMessage(hChkAutoGtw, autoGateway ? BM_SETCHECK : BST_UNCHECKED, BST_CHECKED, 0);

            HWND hChkKill = Ctl(hWnd, "BUTTON", "Kill IBKR Gateway on exit",
                WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, m + gm, y + 44, gw, 22,
                ID_SETTINGS_KILL_GATEWAY);
            SendMessage(hChkKill, killGatewayOnExit ? BM_SETCHECK : BST_UNCHECKED, BST_CHECKED, 0);

            Ctl(hWnd, "BUTTON", "Change executable path",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_OWNERDRAW,
                m + gm, y + 70, gw, 22, ID_SETTINGS_GATEWAY_PATH);

            HWND hGatewayEdit = Ctl(hWnd, "EDIT", "",
                WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_LEFT | ES_READONLY,
                m + gm, y + 98, gw, 20, ID_SETTINGS_GATEWAY_PATH_EDIT, nullptr, WS_EX_CLIENTEDGE);
            SetWindowTextA(hGatewayEdit, pathGateway.c_str());

            HWND hAddressEdit = MakeRow("Address:", ID_SETTINGS_ADDRESS, m, y, 128, 147, false);
            SetWindowTextA(hAddressEdit, addressGateway.c_str());

            HWND hUsernameEdit = MakeRow("Username:", ID_SETTINGS_USERNAME, m, y, 158, 147, false);
            
            HWND hPasswordEdit = MakeRow("Password:", ID_SETTINGS_PASSWORD, m, y, 188, 147, false, ES_PASSWORD);

            // ── Populate from Windows Credential Manager (if previously saved) ────────
            {
                std::string savedUsername, savedPassword;
                if (Credentials_Load(savedUsername, savedPassword)) {
                    SetWindowTextA(hUsernameEdit, savedUsername.c_str());
                    SetWindowTextA(hPasswordEdit, savedPassword.c_str());
                }
            }

            // Client ID, passed as the second parameter to api().connect().
            HWND hClientIdEdit = MakeRow("Client ID:", ID_SETTINGS_CLIENT_ID, m, y, 218, 80, true);
            SetWindowTextA(hClientIdEdit, std::format("{}", clientIdGateway).c_str());

            // Group ID, TWS "linked window" group color id, used for
            // subscribeToGroupEvents()/updateDisplayGroup().
            HWND hGroupIdEdit = MakeRow("Group ID:", ID_SETTINGS_GROUP_ID, m, y, 248, 80, true);
            SetWindowTextA(hGroupIdEdit, std::format("{}", groupIdGateway).c_str());

            y += 288;

            // ── Audio ────────────────────────────────────────────────────────
            GroupBox(hWnd, "Audio:", m, y, w, 14 + (30 * 2));

            HWND hChkSounds = Ctl(hWnd, "BUTTON", "Play notification sounds",
                WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, m + gm, y + 18, gw, 22,
                ID_SETTINGS_PLAY_SOUNDS);
            SendMessage(hChkSounds, playSounds ? BM_SETCHECK : BST_UNCHECKED, BST_CHECKED, 0);

            // ── TTS Voice selector ───────────────────────────────────────────
            Ctl(hWnd, "STATIC", "Voice:", WS_CHILD | WS_VISIBLE,
                m + gm, y + 44, 40, 20);

            HWND hVoiceCombo = Ctl(hWnd, "COMBOBOX", "",
                WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
                m + gm + 44, y + 42, gw - 44, 200, ID_SETTINGS_VOICE_COMBO);

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
                    TTS_IsHerenaCatalan(v.tokenId.c_str(), v.display.c_str(), nullptr))
                    herenaIdx = i;
            }

            // If nothing saved yet, default to first Herena-Catalan found
            if (selectIdx < 0) selectIdx = (herenaIdx >= 0) ? herenaIdx : 0;
            if (!settingsVoices.empty())
                SendMessage(hVoiceCombo, CB_SETCURSEL, selectIdx, 0);

            //y += 82;
            y = m;

            // ── Trading ────────────────────────────────────────────────────── (column 2)
            GroupBox(hWnd, "Trading:", col2_x, y, w, 37 * 5);

            HWND hQtyEdit    = MakeRow("Quantity:", ID_SETTINGS_QTY_VALUE, col2_x, y, 20, 80, true);
            HWND hStopEdit   = MakeRow("Stop:",     ID_SETTINGS_STOP_VALUE, col2_x, y, 53, 80, false);
            HWND hProfitEdit = MakeRow("Profit:",   ID_SETTINGS_PROFIT_VALUE, col2_x, y, 86, 80, false);
            HWND hRiskEdit   = MakeRow("Risk %:",   ID_SETTINGS_RISK_VALUE, col2_x, y, 119, 80, false);
            HWND hSafetyEdit = MakeRow("Safety:",   ID_SETTINGS_SAFETY_VALUE, col2_x, y, 152, 80, false);
            y += 152 + 40;

            SetWindowTextA(hQtyEdit,    std::format("{}",     qtyGateway).c_str());
            SetWindowTextA(hStopEdit,   std::format("{:.2f}", stopGateway).c_str());
            SetWindowTextA(hProfitEdit, std::format("{:.2f}", profitGateway).c_str());
            SetWindowTextA(hRiskEdit,   std::format("{:.2f}", riskGateway).c_str());
            SetWindowTextA(hSafetyEdit, std::format("{:.2f}", safetyGateway).c_str());
            
            // ── Display ──────────────────────────────────────────────────────
            GroupBox(hWnd, "Display:", col2_x, y, w, 14 + (30 * 3));

            HWND hChkDark = Ctl(hWnd, "BUTTON", "Dark mode",
                WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                col2_x + gm, y + 18, gw, 22, ID_SETTINGS_DARK_MODE);
            SendMessage(hChkDark, darkMode ? BM_SETCHECK : BST_UNCHECKED, BST_CHECKED, 0);

            HWND hChkAlerts = Ctl(hWnd, "BUTTON", "Full screen alerts",
                WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                col2_x + gm, y + 44, gw, 22, ID_SETTINGS_FULL_SCREEN_ALERTS);
            SendMessage(hChkAlerts, fullScreenAlerts ? BM_SETCHECK : BST_UNCHECKED, BST_CHECKED, 0);
                
            Ctl(hWnd, "STATIC", "Lock:", WS_CHILD | WS_VISIBLE,
                col2_x + gm, y + 70, 72, 20);
            HWND hLockEdit = Ctl(hWnd, "EDIT", "",
                WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_CENTER | ES_PASSWORD,
                col2_x + gm + 76, y + 67, 147, 26, ID_SETTINGS_LOCK, nullptr, WS_EX_CLIENTEDGE);
            SetWindowTextA(hLockEdit, lockScreen.c_str());

            y += 114;

            // ── System Tools ─────────────────────────────────────────── (column 2)
            GroupBox(hWnd, "Configuration:", col2_x, y, w, 78);

            // Two buttons share the row: Backup (left) and Restore (right), with a 4px gutter.
            int btn = (gw - 4) / 2; // width of each button
            Ctl(hWnd, "BUTTON", "Backup",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_OWNERDRAW,
                col2_x + gm, y + 20, btn, 22, ID_SETTINGS_BACKUP_DOWNLOAD);

            Ctl(hWnd, "BUTTON", "Restore",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_OWNERDRAW,
                col2_x + gm + btn + 4, y + 20, btn, 22, ID_SETTINGS_BACKUP_RESTORE);

            Ctl(hWnd, "BUTTON", "Debug Log",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_OWNERDRAW,
                col2_x + gm, y + 48, gw, 22, ID_SETTINGS_DEBUG_LOG);

            break;
        }

        case WM_DESTROY:
            settingsVoices.clear();
            break;

        case WM_COMMAND:
            for (const auto& setting : settingsChecks) {
                if (LOWORD(wParam) == setting.id) {
                    SaveCheck(hWnd, setting.id, setting.key, *setting.value);
                    break;
                }
            }
            if (LOWORD(wParam) == ID_SETTINGS_GATEWAY_PATH) {
                HWND hPathEdit = GetDlgItem(hWnd, ID_SETTINGS_GATEWAY_PATH_EDIT);
                if (hPathEdit != NULL) {
                    pathGateway = AskGatewayPath(hWnd);
                    if (!pathGateway.empty()) {
                        SaveGatewayPath(pathGateway);
                        SetWindowTextA(hPathEdit, pathGateway.c_str());
                    }
                }
            }
            if (LOWORD(wParam) == ID_SETTINGS_ADDRESS && HIWORD(wParam) == EN_CHANGE) {
                HWND hEdit = GetDlgItem(hWnd, ID_SETTINGS_ADDRESS);
                addressGateway = EditText(hEdit);
                if (addressGateway.empty()) {
                    addressGateway = std::string("127.0.0.1");
                }
                RegSetString(SETTINGS_CLASS_NAME, "IPAddress", addressGateway);
            }
            if (LOWORD(wParam) == ID_SETTINGS_DEBUG_LOG) {
                StartDebugLog();
                FlushDebugBuffer();
            }
            if ((LOWORD(wParam) == ID_SETTINGS_PASSWORD || LOWORD(wParam) == ID_SETTINGS_USERNAME) && HIWORD(wParam) == EN_CHANGE) {
                Settings_SaveCredentialsFromUI(hWnd);
            }
            if (LOWORD(wParam) == ID_SETTINGS_LOCK && HIWORD(wParam) == EN_CHANGE) {
                HWND hEdit = GetDlgItem(hWnd, ID_SETTINGS_LOCK);
                lockScreen = EditText(hEdit);
                RegSetString(SETTINGS_CLASS_NAME, "Lock", lockScreen);
            }
            if (LOWORD(wParam) == ID_SETTINGS_DARK_MODE) {
                HWND hChk = GetDlgItem(hWnd, ID_SETTINGS_DARK_MODE);
                DWORD checked = (SendMessage(hChk, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
                RegSetDword(SETTINGS_CLASS_NAME, "DarkMode", checked);
                darkMode = (checked == 1);
                ApplyDarkModeToAllWindows();
                // Force full repaint of this window and all children
                InvalidateRect(hWnd, NULL, TRUE);
                RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_UPDATENOW);
            }
            if (HIWORD(wParam) == EN_CHANGE) {
                for (const auto& setting : settingsNumericEdits) {
                    if (LOWORD(wParam) == setting.id) {
                        SaveNumericEdit(hWnd, setting);
                        break;
                    }
                }
            }
            if (LOWORD(wParam) == ID_SETTINGS_VOICE_COMBO && HIWORD(wParam) == CBN_SELCHANGE) {
                HWND hCombo = GetDlgItem(hWnd, ID_SETTINGS_VOICE_COMBO);
                int idx = (int)SendMessage(hCombo, CB_GETCURSEL, 0, 0);
                if (idx >= 0 && idx < (int)settingsVoices.size()) {
                    const std::wstring& tid = settingsVoices[idx].tokenId;
                    std::string tidA(tid.begin(), tid.end());
                    Settings_SaveTtsVoice(tidA);
                    // Notify all open windows to hot-swap to the new voice immediately
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
                    // /y suppresses overwrite confirmation since the file picker already handles it.
                    std::string args = std::format("export \"HKCU\\{}\" \"{}\" /y", APP_REG_ROOT, szFile);
                    if (RunRegExe(args)) {
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
                    std::string args = std::format("import \"{}\"", szFile);
                    if (RunRegExe(args)) {
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
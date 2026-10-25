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

static void UpdateDarkModeSettingCheckbox(bool checked) {
    HWND hSettings = FindWindowA(SETTINGS_CLASS_NAME, NULL);
    if (!hSettings || !IsWindow(hSettings)) return;

    HWND hCheckbox = GetDlgItem(hSettings, ID_SETTINGS_DARK_MODE);
    if (hCheckbox)
        SendMessage(hCheckbox, BM_SETCHECK, checked ? BST_CHECKED : BST_UNCHECKED, 0);
}

static std::vector<TtsVoiceEntry> settingsVoices; // populated once on WM_CREATE

// ─── Debug Log ────────────────────────────────────────────────────────────────
void FlushDebugBuffer() {
    if (!hDebugEdit || !IsWindow(hDebugEdit)) return;

    std::string text;
    {
        std::lock_guard<std::mutex> lock(debugBufferMutex);
        size_t total = 0;
        for (const auto& msg : debugBuffer) total += msg.size();
        text.reserve(total);
        for (const auto& msg : debugBuffer) text += msg;
    }

    SendMessageA(hDebugEdit, WM_SETREDRAW, FALSE, 0);
    SetWindowTextA(hDebugEdit, text.c_str());
    SendMessage(hDebugEdit, EM_SETSEL, (WPARAM)-1, (LPARAM)-1);
    SendMessage(hDebugEdit, EM_SCROLLCARET, 0, 0);
    SendMessageA(hDebugEdit, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(hDebugEdit, NULL, TRUE);
}

LRESULT CALLBACK WndProcDebugLog(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE: {
            hDebugEdit = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
                WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL |
                ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_READONLY,
                0, 0, 0, 0, hWnd, NULL, GetModuleHandle(NULL), NULL);
            SendMessage(hDebugEdit, EM_LIMITTEXT, 0, 0);
            debugAppendPending.store(false);
            RECT rc;
            GetClientRect(hWnd, &rc);
            SetWindowPos(hDebugEdit, NULL, 0, 0, rc.right, rc.bottom, SWP_NOZORDER);
            FlushDebugBuffer(); // ← show all buffered messages
            break;
        }
        case WM_DESTROY:
            hDebugEdit = NULL;   // ← avoid a stale handle lingering after the window is destroyed
            debugAppendPending.store(false);
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
            debugAppendPending.store(false, std::memory_order_release);
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

enum class SettingType {
    Boolean,
    Integer,
    Decimal,
    String,
};

enum class SettingsArea {
    Gateway,
    Audio,
    Trading,
    Display,
};

using SettingDefault = std::variant<bool, int, double, std::string>;
using SettingValue = std::variant<bool*, int*, double*, std::string*>;

struct SettingDescriptor {
    const char* key;
    SettingType type;
    SettingDefault defaultValue;
    const char* label;
    int id;
    SettingValue value;
    SettingsArea area;
    int row;
    int editWidth;
    DWORD extraStyle;
};

static const SettingDescriptor settings[] = {
    {"Gateway_AutoStart",  SettingType::Boolean, false,                   "Auto-start IBKR Gateway", ID_SETTINGS_AUTO_GATEWAY, &autoGateway, SettingsArea::Gateway, 18, 0, 0},
    {"Gateway_KillOnExit", SettingType::Boolean, false,                   "Kill IBKR Gateway on exit", ID_SETTINGS_KILL_GATEWAY, &killGatewayOnExit, SettingsArea::Gateway, 44, 0, 0},
    {"PlaySounds",         SettingType::Boolean, false,                   "Play notification sounds", ID_SETTINGS_PLAY_SOUNDS, &playSounds, SettingsArea::Audio, 18, 0, 0},
    {"DarkMode",           SettingType::Boolean, false,                   "Dark mode", ID_SETTINGS_DARK_MODE, &darkMode, SettingsArea::Display, 18, 0, 0},
    {"FullScreenAlerts",   SettingType::Boolean, false,                   "Full screen alerts", ID_SETTINGS_FULL_SCREEN_ALERTS, &fullScreenAlerts, SettingsArea::Display, 44, 0, 0},
    {"ClientId",           SettingType::Integer, 0,                       "Client ID:", ID_SETTINGS_CLIENT_ID, &clientIdGateway, SettingsArea::Gateway, 218, 80, 0},
    {"GroupId",            SettingType::Integer, 4,                       "Group ID:", ID_SETTINGS_GROUP_ID, &groupIdGateway, SettingsArea::Gateway, 248, 80, 0},
    {"OrderQty",           SettingType::Integer, 20,                      "Quantity:", ID_SETTINGS_QTY_VALUE, &qtyGateway, SettingsArea::Trading, 20, 80, 0},
    {"StopPrice",          SettingType::Decimal, 1.0,                     "Stop:", ID_SETTINGS_STOP_VALUE, &stopGateway, SettingsArea::Trading, 53, 80, 0},
    {"ProfitPrice",        SettingType::Decimal, 2.0,                     "Profit:", ID_SETTINGS_PROFIT_VALUE, &profitGateway, SettingsArea::Trading, 86, 80, 0},
    {"RiskPct",            SettingType::Decimal, 1.0,                     "Risk %:", ID_SETTINGS_RISK_VALUE, &riskGateway, SettingsArea::Trading, 119, 80, 0},
    {"Safety",             SettingType::Decimal, 2.0,                     "Safety:", ID_SETTINGS_SAFETY_VALUE, &safetyGateway, SettingsArea::Trading, 152, 80, 0},
    {"IPAddress",          SettingType::String, std::string("127.0.0.1"), "Address:", ID_SETTINGS_ADDRESS, &addressGateway, SettingsArea::Gateway, 128, 147, 0},
    {"Lock",               SettingType::String, std::string(),            "Lock:", ID_SETTINGS_LOCK, &lockScreen, SettingsArea::Display, 70, 147, ES_PASSWORD},
};

static const SettingDescriptor* FindSetting(int id) {
    for (const auto& setting : settings)
        if (setting.id == id) return &setting;
    return nullptr;
}

static void LoadRegistrySettings() {
    for (const auto& setting : settings) {
        switch (setting.type) {
            case SettingType::Boolean:
                *std::get<bool*>(setting.value) =
                    RegGetDword(SETTINGS_CLASS_NAME, setting.key, std::get<bool>(setting.defaultValue)) != 0;
                break;
            case SettingType::Integer:
                *std::get<int*>(setting.value) =
                    static_cast<int>(RegGetDword(SETTINGS_CLASS_NAME, setting.key,
                                                 static_cast<DWORD>(std::get<int>(setting.defaultValue))));
                break;
            case SettingType::Decimal:
                *std::get<double*>(setting.value) =
                    Settings_LoadFloat(setting.key, std::get<double>(setting.defaultValue));
                break;
            case SettingType::String:
                *std::get<std::string*>(setting.value) =
                    RegGetString(SETTINGS_CLASS_NAME, setting.key, std::get<std::string>(setting.defaultValue));
                break;
        }
    }
    
    pathGateway = GetGatewayPath();
}

static void SaveSetting(HWND hWnd, const SettingDescriptor& setting) {
    HWND control = GetDlgItem(hWnd, setting.id);
    switch (setting.type) {
        case SettingType::Boolean: {
            bool& value = *std::get<bool*>(setting.value);
            value = SendMessage(control, BM_GETCHECK, 0, 0) == BST_CHECKED;
            RegSetDword(SETTINGS_CLASS_NAME, setting.key, value);
            break;
        }
        case SettingType::Integer: {
            int& value = *std::get<int*>(setting.value);
            const std::string text = EditText(control);
            if (!text.empty()) value = std::atoi(text.c_str());
            RegSetDword(SETTINGS_CLASS_NAME, setting.key, static_cast<DWORD>(value));
            break;
        }
        case SettingType::Decimal: {
            double& value = *std::get<double*>(setting.value);
            if (GetWindowTextLengthA(control) > 0) value = EditNum(control);
            Settings_SaveFloat(setting.key, value);
            break;
        }
        case SettingType::String: {
            std::string& value = *std::get<std::string*>(setting.value);
            value = EditText(control);
            if (value.empty()) value = std::get<std::string>(setting.defaultValue);
            RegSetString(SETTINGS_CLASS_NAME, setting.key, value);
            break;
        }
    }
}

static void SaveSettingsFromUI(HWND hWnd) {
    for (const auto& setting : settings)
        SaveSetting(hWnd, setting);
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
            auto CreateSettingsControls = [&](SettingsArea area, int xOrigin, int rowBaseY) {
                for (const auto& setting : settings) {
                    if (setting.area != area) continue;
                    if (setting.type == SettingType::Boolean) {
                        HWND checkbox = Ctl(hWnd, "BUTTON", setting.label,
                            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                            xOrigin + gm, rowBaseY + setting.row, gw, 22, setting.id);
                        const bool checked = *std::get<bool*>(setting.value);
                        SendMessage(checkbox, BM_SETCHECK, checked ? BST_CHECKED : BST_UNCHECKED, 0);
                        continue;
                    }

                    HWND edit = MakeRow(setting.label, setting.id, xOrigin, rowBaseY,
                        setting.row, setting.editWidth, setting.type == SettingType::Integer,
                        setting.extraStyle);
                    switch (setting.type) {
                        case SettingType::Integer:
                            SetWindowTextA(edit, std::format("{}",
                                *std::get<int*>(setting.value)).c_str());
                            break;
                        case SettingType::Decimal:
                            SetWindowTextA(edit, std::format("{:.2f}",
                                *std::get<double*>(setting.value)).c_str());
                            break;
                        case SettingType::String:
                            SetWindowTextA(edit, std::get<std::string*>(setting.value)->c_str());
                            break;
                        case SettingType::Boolean:
                            break;
                    }
                }
            };

            // ── Gateway ──────────────────────────────────────────────────────
            GroupBox(hWnd, "Gateway:", m, y, w, 280);

            CreateSettingsControls(SettingsArea::Gateway, m, y);

            Ctl(hWnd, "BUTTON", "Change executable path",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_OWNERDRAW,
                m + gm, y + 70, gw, 22, ID_SETTINGS_GATEWAY_PATH);

            HWND hGatewayEdit = Ctl(hWnd, "EDIT", "",
                WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_LEFT | ES_READONLY,
                m + gm, y + 98, gw, 20, ID_SETTINGS_GATEWAY_PATH_EDIT, nullptr, WS_EX_CLIENTEDGE);
            SetWindowTextA(hGatewayEdit, pathGateway.c_str());

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

            y += 288;

            // ── Audio ────────────────────────────────────────────────────────
            GroupBox(hWnd, "Audio:", m, y, w, 14 + (30 * 2));

            CreateSettingsControls(SettingsArea::Audio, m, y);

            // ── TTS Voice selector ───────────────────────────────────────────
            Ctl(hWnd, "STATIC", "Voice:", WS_CHILD | WS_VISIBLE,
                m + gm, y + 44, 40, 20);

            HWND hVoiceCombo = Ctl(hWnd, "COMBOBOX", "",
                WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
                m + gm + 44, y + 42, gw - 44, 200, ID_SETTINGS_VOICE_COMBO);

            // Enumerate all system voices and fill the combo
            settingsVoices = TTS_EnumerateVoices();
            std::string savedTokenA = Settings_LoadTtsVoice();
            std::wstring savedToken = StringToWide(savedTokenA);

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

            CreateSettingsControls(SettingsArea::Trading, col2_x, y);
            y += 152 + 40;
            
            // ── Display ──────────────────────────────────────────────────────
            GroupBox(hWnd, "Display:", col2_x, y, w, 14 + (30 * 3));

            CreateSettingsControls(SettingsArea::Display, col2_x, y);

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
            if (const SettingDescriptor* setting = FindSetting(LOWORD(wParam));
                setting && setting->type == SettingType::Boolean) {
                SaveSetting(hWnd, *setting);
                if (setting->id == ID_SETTINGS_DARK_MODE) {
                    ApplyDarkModeToAllWindows();
                    InvalidateRect(hWnd, NULL, TRUE);
                    RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_UPDATENOW);
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
            if (LOWORD(wParam) == ID_SETTINGS_DEBUG_LOG) {
                StartDebugLog();
                FlushDebugBuffer();
            }
            if ((LOWORD(wParam) == ID_SETTINGS_PASSWORD || LOWORD(wParam) == ID_SETTINGS_USERNAME) && HIWORD(wParam) == EN_CHANGE) {
                Settings_SaveCredentialsFromUI(hWnd);
            }
            if (HIWORD(wParam) == EN_CHANGE) {
                if (const SettingDescriptor* setting = FindSetting(LOWORD(wParam));
                    setting && setting->type != SettingType::Boolean) {
                    SaveSetting(hWnd, *setting);
                }
            }
            if (LOWORD(wParam) == ID_SETTINGS_VOICE_COMBO && HIWORD(wParam) == CBN_SELCHANGE) {
                HWND hCombo = GetDlgItem(hWnd, ID_SETTINGS_VOICE_COMBO);
                int idx = (int)SendMessage(hCombo, CB_GETCURSEL, 0, 0);
                if (idx >= 0 && idx < (int)settingsVoices.size()) {
                    const std::wstring& tid = settingsVoices[idx].tokenId;
                    std::string tidA = WideToString(tid);
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
                    SaveSettingsFromUI(hWnd);
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
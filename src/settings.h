#pragma once

static const char* SETTINGS_CLASS_NAME = "TNTSettingsWindowClass";

void startSettings() { startGenericWindow(SETTINGS_CLASS_NAME, "Settings", L"IBKRGatewayClient.Settings", 300, 280); }

#define ID_SETTINGS_KILL_GATEWAY 4001
#define ID_SETTINGS_DARK_MODE    4002

bool Settings_KillGatewayOnExit() {
    return Settings_Load("KillGatewayOnExit", 0) != 0;
}

// Call this on every window after creating it
void ApplyDarkModeToAllWindows() {
    // Enumerate all top-level windows owned by this process
    EnumWindows([](HWND hWnd, LPARAM) -> BOOL {
        DWORD pid;
        GetWindowThreadProcessId(hWnd, &pid);
        if (pid == GetCurrentProcessId())
            ApplyDarkMode(hWnd);
        return TRUE;
    }, 0);
}

LRESULT CALLBACK WndProcSettings(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {

    case WM_CREATE: {
        Session_AddWindow(hWnd);
        HINSTANCE hInst = ((LPCREATESTRUCT)lParam)->hInstance;
        int margin = 8;

        HWND hChkKill = CreateWindowA("BUTTON", "Kill IBKR Gateway on exit",
            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
            margin, margin, 250, 24,
            hWnd, (HMENU)ID_SETTINGS_KILL_GATEWAY, hInst, NULL);
        if (Settings_KillGatewayOnExit())
            SendMessage(hChkKill, BM_SETCHECK, BST_CHECKED, 0);

        HWND hChkDark = CreateWindowA("BUTTON", "Dark mode",
            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
            margin, margin + 32, 250, 24,
            hWnd, (HMENU)ID_SETTINGS_DARK_MODE, hInst, NULL);
        if (Settings_DarkMode())
            SendMessage(hChkDark, BM_SETCHECK, BST_CHECKED, 0);

        ApplyDarkMode(hWnd);
        break;
    }

    case WM_COMMAND:
        if (LOWORD(wParam) == ID_SETTINGS_KILL_GATEWAY) {
            HWND hChk = GetDlgItem(hWnd, ID_SETTINGS_KILL_GATEWAY);
            DWORD checked = (SendMessage(hChk, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
            Settings_Save("KillGatewayOnExit", checked);
        }
        if (LOWORD(wParam) == ID_SETTINGS_DARK_MODE) {
            HWND hChk = GetDlgItem(hWnd, ID_SETTINGS_DARK_MODE);
            DWORD checked = (SendMessage(hChk, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
            Settings_Save("DarkMode", checked);
            // Apply immediately to all open windows
            ApplyDarkModeToAllWindows();
        }
        break;

    case WM_CLOSE:
        DestroyWindow(hWnd);
        break;

    case WM_MOVE:
        SaveWinPosition(hWnd);
        break;

    case WM_DESTROY:
        SaveWinPosition(hWnd);
        Session_RemoveWindow(hWnd);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
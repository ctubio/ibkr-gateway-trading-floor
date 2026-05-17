#pragma once

HWND hSettingsWnd = NULL;
static const char* SETTINGS_CLASS_NAME = "TNTSettingsWindowClass";

#define ID_SETTINGS_KILL_GATEWAY 4001

bool Settings_KillGatewayOnExit() {
    return Settings_Load("KillGatewayOnExit", 0) != 0;
}

LRESULT CALLBACK WndProcSettings(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {

    case WM_CREATE: {
        Session_AddWindow(hWnd);
        HINSTANCE hInst = ((LPCREATESTRUCT)lParam)->hInstance;
        int margin = 8;

        HWND hChk = CreateWindowA("BUTTON", "Kill IBKR Gateway on exit",
            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
            margin, margin, 250, 24,
            hWnd, (HMENU)ID_SETTINGS_KILL_GATEWAY, hInst, NULL);

        // Load saved state
        if (Settings_KillGatewayOnExit())
            SendMessage(hChk, BM_SETCHECK, BST_CHECKED, 0);

        break;
    }

    case WM_COMMAND:
        if (LOWORD(wParam) == ID_SETTINGS_KILL_GATEWAY) {
            HWND hChk = GetDlgItem(hWnd, ID_SETTINGS_KILL_GATEWAY);
            DWORD checked = (SendMessage(hChk, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
            Settings_Save("KillGatewayOnExit", checked);
        }
        break;

    case WM_CLOSE:
        DestroyWindow(hWnd);
        break;

    case WM_MOVE:
        SaveWinPosition(hWnd, SETTINGS_CLASS_NAME);
        break;

    case WM_DESTROY:
        SaveWinPosition(hWnd, SETTINGS_CLASS_NAME);
        Session_RemoveWindow(hWnd);
        hSettingsWnd = NULL;
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

void startSettings() {
    if (hSettingsWnd && IsWindow(hSettingsWnd)) {
        ShowWindow(hSettingsWnd, SW_SHOW);
        SetForegroundWindow(hSettingsWnd);
    } else {
        int x = CW_USEDEFAULT, y = CW_USEDEFAULT, w = 300, h = 80;
        LoadWinPosition(SETTINGS_CLASS_NAME, x, y, w, h);

        hSettingsWnd = CreateWindowExA(
            WS_EX_APPWINDOW,
            SETTINGS_CLASS_NAME,
            "Settings",
            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE,
            x, y, w, h,
            NULL, NULL, GetModuleHandle(NULL), NULL
        );

        SetWindowTaskbarId(hSettingsWnd, L"IBKRTunnel.Settings");
    }
}
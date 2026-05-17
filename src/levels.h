#pragma once

HWND hLevelsWnd = NULL;
static const char* LEVELS_CLASS_NAME = "TNTLevelsWindowClass";


LRESULT CALLBACK WndProcLevels(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {

    case WM_CREATE: {
        Session_AddWindow(hWnd);
        break;
    }

    case WM_CLOSE:
        DestroyWindow(hWnd);
        break;

    case WM_MOVE:
        SaveWinPosition(hWnd, LEVELS_CLASS_NAME);
        break;

    case WM_DESTROY:
        SaveWinPosition(hWnd, LEVELS_CLASS_NAME);
        Session_RemoveWindow(hWnd);
        hLevelsWnd = NULL;
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

void startLevels() {
    if (hLevelsWnd && IsWindow(hLevelsWnd)) {
        ShowWindow(hLevelsWnd, SW_SHOW);
        SetForegroundWindow(hLevelsWnd);
    } else {
        int x = CW_USEDEFAULT, y = CW_USEDEFAULT, w = 380, h = 240;
        LoadWinPosition(LEVELS_CLASS_NAME, x, y, w, h);

        hLevelsWnd = CreateWindowExA(
            WS_EX_APPWINDOW,
            LEVELS_CLASS_NAME,
            "Levels",
            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE,
            x, y, w, h,
            NULL, NULL, GetModuleHandle(NULL), NULL
        );

        SetWindowTaskbarId(hLevelsWnd, L"IBKRTunnel.Levels");
    }
}

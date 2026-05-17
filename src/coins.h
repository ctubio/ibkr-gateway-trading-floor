#pragma once

HWND hCoinsWnd = NULL;
static const char* COINS_CLASS_NAME = "TNTCoinsWindowClass";


LRESULT CALLBACK WndProcCoins(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {

    case WM_CREATE: {
        Session_AddWindow(hWnd);
        break;
    }

    case WM_CLOSE:
        DestroyWindow(hWnd);
        break;

    case WM_MOVE:
        SaveWinPosition(hWnd, COINS_CLASS_NAME);
        break;

    case WM_DESTROY:
        SaveWinPosition(hWnd, COINS_CLASS_NAME);
        Session_RemoveWindow(hWnd);
        hCoinsWnd = NULL;
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

void startCoins() {
    if (hCoinsWnd && IsWindow(hCoinsWnd)) {
        ShowWindow(hCoinsWnd, SW_SHOW);
        SetForegroundWindow(hCoinsWnd);
    } else {
        int x = CW_USEDEFAULT, y = CW_USEDEFAULT, w = 380, h = 240;
        LoadWinPosition(COINS_CLASS_NAME, x, y, w, h);

        hCoinsWnd = CreateWindowExA(
            WS_EX_APPWINDOW,
            COINS_CLASS_NAME,
            "Coins",
            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE,
            x, y, w, h,
            NULL, NULL, GetModuleHandle(NULL), NULL
        );

        SetWindowTaskbarId(hCoinsWnd, L"IBKRTunnel.Coins");
    }
}

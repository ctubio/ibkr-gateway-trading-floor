#pragma once

HWND hDiamondsWnd = NULL;
static const char* DIAMONDS_CLASS_NAME = "TNTDiamondsWindowClass";


LRESULT CALLBACK WndProcDiamonds(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {

    case WM_CREATE: {
        Session_AddWindow(hWnd);
        break;
    }

    case WM_CLOSE:
        DestroyWindow(hWnd);
        break;

    case WM_MOVE:
        SaveWinPosition(hWnd, DIAMONDS_CLASS_NAME);
        break;

    case WM_DESTROY:
        SaveWinPosition(hWnd, DIAMONDS_CLASS_NAME);
        Session_RemoveWindow(hWnd);
        hDiamondsWnd = NULL;
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

void startDiamonds() {
    if (hDiamondsWnd && IsWindow(hDiamondsWnd)) {
        ShowWindow(hDiamondsWnd, SW_SHOW);
        SetForegroundWindow(hDiamondsWnd);
    } else {
        int x = CW_USEDEFAULT, y = CW_USEDEFAULT, w = 380, h = 240;
        LoadWinPosition(DIAMONDS_CLASS_NAME, x, y, w, h);

        hDiamondsWnd = CreateWindowExA(
            WS_EX_APPWINDOW,
            DIAMONDS_CLASS_NAME,
            "Diamonds",
            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE,
            x, y, w, h,
            NULL, NULL, GetModuleHandle(NULL), NULL
        );

        SetWindowTaskbarId(hDiamondsWnd, L"IBKRTunnel.Diamonds");
    }
}

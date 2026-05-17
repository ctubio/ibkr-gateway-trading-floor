#pragma once

HWND hTickerWnd = NULL;
static const char* TICKER_CLASS_NAME = "TNTTickerWindowClass";


LRESULT CALLBACK WndProcTicker(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {

    case WM_CREATE: {
        Session_AddWindow(hWnd);
        break;
    }

    case WM_CLOSE:
        DestroyWindow(hWnd);
        break;

    case WM_MOVE:
        SaveWinPosition(hWnd, TICKER_CLASS_NAME);
        break;

    case WM_DESTROY:
        SaveWinPosition(hWnd, TICKER_CLASS_NAME);
        Session_RemoveWindow(hWnd);
        hTickerWnd = NULL;
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

void startTicker() {
    if (hTickerWnd && IsWindow(hTickerWnd)) {
        ShowWindow(hTickerWnd, SW_SHOW);
        SetForegroundWindow(hTickerWnd);
    } else {
        int x = CW_USEDEFAULT, y = CW_USEDEFAULT, w = 380, h = 240;
        LoadWinPosition(TICKER_CLASS_NAME, x, y, w, h);

        hTickerWnd = CreateWindowExA(
            WS_EX_APPWINDOW,
            TICKER_CLASS_NAME,
            "Ticker",
            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE,
            x, y, w, h,
            NULL, NULL, GetModuleHandle(NULL), NULL
        );

        SetWindowTaskbarId(hTickerWnd, L"IBKRTunnel.Ticker");
    }
}

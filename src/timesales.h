#pragma once

HWND hTimesalesWnd = NULL;
static const char* TIMESALES_CLASS_NAME = "TNTTimesalesWindowClass";


LRESULT CALLBACK WndProcTimesales(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {

    case WM_CREATE: {
        Session_AddWindow(hWnd);
        break;
    }

    case WM_CLOSE:
        DestroyWindow(hWnd);
        break;

    case WM_MOVE:
        SaveWinPosition(hWnd, TIMESALES_CLASS_NAME);
        break;

    case WM_DESTROY:
        SaveWinPosition(hWnd, TIMESALES_CLASS_NAME);
        Session_RemoveWindow(hWnd);
        hTimesalesWnd = NULL;
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

void startTimesales() {
    if (hTimesalesWnd && IsWindow(hTimesalesWnd)) {
        ShowWindow(hTimesalesWnd, SW_SHOW);
        SetForegroundWindow(hTimesalesWnd);
    } else {
        int x = CW_USEDEFAULT, y = CW_USEDEFAULT, w = 380, h = 240;
        LoadWinPosition(TIMESALES_CLASS_NAME, x, y, w, h);

        hTimesalesWnd = CreateWindowExA(
            WS_EX_APPWINDOW,
            TIMESALES_CLASS_NAME,
            "Timesales",
            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE,
            x, y, w, h,
            NULL, NULL, GetModuleHandle(NULL), NULL
        );

        SetWindowTaskbarId(hTimesalesWnd, L"IBKRTunnel.Timesales");
    }
}

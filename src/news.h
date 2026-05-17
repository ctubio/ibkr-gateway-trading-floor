#pragma once

HWND hNewsWnd = NULL;
static const char* NEWS_CLASS_NAME = "TNTNewsWindowClass";


LRESULT CALLBACK WndProcNews(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {

    case WM_CREATE: {
        Session_AddWindow(hWnd);
        break;
    }

    case WM_CLOSE:
        DestroyWindow(hWnd);
        break;

    case WM_MOVE:
        SaveWinPosition(hWnd, NEWS_CLASS_NAME);
        break;

    case WM_DESTROY:
        SaveWinPosition(hWnd, NEWS_CLASS_NAME);
        Session_RemoveWindow(hWnd);
        hNewsWnd = NULL;
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

void startNews() {
    if (hNewsWnd && IsWindow(hNewsWnd)) {
        ShowWindow(hNewsWnd, SW_SHOW);
        SetForegroundWindow(hNewsWnd);
    } else {
        int x = CW_USEDEFAULT, y = CW_USEDEFAULT, w = 380, h = 240;
        LoadWinPosition(NEWS_CLASS_NAME, x, y, w, h);

        hNewsWnd = CreateWindowExA(
            WS_EX_APPWINDOW,
            NEWS_CLASS_NAME,
            "News",
            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE,
            x, y, w, h,
            NULL, NULL, GetModuleHandle(NULL), NULL
        );

        SetWindowTaskbarId(hNewsWnd, L"IBKRTunnel.News");
    }
}

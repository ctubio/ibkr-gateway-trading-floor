#pragma once

HWND hOrdersWnd = NULL;
static const char* ORDERS_CLASS_NAME = "TNTOrdersWindowClass";


LRESULT CALLBACK WndProcOrders(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {

    case WM_CREATE: {
        Session_AddWindow(hWnd);
        break;
    }

    case WM_CLOSE:
        DestroyWindow(hWnd);
        break;

    case WM_MOVE:
        SaveWinPosition(hWnd, ORDERS_CLASS_NAME);
        break;

    case WM_DESTROY:
        SaveWinPosition(hWnd, ORDERS_CLASS_NAME);
        Session_RemoveWindow(hWnd);
        hOrdersWnd = NULL;
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

void startOrders() {
    if (hOrdersWnd && IsWindow(hOrdersWnd)) {
        ShowWindow(hOrdersWnd, SW_SHOW);
        SetForegroundWindow(hOrdersWnd);
    } else {
        int x = CW_USEDEFAULT, y = CW_USEDEFAULT, w = 380, h = 240;
        LoadWinPosition(ORDERS_CLASS_NAME, x, y, w, h);

        hOrdersWnd = CreateWindowExA(
            WS_EX_APPWINDOW,
            ORDERS_CLASS_NAME,
            "Orders",
            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE,
            x, y, w, h,
            NULL, NULL, GetModuleHandle(NULL), NULL
        );

        SetWindowTaskbarId(hOrdersWnd, L"IBKRTunnel.Orders");
    }
}

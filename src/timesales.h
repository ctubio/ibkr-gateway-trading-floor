#pragma once

static const char* TIMESALES_CLASS_NAME = "TNTTimesalesWindowClass";

void startTimesales() { startGenericWindow(TIMESALES_CLASS_NAME, "Timesales", L"IBKRGatewayClient.Timesales", 380, 240); }

LRESULT CALLBACK WndProcTimesales(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    return HandleCommonMessages(hWnd, message, wParam, lParam);
}

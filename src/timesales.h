#pragma once

void startTimesales() { startGenericWindow(TIMESALES_CLASS_NAME, "Time and Sales", L"IBKRGatewayClient.Timesales", 380, 240); }

LRESULT CALLBACK WndProcTimesales(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    return HandleCommonMessages(hWnd, message, wParam, lParam);
}

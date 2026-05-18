#pragma once

void startTicker() { startGenericWindow(TICKER_CLASS_NAME, "Ticker", L"IBKRGatewayClient.Ticker", 380, 240); }

LRESULT CALLBACK WndProcTicker(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    return HandleCommonMessages(hWnd, message, wParam, lParam);
}

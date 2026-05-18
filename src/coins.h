#pragma once

void startCoins() { startGenericWindow(COINS_CLASS_NAME, "Coins", L"IBKRGatewayClient.Coins", 380, 240); }

LRESULT CALLBACK WndProcCoins(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    return HandleCommonMessages(hWnd, message, wParam, lParam);
}

#pragma once

void startDiamonds() { startGenericWindow(DIAMONDS_CLASS_NAME, "Diamonds", L"IBKRGatewayClient.Diamonds", 380, 240); }

LRESULT CALLBACK WndProcDiamonds(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    return HandleCommonMessages(hWnd, message, wParam, lParam);
}

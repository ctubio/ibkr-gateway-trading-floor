#pragma once

void startNews() { startGenericWindow(NEWS_CLASS_NAME, "News", L"IBKRGatewayClient.News", 380, 240); }

LRESULT CALLBACK WndProcNews(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    return HandleCommonMessages(hWnd, message, wParam, lParam);
}

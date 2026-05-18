#pragma once

void startLevels() { startGenericWindow(LEVELS_CLASS_NAME, "Levels", L"IBKRGatewayClient.Levels", 380, 240); }

LRESULT CALLBACK WndProcLevels(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    return HandleCommonMessages(hWnd, message, wParam, lParam);
}


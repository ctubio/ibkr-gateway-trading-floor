#pragma once


void startOrders() { startGenericWindow(ORDERS_CLASS_NAME, "Orders", L"IBKRGatewayClient.Orders", 380, 240); }

LRESULT CALLBACK WndProcOrders(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    return HandleCommonMessages(hWnd, message, wParam, lParam);
}
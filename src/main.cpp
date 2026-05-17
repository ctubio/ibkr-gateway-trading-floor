#include "api.h"
#include "shared.h"
#include "book.h"
#include "diamonds.h"
#include "coins.h"
#include "news.h"
#include "timesales.h"
#include "levels.h"
#include "orders.h"
#include "ticker.h"
#include "settings.h"
#include "dashboard.h"

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nShow) {
    HANDLE hMutex = mutex_on();
	if (!hMutex) {
        MessageBoxA(NULL, "Application already running!", "Exception", MB_OK);
        return 0;
    } 
    
    registerWindowClass(hInst, WndProcBook,      BOOK_CLASS_NAME,       2);
    registerWindowClass(hInst, WndProcCoins,     COINS_CLASS_NAME,      3);
    registerWindowClass(hInst, WndProcDiamonds,  DIAMONDS_CLASS_NAME,   4);
    registerWindowClass(hInst, WndProcSettings,  SETTINGS_CLASS_NAME,   5);
    registerWindowClass(hInst, WndProcNews,      NEWS_CLASS_NAME,       6);
    registerWindowClass(hInst, WndProcTicker,    TICKER_CLASS_NAME,     9);
    registerWindowClass(hInst, WndProcTimesales, TIMESALES_CLASS_NAME,  7);
    registerWindowClass(hInst, WndProcLevels,    LEVELS_CLASS_NAME,     8);
    registerWindowClass(hInst, WndProcOrders,    ORDERS_CLASS_NAME,    10);
    registerWindowClass(hInst, WndProcDashboard, DASHBOARD_CLASS_NAME,  1);
    registerSystemIcon(hInst);

    Session_RestoreWindows(startBook, startCoins, startDiamonds, startNews, startSettings, startTimesales, startLevels, startTicker, startOrders);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
	
    mutex_off(hMutex);
    
    return (int)msg.wParam;
}

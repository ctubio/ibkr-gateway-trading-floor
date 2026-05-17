#include "api.h"
#include "shared.h"
#include "settings.h"
#include "book.h"
#include "diamonds.h"
#include "coins.h"
#include "news.h"
#include "timesales.h"
#include "levels.h"
#include "orders.h"
#include "ticker.h"
#include "dashboard.h"

// Global Hotkey Registrations
// Frameless, Translucent, and Non-Rectangular UI
// Window "Click-Through" Overlays
// Modern Window Corner Rounding
// Immersive Dark Mode
// Taskbar Extensions
// Rich Canvas Visualizations & Direct Manipulation

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
	startDashboard(hInst);

    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = g_AppWindows[DASHBOARD_CLASS_NAME];
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = hIconOffline;
    lstrcpy(nid.szTip, "Offline");
    Shell_NotifyIcon(NIM_ADD, &nid);

    Session_RestoreWindows(startBook, startCoins, startDiamonds, startNews, startSettings, startTimesales, startLevels, startTicker, startOrders);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
	
    mutex_off(hMutex);
    
    return (int)msg.wParam;
}

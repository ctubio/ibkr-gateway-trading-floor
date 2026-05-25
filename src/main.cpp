#include "api/gateway.h"
#include "api/registry.h"
#include "api/sound.h"
#include "api/shared.h"

#include "gui/settings.h"
#include "gui/book.h"
#include "gui/timesales.h"
#include "gui/diamonds.h"
#include "gui/coins.h"
#include "gui/news.h"
#include "gui/levels.h"
#include "gui/orders.h"
#include "gui/ticker.h"
#include "gui/dashboard.h"

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nShow) {
    MutexInstance();

    InitDarkBrushes();
    
    RegisterWindowClass(hInst, WndProcBookNewList,   BOOK_NEW_LIST_CLASS_NAME,  2);
    RegisterWindowClass(hInst, WndProcBook,          BOOK_CLASS_NAME,           2);
    RegisterWindowClass(hInst, WndProcCoins,         COINS_CLASS_NAME,          3);
    RegisterWindowClass(hInst, WndProcDiamonds,      DIAMONDS_CLASS_NAME,       4);
    RegisterWindowClass(hInst, WndProcSettings,      SETTINGS_CLASS_NAME,       5);
    RegisterWindowClass(hInst, WndProcDebugLog,      DEBUGLOG_CLASS_NAME,      11);
    RegisterWindowClass(hInst, WndProcNews,          NEWS_CLASS_NAME,           6);
    RegisterWindowClass(hInst, WndProcNewsArticle,   NEWS_ARTICLE_CLASS_NAME,   6);
    RegisterWindowClass(hInst, WndProcTicker,        TICKER_CLASS_NAME,         9);
    RegisterWindowClass(hInst, WndProcTimesales,     TIMESALES_CLASS_NAME,      7);
    RegisterWindowClass(hInst, WndProcLevels,        LEVELS_CLASS_NAME,         8);
    RegisterWindowClass(hInst, WndProcOrders,        ORDERS_CLASS_NAME,        10);
    RegisterWindowClass(hInst, WndProcDashboard,     DASHBOARD_CLASS_NAME,      1);
	StartDashboard(hInst);

    // Initialize common controls (ListView, Tab, etc.)
    INITCOMMONCONTROLSEX icex = { sizeof(icex), ICC_WIN95_CLASSES | ICC_LISTVIEW_CLASSES | ICC_TAB_CLASSES };
    InitCommonControlsEx(&icex);

    Session_RestoreWindows(
        StartBook,    StartCoins,    StartDiamonds,
        StartNews,    StartSettings, StartTimesales,
        StartLevels,  StartTicker,   StartOrders,
        StartDebugLog
    );

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
	
    return (int)msg.wParam;
}
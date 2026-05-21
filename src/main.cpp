
#include <thread>
#include <mutex>
#include <iostream>
#include <chrono>

#include "EWrapper.h"
#include "EClientSocket.h"
#include "EReaderOSSignal.h"
#include "EReader.h"

#include <windows.h>
#include <mmsystem.h>
#include <shellapi.h>
#include <shobjidl.h>

#include <commctrl.h>

#include <dwmapi.h>
#include <initguid.h>
#include <propkey.h>
#include <propvarutil.h>
#include <mmsystem.h>

#include "registry.h"
#include "debug.h"
#include "sound.h"
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

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nShow) {
    mutex_instance();

    InitDarkBrushes();
    
    registerWindowClass(hInst, HandleCommonMessages, BOOK_NEW_LIST_CLASS_NAME,  2);
    registerWindowClass(hInst, WndProcDebugLog,      DEBUGLOG_CLASS_NAME,       5);
    registerWindowClass(hInst, WndProcBook,          BOOK_CLASS_NAME,           2);
    registerWindowClass(hInst, WndProcCoins,         COINS_CLASS_NAME,          3);
    registerWindowClass(hInst, WndProcDiamonds,      DIAMONDS_CLASS_NAME,       4);
    registerWindowClass(hInst, WndProcSettings,      SETTINGS_CLASS_NAME,       5);
    registerWindowClass(hInst, WndProcNews,          NEWS_CLASS_NAME,           6);
    registerWindowClass(hInst, WndProcTicker,        TICKER_CLASS_NAME,         9);
    registerWindowClass(hInst, WndProcTimesales,     TIMESALES_CLASS_NAME,      7);
    registerWindowClass(hInst, WndProcLevels,        LEVELS_CLASS_NAME,         8);
    registerWindowClass(hInst, WndProcOrders,        ORDERS_CLASS_NAME,        10);
    registerWindowClass(hInst, WndProcDashboard,     DASHBOARD_CLASS_NAME,      1);
	startDashboard(hInst);

    // Initialize common controls (ListView, Tab, etc.)
    INITCOMMONCONTROLSEX icex = { sizeof(icex), ICC_WIN95_CLASSES | ICC_LISTVIEW_CLASSES | ICC_TAB_CLASSES };
    InitCommonControlsEx(&icex);

    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = g_AppWindows[DASHBOARD_CLASS_NAME];
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = offlineIcons[DASHBOARD_CLASS_NAME];
    lstrcpy(nid.szTip, "Offline");
    Shell_NotifyIcon(NIM_ADD, &nid);

    Session_RestoreWindows(startBook, startCoins, startDiamonds, startNews, startSettings, startTimesales, startLevels, startTicker, startOrders, startDebugLog);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
	
    return (int)msg.wParam;
}

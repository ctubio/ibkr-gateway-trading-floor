#include "api/gateway.h"
#include "api/registry.h"
#include "api/sound.h"
#include "api/shared.h"

#include "gui/settings.h"
#include "gui/market.h"
#include "gui/watchlist.h"
#include "gui/diamonds.h"
#include "gui/orders.h"
#include "gui/news.h"
#include "gui/dashboard.h"

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nShow) {
    MutexGatewayInstance();

    RegisterWindowClass(hInst, WndProcDashboard,   DASHBOARD_CLASS_NAME,          101);
    RegisterWindowClass(hInst, WndProcOrders,      ORDERS_CLASS_NAME,             103);
    RegisterWindowClass(hInst, WndProcDiamonds,    DIAMONDS_CLASS_NAME,           104);
    RegisterWindowClass(hInst, WndProcWatchlist,   WATCHLIST_CLASS_NAME,          105);
    RegisterWindowClass(hInst, WndProcNewList,     WATCHLIST_NEW_LIST_CLASS_NAME, 105);
    RegisterWindowClass(hInst, WndProcMarket,      MARKET_CLASS_NAME,             106);
    RegisterWindowClass(hInst, WndProcNews,        NEWS_CLASS_NAME,               107);
    RegisterWindowClass(hInst, WndProcNewsArticle, NEWS_ARTICLE_CLASS_NAME,       107);
    RegisterWindowClass(hInst, WndProcSettings,    SETTINGS_CLASS_NAME,           108);
    RegisterWindowClass(hInst, WndProcDebugLog,    DEBUGLOG_CLASS_NAME,           109);
	StartDashboard(hInst);

    INITCOMMONCONTROLSEX icex = { sizeof(icex), ICC_WIN95_CLASSES | ICC_LISTVIEW_CLASSES | ICC_TAB_CLASSES };
    InitCommonControlsEx(&icex);

    Session_RestoreWindows(StartDiamonds, StartNews, StartSettings, StartMarket, StartWatchlist, StartOrders, StartDebugLog);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
	
    return (int)msg.wParam;
}
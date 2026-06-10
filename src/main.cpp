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
    SetUnhandledExceptionFilter(WindowsCrashHandler);

    try {
        MutexGatewayInstance();
        
        RegisterWindowClass(hInst, WndProcDashboard,   DASHBOARD_CLASS_NAME,          101);
        RegisterWindowClass(hInst, WndProcOrders,      ORDERS_CLASS_NAME,             103);
        RegisterWindowClass(hInst, WndProcEditOrder,   ORDERS_EDIT_CLASS_NAME,        103, true);
        RegisterWindowClass(hInst, WndProcDiamonds,    DIAMONDS_CLASS_NAME,           104);
        RegisterWindowClass(hInst, WndProcWatchlist,   WATCHLIST_CLASS_NAME,          105);
        RegisterWindowClass(hInst, WndProcNewList,     WATCHLIST_NEW_LIST_CLASS_NAME, 105, true);
        RegisterWindowClass(hInst, WndProcMarket,      MARKET_CLASS_NAME,             106);
        RegisterWindowClass(hInst, WndProcTsSearch,    MARKET_SEARCH_CLASS_NAME,      106, true);
        RegisterWindowClass(hInst, WndProcNews,        NEWS_CLASS_NAME,               107);
        RegisterWindowClass(hInst, WndProcNewsArticle, NEWS_ARTICLE_CLASS_NAME,       107);
        RegisterWindowClass(hInst, WndProcSettings,    SETTINGS_CLASS_NAME,           108);
        RegisterWindowClass(hInst, WndProcDebugLog,    DEBUGLOG_CLASS_NAME,           109, true);
        StartDashboard(hInst);

        INITCOMMONCONTROLSEX icex = { sizeof(icex), ICC_WIN95_CLASSES | ICC_LISTVIEW_CLASSES | ICC_TAB_CLASSES | ICC_USEREX_CLASSES };
        InitCommonControlsEx(&icex);

        Session_RestoreWindows(StartDiamonds, StartNews, StartSettings, StartMarket, StartWatchlist, StartOrders, StartDebugLog);

        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }   

        return (int)msg.wParam;
    } catch (const std::exception& e) {
        std::string errMsg = "Unhandled C++ Exception:\n\n" + std::string(e.what());  
        MessageBoxA(NULL, errMsg.c_str(), "Fatal Exception", MB_ICONERROR | MB_OK);
        return 1;
    } catch (...) {
        MessageBoxA(NULL, "Caught an unhandled exception of unknown type!", "Fatal Exception", MB_ICONERROR | MB_OK);
        return 1;
    }
}
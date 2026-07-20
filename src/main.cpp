#include "api/gateway.h"
#include "api/registry.h"
#include "api/sound.h"
#include "api/shared.h"
#include "api/server.h"
#include "api/process.h"
#include "api/sparklines.h"

#include "gui/settings.h"
#include "gui/market.h"
#include "gui/watchlist.h"
#include "gui/diamonds.h"
#include "gui/orders.h"
#include "gui/scanner.h"
#include "gui/dashboard.h"

class RegisterWindowRAII {
    HINSTANCE hInst_;
public:
    explicit RegisterWindowRAII(HINSTANCE hInst) : hInst_(hInst) {
        RegisterWindowClass(hInst_, WndProcDashboard,   DASHBOARD_CLASS_NAME,          101);
        RegisterWindowClass(hInst_, WndProcExchange,    DASHBOARD_EXCHANGE_CLASS_NAME, 106, true);
        RegisterWindowClass(hInst_, WndProcOrders,      ORDERS_CLASS_NAME,             103);
        RegisterWindowClass(hInst_, WndProcDiamonds,    DIAMONDS_CLASS_NAME,           104);
        RegisterWindowClass(hInst_, WndProcWatchlist,   WATCHLIST_CLASS_NAME,          105);
        RegisterWindowClass(hInst_, WndProcNewList,     WATCHLIST_NEW_LIST_CLASS_NAME, 105, true);
        RegisterWindowClass(hInst_, WndProcMarket,      MARKET_CLASS_NAME,             106);
        RegisterWindowClass(hInst_, WndProcTsSearch,    MARKET_SEARCH_CLASS_NAME,      106, true);
        RegisterWindowClass(hInst_, WndProcScanner,     SCANNER_CLASS_NAME,            107);
        RegisterWindowClass(hInst_, WndProcSettings,    SETTINGS_CLASS_NAME,           108);
        RegisterWindowClass(hInst_, WndProcDebugLog,    DEBUGLOG_CLASS_NAME,           109, true);
        
        StartDashboard(hInst_);

        Session_RestoreWindows(StartDiamonds, StartScanner, StartSettings, StartMarket, StartWatchlist, StartOrders, StartDebugLog);
    }
    ~RegisterWindowRAII() {
        UnregisterClass(DASHBOARD_CLASS_NAME, hInst_);
        UnregisterClass(DASHBOARD_EXCHANGE_CLASS_NAME, hInst_);
        UnregisterClass(ORDERS_CLASS_NAME, hInst_);
        UnregisterClass(DIAMONDS_CLASS_NAME, hInst_);
        UnregisterClass(WATCHLIST_CLASS_NAME, hInst_);
        UnregisterClass(WATCHLIST_NEW_LIST_CLASS_NAME, hInst_);
        UnregisterClass(MARKET_CLASS_NAME, hInst_);
        UnregisterClass(MARKET_SEARCH_CLASS_NAME, hInst_);
        UnregisterClass(SCANNER_CLASS_NAME, hInst_);
        UnregisterClass(SETTINGS_CLASS_NAME, hInst_);
        UnregisterClass(DEBUGLOG_CLASS_NAME, hInst_);
    }
};

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nShow) {
    SetUnhandledExceptionFilter(WindowsCrashHandler);

    try {
        MutexGatewayInstance();

        RegisterWindowRAII registerWindowRAII(hInst);

        HttpServerRAII httpServerRAII;

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
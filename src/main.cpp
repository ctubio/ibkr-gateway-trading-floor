#include "api/gateway.h"
#include "api/registry.h"
#include "api/sound.h"
#include "api/process.h"
#include "api/shared.h"
#include "api/server.h"
#include "api/sparklines.h"

#include "gui/settings.h"
#include "gui/alerts.h"
#include "gui/market.h"
#include "gui/diamonds.h"
#include "gui/orders.h"
#include "gui/dashboard.h"

class RegisterWindowRAII {
    HINSTANCE hInst_;
    bool startupOk_ = true;
public:
    explicit RegisterWindowRAII(HINSTANCE hInst) : hInst_(hInst) {
        RegisterWindowClass(hInst_, WndProcDashboard,          DASHBOARD_CLASS_NAME,          101);
        RegisterWindowClass(hInst_, WndProcExchange,           DASHBOARD_EXCHANGE_CLASS_NAME, 106, true);
        RegisterWindowClass(hInst_, WndProcAlerts,             ALERTS_CLASS_NAME,             102, true);
        RegisterWindowClass(hInst_, WndProcAlertNotification,  ALERT_NOTIFY_CLASS_NAME,       102, true);
        RegisterWindowClass(hInst_, WndProcOrders,             ORDERS_CLASS_NAME,             103);
        RegisterWindowClass(hInst_, WndProcDiamonds,           DIAMONDS_CLASS_NAME,           104);
        RegisterWindowClass(hInst_, WndProcMarket,             MARKET_CLASS_NAME,             105);
        RegisterWindowClass(hInst_, WndProcTsSearch,           MARKET_SEARCH_CLASS_NAME,      105, true);
        RegisterWindowClass(hInst_, WndProcSettings,           SETTINGS_CLASS_NAME,           107);
        RegisterWindowClass(hInst_, WndProcDebugLog,           DEBUGLOG_CLASS_NAME,           108, true);
        RegisterWindowClass(hInst_, WndProcLock,               LOCK_CLASS_NAME,               110, true);

        darkMode = Settings_DarkMode();

        // Gate the whole app behind the saved keyword, if one is set. Nothing
        // has been hidden yet (lockHotkeys is still false), so on success we
        // just fall straight into normal startup below -- no toggle/reshow
        // needed. On failure/cancel, skip StartDashboard/Session_RestoreWindows
        // entirely and let WinMain exit via ok().
        if (!Settings_LoadString("Lock", "").empty()) {
            if (!PromptLockAtStartup()) {
                startupOk_ = false;
                return;
            }
        }

        StartDashboard(hInst_);

        Session_RestoreWindows(StartDiamonds, StartSettings, StartMarket, StartOrders, StartDebugLog);
    }
    bool ok() const { return startupOk_; }
    ~RegisterWindowRAII() {
        UnregisterClass(DASHBOARD_CLASS_NAME, hInst_);
        UnregisterClass(DASHBOARD_EXCHANGE_CLASS_NAME, hInst_);
        UnregisterClass(ALERTS_CLASS_NAME, hInst_);
        UnregisterClass(ALERT_NOTIFY_CLASS_NAME, hInst_);
        UnregisterClass(ORDERS_CLASS_NAME, hInst_);
        UnregisterClass(DIAMONDS_CLASS_NAME, hInst_);
        UnregisterClass(MARKET_CLASS_NAME, hInst_);
        UnregisterClass(MARKET_SEARCH_CLASS_NAME, hInst_);
        UnregisterClass(SETTINGS_CLASS_NAME, hInst_);
        UnregisterClass(DEBUGLOG_CLASS_NAME, hInst_);
        UnregisterClass(LOCK_CLASS_NAME, hInst_);
    }
};

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nShow) {
    SetUnhandledExceptionFilter(WindowsCrashHandler);

    try {
        MutexGatewayInstance();

        RegisterWindowRAII registerWindowRAII(hInst);
        if (!registerWindowRAII.ok()) {
            return 0; // wrong/cancelled lock keyword at startup — exit without showing anything
        }

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
#include "api/gateway.h"
#include "api/registry.h"
#include "api/speech.h"
#include "api/sound.h"
#include "api/process.h"
#include "api/sparklines.h"
#include "api/caption.h"
#include "api/shared.h"
#include "api/server.h"
#include "api/decision.h"

#include "gui/settings.h"
#include "gui/alerts.h"
#include "gui/market.h"
#include "gui/diamonds.h"
#include "gui/orders.h"
#include "gui/coins.h"

static const struct WindowClassInfo {
    WNDPROC proc;
    const char* name;
    int icon;
    bool popup;
} windowClasses[] = {
    { WndProcCoins,              COINS_CLASS_NAME,                101, false },
    { WndProcExchange,           COINS_EXCHANGE_CLASS_NAME,       106, true  },
    { WndProcAlertsEditor,       ALERTS_EDITOR_CLASS_NAME,        102, true  },
    { WndProcAlertNotification,  ALERT_NOTIFY_CLASS_NAME,         102, true  },
    { WndProcOrders,             ORDERS_CLASS_NAME,               109, false },
    { WndProcDiamonds,           DIAMONDS_CLASS_NAME,             104, false },
    { WndProcMarket,             MARKET_CLASS_NAME,               105, false },
    { WndProcTsSearch,           MARKET_SEARCH_CLASS_NAME,        105, true  },
    { WndProcSettings,           SETTINGS_CLASS_NAME,             107, false },
    { WndProcDebugLog,           DEBUGLOG_CLASS_NAME,             108, true  },
    { WndProcLock,               LOCK_CLASS_NAME,                 110, true  },
    { WndProcScreenFlashOverlay, SCREEN_FLASH_OVERLAY_CLASS_NAME, 102, true  },
};

class RegisterWindowRAII {
    HINSTANCE hInst_;
    bool allowed = true;
public:
    explicit RegisterWindowRAII(HINSTANCE hInst) : hInst_(hInst) {
        for (const auto& windowClass : windowClasses)
            RegisterWindowClass(hInst_, windowClass.proc, windowClass.name, windowClass.icon, windowClass.popup);

        CreateScreenFlashOverlay(hInst_);

        LoadRegistrySettings();

        if (!lockScreen.empty()) {
            if (!PromptLockAtStartup()) {
                allowed = false;
                return;
            }
        }

        StartCoins(hInst_);

        Session_RestoreWindows(StartDiamonds, StartSettings, StartMarket, StartOrders, StartDebugLog);
    }
    bool unlocked() const { return allowed; }
    ~RegisterWindowRAII() {
        DestroyScreenFlashOverlay();
        for (const auto& windowClass : windowClasses)
            UnregisterClass(windowClass.name, hInst_);
    }
};

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nShow) {
    SetUnhandledExceptionFilter(WindowsCrashHandler);

    try {
        taskbarCreatedMessage = RegisterWindowMessageW(L"TaskbarCreated");
        if (!taskbarCreatedMessage) {
            MessageBoxW(NULL, L"Failed to register the TaskbarCreated window message.", L"Trading Floor", MB_ICONERROR | MB_OK);
            return 1;
        }

        if (!MutexGatewayInstance())
            return 0;

        RegisterWindowRAII registerWindowRAII(hInst);
        if (!registerWindowRAII.unlocked())
            return 0;

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
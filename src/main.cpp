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



#include <windows.h>
#include <exception>
#include <fstream>
#include <string>
#include <sstream>

// 1. Windows Global Crash Filter (Intercepts Access Violations, Null Pointers, Segfaults)
LONG WINAPI WindowsCrashHandler(EXCEPTION_POINTERS* exceptionInfo) {
    DWORD code = exceptionInfo->ExceptionRecord->ExceptionCode;
    std::string errorType = "UNKNOWN CRITICAL EXCEPTION";
    
    switch (code) {
        case EXCEPTION_ACCESS_VIOLATION:
            errorType = "EXCEPTION_ACCESS_VIOLATION (Null pointer dereference or invalid memory access)";
            break;
        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
            errorType = "EXCEPTION_ARRAY_BOUNDS_EXCEEDED (Out of bounds array access)";
            break;
        case EXCEPTION_INT_DIVIDE_BY_ZERO:
            errorType = "EXCEPTION_INT_DIVIDE_BY_ZERO (Division by zero)";
            break;
        case EXCEPTION_STACK_OVERFLOW:
            errorType = "EXCEPTION_STACK_OVERFLOW (Infinite recursion or exhausted stack space)";
            break;
        case 0xE06D7363: // Magical Microsoft C++ Exception Code
            errorType = "Unhandled C++ Exception (thrown via throw keyword, missed by try/catch)";
            break;
    }

    std::stringstream ss;
    ss << "!!! CRITICAL APPLICATION CRASH !!!\n\n"
       << "Exception Code: 0x" << std::hex << code << "\n"
       << "Description: " << errorType << "\n"
       << "Fault Address: 0x" << exceptionInfo->ExceptionRecord->ExceptionAddress << "\n\n"
       << "Check 'crash_log.txt' for further details.";

    // Write to a file in case the message box fails
    std::ofstream logFile("crash_log.txt", std::ios::app);
    if (logFile.is_open()) {
        logFile << "[CRASH] Code: 0x" << std::hex << code << " | Desc: " << errorType << std::endl;
        logFile.close();
    }

    // Alert the developer instantly via Windows Pop-up
    MessageBoxA(NULL, ss.str().c_str(), "Gateway Gateway Fatal Error", MB_ICONERROR | MB_OK);

    // Tell Windows to close the application cleanly now that we handled it
    return EXCEPTION_EXECUTE_HANDLER; 
}



int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nShow) {
    // Register the OS-level exception interceptor immediately on boot
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
        
    } 
    // Catch standard C++ library exceptions (std::out_of_range, std::runtime_error, etc.)
    catch (const std::exception& e) {
        std::string errMsg = "Unhandled C++ Exception:\n\n" + std::string(e.what());
        
        std::ofstream logFile("crash_log.txt", std::ios::app);
        if (logFile.is_open()) { logFile << "[C++ EXCEPTION] " << e.what() << std::endl; }
        
        MessageBoxA(NULL, errMsg.c_str(), "Fatal Exception", MB_ICONERROR | MB_OK);
        return 1;
    } 
    // Catch edge-case thrown items that don't inherit from std::exception
    catch (...) {
        MessageBoxA(NULL, "Caught an unhandled exception of unknown type!", "Fatal Exception", MB_ICONERROR | MB_OK);
        return 1;
    }
}
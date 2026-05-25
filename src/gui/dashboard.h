#pragma once

void StartDashboard(HINSTANCE hInst) { StartGenericWindow(DASHBOARD_CLASS_NAME, "IBKR Gateway: Offline", L"IBKRGatewayClient.Dashboard", 324, 70, hInst); }

#define WM_TRAYICON (WM_USER + 1)

#define TIMER_WATCHDOG 1

#define ID_MB_SYMBOLS    1001
#define ID_MB_COINS      1002
#define ID_MB_DIAMONDS   1003
#define ID_MB_SETTINGS   1004
#define ID_MB_NEWS       1005
#define ID_MB_TICKER     1006
#define ID_MB_TIMESALES  1007
#define ID_MB_LEVELS     1008
#define ID_MB_ORDERS     1009
#define ID_M_SYMBOLS     1010
#define ID_M_COINS       1011
#define ID_M_DIAMONDS    1012
#define ID_M_SETTINGS    1013
#define ID_M_NEWS        1014
#define ID_M_TICKER      1015
#define ID_M_TIMESALES   1016
#define ID_M_LEVELS      1017
#define ID_M_ORDERS      1018
#define ID_M_DASHBOARD   1019

#define ID_M_CONNECT    1100
#define ID_M_DISCONNECT 1101
#define ID_M_EXIT       1102

bool shouldBeConnected = true;

struct IconUpdateContext {
    bool connected;
    const std::unordered_map<std::string, HICON>& onlineIcons;
    const std::unordered_map<std::string, HICON>& offlineIcons;
};

// Toggle Always On Top state for a window by its class name

HWND IsWindowAlwaysOnTop(const char* windowClassName) {
    HWND hWnd = FindWindowA(windowClassName, NULL);
    if (!hWnd) {
        return NULL; // Window not found
    }

    LONG_PTR exStyle = GetWindowLongPtr(hWnd, GWL_EXSTYLE);
    
    if (exStyle & WS_EX_TOPMOST) {
        return hWnd;
    }
    return NULL;
}

void ToggleWindowAlwaysOnTop(const char* windowClassName) {
    HWND hWnd = FindWindowA(windowClassName, NULL);
    if (!hWnd) {
        return; // Window not found
    }
    
    LONG_PTR exStyle = GetWindowLongPtr(hWnd, GWL_EXSTYLE);
    
    if (exStyle & WS_EX_TOPMOST) {
        // Currently always on top, remove it
        SetWindowPos(hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    } else {
        // Not always on top, make it so
        SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    }
}

BOOL CALLBACK IconsEnumWindowsProc(HWND hwnd, LPARAM lParam) {IconUpdateContext* ctx = (IconUpdateContext*)lParam;

    char className[256];
    if (GetClassNameA(hwnd, className, sizeof(className)) > 0) {
        std::string key = className;

        // Safely check if this window's class exists in your icon maps
        auto itOnline = ctx->onlineIcons.find(key);
        auto itOffline = ctx->offlineIcons.find(key);

        if (itOnline != ctx->onlineIcons.end() && itOffline != ctx->offlineIcons.end()) {
            HICON hIcon = ctx->connected ? itOnline->second : itOffline->second;
            
            // Apply icons
            SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
            SendMessage(hwnd, WM_SETICON, ICON_BIG,   (LPARAM)hIcon);
        }
    }

    return TRUE; // Continue reading
}

void BindTrayIcon(HWND hWnd) {
    char className[256];
    GetClassNameA(hWnd, className, sizeof(className));

    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hWnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = offlineIcons[className];
    lstrcpy(nid.szTip, "Offline");
    Shell_NotifyIcon(NIM_ADD, &nid);
}

void UpdateTrayIcon(HWND hWnd) {
    std::string tooltip;
    std::string title = "IBKR Gateway: ";
    bool connected = false;

    if (!api.isConnected()) {
        tooltip = "Offline";
        title  += tooltip;
    } else {
        std::string acc = api.getAccountNumber();
        if (acc.empty()) {
            tooltip = "Connecting...";
            title  += tooltip;
        } else {
            connected = api.isMarketDataConnected() && api.isTradingConnected();
            std::string md = api.isMarketDataConnected() ? "MD✓" : "MD✗";
            std::string tr = api.isTradingConnected()    ? "TR✓" : "TR✗";
            tooltip = acc + " | " + md + " " + tr;
            title   = "Account: " + acc;
        }
    }

    strncpy(nid.szTip, tooltip.c_str(), sizeof(nid.szTip) - 1);
    nid.szTip[sizeof(nid.szTip) - 1] = '\0';
    nid.uFlags = NIF_TIP | NIF_ICON;
    nid.hIcon  = connected ? onlineIcons[DASHBOARD_CLASS_NAME] : offlineIcons[DASHBOARD_CLASS_NAME];
    Shell_NotifyIcon(NIM_MODIFY, &nid);

    IconUpdateContext ctx = { connected, onlineIcons, offlineIcons };
    EnumWindows(IconsEnumWindowsProc, (LPARAM)&ctx);
            
    SetWindowTextA(hWnd, title.c_str());
}

void addButtons(HWND hWnd, HINSTANCE hInst, LPCSTR buttonText, int x, int y, HMENU menuId, int iconId) {
		// Create the button
        HWND hBtn = CreateWindow(
            "BUTTON", buttonText,
            WS_VISIBLE | WS_CHILD | BS_ICON,
            x, y, 26, 26,
            hWnd, menuId, hInst, NULL
        );
        SendMessage(hBtn, BM_SETIMAGE, (WPARAM)IMAGE_ICON,
            (LPARAM)LoadImage(hInst, MAKEINTRESOURCE(iconId), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR));

        // Add tooltip
        HWND hTip = CreateWindowA(TOOLTIPS_CLASS, NULL,
            WS_POPUP | TTS_ALWAYSTIP,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            hWnd, NULL, hInst, NULL);

        TOOLINFOA ti = {};
        ti.cbSize   = sizeof(ti);
        ti.uFlags   = TTF_IDISHWND | TTF_SUBCLASS;
        ti.hwnd     = hWnd;
        ti.uId      = (UINT_PTR)hBtn;
        ti.lpszText = (LPSTR)buttonText;
        SendMessage(hTip, TTM_ADDTOOLA, 0, (LPARAM)&ti);
}

LRESULT CALLBACK WndProcDashboard(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE:	{
            LPCREATESTRUCT pcs = (LPCREATESTRUCT)lParam;
            HINSTANCE hInst = pcs->hInstance;

            int steps = 1;
            int stepz = 0;
            //charts from tradingview
            addButtons(hWnd, hInst, "Coins",          (7 * steps++) + (26 * stepz++) + 1, 7, (HMENU)ID_MB_COINS,     3); // total daily eur usd
            addButtons(hWnd, hInst, "Orders",         (7 * steps++) + (26 * stepz++) + 1, 7, (HMENU)ID_MB_ORDERS,   10); // open, messages, history
            addButtons(hWnd, hInst, "Diamonds",       (7 * steps++) + (26 * stepz++) + 1, 7, (HMENU)ID_MB_DIAMONDS,  4); // portfolio + watchlist
            
            addButtons(hWnd, hInst, "Ticker",     6 + (7 * steps++) + (26 * stepz++) + 1, 7, (HMENU)ID_MB_TICKER,    9);        
            addButtons(hWnd, hInst, "Levels",     6 + (7 * steps++) + (26 * stepz++) + 1, 7, (HMENU)ID_MB_LEVELS,    8);
            addButtons(hWnd, hInst, "Timesales",  6 + (7 * steps++) + (26 * stepz++) + 1, 7, (HMENU)ID_MB_TIMESALES, 7); // 3: 0, 100, 1000
            addButtons(hWnd, hInst, "News",       6 + (7 * steps++) + (26 * stepz++) + 1, 7, (HMENU)ID_MB_NEWS,      6);

            addButtons(hWnd, hInst, "Symbols",   12 + (7 * steps++) + (26 * stepz++) + 1, 7, (HMENU)ID_MB_SYMBOLS,   2);
            addButtons(hWnd, hInst, "Settings",  12 + (7 * steps++) + (26 * stepz++) + 1, 7, (HMENU)ID_MB_SETTINGS,  5);

            api.addApiUpdateWindow(hWnd);
            api.setApiErrorWindow(hWnd);

            BindTrayIcon(hWnd);
                    
            SetTimer(hWnd, TIMER_WATCHDOG, 10000, NULL);
            SendMessage(hWnd, WM_TIMER, TIMER_WATCHDOG, 0);
            break;
        }
        
        case WM_API_UPDATE:
            UpdateTrayIcon(hWnd);
            break;

        case WM_API_ERROR: {
            std::string* msg = (std::string*)lParam;
            LogDebug(msg->c_str());
            delete msg;
            break;
        }

        case WM_TIMER:
            if (wParam == TIMER_WATCHDOG) {
                if (shouldBeConnected && !api.isConnected()) {
                    EnsureGatewayRunning(hWnd);
                    api.connect();
                    UpdateTrayIcon(hWnd);
                } else if (!shouldBeConnected && api.isConnected()) {
                    api.disconnect();
                    UpdateTrayIcon(hWnd);
                }
            }
            break;

        case WM_TRAYICON:
            if (lParam == WM_LBUTTONUP) {
                if (IsWindowVisible(hWnd) && !IsIconic(hWnd)) { 
                    ShowWindow(hWnd, SW_HIDE);
                } else {
                    if (IsIconic(hWnd)) { 
                        ShowWindow(hWnd, SW_RESTORE);
                    } else {
                        ShowWindow(hWnd, SW_SHOW);
                    }
                    
                    SetForegroundWindow(hWnd);
                    SetActiveWindow(hWnd);
                    SetFocus(hWnd);
                }
            }
            else if (lParam == WM_RBUTTONUP) {
                HMENU hMenu = CreatePopupMenu();
                // Determine flags based on current API state

                if (api.isConnected()) {
                    std::string accText = std::string("Account: ") + api.getAccountNumber();
                    AppendMenu(hMenu, (MF_STRING | MF_GRAYED), 0, accText.c_str());
                    AppendMenu(hMenu, MF_STRING, ID_M_DISCONNECT, "Disconnect");
                } else {
                    AppendMenu(hMenu, MF_STRING, ID_M_CONNECT, "Connect");
                }
                
                AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);

                if (FindWindowA(DASHBOARD_CLASS_NAME, NULL)) AppendMenu(hMenu, MF_STRING, ID_M_DASHBOARD, IsWindowAlwaysOnTop(DASHBOARD_CLASS_NAME) ? "[*] Dashboard" : "[ ] Dashboard");
                if (FindWindowA(COINS_CLASS_NAME, NULL))     AppendMenu(hMenu, MF_STRING, ID_M_COINS,     IsWindowAlwaysOnTop(COINS_CLASS_NAME) ? "[*] Coins" : "[ ] Coins");
                if (FindWindowA(DIAMONDS_CLASS_NAME, NULL))  AppendMenu(hMenu, MF_STRING, ID_M_DIAMONDS,  IsWindowAlwaysOnTop(DIAMONDS_CLASS_NAME) ? "[*] Diamonds" : "[ ] Diamonds");
                if (FindWindowA(ORDERS_CLASS_NAME, NULL))    AppendMenu(hMenu, MF_STRING, ID_M_ORDERS,    IsWindowAlwaysOnTop(ORDERS_CLASS_NAME) ? "[*] Orders" : "[ ] Orders");
                if (FindWindowA(TICKER_CLASS_NAME, NULL))    AppendMenu(hMenu, MF_STRING, ID_M_TICKER,    IsWindowAlwaysOnTop(TICKER_CLASS_NAME) ? "[*] Ticker" : "[ ] Ticker");
                if (FindWindowA(LEVELS_CLASS_NAME, NULL))    AppendMenu(hMenu, MF_STRING, ID_M_LEVELS,    IsWindowAlwaysOnTop(LEVELS_CLASS_NAME) ? "[*] Levels" : "[ ] Levels");
                if (FindWindowA(TIMESALES_CLASS_NAME, NULL)) AppendMenu(hMenu, MF_STRING, ID_M_TIMESALES, IsWindowAlwaysOnTop(TIMESALES_CLASS_NAME) ? "[*] Timesales" : "[ ] Timesales");
                if (FindWindowA(NEWS_CLASS_NAME, NULL))      AppendMenu(hMenu, MF_STRING, ID_M_NEWS,      IsWindowAlwaysOnTop(NEWS_CLASS_NAME) ? "[*] News" : "[ ] News");
                if (FindWindowA(BOOK_CLASS_NAME, NULL))      AppendMenu(hMenu, MF_STRING, ID_M_SYMBOLS,   IsWindowAlwaysOnTop(BOOK_CLASS_NAME) ? "[*] Symbols" : "[ ] Symbols");
                if (FindWindowA(SETTINGS_CLASS_NAME, NULL))  AppendMenu(hMenu, MF_STRING, ID_M_SETTINGS,  IsWindowAlwaysOnTop(SETTINGS_CLASS_NAME) ? "[*] Settings" : "[ ] Settings");

                AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
                AppendMenu(hMenu, MF_STRING, ID_M_EXIT, "Exit");

                // Track where the mouse is to pop up the menu right above the tray
                POINT pt;
                GetCursorPos(&pt);
                SetForegroundWindow(hWnd); // Fixes a notorious Win32 menu-focus bug
                
                TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hWnd, NULL);
                DestroyMenu(hMenu);
            }
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case ID_M_CONNECT:
                    shouldBeConnected = true; // Turn the auto-watchdog back on
                    SendMessage(hWnd, WM_TIMER, TIMER_WATCHDOG, 0);
                    break;

                case ID_M_DISCONNECT:
                    shouldBeConnected = false; // Stop the watchdog from auto-reconnecting
                    SendMessage(hWnd, WM_TIMER, TIMER_WATCHDOG, 0);
                    break;

                case ID_M_EXIT:
                    api.disconnect();
                    if (Settings_KillGatewayOnExit())
                        KillGateway();
                    Shell_NotifyIcon(NIM_DELETE, &nid); // Remove icon from tray
                    PostQuitMessage(0);
                    break;
                    
                case ID_MB_SYMBOLS:
                    StartBook();
                    break;
                case ID_MB_COINS:
                    StartCoins();
                    break;
                case ID_MB_DIAMONDS:
                    StartDiamonds();
                    break;
                case ID_MB_NEWS:
                    StartNews();
                    break;
                case ID_MB_TIMESALES:
                    StartTimesales();
                    break;
                case ID_MB_LEVELS:
                    StartLevels();
                    break;
                case ID_MB_TICKER:
                    StartTicker();
                    break;
                case ID_MB_SETTINGS:
                    StartSettings();
                    break;
                case ID_MB_ORDERS:
                    StartOrders();
                    break;        
                case ID_M_DASHBOARD:
                    ToggleWindowAlwaysOnTop(DASHBOARD_CLASS_NAME);
                    break;
                case ID_M_SYMBOLS:
                    ToggleWindowAlwaysOnTop(BOOK_CLASS_NAME);
                    break;
                case ID_M_COINS:
                    ToggleWindowAlwaysOnTop(COINS_CLASS_NAME);
                    break;
                case ID_M_DIAMONDS:
                    ToggleWindowAlwaysOnTop(DIAMONDS_CLASS_NAME);
                    break;
                case ID_M_NEWS:
                    ToggleWindowAlwaysOnTop(NEWS_CLASS_NAME);
                    break;
                case ID_M_TIMESALES:
                    ToggleWindowAlwaysOnTop(TIMESALES_CLASS_NAME);
                    break;
                case ID_M_LEVELS:
                    ToggleWindowAlwaysOnTop(LEVELS_CLASS_NAME);
                    break;
                case ID_M_TICKER:
                    ToggleWindowAlwaysOnTop(TICKER_CLASS_NAME);
                    break;
                case ID_M_SETTINGS:
                    ToggleWindowAlwaysOnTop(SETTINGS_CLASS_NAME);
                    break;
                case ID_M_ORDERS:
                    ToggleWindowAlwaysOnTop(ORDERS_CLASS_NAME);
                    break;
            }
            break;
    }
    return HandleCommonMessages(hWnd, message, wParam, lParam);
}

void MutexInstance() {
    HANDLE hMutex = CreateMutex(NULL, TRUE, "Global\\IBKRGatewayClientMutex_17072025");

    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        HWND existingWnd = FindWindow(DASHBOARD_CLASS_NAME, NULL);
        if (existingWnd) {
            DWORD processId;
            GetWindowThreadProcessId(existingWnd, &processId);
            HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, processId);
            if (hProcess) {
                TerminateProcess(hProcess, 0);
                CloseHandle(hProcess);
            }
        }
        
        if (hMutex) CloseHandle(hMutex);
        // Re-create to own the mutex for this instance
        CreateMutex(NULL, TRUE, "Global\\IBKRGatewayClientMutex_17072025");
    }
}
